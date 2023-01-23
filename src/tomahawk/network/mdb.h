#ifndef TH_NETWORK_MONGODB_H
#define TH_NETWORK_MONGODB_H
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

namespace Tomahawk
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
			typedef std::function<void(const std::string&)> OnQueryLog;

			class Transaction;

			class Connection;

			class Cluster;

			class Document;

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

			struct TH_OUT Property
			{
				std::string Name;
				std::string String;
				TDocument* Source;
				Type Mod;
				int64_t Integer;
				uint64_t High;
				uint64_t Low;
				double Number;
				unsigned char ObjectId[12] = { 0 };
				bool Boolean;
				bool IsValid;

				Property() noexcept;
				Property(const Property& Other) = delete;
				Property(Property&& Other);
				~Property();
				Property& operator =(const Property& Other);
				Property& operator =(Property&& Other);
				Core::Unique<TDocument> LoseOwnership();
				std::string& ToString();
				Document Get() const;
				Property operator [](const char* Name);
				Property operator [](const char* Name) const;
			};

			class TH_OUT_TS Util
			{
			public:
				static bool GetId(unsigned char* Id12);
				static bool GetDecimal(const char* Value, int64_t* High, int64_t* Low);
				static unsigned int GetHashId(unsigned char* Id12);
				static int64_t GetTimeId(unsigned char* Id12);
				static std::string IdToString(unsigned char* Id12);
				static std::string StringToId(const std::string& Id24);
			};

			class TH_OUT Document
			{
			private:
				TDocument* Base;
				bool Store;

			public:
				Document(TDocument* NewBase = nullptr);
				Document(const Document& Other) = delete;
				Document(Document&& Other);
				~Document();
				Document& operator =(const Document& Other) = delete;
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
				bool SetDecimalString(const char* Key, const std::string& Value, size_t ArrayId = 0);
				bool SetDecimalInteger(const char* Key, int64_t Value, size_t ArrayId = 0);
				bool SetDecimalNumber(const char* Key, double Value, size_t ArrayId = 0);
				bool SetBoolean(const char* Key, bool Value, size_t ArrayId = 0);
				bool SetObjectId(const char* Key, unsigned char Value[12], size_t ArrayId = 0);
				bool SetNull(const char* Key, size_t ArrayId = 0);
				bool SetProperty(const char* Key, Property* Value, size_t ArrayId = 0);
				bool HasProperty(const char* Key) const;
				bool GetProperty(const char* Key, Property* Output) const;
				size_t Count() const;
				std::string ToRelaxedJSON() const;
				std::string ToExtendedJSON() const;
				std::string ToJSON() const;
				std::string ToIndices() const;
				Core::Unique<Core::Schema> ToSchema(bool IsArray = false) const;
				TDocument* Get() const;
				Document Copy() const;
				Document& Persist(bool Keep = true);
				operator bool() const
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
				static Document FromDocument(Core::Schema* Document);
				static Document FromJSON(const std::string& JSON);
				static Document FromBuffer(const unsigned char* Buffer, size_t Length);
				static Document FromSource(TDocument* Src);

			private:
				static bool Clone(void* It, Property* Output);
			};

			class TH_OUT Address
			{
			private:
				TAddress* Base;

			public:
				Address(TAddress* NewBase = nullptr);
				Address(const Address& Other) = delete;
				Address(Address&& Other);
				~Address();
				Address& operator =(const Address& Other) = delete;
				Address& operator =(Address&& Other);
				void SetOption(const char* Name, int64_t Value);
				void SetOption(const char* Name, bool Value);
				void SetOption(const char* Name, const char* Value);
				void SetAuthMechanism(const char* Value);
				void SetAuthSource(const char* Value);
				void SetCompressors(const char* Value);
				void SetDatabase(const char* Value);
				void SetUsername(const char* Value);
				void SetPassword(const char* Value);
				TAddress* Get() const;
				operator bool() const
				{
					return Base != nullptr;
				}

			public:
				static Address FromURI(const char* Value);
			};

			class TH_OUT Stream
			{
			private:
				Document NetOptions;
				TCollection* Source;
				TStream* Base;
				size_t Count;

			public:
				Stream();
				Stream(TCollection* NewSource, TStream* NewBase, Document&& NewOptions);
				Stream(const Stream& Other) = delete;
				Stream(Stream&& Other);
				~Stream();
				Stream& operator =(const Stream& Other) = delete;
				Stream& operator =(Stream&& Other);
				bool RemoveMany(const Document& Match, const Document& Options);
				bool RemoveOne(const Document& Match, const Document& Options);
				bool ReplaceOne(const Document& Match, const Document& Replacement, const Document& Options);
				bool InsertOne(const Document& Result, const Document& Options);
				bool UpdateOne(const Document& Match, const Document& Result, const Document& Options);
				bool UpdateMany(const Document& Match, const Document& Result, const Document& Options);
				bool TemplateQuery(const std::string& Name, Core::Unique<Core::SchemaArgs> Map, bool Once = true);
				bool Query(const Document& Command);
				Core::Promise<Document> ExecuteWithReply();
				Core::Promise<bool> Execute();
				size_t GetHint() const;
				TStream* Get() const;
				operator bool() const
				{
					return Base != nullptr;
				}

			private:
				bool NextOperation();
			};

			class TH_OUT Cursor
			{
			private:
				TCursor* Base;

			public:
				Cursor(TCursor* NewBase = nullptr);
				Cursor(const Cursor& Other) = delete;
				Cursor(Cursor&& Other);
				~Cursor();
				Cursor& operator =(const Cursor& Other) = delete;
				Cursor& operator =(Cursor&& Other);
				void SetMaxAwaitTime(size_t MaxAwaitTime);
				void SetBatchSize(size_t BatchSize);
				bool SetLimit(int64_t Limit);
				bool SetHint(size_t Hint);
				bool HasError() const;
				bool HasMoreData() const;
				Core::Promise<bool> Next() const;
				int64_t GetId() const;
				int64_t GetLimit() const;
				size_t GetMaxAwaitTime() const;
				size_t GetBatchSize() const;
				size_t GetHint() const;
				Document GetCurrent() const;
				Cursor Clone();
				TCursor* Get() const;
				operator bool() const
				{
					return Base != nullptr;
				}

			private:
				void Cleanup();
			};

			class TH_OUT Response
			{
			private:
				Cursor NetCursor;
				Document NetDocument;
				bool NetSuccess;

			public:
				Response(bool NewSuccess = false);
				Response(Cursor&& NewCursor);
				Response(Document&& NewDocument);
				Response(const Response& Other) = delete;
				Response(Response&& Other);
				~Response() = default;
				Response& operator =(const Response& Other) = delete;
				Response& operator =(Response&& Other);
				Core::Promise<Core::Unique<Core::Schema>> Fetch() const;
				Core::Promise<Core::Unique<Core::Schema>> FetchAll() const;
				Property GetProperty(const char* Name);
				Cursor&& GetCursor();
				Document&& GetDocument();
				bool IsSuccess() const;
				Property operator [](const char* Name)
				{
					return GetProperty(Name);
				}
				operator bool() const
				{
					return IsSuccess();
				}
			};

			class TH_OUT Collection
			{
			private:
				TCollection* Base;

			public:
				Collection(TCollection* NewBase = nullptr);
				Collection(const Collection& Other) = delete;
				Collection(Collection&& Other);
				~Collection();
				Collection& operator =(const Collection& Other) = delete;
				Collection& operator =(Collection&& Other);
				Core::Promise<bool> Rename(const std::string& NewDatabaseName, const std::string& NewCollectionName) const;
				Core::Promise<bool> RenameWithOptions(const std::string& NewDatabaseName, const std::string& NewCollectionName, const Document& Options) const;
				Core::Promise<bool> RenameWithRemove(const std::string& NewDatabaseName, const std::string& NewCollectionName) const;
				Core::Promise<bool> RenameWithOptionsAndRemove(const std::string& NewDatabaseName, const std::string& NewCollectionName, const Document& Options) const;
				Core::Promise<bool> Remove(const Document& Options) const;
				Core::Promise<bool> RemoveIndex(const std::string& Name, const Document& Options) const;
				Core::Promise<Document> RemoveMany(const Document& Match, const Document& Options) const;
				Core::Promise<Document> RemoveOne(const Document& Match, const Document& Options) const;
				Core::Promise<Document> ReplaceOne(const Document& Match, const Document& Replacement, const Document& Options) const;
				Core::Promise<Document> InsertMany(std::vector<Document>& List, const Document& Options) const;
				Core::Promise<Document> InsertOne(const Document& Result, const Document& Options) const;
				Core::Promise<Document> UpdateMany(const Document& Match, const Document& Update, const Document& Options) const;
				Core::Promise<Document> UpdateOne(const Document& Match, const Document& Update, const Document& Options) const;
				Core::Promise<Document> FindAndModify(const Document& Match, const Document& Sort, const Document& Update, const Document& Fields, bool Remove, bool Upsert, bool New) const;
				Core::Promise<size_t> CountDocuments(const Document& Match, const Document& Options) const;
				Core::Promise<size_t> CountDocumentsEstimated(const Document& Options) const;
				Core::Promise<Cursor> FindIndexes(const Document& Options) const;
				Core::Promise<Cursor> FindMany(const Document& Match, const Document& Options) const;
				Core::Promise<Cursor> FindOne(const Document& Match, const Document& Options) const;
				Core::Promise<Cursor> Aggregate(QueryFlags Flags, const Document& Pipeline, const Document& Options) const;
				Core::Promise<Response> TemplateQuery(const std::string& Name, Core::Unique<Core::SchemaArgs> Map, bool Once = true, Transaction* Session = nullptr) const;
				Core::Promise<Response> Query(const Document& Command, Transaction* Session = nullptr) const;
				const char* GetName() const;
				Stream CreateStream(Document&& Options) const;
				TCollection* Get() const;
				operator bool() const
				{
					return Base != nullptr;
				}
			};

			class TH_OUT Database
			{
			private:
				TDatabase* Base;

			public:
				Database(TDatabase* NewBase = nullptr);
				Database(const Database& Other) = delete;
				Database(Database&& Other);
				~Database();
				Database& operator =(const Database& Other) = delete;
				Database& operator =(Database&& Other);
				Core::Promise<bool> RemoveAllUsers();
				Core::Promise<bool> RemoveUser(const std::string& Name);
				Core::Promise<bool> Remove();
				Core::Promise<bool> RemoveWithOptions(const Document& Options);
				Core::Promise<bool> AddUser(const std::string& Username, const std::string& Password, const Document& Roles, const Document& Custom);
				Core::Promise<bool> HasCollection(const std::string& Name) const;
				Core::Promise<Collection> CreateCollection(const std::string& Name, const Document& Options);
				Core::Promise<Cursor> FindCollections(const Document& Options) const;
				std::vector<std::string> GetCollectionNames(const Document& Options) const;
				const char* GetName() const;
				Collection GetCollection(const std::string& Name);
				TDatabase* Get() const;
				operator bool() const
				{
					return Base != nullptr;
				}
			};

			class TH_OUT Watcher
			{
			private:
				TWatcher* Base;

			public:
				Watcher(TWatcher* NewBase);
				Watcher(const Watcher& Other) = delete;
				Watcher(Watcher&& Other);
				~Watcher();
				Watcher& operator =(const Watcher& Other) = delete;
				Watcher& operator =(Watcher&& Other);
				Core::Promise<bool> Next(Document& Result) const;
				Core::Promise<bool> Error(Document& Result) const;
				TWatcher* Get() const;
				operator bool() const
				{
					return Base != nullptr;
				}

			public:
				static Watcher FromConnection(Connection* Connection, const Document& Pipeline, const Document& Options);
				static Watcher FromDatabase(const Database& Src, const Document& Pipeline, const Document& Options);
				static Watcher FromCollection(const Collection& Src, const Document& Pipeline, const Document& Options);
			};

			class TH_OUT Transaction
			{
			private:
				TTransaction* Base;

			public:
				Transaction(TTransaction* NewBase);
				bool Push(Document& QueryOptions) const;
				bool Put(TDocument** QueryOptions) const;
				Core::Promise<bool> Start();
				Core::Promise<bool> Abort();
				Core::Promise<Document> RemoveMany(const Collection& Base, const Document& Match, Document&& Options);
				Core::Promise<Document> RemoveOne(const Collection& Base, const Document& Match, Document&& Options);
				Core::Promise<Document> ReplaceOne(const Collection& Base, const Document& Match, const Document& Replacement, Document&& Options);
				Core::Promise<Document> InsertMany(const Collection& Base, std::vector<Document>& List, Document&& Options);
				Core::Promise<Document> InsertOne(const Collection& Base, const Document& Result, Document&& Options);
				Core::Promise<Document> UpdateMany(const Collection& Base, const Document& Match, const Document& Update, Document&& Options);
				Core::Promise<Document> UpdateOne(const Collection& Base, const Document& Match, const Document& Update, Document&& Options);
				Core::Promise<Cursor> FindMany(const Collection& Base, const Document& Match, Document&& Options) const;
				Core::Promise<Cursor> FindOne(const Collection& Base, const Document& Match, Document&& Options) const;
				Core::Promise<Cursor> Aggregate(const Collection& Base, QueryFlags Flags, const Document& Pipeline, Document&& Options) const;
				Core::Promise<Response> TemplateQuery(const Collection& Base, const std::string& Name, Core::Unique<Core::SchemaArgs> Map, bool Once = true);
				Core::Promise<Response> Query(const Collection& Base, const Document& Command);
				Core::Promise<TransactionState> Commit();
				TTransaction* Get() const;
				operator bool() const
				{
					return Base != nullptr;
				}
			};

			class TH_OUT Connection : public Core::Object
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
				virtual ~Connection() override;
				Core::Promise<bool> Connect(const std::string& Address);
				Core::Promise<bool> Connect(Address* URI);
				Core::Promise<bool> Disconnect();
				Core::Promise<bool> MakeTransaction(const std::function<Core::Promise<bool>(Transaction&)>& Callback);
				Core::Promise<bool> MakeCotransaction(const std::function<bool(Transaction&)>& Callback);
				Core::Promise<Cursor> FindDatabases(const Document& Options) const;
				void SetProfile(const std::string& Name);
				bool SetServer(bool Writeable);
				Transaction& GetSession();
				Database GetDatabase(const std::string& Name) const;
				Database GetDefaultDatabase() const;
				Collection GetCollection(const char* DatabaseName, const char* Name) const;
				Address GetAddress() const;
				Cluster* GetMaster() const;
				TConnection* Get() const;
				std::vector<std::string> GetDatabaseNames(const Document& Options) const;
				bool IsConnected() const;
			};

			class TH_OUT_TS Cluster : public Core::Object
			{
			private:
				std::atomic<bool> Connected;
				TConnectionPool* Pool;
				Address SrcAddress;

			public:
				Cluster();
				virtual ~Cluster() override;
				Core::Promise<bool> Connect(const std::string& Address);
				Core::Promise<bool> Connect(Address* URI);
				Core::Promise<bool> Disconnect();
				void SetProfile(const char* Name);
				void Push(Connection** Client);
				Connection* Pop();
				TConnectionPool* Get() const;
				Address& GetAddress();
			};

			class TH_OUT_TS Driver
			{
			private:
				struct Pose
				{
					std::string Key;
					size_t Offset = 0;
					bool Escape = false;
				};

				struct Sequence
				{
					std::vector<Pose> Positions;
					std::string Request;
					Document Cache;

					Sequence();
					Sequence(const Sequence& Other);
					Sequence& operator =(Sequence&& Other) = default;
				};

			private:
				static Core::Mapping<std::unordered_map<std::string, Sequence>>* Queries;
				static std::mutex* Safe;
				static std::atomic<int> State;
				static OnQueryLog Logger;
				static void* APM;

			public:
				static void Create();
				static void Release();
				static void SetQueryLog(const OnQueryLog& Callback);
				static void AttachQueryLog(TConnection* Connection);
				static void AttachQueryLog(TConnectionPool* Connection);
				static bool AddQuery(const std::string& Name, const char* Buffer, size_t Size);
				static bool AddDirectory(const std::string& Directory, const std::string& Origin = "");
				static bool RemoveQuery(const std::string& Name);
				static Document GetQuery(const std::string& Name, Core::Unique<Core::SchemaArgs> Map, bool Once = true);
				static std::vector<std::string> GetQueries();

			private:
				static std::string GetJSON(Core::Schema* Source, bool Escape);
			};
		}
	}
}
#endif
