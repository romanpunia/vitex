#include "http.h"
#include "../bindings.h"
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
#define CSS_MESSAGE_STYLE "html{font-family:\"Helvetica Neue\",Helvetica,Arial,sans-serif;height:95%%;background-color:#101010;color:#fff;}body{display:flex;align-items:center;justify-content:center;height:100%%;}"
#define CSS_NORMAL_FONT "div{text-align:center;}"
#define CSS_SMALL_FONT "h1{font-size:16px;font-weight:normal;}"
#define HTTP_WEBSOCKET_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define HTTP_MAX_REDIRECTS 128
#define HTTP_HRM_SIZE 1024 * 1024 * 4
#define GZ_HEADER_SIZE 17
#pragma warning(push)
#pragma warning(disable: 4996)

namespace Vitex
{
	namespace Network
	{
		namespace HTTP
		{
			static bool PathTrailingCheck(const std::string_view& Path)
			{
#ifdef VI_MICROSOFT
				return !Path.empty() && (Path.back() == '/' || Path.back() == '\\');
#else
				return !Path.empty() && Path.back() == '/';
#endif
			}
			static bool ConnectionValid(Connection* Target)
			{
				return Target && Target->Root && Target->Route && Target->Route->Router;
			}
			static void TextAppend(Core::Vector<char>& Array, const std::string_view& Src)
			{
				Array.insert(Array.end(), Src.begin(), Src.end());
			}
			static void TextAssign(Core::Vector<char>& Array, const std::string_view& Src)
			{
				Array.assign(Src.begin(), Src.end());
			}
			static Core::String TextSubstring(Core::Vector<char>& Array, size_t Offset, size_t Size)
			{
				return Core::String(Array.data() + Offset, Size);
			}
			static std::string_view HeaderText(const KimvUnorderedMap& Headers, const std::string_view& Key)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Headers.find(Core::HglCast(Key));
				if (It == Headers.end())
					return "";

				if (It->second.empty())
					return "";

				return It->second.back();
			}

			MimeStatic::MimeStatic(const std::string_view& Ext, const std::string_view& T) : Extension(Ext), Type(T)
			{
			}

			void Cookie::SetExpires(int64_t Time)
			{
				Expires = Core::DateTime::FetchWebDateGMT(Time);
			}
			void Cookie::SetExpired()
			{
				SetExpires(0);
			}

			WebSocketFrame::WebSocketFrame(Socket* NewStream, void* NewUserData) : State((uint32_t)WebSocketState::Open), Tunneling((uint32_t)Tunnel::Healthy), Active(true), Deadly(false), Busy(false), Stream(NewStream), Codec(new WebCodec()), UserData(NewUserData)
			{
			}
			WebSocketFrame::~WebSocketFrame() noexcept
			{
				while (!Messages.empty())
				{
					auto& Next = Messages.front();
					Core::Memory::Deallocate(Next.Buffer);
					Messages.pop();
				}

				Core::Memory::Release(Codec);
				if (Lifetime.Destroy)
					Lifetime.Destroy(this);
			}
			Core::ExpectsSystem<size_t> WebSocketFrame::Send(const std::string_view& Buffer, WebSocketOp Opcode, WebSocketCallback&& Callback)
			{
				return Send(0, Buffer, Opcode, std::move(Callback));
			}
			Core::ExpectsSystem<size_t> WebSocketFrame::Send(uint32_t Mask, const std::string_view& Buffer, WebSocketOp Opcode, WebSocketCallback&& Callback)
			{
				Core::UMutex<std::mutex> Unique(Section);
				if (Enqueue(Mask, Buffer, Opcode, std::move(Callback)))
					return (size_t)0;

				Busy = true;
				Unique.Negate();

				uint8_t Header[14];
				size_t HeaderLength = 1;
				Header[0] = 0x80 + ((size_t)Opcode & 0xF);

				if (Buffer.size() < 126)
				{
					Header[1] = (uint8_t)Buffer.size();
					HeaderLength = 2;
				}
				else if (Buffer.size() <= 65535)
				{
					uint16_t Length = htons((uint16_t)Buffer.size());
					Header[1] = 126;
					HeaderLength = 4;
					memcpy(Header + 2, &Length, 2);
				}
				else
				{
					uint32_t Length1 = htonl((uint64_t)Buffer.size() >> 32);
					uint32_t Length2 = htonl((uint64_t)Buffer.size() & 0xFFFFFFFF);
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

				Core::String Copy = Core::String(Buffer);
				auto Status = Stream->WriteQueued(Header, HeaderLength, [this, Copy = std::move(Copy), Callback = std::move(Callback)](SocketPoll Event) mutable
				{
					if (Packet::IsDone(Event))
					{
						if (!Copy.empty())
						{
							Stream->WriteQueued((uint8_t*)Copy.data(), Copy.size(), [this, Callback = std::move(Callback)](SocketPoll Event)
							{
								if (Packet::IsDone(Event) || Packet::IsSkip(Event))
								{
									bool Ignore = IsIgnore();
									Busy = false;

									if (Callback)
										Callback(this);

									if (!Ignore)
										Dequeue();
								}
								else if (Packet::IsError(Event))
								{
									Tunneling = (uint32_t)Tunnel::Gone;
									Busy = false;
									if (Callback)
										Callback(this);
								}
							});
						}
						else
						{
							bool Ignore = IsIgnore();
							Busy = false;

							if (Callback)
								Callback(this);

							if (!Ignore)
								Dequeue();
						}
					}
					else if (Packet::IsError(Event))
					{
						Tunneling = (uint32_t)Tunnel::Gone;
						Busy = false;
						if (Callback)
							Callback(this);
					}
					else if (Packet::IsSkip(Event))
					{
						bool Ignore = IsIgnore();
						Busy = false;

						if (Callback)
							Callback(this);

						if (!Ignore)
							Dequeue();
					}
				});
				if (Status)
					return *Status;
				else if (Status.Error() == std::errc::operation_would_block)
					return (size_t)0;

				return Core::SystemException("ws send error", std::move(Status.Error()));
			}
			Core::ExpectsSystem<void> WebSocketFrame::SendClose(WebSocketCallback&& Callback)
			{
				if (Deadly)
				{
					if (Callback)
						Callback(this);
					return Core::SystemException("ws connection is closed: fd " + Core::ToString(Stream->GetFd()), std::make_error_condition(std::errc::operation_not_permitted));
				}

				if (State == (uint32_t)WebSocketState::Close || Tunneling != (uint32_t)Tunnel::Healthy)
				{
					if (Callback)
						Callback(this);
					return Core::Expectation::Met;
				}

				if (!Active)
				{
					if (Callback)
						Callback(this);
					return Core::SystemException("ws connection is closing: fd " + Core::ToString(Stream->GetFd()), std::make_error_condition(std::errc::operation_not_permitted));
				}

				Finalize();
				auto Status = Send("", WebSocketOp::Close, std::move(Callback));
				if (!Status)
					return Status.Error();

				return Core::Expectation::Met;
			}
			void WebSocketFrame::Dequeue()
			{
				Core::UMutex<std::mutex> Unique(Section);
				if (!IsWriteable() || Messages.empty())
					return;

				Message Next = std::move(Messages.front());
				Messages.pop();
				Unique.Negate();

				Send(Next.Mask, std::string_view(Next.Buffer, Next.Size), Next.Opcode, std::move(Next.Callback));
				Core::Memory::Deallocate(Next.Buffer);
			}
			void WebSocketFrame::Finalize()
			{
				if (Tunneling == (uint32_t)Tunnel::Healthy)
					State = (uint32_t)WebSocketState::Close;
			}
			void WebSocketFrame::Next()
			{
				Core::Codefer(std::bind(&WebSocketFrame::Update, this));
			}
			void WebSocketFrame::Update()
			{
				Core::UMutex<std::mutex> Unique(Section);
			Retry:
				if (State == (uint32_t)WebSocketState::Close || Tunneling != (uint32_t)Tunnel::Healthy)
				{
					if (Tunneling != (uint32_t)Tunnel::Gone)
						Tunneling = (uint32_t)Tunnel::Closing;
					
					if (BeforeDisconnect)
					{
						WebSocketCallback Callback = std::move(BeforeDisconnect);
						BeforeDisconnect = nullptr;
						Unique.Negate();
						Callback(this);
					}
					else if (!Disconnect)
					{
						bool Successful = (Tunneling == (uint32_t)Tunnel::Closing);
						Tunneling = (uint32_t)Tunnel::Gone;
						Active = false;
						Unique.Negate();
						if (Lifetime.Close)
							Lifetime.Close(this, Successful);
					}
					else
					{
						WebSocketCallback Callback = std::move(Disconnect);
						Disconnect = nullptr;
						Receive = nullptr;
						Unique.Negate();
						Callback(this);
					}
				}
				else if (State == (uint32_t)WebSocketState::Receive)
				{
					if (Lifetime.Dead && Lifetime.Dead(this))
					{
						Finalize();
						goto Retry;
					}

					Multiplexer::Get()->WhenReadable(Stream, [this](SocketPoll Event)
					{
						bool IsDone = Packet::IsDone(Event);
						if (!IsDone && !Packet::IsError(Event))
							return true;

						State = (uint32_t)(IsDone ? WebSocketState::Process : WebSocketState::Close);
						Next();
						return true;
					});
				}
				else if (State == (uint32_t)WebSocketState::Process)
				{
					uint8_t Buffer[Core::BLOB_SIZE];
					while (true)
					{
						auto Size = Stream->Read(Buffer, sizeof(Buffer));
						if (Size)
						{
							Codec->ParseFrame(Buffer, *Size);
							continue;
						}

						if (Size.Error() == std::errc::operation_would_block)
						{
							State = (uint32_t)WebSocketState::Receive;
							break;
						}
						else
						{
							Finalize();
							goto Retry;
						}
					}

					WebSocketOp Opcode;
					if (!Codec->GetFrame(&Opcode, &Codec->Data))
						goto Retry;

					State = (uint32_t)WebSocketState::Process;
					if (Opcode == WebSocketOp::Text || Opcode == WebSocketOp::Binary)
					{
						VI_DEBUG("[websocket] sock %i frame data: %.*s", (int)Stream->GetFd(), (int)Codec->Data.size(), Codec->Data.data());
						if (Receive)
						{
							Unique.Negate();
							if (!Receive(this, Opcode, std::string_view(Codec->Data.data(), Codec->Data.size())))
								Next();
						}
					}
					else if (Opcode == WebSocketOp::Ping)
					{
						VI_DEBUG("[websocket] sock %i frame ping", (int)Stream->GetFd());
						Unique.Negate();
						if (!Receive || !Receive(this, Opcode, ""))
							Send("", WebSocketOp::Pong, [this](WebSocketFrame*) { Next(); });
					}
					else if (Opcode == WebSocketOp::Close)
					{
						VI_DEBUG("[websocket] sock %i frame close", (int)Stream->GetFd());
						Unique.Negate();
						if (!Receive || !Receive(this, Opcode, ""))
							SendClose(std::bind(&WebSocketFrame::Next, std::placeholders::_1));
					}
					else if (Receive)
					{
						Unique.Negate();
						if (!Receive(this, Opcode, ""))
							Next();
					}
					else
						goto Retry;
				}
				else if (State == (uint32_t)WebSocketState::Open)
				{
					if (Connect || Receive || Disconnect)
					{
						State = (uint32_t)WebSocketState::Receive;
						if (!Connect)
							goto Retry;

						WebSocketCallback Callback = std::move(Connect);
                        Connect = nullptr;
						Unique.Negate();
						Callback(this);
					}
					else
					{
						Unique.Negate();
						SendClose(std::bind(&WebSocketFrame::Next, std::placeholders::_1));
					}
				}
			}
			bool WebSocketFrame::IsFinished()
			{
				return !Active;
			}
			bool WebSocketFrame::IsIgnore()
			{
				return Deadly || State == (uint32_t)WebSocketState::Close || Tunneling != (uint32_t)Tunnel::Healthy;
			}
			bool WebSocketFrame::IsWriteable()
			{
				return !Busy && !Stream->IsAwaitingWriteable();
			}
			Socket* WebSocketFrame::GetStream()
			{
				return Stream;
			}
			Connection* WebSocketFrame::GetConnection()
			{
				return (Connection*)UserData;
			}
			Client* WebSocketFrame::GetClient()
			{
				return (Client*)UserData;
			}
			bool WebSocketFrame::Enqueue(uint32_t Mask, const std::string_view& Buffer, WebSocketOp Opcode, WebSocketCallback&& Callback)
			{
				if (IsWriteable())
					return false;

				Message Next;
				Next.Mask = Mask;
				Next.Buffer = (Buffer.empty() ? nullptr : Core::Memory::Allocate<char>(sizeof(char) * Buffer.size()));
				Next.Size = Buffer.size();
				Next.Opcode = Opcode;
				Next.Callback = Callback;
				if (Next.Buffer != nullptr)
					memcpy(Next.Buffer, Buffer.data(), sizeof(char) * Buffer.size());

				Messages.emplace(std::move(Next));
				return true;
			}

			RouterGroup::RouterGroup(const std::string_view& NewMatch, RouteMode NewMode) noexcept : Match(NewMatch), Mode(NewMode)
			{
			}
			RouterGroup::~RouterGroup() noexcept
			{
				for (auto* Entry : Routes)
					Core::Memory::Release(Entry);
				Routes.clear();
			}

			RouterEntry* RouterEntry::From(const RouterEntry& Other, const Compute::RegexSource& Source)
			{
				RouterEntry* Route = new RouterEntry(Other);
				Route->Location = Source;
				return Route;
			}

			MapRouter::MapRouter() : Base(new RouterEntry())
			{
				Base->Location = Compute::RegexSource("/");
				Base->Router = this;
			}
			MapRouter::~MapRouter()
			{
				if (Callbacks.OnDestroy != nullptr)
					Callbacks.OnDestroy(this);

				for (auto& Item : Groups)
					Core::Memory::Release(Item);

				Groups.clear();
				Core::Memory::Release(Base);
			}
			void MapRouter::Sort()
			{
				VI_SORT(Groups.begin(), Groups.end(), [](const RouterGroup* A, const RouterGroup* B)
				{
					return A->Match.size() > B->Match.size();
				});

				for (auto& Group : Groups)
				{
					static auto Comparator = [](const RouterEntry* A, const RouterEntry* B)
					{
						if (A->Location.GetRegex().empty())
							return false;

						if (B->Location.GetRegex().empty())
							return true;

						if (A->Level > B->Level)
							return true;
						else if (A->Level < B->Level)
							return false;

						bool fA = A->Location.IsSimple(), fB = B->Location.IsSimple();
						if (fA && !fB)
							return false;
						else if (!fA && fB)
							return true;

						return A->Location.GetRegex().size() > B->Location.GetRegex().size();
					};
					VI_SORT(Group->Routes.begin(), Group->Routes.end(), Comparator);
				}
			}
			RouterGroup* MapRouter::Group(const std::string_view& Match, RouteMode Mode)
			{
				for (auto& Group : Groups)
				{
					if (Group->Match == Match && Group->Mode == Mode)
						return Group;
				}

				auto* Result = new RouterGroup(Match, Mode);
				Groups.emplace_back(Result);

				return Result;
			}
			RouterEntry* MapRouter::Route(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, bool InheritProps)
			{
				if (Pattern.empty() || Pattern == "/")
					return Base;

				HTTP::RouterGroup* Source = nullptr;
				for (auto& Group : Groups)
				{
					if (Group->Match != Match || Group->Mode != Mode)
						continue;

					Source = Group;
					for (auto* Entry : Group->Routes)
					{
						if (Entry->Location.GetRegex() == Pattern)
							return Entry;
					}
				}

				if (!Source)
				{
					auto* Result = new RouterGroup(Match, Mode);
					Groups.emplace_back(Result);
					Source = Groups.back();
				}

				if (!InheritProps)
					return Route(Pattern, Source, nullptr);

				HTTP::RouterEntry* From = Base;
				Compute::RegexResult Result;
				Core::String Src(Pattern);
				Core::Stringify::ToLower(Src);

				for (auto& Group : Groups)
				{
					for (auto* Entry : Group->Routes)
					{
						Core::String Dest(Entry->Location.GetRegex());
						Core::Stringify::ToLower(Dest);

						if (Core::Stringify::StartsWith(Dest, "...") && Core::Stringify::EndsWith(Dest, "..."))
							continue;

						if (Core::Stringify::Find(Src, Dest).Found || Compute::Regex::Match(&Entry->Location, Result, Src))
						{
							From = Entry;
							break;
						}
					}
				}

				return Route(Pattern, Source, From);
			}
			RouterEntry* MapRouter::Route(const std::string_view& Pattern, RouterGroup* Group, RouterEntry* From)
			{
				VI_ASSERT(Group != nullptr, "group should be set");
				if (From != nullptr)
				{
					HTTP::RouterEntry* Result = HTTP::RouterEntry::From(*From, Compute::RegexSource(Pattern));
					Group->Routes.push_back(Result);
					return Result;
				}

				HTTP::RouterEntry* Result = new HTTP::RouterEntry();
				Result->Location = Compute::RegexSource(Pattern);
				Result->Router = this;
				Group->Routes.push_back(Result);
				return Result;
			}
			bool MapRouter::Remove(RouterEntry* Source)
			{
				VI_ASSERT(Source != nullptr, "source should be set");
				for (auto& Group : Groups)
				{
					auto It = std::find(Group->Routes.begin(), Group->Routes.end(), Source);
					if (It != Group->Routes.end())
					{
						Core::Memory::Release(*It);
						Group->Routes.erase(It);
						return true;
					}
				}

				return false;
			}
			bool MapRouter::Get(const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				return Get("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::Get(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Get = std::move(Callback);
				return true;
			}
			bool MapRouter::Post(const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				return Post("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::Post(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Post = std::move(Callback);
				return true;
			}
			bool MapRouter::Put(const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				return Put("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::Put(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Put = std::move(Callback);
				return true;
			}
			bool MapRouter::Patch(const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				return Patch("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::Patch(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Patch = std::move(Callback);
				return true;
			}
			bool MapRouter::Delete(const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				return Delete("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::Delete(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Delete = std::move(Callback);
				return true;
			}
			bool MapRouter::Options(const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				return Options("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::Options(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Options = std::move(Callback);
				return true;
			}
			bool MapRouter::Access(const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				return Access("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::Access(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Access = std::move(Callback);
				return true;
			}
			bool MapRouter::Headers(const std::string_view& Pattern, HeaderCallback&& Callback)
			{
				return Headers("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::Headers(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, HeaderCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Headers = std::move(Callback);
				return true;
			}
			bool MapRouter::Authorize(const std::string_view& Pattern, AuthorizeCallback&& Callback)
			{
				return Authorize("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::Authorize(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, AuthorizeCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Authorize = std::move(Callback);
				return true;
			}
			bool MapRouter::WebSocketInitiate(const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				return WebSocketInitiate("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::WebSocketInitiate(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.WebSocket.Initiate = std::move(Callback);
				return true;
			}
			bool MapRouter::WebSocketConnect(const std::string_view& Pattern, WebSocketCallback&& Callback)
			{
				return WebSocketConnect("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::WebSocketConnect(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, WebSocketCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.WebSocket.Connect = std::move(Callback);
				return true;
			}
			bool MapRouter::WebSocketDisconnect(const std::string_view& Pattern, WebSocketCallback&& Callback)
			{
				return WebSocketDisconnect("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::WebSocketDisconnect(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, WebSocketCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.WebSocket.Disconnect = std::move(Callback);
				return true;
			}
			bool MapRouter::WebSocketReceive(const std::string_view& Pattern, WebSocketReadCallback&& Callback)
			{
				return WebSocketReceive("", RouteMode::Start, Pattern, std::move(Callback));
			}
			bool MapRouter::WebSocketReceive(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, WebSocketReadCallback&& Callback)
			{
				HTTP::RouterEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.WebSocket.Receive = std::move(Callback);
				return true;
			}

			Core::String& Resource::PutHeader(const std::string_view& Label, const std::string_view& Value)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				Core::Vector<Core::String>* Range;
				auto It = Headers.find(Core::HglCast(Label));
				if (It != Headers.end())
					Range = &It->second;
				else
					Range = &Headers[Core::String(Label)];

				Range->push_back(Core::String(Value));
				return Range->back();
			}
			Core::String& Resource::SetHeader(const std::string_view& Label, const std::string_view& Value)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				Core::Vector<Core::String>* Range;
				auto It = Headers.find(Core::HglCast(Label));
				if (It != Headers.end())
					Range = &It->second;
				else
					Range = &Headers[Core::String(Label)];

				Range->clear();
				Range->push_back(Core::String(Value));
				return Range->back();
			}
			Core::String Resource::ComposeHeader(const std::string_view& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return Core::String();

				Core::String Result;
				for (auto& Item : It->second)
				{
					Result.append(Item);
					Result.append(1, ',');
				}

				return (Result.empty() ? Result : Result.substr(0, Result.size() - 1));
			}
			Core::Vector<Core::String>* Resource::GetHeaderRanges(const std::string_view& Label)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				return It != Headers.end() ? &It->second : &Headers[Core::String(Label)];
			}
			Core::String* Resource::GetHeaderBlob(const std::string_view& Label)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				auto& Result = It->second.back();
				return &Result;
			}
			std::string_view Resource::GetHeader(const std::string_view& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return "";

				if (It->second.empty())
					return "";

				return It->second.back();
			}
			const Core::String& Resource::GetInMemoryContents() const
			{
				return Path;
			}

			RequestFrame::RequestFrame()
			{
				memset(Method, 0, sizeof(Method));
				memset(Version, 0, sizeof(Version));
				strcpy(Method, "GET");
				strcpy(Version, "HTTP/1.1");
			}
			void RequestFrame::SetMethod(const std::string_view& Value)
			{
				memset(Method, 0, sizeof(Method));
				memcpy((void*)Method, (void*)Value.data(), std::min<size_t>(Value.size(), sizeof(Method)));
			}
			void RequestFrame::SetVersion(uint32_t Major, uint32_t Minor)
			{
				Core::String Value = "HTTP/" + Core::ToString(Major) + '.' + Core::ToString(Minor);
				memset(Version, 0, sizeof(Version));
				memcpy((void*)Version, (void*)Value.c_str(), std::min<size_t>(Value.size(), sizeof(Version)));
			}
			void RequestFrame::Cleanup()
			{
				memset(Method, 0, sizeof(Method));
				memset(Version, 0, sizeof(Version));
				User.Type = Auth::Unverified;
				User.Token.clear();
				Content.Cleanup();
				Headers.clear();
				Cookies.clear();
				Query.clear();
				Path.clear();
				Location.clear();
				Referrer.clear();
			}
			Core::String& RequestFrame::PutHeader(const std::string_view& Label, const std::string_view& Value)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				Core::Vector<Core::String>* Range;
				auto It = Headers.find(Core::HglCast(Label));
				if (It != Headers.end())
					Range = &It->second;
				else
					Range = &Headers[Core::String(Label)];

				Range->push_back(Core::String(Value));
				return Range->back();
			}
			Core::String& RequestFrame::SetHeader(const std::string_view& Label, const std::string_view& Value)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				Core::Vector<Core::String>* Range;
				auto It = Headers.find(Core::HglCast(Label));
				if (It != Headers.end())
					Range = &It->second;
				else
					Range = &Headers[Core::String(Label)];

				Range->clear();
				Range->push_back(Core::String(Value));
				return Range->back();
			}
			Core::String RequestFrame::ComposeHeader(const std::string_view& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return Core::String();

				Core::String Result;
				for (auto& Item : It->second)
				{
					Result.append(Item);
					Result.append(1, ',');
				}

				return (Result.empty() ? Result : Result.substr(0, Result.size() - 1));
			}
			Core::Vector<Core::String>* RequestFrame::GetHeaderRanges(const std::string_view& Label)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				return It != Headers.end() ? &It->second : &Headers[Core::String(Label)];
			}
			Core::String* RequestFrame::GetHeaderBlob(const std::string_view& Label)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				auto& Result = It->second.back();
				return &Result;
			}
			std::string_view RequestFrame::GetHeader(const std::string_view& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return "";

				if (It->second.empty())
					return "";

				return It->second.back();
			}
			Core::Vector<Core::String>* RequestFrame::GetCookieRanges(const std::string_view& Key)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Cookies.find(Core::HglCast(Key));
				return It != Cookies.end() ? &It->second : &Cookies[Core::String(Key)];
			}
			Core::String* RequestFrame::GetCookieBlob(const std::string_view& Key)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Cookies.find(Core::HglCast(Key));
				if (It == Cookies.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				auto& Result = It->second.back();
				return &Result;
			}
			std::string_view RequestFrame::GetCookie(const std::string_view& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Cookies.find(Core::HglCast(Key));
				if (It == Cookies.end())
					return "";

				if (It->second.empty())
					return "";

				return It->second.back();
			}
			Core::Vector<std::pair<size_t, size_t>> RequestFrame::GetRanges() const
			{
				auto It = Headers.find("Range");
				if (It == Headers.end())
					return Core::Vector<std::pair<size_t, size_t>>();

				Core::Vector<std::pair<size_t, size_t>> Ranges;
				for (auto& Item : It->second)
				{
					Core::TextSettle Result = Core::Stringify::Find(Item, '-');
					if (!Result.Found)
						continue;

					size_t ContentStart = -1, ContentEnd = -1;
					if (Result.Start > 0)
					{
						const char* Left = Item.c_str() + Result.Start - 1;
						size_t LeftSize = (size_t)(isdigit(*Left) > 0);
						if (LeftSize > 0)
						{
							while (isdigit(*(Left - 1)) && LeftSize <= Result.Start - 1)
							{
								--Left;
								++LeftSize;
							}

							if (LeftSize > 0)
							{
								auto From = Core::FromString<size_t>(Core::String(Left, LeftSize));
								if (From)
									ContentStart = *From;
							}
						}
					}

					if (Result.End < Item.size())
					{
						size_t RightSize = 0;
						const char* Right = Item.c_str() + Result.Start + 1;
						while (Right[RightSize] != '\0' && isdigit(Right[RightSize]))
							++RightSize;

						if (RightSize > 0)
						{
							auto To = Core::FromString<size_t>(Core::String(Right, RightSize));
							if (To)
								ContentEnd = *To;
						}
					}

					if (ContentStart != -1 || ContentEnd != -1)
						Ranges.emplace_back(std::make_pair(ContentStart, ContentEnd));
				}

				return Ranges;
			}
			std::pair<size_t, size_t> RequestFrame::GetRange(Core::Vector<std::pair<size_t, size_t>>::iterator Range, size_t ContentLength) const
			{
				if (Range->first == -1 && Range->second == -1)
					return std::make_pair(0, ContentLength);

				if (Range->first == -1)
				{
					if (Range->second > ContentLength)
						Range->second = 0;

					Range->first = ContentLength - Range->second;
					Range->second = ContentLength;
				}
				else if (Range->first > ContentLength)
					Range->first = ContentLength;

				if (Range->second == -1)
					Range->second = ContentLength;
				else if (Range->second > ContentLength)
					Range->second = ContentLength;

				if (Range->first > Range->second)
					Range->first = Range->second;

				return std::make_pair(Range->first, Range->second - Range->first);
			}

			ResponseFrame::ResponseFrame() : StatusCode(-1), Error(false)
			{
			}
			void ResponseFrame::SetCookie(const Cookie& Value)
			{
				for (auto& Cookie : Cookies)
				{
					if (Core::Stringify::CaseEquals(Cookie.Name.c_str(), Value.Name.c_str()))
					{
						Cookie = Value;
						return;
					}
				}

				Cookies.push_back(Value);
			}
			void ResponseFrame::SetCookie(Cookie&& Value)
			{
				for (auto& Cookie : Cookies)
				{
					if (Core::Stringify::CaseEquals(Cookie.Name.c_str(), Value.Name.c_str()))
					{
						Cookie = std::move(Value);
						return;
					}
				}

				Cookies.emplace_back(std::move(Value));
			}
			void ResponseFrame::Cleanup()
			{
				StatusCode = -1;
				Error = false;
				Cookies.clear();
				Headers.clear();
				Content.Cleanup();
			}
			Core::String& ResponseFrame::PutHeader(const std::string_view& Label, const std::string_view& Value)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				Core::Vector<Core::String>* Range;
				auto It = Headers.find(Core::HglCast(Label));
				if (It != Headers.end())
					Range = &It->second;
				else
					Range = &Headers[Core::String(Label)];

				Range->push_back(Core::String(Value));
				return Range->back();
			}
			Core::String& ResponseFrame::SetHeader(const std::string_view& Label, const std::string_view& Value)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				Core::Vector<Core::String>* Range;
				auto It = Headers.find(Core::HglCast(Label));
				if (It != Headers.end())
					Range = &It->second;
				else
					Range = &Headers[Core::String(Label)];

				Range->clear();
				Range->push_back(Core::String(Value));
				return Range->back();
			}
			Core::String ResponseFrame::ComposeHeader(const std::string_view& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return Core::String();

				Core::String Result;
				for (auto& Item : It->second)
				{
					Result.append(Item);
					Result.append(1, ',');
				}

				return (Result.empty() ? Result : Result.substr(0, Result.size() - 1));
			}
			Core::Vector<Core::String>* ResponseFrame::GetHeaderRanges(const std::string_view& Label)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				return It != Headers.end() ? &It->second : &Headers[Core::String(Label)];
			}
			Core::String* ResponseFrame::GetHeaderBlob(const std::string_view& Label)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				auto& Result = It->second.back();
				return &Result;
			}
			std::string_view ResponseFrame::GetHeader(const std::string_view& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return "";

				if (It->second.empty())
					return "";

				return It->second.back();
			}
			Cookie* ResponseFrame::GetCookie(const std::string_view& Key)
			{
				for (size_t i = 0; i < Cookies.size(); i++)
				{
					Cookie* Result = &Cookies[i];
					if (Core::Stringify::CaseEquals(Result->Name.c_str(), Key))
						return Result;
				}

				return nullptr;
			}
			bool ResponseFrame::IsUndefined() const
			{
				return StatusCode <= 0;
			}
			bool ResponseFrame::IsOK() const
			{
				return StatusCode >= 200 && StatusCode < 400;
			}

			ContentFrame::ContentFrame() : Length(0), Offset(0), Prefetch(0), Exceeds(false), Limited(false)
			{
			}
			void ContentFrame::Append(const std::string_view& Text)
			{
				TextAppend(Data, Text);
			}
			void ContentFrame::Assign(const std::string_view& Text)
			{
				TextAssign(Data, Text);
			}
			void ContentFrame::Prepare(const KimvUnorderedMap& Headers, const uint8_t* Buffer, size_t Size)
			{
				auto ContentLength = HeaderText(Headers, "Content-Length");
				Limited = !ContentLength.empty();
				if (Limited)
					Length = strtoull(ContentLength.data(), nullptr, 10);

				Offset = Prefetch = Buffer ? Size : 0;
				if (Offset > 0)
					TextAssign(Data, std::string_view((char*)Buffer, Size));
				else
					Data.clear();

				if (Limited)
					return;

				auto TransferEncoding = HeaderText(Headers, "Transfer-Encoding");
				if (!Core::Stringify::CaseEquals(TransferEncoding, "chunked"))
					Limited = true;
			}
			void ContentFrame::Finalize()
			{
				Length = Offset;
				Limited = true;
			}
			void ContentFrame::Cleanup()
			{
				if (!Resources.empty())
				{
					Core::Vector<Core::String> Paths;
					Paths.reserve(Resources.size());
					for (auto& Item : Resources)
					{
						if (!Item.IsInMemory)
							Paths.push_back(Item.Path);
					}

					if (!Paths.empty())
					{
						Core::Cospawn([Paths = std::move(Paths)]() mutable
						{
							for (auto& Path : Paths)
								Core::OS::File::Remove(Path.c_str());
						});
					}
				}

				Data.clear();
				Resources.clear();
				Length = 0;
				Offset = 0;
				Prefetch = 0;
				Limited = false;
				Exceeds = false;
			}
			Core::ExpectsParser<Core::Schema*> ContentFrame::GetJSON() const
			{
				return Core::Schema::FromJSON(GetText());
			}
			Core::ExpectsParser<Core::Schema*> ContentFrame::GetXML() const
			{
				return Core::Schema::FromXML(GetText());
			}
			Core::String ContentFrame::GetText() const
			{
				return Core::String(Data.data(), Data.size());
			}
			bool ContentFrame::IsFinalized() const
			{
				if (Prefetch > 0)
					return false;

				if (!Limited)
					return Offset >= Length && Length > 0;

				return Offset >= Length || Data.size() >= Length;
			}

			FetchFrame::FetchFrame() : Timeout(10000), VerifyPeers(9), MaxSize(PAYLOAD_SIZE)
			{
			}
			void FetchFrame::Cleanup()
			{
				Content.Cleanup();
				Headers.clear();
				Cookies.clear();
			}
			Core::String& FetchFrame::PutHeader(const std::string_view& Label, const std::string_view& Value)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				Core::Vector<Core::String>* Range;
				auto It = Headers.find(Core::HglCast(Label));
				if (It != Headers.end())
					Range = &It->second;
				else
					Range = &Headers[Core::String(Label)];

				Range->push_back(Core::String(Value));
				return Range->back();
			}
			Core::String& FetchFrame::SetHeader(const std::string_view& Label, const std::string_view& Value)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				Core::Vector<Core::String>* Range;
				auto It = Headers.find(Core::HglCast(Label));
				if (It != Headers.end())
					Range = &It->second;
				else
					Range = &Headers[Core::String(Label)];

				Range->clear();
				Range->push_back(Core::String(Value));
				return Range->back();
			}
			Core::String FetchFrame::ComposeHeader(const std::string_view& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return Core::String();

				Core::String Result;
				for (auto& Item : It->second)
				{
					Result.append(Item);
					Result.append(1, ',');
				}

				return (Result.empty() ? Result : Result.substr(0, Result.size() - 1));
			}
			Core::Vector<Core::String>* FetchFrame::GetHeaderRanges(const std::string_view& Label)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				return It != Headers.end() ? &It->second : &Headers[Core::String(Label)];
			}
			Core::String* FetchFrame::GetHeaderBlob(const std::string_view& Label)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				auto& Result = It->second.back();
				return &Result;
			}
			std::string_view FetchFrame::GetHeader(const std::string_view& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Core::HglCast(Label));
				if (It == Headers.end())
					return "";

				if (It->second.empty())
					return "";

				return It->second.back();
			}
			Core::Vector<Core::String>* FetchFrame::GetCookieRanges(const std::string_view& Key)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Cookies.find(Core::HglCast(Key));
				return It != Cookies.end() ? &It->second : &Cookies[Core::String(Key)];
			}
			Core::String* FetchFrame::GetCookieBlob(const std::string_view& Key)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Cookies.find(Core::HglCast(Key));
				if (It == Cookies.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				auto& Result = It->second.back();
				return &Result;
			}
			std::string_view FetchFrame::GetCookie(const std::string_view& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Cookies.find(Core::HglCast(Key));
				if (It == Cookies.end())
					return "";

				if (It->second.empty())
					return "";

				return It->second.back();
			}
			Core::Vector<std::pair<size_t, size_t>> FetchFrame::GetRanges() const
			{
				auto It = Headers.find("Range");
				if (It == Headers.end())
					return Core::Vector<std::pair<size_t, size_t>>();

				Core::Vector<std::pair<size_t, size_t>> Ranges;
				for (auto& Item : It->second)
				{
					Core::TextSettle Result = Core::Stringify::Find(Item, '-');
					if (!Result.Found)
						continue;

					size_t ContentStart = -1, ContentEnd = -1;
					if (Result.Start > 0)
					{
						const char* Left = Item.c_str() + Result.Start - 1;
						size_t LeftSize = (size_t)(isdigit(*Left) > 0);
						if (LeftSize > 0)
						{
							while (isdigit(*(Left - 1)) && LeftSize <= Result.Start - 1)
							{
								--Left;
								++LeftSize;
							}

							if (LeftSize > 0)
							{
								auto From = Core::FromString<size_t>(Core::String(Left, LeftSize));
								if (From)
									ContentStart = *From;
							}
						}
					}

					if (Result.End < Item.size())
					{
						size_t RightSize = 0;
						const char* Right = Item.c_str() + Result.Start + 1;
						while (Right[RightSize] != '\0' && isdigit(Right[RightSize]))
							++RightSize;

						if (RightSize > 0)
						{
							auto To = Core::FromString<size_t>(Core::String(Right, RightSize));
							if (To)
								ContentEnd = *To;
						}
					}

					if (ContentStart != -1 || ContentEnd != -1)
						Ranges.emplace_back(std::make_pair(ContentStart, ContentEnd));
				}

				return Ranges;
			}
			std::pair<size_t, size_t> FetchFrame::GetRange(Core::Vector<std::pair<size_t, size_t>>::iterator Range, size_t ContentLength) const
			{
				if (Range->first == -1 && Range->second == -1)
					return std::make_pair(0, ContentLength);

				if (Range->first == -1)
				{
					if (Range->second > ContentLength)
						Range->second = 0;

					Range->first = ContentLength - Range->second;
					Range->second = ContentLength;
				}
				else if (Range->first > ContentLength)
					Range->first = ContentLength;

				if (Range->second == -1)
					Range->second = ContentLength;
				else if (Range->second > ContentLength)
					Range->second = ContentLength;

				if (Range->first > Range->second)
					Range->first = Range->second;

				return std::make_pair(Range->first, Range->second - Range->first);
			}

			Connection::Connection(Server* Source) noexcept : Resolver(new HTTP::Parser()), Root(Source)
			{
			}
			Connection::~Connection() noexcept
			{
				Core::Memory::Release(Resolver);
				Core::Memory::Release(WebSocket);
			}
			void Connection::Reset(bool Fully)
			{
				VI_ASSERT(!Route || (Route->Router && Route->Router->Base), "router should be valid");
				if (!Fully)
					Info.Abort = (Info.Abort || Response.StatusCode <= 0);

				if (Route != nullptr)
					Route = Route->Router->Base;
				Request.Cleanup();
				Response.Cleanup();
				SocketConnection::Reset(Fully);
			}
			bool Connection::ComposeResponse(bool ApplyErrorResponse, bool ApplyBodyInlining, HeadersCallback&& Callback)
			{
				VI_ASSERT(ConnectionValid(this), "connection should be valid");
				VI_ASSERT(Callback != nullptr, "callback should be set");
			Retry:
				auto* Content = HrmCache::Get()->Pop();
				auto StatusText = Utils::StatusMessage(Response.StatusCode);
				Content->append(Request.Version).append(" ");
				Content->append(Core::ToString(Response.StatusCode)).append(" ");
				Content->append(StatusText).append("\r\n");

				std::string_view ContentType;
				if (ApplyErrorResponse)
				{
					char Buffer[Core::BLOB_SIZE];
					int Size = snprintf(Buffer, sizeof(Buffer), "<html><head><title>%d %s</title><style>" CSS_MESSAGE_STYLE "%s</style></head><body><div><h1>%d %s</h1></div></body></html>\n", Response.StatusCode, StatusText.data(), Info.Message.size() <= 128 ? CSS_NORMAL_FONT : CSS_SMALL_FONT, Response.StatusCode, Info.Message.empty() ? StatusText.data() : Info.Message.c_str());
					if (Size >= 0)
					{
						Response.Content.Assign(std::string_view(Buffer, (size_t)Size));
						Response.SetHeader("Content-Length", Core::ToString(Size));
						ContentType = Response.GetHeader("Content-Type");
						if (ContentType.empty())
							ContentType = Response.SetHeader("Content-Type", "text/html; charset=" + Route->CharSet).c_str();
					}
				}

				if (Response.GetHeader("Date").empty())
				{
					char Buffer[64];
					Core::DateTime::FetchWebDateGMT(Buffer, sizeof(Buffer), Info.Start / 1000);
					Content->append("Date: ").append(Buffer).append("\r\n");
				}

				if (Response.GetHeader("Connection").empty())
					Content->append(Utils::ConnectionResolve(this));

				if (Response.GetHeader("Accept-Ranges").empty())
					Content->append("Accept-Ranges: bytes\r\n", 22);

				Core::Option<Core::String> Boundary = Core::Optional::None;
				if (ContentType.empty())
				{
					ContentType = Response.GetHeader("Content-Type");
					if (ContentType.empty())
					{
						ContentType = Utils::ContentType(Request.Path, &Route->MimeTypes);
						if (!Request.GetHeader("Range").empty())
						{
							Boundary = Parsing::ParseMultipartDataBoundary();
							Content->append("Content-Type: multipart/byteranges; boundary=").append(*Boundary).append("; charset=").append(Route->CharSet).append("\r\n");
						}
						else
							Content->append("Content-Type: ").append(ContentType).append("; charset=").append(Route->CharSet).append("\r\n");
					}
				}

				if (!Response.Content.Data.empty())
				{
#ifdef VI_ZLIB
					bool Deflate = false, Gzip = false;
					if (Resources::ResourceCompressed(this, Response.Content.Data.size()))
					{
						auto AcceptEncoding = Request.GetHeader("Accept-Encoding");
						if (!AcceptEncoding.empty())
						{
							Deflate = AcceptEncoding.find("deflate") != std::string::npos;
							Gzip = AcceptEncoding.find("gzip") != std::string::npos;
						}

						if (!AcceptEncoding.empty() && (Deflate || Gzip))
						{
							z_stream fStream;
							fStream.zalloc = Z_NULL;
							fStream.zfree = Z_NULL;
							fStream.opaque = Z_NULL;
							fStream.avail_in = (uInt)Response.Content.Data.size();
							fStream.next_in = (Bytef*)Response.Content.Data.data();

							if (deflateInit2(&fStream, Route->Compression.QualityLevel, Z_DEFLATED, (Gzip ? 15 | 16 : 15), Route->Compression.MemoryLevel, (int)Route->Compression.Tune) == Z_OK)
							{
								Core::String Buffer(Response.Content.Data.size(), '\0');
								fStream.avail_out = (uInt)Buffer.size();
								fStream.next_out = (Bytef*)Buffer.c_str();
								bool Compress = (deflate(&fStream, Z_FINISH) == Z_STREAM_END);
								bool Flush = (deflateEnd(&fStream) == Z_OK);

								if (Compress && Flush)
								{
									Response.Content.Assign(std::string_view(Buffer.c_str(), (size_t)fStream.total_out));
									if (Response.GetHeader("Content-Encoding").empty())
									{
										if (Gzip)
											Content->append("Content-Encoding: gzip\r\n", 24);
										else
											Content->append("Content-Encoding: deflate\r\n", 27);
									}
								}
							}
						}
					}
#endif
					if (Response.StatusCode != 413 && !Request.GetHeader("Range").empty())
					{
						Core::Vector<std::pair<size_t, size_t>> Ranges = Request.GetRanges();
						if (Ranges.size() > 1)
						{
							Core::String Data;
							for (auto It = Ranges.begin(); It != Ranges.end(); ++It)
							{
								std::pair<size_t, size_t> Offset = Request.GetRange(It, Response.Content.Data.size());
								Core::String ContentRange = Paths::ConstructContentRange(Offset.first, Offset.second, Response.Content.Data.size());
								if (Data.size() > Root->Router->MaxHeapBuffer)
								{
									Info.Message.assign("Content range produced too much data.");
									Response.StatusCode = 413;
									Response.Content.Data.clear();
									HrmCache::Get()->Push(Content);
									goto Retry;
								}

								Data.append("--", 2);
								if (Boundary)
									Data.append(*Boundary);
								Data.append("\r\n", 2);

								if (!ContentType.empty())
								{
									Data.append("Content-Type: ", 14);
									Data.append(ContentType);
									Data.append("\r\n", 2);
								}

								Data.append("Content-Range: ", 15);
								Data.append(ContentRange.c_str(), ContentRange.size());
								Data.append("\r\n", 2);
								Data.append("\r\n", 2);
								if (Offset.second > 0)
									Data.append(TextSubstring(Response.Content.Data, Offset.first, Offset.second));
								Data.append("\r\n", 2);
							}

							Data.append("--", 2);
							if (Boundary)
								Data.append(*Boundary);
							Data.append("--\r\n", 4);
							Response.Content.Assign(Data);
						}
						else if (!Ranges.empty())
						{
							auto Range = Ranges.begin();
							bool IsFullLength = (Range->first == -1 && Range->second == -1);
							std::pair<size_t, size_t> Offset = Request.GetRange(Range, Response.Content.Data.size());
							if (Response.GetHeader("Content-Range").empty())
								Content->append("Content-Range: ").append(Paths::ConstructContentRange(Offset.first, Offset.second, Response.Content.Data.size())).append("\r\n");
							if (!Offset.second)
								Response.Content.Data.clear();
							else if (!IsFullLength)
								Response.Content.Assign(TextSubstring(Response.Content.Data, Offset.first, Offset.second));
						}
					}

					if (Response.GetHeader("Content-Length").empty())
						Content->append("Content-Length: ").append(Core::ToString(Response.Content.Data.size())).append("\r\n");
				}
				else if (Response.GetHeader("Content-Length").empty())
					Content->append("Content-Length: 0\r\n", 19);

				if (Request.User.Type == Auth::Denied && Response.GetHeader("WWW-Authenticate").empty())
					Content->append("WWW-Authenticate: " + Route->Auth.Type + " realm=\"" + Route->Auth.Realm + "\"\r\n");

				Paths::ConstructHeadFull(&Request, &Response, false, *Content);
				if (Route->Callbacks.Headers)
					Route->Callbacks.Headers(this, *Content);

				Content->append("\r\n", 2);
				if (ApplyBodyInlining)
					Content->append(Response.Content.Data.begin(), Response.Content.Data.end());

				auto Status = Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [this, Content, Callback = std::move(Callback)](SocketPoll Event) mutable
				{
					HrmCache::Get()->Push(Content);
					Callback(this, Event);
				}, false);
				return Status || Status.Error() == std::errc::operation_would_block;
			}
			bool Connection::ErrorResponseRequested()
			{
				return Response.StatusCode >= 400 && !Response.Error && Response.Content.Data.empty();
			}
			bool Connection::BodyInliningRequested()
			{
				return !Response.Content.Data.empty() && Response.Content.Data.size() <= INLINING_SIZE;
			}
			bool Connection::WaitingForWebSocket()
			{
				if (WebSocket != nullptr && !WebSocket->IsFinished())
				{
					WebSocket->SendClose([](WebSocketFrame* Frame) { Frame->Next(); });
					return true;
				}

				Core::Memory::Release(WebSocket);
				return false;
			}
			bool Connection::SendHeaders(int StatusCode, bool SpecifyTransferEncoding, HeadersCallback&& Callback)
			{
				Response.StatusCode = StatusCode;
				if (Response.StatusCode <= 0 || Stream->Outcome > 0 || !Response.Content.Data.empty())
					return false;

				if (SpecifyTransferEncoding && Response.GetHeader("Transfer-Encoding").empty())
					Response.SetHeader("Transfer-Encoding", "chunked");

				ComposeResponse(ErrorResponseRequested(), false, std::move(Callback));
				return true;
			}
			bool Connection::SendChunk(const std::string_view& Chunk, HeadersCallback&& Callback)
			{
				if (Response.StatusCode <= 0 || !Stream->Outcome || !Response.Content.Data.empty())
					return false;

				auto TransferEncoding = Response.GetHeader("Transfer-Encoding");
				bool IsTransferEncodingChunked = Core::Stringify::CaseEquals(TransferEncoding, "chunked");
				if (IsTransferEncodingChunked)
				{
					if (!Chunk.empty())
					{
						Core::String Content = Core::Stringify::Text("%X\r\n", (uint32_t)Chunk.size());
						Content.append(Chunk);
						Content.append("\r\n");
						Stream->WriteQueued((uint8_t*)Content.c_str(), Content.size(), std::bind(Callback, this, std::placeholders::_1));
					}
					else
						Stream->WriteQueued((uint8_t*)"0\r\n\r\n", 5, std::bind(Callback, this, std::placeholders::_1), false);
				}
				else
				{
					if (Chunk.empty())
						return false;

					Stream->WriteQueued((uint8_t*)Chunk.data(), Chunk.size(), std::bind(Callback, this, std::placeholders::_1));
				}

				return true;
			}
			bool Connection::Fetch(ContentCallback&& Callback, bool Eat)
			{
				VI_ASSERT(ConnectionValid(this), "connection should be valid");
				if (!Request.Content.Resources.empty())
				{
					if (Callback)
						Callback(this, SocketPoll::FinishSync, "");
					return false;
				}
				else if (Request.Content.IsFinalized())
				{
					if (!Eat && Callback && !Request.Content.Data.empty())
						Callback(this, SocketPoll::Next, std::string_view(Request.Content.Data.data(), Request.Content.Data.size()));
					if (Callback)
						Callback(this, SocketPoll::FinishSync, "");
					return true;
				}
				else if (Request.Content.Exceeds)
				{
					if (Callback)
						Callback(this, SocketPoll::FinishSync, "");
					return false;
				}
				else if (!Stream->IsValid())
				{
					if (Callback)
						Callback(this, SocketPoll::Reset, "");
					return false;
				}

				auto ContentType = Request.GetHeader("Content-Type");
				if (ContentType == std::string_view("multipart/form-data", 19))
				{
					Request.Content.Exceeds = true;
					if (Callback)
						Callback(this, SocketPoll::FinishSync, "");
					return false;
				}

				auto TransferEncoding = Request.GetHeader("Transfer-Encoding");
				bool IsTransferEncodingChunked = (!Request.Content.Limited && Core::Stringify::CaseEquals(TransferEncoding, "chunked"));
				size_t ContentLength = Request.Content.Length >= Request.Content.Prefetch ? Request.Content.Length - Request.Content.Prefetch : Request.Content.Length;
				if (IsTransferEncodingChunked)
					Resolver->PrepareForChunkedParsing();

				if (Request.Content.Prefetch > 0)
				{
					Request.Content.Prefetch = 0;
					if (IsTransferEncodingChunked)
					{
						size_t DecodedSize = Request.Content.Data.size();
						int64_t Subresult = Resolver->ParseDecodeChunked((uint8_t*)Request.Content.Data.data(), &DecodedSize);
						Request.Content.Data.resize(DecodedSize);
						if (!Eat && Callback && !Request.Content.Data.empty())
							Callback(this, SocketPoll::Next, std::string_view(Request.Content.Data.data(), Request.Content.Data.size()));

						if (Subresult == -1 || Subresult == 0)
						{
							Request.Content.Finalize();
							if (Callback)
								Callback(this, SocketPoll::FinishSync, "");
							return Subresult == 0;
						}
					}
					else if (!Eat && Callback)
						Callback(this, SocketPoll::Next, std::string_view(Request.Content.Data.data(), Request.Content.Data.size()));
				}

				if (IsTransferEncodingChunked)
				{
					return !!Stream->ReadQueued(Root->Router->MaxNetBuffer, [this, Eat, Callback = std::move(Callback)](SocketPoll Event, const uint8_t* Buffer, size_t Recv)
					{
						if (Packet::IsData(Event))
						{
							int64_t Result = Resolver->ParseDecodeChunked((uint8_t*)Buffer, &Recv);
							if (Result == -1)
								return false;

							Request.Content.Offset += Recv;
							if (Eat)
								return Result == -2;
							else if (Callback)
								Callback(this, SocketPoll::Next, std::string_view((char*)Buffer, Recv));

							if (Request.Content.Data.size() < Root->Router->MaxNetBuffer)
								Request.Content.Append(std::string_view((char*)Buffer, Recv));
							return Result == -2;
						}
						else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
						{
							Request.Content.Finalize();
							if (Callback)
								Callback(this, Event, "");
						}

						return true;
					});
				}
				else if (ContentLength > Root->Router->MaxHeapBuffer || ContentLength > Root->Router->MaxNetBuffer)
				{
					Request.Content.Exceeds = true;
					if (Callback)
						Callback(this, SocketPoll::FinishSync, "");
					return true;
				}
				else if (Request.Content.Limited && !ContentLength)
				{
					Request.Content.Finalize();
					if (Callback)
						Callback(this, SocketPoll::FinishSync, "");
					return true;
				}

				return !!Stream->ReadQueued(Request.Content.Limited ? ContentLength : Root->Router->MaxHeapBuffer, [this, Eat, Callback = std::move(Callback)](SocketPoll Event, const uint8_t* Buffer, size_t Recv)
				{
					if (Packet::IsData(Event))
					{
						Request.Content.Offset += Recv;
						if (Eat)
							return true;

						if (Callback)
							Callback(this, SocketPoll::Next, std::string_view((char*)Buffer, Recv));

						if (Request.Content.Data.size() < Root->Router->MaxHeapBuffer)
							Request.Content.Append(std::string_view((char*)Buffer, Recv));
					}
					else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
					{
						Request.Content.Finalize();
						if (Callback)
							Callback(this, Event, "");
					}

					return true;
				});
			}
			bool Connection::Store(ResourceCallback&& Callback, bool Eat)
			{
				VI_ASSERT(ConnectionValid(this), "connection should be valid");
				if (!Request.Content.Resources.empty())
				{
					if (!Callback)
						return true;

					if (!Eat)
					{
						for (auto& Item : Request.Content.Resources)
							Callback(&Item);
					}

					Callback(nullptr);
					return true;
				}
				else if (Request.Content.IsFinalized())
				{
					if (Callback)
						Callback(nullptr);
					return true;
				}
				else if (!Request.Content.Limited)
				{
					if (Callback)
						Callback(nullptr);
					return false;
				}

				auto ContentType = Request.GetHeader("Content-Type");
				const char* BoundaryName = (ContentType.empty() ? nullptr : strstr(ContentType.data(), "boundary="));
				Request.Content.Exceeds = true;

				size_t ContentLength = Request.Content.Length >= Request.Content.Prefetch ? Request.Content.Length - Request.Content.Prefetch : Request.Content.Length;
				if (!ContentType.empty() && BoundaryName != nullptr)
				{
					if (Route->Router->TemporaryDirectory.empty())
						Eat = true;

					Core::String Boundary("--");
					Boundary.append(BoundaryName + 9);

					Resolver->PrepareForMultipartParsing(&Response.Content, &Route->Router->TemporaryDirectory, Route->Router->MaxUploadableResources, Eat, std::move(Callback));
					if (Request.Content.Prefetch > 0)
					{
						Request.Content.Prefetch = 0;
						if (Resolver->MultipartParse(Boundary.c_str(), (uint8_t*)Request.Content.Data.data(), Request.Content.Data.size()) == -1 || Resolver->Multipart.Finish || !ContentLength)
						{
							Request.Content.Finalize();
							if (Resolver->Multipart.Callback)
								Resolver->Multipart.Callback(nullptr);
							return false;
						}
					}

					return !!Stream->ReadQueued(ContentLength, [this, Boundary](SocketPoll Event, const uint8_t* Buffer, size_t Recv)
					{
						if (Packet::IsData(Event))
						{
							Request.Content.Offset += Recv;
							if (Resolver->MultipartParse(Boundary.c_str(), Buffer, Recv) != -1 && !Resolver->Multipart.Finish)
								return true;

							if (Resolver->Multipart.Callback)
								Resolver->Multipart.Callback(nullptr);

							return false;
						}
						else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
						{
							Request.Content.Finalize();
							if (Resolver->Multipart.Callback)
								Resolver->Multipart.Callback(nullptr);
						}

						return true;
					});
				}
				else if (!ContentLength)
				{
					if (Callback)
						Callback(nullptr);
					return true;
				}
				
				if (Eat)
				{
					return !!Stream->ReadQueued(ContentLength, [this, Callback = std::move(Callback)](SocketPoll Event, const uint8_t* Buffer, size_t Recv)
					{
						if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
						{
							Request.Content.Finalize();
							if (Callback)
								Callback(nullptr);
						}
						else
							Request.Content.Offset += Recv;
						return true;
					});
				}

				HTTP::Resource Subresource;
				Subresource.Length = Request.Content.Length;
				Subresource.Type = (ContentType.empty() ? "application/octet-stream" : ContentType);

				auto Hash = Compute::Crypto::HashHex(Compute::Digests::MD5(), *Compute::Crypto::RandomBytes(16));
				Subresource.Path = *Core::OS::Directory::GetWorking() + *Hash;
				FILE* File = Core::OS::File::Open(Subresource.Path.c_str(), "wb").Or(nullptr);
				if (!File || (Request.Content.Prefetch > 0 && fwrite(Request.Content.Data.data(), 1, Request.Content.Data.size(), File) != Request.Content.Data.size()))
				{
					if (File != nullptr)
						Core::OS::File::Close(File);
					if (Callback)
						Callback(nullptr);
					return false;
				}

				Request.Content.Prefetch = 0;
				return !!Stream->ReadQueued(ContentLength, [this, File, Subresource = std::move(Subresource), Callback = std::move(Callback)](SocketPoll Event, const uint8_t* Buffer, size_t Recv)
				{
					if (Packet::IsData(Event))
					{
						Request.Content.Offset += Recv;
						if (fwrite(Buffer, 1, Recv, File) == Recv)
							return true;

						Core::OS::File::Close(File);
						if (Callback)
							Callback(nullptr);

						return false;
					}
					else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
					{
						Request.Content.Finalize();
						if (Packet::IsDone(Event))
						{
							Request.Content.Resources.push_back(Subresource);
							if (Callback)
								Callback(&Request.Content.Resources.back());
						}

						if (Callback)
							Callback(nullptr);

						Core::OS::File::Close(File);
						return false;
					}

					return true;
				});
			}
			bool Connection::Skip(SuccessCallback&& Callback)
			{
				VI_ASSERT(Callback != nullptr, "callback should be set");
				Fetch([Callback = std::move(Callback)](HTTP::Connection* Base, SocketPoll Event, const std::string_view&) mutable
				{
					if (!Packet::IsDone(Event) && !Packet::IsErrorOrSkip(Event))
						return true;

					if (!Base->Request.Content.Exceeds)
					{
						Callback(Base);
						return true;
					}

					return Base->Store([Base, Callback = std::move(Callback)](HTTP::Resource* Resource)
					{
						Callback(Base);
						return true;
					}, true);
				}, true);
				return false;
			}
			bool Connection::Next()
			{
				VI_ASSERT(ConnectionValid(this), "connection should be valid");
				if (WaitingForWebSocket())
					return false;

				if (Response.StatusCode <= 0 || Stream->Outcome > 0)
					return !!Root->Continue(this);

				bool ApplyErrorResponse = ErrorResponseRequested();
				if (ApplyErrorResponse)
				{
					Response.Error = true;
					for (auto& Page : Route->ErrorFiles)
					{
						if (Page.StatusCode != Response.StatusCode && Page.StatusCode != 0)
							continue;

						Request.Path = Page.Pattern;
						Response.SetHeader("X-Error", Info.Message);
						return Routing::RouteGet(this);
					}
				}

				bool ResponseBodyInlined = BodyInliningRequested();
				return ComposeResponse(ApplyErrorResponse, ResponseBodyInlined, [ResponseBodyInlined](Connection* Base, SocketPoll Event)
				{
					auto& Content = Base->Response.Content.Data;
					if (Packet::IsDone(Event) && !ResponseBodyInlined && !Content.empty() && memcmp(Base->Request.Method, "HEAD", 4) != 0)
						Base->Stream->WriteQueued((uint8_t*)Content.data(), Content.size(), [Base](SocketPoll) { Base->Root->Continue(Base); }, false);
					else
						Base->Root->Continue(Base);
				});
			}
			bool Connection::Next(int StatusCode)
			{
				Response.StatusCode = StatusCode;
				return Next();
			}
			Core::ExpectsIO<Core::String> Connection::GetPeerIpAddress() const
			{
				if (!Route->ProxyIpAddress.empty())
				{
					auto ProxyAddress = Request.GetHeader(Route->ProxyIpAddress.c_str());
					if (!ProxyAddress.empty())
						return Core::String(ProxyAddress);
				}

				auto Address = Stream->GetPeerAddress();
				if (!Address)
					return Address.Error();

				return Address->GetIpAddress();
			}

			Query::Query() : Object(Core::Var::Set::Object())
			{
			}
			Query::~Query() noexcept
			{
				Core::Memory::Release(Object);
			}
			void Query::Clear()
			{
				if (Object != nullptr)
					Object->Clear();
			}
			void Query::Steal(Core::Schema** Output)
			{
				if (!Output)
					return;

				Core::Memory::Release(*Output);
				*Output = Object;
				Object = nullptr;
			}
			void Query::NewParameter(Core::Vector<QueryToken>* Tokens, const QueryToken& Name, const QueryToken& Value)
			{
				Core::String Body = Compute::Codec::URLDecode(std::string_view(Name.Value, Name.Length));
				size_t Offset = 0, Length = Body.size();
				char* Data = (char*)Body.c_str();

				for (size_t i = 0; i < Length; i++)
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

				Core::Schema* Parameter = nullptr;
				for (auto& Item : *Tokens)
				{
					if (Parameter != nullptr)
						Parameter = FindParameter(Parameter, &Item);
					else
						Parameter = GetParameter(&Item);
				}

				if (Parameter != nullptr)
					Parameter->Value.Deserialize(Compute::Codec::URLDecode(std::string_view(Value.Value, Value.Length)));
			}
			void Query::Decode(const std::string_view& Type, const std::string_view& Body)
			{
				if (Type.empty() || Body.empty())
					return;

				if (Core::Stringify::CaseEquals(Type, "application/x-www-form-urlencoded"))
					DecodeAXWFD(Body);
				else if (Core::Stringify::CaseEquals(Type, "application/json"))
					DecodeAJSON(Body);
			}
			void Query::DecodeAXWFD(const std::string_view& Body)
			{
				Core::Vector<QueryToken> Tokens;
				size_t Offset = 0, Length = Body.size();
				char* Data = (char*)Body.data();

				for (size_t i = 0; i < Length; i++)
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
			void Query::DecodeAJSON(const std::string_view& Body)
			{
				Core::Memory::Release(Object);
				auto Result = Core::Schema::ConvertFromJSON(Body);
				if (Result)
					Object = *Result;
			}
			Core::String Query::Encode(const std::string_view& Type) const
			{
				if (Core::Stringify::CaseEquals(Type, "application/x-www-form-urlencoded"))
					return EncodeAXWFD();

				if (Core::Stringify::CaseEquals(Type, "application/json"))
					return EncodeAJSON();

				return "";
			}
			Core::String Query::EncodeAXWFD() const
			{
				Core::String Output; auto& Nodes = Object->GetChilds();
				for (auto It = Nodes.begin(); It != Nodes.end(); ++It)
				{
					Output.append(BuildFromBase(*It));
					if (It + 1 < Nodes.end())
						Output.append(1, '&');
				}

				return Output;
			}
			Core::String Query::EncodeAJSON() const
			{
				Core::String Stream;
				Core::Schema::ConvertToJSON(Object, [&Stream](Core::VarForm, const std::string_view& Buffer) { Stream.append(Buffer); });
				return Stream;
			}
			Core::Schema* Query::Get(const std::string_view& Name) const
			{
				return (Core::Schema*)Object->Get(Name);
			}
			Core::Schema* Query::Set(const std::string_view& Name)
			{
				return (Core::Schema*)Object->Set(Name, Core::Var::String(""));
			}
			Core::Schema* Query::Set(const std::string_view& Name, const std::string_view& Value)
			{
				return (Core::Schema*)Object->Set(Name, Core::Var::String(Value));
			}
			Core::Schema* Query::GetParameter(QueryToken* Name)
			{
				VI_ASSERT(Name != nullptr, "token should be set");
				if (Name->Value && Name->Length > 0)
				{
					for (auto* Item : Object->GetChilds())
					{
						if (Item->Key.size() != Name->Length)
							continue;

						if (!strncmp(Item->Key.c_str(), Name->Value, (size_t)Name->Length))
							return (Core::Schema*)Item;
					}
				}

				Core::Schema* New = Core::Var::Set::Object();
				if (Name->Value && Name->Length > 0)
				{
					New->Key.assign(Name->Value, (size_t)Name->Length);
					if (!Core::Stringify::HasInteger(New->Key))
						Object->Value = Core::Var::Object();
					else
						Object->Value = Core::Var::Array();
				}
				else
				{
					New->Key.assign(Core::ToString(Object->Size()));
					Object->Value = Core::Var::Array();
				}

				New->Value = Core::Var::String("");
				Object->Push(New);

				return New;
			}
			Core::String Query::Build(Core::Schema* Base)
			{
				Core::String Output, Label = Compute::Codec::URLEncode(Base->GetParent() != nullptr ? ('[' + Base->Key + ']') : Base->Key);
				if (!Base->Empty())
				{
					auto& Childs = Base->GetChilds();
					for (auto It = Childs.begin(); It != Childs.end(); ++It)
					{
						Output.append(Label).append(Build(*It));
						if (It + 1 < Childs.end())
							Output += '&';
					}
				}
				else
				{
					Core::String V = Base->Value.Serialize();
					if (!V.empty())
						Output.append(Label).append(1, '=').append(Compute::Codec::URLEncode(V));
					else
						Output.append(Label);
				}

				return Output;
			}
			Core::String Query::BuildFromBase(Core::Schema* Base)
			{
				Core::String Output, Label = Compute::Codec::URLEncode(Base->Key);
				if (!Base->Empty())
				{
					auto& Childs = Base->GetChilds();
					for (auto It = Childs.begin(); It != Childs.end(); ++It)
					{
						Output.append(Label).append(Build(*It));
						if (It + 1 < Childs.end())
							Output += '&';
					}
				}
				else
				{
					Core::String V = Base->Value.Serialize();
					if (!V.empty())
						Output.append(Label).append(1, '=').append(Compute::Codec::URLEncode(V));
					else
						Output.append(Label);
				}

				return Output;
			}
			Core::Schema* Query::FindParameter(Core::Schema* Base, QueryToken* Name)
			{
				VI_ASSERT(Name != nullptr, "token should be set");
				if (!Base->Empty() && Name->Value && Name->Length > 0)
				{
					for (auto* Item : Base->GetChilds())
					{
						if (!strncmp(Item->Key.c_str(), Name->Value, (size_t)Name->Length))
							return (Core::Schema*)Item;
					}
				}

				Core::String Key;
				if (Name->Value && Name->Length > 0)
				{
					Key.assign(Name->Value, (size_t)Name->Length);
					if (!Core::Stringify::HasInteger(Key))
						Base->Value = Core::Var::Object();
					else
						Base->Value = Core::Var::Array();
				}
				else
				{
					Key.assign(Core::ToString(Base->Size()));
					Base->Value = Core::Var::Array();
				}

				return Base->Set(Key, Core::Var::String(""));
			}

			Session::Session()
			{
				Query = Core::Var::Set::Object();
			}
			Session::~Session() noexcept
			{
				Core::Memory::Release(Query);
			}
			Core::ExpectsSystem<void> Session::Write(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				auto* Router = Base->Route->Router;
				Core::String Path = Router->Session.Directory + FindSessionId(Base);
				auto Stream = Core::OS::File::Open(Path.c_str(), "wb");
				if (!Stream)
					return Core::SystemException("session write error", std::move(Stream.Error()));

				SessionExpires = time(nullptr) + Router->Session.Expires;
				fwrite(&SessionExpires, sizeof(int64_t), 1, *Stream);
				Query->ConvertToJSONB(Query, [&Stream](Core::VarForm, const std::string_view& Buffer)
				{
					if (!Buffer.empty())
						fwrite(Buffer.data(), Buffer.size(), 1, *Stream);
				});
				Core::OS::File::Close(*Stream);
				return Core::Expectation::Met;
			}
			Core::ExpectsSystem<void> Session::Read(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				Core::String Path = Base->Route->Router->Session.Directory + FindSessionId(Base);
				auto Stream = Core::OS::File::Open(Path.c_str(), "rb");
				if (!Stream)
					return Core::SystemException("session read error", std::move(Stream.Error()));

				if (fread(&SessionExpires, 1, sizeof(int64_t), *Stream) != sizeof(int64_t))
				{
					Core::OS::File::Close(*Stream);
					return Core::SystemException("session read error: invalid format", std::make_error_condition(std::errc::bad_message));
				}

				if (SessionExpires <= time(nullptr))
				{
					SessionId.clear();
					Core::OS::File::Close(*Stream);
					Core::OS::File::Remove(Path.c_str());
					return Core::SystemException("session read error: expired", std::make_error_condition(std::errc::timed_out));
				}

				Core::Memory::Release(Query);
				auto Result = Core::Schema::ConvertFromJSONB([&Stream](uint8_t* Buffer, size_t Size) { return fread(Buffer, sizeof(uint8_t), Size, *Stream) == Size; });
				Core::OS::File::Close(*Stream);
				if (!Result)
					return Core::SystemException(Result.Error().message(), std::make_error_condition(std::errc::bad_message));

				Query = *Result;
				return Core::Expectation::Met;
			}
			void Session::Clear()
			{
				if (Query != nullptr)
					Query->Clear();
			}
			Core::String& Session::FindSessionId(Connection* Base)
			{
				if (!SessionId.empty())
					return SessionId;

				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				auto Value = Base->Request.GetCookie(Base->Route->Router->Session.Cookie.Name.c_str());
				if (Value.empty())
					return GenerateSessionId(Base);

				return SessionId.assign(Value);
			}
			Core::String& Session::GenerateSessionId(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				int64_t Time = time(nullptr);
				auto Hash = Compute::Crypto::HashHex(Compute::Digests::MD5(), Base->Request.Location + Core::ToString(Time));
				if (Hash)
					SessionId = *Hash;
				else
					SessionId = Core::ToString(Time);

				auto* Router = Base->Route->Router;
				if (SessionExpires == 0)
					SessionExpires = Time + Router->Session.Expires;

				Cookie Result;
				Result.Value = SessionId;
				Result.Name = Router->Session.Cookie.Name;
				Result.Domain = Router->Session.Cookie.Domain;
				Result.Path = Router->Session.Cookie.Path;
				Result.SameSite = Router->Session.Cookie.SameSite;
				Result.Secure = Router->Session.Cookie.Secure;
				Result.HttpOnly = Router->Session.Cookie.HttpOnly;
				Result.SetExpires(Time + (int64_t)Router->Session.Cookie.Expires);
				Base->Response.SetCookie(std::move(Result));

				return SessionId;
			}
			Core::ExpectsSystem<void> Session::InvalidateCache(const std::string_view& Path)
			{
				Core::Vector<std::pair<Core::String, Core::FileEntry>> Entries;
				auto Status = Core::OS::Directory::Scan(Path, Entries);
				if (!Status)
					return Core::SystemException("session invalidation scan error", std::move(Status.Error()));

				bool Split = (Path.back() != '\\' && Path.back() != '/');
				for (auto& Item : Entries)
				{
					if (Item.second.IsDirectory)
						continue;

					Core::String Filename = Core::String(Path);
					if (Split)
						Filename.append(1, VI_SPLITTER);
					Filename.append(Item.first);
					Status = Core::OS::File::Remove(Filename);
					if (!Status)
						return Core::SystemException("session invalidation remove error: " + Item.first, std::move(Status.Error()));
				}

				return Core::Expectation::Met;
			}

			Parser::Parser()
			{
			}
			Parser::~Parser() noexcept
			{
				Core::Memory::Deallocate(Multipart.Boundary);
			}
			void Parser::PrepareForRequestParsing(RequestFrame* Request)
			{
				Message.Header.clear();
				Message.Version = Request->Version;
				Message.Method = Request->Method;
				Message.StatusCode = nullptr;
				Message.Location = &Request->Location;
				Message.Query = &Request->Query;
				Message.Cookies = &Request->Cookies;
				Message.Headers = &Request->Headers;
				Message.Content = &Request->Content;
			}
			void Parser::PrepareForResponseParsing(ResponseFrame* Response)
			{
				Message.Header.clear();
				Message.Version = nullptr;
				Message.Method = nullptr;
				Message.StatusCode = &Response->StatusCode;
				Message.Location = nullptr;
				Message.Query = nullptr;
				Message.Cookies = nullptr;
				Message.Headers = &Response->Headers;
				Message.Content = &Response->Content;
			}
			void Parser::PrepareForChunkedParsing()
			{
				Chunked = ChunkedState();
			}
			void Parser::PrepareForMultipartParsing(ContentFrame* Content, Core::String* TemporaryDirectory, size_t MaxResources, bool Skip, ResourceCallback&& Callback)
			{
				Core::Memory::Deallocate(Multipart.Boundary);
				Multipart = MultipartState();
				Multipart.Skip = Skip;
				Multipart.MaxResources = MaxResources;
				Multipart.TemporaryDirectory = TemporaryDirectory;
				Multipart.Callback = std::move(Callback);
				Message.Header.clear();
				Message.Content = Content;
			}
			int64_t Parser::MultipartParse(const std::string_view& Boundary, const uint8_t* Buffer, size_t Length)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				if (!Multipart.Boundary || !Multipart.LookBehind)
				{
					Core::Memory::Deallocate(Multipart.Boundary);
					Multipart.Length = Boundary.size();
					Multipart.Boundary = Core::Memory::Allocate<uint8_t>(sizeof(uint8_t) * (size_t)(Multipart.Length * 2 + 9));
					memcpy(Multipart.Boundary, Boundary.data(), sizeof(uint8_t) * (size_t)Multipart.Length);
					Multipart.Boundary[Multipart.Length] = '\0';
					Multipart.LookBehind = (Multipart.Boundary + Multipart.Length + 1);
					Multipart.State = MultipartStatus::Start;
					Multipart.Index = 0;
				}

				char Value, Lower;
				char LF = 10, CR = 13;
				size_t i = 0, Mark = 0;
				int Last = 0;

				while (i < Length)
				{
					Value = Buffer[i];
					Last = (i == (Length - 1));

					switch (Multipart.State)
					{
						case MultipartStatus::Start:
							Multipart.Index = 0;
							Multipart.State = MultipartStatus::Start_Boundary;
						case MultipartStatus::Start_Boundary:
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
								if (!Parsing::ParseMultipartResourceBegin(this))
									return i;

								Multipart.State = MultipartStatus::Header_Field_Start;
								break;
							}

							if (Value != Multipart.Boundary[Multipart.Index])
								return i;

							Multipart.Index++;
							break;
						case MultipartStatus::Header_Field_Start:
							Mark = i;
							Multipart.State = MultipartStatus::Header_Field;
						case MultipartStatus::Header_Field:
							if (Value == CR)
							{
								Multipart.State = MultipartStatus::Header_Field_Waiting;
								break;
							}

							if (Value == ':')
							{
								if (!Parsing::ParseMultipartHeaderField(this, Buffer + Mark, i - Mark))
									return i;

								Multipart.State = MultipartStatus::Header_Value_Start;
								break;
							}

							Lower = tolower(Value);
							if ((Value != '-') && (Lower < 'a' || Lower > 'z'))
								return i;

							if (Last && !Parsing::ParseMultipartHeaderField(this, Buffer + Mark, (i - Mark) + 1))
								return i;

							break;
						case MultipartStatus::Header_Field_Waiting:
							if (Value != LF)
								return i;

							Multipart.State = MultipartStatus::Resource_Start;
							break;
						case MultipartStatus::Header_Value_Start:
							if (Value == ' ')
								break;

							Mark = i;
							Multipart.State = MultipartStatus::Header_Value;
						case MultipartStatus::Header_Value:
							if (Value == CR)
							{
								if (!Parsing::ParseMultipartHeaderValue(this, Buffer + Mark, i - Mark))
									return i;

								Multipart.State = MultipartStatus::Header_Value_Waiting;
								break;
							}

							if (Last && !Parsing::ParseMultipartHeaderValue(this, Buffer + Mark, (i - Mark) + 1))
								return i;

							break;
						case MultipartStatus::Header_Value_Waiting:
							if (Value != LF)
								return i;

							Multipart.State = MultipartStatus::Header_Field_Start;
							break;
						case MultipartStatus::Resource_Start:
							Mark = i;
							Multipart.State = MultipartStatus::Resource;
						case MultipartStatus::Resource:
							if (Value == CR)
							{
								if (!Parsing::ParseMultipartContentData(this, Buffer + Mark, i - Mark))
									return i;

								Mark = i;
								Multipart.State = MultipartStatus::Resource_Boundary_Waiting;
								Multipart.LookBehind[0] = CR;
								break;
							}

							if (Last && !Parsing::ParseMultipartContentData(this, Buffer + Mark, (i - Mark) + 1))
								return i;

							break;
						case MultipartStatus::Resource_Boundary_Waiting:
							if (Value == LF)
							{
								Multipart.State = MultipartStatus::Resource_Boundary;
								Multipart.LookBehind[1] = LF;
								Multipart.Index = 0;
								break;
							}

							if (!Parsing::ParseMultipartContentData(this, Multipart.LookBehind, 1))
								return i;

							Multipart.State = MultipartStatus::Resource;
							Mark = i--;
							break;
						case MultipartStatus::Resource_Boundary:
							if (Multipart.Boundary[Multipart.Index] != Value)
							{
								if (!Parsing::ParseMultipartContentData(this, Multipart.LookBehind, 2 + (size_t)Multipart.Index))
									return i;

								Multipart.State = MultipartStatus::Resource;
								Mark = i--;
								break;
							}

							Multipart.LookBehind[2 + Multipart.Index] = Value;
							if ((++Multipart.Index) == Multipart.Length)
							{
								if (!Parsing::ParseMultipartResourceEnd(this))
									return i;

								Multipart.State = MultipartStatus::Resource_Waiting;
							}
							break;
						case MultipartStatus::Resource_Waiting:
							if (Value == '-')
							{
								Multipart.State = MultipartStatus::Resource_Hyphen;
								break;
							}

							if (Value == CR)
							{
								Multipart.State = MultipartStatus::Resource_End;
								break;
							}

							return i;
						case MultipartStatus::Resource_Hyphen:
							if (Value == '-')
							{
								Multipart.State = MultipartStatus::End;
								break;
							}

							return i;
						case MultipartStatus::Resource_End:
							if (Value == LF)
							{
								Multipart.State = MultipartStatus::Header_Field_Start;
								if (!Parsing::ParseMultipartResourceBegin(this))
									return i;

								break;
							}

							return i;
						case MultipartStatus::End:
							break;
						default:
							return -1;
					}

					++i;
				}

				return Length;
			}
			int64_t Parser::ParseRequest(const uint8_t* BufferStart, size_t Length, size_t Offset)
			{
				VI_ASSERT(BufferStart != nullptr, "buffer start should be set");
				const uint8_t* Buffer = BufferStart;
				const uint8_t* BufferEnd = BufferStart + Length;
				int Result;
				
				if (IsCompleted(Buffer, BufferEnd, Offset, &Result) == nullptr)
					return (int64_t)Result;

				if ((Buffer = ProcessRequest(Buffer, BufferEnd, &Result)) == nullptr)
					return (int64_t)Result;

				return (int64_t)(Buffer - BufferStart);
			}
			int64_t Parser::ParseResponse(const uint8_t* BufferStart, size_t Length, size_t Offset)
			{
				VI_ASSERT(BufferStart != nullptr, "buffer start should be set");
				const uint8_t* Buffer = BufferStart;
				const uint8_t* BufferEnd = Buffer + Length;
				int Result;

				if (IsCompleted(Buffer, BufferEnd, Offset, &Result) == nullptr)
					return (int64_t)Result;

				if ((Buffer = ProcessResponse(Buffer, BufferEnd, &Result)) == nullptr)
					return (int64_t)Result;

				return (int64_t)(Buffer - BufferStart);
			}
			int64_t Parser::ParseDecodeChunked(uint8_t* Buffer, size_t* Length)
			{
				VI_ASSERT(Buffer != nullptr && Length != nullptr, "buffer should be set");
				size_t Dest = 0, Src = 0, Size = *Length;
				int64_t Result = -2;

				while (true)
				{
					switch (Chunked.State)
					{
						case ChunkedStatus::Size:
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
							Chunked.State = ChunkedStatus::Ext;
						case ChunkedStatus::Ext:
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
									Chunked.State = ChunkedStatus::Head;
									break;
								}
								else
									goto Complete;
							}

							Chunked.State = ChunkedStatus::Data;
						case ChunkedStatus::Data:
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
							Chunked.State = ChunkedStatus::End;
						}
						case ChunkedStatus::End:
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
							Chunked.State = ChunkedStatus::Size;
							break;
						case ChunkedStatus::Head:
							for (;; ++Src)
							{
								if (Src == Size)
									goto Exit;

								if (Buffer[Src] != '\015')
									break;
							}

							if (Buffer[Src++] == '\012')
								goto Complete;

							Chunked.State = ChunkedStatus::Middle;
						case ChunkedStatus::Middle:
							for (;; ++Src)
							{
								if (Src == Size)
									goto Exit;

								if (Buffer[Src] == '\012')
									break;
							}

							++Src;
							Chunked.State = ChunkedStatus::Head;
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
			const uint8_t* Parser::Tokenize(const uint8_t* Buffer, const uint8_t* BufferEnd, const uint8_t** Token, size_t* TokenLength, int* Out)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				VI_ASSERT(BufferEnd != nullptr, "buffer end should be set");
				VI_ASSERT(Token != nullptr, "token should be set");
				VI_ASSERT(TokenLength != nullptr, "token length should be set");
				VI_ASSERT(Out != nullptr, "output should be set");

				const uint8_t* TokenStart = Buffer;
				while (BufferEnd - Buffer >= 8)
				{
					for (int i = 0; i < 8; i++)
					{
						if (!((uint8_t)(*Buffer) - 040u < 0137u))
							goto NonPrintable;
						++Buffer;
					}

					continue;
				NonPrintable:
					if (((uint8_t)*Buffer < '\040' && *Buffer != '\011') || *Buffer == '\177')
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

					if (!((uint8_t)(*Buffer) - 040u < 0137u))
					{
						if (((uint8_t)*Buffer < '\040' && *Buffer != '\011') || *Buffer == '\177')
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
			const uint8_t* Parser::IsCompleted(const uint8_t* Buffer, const uint8_t* BufferEnd, size_t Offset, int* Out)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				VI_ASSERT(BufferEnd != nullptr, "buffer end should be set");
				VI_ASSERT(Out != nullptr, "output should be set");

				int Result = 0;
				Buffer = Offset < 3 ? Buffer : Buffer + Offset - 3;

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
			const uint8_t* Parser::ProcessVersion(const uint8_t* Buffer, const uint8_t* BufferEnd, int* Out)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				VI_ASSERT(BufferEnd != nullptr, "buffer end should be set");
				VI_ASSERT(Out != nullptr, "output should be set");

				if (BufferEnd - Buffer < 9)
				{
					*Out = -2;
					return nullptr;
				}

				const uint8_t* Version = Buffer;
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
				if (!Parsing::ParseVersion(this, Version, Buffer - Version))
				{
					*Out = -1;
					return nullptr;
				}

				return Buffer;
			}
			const uint8_t* Parser::ProcessHeaders(const uint8_t* Buffer, const uint8_t* BufferEnd, int* Out)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				VI_ASSERT(BufferEnd != nullptr, "buffer end should be set");
				VI_ASSERT(Out != nullptr, "output should be set");

				static const char* Mapping =
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\1\0\1\1\1\1\1\0\0\1\1\0\1\1\0\1\1\1\1\1\1\1\1\1\1\0\0\0\0\0\0"
					"\0\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\0\0\1\1"
					"\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\1\0\1\0\1\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
					"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

				if (Message.Headers != nullptr)
					Message.Headers->clear();
				if (Message.Cookies != nullptr)
					Message.Cookies->clear();

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
						const uint8_t* Name = Buffer;
						while (true)
						{
							if (*Buffer == ':')
								break;

							if (!Mapping[(uint8_t)*Buffer])
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

						if (!Parsing::ParseHeaderField(this, Name, (size_t)Length))
						{
							*Out = -1;
							return nullptr;
						}

						++Buffer;
						for (;; ++Buffer)
						{
							if (Buffer == BufferEnd)
							{
								if (!Parsing::ParseHeaderValue(this, (uint8_t*)"", 0))
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
					else if (!Parsing::ParseHeaderField(this, (uint8_t*)"", 0))
					{
						*Out = -1;
						return nullptr;
					}

					const uint8_t* Value; size_t ValueLength;
					if ((Buffer = Tokenize(Buffer, BufferEnd, &Value, &ValueLength, Out)) == nullptr)
					{
						Parsing::ParseHeaderValue(this, (uint8_t*)"", 0);
						return nullptr;
					}

					const uint8_t* ValueEnd = Value + ValueLength;
					for (; ValueEnd != Value; --ValueEnd)
					{
						const uint8_t c = *(ValueEnd - 1);
						if (!(c == ' ' || c == '\t'))
							break;
					}

					if (!Parsing::ParseHeaderValue(this, Value, ValueEnd - Value))
					{
						*Out = -1;
						return nullptr;
					}
				}

				return Buffer;
			}
			const uint8_t* Parser::ProcessRequest(const uint8_t* Buffer, const uint8_t* BufferEnd, int* Out)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				VI_ASSERT(BufferEnd != nullptr, "buffer end should be set");
				VI_ASSERT(Out != nullptr, "output should be set");

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

				const uint8_t* TokenStart = Buffer;
				if (Buffer == BufferEnd)
				{
					*Out = -2;
					return nullptr;
				}

				while (true)
				{
					if (*Buffer == ' ')
						break;

					if (!((uint8_t)(*Buffer) - 040u < 0137u))
					{
						if ((uint8_t)*Buffer < '\040' || *Buffer == '\177')
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

				if (!Parsing::ParseMethodValue(this, TokenStart, Buffer - TokenStart))
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

					if (!((uint8_t)(*Buffer) - 040u < 0137u))
					{
						if ((uint8_t)*Buffer < '\040' || *Buffer == '\177')
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

				uint8_t* Path = (uint8_t*)TokenStart;
				int64_t PL = Buffer - TokenStart, QL = 0;
				while (QL < PL && Path[QL] != '?')
					QL++;

				if (QL > 0 && QL < PL)
				{
					QL = PL - QL - 1;
					PL -= QL + 1;
					if (!Parsing::ParseQueryValue(this, Path + PL + 1, (size_t)QL))
					{
						*Out = -1;
						return nullptr;
					}
				}

				if (!Parsing::ParsePathValue(this, Path, (size_t)PL))
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
			const uint8_t* Parser::ProcessResponse(const uint8_t* Buffer, const uint8_t* BufferEnd, int* Out)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				VI_ASSERT(BufferEnd != nullptr, "buffer end should be set");
				VI_ASSERT(Out != nullptr, "output should be set");

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
				if (!Parsing::ParseStatusCode(this, Status))
				{
					*Out = -1;
					return nullptr;
				}

				const uint8_t* Message; size_t MessageLength;
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
				if (!Parsing::ParseStatusMessage(this, Message, MessageLength))
				{
					*Out = -1;
					return nullptr;
				}

				return ProcessHeaders(Buffer, BufferEnd, Out);
			}

			WebCodec::WebCodec() : State(Bytecode::Begin), Fragment(0)
			{
			}
			bool WebCodec::ParseFrame(const uint8_t* Buffer, size_t Size)
			{
				if (!Buffer || !Size)
					return !Queue.empty();

				if (Payload.capacity() <= Size)
					Payload.resize(Size);

				memcpy(Payload.data(), Buffer, sizeof(char) * Size);
				char* Data = Payload.data();
			ParsePayload:
				while (Size)
				{
					uint8_t Index = *Data;
					switch (State)
					{
						case Bytecode::Begin:
						{
							uint8_t Op = Index & 0x0f;
							if (Index & 0x70)
								return !Queue.empty();

							Final = (Index & 0x80) ? 1 : 0;
							if (Op == 0)
							{
								if (!Fragment)
									return !Queue.empty();

								Control = 0;
							}
							else if (Op & 0x8)
							{
								if (Op != (uint8_t)WebSocketOp::Ping && Op != (uint8_t)WebSocketOp::Pong && Op != (uint8_t)WebSocketOp::Close)
									return !Queue.empty();

								if (!Final)
									return !Queue.empty();

								Control = 1;
								Opcode = (WebSocketOp)Op;
							}
							else
							{
								if (Op != (uint8_t)WebSocketOp::Text && Op != (uint8_t)WebSocketOp::Binary)
									return !Queue.empty();

								Control = 0;
								Fragment = !Final;
								Opcode = (WebSocketOp)Op;
							}

							State = Bytecode::Length;
							Data++; Size--;
							break;
						}
						case Bytecode::Length:
						{
							uint8_t Length = Index & 0x7f;
							Masked = (Index & 0x80) ? 1 : 0;
							Masks = 0;

							if (Control)
							{
								if (Length > 125)
									return !Queue.empty();

								Remains = Length;
								State = Masked ? Bytecode::Mask_0 : Bytecode::End;
							}
							else if (Length < 126)
							{
								Remains = Length;
								State = Masked ? Bytecode::Mask_0 : Bytecode::End;
							}
							else if (Length == 126)
								State = Bytecode::Length_16_0;
							else
								State = Bytecode::Length_64_0;

							Data++; Size--;
							if (State == Bytecode::End && Remains == 0)
							{
								Queue.emplace(std::make_pair(Opcode, Core::Vector<char>()));
								goto FetchPayload;
							}
							break;
						}
						case Bytecode::Length_16_0:
						{
							Remains = (uint64_t)Index << 8;
							State = Bytecode::Length_16_1;
							Data++; Size--;
							break;
						}
						case Bytecode::Length_16_1:
						{
							Remains |= (uint64_t)Index << 0;
							State = Masked ? Bytecode::Mask_0 : Bytecode::End;
							if (Remains < 126)
								return !Queue.empty();

							Data++; Size--;
							break;
						}
						case Bytecode::Length_64_0:
						{
							Remains = (uint64_t)Index << 56;
							State = Bytecode::Length_64_1;
							Data++; Size--;
							break;
						}
						case Bytecode::Length_64_1:
						{
							Remains |= (uint64_t)Index << 48;
							State = Bytecode::Length_64_2;
							Data++; Size--;
							break;
						}
						case Bytecode::Length_64_2:
						{
							Remains |= (uint64_t)Index << 40;
							State = Bytecode::Length_64_3;
							Data++; Size--;
							break;
						}
						case Bytecode::Length_64_3:
						{
							Remains |= (uint64_t)Index << 32;
							State = Bytecode::Length_64_4;
							Data++; Size--;
							break;
						}
						case Bytecode::Length_64_4:
						{
							Remains |= (uint64_t)Index << 24;
							State = Bytecode::Length_64_5;
							Data++; Size--;
							break;
						}
						case Bytecode::Length_64_5:
						{
							Remains |= (uint64_t)Index << 16;
							State = Bytecode::Length_64_6;
							Data++; Size--;
							break;
						}
						case Bytecode::Length_64_6:
						{
							Remains |= (uint64_t)Index << 8;
							State = Bytecode::Length_64_7;
							Data++; Size--;
							break;
						}
						case Bytecode::Length_64_7:
						{
							Remains |= (uint64_t)Index << 0;
							State = Masked ? Bytecode::Mask_0 : Bytecode::End;
							if (Remains < 65536)
								return !Queue.empty();

							Data++; Size--;
							break;
						}
						case Bytecode::Mask_0:
						{
							Mask[0] = Index;
							State = Bytecode::Mask_1;
							Data++; Size--;
							break;
						}
						case Bytecode::Mask_1:
						{
							Mask[1] = Index;
							State = Bytecode::Mask_2;
							Data++; Size--;
							break;
						}
						case Bytecode::Mask_2:
						{
							Mask[2] = Index;
							State = Bytecode::Mask_3;
							Data++; Size--;
							break;
						}
						case Bytecode::Mask_3:
						{
							Mask[3] = Index;
							State = Bytecode::End;
							Data++; Size--;
							if (Remains == 0)
							{
								Queue.emplace(std::make_pair(Opcode, Core::Vector<char>()));
								goto FetchPayload;
							}
							break;
						}
						case Bytecode::End:
						{
							size_t Length = Size;
							if (Length > (size_t)Remains)
								Length = (size_t)Remains;

							if (Masked)
							{
								for (size_t i = 0; i < Length; i++)
									Data[i] ^= Mask[Masks++ % 4];
							}

							Core::Vector<char> Message;
							TextAssign(Message, std::string_view(Data, Length));
							Queue.emplace(std::make_pair(Opcode, std::move(Message)));
							Opcode = WebSocketOp::Continue;

							Data += Length;
							Size -= Length;
							Remains -= Length;
							if (Remains == 0)
								goto FetchPayload;
							break;
						}
					}
				}

				return !Queue.empty();
			FetchPayload:
				if (!Control && !Final)
					return !Queue.empty();

				State = Bytecode::Begin;
				if (Size > 0)
					goto ParsePayload;

				return true;
			}
			bool WebCodec::GetFrame(WebSocketOp* Op, Core::Vector<char>* Message)
			{
				VI_ASSERT(Op != nullptr, "op should be set");
				VI_ASSERT(Message != nullptr, "message should be set");

				if (Queue.empty())
					return false;

				auto& Base = Queue.front();
				*Message = std::move(Base.second);
				*Op = Base.first;
				Queue.pop();

				return true;
			}

			HrmCache::HrmCache() noexcept : HrmCache(HTTP_HRM_SIZE)
			{
			}
			HrmCache::HrmCache(size_t MaxBytesStorage) noexcept : Capacity(MaxBytesStorage), Size(0)
			{
			}
			HrmCache::~HrmCache() noexcept
			{
				Size = Capacity = 0;
				while (!Queue.empty())
				{
					auto* Item = Queue.front();
					Core::Memory::Delete(Item);
					Queue.pop();
				}
			}
			void HrmCache::ShrinkToFit() noexcept
			{
				size_t Freed = 0;
				while (!Queue.empty() && Size > Capacity)
				{
					auto* Item = Queue.front();
					size_t Bytes = Item->capacity();
					Size -= std::min<size_t>(Size, Bytes);
					Freed += Bytes;
					Core::Memory::Delete(Item);
					Queue.pop();
				}
				if (Freed > 0)
					VI_DEBUG("[http] freed up %" PRIu64 " bytes from hrm cache", (uint64_t)Freed);
			}
			void HrmCache::Shrink() noexcept
			{
				Core::UMutex<std::mutex> Unique(Mutex);
				ShrinkToFit();
			}
			void HrmCache::Rescale(size_t MaxBytesStorage) noexcept
			{
				Core::UMutex<std::mutex> Unique(Mutex);
				Capacity = MaxBytesStorage;
				ShrinkToFit();
			}
			void HrmCache::Push(Core::String* Entry)
			{
				Entry->clear();
				Core::UMutex<std::mutex> Unique(Mutex);
				Size += Entry->capacity();
				Queue.push(Entry);
				ShrinkToFit();
			}
			Core::String* HrmCache::Pop() noexcept
			{
				Core::UMutex<std::mutex> Unique(Mutex);
				if (Queue.empty())
					return Core::Memory::New<Core::String>();

				auto* Item = Queue.front();
				Size -= std::min<size_t>(Size, Item->capacity());
				Queue.pop();
				return Item;
			}

			Core::String Utils::ConnectionResolve(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				auto* Router = Base->Root->Router;
				if (Router->KeepAliveMaxCount < 0)
					return "Connection: Close\r\n";

				auto Connection = Base->Request.GetHeader("Connection");
				if ((!Connection.empty() && !Core::Stringify::CaseEquals(Connection, "keep-alive")) || (Connection.empty() && strcmp(Base->Request.Version, "1.1") != 0))
				{
					Base->Info.Reuses = 1;
					return "Connection: Close\r\n";
				}

				if (Router->KeepAliveMaxCount == 0)
					return "Connection: Keep-Alive\r\nKeep-Alive: timeout=" + Core::ToString(Router->SocketTimeout / 1000) + "\r\n";

				if (Base->Info.Reuses <= 1)
					return "Connection: Close\r\n";

				return "Connection: Keep-Alive\r\nKeep-Alive: timeout=" + Core::ToString(Router->SocketTimeout / 1000) + ", max=" + Core::ToString(Router->KeepAliveMaxCount) + "\r\n";
			}
			std::string_view Utils::ContentType(const std::string_view& Path, Core::Vector<MimeType>* Types)
			{
				static MimeStatic MimeTypes[] = { MimeStatic(".3dm", "x-world/x-3dmf"), MimeStatic(".3dmf", "x-world/x-3dmf"), MimeStatic(".a", "application/octet-stream"), MimeStatic(".aab", "application/x-authorware-bin"), MimeStatic(".aac", "audio/aac"), MimeStatic(".aam", "application/x-authorware-map"), MimeStatic(".aas", "application/x-authorware-seg"), MimeStatic(".aat", "application/font-sfnt"), MimeStatic(".abc", "text/vnd.abc"), MimeStatic(".acgi", "text/html"), MimeStatic(".afl", "video/animaflex"), MimeStatic(".ai", "application/postscript"), MimeStatic(".aif", "audio/x-aiff"), MimeStatic(".aifc", "audio/x-aiff"), MimeStatic(".aiff", "audio/x-aiff"), MimeStatic(".aim", "application/x-aim"), MimeStatic(".aip", "text/x-audiosoft-intra"), MimeStatic(".ani", "application/x-navi-animation"), MimeStatic(".aos", "application/x-nokia-9000-communicator-add-on-software"), MimeStatic(".aps", "application/mime"), MimeStatic(".arc", "application/octet-stream"), MimeStatic(".arj", "application/arj"), MimeStatic(".art", "image/x-jg"), MimeStatic(".asf", "video/x-ms-asf"), MimeStatic(".asm", "text/x-asm"), MimeStatic(".asp", "text/asp"), MimeStatic(".asx", "video/x-ms-asf"), MimeStatic(".au", "audio/x-au"), MimeStatic(".avi", "video/x-msvideo"), MimeStatic(".avs", "video/avs-video"), MimeStatic(".bcpio", "application/x-bcpio"), MimeStatic(".bin", "application/x-binary"), MimeStatic(".bm", "image/bmp"), MimeStatic(".bmp", "image/bmp"), MimeStatic(".boo", "application/book"), MimeStatic(".book", "application/book"), MimeStatic(".boz", "application/x-bzip2"), MimeStatic(".bsh", "application/x-bsh"), MimeStatic(".bz", "application/x-bzip"), MimeStatic(".bz2", "application/x-bzip2"), MimeStatic(".c", "text/x-c"), MimeStatic(".c++", "text/x-c"), MimeStatic(".cat", "application/vnd.ms-pki.seccat"), MimeStatic(".cc", "text/x-c"), MimeStatic(".ccad", "application/clariscad"), MimeStatic(".cco", "application/x-cocoa"), MimeStatic(".cdf", "application/x-cdf"), MimeStatic(".cer", "application/pkix-cert"), MimeStatic(".cff", "application/font-sfnt"), MimeStatic(".cha", "application/x-chat"), MimeStatic(".chat", "application/x-chat"), MimeStatic(".class", "application/x-java-class"), MimeStatic(".com", "application/octet-stream"), MimeStatic(".conf", "text/plain"), MimeStatic(".cpio", "application/x-cpio"), MimeStatic(".cpp", "text/x-c"), MimeStatic(".cpt", "application/x-compactpro"), MimeStatic(".crl", "application/pkcs-crl"), MimeStatic(".crt", "application/x-x509-user-cert"), MimeStatic(".csh", "text/x-script.csh"), MimeStatic(".css", "text/css"), MimeStatic(".csv", "text/csv"), MimeStatic(".cxx", "text/plain"), MimeStatic(".dcr", "application/x-director"), MimeStatic(".deepv", "application/x-deepv"), MimeStatic(".def", "text/plain"), MimeStatic(".der", "application/x-x509-ca-cert"), MimeStatic(".dif", "video/x-dv"), MimeStatic(".dir", "application/x-director"), MimeStatic(".dl", "video/x-dl"), MimeStatic(".dll", "application/octet-stream"), MimeStatic(".doc", "application/msword"), MimeStatic(".dot", "application/msword"), MimeStatic(".dp", "application/commonground"), MimeStatic(".drw", "application/drafting"), MimeStatic(".dump", "application/octet-stream"), MimeStatic(".dv", "video/x-dv"), MimeStatic(".dvi", "application/x-dvi"), MimeStatic(".dwf", "model/vnd.dwf"), MimeStatic(".dwg", "image/vnd.dwg"), MimeStatic(".dxf", "image/vnd.dwg"), MimeStatic(".dxr", "application/x-director"), MimeStatic(".el", "text/x-script.elisp"), MimeStatic(".elc", "application/x-bytecode.elisp"), MimeStatic(".env", "application/x-envoy"), MimeStatic(".eps", "application/postscript"), MimeStatic(".es", "application/x-esrehber"), MimeStatic(".etx", "text/x-setext"), MimeStatic(".evy", "application/x-envoy"), MimeStatic(".exe", "application/octet-stream"), MimeStatic(".f", "text/x-fortran"), MimeStatic(".f77", "text/x-fortran"), MimeStatic(".f90", "text/x-fortran"), MimeStatic(".fdf", "application/vnd.fdf"), MimeStatic(".fif", "image/fif"), MimeStatic(".fli", "video/x-fli"), MimeStatic(".flo", "image/florian"), MimeStatic(".flx", "text/vnd.fmi.flexstor"), MimeStatic(".fmf", "video/x-atomic3d-feature"), MimeStatic(".for", "text/x-fortran"), MimeStatic(".fpx", "image/vnd.fpx"), MimeStatic(".frl", "application/freeloader"), MimeStatic(".funk", "audio/make"), MimeStatic(".g", "text/plain"), MimeStatic(".g3", "image/g3fax"), MimeStatic(".gif", "image/gif"), MimeStatic(".gl", "video/x-gl"), MimeStatic(".gsd", "audio/x-gsm"), MimeStatic(".gsm", "audio/x-gsm"), MimeStatic(".gsp", "application/x-gsp"), MimeStatic(".gss", "application/x-gss"), MimeStatic(".gtar", "application/x-gtar"), MimeStatic(".gz", "application/x-gzip"), MimeStatic(".h", "text/x-h"), MimeStatic(".hdf", "application/x-hdf"), MimeStatic(".help", "application/x-helpfile"), MimeStatic(".hgl", "application/vnd.hp-hpgl"), MimeStatic(".hh", "text/x-h"), MimeStatic(".hlb", "text/x-script"), MimeStatic(".hlp", "application/x-helpfile"), MimeStatic(".hpg", "application/vnd.hp-hpgl"), MimeStatic(".hpgl", "application/vnd.hp-hpgl"), MimeStatic(".hqx", "application/binhex"), MimeStatic(".hta", "application/hta"), MimeStatic(".htc", "text/x-component"), MimeStatic(".htm", "text/html"), MimeStatic(".html", "text/html"), MimeStatic(".htmls", "text/html"), MimeStatic(".htt", "text/webviewhtml"), MimeStatic(".htx", "text/html"), MimeStatic(".ice", "x-conference/x-cooltalk"), MimeStatic(".ico", "image/x-icon"), MimeStatic(".idc", "text/plain"), MimeStatic(".ief", "image/ief"), MimeStatic(".iefs", "image/ief"), MimeStatic(".iges", "model/iges"), MimeStatic(".igs", "model/iges"), MimeStatic(".ima", "application/x-ima"), MimeStatic(".imap", "application/x-httpd-imap"), MimeStatic(".inf", "application/inf"), MimeStatic(".ins", "application/x-internett-signup"), MimeStatic(".ip", "application/x-ip2"), MimeStatic(".isu", "video/x-isvideo"), MimeStatic(".it", "audio/it"), MimeStatic(".iv", "application/x-inventor"), MimeStatic(".ivr", "i-world/i-vrml"), MimeStatic(".ivy", "application/x-livescreen"), MimeStatic(".jam", "audio/x-jam"), MimeStatic(".jav", "text/x-java-source"), MimeStatic(".java", "text/x-java-source"), MimeStatic(".jcm", "application/x-java-commerce"), MimeStatic(".jfif", "image/jpeg"), MimeStatic(".jfif-tbnl", "image/jpeg"), MimeStatic(".jpe", "image/jpeg"), MimeStatic(".jpeg", "image/jpeg"), MimeStatic(".jpg", "image/jpeg"), MimeStatic(".jpm", "image/jpm"), MimeStatic(".jps", "image/x-jps"), MimeStatic(".jpx", "image/jpx"), MimeStatic(".js", "application/x-javascript"), MimeStatic(".json", "application/json"), MimeStatic(".jut", "image/jutvision"), MimeStatic(".kar", "music/x-karaoke"), MimeStatic(".kml", "application/vnd.google-earth.kml+xml"), MimeStatic(".kmz", "application/vnd.google-earth.kmz"), MimeStatic(".ksh", "text/x-script.ksh"), MimeStatic(".la", "audio/x-nspaudio"), MimeStatic(".lam", "audio/x-liveaudio"), MimeStatic(".latex", "application/x-latex"), MimeStatic(".lha", "application/x-lha"), MimeStatic(".lhx", "application/octet-stream"), MimeStatic(".lib", "application/octet-stream"), MimeStatic(".list", "text/plain"), MimeStatic(".lma", "audio/x-nspaudio"), MimeStatic(".log", "text/plain"), MimeStatic(".lsp", "text/x-script.lisp"), MimeStatic(".lst", "text/plain"), MimeStatic(".lsx", "text/x-la-asf"), MimeStatic(".ltx", "application/x-latex"), MimeStatic(".lzh", "application/x-lzh"), MimeStatic(".lzx", "application/x-lzx"), MimeStatic(".m", "text/x-m"), MimeStatic(".m1v", "video/mpeg"), MimeStatic(".m2a", "audio/mpeg"), MimeStatic(".m2v", "video/mpeg"), MimeStatic(".m3u", "audio/x-mpegurl"), MimeStatic(".m4v", "video/x-m4v"), MimeStatic(".man", "application/x-troff-man"), MimeStatic(".map", "application/x-navimap"), MimeStatic(".mar", "text/plain"), MimeStatic(".mbd", "application/mbedlet"), MimeStatic(".mc$", "application/x-magic-cap-package-1.0"), MimeStatic(".mcd", "application/x-mathcad"), MimeStatic(".mcf", "text/mcf"), MimeStatic(".mcp", "application/netmc"), MimeStatic(".me", "application/x-troff-me"), MimeStatic(".mht", "message/rfc822"), MimeStatic(".mhtml", "message/rfc822"), MimeStatic(".mid", "audio/x-midi"), MimeStatic(".midi", "audio/x-midi"), MimeStatic(".mif", "application/x-mif"), MimeStatic(".mime", "www/mime"), MimeStatic(".mjf", "audio/x-vnd.audioexplosion.mjuicemediafile"), MimeStatic(".mjpg", "video/x-motion-jpeg"), MimeStatic(".mm", "application/base64"), MimeStatic(".mme", "application/base64"), MimeStatic(".mod", "audio/x-mod"), MimeStatic(".moov", "video/quicktime"), MimeStatic(".mov", "video/quicktime"), MimeStatic(".movie", "video/x-sgi-movie"), MimeStatic(".mp2", "video/x-mpeg"), MimeStatic(".mp3", "audio/x-mpeg-3"), MimeStatic(".mp4", "video/mp4"), MimeStatic(".mpa", "audio/mpeg"), MimeStatic(".mpc", "application/x-project"), MimeStatic(".mpeg", "video/mpeg"), MimeStatic(".mpg", "video/mpeg"), MimeStatic(".mpga", "audio/mpeg"), MimeStatic(".mpp", "application/vnd.ms-project"), MimeStatic(".mpt", "application/x-project"), MimeStatic(".mpv", "application/x-project"), MimeStatic(".mpx", "application/x-project"), MimeStatic(".mrc", "application/marc"), MimeStatic(".ms", "application/x-troff-ms"), MimeStatic(".mv", "video/x-sgi-movie"), MimeStatic(".my", "audio/make"), MimeStatic(".mzz", "application/x-vnd.audioexplosion.mzz"), MimeStatic(".nap", "image/naplps"), MimeStatic(".naplps", "image/naplps"), MimeStatic(".nc", "application/x-netcdf"), MimeStatic(".ncm", "application/vnd.nokia.configuration-message"), MimeStatic(".nif", "image/x-niff"), MimeStatic(".niff", "image/x-niff"), MimeStatic(".nix", "application/x-mix-transfer"), MimeStatic(".nsc", "application/x-conference"), MimeStatic(".nvd", "application/x-navidoc"), MimeStatic(".o", "application/octet-stream"), MimeStatic(".obj", "application/octet-stream"), MimeStatic(".oda", "application/oda"), MimeStatic(".oga", "audio/ogg"), MimeStatic(".ogg", "audio/ogg"), MimeStatic(".ogv", "video/ogg"), MimeStatic(".omc", "application/x-omc"), MimeStatic(".omcd", "application/x-omcdatamaker"), MimeStatic(".omcr", "application/x-omcregerator"), MimeStatic(".otf", "application/font-sfnt"), MimeStatic(".p", "text/x-pascal"), MimeStatic(".p10", "application/x-pkcs10"), MimeStatic(".p12", "application/x-pkcs12"), MimeStatic(".p7a", "application/x-pkcs7-signature"), MimeStatic(".p7c", "application/x-pkcs7-mime"), MimeStatic(".p7m", "application/x-pkcs7-mime"), MimeStatic(".p7r", "application/x-pkcs7-certreqresp"), MimeStatic(".p7s", "application/pkcs7-signature"), MimeStatic(".part", "application/pro_eng"), MimeStatic(".pas", "text/x-pascal"), MimeStatic(".pbm", "image/x-portable-bitmap"), MimeStatic(".pcl", "application/vnd.hp-pcl"), MimeStatic(".pct", "image/x-pct"), MimeStatic(".pcx", "image/x-pcx"), MimeStatic(".pdb", "chemical/x-pdb"), MimeStatic(".pdf", "application/pdf"), MimeStatic(".pfr", "application/font-tdpfr"), MimeStatic(".pfunk", "audio/make"), MimeStatic(".pgm", "image/x-portable-greymap"), MimeStatic(".pic", "image/pict"), MimeStatic(".pict", "image/pict"), MimeStatic(".pkg", "application/x-newton-compatible-pkg"), MimeStatic(".pko", "application/vnd.ms-pki.pko"), MimeStatic(".pl", "text/x-script.perl"), MimeStatic(".plx", "application/x-pixelscript"), MimeStatic(".pm", "text/x-script.perl-module"), MimeStatic(".pm4", "application/x-pagemaker"), MimeStatic(".pm5", "application/x-pagemaker"), MimeStatic(".png", "image/png"), MimeStatic(".pnm", "image/x-portable-anymap"), MimeStatic(".pot", "application/vnd.ms-powerpoint"), MimeStatic(".pov", "model/x-pov"), MimeStatic(".ppa", "application/vnd.ms-powerpoint"), MimeStatic(".ppm", "image/x-portable-pixmap"), MimeStatic(".pps", "application/vnd.ms-powerpoint"), MimeStatic(".ppt", "application/vnd.ms-powerpoint"), MimeStatic(".ppz", "application/vnd.ms-powerpoint"), MimeStatic(".pre", "application/x-freelance"), MimeStatic(".prt", "application/pro_eng"), MimeStatic(".ps", "application/postscript"), MimeStatic(".psd", "application/octet-stream"), MimeStatic(".pvu", "paleovu/x-pv"), MimeStatic(".pwz", "application/vnd.ms-powerpoint"), MimeStatic(".py", "text/x-script.python"), MimeStatic(".pyc", "application/x-bytecode.python"), MimeStatic(".qcp", "audio/vnd.qcelp"), MimeStatic(".qd3", "x-world/x-3dmf"), MimeStatic(".qd3d", "x-world/x-3dmf"), MimeStatic(".qif", "image/x-quicktime"), MimeStatic(".qt", "video/quicktime"), MimeStatic(".qtc", "video/x-qtc"), MimeStatic(".qti", "image/x-quicktime"), MimeStatic(".qtif", "image/x-quicktime"), MimeStatic(".ra", "audio/x-pn-realaudio"), MimeStatic(".ram", "audio/x-pn-realaudio"), MimeStatic(".rar", "application/x-arj-compressed"), MimeStatic(".ras", "image/x-cmu-raster"), MimeStatic(".rast", "image/cmu-raster"), MimeStatic(".rexx", "text/x-script.rexx"), MimeStatic(".rf", "image/vnd.rn-realflash"), MimeStatic(".rgb", "image/x-rgb"), MimeStatic(".rm", "audio/x-pn-realaudio"), MimeStatic(".rmi", "audio/mid"), MimeStatic(".rmm", "audio/x-pn-realaudio"), MimeStatic(".rmp", "audio/x-pn-realaudio"), MimeStatic(".rng", "application/vnd.nokia.ringing-tone"), MimeStatic(".rnx", "application/vnd.rn-realplayer"), MimeStatic(".roff", "application/x-troff"), MimeStatic(".rp", "image/vnd.rn-realpix"), MimeStatic(".rpm", "audio/x-pn-realaudio-plugin"), MimeStatic(".rt", "text/vnd.rn-realtext"), MimeStatic(".rtf", "application/x-rtf"), MimeStatic(".rtx", "application/x-rtf"), MimeStatic(".rv", "video/vnd.rn-realvideo"), MimeStatic(".s", "text/x-asm"), MimeStatic(".s3m", "audio/s3m"), MimeStatic(".saveme", "application/octet-stream"), MimeStatic(".sbk", "application/x-tbook"), MimeStatic(".scm", "text/x-script.scheme"), MimeStatic(".sdml", "text/plain"), MimeStatic(".sdp", "application/x-sdp"), MimeStatic(".sdr", "application/sounder"), MimeStatic(".sea", "application/x-sea"), MimeStatic(".set", "application/set"), MimeStatic(".sgm", "text/x-sgml"), MimeStatic(".sgml", "text/x-sgml"), MimeStatic(".sh", "text/x-script.sh"), MimeStatic(".shar", "application/x-shar"), MimeStatic(".shtm", "text/html"), MimeStatic(".shtml", "text/html"), MimeStatic(".sid", "audio/x-psid"), MimeStatic(".sil", "application/font-sfnt"), MimeStatic(".sit", "application/x-sit"), MimeStatic(".skd", "application/x-koan"), MimeStatic(".skm", "application/x-koan"), MimeStatic(".skp", "application/x-koan"), MimeStatic(".skt", "application/x-koan"), MimeStatic(".sl", "application/x-seelogo"), MimeStatic(".smi", "application/smil"), MimeStatic(".smil", "application/smil"), MimeStatic(".snd", "audio/x-adpcm"), MimeStatic(".so", "application/octet-stream"), MimeStatic(".sol", "application/solids"), MimeStatic(".spc", "text/x-speech"), MimeStatic(".spl", "application/futuresplash"), MimeStatic(".spr", "application/x-sprite"), MimeStatic(".sprite", "application/x-sprite"), MimeStatic(".src", "application/x-wais-source"), MimeStatic(".ssi", "text/x-server-parsed-html"), MimeStatic(".ssm", "application/streamingmedia"), MimeStatic(".sst", "application/vnd.ms-pki.certstore"), MimeStatic(".step", "application/step"), MimeStatic(".stl", "application/vnd.ms-pki.stl"), MimeStatic(".stp", "application/step"), MimeStatic(".sv4cpio", "application/x-sv4cpio"), MimeStatic(".sv4crc", "application/x-sv4crc"), MimeStatic(".svf", "image/x-dwg"), MimeStatic(".svg", "image/svg+xml"), MimeStatic(".svr", "x-world/x-svr"), MimeStatic(".swf", "application/x-shockwave-flash"), MimeStatic(".t", "application/x-troff"), MimeStatic(".talk", "text/x-speech"), MimeStatic(".tar", "application/x-tar"), MimeStatic(".tbk", "application/x-tbook"), MimeStatic(".tcl", "text/x-script.tcl"), MimeStatic(".tcsh", "text/x-script.tcsh"), MimeStatic(".tex", "application/x-tex"), MimeStatic(".texi", "application/x-texinfo"), MimeStatic(".texinfo", "application/x-texinfo"), MimeStatic(".text", "text/plain"), MimeStatic(".tgz", "application/x-compressed"), MimeStatic(".tif", "image/x-tiff"), MimeStatic(".tiff", "image/x-tiff"), MimeStatic(".torrent", "application/x-bittorrent"), MimeStatic(".tr", "application/x-troff"), MimeStatic(".tsi", "audio/tsp-audio"), MimeStatic(".tsp", "audio/tsplayer"), MimeStatic(".tsv", "text/tab-separated-values"), MimeStatic(".ttf", "application/font-sfnt"), MimeStatic(".turbot", "image/florian"), MimeStatic(".txt", "text/plain"), MimeStatic(".uil", "text/x-uil"), MimeStatic(".uni", "text/uri-list"), MimeStatic(".unis", "text/uri-list"), MimeStatic(".unv", "application/i-deas"), MimeStatic(".uri", "text/uri-list"), MimeStatic(".uris", "text/uri-list"), MimeStatic(".ustar", "application/x-ustar"), MimeStatic(".uu", "text/x-uuencode"), MimeStatic(".uue", "text/x-uuencode"), MimeStatic(".vcd", "application/x-cdlink"), MimeStatic(".vcs", "text/x-vcalendar"), MimeStatic(".vda", "application/vda"), MimeStatic(".vdo", "video/vdo"), MimeStatic(".vew", "application/groupwise"), MimeStatic(".viv", "video/vnd.vivo"), MimeStatic(".vivo", "video/vnd.vivo"), MimeStatic(".vmd", "application/vocaltec-media-desc"), MimeStatic(".vmf", "application/vocaltec-media-resource"), MimeStatic(".voc", "audio/x-voc"), MimeStatic(".vos", "video/vosaic"), MimeStatic(".vox", "audio/voxware"), MimeStatic(".vqe", "audio/x-twinvq-plugin"), MimeStatic(".vqf", "audio/x-twinvq"), MimeStatic(".vql", "audio/x-twinvq-plugin"), MimeStatic(".vrml", "model/vrml"), MimeStatic(".vrt", "x-world/x-vrt"), MimeStatic(".vsd", "application/x-visio"), MimeStatic(".vst", "application/x-visio"), MimeStatic(".vsw", "application/x-visio"), MimeStatic(".w60", "application/wordperfect6.0"), MimeStatic(".w61", "application/wordperfect6.1"), MimeStatic(".w6w", "application/msword"), MimeStatic(".wav", "audio/x-wav"), MimeStatic(".wb1", "application/x-qpro"), MimeStatic(".wbmp", "image/vnd.wap.wbmp"), MimeStatic(".web", "application/vnd.xara"), MimeStatic(".webm", "video/webm"), MimeStatic(".wiz", "application/msword"), MimeStatic(".wk1", "application/x-123"), MimeStatic(".wmf", "windows/metafile"), MimeStatic(".wml", "text/vnd.wap.wml"), MimeStatic(".wmlc", "application/vnd.wap.wmlc"), MimeStatic(".wmls", "text/vnd.wap.wmlscript"), MimeStatic(".wmlsc", "application/vnd.wap.wmlscriptc"), MimeStatic(".woff", "application/font-woff"), MimeStatic(".word", "application/msword"), MimeStatic(".wp", "application/wordperfect"), MimeStatic(".wp5", "application/wordperfect"), MimeStatic(".wp6", "application/wordperfect"), MimeStatic(".wpd", "application/wordperfect"), MimeStatic(".wq1", "application/x-lotus"), MimeStatic(".wri", "application/x-wri"), MimeStatic(".wrl", "model/vrml"), MimeStatic(".wrz", "model/vrml"), MimeStatic(".wsc", "text/scriplet"), MimeStatic(".wsrc", "application/x-wais-source"), MimeStatic(".wtk", "application/x-wintalk"), MimeStatic(".x-png", "image/png"), MimeStatic(".xbm", "image/x-xbm"), MimeStatic(".xdr", "video/x-amt-demorun"), MimeStatic(".xgz", "xgl/drawing"), MimeStatic(".xhtml", "application/xhtml+xml"), MimeStatic(".xif", "image/vnd.xiff"), MimeStatic(".xl", "application/vnd.ms-excel"), MimeStatic(".xla", "application/vnd.ms-excel"), MimeStatic(".xlb", "application/vnd.ms-excel"), MimeStatic(".xlc", "application/vnd.ms-excel"), MimeStatic(".xld", "application/vnd.ms-excel"), MimeStatic(".xlk", "application/vnd.ms-excel"), MimeStatic(".xll", "application/vnd.ms-excel"), MimeStatic(".xlm", "application/vnd.ms-excel"), MimeStatic(".xls", "application/vnd.ms-excel"), MimeStatic(".xlt", "application/vnd.ms-excel"), MimeStatic(".xlv", "application/vnd.ms-excel"), MimeStatic(".xlw", "application/vnd.ms-excel"), MimeStatic(".xm", "audio/xm"), MimeStatic(".xml", "text/xml"), MimeStatic(".xmz", "xgl/movie"), MimeStatic(".xpix", "application/x-vnd.ls-xpix"), MimeStatic(".xpm", "image/x-xpixmap"), MimeStatic(".xsl", "application/xml"), MimeStatic(".xslt", "application/xml"), MimeStatic(".xsr", "video/x-amt-showrun"), MimeStatic(".xwd", "image/x-xwd"), MimeStatic(".xyz", "chemical/x-pdb"), MimeStatic(".z", "application/x-compressed"), MimeStatic(".zip", "application/x-zip-compressed"), MimeStatic(".zoo", "application/octet-stream"), MimeStatic(".zsh", "text/x-script.zsh") };

				size_t PathLength = Path.size();
				while (PathLength >= 1 && Path[PathLength - 1] != '.')
					PathLength--;

				if (!PathLength)
					return "application/octet-stream";

				const char* Ptr = Path.data();
				const char* Ext = &Ptr[PathLength - 1];
				int End = ((int)(sizeof(MimeTypes) / sizeof(MimeTypes[0])));
				int Start = 0, Result, Index;

				while (End - Start > 1)
				{
					Index = (Start + End) >> 1;
					Result = Core::Stringify::CaseCompare(Ext, MimeTypes[Index].Extension);
					if (Result == 0)
						return MimeTypes[Index].Type;
					else if (Result < 0)
						End = Index;
					else
						Start = Index;
				}

				if (Core::Stringify::CaseEquals(Ext, MimeTypes[Start].Extension))
					return MimeTypes[Start].Type;

				if (Types != nullptr && !Types->empty())
				{
					for (auto& Item : *Types)
					{
						if (Core::Stringify::CaseEquals(Ext, Item.Extension.c_str()))
							return Item.Type.c_str();
					}
				}

				return "application/octet-stream";
			}
			std::string_view Utils::StatusMessage(int StatusCode)
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
						return "Request URL Too Large";
					case 415:
						return "Unsupported Media Type";
					case 416:
						return "Requested Range Not Satisfiable";
					case 417:
						return "Expectation Failed";
					case 418:
						return "I'm a teapot";
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
						return "Gateway Timeout";
					case 505:
						return "Version Not Supported";
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
							return "Informational";

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

				return "Unknown";
			}

			void Paths::ConstructPath(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				auto* Route = Base->Route;
				if (!Route->Alias.empty())
				{
					Base->Request.Path.assign(Route->Alias);
					if (Route->Router->Callbacks.OnLocation)
						Route->Router->Callbacks.OnLocation(Base);
					return;
				}
				else if (Route->FilesDirectory.empty())
					return;

				for (size_t i = 0; i < Base->Request.Location.size(); i++)
				{
					if (Base->Request.Location[i] == '%' && i + 1 < Base->Request.Location.size())
					{
						if (Base->Request.Location[i + 1] == 'u')
						{
							int Value = 0;
							if (Compute::Codec::HexToDecimal(Base->Request.Location, i + 2, 4, Value))
							{
								char Buffer[4];
								size_t LCount = Compute::Codec::Utf8(Value, Buffer);
								if (LCount > 0)
									Base->Request.Path.append(Buffer, LCount);
								i += 5;
							}
							else
								Base->Request.Path += Base->Request.Location[i];
						}
						else
						{
							int Value = 0;
							if (Compute::Codec::HexToDecimal(Base->Request.Location, i + 1, 2, Value))
							{
								Base->Request.Path += Value;
								i += 2;
							}
							else
								Base->Request.Path += Base->Request.Location[i];
						}
					}
					else if (Base->Request.Location[i] == '+')
						Base->Request.Path += ' ';
					else
						Base->Request.Path += Base->Request.Location[i];
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

				int64_t Size = Buffer - Next;
				if (Size > 0 && Size != (int64_t)Base->Request.Path.size())
					Base->Request.Path.resize(Base->Request.Path.size() - (size_t)Size);

				if (Base->Request.Path.size() > 1 && !Base->Request.Match.Empty())
				{
					auto& Match = Base->Request.Match.Get()[0];
					size_t Start = std::min<size_t>(Base->Request.Path.size(), (size_t)Match.Start);
					size_t End = std::min<size_t>(Base->Request.Path.size(), (size_t)Match.End);
					Core::Stringify::RemovePart(Base->Request.Path, Start, End);
				}
#ifdef VI_MICROSOFT
				Core::Stringify::Replace(Base->Request.Path, '/', '\\');
#endif
				Base->Request.Path = Route->FilesDirectory + Base->Request.Path;
				auto Path = Core::OS::Path::Resolve(Base->Request.Path.c_str());
				if (Path)
					Base->Request.Path = *Path;

				bool PathTrailing = PathTrailingCheck(Base->Request.Path);
				bool LocationTrailing = PathTrailingCheck(Base->Request.Location);
				if (PathTrailing != LocationTrailing)
				{
					if (LocationTrailing)
						Base->Request.Path.append(1, VI_SPLITTER);
					else
						Base->Request.Path.erase(Base->Request.Path.size() - 1, 1);
				}

				if (Route->Router->Callbacks.OnLocation)
					Route->Router->Callbacks.OnLocation(Base);
			}
			void Paths::ConstructHeadFull(RequestFrame* Request, ResponseFrame* Response, bool IsRequest, Core::String& Buffer)
			{
				VI_ASSERT(Request != nullptr, "connection should be set");
				VI_ASSERT(Response != nullptr, "response should be set");

				KimvUnorderedMap& Headers = (IsRequest ? Request->Headers : Response->Headers);
				for (auto& Item : Headers)
				{
					for (auto& Payload : Item.second)
						Buffer.append(Item.first).append(": ").append(Payload).append("\r\n");
				}

				if (IsRequest)
					return;

				for (auto& Item : Response->Cookies)
				{
					if (Item.Name.empty())
						continue;

					Buffer.append("Set-Cookie: ").append(Item.Name).append("=").append(Item.Value);
					if (!Item.Expires.empty())
						Buffer.append("; Expires=").append(Item.Expires);
					if (!Item.Domain.empty())
						Buffer.append("; Domain=").append(Item.Domain);
					if (!Item.Path.empty())
						Buffer.append("; Path=").append(Item.Path);
					if (!Item.SameSite.empty())
						Buffer.append("; SameSite=").append(Item.SameSite);
					if (Item.Secure)
						Buffer.append("; SameSite");
					if (Item.HttpOnly)
						Buffer.append("; HttpOnly");
					Buffer.append("\r\n");
				}
			}
			void Paths::ConstructHeadCache(Connection* Base, Core::String& Buffer)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				if (!Base->Route->StaticFileMaxAge)
					return ConstructHeadUncache(Buffer);

				Buffer.append("Cache-Control: max-age=");
				Buffer.append(Core::ToString(Base->Route->StaticFileMaxAge));
				Buffer.append("\r\n");
			}
			void Paths::ConstructHeadUncache(Core::String& Buffer)
			{
				Buffer.append(
					"Cache-Control: no-cache, no-store, must-revalidate, private, max-age=0\r\n"
					"Pragma: no-cache\r\n"
					"Expires: 0\r\n", 102);
			}
			bool Paths::ConstructRoute(MapRouter* Router, Connection* Base)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				VI_ASSERT(Router != nullptr && Router->Base != nullptr, "router should be valid");
				if (Base->Request.Location.empty() || Base->Request.Location.front() != '/')
					return false;

				if (Router->Listeners.size() > 1)
				{
					auto* Host = Base->Request.GetHeaderBlob("Host");
					if (!Host)
						return false;

					auto Listen = Router->Listeners.find(*Host);
					if (Listen == Router->Listeners.end() && Router->Listeners.find("*") == Router->Listeners.end())
						return false;
				}
				else
				{
					auto Listen = Router->Listeners.begin();
					if (Listen->first.size() != 1 || Listen->first.front() != '*')
					{
						auto* Host = Base->Request.GetHeaderBlob("Host");
						if (!Host || Listen->first != *Host)
							return false;
					}
				}

				Base->Request.Referrer = Base->Request.Location;
				if (Base->Request.Location.size() == 1)
				{
					Base->Route = Router->Base;
					return true;
				}

				for (auto& Group : Router->Groups)
				{
					Core::String& Location = Base->Request.Location;
					if (!Group->Match.empty())
					{
						if (Group->Mode == RouteMode::Exact)
						{
							if (Location != Group->Match)
								continue;

							Location.clear();
						}
						else if (Group->Mode == RouteMode::Start)
						{
							if (!Core::Stringify::StartsWith(Location, Group->Match))
								continue;

							Location.erase(Location.begin(), Location.begin() + Group->Match.size());
						}
						else if (Group->Mode == RouteMode::Match)
						{
							if (!Core::Stringify::Find(Location, Group->Match).Found)
								continue;
						}
						else if (Group->Mode == RouteMode::End)
						{
							if (!Core::Stringify::EndsWith(Location, Group->Match))
								continue;

							Location.erase(Location.end() - Group->Match.size(), Location.end());
						}

						if (Location.empty())
							Location.append(1, '/');
						else if (Location.front() != '/')
							Location.insert(Location.begin(), '/');

						for (auto* Next : Group->Routes)
						{
							VI_ASSERT(Next != nullptr, "route should be set");
							if (Compute::Regex::Match(&Next->Location, Base->Request.Match, Location))
							{
								Base->Route = Next;
								return true;
							}
						}

						Location.assign(Base->Request.Referrer);
					}
					else
					{
						for (auto* Next : Group->Routes)
						{
							VI_ASSERT(Next != nullptr, "route should be set");
							if (Compute::Regex::Match(&Next->Location, Base->Request.Match, Location))
							{
								Base->Route = Next;
								return true;
							}
						}
					}
				}

				Base->Route = Router->Base;
				return true;
			}
			bool Paths::ConstructDirectoryEntries(Connection* Base, const Core::String& NameA, const Core::FileEntry& A, const Core::String& NameB, const Core::FileEntry& B)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				if (A.IsDirectory && !B.IsDirectory)
					return true;

				if (!A.IsDirectory && B.IsDirectory)
					return false;

				const char* Query = (Base->Request.Query.empty() ? nullptr : Base->Request.Query.c_str());
				if (Query != nullptr)
				{
					int Result = 0;
					if (*Query == 'n')
						Result = strcmp(NameA.c_str(), NameB.c_str());
					else if (*Query == 's')
						Result = (A.Size == B.Size) ? 0 : ((A.Size > B.Size) ? 1 : -1);
					else if (*Query == 'd')
						Result = (A.LastModified == B.LastModified) ? 0 : ((A.LastModified > B.LastModified) ? 1 : -1);

					if (Query[1] == 'a')
						return Result < 0;
					else if (Query[1] == 'd')
						return Result > 0;

					return Result < 0;
				}

				return strcmp(NameA.c_str(), NameB.c_str()) < 0;
			}
			Core::String Paths::ConstructContentRange(size_t Offset, size_t Length, size_t ContentLength)
			{
				Core::String Field = "bytes ";
				Field += Core::ToString(Offset);
				Field += '-';
				Field += Core::ToString(Offset + Length);
				Field += '/';
				Field += Core::ToString(ContentLength);

				return Field;
			}

			bool Parsing::ParseMultipartHeaderField(Parser* Parser, const uint8_t* Name, size_t Length)
			{
				return ParseHeaderField(Parser, Name, Length);
			}
			bool Parsing::ParseMultipartHeaderValue(Parser* Parser, const uint8_t* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");

				if (!Length || Parser->Multipart.Skip)
					return true;

				if (Parser->Message.Header.empty())
					return true;

				std::string_view Value = std::string_view((char*)Data, Length);
				if (Parser->Message.Header == "Content-Disposition")
				{
					Core::TextSettle Start = Core::Stringify::Find(Value, "name=\"");
					if (Start.Found)
					{
						Core::TextSettle End = Core::Stringify::Find(Value, '\"', Start.End);
						if (End.Found)
							Parser->Multipart.Data.Key = Value.substr(Start.End, End.End - Start.End - 1);
					}

					Start = Core::Stringify::Find(Value, "filename=\"");
					if (Start.Found)
					{
						Core::TextSettle End = Core::Stringify::Find(Value, '\"', Start.End);
						if (End.Found)
						{
							auto Name = Value.substr(Start.End, End.End - Start.End - 1);
							Parser->Multipart.Data.Name = Core::OS::Path::GetFilename(Name);
						}
					}
				}
				else if (Parser->Message.Header == "Content-Type")
					Parser->Multipart.Data.Type = Value;

				Parser->Multipart.Data.SetHeader(Parser->Message.Header.c_str(), Value);
				Parser->Message.Header.clear();
				return true;
			}
			bool Parsing::ParseMultipartContentData(Parser* Parser, const uint8_t* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");
				if (!Length)
					return true;

				if (Parser->Multipart.Skip || !Parser->Multipart.Stream)
					return false;

				if (fwrite(Data, 1, (size_t)Length, Parser->Multipart.Stream) != (size_t)Length)
					return false;

				Parser->Multipart.Data.Length += Length;
				return true;
			}
			bool Parsing::ParseMultipartResourceBegin(Parser* Parser)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				if (Parser->Multipart.Skip || !Parser->Message.Content)
					return true;

				if (Parser->Multipart.Stream != nullptr)
				{
					Core::OS::File::Close(Parser->Multipart.Stream);
					Parser->Multipart.Stream = nullptr;
					return false;
				}

				if (Parser->Message.Content->Resources.size() >= Parser->Multipart.MaxResources)
				{
					Parser->Multipart.Finish = true;
					return false;
				}

				Parser->Message.Header.clear();
				Parser->Multipart.Data.Headers.clear();
				Parser->Multipart.Data.Name.clear();
				Parser->Multipart.Data.Type = "application/octet-stream";
				Parser->Multipart.Data.IsInMemory = false;
				Parser->Multipart.Data.Length = 0;

				if (Parser->Multipart.TemporaryDirectory != nullptr)
				{
					Parser->Multipart.Data.Path = *Parser->Multipart.TemporaryDirectory;
					if (Parser->Multipart.Data.Path.back() != '/' && Parser->Multipart.Data.Path.back() != '\\')
						Parser->Multipart.Data.Path.append(1, VI_SPLITTER);

					auto Random = Compute::Crypto::RandomBytes(16);
					if (Random)
					{
						auto Hash = Compute::Crypto::HashHex(Compute::Digests::MD5(), *Random);
						if (Hash)
							Parser->Multipart.Data.Path.append(*Hash);
					}
				}

				auto File = Core::OS::File::Open(Parser->Multipart.Data.Path.c_str(), "wb");
				if (!File)
					return false;

				Parser->Multipart.Stream = *File;
				return true;
			}
			bool Parsing::ParseMultipartResourceEnd(Parser* Parser)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				if (Parser->Multipart.Skip || !Parser->Multipart.Stream || !Parser->Message.Content)
					return true;

				Core::OS::File::Close(Parser->Multipart.Stream);
				Parser->Multipart.Stream = nullptr;
				Parser->Message.Content->Resources.push_back(Parser->Multipart.Data);

				if (Parser->Multipart.Callback)
					Parser->Multipart.Callback(&Parser->Message.Content->Resources.back());

				return true;
			}
			bool Parsing::ParseHeaderField(Parser* Parser, const uint8_t* Name, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Name != nullptr, "name should be set");
				if (Length > 0)
					Parser->Message.Header.assign((char*)Name, Length);
				return true;
			}
			bool Parsing::ParseHeaderValue(Parser* Parser, const uint8_t* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");
				if (!Length || Parser->Message.Header.empty())
					return true;

				if (Core::Stringify::CaseEquals(Parser->Message.Header.c_str(), "cookie"))
				{
					if (!Parser->Message.Cookies)
						goto Success;

					Core::Vector<std::pair<Core::String, Core::String>> Cookies;
					const uint8_t* Offset = Data;

					for (size_t i = 0; i < Length; i++)
					{
						if (Data[i] != '=')
							continue;

						Core::String Name((char*)Offset, (size_t)((Data + i) - Offset));
						size_t Set = i;

						while (i + 1 < Length && Data[i] != ';')
							i++;

						if (Data[i] == ';')
							i--;

						Cookies.emplace_back(std::make_pair(std::move(Name), Core::String((char*)Data + Set + 1, i - Set)));
						Offset = Data + (i + 3);
					}

					for (auto&& Item : Cookies)
					{
						auto& Cookie = (*Parser->Message.Cookies)[Item.first];
						Cookie.emplace_back(std::move(Item.second));
					}
				}
				else if (Parser->Message.Headers != nullptr)
				{
					auto& Source = (*Parser->Message.Headers)[Parser->Message.Header]; bool AppendField = true;
					if (!Core::Stringify::CaseEquals(Parser->Message.Header.c_str(), "user-agent"))
					{
						if (Parser->Message.Header.find(',') != std::string::npos)
						{
							AppendField = false;
							Core::Stringify::PmSplit(Source, std::string_view((char*)Data, Length), ',');
							for (auto& Item : Source)
								Core::Stringify::Trim(Item);
						}
					}
					
					if (AppendField)
						Source.emplace_back(Core::String((char*)Data, Length));
				}

			Success:
				Parser->Message.Header.clear();
				return true;
			}
			bool Parsing::ParseVersion(Parser* Parser, const uint8_t* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");
				if (!Length || !Parser->Message.Version)
					return true;

				memset(Parser->Message.Version, 0, LABEL_SIZE);
				memcpy((void*)Parser->Message.Version, (void*)Data, std::min<size_t>(Length, LABEL_SIZE));
				return true;
			}
			bool Parsing::ParseStatusCode(Parser* Parser, size_t Value)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				if (Parser->Message.StatusCode != nullptr)
					*Parser->Message.StatusCode = (int)Value;
				return true;
			}
			bool Parsing::ParseStatusMessage(Parser* Parser, const uint8_t* Name, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				return true;
			}
			bool Parsing::ParseMethodValue(Parser* Parser, const uint8_t* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");
				if (!Length || !Parser->Message.Method)
					return true;

				memset(Parser->Message.Method, 0, LABEL_SIZE);
				memcpy((void*)Parser->Message.Method, (void*)Data, std::min<size_t>(Length, LABEL_SIZE));
				return true;
			}
			bool Parsing::ParsePathValue(Parser* Parser, const uint8_t* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");
				if (!Length || !Parser->Message.Location)
					return true;

				Parser->Message.Location->assign((char*)Data, Length);
				return true;
			}
			bool Parsing::ParseQueryValue(Parser* Parser, const uint8_t* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");
				if (!Length || !Parser->Message.Query)
					return true;

				Parser->Message.Query->assign((char*)Data, Length);
				return true;
			}
			int Parsing::ParseContentRange(const std::string_view& ContentRange, int64_t* Range1, int64_t* Range2)
			{
				VI_ASSERT(Core::Stringify::IsCString(ContentRange), "content range should be set");
				VI_ASSERT(Range1 != nullptr, "range 1 should be set");
				VI_ASSERT(Range2 != nullptr, "range 2 should be set");
				return sscanf(ContentRange.data(), "bytes=%" PRId64 "-%" PRId64, Range1, Range2);
			}
			Core::String Parsing::ParseMultipartDataBoundary()
			{
				static const char Data[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
				std::random_device SeedGenerator;
				std::mt19937 Engine(SeedGenerator());
				Core::String Result = "--sha1-digest-multipart-data-";
				for (int i = 0; i < 16; i++)
					Result += Data[Engine() % (sizeof(Data) - 1)];

				return Result;
			}

			Core::String Permissions::Authorize(const std::string_view& Username, const std::string_view& Password, const std::string_view& Type)
			{
				Core::String Body = Core::String(Username);
				Body.append(1, ':');
				Body.append(Password);

				Core::String Result = Core::String(Type);
				Result.append(1, ' ');
				Result.append(Compute::Codec::Base64Encode(Body));
				return Result;
			}
			bool Permissions::Authorize(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				auto* Route = Base->Route;
				if (!Route->Callbacks.Authorize || Route->Auth.Type.empty())
					return true;

				bool IsSupported = false;
				for (auto& Item : Route->Auth.Methods)
				{
					if (Item == Base->Request.Method)
					{
						IsSupported = true;
						break;
					}
				}

				if (!IsSupported && !Route->Auth.Methods.empty())
				{
					Base->Request.User.Type = Auth::Denied;
					Base->Abort(401, "Authorization method is not allowed");
					return false;
				}

				auto Authorization = Base->Request.GetHeader("Authorization");
				if (Authorization.empty())
				{
					Base->Request.User.Type = Auth::Denied;
					Base->Abort(401, "Provide authorization header to continue.");
					return false;
				}

				size_t Index = 0;
				while (Authorization[Index] != ' ' && Authorization[Index] != '\0')
					Index++;

				Core::String Type(Authorization.data(), Index);
				if (Type != Route->Auth.Type)
				{
					Base->Request.User.Type = Auth::Denied;
					Base->Abort(401, "Authorization type \"%s\" is not allowed.", Type.c_str());
					return false;
				}

				Base->Request.User.Token = Authorization.substr(Index + 1);
				if (Route->Callbacks.Authorize(Base, &Base->Request.User))
				{
					Base->Request.User.Type = Auth::Granted;
					return true;
				}

				Base->Request.User.Type = Auth::Denied;
				Base->Abort(401, "Invalid user access credentials were provided. Access denied.");
				return false;
			}
			bool Permissions::MethodAllowed(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				for (auto& Item : Base->Route->DisallowedMethods)
				{
					if (Item == Base->Request.Method)
						return false;
				}

				return true;
			}
			bool Permissions::WebSocketUpgradeAllowed(Connection* Base)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				auto Upgrade = Base->Request.GetHeader("Upgrade");
				if (Upgrade.empty() || !Core::Stringify::CaseEquals(Upgrade, "websocket"))
					return false;

				auto Connection = Base->Request.GetHeader("Connection");
				if (Connection.empty() || !Core::Stringify::CaseEquals(Connection, "upgrade"))
					return false;

				return true;
			}

			bool Resources::ResourceHasAlternative(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				if (Base->Route->TryFiles.empty())
					return false;

				for (auto& Item : Base->Route->TryFiles)
				{
					if (Core::OS::File::GetState(Item, &Base->Resource))
					{
						Base->Request.Path = Item;
						return true;
					}
				}

				return false;
			}
			bool Resources::ResourceHidden(Connection* Base, Core::String* Path)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				if (Base->Route->HiddenFiles.empty())
					return false;

				Compute::RegexResult Result;
				const auto& Value = (Path ? *Path : Base->Request.Path);
				for (auto& Item : Base->Route->HiddenFiles)
				{
					if (Compute::Regex::Match(&Item, Result, Value))
						return true;
				}

				return false;
			}
			bool Resources::ResourceIndexed(Connection* Base, Core::FileEntry* Resource)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				VI_ASSERT(Resource != nullptr, "resource should be set");
				if (Base->Route->IndexFiles.empty())
					return false;

				Core::String Path = Base->Request.Path;
				if (!Core::Stringify::EndsOf(Path, "/\\"))
					Path.append(1, VI_SPLITTER);

				for (auto& Item : Base->Route->IndexFiles)
				{
					if (Core::OS::File::GetState(Item, Resource))
					{
						Base->Request.Path.assign(Item);
						return true;
					}
					else if (Core::OS::File::GetState(Path + Item, Resource))
					{
						Base->Request.Path.assign(Path.append(Item));
						return true;
					}
				}

				return false;
			}
			bool Resources::ResourceModified(Connection* Base, Core::FileEntry* Resource)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				VI_ASSERT(Resource != nullptr, "resource should be set");

				auto CacheControl = Base->Request.GetHeader("Cache-Control");
				if (!CacheControl.empty() && (Core::Stringify::CaseEquals(CacheControl, "no-cache") || Core::Stringify::CaseEquals(CacheControl, "max-age=0")))
					return true;

				auto IfNoneMatch = Base->Request.GetHeader("If-None-Match");
				if (!IfNoneMatch.empty())
				{
					char ETag[64];
					Core::OS::Net::GetETag(ETag, sizeof(ETag), Resource);
					if (Core::Stringify::CaseEquals(ETag, IfNoneMatch))
						return false;
				}

				auto IfModifiedSince = Base->Request.GetHeader("If-Modified-Since");
				return !(!IfModifiedSince.empty() && Resource->LastModified <= Core::DateTime::ParseWebDate(IfModifiedSince.data()));

			}
			bool Resources::ResourceCompressed(Connection* Base, size_t Size)
			{
#ifdef VI_ZLIB
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				auto* Route = Base->Route;
				if (!Route->Compression.Enabled || Size < Route->Compression.MinLength)
					return false;

				if (Route->Compression.Files.empty())
					return true;

				Compute::RegexResult Result;
				for (auto& Item : Route->Compression.Files)
				{
					if (Compute::Regex::Match(&Item, Result, Base->Request.Path))
						return true;
				}

				return false;
#else
				return false;
#endif
			}

			bool Routing::RouteWebSocket(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				if (!Base->Route->AllowWebSocket)
					return Base->Abort(404, "Websocket protocol is not allowed on this server.");

				auto* WebSocketKey = Base->Request.GetHeaderBlob("Sec-WebSocket-Key");
				if (WebSocketKey != nullptr)
					return Logical::ProcessWebSocket(Base, (uint8_t*)WebSocketKey->c_str(), WebSocketKey->size());

				auto WebSocketKey1 = Base->Request.GetHeader("Sec-WebSocket-Key1");
				if (WebSocketKey1.empty())
					return Base->Abort(400, "Malformed websocket request. Provide first key.");

				auto WebSocketKey2 = Base->Request.GetHeader("Sec-WebSocket-Key2");
				if (WebSocketKey2.empty())
					return Base->Abort(400, "Malformed websocket request. Provide second key.");

				const size_t LegacyKeySize = 8;
				auto ResolveConnection = [Base](SocketPoll Event, const uint8_t* Buffer, size_t Recv)
				{
					if (Packet::IsData(Event))
						Base->Request.Content.Append(std::string_view((char*)Buffer, Recv));
					else if (Packet::IsDone(Event))
						Logical::ProcessWebSocket(Base, (uint8_t*)Base->Request.Content.Data.data(), LegacyKeySize);
					else if (Packet::IsError(Event))
						Base->Abort();
					return true;
				};
				if (Base->Request.Content.Prefetch >= LegacyKeySize)
					return ResolveConnection(SocketPoll::FinishSync, (uint8_t*)"", 0);

				return !!Base->Stream->ReadQueued(LegacyKeySize - Base->Request.Content.Prefetch, ResolveConnection);
			}
			bool Routing::RouteGet(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				if (Base->Route->FilesDirectory.empty() || !Core::OS::File::GetState(Base->Request.Path, &Base->Resource))
				{
					if (Permissions::WebSocketUpgradeAllowed(Base))
						return RouteWebSocket(Base);

					if (!Resources::ResourceHasAlternative(Base))
						return Base->Abort(404, "Requested resource was not found.");
				}

				if (Permissions::WebSocketUpgradeAllowed(Base))
					return RouteWebSocket(Base);

				if (Resources::ResourceHidden(Base, nullptr))
					return Base->Abort(404, "Requested resource was not found.");

				if (Base->Resource.IsDirectory && !Resources::ResourceIndexed(Base, &Base->Resource))
				{
					if (Base->Route->AllowDirectoryListing)
					{
						Core::Cospawn([Base]() { Logical::ProcessDirectory(Base); });
						return true;
					}

					return Base->Abort(403, "Directory listing denied.");
				}

				if (Base->Route->StaticFileMaxAge > 0 && !Resources::ResourceModified(Base, &Base->Resource))
					return Logical::ProcessResourceCache(Base);

				return Logical::ProcessResource(Base);
			}
			bool Routing::RoutePost(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				if (Base->Route->FilesDirectory.empty() || Resources::ResourceHidden(Base, nullptr))
					return Base->Abort(404, "Requested resource was not found.");

				if (!Core::OS::File::GetState(Base->Request.Path, &Base->Resource))
					return Base->Abort(404, "Requested resource was not found.");

				if (Base->Resource.IsDirectory && !Resources::ResourceIndexed(Base, &Base->Resource))
					return Base->Abort(404, "Requested resource was not found.");

				if (Base->Route->StaticFileMaxAge > 0 && !Resources::ResourceModified(Base, &Base->Resource))
					return Logical::ProcessResourceCache(Base);

				return Logical::ProcessResource(Base);
			}
			bool Routing::RoutePut(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				if (Base->Route->FilesDirectory.empty() || Resources::ResourceHidden(Base, nullptr))
					return Base->Abort(403, "Resource overwrite denied.");

				if (!Core::OS::File::GetState(Base->Request.Path, &Base->Resource))
					return Base->Abort(403, "Directory overwrite denied.");

				if (!Base->Resource.IsDirectory)
					return Base->Abort(403, "Directory overwrite denied.");

				auto Range = Base->Request.GetHeader("Range");
				int64_t Range1 = 0, Range2 = 0;

				auto File = Core::OS::File::Open(Base->Request.Path.c_str(), "wb");
				if (!File)
					return Base->Abort(422, "Resource stream cannot be opened.");

				FILE* Stream = *File;
				if (!Range.empty() && HTTP::Parsing::ParseContentRange(Range, &Range1, &Range2))
				{
					if (Base->Response.StatusCode <= 0)
						Base->Response.StatusCode = 206;

					if (!Core::OS::File::Seek64(Stream, Range1, Core::FileSeek::Begin))
						return Base->Abort(416, "Invalid content range offset (%" PRId64 ") was specified.", Range1);
				}
				else
					Base->Response.StatusCode = 204;

				return Base->Fetch([Stream](Connection* Base, SocketPoll Event, const std::string_view& Buffer)
				{
					if (Packet::IsData(Event))
					{
						fwrite(Buffer.data(), sizeof(char) * Buffer.size(), 1, Stream);
						return true;
					}
					else if (Packet::IsDone(Event))
					{
						char Date[64];
						Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

						auto* Content = HrmCache::Get()->Pop();
						Content->append(Base->Request.Version);
						Content->append(" 204 No Content\r\nDate: ").append(Date).append("\r\n");
						Content->append(Utils::ConnectionResolve(Base));
						Content->append("Content-Location: ").append(Base->Request.Location).append("\r\n");

						Core::OS::File::Close(Stream);
						if (Base->Route->Callbacks.Headers)
							Base->Route->Callbacks.Headers(Base, *Content);

						Content->append("\r\n", 2);
						return !Base->Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [Content, Base](SocketPoll Event)
						{
							HrmCache::Get()->Push(Content);
							if (Packet::IsDone(Event))
								Base->Next();
							else if (Packet::IsError(Event))
								Base->Abort();
						}, false);
					}
					else if (Packet::IsError(Event))
					{
						Core::OS::File::Close(Stream);
						return Base->Abort();
					}
					else if (Packet::IsSkip(Event))
						Core::OS::File::Close(Stream);

					return true;
				});
			}
			bool Routing::RoutePatch(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				if (Base->Route->FilesDirectory.empty() || Resources::ResourceHidden(Base, nullptr))
					return Base->Abort(404, "Requested resource was not found.");

				if (!Core::OS::File::GetState(Base->Request.Path, &Base->Resource))
					return Base->Abort(404, "Requested resource was not found.");

				if (Base->Resource.IsDirectory && !Resources::ResourceIndexed(Base, &Base->Resource))
					return Base->Abort(404, "Requested resource cannot be directory.");

				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				auto* Content = HrmCache::Get()->Pop();
				Content->append(Base->Request.Version);
				Content->append(" 204 No Content\r\nDate: ").append(Date).append("\r\n");
				Content->append(Utils::ConnectionResolve(Base));
				Content->append("Content-Location: ").append(Base->Request.Location).append("\r\n");
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, *Content);

				Content->append("\r\n", 2);
				return !!Base->Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [Content, Base](SocketPoll Event)
				{
					HrmCache::Get()->Push(Content);
					if (Packet::IsDone(Event))
						Base->Next(204);
					else if (Packet::IsError(Event))
						Base->Abort();
				}, false);
			}
			bool Routing::RouteDelete(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				if (Base->Route->FilesDirectory.empty() || Resources::ResourceHidden(Base, nullptr))
					return Base->Abort(404, "Requested resource was not found.");

				if (!Core::OS::File::GetState(Base->Request.Path, &Base->Resource))
					return Base->Abort(404, "Requested resource was not found.");

				if (!Base->Resource.IsDirectory)
				{
					if (!Core::OS::File::Remove(Base->Request.Path.c_str()))
						return Base->Abort(403, "Operation denied by system.");
				}
				else if (!Core::OS::Directory::Remove(Base->Request.Path.c_str()))
					return Base->Abort(403, "Operation denied by system.");

				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				auto* Content = HrmCache::Get()->Pop();
				Content->append(Base->Request.Version);
				Content->append(" 204 No Content\r\nDate: ").append(Date).append("\r\n");
				Content->append(Utils::ConnectionResolve(Base));
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, *Content);

				Content->append("\r\n", 2);
				return !!Base->Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [Content, Base](SocketPoll Event)
				{
					HrmCache::Get()->Push(Content);
					if (Packet::IsDone(Event))
						Base->Next(204);
					else if (Packet::IsError(Event))
						Base->Abort();
				}, false);
			}
			bool Routing::RouteOptions(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				auto* Content = HrmCache::Get()->Pop();
				Content->append(Base->Request.Version);
				Content->append(" 204 No Content\r\nDate: ").append(Date).append("\r\n");
				Content->append(Utils::ConnectionResolve(Base));
				Content->append("Allow: GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD\r\n");
				if (Base->Route && Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, *Content);

				Content->append("\r\n", 2);
				return !!Base->Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [Content, Base](SocketPoll Event)
				{
					HrmCache::Get()->Push(Content);
					if (Packet::IsDone(Event))
						Base->Next(204);
					else if (Packet::IsError(Event))
						Base->Abort();
				}, false);
			}

			bool Logical::ProcessDirectory(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				VI_MEASURE(Core::Timings::FileSystem);
				Core::Vector<std::pair<Core::String, Core::FileEntry>> Entries;
				if (!Core::OS::Directory::Scan(Base->Request.Path, Entries))
					return Base->Abort(500, "System denied to directory listing.");

				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				auto* Content = HrmCache::Get()->Pop();
				Content->append(Base->Request.Version);
				Content->append(" 200 OK\r\nDate: ").append(Date).append("\r\n");
				Content->append(Utils::ConnectionResolve(Base));
				Content->append("Content-Type: text/html; charset=").append(Base->Route->CharSet);
				Content->append("\r\nAccept-Ranges: bytes\r\n");

				Paths::ConstructHeadCache(Base, *Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, *Content);

				auto Message = Base->Response.GetHeader("X-Error");
				if (!Message.empty())
					Content->append("X-Error: ").append(Message).append("\r\n");

				size_t Size = Base->Request.Location.size() - 1;
				while (Base->Request.Location[Size] != '/')
					Size--;

				char Direction = (!Base->Request.Query.empty() && Base->Request.Query[1] == 'd') ? 'a' : 'd';
				Core::String Parent(1, '/');
				if (Base->Request.Location.size() > 1)
				{
					Parent = Base->Request.Location.substr(0, Base->Request.Location.size() - 1);
					Parent = Core::OS::Path::GetDirectory(Parent.c_str());
				}

				Base->Response.Content.Assign(
					"<html><head><title>Index of " + Base->Request.Location + "</title>"
					"<style>" CSS_DIRECTORY_STYLE "</style></head>"
					"<body><h1>Index of " + Base->Request.Location + "</h1><pre><table cellpadding=\"0\">"
					"<tr><th><a href=\"?n" + Direction + "\">Name</a></th>"
					"<th><a href=\"?d" + Direction + "\">Modified</a></th>"
					"<th><a href=\"?s" + Direction + "\">Size</a></th></tr>"
					"<tr><td colspan=\"3\"><hr></td></tr>"
					"<tr><td><a href=\"" + Parent + "\">Parent directory</a></td>"
					"<td>&nbsp;-</td><td>&nbsp;&nbsp;-</td></tr>");
				VI_SORT(Entries.begin(), Entries.end(), [&Base](const std::pair<Core::String, Core::FileEntry>& A, const std::pair<Core::String, Core::FileEntry>& B)
				{
					return Paths::ConstructDirectoryEntries(Base, A.first, A.second, B.first, B.second);
				});

				for (auto& Item : Entries)
				{
					if (Resources::ResourceHidden(Base, &Item.first))
						continue;

					char dSize[64];
					if (!Item.second.IsDirectory)
					{
						if (Item.second.Size < 1024)
							snprintf(dSize, sizeof(dSize), "%d bytes", (int)Item.second.Size);
						else if (Item.second.Size < 0x100000)
							snprintf(dSize, sizeof(dSize), "%.1f kb", ((double)Item.second.Size) / 1024.0);
						else if (Item.second.Size < 0x40000000)
							snprintf(dSize, sizeof(Size), "%.1f mb", ((double)Item.second.Size) / 1048576.0);
						else
							snprintf(dSize, sizeof(dSize), "%.1f gb", ((double)Item.second.Size) / 1073741824.0);
					}
					else
						strncpy(dSize, "[DIRECTORY]", sizeof(dSize));

					char dDate[64];
					Core::DateTime::FetchWebDateTime(dDate, sizeof(dDate), Item.second.LastModified);
					Core::String Location = Compute::Codec::URLEncode(Item.first);
					Core::String Link = (Base->Request.Location + ((*(Base->Request.Location.c_str() + 1) != '\0' && Base->Request.Location[Base->Request.Location.size() - 1] != '/') ? "/" : "") + Location);
					if (Item.second.IsDirectory && !Core::Stringify::EndsOf(Link, "/\\"))
						Link.append(1, '/');

					Base->Response.Content.Append("<tr><td><a href=\"" + Link + "\">" + Item.first + "</a></td><td>&nbsp;" + dDate + "</td><td>&nbsp;&nbsp;" + dSize + "</td></tr>\n");
				}
				Base->Response.Content.Append("</table></pre></body></html>");
#ifdef VI_ZLIB
				bool Deflate = false, Gzip = false;
				if (Resources::ResourceCompressed(Base, Base->Response.Content.Data.size()))
				{
					auto AcceptEncoding = Base->Request.GetHeader("Accept-Encoding");
					if (!AcceptEncoding.empty())
					{
						Deflate = AcceptEncoding.find("deflate") != std::string::npos;
						Gzip = AcceptEncoding.find("gzip") != std::string::npos;
					}

					if (!AcceptEncoding.empty() && (Deflate || Gzip))
					{
						z_stream Stream;
						Stream.zalloc = Z_NULL;
						Stream.zfree = Z_NULL;
						Stream.opaque = Z_NULL;
						Stream.avail_in = (uInt)Base->Response.Content.Data.size();
						Stream.next_in = (Bytef*)Base->Response.Content.Data.data();

						if (deflateInit2(&Stream, Base->Route->Compression.QualityLevel, Z_DEFLATED, (Gzip ? 15 | 16 : 15), Base->Route->Compression.MemoryLevel, (int)Base->Route->Compression.Tune) == Z_OK)
						{
							Core::String Buffer(Base->Response.Content.Data.size(), '\0');
							Stream.avail_out = (uInt)Buffer.size();
							Stream.next_out = (Bytef*)Buffer.c_str();
							bool Compress = (deflate(&Stream, Z_FINISH) == Z_STREAM_END);
							bool Flush = (deflateEnd(&Stream) == Z_OK);

							if (Compress && Flush)
							{
								Base->Response.Content.Assign(std::string_view(Buffer.c_str(), (size_t)Stream.total_out));
								if (Base->Response.GetHeader("Content-Encoding").empty())
								{
									if (Gzip)
										Content->append("Content-Encoding: gzip\r\n", 24);
									else
										Content->append("Content-Encoding: deflate\r\n", 27);
								}
							}
						}
					}
				}
#endif
				Content->append("Content-Length: ").append(Core::ToString(Base->Response.Content.Data.size())).append("\r\n\r\n");
				return !!Base->Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [Content, Base](SocketPoll Event)
				{
					HrmCache::Get()->Push(Content);
					if (Packet::IsDone(Event))
					{
						if (memcmp(Base->Request.Method, "HEAD", 4) == 0)
							return (void)Base->Next(200);

						Base->Stream->WriteQueued((uint8_t*)Base->Response.Content.Data.data(), Base->Response.Content.Data.size(), [Base](SocketPoll Event)
						{
							if (Packet::IsDone(Event))
								Base->Next(200);
							else if (Packet::IsError(Event))
								Base->Abort();
						}, false);
					}
					else if (Packet::IsError(Event))
						Base->Abort();
				}, false);
			}
			bool Logical::ProcessResource(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				auto ContentType = Utils::ContentType(Base->Request.Path, &Base->Route->MimeTypes);
				auto Range = Base->Request.GetHeader("Range");
				auto StatusMessage = Utils::StatusMessage(Base->Response.StatusCode = (Base->Response.Error && Base->Response.StatusCode > 0 ? Base->Response.StatusCode : 200));
				int64_t Range1 = 0, Range2 = 0, Count = 0;
				int64_t ContentLength = (int64_t)Base->Resource.Size;

				char ContentRange[128] = { };
				if (!Range.empty() && (Count = Parsing::ParseContentRange(Range, &Range1, &Range2)) > 0 && Range1 >= 0 && Range2 >= 0)
				{
					if (Count == 2)
						ContentLength = (int64_t)(((Range2 > ContentLength) ? ContentLength : Range2) - Range1 + 1);
					else
						ContentLength -= Range1;

					snprintf(ContentRange, sizeof(ContentRange), "Content-Range: bytes %" PRId64 "-%" PRId64 "/%" PRId64 "\r\n", Range1, Range1 + ContentLength - 1, (int64_t)Base->Resource.Size);
					StatusMessage = Utils::StatusMessage(Base->Response.StatusCode = (Base->Response.Error ? Base->Response.StatusCode : 206));
				}
#ifdef VI_ZLIB
				if (Resources::ResourceCompressed(Base, (size_t)ContentLength))
				{
					auto AcceptEncoding = Base->Request.GetHeader("Accept-Encoding");
					if (!AcceptEncoding.empty())
					{
						bool Deflate = AcceptEncoding.find("deflate") != std::string::npos;
						bool Gzip = AcceptEncoding.find("gzip") != std::string::npos;
						if (Deflate || Gzip)
							return ProcessResourceCompress(Base, Deflate, Gzip, ContentRange, (size_t)Range1);
					}
				}
#endif
				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				char LastModified[64];
				Core::DateTime::FetchWebDateGMT(LastModified, sizeof(LastModified), Base->Resource.LastModified);

				char ETag[64];
				Core::OS::Net::GetETag(ETag, sizeof(ETag), &Base->Resource);

				auto* Content = HrmCache::Get()->Pop();
				Content->append(Base->Request.Version).append(" ");
				Content->append(Core::ToString(Base->Response.StatusCode)).append(" ");
				Content->append(StatusMessage).append("\r\n");
				Content->append("Date: ").append(Date).append("\r\n");
				Content->append(Utils::ConnectionResolve(Base));

				auto Origin = Base->Request.GetHeader("Origin");
				if (!Origin.empty())
					Content->append("Access-Control-Allow-Origin: ").append(Base->Route->AccessControlAllowOrigin).append("\r\n");

				Paths::ConstructHeadCache(Base, *Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, *Content);

				auto Message = Base->Response.GetHeader("X-Error");
				if (!Message.empty())
					Content->append("X-Error: ").append(Message).append("\r\n");

				Content->append("Accept-Ranges: bytes\r\nLast-Modified: ").append(LastModified).append("\r\n");
				Content->append("Etag: ").append(ETag).append("\r\n");
				Content->append("Content-Type: ").append(ContentType).append("; charset=").append(Base->Route->CharSet).append("\r\n");
				Content->append("Content-Length: ").append(Core::ToString(ContentLength)).append("\r\n");
				Content->append(ContentRange).append("\r\n");

				if (ContentLength > 0 && strcmp(Base->Request.Method, "HEAD") != 0)
				{
					return !!Base->Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [Content, Base, ContentLength, Range1](SocketPoll Event)
					{
						HrmCache::Get()->Push(Content);
						if (Packet::IsDone(Event))
							Core::Cospawn([Base, ContentLength, Range1]() { Logical::ProcessFile(Base, (size_t)ContentLength, (size_t)Range1); });
						else if (Packet::IsError(Event))
							Base->Abort();
					}, false);
				}
				else
				{
					return !!Base->Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [Content, Base](SocketPoll Event)
					{
						HrmCache::Get()->Push(Content);
						if (Packet::IsDone(Event))
							Base->Next(200);
						else if (Packet::IsError(Event))
							Base->Abort();
					}, false);
				}
			}
			bool Logical::ProcessResourceCompress(Connection* Base, bool Deflate, bool Gzip, const std::string_view& ContentRange, size_t Range)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				VI_ASSERT(Deflate || Gzip, "uncompressable resource");
				auto ContentType = Utils::ContentType(Base->Request.Path, &Base->Route->MimeTypes);
				auto StatusMessage = Utils::StatusMessage(Base->Response.StatusCode = (Base->Response.Error && Base->Response.StatusCode > 0 ? Base->Response.StatusCode : 200));
				int64_t ContentLength = (int64_t)Base->Resource.Size;

				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				char LastModified[64];
				Core::DateTime::FetchWebDateGMT(LastModified, sizeof(LastModified), Base->Resource.LastModified);

				char ETag[64];
				Core::OS::Net::GetETag(ETag, sizeof(ETag), &Base->Resource);

				auto* Content = HrmCache::Get()->Pop();
				Content->append(Base->Request.Version).append(" ");
				Content->append(Core::ToString(Base->Response.StatusCode)).append(" ");
				Content->append(StatusMessage).append("\r\n");
				Content->append("Date: ").append(Date).append("\r\n");
				Content->append(Utils::ConnectionResolve(Base));

				auto Origin = Base->Request.GetHeader("Origin");
				if (!Origin.empty())
					Content->append("Access-Control-Allow-Origin: ").append(Base->Route->AccessControlAllowOrigin).append("\r\n");

				Paths::ConstructHeadCache(Base, *Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, *Content);

				auto Message = Base->Response.GetHeader("X-Error");
				if (!Message.empty())
					Content->append("X-Error: ").append(Message).append("\r\n");

				Content->append("Accept-Ranges: bytes\r\nLast-Modified: ").append(LastModified).append("\r\n");
				Content->append("Etag: ").append(ETag).append("\r\n");
				Content->append("Content-Type: ").append(ContentType).append("; charset=").append(Base->Route->CharSet).append("\r\n");
				Content->append("Content-Encoding: ").append(Gzip ? "gzip" : "deflate").append("\r\n");
				Content->append("Transfer-Encoding: chunked\r\n");
				Content->append(ContentRange).append("\r\n");

				if (ContentLength > 0 && strcmp(Base->Request.Method, "HEAD") != 0)
				{
					return !!Base->Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [Content, Base, Range, ContentLength, Gzip](SocketPoll Event)
					{
						HrmCache::Get()->Push(Content);
						if (Packet::IsDone(Event))
							Core::Cospawn([Base, Range, ContentLength, Gzip]() { Logical::ProcessFileCompress(Base, (size_t)ContentLength, (size_t)Range, Gzip); });
						else if (Packet::IsError(Event))
							Base->Abort();
					}, false);
				}
				else
				{
					return !!Base->Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [Content, Base](SocketPoll Event)
					{
						HrmCache::Get()->Push(Content);
						if (Packet::IsDone(Event))
							Base->Next();
						else if (Packet::IsError(Event))
							Base->Abort();
					}, false);
				}
			}
			bool Logical::ProcessResourceCache(Connection* Base)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				char LastModified[64];
				Core::DateTime::FetchWebDateGMT(LastModified, sizeof(LastModified), Base->Resource.LastModified);

				char ETag[64];
				Core::OS::Net::GetETag(ETag, sizeof(ETag), &Base->Resource);

				auto* Content = HrmCache::Get()->Pop();
				Content->append(Base->Request.Version);
				Content->append(" 304 Not Modified\r\nDate: ");
				Content->append(Date).append("\r\n");

				Paths::ConstructHeadCache(Base, *Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, *Content);

				Content->append("Accept-Ranges: bytes\r\nLast-Modified: ").append(LastModified).append("\r\n");
				Content->append("Etag: ").append(ETag).append("\r\n");
				Content->append(Utils::ConnectionResolve(Base)).append("\r\n");
				return !!Base->Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [Content, Base](SocketPoll Event)
				{
					HrmCache::Get()->Push(Content);
					if (Packet::IsDone(Event))
						Base->Next(304);
					else if (Packet::IsError(Event))
						Base->Abort();
				}, false);
			}
			bool Logical::ProcessFile(Connection* Base, size_t ContentLength, size_t Range)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				VI_MEASURE(Core::Timings::FileSystem);
				Range = (Range > Base->Resource.Size ? Base->Resource.Size : Range);
				if (ContentLength > 0 && Base->Resource.IsReferenced && Base->Resource.Size > 0)
				{
					size_t Limit = Base->Resource.Size - Range;
					if (ContentLength > Limit)
						ContentLength = Limit;

					if (Base->Response.Content.Data.size() >= ContentLength)
					{
						return !!Base->Stream->WriteQueued((uint8_t*)Base->Response.Content.Data.data() + Range, ContentLength, [Base](SocketPoll Event)
						{
							if (Packet::IsDone(Event))
								Base->Next();
							else if (Packet::IsError(Event))
								Base->Abort();
						}, false);
					}
				}

				auto File = Core::OS::File::Open(Base->Request.Path.c_str(), "rb");
				if (!File)
					return Base->Abort(500, "System denied to open resource stream.");

				FILE* Stream = *File;
                if (Base->Route->AllowSendFile)
                {
                    auto Result = Base->Stream->WriteFileQueued(Stream, Range, ContentLength, [Base, Stream, ContentLength, Range](SocketPoll Event)
                    {
                        if (Packet::IsDone(Event))
                        {
                            Core::OS::File::Close(Stream);
                            Base->Next();
                        }
                        else if (Packet::IsError(Event))
							ProcessFileStream(Base, Stream, ContentLength, Range);
                        else if (Packet::IsSkip(Event))
                            Core::OS::File::Close(Stream);
                    });
                    if (Result || Result.Error() != std::errc::not_supported)
                        return true;
                }

				return ProcessFileStream(Base, Stream, ContentLength, Range);
			}
			bool Logical::ProcessFileStream(Connection* Base, FILE* Stream, size_t ContentLength, size_t Range)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				VI_ASSERT(Stream != nullptr, "stream should be set");
				if (!Core::OS::File::Seek64(Stream, Range, Core::FileSeek::Begin))
				{
					Core::OS::File::Close(Stream);
					return Base->Abort(400, "Provided content range offset (%" PRIu64 ") is invalid", Range);
				}

                return ProcessFileChunk(Base, Stream, ContentLength);
			}
			bool Logical::ProcessFileChunk(Connection* Base, FILE* Stream, size_t ContentLength)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				VI_ASSERT(Stream != nullptr, "stream should be set");
				VI_MEASURE(Core::Timings::FileSystem);
            Retry:
                uint8_t Buffer[Core::BLOB_SIZE];
				if (!ContentLength || Base->Root->State != ServerState::Working)
				{
				Cleanup:
					Core::OS::File::Close(Stream);
					if (Base->Root->State != ServerState::Working)
						return Base->Abort();

					return Base->Next() || true;
				}
                
				size_t Read = sizeof(Buffer);
				if ((Read = (size_t)fread(Buffer, 1, Read > ContentLength ? ContentLength : Read, Stream)) <= 0)
					goto Cleanup;
                
				ContentLength -= Read;
                auto Written = Base->Stream->WriteQueued(Buffer, Read, [Base, Stream, ContentLength](SocketPoll Event)
				{
					if (Packet::IsDoneAsync(Event))
					{
						Core::Cospawn([Base, Stream, ContentLength]()
						{
							ProcessFileChunk(Base, Stream, ContentLength);
						});
					}
					else if (Packet::IsError(Event))
					{
						Core::OS::File::Close(Stream);
						Base->Abort();
					}
					else if (Packet::IsSkip(Event))
                        Core::OS::File::Close(Stream);
				});

				if (Written && *Written > 0)
                    goto Retry;
                
				return false;
			}
			bool Logical::ProcessFileCompress(Connection* Base, size_t ContentLength, size_t Range, bool Gzip)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				VI_MEASURE(Core::Timings::FileSystem);
				Range = (Range > Base->Resource.Size ? Base->Resource.Size : Range);
				if (ContentLength > 0 && Base->Resource.IsReferenced && Base->Resource.Size > 0)
				{
					if (Base->Response.Content.Data.size() >= ContentLength)
					{
#ifdef VI_ZLIB
						z_stream ZStream;
						ZStream.zalloc = Z_NULL;
						ZStream.zfree = Z_NULL;
						ZStream.opaque = Z_NULL;
						ZStream.avail_in = (uInt)Base->Response.Content.Data.size();
						ZStream.next_in = (Bytef*)Base->Response.Content.Data.data();

						if (deflateInit2(&ZStream, Base->Route->Compression.QualityLevel, Z_DEFLATED, (Gzip ? 15 | 16 : 15), Base->Route->Compression.MemoryLevel, (int)Base->Route->Compression.Tune) == Z_OK)
						{
							Core::String Buffer(Base->Response.Content.Data.size(), '\0');
							ZStream.avail_out = (uInt)Buffer.size();
							ZStream.next_out = (Bytef*)Buffer.c_str();
							bool Compress = (deflate(&ZStream, Z_FINISH) == Z_STREAM_END);
							bool Flush = (deflateEnd(&ZStream) == Z_OK);

							if (Compress && Flush)
								Base->Response.Content.Assign(std::string_view(Buffer.c_str(), (size_t)ZStream.total_out));
						}
#endif
						return !!Base->Stream->WriteQueued((uint8_t*)Base->Response.Content.Data.data(), ContentLength, [Base](SocketPoll Event)
						{
							if (Packet::IsDone(Event))
								Base->Next();
							else if (Packet::IsError(Event))
								Base->Abort();
						}, false);
					}
				}

				auto File = Core::OS::File::Open(Base->Request.Path.c_str(), "rb");
				if (!File)
					return Base->Abort(500, "System denied to open resource stream.");

				FILE* Stream = *File;
				if (Range > 0 && !Core::OS::File::Seek64(Stream, Range, Core::FileSeek::Begin))
				{
					Core::OS::File::Close(Stream);
					return Base->Abort(400, "Provided content range offset (%" PRIu64 ") is invalid", Range);
				}
#ifdef VI_ZLIB
				z_stream* ZStream = Core::Memory::Allocate<z_stream>(sizeof(z_stream));
				ZStream->zalloc = Z_NULL;
				ZStream->zfree = Z_NULL;
				ZStream->opaque = Z_NULL;

				if (deflateInit2(ZStream, Base->Route->Compression.QualityLevel, Z_DEFLATED, (Gzip ? MAX_WBITS + 16 : MAX_WBITS), Base->Route->Compression.MemoryLevel, (int)Base->Route->Compression.Tune) != Z_OK)
				{
					Core::OS::File::Close(Stream);
					Core::Memory::Deallocate(ZStream);
					return Base->Abort();
				}

				return ProcessFileCompressChunk(Base, Stream, ZStream, ContentLength);
#else
				Core::OS::File::Close(Stream);
				return Base->Abort(500, "Cannot process gzip stream.");
#endif
			}
			bool Logical::ProcessFileCompressChunk(Connection* Base, FILE* Stream, void* CStream, size_t ContentLength)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				VI_ASSERT(Stream != nullptr, "stream should be set");
				VI_ASSERT(CStream != nullptr, "cstream should be set");
				VI_MEASURE(Core::Timings::FileSystem);
#ifdef VI_ZLIB
#define FREE_STREAMING { Core::OS::File::Close(Stream); deflateEnd(ZStream); Core::Memory::Deallocate(ZStream); }
				z_stream* ZStream = (z_stream*)CStream;
            Retry:
                uint8_t Buffer[Core::BLOB_SIZE + GZ_HEADER_SIZE], Deflate[Core::BLOB_SIZE];
				if (!ContentLength || Base->Root->State != ServerState::Working)
				{
				Cleanup:
					FREE_STREAMING;
					if (Base->Root->State != ServerState::Working)
						return Base->Abort();

					return !!Base->Stream->WriteQueued((uint8_t*)"0\r\n\r\n", 5, [Base](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
							Base->Next();
						else if (Packet::IsError(Event))
							Base->Abort();
					}, false);
				}
                
				size_t Read = sizeof(Buffer) - GZ_HEADER_SIZE;
				if ((Read = (size_t)fread(Buffer, 1, Read > ContentLength ? ContentLength : Read, Stream)) <= 0)
					goto Cleanup;

				ContentLength -= Read;
				ZStream->avail_in = (uInt)Read;
				ZStream->next_in = (Bytef*)Buffer;
				ZStream->avail_out = (uInt)sizeof(Deflate);
				ZStream->next_out = (Bytef*)Deflate;
				deflate(ZStream, Z_SYNC_FLUSH);
				Read = (int)sizeof(Deflate) - (int)ZStream->avail_out;

				int Next = snprintf((char*)Buffer, sizeof(Buffer), "%X\r\n", (uint32_t)Read);
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

				auto Written = Base->Stream->WriteQueued(Buffer, Read, [Base, Stream, ZStream, ContentLength](SocketPoll Event)
				{
					if (Packet::IsDoneAsync(Event))
					{
						if (ContentLength > 0)
						{
							Core::Cospawn([Base, Stream, ZStream, ContentLength]()
                            {
                                ProcessFileCompressChunk(Base, Stream, ZStream, ContentLength);
                            });
						}
						else
						{
							FREE_STREAMING;
							Base->Next();
						}
					}
					else if (Packet::IsError(Event))
					{
						FREE_STREAMING;
						Base->Abort();
					}
					else if (Packet::IsSkip(Event))
						FREE_STREAMING;
				});
                if (Written && *Written > 0)
                    goto Retry;
                
				return false;
#undef FREE_STREAMING
#else
				return Base->Next();
#endif
			}
			bool Logical::ProcessWebSocket(Connection* Base, const uint8_t* Key, size_t KeySize)
			{
				VI_ASSERT(ConnectionValid(Base), "connection should be valid");
				VI_ASSERT(Key != nullptr, "key should be set");
				auto Version = Base->Request.GetHeader("Sec-WebSocket-Version");
				if (Version.empty() || Version != "13")
					return Base->Abort(426, "Protocol upgrade required. Version \"%s\" is not allowed", Version);

				char Buffer[128];
				if (KeySize + sizeof(HTTP_WEBSOCKET_KEY) >= sizeof(Buffer))
					return Base->Abort(426, "Protocol upgrade required. Supplied key is invalid", Version);

				snprintf(Buffer, sizeof(Buffer), "%.*s%s", (int)KeySize, Key, HTTP_WEBSOCKET_KEY);
				Base->Request.Content.Data.clear();

				char Encoded20[20];
				Compute::Crypto::Sha1Compute(Buffer, (int)strlen(Buffer), Encoded20);
				if (Base->Response.StatusCode <= 0)
					Base->Response.StatusCode = 101;

				auto* Content = HrmCache::Get()->Pop();
				Content->append(
					"HTTP/1.1 101 Switching Protocols\r\n"
					"Upgrade: websocket\r\n"
					"Connection: Upgrade\r\n"
					"Sec-WebSocket-Accept: ");
				Content->append(Compute::Codec::Base64Encode(std::string_view(Encoded20, 20)));
				Content->append("\r\n");

				auto Protocol = Base->Request.GetHeader("Sec-WebSocket-Protocol");
				if (!Protocol.empty())
				{
					const char* Offset = strchr(Protocol.data(), ',');
					if (Offset != nullptr)
						Content->append("Sec-WebSocket-Protocol: ").append(Protocol, (size_t)(Offset - Protocol.data())).append("\r\n");
					else
						Content->append("Sec-WebSocket-Protocol: ").append(Protocol).append("\r\n");
				}

				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, *Content);

				Content->append("\r\n", 2);
				return !!Base->Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [Content, Base](SocketPoll Event)
				{
					HrmCache::Get()->Push(Content);
					if (Packet::IsDone(Event))
					{
						Base->WebSocket = new WebSocketFrame(Base->Stream, Base);
						Base->WebSocket->Connect = Base->Route->Callbacks.WebSocket.Connect;
						Base->WebSocket->Receive = Base->Route->Callbacks.WebSocket.Receive;
						Base->WebSocket->Disconnect = Base->Route->Callbacks.WebSocket.Disconnect;
						Base->WebSocket->Lifetime.Dead = [Base](WebSocketFrame*)
						{
							return Base->Info.Abort;
						};
						Base->WebSocket->Lifetime.Close = [Base](WebSocketFrame*, bool Successful)
						{
							if (Successful)
								Base->Next();
							else
								Base->Abort();
						};

						Base->Stream->SetIoTimeout(Base->Route->WebSocketTimeout);
						if (!Base->Route->Callbacks.WebSocket.Initiate || !Base->Route->Callbacks.WebSocket.Initiate(Base))
							Base->WebSocket->Next();
					}
					else if (Packet::IsError(Event))
						Base->Abort();
				}, false);
			}

			Server::Server() : SocketServer()
			{
				HrmCache::LinkInstance();
			}
			Server::~Server()
			{
				Unlisten(false);
			}
			Core::ExpectsSystem<void> Server::Update()
			{
				auto* Target = (MapRouter*)Router;
				if (!Target->Session.Directory.empty())
				{
					auto Directory = Core::OS::Path::Resolve(Target->Session.Directory);
					if (Directory)
						Target->Session.Directory = *Directory;

					auto Status = Core::OS::Directory::Patch(Target->Session.Directory);
					if (!Status)
						return Core::SystemException("session directory: invalid path", std::move(Status.Error()));
				}

				if (!Target->TemporaryDirectory.empty())
				{
					auto Directory = Core::OS::Path::Resolve(Target->TemporaryDirectory);
					if (Directory)
						Target->TemporaryDirectory = *Directory;

					auto Status = Core::OS::Directory::Patch(Target->TemporaryDirectory);
					if (!Status)
						return Core::SystemException("temporary directory: invalid path", std::move(Status.Error()));
				}

				auto Status = UpdateRoute(Target->Base);
				if (!Status)
					return Status;

				for (auto& Group : Target->Groups)
				{
					for (auto* Route : Group->Routes)
					{
						Status = UpdateRoute(Route);
						if (!Status)
							return Status;
					}
				}

				Target->Sort();
				return Core::Expectation::Met;
			}
			Core::ExpectsSystem<void> Server::UpdateRoute(RouterEntry* Route)
			{
				Route->Router = (MapRouter*)Router;
				if (!Route->FilesDirectory.empty())
				{
					auto Directory = Core::OS::Path::Resolve(Route->FilesDirectory.c_str());
					if (Directory)
						Route->FilesDirectory = *Directory;
				}

				if (!Route->Alias.empty())
				{
					auto Directory = Core::OS::Path::Resolve(Route->Alias, Route->FilesDirectory.empty() ? *Core::OS::Directory::GetWorking() : Route->FilesDirectory, true);
					if (Directory)
						Route->Alias = *Directory;
				}

				return Core::Expectation::Met;
			}
			Core::ExpectsSystem<void> Server::OnConfigure(SocketRouter* NewRouter)
			{
				VI_ASSERT(NewRouter != nullptr, "router should be set");
				return Update();
			}
			Core::ExpectsSystem<void> Server::OnUnlisten()
			{
				VI_ASSERT(Router != nullptr, "router should be set");
				MapRouter* Target = (MapRouter*)Router;
				if (!Target->TemporaryDirectory.empty())
				{
					auto Status = Core::OS::Directory::Remove(Target->TemporaryDirectory.c_str());
					if (!Status)
						return Core::SystemException("temporary directory remove error: " + Target->TemporaryDirectory, std::move(Status.Error()));
				}

				if (!Target->Session.Directory.empty())
				{
					auto Status = Session::InvalidateCache(Target->Session.Directory);
					if (!Status)
						return Status;
				}

				return Core::Expectation::Met;
			}
			void Server::OnRequestOpen(SocketConnection* Source)
			{
				VI_ASSERT(Source != nullptr, "connection should be set");
				auto* Conf = (MapRouter*)Router;
				auto* Base = (Connection*)Source;

				Base->Resolver->PrepareForRequestParsing(&Base->Request);
				Base->Stream->ReadUntilChunkedQueued("\r\n\r\n", [Base, Conf](SocketPoll Event, const uint8_t* Buffer, size_t Size)
				{
					if (Packet::IsData(Event))
					{
						size_t LastLength = Base->Request.Content.Data.size();
						Base->Request.Content.Append(std::string_view((char*)Buffer, Size));
						if (Base->Request.Content.Data.size() > Conf->MaxHeapBuffer)
						{
							Base->Abort(431, "Request containts too much data in headers");
							return false;
						}

						int64_t Offset = Base->Resolver->ParseRequest((uint8_t*)Base->Request.Content.Data.data(), Base->Request.Content.Data.size(), LastLength);
						if (Offset >= 0 || Offset == -2)
							return true;

						Base->Abort(400, "Invalid request was provided by client");
						return false;
					}
					else if (Packet::IsDone(Event))
					{
						uint32_t Redirects = 0;
						Base->Info.Start = Network::Utils::Clock();
						Base->Request.Content.Prepare(Base->Request.Headers, Buffer, Size);
					Redirect:
						if (!Paths::ConstructRoute(Conf, Base))
							return Base->Abort(400, "Request cannot be resolved");

						auto* Route = Base->Route;
						if (!Route->Redirect.empty())
						{
							if (Redirects++ > HTTP_MAX_REDIRECTS)
								return Base->Abort(500, "Infinite redirects loop detected");

							Base->Request.Location = Route->Redirect;
							goto Redirect;
						}

						Paths::ConstructPath(Base);
						if (!Permissions::MethodAllowed(Base))
							return Base->Abort(405, "Requested method \"%s\" is not allowed on this server", Base->Request.Method);

						if (!memcmp(Base->Request.Method, "GET", 3) || !memcmp(Base->Request.Method, "HEAD", 4))
						{
							if (!Permissions::Authorize(Base))
								return false;

							if (Route->Callbacks.Get && Route->Callbacks.Get(Base))
								return true;

							return Routing::RouteGet(Base);
						}
						else if (!memcmp(Base->Request.Method, "POST", 4))
						{
							if (!Permissions::Authorize(Base))
								return false;

							if (Route->Callbacks.Post && Route->Callbacks.Post(Base))
								return true;

							return Routing::RoutePost(Base);
						}
						else if (!memcmp(Base->Request.Method, "PUT", 3))
						{
							if (!Permissions::Authorize(Base))
								return false;

							if (Route->Callbacks.Put && Route->Callbacks.Put(Base))
								return true;

							return Routing::RoutePut(Base);
						}
						else if (!memcmp(Base->Request.Method, "PATCH", 5))
						{
							if (!Permissions::Authorize(Base))
								return false;

							if (Route->Callbacks.Patch && Route->Callbacks.Patch(Base))
								return true;

							return Routing::RoutePatch(Base);
						}
						else if (!memcmp(Base->Request.Method, "DELETE", 6))
						{
							if (!Permissions::Authorize(Base))
								return false;

							if (Route->Callbacks.Delete && Route->Callbacks.Delete(Base))
								return true;

							return Routing::RouteDelete(Base);
						}
						else if (!memcmp(Base->Request.Method, "OPTIONS", 7))
						{
							if (Route->Callbacks.Options && Route->Callbacks.Options(Base))
								return true;

							return Routing::RouteOptions(Base);
						}

						if (!Permissions::Authorize(Base))
							return false;

						return Base->Abort(405, "Request method \"%s\" is not allowed", Base->Request.Method);
					}
					else if (Packet::IsError(Event))
						Base->Abort();

					return true;
				});
			}
			bool Server::OnRequestCleanup(SocketConnection* Target)
			{
				VI_ASSERT(Target != nullptr, "connection should be set");
				auto Base = (HTTP::Connection*)Target;
				return Base->Skip([](HTTP::Connection* Base)
				{
					Base->Root->Finalize(Base);
					return true;
				});
			}
			void Server::OnRequestStall(SocketConnection* Target)
			{
				auto Base = (HTTP::Connection*)Target;
				Core::String Status = ", pathname: " + Base->Request.Location;
				if (Base->WebSocket != nullptr)
					Status += ", websocket: " + Core::String(Base->WebSocket->IsFinished() ? "alive" : "dead");
				VI_DEBUG("[stall] connection on fd %i%s", (int)Base->Stream->GetFd(), Status.c_str());
			}
			void Server::OnRequestClose(SocketConnection* Target)
			{
				VI_ASSERT(Target != nullptr, "connection should be set");
				auto Base = (HTTP::Connection*)Target;
				if (Base->Response.StatusCode > 0 && Base->Route && Base->Route->Callbacks.Access)
					Base->Route->Callbacks.Access(Base);
				Base->Reset(false);
			}
			SocketConnection* Server::OnAllocate(SocketListener* Host)
			{
				VI_ASSERT(Host != nullptr, "host should be set");
				auto* Base = new HTTP::Connection(this);
				auto* Target = (MapRouter*)Router;
				Base->Route = Target->Base;
				Base->Root = this;
				return Base;
			}
			SocketRouter* Server::OnAllocateRouter()
			{
				return new MapRouter();
			}

			Client::Client(int64_t ReadTimeout) : SocketClient(ReadTimeout), Resolver(new HTTP::Parser()), WebSocket(nullptr), Future(Core::ExpectsPromiseSystem<void>::Null())
			{
				Response.Content.Finalize();
				HrmCache::LinkInstance();
			}
			Client::~Client()
			{
				Core::Memory::Release(Resolver);
				Core::Memory::Release(WebSocket);
			}
			Core::ExpectsPromiseSystem<void> Client::Skip()
			{
				return Fetch(PAYLOAD_SIZE, true);
			}
			Core::ExpectsPromiseSystem<void> Client::Fetch(size_t MaxSize, bool Eat)
			{
				VI_ASSERT(!WebSocket, "cannot read http over websocket");
				if (Response.Content.IsFinalized())
					return Core::ExpectsPromiseSystem<void>(Core::Expectation::Met);
				else if (Response.Content.Exceeds)
					return Core::ExpectsPromiseSystem<void>(Core::SystemException("download content error: payload too large", std::make_error_condition(std::errc::value_too_large)));
				else if (!HasStream())
					return Core::ExpectsPromiseSystem<void>(Core::SystemException("download content error: bad fd", std::make_error_condition(std::errc::bad_file_descriptor)));

				auto ContentType = Response.GetHeader("Content-Type");
				if (ContentType == std::string_view("multipart/form-data", 19))
				{
					Response.Content.Exceeds = true;
					return Core::ExpectsPromiseSystem<void>(Core::SystemException("download content error: requires file saving", std::make_error_condition(std::errc::value_too_large)));
				}
				else if (Eat)
					MaxSize = std::numeric_limits<size_t>::max();

				size_t LeftoverSize = Response.Content.Data.size() - Response.Content.Prefetch;
				if (!Response.Content.Data.empty() && LeftoverSize > 0 && LeftoverSize <= Response.Content.Data.size())
					Response.Content.Data.erase(Response.Content.Data.begin(), Response.Content.Data.begin() + LeftoverSize);

				auto TransferEncoding = Response.GetHeader("Transfer-Encoding");
				bool IsTransferEncodingChunked = (!Response.Content.Limited && Core::Stringify::CaseEquals(TransferEncoding, "chunked"));
				if (IsTransferEncodingChunked)
					Resolver->PrepareForChunkedParsing();

				if (Response.Content.Prefetch > 0)
				{
					LeftoverSize = std::min(MaxSize, Response.Content.Prefetch);
					Response.Content.Prefetch -= LeftoverSize;
					MaxSize -= LeftoverSize;
					if (IsTransferEncodingChunked)
					{
						size_t DecodedSize = LeftoverSize;
						int64_t Subresult = Resolver->ParseDecodeChunked((uint8_t*)Response.Content.Data.data(), &DecodedSize);
						if (Subresult >= 0 || Subresult == -2)
						{
							LeftoverSize -= DecodedSize;
							if (LeftoverSize > 0)
								Response.Content.Data.erase(Response.Content.Data.begin() + DecodedSize, Response.Content.Data.begin() + DecodedSize + LeftoverSize);
							if (Subresult == 0)
							{
								Response.Content.Finalize();
								return Core::ExpectsPromiseSystem<void>(Core::Expectation::Met);
							}
						}
						else if (Subresult == -1)
							return Core::ExpectsPromiseSystem<void>(Core::SystemException("download transfer encoding content parsing error", std::make_error_condition(std::errc::protocol_error)));
					}
					else if (Response.Content.Prefetch > 0)
						return Core::ExpectsPromiseSystem<void>(Core::Expectation::Met);
				}
				else
					Response.Content.Data.clear();

				if (IsTransferEncodingChunked)
				{
					if (!MaxSize)
						return Core::ExpectsPromiseSystem<void>(Core::Expectation::Met);

					int64_t Subresult = -1;
					Core::ExpectsPromiseSystem<void> Result;
					Net.Stream->ReadQueued(MaxSize, [this, Result, Subresult, Eat](SocketPoll Event, const uint8_t* Buffer, size_t Recv) mutable
					{
						if (Packet::IsData(Event))
						{
							Subresult = Resolver->ParseDecodeChunked((uint8_t*)Buffer, &Recv);
							if (Subresult == -1)
								return false;

							Response.Content.Offset += Recv;
							if (!Eat)
								Response.Content.Append(std::string_view((char*)Buffer, Recv));
							return Subresult == -2;
						}
						else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
						{
							if (Subresult != -2)
							{
								Response.Content.Finalize();
								if (!Response.Content.Data.empty())
									VI_DEBUG("[http] fd %i responded\n%.*s", (int)Net.Stream->GetFd(), (int)Response.Content.Data.size(), Response.Content.Data.data());
							}

							if (Packet::IsErrorOrSkip(Event))
								Result.Set(Core::SystemException("download transfer encoding content parsing error", std::make_error_condition(std::errc::protocol_error)));
							else
								Result.Set(Core::Expectation::Met);
						}

						return true;
					});
					return Result;
				}
				else if (!Response.Content.Limited)
				{
					if (!MaxSize)
						return Core::ExpectsPromiseSystem<void>(Core::Expectation::Met);

					Core::ExpectsPromiseSystem<void> Result;
					Net.Stream->ReadQueued(MaxSize, [this, Result, Eat](SocketPoll Event, const uint8_t* Buffer, size_t Recv) mutable
					{
						if (Packet::IsData(Event))
						{
							Response.Content.Offset += Recv;
							if (!Eat)
								Response.Content.Append(std::string_view((char*)Buffer, Recv));
							return true;
						}
						else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
						{
							Response.Content.Finalize();
							if (!Response.Content.Data.empty())
								VI_DEBUG("[http] fd %i responded\n%.*s", (int)Net.Stream->GetFd(), (int)Response.Content.Data.size(), Response.Content.Data.data());

							if (Packet::IsErrorOrSkip(Event))
								Result.Set(Core::SystemException("download content network error", Packet::ToCondition(Event)));
							else
								Result.Set(Core::Expectation::Met);
						}

						return true;
					});
					return Result;
				}
                
				MaxSize = std::min(MaxSize, Response.Content.Length - Response.Content.Offset);
				if (!MaxSize)
				{
					Response.Content.Finalize();
					return Core::ExpectsPromiseSystem<void>(Core::Expectation::Met);
				}
				else if (Response.Content.Offset > Response.Content.Length)
					return Core::ExpectsPromiseSystem<void>(Core::SystemException("download content error: invalid range", std::make_error_condition(std::errc::result_out_of_range)));

				Core::ExpectsPromiseSystem<void> Result;
				Net.Stream->ReadQueued(MaxSize, [this, Result, Eat](SocketPoll Event, const uint8_t* Buffer, size_t Recv) mutable
				{
					if (Packet::IsData(Event))
					{
						Response.Content.Offset += Recv;
						if (!Eat)
							Response.Content.Append(std::string_view((char*)Buffer, Recv));
						return true;
					}
					else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
					{
						if (Response.Content.Length <= Response.Content.Offset)
						{
							Response.Content.Finalize();
							if (!Response.Content.Data.empty())
								VI_DEBUG("[http] fd %i responded\n%.*s", (int)Net.Stream->GetFd(), (int)Response.Content.Data.size(), Response.Content.Data.data());
						}

						if (Packet::IsErrorOrSkip(Event))
							Result.Set(Core::SystemException("download content network error", Packet::ToCondition(Event)));
						else
							Result.Set(Core::Expectation::Met);
					}

					return true;
				});
				return Result;
			}
			Core::ExpectsPromiseSystem<void> Client::Upgrade(HTTP::RequestFrame&& Target)
			{
				VI_ASSERT(WebSocket != nullptr, "websocket should be opened");
				if (!HasStream())
					return Core::ExpectsPromiseSystem<void>(Core::SystemException("upgrade error: bad fd", std::make_error_condition(std::errc::bad_file_descriptor)));

				Target.SetHeader("Pragma", "no-cache");
				Target.SetHeader("Upgrade", "WebSocket");
				Target.SetHeader("Connection", "Upgrade");
				Target.SetHeader("Sec-WebSocket-Version", "13");

				auto Random = Compute::Crypto::RandomBytes(16);
				if (Random)
					Target.SetHeader("Sec-WebSocket-Key", Compute::Codec::Base64Encode(*Random));
				else
					Target.SetHeader("Sec-WebSocket-Key", HTTP_WEBSOCKET_KEY);
	
				return Send(std::move(Target)).Then<Core::ExpectsPromiseSystem<void>>([this](Core::ExpectsSystem<void>&& Status) -> Core::ExpectsPromiseSystem<void>
				{
					VI_DEBUG("[ws] handshake %s", Request.Location.c_str());
					if (!Status)
						return Core::ExpectsPromiseSystem<void>(Status.Error());

					if (Response.StatusCode != 101)
						return Core::ExpectsPromiseSystem<void>(Core::SystemException("upgrade handshake status error", std::make_error_condition(std::errc::protocol_error)));

					if (Response.GetHeader("Sec-WebSocket-Accept").empty())
						return Core::ExpectsPromiseSystem<void>(Core::SystemException("upgrade handshake accept error", std::make_error_condition(std::errc::bad_message)));

					Future = Core::ExpectsPromiseSystem<void>();
					WebSocket->Next();
					return Future;
				});
			}
			Core::ExpectsPromiseSystem<void> Client::Send(HTTP::RequestFrame&& Target)
			{
				VI_ASSERT(!WebSocket || !Target.GetHeader("Sec-WebSocket-Key").empty(), "cannot send http request over websocket");
				if (!HasStream())
					return Core::ExpectsPromiseSystem<void>(Core::SystemException("send error: bad fd", std::make_error_condition(std::errc::bad_file_descriptor)));

				VI_DEBUG("[http] %s %s", Target.Method, Target.Location.c_str());
				if (!Response.Content.IsFinalized() || Response.Content.Exceeds)
					return Core::ExpectsPromiseSystem<void>(Core::SystemException("content error: response body was not read", std::make_error_condition(std::errc::broken_pipe)));

				Core::ExpectsPromiseSystem<void> Result;
				Request = std::move(Target);
				Response.Cleanup();
				State.Done = [Result](SocketClient* Client, Core::ExpectsSystem<void>&& Status) mutable
				{
					HTTP::Client* Base = (HTTP::Client*)Client;
					if (!Status)
						Base->GetResponse()->StatusCode = -1;
					Result.Set(std::move(Status));
				};

				if (Request.GetHeader("Host").empty())
				{
					auto Hostname = State.Address.GetHostname();
					if (Hostname)
					{
						auto Port = State.Address.GetIpPort();
						if (Port && *Port != (Net.Context ? 443 : 80))
							Request.SetHeader("Host", (*Hostname + ':' + Core::ToString(*Port)));
						else
							Request.SetHeader("Host", *Hostname);
					}
				}

				if (Request.GetHeader("Accept").empty())
					Request.SetHeader("Accept", "*/*");

				if (Request.GetHeader("Content-Length").empty())
				{
					Request.Content.Length = Request.Content.Data.size();
					Request.SetHeader("Content-Length", Core::ToString(Request.Content.Data.size()));
				}

				if (Request.GetHeader("Connection").empty())
					Request.SetHeader("Connection", "Keep-Alive");

				auto* Content = HrmCache::Get()->Pop();
				if (Request.Location.empty())
					Request.Location.assign("/");

				if (!Request.Query.empty())
				{
					Content->append(Request.Method).append(" ");
					Content->append(Request.Location).append("?");
					Content->append(Request.Query).append(" ");
					Content->append(Request.Version).append("\r\n");
				}
				else
				{
					Content->append(Request.Method).append(" ");
					Content->append(Request.Location).append(" ");
					Content->append(Request.Version).append("\r\n");
				}

				if (Request.Content.Resources.empty())
				{
					if (!Request.Content.Data.empty())
					{
						if (Request.GetHeader("Content-Type").empty())
							Request.SetHeader("Content-Type", "application/octet-stream");

						if (Request.GetHeader("Content-Length").empty())
							Request.SetHeader("Content-Length", Core::ToString(Request.Content.Data.size()).c_str());
					}
					else if (!memcmp(Request.Method, "POST", 4) || !memcmp(Request.Method, "PUT", 3) || !memcmp(Request.Method, "PATCH", 5))
						Request.SetHeader("Content-Length", "0");

					Paths::ConstructHeadFull(&Request, &Response, true, *Content);
					Content->append("\r\n");

					Net.Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [this, Content](SocketPoll Event)
					{
						HrmCache::Get()->Push(Content);
						if (Packet::IsDone(Event))
						{
							if (!Request.Content.Data.empty())
							{
								Net.Stream->WriteQueued((uint8_t*)Request.Content.Data.data(), Request.Content.Data.size(), [this](SocketPoll Event)
								{
									if (Packet::IsDone(Event))
									{
										Net.Stream->ReadUntilChunkedQueued("\r\n\r\n", [this](SocketPoll Event, const uint8_t* Buffer, size_t Recv)
										{
											if (Packet::IsData(Event))
												Response.Content.Append(std::string_view((char*)Buffer, Recv));
											else if (Packet::IsDone(Event))
												Receive(Buffer, Recv);
											else if (Packet::IsErrorOrSkip(Event))
												Report(Core::SystemException(Event == SocketPoll::Timeout ? "read timeout error" : "read abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));

											return true;
										});
									}
									else if (Packet::IsErrorOrSkip(Event))
										Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
								}, false);
							}
							else
							{
								Net.Stream->ReadUntilChunkedQueued("\r\n\r\n", [this](SocketPoll Event, const uint8_t* Buffer, size_t Recv)
								{
									if (Packet::IsData(Event))
										Response.Content.Append(std::string_view((char*)Buffer, Recv));
									else if (Packet::IsDone(Event))
										Receive(Buffer, Recv);
									else if (Packet::IsErrorOrSkip(Event))
										Report(Core::SystemException(Event == SocketPoll::Timeout ? "read timeout error" : "read abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));

									return true;
								});
							}
						}
						else if (Packet::IsErrorOrSkip(Event))
							Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
					}, false);
				}
				else
				{
					auto RandomBytes = Compute::Crypto::RandomBytes(24);
					if (!RandomBytes)
					{
						Report(Core::SystemException("send boundary error: " + RandomBytes.Error().message(), std::make_error_condition(std::errc::operation_canceled)));
						HrmCache::Get()->Push(Content);
						return Result;
					}

					Core::String Boundary = "----0x" + Compute::Codec::HexEncode(*RandomBytes);
					Request.SetHeader("Content-Type", "multipart/form-data; boundary=" + Boundary);
					Boundary.insert(0, "--");

					Boundaries.clear();
					Boundaries.reserve(Request.Content.Resources.size());

					size_t ContentSize = 0;
					for (auto& Item : Request.Content.Resources)
					{
						if (!Item.IsInMemory && Item.Name.empty())
							Item.Name = Core::OS::Path::GetFilename(Item.Path.c_str());
						if (Item.Type.empty())
							Item.Type = "application/octet-stream";
						if (Item.Key.empty())
							Item.Key = "file" + Core::ToString(Boundaries.size() + 1);

						BoundaryBlock Block;
						Block.Finish = "\r\n";
						Block.File = &Item;
						Block.IsFinal = (Boundaries.size() + 1 == Request.Content.Resources.size());

						for (auto& Header : Item.Headers)
						{
							Block.Data.append(Boundary).append("\r\n");
							Block.Data.append("Content-Disposition: form-data; name=\"").append(Header.first).append("\"\r\n\r\n");
							for (auto& Value : Header.second)
								Block.Data.append(Value);
							Block.Data.append("\r\n\r\n").append(Boundary).append("\r\n");
						}

						Block.Finish.append(Boundary);
						if (Block.IsFinal)
							Block.Finish.append("--\r\n");
						else
							Block.Finish.append("\r\n");

						Block.Data.append(Boundary).append("\r\n");
						Block.Data.append("Content-Disposition: form-data; name=\"").append(Item.Key).append("\"; filename=\"").append(Item.Name).append("\"\r\n");
						Block.Data.append("Content-Type: ").append(Item.Type).append("\r\n\r\n");

						if (!Item.IsInMemory)
						{
							auto State = Core::OS::File::GetState(Item.Path);
							if (State && !State->IsDirectory)
								Item.Length = State->Size;
							else
								Item.Length = 0;
							ContentSize += Block.Data.size() + Item.Length + Block.Finish.size();
						}
						else
						{
							Item.Length = Item.GetInMemoryContents().size();
							Block.Data.append(Item.GetInMemoryContents());
							Block.Data.append(Block.Finish);
							Item.Path.clear();
							Item.Path.shrink_to_fit();
							ContentSize += Block.Data.size();
						}

						Boundaries.emplace_back(std::move(Block));
					}

					Request.SetHeader("Content-Length", Core::ToString(ContentSize));
					Paths::ConstructHeadFull(&Request, &Response, true, *Content);
					Content->append("\r\n");

					Net.Stream->WriteQueued((uint8_t*)Content->c_str(), Content->size(), [this, Content](SocketPoll Event)
					{
						HrmCache::Get()->Push(Content);
						if (Packet::IsDone(Event))
							Upload(0);
						else if (Packet::IsErrorOrSkip(Event))
							Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
					}, false);
				}

				return Result;
			}
			Core::ExpectsPromiseSystem<void> Client::SendFetch(HTTP::RequestFrame&& Target, size_t MaxSize)
			{
				return Send(std::move(Target)).Then<Core::ExpectsPromiseSystem<void>>([this, MaxSize](Core::ExpectsSystem<void>&& Response) -> Core::ExpectsPromiseSystem<void>
				{
					if (!Response)
						return Response;

					return Fetch(MaxSize);
				});
			}
			Core::ExpectsPromiseSystem<Core::Schema*> Client::JSON(HTTP::RequestFrame&& Target, size_t MaxSize)
			{
				return SendFetch(std::move(Target), MaxSize).Then<Core::ExpectsSystem<Core::Schema*>>([this](Core::ExpectsSystem<void>&& Status) -> Core::ExpectsSystem<Core::Schema*>
				{
					if (!Status)
						return Status.Error();

					auto Data = Core::Schema::ConvertFromJSON(std::string_view(Response.Content.Data.data(), Response.Content.Data.size()));
					if (!Data)
						return Core::SystemException(Data.Error().message(), std::make_error_condition(std::errc::bad_message));

					return *Data;
				});
			}
			Core::ExpectsPromiseSystem<Core::Schema*> Client::XML(HTTP::RequestFrame&& Target, size_t MaxSize)
			{
				return SendFetch(std::move(Target), MaxSize).Then<Core::ExpectsSystem<Core::Schema*>>([this](Core::ExpectsSystem<void>&& Status) -> Core::ExpectsSystem<Core::Schema*>
				{
					if (!Status)
						return Status.Error();

					auto Data = Core::Schema::ConvertFromXML(std::string_view(Response.Content.Data.data(), Response.Content.Data.size()));
					if (!Data)
						return Core::SystemException(Data.Error().message(), std::make_error_condition(std::errc::bad_message));

					return *Data;
				});
			}
			Core::ExpectsSystem<void> Client::OnReuse()
			{
				Response.Content.Cleanup();
				Response.Content.Finalize();
				return Core::Expectation::Met;
			}
			Core::ExpectsSystem<void> Client::OnDisconnect()
			{
				Response.Content.Cleanup();
				Response.Content.Finalize();
				Report(Core::Expectation::Met);
				return Core::Expectation::Met;
			}
			void Client::Downgrade()
			{
				VI_ASSERT(WebSocket != nullptr, "websocket should be opened");
				VI_ASSERT(WebSocket->IsFinished(), "websocket connection should be finished");
				Core::Memory::Release(WebSocket);
			}
			WebSocketFrame* Client::GetWebSocket()
			{
				if (WebSocket != nullptr)
					return WebSocket;

				WebSocket = new WebSocketFrame(Net.Stream, this);
				WebSocket->Lifetime.Dead = [](WebSocketFrame*)
				{
					return false;
				};
				WebSocket->Lifetime.Close = [this](WebSocketFrame*, bool Successful)
				{
					if (!Successful)
					{
						Net.Stream->Shutdown();
						Future.Set(Core::SystemException("ws connection abort error", std::make_error_condition(std::errc::connection_reset)));
					}
					else
						Future.Set(Core::Expectation::Met);
				};

				return WebSocket;
			}
			RequestFrame* Client::GetRequest()
			{
				return &Request;
			}
			ResponseFrame* Client::GetResponse()
			{
				return &Response;
			}
			void Client::UploadFile(BoundaryBlock* Boundary, std::function<void(Core::ExpectsSystem<void>&&)>&& Callback)
			{
				auto File = Core::OS::File::Open(Boundary->File->Path.c_str(), "rb");
				if (!File)
					return Callback(Core::SystemException("upload file error", std::move(File.Error())));

				FILE* FileStream = *File;
                auto Result = Net.Stream->WriteFileQueued(FileStream, 0, Boundary->File->Length, [FileStream, Callback](SocketPoll Event)
                {
                    if (Packet::IsDone(Event))
                    {
                        Core::OS::File::Close(FileStream);
						Callback(Core::Expectation::Met);
                    }
                    else if (Packet::IsErrorOrSkip(Event))
					{
						Core::OS::File::Close(FileStream);
						Callback(Core::SystemException("upload file network error", std::make_error_condition(std::errc::connection_aborted)));
					}
				});         
                if (Result || Result.Error() != std::errc::not_supported)
					return Callback(Core::Expectation::Met);

				if (Config.IsAsync)
					return UploadFileChunkQueued(FileStream, Boundary->File->Length, std::move(Callback));

				return UploadFileChunk(FileStream, Boundary->File->Length, std::move(Callback));
			}
			void Client::UploadFileChunk(FILE* FileStream, size_t ContentLength, std::function<void(Core::ExpectsSystem<void>&&)>&& Callback)
			{
				if (!ContentLength)
				{
					Core::OS::File::Close(FileStream);
					return Callback(Core::Expectation::Met);
				}

				uint8_t Buffer[Core::BLOB_SIZE];
				while (ContentLength > 0)
				{
					size_t Read = sizeof(Buffer);
					if ((Read = (size_t)fread(Buffer, 1, Read > ContentLength ? ContentLength : Read, FileStream)) <= 0)
					{
						Core::OS::File::Close(FileStream);
						return Callback(Core::SystemException("upload file io error", Core::OS::Error::GetCondition()));
					}

					ContentLength -= Read;
					auto Written = Net.Stream->Write(Buffer, Read);
					if (!Written || !*Written)
					{
						Core::OS::File::Close(FileStream);
						if (!Written)
							return Callback(Core::SystemException("upload file network error", std::move(Written.Error())));

						return Callback(Core::Expectation::Met);
					}
				}
			}
			void Client::UploadFileChunkQueued(FILE* FileStream, size_t ContentLength, std::function<void(Core::ExpectsSystem<void>&&)>&& Callback)
			{
			Retry:
				if (!ContentLength)
				{
					Core::OS::File::Close(FileStream);
					return Callback(Core::Expectation::Met);
				}

				uint8_t Buffer[Core::BLOB_SIZE];
				size_t Read = sizeof(Buffer);
				if ((Read = (size_t)fread(Buffer, 1, Read > ContentLength ? ContentLength : Read, FileStream)) <= 0)
				{
					Core::OS::File::Close(FileStream);
					return Callback(Core::SystemException("upload file io error", Core::OS::Error::GetCondition()));
				}

				ContentLength -= Read;
				auto Written = Net.Stream->WriteQueued(Buffer, Read, [this, FileStream, ContentLength, Callback](SocketPoll Event) mutable
				{
					if (Packet::IsDoneAsync(Event))
					{
						Core::Cospawn([this, FileStream, ContentLength, Callback = std::move(Callback)]() mutable
						{
							UploadFileChunkQueued(FileStream, ContentLength, std::move(Callback));
						});
					}
					else if (Packet::IsErrorOrSkip(Event))
					{
						Core::OS::File::Close(FileStream);
						return Callback(Core::SystemException("upload file network error", std::make_error_condition(std::errc::connection_aborted)));
					}
				});

				if (Written && *Written > 0)
					goto Retry;
			}
			void Client::Upload(size_t FileId)
			{
				if (FileId < Boundaries.size())
				{
					BoundaryBlock* Boundary = &Boundaries[FileId];
					Net.Stream->WriteQueued((uint8_t*)Boundary->Data.c_str(), Boundary->Data.size(), [this, Boundary, FileId](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
						{
							if (!Boundary->File->IsInMemory)
							{
								BoundaryBlock& Block = *Boundary;
								UploadFile(&Block, [this, Boundary, FileId](Core::ExpectsSystem<void>&& Status)
								{
									if (Status)
									{
										Net.Stream->WriteQueued((uint8_t*)Boundary->Finish.c_str(), Boundary->Finish.size(), [this, FileId](SocketPoll Event)
										{
											if (Packet::IsDone(Event))
												Upload(FileId + 1);
											else if (Packet::IsErrorOrSkip(Event))
												Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
										}, false);
									}
									else
										Report(std::move(Status));
								});
							}
							else
								Upload(FileId + 1);
						}
						else if (Packet::IsErrorOrSkip(Event))
							Report(Core::SystemException(Event == SocketPoll::Timeout ? "write timeout error" : "write abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));
					}, false);
				}
				else
				{
					Net.Stream->ReadUntilChunkedQueued("\r\n\r\n", [this](SocketPoll Event, const uint8_t* Buffer, size_t Recv)
					{
						if (Packet::IsData(Event))
							Response.Content.Append(std::string_view((char*)Buffer, Recv));
						else if (Packet::IsDone(Event))
							Receive(Buffer, Recv);
						else if (Packet::IsErrorOrSkip(Event))
							Report(Core::SystemException(Event == SocketPoll::Timeout ? "read timeout error" : "read abort error", std::make_error_condition(Event == SocketPoll::Timeout ? std::errc::timed_out : std::errc::connection_aborted)));

						return true;
					});
				}
			}
			void Client::ManageKeepAlive()
			{
				auto Connection = Response.Headers.find("Connection");
				if (Connection == Response.Headers.end())
					return DisableReusability();

				if (Connection->second.size() != 1 || !Core::Stringify::CaseEquals(Connection->second.front(), "keep-alive"))
					return DisableReusability();

				return EnableReusability();
			}
			void Client::Receive(const uint8_t* LeftoverBuffer, size_t LeftoverSize)
			{
				Resolver->PrepareForResponseParsing(&Response);
				if (Resolver->ParseResponse((uint8_t*)Response.Content.Data.data(), Response.Content.Data.size(), 0) >= 0)
				{
					Response.Content.Prepare(Response.Headers, LeftoverBuffer, LeftoverSize);
					ManageKeepAlive();
					Report(Core::Expectation::Met);
				}
				else
					Report(Core::SystemException(Core::Stringify::Text("http chunk parse error: %.*s ...", (int)std::min<size_t>(64, Response.Content.Data.size()), Response.Content.Data.data()), std::make_error_condition(std::errc::bad_message)));
			}

			Core::ExpectsPromiseSystem<ResponseFrame> Fetch(const std::string_view& Location, const std::string_view& Method, const FetchFrame& Options)
			{
				Network::Location Origin(Location);
				if (Origin.Protocol != "http" && Origin.Protocol != "https")
					return Core::ExpectsPromiseSystem<ResponseFrame>(Core::SystemException("http fetch: invalid protocol", std::make_error_condition(std::errc::address_family_not_supported)));

				HTTP::RequestFrame Request;
				Request.Cookies = Options.Cookies;
				Request.Headers = Options.Headers;
				Request.Content = Options.Content;
				Request.Location.assign(Origin.Path);
				Request.SetMethod(Method);
				if (!Origin.Username.empty() || !Origin.Password.empty())
					Request.SetHeader("Authorization", Permissions::Authorize(Origin.Username, Origin.Password));

				for (auto& Item : Origin.Query)
					Request.Query += Item.first + "=" + Item.second + "&";
				if (!Request.Query.empty())
					Request.Query.pop_back();

				size_t MaxSize = Options.MaxSize;
				uint64_t Timeout = Options.Timeout;
				bool Secure = Origin.Protocol == "https";
				Core::String Hostname = Origin.Hostname;
				Core::String Port = Origin.Port > 0 ? Core::ToString(Origin.Port) : Core::String(Secure ? "443" : "80");
				int32_t VerifyPeers = (Secure ? (Options.VerifyPeers >= 0 ? Options.VerifyPeers : PEER_NOT_VERIFIED) : PEER_NOT_SECURE);
				return DNS::Get()->LookupDeferred(Hostname, Port, DNSType::Connect, SocketProtocol::TCP, SocketType::Stream).Then<Core::ExpectsPromiseSystem<ResponseFrame>>([MaxSize, Timeout, VerifyPeers, Request = std::move(Request), Origin = std::move(Origin)](Core::ExpectsSystem<SocketAddress>&& Address) mutable -> Core::ExpectsPromiseSystem<ResponseFrame>
				{
					if (!Address)
						return Core::ExpectsPromiseSystem<ResponseFrame>(Address.Error());

					HTTP::Client* Client = new HTTP::Client(Timeout);
					return Client->Connect(*Address, true, VerifyPeers).Then<Core::ExpectsPromiseSystem<void>>([Client, MaxSize, Request = std::move(Request)](Core::ExpectsSystem<void>&& Status) mutable -> Core::ExpectsPromiseSystem<void>
					{
						if (!Status)
							return Core::ExpectsPromiseSystem<void>(Status);

						return Client->SendFetch(std::move(Request), MaxSize);
					}).Then<Core::ExpectsPromiseSystem<ResponseFrame>>([Client](Core::ExpectsSystem<void>&& Status) -> Core::ExpectsPromiseSystem<ResponseFrame>
					{
						if (!Status)
						{
							Client->Release();
							return Core::ExpectsPromiseSystem<ResponseFrame>(Status.Error());
						}

						auto Response = std::move(*Client->GetResponse());
						return Client->Disconnect().Then<Core::ExpectsSystem<ResponseFrame>>([Client, Response = std::move(Response)](Core::ExpectsSystem<void>&&) mutable -> Core::ExpectsSystem<ResponseFrame>
						{
							Client->Release();
							return std::move(Response);
						});
					});
				});
			}
		}
	}
}
#pragma warning(pop)