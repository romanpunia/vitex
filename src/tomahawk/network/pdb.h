#ifndef TH_NETWORK_POSTGRESQL_H
#define TH_NETWORK_POSTGRESQL_H
#include "../core/network.h"

struct pg_conn;
struct pg_result;
struct pgNotify;

namespace Tomahawk
{
	namespace Network
	{
		namespace PDB
		{
			class Notify;

			class Row;

			class Request;

			class Response;

			class Cluster;

			class Driver;

			class Connection;

			typedef std::function<void(Notify)> OnNotification;
			typedef pg_conn TConnection;
			typedef pg_result TResponse;
			typedef pgNotify TNotify;
			typedef uint64_t ObjectId;

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

			class TH_OUT Util
			{
			public:
				static std::string InlineArray(Cluster* Client, Core::Document* Array);
				static std::string InlineQuery(Cluster* Client, Core::Document* Where);
			};

			class TH_OUT Address
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
				const char** CreateKeys() const;
				const char** CreateValues() const;

			private:
				static std::string GetKeyName(AddressOp Key);
			};

			class TH_OUT Notify
			{
			private:
				TNotify* Base;

			public:
				Notify(TNotify* NewBase);
				void Release();
				Core::Document* GetDocument() const;
				std::string GetName() const;
				std::string GetData() const;
				int GetPid() const;
				TNotify* Get() const;
				operator bool() const
				{
					return Base != nullptr;
				}
			};

			class TH_OUT Column
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
				Core::Document* GetInline() const;
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
				operator bool() const
				{
					return Base != nullptr;
				}
			};

			class TH_OUT Row
			{
				friend Column;
				friend Response;

			private:
				TResponse* Base;
				size_t RowIndex;

			private:
				Row(TResponse* NewBase, size_t fRowIndex);

			public:
				Core::Document* GetObject() const;
				size_t GetIndex() const;
				size_t GetSize() const;
				Response GetCursor() const;
				Column GetColumn(size_t Index) const;
				Column GetColumn(const char* Name) const;
				bool GetColumns(Column* Output, size_t Size) const;
				operator bool() const
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

			class TH_OUT Response
			{
			private:
				TResponse* Base;

			public:
				Response();
				Response(TResponse* NewBase);
				void Release();
				Core::Document* GetArray() const;
				Core::Document* GetObject(size_t Index = 0) const;
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
				TResponse* Get() const;
				bool IsEmpty() const;
				bool IsError() const;
				bool IsErrorOrEmpty() const
				{
					return IsError() || IsEmpty();
				}
				operator bool() const
				{
					return Base != nullptr;
				}
				Row operator [](size_t Index)
				{
					return GetRow(Index);
				}
				Row operator [](size_t Index) const
				{
					return GetRow(Index);
				}
			};

			class TH_OUT Cursor
			{
				friend Cluster;

			private:
				std::vector<Response> Base;

			public:
				Cursor();
				void Release();
				bool OK();
				bool IsEmpty() const;
				bool IsError() const;
				bool IsErrorOrEmpty() const
				{
					return IsError() || IsEmpty();
				}
				size_t Size() const;
				Response First() const;
				Response Last() const;
				Response GetResponse(size_t Index) const;
				Column operator [](const char* Name)
				{
					return First().Front().GetColumn(Name);
				}
				Column operator [](const char* Name) const
				{
					return First().Front().GetColumn(Name);
				}
				operator bool() const
				{
					return !Base.empty();
				}
			};

			class TH_OUT Connection
			{
				friend Cluster;

			private:
				TConnection* Base;
				Socket* Stream;
				Request* Current;
				uint64_t Session;
				QueryState State;

			public:
				TConnection* GetBase() const;
				Socket* GetStream() const;
				Request* GetCurrent() const;
				QueryState GetState() const;
				bool IsBusy() const;
			};

			class TH_OUT Request
			{
				friend Cluster;

			private:
				Core::Async<Cursor> Future;
				std::string Command;
				uint64_t Session = 0;
				Cursor Result;

			public:
				std::string GetCommand() const;
				Cursor GetResult() const;
				uint64_t GetSession() const;
				bool IsPending() const;
			};

			class TH_OUT Cluster : public Core::Object
			{
				friend Driver;

			private:
				std::unordered_map<std::string, OnNotification> Listeners;
				std::unordered_map<Socket*, Connection*> Pool;
				std::vector<Request*> Requests;
				std::atomic<uint64_t> Session;
				std::mutex Update;
				Address Source;

			public:
				Cluster();
				virtual ~Cluster() override;
				void SetChannel(const std::string& Name, const OnNotification& NewCallback);
				Core::Async<uint64_t> TxBegin(const std::string& Command);
				Core::Async<bool> TxEnd(const std::string& Command, uint64_t Token);
				Core::Async<bool> Connect(const Address& URI, size_t Connections);
				Core::Async<bool> Disconnect();
				Core::Async<Cursor> EmplaceQuery(const std::string& Command, Core::DocumentList* Map, bool Once = true, uint64_t Token = 0);
				Core::Async<Cursor> TemplateQuery(const std::string& Name, Core::DocumentArgs* Map, bool Once = true, uint64_t Token = 0);
				Core::Async<Cursor> Query(const std::string& Command, uint64_t Token = 0);
				TConnection* GetConnection(QueryState State);
				TConnection* GetConnection() const;
				bool IsConnected() const;

			private:
				void Reestablish(Connection* Base);
				void Commit(uint64_t Token);
				bool Consume(Connection* Base);
				bool Reprocess(Connection* Base);
				bool Transact(Connection* Base, Request* Token);
			};

			class TH_OUT Driver
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
					std::string Cache;
				};

			private:
				static std::unordered_map<std::string, Sequence>* Queries;
				static std::mutex* Safe;
				static std::atomic<bool> Active;
				static std::atomic<int> State;

			public:
				static void Create();
				static void Release();
				static bool AddQuery(const std::string& Name, const char* Buffer, size_t Size);
				static bool AddDirectory(const std::string& Directory, const std::string& Origin = "");
				static bool RemoveQuery(const std::string& Name);
				static std::string Emplace(Cluster* Base, const std::string& SQL, Core::DocumentList* Map, bool Once = true);
				static std::string GetQuery(Cluster* Base, const std::string& Name, Core::DocumentArgs* Map, bool Once = true);
				static std::vector<std::string> GetQueries();

			private:
				static std::string GetCharArray(TConnection* Base, const std::string& Src);
				static std::string GetByteArray(TConnection* Base, const char* Src, size_t Size);
				static std::string GetSQL(TConnection* Base, Core::Document* Source, bool Escape);
			};
		}
	}
}
#endif
