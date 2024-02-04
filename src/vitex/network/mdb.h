#ifndef VI_NETWORK_MONGODB_H
#define VI_NETWORK_MONGODB_H
#include "../core/network.h"

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
			typedef std::function<void(const Core::String&)> OnQueryLog;

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
				unsigned char ObjectId[12] = { };
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
				Property At(const Core::String& Name) const;
				Property operator [](const char* Name);
				Property operator [](const char* Name) const;
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
				bool SetSchema(const char* Key, const Document& Value, size_t ArrayId = 0);
				bool SetArray(const char* Key, const Document& Array, size_t ArrayId = 0);
				bool SetString(const char* Key, const char* Value, size_t ArrayId = 0);
				bool SetBlob(const char* Key, const char* Value, size_t Length, size_t ArrayId = 0);
				bool SetInteger(const char* Key, int64_t Value, size_t ArrayId = 0);
				bool SetNumber(const char* Key, double Value, size_t ArrayId = 0);
				bool SetDecimal(const char* Key, uint64_t High, uint64_t Low, size_t ArrayId = 0);
				bool SetDecimalString(const char* Key, const Core::String& Value, size_t ArrayId = 0);
				bool SetDecimalInteger(const char* Key, int64_t Value, size_t ArrayId = 0);
				bool SetDecimalNumber(const char* Key, double Value, size_t ArrayId = 0);
				bool SetBoolean(const char* Key, bool Value, size_t ArrayId = 0);
				bool SetObjectId(const char* Key, unsigned char Value[12], size_t ArrayId = 0);
				bool SetNull(const char* Key, size_t ArrayId = 0);
				bool SetProperty(const char* Key, Property* Value, size_t ArrayId = 0);
				bool HasProperty(const char* Key) const;
				bool GetProperty(const char* Key, Property* Output) const;
				bool SetSchemaAt(const Core::String& Key, const Document& Value, size_t ArrayId = 0);
				bool SetArrayAt(const Core::String& Key, const Document& Array, size_t ArrayId = 0);
				bool SetStringAt(const Core::String& Key, const Core::String& Value, size_t ArrayId = 0);
				bool SetIntegerAt(const Core::String& Key, int64_t Value, size_t ArrayId = 0);
				bool SetNumberAt(const Core::String& Key, double Value, size_t ArrayId = 0);
				bool SetDecimalAt(const Core::String& Key, uint64_t High, uint64_t Low, size_t ArrayId = 0);
				bool SetDecimalStringAt(const Core::String& Key, const Core::String& Value, size_t ArrayId = 0);
				bool SetDecimalIntegerAt(const Core::String& Key, int64_t Value, size_t ArrayId = 0);
				bool SetDecimalNumberAt(const Core::String& Key, double Value, size_t ArrayId = 0);
				bool SetBooleanAt(const Core::String& Key, bool Value, size_t ArrayId = 0);
				bool SetObjectIdAt(const Core::String& Key, const Core::String& Value, size_t ArrayId = 0);
				bool SetNullAt(const Core::String& Key, size_t ArrayId = 0);
				bool SetPropertyAt(const Core::String& Key, Property* Value, size_t ArrayId = 0);
				bool HasPropertyAt(const Core::String& Key) const;
				bool GetPropertyAt(const Core::String& Key, Property* Output) const;
				size_t Count() const;
				Core::String ToRelaxedJSON() const;
				Core::String ToExtendedJSON() const;
				Core::String ToJSON() const;
				Core::String ToIndices() const;
				Core::Unique<Core::Schema> ToSchema(bool IsArray = false) const;
				TDocument* Get() const;
				Document Copy() const;
				Document& Persist(bool Keep = true);
				Property At(const Core::String& Name) const;
				explicit operator bool() const
				{
					return Base != nullptr;
				}
				Property operator [](const char* Name)
				{
					Property Result;
					GetProperty(Name, &Result);
					return Result;
				}
				Property operator [](const char* Name) const
				{
					Property Result;
					GetProperty(Name, &Result);
					return Result;
				}

			public:
				static Document FromEmpty();
				static Document FromSchema(Core::Schema* Document);
				static ExpectsDB<Document> FromJSON(const Core::String& JSON);
				static Document FromBuffer(const unsigned char* Buffer, size_t Length);
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
				void SetOption(const Core::String& Name, int64_t Value);
				void SetOption(const Core::String& Name, bool Value);
				void SetOption(const Core::String& Name, const Core::String& Value);
				void SetAuthMechanism(const Core::String& Value);
				void SetAuthSource(const Core::String& Value);
				void SetCompressors(const Core::String& Value);
				void SetDatabase(const Core::String& Value);
				void SetUsername(const Core::String& Value);
				void SetPassword(const Core::String& Value);
				TAddress* Get() const;
				explicit operator bool() const
				{
					return Base != nullptr;
				}

			public:
				static ExpectsDB<Address> FromURL(const Core::String& Location);
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
				ExpectsDB<void> TemplateQuery(const Core::String& Name, Core::Unique<Core::SchemaArgs> Map, bool Once = true);
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
				ExpectsPromiseDB<Property> GetProperty(const Core::String& Name);
				Cursor&& GetCursor();
				Document&& GetDocument();
				bool Success() const;
				ExpectsPromiseDB<Property> operator [](const Core::String& Name)
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
				ExpectsPromiseDB<Response> TemplateQuery(Collection& Base, const Core::String& Name, Core::Unique<Core::SchemaArgs> Map, bool Once = true);
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
				ExpectsPromiseDB<void> Rename(const Core::String& NewDatabaseName, const Core::String& NewCollectionName);
				ExpectsPromiseDB<void> RenameWithOptions(const Core::String& NewDatabaseName, const Core::String& NewCollectionName, const Document& Options);
				ExpectsPromiseDB<void> RenameWithRemove(const Core::String& NewDatabaseName, const Core::String& NewCollectionName);
				ExpectsPromiseDB<void> RenameWithOptionsAndRemove(const Core::String& NewDatabaseName, const Core::String& NewCollectionName, const Document& Options);
				ExpectsPromiseDB<void> Remove(const Document& Options);
				ExpectsPromiseDB<void> RemoveIndex(const Core::String& Name, const Document& Options);
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
				ExpectsPromiseDB<Response> TemplateQuery(const Core::String& Name, Core::Unique<Core::SchemaArgs> Map, bool Once = true, const Transaction& Session = Transaction());
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
				ExpectsPromiseDB<void> RemoveUser(const Core::String& Name);
				ExpectsPromiseDB<void> Remove();
				ExpectsPromiseDB<void> RemoveWithOptions(const Document& Options);
				ExpectsPromiseDB<void> AddUser(const Core::String& Username, const Core::String& Password, const Document& Roles, const Document& Custom);
				ExpectsPromiseDB<void> HasCollection(const Core::String& Name);
				ExpectsPromiseDB<Collection> CreateCollection(const Core::String& Name, const Document& Options);
				ExpectsPromiseDB<Cursor> FindCollections(const Document& Options);
				ExpectsDB<Core::Vector<Core::String>> GetCollectionNames(const Document& Options) const;
				Core::String GetName() const;
				Collection GetCollection(const Core::String& Name);
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
				void SetProfile(const Core::String& Name);
				ExpectsDB<void> SetServer(bool Writeable);
				ExpectsDB<Transaction*> GetSession();
				Database GetDatabase(const Core::String& Name) const;
				Database GetDefaultDatabase() const;
				Collection GetCollection(const Core::String& DatabaseName, const Core::String& Name) const;
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
				void SetProfile(const Core::String& Name);
				void Push(Connection** Client);
				Connection* Pop();
				TConnectionPool* Get() const;
				Address& GetAddress();
			};

			class VI_OUT_TS Utils
			{
			public:
				static bool GetId(unsigned char* Id12) noexcept;
				static bool GetDecimal(const char* Value, int64_t* High, int64_t* Low) noexcept;
				static unsigned int GetHashId(unsigned char* Id12) noexcept;
				static int64_t GetTimeId(unsigned char* Id12) noexcept;
				static Core::String IdToString(unsigned char* Id12) noexcept;
				static Core::String StringToId(const Core::String& Id24) noexcept;
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
				void AddConstant(const Core::String& Name, const Core::String& Value) noexcept;
				ExpectsDB<void> AddQuery(const Core::String& Name, const Core::String& Data);
				ExpectsDB<void> AddQueryFromBuffer(const Core::String& Name, const char* Buffer, size_t Size);
				ExpectsDB<void> AddDirectory(const Core::String& Directory, const Core::String& Origin = "");
				bool RemoveConstant(const Core::String& Name) noexcept;
				bool RemoveQuery(const Core::String& Name) noexcept;
				bool LoadCacheDump(Core::Schema* Dump) noexcept;
				Core::Schema* GetCacheDump() noexcept;
				ExpectsDB<Document> GetQuery(const Core::String& Name, Core::Unique<Core::SchemaArgs> Map, bool Once = true) noexcept;
				Core::Vector<Core::String> GetQueries() noexcept;
			};
		}
	}
}
#endif
