#include "pdb.h"
#ifdef TH_HAS_POSTGRESQL
#include <libpq-fe.h>
#endif

namespace Tomahawk
{
	namespace Network
	{
		namespace PDB
		{
			class ArrayFilter;

			typedef std::function<void(void*, ArrayFilter*, char*, size_t)> FilterCallback;

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
					if (!Callback)
						return false;

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

			static Core::Document* ToDocument(const char* Data, int Size, unsigned int Id);
			static void ToArrayField(void* Context, ArrayFilter* Subdata, char* Data, size_t Size)
			{
#ifdef TH_HAS_POSTGRESQL
				std::pair<Core::Document*, Oid>* Base = (std::pair<Core::Document*, Oid>*)Context;
				if (Subdata != nullptr)
				{
					std::pair<Core::Document*, Oid> Next;
					Next.first = Core::Document::Array();
					Next.second = Base->second;

					if (!Subdata->Parse(&Next, ToArrayField))
					{
						Base->first->Push(new Core::Document(Core::Var::Null()));
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
						Base->first->Push(ToDocument(Result.Get(), (int)Result.Size(), Base->second));
					else
						Base->first->Push(new Core::Document(Core::Var::Null()));
				}
#endif
			}
			static unsigned char* ToBytea(const char* Data, size_t* OutSize)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Data || !OutSize)
					return nullptr;

				unsigned char* Result = PQunescapeBytea((const unsigned char*)Data, OutSize);
				if (!Result)
					return nullptr;

				unsigned char* Copy = (unsigned char*)TH_MALLOC(*OutSize * sizeof(unsigned char));
				memcpy(Copy, Result, *OutSize * sizeof(unsigned char));
				PQfreemem(Result);

				return Result;
#else
				return nullptr;
#endif
			}
			static Core::Variant ToVariant(const char* Data, int Size, unsigned int Id)
			{
#ifdef TH_HAS_POSTGRESQL
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
#else
				return Core::Var::Undefined();
#endif
			}
			static Core::Document* ToArray(const char* Data, int Size, unsigned int Id)
			{
#ifdef TH_HAS_POSTGRESQL
				std::pair<Core::Document*, Oid> Context;
				Context.first = Core::Document::Array();
				Context.second = Id;

				ArrayFilter Filter(Data, (size_t)Size);
				if (!Filter.Parse(&Context, ToArrayField))
				{
					TH_RELEASE(Context.first);
					return new Core::Document(Core::Var::String(Data, (size_t)Size));
				}

				return Context.first;
#else
				return nullptr;
#endif
			}
			Core::Document* ToDocument(const char* Data, int Size, unsigned int Id)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Data)
					return new Core::Document(Core::Var::Null());

				OidType Type = (OidType)Id;
				switch (Type)
				{
					case OidType::JSON:
					case OidType::JSONB:
					{
						Core::Document* Result = Core::Document::ReadJSON((int64_t)Size, [Data](char* Dest, int64_t Size)
						{
							memcpy(Dest, Data, Size);
							return true;
						});

						if (Result != nullptr)
							return Result;

						return new Core::Document(Core::Var::String(Data, (size_t)Size));
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
						return new Core::Document(ToVariant(Data, Size, Id));
				}
#else
				return nullptr;
#endif
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
						TH_ERROR("[pg] couldn't parse URI string\n\t%s", Message);
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

			Notify::Notify(TNotify* NewBase) : Base(NewBase)
			{
			}
			void Notify::Release()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return;

				PQfreeNotify(Base);
				Base = nullptr;
#endif
			}
			std::string Notify::GetName() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return std::string();

				if (!Base->relname)
					return std::string();

				return Base->relname;
#else
				return std::string();
#endif
			}
			std::string Notify::GetExtra() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return std::string();

				if (!Base->extra)
					return std::string();

				return Base->extra;
#else
				return std::string();
#endif
			}
			int Notify::GetPid() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return -1;

				return Base->be_pid;
#else
				return -1;
#endif
			}
			TNotify* Notify::Get() const
			{
				return Base;
			}

			Column::Column(TResult* NewBase, size_t fRowIndex, size_t fColumnIndex) : Base(NewBase), RowIndex(fRowIndex), ColumnIndex(fColumnIndex)
			{
			}
			int Column::SetValueText(char* Data, size_t Size)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return -1;

				return PQsetvalue(Base, RowIndex, ColumnIndex, Data, Size);
#else
				return -1;
#endif
			}
			std::string Column::GetName() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return std::string();

				char* Text = PQfname(Base, ColumnIndex);
				if (!Text)
					return std::string();

				return Text;
#else
				return std::string();
#endif
			}
			Core::Variant Column::GetValue() const
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
			Core::Document* Column::GetValueAuto() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				if (PQgetisnull(Base, RowIndex, ColumnIndex) == 1)
					return nullptr;

				char* Data = PQgetvalue(Base, RowIndex, ColumnIndex);
				int Size = PQgetlength(Base, RowIndex, ColumnIndex);
				Oid Type = PQftype(Base, ColumnIndex);

				return ToDocument(Data, Size, Type);
#else
				return nullptr;
#endif
			}
			char* Column::GetValueData() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				return PQgetvalue(Base, RowIndex, ColumnIndex);
#else
				return nullptr;
#endif
			}
			int Column::GetFormatId() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return 0;

				return PQfformat(Base, ColumnIndex);
#else
				return 0;
#endif
			}
			int Column::GetModId() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return -1;

				return PQfmod(Base, ColumnIndex);
#else
				return -1;
#endif
			}
			int Column::GetTableIndex() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return -1;

				return PQftablecol(Base, ColumnIndex);
#else
				return -1;
#endif
			}
			ObjectId Column::GetTableId() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return InvalidOid;

				return PQftable(Base, ColumnIndex);
#else
				return 0;
#endif
			}
			ObjectId Column::GetTypeId() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return InvalidOid;

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
			size_t Column::GetValueSize() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return 0;

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
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return Row(nullptr, std::numeric_limits<size_t>::max());

				return Row(Base, RowIndex);
#else
				return Row(nullptr, std::numeric_limits<size_t>::max());
#endif
			}
			bool Column::IsNull() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				return PQgetisnull(Base, RowIndex, ColumnIndex) == 1;
#else
				return true;
#endif
			}

			Row::Row(TResult* NewBase, size_t fRowIndex) : Base(NewBase), RowIndex(fRowIndex)
			{
			}
			Core::Document* Row::GetDocument() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				int Size = PQnfields(Base);
				if (Size <= 0)
					return nullptr;

				Core::Document* Result = Core::Document::Object();
				Result->GetNodes()->reserve((size_t)Size);

				for (int j = 0; j < Size; j++)
				{
					char* Name = PQfname(Base, j);
					char* Data = PQgetvalue(Base, RowIndex, j);
					int Size = PQgetlength(Base, RowIndex, j);
					bool Null = PQgetisnull(Base, RowIndex, j) == 1;
					Oid Type = PQftype(Base, j);

					if (!Null)
						Result->Set(Name ? Name : std::to_string(j), ToDocument(Data, Size, Type));
					else
						Result->Set(Name ? Name : std::to_string(j), Core::Var::Null());
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
			Result Row::GetResult() const
			{
				return Result(Base);
			}
			Column Row::GetColumn(size_t Index) const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || Index >= GetSize())
					return Column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				return Column(Base, RowIndex, Index);
#else
				return Column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());
#endif
			}
			Column Row::GetColumn(const char* Name) const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || !Name)
					return Column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

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
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || !Output || Size < 1)
					return false;

				Size = std::min(GetSize(), Size);
				for (size_t i = 0; i < Size; i++)
					Output[i] = Column(Base, RowIndex, i);

				return true;
#else
				return false;
#endif
			}

			Result::Result() : Base(nullptr)
			{
			}
			Result::Result(TResult* NewBase) : Base(NewBase)
			{
			}
			void Result::Release()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return;

				PQclear(Base);
				Base = nullptr;
#endif
			}
			Core::Document* Result::GetDocument() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return nullptr;

				int RowsSize = PQntuples(Base);
				if (RowsSize <= 0)
					return nullptr;

				int ColumnsSize = PQnfields(Base);
				if (ColumnsSize <= 0)
					return nullptr;

				std::vector<std::pair<std::string, Oid>> Meta;
				Meta.reserve((size_t)ColumnsSize);

				for (int j = 0; j < ColumnsSize; j++)
				{
					char* Name = PQfname(Base, j);
					Meta.emplace_back(std::make_pair(Name ? Name : std::to_string(j), PQftype(Base, j)));
				}

				Core::Document* Result = Core::Document::Array();
				Result->GetNodes()->reserve((size_t)RowsSize);

				for (int i = 0; i < RowsSize; i++)
				{
					Core::Document* Subresult = Core::Document::Object();
					Subresult->GetNodes()->reserve((size_t)ColumnsSize);

					for (int j = 0; j < ColumnsSize; j++)
					{
						char* Data = PQgetvalue(Base, i, j);
						int Size = PQgetlength(Base, i, j);
						bool Null = PQgetisnull(Base, i, j) == 1;
						auto& Field = Meta[j];

						if (!Null)
							Subresult->Set(Field.first, ToDocument(Data, Size, Field.second));
						else
							Subresult->Set(Field.first, Core::Var::Null());
					}

					Result->Push(Subresult);
				}

				return Result;
#else
				return nullptr;
#endif
			}
			std::string Result::GetCommandStatusText() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return std::string();

				char* Text = PQcmdStatus(Base);
				if (!Text)
					return std::string();

				return Text;
#else
				return std::string();
#endif
			}
			std::string Result::GetStatusText() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return std::string();

				char* Text = PQresStatus(PQresultStatus(Base));
				if (!Text)
					return std::string();

				return Text;
#else
				return std::string();
#endif
			}
			std::string Result::GetErrorText() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return std::string();

				char* Text = PQresultErrorMessage(Base);
				if (!Text)
					return std::string();

				return Text;
#else
				return std::string();
#endif
			}
			std::string Result::GetErrorField(FieldCode Field) const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return std::string();

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
			int Result::GetNameIndex(const std::string& Name) const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || Name.empty())
					return -1;

				return PQfnumber(Base, Name.c_str());
#else
				return 0;
#endif
			}
			QueryExec Result::GetStatus() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return QueryExec::Empty_Query;

				return (QueryExec)PQresultStatus(Base);
#else
				return QueryExec::Empty_Query;
#endif
			}
			ObjectId Result::GetValueId() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return InvalidOid;

				return PQoidValue(Base);
#else
				return 0;
#endif
			}
			size_t Result::GetAffectedRows() const
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
			size_t Result::GetSize() const
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
			Row Result::GetRow(size_t Index) const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || Index >= GetSize())
					return Row(nullptr, std::numeric_limits<size_t>::max());

				return Row(Base, Index);
#else
				return Row(nullptr, std::numeric_limits<size_t>::max());
#endif
			}
			Row Result::First() const
			{
				return GetRow(0);
			}
			Row Result::Last() const
			{
				return GetRow(GetSize() - 1);
			}
			TResult* Result::Get() const
			{
				return Base;
			}
			bool Result::IsEmpty() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return true;

				return PQntuples(Base) <= 0;
#else
				return true;
#endif
			}
			bool Result::IsError() const
			{
#ifdef TH_HAS_POSTGRESQL
				QueryExec State = GetStatus();
				return State == QueryExec::Fatal_Error || State == QueryExec::Non_Fatal_Error || State == QueryExec::Bad_Response;
#else
				return false;
#endif
			}

			Connection::Connection() : Base(nullptr), Master(nullptr), State(-1)
			{
				Driver::Create();
				Driver::Listen(this);
			}
			Connection::~Connection()
			{
				Cmd.Prev.Release();
				Cmd.Next.Release();
				Driver::Unlisten(this);
				Driver::Release();
			}
			void Connection::SetNotificationCallback(const OnNotification& NewCallback)
			{
				Safe.lock();
				Callback = NewCallback;
				Safe.unlock();
			}
			int Connection::SetEncoding(const std::string& Name)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return -1;

				return PQsetClientEncoding(Base, Name.c_str());
#else
				return -1;
#endif
			}
			Core::Async<bool> Connection::Connect(const Address& URI)
			{
#ifdef TH_HAS_POSTGRESQL
				if (Master != nullptr)
					return Core::Async<bool>::Store(false);

				if (State != -1)
				{
					return Disconnect().Then<Core::Async<bool>>([this, URI](bool)
					{
						return this->Connect(URI);
					});
				}

				return Core::Async<bool>([this, URI](Core::Async<bool>& Future)
				{
					const char** Keys = URI.CreateKeys();
					const char** Values = URI.CreateValues();
					Base = PQconnectdbParams(Keys, Values, 0);
					TH_FREE(Keys);
					TH_FREE(Values);

					if (!Base)
					{
						TH_ERROR("couldn't connect to requested URI");
						return Future.Set(false);
					}

					PQsetnonblocking(Base, 1);
					PQsetNoticeProcessor(Base, [](void*, const char* Message)
					{
						if (Message != nullptr)
							TH_WARN("[pq] %s", Message);
					}, nullptr);
					State = 0;
					Future.Set(true);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Connection::Disconnect()
			{
#ifdef TH_HAS_POSTGRESQL
				if (State == -1 || !Base)
					return Core::Async<bool>::Store(false);

				return Core::Async<bool>([this](Core::Async<bool>& Future)
				{
					State = -1;
					if (!Base)
						return Future.Set(true);

					if (!Master)
					{
						PQfinish(Base);
						Base = nullptr;
					}
					else
						Master->Clear(this);

					Future.Set(true);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Connection::Query(const std::string& Command, bool Chunked, bool Prefetch)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || Command.empty() || State == -1)
					return Core::Async<bool>::Store(false);

				if (State == 0)
				{
					if (!SendQuery(Command, Chunked))
						return Core::Async<bool>::Store(false);

					return (Prefetch ? Next() : Core::Async<bool>::Store(true));
				}

				return Cancel().Then<Core::Async<bool>>([this, Command, Chunked, Prefetch](bool&&)
				{
					if (!SendQuery(Command, Chunked))
						return Core::Async<bool>::Store(false);

					return (Prefetch ? Next() : Core::Async<bool>::Store(true));
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Connection::Next()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || State != 1)
					return Core::Async<bool>::Store(false);

				Safe.lock();
				Cmd.Future.Steal(Core::Async<bool>());
				Cmd.State = (Cmd.Next ? 1 : -1);
				State = 2;
				Safe.unlock();

				return Cmd.Future;
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Connection::Cancel()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || State != 1)
					return Core::Async<bool>::Store(false);

				PGcancel* Cancel = PQgetCancel(Base);
				if (!Cancel)
					return Core::Async<bool>::Store(false);

				char Error[256];
				int Code = PQcancel(Cancel, Error, sizeof(Error));
				if (Code != 1)
					TH_ERROR("[pqerr] %s", Error);

				PQfreeCancel(Cancel);
				if (Code != 1)
					return Core::Async<bool>::Store(false);

				Safe.lock();
				Cmd.Future.Steal(Core::Async<bool>());
				Cmd.State = -1;
				State = 3;
				Safe.unlock();

				return Cmd.Future;
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			bool Connection::SendQuery(const std::string& Command, bool Chunked)
			{
#ifdef TH_HAS_POSTGRESQL
				Safe.lock();
				if (PQsendQueryParams(Base, Command.c_str(), 0, nullptr, nullptr, nullptr, nullptr, 0) != 1)
				{
					char* Message = PQerrorMessage(Base);
					if (Message != nullptr)
						TH_ERROR("[pqerr] %s", Message);

					Safe.unlock();
					return false;
				}

				Cmd.State = -1;
				State = 1;

				if (Chunked)
					PQsetSingleRowMode(Base);

				Safe.unlock();
				return true;
#else
				return false;
#endif
			}
			bool Connection::NextSync()
			{
				return Next().Get();
			}
			Result& Connection::GetCurrent()
			{
				return Cmd.Prev;
			}
			std::string Connection::GetErrorMessage() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return std::string();

				char* Text = PQerrorMessage(Base);
				if (!Text)
					return std::string();

				return Text;
#else
				return std::string();
#endif
			}
			std::string Connection::EscapeLiteral(const char* Data, size_t Size)
			{
				if (!Data || !Size)
					return std::string();

#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return std::string(Data, Size);

				char* Result = PQescapeLiteral(Base, Data, Size);
				if (!Result)
					return std::string();

				std::string Copy(Result);
				PQfreemem(Result);

				return Copy;
#else
				return std::string(Data, Size);
#endif
			}
			std::string Connection::EscapeLiteral(const std::string& Value)
			{
				return EscapeLiteral(Value.c_str(), Value.size());
			}
			std::string Connection::EscapeIdentifier(const char* Data, size_t Size)
			{
				if (!Data || !Size)
					return std::string();

#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return std::string(Data, Size);

				char* Result = PQescapeIdentifier(Base, Data, Size);
				if (!Result)
					return std::string();

				std::string Copy(Result);
				PQfreemem(Result);

				return Copy;
#else
				return std::string(Data, Size);
#endif
			}
			std::string Connection::EscapeIdentifier(const std::string& Value)
			{
				return EscapeIdentifier(Value.c_str(), Value.size());
			}
			unsigned char* Connection::EscapeBytea(const unsigned char* Data, size_t Size, size_t* OutSize)
			{
				if (!Data || !Size || !OutSize)
					return nullptr;

#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return nullptr;

				unsigned char* Result = PQescapeByteaConn(Base, Data, Size, OutSize);
				if (!Result)
					return nullptr;

				unsigned char* Copy = (unsigned char*)TH_MALLOC(*OutSize * sizeof(unsigned char));
				memcpy(Copy, Result, *OutSize * sizeof(unsigned char));
				PQfreemem(Result);

				return Copy;
#else
				return nullptr;
#endif
			}
			unsigned char* Connection::UnescapeBytea(const unsigned char* Data, size_t* OutSize)
			{
				return ToBytea((const char*)Data, OutSize);
			}
			std::string Connection::GetEncoding() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return std::string();

				int Code = PQclientEncoding(Base);
				const char* Name = pg_encoding_to_char(Code);

				return Name ? Name : std::string();
#else
				return std::string();
#endif
			}
			int Connection::GetServerVersion() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return -1;

				return PQserverVersion(Base);
#else
				return -1;
#endif
			}
			int Connection::GetBackendPid() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return -1;

				return PQbackendPID(Base);
#else
				return -1;
#endif
			}
			ConnectionState Connection::GetStatus() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return ConnectionState::Bad;
				
				return (ConnectionState)PQstatus(Base);
#else
				return ConnectionState::Bad;
#endif
			}
			TransactionState Connection::GetTransactionState() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return TransactionState::None;

				return (TransactionState)PQtransactionStatus(Base);
#else
				return TransactionState::None;
#endif
			}
			TConnection* Connection::Get() const
			{
				return Base;
			}
			bool Connection::IsConnected() const
			{
				return State != -1;
			}

			Queue::Queue() : Connected(false)
			{
				Driver::Create();
			}
			Queue::~Queue()
			{
				Disconnect();
				Driver::Release();
			}
			Core::Async<bool> Queue::Connect(const Address& URI)
			{
#ifdef TH_HAS_POSTGRESQL
				if (Connected || URI.Get().empty())
					return Core::Async<bool>::Store(false);

				return Core::Async<bool>([this, URI](Core::Async<bool>& Future)
				{
					const char** Keys = URI.CreateKeys();
					const char** Values = URI.CreateValues();
					PGPing Result = PQpingParams(Keys, Values, 0);
					TH_FREE(Keys);
					TH_FREE(Values);

					switch (Result)
					{
						case PGPing::PQPING_OK:
							Source = URI;
							Connected = true;
							Future.Set(true);
							break;
						case PGPing::PQPING_REJECT:
							TH_ERROR("[pg] server connection rejected");
							Future.Set(false);
							break;
						case PGPing::PQPING_NO_ATTEMPT:
							TH_ERROR("[pg] invalid params");
							Future.Set(false);
							break;
						case PGPing::PQPING_NO_RESPONSE:
							TH_ERROR("[pg] couldn't connect to server");
							Future.Set(false);
							break;
						default:
							Future.Set(false);
							break;
					}
				});
#else
                return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Queue::Disconnect()
			{
				if (!Connected)
					return Core::Async<bool>::Store(false);

				return Core::Async<bool>([this](Core::Async<bool>& Future)
				{
					Safe.lock();
					for (auto& Base : Active)
						Clear(Base);

					for (auto& Base : Inactive)
					{
						Clear(Base);
						TH_RELEASE(Base);
					}

					Connected = false;
					Active.clear();
					Inactive.clear();
					Safe.unlock();
					Future.Set(true);
				});
			}
			void Queue::Clear(Connection* Client)
			{
				if (!Client)
					return;
#ifdef TH_HAS_POSTGRESQL
				if (Client->Base != nullptr)
					PQfinish(Client->Base);
#endif
				Client->Base = nullptr;
				Client->Master = nullptr;
				Client->State = -1;
			}
			bool Queue::Push(Connection** Client)
			{
				if (!Client || !*Client)
					return false;

				Safe.lock();
				if (Active.find(*Client) == Active.end())
				{
					Inactive.erase(*Client);
					Clear(*Client);
					TH_RELEASE(*Client);
				}
				else
				{
					Active.erase(*Client);
					Inactive.insert(*Client);
				}
				Safe.unlock();

				return true;
			}
			Connection* Queue::Pop()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Connected)
					return nullptr;

				Safe.lock();
				if (!Inactive.empty())
				{
					Connection* Result = *Inactive.begin();
					Inactive.erase(Inactive.begin());
					Active.insert(Result);
					Safe.unlock();

					return Result;
				}

				Connection* Result = new Connection();
				Active.insert(Result);
				Safe.unlock();

				if (Result->Connect(Source).Get())
				{
					Result->Master = this;
					return Result;
				}

				Safe.lock();
				Active.erase(Result);
				Safe.unlock();

				TH_RELEASE(Result);
				return nullptr;
#else
				return nullptr;
#endif
			}

			void Driver::Create()
			{
#ifdef TH_HAS_POSTGRESQL
				if (State <= 0)
				{
					Queries = new std::unordered_map<std::string, Sequence>();
					Listeners = new std::unordered_set<Connection*>();
					Safe = TH_NEW(std::mutex);
					State = 1;

					Assign(Core::Schedule::Get());
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

					if (Listeners != nullptr)
					{
						delete Listeners;
						Listeners = nullptr;
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
			void Driver::Assign(Core::Schedule* Queue)
			{
				if (Queue != nullptr)
				{
					if (!Active)
					{
						Queue->SetTask(Driver::Resolve);
						Active = true;
					}
				}
				else if (Active)
					Active = false;
			}
			void Driver::Resolve()
			{
				Core::Schedule* Queue = Core::Schedule::Get();
				if (!Queue)
					return;

				if (Dispatch() <= 0)
					std::this_thread::sleep_for(std::chrono::microseconds(100));

				if (Queue->IsActive() && Active)
					Queue->SetTask(Driver::Resolve);
			}
			int Driver::Dispatch()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Safe || !Listeners || Listeners->empty())
					return -1;

				int Count = 0;
				Safe->lock();
				for (auto It = Listeners->begin(); It != Listeners->end(); It++)
				{
					Connection* Src = *It;
					if (!Src)
						continue;

					int Consume = PQconsumeInput(Src->Base);
					if (Consume != 1)
						continue;

					int Busy = PQisBusy(Src->Base);
					if (Busy == 1)
						continue;

					PGnotify* Notification = PQnotifies(Src->Base);
					if (Notification != nullptr)
					{
						Src->Safe.lock();
						OnNotification Callback = Src->Callback;
						Src->Safe.unlock();

						if (Callback)
							Callback(Notify(Notification));
						else
							PQfreeNotify(Notification);
						Count++;
					} else if (Src->State <= 1)
						continue;
					else
						Count++;

					Result Output(PQgetResult(Src->Base));
					bool Continue = (Output.Get() != nullptr);
					if (!Continue)
						PQflush(Src->Base);

					if (Src->State == 2)
					{
						if (Src->Cmd.State == -1)
						{
							Src->Safe.lock();
							Src->Cmd.Prev.Release();
							Src->Cmd.Prev = Output;
							Src->Cmd.State = 0;
							Src->Safe.unlock();
						}
						else if (Src->Cmd.State == 0)
						{
							Src->Safe.lock();
							Core::Async<bool> Subresult(std::move(Src->Cmd.Future));
							Src->Cmd.Next.Release();
							Src->Cmd.Next = Output;
							Src->Cmd.State = 1;
							Src->State = (Continue ? 1 : 0);
							Src->Safe.unlock();
							Subresult.Set(Continue);
						}
						else if (Src->Cmd.State == 1)
						{
							Src->Safe.lock();
							Src->Cmd.Prev.Release();
							Src->Cmd.Prev = Src->Cmd.Next;
							Src->Cmd.State = 0;
							Src->Safe.unlock();
						}
					}
					else if (Src->State == 3)
					{
						if (Continue)
						{
							Output.Release();
							continue;
						}

						Src->Safe.lock();
						Core::Async<bool> Subresult(std::move(Src->Cmd.Future));
						Src->State = 0;
						Src->Safe.unlock();
						Subresult.Set(true);
					}
				}
				Safe->unlock();

				return Count;
#else
				return -1;
#endif
			}
			bool Driver::Listen(Connection* Value)
			{
				if (!Listeners || !Safe || !Value)
					return false;

				Safe->lock();
				Listeners->insert(Value);
				Safe->unlock();

				return true;
			}
			bool Driver::Unlisten(Connection* Value)
			{
				if (!Listeners || !Safe || !Value)
					return false;

				Safe->lock();
				Listeners->erase(Value);
				Safe->unlock();

				return true;
			}
			bool Driver::AddQuery(const std::string& Name, const char* Buffer, size_t Size)
			{
				if (!Queries || !Safe || Name.empty() || !Buffer || !Size)
					return false;

				Sequence Result;
				Result.Request.assign(Buffer, Size);

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
                                Next.Key = Base.R().substr((size_t)Arg + 2, (size_t)Index - (size_t)Arg - 2);
								Next.Offset = (size_t)Arg;
								Result.Positions.push_back(std::move(Next));
								Base.RemovePart(Arg, Index + 1);
								Index -= Index - Arg - 1; Args++;
							}

							Spec = false;
							Index++;
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
					else if (!Lock && (V == '\n' || V == '\r' || V == '\t' || (L == ' ' && V == ' ')))
					{
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
					AddQuery(Base.Substring(1).R(), Buffer, Size);
					TH_FREE(Buffer);
				}

				return true;
			}
			bool Driver::RemoveQuery(const std::string& Name)
			{
				if (!Queries || !Safe)
					return false;

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
			std::string Driver::GetQuery(Connection* Base, const std::string& Name, Core::DocumentArgs* Map, bool Once)
			{
				if (!Queries || !Safe)
					return std::string();

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

				Sequence Origin = It->second;
				size_t Offset = 0;
				Safe->unlock();

				Core::Parser Result(&Origin.Request);
				for (auto& Word : Origin.Positions)
				{
					auto It = Map->find(Word.Key);
					if (It == Map->end())
						continue;

					std::string Value = GetSQL(Base, It->second, Word.Escape);
					if (Value.empty())
						continue;

					Result.Insert(Value, Word.Offset + Offset);
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
					TH_ERROR("could not construct query: \"%s\"", Name.c_str());

				return Data;
			}
			std::vector<std::string> Driver::GetQueries()
			{
				std::vector<std::string> Result;
				if (!Queries || !Safe)
					return Result;

				Safe->lock();
				Result.reserve(Queries->size());
				for (auto& Item : *Queries)
					Result.push_back(Item.first);
				Safe->unlock();

				return Result;
			}
			std::string Driver::GetCharArray(Connection* Base, const std::string& Src)
			{
#ifdef TH_HAS_POSTGRESQL
				if (Src.empty())
					return "''";

				if (!Base || !Base->Get())
				{
					Core::Parser Dest(Src);
					Dest.Replace(TH_PREFIX_STR, TH_PREFIX_STR "\\");
					return Dest.Insert('\'', 0).Append('\'').R();
				}

				char* Subresult = PQescapeLiteral(Base->Get(), Src.c_str(), Src.size());
				std::string Result(Subresult);
				PQfreemem(Subresult);

				Core::Parser(&Result).Replace(TH_PREFIX_STR, TH_PREFIX_STR "\\");
				return Result;
#else
				return "'" + Src + "'";
#endif
			}
			std::string Driver::GetByteArray(Connection* Base, const char* Src, size_t Size)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Src || !Size)
					return "''";

				if (!Base || !Base->Get())
					return "'\\x" + Compute::Common::HexEncode(Src, Size) + "'::bytea";

				size_t Length = 0;
				char* Subresult = (char*)PQescapeByteaConn(Base->Get(), (unsigned char*)Src, Size, &Length);
				std::string Result(Subresult, Length);
				PQfreemem((unsigned char*)Subresult);

				return Result;
#else
				return "'\\x" + Compute::Common::HexEncode(Src, Size) + "'::bytea";
#endif
			}
			std::string Driver::GetSQL(Connection* Base, Core::Document* Source, bool Escape)
			{
				if (!Source)
					return "NULL";

				Core::Document* Parent = Source->GetParent();
				bool Array = (Parent && Parent->Value.GetType() == Core::VarType::Array);

				switch (Source->Value.GetType())
				{
					case Core::VarType::Object:
					{
						if (Array)
							return "";

						std::string Result;
						Core::Document::WriteJSON(Source, [&Result](Core::VarForm, const char* Buffer, int64_t Length)
						{
							if (Buffer != nullptr && Length > 0)
								Result.append(Buffer, Length);
						});

						return GetCharArray(Base, Result);
					}
					case Core::VarType::Array:
					{
						std::string Result = (Array ? "[" : "ARRAY[");
						for (auto* Node : *Source->GetNodes())
							Result.append(GetSQL(Base, Node, true)).append(1, ',');

						if (!Source->GetNodes()->empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "]";
					}
					case Core::VarType::String:
					{
						std::string Result(GetCharArray(Base, Source->Value.GetBlob()));
						if (Escape)
							return Result;

						if (Result.front() != '\'' || Result.back() != '\'')
							return Result;

						if (Result.size() == 2)
							return "";

                        Result = Result.substr(1, Result.size() - 2);
						return Result;
					}
					case Core::VarType::Integer:
						return std::to_string(Source->Value.GetInteger());
					case Core::VarType::Number:
						return std::to_string(Source->Value.GetNumber());
					case Core::VarType::Boolean:
						return Source->Value.GetBoolean() ? "TRUE" : "FALSE";
					case Core::VarType::Decimal:
					{
                        std::string Result(GetCharArray(Base, Source->Value.GetDecimal().ToString()));
						return (Result.size() >= 2 ? Result.substr(1, Result.size() - 2) : Result);
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
			std::unordered_set<Connection*>* Driver::Listeners = nullptr;
			std::mutex* Driver::Safe = nullptr;
			std::atomic<bool> Driver::Active(false);
			std::atomic<int> Driver::State(0);
		}
	}
}