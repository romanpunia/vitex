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
			std::string Column::GetValueText() const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return std::string();

				char* Text = PQgetvalue(Base, RowIndex, ColumnIndex);
				if (!Text)
					return std::string();
				
				int Size = PQgetlength(Base, RowIndex, ColumnIndex);
				if (Size < 0)
					return Text;

				return std::string(Text, (size_t)Size);
#else
				return std::string();
#endif
			}
			bool Column::GetBool() const
			{
				char* Value = GetValueData();
				if (!Value)
					return false;

				return Value[0] == 't' || Value[0] == 'T' || Value[0] == 'y' || Value[0] == 'Y' || Value[0] == '1';
			}
			short Column::GetShort() const
			{
				Core::Parser Src(GetValueData(), GetValueSize());
				if (!Src.HasInteger())
					return 0;

				return (short)Src.ToInt();
			}
			unsigned short Column::GetUShort() const
			{
				Core::Parser Src(GetValueData(), GetValueSize());
				if (!Src.HasInteger())
					return 0;

				return (unsigned short)Src.ToUInt();
			}
			long Column::GetLong() const
			{
				Core::Parser Src(GetValueData(), GetValueSize());
				if (!Src.HasInteger())
					return 0;

				return Src.ToLong();
			}
			unsigned long Column::GetULong() const
			{
				Core::Parser Src(GetValueData(), GetValueSize());
				if (!Src.HasInteger())
					return 0;

				return Src.ToULong();
			}
			int Column::GetInt32() const
			{
				Core::Parser Src(GetValueData(), GetValueSize());
				if (!Src.HasInteger())
					return 0;

				return Src.ToInt();
			}
			unsigned Column::GetUInt32() const
			{
				Core::Parser Src(GetValueData(), GetValueSize());
				if (!Src.HasInteger())
					return 0;

				return Src.ToUInt();
			}
			int64_t Column::GetInt64() const
			{
				Core::Parser Src(GetValueData(), GetValueSize());
				if (!Src.HasInteger())
					return 0;

				return Src.ToInt64();
			}
			uint64_t Column::GetUInt64() const
			{
				Core::Parser Src(GetValueData(), GetValueSize());
				if (!Src.HasInteger())
					return 0;

				return Src.ToUInt64();
			}
			Core::Decimal Column::GetDecimal() const
			{
				return Core::Decimal(GetValueText());
			}
			float Column::GetFloat() const
			{
				Core::Parser Src(GetValueData(), GetValueSize());
				if (!Src.HasNumber())
					return 0;

				return Src.ToFloat();
			}
			double Column::GetDouble() const
			{
				Core::Parser Src(GetValueData(), GetValueSize());
				if (!Src.HasNumber())
					return 0;

				return Src.ToDouble();
			}
			char Column::GetChar() const
			{
				char* Value = GetValueData();
				if (!Value)
					return '\0';

				return *Value;
			}
			unsigned char* Column::GetBlob(size_t* OutSize) const
			{
#ifdef TH_HAS_POSTGRESQL
				if (!OutSize)
					return nullptr;

				unsigned char* Result = PQunescapeBytea(reinterpret_cast<unsigned char*>(GetValueData()), OutSize);
				unsigned char* Copy = (unsigned char*)TH_MALLOC(*OutSize * sizeof(unsigned char));
				memcpy(Copy, Result, *OutSize * sizeof(unsigned char));
				PQfreemem(Result);

				return Copy;
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
			TResult* Result::Get() const
			{
				return Base;
			}

			Results::Results() : Source(nullptr)
			{
			}
			Results::Results(Connection* NewSource) : Source(NewSource)
			{
			}
			void Results::Release()
			{
				if (!Source || Data.empty())
					return;

				Source = nullptr;
				for (auto& Item : Data)
					Item.Release();
				Data.clear();
			}
			void Results::Push(TResult* NewResult)
			{
				Data.emplace_back(NewResult);
			}
			void Results::Swap(Results&& Other)
			{
				Source = Other.Source;
				Data = std::move(Other.Data);
			}
			size_t Results::GetSize() const
			{
				return Data.size();
			}
			Result& Results::GetResult(size_t Index)
			{
				return Data[Index];
			}
			const Result& Results::GetResult(size_t Index) const
			{
				return Data[Index];
			}
			Connection* Results::GetSource() const
			{
				return Source;
			}

			Connection::Connection() : Operation(this), Base(nullptr), Master(nullptr), Connected(false)
			{
				Driver::Create();
			}
			Connection::~Connection()
			{
				Driver::Release();
			}
			void Connection::SetNotificationCallback(const OnNotification& NewCallback)
			{
				Safe.lock();
				Callback = NewCallback;
				Safe.unlock();
			}
			Core::Async<bool> Connection::Connect(const Address& URI)
			{
#ifdef TH_HAS_POSTGRESQL
				if (Master != nullptr)
					return Core::Async<bool>::Store(false);

				if (Connected)
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
					Connected = true;
					Future.Set(true);
				});
#else
				return Core::Async<bool>::Store(false);
#endif
			}
			Core::Async<bool> Connection::Disconnect()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Connected || !Base)
					return Core::Async<bool>::Store(false);

				return Core::Async<bool>([this](Core::Async<bool>& Future)
				{
					Connected = false;
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
			Core::Async<Results> Connection::QuerySet(const std::string& Command)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || Command.empty() || Acquired)
					return Core::Async<Results>::Store(Results());

				Safe.lock();
				Acquired = true;
				int Code = PQsendQuery(Base, Command.c_str());
				if (Code != 1)
				{
					char* Message = PQerrorMessage(Base);
					Acquired = false;
					Safe.unlock();

					if (Message != nullptr)
						TH_ERROR("[pqerr] %s", Message);

					return Core::Async<Results>::Store(Results());
				}

				Driver::Listen(this);
				Safe.unlock();

				return Income;
#else
				return Core::Async<Results>::Store(Results());
#endif
			}
			Core::Async<Result> Connection::Query(const std::string& Command)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || Command.empty() || Acquired)
					return Core::Async<Result>::Store(Result(nullptr));
	
				return Core::Async<Result>([this, Command](Core::Async<Result>& Future)
				{
					Safe.lock();
					Acquired = true;
					TResult* Data = PQexec(Base, Command.c_str());
					Acquired = false;
					Safe.unlock();

					if (!Data)
					{
						char* Message = PQerrorMessage(Base);
						if (Message != nullptr)
							TH_ERROR("[pqerr] %s", Message);
					}

					PQflush(Base);
					Future.Set(Result(Data));
				});
#else
				return Core::Async<Result>::Store(Result(nullptr));
#endif
			}
			Core::Async<Result> Connection::Subscribe(const std::string& Channel)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || Channel.empty())
					return Core::Async<Result>::Store(Result(nullptr));

				return Query("LISTEN " + EscapeIdentifier(Channel));
#else
				return Core::Async<Result>::Store(Result(nullptr));
#endif
			}
			Core::Async<Result> Connection::Unsubscribe(const std::string& Channel)
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base || Channel.empty())
					return Core::Async<Result>::Store(Result(nullptr));

				return Query("UNLISTEN " + EscapeIdentifier(Channel));
#else
				return Core::Async<Result>::Store(Result(nullptr));
#endif
			}
			bool Connection::CancelQuery()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return false;

				PGcancel* Cancel = PQgetCancel(Base);
				if (!Cancel)
					return false;

				char Error[256];
				int Code = PQcancel(Cancel, Error, sizeof(Error));

				if (Code != 1)
					TH_ERROR("[pqerr] %s", Error);

				PQfreeCancel(Cancel);
				return Code == 1;
#else
				return false;
#endif
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
				if (!Data || !OutSize)
					return nullptr;

#ifdef TH_HAS_POSTGRESQL
				if (!Base)
					return nullptr;

				unsigned char* Result = PQunescapeBytea(Data, OutSize);
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
				return Connected;
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
				Client->Connected = false;
			}
			bool Queue::Push(Connection** Client)
			{
				if (!Client || !*Client)
					return false;

				Safe.lock();
				Clear(*Client);
				TH_RELEASE(*Client);
				Safe.unlock();

				return true;
			}
			Core::Async<Connection*> Queue::Pop()
			{
#ifdef TH_HAS_POSTGRESQL
				if (!Connected)
					return Core::Async<Connection*>::Store(nullptr);

				Safe.lock();
				if (!Inactive.empty())
				{
					Connection* Result = *Inactive.begin();
					Inactive.erase(Inactive.begin());
					Active.insert(Result);
					Safe.unlock();

					return Core::Async<Connection*>::Store(Result);
				}

				Connection* Result = new Connection();
				Active.insert(Result);
				Safe.unlock();

				return Result->Connect(Source).Then<Core::Async<Connection*>>([this, Result](bool&& Subresult)
				{
					if (Subresult)
					{
						Result->Master = this;
						return Core::Async<Connection*>::Store(Result);
					}

					this->Safe.lock();
					this->Active.erase(Result);
					this->Safe.unlock();

					TH_RELEASE(Result);
					return Core::Async<Connection*>::Store(nullptr);
				});
#else
				return Core::Async<Connection*>::Store(nullptr);
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

				Dispatch();
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
					Connection* Base = *It;
					int Consume = PQconsumeInput(Base->Base);
					if (Consume != 1)
						continue;

					int Busy = PQisBusy(Base->Base);
					if (Busy == 1)
						continue;

					PGnotify* Notification = PQnotifies(Base->Base);
					if (Notification != nullptr)
					{
						Safe->lock();
						OnNotification Callback = Base->Callback;
						Safe->unlock();

						if (Callback)
							Callback(Notify(Notification));
						else
							PQfreeNotify(Notification);
					}

					if (!Base->Acquired)
						continue;

					TResult* NewResult = PQgetResult(Base->Base);
					if (!NewResult)
					{
						Base->Safe.lock();	
						Core::Async<Results> Result = std::move(Base->Income);
						Base->Income.Steal(Core::Async<Results>());

						Results Data;
						Data.Swap(std::move(Base->Operation));
						Base->Acquired = false;
						Base->Safe.unlock();

						PQflush(Base->Base);
						Result.Set(Data);
					}
					else
					{
						Base->Safe.lock();
						Base->Operation.Push(NewResult);
						Base->Safe.unlock();
					}

					Count++;
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
					return nullptr;

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

					return nullptr;
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