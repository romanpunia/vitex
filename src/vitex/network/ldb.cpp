#include "ldb.h"
#ifdef VI_SQLITE
#include <sqlite3.h>
#endif

namespace Vitex
{
	namespace Network
	{
		namespace LDB
		{
#ifdef VI_SQLITE
			static void sqlite3_function_call(sqlite3_context* Context, int ArgsCount, sqlite3_value** Args)
			{
				OnFunctionResult* Callable = (OnFunctionResult*)sqlite3_user_data(Context);
				VI_ASSERT(Callable != nullptr, "callable is null");
				Utils::ContextReturn(Context, (*Callable)(Utils::ContextArgs(Args, (size_t)ArgsCount)));
			}
			static void sqlite3_aggregate_step_call(sqlite3_context* Context, int ArgsCount, sqlite3_value** Args)
			{
				Aggregate* Callable = (Aggregate*)sqlite3_user_data(Context);
				VI_ASSERT(Callable != nullptr, "callable is null");
				Callable->Step(Utils::ContextArgs(Args, (size_t)ArgsCount));
			}
			static void sqlite3_aggregate_finalize_call(sqlite3_context* Context)
			{
				Aggregate* Callable = (Aggregate*)sqlite3_user_data(Context);
				VI_ASSERT(Callable != nullptr, "callable is null");
				Utils::ContextReturn(Context, Callable->Finalize());
			}
			static void sqlite3_window_step_call(sqlite3_context* Context, int ArgsCount, sqlite3_value** Args)
			{
				Window* Callable = (Window*)sqlite3_user_data(Context);
				VI_ASSERT(Callable != nullptr, "callable is null");
				Callable->Step(Utils::ContextArgs(Args, (size_t)ArgsCount));
			}
			static void sqlite3_window_finalize_call(sqlite3_context* Context)
			{
				Window* Callable = (Window*)sqlite3_user_data(Context);
				VI_ASSERT(Callable != nullptr, "callable is null");
				Utils::ContextReturn(Context, Callable->Finalize());
			}
			static void sqlite3_window_inverse_call(sqlite3_context* Context, int ArgsCount, sqlite3_value** Args)
			{
				Window* Callable = (Window*)sqlite3_user_data(Context);
				VI_ASSERT(Callable != nullptr, "callable is null");
				Callable->Inverse(Utils::ContextArgs(Args, (size_t)ArgsCount));
			}
			static void sqlite3_window_value_call(sqlite3_context* Context)
			{
				Window* Callable = (Window*)sqlite3_user_data(Context);
				VI_ASSERT(Callable != nullptr, "callable is null");
				Utils::ContextReturn(Context, Callable->Value());
			}
#endif
			DatabaseException::DatabaseException(TConnection* Connection)
			{
#ifdef VI_SQLITE
				VI_ASSERT(Connection != nullptr, "base should be set");
				const char* NewMessage = sqlite3_errmsg(Connection);
				if (NewMessage && NewMessage[0] != '\0')
				{
					VI_DEBUG("[sqlite] %s", NewMessage);
					Message = NewMessage;
				}
				else
					Message = "internal driver error";
#else
				Message = "not supported";
#endif
			}
			DatabaseException::DatabaseException(Core::String&& NewMessage)
			{
				Message = std::move(NewMessage);
			}
			const char* DatabaseException::type() const noexcept
			{
				return "sqlite_error";
			}

			Column::Column(const Response* NewBase, size_t fRowIndex, size_t fColumnIndex) : Base((Response*)NewBase), RowIndex(fRowIndex), ColumnIndex(fColumnIndex)
			{
			}
			Core::String Column::GetName() const
			{
#ifdef VI_SQLITE
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), "column should be valid");
				return ColumnIndex < Base->Columns.size() ? Base->Columns[ColumnIndex] : Core::ToString(ColumnIndex);
#else
				return Core::String();
#endif
			}
			Core::Variant Column::Get() const
			{
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return Core::Var::Undefined();

				return Base->Values[RowIndex][ColumnIndex];
			}
			Core::Schema* Column::GetInline() const
			{
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				if (Nullable())
					return nullptr;

				auto& Column = Base->Values[RowIndex][ColumnIndex];
				return Utils::GetSchemaFromValue(Column);
			}
			size_t Column::Index() const
			{
				return ColumnIndex;
			}
			size_t Column::Size() const
			{
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return 0;

				return Base->Values[RowIndex].size();
			}
			size_t Column::RawSize() const
			{
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(ColumnIndex != std::numeric_limits<size_t>::max(), "column should be valid");
				return Base->Values[RowIndex][ColumnIndex].Size();
			}
			Row Column::GetRow() const
			{
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");
				return Row(Base, RowIndex);
			}
			bool Column::Nullable() const
			{
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || ColumnIndex == std::numeric_limits<size_t>::max())
					return true;

				auto& Column = Base->Values[RowIndex][ColumnIndex];
				return Column.Is(Core::VarType::Null) || Column.Is(Core::VarType::Undefined);
			}

			Row::Row(const Response* NewBase, size_t fRowIndex) : Base((Response*)NewBase), RowIndex(fRowIndex)
			{
			}
			Core::Schema* Row::GetObject() const
			{
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				auto& Row = Base->Values[RowIndex];
				Core::Schema* Result = Core::Var::Set::Object();
				Result->GetChilds().reserve(Row.size());
				for (size_t j = 0; j < Row.size(); j++)
					Result->Set(j < Base->Columns.size() ? Base->Columns[j] : Core::ToString(j), Utils::GetSchemaFromValue(Row[j]));

				return Result;
			}
			Core::Schema* Row::GetArray() const
			{
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return nullptr;

				auto& Row = Base->Values[RowIndex];
				Core::Schema* Result = Core::Var::Set::Array();
				Result->GetChilds().reserve(Row.size());
				for (auto& Column : Row)
					Result->Push(Utils::GetSchemaFromValue(Column));

				return Result;
			}
			size_t Row::Index() const
			{
				return RowIndex;
			}
			size_t Row::Size() const
			{
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return 0;

				if (RowIndex >= Base->Values.size())
					return 0;

				auto& Row = Base->Values[RowIndex];
				return Row.size();
			}
			Column Row::GetColumn(size_t Index) const
			{
				if (!Base || RowIndex == std::numeric_limits<size_t>::max() || Index >= Base->Columns.size())
					return Column(Base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				if (Index >= Base->Values[RowIndex].size())
					return Column(Base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				return Column(Base, RowIndex, Index);
			}
			Column Row::GetColumn(const std::string_view& Name) const
			{
				if (!Base || RowIndex == std::numeric_limits<size_t>::max())
					return Column(Base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				size_t Index = Base->GetColumnIndex(Name);
				if (Index >= Base->Values[RowIndex].size())
					return Column(Base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());
				
				return Column(Base, RowIndex, Index);
			}
			bool Row::GetColumns(Column* Output, size_t OutputSize) const
			{
				VI_ASSERT(Output != nullptr && OutputSize > 0, "output should be valid");
				VI_ASSERT(Base != nullptr, "context should be valid");
				VI_ASSERT(RowIndex != std::numeric_limits<size_t>::max(), "row should be valid");

				OutputSize = std::min(Size(), OutputSize);
				for (size_t i = 0; i < OutputSize; i++)
					Output[i] = Column(Base, RowIndex, i);

				return true;
			}

			Core::Schema* Response::GetArrayOfObjects() const
			{
				Core::Schema* Result = Core::Var::Set::Array();
				if (Values.empty() || Columns.empty())
					return Result;

				Result->Reserve(Values.size());
				for (auto& Row : Values)
				{
					Core::Schema* Subresult = Core::Var::Set::Object();
					Subresult->GetChilds().reserve(Row.size());
					for (size_t j = 0; j < Row.size(); j++)
						Subresult->Set(j < Columns.size() ? Columns[j] : Core::ToString(j), Utils::GetSchemaFromValue(Row[j]));
					Result->Push(Subresult);
				}

				return Result;
			}
			Core::Schema* Response::GetArrayOfArrays() const
			{
				Core::Schema* Result = Core::Var::Set::Array();
				if (Values.empty())
					return Result;

				Result->Reserve(Values.size());
				for (auto& Row : Values)
				{
					Core::Schema* Subresult = Core::Var::Set::Array();
					Subresult->GetChilds().reserve(Row.size());
					for (auto& Column : Row)
						Subresult->Push(Utils::GetSchemaFromValue(Column));
					Result->Push(Subresult);
				}

				return Result;
			}
			Core::Schema* Response::GetObject(size_t Index) const
			{
				return GetRow(Index).GetObject();
			}
			Core::Schema* Response::GetArray(size_t Index) const
			{
				return GetRow(Index).GetArray();
			}
			const Core::Vector<Core::String>& Response::GetColumns() const
			{
				return Columns;
			}
			Core::String Response::GetStatusText() const
			{
				return StatusMessage;
			}
			int Response::GetStatusCode() const
			{
				return StatusCode;
			}
			size_t Response::GetColumnIndex(const std::string_view& Name) const
			{
				size_t Size = Name.size(), Index = 0;
				for (auto& Next : Columns)
				{
					if (!strncmp(Next.c_str(), Name.data(), std::min(Next.size(), Size)))
						return Index;
					++Index;
				}
				return std::numeric_limits<size_t>::max();
			}
			size_t Response::AffectedRows() const
			{
				return Stats.AffectedRows;
			}
			size_t Response::LastInsertedRowId() const
			{
				return Stats.LastInsertedRowId;
			}
			size_t Response::Size() const
			{
				return Values.size();
			}
			Row Response::GetRow(size_t Index) const
			{
				if (Index >= Values.size())
					return Row(this, std::numeric_limits<size_t>::max());

				return Row(this, Index);
			}
			Row Response::Front() const
			{
				if (Empty())
					return Row(nullptr, std::numeric_limits<size_t>::max());

				return GetRow(0);
			}
			Row Response::Back() const
			{
				if (Empty())
					return Row(nullptr, std::numeric_limits<size_t>::max());

				return GetRow(Size() - 1);
			}
			Response Response::Copy() const
			{
				return *this;
			}
			bool Response::Empty() const
			{
				return Values.empty();
			}
			bool Response::Error() const
			{
				return !StatusMessage.empty() || StatusCode != 0;
			}

			Cursor::Cursor() : Cursor(nullptr)
			{
			}
			Cursor::Cursor(TConnection* Connection) : Executor(Connection)
			{
				VI_WATCH(this, "sqlite-result cursor");
			}
			Cursor::Cursor(Cursor&& Other) : Base(std::move(Other.Base)), Executor(Other.Executor)
			{
				VI_WATCH(this, "sqlite-result cursor (moved)");
			}
			Cursor::~Cursor()
			{
				VI_UNWATCH(this);
			}
			Cursor& Cursor::operator =(Cursor&& Other)
			{
				if (&Other == this)
					return *this;

				Base = std::move(Other.Base);
				Executor = Other.Executor;
				return *this;
			}
			bool Cursor::Success() const
			{
				return !Error();
			}
			bool Cursor::Empty() const
			{
				if (Base.empty())
					return true;

				for (auto& Item : Base)
				{
					if (!Item.Empty())
						return false;
				}

				return true;
			}
			bool Cursor::Error() const
			{
				if (Base.empty())
					return true;

				for (auto& Item : Base)
				{
					if (Item.Error())
						return true;
				}

				return false;
			}
			size_t Cursor::Size() const
			{
				return Base.size();
			}
			size_t Cursor::AffectedRows() const
			{
				size_t Size = 0;
				for (auto& Item : Base)
					Size += Item.AffectedRows();
				return Size;
			}
			Cursor Cursor::Copy() const
			{
				Cursor Result(Executor);
				if (Base.empty())
					return Result;

				Result.Base.clear();
				Result.Base.reserve(Base.size());

				for (auto& Item : Base)
					Result.Base.emplace_back(Item.Copy());

				return Result;
			}
			TConnection* Cursor::GetExecutor() const
			{
				return Executor;
			}
			const Response& Cursor::First() const
			{
				VI_ASSERT(!Base.empty(), "index outside of range");
				return Base.front();
			}
			const Response& Cursor::Last() const
			{
				VI_ASSERT(!Base.empty(), "index outside of range");
				return Base.back();
			}
			const Response& Cursor::At(size_t Index) const
			{
				VI_ASSERT(Index < Base.size(), "index outside of range");
				return Base[Index];
			}
			Core::Schema* Cursor::GetArrayOfObjects(size_t ResponseIndex) const
			{
				if (ResponseIndex >= Base.size())
					return Core::Var::Set::Array();

				return Base[ResponseIndex].GetArrayOfObjects();
			}
			Core::Schema* Cursor::GetArrayOfArrays(size_t ResponseIndex) const
			{
				if (ResponseIndex >= Base.size())
					return Core::Var::Set::Array();

				return Base[ResponseIndex].GetArrayOfArrays();
			}
			Core::Schema* Cursor::GetObject(size_t ResponseIndex, size_t Index) const
			{
				if (ResponseIndex >= Base.size())
					return nullptr;

				return Base[ResponseIndex].GetObject(Index);
			}
			Core::Schema* Cursor::GetArray(size_t ResponseIndex, size_t Index) const
			{
				if (ResponseIndex >= Base.size())
					return nullptr;

				return Base[ResponseIndex].GetArray(Index);
			}

			Cluster::Cluster()
			{
				LibraryHandle = Driver::Get();
				if (LibraryHandle != nullptr)
					LibraryHandle->AddRef();
			}
			Cluster::~Cluster() noexcept
			{
#ifdef VI_SQLITE
				for (auto* Item : Idle)
					sqlite3_close(Item);
				for (auto* Item : Busy)
				{
					sqlite3_interrupt(Item);
					sqlite3_close(Item);
				}
#endif
				for (auto* Item : Functions)
					Core::Memory::Delete(Item);
				for (auto* Item : Aggregates)
					Core::Memory::Release(Item);
				for (auto* Item : Windows)
					Core::Memory::Release(Item);
				Idle.clear();
				Busy.clear();
				Functions.clear();
				Aggregates.clear();
				Windows.clear();
				Core::Memory::Release(LibraryHandle);
			}
			void Cluster::SetWalAutocheckpoint(uint32_t MaxFrames)
			{
#ifdef VI_SQLITE
				Core::UMutex<std::mutex> Unique(Update);
				for (auto* Item : Idle)
					sqlite3_wal_autocheckpoint(Item, (int)MaxFrames);
#endif
			}
			void Cluster::SetSoftHeapLimit(uint64_t Memory)
			{
#ifdef VI_SQLITE
				Core::UMutex<std::mutex> Unique(Update);
				sqlite3_soft_heap_limit64((int64_t)Memory);
#endif
			}
			void Cluster::SetHardHeapLimit(uint64_t Memory)
			{
#ifdef VI_SQLITE
				Core::UMutex<std::mutex> Unique(Update);
				sqlite3_hard_heap_limit64((int64_t)Memory);
#endif
			}
			void Cluster::SetSharedCache(bool Enabled)
			{
#ifdef VI_SQLITE
				Core::UMutex<std::mutex> Unique(Update);
				sqlite3_enable_shared_cache((int)Enabled);
#endif
			}
			void Cluster::SetExtensions(bool Enabled)
			{
#ifdef VI_SQLITE
				Core::UMutex<std::mutex> Unique(Update);
				for (auto* Item : Idle)
					sqlite3_enable_load_extension(Item, (int)Enabled);
#endif
			}
			void Cluster::SetFunction(const std::string_view& Name, uint8_t Args, OnFunctionResult&& Context)
			{
#ifdef VI_SQLITE
				VI_ASSERT(Context != nullptr, "callable should be set");
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				Core::UMutex<std::mutex> Unique(Update);
				OnFunctionResult* Callable = Core::Memory::New<OnFunctionResult>(std::move(Context));
				for (auto* Item : Idle)
					sqlite3_create_function_v2(Item, Name.data(), (int)Args, SQLITE_UTF8, Callable, &sqlite3_function_call, nullptr, nullptr, nullptr);
				Functions.push_back(Callable);
#endif
			}
			void Cluster::SetAggregateFunction(const std::string_view& Name, uint8_t Args, Core::Unique<Aggregate> Context)
			{
#ifdef VI_SQLITE
				VI_ASSERT(Context != nullptr, "callable should be set");
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				Core::UMutex<std::mutex> Unique(Update);
				for (auto* Item : Idle)
					sqlite3_create_function_v2(Item, Name.data(), (int)Args, SQLITE_UTF8, Context, nullptr, &sqlite3_aggregate_step_call, &sqlite3_aggregate_finalize_call, nullptr);
				Aggregates.push_back(Context);
#endif
			}
			void Cluster::SetWindowFunction(const std::string_view& Name, uint8_t Args, Core::Unique<Window> Context)
			{
#ifdef VI_SQLITE
				VI_ASSERT(Context != nullptr, "callable should be set");
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				Core::UMutex<std::mutex> Unique(Update);
				for (auto* Item : Idle)
					sqlite3_create_window_function(Item, Name.data(), (int)Args, SQLITE_UTF8, Context, &sqlite3_window_step_call, &sqlite3_window_finalize_call, &sqlite3_window_value_call, &sqlite3_window_inverse_call, nullptr);
				Windows.push_back(Context);
#endif
			}
			void Cluster::OverloadFunction(const std::string_view& Name, uint8_t Args)
			{
#ifdef VI_SQLITE
				VI_ASSERT(Core::Stringify::IsCString(Name), "name should be set");
				Core::UMutex<std::mutex> Unique(Update);
				for (auto* Item : Idle)
					sqlite3_overload_function(Item, Name.data(), (int)Args);
#endif
			}
			Core::Vector<Checkpoint> Cluster::WalCheckpoint(CheckpointMode Mode, const std::string_view& Database)
			{
				VI_ASSERT(Database.empty() || Core::Stringify::IsCString(Database), "database should be set");
				Core::Vector<Checkpoint> Checkpoints;
#ifdef VI_SQLITE
				Core::UMutex<std::mutex> Unique(Update);
				for (auto* Item : Idle)
				{
					int FramesSize, FramesCount;
					Checkpoint Result;
					Result.Status = sqlite3_wal_checkpoint_v2(Item, Database.empty() ? nullptr : Database.data(), (int)Mode, &FramesSize, &FramesCount);
					Result.FramesSize = (uint32_t)FramesSize;
					Result.FramesCount = (uint32_t)FramesCount;
					Result.Database = Database;
					Checkpoints.push_back(std::move(Result));
				}
#endif
				return Checkpoints;
			}
			size_t Cluster::FreeMemoryUsed(size_t Bytes)
			{
#ifdef VI_SQLITE
				int Freed = sqlite3_release_memory((int)Bytes);
				return Freed >= 0 ? (size_t)Freed : 0;
#else
				return 0;
#endif
			}
			size_t Cluster::GetMemoryUsed() const
			{
#ifdef VI_SQLITE
				return (size_t)sqlite3_memory_used();
#else
				return 0;
#endif
			}
			ExpectsPromiseDB<SessionId> Cluster::TxBegin(Isolation Type)
			{
				switch (Type)
				{
					case Isolation::Immediate:
						return TxStart("BEGIN IMMEDIATE TRANSACTION");
					case Isolation::Exclusive:
						return TxStart("BEGIN EXCLUSIVE TRANSACTION");
					case Isolation::Deferred:
					default:
						return TxStart("BEGIN TRANSACTION");
				}
			}
			ExpectsPromiseDB<SessionId> Cluster::TxStart(const std::string_view& Command)
			{
				return Query(Command, (size_t)QueryOp::TransactionStart).Then<ExpectsDB<SessionId>>([](ExpectsDB<Cursor>&& Result) -> ExpectsDB<SessionId>
				{
					if (!Result)
						return Result.Error();
					else if (Result->Success())
						return Result->GetExecutor();

					return DatabaseException("transaction start error");
				});
			}
			ExpectsPromiseDB<void> Cluster::TxEnd(const std::string_view& Command, SessionId Session)
			{
				return Query(Command, (size_t)QueryOp::TransactionEnd, Session).Then<ExpectsDB<void>>([](ExpectsDB<Cursor>&& Result) -> ExpectsDB<void>
				{
					if (!Result)
						return Result.Error();
					else if (Result->Success())
						return Core::Expectation::Met;

					return DatabaseException("transaction end error");
				});
			}
			ExpectsPromiseDB<void> Cluster::TxCommit(SessionId Session)
			{
				return TxEnd("COMMIT", Session);
			}
			ExpectsPromiseDB<void> Cluster::TxRollback(SessionId Session)
			{
				return TxEnd("ROLLBACK", Session);
			}
			ExpectsPromiseDB<void> Cluster::Connect(const std::string_view& Location, size_t Connections)
			{
#ifdef VI_SQLITE
				VI_ASSERT(Connections > 0, "connections count should be at least 1");
				bool IsInMemory = Location == ":memory:";
				if (IsInMemory)
				{
					if (!Core::OS::Control::Has(Core::AccessOption::Mem))
						return ExpectsPromiseDB<void>(DatabaseException("connect failed: permission denied"));
				}
				else if (!Core::OS::Control::Has(Core::AccessOption::Fs))
					return ExpectsPromiseDB<void>(DatabaseException("connect failed: permission denied"));

				if (IsConnected())
					return Disconnect().Then<ExpectsPromiseDB<void>>([this, Location, Connections](ExpectsDB<void>&&) { return this->Connect(Location, Connections); });

				Core::UMutex<std::mutex> Unique(Update);
				Source = Location;
				Unique.Negate();

				return Core::Cotask<ExpectsDB<void>>([this, IsInMemory, Connections]() -> ExpectsDB<void>
				{
					VI_MEASURE(Core::Timings::Intensive);
					VI_DEBUG("[sqlite] try open database using %i connections", (int)Connections);

					int Flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI | SQLITE_OPEN_NOMUTEX;
					if (IsInMemory)
						Flags |= SQLITE_OPEN_MEMORY;

					Core::UMutex<std::mutex> Unique(Update);
					Idle.reserve(Connections);
					Busy.reserve(Connections);

					for (size_t i = 0; i < Connections; i++)
					{
						VI_DEBUG("[sqlite] try connect to %s", Source.c_str());
						TConnection* Connection = nullptr;
						int Code = sqlite3_open_v2(Source.c_str(), &Connection, Flags, nullptr);
						if (Code != SQLITE_OK)
						{
							DatabaseException Error(Connection);
							sqlite3_close(Connection);
							Connection = nullptr;
							return Error;
						}

						VI_DEBUG("[sqlite] OK open database on 0x%" PRIXPTR, (uintptr_t)Connection);
						Idle.insert(Connection);
					}

					return Core::Expectation::Met;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException("connect: not supported"));
#endif
			}
			ExpectsPromiseDB<void> Cluster::Disconnect()
			{
#ifdef VI_SQLITE
				if (!IsConnected())
					return ExpectsPromiseDB<void>(DatabaseException("disconnect: not connected"));

				return Core::Cotask<ExpectsDB<void>>([this]() -> ExpectsDB<void>
				{
					Core::UMutex<std::mutex> Unique(Update);
					for (auto* Item : Idle)
						sqlite3_close(Item);
					for (auto* Item : Busy)
					{
						sqlite3_interrupt(Item);
						sqlite3_close(Item);
					}

					Idle.clear();
					Busy.clear();
					return Core::Expectation::Met;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException("disconnect: not supported"));
#endif
			}
			ExpectsPromiseDB<void> Cluster::Flush()
			{
#ifdef VI_SQLITE
				if (!IsConnected())
					return ExpectsPromiseDB<void>(DatabaseException("flush: not connected"));

				return Core::Cotask<ExpectsDB<void>>([this]() -> ExpectsDB<void>
				{
					Core::UMutex<std::mutex> Unique(Update);
					for (auto* Item : Idle)
						sqlite3_db_cacheflush(Item);
					return Core::Expectation::Met;
				});
#else
				return ExpectsPromiseDB<void>(DatabaseException("flush: not supported"));
#endif
			}
			ExpectsPromiseDB<Cursor> Cluster::EmplaceQuery(const std::string_view& Command, Core::SchemaList* Map, size_t Opts, SessionId Session)
			{
				auto Template = Driver::Get()->Emplace(Command, Map, !(Opts & (size_t)QueryOp::ReuseArgs));
				if (!Template)
					return ExpectsPromiseDB<Cursor>(Template.Error());

				return Query(*Template, Opts, Session);
			}
			ExpectsPromiseDB<Cursor> Cluster::TemplateQuery(const std::string_view& Name, Core::SchemaArgs* Map, size_t Opts, SessionId Session)
			{
				VI_DEBUG("[sqlite] template query %s", Name.empty() ? "empty-query-name" : Core::String(Name).c_str());
				auto Template = Driver::Get()->GetQuery(Name, Map, !(Opts & (size_t)QueryOp::ReuseArgs));
				if (!Template)
					return ExpectsPromiseDB<Cursor>(Template.Error());

				return Query(*Template, Opts, Session);
			}
			ExpectsPromiseDB<Cursor> Cluster::Query(const std::string_view& Command, size_t Opts, SessionId Session)
			{
				VI_ASSERT(!Command.empty(), "command should not be empty");
#ifdef VI_SQLITE
				Core::String QueryCommand = Core::String(Command);
				Driver::Get()->LogQuery(Command);
				return Core::Coasync<ExpectsDB<Cursor>>([this, QueryCommand = std::move(QueryCommand), Opts, Session]() mutable -> ExpectsPromiseDB<Cursor>
				{
					Core::String Statement = std::move(QueryCommand);
					TConnection* Connection = VI_AWAIT(AcquireConnection(Session, Opts));
					if (!Connection)
						Coreturn DatabaseException("acquire connection error: no candidate");

					auto Time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
					VI_DEBUG("[sqlite] execute query on 0x%" PRIXPTR "%s: %.64s%s", (uintptr_t)Connection, Session ? " (transaction)" : "", Statement.data(), Statement.size() > 64 ? " ..." : "");

					size_t Queries = 0;
					Cursor Result(Connection);
					while (!Statement.empty())
					{
						if (++Queries > 1)
						{
							if (!VI_AWAIT(Core::Cotask<ExpectsDB<void>>([this, Connection, &Result, &Statement]() { return ConsumeStatement(Connection, &Result, &Statement); })))
							{
								ReleaseConnection(Connection, Opts);
								Coreturn Result;
							}
						}
						else
						{
							if (!ConsumeStatement(Connection, &Result, &Statement))
							{
								ReleaseConnection(Connection, Opts);
								Coreturn Result;
							}
						}
					}

					VI_DEBUG("[sqlite] OK execute on 0x%" PRIXPTR " (%" PRIu64 " ms)", (uintptr_t)Connection, (uint64_t)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - Time).count());
					ReleaseConnection(Connection, Opts);
					Coreturn Result;
				});
#else
				return ExpectsPromiseDB<Cursor>(DatabaseException("query: not supported"));
#endif
			}
			TConnection* Cluster::TryAcquireConnection(SessionId Session, size_t Opts)
			{
#ifdef VI_SQLITE
				auto It = Idle.begin();
				while (It != Idle.end())
				{
					bool IsInTransaction = (!sqlite3_get_autocommit(*It));
					if (Session == *It)
					{
						if (!IsInTransaction)
							return nullptr;
						break;
					}
					else if (!IsInTransaction && !(Opts & (size_t)QueryOp::TransactionEnd))
					{
						if (Opts & (size_t)QueryOp::TransactionStart)
							VI_DEBUG("[sqlite] acquire transaction on 0x%" PRIXPTR, (uintptr_t)*It);
						break;
					}
					else if (IsInTransaction && !(Opts & (size_t)QueryOp::TransactionStart))
						break;
					++It;
				}
				if (It == Idle.end())
					return nullptr;

				TConnection* Connection = *It;
				Busy.insert(Connection);
				Idle.erase(It);
				return Connection;
#else
				return nullptr;
#endif
			}
			Core::Promise<TConnection*> Cluster::AcquireConnection(SessionId Session, size_t Opts)
			{
				Core::UMutex<std::mutex> Unique(Update);
				TConnection* Connection = TryAcquireConnection(Session, Opts);
				if (Connection != nullptr)
					return Core::Promise<TConnection*>(Connection);

				Request Target;
				Target.Session = Session;
				Target.Opts = Opts;
				Queues[Session].push(Target);
				return Target.Future;
			}
			void Cluster::ReleaseConnection(TConnection* Connection, size_t Opts)
			{
				Core::UMutex<std::mutex> Unique(Update);
				if (Opts & (size_t)QueryOp::TransactionEnd)
					VI_DEBUG("[sqlite] release transaction on 0x%" PRIXPTR, (uintptr_t)Connection);
				Busy.erase(Connection);
				Idle.insert(Connection);

				auto It = Queues.find(Connection);
				if (It == Queues.end() || It->second.empty())
					It = Queues.find(nullptr);

				if (It == Queues.end() || It->second.empty())
					return;

				Request& Target = It->second.front();
				TConnection* NewConnection = TryAcquireConnection(Target.Session, Target.Opts);
				if (!NewConnection)
					return;

				auto Future = std::move(Target.Future);
				It->second.pop();
				Unique.Negate();
				Future.Set(NewConnection);
			}
			TConnection* Cluster::GetIdleConnection()
			{
				Core::UMutex<std::mutex> Unique(Update);
				if (!Idle.empty())
					return *Idle.begin();
				return nullptr;
			}
			TConnection* Cluster::GetBusyConnection()
			{
				Core::UMutex<std::mutex> Unique(Update);
				if (!Busy.empty())
					return *Busy.begin();
				return nullptr;
			}
			TConnection* Cluster::GetAnyConnection()
			{
				Core::UMutex<std::mutex> Unique(Update);
				if (!Idle.empty())
					return *Idle.begin();
				else if (!Busy.empty())
					return *Busy.begin();
				return nullptr;
			}
			const Core::String& Cluster::GetAddress()
			{
				return Source;
			}
			bool Cluster::IsConnected()
			{
				Core::UMutex<std::mutex> Unique(Update);
				return !Idle.empty() || !Busy.empty();
			}
			ExpectsDB<void> Cluster::ConsumeStatement(TConnection* Connection, Cursor* Result, Core::String* Statement)
			{
#ifdef VI_SQLITE
				VI_MEASURE(Core::Timings::Intensive);
				const char* TrailingStatement = nullptr;
				sqlite3_stmt* Target = nullptr;
				Result->Base.emplace_back();

				auto& Context = Result->Base.back();
				int Code = sqlite3_prepare_v2(Connection, Statement->c_str(), (int)Statement->size(), &Target, &TrailingStatement);
				if (TrailingStatement != nullptr)
					Statement->erase(Statement->begin(), Statement->begin() + (TrailingStatement - Statement->c_str()));
				else
					Statement->clear();

				if (Code != SQLITE_OK)
					goto StopExecution;
			NextReturnable:
				Code = sqlite3_step(Target);
				if (Context.Columns.empty())
					ConsumeColumns(Target, &Context);
				switch (Code)
				{
					case SQLITE_ROW:
					{
						ConsumeRow(Target, &Context);
						goto NextReturnable;
					}
					case SQLITE_DONE:
						goto NextStatement;
					case SQLITE_ERROR:
					default:
						goto StopExecution;
				}
			NextStatement:
				Context.StatusCode = SQLITE_OK;
				Context.Stats.AffectedRows = sqlite3_changes64(Connection);
				Context.Stats.LastInsertedRowId = sqlite3_last_insert_rowid(Connection);
				sqlite3_finalize(Target);
				return Core::Expectation::Met;
			StopExecution:
				Context.StatusCode = Code;
				Context.StatusMessage = sqlite3_errmsg(Connection);
				if (Target != nullptr)
					sqlite3_finalize(Target);
				return DatabaseException(Core::String(Context.StatusMessage));
#else
				return DatabaseException("consume: not supported");
#endif
			}
			void Cluster::ConsumeColumns(TStatement* Target, Response* Context)
			{
#ifdef VI_SQLITE
				int Columns = sqlite3_column_count(Target);
				if (Columns > 0 && Columns > (int)Context->Columns.size())
				{
					Context->Columns.reserve((int)Columns);
					for (int i = 0; i < Columns; i++)
						Context->Columns.push_back(sqlite3_column_name(Target, i));
				}
#endif
			}
			void Cluster::ConsumeRow(TStatement* Target, Response* Context)
			{
#ifdef VI_SQLITE
				Context->Values.emplace_back();
				auto& Row = Context->Values.back();
				if (Context->Columns.empty())
					return;

				int Columns = (int)Context->Columns.size();
				Row.reserve(Context->Columns.size());
				for (int i = 0; i < Columns; i++)
					{
						switch (sqlite3_column_type(Target, i))
						{
							case SQLITE_INTEGER:
								Row.push_back(Core::Var::Integer(sqlite3_column_int64(Target, i)));
								break;
							case SQLITE_FLOAT:
								Row.push_back(Core::Var::Number(sqlite3_column_double(Target, i)));
								break;
							case SQLITE_TEXT:
							{
								std::string_view Text((const char*)sqlite3_column_text(Target, i), (size_t)sqlite3_column_bytes(Target, i));
								if (Core::Stringify::HasDecimal(Text))
									Row.push_back(Core::Var::DecimalString(Text));
								else if (Core::Stringify::HasInteger(Text))
									Row.push_back(Core::Var::Integer(Core::FromString<int64_t>(Text).Or(0)));
								else if (Core::Stringify::HasNumber(Text))
									Row.push_back(Core::Var::Number(Core::FromString<double>(Text).Or(0.0)));
								else
									Row.push_back(Core::Var::String(Text));
								break;
							}
							case SQLITE_BLOB:
							{
								const uint8_t* Blob = (const uint8_t*)sqlite3_column_blob(Target, i);
								size_t BlobSize = (size_t)sqlite3_column_bytes(Target, i);
								Row.push_back(Core::Var::Binary(Blob ? Blob : (uint8_t*)"", BlobSize));
								break;
							}
							case SQLITE_NULL:
								Row.push_back(Core::Var::Null());
								break;
							default:
								Row.push_back(Core::Var::Undefined());
								break;
						}
					}
#endif
			}

			ExpectsDB<Core::String> Utils::InlineArray(Core::Schema* Array)
			{
				VI_ASSERT(Array != nullptr, "array should be set");
				Core::SchemaList Map;
				Core::String Def;

				for (auto* Item : Array->GetChilds())
				{
					if (Item->Value.IsObject())
					{
						if (Item->Empty())
							continue;

						Def += '(';
						for (auto* Sub : Item->GetChilds())
						{
							Map.push_back(Sub);
							Def += "?,";
						}
						Def.erase(Def.end() - 1);
						Def += "),";
					}
					else
					{
						Map.push_back(Item);
						Def += "?,";
					}
				}

				auto Result = LDB::Driver::Get()->Emplace(Def, &Map, false);
				if (Result && !Result->empty() && Result->back() == ',')
					Result->erase(Result->end() - 1);

				Core::Memory::Release(Array);
				return Result;
			}
			ExpectsDB<Core::String> Utils::InlineQuery(Core::Schema* Where, const Core::UnorderedMap<Core::String, Core::String>& Whitelist, const std::string_view& Default)
			{
				VI_ASSERT(Where != nullptr, "array should be set");
				Core::SchemaList Map;
				Core::String Allow = "abcdefghijklmnopqrstuvwxyz._", Def;
				for (auto* Statement : Where->GetChilds())
				{
					Core::String Op = Statement->Value.GetBlob();
					if (Op == "=" || Op == "<>" || Op == "<=" || Op == "<" || Op == ">" || Op == ">=" || Op == "+" || Op == "-" || Op == "*" || Op == "/" || Op == "(" || Op == ")" || Op == "TRUE" || Op == "FALSE")
						Def += Op;
					else if (Op == "~==")
						Def += " LIKE ";
					else if (Op == "~=")
						Def += " ILIKE ";
					else if (Op == "[")
						Def += " ANY(";
					else if (Op == "]")
						Def += ")";
					else if (Op == "&")
						Def += " AND ";
					else if (Op == "|")
						Def += " OR ";
					else if (Op == "TRUE")
						Def += " TRUE ";
					else if (Op == "FALSE")
						Def += " FALSE ";
					else if (!Op.empty())
					{
						if (Op.front() == '@')
						{
							Op = Op.substr(1);
							if (!Whitelist.empty())
							{
								auto It = Whitelist.find(Op);
								if (It != Whitelist.end() && Op.find_first_not_of(Allow) == Core::String::npos)
									Def += It->second.empty() ? Op : It->second;
							}
							else if (Op.find_first_not_of(Allow) == Core::String::npos)
								Def += Op;
						}
						else if (!Core::Stringify::HasNumber(Op))
						{
							Def += "?";
							Map.push_back(Statement);
						}
						else
							Def += Op;
					}
				}

				auto Result = LDB::Driver::Get()->Emplace(Def, &Map, false);
				if (Result && Result->empty())
					Result = Core::String(Default);

				Core::Memory::Release(Where);
				return Result;
			}
			Core::String Utils::GetCharArray(const std::string_view& Src) noexcept
			{
				if (Src.empty())
					return "''";

				Core::String Dest(Src);
				Core::Stringify::Replace(Dest, "'", "''");
				Dest.insert(Dest.begin(), '\'');
				Dest.append(1, '\'');
				return Dest;
			}
			Core::String Utils::GetByteArray(const std::string_view& Src) noexcept
			{
				if (Src.empty())
					return "''";

				return "x'" + Compute::Codec::HexEncode(Src) + "'";
			}
			Core::String Utils::GetSQL(Core::Schema* Source, bool Escape, bool Negate) noexcept
			{
				if (!Source)
					return "NULL";

				Core::Schema* Parent = Source->GetParent();
				switch (Source->Value.GetType())
				{
					case Core::VarType::Object:
					{
						Core::String Result;
						Core::Schema::ConvertToJSON(Source, [&Result](Core::VarForm, const std::string_view& Buffer) { Result.append(Buffer); });
						return Escape ? GetCharArray(Result) : Result;
					}
					case Core::VarType::Array:
					{
						Core::String Result = (Parent && Parent->Value.GetType() == Core::VarType::Array ? "[" : "ARRAY[");
						for (auto* Node : Source->GetChilds())
							Result.append(GetSQL(Node, Escape, Negate)).append(1, ',');

						if (!Source->Empty())
							Result = Result.substr(0, Result.size() - 1);

						return Result + "]";
					}
					case Core::VarType::String:
					{
						if (Escape)
							return GetCharArray(Source->Value.GetBlob());

						return Source->Value.GetBlob();
					}
					case Core::VarType::Integer:
						return Core::ToString(Negate ? -Source->Value.GetInteger() : Source->Value.GetInteger());
					case Core::VarType::Number:
						return Core::ToString(Negate ? -Source->Value.GetNumber() : Source->Value.GetNumber());
					case Core::VarType::Boolean:
						return (Negate ? !Source->Value.GetBoolean() : Source->Value.GetBoolean()) ? "TRUE" : "FALSE";
					case Core::VarType::Decimal:
					{
						Core::Decimal Value = Source->Value.GetDecimal();
						if (Value.IsNaN())
							return "NULL";

						Core::String Result = (Negate ? '-' + Value.ToString() : Value.ToString());
						return Result.find('.') != Core::String::npos ? Result : Result + ".0";
					}
					case Core::VarType::Binary:
						return GetByteArray(Source->Value.GetString());
					case Core::VarType::Null:
					case Core::VarType::Undefined:
						return "NULL";
					default:
						break;
				}

				return "NULL";
			}
			Core::Schema* Utils::GetSchemaFromValue(const Core::Variant& Value) noexcept
			{
				if (!Value.Is(Core::VarType::String))
					return new Core::Schema(Value);

				auto Data = Core::Schema::FromJSON(Value.GetBlob());
				return Data ? *Data : new Core::Schema(Value);
			}
			Core::VariantList Utils::ContextArgs(TValue** Values, size_t ValuesCount) noexcept
			{
				Core::VariantList Args;
#ifdef VI_SQLITE
				Args.reserve(ValuesCount);
				for (size_t i = 0; i < ValuesCount; i++)
				{
					TValue* Value = Values[i];
					switch (sqlite3_value_type(Value))
					{
						case SQLITE_INTEGER:
							Args.push_back(Core::Var::Integer(sqlite3_value_int64(Value)));
							break;
						case SQLITE_FLOAT:
							Args.push_back(Core::Var::Number(sqlite3_value_double(Value)));
							break;
						case SQLITE_TEXT:
						{
							std::string_view Text((const char*)sqlite3_value_text(Value), (size_t)sqlite3_value_bytes(Value));
							if (Core::Stringify::HasDecimal(Text))
								Args.push_back(Core::Var::DecimalString(Text));
							else if (Core::Stringify::HasInteger(Text))
								Args.push_back(Core::Var::Integer(Core::FromString<int64_t>(Text).Or(0)));
							else if (Core::Stringify::HasNumber(Text))
								Args.push_back(Core::Var::Number(Core::FromString<double>(Text).Or(0.0)));
							else
								Args.push_back(Core::Var::String(Text));
							break;
						}
						case SQLITE_BLOB:
						{
							const uint8_t* Blob = (const uint8_t*)sqlite3_value_blob(Value);
							size_t BlobSize = (size_t)sqlite3_value_bytes(Value);
							Args.push_back(Core::Var::Binary(Blob ? Blob : (uint8_t*)"", BlobSize));
							break;
						}
						case SQLITE_NULL:
							Args.push_back(Core::Var::Null());
							break;
						default:
							Args.push_back(Core::Var::Undefined());
							break;
					}
				}
#endif
				return Args;
			}
			void Utils::ContextReturn(TContext* Context, const Core::Variant& Result) noexcept
			{
#ifdef VI_SQLITE
				switch (Result.GetType())
				{
					case Core::VarType::String:
						sqlite3_result_text64(Context, Result.GetString().data(), (uint64_t)Result.Size(), SQLITE_TRANSIENT, SQLITE_UTF8);
						break;
					case Core::VarType::Binary:
						sqlite3_result_blob64(Context, Result.GetString().data(), (uint64_t)Result.Size(), SQLITE_TRANSIENT);
						break;
					case Core::VarType::Integer:
						sqlite3_result_int64(Context, Result.GetInteger());
						break;
					case Core::VarType::Number:
						sqlite3_result_double(Context, Result.GetNumber());
						break;
					case Core::VarType::Decimal:
					{
						auto Numeric = Result.GetBlob();
						sqlite3_result_text64(Context, Numeric.c_str(), (uint64_t)Numeric.size(), SQLITE_TRANSIENT, SQLITE_UTF8);
						break;
					}
					case Core::VarType::Boolean:
						sqlite3_result_int(Context, (int)Result.GetBoolean());
						break;
					case Core::VarType::Null:
					case Core::VarType::Undefined:
					case Core::VarType::Object:
					case Core::VarType::Array:
					case Core::VarType::Pointer:
					default:
						sqlite3_result_null(Context);
						break;
				}
#endif
			}

			Driver::Driver() noexcept : Active(false), Logger(nullptr)
			{
#ifdef VI_SQLITE
				sqlite3_initialize();
#endif
				Network::Multiplexer::Get()->Activate();
			}
			Driver::~Driver() noexcept
			{
				Network::Multiplexer::Get()->Deactivate();
#ifdef VI_SQLITE
				sqlite3_shutdown();
#endif
			}
			void Driver::SetQueryLog(const OnQueryLog& Callback) noexcept
			{
				Logger = Callback;
			}
			void Driver::LogQuery(const std::string_view& Command) noexcept
			{
				if (Logger)
					Logger(Command);
			}
			void Driver::AddConstant(const std::string_view& Name, const std::string_view& Value)
			{
				VI_ASSERT(!Name.empty(), "name should not be empty");
				Core::UMutex<std::mutex> Unique(Exclusive);
				Constants[Core::String(Name)] = Value;
			}
			ExpectsDB<void> Driver::AddQuery(const std::string_view& Name, const std::string_view& Buffer)
			{
				VI_ASSERT(!Name.empty(), "name should not be empty");
				if (Buffer.empty())
					return DatabaseException("import empty query error: " + Core::String(Name));

				Sequence Result;
				Result.Request.assign(Buffer);

				Core::String Lines = "\r\n";
				Core::String Enums = " \r\n\t\'\"()<>=%&^*/+-,!?:;";
				Core::String Erasable = " \r\n\t\'\"()<>=%&^*/+-,.!?:;";
				Core::String Quotes = "\"'`";

				Core::String& Base = Result.Request;
				Core::Stringify::ReplaceInBetween(Base, "/*", "*/", "", false);
				Core::Stringify::ReplaceStartsWithEndsOf(Base, "--", Lines.c_str(), "");
				Core::Stringify::Trim(Base);
				Core::Stringify::Compress(Base, Erasable.c_str(), Quotes.c_str());

				auto Enumerations = Core::Stringify::FindStartsWithEndsOf(Base, "#", Enums.c_str(), Quotes.c_str());
				if (!Enumerations.empty())
				{
					int64_t Offset = 0;
					Core::UMutex<std::mutex> Unique(Exclusive);
					for (auto& Item : Enumerations)
					{
						size_t Size = Item.second.End - Item.second.Start;
						Item.second.Start = (size_t)((int64_t)Item.second.Start + Offset);
						Item.second.End = (size_t)((int64_t)Item.second.End + Offset);

						auto It = Constants.find(Item.first);
						if (It == Constants.end())
							return DatabaseException("query expects " + Item.first + " constrant: " + Core::String(Name));

						Core::Stringify::ReplacePart(Base, Item.second.Start, Item.second.End, It->second);
						size_t NewSize = It->second.size();
						Offset += (int64_t)NewSize - (int64_t)Size;
						Item.second.End = Item.second.Start + NewSize;
					}
				}

				Core::Vector<std::pair<Core::String, Core::TextSettle>> Variables;
				for (auto& Item : Core::Stringify::FindInBetween(Base, "$<", ">", Quotes.c_str()))
				{
					Item.first += ";escape";
					if (Core::Stringify::IsPrecededBy(Base, Item.second.Start, "-"))
					{
						Item.first += ";negate";
						--Item.second.Start;
					}

					Variables.emplace_back(std::move(Item));
				}

				for (auto& Item : Core::Stringify::FindInBetween(Base, "@<", ">", Quotes.c_str()))
				{
					Item.first += ";unsafe";
					if (Core::Stringify::IsPrecededBy(Base, Item.second.Start, "-"))
					{
						Item.first += ";negate";
						--Item.second.Start;
					}

					Variables.emplace_back(std::move(Item));
				}

				Core::Stringify::ReplaceParts(Base, Variables, "", [&Erasable](const std::string_view& Name, char Left, int Side)
				{
					if (Side < 0 && Name.find(";negate") != Core::String::npos)
						return '\0';

					return Erasable.find(Left) == Core::String::npos ? ' ' : '\0';
				});

				for (auto& Item : Variables)
				{
					Pose Position;
					Position.Negate = Item.first.find(";negate") != Core::String::npos;
					Position.Escape = Item.first.find(";escape") != Core::String::npos;
					Position.Offset = Item.second.Start;
					Position.Key = Item.first.substr(0, Item.first.find(';'));
					Result.Positions.emplace_back(std::move(Position));
				}

				if (Variables.empty())
					Result.Cache = Result.Request;

				Core::UMutex<std::mutex> Unique(Exclusive);
				Queries[Core::String(Name)] = std::move(Result);
				return Core::Expectation::Met;
			}
			ExpectsDB<void> Driver::AddDirectory(const std::string_view& Directory, const std::string_view& Origin)
			{
				Core::Vector<std::pair<Core::String, Core::FileEntry>> Entries;
				if (!Core::OS::Directory::Scan(Directory, Entries))
					return DatabaseException("import directory scan error: " + Core::String(Directory));

				Core::String Path = Core::String(Directory);
				if (Path.back() != '/' && Path.back() != '\\')
					Path.append(1, '/');

				size_t Size = 0;
				for (auto& File : Entries)
				{
					Core::String Base(Path + File.first);
					if (File.second.IsDirectory)
					{
						auto Status = AddDirectory(Base, Origin.empty() ? Directory : Origin);
						if (!Status)
							return Status;
						else
							continue;
					}

					if (!Core::Stringify::EndsWith(Base, ".sql"))
						continue;

					auto Buffer = Core::OS::File::ReadAll(Base, &Size);
					if (!Buffer)
						continue;

					Core::Stringify::Replace(Base, Origin.empty() ? Directory : Origin, "");
					Core::Stringify::Replace(Base, "\\", "/");
					Core::Stringify::Replace(Base, ".sql", "");
					if (Core::Stringify::StartsOf(Base, "\\/"))
						Base.erase(0, 1);

					auto Status = AddQuery(Base, std::string_view((char*)*Buffer, Size));
					Core::Memory::Deallocate(*Buffer);
					if (!Status)
						return Status;
				}

				return Core::Expectation::Met;
			}
			bool Driver::RemoveConstant(const std::string_view& Name) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Constants.find(Core::HglCast(Name));
				if (It == Constants.end())
					return false;

				Constants.erase(It);
				return true;
			}
			bool Driver::RemoveQuery(const std::string_view& Name) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Queries.find(Core::HglCast(Name));
				if (It == Queries.end())
					return false;

				Queries.erase(It);
				return true;
			}
			bool Driver::LoadCacheDump(Core::Schema* Dump) noexcept
			{
				VI_ASSERT(Dump != nullptr, "dump should be set");
				size_t Count = 0;
				Core::UMutex<std::mutex> Unique(Exclusive);
				Queries.clear();

				for (auto* Data : Dump->GetChilds())
				{
					Sequence Result;
					Result.Cache = Data->GetVar("cache").GetBlob();
					Result.Request = Data->GetVar("request").GetBlob();

					if (Result.Request.empty())
						Result.Request = Result.Cache;

					Core::Schema* Positions = Data->Get("positions");
					if (Positions != nullptr)
					{
						for (auto* Position : Positions->GetChilds())
						{
							Pose Next;
							Next.Key = Position->GetVar(0).GetBlob();
							Next.Offset = (size_t)Position->GetVar(1).GetInteger();
							Next.Escape = Position->GetVar(2).GetBoolean();
							Next.Negate = Position->GetVar(3).GetBoolean();
							Result.Positions.emplace_back(std::move(Next));
						}
					}

					Core::String Name = Data->GetVar("name").GetBlob();
					Queries[Name] = std::move(Result);
					++Count;
				}

				if (Count > 0)
					VI_DEBUG("[sqlite] OK load %" PRIu64 " parsed query templates", (uint64_t)Count);

				return Count > 0;
			}
			Core::Schema* Driver::GetCacheDump() noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				Core::Schema* Result = Core::Var::Set::Array();
				for (auto& Query : Queries)
				{
					Core::Schema* Data = Result->Push(Core::Var::Set::Object());
					Data->Set("name", Core::Var::String(Query.first));

					if (Query.second.Cache.empty())
						Data->Set("request", Core::Var::String(Query.second.Request));
					else
						Data->Set("cache", Core::Var::String(Query.second.Cache));

					auto* Positions = Data->Set("positions", Core::Var::Set::Array());
					for (auto& Position : Query.second.Positions)
					{
						auto* Next = Positions->Push(Core::Var::Set::Array());
						Next->Push(Core::Var::String(Position.Key));
						Next->Push(Core::Var::Integer(Position.Offset));
						Next->Push(Core::Var::Boolean(Position.Escape));
						Next->Push(Core::Var::Boolean(Position.Negate));
					}
				}

				VI_DEBUG("[sqlite] OK save %" PRIu64 " parsed query templates", (uint64_t)Queries.size());
				return Result;
			}
			ExpectsDB<Core::String> Driver::Emplace(const std::string_view& SQL, Core::SchemaList* Map, bool Once) noexcept
			{
				if (!Map || Map->empty())
					return Core::String(SQL);

				Core::String Buffer(SQL);
				Core::TextSettle Set;
				Core::String& Src = Buffer;
				size_t Offset = 0;
				size_t Next = 0;

				while ((Set = Core::Stringify::Find(Buffer, '?', Offset)).Found)
				{
					if (Next >= Map->size())
						return DatabaseException("query expects at least " + Core::ToString(Next + 1) + " arguments: " + Core::String(SQL.substr(Set.Start, 64)));

					bool Escape = true, Negate = false;
					if (Set.Start > 0)
					{
						if (Src[Set.Start - 1] == '\\')
						{
							Offset = Set.Start;
							Buffer.erase(Set.Start - 1, 1);
							continue;
						}
						else if (Src[Set.Start - 1] == '$')
						{
							if (Set.Start > 1 && Src[Set.Start - 2] == '-')
							{
								Negate = true;
								Set.Start--;
							}

							Escape = false;
							Set.Start--;
						}
						else if (Src[Set.Start - 1] == '-')
						{
							Negate = true;
							Set.Start--;
						}
					}
					Core::String Value = Utils::GetSQL((*Map)[Next++], Escape, Negate);
					Buffer.erase(Set.Start, (Escape ? 1 : 2));
					Buffer.insert(Set.Start, Value);
					Offset = Set.Start + Value.size();
				}

				if (!Once)
					return Src;

				for (auto* Item : *Map)
					Core::Memory::Release(Item);
				Map->clear();
				return Src;
			}
			ExpectsDB<Core::String> Driver::GetQuery(const std::string_view& Name, Core::SchemaArgs* Map, bool Once) noexcept
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Queries.find(Core::HglCast(Name));
				if (It == Queries.end())
				{
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							Core::Memory::Release(Item.second);
						Map->clear();
					}

					return DatabaseException("query not found: " + Core::String(Name));
				}

				if (!It->second.Cache.empty())
				{
					Core::String Result = It->second.Cache;
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							Core::Memory::Release(Item.second);
						Map->clear();
					}

					return Result;
				}

				if (!Map || Map->empty())
				{
					Core::String Result = It->second.Request;
					if (Once && Map != nullptr)
					{
						for (auto& Item : *Map)
							Core::Memory::Release(Item.second);
						Map->clear();
					}

					return Result;
				}

				Sequence Origin = It->second;
				size_t Offset = 0;
				Unique.Negate();

				Core::String& Result = Origin.Request;
				for (auto& Word : Origin.Positions)
				{
					auto It = Map->find(Word.Key);
					if (It == Map->end())
						return DatabaseException("query expects " + Word.Key + " constrant: " + Core::String(Name));

					Core::String Value = Utils::GetSQL(It->second, Word.Escape, Word.Negate);
					if (Value.empty())
						continue;

					Result.insert(Word.Offset + Offset, Value);
					Offset += Value.size();
				}

				if (Once)
				{
					for (auto& Item : *Map)
						Core::Memory::Release(Item.second);
					Map->clear();
				}

				Core::String Data = Origin.Request;
				if (Data.empty())
					return DatabaseException("query construction error: " + Core::String(Name));

				return Data;
			}
			Core::Vector<Core::String> Driver::GetQueries() noexcept
			{
				Core::Vector<Core::String> Result;
				Core::UMutex<std::mutex> Unique(Exclusive);
				Result.reserve(Queries.size());
				for (auto& Item : Queries)
					Result.push_back(Item.first);

				return Result;
			}
		}
	}
}
