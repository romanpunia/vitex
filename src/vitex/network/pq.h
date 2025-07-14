#ifndef VI_NETWORK_PQ_H
#define VI_NETWORK_PQ_H
#include "../network.h"

struct pg_conn;
struct pg_result;
struct pgNotify;

namespace vitex
{
	namespace network
	{
		namespace pq
		{
			class notify;

			class row;

			class request;

			class response;

			class cursor;

			class cluster;

			class driver;

			class connection;

			typedef std::function<core::promise<bool>(const core::vector<core::string>&)> on_reconnect;
			typedef std::function<void(const std::string_view&)> on_query_log;
			typedef std::function<void(const notify&)> on_notification;
			typedef std::function<void(cursor&)> on_result;
			typedef connection* session_id;
			typedef pg_conn tconnection;
			typedef pg_result tresponse;
			typedef pgNotify tnotify;
			typedef uint64_t object_id;

			enum class isolation
			{
				serializable,
				repeatable_read,
				read_commited,
				read_uncommited,
				default_isolation = read_commited
			};

			enum class query_op
			{
				cache_short = (1 << 0),
				cache_mid = (1 << 1),
				cache_long = (1 << 2)
			};

			enum class address_op
			{
				host,
				ip,
				port,
				database,
				user,
				password,
				timeout,
				encoding,
				options,
				profile,
				fallback_profile,
				keep_alive,
				keep_alive_idle,
				keep_alive_interval,
				keep_alive_count,
				TTY,
				SSL,
				SSL_Compression,
				SSL_Cert,
				SSL_Root_Cert,
				SSL_Key,
				SSL_CRL,
				require_peer,
				require_ssl,
				krb_server_name,
				service
			};

			enum class query_exec
			{
				empty_query = 0,
				command_ok,
				tuples_ok,
				copy_out,
				copy_in,
				bad_response,
				non_fatal_error,
				fatal_error,
				copy_both,
				single_tuple
			};

			enum class field_code
			{
				severity,
				severity_nonlocalized,
				sql_state,
				message_primary,
				message_detail,
				message_hint,
				statement_position,
				internal_position,
				internal_query,
				context,
				schema_name,
				table_name,
				column_name,
				data_type_name,
				constraint_name,
				source_file,
				source_line,
				source_function
			};

			enum class transaction_state
			{
				idle,
				active,
				in_transaction,
				in_error,
				none
			};

			enum class caching
			{
				never,
				miss,
				cached
			};

			enum class oid_type
			{
				JSON = 114,
				JSONB = 3802,
				any_array = 2277,
				name_array = 1003,
				text_array = 1009,
				date_array = 1182,
				time_array = 1183,
				uuid_array = 2951,
				cstring_array = 1263,
				bp_char_array = 1014,
				var_char_array = 1015,
				bit_array = 1561,
				var_bit_array = 1563,
				char_array = 1002,
				int2_array = 1005,
				int4_array = 1007,
				int8_array = 1016,
				bool_array = 1000,
				float4_array = 1021,
				float8_array = 1022,
				money_array = 791,
				numeric_array = 1231,
				bytea_array = 1001,
				any = 2276,
				name = 19,
				text = 25,
				date = 1082,
				time = 1083,
				UUID = 2950,
				cstring = 2275,
				bp_char = 1042,
				var_char = 1043,
				bit = 1560,
				var_bit = 1562,
				symbol = 18,
				int2 = 21,
				int4 = 23,
				int8 = 20,
				boolf = 16,
				float4 = 700,
				float8 = 701,
				money = 790,
				numeric = 1700,
				bytea = 17
			};

			enum class query_state
			{
				lost,
				idle,
				idle_in_transaction,
				busy,
				busy_in_transaction
			};

			inline size_t operator |(query_op a, query_op b)
			{
				return static_cast<size_t>(static_cast<size_t>(a) | static_cast<size_t>(b));
			}

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

			class address
			{
			private:
				core::unordered_map<core::string, core::string> params;

			public:
				address();
				void override(const std::string_view& key, const std::string_view& value);
				bool set(address_op key, const std::string_view& value);
				core::string get(address_op key) const;
				core::string get_address() const;
				const core::unordered_map<core::string, core::string>& get() const;
				const char** create_keys() const;
				const char** create_values() const;

			public:
				static expects_db<address> from_url(const std::string_view& location);

			private:
				static std::string_view get_key_name(address_op key);
			};

			class notify
			{
			private:
				core::string name;
				core::string data;
				int pid;

			public:
				notify(tnotify* new_base);
				core::schema* get_schema() const;
				core::string get_name() const;
				core::string get_data() const;
				int get_pid() const;
			};

			class column
			{
				friend row;

			private:
				tresponse* base;
				size_t row_index;
				size_t column_index;

			private:
				column(tresponse* new_base, size_t fRowIndex, size_t fColumnIndex);

			public:
				core::string get_name() const;
				core::string get_value_text() const;
				core::variant get() const;
				core::schema* get_inline() const;
				char* get_raw() const;
				int get_format_id() const;
				int get_mod_id() const;
				int get_table_index() const;
				object_id get_table_id() const;
				object_id get_type_id() const;
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
				tresponse* base;
				size_t row_index;

			private:
				row(tresponse* new_base, size_t fRowIndex);

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
				tresponse* base;
				bool failure;

			public:
				response();
				response(tresponse* new_base);
				response(const response& other) = delete;
				response(response&& other);
				~response();
				response& operator =(const response& other) = delete;
				response& operator =(response&& other);
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
				core::vector<core::string> get_columns() const;
				core::string get_command_status_text() const;
				core::string get_status_text() const;
				core::string get_error_text() const;
				core::string get_error_field(field_code field) const;
				int get_name_index(const std::string_view& name) const;
				query_exec get_status() const;
				object_id get_value_id() const;
				size_t affected_rows() const;
				size_t size() const;
				row get_row(size_t index) const;
				row front() const;
				row back() const;
				response copy() const;
				tresponse* get() const;
				bool empty() const;
				bool error() const;
				bool error_or_empty() const
				{
					return error() || empty();
				}
				bool exists() const
				{
					return base != nullptr;
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
				friend cluster;

			public:
				core::vector<response> base;

			private:
				connection* executor;
				caching cache;

			public:
				cursor();
				cursor(connection* new_executor, caching type);
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
				const response& first() const;
				const response& last() const;
				const response& at(size_t index) const;
				connection* get_executor() const;
				caching get_cache_status() const;
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
				friend cluster;

			private:
				core::unordered_set<core::string> listens;
				tconnection* base;
				socket* stream;
				request* current;
				query_state status;

			public:
				connection(tconnection* new_base, socket_t fd);
				~connection() noexcept;
				tconnection* get_base() const;
				socket* get_stream() const;
				request* get_current() const;
				query_state get_state() const;
				transaction_state get_tx_state() const;
				bool in_transaction() const;
				bool busy() const;

			private:
				void make_busy(request* data);
				request* make_idle();
				request* make_lost();
			};

			class request final : public core::reference<request>
			{
				friend cluster;

			private:
				expects_promise_db<cursor> future;
				core::vector<char> command;
				std::chrono::microseconds time;
				session_id session;
				on_result callback;
				cursor result;
				uint64_t id;
				size_t options;

			public:
				request(const std::string_view& commands, session_id new_session, caching status, uint64_t rid, size_t new_options);
				void report_cursor();
				void report_failure();
				cursor&& get_result();
				const core::vector<char>& get_command() const;
				session_id get_session() const;
				uint64_t get_timing() const;
				uint64_t get_id() const;
				bool pending() const;
			};

			class cluster final : public core::reference<cluster>
			{
				friend driver;

			private:
				struct
				{
					core::unordered_map<core::string, std::pair<int64_t, cursor>> objects;
					std::mutex context;
					uint64_t short_duration = 10;
					uint64_t mid_duration = 30;
					uint64_t long_duration = 60;
					uint64_t cleanup_duration = 300;
					int64_t next_cleanup = 0;
				} cache;

			private:
				core::unordered_map<core::string, core::unordered_map<uint64_t, on_notification>> listeners;
				core::unordered_map<socket*, connection*> pool;
				core::vector<request*> requests;
				std::atomic<uint64_t> channel;
				std::atomic<uint64_t> counter;
				std::recursive_mutex update;
				on_reconnect reconnected;
				address source;

			public:
				cluster();
				~cluster() noexcept;
				void clear_cache();
				void set_cache_cleanup(uint64_t interval);
				void set_cache_duration(query_op cache_id, uint64_t duration);
				void set_when_reconnected(const on_reconnect& new_callback);
				uint64_t add_channel(const std::string_view& name, const on_notification& new_callback);
				bool remove_channel(const std::string_view& name, uint64_t id);
				expects_promise_db<session_id> tx_begin(isolation type);
				expects_promise_db<session_id> tx_start(const std::string_view& command);
				expects_promise_db<void> tx_end(const std::string_view& command, session_id session);
				expects_promise_db<void> tx_commit(session_id session);
				expects_promise_db<void> tx_rollback(session_id session);
				expects_promise_db<void> connect(const address& location, size_t connections = 1);
				expects_promise_db<void> disconnect();
				expects_promise_db<void> listen(const core::vector<core::string>& channels);
				expects_promise_db<void> unlisten(const core::vector<core::string>& channels);
				expects_promise_db<cursor> emplace_query(const std::string_view& command, core::schema_list* map, size_t query_ops = 0, session_id session = nullptr);
				expects_promise_db<cursor> template_query(const std::string_view& name, core::schema_args* map, size_t query_ops = 0, session_id session = nullptr);
				expects_promise_db<cursor> query(const std::string_view& command, size_t query_ops = 0, session_id session = nullptr);
				connection* get_connection(query_state state);
				connection* get_any_connection() const;
				bool is_connected() const;

			private:
				core::string get_cache_oid(const std::string_view& payload, size_t query_opts);
				bool get_cache(const std::string_view& cache_oid, cursor* data);
				void set_cache(const std::string_view& cache_oid, cursor* data, size_t query_opts);
				bool reestablish(connection* base);
				bool consume(connection* base);
				bool reprocess(connection* base);
				bool flush(connection* base, bool listen_for_results);
				bool dispatch(connection* base);
				bool is_managing(session_id session);
				connection* is_listens(const std::string_view& name);
			};

			class utils
			{
			public:
				static expects_db<core::string> inline_array(cluster* client, core::uptr<core::schema>&& array);
				static expects_db<core::string> inline_query(cluster* client, core::uptr<core::schema>&& where, const core::unordered_map<core::string, core::string>& whitelist, const std::string_view& default_value = "TRUE");
				static core::string get_char_array(connection* base, const std::string_view& src) noexcept;
				static core::string get_byte_array(connection* base, const std::string_view& src) noexcept;
				static core::string get_sql(connection* base, core::schema* source, bool escape, bool negate) noexcept;
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
				void add_constant(const std::string_view& name, const std::string_view& value) noexcept;
				expects_db<void> add_query(const std::string_view& name, const std::string_view& data);
				expects_db<void> add_directory(const std::string_view& directory, const std::string_view& origin = "");
				bool remove_constant(const std::string_view& name) noexcept;
				bool remove_query(const std::string_view& name) noexcept;
				bool load_cache_dump(core::schema* dump) noexcept;
				core::schema* get_cache_dump() noexcept;
				expects_db<core::string> emplace(cluster* base, const std::string_view& SQL, core::schema_list* map) noexcept;
				expects_db<core::string> get_query(cluster* base, const std::string_view& name, core::schema_args* map) noexcept;
				core::vector<core::string> get_queries() noexcept;
			};
		}
	}
}
#endif