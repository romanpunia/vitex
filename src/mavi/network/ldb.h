#ifndef VI_NETWORK_SQLITE_H
#define VI_NETWORK_SQLITE_H
#include "../core/network.h"

struct sqlite3;
struct sqlite3_stmt;
struct sqlite3_context;
struct sqlite3_value;

namespace Mavi
{
	namespace Network
	{
		namespace LDB
		{
			class Row;

			class Response;

			class Cursor;

			class Cluster;

			class Driver;

			typedef std::function<Core::Variant(const Core::VariantList&)> OnFunctionResult;
			typedef std::function<void(const Core::String&)> OnQueryLog;
			typedef std::function<void(Cursor&)> OnResult;
			typedef sqlite3 TConnection;
			typedef sqlite3_stmt TStatement;
			typedef sqlite3_context TContext;
			typedef sqlite3_value TValue;
			typedef TConnection* SessionId;

			enum class Isolation
			{
				Deferred,
				Immediate,
				Exclusive,
				Default = Deferred
			};

			enum class QueryOp
			{
				DeleteArgs = 0,
				ReuseArgs = (1 << 0),
				TransactionStart = (1 << 1),
				TransactionEnd = (1 << 2)
			};

			enum class CheckpointMode
			{
				Passive = 0,
				Full = 1,
				Restart = 2,
				Truncate = 3
			};

			inline size_t operator |(QueryOp A, QueryOp B)
			{
				return static_cast<size_t>(static_cast<size_t>(A) | static_cast<size_t>(B));
			}
			
			struct VI_OUT Checkpoint
			{
				Core::String Database;
				uint32_t FramesSize = 0;
				uint32_t FramesCount = 0;
				int32_t Status = -1;
			};

			class VI_OUT Aggregate : public Core::Reference<Aggregate>
			{
			public:
				virtual ~Aggregate() = default;
				virtual void Step(const Core::VariantList& Args) = 0;
				virtual Core::Variant Finalize() = 0;
			};

			class VI_OUT Window : public Core::Reference<Window>
			{
			public:
				virtual ~Window() = default;
				virtual void Step(const Core::VariantList& Args) = 0;
				virtual void Inverse(const Core::VariantList& Args) = 0;
				virtual Core::Variant Value() = 0;
				virtual Core::Variant Finalize() = 0;
			};

			class VI_OUT Column
			{
				friend Row;

			private:
				Response* Base;
				size_t RowIndex;
				size_t ColumnIndex;

			private:
				Column(const Response* NewBase, size_t fRowIndex, size_t fColumnIndex);

			public:
				Core::String GetName() const;
				Core::Variant Get() const;
				Core::Schema* GetInline() const;
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
				Response* Base;
				size_t RowIndex;

			private:
				Row(const Response* NewBase, size_t fRowIndex);

			public:
				Core::Unique<Core::Schema> GetObject() const;
				Core::Unique<Core::Schema> GetArray() const;
				size_t Index() const;
				size_t Size() const;
				Column GetColumn(size_t Index) const;
				Column GetColumn(const char* Name) const;
				Column GetColumnByName(const Core::String& Name) const;
				bool GetColumns(Column* Output, size_t Size) const;
				bool Exists() const
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

			class VI_OUT Response
			{
				friend Cluster;
				friend Row;
				friend Column;

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
				struct
				{
					uint64_t AffectedRows = 0;
					uint64_t LastInsertedRowId = 0;
				} Stats;

			private:
				Core::Vector<Core::String> Columns;
				Core::Vector<Core::Vector<Core::Variant>> Values;
				Core::String StatusMessage;
				int StatusCode = -1;

			public:
				Response() = default;
				Response(const Response& Other) = default;
				Response(Response&& Other) = default;
				~Response() = default;
				Response& operator =(const Response& Other) = default;
				Response& operator =(Response&& Other) = default;
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
				Core::String GetStatusText() const;
				int GetStatusCode() const;
				size_t GetColumnIndex(const char* Name) const;
				size_t AffectedRows() const;
				size_t LastInsertedRowId() const;
				size_t Size() const;
				Row GetRow(size_t Index) const;
				Row Front() const;
				Row Back() const;
				Response Copy() const;
				bool Empty() const;
				bool Error() const;
				bool ErrorOrEmpty() const
				{
					return Error() || Empty();
				}
				bool Exists() const
				{
					return StatusCode == 0;
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
				TConnection* Executor;

			public:
				Cursor(TConnection* Connection);
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
				Column GetColumn(const Core::String& Name) const
				{
					return First().Front().GetColumn(Name.c_str());
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
				TConnection* GetExecutor() const;
				const Response& First() const;
				const Response& Last() const;
				const Response& At(size_t Index) const;
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

			class VI_OUT_TS Cluster final : public Core::Reference<Cluster>
			{
				friend Driver;

			private:
				struct Request
				{
					Core::Promise<TConnection*> Future;
					SessionId Session = nullptr;
					size_t Opts = 0;
				};

			private:
				Core::UnorderedMap<TConnection*, Core::SingleQueue<Request>> Queues;
				Core::UnorderedSet<TConnection*> Idle;
				Core::UnorderedSet<TConnection*> Busy;
				Core::Vector<OnFunctionResult*> Functions;
				Core::Vector<Aggregate*> Aggregates;
				Core::Vector<Window*> Windows;
				Core::String Source;
				Driver* LibraryHandle;
				std::mutex Update;

			public:
				Cluster();
				~Cluster() noexcept;
				void SetWalAutocheckpoint(uint32_t MaxFrames);
				void SetSoftHeapLimit(uint64_t Memory);
				void SetHardHeapLimit(uint64_t Memory);
				void SetSharedCache(bool Enabled);
				void SetExtensions(bool Enabled);
				void SetFunction(const Core::String& Name, uint8_t Args, OnFunctionResult&& Context);
				void SetAggregateFunction(const Core::String& Name, uint8_t Args, Core::Unique<Aggregate> Context);
				void SetWindowFunction(const Core::String& Name, uint8_t Args, Core::Unique<Window> Context);
				void OverloadFunction(const Core::String& Name, uint8_t Args);
				Core::Vector<Checkpoint> WalCheckpoint(CheckpointMode Mode, const Core::String& Database = Core::String());
				size_t FreeMemoryUsed(size_t Bytes);
				size_t GetMemoryUsed() const;
				Core::Promise<SessionId> TxBegin(Isolation Type);
				Core::Promise<SessionId> TxStart(const Core::String& Command);
				Core::Promise<bool> TxEnd(const Core::String& Command, SessionId Session);
				Core::Promise<bool> TxCommit(SessionId Session);
				Core::Promise<bool> TxRollback(SessionId Session);
				Core::Promise<bool> Connect(const Core::String& URI, size_t Connections);
				Core::Promise<bool> Disconnect();
				Core::Promise<bool> Flush();
				Core::Promise<Cursor> EmplaceQuery(const Core::String& Command, Core::SchemaList* Map, size_t QueryOps = 0, SessionId Session = nullptr);
				Core::Promise<Cursor> TemplateQuery(const Core::String& Name, Core::SchemaArgs* Map, size_t QueryOps = 0, SessionId Session = nullptr);
				Core::Promise<Cursor> Query(const Core::String& Command, size_t QueryOps = 0, SessionId Session = nullptr);
				TConnection* GetIdleConnection();
				TConnection* GetBusyConnection();
				TConnection* GetAnyConnection();
				bool IsConnected();

			private:
				TConnection* TryAcquireConnection(SessionId Session, size_t Opts);
				Core::Promise<TConnection*> AcquireConnection(SessionId Session, size_t Opts);
				void ReleaseConnection(TConnection* Connection, size_t Opts);
				bool Consume(TStatement* Statement, Response* Result);
			};

			class VI_OUT_TS Utils
			{
			public:
				static Core::String InlineArray(Core::Unique<Core::Schema> Array);
				static Core::String InlineQuery(Core::Unique<Core::Schema> Where, const Core::UnorderedMap<Core::String, Core::String>& Whitelist, const Core::String& Default = "TRUE");
				static Core::String GetCharArray(const Core::String& Src) noexcept;
				static Core::String GetByteArray(const Core::String& Src) noexcept;
				static Core::String GetByteArray(const char* Src, size_t Size) noexcept;
				static Core::String GetSQL(Core::Schema* Source, bool Escape, bool Negate) noexcept;
				static Core::Schema* GetSchemaFromValue(const Core::Variant& Value) noexcept;
				static Core::VariantList ContextArgs(TValue** Values, size_t ValuesCount) noexcept;
				static void ContextReturn(TContext* Context, const Core::Variant& Result) noexcept;
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
				void LogQuery(const Core::String& Command) noexcept;
				bool AddConstant(const Core::String& Name, const Core::String& Value) noexcept;
				bool AddQuery(const Core::String& Name, const Core::String& Data) noexcept;
				bool AddQuery(const Core::String& Name, const char* Buffer, size_t Size) noexcept;
				bool AddDirectory(const Core::String& Directory, const Core::String& Origin = "") noexcept;
				bool RemoveConstant(const Core::String& Name) noexcept;
				bool RemoveQuery(const Core::String& Name) noexcept;
				bool LoadCacheDump(Core::Schema* Dump) noexcept;
				Core::Schema* GetCacheDump() noexcept;
				Core::String Emplace(const Core::String& SQL, Core::SchemaList* Map, bool Once = true) noexcept;
				Core::String GetQuery(const Core::String& Name, Core::SchemaArgs* Map, bool Once = true) noexcept;
				Core::Vector<Core::String> GetQueries() noexcept;
			};
		}
	}
}
#endif