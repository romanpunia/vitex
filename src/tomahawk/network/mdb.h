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

#define TH_MDB_COMPILE(Args, Code) Tomahawk::Network::MDB::Driver::GetSubquery(#Code, Args, true)
#define TH_MDB_RECOMPILE(Args, Code) Tomahawk::Network::MDB::Driver::GetSubquery(#Code, Args, false)

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
			typedef std::unordered_map<std::string, Rest::Document*> QueryMap;

			class Connection;

			class Queue;

			enum Query
			{
				Query_None = 0,
				Query_Tailable_Cursor = 1 << 1,
				Query_Slave_Ok = 1 << 2,
				Query_Oplog_Replay = 1 << 3,
				Query_No_Cursor_Timeout = 1 << 4,
				Query_Await_Data = 1 << 5,
				Query_Exhaust = 1 << 6,
				Query_Partial = 1 << 7,
			};

			enum Type
			{
				Type_Unknown,
				Type_Uncastable,
				Type_Null,
				Type_Document,
				Type_Array,
				Type_String,
				Type_Integer,
				Type_Number,
				Type_Boolean,
				Type_ObjectId,
				Type_Decimal
			};

			struct TH_OUT Property
			{
				std::string Name;
				std::string String;
				TDocument* Document = nullptr;
				TDocument* Array = nullptr;
				Type Mod = Type_Unknown;
				int64_t Integer = 0;
				uint64_t High = 0;
				uint64_t Low = 0;
				double Number = 0;
				unsigned char ObjectId[12] = { 0 };
				bool Boolean = false;
				bool IsValid = false;

				~Property();
				void Release();
				std::string& ToString();
			};

			class TH_OUT Util
			{
			public:
				static bool ParseDecimal(const char* Value, int64_t* High, int64_t* Low);
				static bool GenerateId(unsigned char* Id12);
				static unsigned int GetHashId(unsigned char* Id12);
				static int64_t GetTimeId(unsigned char* Id12);
				static std::string OIdToString(unsigned char* Id12);
				static std::string StringToOId(const std::string& Id24);
			};

			class TH_OUT Document
			{
			public:
				static TDocument* Create(bool Array = false);
				static TDocument* Create(Rest::Document* Document);
				static TDocument* Create(const std::string& JSON);
				static TDocument* Create(const unsigned char* Buffer, uint64_t Length);
				static void Release(TDocument** Document);
				static void Loop(TDocument* Document, void* Context, const std::function<bool(TDocument*, Property*, void*)>& Callback);
				static void Join(TDocument* Document, TDocument* Value);
				static bool SetDocument(TDocument* Document, const char* Key, TDocument** Value, uint64_t ArrayId = 0);
				static bool SetArray(TDocument* Document, const char* Key, TDocument** Array, uint64_t ArrayId = 0);
				static bool SetString(TDocument* Document, const char* Key, const char* Value, uint64_t ArrayId = 0);
				static bool SetStringBuffer(TDocument* Document, const char* Key, const char* Value, uint64_t Length, uint64_t ArrayId = 0);
				static bool SetInteger(TDocument* Document, const char* Key, int64_t Value, uint64_t ArrayId = 0);
				static bool SetNumber(TDocument* Document, const char* Key, double Value, uint64_t ArrayId = 0);
				static bool SetDecimal(TDocument* Document, const char* Key, uint64_t High, uint64_t Low, uint64_t ArrayId = 0);
				static bool SetDecimalString(TDocument* Document, const char* Key, const std::string& Value, uint64_t ArrayId = 0);
				static bool SetDecimalInteger(TDocument* Document, const char* Key, int64_t Value, uint64_t ArrayId = 0);
				static bool SetDecimalNumber(TDocument* Document, const char* Key, double Value, uint64_t ArrayId = 0);
				static bool SetBoolean(TDocument* Document, const char* Key, bool Value, uint64_t ArrayId = 0);
				static bool SetObjectId(TDocument* Document, const char* Key, unsigned char Value[12], uint64_t ArrayId = 0);
				static bool SetNull(TDocument* Document, const char* Key, uint64_t ArrayId = 0);
				static bool Set(TDocument* Document, const char* Key, Property* Value, uint64_t ArrayId = 0);
				static bool Has(TDocument* Document, const char* Key);
				static bool Get(TDocument* Document, const char* Key, Property* Output);
				static bool Find(TDocument* Document, const char* Key, Property* Output);
				static uint64_t Count(TDocument* Document);
				static std::string ToRelaxedJSON(TDocument* Document);
				static std::string ToExtendedJSON(TDocument* Document);
				static std::string ToJSON(TDocument* Document);
				static Rest::Document* ToDocument(TDocument* Document, bool IsArray = false);
				static TDocument* Copy(TDocument* Document);

			private:
				static bool Clone(void* It, Property* Output);
			};

			class TH_OUT Address
			{
			public:
				static TAddress* Create(const char* Uri);
				static void Release(TAddress** URI);
				static void SetOption(TAddress* URI, const char* Name, int64_t Value);
				static void SetOption(TAddress* URI, const char* Name, bool Value);
				static void SetOption(TAddress* URI, const char* Name, const char* Value);
				static void SetAuthMechanism(TAddress* URI, const char* Value);
				static void SetAuthSource(TAddress* URI, const char* Value);
				static void SetCompressors(TAddress* URI, const char* Value);
				static void SetDatabase(TAddress* URI, const char* Value);
				static void SetUsername(TAddress* URI, const char* Value);
				static void SetPassword(TAddress* URI, const char* Value);
			};

			class TH_OUT Stream
			{
			public:
				static TStream* Create(bool IsOrdered);
				static void Release(TStream** Operation);
				static bool RemoveMany(TStream* Operation, TDocument** Selector, TDocument** Options);
				static bool RemoveOne(TStream* Operation, TDocument** Selector, TDocument** Options);
				static bool ReplaceOne(TStream* Operation, TDocument** Selector, TDocument** Replacement, TDocument** Options);
				static bool Insert(TStream* Operation, TDocument** Document, TDocument** Options);
				static bool UpdateOne(TStream* Operation, TDocument** Selector, TDocument** Document, TDocument** Options);
				static bool UpdateMany(TStream* Operation, TDocument** Selector, TDocument** Document, TDocument** Options);
				static bool RemoveMany(TStream* Operation, TDocument* Selector, TDocument* Options);
				static bool RemoveOne(TStream* Operation, TDocument* Selector, TDocument* Options);
				static bool ReplaceOne(TStream* Operation, TDocument* Selector, TDocument* Replacement, TDocument* Options);
				static bool Insert(TStream* Operation, TDocument* Document, TDocument* Options);
				static bool UpdateOne(TStream* Operation, TDocument* Selector, TDocument* Document, TDocument* Options);
				static bool UpdateMany(TStream* Operation, TDocument* Selector, TDocument* Document, TDocument* Options);
				static bool Execute(TStream** Operation, TDocument** Reply);
				static bool Execute(TStream** Operation);
				static uint64_t GetHint(TStream* Operation);
			};

			class TH_OUT Watcher
			{
			public:
				static TWatcher* Create(TConnection* Connection, TDocument** Pipeline, TDocument** Options);
				static TWatcher* Create(TDatabase* Database, TDocument** Pipeline, TDocument** Options);
				static TWatcher* Create(TCollection* Collection, TDocument** Pipeline, TDocument** Options);
				static void Release(TWatcher** Stream);
				static bool Next(TWatcher* Stream, TDocument** Result);
				static bool Error(TWatcher* Stream, TDocument** Result);
			};

			class TH_OUT Cursor
			{
			public:
				static void Release(TCursor** Cursor);
				static void SetMaxAwaitTime(TCursor* Cursor, uint64_t MaxAwaitTime);
				static void SetBatchSize(TCursor* Cursor, uint64_t BatchSize);
				static void Receive(TCursor* Cursor, void* Context, bool(* Next)(TCursor*, TDocument*, void*));
				static bool Next(TCursor* Cursor);
				static bool SetLimit(TCursor* Cursor, int64_t Limit);
				static bool SetHint(TCursor* Cursor, uint64_t Hint);
				static bool HasError(TCursor* Cursor);
				static bool HasMoreData(TCursor* Cursor);
				static int64_t GetId(TCursor* Cursor);
				static int64_t GetLimit(TCursor* Cursor);
				static uint64_t GetMaxAwaitTime(TCursor* Cursor);
				static uint64_t GetBatchSize(TCursor* Cursor);
				static uint64_t GetHint(TCursor* Cursor);
				static TDocument* GetCurrent(TCursor* Cursor);
			};

			class TH_OUT Collection
			{
			public:
				static void Release(TCollection** Collection);
				static bool UpdateMany(TCollection* Collection, TDocument** Selector, TDocument** Update, TDocument** Options, TDocument** Reply);
				static bool UpdateOne(TCollection* Collection, TDocument** Selector, TDocument** Update, TDocument** Options, TDocument** Reply);
				static bool Rename(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName);
				static bool RenameWithOptions(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName, TDocument** Options);
				static bool RenameWithRemove(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName);
				static bool RenameWithOptionsAndRemove(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName, TDocument** Options);
				static bool Remove(TCollection* Collection, TDocument** Options = nullptr);
				static bool RemoveMany(TCollection* Collection, TDocument** Selector, TDocument** Options, TDocument** Reply);
				static bool RemoveOne(TCollection* Collection, TDocument** Selector, TDocument** Options, TDocument** Reply);
				static bool RemoveIndex(TCollection* Collection, const char* Name, TDocument** Options = nullptr);
				static bool ReplaceOne(TCollection* Collection, TDocument** Selector, TDocument** Replacement, TDocument** Options, TDocument** Reply);
				static bool InsertMany(TCollection* Collection, TDocument** Documents, uint64_t Length, TDocument** Options, TDocument** Reply);
				static bool InsertMany(TCollection* Collection, const std::vector<TDocument*>& Documents, TDocument** Options, TDocument** Reply);
				static bool InsertOne(TCollection* Collection, TDocument** Document, TDocument** Options, TDocument** Reply);
				static bool FindAndModify(TCollection* Collection, TDocument** Selector, TDocument** Sort, TDocument** Update, TDocument** Fields, TDocument** Reply, bool Remove, bool Upsert, bool New);
				static uint64_t CountElementsInArray(TCollection* Collection, TDocument** Match, TDocument** Filter, TDocument** Options);
				static uint64_t CountDocuments(TCollection* Collection, TDocument** Filter, TDocument** Options, TDocument** Reply);
				static uint64_t CountDocumentsEstimated(TCollection* Collection, TDocument** Options, TDocument** Reply);
				static std::string StringifyKeyIndexes(TCollection* Collection, TDocument** Keys);
				static const char* GetName(TCollection* Collection);
				static TCursor* FindIndexes(TCollection* Collection, TDocument** Options);
				static TCursor* FindMany(TCollection* Collection, TDocument** Filter, TDocument** Options);
				static TCursor* FindOne(TCollection* Collection, TDocument** Filter, TDocument** Options);
				static TCursor* Aggregate(TCollection* Collection, Query Flags, TDocument** Pipeline, TDocument** Options);
				static TStream* CreateStream(TCollection* Collection, TDocument** Options);
			};

			class TH_OUT Database
			{
			public:
				static void Release(TDatabase** Database);
				static bool HasCollection(TDatabase* Database, const char* Name);
				static bool RemoveAllUsers(TDatabase* Database);
				static bool RemoveUser(TDatabase* Database, const char* Name);
				static bool Remove(TDatabase* Database);
				static bool RemoveWithOptions(TDatabase* Database, TDocument** Options);
				static bool AddUser(TDatabase* Database, const char* Username, const char* Password, TDocument** Roles, TDocument** Custom);
				static std::vector<std::string> GetCollectionNames(TDatabase* Database, TDocument** Options);
				static const char* GetName(TDatabase* Database);
				static TCursor* FindCollections(TDatabase* Database, TDocument** Options);
				static TCollection* CreateCollection(TDatabase* Database, const char* Name, TDocument** Options);
				static TCollection* GetCollection(TDatabase* Database, const char* Name);
			};

			class TH_OUT Transaction
			{
			public:
				static TTransaction* Create(Connection* Client, TDocument** NewUid);
				static void Release(Connection* Client);
				static bool Start(TTransaction* Session);
				static bool Abort(TTransaction* Session);
				static bool Commit(TTransaction* Session, TDocument** Reply);
			};

			class TH_OUT Connection : public Rest::Object
			{
				friend Queue;
				friend Transaction;

			private:
				TTransaction* Session;
				TConnection* Base;
				Queue* Master;
				bool Connected;

			public:
				Connection();
				virtual ~Connection() override;
				void SetProfile(const char* Name);
				bool SetServer(bool Writeable);
				bool Connect(const std::string& Address);
				bool Connect(TAddress* URI);
				bool Disconnect();
				TCursor* FindDatabases(TDocument** Options);
				TDatabase* GetDatabase(const char* Name);
				TDatabase* GetDefaultDatabase();
				TCollection* GetCollection(const char* DatabaseName, const char* Name);
				TConnection* Get();
				TAddress* GetAddress();
				Queue* GetMaster();
				std::vector<std::string> GetDatabaseNames(TDocument** Options);
				bool IsConnected();
			};

			class TH_OUT Queue : public Rest::Object
			{
			private:
				TConnectionPool* Pool;
				TAddress* Address;
				bool Connected;

			public:
				Queue();
				virtual ~Queue() override;
				void SetProfile(const char* Name);
				bool Connect(const std::string& Address);
				bool Connect(TAddress* URI);
				bool Disconnect();
				void Push(Connection** Client);
				Connection* Pop();
				TConnectionPool* Get();
				TAddress* GetAddress();
			};

			class TH_OUT Driver
			{
			private:
				struct Sequence
				{
					TDocument* Cache = nullptr;
					std::string Request;
				};

			private:
				static std::unordered_map<std::string, Sequence>* Queries;
				static std::mutex* Safe;
				static int State;

			public:
				static void Create();
				static void Release();
				static bool AddQuery(const std::string& Name, const char* Buffer, size_t Size);
				static bool AddDirectory(const std::string& Directory, const std::string& Origin = "");
				static bool RemoveQuery(const std::string& Name);
				static TDocument* GetQuery(const std::string& Name, QueryMap* Map, bool Once = true);
				static TDocument* GetSubquery(const char* Buffer, QueryMap* Map, bool Once = true);
				static std::vector<std::string> GetQueries();

			private:
				static std::string GetJSON(Rest::Document* Source);
			};
		}
	}
}
#endif