#include "http.h"
#include "../bindings.h"
#ifdef VI_MICROSOFT
#include <ws2tcpip.h>
#include <io.h>
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
#include <fcntl.h>
#include <poll.h>
#endif
#include <random>
#include <string>
extern "C"
{
#ifdef VI_ZLIB
#include <zlib.h>
#endif
#ifdef VI_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <openssl/dh.h>
#endif
}
#define CSS_DIRECTORY_STYLE "html{background-color:#101010;color:#fff;}th{text-align:left;}a:link{color:#5D80FF;text-decoration:none;}a:visited{color:#F193FF;}a:hover{opacity:0.9; cursor:pointer}a:active{opacity:0.8;cursor:default;}"
#define CSS_MESSAGE_STYLE "html{font-family:\"Helvetica neue\",helvetica,arial,sans-serif;height:95%%;background-color:#101010;color:#fff;}body{display:flex;align-items:center;justify-content:center;height:100%%;}"
#define CSS_NORMAL_FONT "div{text-align:center;}"
#define CSS_SMALL_FONT "h1{font-size:16px;font-weight:normal;}"
#define HTTP_WEBSOCKET_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define HTTP_WEBSOCKET_LEGACY_KEY_SIZE 8
#define HTTP_MAX_REDIRECTS 128
#define HTTP_HRM_SIZE 1024 * 1024 * 4
#define HTTP_KIMV_LOAD_FACTOR 48
#define GZ_HEADER_SIZE 17
#pragma warning(push)
#pragma warning(disable: 4996)

namespace vitex
{
	namespace network
	{
		namespace http
		{
			static bool path_trailing_check(const std::string_view& path)
			{
#ifdef VI_MICROSOFT
				return !path.empty() && (path.back() == '/' || path.back() == '\\');
#else
				return !path.empty() && path.back() == '/';
#endif
			}
			static bool connection_valid(connection* target)
			{
				return target && target->root && target->route && target->route->router;
			}
			static void text_append(core::vector<char>& array, const std::string_view& src)
			{
				array.insert(array.end(), src.begin(), src.end());
			}
			static void text_assign(core::vector<char>& array, const std::string_view& src)
			{
				array.assign(src.begin(), src.end());
			}
			static core::string text_substring(core::vector<char>& array, size_t offset, size_t size)
			{
				return core::string(array.data() + offset, size);
			}
			static std::string_view header_text(const kimv_unordered_map& headers, const std::string_view& key)
			{
				VI_ASSERT(!key.empty(), "key should not be empty");
				auto it = headers.find(core::key_lookup_cast(key));
				if (it == headers.end())
					return "";

				if (it->second.empty())
					return "";

				return it->second.back();
			}
			static std::string_view header_date(char buffer[64], time_t time)
			{
				static const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
				static const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

				struct tm date { };
				if (!core::date_time::make_global_time(time, &date))
				{
					*buffer = '\0';
					return std::string_view(buffer, 0);
				}

				char numeric[core::NUMSTR_SIZE]; size_t size = 0;
				auto append_text = [&buffer, &size](const char* text, size_t text_size) { memcpy(buffer + size, text, text_size); size += text_size; };
				auto append_view = [&buffer, &size](const std::string_view& text) { memcpy(buffer + size, text.data(), text.size()); size += text.size(); };
				auto append_char = [&buffer, &size](char text) { memcpy(buffer + size++, &text, 1); };
				append_text(days[date.tm_wday], 3);
				append_text(", ", 2);
				if (date.tm_mday < 10)
				{
					append_char('0');
					append_char('0' + date.tm_mday);
				}
				else
					append_view(core::to_string_view(numeric, sizeof(numeric), (uint8_t)date.tm_mday));
				append_char(' ');
				append_text(months[date.tm_mon], 3);
				append_char(' ');
				append_view(core::to_string_view(numeric, sizeof(numeric), (uint32_t)(date.tm_year + 1900)));
				append_char(' ');
				if (date.tm_hour < 10)
				{
					append_char('0');
					append_char('0' + date.tm_hour);
				}
				else
					append_view(core::to_string_view(numeric, sizeof(numeric), (uint8_t)date.tm_hour));
				append_char(':');
				if (date.tm_min < 10)
				{
					append_char('0');
					append_char('0' + date.tm_min);
				}
				else
					append_view(core::to_string_view(numeric, sizeof(numeric), (uint8_t)date.tm_min));
				append_char(':');
				if (date.tm_sec < 10)
				{
					append_char('0');
					append_char('0' + date.tm_sec);
				}
				else
					append_view(core::to_string_view(numeric, sizeof(numeric), (uint8_t)date.tm_sec));
				append_text(" GMT\0", 5);
				return std::string_view(buffer, size - 1);
			}
			static void cleanup_hash_map(kimv_unordered_map& map)
			{
				if (map.size() <= HTTP_KIMV_LOAD_FACTOR)
				{
					for (auto& item : map)
						item.second.clear();
				}
				else
					map.clear();
			}

			mime_static::mime_static(const std::string_view& ext, const std::string_view& t) : extension(ext), type(t)
			{
			}

			void cookie::set_expires(int64_t time)
			{
				char date[64];
				expires = header_date(date, time);
			}
			void cookie::set_expired()
			{
				set_expires(0);
			}

			web_socket_frame::web_socket_frame(socket* new_stream, void* new_user_data) : stream(new_stream), codec(new web_codec()), state((uint32_t)web_socket_state::open), tunneling((uint32_t)tunnel::healthy), active(true), deadly(false), busy(false), user_data(new_user_data)
			{
			}
			web_socket_frame::~web_socket_frame() noexcept
			{
				while (!messages.empty())
				{
					auto& next = messages.front();
					core::memory::deallocate(next.buffer);
					messages.pop();
				}

				core::memory::release(codec);
				if (lifetime.destroy)
					lifetime.destroy(this);
			}
			core::expects_system<size_t> web_socket_frame::send(const std::string_view& buffer, web_socket_op opcode, web_socket_callback&& callback)
			{
				return send(0, buffer, opcode, std::move(callback));
			}
			core::expects_system<size_t> web_socket_frame::send(uint32_t mask, const std::string_view& buffer, web_socket_op opcode, web_socket_callback&& callback)
			{
				core::umutex<std::mutex> unique(section);
				if (enqueue(mask, buffer, opcode, std::move(callback)))
					return (size_t)0;

				busy = true;
				unique.negate();

				uint8_t header[14];
				size_t header_length = 1;
				header[0] = 0x80 + ((size_t)opcode & 0xF);

				if (buffer.size() < 126)
				{
					header[1] = (uint8_t)buffer.size();
					header_length = 2;
				}
				else if (buffer.size() <= 65535)
				{
					uint16_t length = htons((uint16_t)buffer.size());
					header[1] = 126;
					header_length = 4;
					memcpy(header + 2, &length, 2);
				}
				else
				{
					uint32_t length1 = htonl((uint64_t)buffer.size() >> 32);
					uint32_t length2 = htonl((uint64_t)buffer.size() & 0xFFFFFFFF);
					header[1] = 127;
					header_length = 10;
					memcpy(header + 2, &length1, 4);
					memcpy(header + 6, &length2, 4);
				}

				if (mask)
				{
					header[1] |= 0x80;
					memcpy(header + header_length, &mask, 4);
					header_length += 4;
				}

				core::string copy = core::string(buffer);
				auto status = stream->write_queued(header, header_length, [this, copy = std::move(copy), callback = std::move(callback)](socket_poll event) mutable
				{
					if (packet::is_done(event))
					{
						if (!copy.empty())
						{
							stream->write_queued((uint8_t*)copy.data(), copy.size(), [this, callback = std::move(callback)](socket_poll event)
							{
								if (packet::is_done(event) || packet::is_skip(event))
								{
									bool ignore = is_ignore();
									busy = false;

									if (callback)
										callback(this);

									if (!ignore)
										dequeue();
								}
								else if (packet::is_error(event))
								{
									tunneling = (uint32_t)tunnel::gone;
									busy = false;
									if (callback)
										callback(this);
								}
							});
						}
						else
						{
							bool ignore = is_ignore();
							busy = false;

							if (callback)
								callback(this);

							if (!ignore)
								dequeue();
						}
					}
					else if (packet::is_error(event))
					{
						tunneling = (uint32_t)tunnel::gone;
						busy = false;
						if (callback)
							callback(this);
					}
					else if (packet::is_skip(event))
					{
						bool ignore = is_ignore();
						busy = false;

						if (callback)
							callback(this);

						if (!ignore)
							dequeue();
					}
				});
				if (status)
					return *status;
				else if (status.error() == std::errc::operation_would_block)
					return (size_t)0;

				return core::system_exception("ws send error", std::move(status.error()));
			}
			core::expects_system<void> web_socket_frame::send_close(web_socket_callback&& callback)
			{
				if (deadly)
				{
					if (callback)
						callback(this);
					return core::system_exception("ws connection is closed: fd " + core::to_string(stream->get_fd()), std::make_error_condition(std::errc::operation_not_permitted));
				}

				if (state == (uint32_t)web_socket_state::close || tunneling != (uint32_t)tunnel::healthy)
				{
					if (callback)
						callback(this);
					return core::expectation::met;
				}

				if (!active)
				{
					if (callback)
						callback(this);
					return core::system_exception("ws connection is closing: fd " + core::to_string(stream->get_fd()), std::make_error_condition(std::errc::operation_not_permitted));
				}

				finalize();
				auto status = send("", web_socket_op::close, std::move(callback));
				if (!status)
					return status.error();

				return core::expectation::met;
			}
			void web_socket_frame::dequeue()
			{
				core::umutex<std::mutex> unique(section);
				if (!is_writeable() || messages.empty())
					return;

				message next = std::move(messages.front());
				messages.pop();
				unique.negate();

				send(next.mask, std::string_view(next.buffer, next.size), next.opcode, std::move(next.callback));
				core::memory::deallocate(next.buffer);
			}
			void web_socket_frame::finalize()
			{
				if (tunneling == (uint32_t)tunnel::healthy)
					state = (uint32_t)web_socket_state::close;
			}
			void web_socket_frame::next()
			{
				core::codefer(std::bind(&web_socket_frame::update, this));
			}
			void web_socket_frame::update()
			{
				core::umutex<std::mutex> unique(section);
			retry:
				if (state == (uint32_t)web_socket_state::close || tunneling != (uint32_t)tunnel::healthy)
				{
					if (tunneling != (uint32_t)tunnel::gone)
						tunneling = (uint32_t)tunnel::closing;

					if (before_disconnect)
					{
						web_socket_callback callback = std::move(before_disconnect);
						before_disconnect = nullptr;
						unique.negate();
						callback(this);
					}
					else if (!disconnect)
					{
						bool successful = (tunneling == (uint32_t)tunnel::closing);
						tunneling = (uint32_t)tunnel::gone;
						active = false;
						unique.negate();
						if (lifetime.close)
							lifetime.close(this, successful);
					}
					else
					{
						web_socket_callback callback = std::move(disconnect);
						disconnect = nullptr;
						receive = nullptr;
						unique.negate();
						callback(this);
					}
				}
				else if (state == (uint32_t)web_socket_state::receive)
				{
					if (lifetime.dead && lifetime.dead(this))
					{
						finalize();
						goto retry;
					}

					multiplexer::get()->when_readable(stream, [this](socket_poll event)
					{
						bool is_done = packet::is_done(event);
						if (!is_done && !packet::is_error(event))
							return true;

						state = (uint32_t)(is_done ? web_socket_state::process : web_socket_state::close);
						next();
						return true;
					});
				}
				else if (state == (uint32_t)web_socket_state::process)
				{
					uint8_t buffer[core::BLOB_SIZE];
					while (true)
					{
						auto size = stream->read(buffer, sizeof(buffer));
						if (size)
						{
							codec->parse_frame(buffer, *size);
							continue;
						}

						if (size.error() == std::errc::operation_would_block)
						{
							state = (uint32_t)web_socket_state::receive;
							break;
						}
						else
						{
							finalize();
							goto retry;
						}
					}

					web_socket_op opcode;
					if (!codec->get_frame(&opcode, &codec->data))
						goto retry;

					state = (uint32_t)web_socket_state::process;
					if (opcode == web_socket_op::text || opcode == web_socket_op::binary)
					{
						VI_DEBUG("websocket sock %i frame data: %.*s", (int)stream->get_fd(), (int)codec->data.size(), codec->data.data());
						if (receive)
						{
							unique.negate();
							if (!receive(this, opcode, std::string_view(codec->data.data(), codec->data.size())))
								next();
						}
					}
					else if (opcode == web_socket_op::ping)
					{
						VI_DEBUG("websocket sock %i frame ping", (int)stream->get_fd());
						unique.negate();
						if (!receive || !receive(this, opcode, ""))
							send("", web_socket_op::pong, [this](web_socket_frame*) { next(); });
					}
					else if (opcode == web_socket_op::close)
					{
						VI_DEBUG("websocket sock %i frame close", (int)stream->get_fd());
						unique.negate();
						if (!receive || !receive(this, opcode, ""))
							send_close(std::bind(&web_socket_frame::next, std::placeholders::_1));
					}
					else if (receive)
					{
						unique.negate();
						if (!receive(this, opcode, ""))
							next();
					}
					else
						goto retry;
				}
				else if (state == (uint32_t)web_socket_state::open)
				{
					if (connect || receive || disconnect)
					{
						state = (uint32_t)web_socket_state::receive;
						if (!connect)
							goto retry;

						web_socket_callback callback = std::move(connect);
						connect = nullptr;
						unique.negate();
						callback(this);
					}
					else
					{
						unique.negate();
						send_close(std::bind(&web_socket_frame::next, std::placeholders::_1));
					}
				}
			}
			bool web_socket_frame::is_finished()
			{
				return !active;
			}
			bool web_socket_frame::is_ignore()
			{
				return deadly || state == (uint32_t)web_socket_state::close || tunneling != (uint32_t)tunnel::healthy;
			}
			bool web_socket_frame::is_writeable()
			{
				return !busy && !stream->is_awaiting_writeable();
			}
			socket* web_socket_frame::get_stream()
			{
				return stream;
			}
			connection* web_socket_frame::get_connection()
			{
				return (connection*)user_data;
			}
			client* web_socket_frame::get_client()
			{
				return (client*)user_data;
			}
			bool web_socket_frame::enqueue(uint32_t mask, const std::string_view& buffer, web_socket_op opcode, web_socket_callback&& callback)
			{
				if (is_writeable())
					return false;

				message next;
				next.mask = mask;
				next.buffer = (buffer.empty() ? nullptr : core::memory::allocate<char>(sizeof(char) * buffer.size()));
				next.size = buffer.size();
				next.opcode = opcode;
				next.callback = callback;
				if (next.buffer != nullptr)
					memcpy(next.buffer, buffer.data(), sizeof(char) * buffer.size());

				messages.emplace(std::move(next));
				return true;
			}

			router_group::router_group(const std::string_view& new_match, route_mode new_mode) noexcept : match(new_match), mode(new_mode)
			{
			}
			router_group::~router_group() noexcept
			{
				for (auto* entry : routes)
					core::memory::release(entry);
				routes.clear();
			}

			router_entry* router_entry::from(const router_entry& other, const compute::regex_source& source)
			{
				router_entry* route = new router_entry(other);
				route->location = source;
				return route;
			}

			map_router::map_router() : base(new router_entry())
			{
				base->location = compute::regex_source("/");
				base->router = this;
			}
			map_router::~map_router()
			{
				if (callbacks.on_destroy != nullptr)
					callbacks.on_destroy(this);

				for (auto& item : groups)
					core::memory::release(item);

				groups.clear();
				core::memory::release(base);
			}
			void map_router::sort()
			{
				VI_SORT(groups.begin(), groups.end(), [](const router_group* a, const router_group* b)
				{
					return a->match.size() > b->match.size();
				});

				for (auto& group : groups)
				{
					static auto comparator = [](const router_entry* a, const router_entry* b)
					{
						if (a->location.get_regex().empty())
							return false;

						if (b->location.get_regex().empty())
							return true;

						if (a->level > b->level)
							return true;
						else if (a->level < b->level)
							return false;

						bool fA = a->location.is_simple(), fB = b->location.is_simple();
						if (fA && !fB)
							return false;
						else if (!fA && fB)
							return true;

						return a->location.get_regex().size() > b->location.get_regex().size();
					};
					VI_SORT(group->routes.begin(), group->routes.end(), comparator);
				}
			}
			router_group* map_router::group(const std::string_view& match, route_mode mode)
			{
				for (auto& group : groups)
				{
					if (group->match == match && group->mode == mode)
						return group;
				}

				auto* result = new router_group(match, mode);
				groups.emplace_back(result);

				return result;
			}
			router_entry* map_router::route(const std::string_view& match, route_mode mode, const std::string_view& pattern, bool inherit_props)
			{
				if (pattern.empty() || pattern == "/")
					return base;

				http::router_group* source = nullptr;
				for (auto& group : groups)
				{
					if (group->match != match || group->mode != mode)
						continue;

					source = group;
					for (auto* entry : group->routes)
					{
						if (entry->location.get_regex() == pattern)
							return entry;
					}
				}

				if (!source)
				{
					auto* result = new router_group(match, mode);
					groups.emplace_back(result);
					source = groups.back();
				}

				if (!inherit_props)
					return route(pattern, source, nullptr);

				http::router_entry* from = base;
				compute::regex_result result;
				core::string src(pattern);
				core::stringify::to_lower(src);

				for (auto& group : groups)
				{
					for (auto* entry : group->routes)
					{
						core::string dest(entry->location.get_regex());
						core::stringify::to_lower(dest);

						if (core::stringify::starts_with(dest, "...") && core::stringify::ends_with(dest, "..."))
							continue;

						if (core::stringify::find(src, dest).found || compute::regex::match(&entry->location, result, src))
						{
							from = entry;
							break;
						}
					}
				}

				return route(pattern, source, from);
			}
			router_entry* map_router::route(const std::string_view& pattern, router_group* group, router_entry* from)
			{
				VI_ASSERT(group != nullptr, "group should be set");
				if (from != nullptr)
				{
					http::router_entry* result = http::router_entry::from(*from, compute::regex_source(pattern));
					group->routes.push_back(result);
					return result;
				}

				http::router_entry* result = new http::router_entry();
				result->location = compute::regex_source(pattern);
				result->router = this;
				group->routes.push_back(result);
				return result;
			}
			bool map_router::remove(router_entry* source)
			{
				VI_ASSERT(source != nullptr, "source should be set");
				for (auto& group : groups)
				{
					auto it = std::find(group->routes.begin(), group->routes.end(), source);
					if (it != group->routes.end())
					{
						core::memory::release(*it);
						group->routes.erase(it);
						return true;
					}
				}

				return false;
			}
			bool map_router::get(const std::string_view& pattern, success_callback&& callback)
			{
				return get("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::get(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.get = std::move(callback);
				return true;
			}
			bool map_router::post(const std::string_view& pattern, success_callback&& callback)
			{
				return post("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::post(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.post = std::move(callback);
				return true;
			}
			bool map_router::put(const std::string_view& pattern, success_callback&& callback)
			{
				return put("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::put(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.put = std::move(callback);
				return true;
			}
			bool map_router::patch(const std::string_view& pattern, success_callback&& callback)
			{
				return patch("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::patch(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.patch = std::move(callback);
				return true;
			}
			bool map_router::deinit(const std::string_view& pattern, success_callback&& callback)
			{
				return deinit("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::deinit(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.deinit = std::move(callback);
				return true;
			}
			bool map_router::options(const std::string_view& pattern, success_callback&& callback)
			{
				return options("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::options(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.options = std::move(callback);
				return true;
			}
			bool map_router::access(const std::string_view& pattern, success_callback&& callback)
			{
				return access("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::access(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.access = std::move(callback);
				return true;
			}
			bool map_router::headers(const std::string_view& pattern, header_callback&& callback)
			{
				return headers("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::headers(const std::string_view& match, route_mode mode, const std::string_view& pattern, header_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.headers = std::move(callback);
				return true;
			}
			bool map_router::authorize(const std::string_view& pattern, authorize_callback&& callback)
			{
				return authorize("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::authorize(const std::string_view& match, route_mode mode, const std::string_view& pattern, authorize_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.authorize = std::move(callback);
				return true;
			}
			bool map_router::web_socket_initiate(const std::string_view& pattern, success_callback&& callback)
			{
				return web_socket_initiate("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::web_socket_initiate(const std::string_view& match, route_mode mode, const std::string_view& pattern, success_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.web_socket.initiate = std::move(callback);
				return true;
			}
			bool map_router::web_socket_connect(const std::string_view& pattern, web_socket_callback&& callback)
			{
				return web_socket_connect("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::web_socket_connect(const std::string_view& match, route_mode mode, const std::string_view& pattern, web_socket_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.web_socket.connect = std::move(callback);
				return true;
			}
			bool map_router::web_socket_disconnect(const std::string_view& pattern, web_socket_callback&& callback)
			{
				return web_socket_disconnect("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::web_socket_disconnect(const std::string_view& match, route_mode mode, const std::string_view& pattern, web_socket_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.web_socket.disconnect = std::move(callback);
				return true;
			}
			bool map_router::web_socket_receive(const std::string_view& pattern, web_socket_read_callback&& callback)
			{
				return web_socket_receive("", route_mode::start, pattern, std::move(callback));
			}
			bool map_router::web_socket_receive(const std::string_view& match, route_mode mode, const std::string_view& pattern, web_socket_read_callback&& callback)
			{
				http::router_entry* value = route(match, mode, pattern, true);
				if (!value)
					return false;

				value->callbacks.web_socket.receive = std::move(callback);
				return true;
			}

			core::string& resource::put_header(const std::string_view& label, const std::string_view& value)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				core::vector<core::string>* range;
				auto it = headers.find(core::key_lookup_cast(label));
				if (it != headers.end())
					range = &it->second;
				else
					range = &headers[core::string(label)];

				range->push_back(core::string(value));
				return range->back();
			}
			core::string& resource::set_header(const std::string_view& label, const std::string_view& value)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				core::vector<core::string>* range;
				auto it = headers.find(core::key_lookup_cast(label));
				if (it != headers.end())
					range = &it->second;
				else
					range = &headers[core::string(label)];

				range->clear();
				range->push_back(core::string(value));
				return range->back();
			}
			core::string resource::compose_header(const std::string_view& label) const
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end())
					return core::string();

				core::string result;
				for (auto& item : it->second)
				{
					result.append(item);
					result.append(1, ',');
				}

				return (result.empty() ? result : result.substr(0, result.size() - 1));
			}
			core::vector<core::string>* resource::get_header_ranges(const std::string_view& label)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				return it != headers.end() ? &it->second : &headers[core::string(label)];
			}
			core::string* resource::get_header_blob(const std::string_view& label)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end() || it->second.empty())
					return nullptr;

				auto& result = it->second.back();
				return &result;
			}
			std::string_view resource::get_header(const std::string_view& label) const
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end() || it->second.empty())
					return "";

				return it->second.back();
			}
			const core::string& resource::get_in_memory_contents() const
			{
				return path;
			}

			request_frame::request_frame()
			{
				memset(method, 0, sizeof(method));
				memset(version, 0, sizeof(version));
				strcpy(method, "GET");
				strcpy(version, "HTTP/1.1");
			}
			void request_frame::set_method(const std::string_view& value)
			{
				memset(method, 0, sizeof(method));
				memcpy((void*)method, (void*)value.data(), std::min<size_t>(value.size(), sizeof(method)));
			}
			void request_frame::set_version(uint32_t major, uint32_t minor)
			{
				core::string value = "HTTP/" + core::to_string(major) + '.' + core::to_string(minor);
				memset(version, 0, sizeof(version));
				memcpy((void*)version, (void*)value.c_str(), std::min<size_t>(value.size(), sizeof(version)));
			}
			void request_frame::cleanup()
			{
				memset(method, 0, sizeof(method));
				memset(version, 0, sizeof(version));
				cleanup_hash_map(headers);
				cleanup_hash_map(cookies);
				user.type = auth::unverified;
				user.token.clear();
				content.cleanup();
				query.clear();
				path.clear();
				location.clear();
				referrer.clear();
			}
			core::string& request_frame::put_header(const std::string_view& label, const std::string_view& value)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				core::vector<core::string>* range;
				auto it = headers.find(core::key_lookup_cast(label));
				if (it != headers.end())
					range = &it->second;
				else
					range = &headers[core::string(label)];

				range->push_back(core::string(value));
				return range->back();
			}
			core::string& request_frame::set_header(const std::string_view& label, const std::string_view& value)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				core::vector<core::string>* range;
				auto it = headers.find(core::key_lookup_cast(label));
				if (it != headers.end())
					range = &it->second;
				else
					range = &headers[core::string(label)];

				range->clear();
				range->push_back(core::string(value));
				return range->back();
			}
			core::string request_frame::compose_header(const std::string_view& label) const
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end())
					return core::string();

				core::string result;
				for (auto& item : it->second)
				{
					result.append(item);
					result.append(1, ',');
				}

				return (result.empty() ? result : result.substr(0, result.size() - 1));
			}
			core::vector<core::string>* request_frame::get_header_ranges(const std::string_view& label)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				return it != headers.end() ? &it->second : &headers[core::string(label)];
			}
			core::string* request_frame::get_header_blob(const std::string_view& label)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end() || it->second.empty())
					return nullptr;

				auto& result = it->second.back();
				return &result;
			}
			std::string_view request_frame::get_header(const std::string_view& label) const
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end() || it->second.empty())
					return "";

				return it->second.back();
			}
			core::vector<core::string>* request_frame::get_cookie_ranges(const std::string_view& key)
			{
				VI_ASSERT(!key.empty(), "key should not be empty");
				auto it = cookies.find(core::key_lookup_cast(key));
				return it != cookies.end() ? &it->second : &cookies[core::string(key)];
			}
			core::string* request_frame::get_cookie_blob(const std::string_view& key)
			{
				VI_ASSERT(!key.empty(), "key should not be empty");
				auto it = cookies.find(core::key_lookup_cast(key));
				if (it == cookies.end() || it->second.empty())
					return nullptr;

				auto& result = it->second.back();
				return &result;
			}
			std::string_view request_frame::get_cookie(const std::string_view& key) const
			{
				VI_ASSERT(!key.empty(), "key should not be empty");
				auto it = cookies.find(core::key_lookup_cast(key));
				if (it == cookies.end() || it->second.empty())
					return "";

				return it->second.back();
			}
			core::vector<std::pair<size_t, size_t>> request_frame::get_ranges() const
			{
				auto it = headers.find("Range");
				if (it == headers.end())
					return core::vector<std::pair<size_t, size_t>>();

				core::vector<std::pair<size_t, size_t>> ranges;
				for (auto& item : it->second)
				{
					core::text_settle result = core::stringify::find(item, '-');
					if (!result.found)
						continue;

					size_t content_start = -1, content_end = -1;
					if (result.start > 0)
					{
						const char* left = item.c_str() + result.start - 1;
						size_t left_size = (size_t)(isdigit(*left) > 0);
						if (left_size > 0)
						{
							while (isdigit(*(left - 1)) && left_size <= result.start - 1)
							{
								--left;
								++left_size;
							}

							if (left_size > 0)
							{
								auto from = core::from_string<size_t>(core::string(left, left_size));
								if (from)
									content_start = *from;
							}
						}
					}

					if (result.end < item.size())
					{
						size_t right_size = 0;
						const char* right = item.c_str() + result.start + 1;
						while (right[right_size] != '\0' && isdigit(right[right_size]))
							++right_size;

						if (right_size > 0)
						{
							auto to = core::from_string<size_t>(core::string(right, right_size));
							if (to)
								content_end = *to;
						}
					}

					if (content_start != -1 || content_end != -1)
						ranges.emplace_back(std::make_pair(content_start, content_end));
				}

				return ranges;
			}
			std::pair<size_t, size_t> request_frame::get_range(core::vector<std::pair<size_t, size_t>>::iterator range, size_t content_length) const
			{
				if (range->first == -1 && range->second == -1)
					return std::make_pair(0, content_length);

				if (range->first == -1)
				{
					if (range->second > content_length)
						range->second = 0;

					range->first = content_length - range->second;
					range->second = content_length;
				}
				else if (range->first > content_length)
					range->first = content_length;

				if (range->second == -1)
					range->second = content_length;
				else if (range->second > content_length)
					range->second = content_length;

				if (range->first > range->second)
					range->first = range->second;

				return std::make_pair(range->first, range->second - range->first);
			}

			response_frame::response_frame() : status_code(-1), error(false)
			{
			}
			void response_frame::set_cookie(const cookie& value)
			{
				for (auto& cookie : cookies)
				{
					if (core::stringify::case_equals(cookie.name.c_str(), value.name.c_str()))
					{
						cookie = value;
						return;
					}
				}

				cookies.push_back(value);
			}
			void response_frame::set_cookie(cookie&& value)
			{
				for (auto& cookie : cookies)
				{
					if (core::stringify::case_equals(cookie.name.c_str(), value.name.c_str()))
					{
						cookie = std::move(value);
						return;
					}
				}

				cookies.emplace_back(std::move(value));
			}
			void response_frame::cleanup()
			{
				cleanup_hash_map(headers);
				status_code = -1;
				error = false;
				cookies.clear();
				content.cleanup();
			}
			core::string& response_frame::put_header(const std::string_view& label, const std::string_view& value)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				core::vector<core::string>* range;
				auto it = headers.find(core::key_lookup_cast(label));
				if (it != headers.end())
					range = &it->second;
				else
					range = &headers[core::string(label)];

				range->push_back(core::string(value));
				return range->back();
			}
			core::string& response_frame::set_header(const std::string_view& label, const std::string_view& value)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				core::vector<core::string>* range;
				auto it = headers.find(core::key_lookup_cast(label));
				if (it != headers.end())
					range = &it->second;
				else
					range = &headers[core::string(label)];

				range->clear();
				range->push_back(core::string(value));
				return range->back();
			}
			core::string response_frame::compose_header(const std::string_view& label) const
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end())
					return core::string();

				core::string result;
				for (auto& item : it->second)
				{
					result.append(item);
					result.append(1, ',');
				}

				return (result.empty() ? result : result.substr(0, result.size() - 1));
			}
			core::vector<core::string>* response_frame::get_header_ranges(const std::string_view& label)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				return it != headers.end() ? &it->second : &headers[core::string(label)];
			}
			core::string* response_frame::get_header_blob(const std::string_view& label)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end() || it->second.empty())
					return nullptr;

				auto& result = it->second.back();
				return &result;
			}
			std::string_view response_frame::get_header(const std::string_view& label) const
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end() || it->second.empty())
					return "";

				return it->second.back();
			}
			cookie* response_frame::get_cookie(const std::string_view& key)
			{
				for (size_t i = 0; i < cookies.size(); i++)
				{
					cookie* result = &cookies[i];
					if (core::stringify::case_equals(result->name.c_str(), key))
						return result;
				}

				return nullptr;
			}
			bool response_frame::is_undefined() const
			{
				return status_code <= 0;
			}
			bool response_frame::is_ok() const
			{
				return status_code >= 200 && status_code < 400;
			}

			content_frame::content_frame() : length(0), offset(0), prefetch(0), exceeds(false), limited(false)
			{
			}
			void content_frame::append(const std::string_view& text)
			{
				text_append(data, text);
			}
			void content_frame::assign(const std::string_view& text)
			{
				text_assign(data, text);
			}
			void content_frame::prepare(const kimv_unordered_map& headers, const uint8_t* buffer, size_t size)
			{
				auto content_length = header_text(headers, "Content-Length");
				limited = !content_length.empty();
				if (limited)
					length = strtoull(content_length.data(), nullptr, 10);

				offset = prefetch = buffer ? size : 0;
				if (offset > 0)
					text_assign(data, std::string_view((char*)buffer, size));
				else
					data.clear();

				if (limited)
					return;

				auto transfer_encoding = header_text(headers, "Transfer-Encoding");
				if (!core::stringify::case_equals(transfer_encoding, "chunked"))
					limited = true;
			}
			void content_frame::finalize()
			{
				length = offset;
				limited = true;
			}
			void content_frame::cleanup()
			{
				if (!resources.empty())
				{
					core::vector<core::string> paths;
					paths.reserve(resources.size());
					for (auto& item : resources)
					{
						if (!item.is_in_memory)
							paths.push_back(item.path);
					}

					if (!paths.empty())
					{
						core::cospawn([paths = std::move(paths)]() mutable
						{
							for (auto& path : paths)
								core::os::file::remove(path.c_str());
						});
					}
				}

				data.clear();
				resources.clear();
				length = 0;
				offset = 0;
				prefetch = 0;
				limited = false;
				exceeds = false;
			}
			core::expects_parser<core::schema*> content_frame::get_json() const
			{
				return core::schema::from_json(get_text());
			}
			core::expects_parser<core::schema*> content_frame::get_xml() const
			{
				return core::schema::from_xml(get_text());
			}
			core::string content_frame::get_text() const
			{
				return core::string(data.data(), data.size());
			}
			bool content_frame::is_finalized() const
			{
				if (prefetch > 0)
					return false;

				if (!limited)
					return offset >= length && length > 0;

				return offset >= length || data.size() >= length;
			}

			fetch_frame::fetch_frame() : timeout(10000), max_size(PAYLOAD_SIZE), verify_peers(9)
			{
			}
			void fetch_frame::cleanup()
			{
				cleanup_hash_map(headers);
				cleanup_hash_map(cookies);
				content.cleanup();
			}
			core::string& fetch_frame::put_header(const std::string_view& label, const std::string_view& value)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				core::vector<core::string>* range;
				auto it = headers.find(core::key_lookup_cast(label));
				if (it != headers.end())
					range = &it->second;
				else
					range = &headers[core::string(label)];

				range->push_back(core::string(value));
				return range->back();
			}
			core::string& fetch_frame::set_header(const std::string_view& label, const std::string_view& value)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				core::vector<core::string>* range;
				auto it = headers.find(core::key_lookup_cast(label));
				if (it != headers.end())
					range = &it->second;
				else
					range = &headers[core::string(label)];

				range->clear();
				range->push_back(core::string(value));
				return range->back();
			}
			core::string fetch_frame::compose_header(const std::string_view& label) const
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end())
					return core::string();

				core::string result;
				for (auto& item : it->second)
				{
					result.append(item);
					result.append(1, ',');
				}

				return (result.empty() ? result : result.substr(0, result.size() - 1));
			}
			core::vector<core::string>* fetch_frame::get_header_ranges(const std::string_view& label)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				return it != headers.end() ? &it->second : &headers[core::string(label)];
			}
			core::string* fetch_frame::get_header_blob(const std::string_view& label)
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end() || it->second.empty())
					return nullptr;

				auto& result = it->second.back();
				return &result;
			}
			std::string_view fetch_frame::get_header(const std::string_view& label) const
			{
				VI_ASSERT(!label.empty(), "label should not be empty");
				auto it = headers.find(core::key_lookup_cast(label));
				if (it == headers.end() || it->second.empty())
					return "";

				return it->second.back();
			}
			core::vector<core::string>* fetch_frame::get_cookie_ranges(const std::string_view& key)
			{
				VI_ASSERT(!key.empty(), "key should not be empty");
				auto it = cookies.find(core::key_lookup_cast(key));
				return it != cookies.end() ? &it->second : &cookies[core::string(key)];
			}
			core::string* fetch_frame::get_cookie_blob(const std::string_view& key)
			{
				VI_ASSERT(!key.empty(), "key should not be empty");
				auto it = cookies.find(core::key_lookup_cast(key));
				if (it == cookies.end() || it->second.empty())
					return nullptr;

				auto& result = it->second.back();
				return &result;
			}
			std::string_view fetch_frame::get_cookie(const std::string_view& key) const
			{
				VI_ASSERT(!key.empty(), "key should not be empty");
				auto it = cookies.find(core::key_lookup_cast(key));
				if (it == cookies.end())
					return "";

				if (it->second.empty())
					return "";

				return it->second.back();
			}
			core::vector<std::pair<size_t, size_t>> fetch_frame::get_ranges() const
			{
				auto it = headers.find("Range");
				if (it == headers.end())
					return core::vector<std::pair<size_t, size_t>>();

				core::vector<std::pair<size_t, size_t>> ranges;
				for (auto& item : it->second)
				{
					core::text_settle result = core::stringify::find(item, '-');
					if (!result.found)
						continue;

					size_t content_start = -1, content_end = -1;
					if (result.start > 0)
					{
						const char* left = item.c_str() + result.start - 1;
						size_t left_size = (size_t)(isdigit(*left) > 0);
						if (left_size > 0)
						{
							while (isdigit(*(left - 1)) && left_size <= result.start - 1)
							{
								--left;
								++left_size;
							}

							if (left_size > 0)
							{
								auto from = core::from_string<size_t>(core::string(left, left_size));
								if (from)
									content_start = *from;
							}
						}
					}

					if (result.end < item.size())
					{
						size_t right_size = 0;
						const char* right = item.c_str() + result.start + 1;
						while (right[right_size] != '\0' && isdigit(right[right_size]))
							++right_size;

						if (right_size > 0)
						{
							auto to = core::from_string<size_t>(core::string(right, right_size));
							if (to)
								content_end = *to;
						}
					}

					if (content_start != -1 || content_end != -1)
						ranges.emplace_back(std::make_pair(content_start, content_end));
				}

				return ranges;
			}
			std::pair<size_t, size_t> fetch_frame::get_range(core::vector<std::pair<size_t, size_t>>::iterator range, size_t content_length) const
			{
				if (range->first == -1 && range->second == -1)
					return std::make_pair(0, content_length);

				if (range->first == -1)
				{
					if (range->second > content_length)
						range->second = 0;

					range->first = content_length - range->second;
					range->second = content_length;
				}
				else if (range->first > content_length)
					range->first = content_length;

				if (range->second == -1)
					range->second = content_length;
				else if (range->second > content_length)
					range->second = content_length;

				if (range->first > range->second)
					range->first = range->second;

				return std::make_pair(range->first, range->second - range->first);
			}

			connection::connection(server* source) noexcept : resolver(new http::parser()), root(source)
			{
			}
			connection::~connection() noexcept
			{
				core::memory::release(resolver);
				core::memory::release(web_socket);
			}
			void connection::reset(bool fully)
			{
				VI_ASSERT(!route || (route->router && route->router->base), "router should be valid");
				if (!fully)
					info.abort = (info.abort || response.status_code <= 0);
				if (route != nullptr)
					route = route->router->base;
				request.cleanup();
				response.cleanup();
				socket_connection::reset(fully);
			}
			bool connection::compose_response(bool apply_error_response, bool apply_body_inlining, headers_callback&& callback)
			{
				VI_ASSERT(connection_valid(this), "connection should be valid");
				VI_ASSERT(callback != nullptr, "callback should be set");
			retry:
				auto* content = hrm_cache::get()->pop();
				auto status_text = utils::status_message(response.status_code);
				content->append(request.version).append(" ");
				content->append(core::to_string(response.status_code)).append(" ");
				content->append(status_text).append("\r\n");

				std::string_view content_type;
				if (apply_error_response)
				{
					char buffer[core::BLOB_SIZE];
					int size = snprintf(buffer, sizeof(buffer), "<html><head><title>%d %s</title><style>" CSS_MESSAGE_STYLE "%s</style></head><body><div><h1>%d %s</h1></div></body></html>\n", response.status_code, status_text.data(), info.message.size() <= 128 ? CSS_NORMAL_FONT : CSS_SMALL_FONT, response.status_code, info.message.empty() ? status_text.data() : info.message.c_str());
					if (size >= 0)
					{
						response.content.assign(std::string_view(buffer, (size_t)size));
						response.set_header("Content-Length", core::to_string(size));
						content_type = response.get_header("Content-Type");
						if (content_type.empty())
							content_type = response.set_header("Content-Type", "text/html; charset=" + route->char_set).c_str();
					}
				}

				if (response.get_header("Date").empty())
				{
					char date[64];
					content->append("Date: ");
					content->append(header_date(date, info.start / 1000));
					content->append("\r\n");
				}

				if (response.get_header("Connection").empty())
					utils::update_keep_alive_headers(this, *content);

				if (response.get_header("Accept-Ranges").empty())
					content->append("Accept-Ranges: bytes\r\n", 22);

				core::option<core::string> boundary = core::optional::none;
				if (content_type.empty())
				{
					content_type = response.get_header("Content-Type");
					if (content_type.empty())
					{
						content_type = utils::content_type(request.path, &route->mime_types);
						if (!request.get_header("Range").empty())
						{
							boundary = parsing::parse_multipart_data_boundary();
							content->append("Content-Type: multipart/byteranges; boundary=").append(*boundary).append("; charset=").append(route->char_set).append("\r\n");
						}
						else
							content->append("Content-Type: ").append(content_type).append("; charset=").append(route->char_set).append("\r\n");
					}
				}

				if (!response.content.data.empty())
				{
#ifdef VI_ZLIB
					bool deflate = false, gzip = false;
					if (resources::resource_compressed(this, response.content.data.size()))
					{
						auto accept_encoding = request.get_header("Accept-Encoding");
						if (!accept_encoding.empty())
						{
							deflate = accept_encoding.find("deflate") != std::string::npos;
							gzip = accept_encoding.find("gzip") != std::string::npos;
						}

						if (!accept_encoding.empty() && (deflate || gzip))
						{
							z_stream fStream;
							fStream.zalloc = Z_NULL;
							fStream.zfree = Z_NULL;
							fStream.opaque = Z_NULL;
							fStream.avail_in = (uInt)response.content.data.size();
							fStream.next_in = (Bytef*)response.content.data.data();

							if (deflateInit2(&fStream, route->compression.quality_level, Z_DEFLATED, (gzip ? 15 | 16 : 15), route->compression.memory_level, (int)route->compression.tune) == Z_OK)
							{
								core::string buffer(response.content.data.size(), '\0');
								fStream.avail_out = (uInt)buffer.size();
								fStream.next_out = (Bytef*)buffer.c_str();
								bool compress = (::deflate(&fStream, Z_FINISH) == Z_STREAM_END);
								bool flush = (deflateEnd(&fStream) == Z_OK);

								if (compress && flush)
								{
									response.content.assign(std::string_view(buffer.c_str(), (size_t)fStream.total_out));
									if (response.get_header("Content-Encoding").empty())
									{
										if (gzip)
											content->append("Content-Encoding: gzip\r\n", 24);
										else
											content->append("Content-Encoding: deflate\r\n", 27);
									}
								}
							}
						}
					}
#endif
					if (response.status_code != 413 && !request.get_header("Range").empty())
					{
						core::vector<std::pair<size_t, size_t>> ranges = request.get_ranges();
						if (ranges.size() > 1)
						{
							core::string data;
							for (auto it = ranges.begin(); it != ranges.end(); ++it)
							{
								std::pair<size_t, size_t> offset = request.get_range(it, response.content.data.size());
								core::string content_range = paths::construct_content_range(offset.first, offset.second, response.content.data.size());
								if (data.size() > root->router->max_heap_buffer)
								{
									info.message.assign("Content range produced too much data.");
									response.status_code = 413;
									response.content.data.clear();
									hrm_cache::get()->push(content);
									goto retry;
								}

								data.append("--", 2);
								if (boundary)
									data.append(*boundary);
								data.append("\r\n", 2);

								if (!content_type.empty())
								{
									data.append("Content-Type: ", 14);
									data.append(content_type);
									data.append("\r\n", 2);
								}

								data.append("Content-Range: ", 15);
								data.append(content_range.c_str(), content_range.size());
								data.append("\r\n", 2);
								data.append("\r\n", 2);
								if (offset.second > 0)
									data.append(text_substring(response.content.data, offset.first, offset.second));
								data.append("\r\n", 2);
							}

							data.append("--", 2);
							if (boundary)
								data.append(*boundary);
							data.append("--\r\n", 4);
							response.content.assign(data);
						}
						else if (!ranges.empty())
						{
							auto range = ranges.begin();
							bool is_full_length = (range->first == -1 && range->second == -1);
							std::pair<size_t, size_t> offset = request.get_range(range, response.content.data.size());
							if (response.get_header("Content-Range").empty())
								content->append("Content-Range: ").append(paths::construct_content_range(offset.first, offset.second, response.content.data.size())).append("\r\n");
							if (!offset.second)
								response.content.data.clear();
							else if (!is_full_length)
								response.content.assign(text_substring(response.content.data, offset.first, offset.second));
						}
					}

					if (response.get_header("Content-Length").empty())
						content->append("Content-Length: ").append(core::to_string(response.content.data.size())).append("\r\n");
				}
				else if (response.get_header("Content-Length").empty())
					content->append("Content-Length: 0\r\n", 19);

				if (request.user.type == auth::denied && response.get_header("WWW-Authenticate").empty())
					content->append("WWW-Authenticate: " + route->auth.type + " realm=\"" + route->auth.realm + "\"\r\n");

				paths::construct_head_full(&request, &response, false, *content);
				if (route->callbacks.headers)
					route->callbacks.headers(this, *content);

				content->append("\r\n", 2);
				if (apply_body_inlining)
					content->append(response.content.data.begin(), response.content.data.end());

				auto status = stream->write_queued((uint8_t*)content->c_str(), content->size(), [this, content, callback = std::move(callback)](socket_poll event) mutable
				{
					hrm_cache::get()->push(content);
					callback(this, event);
				}, false);
				return status || status.error() == std::errc::operation_would_block;
			}
			bool connection::error_response_requested()
			{
				return response.status_code >= 400 && !response.error && response.content.data.empty();
			}
			bool connection::body_inlining_requested()
			{
				return !response.content.data.empty() && response.content.data.size() <= INLINING_SIZE;
			}
			bool connection::waiting_for_web_socket()
			{
				if (web_socket != nullptr && !web_socket->is_finished())
				{
					web_socket->send_close([](web_socket_frame* frame) { frame->next(); });
					return true;
				}

				core::memory::release(web_socket);
				return false;
			}
			bool connection::send_headers(int status_code, bool specify_transfer_encoding, headers_callback&& callback)
			{
				response.status_code = status_code;
				if (response.status_code <= 0 || stream->outcome > 0 || !response.content.data.empty())
					return false;

				if (specify_transfer_encoding && response.get_header("Transfer-Encoding").empty())
					response.set_header("Transfer-Encoding", "chunked");

				compose_response(error_response_requested(), false, std::move(callback));
				return true;
			}
			bool connection::send_chunk(const std::string_view& chunk, headers_callback&& callback)
			{
				if (response.status_code <= 0 || !stream->outcome || !response.content.data.empty())
					return false;

				auto transfer_encoding = response.get_header("Transfer-Encoding");
				bool is_transfer_encoding_chunked = core::stringify::case_equals(transfer_encoding, "chunked");
				if (is_transfer_encoding_chunked)
				{
					if (!chunk.empty())
					{
						core::string content = core::stringify::text("%x\r\n", (uint32_t)chunk.size());
						content.append(chunk);
						content.append("\r\n");
						stream->write_queued((uint8_t*)content.c_str(), content.size(), std::bind(callback, this, std::placeholders::_1));
					}
					else
						stream->write_queued((uint8_t*)"0\r\n\r\n", 5, std::bind(callback, this, std::placeholders::_1), false);
				}
				else
				{
					if (chunk.empty())
						return false;

					stream->write_queued((uint8_t*)chunk.data(), chunk.size(), std::bind(callback, this, std::placeholders::_1));
				}

				return true;
			}
			bool connection::fetch(content_callback&& callback, bool eat)
			{
				VI_ASSERT(connection_valid(this), "connection should be valid");
				if (!request.content.resources.empty())
				{
					if (callback)
						callback(this, socket_poll::finish_sync, "");
					return false;
				}
				else if (request.content.is_finalized())
				{
					if (!eat && callback && !request.content.data.empty())
						callback(this, socket_poll::next, std::string_view(request.content.data.data(), request.content.data.size()));
					if (callback)
						callback(this, socket_poll::finish_sync, "");
					return true;
				}
				else if (request.content.exceeds)
				{
					if (callback)
						callback(this, socket_poll::finish_sync, "");
					return false;
				}
				else if (!stream->is_valid())
				{
					if (callback)
						callback(this, socket_poll::reset, "");
					return false;
				}

				auto content_type = request.get_header("Content-Type");
				if (content_type == std::string_view("multipart/form-data", 19))
				{
					request.content.exceeds = true;
					if (callback)
						callback(this, socket_poll::finish_sync, "");
					return false;
				}

				auto transfer_encoding = request.get_header("Transfer-Encoding");
				bool is_transfer_encoding_chunked = (!request.content.limited && core::stringify::case_equals(transfer_encoding, "chunked"));
				size_t content_length = request.content.length >= request.content.prefetch ? request.content.length - request.content.prefetch : request.content.length;
				if (is_transfer_encoding_chunked)
					resolver->prepare_for_chunked_parsing();

				if (request.content.prefetch > 0)
				{
					request.content.prefetch = 0;
					if (is_transfer_encoding_chunked)
					{
						size_t decoded_size = request.content.data.size();
						int64_t subresult = resolver->parse_decode_chunked((uint8_t*)request.content.data.data(), &decoded_size);
						request.content.data.resize(decoded_size);
						if (!eat && callback && !request.content.data.empty())
							callback(this, socket_poll::next, std::string_view(request.content.data.data(), request.content.data.size()));

						if (subresult == -1 || subresult == 0)
						{
							request.content.finalize();
							if (callback)
								callback(this, socket_poll::finish_sync, "");
							return subresult == 0;
						}
					}
					else if (!eat && callback)
						callback(this, socket_poll::next, std::string_view(request.content.data.data(), request.content.data.size()));
				}

				if (is_transfer_encoding_chunked)
				{
					return !!stream->read_queued(root->router->max_net_buffer, [this, eat, callback = std::move(callback)](socket_poll event, const uint8_t* buffer, size_t recv)
					{
						if (packet::is_data(event))
						{
							int64_t result = resolver->parse_decode_chunked((uint8_t*)buffer, &recv);
							if (result == -1)
								return false;

							request.content.offset += recv;
							if (eat)
								return result == -2;
							else if (callback)
								callback(this, socket_poll::next, std::string_view((char*)buffer, recv));

							if (request.content.data.size() < root->router->max_net_buffer)
								request.content.append(std::string_view((char*)buffer, recv));
							return result == -2;
						}
						else if (packet::is_done(event) || packet::is_error_or_skip(event))
						{
							request.content.finalize();
							if (callback)
								callback(this, event, "");
						}

						return true;
					});
				}
				else if (content_length > root->router->max_heap_buffer || content_length > root->router->max_net_buffer)
				{
					request.content.exceeds = true;
					if (callback)
						callback(this, socket_poll::finish_sync, "");
					return true;
				}
				else if (request.content.limited && !content_length)
				{
					request.content.finalize();
					if (callback)
						callback(this, socket_poll::finish_sync, "");
					return true;
				}

				return !!stream->read_queued(request.content.limited ? content_length : root->router->max_heap_buffer, [this, eat, callback = std::move(callback)](socket_poll event, const uint8_t* buffer, size_t recv)
				{
					if (packet::is_data(event))
					{
						request.content.offset += recv;
						if (eat)
							return true;

						if (callback)
							callback(this, socket_poll::next, std::string_view((char*)buffer, recv));

						if (request.content.data.size() < root->router->max_heap_buffer)
							request.content.append(std::string_view((char*)buffer, recv));
					}
					else if (packet::is_done(event) || packet::is_error_or_skip(event))
					{
						request.content.finalize();
						if (callback)
							callback(this, event, "");
					}

					return true;
				});
			}
			bool connection::store(resource_callback&& callback, bool eat)
			{
				VI_ASSERT(connection_valid(this), "connection should be valid");
				if (!request.content.resources.empty())
				{
					if (!callback)
						return true;

					if (!eat)
					{
						for (auto& item : request.content.resources)
							callback(&item);
					}

					callback(nullptr);
					return true;
				}
				else if (request.content.is_finalized())
				{
					if (callback)
						callback(nullptr);
					return true;
				}
				else if (!request.content.limited)
				{
					if (callback)
						callback(nullptr);
					return false;
				}

				auto content_type = request.get_header("Content-Type");
				const char* boundary_name = (content_type.empty() ? nullptr : strstr(content_type.data(), "boundary="));
				request.content.exceeds = true;

				size_t content_length = request.content.length >= request.content.prefetch ? request.content.length - request.content.prefetch : request.content.length;
				if (!content_type.empty() && boundary_name != nullptr)
				{
					if (route->router->temporary_directory.empty())
						eat = true;

					core::string boundary("--");
					boundary.append(boundary_name + 9);

					resolver->prepare_for_multipart_parsing(&response.content, &route->router->temporary_directory, route->router->max_uploadable_resources, eat, std::move(callback));
					if (request.content.prefetch > 0)
					{
						request.content.prefetch = 0;
						if (resolver->multipart_parse(boundary.c_str(), (uint8_t*)request.content.data.data(), request.content.data.size()) == -1 || resolver->multipart.finish || !content_length)
						{
							request.content.finalize();
							if (resolver->multipart.callback)
								resolver->multipart.callback(nullptr);
							return false;
						}
					}

					return !!stream->read_queued(content_length, [this, boundary](socket_poll event, const uint8_t* buffer, size_t recv)
					{
						if (packet::is_data(event))
						{
							request.content.offset += recv;
							if (resolver->multipart_parse(boundary.c_str(), buffer, recv) != -1 && !resolver->multipart.finish)
								return true;

							if (resolver->multipart.callback)
								resolver->multipart.callback(nullptr);

							return false;
						}
						else if (packet::is_done(event) || packet::is_error_or_skip(event))
						{
							request.content.finalize();
							if (resolver->multipart.callback)
								resolver->multipart.callback(nullptr);
						}

						return true;
					});
				}
				else if (!content_length)
				{
					if (callback)
						callback(nullptr);
					return true;
				}

				if (eat)
				{
					return !!stream->read_queued(content_length, [this, callback = std::move(callback)](socket_poll event, const uint8_t* buffer, size_t recv)
					{
						if (packet::is_done(event) || packet::is_error_or_skip(event))
						{
							request.content.finalize();
							if (callback)
								callback(nullptr);
						}
						else
							request.content.offset += recv;
						return true;
					});
				}

				http::resource subresource;
				subresource.length = request.content.length;
				subresource.type = (content_type.empty() ? "application/octet-stream" : content_type);

				auto hash = compute::crypto::hash_hex(compute::digests::md5(), *compute::crypto::random_bytes(16));
				subresource.path = *core::os::directory::get_working() + *hash;
				FILE* file = core::os::file::open(subresource.path.c_str(), "wb").or_else(nullptr);
				if (!file || (request.content.prefetch > 0 && fwrite(request.content.data.data(), 1, request.content.data.size(), file) != request.content.data.size()))
				{
					if (file != nullptr)
						core::os::file::close(file);
					if (callback)
						callback(nullptr);
					return false;
				}

				request.content.prefetch = 0;
				return !!stream->read_queued(content_length, [this, file, subresource = std::move(subresource), callback = std::move(callback)](socket_poll event, const uint8_t* buffer, size_t recv)
				{
					if (packet::is_data(event))
					{
						request.content.offset += recv;
						if (fwrite(buffer, 1, recv, file) == recv)
							return true;

						core::os::file::close(file);
						if (callback)
							callback(nullptr);

						return false;
					}
					else if (packet::is_done(event) || packet::is_error_or_skip(event))
					{
						request.content.finalize();
						if (packet::is_done(event))
						{
							request.content.resources.push_back(subresource);
							if (callback)
								callback(&request.content.resources.back());
						}

						if (callback)
							callback(nullptr);

						core::os::file::close(file);
						return false;
					}

					return true;
				});
			}
			bool connection::skip(success_callback&& callback)
			{
				VI_ASSERT(callback != nullptr, "callback should be set");
				fetch([callback = std::move(callback)](http::connection* base, socket_poll event, const std::string_view&) mutable
				{
					if (!packet::is_done(event) && !packet::is_error_or_skip(event))
						return true;

					if (!base->request.content.exceeds)
					{
						callback(base);
						return true;
					}

					return base->store([base, callback = std::move(callback)](http::resource* resource)
					{
						callback(base);
						return true;
					}, true);
				}, true);
				return false;
			}
			bool connection::next()
			{
				VI_ASSERT(connection_valid(this), "connection should be valid");
				if (waiting_for_web_socket())
					return false;

				if (response.status_code <= 0 || stream->outcome > 0)
					return !!root->next(this);

				bool apply_error_response = error_response_requested();
				if (apply_error_response)
				{
					response.error = true;
					for (auto& page : route->error_files)
					{
						if (page.status_code != response.status_code && page.status_code != 0)
							continue;

						request.path = page.pattern;
						response.set_header("X-Error", info.message);
						return routing::route_get(this);
					}
				}

				bool response_body_inlined = body_inlining_requested();
				return compose_response(apply_error_response, response_body_inlined, [response_body_inlined](connection* base, socket_poll event)
				{
					auto& content = base->response.content.data;
					if (packet::is_done(event) && !response_body_inlined && !content.empty() && memcmp(base->request.method, "HEAD", 4) != 0)
						base->stream->write_queued((uint8_t*)content.data(), content.size(), [base](socket_poll) { base->root->next(base); }, false);
					else
						base->root->next(base);
				});
			}
			bool connection::next(int status_code)
			{
				response.status_code = status_code;
				return next();
			}
			bool connection::is_skip_required() const
			{
				if (!request.content.resources.empty() || request.content.is_finalized() || request.content.exceeds || !stream->is_valid())
					return false;

				return true;
			}
			core::expects_io<core::string> connection::get_peer_ip_address() const
			{
				if (!route->proxy_ip_address.empty())
				{
					auto proxy_address = request.get_header(route->proxy_ip_address.c_str());
					if (!proxy_address.empty())
						return core::string(proxy_address);
				}

				auto address = stream->get_peer_address();
				if (!address)
					return address.error();

				return address->get_ip_address();
			}

			query::query() : object(core::var::set::object())
			{
			}
			query::~query() noexcept
			{
				core::memory::release(object);
			}
			void query::clear()
			{
				if (object != nullptr)
					object->clear();
			}
			void query::steal(core::schema** output)
			{
				if (!output)
					return;

				core::memory::release(*output);
				*output = object;
				object = nullptr;
			}
			void query::new_parameter(core::vector<query_token>* tokens, const query_token& name, const query_token& value)
			{
				core::string body = compute::codec::url_decode(std::string_view(name.value, name.length));
				size_t offset = 0, length = body.size();
				char* data = (char*)body.c_str();

				for (size_t i = 0; i < length; i++)
				{
					if (data[i] != '[')
					{
						if (tokens->empty() && i + 1 >= length)
						{
							query_token token;
							token.value = data + offset;
							token.length = i - offset + 1;
							tokens->push_back(token);
						}

						continue;
					}

					if (tokens->empty())
					{
						query_token token;
						token.value = data + offset;
						token.length = i - offset;
						tokens->push_back(token);
					}

					offset = i;
					while (i + 1 < length && data[i + 1] != ']')
						i++;

					query_token token;
					token.value = data + offset + 1;
					token.length = i - offset;
					tokens->push_back(token);

					if (i + 1 >= length)
						break;

					offset = i + 1;
				}

				if (!value.value || !value.length)
					return;

				core::schema* parameter = nullptr;
				for (auto& item : *tokens)
				{
					if (parameter != nullptr)
						parameter = find_parameter(parameter, &item);
					else
						parameter = get_parameter(&item);
				}

				if (parameter != nullptr)
					parameter->value.deserialize(compute::codec::url_decode(std::string_view(value.value, value.length)));
			}
			void query::decode(const std::string_view& type, const std::string_view& body)
			{
				if (type.empty() || body.empty())
					return;

				if (core::stringify::case_equals(type, "application/x-www-form-urlencoded"))
					decode_axwfd(body);
				else if (core::stringify::case_equals(type, "application/json"))
					decode_ajson(body);
			}
			void query::decode_axwfd(const std::string_view& body)
			{
				core::vector<query_token> tokens;
				size_t offset = 0, length = body.size();
				char* data = (char*)body.data();

				for (size_t i = 0; i < length; i++)
				{
					if (data[i] != '&' && data[i] != '=' && i + 1 < length)
						continue;

					query_token name;
					name.value = data + offset;
					name.length = i - offset;
					offset = i;

					if (data[i] == '=')
					{
						while (i + 1 < length && data[i + 1] != '&')
							i++;
					}

					query_token value;
					value.value = data + offset + 1;
					value.length = i - offset;

					new_parameter(&tokens, name, value);
					tokens.clear();
					offset = i + 2;
					i++;
				}
			}
			void query::decode_ajson(const std::string_view& body)
			{
				core::memory::release(object);
				auto result = core::schema::convert_from_json(body);
				if (result)
					object = *result;
			}
			core::string query::encode(const std::string_view& type) const
			{
				if (core::stringify::case_equals(type, "application/x-www-form-urlencoded"))
					return encode_axwfd();

				if (core::stringify::case_equals(type, "application/json"))
					return encode_ajson();

				return "";
			}
			core::string query::encode_axwfd() const
			{
				core::string output; auto& nodes = object->get_childs();
				for (auto it = nodes.begin(); it != nodes.end(); ++it)
				{
					output.append(build_from_base(*it));
					if (it + 1 < nodes.end())
						output.append(1, '&');
				}

				return output;
			}
			core::string query::encode_ajson() const
			{
				core::string stream;
				core::schema::convert_to_json(object, [&stream](core::var_form, const std::string_view& buffer) { stream.append(buffer); });
				return stream;
			}
			core::schema* query::get(const std::string_view& name) const
			{
				return (core::schema*)object->get(name);
			}
			core::schema* query::set(const std::string_view& name)
			{
				return (core::schema*)object->set(name, core::var::string(""));
			}
			core::schema* query::set(const std::string_view& name, const std::string_view& value)
			{
				return (core::schema*)object->set(name, core::var::string(value));
			}
			core::schema* query::get_parameter(query_token* name)
			{
				VI_ASSERT(name != nullptr, "token should be set");
				if (name->value && name->length > 0)
				{
					for (auto* item : object->get_childs())
					{
						if (item->key.size() != name->length)
							continue;

						if (!strncmp(item->key.c_str(), name->value, (size_t)name->length))
							return (core::schema*)item;
					}
				}

				core::schema* init = core::var::set::object();
				if (name->value && name->length > 0)
				{
					init->key.assign(name->value, (size_t)name->length);
					if (!core::stringify::has_integer(init->key))
						object->value = core::var::object();
					else
						object->value = core::var::array();
				}
				else
				{
					init->key.assign(core::to_string(object->size()));
					object->value = core::var::array();
				}

				init->value = core::var::string("");
				object->push(init);

				return init;
			}
			core::string query::build(core::schema* base)
			{
				core::string output, label = compute::codec::url_encode(base->get_parent() != nullptr ? ('[' + base->key + ']') : base->key);
				if (!base->empty())
				{
					auto& childs = base->get_childs();
					for (auto it = childs.begin(); it != childs.end(); ++it)
					{
						output.append(label).append(build(*it));
						if (it + 1 < childs.end())
							output += '&';
					}
				}
				else
				{
					core::string v = base->value.serialize();
					if (!v.empty())
						output.append(label).append(1, '=').append(compute::codec::url_encode(v));
					else
						output.append(label);
				}

				return output;
			}
			core::string query::build_from_base(core::schema* base)
			{
				core::string output, label = compute::codec::url_encode(base->key);
				if (!base->empty())
				{
					auto& childs = base->get_childs();
					for (auto it = childs.begin(); it != childs.end(); ++it)
					{
						output.append(label).append(build(*it));
						if (it + 1 < childs.end())
							output += '&';
					}
				}
				else
				{
					core::string v = base->value.serialize();
					if (!v.empty())
						output.append(label).append(1, '=').append(compute::codec::url_encode(v));
					else
						output.append(label);
				}

				return output;
			}
			core::schema* query::find_parameter(core::schema* base, query_token* name)
			{
				VI_ASSERT(name != nullptr, "token should be set");
				if (!base->empty() && name->value && name->length > 0)
				{
					for (auto* item : base->get_childs())
					{
						if (!strncmp(item->key.c_str(), name->value, (size_t)name->length))
							return (core::schema*)item;
					}
				}

				core::string key;
				if (name->value && name->length > 0)
				{
					key.assign(name->value, (size_t)name->length);
					if (!core::stringify::has_integer(key))
						base->value = core::var::object();
					else
						base->value = core::var::array();
				}
				else
				{
					key.assign(core::to_string(base->size()));
					base->value = core::var::array();
				}

				return base->set(key, core::var::string(""));
			}

			session::session()
			{
				query = core::var::set::object();
			}
			session::~session() noexcept
			{
				core::memory::release(query);
			}
			core::expects_system<void> session::write(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				auto* router = base->route->router;
				core::string path = router->session.directory + find_session_id(base);
				auto stream = core::os::file::open(path.c_str(), "wb");
				if (!stream)
					return core::system_exception("session write error", std::move(stream.error()));

				session_expires = time(nullptr) + router->session.expires;
				fwrite(&session_expires, sizeof(int64_t), 1, *stream);
				query->convert_to_jsonb(query, [&stream](core::var_form, const std::string_view& buffer)
				{
					if (!buffer.empty())
						fwrite(buffer.data(), buffer.size(), 1, *stream);
				});
				core::os::file::close(*stream);
				return core::expectation::met;
			}
			core::expects_system<void> session::read(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				core::string path = base->route->router->session.directory + find_session_id(base);
				auto stream = core::os::file::open(path.c_str(), "rb");
				if (!stream)
					return core::system_exception("session read error", std::move(stream.error()));

				if (fread(&session_expires, 1, sizeof(int64_t), *stream) != sizeof(int64_t))
				{
					core::os::file::close(*stream);
					return core::system_exception("session read error: invalid format", std::make_error_condition(std::errc::bad_message));
				}

				if (session_expires <= time(nullptr))
				{
					session_id.clear();
					core::os::file::close(*stream);
					core::os::file::remove(path.c_str());
					return core::system_exception("session read error: expired", std::make_error_condition(std::errc::timed_out));
				}

				core::memory::release(query);
				auto result = core::schema::convert_from_jsonb([&stream](uint8_t* buffer, size_t size) { return fread(buffer, sizeof(uint8_t), size, *stream) == size; });
				core::os::file::close(*stream);
				if (!result)
					return core::system_exception(result.error().message(), std::make_error_condition(std::errc::bad_message));

				query = *result;
				return core::expectation::met;
			}
			void session::clear()
			{
				if (query != nullptr)
					query->clear();
			}
			core::string& session::find_session_id(connection* base)
			{
				if (!session_id.empty())
					return session_id;

				VI_ASSERT(connection_valid(base), "connection should be valid");
				auto value = base->request.get_cookie(base->route->router->session.cookie.name.c_str());
				if (value.empty())
					return generate_session_id(base);

				return session_id.assign(value);
			}
			core::string& session::generate_session_id(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				int64_t time = ::time(nullptr);
				auto hash = compute::crypto::hash_hex(compute::digests::md5(), base->request.location + core::to_string(time));
				if (hash)
					session_id = *hash;
				else
					session_id = core::to_string(time);

				auto* router = base->route->router;
				if (session_expires == 0)
					session_expires = time + router->session.expires;

				cookie result;
				result.value = session_id;
				result.name = router->session.cookie.name;
				result.domain = router->session.cookie.domain;
				result.path = router->session.cookie.path;
				result.same_site = router->session.cookie.same_site;
				result.secure = router->session.cookie.secure;
				result.http_only = router->session.cookie.http_only;
				result.set_expires(time + (int64_t)router->session.cookie.expires);
				base->response.set_cookie(std::move(result));

				return session_id;
			}
			core::expects_system<void> session::invalidate_cache(const std::string_view& path)
			{
				core::vector<std::pair<core::string, core::file_entry>> entries;
				auto status = core::os::directory::scan(path, entries);
				if (!status)
					return core::system_exception("session invalidation scan error", std::move(status.error()));

				bool split = (path.back() != '\\' && path.back() != '/');
				for (auto& item : entries)
				{
					if (item.second.is_directory)
						continue;

					core::string filename = core::string(path);
					if (split)
						filename.append(1, VI_SPLITTER);
					filename.append(item.first);
					status = core::os::file::remove(filename);
					if (!status)
						return core::system_exception("session invalidation remove error: " + item.first, std::move(status.error()));
				}

				return core::expectation::met;
			}

			parser::parser()
			{
			}
			parser::~parser() noexcept
			{
				core::memory::deallocate(multipart.boundary);
			}
			void parser::prepare_for_request_parsing(request_frame* request)
			{
				message.header.clear();
				message.version = request->version;
				message.method = request->method;
				message.status_code = nullptr;
				message.location = &request->location;
				message.query = &request->query;
				message.cookies = &request->cookies;
				message.headers = &request->headers;
				message.content = &request->content;
			}
			void parser::prepare_for_response_parsing(response_frame* response)
			{
				message.header.clear();
				message.version = nullptr;
				message.method = nullptr;
				message.status_code = &response->status_code;
				message.location = nullptr;
				message.query = nullptr;
				message.cookies = nullptr;
				message.headers = &response->headers;
				message.content = &response->content;
			}
			void parser::prepare_for_chunked_parsing()
			{
				chunked = chunked_state();
			}
			void parser::prepare_for_multipart_parsing(content_frame* content, core::string* temporary_directory, size_t max_resources, bool skip, resource_callback&& callback)
			{
				core::memory::deallocate(multipart.boundary);
				multipart = multipart_state();
				multipart.skip = skip;
				multipart.max_resources = max_resources;
				multipart.temporary_directory = temporary_directory;
				multipart.callback = std::move(callback);
				message.header.clear();
				message.content = content;
			}
			int64_t parser::multipart_parse(const std::string_view& boundary, const uint8_t* buffer, size_t length)
			{
				VI_ASSERT(buffer != nullptr, "buffer should be set");
				if (!multipart.boundary || !multipart.look_behind)
				{
					core::memory::deallocate(multipart.boundary);
					multipart.length = boundary.size();
					multipart.boundary = core::memory::allocate<uint8_t>(sizeof(uint8_t) * (size_t)(multipart.length * 2 + 9));
					memcpy(multipart.boundary, boundary.data(), sizeof(uint8_t) * (size_t)multipart.length);
					multipart.boundary[multipart.length] = '\0';
					multipart.look_behind = (multipart.boundary + multipart.length + 1);
					multipart.state = multipart_status::start;
					multipart.index = 0;
				}

				char value, lower;
				char LF = 10, CR = 13;
				size_t i = 0, mark = 0;
				int last = 0;

				while (i < length)
				{
					value = buffer[i];
					last = (i == (length - 1));

					switch (multipart.state)
					{
						case multipart_status::start:
							multipart.index = 0;
							multipart.state = multipart_status::start_boundary;
						case multipart_status::start_boundary:
							if (multipart.index == multipart.length)
							{
								if (value != CR)
									return i;

								multipart.index++;
								break;
							}
							else if (multipart.index == (multipart.length + 1))
							{
								if (value != LF)
									return i;

								multipart.index = 0;
								if (!parsing::parse_multipart_resource_begin(this))
									return i;

								multipart.state = multipart_status::header_field_start;
								break;
							}

							if (value != multipart.boundary[multipart.index])
								return i;

							multipart.index++;
							break;
						case multipart_status::header_field_start:
							mark = i;
							multipart.state = multipart_status::header_field;
						case multipart_status::header_field:
							if (value == CR)
							{
								multipart.state = multipart_status::header_field_waiting;
								break;
							}

							if (value == ':')
							{
								if (!parsing::parse_multipart_header_field(this, buffer + mark, i - mark))
									return i;

								multipart.state = multipart_status::header_value_start;
								break;
							}

							lower = tolower(value);
							if ((value != '-') && (lower < 'a' || lower > 'z'))
								return i;

							if (last && !parsing::parse_multipart_header_field(this, buffer + mark, (i - mark) + 1))
								return i;

							break;
						case multipart_status::header_field_waiting:
							if (value != LF)
								return i;

							multipart.state = multipart_status::resource_start;
							break;
						case multipart_status::header_value_start:
							if (value == ' ')
								break;

							mark = i;
							multipart.state = multipart_status::header_value;
						case multipart_status::header_value:
							if (value == CR)
							{
								if (!parsing::parse_multipart_header_value(this, buffer + mark, i - mark))
									return i;

								multipart.state = multipart_status::header_value_waiting;
								break;
							}

							if (last && !parsing::parse_multipart_header_value(this, buffer + mark, (i - mark) + 1))
								return i;

							break;
						case multipart_status::header_value_waiting:
							if (value != LF)
								return i;

							multipart.state = multipart_status::header_field_start;
							break;
						case multipart_status::resource_start:
							mark = i;
							multipart.state = multipart_status::resource;
						case multipart_status::resource:
							if (value == CR)
							{
								if (!parsing::parse_multipart_content_data(this, buffer + mark, i - mark))
									return i;

								mark = i;
								multipart.state = multipart_status::resource_boundary_waiting;
								multipart.look_behind[0] = CR;
								break;
							}

							if (last && !parsing::parse_multipart_content_data(this, buffer + mark, (i - mark) + 1))
								return i;

							break;
						case multipart_status::resource_boundary_waiting:
							if (value == LF)
							{
								multipart.state = multipart_status::resource_boundary;
								multipart.look_behind[1] = LF;
								multipart.index = 0;
								break;
							}

							if (!parsing::parse_multipart_content_data(this, multipart.look_behind, 1))
								return i;

							multipart.state = multipart_status::resource;
							mark = i--;
							break;
						case multipart_status::resource_boundary:
							if (multipart.boundary[multipart.index] != value)
							{
								if (!parsing::parse_multipart_content_data(this, multipart.look_behind, 2 + (size_t)multipart.index))
									return i;

								multipart.state = multipart_status::resource;
								mark = i--;
								break;
							}

							multipart.look_behind[2 + multipart.index] = value;
							if ((++multipart.index) == multipart.length)
							{
								if (!parsing::parse_multipart_resource_end(this))
									return i;

								multipart.state = multipart_status::resource_waiting;
							}
							break;
						case multipart_status::resource_waiting:
							if (value == '-')
							{
								multipart.state = multipart_status::resource_hyphen;
								break;
							}

							if (value == CR)
							{
								multipart.state = multipart_status::resource_end;
								break;
							}

							return i;
						case multipart_status::resource_hyphen:
							if (value == '-')
							{
								multipart.state = multipart_status::end;
								break;
							}

							return i;
						case multipart_status::resource_end:
							if (value == LF)
							{
								multipart.state = multipart_status::header_field_start;
								if (!parsing::parse_multipart_resource_begin(this))
									return i;

								break;
							}

							return i;
						case multipart_status::end:
							break;
						default:
							return -1;
					}

					++i;
				}

				return length;
			}
			int64_t parser::parse_request(const uint8_t* buffer_start, size_t length, size_t offset)
			{
				VI_ASSERT(buffer_start != nullptr, "buffer start should be set");
				const uint8_t* buffer = buffer_start;
				const uint8_t* buffer_end = buffer_start + length;
				int result;

				if (is_completed(buffer, buffer_end, offset, &result) == nullptr)
					return (int64_t)result;

				if ((buffer = process_request(buffer, buffer_end, &result)) == nullptr)
					return (int64_t)result;

				return (int64_t)(buffer - buffer_start);
			}
			int64_t parser::parse_response(const uint8_t* buffer_start, size_t length, size_t offset)
			{
				if (!buffer_start)
					return 0;

				int result;
				const uint8_t* buffer = buffer_start;
				const uint8_t* buffer_end = buffer + length;
				if (is_completed(buffer, buffer_end, offset, &result) == nullptr)
					return (int64_t)result;

				if ((buffer = process_response(buffer, buffer_end, &result)) == nullptr)
					return (int64_t)result;

				return (int64_t)(buffer - buffer_start);
			}
			int64_t parser::parse_decode_chunked(uint8_t* buffer, size_t* length)
			{
				VI_ASSERT(buffer != nullptr && length != nullptr, "buffer should be set");
				size_t dest = 0, src = 0, size = *length;
				int64_t result = -2;

				while (true)
				{
					switch (chunked.state)
					{
						case chunked_status::size:
							for (;; ++src)
							{
								if (src == size)
									goto exit;

								int v = buffer[src];
								if ('0' <= v && v <= '9')
									v = v - '0';
								else if ('A' <= v && v <= 'F')
									v = v - 'A' + 0xa;
								else if ('a' <= v && v <= 'f')
									v = v - 'a' + 0xa;
								else
									v = -1;

								if (v == -1)
								{
									if (chunked.hex_count == 0)
									{
										result = -1;
										goto exit;
									}
									break;
								}

								if (chunked.hex_count == sizeof(size_t) * 2)
								{
									result = -1;
									goto exit;
								}

								chunked.length = chunked.length * 16 + v;
								++chunked.hex_count;
							}

							chunked.hex_count = 0;
							chunked.state = chunked_status::ext;
						case chunked_status::ext:
							for (;; ++src)
							{
								if (src == size)
									goto exit;
								if (buffer[src] == '\012')
									break;
							}

							++src;
							if (chunked.length == 0)
							{
								if (chunked.consume_trailer)
								{
									chunked.state = chunked_status::head;
									break;
								}
								else
									goto complete;
							}

							chunked.state = chunked_status::data;
						case chunked_status::data:
						{
							size_t avail = size - src;
							if (avail < chunked.length)
							{
								if (dest != src)
									memmove(buffer + dest, buffer + src, avail);

								src += avail;
								dest += avail;
								chunked.length -= avail;
								goto exit;
							}

							if (dest != src)
								memmove(buffer + dest, buffer + src, chunked.length);

							src += chunked.length;
							dest += chunked.length;
							chunked.length = 0;
							chunked.state = chunked_status::end;
						}
						case chunked_status::end:
							for (;; ++src)
							{
								if (src == size)
									goto exit;

								if (buffer[src] != '\015')
									break;
							}

							if (buffer[src] != '\012')
							{
								result = -1;
								goto exit;
							}

							++src;
							chunked.state = chunked_status::size;
							break;
						case chunked_status::head:
							for (;; ++src)
							{
								if (src == size)
									goto exit;

								if (buffer[src] != '\015')
									break;
							}

							if (buffer[src++] == '\012')
								goto complete;

							chunked.state = chunked_status::middle;
						case chunked_status::middle:
							for (;; ++src)
							{
								if (src == size)
									goto exit;

								if (buffer[src] == '\012')
									break;
							}

							++src;
							chunked.state = chunked_status::head;
							break;
						default:
							return -1;
					}
				}

			complete:
				result = size - src;

			exit:
				if (dest != src)
					memmove(buffer + dest, buffer + src, size - src);

				*length = dest;
				return result;
			}
			const uint8_t* parser::tokenize(const uint8_t* buffer, const uint8_t* buffer_end, const uint8_t** token, size_t* token_length, int* out)
			{
				VI_ASSERT(buffer != nullptr, "buffer should be set");
				VI_ASSERT(buffer_end != nullptr, "buffer end should be set");
				VI_ASSERT(token != nullptr, "token should be set");
				VI_ASSERT(token_length != nullptr, "token length should be set");
				VI_ASSERT(out != nullptr, "output should be set");

				const uint8_t* token_start = buffer;
				while (buffer_end - buffer >= 8)
				{
					for (int i = 0; i < 8; i++)
					{
						if (!((uint8_t)(*buffer) - 040u < 0137u))
							goto non_printable;
						++buffer;
					}

					continue;
				non_printable:
					if (((uint8_t)*buffer < '\040' && *buffer != '\011') || *buffer == '\177')
						goto found_control;
					++buffer;
				}

				for (;; ++buffer)
				{
					if (buffer == buffer_end)
					{
						*out = -2;
						return nullptr;
					}

					if (!((uint8_t)(*buffer) - 040u < 0137u))
					{
						if (((uint8_t)*buffer < '\040' && *buffer != '\011') || *buffer == '\177')
							goto found_control;
					}
				}

			found_control:
				if (*buffer == '\015')
				{
					++buffer;
					if (buffer == buffer_end)
					{
						*out = -2;
						return nullptr;
					}

					if (*buffer++ != '\012')
					{
						*out = -1;
						return nullptr;
					}

					*token_length = buffer - 2 - token_start;
				}
				else if (*buffer == '\012')
				{
					*token_length = buffer - token_start;
					++buffer;
				}
				else
				{
					*out = -1;
					return nullptr;
				}

				*token = token_start;
				return buffer;
			}
			const uint8_t* parser::is_completed(const uint8_t* buffer, const uint8_t* buffer_end, size_t offset, int* out)
			{
				VI_ASSERT(buffer != nullptr, "buffer should be set");
				VI_ASSERT(buffer_end != nullptr, "buffer end should be set");
				VI_ASSERT(out != nullptr, "output should be set");

				int result = 0;
				buffer = offset < 3 ? buffer : buffer + offset - 3;

				while (true)
				{
					if (buffer == buffer_end)
					{
						*out = -2;
						return nullptr;
					}

					if (*buffer == '\015')
					{
						++buffer;
						if (buffer == buffer_end)
						{
							*out = -2;
							return nullptr;
						}

						if (*buffer++ != '\012')
						{
							*out = -1;
							return nullptr;
						}
						++result;
					}
					else if (*buffer == '\012')
					{
						++buffer;
						++result;
					}
					else
					{
						++buffer;
						result = 0;
					}

					if (result == 2)
						return buffer;
				}

				return nullptr;
			}
			const uint8_t* parser::process_version(const uint8_t* buffer, const uint8_t* buffer_end, int* out)
			{
				VI_ASSERT(buffer != nullptr, "buffer should be set");
				VI_ASSERT(buffer_end != nullptr, "buffer end should be set");
				VI_ASSERT(out != nullptr, "output should be set");

				if (buffer_end - buffer < 9)
				{
					*out = -2;
					return nullptr;
				}

				const uint8_t* version = buffer;
				if (*(buffer++) != 'H')
				{
					*out = -1;
					return nullptr;
				}

				if (*(buffer++) != 'T')
				{
					*out = -1;
					return nullptr;
				}

				if (*(buffer++) != 'T')
				{
					*out = -1;
					return nullptr;
				}

				if (*(buffer++) != 'P')
				{
					*out = -1;
					return nullptr;
				}

				if (*(buffer++) != '/')
				{
					*out = -1;
					return nullptr;
				}

				if (*(buffer++) != '1')
				{
					*out = -1;
					return nullptr;
				}

				if (*(buffer++) != '.')
				{
					*out = -1;
					return nullptr;
				}

				if (*buffer < '0' || '9' < *buffer)
				{
					*out = -1;
					return nullptr;
				}

				buffer++;
				if (!parsing::parse_version(this, version, buffer - version))
				{
					*out = -1;
					return nullptr;
				}

				return buffer;
			}
			const uint8_t* parser::process_headers(const uint8_t* buffer, const uint8_t* buffer_end, int* out)
			{
				VI_ASSERT(buffer != nullptr, "buffer should be set");
				VI_ASSERT(buffer_end != nullptr, "buffer end should be set");
				VI_ASSERT(out != nullptr, "output should be set");

				static const char* mapping =
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\1\0\1\1\1\1\1\0\0\1\1\0\1\1\0\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0"
					"\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\1"
					"\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\1\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

				if (message.headers != nullptr)
					message.headers->clear();
				if (message.cookies != nullptr)
					message.cookies->clear();

				while (true)
				{
					if (buffer == buffer_end)
					{
						*out = -2;
						return nullptr;
					}

					if (*buffer == '\015')
					{
						++buffer;
						if (buffer == buffer_end)
						{
							*out = -2;
							return nullptr;
						}

						if (*buffer++ != '\012')
						{
							*out = -1;
							return nullptr;
						}

						break;
					}
					else if (*buffer == '\012')
					{
						++buffer;
						break;
					}

					if (!(*buffer == ' ' || *buffer == '\t'))
					{
						const uint8_t* name = buffer;
						while (true)
						{
							if (*buffer == ':')
								break;

							if (!mapping[(uint8_t)*buffer])
							{
								*out = -1;
								return nullptr;
							}

							++buffer;
							if (buffer == buffer_end)
							{
								*out = -2;
								return nullptr;
							}
						}

						int64_t length = buffer - name;
						if (length == 0)
						{
							*out = -1;
							return nullptr;
						}

						if (!parsing::parse_header_field(this, name, (size_t)length))
						{
							*out = -1;
							return nullptr;
						}

						++buffer;
						for (;; ++buffer)
						{
							if (buffer == buffer_end)
							{
								if (!parsing::parse_header_value(this, (uint8_t*)"", 0))
								{
									*out = -1;
									return nullptr;
								}

								*out = -2;
								return nullptr;
							}

							if (!(*buffer == ' ' || *buffer == '\t'))
								break;
						}
					}
					else if (!parsing::parse_header_field(this, (uint8_t*)"", 0))
					{
						*out = -1;
						return nullptr;
					}

					const uint8_t* value; size_t value_length;
					if ((buffer = tokenize(buffer, buffer_end, &value, &value_length, out)) == nullptr)
					{
						parsing::parse_header_value(this, (uint8_t*)"", 0);
						return nullptr;
					}

					const uint8_t* value_end = value + value_length;
					for (; value_end != value; --value_end)
					{
						const uint8_t c = *(value_end - 1);
						if (!(c == ' ' || c == '\t'))
							break;
					}

					if (!parsing::parse_header_value(this, value, value_end - value))
					{
						*out = -1;
						return nullptr;
					}
				}

				return buffer;
			}
			const uint8_t* parser::process_request(const uint8_t* buffer, const uint8_t* buffer_end, int* out)
			{
				VI_ASSERT(buffer != nullptr, "buffer should be set");
				VI_ASSERT(buffer_end != nullptr, "buffer end should be set");
				VI_ASSERT(out != nullptr, "output should be set");

				if (buffer == buffer_end)
				{
					*out = -2;
					return nullptr;
				}

				if (*buffer == '\015')
				{
					++buffer;
					if (buffer == buffer_end)
					{
						*out = -2;
						return nullptr;
					}

					if (*buffer++ != '\012')
					{
						*out = -1;
						return nullptr;
					}
				}
				else if (*buffer == '\012')
					++buffer;

				const uint8_t* token_start = buffer;
				if (buffer == buffer_end)
				{
					*out = -2;
					return nullptr;
				}

				while (true)
				{
					if (*buffer == ' ')
						break;

					if (!((uint8_t)(*buffer) - 040u < 0137u))
					{
						if ((uint8_t)*buffer < '\040' || *buffer == '\177')
						{
							*out = -1;
							return nullptr;
						}
					}

					++buffer;
					if (buffer == buffer_end)
					{
						*out = -2;
						return nullptr;
					}
				}

				if (buffer - token_start == 0)
					return nullptr;

				if (!parsing::parse_method_value(this, token_start, buffer - token_start))
				{
					*out = -1;
					return nullptr;
				}

				do
				{
					++buffer;
				} while (*buffer == ' ');

				token_start = buffer;
				if (buffer == buffer_end)
				{
					*out = -2;
					return nullptr;
				}

				while (true)
				{
					if (*buffer == ' ')
						break;

					if (!((uint8_t)(*buffer) - 040u < 0137u))
					{
						if ((uint8_t)*buffer < '\040' || *buffer == '\177')
						{
							*out = -1;
							return nullptr;
						}
					}

					++buffer;
					if (buffer == buffer_end)
					{
						*out = -2;
						return nullptr;
					}
				}

				if (buffer - token_start == 0)
					return nullptr;

				uint8_t* path = (uint8_t*)token_start;
				int64_t PL = buffer - token_start, QL = 0;
				while (QL < PL && path[QL] != '?')
					QL++;

				if (QL > 0 && QL < PL)
				{
					QL = PL - QL - 1;
					PL -= QL + 1;
					if (!parsing::parse_query_value(this, path + PL + 1, (size_t)QL))
					{
						*out = -1;
						return nullptr;
					}
				}

				if (!parsing::parse_path_value(this, path, (size_t)PL))
				{
					*out = -1;
					return nullptr;
				}

				do
				{
					++buffer;
				} while (*buffer == ' ');
				if ((buffer = process_version(buffer, buffer_end, out)) == nullptr)
					return nullptr;

				if (*buffer == '\015')
				{
					++buffer;
					if (buffer == buffer_end)
					{
						*out = -2;
						return nullptr;
					}

					if (*buffer++ != '\012')
					{
						*out = -1;
						return nullptr;
					}
				}
				else if (*buffer != '\012')
				{
					*out = -1;
					return nullptr;
				}
				else
					++buffer;

				return process_headers(buffer, buffer_end, out);
			}
			const uint8_t* parser::process_response(const uint8_t* buffer, const uint8_t* buffer_end, int* out)
			{
				VI_ASSERT(buffer != nullptr, "buffer should be set");
				VI_ASSERT(buffer_end != nullptr, "buffer end should be set");
				VI_ASSERT(out != nullptr, "output should be set");

				if ((buffer = process_version(buffer, buffer_end, out)) == nullptr)
					return nullptr;

				if (*buffer != ' ')
				{
					*out = -1;
					return nullptr;
				}

				do
				{
					++buffer;
				} while (*buffer == ' ');
				if (buffer_end - buffer < 4)
				{
					*out = -2;
					return nullptr;
				}

				int result = 0, status = 0;
				if (*buffer < '0' || '9' < *buffer)
				{
					*out = -1;
					return nullptr;
				}

				*(&result) = 100 * (*buffer++ - '0');
				status = result;
				if (*buffer < '0' || '9' < *buffer)
				{
					*out = -1;
					return nullptr;
				}

				*(&result) = 10 * (*buffer++ - '0');
				status += result;
				if (*buffer < '0' || '9' < *buffer)
				{
					*out = -1;
					return nullptr;
				}

				*(&result) = (*buffer++ - '0');
				status += result;
				if (!parsing::parse_status_code(this, status))
				{
					*out = -1;
					return nullptr;
				}

				const uint8_t* message; size_t message_length;
				if ((buffer = tokenize(buffer, buffer_end, &message, &message_length, out)) == nullptr)
					return nullptr;

				if (message_length == 0)
					return process_headers(buffer, buffer_end, out);

				if (*message != ' ')
				{
					*out = -1;
					return nullptr;
				}

				do
				{
					++message;
					--message_length;
				} while (*message == ' ');
				if (!parsing::parse_status_message(this, message, message_length))
				{
					*out = -1;
					return nullptr;
				}

				return process_headers(buffer, buffer_end, out);
			}

			web_codec::web_codec() : state(bytecode::begin), fragment(0)
			{
			}
			bool web_codec::parse_frame(const uint8_t* buffer, size_t size)
			{
				if (!buffer || !size)
					return !queue.empty();

				if (payload.capacity() <= size)
					payload.resize(size);

				memcpy(payload.data(), buffer, sizeof(char) * size);
				char* data = payload.data();
			parse_payload:
				while (size)
				{
					uint8_t index = *data;
					switch (state)
					{
						case bytecode::begin:
						{
							uint8_t op = index & 0x0f;
							if (index & 0x70)
								return !queue.empty();

							final = (index & 0x80) ? 1 : 0;
							if (op == 0)
							{
								if (!fragment)
									return !queue.empty();

								control = 0;
							}
							else if (op & 0x8)
							{
								if (op != (uint8_t)web_socket_op::ping && op != (uint8_t)web_socket_op::pong && op != (uint8_t)web_socket_op::close)
									return !queue.empty();

								if (!final)
									return !queue.empty();

								control = 1;
								opcode = (web_socket_op)op;
							}
							else
							{
								if (op != (uint8_t)web_socket_op::text && op != (uint8_t)web_socket_op::binary)
									return !queue.empty();

								control = 0;
								fragment = !final;
								opcode = (web_socket_op)op;
							}

							state = bytecode::length;
							data++; size--;
							break;
						}
						case bytecode::length:
						{
							uint8_t length = index & 0x7f;
							masked = (index & 0x80) ? 1 : 0;
							masks = 0;

							if (control)
							{
								if (length > 125)
									return !queue.empty();

								remains = length;
								state = masked ? bytecode::mask0 : bytecode::end;
							}
							else if (length < 126)
							{
								remains = length;
								state = masked ? bytecode::mask0 : bytecode::end;
							}
							else if (length == 126)
								state = bytecode::length160;
							else
								state = bytecode::length640;

							data++; size--;
							if (state == bytecode::end && remains == 0)
							{
								queue.emplace(std::make_pair(opcode, core::vector<char>()));
								goto fetch_payload;
							}
							break;
						}
						case bytecode::length160:
						{
							remains = (uint64_t)index << 8;
							state = bytecode::length161;
							data++; size--;
							break;
						}
						case bytecode::length161:
						{
							remains |= (uint64_t)index << 0;
							state = masked ? bytecode::mask0 : bytecode::end;
							if (remains < 126)
								return !queue.empty();

							data++; size--;
							break;
						}
						case bytecode::length640:
						{
							remains = (uint64_t)index << 56;
							state = bytecode::length641;
							data++; size--;
							break;
						}
						case bytecode::length641:
						{
							remains |= (uint64_t)index << 48;
							state = bytecode::length642;
							data++; size--;
							break;
						}
						case bytecode::length642:
						{
							remains |= (uint64_t)index << 40;
							state = bytecode::length643;
							data++; size--;
							break;
						}
						case bytecode::length643:
						{
							remains |= (uint64_t)index << 32;
							state = bytecode::length644;
							data++; size--;
							break;
						}
						case bytecode::length644:
						{
							remains |= (uint64_t)index << 24;
							state = bytecode::length645;
							data++; size--;
							break;
						}
						case bytecode::length645:
						{
							remains |= (uint64_t)index << 16;
							state = bytecode::length646;
							data++; size--;
							break;
						}
						case bytecode::length646:
						{
							remains |= (uint64_t)index << 8;
							state = bytecode::length647;
							data++; size--;
							break;
						}
						case bytecode::length647:
						{
							remains |= (uint64_t)index << 0;
							state = masked ? bytecode::mask0 : bytecode::end;
							if (remains < 65536)
								return !queue.empty();

							data++; size--;
							break;
						}
						case bytecode::mask0:
						{
							mask[0] = index;
							state = bytecode::mask1;
							data++; size--;
							break;
						}
						case bytecode::mask1:
						{
							mask[1] = index;
							state = bytecode::mask2;
							data++; size--;
							break;
						}
						case bytecode::mask2:
						{
							mask[2] = index;
							state = bytecode::mask3;
							data++; size--;
							break;
						}
						case bytecode::mask3:
						{
							mask[3] = index;
							state = bytecode::end;
							data++; size--;
							if (remains == 0)
							{
								queue.emplace(std::make_pair(opcode, core::vector<char>()));
								goto fetch_payload;
							}
							break;
						}
						case bytecode::end:
						{
							size_t length = size;
							if (length > (size_t)remains)
								length = (size_t)remains;

							if (masked)
							{
								for (size_t i = 0; i < length; i++)
									data[i] ^= mask[masks++ % 4];
							}

							core::vector<char> message;
							text_assign(message, std::string_view(data, length));
							if (opcode == web_socket_op::next && !queue.empty())
							{
								auto& top = queue.front();
								top.second.insert(top.second.end(), message.begin(), message.end());
							}
							else
								queue.emplace(std::make_pair(opcode == web_socket_op::next ? web_socket_op::text : opcode, std::move(message)));

							opcode = web_socket_op::next;
							data += length;
							size -= length;
							remains -= length;
							if (remains == 0)
								goto fetch_payload;
							break;
						}
					}
				}

				return !queue.empty();
			fetch_payload:
				if (!control && !final)
					return !queue.empty();

				state = bytecode::begin;
				if (size > 0)
					goto parse_payload;

				return true;
			}
			bool web_codec::get_frame(web_socket_op* op, core::vector<char>* message)
			{
				VI_ASSERT(op != nullptr, "op should be set");
				VI_ASSERT(message != nullptr, "message should be set");

				if (queue.empty())
					return false;

				auto& base = queue.front();
				*message = std::move(base.second);
				*op = base.first;
				queue.pop();

				return true;
			}

			hrm_cache::hrm_cache() noexcept : hrm_cache(HTTP_HRM_SIZE)
			{
			}
			hrm_cache::hrm_cache(size_t max_bytes_storage) noexcept : capacity(max_bytes_storage), size(0)
			{
			}
			hrm_cache::~hrm_cache() noexcept
			{
				size = capacity = 0;
				while (!queue.empty())
				{
					auto* item = queue.front();
					core::memory::deinit(item);
					queue.pop();
				}
			}
			void hrm_cache::shrink_to_fit() noexcept
			{
				size_t freed = 0;
				while (!queue.empty() && size > capacity)
				{
					auto* item = queue.front();
					size_t bytes = item->capacity();
					size -= std::min<size_t>(size, bytes);
					freed += bytes;
					core::memory::deinit(item);
					queue.pop();
				}
				if (freed > 0)
					VI_DEBUG("http freed up %" PRIu64 " bytes from hrm cache", (uint64_t)freed);
			}
			void hrm_cache::shrink() noexcept
			{
				core::umutex<std::mutex> unique(mutex);
				shrink_to_fit();
			}
			void hrm_cache::rescale(size_t max_bytes_storage) noexcept
			{
				core::umutex<std::mutex> unique(mutex);
				capacity = max_bytes_storage;
				shrink_to_fit();
			}
			void hrm_cache::push(core::string* entry)
			{
				entry->clear();
				core::umutex<std::mutex> unique(mutex);
				size += entry->capacity();
				queue.push(entry);
				shrink_to_fit();
			}
			core::string* hrm_cache::pop() noexcept
			{
				core::umutex<std::mutex> unique(mutex);
				if (queue.empty())
					return core::memory::init<core::string>();

				auto* item = queue.front();
				size -= std::min<size_t>(size, item->capacity());
				queue.pop();
				return item;
			}

			void utils::update_keep_alive_headers(connection* base, core::string& content)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				auto* router = base->root->router;
				if (router->keep_alive_max_count < 0)
				{
					content.append("Connection: close\r\n");
					return;
				}

				auto connection = base->request.get_header("Connection");
				if ((!connection.empty() && !core::stringify::case_equals(connection, "keep-alive")) || (connection.empty() && strcmp(base->request.version, "1.1") != 0))
				{
					base->info.reuses = 1;
					content.append("Connection: close\r\n");
					return;
				}

				if (router->keep_alive_max_count == 0)
				{
					content.append("Connection: keep-alive\r\n");
					if (router->socket_timeout > 0)
					{
						char timeout[core::NUMSTR_SIZE];
						content.append("Keep-alive: timeout=");
						content.append(core::to_string_view(timeout, sizeof(timeout), router->socket_timeout / 1000));
						content.append("\r\n");
					}
					return;
				}

				if (base->info.reuses <= 1)
				{
					content.append("Connection: close\r\n");
					return;
				}

				char timeout[core::NUMSTR_SIZE];
				content.append("Connection: keep-alive\r\nKeep-alive: ");
				if (router->socket_timeout > 0)
				{
					content.append("timeout=");
					content.append(core::to_string_view(timeout, sizeof(timeout), router->socket_timeout / 1000));
					content.append(", ");
				}
				content.append("max=");
				content.append(core::to_string_view(timeout, sizeof(timeout), router->keep_alive_max_count));
				content.append("\r\n");
			}
			std::string_view utils::content_type(const std::string_view& path, core::vector<mime_type>* types)
			{
				static mime_static mime_types[] = { mime_static(".3dm", "x-world/x-3dmf"), mime_static(".3dmf", "x-world/x-3dmf"), mime_static(".a", "application/octet-stream"), mime_static(".aab", "application/x-authorware-bin"), mime_static(".aac", "audio/aac"), mime_static(".aam", "application/x-authorware-map"), mime_static(".aas", "application/x-authorware-seg"), mime_static(".aat", "application/font-sfnt"), mime_static(".abc", "text/vnd.abc"), mime_static(".acgi", "text/html"), mime_static(".afl", "video/animaflex"), mime_static(".ai", "application/postscript"), mime_static(".aif", "audio/x-aiff"), mime_static(".aifc", "audio/x-aiff"), mime_static(".aiff", "audio/x-aiff"), mime_static(".aim", "application/x-aim"), mime_static(".aip", "text/x-audiosoft-intra"), mime_static(".ani", "application/x-navi-animation"), mime_static(".aos", "application/x-nokia-9000-communicator-add-on-software"), mime_static(".aps", "application/mime"), mime_static(".arc", "application/octet-stream"), mime_static(".arj", "application/arj"), mime_static(".art", "image/x-jg"), mime_static(".asf", "video/x-ms-asf"), mime_static(".asm", "text/x-asm"), mime_static(".asp", "text/asp"), mime_static(".asx", "video/x-ms-asf"), mime_static(".au", "audio/x-au"), mime_static(".avi", "video/x-msvideo"), mime_static(".avs", "video/avs-video"), mime_static(".bcpio", "application/x-bcpio"), mime_static(".bin", "application/x-binary"), mime_static(".bm", "image/bmp"), mime_static(".bmp", "image/bmp"), mime_static(".boo", "application/book"), mime_static(".book", "application/book"), mime_static(".boz", "application/x-bzip2"), mime_static(".bsh", "application/x-bsh"), mime_static(".bz", "application/x-bzip"), mime_static(".bz2", "application/x-bzip2"), mime_static(".c", "text/x-c"), mime_static(".c++", "text/x-c"), mime_static(".cat", "application/vnd.ms-pki.seccat"), mime_static(".cc", "text/x-c"), mime_static(".ccad", "application/clariscad"), mime_static(".cco", "application/x-cocoa"), mime_static(".cdf", "application/x-cdf"), mime_static(".cer", "application/pkix-cert"), mime_static(".cff", "application/font-sfnt"), mime_static(".cha", "application/x-chat"), mime_static(".chat", "application/x-chat"), mime_static(".class", "application/x-java-class"), mime_static(".com", "application/octet-stream"), mime_static(".conf", "text/plain"), mime_static(".cpio", "application/x-cpio"), mime_static(".cpp", "text/x-c"), mime_static(".cpt", "application/x-compactpro"), mime_static(".crl", "application/pkcs-crl"), mime_static(".crt", "application/x-x509-user-cert"), mime_static(".csh", "text/x-script.csh"), mime_static(".css", "text/css"), mime_static(".csv", "text/csv"), mime_static(".cxx", "text/plain"), mime_static(".dcr", "application/x-director"), mime_static(".deepv", "application/x-deepv"), mime_static(".def", "text/plain"), mime_static(".der", "application/x-x509-ca-cert"), mime_static(".dif", "video/x-dv"), mime_static(".dir", "application/x-director"), mime_static(".dl", "video/x-dl"), mime_static(".dll", "application/octet-stream"), mime_static(".doc", "application/msword"), mime_static(".dot", "application/msword"), mime_static(".dp", "application/commonground"), mime_static(".drw", "application/drafting"), mime_static(".dump", "application/octet-stream"), mime_static(".dv", "video/x-dv"), mime_static(".dvi", "application/x-dvi"), mime_static(".dwf", "model/vnd.dwf"), mime_static(".dwg", "image/vnd.dwg"), mime_static(".dxf", "image/vnd.dwg"), mime_static(".dxr", "application/x-director"), mime_static(".el", "text/x-script.elisp"), mime_static(".elc", "application/x-bytecode.elisp"), mime_static(".env", "application/x-envoy"), mime_static(".eps", "application/postscript"), mime_static(".es", "application/x-esrehber"), mime_static(".etx", "text/x-setext"), mime_static(".evy", "application/x-envoy"), mime_static(".exe", "application/octet-stream"), mime_static(".f", "text/x-fortran"), mime_static(".f77", "text/x-fortran"), mime_static(".f90", "text/x-fortran"), mime_static(".fdf", "application/vnd.fdf"), mime_static(".fif", "image/fif"), mime_static(".fli", "video/x-fli"), mime_static(".flo", "image/florian"), mime_static(".flx", "text/vnd.fmi.flexstor"), mime_static(".fmf", "video/x-atomic3d-feature"), mime_static(".for", "text/x-fortran"), mime_static(".fpx", "image/vnd.fpx"), mime_static(".frl", "application/freeloader"), mime_static(".funk", "audio/make"), mime_static(".g", "text/plain"), mime_static(".g3", "image/g3fax"), mime_static(".gif", "image/gif"), mime_static(".gl", "video/x-gl"), mime_static(".gsd", "audio/x-gsm"), mime_static(".gsm", "audio/x-gsm"), mime_static(".gsp", "application/x-gsp"), mime_static(".gss", "application/x-gss"), mime_static(".gtar", "application/x-gtar"), mime_static(".gz", "application/x-gzip"), mime_static(".h", "text/x-h"), mime_static(".hdf", "application/x-hdf"), mime_static(".help", "application/x-helpfile"), mime_static(".hgl", "application/vnd.hp-hpgl"), mime_static(".hh", "text/x-h"), mime_static(".hlb", "text/x-script"), mime_static(".hlp", "application/x-helpfile"), mime_static(".hpg", "application/vnd.hp-hpgl"), mime_static(".hpgl", "application/vnd.hp-hpgl"), mime_static(".hqx", "application/binhex"), mime_static(".hta", "application/hta"), mime_static(".htc", "text/x-component"), mime_static(".htm", "text/html"), mime_static(".html", "text/html"), mime_static(".htmls", "text/html"), mime_static(".htt", "text/webviewhtml"), mime_static(".htx", "text/html"), mime_static(".ice", "x-conference/x-cooltalk"), mime_static(".ico", "image/x-icon"), mime_static(".idc", "text/plain"), mime_static(".ief", "image/ief"), mime_static(".iefs", "image/ief"), mime_static(".iges", "model/iges"), mime_static(".igs", "model/iges"), mime_static(".ima", "application/x-ima"), mime_static(".imap", "application/x-httpd-imap"), mime_static(".inf", "application/inf"), mime_static(".ins", "application/x-internett-signup"), mime_static(".ip", "application/x-ip2"), mime_static(".isu", "video/x-isvideo"), mime_static(".it", "audio/it"), mime_static(".iv", "application/x-inventor"), mime_static(".ivr", "i-world/i-vrml"), mime_static(".ivy", "application/x-livescreen"), mime_static(".jam", "audio/x-jam"), mime_static(".jav", "text/x-java-source"), mime_static(".java", "text/x-java-source"), mime_static(".jcm", "application/x-java-commerce"), mime_static(".jfif", "image/jpeg"), mime_static(".jfif-tbnl", "image/jpeg"), mime_static(".jpe", "image/jpeg"), mime_static(".jpeg", "image/jpeg"), mime_static(".jpg", "image/jpeg"), mime_static(".jpm", "image/jpm"), mime_static(".jps", "image/x-jps"), mime_static(".jpx", "image/jpx"), mime_static(".js", "application/x-javascript"), mime_static(".json", "application/json"), mime_static(".jut", "image/jutvision"), mime_static(".kar", "music/x-karaoke"), mime_static(".kml", "application/vnd.google-earth.kml+xml"), mime_static(".kmz", "application/vnd.google-earth.kmz"), mime_static(".ksh", "text/x-script.ksh"), mime_static(".la", "audio/x-nspaudio"), mime_static(".lam", "audio/x-liveaudio"), mime_static(".latex", "application/x-latex"), mime_static(".lha", "application/x-lha"), mime_static(".lhx", "application/octet-stream"), mime_static(".lib", "application/octet-stream"), mime_static(".list", "text/plain"), mime_static(".lma", "audio/x-nspaudio"), mime_static(".log", "text/plain"), mime_static(".lsp", "text/x-script.lisp"), mime_static(".lst", "text/plain"), mime_static(".lsx", "text/x-la-asf"), mime_static(".ltx", "application/x-latex"), mime_static(".lzh", "application/x-lzh"), mime_static(".lzx", "application/x-lzx"), mime_static(".m", "text/x-m"), mime_static(".m1v", "video/mpeg"), mime_static(".m2a", "audio/mpeg"), mime_static(".m2v", "video/mpeg"), mime_static(".m3u", "audio/x-mpegurl"), mime_static(".m4v", "video/x-m4v"), mime_static(".man", "application/x-troff-man"), mime_static(".map", "application/x-navimap"), mime_static(".mar", "text/plain"), mime_static(".mbd", "application/mbedlet"), mime_static(".mc$", "application/x-magic-cap-package-1.0"), mime_static(".mcd", "application/x-mathcad"), mime_static(".mcf", "text/mcf"), mime_static(".mcp", "application/netmc"), mime_static(".me", "application/x-troff-me"), mime_static(".mht", "message/rfc822"), mime_static(".mhtml", "message/rfc822"), mime_static(".mid", "audio/x-midi"), mime_static(".midi", "audio/x-midi"), mime_static(".mif", "application/x-mif"), mime_static(".mime", "www/mime"), mime_static(".mjf", "audio/x-vnd.audioexplosion.mjuicemediafile"), mime_static(".mjpg", "video/x-motion-jpeg"), mime_static(".mm", "application/base64"), mime_static(".mme", "application/base64"), mime_static(".mod", "audio/x-mod"), mime_static(".moov", "video/quicktime"), mime_static(".mov", "video/quicktime"), mime_static(".movie", "video/x-sgi-movie"), mime_static(".mp2", "video/x-mpeg"), mime_static(".mp3", "audio/x-mpeg-3"), mime_static(".mp4", "video/mp4"), mime_static(".mpa", "audio/mpeg"), mime_static(".mpc", "application/x-project"), mime_static(".mpeg", "video/mpeg"), mime_static(".mpg", "video/mpeg"), mime_static(".mpga", "audio/mpeg"), mime_static(".mpp", "application/vnd.ms-project"), mime_static(".mpt", "application/x-project"), mime_static(".mpv", "application/x-project"), mime_static(".mpx", "application/x-project"), mime_static(".mrc", "application/marc"), mime_static(".ms", "application/x-troff-ms"), mime_static(".mv", "video/x-sgi-movie"), mime_static(".my", "audio/make"), mime_static(".mzz", "application/x-vnd.audioexplosion.mzz"), mime_static(".nap", "image/naplps"), mime_static(".naplps", "image/naplps"), mime_static(".nc", "application/x-netcdf"), mime_static(".ncm", "application/vnd.nokia.configuration-message"), mime_static(".nif", "image/x-niff"), mime_static(".niff", "image/x-niff"), mime_static(".nix", "application/x-mix-transfer"), mime_static(".nsc", "application/x-conference"), mime_static(".nvd", "application/x-navidoc"), mime_static(".o", "application/octet-stream"), mime_static(".obj", "application/octet-stream"), mime_static(".oda", "application/oda"), mime_static(".oga", "audio/ogg"), mime_static(".ogg", "audio/ogg"), mime_static(".ogv", "video/ogg"), mime_static(".omc", "application/x-omc"), mime_static(".omcd", "application/x-omcdatamaker"), mime_static(".omcr", "application/x-omcregerator"), mime_static(".otf", "application/font-sfnt"), mime_static(".p", "text/x-pascal"), mime_static(".p10", "application/x-pkcs10"), mime_static(".p12", "application/x-pkcs12"), mime_static(".p7a", "application/x-pkcs7-signature"), mime_static(".p7c", "application/x-pkcs7-mime"), mime_static(".p7m", "application/x-pkcs7-mime"), mime_static(".p7r", "application/x-pkcs7-certreqresp"), mime_static(".p7s", "application/pkcs7-signature"), mime_static(".part", "application/pro_eng"), mime_static(".pas", "text/x-pascal"), mime_static(".pbm", "image/x-portable-bitmap"), mime_static(".pcl", "application/vnd.hp-pcl"), mime_static(".pct", "image/x-pct"), mime_static(".pcx", "image/x-pcx"), mime_static(".pq", "chemical/x-pq"), mime_static(".pdf", "application/pdf"), mime_static(".pfr", "application/font-tdpfr"), mime_static(".pfunk", "audio/make"), mime_static(".pgm", "image/x-portable-greymap"), mime_static(".pic", "image/pict"), mime_static(".pict", "image/pict"), mime_static(".pkg", "application/x-newton-compatible-pkg"), mime_static(".pko", "application/vnd.ms-pki.pko"), mime_static(".pl", "text/x-script.perl"), mime_static(".plx", "application/x-pixelscript"), mime_static(".pm", "text/x-script.perl-module"), mime_static(".pm4", "application/x-pagemaker"), mime_static(".pm5", "application/x-pagemaker"), mime_static(".png", "image/png"), mime_static(".pnm", "image/x-portable-anymap"), mime_static(".pot", "application/vnd.ms-powerpoint"), mime_static(".pov", "model/x-pov"), mime_static(".ppa", "application/vnd.ms-powerpoint"), mime_static(".ppm", "image/x-portable-pixmap"), mime_static(".pps", "application/vnd.ms-powerpoint"), mime_static(".ppt", "application/vnd.ms-powerpoint"), mime_static(".ppz", "application/vnd.ms-powerpoint"), mime_static(".pre", "application/x-freelance"), mime_static(".prt", "application/pro_eng"), mime_static(".ps", "application/postscript"), mime_static(".psd", "application/octet-stream"), mime_static(".pvu", "paleovu/x-pv"), mime_static(".pwz", "application/vnd.ms-powerpoint"), mime_static(".py", "text/x-script.python"), mime_static(".pyc", "application/x-bytecode.python"), mime_static(".qcp", "audio/vnd.qcelp"), mime_static(".qd3", "x-world/x-3dmf"), mime_static(".qd3d", "x-world/x-3dmf"), mime_static(".qif", "image/x-quicktime"), mime_static(".qt", "video/quicktime"), mime_static(".qtc", "video/x-qtc"), mime_static(".qti", "image/x-quicktime"), mime_static(".qtif", "image/x-quicktime"), mime_static(".ra", "audio/x-pn-realaudio"), mime_static(".ram", "audio/x-pn-realaudio"), mime_static(".rar", "application/x-arj-compressed"), mime_static(".ras", "image/x-cmu-raster"), mime_static(".rast", "image/cmu-raster"), mime_static(".rexx", "text/x-script.rexx"), mime_static(".rf", "image/vnd.rn-realflash"), mime_static(".rgb", "image/x-rgb"), mime_static(".rm", "audio/x-pn-realaudio"), mime_static(".rmi", "audio/mid"), mime_static(".rmm", "audio/x-pn-realaudio"), mime_static(".rmp", "audio/x-pn-realaudio"), mime_static(".rng", "application/vnd.nokia.ringing-tone"), mime_static(".rnx", "application/vnd.rn-realplayer"), mime_static(".roff", "application/x-troff"), mime_static(".rp", "image/vnd.rn-realpix"), mime_static(".rpm", "audio/x-pn-realaudio-plugin"), mime_static(".rt", "text/vnd.rn-realtext"), mime_static(".rtf", "application/x-rtf"), mime_static(".rtx", "application/x-rtf"), mime_static(".rv", "video/vnd.rn-realvideo"), mime_static(".s", "text/x-asm"), mime_static(".s3m", "audio/s3m"), mime_static(".saveme", "application/octet-stream"), mime_static(".sbk", "application/x-tbook"), mime_static(".scm", "text/x-script.scheme"), mime_static(".sdml", "text/plain"), mime_static(".sdp", "application/x-sdp"), mime_static(".sdr", "application/sounder"), mime_static(".sea", "application/x-sea"), mime_static(".set", "application/set"), mime_static(".sgm", "text/x-sgml"), mime_static(".sgml", "text/x-sgml"), mime_static(".sh", "text/x-script.sh"), mime_static(".shar", "application/x-shar"), mime_static(".shtm", "text/html"), mime_static(".shtml", "text/html"), mime_static(".sid", "audio/x-psid"), mime_static(".sil", "application/font-sfnt"), mime_static(".sit", "application/x-sit"), mime_static(".skd", "application/x-koan"), mime_static(".skm", "application/x-koan"), mime_static(".skp", "application/x-koan"), mime_static(".skt", "application/x-koan"), mime_static(".sl", "application/x-seelogo"), mime_static(".smi", "application/smil"), mime_static(".smil", "application/smil"), mime_static(".snd", "audio/x-adpcm"), mime_static(".so", "application/octet-stream"), mime_static(".sol", "application/solids"), mime_static(".spc", "text/x-speech"), mime_static(".spl", "application/futuresplash"), mime_static(".spr", "application/x-sprite"), mime_static(".sprite", "application/x-sprite"), mime_static(".src", "application/x-wais-source"), mime_static(".ssi", "text/x-server-parsed-html"), mime_static(".ssm", "application/streamingmedia"), mime_static(".sst", "application/vnd.ms-pki.certstore"), mime_static(".step", "application/step"), mime_static(".stl", "application/vnd.ms-pki.stl"), mime_static(".stp", "application/step"), mime_static(".sv4cpio", "application/x-sv4cpio"), mime_static(".sv4crc", "application/x-sv4crc"), mime_static(".svf", "image/x-dwg"), mime_static(".svg", "image/svg+xml"), mime_static(".svr", "x-world/x-svr"), mime_static(".swf", "application/x-shockwave-flash"), mime_static(".t", "application/x-troff"), mime_static(".talk", "text/x-speech"), mime_static(".tar", "application/x-tar"), mime_static(".tbk", "application/x-tbook"), mime_static(".tcl", "text/x-script.tcl"), mime_static(".tcsh", "text/x-script.tcsh"), mime_static(".tex", "application/x-tex"), mime_static(".texi", "application/x-texinfo"), mime_static(".texinfo", "application/x-texinfo"), mime_static(".text", "text/plain"), mime_static(".tgz", "application/x-compressed"), mime_static(".tif", "image/x-tiff"), mime_static(".tiff", "image/x-tiff"), mime_static(".torrent", "application/x-bittorrent"), mime_static(".tr", "application/x-troff"), mime_static(".tsi", "audio/tsp-audio"), mime_static(".tsp", "audio/tsplayer"), mime_static(".tsv", "text/tab-separated-values"), mime_static(".ttf", "application/font-sfnt"), mime_static(".turbot", "image/florian"), mime_static(".txt", "text/plain"), mime_static(".uil", "text/x-uil"), mime_static(".uni", "text/uri-list"), mime_static(".unis", "text/uri-list"), mime_static(".unv", "application/i-deas"), mime_static(".uri", "text/uri-list"), mime_static(".uris", "text/uri-list"), mime_static(".ustar", "application/x-ustar"), mime_static(".uu", "text/x-uuencode"), mime_static(".uue", "text/x-uuencode"), mime_static(".vcd", "application/x-cdlink"), mime_static(".vcs", "text/x-vcalendar"), mime_static(".vda", "application/vda"), mime_static(".vdo", "video/vdo"), mime_static(".vew", "application/groupwise"), mime_static(".viv", "video/vnd.vivo"), mime_static(".vivo", "video/vnd.vivo"), mime_static(".vmd", "application/vocaltec-media-desc"), mime_static(".vmf", "application/vocaltec-media-resource"), mime_static(".voc", "audio/x-voc"), mime_static(".vos", "video/vosaic"), mime_static(".vox", "audio/voxware"), mime_static(".vqe", "audio/x-twinvq-plugin"), mime_static(".vqf", "audio/x-twinvq"), mime_static(".vql", "audio/x-twinvq-plugin"), mime_static(".vrml", "model/vrml"), mime_static(".vrt", "x-world/x-vrt"), mime_static(".vsd", "application/x-visio"), mime_static(".vst", "application/x-visio"), mime_static(".vsw", "application/x-visio"), mime_static(".w60", "application/wordperfect6.0"), mime_static(".w61", "application/wordperfect6.1"), mime_static(".w6w", "application/msword"), mime_static(".wav", "audio/x-wav"), mime_static(".wb1", "application/x-qpro"), mime_static(".wbmp", "image/vnd.wap.wbmp"), mime_static(".web", "application/vnd.xara"), mime_static(".webm", "video/webm"), mime_static(".webp", "image/webp"), mime_static(".wiz", "application/msword"), mime_static(".wk1", "application/x-123"), mime_static(".wmf", "windows/metafile"), mime_static(".wml", "text/vnd.wap.wml"), mime_static(".wmlc", "application/vnd.wap.wmlc"), mime_static(".wmls", "text/vnd.wap.wmlscript"), mime_static(".wmlsc", "application/vnd.wap.wmlscriptc"), mime_static(".woff", "application/font-woff"), mime_static(".word", "application/msword"), mime_static(".wp", "application/wordperfect"), mime_static(".wp5", "application/wordperfect"), mime_static(".wp6", "application/wordperfect"), mime_static(".wpd", "application/wordperfect"), mime_static(".wq1", "application/x-lotus"), mime_static(".wri", "application/x-wri"), mime_static(".wrl", "model/vrml"), mime_static(".wrz", "model/vrml"), mime_static(".wsc", "text/scriplet"), mime_static(".wsrc", "application/x-wais-source"), mime_static(".wtk", "application/x-wintalk"), mime_static(".x-png", "image/png"), mime_static(".xbm", "image/x-xbm"), mime_static(".xdr", "video/x-amt-demorun"), mime_static(".xgz", "xgl/drawing"), mime_static(".xhtml", "application/xhtml+xml"), mime_static(".xif", "image/vnd.xiff"), mime_static(".xl", "application/vnd.ms-excel"), mime_static(".xla", "application/vnd.ms-excel"), mime_static(".xlb", "application/vnd.ms-excel"), mime_static(".xlc", "application/vnd.ms-excel"), mime_static(".xld", "application/vnd.ms-excel"), mime_static(".xlk", "application/vnd.ms-excel"), mime_static(".xll", "application/vnd.ms-excel"), mime_static(".xlm", "application/vnd.ms-excel"), mime_static(".xls", "application/vnd.ms-excel"), mime_static(".xlt", "application/vnd.ms-excel"), mime_static(".xlv", "application/vnd.ms-excel"), mime_static(".xlw", "application/vnd.ms-excel"), mime_static(".xm", "audio/xm"), mime_static(".xml", "text/xml"), mime_static(".xmz", "xgl/movie"), mime_static(".xpix", "application/x-vnd.ls-xpix"), mime_static(".xpm", "image/x-xpixmap"), mime_static(".xsl", "application/xml"), mime_static(".xslt", "application/xml"), mime_static(".xsr", "video/x-amt-showrun"), mime_static(".xwd", "image/x-xwd"), mime_static(".xyz", "chemical/x-pq"), mime_static(".z", "application/x-compressed"), mime_static(".zip", "application/x-zip-compressed"), mime_static(".zoo", "application/octet-stream"), mime_static(".zsh", "text/x-script.zsh") };

				size_t path_length = path.size();
				while (path_length >= 1 && path[path_length - 1] != '.')
					path_length--;

				if (!path_length)
					return "application/octet-stream";

				const char* ptr = path.data();
				const char* ext = &ptr[path_length - 1];
				int end = ((int)(sizeof(mime_types) / sizeof(mime_types[0])));
				int start = 0, result, index;

				while (end - start > 1)
				{
					index = (start + end) >> 1;
					result = core::stringify::case_compare(ext, mime_types[index].extension);
					if (result == 0)
						return mime_types[index].type;
					else if (result < 0)
						end = index;
					else
						start = index;
				}

				if (core::stringify::case_equals(ext, mime_types[start].extension))
					return mime_types[start].type;

				if (types != nullptr && !types->empty())
				{
					for (auto& item : *types)
					{
						if (core::stringify::case_equals(ext, item.extension.c_str()))
							return item.type.c_str();
					}
				}

				return "application/octet-stream";
			}
			std::string_view utils::status_message(int status_code)
			{
				switch (status_code)
				{
					case 100:
						return "Continue";
					case 101:
						return "Switching Protocols";
					case 102:
						return "Processing";
					case 200:
						return "OK";
					case 201:
						return "Created";
					case 202:
						return "Accepted";
					case 203:
						return "Non-Authoritative Information";
					case 204:
						return "No Content";
					case 205:
						return "Reset Content";
					case 206:
						return "Partial Content";
					case 207:
						return "Multi-Status";
					case 208:
						return "Already Reported";
					case 226:
						return "IM Used";
					case 218:
						return "This is fine";
					case 300:
						return "Multiple Choices";
					case 301:
						return "Moved Permanently";
					case 302:
						return "Found";
					case 303:
						return "See Other";
					case 304:
						return "Not Modified";
					case 305:
						return "Use Proxy";
					case 307:
						return "Temporary Redirect";
					case 308:
						return "Permanent Redirect";
					case 400:
						return "Bad Request";
					case 401:
						return "Unauthorized";
					case 402:
						return "Payment Required";
					case 403:
						return "Forbidden";
					case 404:
						return "Not Found";
					case 405:
						return "Method Not Allowed";
					case 406:
						return "Not Acceptable";
					case 407:
						return "Proxy authentication Required";
					case 408:
						return "Request time-out";
					case 409:
						return "Conflict";
					case 410:
						return "Gone";
					case 411:
						return "Length Required";
					case 412:
						return "Precondition Failed";
					case 413:
						return "Request entity too Large";
					case 414:
						return "Request URL too Large";
					case 415:
						return "Unsupported media Type";
					case 416:
						return "Requested range not Satisfiable";
					case 417:
						return "Expectation Failed";
					case 418:
						return "I'm a teapot";
					case 419:
						return "Authentication Timeout";
					case 420:
						return "Enhance your Calm";
					case 421:
						return "Misdirected Request";
					case 422:
						return "Unproccessable entity";
					case 423:
						return "Locked";
					case 424:
						return "Failed Dependency";
					case 426:
						return "Upgrade Required";
					case 428:
						return "Precondition Required";
					case 429:
						return "Too many Requests";
					case 431:
						return "Request header fields too Large";
					case 440:
						return "Login Timeout";
					case 451:
						return "Unavailable destination legal Reasons";
					case 500:
						return "Internal server Error";
					case 501:
						return "Not Implemented";
					case 502:
						return "Bad Gateway";
					case 503:
						return "Service Unavailable";
					case 504:
						return "Gateway Timeout";
					case 505:
						return "Version not Supported";
					case 506:
						return "Variant also Negotiates";
					case 507:
						return "Insufficient Storage";
					case 508:
						return "Loop Detected";
					case 509:
						return "Bandwidth limit Exceeded";
					case 510:
						return "Not Extended";
					case 511:
						return "Network authentication Required";
					default:
						if (status_code >= 100 && status_code < 200)
							return "Informational";

						if (status_code >= 200 && status_code < 300)
							return "Success";

						if (status_code >= 300 && status_code < 400)
							return "Redirection";

						if (status_code >= 400 && status_code < 500)
							return "Client Error";

						if (status_code >= 500 && status_code < 600)
							return "Server Error";
						break;
				}

				return "Unknown";
			}

			void paths::construct_path(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				auto* route = base->route;
				if (!route->alias.empty())
				{
					base->request.path.assign(route->alias);
					if (route->router->callbacks.on_location)
						route->router->callbacks.on_location(base);
					return;
				}
				else if (route->files_directory.empty())
					return;

				for (size_t i = 0; i < base->request.location.size(); i++)
				{
					if (base->request.location[i] == '%' && i + 1 < base->request.location.size())
					{
						if (base->request.location[i + 1] == 'u')
						{
							int value = 0;
							if (compute::codec::hex_to_decimal(base->request.location, i + 2, 4, value))
							{
								char buffer[4];
								size_t lcount = compute::codec::utf8(value, buffer);
								if (lcount > 0)
									base->request.path.append(buffer, lcount);
								i += 5;
							}
							else
								base->request.path += base->request.location[i];
						}
						else
						{
							int value = 0;
							if (compute::codec::hex_to_decimal(base->request.location, i + 1, 2, value))
							{
								base->request.path += value;
								i += 2;
							}
							else
								base->request.path += base->request.location[i];
						}
					}
					else if (base->request.location[i] == '+')
						base->request.path += ' ';
					else
						base->request.path += base->request.location[i];
				}

				char* buffer = (char*)base->request.path.c_str();
				char* next = buffer;
				while (buffer[0] == '.' && buffer[1] == '.')
					buffer++;

				while (*buffer != '\0')
				{
					*next++ = *buffer++;
					if (buffer[-1] != '/' && buffer[-1] != '\\')
						continue;

					while (buffer[0] != '\0')
					{
						if (buffer[0] == '/' || buffer[0] == '\\')
							buffer++;
						else if (buffer[0] == '.' && buffer[1] == '.')
							buffer += 2;
						else
							break;
					}
				}

				int64_t size = buffer - next;
				if (size > 0 && size != (int64_t)base->request.path.size())
					base->request.path.resize(base->request.path.size() - (size_t)size);

				if (base->request.path.size() > 1 && !base->request.match.empty())
				{
					auto& match = base->request.match.get()[0];
					size_t start = std::min<size_t>(base->request.path.size(), (size_t)match.start);
					size_t end = std::min<size_t>(base->request.path.size(), (size_t)match.end);
					core::stringify::remove_part(base->request.path, start, end);
				}
#ifdef VI_MICROSOFT
				core::stringify::replace(base->request.path, '/', '\\');
#endif
				base->request.path = route->files_directory + base->request.path;
				auto path = core::os::path::resolve(base->request.path.c_str());
				if (path)
					base->request.path = *path;

				bool path_trailing = path_trailing_check(base->request.path);
				bool location_trailing = path_trailing_check(base->request.location);
				if (path_trailing != location_trailing)
				{
					if (location_trailing)
						base->request.path.append(1, VI_SPLITTER);
					else
						base->request.path.erase(base->request.path.size() - 1, 1);
				}

				if (route->router->callbacks.on_location)
					route->router->callbacks.on_location(base);
			}
			void paths::construct_head_full(request_frame* request, response_frame* response, bool is_request, core::string& buffer)
			{
				VI_ASSERT(request != nullptr, "connection should be set");
				VI_ASSERT(response != nullptr, "response should be set");

				kimv_unordered_map& headers = (is_request ? request->headers : response->headers);
				for (auto& item : headers)
				{
					for (auto& payload : item.second)
						buffer.append(item.first).append(": ").append(payload).append("\r\n");
				}

				if (is_request)
					return;

				for (auto& item : response->cookies)
				{
					if (item.name.empty())
						continue;

					buffer.append("Set-Cookie: ").append(item.name).append("=").append(item.value);
					if (!item.expires.empty())
						buffer.append("; expires=").append(item.expires);
					if (!item.domain.empty())
						buffer.append("; domain=").append(item.domain);
					if (!item.path.empty())
						buffer.append("; path=").append(item.path);
					if (!item.same_site.empty())
						buffer.append("; same_site=").append(item.same_site);
					if (item.secure)
						buffer.append("; SameSite");
					if (item.http_only)
						buffer.append("; HttpOnly");
					buffer.append("\r\n");
				}
			}
			void paths::construct_head_cache(connection* base, core::string& buffer)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				if (!base->route->static_file_max_age)
					return construct_head_uncache(buffer);

				buffer.append("Cache-Control: max-age=");
				buffer.append(core::to_string(base->route->static_file_max_age));
				buffer.append("\r\n");
			}
			void paths::construct_head_uncache(core::string& buffer)
			{
				buffer.append(
					"Cache-Control: no-cache, no-store, must-revalidate, private, max-age=0\r\n"
					"Pragma: no-cache\r\n"
					"Expires: 0\r\n", 102);
			}
			bool paths::construct_route(map_router* router, connection* base)
			{
				VI_ASSERT(base != nullptr, "connection should be set");
				VI_ASSERT(router != nullptr && router->base != nullptr, "router should be valid");
				if (base->request.location.empty() || base->request.location.front() != '/')
					return false;

				if (router->listeners.size() > 1)
				{
					auto* host = base->request.get_header_blob("Host");
					if (!host)
						return false;

					auto listen = router->listeners.find(*host);
					if (listen == router->listeners.end() && router->listeners.find("*") == router->listeners.end())
						return false;
				}
				else
				{
					auto listen = router->listeners.begin();
					if (listen->first.size() != 1 || listen->first.front() != '*')
					{
						auto* host = base->request.get_header_blob("Host");
						if (!host || listen->first != *host)
							return false;
					}
				}

				base->request.referrer = base->request.location;
				if (base->request.location.size() == 1)
				{
					base->route = router->base;
					return true;
				}

				for (auto& group : router->groups)
				{
					core::string& location = base->request.location;
					if (!group->match.empty())
					{
						if (group->mode == route_mode::exact)
						{
							if (location != group->match)
								continue;

							location.clear();
						}
						else if (group->mode == route_mode::start)
						{
							if (!core::stringify::starts_with(location, group->match))
								continue;

							location.erase(location.begin(), location.begin() + group->match.size());
						}
						else if (group->mode == route_mode::match)
						{
							if (!core::stringify::find(location, group->match).found)
								continue;
						}
						else if (group->mode == route_mode::end)
						{
							if (!core::stringify::ends_with(location, group->match))
								continue;

							location.erase(location.end() - group->match.size(), location.end());
						}

						if (location.empty())
							location.append(1, '/');
						else if (location.front() != '/')
							location.insert(location.begin(), '/');

						for (auto* next : group->routes)
						{
							VI_ASSERT(next != nullptr, "route should be set");
							if (compute::regex::match(&next->location, base->request.match, location))
							{
								base->route = next;
								return true;
							}
						}

						location.assign(base->request.referrer);
					}
					else
					{
						for (auto* next : group->routes)
						{
							VI_ASSERT(next != nullptr, "route should be set");
							if (compute::regex::match(&next->location, base->request.match, location))
							{
								base->route = next;
								return true;
							}
						}
					}
				}

				base->route = router->base;
				return true;
			}
			bool paths::construct_directory_entries(connection* base, const core::string& name_a, const core::file_entry& a, const core::string& name_b, const core::file_entry& b)
			{
				VI_ASSERT(base != nullptr, "connection should be set");
				if (a.is_directory && !b.is_directory)
					return true;

				if (!a.is_directory && b.is_directory)
					return false;

				const char* query = (base->request.query.empty() ? nullptr : base->request.query.c_str());
				if (query != nullptr)
				{
					int result = 0;
					if (*query == 'n')
						result = strcmp(name_a.c_str(), name_b.c_str());
					else if (*query == 's')
						result = (a.size == b.size) ? 0 : ((a.size > b.size) ? 1 : -1);
					else if (*query == 'd')
						result = (a.last_modified == b.last_modified) ? 0 : ((a.last_modified > b.last_modified) ? 1 : -1);

					if (query[1] == 'a')
						return result < 0;
					else if (query[1] == 'd')
						return result > 0;

					return result < 0;
				}

				return strcmp(name_a.c_str(), name_b.c_str()) < 0;
			}
			core::string paths::construct_content_range(size_t offset, size_t length, size_t content_length)
			{
				core::string field = "bytes ";
				field += core::to_string(offset);
				field += '-';
				field += core::to_string(offset + length);
				field += '/';
				field += core::to_string(content_length);

				return field;
			}

			bool parsing::parse_multipart_header_field(parser* parser, const uint8_t* name, size_t length)
			{
				return parse_header_field(parser, name, length);
			}
			bool parsing::parse_multipart_header_value(parser* parser, const uint8_t* data, size_t length)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				VI_ASSERT(data != nullptr, "data should be set");

				if (!length || parser->multipart.skip)
					return true;

				if (parser->message.header.empty())
					return true;

				std::string_view value = std::string_view((char*)data, length);
				if (parser->message.header == "Content-Disposition")
				{
					core::text_settle start = core::stringify::find(value, "name=\"");
					if (start.found)
					{
						core::text_settle end = core::stringify::find(value, '\"', start.end);
						if (end.found)
							parser->multipart.data.key = value.substr(start.end, end.end - start.end - 1);
					}

					start = core::stringify::find(value, "filename=\"");
					if (start.found)
					{
						core::text_settle end = core::stringify::find(value, '\"', start.end);
						if (end.found)
						{
							auto name = value.substr(start.end, end.end - start.end - 1);
							parser->multipart.data.name = core::os::path::get_filename(name);
						}
					}
				}
				else if (parser->message.header == "Content-Type")
					parser->multipart.data.type = value;

				parser->multipart.data.set_header(parser->message.header.c_str(), value);
				parser->message.header.clear();
				return true;
			}
			bool parsing::parse_multipart_content_data(parser* parser, const uint8_t* data, size_t length)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				VI_ASSERT(data != nullptr, "data should be set");
				if (!length)
					return true;

				if (parser->multipart.skip || !parser->multipart.stream)
					return false;

				if (fwrite(data, 1, (size_t)length, parser->multipart.stream) != (size_t)length)
					return false;

				parser->multipart.data.length += length;
				return true;
			}
			bool parsing::parse_multipart_resource_begin(parser* parser)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				if (parser->multipart.skip || !parser->message.content)
					return true;

				if (parser->multipart.stream != nullptr)
				{
					core::os::file::close(parser->multipart.stream);
					parser->multipart.stream = nullptr;
					return false;
				}

				if (parser->message.content->resources.size() >= parser->multipart.max_resources)
				{
					parser->multipart.finish = true;
					return false;
				}

				parser->message.header.clear();
				parser->multipart.data.headers.clear();
				parser->multipart.data.name.clear();
				parser->multipart.data.type = "application/octet-stream";
				parser->multipart.data.is_in_memory = false;
				parser->multipart.data.length = 0;

				if (parser->multipart.temporary_directory != nullptr)
				{
					parser->multipart.data.path = *parser->multipart.temporary_directory;
					if (parser->multipart.data.path.back() != '/' && parser->multipart.data.path.back() != '\\')
						parser->multipart.data.path.append(1, VI_SPLITTER);

					auto random = compute::crypto::random_bytes(16);
					if (random)
					{
						auto hash = compute::crypto::hash_hex(compute::digests::md5(), *random);
						if (hash)
							parser->multipart.data.path.append(*hash);
					}
				}

				auto file = core::os::file::open(parser->multipart.data.path.c_str(), "wb");
				if (!file)
					return false;

				parser->multipart.stream = *file;
				return true;
			}
			bool parsing::parse_multipart_resource_end(parser* parser)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				if (parser->multipart.skip || !parser->multipart.stream || !parser->message.content)
					return true;

				core::os::file::close(parser->multipart.stream);
				parser->multipart.stream = nullptr;
				parser->message.content->resources.push_back(parser->multipart.data);

				if (parser->multipart.callback)
					parser->multipart.callback(&parser->message.content->resources.back());

				return true;
			}
			bool parsing::parse_header_field(parser* parser, const uint8_t* name, size_t length)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				VI_ASSERT(name != nullptr, "name should be set");
				if (length > 0)
					parser->message.header.assign((char*)name, length);
				return true;
			}
			bool parsing::parse_header_value(parser* parser, const uint8_t* data, size_t length)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				VI_ASSERT(data != nullptr, "data should be set");
				if (!length || parser->message.header.empty())
					return true;

				if (core::stringify::case_equals(parser->message.header.c_str(), "cookie"))
				{
					if (!parser->message.cookies)
						goto success;

					core::vector<std::pair<core::string, core::string>> cookies;
					const uint8_t* offset = data;

					for (size_t i = 0; i < length; i++)
					{
						if (data[i] != '=')
							continue;

						core::string name((char*)offset, (size_t)((data + i) - offset));
						size_t set = i;

						while (i + 1 < length && data[i] != ';')
							i++;

						if (data[i] == ';')
							i--;

						cookies.emplace_back(std::make_pair(std::move(name), core::string((char*)data + set + 1, i - set)));
						offset = data + (i + 3);
					}

					for (auto&& item : cookies)
					{
						auto& cookie = (*parser->message.cookies)[item.first];
						cookie.emplace_back(std::move(item.second));
					}
				}
				else if (parser->message.headers != nullptr)
				{
					auto& source = (*parser->message.headers)[parser->message.header]; bool append_field = true;
					if (!core::stringify::case_equals(parser->message.header.c_str(), "user-agent"))
					{
						if (parser->message.header.find(',') != std::string::npos)
						{
							append_field = false;
							core::stringify::pm_split(source, std::string_view((char*)data, length), ',');
							for (auto& item : source)
								core::stringify::trim(item);
						}
					}

					if (append_field)
						source.emplace_back((char*)data, length);
				}

			success:
				parser->message.header.clear();
				return true;
			}
			bool parsing::parse_version(parser* parser, const uint8_t* data, size_t length)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				VI_ASSERT(data != nullptr, "data should be set");
				if (!length || !parser->message.version)
					return true;

				memset(parser->message.version, 0, LABEL_SIZE);
				memcpy((void*)parser->message.version, (void*)data, std::min<size_t>(length, LABEL_SIZE));
				return true;
			}
			bool parsing::parse_status_code(parser* parser, size_t value)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				if (parser->message.status_code != nullptr)
					*parser->message.status_code = (int)value;
				return true;
			}
			bool parsing::parse_status_message(parser* parser, const uint8_t* name, size_t length)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				return true;
			}
			bool parsing::parse_method_value(parser* parser, const uint8_t* data, size_t length)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				VI_ASSERT(data != nullptr, "data should be set");
				if (!length || !parser->message.method)
					return true;

				memset(parser->message.method, 0, LABEL_SIZE);
				memcpy((void*)parser->message.method, (void*)data, std::min<size_t>(length, LABEL_SIZE));
				return true;
			}
			bool parsing::parse_path_value(parser* parser, const uint8_t* data, size_t length)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				VI_ASSERT(data != nullptr, "data should be set");
				if (!length || !parser->message.location)
					return true;

				parser->message.location->assign((char*)data, length);
				return true;
			}
			bool parsing::parse_query_value(parser* parser, const uint8_t* data, size_t length)
			{
				VI_ASSERT(parser != nullptr, "parser should be set");
				VI_ASSERT(data != nullptr, "data should be set");
				if (!length || !parser->message.query)
					return true;

				parser->message.query->assign((char*)data, length);
				return true;
			}
			int parsing::parse_content_range(const std::string_view& content_range, int64_t* range1, int64_t* range2)
			{
				VI_ASSERT(core::stringify::is_cstring(content_range), "content range should be set");
				VI_ASSERT(range1 != nullptr, "range 1 should be set");
				VI_ASSERT(range2 != nullptr, "range 2 should be set");
				return sscanf(content_range.data(), "bytes=%" PRId64 "-%" PRId64, range1, range2);
			}
			core::string parsing::parse_multipart_data_boundary()
			{
				static const char data[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
				std::random_device seed_generator;
				std::mt19937 engine(seed_generator());
				core::string result = "--sha1-digest-multipart-data-";
				for (int i = 0; i < 16; i++)
					result += data[engine() % (sizeof(data) - 1)];

				return result;
			}

			core::string permissions::authorize(const std::string_view& username, const std::string_view& password, const std::string_view& type)
			{
				core::string body = core::string(username);
				body.append(1, ':');
				body.append(password);

				core::string result = core::string(type);
				result.append(1, ' ');
				result.append(compute::codec::base64_encode(body));
				return result;
			}
			bool permissions::authorize(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				auto* route = base->route;
				if (!route->callbacks.authorize || route->auth.type.empty())
					return true;

				bool is_supported = false;
				for (auto& item : route->auth.methods)
				{
					if (item == base->request.method)
					{
						is_supported = true;
						break;
					}
				}

				if (!is_supported && !route->auth.methods.empty())
				{
					base->request.user.type = auth::denied;
					base->abort(401, "Authorization method is not allowed");
					return false;
				}

				auto authorization = base->request.get_header("Authorization");
				if (authorization.empty())
				{
					base->request.user.type = auth::denied;
					base->abort(401, "Provide authorization header to continue.");
					return false;
				}

				size_t index = 0;
				while (authorization[index] != ' ' && authorization[index] != '\0')
					index++;

				core::string type(authorization.data(), index);
				if (type != route->auth.type)
				{
					base->request.user.type = auth::denied;
					base->abort(401, "Authorization type \"%s\" is not allowed.", type.c_str());
					return false;
				}

				base->request.user.token = authorization.substr(index + 1);
				if (route->callbacks.authorize(base, &base->request.user))
				{
					base->request.user.type = auth::granted;
					return true;
				}

				base->request.user.type = auth::denied;
				base->abort(401, "Invalid user access credentials were provided. access denied.");
				return false;
			}
			bool permissions::method_allowed(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				for (auto& item : base->route->disallowed_methods)
				{
					if (item == base->request.method)
						return false;
				}

				return true;
			}
			bool permissions::web_socket_upgrade_allowed(connection* base)
			{
				VI_ASSERT(base != nullptr, "connection should be set");
				auto upgrade = base->request.get_header("Upgrade");
				if (upgrade.empty() || !core::stringify::case_equals(upgrade, "websocket"))
					return false;

				auto connection = base->request.get_header("Connection");
				if (connection.empty() || !core::stringify::case_equals(connection, "upgrade"))
					return false;

				return true;
			}

			bool resources::resource_has_alternative(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				if (base->route->try_files.empty())
					return false;

				for (auto& item : base->route->try_files)
				{
					if (core::os::file::get_state(item, &base->resource))
					{
						base->request.path = item;
						return true;
					}
				}

				return false;
			}
			bool resources::resource_hidden(connection* base, core::string* path)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				if (base->route->hidden_files.empty())
					return false;

				compute::regex_result result;
				const auto& value = (path ? *path : base->request.path);
				for (auto& item : base->route->hidden_files)
				{
					if (compute::regex::match(&item, result, value))
						return true;
				}

				return false;
			}
			bool resources::resource_indexed(connection* base, core::file_entry* resource)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				VI_ASSERT(resource != nullptr, "resource should be set");
				if (base->route->index_files.empty())
					return false;

				core::string path = base->request.path;
				if (!core::stringify::ends_of(path, "/\\"))
					path.append(1, VI_SPLITTER);

				for (auto& item : base->route->index_files)
				{
					if (core::os::file::get_state(item, resource))
					{
						base->request.path.assign(item);
						return true;
					}
					else if (core::os::file::get_state(path + item, resource))
					{
						base->request.path.assign(path.append(item));
						return true;
					}
				}

				return false;
			}
			bool resources::resource_modified(connection* base, core::file_entry* resource)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				VI_ASSERT(resource != nullptr, "resource should be set");

				auto cache_control = base->request.get_header("Cache-Control");
				if (!cache_control.empty() && (core::stringify::case_equals(cache_control, "no-cache") || core::stringify::case_equals(cache_control, "max-age=0")))
					return true;

				auto if_none_match = base->request.get_header("If-None-Match");
				if (!if_none_match.empty())
				{
					char etag[64];
					core::os::net::get_etag(etag, sizeof(etag), resource);
					if (core::stringify::case_equals(etag, if_none_match))
						return false;
				}

				auto if_modified_since = base->request.get_header("If-Modified-Since");
				if (if_modified_since.empty())
					return false;

				return resource->last_modified > core::date_time::seconds_from_serialized(if_modified_since, core::date_time::format_web_time());

			}
			bool resources::resource_compressed(connection* base, size_t size)
			{
#ifdef VI_ZLIB
				VI_ASSERT(connection_valid(base), "connection should be valid");
				auto* route = base->route;
				if (!route->compression.enabled || size < route->compression.min_length)
					return false;

				if (route->compression.files.empty())
					return true;

				compute::regex_result result;
				for (auto& item : route->compression.files)
				{
					if (compute::regex::match(&item, result, base->request.path))
						return true;
				}

				return false;
#else
				return false;
#endif
			}

			bool routing::route_web_socket(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				if (!base->route->allow_web_socket)
					return base->abort(404, "Websocket protocol is not allowed on this server.");

				auto* web_socket_key = base->request.get_header_blob("Sec-WebSocket-Key");
				if (web_socket_key != nullptr)
					return logical::process_web_socket(base, (uint8_t*)web_socket_key->c_str(), web_socket_key->size());

				auto web_socket_key1 = base->request.get_header("Sec-WebSocket-Key1");
				if (web_socket_key1.empty())
					return base->abort(400, "Malformed websocket request. provide first key.");

				auto web_socket_key2 = base->request.get_header("Sec-WebSocket-Key2");
				if (web_socket_key2.empty())
					return base->abort(400, "Malformed websocket request. provide second key.");

				auto resolve_connection = [base](socket_poll event, const uint8_t* buffer, size_t recv)
				{
					if (packet::is_data(event))
						base->request.content.append(std::string_view((char*)buffer, recv));
					else if (packet::is_done(event))
						logical::process_web_socket(base, (uint8_t*)base->request.content.data.data(), HTTP_WEBSOCKET_LEGACY_KEY_SIZE);
					else if (packet::is_error(event))
						base->abort();
					return true;
				};
				if (base->request.content.prefetch >= HTTP_WEBSOCKET_LEGACY_KEY_SIZE)
					return resolve_connection(socket_poll::finish_sync, (uint8_t*)"", 0);

				return !!base->stream->read_queued(HTTP_WEBSOCKET_LEGACY_KEY_SIZE - base->request.content.prefetch, resolve_connection);
			}
			bool routing::route_get(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				if (base->route->files_directory.empty() || !core::os::file::get_state(base->request.path, &base->resource))
				{
					if (permissions::web_socket_upgrade_allowed(base))
						return route_web_socket(base);

					if (!resources::resource_has_alternative(base))
						return base->abort(404, "Requested resource was not found.");
				}

				if (permissions::web_socket_upgrade_allowed(base))
					return route_web_socket(base);

				if (resources::resource_hidden(base, nullptr))
					return base->abort(404, "Requested resource was not found.");

				if (base->resource.is_directory && !resources::resource_indexed(base, &base->resource))
				{
					if (base->route->allow_directory_listing)
					{
						core::cospawn([base]() { logical::process_directory(base); });
						return true;
					}

					return base->abort(403, "Directory listing denied.");
				}

				if (base->route->static_file_max_age > 0 && !resources::resource_modified(base, &base->resource))
					return logical::process_resource_cache(base);

				return logical::process_resource(base);
			}
			bool routing::route_post(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				if (base->route->files_directory.empty() || resources::resource_hidden(base, nullptr))
					return base->abort(404, "Requested resource was not found.");

				if (!core::os::file::get_state(base->request.path, &base->resource))
					return base->abort(404, "Requested resource was not found.");

				if (base->resource.is_directory && !resources::resource_indexed(base, &base->resource))
					return base->abort(404, "Requested resource was not found.");

				if (base->route->static_file_max_age > 0 && !resources::resource_modified(base, &base->resource))
					return logical::process_resource_cache(base);

				return logical::process_resource(base);
			}
			bool routing::route_put(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				if (base->route->files_directory.empty() || resources::resource_hidden(base, nullptr))
					return base->abort(403, "Resource overwrite denied.");

				if (!core::os::file::get_state(base->request.path, &base->resource))
					return base->abort(403, "Directory overwrite denied.");

				if (!base->resource.is_directory)
					return base->abort(403, "Directory overwrite denied.");

				auto range = base->request.get_header("Range");
				int64_t range1 = 0, range2 = 0;

				auto file = core::os::file::open(base->request.path.c_str(), "wb");
				if (!file)
					return base->abort(422, "Resource stream cannot be opened.");

				FILE* stream = *file;
				if (!range.empty() && http::parsing::parse_content_range(range, &range1, &range2))
				{
					if (base->response.status_code <= 0)
						base->response.status_code = 206;

					if (!core::os::file::seek64(stream, range1, core::file_seek::begin))
						return base->abort(416, "Invalid content range offset (%" PRId64 ") was specified.", range1);
				}
				else
					base->response.status_code = 204;

				return base->fetch([stream](connection* base, socket_poll event, const std::string_view& buffer)
				{
					if (packet::is_data(event))
					{
						fwrite(buffer.data(), sizeof(char) * buffer.size(), 1, stream);
						return true;
					}
					else if (packet::is_done(event))
					{
						char date[64];
						auto* content = hrm_cache::get()->pop();
						content->append(base->request.version);
						content->append(" 204 No Content\r\nDate: ");
						content->append(header_date(date, base->info.start / 1000));
						content->append("\r\n");
						content->append("Content-Location: ").append(base->request.location).append("\r\n");
						core::os::file::close(stream);

						utils::update_keep_alive_headers(base, *content);
						if (base->route->callbacks.headers)
							base->route->callbacks.headers(base, *content);

						content->append("\r\n", 2);
						return !base->stream->write_queued((uint8_t*)content->c_str(), content->size(), [content, base](socket_poll event)
						{
							hrm_cache::get()->push(content);
							if (packet::is_done(event))
								base->next();
							else if (packet::is_error(event))
								base->abort();
						}, false);
					}
					else if (packet::is_error(event))
					{
						core::os::file::close(stream);
						return base->abort();
					}
					else if (packet::is_skip(event))
						core::os::file::close(stream);

					return true;
				});
			}
			bool routing::route_patch(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				if (base->route->files_directory.empty() || resources::resource_hidden(base, nullptr))
					return base->abort(404, "Requested resource was not found.");

				if (!core::os::file::get_state(base->request.path, &base->resource))
					return base->abort(404, "Requested resource was not found.");

				if (base->resource.is_directory && !resources::resource_indexed(base, &base->resource))
					return base->abort(404, "Requested resource cannot be directory.");

				char date[64];
				auto* content = hrm_cache::get()->pop();
				content->append(base->request.version);
				content->append(" 204 No Content\r\nDate: ");
				content->append(header_date(date, base->info.start / 1000));
				content->append("\r\n");
				content->append("Content-Location: ").append(base->request.location).append("\r\n");

				utils::update_keep_alive_headers(base, *content);
				if (base->route->callbacks.headers)
					base->route->callbacks.headers(base, *content);

				content->append("\r\n", 2);
				return !!base->stream->write_queued((uint8_t*)content->c_str(), content->size(), [content, base](socket_poll event)
				{
					hrm_cache::get()->push(content);
					if (packet::is_done(event))
						base->next(204);
					else if (packet::is_error(event))
						base->abort();
				}, false);
			}
			bool routing::route_delete(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				if (base->route->files_directory.empty() || resources::resource_hidden(base, nullptr))
					return base->abort(404, "Requested resource was not found.");

				if (!core::os::file::get_state(base->request.path, &base->resource))
					return base->abort(404, "Requested resource was not found.");

				if (!base->resource.is_directory)
				{
					if (!core::os::file::remove(base->request.path.c_str()))
						return base->abort(403, "Operation denied by system.");
				}
				else if (!core::os::directory::remove(base->request.path.c_str()))
					return base->abort(403, "Operation denied by system.");

				char date[64];
				auto* content = hrm_cache::get()->pop();
				content->append(base->request.version);
				content->append(" 204 No Content\r\nDate: ");
				content->append(header_date(date, base->info.start / 1000));
				content->append("\r\n");

				utils::update_keep_alive_headers(base, *content);
				if (base->route->callbacks.headers)
					base->route->callbacks.headers(base, *content);

				content->append("\r\n", 2);
				return !!base->stream->write_queued((uint8_t*)content->c_str(), content->size(), [content, base](socket_poll event)
				{
					hrm_cache::get()->push(content);
					if (packet::is_done(event))
						base->next(204);
					else if (packet::is_error(event))
						base->abort();
				}, false);
			}
			bool routing::route_options(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				char date[64];
				auto* content = hrm_cache::get()->pop();
				content->append(base->request.version);
				content->append(" 204 No Content\r\nDate: ");
				content->append(header_date(date, base->info.start / 1000));
				content->append("\r\n");
				content->append("Allow: GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD\r\n");

				utils::update_keep_alive_headers(base, *content);
				if (base->route && base->route->callbacks.headers)
					base->route->callbacks.headers(base, *content);

				content->append("\r\n", 2);
				return !!base->stream->write_queued((uint8_t*)content->c_str(), content->size(), [content, base](socket_poll event)
				{
					hrm_cache::get()->push(content);
					if (packet::is_done(event))
						base->next(204);
					else if (packet::is_error(event))
						base->abort();
				}, false);
			}

			bool logical::process_directory(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				VI_MEASURE(core::timings::file_system);
				core::vector<std::pair<core::string, core::file_entry>> entries;
				if (!core::os::directory::scan(base->request.path, entries))
					return base->abort(500, "System denied to directory listing.");

				char date[64];
				auto* content = hrm_cache::get()->pop();
				content->append(base->request.version);
				content->append(" 200 OK\r\nDate: ");
				content->append(header_date(date, base->info.start / 1000));
				content->append("\r\n");
				content->append("Content-Type: text/html; charset=").append(base->route->char_set);
				content->append("\r\nAccept-ranges: bytes\r\n");

				paths::construct_head_cache(base, *content);
				utils::update_keep_alive_headers(base, *content);
				if (base->route->callbacks.headers)
					base->route->callbacks.headers(base, *content);

				auto message = base->response.get_header("X-Error");
				if (!message.empty())
					content->append("X-error: ").append(message).append("\r\n");

				size_t size = base->request.location.size() - 1;
				while (base->request.location[size] != '/')
					size--;

				char direction = (!base->request.query.empty() && base->request.query[1] == 'd') ? 'a' : 'd';
				core::string parent(1, '/');
				if (base->request.location.size() > 1)
				{
					parent = base->request.location.substr(0, base->request.location.size() - 1);
					parent = core::os::path::get_directory(parent.c_str());
				}

				base->response.content.assign(
					"<html><head><title>index of " + base->request.location + "</title>"
					"<style>" CSS_DIRECTORY_STYLE "</style></head>"
					"<body><h1>index of " + base->request.location + "</h1><pre><table cellpadding=\"0\">"
					"<tr><th><a href=\"?n" + direction + "\">name</a></th>"
					"<th><a href=\"?d" + direction + "\">modified</a></th>"
					"<th><a href=\"?s" + direction + "\">size</a></th></tr>"
					"<tr><td colspan=\"3\"><hr></td></tr>"
					"<tr><td><a href=\"" + parent + "\">parent directory</a></td>"
					"<td>&nbsp;-</td><td>&nbsp;&nbsp;-</td></tr>");
				VI_SORT(entries.begin(), entries.end(), [&base](const std::pair<core::string, core::file_entry>& a, const std::pair<core::string, core::file_entry>& b)
				{
					return paths::construct_directory_entries(base, a.first, a.second, b.first, b.second);
				});

				for (auto& item : entries)
				{
					if (resources::resource_hidden(base, &item.first))
						continue;

					char file_size[64];
					if (!item.second.is_directory)
					{
						if (item.second.size < 1024)
							snprintf(file_size, sizeof(file_size), "%d bytes", (int)item.second.size);
						else if (item.second.size < 0x100000)
							snprintf(file_size, sizeof(file_size), "%.1f kb", ((double)item.second.size) / 1024.0);
						else if (item.second.size < 0x40000000)
							snprintf(file_size, sizeof(size), "%.1f mb", ((double)item.second.size) / 1048576.0);
						else
							snprintf(file_size, sizeof(file_size), "%.1f gb", ((double)item.second.size) / 1073741824.0);
					}
					else
						strncpy(file_size, "[DIRECTORY]", sizeof(file_size));

					char last_modified[64];
					std::string_view file_date = header_date(last_modified, item.second.last_modified);
					core::string location = compute::codec::url_encode(item.first);
					core::string link = (base->request.location + ((*(base->request.location.c_str() + 1) != '\0' && base->request.location[base->request.location.size() - 1] != '/') ? "/" : "") + location);
					if (item.second.is_directory && !core::stringify::ends_of(link, "/\\"))
						link.append(1, '/');

					base->response.content.append("<tr><td><a href=\"");
					base->response.content.append(link);
					base->response.content.append("\">");
					base->response.content.append(item.first);
					base->response.content.append("</a></td><td>&nbsp;");
					base->response.content.append(file_date);
					base->response.content.append("</td><td>&nbsp;&nbsp;");
					base->response.content.append(file_size);
					base->response.content.append("</td></tr>\n");
				}
				base->response.content.append("</table></pre></body></html>");
#ifdef VI_ZLIB
				bool deflate = false, gzip = false;
				if (resources::resource_compressed(base, base->response.content.data.size()))
				{
					auto accept_encoding = base->request.get_header("Accept-Encoding");
					if (!accept_encoding.empty())
					{
						deflate = accept_encoding.find("deflate") != std::string::npos;
						gzip = accept_encoding.find("gzip") != std::string::npos;
					}

					if (!accept_encoding.empty() && (deflate || gzip))
					{
						z_stream stream;
						stream.zalloc = Z_NULL;
						stream.zfree = Z_NULL;
						stream.opaque = Z_NULL;
						stream.avail_in = (uInt)base->response.content.data.size();
						stream.next_in = (Bytef*)base->response.content.data.data();

						if (deflateInit2(&stream, base->route->compression.quality_level, Z_DEFLATED, (gzip ? 15 | 16 : 15), base->route->compression.memory_level, (int)base->route->compression.tune) == Z_OK)
						{
							core::string buffer(base->response.content.data.size(), '\0');
							stream.avail_out = (uInt)buffer.size();
							stream.next_out = (Bytef*)buffer.c_str();
							bool compress = (::deflate(&stream, Z_FINISH) == Z_STREAM_END);
							bool flush = (deflateEnd(&stream) == Z_OK);

							if (compress && flush)
							{
								base->response.content.assign(std::string_view(buffer.c_str(), (size_t)stream.total_out));
								if (base->response.get_header("Content-Encoding").empty())
								{
									if (gzip)
										content->append("Content-Encoding: gzip\r\n", 24);
									else
										content->append("Content-Encoding: deflate\r\n", 27);
								}
							}
						}
					}
				}
#endif
				content->append("Content-Length: ").append(core::to_string(base->response.content.data.size())).append("\r\n\r\n");
				return !!base->stream->write_queued((uint8_t*)content->c_str(), content->size(), [content, base](socket_poll event)
				{
					hrm_cache::get()->push(content);
					if (packet::is_done(event))
					{
						if (memcmp(base->request.method, "HEAD", 4) == 0)
							return (void)base->next(200);

						base->stream->write_queued((uint8_t*)base->response.content.data.data(), base->response.content.data.size(), [base](socket_poll event)
						{
							if (packet::is_done(event))
								base->next(200);
							else if (packet::is_error(event))
								base->abort();
						}, false);
					}
					else if (packet::is_error(event))
						base->abort();
				}, false);
			}
			bool logical::process_resource(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				auto content_type = utils::content_type(base->request.path, &base->route->mime_types);
				auto range = base->request.get_header("Range");
				auto status_message = utils::status_message(base->response.status_code = (base->response.error && base->response.status_code > 0 ? base->response.status_code : 200));
				int64_t range1 = 0, range2 = 0, count = 0;
				int64_t content_length = (int64_t)base->resource.size;

				char content_range[128] = { };
				if (!range.empty() && (count = parsing::parse_content_range(range, &range1, &range2)) > 0 && range1 >= 0 && range2 >= 0)
				{
					if (count == 2)
						content_length = (int64_t)(((range2 > content_length) ? content_length : range2) - range1 + 1);
					else
						content_length -= range1;

					snprintf(content_range, sizeof(content_range), "Content-Range: bytes %" PRId64 "-%" PRId64 "/%" PRId64 "\r\n", range1, range1 + content_length - 1, (int64_t)base->resource.size);
					status_message = utils::status_message(base->response.status_code = (base->response.error ? base->response.status_code : 206));
				}
#ifdef VI_ZLIB
				if (resources::resource_compressed(base, (size_t)content_length))
				{
					auto accept_encoding = base->request.get_header("Accept-Encoding");
					if (!accept_encoding.empty())
					{
						bool deflate = accept_encoding.find("deflate") != std::string::npos;
						bool gzip = accept_encoding.find("gzip") != std::string::npos;
						if (deflate || gzip)
							return process_resource_compress(base, deflate, gzip, content_range, (size_t)range1);
					}
				}
#endif
				char date[64];
				auto* content = hrm_cache::get()->pop();
				content->append(base->request.version).append(" ");
				content->append(core::to_string(base->response.status_code)).append(" ");
				content->append(status_message).append("\r\n");
				content->append("Date: ");
				content->append(header_date(date, base->info.start / 1000));
				content->append("\r\n");

				auto origin = base->request.get_header("Origin");
				if (!origin.empty())
					content->append("Access-Control-Allow-Origin: ").append(base->route->access_control_allow_origin).append("\r\n");

				paths::construct_head_cache(base, *content);
				utils::update_keep_alive_headers(base, *content);
				if (base->route->callbacks.headers)
					base->route->callbacks.headers(base, *content);

				auto message = base->response.get_header("X-Error");
				if (!message.empty())
					content->append("X-error: ").append(message).append("\r\n");

				content->append("Accept-Ranges: bytes\r\nLast-modified: ");
				content->append(header_date(date, base->resource.last_modified));
				content->append("\r\n");

				core::os::net::get_etag(date, sizeof(date), &base->resource);
				content->append("Etag: ").append(date, strnlen(date, sizeof(date))).append("\r\n");
				content->append("Content-Type: ").append(content_type).append("; charset=").append(base->route->char_set).append("\r\n");
				content->append("Content-Length: ").append(core::to_string(content_length)).append("\r\n");
				content->append(content_range).append("\r\n");

				if (content_length > 0 && strcmp(base->request.method, "HEAD") != 0)
				{
					return !!base->stream->write_queued((uint8_t*)content->c_str(), content->size(), [content, base, content_length, range1](socket_poll event)
					{
						hrm_cache::get()->push(content);
						if (packet::is_done(event))
							core::cospawn([base, content_length, range1]() { logical::process_file(base, (size_t)content_length, (size_t)range1); });
						else if (packet::is_error(event))
							base->abort();
					}, false);
				}
				else
				{
					return !!base->stream->write_queued((uint8_t*)content->c_str(), content->size(), [content, base](socket_poll event)
					{
						hrm_cache::get()->push(content);
						if (packet::is_done(event))
							base->next(200);
						else if (packet::is_error(event))
							base->abort();
					}, false);
				}
			}
			bool logical::process_resource_compress(connection* base, bool deflate, bool gzip, const std::string_view& content_range, size_t range)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				VI_ASSERT(deflate || gzip, "uncompressable resource");
				auto content_type = utils::content_type(base->request.path, &base->route->mime_types);
				auto status_message = utils::status_message(base->response.status_code = (base->response.error && base->response.status_code > 0 ? base->response.status_code : 200));
				int64_t content_length = (int64_t)base->resource.size;

				char date[64];
				auto* content = hrm_cache::get()->pop();
				content->append(base->request.version).append(" ");
				content->append(core::to_string(base->response.status_code)).append(" ");
				content->append(status_message).append("\r\n");
				content->append("Date: ");
				content->append(header_date(date, base->info.start / 1000));
				content->append("\r\n");

				auto origin = base->request.get_header("Origin");
				if (!origin.empty())
					content->append("Access-Control-Allow-Origin: ").append(base->route->access_control_allow_origin).append("\r\n");

				paths::construct_head_cache(base, *content);
				utils::update_keep_alive_headers(base, *content);
				if (base->route->callbacks.headers)
					base->route->callbacks.headers(base, *content);

				auto message = base->response.get_header("X-Error");
				if (!message.empty())
					content->append("X-error: ").append(message).append("\r\n");

				content->append("Accept-Ranges: bytes\r\nLast-modified: ");
				content->append(header_date(date, base->resource.last_modified));
				content->append("\r\n");

				core::os::net::get_etag(date, sizeof(date), &base->resource);
				content->append("Etag: ").append(date, strnlen(date, sizeof(date))).append("\r\n");
				content->append("Content-Type: ").append(content_type).append("; charset=").append(base->route->char_set).append("\r\n");
				content->append("Content-Encoding: ").append(gzip ? "gzip" : "deflate").append("\r\n");
				content->append("Transfer-Encoding: chunked\r\n");
				content->append(content_range).append("\r\n");

				if (content_length > 0 && strcmp(base->request.method, "HEAD") != 0)
				{
					return !!base->stream->write_queued((uint8_t*)content->c_str(), content->size(), [content, base, range, content_length, gzip](socket_poll event)
					{
						hrm_cache::get()->push(content);
						if (packet::is_done(event))
							core::cospawn([base, range, content_length, gzip]() { logical::process_file_compress(base, (size_t)content_length, (size_t)range, gzip); });
						else if (packet::is_error(event))
							base->abort();
					}, false);
				}
				else
				{
					return !!base->stream->write_queued((uint8_t*)content->c_str(), content->size(), [content, base](socket_poll event)
					{
						hrm_cache::get()->push(content);
						if (packet::is_done(event))
							base->next();
						else if (packet::is_error(event))
							base->abort();
					}, false);
				}
			}
			bool logical::process_resource_cache(connection* base)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				char date[64];
				auto* content = hrm_cache::get()->pop();
				content->append(base->request.version);
				content->append(" 304 not modified\r\nDate: ");
				content->append(header_date(date, base->info.start / 1000));
				content->append("\r\n");

				paths::construct_head_cache(base, *content);
				utils::update_keep_alive_headers(base, *content);
				if (base->route->callbacks.headers)
					base->route->callbacks.headers(base, *content);

				content->append("Accept-Ranges: bytes\r\nLast-modified: ");
				content->append(header_date(date, base->resource.last_modified));
				content->append("\r\n");

				core::os::net::get_etag(date, sizeof(date), &base->resource);
				content->append("Etag: ").append(date, strnlen(date, sizeof(date))).append("\r\n\r\n");
				return !!base->stream->write_queued((uint8_t*)content->c_str(), content->size(), [content, base](socket_poll event)
				{
					hrm_cache::get()->push(content);
					if (packet::is_done(event))
						base->next(304);
					else if (packet::is_error(event))
						base->abort();
				}, false);
			}
			bool logical::process_file(connection* base, size_t content_length, size_t range)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				VI_MEASURE(core::timings::file_system);
				range = (range > base->resource.size ? base->resource.size : range);
				if (content_length > 0 && base->resource.is_referenced && base->resource.size > 0)
				{
					size_t limit = base->resource.size - range;
					if (content_length > limit)
						content_length = limit;

					if (base->response.content.data.size() >= content_length)
					{
						return !!base->stream->write_queued((uint8_t*)base->response.content.data.data() + range, content_length, [base](socket_poll event)
						{
							if (packet::is_done(event))
								base->next();
							else if (packet::is_error(event))
								base->abort();
						}, false);
					}
				}

				auto file = core::os::file::open(base->request.path.c_str(), "rb");
				if (!file)
					return base->abort(500, "System denied to open resource stream.");

				FILE* stream = *file;
				if (base->route->allow_send_file)
				{
					auto result = base->stream->write_file_queued(stream, range, content_length, [base, stream, content_length, range](socket_poll event)
					{
						if (packet::is_done(event))
						{
							core::os::file::close(stream);
							base->next();
						}
						else if (packet::is_error(event))
							process_file_stream(base, stream, content_length, range);
						else if (packet::is_skip(event))
							core::os::file::close(stream);
					});
					if (result || result.error() != std::errc::not_supported)
						return true;
				}

				return process_file_stream(base, stream, content_length, range);
			}
			bool logical::process_file_stream(connection* base, FILE* stream, size_t content_length, size_t range)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				VI_ASSERT(stream != nullptr, "stream should be set");
				if (!core::os::file::seek64(stream, range, core::file_seek::begin))
				{
					core::os::file::close(stream);
					return base->abort(400, "Provided content range offset (%" PRIu64 ") is invalid", range);
				}

				return process_file_chunk(base, stream, content_length);
			}
			bool logical::process_file_chunk(connection* base, FILE* stream, size_t content_length)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				VI_ASSERT(stream != nullptr, "stream should be set");
				VI_MEASURE(core::timings::file_system);
			retry:
				uint8_t buffer[core::BLOB_SIZE];
				if (!content_length || base->root->state != server_state::working)
				{
				cleanup:
					core::os::file::close(stream);
					if (base->root->state != server_state::working)
						return base->abort();

					return base->next() || true;
				}

				size_t read = sizeof(buffer);
				if ((read = (size_t)fread(buffer, 1, read > content_length ? content_length : read, stream)) <= 0)
					goto cleanup;

				content_length -= read;
				auto written = base->stream->write_queued(buffer, read, [base, stream, content_length](socket_poll event)
				{
					if (packet::is_done_async(event))
					{
						core::cospawn([base, stream, content_length]()
						{
							process_file_chunk(base, stream, content_length);
						});
					}
					else if (packet::is_error(event))
					{
						core::os::file::close(stream);
						base->abort();
					}
					else if (packet::is_skip(event))
						core::os::file::close(stream);
				});

				if (written && *written > 0)
					goto retry;

				return false;
			}
			bool logical::process_file_compress(connection* base, size_t content_length, size_t range, bool gzip)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				VI_MEASURE(core::timings::file_system);
				range = (range > base->resource.size ? base->resource.size : range);
				if (content_length > 0 && base->resource.is_referenced && base->resource.size > 0)
				{
					if (base->response.content.data.size() >= content_length)
					{
#ifdef VI_ZLIB
						z_stream zstream;
						zstream.zalloc = Z_NULL;
						zstream.zfree = Z_NULL;
						zstream.opaque = Z_NULL;
						zstream.avail_in = (uInt)base->response.content.data.size();
						zstream.next_in = (Bytef*)base->response.content.data.data();

						if (deflateInit2(&zstream, base->route->compression.quality_level, Z_DEFLATED, (gzip ? 15 | 16 : 15), base->route->compression.memory_level, (int)base->route->compression.tune) == Z_OK)
						{
							core::string buffer(base->response.content.data.size(), '\0');
							zstream.avail_out = (uInt)buffer.size();
							zstream.next_out = (Bytef*)buffer.c_str();
							bool compress = (deflate(&zstream, Z_FINISH) == Z_STREAM_END);
							bool flush = (deflateEnd(&zstream) == Z_OK);

							if (compress && flush)
								base->response.content.assign(std::string_view(buffer.c_str(), (size_t)zstream.total_out));
						}
#endif
						return !!base->stream->write_queued((uint8_t*)base->response.content.data.data(), content_length, [base](socket_poll event)
						{
							if (packet::is_done(event))
								base->next();
							else if (packet::is_error(event))
								base->abort();
						}, false);
					}
				}

				auto file = core::os::file::open(base->request.path.c_str(), "rb");
				if (!file)
					return base->abort(500, "System denied to open resource stream.");

				FILE* stream = *file;
				if (range > 0 && !core::os::file::seek64(stream, range, core::file_seek::begin))
				{
					core::os::file::close(stream);
					return base->abort(400, "Provided content range offset (%" PRIu64 ") is invalid", range);
				}
#ifdef VI_ZLIB
				z_stream* zstream = core::memory::allocate<z_stream>(sizeof(z_stream));
				zstream->zalloc = Z_NULL;
				zstream->zfree = Z_NULL;
				zstream->opaque = Z_NULL;

				if (deflateInit2(zstream, base->route->compression.quality_level, Z_DEFLATED, (gzip ? MAX_WBITS + 16 : MAX_WBITS), base->route->compression.memory_level, (int)base->route->compression.tune) != Z_OK)
				{
					core::os::file::close(stream);
					core::memory::deallocate(zstream);
					return base->abort();
				}

				return process_file_compress_chunk(base, stream, zstream, content_length);
#else
				core::os::file::close(stream);
				return base->abort(500, "Cannot process gzip stream.");
#endif
			}
			bool logical::process_file_compress_chunk(connection* base, FILE* stream, void* cstream, size_t content_length)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				VI_ASSERT(stream != nullptr, "stream should be set");
				VI_ASSERT(cstream != nullptr, "cstream should be set");
				VI_MEASURE(core::timings::file_system);
#ifdef VI_ZLIB
#define FREE_STREAMING { core::os::file::close(stream); deflateEnd(zstream); core::memory::deallocate(zstream); }
				z_stream* zstream = (z_stream*)cstream;
			retry:
				uint8_t buffer[core::BLOB_SIZE + GZ_HEADER_SIZE], deflate[core::BLOB_SIZE];
				if (!content_length || base->root->state != server_state::working)
				{
				cleanup:
					FREE_STREAMING;
					if (base->root->state != server_state::working)
						return base->abort();

					return !!base->stream->write_queued((uint8_t*)"0\r\n\r\n", 5, [base](socket_poll event)
					{
						if (packet::is_done(event))
							base->next();
						else if (packet::is_error(event))
							base->abort();
					}, false);
				}

				size_t read = sizeof(buffer) - GZ_HEADER_SIZE;
				if ((read = (size_t)fread(buffer, 1, read > content_length ? content_length : read, stream)) <= 0)
					goto cleanup;

				content_length -= read;
				zstream->avail_in = (uInt)read;
				zstream->next_in = (Bytef*)buffer;
				zstream->avail_out = (uInt)sizeof(deflate);
				zstream->next_out = (Bytef*)deflate;
				::deflate(zstream, Z_SYNC_FLUSH);
				read = (int)sizeof(deflate) - (int)zstream->avail_out;

				int next = snprintf((char*)buffer, sizeof(buffer), "%x\r\n", (uint32_t)read);
				memcpy(buffer + next, deflate, read);
				read += next;

				if (!content_length)
				{
					memcpy(buffer + read, "\r\n0\r\n\r\n", sizeof(char) * 7);
					read += sizeof(char) * 7;
				}
				else
				{
					memcpy(buffer + read, "\r\n", sizeof(char) * 2);
					read += sizeof(char) * 2;
				}

				auto written = base->stream->write_queued(buffer, read, [base, stream, zstream, content_length](socket_poll event)
				{
					if (packet::is_done_async(event))
					{
						if (content_length > 0)
						{
							core::cospawn([base, stream, zstream, content_length]()
							{
								process_file_compress_chunk(base, stream, zstream, content_length);
							});
						}
						else
						{
							FREE_STREAMING;
							base->next();
						}
					}
					else if (packet::is_error(event))
					{
						FREE_STREAMING;
						base->abort();
					}
					else if (packet::is_skip(event))
						FREE_STREAMING;
				});
				if (written && *written > 0)
					goto retry;

				return false;
#undef FREE_STREAMING
#else
				return base->next();
#endif
			}
			bool logical::process_web_socket(connection* base, const uint8_t* key, size_t key_size)
			{
				VI_ASSERT(connection_valid(base), "connection should be valid");
				VI_ASSERT(key != nullptr, "key should be set");
				auto version = base->request.get_header("Sec-WebSocket-Version");
				if (version.empty() || version != "13")
					return base->abort(426, "Protocol upgrade required. version \"%s\" is not allowed", version);

				char buffer[128];
				if (key_size + sizeof(HTTP_WEBSOCKET_KEY) >= sizeof(buffer))
					return base->abort(426, "Protocol upgrade required. supplied key is invalid", version);

				snprintf(buffer, sizeof(buffer), "%.*s%s", (int)key_size, key, HTTP_WEBSOCKET_KEY);
				base->request.content.data.clear();

				char encoded20[20];
				compute::crypto::sha1_compute(buffer, (int)strlen(buffer), encoded20);
				if (base->response.status_code <= 0)
					base->response.status_code = 101;

				auto* content = hrm_cache::get()->pop();
				content->append(
					"HTTP/1.1 101 Switching Protocols\r\n"
					"Upgrade: websocket\r\n"
					"Connection: upgrade\r\n"
					"Sec-WebSocket-Accept: ");
				content->append(compute::codec::base64_encode(std::string_view(encoded20, 20)));
				content->append("\r\n");

				auto protocol = base->request.get_header("Sec-WebSocket-Protocol");
				if (!protocol.empty())
				{
					const char* offset = strchr(protocol.data(), ',');
					if (offset != nullptr)
						content->append("Sec-WebSocket-protocol: ").append(protocol, (size_t)(offset - protocol.data())).append("\r\n");
					else
						content->append("Sec-WebSocket-protocol: ").append(protocol).append("\r\n");
				}

				if (base->route->callbacks.headers)
					base->route->callbacks.headers(base, *content);

				content->append("\r\n", 2);
				return !!base->stream->write_queued((uint8_t*)content->c_str(), content->size(), [content, base](socket_poll event)
				{
					hrm_cache::get()->push(content);
					if (packet::is_done(event))
					{
						base->web_socket = new web_socket_frame(base->stream, base);
						base->web_socket->connect = base->route->callbacks.web_socket.connect;
						base->web_socket->receive = base->route->callbacks.web_socket.receive;
						base->web_socket->disconnect = base->route->callbacks.web_socket.disconnect;
						base->web_socket->lifetime.dead = [base](web_socket_frame*)
						{
							return base->info.abort;
						};
						base->web_socket->lifetime.close = [base](web_socket_frame*, bool successful)
						{
							if (successful)
								base->next();
							else
								base->abort();
						};

						base->stream->set_io_timeout(base->route->web_socket_timeout);
						if (!base->route->callbacks.web_socket.initiate || !base->route->callbacks.web_socket.initiate(base))
							base->web_socket->next();
					}
					else if (packet::is_error(event))
						base->abort();
				}, false);
			}

			server::server() : socket_server()
			{
				hrm_cache::link_instance();
			}
			server::~server()
			{
				unlisten(false);
			}
			core::expects_system<void> server::update()
			{
				auto* target = (map_router*)router;
				if (!target->session.directory.empty())
				{
					auto directory = core::os::path::resolve(target->session.directory);
					if (directory)
						target->session.directory = *directory;

					auto status = core::os::directory::patch(target->session.directory);
					if (!status)
						return core::system_exception("session directory: invalid path", std::move(status.error()));
				}

				if (!target->temporary_directory.empty())
				{
					auto directory = core::os::path::resolve(target->temporary_directory);
					if (directory)
						target->temporary_directory = *directory;

					auto status = core::os::directory::patch(target->temporary_directory);
					if (!status)
						return core::system_exception("temporary directory: invalid path", std::move(status.error()));
				}

				auto status = update_route(target->base);
				if (!status)
					return status;

				for (auto& group : target->groups)
				{
					for (auto* route : group->routes)
					{
						status = update_route(route);
						if (!status)
							return status;
					}
				}

				target->sort();
				return core::expectation::met;
			}
			core::expects_system<void> server::update_route(router_entry* route)
			{
				route->router = (map_router*)router;
				if (!route->files_directory.empty())
				{
					auto directory = core::os::path::resolve(route->files_directory.c_str());
					if (directory)
						route->files_directory = *directory;
				}

				if (!route->alias.empty())
				{
					auto directory = core::os::path::resolve(route->alias, route->files_directory.empty() ? *core::os::directory::get_working() : route->files_directory, true);
					if (directory)
						route->alias = *directory;
				}

				return core::expectation::met;
			}
			core::expects_system<void> server::on_configure(socket_router* new_router)
			{
				VI_ASSERT(new_router != nullptr, "router should be set");
				return update();
			}
			core::expects_system<void> server::on_unlisten()
			{
				VI_ASSERT(router != nullptr, "router should be set");
				map_router* target = (map_router*)router;
				if (!target->temporary_directory.empty())
				{
					auto status = core::os::directory::remove(target->temporary_directory.c_str());
					if (!status)
						return core::system_exception("temporary directory remove error: " + target->temporary_directory, std::move(status.error()));
				}

				if (!target->session.directory.empty())
				{
					auto status = session::invalidate_cache(target->session.directory);
					if (!status)
						return status;
				}

				return core::expectation::met;
			}
			void server::on_request_open(socket_connection* source)
			{
				VI_ASSERT(source != nullptr, "connection should be set");
				auto* conf = (map_router*)router;
				auto* base = (connection*)source;

				base->resolver->prepare_for_request_parsing(&base->request);
				base->stream->read_until_chunked_queued("\r\n\r\n", [base, conf](socket_poll event, const uint8_t* buffer, size_t size)
				{
					if (packet::is_data(event))
					{
						size_t last_length = base->request.content.data.size();
						base->request.content.append(std::string_view((char*)buffer, size));
						if (base->request.content.data.size() > conf->max_heap_buffer)
						{
							base->abort(431, "Request containts too much data in headers");
							return false;
						}

						int64_t offset = base->resolver->parse_request((uint8_t*)base->request.content.data.data(), base->request.content.data.size(), last_length);
						if (offset >= 0 || offset == -2)
							return true;

						base->abort(400, "Invalid request was provided by client");
						return false;
					}
					else if (packet::is_done(event))
					{
						uint32_t redirects = 0;
						base->info.start = network::utils::clock();
						base->request.content.prepare(base->request.headers, buffer, size);
					redirect:
						if (!paths::construct_route(conf, base))
							return base->abort(400, "Request cannot be resolved");

						auto* route = base->route;
						if (!route->redirect.empty())
						{
							if (redirects++ > HTTP_MAX_REDIRECTS)
								return base->abort(500, "Infinite redirects loop detected");

							base->request.location = route->redirect;
							goto redirect;
						}

						paths::construct_path(base);
						if (!permissions::method_allowed(base))
							return base->abort(405, "Requested method \"%s\" is not allowed on this server", base->request.method);

						if (!memcmp(base->request.method, "GET", 3) || !memcmp(base->request.method, "HEAD", 4))
						{
							if (!permissions::authorize(base))
								return false;

							if (route->callbacks.get && route->callbacks.get(base))
								return true;

							return routing::route_get(base);
						}
						else if (!memcmp(base->request.method, "POST", 4))
						{
							if (!permissions::authorize(base))
								return false;

							if (route->callbacks.post && route->callbacks.post(base))
								return true;

							return routing::route_post(base);
						}
						else if (!memcmp(base->request.method, "PUT", 3))
						{
							if (!permissions::authorize(base))
								return false;

							if (route->callbacks.put && route->callbacks.put(base))
								return true;

							return routing::route_put(base);
						}
						else if (!memcmp(base->request.method, "PATCH", 5))
						{
							if (!permissions::authorize(base))
								return false;

							if (route->callbacks.patch && route->callbacks.patch(base))
								return true;

							return routing::route_patch(base);
						}
						else if (!memcmp(base->request.method, "DELETE", 6))
						{
							if (!permissions::authorize(base))
								return false;

							if (route->callbacks.deinit && route->callbacks.deinit(base))
								return true;

							return routing::route_delete(base);
						}
						else if (!memcmp(base->request.method, "OPTIONS", 7))
						{
							if (route->callbacks.options && route->callbacks.options(base))
								return true;

							return routing::route_options(base);
						}

						if (!permissions::authorize(base))
							return false;

						return base->abort(405, "Request method \"%s\" is not allowed", base->request.method);
					}
					else if (packet::is_error(event))
						base->abort();

					return true;
				});
			}
			bool server::on_request_cleanup(socket_connection* target)
			{
				VI_ASSERT(target != nullptr, "connection should be set");
				auto base = (http::connection*)target;
				if (!base->is_skip_required())
					return true;

				return base->skip([](http::connection* base)
				{
					base->root->finalize(base);
					return true;
				});
			}
			void server::on_request_stall(socket_connection* target)
			{
				auto base = (http::connection*)target;
				core::string status = ", pathname: " + base->request.location;
				if (base->web_socket != nullptr)
					status += ", websocket: " + core::string(base->web_socket->is_finished() ? "alive" : "dead");
				VI_DEBUG("stall connection on fd %i%s", (int)base->stream->get_fd(), status.c_str());
			}
			void server::on_request_close(socket_connection* target)
			{
				VI_ASSERT(target != nullptr, "connection should be set");
				auto base = (http::connection*)target;
				if (base->response.status_code > 0 && base->route && base->route->callbacks.access)
					base->route->callbacks.access(base);
				base->reset(false);
			}
			socket_connection* server::on_allocate(socket_listener* host)
			{
				VI_ASSERT(host != nullptr, "host should be set");
				auto* base = new http::connection(this);
				auto* target = (map_router*)router;
				base->route = target->base;
				base->root = this;
				return base;
			}
			socket_router* server::on_allocate_router()
			{
				return new map_router();
			}

			client::client(int64_t read_timeout) : socket_client(read_timeout), resolver(new http::parser()), web_socket(nullptr), future(core::expects_promise_system<void>::null())
			{
				response.content.finalize();
				hrm_cache::link_instance();
			}
			client::~client()
			{
				core::memory::release(resolver);
				core::memory::release(web_socket);
			}
			core::expects_promise_system<void> client::skip()
			{
				return fetch(PAYLOAD_SIZE, true);
			}
			core::expects_promise_system<void> client::fetch(size_t max_size, bool eat)
			{
				VI_ASSERT(!web_socket, "cannot read http over websocket");
				if (response.content.is_finalized())
					return core::expects_promise_system<void>(core::expectation::met);
				else if (response.content.exceeds)
					return core::expects_promise_system<void>(core::system_exception("download content error: payload too large", std::make_error_condition(std::errc::value_too_large)));
				else if (!has_stream())
					return core::expects_promise_system<void>(core::system_exception("download content error: bad fd", std::make_error_condition(std::errc::bad_file_descriptor)));

				auto content_type = response.get_header("Content-Type");
				if (content_type == std::string_view("multipart/form-data", 19))
				{
					response.content.exceeds = true;
					return core::expects_promise_system<void>(core::system_exception("download content error: requires file saving", std::make_error_condition(std::errc::value_too_large)));
				}
				else if (eat)
					max_size = std::numeric_limits<size_t>::max();

				size_t leftover_size = response.content.data.size() - response.content.prefetch;
				if (!response.content.data.empty() && leftover_size > 0 && leftover_size <= response.content.data.size())
					response.content.data.erase(response.content.data.begin(), response.content.data.begin() + leftover_size);

				auto transfer_encoding = response.get_header("Transfer-Encoding");
				bool is_transfer_encoding_chunked = (!response.content.limited && core::stringify::case_equals(transfer_encoding, "chunked"));
				if (is_transfer_encoding_chunked)
					resolver->prepare_for_chunked_parsing();

				if (response.content.prefetch > 0)
				{
					leftover_size = std::min(max_size, response.content.prefetch);
					response.content.prefetch -= leftover_size;
					max_size -= leftover_size;
					if (is_transfer_encoding_chunked)
					{
						size_t decoded_size = leftover_size;
						int64_t subresult = resolver->parse_decode_chunked((uint8_t*)response.content.data.data(), &decoded_size);
						if (subresult >= 0 || subresult == -2)
						{
							leftover_size -= decoded_size;
							if (leftover_size > 0)
								response.content.data.erase(response.content.data.begin() + decoded_size, response.content.data.begin() + decoded_size + leftover_size);
							if (subresult == 0)
							{
								response.content.finalize();
								return core::expects_promise_system<void>(core::expectation::met);
							}
						}
						else if (subresult == -1)
							return core::expects_promise_system<void>(core::system_exception("download transfer encoding content parsing error", std::make_error_condition(std::errc::protocol_error)));
					}
					else if (response.content.prefetch > 0)
						return core::expects_promise_system<void>(core::expectation::met);
				}
				else
					response.content.data.clear();

				if (is_transfer_encoding_chunked)
				{
					if (!max_size)
						return core::expects_promise_system<void>(core::expectation::met);

					int64_t subresult = -1;
					core::expects_promise_system<void> result;
					net.stream->read_queued(max_size, [this, result, subresult, eat](socket_poll event, const uint8_t* buffer, size_t recv) mutable
					{
						if (packet::is_data(event))
						{
							subresult = resolver->parse_decode_chunked((uint8_t*)buffer, &recv);
							if (subresult == -1)
								return false;

							response.content.offset += recv;
							if (!eat)
								response.content.append(std::string_view((char*)buffer, recv));
							return subresult == -2;
						}
						else if (packet::is_done(event) || packet::is_error_or_skip(event))
						{
							if (subresult != -2)
							{
								response.content.finalize();
								if (!response.content.data.empty())
									VI_DEBUG("http fd %i responded\n%.*s", (int)net.stream->get_fd(), (int)response.content.data.size(), response.content.data.data());
							}

							if (packet::is_error_or_skip(event))
								result.set(core::system_exception("download transfer encoding content parsing error", std::make_error_condition(std::errc::protocol_error)));
							else
								result.set(core::expectation::met);
						}

						return true;
					});
					return result;
				}
				else if (!response.content.limited)
				{
					if (!max_size)
						return core::expects_promise_system<void>(core::expectation::met);

					core::expects_promise_system<void> result;
					net.stream->read_queued(max_size, [this, result, eat](socket_poll event, const uint8_t* buffer, size_t recv) mutable
					{
						if (packet::is_data(event))
						{
							response.content.offset += recv;
							if (!eat)
								response.content.append(std::string_view((char*)buffer, recv));
							return true;
						}
						else if (packet::is_done(event) || packet::is_error_or_skip(event))
						{
							response.content.finalize();
							if (!response.content.data.empty())
								VI_DEBUG("http fd %i responded\n%.*s", (int)net.stream->get_fd(), (int)response.content.data.size(), response.content.data.data());

							if (packet::is_error_or_skip(event))
								result.set(core::system_exception("download content network error", packet::to_condition(event)));
							else
								result.set(core::expectation::met);
						}

						return true;
					});
					return result;
				}

				max_size = std::min(max_size, response.content.length - response.content.offset);
				if (!max_size)
				{
					response.content.finalize();
					return core::expects_promise_system<void>(core::expectation::met);
				}
				else if (response.content.offset > response.content.length)
					return core::expects_promise_system<void>(core::system_exception("download content error: invalid range", std::make_error_condition(std::errc::result_out_of_range)));

				core::expects_promise_system<void> result;
				net.stream->read_queued(max_size, [this, result, eat](socket_poll event, const uint8_t* buffer, size_t recv) mutable
				{
					if (packet::is_data(event))
					{
						response.content.offset += recv;
						if (!eat)
							response.content.append(std::string_view((char*)buffer, recv));
						return true;
					}
					else if (packet::is_done(event) || packet::is_error_or_skip(event))
					{
						if (response.content.length <= response.content.offset)
						{
							response.content.finalize();
							if (!response.content.data.empty())
								VI_DEBUG("http fd %i responded\n%.*s", (int)net.stream->get_fd(), (int)response.content.data.size(), response.content.data.data());
						}

						if (packet::is_error_or_skip(event))
							result.set(core::system_exception("download content network error", packet::to_condition(event)));
						else
							result.set(core::expectation::met);
					}

					return true;
				});
				return result;
			}
			core::expects_promise_system<void> client::upgrade(http::request_frame&& target)
			{
				VI_ASSERT(web_socket != nullptr, "websocket should be opened");
				if (!has_stream())
					return core::expects_promise_system<void>(core::system_exception("upgrade error: bad fd", std::make_error_condition(std::errc::bad_file_descriptor)));

				target.set_header("Pragma", "no-cache");
				target.set_header("Upgrade", "WebSocket");
				target.set_header("Connection", "Upgrade");
				target.set_header("Sec-WebSocket-Version", "13");

				auto random = compute::crypto::random_bytes(16);
				if (random)
					target.set_header("Sec-WebSocket-Key", compute::codec::base64_encode(*random));
				else
					target.set_header("Sec-WebSocket-Key", HTTP_WEBSOCKET_KEY);

				return send(std::move(target)).then<core::expects_promise_system<void>>([this](core::expects_system<void>&& status) -> core::expects_promise_system<void>
				{
					VI_DEBUG("ws handshake %s", request.location.c_str());
					if (!status)
						return core::expects_promise_system<void>(status.error());

					if (response.status_code != 101)
						return core::expects_promise_system<void>(core::system_exception("upgrade handshake status error", std::make_error_condition(std::errc::protocol_error)));

					if (response.get_header("Sec-WebSocket-Accept").empty())
						return core::expects_promise_system<void>(core::system_exception("upgrade handshake accept error", std::make_error_condition(std::errc::bad_message)));

					future = core::expects_promise_system<void>();
					web_socket->next();
					return future;
				});
			}
			core::expects_promise_system<void> client::send(http::request_frame&& target)
			{
				VI_ASSERT(!web_socket || !target.get_header("Sec-WebSocket-Key").empty(), "cannot send http request over websocket");
				if (!has_stream())
					return core::expects_promise_system<void>(core::system_exception("send error: bad fd", std::make_error_condition(std::errc::bad_file_descriptor)));

				VI_DEBUG("http %s %s", target.method, target.location.c_str());
				if (!response.content.is_finalized() || response.content.exceeds)
					return core::expects_promise_system<void>(core::system_exception("content error: response body was not read", std::make_error_condition(std::errc::broken_pipe)));

				core::expects_promise_system<void> result;
				request = std::move(target);
				response.cleanup();
				state.resolver = [this, result](core::expects_system<void>&& status) mutable
				{
					if (!status)
						response.status_code = -1;
					result.set(std::move(status));
				};

				if (request.get_header("Host").empty())
				{
					auto hostname = state.address.get_hostname();
					if (hostname)
					{
						auto port = state.address.get_ip_port();
						if (port && *port != (net.stream->is_secure() ? 443 : 80))
							request.set_header("Host", (*hostname + ':' + core::to_string(*port)));
						else
							request.set_header("Host", *hostname);
					}
				}

				if (request.get_header("Accept").empty())
					request.set_header("Accept", "*/*");

				if (request.get_header("Content-Length").empty())
				{
					request.content.length = request.content.data.size();
					request.set_header("Content-Length", core::to_string(request.content.data.size()));
				}

				if (request.get_header("Connection").empty())
					request.set_header("Connection", "Keep-Alive");

				auto* content = hrm_cache::get()->pop();
				if (request.location.empty())
					request.location.assign("/");

				if (!request.query.empty())
				{
					content->append(request.method).append(" ");
					content->append(request.location).append("?");
					content->append(request.query).append(" ");
					content->append(request.version).append("\r\n");
				}
				else
				{
					content->append(request.method).append(" ");
					content->append(request.location).append(" ");
					content->append(request.version).append("\r\n");
				}

				if (request.content.resources.empty())
				{
					if (!request.content.data.empty())
					{
						if (request.get_header("Content-Type").empty())
							request.set_header("Content-Type", "application/octet-stream");

						if (request.get_header("Content-Length").empty())
							request.set_header("Content-Length", core::to_string(request.content.data.size()).c_str());
					}
					else if (!memcmp(request.method, "POST", 4) || !memcmp(request.method, "PUT", 3) || !memcmp(request.method, "PATCH", 5))
						request.set_header("Content-Length", "0");

					paths::construct_head_full(&request, &response, true, *content);
					content->append("\r\n");

					net.stream->write_queued((uint8_t*)content->c_str(), content->size(), [this, content](socket_poll event)
					{
						hrm_cache::get()->push(content);
						if (packet::is_done(event))
						{
							if (!request.content.data.empty())
							{
								net.stream->write_queued((uint8_t*)request.content.data.data(), request.content.data.size(), [this](socket_poll event)
								{
									if (packet::is_done(event))
									{
										net.stream->read_until_chunked_queued("\r\n\r\n", [this](socket_poll event, const uint8_t* buffer, size_t recv)
										{
											if (packet::is_data(event))
												response.content.append(std::string_view((char*)buffer, recv));
											else if (packet::is_done(event) || packet::is_error_or_skip(event))
												receive(event, buffer, recv);
											return true;
										});
									}
									else if (packet::is_error_or_skip(event))
										report(core::system_exception(event == socket_poll::timeout ? "write timeout error" : "write abort error", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
								}, false);
							}
							else
							{
								net.stream->read_until_chunked_queued("\r\n\r\n", [this](socket_poll event, const uint8_t* buffer, size_t recv)
								{
									if (packet::is_data(event))
										response.content.append(std::string_view((char*)buffer, recv));
									else if (packet::is_done(event) || packet::is_error_or_skip(event))
										receive(event, buffer, recv);
									return true;
								});
							}
						}
						else if (packet::is_error_or_skip(event))
							report(core::system_exception(event == socket_poll::timeout ? "write timeout error" : "write abort error", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
					}, false);
				}
				else
				{
					auto random_bytes = compute::crypto::random_bytes(24);
					if (!random_bytes)
					{
						report(core::system_exception("send boundary error: " + random_bytes.error().message(), std::make_error_condition(std::errc::operation_canceled)));
						hrm_cache::get()->push(content);
						return result;
					}

					core::string boundary = "----0x" + compute::codec::hex_encode(*random_bytes);
					request.set_header("Content-Type", "multipart/form-data; boundary=" + boundary);
					boundary.insert(0, "--");

					boundaries.clear();
					boundaries.reserve(request.content.resources.size());

					size_t content_size = 0;
					for (auto& item : request.content.resources)
					{
						if (!item.is_in_memory && item.name.empty())
							item.name = core::os::path::get_filename(item.path.c_str());
						if (item.type.empty())
							item.type = "application/octet-stream";
						if (item.key.empty())
							item.key = "file" + core::to_string(boundaries.size() + 1);

						boundary_block block;
						block.finish = "\r\n";
						block.file = &item;
						block.is_final = (boundaries.size() + 1 == request.content.resources.size());

						for (auto& header : item.headers)
						{
							block.data.append(boundary).append("\r\n");
							block.data.append("Content-disposition: form-data; name=\"").append(header.first).append("\"\r\n\r\n");
							for (auto& value : header.second)
								block.data.append(value);
							block.data.append("\r\n\r\n").append(boundary).append("\r\n");
						}

						block.finish.append(boundary);
						if (block.is_final)
							block.finish.append("--\r\n");
						else
							block.finish.append("\r\n");

						block.data.append(boundary).append("\r\n");
						block.data.append("Content-disposition: form-data; name=\"").append(item.key).append("\"; filename=\"").append(item.name).append("\"\r\n");
						block.data.append("Content-Type: ").append(item.type).append("\r\n\r\n");

						if (!item.is_in_memory)
						{
							auto state = core::os::file::get_state(item.path);
							if (state && !state->is_directory)
								item.length = state->size;
							else
								item.length = 0;
							content_size += block.data.size() + item.length + block.finish.size();
						}
						else
						{
							item.length = item.get_in_memory_contents().size();
							block.data.append(item.get_in_memory_contents());
							block.data.append(block.finish);
							item.path.clear();
							item.path.shrink_to_fit();
							content_size += block.data.size();
						}

						boundaries.emplace_back(std::move(block));
					}

					request.set_header("Content-Length", core::to_string(content_size));
					paths::construct_head_full(&request, &response, true, *content);
					content->append("\r\n");

					net.stream->write_queued((uint8_t*)content->c_str(), content->size(), [this, content](socket_poll event)
					{
						hrm_cache::get()->push(content);
						if (packet::is_done(event))
							upload(0);
						else if (packet::is_error_or_skip(event))
							report(core::system_exception(event == socket_poll::timeout ? "write timeout error" : "write abort error", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
					}, false);
				}

				return result;
			}
			core::expects_promise_system<void> client::send_fetch(http::request_frame&& target, size_t max_size)
			{
				return send(std::move(target)).then<core::expects_promise_system<void>>([this, max_size](core::expects_system<void>&& response) -> core::expects_promise_system<void>
				{
					if (!response)
						return response;

					return fetch(max_size);
				});
			}
			core::expects_promise_system<core::schema*> client::json(http::request_frame&& target, size_t max_size)
			{
				return send_fetch(std::move(target), max_size).then<core::expects_system<core::schema*>>([this](core::expects_system<void>&& status) -> core::expects_system<core::schema*>
				{
					if (!status)
						return status.error();

					auto data = core::schema::convert_from_json(std::string_view(response.content.data.data(), response.content.data.size()));
					if (!data)
						return core::system_exception(data.error().message(), std::make_error_condition(std::errc::bad_message));

					return *data;
				});
			}
			core::expects_promise_system<core::schema*> client::xml(http::request_frame&& target, size_t max_size)
			{
				return send_fetch(std::move(target), max_size).then<core::expects_system<core::schema*>>([this](core::expects_system<void>&& status) -> core::expects_system<core::schema*>
				{
					if (!status)
						return status.error();

					auto data = core::schema::convert_from_xml(std::string_view(response.content.data.data(), response.content.data.size()));
					if (!data)
						return core::system_exception(data.error().message(), std::make_error_condition(std::errc::bad_message));

					return *data;
				});
			}
			core::expects_system<void> client::on_reuse()
			{
				response.content.cleanup();
				response.content.finalize();
				report(core::expectation::met);
				return core::expectation::met;
			}
			core::expects_system<void> client::on_disconnect()
			{
				response.content.cleanup();
				response.content.finalize();
				report(core::expectation::met);
				return core::expectation::met;
			}
			void client::downgrade()
			{
				VI_ASSERT(web_socket != nullptr, "websocket should be opened");
				VI_ASSERT(web_socket->is_finished(), "websocket connection should be finished");
				core::memory::release(web_socket);
			}
			web_socket_frame* client::get_web_socket()
			{
				if (web_socket != nullptr)
					return web_socket;

				web_socket = new web_socket_frame(net.stream, this);
				web_socket->lifetime.dead = [](web_socket_frame*)
				{
					return false;
				};
				web_socket->lifetime.close = [this](web_socket_frame*, bool successful)
				{
					if (!successful)
					{
						net.stream->shutdown();
						future.set(core::system_exception("ws connection abort error", std::make_error_condition(std::errc::connection_reset)));
					}
					else
						future.set(core::expectation::met);
				};

				return web_socket;
			}
			request_frame* client::get_request()
			{
				return &request;
			}
			response_frame* client::get_response()
			{
				return &response;
			}
			void client::upload_file(boundary_block* boundary, std::function<void(core::expects_system<void>&&)>&& callback)
			{
				auto file = core::os::file::open(boundary->file->path.c_str(), "rb");
				if (!file)
					return callback(core::system_exception("upload file error", std::move(file.error())));

				FILE* file_stream = *file;
				auto result = net.stream->write_file_queued(file_stream, 0, boundary->file->length, [file_stream, callback](socket_poll event)
				{
					if (packet::is_done(event))
					{
						core::os::file::close(file_stream);
						callback(core::expectation::met);
					}
					else if (packet::is_error_or_skip(event))
					{
						core::os::file::close(file_stream);
						callback(core::system_exception("upload file network error", std::make_error_condition(std::errc::connection_aborted)));
					}
				});
				if (result || result.error() != std::errc::not_supported)
					return callback(core::expectation::met);

				if (config.is_non_blocking)
					return upload_file_chunk_queued(file_stream, boundary->file->length, std::move(callback));

				return upload_file_chunk(file_stream, boundary->file->length, std::move(callback));
			}
			void client::upload_file_chunk(FILE* file_stream, size_t content_length, std::function<void(core::expects_system<void>&&)>&& callback)
			{
				if (!content_length)
				{
					core::os::file::close(file_stream);
					return callback(core::expectation::met);
				}

				uint8_t buffer[core::BLOB_SIZE];
				while (content_length > 0)
				{
					size_t read = sizeof(buffer);
					if ((read = (size_t)fread(buffer, 1, read > content_length ? content_length : read, file_stream)) <= 0)
					{
						core::os::file::close(file_stream);
						return callback(core::system_exception("upload file io error", core::os::error::get_condition()));
					}

					content_length -= read;
					auto written = net.stream->write(buffer, read);
					if (!written || !*written)
					{
						core::os::file::close(file_stream);
						if (!written)
							return callback(core::system_exception("upload file network error", std::move(written.error())));

						return callback(core::expectation::met);
					}
				}
			}
			void client::upload_file_chunk_queued(FILE* file_stream, size_t content_length, std::function<void(core::expects_system<void>&&)>&& callback)
			{
			retry:
				if (!content_length)
				{
					core::os::file::close(file_stream);
					return callback(core::expectation::met);
				}

				uint8_t buffer[core::BLOB_SIZE];
				size_t read = sizeof(buffer);
				if ((read = (size_t)fread(buffer, 1, read > content_length ? content_length : read, file_stream)) <= 0)
				{
					core::os::file::close(file_stream);
					return callback(core::system_exception("upload file io error", core::os::error::get_condition()));
				}

				content_length -= read;
				auto written = net.stream->write_queued(buffer, read, [this, file_stream, content_length, callback](socket_poll event) mutable
				{
					if (packet::is_done_async(event))
					{
						core::cospawn([this, file_stream, content_length, callback = std::move(callback)]() mutable
						{
							upload_file_chunk_queued(file_stream, content_length, std::move(callback));
						});
					}
					else if (packet::is_error_or_skip(event))
					{
						core::os::file::close(file_stream);
						return callback(core::system_exception("upload file network error", std::make_error_condition(std::errc::connection_aborted)));
					}
				});

				if (written && *written > 0)
					goto retry;
			}
			void client::upload(size_t file_id)
			{
				if (file_id < boundaries.size())
				{
					boundary_block* boundary = &boundaries[file_id];
					net.stream->write_queued((uint8_t*)boundary->data.c_str(), boundary->data.size(), [this, boundary, file_id](socket_poll event)
					{
						if (packet::is_done(event))
						{
							if (!boundary->file->is_in_memory)
							{
								boundary_block& block = *boundary;
								upload_file(&block, [this, boundary, file_id](core::expects_system<void>&& status)
								{
									if (status)
									{
										net.stream->write_queued((uint8_t*)boundary->finish.c_str(), boundary->finish.size(), [this, file_id](socket_poll event)
										{
											if (packet::is_done(event))
												upload(file_id + 1);
											else if (packet::is_error_or_skip(event))
												report(core::system_exception(event == socket_poll::timeout ? "write timeout error" : "write abort error", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
										}, false);
									}
									else
										report(std::move(status));
								});
							}
							else
								upload(file_id + 1);
						}
						else if (packet::is_error_or_skip(event))
							report(core::system_exception(event == socket_poll::timeout ? "write timeout error" : "write abort error", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
					}, false);
				}
				else
				{
					net.stream->read_until_chunked_queued("\r\n\r\n", [this](socket_poll event, const uint8_t* buffer, size_t recv)
					{
						if (packet::is_data(event))
							response.content.append(std::string_view((char*)buffer, recv));
						else if (packet::is_done(event) || packet::is_error_or_skip(event))
							receive(event, buffer, recv);
						return true;
					});
				}
			}
			void client::manage_keep_alive()
			{
				auto connection = response.headers.find("Connection");
				if (connection == response.headers.end())
					return enable_reusability();

				if (connection->second.size() != 1 || !core::stringify::case_equals(connection->second.front(), "keep-alive"))
					return disable_reusability();

				auto alt_svc = response.headers.find("Alt-Svc");
				if (alt_svc == response.headers.end())
					return enable_reusability();

				const char prefix[] = "=\":";
				char hostname[sizeof(prefix) + core::NUMSTR_SIZE];
				memcpy(hostname, prefix, sizeof(prefix) - 1);

				auto port = state.address.get_ip_port();
				auto service = port ? core::to_string_view<uint16_t>(hostname + (sizeof(prefix) - 1), sizeof(hostname) - (sizeof(prefix) - 1), *port) : std::string_view();
				service = std::string_view(hostname, (sizeof(prefix) - 1) + service.size());

				for (auto& command : alt_svc->second)
				{
					size_t prefix_index = command.find('h');
					if (prefix_index == std::string::npos || ++prefix_index + 1 >= command.size())
						continue;
					else if (command[prefix_index] != '2' && command[prefix_index] != '3')
						continue;
					else if (command[prefix_index + 1] == '-')
						while (++prefix_index < command.size() && core::stringify::is_numeric(command[prefix_index]));
					if (command.find(service, prefix_index) != std::string::npos)
						return enable_reusability();
				}

				return disable_reusability();
			}
			void client::receive(socket_poll event, const uint8_t* leftover_buffer, size_t leftover_size)
			{
				resolver->prepare_for_response_parsing(&response);
				if (resolver->parse_response((uint8_t*)response.content.data.data(), response.content.data.size(), 0) >= 0)
				{
					response.content.prepare(response.headers, leftover_buffer, leftover_size);
					manage_keep_alive();
					report(core::expectation::met);
				}
				else if (packet::is_error_or_skip(event))
					report(core::system_exception(event == socket_poll::timeout ? "read timeout error" : "read abort error", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
				else
					report(core::system_exception(core::stringify::text("http chunk parse error: %.*s ...", (int)std::min<size_t>(64, response.content.data.size()), response.content.data.data()), std::make_error_condition(std::errc::bad_message)));
			}

			core::expects_promise_system<response_frame> fetch(const std::string_view& location, const std::string_view& method, const fetch_frame& options)
			{
				network::location origin(location);
				if (origin.protocol != "http" && origin.protocol != "https")
					return core::expects_promise_system<response_frame>(core::system_exception("http fetch: invalid protocol", std::make_error_condition(std::errc::address_family_not_supported)));

				http::request_frame request;
				request.cookies = options.cookies;
				request.headers = options.headers;
				request.content = options.content;
				request.location.assign(origin.path);
				request.set_method(method);
				if (!origin.username.empty() || !origin.password.empty())
					request.set_header("Authorization", permissions::authorize(origin.username, origin.password));

				for (auto& item : origin.query)
					request.query += item.first + "=" + item.second + "&";
				if (!request.query.empty())
					request.query.pop_back();

				size_t max_size = options.max_size;
				uint64_t timeout = options.timeout;
				bool secure = origin.protocol == "https";
				core::string hostname = origin.hostname;
				core::string port = origin.port > 0 ? core::to_string(origin.port) : core::string(secure ? "443" : "80");
				int32_t verify_peers = (secure ? (options.verify_peers >= 0 ? options.verify_peers : PEER_NOT_VERIFIED) : PEER_NOT_SECURE);
				return dns::get()->lookup_deferred(hostname, port, dns_type::connect, socket_protocol::TCP, socket_type::stream).then<core::expects_promise_system<response_frame>>([max_size, timeout, verify_peers, request = std::move(request), origin = std::move(origin)](core::expects_system<socket_address>&& address) mutable -> core::expects_promise_system<response_frame>
				{
					if (!address)
						return core::expects_promise_system<response_frame>(address.error());

					http::client* client = new http::client(timeout);
					return client->connect_async(*address, verify_peers).then<core::expects_promise_system<void>>([client, max_size, request = std::move(request)](core::expects_system<void>&& status) mutable -> core::expects_promise_system<void>
					{
						if (!status)
							return core::expects_promise_system<void>(status);

						return client->send_fetch(std::move(request), max_size);
					}).then<core::expects_promise_system<response_frame>>([client](core::expects_system<void>&& status) -> core::expects_promise_system<response_frame>
					{
						if (!status)
						{
							client->release();
							return core::expects_promise_system<response_frame>(status.error());
						}

						auto response = std::move(*client->get_response());
						return client->disconnect().then<core::expects_system<response_frame>>([client, response = std::move(response)](core::expects_system<void>&&) mutable -> core::expects_system<response_frame>
						{
							client->release();
							return std::move(response);
						});
					});
				});
			}
		}
	}
}
#pragma warning(pop)