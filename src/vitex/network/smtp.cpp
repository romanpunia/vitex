#include "smtp.h"
#ifdef VI_MICROSOFT
#include <WS2tcpip.h>
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

namespace Vitex
{
	namespace Network
	{
		namespace SMTP
		{
			Client::Client(const std::string_view& Domain, int64_t ReadTimeout) : SocketClient(ReadTimeout), AttachmentFile(nullptr), Hoster(Domain), Pending(0), Authorized(false)
			{
				Config.IsAutoHandshake = false;
			}
			Core::ExpectsPromiseSystem<void> Client::Send(RequestFrame&& Root)
			{
				if (!HasStream())
					return Core::ExpectsPromiseSystem<void>(Core::SystemException("send error: bad fd", std::make_error_condition(std::errc::bad_file_descriptor)));

				Core::ExpectsPromiseSystem<void> Result;
				if (&Request != &Root)
					Request = std::move(Root);

				VI_DEBUG("[smtp] message to %s", Root.Receiver.c_str());
				State.Resolver = [this, Result](Core::ExpectsSystem<void>&& Status) mutable
				{
					if (!Buffer.empty())
						VI_DEBUG("[smtp] fd %i responded\n%.*s", (int)Net.Stream->GetFd(), (int)Buffer.size(), Buffer.data());

					Buffer.clear();
					Result.Set(std::move(Status));
				};
				if (!Authorized && Request.Authenticate && CanRequest("AUTH"))
					Authorize(std::bind(&Client::PrepareAndSend, this));
				else
					PrepareAndSend();

				return Result;
			}
			Core::ExpectsSystem<void> Client::OnConnect()
			{
				Authorized = false;
				ReadResponse(220, [this]()
				{
					SendRequest(250, Core::Stringify::Text("EHLO %s\r\n", Hoster.empty() ? "domain" : Hoster.c_str()), [this]()
					{
						if (this->IsSecure())
						{
							if (!CanRequest("STARTTLS"))
								return Report(Core::SystemException("connect tls: server has no support", std::make_error_condition(std::errc::protocol_not_supported)));

							SendRequest(220, "STARTTLS\r\n", [this]()
							{
								Handshake([this](Core::ExpectsSystem<void>&& Status)
								{
									if (!Status)
										return Report(std::move(Status));

									SendRequest(250, Core::Stringify::Text("EHLO %s\r\n", Hoster.empty() ? "domain" : Hoster.c_str()), [this]()
									{
										Report(Core::Expectation::Met);
									});
								});
							});
						}
						else
							Report(Core::Expectation::Met);
					});
				});
				return Core::Expectation::Met;
			}
			Core::ExpectsSystem<void> Client::OnDisconnect()
			{
				SendRequest(221, "QUIT\r\n", [this]()
				{
					Authorized = false;
					Report(Core::Expectation::Met);
				});
				return Core::Expectation::Met;
			}
			bool Client::ReadResponses(int Code, ReplyCallback&& Callback)
			{
				return ReadResponse(Code, [this, Callback = std::move(Callback), Code]() mutable
				{
					if (--Pending <= 0)
					{
						Pending = 0;
						if (Callback)
							Callback();
					}
					else
						ReadResponses(Code, std::move(Callback));
				});
			}
			bool Client::ReadResponse(int Code, ReplyCallback&& Callback)
			{
				Command.clear();
				return Net.Stream->ReadUntilQueued("\r\n", [this, Callback = std::move(Callback), Code](SocketPoll Event, const uint8_t* Data, size_t Recv) mutable
				{
					if (Packet::IsData(Event))
					{
						Buffer.append((char*)Data, Recv);
						Command.append((char*)Data, Recv);
					}
					else if (Packet::IsDone(Event))
					{
						if (!isdigit(Command[0]) || !isdigit(Command[1]) || !isdigit(Command[2]))
						{
							Report(Core::SystemException("receive response: bad command", std::make_error_condition(std::errc::bad_message)));
							return false;
						}

						if (Command[3] != ' ')
							return ReadResponse(Code, std::move(Callback));

						int ReplyCode = (Command[0] - '0') * 100 + (Command[1] - '0') * 10 + Command[2] - '0';
						if (ReplyCode != Code)
						{
							Report(Core::SystemException("receiving unexpected response code: " + Core::ToString(ReplyCode) + " is not " + Core::ToString(Code), std::make_error_condition(std::errc::bad_message)));
							return false;
						}

						if (Callback)
							Callback();
					}
					else if (Packet::IsErrorOrSkip(Event))
					{
						Report(Core::SystemException("receive response: aborted", std::make_error_condition(std::errc::connection_aborted)));
						return false;
					}

					return true;
				}) || true;
			}
			bool Client::SendRequest(int Code, const std::string_view& Content, ReplyCallback&& Callback)
			{
				return Net.Stream->WriteQueued((uint8_t*)Content.data(), Content.size(), [this, Callback = std::move(Callback), Code](SocketPoll Event) mutable
				{
					if (Packet::IsDone(Event))
						ReadResponse(Code, std::move(Callback));
					else if (Packet::IsErrorOrSkip(Event))
						Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
				}) || true;
			}
			bool Client::CanRequest(const std::string_view& Keyword)
			{
				size_t L1 = Buffer.size(), L2 = Keyword.size();
				if (L1 < L2)
					return false;

				for (size_t i = 0; i < L1 - L2 + 1; i++)
				{
#ifdef VI_MICROSOFT
					if (_strnicmp(Keyword.data(), Buffer.c_str() + i, L2) != 0 || !i)
						continue;
#else
					if (strncmp(Keyword.data(), Buffer.c_str() + i, L2) != 0 || !i)
						continue;
#endif
					if (Buffer[i - 1] != '-' && Buffer[i - 1] != ' ' && Buffer[i - 1] != '=')
						continue;

					if (i + L2 >= L1)
						continue;

					if (Buffer[i + L2] == ' ' || Buffer[i + L2] == '=')
						return true;

					if (i + L2 + 1 >= L1)
						continue;

					if (Buffer[i + L2] == '\r' && Buffer[i + L2 + 1] == '\n')
						return true;
				}
				return false;
			}
			bool Client::Authorize(ReplyCallback&& Callback)
			{
				if (!Callback || Authorized)
				{
					if (Callback)
						Callback();

					return true;
				}

				if (Request.Login.empty())
				{
					Report(Core::SystemException("smtp authorize: invalid login", std::make_error_condition(std::errc::invalid_argument)));
					return false;
				}

				if (Request.Password.empty())
				{
					Report(Core::SystemException("smtp authorize: invalid password", std::make_error_condition(std::errc::invalid_argument)));
					return false;
				}

				if (CanRequest("LOGIN"))
				{
					return SendRequest(334, "AUTH LOGIN\r\n", [this, Callback = std::move(Callback)]()
					{
						Core::String Hash = Compute::Codec::Base64Encode(Request.Login);
						SendRequest(334, Hash.append("\r\n"), [this, Callback = std::move(Callback)]()
						{
							Core::String Hash = Compute::Codec::Base64Encode(Request.Password);
							SendRequest(235, Hash.append("\r\n"), [this, Callback = std::move(Callback)]()
							{
								Authorized = true;
								Callback();
							});
						});
					});
				}
				else if (CanRequest("PLAIN"))
				{
					Core::String Hash = Core::Stringify::Text("%s^%s^%s", Request.Login.c_str(), Request.Login.c_str(), Request.Password.c_str());
					char* Escape = (char*)Hash.c_str();

					for (size_t i = 0; i < Hash.size(); i++)
					{
						if (Escape[i] == 94)
							Escape[i] = 0;
					}

					return SendRequest(235, Core::Stringify::Text("AUTH PLAIN %s\r\n", Compute::Codec::Base64Encode(Hash).c_str()), [this, Callback = std::move(Callback)]()
					{
						Authorized = true;
						Callback();
					});
				}
				else if (CanRequest("CRAM-MD5"))
				{
					return SendRequest(334, "AUTH CRAM-MD5\r\n", [this, Callback = std::move(Callback)]()
					{
						Core::String EncodedChallenge = Command.c_str() + 4;
						Core::String DecodedChallenge = Compute::Codec::Base64Decode(EncodedChallenge);
						uint8_t* UserChallenge = Unicode(DecodedChallenge.c_str());
						uint8_t* UserPassword = Unicode(Request.Password.c_str());

						if (!UserChallenge || !UserPassword)
						{
							Core::Memory::Deallocate(UserChallenge);
							Core::Memory::Deallocate(UserPassword);
							return Report(Core::SystemException("smtp authorize: invalid challenge", std::make_error_condition(std::errc::invalid_argument)));
						}

						size_t PasswordLength = Request.Password.size();
						if (PasswordLength > 64)
						{
							Compute::MD5Hasher PasswordMD5;
							PasswordMD5.Update(UserPassword, (uint32_t)PasswordLength);
							PasswordMD5.Finalize();

							Core::Memory::Deallocate(UserPassword);
							UserPassword = PasswordMD5.RawDigest();
							PasswordLength = 16;
						}

						uint8_t IPad[65] = { };
						uint8_t OPad[65] = { };
						memcpy(IPad, UserPassword, (size_t)PasswordLength);
						memcpy(OPad, UserPassword, (size_t)PasswordLength);

						for (size_t i = 0; i < 64; i++)
						{
							IPad[i] ^= 0x36;
							OPad[i] ^= 0x5c;
						}

						Compute::MD5Hasher MD5Pass1;
						MD5Pass1.Update(IPad, 64);
						MD5Pass1.Update(UserChallenge, (uint32_t)DecodedChallenge.size());
						MD5Pass1.Finalize();
						uint8_t* UserResult = MD5Pass1.RawDigest();

						Compute::MD5Hasher MD5Pass2;
						MD5Pass2.Update(OPad, 64);
						MD5Pass2.Update(UserResult, 16);
						MD5Pass2.Finalize();
						const char* UserBase = MD5Pass2.HexDigest();

						Core::Memory::Deallocate(UserChallenge);
						Core::Memory::Deallocate(UserPassword);
						Core::Memory::Deallocate(UserResult);

						DecodedChallenge = Request.Login + ' ' + UserBase;
						EncodedChallenge = Compute::Codec::Base64Encode(DecodedChallenge);

						Core::Memory::Deallocate((void*)UserBase);
						SendRequest(235, Core::Stringify::Text("%s\r\n", EncodedChallenge.c_str()), [this, Callback = std::move(Callback)]()
						{
							Authorized = true;
							Callback();
						});
					});
				}
				else if (CanRequest("DIGEST-MD5"))
				{
					return SendRequest(334, "AUTH DIGEST-MD5\r\n", [this, Callback = std::move(Callback)]()
					{
						Core::String EncodedChallenge = Command.c_str() + 4;
						Core::String DecodedChallenge = Compute::Codec::Base64Decode(EncodedChallenge);

						Core::TextSettle Result1 = Core::Stringify::Find(DecodedChallenge, "nonce");
						if (!Result1.Found)
							return Report(Core::SystemException("smtp authorize: bad digest", std::make_error_condition(std::errc::bad_message)));

						Core::TextSettle Result2 = Core::Stringify::Find(DecodedChallenge, "\"", Result1.Start + 7);
						if (!Result2.Found)
							return Report(Core::SystemException("smtp authorize: bad digest", std::make_error_condition(std::errc::bad_message)));

						Core::String Nonce = DecodedChallenge.substr(Result1.Start + 7, Result2.Start - (Result1.Start + 7));
						Core::String Realm;

						Result1 = Core::Stringify::Find(DecodedChallenge, "realm");
						if (Result1.Found)
						{
							Result2 = Core::Stringify::Find(DecodedChallenge, "\"", Result1.Start + 7);
							if (!Result2.Found)
								return Report(Core::SystemException("smtp authorize: bad digest", std::make_error_condition(std::errc::bad_message)));

							Realm = DecodedChallenge.substr(Result1.Start + 7, Result2.Start - (Result1.Start + 7));
						}

						char CNonce[17], NC[9];
						snprintf(CNonce, 17, "%x", (uint32_t)time(nullptr));
						snprintf(NC, 9, "%08d", 1);

						struct sockaddr_storage Storage;
						socket_size_t Length = sizeof(Storage);

						if (!getpeername(Net.Stream->GetFd(), (struct sockaddr*)&Storage, &Length))
							return Report(Core::SystemException("smtp authorize: no peer name", std::make_error_condition(std::errc::identifier_removed)));

						char InetAddress[INET_ADDRSTRLEN];
						sockaddr_in* Address = (sockaddr_in*)&Storage;
						Core::String Location = "smtp/";

						if (inet_ntop(AF_INET, &(Address->sin_addr), InetAddress, INET_ADDRSTRLEN) != nullptr)
							Location += InetAddress;
						else
							Location += "127.0.0.1";

						Core::UPtr<uint8_t> UserRealm = Unicode(Realm.c_str());
						Core::UPtr<uint8_t> UserUsername = Unicode(Request.Login.c_str());
						Core::UPtr<uint8_t> UserPassword = Unicode(Request.Password.c_str());
						Core::UPtr<uint8_t> UserNonce = Unicode(Nonce.c_str());
						Core::UPtr<uint8_t> UserCNonce = Unicode(CNonce);
						Core::UPtr<uint8_t> UserLocation = Unicode(Location.c_str());
						Core::UPtr<uint8_t> UserNc = Unicode(NC);
						Core::UPtr<uint8_t> UserQop = Unicode("auth");
						if (!UserRealm || !UserUsername || !UserPassword || !UserNonce || !UserCNonce || !UserLocation || !UserNc || !UserQop)
							return Report(Core::SystemException("smtp authorize: denied", std::make_error_condition(std::errc::permission_denied)));

						Compute::MD5Hasher MD5A1A;
						MD5A1A.Update(*UserUsername, (uint32_t)Request.Login.size());
						MD5A1A.Update((uint8_t*)":", 1);
						MD5A1A.Update(*UserRealm, (uint32_t)Realm.size());
						MD5A1A.Update((uint8_t*)":", 1);
						MD5A1A.Update(*UserPassword, (uint32_t)Request.Password.size());
						MD5A1A.Finalize();

						Core::UPtr<uint8_t> UserA1A = MD5A1A.RawDigest();
						Compute::MD5Hasher MD5A1B;
						MD5A1B.Update(*UserA1A, 16);
						MD5A1B.Update((uint8_t*)":", 1);
						MD5A1B.Update(*UserNonce, (uint32_t)Nonce.size());
						MD5A1B.Update((uint8_t*)":", 1);
						MD5A1B.Update(*UserCNonce, (uint32_t)strlen(CNonce));
						MD5A1B.Finalize();

						Core::UPtr<char> UserA1B = MD5A1B.HexDigest();
						Compute::MD5Hasher MD5A2;
						MD5A2.Update((uint8_t*)"AUTHENTICATE:", 13);
						MD5A2.Update(*UserLocation, (uint32_t)Location.size());
						MD5A2.Finalize();

						Core::UPtr<char> UserA2A = MD5A2.HexDigest();
						Core::UPtr<uint8_t> UserA2B = Unicode(*UserA2A);
						UserA1A = Unicode(*UserA1B);

						Compute::MD5Hasher MD5Pass;
						MD5Pass.Update(*UserA1A, 32);
						MD5Pass.Update((uint8_t*)":", 1);
						MD5Pass.Update(*UserNonce, (uint32_t)Nonce.size());
						MD5Pass.Update((uint8_t*)":", 1);
						MD5Pass.Update(*UserNc, (uint32_t)strlen(NC));
						MD5Pass.Update((uint8_t*)":", 1);
						MD5Pass.Update(*UserCNonce, (uint32_t)strlen(CNonce));
						MD5Pass.Update((uint8_t*)":", 1);
						MD5Pass.Update(*UserQop, 4);
						MD5Pass.Update((uint8_t*)":", 1);
						MD5Pass.Update(*UserA2B, 32);
						MD5Pass.Finalize();

						Core::UPtr<char> DecodedChallengeDigest = MD5Pass.HexDigest();
						DecodedChallenge = *DecodedChallengeDigest;

						Core::String Content;
						if (strstr(Command.c_str(), "charset") != nullptr)
							Core::Stringify::Append(Content, "charset=utf-8,username=\"%s\"", Request.Login.c_str());
						else
							Core::Stringify::Append(Content, "username=\"%s\"", Request.Login.c_str());

						if (!Realm.empty())
							Core::Stringify::Append(Content, ",realm=\"%s\"", Realm.c_str());

						Core::Stringify::Append(Content, ",nonce=\"%s\""
							",nc=%s"
							",cnonce=\"%s\""
							",digest-uri=\"%s\""
							",Response=%s"
							",qop=auth", Nonce.c_str(), NC, CNonce, Location.c_str(), DecodedChallenge.c_str());

						EncodedChallenge = Compute::Codec::Base64Encode(Content);
						SendRequest(334, Core::Stringify::Text("%s\r\n", EncodedChallenge.c_str()), [this, Callback = std::move(Callback)]()
						{
							SendRequest(235, "\r\n", [this, Callback = std::move(Callback)]()
							{
								Authorized = true;
								Callback();
							});
						});
					});
				}

				Report(Core::SystemException("smtp authorize: type not supported", std::make_error_condition(std::errc::not_supported)));
				return false;	
			}
			bool Client::PrepareAndSend()
			{
				if (Request.SenderAddress.empty())
				{
					Report(Core::SystemException("smtp send: invalid sender address", std::make_error_condition(std::errc::invalid_argument)));
					return false;
				}

				if (Request.Recipients.empty())
				{
					Report(Core::SystemException("smtp send: invalid recipient address", std::make_error_condition(std::errc::invalid_argument)));
					return false;
				}

				if (!Request.Attachments.empty())
				{
					auto Random = Compute::Crypto::RandomBytes(64);
					if (Random)
					{
						auto Hash = Compute::Crypto::HashHex(Compute::Digests::MD5(), *Random);
						if (Hash)
							Boundary = *Hash;
						else
							Boundary = "0x00000000";
					}
					else
						Boundary = "0x00000000";
				}

				Core::String Content;
				Core::Stringify::Append(Content, "MAIL FROM: <%s>\r\n", Request.SenderAddress.c_str());

				for (auto& Item : Request.Recipients)
					Core::Stringify::Append(Content, "RCPT TO: <%s>\r\n", Item.Address.c_str());

				for (auto& Item : Request.CCRecipients)
					Core::Stringify::Append(Content, "RCPT TO: <%s>\r\n", Item.Address.c_str());

				for (auto& Item : Request.BCCRecipients)
					Core::Stringify::Append(Content, "RCPT TO: <%s>\r\n", Item.Address.c_str());

				Net.Stream->WriteQueued((uint8_t*)Content.data(), Content.size(), [this](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
					{
						Pending = (int32_t)(1 + Request.Recipients.size() + Request.CCRecipients.size() + Request.BCCRecipients.size());
						ReadResponses(250, [this]()
						{
							Pending = (int32_t)Request.Attachments.size();
							SendRequest(354, "DATA\r\n", [this]()
							{
								char Date[64];
								Core::String Content = "Date: ";
								Content.append(Core::DateTime::SerializeGlobal(Date, sizeof(Date), Core::DateTime::Now(), Core::DateTime::FormatWebTime()));
								Content.append("\r\nFrom: ");

								if (!Request.SenderName.empty())
									Content.append(Request.SenderName.c_str());

								Core::Stringify::Append(Content, " <%s>\r\n", Request.SenderAddress.c_str());
								for (auto& Item : Request.Headers)
									Core::Stringify::Append(Content, "%s: %s\r\n", Item.first.c_str(), Item.second.c_str());

								if (!Request.Mailer.empty())
									Core::Stringify::Append(Content, "X-Mailer: %s\r\n", Request.Mailer.c_str());

								if (!Request.Receiver.empty())
									Core::Stringify::Append(Content, "Reply-To: %s\r\n", Request.Receiver.c_str());

								if (Request.NoNotification)
									Core::Stringify::Append(Content, "Disposition-Notification-To: %s\r\n", !Request.Receiver.empty() ? Request.Receiver.c_str() : Request.SenderName.c_str());

								switch (Request.Prior)
								{
									case Priority::High:
										Content.append("X-Priority: 2 (High)\r\n");
										break;
									case Priority::Normal:
										Content.append("X-Priority: 3 (Normal)\r\n");
										break;
									case Priority::Low:
										Content.append("X-Priority: 4 (Low)\r\n");
										break;
									default:
										Content.append("X-Priority: 3 (Normal)\r\n");
										break;
								}

								Core::String Recipients;
								for (size_t i = 0; i < Request.Recipients.size(); i++)
								{
									if (i > 0)
										Recipients += ',';

									if (!Request.Recipients[i].Name.empty())
									{
										Recipients.append(Request.Recipients[i].Name);
										Recipients += ' ';
									}

									Recipients += '<';
									Recipients.append(Request.Recipients[i].Address);
									Recipients += '>';
								}

								Core::Stringify::Append(Content, "To: %s\r\n", Recipients.c_str());
								if (!Request.CCRecipients.empty())
								{
									Recipients.clear();
									for (size_t i = 0; i < Request.CCRecipients.size(); i++)
									{
										if (i > 0)
											Recipients += ',';

										if (!Request.CCRecipients[i].Name.empty())
										{
											Recipients.append(Request.CCRecipients[i].Name);
											Recipients += ' ';
										}

										Recipients += '<';
										Recipients.append(Request.CCRecipients[i].Address);
										Recipients += '>';
									}
									Core::Stringify::Append(Content, "Cc: %s\r\n", Recipients.c_str());
								}

								if (!Request.BCCRecipients.empty())
								{
									Recipients.clear();
									for (size_t i = 0; i < Request.BCCRecipients.size(); i++)
									{
										if (i > 0)
											Recipients += ',';

										if (!Request.BCCRecipients[i].Name.empty())
										{
											Recipients.append(Request.BCCRecipients[i].Name);
											Recipients += ' ';
										}

										Recipients += '<';
										Recipients.append(Request.BCCRecipients[i].Address);
										Recipients += '>';
									}
									Core::Stringify::Append(Content, "Bcc: %s\r\n", Recipients.c_str());
								}

								Core::Stringify::Append(Content, "Subject: %s\r\nMIME-Version: 1.0\r\n", Request.Subject.c_str());
								if (!Request.Attachments.empty())
								{
									Core::Stringify::Append(Content, "Content-Type: multipart/mixed; boundary=\"%s\"\r\n\r\n", Boundary.c_str());
									Core::Stringify::Append(Content, "--%s\r\n", Boundary.c_str());
								}

								if (Request.AllowHTML)
									Core::Stringify::Append(Content, "Content-Type: text/html; charset=\"%s\"\r\n", Request.Charset.c_str());
								else
									Core::Stringify::Append(Content, "Content-type: text/plain; charset=\"%s\"\r\n", Request.Charset.c_str());

								Content.append("Content-Transfer-Encoding: 7bit\r\n\r\n");
								Net.Stream->WriteQueued((uint8_t*)Content.c_str(), Content.size(), [this](SocketPoll Event)
								{
									if (Packet::IsDone(Event))
									{
										Core::String Content;
										for (auto& Item : Request.Messages)
											Core::Stringify::Append(Content, "%s\r\n", Item.c_str());

										if (Request.Messages.empty())
											Content.assign(" \r\n");

										Net.Stream->WriteQueued((uint8_t*)Content.c_str(), Content.size(), [this](SocketPoll Event)
										{
											if (Packet::IsDone(Event))
												SendAttachment();
											else if (Packet::IsErrorOrSkip(Event))
												Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
										});
									}
									else if (Packet::IsErrorOrSkip(Event))
										Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
								});
							});
						});
					}
					else if (Packet::IsErrorOrSkip(Event))
						Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
				});

				return true;
			}
			bool Client::SendAttachment()
			{
				if (Pending <= 0)
				{
					Core::String Content;
					if (!Request.Attachments.empty())
						Core::Stringify::Append(Content, "\r\n--%s--\r\n", Boundary.c_str());

					Content.append("\r\n.\r\n");
					return !Net.Stream->WriteQueued((uint8_t*)Content.c_str(), Content.size(), [this](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
						{
							ReadResponses(250, [this]()
							{
								Report(Core::Expectation::Met);
							});
						}
						else if (Packet::IsErrorOrSkip(Event))
							Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
					});
				}

				Attachment& It = Request.Attachments.at(Request.Attachments.size() - Pending);
				const char* Name = It.Path.c_str();
				size_t Id = strlen(Name) - 1;

				if (Id > 0 && (Name[Id] == '\\' || Name[Id] == '/'))
					Name = Name - 1;

				Core::String Hash = Core::Stringify::Text("=?UTF-8?B?%s?=", Compute::Codec::Base64Encode(std::string_view(Name, Id + 1)).c_str());
				Core::String Content;
				Core::Stringify::Append(Content, "--%s\r\n", Boundary.c_str());
				Core::Stringify::Append(Content, "Content-Type: application/x-msdownload; name=\"%s\"\r\n", Hash.c_str());
				Core::Stringify::Append(Content, "Content-Transfer-Encoding: base64\r\n");
				Core::Stringify::Append(Content, "Content-Disposition: attachment; filename=\"%s\"\r\n\r\n", Hash.c_str());

				return Net.Stream->WriteQueued((uint8_t*)Content.c_str(), Content.size(), [this, Name](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
					{
						auto File = Core::OS::File::Open(Name, "rb");
						if (File)
						{
							AttachmentFile = *File;
							ProcessAttachment();
						}
						else
							Report(Core::SystemException("smtp send attachment: open error", std::move(File.Error())));
					}
					else if (Packet::IsErrorOrSkip(Event))
						Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
				}) || true;
			}
			bool Client::ProcessAttachment()
			{
				char Data[Core::BLOB_SIZE];
				Attachment& It = Request.Attachments.at(Request.Attachments.size() - Pending);
				size_t Count = It.Length > Core::BLOB_SIZE ? Core::BLOB_SIZE : It.Length;
				size_t Size = (size_t)fread(Data, sizeof(char), Count, AttachmentFile);
				if (Size != Count)
				{
					Report(Core::SystemException("smtp read attachment error: " + It.Path, std::make_error_condition(std::errc::io_error)));
					return false;
				}

				Core::String Content = Compute::Codec::Base64Encode(std::string_view(Data, Size));
				Content.append("\r\n");

				It.Length -= Size;
				if (!It.Length)
					Core::OS::File::Close(AttachmentFile);

				bool SendNext = (!It.Length);
				return Net.Stream->WriteQueued((uint8_t*)Content.c_str(), Content.size(), [this, SendNext](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
					{
						if (SendNext)
							SendAttachment();
						else
							ProcessAttachment();
					}
					else if (Packet::IsErrorOrSkip(Event))
						Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
				}) || true;
			}
			uint8_t* Client::Unicode(const std::string_view& Value)
			{
				size_t Length = Value.size();
				auto* Output = Core::Memory::Allocate<uint8_t>(sizeof(uint8_t) * (Length + 1));
				for (size_t i = 0; i < Length; i++)
					Output[i] = (uint8_t)Value[i];

				Output[Length] = '\0';
				return Output;
			}
			RequestFrame* Client::GetRequest()
			{
				return &Request;
			}
		}
	}
}