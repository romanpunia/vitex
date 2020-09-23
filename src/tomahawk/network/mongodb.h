#ifndef TH_NETWORK_MONGODB_H
#define TH_NETWORK_MONGODB_H

#include "../core/network.h"
#include "bson.h"

struct _mongoc_client_t;
struct _mongoc_uri_t;
struct _mongoc_database_t;
struct _mongoc_collection_t;
struct _mongoc_server_description_t;
struct _mongoc_cursor_t;
struct _mongoc_read_concern_t;
struct _mongoc_read_prefs_t;
struct _mongoc_write_concern_t;
struct _mongoc_topology_description_t;
struct _mongoc_bulk_operation_t;
struct _mongoc_find_and_modify_opts_t;
struct _mongoc_client_pool_t;
struct _mongoc_change_stream_t;

namespace Tomahawk
{
	namespace Network
	{
		namespace MongoDB
		{
			typedef _mongoc_client_pool_t TConnectionPool;
			typedef _mongoc_find_and_modify_opts_t TFindAndModifyOptions;
			typedef _mongoc_bulk_operation_t TBulkOperation;
			typedef _mongoc_topology_description_t TTopologyDescription;
			typedef _mongoc_write_concern_t TWriteConcern;
			typedef _mongoc_read_prefs_t TReadPreferences;
			typedef _mongoc_read_concern_t TReadConcern;
			typedef _mongoc_cursor_t TCursor;
			typedef _mongoc_server_description_t TServerDescription;
			typedef _mongoc_collection_t TCollection;
			typedef _mongoc_database_t TDatabase;
			typedef _mongoc_uri_t TURI;
			typedef _mongoc_client_t TConnection;
			typedef _mongoc_change_stream_t TChangeStream;
			typedef std::unordered_map<std::string, Rest::Document*> QueryMap;

			class Client;

			class ClientPool;

			enum ReadMode
			{
				ReadMode_Primary = (1 << 0),
				ReadMode_Secondary = (1 << 1),
				ReadMode_Primary_Preferred = (1 << 2) | ReadMode_Primary,
				ReadMode_Secondary_Preferred = (1 << 2) | ReadMode_Secondary,
				ReadMode_Nearest = (1 << 3) | ReadMode_Secondary,
			};

			enum QueryMode
			{
				QueryMode_None = 0,
				QueryMode_Tailable_Cursor = 1 << 1,
				QueryMode_Slave_Ok = 1 << 2,
				QueryMode_Oplog_Replay = 1 << 3,
				QueryMode_No_Cursor_Timeout = 1 << 4,
				QueryMode_Await_Data = 1 << 5,
				QueryMode_Exhaust = 1 << 6,
				QueryMode_Partial = 1 << 7,
			};

			enum InsertMode
			{
				InsertMode_None = 0,
				InsertMode_Continue_On_Error = 1 << 0,
			};

			enum UpdateMode
			{
				UpdateMode_None = 0,
				UpdateMode_Upsert = 1 << 0,
				UpdateMode_Multi_Update = 1 << 1,
			};

			enum RemoveMode
			{
				RemoveMode_None = 0,
				RemoveMode_Single_Remove = 1 << 0,
			};

			enum FindAndModifyMode
			{
				FindAndModifyMode_None = 0,
				FindAndModifyMode_Remove = 1 << 0,
				FindAndModifyMode_Upsert = 1 << 1,
				FindAndModifyMode_Return_New = 1 << 2,
			};

			struct TH_OUT HostList
			{
				HostList* Next = nullptr;
				char Hostname[256] = { 0 };
				char Address[262] = { 0 };
				uint16_t Port = 0;
				int Family = 0;
				void* Padding[4] = { nullptr };
			};

			struct TH_OUT SSLOptions
			{
				const char* PEMFile = nullptr;
				const char* PEMPassword = nullptr;
				const char* CAFile = nullptr;
				const char* CADirectory = nullptr;
				const char* CRLFile = nullptr;
				bool WeakCertificateValidation = false;
				bool AllowInvalidHostname = false;
				void* Unused[7] = { nullptr };
			};

			struct TH_OUT APMCommandStarted
			{
				BSON::TDocument* Command = nullptr;
				const char* CommandName = nullptr;
				const char* DatabaseName = nullptr;
				int64_t RequestId = 0;
				int64_t OperationId = 0;
				uint64_t ServerId = 0;
				HostList* Hosts = nullptr;
				void* Event = nullptr;
			};

			struct TH_OUT APMCommandSucceeded
			{
				BSON::TDocument* Reply = nullptr;
				const char* CommandName = nullptr;
				const char* DatabaseName = nullptr;
				int64_t Duration = 0;
				int64_t RequestId = 0;
				int64_t OperationId = 0;
				uint64_t ServerId = 0;
				HostList* Hosts = nullptr;
				void* Event = nullptr;
			};

			struct TH_OUT APMCommandFailed
			{
				BSON::TDocument* Reply = nullptr;
				const char* CommandName = nullptr;
				const char* DatabaseName = nullptr;
				int64_t Duration = 0;
				int64_t RequestId = 0;
				int64_t OperationId = 0;
				uint64_t ServerId = 0;
				HostList* Hosts = nullptr;
				void* Event = nullptr;
			};

			struct TH_OUT APMServerChanged
			{
				TServerDescription* New = nullptr;
				TServerDescription* Previous = nullptr;
				HostList* Hosts = nullptr;
				void* Event = nullptr;
			};

			struct TH_OUT APMServerOpened
			{
				HostList* Hosts = nullptr;
				void* Event = nullptr;
			};

			struct TH_OUT APMServerClosed
			{
				HostList* Hosts = nullptr;
				void* Event = nullptr;
			};

			struct TH_OUT APMTopologyChanged
			{
				TTopologyDescription* New = nullptr;
				TTopologyDescription* Previous = nullptr;
				void* Event = nullptr;
			};

			struct TH_OUT APMTopologyOpened
			{
				void* Event = nullptr;
			};

			struct TH_OUT APMTopologyClosed
			{
				void* Event = nullptr;
			};

			struct TH_OUT APMServerHeartbeatStarted
			{
				HostList* Hosts = nullptr;
				void* Event = nullptr;
			};

			struct TH_OUT APMServerHeartbeatSucceeded
			{
				BSON::TDocument* Reply = nullptr;
				int64_t Duration = 0;
				HostList* Hosts = nullptr;
				void* Event = nullptr;
			};

			struct TH_OUT APMServerHeartbeatFailed
			{
				int64_t Duration = 0;
				const char* Message = nullptr;
				uint64_t ErrorCode = 0;
				uint64_t Domain = 0;
				HostList* Hosts = nullptr;
				void* Event = nullptr;
			};

			struct TH_OUT APMCallbacks
			{
				void (* OnCommandStarted)(Client* Client, APMCommandStarted* Command) = nullptr;
				void (* OnCommandSucceeded)(Client* Client, APMCommandSucceeded* Command) = nullptr;
				void (* OnCommandFailed)(Client* Client, APMCommandFailed* Command) = nullptr;
				void (* OnServerChanged)(Client* Client, APMServerChanged* Command) = nullptr;
				void (* OnServerOpened)(Client* Client, APMServerOpened* Command) = nullptr;
				void (* OnServerClosed)(Client* Client, APMServerClosed* Command) = nullptr;
				void (* OnTopologyChanged)(Client* Client, APMTopologyChanged* Command) = nullptr;
				void (* OnTopologyOpened)(Client* Client, APMTopologyOpened* Command) = nullptr;
				void (* OnTopologyClosed)(Client* Client, APMTopologyClosed* Command) = nullptr;
				void (* OnServerHeartbeatStarted)(Client* Client, APMServerHeartbeatStarted* Command) = nullptr;
				void (* OnServerHeartbeatSucceeded)(Client* Client, APMServerHeartbeatSucceeded* Command) = nullptr;
				void (* OnServerHeartbeatFailed)(Client* Client, APMServerHeartbeatFailed* Command) = nullptr;
			};

			class TH_OUT QueryCache : public Rest::Object
			{
			private:
				struct Sequence
				{
					BSON::TDocument* Cache = nullptr;
					std::string Request;
				};

			private:
				std::unordered_map<std::string, Sequence> Queries;
				std::mutex Safe;

			public:
				QueryCache();
				virtual ~QueryCache() override;
				bool AddQuery(const std::string& Name, const char* Buffer, size_t Size);
				bool AddDirectory(const std::string& Directory, const std::string& Origin = "");
				bool RemoveQuery(const std::string& Name);
				BSON::TDocument* GetQuery(const std::string& Name, QueryMap* Map, bool Once = true);
				std::vector<std::string> GetQueries();

			private:
				std::string GetJSON(Rest::Document* Source);
			};

			class TH_OUT Connector
			{
			private:
				static QueryCache* Cache;
				static int State;

			public:
				static void Create();
				static void Release();
				static bool AddQuery(const std::string& Name, const char* Buffer, size_t Size);
				static bool AddDirectory(const std::string& Directory);
				static bool RemoveQuery(const std::string& Name);
				static BSON::TDocument* GetQuery(const std::string& Name, QueryMap* Map, bool Once = true);
				static std::vector<std::string> GetQueries();
			};

			class TH_OUT URI
			{
			public:
				static TURI* Create(const char* Uri);
				static void Release(TURI** URI);
				static void SetOption(TURI* URI, const char* Name, int64_t Value);
				static void SetOption(TURI* URI, const char* Name, bool Value);
				static void SetOption(TURI* URI, const char* Name, const char* Value);
				static void SetAuthMechanism(TURI* URI, const char* Value);
				static void SetAuthSource(TURI* URI, const char* Value);
				static void SetCompressors(TURI* URI, const char* Value);
				static void SetDatabase(TURI* URI, const char* Value);
				static void SetUsername(TURI* URI, const char* Value);
				static void SetPassword(TURI* URI, const char* Value);
			};

			class TH_OUT FindAndModifyOptions
			{
			public:
				static TFindAndModifyOptions* Create();
				static void Release(TFindAndModifyOptions** Options);
				static bool SetFlags(TFindAndModifyOptions* Options, FindAndModifyMode Flags);
				static bool SetMaxTimeMs(TFindAndModifyOptions* Options, uint64_t Time);
				static bool SetFields(TFindAndModifyOptions* Options, BSON::TDocument** Fields);
				static bool SetSort(TFindAndModifyOptions* Options, BSON::TDocument** Sort);
				static bool SetUpdate(TFindAndModifyOptions* Options, BSON::TDocument** Update);
				static bool SetBypassDocumentValidation(TFindAndModifyOptions* Options, bool Bypass);
				static bool Append(TFindAndModifyOptions* Options, BSON::TDocument** Value);
				static bool WillBypassDocumentValidation(TFindAndModifyOptions* Options);
				static uint64_t GetMaxTimeMs(TFindAndModifyOptions* Options);
				static FindAndModifyMode GetFlags(TFindAndModifyOptions* Options);
				static BSON::TDocument* GetFields(TFindAndModifyOptions* Options);
				static BSON::TDocument* GetSort(TFindAndModifyOptions* Options);
				static BSON::TDocument* GetUpdate(TFindAndModifyOptions* Options);
			};

			class TH_OUT ReadConcern
			{
			public:
				static TReadConcern* Create();
				static void Release(TReadConcern** ReadConcern);
				static void Append(TReadConcern* ReadConcern, BSON::TDocument* Options);
				static void SetLevel(TReadConcern* ReadConcern, const char* Level);
				static bool IsDefault(TReadConcern* ReadConcern);
				static const char* GetLevel(TReadConcern* ReadConcern);
			};

			class TH_OUT WriteConcern
			{
			public:
				static TWriteConcern* Create();
				static void Release(TWriteConcern** WriteConcern);
				static void Append(TWriteConcern* WriteConcern, BSON::TDocument* Options);
				static void SetToken(TWriteConcern* WriteConcern, uint64_t W);
				static void SetMajority(TWriteConcern* WriteConcern, uint64_t Timeout);
				static void SetTimeout(TWriteConcern* WriteConcern, uint64_t Timeout);
				static void SetTag(TWriteConcern* WriteConcern, const char* Tag);
				static void SetJournaled(TWriteConcern* WriteConcern, bool Enabled);
				static bool ShouldBeJournaled(TWriteConcern* WriteConcern);
				static bool HasJournal(TWriteConcern* WriteConcern);
				static bool HasMajority(TWriteConcern* WriteConcern);
				static bool IsAcknowledged(TWriteConcern* WriteConcern);
				static bool IsDefault(TWriteConcern* WriteConcern);
				static bool IsValid(TWriteConcern* WriteConcern);
				static int64_t GetToken(TWriteConcern* WriteConcern);
				static int64_t GetTimeout(TWriteConcern* WriteConcern);
				static const char* GetTag(TWriteConcern* WriteConcern);
			};

			class TH_OUT ReadPreferences
			{
			public:
				static TReadPreferences* Create();
				static void Release(TReadPreferences** ReadPreferences);
				static void SetTags(TReadPreferences* ReadPreferences, BSON::TDocument** Tags);
				static void SetMode(TReadPreferences* ReadPreferences, ReadMode Mode);
				static void SetMaxStalenessSeconds(TReadPreferences* ReadPreferences, uint64_t MaxStalenessSeconds);
				static bool IsValid(TReadPreferences* ReadPreferences);
				static ReadMode GetMode(TReadPreferences* ReadPreferences);
				static uint64_t GetMaxStalenessSeconds(TReadPreferences* ReadPreferences);
			};

			class TH_OUT BulkOperation
			{
			public:
				static TBulkOperation* Create(bool IsOrdered);
				static void Release(TBulkOperation** Operation);
				static bool RemoveMany(TBulkOperation* Operation, BSON::TDocument** Selector, BSON::TDocument** Options);
				static bool RemoveOne(TBulkOperation* Operation, BSON::TDocument** Selector, BSON::TDocument** Options);
				static bool ReplaceOne(TBulkOperation* Operation, BSON::TDocument** Selector, BSON::TDocument** Replacement, BSON::TDocument** Options);
				static bool Insert(TBulkOperation* Operation, BSON::TDocument** Document, BSON::TDocument** Options);
				static bool UpdateOne(TBulkOperation* Operation, BSON::TDocument** Selector, BSON::TDocument** Document, BSON::TDocument** Options);
				static bool UpdateMany(TBulkOperation* Operation, BSON::TDocument** Selector, BSON::TDocument** Document, BSON::TDocument** Options);
				static bool RemoveMany(TBulkOperation* Operation, BSON::TDocument* Selector, BSON::TDocument* Options);
				static bool RemoveOne(TBulkOperation* Operation, BSON::TDocument* Selector, BSON::TDocument* Options);
				static bool ReplaceOne(TBulkOperation* Operation, BSON::TDocument* Selector, BSON::TDocument* Replacement, BSON::TDocument* Options);
				static bool Insert(TBulkOperation* Operation, BSON::TDocument* Document, BSON::TDocument* Options);
				static bool UpdateOne(TBulkOperation* Operation, BSON::TDocument* Selector, BSON::TDocument* Document, BSON::TDocument* Options);
				static bool UpdateMany(TBulkOperation* Operation, BSON::TDocument* Selector, BSON::TDocument* Document, BSON::TDocument* Options);
				static bool Execute(TBulkOperation** Operation, BSON::TDocument** Reply);
				static bool Execute(TBulkOperation** Operation);
				static uint64_t GetHint(TBulkOperation* Operation);
				static TWriteConcern* GetWriteConcern(TBulkOperation* Operation);
			};

			class TH_OUT ChangeStream
			{
			public:
				static TChangeStream* Create(TConnection* Connection, BSON::TDocument** Pipeline, BSON::TDocument** Options);
				static TChangeStream* Create(TDatabase* Database, BSON::TDocument** Pipeline, BSON::TDocument** Options);
				static TChangeStream* Create(TCollection* Collection, BSON::TDocument** Pipeline, BSON::TDocument** Options);
				static void Release(TChangeStream** Stream);
				static bool Next(TChangeStream* Stream, BSON::TDocument** Result);
				static bool Error(TChangeStream* Stream, BSON::TDocument** Result);
			};

			class TH_OUT Cursor
			{
			public:
				static void Release(TCursor** Cursor);
				static void SetMaxAwaitTime(TCursor* Cursor, uint64_t MaxAwaitTime);
				static void SetBatchSize(TCursor* Cursor, uint64_t BatchSize);
				static void Receive(TCursor* Cursor, void* Context, bool(* Next)(TCursor*, BSON::TDocument*, void*));
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
				static HostList GetHosts(TCursor* Cursor);
				static BSON::TDocument* GetCurrent(TCursor* Cursor);
			};

			class TH_OUT Collection
			{
			public:
				static void Release(TCollection** Collection);
				static void SetReadConcern(TCollection* Collection, TReadConcern* Concern);
				static void SetWriteConcern(TCollection* Collection, TWriteConcern* Concern);
				static void SetReadPreferences(TCollection* Collection, TReadPreferences* Preferences);
				static bool UpdateDocument(TCollection* Collection, UpdateMode Flags, BSON::TDocument** Selector, BSON::TDocument** Update, TWriteConcern* Concern);
				static bool UpdateMany(TCollection* Collection, BSON::TDocument** Selector, BSON::TDocument** Update, BSON::TDocument** Options, BSON::TDocument** Reply);
				static bool UpdateOne(TCollection* Collection, BSON::TDocument** Selector, BSON::TDocument** Update, BSON::TDocument** Options, BSON::TDocument** Reply);
				static bool Rename(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName);
				static bool RenameWithOptions(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName, BSON::TDocument** Options);
				static bool RenameWithRemove(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName);
				static bool RenameWithOptionsAndRemove(TCollection* Collection, const char* NewDatabaseName, const char* NewCollectionName, BSON::TDocument** Options);
				static bool Remove(TCollection* Collection, BSON::TDocument** Options = nullptr);
				static bool RemoveDocument(TCollection* Collection, RemoveMode Flags, BSON::TDocument** Selector, TWriteConcern* Concern);
				static bool RemoveMany(TCollection* Collection, BSON::TDocument** Selector, BSON::TDocument** Options, BSON::TDocument** Reply);
				static bool RemoveOne(TCollection* Collection, BSON::TDocument** Selector, BSON::TDocument** Options, BSON::TDocument** Reply);
				static bool RemoveIndex(TCollection* Collection, const char* Name, BSON::TDocument** Options = nullptr);
				static bool ReplaceOne(TCollection* Collection, BSON::TDocument** Selector, BSON::TDocument** Replacement, BSON::TDocument** Options, BSON::TDocument** Reply);
				static bool InsertDocument(TCollection* Collection, InsertMode Flags, BSON::TDocument** Document, TWriteConcern* Concern);
				static bool InsertMany(TCollection* Collection, BSON::TDocument** Documents, uint64_t Length, BSON::TDocument** Options, BSON::TDocument** Reply);
				static bool InsertMany(TCollection* Collection, const std::vector<BSON::TDocument*>& Documents, BSON::TDocument** Options, BSON::TDocument** Reply);
				static bool InsertOne(TCollection* Collection, BSON::TDocument** Document, BSON::TDocument** Options, BSON::TDocument** Reply);
				static bool ExecuteCommandWithReply(TCollection* Collection, BSON::TDocument** Command, BSON::TDocument** Reply, TReadPreferences* Preferences = nullptr);
				static bool ExecuteCommandWithOptions(TCollection* Collection, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences = nullptr);
				static bool ExecuteReadCommandWithOptions(TCollection* Collection, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences = nullptr);
				static bool ExecuteReadWriteCommandWithOptions(TCollection* Collection, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply);
				static bool ExecuteWriteCommandWithOptions(TCollection* Collection, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply);
				static bool FindAndModify(TCollection* Collection, BSON::TDocument** Query, BSON::TDocument** Sort, BSON::TDocument** Update, BSON::TDocument** Fields, BSON::TDocument** Reply, bool Remove, bool Upsert, bool New);
				static bool FindAndModifyWithOptions(TCollection* Collection, BSON::TDocument** Query, BSON::TDocument** Reply, TFindAndModifyOptions** Options);
				static uint64_t CountElementsInArray(TCollection* Collection, BSON::TDocument** Match, BSON::TDocument** Filter, BSON::TDocument** Options, TReadPreferences* Preferences);
				static uint64_t CountDocuments(TCollection* Collection, BSON::TDocument** Filter, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences);
				static uint64_t CountDocumentsEstimated(TCollection* Collection, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences);
				static std::string StringifyKeyIndexes(TCollection* Collection, BSON::TDocument** Keys);
				static const char* GetName(TCollection* Collection);
				static TReadConcern* GetReadConcern(TCollection* Collection);
				static TReadPreferences* GetReadPreferences(TCollection* Collection);
				static TWriteConcern* GetWriteConcern(TCollection* Collection);
				static TCursor* FindIndexes(TCollection* Collection, BSON::TDocument** Options);
				static TCursor* FindMany(TCollection* Collection, BSON::TDocument** Filter, BSON::TDocument** Options, TReadPreferences* Preferences);
				static TCursor* FindOne(TCollection* Collection, BSON::TDocument** Filter, BSON::TDocument** Options, TReadPreferences* Preferences);
				static TCursor* Aggregate(TCollection* Collection, QueryMode Flags, BSON::TDocument** Pipeline, BSON::TDocument** Options, TReadPreferences* Preferences);
				static TCursor* ExecuteCommand(TCollection* Collection, QueryMode Flags, uint64_t Skip, uint64_t Limit, uint64_t BatchSize, BSON::TDocument** Command, BSON::TDocument** Fields, TReadPreferences* Preferences = nullptr);
				static TBulkOperation* CreateBulkOperation(TCollection* Collection, BSON::TDocument** Options);
			};

			class TH_OUT Database
			{
			public:
				static void Release(TDatabase** Database);
				static void SetReadConcern(TDatabase* Database, TReadConcern* Concern);
				static void SetWriteConcern(TDatabase* Database, TWriteConcern* Concern);
				static void SetReadPreferences(TDatabase* Database, TReadPreferences* Preferences);
				static bool HasCollection(TDatabase* Database, const char* Name);
				static bool RemoveAllUsers(TDatabase* Database);
				static bool RemoveUser(TDatabase* Database, const char* Name);
				static bool Remove(TDatabase* Database);
				static bool RemoveWithOptions(TDatabase* Database, BSON::TDocument** Options);
				static bool AddUser(TDatabase* Database, const char* Username, const char* Password, BSON::TDocument** Roles, BSON::TDocument** Custom);
				static bool ExecuteCommandWithReply(TDatabase* Database, BSON::TDocument** Command, BSON::TDocument** Reply, TReadPreferences* Preferences = nullptr);
				static bool ExecuteCommandWithOptions(TDatabase* Database, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences = nullptr);
				static bool ExecuteReadCommandWithOptions(TDatabase* Database, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply, TReadPreferences* Preferences = nullptr);
				static bool ExecuteReadWriteCommandWithOptions(TDatabase* Database, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply);
				static bool ExecuteWriteCommandWithOptions(TDatabase* Database, BSON::TDocument** Command, BSON::TDocument** Options, BSON::TDocument** Reply);
				static std::vector<std::string> GetCollectionNames(TDatabase* Database, BSON::TDocument** Options);
				static const char* GetName(TDatabase* Database);
				static TCursor* ExecuteCommand(TDatabase* Database, QueryMode Flags, uint64_t Skip, uint64_t Limit, uint64_t BatchSize, BSON::TDocument** Command, BSON::TDocument** Fields, TReadPreferences* Preferences = nullptr);
				static TCursor* FindCollections(TDatabase* Database, BSON::TDocument** Options);
				static TCollection* CreateCollection(TDatabase* Database, const char* Name, BSON::TDocument** Options);
				static TCollection* GetCollection(TDatabase* Database, const char* Name);
				static TReadConcern* GetReadConcern(TDatabase* Database);
				static TReadPreferences* GetReadPreferences(TDatabase* Database);
				static TWriteConcern* GetWriteConcern(TDatabase* Database);
			};

			class TH_OUT ServerDescription
			{
			public:
				static void Release(TServerDescription** Description);
				static uint64_t GetId(TServerDescription* Description);
				static int64_t GetTripTime(TServerDescription* Description);
				static const char* GetType(TServerDescription* Description);
				static BSON::TDocument* IsMaster(TServerDescription* Description);
				static HostList* GetHosts(TServerDescription* Description);
			};

			class TH_OUT TopologyDescription
			{
			public:
				static bool HasReadableServer(TTopologyDescription* Description, TReadPreferences* Preferences);
				static bool HasWritableServer(TTopologyDescription* Description);
				static std::vector<TServerDescription*> GetServers(TTopologyDescription* Description);
				static const char* GetType(TTopologyDescription* Description);
			};

			class TH_OUT Client : public Rest::Object
			{
				friend ClientPool;

			private:
				TConnection* Connection = nullptr;
				ClientPool* Master = nullptr;
				bool Connected = false;

			public:
				APMCallbacks ApmCallbacks;

			public:
				Client();
				virtual ~Client() override;
				bool Connect(const char* Address);
				bool ConnectWithURI(TURI* URI);
				bool Disconnect();
				bool SelectServer(bool ForWrites);
				void SetAppName(const char* Name);
				void SetReadConcern(TReadConcern* Concern);
				void SetReadPreferences(TReadPreferences* Preferences);
				void SetWriteConcern(TWriteConcern* Concern);
				void SetSSLOptions(const SSLOptions& Options);
				void SetAPMCallbacks(const APMCallbacks& Callbacks);
				TCursor* ExecuteCommand(const char* DatabaseName, BSON::TDocument** Query, TReadPreferences* Preferences = nullptr);
				BSON::TDocument* ExecuteCommandResult(const char* DatabaseName, BSON::TDocument** Query, TReadPreferences* Preferences = nullptr);
				BSON::TDocument* ExecuteCommandResult(const char* DatabaseName, BSON::TDocument** Query, BSON::TDocument** Options, TReadPreferences* Preferences = nullptr);
				BSON::TDocument* ExecuteReadCommandResult(const char* DatabaseName, BSON::TDocument** Query, BSON::TDocument** Options, TReadPreferences* Preferences = nullptr);
				BSON::TDocument* ExecuteWriteCommandResult(const char* DatabaseName, BSON::TDocument** Query, BSON::TDocument** Options);
				TServerDescription* GetServerDescription(unsigned int ServerId);
				TCursor* FindDatabases(BSON::TDocument** Options);
				TDatabase* GetDatabase(const char* Name);
				TDatabase* GetDefaultDatabase();
				TCollection* GetCollection(const char* DatabaseName, const char* Name);
				TReadConcern* GetReadConcern();
				TReadPreferences* GetReadPreferences();
				TWriteConcern* GetWriteConcern();
				TConnection* Get();
				TURI* GetURI();
				ClientPool* GetMaster();
				std::vector<std::string> GetDatabaseNames(BSON::TDocument** Options);
				bool IsConnected();
			};

			class TH_OUT ClientPool : public Rest::Object
			{
			private:
				TConnectionPool* Clients = nullptr;
				TURI* Address = nullptr;
				bool Connected = false;

			public:
				APMCallbacks ApmCallbacks;

			public:
				ClientPool();
				virtual ~ClientPool() override;
				bool Connect(const char* Address);
				bool ConnectWithURI(TURI* URI);
				bool Disconnect();
				void Push(Client** Client);
				void SetAppName(const char* Name);
				void SetSSLOptions(const SSLOptions& Options);
				void SetAPMCallbacks(const APMCallbacks& Callbacks);
				Client* Pop();
				TConnectionPool* Get();
				TURI* GetURI();
			};

			inline FindAndModifyMode operator |(FindAndModifyMode A, FindAndModifyMode B)
			{
				return static_cast<FindAndModifyMode>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
			}
		}
	}
}
#endif