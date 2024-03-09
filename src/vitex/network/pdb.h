#ifndef VI_NETWORK_POSTGRESQL_H
#define VI_NETWORK_POSTGRESQL_H
#include "../network.h"

struct pg_conn;
struct pg_result;
struct pgNotify;

namespace Vitex
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

			typedef std::function<Core::Promise<bool>(const Core::Vector<Core::String>&)> OnReconnect;
			typedef std::function<void(const std::string_view&)> OnQueryLog;
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
				CacheShort = (1 << 0),
				CacheMid = (1 << 1),
				CacheLong = (1 << 2)
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
				Command_OK,
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

			enum class TransactionState
			{
				Idle,
				Active,
				In_Transaction,
				In_Error,
				None
			};

			enum class Caching
			{
				Never,
				Miss,
				Cached
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
				IdleInTransaction,
				Busy,
				BusyInTransaction
			};

			inline size_t operator |(QueryOp A, QueryOp B)
			{
				return static_cast<size_t>(static_cast<size_t>(A) | static_cast<size_t>(B));
			}

			class DatabaseException final : public Core::BasicException
			{
			public:
				VI_OUT DatabaseException(TConnection* Connection);
				VI_OUT DatabaseException(Core::String&& Message);
				VI_OUT const char* type() const noexcept override;
			};

			template <typename V>
			using ExpectsDB = Core::Expects<V, DatabaseException>;

			template <typename T, typename Executor = Core::ParallelExecutor>
			using ExpectsPromiseDB = Core::BasicPromise<ExpectsDB<T>, Executor>;

			class VI_OUT Address
			{
			private:
				Core::UnorderedMap<Core::String, Core::String> Params;

			public:
				Address();
				void Override(const std::string_view& Key, const std::string_view& Value);
				bool Set(AddressOp Key, const std::string_view& Value);
				Core::String Get(AddressOp Key) const;
				Core::String GetAddress() const;
				const Core::UnorderedMap<Core::String, Core::String>& Get() const;
				Core::Unique<const char*> CreateKeys() const;
				Core::Unique<const char*> CreateValues() const;

			public:
				static ExpectsDB<Address> FromURL(const std::string_view& Location);

			private:
				static std::string_view GetKeyName(AddressOp Key);
			};

			class VI_OUT Notify
			{
			private:
				Core::String Name;
				Core::String Data;
				int Pid;

			public:
				Notify(TNotify* NewBase);
				Core::Unique<Core::Schema> GetSchema() const;
				Core::String GetName() const;
				Core::String GetData() const;
				int GetPid() const;
			};

			class VI_OUT Column
			{
				friend Row;

			private:
				TResponse* Base;
				size_t RowIndex;
				size_t ColumnIndex;

			private:
				Column(TResponse* NewBase, size_t fRowIndex, size_t fColumnIndex);

			public:
				Core::String GetName() const;
				Core::String GetValueText() const;
				Core::Variant Get() const;
				Core::Schema* GetInline() const;
				char* GetRaw() const;
				int GetFormatId() const;
				int GetModId() const;
				int GetTableIndex() const;
				ObjectId GetTableId() const;
				ObjectId GetTypeId() const;
				size_t Index() const;
				size_t Size() const;
				size_t RawSize() const;
				Row GetRow() const;
				bool Nullable() const;
				bool Exists() const
				{
					return Base != nullptr;
				}
			};

			class VI_OUT Row
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
				size_t Index() const;
				size_t Size() const;
				Column GetColumn(size_t Index) const;
				Column GetColumn(const std::string_view& Name) const;
				bool GetColumns(Column* Output, size_t Size) const;
				bool Exists() const
				{
					return Base != nullptr;
				}
				Column operator [](const std::string_view& Name)
				{
					return GetColumn(Name);
				}
				Column operator [](const std::string_view& Name) const
				{
					return GetColumn(Name);
				}
			};

			class VI_OUT Response
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
				bool Failure;

			public:
				Response();
				Response(TResponse* NewBase);
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
				Core::Vector<Core::String> GetColumns() const;
				Core::String GetCommandStatusText() const;
				Core::String GetStatusText() const;
				Core::String GetErrorText() const;
				Core::String GetErrorField(FieldCode Field) const;
				int GetNameIndex(const std::string_view& Name) const;
				QueryExec GetStatus() const;
				ObjectId GetValueId() const;
				size_t AffectedRows() const;
				size_t Size() const;
				Row GetRow(size_t Index) const;
				Row Front() const;
				Row Back() const;
				Response Copy() const;
				TResponse* Get() const;
				bool Empty() const;
				bool Error() const;
				bool ErrorOrEmpty() const
				{
					return Error() || Empty();
				}
				bool Exists() const
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
					return { this, Size() };
				}
			};

			class VI_OUT Cursor
			{
				friend Cluster;

			private:
				Core::Vector<Response> Base;
				Connection* Executor;
				Caching Cache;

			public:
				Cursor();
				Cursor(Connection* NewExecutor, Caching Type);
				Cursor(const Cursor& Other) = delete;
				Cursor(Cursor&& Other);
				~Cursor();
				Cursor& operator =(const Cursor& Other) = delete;
				Cursor& operator =(Cursor&& Other);
				Column operator [](const std::string_view& Name)
				{
					return First().Front().GetColumn(Name);
				}
				Column operator [](const std::string_view& Name) const
				{
					return First().Front().GetColumn(Name);
				}
				Column GetColumn(const std::string_view& Name) const
				{
					return First().Front().GetColumn(Name);
				}
				bool Success() const;
				bool Empty() const;
				bool Error() const;
				bool ErrorOrEmpty() const
				{
					return Error() || Empty();
				}
				size_t Size() const;
				size_t AffectedRows() const;
				Cursor Copy() const;
				const Response& First() const;
				const Response& Last() const;
				const Response& At(size_t Index) const;
				Connection* GetExecutor() const;
				Caching GetCacheStatus() const;
				Core::Unique<Core::Schema> GetArrayOfObjects(size_t ResponseIndex = 0) const;
				Core::Unique<Core::Schema> GetArrayOfArrays(size_t ResponseIndex = 0) const;
				Core::Unique<Core::Schema> GetObject(size_t ResponseIndex = 0, size_t Index = 0) const;
				Core::Unique<Core::Schema> GetArray(size_t ResponseIndex = 0, size_t Index = 0) const;

			public:
				Core::Vector<Response>::iterator begin()
				{
					return Base.begin();
				}
				Core::Vector<Response>::iterator end()
				{
					return Base.end();
				}
			};

			class VI_OUT Connection final : public Core::Reference<Connection>
			{
				friend Cluster;

			private:
				Core::UnorderedSet<Core::String> Listens;
				TConnection* Base;
				Socket* Stream;
				Request* Current;
				QueryState Status;

			public:
				Connection(TConnection* NewBase, socket_t Fd);
				~Connection() noexcept;
				TConnection* GetBase() const;
				Socket* GetStream() const;
				Request* GetCurrent() const;
				QueryState GetState() const;
				TransactionState GetTxState() const;
				bool InTransaction() const;
				bool Busy() const;

			private:
				void MakeBusy(Request* Data);
				Request* MakeIdle();
				Request* MakeLost();
			};

			class VI_OUT Request final : public Core::Reference<Request>
			{
				friend Cluster;

			private:
				ExpectsPromiseDB<Cursor> Future;
				Core::Vector<char> Command;
				std::chrono::microseconds Time;
				SessionId Session;
				OnResult Callback;
				Cursor Result;
				uint64_t Id;
				size_t Options;

			public:
				Request(const std::string_view& Commands, SessionId NewSession, Caching Status, uint64_t Rid, size_t NewOptions);
				void ReportCursor();
				void ReportFailure();
				Cursor&& GetResult();
				const Core::Vector<char>& GetCommand() const;
				SessionId GetSession() const;
				uint64_t GetTiming() const;
				uint64_t GetId() const;
				bool Pending() const;
			};

			class VI_OUT_TS Cluster final : public Core::Reference<Cluster>
			{
				friend Driver;

			private:
				struct
				{
					Core::UnorderedMap<Core::String, std::pair<int64_t, Cursor>> Objects;
					std::mutex Context;
					uint64_t ShortDuration = 10;
					uint64_t MidDuration = 30;
					uint64_t LongDuration = 60;
					uint64_t CleanupDuration = 300;
					int64_t NextCleanup = 0;
				} Cache;

			private:
				Core::UnorderedMap<Core::String, Core::UnorderedMap<uint64_t, OnNotification>> Listeners;
				Core::UnorderedMap<Socket*, Connection*> Pool;
				Core::Vector<Request*> Requests;
				std::atomic<uint64_t> Channel;
				std::atomic<uint64_t> Counter;
				std::recursive_mutex Update;
				OnReconnect Reconnected;
				Address Source;

			public:
				Cluster();
				~Cluster() noexcept;
				void ClearCache();
				void SetCacheCleanup(uint64_t Interval);
				void SetCacheDuration(QueryOp CacheId, uint64_t Duration);
				void SetWhenReconnected(const OnReconnect& NewCallback);
				uint64_t AddChannel(const std::string_view& Name, const OnNotification& NewCallback);
				bool RemoveChannel(const std::string_view& Name, uint64_t Id);
				ExpectsPromiseDB<SessionId> TxBegin(Isolation Type);
				ExpectsPromiseDB<SessionId> TxStart(const std::string_view& Command);
				ExpectsPromiseDB<void> TxEnd(const std::string_view& Command, SessionId Session);
				ExpectsPromiseDB<void> TxCommit(SessionId Session);
				ExpectsPromiseDB<void> TxRollback(SessionId Session);
				ExpectsPromiseDB<void> Connect(const Address& Location, size_t Connections = 1);
				ExpectsPromiseDB<void> Disconnect();
				ExpectsPromiseDB<void> Listen(const Core::Vector<Core::String>& Channels);
				ExpectsPromiseDB<void> Unlisten(const Core::Vector<Core::String>& Channels);
				ExpectsPromiseDB<Cursor> EmplaceQuery(const std::string_view& Command, Core::SchemaList* Map, size_t QueryOps = 0, SessionId Session = nullptr);
				ExpectsPromiseDB<Cursor> TemplateQuery(const std::string_view& Name, Core::SchemaArgs* Map, size_t QueryOps = 0, SessionId Session = nullptr);
				ExpectsPromiseDB<Cursor> Query(const std::string_view& Command, size_t QueryOps = 0, SessionId Session = nullptr);
				Connection* GetConnection(QueryState State);
				Connection* GetAnyConnection() const;
				bool IsConnected() const;

			private:
				Core::String GetCacheOid(const std::string_view& Payload, size_t QueryOpts);
				bool GetCache(const std::string_view& CacheOid, Cursor* Data);
				void SetCache(const std::string_view& CacheOid, Cursor* Data, size_t QueryOpts);
				bool Reestablish(Connection* Base);
				bool Consume(Connection* Base);
				bool Reprocess(Connection* Base);
				bool Flush(Connection* Base, bool ListenForResults);
				bool Dispatch(Connection* Base);
				bool IsManaging(SessionId Session);
				Connection* IsListens(const std::string_view& Name);
			};

			class VI_OUT_TS Utils
			{
			public:
				static ExpectsDB<Core::String> InlineArray(Cluster* Client, Core::UPtr<Core::Schema>&& Array);
				static ExpectsDB<Core::String> InlineQuery(Cluster* Client, Core::UPtr<Core::Schema>&& Where, const Core::UnorderedMap<Core::String, Core::String>& Whitelist, const std::string_view& Default = "TRUE");
				static Core::String GetCharArray(Connection* Base, const std::string_view& Src) noexcept;
				static Core::String GetByteArray(Connection* Base, const std::string_view& Src) noexcept;
				static Core::String GetSQL(Connection* Base, Core::Schema* Source, bool Escape, bool Negate) noexcept;
			};

			class VI_OUT_TS Driver final : public Core::Singleton<Driver>
			{
			private:
				struct Pose
				{
					Core::String Key;
					size_t Offset = 0;
					bool Escape = false;
					bool Negate = false;
				};

				struct Sequence
				{
					Core::Vector<Pose> Positions;
					Core::String Request;
					Core::String Cache;
				};

			private:
				Core::UnorderedMap<Core::String, Sequence> Queries;
				Core::UnorderedMap<Core::String, Core::String> Constants;
				std::mutex Exclusive;
				std::atomic<bool> Active;
				OnQueryLog Logger;

			public:
				Driver() noexcept;
				virtual ~Driver() noexcept override;
				void SetQueryLog(const OnQueryLog& Callback) noexcept;
				void LogQuery(const std::string_view& Command) noexcept;
				void AddConstant(const std::string_view& Name, const std::string_view& Value) noexcept;
				ExpectsDB<void> AddQuery(const std::string_view& Name, const std::string_view& Data);
				ExpectsDB<void> AddDirectory(const std::string_view& Directory, const std::string_view& Origin = "");
				bool RemoveConstant(const std::string_view& Name) noexcept;
				bool RemoveQuery(const std::string_view& Name) noexcept;
				bool LoadCacheDump(Core::Schema* Dump) noexcept;
				Core::Schema* GetCacheDump() noexcept;
				ExpectsDB<Core::String> Emplace(Cluster* Base, const std::string_view& SQL, Core::SchemaList* Map) noexcept;
				ExpectsDB<Core::String> GetQuery(Cluster* Base, const std::string_view& Name, Core::SchemaArgs* Map) noexcept;
				Core::Vector<Core::String> GetQueries() noexcept;
			};
		}
	}
}
#endif