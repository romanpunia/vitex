#include "network.h"
#if defined(VI_MICROSOFT) && defined(VI_WEPOLL)
#define NET_EPOLL 1
#elif defined(VI_APPLE) || defined(__FreeBSD__)
#define NET_KQUEUE 1
#elif defined(__sun) && defined(__SVR4)
#define NET_POLL 1
#elif defined(VI_LINUX)
#define NET_EPOLL 1
#else
#define NET_POLL 1
#endif
#ifdef VI_MICROSOFT
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <io.h>
#ifdef VI_WEPOLL
#include <wepoll.h>
#endif
#define INVALID_EPOLL nullptr
#else
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#ifdef NET_EPOLL
#include <sys/epoll.h>
#include <sys/sendfile.h>
#else
#include <sys/event.h>
#endif
#include <fcntl.h>
#include <poll.h>
#define INVALID_SOCKET -1
#define INVALID_EPOLL -1
#define SOCKET_ERROR -1
#define closesocket ::close
#define epoll_close ::close
#define SD_SEND SHUT_WR
#define SD_BOTH SHUT_RDWR
#endif
#define DNS_TIMEOUT 21600
#define CONNECT_TIMEOUT 2000
#define MAX_READ_UNTIL 512
#define CLOSE_TIMEOUT 10
#define SERVER_BLOCKED_WAIT_US 100
#pragma warning(push)
#pragma warning(disable: 4996)
#include <concurrentqueue.h>
#ifdef VI_OPENSSL
extern "C"
{
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <openssl/dh.h>
}
#undef hex_to_string
#endif

namespace vitex
{
	namespace network
	{
		static core::expects_io<socket_t> execute_socket(int net, int type, int protocol)
		{
			if (!core::os::control::has(core::access_option::net))
				return std::make_error_condition(std::errc::permission_denied);

			socket_t socket = (socket_t)::socket(net, type, protocol);
			if (socket == INVALID_SOCKET)
				return core::os::error::get_condition_or();

			return socket;
		}
		static core::expects_io<socket_t> execute_accept(socket_t fd, sockaddr* address, socket_size_t* address_length)
		{
			if (!core::os::control::has(core::access_option::net))
				return std::make_error_condition(std::errc::permission_denied);

			socket_t socket = (socket_t)accept(fd, address, address_length);
			if (socket == INVALID_SOCKET)
				return utils::get_last_error(nullptr, -1);

			return socket;
		}
		static core::expects_io<void> set_socket_blocking(socket_t fd, bool enabled)
		{
			VI_TRACE("[net] fd %i setopt: blocking %s", (int)fd, enabled ? "on" : "off");
#ifdef VI_MICROSOFT
			unsigned long mode = (enabled ? 0 : 1);
			if (ioctlsocket(fd, (long)FIONBIO, &mode) != 0)
				return core::os::error::get_condition_or();
#else
			int flags = fcntl(fd, F_GETFL, 0);
			if (flags == -1)
				return core::os::error::get_condition_or();

			flags = enabled ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
			if (fcntl(fd, F_SETFL, flags) == -1)
				return core::os::error::get_condition_or();
#endif
			return core::expectation::met;
		}
		static core::string get_address_identification(const socket_address& address)
		{
			core::string result;
			auto hostname = address.get_hostname();
			if (hostname)
				result.append(*hostname);

			auto port = address.get_ip_port();
			if (port)
				result.append(1, ':').append(core::to_string(*port));

			return result;
		}
		static core::string get_ip_address_identification(const socket_address& address)
		{
			core::string result;
			auto hostname = address.get_ip_address();
			if (hostname)
				result.append(*hostname);

			auto port = address.get_ip_port();
			if (port)
				result.append(1, ':').append(core::to_string(*port));

			return result;
		}
		static bool has_ip_v4_address(const std::string_view& hostname)
		{
			size_t index = 0;
			while (index < hostname.size())
			{
				char v = hostname[index++];
				if (!core::stringify::is_numeric(v) && v != '.')
					return false;
			}

			return true;
		}
		static bool has_ip_v6_address(const std::string_view& hostname)
		{
			size_t index = 0;
			while (index < hostname.size())
			{
				char v = hostname[index++];
				if (!core::stringify::is_numeric(v) && v != 'a' && v != 'b' && v != 'c' && v != 'd' && v != 'e' && v != 'f' && v != 'A' && v != 'B' && v != 'C' && v != 'D' && v != 'E' && v != 'F' && v != ':')
					return false;
			}

			return true;
		}
		static addrinfo* try_connect_dns(const core::unordered_map<socket_t, addrinfo*>& hosts, uint64_t timeout)
		{
			VI_MEASURE(core::timings::networking);

			core::vector<pollfd> sockets4, sockets6;
			for (auto& host : hosts)
			{
				VI_DEBUG("[net] resolve dns on fd %i", (int)host.first);
				set_socket_blocking(host.first, false);
				int status = connect(host.first, host.second->ai_addr, (int)host.second->ai_addrlen);
				if (status != 0 && utils::get_last_error(nullptr, status) != std::errc::operation_would_block)
					continue;

				pollfd fd;
				fd.fd = host.first;
				fd.events = POLLOUT;
				if (host.second->ai_family == AF_INET6)
					sockets6.push_back(fd);
				else
					sockets4.push_back(fd);
			}

			if (!sockets4.empty() && utils::poll(sockets4.data(), (int)sockets4.size(), (int)timeout) > 0)
			{
				for (auto& fd : sockets4)
				{
					if (fd.revents & POLLOUT)
					{
						auto it = hosts.find(fd.fd);
						if (it != hosts.end())
							return it->second;
					}
				}
			}

			if (!sockets6.empty() && utils::poll(sockets6.data(), (int)sockets6.size(), (int)timeout) > 0)
			{
				for (auto& fd : sockets6)
				{
					if (fd.revents & POLLOUT)
					{
						auto it = hosts.find(fd.fd);
						if (it != hosts.end())
							return it->second;
					}
				}
			}

			return nullptr;
		}
#ifdef VI_OPENSSL
		static std::pair<core::string, time_t> asn1_get_time(ASN1_TIME* time)
		{
			if (!time)
				return std::make_pair<core::string, time_t>(core::string(), time_t(0));

			struct tm date;
			memset(&date, 0, sizeof(date));
			ASN1_TIME_to_tm(time, &date);
			
			time_t time_stamp = mktime(&date);
			return std::make_pair(core::date_time::serialize_global(std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(time_stamp)), core::date_time::format_web_time()), time_stamp);
		}
#endif
#ifdef NET_POLL
		struct epoll_descriptor
		{
			pollfd fd;
			void* data = nullptr;
		};

		struct epoll_queue
		{
			core::unordered_map<socket_t, epoll_descriptor> fds;
			core::vector<pollfd> events;
			std::mutex mutex;
			std::atomic<uint8_t> iteration;
			socket_t outgoing;
			socket_t incoming;
			bool dirty;

			epoll_queue(size_t) : iteration(0), outgoing(INVALID_SOCKET), incoming(INVALID_SOCKET), dirty(true)
			{
				initialize().unwrap();
			}
			~epoll_queue()
			{
				if (incoming != INVALID_SOCKET)
					closesocket(incoming);
				if (outgoing != INVALID_SOCKET)
					closesocket(outgoing);
			}
			core::expects_system<void> initialize()
			{
				auto listenable = execute_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (!listenable)
					return core::system_exception("epoll initialize: listener initialization failed", std::move(listenable.error()));

				socket_t listener = *listenable; int reuse_address = 1;
				if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_address, sizeof(reuse_address)) != 0)
				{
					closesocket(listener);
					return core::system_exception("epoll initialize: listener configuration failed");
				}

				struct sockaddr_in address;
				memset(&address, 0, sizeof(address));
				address.sin_family = AF_INET;
				address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
				address.sin_port = 0;
				if (bind(listener, (struct sockaddr*)&address, sizeof(address)) != 0)
				{
					closesocket(listener);
					return core::system_exception("epoll initialize: listener bind failed");
				}
				else if (listen(listener, 1) != 0)
				{
					closesocket(listener);
					return core::system_exception("epoll initialize: listener listen failed");
				}

				int listener_address_size = sizeof(address);
				struct sockaddr listener_address;
				memset(&listener_address, 0, sizeof(listener_address));
				if (getsockname(listener, &listener_address, &listener_address_size) == -1)
				{
					closesocket(listener);
					return core::system_exception("epoll initialize: listener listen failed");
				}

				auto connectable = execute_socket(AF_INET, SOCK_STREAM, 0);
				if (!connectable)
				{
					closesocket(listener);
					return core::system_exception("epoll initialize: outgoing pipe initialization failed", std::move(connectable.error()));
				}

				outgoing = *connectable;
				if (connect(outgoing, &listener_address, listener_address_size) != 0)
				{
					closesocket(listener);
					closesocket(outgoing);
					return core::system_exception("epoll initialize: outgoing pipe connect failed");
				}

				auto acceptable = execute_accept(listener, nullptr, nullptr);
				closesocket(listener);
				if (acceptable)
				{
					incoming = *acceptable;
					set_socket_blocking(outgoing, false);
					set_socket_blocking(incoming, false);
					upsert(incoming, true, false, nullptr);
					return core::expectation::met;
				}

				closesocket(outgoing);
				return core::system_exception("epoll initialize: outgoing pipe accept failed", std::move(acceptable.error()));
			}
			bool upsert(socket_t fd, bool readable, bool writeable, void* data)
			{
				core::umutex<std::mutex> unique(mutex);
				auto it = fds.find(fd);
				if (it == fds.end())
				{
					epoll_descriptor event;
					event.fd.fd = fd;
#ifdef POLLRDHUP
					event.fd.events = POLLRDHUP;
#else
					event.fd.events = 0;
#endif
					it = fds.insert(std::make_pair(fd, event)).first;
					dirty = true;
				}

				auto& target = it->second;
				target.fd.revents = 0;

				if (readable)
				{
					if (!(target.fd.events & POLLIN))
					{
						target.fd.events |= POLLIN;
						dirty = true;
					}
				}
				else if (target.fd.events & POLLIN)
				{
					target.fd.events &= ~(POLLIN);
					dirty = true;
				}

				if (writeable)
				{
					if (!(target.fd.events & POLLOUT))
					{
						target.fd.events |= POLLOUT;
						dirty = true;
					}
				}
				else if (target.fd.events & POLLOUT)
				{
					target.fd.events &= ~(POLLOUT);
					dirty = true;
				}

				if (target.data != data)
				{
					target.data = data;
					dirty = true;
				}

				if (dirty)
				{
					uint8_t buffer = iteration++;
					send(outgoing, (char*)&buffer, 1, 0);
				}

				return true;
			}
			void* unwrap(pollfd& event)
			{
				if (!event.events || !event.revents)
					return nullptr;

				if (event.fd == incoming)
				{
					if (event.revents & POLLIN)
					{
						uint8_t buffer;
						while (recv(incoming, (char*)&buffer, 1, 0) > 0);
					}
					return nullptr;
				}

				core::umutex<std::mutex> unique(mutex);
				auto it = fds.find(event.fd);
				if (it == fds.end())
					return nullptr;
				else if (!it->second.fd.events)
					return nullptr;

				return it->second.data;
			}
			core::vector<pollfd>& compile()
			{
				core::umutex<std::mutex> unique(mutex);
				if (!dirty)
					return events;

				events.clear();
				events.reserve(fds.size());
				for (auto it = fds.begin(); it != fds.end();)
				{
					if (it->second.fd.events > 0)
					{
						events.emplace_back(it->second.fd);
						++it;
					}
					else
						fds.erase(it++);
				}

				dirty = false;
				return events;
			}
		};
#elif defined(NET_KQUEUE)
		struct epoll_queue
		{
			struct kevent* data;
			size_t size;

			epoll_queue(size_t new_size) : size(new_size)
			{
				data = core::memory::allocate<struct kevent>(sizeof(struct kevent) * size);
			}
			~epoll_queue()
			{
				core::memory::deallocate(data);
			}
		};
#elif defined(NET_EPOLL)
		struct epoll_queue
		{
			epoll_event* data;
			size_t size;

			epoll_queue(size_t new_size) : size(new_size)
			{
				data = core::memory::allocate<epoll_event>(sizeof(epoll_event) * size);
			}
			~epoll_queue()
			{
				core::memory::deallocate(data);
			}
		};
#endif
		location::location(const std::string_view& from) noexcept : body(from), port(0)
		{
			VI_ASSERT(!body.empty(), "location should not be empty");
			core::stringify::replace(body, '\\', '/');

			const char* parameters_begin = nullptr;
			const char* path_begin = nullptr;
			const char* host_begin = strchr(body.c_str(), ':');
			if (host_begin != nullptr)
			{
				if (strncmp(host_begin, "://", 3) != 0)
				{
					while (*host_begin != '\0' && *host_begin != '/' && *host_begin != ':' && *host_begin != '?' && *host_begin != '#')
						++host_begin;

					path_begin = *host_begin == '\0' || *host_begin == '/' ? host_begin : host_begin + 1;
					protocol = core::string(body.c_str(), host_begin);
					goto inline_url;
				}
				else
				{
					protocol = core::string(body.c_str(), host_begin);
					host_begin += 3;
				}
			}
			else
				host_begin = body.c_str();

			if (host_begin != body.c_str())
			{
				const char* at_symbol = strchr(host_begin, '@');
				path_begin = strchr(host_begin, '/');

				if (at_symbol != nullptr && (path_begin == nullptr || at_symbol < path_begin))
				{
					core::string login_password;
					login_password = core::string(host_begin, at_symbol);
					host_begin = at_symbol + 1;
					path_begin = strchr(host_begin, '/');

					const char* password_ptr = strchr(login_password.c_str(), ':');
					if (password_ptr)
					{
						username = compute::codec::url_decode(core::string(login_password.c_str(), password_ptr));
						password = compute::codec::url_decode(core::string(password_ptr + 1));
					}
					else
						username = compute::codec::url_decode(login_password);
				}

				const char* ip_v6_end = strchr(host_begin, ']');
				const char* port_begin = strchr(ip_v6_end ? ip_v6_end : host_begin, ':');
				if (port_begin != nullptr && (path_begin == nullptr || port_begin < path_begin))
				{
					if (1 != sscanf(port_begin, ":%" SCNd16, &port))
						goto finalize_url;

					hostname = core::string(host_begin + (ip_v6_end ? 1 : 0), ip_v6_end ? ip_v6_end : port_begin);
					if (!path_begin)
						goto finalize_url;
				}
				else
				{
					if (path_begin == nullptr)
					{
						hostname = host_begin;
						goto finalize_url;
					}

					hostname = core::string(host_begin + (ip_v6_end ? 1 : 0), ip_v6_end ? ip_v6_end : path_begin);
				}
			}
			else
				path_begin = body.c_str();

		inline_url:
			parameters_begin = strchr(path_begin, '?');
			if (parameters_begin != nullptr)
			{
				const char* parameters_end = strchr(++parameters_begin, '#');
				core::string parameters(parameters_begin, parameters_end ? parameters_end - parameters_begin : strlen(parameters_begin));
				path = core::string(path_begin, parameters_begin - 1);

				if (!parameters_end)
				{
					parameters_end = strchr(path.c_str(), '#');
					if (parameters_end != nullptr && parameters_end > path.c_str())
					{
						fragment = parameters_end + 1;
						path = core::string(path.c_str(), parameters_end);
					}
				}
				else
					fragment = parameters_end + 1;

				for (auto& item : core::stringify::split(parameters, '&'))
				{
					core::vector<core::string> key_value = core::stringify::split(item, '=');
					key_value[0] = compute::codec::url_decode(key_value[0]);

					if (key_value.size() >= 2)
						query[key_value[0]] = compute::codec::url_decode(key_value[1]);
					else
						query[key_value[0]] = "";
				}
			}
			else
			{
				const char* parameters_end = strchr(path_begin, '#');
				if (parameters_end != nullptr)
				{
					fragment = parameters_end + 1;
					path = core::string(path_begin, parameters_end);
				}
				else
					path = path_begin;
			}

		finalize_url:
			if (protocol.size() < 2)
			{
				path = protocol + ':' + path;
				protocol = "file";
			}

			if (!path.empty())
				path.assign(compute::codec::url_decode(path));
			else
				path.assign("/");

			if (port > 0)
				return;

			if (protocol == "http" || protocol == "ws")
				port = 80;
			else if (protocol == "https" || protocol == "wss")
				port = 443;
			else if (protocol == "ftp")
				port = 21;
			else if (protocol == "gopher")
				port = 70;
			else if (protocol == "imap")
				port = 143;
			else if (protocol == "idap")
				port = 389;
			else if (protocol == "nfs")
				port = 2049;
			else if (protocol == "nntp")
				port = 119;
			else if (protocol == "pop")
				port = 110;
			else if (protocol == "smtp")
				port = 25;
			else if (protocol == "telnet")
				port = 23;

			if (port > 0 || protocol == "file")
				return;

			servent* entry = getservbyname(protocol.c_str(), nullptr);
			if (entry != nullptr)
				port = entry->s_port;
		}

		x509_blob::x509_blob(void* new_pointer) noexcept : pointer(new_pointer)
		{
		}
		x509_blob::x509_blob(const x509_blob& other) noexcept : pointer(nullptr)
		{
#ifdef VI_OPENSSL
			if (other.pointer != nullptr)
				pointer = X509_dup((X509*)other.pointer);
#endif
		}
		x509_blob::x509_blob(x509_blob&& other) noexcept : pointer(other.pointer)
		{
			other.pointer = nullptr;
		}
		x509_blob::~x509_blob() noexcept
		{
#ifdef VI_OPENSSL
			if (pointer != nullptr)
				X509_free((X509*)pointer);
#endif
		}
		x509_blob& x509_blob::operator= (const x509_blob& other) noexcept
		{
			if (this == &other)
				return *this;

#ifdef VI_OPENSSL
			if (pointer != nullptr)
				X509_free((X509*)pointer);
			if (other.pointer != nullptr)
				pointer = X509_dup((X509*)other.pointer);
#endif
			return *this;
		}
		x509_blob& x509_blob::operator= (x509_blob&& other) noexcept
		{
			if (this == &other)
				return *this;

			pointer = other.pointer;
			other.pointer = nullptr;
			return *this;
		}

		core::unordered_map<core::string, core::vector<core::string>> certificate::get_full_extensions() const
		{
			core::unordered_map<core::string, core::vector<core::string>> data;
			data.reserve(extensions.size());

			for (auto& item : extensions)
				data[item.first] = core::stringify::split(item.second, '\n');

			return data;
		}

		bool socket_cidr::is_matching(const compute::uint128& target_value)
		{
			return target_value == value || (target_value >= min_value && target_value <= max_value);
		}

		socket_address::socket_address() noexcept : address_size(0)
		{
			info.port = 0;
			info.flags = 0;
			info.family = AF_INET;
			info.type = SOCK_STREAM;
			info.protocol = IPPROTO_IP;
			memset(address_buffer, 0, sizeof(address_buffer));
		}
		socket_address::socket_address(const std::string_view& ip_address, uint16_t port) noexcept : socket_address()
		{
			sockaddr_in address4;
			memset(&address4, 0, sizeof(address4));
			address4.sin_family = AF_INET;
			address4.sin_port = htons(port);

			sockaddr_in6 address6;
			memset(&address6, 0, sizeof(address6));
			address6.sin6_family = AF_INET6;
			address6.sin6_port = htons(port);

			if (inet_pton(AF_INET, ip_address.data(), &address4.sin_addr) != 1)
			{
				if (inet_pton(AF_INET6, ip_address.data(), &address6.sin6_addr) == 1)
				{
					info.hostname = core::string(ip_address);
					info.port = port;
					info.family = AF_INET6;
					address_size = sizeof(address6);
					memcpy(address_buffer, &address6, address_size);
				}
			}
			else
			{
				info.hostname = core::string(ip_address);
				info.port = port;
				address_size = sizeof(address4);
				memcpy(address_buffer, &address4, address_size);
			}

			size_t leftovers = sizeof(address_buffer) - address_size;
			if (leftovers > 0)
				memset(address_buffer + address_size, 0, leftovers);
		}
		socket_address::socket_address(const std::string_view& hostname, uint16_t port, addrinfo* address_info) noexcept
		{
			VI_ASSERT(address_info != nullptr, "address info should be set");
			if (!hostname.empty())
				info.hostname = core::string(hostname);
			else if (address_info->ai_canonname != nullptr)
				info.hostname = address_info->ai_canonname;
			info.port = port;
			info.flags = address_info->ai_flags;
			info.family = address_info->ai_family;
			info.type = address_info->ai_socktype;
			info.protocol = address_info->ai_protocol;
			address_size = std::min<size_t>(sizeof(address_buffer), address_info->ai_addrlen);
			switch (address_info->ai_family)
			{
				case AF_INET:
					memcpy(address_buffer, (sockaddr_in*)address_info->ai_addr, address_size);
					break;
				case AF_INET6:
					memcpy(address_buffer, (sockaddr_in6*)address_info->ai_addr, address_size);
					break;
				default:
					memcpy(address_buffer, address_info->ai_addr, address_size);
					break;
			}

			size_t leftovers = sizeof(address_buffer) - address_size;
			if (leftovers > 0)
				memset(address_buffer + address_size, 0, leftovers);
		}
		socket_address::socket_address(const std::string_view& hostname, uint16_t port, sockaddr* address, size_t new_address_size) noexcept : socket_address()
		{
			VI_ASSERT(address != nullptr, "address should be set");
			if (!hostname.empty())
				info.hostname = core::string(hostname);
			info.port = port;
			info.family = (int32_t)address->sa_family;
			address_size = std::min<size_t>(sizeof(address_buffer), new_address_size);
			memcpy(address_buffer, address, address_size);
		}
		const sockaddr_in* socket_address::get_address4() const noexcept
		{
			auto* raw = get_raw_address();
			return raw->sa_family == AF_INET ? (sockaddr_in*)address_buffer : nullptr;
		}
		const sockaddr_in6* socket_address::get_address6() const noexcept
		{
			auto* raw = get_raw_address();
			return raw->sa_family == AF_INET6 ? (sockaddr_in6*)address_buffer : nullptr;
		}
		const sockaddr* socket_address::get_raw_address() const noexcept
		{
			return (sockaddr*)address_buffer;
		}
		size_t socket_address::get_address_size() const noexcept
		{
			return address_size;
		}
		int32_t socket_address::get_flags() const noexcept
		{
			return info.flags;
		}
		int32_t socket_address::get_family() const noexcept
		{
			return info.family;
		}
		int32_t socket_address::get_type() const noexcept
		{
			return info.type;
		}
		int32_t socket_address::get_protocol() const noexcept
		{
			return info.protocol;
		}
		dns_type socket_address::get_resolver_type() const noexcept
		{
			return (info.flags & AI_PASSIVE ? dns_type::listen : dns_type::connect);
		}
		socket_protocol socket_address::get_protocol_type() const noexcept
		{
			switch (info.protocol)
			{
				case IPPROTO_IP:
					return socket_protocol::IP;
				case IPPROTO_ICMP:
					return socket_protocol::ICMP;
				case IPPROTO_UDP:
					return socket_protocol::UDP;
				case IPPROTO_RAW:
					return socket_protocol::RAW;
				case IPPROTO_TCP:
				default:
					return socket_protocol::TCP;
			}
		}
		socket_type socket_address::get_socket_type() const noexcept
		{
			switch (info.type)
			{
				case SOCK_DGRAM:
					return socket_type::datagram;
				case SOCK_RAW:
					return socket_type::raw;
				case SOCK_RDM:
					return socket_type::reliably_delivered_message;
				case SOCK_SEQPACKET:
					return socket_type::sequence_packet_stream;
				case SOCK_STREAM:
				default:
					return socket_type::stream;
			}
		}
		bool socket_address::is_valid() const noexcept
		{
			return address_size >= sizeof(sockaddr);
		}
		core::expects_io<core::string> socket_address::get_hostname() const noexcept
		{
			if (!info.hostname.empty())
			{
				size_t offset = info.hostname.rfind(':');
				if (!offset || offset == std::string::npos)
					return info.hostname;

				auto source_name = info.hostname.substr(0, offset);
				if (!source_name.empty())
					return source_name;
			}

			return get_ip_address();
		}
		core::expects_io<core::string> socket_address::get_ip_address() const noexcept
		{
			char buffer[NI_MAXHOST];
			if (getnameinfo(get_raw_address(), (socklen_t)get_address_size(), buffer, sizeof(buffer), nullptr, 0, NI_NUMERICHOST) != 0)
				return core::os::error::get_condition_or(std::errc::host_unreachable);

			return core::string(buffer, strnlen(buffer, sizeof(buffer)));
		}
		core::expects_io<uint16_t> socket_address::get_ip_port() const noexcept
		{
			if (info.port > 0)
				return info.port;

			const sockaddr_in* address4 = get_address4();
			if (address4 != nullptr)
				return (uint16_t)ntohs(address4->sin_port);

			const sockaddr_in6* address6 = get_address6();
			if (address6 != nullptr)
				return (uint16_t)ntohs(address6->sin6_port);

			return std::make_error_condition(std::errc::address_family_not_supported);
		}
		core::expects_io<compute::uint128> socket_address::get_ip_value() const noexcept
		{
			const sockaddr_in* address4 = get_address4();
			if (address4 != nullptr)
			{
				uint32_t input = 0;
				memcpy((char*)&input, (char*)&address4->sin_addr, sizeof(input));
				return compute::uint128(core::os::hw::to_endianness(core::os::hw::endian::big, input));
			}

			const sockaddr_in6* address6 = get_address6();
			if (address6 != nullptr)
			{
				uint64_t input_h = 0, input_l = 0;
				memcpy((char*)&input_h, (char*)&address6->sin6_addr + sizeof(input_h) * 0, sizeof(input_h));
				memcpy((char*)&input_l, (char*)&address6->sin6_addr + sizeof(input_l) * 1, sizeof(input_l));
				return compute::uint128(core::os::hw::to_endianness(core::os::hw::endian::big, input_h), core::os::hw::to_endianness(core::os::hw::endian::big, input_l));
			}

			return std::make_error_condition(std::errc::address_family_not_supported);
		}

		data_frame& data_frame::operator= (const data_frame& other)
		{
			VI_ASSERT(this != &other, "this should not be other");
			message = other.message;
			reuses = other.reuses;
			start = other.start;
			finish = other.finish;
			timeout = other.timeout;
			abort = other.abort;
			return *this;
		}

		epoll_interface::epoll_interface(size_t max_events) noexcept
		{
			VI_ASSERT(max_events > 0, "array size should be greater than zero");
			queue = core::memory::init<epoll_queue>(max_events);
#ifdef NET_POLL
			handle = (epoll_handle)(uintptr_t)std::numeric_limits<size_t>::max();
#elif defined(NET_KQUEUE)
			handle = kqueue();
#elif defined(NET_EPOLL)
			handle = epoll_create(1);
#endif
		}
		epoll_interface::epoll_interface(epoll_interface&& other) noexcept : queue(other.queue), handle(other.handle)
		{
			other.queue = nullptr;
			other.handle = INVALID_EPOLL;
		}
		epoll_interface::~epoll_interface() noexcept
		{
			core::memory::deinit(queue);
#ifdef NET_POLL
			handle = INVALID_EPOLL;
#else
			if (handle != INVALID_EPOLL)
			{
				epoll_close(handle);
				handle = INVALID_EPOLL;
			}
#endif
		}
		epoll_interface& epoll_interface::operator= (epoll_interface&& other) noexcept
		{
			if (this == &other)
				return *this;

			this->~epoll_interface();
			queue = other.queue;
			handle = other.handle;
			other.handle = INVALID_EPOLL;
			other.queue = nullptr;
			return *this;
		}
		bool epoll_interface::add(socket* fd, bool readable, bool writeable) noexcept
		{
			VI_ASSERT(handle != INVALID_EPOLL, "epoll should be initialized");
			VI_ASSERT(fd != nullptr && fd->fd != INVALID_SOCKET, "socket should be set and valid");
			VI_ASSERT(readable || writeable, "add should set readable and/or writeable");
			VI_TRACE("[net] epoll add fd %i c%s%s", (int)fd->fd, readable ? "r" : "", writeable ? "w" : "");
#ifdef NET_POLL
			VI_ASSERT(queue != nullptr, "epoll should be initialized");
			return queue->upsert(fd->fd, readable, writeable, (void*)fd);
#elif defined(NET_KQUEUE)
			struct kevent read_event;
			EV_SET(&read_event, fd->fd, EVFILT_READ, EV_ADD, 0, 0, (void*)fd);
			int result1 = readable ? kevent(handle, &read_event, 1, nullptr, 0, nullptr) : 0;

			struct kevent write_event;
			EV_SET(&write_event, fd->fd, EVFILT_WRITE, EV_ADD, 0, 0, (void*)fd);
			int result2 = writeable ? kevent(handle, &write_event, 1, nullptr, 0, nullptr) : 0;
			return result1 != -1 && result2 != -1;
#elif defined(NET_EPOLL)
			epoll_event event;
			event.data.ptr = (void*)fd;
#ifdef EPOLLRDHUP
			event.events = EPOLLRDHUP;
#else
			event.events = 0;
#endif
			if (readable)
				event.events |= EPOLLIN;
			if (writeable)
				event.events |= EPOLLOUT;
			return epoll_ctl(handle, EPOLL_CTL_ADD, fd->fd, &event) == 0;
#endif
		}
		bool epoll_interface::update(socket* fd, bool readable, bool writeable) noexcept
		{
			VI_ASSERT(handle != INVALID_EPOLL, "epoll should be initialized");
			VI_ASSERT(fd != nullptr && fd->fd != INVALID_SOCKET, "socket should be set and valid");
			VI_ASSERT(readable || writeable, "update should set readable and/or writeable");
			VI_TRACE("[net] epoll update fd %i c%s%s", (int)fd->fd, readable ? "r" : "", writeable ? "w" : "");
#ifdef NET_POLL
			VI_ASSERT(queue != nullptr, "epoll should be initialized");
			return queue->upsert(fd->fd, readable, writeable, (void*)fd);
#elif defined(NET_KQUEUE)
			struct kevent read_event;
			EV_SET(&read_event, fd->fd, EVFILT_READ, EV_ADD, 0, 0, (void*)fd);
			int result1 = readable ? kevent(handle, &read_event, 1, nullptr, 0, nullptr) : 0;

			struct kevent write_event;
			EV_SET(&write_event, fd->fd, EVFILT_WRITE, EV_ADD, 0, 0, (void*)fd);
			int result2 = writeable ? kevent(handle, &write_event, 1, nullptr, 0, nullptr) : 0;
			return result1 != -1 && result2 != -1;
#elif defined(NET_EPOLL)
			epoll_event event;
			event.data.ptr = (void*)fd;
#ifdef EPOLLRDHUP
			event.events = EPOLLRDHUP;
#else
			event.events = 0;
#endif
			if (readable)
				event.events |= EPOLLIN;
			if (writeable)
				event.events |= EPOLLOUT;
			return epoll_ctl(handle, EPOLL_CTL_MOD, fd->fd, &event) == 0;
#endif
		}
		bool epoll_interface::remove(socket* fd) noexcept
		{
			VI_ASSERT(handle != INVALID_EPOLL, "epoll should be initialized");
			VI_ASSERT(fd != nullptr && fd->fd != INVALID_SOCKET, "socket should be set and valid");
			VI_TRACE("[net] epoll remove fd %i", (int)fd->fd);
#ifdef NET_POLL
			VI_ASSERT(queue != nullptr, "epoll should be initialized");
			queue->upsert(fd->fd, false, false, (void*)fd);
			return true;
#elif defined(NET_KQUEUE)
			struct kevent event;
			EV_SET(&event, fd->fd, EVFILT_READ, EV_DELETE, 0, 0, (void*)nullptr);
			int result1 = kevent(handle, &event, 1, nullptr, 0, nullptr);

			EV_SET(&event, fd->fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void*)nullptr);
			int result2 = kevent(handle, &event, 1, nullptr, 0, nullptr);
			return result1 != -1 && result2 != -1;
#elif defined(NET_EPOLL)
			epoll_event event;
			event.data.ptr = (void*)fd;
#ifdef EPOLLRDHUP
			event.events = EPOLLRDHUP | EPOLLIN | EPOLLOUT;
#else
			event.events = EPOLLIN | EPOLLOUT;
#endif
			return epoll_ctl(handle, EPOLL_CTL_DEL, fd->fd, &event) == 0;
#endif
		}
		int epoll_interface::wait(epoll_fd* data, size_t data_size, uint64_t timeout) noexcept
		{
			VI_ASSERT(capacity() >= data_size, "epollfd array should be less than or equal to internal events buffer");
#ifdef NET_POLL
			auto& events = queue->compile();
			if (events.empty())
				return 0;

			VI_TRACE("[net] poll wait %i fds (%" PRIu64 " ms)", (int)data_size, timeout);
			int count = utils::poll(events.data(), (int)events.size(), (int)timeout);
			if (count <= 0)
				return count;

			size_t incoming = 0;
			for (auto& event : events)
			{
				socket* target = (socket*)queue->unwrap(event);
				if (!target)
					continue;

				auto& fd = data[incoming++];
				fd.base = target;
				fd.readable = (event.revents & POLLIN);
				fd.writeable = (event.revents & POLLOUT);
#ifdef POLLRDHUP
				fd.closeable = (event.revents & POLLHUP || event.revents & POLLRDHUP || event.revents & POLLNVAL || event.revents & POLLERR);
#else
				fd.closeable = (event.revents & POLLHUP || event.revents & POLLNVAL || event.revents & POLLERR);
#endif
			}
			VI_TRACE("[net] poll recv %i events", (int)incoming);
#elif defined(NET_KQUEUE)
			VI_TRACE("[net] kqueue wait %i fds (%" PRIu64 " ms)", (int)data_size, timeout);
			struct timespec wait;
			wait.tv_sec = (int)timeout / 1000;
			wait.tv_nsec = ((int)timeout % 1000) * 1000000;

			int count = kevent(handle, nullptr, 0, queue->data, (int)data_size, &wait);
			if (count <= 0)
				return count;

			size_t incoming = 0;
			for (auto it = queue->data; it != queue->data + count; it++)
			{
				auto& fd = data[incoming++];
				fd.base = (socket*)it->udata;
				fd.readable = (it->filter == EVFILT_READ);
				fd.writeable = (it->filter == EVFILT_WRITE);
				fd.closeable = (it->flags & EV_EOF);
			}
			VI_TRACE("[net] kqueue recv %i events", (int)incoming);
#elif defined(NET_EPOLL)
			VI_TRACE("[net] epoll wait %i fds (%" PRIu64 " ms)", (int)data_size, timeout);
			int count = epoll_wait(handle, queue->data, (int)data_size, (int)timeout);
			if (count <= 0)
				return count;

			size_t incoming = 0;
			for (auto it = queue->data; it != queue->data + count; it++)
			{
				auto& fd = data[incoming++];
				fd.base = (socket*)it->data.ptr;
				fd.readable = (it->events & EPOLLIN);
				fd.writeable = (it->events & EPOLLOUT);
#ifdef EPOLLRDHUP
				fd.closeable = (it->events & EPOLLHUP || it->events & EPOLLRDHUP || it->events & EPOLLERR);
#else
				fd.closeable = (it->events & EPOLLHUP || it->events & EPOLLERR);
#endif
			}
			VI_TRACE("[net] epoll recv %i events", (int)incoming);
#endif
			return (int)incoming;
		}
		size_t epoll_interface::capacity() noexcept
		{
#ifdef NET_POLL
			return std::numeric_limits<size_t>::max() / sizeof(epoll_fd);
#else
			VI_ASSERT(queue != nullptr, "epoll should be initialized");
			return queue->size;
#endif
		}

		core::expects_io<certificate_blob> utils::generate_self_signed_certificate(uint32_t days, const std::string_view& addresses_comma_separated, const std::string_view& domains_comma_separated) noexcept
		{
			core::uptr<certificate_builder> builder = new certificate_builder();
			builder->set_serial_number();
			builder->set_version(2);
			builder->set_not_before();
			builder->set_not_after(86400 * days);
			builder->set_public_key();
			builder->add_subject_info("CN", "Self-signed certificate");
			builder->add_issuer_info("CN", "CA for self-signed certificates");

			core::string alternative_names;
			for (auto& domain : core::stringify::split(domains_comma_separated, ','))
				alternative_names += "dns: " + domain + ", ";

			for (auto& address : core::stringify::split(addresses_comma_separated, ','))
				alternative_names += "IP: " + address + ", ";

			if (!alternative_names.empty())
			{
				alternative_names.erase(alternative_names.size() - 2, 2);
				builder->add_standard_extension(nullptr, "subjectAltName", alternative_names.c_str());
			}

			builder->sign(compute::digests::sha256());
			return builder->build();
		}
		core::expects_io<certificate> utils::get_certificate_from_x509(void* certificate_x509) noexcept
		{
			VI_ASSERT(certificate_x509 != nullptr, "certificate should be set");
			VI_MEASURE(core::timings::networking);
#ifdef VI_OPENSSL
			X509* blob = (X509*)certificate_x509;
			X509_NAME* subject = X509_get_subject_name(blob);
			X509_NAME* issuer = X509_get_issuer_name(blob);
			ASN1_INTEGER* serial = X509_get_serialNumber(blob);
			auto not_before = asn1_get_time(X509_get_notBefore(blob));
			auto not_after = asn1_get_time(X509_get_notAfter(blob));

			certificate output;
			output.version = (int32_t)X509_get_version(blob);
			output.signature = X509_get_signature_type(blob);
			output.not_before_date = not_before.first;
			output.not_before_time = not_before.second;
			output.not_after_date = not_after.first;
			output.not_after_time = not_after.second;
			output.blob.pointer = blob;

			int extensions = X509_get_ext_count(blob);
			output.extensions.reserve((size_t)extensions);

			for (int i = 0; i < extensions; i++)
			{
				core::string name, value;
				X509_EXTENSION* extension = X509_get_ext(blob, i);
				ASN1_OBJECT* object = X509_EXTENSION_get_object(extension);

				BIO* extension_bio = BIO_new(BIO_s_mem());
				if (extension_bio != nullptr)
				{
					BUF_MEM* extension_memory = nullptr;
					i2a_ASN1_OBJECT(extension_bio, object);
					if (BIO_get_mem_ptr(extension_bio, &extension_memory) != 0 && extension_memory->data != nullptr && extension_memory->length > 0)
						name = core::string(extension_memory->data, (size_t)extension_memory->length);
					BIO_free(extension_bio);
				}

				extension_bio = BIO_new(BIO_s_mem());
				if (extension_bio != nullptr)
				{
					BUF_MEM* extension_memory = nullptr;
					X509V3_EXT_print(extension_bio, extension, 0, 0);
					if (BIO_get_mem_ptr(extension_bio, &extension_memory) != 0 && extension_memory->data != nullptr && extension_memory->length > 0)
						value = core::string(extension_memory->data, (size_t)extension_memory->length);
					BIO_free(extension_bio);
				}

				output.extensions[name] = std::move(value);
			}

			char subject_buffer[core::CHUNK_SIZE];
			X509_NAME_oneline(subject, subject_buffer, (int)sizeof(subject_buffer));
			output.subject_name = subject_buffer;

			char issuer_buffer[core::CHUNK_SIZE], serial_buffer[core::CHUNK_SIZE];
			X509_NAME_oneline(issuer, issuer_buffer, (int)sizeof(issuer_buffer));
			output.issuer_name = issuer_buffer;

			uint8_t buffer[256];
			int length = i2d_ASN1_INTEGER(serial, nullptr);
			if (length > 0 && (unsigned)length < (unsigned)sizeof(buffer))
			{
				uint8_t* pointer = buffer;
				if (!compute::codec::hex_to_string(std::string_view((char*)buffer, (uint64_t)i2d_ASN1_INTEGER(serial, &pointer)), serial_buffer, sizeof(serial_buffer)))
					*serial_buffer = '\0';
			}
			else
				*serial_buffer = '\0';
			output.serial_number = serial_buffer;

			const EVP_MD* digest = EVP_get_digestbyname("sha256");
			uint32_t buffer_length = sizeof(buffer) - 1;
			X509_digest(blob, digest, buffer, &buffer_length);
			output.fingerprint = compute::codec::hex_encode(core::string((const char*)buffer, buffer_length));

			X509_pubkey_digest(blob, digest, buffer, &buffer_length);
			output.public_key = compute::codec::hex_encode(core::string((const char*)buffer, buffer_length));

			return output;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::vector<core::string> utils::get_host_ip_addresses() noexcept
		{
			core::vector<core::string> ip_addresses;
			ip_addresses.push_back("127.0.0.1");

			char hostname[core::CHUNK_SIZE];
			struct addrinfo* addresses = nullptr;
			if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR)
				return ip_addresses;

			struct addrinfo hints;
			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family = AF_INET;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_socktype = SOCK_STREAM;

			if (getaddrinfo(hostname, nullptr, &hints, &addresses) == 0)
			{
				for (auto it = addresses; it != nullptr; it = it->ai_next)
				{
					char ip_address[INET_ADDRSTRLEN];
					if (inet_ntop(it->ai_family, &(((struct sockaddr_in*)it->ai_addr)->sin_addr), ip_address, sizeof(ip_address)))
						ip_addresses.push_back(core::string(ip_address, strnlen(ip_address, sizeof(ip_address))));
				}
				freeaddrinfo(addresses);
			}

			hints.ai_family = AF_INET6;
			if (getaddrinfo(hostname, nullptr, &hints, &addresses) == 0)
			{
				for (auto it = addresses; it != nullptr; it = it->ai_next)
				{
					char ip_address[INET6_ADDRSTRLEN];
					if (inet_ntop(it->ai_family, &(((struct sockaddr_in6*)it->ai_addr)->sin6_addr), ip_address, sizeof(ip_address)))
						ip_addresses.push_back(core::string(ip_address, strnlen(ip_address, sizeof(ip_address))));
				}
				freeaddrinfo(addresses);
			}

			return ip_addresses;
		}
		int utils::poll(pollfd* fd, int fd_count, int timeout) noexcept
		{
			VI_ASSERT(fd != nullptr, "poll should be set");
			VI_TRACE("[net] poll wait %i fds (%i ms)", fd_count, timeout);
#if defined(VI_MICROSOFT)
			int count = WSAPoll(fd, fd_count, timeout);
#else
			int count = poll(fd, fd_count, timeout);
#endif
			if (count > 0)
				VI_TRACE("[net] poll recv %i events", count);
			return count;
		}
		int utils::poll(poll_fd* fd, int fd_count, int timeout) noexcept
		{
			VI_ASSERT(fd != nullptr, "poll should be set");
			core::vector<pollfd> fds;
			fds.resize(fd_count);

			for (size_t i = 0; i < (size_t)fd_count; i++)
			{
				auto& next = fds[i];
				next.revents = 0;
				next.events = 0;

				auto& base = fd[i];
				next.fd = base.fd;

				if (base.events & input_normal)
					next.events |= POLLRDNORM;
				if (base.events & input_band)
					next.events |= POLLRDBAND;
				if (base.events & input_priority)
					next.events |= POLLPRI;
				if (base.events & input)
					next.events |= POLLIN;
				if (base.events & output_normal)
					next.events |= POLLWRNORM;
				if (base.events & output_band)
					next.events |= POLLWRBAND;
				if (base.events & output)
					next.events |= POLLOUT;
				if (base.events & error)
					next.events |= POLLERR;
#ifdef POLLRDHUP
				if (base.events & hangup)
					next.events |= POLLRDHUP;
#endif
			}

			int size = poll(fds.data(), fd_count, timeout);
			for (size_t i = 0; i < (size_t)fd_count; i++)
			{
				auto& next = fd[i];
				next.returns = 0;

				auto& base = fds[i];
				if (base.revents & POLLRDNORM)
					next.returns |= input_normal;
				if (base.revents & POLLRDBAND)
					next.returns |= input_band;
				if (base.revents & POLLPRI)
					next.returns |= input_priority;
				if (base.revents & POLLIN)
					next.returns |= input;
				if (base.revents & POLLWRNORM)
					next.returns |= output_normal;
				if (base.revents & POLLWRBAND)
					next.returns |= output_band;
				if (base.revents & POLLOUT)
					next.returns |= output;
				if (base.revents & POLLERR)
					next.returns |= error;
#ifdef POLLRDHUP
				if (base.revents & POLLRDHUP || base.revents & POLLHUP)
					next.returns |= hangup;
#else
				if (base.revents & POLLHUP)
					next.returns |= hangup;
#endif
			}

			return size;
		}
		std::error_condition utils::get_last_error(ssl_st* device, int error_code) noexcept
		{
#ifdef VI_OPENSSL
			if (device != nullptr)
			{
				error_code = SSL_get_error(device, error_code);
				switch (error_code)
				{
					case SSL_ERROR_WANT_READ:
					case SSL_ERROR_WANT_WRITE:
					case SSL_ERROR_WANT_CONNECT:
					case SSL_ERROR_WANT_ACCEPT:
					case SSL_ERROR_WANT_ASYNC:
					case SSL_ERROR_WANT_ASYNC_JOB:
					case SSL_ERROR_WANT_X509_LOOKUP:
					case SSL_ERROR_WANT_CLIENT_HELLO_CB:
					case SSL_ERROR_WANT_RETRY_VERIFY:
						return std::make_error_condition(std::errc::operation_would_block);
					case SSL_ERROR_SSL:
						utils::display_transport_log();
						return std::make_error_condition(std::errc::protocol_error);
					case SSL_ERROR_ZERO_RETURN:
						utils::display_transport_log();
						return std::make_error_condition(std::errc::bad_message);
					case SSL_ERROR_SYSCALL:
						utils::display_transport_log();
						return std::make_error_condition(std::errc::io_error);
					case SSL_ERROR_NONE:
					default:
						utils::display_transport_log();
						return std::make_error_condition(std::errc());
				}
			}
#endif
#ifdef VI_MICROSOFT
			error_code = WSAGetLastError();
#else
			error_code = errno;
#endif
			switch (error_code)
			{
#ifdef VI_MICROSOFT
				case WSAEWOULDBLOCK:
#endif
				case EWOULDBLOCK:
				case EINPROGRESS:
					return std::make_error_condition(std::errc::operation_would_block);
				case 0:
					return std::make_error_condition(std::errc::connection_reset);
				default:
					return core::os::error::get_condition(error_code);
			}
		}
		core::option<socket_cidr> utils::parse_address_mask(const std::string_view& mask) noexcept
		{
			auto is_ip_v4 = core::stringify::find(mask, '.');
			auto is_ip_v6 = core::stringify::find(mask, ':');
			auto is_mask = core::stringify::find(mask, '/');
			if (mask.empty() || !is_mask.found || (!is_ip_v4.found && !is_ip_v6.found) || (is_ip_v4.found && is_ip_v6.found))
				return core::optional::none;

			auto range = core::from_string<uint8_t>(mask.substr(is_mask.end));
			if (!range || (is_ip_v4.found && *range > 32) || (is_ip_v6.found && *range > 128))
				return core::optional::none;

			auto address = mask.substr(0, is_mask.start);
			if (is_ip_v6.found)
			{
				auto blocks = core::stringify::split(address, ':');
				if (blocks.size() > 8)
					return core::optional::none;

				auto a = core::os::hw::to_endianness(core::os::hw::endian::big, blocks.size() < 1 ? uint16_t(0) : core::from_string<uint16_t>(blocks[0], 16).or_else(0));
				auto b = core::os::hw::to_endianness(core::os::hw::endian::big, blocks.size() < 2 ? uint16_t(0) : core::from_string<uint16_t>(blocks[1], 16).or_else(0));
				auto c = core::os::hw::to_endianness(core::os::hw::endian::big, blocks.size() < 3 ? uint16_t(0) : core::from_string<uint16_t>(blocks[2], 16).or_else(0));
				auto d = core::os::hw::to_endianness(core::os::hw::endian::big, blocks.size() < 4 ? uint16_t(0) : core::from_string<uint16_t>(blocks[3], 16).or_else(0));
				auto e = core::os::hw::to_endianness(core::os::hw::endian::big, blocks.size() < 5 ? uint16_t(0) : core::from_string<uint16_t>(blocks[4], 16).or_else(0));
				auto f = core::os::hw::to_endianness(core::os::hw::endian::big, blocks.size() < 6 ? uint16_t(0) : core::from_string<uint16_t>(blocks[5], 16).or_else(0));
				auto g = core::os::hw::to_endianness(core::os::hw::endian::big, blocks.size() < 7 ? uint16_t(0) : core::from_string<uint16_t>(blocks[6], 16).or_else(0));
				auto h = core::os::hw::to_endianness(core::os::hw::endian::big, blocks.size() < 8 ? uint16_t(0) : core::from_string<uint16_t>(blocks[7], 16).or_else(0));

				uint64_t base_value_h = 0, base_value_l = 0;
				memcpy((char*)&base_value_h + sizeof(uint16_t) * 0, &a, sizeof(uint16_t));
				memcpy((char*)&base_value_h + sizeof(uint16_t) * 1, &b, sizeof(uint16_t));
				memcpy((char*)&base_value_h + sizeof(uint16_t) * 2, &c, sizeof(uint16_t));
				memcpy((char*)&base_value_h + sizeof(uint16_t) * 3, &d, sizeof(uint16_t));
				memcpy((char*)&base_value_l + sizeof(uint16_t) * 0, &e, sizeof(uint16_t));
				memcpy((char*)&base_value_l + sizeof(uint16_t) * 1, &f, sizeof(uint16_t));
				memcpy((char*)&base_value_l + sizeof(uint16_t) * 2, &g, sizeof(uint16_t));
				memcpy((char*)&base_value_l + sizeof(uint16_t) * 3, &h, sizeof(uint16_t));
				base_value_h = core::os::hw::to_endianness(core::os::hw::endian::big, base_value_h);
				base_value_l = core::os::hw::to_endianness(core::os::hw::endian::big, base_value_l);

				compute::uint128 base_value = compute::uint128(base_value_h, base_value_l);
				compute::uint128 range_value = ~(compute::uint128::max() << (128 - *range));
				compute::uint128 min_value = base_value & (compute::uint128::max() << (128 - *range));
				compute::uint128 max_value = min_value + range_value;
				socket_cidr result;
				result.min_value = min_value;
				result.max_value = max_value;
				result.value = base_value;
				result.mask = *range;
				return result;
			}
			else
			{
				auto blocks = core::stringify::split(address, '.');
				if (blocks.size() > 4)
					return core::optional::none;

				auto a = blocks.size() < 1 ? uint8_t(0) : core::from_string<uint8_t>(blocks[0]).or_else(0);
				auto b = blocks.size() < 2 ? uint8_t(0) : core::from_string<uint8_t>(blocks[1]).or_else(0);
				auto c = blocks.size() < 3 ? uint8_t(0) : core::from_string<uint8_t>(blocks[2]).or_else(0);
				auto d = blocks.size() < 4 ? uint8_t(0) : core::from_string<uint8_t>(blocks[3]).or_else(0);

				uint32_t base_value = 0;
				memcpy((char*)&base_value + sizeof(uint8_t) * 0, &a, sizeof(uint8_t));
				memcpy((char*)&base_value + sizeof(uint8_t) * 1, &b, sizeof(uint8_t));
				memcpy((char*)&base_value + sizeof(uint8_t) * 2, &c, sizeof(uint8_t));
				memcpy((char*)&base_value + sizeof(uint8_t) * 3, &d, sizeof(uint8_t));
				base_value = core::os::hw::to_endianness(core::os::hw::endian::big, base_value);

				uint32_t range_value = ~(std::numeric_limits<uint32_t>::max() << (32 - *range));
				uint32_t min_value = base_value & (std::numeric_limits<uint32_t>::max() << (32 - *range));
				uint32_t max_value = min_value + range_value;
				socket_cidr result;
				result.min_value = min_value;
				result.max_value = max_value;
				result.value = base_value;
				result.mask = *range;
				return result;
			}
		}
		bool utils::is_invalid(socket_t fd) noexcept
		{
			return fd == INVALID_SOCKET;
		}
		int64_t utils::clock() noexcept
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}
		void utils::display_transport_log() noexcept
		{
			compute::crypto::display_crypto_log();
		}

		transport_layer::transport_layer() noexcept : is_installed(false)
		{
		}
		transport_layer::~transport_layer() noexcept
		{
#ifdef VI_OPENSSL
			for (auto& context : servers)
				SSL_CTX_free(context);
			servers.clear();

			for (auto& context : clients)
				SSL_CTX_free(context);
			clients.clear();
#endif
		}
		core::expects_io<ssl_ctx_st*> transport_layer::create_server_context(size_t verify_depth, secure_layer_options options, const std::string_view& ciphers_list) noexcept
		{
#ifdef VI_OPENSSL
			VI_ASSERT(core::stringify::is_cstring(ciphers_list), "ciphers list should be set");
			core::umutex<std::mutex> unique(exclusive);
			SSL_CTX* context; bool load_certificates = false;
			if (servers.empty())
			{
				context = SSL_CTX_new(TLS_server_method());
				if (!context)
				{
					utils::display_transport_log();
					return std::make_error_condition(std::errc::protocol_not_supported);
				}
				VI_DEBUG("[net] OK create client 0x%" PRIuPTR " TLS context", context);
				load_certificates = verify_depth > 0;
			}
			else
			{
				context = *servers.begin();
				servers.erase(servers.begin());
				VI_DEBUG("[net] pop client 0x%" PRIuPTR " TLS context from cache", context);
				load_certificates = verify_depth > 0 && SSL_CTX_get_verify_depth(context) <= 0;
			}
			unique.negate();

			auto status = initialize_context(context, load_certificates);
			if (!status)
			{
				SSL_CTX_free(context);
				return status.error();
			}

			long flags = SSL_OP_ALL;
			if ((size_t)options & (size_t)secure_layer_options::no_sslv2)
				flags |= SSL_OP_NO_SSLv2;
			if ((size_t)options & (size_t)secure_layer_options::no_sslv3)
				flags |= SSL_OP_NO_SSLv3;
			if ((size_t)options & (size_t)secure_layer_options::no_tlsv1)
				flags |= SSL_OP_NO_TLSv1;
			if ((size_t)options & (size_t)secure_layer_options::no_tlsv11)
				flags |= SSL_OP_NO_TLSv1_1;
#ifdef SSL_OP_NO_TLSv1_2
			if ((size_t)options & (size_t)secure_layer_options::no_tlsv12)
				flags |= SSL_OP_NO_TLSv1_2;
#endif
#ifdef SSL_OP_NO_TLSv1_2
			if ((size_t)options & (size_t)secure_layer_options::no_tlsv13)
				flags |= SSL_OP_NO_TLSv1_3;
#endif
			SSL_CTX_set_options(context, flags);
			SSL_CTX_set_verify_depth(context, (int)verify_depth);
			if (!ciphers_list.empty() && SSL_CTX_set_cipher_list(context, ciphers_list.data()) != 1)
			{
				SSL_CTX_free(context);
				utils::display_transport_log();
				return std::make_error_condition(std::errc::protocol_not_supported);
			}

			VI_DEBUG("[net] OK create server 0x%" PRIuPTR " TLS context", context);
			return context;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<ssl_ctx_st*> transport_layer::create_client_context(size_t verify_depth) noexcept
		{
#ifdef VI_OPENSSL
			core::umutex<std::mutex> unique(exclusive);
			SSL_CTX* context; bool load_certificates = false;
			if (clients.empty())
			{
				context = SSL_CTX_new(TLS_client_method());
				if (!context)
				{
					utils::display_transport_log();
					return std::make_error_condition(std::errc::protocol_not_supported);
				}
				VI_DEBUG("[net] OK create client 0x%" PRIuPTR " TLS context", context);
				load_certificates = verify_depth > 0;
			}
			else
			{
				context = *clients.begin();
				clients.erase(clients.begin());
				VI_DEBUG("[net] pop client 0x%" PRIuPTR " TLS context from cache", context);
				load_certificates = verify_depth > 0 && SSL_CTX_get_verify_depth(context) <= 0;
			}
			unique.negate();

			auto status = initialize_context(context, load_certificates);
			if (!status)
			{
				SSL_CTX_free(context);
				return status.error();
			}

			SSL_CTX_set_verify_depth(context, (int)verify_depth);
			return context;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		void transport_layer::free_server_context(ssl_ctx_st* context) noexcept
		{
			if (!context)
				return;

			core::umutex<std::mutex> unique(exclusive);
			servers.insert(context);
		}
		void transport_layer::free_client_context(ssl_ctx_st* context) noexcept
		{
			if (!context)
				return;

			core::umutex<std::mutex> unique(exclusive);
			clients.insert(context);
		}
		core::expects_io<void> transport_layer::initialize_context(ssl_ctx_st* context, bool load_certificates) noexcept
		{
#ifdef VI_OPENSSL
			SSL_CTX_set_options(context, SSL_OP_SINGLE_DH_USE);
			SSL_CTX_set_options(context, SSL_OP_CIPHER_SERVER_PREFERENCE);
			SSL_CTX_set_mode(context, SSL_MODE_ENABLE_PARTIAL_WRITE);
			SSL_CTX_set_mode(context, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
#ifdef SSL_CTX_set_ecdh_auto
			SSL_CTX_set_ecdh_auto(context, 1);
#endif
			auto session_id = compute::crypto::random_bytes(12);
			if (session_id)
			{
				auto context_id = compute::codec::hex_encode(*session_id);
				SSL_CTX_set_session_id_context(context, (const uint8_t*)context_id.c_str(), (uint32_t)context_id.size());
			}

			if (load_certificates)
			{
#ifdef VI_MICROSOFT
				HCERTSTORE store = CertOpenSystemStore(0, "ROOT");
				if (!store)
				{
					utils::display_transport_log();
					return std::make_error_condition(std::errc::permission_denied);
				}

				X509_STORE* storage = SSL_CTX_get_cert_store(context);
				if (!storage)
				{
					CertCloseStore(store, 0);
					utils::display_transport_log();
					return std::make_error_condition(std::errc::bad_file_descriptor);
				}

				PCCERT_CONTEXT next = nullptr; uint32_t count = 0;
				while ((next = CertEnumCertificatesInStore(store, next)) != nullptr)
				{
					X509* certificate = d2i_X509(nullptr, (const uint8_t**)&next->pbCertEncoded, next->cbCertEncoded);
					if (!certificate)
						continue;

					if (X509_STORE_add_cert(storage, certificate) && !is_installed)
						VI_TRACE("[net] OK load root level certificate: %s", compute::codec::hex_encode((const char*)next->pCertInfo->serial_number.pbData, (size_t)next->pCertInfo->serial_number.cbData).c_str());
					X509_free(certificate);
					++count;
				}

				(void)count;
				VI_DEBUG("[net] OK load %i root level certificates from ROOT registry", count);
				CertCloseStore(store, 0);
				utils::display_transport_log();
#else
				if (!SSL_CTX_set_default_verify_paths(context))
				{
					utils::display_transport_log();
					return std::make_error_condition(std::errc::no_such_file_or_directory);
				}
#endif
			}

			is_installed = true;
			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}

		dns::dns() noexcept
		{
			VI_TRACE("[dns] OK initialize cache");
		}
		dns::~dns() noexcept
		{
			VI_TRACE("[dns] cleanup cache");
			names.clear();
		}
		core::expects_system<core::string> dns::reverse_lookup(const std::string_view& hostname, const std::string_view& service)
		{
			VI_ASSERT(hostname.empty() || core::stringify::is_cstring(hostname), "host should be set");
			VI_ASSERT(service.empty() || core::stringify::is_cstring(service), "service should be set");
			VI_MEASURE((uint64_t)core::timings::networking * 3);

			struct addrinfo hints;
			memset(&hints, 0, sizeof(hints));
			hints.ai_family = AF_UNSPEC;

			if (has_ip_v4_address(hostname) || has_ip_v6_address(hostname))
				hints.ai_flags |= AI_NUMERICHOST;
			if (core::stringify::has_integer(service))
				hints.ai_flags |= AI_NUMERICSERV;

			struct addrinfo* address = nullptr;
			if (getaddrinfo(hostname.empty() ? nullptr : hostname.data(), service.empty() ? nullptr : service.data(), &hints, &address) != 0)
				return core::system_exception(core::stringify::text("dns resolve %s:%s address: invalid address", hostname.data(), service.data()));

			socket_address target = socket_address(hostname, core::from_string<uint16_t>(service).or_else(0), address);
			freeaddrinfo(address);
			return reverse_address_lookup(target);
		}
		core::expects_promise_system<core::string> dns::reverse_lookup_deferred(const std::string_view& source_hostname, const std::string_view& source_service)
		{
			core::string hostname = core::string(source_hostname), service = core::string(source_service);
			return core::cotask<core::expects_system<core::string>>([this, hostname = std::move(hostname), service = std::move(service)]() mutable
			{
				return reverse_lookup(hostname, service);
			});
		}
		core::expects_system<core::string> dns::reverse_address_lookup(const socket_address& address)
		{
			VI_MEASURE((uint64_t)core::timings::networking * 3);
			char reverse_hostname[NI_MAXHOST], reverse_service[NI_MAXSERV];
			if (getnameinfo(address.get_raw_address(), (socklen_t)address.get_address_size(), reverse_hostname, NI_MAXHOST, reverse_service, NI_MAXSERV, NI_NUMERICSERV) != 0)
				return core::system_exception(core::stringify::text("dns reverse resolve %s address: invalid address", get_address_identification(address).c_str()));

			VI_DEBUG("[net] dns reverse resolved for entity %s (host %s:%s is used)", get_address_identification(address).c_str(), reverse_hostname, reverse_service);
			return core::string(reverse_hostname, strnlen(reverse_hostname, sizeof(reverse_hostname)));
		}
		core::expects_promise_system<core::string> dns::reverse_address_lookup_deferred(const socket_address& address)
		{
			return core::cotask<core::expects_system<core::string>>([this, address]() mutable
			{
				return reverse_address_lookup(address);
			});
		}
		core::expects_system<socket_address> dns::lookup(const std::string_view& hostname, const std::string_view& service, dns_type resolver, socket_protocol proto, socket_type type)
		{
			VI_ASSERT(!hostname.empty() && core::stringify::is_cstring(hostname), "host should be set");
			VI_MEASURE((uint64_t)core::timings::networking * 3);
			int64_t time = ::time(nullptr);
			struct addrinfo hints;
			memset(&hints, 0, sizeof(struct addrinfo));
			hints.ai_family = AF_UNSPEC;
			switch (proto)
			{
				case socket_protocol::IP:
					hints.ai_protocol = IPPROTO_IP;
					break;
				case socket_protocol::ICMP:
					hints.ai_protocol = IPPROTO_ICMP;
					break;
				case socket_protocol::UDP:
					hints.ai_protocol = IPPROTO_UDP;
					break;
				case socket_protocol::RAW:
					hints.ai_protocol = IPPROTO_RAW;
					break;
				case socket_protocol::TCP:
				default:
					hints.ai_protocol = IPPROTO_TCP;
					break;
			}
			switch (type)
			{
				case socket_type::datagram:
					hints.ai_socktype = SOCK_DGRAM;
					break;
				case socket_type::raw:
					hints.ai_socktype = SOCK_RAW;
					break;
				case socket_type::reliably_delivered_message:
					hints.ai_socktype = SOCK_RDM;
					break;
				case socket_type::sequence_packet_stream:
					hints.ai_socktype = SOCK_SEQPACKET;
					break;
				case socket_type::stream:
				default:
					hints.ai_socktype = SOCK_STREAM;
					break;
			}
			switch (resolver)
			{
				case dns_type::connect:
					hints.ai_flags = AI_CANONNAME;
					break;
				case dns_type::listen:
					hints.ai_flags = AI_CANONNAME | AI_PASSIVE;
					break;
				default:
					break;
			}
			if (has_ip_v4_address(hostname) || has_ip_v6_address(hostname))
				hints.ai_flags |= AI_NUMERICHOST;
			if (core::stringify::has_integer(service))
				hints.ai_flags |= AI_NUMERICSERV;

			char buffer[core::CHUNK_SIZE];
			size_t header_size = sizeof(uint16_t) * 7;
			size_t max_service_size = sizeof(buffer) - header_size;
			size_t service_size = std::min<size_t>(service.size(), max_service_size);
			size_t hostname_size = std::min<size_t>(hostname.size(), max_service_size - service_size);
			memcpy(buffer + sizeof(uint32_t) * 0, &resolver, sizeof(uint32_t));
			memcpy(buffer + sizeof(uint32_t) * 1, &proto, sizeof(uint32_t));
			memcpy(buffer + sizeof(uint32_t) * 2, &type, sizeof(uint32_t));
			memcpy(buffer + sizeof(uint32_t) * 3, service.data(), service_size);
			memcpy(buffer + sizeof(uint32_t) * 3 + service_size, hostname.data(), hostname_size);

			core::key_hasher<core::string> hasher;
			size_t hash = hasher(std::string_view(buffer, header_size + service_size + hostname_size));
			{
				core::umutex<std::mutex> unique(exclusive);
				auto it = names.find(hash);
				if (it != names.end() && it->second.first > time)
					return it->second.second;
			}

			struct addrinfo* addresses = nullptr;
			if (getaddrinfo(hostname.empty() ? nullptr : hostname.data(), service.empty() ? nullptr : service.data(), &hints, &addresses) != 0)
				return core::system_exception(core::stringify::text("dns resolve %s:%s address: invalid address", hostname.data(), service.data()));

			struct addrinfo* target_address = nullptr;
			core::unordered_map<socket_t, addrinfo*> hosts;
			for (auto it = addresses; it != nullptr; it = it->ai_next)
			{
				auto connectable = execute_socket(it->ai_family, it->ai_socktype, it->ai_protocol);
				if (!connectable && connectable.error() == std::errc::permission_denied)
				{
					for (auto& host : hosts)
					{
						closesocket(host.first);
						VI_DEBUG("[net] close dns fd %i", (int)host.first);
					}
					return core::system_exception(core::stringify::text("dns resolve %s:%s address", hostname.data(), service.data()), std::move(connectable.error()));
				}
				else if (!connectable)
					continue;

				socket_t connection = *connectable;
				VI_DEBUG("[net] open dns fd %i", (int)connection);
				if (resolver == dns_type::connect)
				{
					hosts[connection] = it;
					continue;
				}

				target_address = it;
				closesocket(connection);
				VI_DEBUG("[net] close dns fd %i", (int)connection);
				break;
			}

			if (resolver == dns_type::connect)
				target_address = try_connect_dns(hosts, CONNECT_TIMEOUT);

			for (auto& host : hosts)
			{
				closesocket(host.first);
				VI_DEBUG("[net] close dns fd %i", (int)host.first);
			}

			if (!target_address)
			{
				freeaddrinfo(addresses);
				return core::system_exception(core::stringify::text("dns resolve %s:%s address: invalid address", hostname.data(), service.data()), std::make_error_condition(std::errc::host_unreachable));
			}

			socket_address result = socket_address(hostname, core::from_string<uint16_t>(service).or_else(0), target_address);
			VI_DEBUG("[net] dns resolved for entity %s:%s (address %s is used)", hostname.data(), service.data(), get_ip_address_identification(result).c_str());
			freeaddrinfo(addresses);

			core::umutex<std::mutex> unique(exclusive);
			auto it = names.find(hash);
			if (it != names.end())
			{
				it->second.first = time + DNS_TIMEOUT;
				result = it->second.second;
			}
			else
				names[hash] = std::make_pair(time + DNS_TIMEOUT, result);

			return result;
		}
		core::expects_promise_system<socket_address> dns::lookup_deferred(const std::string_view& source_hostname, const std::string_view& source_service, dns_type resolver, socket_protocol proto, socket_type type)
		{
			core::string hostname = core::string(source_hostname), service = core::string(source_service);
			return core::cotask<core::expects_system<socket_address>>([this, hostname = std::move(hostname), service = std::move(service), resolver, proto, type]() mutable
			{
				return lookup(hostname, service, resolver, proto, type);
			});
		}

		multiplexer::multiplexer() noexcept : multiplexer(100, 256)
		{
		}
		multiplexer::multiplexer(uint64_t dispatch_timeout, size_t max_events) noexcept : handle(max_events), activations(0), default_timeout(dispatch_timeout)
		{
			VI_TRACE("[net] OK initialize multiplexer (%" PRIu64 " events)", (uint64_t)max_events);
			fds.resize(max_events);
		}
		multiplexer::~multiplexer() noexcept
		{
			shutdown();
			VI_TRACE("[net] free multiplexer");
		}
		void multiplexer::rescale(uint64_t dispatch_timeout, size_t max_events) noexcept
		{
			default_timeout = dispatch_timeout;
			handle = epoll_interface(max_events);
		}
		void multiplexer::activate() noexcept
		{
			try_listen();
		}
		void multiplexer::deactivate() noexcept
		{
			try_unlisten();
		}
		void multiplexer::shutdown() noexcept
		{
			VI_MEASURE(core::timings::file_system);
			dispatch_timers(core::schedule::get_clock());

			core::ordered_map<std::chrono::microseconds, socket*> dirty_timers;
			core::unordered_set<socket*> dirty_trackers;
			core::umutex<std::mutex> unique(exclusive);
			VI_DEBUG("[net] shutdown multiplexer on fds (sockets = %i)", (int)(timers.size() + trackers.size()));
			dirty_timers.swap(timers);
			dirty_trackers.swap(trackers);

			for (auto& item : dirty_trackers)
			{
				VI_DEBUG("[net] sock reset on fd %i", (int)item->fd);
				item->events.expiration = std::chrono::microseconds(0);
				cancel_events(item, socket_poll::reset);
			}

			for (auto& item : dirty_timers)
			{
				VI_DEBUG("[net] sock timeout on fd %i", (int)item.second->fd);
				item.second->events.expiration = std::chrono::microseconds(0);
				cancel_events(item.second, socket_poll::timeout);
			}
		}
		int multiplexer::dispatch(uint64_t event_timeout) noexcept
		{
			int count = handle.wait(fds.data(), fds.size(), event_timeout);
			auto time = core::schedule::get_clock();
			if (count > 0)
			{
				VI_MEASURE(core::timings::file_system);
				size_t size = (size_t)count;
				for (size_t i = 0; i < size; i++)
					dispatch_events(fds[i], time);
			}

			dispatch_timers(time);
			return count;
		}
		void multiplexer::dispatch_timers(const std::chrono::microseconds& time) noexcept
		{
			VI_MEASURE(core::timings::file_system);
			if (timers.empty())
				return;

			core::umutex<std::mutex> unique(exclusive);
			while (!timers.empty())
			{
				auto it = timers.begin();
				if (it->first > time)
					break;

				VI_DEBUG("[net] sock timeout on fd %i", (int)it->second->fd);
				it->second->events.expiration = std::chrono::microseconds(0);
				cancel_events(it->second, socket_poll::timeout);
				timers.erase(it);
			}
		}
		bool multiplexer::dispatch_events(const epoll_fd& fd, const std::chrono::microseconds& time) noexcept
		{
			VI_ASSERT(fd.base != nullptr, "no socket is connected to epoll fd");
			VI_TRACE("[net] sock event:%s%s%s on fd %i", fd.closeable ? "c" : "", fd.readable ? "r" : "", fd.writeable ? "w" : "", (int)fd.base->fd);
			if (fd.closeable)
			{
				VI_DEBUG("[net] sock reset on fd %i", (int)fd.base->fd);
				cancel_events(fd.base, socket_poll::reset);
				return false;
			}
			else if (!fd.readable && !fd.writeable)
				return true;

			core::umutex<std::mutex> unique(fd.base->events.mutex);
			bool was_listening_read = !!fd.base->events.read_callback;
			bool was_listening_write = !!fd.base->events.write_callback;
			bool still_listening_read = !fd.readable && was_listening_read;
			bool still_listening_write = !fd.writeable && was_listening_write;
			if (still_listening_read || still_listening_write)
			{
				if (was_listening_read != still_listening_read || was_listening_write != still_listening_write)
					handle.update(fd.base, still_listening_read, still_listening_write);
				update_timeout(fd.base, time);
			}
			else if (was_listening_read || was_listening_write)
			{
				handle.remove(fd.base);
				remove_timeout(fd.base);
			}

			if (fd.readable && fd.writeable)
			{
				poll_event_callback read_callback, write_callback;
				fd.base->events.read_callback.swap(read_callback);
				fd.base->events.write_callback.swap(write_callback);
				unique.negate();
				core::cospawn([read_callback = std::move(read_callback), write_callback = std::move(write_callback)]() mutable
				{
					if (write_callback)
						write_callback(socket_poll::finish);
					if (read_callback)
						read_callback(socket_poll::finish);
				});
			}
			else if (fd.readable && was_listening_read)
			{
				poll_event_callback read_callback;
				fd.base->events.read_callback.swap(read_callback);
				unique.negate();
				core::cospawn([read_callback = std::move(read_callback)]() mutable { read_callback(socket_poll::finish); });
			}
			else if (fd.writeable && was_listening_write)
			{
				poll_event_callback write_callback;
				fd.base->events.write_callback.swap(write_callback);
				unique.negate();
				core::cospawn([write_callback = std::move(write_callback)]() mutable { write_callback(socket_poll::finish); });
			}

			return still_listening_read || still_listening_write;
		}
		bool multiplexer::when_readable(socket* value, poll_event_callback&& when_ready) noexcept
		{
			VI_ASSERT(value != nullptr && value->fd != INVALID_SOCKET, "socket should be set and valid");
			VI_ASSERT(when_ready != nullptr, "readable callback should be set");
			core::umutex<std::mutex> unique(value->events.mutex);
			bool was_listening_read = !!value->events.read_callback;
			bool still_listening_write = !!value->events.write_callback;
			value->events.read_callback.swap(when_ready);
			bool listening = (when_ready ? handle.update(value, true, still_listening_write) : handle.add(value, true, still_listening_write));
			if (!was_listening_read && !still_listening_write)
				add_timeout(value, core::schedule::get_clock());

			unique.negate();
			if (when_ready)
				core::cospawn([when_ready = std::move(when_ready)]() mutable { when_ready(socket_poll::cancel); });

			return listening;
		}
		bool multiplexer::when_writeable(socket* value, poll_event_callback&& when_ready) noexcept
		{
			VI_ASSERT(value != nullptr && value->fd != INVALID_SOCKET, "socket should be set and valid");
			core::umutex<std::mutex> unique(value->events.mutex);
			bool still_listening_read = !!value->events.read_callback;
			bool was_listening_write = !!value->events.write_callback;
			value->events.write_callback.swap(when_ready);
			bool listening = (when_ready ? handle.update(value, still_listening_read, true) : handle.add(value, still_listening_read, true));
			if (!was_listening_write && !still_listening_read)
				add_timeout(value, core::schedule::get_clock());

			unique.negate();
			if (when_ready)
				core::cospawn([when_ready = std::move(when_ready)]() mutable { when_ready(socket_poll::cancel); });

			return listening;
		}
		bool multiplexer::cancel_events(socket* value, socket_poll event) noexcept
		{
			VI_ASSERT(value != nullptr, "socket should be set and valid");
			core::umutex<std::mutex> unique(value->events.mutex);
			poll_event_callback read_callback, write_callback;
			value->events.read_callback.swap(read_callback);
			value->events.write_callback.swap(write_callback);
			bool was_listening = read_callback || write_callback;
			bool not_listening = was_listening && value->is_valid() ? handle.remove(value) : true;
			if (was_listening)
				remove_timeout(value);

			unique.negate();
			if (packet::is_done(event) || !was_listening)
				return not_listening;

			if (read_callback && write_callback)
			{
				core::cospawn([event, read_callback = std::move(read_callback), write_callback = std::move(write_callback)]() mutable
				{
					if (write_callback)
						write_callback(event);
					if (read_callback)
						read_callback(event);
				});
			}
			else if (read_callback)
				core::cospawn([event, read_callback = std::move(read_callback)]() mutable { read_callback(event); });
			else if (write_callback)
				core::cospawn([event, write_callback = std::move(write_callback)]() mutable { write_callback(event); });
			return not_listening;
		}
		bool multiplexer::clear_events(socket* value) noexcept
		{
			return cancel_events(value, socket_poll::finish);
		}
		bool multiplexer::is_listening() noexcept
		{
			return activations > 0;
		}
		void multiplexer::add_timeout(socket* value, const std::chrono::microseconds& time) noexcept
		{
			if (value->events.timeout > 0)
			{
				VI_TRACE("[net] sock set timeout on fd %i (time = %i)", (int)value->fd, (int)value->events.timeout);
				auto expiration = time + std::chrono::milliseconds(value->events.timeout);
				core::umutex<std::mutex> unique(exclusive);
				while (timers.find(expiration) != timers.end())
					++expiration;

				timers[expiration] = value;
				value->events.expiration = expiration;
			}
			else
			{
				core::umutex<std::mutex> unique(exclusive);
				value->events.expiration = std::chrono::microseconds(-1);
				trackers.insert(value);
			}
		}
		void multiplexer::update_timeout(socket* value, const std::chrono::microseconds& time) noexcept
		{
			remove_timeout(value);
			add_timeout(value, time);
		}
		void multiplexer::remove_timeout(socket* value) noexcept
		{
			VI_TRACE("[net] sock cancel timeout on fd %i", (int)value->fd);
			if (value->events.expiration > std::chrono::microseconds(0))
			{
				core::umutex<std::mutex> unique(exclusive);
				auto it = timers.find(value->events.expiration);
				VI_ASSERT(it != timers.end(), "socket timeout update de-sync happend");
				value->events.expiration = std::chrono::microseconds(0);
				if (it != timers.end())
					timers.erase(it);
			}
			else if (value->events.expiration < std::chrono::microseconds(0))
			{
				core::umutex<std::mutex> unique(exclusive);
				value->events.expiration = std::chrono::microseconds(0);
				trackers.erase(value);
			}
		}
		void multiplexer::try_dispatch() noexcept
		{
			auto* queue = core::schedule::get();
			dispatch(queue->has_parallel_threads(core::difficulty::sync) ? default_timeout : 5);
			try_enqueue();
		}
		void multiplexer::try_enqueue() noexcept
		{
			if (!activations)
				return;

			auto* queue = core::schedule::get();
			queue->set_task(std::bind(&multiplexer::try_dispatch, this));
		}
		void multiplexer::try_listen() noexcept
		{
			if (!activations++)
			{
				VI_DEBUG("[net] start events polling");
				try_enqueue();
			}
		}
		void multiplexer::try_unlisten() noexcept
		{
			VI_ASSERT(activations > 0, "events poller is already inactive");
			if (!--activations)
				VI_DEBUG("[net] stop events polling");
		}
		size_t multiplexer::get_activations() noexcept
		{
			return activations;
		}

		uplinks::uplinks() noexcept : max_duplicates(1)
		{
			multiplexer::get()->activate();
		}
		uplinks::~uplinks() noexcept
		{
			auto* dispatcher = multiplexer::get();
			core::single_queue<acquire_callback> queue;
			core::umutex<std::recursive_mutex> unique(exclusive);
			for (auto& item : connections)
			{
				for (auto& stream : item.second.streams)
				{
					dispatcher->cancel_events(stream);
					core::memory::release(stream);
				}

				while (!item.second.requests.empty())
				{
					queue.push(std::move(item.second.requests.front()));
					item.second.requests.pop();
				}
			}

			max_duplicates = 0;
			connections.clear();
			dispatcher->deactivate();
			unique.negate();

			while (!queue.empty())
			{
				queue.front()(nullptr);
				queue.pop();
			}
		}
		void uplinks::set_max_duplicates(size_t max)
		{
			max_duplicates = max + 1;
		}
		void uplinks::listen_connection(core::string&& id, socket* stream)
		{
			stream->add_ref();
			multiplexer::get()->when_readable(stream, [this, id = std::move(id), stream](socket_poll event) mutable
			{
				if (packet::is_error(event))
				{
				expire:
					core::umutex<std::recursive_mutex> unique(exclusive);
					auto it = connections.find(id);
					if (it != connections.end())
					{
						auto queue = std::move(it->second.requests);
						core::uptr<socket> target_stream = stream;
						multiplexer::get()->cancel_events(stream);
						it->second.streams.erase(stream);
						if (it->second.streams.empty() && it->second.requests.empty())
							connections.erase(it);
						unique.negate();

						VI_DEBUG("[uplink] expire fd %i of %s", (int)stream->get_fd(), id.c_str());
						while (!queue.empty())
						{
							queue.front()(nullptr);
							queue.pop();
						}
					}
				}
				else if (!packet::is_skip(event))
				{
				retry:
					uint8_t buffer;
					auto status = stream->read(&buffer, sizeof(buffer));
					if (status)
						goto retry;
					else if (status.error() != std::errc::operation_would_block)
						goto expire;
					else
						listen_connection(std::move(id), stream);
				}
				stream->release();
			});
		}
		bool uplinks::push_connection(const socket_address& address, socket* stream)
		{
			if (!max_duplicates)
				return false;

			auto name = get_address_identification(address);
			core::umutex<std::recursive_mutex> unique(exclusive);
			auto it = connections.find(name);
			if (it == connections.end())
			{
				if (!stream)
					return false;

				VI_DEBUG("[uplink] store fd %i of %s", (int)stream->get_fd(), name.c_str());
				auto& pool = connections[name];
				pool.streams.insert(stream);
				stream->set_io_timeout(0);
				stream->set_blocking(false);
				listen_connection(std::move(name), stream);
				return true;
			}
			else if (!it->second.requests.empty())
			{
				auto callback = std::move(it->second.requests.front());
				it->second.requests.pop();
				unique.negate();
				callback(stream);
				if (!stream)
					return false;

				VI_DEBUG("[uplink] reuse fd %i of %s", (int)stream->get_fd(), name.c_str());
				return true;
			}
			else if (!stream)
			{
				if (it->second.streams.empty())
					connections.erase(it);
				return false;
			}
			else if (it->second.streams.size() >= max_duplicates)
				return false;

			VI_DEBUG("[uplink] store fd %i of %s", (int)stream->get_fd(), name.c_str());
			it->second.streams.insert(stream);
			stream->set_io_timeout(0);
			stream->set_blocking(false);
			listen_connection(std::move(name), stream);
			return true;
		}
		bool uplinks::pop_connection_queued(const socket_address& address, acquire_callback&& callback)
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			if (!max_duplicates)
			{
				callback(nullptr);
				return false;
			}

			auto name = get_address_identification(address);
			core::umutex<std::recursive_mutex> unique(exclusive);
			auto it = connections.find(name);
			if (it == connections.end())
			{
				auto& item = connections[name];
				item.duplicates = max_duplicates - 1;
				callback(nullptr);
				return false;
			}
			else if (it->second.streams.empty())
			{
				if (it->second.duplicates > 0)
				{
					--it->second.duplicates;
					callback(nullptr);
					return false;
				}

				it->second.requests.emplace(std::move(callback));
				return true;
			}

			socket* stream = *it->second.streams.begin();
			it->second.streams.erase(it->second.streams.begin());
			unique.unlock();

			VI_DEBUG("[uplink] reuse fd %i of %s", (int)stream->get_fd(), name.c_str());
			multiplexer::get()->cancel_events(stream);
			callback(stream);

			return true;
		}
		core::promise<socket*> uplinks::pop_connection(const socket_address& address)
		{
			core::promise<socket*> future;
			pop_connection_queued(address, [future](socket* target) mutable { future.set(target); });
			return future;
		}
		size_t uplinks::get_max_duplicates() const
		{
			return max_duplicates;
		}
		size_t uplinks::get_size()
		{
			core::umutex<std::recursive_mutex> unique(exclusive);
			return connections.size();
		}

		certificate_builder::certificate_builder() noexcept
		{
#ifdef VI_OPENSSL
			certificate = X509_new();
			private_key = EVP_PKEY_new();
#endif
		}
		certificate_builder::~certificate_builder() noexcept
		{
#ifdef VI_OPENSSL
			if (certificate != nullptr)
				X509_free((X509*)certificate);
			if (private_key != nullptr)
				EVP_PKEY_free((EVP_PKEY*)private_key);
#endif
		}
		core::expects_io<void> certificate_builder::set_serial_number(uint32_t bits)
		{
#ifdef VI_OPENSSL
			VI_ASSERT(bits > 0, "bits should be greater than zero");
			ASN1_STRING* serial_number = X509_get_serialNumber((X509*)certificate);
			if (!serial_number)
				return std::make_error_condition(std::errc::bad_message);

			auto data = compute::crypto::random_bytes(bits / 8);
			if (!data)
				return std::make_error_condition(std::errc::bad_message);

			if (!ASN1_STRING_set(serial_number, data->data(), static_cast<int>(data->size())))
				return std::make_error_condition(std::errc::invalid_argument);

			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> certificate_builder::set_version(uint8_t version)
		{
#ifdef VI_OPENSSL
			if (!X509_set_version((X509*)certificate, version))
				return std::make_error_condition(std::errc::invalid_argument);

			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> certificate_builder::set_not_after(int64_t offset_seconds)
		{
#ifdef VI_OPENSSL
			if (!X509_gmtime_adj(X509_get_notAfter((X509*)certificate), (long)offset_seconds))
				return std::make_error_condition(std::errc::invalid_argument);

			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> certificate_builder::set_not_before(int64_t offset_seconds)
		{
#ifdef VI_OPENSSL
			if (!X509_gmtime_adj(X509_get_notBefore((X509*)certificate), (long)offset_seconds))
				return std::make_error_condition(std::errc::invalid_argument);

			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> certificate_builder::set_public_key(uint32_t bits)
		{
#ifdef VI_OPENSSL
			VI_ASSERT(bits > 0, "bits should be greater than zero");
			EVP_PKEY* new_private_key = EVP_RSA_gen(bits);
			if (!new_private_key)
				return std::make_error_condition(std::errc::function_not_supported);

			EVP_PKEY_free((EVP_PKEY*)private_key);
			private_key = new_private_key;

			if (!X509_set_pubkey((X509*)certificate, (EVP_PKEY*)private_key))
				return std::make_error_condition(std::errc::bad_message);

			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> certificate_builder::set_issuer(const x509_blob& issuer)
		{
#ifdef VI_OPENSSL
			if (!issuer.pointer)
				return std::make_error_condition(std::errc::invalid_argument);

			X509_NAME* subject_name = X509_get_subject_name((X509*)issuer.pointer);
			if (!subject_name)
				return std::make_error_condition(std::errc::invalid_argument);

			if (!X509_set_issuer_name((X509*)certificate, subject_name))
				return std::make_error_condition(std::errc::invalid_argument);

			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> certificate_builder::add_custom_extension(bool critical, const std::string_view& key, const std::string_view& value, const std::string_view& description)
		{
			VI_ASSERT(!key.empty() && core::stringify::is_cstring(key), "key should be set");
			VI_ASSERT(description.empty() || core::stringify::is_cstring(description), "description should be set");
#ifdef VI_OPENSSL
			const int NID = OBJ_create(key.data(), value.data(), description.empty() ? nullptr : description.data());
			ASN1_OCTET_STRING* data = ASN1_OCTET_STRING_new();
			if (!data)
				return std::make_error_condition(std::errc::not_enough_memory);

			if (!ASN1_OCTET_STRING_set(data, reinterpret_cast<unsigned const char*>(value.data()), (int)value.size()))
			{
				ASN1_OCTET_STRING_free(data);
				return std::make_error_condition(std::errc::invalid_argument);
			}

			X509_EXTENSION* extension = X509_EXTENSION_create_by_NID(nullptr, NID, critical, data);
			if (!extension)
			{
				ASN1_OCTET_STRING_free(data);
				return std::make_error_condition(std::errc::invalid_argument);
			}

			bool success = X509_add_ext((X509*)certificate, extension, -1) == 1;
			X509_EXTENSION_free(extension);
			ASN1_OCTET_STRING_free(data);

			if (!success)
				return std::make_error_condition(std::errc::invalid_argument);

			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> certificate_builder::add_standard_extension(const x509_blob& issuer, int NID, const std::string_view& value)
		{
			VI_ASSERT(core::stringify::is_cstring(value), "value should be set");
#ifdef VI_OPENSSL
			X509V3_CTX context;
			X509V3_set_ctx_nodb(&context);
			X509V3_set_ctx(&context, (X509*)(issuer.pointer ? issuer.pointer : certificate), (X509*)certificate, nullptr, nullptr, 0);

			X509_EXTENSION* extension = X509V3_EXT_conf_nid(nullptr, &context, NID, value.data());
			if (!extension)
				return std::make_error_condition(std::errc::invalid_argument);

			bool success = X509_add_ext((X509*)certificate, extension, -1) == 1;
			X509_EXTENSION_free(extension);
			if (!success)
				return std::make_error_condition(std::errc::invalid_argument);

			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> certificate_builder::add_standard_extension(const x509_blob& issuer, const std::string_view& name_of_nid, const std::string_view& value)
		{
			VI_ASSERT(!name_of_nid.empty() && core::stringify::is_cstring(name_of_nid), "name of nid should be set");
#ifdef VI_OPENSSL
			int NID = OBJ_txt2nid(name_of_nid.data());
			if (NID == NID_undef)
				return std::make_error_condition(std::errc::invalid_argument);

			return add_standard_extension(issuer, NID, value);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> certificate_builder::add_subject_info(const std::string_view& key, const std::string_view& value)
		{
			VI_ASSERT(!key.empty() && core::stringify::is_cstring(key), "key should be set");
#ifdef VI_OPENSSL
			X509_NAME* subject_name = X509_get_subject_name((X509*)certificate);
			if (!subject_name)
				return std::make_error_condition(std::errc::bad_message);

			if (!X509_NAME_add_entry_by_txt(subject_name, key.data(), MBSTRING_ASC, (uint8_t*)value.data(), (int)value.size(), -1, 0))
				return std::make_error_condition(std::errc::invalid_argument);

			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> certificate_builder::add_issuer_info(const std::string_view& key, const std::string_view& value)
		{
			VI_ASSERT(!key.empty() && core::stringify::is_cstring(key), "key should be set");
#ifdef VI_OPENSSL
			X509_NAME* issuer_name = X509_get_issuer_name((X509*)certificate);
			if (!issuer_name)
				return std::make_error_condition(std::errc::invalid_argument);

			if (!X509_NAME_add_entry_by_txt(issuer_name, key.data(), MBSTRING_ASC, (uint8_t*)value.data(), (int)value.size(), -1, 0))
				return std::make_error_condition(std::errc::invalid_argument);

			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> certificate_builder::sign(compute::digest algorithm)
		{
#ifdef VI_OPENSSL
			VI_ASSERT(algorithm != nullptr, "algorithm should be set");
			if (X509_sign((X509*)certificate, (EVP_PKEY*)private_key, (EVP_MD*)algorithm) != 0)
				return std::make_error_condition(std::errc::invalid_argument);

			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<certificate_blob> certificate_builder::build()
		{
#ifdef VI_OPENSSL
			certificate_blob blob;
			BIO* bio = BIO_new(BIO_s_mem());
			PEM_write_bio_X509(bio, (X509*)certificate);

			BUF_MEM* bio_memory = nullptr;
			if (BIO_get_mem_ptr(bio, &bio_memory) != 0 && bio_memory->data != nullptr && bio_memory->length > 0)
				blob.certificate = core::string(bio_memory->data, (size_t)bio_memory->length);

			BIO_free(bio);
			bio = BIO_new(BIO_s_mem());
			PEM_write_bio_PrivateKey(bio, (EVP_PKEY*)private_key, nullptr, nullptr, 0, nullptr, nullptr);

			bio_memory = nullptr;
			if (BIO_get_mem_ptr(bio, &bio_memory) != 0 && bio_memory->data != nullptr && bio_memory->length > 0)
				blob.private_key = core::string(bio_memory->data, (size_t)bio_memory->length);

			BIO_free(bio);
			return blob;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		void* certificate_builder::get_certificate_x509()
		{
			return certificate;
		}
		void* certificate_builder::get_private_key_evppkey()
		{
			return private_key;
		}

		socket::ievents::ievents(ievents&& other) noexcept : read_callback(other.read_callback), write_callback(other.write_callback), expiration(other.expiration), timeout(other.timeout)
		{
			other.expiration = std::chrono::milliseconds(0);
			other.timeout = 0;
		}
		socket::ievents& socket::ievents::operator=(ievents&& other) noexcept
		{
			if (this == &other)
				return *this;

			read_callback = std::move(other.read_callback);
			write_callback = std::move(other.write_callback);
			expiration = other.expiration;
			timeout = other.timeout;
			other.expiration = std::chrono::milliseconds(0);
			other.timeout = 0;
			return *this;
		}

		socket::socket() noexcept : device(nullptr), fd(INVALID_SOCKET), income(0), outcome(0)
		{
			VI_WATCH(this, "socket fd (empty)");
		}
		socket::socket(socket_t from_fd) noexcept : device(nullptr), fd(from_fd), income(0), outcome(0)
		{
			VI_WATCH(this, "socket fd");
		}
		socket::socket(socket&& other) noexcept : events(std::move(other.events)), device(other.device), fd(other.fd), income(other.income), outcome(other.outcome)
		{
			VI_WATCH(this, "socket fd (moved)");
			other.device = nullptr;
			other.fd = INVALID_SOCKET;
		}
		socket::~socket() noexcept
		{
			VI_UNWATCH(this);
			shutdown();
		}
		socket& socket::operator= (socket&& other) noexcept
		{
			if (this == &other)
				return *this;

			shutdown();
			events = std::move(other.events);
			device = other.device;
			fd = other.fd;
			income = other.income;
			outcome = other.outcome;
			other.device = nullptr;
			other.fd = INVALID_SOCKET;
			return *this;
		}
		core::expects_io<void> socket::accept(socket_accept* incoming)
		{
			VI_ASSERT(incoming != nullptr, "incoming socket should be set");
			char address[ADDRESS_SIZE];
			socket_size_t length = sizeof(address);
			auto new_fd = execute_accept(fd, (sockaddr*)&address, &length);
			if (!new_fd)
			{
				VI_TRACE("[net] fd %i: not acceptable", (int)fd);
				return new_fd.error();
			}

			VI_DEBUG("[net] accept fd %i on %i fd", (int)*new_fd, (int)fd);
			incoming->address = socket_address(std::string_view(), 0, (sockaddr*)&address, length);
			incoming->fd = *new_fd;
			return core::expectation::met;
		}
		core::expects_io<void> socket::accept_queued(socket_accepted_callback&& callback)
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			if (!multiplexer::get()->when_readable(this, [this, callback = std::move(callback)](socket_poll event) mutable
			{
				socket_accept incoming;
				if (!packet::is_done(event))
				{
					incoming.fd = INVALID_SOCKET;
					callback(incoming);
					return;
				}

				while (accept(&incoming))
				{
					if (!callback(incoming))
						break;
				}
				accept_queued(std::move(callback));
			}))
				return core::os::error::get_condition_or();

			return core::expectation::met;
		}
		core::expects_promise_io<socket_accept> socket::accept_deferred()
		{
			socket_accept incoming;
			auto status = accept(&incoming);
			if (status)
				return core::expects_promise_io<socket_accept>(std::move(incoming));
			else if (status.error() != std::errc::operation_would_block)
				return core::expects_promise_io<socket_accept>(std::move(status.error()));

			core::expects_promise_io<socket_accept> future;
			if (!multiplexer::get()->when_readable(this, [this, future](socket_poll event) mutable
			{
				if (!future.is_pending())
					return;

				if (packet::is_done(event))
				{
					socket_accept incoming;
					auto status = accept(&incoming);
					if (status)
						future.set(std::move(incoming));
					else
						future.set(std::move(status.error()));
				}
				else
					future.set(std::make_error_condition(std::errc::connection_aborted));
			}))
			{
				if (future.is_pending())
					future.set(core::os::error::get_condition_or());
			}

			return future;
		}
		core::expects_io<void> socket::shutdown(bool gracefully)
		{
#ifdef VI_OPENSSL
			if (device != nullptr)
			{
				VI_TRACE("[net] fd %i free ssl device", (int)fd);
				SSL_free(device);
				device = nullptr;
			}
#endif
			clear_events(gracefully);
			if (fd == INVALID_SOCKET)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			uint8_t buffer;
			set_blocking(false);
			while (read(&buffer, 1));

			int error = 1;
			socklen_t size = sizeof(error);
			getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&error, &size);
			::shutdown(fd, SD_BOTH);
			closesocket(fd);
			VI_DEBUG("[net] sock fd %i shutdown", (int)fd);
			fd = INVALID_SOCKET;
			return core::expectation::met;
		}
		core::expects_io<void> socket::close()
		{
#ifdef VI_OPENSSL
			if (device != nullptr)
			{
				VI_TRACE("[net] fd %i free ssl device", (int)fd);
				SSL_free(device);
				device = nullptr;
			}
#endif
			clear_events(false);
			if (fd == INVALID_SOCKET)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			int error = 1;
			socklen_t size = sizeof(error);
			set_blocking(false);
			getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&error, &size);
			::shutdown(fd, SD_SEND);

			pollfd handle;
			handle.fd = fd;
			handle.events = POLLIN;

			VI_MEASURE(core::timings::networking);
			VI_TRACE("[net] fd %i graceful shutdown: %i", (int)fd, error);
			auto time = core::schedule::get_clock();
			while (core::schedule::get_clock() - time <= std::chrono::milliseconds(10))
			{
				auto status = read((uint8_t*)&error, 1);
				if (status)
				{
					VI_MEASURE_LOOP();
					continue;
				}
				else if (status.error() != std::errc::operation_would_block)
					break;

				handle.revents = 0;
				utils::poll(&handle, 1, 1);
			}

			closesocket(fd);
			VI_DEBUG("[net] sock fd %i closed", (int)fd);
			fd = INVALID_SOCKET;
			return core::expectation::met;
		}
		core::expects_io<void> socket::close_queued(socket_status_callback&& callback)
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
#ifdef VI_OPENSSL
			if (device != nullptr)
			{
				VI_TRACE("[net] fd %i free ssl device", (int)fd);
				SSL_free(device);
				device = nullptr;
			}
#endif
			clear_events(false);
			if (fd == INVALID_SOCKET)
			{
				callback(std::make_error_condition(std::errc::bad_file_descriptor));
				return std::make_error_condition(std::errc::bad_file_descriptor);
			}

			int error = 1;
			socklen_t size = sizeof(error);
			set_blocking(false);
			getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&error, &size);
			::shutdown(fd, SD_SEND);

			VI_TRACE("[net] fd %i graceful shutdown: %i", (int)fd, error);
			return try_close_queued(std::move(callback), core::schedule::get_clock(), true);
		}
		core::expects_promise_io<void> socket::close_deferred()
		{
			core::expects_promise_io<void> future;
			close_queued([future](const core::option<std::error_condition>& error) mutable
			{
				if (error)
					future.set(*error);
				else
					future.set(core::expectation::met);
			});
			return future;
		}
		core::expects_io<void> socket::try_close_queued(socket_status_callback&& callback, const std::chrono::microseconds& time, bool keep_trying)
		{
			events.timeout = CLOSE_TIMEOUT;
			while (keep_trying && core::schedule::get_clock() - time <= std::chrono::milliseconds(10))
			{
				uint8_t buffer;
				auto status = read(&buffer, 1);
				if (status)
					continue;
				else if (status.error() != std::errc::operation_would_block)
					break;

				if (core::schedule::is_available())
				{
					multiplexer::get()->when_readable(this, [this, time, callback = std::move(callback)](socket_poll event) mutable
					{
						if (!packet::is_skip(event))
							try_close_queued(std::move(callback), time, packet::is_done(event));
						else
							callback(core::optional::none);
					});
					return status.error();
				}
				else
				{
					pollfd handle;
					handle.fd = fd;
					handle.events = POLLIN;
					utils::poll(&handle, 1, 1);
				}
			}

			closesocket(fd);
			VI_DEBUG("[net] sock fd %i closed", (int)fd);
			fd = INVALID_SOCKET;

			callback(core::optional::none);
			return core::expectation::met;
		}
		core::expects_io<size_t> socket::write_file(FILE* stream, size_t offset, size_t size)
		{
			VI_ASSERT(stream != nullptr, "stream should be set");
			VI_ASSERT(offset >= 0, "offset should be set and positive");
			VI_ASSERT(size > 0, "size should be set and greater than zero");
			VI_MEASURE(core::timings::networking);
			VI_TRACE("[net] fd %i sendfile %" PRId64 " off, %" PRId64 " bytes", (int)fd, offset, size);
			off_t seek = (off_t)offset, length = (off_t)size;
#ifdef VI_OPENSSL
			if (device != nullptr)
			{
				static bool is_unsupported = false;
				if (is_unsupported)
					return std::make_error_condition(std::errc::not_supported);

				ossl_ssize_t value = SSL_sendfile(device, VI_FILENO(stream), seek, length, 0);
				if (value < 0)
				{
					auto condition = utils::get_last_error(device, (int)value);
					is_unsupported = (condition == std::errc::protocol_error);
					return is_unsupported ? std::make_error_condition(std::errc::not_supported) : condition;
				}

				size_t written = (size_t)value;
				outcome += written;
				return written;
			}
#endif
#ifdef VI_APPLE
			int value = sendfile(VI_FILENO(stream), fd, seek, &length, nullptr, 0);
			if (value < 0)
				return utils::get_last_error(device, value);

			size_t written = (size_t)length;
			outcome += written;
			return written;
#elif defined(VI_LINUX)
			ssize_t value = sendfile(fd, VI_FILENO(stream), &seek, size);
			if (value < 0)
				return utils::get_last_error(device, (int)value);

			size_t written = (size_t)value;
			outcome += written;
			return written;
#else
			(void)seek;
			(void)length;
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<size_t> socket::write_file_queued(FILE* stream, size_t offset, size_t size, socket_written_callback&& callback, size_t temp_buffer)
		{
			VI_ASSERT(stream != nullptr, "stream should be set");
			VI_ASSERT(callback != nullptr, "callback should be set");
			VI_ASSERT(offset >= 0, "offset should be set and positive");
			VI_ASSERT(size > 0, "size should be set and greater than zero");
			if (fd == INVALID_SOCKET)
			{
				callback(socket_poll::reset);
				return std::make_error_condition(std::errc::bad_file_descriptor);
			}

			size_t written = 0;
			while (size > 0)
			{
				auto status = write_file(stream, offset, size);
				if (!status)
				{
					if (status.error() == std::errc::operation_would_block)
					{
						multiplexer::get()->when_writeable(this, [this, temp_buffer, stream, offset, size, callback = std::move(callback)](socket_poll event) mutable
						{
							if (packet::is_done(event))
								write_file_queued(stream, offset, size, std::move(callback), ++temp_buffer);
							else
								callback(event);
						});
					}
					else if (status.error() != std::errc::not_supported)
						callback(socket_poll::reset);

					return status;
				}
				else
				{
					size_t written_size = *status;
					size -= written_size;
					offset += written_size;
					written += written_size;
				}
			}

			callback(temp_buffer != 0 ? socket_poll::finish : socket_poll::finish_sync);
			return written;
		}
		core::expects_promise_io<size_t> socket::write_file_deferred(FILE* stream, size_t offset, size_t size)
		{
			core::expects_promise_io<size_t> future;
			write_file_queued(stream, offset, size, [future, size](socket_poll event) mutable
			{
				if (packet::is_done(event))
					future.set(size);
				else
					future.set(packet::to_condition(event));
			});
			return future;
		}
		core::expects_io<size_t> socket::write(const uint8_t* buffer, size_t size)
		{
			VI_MEASURE(core::timings::networking);
			if (fd == INVALID_SOCKET)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			VI_TRACE("[net] fd %i write %i bytes", (int)fd, (int)size);
#ifdef VI_OPENSSL
			if (device != nullptr)
			{
				int value = SSL_write(device, buffer, (int)size);
				if (value <= 0)
					return utils::get_last_error(device, value);

				size_t written = (size_t)value;
				outcome += written;
				return written;
			}
#endif
			int value = (int)send(fd, (char*)buffer, (int)size, 0);
			if (value == 0)
				return std::make_error_condition(std::errc::operation_would_block);
			else if (value < 0)
				return utils::get_last_error(device, value);

			size_t written = (size_t)value;
			outcome += written;
			return written;
		}
		core::expects_io<size_t> socket::write_queued(const uint8_t* buffer, size_t size, socket_written_callback&& callback, bool copy_buffer_when_async, uint8_t* temp_buffer, size_t temp_offset)
		{
			VI_ASSERT(buffer != nullptr && size > 0, "buffer should be set");
			VI_ASSERT(callback != nullptr, "callback should be set");
			if (fd == INVALID_SOCKET)
			{
				if (copy_buffer_when_async && temp_buffer != nullptr)
					core::memory::deallocate(temp_buffer);
				callback(socket_poll::reset);
				return std::make_error_condition(std::errc::bad_file_descriptor);
			}

			size_t payload = size;
			size_t written = 0;

			while (size > 0)
			{
				auto status = write(buffer + temp_offset, (int)size);
				if (!status)
				{
					if (status.error() == std::errc::operation_would_block)
					{
						if (!temp_buffer)
						{
							if (copy_buffer_when_async)
							{
								temp_buffer = core::memory::allocate<uint8_t>(payload);
								memcpy(temp_buffer, buffer, payload);
							}
							else
								temp_buffer = (uint8_t*)buffer;
						}

						multiplexer::get()->when_writeable(this, [this, copy_buffer_when_async, temp_buffer, temp_offset, size, callback = std::move(callback)](socket_poll event) mutable
						{
							if (!packet::is_done(event))
							{
								if (copy_buffer_when_async && temp_buffer != nullptr)
									core::memory::deallocate(temp_buffer);
								callback(event);
							}
							else
								write_queued(temp_buffer, size, std::move(callback), copy_buffer_when_async, temp_buffer, temp_offset);
						});
					}
					else
					{
						if (copy_buffer_when_async && temp_buffer != nullptr)
							core::memory::deallocate(temp_buffer);
						callback(socket_poll::reset);
					}

					return status;
				}
				else
				{
					size_t written_size = *status;
					size -= written_size;
					temp_offset += written_size;
					written += written_size;
				}
			}

			if (copy_buffer_when_async && temp_buffer != nullptr)
				core::memory::deallocate(temp_buffer);

			callback(temp_buffer ? socket_poll::finish : socket_poll::finish_sync);
			return written;
		}
		core::expects_promise_io<size_t> socket::write_deferred(const uint8_t* buffer, size_t size, bool copy_buffer_when_async)
		{
			core::expects_promise_io<size_t> future;
			write_queued(buffer, size, [future, size](socket_poll event) mutable
			{
				if (packet::is_done(event))
					future.set(size);
				else
					future.set(packet::to_condition(event));
			}, copy_buffer_when_async);
			return future;
		}
		core::expects_io<size_t> socket::read(uint8_t* buffer, size_t size)
		{
			VI_ASSERT(buffer != nullptr, "buffer should be set");
			VI_MEASURE(core::timings::networking);
			if (fd == INVALID_SOCKET)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			VI_TRACE("[net] fd %i read %i bytes", (int)fd, (int)size);
#ifdef VI_OPENSSL
			if (device != nullptr)
			{
				int value = SSL_read(device, buffer, (int)size);
				if (value <= 0)
					return utils::get_last_error(device, value);

				size_t received = (size_t)value;
				income += received;
				return received;
			}
#endif
			int value = (int)recv(fd, (char*)buffer, (int)size, 0);
			if (value == 0)
				return std::make_error_condition(std::errc::connection_reset);
			else if (value < 0)
				return utils::get_last_error(device, value);

			size_t received = (size_t)value;
			income += received;
			return received;
		}
		core::expects_io<size_t> socket::read_queued(size_t size, socket_read_callback&& callback, size_t temp_buffer)
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			VI_MEASURE(core::timings::networking);
			if (fd == INVALID_SOCKET)
			{
				callback(socket_poll::reset, nullptr, 0);
				return std::make_error_condition(std::errc::bad_file_descriptor);
			}

			uint8_t buffer[core::BLOB_SIZE];
			size_t offset = 0;

			while (size > 0)
			{
				auto status = read(buffer, size > sizeof(buffer) ? sizeof(buffer) : size);
				if (!status)
				{
					if (status.error() == std::errc::operation_would_block)
					{
						multiplexer::get()->when_readable(this, [this, size, temp_buffer, callback = std::move(callback)](socket_poll event) mutable
						{
							if (packet::is_done(event))
								read_queued(size, std::move(callback), ++temp_buffer);
							else
								callback(event, nullptr, 0);
						});
					}
					else
						callback(socket_poll::reset, nullptr, 0);

					return status;
				}

				size_t received = *status;
				if (!callback(socket_poll::next, buffer, received))
					break;

				size -= received;
				offset += received;
			}

			callback(temp_buffer != 0 ? socket_poll::finish : socket_poll::finish_sync, nullptr, 0);
			return offset;
		}
		core::expects_promise_io<core::string> socket::read_deferred(size_t size)
		{
			core::string* merge = core::memory::init<core::string>();
			merge->reserve(size);

			core::expects_promise_io<core::string> future;
			read_queued(size, [future, merge](socket_poll event, const uint8_t* buffer, size_t size) mutable
			{
				if (packet::is_done(event))
				{
					future.set(std::move(*merge));
					core::memory::deinit(merge);
				}
				else if (!packet::is_data(event))
				{
					future.set(packet::to_condition(event));
					core::memory::deinit(merge);
				}
				else if (buffer != nullptr && size > 0)
					merge->append((char*)buffer, size);

				return true;
			});
			return future;
		}
		core::expects_io<size_t> socket::read_until(const std::string_view& match, socket_read_callback&& callback)
		{
			VI_ASSERT(!match.empty(), "match should be set");
			VI_ASSERT(callback != nullptr, "callback should be set");
			VI_MEASURE(core::timings::networking);
			if (fd == INVALID_SOCKET)
			{
				callback(socket_poll::reset, nullptr, 0);
				return std::make_error_condition(std::errc::bad_file_descriptor);
			}

			uint8_t buffer[MAX_READ_UNTIL];
			size_t size = match.size(), cache = 0, receiving = 0, index = 0;
			auto notify_incoming = [&callback, &buffer, &cache, &receiving]()
			{
				if (!cache)
					return true;

				size_t size = cache;
				cache = 0; receiving += size;
				return callback(socket_poll::next, buffer, size);
			};

			VI_ASSERT(size > 0, "match should not be empty");
			while (index < size)
			{
				auto status = read(buffer + cache, 1);
				if (!status)
				{
					if (notify_incoming())
						callback(socket_poll::reset, nullptr, 0);
					return status;
				}

				size_t offset = cache;
				if (++cache >= sizeof(buffer) && !notify_incoming())
					break;

				if (match[index] == buffer[offset])
				{
					if (++index >= size)
					{
						if (notify_incoming())
							callback(socket_poll::finish_sync, nullptr, 0);
						break;
					}
				}
				else
					index = 0;
			}

			return receiving;
		}
		core::expects_io<size_t> socket::read_until_queued(core::string&& match, socket_read_callback&& callback, size_t temp_index, bool temp_buffer)
		{
			VI_ASSERT(!match.empty(), "match should be set");
			VI_ASSERT(callback != nullptr, "callback should be set");
			VI_MEASURE(core::timings::networking);
			if (fd == INVALID_SOCKET)
			{
				callback(socket_poll::reset, nullptr, 0);
				return std::make_error_condition(std::errc::bad_file_descriptor);
			}

			uint8_t buffer[MAX_READ_UNTIL];
			size_t size = match.size(), cache = 0, receiving = 0;
			auto notify_incoming = [&callback, &buffer, &cache, &receiving]()
			{
				if (!cache)
					return true;

				size_t size = cache;
				cache = 0; receiving += size;
				return callback(socket_poll::next, buffer, size);
			};

			VI_ASSERT(size > 0, "match should not be empty");
			while (temp_index < size)
			{
				auto status = read(buffer + cache, 1);
				if (!status)
				{
					if (status.error() == std::errc::operation_would_block)
					{
						multiplexer::get()->when_readable(this, [this, temp_index, match = std::move(match), callback = std::move(callback)](socket_poll event) mutable
						{
							if (packet::is_done(event))
								read_until_queued(std::move(match), std::move(callback), temp_index, true);
							else
								callback(event, nullptr, 0);
						});
					}
					else if (notify_incoming())
						callback(socket_poll::reset, nullptr, 0);

					return status;
				}

				size_t offset = cache++;
				if (cache >= sizeof(buffer) && !notify_incoming())
					break;

				if (match[temp_index] == buffer[offset])
				{
					if (++temp_index >= size)
					{
						if (notify_incoming())
							callback(temp_buffer ? socket_poll::finish : socket_poll::finish_sync, nullptr, 0);
						break;
					}
				}
				else
					temp_index = 0;
			}

			return receiving;
		}
		core::expects_promise_io<core::string> socket::read_until_deferred(core::string&& match, size_t max_size)
		{
			core::string* merge = core::memory::init<core::string>();
			merge->reserve(max_size);

			core::expects_promise_io<core::string> future;
			read_until_queued(std::move(match), [future, merge, max_size](socket_poll event, const uint8_t* buffer, size_t size) mutable
			{
				if (packet::is_done(event))
				{
					future.set(std::move(*merge));
					core::memory::deinit(merge);
				}
				else if (!packet::is_data(event))
				{
					future.set(packet::to_condition(event));
					core::memory::deinit(merge);
				}
				else if (buffer != nullptr && size > 0)
				{
					size = std::min<size_t>(max_size, size);
					merge->append((char*)buffer, size);
					max_size -= size;
					return max_size > 0;
				}

				return true;
			});
			return future;
		}
		core::expects_io<size_t> socket::read_until_chunked(const std::string_view& match, socket_read_callback&& callback)
		{
			VI_ASSERT(!match.empty(), "match should be set");
			VI_ASSERT(callback != nullptr, "callback should be set");
			VI_MEASURE(core::timings::networking);
			if (fd == INVALID_SOCKET)
			{
				callback(socket_poll::reset, nullptr, 0);
				return std::make_error_condition(std::errc::bad_file_descriptor);
			}

			uint8_t buffer[MAX_READ_UNTIL];
			size_t size = match.size(), cache = 0, receiving = 0, index = 0;
			auto notify_incoming = [&callback, &buffer, &cache, &receiving]()
			{
				if (!cache)
					return true;

				size_t size = cache;
				cache = 0; receiving += size;
				return callback(socket_poll::next, buffer, size);
			};

			VI_ASSERT(size > 0, "match should not be empty");
			while (index < size)
			{
				auto status = read(buffer + cache, sizeof(buffer) - cache);
				if (!status)
				{
					if (notify_incoming())
						callback(socket_poll::reset, nullptr, 0);
					return status;
				}

				size_t length = *status;
				size_t remaining = length;
				while (remaining-- > 0)
				{
					size_t offset = cache;
					if (++cache >= length && !notify_incoming())
						break;

					if (match[index] == buffer[offset])
					{
						if (++index >= size)
						{
							if (notify_incoming())
								callback(socket_poll::finish_sync, nullptr, 0);
							break;
						}
					}
					else
						index = 0;
				}
			}

			return receiving;
		}
		core::expects_io<size_t> socket::read_until_chunked_queued(core::string&& match, socket_read_callback&& callback, size_t temp_index, bool temp_buffer)
		{
			VI_ASSERT(!match.empty(), "match should be set");
			VI_ASSERT(callback != nullptr, "callback should be set");
			VI_MEASURE(core::timings::networking);
			if (fd == INVALID_SOCKET)
			{
				callback(socket_poll::reset, nullptr, 0);
				return std::make_error_condition(std::errc::bad_file_descriptor);
			}

			uint8_t buffer[MAX_READ_UNTIL];
			size_t size = match.size(), cache = 0, receiving = 0, remaining = 0;
			auto notify_incoming = [&callback, &buffer, &cache, &receiving]()
			{
				if (!cache)
					return true;

				size_t size = cache;
				cache = 0; receiving += size;
				return callback(socket_poll::next, buffer, size);
			};

			VI_ASSERT(size > 0, "match should not be empty");
			while (temp_index < size)
			{
				auto status = read(buffer + cache, sizeof(buffer) - cache);
				if (!status)
				{
					if (status.error() == std::errc::operation_would_block)
					{
						multiplexer::get()->when_readable(this, [this, temp_index, match = std::move(match), callback = std::move(callback)](socket_poll event) mutable
						{
							if (packet::is_done(event))
								read_until_chunked_queued(std::move(match), std::move(callback), temp_index, true);
							else
								callback(event, nullptr, 0);
						});
					}
					else if (notify_incoming())
						callback(socket_poll::reset, nullptr, 0);

					return status;
				}

				size_t length = *status;
				remaining = length;
				while (remaining-- > 0)
				{
					size_t offset = cache;
					if (++cache >= length && !notify_incoming())
						break;

					if (match[temp_index] == buffer[offset])
					{
						if (++temp_index >= size)
						{
							if (notify_incoming())
							{
								cache = ++offset;
								callback(temp_buffer ? socket_poll::finish : socket_poll::finish_sync, remaining > 0 ? buffer + cache : nullptr, remaining);
							}
							break;
						}
					}
					else
						temp_index = 0;
				}
			}

			return receiving;
		}
		core::expects_promise_io<core::string> socket::read_until_chunked_deferred(core::string&& match, size_t max_size)
		{
			core::string* merge = core::memory::init<core::string>();
			merge->reserve(max_size);

			core::expects_promise_io<core::string> future;
			read_until_chunked_queued(std::move(match), [future, merge, max_size](socket_poll event, const uint8_t* buffer, size_t size) mutable
			{
				if (packet::is_done(event))
				{
					future.set(std::move(*merge));
					core::memory::deinit(merge);
				}
				else if (!packet::is_data(event))
				{
					future.set(packet::to_condition(event));
					core::memory::deinit(merge);
				}
				else if (buffer != nullptr && size > 0)
				{
					size = std::min<size_t>(max_size, size);
					merge->append((char*)buffer, size);
					max_size -= size;
					return max_size > 0;
				}

				return true;
			});
			return future;
		}
		core::expects_io<void> socket::connect(const socket_address& address, uint64_t timeout)
		{
			VI_MEASURE(core::timings::networking);
			VI_DEBUG("[net] connect fd %i", (int)fd);
			if (::connect(fd, address.get_raw_address(), (int)address.get_address_size()) != 0)
				return core::os::error::get_condition_or();

			return core::expectation::met;
		}
		core::expects_io<void> socket::connect_queued(const socket_address& address, socket_status_callback&& callback)
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			VI_MEASURE(core::timings::networking);
			if (fd == INVALID_SOCKET)
			{
				callback(std::make_error_condition(std::errc::bad_file_descriptor));
				return std::make_error_condition(std::errc::bad_file_descriptor);
			}

			VI_DEBUG("[net] connect fd %i", (int)fd);
			int status = ::connect(fd, address.get_raw_address(), (int)address.get_address_size());
			if (status == 0)
			{
				callback(core::optional::none);
				return core::expectation::met;
			}
			else if (utils::get_last_error(device, status) != std::errc::operation_would_block)
			{
				auto condition = core::os::error::get_condition_or();
				callback(condition);
				return condition;
			}

			multiplexer::get()->when_writeable(this, [callback = std::move(callback)](socket_poll event) mutable
			{
				if (packet::is_done(event))
					callback(core::optional::none);
				else if (packet::is_timeout(event))
					callback(std::make_error_condition(std::errc::timed_out));
				else
					callback(std::make_error_condition(std::errc::connection_refused));
			});
			return std::make_error_condition(std::errc::operation_would_block);
		}
		core::expects_promise_io<void> socket::connect_deferred(const socket_address& address)
		{
			core::expects_promise_io<void> future;
			connect_queued(address, [future](const core::option<std::error_condition>& error) mutable
			{
				if (error)
					future.set(*error);
				else
					future.set(core::expectation::met);
			});
			return future;
		}
		core::expects_io<void> socket::open(const socket_address& address)
		{
			VI_MEASURE(core::timings::networking);
			VI_DEBUG("[net] assign fd");
			if (!core::os::control::has(core::access_option::net))
				return std::make_error_condition(std::errc::permission_denied);

			auto openable = execute_socket(address.get_family(), address.get_type(), address.get_protocol());
			if (!openable)
				return openable.error();

			int option = 1; fd = *openable;
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&option, sizeof(option));
			return core::expectation::met;
		}
		core::expects_io<void> socket::secure(ssl_ctx_st* context, const std::string_view& hostname)
		{
#ifdef VI_OPENSSL
			VI_MEASURE(core::timings::networking);
			VI_ASSERT(hostname.empty() || core::stringify::is_cstring(hostname), "hostname should be set");
			VI_TRACE("[net] fd %i create ssl device on %.*s", (int)fd, (int)hostname.size(), hostname.data());
			if (device != nullptr)
				SSL_free(device);

			device = SSL_new(context);
			if (!device)
				return std::make_error_condition(std::errc::broken_pipe);

			SSL_set_mode(device, SSL_MODE_ENABLE_PARTIAL_WRITE);
			SSL_set_mode(device, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
#ifndef OPENSSL_NO_TLSEXT
			if (!hostname.empty())
				SSL_set_tlsext_host_name(device, hostname.data());
#endif
#endif
			return core::expectation::met;
		}
		core::expects_io<void> socket::bind(const socket_address& address)
		{
			VI_MEASURE(core::timings::networking);
			VI_DEBUG("[net] bind fd %i", (int)fd);

			if (::bind(fd, address.get_raw_address(), (int)address.get_address_size()) != 0)
				return core::os::error::get_condition_or();

			return core::expectation::met;
		}
		core::expects_io<void> socket::listen(int backlog)
		{
			VI_MEASURE(core::timings::networking);
			VI_DEBUG("[net] listen fd %i", (int)fd);
			if (::listen(fd, backlog) != 0)
				return core::os::error::get_condition_or();

			return core::expectation::met;
		}
		core::expects_io<void> socket::clear_events(bool gracefully)
		{
			VI_MEASURE(core::timings::networking);
			VI_TRACE("[net] fd %i clear events %s", (int)fd, gracefully ? "gracefully" : "forcefully");
			auto* handle = multiplexer::get();
			bool success = gracefully ? handle->cancel_events(this, socket_poll::reset) : handle->clear_events(this);
			if (!success)
				return std::make_error_condition(std::errc::not_connected);

			return core::expectation::met;
		}
		core::expects_io<void> socket::migrate_to(socket_t new_fd, bool gracefully)
		{
			VI_MEASURE(core::timings::networking);
			VI_TRACE("[net] migrate fd %i to fd %i", (int)fd, (int)new_fd);
			if (!gracefully)
			{
				fd = new_fd;
				return core::expectation::met;
			}

			auto status = clear_events(false);
			fd = new_fd;
			return status;
		}
		core::expects_io<void> socket::set_close_on_exec()
		{
			VI_TRACE("[net] fd %i setopt: cloexec", (int)fd);
#if defined(_WIN32)
			return std::make_error_condition(std::errc::not_supported);
#else
			if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
				return core::os::error::get_condition_or();

			return core::expectation::met;
#endif
		}
		core::expects_io<void> socket::set_time_wait(int timeout)
		{
			linger linger;
			linger.l_onoff = (timeout >= 0 ? 1 : 0);
			linger.l_linger = timeout;

			VI_TRACE("[net] fd %i setopt: timewait %i", (int)fd, timeout);
			if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger)) != 0)
				return core::os::error::get_condition_or();

			return core::expectation::met;
		}
		core::expects_io<void> socket::set_socket(int option, void* value, size_t size)
		{
			VI_TRACE("[net] fd %i setsockopt: opt%i %i bytes", (int)fd, option, (int)size);
			if (::setsockopt(fd, SOL_SOCKET, option, (const char*)value, (int)size) != 0)
				return core::os::error::get_condition_or();

			return core::expectation::met;
		}
		core::expects_io<void> socket::set_any(int level, int option, void* value, size_t size)
		{
			VI_TRACE("[net] fd %i setopt: l%i opt%i %i bytes", (int)fd, level, option, (int)size);
			if (::setsockopt(fd, level, option, (const char*)value, (int)size) != 0)
				return core::os::error::get_condition_or();

			return core::expectation::met;
		}
		core::expects_io<void> socket::set_socket_flag(int option, int value)
		{
			return set_socket(option, (void*)&value, sizeof(int));
		}
		core::expects_io<void> socket::set_any_flag(int level, int option, int value)
		{
			VI_TRACE("[net] fd %i setopt: l%i opt%i %i", (int)fd, level, option, value);
			return set_any(level, option, (void*)&value, sizeof(int));
		}
		core::expects_io<void> socket::set_blocking(bool enabled)
		{
			return set_socket_blocking(fd, enabled);
		}
		core::expects_io<void> socket::set_no_delay(bool enabled)
		{
			return set_any_flag(IPPROTO_TCP, TCP_NODELAY, (enabled ? 1 : 0));
		}
		core::expects_io<void> socket::set_keep_alive(bool enabled)
		{
			return set_socket_flag(SO_KEEPALIVE, (enabled ? 1 : 0));
		}
		core::expects_io<void> socket::set_timeout(int timeout)
		{
			VI_TRACE("[net] fd %i setopt: rwtimeout %i", (int)fd, timeout);
#ifdef VI_MICROSOFT
			DWORD time = (DWORD)timeout;
#else
			struct timeval time;
			memset(&time, 0, sizeof(time));
			time.tv_sec = timeout / 1000;
			time.tv_usec = (timeout * 1000) % 1000000;
#endif
			if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof(time)) != 0)
				return core::os::error::get_condition_or();

			if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&time, sizeof(time)) != 0)
				return core::os::error::get_condition_or();

			return core::expectation::met;
		}
		core::expects_io<void> socket::get_socket(int option, void* value, size_t* size)
		{
			return get_any(SOL_SOCKET, option, value, size);
		}
		core::expects_io<void> socket::get_any(int level, int option, void* value, size_t* size)
		{
			socket_size_t length = (socket_size_t)level;
			if (::getsockopt(fd, level, option, (char*)value, &length) == -1)
				return core::os::error::get_condition_or();

			if (size != nullptr)
				*size = (size_t)length;

			return core::expectation::met;
		}
		core::expects_io<void> socket::get_socket_flag(int option, int* value)
		{
			size_t size = sizeof(int);
			return get_socket(option, (void*)value, &size);
		}
		core::expects_io<void> socket::get_any_flag(int level, int option, int* value)
		{
			return get_any(level, option, (char*)value, nullptr);
		}
		core::expects_io<socket_address> socket::get_peer_address()
		{
			struct sockaddr_storage address;
			socklen_t length = sizeof(address);
			if (getpeername(fd, (sockaddr*)&address, &length) == -1)
				return core::os::error::get_condition_or();

			return socket_address(std::string_view(), 0, (sockaddr*)&address, length);
		}
		core::expects_io<socket_address> socket::get_this_address()
		{
			struct sockaddr_storage address;
			socket_size_t length = sizeof(address);
			if (getsockname(fd, reinterpret_cast<struct sockaddr*>(&address), &length) == -1)
				return core::os::error::get_condition_or();

			return socket_address(std::string_view(), 0, (sockaddr*)&address, length);
		}
		core::expects_io<certificate> socket::get_certificate()
		{
			if (!device)
				return std::make_error_condition(std::errc::not_supported);
#ifdef VI_OPENSSL
			X509* blob = SSL_get_peer_certificate(device);
			if (!blob)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			return utils::get_certificate_from_x509(blob);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		void socket::set_io_timeout(uint64_t timeout_ms)
		{
			events.timeout = timeout_ms;
		}
		socket_t socket::get_fd() const
		{
			return fd;
		}
		ssl_st* socket::get_device() const
		{
			return device;
		}
		bool socket::is_valid() const
		{
			return fd != INVALID_SOCKET;
		}
		bool socket::is_awaiting_events()
		{
			core::umutex<std::mutex> unique(events.mutex);
			return events.read_callback || events.write_callback;
		}
		bool socket::is_awaiting_readable()
		{
			core::umutex<std::mutex> unique(events.mutex);
			return !!events.read_callback;
		}
		bool socket::is_awaiting_writeable()
		{
			core::umutex<std::mutex> unique(events.mutex);
			return !!events.write_callback;
		}
		bool socket::is_secure() const
		{
			return device != nullptr;
		}

		socket_listener::socket_listener(const std::string_view& new_name, const socket_address& new_address, bool secure) : name(new_name), address(new_address), stream(new socket()), is_secure(secure)
		{
		}
		socket_listener::~socket_listener() noexcept
		{
			if (stream != nullptr)
				stream->shutdown();
			core::memory::release(stream);
		}

		socket_router::~socket_router() noexcept
		{
#ifdef VI_OPENSSL
			auto* transporter = transport_layer::get();
			for (auto& item : certificates)
				transporter->free_server_context(item.second.context);
			certificates.clear();
#endif
		}
		core::expects_system<router_listener*> socket_router::listen(const std::string_view& hostname, const std::string_view& service, bool secure)
		{
			return listen("*", hostname, service, secure);
		}
		core::expects_system<router_listener*> socket_router::listen(const std::string_view& pattern, const std::string_view& hostname, const std::string_view& service, bool secure)
		{
			auto address = dns::get()->lookup(hostname, service, dns_type::listen, socket_protocol::TCP, socket_type::stream);
			if (!address)
				return address.error();

			router_listener& info = listeners[core::string(pattern.empty() ? "*" : pattern)];
			info.address = std::move(*address);
			info.is_secure = secure;
			return &info;
		}

		socket_connection::~socket_connection() noexcept
		{
			core::memory::release(stream);
			core::memory::release(host);
		}
		bool socket_connection::abort()
		{
			info.abort = true;
			return next(-1);
		}
		bool socket_connection::abort(int status_code, const char* error_message, ...)
		{
			char buffer[core::BLOB_SIZE];
			if (info.abort)
				return next();

			va_list args;
			va_start(args, error_message);
			int count = vsnprintf(buffer, sizeof(buffer), error_message, args);
			va_end(args);

			info.message.assign(buffer, count);
			return next(status_code);
		}
		bool socket_connection::next()
		{
			return false;
		}
		bool socket_connection::next(int)
		{
			return next();
		}
		bool socket_connection::closable(socket_router* router)
		{
			if (info.abort)
				return true;
			else if (info.reuses > 1)
				return false;

			return router && router->keep_alive_max_count != 0;
		}
		void socket_connection::reset(bool fully)
		{
			if (fully)
			{
				info.message.clear();
				info.reuses = 1;
				info.start = 0;
				info.finish = 0;
				info.timeout = 0;
				info.abort = false;
			}

			if (stream != nullptr)
			{
				stream->income = 0;
				stream->outcome = 0;
			}
		}

		socket_server::socket_server() noexcept : router(nullptr), shutdown_timeout((uint64_t)core::timings::hangup), backlog(1024), state(server_state::idle)
		{
			multiplexer::get()->activate();
		}
		socket_server::~socket_server() noexcept
		{
			unlisten(false);
			if (network::multiplexer::has_instance())
				multiplexer::get()->deactivate();
		}
		void socket_server::set_router(socket_router* init)
		{
			core::memory::release(router);
			router = init;
		}
		void socket_server::set_backlog(size_t value)
		{
			VI_ASSERT(value > 0, "backlog must be greater than zero");
			backlog = value;
		}
		void socket_server::set_shutdown_timeout(uint64_t timeout_ms)
		{
			shutdown_timeout = timeout_ms;
		}
		core::expects_system<void> socket_server::configure(socket_router* new_router)
		{
			VI_ASSERT(state == server_state::idle, "server should not be running");
			if (new_router != nullptr)
			{
				if (router != new_router)
				{
					core::memory::release(router);
					router = new_router;
				}
			}
			else if (!router && !(router = on_allocate_router()))
			{
				VI_ASSERT(false, "router is not valid");
				return core::system_exception("configure server: invalid router", std::make_error_condition(std::errc::invalid_argument));
			}

			auto configure_status = on_configure(router);
			if (!configure_status)
				return configure_status;

			if (router->listeners.empty())
			{
				VI_ASSERT(false, "there are no listeners provided");
				return core::system_exception("configure server: invalid listeners", std::make_error_condition(std::errc::invalid_argument));
			}

			for (auto&& it : router->listeners)
			{
				socket_listener* host = new socket_listener(it.first, it.second.address, it.second.is_secure);
				auto status = host->stream->open(host->address);
				if (!status)
					return core::system_exception(core::stringify::text("open %s listener error", get_address_identification(host->address).c_str()), std::move(status.error()));

				status = host->stream->bind(host->address);
				if (!status)
					return core::system_exception(core::stringify::text("bind %s listener error", get_address_identification(host->address).c_str()), std::move(status.error()));

				status = host->stream->listen((int)router->backlog_queue);
				if (!status)
					return core::system_exception(core::stringify::text("listen %s listener error", get_address_identification(host->address).c_str()), std::move(status.error()));

				host->stream->set_close_on_exec();
				host->stream->set_blocking(false);
				listeners.push_back(host);
			}
#ifdef VI_OPENSSL
			for (auto&& it : router->certificates)
			{
				auto context = transport_layer::get()->create_server_context(it.second.verify_peers, it.second.options, it.second.ciphers);
				if (!context)
					return core::system_exception("create server transport layer error: " + it.first, std::move(context.error()));

				it.second.context = *context;
				if (it.second.verify_peers > 0)
					SSL_CTX_set_verify(it.second.context, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, nullptr);
				else
					SSL_CTX_set_verify(it.second.context, SSL_VERIFY_NONE, nullptr);

				if (it.second.blob.certificate.empty() || it.second.blob.private_key.empty())
					return core::system_exception("invalid server transport layer private key or certificate error: " + it.first, std::make_error_condition(std::errc::invalid_argument));

				BIO* certificate_bio = BIO_new_mem_buf((void*)it.second.blob.certificate.c_str(), (int)it.second.blob.certificate.size());
				if (!certificate_bio)
					return core::system_exception(core::stringify::text("server transport layer certificate %s memory error: %s", it.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::not_enough_memory));

				X509* certificate = PEM_read_bio_X509(certificate_bio, nullptr, 0, nullptr);
				BIO_free(certificate_bio);
				if (!certificate)
					return core::system_exception(core::stringify::text("server transport layer certificate %s parse error: %s", it.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::invalid_argument));

				if (SSL_CTX_use_certificate(it.second.context, certificate) != 1)
					return core::system_exception(core::stringify::text("server transport layer certificate %s import error: %s", it.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::bad_message));

				BIO* private_key_bio = BIO_new_mem_buf((void*)it.second.blob.private_key.c_str(), (int)it.second.blob.private_key.size());
				if (!private_key_bio)
					return core::system_exception(core::stringify::text("server transport layer private key %s memory error: %s", it.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::not_enough_memory));

				EVP_PKEY* private_key = PEM_read_bio_PrivateKey(private_key_bio, nullptr, 0, nullptr);
				BIO_free(private_key_bio);
				if (!private_key)
					return core::system_exception(core::stringify::text("server transport layer private key %s parse error: %s", it.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::invalid_argument));

				if (SSL_CTX_use_PrivateKey(it.second.context, private_key) != 1)
					return core::system_exception(core::stringify::text("server transport layer private key %s import error: %s", it.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::bad_message));

				if (!SSL_CTX_check_private_key(it.second.context))
					return core::system_exception(core::stringify::text("server transport layer private key %s verify error: %s", it.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::bad_message));
			}
#endif
			return core::expectation::met;
		}
		core::expects_system<void> socket_server::unlisten(bool gracefully)
		{
			VI_MEASURE(shutdown_timeout);
			if (!router && state == server_state::idle)
				return core::expectation::met;

			VI_DEBUG("[net] shutdown server %s", gracefully ? "gracefully" : "fast");
			core::single_queue<socket_connection*> queue;
			state = server_state::stopping;
			{
				core::umutex<std::recursive_mutex> unique(exclusive);
				for (auto it : listeners)
					it->stream->shutdown();

				for (auto* base : active)
					queue.push(base);

				while (!queue.empty())
				{
					auto* base = queue.front();
					base->info.abort = true;
					if (!core::schedule::is_available())
						base->stream->set_blocking(true);
					if (gracefully)
						base->stream->clear_events(true);
					else
						base->stream->shutdown(true);
					queue.pop();
				}
			}

			auto timeout = std::chrono::milliseconds(shutdown_timeout);
			auto time = core::schedule::get_clock();
			do
			{
				if (shutdown_timeout > 0 && core::schedule::get_clock() - time > timeout)
				{
					core::umutex<std::recursive_mutex> unique(exclusive);
					VI_DEBUG("[stall] server is leaking connections: %i", (int)active.size());
					for (auto* next : active)
						on_request_stall(next);
					break;
				}

				free_all();
				std::this_thread::sleep_for(std::chrono::microseconds(SERVER_BLOCKED_WAIT_US));
				VI_MEASURE_LOOP();
			} while (core::schedule::is_available() && (!inactive.empty() || !active.empty()));

			auto unlisten_status = on_unlisten();
			if (!unlisten_status)
				return unlisten_status;

			if (!core::schedule::is_available())
			{
				core::umutex<std::recursive_mutex> unique(exclusive);
				if (!active.empty())
				{
					inactive.insert(active.begin(), active.end());
					active.clear();
				}
			}

			free_all();
			auto after_unlisten_status = on_after_unlisten();
			if (!after_unlisten_status)
				return after_unlisten_status;

			for (auto it : listeners)
				core::memory::release(it);

			core::memory::release(router);
			listeners.clear();
			state = server_state::idle;
			router = nullptr;
			return core::expectation::met;
		}
		core::expects_system<void> socket_server::listen()
		{
			if (listeners.empty() || state != server_state::idle)
				return core::expectation::met;

			state = server_state::working;
			auto listen_status = on_listen();
			if (!listen_status)
				return listen_status;

			VI_DEBUG("[net] listen server on %i listeners", (int)listeners.size());
			for (auto&& source : listeners)
			{
				source->stream->accept_queued([this, source](socket_accept& incoming)
				{
					if (state != server_state::working)
					{
						if (incoming.fd != INVALID_SOCKET)
							closesocket(incoming.fd);
						return false;
					}
					else if (incoming.fd == INVALID_SOCKET)
						return false;

					core::cospawn([this, source, incoming]() mutable { accept(source, std::move(incoming)); });
					return true;
				});
			}

			return core::expectation::met;
		}
		core::expects_io<void> socket_server::refuse(socket_connection* base)
		{
			return base->stream->close_queued([this, base](const core::option<std::error_condition>&) { push(base); });
		}
		core::expects_io<void> socket_server::accept(socket_listener* host, socket_accept&& incoming)
		{
			auto* base = pop(host);
			if (!base)
			{
				closesocket(incoming.fd);
				return std::make_error_condition(state == server_state::working ? std::errc::not_enough_memory : std::errc::connection_aborted);
			}

			base->address = std::move(incoming.address);
			base->stream->set_io_timeout(router->socket_timeout);
			base->stream->migrate_to(incoming.fd, false);
			base->stream->set_close_on_exec();
			base->stream->set_no_delay(router->enable_no_delay);
			base->stream->set_keep_alive(true);
			base->stream->set_blocking(false);

			if (router->graceful_time_wait >= 0)
				base->stream->set_time_wait((int)router->graceful_time_wait);

			if (router->max_connections > 0 && active.size() >= router->max_connections)
			{
				refuse(base);
				return std::make_error_condition(std::errc::too_many_files_open);
			}

			if (!host->is_secure)
			{
				on_request_open(base);
				return core::expectation::met;
			}

			if (!handshake_then_open(base, host))
			{
				refuse(base);
				return std::make_error_condition(std::errc::protocol_error);
			}

			return core::expectation::met;
		}
		core::expects_io<void> socket_server::handshake_then_open(socket_connection* base, socket_listener* host)
		{
			VI_ASSERT(base != nullptr, "socket should be set");
			VI_ASSERT(base->stream != nullptr, "socket should be set");
			VI_ASSERT(host != nullptr, "host should be set");

			if (router->certificates.empty())
				return std::make_error_condition(std::errc::not_supported);

			auto source = router->certificates.find(host->name);
			if (source == router->certificates.end())
				return std::make_error_condition(std::errc::not_supported);

			ssl_ctx_st* context = source->second.context;
			if (!context)
				return std::make_error_condition(std::errc::host_unreachable);

			auto status = base->stream->secure(context, "");
			if (!status)
				return status;
#ifdef VI_OPENSSL
			if (SSL_set_fd(base->stream->get_device(), (int)base->stream->get_fd()) != 1)
				return std::make_error_condition(std::errc::connection_aborted);

			try_handshake_then_begin(base);
			return core::expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> socket_server::try_handshake_then_begin(socket_connection* base)
		{
#ifdef VI_OPENSSL
			int error_code = SSL_accept(base->stream->get_device());
			if (error_code != -1)
			{
				on_request_open(base);
				return core::expectation::met;
			}

			switch (SSL_get_error(base->stream->get_device(), error_code))
			{
				case SSL_ERROR_WANT_ACCEPT:
				case SSL_ERROR_WANT_READ:
				{
					multiplexer::get()->when_readable(base->stream, [this, base](socket_poll event)
					{
						if (packet::is_done(event))
							try_handshake_then_begin(base);
						else
							refuse(base);
					});
					return std::make_error_condition(std::errc::operation_would_block);
				}
				case SSL_ERROR_WANT_WRITE:
				{
					multiplexer::get()->when_writeable(base->stream, [this, base](socket_poll event)
					{
						if (packet::is_done(event))
							try_handshake_then_begin(base);
						else
							refuse(base);
					});
					return std::make_error_condition(std::errc::operation_would_block);
				}
				default:
				{
					utils::display_transport_log();
					return refuse(base);
				}
			}
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		core::expects_io<void> socket_server::next(socket_connection* base)
		{
			VI_ASSERT(base != nullptr, "socket should be set");
			if (!base->closable(router) && !on_request_cleanup(base))
				return std::make_error_condition(std::errc::operation_in_progress);

			return finalize(base);
		}
		core::expects_io<void> socket_server::finalize(socket_connection* base)
		{
			VI_ASSERT(base != nullptr, "socket should be set");
			if (router->keep_alive_max_count != 0 && base->info.reuses > 0)
				--base->info.reuses;

			base->info.finish = utils::clock();
			on_request_close(base);

			base->stream->set_io_timeout(router->socket_timeout);
			if (base->info.abort || !base->info.reuses)
				return base->stream->close_queued([this, base](const core::option<std::error_condition>&) { push(base); });
			else if (base->stream->is_valid())
				core::codefer(std::bind(&socket_server::on_request_open, this, base));
			else
				push(base);

			return core::expectation::met;
		}
		core::expects_system<void> socket_server::on_configure(socket_router*)
		{
			return core::expectation::met;
		}
		core::expects_system<void> socket_server::on_listen()
		{
			return core::expectation::met;
		}
		core::expects_system<void> socket_server::on_unlisten()
		{
			return core::expectation::met;
		}
		core::expects_system<void> socket_server::on_after_unlisten()
		{
			return core::expectation::met;
		}
		void socket_server::on_request_stall(socket_connection* base)
		{
		}
		void socket_server::on_request_open(socket_connection*)
		{
		}
		void socket_server::on_request_close(socket_connection*)
		{
		}
		bool socket_server::on_request_cleanup(socket_connection*)
		{
			return true;
		}
		bool socket_server::free_all()
		{
			VI_MEASURE(core::timings::pass);
			if (inactive.empty())
				return false;

			core::umutex<std::recursive_mutex> unique(exclusive);
			for (auto* item : inactive)
				core::memory::release(item);
			inactive.clear();
			return true;
		}
		void socket_server::push(socket_connection* base)
		{
			VI_MEASURE(core::timings::pass);
			core::umutex<std::recursive_mutex> unique(exclusive);
			auto it = active.find(base);
			if (it != active.end())
				active.erase(it);

			base->reset(true);
			if (inactive.size() < backlog)
				inactive.insert(base);
			else
				core::memory::release(base);
		}
		socket_connection* socket_server::pop(socket_listener* host)
		{
			VI_ASSERT(host != nullptr, "host should be set");
			if (state != server_state::working)
				return nullptr;

			socket_connection* result = nullptr;
			if (!inactive.empty())
			{
				core::umutex<std::recursive_mutex> unique(exclusive);
				if (!inactive.empty())
				{
					auto it = inactive.begin();
					result = *it;
					inactive.erase(it);
					active.insert(result);
				}
			}

			if (!result)
			{
				result = on_allocate(host);
				if (!result)
					return nullptr;

				result->stream = new socket();
				core::umutex<std::recursive_mutex> unique(exclusive);
				active.insert(result);
			}

			core::memory::release(result->host);
			result->info.reuses = (router->keep_alive_max_count > 0 ? (size_t)router->keep_alive_max_count : 1);
			result->host = host;
			result->host->add_ref();

			return result;
		}
		socket_connection* socket_server::on_allocate(socket_listener*)
		{
			return new socket_connection();
		}
		socket_router* socket_server::on_allocate_router()
		{
			return new socket_router();
		}
		const core::unordered_set<socket_connection*>& socket_server::get_active_clients()
		{
			return active;
		}
		const core::unordered_set<socket_connection*>& socket_server::get_pooled_clients()
		{
			return inactive;
		}
		const core::vector<socket_listener*>& socket_server::get_listeners()
		{
			return listeners;
		}
		server_state socket_server::get_state() const
		{
			return state;
		}
		socket_router* socket_server::get_router()
		{
			return router;
		}
		size_t socket_server::get_backlog() const
		{
			return backlog;
		}
		uint64_t socket_server::get_shutdown_timeout() const
		{
			return shutdown_timeout;
		}

		socket_client::socket_client(int64_t request_timeout) noexcept
		{
			timeout.idle = request_timeout;
		}
		socket_client::~socket_client() noexcept
		{
			if (!try_store_stream())
				destroy_stream();
#ifdef VI_OPENSSL
			if (net.context != nullptr)
			{
				transport_layer::get()->free_client_context(net.context);
				net.context = nullptr;
			}
#endif
			if (config.is_non_blocking && multiplexer::has_instance())
				multiplexer::get()->deactivate();
		}
		core::expects_system<void> socket_client::on_connect()
		{
			report(core::expectation::met);
			return core::expectation::met;
		}
		core::expects_system<void> socket_client::on_reuse()
		{
			report(core::expectation::met);
			return core::expectation::met;
		}
		core::expects_system<void> socket_client::on_disconnect()
		{
			report(core::expectation::met);
			return core::expectation::met;
		}
		core::expects_system<void> socket_client::connect_queued(const socket_address& address, bool as_non_blocking, int32_t verify_peers, socket_client_callback&& callback)
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			bool async_policy_updated = as_non_blocking != config.is_non_blocking;
			if (async_policy_updated && !as_non_blocking && multiplexer::has_instance())
				multiplexer::get()->deactivate();

			config.verify_peers = verify_peers;
			config.is_non_blocking = as_non_blocking;
			state.address = address;
			if (async_policy_updated && config.is_non_blocking)
				multiplexer::get()->activate();

			try_reuse_stream(address, [this, callback = std::move(callback)](bool is_reusing) mutable
			{
				if (is_reusing)
					return callback(core::expectation::met);

				auto status = net.stream->open(state.address);
				if (!status)
					return callback(core::system_exception(core::stringify::text("connect to %s error", get_address_identification(state.address).c_str()), std::move(status.error())));

				configure_stream();
				state.resolver = std::move(callback);
				net.stream->connect_queued(state.address, std::bind(&socket_client::dispatch_connection, this, std::placeholders::_1));
			});
			return core::expectation::met;
		}
		core::expects_system<void> socket_client::disconnect_queued(socket_client_callback&& callback)
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			if (!has_stream())
				return core::expects_system<void>(core::system_exception("socket: not connected", std::make_error_condition(std::errc::bad_file_descriptor)));

			state.resolver = std::move(callback);
			if (!try_store_stream())
				on_disconnect();
			else
				on_reuse();

			return core::expects_system<void>(core::expectation::met);
		}
		core::expects_promise_system<void> socket_client::connect_sync(const socket_address& address, int32_t verify_peers)
		{
			core::expects_promise_system<void> future;
			connect_queued(address, false, verify_peers, [future](core::expects_system<void>&& status) mutable { future.set(std::move(status)); });
			return future;
		}
		core::expects_promise_system<void> socket_client::connect_async(const socket_address& address, int32_t verify_peers)
		{
			core::expects_promise_system<void> future;
			connect_queued(address, true, verify_peers, [future](core::expects_system<void>&& status) mutable { future.set(std::move(status)); });
			return future;
		}
		core::expects_promise_system<void> socket_client::disconnect()
		{
			core::expects_promise_system<void> future;
			disconnect_queued([future](core::expects_system<void>&& status) mutable { future.set(std::move(status)); });
			return future;
		}
		void socket_client::handshake(std::function<void(core::expects_system<void>&&)>&& callback)
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			if (!has_stream())
				return callback(core::system_exception("ssl handshake error: bad fd", std::make_error_condition(std::errc::bad_file_descriptor)));
#ifdef VI_OPENSSL
			if (net.stream->get_device() || !net.context)
				return callback(core::system_exception("ssl handshake error: invalid operation", std::make_error_condition(std::errc::bad_file_descriptor)));

			auto hostname = state.address.get_hostname();
			auto status = net.stream->secure(net.context, hostname ? std::string_view(*hostname) : std::string_view());
			if (!status)
				return callback(core::system_exception("ssl handshake establish error", std::move(status.error())));

			if (SSL_set_fd(net.stream->get_device(), (int)net.stream->get_fd()) != 1)
				return callback(core::system_exception("ssl handshake set error", std::make_error_condition(std::errc::bad_file_descriptor)));

			if (config.verify_peers > 0)
				SSL_set_verify(net.stream->get_device(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
			else
				SSL_set_verify(net.stream->get_device(), SSL_VERIFY_NONE, nullptr);

			try_handshake(std::move(callback));
#else
			callback(core::system_exception("ssl handshake error: unsupported", std::make_error_condition(std::errc::not_supported)));
#endif
		}
		void socket_client::try_handshake(std::function<void(core::expects_system<void>&&)>&& callback)
		{
#ifdef VI_OPENSSL
			int error_code = SSL_connect(net.stream->get_device());
			if (error_code != -1)
				return callback(core::expectation::met);

			switch (SSL_get_error(net.stream->get_device(), error_code))
			{
				case SSL_ERROR_WANT_READ:
				{
					multiplexer::get()->when_readable(net.stream, [this, callback = std::move(callback)](socket_poll event) mutable
					{
						if (!packet::is_done(event))
							callback(core::system_exception(core::stringify::text("ssl connect read error: %s", ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::timed_out)));
						else
							try_handshake(std::move(callback));
					});
					break;
				}
				case SSL_ERROR_WANT_CONNECT:
				case SSL_ERROR_WANT_WRITE:
				{
					multiplexer::get()->when_writeable(net.stream, [this, callback = std::move(callback)](socket_poll event) mutable
					{
						if (!packet::is_done(event))
							callback(core::system_exception(core::stringify::text("ssl connect write error: %s", ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::timed_out)));
						else
							try_handshake(std::move(callback));
					});
					break;
				}
				default:
				{
					utils::display_transport_log();
					return callback(core::system_exception(core::stringify::text("ssl connection aborted error: %s", ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::connection_aborted)));
				}
			}
#else
			callback(core::system_exception("ssl connect error: unsupported", std::make_error_condition(std::errc::not_supported)));
#endif
		}
		void socket_client::dispatch_connection(const core::option<std::error_condition>& error_code)
		{
			if (error_code)
				return report(core::system_exception(core::stringify::text("connect to %s error", get_address_identification(state.address).c_str()), std::error_condition(*error_code)));
#ifdef VI_OPENSSL
			if (config.verify_peers < 0)
				return dispatch_simple_handshake();

			if (!net.context || (config.verify_peers > 0 && SSL_CTX_get_verify_depth(net.context) <= 0))
			{
				auto* transporter = transport_layer::get();
				if (net.context != nullptr)
				{
					transporter->free_client_context(net.context);
					net.context = nullptr;
				}

				auto new_context = transporter->create_client_context((size_t)config.verify_peers);
				if (!new_context)
					return report(core::system_exception(core::stringify::text("ssl connect to %s error: initialization failed", get_address_identification(state.address).c_str()), std::move(new_context.error())));
				else
					net.context = *new_context;
			}

			if (!config.is_auto_handshake)
				return dispatch_simple_handshake();

			auto* context = this;
			handshake([context](core::expects_system<void>&& error_code) mutable
			{
				context->dispatch_secure_handshake(std::move(error_code));
			});
#else
			dispatch_simple_handshake();
#endif
		}
		void socket_client::dispatch_secure_handshake(core::expects_system<void>&& status)
		{
			if (!status)
				report(std::move(status));
			else
				dispatch_simple_handshake();
		}
		void socket_client::dispatch_simple_handshake()
		{
			on_connect();
		}
		bool socket_client::try_reuse_stream(const socket_address& address, std::function<void(bool)>&& callback)
		{
			if (!uplinks::has_instance())
			{
				create_stream();
				callback(false);
				return false;
			}

			destroy_stream();
			return uplinks::get()->pop_connection_queued(address, [this, callback = std::move(callback)](socket*&& reusing_stream) mutable
			{
				net.stream = reusing_stream;
				if (reusing_stream != nullptr)
				{
					configure_stream();
					callback(true);
					return true;
				}

				create_stream();
				callback(false);
				return false;
			});
		}
		bool socket_client::try_store_stream()
		{
			if (!uplinks::has_instance() || !net.stream)
			{
				timeout.cache = -1;
				return false;
			}
			else if (!uplinks::get()->push_connection(state.address, timeout.cache > 0 ? net.stream : nullptr))
			{
				timeout.cache = -1;
				return false;
			}

			net.stream = nullptr;
			timeout.cache = -1;
			return true;
		}
		void socket_client::create_stream()
		{
			if (!net.stream)
				net.stream = new socket();
		}
		void socket_client::configure_stream()
		{
			if (!net.stream)
				return;

			net.stream->set_io_timeout(timeout.idle);
			net.stream->set_blocking(!config.is_non_blocking);
			net.stream->set_close_on_exec();
		}
		void socket_client::destroy_stream()
		{
			core::memory::release(net.stream);
		}
		void socket_client::apply_reusability(bool keep_alive)
		{
			timeout.cache = keep_alive ? 0 : 1;
		}
		void socket_client::enable_reusability()
		{
			if (timeout.cache < 0)
				timeout.cache = 1;
		}
		void socket_client::disable_reusability()
		{
			if (timeout.cache < 0)
				timeout.cache = 0;
		}
		void socket_client::report(core::expects_system<void>&& status)
		{
			if (!status && has_stream())
			{
				net.stream->close_queued([this, status = std::move(status)](const core::option<std::error_condition>&) mutable
				{
					socket_client_callback callback(std::move(state.resolver));
					if (callback)
						callback(std::move(status));
				});
			}
			else
			{
				socket_client_callback callback(std::move(state.resolver));
				if (callback)
					callback(std::move(status));
			}
		}
		const socket_address& socket_client::get_peer_address() const
		{
			return state.address;
		}
		bool socket_client::has_stream() const
		{
			return net.stream != nullptr && net.stream->is_valid();
		}
		bool socket_client::is_secure() const
		{
			return config.verify_peers >= 0;
		}
		bool socket_client::is_verified() const
		{
			return config.verify_peers > 0;
		}
		socket* socket_client::get_stream()
		{
			return net.stream;
		}
	}
}
#pragma warning(pop)
