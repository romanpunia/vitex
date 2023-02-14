#ifndef ED_NETWORK_POSTGRESQL_H
#define ED_NETWORK_POSTGRESQL_H
#include "../core/network.h"

struct pg_conn;
struct pg_result;
struct pgNotify;

namespace Edge
{
	namespace Network
	{
		namespace PDB
		{
			class Notify;

			class Row;

			class Request;

			class Response;

			class Cursor;

			class Cluster;

			class Driver;

			class Connection;

			typedef std::function<Core::Promise<bool>(const std::vector<std::string>&)> OnReconnect;
			typedef std::function<void(const std::string&)> OnQueryLog;
			typedef std::function<void(const Notify&)> OnNotification;
			typedef std::function<void(Cursor&)> OnResult;
			typedef Connection* SessionId;
			typedef pg_conn TConnection;
			typedef pg_result TResponse;
			typedef pgNotify TNotify;
			typedef uint64_t ObjectId;

			enum class Isolation
			{
				Serializable,
				RepeatableRead,
				ReadCommited,
				ReadUncommited,
				Default = ReadCommited
			};

			enum class QueryOp
			{
				DeleteArgs = 0,
				ReuseArgs = (1 << 0),
				TransactionAlways = (1 << 1),
				TransactionStart = (1 << 2),
				TransactionEnd = (1 << 3),
				CacheShort = (1 << 4),
				CacheMid = (1 << 5),
				CacheLong = (1 << 6)
			};

			enum class AddressOp
			{
				Host,
				Ip,
				Port,
				Database,
				User,
				Password,
				Timeout,
				Encoding,
				Options,
				Profile,
				Fallback_Profile,
				KeepAlive,
				KeepAlive_Idle,
				KeepAlive_Interval,
				KeepAlive_Count,
				TTY,
				SSL,
				SSL_Compression,
				SSL_Cert,
				SSL_Root_Cert,
				SSL_Key,
				SSL_CRL,
				Require_Peer,
				Require_SSL,
				KRB_Server_Name,
				Service
			};

			enum class QueryExec
			{
				Empty_Query = 0,
				Command_Ok,
				Tuples_OK,
				Copy_Out,
				Copy_In,
				Bad_Response,
				Non_Fatal_Error,
				Fatal_Error,
				Copy_Both,
				Single_Tuple
			};

			enum class FieldCode
			{
				Severity,
				Severity_Nonlocalized,
				SQL_State,
				Message_Primary,
				Message_Detail,
				Message_Hint,
				Statement_Position,
				Internal_Position,
				Internal_Query,
				Context,
				Schema_Name,
				Table_Name,
				Column_Name,
				Data_Type_Name,
				Constraint_Name,
				Source_File,
				Source_Line,
				Source_Function
			};

			enum class ConnectionState
			{
				Ok,
				Bad,
				Started,
				Made,
				Awaiting_Response,
				Auth_Ok,
				Set_Env,
				SSL_Startup,
				Needed,
				Check_Writable,
				Consume,
				GSS_Startup
			};

			enum class TransactionState
			{
				Idle,
				Active,
				In_Transaction,
				In_Error,
				None
			};

			enum class OidType
			{
				JSON = 114,
				JSONB = 3802,
				Any_Array = 2277,
				Name_Array = 1003,
				Text_Array = 1009,
				Date_Array = 1182,
				Time_Array = 1183,
				UUID_Array = 2951,
				CString_Array = 1263,
				BpChar_Array = 1014,
				VarChar_Array = 1015,
				Bit_Array = 1561,
				VarBit_Array = 1563,
				Char_Array = 1002,
				Int2_Array = 1005,
				Int4_Array = 1007,
				Int8_Array = 1016,
				Bool_Array = 1000,
				Float4_Array = 1021,
				Float8_Array = 1022,
				Money_Array = 791,
				Numeric_Array = 1231,
				Bytea_Array = 1001,
				Any = 2276,
				Name = 19,
				Text = 25,
				Date = 1082,
				Time = 1083,
				UUID = 2950,
				CString = 2275,
				BpChar = 1042,
				VarChar = 1043,
				Bit = 1560,
				VarBit = 1562,
				Char = 18,
				Int2 = 21,
				Int4 = 23,
				Int8 = 20,
				Bool = 16,
				Float4 = 700,
				Float8 = 701,
				Money = 790,
				Numeric = 1700,
				Bytea = 17
			};

			enum class QueryState
			{
				Lost,
				Idle,
				Busy
			};

			inline size_t operator |(QueryOp A, QueryOp B)
			{
				return static_cast<size_t>(static_cast<size_t>(A) | static_cast<size_t>(B));
			}

			class ED_OUT_TS Util
			{
			public:
				static std::string InlineArray(Cluster* Client, Core::Unique<Core::Schema> Array);
				static std::string InlineQuery(Cluster* Client, Core::Unique<Core::Schema> Where, const std::unordered_set<std::string>& Whitelist, const std::string& Default = "TRUE");
			};

			class ED_OUT Address
			{
			private:
				std::unordered_map<std::string, std::string> Params;

			public:
				Address();
				Address(const std::string& URI);
				void Override(const std::string& Key, const std::string& Value);
				bool Set(AddressOp Key, const std::string& Value);
				std::string Get(AddressOp Key) const;
				std::string GetAddress() const;
				const std::unordered_map<std::string, std::string>& Get() const;
				Core::Unique<const char*> CreateKeys() const;
				Core::Unique<const char*> CreateValues() const;

			private:
				static std::string GetKeyName(AddressOp Key);
			};

			class ED_OUT Notify
			{
			private:
				std::string Name;
				std::string Data;
				int Pid;

			public:
				Notify(TNotify* NewBase);
				Core::Unique<Core::Schema> GetSchema() const;
				std::string GetName() const;
				std::string GetData() const;
				int GetPid() const;
			};

			class ED_OUT Column
			{
				friend Row;

			private:
				TResponse* Base;
				size_t RowIndex;
				size_t ColumnIndex;

			private:
				Column(TResponse* NewBase, size_t fRowIndex, size_t fColumnIndex);

			public:
				int SetValueText(char* Data, size_t Size);
				std::string GetName() const;
				Core::Variant Get() const;
				Core::Schema* GetInline() const;
				char* GetRaw() const;
				int GetFormatId() const;
				int GetModId() const;
				int GetTableIndex() const;
				ObjectId GetTableId() const;
				ObjectId GetTypeId() const;
				size_t GetIndex() const;
				size_t GetSize() const;
				size_t GetRawSize() const;
				Row GetRow() const;
				bool IsNull() const;
				bool IsExists() const
				{
					return Base != nullptr;
				}
			};

			class ED_OUT Row
			{
				friend Column;
				friend Response;

			private:
				TResponse* Base;
				size_t RowIndex;

			private:
				Row(TResponse* NewBase, size_t fRowIndex);

			public:
				Core::Unique<Core::Schema> GetObject() const;
				Core::Unique<Core::Schema> GetArray() const;
				size_t GetIndex() const;
				size_t GetSize() const;
				Response GetCursor() const;
				Column GetColumn(size_t Index) const;
				Column GetColumn(const char* Name) const;
				bool GetColumns(Column* Output, size_t Size) const;
				bool IsExists() const
				{
					return Base != nullptr;
				}
				Column operator [](const char* Name)
				{
					return GetColumn(Name);
				}
				Column operator [](const char* Name) const
				{
					return GetColumn(Name);
				}
			};

			class ED_OUT Response
			{
			public:
				struct Iterator
				{
					const Response* Source;
					size_t Index;

					Iterator& operator ++ ()
					{
						++Index;
						return *this;
					}
					Row operator * () const
					{
						return Source->GetRow(Index);
					}
					bool operator != (const Iterator& Other) const
					{
						return Index != Other.Index;
					}
				};

			private:
				TResponse* Base;
				bool Error;

			public:
				Response(TResponse* NewBase = nullptr);
				Response(const Response& Other) = delete;
				Response(Response&& Other);
				~Response();
				Response& operator =(const Response& Other) = delete;
				Response& operator =(Response&& Other);
				Row operator [](size_t Index)
				{
					return GetRow(Index);
				}
				Row operator [](size_t Index) const
				{
					return GetRow(Index);
				}
				Core::Unique<Core::Schema> GetArrayOfObjects() const;
				Core::Unique<Core::Schema> GetArrayOfArrays() const;
				Core::Unique<Core::Schema> GetObject(size_t Index = 0) const;
				Core::Unique<Core::Schema> GetArray(size_t Index = 0) const;
				std::string GetCommandStatusText() const;
				std::string GetStatusText() const;
				std::string GetErrorText() const;
				std::string GetErrorField(FieldCode Field) const;
				int GetNameIndex(const std::string& Name) const;
				QueryExec GetStatus() const;
				ObjectId GetValueId() const;
				size_t GetAffectedRows() const;
				size_t GetSize() const;
				Row GetRow(size_t Index) const;
				Row Front() const;
				Row Back() const;
				Response Copy() const;
				TResponse* Get() const;
				bool IsEmpty() const;
				bool IsError() const;
				bool IsErrorOrEmpty() const
				{
					return IsError() || IsEmpty();
				}
				bool IsExists() const
				{
					return Base != nullptr;
				}

			public:
				Iterator begin() const
				{
					return { this, 0 };
				}
				Iterator end() const
				{
					return { this, GetSize() };
				}
			};

			class ED_OUT Cursor
			{
				friend Cluster;

			private:
				std::vector<Response> Base;
				Connection* Executor;

			public:
				Cursor();
				Cursor(Connection* NewExecutor);
				Cursor(const Cursor& Other) = delete;
				Cursor(Cursor&& Other);
				~Cursor();
				Cursor& operator =(const Cursor& Other) = delete;
				Cursor& operator =(Cursor&& Other);
				Column operator [](const char* Name)
				{
					return First().Front().GetColumn(Name);
				}
				Column operator [](const char* Name) const
				{
					return First().Front().GetColumn(Name);
				}
				bool IsSuccess() const;
				bool IsEmpty() const;
				bool IsError() const;
				bool IsErrorOrEmpty() const
				{
					return IsError() || IsEmpty();
				}
				size_t GetSize() const;
				size_t GetAffectedRows() const;
				Cursor Copy() const;
				const Response& First() const;
				const Response& Last() const;
				const Response& At(size_t Index) const;
				Connection* GetExecutor() const;
				Core::Unique<Core::Schema> GetArrayOfObjects(size_t ResponseIndex = 0) const;
				Core::Unique<Core::Schema> GetArrayOfArrays(size_t ResponseIndex = 0) const;
				Core::Unique<Core::Schema> GetObject(size_t ResponseIndex = 0, size_t Index = 0) const;
				Core::Unique<Core::Schema> GetArray(size_t ResponseIndex = 0, size_t Index = 0) const;

			public:
				std::vector<Response>::iterator begin()
				{
					return Base.begin();
				}
				std::vector<Response>::iterator end()
				{
					return Base.end();
				}
			};

			class ED_OUT Connection : public Core::Object
			{
				friend Cluster;

			private:
				std::unordered_set<std::string> Listens;
				TConnection* Base;
				Socket* Stream;
				Request* Current;
				QueryState State;
				bool InSession;

			public:
				Connection(TConnection* NewBase, socket_t Fd);
				virtual ~Connection() override;
				TConnection* GetBase() const;
				Socket* GetStream() const;
				Request* GetCurrent() const;
				QueryState GetState() const;
				bool IsInSession() const;
				bool IsBusy() const;
			};

			class ED_OUT Request : public Core::Object
			{
				friend Cluster;

			private:
				Core::Promise<Cursor> Future;
				std::vector<char> Command;
				SessionId Session;
				OnResult Callback;
				Cursor Result;
				size_t Options;

			public:
				Request(const std::string& Commands);
				virtual ~Request() = default;
				void Finalize(Cursor& Subresult);
				void Failure();
				Cursor&& GetResult();
				const std::vector<char>& GetCommand() const;
				SessionId GetSession() const;
				bool IsPending() const;
			};

			class ED_OUT_TS Cluster : public Core::Object
			{
				friend Driver;

			private:
				struct
				{
					std::unordered_map<std::string, std::pair<int64_t, Cursor>> Objects;
					std::mutex Context;
					uint64_t ShortDuration = 10;
					uint64_t MidDuration = 30;
					uint64_t LongDuration = 60;
					uint64_t CleanupDuration = 300;
					int64_t NextCleanup = 0;
				} Cache;

			private:
				std::unordered_map<std::string, std::unordered_map<uint64_t, OnNotification>> Listeners;
				std::unordered_map<Socket*, Connection*> Pool;
				std::vector<Request*> Requests;
				std::atomic<uint64_t> Channel;
				std::mutex Update;
				OnReconnect Reconnected;
				Address Source;

			public:
				Cluster();
				virtual ~Cluster() override;
				void ClearCache();
				void SetCacheCleanup(uint64_t Interval);
				void SetCacheDuration(QueryOp CacheId, uint64_t Duration);
				void SetWhenReconnected(const OnReconnect& NewCallback);
				uint64_t AddChannel(const std::string& Name, const OnNotification& NewCallback);
				bool RemoveChannel(const std::string& Name, uint64_t Id);
				Core::Promise<SessionId> TxBegin(Isolation Type);
				Core::Promise<SessionId> TxBegin(const std::string& Command);
				Core::Promise<bool> TxEnd(const std::string& Command, SessionId Session);
				Core::Promise<bool> TxCommit(SessionId Session);
				Core::Promise<bool> TxRollback(SessionId Session);
				Core::Promise<bool> Connect(const Address& URI, size_t Connections);
				Core::Promise<bool> Disconnect();
				Core::Promise<bool> Listen(const std::vector<std::string>& Channels);
				Core::Promise<bool> Unlisten(const std::vector<std::string>& Channels);
				Core::Promise<Cursor> EmplaceQuery(const std::string& Command, Core::SchemaList* Map, size_t QueryOps = 0, SessionId Session = nullptr);
				Core::Promise<Cursor> TemplateQuery(const std::string& Name, Core::SchemaArgs* Map, size_t QueryOps = 0, SessionId Session = nullptr);
				Core::Promise<Cursor> Query(const std::string& Command, size_t QueryOps = 0, SessionId Session = nullptr);
				Connection* GetConnection(QueryState State);
				Connection* GetConnection() const;
				bool IsConnected() const;

			private:
				std::string GetCacheOid(const std::string& Payload, size_t QueryOpts);
				bool GetCache(const std::string& CacheOid, Cursor* Data);
				void SetCache(const std::string& CacheOid, Cursor* Data, size_t QueryOpts);
				void TryUnassign(Connection* Base, Request* Context);
				bool Reestablish(Connection* Base);
				bool Consume(Connection* Base);
				bool Reprocess(Connection* Base);
				bool Flush(Connection* Base, bool ListenForResults);
				bool Dispatch(Connection* Base, bool Connected);
				bool TryAssign(Connection* Base, Request* Context);
				Connection* IsListens(const std::string& Name);
			};

			class ED_OUT_TS Driver
			{
			private:
				struct Pose
				{
					std::string Key;
					size_t Offset = 0;
					bool Escape = false;
					bool Negate = false;
				};

				struct Sequence
				{
					std::vector<Pose> Positions;
					std::string Request;
					std::string Cache;
				};

			private:
				static Core::Mapping<std::unordered_map<std::string, Sequence>>* Queries;
				static Core::Mapping<std::unordered_map<std::string, std::string>>* Constants;
				static std::mutex* Safe;
				static std::atomic<bool> Active;
				static std::atomic<int> State;
				static OnQueryLog Logger;

			public:
				static void Create();
				static void Release();
				static void SetQueryLog(const OnQueryLog& Callback);
				static void LogQuery(const std::string& Command);
				static bool AddConstant(const std::string& Name, const std::string& Value);
				static bool AddQuery(const std::string& Name, const char* Buffer, size_t Size);
				static bool AddDirectory(const std::string& Directory, const std::string& Origin = "");
				static bool RemoveConstant(const std::string& Name);
				static bool RemoveQuery(const std::string& Name);
				static bool LoadCacheDump(Core::Schema* Dump);
				static Core::Schema* GetCacheDump();
				static std::string Emplace(Cluster* Base, const std::string& SQL, Core::SchemaList* Map, bool Once = true);
				static std::string GetQuery(Cluster* Base, const std::string& Name, Core::SchemaArgs* Map, bool Once = true);
				static std::string GetCharArray(TConnection* Base, const std::string& Src);
				static std::string GetByteArray(TConnection* Base, const char* Src, size_t Size);
				static std::string GetSQL(TConnection* Base, Core::Schema* Source, bool Escape, bool Negate);
				static std::vector<std::string> GetQueries();
			};
		}
	}
}
#endif