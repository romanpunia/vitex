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
			
			class Result;

			class Connection;

			class Queue;

			class Driver;

			typedef std::function<void(Notify)> OnNotification;
			typedef pg_conn TConnection;
			typedef pg_result TResult;
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
				std::string GetName() const;
				std::string GetExtra() const;
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
				TResult* Base;
				size_t RowIndex;
				size_t ColumnIndex;

			private:
				Column(TResult* NewBase, size_t fRowIndex, size_t fColumnIndex);

			public:
				int SetValueText(char* Data, size_t Size);
				std::string GetName() const;
				std::string GetValueText() const;
				bool GetBool() const;
				short GetShort() const;
				unsigned short GetUShort() const;
				long GetLong() const;
				unsigned long GetULong() const;
				int32_t GetInt32() const;
				uint32_t GetUInt32() const;
				int64_t GetInt64() const;
				uint64_t GetUInt64() const;
				Core::Decimal GetDecimal() const;
				float GetFloat() const;
				double GetDouble() const;
				char GetChar() const;
				unsigned char* GetBlob(size_t* Size) const;
				char* GetValueData() const;
				int GetFormatId() const;
				int GetModId() const;
				int GetTableIndex() const;
				ObjectId GetTableId() const;
				ObjectId GetTypeId() const;
				size_t GetIndex() const;
				size_t GetSize() const;
				size_t GetValueSize() const;
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
				friend Result;

			private:
				TResult* Base;
				size_t RowIndex;

			private:
				Row(TResult* NewBase, size_t fRowIndex);

			public:
				size_t GetIndex() const;
				size_t GetSize() const;
				Result GetResult() const;
				Column GetColumn(size_t Index) const;
				bool GetColumns(Column* Output, size_t Size) const;
				operator bool() const
				{
					return Base != nullptr;
				}
				Column operator [](size_t Index)
				{
					return GetColumn(Index);
				}
				Column operator [](size_t Index) const
				{
					return GetColumn(Index);
				}
			};

			class TH_OUT Result
			{
			private:
				TResult* Base;

			public:
				Result();
				Result(TResult* NewBase);
				void Release();
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
				TResult* Get() const;
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

			class TH_OUT Results
			{
				friend Connection;
				friend Driver;

			private:
				std::vector<Result> Data;
				Connection* Source;

			private:
				Results(Connection* NewSource);

			public:
				Results();
				void Release();
				size_t GetSize() const;
				Result& GetResult(size_t Index);
				const Result& GetResult(size_t Index) const;
				Connection* GetSource() const;
				operator bool() const
				{
					return Source != nullptr && !Data.empty();
				}
				Result operator [](size_t Index)
				{
					return GetResult(Index);
				}
				Result operator [](size_t Index) const
				{
					return GetResult(Index);
				}

			private:
				void Push(TResult* NewResult);
				void Swap(Results&& Other);
			};

			class TH_OUT Connection : public Core::Object
			{
				friend Queue;
				friend Driver;

			private:
				Core::Async<Results> Income;
				std::atomic<bool> Acquired;
				std::atomic<bool> Connected;
				std::mutex Safe;
				OnNotification Callback;
				TConnection* Base;
				Queue* Master;
				Results Operation;

			public:
				Connection();
				virtual ~Connection() override;
				void SetNotificationCallback(const OnNotification& NewCallback);
				int SetEncoding(const std::string& Name);
				Core::Async<bool> Connect(const Address& URI);
				Core::Async<bool> Disconnect();
				Core::Async<Results> QuerySet(const std::string& Command);
				Core::Async<Result> Query(const std::string& Command);
				Core::Async<Result> Subscribe(const std::string& Channel);
				Core::Async<Result> Unsubscribe(const std::string& Channel);
				bool CancelQuery();
				std::string GetEncoding() const;
				std::string GetErrorMessage() const;
				std::string EscapeLiteral(const char* Data, size_t Size);
				std::string EscapeLiteral(const std::string& Value);
				std::string EscapeIdentifier(const char* Data, size_t Size);
				std::string EscapeIdentifier(const std::string& Value);
				unsigned char* EscapeBytea(const unsigned char* Data, size_t Size, size_t* OutSize);
				unsigned char* UnescapeBytea(const unsigned char* Data, size_t *OutSize);
				int GetServerVersion() const;
				int GetBackendPid() const;
				ConnectionState GetStatus() const;
				TransactionState GetTransactionState() const;
				TConnection* Get() const;
				bool IsConnected() const;
			};

			class TH_OUT Queue : public Core::Object
			{
				friend Connection;

			private:
				std::unordered_set<Connection*> Active;
				std::unordered_set<Connection*> Inactive;
				std::atomic<bool> Connected;
				std::mutex Safe;
				Address Source;

			public:
				Queue();
				virtual ~Queue() override;
				Core::Async<bool> Connect(const Address& URI);
				Core::Async<bool> Disconnect();
				bool Push(Connection** Client);
				Core::Async<Connection*> Pop();

			private:
				void Clear(Connection* Client);
			};

			class TH_OUT Driver
			{
			private:
				struct Pose
				{
					std::string Key;
					size_t Offset;
					bool Escape;
				};

				struct Sequence
				{
					std::vector<Pose> Positions;
					std::string Request;
					std::string Cache;
				};

			private:
				static std::unordered_map<std::string, Sequence>* Queries;
				static std::unordered_set<Connection*>* Listeners;
				static std::mutex* Safe;
				static std::atomic<bool> Active;
				static std::atomic<int> State;

			public:
				static void Create();
				static void Release();
				static void Assign(Core::Schedule* Queue);
				static int Dispatch();
				static bool Listen(Connection* Value);
				static bool Unlisten(Connection* Value);
				static bool AddQuery(const std::string& Name, const char* Buffer, size_t Size);
				static bool AddDirectory(const std::string& Directory, const std::string& Origin = "");
				static bool RemoveQuery(const std::string& Name);
				static std::string GetQuery(Connection* Base, const std::string& Name, Core::DocumentArgs* Map, bool Once = true);
				static std::vector<std::string> GetQueries();

			private:
				static void Resolve();
				static std::string GetCharArray(Connection* Base, const std::string& Src);
				static std::string GetByteArray(Connection* Base, const char* Src, size_t Size);
				static std::string GetSQL(Connection* Base, Core::Document* Source, bool Escape);
			};
		}
	}
}
#endif