#ifndef VI_NETWORK_H
#define VI_NETWORK_H
#include "compute.h"
#include <atomic>
struct ssl_ctx_st;
struct ssl_st;
struct addrinfo;
struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;
struct pollfd;

namespace vitex
{
	namespace network
	{
		struct epoll_queue;

		struct socket_accept;

		struct epoll_interface;

		class multiplexer;

		class socket;

		class socket_connection;

		enum
		{
			ADDRESS_SIZE = 64,
			PEER_NOT_SECURE = -1,
			PEER_NOT_VERIFIED = 0,
			PEER_VERITY_DEFAULT = 100,
		};

		enum class secure_layer_options
		{
			all = 0,
			no_sslv2 = 1 << 1,
			no_sslv3 = 1 << 2,
			no_tlsv1 = 1 << 3,
			no_tlsv11 = 1 << 4,
			no_tlsv12 = 1 << 5,
			no_tlsv13 = 1 << 6
		};

		enum class server_state
		{
			working,
			stopping,
			idle
		};

		enum class socket_poll
		{
			next = 0,
			reset = 1,
			timeout = 2,
			cancel = 3,
			finish = 4,
			finish_sync = 5
		};

		enum class socket_protocol
		{
			IP,
			ICMP,
			TCP,
			UDP,
			RAW
		};

		enum class socket_type
		{
			stream,
			datagram,
			raw,
			reliably_delivered_message,
			sequence_packet_stream
		};

		enum class dns_type
		{
			connect,
			listen
		};

		typedef std::function<void(core::expects_system<void>&&)> socket_client_callback;
		typedef std::function<void(socket_poll)> poll_event_callback;
		typedef std::function<void(socket_poll)> socket_written_callback;
		typedef std::function<void(const core::option<std::error_condition>&)> socket_status_callback;
		typedef std::function<bool(socket_poll, const uint8_t*, size_t)> socket_read_callback;
		typedef std::function<bool(socket_accept&)> socket_accepted_callback;

		struct location
		{
		public:
			core::unordered_map<core::string, core::string> query;
			core::string protocol;
			core::string username;
			core::string password;
			core::string hostname;
			core::string fragment;
			core::string path;
			core::string body;
			uint16_t port;

		public:
			location(const std::string_view& from) noexcept;
			location(const location&) = default;
			location(location&&) noexcept = default;
			location& operator= (const location&) = default;
			location& operator= (location&&) noexcept = default;
		};

		struct x509_blob
		{
			void* pointer;

			x509_blob(void* new_pointer) noexcept;
			x509_blob(const x509_blob& other) noexcept;
			x509_blob(x509_blob&& other) noexcept;
			~x509_blob() noexcept;
			x509_blob& operator= (const x509_blob& other) noexcept;
			x509_blob& operator= (x509_blob&& other) noexcept;
		};

		struct certificate_blob
		{
			core::string private_key;
			core::string certificate;
		};

		struct certificate
		{
			core::unordered_map<core::string, core::string> extensions;
			core::string subject_name;
			core::string issuer_name;
			core::string serial_number;
			core::string fingerprint;
			core::string public_key;
			core::string not_before_date;
			core::string not_after_date;
			int64_t not_before_time = 0;
			int64_t not_after_time = 0;
			int32_t version = 0;
			int32_t signature = 0;
			x509_blob blob = x509_blob(nullptr);

			core::unordered_map<core::string, core::vector<core::string>> get_full_extensions() const;
		};

		struct data_frame
		{
			core::string message;
			size_t reuses = 1;
			int64_t start = 0;
			int64_t finish = 0;
			int64_t timeout = 0;
			bool abort = false;

			data_frame() = default;
			data_frame& operator= (const data_frame& other);
		};

		struct socket_certificate
		{
			certificate_blob blob;
			core::string ciphers = "ALL";
			ssl_ctx_st* context = nullptr;
			secure_layer_options options = secure_layer_options::all;
			uint32_t verify_peers = 100;
		};

		struct socket_cidr
		{
			compute::uint128 min_value;
			compute::uint128 max_value;
			compute::uint128 value;
			uint8_t mask;

			bool is_matching(const compute::uint128& value);
		};

		struct socket_address
		{
		private:
			struct
			{
				core::string hostname;
				uint16_t port;
				uint16_t padding;
				int32_t flags;
				int32_t family;
				int32_t type;
				int32_t protocol;
			} info;

		private:
			char address_buffer[ADDRESS_SIZE];
			size_t address_size;

		public:
			socket_address() noexcept;
			socket_address(const std::string_view& ip_address, uint16_t port) noexcept;
			socket_address(const std::string_view& hostname, uint16_t port, addrinfo* address_info) noexcept;
			socket_address(const std::string_view& hostname, uint16_t port, sockaddr* address, size_t address_size) noexcept;
			socket_address(socket_address&&) = default;
			socket_address(const socket_address&) = default;
			~socket_address() = default;
			socket_address& operator= (const socket_address&) = default;
			socket_address& operator= (socket_address&&) = default;
			const sockaddr_in* get_address4() const noexcept;
			const sockaddr_in6* get_address6() const noexcept;
			const sockaddr* get_raw_address() const noexcept;
			size_t get_address_size() const noexcept;
			int32_t get_flags() const noexcept;
			int32_t get_family() const noexcept;
			int32_t get_type() const noexcept;
			int32_t get_protocol() const noexcept;
			dns_type get_resolver_type() const noexcept;
			socket_protocol get_protocol_type() const noexcept;
			socket_type get_socket_type() const noexcept;
			bool is_valid() const noexcept;
			core::expects_io<core::string> get_hostname() const noexcept;
			core::expects_io<core::string> get_ip_address() const noexcept;
			core::expects_io<uint16_t> get_ip_port() const noexcept;
			core::expects_io<compute::uint128> get_ip_value() const noexcept;
		};

		struct socket_accept
		{
			socket_address address;
			socket_t fd = 0;
		};

		struct router_listener
		{
			socket_address address;
			bool is_secure;
		};

		struct epoll_fd
		{
			socket* base;
			bool readable;
			bool writeable;
			bool closeable;
		};

		struct epoll_interface
		{
		private:
			epoll_queue* queue;
			epoll_handle handle;

		public:
			epoll_interface(size_t max_events) noexcept;
			epoll_interface(const epoll_interface&) = delete;
			epoll_interface(epoll_interface&& other) noexcept;
			~epoll_interface() noexcept;
			epoll_interface& operator= (const epoll_interface&) = delete;
			epoll_interface& operator= (epoll_interface&& other) noexcept;
			bool add(socket* fd, bool readable, bool writeable) noexcept;
			bool update(socket* fd, bool readable, bool writeable) noexcept;
			bool remove(socket* fd) noexcept;
			int wait(epoll_fd* data, size_t data_size, uint64_t timeout) noexcept;
			size_t capacity() noexcept;
		};

		class packet
		{
		public:
			static bool is_data(socket_poll event)
			{
				return event == socket_poll::next;
			}
			static bool is_skip(socket_poll event)
			{
				return event == socket_poll::cancel;
			}
			static bool is_done(socket_poll event)
			{
				return event == socket_poll::finish || event == socket_poll::finish_sync;
			}
			static bool is_done_sync(socket_poll event)
			{
				return event == socket_poll::finish_sync;
			}
			static bool is_done_async(socket_poll event)
			{
				return event == socket_poll::finish;
			}
			static bool is_timeout(socket_poll event)
			{
				return event == socket_poll::timeout;
			}
			static bool is_error(socket_poll event)
			{
				return event == socket_poll::reset || event == socket_poll::timeout;
			}
			static bool is_error_or_skip(socket_poll event)
			{
				return is_error(event) || event == socket_poll::cancel;
			}
			static std::error_condition to_condition(socket_poll event)
			{
				switch (event)
				{
					case socket_poll::next:
						return std::make_error_condition(std::errc::operation_in_progress);
					case socket_poll::reset:
						return std::make_error_condition(std::errc::connection_reset);
					case socket_poll::timeout:
						return std::make_error_condition(std::errc::timed_out);
					case socket_poll::cancel:
						return std::make_error_condition(std::errc::operation_canceled);
					case socket_poll::finish:
					case socket_poll::finish_sync:
					default:
						return std::make_error_condition(std::errc());
				}
			}
		};

		class utils
		{
		public:
			enum poll_event : uint32_t
			{
				input_normal = (1 << 0),
				input_band = (1 << 1),
				input_priority = (1 << 2),
				input = (1 << 3),
				output_normal = (1 << 4),
				output_band = (1 << 5),
				output = (1 << 6),
				error = (1 << 7),
				hangup = (1 << 8)
			};

			struct poll_fd
			{
				socket_t fd = 0;
				uint32_t events = 0;
				uint32_t returns = 0;
			};

		public:
			static core::expects_io<certificate_blob> generate_self_signed_certificate(uint32_t days = 365 * 4, const std::string_view& addresses_comma_separated = "127.0.0.1", const std::string_view& domains_comma_separated = std::string_view()) noexcept;
			static core::expects_io<certificate> get_certificate_from_x509(void* certificate_x509) noexcept;
			static core::vector<core::string> get_host_ip_addresses() noexcept;
			static int poll(pollfd* fd, int fd_count, int timeout) noexcept;
			static int poll(poll_fd* fd, int fd_count, int timeout) noexcept;
			static std::error_condition GetLastError(ssl_st* device, int error_code) noexcept;
			static core::option<socket_cidr> parse_address_mask(const std::string_view& mask) noexcept;
			static bool is_invalid(socket_t fd) noexcept;
			static int64_t clock() noexcept;
			static void display_transport_log() noexcept;
		};

		class transport_layer final : public core::singleton<transport_layer>
		{
		private:
			std::mutex exclusive;
			core::unordered_set<ssl_ctx_st*> servers;
			core::unordered_set<ssl_ctx_st*> clients;
			bool is_installed;

		public:
			transport_layer() noexcept;
			virtual ~transport_layer() noexcept override;
			core::expects_io<ssl_ctx_st*> create_server_context(size_t verify_depth, secure_layer_options options, const std::string_view& ciphers_list) noexcept;
			core::expects_io<ssl_ctx_st*> create_client_context(size_t verify_depth) noexcept;
			void free_server_context(ssl_ctx_st* context) noexcept;
			void free_client_context(ssl_ctx_st* context) noexcept;

		private:
			core::expects_io<void> initialize_context(ssl_ctx_st* context, bool load_certificates) noexcept;
		};

		class dns final : public core::singleton<dns>
		{
		private:
			std::mutex exclusive;
			core::unordered_map<size_t, std::pair<int64_t, socket_address>> names;

		public:
			dns() noexcept;
			virtual ~dns() noexcept override;
			core::expects_system<core::string> reverse_lookup(const std::string_view& hostname, const std::string_view& service = std::string_view());
			core::expects_promise_system<core::string> reverse_lookup_deferred(const std::string_view& hostname, const std::string_view& service = std::string_view());
			core::expects_system<core::string> reverse_address_lookup(const socket_address& address);
			core::expects_promise_system<core::string> reverse_address_lookup_deferred(const socket_address& address);
			core::expects_system<socket_address> lookup(const std::string_view& hostname, const std::string_view& service, dns_type resolver, socket_protocol proto = socket_protocol::TCP, socket_type type = socket_type::stream);
			core::expects_promise_system<socket_address> lookup_deferred(const std::string_view& hostname, const std::string_view& service, dns_type resolver, socket_protocol proto = socket_protocol::TCP, socket_type type = socket_type::stream);
		};

		class multiplexer final : public core::singleton<multiplexer>
		{
		private:
			std::mutex exclusive;
			core::unordered_set<socket*> trackers;
			core::vector<epoll_fd> fds;
			core::ordered_map<std::chrono::microseconds, socket*> timers;
			epoll_interface handle;
			std::atomic<size_t> activations;
			uint64_t default_timeout;

		public:
			multiplexer() noexcept;
			multiplexer(uint64_t dispatch_timeout, size_t max_events) noexcept;
			virtual ~multiplexer() noexcept override;
			void rescale(uint64_t dispatch_timeout, size_t max_events) noexcept;
			void activate() noexcept;
			void deactivate() noexcept;
			void shutdown() noexcept;
			int dispatch(uint64_t timeout) noexcept;
			bool when_readable(socket* value, poll_event_callback&& when_ready) noexcept;
			bool when_writeable(socket* value, poll_event_callback&& when_ready) noexcept;
			bool cancel_events(socket* value, socket_poll event = socket_poll::cancel) noexcept;
			bool clear_events(socket* value) noexcept;
			bool is_listening() noexcept;
			size_t get_activations() noexcept;

		private:
			void dispatch_timers(const std::chrono::microseconds& time) noexcept;
			bool dispatch_events(const epoll_fd& fd, const std::chrono::microseconds& time) noexcept;
			void try_dispatch() noexcept;
			void try_enqueue() noexcept;
			void try_listen() noexcept;
			void try_unlisten() noexcept;
			void add_timeout(socket* value, const std::chrono::microseconds& time) noexcept;
			void update_timeout(socket* value, const std::chrono::microseconds& time) noexcept;
			void remove_timeout(socket* value) noexcept;
		};

		class uplinks final : public core::singleton<uplinks>
		{
		public:
			typedef std::function<void(socket*)> acquire_callback;

		private:
			struct connection_queue
			{
				core::single_queue<acquire_callback> requests;
				core::unordered_set<socket*> streams;
				size_t duplicates = 0;
			};

		private:
			std::recursive_mutex exclusive;
			core::unordered_map<core::string, connection_queue> connections;
			size_t max_duplicates;

		public:
			uplinks() noexcept;
			virtual ~uplinks() noexcept override;
			void set_max_duplicates(size_t max);
			bool push_connection(const socket_address& address, socket* target);
			bool pop_connection_queued(const socket_address& address, acquire_callback&& callback);
			core::promise<socket*> pop_connection(const socket_address& address);
			size_t get_max_duplicates() const;
			size_t get_size();

		private:
			void listen_connection(core::string&& id, socket* target);
		};

		class certificate_builder final : public core::reference<certificate_builder>
		{
		private:
			void* certificate;
			void* private_key;

		public:
			certificate_builder() noexcept;
			~certificate_builder() noexcept;
			core::expects_io<void> set_serial_number(uint32_t bits = 160);
			core::expects_io<void> set_version(uint8_t version);
			core::expects_io<void> set_not_after(int64_t offset_seconds);
			core::expects_io<void> set_not_before(int64_t offset_seconds = 0);
			core::expects_io<void> set_public_key(uint32_t bits = 4096);
			core::expects_io<void> set_issuer(const x509_blob& issuer);
			core::expects_io<void> add_custom_extension(bool critical, const std::string_view& key, const std::string_view& value, const std::string_view& description = std::string_view());
			core::expects_io<void> add_standard_extension(const x509_blob& issuer, int NID, const std::string_view& value);
			core::expects_io<void> add_standard_extension(const x509_blob& issuer, const std::string_view& name_of_nid, const std::string_view& value);
			core::expects_io<void> add_subject_info(const std::string_view& key, const std::string_view& value);
			core::expects_io<void> add_issuer_info(const std::string_view& key, const std::string_view& value);
			core::expects_io<void> sign(compute::digest algorithm);
			core::expects_io<certificate_blob> build();
			void* get_certificate_x509();
			void* get_private_key_evppkey();
		};

		class socket final : public core::reference<socket>
		{
			friend epoll_interface;
			friend multiplexer;

		private:
			struct ievents
			{
				std::mutex mutex;
				poll_event_callback read_callback = nullptr;
				poll_event_callback write_callback = nullptr;
				std::chrono::microseconds expiration = std::chrono::microseconds(0);
				uint64_t timeout = 0;

				ievents() = default;
				ievents(ievents&& other) noexcept;
				ievents& operator=(ievents&& other) noexcept;
				~ievents() = default;
			} events;

		private:
			ssl_st* device;
			socket_t fd;

		public:
			size_t income;
			size_t outcome;

		public:
			socket() noexcept;
			socket(socket_t from_fd) noexcept;
			socket(const socket& other) = delete;
			socket(socket&& other) noexcept;
			~socket() noexcept;
			socket& operator =(const socket& other) = delete;
			socket& operator =(socket&& other) noexcept;
			core::expects_io<void> accept(socket_accept* incoming);
			core::expects_io<void> accept_queued(socket_accepted_callback&& callback);
			core::expects_promise_io<socket_accept> accept_deferred();
			core::expects_io<void> shutdown(bool gracefully = false);
			core::expects_io<void> close();
			core::expects_io<void> close_queued(socket_status_callback&& callback);
			core::expects_promise_io<void> close_deferred();
			core::expects_io<size_t> write_file(FILE* stream, size_t offset, size_t size);
			core::expects_io<size_t> write_file_queued(FILE* stream, size_t offset, size_t size, socket_written_callback&& callback, size_t temp_buffer = 0);
			core::expects_promise_io<size_t> write_file_deferred(FILE* stream, size_t offset, size_t size);
			core::expects_io<size_t> write(const uint8_t* buffer, size_t size);
			core::expects_io<size_t> write_queued(const uint8_t* buffer, size_t size, socket_written_callback&& callback, bool copy_buffer_when_async = true, uint8_t* temp_buffer = nullptr, size_t temp_offset = 0);
			core::expects_promise_io<size_t> write_deferred(const uint8_t* buffer, size_t size, bool copy_buffer_when_async = true);
			core::expects_io<size_t> read(uint8_t* buffer, size_t size);
			core::expects_io<size_t> read_queued(size_t size, socket_read_callback&& callback, size_t temp_buffer = 0);
			core::expects_promise_io<core::string> read_deferred(size_t size);
			core::expects_io<size_t> read_until(const std::string_view& match, socket_read_callback&& callback);
			core::expects_io<size_t> read_until_queued(core::string&& match, socket_read_callback&& callback, size_t temp_index = 0, bool temp_buffer = false);
			core::expects_promise_io<core::string> read_until_deferred(core::string&& match, size_t max_size);
			core::expects_io<size_t> read_until_chunked(const std::string_view& match, socket_read_callback&& callback);
			core::expects_io<size_t> read_until_chunked_queued(core::string&& match, socket_read_callback&& callback, size_t temp_index = 0, bool temp_buffer = false);
			core::expects_promise_io<core::string> read_until_chunked_deferred(core::string&& match, size_t max_size);
			core::expects_io<void> connect(const socket_address& address, uint64_t timeout);
			core::expects_io<void> connect_queued(const socket_address& address, socket_status_callback&& callback);
			core::expects_promise_io<void> connect_deferred(const socket_address& address);
			core::expects_io<void> open(const socket_address& address);
			core::expects_io<void> secure(ssl_ctx_st* context, const std::string_view& server_name);
			core::expects_io<void> bind(const socket_address& address);
			core::expects_io<void> listen(int backlog);
			core::expects_io<void> clear_events(bool gracefully);
			core::expects_io<void> migrate_to(socket_t fd, bool gracefully = true);
			core::expects_io<void> set_close_on_exec();
			core::expects_io<void> set_time_wait(int timeout);
			core::expects_io<void> set_socket(int option, void* value, size_t size);
			core::expects_io<void> set_any(int level, int option, void* value, size_t size);
			core::expects_io<void> set_socket_flag(int option, int value);
			core::expects_io<void> set_any_flag(int level, int option, int value);
			core::expects_io<void> set_blocking(bool enabled);
			core::expects_io<void> set_no_delay(bool enabled);
			core::expects_io<void> set_keep_alive(bool enabled);
			core::expects_io<void> set_timeout(int timeout);
			core::expects_io<void> get_socket(int option, void* value, size_t* size);
			core::expects_io<void> get_any(int level, int option, void* value, size_t* size);
			core::expects_io<void> get_socket_flag(int option, int* value);
			core::expects_io<void> get_any_flag(int level, int option, int* value);
			core::expects_io<socket_address> get_peer_address();
			core::expects_io<socket_address> get_this_address();
			core::expects_io<certificate> get_certificate();
			void set_io_timeout(uint64_t timeout_ms);
			socket_t get_fd() const;
			ssl_st* get_device() const;
			bool is_awaiting_readable();
			bool is_awaiting_writeable();
			bool is_awaiting_events();
			bool is_secure() const;
			bool is_valid() const;

		private:
			core::expects_io<void> try_close_queued(socket_status_callback&& callback, const std::chrono::microseconds& time, bool keep_trying);
		};

		class socket_listener final : public core::reference<socket_listener>
		{
		public:
			core::string name;
			socket_address address;
			socket* stream;
			bool is_secure;

		public:
			socket_listener(const std::string_view& new_name, const socket_address& new_address, bool secure);
			~socket_listener() noexcept;
		};

		class socket_router : public core::reference<socket_router>
		{
		public:
			core::unordered_map<core::string, socket_certificate> certificates;
			core::unordered_map<core::string, router_listener> listeners;
			size_t max_heap_buffer = 1024 * 1024 * 4;
			size_t max_net_buffer = 1024 * 1024 * 32;
			size_t backlog_queue = 20;
			size_t socket_timeout = 10000;
			size_t max_connections = 0;
			int64_t keep_alive_max_count = 0;
			int64_t graceful_time_wait = -1;
			bool enable_no_delay = false;

		public:
			socket_router() = default;
			virtual ~socket_router() noexcept;
			core::expects_system<router_listener*> listen(const std::string_view& hostname, const std::string_view& service, bool secure = false);
			core::expects_system<router_listener*> listen(const std::string_view& pattern, const std::string_view& hostname, const std::string_view& service, bool secure = false);
		};

		class socket_connection : public core::reference<socket_connection>
		{
		public:
			socket_address address;
			data_frame info;
			socket* stream = nullptr;
			socket_listener* host = nullptr;

		public:
			socket_connection() = default;
			virtual ~socket_connection() noexcept;
			virtual void reset(bool fully);
			virtual bool abort();
			virtual bool abort(int, const char* format, ...);
			virtual bool next();
			virtual bool next(int);
			virtual bool closable(socket_router* router);
		};

		class socket_server : public core::reference<socket_server>
		{
			friend socket_connection;

		protected:
			std::recursive_mutex exclusive;
			core::unordered_set<socket_connection*> active;
			core::unordered_set<socket_connection*> inactive;
			core::vector<socket_listener*> listeners;
			socket_router* router;
			uint64_t shutdown_timeout;
			size_t backlog;
			std::atomic<server_state> state;

		public:
			socket_server() noexcept;
			virtual ~socket_server() noexcept;
			core::expects_system<void> configure(socket_router* init);
			core::expects_system<void> unlisten(bool gracefully);
			core::expects_system<void> listen();
			void set_router(socket_router* init);
			void set_backlog(size_t value);
			void set_shutdown_timeout(uint64_t timeout_ms);
			size_t get_backlog() const;
			uint64_t get_shutdown_timeout() const;
			server_state get_state() const;
			socket_router* get_router();
			const core::unordered_set<socket_connection*>& get_active_clients();
			const core::unordered_set<socket_connection*>& get_pooled_clients();
			const core::vector<socket_listener*>& get_listeners();

		protected:
			virtual core::expects_system<void> on_configure(socket_router* init);
			virtual core::expects_system<void> on_listen();
			virtual core::expects_system<void> on_unlisten();
			virtual core::expects_system<void> on_after_unlisten();
			virtual void on_request_open(socket_connection* base);
			virtual bool on_request_cleanup(socket_connection* base);
			virtual void on_request_stall(socket_connection* base);
			virtual void on_request_close(socket_connection* base);
			virtual socket_connection* on_allocate(socket_listener* host);
			virtual socket_router* on_allocate_router();
			virtual core::expects_io<void> try_handshake_then_begin(socket_connection* base);
			virtual core::expects_io<void> refuse(socket_connection* base);
			virtual core::expects_io<void> accept(socket_listener* host, socket_accept&& incoming);
			virtual core::expects_io<void> handshake_then_open(socket_connection* fd, socket_listener* host);
			virtual core::expects_io<void> next(socket_connection* base);
			virtual core::expects_io<void> finalize(socket_connection* base);
			virtual socket_connection* pop(socket_listener* host);
			virtual void push(socket_connection* base);
			virtual bool free_all();
		};

		class socket_client : public core::reference<socket_client>
		{
		protected:
			struct
			{
				int64_t idle = 0;
				int8_t cache = -1;
			} timeout;

			struct
			{
				socket_client_callback resolver;
				socket_address address;
			} state;

			struct
			{
				bool is_auto_handshake = true;
				bool is_non_blocking = false;
				int32_t verify_peers = 100;
			} config;

			struct
			{
				ssl_ctx_st* context = nullptr;
				socket* stream = nullptr;
			} net;

		public:
			socket_client(int64_t request_timeout) noexcept;
			virtual ~socket_client() noexcept;
			core::expects_system<void> connect_queued(const socket_address& address, bool as_non_blocking, int32_t verify_peers, socket_client_callback&& callback);
			core::expects_system<void> disconnect_queued(socket_client_callback&& callback);
			core::expects_promise_system<void> connect_sync(const socket_address& address, int32_t verify_peers = PEER_NOT_SECURE);
			core::expects_promise_system<void> connect_async(const socket_address& address, int32_t verify_peers = PEER_NOT_SECURE);
			core::expects_promise_system<void> disconnect();
			void apply_reusability(bool keep_alive);
			const socket_address& get_peer_address() const;
			bool has_stream() const;
			bool is_secure() const;
			bool is_verified() const;
			socket* get_stream();

		protected:
			virtual core::expects_system<void> on_connect();
			virtual core::expects_system<void> on_reuse();
			virtual core::expects_system<void> on_disconnect();
			virtual bool try_reuse_stream(const socket_address& address, std::function<void(bool)>&& callback);
			virtual bool try_store_stream();
			virtual void try_handshake(std::function<void(core::expects_system<void>&&)>&& callback);
			virtual void dispatch_connection(const core::option<std::error_condition>& error_code);
			virtual void dispatch_secure_handshake(core::expects_system<void>&& status);
			virtual void dispatch_simple_handshake();
			virtual void create_stream();
			virtual void configure_stream();
			virtual void destroy_stream();
			virtual void enable_reusability();
			virtual void disable_reusability();
			virtual void handshake(std::function<void(core::expects_system<void>&&)>&& callback);
			virtual void report(core::expects_system<void>&& status);
		};
	}
}
#endif
