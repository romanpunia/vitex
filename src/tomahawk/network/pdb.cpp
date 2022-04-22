#include "pdb.h"
#include <inttypes.h>
#ifdef TH_HAS_POSTGRESQL
#include <libpq-fe.h>
#endif
#define CACHE_MAGIC "92343fa9e7e09897cbf6a0e7095c8abc"

namespace Tomahawk
{
	namespace Network
	{
		namespace PDB
		{
			class ArrayFilter;

			typedef std::function<void(void*, ArrayFilter*, char*, size_t)> FilterCallback;
			typedef std::vector<std::function<void(bool)>> FuturesList;
#ifdef TH_HAS_POSTGRESQL
			class ArrayFilter
			{
			private:
				std::vector<char> Records;
				const char* Source;
				size_t Size;
				size_t Position;
				size_t Dimension;
				bool Subroutine;

			public:
				ArrayFilter(const char* NewSource, size_t NewSize) : Source(NewSource), Size(NewSize), Position(0), Dimension(0), Subroutine(false)
				{
				}
				bool Parse(void* Context, const FilterCallback& Callback)
				{
					TH_ASSERT(Callback, false, "callback should be set");
					if (Source[0] == '[')
					{
						while (Position < Size)
						{
							if (Next().first == '=')
								break;
						}
					}

					bool Quote = false;
					while (Position < Size)
					{
						auto Sym = Next();
						if (Sym.first == '{' && !Quote)
						{
							Dimension++;
							if (Dimension > 1)
							{
								ArrayFilter Subparser(Source + (Position - 1), Size - (Position - 1));
								Subparser.Subroutine = true;
								Callback(Context, &Subparser, nullptr, 0);
								Position += Subparser.Position - 2;
							}
						}
						else if (Sym.first == '}' && !Quote)
						{
							Dimension--;
							if (!Dimension)
							{
								Emplace(Context, Callback, false);
								if (Subroutine)
									return true;
							}
						}
						else if (Sym.first == '"' && !Sym.second)
						{
							if (Quote)
								Emplace(Context, Callback, true);
							Quote = !Quote;
						}
						else if (Sym.first == ',' && !Quote)
							Emplace(Context, Callback, false);
						else
							Records.push_back(Sym.first);
					}

					return Dimension == 0;
				}

			private:
				void Emplace(void* Context, const FilterCallback& Callback, bool Empties)
				{
					TH_ASSERT_V(Callback, "callback should be set");
					if (Records.empty() && !Empties)
						return;

					Callback(Context, nullptr, Records.data(), Records.size());
					Records.clear();
				}
				std::pair<char, bool> Next()
				{
					char V = Source[Position++];
					if (V != '\\')
						return std::make_pair(V, false);

					return std::make_pair(Source[Position++], true);
				}
			};

			static void PQlogMessage(TConnection* Base)
			{
				TH_ASSERT_V(Base != nullptr, "base should be set");
				char* Message = PQerrorMessage(Base);
				if (!Message || Message[0] == '\0')
					return;

				Core::Parser Buffer(Message);
				if (Buffer.Empty())
					return;

				std::string Result;
				Buffer.Erase(Buffer.Size() - 1);

				std::vector<std::string> Errors = Buffer.Split('\n');
				for (auto& Item : Errors)
					Result += "\n\t" + Item;

				TH_ERR("[pqerr] %s", Errors.size() > 1 ? Result.c_str() : Result.c_str() + 2);
			}
			static void PQlogNotice(void*, const char* Message)
			{
				if (!Message || Message[0] == '\0')
					return;

				Core::Parser Buffer(Message);
				if (Buffer.Empty())
					return;

				std::string Result;
				Buffer.Erase(Buffer.Size() - 1);

				std::vector<std::string> Errors = Buffer.Split('\n');
				for (auto& Item : Errors)
					Result += "\n\t" + Item;

				TH_WARN("[pqnotice] %s", Errors.size() > 1 ? Result.c_str() : Result.c_str() + 2);
			}
			static Core::Schema* ToSchema(const char* Data, int Size, unsigned int Id);
			static void ToArrayField(void* Context, ArrayFilter* Subdata, char* Data, size_t Size)
			{
				TH_ASSERT_V(Context != nullptr, "context should be set");
				std::pair<Core::Schema*, Oid>* Base = (std::pair<Core::Schema*, Oid>*)Context;
				if (Subdata != nullptr)
				{
					std::pair<Core::Schema*, Oid> Next;
					Next.first = Core::Var::Set::Array();
					Next.second = Base->second;

					if (!Subdata->Parse(&Next, ToArrayField))
					{
						Base->first->Push(new Core::Schema(Core::Var::Null()));
						TH_RELEASE(Next.first);
					}
					else
						Base->first->Push(Next.first);
				}
				else if (Data != nullptr && Size > 0)
				{
					Core::Parser Result(Data, Size);
					Result.Trim();

					if (Result.Empty())
						return;

					if (Result.R() != "NULL")
						Base->first->Push(ToSchema(Result.Get(), (int)Result.Size(), Base->second));
					else
						Base->first->Push(new Core::Schema(Core::Var::Null()));
				}
			}
			static Core::Variant ToVariant(const char* Data, int Size, unsigned int Id)
			{
				if (!Data)
					return Core::Var::Null();

				OidType Type = (OidType)Id;
				switch (Type)
				{
					case OidType::Char:
					case OidType::Int2:
					case OidType::Int4:
					case OidType::Int8:
					{
						Core::Parser Source(Data, (size_t)Size);
						if (Source.HasInteger())
							return Core::Var::Integer(Source.ToInt64());

						return Core::Var::String(Data, (size_t)Size);
					}
					case OidType::Bool:
					{
						if (Data[0] == 't')
							return Core::Var::Boolean(true);
						else if (Data[0] == 'f')
							return Core::Var::Boolean(false);

						Core::Parser Source(Data, (size_t)Size);
						Source.ToLower();

						bool Value = (Source.R() == "true" || Source.R() == "yes" || Source.R() == "on" || Source.R() == "1");
						return Core::Var::Boolean(Value);
					}
					case OidType::Float4:
					case OidType::Float8:
					{
						Core::Parser Source(Data, (size_t)Size);
						if (Source.HasNumber())
							return Core::Var::Number(Source.ToDouble());

						return Core::Var::String(Data, (size_t)Size);
					}
					case OidType::Money:
					case OidType::Numeric:
						return Core::Var::DecimalString(std::string(Data, (size_t)Size));
					case OidType::Bytea:
					{
						size_t Length = 0;
						unsigned char* Buffer = PQunescapeBytea((const unsigned char*)Data, &Length);
						Core::Variant Result = Core::Var::Base64(Buffer, Length);

						PQfreemem(Buffer);
						return Result;
					}
					case OidType::JSON:
					case OidType::JSONB:
					case OidType::Any_Array:
					case OidType::Name_Array:
					case OidType::Text_Array:
					case OidType::Date_Array:
					case OidType::Time_Array:
					case OidType::UUID_Array:
					case OidType::CString_Array:
					case OidType::BpChar_Array:
					case OidType::VarChar_Array:
					case OidType::Bit_Array:
					case OidType::VarBit_Array:
					case OidType::Char_Array:
					case OidType::Int2_Array:
					case OidType::Int4_Array:
					case OidType::Int8_Array:
					case OidType::Bool_Array:
					case OidType::Float4_Array:
					case OidType::Float8_Array:
					case OidType::Money_Array:
					case OidType::Numeric_Array:
					case OidType::Bytea_Array:
					case OidType::Any:
					case OidType::Name:
					case OidType::Text:
					case OidType::Date:
					case OidType::Time:
					case OidType::UUID:
					case OidType::CString:
					case OidType::BpChar:
					case OidType::VarChar:
					case OidType::Bit:
					case OidType::VarBit:
					default:
						return Core::Var::String(Data, (size_t)Size);
				}
			}
			static Core::Schema* ToArray(const char* Data, int Size, unsigned int Id)
			{
				std::pair<Core::Schema*, Oid> Context;
				Context.first = Core::Var::Set::Array();
				Context.second = Id;

				ArrayFilter Filter(Data, (size_t)Size);
				if (!Filter.Parse(&Context, ToArrayField))
				{
					TH_RELEASE(Context.first);
					return new Core::Schema(Core::Var::String(Data, (size_t)Size));
				}

				return Context.first;
			}
			Core::Schema* ToSchema(const char* Data, int Size, unsigned int Id)
			{
				if (!Data)
					return nullptr;

				OidType Type = (OidType)Id;
				switch (Type)
				{
					case OidType::JSON:
					case OidType::JSONB:
					{
						Core::Schema* Result = Core::Schema::ReadJSON(Data, (size_t)Size);
						if (Result != nullptr)
							return Result;

						return new Core::Schema(Core::Var::String(Data, (size_t)Size));
					}
					case OidType::Any_Array:
						return ToArray(Data, Size, (Oid)OidType::Any);
					case OidType::Name_Array:
						return ToArray(Data, Size, (Oid)OidType::Name);
						break;
					case OidType::Text_Array:
						return ToArray(Data, Size, (Oid)OidType::Name);
						break;
					case OidType::Date_Array:
						return ToArray(Data, Size, (Oid)OidType::Name);
						break;
					case OidType::Time_Array:
						return ToArray(Data, Size, (Oid)OidType::Name);
						break;
					case OidType::UUID_Array:
						return ToArray(Data, Size, (Oid)OidType::UUID);
						break;
					case OidType::CString_Array:
						return ToArray(Data, Size, (Oid)OidType::CString);
						break;
					case OidType::BpChar_Array:
						return ToArray(Data, Size, (Oid)OidType::BpChar);
						break;
					case OidType::VarChar_Array:
						return ToArray(Data, Size, (Oid)OidType::VarChar);
						break;
					case OidType::Bit_Array:
						return ToArray(Data, Size, (Oid)OidType::Bit);
						break;
					case OidType::VarBit_Array:
						return ToArray(Data, Size, (Oid)OidType::VarBit);
						break;
					case OidType::Char_Array:
						return ToArray(Data, Size, (Oid)OidType::Char);
						break;
					case OidType::Int2_Array:
						return ToArray(Data, Size, (Oid)OidType::Int2);
						break;
					case OidType::Int4_Array:
						return ToArray(Data, Size, (Oid)OidType::Int4);
						break;
					case OidType::Int8_Array:
						return ToArray(Data, Size, (Oid)OidType::Int8);
						break;
					case OidType::Bool_Array:
						return ToArray(Data, Size, (Oid)OidType::Bool);
						break;
					case OidType::Float4_Array:
						return ToArray(Data, Size, (Oid)OidType::Float4);
						break;
					case OidType::Float8_Array:
						return ToArray(Data, Size, (Oid)OidType::Float8);
						break;
					case OidType::Money_Array:
						return ToArray(Data, Size, (Oid)OidType::Money);
						break;
					case OidType::Numeric_Array:
						return ToArray(Data, Size, (Oid)OidType::Numeric);
						break;
					case OidType::Bytea_Array:
						return ToArray(Data, Size, (Oid)OidType::Bytea);
						break;
					default:
						return new Core::Schema(ToVariant(Data, Size, Id));
				}
			}
#endif
			std::string Util::InlineArray(Cluster* Client, Core::Schema* Array)
			{
				TH_ASSERT(Client != nullptr, std::string(), "cluster should be set");
				TH_ASSERT(Array != nullptr, std::string(), "array should be set");

				Core::SchemaList Map;
				std::string Def;

				for (auto* Item : Array->GetChilds())
				{
					if (Item->Value.IsObject())
					{
						if (Item->IsEmpty())
							continue;

						Def += '(';
						for (auto* Sub : Item->GetChilds())
						{
							Map.push_back(Sub);
							Def += "?,";
						}
						Def.erase(Def.end() - 1);
						Def += "),";
					}
					else
					{
						Map.push_back(Item);
						Def += "?,";
					}
				}

				std::string Result = PDB::Driver::Emplace(Client, Def, &Map, false);
				if (!Result.empty() && Result.back() == ',')
					Result.erase(Result.end() - 1);

				TH_RELEASE(Array);
				return Result;
			}
			std::string Util::InlineQuery(Cluster* Client, Core::Schema* Where, const std::unordered_set<std::string>& Whitelist, const std::string& Default)
			{
				TH_ASSERT(Client != nullptr, std::string(), "cluster should be set");
				TH_ASSERT(Where != nullptr, std::string(), "array should be set");

				Core::SchemaList Map;
				std::string Allow = "abcdefghijklmnopqrstuvwxyz._", Def;
				for (auto* Statement : Where->GetChilds())
				{
					std::string Op = Statement->Value.GetBlob();
					if (Op == "=" || Op == "<>" || Op == "<=" || Op == "<" || Op == ">" || Op == ">=" || Op == "+" || Op == "-" || Op == "*" || Op == "/" || Op == "(" || Op == ")" || Op == "TRUE" || Op == "FALSE")
						Def += Op;
					else if (Op == "~==")
						Def += " LIKE ";
					else if (Op == "~=")
						Def += " ILIKE ";
					else if (Op == "&")
						Def += " AND ";
					else if (Op == "|")
						Def += " OR ";
					else if (!Op.empty())
					{
						if (Op.front() == '@')
						{
							Op = Op.substr(1);
							if (Whitelist.empty() || Whitelist.find(Op) != Whitelist.end())
							{
								if (Op.find_first_not_of(Allow) == std::string::npos)
									Def += Op;
							}
						}
						else if (!Core::Parser(&Op).HasNumber())
						{
							Def += "?";
							Map.push_back(Statement);
						}
						else
							Def += Op;
					}
				}

				std::string Result = PDB::Driver::Emplace(Client, Def, &Map, false);
				if (Result.empty())
					Result = Default;

				TH_RELEASE(Where);
				return Result;
			}

			Address::Address()
			{
			}
			Address::Address(const std::string& URI)
			{
#ifdef TH_HAS_POSTGRESQL
				char* Message = nullptr;
				PQconninfoOption* Result = PQconninfoParse(URI.c_str(), &Message);
				if (Result != nullptr)
				{
					size_t Index = 0;
					while (Result[Index].keyword != nullptr)
					{
						auto& Info = Result[Index];
						Params[Info.keyword] = Info.val;
						Index++;
					}
					PQconninfoFree(Result);
				}
				else
				{
					if (Message != nullptr)
					{
						TH_ERR("[pq] couldn't parse URI string\n\t%s", Message);
						PQfreemem(Message);
					}
				}
#endif
			}
			void Address::Override(const std::string& Key, const std::string& Value)
			{
				Params[Key] = Value;
			}
			bool Address::Set(AddressOp Key, const std::string& Value)
			{
				std::string Name = GetKeyName(Key);
				if (Name.empty())
					return false;

				Params[Name] = Value;
				return true;
			}
			std::string Address::Get(AddressOp Key) const
			{
				auto It = Params.find(GetKeyName(Key));
				if (It == Params.end())
					return "";

				return It->second;
			}
			std::string Address::GetAddress() const
			{
				std::string Hostname = Get(AddressOp::Ip);
				if (Hostname.empty())
					Hostname = Get(AddressOp::Host);

				return "postgresql://" + Hostname + ':' + Get(AddressOp::Port) + '/';
			}
			const std::unordered_map<std::string, std::string>& Address::Get() const
			{
				return Params;
			}
			std::string Address::GetKeyName(AddressOp Key)
			{
				switch (Key)
				{
					case AddressOp::Host:
						return "host";
					case AddressOp::Ip:
						return "hostaddr";
					case AddressOp::Port:
						return "port";
					case AddressOp::Database:
						return "dbname";
					case AddressOp::User:
						return "user";
					case AddressOp::Password:
						return "password";
					case AddressOp::Timeout:
						return "connect_timeout";
					case AddressOp::Encoding:
						return "client_encoding";
					case AddressOp::Options:
						return "options";
					case AddressOp::Profile:
						return "application_name";
					case AddressOp::Fallback_Profile:
						return "fallback_application_name";
					case AddressOp::KeepAlive:
						return "keepalives";
					case AddressOp::KeepAlive_Idle:
						return "keepalives_idle";
					case AddressOp::KeepAlive_Interval:
						return "keepalives_interval";
					case AddressOp::KeepAlive_Count:
						return "keepalives_count";
					case AddressOp::TTY:
						return "tty";
					case AddressOp::SSL:
						return "sslmode";
					case AddressOp::SSL_Compression:
						return "sslcompression";
					case AddressOp::SSL_Cert:
						return "sslcert";
					case AddressOp::SSL_Root_Cert:
						return "sslrootcert";
					case AddressOp::SSL_Key:
						return "sslkey";
					case AddressOp::SSL_CRL:
						return "sslcrl";
					case AddressOp::Require_Peer:
						return "requirepeer";
					case AddressOp::Require_SSL:
						return "requiressl";
					case AddressOp::KRB_Server_Name:
						return "krbservname";
					case AddressOp::Service:
						return "service";
					default:
						return "";
				}
			}
			const char** Address::CreateKeys() const
			{
				const char** Result = (const char**)TH_MALLOC(sizeof(const char*) * (Params.size() + 1));
				size_t Index = 0;

				for (auto& Key : Params)
					Result[Index++] = Key.first.c_str();

				Result[Index] = nullptr;
				return Result;
			}
			const char** Address::CreateValues() const
			{
				const char** Result = (const char**)TH_MALLOC(sizeof(const char*) * (Params.size() + 1));
				size_t Index = 0;

				for (auto& Key : Params)
					Result[Index++] = Key.second.c_str();

				Result[Index] = nullptr;
				return Result;
			}

			Notify::Notify(TNotify* Base) : Pid(0)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return;

				if (Base->relname != nullptr)
					Name = Base->relname;

				if (Base->extra != nullptr)
					Data = Base->extra;

				Pid = Base->be_pid;
#endif
			}
			Core::Schema* Notify::GetSchema() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (Data.empty())
					return nullptr;

				return Core::Schema::ReadJSON(Data.c_str(), Data.size());
#else
				return nullptr;
#endif
			}
			std::string Notify::GetName() const
			{
				return Name;
			}
			std::string Notify::GetData() const
			{
				return Data;
			}
			int Notify::GetPid() const
			{
				return Pid;
			}

			Column::Column(TResponse* NewBase, size_t fRowIndex, size_t fColumnIndex) : Base(NewBase), RowIndex(fRowIndex), ColumnIndex(fColumnIndex)
			{
			}
			int Column::SetValueText(char* Data, size_t Size)
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Data != nullptr, -1, "data should be set");
				TH_ASSERT(Base != nullptr, -1, "context should be valid");
				TH_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), -1, "row should be valid");
				TH_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), -1, "column should be valid");

				return PQsetvalue(Base, RowIndex, ColumnIndex, Data, Size);
#else
				return -1;
#endif
			}
			std::string Column::GetName() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, std::string(), "context should be valid");
				TH_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), std::string(), "row should be valid");
				TH_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), std::string(), "column should be valid");

				char* Text = PQfname(Base, ColumnIndex);
				if (!Text)
					return std::string();

				return Text;
#else
				return std::string();
#endif
			}
			Core::Variant Column::Get() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return Core::Var::Undefined();

				if (PQgetisnull(Base, RowIndex, ColumnIndex) == 1)
					return Core::Var::Null();

				char* Data = PQgetvalue(Base, RowIndex, ColumnIndex);
				int Size = PQgetlength(Base, RowIndex, ColumnIndex);
				Oid Type = PQftype(Base, ColumnIndex);

				return ToVariant(Data, Size, Type);
#else
				return Core::Var::Undefined();
#endif
			}
			Core::Schema* Column::GetInline() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				if (PQgetisnull(Base, RowIndex, ColumnIndex) == 1)
					return nullptr;

				char* Data = PQgetvalue(Base, RowIndex, ColumnIndex);
				int Size = PQgetlength(Base, RowIndex, ColumnIndex);
				Oid Type = PQftype(Base, ColumnIndex);

				return ToSchema(Data, Size, Type);
#else
				return nullptr;
#endif
			}
			char* Column::GetRaw() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, nullptr, "context should be valid");
				TH_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), nullptr, "row should be valid");
				TH_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), nullptr, "column should be valid");

				return PQgetvalue(Base, RowIndex, ColumnIndex);
#else
				return nullptr;
#endif
			}
			int Column::GetFormatId() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, 0, "context should be valid");
				TH_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), 0, "row should be valid");
				TH_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), 0, "column should be valid");

				return PQfformat(Base, ColumnIndex);
#else
				return 0;
#endif
			}
			int Column::GetModId() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, -1, "context should be valid");
				TH_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), -1, "row should be valid");
				TH_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), -1, "column should be valid");

				return PQfmod(Base, ColumnIndex);
#else
				return -1;
#endif
			}
			int Column::GetTableIndex() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, -1, "context should be valid");
				TH_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), -1, "row should be valid");
				TH_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), -1, "column should be valid");

				return PQftablecol(Base, ColumnIndex);
#else
				return -1;
#endif
			}
			ObjectId Column::GetTableId() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, InvalidOid, "context should be valid");
				TH_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), InvalidOid, "row should be valid");
				TH_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), InvalidOid, "column should be valid");

				return PQftable(Base, ColumnIndex);
#else
				return 0;
#endif
			}
			ObjectId Column::GetTypeId() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, InvalidOid, "context should be valid");
				TH_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), InvalidOid, "row should be valid");
				TH_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), InvalidOid, "column should be valid");

				return PQftype(Base, ColumnIndex);
#else
				return 0;
#endif
			}
			size_t Column::GetIndex() const
			{
				return ColumnIndex;
			}
			size_t Column::GetSize() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return 0;

				int Size = PQnfields(Base);
				if (Size < 0)
					return 0;

				return (size_t)Size;
#else
				return 0;
#endif
			}
			size_t Column::GetRawSize() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, 0, "context should be valid");
				TH_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), 0, "row should be valid");
				TH_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), 0, "column should be valid");

				int Size = PQgetlength(Base, RowIndex, ColumnIndex);
				if (Size < 0)
					return 0;

				return (size_t)Size;
#else
				return 0;
#endif
			}
			Row Column::GetRow() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, Row(nullptr, std::numeric_limits<size_t>::max()), "context should be valid");
				TH_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), Row(nullptr, std::numeric_limits<size_t>::max()), "row should be valid");

				return Row(Base, RowIndex);
#else
				return Row(nullptr, std::numeric_limits<size_t>::max());
#endif
			}
			bool Column::IsNull() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return true;

				return PQgetisnull(Base, RowIndex, ColumnIndex) == 1;
#else
				return true;
#endif
			}

			Row::Row(TResponse* NewBase, size_t fRowIndex) : Base(NewBase), RowIndex(fRowIndex)
			{
			}
			Core::Schema* Row::GetObject() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				int Size = PQnfields(Base);
				if (Size <= 0)
					return Core::Var::Set::Object();

				Core::Schema* Result = Core::Var::Set::Object();
				Result->GetChilds().reserve((size_t)Size);

				for (int j = 0; j < Size; j++)
				{
					char* Name = PQfname(Base, j);
					char* Data = PQgetvalue(Base, RowIndex, j);
					int Count = PQgetlength(Base, RowIndex, j);
					bool Null = PQgetisnull(Base, RowIndex, j) == 1;
					Oid Type = PQftype(Base, j);

					if (!Null)
						Result->Set(Name ? Name : std::to_string(j), ToSchema(Data, Count, Type));
					else
						Result->Set(Name ? Name : std::to_string(j), Core::Var::Null());
				}

				return Result;
#else
				return nullptr;
#endif
			}
			Core::Schema* Row::GetArray() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				int Size = PQnfields(Base);
				if (Size <= 0)
					return Core::Var::Set::Object();

				Core::Schema* Result = Core::Var::Set::Array();
				Result->GetChilds().reserve((size_t)Size);

				for (int j = 0; j < Size; j++)
				{
					char* Data = PQgetvalue(Base, RowIndex, j);
					int Count = PQgetlength(Base, RowIndex, j);
					bool Null = PQgetisnull(Base, RowIndex, j) == 1;
					Oid Type = PQftype(Base, j);
					Result->Push(Null ? Core::Var::Set::Null() : ToSchema(Data, Count, Type));
				}

				return Result;
#else
				return nullptr;
#endif
			}
			size_t Row::GetIndex() const
			{
				return RowIndex;
			}
			size_t Row::GetSize() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return 0;

				int Size = PQnfields(Base);
				if (Size < 0)
					return 0;

				return (size_t)Size;
#else
				return 0;
#endif
			}
			Response Row::GetCursor() const
			{
				return Response(Base);
			}
			Column Row::GetColumn(size_t Index) const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || (int)Index >= PQnfields(Base))
					return Column(Base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				return Column(Base, RowIndex, Index);
#else
				return Column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());
#endif
			}
			Column Row::GetColumn(const char* Name) const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Name != nullptr, Column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()), "name should be set");
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return Column(Base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				int Index = PQfnumber(Base, Name);
				if (Index < 0)
					return Column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				return Column(Base, RowIndex, (size_t)Index);
#else
				return Column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());
#endif
			}
			bool Row::GetColumns(Column* Output, size_t Size) const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Output != nullptr && Size > 0, false, "output should be valid");
				TH_ASSERT(Base != nullptr, false, "context should be valid");
				TH_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), false, "row should be valid");

				Size = std::min(GetSize(), Size);
				for (size_t i = 0; i < Size; i++)
					Output[i] = Column(Base, RowIndex, i);

				return true;
#else
				return false;
#endif
			}

			Response::Response() : Base(nullptr), Error(false)
			{
			}
			Response::Response(TResponse* NewBase) : Base(NewBase), Error(false)
			{
			}
			void Response::Release()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return;

				PQclear(Base);
				Base = nullptr;
#endif
			}
			Core::Schema* Response::GetArrayOfObjects() const
			{
#ifdef TH_HAS_POSTGRESQL
				Core::Schema* Result = Core::Var::Set::Array();
				if (!Base)
					return Result;

				int RowsSize = PQntuples(Base);
				if (RowsSize <= 0)
					return Result;

				int ColumnsSize = PQnfields(Base);
				if (ColumnsSize <= 0)
					return Result;

				std::vector<std::pair<std::string, Oid>> Meta;
				Meta.reserve((size_t)ColumnsSize);

				for (int j = 0; j < ColumnsSize; j++)
				{
					char* Name = PQfname(Base, j);
					Meta.emplace_back(std::make_pair(Name ? Name : std::to_string(j), PQftype(Base, j)));
				}

				Result->Reserve((size_t)RowsSize);
				for (int i = 0; i < RowsSize; i++)
				{
					Core::Schema* Subresult = Core::Var::Set::Object();
					Subresult->GetChilds().reserve((size_t)ColumnsSize);

					for (int j = 0; j < ColumnsSize; j++)
					{
						char* Data = PQgetvalue(Base, i, j);
						int Size = PQgetlength(Base, i, j);
						bool Null = PQgetisnull(Base, i, j) == 1;
						auto& Field = Meta[j];

						if (!Null)
							Subresult->Set(Field.first, ToSchema(Data, Size, Field.second));
						else
							Subresult->Set(Field.first, Core::Var::Null());
					}

					Result->Push(Subresult);
				}

				return Result;
#else
				return Core::Var::Set::Array();
#endif
			}
			Core::Schema* Response::GetArrayOfArrays() const
			{
#ifdef TH_HAS_POSTGRESQL
				Core::Schema* Result = Core::Var::Set::Array();
				if (!Base)
					return Result;

				int RowsSize = PQntuples(Base);
				if (RowsSize <= 0)
					return Result;

				int ColumnsSize = PQnfields(Base);
				if (ColumnsSize <= 0)
					return Result;

				std::vector<Oid> Meta;
				Meta.reserve((size_t)ColumnsSize);

				for (int j = 0; j < ColumnsSize; j++)
					Meta.emplace_back(PQftype(Base, j));

				Result->Reserve((size_t)RowsSize);
				for (int i = 0; i < RowsSize; i++)
				{
					Core::Schema* Subresult = Core::Var::Set::Array();
					Subresult->GetChilds().reserve((size_t)ColumnsSize);

					for (int j = 0; j < ColumnsSize; j++)
					{
						char* Data = PQgetvalue(Base, i, j);
						int Size = PQgetlength(Base, i, j);
						bool Null = PQgetisnull(Base, i, j) == 1;
						Subresult->Push(Null ? Core::Var::Set::Null() : ToSchema(Data, Size, Meta[j]));
					}

					Result->Push(Subresult);
				}

				return Result;
#else
				return Core::Var::Set::Array();
#endif
			}
			Core::Schema* Response::GetObject(size_t Index) const
			{
				return GetRow(Index).GetObject();
			}
			Core::Schema* Response::GetArray(size_t Index) const
			{
				return GetRow(Index).GetArray();
			}
			std::string Response::GetCommandStatusText() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, std::string(), "context should be valid");
				char* Text = PQcmdStatus(Base);
				if (!Text)
					return std::string();

				return Text;
#else
				return std::string();
#endif
			}
			std::string Response::GetStatusText() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, std::string(), "context should be valid");
				char* Text = PQresStatus(PQresultStatus(Base));
				if (!Text)
					return std::string();

				return Text;
#else
				return std::string();
#endif
			}
			std::string Response::GetErrorText() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, std::string(), "context should be valid");
				char* Text = PQresultErrorMessage(Base);
				if (!Text)
					return std::string();

				return Text;
#else
				return std::string();
#endif
			}
			std::string Response::GetErrorField(FieldCode Field) const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, std::string(), "context should be valid");
				int Code;
				switch (Field)
				{
					case FieldCode::Severity:
						Code = PG_DIAG_SEVERITY;
						break;
					case FieldCode::Severity_Nonlocalized:
						Code = PG_DIAG_SEVERITY_NONLOCALIZED;
						break;
					case FieldCode::SQL_State:
						Code = PG_DIAG_SQLSTATE;
						break;
					case FieldCode::Message_Primary:
						Code = PG_DIAG_MESSAGE_PRIMARY;
						break;
					case FieldCode::Message_Detail:
						Code = PG_DIAG_MESSAGE_DETAIL;
						break;
					case FieldCode::Message_Hint:
						Code = PG_DIAG_MESSAGE_HINT;
						break;
					case FieldCode::Statement_Position:
						Code = PG_DIAG_STATEMENT_POSITION;
						break;
					case FieldCode::Internal_Position:
						Code = PG_DIAG_INTERNAL_POSITION;
						break;
					case FieldCode::Internal_Query:
						Code = PG_DIAG_INTERNAL_QUERY;
						break;
					case FieldCode::Context:
						Code = PG_DIAG_CONTEXT;
						break;
					case FieldCode::Schema_Name:
						Code = PG_DIAG_SCHEMA_NAME;
						break;
					case FieldCode::Table_Name:
						Code = PG_DIAG_TABLE_NAME;
						break;
					case FieldCode::Column_Name:
						Code = PG_DIAG_COLUMN_NAME;
						break;
					case FieldCode::Data_Type_Name:
						Code = PG_DIAG_DATATYPE_NAME;
						break;
					case FieldCode::Constraint_Name:
						Code = PG_DIAG_CONSTRAINT_NAME;
						break;
					case FieldCode::Source_File:
						Code = PG_DIAG_SOURCE_FILE;
						break;
					case FieldCode::Source_Line:
						Code = PG_DIAG_SOURCE_LINE;
						break;
					case FieldCode::Source_Function:
						Code = PG_DIAG_SOURCE_FUNCTION;
						break;
					default:
						return std::string();
				}

				char* Text = PQresultErrorField(Base, Code);
				if (!Text)
					return std::string();

				return Text;
#else
				return std::string();
#endif
			}
			int Response::GetNameIndex(const std::string& Name) const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, -1, "context should be valid");
				TH_ASSERT(!Name.empty(), -1, "name should not be empty");

				return PQfnumber(Base, Name.c_str());
#else
				return 0;
#endif
			}
			QueryExec Response::GetStatus() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, QueryExec::Empty_Query, "context should be valid");
				return (QueryExec)PQresultStatus(Base);
#else
				return QueryExec::Empty_Query;
#endif
			}
			ObjectId Response::GetValueId() const
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Base != nullptr, InvalidOid, "context should be valid");
				return PQoidValue(Base);
#else
				return 0;
#endif
			}
			size_t Response::GetAffectedRows() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return 0;

				char* Count = PQcmdTuples(Base);
				if (!Count || Count[0] == '\0')
					return 0;

				Core::Parser Copy(Count);
				if (!Copy.HasInteger())
					return 0;

				return (size_t)Copy.ToInt64();
#else
				return 0;
#endif
			}
			size_t Response::GetSize() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return 0;

				int Size = PQntuples(Base);
				if (Size < 0)
					return 0;

				return (size_t)Size;
#else
				return 0;
#endif
			}
			Row Response::GetRow(size_t Index) const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || (int)Index >= PQntuples(Base))
					return Row(Base, std::numeric_limits<size_t>::max());

				return Row(Base, Index);
#else
				return Row(nullptr, std::numeric_limits<size_t>::max());
#endif
			}
			Row Response::Front() const
			{
				if (IsEmpty())
					return Row(nullptr, std::numeric_limits<size_t>::max());

				return GetRow(0);
			}
			Row Response::Back() const
			{
				if (IsEmpty())
					return Row(nullptr, std::numeric_limits<size_t>::max());

				return GetRow(GetSize() - 1);
			}
			Response Response::Copy() const
			{
#ifdef TH_HAS_POSTGRESQL
				Response Result;
				if (!Base)
					return Result;

				Result.Error = IsError();
				Result.Base = PQcopyResult(Base, PG_COPYRES_ATTRS | PG_COPYRES_TUPLES | PG_COPYRES_EVENTS | PG_COPYRES_NOTICEHOOKS);

				return Result;
#else
				return Response();
#endif
			}
			TResponse* Response::Get() const
			{
				return Base;
			}
			bool Response::IsEmpty() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return true;

				return PQntuples(Base) <= 0;
#else
				return true;
#endif
			}
			bool Response::IsError() const
			{
				if (Error)
					return true;
#ifdef TH_HAS_POSTGRESQL
				QueryExec State = (Base ? GetStatus() : QueryExec::Non_Fatal_Error);
				return State == QueryExec::Fatal_Error || State == QueryExec::Non_Fatal_Error || State == QueryExec::Bad_Response;
#else
				return false;
#endif
			}

			Cursor::Cursor()
			{
			}
			void Cursor::Release()
			{
				for (auto& Item : Base)
					Item.Release();
				Base.clear();
			}
			bool Cursor::OK()
			{
				bool Result = !IsError();
				Release();

				return Result;
			}
			bool Cursor::IsEmpty() const
			{
				if (Base.empty())
					return true;

				for (auto& Item : Base)
				{
					if (!Item.IsEmpty())
						return false;
				}

				return true;
			}
			bool Cursor::IsError() const
			{
				if (Base.empty())
					return true;

				for (auto& Item : Base)
				{
					if (Item.IsError())
						return true;
				}

				return false;
			}
			size_t Cursor::Size() const
			{
				return Base.size();
			}
			Cursor Cursor::Copy() const
			{
				Cursor Result;
				if (Base.empty())
					return Result;

				Result.Base = Base;
				for (auto& Item : Result.Base)
					Item = Item.Copy();

				return Result;
			}
			Response Cursor::First() const
			{
				if (Base.empty())
					return Response(nullptr);

				return Base.front();
			}
			Response Cursor::Last() const
			{
				if (Base.empty())
					return Response(nullptr);

				return Base.back();
			}
			Response Cursor::GetResponse(size_t Index) const
			{
				TH_ASSERT(Index < Base.size(), Response(nullptr), "index outside of range");
				return Base[Index];
			}

			TConnection* Connection::GetBase() const
			{
				return Base;
			}
			Socket* Connection::GetStream() const
			{
				return Stream;
			}
			Request* Connection::GetCurrent() const
			{
				return Current;
			}
			QueryState Connection::GetState() const
			{
				return State;
			}
			bool Connection::IsBusy() const
			{
				return Current != nullptr;
			}

			Request::Request(const std::string& Commands) : Command(Commands.begin(), Commands.end())
			{
				Command.emplace_back('\0');
			}
			void Request::Finalize(Cursor& Subresult)
			{
				if (Callback)
					Callback(Subresult);
			}
			const std::vector<char>& Request::GetCommand() const
			{
				return Command;
			}
			Cursor Request::GetResult() const
			{
				return Result;
			}
			uint64_t Request::GetSession() const
			{
				return Session;
			}
			bool Request::IsPending() const
			{
				return Future.IsPending();
			}

			Cluster::Cluster() : Session(0)
			{
				Driver::Create();
			}
			Cluster::~Cluster()
			{
				Update.lock();
#ifdef TH_HAS_POSTGRESQL
				for (auto& Item : Pool)
				{
					Item.first->Clear(false);
					PQfinish(Item.second->Base);
					TH_DELETE(Connection, Item.second);
					TH_DELETE(Socket, Item.first);
				}
#endif
				for (auto* Item : Requests)
				{
					Item->Future = Cursor();
					Item->Result.Release();
					TH_DELETE(Request, Item);
				}

				for (auto& Item : Cache.Objects)
					Item.second.second.Release();

				Update.unlock();
				Driver::Release();
			}
			void Cluster::SetCacheCleanup(uint64_t Interval)
			{
				Cache.CleanupDuration = Interval;
			}
			void Cluster::SetCacheDuration(QueryOp CacheId, uint64_t Duration)
			{
				switch (CacheId)
				{
					case QueryOp::CacheShort:
						Cache.ShortDuration = Duration;
						break;
					case QueryOp::CacheMid:
						Cache.MidDuration = Duration;
						break;
					case QueryOp::CacheLong:
						Cache.LongDuration = Duration;
						break;
					default:
						TH_ASSERT_V(false, "cache id should be valid cache category");
						break;
				}
			}
			uint64_t Cluster::AddChannel(const std::string& Name, const OnNotification& NewCallback)
			{
				TH_ASSERT(NewCallback != nullptr, 0, "callback should be set");

				uint64_t Id = Channel++;
				Update.lock();
				Listeners[Name][Id] = NewCallback;
				Update.unlock();

				return Id;
			}
			bool Cluster::RemoveChannel(const std::string& Name, uint64_t Id)
			{
				Update.lock();
				auto& Base = Listeners[Name];
				auto It = Base.find(Id);
				if (It == Base.end())
				{
					Update.unlock();
					return false;
				}

				Base.erase(It);
				Update.unlock();
				return true;
			}
			Core::Async<uint64_t> Cluster::TxBegin()
			{
				return TxBegin("BEGIN");
			}
			Core::Async<uint64_t> Cluster::TxBegin(const std::string& Command)
			{
				uint64_t Token = Session++;
				if (!Token)
					Token = Session++;

				return Query(Command, 0, Token).Then<uint64_t>([this, Token](Cursor&& Result)
				{
					if (Result.OK())
						return Token;

					Commit(Token);
					return uint64_t(0);
				});
			}
			Core::Async<bool> Cluster::TxEnd(const std::string& Command, uint64_t Token)
			{
				return Query(Command, 0, Token).Then<bool>([this, Token](Cursor&& Result)
				{
					Commit(Token);
					return Result.OK();
				});
			}
			Core::Async<bool> Cluster::TxCommit(uint64_t Token)
			{
				return TxEnd("COMMIT", Token);
			}
			Core::Async<bool> Cluster::TxRollback(uint64_t Token)
			{
				return TxEnd("ROLLBACK", Token);
			}
			Core::Async<bool> Cluster::Connect(const Address& URI, size_t Connections)
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(Connections > 0, false, "connections count should be at least 1");
				Update.lock();
				Source = URI;

				if (!Pool.empty())
				{
					Update.unlock();
					return Disconnect().Then<Core::Async<bool>>([this, URI, Connections](bool)
					{
						return this->Connect(URI, Connections);
					});
				}

				Update.unlock();
				return Core::Async<bool>::Execute([this, Connections](Core::Async<bool>& Future)
				{
					TH_PPUSH("postgres-conn", TH_PERF_MAX);
					const char** Keys = Source.CreateKeys();
					const char** Values = Source.CreateValues();

					Update.lock();
					for (size_t i = 0; i < Connections; i++)
					{
						TH_TRACE("[pq] try connect on group %i/%i", (int)i, (int)Connections);
						TConnection* Base = PQconnectdbParams(Keys, Values, 0);
						if (!Base || PQstatus(Base) != ConnStatusType::CONNECTION_OK)
						{
							Update.unlock();
							TH_FREE(Keys);
							TH_FREE(Values);

							if (Base != nullptr)
							{
								PQlogMessage(Base);
								PQfinish(Base);
							}

							Future = false;
							TH_PPOP();
							return;
						}

						TH_TRACE("[pq] OK connect on group %i as 0x%" PRIXPTR, (int)i, (uintptr_t)Base);
						PQsetnonblocking(Base, 1);
						PQsetNoticeProcessor(Base, PQlogNotice, nullptr);
						PQlogMessage(Base);

						Connection* Next = TH_NEW(Connection);
						Next->Stream = TH_NEW(Socket, (socket_t)PQsocket(Base));
						Next->Base = Base;
						Next->Current = nullptr;
						Next->State = QueryState::Idle;

						Pool.insert(std::make_pair(Next->Stream, Next));
						Reprocess(Next);
					}
					Update.unlock();

					Future = true;
					TH_FREE(Keys);
					TH_FREE(Values);
					TH_PPOP();
				});
#else
				return false;
#endif
			}
			Core::Async<bool> Cluster::Disconnect()
			{
#ifdef TH_HAS_POSTGRESQL
				TH_ASSERT(!Pool.empty(), false, "connection should be established");
				return Core::Async<bool>::Execute([this](Core::Async<bool>& Future)
				{
					Update.lock();
					for (auto& Item : Pool)
					{
						Item.first->Clear(false);
						PQfinish(Item.second->Base);
						TH_DELETE(Connection, Item.second);
						TH_DELETE(Socket, Item.first);
					}
					Pool.clear();
					Update.unlock();
					Future = true;
				});
#else
				return false;
#endif
			}
			Core::Async<Cursor> Cluster::EmplaceQuery(const std::string& Command, Core::SchemaList* Map, uint64_t Opts, uint64_t Token)
			{
				bool Once = !(Opts & (uint64_t)QueryOp::ReuseArgs);
				return Query(Driver::Emplace(this, Command, Map, Once), Opts, Token);
			}
			Core::Async<Cursor> Cluster::TemplateQuery(const std::string& Name, Core::SchemaArgs* Map, uint64_t Opts, uint64_t Token)
			{
				TH_TRACE("[pq] template query %s", Name.empty() ? "empty-query-name" : Name.c_str());

				bool Once = !(Opts & (uint64_t)QueryOp::ReuseArgs);
				return Query(Driver::GetQuery(this, Name, Map, Once), Opts, Token);
			}
			Core::Async<Cursor> Cluster::Query(const std::string& Command, uint64_t Opts, uint64_t Token)
			{
				TH_ASSERT(!Command.empty(), Cursor(), "command should not be empty");

				std::string Reference;
				if (Opts & (uint64_t)QueryOp::CacheShort || Opts & (uint64_t)QueryOp::CacheMid || Opts & (uint64_t)QueryOp::CacheLong)
				{
					Cursor Result;
					Reference = GetCacheOid(Command, Opts);

					if (GetCache(Reference, &Result))
					{
						Driver::LogQuery(Command);
						TH_TRACE("[pq] OK execute on NULL (memory-cache)");

						return Result;
					}
				}

				Request* Next = TH_NEW(Request, Command);
				Next->Session = Token;

				if (!Reference.empty())
				{
					Next->Callback = [this, Reference, Opts](Cursor& Data)
					{
						SetCache(Reference, &Data, Opts);
					};
				}

				Core::Async<Cursor> Future = Next->Future;
				Update.lock();
				Requests.push_back(Next);
				for (auto& Item : Pool)
				{
					if (Consume(Item.second))
						break;
				}
				Update.unlock();

				Driver::LogQuery(Command);
				return Future;
			}
			TConnection* Cluster::GetConnection(QueryState State)
			{
				Update.lock();
				for (auto& Item : Pool)
				{
					if (Item.second->State == State)
					{
						TConnection* Base = Item.second->Base;
						Update.unlock();
						return Base;
					}
				}

				Update.unlock();
				return nullptr;
			}
			TConnection* Cluster::GetConnection() const
			{
				for (auto& Item : Pool)
					return Item.second->Base;

				return nullptr;
			}
			std::string Cluster::GetCacheOid(const std::string& Payload, uint64_t Opts)
			{
				std::string Reference = Compute::Common::HexEncode(Compute::Common::HMAC(Compute::Digests::SHA256(), Payload, CACHE_MAGIC));
				if (Opts & (uint64_t)QueryOp::CacheShort)
					Reference.append(".s");
				else if (Opts & (uint64_t)QueryOp::CacheMid)
					Reference.append(".m");
				else if (Opts & (uint64_t)QueryOp::CacheLong)
					Reference.append(".l");

				return Reference;
			}
			bool Cluster::IsConnected() const
			{
				return !Pool.empty();
			}
			bool Cluster::GetCache(const std::string& CacheOid, Cursor* Data)
			{
				TH_ASSERT(!CacheOid.empty(), false, "cache oid should not be empty");
				TH_ASSERT(Data != nullptr, false, "cursor should be set");

				Cache.Context.lock();
				auto It = Cache.Objects.find(CacheOid);
				if (It != Cache.Objects.end())
				{
					int64_t Time = time(nullptr);
					if (It->second.first < Time)
					{
						*Data = std::move(It->second.second);
						Cache.Objects.erase(It);
					}
					else
						*Data = It->second.second.Copy();

					Cache.Context.unlock();
					return true;
				}

				Cache.Context.unlock();
				return false;
			}
			void Cluster::SetCache(const std::string& CacheOid, Cursor* Data, uint64_t Opts)
			{
				TH_ASSERT_V(!CacheOid.empty(), "cache oid should not be empty");
				TH_ASSERT_V(Data != nullptr, "cursor should be set");

				int64_t Time = time(nullptr);
				int64_t Timeout = Time;
				if (Opts & (uint64_t)QueryOp::CacheShort)
					Timeout += Cache.ShortDuration;
				else if (Opts & (uint64_t)QueryOp::CacheMid)
					Timeout += Cache.MidDuration;
				else if (Opts & (uint64_t)QueryOp::CacheLong)
					Timeout += Cache.LongDuration;

				Cache.Context.lock();
				if (Cache.NextCleanup < Time)
				{
					Cache.NextCleanup = Time + Cache.CleanupDuration;
					for (auto It = Cache.Objects.begin(); It != Cache.Objects.end();)
					{
						if (It->second.first < Time)
						{
							It->second.second.Release();
							It = Cache.Objects.erase(It);
						}
						else
							++It;
					}
				}

				auto It = Cache.Objects.find(CacheOid);
				if (It != Cache.Objects.end())
				{
					It->second.second.Release();
					It->second.second = Data->Copy();
					It->second.first = Timeout;
				}
				else
					Cache.Objects[CacheOid] = std::make_pair(Timeout, Data->Copy());
				Cache.Context.unlock();
			}
			void Cluster::Commit(uint64_t Token)
			{
				Update.lock();
				for (auto& Item : Pool)
				{
					if (Item.second->Session == Token)
					{
						TH_TRACE("[pq] end tx-%llu on 0x%" PRIXPTR, Token, (uintptr_t)Item.second);
						Item.second->Session = 0;
						break;
					}
				}
				Update.unlock();
			}
			bool Cluster::Reestablish(Connection* Target)
			{
#ifdef TH_HAS_POSTGRESQL
				const char** Keys = Source.CreateKeys();
				const char** Values = Source.CreateValues();

				Update.lock();
				if (Target->Current != nullptr)
				{
					Request* Current = Target->Current;
					Current->Result.Release();
					Target->Current = nullptr;

					Update.unlock();
					Current->Future = Cursor();
					Update.lock();

					TH_ERR("[pqerr] query reset on 0x%" PRIXPTR "\n\tconnection lost", (uintptr_t)Target->Base);
					TH_DELETE(Request, Current);
				}

				Target->Stream->Clear(false);
				PQfinish(Target->Base);

				TH_TRACE("[pq] try reconnect on 0x%" PRIXPTR, (uintptr_t)Target->Base);
				Target->Base = PQconnectdbParams(Keys, Values, 0);
				TH_FREE(Keys);
				TH_FREE(Values);

				if (!Target->Base || PQstatus(Target->Base) != ConnStatusType::CONNECTION_OK)
				{
					Update.unlock();
					if (Target != nullptr)
						PQlogMessage(Target->Base);
		
					Core::Schedule::Get()->SetTask([this, Target]()
					{
						Reestablish(Target);
					}, Core::Difficulty::Heavy);
					return false;
				}

				TH_TRACE("[pq] OK reconnect on 0x%" PRIXPTR, (uintptr_t)Target->Base);
				Target->State = QueryState::Idle;
				Target->Stream->SetFd((socket_t)PQsocket(Target->Base));
				PQsetnonblocking(Target->Base, 1);
				PQsetNoticeProcessor(Target->Base, PQlogNotice, nullptr);
				Consume(Target);
				Update.unlock();
				
				return Reprocess(Target);
#else
				return false;
#endif
			}
			bool Cluster::Consume(Connection* Base)
			{
#ifdef TH_HAS_POSTGRESQL
				if (Base->State != QueryState::Idle || Requests.empty())
					return false;

				Request* Next = Requests.front();
				if (!Transact(Base, Next))
					return false;

				Base->Current = Next;
				Requests.erase(Requests.begin());
				if (!Base->Current)
					return false;

				TH_PPUSH("postgres-send", TH_PERF_MAX);
				if (Base->Session != 0)
					TH_TRACE("[pq] execute query on 0x%" PRIXPTR " tx-%llu\n\t%.64s%s", (uintptr_t)Base, Base->Session, Base->Current->Command.data(), Base->Current->Command.size() > 64 ? " ..." : "");
				else
					TH_TRACE("[pq] execute query on 0x%" PRIXPTR "\n\t%.64s%s", (uintptr_t)Base, Base->Current->Command.data(), Base->Current->Command.size() > 64 ? " ..." : "");
				
				if (PQsendQuery(Base->Base, Base->Current->Command.data()) == 1)
					TH_PRET(Flush(Base, false));

				Request* Item = Base->Current;
				Base->Current = nullptr;
				PQlogMessage(Base->Base);

				Update.unlock();
				Item->Future = Cursor();
				Update.lock();

				Item->Result.Release();
				TH_DELETE(Request, Item);
				TH_PPOP();

				return true;
#else
				return false;
#endif
			}
			bool Cluster::Reprocess(Connection* Source)
			{
				return Source->Stream->SetReadNotify([this, Source](NetEvent Event, const char*, size_t)
				{
					if (Packet::IsSkip(Event))
						return true;

					return Dispatch(Source, !Packet::IsError(Event));
				}) == 1;
			}
			bool Cluster::Flush(Connection* Base, bool Blocked)
			{
#ifdef TH_HAS_POSTGRESQL
				Base->State = QueryState::Busy;
				if (PQflush(Base->Base) == 1)
				{
					Base->Stream->Skip((uint32_t)Network::SocketEvent::Read | (uint32_t)Network::SocketEvent::Write, Network::NetEvent::Cancelled);
					return Base->Stream->SetWriteNotify([this, Base](NetEvent Event, size_t)
					{
						if (Packet::IsSkip(Event))
							return true;

						return Flush(Base, true);
					}) == 1;
				}

				if (Blocked)
					return Reprocess(Base);

				return true;
#else
				return Reprocess(Base);
#endif
			}
			bool Cluster::Dispatch(Connection* Source, bool Connected)
			{
#ifdef TH_HAS_POSTGRESQL
				TH_PPUSH("postgres-recv", TH_PERF_MAX);
				Update.lock();
				if (!Connected)
				{
					Source->State = QueryState::Lost;
					Update.unlock();
					TH_PPOP();

					return Core::Schedule::Get()->SetTask([this, Source]()
					{
						Reestablish(Source);
					}, Core::Difficulty::Heavy) != TH_INVALID_TASK_ID;
				}
			Retry:
				Consume(Source);

				if (PQconsumeInput(Source->Base) != 1)
				{
					PQlogMessage(Source->Base);
					Source->State = QueryState::Lost;
					goto Finalize;
				}

				if (PQisBusy(Source->Base) == 0)
				{
					PGnotify* Notification = PQnotifies(Source->Base);
					if (Notification != nullptr && Notification->relname != nullptr)
					{
						auto It = Listeners.find(Notification->relname);
						if (It != Listeners.end() && !It->second.empty())
						{
							Notify Event(Notification);
							for (auto& Item : It->second)
							{
								OnNotification Callback = Item.second;
								Core::Schedule::Get()->SetTask([Event, Callback = std::move(Callback)]()
								{
									Callback(Event);
								}, Core::Difficulty::Light);
							}
						}
						TH_TRACE("[pq] notification on channel @%s:\n\t%s", Notification->relname, Notification->extra ? Notification->extra : "[payload]");
						PQfreeNotify(Notification);
					}

					if (Source->State == QueryState::Busy)
					{
						Response Frame(PQgetResult(Source->Base));
						if (Source->Current != nullptr)
						{
							if (!Frame)
							{
								Core::Async<Cursor> Future = Source->Current->Future;
								Cursor Results(std::move(Source->Current->Result));
								Request* Item = Source->Current;
								Source->State = QueryState::Idle;
								Source->Current = nullptr;
								PQlogMessage(Source->Base);

								if (!Results.IsError())
									TH_TRACE("[pq] OK execute on 0x%" PRIXPTR, (uintptr_t)Source);

								Update.unlock();
								Item->Finalize(Results);
								Future = std::move(Results);
								Update.lock();
								TH_DELETE(Request, Item);
							}
							else
							{
								Source->Current->Result.Base.emplace_back(Frame);
								goto Retry;
							}
						}
						else
						{
							Source->State = (Frame ? QueryState::Busy : QueryState::Idle);
							Frame.Release();
						}
					}
				}

				Consume(Source);
			Finalize:
				Update.unlock();
				TH_PPOP();

				return Reprocess(Source);
#else
				return false;
#endif
			}
			bool Cluster::Transact(Connection* Base, Request* Next)
			{
				if (Base->Session != 0)
					return Base->Session == Next->Session;

				if (Next->Session == 0)
					return true;

				for (auto& Item : Pool)
				{
					if (Base == Item.second)
						continue;

					if (Item.second->Session == Next->Session)
						return false;
				}

				TH_TRACE("[pq] start tx-%llu on 0x%" PRIXPTR, Next->Session, (uintptr_t)Base);
				Base->Session = Next->Session;
				return true;
			}

			void Driver::Create()
			{
#ifdef TH_HAS_POSTGRESQL
				if (State <= 0)
				{
					Queries = new std::unordered_map<std::string, Sequence>();
					Safe = TH_NEW(std::mutex);
					State = 1;
				}
				else
					State++;
#endif
			}
			void Driver::Release()
			{
#ifdef TH_HAS_POSTGRESQL
				if (State == 1)
				{
					if (Safe != nullptr)
						Safe->lock();

					State = 0;
					if (Queries != nullptr)
					{
						delete Queries;
						Queries = nullptr;
					}

					if (Safe != nullptr)
					{
						Safe->unlock();
						TH_DELETE(mutex, Safe);
						Safe = nullptr;
					}
				}
				else if (State > 0)
					State--;
#endif
			}
			void Driver::SetQueryLog(const OnQueryLog& Callback)
			{
				Logger = Callback;
			}
			void Driver::LogQuery(const std::string& Command)
			{
				if (Logger)
					Logger(Command + '\n');
			}
			bool Driver::AddQuery(const std::string& Name, const char* Buffer, size_t Size)
			{
				TH_ASSERT(Queries && Safe, false, "driver should be initialized");
				TH_ASSERT(!Name.empty(), false, "name should not be empty");
				TH_ASSERT(Buffer, false, "buffer should be set");

				if (!Size)
					return false;

				Sequence Result;
				Result.Request.assign(Buffer, Size);

				Core::Parser Tokens(" \n\r\t\'\"()<>=%&^*/+-,.!?:;");
				Core::Parser Base(&Result.Request);
				Base.Trim();

				uint64_t Args = 0;
				uint64_t Index = 0;
				int64_t Arg = -1;
				bool Spec = false;
				bool Lock = false;

				while (Index < Base.Size())
				{
					char V = Base.R()[Index];
					char L = Base.R()[!Index ? Index : Index - 1];

					if (V == '\'')
					{
						if (Lock)
						{
							if (Spec)
								Spec = false;
							else
								Lock = false;
						}
						else if (Spec)
							Spec = false;
						else
							Lock = true;
						Index++;
					}
					else if (V == '>')
					{
						if (!Spec && Arg >= 0)
						{
							if (Arg < Base.Size())
							{
								Pose Next;
								Next.Escape = (Base.R()[Arg] == '$');
								Next.Negate = (Arg >= 1 ? Base.R()[Arg - 1] == '-' : false);
								Next.Key = Base.R().substr((size_t)Arg + 2, (size_t)Index - (size_t)Arg - 2);

								if (Next.Negate)
									Arg--;

								Next.Offset = (size_t)Arg;
								Result.Positions.push_back(std::move(Next));
								Base.RemovePart(Arg, Index + 1);
								Index -= Index - Arg - 1; Args++;
							}
							else
								Index++;

							Spec = false;
							Arg = -1;
						}
						else if (Spec)
						{
							Spec = false;
							Base.Erase(Index - 1, 1);
						}
						else
							Index++;
					}
					else if ((L == '@' || L == '$') && V == '<')
					{
						if (!Spec && Arg < 0)
						{
							Arg = (!Index ? Index : Index - 1);
							Index++;
						}
						else if (Spec)
						{
							Spec = false;
							Base.Erase(Index - 1, 1);
						}
						else
							Index++;
					}
					else if (Lock && V == '\\')
					{
						Spec = true;
						Index++;
					}
					else if (!Lock && (V == '\n' || V == '\r' || V == '\t' || V == ' '))
					{
						if (!Tokens.Find(L).Found && (Index + 1 < Base.Size() && !Tokens.Find(Base.R()[Index + 1]).Found))
						{
							Base.R()[Index] = ' ';
							Index++;
						}
						else
							Base.Erase(Index, 1);
					}
					else
					{
						Spec = false;
						Index++;
					}
				}

				if (Args < 1)
					Result.Cache = Result.Request;

				Safe->lock();
				(*Queries)[Name] = Result;
				Safe->unlock();

				return true;
			}
			bool Driver::AddDirectory(const std::string& Directory, const std::string& Origin)
			{
				std::vector<Core::ResourceEntry> Entries;
				if (!Core::OS::Directory::Scan(Directory, &Entries))
					return false;

				std::string Path = Directory;
				if (Path.back() != '/' && Path.back() != '\\')
					Path.append(1, '/');

				uint64_t Size = 0;
				for (auto& File : Entries)
				{
					Core::Parser Base(Path + File.Path);
					if (File.Source.IsDirectory)
					{
						AddDirectory(Base.R(), Origin.empty() ? Directory : Origin);
						continue;
					}

					if (!Base.EndsWith(".sql"))
						continue;

					char* Buffer = (char*)Core::OS::File::ReadAll(Base.Get(), &Size);
					if (!Buffer)
						continue;

					Base.Replace(Origin.empty() ? Directory : Origin, "").Replace("\\", "/").Replace(".sql", "");
					if (Base.StartsOf("\\/"))
						Base.Substring(1);

					AddQuery(Base.R(), Buffer, Size);
					TH_FREE(Buffer);
				}

				return true;
			}
			bool Driver::RemoveQuery(const std::string& Name)
			{
				TH_ASSERT(Queries && Safe, false, "driver should be initialized");
				Safe->lock();
				auto It = Queries->find(Name);
				if (It == Queries->end())
				{
					Safe->unlock();
					return false;
				}

				Queries->erase(It);
				Safe->unlock();
				return true;
			}
			std::string Driver::Emplace(Cluster* Base, const std::string& SQL, Core::SchemaList* Map, bool Once)
			{
				if (!Map || Map->empty())
					return SQL;

				TConnection* Remote = Base->GetConnection();
				Core::Parser Buffer(SQL);
				Core::Parser::Settle Set;
				std::string& Src = Buffer.R();
				uint64_t Offset = 0;
				size_t Next = 0;

				while ((Set = Buffer.Find('?', Offset)).Found)
				{
					if (Next >= Map->size())
					{
						TH_ERR("[pq] emplace query %.64s\n\texpects: at least %llu args", SQL.c_str(), (uint64_t)(Next)+1);
						break;
					}

					bool Escape = true, Negate = false;
					if (Set.Start > 0)
					{
						if (Src[Set.Start - 1] == '\\')
						{
							Offset = Set.Start;
							Buffer.Erase(Set.Start - 1, 1);
							continue;
						}
						else if (Src[Set.Start - 1] == '$')
						{
							if (Set.Start > 1 && Src[Set.Start - 2] == '-')
							{
								Negate = true;
								Set.Start--;
							}

							Escape = false;
							Set.Start--;
						}
						else if (Src[Set.Start - 1] == '-')
						{
							Negate = true;
							Set.Start--;
						}
					}
					std::string Value = GetSQL(Remote, (*Map)[Next++], Escape, Negate);
					Buffer.Erase(Set.Start, (Escape ? 1 : 2));
					Buffer.Insert(Value, Set.Start);
					Offset = Set.Start + (uint64_t)Value.size();
				}

				if (!Once)
					return Src;

				for (auto* Item : *Map)
					TH_RELEASE(Item);
				Map->clear();

				return Src;
			}
			std::string Driver::GetQuery(Cluster* Base, const std::string& Name, Core::SchemaArgs* Map, bool Once)
			{
				TH_ASSERT(Queries && Safe, std::string(), "driver should be initialized");
				Safe->lock();
				auto It = Queries->find(Name);
				if (It == Queries->end())
				{
					Safe->unlock();
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							TH_RELEASE(Item.second);
						Map->clear();
					}

					TH_ERR("[pq] template query %s does not exist", Name.c_str());
					return std::string();
				}

				if (!It->second.Cache.empty())
				{
					std::string Result = It->second.Cache;
					Safe->unlock();

					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							TH_RELEASE(Item.second);
						Map->clear();
					}

					return Result;
				}

				if (!Map || Map->empty())
				{
					std::string Result = It->second.Request;
					Safe->unlock();

					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							TH_RELEASE(Item.second);
						Map->clear();
					}

					return Result;
				}

				TConnection* Remote = Base->GetConnection();
				Sequence Origin = It->second;
				size_t Offset = 0;
				Safe->unlock();

				Core::Parser Result(&Origin.Request);
				for (auto& Word : Origin.Positions)
				{
					auto It = Map->find(Word.Key);
					if (It == Map->end())
					{
						TH_ERR("[pq] template query %s\n\texpects parameter: %s", Name.c_str(), Word.Key.c_str());
						continue;
					}

					std::string Value = GetSQL(Remote, It->second, Word.Escape, Word.Negate);
					if (Value.empty())
						continue;

					Result.Insert(Value, (uint64_t)Word.Offset + Offset);
					Offset += Value.size();
				}

				if (Once)
				{
					for (auto& Item : *Map)
						TH_RELEASE(Item.second);
					Map->clear();
				}

				std::string Data = Origin.Request;
				if (Data.empty())
					TH_ERR("[pq] could not construct query: \"%s\"", Name.c_str());

				return Data;
			}
			std::vector<std::string> Driver::GetQueries()
			{
				std::vector<std::string> Result;
				TH_ASSERT(Queries && Safe, Result, "driver should be initialized");

				Safe->lock();
				Result.reserve(Queries->size());
				for (auto& Item : *Queries)
					Result.push_back(Item.first);
				Safe->unlock();

				return Result;
			}
			std::string Driver::GetCharArray(TConnection* Base, const std::string& Src)
			{
#ifdef TH_HAS_POSTGRESQL
				if (Src.empty())
					return "''";

				if (!Base)
				{
					Core::Parser Dest(Src);
					return Dest.Insert('\'', 0).Append('\'').R();
				}

				char* Subresult = PQescapeLiteral(Base, Src.c_str(), Src.size());
				std::string Result(Subresult);
				PQfreemem(Subresult);

				return Result;
#else
				return "'" + Src + "'";
#endif
			}
			std::string Driver::GetByteArray(TConnection* Base, const char* Src, size_t Size)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Src || !Size)
					return "''";

				if (!Base)
					return "'\\x" + Compute::Common::HexEncode(Src, Size) + "'::bytea";

				size_t Length = 0;
				char* Subresult = (char*)PQescapeByteaConn(Base, (unsigned char*)Src, Size, &Length);
				std::string Result(Subresult, Length > 1 ? Length - 1 : Length);
				PQfreemem((unsigned char*)Subresult);

				Result.insert(Result.begin(), '\'');
				Result.append("'::bytea");
				return Result;
#else
				return "'\\x" + Compute::Common::HexEncode(Src, Size) + "'::bytea";
#endif
			}
			std::string Driver::GetSQL(TConnection* Base, Core::Schema* Source, bool Escape, bool Negate)
			{
				if (!Source)
					return "NULL";

				Core::Schema* Parent = Source->GetParent();
				switch (Source->Value.GetType())
				{
					case Core::VarType::Object:
					{
						std::string Result;
						Core::Schema::WriteJSON(Source, [&Result](Core::VarForm, const char* Buffer, int64_t Length)
						{
							if (Buffer != nullptr && Length > 0)
								Result.append(Buffer, Length);
						});

						return Escape ? GetCharArray(Base, Result) : Result;
					}
					case Core::VarType::Array:
					{
						std::string Result = (Parent && Parent->Value.GetType() == Core::VarType::Array ? "[" : "ARRAY[");
						for (auto* Node : Source->GetChilds())
							Result.append(GetSQL(Base, Node, Escape, Negate)).append(1, ',');

						if (!Source->IsEmpty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "]";
					}
					case Core::VarType::String:
					{
						if (Escape)
							return GetCharArray(Base, Source->Value.GetBlob());

						return Source->Value.GetBlob();
					}
					case Core::VarType::Integer:
						return std::to_string(Negate ? -Source->Value.GetInteger() : Source->Value.GetInteger());
					case Core::VarType::Number:
					{
						bool Trailing = (Source->Value.GetNumber() != (double)Source->Value.GetInteger());
						std::string Result = std::to_string(Negate ? -Source->Value.GetNumber() : Source->Value.GetNumber());
						return Trailing ? Result : Result + ".0";
					}
					case Core::VarType::Boolean:
						return (Negate ? !Source->Value.GetBoolean() : Source->Value.GetBoolean()) ? "TRUE" : "FALSE";
					case Core::VarType::Decimal:
					{
						Core::Decimal Value = Source->Value.GetDecimal();
						std::string Result = (Negate ? '-' + Value.ToString() : Value.ToString());
						return Result.find('.') != std::string::npos ? Result : Result + ".0";
					}
					case Core::VarType::Base64:
						return GetByteArray(Base, Source->Value.GetString(), Source->Value.GetSize());
					case Core::VarType::Null:
					case Core::VarType::Undefined:
						return "NULL";
					default:
						break;
				}

				return "NULL";
			}
			std::unordered_map<std::string, Driver::Sequence>* Driver::Queries = nullptr;
			std::mutex* Driver::Safe = nullptr;
			std::atomic<bool> Driver::Active(false);
			std::atomic<int> Driver::State(0);
			OnQueryLog Driver::Logger = nullptr;
		}
	}
}
