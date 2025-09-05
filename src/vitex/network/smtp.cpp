#include "smtp.h"
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
#endif
extern "C"
{
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

namespace vitex
{
	namespace network
	{
		namespace smtp
		{
			client::client(const std::string_view& domain, int64_t read_timeout) : socket_client(read_timeout), attachment_file(nullptr), hoster(domain), pending(0), authorized(false)
			{
				config.is_auto_handshake = false;
			}
			core::expects_promise_system<void> client::send(request_frame&& root)
			{
				if (!has_stream())
					return core::expects_promise_system<void>(core::system_exception("send failed", std::make_error_condition(std::errc::bad_file_descriptor)));

				core::expects_promise_system<void> result;
				if (&request != &root)
					request = std::move(root);

				VI_DEBUG("smtp message to %s", root.receiver.c_str());
				state.resolver = [this, result](core::expects_system<void>&& status) mutable
				{
					if (!buffer.empty())
						VI_DEBUG("smtp fd %i responded\n%.*s", (int)net.stream->get_fd(), (int)buffer.size(), buffer.data());

					buffer.clear();
					result.set(std::move(status));
				};
				if (!authorized && request.authenticate && can_request("AUTH"))
					authorize(std::bind(&client::prepare_and_send, this));
				else
					prepare_and_send();

				return result;
			}
			core::expects_system<void> client::on_connect()
			{
				authorized = false;
				read_response(220, [this]()
				{
					send_request(250, core::stringify::text("EHLO %s\r\n", hoster.empty() ? "domain" : hoster.c_str()), [this]()
					{
						if (this->is_secure())
						{
							if (!can_request("STARTTLS"))
								return report(core::system_exception("connect tls failed", std::make_error_condition(std::errc::protocol_not_supported)));

							send_request(220, "STARTTLS\r\n", [this]()
							{
								handshake([this](core::expects_system<void>&& status)
								{
									if (!status)
										return report(std::move(status));

									send_request(250, core::stringify::text("EHLO %s\r\n", hoster.empty() ? "domain" : hoster.c_str()), [this]()
									{
										report(core::expectation::met);
									});
								});
							});
						}
						else
							report(core::expectation::met);
					});
				});
				return core::expectation::met;
			}
			core::expects_system<void> client::on_disconnect()
			{
				send_request(221, "QUIT\r\n", [this]()
				{
					authorized = false;
					report(core::expectation::met);
				});
				return core::expectation::met;
			}
			bool client::read_responses(int code, reply_callback&& callback)
			{
				return read_response(code, [this, callback = std::move(callback), code]() mutable
				{
					if (--pending <= 0)
					{
						pending = 0;
						if (callback)
							callback();
					}
					else
						read_responses(code, std::move(callback));
				});
			}
			bool client::read_response(int code, reply_callback&& callback)
			{
				command.clear();
				return net.stream->read_until_queued("\r\n", [this, callback = std::move(callback), code](socket_poll event, const uint8_t* data, size_t recv) mutable
				{
					if (packet::is_data(event))
					{
						buffer.append((char*)data, recv);
						command.append((char*)data, recv);
					}
					else if (packet::is_done(event))
					{
						if (!isdigit(command[0]) || !isdigit(command[1]) || !isdigit(command[2]))
						{
							report(core::system_exception("receive response: bad command", std::make_error_condition(std::errc::bad_message)));
							return false;
						}

						if (command[3] != ' ')
							return read_response(code, std::move(callback));

						int reply_code = (command[0] - '0') * 100 + (command[1] - '0') * 10 + command[2] - '0';
						if (reply_code != code)
						{
							report(core::system_exception("receiving unexpected response code: " + core::to_string(reply_code) + " is not " + core::to_string(code), std::make_error_condition(std::errc::bad_message)));
							return false;
						}

						if (callback)
							callback();
					}
					else if (packet::is_error_or_skip(event))
					{
						report(core::system_exception("receive response failed", std::make_error_condition(std::errc::connection_aborted)));
						return false;
					}

					return true;
				}) || true;
			}
			bool client::send_request(int code, const std::string_view& content, reply_callback&& callback)
			{
				return net.stream->write_queued((uint8_t*)content.data(), content.size(), [this, callback = std::move(callback), code](socket_poll event) mutable
				{
					if (packet::is_done(event))
						read_response(code, std::move(callback));
					else if (packet::is_error_or_skip(event))
						report(core::system_exception("write failed", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
				}) || true;
			}
			bool client::can_request(const std::string_view& keyword)
			{
				size_t L1 = buffer.size(), L2 = keyword.size();
				if (L1 < L2)
					return false;

				for (size_t i = 0; i < L1 - L2 + 1; i++)
				{
#ifdef VI_MICROSOFT
					if (_strnicmp(keyword.data(), buffer.c_str() + i, L2) != 0 || !i)
						continue;
#else
					if (strncmp(keyword.data(), buffer.c_str() + i, L2) != 0 || !i)
						continue;
#endif
					if (buffer[i - 1] != '-' && buffer[i - 1] != ' ' && buffer[i - 1] != '=')
						continue;

					if (i + L2 >= L1)
						continue;

					if (buffer[i + L2] == ' ' || buffer[i + L2] == '=')
						return true;

					if (i + L2 + 1 >= L1)
						continue;

					if (buffer[i + L2] == '\r' && buffer[i + L2 + 1] == '\n')
						return true;
				}
				return false;
			}
			bool client::authorize(reply_callback&& callback)
			{
				if (!callback || authorized)
				{
					if (callback)
						callback();

					return true;
				}

				if (request.login.empty())
				{
					report(core::system_exception("smtp authorize: invalid login", std::make_error_condition(std::errc::invalid_argument)));
					return false;
				}

				if (request.password.empty())
				{
					report(core::system_exception("smtp authorize: invalid password", std::make_error_condition(std::errc::invalid_argument)));
					return false;
				}

				if (can_request("LOGIN"))
				{
					return send_request(334, "AUTH LOGIN\r\n", [this, callback = std::move(callback)]()
					{
						core::string hash = compute::codec::base64_encode(request.login);
						send_request(334, hash.append("\r\n"), [this, callback = std::move(callback)]()
						{
							core::string hash = compute::codec::base64_encode(request.password);
							send_request(235, hash.append("\r\n"), [this, callback = std::move(callback)]()
							{
								authorized = true;
								callback();
							});
						});
					});
				}
				else if (can_request("PLAIN"))
				{
					core::string hash = core::stringify::text("%s^%s^%s", request.login.c_str(), request.login.c_str(), request.password.c_str());
					char* escape = (char*)hash.c_str();

					for (size_t i = 0; i < hash.size(); i++)
					{
						if (escape[i] == 94)
							escape[i] = 0;
					}

					return send_request(235, core::stringify::text("AUTH PLAIN %s\r\n", compute::codec::base64_encode(hash).c_str()), [this, callback = std::move(callback)]()
					{
						authorized = true;
						callback();
					});
				}
				else if (can_request("CRAM-MD5"))
				{
					return send_request(334, "AUTH CRAM-MD5\r\n", [this, callback = std::move(callback)]()
					{
						core::string encoded_challenge = command.c_str() + 4;
						core::string decoded_challenge = compute::codec::base64_decode(encoded_challenge);
						uint8_t* user_challenge = unicode(decoded_challenge.c_str());
						uint8_t* user_password = unicode(request.password.c_str());

						if (!user_challenge || !user_password)
						{
							core::memory::deallocate(user_challenge);
							core::memory::deallocate(user_password);
							return report(core::system_exception("smtp authorize: invalid challenge", std::make_error_condition(std::errc::invalid_argument)));
						}

						size_t password_length = request.password.size();
						if (password_length > 64)
						{
							compute::md5_hasher password_md5;
							password_md5.update(user_password, (uint32_t)password_length);
							password_md5.finalize();

							core::memory::deallocate(user_password);
							user_password = password_md5.raw_digest();
							password_length = 16;
						}

						uint8_t ipad[65] = { };
						uint8_t opad[65] = { };
						memcpy(ipad, user_password, (size_t)password_length);
						memcpy(opad, user_password, (size_t)password_length);

						for (size_t i = 0; i < 64; i++)
						{
							ipad[i] ^= 0x36;
							opad[i] ^= 0x5c;
						}

						compute::md5_hasher md5_pass1;
						md5_pass1.update(ipad, 64);
						md5_pass1.update(user_challenge, (uint32_t)decoded_challenge.size());
						md5_pass1.finalize();
						uint8_t* user_result = md5_pass1.raw_digest();

						compute::md5_hasher md5_pass2;
						md5_pass2.update(opad, 64);
						md5_pass2.update(user_result, 16);
						md5_pass2.finalize();
						const char* user_base = md5_pass2.hex_digest();

						core::memory::deallocate(user_challenge);
						core::memory::deallocate(user_password);
						core::memory::deallocate(user_result);

						decoded_challenge = request.login + ' ' + user_base;
						encoded_challenge = compute::codec::base64_encode(decoded_challenge);

						core::memory::deallocate((void*)user_base);
						send_request(235, core::stringify::text("%s\r\n", encoded_challenge.c_str()), [this, callback = std::move(callback)]()
						{
							authorized = true;
							callback();
						});
					});
				}
				else if (can_request("DIGEST-MD5"))
				{
					return send_request(334, "AUTH DIGEST-MD5\r\n", [this, callback = std::move(callback)]()
					{
						core::string encoded_challenge = command.c_str() + 4;
						core::string decoded_challenge = compute::codec::base64_decode(encoded_challenge);

						core::text_settle result1 = core::stringify::find(decoded_challenge, "nonce");
						if (!result1.found)
							return report(core::system_exception("smtp authorize: bad digest", std::make_error_condition(std::errc::bad_message)));

						core::text_settle result2 = core::stringify::find(decoded_challenge, "\"", result1.start + 7);
						if (!result2.found)
							return report(core::system_exception("smtp authorize: bad digest", std::make_error_condition(std::errc::bad_message)));

						core::string nonce = decoded_challenge.substr(result1.start + 7, result2.start - (result1.start + 7));
						core::string realm;

						result1 = core::stringify::find(decoded_challenge, "realm");
						if (result1.found)
						{
							result2 = core::stringify::find(decoded_challenge, "\"", result1.start + 7);
							if (!result2.found)
								return report(core::system_exception("smtp authorize: bad digest", std::make_error_condition(std::errc::bad_message)));

							realm = decoded_challenge.substr(result1.start + 7, result2.start - (result1.start + 7));
						}

						char cnonce[17], NC[9];
						snprintf(cnonce, 17, "%x", (uint32_t)time(nullptr));
						snprintf(NC, 9, "%08d", 1);

						struct sockaddr_storage storage;
						socket_size_t length = sizeof(storage);

						if (!getpeername(net.stream->get_fd(), (struct sockaddr*)&storage, &length))
							return report(core::system_exception("smtp authorize: no peer name", std::make_error_condition(std::errc::identifier_removed)));

						char inet_address[INET_ADDRSTRLEN];
						sockaddr_in* address = (sockaddr_in*)&storage;
						core::string location = "smtp/";

						if (inet_ntop(AF_INET, &(address->sin_addr), inet_address, INET_ADDRSTRLEN) != nullptr)
							location += inet_address;
						else
							location += "127.0.0.1";

						core::uptr<uint8_t> user_realm = unicode(realm.c_str());
						core::uptr<uint8_t> user_username = unicode(request.login.c_str());
						core::uptr<uint8_t> user_password = unicode(request.password.c_str());
						core::uptr<uint8_t> user_nonce = unicode(nonce.c_str());
						core::uptr<uint8_t> user_cnonce = unicode(cnonce);
						core::uptr<uint8_t> user_location = unicode(location.c_str());
						core::uptr<uint8_t> user_nc = unicode(NC);
						core::uptr<uint8_t> user_qop = unicode("auth");
						if (!user_realm || !user_username || !user_password || !user_nonce || !user_cnonce || !user_location || !user_nc || !user_qop)
							return report(core::system_exception("smtp authorize: denied", std::make_error_condition(std::errc::permission_denied)));

						compute::md5_hasher MD5A1A;
						MD5A1A.update(*user_username, (uint32_t)request.login.size());
						MD5A1A.update((uint8_t*)":", 1);
						MD5A1A.update(*user_realm, (uint32_t)realm.size());
						MD5A1A.update((uint8_t*)":", 1);
						MD5A1A.update(*user_password, (uint32_t)request.password.size());
						MD5A1A.finalize();

						core::uptr<uint8_t> user_a1a = MD5A1A.raw_digest();
						compute::md5_hasher MD5A1B;
						MD5A1B.update(*user_a1a, 16);
						MD5A1B.update((uint8_t*)":", 1);
						MD5A1B.update(*user_nonce, (uint32_t)nonce.size());
						MD5A1B.update((uint8_t*)":", 1);
						MD5A1B.update(*user_cnonce, (uint32_t)strlen(cnonce));
						MD5A1B.finalize();

						core::uptr<char> user_a1b = MD5A1B.hex_digest();
						compute::md5_hasher MD5A2;
						MD5A2.update((uint8_t*)"AUTHENTICATE:", 13);
						MD5A2.update(*user_location, (uint32_t)location.size());
						MD5A2.finalize();

						core::uptr<char> user_a2a = MD5A2.hex_digest();
						core::uptr<uint8_t> user_a2b = unicode(*user_a2a);
						user_a1a = unicode(*user_a1b);

						compute::md5_hasher md5_pass;
						md5_pass.update(*user_a1a, 32);
						md5_pass.update((uint8_t*)":", 1);
						md5_pass.update(*user_nonce, (uint32_t)nonce.size());
						md5_pass.update((uint8_t*)":", 1);
						md5_pass.update(*user_nc, (uint32_t)strlen(NC));
						md5_pass.update((uint8_t*)":", 1);
						md5_pass.update(*user_cnonce, (uint32_t)strlen(cnonce));
						md5_pass.update((uint8_t*)":", 1);
						md5_pass.update(*user_qop, 4);
						md5_pass.update((uint8_t*)":", 1);
						md5_pass.update(*user_a2b, 32);
						md5_pass.finalize();

						core::uptr<char> decoded_challenge_digest = md5_pass.hex_digest();
						decoded_challenge = *decoded_challenge_digest;

						core::string content;
						if (strstr(command.c_str(), "charset") != nullptr)
							core::stringify::append(content, "charset=utf-8,username=\"%s\"", request.login.c_str());
						else
							core::stringify::append(content, "username=\"%s\"", request.login.c_str());

						if (!realm.empty())
							core::stringify::append(content, ",realm=\"%s\"", realm.c_str());

						core::stringify::append(content, ",nonce=\"%s\""
							",nc=%s"
							",cnonce=\"%s\""
							",digest-uri=\"%s\""
							",response=%s"
							",qop=auth", nonce.c_str(), NC, cnonce, location.c_str(), decoded_challenge.c_str());

						encoded_challenge = compute::codec::base64_encode(content);
						send_request(334, core::stringify::text("%s\r\n", encoded_challenge.c_str()), [this, callback = std::move(callback)]()
						{
							send_request(235, "\r\n", [this, callback = std::move(callback)]()
							{
								authorized = true;
								callback();
							});
						});
					});
				}

				report(core::system_exception("smtp authorize: type not supported", std::make_error_condition(std::errc::not_supported)));
				return false;
			}
			bool client::prepare_and_send()
			{
				if (request.sender_address.empty())
				{
					report(core::system_exception("smtp send: invalid sender address", std::make_error_condition(std::errc::invalid_argument)));
					return false;
				}

				if (request.recipients.empty())
				{
					report(core::system_exception("smtp send: invalid recipient address", std::make_error_condition(std::errc::invalid_argument)));
					return false;
				}

				if (!request.attachments.empty())
				{
					auto random = compute::crypto::random_bytes(64);
					if (random)
					{
						auto hash = compute::crypto::hash_hex(compute::digests::md5(), *random);
						if (hash)
							boundary = *hash;
						else
							boundary = "0x00000000";
					}
					else
						boundary = "0x00000000";
				}

				core::string content;
				core::stringify::append(content, "MAIL FROM: <%s>\r\n", request.sender_address.c_str());

				for (auto& item : request.recipients)
					core::stringify::append(content, "RCPT TO: <%s>\r\n", item.address.c_str());

				for (auto& item : request.cc_recipients)
					core::stringify::append(content, "RCPT TO: <%s>\r\n", item.address.c_str());

				for (auto& item : request.bcc_recipients)
					core::stringify::append(content, "RCPT TO: <%s>\r\n", item.address.c_str());

				net.stream->write_queued((uint8_t*)content.data(), content.size(), [this](socket_poll event)
				{
					if (packet::is_done(event))
					{
						pending = (int32_t)(1 + request.recipients.size() + request.cc_recipients.size() + request.bcc_recipients.size());
						read_responses(250, [this]()
						{
							pending = (int32_t)request.attachments.size();
							send_request(354, "DATA\r\n", [this]()
							{
								char date[64];
								core::string content = "Date: ";
								content.append(core::date_time::serialize_global(date, sizeof(date), core::date_time::now(), core::date_time::format_web_time()));
								content.append("\r\nFrom: ");

								if (!request.sender_name.empty())
									content.append(request.sender_name.c_str());

								core::stringify::append(content, " <%s>\r\n", request.sender_address.c_str());
								for (auto& item : request.headers)
									core::stringify::append(content, "%s: %s\r\n", item.first.c_str(), item.second.c_str());

								if (!request.mailer.empty())
									core::stringify::append(content, "X-mailer: %s\r\n", request.mailer.c_str());

								if (!request.receiver.empty())
									core::stringify::append(content, "Reply-to: %s\r\n", request.receiver.c_str());

								if (request.no_notification)
									core::stringify::append(content, "Disposition-notification-to: %s\r\n", !request.receiver.empty() ? request.receiver.c_str() : request.sender_name.c_str());

								switch (request.prior)
								{
									case priority::high:
										content.append("X-priority: 2 (high)\r\n");
										break;
									case priority::normal:
										content.append("X-priority: 3 (normal)\r\n");
										break;
									case priority::low:
										content.append("X-priority: 4 (low)\r\n");
										break;
									default:
										content.append("X-priority: 3 (normal)\r\n");
										break;
								}

								core::string recipients;
								for (size_t i = 0; i < request.recipients.size(); i++)
								{
									if (i > 0)
										recipients += ',';

									if (!request.recipients[i].name.empty())
									{
										recipients.append(request.recipients[i].name);
										recipients += ' ';
									}

									recipients += '<';
									recipients.append(request.recipients[i].address);
									recipients += '>';
								}

								core::stringify::append(content, "To: %s\r\n", recipients.c_str());
								if (!request.cc_recipients.empty())
								{
									recipients.clear();
									for (size_t i = 0; i < request.cc_recipients.size(); i++)
									{
										if (i > 0)
											recipients += ',';

										if (!request.cc_recipients[i].name.empty())
										{
											recipients.append(request.cc_recipients[i].name);
											recipients += ' ';
										}

										recipients += '<';
										recipients.append(request.cc_recipients[i].address);
										recipients += '>';
									}
									core::stringify::append(content, "Cc: %s\r\n", recipients.c_str());
								}

								if (!request.bcc_recipients.empty())
								{
									recipients.clear();
									for (size_t i = 0; i < request.bcc_recipients.size(); i++)
									{
										if (i > 0)
											recipients += ',';

										if (!request.bcc_recipients[i].name.empty())
										{
											recipients.append(request.bcc_recipients[i].name);
											recipients += ' ';
										}

										recipients += '<';
										recipients.append(request.bcc_recipients[i].address);
										recipients += '>';
									}
									core::stringify::append(content, "Bcc: %s\r\n", recipients.c_str());
								}

								core::stringify::append(content, "Subject: %s\r\nMIME-version: 1.0\r\n", request.subject.c_str());
								if (!request.attachments.empty())
								{
									core::stringify::append(content, "Content-type: multipart/mixed; boundary=\"%s\"\r\n\r\n", boundary.c_str());
									core::stringify::append(content, "--%s\r\n", boundary.c_str());
								}

								if (request.allow_html)
									core::stringify::append(content, "Content-type: text/html; charset=\"%s\"\r\n", request.charset.c_str());
								else
									core::stringify::append(content, "Content-type: text/plain; charset=\"%s\"\r\n", request.charset.c_str());

								content.append("Content-transfer-encoding: 7bit\r\n\r\n");
								net.stream->write_queued((uint8_t*)content.c_str(), content.size(), [this](socket_poll event)
								{
									if (packet::is_done(event))
									{
										core::string content;
										for (auto& item : request.messages)
											core::stringify::append(content, "%s\r\n", item.c_str());

										if (request.messages.empty())
											content.assign(" \r\n");

										net.stream->write_queued((uint8_t*)content.c_str(), content.size(), [this](socket_poll event)
										{
											if (packet::is_done(event))
												send_attachment();
											else if (packet::is_error_or_skip(event))
												report(core::system_exception("write failed", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
										});
									}
									else if (packet::is_error_or_skip(event))
										report(core::system_exception("write failed", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
								});
							});
						});
					}
					else if (packet::is_error_or_skip(event))
						report(core::system_exception("write failed", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
				});

				return true;
			}
			bool client::send_attachment()
			{
				if (pending <= 0)
				{
					core::string content;
					if (!request.attachments.empty())
						core::stringify::append(content, "\r\n--%s--\r\n", boundary.c_str());

					content.append("\r\n.\r\n");
					return !net.stream->write_queued((uint8_t*)content.c_str(), content.size(), [this](socket_poll event)
					{
						if (packet::is_done(event))
						{
							read_responses(250, [this]()
							{
								report(core::expectation::met);
							});
						}
						else if (packet::is_error_or_skip(event))
							report(core::system_exception("write failed", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
					});
				}

				attachment& it = request.attachments.at(request.attachments.size() - pending);
				const char* name = it.path.c_str();
				size_t id = strlen(name) - 1;

				if (id > 0 && (name[id] == '\\' || name[id] == '/'))
					name = name - 1;

				core::string hash = core::stringify::text("=?UTF-8?b?%s?=", compute::codec::base64_encode(std::string_view(name, id + 1)).c_str());
				core::string content;
				core::stringify::append(content, "--%s\r\n", boundary.c_str());
				core::stringify::append(content, "Content-type: application/x-msdownload; name=\"%s\"\r\n", hash.c_str());
				core::stringify::append(content, "Content-transfer-encoding: base64\r\n");
				core::stringify::append(content, "Content-disposition: attachment; filename=\"%s\"\r\n\r\n", hash.c_str());

				return net.stream->write_queued((uint8_t*)content.c_str(), content.size(), [this, name](socket_poll event)
				{
					if (packet::is_done(event))
					{
						auto file = core::os::file::open(name, "rb");
						if (file)
						{
							attachment_file = *file;
							process_attachment();
						}
						else
							report(core::system_exception("smtp send attachment: open failed", std::move(file.error())));
					}
					else if (packet::is_error_or_skip(event))
						report(core::system_exception("write failed", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
				}) || true;
			}
			bool client::process_attachment()
			{
				char data[core::BLOB_SIZE];
				attachment& it = request.attachments.at(request.attachments.size() - pending);
				size_t count = it.length > core::BLOB_SIZE ? core::BLOB_SIZE : it.length;
				size_t size = (size_t)fread(data, sizeof(char), count, attachment_file);
				if (size != count)
				{
					report(core::system_exception("smtp read attachment failed: " + it.path, std::make_error_condition(std::errc::io_error)));
					return false;
				}

				core::string content = compute::codec::base64_encode(std::string_view(data, size));
				content.append("\r\n");

				it.length -= size;
				if (!it.length)
					core::os::file::close(attachment_file);

				bool send_next = (!it.length);
				return net.stream->write_queued((uint8_t*)content.c_str(), content.size(), [this, send_next](socket_poll event)
				{
					if (packet::is_done(event))
					{
						if (send_next)
							send_attachment();
						else
							process_attachment();
					}
					else if (packet::is_error_or_skip(event))
						report(core::system_exception("write failed", std::make_error_condition(event == socket_poll::timeout ? std::errc::timed_out : std::errc::connection_aborted)));
				}) || true;
			}
			uint8_t* client::unicode(const std::string_view& value)
			{
				size_t length = value.size();
				auto* output = core::memory::allocate<uint8_t>(sizeof(uint8_t) * (length + 1));
				for (size_t i = 0; i < length; i++)
					output[i] = (uint8_t)value[i];

				output[length] = '\0';
				return output;
			}
			request_frame* client::get_request()
			{
				return &request;
			}
		}
	}
}