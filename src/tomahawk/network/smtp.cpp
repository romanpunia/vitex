#include "smtp.h"
#ifdef TH_MICROSOFT
#include <WS2tcpip.h>
#include <io.h>
#define ERRNO WSAGetLastError()
#define ERRWOULDBLOCK WSAEWOULDBLOCK
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
#define ERRNO errno
#define ERRWOULDBLOCK EWOULDBLOCK
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif
extern "C"
{
#ifdef TH_HAS_OPENSSL
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

namespace Tomahawk
{
	namespace Network
	{
		namespace SMTP
		{
			Client::Client(const std::string& Domain, int64_t ReadTimeout) : SocketClient(ReadTimeout), Hoster(Domain), AttachmentFile(nullptr), Pending(false), Staging(false), Authorized(false)
			{
				AutoCertify = false;
			}
			Client::~Client()
			{
			}
			bool Client::OnResolveHost(Host* Address)
			{
				TH_ASSERT(Address != nullptr, false, "address should be set");
				if (Address->Port == 0)
				{
					servent* Entry = getservbyname("mail", 0);
					if (Entry != nullptr)
						Address->Port = Entry->s_port;
					else
						Address->Port = htons(25);
				}

				return true;
			}
			bool Client::OnConnect()
			{
				Authorized = Staging = false;
				return ReadResponse(220, [this]()
				{
					SendRequest(250, Core::Form("EHLO %s\r\n", Hoster.empty() ? "domain" : Hoster.c_str()).R(), [this]()
					{
						if (this->Hostname.Secure)
						{
							if (!CanRequest("STARTTLS"))
								return (void)Error("tls is not supported by smtp server");

							SendRequest(220, "STARTTLS\r\n", [this]()
							{
								if (!Certify())
									return;

								SendRequest(250, Core::Form("EHLO %s\r\n", Hoster.empty() ? "domain" : Hoster.c_str()).R(), [this]()
								{
									Success(0);
								});
							});
						}
						else
							Success(0);
					});
				});
			}
			bool Client::OnClose()
			{
				return SendRequest(221, "QUIT\r\n", [this]()
				{
					Authorized = false;
					Stream.CloseAsync(true, [this](Socket*)
					{
						return Success(0);
					});
				}) || true;
			}
			Core::Async<int> Client::Send(RequestFrame&& Root)
			{
				if (!Stream.IsValid())
					return -1;

				Core::Async<int> Result;
				if (!Staging)
				{
					if (&Request != &Root)
						Request = std::move(Root);

					TH_TRACE("[smtp] TO %s", Root.Receiver.c_str());
					Done = [this, Result](SocketClient*, int Code) mutable
					{
						if (!Buffer.empty())
							TH_TRACE("[smtp] %i responded\n%.*s", (int)Stream.GetFd(), (int)Buffer.size(), Buffer.data());

						Buffer.clear();
						Result = Code;
					};
				}

				if (!Authorized && Request.Authenticate && CanRequest("AUTH"))
				{
					Staging = true;
					Authorize([this, Result]() mutable
					{
						Send(std::move(Request)).Await([Result](int Code) mutable
						{
							Result = Code;
						});
					});

					return Result;
				}

				Staging = false;
				Stage("request dispatching");
				if (Request.SenderAddress.empty())
				{
					Error("empty sender address");
					return Result;
				}

				if (Request.Recipients.empty())
				{
					Error("no recipients selected");
					return Result;
				}

				if (!Request.Attachments.empty())
					Boundary = Compute::Common::MD5Hash(Compute::Common::RandomBytes(64));

				Core::Parser Content;
				Content.fAppend("MAIL FROM: <%s>\r\n", Request.SenderAddress.c_str());

				for (auto& Item : Request.Recipients)
					Content.fAppend("RCPT TO: <%s>\r\n", Item.Address.c_str());

				for (auto& Item : Request.CCRecipients)
					Content.fAppend("RCPT TO: <%s>\r\n", Item.Address.c_str());

				for (auto& Item : Request.BCCRecipients)
					Content.fAppend("RCPT TO: <%s>\r\n", Item.Address.c_str());

				Stream.WriteAsync(Content.Get(), Content.Size(), [this](NetEvent Event, size_t Sent)
				{
					if (Packet::IsDone(Event))
					{
						Pending = 1 + Request.Recipients.size() + Request.CCRecipients.size() + Request.BCCRecipients.size();
						ReadResponses(250, [this]()
						{
							Pending = Request.Attachments.size();
							SendRequest(354, "DATA\r\n", [this]()
							{
								Core::Parser Content;
								Content.fAppend("Date: %s\r\nFrom: ", Core::DateTime::GetGMTBasedString(time(nullptr)).c_str());

								if (!Request.SenderName.empty())
									Content.Append(Request.SenderName.c_str());

								Content.fAppend(" <%s>\r\n", Request.SenderAddress.c_str());
								if (!Request.Mailer.empty())
									Content.fAppend("X-Mailer: %s\r\n", Request.Mailer.c_str());
								else
									Content.Append("X-Mailer: Lynx\r\n");

								if (!Request.Receiver.empty())
									Content.fAppend("Reply-To: %s\r\n", Request.Receiver.c_str());

								if (Request.NoNotification)
									Content.fAppend("Disposition-Notification-To: %s\r\n", !Request.Receiver.empty() ? Request.Receiver.c_str() : Request.SenderName.c_str());

								switch (Request.Prior)
								{
									case Priority::High:
										Content.Append("X-Priority: 2 (High)\r\n");
										break;
									case Priority::Normal:
										Content.Append("X-Priority: 3 (Normal)\r\n");
										break;
									case Priority::Low:
										Content.Append("X-Priority: 4 (Low)\r\n");
										break;
									default:
										Content.Append("X-Priority: 3 (Normal)\r\n");
										break;
								}

								std::string Recipients;
								for (uint64_t i = 0; i < Request.Recipients.size(); i++)
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

								Content.fAppend("To: %s\r\n", Recipients.c_str());
								if (!Request.CCRecipients.empty())
								{
									Recipients.clear();
									for (uint64_t i = 0; i < Request.CCRecipients.size(); i++)
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
									Content.fAppend("Cc: %s\r\n", Recipients.c_str());
								}

								if (!Request.BCCRecipients.empty())
								{
									Recipients.clear();
									for (uint64_t i = 0; i < Request.BCCRecipients.size(); i++)
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
									Content.fAppend("Bcc: %s\r\n", Recipients.c_str());
								}

								Content.fAppend("Subject: %s\r\nMIME-Version: 1.0\r\n", Request.Subject.c_str());
								if (!Request.Attachments.empty())
								{
									Content.fAppend("Content-Type: multipart/mixed; boundary=\"%s\"\r\n\r\n", Boundary.c_str());
									Content.fAppend("--%s\r\n", Boundary.c_str());
								}

								if (Request.AllowHTML)
									Content.fAppend("Content-Type: text/html; charset=\"%s\"\r\n", Request.Charset.c_str());
								else
									Content.fAppend("Content-type: text/plain; charset=\"%s\"\r\n", Request.Charset.c_str());

								Content.Append("Content-Transfer-Encoding: 7bit\r\n\r\n");
								Stream.WriteAsync(Content.Get(), Content.Size(), [this](NetEvent Event, size_t Sent)
								{
									if (Packet::IsDone(Event))
									{
										Core::Parser Content;
										for (auto& Item : Request.Messages)
											Content.fAppend("%s\r\n", Item.c_str());

										if (Request.Messages.empty())
											Content.Assign(" \r\n");

										Stream.WriteAsync(Content.Get(), Content.Size(), [this](NetEvent Event, size_t Sent)
										{
											if (Packet::IsDone(Event))
											{
												Stage("smtp attachment delivery");
												SendAttachment();
											}
											else if (Packet::IsErrorOrSkip(Event))
												Error("smtp socket write %s", (Event == NetEvent::Timeout ? "timeout" : "error"));
										});
									}
									else if (Packet::IsErrorOrSkip(Event))
										Error("smtp socket write %s", (Event == NetEvent::Timeout ? "timeout" : "error"));
								});
							});
						});
					}
					else if (Packet::IsErrorOrSkip(Event))
						Error("smtp socket write %s", (Event == NetEvent::Timeout ? "timeout" : "error"));
				});

				return Result;
			}
			bool Client::ReadResponses(int Code, const ReplyCallback& Callback)
			{
				return ReadResponse(Code, [this, Callback, Code]()
				{
					Pending--;
					if (Pending <= 0)
					{
						Pending = 0;
						if (Callback)
							Callback();
					}
					else
						ReadResponses(Code, Callback);
				});
			}
			bool Client::ReadResponse(int Code, const ReplyCallback& Callback)
			{
				Command.clear();
				return Stream.ReadUntilAsync("\r\n", [this, Callback, Code](NetEvent Event, const char* Data, size_t Recv)
				{
					if (Packet::IsData(Event))
					{
						Buffer.append(Data, Recv);
						Command.append(Data, Recv);
						return true;
					}
					else if (Packet::IsDone(Event))
					{
						if (!isdigit(Command[0]) || !isdigit(Command[1]) || !isdigit(Command[2]))
							return Error("cannot parse smtp response");

						if (Command[3] != ' ')
							return ReadResponse(Code, Callback);

						int ReplyCode = (Command[0] - '0') * 100 + (Command[1] - '0') * 10 + Command[2] - '0';
						if (ReplyCode != Code)
							return Error("smtp status is %i but wanted %i", ReplyCode, Code);

						if (Callback)
							Callback();
					}
					else if (Packet::IsErrorOrSkip(Event))
						return Error("connection error");

					return true;
				}) || true;
			}
			bool Client::SendRequest(int Code, const std::string& Content, const ReplyCallback& Callback)
			{
				return Stream.WriteAsync(Content.c_str(), Content.size(), [this, Callback, Code](NetEvent Event, size_t Sent)
				{
					if (Packet::IsDone(Event))
						ReadResponse(Code, Callback);
					else if (Packet::IsErrorOrSkip(Event))
						Error("smtp socket write %s", (Event == NetEvent::Timeout ? "timeout" : "error"));
				}) || true;
			}
			bool Client::CanRequest(const char* Keyword)
			{
				TH_ASSERT(Keyword != nullptr, false, "keyword should be set");

				uint64_t L1 = Buffer.size(), L2 = strlen(Keyword);
				if (L1 < L2)
					return false;

				for (uint64_t i = 0; i < L1 - L2 + 1; i++)
				{
#ifdef TH_MICROSOFT
					if (_strnicmp(Keyword, Buffer.c_str() + i, (size_t)L2) != 0 || !i)
						continue;
#else
					if (strncmp(Keyword, Buffer.c_str() + i, (size_t)L2) != 0 || !i)
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
			bool Client::Authorize(const ReplyCallback& Callback)
			{
				if (!Callback || Authorized)
				{
					if (Callback)
						Callback();

					return true;
				}

				if (Request.Login.empty())
					return Error("smtp login cannot be used");

				if (Request.Password.empty())
					return Error("smtp password cannot be used");

				if (CanRequest("LOGIN"))
				{
					return SendRequest(334, "AUTH LOGIN\r\n", [this, Callback]()
					{
						std::string Hash = Compute::Common::Base64Encode(Request.Login);
						SendRequest(334, Hash.append("\r\n"), [this, Callback]()
						{
							std::string Hash = Compute::Common::Base64Encode(Request.Password);
							SendRequest(235, Hash.append("\r\n"), [this, Callback]()
							{
								Authorized = true;
								Callback();
							});
						});
					});
				}
				else if (CanRequest("PLAIN"))
				{
					std::string Hash = Core::Form("%s^%s^%s", Request.Login.c_str(), Request.Login.c_str(), Request.Password.c_str()).R();
					char* Escape = (char*)Hash.c_str();

					for (uint64_t i = 0; i < Hash.size(); i++)
					{
						if (Escape[i] == 94)
							Escape[i] = 0;
					}

					return SendRequest(235, Core::Form("AUTH PLAIN %s\r\n", Compute::Common::Base64Encode(Hash).c_str()).R(), [this, Callback]()
					{
						Authorized = true;
						Callback();
					});
				}
				else if (CanRequest("CRAM-MD5"))
				{
					return SendRequest(334, "AUTH CRAM-MD5\r\n", [this, Callback]()
					{
						std::string EncodedChallenge = Command.c_str() + 4;
						std::string DecodedChallenge = Compute::Common::Base64Decode(EncodedChallenge);
						unsigned char* UserChallenge = Unicode(DecodedChallenge.c_str());
						unsigned char* UserPassword = Unicode(Request.Password.c_str());

						if (!UserChallenge || !UserPassword)
						{
							TH_FREE(UserChallenge);
							TH_FREE(UserPassword);
							return (void)Error("smtp password cannot be used");
						}

						uint64_t PasswordLength = Request.Password.size();
						if (PasswordLength > 64)
						{
							Compute::MD5Hasher PasswordMD5;
							PasswordMD5.Update(UserPassword, (unsigned int)PasswordLength);
							PasswordMD5.Finalize();

							TH_FREE(UserPassword);
							UserPassword = PasswordMD5.RawDigest();
							PasswordLength = 16;
						}

						unsigned char IPad[65] = { 0 };
						unsigned char OPad[65] = { 0 };
						memcpy(IPad, UserPassword, (size_t)PasswordLength);
						memcpy(OPad, UserPassword, (size_t)PasswordLength);

						for (uint64_t i = 0; i < 64; i++)
						{
							IPad[i] ^= 0x36;
							OPad[i] ^= 0x5c;
						}

						Compute::MD5Hasher MD5Pass1;
						MD5Pass1.Update(IPad, 64);
						MD5Pass1.Update(UserChallenge, (unsigned int)DecodedChallenge.size());
						MD5Pass1.Finalize();
						unsigned char* UserResult = MD5Pass1.RawDigest();

						Compute::MD5Hasher MD5Pass2;
						MD5Pass2.Update(OPad, 64);
						MD5Pass2.Update(UserResult, 16);
						MD5Pass2.Finalize();
						const char* UserBase = MD5Pass2.HexDigest();

						TH_FREE(UserChallenge);
						TH_FREE(UserPassword);
						TH_FREE(UserResult);

						DecodedChallenge = Request.Login + ' ' + UserBase;
						EncodedChallenge = Compute::Common::Base64Encode(reinterpret_cast<const unsigned char*>(DecodedChallenge.c_str()), DecodedChallenge.size());

						TH_FREE((void*)UserBase);
						SendRequest(235, Core::Form("%s\r\n", EncodedChallenge.c_str()).R(), [this, Callback]()
						{
							Authorized = true;
							Callback();
						});
					});
				}
				else if (CanRequest("DIGEST-MD5"))
				{
					return SendRequest(334, "AUTH DIGEST-MD5\r\n", [this, Callback]()
					{
						std::string EncodedChallenge = Command.c_str() + 4;
						Core::Parser DecodedChallenge = Compute::Common::Base64Decode(EncodedChallenge);

						Core::Parser::Settle Result1 = DecodedChallenge.Find("nonce");
						if (!Result1.Found)
							return (void)Error("smtp has delivered bad digest");

						Core::Parser::Settle Result2 = DecodedChallenge.Find("\"", Result1.Start + 7);
						if (!Result2.Found)
							return (void)Error("smtp has delivered bad digest");

						std::string Nonce = DecodedChallenge.Splice(Result1.Start + 7, Result2.Start).R();
						std::string Realm;

						Result1 = DecodedChallenge.Find("realm");
						if (Result1.Found)
						{
							Result2 = DecodedChallenge.Find("\"", Result1.Start + 7);
							if (!Result2.Found)
								return (void)Error("smtp has delivered bad digest");

							Realm = DecodedChallenge.Splice(Result1.Start + 7, Result2.Start).R();
						}

						char CNonce[17], NC[9];
						snprintf(CNonce, 17, "%x", (unsigned int)time(nullptr));
						snprintf(NC, 9, "%08d", 1);

						struct sockaddr_storage Storage;
						socket_size_t Length = sizeof(Storage);

						if (!getpeername(Stream.GetFd(), (struct sockaddr*)&Storage, &Length))
							return (void)Error("cannot detect peer name");

						char InetAddress[INET_ADDRSTRLEN];
						struct sockaddr_in* Address = (struct sockaddr_in*)&Storage;
						std::string URI = "smtp/";

						if (inet_ntop(AF_INET, &(Address->sin_addr), InetAddress, INET_ADDRSTRLEN) != nullptr)
							URI += InetAddress;
						else
							URI += "127.0.0.1";

						unsigned char* UserRealm = Unicode(Realm.c_str());
						unsigned char* UserUsername = Unicode(Request.Login.c_str());
						unsigned char* UserPassword = Unicode(Request.Password.c_str());
						unsigned char* UserNonce = Unicode(Nonce.c_str());
						unsigned char* UserCNonce = Unicode(CNonce);
						unsigned char* UserUri = Unicode(URI.c_str());
						unsigned char* UserNc = Unicode(NC);
						unsigned char* UserQop = Unicode("auth");

						if (!UserRealm || !UserUsername || !UserPassword || !UserNonce || !UserCNonce || !UserUri || !UserNc || !UserQop)
							return (void)Error("smtp auth failed");

						Compute::MD5Hasher MD5A1A;
						MD5A1A.Update(UserUsername, (unsigned int)Request.Login.size());
						MD5A1A.Update((unsigned char*)":", 1);
						MD5A1A.Update(UserRealm, (unsigned int)Realm.size());
						MD5A1A.Update((unsigned char*)":", 1);
						MD5A1A.Update(UserPassword, (unsigned int)Request.Password.size());
						MD5A1A.Finalize();
						unsigned char* UserA1A = MD5A1A.RawDigest();

						Compute::MD5Hasher MD5A1B;
						MD5A1B.Update(UserA1A, 16);
						MD5A1B.Update((unsigned char*)":", 1);
						MD5A1B.Update(UserNonce, (unsigned int)Nonce.size());
						MD5A1B.Update((unsigned char*)":", 1);
						MD5A1B.Update(UserCNonce, (unsigned int)strlen(CNonce));
						MD5A1B.Finalize();
						char* UserA1B = MD5A1B.HexDigest();

						Compute::MD5Hasher MD5A2;
						MD5A2.Update((unsigned char*)"AUTHENTICATE:", 13);
						MD5A2.Update(UserUri, (unsigned int)URI.size());
						MD5A2.Finalize();
						char* UserA2A = MD5A2.HexDigest();

						TH_FREE(UserA1A);
						UserA1A = Unicode(UserA1B);
						unsigned char* UserA2B = Unicode(UserA2A);

						Compute::MD5Hasher MD5Pass;
						MD5Pass.Update(UserA1A, 32);
						MD5Pass.Update((unsigned char*)":", 1);
						MD5Pass.Update(UserNonce, (unsigned int)Nonce.size());
						MD5Pass.Update((unsigned char*)":", 1);
						MD5Pass.Update(UserNc, (unsigned int)strlen(NC));
						MD5Pass.Update((unsigned char*)":", 1);
						MD5Pass.Update(UserCNonce, (unsigned int)strlen(CNonce));
						MD5Pass.Update((unsigned char*)":", 1);
						MD5Pass.Update(UserQop, 4);
						MD5Pass.Update((unsigned char*)":", 1);
						MD5Pass.Update(UserA2B, 32);
						MD5Pass.Finalize();
						DecodedChallenge = MD5Pass.HexDigest();

						TH_FREE(UserRealm);
						TH_FREE(UserUsername);
						TH_FREE(UserPassword);
						TH_FREE(UserNonce);
						TH_FREE(UserCNonce);
						TH_FREE(UserUri);
						TH_FREE(UserNc);
						TH_FREE(UserQop);
						TH_FREE(UserA1A);
						TH_FREE(UserA1B);
						TH_FREE(UserA2A);
						TH_FREE(UserA2B);

						Core::Parser Content;
						if (strstr(Command.c_str(), "charset") != nullptr)
							Content.fAppend("charset=utf-8,username=\"%s\"", Request.Login.c_str());
						else
							Content.fAppend("username=\"%s\"", Request.Login.c_str());

						if (!Realm.empty())
							Content.fAppend(",realm=\"%s\"", Realm.c_str());

						Content.fAppend(",nonce=\"%s\""
							",nc=%s"
							",cnonce=\"%s\""
							",digest-uri=\"%s\""
							",Response=%s"
							",qop=auth", Nonce.c_str(), NC, CNonce, URI.c_str(), DecodedChallenge.Get());

						EncodedChallenge = Compute::Common::Base64Encode(Content.R());
						SendRequest(334, Core::Form("%s\r\n", EncodedChallenge.c_str()).R(), [this, Callback]()
						{
							SendRequest(235, "\r\n", [this, Callback]()
							{
								Authorized = true;
								Callback();
							});
						});
					});
				}

				return Error("smtp server does not support any of active auth types");
			}
			bool Client::SendAttachment()
			{
				if (Pending <= 0)
				{
					Stage("smtp request delivery");

					Core::Parser Content;
					if (!Request.Attachments.empty())
						Content.fAppend("\r\n--%s--\r\n", Boundary.c_str());

					Content.Append("\r\n.\r\n");
					return !Stream.WriteAsync(Content.Get(), Content.Size(), [this](NetEvent Event, size_t Sent)
					{
						if (Packet::IsDone(Event))
						{
							ReadResponses(250, [this]()
							{
								Success(0);
							});
						}
						else if (Packet::IsErrorOrSkip(Event))
							Error("smtp socket write %s", (Event == NetEvent::Timeout ? "timeout" : "error"));
					});
				}

				Attachment& It = Request.Attachments.at(Request.Attachments.size() - Pending);
				const char* Name = It.Path.c_str();
				uint64_t Id = strlen(Name) - 1;

				if (Id > 0 && (Name[Id] == '\\' || Name[Id] == '/'))
					Name = Name - 1;

				std::string Hash = Core::Form("=?UTF-8?B?%s?=", Compute::Common::Base64Encode((unsigned char*)Name, Id + 1).c_str()).R();
				Core::Parser Content;
				Content.fAppend("--%s\r\n", Boundary.c_str());
				Content.fAppend("Content-Type: application/x-msdownload; name=\"%s\"\r\n", Hash.c_str());
				Content.fAppend("Content-Transfer-Encoding: base64\r\n");
				Content.fAppend("Content-Disposition: attachment; filename=\"%s\"\r\n\r\n", Hash.c_str());

				return Stream.WriteAsync(Content.Get(), Content.Size(), [this, Name](NetEvent Event, size_t Sent)
				{
					if (Packet::IsDone(Event))
					{
						AttachmentFile = (FILE*)Core::OS::File::Open(Name, "rb");
						if (AttachmentFile != nullptr)
							Error("cannot open attachment resource");
						else
							ProcessAttachment();
					}
					else if (Packet::IsErrorOrSkip(Event))
						Error("smtp socket write %s", (Event == NetEvent::Timeout ? "timeout" : "error"));
				}) || true;
			}
			bool Client::ProcessAttachment()
			{
				char Data[8192];
				Attachment& It = Request.Attachments.at(Request.Attachments.size() - Pending);
				size_t Count = It.Length > 8192 ? 8192 : It.Length;
				int64_t Size = fread(Data, sizeof(char), Count, AttachmentFile);
				if (Size != Count)
					return Error("cannot read attachment block from %s", It.Path.c_str());

				std::string Content = Compute::Common::Base64Encode((const unsigned char*)Data, Size);
				Content.append("\r\n");

				It.Length -= Size;
				if (!It.Length)
					TH_CLOSE(AttachmentFile);

				bool SendNext = (!It.Length);
				return Stream.WriteAsync(Content.c_str(), Content.size(), [this, SendNext](NetEvent Event, size_t Sent)
				{
					if (Packet::IsDone(Event))
					{
						if (SendNext)
							SendAttachment();
						else
							ProcessAttachment();
					}
					else if (Packet::IsErrorOrSkip(Event))
						Error("smtp socket write %s", (Event == NetEvent::Timeout ? "timeout" : "error"));
				}) || true;
			}
			unsigned char* Client::Unicode(const char* String)
			{
				uint64_t Length = strlen(String);
				auto Output = (unsigned char*)TH_MALLOC(sizeof(unsigned char) * (size_t)(Length + 1));

				for (uint64_t i = 0; i < Length; i++)
					Output[i] = (unsigned char)String[i];

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
