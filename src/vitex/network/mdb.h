#ifndef VI_NETWORK_MONGODB_H
#define VI_NETWORK_MONGODB_H
#include "../network.h"

struct _bson_t;
struct _mongoc_client_t;
struct _mongoc_uri_t;
struct _mongoc_database_t;
struct _mongoc_collection_t;
struct _mongoc_cursor_t;
struct _mongoc_bulk_operation_t;
struct _mongoc_client_pool_t;
struct _mongoc_change_stream_t;
struct _mongoc_client_session_t;

namespace Vitex
{
	namespace Network
	{
		namespace MDB
		{
			typedef _bson_t TDocument;
			typedef _mongoc_client_pool_t TConnectionPool;
			typedef _mongoc_bulk_operation_t TStream;
			typedef _mongoc_cursor_t TCursor;
			typedef _mongoc_collection_t TCollection;
			typedef _mongoc_database_t TDatabase;
			typedef _mongoc_uri_t TAddress;
			typedef _mongoc_client_t TConnection;
			typedef _mongoc_change_stream_t TWatcher;
			typedef _mongoc_client_session_t TTransaction;
			typedef std::function<void(const std::string_view&)> OnQueryLog;

			class Transaction;

			class Connection;

			class Cluster;

			class Document;

			class Collection;

			enum class QueryFlags
			{
				None = 0,
				Tailable_Cursor = 1 << 1,
				Slave_Ok = 1 << 2,
				Oplog_Replay = 1 << 3,
				No_Cursor_Timeout = 1 << 4,
				Await_Data = 1 << 5,
				Exhaust = 1 << 6,
				Partial = 1 << 7,
			};

			enum class Type
			{
				Unknown,
				Uncastable,
				Null,
				Document,
				Array,
				String,
				Integer,
				Number,
				Boolean,
				ObjectId,
				Decimal
			};

			enum class TransactionState
			{
				OK,
				Retry_Commit,
				Retry,
				Fatal
			};

			inline QueryFlags operator |(QueryFlags A, QueryFlags B)
			{
				return static_cast<QueryFlags>(static_cast<size_t>(A) | static_cast<size_t>(B));
			}

			struct VI_OUT Property
			{
				Core::String Name;
				Core::String String;
				uint8_t ObjectId[12] = { };
				TDocument* Source;
				int64_t Integer;
				uint64_t High;
				uint64_t Low;
				double Number;
				bool Boolean;
				Type Mod;
				bool IsValid;

				Property() noexcept;
				Property(const Property& Other) = delete;
				Property(Property&& Other);
				~Property();
				Property& operator =(const Property& Other);
				Property& operator =(Property&& Other);
				Core::Unique<TDocument> Reset();
				Core::String& ToString();
				Core::String ToObjectId();
				Document AsDocument() const;
				Property At(const std::string_view& Name) const;
				Property operator [](const std::string_view& Name);
				Property operator [](const std::string_view& Name) const;
			};

			class DatabaseException final : public Core::BasicException
			{
			private:
				int ErrorCode;

			public:
				VI_OUT DatabaseException(int ErrorCode, Core::String&& Message);
				VI_OUT const char* type() const noexcept override;
				VI_OUT int error_code() const noexcept;
			};

			template <typename V>
			using ExpectsDB = Core::Expects<V, DatabaseException>;

			template <typename T, typename Executor = Core::ParallelExecutor>
			using ExpectsPromiseDB = Core::BasicPromise<ExpectsDB<T>, Executor>;

			class VI_OUT Document
			{
			private:
				TDocument* Base;
				bool Store;

			public:
				Document();
				Document(TDocument* NewBase);
				Document(const Document& Other);
				Document(Document&& Other);
				~Document();
				Document& operator =(const Document& Other);
				Document& operator =(Document&& Other);
				void Cleanup();
				void Join(const Document& Value);
				void Loop(const std::function<bool(Property*)>& Callback) const;
				bool SetSchema(const std::string_view& Key, const Document& Value, size_t ArrayId = 0);
				bool SetArray(const std::string_view& Key, const Document& Array, size_t ArrayId = 0);
				bool SetString(const std::string_view& Key, const std::string_view& Value, size_t ArrayId = 0);
				bool SetInteger(const std::string_view& Key, int64_t Value, size_t ArrayId = 0);
				bool SetNumber(const std::string_view& Key, double Value, size_t ArrayId = 0);
				bool SetDecimal(const std::string_view& Key, uint64_t High, uint64_t Low, size_t ArrayId = 0);
				bool SetDecimalString(const std::string_view& Key, const std::string_view& Value, size_t ArrayId = 0);
				bool SetDecimalInteger(const std::string_view& Key, int64_t Value, size_t ArrayId = 0);
				bool SetDecimalNumber(const std::string_view& Key, double Value, size_t ArrayId = 0);
				bool SetBoolean(const std::string_view& Key, bool Value, size_t ArrayId = 0);
				bool SetObjectId(const std::string_view& Key, uint8_t Value[12], size_t ArrayId = 0);
				bool SetNull(const std::string_view& Key, size_t ArrayId = 0);
				bool SetProperty(const std::string_view& Key, Property* Value, size_t ArrayId = 0);
				bool HasProperty(const std::string_view& Key) const;
				bool GetProperty(const std::string_view& Key, Property* Output) const;
				size_t Count() const;
				Core::String ToRelaxedJSON() const;
				Core::String ToExtendedJSON() const;
				Core::String ToJSON() const;
				Core::String ToIndices() const;
				Core::Unique<Core::Schema> ToSchema(bool IsArray = false) const;
				TDocument* Get() const;
				Document Copy() const;
				Document& Persist(bool Keep = true);
				Property At(const std::string_view& Name) const;
				explicit operator bool() const
				{
					return Base != nullptr;
				}
				Property operator [](const std::string_view& Name)
				{
					Property Result;
					GetProperty(Name, &Result);
					return Result;
				}
				Property operator [](const std::string_view& Name) const
				{
					Property Result;
					GetProperty(Name, &Result);
					return Result;
				}

			public:
				static Document FromEmpty();
				static Document FromSchema(Core::Schema* Document);
				static ExpectsDB<Document> FromJSON(const std::string_view& JSON);
				static Document FromBuffer(const uint8_t* Buffer, size_t Length);
				static Document FromSource(TDocument* Src);

			private:
				static bool Clone(void* It, Property* Output);
			};

			class VI_OUT Address
			{
			private:
				TAddress* Base;

			public:
				Address();
				Address(TAddress* NewBase);
				Address(const Address& Other);
				Address(Address&& Other);
				~Address();
				Address& operator =(const Address& Other);
				Address& operator =(Address&& Other);
				void SetOption(const std::string_view& Name, int64_t Value);
				void SetOption(const std::string_view& Name, bool Value);
				void SetOption(const std::string_view& Name, const std::string_view& Value);
				void SetAuthMechanism(const std::string_view& Value);
				void SetAuthSource(const std::string_view& Value);
				void SetCompressors(const std::string_view& Value);
				void SetDatabase(const std::string_view& Value);
				void SetUsername(const std::string_view& Value);
				void SetPassword(const std::string_view& Value);
				TAddress* Get() const;
				explicit operator bool() const
				{
					return Base != nullptr;
				}

			public:
				static ExpectsDB<Address> FromURL(const std::string_view& Location);
			};

			class VI_OUT Stream
			{
			private:
				Document NetOptions;
				TCollection* Source;
				TStream* Base;
				size_t Count;

			public:
				Stream();
				Stream(TCollection* NewSource, TStream* NewBase, Document&& NewOptions);
				Stream(const Stream& Other);
				Stream(Stream&& Other);
				~Stream();
				Stream& operator =(const Stream& Other);
				Stream& operator =(Stream&& Other);
				ExpectsDB<void> RemoveMany(const Document& Match, const Document& Options);
				ExpectsDB<void> RemoveOne(const Document& Match, const Document& Options);
				ExpectsDB<void> ReplaceOne(const Document& Match, const Document& Replacement, const Document& Options);
				ExpectsDB<void> InsertOne(const Document& Result, const Document& Options);
				ExpectsDB<void> UpdateOne(const Document& Match, const Document& Result, const Document& Options);
				ExpectsDB<void> UpdateMany(const Document& Match, const Document& Result, const Document& Options);
				ExpectsDB<void> TemplateQuery(const std::string_view& Name, Core::SchemaArgs* Map);
				ExpectsDB<void> Query(const Document& Command);
				ExpectsPromiseDB<Document> ExecuteWithReply();
				ExpectsPromiseDB<void> Execute();
				size_t GetHint() const;
				TStream* Get() const;
				explicit operator bool() const
				{
					return Base != nullptr;
				}

			private:
				ExpectsDB<void> NextOperation();
			};

			class VI_OUT Cursor
			{
			private:
				TCursor* Base;

			public:
				Cursor();
				Cursor(TCursor* NewBase);
				Cursor(const Cursor& Other) = delete;
				Cursor(Cursor&& Other);
				~Cursor();
				Cursor& operator =(const Cursor& Other) = delete;
				Cursor& operator =(Cursor&& Other);
				void SetMaxAwaitTime(size_t MaxAwaitTime);
				void SetBatchSize(size_t BatchSize);
				bool SetLimit(int64_t Limit);
				bool SetHint(size_t Hint);
				bool Empty() const;
				Core::Option<DatabaseException> Error() const;
				ExpectsPromiseDB<void> Next();
				int64_t GetId() const;
				int64_t GetLimit() const;
				size_t GetMaxAwaitTime() const;
				size_t GetBatchSize() const;
				size_t GetHint() const;
				Document Current() const;
				Cursor Clone();
				TCursor* Get() const;
				bool ErrorOrEmpty() const
				{
					return Error() || Empty();
				}
				explicit operator bool() const
				{
					return Base != nullptr;
				}

			private:
				void Cleanup();
			};

			class VI_OUT Response
			{
			private:
				Cursor NetCursor;
				Document NetDocument;
				bool NetSuccess;

			public:
				Response();
				Response(bool NewSuccess);
				Response(Cursor& NewCursor);
				Response(Document& NewDocument);
				Response(const Response& Other) = delete;
				Response(Response&& Other);
				Response& operator =(const Response& Other) = delete;
				Response& operator =(Response&& Other);
				ExpectsPromiseDB<Core::Unique<Core::Schema>> Fetch();
				ExpectsPromiseDB<Core::Unique<Core::Schema>> FetchAll();
				ExpectsPromiseDB<Property> GetProperty(const std::string_view& Name);
				Cursor&& GetCursor();
				Document&& GetDocument();
				bool Success() const;
				ExpectsPromiseDB<Property> operator [](const std::string_view& Name)
				{
					return GetProperty(Name);
				}
				explicit operator bool() const
				{
					return Success();
				}
			};

			class VI_OUT Transaction
			{
			private:
				TTransaction* Base;

			public:
				Transaction();
				Transaction(TTransaction* NewBase);
				Transaction(const Transaction& Other) = default;
				Transaction(Transaction&& Other) = default;
				~Transaction() = default;
				Transaction& operator =(const Transaction& Other) = default;
				Transaction& operator =(Transaction&& Other) = default;
				ExpectsDB<void> Push(const Document& QueryOptions);
				ExpectsDB<void> Put(TDocument** QueryOptions) const;
				ExpectsPromiseDB<void> Begin();
				ExpectsPromiseDB<void> Rollback();
				ExpectsPromiseDB<Document> RemoveMany(Collection& Base, const Document& Match, const Document& Options);
				ExpectsPromiseDB<Document> RemoveOne(Collection& Base, const Document& Match, const Document& Options);
				ExpectsPromiseDB<Document> ReplaceOne(Collection& Base, const Document& Match, const Document& Replacement, const Document& Options);
				ExpectsPromiseDB<Document> InsertMany(Collection& Base, Core::Vector<Document>& List, const Document& Options);
				ExpectsPromiseDB<Document> InsertOne(Collection& Base, const Document& Result, const Document& Options);
				ExpectsPromiseDB<Document> UpdateMany(Collection& Base, const Document& Match, const Document& Update, const Document& Options);
				ExpectsPromiseDB<Document> UpdateOne(Collection& Base, const Document& Match, const Document& Update, const Document& Options);
				ExpectsPromiseDB<Cursor> FindMany(Collection& Base, const Document& Match, const Document& Options);
				ExpectsPromiseDB<Cursor> FindOne(Collection& Base, const Document& Match, const Document& Options);
				ExpectsPromiseDB<Cursor> Aggregate(Collection& Base, QueryFlags Flags, const Document& Pipeline, const Document& Options);
				ExpectsPromiseDB<Response> TemplateQuery(Collection& Base, const std::string_view& Name, Core::SchemaArgs* Map);
				ExpectsPromiseDB<Response> Query(Collection& Base, const Document& Command);
				Core::Promise<TransactionState> Commit();
				TTransaction* Get() const;
				explicit operator bool() const
				{
					return Base != nullptr;
				}
			};

			class VI_OUT Collection
			{
			private:
				TCollection* Base;

			public:
				Collection();
				Collection(TCollection* NewBase);
				Collection(const Collection& Other) = delete;
				Collection(Collection&& Other);
				~Collection();
				Collection& operator =(const Collection& Other) = delete;
				Collection& operator =(Collection&& Other);
				ExpectsPromiseDB<void> Rename(const std::string_view& NewDatabaseName, const std::string_view& NewCollectionName);
				ExpectsPromiseDB<void> RenameWithOptions(const std::string_view& NewDatabaseName, const std::string_view& NewCollectionName, const Document& Options);
				ExpectsPromiseDB<void> RenameWithRemove(const std::string_view& NewDatabaseName, const std::string_view& NewCollectionName);
				ExpectsPromiseDB<void> RenameWithOptionsAndRemove(const std::string_view& NewDatabaseName, const std::string_view& NewCollectionName, const Document& Options);
				ExpectsPromiseDB<void> Remove(const Document& Options);
				ExpectsPromiseDB<void> RemoveIndex(const std::string_view& Name, const Document& Options);
				ExpectsPromiseDB<Document> RemoveMany(const Document& Match, const Document& Options);
				ExpectsPromiseDB<Document> RemoveOne(const Document& Match, const Document& Options);
				ExpectsPromiseDB<Document> ReplaceOne(const Document& Match, const Document& Replacement, const Document& Options);
				ExpectsPromiseDB<Document> InsertMany(Core::Vector<Document>& List, const Document& Options);
				ExpectsPromiseDB<Document> InsertOne(const Document& Result, const Document& Options);
				ExpectsPromiseDB<Document> UpdateMany(const Document& Match, const Document& Update, const Document& Options);
				ExpectsPromiseDB<Document> UpdateOne(const Document& Match, const Document& Update, const Document& Options);
				ExpectsPromiseDB<Document> FindAndModify(const Document& Match, const Document& Sort, const Document& Update, const Document& Fields, bool Remove, bool Upsert, bool New);
				Core::Promise<size_t> CountDocuments(const Document& Match, const Document& Options);
				Core::Promise<size_t> CountDocumentsEstimated(const Document& Options);
				ExpectsPromiseDB<Cursor> FindIndexes(const Document& Options);
				ExpectsPromiseDB<Cursor> FindMany(const Document& Match, const Document& Options);
				ExpectsPromiseDB<Cursor> FindOne(const Document& Match, const Document& Options);
				ExpectsPromiseDB<Cursor> Aggregate(QueryFlags Flags, const Document& Pipeline, const Document& Options);
				ExpectsPromiseDB<Response> TemplateQuery(const std::string_view& Name, Core::SchemaArgs* Map, const Transaction& Session = Transaction());
				ExpectsPromiseDB<Response> Query(const Document& Command, const Transaction& Session = Transaction());
				Core::String GetName() const;
				ExpectsDB<Stream> CreateStream(Document& Options);
				TCollection* Get() const;
				explicit operator bool() const
				{
					return Base != nullptr;
				}
			};

			class VI_OUT Database
			{
			private:
				TDatabase* Base;

			public:
				Database();
				Database(TDatabase* NewBase);
				Database(const Database& Other) = delete;
				Database(Database&& Other);
				~Database();
				Database& operator =(const Database& Other) = delete;
				Database& operator =(Database&& Other);
				ExpectsPromiseDB<void> RemoveAllUsers();
				ExpectsPromiseDB<void> RemoveUser(const std::string_view& Name);
				ExpectsPromiseDB<void> Remove();
				ExpectsPromiseDB<void> RemoveWithOptions(const Document& Options);
				ExpectsPromiseDB<void> AddUser(const std::string_view& Username, const std::string_view& Password, const Document& Roles, const Document& Custom);
				ExpectsPromiseDB<void> HasCollection(const std::string_view& Name);
				ExpectsPromiseDB<Collection> CreateCollection(const std::string_view& Name, const Document& Options);
				ExpectsPromiseDB<Cursor> FindCollections(const Document& Options);
				ExpectsDB<Core::Vector<Core::String>> GetCollectionNames(const Document& Options) const;
				Core::String GetName() const;
				Collection GetCollection(const std::string_view& Name);
				TDatabase* Get() const;
				explicit operator bool() const
				{
					return Base != nullptr;
				}
			};

			class VI_OUT Watcher
			{
			private:
				TWatcher* Base;

			public:
				Watcher();
				Watcher(TWatcher* NewBase);
				Watcher(const Watcher& Other);
				Watcher(Watcher&& Other);
				~Watcher();
				Watcher& operator =(const Watcher& Other);
				Watcher& operator =(Watcher&& Other);
				ExpectsPromiseDB<void> Next(Document& Result);
				ExpectsPromiseDB<void> Error(Document& Result);
				TWatcher* Get() const;
				explicit operator bool() const
				{
					return Base != nullptr;
				}

			public:
				static ExpectsDB<Watcher> FromConnection(Connection* Connection, const Document& Pipeline, const Document& Options);
				static ExpectsDB<Watcher> FromDatabase(const Database& Src, const Document& Pipeline, const Document& Options);
				static ExpectsDB<Watcher> FromCollection(const Collection& Src, const Document& Pipeline, const Document& Options);
			};

			class VI_OUT Connection final : public Core::Reference<Connection>
			{
				friend Cluster;
				friend Transaction;

			private:
				std::atomic<bool> Connected;
				Transaction Session;
				TConnection* Base;
				Cluster* Master;

			public:
				Connection();
				~Connection() noexcept;
				ExpectsPromiseDB<void> Connect(Address* Location);
				ExpectsPromiseDB<void> Disconnect();
				ExpectsPromiseDB<void> MakeTransaction(std::function<Core::Promise<bool>(Transaction*)>&& Callback);
				ExpectsPromiseDB<Cursor> FindDatabases(const Document& Options);
				void SetProfile(const std::string_view& Name);
				ExpectsDB<void> SetServer(bool Writeable);
				ExpectsDB<Transaction*> GetSession();
				Database GetDatabase(const std::string_view& Name) const;
				Database GetDefaultDatabase() const;
				Collection GetCollection(const std::string_view& DatabaseName, const std::string_view& Name) const;
				Address GetAddress() const;
				Cluster* GetMaster() const;
				TConnection* Get() const;
				ExpectsDB<Core::Vector<Core::String>> GetDatabaseNames(const Document& Options) const;
				bool IsConnected() const;
			};

			class VI_OUT_TS Cluster final : public Core::Reference<Cluster>
			{
			private:
				std::atomic<bool> Connected;
				TConnectionPool* Pool;
				Address SrcAddress;

			public:
				Cluster();
				~Cluster() noexcept;
				ExpectsPromiseDB<void> Connect(Address* URI);
				ExpectsPromiseDB<void> Disconnect();
				void SetProfile(const std::string_view& Name);
				void Push(Connection** Client);
				Connection* Pop();
				TConnectionPool* Get() const;
				Address& GetAddress();
			};

			class VI_OUT_TS Utils
			{
			public:
				static bool GetId(uint8_t* Id12) noexcept;
				static bool GetDecimal(const std::string_view& Value, int64_t* High, int64_t* Low) noexcept;
				static uint32_t GetHashId(uint8_t* Id12) noexcept;
				static int64_t GetTimeId(uint8_t* Id12) noexcept;
				static Core::String IdToString(uint8_t* Id12) noexcept;
				static Core::String StringToId(const std::string_view& Id24) noexcept;
				static Core::String GetJSON(Core::Schema* Source, bool Escape) noexcept;
			};

			class VI_OUT_TS Driver final : public Core::Singleton<Driver>
			{
			private:
				struct Pose
				{
					Core::String Key;
					size_t Offset = 0;
					bool Escape = false;
				};

				struct Sequence
				{
					Core::Vector<Pose> Positions;
					Core::String Request;
					Document Cache;

					Sequence();
					Sequence(const Sequence& Other);
					Sequence& operator =(Sequence&& Other) = default;
				};

			private:
				Core::UnorderedMap<Core::String, Sequence> Queries;
				Core::UnorderedMap<Core::String, Core::String> Constants;
				std::mutex Exclusive;
				OnQueryLog Logger;
				void* APM;

			public:
				Driver() noexcept;
				virtual ~Driver() noexcept override;
				void SetQueryLog(const OnQueryLog& Callback) noexcept;
				void AttachQueryLog(TConnection* Connection) noexcept;
				void AttachQueryLog(TConnectionPool* Connection) noexcept;
				void AddConstant(const std::string_view& Name, const std::string_view& Value) noexcept;
				ExpectsDB<void> AddQuery(const std::string_view& Name, const std::string_view& Data);
				ExpectsDB<void> AddDirectory(const std::string_view& Directory, const std::string_view& Origin = "");
				bool RemoveConstant(const std::string_view& Name) noexcept;
				bool RemoveQuery(const std::string_view& Name) noexcept;
				bool LoadCacheDump(Core::Schema* Dump) noexcept;
				Core::Schema* GetCacheDump() noexcept;
				ExpectsDB<Document> GetQuery(const std::string_view& Name, Core::SchemaArgs* Map) noexcept;
				Core::Vector<Core::String> GetQueries() noexcept;
			};
		}
	}
}
#endif
