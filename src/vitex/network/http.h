#ifndef VI_NETWORK_HTTP_H
#define VI_NETWORK_HTTP_H
#include "../network.h"

namespace vitex
{
	namespace network
	{
		namespace http
		{
			enum
			{
				LABEL_SIZE = 16,
				INLINING_SIZE = 768,
				PAYLOAD_SIZE = (size_t)(1024 * 64)
			};

			enum class auth
			{
				granted,
				denied,
				unverified
			};

			enum class web_socket_op
			{
				next = 0x00,
				text = 0x01,
				binary = 0x02,
				close = 0x08,
				ping = 0x09,
				pong = 0x0A
			};

			enum class web_socket_state
			{
				open,
				receive,
				process,
				close
			};

			enum class compression_tune
			{
				filtered = 1,
				huffman = 2,
				rle = 3,
				fixed = 4,
				placeholder = 0
			};

			enum class route_mode
			{
				exact,
				start,
				match,
				end
			};

			template <typename t, t offset_basis, t prime>
			struct lfnv1a_hash
			{
				static_assert(std::is_unsigned<t>::value, "Q needs to be unsigned integer");

				inline t operator()(const void* address, size_t size) const noexcept
				{
					const auto data = static_cast<const uint8_t*>(address);
					auto state = offset_basis;
					for (size_t i = 0; i < size; ++i)
						state = (state ^ (size_t)tolower(data[i])) * prime;
					return state;
				}
			};

			template <size_t bits>
			struct lfnv1a_bits;

			template <>
			struct lfnv1a_bits<32> { using type = lfnv1a_hash<uint32_t, UINT32_C(2166136261), UINT32_C(16777619)>; };

			template <>
			struct lfnv1a_bits<64> { using type = lfnv1a_hash<uint64_t, UINT64_C(14695981039346656037), UINT64_C(1099511628211)>; };

			template <size_t bits>
			using LFNV1A = typename lfnv1a_bits<bits>::type;

			struct kimv_equal_to
			{
				typedef core::string first_argument_type;
				typedef core::string second_argument_type;
				typedef bool result_type;
				using is_transparent = void;

				inline result_type operator()(const core::string& left, const core::string& right) const noexcept
				{
					return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(), [](uint8_t a, uint8_t b) { return tolower(a) == tolower(b); });
				}
				inline result_type operator()(const core::string& left, const char* right) const noexcept
				{
					size_t size = strlen(right);
					return left.size() == size && std::equal(left.begin(), left.end(), right, [](uint8_t a, uint8_t b) { return tolower(a) == tolower(b); });
				}
				inline result_type operator()(const core::string& left, const std::string_view& right) const noexcept
				{
					return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(), [](uint8_t a, uint8_t b) { return tolower(a) == tolower(b); });
				}
				inline result_type operator()(const char* left, const core::string& right) const noexcept
				{
					size_t size = strlen(left);
					return size == right.size() && std::equal(left, left + size, right.begin(), [](uint8_t a, uint8_t b) { return tolower(a) == tolower(b); });
				}
				inline result_type operator()(const std::string_view& left, const core::string& right) const noexcept
				{
					return left.size() == right.size() && std::equal(left.begin(), left.end(), right.begin(), [](uint8_t a, uint8_t b) { return tolower(a) == tolower(b); });
				}
			};

			struct kimv_key_hasher
			{
				typedef float argument_type;
				typedef size_t result_type;
				using is_transparent = void;

				inline result_type operator()(const char* value) const noexcept
				{
					return LFNV1A<8 * sizeof(size_t)>()(value, strlen(value));
				}
				inline result_type operator()(const std::string_view& value) const noexcept
				{
					return LFNV1A<8 * sizeof(size_t)>()(value.data(), value.size());
				}
				inline result_type operator()(const core::string& value) const noexcept
				{
					return LFNV1A<8 * sizeof(size_t)>()(value.c_str(), value.size());
				}
			};

			typedef core::unordered_map<core::string, core::vector<core::string>, kimv_key_hasher, kimv_equal_to> kimv_unordered_map;
			typedef std::function<bool(class connection*)> success_callback;
			typedef std::function<void(class connection*, socket_poll)> headers_callback;
			typedef std::function<bool(class connection*, socket_poll, const std::string_view&)> content_callback;
			typedef std::function<bool(class connection*, struct credentials*)> authorize_callback;
			typedef std::function<bool(class connection*, core::string&)> header_callback;
			typedef std::function<bool(struct resource*)> resource_callback;
			typedef std::function<void(class web_socket_frame*)> web_socket_callback;
			typedef std::function<bool(class web_socket_frame*, web_socket_op, const std::string_view&)> web_socket_read_callback;
			typedef std::function<void(class web_socket_frame*, bool)> web_socket_status_callback;
			typedef std::function<bool(class web_socket_frame*)> web_socket_check_callback;

			class parser;

			class connection;

			class client;

			class router_entry;

			class map_router;

			class server;

			class query;

			class web_codec;

			struct error_file
			{
				core::string pattern;
				int status_code = 0;
			};

			struct mime_type
			{
				core::string extension;
				core::string type;
			};

			struct mime_static
			{
				std::string_view extension = "";
				std::string_view type = "";

				mime_static(const std::string_view& ext, const std::string_view& t);
			};

			struct credentials
			{
				core::string token;
				auth type = auth::unverified;
			};

			struct resource
			{
				kimv_unordered_map headers;
				core::string path;
				core::string type;
				core::string name;
				core::string key;
				size_t length = 0;
				bool is_in_memory = false;

				core::string& put_header(const std::string_view& key, const std::string_view& value);
				core::string& set_header(const std::string_view& key, const std::string_view& value);
				core::string compose_header(const std::string_view& key) const;
				core::vector<core::string>* get_header_ranges(const std::string_view& key);
				core::string* get_header_blob(const std::string_view& key);
				std::string_view get_header(const std::string_view& key) const;
				const core::string& get_in_memory_contents() const;
			};

			struct cookie
			{
				core::string name;
				core::string value;
				core::string domain;
				core::string path = "/";
				core::string same_site;
				core::string expires;
				bool secure = false;
				bool http_only = false;

				void set_expires(int64_t time);
				void set_expired();
			};

			struct content_frame
			{
				core::vector<resource> resources;
				core::vector<char> data;
				size_t length;
				size_t offset;
				size_t prefetch;
				bool exceeds;
				bool limited;

				content_frame();
				content_frame(const content_frame&) = default;
				content_frame(content_frame&&) noexcept = default;
				content_frame& operator= (const content_frame&) = default;
				content_frame& operator= (content_frame&&) noexcept = default;
				void append(const std::string_view& data);
				void assign(const std::string_view& data);
				void prepare(const kimv_unordered_map& headers, const uint8_t* buffer, size_t size);
				void finalize();
				void cleanup();
				core::expects_parser<core::schema*> get_json() const;
				core::expects_parser<core::schema*> get_xml() const;
				core::string get_text() const;
				bool is_finalized() const;
			};

			struct request_frame
			{
				content_frame content;
				kimv_unordered_map cookies;
				kimv_unordered_map headers;
				compute::regex_result match;
				credentials user;
				core::string query;
				core::string path;
				core::string location;
				core::string referrer;
				char method[LABEL_SIZE];
				char version[LABEL_SIZE];

				request_frame();
				request_frame(const request_frame&) = default;
				request_frame(request_frame&&) noexcept = default;
				request_frame& operator= (const request_frame&) = default;
				request_frame& operator= (request_frame&&) noexcept = default;
				void set_method(const std::string_view& value);
				void set_version(uint32_t major, uint32_t minor);
				void cleanup();
				core::string& put_header(const std::string_view& key, const std::string_view& value);
				core::string& set_header(const std::string_view& key, const std::string_view& value);
				core::string compose_header(const std::string_view& key) const;
				core::vector<core::string>* get_header_ranges(const std::string_view& key);
				core::string* get_header_blob(const std::string_view& key);
				std::string_view get_header(const std::string_view& key) const;
				core::vector<core::string>* get_cookie_ranges(const std::string_view& key);
				core::string* get_cookie_blob(const std::string_view& key);
				std::string_view get_cookie(const std::string_view& key) const;
				core::vector<std::pair<size_t, size_t>> get_ranges() const;
				std::pair<size_t, size_t> get_range(core::vector<std::pair<size_t, size_t>>::iterator range, size_t content_length) const;
			};

			struct response_frame
			{
				content_frame content;
				kimv_unordered_map headers;
				core::vector<cookie> cookies;
				int status_code;
				bool error;

				response_frame();
				response_frame(const response_frame&) = default;
				response_frame(response_frame&&) noexcept = default;
				response_frame& operator= (const response_frame&) = default;
				response_frame& operator= (response_frame&&) noexcept = default;
				void set_cookie(const cookie& value);
				void set_cookie(cookie&& value);
				void cleanup();
				core::string& put_header(const std::string_view& key, const std::string_view& value);
				core::string& set_header(const std::string_view& key, const std::string_view& value);
				core::string compose_header(const std::string_view& key) const;
				core::vector<core::string>* get_header_ranges(const std::string_view& key);
				core::string* get_header_blob(const std::string_view& key);
				std::string_view get_header(const std::string_view& key) const;
				cookie* get_cookie(const std::string_view& key);
				bool is_undefined() const;
				bool is_ok() const;
			};

			struct fetch_frame
			{
				content_frame content;
				kimv_unordered_map cookies;
				kimv_unordered_map headers;
				uint64_t timeout;
				size_t max_size;
				uint32_t verify_peers;

				fetch_frame();
				fetch_frame(const fetch_frame&) = default;
				fetch_frame(fetch_frame&&) noexcept = default;
				fetch_frame& operator= (const fetch_frame&) = default;
				fetch_frame& operator= (fetch_frame&&) noexcept = default;
				void cleanup();
				core::string& put_header(const std::string_view& key, const std::string_view& value);
				core::string& set_header(const std::string_view& key, const std::string_view& value);
				core::string compose_header(const std::string_view& key) const;
				core::vector<core::string>* get_header_ranges(const std::string_view& key);
				core::string* get_header_blob(const std::string_view& key);
				std::string_view get_header(const std::string_view& key) const;
				core::vector<core::string>* get_cookie_ranges(const std::string_view& key);
				core::string* get_cookie_blob(const std::string_view& key);
				std::string_view get_cookie(const std::string_view& key) const;
				core::vector<std::pair<size_t, size_t>> get_ranges() const;
				std::pair<size_t, size_t> get_range(core::vector<std::pair<size_t, size_t>>::iterator range, size_t content_length) const;
			};

			class web_socket_frame final : public core::reference<web_socket_frame>
			{
				friend class connection;

			private:
				enum class tunnel
				{
					healthy,
					closing,
					gone
				};

			private:
				struct message
				{
					uint32_t mask;
					char* buffer;
					size_t size;
					web_socket_op opcode;
					web_socket_callback callback;
				};

			public:
				struct
				{
					web_socket_callback destroy;
					web_socket_status_callback close;
					web_socket_check_callback dead;
				} lifetime;

			private:
				std::mutex section;
				core::single_queue<message> messages;
				socket* stream;
				web_codec* codec;
				std::atomic<uint32_t> state;
				std::atomic<uint32_t> tunneling;
				std::atomic<bool> active;
				std::atomic<bool> deadly;
				std::atomic<bool> busy;

			public:
				web_socket_callback connect;
				web_socket_callback before_disconnect;
				web_socket_callback disconnect;
				web_socket_read_callback receive;
				void* user_data;

			public:
				web_socket_frame(socket* new_stream, void* new_user_data);
				~web_socket_frame() noexcept;
				core::expects_system<size_t> send(const std::string_view& buffer, web_socket_op op_code, web_socket_callback&& callback);
				core::expects_system<size_t> send(uint32_t mask, const std::string_view& buffer, web_socket_op op_code, web_socket_callback&& callback);
				core::expects_system<void> send_close(web_socket_callback&& callback);
				void next();
				bool is_finished();
				socket* get_stream();
				connection* get_connection();
				client* get_client();

			private:
				void update();
				void finalize();
				void dequeue();
				bool enqueue(uint32_t mask, const std::string_view& buffer, web_socket_op op_code, web_socket_callback&& callback);
				bool is_writeable();
				bool is_ignore();
			};

			class router_group final : public core::reference<router_group>
			{
			public:
				core::string match;
				core::vector<router_entry*> routes;
				route_mode mode;

			public:
				router_group(const std::string_view& new_match, route_mode new_mode) noexcept;
				~router_group() noexcept;
			};

			class router_entry final : public core::reference<router_entry>
			{
				friend map_router;

			public:
				struct entry_callbacks
				{
					struct web_socket_callbacks
					{
						success_callback initiate;
						web_socket_callback connect;
						web_socket_callback disconnect;
						web_socket_read_callback receive;
					} web_socket;

					success_callback get;
					success_callback post;
					success_callback put;
					success_callback patch;
					success_callback deinit;
					success_callback options;
					success_callback access;
					header_callback headers;
					authorize_callback authorize;
				} callbacks;

				struct entry_auth
				{
					core::string type;
					core::string realm;
					core::vector<core::string> methods;
				} auth;

				struct entry_compression
				{
					core::vector<compute::regex_source> files;
					compression_tune tune = compression_tune::placeholder;
					size_t min_length = 16384;
					int quality_level = 8;
					int memory_level = 8;
					bool enabled = false;
				} compression;

			public:
				compute::regex_source location;
				core::string files_directory;
				core::string char_set = "utf-8";
				core::string proxy_ip_address;
				core::string access_control_allow_origin;
				core::string redirect;
				core::string alias;
				core::vector<compute::regex_source> hidden_files;
				core::vector<error_file> error_files;
				core::vector<mime_type> mime_types;
				core::vector<core::string> index_files;
				core::vector<core::string> try_files;
				core::vector<core::string> disallowed_methods;
				map_router* router = nullptr;
				size_t web_socket_timeout = 30000;
				size_t static_file_max_age = 604800;
				size_t level = 0;
				bool allow_directory_listing = false;
				bool allow_web_socket = false;
				bool allow_send_file = true;

			private:
				static router_entry* from(const router_entry& other, const compute::regex_source& source);
			};

			class map_router final : public socket_router
			{
			public:
				struct router_session
				{
					struct router_cookie
					{
						core::string name = "sid";
						core::string domain;
						core::string path = "/";
						core::string same_site = "Strict";
						uint64_t expires = 31536000;
						bool secure = false;
						bool http_only = true;
					} cookie;

					core::string directory;
					uint64_t expires = 604800;
				} session;

				struct router_callbacks
				{
					std::function<void(map_router*)> on_destroy;
					success_callback on_location;
				} callbacks;

			public:
				core::string temporary_directory = "./temp";
				core::vector<router_group*> groups;
				size_t max_uploadable_resources = 10;
				router_entry* base = nullptr;

			public:
				map_router();
				~map_router() override;
				void sort();
				router_group* group(const std::string_view& match, route_mode mode);
				router_entry* route(const std::string_view& match, route_mode mode, const std::string_view& pattern, bool inherit_props);
				router_entry* route(const std::string_view& pattern, router_group* group, router_entry* from);
				bool remove(router_entry* source);
				bool get(const std::string_view& pattern, success_callback&& callback);
				bool get(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback);
				bool post(const std::string_view& pattern, success_callback&& callback);
				bool post(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback);
				bool put(const std::string_view& pattern, success_callback&& callback);
				bool put(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback);
				bool patch(const std::string_view& pattern, success_callback&& callback);
				bool patch(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback);
				bool deinit(const std::string_view& pattern, success_callback&& callback);
				bool deinit(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback);
				bool options(const std::string_view& pattern, success_callback&& callback);
				bool options(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback);
				bool access(const std::string_view& pattern, success_callback&& callback);
				bool access(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback);
				bool headers(const std::string_view& pattern, header_callback&& callback);
				bool headers(const std::string_view& match, route_mode mode, const std::string_view& pattern, header_callback&& callback);
				bool authorize(const std::string_view& pattern, authorize_callback&& callback);
				bool authorize(const std::string_view& match, route_mode mode, const std::string_view& pattern, authorize_callback&& callback);
				bool web_socket_initiate(const std::string_view& pattern, success_callback&& callback);
				bool web_socket_initiate(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback);
				bool web_socket_connect(const std::string_view& pattern, web_socket_callback&& callback);
				bool web_socket_connect(const std::string_view& match, route_mode mode, const std::string_view& pattern, web_socket_callback&& callback);
				bool web_socket_disconnect(const std::string_view& pattern, web_socket_callback&& callback);
				bool web_socket_disconnect(const std::string_view& match, route_mode mode, const std::string_view& pattern, web_socket_callback&& callback);
				bool web_socket_receive(const std::string_view& pattern, web_socket_read_callback&& callback);
				bool web_socket_receive(const std::string_view& match, route_mode mode, const std::string_view& pattern, web_socket_read_callback&& callback);
			};

			class connection final : public socket_connection
			{
			public:
				request_frame request;
				response_frame response;
				core::file_entry resource;
				parser* resolver = nullptr;
				web_socket_frame* web_socket = nullptr;
				router_entry* route = nullptr;
				server* root = nullptr;

			public:
				connection(server* source) noexcept;
				~connection() noexcept override;
				void reset(bool fully) override;
				bool next() override;
				bool next(int status_code) override;
				bool send_headers(int status_code, bool specify_transfer_encoding, headers_callback&& callback);
				bool send_chunk(const std::string_view& chunk, headers_callback&& callback);
				bool fetch(content_callback&& callback = nullptr, bool eat = false);
				bool store(resource_callback&& callback = nullptr, bool eat = false);
				bool skip(success_callback&& callback);
				core::expects_io<core::string> get_peer_ip_address() const;
				bool is_skip_required() const;

			private:
				bool compose_response(bool apply_error_response, bool apply_body_inline, headers_callback&& callback);
				bool error_response_requested();
				bool body_inlining_requested();
				bool waiting_for_web_socket();
			};

			class query final : public core::reference<query>
			{
			private:
				struct query_token
				{
					char* value = nullptr;
					size_t length = 0;
				};

			public:
				core::schema* object;

			public:
				query();
				~query() noexcept;
				void clear();
				void steal(core::schema** output);
				void decode(const std::string_view& content_type, const std::string_view& body);
				core::string encode(const std::string_view& content_type) const;
				core::schema* get(const std::string_view& name) const;
				core::schema* set(const std::string_view& name);
				core::schema* set(const std::string_view& name, const std::string_view& value);

			private:
				void new_parameter(core::vector<query_token>* tokens, const query_token& name, const query_token& value);
				void decode_axwfd(const std::string_view& body);
				void decode_ajson(const std::string_view& body);
				core::string encode_axwfd() const;
				core::string encode_ajson() const;
				core::schema* get_parameter(query_token* name);

			private:
				static core::string build(core::schema* base);
				static core::string build_from_base(core::schema* base);
				static core::schema* find_parameter(core::schema* base, query_token* name);
			};

			class session final : public core::reference<session>
			{
			public:
				core::string session_id;
				core::schema* query = nullptr;
				int64_t session_expires = 0;

			public:
				session();
				~session() noexcept;
				core::expects_system<void> write(connection* base);
				core::expects_system<void> read(connection* base);
				void clear();

			private:
				core::string& find_session_id(connection* base);
				core::string& generate_session_id(connection* base);

			public:
				static core::expects_system<void> invalidate_cache(const std::string_view& path);
			};

			class parser final : public core::reference<parser>
			{
			private:
				enum class multipart_status : uint8_t
				{
					uninitialized = 1,
					start,
					start_boundary,
					header_field_start,
					header_field,
					header_field_waiting,
					header_value_start,
					header_value,
					header_value_waiting,
					resource_start,
					resource,
					resource_boundary_waiting,
					resource_boundary,
					resource_waiting,
					resource_end,
					resource_hyphen,
					end
				};

				enum class chunked_status : int8_t
				{
					size,
					ext,
					data,
					end,
					head,
					middle
				};

			public:
				struct message_state
				{
					core::string header;
					char* version = nullptr;
					char* method = nullptr;
					int* status_code = nullptr;
					core::string* location = nullptr;
					core::string* query = nullptr;
					kimv_unordered_map* cookies = nullptr;
					kimv_unordered_map* headers = nullptr;
					content_frame* content = nullptr;
				} message;

				struct chunked_state
				{
					size_t length = 0;
					chunked_status state = chunked_status::size;
					int8_t consume_trailer = 1;
					int8_t hex_count = 0;
				} chunked;

				struct multipart_state
				{
					resource data;
					resource_callback callback;
					core::string* temporary_directory = nullptr;
					FILE* stream = nullptr;
					uint8_t* look_behind = nullptr;
					uint8_t* boundary = nullptr;
					int64_t index = 0;
					int64_t length = 0;
					size_t max_resources = 0;
					bool skip = false;
					bool finish = false;
					multipart_status state = multipart_status::start;
				} multipart;

			public:
				parser();
				~parser() noexcept;
				void prepare_for_request_parsing(request_frame* request);
				void prepare_for_response_parsing(response_frame* response);
				void prepare_for_chunked_parsing();
				void prepare_for_multipart_parsing(content_frame* content, core::string* temporary_directory, size_t max_resources, bool skip, resource_callback&& callback);
				int64_t multipart_parse(const std::string_view& boundary, const uint8_t* buffer, size_t length);
				int64_t parse_request(const uint8_t* buffer_start, size_t length, size_t length_last_time);
				int64_t parse_response(const uint8_t* buffer_start, size_t length, size_t length_last_time);
				int64_t parse_decode_chunked(uint8_t* buffer, size_t* buffer_length);

			private:
				const uint8_t* is_completed(const uint8_t* buffer, const uint8_t* buffer_end, size_t offset, int* out);
				const uint8_t* tokenize(const uint8_t* buffer, const uint8_t* buffer_end, const uint8_t** token, size_t* token_length, int* out);
				const uint8_t* process_version(const uint8_t* buffer, const uint8_t* buffer_end, int* out);
				const uint8_t* process_headers(const uint8_t* buffer, const uint8_t* buffer_end, int* out);
				const uint8_t* process_request(const uint8_t* buffer, const uint8_t* buffer_end, int* out);
				const uint8_t* process_response(const uint8_t* buffer, const uint8_t* buffer_end, int* out);
			};

			class web_codec final : public core::reference<web_codec>
			{
			public:
				typedef core::single_queue<std::pair<web_socket_op, core::vector<char>>> message_queue;

			private:
				enum class bytecode
				{
					begin = 0,
					length,
					length160,
					length161,
					length640,
					length641,
					length642,
					length643,
					length644,
					length645,
					length646,
					length647,
					mask0,
					mask1,
					mask2,
					mask3,
					end
				};

			private:
				message_queue queue;
				core::vector<char> payload;
				uint64_t remains;
				web_socket_op opcode;
				bytecode state;
				uint8_t mask[4];
				uint8_t fragment;
				uint8_t final;
				uint8_t control;
				uint8_t masked;
				uint8_t masks;

			public:
				core::vector<char> data;

			public:
				web_codec();
				bool parse_frame(const uint8_t* buffer, size_t size);
				bool get_frame(web_socket_op* op, core::vector<char>* message);
			};

			class hrm_cache final : public core::singleton<hrm_cache>
			{
			private:
				std::mutex mutex;
				core::single_queue<core::string*> queue;
				size_t capacity;
				size_t size;

			public:
				hrm_cache() noexcept;
				hrm_cache(size_t max_bytes_storage) noexcept;
				virtual ~hrm_cache() noexcept override;
				void rescale(size_t max_bytes_storage) noexcept;
				void shrink() noexcept;
				void push(core::string* entry);
				core::string* pop() noexcept;

			private:
				void shrink_to_fit() noexcept;
			};

			class utils
			{
			public:
				static void update_keep_alive_headers(connection* base, core::string& content);
				static std::string_view status_message(int status_code);
				static std::string_view content_type(const std::string_view& path, core::vector<mime_type>* mime_types);
			};

			class paths
			{
			public:
				static void construct_path(connection* base);
				static void construct_head_full(request_frame* request, response_frame* response, bool is_request, core::string& buffer);
				static void construct_head_cache(connection* base, core::string& buffer);
				static void construct_head_uncache(core::string& buffer);
				static bool construct_route(map_router* router, connection* base);
				static bool construct_directory_entries(connection* base, const core::string& name_a, const core::file_entry& a, const core::string& name_b, const core::file_entry& b);
				static core::string construct_content_range(size_t offset, size_t length, size_t content_length);
			};

			class parsing
			{
			public:
				static bool parse_multipart_header_field(parser* target, const uint8_t* name, size_t length);
				static bool parse_multipart_header_value(parser* target, const uint8_t* name, size_t length);
				static bool parse_multipart_content_data(parser* target, const uint8_t* name, size_t length);
				static bool parse_multipart_resource_begin(parser* target);
				static bool parse_multipart_resource_end(parser* target);
				static bool parse_header_field(parser* target, const uint8_t* name, size_t length);
				static bool parse_header_value(parser* target, const uint8_t* name, size_t length);
				static bool parse_version(parser* target, const uint8_t* name, size_t length);
				static bool parse_status_code(parser* target, size_t length);
				static bool parse_status_message(parser* target, const uint8_t* name, size_t length);
				static bool parse_method_value(parser* target, const uint8_t* name, size_t length);
				static bool parse_path_value(parser* target, const uint8_t* name, size_t length);
				static bool parse_query_value(parser* target, const uint8_t* name, size_t length);
				static int parse_content_range(const std::string_view& content_range, int64_t* range1, int64_t* range2);
				static core::string parse_multipart_data_boundary();
				static void parse_cookie(const std::string_view& value);
			};

			class permissions
			{
			public:
				static core::string authorize(const std::string_view& username, const std::string_view& password, const std::string_view& type = "Basic");
				static bool authorize(connection* base);
				static bool method_allowed(connection* base);
				static bool web_socket_upgrade_allowed(connection* base);
			};

			class resources
			{
			public:
				static bool resource_has_alternative(connection* base);
				static bool resource_hidden(connection* base, core::string* path);
				static bool resource_indexed(connection* base, core::file_entry* resource);
				static bool resource_modified(connection* base, core::file_entry* resource);
				static bool resource_compressed(connection* base, size_t size);
			};

			class routing
			{
			public:
				static bool route_web_socket(connection* base);
				static bool route_get(connection* base);
				static bool route_post(connection* base);
				static bool route_put(connection* base);
				static bool route_patch(connection* base);
				static bool route_delete(connection* base);
				static bool route_options(connection* base);
			};

			class logical
			{
			public:
				static bool process_directory(connection* base);
				static bool process_resource(connection* base);
				static bool process_resource_compress(connection* base, bool deflate, bool gzip, const std::string_view& content_range, size_t range);
				static bool process_resource_cache(connection* base);
				static bool process_file(connection* base, size_t content_length, size_t range);
				static bool process_file_stream(connection* base, FILE* stream, size_t content_length, size_t range);
				static bool process_file_chunk(connection* base, FILE* stream, size_t content_length);
				static bool process_file_compress(connection* base, size_t content_length, size_t range, bool gzip);
				static bool process_file_compress_chunk(connection* base, FILE* stream, void* cstream, size_t content_length);
				static bool process_web_socket(connection* base, const uint8_t* key, size_t key_size);
			};

			class server final : public socket_server
			{
				friend connection;
				friend logical;
				friend utils;

			public:
				server();
				~server() override;
				core::expects_system<void> update();

			private:
				core::expects_system<void> update_route(router_entry* route);
				core::expects_system<void> on_configure(socket_router* init) override;
				core::expects_system<void> on_unlisten() override;
				void on_request_open(socket_connection* base) override;
				bool on_request_cleanup(socket_connection* base) override;
				void on_request_stall(socket_connection* data) override;
				void on_request_close(socket_connection* base) override;
				socket_connection* on_allocate(socket_listener* host) override;
				socket_router* on_allocate_router() override;
			};

			class client final : public socket_client
			{
			private:
				struct boundary_block
				{
					resource* file;
					core::string data;
					core::string finish;
					bool is_final;
				};

			private:
				parser* resolver;
				web_socket_frame* web_socket;
				request_frame request;
				response_frame response;
				core::vector<boundary_block> boundaries;
				core::expects_promise_system<void> future;

			public:
				client(int64_t read_timeout);
				~client() override;
				core::expects_promise_system<void> skip();
				core::expects_promise_system<void> fetch(size_t max_size = PAYLOAD_SIZE, bool eat = false);
				core::expects_promise_system<void> upgrade(request_frame&& root);
				core::expects_promise_system<void> send(request_frame&& root);
				core::expects_promise_system<void> send_fetch(request_frame&& root, size_t max_size = PAYLOAD_SIZE);
				core::expects_promise_system<core::schema*> json(request_frame&& root, size_t max_size = PAYLOAD_SIZE);
				core::expects_promise_system<core::schema*> xml(request_frame&& root, size_t max_size = PAYLOAD_SIZE);
				void downgrade();
				web_socket_frame* get_web_socket();
				request_frame* get_request();
				response_frame* get_response();

			private:
				core::expects_system<void> on_reuse() override;
				core::expects_system<void> on_disconnect() override;
				void upload_file(boundary_block* boundary, std::function<void(core::expects_system<void>&&)>&& callback);
				void upload_file_chunk(FILE* stream, size_t content_length, std::function<void(core::expects_system<void>&&)>&& callback);
				void upload_file_chunk_queued(FILE* stream, size_t content_length, std::function<void(core::expects_system<void>&&)>&& callback);
				void upload(size_t file_id);
				void manage_keep_alive();
				void receive(socket_poll event, const uint8_t* leftover_buffer, size_t leftover_size);
			};

			core::expects_promise_system<response_frame> fetch(const std::string_view& location, const std::string_view& method = "GET", const fetch_frame& options = fetch_frame());
		}
	}
}
#endif
