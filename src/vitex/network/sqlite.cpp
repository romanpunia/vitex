#include "sqlite.h"
#ifdef VI_SQLITE
#include <sqlite3.h>
#ifdef VI_APPLE
#include <sqlite3ext.h>
#endif
#endif

namespace vitex
{
	namespace network
	{
		namespace sqlite
		{
#ifdef VI_SQLITE
			class sqlite3_util
			{
			public:
				static void function_call(sqlite3_context* context, int args_count, sqlite3_value** args)
				{
					on_function_result* callable = (on_function_result*)sqlite3_user_data(context);
					VI_ASSERT(callable != nullptr, "callable is null");
					utils::context_return(context, (*callable)(utils::context_args(args, (size_t)args_count)));
				}
				static void aggregate_step_call(sqlite3_context* context, int args_count, sqlite3_value** args)
				{
					aggregate* callable = (aggregate*)sqlite3_user_data(context);
					VI_ASSERT(callable != nullptr, "callable is null");
					callable->step(utils::context_args(args, (size_t)args_count));
				}
				static void aggregate_finalize_call(sqlite3_context* context)
				{
					aggregate* callable = (aggregate*)sqlite3_user_data(context);
					VI_ASSERT(callable != nullptr, "callable is null");
					utils::context_return(context, callable->finalize());
				}
				static void window_step_call(sqlite3_context* context, int args_count, sqlite3_value** args)
				{
					window* callable = (window*)sqlite3_user_data(context);
					VI_ASSERT(callable != nullptr, "callable is null");
					callable->step(utils::context_args(args, (size_t)args_count));
				}
				static void window_finalize_call(sqlite3_context* context)
				{
					window* callable = (window*)sqlite3_user_data(context);
					VI_ASSERT(callable != nullptr, "callable is null");
					utils::context_return(context, callable->finalize());
				}
				static void window_inverse_call(sqlite3_context* context, int args_count, sqlite3_value** args)
				{
					window* callable = (window*)sqlite3_user_data(context);
					VI_ASSERT(callable != nullptr, "callable is null");
					callable->inverse(utils::context_args(args, (size_t)args_count));
				}
				static void window_value_call(sqlite3_context* context)
				{
					window* callable = (window*)sqlite3_user_data(context);
					VI_ASSERT(callable != nullptr, "callable is null");
					utils::context_return(context, callable->value());
				}
				static expects_db<void> execute(sqlite3* handle, sqlite3_stmt* target, sqlite::response& response, uint64_t timeout)
				{
					bool slept = false;
				next_returnable:
					int code = sqlite3_step(target);
					if (response.columns.empty())
					{
						int columns = sqlite3_column_count(target);
						if (columns > 0 && columns > (int)response.columns.size())
						{
							response.columns.reserve((int)columns);
							for (int i = 0; i < columns; i++)
								response.columns.push_back(sqlite3_column_name(target, i));
						}
					}
					switch (code)
					{
						case SQLITE_ROW:
						{
							response.values.emplace_back();
							auto& row = response.values.back();
							if (!response.columns.empty())
							{
								int columns = (int)response.columns.size();
								row.reserve(response.columns.size());
								for (int i = 0; i < columns; i++)
								{
									switch (sqlite3_column_type(target, i))
									{
										case SQLITE_INTEGER:
											row.push_back(core::var::integer(sqlite3_column_int64(target, i)));
											break;
										case SQLITE_FLOAT:
											row.push_back(core::var::number(sqlite3_column_double(target, i)));
											break;
										case SQLITE_TEXT:
										{
											std::string_view text((const char*)sqlite3_column_text(target, i), (size_t)sqlite3_column_bytes(target, i));
											if (core::stringify::has_decimal(text))
												row.push_back(core::var::decimal_string(text));
											else if (core::stringify::has_integer(text))
												row.push_back(core::var::integer(core::from_string<int64_t>(text).otherwise(0)));
											else if (core::stringify::has_number(text))
												row.push_back(core::var::number(core::from_string<double>(text).otherwise(0.0)));
											else
												row.push_back(core::var::string(text));
											break;
										}
										case SQLITE_BLOB:
										{
											const uint8_t* blob = (const uint8_t*)sqlite3_column_blob(target, i);
											size_t blob_size = (size_t)sqlite3_column_bytes(target, i);
											row.push_back(core::var::binary(blob ? blob : (uint8_t*)"", blob_size));
											break;
										}
										case SQLITE_NULL:
											row.push_back(core::var::null());
											break;
										default:
											row.push_back(core::var::undefined());
											break;
									}
								}
							}
							goto next_returnable;
						}
						case SQLITE_DONE:
							goto next_statement;
						case SQLITE_BUSY:
						case SQLITE_LOCKED:
							if (slept)
								goto stop_execution;

							std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
							slept = true;
							goto next_statement;
						case SQLITE_ERROR:
						default:
							goto stop_execution;
					}
				next_statement:
					response.status_code = SQLITE_OK;
					response.stats.affected_rows = sqlite3_changes64(handle);
					response.stats.last_inserted_row_id = sqlite3_last_insert_rowid(handle);
					return core::expectation::met;
				stop_execution:
					int error = sqlite3_errcode(handle);
					response.status_code = error == SQLITE_OK ? code : error;
					response.status_message = error == SQLITE_OK ? sqlite3_errstr(code) : sqlite3_errmsg(handle);
					return database_exception(core::string(response.status_message));
				}
			};
#endif
			database_exception::database_exception(tconnection* connection)
			{
#ifdef VI_SQLITE
				VI_ASSERT(connection != nullptr, "base should be set");
				const char* new_message = sqlite3_errmsg(connection);
				if (new_message && new_message[0] != '\0')
				{
					VI_DEBUG("[sqlite] %s", new_message);
					error_message = new_message;
				}
				else
					error_message = "internal driver error";
#else
				error_message = "not supported";
#endif
			}
			database_exception::database_exception(core::string&& new_message)
			{
				error_message = std::move(new_message);
			}
			const char* database_exception::type() const noexcept
			{
				return "sqlite_error";
			}

			column::column(const response* new_base, size_t fRowIndex, size_t fColumnIndex) : base((response*)new_base), row_index(fRowIndex), column_index(fColumnIndex)
			{
			}
			core::string column::get_name() const
			{
#ifdef VI_SQLITE
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(column_index != std::numeric_limits<size_t>::max(), "column should be valid");
				return column_index < base->columns.size() ? base->columns[column_index] : core::to_string(column_index);
#else
				return core::string();
#endif
			}
			core::variant column::get() const
			{
				if (!base || row_index == std::numeric_limits<size_t>::max() || column_index == std::numeric_limits<size_t>::max())
					return core::var::undefined();

				return base->values[row_index][column_index];
			}
			core::schema* column::get_inline() const
			{
				if (!base || row_index == std::numeric_limits<size_t>::max() || column_index == std::numeric_limits<size_t>::max())
					return nullptr;

				if (nullable())
					return nullptr;

				auto& column = base->values[row_index][column_index];
				return utils::get_schema_from_value(column);
			}
			size_t column::index() const
			{
				return column_index;
			}
			size_t column::size() const
			{
				if (!base || row_index == std::numeric_limits<size_t>::max() || column_index == std::numeric_limits<size_t>::max())
					return 0;

				return base->values[row_index].size();
			}
			size_t column::raw_size() const
			{
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(column_index != std::numeric_limits<size_t>::max(), "column should be valid");
				return base->values[row_index][column_index].size();
			}
			row column::get_row() const
			{
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");
				return row(base, row_index);
			}
			bool column::nullable() const
			{
				if (!base || row_index == std::numeric_limits<size_t>::max() || column_index == std::numeric_limits<size_t>::max())
					return true;

				auto& column = base->values[row_index][column_index];
				return column.is(core::var_type::null) || column.is(core::var_type::undefined);
			}

			row::row(const response* new_base, size_t fRowIndex) : base((response*)new_base), row_index(fRowIndex)
			{
			}
			core::schema* row::get_object() const
			{
				if (!base || row_index == std::numeric_limits<size_t>::max())
					return nullptr;

				auto& row = base->values[row_index];
				core::schema* result = core::var::set::object();
				result->get_childs().reserve(row.size());
				for (size_t j = 0; j < row.size(); j++)
					result->set(j < base->columns.size() ? base->columns[j] : core::to_string(j), utils::get_schema_from_value(row[j]));

				return result;
			}
			core::schema* row::get_array() const
			{
				if (!base || row_index == std::numeric_limits<size_t>::max())
					return nullptr;

				auto& row = base->values[row_index];
				core::schema* result = core::var::set::array();
				result->get_childs().reserve(row.size());
				for (auto& column : row)
					result->push(utils::get_schema_from_value(column));

				return result;
			}
			size_t row::index() const
			{
				return row_index;
			}
			size_t row::size() const
			{
				if (!base || row_index == std::numeric_limits<size_t>::max())
					return 0;

				if (row_index >= base->values.size())
					return 0;

				auto& row = base->values[row_index];
				return row.size();
			}
			column row::get_column(size_t index) const
			{
				if (!base || row_index == std::numeric_limits<size_t>::max() || index >= base->columns.size())
					return column(base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				if (index >= base->values[row_index].size())
					return column(base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				return column(base, row_index, index);
			}
			column row::get_column(const std::string_view& name) const
			{
				if (!base || row_index == std::numeric_limits<size_t>::max())
					return column(base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				size_t index = base->get_column_index(name);
				if (index >= base->values[row_index].size())
					return column(base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				return column(base, row_index, index);
			}
			bool row::get_columns(column* output, size_t output_size) const
			{
				VI_ASSERT(output != nullptr && output_size > 0, "output should be valid");
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");

				output_size = std::min(size(), output_size);
				for (size_t i = 0; i < output_size; i++)
					output[i] = column(base, row_index, i);

				return true;
			}

			core::schema* response::get_array_of_objects() const
			{
				core::schema* result = core::var::set::array();
				if (values.empty() || columns.empty())
					return result;

				result->reserve(values.size());
				for (auto& row : values)
				{
					core::schema* subresult = core::var::set::object();
					subresult->get_childs().reserve(row.size());
					for (size_t j = 0; j < row.size(); j++)
						subresult->set(j < columns.size() ? columns[j] : core::to_string(j), utils::get_schema_from_value(row[j]));
					result->push(subresult);
				}

				return result;
			}
			core::schema* response::get_array_of_arrays() const
			{
				core::schema* result = core::var::set::array();
				if (values.empty())
					return result;

				result->reserve(values.size());
				for (auto& row : values)
				{
					core::schema* subresult = core::var::set::array();
					subresult->get_childs().reserve(row.size());
					for (auto& column : row)
						subresult->push(utils::get_schema_from_value(column));
					result->push(subresult);
				}

				return result;
			}
			core::schema* response::get_object(size_t index) const
			{
				return get_row(index).get_object();
			}
			core::schema* response::get_array(size_t index) const
			{
				return get_row(index).get_array();
			}
			const core::vector<core::string>& response::get_columns() const
			{
				return columns;
			}
			core::string response::get_status_text() const
			{
				return status_message;
			}
			int response::get_status_code() const
			{
				return status_code;
			}
			size_t response::get_column_index(const std::string_view& name) const
			{
				size_t size = name.size(), index = 0;
				for (auto& next : columns)
				{
					if (!strncmp(next.c_str(), name.data(), std::min(next.size(), size)))
						return index;
					++index;
				}
				return std::numeric_limits<size_t>::max();
			}
			size_t response::affected_rows() const
			{
				return stats.affected_rows;
			}
			size_t response::last_inserted_row_id() const
			{
				return stats.last_inserted_row_id;
			}
			size_t response::size() const
			{
				return values.size();
			}
			row response::get_row(size_t index) const
			{
				if (index >= values.size())
					return row(this, std::numeric_limits<size_t>::max());

				return row(this, index);
			}
			row response::front() const
			{
				if (empty())
					return row(nullptr, std::numeric_limits<size_t>::max());

				return get_row(0);
			}
			row response::back() const
			{
				if (empty())
					return row(nullptr, std::numeric_limits<size_t>::max());

				return get_row(size() - 1);
			}
			response response::copy() const
			{
				return *this;
			}
			bool response::empty() const
			{
				return values.empty();
			}
			bool response::error() const
			{
#ifdef VI_SQLITE
				return !status_message.empty() && status_code != SQLITE_OK;
#else
				return !status_message.empty() && status_code != 0;
#endif
			}

			cursor::cursor() : cursor(nullptr)
			{
			}
			cursor::cursor(tconnection* connection) : executor(connection)
			{
				VI_WATCH(this, "sqlite-result cursor");
			}
			cursor::cursor(cursor&& other) : base(std::move(other.base)), executor(other.executor)
			{
				VI_WATCH(this, "sqlite-result cursor (moved)");
			}
			cursor::~cursor()
			{
				VI_UNWATCH(this);
			}
			cursor& cursor::operator =(cursor&& other)
			{
				if (&other == this)
					return *this;

				base = std::move(other.base);
				executor = other.executor;
				return *this;
			}
			bool cursor::success() const
			{
				return !error();
			}
			bool cursor::empty() const
			{
				if (base.empty())
					return true;

				for (auto& item : base)
				{
					if (!item.empty())
						return false;
				}

				return true;
			}
			bool cursor::error() const
			{
				if (base.empty())
					return true;

				for (auto& item : base)
				{
					if (item.error())
						return true;
				}

				return false;
			}
			size_t cursor::size() const
			{
				return base.size();
			}
			size_t cursor::affected_rows() const
			{
				size_t size = 0;
				for (auto& item : base)
					size += item.affected_rows();
				return size;
			}
			cursor cursor::copy() const
			{
				cursor result(executor);
				if (base.empty())
					return result;

				result.base.clear();
				result.base.reserve(base.size());

				for (auto& item : base)
					result.base.emplace_back(item.copy());

				return result;
			}
			tconnection* cursor::get_executor() const
			{
				return executor;
			}
			const response& cursor::first() const
			{
				VI_ASSERT(!base.empty(), "index outside of range");
				return base.front();
			}
			const response& cursor::last() const
			{
				VI_ASSERT(!base.empty(), "index outside of range");
				return base.back();
			}
			const response& cursor::at(size_t index) const
			{
				VI_ASSERT(index < base.size(), "index outside of range");
				return base[index];
			}
			core::schema* cursor::get_array_of_objects(size_t response_index) const
			{
				if (response_index >= base.size())
					return core::var::set::array();

				return base[response_index].get_array_of_objects();
			}
			core::schema* cursor::get_array_of_arrays(size_t response_index) const
			{
				if (response_index >= base.size())
					return core::var::set::array();

				return base[response_index].get_array_of_arrays();
			}
			core::schema* cursor::get_object(size_t response_index, size_t index) const
			{
				if (response_index >= base.size())
					return nullptr;

				return base[response_index].get_object(index);
			}
			core::schema* cursor::get_array(size_t response_index, size_t index) const
			{
				if (response_index >= base.size())
					return nullptr;

				return base[response_index].get_array(index);
			}

			connection::connection() : handle(nullptr), timeout(0)
			{
				library_handle = driver::get();
				if (library_handle != nullptr)
					library_handle->add_ref();
			}
			connection::~connection() noexcept
			{
#ifdef VI_SQLITE
				for (auto& item : statements)
					sqlite3_finalize(item.second);
				if (handle != nullptr)
				{
					sqlite3_close(handle);
					handle = nullptr;
				}
#endif
				for (auto* item : functions)
					core::memory::deinit(item);
				for (auto* item : aggregates)
					core::memory::release(item);
				for (auto* item : windows)
					core::memory::release(item);
				functions.clear();
				aggregates.clear();
				windows.clear();
				core::memory::release(library_handle);
			}
			void connection::set_wal_autocheckpoint(uint32_t max_frames)
			{
#ifdef VI_SQLITE
				core::umutex<std::mutex> unique(update);
				if (handle != nullptr)
					sqlite3_wal_autocheckpoint(handle, (int)max_frames);
#endif
			}
			void connection::set_soft_heap_limit(uint64_t memory)
			{
#ifdef VI_SQLITE
				core::umutex<std::mutex> unique(update);
				sqlite3_soft_heap_limit64((int64_t)memory);
#endif
			}
			void connection::set_hard_heap_limit(uint64_t memory)
			{
#ifdef VI_SQLITE
#ifndef VI_APPLE
				core::umutex<std::mutex> unique(update);
				sqlite3_hard_heap_limit64((int64_t)memory);
#endif
#endif
			}
			void connection::set_shared_cache(bool enabled)
			{
#ifdef VI_SQLITE
#ifndef VI_APPLE
				core::umutex<std::mutex> unique(update);
				sqlite3_enable_shared_cache((int)enabled);
#endif
#endif
			}
			void connection::set_extensions(bool enabled)
			{
#ifdef VI_SQLITE
#ifndef SQLITE_OMIT_LOAD_EXTENSION
				core::umutex<std::mutex> unique(update);
				if (handle != nullptr)
					sqlite3_enable_load_extension(handle, (int)enabled);
#endif
#endif
			}
			void connection::set_busy_timeout(uint64_t ms)
			{
				timeout = ms;
			}
			void connection::set_function(const std::string_view& name, uint8_t args, on_function_result&& context)
			{
#ifdef VI_SQLITE
				VI_ASSERT(context != nullptr, "callable should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				core::umutex<std::mutex> unique(update);
				on_function_result* callable = core::memory::init<on_function_result>(std::move(context));
				if (handle != nullptr)
					sqlite3_create_function_v2(handle, name.data(), (int)args, SQLITE_UTF8, callable, &sqlite3_util::function_call, nullptr, nullptr, nullptr);
				functions.push_back(callable);
#endif
			}
			void connection::set_aggregate_function(const std::string_view& name, uint8_t args, core::unique<aggregate> context)
			{
#ifdef VI_SQLITE
				VI_ASSERT(context != nullptr, "callable should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				core::umutex<std::mutex> unique(update);
				if (handle != nullptr)
					sqlite3_create_function_v2(handle, name.data(), (int)args, SQLITE_UTF8, context, nullptr, &sqlite3_util::aggregate_step_call, &sqlite3_util::aggregate_finalize_call, nullptr);
				aggregates.push_back(context);
#endif
			}
			void connection::set_window_function(const std::string_view& name, uint8_t args, core::unique<window> context)
			{
#ifdef VI_SQLITE
				VI_ASSERT(context != nullptr, "callable should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				core::umutex<std::mutex> unique(update);
				if (handle != nullptr)
					sqlite3_create_window_function(handle, name.data(), (int)args, SQLITE_UTF8, context, &sqlite3_util::window_step_call, &sqlite3_util::window_finalize_call, &sqlite3_util::window_value_call, &sqlite3_util::window_inverse_call, nullptr);
				windows.push_back(context);
#endif
			}
			void connection::overload_function(const std::string_view& name, uint8_t args)
			{
#ifdef VI_SQLITE
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				core::umutex<std::mutex> unique(update);
				if (handle != nullptr)
					sqlite3_overload_function(handle, name.data(), (int)args);
#endif
			}
			core::vector<checkpoint> connection::wal_checkpoint(checkpoint_mode mode, const std::string_view& database)
			{
				VI_ASSERT(database.empty() || core::stringify::is_cstring(database), "database should be set");
				core::vector<checkpoint> checkpoints;
#ifdef VI_SQLITE
				core::umutex<std::mutex> unique(update);
				if (handle != nullptr)
				{
					int frames_size, frames_count;
					checkpoint result;
					result.status = sqlite3_wal_checkpoint_v2(handle, database.empty() ? nullptr : database.data(), (int)mode, &frames_size, &frames_count);
					result.frames_size = (uint32_t)frames_size;
					result.frames_count = (uint32_t)frames_count;
					result.database = database;
					checkpoints.push_back(std::move(result));
				}
#endif
				return checkpoints;
			}
			size_t connection::free_memory_used(size_t bytes)
			{
#ifdef VI_SQLITE
				int freed = sqlite3_release_memory((int)bytes);
				return freed >= 0 ? (size_t)freed : 0;
#else
				return 0;
#endif
			}
			size_t connection::get_memory_used() const
			{
#ifdef VI_SQLITE
				return (size_t)sqlite3_memory_used();
#else
				return 0;
#endif
			}
			expects_db<void> connection::bind_null(tstatement* statement, size_t index)
			{
				VI_ASSERT(statement != nullptr, "statement should not be empty");
#ifdef VI_SQLITE
				int code = sqlite3_bind_null(statement, (int)index + 1);
				if (code == SQLITE_OK)
					return core::expectation::met;

				return database_exception(core::string(sqlite3_errstr(code)));
#else
				return expects_db<void>(database_exception("bind: not supported"));
#endif
			}
			expects_db<void> connection::bind_pointer(tstatement* statement, size_t index, void* value)
			{
				VI_ASSERT(statement != nullptr, "statement should not be empty");
#ifdef VI_SQLITE
				int code = sqlite3_bind_pointer(statement, (int)index + 1, value, "ptr", SQLITE_STATIC);
				if (code == SQLITE_OK)
					return core::expectation::met;

				return database_exception(core::string(sqlite3_errstr(code)));
#else
				return expects_db<void>(database_exception("bind: not supported"));
#endif
			}
			expects_db<void> connection::bind_string(tstatement* statement, size_t index, const std::string_view& value)
			{
				VI_ASSERT(statement != nullptr, "statement should not be empty");
#ifdef VI_SQLITE
				int code = sqlite3_bind_text64(statement, (int)index + 1, value.data(), (sqlite3_uint64)value.size(), SQLITE_STATIC, SQLITE_UTF8);
				if (code == SQLITE_OK)
					return core::expectation::met;

				return database_exception(core::string(sqlite3_errstr(code)));
#else
				return expects_db<void>(database_exception("bind: not supported"));
#endif
			}
			expects_db<void> connection::bind_blob(tstatement* statement, size_t index, const std::string_view& value)
			{
				VI_ASSERT(statement != nullptr, "statement should not be empty");
#ifdef VI_SQLITE
				int code = sqlite3_bind_blob64(statement, (int)index + 1, value.data(), (sqlite3_uint64)value.size(), SQLITE_STATIC);
				if (code == SQLITE_OK)
					return core::expectation::met;

				return database_exception(core::string(sqlite3_errstr(code)));
#else
				return expects_db<void>(database_exception("bind: not supported"));
#endif
			}
			expects_db<void> connection::bind_boolean(tstatement* statement, size_t index, bool value)
			{
				VI_ASSERT(statement != nullptr, "statement should not be empty");
#ifdef VI_SQLITE
				int code = sqlite3_bind_int(statement, (int)index + 1, value);
				if (code == SQLITE_OK)
					return core::expectation::met;

				return database_exception(core::string(sqlite3_errstr(code)));
#else
				return expects_db<void>(database_exception("bind: not supported"));
#endif
			}
			expects_db<void> connection::bind_int32(tstatement* statement, size_t index, int32_t value)
			{
				VI_ASSERT(statement != nullptr, "statement should not be empty");
#ifdef VI_SQLITE
				int code = sqlite3_bind_int(statement, (int)index + 1, value);
				if (code == SQLITE_OK)
					return core::expectation::met;

				return database_exception(core::string(sqlite3_errstr(code)));
#else
				return expects_db<void>(database_exception("bind: not supported"));
#endif
			}
			expects_db<void> connection::bind_int64(tstatement* statement, size_t index, int64_t value)
			{
				VI_ASSERT(statement != nullptr, "statement should not be empty");
#ifdef VI_SQLITE
				int code = sqlite3_bind_int64(statement, (int)index + 1, value);
				if (code == SQLITE_OK)
					return core::expectation::met;

				return database_exception(core::string(sqlite3_errstr(code)));
#else
				return expects_db<void>(database_exception("bind: not supported"));
#endif
			}
			expects_db<void> connection::bind_double(tstatement* statement, size_t index, double value)
			{
				VI_ASSERT(statement != nullptr, "statement should not be empty");
#ifdef VI_SQLITE
				int code = sqlite3_bind_double(statement, (int)index + 1, value);
				if (code == SQLITE_OK)
					return core::expectation::met;

				return database_exception(core::string(sqlite3_errstr(code)));
#else
				return expects_db<void>(database_exception("bind: not supported"));
#endif
			}
			expects_db<void> connection::bind_decimal(tstatement* statement, size_t index, const core::decimal& value, core::vector<core::string>& temps)
			{
				VI_ASSERT(statement != nullptr, "statement should not be empty");
				core::string numeric = value.to_string();
				if (!value.decimal_places())
				{
					auto integer = core::from_string<int64_t>(numeric);
					if (integer)
						return bind_int64(statement, index, *integer);
				}
				else if (value.decimal_places() <= 3 && value.integer_places() <= 6)
				{
					auto doublef = core::from_string<double>(numeric);
					if (doublef)
						return bind_double(statement, index, *doublef);
				}

				temps.push_back(std::move(numeric));
				return bind_string(statement, index, temps.back());
			}
			expects_db<tstatement*> connection::prepare_statement(const std::string_view& command, std::string_view* leftover_command)
			{
#ifdef VI_SQLITE
				core::umutex<std::mutex> unique(update);
				auto prepared = statements.find(command);
				if (prepared != statements.end())
					return prepared->second;

				const char* trailing_statement = nullptr;
				sqlite3_stmt* target = nullptr;
				int code = sqlite3_prepare_v2(handle, command.data(), (int)command.size(), &target, &trailing_statement);
				size_t length = trailing_statement ? trailing_statement - command.data() : command.size();
				if (leftover_command)
					*leftover_command = command.substr(length);

				if (code != SQLITE_OK)
				{
					int error = sqlite3_errcode(handle);
					auto* message = error == SQLITE_OK ? sqlite3_errstr(code) : sqlite3_errmsg(handle);
					if (target != nullptr)
						sqlite3_finalize(target);
					return database_exception(core::string(message));
				}

				statements[core::string(command.substr(0, length))] = target;
				return target;
#else
				return expects_db<tstatement*>(database_exception("prepare: not supported"));
#endif
			}
			expects_db<session_id> connection::tx_begin(isolation type)
			{
				switch (type)
				{
					case isolation::immediate:
						return tx_start("BEGIN IMMEDIATE TRANSACTION");
					case isolation::exclusive:
						return tx_start("BEGIN EXCLUSIVE TRANSACTION");
					case isolation::deferred:
					default:
						return tx_start("BEGIN TRANSACTION");
				}
			}
			expects_db<session_id> connection::tx_start(const std::string_view& command)
			{
				auto result = query(command, (size_t)query_op::transaction_start);
				if (!result)
					return result.error();
				else if (result->success())
					return result->get_executor();

				return database_exception("transaction start error");
			}
			expects_db<void> connection::tx_end(const std::string_view& command, session_id session)
			{
				auto result = query(command, (size_t)query_op::transaction_end, session);
				if (!result)
					return result.error();
				else if (result->success())
					return core::expectation::met;

				return database_exception("transaction end error");
			}
			expects_db<void> connection::tx_commit(session_id session)
			{
				return tx_end("COMMIT", session);
			}
			expects_db<void> connection::tx_rollback(session_id session)
			{
				return tx_end("ROLLBACK", session);
			}
			expects_db<void> connection::connect(const std::string_view& location)
			{
#ifdef VI_SQLITE
				bool is_in_memory = location == ":memory:";
				if (is_in_memory)
				{
					if (!core::os::control::has(core::access_option::mem))
						return expects_db<void>(database_exception("connect failed: permission denied"));
				}
				else if (!core::os::control::has(core::access_option::fs))
					return expects_db<void>(database_exception("connect failed: permission denied"));
				else if (is_connected())
					disconnect();

				core::umutex<std::mutex> unique(update);
				source = location;
				unique.negate();

				VI_MEASURE(core::timings::intensive);
				VI_DEBUG("[sqlite] try open database %s", source.c_str());

				int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI | SQLITE_OPEN_NOMUTEX;
				if (is_in_memory)
					flags |= SQLITE_OPEN_MEMORY;

				int code = sqlite3_open_v2(source.c_str(), &handle, flags, nullptr);
				if (code != SQLITE_OK)
				{
					database_exception error(handle);
					sqlite3_close(handle);
					handle = nullptr;
					return error;
				}

				VI_DEBUG("[sqlite] OK open database on 0x%" PRIXPTR, (uintptr_t)handle);
				return core::expectation::met;
#else
				return expects_db<void>(database_exception("connect: not supported"));
#endif
			}
			expects_db<void> connection::disconnect()
			{
#ifdef VI_SQLITE
				if (!is_connected())
					return expects_db<void>(database_exception("disconnect: not connected"));

				core::umutex<std::mutex> unique(update);
				if (handle != nullptr)
				{
					sqlite3_close(handle);
					handle = nullptr;
				}
				return core::expectation::met;
#else
				return expects_db<void>(database_exception("disconnect: not supported"));
#endif
			}
			expects_db<void> connection::flush()
			{
#ifdef VI_SQLITE
				if (!is_connected())
					return expects_db<void>(database_exception("flush: not connected"));

				core::umutex<std::mutex> unique(update);
				if (handle != nullptr)
					sqlite3_db_cacheflush(handle);
				return core::expectation::met;
#else
				return expects_db<void>(database_exception("flush: not supported"));
#endif
			}
			expects_db<cursor> connection::prepared_query(tstatement* statement, session_id session)
			{
				VI_ASSERT(statement != nullptr, "statement should not be empty");
#ifdef VI_SQLITE
				if (!handle)
					return expects_db<cursor>(database_exception("acquire connection error: no candidate"));

				auto* log = driver::get();
				if (log->is_log_active())
					log->log_query(core::stringify::text("PREPARED QUERY 0x%" PRIXPTR "%s", (uintptr_t)statement, session ? " (transaction)" : ""));

				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
				VI_MEASURE(core::timings::intensive);

				cursor result(handle);
				result.base.emplace_back();
				auto status = sqlite3_util::execute(handle, statement, result.base.back(), timeout);
				sqlite3_reset(statement);
				if (!status)
					return status.error();

				VI_DEBUG("[sqlite] OK execute on 0x%" PRIXPTR " using statement 0x%" PRIXPTR " (%s%" PRIu64 " ms)", (uintptr_t)handle, (uintptr_t)statement, session ? "transaction, " : "", (uint64_t)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - time).count()); (void)time;
				return expects_db<cursor>(std::move(result));
#else
				return expects_db<cursor>(database_exception("query: not supported"));
#endif
			}
			expects_db<cursor> connection::emplace_query(const std::string_view& command, core::schema_list* map, size_t opts, session_id session)
			{
				auto pattern = driver::get()->emplace(command, map);
				if (!pattern)
					return expects_db<cursor>(pattern.error());

				return query(*pattern, opts, session);
			}
			expects_db<cursor> connection::template_query(const std::string_view& name, core::schema_args* map, size_t opts, session_id session)
			{
				VI_DEBUG("[sqlite] template query %s", name.empty() ? "empty-query-name" : core::string(name).c_str());
				auto pattern = driver::get()->get_query(name, map);
				if (!pattern)
					return expects_db<cursor>(pattern.error());

				return query(*pattern, opts, session);
			}
			expects_db<cursor> connection::query(const std::string_view& command, size_t opts, session_id session)
			{
				VI_ASSERT(!command.empty(), "command should not be empty");
#ifdef VI_SQLITE
				driver::get()->log_query(command);
				if (!handle)
					return expects_db<cursor>(database_exception("acquire connection error: no candidate"));

				auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
				VI_DEBUG("[sqlite] execute query on 0x%" PRIXPTR "%s: %.64s%s", (uintptr_t)handle, session ? " (transaction)" : "", command.data(), command.size() > 64 ? " ..." : "");

				size_t offset = 0;
				cursor result(handle);
				while (offset < command.size())
				{
					VI_MEASURE(core::timings::intensive);
					const char* trailing_statement = nullptr;
					sqlite3_stmt* target = nullptr;
					int code = sqlite3_prepare_v2(handle, command.data() + offset, (int)(command.size() - offset), &target, &trailing_statement);
					offset = (trailing_statement ? trailing_statement - command.data() : command.size());
					result.base.emplace_back();
					if (code != SQLITE_OK)
					{
						int error = sqlite3_errcode(handle);
						auto& response = result.base.back();
						response.status_code = error == SQLITE_OK ? code : error;
						response.status_message = error == SQLITE_OK ? sqlite3_errstr(code) : sqlite3_errmsg(handle);
						if (target != nullptr)
							sqlite3_finalize(target);
						return database_exception(core::string(response.status_message));
					}
					else
					{
						auto status = sqlite3_util::execute(handle, target, result.base.back(), timeout);
						sqlite3_finalize(target);
						if (!status)
							return status.error();
					}
				}

				VI_DEBUG("[sqlite] OK execute on 0x%" PRIXPTR " (%" PRIu64 " ms)", (uintptr_t)handle, (uint64_t)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - time).count()); (void)time;
				return expects_db<cursor>(std::move(result));
#else
				return expects_db<cursor>(database_exception("query: not supported"));
#endif
			}
			tconnection* connection::get_connection()
			{
				return handle;
			}
			const core::string& connection::get_address()
			{
				return source;
			}
			bool connection::is_connected()
			{
				core::umutex<std::mutex> unique(update);
				return handle != nullptr;
			}

			cluster::cluster() : timeout(0)
			{
				library_handle = driver::get();
				if (library_handle != nullptr)
					library_handle->add_ref();
			}
			cluster::~cluster() noexcept
			{
#ifdef VI_SQLITE
				for (auto* item : idle)
					sqlite3_close(item);
				for (auto* item : busy)
				{
					sqlite3_interrupt(item);
					sqlite3_close(item);
				}
#endif
				for (auto* item : functions)
					core::memory::deinit(item);
				for (auto* item : aggregates)
					core::memory::release(item);
				for (auto* item : windows)
					core::memory::release(item);
				idle.clear();
				busy.clear();
				functions.clear();
				aggregates.clear();
				windows.clear();
				core::memory::release(library_handle);
			}
			void cluster::set_wal_autocheckpoint(uint32_t max_frames)
			{
#ifdef VI_SQLITE
				core::umutex<std::mutex> unique(update);
				for (auto* item : idle)
					sqlite3_wal_autocheckpoint(item, (int)max_frames);
#endif
			}
			void cluster::set_soft_heap_limit(uint64_t memory)
			{
#ifdef VI_SQLITE
				core::umutex<std::mutex> unique(update);
				sqlite3_soft_heap_limit64((int64_t)memory);
#endif
			}
			void cluster::set_hard_heap_limit(uint64_t memory)
			{
#ifdef VI_SQLITE
#ifndef VI_APPLE
				core::umutex<std::mutex> unique(update);
				sqlite3_hard_heap_limit64((int64_t)memory);
#endif
#endif
			}
			void cluster::set_shared_cache(bool enabled)
			{
#ifdef VI_SQLITE
#ifndef VI_APPLE
				core::umutex<std::mutex> unique(update);
				sqlite3_enable_shared_cache((int)enabled);
#endif
#endif
			}
			void cluster::set_extensions(bool enabled)
			{
#ifdef VI_SQLITE
#ifndef SQLITE_OMIT_LOAD_EXTENSION
				core::umutex<std::mutex> unique(update);
				for (auto* item : idle)
					sqlite3_enable_load_extension(item, (int)enabled);
#endif
#endif
			}
			void cluster::set_function(const std::string_view& name, uint8_t args, on_function_result&& context)
			{
#ifdef VI_SQLITE
				VI_ASSERT(context != nullptr, "callable should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				core::umutex<std::mutex> unique(update);
				on_function_result* callable = core::memory::init<on_function_result>(std::move(context));
				for (auto* item : idle)
					sqlite3_create_function_v2(item, name.data(), (int)args, SQLITE_UTF8, callable, &sqlite3_util::function_call, nullptr, nullptr, nullptr);
				functions.push_back(callable);
#endif
			}
			void cluster::set_aggregate_function(const std::string_view& name, uint8_t args, core::unique<aggregate> context)
			{
#ifdef VI_SQLITE
				VI_ASSERT(context != nullptr, "callable should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				core::umutex<std::mutex> unique(update);
				for (auto* item : idle)
					sqlite3_create_function_v2(item, name.data(), (int)args, SQLITE_UTF8, context, nullptr, &sqlite3_util::aggregate_step_call, &sqlite3_util::aggregate_finalize_call, nullptr);
				aggregates.push_back(context);
#endif
			}
			void cluster::set_window_function(const std::string_view& name, uint8_t args, core::unique<window> context)
			{
#ifdef VI_SQLITE
				VI_ASSERT(context != nullptr, "callable should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				core::umutex<std::mutex> unique(update);
				for (auto* item : idle)
					sqlite3_create_window_function(item, name.data(), (int)args, SQLITE_UTF8, context, &sqlite3_util::window_step_call, &sqlite3_util::window_finalize_call, &sqlite3_util::window_value_call, &sqlite3_util::window_inverse_call, nullptr);
				windows.push_back(context);
#endif
			}
			void cluster::overload_function(const std::string_view& name, uint8_t args)
			{
#ifdef VI_SQLITE
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				core::umutex<std::mutex> unique(update);
				for (auto* item : idle)
					sqlite3_overload_function(item, name.data(), (int)args);
#endif
			}
			core::vector<checkpoint> cluster::wal_checkpoint(checkpoint_mode mode, const std::string_view& database)
			{
				VI_ASSERT(database.empty() || core::stringify::is_cstring(database), "database should be set");
				core::vector<checkpoint> checkpoints;
#ifdef VI_SQLITE
				core::umutex<std::mutex> unique(update);
				for (auto* item : idle)
				{
					int frames_size, frames_count;
					checkpoint result;
					result.status = sqlite3_wal_checkpoint_v2(item, database.empty() ? nullptr : database.data(), (int)mode, &frames_size, &frames_count);
					result.frames_size = (uint32_t)frames_size;
					result.frames_count = (uint32_t)frames_count;
					result.database = database;
					checkpoints.push_back(std::move(result));
				}
#endif
				return checkpoints;
			}
			size_t cluster::free_memory_used(size_t bytes)
			{
#ifdef VI_SQLITE
				int freed = sqlite3_release_memory((int)bytes);
				return freed >= 0 ? (size_t)freed : 0;
#else
				return 0;
#endif
			}
			size_t cluster::get_memory_used() const
			{
#ifdef VI_SQLITE
				return (size_t)sqlite3_memory_used();
#else
				return 0;
#endif
			}
			expects_promise_db<session_id> cluster::tx_begin(isolation type)
			{
				switch (type)
				{
					case isolation::immediate:
						return tx_start("BEGIN IMMEDIATE TRANSACTION");
					case isolation::exclusive:
						return tx_start("BEGIN EXCLUSIVE TRANSACTION");
					case isolation::deferred:
					default:
						return tx_start("BEGIN TRANSACTION");
				}
			}
			expects_promise_db<session_id> cluster::tx_start(const std::string_view& command)
			{
				return query(command, (size_t)query_op::transaction_start).then<expects_db<session_id>>([](expects_db<cursor>&& result) -> expects_db<session_id>
				{
					if (!result)
						return result.error();
					else if (result->success())
						return result->get_executor();

					return database_exception("transaction start error");
				});
			}
			expects_promise_db<void> cluster::tx_end(const std::string_view& command, session_id session)
			{
				return query(command, (size_t)query_op::transaction_end, session).then<expects_db<void>>([](expects_db<cursor>&& result) -> expects_db<void>
				{
					if (!result)
						return result.error();
					else if (result->success())
						return core::expectation::met;

					return database_exception("transaction end error");
				});
			}
			expects_promise_db<void> cluster::tx_commit(session_id session)
			{
				return tx_end("COMMIT", session);
			}
			expects_promise_db<void> cluster::tx_rollback(session_id session)
			{
				return tx_end("ROLLBACK", session);
			}
			expects_promise_db<void> cluster::connect(const std::string_view& location, size_t connections)
			{
#ifdef VI_SQLITE
				VI_ASSERT(connections > 0, "connections count should be at least 1");
				bool is_in_memory = location == ":memory:";
				if (is_in_memory)
				{
					if (!core::os::control::has(core::access_option::mem))
						return expects_promise_db<void>(database_exception("connect failed: permission denied"));
				}
				else if (!core::os::control::has(core::access_option::fs))
					return expects_promise_db<void>(database_exception("connect failed: permission denied"));

				if (is_connected())
				{
					auto copy = core::string(location);
					return disconnect().then<expects_promise_db<void>>([this, copy = std::move(copy), connections](expects_db<void>&&) { return this->connect(copy, connections); });
				}

				core::umutex<std::mutex> unique(update);
				source = location;
				unique.negate();

				return core::cotask<expects_db<void>>([this, is_in_memory, connections]() -> expects_db<void>
				{
					VI_MEASURE(core::timings::intensive);
					VI_DEBUG("[sqlite] try open database using %i connections", (int)connections);

					int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI | SQLITE_OPEN_NOMUTEX;
					if (is_in_memory)
						flags |= SQLITE_OPEN_MEMORY;

					core::umutex<std::mutex> unique(update);
					idle.reserve(connections);
					busy.reserve(connections);

					for (size_t i = 0; i < connections; i++)
					{
						VI_DEBUG("[sqlite] try connect to %s", source.c_str());
						tconnection* connection = nullptr;
						int code = sqlite3_open_v2(source.c_str(), &connection, flags, nullptr);
						if (code != SQLITE_OK)
						{
							database_exception error(connection);
							sqlite3_close(connection);
							connection = nullptr;
							return error;
						}

						VI_DEBUG("[sqlite] OK open database on 0x%" PRIXPTR, (uintptr_t)connection);
						idle.insert(connection);
					}

					return core::expectation::met;
				});
#else
				return expects_promise_db<void>(database_exception("connect: not supported"));
#endif
			}
			expects_promise_db<void> cluster::disconnect()
			{
#ifdef VI_SQLITE
				if (!is_connected())
					return expects_promise_db<void>(database_exception("disconnect: not connected"));

				return core::cotask<expects_db<void>>([this]() -> expects_db<void>
				{
					core::umutex<std::mutex> unique(update);
					for (auto* item : idle)
						sqlite3_close(item);
					for (auto* item : busy)
					{
						sqlite3_interrupt(item);
						sqlite3_close(item);
					}

					idle.clear();
					busy.clear();
					return core::expectation::met;
				});
#else
				return expects_promise_db<void>(database_exception("disconnect: not supported"));
#endif
			}
			expects_promise_db<void> cluster::flush()
			{
#ifdef VI_SQLITE
				if (!is_connected())
					return expects_promise_db<void>(database_exception("flush: not connected"));

				return core::cotask<expects_db<void>>([this]() -> expects_db<void>
				{
					core::umutex<std::mutex> unique(update);
					for (auto* item : idle)
						sqlite3_db_cacheflush(item);
					return core::expectation::met;
				});
#else
				return expects_promise_db<void>(database_exception("flush: not supported"));
#endif
			}
			expects_promise_db<cursor> cluster::emplace_query(const std::string_view& command, core::schema_list* map, size_t opts, session_id session)
			{
				auto pattern = driver::get()->emplace(command, map);
				if (!pattern)
					return expects_promise_db<cursor>(pattern.error());

				return query(*pattern, opts, session);
			}
			expects_promise_db<cursor> cluster::template_query(const std::string_view& name, core::schema_args* map, size_t opts, session_id session)
			{
				VI_DEBUG("[sqlite] template query %s", name.empty() ? "empty-query-name" : core::string(name).c_str());
				auto pattern = driver::get()->get_query(name, map);
				if (!pattern)
					return expects_promise_db<cursor>(pattern.error());

				return query(*pattern, opts, session);
			}
			expects_promise_db<cursor> cluster::query(const std::string_view& command, size_t opts, session_id session)
			{
				VI_ASSERT(!command.empty(), "command should not be empty");
#ifdef VI_SQLITE
				core::string copy = core::string(command);
				driver::get()->log_query(command);
				return core::coasync<expects_db<cursor>>([this, copy = std::move(copy), opts, session]() mutable -> expects_promise_db<cursor>
				{
					std::string_view command = copy;
					tconnection* connection = VI_AWAIT(acquire_connection(session, opts));
					if (!connection)
						coreturn expects_db<cursor>(database_exception("acquire connection error: no candidate"));

					auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
					VI_DEBUG("[sqlite] execute query on 0x%" PRIXPTR "%s: %.64s%s", (uintptr_t)connection, session ? " (transaction)" : "", command.data(), command.size() > 64 ? " ..." : "");

					size_t queries = 0, offset = 0;
					cursor result(connection);
					while (offset < command.size())
					{
						VI_MEASURE(core::timings::intensive);
						const char* trailing_statement = nullptr;
						sqlite3_stmt* target = nullptr;
						int code = sqlite3_prepare_v2(connection, command.data() + offset, (int)(command.size() - offset), &target, &trailing_statement);
						offset = (trailing_statement ? trailing_statement - command.data() : command.size());
						result.base.emplace_back();
						if (code != SQLITE_OK)
						{
							int error = sqlite3_errcode(connection);
							auto& response = result.base.back();
							response.status_code = error == SQLITE_OK ? code : error;
							response.status_message = error == SQLITE_OK ? sqlite3_errstr(code) : sqlite3_errmsg(connection);
							if (target != nullptr)
								sqlite3_finalize(target);
							coreturn expects_db<cursor>(database_exception(core::string(response.status_message)));
						}

						VI_MEASURE(core::timings::intensive);
						if (++queries > 1)
						{
							if (!VI_AWAIT(core::cotask<expects_db<void>>(std::bind(&sqlite3_util::execute, connection, target, result.base.back(), timeout))))
							{
								sqlite3_finalize(target);
								release_connection(connection, opts);
								coreturn expects_db<cursor>(std::move(result));
							}
						}
						else if (!sqlite3_util::execute(connection, target, result.base.back(), timeout))
						{
							sqlite3_finalize(target);
							release_connection(connection, opts);
							coreturn expects_db<cursor>(std::move(result));
						}
						sqlite3_finalize(target);
					}

					VI_DEBUG("[sqlite] OK execute on 0x%" PRIXPTR " (%" PRIu64 " ms)", (uintptr_t)connection, (uint64_t)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - time).count());
					release_connection(connection, opts); (void)time;
					coreturn expects_db<cursor>(std::move(result));
				});
#else
				return expects_promise_db<cursor>(database_exception("query: not supported"));
#endif
			}
			tconnection* cluster::try_acquire_connection(session_id session, size_t opts)
			{
#ifdef VI_SQLITE
				auto it = idle.begin();
				while (it != idle.end())
				{
					bool is_in_transaction = (!sqlite3_get_autocommit(*it));
					if (session == *it)
					{
						if (!is_in_transaction)
							return nullptr;
						break;
					}
					else if (!is_in_transaction && !(opts & (size_t)query_op::transaction_end))
					{
						if (opts & (size_t)query_op::transaction_start)
							VI_DEBUG("[sqlite] acquire transaction on 0x%" PRIXPTR, (uintptr_t)*it);
						break;
					}
					else if (is_in_transaction && !(opts & (size_t)query_op::transaction_start))
						break;
					++it;
				}
				if (it == idle.end())
					return nullptr;

				tconnection* connection = *it;
				busy.insert(connection);
				idle.erase(it);
				return connection;
#else
				return nullptr;
#endif
			}
			core::promise<tconnection*> cluster::acquire_connection(session_id session, size_t opts)
			{
				core::umutex<std::mutex> unique(update);
				tconnection* connection = try_acquire_connection(session, opts);
				if (connection != nullptr)
					return core::promise<tconnection*>(connection);

				request target;
				target.session = session;
				target.opts = opts;
				queues[session].push(target);
				return target.future;
			}
			void cluster::release_connection(tconnection* connection, size_t opts)
			{
				core::umutex<std::mutex> unique(update);
				if (opts & (size_t)query_op::transaction_end)
					VI_DEBUG("[sqlite] release transaction on 0x%" PRIXPTR, (uintptr_t)connection);
				busy.erase(connection);
				idle.insert(connection);

				auto it = queues.find(connection);
				if (it == queues.end() || it->second.empty())
					it = queues.find(nullptr);

				if (it == queues.end() || it->second.empty())
					return;

				request& target = it->second.front();
				tconnection* new_connection = try_acquire_connection(target.session, target.opts);
				if (!new_connection)
					return;

				auto future = std::move(target.future);
				it->second.pop();
				unique.negate();
				future.set(new_connection);
			}
			tconnection* cluster::get_idle_connection()
			{
				core::umutex<std::mutex> unique(update);
				if (!idle.empty())
					return *idle.begin();
				return nullptr;
			}
			tconnection* cluster::get_busy_connection()
			{
				core::umutex<std::mutex> unique(update);
				if (!busy.empty())
					return *busy.begin();
				return nullptr;
			}
			tconnection* cluster::get_any_connection()
			{
				core::umutex<std::mutex> unique(update);
				if (!idle.empty())
					return *idle.begin();
				else if (!busy.empty())
					return *busy.begin();
				return nullptr;
			}
			const core::string& cluster::get_address()
			{
				return source;
			}
			bool cluster::is_connected()
			{
				core::umutex<std::mutex> unique(update);
				return !idle.empty() || !busy.empty();
			}

			expects_db<core::string> utils::inline_array(core::uptr<core::schema>&& array)
			{
				VI_ASSERT(array, "array should be set");
				core::schema_list map;
				core::string def;

				for (auto* item : array->get_childs())
				{
					if (item->value.is_object())
					{
						if (item->empty())
							continue;

						def += '(';
						for (auto* sub : item->get_childs())
						{
							sub->add_ref();
							map.emplace_back(sub);
							def += "?,";
						}
						def.erase(def.end() - 1);
						def += "),";
					}
					else
					{
						item->add_ref();
						map.emplace_back(item);
						def += "?,";
					}
				}

				auto result = sqlite::driver::get()->emplace(def, &map);
				if (result && !result->empty() && result->back() == ',')
					result->erase(result->end() - 1);

				return result;
			}
			expects_db<core::string> utils::inline_query(core::uptr<core::schema>&& where, const core::unordered_map<core::string, core::string>& whitelist, const std::string_view& placeholder)
			{
				VI_ASSERT(where, "array should be set");
				core::schema_list map;
				core::string allow = "abcdefghijklmnopqrstuvwxyz._", def;
				for (auto* statement : where->get_childs())
				{
					core::string op = statement->value.get_blob();
					if (op == "=" || op == "<>" || op == "<=" || op == "<" || op == ">" || op == ">=" || op == "+" || op == "-" || op == "*" || op == "/" || op == "(" || op == ")" || op == "TRUE" || op == "FALSE")
						def += op;
					else if (op == "~==")
						def += " LIKE ";
					else if (op == "~=")
						def += " ILIKE ";
					else if (op == "[")
						def += " ANY(";
					else if (op == "]")
						def += ")";
					else if (op == "&")
						def += " AND ";
					else if (op == "|")
						def += " OR ";
					else if (op == "TRUE")
						def += " TRUE ";
					else if (op == "FALSE")
						def += " FALSE ";
					else if (!op.empty())
					{
						if (op.front() == '@')
						{
							op = op.substr(1);
							if (!whitelist.empty())
							{
								auto it = whitelist.find(op);
								if (it != whitelist.end() && op.find_first_not_of(allow) == core::string::npos)
									def += it->second.empty() ? op : it->second;
							}
							else if (op.find_first_not_of(allow) == core::string::npos)
								def += op;
						}
						else if (!core::stringify::has_number(op))
						{
							statement->add_ref();
							map.push_back(statement);
							def += "?";
						}
						else
							def += op;
					}
				}

				auto result = sqlite::driver::get()->emplace(def, &map);
				if (result && result->empty())
					result = core::string(placeholder);

				return result;
			}
			core::string utils::get_char_array(const std::string_view& src) noexcept
			{
				if (src.empty())
					return "''";

				core::string dest(src);
				core::stringify::replace(dest, "'", "''");
				dest.insert(dest.begin(), '\'');
				dest.append(1, '\'');
				return dest;
			}
			core::string utils::get_byte_array(const std::string_view& src) noexcept
			{
				if (src.empty())
					return "x''";

				return "x'" + compute::codec::hex_encode(src) + "'";
			}
			core::string utils::get_sql(core::schema* source, bool escape, bool negate) noexcept
			{
				if (!source)
					return "NULL";

				core::schema* parent = source->get_parent();
				switch (source->value.get_type())
				{
					case core::var_type::object:
					{
						core::string result;
						core::schema::convert_to_json(source, [&result](core::var_form, const std::string_view& buffer) { result.append(buffer); });
						return escape ? get_char_array(result) : result;
					}
					case core::var_type::array:
					{
						core::string result = (parent && parent->value.get_type() == core::var_type::array ? "[" : "ARRAY[");
						for (auto* node : source->get_childs())
							result.append(get_sql(node, escape, negate)).append(1, ',');

						if (!source->empty())
							result = result.substr(0, result.size() - 1);

						return result + "]";
					}
					case core::var_type::string:
					{
						if (escape)
							return get_char_array(source->value.get_blob());

						return source->value.get_blob();
					}
					case core::var_type::integer:
						return core::to_string(negate ? -source->value.get_integer() : source->value.get_integer());
					case core::var_type::number:
						return core::to_string(negate ? -source->value.get_number() : source->value.get_number());
					case core::var_type::boolean:
						return (negate ? !source->value.get_boolean() : source->value.get_boolean()) ? "TRUE" : "FALSE";
					case core::var_type::decimal:
					{
						core::decimal value = source->value.get_decimal();
						if (value.is_nan())
							return "NULL";

						core::string result = (negate ? '-' + value.to_string() : value.to_string());
						return result.find('.') != core::string::npos ? result : result + ".0";
					}
					case core::var_type::binary:
						return get_byte_array(source->value.get_string());
					case core::var_type::null:
					case core::var_type::undefined:
						return "NULL";
					default:
						break;
				}

				return "NULL";
			}
			core::schema* utils::get_schema_from_value(const core::variant& value) noexcept
			{
				if (!value.is(core::var_type::string))
					return new core::schema(value);

				auto data = core::schema::from_json(value.get_blob());
				return data ? *data : new core::schema(value);
			}
			core::variant_list utils::context_args(tvalue** values, size_t values_count) noexcept
			{
				core::variant_list args;
#ifdef VI_SQLITE
				args.reserve(values_count);
				for (size_t i = 0; i < values_count; i++)
				{
					tvalue* value = values[i];
					switch (sqlite3_value_type(value))
					{
						case SQLITE_INTEGER:
							args.push_back(core::var::integer(sqlite3_value_int64(value)));
							break;
						case SQLITE_FLOAT:
							args.push_back(core::var::number(sqlite3_value_double(value)));
							break;
						case SQLITE_TEXT:
						{
							std::string_view text((const char*)sqlite3_value_text(value), (size_t)sqlite3_value_bytes(value));
							if (core::stringify::has_decimal(text))
								args.push_back(core::var::decimal_string(text));
							else if (core::stringify::has_integer(text))
								args.push_back(core::var::integer(core::from_string<int64_t>(text).otherwise(0)));
							else if (core::stringify::has_number(text))
								args.push_back(core::var::number(core::from_string<double>(text).otherwise(0.0)));
							else
								args.push_back(core::var::string(text));
							break;
						}
						case SQLITE_BLOB:
						{
							const uint8_t* blob = (const uint8_t*)sqlite3_value_blob(value);
							size_t blob_size = (size_t)sqlite3_value_bytes(value);
							args.push_back(core::var::binary(blob ? blob : (uint8_t*)"", blob_size));
							break;
						}
						case SQLITE_NULL:
							args.push_back(core::var::null());
							break;
						default:
							args.push_back(core::var::undefined());
							break;
					}
				}
#endif
				return args;
			}
			void utils::context_return(tcontext* context, const core::variant& result) noexcept
			{
#ifdef VI_SQLITE
				switch (result.get_type())
				{
					case core::var_type::string:
						sqlite3_result_text64(context, result.get_string().data(), (uint64_t)result.size(), SQLITE_TRANSIENT, SQLITE_UTF8);
						break;
					case core::var_type::binary:
						sqlite3_result_blob64(context, result.get_string().data(), (uint64_t)result.size(), SQLITE_TRANSIENT);
						break;
					case core::var_type::integer:
						sqlite3_result_int64(context, result.get_integer());
						break;
					case core::var_type::number:
						sqlite3_result_double(context, result.get_number());
						break;
					case core::var_type::decimal:
					{
						auto numeric = result.get_blob();
						sqlite3_result_text64(context, numeric.c_str(), (uint64_t)numeric.size(), SQLITE_TRANSIENT, SQLITE_UTF8);
						break;
					}
					case core::var_type::boolean:
						sqlite3_result_int(context, (int)result.get_boolean());
						break;
					case core::var_type::null:
					case core::var_type::undefined:
					case core::var_type::object:
					case core::var_type::array:
					case core::var_type::pointer:
					default:
						sqlite3_result_null(context);
						break;
				}
#endif
			}

			driver::driver() noexcept : active(false), logger(nullptr)
			{
#ifdef VI_SQLITE
				sqlite3_initialize();
#endif
			}
			driver::~driver() noexcept
			{
#ifdef VI_SQLITE
				sqlite3_shutdown();
#endif
			}
			void driver::set_query_log(const on_query_log& callback) noexcept
			{
				logger = callback;
			}
			void driver::log_query(const std::string_view& command) noexcept
			{
				if (logger)
					logger(command);
			}
			void driver::add_constant(const std::string_view& name, const std::string_view& value)
			{
				VI_ASSERT(!name.empty(), "name should not be empty");
				core::umutex<std::mutex> unique(exclusive);
				constants[core::string(name)] = value;
			}
			expects_db<void> driver::add_query(const std::string_view& name, const std::string_view& buffer)
			{
				VI_ASSERT(!name.empty(), "name should not be empty");
				if (buffer.empty())
					return database_exception("import empty query error: " + core::string(name));

				sequence result;
				result.request.assign(buffer);

				core::string lines = "\r\n";
				core::string enums = " \r\n\t\'\"()<>=%&^*/+-,!?:;";
				core::string erasable = " \r\n\t\'\"()<>=%&^*/+-,.!?:;";
				core::string quotes = "\"'`";

				core::string& base = result.request;
				core::stringify::replace_in_between(base, "/*", "*/", "", false);
				core::stringify::replace_starts_with_ends_of(base, "--", lines.c_str(), "");
				core::stringify::trim(base);
				core::stringify::compress(base, erasable.c_str(), quotes.c_str());

				auto enumerations = core::stringify::find_starts_with_ends_of(base, "#", enums.c_str(), quotes.c_str());
				if (!enumerations.empty())
				{
					int64_t offset = 0;
					core::umutex<std::mutex> unique(exclusive);
					for (auto& item : enumerations)
					{
						size_t size = item.second.end - item.second.start;
						item.second.start = (size_t)((int64_t)item.second.start + offset);
						item.second.end = (size_t)((int64_t)item.second.end + offset);

						auto it = constants.find(item.first);
						if (it == constants.end())
							return database_exception("query expects @" + item.first + " constant: " + core::string(name));

						core::stringify::replace_part(base, item.second.start, item.second.end, it->second);
						size_t new_size = it->second.size();
						offset += (int64_t)new_size - (int64_t)size;
						item.second.end = item.second.start + new_size;
					}
				}

				core::vector<std::pair<core::string, core::text_settle>> variables;
				for (auto& item : core::stringify::find_in_between(base, "$<", ">", quotes.c_str()))
				{
					item.first += ";escape";
					if (core::stringify::is_preceded_by(base, item.second.start, "-"))
					{
						item.first += ";negate";
						--item.second.start;
					}

					variables.emplace_back(std::move(item));
				}

				for (auto& item : core::stringify::find_in_between(base, "@<", ">", quotes.c_str()))
				{
					item.first += ";unsafe";
					if (core::stringify::is_preceded_by(base, item.second.start, "-"))
					{
						item.first += ";negate";
						--item.second.start;
					}

					variables.emplace_back(std::move(item));
				}

				core::stringify::replace_parts(base, variables, "", [&erasable](const std::string_view& name, char left, int side)
				{
					if (side < 0 && name.find(";negate") != core::string::npos)
						return '\0';

					return erasable.find(left) == core::string::npos ? ' ' : '\0';
				});

				for (auto& item : variables)
				{
					pose position;
					position.negate = item.first.find(";negate") != core::string::npos;
					position.escape = item.first.find(";escape") != core::string::npos;
					position.offset = item.second.start;
					position.key = item.first.substr(0, item.first.find(';'));
					result.positions.emplace_back(std::move(position));
				}

				if (variables.empty())
					result.cache = result.request;

				core::umutex<std::mutex> unique(exclusive);
				queries[core::string(name)] = std::move(result);
				return core::expectation::met;
			}
			expects_db<void> driver::add_directory(const std::string_view& directory, const std::string_view& origin)
			{
				core::vector<std::pair<core::string, core::file_entry>> entries;
				if (!core::os::directory::scan(directory, entries))
					return database_exception("import directory scan error: " + core::string(directory));

				core::string path = core::string(directory);
				if (path.back() != '/' && path.back() != '\\')
					path.append(1, '/');

				size_t size = 0;
				for (auto& file : entries)
				{
					core::string base(path + file.first);
					if (file.second.is_directory)
					{
						auto status = add_directory(base, origin.empty() ? directory : origin);
						if (!status)
							return status;
						else
							continue;
					}

					if (!core::stringify::ends_with(base, ".sql"))
						continue;

					auto buffer = core::os::file::read_all(base, &size);
					if (!buffer)
						continue;

					core::stringify::replace(base, origin.empty() ? directory : origin, "");
					core::stringify::replace(base, "\\", "/");
					core::stringify::replace(base, ".sql", "");
					if (core::stringify::starts_of(base, "\\/"))
						base.erase(0, 1);

					auto status = add_query(base, std::string_view((char*)*buffer, size));
					core::memory::deallocate(*buffer);
					if (!status)
						return status;
				}

				return core::expectation::met;
			}
			bool driver::remove_constant(const std::string_view& name) noexcept
			{
				core::umutex<std::mutex> unique(exclusive);
				auto it = constants.find(core::key_lookup_cast(name));
				if (it == constants.end())
					return false;

				constants.erase(it);
				return true;
			}
			bool driver::remove_query(const std::string_view& name) noexcept
			{
				core::umutex<std::mutex> unique(exclusive);
				auto it = queries.find(core::key_lookup_cast(name));
				if (it == queries.end())
					return false;

				queries.erase(it);
				return true;
			}
			bool driver::load_cache_dump(core::schema* dump) noexcept
			{
				VI_ASSERT(dump != nullptr, "dump should be set");
				size_t count = 0;
				core::umutex<std::mutex> unique(exclusive);
				queries.clear();

				for (auto* data : dump->get_childs())
				{
					sequence result;
					result.cache = data->get_var("cache").get_blob();
					result.request = data->get_var("request").get_blob();

					if (result.request.empty())
						result.request = result.cache;

					core::schema* positions = data->get("positions");
					if (positions != nullptr)
					{
						for (auto* position : positions->get_childs())
						{
							pose next;
							next.key = position->get_var(0).get_blob();
							next.offset = (size_t)position->get_var(1).get_integer();
							next.escape = position->get_var(2).get_boolean();
							next.negate = position->get_var(3).get_boolean();
							result.positions.emplace_back(std::move(next));
						}
					}

					core::string name = data->get_var("name").get_blob();
					queries[name] = std::move(result);
					++count;
				}

				if (count > 0)
					VI_DEBUG("[sqlite] OK load %" PRIu64 " parsed query templates", (uint64_t)count);

				return count > 0;
			}
			core::schema* driver::get_cache_dump() noexcept
			{
				core::umutex<std::mutex> unique(exclusive);
				core::schema* result = core::var::set::array();
				for (auto& query : queries)
				{
					core::schema* data = result->push(core::var::set::object());
					data->set("name", core::var::string(query.first));

					if (query.second.cache.empty())
						data->set("request", core::var::string(query.second.request));
					else
						data->set("cache", core::var::string(query.second.cache));

					auto* positions = data->set("positions", core::var::set::array());
					for (auto& position : query.second.positions)
					{
						auto* next = positions->push(core::var::set::array());
						next->push(core::var::string(position.key));
						next->push(core::var::integer(position.offset));
						next->push(core::var::boolean(position.escape));
						next->push(core::var::boolean(position.negate));
					}
				}

				VI_DEBUG("[sqlite] OK save %" PRIu64 " parsed query templates", (uint64_t)queries.size());
				return result;
			}
			expects_db<core::string> driver::emplace(const std::string_view& SQL, core::schema_list* map) noexcept
			{
				if (!map || map->empty())
					return core::string(SQL);

				core::string buffer(SQL);
				core::text_settle set;
				core::string& src = buffer;
				size_t offset = 0;
				size_t next = 0;

				while ((set = core::stringify::find(buffer, '?', offset)).found)
				{
					if (next >= map->size())
						return database_exception("query expects at least " + core::to_string(next + 1) + " arguments: " + core::string(buffer.substr(set.start, 64)));

					bool escape = true, negate = false;
					if (set.start > 0)
					{
						if (src[set.start - 1] == '\\')
						{
							offset = set.start;
							buffer.erase(set.start - 1, 1);
							continue;
						}
						else if (src[set.start - 1] == '$')
						{
							if (set.start > 1 && src[set.start - 2] == '-')
							{
								negate = true;
								set.start--;
							}

							escape = false;
							set.start--;
						}
						else if (src[set.start - 1] == '-')
						{
							negate = true;
							set.start--;
						}
					}
					core::string value = utils::get_sql(*(*map)[next++], escape, negate);
					buffer.erase(set.start, (escape ? 1 : 2));
					buffer.insert(set.start, value);
					offset = set.start + value.size();
				}

				return src;
			}
			expects_db<core::string> driver::get_query(const std::string_view& name, core::schema_args* map) noexcept
			{
				core::umutex<std::mutex> unique(exclusive);
				auto it = queries.find(core::key_lookup_cast(name));
				if (it == queries.end())
					return database_exception("query not found: " + core::string(name));

				if (!it->second.cache.empty())
					return it->second.cache;

				if (!map || map->empty())
					return it->second.request;

				sequence origin = it->second;
				size_t offset = 0;
				unique.negate();

				core::string& result = origin.request;
				for (auto& word : origin.positions)
				{
					auto it = map->find(word.key);
					if (it == map->end())
						return database_exception("query expects @" + word.key + " constant: " + core::string(name));

					core::string value = utils::get_sql(*it->second, word.escape, word.negate);
					if (value.empty())
						continue;

					result.insert(word.offset + offset, value);
					offset += value.size();
				}

				if (result.empty())
					return database_exception("query construction error: " + core::string(name));

				return result;
			}
			core::vector<core::string> driver::get_queries() noexcept
			{
				core::vector<core::string> result;
				core::umutex<std::mutex> unique(exclusive);
				result.reserve(queries.size());
				for (auto& item : queries)
					result.push_back(item.first);

				return result;
			}
			bool driver::is_log_active() const noexcept
			{
				return !!logger;
			}
		}
	}
}
