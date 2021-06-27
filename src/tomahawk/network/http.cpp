#include "http.h"
#include "../script/std-lib.h"
#ifdef TH_MICROSOFT
#include <WS2tcpip.h>
#include <io.h>
#include <wepoll.h>
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
#include <future>
#include <random>
#include <string>
extern "C"
{
#ifdef TH_HAS_ZLIB
#include <zlib.h>
#endif
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
		namespace HTTP
		{
			static void TextAppend(std::vector<char>& Array, const std::string& Src)
			{
				Array.insert(Array.end(), Src.begin(), Src.end());
			}
			static void TextAppend(std::vector<char>& Array, const char* Buffer, size_t Size)
			{
				Array.insert(Array.end(), Buffer, Buffer + Size);
			}
			static void TextAssign(std::vector<char>& Array, const std::string& Src)
			{
				Array.assign(Src.begin(), Src.end());
			}
			static void TextAssign(std::vector<char>& Array, const char* Buffer, size_t Size)
			{
				Array.assign(Buffer, Buffer + Size);
			}
			static std::string TextSubstring(std::vector<char>& Array, size_t Offset, size_t Size)
			{
				return std::string(Array.data() + Offset, Size);
			}

			MimeStatic::MimeStatic(const char* Ext, const char* T) : Extension(Ext), Type(T)
			{
			}

			void WebSocketFrame::Write(const char* Data, int64_t Length, WebSocketOp OpCode, const SuccessCallback& Callback)
			{
				Util::WebSocketWriteMask(Base, Data, Length, OpCode, 0, Callback);
			}
			void WebSocketFrame::Finish()
			{
				if (Error || State == (uint32_t)WebSocketState::Close)
				{
					State = (uint32_t)WebSocketState::Free;
					return Next();
				}

				if (State != (uint32_t)WebSocketState::Free)
					State = (uint32_t)WebSocketState::Close;

				Util::WebSocketWriteMask(Base, Base->Info.Message.c_str(), Base->Info.Message.size(), WebSocketOp::Close, 0, [this](HTTP::Connection*)
				{
					Next();
					return true;
				});
			}
			void WebSocketFrame::Next()
			{
				if (State & (uint32_t)WebSocketState::Free)
				{
					if (Disconnect)
					{
						WebSocketCallback Callback = Disconnect;
						Disconnect = nullptr;
						return Callback(this);
					}

					if (Base->Gateway && !Base->Gateway->IsDone())
						return (void)Base->Gateway->Done(true);

					Base->Stream->CloseAsync(true, [this](Socket*)
					{
						if (Base->Response.StatusCode <= 0)
							Base->Response.StatusCode = 101;

						Save = true;
						Base->Info.KeepAlive = 0;
						Base->Unlock();

						auto* Store = Base;
						return Core::Schedule::Get()->SetTask([Store]()
						{
							Store->Finish();
						});
					});
				}
				else if (State & (uint32_t)WebSocketState::Handshake)
				{
					if (Connect || Receive)
					{
						State = (uint32_t)WebSocketState::Active;
						if (Connect)
							Connect(this);
						else
							Next();
					}
					else
						Finish();
				}
				else if (State & (uint32_t)WebSocketState::Reset)
				{
					State = (uint32_t)WebSocketState::Free;
					Finish();
				}
				else
					Util::ProcessWebSocketPass(Base);
			}
			void WebSocketFrame::Notify()
			{
				Notified = true;
				Base->Stream->Skip((uint32_t)SocketEvent::Read, 0);
			}
			bool WebSocketFrame::IsFinished()
			{
				return Save;
			}

			GatewayFrame::GatewayFrame(char* Data, int64_t DataSize) : Size(DataSize), Compiler(nullptr), Base(nullptr), Buffer(Data), Save(false)
			{
			}
			void GatewayFrame::Execute(Script::VMResume State)
			{
				if (State == Script::VMResume::Finish_With_Error)
				{
					if (Base->Response.StatusCode <= 0)
						Base->Response.StatusCode = 500;

					if (Base->Route->Gateway.ReportErrors)
					{
						Script::VMContext* Context = Compiler->GetContext();
						const char* Exception = Context->GetExceptionString();
						const char* Function = Context->GetExceptionFunction().GetName();
						int Column = 0; int Line = Context->GetExceptionLineNumber(&Column, nullptr);
						Base->Info.Message = Core::Form("exception at line %i (col. %i) from %s: %s", Line, Column, Function ? Function : "anonymous", Exception ? Exception : "uncaught empty exception").R();
					}
					else
						Base->Info.Message.assign("Internal processing error occurred.");
				}

				if (Base->WebSocket != nullptr)
				{
					if (State == Script::VMResume::Continue || IsScheduled())
						Base->WebSocket->Next();
					else if (Base->WebSocket->State == (uint32_t)WebSocketState::Active || Base->WebSocket->State == (uint32_t)WebSocketState::Handshake)
						Base->WebSocket->Finish();
					else
						Finish();
				}
				else if (State != Script::VMResume::Continue)
					Finish();
			}
			bool GatewayFrame::Done(bool Normal)
			{
				if (!Save)
				{
					if (Compiler != nullptr)
					{
						if (!Compiler->Clear())
							return false;

						TH_CLEAR(Compiler);
						Save = true;
					}
					else
						Save = true;
				}

				if (Base->WebSocket != nullptr)
				{
					Base->Response.Buffer.clear();
					if (!Normal)
						Base->WebSocket->Finish();
					else
						Base->WebSocket->Next();

					return false;
				}

				auto* Store = Base;
				return Core::Schedule::Get()->SetTask([Store]()
				{
					Store->Finish();
				}) && false;
			}
			bool GatewayFrame::Finish()
			{
				Base->Unlock();
				if (Buffer != nullptr)
				{
					TH_FREE(Buffer);
					Buffer = nullptr;
					Size = 0;
				}

				if (!Base->WebSocket)
					return Done(true);

				Base->WebSocket->Next();
				return true;
			}
			bool GatewayFrame::Error(int StatusCode, const char* Text)
			{
				Base->Response.StatusCode = StatusCode;
				Base->Info.Message.assign(Text);
				return Error();
			}
			bool GatewayFrame::Error()
			{
				Base->Unlock();
				if (Buffer != nullptr)
				{
					TH_FREE(Buffer);
					Buffer = nullptr;
					Size = 0;
				}

				if (!Base->WebSocket)
					return Done(false);

				Base->WebSocket->Finish();
				return true;
			}
			bool GatewayFrame::Start()
			{
				if (!Base || (Size != -1 && (!Buffer || Size == 0)))
				{
					if (!Compiler && Base != nullptr && Base->Response.StatusCode <= 0)
						Base->Response.StatusCode = 200;

					return Finish();
				}

				Base->Lock();
				return Core::Schedule::Get()->SetTask([this]()
				{
					int Result = Compiler->LoadCode(Base->Request.Path, Buffer, Size);
					if (Result < 0)
						return Error(500, "Module cannot be loaded.");

					Result = Compiler->Compile(true);
					if (Result < 0)
						return Error(500, "Module cannot be compiled.");

					Script::VMFunction Main = GetMain(Compiler->GetModule());
					if (!Main.IsValid())
						return Error(400, "Requested method is not allowed.");

					Script::VMContext* Context = Compiler->GetContext();
					Context->SetResumeCallback(std::bind(&GatewayFrame::Execute, this, std::placeholders::_1));

					Result = Context->Prepare(Main);
					if (Result < 0)
						return Error(500, "Module execution cannot be prepared.");

					Result = Context->Execute();
					if (Result == (int)Script::VMExecState::FINISHED || Result == (int)Script::VMExecState::ABORTED)
					{
						Execute(Script::VMResume::Finish);
						return true;
					}
					else if (Result == (int)Script::VMExecState::ERR)
					{
						Execute(Script::VMResume::Finish_With_Error);
						return true;
					}
					else if (Result == (int)Script::VMExecState::EXCEPTION)
					{
						Execute(Context->IsThrown() ? Script::VMResume::Finish_With_Error : Script::VMResume::Finish);
						return true;
					}

					return true;
				});
			}
			bool GatewayFrame::IsDone()
			{
				return Save;
			}
			bool GatewayFrame::IsScheduled()
			{
				if (!Base->WebSocket)
					return false;

				if (Base->WebSocket->State != (uint32_t)WebSocketState::Active && Base->WebSocket->State != (uint32_t)WebSocketState::Handshake)
					return false;

				return
					Base->WebSocket->Connect ||
					Base->WebSocket->Disconnect ||
					Base->WebSocket->Notification ||
					Base->WebSocket->Receive;
			}
			Script::VMFunction GatewayFrame::GetMain(const Script::VMModule& Mod)
			{
				Script::VMFunction Result = Mod.GetFunctionByName("Main");
				if (Result.IsValid())
					return Result;

				return Mod.GetFunctionByName(Base->Request.Method);
			}
			Script::VMContext* GatewayFrame::GetContext()
			{
				return (Compiler ? Compiler->GetContext() : nullptr);
			}

			SiteEntry::SiteEntry() : Base(TH_NEW(RouteEntry))
			{
				Base->URI = Compute::RegexSource("/");
				Base->Site = this;
			}
			SiteEntry::~SiteEntry()
			{
				for (auto* Entry : Routes)
					TH_DELETE(RouteEntry, Entry);
				Routes.clear();

				TH_DELETE(RouteEntry, Base);
				Base = nullptr;
			}
			RouteEntry* SiteEntry::Route(const std::string& Pattern)
			{
				if (Pattern.empty() || Pattern == "/")
					return Base;

				for (auto* Entry : Routes)
				{
					if (Entry->URI.GetRegex() == Pattern)
						return Entry;
				}

				HTTP::RouteEntry* From = Base;
				Compute::RegexResult Result;
				Core::Parser Src(Pattern);
				Src.ToLower();

				for (auto* Entry : Routes)
				{
					Core::Parser Dest(Entry->URI.GetRegex());
					Dest.ToLower();

					if (Dest.StartsWith("...") && Dest.EndsWith("..."))
						continue;

					if (Src.Find(Dest.R()).Found || Compute::Regex::Match(&Entry->URI, Result, Src.R()))
					{
						From = Entry;
						break;
					}
				}

				return Route(Pattern, From);
			}
			RouteEntry* SiteEntry::Route(const std::string& Pattern, RouteEntry* From)
			{
				HTTP::RouteEntry* Result = TH_NEW(HTTP::RouteEntry, *From);
				Result->URI = Compute::RegexSource(Pattern);
				Routes.push_back(Result);

				return Result;
			}
			bool SiteEntry::Remove(RouteEntry* Source)
			{
				if (!Source)
					return false;

				auto It = std::find(Routes.begin(), Routes.end(), Source);
				if (It == Routes.end())
					return false;

				TH_DELETE(RouteEntry, *It);
				Routes.erase(It);
				return true;
			}
			bool SiteEntry::Get(const char* Pattern, SuccessCallback Callback)
			{
				HTTP::RouteEntry* Value = Route(Pattern);
				if (!Value)
					return false;

				Value->Callbacks.Get = Callback;
				return true;
			}
			bool SiteEntry::Post(const char* Pattern, SuccessCallback Callback)
			{
				HTTP::RouteEntry* Value = Route(Pattern);
				if (!Value)
					return false;

				Value->Callbacks.Post = Callback;
				return true;
			}
			bool SiteEntry::Put(const char* Pattern, SuccessCallback Callback)
			{
				HTTP::RouteEntry* Value = Route(Pattern);
				if (!Value)
					return false;

				Value->Callbacks.Put = Callback;
				return true;
			}
			bool SiteEntry::Patch(const char* Pattern, SuccessCallback Callback)
			{
				HTTP::RouteEntry* Value = Route(Pattern);
				if (!Value)
					return false;

				Value->Callbacks.Patch = Callback;
				return true;
			}
			bool SiteEntry::Delete(const char* Pattern, SuccessCallback Callback)
			{
				HTTP::RouteEntry* Value = Route(Pattern);
				if (!Value)
					return false;

				Value->Callbacks.Delete = Callback;
				return true;
			}
			bool SiteEntry::Options(const char* Pattern, SuccessCallback Callback)
			{
				HTTP::RouteEntry* Value = Route(Pattern);
				if (!Value)
					return false;

				Value->Callbacks.Options = Callback;
				return true;
			}
			bool SiteEntry::Access(const char* Pattern, SuccessCallback Callback)
			{
				HTTP::RouteEntry* Value = Route(Pattern);
				if (!Value)
					return false;

				Value->Callbacks.Access = Callback;
				return true;
			}
			bool SiteEntry::WebSocketConnect(const char* Pattern, WebSocketCallback Callback)
			{
				HTTP::RouteEntry* Value = Route(Pattern);
				if (!Value)
					return false;

				Value->Callbacks.WebSocket.Connect = Callback;
				return true;
			}
			bool SiteEntry::WebSocketDisconnect(const char* Pattern, WebSocketCallback Callback)
			{
				HTTP::RouteEntry* Value = Route(Pattern);
				if (!Value)
					return false;

				Value->Callbacks.WebSocket.Disconnect = Callback;
				return true;
			}
			bool SiteEntry::WebSocketReceive(const char* Pattern, WebSocketReadCallback Callback)
			{
				HTTP::RouteEntry* Value = Route(Pattern);
				if (!Value)
					return false;

				Value->Callbacks.WebSocket.Receive = Callback;
				return true;
			}

			MapRouter::MapRouter() : VM(nullptr)
			{
			}
			MapRouter::~MapRouter()
			{
				for (auto* Entry : Sites)
					TH_DELETE(SiteEntry, Entry);
			}
			SiteEntry* MapRouter::Site(const char* Pattern)
			{
				if (!Pattern)
					return nullptr;

				for (auto* Entry : Sites)
				{
					if (Entry->SiteName.find(Pattern) != std::string::npos)
						return Entry;
				}

				HTTP::SiteEntry* Result = TH_NEW(HTTP::SiteEntry);
				Result->SiteName = Core::Parser(Pattern).ToLower().R();
				Sites.push_back(Result);

				return Result;
			}

			void Resource::SetHeader(const char* fKey, const char* Value)
			{
				if (!fKey)
					return;

				for (auto It = Headers.begin(); It != Headers.end(); ++It)
				{
					if (strcmp(It->Key.c_str(), fKey) != 0)
						continue;

					if (Value != nullptr)
						It->Value = Value;
					else
						Headers.erase(It);

					return;
				}

				if (!Value)
					return;

				Header Header;
				Header.Key = fKey;
				Header.Value = Value;
				Headers.push_back(Header);
			}
			void Resource::SetHeader(const char* fKey, const std::string& Value)
			{
				if (!fKey)
					return;

				for (auto& Head : Headers)
				{
					if (Head.Key == fKey)
					{
						Head.Value = Value;
						return;
					}
				}

				Header Header;
				Header.Key = fKey;
				Header.Value = Value;
				Headers.push_back(Header);
			}
			const char* Resource::GetHeader(const char* fKey)
			{
				if (!fKey)
					return nullptr;

				for (auto& Head : Headers)
				{
					if (!Core::Parser::CaseCompare(Head.Key.c_str(), fKey))
						return Head.Value.c_str();
				}

				return nullptr;
			}

			void RequestFrame::SetHeader(const char* fKey, const char* Value)
			{
				if (!fKey)
					return;

				for (auto It = Headers.begin(); It != Headers.end(); ++It)
				{
					if (strcmp(It->Key.c_str(), fKey) != 0)
						continue;

					if (Value != nullptr)
						It->Value = Value;
					else
						Headers.erase(It);

					return;
				}

				if (!Value)
					return;

				Header Header;
				Header.Key = fKey;
				Header.Value = Value;
				Headers.push_back(Header);
			}
			void RequestFrame::SetHeader(const char* Key, const std::string& Value)
			{
				if (!Key)
					return;

				for (auto& Head : Headers)
				{
					if (Head.Key == Key)
					{
						Head.Value = Value;
						return;
					}
				}

				Header Header;
				Header.Key = Key;
				Header.Value = Value;
				Headers.push_back(Header);
			}
			const char* RequestFrame::GetCookie(const char* Key)
			{
				if (!Key)
					return nullptr;

				if (!Cookies.empty())
				{
					for (auto& Cookie : Cookies)
					{
						if (Cookie.Key == Key)
							return Cookie.Value.c_str();
					}

					return nullptr;
				}

				const char* Cookie = GetHeader("Cookie");
				if (!Cookie)
					return nullptr;

				uint64_t Length = strlen(Cookie);
				const char* Offset = Cookie;
				Cookies.reserve(2);

				for (uint64_t i = 0; i < Length; i++)
				{
					if (Cookie[i] != '=')
						continue;

					uint64_t NameLength = (uint64_t)((Cookie + i) - Offset), Set = i;
					std::string Name(Offset, NameLength);

					while (i + 1 < Length && Cookie[i] != ';')
						i++;

					if (Cookie[i] == ';')
						i--;

					Cookies.push_back({ Name, std::string(Cookie + Set + 1, i - Set) });
					Offset = Cookie + (i + 3);
				}

				if (Cookies.empty())
					return nullptr;

				for (auto& Cookie : Cookies)
				{
					if (Cookie.Key == Key)
						return Cookie.Value.c_str();
				}

				return nullptr;
			}
			const char* RequestFrame::GetHeader(const char* Key)
			{
				if (!Key)
					return nullptr;

				for (auto& Head : Headers)
				{
					if (!Core::Parser::CaseCompare(Head.Key.c_str(), Key))
						return Head.Value.c_str();
				}

				return nullptr;
			}
			std::vector<std::pair<int64_t, int64_t>> RequestFrame::GetRanges()
			{
				const char* Range = GetHeader("Range");
				if (Range == nullptr)
					return std::vector<std::pair<int64_t, int64_t>>();

				std::vector<std::string> Bases = Core::Parser(Range).Split(',');
				std::vector<std::pair<int64_t, int64_t>> Ranges;

				for (auto& Item : Bases)
				{
					Core::Parser::Settle Result = Core::Parser(&Item).Find('-');
					if (!Result.Found)
						continue;

					const char* Start = Item.c_str() + Result.Start;
					uint64_t StartLength = 0;

					while (Result.Start > 0 && *Start-- != '\0' && isdigit(*Start))
						StartLength++;

					const char* End = Item.c_str() + Result.Start;
					uint64_t EndLength = 0;

					while (*End++ != '\0' && isdigit(*End))
						EndLength++;

					int64_t From = std::stoll(std::string(Start, StartLength));
					if (From == -1)
						break;

					int64_t To = std::stoll(std::string(End, EndLength));
					if (To == -1 || To < From)
						break;

					Ranges.emplace_back(std::make_pair((uint64_t)From, (uint64_t)To));
				}

				return Ranges;
			}
			std::pair<uint64_t, uint64_t> RequestFrame::GetRange(std::vector<std::pair<int64_t, int64_t>>::iterator Range, uint64_t ContenLength)
			{
				if (Range->first == -1 && Range->second == -1)
					return std::make_pair(0, ContenLength);

				if (Range->first == -1)
				{
					Range->first = ContenLength - Range->second;
					Range->second = ContenLength - 1;
				}

				if (Range->second == -1)
					Range->second = ContenLength - 1;

				return std::make_pair(Range->first, Range->second - Range->first + 1);
			}

			void ResponseFrame::PutBuffer(const std::string& Data)
			{
				TextAppend(Buffer, Data);
			}
			void ResponseFrame::SetBuffer(const std::string& Data)
			{
				TextAssign(Buffer, Data);
			}
			void ResponseFrame::SetHeader(const char* Key, const char* Value)
			{
				if (!Key)
					return;

				for (auto It = Headers.begin(); It != Headers.end(); ++It)
				{
					if (strcmp(It->Key.c_str(), Key) != 0)
						continue;

					if (Value != nullptr)
						It->Value = Value;
					else
						Headers.erase(It);

					return;
				}

				if (!Value)
					return;

				Header Header;
				Header.Key = Key;
				Header.Value = Value;
				Headers.push_back(Header);
			}
			void ResponseFrame::SetHeader(const char* Key, const std::string& Value)
			{
				if (!Key)
					return;

				for (auto& Head : Headers)
				{
					if (strcmp(Head.Key.c_str(), Key) == 0)
					{
						Head.Value = Value;
						return;
					}
				}

				Header Header;
				Header.Key = Key;
				Header.Value = Value;
				Headers.push_back(Header);
			}
			void ResponseFrame::SetCookie(const char* Key, const char* Value, uint64_t Expires, const char* Domain, const char* Path, bool Secure, bool HTTPOnly)
			{
				if (!Key || !Value)
					return;

				for (auto& Cookie : Cookies)
				{
					if (strcmp(Cookie.Name.c_str(), Key) == 0)
					{
						if (Domain != nullptr)
							Cookie.Domain = Domain;

						if (Path != nullptr)
							Cookie.Path = Path;

						Cookie.Value = Value;
						Cookie.Secure = Secure;
						Cookie.Expires = Expires;
						Cookie.HTTPOnly = HTTPOnly;
						return;
					}
				}

				Cookie Cookie;
				Cookie.Name = Key;
				Cookie.Value = Value;
				Cookie.Secure = Secure;
				Cookie.Expires = Expires;
				Cookie.HTTPOnly = HTTPOnly;

				if (Domain != nullptr)
					Cookie.Domain = Domain;

				if (Path != nullptr)
					Cookie.Path = Path;

				Cookies.emplace_back(std::move(Cookie));
			}
			const char* ResponseFrame::GetHeader(const char* Key)
			{
				if (!Key)
					return nullptr;

				for (auto& Head : Headers)
				{
					if (!Core::Parser::CaseCompare(Head.Key.c_str(), Key))
						return Head.Value.c_str();
				}

				return nullptr;
			}
			Cookie* ResponseFrame::GetCookie(const char* Key)
			{
				if (!Key)
					return nullptr;

				for (uint64_t i = 0; i < Cookies.size(); i++)
				{
					Cookie* Result = &Cookies[i];
					if (!Core::Parser::CaseCompare(Result->Name.c_str(), Key))
						return Result;
				}

				return nullptr;
			}
			std::string ResponseFrame::GetBuffer()
			{
				return std::string(Buffer.data(), Buffer.size());
			}

			bool Connection::Consume(const ContentCallback& Callback)
			{
				if (Request.ContentState == Content::Lost || Request.ContentState == Content::Empty || Request.ContentState == Content::Saved || Request.ContentState == Content::Wants_Save)
				{
					if (Callback)
						Callback(this, nullptr, 0);

					return true;
				}

				if (Request.ContentState == Content::Corrupted || Request.ContentState == Content::Payload_Exceeded || Request.ContentState == Content::Save_Exception)
				{
					if (Callback)
						Callback(this, nullptr, -1);

					return true;
				}

				if (Request.ContentState == Content::Cached)
				{
					if (Callback)
					{
						Callback(this, Request.Buffer.c_str(), (int)Request.Buffer.size());
						Callback(this, nullptr, 0);
					}

					return true;
				}

				if ((memcmp(Request.Method, "POST", 4) != 0 && memcmp(Request.Method, "PATCH", 5) != 0 && memcmp(Request.Method, "PUT", 3) != 0) && memcmp(Request.Method, "DELETE", 4) != 0)
				{
					Request.ContentState = Content::Empty;
					if (Callback)
						Callback(this, nullptr, 0);

					return false;
				}

				const char* ContentType = Request.GetHeader("Content-Type");
				if (ContentType && !strncmp(ContentType, "multipart/form-data", 19))
				{
					Request.ContentState = Content::Wants_Save;
					if (Callback)
						Callback(this, nullptr, 0);

					return true;
				}

				const char* TransferEncoding = Request.GetHeader("Transfer-Encoding");
				if (TransferEncoding && !Core::Parser::CaseCompare(TransferEncoding, "chunked"))
				{
					Parser* Parser = new HTTP::Parser();
					return Stream->ReadAsync((int64_t)Root->Router->PayloadMaxLength, [Parser, Callback](Network::Socket* Socket, const char* Buffer, int64_t Size)
					{
						Connection* Base = Socket->Context<Connection>();
						if (!Base)
							return false;

						if (Size > 0)
						{
							int64_t Result = Parser->ParseDecodeChunked((char*)Buffer, &Size);
							if (Result == -1)
							{
								TH_RELEASE(Parser);
								Base->Request.ContentState = Content::Corrupted;

								if (Callback)
									Callback(Base, nullptr, (int)Result);

								return false;
							}
							else if (Result >= 0 || Result == -2)
							{
								if (Callback)
									Callback(Base, Buffer, (int)Size);

								if (!Base->Route || Base->Request.Buffer.size() < Base->Route->MaxCacheLength)
									Base->Request.Buffer.append(Buffer, Size);
							}

							return Result == -2;
						}

						TH_RELEASE(Parser);
						if (Size != -1)
						{
							if (!Base->Route || Base->Request.Buffer.size() < Base->Route->MaxCacheLength)
								Base->Request.ContentState = Content::Cached;
							else
								Base->Request.ContentState = Content::Lost;
						}
						else
							Base->Request.ContentState = Content::Corrupted;

						if (Callback)
							Callback(Base, nullptr, Size);

						return true;
					}) > 0;
				}
				else if (!Request.GetHeader("Content-Length"))
				{
					return Stream->ReadAsync((int64_t)Root->Router->PayloadMaxLength, [Callback](Network::Socket* Socket, const char* Buffer, int64_t Size)
					{
						Connection* Base = Socket->Context<Connection>();
						if (!Base)
							return false;

						if (Size <= 0)
						{
							if (!Base->Route || Base->Request.Buffer.size() < Base->Route->MaxCacheLength)
								Base->Request.ContentState = Content::Cached;
							else
								Base->Request.ContentState = Content::Lost;

							if (Callback)
								Callback(Base, nullptr, Size);

							return false;
						}

						if (Callback)
							Callback(Base, Buffer, Size);

						if (!Base->Route || Base->Request.Buffer.size() < Base->Route->MaxCacheLength)
							Base->Request.Buffer.append(Buffer, Size);

						return true;
					}) > 0;
				}

				if (Request.ContentLength > Root->Router->PayloadMaxLength)
				{
					Request.ContentState = Content::Payload_Exceeded;
					if (Callback)
						Callback(this, nullptr, 0);

					return false;
				}

				if (!Route || Request.ContentLength > Route->MaxCacheLength)
				{
					Request.ContentState = Content::Wants_Save;
					if (Callback)
						Callback(this, nullptr, 0);

					return true;
				}

				return Stream->ReadAsync((int64_t)Request.ContentLength, [Callback](Network::Socket* Socket, const char* Buffer, int64_t Size)
				{
					Connection* Base = Socket->Context<Connection>();
					if (!Base)
						return false;

					if (Size <= 0)
					{
						if (Size != -1)
						{
							if (!Base->Route || Base->Request.Buffer.size() < Base->Route->MaxCacheLength)
								Base->Request.ContentState = Content::Cached;
							else
								Base->Request.ContentState = Content::Lost;
						}
						else
							Base->Request.ContentState = Content::Corrupted;

						if (Callback)
							Callback(Base, nullptr, Size);

						return false;
					}

					if (Callback)
						Callback(Base, Buffer, Size);

					if (!Base->Route || Base->Request.Buffer.size() < Base->Route->MaxCacheLength)
						Base->Request.Buffer.append(Buffer, Size);

					return true;
				}) > 0;
			}
			bool Connection::Store(const ResourceCallback& Callback)
			{
				if (!Route || Request.ContentState == Content::Lost || Request.ContentState == Content::Empty || Request.ContentState == Content::Cached)
				{
					if (Callback)
						Callback(this, nullptr, 0);

					return false;
				}

				if (Request.ContentState == Content::Corrupted || Request.ContentState == Content::Payload_Exceeded || Request.ContentState == Content::Save_Exception)
				{
					if (Callback)
						Callback(this, nullptr, -1);

					return false;
				}

				if (Request.ContentState == Content::Saved)
				{
					if (!Callback || Request.Resources.empty())
						return true;

					for (auto& Item : Request.Resources)
						Callback(this, &Item, (int64_t)Item.Length);

					Callback(this, nullptr, 0);
					return true;
				}

				const char* ContentType = Request.GetHeader("Content-Type"), *BoundaryName;
				if (ContentType && !strncmp(ContentType, "multipart/form-data", 19))
					Request.ContentState = Content::Wants_Save;

				if (ContentType != nullptr && (BoundaryName = strstr(ContentType, "boundary=")))
				{
					std::string Boundary("--");
					Boundary.append(BoundaryName + 9);

					ParserFrame* Segment = TH_NEW(ParserFrame);
					Segment->Route = Route;
					Segment->Request = &Request;
					Segment->Response = &Response;
					Segment->Callback = Callback;

					Parser* Parser = new HTTP::Parser();
					Parser->OnContentData = Util::ParseMultipartContentData;
					Parser->OnHeaderField = Util::ParseMultipartHeaderField;
					Parser->OnHeaderValue = Util::ParseMultipartHeaderValue;
					Parser->OnResourceBegin = Util::ParseMultipartResourceBegin;
					Parser->OnResourceEnd = Util::ParseMultipartResourceEnd;
					Parser->UserPointer = Segment;

					return Stream->ReadAsync((int64_t)Request.ContentLength, [Parser, Segment, Boundary](Network::Socket* Socket, const char* Buffer, int64_t Size)
					{
						Connection* Base = Socket->Context<Connection>();
						if (!Base)
							return false;

						if (Size <= 0)
						{
							if (Size == -1)
								Base->Request.ContentState = Content::Corrupted;
							else
								Base->Request.ContentState = Content::Saved;

							if (Segment->Callback)
								Segment->Callback(Base, nullptr, Size);

							TH_RELEASE(Parser);
							TH_DELETE(ParserFrame, Segment);

							return true;
						}

						if (Parser->MultipartParse(Boundary.c_str(), Buffer, Size) == -1 || Segment->Close)
						{
							Base->Request.ContentState = Content::Saved;
							if (Segment->Callback)
								Segment->Callback(Base, nullptr, 0);

							TH_RELEASE(Parser);
							TH_DELETE(ParserFrame, Segment);

							return false;
						}

						return true;
					}) > 0;
				}
				else if (Request.ContentLength > 0)
				{
					HTTP::Resource fResource;
					fResource.Length = Request.ContentLength;
					fResource.Type = (ContentType ? ContentType : "application/octet-stream");
					fResource.Path = Core::OS::Directory::Get() + Compute::Common::MD5Hash(Compute::Common::RandomBytes(16));

					FILE* File = (FILE*)Core::OS::File::Open(fResource.Path.c_str(), "wb");
					if (!File)
					{
						Request.ContentState = Content::Save_Exception;
						return false;
					}

					return Stream->ReadAsync((int64_t)Request.ContentLength, [File, fResource, Callback](Network::Socket* Socket, const char* Buffer, int64_t Size)
					{
						Connection* Base = Socket->Context<Connection>();
						if (!Base)
							return false;

						if (Size <= 0)
						{
							if (Size != -1)
							{
								Base->Request.ContentState = Content::Saved;
								Base->Request.Resources.push_back(fResource);

								if (Callback)
									Callback(Base, &Base->Request.Resources.back(), Size);
							}
							else
								Base->Request.ContentState = Content::Corrupted;

							if (Callback)
								Callback(Base, nullptr, Size);

							fclose(File);
							return false;
						}

						if (fwrite(Buffer, 1, (size_t)Size, File) == Size)
							return true;

						Base->Request.ContentState = Content::Save_Exception;
						fclose(File);

						if (Callback)
							Callback(Base, nullptr, 0);

						return false;
					}) > 0;
				}
				else if (Callback)
					Callback(this, nullptr, 0);

				return true;
			}
			bool Connection::Finish()
			{
				Info.Sync.lock();
				if (Info.Await)
				{
					Info.Sync.unlock();
					return false;
				}

				Info.Sync.unlock();
				if (WebSocket != nullptr)
				{
					if (!WebSocket->IsFinished())
					{
						WebSocket->Finish();
						return true;
					}

					TH_DELETE(WebSocketFrame, WebSocket);
					WebSocket = nullptr;
				}

				if (Gateway != nullptr)
				{
					if (!Gateway->IsDone())
					{
						Gateway->Finish();
						return true;
					}

					TH_DELETE(GatewayFrame, Gateway);
					Gateway = nullptr;
				}

				if (Response.StatusCode < 0 || Stream->Outcome > 0 || !Stream->IsValid())
					return Root->Manage(this);

				if (Response.StatusCode >= 400 && !Response.Error)
				{
					Response.Error = true;
					if (Route != nullptr)
					{
						for (auto& Page : Route->ErrorFiles)
						{
							if (Page.StatusCode != Response.StatusCode && Page.StatusCode != 0)
								continue;

							Request.Path = Page.Pattern;
							Response.SetHeader("X-Error", Info.Message);
							return Util::RouteGET(this);
						}
					}

					const char* StatusText = Util::StatusMessage(Response.StatusCode);
					Core::Parser Content;
					Content.fAppend("%s %d %s\r\n", Request.Version, Response.StatusCode, StatusText);

					Util::ConstructHeadUncache(this, &Content);
					if (Route && Route->Callbacks.Headers)
						Route->Callbacks.Headers(this, nullptr);

					char Date[64];
					Core::DateTime::TimeFormatGMT(Date, sizeof(Date), Info.Start / 1000);

					std::string Auth;
					if (Route && Request.User.Type == Auth::Denied)
						Auth = "WWW-Authenticate: " + Route->Auth.Type + " realm=\"" + Route->Auth.Realm + "\"\r\n";

					int HasContents = (Response.StatusCode > 199 && Response.StatusCode != 204 && Response.StatusCode != 304);
					if (HasContents)
					{
						char Buffer[2048];
						snprintf(Buffer, sizeof(Buffer), "<html><head><title>%d %s</title><style>html{font-family:\"Helvetica Neue\",Helvetica,Arial,sans-serif;height:95%%;}body{display:flex;align-items:center;justify-content:center;height:100%%;}div{text-align:center;}</style></head><body><div><h1>%d %s</h1></div></body></html>\n", Response.StatusCode, StatusText, Response.StatusCode, Info.Message.empty() ? StatusText : Info.Message.c_str());

						if (Route && Route->Callbacks.Headers)
							Route->Callbacks.Headers(this, &Content);

						Content.fAppend("Date: %s\r\n%sContent-Type: text/html; charset=%s\r\nAccept-Ranges: bytes\r\nContent-Length: %llu\r\n%s\r\n%s", Date, Util::ConnectionResolve(this).c_str(), Route ? Route->CharSet.c_str() : "utf-8", (uint64_t)strlen(Buffer), Auth.c_str(), Buffer);
					}
					else
					{
						if (Route && Route->Callbacks.Headers)
							Route->Callbacks.Headers(this, &Content);

						Content.fAppend("Date: %s\r\nAccept-Ranges: bytes\r\n%s%s\r\n", Date, Util::ConnectionResolve(this).c_str(), Auth.c_str());
					}

					return Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [](Network::Socket* Socket, int64_t Size)
					{
						auto Base = Socket->Context<HTTP::Connection>();
						if (Size < 0)
							return Base->Root->Manage(Base);
						else if (Size > 0)
							return true;

						return Base->Root->Manage(Base);
					});
				}

				Core::Parser Content;
				std::string Boundary;
				const char* ContentType;
				Content.fAppend("%s %d %s\r\n", Request.Version, Response.StatusCode, Util::StatusMessage(Response.StatusCode));

				if (!Response.GetHeader("Date"))
				{
					char Buffer[64];
					Core::DateTime::TimeFormatGMT(Buffer, sizeof(Buffer), Info.Start / 1000);
					Content.fAppend("Date: %s\r\n", Buffer);
				}

				if (!Response.GetHeader("Connection"))
					Content.Append(Util::ConnectionResolve(this));

				if (!Response.GetHeader("Accept-Ranges"))
					Content.Append("Accept-Ranges: bytes\r\n", 22);

				if (!Response.GetHeader("Content-Type"))
				{
					if (Route != nullptr)
						ContentType = Util::ContentType(Request.Path, &Route->MimeTypes);
					else
						ContentType = "application/octet-stream";

					if (Request.GetHeader("Range") != nullptr)
					{
						Boundary = Util::ParseMultipartDataBoundary();
						Content.fAppend("Content-Type: multipart/byteranges; boundary=%s; charset=%s\r\n", ContentType, Boundary.c_str(), Route ? Route->CharSet.c_str() : "utf-8");
					}
					else
						Content.fAppend("Content-Type: %s; charset=%s\r\n", ContentType, Route ? Route->CharSet.c_str() : "utf-8");
				}
				else
					ContentType = Response.GetHeader("Content-Type");

				if (!Response.Buffer.empty())
				{
#ifdef TH_HAS_ZLIB
					bool Deflate = false, Gzip = false;
					if (Util::ResourceCompressed(this, Response.Buffer.size()))
					{
						const char* AcceptEncoding = Request.GetHeader("Accept-Encoding");
						if (AcceptEncoding != nullptr)
						{
							Deflate = strstr(AcceptEncoding, "deflate") != nullptr;
							Gzip = strstr(AcceptEncoding, "gzip") != nullptr;
						}

						if (AcceptEncoding != nullptr && (Deflate || Gzip))
						{
							z_stream fStream;
							fStream.zalloc = Z_NULL;
							fStream.zfree = Z_NULL;
							fStream.opaque = Z_NULL;
							fStream.avail_in = (uInt)Response.Buffer.size();
							fStream.next_in = (Bytef*)Response.Buffer.data();

							if (deflateInit2(&fStream, Route ? Route->Compression.QualityLevel : 8, Z_DEFLATED, (Gzip ? 15 | 16 : 15), Route ? Route->Compression.MemoryLevel : 8, Route ? (int)Route->Compression.Tune : 0) == Z_OK)
							{
								std::string Buffer(Response.Buffer.size(), '\0');
								fStream.avail_out = (uInt)Buffer.size();
								fStream.next_out = (Bytef*)Buffer.c_str();
								bool Compress = (deflate(&fStream, Z_FINISH) == Z_STREAM_END);
								bool Flush = (deflateEnd(&fStream) == Z_OK);

								if (Compress && Flush)
								{
									TextAssign(Response.Buffer, Buffer.c_str(), (uint64_t)fStream.total_out);
									if (!Response.GetHeader("Content-Encoding"))
									{
										if (Gzip)
											Content.Append("Content-Encoding: gzip\r\n", 24);
										else
											Content.Append("Content-Encoding: deflate\r\n", 27);
									}
								}
							}
						}
					}
#endif
					if (Request.GetHeader("Range") != nullptr)
					{
						std::vector<std::pair<int64_t, int64_t>> Ranges = Request.GetRanges();
						if (Ranges.size() > 1)
						{
							std::string Data;
							for (auto It = Ranges.begin(); It != Ranges.end(); ++It)
							{
								std::pair<uint64_t, uint64_t> Offset = Request.GetRange(It, Response.Buffer.size());
								std::string ContentRange = Util::ConstructContentRange(Offset.first, Offset.second, Response.Buffer.size());

								Data.append("--", 2);
								Data.append(Boundary);
								Data.append("\r\n", 2);

								if (ContentType != nullptr)
								{
									Data.append("Content-Type: ", 14);
									Data.append(ContentType);
									Data.append("\r\n", 2);
								}

								Data.append("Content-Range: ", 15);
								Data.append(ContentRange.c_str(), ContentRange.size());
								Data.append("\r\n", 2);
								Data.append("\r\n", 2);
								Data.append(TextSubstring(Response.Buffer, Offset.first, Offset.second));
								Data.append("\r\n", 2);
							}

							Data.append("--", 2);
							Data.append(Boundary);
							Data.append("--\r\n", 4);

							TextAssign(Response.Buffer, Data);
						}
						else
						{
							std::pair<uint64_t, uint64_t> Offset = Request.GetRange(Ranges.begin(), Response.Buffer.size());
							if (!Response.GetHeader("Content-Range"))
								Content.fAppend("Content-Range: %s\r\n", Util::ConstructContentRange(Offset.first, Offset.second, Response.Buffer.size()).c_str());

							TextAssign(Response.Buffer, TextSubstring(Response.Buffer, Offset.first, Offset.second));
						}
					}

					if (!Response.GetHeader("Content-Length"))
						Content.fAppend("Content-Length: %llu\r\n", (uint64_t)Response.Buffer.size());
				}
				else if (!Response.GetHeader("Content-Length"))
					Content.Append("Content-Length: 0\r\n", 19);

				for (auto& Item : Response.Headers)
					Content.fAppend("%s: %s\r\n", Item.Key.c_str(), Item.Value.c_str());

				for (auto& Item : Response.Cookies)
				{
					if (!Item.Domain.empty())
						Item.Domain.insert(0, "; domain=");

					if (!Item.Path.empty())
						Item.Path.insert(0, "; path=");

					Content.fAppend("Set-Cookie: %s=%s; expires=%s%s%s%s%s\r\n", Item.Name.c_str(), Item.Value.c_str(), Core::DateTime::GetGMTBasedString(Item.Expires).c_str(), Item.Path.c_str(), Item.Domain.c_str(), Item.Secure ? "; secure" : "", Item.HTTPOnly ? "; HTTPOnly" : "");
				}

				if (Route && Route->Callbacks.Headers)
					Route->Callbacks.Headers(this, &Content);

				Content.Append("\r\n", 2);
				return Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [](Socket* Socket, int64_t Size)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Size < 0)
						return Base->Root->Manage(Base);
					else if (Size > 0)
						return true;

					if (memcmp(Base->Request.Method, "HEAD", 4) == 0)
						return Base->Root->Manage(Base);

					if (!Base->Response.Buffer.empty())
					{
						return !Socket->WriteAsync(Base->Response.Buffer.data(), (int64_t)Base->Response.Buffer.size(), [](Network::Socket* Socket, int64_t Size)
						{
							auto Base = Socket->Context<HTTP::Connection>();
							if (Size > 0)
								return true;

							return Base->Root->Manage(Base);
						});
					}

					return Base->Root->Manage(Base);
				});
			}
			bool Connection::Finish(int StatusCode)
			{
				Response.StatusCode = StatusCode;
				return Finish();
			}
			bool Connection::Certify(Certificate* Output)
			{
#ifdef TH_HAS_OPENSSL
				if (!Output)
					return false;

				X509* Certificate = SSL_get_peer_certificate(Stream->GetDevice());
				if (!Certificate)
					return false;

				const EVP_MD* Digest = EVP_get_digestbyname("sha1");
				X509_NAME* Subject = X509_get_subject_name(Certificate);
				X509_NAME* Issuer = X509_get_issuer_name(Certificate);
				ASN1_INTEGER* Serial = X509_get_serialNumber(Certificate);

				char SubjectBuffer[1024];
				X509_NAME_oneline(Subject, SubjectBuffer, (int)sizeof(SubjectBuffer));

				char IssuerBuffer[1024], SerialBuffer[1024];
				X509_NAME_oneline(Issuer, IssuerBuffer, (int)sizeof(IssuerBuffer));

				unsigned char Buffer[256];
				int Length = i2d_ASN1_INTEGER(Serial, nullptr);

				if (Length > 0 && (unsigned)Length < (unsigned)sizeof(Buffer))
				{
					unsigned char* Pointer = Buffer;
					int Size = i2d_ASN1_INTEGER(Serial, &Pointer);

					if (!Compute::Common::HexToString(Buffer, (uint64_t)Size, SerialBuffer, sizeof(SerialBuffer)))
						*SerialBuffer = '\0';
				}
				else
					*SerialBuffer = '\0';

				unsigned int Size = 0;
				ASN1_digest((int(*)(void*, unsigned char**))i2d_X509, Digest, (char*)Certificate, Buffer, &Size);

				char FingerBuffer[1024];
				if (!Compute::Common::HexToString(Buffer, (uint64_t)Size, FingerBuffer, sizeof(FingerBuffer)))
					*FingerBuffer = '\0';

				Output->Subject = SubjectBuffer;
				Output->Issuer = IssuerBuffer;
				Output->Serial = SerialBuffer;
				Output->Finger = FingerBuffer;

				X509_free(Certificate);
				return true;
#else
				return false;
#endif
			}

			QueryParameter::QueryParameter() : Core::Document(Core::Var::Object())
			{
			}
			std::string QueryParameter::Build()
			{
				std::string Output, Label = Compute::Common::URIEncode(Parent != nullptr ? ('[' + Key + ']') : Key);
				if (Value.IsObject())
				{
					for (auto It = Nodes.begin(); It != Nodes.end(); ++It)
					{
						Output.append(Label).append((*It)->As<QueryParameter>()->Build());
						if (It + 1 < Nodes.end())
							Output += '&';
					}
				}
				else
				{
					std::string V = Value.Serialize();
					if (!V.empty())
						Output.append(Label).append(1, '=').append(Compute::Common::URIEncode(V));
					else
						Output.append(Label);
				}

				return Output;
			}
			std::string QueryParameter::BuildFromBase()
			{
				std::string Output, Label = Compute::Common::URIEncode(Key);
				if (Value.IsObject())
				{
					for (auto It = Nodes.begin(); It != Nodes.end(); ++It)
					{
						Output.append(Label).append((*It)->As<QueryParameter>()->Build());
						if (It + 1 < Nodes.end())
							Output += '&';
					}
				}
				else
				{
					std::string V = Value.Serialize();
					if (!V.empty())
						Output.append(Label).append(1, '=').append(Compute::Common::URIEncode(V));
					else
						Output.append(Label);
				}

				return Output;
			}
			QueryParameter* QueryParameter::Find(QueryToken* Name)
			{
				if (!Name)
					return nullptr;

				if (Name->Value && Name->Length > 0)
				{
					for (auto* Item : Nodes)
					{
						if (!strncmp(Item->Key.c_str(), Name->Value, (size_t)Name->Length))
							return (QueryParameter*)Item;
					}
				}

				QueryParameter* New = new QueryParameter();
				if (Name->Value && Name->Length > 0)
				{
					New->Key.assign(Name->Value, (size_t)Name->Length);
					if (!Core::Parser(&New->Key).HasInteger())
						Value = Core::Var::Object();
					else
						Value = Core::Var::Array();
				}
				else
				{
					New->Key.assign(std::to_string(Nodes.size()));
					Value = Core::Var::Array();
				}

				New->Value = Core::Var::String("", 0);
				New->Parent = this;
				Nodes.push_back(New);

				return New;
			}

			Query::Query() : Object(new QueryParameter())
			{
			}
			Query::~Query()
			{
				TH_RELEASE(Object);
			}
			void Query::Clear()
			{
				if (Object != nullptr)
					Object->Clear();
			}
			void Query::NewParameter(std::vector<QueryToken>* Tokens, const QueryToken& Name, const QueryToken& Value)
			{
				std::string URI = Compute::Common::URIDecode(Name.Value, Name.Length);
				char* Data = (char*)URI.c_str();

				uint64_t Offset = 0, Length = URI.size();
				for (uint64_t i = 0; i < Length; i++)
				{
					if (Data[i] != '[')
					{
						if (Tokens->empty() && i + 1 >= Length)
						{
							QueryToken Token;
							Token.Value = Data + Offset;
							Token.Length = i - Offset + 1;
							Tokens->push_back(Token);
						}

						continue;
					}

					if (Tokens->empty())
					{
						QueryToken Token;
						Token.Value = Data + Offset;
						Token.Length = i - Offset;
						Tokens->push_back(Token);
					}

					Offset = i;
					while (i + 1 < Length && Data[i + 1] != ']')
						i++;

					QueryToken Token;
					Token.Value = Data + Offset + 1;
					Token.Length = i - Offset;
					Tokens->push_back(Token);

					if (i + 1 >= Length)
						break;

					Offset = i + 1;
				}

				if (!Value.Value || !Value.Length)
					return;

				QueryParameter* Parameter = nullptr;
				for (auto& Item : *Tokens)
				{
					if (Parameter != nullptr)
						Parameter = Parameter->Find(&Item);
					else
						Parameter = GetParameter(&Item);
				}

				if (Parameter != nullptr)
					Parameter->Value.Deserialize(Compute::Common::URIDecode(Value.Value, Value.Length));
			}
			void Query::Decode(const char* Type, const std::string& URI)
			{
				if (!Type || URI.empty())
					return;

				if (!Core::Parser::CaseCompare(Type, "application/x-www-form-urlencoded"))
					DecodeAXWFD(URI);
				else if (!Core::Parser::CaseCompare(Type, "application/json"))
					DecodeAJSON(URI);
			}
			void Query::DecodeAXWFD(const std::string& URI)
			{
				std::vector<QueryToken> Tokens;
				char* Data = (char*)URI.c_str();

				uint64_t Offset = 0, Length = URI.size();
				for (uint64_t i = 0; i < Length; i++)
				{
					if (Data[i] != '&' && Data[i] != '=' && i + 1 < Length)
						continue;

					QueryToken Name;
					Name.Value = Data + Offset;
					Name.Length = i - Offset;
					Offset = i;

					if (Data[i] == '=')
					{
						while (i + 1 < Length && Data[i + 1] != '&')
							i++;
					}

					QueryToken Value;
					Value.Value = Data + Offset + 1;
					Value.Length = i - Offset;

					NewParameter(&Tokens, Name, Value);
					Tokens.clear();
					Offset = i + 2;
					i++;
				}
			}
			void Query::DecodeAJSON(const std::string& URI)
			{
				size_t Offset = 0;
				TH_CLEAR(Object);

				Object = (QueryParameter*)Core::Document::ReadJSON(URI.size(), [&URI, &Offset](char* Buffer, int64_t Size)
				{
					if (!Buffer || !Size)
						return true;

					size_t Length = URI.size() - Offset;
					if (Size < Length)
						Length = Size;

					if (!Length)
						return false;

					memcpy(Buffer, URI.c_str() + Offset, Length);
					Offset += Length;
					return true;
				});
			}
			std::string Query::Encode(const char* Type)
			{
				if (Type != nullptr)
				{
					if (!Core::Parser::CaseCompare(Type, "application/x-www-form-urlencoded"))
						return EncodeAXWFD();

					if (!Core::Parser::CaseCompare(Type, "application/json"))
						return EncodeAJSON();
				}

				return "";
			}
			std::string Query::EncodeAXWFD()
			{
				std::string Output; auto* Nodes = Object->GetNodes();
				for (auto It = Nodes->begin(); It != Nodes->end(); ++It)
				{
					if (It + 1 < Nodes->end())
						Output.append((*It)->As<QueryParameter>()->BuildFromBase()).append(1, '&');
					else
						Output.append((*It)->As<QueryParameter>()->BuildFromBase());
				}

				return Output;
			}
			std::string Query::EncodeAJSON()
			{
				std::string Stream;
				Core::Document::WriteJSON(Object, [&Stream](Core::VarForm, const char* Buffer, int64_t Length)
				{
					if (Buffer != nullptr && Length > 0)
						Stream.append(Buffer, Length);
				});

				return Stream;
			}
			QueryParameter* Query::Get(const char* Name)
			{
				return (QueryParameter*)Object->Get(Name);
			}
			QueryParameter* Query::Set(const char* Name)
			{
				return (QueryParameter*)Object->Set(Name, Core::Var::String("", 0));
			}
			QueryParameter* Query::Set(const char* Name, const char* Value)
			{
				return (QueryParameter*)Object->Set(Name, Core::Var::String(Value));
			}
			QueryParameter* Query::GetParameter(QueryToken* Name)
			{
				if (!Name)
					return nullptr;

				if (Name->Value && Name->Length > 0)
				{
					for (auto* Item : *Object->GetNodes())
					{
						if (Item->Key.size() != Name->Length)
							continue;

						if (!strncmp(Item->Key.c_str(), Name->Value, (size_t)Name->Length))
							return (QueryParameter*)Item;
					}
				}

				QueryParameter* New = new QueryParameter();
				if (Name->Value && Name->Length > 0)
				{
					New->Key.assign(Name->Value, (size_t)Name->Length);
					if (!Core::Parser(&New->Key).HasInteger())
						Object->Value = Core::Var::Object();
					else
						Object->Value = Core::Var::Array();
				}
				else
				{
					New->Key.assign(std::to_string(Object->GetNodes()->size()));
					Object->Value = Core::Var::Array();
				}

				New->Value = Core::Var::String("", 0);
				Object->GetNodes()->push_back(New);

				return New;
			}

			Session::Session()
			{
				Query = Core::Document::Object();
			}
			Session::~Session()
			{
				TH_RELEASE(Query);
			}
			void Session::Clear()
			{
				if (Query != nullptr)
					Query->Clear();
			}
			bool Session::Write(Connection* Base)
			{
				if (!Base || !Base->Route || !Query || Query->IsSaved())
					return false;

				std::string Document = Base->Route->Site->Gateway.Session.DocumentRoot + FindSessionId(Base);

				FILE* Stream = (FILE*)Core::OS::File::Open(Document.c_str(), "wb");
				if (!Stream)
					return false;

				SessionExpires = time(nullptr) + Base->Route->Site->Gateway.Session.Expires;
				fwrite(&SessionExpires, sizeof(int64_t), 1, Stream);

				Query->WriteJSONB(Query, [Stream](Core::VarForm, const char* Buffer, int64_t Size)
				{
					if (Buffer != nullptr && Size > 0)
						fwrite(Buffer, Size, 1, Stream);
				});

				fclose(Stream);
				return true;
			}
			bool Session::Read(Connection* Base)
			{
				if (!Base || !Base->Route)
					return false;

				std::string Document = Base->Route->Site->Gateway.Session.DocumentRoot + FindSessionId(Base);

				FILE* Stream = (FILE*)Core::OS::File::Open(Document.c_str(), "rb");
				if (!Stream)
					return false;

				fseek(Stream, 0, SEEK_END);
				if (ftell(Stream) == 0)
				{
					fclose(Stream);
					return false;
				}

				fseek(Stream, 0, SEEK_SET);
				if (fread(&SessionExpires, 1, sizeof(int64_t), Stream) != sizeof(int64_t))
				{
					fclose(Stream);
					return false;
				}

				if (SessionExpires <= time(nullptr))
				{
					SessionId.clear();
					fclose(Stream);

					if (!Core::OS::File::Remove(Document.c_str()))
						TH_ERROR("session file %s cannot be deleted", Document.c_str());

					return false;
				}


				Core::Document* V = Core::Document::ReadJSONB([Stream](char* Buffer, int64_t Size)
				{
					if (!Buffer || !Size)
						return true;

					return fread(Buffer, sizeof(char), Size, Stream) == Size;
				});

				if (V != nullptr)
				{
					TH_RELEASE(Query);
					Query = V;
				}

				fclose(Stream);
				return true;
			}
			std::string& Session::FindSessionId(Connection* Base)
			{
				if (!Base || !Base->Route || !SessionId.empty())
					return SessionId;

				const char* Value = Base->Request.GetCookie(Base->Route->Site->Gateway.Session.Name.c_str());
				if (!Value)
					return GenerateSessionId(Base);

				return SessionId.assign(Value);
			}
			std::string& Session::GenerateSessionId(Connection* Base)
			{
				if (!Base || !Base->Route)
					return SessionId;

				int64_t Time = time(nullptr);
				SessionId = Compute::Common::MD5Hash(Base->Request.URI + std::to_string(Time));
				IsNewSession = true;

				if (SessionExpires == 0)
					SessionExpires = Time + Base->Route->Site->Gateway.Session.Expires;

				Base->Response.SetCookie(Base->Route->Site->Gateway.Session.Name.c_str(), SessionId.c_str(), Time + (int64_t)Base->Route->Site->Gateway.Session.CookieExpires, Base->Route->Site->Gateway.Session.Domain.c_str(), Base->Route->Site->Gateway.Session.Path.c_str(), false);
				return SessionId;
			}
			bool Session::InvalidateCache(const std::string& Path)
			{
				std::vector<Core::ResourceEntry> Entries;
				if (!Core::OS::Directory::Scan(Path, &Entries))
					return false;

				bool Split = (Path.back() != '\\' && Path.back() != '/');
				for (auto& Item : Entries)
				{
					if (Item.Source.IsDirectory)
						continue;

					std::string Filename = (Split ? Path + '/' : Path) + Item.Path;
					if (!Core::OS::File::Remove(Filename.c_str()))
						TH_ERROR("couldn't invalidate session\n\t%s", Item.Path.c_str());
				}

				return true;
			}

			Parser::Parser()
			{
			}
			Parser::~Parser()
			{
				TH_FREE(Multipart.Boundary);
			}
			int64_t Parser::MultipartParse(const char* Boundary, const char* Buffer, int64_t Length)
			{
				if (!Multipart.Boundary || !Multipart.LookBehind)
				{
					if (!Boundary)
						return -1;

					if (Multipart.Boundary)
						TH_FREE(Multipart.Boundary);

					Multipart.Length = strlen(Boundary);
					Multipart.Boundary = (char*)TH_MALLOC(sizeof(char) * (size_t)(Multipart.Length * 2 + 9));
					memcpy(Multipart.Boundary, Boundary, sizeof(char) * (size_t)Multipart.Length);
					Multipart.Boundary[Multipart.Length] = '\0';
					Multipart.LookBehind = (Multipart.Boundary + Multipart.Length + 1);
					Multipart.Index = 0;
					Multipart.State = MultipartState_Start;
				}

				char Value, Lower;
				char LF = 10, CR = 13;
				int64_t i = 0, Mark = 0;
				int Last = 0;

				while (i < Length)
				{
					Value = Buffer[i];
					Last = (i == (Length - 1));

					switch (Multipart.State)
					{
						case MultipartState_Start:
							Multipart.Index = 0;
							Multipart.State = MultipartState_Start_Boundary;
						case MultipartState_Start_Boundary:
							if (Multipart.Index == Multipart.Length)
							{
								if (Value != CR)
									return i;

								Multipart.Index++;
								break;
							}
							else if (Multipart.Index == (Multipart.Length + 1))
							{
								if (Value != LF)
									return i;

								Multipart.Index = 0;
								if (OnResourceBegin && !OnResourceBegin(this))
									return i;

								Multipart.State = MultipartState_Header_Field_Start;
								break;
							}

							if (Value != Multipart.Boundary[Multipart.Index])
								return i;

							Multipart.Index++;
							break;
						case MultipartState_Header_Field_Start:
							Mark = i;
							Multipart.State = MultipartState_Header_Field;
						case MultipartState_Header_Field:
							if (Value == CR)
							{
								Multipart.State = MultipartState_Header_Field_Waiting;
								break;
							}

							if (Value == ':')
							{
								if (OnHeaderField && !OnHeaderField(this, Buffer + Mark, i - Mark))
									return i;

								Multipart.State = MultipartState_Header_Value_Start;
								break;
							}

							Lower = tolower(Value);
							if ((Value != '-') && (Lower < 'a' || Lower > 'z'))
								return i;

							if (Last && OnHeaderField && !OnHeaderField(this, Buffer + Mark, (i - Mark) + 1))
								return i;

							break;
						case MultipartState_Header_Field_Waiting:
							if (Value != LF)
								return i;

							Multipart.State = MultipartState_Resource_Start;
							break;
						case MultipartState_Header_Value_Start:
							if (Value == ' ')
								break;

							Mark = i;
							Multipart.State = MultipartState_Header_Value;
						case MultipartState_Header_Value:
							if (Value == CR)
							{
								if (OnHeaderValue && !OnHeaderValue(this, Buffer + Mark, i - Mark))
									return i;

								Multipart.State = MultipartState_Header_Value_Waiting;
								break;
							}

							if (Last && OnHeaderValue && !OnHeaderValue(this, Buffer + Mark, (i - Mark) + 1))
								return i;

							break;
						case MultipartState_Header_Value_Waiting:
							if (Value != LF)
								return i;

							Multipart.State = MultipartState_Header_Field_Start;
							break;
						case MultipartState_Resource_Start:
							Mark = i;
							Multipart.State = MultipartState_Resource;
						case MultipartState_Resource:
							if (Value == CR)
							{
								if (OnContentData && !OnContentData(this, Buffer + Mark, i - Mark))
									return i;

								Mark = i;
								Multipart.State = MultipartState_Resource_Boundary_Waiting;
								Multipart.LookBehind[0] = CR;
								break;
							}

							if (Last && OnContentData && !OnContentData(this, Buffer + Mark, (i - Mark) + 1))
								return i;

							break;
						case MultipartState_Resource_Boundary_Waiting:
							if (Value == LF)
							{
								Multipart.State = MultipartState_Resource_Boundary;
								Multipart.LookBehind[1] = LF;
								Multipart.Index = 0;
								break;
							}

							if (OnContentData && !OnContentData(this, Multipart.LookBehind, 1))
								return i;

							Multipart.State = MultipartState_Resource;
							Mark = i--;
							break;
						case MultipartState_Resource_Boundary:
							if (Multipart.Boundary[Multipart.Index] != Value)
							{
								if (OnContentData && !OnContentData(this, Multipart.LookBehind, 2 + Multipart.Index))
									return i;

								Multipart.State = MultipartState_Resource;
								Mark = i--;
								break;
							}

							Multipart.LookBehind[2 + Multipart.Index] = Value;
							if ((++Multipart.Index) == Multipart.Length)
							{
								if (OnResourceEnd && !OnResourceEnd(this))
									return i;

								Multipart.State = MultipartState_Resource_Waiting;
							}
							break;
						case MultipartState_Resource_Waiting:
							if (Value == '-')
							{
								Multipart.State = MultipartState_Resource_Hyphen;
								break;
							}

							if (Value == CR)
							{
								Multipart.State = MultipartState_Resource_End;
								break;
							}

							return i;
						case MultipartState_Resource_Hyphen:
							if (Value == '-')
							{
								Multipart.State = MultipartState_End;
								break;
							}

							return i;
						case MultipartState_Resource_End:
							if (Value == LF)
							{
								Multipart.State = MultipartState_Header_Field_Start;
								if (OnResourceBegin && !OnResourceBegin(this))
									return i;

								break;
							}

							return i;
						case MultipartState_End:
							break;
						default:
							return -1;
					}

					++i;
				}

				return Length;
			}
			int64_t Parser::ParseRequest(const char* BufferStart, uint64_t Length, uint64_t LastLength)
			{
				const char* Buffer = BufferStart;
				const char* BufferEnd = BufferStart + Length;
				int Result;

				if (LastLength != 0 && Complete(Buffer, BufferEnd, LastLength, &Result) == nullptr)
					return (int64_t)Result;

				if ((Buffer = ProcessRequest(Buffer, BufferEnd, &Result)) == nullptr)
					return (int64_t)Result;

				return (int64_t)(Buffer - BufferStart);
			}
			int64_t Parser::ParseResponse(const char* BufferStart, uint64_t Length, uint64_t LastLength)
			{
				const char* Buffer = BufferStart;
				const char* BufferEnd = Buffer + Length;
				int Result;

				if (LastLength != 0 && Complete(Buffer, BufferEnd, LastLength, &Result) == nullptr)
					return (int64_t)Result;

				if ((Buffer = ProcessResponse(Buffer, BufferEnd, &Result)) == nullptr)
					return (int64_t)Result;

				return (int64_t)(Buffer - BufferStart);
			}
			int64_t Parser::ParseDecodeChunked(char* Buffer, int64_t* Length)
			{
				if (!Buffer || !Length)
					return -1;

				size_t Dest = 0, Src = 0, Size = *Length;
				int64_t Result = -2;

				while (true)
				{
					switch (Chunked.State)
					{
						case ChunkedState_Size:
							for (;; ++Src)
							{
								if (Src == Size)
									goto Exit;

								int V = Buffer[Src];
								if ('0' <= V && V <= '9')
									V = V - '0';
								else if ('A' <= V && V <= 'F')
									V = V - 'A' + 0xa;
								else if ('a' <= V && V <= 'f')
									V = V - 'a' + 0xa;
								else
									V = -1;

								if (V == -1)
								{
									if (Chunked.HexCount == 0)
									{
										Result = -1;
										goto Exit;
									}
									break;
								}

								if (Chunked.HexCount == sizeof(size_t) * 2)
								{
									Result = -1;
									goto Exit;
								}

								Chunked.Length = Chunked.Length * 16 + V;
								++Chunked.HexCount;
							}

							Chunked.HexCount = 0;
							Chunked.State = ChunkedState_Ext;
						case ChunkedState_Ext:
							for (;; ++Src)
							{
								if (Src == Size)
									goto Exit;
								if (Buffer[Src] == '\012')
									break;
							}

							++Src;
							if (Chunked.Length == 0)
							{
								if (Chunked.ConsumeTrailer)
								{
									Chunked.State = ChunkedState_Head;
									break;
								}
								else
									goto Complete;
							}

							Chunked.State = ChunkedState_Data;
						case ChunkedState_Data:
						{
							size_t avail = Size - Src;
							if (avail < Chunked.Length)
							{
								if (Dest != Src)
									memmove(Buffer + Dest, Buffer + Src, avail);

								Src += avail;
								Dest += avail;
								Chunked.Length -= avail;
								goto Exit;
							}

							if (Dest != Src)
								memmove(Buffer + Dest, Buffer + Src, Chunked.Length);

							Src += Chunked.Length;
							Dest += Chunked.Length;
							Chunked.Length = 0;
							Chunked.State = ChunkedState_End;
						}
						case ChunkedState_End:
							for (;; ++Src)
							{
								if (Src == Size)
									goto Exit;

								if (Buffer[Src] != '\015')
									break;
							}

							if (Buffer[Src] != '\012')
							{
								Result = -1;
								goto Exit;
							}

							++Src;
							Chunked.State = ChunkedState_Size;
							break;
						case ChunkedState_Head:
							for (;; ++Src)
							{
								if (Src == Size)
									goto Exit;

								if (Buffer[Src] != '\015')
									break;
							}

							if (Buffer[Src++] == '\012')
								goto Complete;

							Chunked.State = ChunkedState_Middle;
						case ChunkedState_Middle:
							for (;; ++Src)
							{
								if (Src == Size)
									goto Exit;

								if (Buffer[Src] == '\012')
									break;
							}

							++Src;
							Chunked.State = ChunkedState_Head;
							break;
						default:
							return -1;
					}
				}

			Complete:
				Result = Size - Src;

			Exit:
				if (Dest != Src)
					memmove(Buffer + Dest, Buffer + Src, Size - Src);

				*Length = Dest;
				return Result;
			}
			const char* Parser::Tokenize(const char* Buffer, const char* BufferEnd, const char** Token, uint64_t* TokenLength, int* Out)
			{
				const char* TokenStart = Buffer;
				while (BufferEnd - Buffer >= 8)
				{
					for (int i = 0; i < 8; i++)
					{
						if (!((unsigned char)(*Buffer) - 040u < 0137u))
							goto NonPrintable;
						++Buffer;
					}

					continue;
				NonPrintable:
					if (((unsigned char)*Buffer < '\040' && *Buffer != '\011') || *Buffer == '\177')
						goto FoundControl;
					++Buffer;
				}

				for (;; ++Buffer)
				{
					if (Buffer == BufferEnd)
					{
						*Out = -2;
						return nullptr;
					}

					if (!((unsigned char)(*Buffer) - 040u < 0137u))
					{
						if (((unsigned char)*Buffer < '\040' && *Buffer != '\011') || *Buffer == '\177')
							goto FoundControl;
					}
				}

			FoundControl:
				if (*Buffer == '\015')
				{
					++Buffer;
					if (Buffer == BufferEnd)
					{
						*Out = -2;
						return nullptr;
					}

					if (*Buffer++ != '\012')
					{
						*Out = -1;
						return nullptr;
					}

					*TokenLength = Buffer - 2 - TokenStart;
				}
				else if (*Buffer == '\012')
				{
					*TokenLength = Buffer - TokenStart;
					++Buffer;
				}
				else
				{
					*Out = -1;
					return nullptr;
				}

				*Token = TokenStart;
				return Buffer;
			}
			const char* Parser::Complete(const char* Buffer, const char* BufferEnd, uint64_t LastLength, int* Out)
			{
				int Result = 0;
				Buffer = LastLength < 3 ? Buffer : Buffer + LastLength - 3;

				while (true)
				{
					if (Buffer == BufferEnd)
					{
						*Out = -2;
						return nullptr;
					}

					if (*Buffer == '\015')
					{
						++Buffer;
						if (Buffer == BufferEnd)
						{
							*Out = -2;
							return nullptr;
						}

						if (*Buffer++ != '\012')
						{
							*Out = -1;
							return nullptr;
						}
						++Result;
					}
					else if (*Buffer == '\012')
					{
						++Buffer;
						++Result;
					}
					else
					{
						++Buffer;
						Result = 0;
					}

					if (Result == 2)
						return Buffer;
				}

				return nullptr;
			}
			const char* Parser::ProcessVersion(const char* Buffer, const char* BufferEnd, int* Out)
			{
				if (BufferEnd - Buffer < 9)
				{
					*Out = -2;
					return nullptr;
				}

				const char* Version = Buffer;
				if (*(Buffer++) != 'H')
				{
					*Out = -1;
					return nullptr;
				}

				if (*(Buffer++) != 'T')
				{
					*Out = -1;
					return nullptr;
				}

				if (*(Buffer++) != 'T')
				{
					*Out = -1;
					return nullptr;
				}

				if (*(Buffer++) != 'P')
				{
					*Out = -1;
					return nullptr;
				}

				if (*(Buffer++) != '/')
				{
					*Out = -1;
					return nullptr;
				}

				if (*(Buffer++) != '1')
				{
					*Out = -1;
					return nullptr;
				}

				if (*(Buffer++) != '.')
				{
					*Out = -1;
					return nullptr;
				}

				if (*Buffer < '0' || '9' < *Buffer)
				{
					*Out = -1;
					return nullptr;
				}

				Buffer++;
				if (OnVersion && !OnVersion(this, Version, Buffer - Version))
				{
					*Out = -1;
					return nullptr;
				}

				return Buffer;
			}
			const char* Parser::ProcessHeaders(const char* Buffer, const char* BufferEnd, int* Out)
			{
				static const char* Mapping =
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\1\0\1\1\1\1\1\0\0\1\1\0\1\1\0\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0"
					"\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\1"
					"\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\1\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

				while (true)
				{
					if (Buffer == BufferEnd)
					{
						*Out = -2;
						return nullptr;
					}

					if (*Buffer == '\015')
					{
						++Buffer;
						if (Buffer == BufferEnd)
						{
							*Out = -2;
							return nullptr;
						}

						if (*Buffer++ != '\012')
						{
							*Out = -1;
							return nullptr;
						}

						break;
					}
					else if (*Buffer == '\012')
					{
						++Buffer;
						break;
					}

					if (!(*Buffer == ' ' || *Buffer == '\t'))
					{
						const char* Name = Buffer;
						while (true)
						{
							if (*Buffer == ':')
								break;

							if (!Mapping[(unsigned char)*Buffer])
							{
								*Out = -1;
								return nullptr;
							}

							++Buffer;
							if (Buffer == BufferEnd)
							{
								*Out = -2;
								return nullptr;
							}
						}

						int64_t Length = Buffer - Name;
						if (Length == 0)
						{
							*Out = -1;
							return nullptr;
						}

						if (OnHeaderField && !OnHeaderField(this, Name, Length))
						{
							*Out = -1;
							return nullptr;
						}

						++Buffer;
						for (;; ++Buffer)
						{
							if (Buffer == BufferEnd)
							{
								if (OnHeaderValue && !OnHeaderValue(this, "", 0))
								{
									*Out = -1;
									return nullptr;
								}

								*Out = -2;
								return nullptr;
							}

							if (!(*Buffer == ' ' || *Buffer == '\t'))
								break;
						}
					}
					else if (OnHeaderField && !OnHeaderField(this, "", 0))
					{
						*Out = -1;
						return nullptr;
					}

					const char* Value;
					uint64_t ValueLength;
					if ((Buffer = Tokenize(Buffer, BufferEnd, &Value, &ValueLength, Out)) == nullptr)
					{
						if (OnHeaderValue)
							OnHeaderValue(this, "", 0);

						return nullptr;
					}

					const char* ValueEnd = Value + ValueLength;
					for (; ValueEnd != Value; --ValueEnd)
					{
						const char c = *(ValueEnd - 1);
						if (!(c == ' ' || c == '\t'))
							break;
					}

					if (OnHeaderValue && !OnHeaderValue(this, Value, ValueEnd - Value))
					{
						*Out = -1;
						return nullptr;
					}
				}

				return Buffer;
			}
			const char* Parser::ProcessRequest(const char* Buffer, const char* BufferEnd, int* Out)
			{
				if (Buffer == BufferEnd)
				{
					*Out = -2;
					return nullptr;
				}

				if (*Buffer == '\015')
				{
					++Buffer;
					if (Buffer == BufferEnd)
					{
						*Out = -2;
						return nullptr;
					}

					if (*Buffer++ != '\012')
					{
						*Out = -1;
						return nullptr;
					}
				}
				else if (*Buffer == '\012')
					++Buffer;

				const char* TokenStart = Buffer;
				if (Buffer == BufferEnd)
				{
					*Out = -2;
					return nullptr;
				}

				while (true)
				{
					if (*Buffer == ' ')
						break;

					if (!((unsigned char)(*Buffer) - 040u < 0137u))
					{
						if ((unsigned char)*Buffer < '\040' || *Buffer == '\177')
						{
							*Out = -1;
							return nullptr;
						}
					}

					++Buffer;
					if (Buffer == BufferEnd)
					{
						*Out = -2;
						return nullptr;
					}
				}

				if (Buffer - TokenStart == 0)
					return nullptr;

				if (OnMethodValue && !OnMethodValue(this, TokenStart, Buffer - TokenStart))
				{
					*Out = -1;
					return nullptr;
				}

				do
				{
					++Buffer;
				} while (*Buffer == ' ');

				TokenStart = Buffer;
				if (Buffer == BufferEnd)
				{
					*Out = -2;
					return nullptr;
				}

				while (true)
				{
					if (*Buffer == ' ')
						break;

					if (!((unsigned char)(*Buffer) - 040u < 0137u))
					{
						if ((unsigned char)*Buffer < '\040' || *Buffer == '\177')
						{
							*Out = -1;
							return nullptr;
						}
					}

					++Buffer;
					if (Buffer == BufferEnd)
					{
						*Out = -2;
						return nullptr;
					}
				}

				if (Buffer - TokenStart == 0)
					return nullptr;

				char* Path = (char*)TokenStart;
				int64_t PL = Buffer - TokenStart, QL = 0;
				while (QL < PL && Path[QL] != '?')
					QL++;

				if (QL > 0 && QL < PL)
				{
					QL = PL - QL - 1;
					PL -= QL + 1;
					if (OnQueryValue && !OnQueryValue(this, Path + PL + 1, QL))
					{
						*Out = -1;
						return nullptr;
					}
				}

				if (OnPathValue && !OnPathValue(this, Path, PL))
				{
					*Out = -1;
					return nullptr;
				}

				do
				{
					++Buffer;
				} while (*Buffer == ' ');
				if ((Buffer = ProcessVersion(Buffer, BufferEnd, Out)) == nullptr)
					return nullptr;

				if (*Buffer == '\015')
				{
					++Buffer;
					if (Buffer == BufferEnd)
					{
						*Out = -2;
						return nullptr;
					}

					if (*Buffer++ != '\012')
					{
						*Out = -1;
						return nullptr;
					}
				}
				else if (*Buffer != '\012')
				{
					*Out = -1;
					return nullptr;
				}
				else
					++Buffer;

				return ProcessHeaders(Buffer, BufferEnd, Out);
			}
			const char* Parser::ProcessResponse(const char* Buffer, const char* BufferEnd, int* Out)
			{
				if ((Buffer = ProcessVersion(Buffer, BufferEnd, Out)) == nullptr)
					return nullptr;

				if (*Buffer != ' ')
				{
					*Out = -1;
					return nullptr;
				}

				do
				{
					++Buffer;
				} while (*Buffer == ' ');
				if (BufferEnd - Buffer < 4)
				{
					*Out = -2;
					return nullptr;
				}

				int Result = 0, Status = 0;
				if (*Buffer < '0' || '9' < *Buffer)
				{
					*Out = -1;
					return nullptr;
				}

				*(&Result) = 100 * (*Buffer++ - '0');
				Status = Result;
				if (*Buffer < '0' || '9' < *Buffer)
				{
					*Out = -1;
					return nullptr;
				}

				*(&Result) = 10 * (*Buffer++ - '0');
				Status += Result;
				if (*Buffer < '0' || '9' < *Buffer)
				{
					*Out = -1;
					return nullptr;
				}

				*(&Result) = (*Buffer++ - '0');
				Status += Result;
				if (OnStatusCode && !OnStatusCode(this, (int64_t)Status))
				{
					*Out = -1;
					return nullptr;
				}

				const char* Message;
				uint64_t MessageLength;
				if ((Buffer = Tokenize(Buffer, BufferEnd, &Message, &MessageLength, Out)) == nullptr)
					return nullptr;

				if (MessageLength == 0)
					return ProcessHeaders(Buffer, BufferEnd, Out);

				if (*Message != ' ')
				{
					*Out = -1;
					return nullptr;
				}

				do
				{
					++Message;
					--MessageLength;
				} while (*Message == ' ');
				if (OnStatusMessage && !OnStatusMessage(this, Message, MessageLength))
				{
					*Out = -1;
					return nullptr;
				}

				return ProcessHeaders(Buffer, BufferEnd, Out);
			}

			void Util::ConstructPath(Connection* Base)
			{
				if (!Base || !Base->Route)
					return;

				for (uint64_t i = 0; i < Base->Request.URI.size(); i++)
				{
					if (Base->Request.URI[i] == '%' && i + 1 < Base->Request.URI.size())
					{
						if (Base->Request.URI[i + 1] == 'u')
						{
							int Value = 0;
							if (Compute::Common::HexToDecimal(Base->Request.URI, i + 2, 4, Value))
							{
								char Buffer[4];
								uint64_t LCount = Compute::Common::Utf8(Value, Buffer);
								if (LCount > 0)
									Base->Request.Path.append(Buffer, LCount);

								i += 5;
							}
							else
								Base->Request.Path += Base->Request.URI[i];
						}
						else
						{
							int Value = 0;
							if (Compute::Common::HexToDecimal(Base->Request.URI, i + 1, 2, Value))
							{
								Base->Request.Path += Value;
								i += 2;
							}
							else
								Base->Request.Path += Base->Request.URI[i];
						}
					}
					else if (Base->Request.URI[i] == '+')
						Base->Request.Path += ' ';
					else
						Base->Request.Path += Base->Request.URI[i];
				}

				char* Buffer = (char*)Base->Request.Path.c_str();
				char* Next = Buffer;
				while (Buffer[0] == '.' && Buffer[1] == '.')
					Buffer++;

				while (*Buffer != '\0')
				{
					*Next++ = *Buffer++;
					if (Buffer[-1] != '/' && Buffer[-1] != '\\')
						continue;

					while (Buffer[0] != '\0')
					{
						if (Buffer[0] == '/' || Buffer[0] == '\\')
							Buffer++;
						else if (Buffer[0] == '.' && Buffer[1] == '.')
							Buffer += 2;
						else
							break;
					}
				}

				*Next = '\0';
				if (!Base->Request.Match.Empty())
				{
					auto& Match = Base->Request.Match.Get()[0];
					Base->Request.Path = Base->Route->DocumentRoot + Core::Parser(Base->Request.Path).RemovePart(Match.Start, Match.Length).R();
				}
				else
					Base->Request.Path = Base->Route->DocumentRoot + Base->Request.Path;

				Base->Request.Path = Core::OS::Path::Resolve(Base->Request.Path.c_str());
				if (Core::Parser(&Base->Request.Path).EndsOf("/\\"))
				{
					if (!Core::Parser(&Base->Request.URI).EndsOf("/\\"))
						Base->Request.Path.erase(Base->Request.Path.size() - 1, 1);
				}
				else if (Core::Parser(&Base->Request.URI).EndsOf("/\\"))
					Base->Request.Path.append(1, '/');

				if (Base->Route->Site->Callbacks.OnRewriteURL)
					Base->Route->Site->Callbacks.OnRewriteURL(Base);
			}
			void Util::ConstructHeadFull(RequestFrame* Request, ResponseFrame* Response, bool IsRequest, Core::Parser* Buffer)
			{
				if (!Request || !Response || !Buffer)
					return;

				std::vector<Header>* Headers = (IsRequest ? &Request->Headers : &Response->Headers);
				for (auto& Item : *Headers)
					Buffer->fAppend("%s: %s\r\n", Item.Key.c_str(), Item.Value.c_str());

				if (IsRequest)
					return;

				for (auto& Item : Response->Cookies)
				{
					if (!Item.Domain.empty())
						Item.Domain.insert(0, "; domain=");

					if (!Item.Path.empty())
						Item.Path.insert(0, "; path=");

					Buffer->fAppend("Set-Cookie: %s=%s; expires=%s%s%s%s%s\r\n", Item.Name.c_str(), Item.Value.c_str(), Core::DateTime::GetGMTBasedString(Item.Expires).c_str(), Item.Path.c_str(), Item.Domain.c_str(), Item.Secure ? "; secure" : "", Item.HTTPOnly ? "; HTTPOnly" : "");
				}
			}
			void Util::ConstructHeadCache(Connection* Base, Core::Parser* Buffer)
			{
				if (!Base || !Base->Route || !Buffer)
					return;

				if (!Base->Route->StaticFileMaxAge)
					return ConstructHeadUncache(Base, Buffer);

				Buffer->fAppend("Cache-Control: max-age=%llu\r\n", Base->Route->StaticFileMaxAge);
			}
			void Util::ConstructHeadUncache(Connection* Base, Core::Parser* Buffer)
			{
				if (!Base || !Buffer)
					return;

				Buffer->Append("Cache-Control: no-cache, no-store, must-revalidate, private, max-age=0\r\n"
					"Pragma: no-cache\r\n"
					"Expires: 0\r\n", 102);
			}
			bool Util::ConstructRoute(MapRouter* Router, Connection* Base)
			{
				if (!Router || !Base || Router->Sites.empty())
					return false;

				std::string* Host = nullptr;
				for (auto& Header : Base->Request.Headers)
				{
					if (Core::Parser::CaseCompare(Header.Key.c_str(), "Host"))
						continue;

					Host = &Header.Value;
					break;
				}

				if (!Host)
					return false;

				Core::Parser(Host).ToLower();
				for (auto* Entry : Router->Sites)
				{
					if (Entry->SiteName != "*" && Entry->SiteName.find(*Host) == std::string::npos)
						continue;

					auto Hostname = Entry->Hosts.find(Base->Host->Name);
					if (Hostname == Entry->Hosts.end())
						return false;

					for (auto* Basis : Entry->Routes)
					{
						if (Compute::Regex::Match(&Basis->URI, Base->Request.Match, Base->Request.URI))
						{
							Base->Route = Basis;
							return true;
						}
					}

					Base->Route = Entry->Base;
					return true;
				}

				return false;
			}
			bool Util::WebSocketWrite(Connection* Base, const char* Buffer, int64_t Size, WebSocketOp Opcode, const SuccessCallback& Callback)
			{
				return WebSocketWriteMask(Base, Buffer, Size, Opcode, 0, Callback);
			}
			bool Util::WebSocketWriteMask(Connection* Base, const char* Buffer, int64_t Size, WebSocketOp Opcode, unsigned int Mask, const SuccessCallback& Callback)
			{
				if (!Base || !Buffer)
					return false;

				unsigned char Header[14];
				size_t HeaderLength = 1;
				Header[0] = 0x80 + ((size_t)Opcode & 0xF);

				if (Size < 126)
				{
					Header[1] = (unsigned char)Size;
					HeaderLength = 2;
				}
				else if (Size <= 65535)
				{
					uint16_t Length = htons((uint16_t)Size);
					Header[1] = 126;
					HeaderLength = 4;
					memcpy(Header + 2, &Length, 2);
				}
				else
				{
					uint32_t Length1 = htonl((uint64_t)Size >> 32);
					uint32_t Length2 = htonl(Size & 0xFFFFFFFF);
					Header[1] = 127;
					HeaderLength = 10;
					memcpy(Header + 2, &Length1, 4);
					memcpy(Header + 6, &Length2, 4);
				}

				if (Mask)
				{
					Header[1] |= 0x80;
					memcpy(Header + HeaderLength, &Mask, 4);
					HeaderLength += 4;
				}

				return !Base->Stream->WriteAsync((const char*)Header, HeaderLength, [Buffer, Size, Callback](Socket* Socket, int64_t Length)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Length < 0)
					{
						if (Length != -2)
							Base->WebSocket->Error = true;

						return Base->Break();
					}
					else if (Length > 0)
						return true;

					if (Size <= 0)
					{
						if (Callback)
							Callback(Base);

						return true;
					}

					return !Base->Stream->WriteAsync(Buffer, Size, [Callback](Network::Socket* Socket, int64_t Size)
					{
						auto Base = Socket->Context<HTTP::Connection>();
						if (Size < 0)
						{
							if (Size != -2)
								Base->WebSocket->Error = true;

							return Base->Break();
						}
						else if (Size > 0)
							return true;

						if (Callback)
							Callback(Base);

						return true;
					});
				});
			}
			bool Util::ConstructDirectoryEntries(const Core::ResourceEntry& A, const Core::ResourceEntry& B)
			{
				if (A.Source.IsDirectory && !B.Source.IsDirectory)
					return true;

				if (!A.Source.IsDirectory && B.Source.IsDirectory)
					return false;

				auto Base = (HTTP::Connection*)A.UserData;
				if (!Base)
					return false;

				const char* Query = (Base->Request.Query.empty() ? nullptr : Base->Request.Query.c_str());
				if (Query != nullptr)
				{
					int Result = 0;
					if (*Query == 'n')
						Result = strcmp(A.Path.c_str(), B.Path.c_str());
					else if (*Query == 's')
						Result = (A.Source.Size == B.Source.Size) ? 0 : ((A.Source.Size > B.Source.Size) ? 1 : -1);
					else if (*Query == 'd')
						Result = (A.Source.LastModified == B.Source.LastModified) ? 0 : ((A.Source.LastModified > B.Source.LastModified) ? 1 : -1);

					if (Query[1] == 'a')
						return Result < 0;
					else if (Query[1] == 'd')
						return Result > 0;

					return Result < 0;
				}

				return strcmp(A.Path.c_str(), B.Path.c_str()) < 0;
			}
			std::string Util::ConnectionResolve(Connection* Base)
			{
				if (!Base)
					return "";

				if (Base->Info.KeepAlive <= 0)
					return "Connection: Close\r\n";

				if (Base->Response.StatusCode == 401)
					return "Connection: Close\r\n";

				if (Base->Root->Router->KeepAliveMaxCount == 0)
					return "Connection: Close\r\n";

				const char* Connection = Base->Request.GetHeader("Connection");
				if (Connection != nullptr && Core::Parser::CaseCompare(Connection, "keep-alive"))
					return "Connection: Close\r\n";

				if (!Connection && strcmp(Base->Request.Version, "1.1") != 0)
					return "Connection: Close\r\n";

				if (Base->Root->Router->KeepAliveMaxCount < 0)
					return "Connection: Keep-Alive\r\nKeep-Alive: timeout=" + std::to_string(Base->Root->Router->SocketTimeout / 1000) + "\r\n";

				return "Connection: Keep-Alive\r\nKeep-Alive: timeout=" + std::to_string(Base->Root->Router->SocketTimeout / 1000) + ", max=" + std::to_string(Base->Root->Router->KeepAliveMaxCount) + "\r\n";
			}
			std::string Util::ConstructContentRange(uint64_t Offset, uint64_t Length, uint64_t ContentLength)
			{
				std::string Field = "bytes ";
				Field += std::to_string(Offset);
				Field += '-';
				Field += std::to_string(Offset + Length - 1);
				Field += '/';
				Field += std::to_string(ContentLength);

				return Field;
			}
			const char* Util::ContentType(const std::string& Path, std::vector<MimeType>* Types)
			{
				static MimeStatic MimeTypes[] = { MimeStatic(".3dm", "x-world/x-3dmf"), MimeStatic(".3dmf", "x-world/x-3dmf"), MimeStatic(".a", "application/octet-stream"), MimeStatic(".aab", "application/x-authorware-bin"), MimeStatic(".aac", "audio/aac"), MimeStatic(".aam", "application/x-authorware-map"), MimeStatic(".aas", "application/x-authorware-seg"), MimeStatic(".aat", "application/font-sfnt"), MimeStatic(".abc", "text/vnd.abc"), MimeStatic(".acgi", "text/html"), MimeStatic(".afl", "video/animaflex"), MimeStatic(".ai", "application/postscript"), MimeStatic(".aif", "audio/x-aiff"), MimeStatic(".aifc", "audio/x-aiff"), MimeStatic(".aiff", "audio/x-aiff"), MimeStatic(".aim", "application/x-aim"), MimeStatic(".aip", "text/x-audiosoft-intra"), MimeStatic(".ani", "application/x-navi-animation"), MimeStatic(".aos", "application/x-nokia-9000-communicator-add-on-software"), MimeStatic(".aps", "application/mime"), MimeStatic(".arc", "application/octet-stream"), MimeStatic(".arj", "application/arj"), MimeStatic(".art", "image/x-jg"), MimeStatic(".asf", "video/x-ms-asf"), MimeStatic(".asm", "text/x-asm"), MimeStatic(".asp", "text/asp"), MimeStatic(".asx", "video/x-ms-asf"), MimeStatic(".au", "audio/x-au"), MimeStatic(".avi", "video/x-msvideo"), MimeStatic(".avs", "video/avs-video"), MimeStatic(".bcpio", "application/x-bcpio"), MimeStatic(".bin", "application/x-binary"), MimeStatic(".bm", "image/bmp"), MimeStatic(".bmp", "image/bmp"), MimeStatic(".boo", "application/book"), MimeStatic(".book", "application/book"), MimeStatic(".boz", "application/x-bzip2"), MimeStatic(".bsh", "application/x-bsh"), MimeStatic(".bz", "application/x-bzip"), MimeStatic(".bz2", "application/x-bzip2"), MimeStatic(".c", "text/x-c"), MimeStatic(".c++", "text/x-c"), MimeStatic(".cat", "application/vnd.ms-pki.seccat"), MimeStatic(".cc", "text/x-c"), MimeStatic(".ccad", "application/clariscad"), MimeStatic(".cco", "application/x-cocoa"), MimeStatic(".cdf", "application/x-cdf"), MimeStatic(".cer", "application/pkix-cert"), MimeStatic(".cff", "application/font-sfnt"), MimeStatic(".cha", "application/x-chat"), MimeStatic(".chat", "application/x-chat"), MimeStatic(".class", "application/x-java-class"), MimeStatic(".com", "application/octet-stream"), MimeStatic(".conf", "text/plain"), MimeStatic(".cpio", "application/x-cpio"), MimeStatic(".cpp", "text/x-c"), MimeStatic(".cpt", "application/x-compactpro"), MimeStatic(".crl", "application/pkcs-crl"), MimeStatic(".crt", "application/x-x509-user-cert"), MimeStatic(".csh", "text/x-script.csh"), MimeStatic(".css", "text/css"), MimeStatic(".csv", "text/csv"), MimeStatic(".cxx", "text/plain"), MimeStatic(".dcr", "application/x-director"), MimeStatic(".deepv", "application/x-deepv"), MimeStatic(".def", "text/plain"), MimeStatic(".der", "application/x-x509-ca-cert"), MimeStatic(".dif", "video/x-dv"), MimeStatic(".dir", "application/x-director"), MimeStatic(".dl", "video/x-dl"), MimeStatic(".dll", "application/octet-stream"), MimeStatic(".doc", "application/msword"), MimeStatic(".dot", "application/msword"), MimeStatic(".dp", "application/commonground"), MimeStatic(".drw", "application/drafting"), MimeStatic(".dump", "application/octet-stream"), MimeStatic(".dv", "video/x-dv"), MimeStatic(".dvi", "application/x-dvi"), MimeStatic(".dwf", "model/vnd.dwf"), MimeStatic(".dwg", "image/vnd.dwg"), MimeStatic(".dxf", "image/vnd.dwg"), MimeStatic(".dxr", "application/x-director"), MimeStatic(".el", "text/x-script.elisp"), MimeStatic(".elc", "application/x-bytecode.elisp"), MimeStatic(".env", "application/x-envoy"), MimeStatic(".eps", "application/postscript"), MimeStatic(".es", "application/x-esrehber"), MimeStatic(".etx", "text/x-setext"), MimeStatic(".evy", "application/x-envoy"), MimeStatic(".exe", "application/octet-stream"), MimeStatic(".f", "text/x-fortran"), MimeStatic(".f77", "text/x-fortran"), MimeStatic(".f90", "text/x-fortran"), MimeStatic(".fdf", "application/vnd.fdf"), MimeStatic(".fif", "image/fif"), MimeStatic(".fli", "video/x-fli"), MimeStatic(".flo", "image/florian"), MimeStatic(".flx", "text/vnd.fmi.flexstor"), MimeStatic(".fmf", "video/x-atomic3d-feature"), MimeStatic(".for", "text/x-fortran"), MimeStatic(".fpx", "image/vnd.fpx"), MimeStatic(".frl", "application/freeloader"), MimeStatic(".funk", "audio/make"), MimeStatic(".g", "text/plain"), MimeStatic(".g3", "image/g3fax"), MimeStatic(".gif", "image/gif"), MimeStatic(".gl", "video/x-gl"), MimeStatic(".gsd", "audio/x-gsm"), MimeStatic(".gsm", "audio/x-gsm"), MimeStatic(".gsp", "application/x-gsp"), MimeStatic(".gss", "application/x-gss"), MimeStatic(".gtar", "application/x-gtar"), MimeStatic(".gz", "application/x-gzip"), MimeStatic(".h", "text/x-h"), MimeStatic(".hdf", "application/x-hdf"), MimeStatic(".help", "application/x-helpfile"), MimeStatic(".hgl", "application/vnd.hp-hpgl"), MimeStatic(".hh", "text/x-h"), MimeStatic(".hlb", "text/x-script"), MimeStatic(".hlp", "application/x-helpfile"), MimeStatic(".hpg", "application/vnd.hp-hpgl"), MimeStatic(".hpgl", "application/vnd.hp-hpgl"), MimeStatic(".hqx", "application/binhex"), MimeStatic(".hta", "application/hta"), MimeStatic(".htc", "text/x-component"), MimeStatic(".htm", "text/html"), MimeStatic(".html", "text/html"), MimeStatic(".htmls", "text/html"), MimeStatic(".htt", "text/webviewhtml"), MimeStatic(".htx", "text/html"), MimeStatic(".ice", "x-conference/x-cooltalk"), MimeStatic(".ico", "image/x-icon"), MimeStatic(".idc", "text/plain"), MimeStatic(".ief", "image/ief"), MimeStatic(".iefs", "image/ief"), MimeStatic(".iges", "model/iges"), MimeStatic(".igs", "model/iges"), MimeStatic(".ima", "application/x-ima"), MimeStatic(".imap", "application/x-httpd-imap"), MimeStatic(".inf", "application/inf"), MimeStatic(".ins", "application/x-internett-signup"), MimeStatic(".ip", "application/x-ip2"), MimeStatic(".isu", "video/x-isvideo"), MimeStatic(".it", "audio/it"), MimeStatic(".iv", "application/x-inventor"), MimeStatic(".ivr", "i-world/i-vrml"), MimeStatic(".ivy", "application/x-livescreen"), MimeStatic(".jam", "audio/x-jam"), MimeStatic(".jav", "text/x-java-source"), MimeStatic(".java", "text/x-java-source"), MimeStatic(".jcm", "application/x-java-commerce"), MimeStatic(".jfif", "image/jpeg"), MimeStatic(".jfif-tbnl", "image/jpeg"), MimeStatic(".jpe", "image/jpeg"), MimeStatic(".jpeg", "image/jpeg"), MimeStatic(".jpg", "image/jpeg"), MimeStatic(".jpm", "image/jpm"), MimeStatic(".jps", "image/x-jps"), MimeStatic(".jpx", "image/jpx"), MimeStatic(".js", "application/x-javascript"), MimeStatic(".json", "application/json"), MimeStatic(".jut", "image/jutvision"), MimeStatic(".kar", "music/x-karaoke"), MimeStatic(".kml", "application/vnd.google-earth.kml+xml"), MimeStatic(".kmz", "application/vnd.google-earth.kmz"), MimeStatic(".ksh", "text/x-script.ksh"), MimeStatic(".la", "audio/x-nspaudio"), MimeStatic(".lam", "audio/x-liveaudio"), MimeStatic(".latex", "application/x-latex"), MimeStatic(".lha", "application/x-lha"), MimeStatic(".lhx", "application/octet-stream"), MimeStatic(".lib", "application/octet-stream"), MimeStatic(".list", "text/plain"), MimeStatic(".lma", "audio/x-nspaudio"), MimeStatic(".log", "text/plain"), MimeStatic(".lsp", "text/x-script.lisp"), MimeStatic(".lst", "text/plain"), MimeStatic(".lsx", "text/x-la-asf"), MimeStatic(".ltx", "application/x-latex"), MimeStatic(".lzh", "application/x-lzh"), MimeStatic(".lzx", "application/x-lzx"), MimeStatic(".m", "text/x-m"), MimeStatic(".m1v", "video/mpeg"), MimeStatic(".m2a", "audio/mpeg"), MimeStatic(".m2v", "video/mpeg"), MimeStatic(".m3u", "audio/x-mpegurl"), MimeStatic(".m4v", "video/x-m4v"), MimeStatic(".man", "application/x-troff-man"), MimeStatic(".map", "application/x-navimap"), MimeStatic(".mar", "text/plain"), MimeStatic(".mbd", "application/mbedlet"), MimeStatic(".mc$", "application/x-magic-cap-package-1.0"), MimeStatic(".mcd", "application/x-mathcad"), MimeStatic(".mcf", "text/mcf"), MimeStatic(".mcp", "application/netmc"), MimeStatic(".me", "application/x-troff-me"), MimeStatic(".mht", "message/rfc822"), MimeStatic(".mhtml", "message/rfc822"), MimeStatic(".mid", "audio/x-midi"), MimeStatic(".midi", "audio/x-midi"), MimeStatic(".mif", "application/x-mif"), MimeStatic(".mime", "www/mime"), MimeStatic(".mjf", "audio/x-vnd.audioexplosion.mjuicemediafile"), MimeStatic(".mjpg", "video/x-motion-jpeg"), MimeStatic(".mm", "application/base64"), MimeStatic(".mme", "application/base64"), MimeStatic(".mod", "audio/x-mod"), MimeStatic(".moov", "video/quicktime"), MimeStatic(".mov", "video/quicktime"), MimeStatic(".movie", "video/x-sgi-movie"), MimeStatic(".mp2", "video/x-mpeg"), MimeStatic(".mp3", "audio/x-mpeg-3"), MimeStatic(".mp4", "video/mp4"), MimeStatic(".mpa", "audio/mpeg"), MimeStatic(".mpc", "application/x-project"), MimeStatic(".mpeg", "video/mpeg"), MimeStatic(".mpg", "video/mpeg"), MimeStatic(".mpga", "audio/mpeg"), MimeStatic(".mpp", "application/vnd.ms-project"), MimeStatic(".mpt", "application/x-project"), MimeStatic(".mpv", "application/x-project"), MimeStatic(".mpx", "application/x-project"), MimeStatic(".mrc", "application/marc"), MimeStatic(".ms", "application/x-troff-ms"), MimeStatic(".mv", "video/x-sgi-movie"), MimeStatic(".my", "audio/make"), MimeStatic(".mzz", "application/x-vnd.audioexplosion.mzz"), MimeStatic(".nap", "image/naplps"), MimeStatic(".naplps", "image/naplps"), MimeStatic(".nc", "application/x-netcdf"), MimeStatic(".ncm", "application/vnd.nokia.configuration-message"), MimeStatic(".nif", "image/x-niff"), MimeStatic(".niff", "image/x-niff"), MimeStatic(".nix", "application/x-mix-transfer"), MimeStatic(".nsc", "application/x-conference"), MimeStatic(".nvd", "application/x-navidoc"), MimeStatic(".o", "application/octet-stream"), MimeStatic(".obj", "application/octet-stream"), MimeStatic(".oda", "application/oda"), MimeStatic(".oga", "audio/ogg"), MimeStatic(".ogg", "audio/ogg"), MimeStatic(".ogv", "video/ogg"), MimeStatic(".omc", "application/x-omc"), MimeStatic(".omcd", "application/x-omcdatamaker"), MimeStatic(".omcr", "application/x-omcregerator"), MimeStatic(".otf", "application/font-sfnt"), MimeStatic(".p", "text/x-pascal"), MimeStatic(".p10", "application/x-pkcs10"), MimeStatic(".p12", "application/x-pkcs12"), MimeStatic(".p7a", "application/x-pkcs7-signature"), MimeStatic(".p7c", "application/x-pkcs7-mime"), MimeStatic(".p7m", "application/x-pkcs7-mime"), MimeStatic(".p7r", "application/x-pkcs7-certreqresp"), MimeStatic(".p7s", "application/pkcs7-signature"), MimeStatic(".part", "application/pro_eng"), MimeStatic(".pas", "text/x-pascal"), MimeStatic(".pbm", "image/x-portable-bitmap"), MimeStatic(".pcl", "application/vnd.hp-pcl"), MimeStatic(".pct", "image/x-pct"), MimeStatic(".pcx", "image/x-pcx"), MimeStatic(".pdb", "chemical/x-pdb"), MimeStatic(".pdf", "application/pdf"), MimeStatic(".pfr", "application/font-tdpfr"), MimeStatic(".pfunk", "audio/make"), MimeStatic(".pgm", "image/x-portable-greymap"), MimeStatic(".pic", "image/pict"), MimeStatic(".pict", "image/pict"), MimeStatic(".pkg", "application/x-newton-compatible-pkg"), MimeStatic(".pko", "application/vnd.ms-pki.pko"), MimeStatic(".pl", "text/x-script.perl"), MimeStatic(".plx", "application/x-pixelscript"), MimeStatic(".pm", "text/x-script.perl-module"), MimeStatic(".pm4", "application/x-pagemaker"), MimeStatic(".pm5", "application/x-pagemaker"), MimeStatic(".png", "image/png"), MimeStatic(".pnm", "image/x-portable-anymap"), MimeStatic(".pot", "application/vnd.ms-powerpoint"), MimeStatic(".pov", "model/x-pov"), MimeStatic(".ppa", "application/vnd.ms-powerpoint"), MimeStatic(".ppm", "image/x-portable-pixmap"), MimeStatic(".pps", "application/vnd.ms-powerpoint"), MimeStatic(".ppt", "application/vnd.ms-powerpoint"), MimeStatic(".ppz", "application/vnd.ms-powerpoint"), MimeStatic(".pre", "application/x-freelance"), MimeStatic(".prt", "application/pro_eng"), MimeStatic(".ps", "application/postscript"), MimeStatic(".psd", "application/octet-stream"), MimeStatic(".pvu", "paleovu/x-pv"), MimeStatic(".pwz", "application/vnd.ms-powerpoint"), MimeStatic(".py", "text/x-script.python"), MimeStatic(".pyc", "application/x-bytecode.python"), MimeStatic(".qcp", "audio/vnd.qcelp"), MimeStatic(".qd3", "x-world/x-3dmf"), MimeStatic(".qd3d", "x-world/x-3dmf"), MimeStatic(".qif", "image/x-quicktime"), MimeStatic(".qt", "video/quicktime"), MimeStatic(".qtc", "video/x-qtc"), MimeStatic(".qti", "image/x-quicktime"), MimeStatic(".qtif", "image/x-quicktime"), MimeStatic(".ra", "audio/x-pn-realaudio"), MimeStatic(".ram", "audio/x-pn-realaudio"), MimeStatic(".rar", "application/x-arj-compressed"), MimeStatic(".ras", "image/x-cmu-raster"), MimeStatic(".rast", "image/cmu-raster"), MimeStatic(".rexx", "text/x-script.rexx"), MimeStatic(".rf", "image/vnd.rn-realflash"), MimeStatic(".rgb", "image/x-rgb"), MimeStatic(".rm", "audio/x-pn-realaudio"), MimeStatic(".rmi", "audio/mid"), MimeStatic(".rmm", "audio/x-pn-realaudio"), MimeStatic(".rmp", "audio/x-pn-realaudio"), MimeStatic(".rng", "application/vnd.nokia.ringing-tone"), MimeStatic(".rnx", "application/vnd.rn-realplayer"), MimeStatic(".roff", "application/x-troff"), MimeStatic(".rp", "image/vnd.rn-realpix"), MimeStatic(".rpm", "audio/x-pn-realaudio-plugin"), MimeStatic(".rt", "text/vnd.rn-realtext"), MimeStatic(".rtf", "application/x-rtf"), MimeStatic(".rtx", "application/x-rtf"), MimeStatic(".rv", "video/vnd.rn-realvideo"), MimeStatic(".s", "text/x-asm"), MimeStatic(".s3m", "audio/s3m"), MimeStatic(".saveme", "application/octet-stream"), MimeStatic(".sbk", "application/x-tbook"), MimeStatic(".scm", "text/x-script.scheme"), MimeStatic(".sdml", "text/plain"), MimeStatic(".sdp", "application/x-sdp"), MimeStatic(".sdr", "application/sounder"), MimeStatic(".sea", "application/x-sea"), MimeStatic(".set", "application/set"), MimeStatic(".sgm", "text/x-sgml"), MimeStatic(".sgml", "text/x-sgml"), MimeStatic(".sh", "text/x-script.sh"), MimeStatic(".shar", "application/x-shar"), MimeStatic(".shtm", "text/html"), MimeStatic(".shtml", "text/html"), MimeStatic(".sid", "audio/x-psid"), MimeStatic(".sil", "application/font-sfnt"), MimeStatic(".sit", "application/x-sit"), MimeStatic(".skd", "application/x-koan"), MimeStatic(".skm", "application/x-koan"), MimeStatic(".skp", "application/x-koan"), MimeStatic(".skt", "application/x-koan"), MimeStatic(".sl", "application/x-seelogo"), MimeStatic(".smi", "application/smil"), MimeStatic(".smil", "application/smil"), MimeStatic(".snd", "audio/x-adpcm"), MimeStatic(".so", "application/octet-stream"), MimeStatic(".sol", "application/solids"), MimeStatic(".spc", "text/x-speech"), MimeStatic(".spl", "application/futuresplash"), MimeStatic(".spr", "application/x-sprite"), MimeStatic(".sprite", "application/x-sprite"), MimeStatic(".src", "application/x-wais-source"), MimeStatic(".ssi", "text/x-server-parsed-html"), MimeStatic(".ssm", "application/streamingmedia"), MimeStatic(".sst", "application/vnd.ms-pki.certstore"), MimeStatic(".step", "application/step"), MimeStatic(".stl", "application/vnd.ms-pki.stl"), MimeStatic(".stp", "application/step"), MimeStatic(".sv4cpio", "application/x-sv4cpio"), MimeStatic(".sv4crc", "application/x-sv4crc"), MimeStatic(".svf", "image/x-dwg"), MimeStatic(".svg", "image/svg+xml"), MimeStatic(".svr", "x-world/x-svr"), MimeStatic(".swf", "application/x-shockwave-flash"), MimeStatic(".t", "application/x-troff"), MimeStatic(".talk", "text/x-speech"), MimeStatic(".tar", "application/x-tar"), MimeStatic(".tbk", "application/x-tbook"), MimeStatic(".tcl", "text/x-script.tcl"), MimeStatic(".tcsh", "text/x-script.tcsh"), MimeStatic(".tex", "application/x-tex"), MimeStatic(".texi", "application/x-texinfo"), MimeStatic(".texinfo", "application/x-texinfo"), MimeStatic(".text", "text/plain"), MimeStatic(".tgz", "application/x-compressed"), MimeStatic(".tif", "image/x-tiff"), MimeStatic(".tiff", "image/x-tiff"), MimeStatic(".torrent", "application/x-bittorrent"), MimeStatic(".tr", "application/x-troff"), MimeStatic(".tsi", "audio/tsp-audio"), MimeStatic(".tsp", "audio/tsplayer"), MimeStatic(".tsv", "text/tab-separated-values"), MimeStatic(".ttf", "application/font-sfnt"), MimeStatic(".turbot", "image/florian"), MimeStatic(".txt", "text/plain"), MimeStatic(".uil", "text/x-uil"), MimeStatic(".uni", "text/uri-list"), MimeStatic(".unis", "text/uri-list"), MimeStatic(".unv", "application/i-deas"), MimeStatic(".uri", "text/uri-list"), MimeStatic(".uris", "text/uri-list"), MimeStatic(".ustar", "application/x-ustar"), MimeStatic(".uu", "text/x-uuencode"), MimeStatic(".uue", "text/x-uuencode"), MimeStatic(".vcd", "application/x-cdlink"), MimeStatic(".vcs", "text/x-vcalendar"), MimeStatic(".vda", "application/vda"), MimeStatic(".vdo", "video/vdo"), MimeStatic(".vew", "application/groupwise"), MimeStatic(".viv", "video/vnd.vivo"), MimeStatic(".vivo", "video/vnd.vivo"), MimeStatic(".vmd", "application/vocaltec-media-desc"), MimeStatic(".vmf", "application/vocaltec-media-resource"), MimeStatic(".voc", "audio/x-voc"), MimeStatic(".vos", "video/vosaic"), MimeStatic(".vox", "audio/voxware"), MimeStatic(".vqe", "audio/x-twinvq-plugin"), MimeStatic(".vqf", "audio/x-twinvq"), MimeStatic(".vql", "audio/x-twinvq-plugin"), MimeStatic(".vrml", "model/vrml"), MimeStatic(".vrt", "x-world/x-vrt"), MimeStatic(".vsd", "application/x-visio"), MimeStatic(".vst", "application/x-visio"), MimeStatic(".vsw", "application/x-visio"), MimeStatic(".w60", "application/wordperfect6.0"), MimeStatic(".w61", "application/wordperfect6.1"), MimeStatic(".w6w", "application/msword"), MimeStatic(".wav", "audio/x-wav"), MimeStatic(".wb1", "application/x-qpro"), MimeStatic(".wbmp", "image/vnd.wap.wbmp"), MimeStatic(".web", "application/vnd.xara"), MimeStatic(".webm", "video/webm"), MimeStatic(".wiz", "application/msword"), MimeStatic(".wk1", "application/x-123"), MimeStatic(".wmf", "windows/metafile"), MimeStatic(".wml", "text/vnd.wap.wml"), MimeStatic(".wmlc", "application/vnd.wap.wmlc"), MimeStatic(".wmls", "text/vnd.wap.wmlscript"), MimeStatic(".wmlsc", "application/vnd.wap.wmlscriptc"), MimeStatic(".woff", "application/font-woff"), MimeStatic(".word", "application/msword"), MimeStatic(".wp", "application/wordperfect"), MimeStatic(".wp5", "application/wordperfect"), MimeStatic(".wp6", "application/wordperfect"), MimeStatic(".wpd", "application/wordperfect"), MimeStatic(".wq1", "application/x-lotus"), MimeStatic(".wri", "application/x-wri"), MimeStatic(".wrl", "model/vrml"), MimeStatic(".wrz", "model/vrml"), MimeStatic(".wsc", "text/scriplet"), MimeStatic(".wsrc", "application/x-wais-source"), MimeStatic(".wtk", "application/x-wintalk"), MimeStatic(".x-png", "image/png"), MimeStatic(".xbm", "image/x-xbm"), MimeStatic(".xdr", "video/x-amt-demorun"), MimeStatic(".xgz", "xgl/drawing"), MimeStatic(".xhtml", "application/xhtml+xml"), MimeStatic(".xif", "image/vnd.xiff"), MimeStatic(".xl", "application/vnd.ms-excel"), MimeStatic(".xla", "application/vnd.ms-excel"), MimeStatic(".xlb", "application/vnd.ms-excel"), MimeStatic(".xlc", "application/vnd.ms-excel"), MimeStatic(".xld", "application/vnd.ms-excel"), MimeStatic(".xlk", "application/vnd.ms-excel"), MimeStatic(".xll", "application/vnd.ms-excel"), MimeStatic(".xlm", "application/vnd.ms-excel"), MimeStatic(".xls", "application/vnd.ms-excel"), MimeStatic(".xlt", "application/vnd.ms-excel"), MimeStatic(".xlv", "application/vnd.ms-excel"), MimeStatic(".xlw", "application/vnd.ms-excel"), MimeStatic(".xm", "audio/xm"), MimeStatic(".xml", "text/xml"), MimeStatic(".xmz", "xgl/movie"), MimeStatic(".xpix", "application/x-vnd.ls-xpix"), MimeStatic(".xpm", "image/x-xpixmap"), MimeStatic(".xsl", "application/xml"), MimeStatic(".xslt", "application/xml"), MimeStatic(".xsr", "video/x-amt-showrun"), MimeStatic(".xwd", "image/x-xwd"), MimeStatic(".xyz", "chemical/x-pdb"), MimeStatic(".z", "application/x-compressed"), MimeStatic(".zip", "application/x-zip-compressed"), MimeStatic(".zoo", "application/octet-stream"), MimeStatic(".zsh", "text/x-script.zsh") };

				uint64_t PathLength = Path.size();
				while (PathLength >= 1 && Path[PathLength - 1] != '.')
					PathLength--;

				if (!PathLength)
					return "application/octet-stream";

				const char* Ext = &Path.c_str()[PathLength - 1];
				int End = ((int)(sizeof(MimeTypes) / sizeof(MimeTypes[0])));
				int Start = 0, Result, Index;

				while (End - Start > 1)
				{
					Index = (Start + End) >> 1;
					if ((Result = Core::Parser::CaseCompare(Ext, MimeTypes[Index].Extension)) == 0)
						return MimeTypes[Index].Type;

					if (Result < 0)
						End = Index;
					else
						Start = Index;
				}

				if (!Core::Parser::CaseCompare(Ext, MimeTypes[Start].Extension))
					return MimeTypes[Start].Type;

				if (Types != nullptr && !Types->empty())
				{
					for (auto& Item : *Types)
					{
						if (!Core::Parser::CaseCompare(Ext, Item.Extension.c_str()))
							return Item.Type.c_str();
					}
				}

				return "application/octet-stream";
			}
			const char* Util::StatusMessage(int StatusCode)
			{
				switch (StatusCode)
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
						return "IM used";
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
						return "Proxy Authentication Required";
					case 408:
						return "Request Time-out";
					case 409:
						return "Conflict";
					case 410:
						return "Gone";
					case 411:
						return "Length Required";
					case 412:
						return "Precondition Failed";
					case 413:
						return "Request Entity Too Large";
					case 414:
						return "Request-URI Too Large";
					case 415:
						return "Unsupported Media Type";
					case 416:
						return "Requested range not satisfiable";
					case 417:
						return "Expectation Failed";
					case 418:
						return "I am a teapot";
					case 419:
						return "Authentication Timeout";
					case 420:
						return "Enhance Your Calm";
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
						return "Too Many Requests";
					case 431:
						return "Request Header Fields Too Large";
					case 440:
						return "Login Timeout";
					case 451:
						return "Unavailable For Legal Reasons";
					case 500:
						return "Internal Server Error";
					case 501:
						return "Not Implemented";
					case 502:
						return "Bad Gateway";
					case 503:
						return "Service Unavailable";
					case 504:
						return "Gateway Time-out";
					case 505:
						return "Version not supported";
					case 506:
						return "Variant Also Negotiates";
					case 507:
						return "Insufficient Storage";
					case 508:
						return "Loop Detected";
					case 509:
						return "Bandwidth Limit Exceeded";
					case 510:
						return "Not Extended";
					case 511:
						return "Network Authentication Required";
					default:
						if (StatusCode >= 100 && StatusCode < 200)
							return "Information";

						if (StatusCode >= 200 && StatusCode < 300)
							return "Success";

						if (StatusCode >= 300 && StatusCode < 400)
							return "Redirection";

						if (StatusCode >= 400 && StatusCode < 500)
							return "Client Error";

						if (StatusCode >= 500 && StatusCode < 600)
							return "Server Error";
						break;
				}

				return "";
			}
			bool Util::ParseMultipartHeaderField(Parser* Parser, const char* Name, uint64_t Length)
			{
				return ParseHeaderField(Parser, Name, Length);
			}
			bool Util::ParseMultipartHeaderValue(Parser* Parser, const char* Data, uint64_t Length)
			{
				if (!Parser || !Data || !Length)
					return true;

				ParserFrame* Segment = (ParserFrame*)Parser->UserPointer;
				if (!Segment)
					return true;

				if (!Segment->Header.empty())
				{
					std::string Value(Data, Length);
					if (Segment->Header == "Content-Disposition")
					{
						Core::Parser::Settle Start = Core::Parser(&Value).Find("name=\"");
						if (Start.Found)
						{
							Core::Parser::Settle End = Core::Parser(&Value).Find('\"', Start.End);
							if (End.Found)
								Segment->Source.Key = Value.substr(Start.End, End.End - Start.End - 1);
						}

						Start = Core::Parser(&Value).Find("filename=\"");
						if (Start.Found)
						{
							Core::Parser::Settle End = Core::Parser(&Value).Find('\"', Start.End);
							if (End.Found)
								Segment->Source.Name = Value.substr(Start.End, End.End - Start.End - 1);
						}
					}
					else if (Segment->Header == "Content-Type")
						Segment->Source.Type = Value;

					Segment->Source.SetHeader(Segment->Header.c_str(), Value);
					Segment->Header.clear();
				}

				return true;
			}
			bool Util::ParseMultipartContentData(Parser* Parser, const char* Data, uint64_t Length)
			{
				if (!Parser || !Data || !Length)
					return true;

				ParserFrame* Segment = (ParserFrame*)Parser->UserPointer;
				if (!Segment || !Segment->Stream)
					return false;

				if (fwrite(Data, 1, (size_t)Length, Segment->Stream) != (size_t)Length)
					return false;

				Segment->Source.Length += Length;
				return true;
			}
			bool Util::ParseMultipartResourceBegin(Parser* Parser)
			{
				if (!Parser)
					return true;

				ParserFrame* Segment = (ParserFrame*)Parser->UserPointer;
				if (!Segment || !Segment->Request)
					return true;

				if (Segment->Stream != nullptr)
				{
					fclose(Segment->Stream);
					return false;
				}

				if (Segment->Route && Segment->Request->Resources.size() >= Segment->Route->Site->MaxResources)
				{
					Segment->Close = true;
					return false;
				}

				Segment->Header.clear();
				Segment->Source.Headers.clear();
				Segment->Source.Name.clear();
				Segment->Source.Type = "application/octet-stream";
				Segment->Source.Memory = false;
				Segment->Source.Length = 0;

				if (Segment->Route)
					Segment->Source.Path = Segment->Route->Site->ResourceRoot + Compute::Common::MD5Hash(Compute::Common::RandomBytes(16));

				Segment->Stream = (FILE*)Core::OS::File::Open(Segment->Source.Path.c_str(), "wb");
				return Segment->Stream != nullptr;
			}
			bool Util::ParseMultipartResourceEnd(Parser* Parser)
			{
				if (!Parser)
					return true;

				ParserFrame* Segment = (ParserFrame*)Parser->UserPointer;
				if (!Segment || !Segment->Stream || !Segment->Request)
					return false;

				fclose(Segment->Stream);
				Segment->Stream = nullptr;
				Segment->Request->Resources.push_back(Segment->Source);

				if (Segment->Callback)
					Segment->Callback(nullptr, &Segment->Request->Resources.back(), Segment->Source.Length);

				return true;
			}
			bool Util::ParseHeaderField(Parser* Parser, const char* Name, uint64_t Length)
			{
				if (!Parser || !Name || !Length)
					return true;

				ParserFrame* Segment = (ParserFrame*)Parser->UserPointer;
				if (!Segment)
					return true;

				Segment->Header.assign(Name, Length);
				return true;
			}
			bool Util::ParseHeaderValue(Parser* Parser, const char* Data, uint64_t Length)
			{
				if (!Parser || !Data || !Length)
					return true;

				ParserFrame* Segment = (ParserFrame*)Parser->UserPointer;
				if (!Segment || Segment->Header.empty())
					return true;

				Header Value;
				Value.Key = Segment->Header;
				Value.Value.assign(Data, Length);

				if (Segment->Request)
					Segment->Request->Headers.push_back(Value);

				if (Segment->Response)
					Segment->Response->Headers.push_back(Value);

				Segment->Header.clear();
				return true;
			}
			bool Util::ParseVersion(Parser* Parser, const char* Data, uint64_t Length)
			{
				if (!Parser || !Data || !Length)
					return true;

				ParserFrame* Segment = (ParserFrame*)Parser->UserPointer;
				if (!Segment || !Segment->Request)
					return true;

				memcpy((void*)Segment->Request->Version, (void*)Data, (size_t)Length);
				return true;
			}
			bool Util::ParseStatusCode(Parser* Parser, uint64_t Value)
			{
				if (!Parser)
					return true;

				ParserFrame* Segment = (ParserFrame*)Parser->UserPointer;
				if (!Segment || !Segment->Response)
					return true;

				Segment->Response->StatusCode = (int)Value;
				return true;
			}
			bool Util::ParseMethodValue(Parser* Parser, const char* Data, uint64_t Length)
			{
				if (!Parser || !Data || !Length)
					return true;

				ParserFrame* Segment = (ParserFrame*)Parser->UserPointer;
				if (!Segment || !Segment->Request)
					return true;

				memcpy((void*)Segment->Request->Method, (void*)Data, (size_t)Length);
				return true;
			}
			bool Util::ParsePathValue(Parser* Parser, const char* Data, uint64_t Length)
			{
				if (!Parser || !Data || !Length)
					return true;

				ParserFrame* Segment = (ParserFrame*)Parser->UserPointer;
				if (!Segment || !Segment->Request)
					return true;

				Segment->Request->URI.assign(Data, Length);
				return true;
			}
			bool Util::ParseQueryValue(Parser* Parser, const char* Data, uint64_t Length)
			{
				if (!Parser || !Data || !Length)
					return true;

				ParserFrame* Segment = (ParserFrame*)Parser->UserPointer;
				if (!Segment || !Segment->Request)
					return true;

				Segment->Request->Query.assign(Data, Length);
				return true;
			}
			int Util::ParseContentRange(const char* ContentRange, int64_t* Range1, int64_t* Range2)
			{
				return sscanf(ContentRange, "bytes=%lld-%lld", Range1, Range2);
			}
			std::string Util::ParseMultipartDataBoundary()
			{
				static const char Data[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

				std::random_device SeedGenerator;
				std::mt19937 Engine(SeedGenerator());
				std::string Result = "--sha1-digest-multipart-data-";

				for (int i = 0; i < 16; i++)
					Result += Data[Engine() % (sizeof(Data) - 1)];

				return Result;
			}
			bool Util::Authorize(Connection* Base)
			{
				if (!Base || !Base->Route)
					return false;

				if (Base->Route->Auth.Type.empty())
					return true;

				bool IsSupported = false;
				for (auto& Item : Base->Route->Auth.Methods)
				{
					if (Item == Base->Request.Method)
					{
						IsSupported = true;
						break;
					}
				}

				if (!IsSupported && !Base->Route->Auth.Methods.empty())
				{
					Base->Request.User.Type = Auth::Denied;
					Base->Error(401, "Authorization method is not allowed");
					return false;
				}

				const char* Authorization = Base->Request.GetHeader("Authorization");
				if (!Authorization)
				{
					Base->Request.User.Type = Auth::Denied;
					Base->Error(401, "Provide authorization header to continue.");
					return false;
				}

				uint64_t Index = 0;
				while (Authorization[Index] != ' ' && Authorization[Index] != '\0')
					Index++;

				std::string Type = std::string(Authorization, Index);
				std::string Credentials = Compute::Common::Base64Decode(Authorization + Index + 1);
				Index = 0;

				while (Credentials[Index] != ':' && Credentials[Index] != '\0')
					Index++;

				Base->Request.User.Username = std::string(Credentials.c_str(), Index);
				Base->Request.User.Password = std::string(Credentials.c_str() + Index + 1);
				if (Base->Route->Callbacks.Authorize && Base->Route->Callbacks.Authorize(Base, &Base->Request.User, Type))
				{
					Base->Request.User.Type = Auth::Granted;
					return true;
				}

				if (Type != Base->Route->Auth.Type)
				{
					Base->Request.User.Type = Auth::Denied;
					Base->Error(401, "Authorization type \"%s\" is not allowed.", Type.c_str());
					return false;
				}

				for (auto& Item : Base->Route->Auth.Users)
				{
					if (Item.Password != Base->Request.User.Password || Item.Username != Base->Request.User.Username)
						continue;

					Base->Request.User.Type = Auth::Granted;
					return true;
				}

				Base->Request.User.Type = Auth::Denied;
				Base->Error(401, "Invalid user access credentials were provided. Access denied.");
				return false;
			}
			bool Util::MethodAllowed(Connection* Base)
			{
				if (!Base || !Base->Route)
					return false;

				for (auto& Item : Base->Route->DisallowedMethods)
				{
					if (Item == Base->Request.Method)
						return false;
				}

				return true;
			}
			bool Util::WebSocketUpgradeAllowed(Connection* Base)
			{
				if (!Base)
					return false;

				const char* Upgrade = Base->Request.GetHeader("Upgrade");
				if (!Upgrade)
					return false;

				if (Core::Parser::CaseCompare(Upgrade, "websocket") != 0)
					return false;

				const char* Connection = Base->Request.GetHeader("Connection");
				if (!Connection)
					return false;

				if (Core::Parser::CaseCompare(Connection, "upgrade") != 0)
					return false;

				return true;
			}
			bool Util::ResourceHidden(Connection* Base, std::string* Path)
			{
				if (!Base || !Base->Route || Base->Route->HiddenFiles.empty())
					return false;

				const std::string& Value = (Path ? *Path : Base->Request.Path);
				Compute::RegexResult Result;

				for (auto& Item : Base->Route->HiddenFiles)
				{
					if (Compute::Regex::Match(&Item, Result, Value))
						return true;
				}

				return false;
			}
			bool Util::ResourceIndexed(Connection* Base, Core::Resource* Resource)
			{
				if (!Base || !Base->Route || !Resource || Base->Route->IndexFiles.empty())
					return false;

				std::string Path = Base->Request.Path;
				if (!Core::Parser(&Path).EndsOf("/\\"))
				{
#ifdef TH_MICROSOFT
					Path.append(1, '\\');
#else
					Path.append(1, '/');
#endif
				}

				for (auto& Item : Base->Route->IndexFiles)
				{
					if (!Core::OS::File::State(Path + Item, Resource))
						continue;

					Base->Request.Path.assign(Path.append(Item));
					return true;
				}

				return false;
			}
			bool Util::ResourceProvided(Connection* Base, Core::Resource* Resource)
			{
				if (!Base || !Base->Route || !Base->Route->Site->Gateway.Enabled || !Resource)
					return false;

				if (!Base->Route->Gateway.Methods.empty())
				{
					for (auto& Item : Base->Route->Gateway.Methods)
					{
						if (Item == Base->Request.Method)
							return false;
					}
				}

				if (Base->Route->Gateway.Files.empty())
					return false;

				Compute::RegexResult Result;
				for (auto& Item : Base->Route->Gateway.Files)
				{
					if (!Compute::Regex::Match(&Item, Result, Base->Request.Path))
						continue;

					return Resource->Size > 0;
				}

				return false;
			}
			bool Util::ResourceModified(Connection* Base, Core::Resource* Resource)
			{
				const char* CacheControl = Base->Request.GetHeader("Cache-Control");
				if (CacheControl != nullptr && (!Core::Parser::CaseCompare("no-cache", CacheControl) || !Core::Parser::CaseCompare("max-age=0", CacheControl)))
					return true;

				const char* IfNoneMatch = Base->Request.GetHeader("If-None-Match");
				if (IfNoneMatch != nullptr)
				{
					char ETag[64];
					Core::OS::Net::GetETag(ETag, sizeof(ETag), Resource);
					if (!Core::Parser::CaseCompare(ETag, IfNoneMatch))
						return false;
				}

				const char* IfModifiedSince = Base->Request.GetHeader("If-Modified-Since");
				return !(IfModifiedSince != nullptr && Resource->LastModified <= Core::DateTime::ReadGMTBasedString(IfModifiedSince));

			}
			bool Util::ResourceCompressed(Connection* Base, uint64_t Size)
			{
#ifdef TH_HAS_ZLIB
				if (!Base || !Base->Route)
					return false;

				if (!Base->Route->Compression.Enabled || Size < Base->Route->Compression.MinLength)
					return false;

				if (Base->Route->Compression.Files.empty())
					return true;

				Compute::RegexResult Result;
				for (auto& Item : Base->Route->Compression.Files)
				{
					if (Compute::Regex::Match(&Item, Result, Base->Request.Path))
						return true;
				}

				return false;
#else
				return false;
#endif
			}
			bool Util::RouteWEBSOCKET(Connection* Base)
			{
				if (!Base)
					return false;

				if (!Base->Route || !Base->Route->AllowWebSocket)
					return Base->Error(404, "Web Socket protocol is not allowed on this server.");

				const char* WebSocketKey = Base->Request.GetHeader("Sec-WebSocket-Key");
				if (WebSocketKey != nullptr)
					return ProcessWebSocket(Base, WebSocketKey);

				const char* WebSocketKey1 = Base->Request.GetHeader("Sec-WebSocket-Key1");
				if (!WebSocketKey1)
					return Base->Error(400, "Malformed websocket request. Provide first key.");

				const char* WebSocketKey2 = Base->Request.GetHeader("Sec-WebSocket-Key2");
				if (!WebSocketKey2)
					return Base->Error(400, "Malformed websocket request. Provide second key.");

				return Base->Stream->ReadAsync(8, [](Socket* Socket, const char* Buffer, int64_t Size)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Size > 0)
					{
						Base->Request.Buffer.append(Buffer, Size);
						return true;
					}
					else if (Size < 0)
						return Base->Break();

					return Util::ProcessWebSocket(Base, Base->Request.Buffer.c_str());
				});
			}
			bool Util::RouteGET(Connection* Base)
			{
				if (!Base)
					return false;

				if (!Base->Route)
					return Base->Error(404, "Requested resource was not found.");

				if (!Core::OS::File::State(Base->Request.Path, &Base->Resource))
				{
					if (Base->Route->Default.empty() || !Core::OS::File::State(Base->Route->Default, &Base->Resource))
					{
						if (WebSocketUpgradeAllowed(Base))
						{
							return Core::Schedule::Get()->SetTask([Base]()
							{
								RouteWEBSOCKET(Base);
							});
						}

						return Base->Error(404, "Requested resource was not found.");
					}

					Base->Request.Path.assign(Base->Route->Default);
				}

				if (WebSocketUpgradeAllowed(Base))
				{
					return Core::Schedule::Get()->SetTask([Base]()
					{
						RouteWEBSOCKET(Base);
					});
				}

				if (ResourceHidden(Base, nullptr))
					return Base->Error(404, "Requested resource was not found.");

				if (Base->Resource.IsDirectory && !ResourceIndexed(Base, &Base->Resource))
				{
					if (Base->Route->AllowDirectoryListing)
					{
						return Core::Schedule::Get()->SetTask([Base]()
						{
							ProcessDirectory(Base);
						});
					}

					return Base->Error(403, "Directory listing denied.");
				}

				if (ResourceProvided(Base, &Base->Resource))
					return ProcessGateway(Base);

				if (Base->Route->StaticFileMaxAge > 0 && !ResourceModified(Base, &Base->Resource))
				{
					return Core::Schedule::Get()->SetTask([Base]()
					{
						ProcessResourceCache(Base);
					});
				}

				if (Base->Resource.Size > Base->Root->Router->PayloadMaxLength)
					return Base->Error(413, "Entity payload is too big to process.");

				return Core::Schedule::Get()->SetTask([Base]()
				{
					ProcessResource(Base);
				});
			}
			bool Util::RoutePOST(Connection* Base)
			{
				if (!Base)
					return false;

				if (!Base->Route)
					return Base->Error(404, "Requested resource was not found.");

				if (!Core::OS::File::State(Base->Request.Path, &Base->Resource))
				{
					if (Base->Route->Default.empty() || !Core::OS::File::State(Base->Route->Default, &Base->Resource))
						return Base->Error(404, "Requested resource was not found.");

					Base->Request.Path.assign(Base->Route->Default);
				}

				if (ResourceHidden(Base, nullptr))
					return Base->Error(404, "Requested resource was not found.");

				if (Base->Resource.IsDirectory && !ResourceIndexed(Base, &Base->Resource))
					return Base->Error(404, "Requested resource was not found.");

				if (ResourceProvided(Base, &Base->Resource))
					return ProcessGateway(Base);

				if (Base->Route->StaticFileMaxAge > 0 && !ResourceModified(Base, &Base->Resource))
				{
					return Core::Schedule::Get()->SetTask([Base]()
					{
						ProcessResourceCache(Base);
					});
				}

				if (Base->Resource.Size > Base->Root->Router->PayloadMaxLength)
					return Base->Error(413, "Entity payload is too big to process.");

				return Core::Schedule::Get()->SetTask([Base]()
				{
					ProcessResource(Base);
				});
			}
			bool Util::RoutePUT(Connection* Base)
			{
				if (!Base)
					return false;

				if (!Base->Route || ResourceHidden(Base, nullptr))
					return Base->Error(403, "Resource overwrite denied.");

				if (!Core::OS::File::State(Base->Request.Path, &Base->Resource))
				{
					if (Base->Route->Default.empty() || !Core::OS::File::State(Base->Route->Default, &Base->Resource))
						return Base->Error(403, "Directory overwrite denied.");

					Base->Request.Path.assign(Base->Route->Default);
				}

				if (ResourceProvided(Base, &Base->Resource))
					return ProcessGateway(Base);

				if (!Base->Resource.IsDirectory)
					return Base->Error(403, "Directory overwrite denied.");

				const char* Range = Base->Request.GetHeader("Range");
				int64_t Range1 = 0, Range2 = 0;

				FILE* Stream = (FILE*)Core::OS::File::Open(Base->Request.Path.c_str(), "wb");
				if (!Stream)
					return Base->Error(422, "Resource stream cannot be opened.");

				if (Range != nullptr && HTTP::Util::ParseContentRange(Range, &Range1, &Range2))
				{
					if (Base->Response.StatusCode <= 0)
						Base->Response.StatusCode = 206;
#ifdef TH_MICROSOFT
					if (_lseeki64(_fileno(Stream), Range1, SEEK_SET) != 0)
						return Base->Error(416, "Invalid content range offset (%lld) was specified.", Range1);
#elif defined(TH_APPLE)
					if (fseek(Stream, Range1, SEEK_SET) != 0)
						return Base->Error(416, "Invalid content range offset (%lld) was specified.", Range1);
#else
					if (lseek64(fileno(Stream), Range1, SEEK_SET) != 0)
						return Base->Error(416, "Invalid content range offset (%lld) was specified.", Range1);
#endif
				}
				else
					Base->Response.StatusCode = 204;

				return Base->Consume([=](Connection* Base, const char* Buffer, int64_t Size)
				{
					if (Size < 0)
					{
						fclose(Stream);
						return Base->Break();
					}
					else if (Size > 0)
					{
						fwrite(Buffer, sizeof(char) * (size_t)Size, 1, Stream);
						return true;
					}

					char Date[64];
					Core::DateTime::TimeFormatGMT(Date, sizeof(Date), Base->Info.Start / 1000);

					Core::Parser Content;
					Content.fAppend("%s 204 No Content\r\nDate: %s\r\n%sContent-Location: %s\r\n", Base->Request.Version, Date, Util::ConnectionResolve(Base).c_str(), Base->Request.URI.c_str());

					fclose(Stream);
					if (Base->Route->Callbacks.Headers)
						Base->Route->Callbacks.Headers(Base, nullptr);

					Content.Append("\r\n", 2);
					return !Base->Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [](Socket* Socket, int64_t Size)
					{
						auto Base = Socket->Context<HTTP::Connection>();
						if (Size < 0)
							return Base->Break();
						else if (Size > 0)
							return true;

						return Base->Finish();
					});
				});
			}
			bool Util::RoutePATCH(Connection* Base)
			{
				if (!Base)
					return false;

				if (!Base->Route)
					return Base->Error(403, "Operation denied by server.");

				if (!Core::OS::File::State(Base->Request.Path, &Base->Resource))
				{
					if (Base->Route->Default.empty() || !Core::OS::File::State(Base->Route->Default, &Base->Resource))
						return Base->Error(404, "Requested resource was not found.");

					Base->Request.Path.assign(Base->Route->Default);
				}

				if (ResourceHidden(Base, nullptr))
					return Base->Error(404, "Requested resource was not found.");

				if (Base->Resource.IsDirectory && !ResourceIndexed(Base, &Base->Resource))
					return Base->Error(404, "Requested resource cannot be directory.");

				if (ResourceProvided(Base, &Base->Resource))
					return ProcessGateway(Base);

				char Date[64];
				Core::DateTime::TimeFormatGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				Core::Parser Content;
				Content.fAppend("%s 204 No Content\r\nDate: %s\r\n%sContent-Location: %s\r\n", Base->Request.Version, Date, Util::ConnectionResolve(Base).c_str(), Base->Request.URI.c_str());

				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, nullptr);

				Content.Append("\r\n", 2);
				return Base->Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [](Socket* Socket, int64_t Size)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Size < 0)
						return Base->Break();
					else if (Size > 0)
						return true;

					return Base->Finish(204);
				});
			}
			bool Util::RouteDELETE(Connection* Base)
			{
				if (!Base)
					return false;

				if (!Base->Route || ResourceHidden(Base, nullptr))
					return Base->Error(403, "Operation denied by server.");

				if (!Core::OS::File::State(Base->Request.Path, &Base->Resource))
				{
					if (Base->Route->Default.empty() || !Core::OS::File::State(Base->Route->Default, &Base->Resource))
						return Base->Error(404, "Requested resource was not found.");

					Base->Request.Path.assign(Base->Route->Default);
				}

				if (ResourceProvided(Base, &Base->Resource))
					return ProcessGateway(Base);

				if (!Base->Resource.IsDirectory)
				{
					if (Core::OS::File::Remove(Base->Request.Path.c_str()) != 0)
						return Base->Error(403, "Operation denied by system.");
				}
				else if (Core::OS::Directory::Remove(Base->Request.Path.c_str()) != 0)
					return Base->Error(403, "Operation denied by system.");

				char Date[64];
				Core::DateTime::TimeFormatGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				Core::Parser Content;
				Content.fAppend("%s 204 No Content\r\nDate: %s\r\n%s", Base->Request.Version, Date, Util::ConnectionResolve(Base).c_str());

				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, nullptr);

				Content.Append("\r\n", 2);
				return Base->Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [](Socket* Socket, int64_t Size)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Size < 0)
						return Base->Break();
					else if (Size > 0)
						return true;

					return Base->Finish(204);
				});
			}
			bool Util::RouteOPTIONS(Connection* Base)
			{
				if (!Base)
					return false;

				char Date[64];
				Core::DateTime::TimeFormatGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				Core::Parser Content;
				Content.fAppend("%s 204 No Content\r\nDate: %s\r\n%sAllow: GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD\r\n", Base->Request.Version, Date, Util::ConnectionResolve(Base).c_str());

				if (Base->Route && Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, &Content);

				Content.Append("\r\n", 2);
				return Base->Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [](Socket* Socket, int64_t Size)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Size < 0)
						return Base->Break();
					else if (Size > 0)
						return true;

					return Base->Finish(204);
				});
			}
			bool Util::ProcessDirectory(Connection* Base)
			{
				if (!Base || !Base->Route)
					return false;

				std::vector<Core::ResourceEntry> Entries;
				if (!Core::OS::Directory::Scan(Base->Request.Path, &Entries))
					return Base->Error(500, "System denied to directory listing.");

				char Date[64];
				Core::DateTime::TimeFormatGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				Core::Parser Content;
				Content.fAppend("%s 200 OK\r\nDate: %s\r\n%sContent-Type: text/html; charset=%s\r\nAccept-Ranges: bytes\r\n", Base->Request.Version, Date, ConnectionResolve(Base).c_str(), Base->Route->CharSet.c_str());

				ConstructHeadCache(Base, &Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, &Content);

				const char* Message = Base->Response.GetHeader("X-Error");
				if (Message != nullptr)
					Content.fAppend("X-Error: %s\n\r", Message);

				uint64_t Size = Base->Request.URI.size() - 1;
				while (Base->Request.URI[Size] != '/')
					Size--;

				char Direction = (!Base->Request.Query.empty() && Base->Request.Query[1] == 'd') ? 'a' : 'd';
				std::string Name = Compute::Common::URIDecode(Base->Request.URI);
				std::string Parent(1, '/');
				if (Base->Request.URI.size() > 1)
				{
					Parent = Base->Request.URI.substr(0, Base->Request.URI.size() - 1);
					Parent = Core::OS::Path::GetDirectory(Parent.c_str());
				}

				TextAssign(Base->Response.Buffer,
					"<html><head><title>Index of " + Name + "</title>"
					"<style>th {text-align: left;}</style></head>"
					"<body><h1>Index of " + Name + "</h1><pre><table cellpadding=\"0\">"
					"<tr><th><a href=\"?n" + Direction + "\">Name</a></th>"
					"<th><a href=\"?d" + Direction + "\">Modified</a></th>"
					"<th><a href=\"?s" + Direction + "\">Size</a></th></tr>"
					"<tr><td colspan=\"3\"><hr></td></tr>"
					"<tr><td><a href=\"" + Parent + "\">Parent directory</a></td>"
					"<td>&nbsp;-</td><td>&nbsp;&nbsp;-</td></tr>");

				for (auto& Item : Entries)
					Item.UserData = Base;

				std::sort(Entries.begin(), Entries.end(), Util::ConstructDirectoryEntries);
				for (auto& Item : Entries)
				{
					if (ResourceHidden(Base, &Item.Path))
						continue;

					char dSize[64];
					if (!Item.Source.IsDirectory)
					{
						if (Item.Source.Size < 1024)
							snprintf(dSize, sizeof(dSize), "%db", (int)Item.Source.Size);
						else if (Item.Source.Size < 0x100000)
							snprintf(dSize, sizeof(dSize), "%.1fk", ((double)Item.Source.Size) / 1024.0);
						else if (Item.Source.Size < 0x40000000)
							snprintf(dSize, sizeof(Size), "%.1fM", ((double)Item.Source.Size) / 1048576.0);
						else
							snprintf(dSize, sizeof(dSize), "%.1fG", ((double)Item.Source.Size) / 1073741824.0);
					}
					else
						strcpy(dSize, "[DIRECTORY]");

					char dDate[64];
					Core::DateTime::TimeFormatLCL(dDate, sizeof(dDate), Item.Source.LastModified);

					std::string URI = Compute::Common::URIEncode(Item.Path);
					std::string HREF = (Base->Request.URI + ((*(Base->Request.URI.c_str() + 1) != '\0' && Base->Request.URI[Base->Request.URI.size() - 1] != '/') ? "/" : "") + URI);
					if (Item.Source.IsDirectory && !Core::Parser(&HREF).EndsOf("/\\"))
						HREF.append(1, '/');

					TextAppend(Base->Response.Buffer, "<tr><td><a href=\"" + HREF + "\">" + Item.Path + "</a></td><td>&nbsp;" + dDate + "</td><td>&nbsp;&nbsp;" + dSize + "</td></tr>\n");
				}
				TextAppend(Base->Response.Buffer, "</table></pre></body></html>");

#ifdef TH_HAS_ZLIB
				bool Deflate = false, Gzip = false;
				if (Util::ResourceCompressed(Base, Base->Response.Buffer.size()))
				{
					const char* AcceptEncoding = Base->Request.GetHeader("Accept-Encoding");
					if (AcceptEncoding != nullptr)
					{
						Deflate = strstr(AcceptEncoding, "deflate") != nullptr;
						Gzip = strstr(AcceptEncoding, "gzip") != nullptr;
					}

					if (AcceptEncoding != nullptr && (Deflate || Gzip))
					{
						z_stream Stream;
						Stream.zalloc = Z_NULL;
						Stream.zfree = Z_NULL;
						Stream.opaque = Z_NULL;
						Stream.avail_in = (uInt)Base->Response.Buffer.size();
						Stream.next_in = (Bytef*)Base->Response.Buffer.data();

						if (deflateInit2(&Stream, Base->Route->Compression.QualityLevel, Z_DEFLATED, (Gzip ? 15 | 16 : 15), Base->Route->Compression.MemoryLevel, (int)Base->Route->Compression.Tune) == Z_OK)
						{
							std::string Buffer(Base->Response.Buffer.size(), '\0');
							Stream.avail_out = (uInt)Buffer.size();
							Stream.next_out = (Bytef*)Buffer.c_str();
							bool Compress = (deflate(&Stream, Z_FINISH) == Z_STREAM_END);
							bool Flush = (deflateEnd(&Stream) == Z_OK);

							if (Compress && Flush)
							{
								TextAssign(Base->Response.Buffer, Buffer.c_str(), (uint64_t)Stream.total_out);
								if (!Base->Response.GetHeader("Content-Encoding"))
								{
									if (Gzip)
										Content.Append("Content-Encoding: gzip\r\n", 24);
									else
										Content.Append("Content-Encoding: deflate\r\n", 27);
								}
							}
						}
					}
				}
#endif
				Content.fAppend("Content-Length: %llu\r\n\r\n", (uint64_t)Base->Response.Buffer.size());
				if (strcmp(Base->Request.Method, "HEAD") != 0)
					Content.Append(Base->Response.Buffer.data());

				Base->Response.Buffer.clear();
				return Base->Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [](Socket* Socket, int64_t Size)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Size < 0)
						return Base->Break();
					else if (Size > 0)
						return true;

					return Base->Finish(200);
				});
			}
			bool Util::ProcessResource(Connection* Base)
			{
				if (!Base || !Base->Route)
					return false;

				const char* ContentType = Util::ContentType(Base->Request.Path, &Base->Route->MimeTypes);
				const char* Range = Base->Request.GetHeader("Range");
				const char* StatusMessage = Util::StatusMessage(Base->Response.StatusCode = (Base->Response.Error && Base->Response.StatusCode > 0 ? Base->Response.StatusCode : 200));
				int64_t Range1 = 0, Range2 = 0, Count = 0;
				int64_t ContentLength = Base->Resource.Size;

				char ContentRange[128] = { 0 };
				if (Range != nullptr && (Count = Util::ParseContentRange(Range, &Range1, &Range2)) > 0 && Range1 >= 0 && Range2 >= 0)
				{
					if (Count == 2)
						ContentLength = ((Range2 > ContentLength) ? ContentLength : Range2) - Range1 + 1;
					else
						ContentLength -= Range1;

					snprintf(ContentRange, sizeof(ContentRange), "Content-Range: bytes %lld-%lld/%lld\r\n", Range1, Range1 + ContentLength - 1, (int64_t)Base->Resource.Size);
					StatusMessage = Util::StatusMessage(Base->Response.StatusCode = (Base->Response.Error && Base->Response.StatusCode > 0 ? Base->Response.StatusCode : 206));
				}

#ifdef TH_HAS_ZLIB
				if (Util::ResourceCompressed(Base, (uint64_t)ContentLength))
				{
					const char* AcceptEncoding = Base->Request.GetHeader("Accept-Encoding");
					if (AcceptEncoding != nullptr)
					{
						bool Deflate = strstr(AcceptEncoding, "deflate") != nullptr;
						bool Gzip = strstr(AcceptEncoding, "gzip") != nullptr;

						if (Deflate || Gzip)
							return ProcessResourceCompress(Base, Deflate, Gzip, ContentRange, Range1);
					}
				}
#endif
				const char* Origin = Base->Request.GetHeader("Origin");
				const char* CORS1 = "", *CORS2 = "", *CORS3 = "";
				if (Origin != nullptr)
				{
					CORS1 = "Access-Control-Allow-Origin: ";
					CORS2 = Base->Route->AccessControlAllowOrigin.c_str();
					CORS3 = "\r\n";
				}

				char Date[64];
				Core::DateTime::TimeFormatGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				char LastModified[64];
				Core::DateTime::TimeFormatGMT(LastModified, sizeof(LastModified), Base->Resource.LastModified);

				char ETag[64];
				Core::OS::Net::GetETag(ETag, sizeof(ETag), &Base->Resource);

				Core::Parser Content;
				Content.fAppend("%s %d %s\r\n%s%s%sDate: %s\r\n", Base->Request.Version, Base->Response.StatusCode, StatusMessage, CORS1, CORS2, CORS3, Date);

				Util::ConstructHeadCache(Base, &Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, &Content);

				const char* Message = Base->Response.GetHeader("X-Error");
				if (Message != nullptr)
					Content.fAppend("X-Error: %s\n\r", Message);

				Content.fAppend("Accept-Ranges: bytes\r\n"
					"Last-Modified: %s\r\nEtag: %s\r\n"
					"Content-Type: %s; charset=%s\r\n"
					"Content-Length: %lld\r\n"
					"%s%s\r\n", LastModified, ETag, ContentType, Base->Route->CharSet.c_str(), ContentLength, Util::ConnectionResolve(Base).c_str(), ContentRange);

				if (!ContentLength || !strcmp(Base->Request.Method, "HEAD"))
				{
					return Base->Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [](Socket* Socket, int64_t Size)
					{
						auto Base = Socket->Context<HTTP::Connection>();
						if (Size < 0)
							return Base->Break();
						else if (Size > 0)
							return true;

						return Base->Finish();
					});
				}

				return Base->Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [ContentLength, Range1](Socket* Socket, int64_t Size)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Size < 0)
						return Base->Break();
					else if (Size > 0)
						return true;

					return Core::Schedule::Get()->SetTask([Base, ContentLength, Range1]()
					{
						Util::ProcessFile(Base, ContentLength, Range1);
					});
				});
			}
			bool Util::ProcessResourceCompress(Connection* Base, bool Deflate, bool Gzip, const char* ContentRange, uint64_t Range)
			{
				if (!Base || !Base->Route || (!Deflate && !Gzip) || !ContentRange)
					return false;

				const char* ContentType = Util::ContentType(Base->Request.Path, &Base->Route->MimeTypes);
				const char* StatusMessage = Util::StatusMessage(Base->Response.StatusCode = (Base->Response.Error && Base->Response.StatusCode > 0 ? Base->Response.StatusCode : 200));
				int64_t ContentLength = Base->Resource.Size;

				const char* Origin = Base->Request.GetHeader("Origin");
				const char* CORS1 = "", *CORS2 = "", *CORS3 = "";
				if (Origin != nullptr)
				{
					CORS1 = "Access-Control-Allow-Origin: ";
					CORS2 = Base->Route->AccessControlAllowOrigin.c_str();
					CORS3 = "\r\n";
				}

				char Date[64];
				Core::DateTime::TimeFormatGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				char LastModified[64];
				Core::DateTime::TimeFormatGMT(LastModified, sizeof(LastModified), Base->Resource.LastModified);

				char ETag[64];
				Core::OS::Net::GetETag(ETag, sizeof(ETag), &Base->Resource);

				Core::Parser Content;
				Content.fAppend("%s %d %s\r\n%s%s%sDate: %s\r\n", Base->Request.Version, Base->Response.StatusCode, StatusMessage, CORS1, CORS2, CORS3, Date);

				Util::ConstructHeadCache(Base, &Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, &Content);

				const char* Message = Base->Response.GetHeader("X-Error");
				if (Message != nullptr)
					Content.fAppend("X-Error: %s\n\r", Message);

				Content.fAppend("Accept-Ranges: bytes\r\n"
					"Last-Modified: %s\r\nEtag: %s\r\n"
					"Content-Type: %s; charset=%s\r\n"
					"Content-Encoding: %s\r\n"
					"Transfer-Encoding: chunked\r\n"
					"%s%s\r\n", LastModified, ETag, ContentType, Base->Route->CharSet.c_str(), (Gzip ? "gzip" : "deflate"), Util::ConnectionResolve(Base).c_str(), ContentRange);

				if (!ContentLength || !strcmp(Base->Request.Method, "HEAD"))
				{
					return Base->Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [](Socket* Socket, int64_t Size)
					{
						auto Base = Socket->Context<HTTP::Connection>();
						if (Size < 0)
							return Base->Break();
						else if (Size > 0)
							return true;

						return Base->Finish();
					});
				}

				return Base->Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [Range, ContentLength, Gzip](Socket* Socket, int64_t Size)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Size < 0)
						return Base->Break();
					else if (Size > 0)
						return true;

					return Core::Schedule::Get()->SetTask([Base, Range, ContentLength, Gzip]()
					{
						Util::ProcessFileCompress(Base, ContentLength, Range, Gzip);
					});
				});
			}
			bool Util::ProcessResourceCache(Connection* Base)
			{
				if (!Base || !Base->Route)
					return false;

				char Date[64];
				Core::DateTime::TimeFormatGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				char LastModified[64];
				Core::DateTime::TimeFormatGMT(LastModified, sizeof(LastModified), Base->Resource.LastModified);

				char ETag[64];
				Core::OS::Net::GetETag(ETag, sizeof(ETag), &Base->Resource);

				Core::Parser Content;
				Content.fAppend("%s 304 %s\r\nDate: %s\r\n", Base->Request.Version, HTTP::Util::StatusMessage(304), Date);

				Util::ConstructHeadCache(Base, &Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, &Content);

				Content.fAppend("Accept-Ranges: bytes\r\nLast-Modified: %s\r\nEtag: %s\r\n%s\r\n", LastModified, ETag, Util::ConnectionResolve(Base).c_str());
				return Base->Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [](Socket* Socket, int64_t Size)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Size < 0)
						return Base->Break();
					else if (Size > 0)
						return true;

					return Base->Finish(304);
				});
			}
			bool Util::ProcessFile(Connection* Base, uint64_t ContentLength, uint64_t Range)
			{
				if (!Base || !Base->Route)
					return false;

				Range = (Range > Base->Resource.Size ? Base->Resource.Size : Range);
				if (ContentLength > 0 && Base->Resource.IsReferenced && Base->Resource.Size > 0)
				{
					uint64_t sLimit = Base->Resource.Size - Range;
					if (ContentLength > sLimit)
						ContentLength = sLimit;

					if (Base->Response.Buffer.size() >= ContentLength)
					{
						return Base->Stream->WriteAsync(Base->Response.Buffer.data() + Range, (int64_t)ContentLength, [](Socket* Socket, int64_t Size)
						{
							auto Base = Socket->Context<HTTP::Connection>();
							if (Size < 0)
								return Base->Break();
							else if (Size > 0)
								return true;

							return Base->Finish();
						});
					}
				}

				FILE* Stream = (FILE*)Core::OS::File::Open(Base->Request.Path.c_str(), "rb");
				if (!Stream)
					return Base->Error(500, "System denied to open resource stream.");

#ifdef TH_MICROSOFT
				if (Range > 0 && _lseeki64(_fileno(Stream), Range, SEEK_SET) == -1)
				{
					fclose(Stream);
					return Base->Error(400, "Provided content range offset (%llu) is invalid", Range);
				}
#elif defined(TH_APPLE)
				if (Range > 0 && fseek(Stream, Range, SEEK_SET) == -1)
				{
					fclose(Stream);
					return Base->Error(400, "Provided content range offset (%llu) is invalid", Range);
				}
#else
				if (Range > 0 && lseek64(fileno(Stream), Range, SEEK_SET) == -1)
				{
					fclose(Stream);
					return Base->Error(400, "Provided content range offset (%llu) is invalid", Range);
				}
#endif
				Server* Router = Base->Root;
				if (!Base->Route->AllowSendFile)
					return ProcessFileChunk(Base, Router, Stream, ContentLength);

				Base->Stream->SetBlocking(true);
				Base->Stream->SetTimeout((int)Base->Root->Router->SocketTimeout);
				bool Result = Core::OS::Net::SendFile(Stream, Base->Stream->GetFd(), ContentLength);
				Base->Stream->SetTimeout(0);
				Base->Stream->SetBlocking(false);

				if (Router->State != ServerState::Working)
				{
					fclose(Stream);
					return Base->Break();
				}

				if (!Result)
					return ProcessFileChunk(Base, Router, Stream, ContentLength);

				fclose(Stream);
				return Base->Finish();
			}
			bool Util::ProcessFileChunk(Connection* Base, Server* Router, FILE* Stream, uint64_t ContentLength)
			{
				if (!ContentLength || Router->State != ServerState::Working)
				{
				Cleanup:
					fclose(Stream);
					if (Router->State != ServerState::Working)
						return Base->Break();

					return Base->Finish() || true;
				}

				char Buffer[8192]; int Read = sizeof(Buffer);
				if ((Read = (int)fread(Buffer, 1, (size_t)(Read > ContentLength ? ContentLength : Read), Stream)) <= 0)
					goto Cleanup;

				ContentLength -= (int64_t)Read;
				Base->Stream->WriteAsync(Buffer, Read, [Base, Router, Stream, ContentLength](Socket*, int64_t Size)
				{
					if (Size < 0)
					{
						fclose(Stream);
						return Base->Break() && false;
					}
					else if (Size > 0)
						return true;

					return Core::Schedule::Get()->SetTask([Base, Router, Stream, ContentLength]()
					{
						ProcessFileChunk(Base, Router, Stream, ContentLength);
					});
				});

				return false;
			}
			bool Util::ProcessFileCompress(Connection* Base, uint64_t ContentLength, uint64_t Range, bool Gzip)
			{
				if (!Base || !Base->Route)
					return false;

				Range = (Range > Base->Resource.Size ? Base->Resource.Size : Range);
				if (ContentLength > 0 && Base->Resource.IsReferenced && Base->Resource.Size > 0)
				{
					if (Base->Response.Buffer.size() >= ContentLength)
					{
#ifdef TH_HAS_ZLIB
						z_stream ZStream;
						ZStream.zalloc = Z_NULL;
						ZStream.zfree = Z_NULL;
						ZStream.opaque = Z_NULL;
						ZStream.avail_in = (uInt)Base->Response.Buffer.size();
						ZStream.next_in = (Bytef*)Base->Response.Buffer.data();

						if (deflateInit2(&ZStream, Base->Route->Compression.QualityLevel, Z_DEFLATED, (Gzip ? 15 | 16 : 15), Base->Route->Compression.MemoryLevel, (int)Base->Route->Compression.Tune) == Z_OK)
						{
							std::string Buffer(Base->Response.Buffer.size(), '\0');
							ZStream.avail_out = (uInt)Buffer.size();
							ZStream.next_out = (Bytef*)Buffer.c_str();
							bool Compress = (deflate(&ZStream, Z_FINISH) == Z_STREAM_END);
							bool Flush = (deflateEnd(&ZStream) == Z_OK);

							if (Compress && Flush)
								TextAssign(Base->Response.Buffer, Buffer.c_str(), (uint64_t)ZStream.total_out);
						}
#endif
						return Base->Stream->WriteAsync(Base->Response.Buffer.data(), (int64_t)ContentLength, [](Socket* Socket, int64_t Size)
						{
							auto Base = Socket->Context<HTTP::Connection>();
							if (Size < 0)
								return Base->Break();
							else if (Size > 0)
								return true;

							return Base->Finish();
						});
					}
				}

				FILE* Stream = (FILE*)Core::OS::File::Open(Base->Request.Path.c_str(), "rb");
				if (!Stream)
					return Base->Error(500, "System denied to open resource stream.");

#ifdef TH_MICROSOFT
				if (Range > 0 && _lseeki64(_fileno(Stream), Range, SEEK_SET) == -1)
				{
					fclose(Stream);
					return Base->Error(400, "Provided content range offset (%llu) is invalid", Range);
				}
#elif defined(TH_APPLE)
				if (Range > 0 && fseek(Stream, Range, SEEK_SET) == -1)
				{
					fclose(Stream);
					return Base->Error(400, "Provided content range offset (%llu) is invalid", Range);
				}
#else
				if (Range > 0 && lseek64(fileno(Stream), Range, SEEK_SET) == -1)
				{
					fclose(Stream);
					return Base->Error(400, "Provided content range offset (%llu) is invalid", Range);
				}
#endif
#ifdef TH_HAS_ZLIB
				Server* Server = Base->Root;
				z_stream* ZStream = (z_stream*)TH_MALLOC(sizeof(z_stream));
				ZStream->zalloc = Z_NULL;
				ZStream->zfree = Z_NULL;
				ZStream->opaque = Z_NULL;

				if (deflateInit2(ZStream, Base->Route->Compression.QualityLevel, Z_DEFLATED, (Gzip ? MAX_WBITS + 16 : MAX_WBITS), Base->Route->Compression.MemoryLevel, (int)Base->Route->Compression.Tune) != Z_OK)
				{
					fclose(Stream);
					TH_FREE(ZStream);
					return Base->Break();
				}

				return ProcessFileCompressChunk(Base, Server, Stream, ZStream, ContentLength);
#else
				fclose(Stream);
				return Base->Error(500, "Cannot process gzip stream.");
#endif
			}
			bool Util::ProcessFileCompressChunk(Connection* Base, Server* Router, FILE* Stream, void* CStream, uint64_t ContentLength)
			{
#ifdef TH_HAS_ZLIB
#define FREE_STREAMING { fclose(Stream); deflateEnd(ZStream); TH_FREE(ZStream); }
				z_stream* ZStream = (z_stream*)CStream;
				if (!ContentLength || Router->State != ServerState::Working)
				{
				Cleanup:
					FREE_STREAMING;
					if (Router->State != ServerState::Working)
						return Base->Break();

					return Base->Stream->WriteAsync("0\r\n\r\n", 5, [Base](Socket*, int64_t Size)
					{
						if (Size < 0)
							return Base->Break() && false;
						else if (Size > 0)
							return true;

						return Base->Finish() || true;
					}) || true;
				}

				static const int Head = 17;
				char Buffer[8192 + Head], Deflate[8192];
				int Read = sizeof(Buffer) - Head;

				if ((Read = (int)fread(Buffer, 1, (size_t)(Read > ContentLength ? ContentLength : Read), Stream)) <= 0)
					goto Cleanup;

				ContentLength -= (int64_t)Read;
				ZStream->avail_in = (uInt)Read;
				ZStream->next_in = (Bytef*)Buffer;
				ZStream->avail_out = (uInt)sizeof(Deflate);
				ZStream->next_out = (Bytef*)Deflate;
				deflate(ZStream, Z_SYNC_FLUSH);
				Read = (int)sizeof(Deflate) - (int)ZStream->avail_out;

				int Next = snprintf(Buffer, sizeof(Buffer), "%X\r\n", Read);
				memcpy(Buffer + Next, Deflate, Read);
				Read += Next;

				if (!ContentLength)
				{
					memcpy(Buffer + Read, "\r\n0\r\n\r\n", sizeof(char) * 7);
					Read += sizeof(char) * 7;
				}
				else
				{
					memcpy(Buffer + Read, "\r\n", sizeof(char) * 2);
					Read += sizeof(char) * 2;
				}

				Base->Stream->WriteAsync(Buffer, Read, [Base, Router, Stream, ZStream, ContentLength](Socket*, int64_t Size)
				{
					if (Size < 0)
					{
						FREE_STREAMING;
						return Base->Break() && false;
					}
					else if (Size > 0)
						return true;

					if (ContentLength > 0)
					{
						return Core::Schedule::Get()->SetTask([Base, Router, Stream, ZStream, ContentLength]()
						{
							ProcessFileCompressChunk(Base, Router, Stream, ZStream, ContentLength);
						});
					}

					FREE_STREAMING;
					return (Router->State == ServerState::Working ? Base->Finish() : Base->Break()) && false;
				});

				return false;
#undef FREE_STREAMING
#else
				return Base->Finish();
#endif
			}
			bool Util::ProcessGateway(Connection* Base)
			{
				if (!Base || !Base->Route || !Base->Route->Callbacks.Gateway)
					return false;

				Script::VMManager* VM = ((MapRouter*)Base->Root->GetRouter())->VM;
				if (!VM)
					return Base->Error(500, "Gateway cannot be issued.") && false;

				return Core::Schedule::Get()->SetTask([Base, VM]()
				{
					Script::VMCompiler* Compiler = VM->CreateCompiler();
					if (Compiler->Prepare(Core::OS::Path::GetFilename(Base->Request.Path.c_str()), Base->Request.Path, true, true) < 0)
					{
						TH_RELEASE(Compiler);
						return (void)Base->Error(500, "Gateway module cannot be prepared.");
					}

					char* Buffer = nullptr;
					if (Base->Route->Callbacks.Gateway)
					{
						if (!Base->Route->Callbacks.Gateway(Base, Compiler))
						{
							TH_RELEASE(Compiler);
							return (void)Base->Error(500, "Gateway creation exception.");
						}
					}

					int64_t Size = -1;
					if (!Compiler->IsCached())
					{
						FILE* Stream = (FILE*)Core::OS::File::Open(Base->Request.Path.c_str(), "rb");
						if (!Stream)
							return (void)Base->Error(404, "Gateway resource was not found.");

						Size = Base->Resource.Size;
						Buffer = (char*)TH_MALLOC((size_t)(Size + 1) * sizeof(char));

						if (fread(Buffer, 1, (size_t)Size, Stream) != (size_t)Size)
						{
							fclose(Stream);
							TH_FREE(Buffer);
							return (void)Base->Error(500, "Gateway resource stream exception.");
						}

						Buffer[Size] = '\0';
						fclose(Stream);
					}

					Base->Gateway = TH_NEW(GatewayFrame, Buffer, Size);
					Base->Gateway->Compiler = Compiler;
					Base->Gateway->Base = Base;
					Base->Gateway->Start();
				});
			}
			bool Util::ProcessWebSocket(Connection* Base, const char* Key)
			{
				static const char* Magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
				if (!Base || !Key)
					return false;

				const char* Version = Base->Request.GetHeader("Sec-WebSocket-Version");
				if (!Base->Route || !Version || strcmp(Version, "13") != 0)
					return Base->Error(426, "Protocol upgrade required. Version \"%s\" is not allowed", Version);

				char Buffer[100];
				snprintf(Buffer, sizeof(Buffer), "%s%s", Key, Magic);
				Base->Request.Buffer.clear();

				char Encoded20[20];
				Compute::Common::Sha1Compute(Buffer, (int)strlen(Buffer), (unsigned char*)Encoded20);

				Core::Parser Content;
				Content.fAppend(
					"HTTP/1.1 101 Switching Protocols\r\n"
					"Upgrade: websocket\r\n"
					"Connection: Upgrade\r\n"
					"Sec-WebSocket-Accept: %s\r\n", Compute::Common::Base64Encode((const unsigned char*)Encoded20, 20).c_str());

				const char* Protocol = Base->Request.GetHeader("Sec-WebSocket-Protocol");
				if (Protocol != nullptr)
				{
					const char* Offset = strchr(Protocol, ',');
					if (Offset != nullptr)
						Content.fAppend("Sec-WebSocket-Protocol: %.*s\r\n", (int)(Offset - Protocol), Protocol);
					else
						Content.fAppend("Sec-WebSocket-Protocol: %s\r\n", Protocol);
				}

				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, &Content);

				Content.Append("\r\n", 2);
				return !Base->Stream->WriteAsync(Content.Get(), (int64_t)Content.Size(), [](Socket* Socket, int64_t Size)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Size < 0)
						return Base->Break();
					else if (Size > 0)
						return true;

					Base->WebSocket = TH_NEW(WebSocketFrame);
					Base->WebSocket->Connect = Base->Route->Callbacks.WebSocket.Connect;
					Base->WebSocket->Receive = Base->Route->Callbacks.WebSocket.Receive;
					Base->WebSocket->Disconnect = Base->Route->Callbacks.WebSocket.Disconnect;
					Base->WebSocket->Base = Base;
					Base->Stream->SetAsyncTimeout(Base->Route->WebSocketTimeout);

					if (ResourceProvided(Base, &Base->Resource))
						return ProcessGateway(Base);

					Base->WebSocket->Next();
					return true;
				});
			}
			bool Util::ProcessWebSocketPass(Connection* Base)
			{
				if (!Base || !Base->WebSocket)
					return false;

				if (Base->WebSocket->Notified)
				{
					Base->WebSocket->Notified = false;
					if (Base->WebSocket->Notification)
					{
						Base->WebSocket->Notification(Base->WebSocket);
						return true;
					}
				}

				if (Base->WebSocket->Clear)
				{
					Base->WebSocket->Buffer.clear();
					Base->Request.Buffer.clear();
					Base->WebSocket->Clear = false;
				}

				Base->WebSocket->HeaderLength = 0;
				if (Base->Request.Buffer.size() >= 2)
				{
					Base->WebSocket->BodyLength = Base->Request.Buffer[1] & 127;
					Base->WebSocket->MaskLength = (Base->Request.Buffer[1] & 128) ? 4 : 0;

					if (Base->WebSocket->BodyLength < 126 && Base->Request.Buffer.size() >= Base->WebSocket->MaskLength)
					{
						Base->WebSocket->DataLength = Base->WebSocket->BodyLength;
						Base->WebSocket->HeaderLength = 2 + Base->WebSocket->MaskLength;
					}
					else if (Base->WebSocket->BodyLength == 126 && Base->Request.Buffer.size() >= Base->WebSocket->MaskLength + 4)
					{
						Base->WebSocket->HeaderLength = Base->WebSocket->MaskLength + 4;
						Base->WebSocket->DataLength = (((uint64_t)Base->Request.Buffer[2]) << 8) + Base->Request.Buffer[3];
					}
					else if (Base->Request.Buffer.size() >= 10 + Base->WebSocket->MaskLength + 10)
					{
						Base->WebSocket->HeaderLength = Base->WebSocket->MaskLength + 10;
						Base->WebSocket->DataLength = (((uint64_t)ntohl(*(uint32_t*)(void*)&Base->Request.Buffer[2])) << 32) + ntohl(*(uint32_t*)(void*)&Base->Request.Buffer[6]);
					}
				}

				if (Base->WebSocket->HeaderLength > 0 && Base->Request.Buffer.size() >= Base->WebSocket->HeaderLength)
				{
					if (Base->WebSocket->MaskLength > 0)
						memcpy(Base->WebSocket->Mask, Base->Request.Buffer.c_str() + Base->WebSocket->HeaderLength - Base->WebSocket->MaskLength, sizeof(Base->WebSocket->Mask));
					else
						memset(Base->WebSocket->Mask, 0, sizeof(Base->WebSocket->Mask));

					if (Base->WebSocket->DataLength + Base->WebSocket->HeaderLength > Base->Request.Buffer.size())
					{
						Base->WebSocket->Opcode = Base->Request.Buffer[0] & 0xF;
						Base->WebSocket->BodyLength = Base->Request.Buffer.size() - Base->WebSocket->HeaderLength;
						Base->WebSocket->Buffer.assign(Base->Request.Buffer.c_str() + Base->WebSocket->HeaderLength, Base->WebSocket->BodyLength);

						return Base->Stream->ReadAsync(Base->WebSocket->DataLength - Base->WebSocket->BodyLength, [](Socket* Socket, const char* Buffer, int64_t Size)
						{
							auto Base = Socket->Context<HTTP::Connection>();
							if (!Base)
								return false;

							if (Size > 0)
							{
								Base->Request.Buffer.append(Buffer, Size);
								Base->WebSocket->BodyLength += Size;
								return true;
							}
							else if (Size < 0)
							{
								Base->WebSocket->Finish();
								return false;
							}

							return ProcessWebSocketPass(Base);
						});
					}
					else
					{
						Base->WebSocket->Opcode = Base->Request.Buffer[0] & 0xF;
						Base->WebSocket->BodyLength = Base->WebSocket->DataLength + Base->WebSocket->HeaderLength;
						Base->WebSocket->Buffer.assign(Base->Request.Buffer.c_str() + Base->WebSocket->HeaderLength, Base->WebSocket->DataLength);
					}

					if (Base->WebSocket->MaskLength > 0)
					{
						for (uint64_t i = 0; i < Base->WebSocket->DataLength; i++)
							Base->WebSocket->Buffer[i] ^= Base->WebSocket->Mask[i % 4];
					}

					Base->WebSocket->Clear = true;
					if (Base->WebSocket->Opcode == (unsigned char)WebSocketOp::Close)
					{
						if (Base->WebSocket->State != (uint32_t)WebSocketState::Close)
							Base->WebSocket->State = (uint32_t)WebSocketState::Reset;
						else
							Base->WebSocket->State = (uint32_t)WebSocketState::Free;

						Base->WebSocket->Next();
					}
					else if (Base->WebSocket->Opcode == (unsigned char)WebSocketOp::Ping)
					{
						Base->WebSocket->Write("", 0, WebSocketOp::Pong, [](Connection* Base)
						{
							Base->WebSocket->Next();
							return true;
						});
					}
					else if (Base->WebSocket->Receive)
						Base->WebSocket->Receive(Base->WebSocket, Base->WebSocket->Buffer.c_str(), (int64_t)Base->WebSocket->Buffer.size(), (WebSocketOp)Base->WebSocket->Opcode);
					else
						Base->WebSocket->Finish();

					return true;
				}

				return !Base->Stream->ReadAsync(8192, [](Socket* Socket, const char* Buffer, int64_t Size)
				{
					auto Base = Socket->Context<HTTP::Connection>();
					if (Size < 0)
					{
						if (Size != -2)
							Base->WebSocket->Error = true;

						return Base->Break();
					}
					else if (Size > 0)
					{
						Base->Request.Buffer.append(Buffer, Size);
						return false;
					}

					return ProcessWebSocketPass(Base);
				});
			}

			Server::Server() : SocketServer()
			{
			}
			Server::~Server()
			{
				Unlisten();
			}
			bool Server::OnConfigure(SocketRouter* NewRouter)
			{
				std::unordered_set<std::string> Modules;
				std::string Directory = Core::OS::Directory::Get();
				auto* Root = (MapRouter*)NewRouter;

				Root->ModuleRoot = Core::OS::Path::ResolveDirectory(Root->ModuleRoot.c_str());
				for (auto* Entry : Root->Sites)
				{
					Entry->Gateway.Session.DocumentRoot = Core::OS::Path::ResolveDirectory(Entry->Gateway.Session.DocumentRoot.c_str());
					Entry->ResourceRoot = Core::OS::Path::ResolveDirectory(Entry->ResourceRoot.c_str());
					Entry->Base->DocumentRoot = Core::OS::Path::ResolveDirectory(Entry->Base->DocumentRoot.c_str());
					Entry->Base->URI = Compute::RegexSource("/");
					Entry->Base->Site = Entry;
					Entry->Router = Root;

					if (!Entry->Gateway.Session.DocumentRoot.empty())
						Core::OS::Directory::Patch(Entry->Gateway.Session.DocumentRoot);

					if (!Entry->ResourceRoot.empty())
						Core::OS::Directory::Patch(Entry->ResourceRoot);

					if (Entry->Hosts.empty())
						TH_WARN("site \"%s\" has no hosts", Entry->SiteName.c_str());

					if (!Entry->Base->Default.empty())
						Entry->Base->Default = Core::OS::Path::ResolveResource(Entry->Base->Default, Entry->Base->DocumentRoot);

					for (auto& Item : Entry->Base->ErrorFiles)
						Item.Pattern = Core::OS::Path::Resolve(Item.Pattern.c_str());

					for (auto* Route : Entry->Routes)
					{
						Route->DocumentRoot = Core::OS::Path::ResolveDirectory(Route->DocumentRoot.c_str());
						Route->Site = Entry;

						if (!Route->Default.empty())
							Route->Default = Core::OS::Path::ResolveResource(Route->Default, Route->DocumentRoot);

						for (auto& File : Route->ErrorFiles)
							File.Pattern = Core::OS::Path::Resolve(File.Pattern.c_str());

						if (!Root->VM || !Entry->Gateway.Enabled || !Entry->Gateway.Verify)
							continue;

						if (Modules.find(Route->DocumentRoot) != Modules.end())
							continue;

						Modules.insert(Route->DocumentRoot);
						for (auto& Exp : Route->Gateway.Files)
						{
							std::vector<std::string> Result = Root->VM->VerifyModules(Route->DocumentRoot, Exp);
							if (!Result.empty())
							{
								std::string Files;
								for (auto& Name : Result)
									Files += "\n\t" + Name;

								TH_ERROR("(vm) there are errors in %i module(s)%s", (int)Result.size(), Files.c_str());
								Entry->Gateway.Enabled = false;
								break;
							}
						}

						if (Entry->Gateway.Enabled && !Route->Gateway.Files.empty())
							TH_INFO("(vm) modules are verified for: %s", Route->DocumentRoot.c_str());
					}

					qsort((void*)Entry->Routes.data(), (size_t)Entry->Routes.size(), sizeof(HTTP::RouteEntry*), [](const void* A1, const void* B1) -> int
					{
						HTTP::RouteEntry* A = *(HTTP::RouteEntry**)B1;
						HTTP::RouteEntry* B = *(HTTP::RouteEntry**)A1;
						A->URI.IgnoreCase = true;
						B->URI.IgnoreCase = true;

						if (A->URI.GetRegex().empty())
							return -1;

						if (B->URI.GetRegex().empty())
							return 1;

						bool fA = A->URI.IsSimple(), fB = B->URI.IsSimple();
						if (fA && !fB)
							return -1;
						else if (!fA && fB)
							return 1;

						return (int)A->URI.GetRegex().size() - (int)B->URI.GetRegex().size();
					});
				}

				return true;
			}
			bool Server::OnRequestEnded(SocketConnection* Root, bool Check)
			{
				auto Base = (HTTP::Connection*)Root;
				if (Check)
				{
					if (Base->Request.ContentLength > 0 && Base->Request.ContentState == Content::Not_Loaded)
					{
						Base->Consume([](Connection* Base, const char*, int64_t Size)
						{
							return (Size <= 0 ? Base->Root->Manage(Base) : true);
						});

						return false;
					}

					return true;
				}

				for (auto& Item : Base->Request.Resources)
				{
					if (!Item.Memory)
						Core::OS::File::Remove(Item.Path.c_str());
				}

				if (Base->Info.KeepAlive >= -1 && Base->Response.StatusCode >= 0 && Base->Route && Base->Route->Callbacks.Access)
					Base->Route->Callbacks.Access(Base);

				Base->Route = nullptr;
				Base->Stream->Income = 0;
				Base->Stream->Outcome = 0;
				Base->Info.Close = (Base->Info.Close || Base->Response.StatusCode < 0);
				Base->Request.ContentState = Content::Not_Loaded;
				Base->Response.Error = false;
				Base->Response.StatusCode = -1;
				Base->Response.Buffer.clear();
				Base->Response.Cookies.clear();
				Base->Response.Headers.clear();
				Base->Request.Resources.clear();
				Base->Request.Buffer.clear();
				Base->Request.Headers.clear();
				Base->Request.Cookies.clear();
				Base->Request.Query.clear();
				Base->Request.Path.clear();
				Base->Request.URI.clear();

				memset(Base->Request.Method, 0, sizeof(Base->Request.Method));
				memset(Base->Request.Version, 0, sizeof(Base->Request.Version));
				memset(Base->Request.RemoteAddress, 0, sizeof(Base->Request.RemoteAddress));

				return true;
			}
			bool Server::OnRequestBegin(SocketConnection* Base)
			{
				auto Conf = (MapRouter*)Router;
				return Base->Stream->ReadUntilAsync("\r\n\r\n", [Conf](Socket* Fd, const char* Buffer, int64_t Size)
				{
					auto Base = Fd->Context<HTTP::Connection>();
					if (Size > 0)
					{
						Base->Request.Buffer.append(Buffer, Size);
						return true;
					}
					else if (Size < 0)
						return Base->Break();

					ParserFrame Segment;
					Segment.Request = &Base->Request;

					HTTP::Parser* Parser = new HTTP::Parser();
					Parser->OnMethodValue = Util::ParseMethodValue;
					Parser->OnPathValue = Util::ParsePathValue;
					Parser->OnQueryValue = Util::ParseQueryValue;
					Parser->OnVersion = Util::ParseVersion;
					Parser->OnHeaderField = Util::ParseHeaderField;
					Parser->OnHeaderValue = Util::ParseHeaderValue;
					Parser->UserPointer = &Segment;

					strcpy(Base->Request.RemoteAddress, Base->Stream->GetRemoteAddress().c_str());
					Base->Info.Start = Driver::Clock();

					if (Parser->ParseRequest(Base->Request.Buffer.c_str(), Base->Request.Buffer.size(), 0) < 0)
					{
						Base->Request.Buffer.clear();
						TH_RELEASE(Parser);

						return Base->Error(400, "Invalid request was provided by client");
					}

					Base->Request.Buffer.clear();
					TH_RELEASE(Parser);

					if (!Util::ConstructRoute(Conf, Base) || !Base->Route)
						return Base->Error(400, "Request cannot be resolved");

					if (!Base->Route->Refer.empty())
					{
						Base->Request.URI = Base->Route->Refer;
						if (!Util::ConstructRoute(Conf, Base))
							Base->Route = Base->Route->Site->Base;
					}

					const char* ContentLength = Base->Request.GetHeader("Content-Length");
					if (ContentLength != nullptr)
					{
						int64_t Len = std::atoll(ContentLength);
						Base->Request.ContentLength = (Len <= 0 ? 0 : Len);
					}

					if (!Base->Request.ContentLength)
						Base->Request.ContentState = Content::Empty;

					if (!Base->Route->ProxyIpAddress.empty())
					{
						const char* Address = Base->Request.GetHeader(Base->Route->ProxyIpAddress.c_str());
						if (Address != nullptr)
							strcpy(Base->Request.RemoteAddress, Address);
					}

					Util::ConstructPath(Base);
					if (!Util::MethodAllowed(Base))
						return Base->Error(405, "Requested method \"%s\" is not allowed on this server", Base->Request.Method);

					if (!Util::Authorize(Base))
						return false;

					if (!memcmp(Base->Request.Method, "GET", 3) || !memcmp(Base->Request.Method, "HEAD", 4))
					{
						if (Base->Route->Callbacks.Get)
							return Base->Route->Callbacks.Get(Base);

						return Util::RouteGET(Base);
					}
					else if (!memcmp(Base->Request.Method, "POST", 4))
					{
						if (Base->Route->Callbacks.Post)
							return Base->Route->Callbacks.Post(Base);

						return Util::RoutePOST(Base);
					}
					else if (!memcmp(Base->Request.Method, "PUT", 3))
					{
						if (Base->Route->Callbacks.Put)
							return Base->Route->Callbacks.Put(Base);

						return Util::RoutePUT(Base);
					}
					else if (!memcmp(Base->Request.Method, "PATCH", 5))
					{
						if (Base->Route->Callbacks.Patch)
							return Base->Route->Callbacks.Patch(Base);

						return Util::RoutePATCH(Base);
					}
					else if (!memcmp(Base->Request.Method, "DELETE", 6))
					{
						if (Base->Route->Callbacks.Delete)
							return Base->Route->Callbacks.Delete(Base);

						return Util::RouteDELETE(Base);
					}
					else if (!memcmp(Base->Request.Method, "OPTIONS", 7))
					{
						if (Base->Route->Callbacks.Options)
							return Base->Route->Callbacks.Options(Base);

						return Util::RouteOPTIONS(Base);
					}

					return Base->Error(405, "Request method \"%s\" is not allowed", Base->Request.Method);
				});
			}
			bool Server::OnDeallocate(SocketConnection* Base)
			{
				if (!Base)
					return false;

				HTTP::Connection* sBase = (HTTP::Connection*)Base;
				TH_DELETE(Connection, sBase);
				return true;
			}
			bool Server::OnDeallocateRouter(SocketRouter* Base)
			{
				if (!Base)
					return false;

				HTTP::MapRouter* sBase = (HTTP::MapRouter*)Base;
				TH_DELETE(MapRouter, sBase);
				return true;
			}
			bool Server::OnListen()
			{
				return true;
			}
			bool Server::OnUnlisten()
			{
				MapRouter* Root = (MapRouter*)Router;
				for (auto* Entry : Root->Sites)
				{
					if (!Entry->ResourceRoot.empty())
					{
						if (!Core::OS::Directory::Remove(Entry->ResourceRoot.c_str()))
							TH_ERROR("resource directory %s cannot be deleted", Entry->ResourceRoot.c_str());

						if (!Core::OS::Directory::Create(Entry->ResourceRoot.c_str()))
							TH_ERROR("resource directory %s cannot be created", Entry->ResourceRoot.c_str());
					}

					if (!Entry->Gateway.Session.DocumentRoot.empty())
						Session::InvalidateCache(Entry->Gateway.Session.DocumentRoot);
				}

				return true;
			}
			SocketConnection* Server::OnAllocate(Listener* Host, Socket* Stream)
			{
				auto Base = TH_NEW(HTTP::Connection);
				Base->Root = this;

				return Base;
			}
			SocketRouter* Server::OnAllocateRouter()
			{
				return TH_NEW(MapRouter);
			}

			Client::Client(int64_t ReadTimeout) : SocketClient(ReadTimeout)
			{
			}
			Client::~Client()
			{
			}
			Core::Async<ResponseFrame*> Client::Send(HTTP::RequestFrame* Root)
			{
				if (!Root || !Stream.IsValid())
					return Core::Async<ResponseFrame*>::Store(nullptr);

				Core::Async<ResponseFrame*> Result;
				Stage("request delivery");

				Request = *Root;
				Request.ContentState = Content::Not_Loaded;
				Done = [Result](SocketClient* Client, int Code) mutable
				{
					HTTP::Client* Base = Client->As<HTTP::Client>();
					if (Code < 0)
						Base->GetResponse()->StatusCode = -1;

					Result.Set(Base->GetResponse());
				};

				Core::Parser Content;
				if (!Request.GetHeader("Host"))
				{
					if (Context != nullptr)
					{
						if (Hostname.Port == 443)
							Request.SetHeader("Host", Hostname.Hostname.c_str());
						else
							Request.SetHeader("Host", (Hostname.Hostname + ':' + std::to_string(Hostname.Port)));
					}
					else
					{
						if (Hostname.Port == 80)
							Request.SetHeader("Host", Hostname.Hostname);
						else
							Request.SetHeader("Host", (Hostname.Hostname + ':' + std::to_string(Hostname.Port)));
					}
				}

				if (!Request.GetHeader("Accept"))
					Request.SetHeader("Accept", "*/*");

				if (!Request.GetHeader("User-Agent"))
					Request.SetHeader("User-Agent", "Lynx/1.1");

				if (!Request.GetHeader("Content-Length"))
				{
					Request.ContentLength = Request.Buffer.size();
					Request.SetHeader("Content-Length", std::to_string(Request.Buffer.size()));
				}

				if (!Request.GetHeader("Connection"))
					Request.SetHeader("Connection", "Keep-Alive");

				if (!Request.Buffer.empty())
				{
					if (!Request.GetHeader("Content-Type"))
						Request.SetHeader("Content-Type", "application/octet-stream");

					if (!Request.GetHeader("Content-Length"))
						Request.SetHeader("Content-Length", std::to_string(Request.Buffer.size()).c_str());
				}
				else if (!memcmp(Request.Method, "POST", 4) || !memcmp(Request.Method, "PUT", 3) || !memcmp(Request.Method, "PATCH", 5))
					Request.SetHeader("Content-Length", "0");

				if (!Request.Query.empty())
					Content.fAppend("%s %s?%s %s\r\n", Request.Method, Request.URI.c_str(), Request.Query.c_str(), Request.Version);
				else
					Content.fAppend("%s %s %s\r\n", Request.Method, Request.URI.c_str(), Request.Version);

				Util::ConstructHeadFull(&Request, &Response, true, &Content);
				Content.Append("\r\n");

				Response.Buffer.clear();
				Stream.WriteAsync(Content.Get(), (int64_t)Content.Size(), [this](Socket*, int64_t Size)
				{
					if (Size < 0)
						return Error("http socket write %s", (Size == -2 ? "timeout" : "error"));
					else if (Size > 0)
						return true;

					if (this->Request.Buffer.empty())
					{
						return !Stream.ReadUntilAsync("\r\n\r\n", [this](Socket*, const char* Buffer, int64_t Size)
						{
							if (Size < 0)
								return Error("http socket read %s", (Size == -2 ? "timeout" : "error"));
							else if (Size == 0)
								return Receive();

							TextAppend(this->Response.Buffer, Buffer, Size);
							return true;
						});
					}

					return !Stream.WriteAsync(this->Request.Buffer.c_str(), (int64_t)this->Request.Buffer.size(), [this](Socket*, int64_t Size)
					{
						if (Size < 0)
							return Error("http socket write %s", (Size == -2 ? "timeout" : "error"));
						else if (Size > 0)
							return true;

						return !Stream.ReadUntilAsync("\r\n\r\n", [this](Socket*, const char* Buffer, int64_t Size)
						{
							if (Size < 0)
								return Error("http socket read %s", (Size == -2 ? "timeout" : "error"));
							else if (Size == 0)
								return Receive();

							TextAppend(this->Response.Buffer, Buffer, Size);
							return true;
						});
					});
				});

				return Result;
			}
			Core::Async<ResponseFrame*> Client::Consume(int64_t MaxSize)
			{
				if (Request.ContentState == Content::Lost || Request.ContentState == Content::Empty || Request.ContentState == Content::Saved || Request.ContentState == Content::Wants_Save)
					return Core::Async<ResponseFrame*>::Store(GetResponse());

				if (Request.ContentState == Content::Corrupted || Request.ContentState == Content::Payload_Exceeded || Request.ContentState == Content::Save_Exception)
					return Core::Async<ResponseFrame*>::Store(GetResponse());

				if (Request.ContentState == Content::Cached)
					return Core::Async<ResponseFrame*>::Store(GetResponse());

				Response.Buffer.clear();

				const char* ContentType = Response.GetHeader("Content-Type");
				if (ContentType && !strncmp(ContentType, "multipart/form-data", 19))
				{
					Request.ContentState = Content::Wants_Save;
					return Core::Async<ResponseFrame*>::Store(GetResponse());
				}

				const char* TransferEncoding = Response.GetHeader("Transfer-Encoding");
				if (TransferEncoding && !Core::Parser::CaseCompare(TransferEncoding, "chunked"))
				{
					Core::Async<ResponseFrame*> Result;
					Parser* Parser = new HTTP::Parser();

					Stream.ReadAsync(MaxSize, [this, Parser, Result, MaxSize](Network::Socket* Socket, const char* Buffer, int64_t Size) mutable
					{
						if (Size > 0)
						{
							int64_t Subresult = Parser->ParseDecodeChunked((char*)Buffer, &Size);
							if (Subresult == -1)
							{
								TH_RELEASE(Parser);
								Request.ContentState = Content::Corrupted;
								Result.Set(GetResponse());

								return false;
							}
							else if (Subresult >= 0 || Subresult == -2)
							{
								if (Response.Buffer.size() < MaxSize)
									TextAppend(Response.Buffer, Buffer, Size);
							}

							return Subresult == -2;
						}

						TH_RELEASE(Parser);
						if (Size != -1)
						{
							if (Response.Buffer.size() < MaxSize)
								Request.ContentState = Content::Cached;
							else
								Request.ContentState = Content::Lost;
						}
						else
							Request.ContentState = Content::Corrupted;

						Result.Set(GetResponse());
						return true;
					});

					return Result;
				}
				else if (!Response.GetHeader("Content-Length"))
				{
					Core::Async<ResponseFrame*> Result;
					Stream.ReadAsync(MaxSize, [this, Result, MaxSize](Network::Socket* Socket, const char* Buffer, int64_t Size) mutable
					{
						if (Size <= 0)
						{
							if (Response.Buffer.size() < MaxSize)
								Request.ContentState = Content::Cached;
							else
								Request.ContentState = Content::Lost;

							Result.Set(GetResponse());
							return false;
						}

						if (Response.Buffer.size() < MaxSize)
							TextAppend(Response.Buffer, Buffer, Size);

						return true;
					});

					return Result;
				}

				const char* HContentLength = Response.GetHeader("Content-Length");
				if (!HContentLength)
				{
					Request.ContentState = Content::Corrupted;
					return Core::Async<ResponseFrame*>::Store(GetResponse());
				}

				Core::Parser HLength = HContentLength;
				if (!HLength.HasInteger())
				{
					Request.ContentState = Content::Corrupted;
					return Core::Async<ResponseFrame*>::Store(GetResponse());
				}

				int64_t Length = HLength.ToInt64();
				if (Length <= 0)
				{
					Request.ContentState = Content::Empty;
					return Core::Async<ResponseFrame*>::Store(GetResponse());
				}

				if (Length > MaxSize)
				{
					Request.ContentState = Content::Wants_Save;
					return Core::Async<ResponseFrame*>::Store(GetResponse());
				}

				Core::Async<ResponseFrame*> Result;
				Stream.ReadAsync(Length, [this, Result, MaxSize](Network::Socket* Socket, const char* Buffer, int64_t Size) mutable
				{
					if (Size <= 0)
					{
						if (Size != -1)
						{
							if (Response.Buffer.size() < MaxSize)
								Request.ContentState = Content::Cached;
							else
								Request.ContentState = Content::Lost;
						}
						else
							Request.ContentState = Content::Corrupted;

						Result.Set(GetResponse());
						return false;
					}

					if (Response.Buffer.size() < MaxSize)
						TextAppend(Response.Buffer, Buffer, Size);

					return true;
				});

				return Result;
			}
			bool Client::Receive()
			{
				ParserFrame Segment;
				Segment.Response = &Response;
				Stage("http response receive");

				Parser* Parser = new HTTP::Parser();
				Parser->OnMethodValue = Util::ParseMethodValue;
				Parser->OnPathValue = Util::ParsePathValue;
				Parser->OnQueryValue = Util::ParseQueryValue;
				Parser->OnVersion = Util::ParseVersion;
				Parser->OnStatusCode = Util::ParseStatusCode;
				Parser->OnHeaderField = Util::ParseHeaderField;
				Parser->OnHeaderValue = Util::ParseHeaderValue;
				Parser->UserPointer = &Segment;

				strcpy(Request.RemoteAddress, Stream.GetRemoteAddress().c_str());
				if (Parser->ParseResponse(Response.Buffer.data(), Response.Buffer.size(), 0) < 0)
				{
					TH_RELEASE(Parser);
					return Error("cannot parse http response");
				}

				TH_RELEASE(Parser);
				return Success(0);
			}
			RequestFrame* Client::GetRequest()
			{
				return &Request;
			}
			ResponseFrame* Client::GetResponse()
			{
				return &Response;
			}
		}
	}
}
