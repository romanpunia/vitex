#ifndef VI_NETWORK_SQLITE_H
#define VI_NETWORK_SQLITE_H
#include "../network.h"

struct sqlite3;
struct sqlite3_stmt;
struct sqlite3_context;
struct sqlite3_value;

namespace vitex
{
	namespace network
	{
		namespace sqlite
		{
			class row;

			class response;

			class cursor;

			class connection;

			class cluster;

			class driver;

			typedef int collation_comparator(void* user_data, int n1, const void* s1, int n2, const void* s2);
			typedef std::function<core::variant(const core::variant_list&)> on_function_result;
			typedef std::function<void(const std::string_view&)> on_query_log;
			typedef std::function<void(cursor&)> on_result;
			typedef sqlite3 tconnection;
			typedef sqlite3_stmt tstatement;
			typedef sqlite3_context tcontext;
			typedef sqlite3_value tvalue;
			typedef tconnection* session_id;

			enum class text_representation
			{
				utf8,
				utf16le,
				utf16be,
				utf16,
				utf16_aligned
			};

			enum class isolation
			{
				deferred,
				immediate,
				exclusive,
				default_isolation = deferred
			};

			enum class query_op
			{
				delete_args = 0,
				transaction_start = (1 << 0),
				transaction_end = (1 << 1)
			};

			enum class checkpoint_mode
			{
				passive = 0,
				full = 1,
				restart = 2,
				truncate = 3
			};

			inline size_t operator |(query_op a, query_op b)
			{
				return static_cast<size_t>(static_cast<size_t>(a) | static_cast<size_t>(b));
			}

			struct checkpoint
			{
				core::string database;
				uint32_t frames_size = 0;
				uint32_t frames_count = 0;
				int32_t status = -1;
			};

			class database_exception final : public core::basic_exception
			{
			public:
				database_exception(tconnection* connection);
				database_exception(core::string&& message);
				const char* type() const noexcept override;
			};

			template <typename v>
			using expects_db = core::expects<v, database_exception>;

			template <typename t, typename executor = core::parallel_executor>
			using expects_promise_db = core::basic_promise<expects_db<t>, executor>;

			class aggregate : public core::reference<aggregate>
			{
			public:
				virtual ~aggregate() = default;
				virtual void step(const core::variant_list& args) = 0;
				virtual core::variant finalize() = 0;
			};

			class window : public core::reference<window>
			{
			public:
				virtual ~window() = default;
				virtual void step(const core::variant_list& args) = 0;
				virtual void inverse(const core::variant_list& args) = 0;
				virtual core::variant value() = 0;
				virtual core::variant finalize() = 0;
			};

			class column
			{
				friend row;

			private:
				response* base;
				size_t row_index;
				size_t column_index;

			private:
				column(const response* new_base, size_t fRowIndex, size_t fColumnIndex);

			public:
				core::string get_name() const;
				core::variant get() const;
				core::schema* get_inline() const;
				size_t index() const;
				size_t size() const;
				size_t raw_size() const;
				row get_row() const;
				bool nullable() const;
				bool exists() const
				{
					return base != nullptr;
				}
			};

			class row
			{
				friend column;
				friend response;

			private:
				response* base;
				size_t row_index;

			private:
				row(const response* new_base, size_t fRowIndex);

			public:
				core::schema* get_object() const;
				core::schema* get_array() const;
				size_t index() const;
				size_t size() const;
				column get_column(size_t index) const;
				column get_column(const std::string_view& name) const;
				bool get_columns(column* output, size_t size) const;
				bool exists() const
				{
					return base != nullptr;
				}
				column operator [](const std::string_view& name)
				{
					return get_column(name);
				}
				column operator [](const std::string_view& name) const
				{
					return get_column(name);
				}
			};

			class response
			{
				friend class sqlite3_util;
				friend connection;
				friend cluster;
				friend row;
				friend column;

			public:
				struct iterator
				{
					const response* source;
					size_t index;

					iterator& operator ++ ()
					{
						++index;
						return *this;
					}
					row operator * () const
					{
						return source->get_row(index);
					}
					bool operator != (const iterator& other) const
					{
						return index != other.index;
					}
				};

			private:
				struct
				{
					uint64_t affected_rows = 0;
					uint64_t last_inserted_row_id = 0;
				} stats;

			private:
				core::vector<core::string> columns;
				core::vector<core::vector<core::variant>> values;
				core::string status_message;
				int status_code = -1;

			public:
				response() = default;
				response(const response& other) = default;
				response(response&& other) = default;
				~response() = default;
				response& operator =(const response& other) = default;
				response& operator =(response&& other) = default;
				row operator [](size_t index)
				{
					return get_row(index);
				}
				row operator [](size_t index) const
				{
					return get_row(index);
				}
				core::schema* get_array_of_objects() const;
				core::schema* get_array_of_arrays() const;
				core::schema* get_object(size_t index = 0) const;
				core::schema* get_array(size_t index = 0) const;
				const core::vector<core::string>& get_columns() const;
				core::string get_status_text() const;
				int get_status_code() const;
				size_t get_column_index(const std::string_view& name) const;
				size_t affected_rows() const;
				size_t last_inserted_row_id() const;
				size_t size() const;
				row get_row(size_t index) const;
				row front() const;
				row back() const;
				response copy() const;
				bool empty() const;
				bool error() const;
				bool error_or_empty() const
				{
					return error() || empty();
				}
				bool exists() const
				{
					return status_code == 0;
				}

			public:
				iterator begin() const
				{
					return { this, 0 };
				}
				iterator end() const
				{
					return { this, size() };
				}
			};

			class cursor
			{
				friend class sqlite3_util;

			public:
				core::vector<response> base;

			private:
				tconnection* executor;

			public:
				cursor();
				cursor(tconnection* connection);
				cursor(const cursor& other) = delete;
				cursor(cursor&& other);
				~cursor();
				cursor& operator =(const cursor& other) = delete;
				cursor& operator =(cursor&& other);
				column operator [](const std::string_view& name)
				{
					return first().front().get_column(name);
				}
				column operator [](const std::string_view& name) const
				{
					return first().front().get_column(name);
				}
				column get_column(const std::string_view& name) const
				{
					return first().front().get_column(name);
				}
				bool success() const;
				bool empty() const;
				bool error() const;
				bool error_or_empty() const
				{
					return error() || empty();
				}
				size_t size() const;
				size_t affected_rows() const;
				cursor copy() const;
				tconnection* get_executor() const;
				const response& first() const;
				const response& last() const;
				const response& at(size_t index) const;
				core::schema* get_array_of_objects(size_t response_index = 0) const;
				core::schema* get_array_of_arrays(size_t response_index = 0) const;
				core::schema* get_object(size_t response_index = 0, size_t index = 0) const;
				core::schema* get_array(size_t response_index = 0, size_t index = 0) const;

			public:
				core::vector<response>::iterator begin()
				{
					return base.begin();
				}
				core::vector<response>::iterator end()
				{
					return base.end();
				}
			};

			class connection final : public core::reference<connection>
			{
				friend driver;

			private:
				core::unordered_map<core::string, tstatement*> statements;
				core::vector<on_function_result*> functions;
				core::vector<aggregate*> aggregates;
				core::vector<window*> windows;
				core::string source;
				tconnection* handle;
				driver* library_handle;
				uint64_t timeout;
				std::mutex update;

			public:
				connection();
				~connection() noexcept;
				void set_wal_autocheckpoint(uint32_t max_frames);
				void set_soft_heap_limit(uint64_t memory);
				void set_hard_heap_limit(uint64_t memory);
				void set_shared_cache(bool enabled);
				void set_extensions(bool enabled);
				void set_busy_timeout(uint64_t ms);
				void set_collation(const std::string_view& name, text_representation type, void* user_data, collation_comparator comparator_callback);
				void set_function(const std::string_view& name, uint8_t args, on_function_result&& context);
				void set_aggregate_function(const std::string_view& name, uint8_t args, aggregate* context);
				void set_window_function(const std::string_view& name, uint8_t args, window* context);
				void overload_function(const std::string_view& name, uint8_t args);
				core::vector<checkpoint> wal_checkpoint(checkpoint_mode mode, const std::string_view& database = std::string_view());
				size_t free_memory_used(size_t bytes);
				size_t get_memory_used() const;
				expects_db<void> bind_null(tstatement* statement, size_t index);
				expects_db<void> bind_pointer(tstatement* statement, size_t index, void* value);
				expects_db<void> bind_string(tstatement* statement, size_t index, const std::string_view& value);
				expects_db<void> bind_blob(tstatement* statement, size_t index, const std::string_view& value);
				expects_db<void> bind_boolean(tstatement* statement, size_t index, bool value);
				expects_db<void> bind_int32(tstatement* statement, size_t index, int32_t value);
				expects_db<void> bind_int64(tstatement* statement, size_t index, int64_t value);
				expects_db<void> bind_double(tstatement* statement, size_t index, double value);
				expects_db<void> bind_decimal(tstatement* statement, size_t index, const core::decimal& value, core::vector<core::string>& temps);
				expects_db<tstatement*> prepare_statement(const std::string_view& command, std::string_view* leftover_command);
				expects_db<session_id> tx_begin(isolation type);
				expects_db<session_id> tx_start(const std::string_view& command);
				expects_db<void> tx_end(const std::string_view& command, session_id session);
				expects_db<void> tx_commit(session_id session);
				expects_db<void> tx_rollback(session_id session);
				expects_db<void> connect(const std::string_view& location);
				expects_db<void> disconnect();
				expects_db<void> flush();
				expects_db<cursor> prepared_query(tstatement* statement, session_id session = nullptr);
				expects_db<cursor> emplace_query(const std::string_view& command, core::schema_list* map, size_t query_ops = 0, session_id session = nullptr);
				expects_db<cursor> template_query(const std::string_view& name, core::schema_args* map, size_t query_ops = 0, session_id session = nullptr);
				expects_db<cursor> query(const std::string_view& command, size_t query_ops = 0, session_id session = nullptr);
				tconnection* get_connection();
				const core::string& get_address();
				bool is_connected();
			};

			class cluster final : public core::reference<cluster>
			{
				friend driver;

			private:
				struct request
				{
					core::promise<tconnection*> future;
					session_id session = nullptr;
					size_t opts = 0;
				};

			private:
				core::unordered_map<tconnection*, core::single_queue<request>> queues;
				core::unordered_set<tconnection*> idle;
				core::unordered_set<tconnection*> busy;
				core::vector<on_function_result*> functions;
				core::vector<aggregate*> aggregates;
				core::vector<window*> windows;
				core::string source;
				driver* library_handle;
				uint64_t timeout;
				std::mutex update;

			public:
				cluster();
				~cluster() noexcept;
				void set_wal_autocheckpoint(uint32_t max_frames);
				void set_soft_heap_limit(uint64_t memory);
				void set_hard_heap_limit(uint64_t memory);
				void set_shared_cache(bool enabled);
				void set_extensions(bool enabled);
				void set_busy_timeout(uint64_t ms);
				void set_function(const std::string_view& name, uint8_t args, on_function_result&& context);
				void set_aggregate_function(const std::string_view& name, uint8_t args, aggregate* context);
				void set_window_function(const std::string_view& name, uint8_t args, window* context);
				void overload_function(const std::string_view& name, uint8_t args);
				core::vector<checkpoint> wal_checkpoint(checkpoint_mode mode, const std::string_view& database = std::string_view());
				size_t free_memory_used(size_t bytes);
				size_t get_memory_used() const;
				expects_promise_db<session_id> tx_begin(isolation type);
				expects_promise_db<session_id> tx_start(const std::string_view& command);
				expects_promise_db<void> tx_end(const std::string_view& command, session_id session);
				expects_promise_db<void> tx_commit(session_id session);
				expects_promise_db<void> tx_rollback(session_id session);
				expects_promise_db<void> connect(const std::string_view& location, size_t connections);
				expects_promise_db<void> disconnect();
				expects_promise_db<void> flush();
				expects_promise_db<cursor> emplace_query(const std::string_view& command, core::schema_list* map, size_t query_ops = 0, session_id session = nullptr);
				expects_promise_db<cursor> template_query(const std::string_view& name, core::schema_args* map, size_t query_ops = 0, session_id session = nullptr);
				expects_promise_db<cursor> query(const std::string_view& command, size_t query_ops = 0, session_id session = nullptr);
				tconnection* get_idle_connection();
				tconnection* get_busy_connection();
				tconnection* get_any_connection();
				const core::string& get_address();
				bool is_connected();

			private:
				tconnection* try_acquire_connection(session_id session, size_t opts);
				core::promise<tconnection*> acquire_connection(session_id session, size_t opts);
				void release_connection(tconnection* connection, size_t opts);
			};

			class utils
			{
			public:
				static expects_db<core::string> inline_array(core::uptr<core::schema>&& array);
				static expects_db<core::string> inline_query(core::uptr<core::schema>&& where, const core::unordered_map<core::string, core::string>& whitelist, const std::string_view& default_value = "TRUE");
				static core::string get_char_array(const std::string_view& src) noexcept;
				static core::string get_byte_array(const std::string_view& src) noexcept;
				static core::string get_sql(core::schema* source, bool escape, bool negate) noexcept;
				static core::schema* get_schema_from_value(const core::variant& value) noexcept;
				static core::variant_list context_args(tvalue** values, size_t values_count) noexcept;
				static void context_return(tcontext* context, const core::variant& result) noexcept;
			};

			class driver final : public core::singleton<driver>
			{
			private:
				struct pose
				{
					core::string key;
					size_t offset = 0;
					bool escape = false;
					bool negate = false;
				};

				struct sequence
				{
					core::vector<pose> positions;
					core::string request;
					core::string cache;
				};

			private:
				core::unordered_map<core::string, sequence> queries;
				core::unordered_map<core::string, core::string> constants;
				std::mutex exclusive;
				std::atomic<bool> active;
				on_query_log logger;

			public:
				driver() noexcept;
				virtual ~driver() noexcept override;
				void set_query_log(const on_query_log& callback) noexcept;
				void log_query(const std::string_view& command) noexcept;
				void add_constant(const std::string_view& name, const std::string_view& value);
				expects_db<void> add_query(const std::string_view& name, const std::string_view& data);
				expects_db<void> add_directory(const std::string_view& directory, const std::string_view& origin = "");
				bool remove_constant(const std::string_view& name) noexcept;
				bool remove_query(const std::string_view& name) noexcept;
				bool load_cache_dump(core::schema* dump) noexcept;
				core::schema* get_cache_dump() noexcept;
				expects_db<core::string> emplace(const std::string_view& SQL, core::schema_list* map) noexcept;
				expects_db<core::string> get_query(const std::string_view& name, core::schema_args* map) noexcept;
				core::vector<core::string> get_queries() noexcept;
				bool is_log_active() const noexcept;
			};
		}
	}
}
#endif