#include "pdb.h"
#ifdef VI_POSTGRESQL
#include <libpq-fe.h>
#endif
#define CACHE_MAGIC "92343fa9e7e09897cbf6a0e7095c8abc"

namespace Mavi
{
	namespace Network
	{
		namespace PDB
		{
			class ArrayFilter;

			typedef std::function<void(void*, ArrayFilter*, char*, size_t)> FilterCallback;
			typedef Core::Vector<std::function<void(bool)>> FuturesList;
#ifdef VI_POSTGRESQL
			class ArrayFilter
			{
			private:
				Core::Vector<char> Records;
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
					VI_ASSERT(Callback, "callback should be set");
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
					VI_ASSERT(Callback, "callback should be set");
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
				VI_ASSERT(Base != nullptr, "base should be set");
				char* Message = PQerrorMessage(Base);
				if (!Message || Message[0] == '\0')
					return;

				Core::String Buffer(Message);
				if (Buffer.empty())
					return;

				Core::String Result;
				Buffer.erase(Buffer.size() - 1);

				Core::Vector<Core::String> Errors = Core::Stringify::Split(Buffer, '\n');
				for (auto& Item : Errors)
					Result += ", " + Item;

				VI_ERR("[pqerr] %s", Errors.size() > 1 ? Result.c_str() : Result.c_str() + 2);
			}
			static void PQlogNotice(void*, const char* Message)
			{
				if (!Message || Message[0] == '\0')
					return;

				Core::String Buffer(Message);
				if (Buffer.empty())
					return;

				Core::String Result;
				Buffer.erase(Buffer.size() - 1);

				Core::Vector<Core::String> Errors = Core::Stringify::Split(Buffer, '\n');
				for (auto& Item : Errors)
					Result += ", " + Item;

				VI_WARN("[pqnotice] %s", Errors.size() > 1 ? Result.c_str() : Result.c_str() + 2);
			}
			static Core::Schema* ToSchema(const char* Data, int Size, unsigned int Id);
			static void ToArrayField(void* Context, ArrayFilter* Subdata, char* Data, size_t Size)
			{
				VI_ASSERT(Context != nullptr, "context should be set");
				std::pair<Core::Schema*, Oid>* Base = (std::pair<Core::Schema*, Oid>*)Context;
				if (Subdata != nullptr)
				{
					std::pair<Core::Schema*, Oid> Next;
					Next.first = Core::Var::Set::Array();
					Next.second = Base->second;

					if (!Subdata->Parse(&Next, ToArrayField))
					{
						Base->first->Push(new Core::Schema(Core::Var::Null()));
						VI_RELEASE(Next.first);
					}
					else
						Base->first->Push(Next.first);
				}
				else if (Data != nullptr && Size > 0)
				{
					Core::String Result(Data, Size);
					Core::Stringify::Trim(Result);

					if (Result.empty())
						return;

					if (Result != "NULL")
						Base->first->Push(ToSchema(Result.c_str(), (int)Result.size(), Base->second));
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
						Core::String Source(Data, (size_t)Size);
						if (Core::Stringify::HasInteger(Source))
							return Core::Var::Integer(*Core::FromString<int64_t>(Source));

						return Core::Var::String(Data, (size_t)Size);
					}
					case OidType::Bool:
					{
						if (Data[0] == 't')
							return Core::Var::Boolean(true);
						else if (Data[0] == 'f')
							return Core::Var::Boolean(false);

						Core::String Source(Data, (size_t)Size);
						Core::Stringify::ToLower(Source);

						bool Value = (Source == "true" || Source == "yes" || Source == "on" || Source == "1");
						return Core::Var::Boolean(Value);
					}
					case OidType::Float4:
					case OidType::Float8:
					{
						Core::String Source(Data, (size_t)Size);
						if (Core::Stringify::HasNumber(Source))
							return Core::Var::Number(*Core::FromString<double>(Source));

						return Core::Var::String(Data, (size_t)Size);
					}
					case OidType::Money:
					case OidType::Numeric:
						return Core::Var::DecimalString(Core::String(Data, (size_t)Size));
					case OidType::Bytea:
					{
						size_t Length = 0;
						unsigned char* Buffer = PQunescapeBytea((const unsigned char*)Data, &Length);
						Core::Variant Result = Core::Var::Binary(Buffer, Length);

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
					VI_RELEASE(Context.first);
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
						auto Result = Core::Schema::ConvertFromJSON(Data, (size_t)Size);
						if (Result)
							return *Result;

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
			Address::Address()
			{
			}
			Address::Address(const Core::String& URI)
			{
#ifdef VI_POSTGRESQL
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
						VI_ERR("[pq] cannot parse URI string: %s", Message);
						PQfreemem(Message);
					}
				}
#endif
			}
			void Address::Override(const Core::String& Key, const Core::String& Value)
			{
				Params[Key] = Value;
			}
			bool Address::Set(AddressOp Key, const Core::String& Value)
			{
				Core::String Name = GetKeyName(Key);
				if (Name.empty())
					return false;

				Params[Name] = Value;
				return true;
			}
			Core::String Address::Get(AddressOp Key) const
			{
				auto It = Params.find(GetKeyName(Key));
				if (It == Params.end())
					return "";

				return It->second;
			}
			Core::String Address::GetAddress() const
			{
				Core::String Hostname = Get(AddressOp::Ip);
				if (Hostname.empty())
					Hostname = Get(AddressOp::Host);

				return "postgresql://" + Hostname + ':' + Get(AddressOp::Port) + '/';
			}
			const Core::UnorderedMap<Core::String, Core::String>& Address::Get() const
			{
				return Params;
			}
			Core::String Address::GetKeyName(AddressOp Key)
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
				const char** Result = VI_MALLOC(const char*, sizeof(const char*) * (Params.size() + 1));
				size_t Index = 0;

				for (auto& Key : Params)
					Result[Index++] = Key.first.c_str();

				Result[Index] = nullptr;
				return Result;
			}
			const char** Address::CreateValues() const
			{
				const char** Result = VI_MALLOC(const char*, sizeof(const char*) * (Params.size() + 1));
				size_t Index = 0;

				for (auto& Key : Params)
					Result[Index++] = Key.second.c_str();

				Result[Index] = nullptr;
				return Result;
			}

			Notify::Notify(TNotify* Base) : Pid(0)
			{
#ifdef VI_POSTGRESQL
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
#ifdef VI_POSTGRESQL
				if (Data.empty())
					return nullptr;

				auto Result = Core::Schema::ConvertFromJSON(Data.c_str(), Data.size());
				return Result ? *Result : nullptr;
#else
				return nullptr;
#endif
			}
			Core::String Notify::GetName() const
			{
				return Name;
			}
			Core::String Notify::GetData() const
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
			int Column::Set(const Core::Variant& Value)
			{
				switch (Value.GetType())
				{
					case Core::VarType::Null:
					case Core::VarType::Undefined:
					case Core::VarType::Object:
					case Core::VarType::Array:
					case Core::VarType::Pointer:
						return SetValueText(nullptr, 0);
					case Core::VarType::String:
					case Core::VarType::Binary:
						return SetValueText(Value.GetBlob());
					case Core::VarType::Integer:
					case Core::VarType::Number:
					case Core::VarType::Decimal:
						return SetValueText(Value.GetBlob());
					case Core::VarType::Boolean:
						return SetValueText(Value.GetBoolean() ? "TRUE" : "FALSE");
					default:
						return -1;
				}
			}
			int Column::SetInline(Core::Schema* Value)
			{
				if (!Value)
					return SetValueText(nullptr, 0);

				if (!Value->Value.IsObject())
					return Set(Value->Value);

				return SetValueText(Core::Schema::ToJSON(Value));
			}
			int Column::SetValueText(const Core::String& Value)
			{
				return SetValueText((char*)Value.c_str(), Value.size());
			}
			int Column::SetValueText(char* Data, size_t Size)
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQsetvalue(Base, (int)RowIndex, (int)ColumnIndex, Data, (int)Size);
#else
				return -1;
#endif
			}
			Core::String Column::GetName() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), "column should be valid");

				char* Text = PQfname(Base, (int)ColumnIndex);
				if (!Text)
					return Core::String();

				return Text;
#else
				return Core::String();
#endif
			}
			Core::Variant Column::Get() const
			{
#ifdef VI_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return Core::Var::Undefined();

				if (PQgetisnull(Base, (int)RowIndex, (int)ColumnIndex) == 1)
					return Core::Var::Null();

				char* Data = PQgetvalue(Base, (int)RowIndex, (int)ColumnIndex);
				int Size = PQgetlength(Base, (int)RowIndex, (int)ColumnIndex);
				Oid Type = PQftype(Base, (int)ColumnIndex);

				return ToVariant(Data, Size, Type);
#else
				return Core::Var::Undefined();
#endif
			}
			Core::Schema* Column::GetInline() const
			{
#ifdef VI_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				if (PQgetisnull(Base, (int)RowIndex, (int)ColumnIndex) == 1)
					return nullptr;

				char* Data = PQgetvalue(Base, (int)RowIndex, (int)ColumnIndex);
				int Size = PQgetlength(Base, (int)RowIndex, (int)ColumnIndex);
				Oid Type = PQftype(Base, (int)ColumnIndex);

				return ToSchema(Data, Size, Type);
#else
				return nullptr;
#endif
			}
			char* Column::GetRaw() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQgetvalue(Base, (int)RowIndex, (int)ColumnIndex);
#else
				return nullptr;
#endif
			}
			int Column::GetFormatId() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQfformat(Base, (int)ColumnIndex);
#else
				return 0;
#endif
			}
			int Column::GetModId() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQfmod(Base, (int)ColumnIndex);
#else
				return -1;
#endif
			}
			int Column::GetTableIndex() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQftablecol(Base, (int)ColumnIndex);
#else
				return -1;
#endif
			}
			ObjectId Column::GetTableId() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQftable(Base, (int)ColumnIndex);
#else
				return 0;
#endif
			}
			ObjectId Column::GetTypeId() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQftype(Base, (int)ColumnIndex);
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
#ifdef VI_POSTGRESQL
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
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), "column should be valid");

				int Size = PQgetlength(Base, (int)RowIndex, (int)ColumnIndex);
				if (Size < 0)
					return 0;

				return (size_t)Size;
#else
				return 0;
#endif
			}
			Row Column::GetRow() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");

				return Row(Base, RowIndex);
#else
				return Row(nullptr, std::numeric_limits<size_t>::max());
#endif
			}
			bool Column::IsNull() const
			{
#ifdef VI_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return true;

				return PQgetisnull(Base, (int)RowIndex, (int)ColumnIndex) == 1;
#else
				return true;
#endif
			}

			Row::Row(TResponse* NewBase, size_t fRowIndex) : Base(NewBase), RowIndex(fRowIndex)
			{
			}
			Core::Schema* Row::GetObject() const
			{
#ifdef VI_POSTGRESQL
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
					char* Data = PQgetvalue(Base, (int)RowIndex, j);
					int Count = PQgetlength(Base, (int)RowIndex, j);
					bool Null = PQgetisnull(Base, (int)RowIndex, j) == 1;
					Oid Type = PQftype(Base, j);

					if (!Null)
						Result->Set(Name ? Name : Core::ToString(j), ToSchema(Data, Count, Type));
					else
						Result->Set(Name ? Name : Core::ToString(j), Core::Var::Null());
				}

				return Result;
#else
				return nullptr;
#endif
			}
			Core::Schema* Row::GetArray() const
			{
#ifdef VI_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				int Size = PQnfields(Base);
				if (Size <= 0)
					return Core::Var::Set::Object();

				Core::Schema* Result = Core::Var::Set::Array();
				Result->GetChilds().reserve((size_t)Size);

				for (int j = 0; j < Size; j++)
				{
					char* Data = PQgetvalue(Base, (int)RowIndex, j);
					int Count = PQgetlength(Base, (int)RowIndex, j);
					bool Null = PQgetisnull(Base, (int)RowIndex, j) == 1;
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
#ifdef VI_POSTGRESQL
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
#ifdef VI_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || (int)Index >= PQnfields(Base))
					return Column(Base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				return Column(Base, RowIndex, Index);
#else
				return Column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());
#endif
			}
			Column Row::GetColumn(const char* Name) const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Name != nullptr, "name should be set");
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
#ifdef VI_POSTGRESQL
				VI_ASSERT(Output != nullptr && Size > 0, "output should be valid");
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");

				Size = std::min(GetSize(), Size);
				for (size_t i = 0; i < Size; i++)
					Output[i] = Column(Base, RowIndex, i);

				return true;
#else
				return false;
#endif
			}

			Response::Response(TResponse* NewBase) : Base(NewBase), Error(false)
			{
			}
			Response::Response(Response&& Other) : Base(Other.Base), Error(Other.Error)
			{
				Other.Base = nullptr;
				Other.Error = false;
			}
			Response::~Response()
			{
#ifdef VI_POSTGRESQL
				if (Base != nullptr)
					PQclear(Base);
				Base = nullptr;
#endif
			}
			Response& Response::operator =(Response&& Other)
			{
				if (&Other == this)
					return *this;

#ifdef VI_POSTGRESQL
				if (Base != nullptr)
					PQclear(Base);
#endif
				Base = Other.Base;
				Error = Other.Error;
				Other.Base = nullptr;
				Other.Error = false;
				return *this;
			}
			Core::Schema* Response::GetArrayOfObjects() const
			{
#ifdef VI_POSTGRESQL
				Core::Schema* Result = Core::Var::Set::Array();
				if (!Base)
					return Result;

				int RowsSize = PQntuples(Base);
				if (RowsSize <= 0)
					return Result;

				int ColumnsSize = PQnfields(Base);
				if (ColumnsSize <= 0)
					return Result;

				Core::Vector<std::pair<Core::String, Oid>> Meta;
				Meta.reserve((size_t)ColumnsSize);

				for (int j = 0; j < ColumnsSize; j++)
				{
					char* Name = PQfname(Base, j);
					Meta.emplace_back(std::make_pair(Name ? Name : Core::ToString(j), PQftype(Base, j)));
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
#ifdef VI_POSTGRESQL
				Core::Schema* Result = Core::Var::Set::Array();
				if (!Base)
					return Result;

				int RowsSize = PQntuples(Base);
				if (RowsSize <= 0)
					return Result;

				int ColumnsSize = PQnfields(Base);
				if (ColumnsSize <= 0)
					return Result;

				Core::Vector<Oid> Meta;
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
			Core::String Response::GetCommandStatusText() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				char* Text = PQcmdStatus(Base);
				if (!Text)
					return Core::String();

				return Text;
#else
				return Core::String();
#endif
			}
			Core::String Response::GetStatusText() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				char* Text = PQresStatus(PQresultStatus(Base));
				if (!Text)
					return Core::String();

				return Text;
#else
				return Core::String();
#endif
			}
			Core::String Response::GetErrorText() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				char* Text = PQresultErrorMessage(Base);
				if (!Text)
					return Core::String();

				return Text;
#else
				return Core::String();
#endif
			}
			Core::String Response::GetErrorField(FieldCode Field) const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
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
						return Core::String();
				}

				char* Text = PQresultErrorField(Base, Code);
				if (!Text)
					return Core::String();

				return Text;
#else
				return Core::String();
#endif
			}
			int Response::GetNameIndex(const Core::String& Name) const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(!Name.empty(), "name should not be empty");

				return PQfnumber(Base, Name.c_str());
#else
				return 0;
#endif
			}
			QueryExec Response::GetStatus() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				return (QueryExec)PQresultStatus(Base);
#else
				return QueryExec::Empty_Query;
#endif
			}
			ObjectId Response::GetValueId() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				return PQoidValue(Base);
#else
				return 0;
#endif
			}
			size_t Response::GetAffectedRows() const
			{
#ifdef VI_POSTGRESQL
				if (!Base)
					return 0;

				char* Count = PQcmdTuples(Base);
				if (!Count || Count[0] == '\0')
					return 0;

				auto Numeric = Core::FromString<uint64_t>(Count);
				if (!Numeric)
					return 0;

				return (size_t)*Numeric;
#else
				return 0;
#endif
			}
			size_t Response::GetSize() const
			{
#ifdef VI_POSTGRESQL
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
#ifdef VI_POSTGRESQL
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
#ifdef VI_POSTGRESQL
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
#ifdef VI_POSTGRESQL
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
#ifdef VI_POSTGRESQL
				QueryExec State = (Base ? GetStatus() : QueryExec::Non_Fatal_Error);
				return State == QueryExec::Fatal_Error || State == QueryExec::Non_Fatal_Error || State == QueryExec::Bad_Response;
#else
				return false;
#endif
			}

			Cursor::Cursor() : Cursor(nullptr, Caching::Never)
			{
			}
			Cursor::Cursor(Connection* NewExecutor, Caching Status) : Executor(NewExecutor), Cache(Status)
			{
				VI_WATCH(this, "pq-result cursor");
			}
			Cursor::Cursor(Cursor&& Other) : Base(std::move(Other.Base)), Executor(Other.Executor), Cache(Other.Cache)
			{
				VI_WATCH(this, "pq-result cursor (moved)");
				Other.Executor = nullptr;
			}
			Cursor::~Cursor()
			{
				VI_UNWATCH(this);
			}
			Cursor& Cursor::operator =(Cursor&& Other)
			{
				if (&Other == this)
					return *this;

				Base = std::move(Other.Base);
				Executor = Other.Executor;
				Cache = Other.Cache;
				Other.Executor = nullptr;

				return *this;
			}
			bool Cursor::IsSuccess() const
			{
				return !IsError();
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
			size_t Cursor::GetSize() const
			{
				return Base.size();
			}
			size_t Cursor::GetAffectedRows() const
			{
				size_t Size = 0;
				for (auto& Item : Base)
					Size += Item.GetAffectedRows();
				return Size;
			}
			Cursor Cursor::Copy() const
			{
				Cursor Result(Executor, Caching::Cached);
				if (Base.empty())
					return Result;

				Result.Base.clear();
				Result.Base.reserve(Base.size());

				for (auto& Item : Base)
					Result.Base.emplace_back(Item.Copy());

				return Result;
			}
			const Response& Cursor::First() const
			{
				VI_ASSERT(!Base.empty(), "index outside of range");
				return Base.front();
			}
			const Response& Cursor::Last() const
			{
				VI_ASSERT(!Base.empty(), "index outside of range");
				return Base.back();
			}
			const Response& Cursor::At(size_t Index) const
			{
				VI_ASSERT(Index < Base.size(), "index outside of range");
				return Base[Index];
			}
			Connection* Cursor::GetExecutor() const
			{
				return Executor;
			}
			Caching Cursor::GetCacheStatus() const
			{
				return Cache;
			}
			Core::Schema* Cursor::GetArrayOfObjects(size_t ResponseIndex) const
			{
				if (ResponseIndex >= Base.size())
					return Core::Var::Set::Array();

				return Base[ResponseIndex].GetArrayOfObjects();
			}
			Core::Schema* Cursor::GetArrayOfArrays(size_t ResponseIndex) const
			{
				if (ResponseIndex >= Base.size())
					return Core::Var::Set::Array();

				return Base[ResponseIndex].GetArrayOfArrays();
			}
			Core::Schema* Cursor::GetObject(size_t ResponseIndex, size_t Index) const
			{
				if (ResponseIndex >= Base.size())
					return nullptr;

				return Base[ResponseIndex].GetObject(Index);
			}
			Core::Schema* Cursor::GetArray(size_t ResponseIndex, size_t Index) const
			{
				if (ResponseIndex >= Base.size())
					return nullptr;

				return Base[ResponseIndex].GetArray(Index);
			}

			Connection::Connection(TConnection* NewBase, socket_t Fd) : Base(NewBase), Stream(new Socket(Fd)), Current(nullptr), State(QueryState::Idle), InSession(false)
			{
			}
			Connection::~Connection() noexcept
			{
				VI_RELEASE(Stream);
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
			bool Connection::IsInSession() const
			{
				return InSession;
			}
			bool Connection::IsBusy() const
			{
				return Current != nullptr;
			}

			Request::Request(const Core::String& Commands, Caching Status) : Command(Commands.begin(), Commands.end()), Time(Core::Schedule::GetClock()), Session(0), Result(nullptr, Status), Options(0)
			{
				Command.emplace_back('\0');
			}
			void Request::Finalize(Cursor& Subresult)
			{
				if (Callback)
					Callback(Subresult);
			}
			void Request::Failure()
			{
				if (Future.IsPending())
					Future.Set(Cursor());
			}
			Cursor&& Request::GetResult()
			{
				return std::move(Result);
			}
			const Core::Vector<char>& Request::GetCommand() const
			{
				return Command;
			}
			SessionId Request::GetSession() const
			{
				return Session;
			}
			uint64_t Request::GetTiming() const
			{
				return (uint64_t)((Core::Schedule::GetClock() - Time).count() / 1000);
			}
			bool Request::IsPending() const
			{
				return Future.IsPending();
			}

			Cluster::Cluster()
			{
				Multiplexer::Get()->Activate();
			}
			Cluster::~Cluster() noexcept
			{
				Core::UMutex<std::mutex> Unique(Update);
#ifdef VI_POSTGRESQL
				for (auto& Item : Pool)
				{
					Item.first->ClearEvents(false);
					PQfinish(Item.second->Base);
					VI_RELEASE(Item.second);
				}
#endif
				for (auto* Item : Requests)
				{
					Item->Failure();
					VI_RELEASE(Item);
				}
				Multiplexer::Get()->Deactivate();
			}
			void Cluster::ClearCache()
			{
				Core::UMutex<std::mutex> Unique(Cache.Context);
				Cache.Objects.clear();
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
						VI_ASSERT(false, "cache id should be valid cache category");
						break;
				}
			}
			void Cluster::SetWhenReconnected(const OnReconnect& NewCallback)
			{
				Core::UMutex<std::mutex> Unique(Update);
				Reconnected = NewCallback;
			}
			uint64_t Cluster::AddChannel(const Core::String& Name, const OnNotification& NewCallback)
			{
				VI_ASSERT(NewCallback != nullptr, "callback should be set");

				uint64_t Id = Channel++;
				Core::UMutex<std::mutex> Unique(Update);
				Listeners[Name][Id] = NewCallback;
				return Id;
			}
			bool Cluster::RemoveChannel(const Core::String& Name, uint64_t Id)
			{
				Core::UMutex<std::mutex> Unique(Update);
				auto& Base = Listeners[Name];
				auto It = Base.find(Id);
				if (It == Base.end())
					return false;

				Base.erase(It);
				return true;
			}
			Core::Promise<SessionId> Cluster::TxBegin(Isolation Type)
			{
				switch (Type)
				{
					case Isolation::Serializable:
						return TxBegin("BEGIN ISOLATION LEVEL SERIALIZABLE");
					case Isolation::RepeatableRead:
						return TxBegin("BEGIN ISOLATION LEVEL REPEATABLE READ");
					case Isolation::ReadUncommited:
						return TxBegin("BEGIN ISOLATION LEVEL READ UNCOMMITTED");
					case Isolation::ReadCommited:
					default:
						return TxBegin("BEGIN");
				}
			}
			Core::Promise<SessionId> Cluster::TxBegin(const Core::String& Command)
			{
				return Query(Command, (size_t)QueryOp::TransactionStart).Then<SessionId>([](Cursor&& Result)
				{
					return Result.IsSuccess() ? Result.GetExecutor() : nullptr;
				});
			}
			Core::Promise<bool> Cluster::TxEnd(const Core::String& Command, SessionId Session)
			{
				return Query(Command, (size_t)QueryOp::TransactionEnd, Session).Then<bool>([](Cursor&& Result)
				{
					return Result.IsSuccess();
				});
			}
			Core::Promise<bool> Cluster::TxCommit(SessionId Session)
			{
				return TxEnd("COMMIT", Session);
			}
			Core::Promise<bool> Cluster::TxRollback(SessionId Session)
			{
				return TxEnd("ROLLBACK", Session);
			}
			Core::Promise<bool> Cluster::Connect(const Address& URI, size_t Connections)
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Connections > 0, "connections count should be at least 1");
				Core::UMutex<std::mutex> Unique(Update);
				Source = URI;

				if (!Pool.empty())
				{
					Unique.Negate();
					return Disconnect().Then<Core::Promise<bool>>([this, URI, Connections](bool)
					{
						return this->Connect(URI, Connections);
					});
				}

				return Core::Cotask<bool>([this, Connections]()
				{
					VI_MEASURE(Core::Timings::Intensive);
					const char** Keys = Source.CreateKeys();
					const char** Values = Source.CreateValues();
					Core::UMutex<std::mutex> Unique(Update);
					Core::UnorderedMap<socket_t, TConnection*> Queue;
					Core::Vector<Network::Utils::PollFd> Sockets;
					TConnection* Error = nullptr;

					VI_DEBUG("[pq] try connect using %i connections", (int)Connections);
					Queue.reserve(Connections);
					
					auto& Props = Source.Get();
					auto Hostname = Props.find("host");
					auto Port = Props.find("port");
					auto ConnectTimeoutValue = Props.find("connect_timeout");
					auto ConnectTimeoutSeconds = ConnectTimeoutValue == Props.end() ? Core::ExpectsIO<uint64_t>(std::make_error_condition(std::errc::invalid_argument)) : Core::FromString<uint64_t>(ConnectTimeoutValue->second);
					Core::String Address = Core::Stringify::Text("%s:%s", Hostname != Props.end() ? Hostname->second.c_str() : "0.0.0.0", Port != Props.end() ? Port->second.c_str() : "5432");
					uint64_t ConnectTimeout = (ConnectTimeoutSeconds ? *ConnectTimeoutSeconds : 10);
					time_t ConnectInitiated = time(nullptr);

					for (size_t i = 0; i < Connections; i++)
					{
						TConnection* Base = PQconnectStartParams(Keys, Values, 0);
						if (!Base)
							goto Failure;

						VI_DEBUG("[pq] try connect to %s on 0x%" PRIXPTR, Address.c_str(), Base);
						if (PQstatus(Base) == ConnStatusType::CONNECTION_BAD)
						{
							PQfinish(Base);
							goto Failure;
						}

						Network::Utils::PollFd Fd;
						Fd.Fd = (socket_t)PQsocket(Base);

						Queue[Fd.Fd] = Base;
						Sockets.emplace_back(std::move(Fd));
					}

					do
					{
						for (auto It = Sockets.begin(); It != Sockets.end(); It++)
						{
							Network::Utils::PollFd& Fd = *It; TConnection* Base = Queue[Fd.Fd];
							if (Fd.Events == 0 || Fd.Returns & Network::Utils::Input || Fd.Returns & Network::Utils::Output)
							{
								bool Ready = false;
								switch (PQconnectPoll(Base))
								{
									case PGRES_POLLING_ACTIVE:
										break;
									case PGRES_POLLING_FAILED:
										Error = Base;
										goto Failure;
									case PGRES_POLLING_WRITING:
										Fd.Events = Network::Utils::Output;
										break;
									case PGRES_POLLING_READING:
										Fd.Events = Network::Utils::Input;
										break;
									case PGRES_POLLING_OK:
									{
										VI_DEBUG("[pq] OK connect on 0x%" PRIXPTR, (uintptr_t)Base);
										PQsetnonblocking(Base, 1);
										PQsetNoticeProcessor(Base, PQlogNotice, nullptr);
										PQlogMessage(Base);

										Connection* Next = new Connection(Base, Fd.Fd);
										Pool.insert(std::make_pair(Next->Stream, Next));
										Queue.erase(Fd.Fd);
										Sockets.erase(It);

										Reprocess(Next);
										Ready = true;

										break;
									}
								}

								if (Ready)
									break;
							}
							else if (Fd.Returns & Network::Utils::Error || Fd.Returns & Network::Utils::Hangup)
							{
								Error = Base;
								goto Failure;
							}
						}

						if (time(nullptr) - ConnectInitiated >= (time_t)ConnectTimeout)
						{
							VI_ERR("[pqerr] connection to %s has timed out (took %" PRIu64 " seconds)", Address.c_str(), ConnectTimeout);
							goto Failure;
						}
					} while (!Sockets.empty() && Network::Utils::Poll(Sockets.data(), (int)Sockets.size(), 50) >= 0);

					VI_FREE(Keys);
					VI_FREE(Values);
					return true;
				Failure:
					if (Error != nullptr)
						PQlogMessage(Error);

					for (auto& Base : Queue)
						PQfinish(Base.second);

					VI_FREE(Keys);
					VI_FREE(Values);
					return false;
				});
#else
				return Core::Promise<bool>(false);
#endif
			}
			Core::Promise<bool> Cluster::Disconnect()
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(!Pool.empty(), "connection should be established");
				return Core::Cotask<bool>([this]()
				{
					Core::UMutex<std::mutex> Unique(Update);
					for (auto& Item : Pool)
					{
						Item.first->ClearEvents(false);
						PQfinish(Item.second->Base);
						VI_RELEASE(Item.second);
					}

					Pool.clear();
					return true;
				});
#else
				return Core::Promise<bool>(false);
#endif
			}
			Core::Promise<bool> Cluster::Listen(const Core::Vector<Core::String>& Channels)
			{
				VI_ASSERT(!Channels.empty(), "channels should not be empty");
				Core::Vector<Core::String> Actual;
				{
					Core::UMutex<std::mutex> Unique(Update);
					Actual.reserve(Channels.size());
					for (auto& Item : Channels)
					{
						if (!IsListens(Item))
							Actual.push_back(Item);
					}
				}

				if (Actual.empty())
					return Core::Promise<bool>(true);

				Core::String Command;
				for (auto& Item : Actual)
					Command += "LISTEN " + Item + ';';

				return Query(Command).Then<bool>([this, Actual = std::move(Actual)](Cursor&& Result) mutable
				{
					Connection* Base = Result.GetExecutor();
					if (!Base || !Result.IsSuccess())
						return false;

					Core::UMutex<std::mutex> Unique(Update);
					for (auto& Item : Actual)
						Base->Listens.insert(Item);
					return true;
				});
			}
			Core::Promise<bool> Cluster::Unlisten(const Core::Vector<Core::String>& Channels)
			{
				VI_ASSERT(!Channels.empty(), "channels should not be empty");
				Core::UnorderedMap<Connection*, Core::String> Commands;
				{
					Core::UMutex<std::mutex> Unique(Update);
					for (auto& Item : Channels)
					{
						Connection* Next = IsListens(Item);
						if (Next != nullptr)
						{
							Commands[Next] += "UNLISTEN " + Item + ';';
							Next->Listens.erase(Item);
						}
					}
				}

				if (Commands.empty())
					return Core::Promise<bool>(true);

				return Core::Coasync<bool>([this, Commands = std::move(Commands)]() mutable -> Core::Promise<bool>
				{
					size_t Count = 0;
					for (auto& Next : Commands)
						Count += VI_AWAIT(Query(Next.second, (size_t)QueryOp::TransactionAlways, Next.first)).IsSuccess() ? 1 : 0;

					Coreturn Count > 0;
				});
			}
			Core::Promise<Cursor> Cluster::EmplaceQuery(const Core::String& Command, Core::SchemaList* Map, size_t Opts, SessionId Session)
			{
				bool Once = !(Opts & (size_t)QueryOp::ReuseArgs);
				return Query(Driver::Get()->Emplace(this, Command, Map, Once), Opts, Session);
			}
			Core::Promise<Cursor> Cluster::TemplateQuery(const Core::String& Name, Core::SchemaArgs* Map, size_t Opts, SessionId Session)
			{
				VI_DEBUG("[pq] template query %s", Name.empty() ? "empty-query-name" : Name.c_str());

				bool Once = !(Opts & (size_t)QueryOp::ReuseArgs);
				return Query(Driver::Get()->GetQuery(this, Name, Map, Once), Opts, Session);
			}
			Core::Promise<Cursor> Cluster::Query(const Core::String& Command, size_t Opts, SessionId Session)
			{
				VI_ASSERT(!Command.empty(), "command should not be empty");

				bool MayCache = Opts & (size_t)QueryOp::CacheShort || Opts & (size_t)QueryOp::CacheMid || Opts & (size_t)QueryOp::CacheLong;
				Core::String Reference;

				if (MayCache)
				{
					Cursor Result(nullptr, Caching::Cached);
					Reference = GetCacheOid(Command, Opts);

					if (GetCache(Reference, &Result))
					{
						Driver::Get()->LogQuery(Command);
						VI_DEBUG("[pq] OK execute on NULL (memory-cache)");

						return Core::Promise<Cursor>(std::move(Result));
					}
				}

				Request* Next = new Request(Command, MayCache ? Caching::Miss : Caching::Never);
				Next->Session = Session;
				Next->Options = Opts;

				if (!Reference.empty())
				{
					Next->Callback = [this, Reference, Opts](Cursor& Data)
					{
						SetCache(Reference, &Data, Opts);
					};
				}

				Core::Promise<Cursor> Future = Next->Future;
				bool IsInQueue = true;
				{
					Core::UMutex<std::mutex> Unique(Update);
					Requests.push_back(Next);
					for (auto& Item : Pool)
					{
						if (Consume(Item.second, Unique))
						{
							IsInQueue = false;
							break;
						}
					}
				}
				Driver::Get()->LogQuery(Command);
#ifndef NDEBUG
				if (!IsInQueue || Next->Session == 0)
					return Future;

				Core::String Tx = Command;
				Core::Stringify::Trim(Tx);
				Core::Stringify::ToUpper(Tx);

				if (Tx != "COMMIT" && Tx != "ROLLBACK")
					return Future;
				{
					Core::UMutex<std::mutex> Unique(Update);
					for (auto& Item : Pool)
					{
						if (Item.second->InSession && Item.second == Next->Session)
							return Future;
					}

					auto It = std::find(Requests.begin(), Requests.end(), Next);
					if (It != Requests.end())
						Requests.erase(It);
				}
				Future.Set(Cursor());
				VI_RELEASE(Next);
				VI_ASSERT(false, "[pq] transaction %" PRIu64 " does not exist", (void*)Session);
#endif
				return Future;
			}
			Connection* Cluster::GetConnection(QueryState State)
			{
				Core::UMutex<std::mutex> Unique(Update);
				for (auto& Item : Pool)
				{
					if (Item.second->State == State)
						return Item.second;
				}

				return nullptr;
			}
			Connection* Cluster::GetConnection() const
			{
				for (auto& Item : Pool)
					return Item.second;

				return nullptr;
			}
			Connection* Cluster::IsListens(const Core::String& Name)
			{
				for (auto& Item : Pool)
				{
					if (Item.second->Listens.count(Name) > 0)
						return Item.second;
				}

				return nullptr;
			}
			Core::String Cluster::GetCacheOid(const Core::String& Payload, size_t Opts)
			{
				auto Hash = Compute::Crypto::Hash(Compute::Digests::SHA256(), Payload);
				Core::String Reference = Hash ? Compute::Codec::HexEncode(*Hash) : Compute::Codec::HexEncode(Payload.substr(0, 32));
				if (Opts & (size_t)QueryOp::CacheShort)
					Reference.append(".s");
				else if (Opts & (size_t)QueryOp::CacheMid)
					Reference.append(".m");
				else if (Opts & (size_t)QueryOp::CacheLong)
					Reference.append(".l");

				return Reference;
			}
			bool Cluster::IsConnected() const
			{
				return !Pool.empty();
			}
			bool Cluster::GetCache(const Core::String& CacheOid, Cursor* Data)
			{
				VI_ASSERT(!CacheOid.empty(), "cache oid should not be empty");
				VI_ASSERT(Data != nullptr, "cursor should be set");

				Core::UMutex<std::mutex> Unique(Cache.Context);
				auto It = Cache.Objects.find(CacheOid);
				if (It == Cache.Objects.end())
					return false;

				int64_t Time = time(nullptr);
				if (It->second.first < Time)
				{
					*Data = std::move(It->second.second);
					Cache.Objects.erase(It);
				}
				else
					*Data = It->second.second.Copy();

				return true;
			}
			void Cluster::SetCache(const Core::String& CacheOid, Cursor* Data, size_t Opts)
			{
				VI_ASSERT(!CacheOid.empty(), "cache oid should not be empty");
				VI_ASSERT(Data != nullptr, "cursor should be set");

				int64_t Time = time(nullptr);
				int64_t Timeout = Time;
				if (Opts & (size_t)QueryOp::CacheShort)
					Timeout += Cache.ShortDuration;
				else if (Opts & (size_t)QueryOp::CacheMid)
					Timeout += Cache.MidDuration;
				else if (Opts & (size_t)QueryOp::CacheLong)
					Timeout += Cache.LongDuration;

				Core::UMutex<std::mutex> Unique(Cache.Context);
				if (Cache.NextCleanup < Time)
				{
					Cache.NextCleanup = Time + Cache.CleanupDuration;
					for (auto It = Cache.Objects.begin(); It != Cache.Objects.end();)
					{
						if (It->second.first < Time)
							It = Cache.Objects.erase(It);
						else
							++It;
					}
				}

				auto It = Cache.Objects.find(CacheOid);
				if (It != Cache.Objects.end())
				{
					It->second.second = Data->Copy();
					It->second.first = Timeout;
				}
				else
					Cache.Objects[CacheOid] = std::make_pair(Timeout, Data->Copy());
			}
			void Cluster::TryUnassign(Connection* Base, Request* Context)
			{
				if (!(Context->Options & (size_t)QueryOp::TransactionEnd))
					return;

				VI_DEBUG("[pq] end transaction on 0x%" PRIXPTR, (uintptr_t)Base);
				Base->InSession = false;
			}
			bool Cluster::Reestablish(Connection* Target)
			{
#ifdef VI_POSTGRESQL
				const char** Keys = Source.CreateKeys();
				const char** Values = Source.CreateValues();

				Core::UMutex<std::mutex> Unique(Update);
				if (Target->Current != nullptr)
				{
					Request* Current = Target->Current;
					Target->Current = nullptr;

					Unique.Negated([&Current]() { Current->Failure(); });
					VI_ERR("[pqerr] query reset on 0x%" PRIXPTR ": connection lost", (uintptr_t)Target->Base);
					VI_RELEASE(Current);
				}

				Target->Stream->ClearEvents(false);
				PQfinish(Target->Base);

				VI_DEBUG("[pq] try reconnect on 0x%" PRIXPTR, (uintptr_t)Target->Base);
				Target->Base = PQconnectdbParams(Keys, Values, 0);
				VI_FREE(Keys);
				VI_FREE(Values);

				if (!Target->Base || PQstatus(Target->Base) != ConnStatusType::CONNECTION_OK)
				{
					if (Target != nullptr)
						PQlogMessage(Target->Base);

					Core::Schedule::Get()->SetTask([this, Target]() { Reestablish(Target); });
					return false;
				}

				Core::Vector<Core::String> Channels;
				Channels.reserve(Target->Listens.size());
				for (auto& Item : Target->Listens)
					Channels.push_back(Item);
				Target->Listens.clear();

				VI_DEBUG("[pq] OK reconnect on 0x%" PRIXPTR, (uintptr_t)Target->Base);
				Target->State = QueryState::Idle;
				Target->Stream->MigrateTo((socket_t)PQsocket(Target->Base));
				PQsetnonblocking(Target->Base, 1);
				PQsetNoticeProcessor(Target->Base, PQlogNotice, nullptr);
				Consume(Target, Unique);
				Unique.Negate();

				bool Success = Reprocess(Target);
				if (!Channels.empty())
				{
					if (Reconnected)
					{
						Reconnected(Channels).When([this, Channels](bool&& ListenToChannels)
						{
							if (ListenToChannels)
								Listen(Channels);
						});
					}
					else
						Listen(Channels);
				}
				else if (Reconnected)
					Reconnected(Channels);

				return Success;
#else
				return false;
#endif
			}
			bool Cluster::Consume(Connection* Base, Core::UMutex<std::mutex>& Unique)
			{
#ifdef VI_POSTGRESQL
				if (Base->State != QueryState::Idle || Requests.empty())
					return false;

				for (auto It = Requests.begin(); It != Requests.end(); ++It)
				{
					Request* Context = *It;
					if (TryAssign(Base, Context))
					{
						Context->Result.Executor = Base;
						Base->Current = Context;
						Requests.erase(It);
						break;
					}
				}

				if (!Base->Current)
					return false;

				VI_MEASURE(Core::Timings::Intensive);
				VI_DEBUG("[pq] execute query on 0x%" PRIXPTR "%s: %.64s%s", (uintptr_t)Base, Base->InSession ? " (transaction)" : "", Base->Current->Command.data(), Base->Current->Command.size() > 64 ? " ..." : "");

				if (PQsendQuery(Base->Base, Base->Current->Command.data()) == 1)
				{
					Flush(Base, false);
					return true;
				}

				Request* Item = Base->Current;
				Base->Current = nullptr;
				PQlogMessage(Base->Base);
				Unique.Negated([&Item]() { Item->Failure(); });
				VI_RELEASE(Item);
				return true;
#else
				return false;
#endif
			}
			bool Cluster::Reprocess(Connection* Source)
			{
				return Multiplexer::Get()->WhenReadable(Source->Stream, [this, Source](SocketPoll Event)
				{
					if (!Packet::IsSkip(Event))
						Dispatch(Source, !Packet::IsError(Event));
				});
			}
			bool Cluster::Flush(Connection* Base, bool ListenForResults)
			{
#ifdef VI_POSTGRESQL
				Base->State = QueryState::Busy;
				if (PQflush(Base->Base) == 1)
				{
					auto* Stream = Multiplexer::Get();
					Stream->CancelEvents(Base->Stream);
					return Stream->WhenWriteable(Base->Stream, [this, Base](SocketPoll Event)
					{
						if (!Packet::IsSkip(Event))
							Flush(Base, true);
					});
				}
#endif
				if (ListenForResults)
					return Reprocess(Base);

				return true;
			}
			bool Cluster::Dispatch(Connection* Source, bool Connected)
			{
#ifdef VI_POSTGRESQL
				VI_MEASURE(Core::Timings::Intensive);
				Core::UMutex<std::mutex> Unique(Update);
				if (!Connected)
				{
					Source->State = QueryState::Lost;
					return Core::Schedule::Get()->SetTask([this, Source]() { Reestablish(Source); }) != Core::INVALID_TASK_ID;
				}

			Retry:
				Consume(Source, Unique);
				if (PQconsumeInput(Source->Base) != 1)
				{
					PQlogMessage(Source->Base);
					Source->State = QueryState::Lost;
					goto Finalize;
				}

				if (PQisBusy(Source->Base) == 0)
				{
					PGnotify* Notification = nullptr;
					while ((Notification = PQnotifies(Source->Base)) != nullptr)
					{
						if (Notification != nullptr && Notification->relname != nullptr)
						{
							auto It = Listeners.find(Notification->relname);
							if (It != Listeners.end() && !It->second.empty())
							{
								Notify Event(Notification);
								for (auto& Item : It->second)
								{
									OnNotification Callback = Item.second;
									Core::Schedule::Get()->SetTask([Event, Callback = std::move(Callback)]() { Callback(Event); });
								}
							}
							VI_DEBUG("[pq] notification on channel @%s: %s", Notification->relname, Notification->extra ? Notification->extra : "[payload]");
							PQfreeNotify(Notification);
						}
					}

					Response Frame(PQgetResult(Source->Base));
					if (Source->Current != nullptr)
					{
						if (!Frame.IsExists())
						{
							Core::Promise<Cursor> Future = Source->Current->Future;
							Cursor Results(std::move(Source->Current->Result));
							Request* Item = Source->Current;
							Source->State = QueryState::Idle;
							Source->Current = nullptr;
							PQlogMessage(Source->Base);

							if (!Results.IsError())
							{
								VI_DEBUG("[pq] OK execute on 0x%" PRIXPTR " (%" PRIu64 " ms)", (uintptr_t)Source, Item->GetTiming());
								TryUnassign(Source, Item);
							}

							Unique.Negated([&Item, &Future, &Results]()
							{
								Item->Finalize(Results);
								Future.Set(std::move(Results));
							});
							VI_RELEASE(Item);
						}
						else
							Source->Current->Result.Base.emplace_back(std::move(Frame));
					}
					else
						Source->State = (Frame.IsExists() ? QueryState::Busy : QueryState::Idle);

					if (Source->State == QueryState::Busy || Consume(Source, Unique))
						goto Retry;
				}

			Finalize:
				return Reprocess(Source);
#else
				return false;
#endif
			}
			bool Cluster::TryAssign(Connection* Base, Request* Context)
			{
				if (Base->InSession || (Context->Session != nullptr && (Context->Options & (size_t)QueryOp::TransactionAlways)))
					return Base == Context->Session;

				if (!Context->Session && !(Context->Options & (size_t)QueryOp::TransactionStart))
					return true;

				for (auto& Item : Pool)
				{
					if (Item.second == Context->Session)
						return false;
				}

				VI_DEBUG("[pq] start transaction on 0x%" PRIXPTR, (uintptr_t)Base);
				Base->InSession = true;
				return true;
			}

			Core::String Utils::InlineArray(Cluster* Client, Core::Schema* Array)
			{
				VI_ASSERT(Client != nullptr, "cluster should be set");
				VI_ASSERT(Array != nullptr, "array should be set");

				Core::SchemaList Map;
				Core::String Def;

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

				Core::String Result = PDB::Driver::Get()->Emplace(Client, Def, &Map, false);
				if (!Result.empty() && Result.back() == ',')
					Result.erase(Result.end() - 1);

				VI_RELEASE(Array);
				return Result;
			}
			Core::String Utils::InlineQuery(Cluster* Client, Core::Schema* Where, const Core::UnorderedSet<Core::String>& Whitelist, const Core::String& Default)
			{
				VI_ASSERT(Client != nullptr, "cluster should be set");
				VI_ASSERT(Where != nullptr, "array should be set");

				Core::SchemaList Map;
				Core::String Allow = "abcdefghijklmnopqrstuvwxyz._", Def;
				for (auto* Statement : Where->GetChilds())
				{
					Core::String Op = Statement->Value.GetBlob();
					if (Op == "=" || Op == "<>" || Op == "<=" || Op == "<" || Op == ">" || Op == ">=" || Op == "+" || Op == "-" || Op == "*" || Op == "/" || Op == "(" || Op == ")" || Op == "TRUE" || Op == "FALSE")
						Def += Op;
					else if (Op == "~==")
						Def += " LIKE ";
					else if (Op == "~=")
						Def += " ILIKE ";
					else if (Op == "[")
						Def += " ANY(";
					else if (Op == "]")
						Def += ")";
					else if (Op == "&")
						Def += " AND ";
					else if (Op == "|")
						Def += " OR ";
					else if (Op == "TRUE")
						Def += " TRUE ";
					else if (Op == "FALSE")
						Def += " FALSE ";
					else if (!Op.empty())
					{
						if (Op.front() == '@')
						{
							Op = Op.substr(1);
							if (Whitelist.empty() || Whitelist.find(Op) != Whitelist.end())
							{
								if (Op.find_first_not_of(Allow) == Core::String::npos)
									Def += Op;
							}
						}
						else if (!Core::Stringify::HasNumber(Op))
						{
							Def += "?";
							Map.push_back(Statement);
						}
						else
							Def += Op;
					}
				}

				Core::String Result = PDB::Driver::Get()->Emplace(Client, Def, &Map, false);
				if (Result.empty())
					Result = Default;

				VI_RELEASE(Where);
				return Result;
			}
			Core::String Utils::GetCharArray(TConnection* Base, const Core::String& Src) noexcept
			{
#ifdef VI_POSTGRESQL
				if (Src.empty())
					return "''";

				if (!Base)
				{
					Core::String Dest(Src);
					Dest.insert(Dest.begin(), '\'');
					Dest.append(1, '\'');
					return Dest;
				}

				char* Subresult = PQescapeLiteral(Base, Src.c_str(), Src.size());
				Core::String Result(Subresult);
				PQfreemem(Subresult);

				return Result;
#else
				return "'" + Src + "'";
#endif
			}
			Core::String Utils::GetByteArray(TConnection* Base, const char* Src, size_t Size) noexcept
			{
#ifdef VI_POSTGRESQL
				if (!Src || !Size)
					return "''";

				if (!Base)
					return "'\\x" + Compute::Codec::HexEncode(Src, Size) + "'::bytea";

				size_t Length = 0;
				char* Subresult = (char*)PQescapeByteaConn(Base, (unsigned char*)Src, Size, &Length);
				Core::String Result(Subresult, Length > 1 ? Length - 1 : Length);
				PQfreemem((unsigned char*)Subresult);

				Result.insert(Result.begin(), '\'');
				Result.append("'::bytea");
				return Result;
#else
				return "'\\x" + Compute::Codec::HexEncode(Src, Size) + "'::bytea";
#endif
			}
			Core::String Utils::GetSQL(TConnection* Base, Core::Schema* Source, bool Escape, bool Negate) noexcept
			{
				if (!Source)
					return "NULL";

				Core::Schema* Parent = Source->GetParent();
				switch (Source->Value.GetType())
				{
					case Core::VarType::Object:
					{
						Core::String Result;
						Core::Schema::ConvertToJSON(Source, [&Result](Core::VarForm, const char* Buffer, size_t Length)
						{
							if (Buffer != nullptr && Length > 0)
								Result.append(Buffer, Length);
						});

						return Escape ? GetCharArray(Base, Result) : Result;
					}
					case Core::VarType::Array:
					{
						Core::String Result = (Parent && Parent->Value.GetType() == Core::VarType::Array ? "[" : "ARRAY[");
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
						return Core::ToString(Negate ? -Source->Value.GetInteger() : Source->Value.GetInteger());
					case Core::VarType::Number:
					{
						bool Trailing = (Source->Value.GetNumber() != (double)Source->Value.GetInteger());
						Core::String Result = Core::ToString(Negate ? -Source->Value.GetNumber() : Source->Value.GetNumber());
						return Trailing ? Result : Result + ".0";
					}
					case Core::VarType::Boolean:
						return (Negate ? !Source->Value.GetBoolean() : Source->Value.GetBoolean()) ? "TRUE" : "FALSE";
					case Core::VarType::Decimal:
					{
						Core::Decimal Value = Source->Value.GetDecimal();
						Core::String Result = (Negate ? '-' + Value.ToString() : Value.ToString());
						return Result.find('.') != Core::String::npos ? Result : Result + ".0";
					}
					case Core::VarType::Binary:
						return GetByteArray(Base, Source->Value.GetString(), Source->Value.GetSize());
					case Core::VarType::Null:
					case Core::VarType::Undefined:
						return "NULL";
					default:
						break;
				}

				return "NULL";
			}

			Driver::Driver() noexcept : Active(false), Logger(nullptr)
			{
				Network::Multiplexer::Get()->Activate();
			}
			Driver::~Driver() noexcept
			{
				Network::Multiplexer::Get()->Deactivate();
			}
			void Driver::SetQueryLog(const OnQueryLog& Callback) noexcept
			{
				Logger = Callback;
			}
			void Driver::LogQuery(const Core::String& Command) noexcept
			{
				if (Logger)
					Logger(Command + '\n');
			}
			bool Driver::AddConstant(const Core::String& Name, const Core::String& Value) noexcept
			{
				VI_ASSERT(!Name.empty(), "name should not be empty");
				Core::UMutex<std::mutex> Unique(Exclusive);
				Constants[Name] = Value;
				return true;
			}
			bool Driver::AddQuery(const Core::String& Name, const char* Buffer, size_t Size) noexcept
			{
				VI_ASSERT(!Name.empty(), "name should not be empty");
				VI_ASSERT(Buffer, "buffer should be set");

				if (!Size)
					return false;

				Sequence Result;
				Result.Request.assign(Buffer, Size);

				Core::String Lines = "\r\n";
				Core::String Enums = " \r\n\t\'\"()<>=%&^*/+-,!?:;";
				Core::String Erasable = " \r\n\t\'\"()<>=%&^*/+-,.!?:;";
				Core::String Quotes = "\"'`";

				Core::String& Base = Result.Request;
				Core::Stringify::ReplaceInBetween(Base, "/*", "*/", "", false);
				Core::Stringify::ReplaceStartsWithEndsOf(Base, "--", Lines.c_str(), "");
				Core::Stringify::Trim(Base);
				Core::Stringify::Compress(Base, Erasable.c_str(), Quotes.c_str());

				auto Enumerations = Core::Stringify::FindStartsWithEndsOf(Base, "#", Enums.c_str(), Quotes.c_str());
				if (!Enumerations.empty())
				{
					int64_t Offset = 0;
					Core::UMutex<std::mutex> Unique(Exclusive);
					for (auto& Item : Enumerations)
					{
						size_t Size = Item.second.End - Item.second.Start, NewSize = 0;
						Item.second.Start = (size_t)((int64_t)Item.second.Start + Offset);
						Item.second.End = (size_t)((int64_t)Item.second.End + Offset);

						auto It = Constants.find(Item.first);
						if (It == Constants.end())
						{
							VI_ERR("[pq] template query @%s expects constant: %s", Name.c_str(), Item.first.c_str());
							Core::Stringify::ReplacePart(Base, Item.second.Start, Item.second.End, "");
						}
						else
						{
							Core::Stringify::ReplacePart(Base, Item.second.Start, Item.second.End, It->second);
							NewSize = It->second.size();
						}

						Offset += (int64_t)NewSize - (int64_t)Size;
						Item.second.End = Item.second.Start + NewSize;
					}
				}

				Core::Vector<std::pair<Core::String, Core::TextSettle>> Variables;
				for (auto& Item : Core::Stringify::FindInBetween(Base, "$<", ">", Quotes.c_str()))
				{
					Item.first += ";escape";
					if (Core::Stringify::IsPrecededBy(Base, Item.second.Start, "-"))
					{
						Item.first += ";negate";
						--Item.second.Start;
					}

					Variables.emplace_back(std::move(Item));
				}

				for (auto& Item : Core::Stringify::FindInBetween(Base, "@<", ">", Quotes.c_str()))
				{
					Item.first += ";unsafe";
					if (Core::Stringify::IsPrecededBy(Base, Item.second.Start, "-"))
					{
						Item.first += ";negate";
						--Item.second.Start;
					}

					Variables.emplace_back(std::move(Item));
				}

				Core::Stringify::ReplaceParts(Base, Variables, "", [&Erasable](const Core::String& Name, char Left, int Side)
				{
					if (Side < 0 && Name.find(";negate") != Core::String::npos)
						return '\0';

					return Erasable.find(Left) == Core::String::npos ? ' ' : '\0';
				});

				for (auto& Item : Variables)
				{
					Pose Position;
					Position.Negate = Item.first.find(";negate") != Core::String::npos;
					Position.Escape = Item.first.find(";escape") != Core::String::npos;
					Position.Offset = Item.second.Start;
					Position.Key = Item.first.substr(0, Item.first.find(';'));
					Result.Positions.emplace_back(std::move(Position));
				}

				if (Variables.empty())
					Result.Cache = Result.Request;

				Core::UMutex<std::mutex> Unique(Exclusive);
				Queries[Name] = std::move(Result);
				return true;
			}
			bool Driver::AddDirectory(const Core::String& Directory, const Core::String& Origin) noexcept
			{
				Core::Vector<std::pair<Core::String, Core::FileEntry>> Entries;
				if (!Core::OS::Directory::Scan(Directory, &Entries))
					return false;

				Core::String Path = Directory;
				if (Path.back() != '/' && Path.back() != '\\')
					Path.append(1, '/');

				size_t Size = 0;
				for (auto& File : Entries)
				{
					Core::String Base(Path + File.first);
					if (File.second.IsDirectory)
					{
						AddDirectory(Base, Origin.empty() ? Directory : Origin);
						continue;
					}

					if (!Core::Stringify::EndsWith(Base, ".sql"))
						continue;

					auto Buffer = Core::OS::File::ReadAll(Base, &Size);
					if (!Buffer)
						continue;

					Core::Stringify::Replace(Base, Origin.empty() ? Directory : Origin, "");
					Core::Stringify::Replace(Base, "\\", "/");
					Core::Stringify::Replace(Base, ".sql", "");
					if (Core::Stringify::StartsOf(Base, "\\/"))
						Base.erase(0, 1);

					AddQuery(Base, (char*)*Buffer, Size);
					VI_FREE(*Buffer);
				}

				return true;
			}
			bool Driver::RemoveConstant(const Core::String& Name) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Constants.find(Name);
				if (It == Constants.end())
					return false;

				Constants.erase(It);
				return true;
			}
			bool Driver::RemoveQuery(const Core::String& Name) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Queries.find(Name);
				if (It == Queries.end())
					return false;

				Queries.erase(It);
				return true;
			}
			bool Driver::LoadCacheDump(Core::Schema* Dump) noexcept
			{
				VI_ASSERT(Dump != nullptr, "dump should be set");
				size_t Count = 0;
				Core::UMutex<std::mutex> Unique(Exclusive);
				Queries.clear();

				for (auto* Data : Dump->GetChilds())
				{
					Sequence Result;
					Result.Cache = Data->GetVar("cache").GetBlob();
					Result.Request = Data->GetVar("request").GetBlob();

					if (Result.Request.empty())
						Result.Request = Result.Cache;

					Core::Schema* Positions = Data->Get("positions");
					if (Positions != nullptr)
					{
						for (auto* Position : Positions->GetChilds())
						{
							Pose Next;
							Next.Key = Position->GetVar(0).GetBlob();
							Next.Offset = (size_t)Position->GetVar(1).GetInteger();
							Next.Escape = Position->GetVar(2).GetBoolean();
							Next.Negate = Position->GetVar(3).GetBoolean();
							Result.Positions.emplace_back(std::move(Next));
						}
					}

					Core::String Name = Data->GetVar("name").GetBlob();
					Queries[Name] = std::move(Result);
					++Count;
				}

				if (Count > 0)
					VI_DEBUG("[pq] OK load %" PRIu64 " parsed query templates", (uint64_t)Count);

				return Count > 0;
			}
			Core::Schema* Driver::GetCacheDump() noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				Core::Schema* Result = Core::Var::Set::Array();
				for (auto& Query : Queries)
				{
					Core::Schema* Data = Result->Push(Core::Var::Set::Object());
					Data->Set("name", Core::Var::String(Query.first));

					if (Query.second.Cache.empty())
						Data->Set("request", Core::Var::String(Query.second.Request));
					else
						Data->Set("cache", Core::Var::String(Query.second.Cache));

					auto* Positions = Data->Set("positions", Core::Var::Set::Array());
					for (auto& Position : Query.second.Positions)
					{
						auto* Next = Positions->Push(Core::Var::Set::Array());
						Next->Push(Core::Var::String(Position.Key));
						Next->Push(Core::Var::Integer(Position.Offset));
						Next->Push(Core::Var::Boolean(Position.Escape));
						Next->Push(Core::Var::Boolean(Position.Negate));
					}
				}

				VI_DEBUG("[pq] OK save %" PRIu64 " parsed query templates", (uint64_t)Queries.size());
				return Result;
			}
			Core::String Driver::Emplace(Cluster* Base, const Core::String& SQL, Core::SchemaList* Map, bool Once) noexcept
			{
				if (!Map || Map->empty())
					return SQL;

				Connection* Remote = Base->GetConnection();
				Core::String Buffer(SQL);
				Core::TextSettle Set;
				Core::String& Src = Buffer;
				size_t Offset = 0;
				size_t Next = 0;

				while ((Set = Core::Stringify::Find(Buffer, '?', Offset)).Found)
				{
					if (Next >= Map->size())
					{
						VI_ERR("[pq] emplace query \"%.64s\" expects at least %" PRIu64 " args", SQL.c_str(), (uint64_t)(Next)+1);
						break;
					}

					bool Escape = true, Negate = false;
					if (Set.Start > 0)
					{
						if (Src[Set.Start - 1] == '\\')
						{
							Offset = Set.Start;
							Buffer.erase(Set.Start - 1, 1);
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
					Core::String Value = Utils::GetSQL(Remote->GetBase(), (*Map)[Next++], Escape, Negate);
					Buffer.erase(Set.Start, (Escape ? 1 : 2));
					Buffer.insert(Set.Start, Value);
					Offset = Set.Start + Value.size();
				}

				if (!Once)
					return Src;

				for (auto* Item : *Map)
					VI_RELEASE(Item);
				Map->clear();

				return Src;
			}
			Core::String Driver::GetQuery(Cluster* Base, const Core::String& Name, Core::SchemaArgs* Map, bool Once) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Queries.find(Name);
				if (It == Queries.end())
				{
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							VI_RELEASE(Item.second);
						Map->clear();
					}

					VI_ERR("[pq] template query %s does not exist", Name.c_str());
					return Core::String();
				}

				if (!It->second.Cache.empty())
				{
					Core::String Result = It->second.Cache;
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							VI_RELEASE(Item.second);
						Map->clear();
					}

					return Result;
				}

				if (!Map || Map->empty())
				{
					Core::String Result = It->second.Request;
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							VI_RELEASE(Item.second);
						Map->clear();
					}

					return Result;
				}

				Connection* Remote = Base->GetConnection();
				Sequence Origin = It->second;
				size_t Offset = 0;
				Unique.Negate();

				Core::String& Result = Origin.Request;
				for (auto& Word : Origin.Positions)
				{
					auto It = Map->find(Word.Key);
					if (It == Map->end())
					{
						VI_ERR("[pq] template query @%s expects parameter: %s", Name.c_str(), Word.Key.c_str());
						continue;
					}

					Core::String Value = Utils::GetSQL(Remote->GetBase(), It->second, Word.Escape, Word.Negate);
					if (Value.empty())
						continue;

					Result.insert(Word.Offset + Offset, Value);
					Offset += Value.size();
				}

				if (Once)
				{
					for (auto& Item : *Map)
						VI_RELEASE(Item.second);
					Map->clear();
				}

				Core::String Data = Origin.Request;
				if (Data.empty())
					VI_ERR("[pq] could not construct query: \"%s\"", Name.c_str());

				return Data;
			}
			Core::Vector<Core::String> Driver::GetQueries() noexcept
			{
				Core::Vector<Core::String> Result;
				Core::UMutex<std::mutex> Unique(Exclusive);
				Result.reserve(Queries.size());
				for (auto& Item : Queries)
					Result.push_back(Item.first);

				return Result;
			}
		}
	}
}
