#include "pdb.h"
#ifdef VI_POSTGRESQL
#include <libpq-fe.h>
#endif
#ifdef VI_OPENSSL
extern "C"
{
#include <openssl/opensslv.h>
}
#endif

namespace Vitex
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

			static void PQlogNotice(void*, const char* Message)
			{
				if (!Message || Message[0] == '\0')
					return;

				Core::String Buffer(Message);
				for (auto& Item : Core::Stringify::Split(Buffer, '\n'))
				{
					VI_DEBUG("[pq] %s", Item.c_str());
					(void)Item;
				}
			}
			static void PQlogNoticeOf(TConnection* Connection)
			{
#ifdef VI_SQLITE
				VI_ASSERT(Connection != nullptr, "base should be set");
				char* Message = PQerrorMessage(Connection);
				if (!Message || Message[0] == '\0')
					return;

				Core::String Buffer(Message);
				Buffer.erase(Buffer.size() - 1);
				for (auto& Item : Core::Stringify::Split(Buffer, '\n'))
				{
					VI_DEBUG("[pq] %s", Item.c_str());
					(void)Item;
				}
#endif
			}
			static Core::Schema* ToSchema(const char* Data, int Size, uint32_t Id);
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
						Core::Memory::Release(Next.first);
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
			static Core::Variant ToVariant(const char* Data, int Size, uint32_t Id)
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

						return Core::Var::String(std::string_view(Data, (size_t)Size));
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

						return Core::Var::String(std::string_view(Data, (size_t)Size));
					}
					case OidType::Money:
					case OidType::Numeric:
						return Core::Var::DecimalString(Core::String(Data, (size_t)Size));
					case OidType::Bytea:
					{
						size_t Length = 0;
						uint8_t* Buffer = PQunescapeBytea((const uint8_t*)Data, &Length);
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
						return Core::Var::String(std::string_view(Data, (size_t)Size));
				}
			}
			static Core::Schema* ToArray(const char* Data, int Size, uint32_t Id)
			{
				std::pair<Core::Schema*, Oid> Context;
				Context.first = Core::Var::Set::Array();
				Context.second = Id;

				ArrayFilter Filter(Data, (size_t)Size);
				if (!Filter.Parse(&Context, ToArrayField))
				{
					Core::Memory::Release(Context.first);
					return new Core::Schema(Core::Var::String(std::string_view(Data, (size_t)Size)));
				}

				return Context.first;
			}
			Core::Schema* ToSchema(const char* Data, int Size, uint32_t Id)
			{
				if (!Data)
					return nullptr;

				OidType Type = (OidType)Id;
				switch (Type)
				{
					case OidType::JSON:
					case OidType::JSONB:
					{
						auto Result = Core::Schema::ConvertFromJSON(std::string_view(Data, (size_t)Size));
						if (Result)
							return *Result;

						return new Core::Schema(Core::Var::String(std::string_view(Data, (size_t)Size)));
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
			DatabaseException::DatabaseException(TConnection* Connection)
			{
#ifdef VI_SQLITE
				VI_ASSERT(Connection != nullptr, "base should be set");
				char* NewMessage = PQerrorMessage(Connection);
				if (!NewMessage || NewMessage[0] == '\0')
					return;

				Core::String Buffer(NewMessage);
				Buffer.erase(Buffer.size() - 1);
				for (auto& Item : Core::Stringify::Split(Buffer, '\n'))
				{
					if (Item.empty())
						continue;
					if (Message.empty())
						Message += Item;
					else
						Message += ", " + Item;
				}
#else
				Message = "not supported";
#endif
			}
			DatabaseException::DatabaseException(Core::String&& NewMessage)
			{
				Message = std::move(NewMessage);
			}
			const char* DatabaseException::type() const noexcept
			{
				return "postgresql_error";
			}

			Address::Address()
			{
			}
			void Address::Override(const std::string_view& Key, const std::string_view& Value)
			{
				Params[Core::String(Key)] = Value;
			}
			bool Address::Set(AddressOp Key, const std::string_view& Value)
			{
				Core::String Name = Core::String(GetKeyName(Key));
				if (Name.empty())
					return false;

				Params[Name] = Value;
				return true;
			}
			Core::String Address::Get(AddressOp Key) const
			{
				auto It = Params.find(Core::HglCast(GetKeyName(Key)));
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
			std::string_view Address::GetKeyName(AddressOp Key)
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
				const char** Result = Core::Memory::Allocate<const char*>(sizeof(const char*) * (Params.size() + 1));
				size_t Index = 0;

				for (auto& Key : Params)
					Result[Index++] = Key.first.c_str();

				Result[Index] = nullptr;
				return Result;
			}
			const char** Address::CreateValues() const
			{
				const char** Result = Core::Memory::Allocate<const char*>(sizeof(const char*) * (Params.size() + 1));
				size_t Index = 0;

				for (auto& Key : Params)
					Result[Index++] = Key.second.c_str();

				Result[Index] = nullptr;
				return Result;
			}
			ExpectsDB<Address> Address::FromURL(const std::string_view& Location)
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Core::Stringify::IsCString(Location), "location should be set");
				char* Message = nullptr;
				PQconninfoOption* Result = PQconninfoParse(Location.data(), &Message);
				if (!Result)
				{
					DatabaseException Exception("address parser error: " + Core::String(Message));
					PQfreemem(Message);
					return Exception;
				}

				Address Target;
				size_t Index = 0;
				while (Result[Index].keyword != nullptr)
				{
					auto& Info = Result[Index];
					Target.Params[Info.keyword] = Info.val ? Info.val : "";
					Index++;
				}
				PQconninfoFree(Result);
				return Target;
#else
				return DatabaseException("address parser: not supported");
#endif
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

				auto Result = Core::Schema::ConvertFromJSON(Data);
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
			Core::String Column::GetValueText() const
			{
				return Core::String(GetRaw(), RawSize());
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
			size_t Column::Index() const
			{
				return ColumnIndex;
			}
			size_t Column::Size() const
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
			size_t Column::RawSize() const
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
			bool Column::Nullable() const
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
					return Core::Var::Set::Array();

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
			size_t Row::Index() const
			{
				return RowIndex;
			}
			size_t Row::Size() const
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
			Column Row::GetColumn(const std::string_view& Name) const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return Column(Base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				int Index = PQfnumber(Base, Name.data());
				if (Index < 0)
					return Column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				return Column(Base, RowIndex, (size_t)Index);
#else
				return Column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());
#endif
			}
			bool Row::GetColumns(Column* Output, size_t OutputSize) const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Output != nullptr && OutputSize > 0, "output should be valid");
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");

				OutputSize = std::min(Size(), OutputSize);
				for (size_t i = 0; i < OutputSize; i++)
					Output[i] = Column(Base, RowIndex, i);

				return true;
#else
				return false;
#endif
			}

			Response::Response() : Response(nullptr)
			{

			}
			Response::Response(TResponse* NewBase) : Base(NewBase), Failure(false)
			{
			}
			Response::Response(Response&& Other) : Base(Other.Base), Failure(Other.Failure)
			{
				Other.Base = nullptr;
				Other.Failure = false;
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
				Failure = Other.Failure;
				Other.Base = nullptr;
				Other.Failure = false;
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
			Core::Vector<Core::String> Response::GetColumns() const
			{
				Core::Vector<Core::String> Columns;
#ifdef VI_POSTGRESQL
				if (!Base)
					return Columns;

				int ColumnsSize = PQnfields(Base);
				if (ColumnsSize <= 0)
					return Columns;

				Columns.reserve((size_t)ColumnsSize);
				for (int j = 0; j < ColumnsSize; j++)
				{
					char* Name = PQfname(Base, j);
					Columns.emplace_back(Name ? Name : Core::ToString(j));
				}
#endif
				return Columns;
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
			int Response::GetNameIndex(const std::string_view& Name) const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");

				return PQfnumber(Base, Name.data());
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
			size_t Response::AffectedRows() const
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
			size_t Response::Size() const
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
				if (Empty())
					return Row(nullptr, std::numeric_limits<size_t>::max());

				return GetRow(0);
			}
			Row Response::Back() const
			{
				if (Empty())
					return Row(nullptr, std::numeric_limits<size_t>::max());

				return GetRow(Size() - 1);
			}
			Response Response::Copy() const
			{
#ifdef VI_POSTGRESQL
				Response Result;
				if (!Base)
					return Result;

				Result.Failure = Error();
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
			bool Response::Empty() const
			{
#ifdef VI_POSTGRESQL
				if (!Base)
					return true;

				return PQntuples(Base) <= 0;
#else
				return true;
#endif
			}
			bool Response::Error() const
			{
				if (Failure)
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
			bool Cursor::Success() const
			{
				return !Error();
			}
			bool Cursor::Empty() const
			{
				if (Base.empty())
					return true;

				for (auto& Item : Base)
				{
					if (!Item.Empty())
						return false;
				}

				return true;
			}
			bool Cursor::Error() const
			{
				if (Base.empty())
					return true;

				for (auto& Item : Base)
				{
					if (Item.Error())
						return true;
				}

				return false;
			}
			size_t Cursor::Size() const
			{
				return Base.size();
			}
			size_t Cursor::AffectedRows() const
			{
				size_t Size = 0;
				for (auto& Item : Base)
					Size += Item.AffectedRows();
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

			Connection::Connection(TConnection* NewBase, socket_t Fd) : Base(NewBase), Stream(new Socket(Fd)), Current(nullptr), State(QueryState::Idle), Session(false)
			{
			}
			Connection::~Connection() noexcept
			{
				Core::Memory::Release(Stream);
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
			TransactionState Connection::GetTxState() const
			{
#ifdef VI_POSTGRESQL
				PGTransactionStatusType Type = PQtransactionStatus(Base);
				switch (Type)
				{
					case PQTRANS_IDLE:
						return TransactionState::Idle;
					case PQTRANS_ACTIVE:
						return TransactionState::Active;
					case PQTRANS_INTRANS:
						return TransactionState::In_Transaction;
					case PQTRANS_INERROR:
						return TransactionState::In_Error;
					case PQTRANS_UNKNOWN:
					default:
						return TransactionState::None;
				}
#else
				return TransactionState::None;
#endif
			}
			bool Connection::InSession() const
			{
				return Session;
			}
			bool Connection::Busy() const
			{
				return Current != nullptr;
			}

			Request::Request(const std::string_view& Commands, Caching Status) : Command(Commands.begin(), Commands.end()), Time(Core::Schedule::GetClock()), Session(0), Result(nullptr, Status), Options(0)
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
			bool Request::Pending() const
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
					Core::Memory::Release(Item.second);
				}
#endif
				for (auto* Item : Requests)
				{
					Item->Failure();
					Core::Memory::Release(Item);
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
			uint64_t Cluster::AddChannel(const std::string_view& Name, const OnNotification& NewCallback)
			{
				VI_ASSERT(NewCallback != nullptr, "callback should be set");

				uint64_t Id = Channel++;
				Core::UMutex<std::mutex> Unique(Update);
				Listeners[Core::String(Name)][Id] = NewCallback;
				return Id;
			}
			bool Cluster::RemoveChannel(const std::string_view& Name, uint64_t Id)
			{
				Core::UMutex<std::mutex> Unique(Update);
				auto& Base = Listeners[Core::String(Name)];
				auto It = Base.find(Id);
				if (It == Base.end())
					return false;

				Base.erase(It);
				return true;
			}
			ExpectsPromiseDB<SessionId> Cluster::TxBegin(Isolation Type)
			{
				switch (Type)
				{
					case Isolation::Serializable:
						return TxStart("BEGIN ISOLATION LEVEL SERIALIZABLE");
					case Isolation::RepeatableRead:
						return TxStart("BEGIN ISOLATION LEVEL REPEATABLE READ");
					case Isolation::ReadUncommited:
						return TxStart("BEGIN ISOLATION LEVEL READ UNCOMMITTED");
					case Isolation::ReadCommited:
					default:
						return TxStart("BEGIN");
				}
			}
			ExpectsPromiseDB<SessionId> Cluster::TxStart(const std::string_view& Command)
			{
				return Query(Command, (size_t)QueryOp::TransactionStart).Then<ExpectsDB<SessionId>>([](ExpectsDB<Cursor>&& Result) -> ExpectsDB<SessionId>
				{
					if (!Result)
						return Result.Error();
					else if (Result->Success())
						return Result->GetExecutor();

					return DatabaseException("transaction start error");
				});
			}
			ExpectsPromiseDB<void> Cluster::TxEnd(const std::string_view& Command, SessionId Session)
			{
				return Query(Command, (size_t)QueryOp::TransactionEnd, Session).Then<ExpectsDB<void>>([](ExpectsDB<Cursor>&& Result) -> ExpectsDB<void>
				{
					if (!Result)
						return Result.Error();
					else if (Result->Success())
						return Core::Expectation::Met;

					return DatabaseException("transaction end error");
				});
			}
			ExpectsPromiseDB<void> Cluster::TxCommit(SessionId Session)
			{
				return TxEnd("COMMIT", Session);
			}
			ExpectsPromiseDB<void> Cluster::TxRollback(SessionId Session)
			{
				return TxEnd("ROLLBACK", Session);
			}
			ExpectsPromiseDB<void> Cluster::Connect(const Address& Location, size_t Connections)
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(Connections > 0, "connections count should be at least 1");
				if (!Core::OS::Control::Has(Core::AccessOption::Net))
					return ExpectsPromiseDB<void>(DatabaseException("connect failed: permission denied"));
#if OPENSSL_VERSION_MAJOR >= 3 && OPENSSL_VERSION_MINOR >= 2
				int Version = PQlibVersion();
				if (Version < 160002)
					return ExpectsPromiseDB<void>(DatabaseException(Core::Stringify::Text("connect failed: libpq <= %.2f will cause a segfault starting with openssl 3.2", (double)Version / 10000.0)));
#endif
				Core::UMutex<std::mutex> Unique(Update);
				Source = Location;
				if (!Pool.empty())
				{
					Unique.Negate();
					return Disconnect().Then<ExpectsPromiseDB<void>>([this, Location, Connections](ExpectsDB<void>&&) { return this->Connect(Location, Connections); });
				}

				Unique.Negate();
				return Core::Cotask<ExpectsDB<void>>([this, Connections]() -> ExpectsDB<void>
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

						Network::Utils::PollFd Fd;
						Fd.Fd = (socket_t)PQsocket(Base);
						Queue[Fd.Fd] = Base;
						Sockets.emplace_back(std::move(Fd));

						VI_DEBUG("[pq] try connect to %s on 0x%" PRIXPTR, Address.c_str(), Base);
						if (PQstatus(Base) == ConnStatusType::CONNECTION_BAD)
						{
							Error = Base;
							goto Failure;
						}
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
										PQsetNoticeProcessor(Base, PQlogNotice, nullptr);
										PQsetnonblocking(Base, 1);

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
							for (auto& Base : Queue)
								PQfinish(Base.second);
							Core::Memory::Deallocate(Keys);
							Core::Memory::Deallocate(Values);
							return DatabaseException(Core::Stringify::Text("connection to %s has timed out (took %" PRIu64 " seconds)", Address.c_str(), ConnectTimeout));
						}
					} while (!Sockets.empty() && Network::Utils::Poll(Sockets.data(), (int)Sockets.size(), 50) >= 0);

					Core::Memory::Deallocate(Keys);
					Core::Memory::Deallocate(Values);
					return Core::Expectation::Met;
				Failure:
					DatabaseException Exception = DatabaseException(Error);
					for (auto& Base : Queue)
						PQfinish(Base.second);
					
					Core::Memory::Deallocate(Keys);
					Core::Memory::Deallocate(Values);
					return Exception;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException("connect: not supported"));
#endif
			}
			ExpectsPromiseDB<void> Cluster::Disconnect()
			{
#ifdef VI_POSTGRESQL
				if (Pool.empty())
					return ExpectsPromiseDB<void>(DatabaseException("disconnect: not connected"));

				return Core::Cotask<ExpectsDB<void>>([this]() -> ExpectsDB<void>
				{
					Core::UMutex<std::mutex> Unique(Update);
					for (auto& Item : Pool)
					{
						Item.first->ClearEvents(false);
						PQfinish(Item.second->Base);
						Core::Memory::Release(Item.second);
					}

					Pool.clear();
					return Core::Expectation::Met;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException("disconnect: not supported"));
#endif
			}
			ExpectsPromiseDB<void> Cluster::Listen(const Core::Vector<Core::String>& Channels)
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
					return ExpectsPromiseDB<void>(Core::Expectation::Met);

				Core::String Command;
				for (auto& Item : Actual)
					Command += "LISTEN " + Item + ';';

				return Query(Command).Then<ExpectsDB<void>>([this, Actual = std::move(Actual)](ExpectsDB<Cursor>&& Result) mutable -> ExpectsDB<void>
				{
					if (!Result)
						return Result.Error();

					Connection* Base = Result->GetExecutor();
					if (!Base || !Result->Success())
						return DatabaseException("listen error");

					Core::UMutex<std::mutex> Unique(Update);
					for (auto& Item : Actual)
						Base->Listens.insert(Item);
					return Core::Expectation::Met;
				});
			}
			ExpectsPromiseDB<void> Cluster::Unlisten(const Core::Vector<Core::String>& Channels)
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
					return ExpectsPromiseDB<void>(Core::Expectation::Met);

				return Core::Coasync<ExpectsDB<void>>([this, Commands = std::move(Commands)]() mutable -> ExpectsPromiseDB<void>
				{
					size_t Count = 0;
					for (auto& Next : Commands)
					{
						auto Status = VI_AWAIT(Query(Next.second, (size_t)QueryOp::TransactionAlways, Next.first));
						Count += Status && Status->Success() ? 1 : 0;
					}
					if (!Count)
						Coreturn DatabaseException("unlisten error");

					Coreturn Core::Expectation::Met;
				});
			}
			ExpectsPromiseDB<Cursor> Cluster::EmplaceQuery(const std::string_view& Command, Core::SchemaList* Map, size_t Opts, SessionId Session)
			{
				auto Template = Driver::Get()->Emplace(this, Command, Map, !(Opts & (size_t)QueryOp::ReuseArgs));
				if (!Template)
					return ExpectsPromiseDB<Cursor>(Template.Error());

				return Query(*Template, Opts, Session);
			}
			ExpectsPromiseDB<Cursor> Cluster::TemplateQuery(const std::string_view& Name, Core::SchemaArgs* Map, size_t Opts, SessionId Session)
			{
				VI_DEBUG("[pq] template query %s", Name.empty() ? "empty-query-name" : Core::String(Name).c_str());
				auto Template = Driver::Get()->GetQuery(this, Name, Map, !(Opts & (size_t)QueryOp::ReuseArgs));
				if (!Template)
					return ExpectsPromiseDB<Cursor>(Template.Error());

				return Query(*Template, Opts, Session);
			}
			ExpectsPromiseDB<Cursor> Cluster::Query(const std::string_view& Command, size_t Opts, SessionId Session)
			{
				VI_ASSERT(!Command.empty(), "command should not be empty");
				Core::String Reference;
				bool MayCache = Opts & (size_t)QueryOp::CacheShort || Opts & (size_t)QueryOp::CacheMid || Opts & (size_t)QueryOp::CacheLong;
				if (MayCache)
				{
					Cursor Result(nullptr, Caching::Cached);
					Reference = GetCacheOid(Command, Opts);
					if (GetCache(Reference, &Result))
					{
						Driver::Get()->LogQuery(Command);
						VI_DEBUG("[pq] OK execute on NULL (memory-cache)");
						return ExpectsPromiseDB<Cursor>(std::move(Result));
					}
				}

				Request* Next = new Request(Command, MayCache ? Caching::Miss : Caching::Never);
				Next->Session = Session;
				Next->Options = Opts;
				if (!Reference.empty())
					Next->Callback = [this, Reference, Opts](Cursor& Data) { SetCache(Reference, &Data, Opts); };

				ExpectsPromiseDB<Cursor> Future = Next->Future;
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
				(void)IsInQueue;
#ifndef NDEBUG
				if (!IsInQueue || Next->Session == 0)
					return Future;

				if (!ValidateTransaction(Command, Next))
				{
					Future.Set(Cursor());
					Core::Memory::Release(Next);
				}
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
			Connection* Cluster::GetAnyConnection() const
			{
				for (auto& Item : Pool)
					return Item.second;

				return nullptr;
			}
			Connection* Cluster::IsListens(const std::string_view& Name)
			{
				auto Copy = Core::HglCast(Name);
				for (auto& Item : Pool)
				{
					if (Item.second->Listens.count(Copy) > 0)
						return Item.second;
				}

				return nullptr;
			}
			Core::String Cluster::GetCacheOid(const std::string_view& Payload, size_t Opts)
			{
				auto Hash = Compute::Crypto::HashHex(Compute::Digests::SHA256(), Payload);
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
			bool Cluster::GetCache(const std::string_view& CacheOid, Cursor* Data)
			{
				VI_ASSERT(!CacheOid.empty(), "cache oid should not be empty");
				VI_ASSERT(Data != nullptr, "cursor should be set");

				Core::UMutex<std::mutex> Unique(Cache.Context);
				auto It = Cache.Objects.find(Core::HglCast(CacheOid));
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
			void Cluster::SetCache(const std::string_view& CacheOid, Cursor* Data, size_t Opts)
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

				auto It = Cache.Objects.find(Core::HglCast(CacheOid));
				if (It != Cache.Objects.end())
				{
					It->second.second = Data->Copy();
					It->second.first = Timeout;
				}
				else
					Cache.Objects[Core::String(CacheOid)] = std::make_pair(Timeout, Data->Copy());
			}
			void Cluster::TryUnassign(Connection* Base, Request* Context)
			{
				if (!(Context->Options & (size_t)QueryOp::TransactionEnd))
					return;

				VI_DEBUG("[pq] release transaction on 0x%" PRIXPTR, (uintptr_t)Base);
				Base->Session = false;
			}
			bool Cluster::ValidateTransaction(const std::string_view& Command, Request* Next)
			{
				Core::String Tx = Core::String(Command);
				Core::Stringify::Trim(Tx);
				Core::Stringify::ToUpper(Tx);

				if (Tx != "COMMIT" && Tx != "ROLLBACK")
					return true;
				{
					Core::UMutex<std::mutex> Unique(Update);
					for (auto& Item : Pool)
					{
						if (Item.second->Session && Item.second == Next->Session)
							return true;
					}

					auto It = std::find(Requests.begin(), Requests.end(), Next);
					if (It != Requests.end())
						Requests.erase(It);
				}
				VI_ASSERT(false, "transaction %" PRIu64 " does not exist", (void*)Next->Session);
				return false;
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
					Unique.Negate();
					Current->Failure();
					Unique.Negate();
					VI_DEBUG("[pqerr] query reset on 0x%" PRIXPTR ": connection lost", (uintptr_t)Target->Base);
					Core::Memory::Release(Current);
				}

				Target->Stream->ClearEvents(false);
				PQfinish(Target->Base);

				VI_DEBUG("[pq] try reconnect on 0x%" PRIXPTR, (uintptr_t)Target->Base);
				Target->Base = PQconnectdbParams(Keys, Values, 0);
				Core::Memory::Deallocate(Keys);
				Core::Memory::Deallocate(Values);

				if (!Target->Base || PQstatus(Target->Base) != ConnStatusType::CONNECTION_OK)
				{
					if (Target != nullptr)
						PQlogNoticeOf(Target->Base);

					Core::Codefer([this, Target]() { Reestablish(Target); });
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
				VI_DEBUG("[pq] execute query on 0x%" PRIXPTR "%s: %.64s%s", (uintptr_t)Base, Base->Session ? " (transaction)" : "", Base->Current->Command.data(), Base->Current->Command.size() > 64 ? " ..." : "");

				if (PQsendQuery(Base->Base, Base->Current->Command.data()) == 1)
				{
					Flush(Base, false);
					return true;
				}

				Core::UPtr<Request> Item = Base->Current;
				Base->Current = nullptr;
				PQlogNoticeOf(Base->Base);
				Unique.Negate();
				Item->Failure();
				Unique.Negate();
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
					Core::Codefer([this, Source]() { Reestablish(Source); });
					return true;
				}

			Retry:
				Consume(Source, Unique);
				if (PQconsumeInput(Source->Base) != 1)
				{
					PQlogNoticeOf(Source->Base);
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
									Core::Codefer([Event, Callback = std::move(Callback)]() { Callback(Event); });
								}
							}
							VI_DEBUG("[pq] notification on channel @%s: %s", Notification->relname, Notification->extra ? Notification->extra : "[payload]");
							PQfreeNotify(Notification);
						}
					}

					Response Frame(PQgetResult(Source->Base));
					if (Source->Current != nullptr)
					{
						if (!Frame.Exists())
						{
							ExpectsPromiseDB<Cursor> Future = Source->Current->Future;
							Cursor Results(std::move(Source->Current->Result));
							Core::UPtr<Request> Item = Source->Current;
							Source->State = QueryState::Idle;
							Source->Current = nullptr;
							PQlogNoticeOf(Source->Base);

							if (!Results.Error())
							{
								VI_DEBUG("[pq] OK execute on 0x%" PRIXPTR " (%" PRIu64 " ms)", (uintptr_t)Source, Item->GetTiming());
								TryUnassign(Source, *Item);
							}

							Unique.Negate();
							Item->Finalize(Results);
							Future.Set(std::move(Results));
							Unique.Negate();
						}
						else
							Source->Current->Result.Base.emplace_back(std::move(Frame));
					}
					else
						Source->State = (Frame.Exists() ? QueryState::Busy : QueryState::Idle);

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
				if (Base->Session || (Context->Session != nullptr && (Context->Options & (size_t)QueryOp::TransactionAlways)))
					return Base == Context->Session;

				if (!Context->Session && !(Context->Options & (size_t)QueryOp::TransactionStart))
					return true;

				for (auto& Item : Pool)
				{
					if (Item.second == Context->Session)
						return false;
				}

				VI_DEBUG("[pq] acquire transaction on 0x%" PRIXPTR, (uintptr_t)Base);
				Base->Session = true;
				return true;
			}

			ExpectsDB<Core::String> Utils::InlineArray(Cluster* Client, Core::Schema* Array)
			{
				VI_ASSERT(Client != nullptr, "cluster should be set");
				VI_ASSERT(Array != nullptr, "array should be set");

				Core::SchemaList Map;
				Core::String Def;

				for (auto* Item : Array->GetChilds())
				{
					if (Item->Value.IsObject())
					{
						if (Item->Empty())
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

				auto Result = PDB::Driver::Get()->Emplace(Client, Def, &Map, false);
				if (Result && !Result->empty() && Result->back() == ',')
					Result->erase(Result->end() - 1);

				Core::Memory::Release(Array);
				return Result;
			}
			ExpectsDB<Core::String> Utils::InlineQuery(Cluster* Client, Core::Schema* Where, const Core::UnorderedMap<Core::String, Core::String>& Whitelist, const std::string_view& Default)
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
							if (!Whitelist.empty())
							{
								auto It = Whitelist.find(Op);
								if (It != Whitelist.end() && Op.find_first_not_of(Allow) == Core::String::npos)
									Def += It->second.empty() ? Op : It->second;
							}
							else if (Op.find_first_not_of(Allow) == Core::String::npos)
								Def += Op;
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

				auto Result = PDB::Driver::Get()->Emplace(Client, Def, &Map, false);
				if (Result && Result->empty())
					Result = Core::String(Default);

				Core::Memory::Release(Where);
				return Result;
			}
			Core::String Utils::GetCharArray(Connection* Base, const std::string_view& Src) noexcept
			{
#ifdef VI_POSTGRESQL
				if (Src.empty())
					return "''";

				if (!Base)
				{
					Core::String Dest(Src);
					Core::Stringify::Replace(Dest, "'", "''");
					Dest.insert(Dest.begin(), '\'');
					Dest.append(1, '\'');
					return Dest;
				}

				char* Subresult = PQescapeLiteral(Base->GetBase(), Src.data(), Src.size());
				Core::String Result(Subresult);
				PQfreemem(Subresult);

				return Result;
#else
				Core::String Dest(Src);
				Core::Stringify::Replace(Dest, "'", "''");
				Dest.insert(Dest.begin(), '\'');
				Dest.append(1, '\'');
				return Dest;
#endif
			}
			Core::String Utils::GetByteArray(Connection* Base, const std::string_view& Src) noexcept
			{
#ifdef VI_POSTGRESQL
				if (Src.empty())
					return "''";

				if (!Base)
					return "'\\x" + Compute::Codec::HexEncode(Src) + "'::bytea";

				size_t Length = 0;
				char* Subresult = (char*)PQescapeByteaConn(Base->GetBase(), (uint8_t*)Src.data(), Src.size(), &Length);
				Core::String Result(Subresult, Length > 1 ? Length - 1 : Length);
				PQfreemem((uint8_t*)Subresult);

				Result.insert(Result.begin(), '\'');
				Result.append("'::bytea");
				return Result;
#else
				return "'\\x" + Compute::Codec::HexEncode(Src) + "'::bytea";
#endif
			}
			Core::String Utils::GetSQL(Connection* Base, Core::Schema* Source, bool Escape, bool Negate) noexcept
			{
				if (!Source)
					return "NULL";

				Core::Schema* Parent = Source->GetParent();
				switch (Source->Value.GetType())
				{
					case Core::VarType::Object:
					{
						Core::String Result;
						Core::Schema::ConvertToJSON(Source, [&Result](Core::VarForm, const std::string_view& Buffer) { Result.append(Buffer); });
						return Escape ? GetCharArray(Base, Result) : Result;
					}
					case Core::VarType::Array:
					{
						Core::String Result = (Parent && Parent->Value.GetType() == Core::VarType::Array ? "[" : "ARRAY[");
						for (auto* Node : Source->GetChilds())
							Result.append(GetSQL(Base, Node, Escape, Negate)).append(1, ',');

						if (!Source->Empty())
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
						return Core::ToString(Negate ? -Source->Value.GetNumber() : Source->Value.GetNumber());
					case Core::VarType::Boolean:
						return (Negate ? !Source->Value.GetBoolean() : Source->Value.GetBoolean()) ? "TRUE" : "FALSE";
					case Core::VarType::Decimal:
					{
						Core::Decimal Value = Source->Value.GetDecimal();
						if (Value.IsNaN())
							return "NULL";

						Core::String Result = (Negate ? '-' + Value.ToString() : Value.ToString());
						return Result.find('.') != Core::String::npos ? Result : Result + ".0";
					}
					case Core::VarType::Binary:
						return GetByteArray(Base, Source->Value.GetString());
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
			void Driver::LogQuery(const std::string_view& Command) noexcept
			{
				if (Logger)
					Logger(Command);
			}
			void Driver::AddConstant(const std::string_view& Name, const std::string_view& Value) noexcept
			{
				VI_ASSERT(!Name.empty(), "name should not be empty");
				Core::UMutex<std::mutex> Unique(Exclusive);
				Constants[Core::String(Name)] = Value;
			}
			ExpectsDB<void> Driver::AddQuery(const std::string_view& Name, const std::string_view& Buffer)
			{
				VI_ASSERT(!Name.empty(), "name should not be empty");
				if (Buffer.empty())
					return DatabaseException("import empty query error: " + Core::String(Name));

				Sequence Result;
				Result.Request.assign(Buffer);

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
						size_t Size = Item.second.End - Item.second.Start;
						Item.second.Start = (size_t)((int64_t)Item.second.Start + Offset);
						Item.second.End = (size_t)((int64_t)Item.second.End + Offset);

						auto It = Constants.find(Item.first);
						if (It == Constants.end())
							return DatabaseException("query expects @" + Item.first + " constant: " + Core::String(Name));

						Core::Stringify::ReplacePart(Base, Item.second.Start, Item.second.End, It->second);
						size_t NewSize = It->second.size();
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

				Core::Stringify::ReplaceParts(Base, Variables, "", [&Erasable](const std::string_view& Name, char Left, int Side)
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
				Queries[Core::String(Name)] = std::move(Result);
				return Core::Expectation::Met;
			}
			ExpectsDB<void> Driver::AddDirectory(const std::string_view& Directory, const std::string_view& Origin)
			{
				Core::Vector<std::pair<Core::String, Core::FileEntry>> Entries;
				if (!Core::OS::Directory::Scan(Directory, Entries))
					return DatabaseException("import directory scan error: " + Core::String(Directory));

				Core::String Path = Core::String(Directory);
				if (Path.back() != '/' && Path.back() != '\\')
					Path.append(1, '/');

				size_t Size = 0;
				for (auto& File : Entries)
				{
					Core::String Base(Path + File.first);
					if (File.second.IsDirectory)
					{
						auto Status = AddDirectory(Base, Origin.empty() ? Directory : Origin);
						if (!Status)
							return Status;
						else
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

					auto Status = AddQuery(Base, std::string_view((char*)*Buffer, Size));
					Core::Memory::Deallocate(*Buffer);
					if (!Status)
						return Status;
				}

				return Core::Expectation::Met;
			}
			bool Driver::RemoveConstant(const std::string_view& Name) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Constants.find(Core::HglCast(Name));
				if (It == Constants.end())
					return false;

				Constants.erase(It);
				return true;
			}
			bool Driver::RemoveQuery(const std::string_view& Name) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Queries.find(Core::HglCast(Name));
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
			ExpectsDB<Core::String> Driver::Emplace(Cluster* Base, const std::string_view& SQL, Core::SchemaList* Map, bool Once) noexcept
			{
				if (!Map || Map->empty())
					return Core::String(SQL);

				Connection* Remote = Base->GetAnyConnection();
				Core::String Buffer(SQL);
				Core::TextSettle Set;
				Core::String& Src = Buffer;
				size_t Offset = 0;
				size_t Next = 0;

				while ((Set = Core::Stringify::Find(Buffer, '?', Offset)).Found)
				{
					if (Next >= Map->size())
						return DatabaseException("query expects at least " + Core::ToString(Next + 1) + " arguments: " + Core::String(SQL.substr(Set.Start, 64)));

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
					Core::String Value = Utils::GetSQL(Remote, (*Map)[Next++], Escape, Negate);
					Buffer.erase(Set.Start, (Escape ? 1 : 2));
					Buffer.insert(Set.Start, Value);
					Offset = Set.Start + Value.size();
				}

				if (!Once)
					return Src;

				for (auto* Item : *Map)
					Core::Memory::Release(Item);
				Map->clear();

				return Src;
			}
			ExpectsDB<Core::String> Driver::GetQuery(Cluster* Base, const std::string_view& Name, Core::SchemaArgs* Map, bool Once) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Queries.find(Core::HglCast(Name));
				if (It == Queries.end())
				{
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							Core::Memory::Release(Item.second);
						Map->clear();
					}

					return DatabaseException("query not found: " + Core::String(Name));
				}

				if (!It->second.Cache.empty())
				{
					Core::String Result = It->second.Cache;
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							Core::Memory::Release(Item.second);
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
							Core::Memory::Release(Item.second);
						Map->clear();
					}

					return Result;
				}

				Connection* Remote = Base->GetAnyConnection();
				Sequence Origin = It->second;
				size_t Offset = 0;
				Unique.Negate();

				Core::String& Result = Origin.Request;
				for (auto& Word : Origin.Positions)
				{
					auto It = Map->find(Word.Key);
					if (It == Map->end())
						return DatabaseException("query expects @" + Word.Key + " constant: " + Core::String(Name));

					Core::String Value = Utils::GetSQL(Remote, It->second, Word.Escape, Word.Negate);
					if (Value.empty())
						continue;

					Result.insert(Word.Offset + Offset, Value);
					Offset += Value.size();
				}

				if (Once)
				{
					for (auto& Item : *Map)
						Core::Memory::Release(Item.second);
					Map->clear();
				}

				Core::String Data = Origin.Request;
				if (Data.empty())
					return DatabaseException("query construction error: " + Core::String(Name));

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
