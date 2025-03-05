#ifndef VI_NETWORK_MONGO_H
#define VI_NETWORK_MONGO_H
#include "../network.h"

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

namespace vitex
{
	namespace network
	{
		namespace mongo
		{
			typedef _bson_t tdocument;
			typedef _mongoc_client_pool_t tconnection_pool;
			typedef _mongoc_bulk_operation_t tstream;
			typedef _mongoc_cursor_t tcursor;
			typedef _mongoc_collection_t tcollection;
			typedef _mongoc_database_t tdatabase;
			typedef _mongoc_uri_t taddress;
			typedef _mongoc_client_t tconnection;
			typedef _mongoc_change_stream_t twatcher;
			typedef _mongoc_client_session_t ttransaction;
			typedef std::function<void(const std::string_view&)> on_query_log;

			class transaction;

			class connection;

			class cluster;

			class document;

			class collection;

			enum class query_flags
			{
				none = 0,
				tailable_cursor = 1 << 1,
				slave_ok = 1 << 2,
				oplog_replay = 1 << 3,
				no_cursor_timeout = 1 << 4,
				await_data = 1 << 5,
				exhaust = 1 << 6,
				partial = 1 << 7,
			};

			enum class type
			{
				unknown,
				uncastable,
				null,
				document,
				array,
				string,
				integer,
				number,
				boolean,
				object_id,
				decimal
			};

			enum class transaction_state
			{
				OK,
				retry_commit,
				retry,
				fatal
			};

			inline query_flags operator |(query_flags a, query_flags b)
			{
				return static_cast<query_flags>(static_cast<size_t>(a) | static_cast<size_t>(b));
			}

			struct property
			{
				core::string name;
				core::string string;
				uint8_t object_id[12] = { };
				tdocument* source;
				int64_t integer;
				uint64_t high;
				uint64_t low;
				double number;
				bool boolean;
				type mod;
				bool is_valid;

				property() noexcept;
				property(const property& other) = delete;
				property(property&& other);
				~property();
				property& operator =(const property& other);
				property& operator =(property&& other);
				core::unique<tdocument> reset();
				core::string& to_string();
				core::string to_object_id();
				document as_document() const;
				property at(const std::string_view& name) const;
				property operator [](const std::string_view& name);
				property operator [](const std::string_view& name) const;
			};

			class database_exception final : public core::basic_exception
			{
			private:
				int error_code;

			public:
				database_exception(int error_code, core::string&& message);
				const char* type() const noexcept override;
				int code() const noexcept;
			};

			template <typename v>
			using expects_db = core::expects<v, database_exception>;

			template <typename t, typename executor = core::parallel_executor>
			using expects_promise_db = core::basic_promise<expects_db<t>, executor>;

			class document
			{
			private:
				tdocument* base;
				bool store;

			public:
				document();
				document(tdocument* new_base);
				document(const document& other);
				document(document&& other);
				~document();
				document& operator =(const document& other);
				document& operator =(document&& other);
				void cleanup();
				void join(const document& value);
				void loop(const std::function<bool(property*)>& callback) const;
				bool set_schema(const std::string_view& key, const document& value, size_t array_id = 0);
				bool set_array(const std::string_view& key, const document& array, size_t array_id = 0);
				bool set_string(const std::string_view& key, const std::string_view& value, size_t array_id = 0);
				bool set_integer(const std::string_view& key, int64_t value, size_t array_id = 0);
				bool set_number(const std::string_view& key, double value, size_t array_id = 0);
				bool set_decimal(const std::string_view& key, uint64_t high, uint64_t low, size_t array_id = 0);
				bool set_decimal_string(const std::string_view& key, const std::string_view& value, size_t array_id = 0);
				bool set_decimal_integer(const std::string_view& key, int64_t value, size_t array_id = 0);
				bool set_decimal_number(const std::string_view& key, double value, size_t array_id = 0);
				bool set_boolean(const std::string_view& key, bool value, size_t array_id = 0);
				bool set_object_id(const std::string_view& key, uint8_t value[12], size_t array_id = 0);
				bool set_null(const std::string_view& key, size_t array_id = 0);
				bool set_property(const std::string_view& key, property* value, size_t array_id = 0);
				bool has_property(const std::string_view& key) const;
				bool get_property(const std::string_view& key, property* output) const;
				size_t count() const;
				core::string to_relaxed_json() const;
				core::string to_extended_json() const;
				core::string to_json() const;
				core::string to_indices() const;
				core::unique<core::schema> to_schema(bool is_array = false) const;
				tdocument* get() const;
				document copy() const;
				document& persist(bool keep = true);
				property at(const std::string_view& name) const;
				explicit operator bool() const
				{
					return base != nullptr;
				}
				property operator [](const std::string_view& name)
				{
					property result;
					get_property(name, &result);
					return result;
				}
				property operator [](const std::string_view& name) const
				{
					property result;
					get_property(name, &result);
					return result;
				}

			public:
				static document from_empty();
				static document from_schema(core::schema* document);
				static expects_db<document> from_json(const std::string_view& JSON);
				static document from_buffer(const uint8_t* buffer, size_t length);
				static document from_source(tdocument* src);

			private:
				static bool clone(void* it, property* output);
			};

			class address
			{
			private:
				taddress* base;

			public:
				address();
				address(taddress* new_base);
				address(const address& other);
				address(address&& other);
				~address();
				address& operator =(const address& other);
				address& operator =(address&& other);
				void set_option(const std::string_view& name, int64_t value);
				void set_option(const std::string_view& name, bool value);
				void set_option(const std::string_view& name, const std::string_view& value);
				void set_auth_mechanism(const std::string_view& value);
				void set_auth_source(const std::string_view& value);
				void set_compressors(const std::string_view& value);
				void set_database(const std::string_view& value);
				void set_username(const std::string_view& value);
				void set_password(const std::string_view& value);
				taddress* get() const;
				explicit operator bool() const
				{
					return base != nullptr;
				}

			public:
				static expects_db<address> from_url(const std::string_view& location);
			};

			class stream
			{
			private:
				document net_options;
				tcollection* source;
				tstream* base;
				size_t count;

			public:
				stream();
				stream(tcollection* new_source, tstream* new_base, document&& new_options);
				stream(const stream& other);
				stream(stream&& other);
				~stream();
				stream& operator =(const stream& other);
				stream& operator =(stream&& other);
				expects_db<void> remove_many(const document& match, const document& options);
				expects_db<void> remove_one(const document& match, const document& options);
				expects_db<void> replace_one(const document& match, const document& replacement, const document& options);
				expects_db<void> insert_one(const document& result, const document& options);
				expects_db<void> update_one(const document& match, const document& result, const document& options);
				expects_db<void> update_many(const document& match, const document& result, const document& options);
				expects_db<void> template_query(const std::string_view& name, core::schema_args* map);
				expects_db<void> query(const document& command);
				expects_promise_db<document> execute_with_reply();
				expects_promise_db<void> execute();
				size_t get_hint() const;
				tstream* get() const;
				explicit operator bool() const
				{
					return base != nullptr;
				}

			private:
				expects_db<void> next_operation();
			};

			class cursor
			{
			private:
				tcursor* base;

			public:
				cursor();
				cursor(tcursor* new_base);
				cursor(const cursor& other) = delete;
				cursor(cursor&& other);
				~cursor();
				cursor& operator =(const cursor& other) = delete;
				cursor& operator =(cursor&& other);
				void set_max_await_time(size_t max_await_time);
				void set_batch_size(size_t batch_size);
				bool set_limit(int64_t limit);
				bool set_hint(size_t hint);
				bool empty() const;
				core::option<database_exception> error() const;
				expects_promise_db<void> next();
				int64_t get_id() const;
				int64_t get_limit() const;
				size_t get_max_await_time() const;
				size_t get_batch_size() const;
				size_t get_hint() const;
				document current() const;
				cursor clone();
				tcursor* get() const;
				bool error_or_empty() const
				{
					return error() || empty();
				}
				explicit operator bool() const
				{
					return base != nullptr;
				}

			private:
				void cleanup();
			};

			class response
			{
			private:
				cursor net_cursor;
				document net_document;
				bool net_success;

			public:
				response();
				response(bool new_success);
				response(cursor& new_cursor);
				response(document& new_document);
				response(const response& other) = delete;
				response(response&& other);
				response& operator =(const response& other) = delete;
				response& operator =(response&& other);
				expects_promise_db<core::unique<core::schema>> fetch();
				expects_promise_db<core::unique<core::schema>> fetch_all();
				expects_promise_db<property> get_property(const std::string_view& name);
				cursor&& get_cursor();
				document&& get_document();
				bool success() const;
				expects_promise_db<property> operator [](const std::string_view& name)
				{
					return get_property(name);
				}
				explicit operator bool() const
				{
					return success();
				}
			};

			class transaction
			{
			private:
				ttransaction* base;

			public:
				transaction();
				transaction(ttransaction* new_base);
				transaction(const transaction& other) = default;
				transaction(transaction&& other) = default;
				~transaction() = default;
				transaction& operator =(const transaction& other) = default;
				transaction& operator =(transaction&& other) = default;
				expects_db<void> push(const document& query_options);
				expects_db<void> put(tdocument** query_options) const;
				expects_promise_db<void> begin();
				expects_promise_db<void> rollback();
				expects_promise_db<document> remove_many(collection& base, const document& match, const document& options);
				expects_promise_db<document> remove_one(collection& base, const document& match, const document& options);
				expects_promise_db<document> replace_one(collection& base, const document& match, const document& replacement, const document& options);
				expects_promise_db<document> insert_many(collection& base, core::vector<document>& list, const document& options);
				expects_promise_db<document> insert_one(collection& base, const document& result, const document& options);
				expects_promise_db<document> update_many(collection& base, const document& match, const document& update, const document& options);
				expects_promise_db<document> update_one(collection& base, const document& match, const document& update, const document& options);
				expects_promise_db<cursor> find_many(collection& base, const document& match, const document& options);
				expects_promise_db<cursor> find_one(collection& base, const document& match, const document& options);
				expects_promise_db<cursor> aggregate(collection& base, query_flags flags, const document& pipeline, const document& options);
				expects_promise_db<response> template_query(collection& base, const std::string_view& name, core::schema_args* map);
				expects_promise_db<response> query(collection& base, const document& command);
				core::promise<transaction_state> commit();
				ttransaction* get() const;
				explicit operator bool() const
				{
					return base != nullptr;
				}
			};

			class collection
			{
			private:
				tcollection* base;

			public:
				collection();
				collection(tcollection* new_base);
				collection(const collection& other) = delete;
				collection(collection&& other);
				~collection();
				collection& operator =(const collection& other) = delete;
				collection& operator =(collection&& other);
				expects_promise_db<void> rename(const std::string_view& new_database_name, const std::string_view& new_collection_name);
				expects_promise_db<void> rename_with_options(const std::string_view& new_database_name, const std::string_view& new_collection_name, const document& options);
				expects_promise_db<void> rename_with_remove(const std::string_view& new_database_name, const std::string_view& new_collection_name);
				expects_promise_db<void> rename_with_options_and_remove(const std::string_view& new_database_name, const std::string_view& new_collection_name, const document& options);
				expects_promise_db<void> remove(const document& options);
				expects_promise_db<void> remove_index(const std::string_view& name, const document& options);
				expects_promise_db<document> remove_many(const document& match, const document& options);
				expects_promise_db<document> remove_one(const document& match, const document& options);
				expects_promise_db<document> replace_one(const document& match, const document& replacement, const document& options);
				expects_promise_db<document> insert_many(core::vector<document>& list, const document& options);
				expects_promise_db<document> insert_one(const document& result, const document& options);
				expects_promise_db<document> update_many(const document& match, const document& update, const document& options);
				expects_promise_db<document> update_one(const document& match, const document& update, const document& options);
				expects_promise_db<document> find_and_modify(const document& match, const document& sort, const document& update, const document& fields, bool remove, bool upsert, bool init);
				core::promise<size_t> count_documents(const document& match, const document& options);
				core::promise<size_t> count_documents_estimated(const document& options);
				expects_promise_db<cursor> find_indexes(const document& options);
				expects_promise_db<cursor> find_many(const document& match, const document& options);
				expects_promise_db<cursor> find_one(const document& match, const document& options);
				expects_promise_db<cursor> aggregate(query_flags flags, const document& pipeline, const document& options);
				expects_promise_db<response> template_query(const std::string_view& name, core::schema_args* map, const transaction& session = transaction());
				expects_promise_db<response> query(const document& command, const transaction& session = transaction());
				core::string get_name() const;
				expects_db<stream> create_stream(document& options);
				tcollection* get() const;
				explicit operator bool() const
				{
					return base != nullptr;
				}
			};

			class database
			{
			private:
				tdatabase* base;

			public:
				database();
				database(tdatabase* new_base);
				database(const database& other) = delete;
				database(database&& other);
				~database();
				database& operator =(const database& other) = delete;
				database& operator =(database&& other);
				expects_promise_db<void> remove_all_users();
				expects_promise_db<void> remove_user(const std::string_view& name);
				expects_promise_db<void> remove();
				expects_promise_db<void> remove_with_options(const document& options);
				expects_promise_db<void> add_user(const std::string_view& username, const std::string_view& password, const document& roles, const document& custom);
				expects_promise_db<void> has_collection(const std::string_view& name);
				expects_promise_db<collection> create_collection(const std::string_view& name, const document& options);
				expects_promise_db<cursor> find_collections(const document& options);
				expects_db<core::vector<core::string>> get_collection_names(const document& options) const;
				core::string get_name() const;
				collection get_collection(const std::string_view& name);
				tdatabase* get() const;
				explicit operator bool() const
				{
					return base != nullptr;
				}
			};

			class watcher
			{
			private:
				twatcher* base;

			public:
				watcher();
				watcher(twatcher* new_base);
				watcher(const watcher& other);
				watcher(watcher&& other);
				~watcher();
				watcher& operator =(const watcher& other);
				watcher& operator =(watcher&& other);
				expects_promise_db<void> next(document& result);
				expects_promise_db<void> error(document& result);
				twatcher* get() const;
				explicit operator bool() const
				{
					return base != nullptr;
				}

			public:
				static expects_db<watcher> from_connection(connection* connection, const document& pipeline, const document& options);
				static expects_db<watcher> from_database(const database& src, const document& pipeline, const document& options);
				static expects_db<watcher> from_collection(const collection& src, const document& pipeline, const document& options);
			};

			class connection final : public core::reference<connection>
			{
				friend cluster;
				friend transaction;

			private:
				std::atomic<bool> connected;
				transaction session;
				tconnection* base;
				cluster* master;

			public:
				connection();
				~connection() noexcept;
				expects_promise_db<void> connect(address* location);
				expects_promise_db<void> disconnect();
				expects_promise_db<void> make_transaction(std::function<core::promise<bool>(transaction*)>&& callback);
				expects_promise_db<cursor> find_databases(const document& options);
				void set_profile(const std::string_view& name);
				expects_db<void> set_server(bool writeable);
				expects_db<transaction*> get_session();
				database get_database(const std::string_view& name) const;
				database get_default_database() const;
				collection get_collection(const std::string_view& database_name, const std::string_view& name) const;
				address get_address() const;
				cluster* get_master() const;
				tconnection* get() const;
				expects_db<core::vector<core::string>> get_database_names(const document& options) const;
				bool is_connected() const;
			};

			class cluster final : public core::reference<cluster>
			{
			private:
				std::atomic<bool> connected;
				tconnection_pool* pool;
				address src_address;

			public:
				cluster();
				~cluster() noexcept;
				expects_promise_db<void> connect(address* URI);
				expects_promise_db<void> disconnect();
				void set_profile(const std::string_view& name);
				void push(connection** client);
				connection* pop();
				tconnection_pool* get() const;
				address& get_address();
			};

			class utils
			{
			public:
				static bool get_id(uint8_t* id12) noexcept;
				static bool get_decimal(const std::string_view& value, int64_t* high, int64_t* low) noexcept;
				static uint32_t get_hash_id(uint8_t* id12) noexcept;
				static int64_t get_time_id(uint8_t* id12) noexcept;
				static core::string id_to_string(uint8_t* id12) noexcept;
				static core::string string_to_id(const std::string_view& id24) noexcept;
				static core::string get_json(core::schema* source, bool escape) noexcept;
			};

			class driver final : public core::singleton<driver>
			{
			private:
				struct pose
				{
					core::string key;
					size_t offset = 0;
					bool escape = false;
				};

				struct sequence
				{
					core::vector<pose> positions;
					core::string request;
					document cache;

					sequence();
					sequence(const sequence& other);
					sequence& operator =(sequence&& other) = default;
				};

			private:
				core::unordered_map<core::string, sequence> queries;
				core::unordered_map<core::string, core::string> constants;
				std::mutex exclusive;
				on_query_log logger;
				void* APM;

			public:
				driver() noexcept;
				virtual ~driver() noexcept override;
				void set_query_log(const on_query_log& callback) noexcept;
				void attach_query_log(tconnection* connection) noexcept;
				void attach_query_log(tconnection_pool* connection) noexcept;
				void add_constant(const std::string_view& name, const std::string_view& value) noexcept;
				expects_db<void> add_query(const std::string_view& name, const std::string_view& data);
				expects_db<void> add_directory(const std::string_view& directory, const std::string_view& origin = "");
				bool remove_constant(const std::string_view& name) noexcept;
				bool remove_query(const std::string_view& name) noexcept;
				bool load_cache_dump(core::schema* dump) noexcept;
				core::schema* get_cache_dump() noexcept;
				expects_db<document> get_query(const std::string_view& name, core::schema_args* map) noexcept;
				core::vector<core::string> get_queries() noexcept;
			};
		}
	}
}
#endif
