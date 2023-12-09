#include "http.h"
#include "../core/bindings.h"
#ifdef VI_MICROSOFT
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
#define WEBSOCKET_KEY "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define MAX_REDIRECTS 128
#define GZ_HEADER_SIZE 17
#pragma warning(push)
#pragma warning(disable: 4996)

namespace Mavi
{
	namespace Network
	{
		namespace HTTP
		{
			static void TextAppend(Core::Vector<char>& Array, const Core::String& Src)
			{
				Array.insert(Array.end(), Src.begin(), Src.end());
			}
			static void TextAppend(Core::Vector<char>& Array, const char* Buffer, size_t Size)
			{
				Array.insert(Array.end(), Buffer, Buffer + Size);
			}
			static void TextAssign(Core::Vector<char>& Array, const Core::String& Src)
			{
				Array.assign(Src.begin(), Src.end());
			}
			static void TextAssign(Core::Vector<char>& Array, const char* Buffer, size_t Size)
			{
				Array.assign(Buffer, Buffer + Size);
			}
			static Core::String TextSubstring(Core::Vector<char>& Array, size_t Offset, size_t Size)
			{
				return Core::String(Array.data() + Offset, Size);
			}
			static Core::String TextHTML(const Core::String& Result)
			{
				if (Result.empty())
					return Result;

				Core::String Copy(Result);
				Core::Stringify::Replace(Copy, "&", "&amp;");
				Core::Stringify::Replace(Copy, "<", "&lt;");
				Core::Stringify::Replace(Copy, ">", "&gt;");
				Core::Stringify::Replace(Copy, "\n", "<br>");
				return Copy;
			}

			MimeStatic::MimeStatic(const char* Ext, const char* T) : Extension(Ext), Type(T)
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

			WebSocketFrame::WebSocketFrame(Socket* NewStream) : State((uint32_t)WebSocketState::Open), Tunneling((uint32_t)Tunnel::Healthy), Active(true), Deadly(false), Busy(false), Stream(NewStream), Codec(new WebCodec())
			{
			}
			WebSocketFrame::~WebSocketFrame() noexcept
			{
				while (!Messages.empty())
				{
					auto& Next = Messages.front();
					VI_FREE(Next.Buffer);
					Messages.pop();
				}

				VI_RELEASE(Codec);
				if (Lifetime.Destroy)
					Lifetime.Destroy(this);
			}
			void WebSocketFrame::Send(const char* Buffer, size_t Size, WebSocketOp Opcode, const WebSocketCallback& Callback)
			{
				Send(0, Buffer, Size, Opcode, Callback);
			}
			void WebSocketFrame::Send(unsigned int Mask, const char* Buffer, size_t Size, WebSocketOp Opcode, const WebSocketCallback& Callback)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				Core::UMutex<std::mutex> Unique(Section);
				if (Enqueue(Mask, Buffer, Size, Opcode, Callback))
					return;

				Busy = true;
				Unique.Negate();

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
					uint32_t Length2 = htonl((uint64_t)Size & 0xFFFFFFFF);
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

				Stream->WriteAsync((const char*)Header, HeaderLength, [this, Buffer, Size, Callback](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
					{
						if (Size > 0)
						{
							Stream->WriteAsync(Buffer, Size, [this, Callback](SocketPoll Event)
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
			}
			void WebSocketFrame::Dequeue()
			{
				Core::UMutex<std::mutex> Unique(Section);
				if (!IsWriteable() || Messages.empty())
					return;

				Message Next = std::move(Messages.front());
				Messages.pop();
				Unique.Negate();

				Send(Next.Mask, Next.Buffer, Next.Size, Next.Opcode, Next.Callback);
				VI_FREE(Next.Buffer);
			}
			void WebSocketFrame::Finish()
			{
				if (Deadly)
					return;

				if (State == (uint32_t)WebSocketState::Close || Tunneling != (uint32_t)Tunnel::Healthy)
					return Next();

				if (!Active)
				{
					VI_ERR("[websocket] sock %i already closing", (int)Stream->GetFd());
					return;
				}

				Finalize();
				Send("", 0, WebSocketOp::Close, [this](WebSocketFrame*)
				{
					Next();
				});
			}
			void WebSocketFrame::Finalize()
			{
				if (Tunneling == (uint32_t)Tunnel::Healthy)
					State = (uint32_t)WebSocketState::Close;
			}
			void WebSocketFrame::Next()
			{
				Core::Schedule::Get()->SetTask(std::bind(&WebSocketFrame::Update, this));
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
					char Buffer[Core::BLOB_SIZE];
					while (true)
					{
						auto Status = Stream->Read(Buffer, sizeof(Buffer), [this](SocketPoll Event, const char* Buffer, size_t Recv)
						{
							if (Packet::IsData(Event))
								Codec->ParseFrame(Buffer, Recv);

							return true;
						});
						if (Status)
							continue;

						if (Status.Error() == std::errc::operation_would_block)
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
						if (Opcode == WebSocketOp::Binary)
							VI_DEBUG("[websocket] sock %i frame binary: %s", (int)Stream->GetFd(), Compute::Codec::HexEncode(Codec->Data.data(), Codec->Data.size()).c_str());
						else
							VI_DEBUG("[websocket] sock %i frame text: %.*s", (int)Stream->GetFd(), (int)Codec->Data.size(), Codec->Data.data());

						if (Receive)
						{
							Unique.Negate();
							if (!Receive(this, Opcode, Codec->Data.data(), Codec->Data.size()))
								Next();
						}
					}
					else if (Opcode == WebSocketOp::Ping)
					{
						VI_DEBUG("[websocket] sock %i frame ping", (int)Stream->GetFd());
						Unique.Negate();
						if (!Receive || !Receive(this, Opcode, "", 0))
							Send("", 0, WebSocketOp::Pong, [this](WebSocketFrame*) { Next(); });
					}
					else if (Opcode == WebSocketOp::Close)
					{
						VI_DEBUG("[websocket] sock %i frame close", (int)Stream->GetFd());
						Unique.Negate();
						if (!Receive || !Receive(this, Opcode, "", 0))
							Finish();
					}
					else if (Receive)
					{
						Unique.Negate();
						if (!Receive(this, Opcode, "", 0))
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
						Finish();
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
				return !Busy && !Stream->IsPendingForWrite();
			}
			Socket* WebSocketFrame::GetStream()
			{
				return Stream;
			}
			Connection* WebSocketFrame::GetConnection()
			{
				return Stream ? (Connection*)Stream->UserData : nullptr;
			}
			bool WebSocketFrame::Enqueue(unsigned int Mask, const char* Buffer, size_t Size, WebSocketOp Opcode, const WebSocketCallback& Callback)
			{
				if (IsWriteable())
					return false;

				Message Next;
				Next.Mask = Mask;
				Next.Buffer = (Size > 0 ? VI_MALLOC(char, sizeof(char) * Size) : nullptr);
				Next.Size = Size;
				Next.Opcode = Opcode;
				Next.Callback = Callback;

				if (Next.Buffer != nullptr)
					memcpy(Next.Buffer, Buffer, sizeof(char) * Size);

				Messages.emplace(std::move(Next));
				return true;
			}

			RouteGroup::RouteGroup(const Core::String& NewMatch, RouteMode NewMode) noexcept : Match(NewMatch), Mode(NewMode)
			{
			}
			RouteGroup::~RouteGroup() noexcept
			{
				for (auto* Entry : Routes)
					VI_RELEASE(Entry);
				Routes.clear();
			}

			RouteEntry::RouteEntry(RouteEntry* Other, const Compute::RegexSource& Source)
			{
				VI_ASSERT(Other != nullptr, "other should be set");
				Callbacks = Other->Callbacks;
				Auth = Other->Auth;
				Compression = Other->Compression;
				HiddenFiles = Other->HiddenFiles;
				ErrorFiles = Other->ErrorFiles;
				MimeTypes = Other->MimeTypes;
				IndexFiles = Other->IndexFiles;
				TryFiles = Other->TryFiles;
				DisallowedMethods = Other->DisallowedMethods;
				DocumentRoot = Other->DocumentRoot;
				CharSet = Other->CharSet;
				ProxyIpAddress = Other->ProxyIpAddress;
				AccessControlAllowOrigin = Other->AccessControlAllowOrigin;
				Redirect = Other->Redirect;
				Override = Other->Override;
				WebSocketTimeout = Other->WebSocketTimeout;
				StaticFileMaxAge = Other->StaticFileMaxAge;
				MaxCacheLength = Other->MaxCacheLength;
				Level = Other->Level;
				AllowDirectoryListing = Other->AllowDirectoryListing;
				AllowWebSocket = Other->AllowWebSocket;
				AllowSendFile = Other->AllowSendFile;
				Site = Other->Site;
				URI = Source;
			}

			SiteEntry::SiteEntry() : Base(new RouteEntry())
			{
				Base->URI = Compute::RegexSource("/");
				Base->Site = this;
			}
			SiteEntry::~SiteEntry() noexcept
			{
				for (auto& Item : Groups)
					VI_RELEASE(Item);

				Groups.clear();
				VI_CLEAR(Base);
			}
			void SiteEntry::Sort()
			{
				VI_SORT(Groups.begin(), Groups.end(), [](const RouteGroup* A, const RouteGroup* B)
				{
					return A->Match.size() > B->Match.size();
				});

				for (auto& Group : Groups)
				{
					static auto Comparator = [](const RouteEntry* A, const RouteEntry* B)
					{
						if (A->URI.GetRegex().empty())
							return false;

						if (B->URI.GetRegex().empty())
							return true;

						if (A->Level > B->Level)
							return true;
						else if (A->Level < B->Level)
							return false;

						bool fA = A->URI.IsSimple(), fB = B->URI.IsSimple();
						if (fA && !fB)
							return false;
						else if (!fA && fB)
							return true;

						return A->URI.GetRegex().size() > B->URI.GetRegex().size();
					};
					VI_SORT(Group->Routes.begin(), Group->Routes.end(), Comparator);
				}
			}
			RouteGroup* SiteEntry::Group(const Core::String& Match, RouteMode Mode)
			{
				for (auto& Group : Groups)
				{
					if (Group->Match == Match && Group->Mode == Mode)
						return Group;
				}

				auto* Result = new RouteGroup(Match, Mode);
				Groups.emplace_back(Result);

				return Result;
			}
			RouteEntry* SiteEntry::Route(const Core::String& Match, RouteMode Mode, const Core::String& Pattern, bool InheritProps)
			{
				if (Pattern.empty() || Pattern == "/")
					return Base;

				HTTP::RouteGroup* Source = nullptr;
				for (auto& Group : Groups)
				{
					if (Group->Match != Match || Group->Mode != Mode)
						continue;

					Source = Group;
					for (auto* Entry : Group->Routes)
					{
						if (Entry->URI.GetRegex() == Pattern)
							return Entry;
					}
				}

				if (!Source)
				{
					auto* Result = new RouteGroup(Match, Mode);
					Groups.emplace_back(Result);
					Source = Groups.back();
				}

				if (!InheritProps)
					return Route(Pattern, Source, nullptr);

				HTTP::RouteEntry* From = Base;
				Compute::RegexResult Result;
				Core::String Src(Pattern);
				Core::Stringify::ToLower(Src);

				for (auto& Group : Groups)
				{
					for (auto* Entry : Group->Routes)
					{
						Core::String Dest(Entry->URI.GetRegex());
						Core::Stringify::ToLower(Dest);

						if (Core::Stringify::StartsWith(Dest, "...") && Core::Stringify::EndsWith(Dest, "..."))
							continue;

						if (Core::Stringify::Find(Src, Dest).Found || Compute::Regex::Match(&Entry->URI, Result, Src))
						{
							From = Entry;
							break;
						}
					}
				}

				return Route(Pattern, Source, From);
			}
			RouteEntry* SiteEntry::Route(const Core::String& Pattern, RouteGroup* Group, RouteEntry* From)
			{
				VI_ASSERT(Group != nullptr, "group should be set");
				if (From != nullptr)
				{
					HTTP::RouteEntry* Result = new HTTP::RouteEntry(From, Compute::RegexSource(Pattern));
					Group->Routes.push_back(Result);
					return Result;
				}

				HTTP::RouteEntry* Result = new HTTP::RouteEntry();
				Result->URI = Compute::RegexSource(Pattern);
				Group->Routes.push_back(Result);
				return Result;
			}
			bool SiteEntry::Remove(RouteEntry* Source)
			{
				VI_ASSERT(Source != nullptr, "source should be set");
				for (auto& Group : Groups)
				{
					auto It = std::find(Group->Routes.begin(), Group->Routes.end(), Source);
					if (It != Group->Routes.end())
					{
						VI_RELEASE(*It);
						Group->Routes.erase(It);
						return true;
					}
				}

				return false;
			}
			bool SiteEntry::Get(const char* Pattern, const SuccessCallback& Callback)
			{
				return Get("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::Get(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Get = Callback;
				return true;
			}
			bool SiteEntry::Post(const char* Pattern, const SuccessCallback& Callback)
			{
				return Post("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::Post(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Post = Callback;
				return true;
			}
			bool SiteEntry::Put(const char* Pattern, const SuccessCallback& Callback)
			{
				return Put("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::Put(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Put = Callback;
				return true;
			}
			bool SiteEntry::Patch(const char* Pattern, const SuccessCallback& Callback)
			{
				return Patch("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::Patch(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Patch = Callback;
				return true;
			}
			bool SiteEntry::Delete(const char* Pattern, const SuccessCallback& Callback)
			{
				return Delete("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::Delete(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Delete = Callback;
				return true;
			}
			bool SiteEntry::Options(const char* Pattern, const SuccessCallback& Callback)
			{
				return Options("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::Options(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Options = Callback;
				return true;
			}
			bool SiteEntry::Access(const char* Pattern, const SuccessCallback& Callback)
			{
				return Access("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::Access(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Access = Callback;
				return true;
			}
			bool SiteEntry::Headers(const char* Pattern, const HeaderCallback& Callback)
			{
				return Headers("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::Headers(const Core::String& Match, RouteMode Mode, const char* Pattern, const HeaderCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Headers = Callback;
				return true;
			}
			bool SiteEntry::Authorize(const char* Pattern, const AuthorizeCallback& Callback)
			{
				return Authorize("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::Authorize(const Core::String& Match, RouteMode Mode, const char* Pattern, const AuthorizeCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.Authorize = Callback;
				return true;
			}
			bool SiteEntry::WebSocketInitiate(const char* Pattern, const SuccessCallback& Callback)
			{
				return WebSocketInitiate("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::WebSocketInitiate(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.WebSocket.Initiate = Callback;
				return true;
			}
			bool SiteEntry::WebSocketConnect(const char* Pattern, const WebSocketCallback& Callback)
			{
				return WebSocketConnect("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::WebSocketConnect(const Core::String& Match, RouteMode Mode, const char* Pattern, const WebSocketCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.WebSocket.Connect = Callback;
				return true;
			}
			bool SiteEntry::WebSocketDisconnect(const char* Pattern, const WebSocketCallback& Callback)
			{
				return WebSocketDisconnect("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::WebSocketDisconnect(const Core::String& Match, RouteMode Mode, const char* Pattern, const WebSocketCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.WebSocket.Disconnect = Callback;
				return true;
			}
			bool SiteEntry::WebSocketReceive(const char* Pattern, const WebSocketReadCallback& Callback)
			{
				return WebSocketReceive("", RouteMode::Start, Pattern, Callback);
			}
			bool SiteEntry::WebSocketReceive(const Core::String& Match, RouteMode Mode, const char* Pattern, const WebSocketReadCallback& Callback)
			{
				HTTP::RouteEntry* Value = Route(Match, Mode, Pattern, true);
				if (!Value)
					return false;

				Value->Callbacks.WebSocket.Receive = Callback;
				return true;
			}

			MapRouter::MapRouter()
			{
			}
			MapRouter::~MapRouter()
			{
				if (Lifetime.Destroy != nullptr)
					Lifetime.Destroy(this);
				for (auto& Entry : Sites)
					VI_RELEASE(Entry.second);
				Sites.clear();
			}
			SiteEntry* MapRouter::Site()
			{
				return Site("*");
			}
			SiteEntry* MapRouter::Site(const char* Pattern)
			{
				VI_ASSERT(Pattern != nullptr, "pattern should be set");
				auto It = Sites.find(Pattern);
				if (It != Sites.end())
					return It->second;

				Core::String Name = Pattern;
				if (!Name.empty())
					Core::Stringify::ToLower(Name);
				else
					Name = "*";

				HTTP::SiteEntry* Result = new HTTP::SiteEntry();
				Sites[Name] = Result;

				return Result;
			}

			void Resource::PutHeader(const Core::String& Label, const Core::String& Value)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				Headers[Label].push_back(Value);
			}
			void Resource::SetHeader(const Core::String& Label, const Core::String& Value)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto& Range = Headers[Label];
				Range.clear();
				Range.push_back(Value);
			}
			Core::String Resource::ComposeHeader(const Core::String& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Label);
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
			RangePayload* Resource::GetHeaderRanges(const Core::String& Label)
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				return (RangePayload*)&Headers[Label];
			}
			const Core::String* Resource::GetHeaderBlob(const Core::String& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Label);
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				const Core::String& Result = It->second.back();
				return &Result;
			}
			const char* Resource::GetHeader(const Core::String& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Label);
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				return It->second.back().c_str();
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
			void RequestFrame::SetMethod(const char* Value)
			{
				VI_ASSERT(Value != nullptr, "value should be set");
				memset(Method, 0, sizeof(Method));
				memcpy((void*)Method, (void*)Value, std::min<size_t>(strlen(Value), sizeof(Method)));
			}
			void RequestFrame::SetVersion(unsigned int Major, unsigned int Minor)
			{
				Core::String Value = "HTTP/" + Core::ToString(Major) + '.' + Core::ToString(Minor);
				memset(Version, 0, sizeof(Version));
				memcpy((void*)Version, (void*)Value.c_str(), std::min<size_t>(Value.size(), sizeof(Version)));
			}
			void RequestFrame::PutHeader(const Core::String& Key, const Core::String& Value)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				Headers[Key].push_back(Value);
			}
			void RequestFrame::SetHeader(const Core::String& Key, const Core::String& Value)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto& Range = Headers[Key];
				Range.clear();
				Range.push_back(Value);
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
				URI.clear();
				Where.clear();
			}
			Core::String RequestFrame::ComposeHeader(const Core::String& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Label);
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
			RangePayload* RequestFrame::GetCookieRanges(const Core::String& Key)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				return (RangePayload*)&Cookies[Key];
			}
			const Core::String* RequestFrame::GetCookieBlob(const Core::String& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Cookies.find(Key);
				if (It == Cookies.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				const Core::String& Result = It->second.back();
				return &Result;
			}
			const char* RequestFrame::GetCookie(const Core::String& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Cookies.find(Key);
				if (It == Cookies.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				return It->second.back().c_str();
			}
			RangePayload* RequestFrame::GetHeaderRanges(const Core::String& Key)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				return (RangePayload*)&Headers[Key];
			}
			const Core::String* RequestFrame::GetHeaderBlob(const Core::String& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Headers.find(Key);
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				const Core::String& Result = It->second.back();
				return &Result;
			}
			const char* RequestFrame::GetHeader(const Core::String& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Headers.find(Key);
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				return It->second.back().c_str();
			}
			Core::Vector<std::pair<size_t, size_t>> RequestFrame::GetRanges() const
			{
				const char* Range = GetHeader("Range");
				if (Range == nullptr)
					return Core::Vector<std::pair<size_t, size_t>>();

				Core::Vector<Core::String> Bases = Core::Stringify::Split(Range, ',');
				Core::Vector<std::pair<size_t, size_t>> Ranges;

				for (auto& Item : Bases)
				{
					Core::TextSettle Result = Core::Stringify::Find(Item, '-');
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

					int64_t From = std::stoll(std::string(Start, (size_t)StartLength));
					if (From == -1)
						break;

					int64_t To = std::stoll(std::string(End, (size_t)EndLength));
					if (To == -1 || To < From)
						break;

					Ranges.emplace_back(std::make_pair((size_t)From, (size_t)To));
				}

				return Ranges;
			}
			std::pair<size_t, size_t> RequestFrame::GetRange(Core::Vector<std::pair<size_t, size_t>>::iterator Range, size_t ContenLength) const
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

			ResponseFrame::ResponseFrame() : StatusCode(-1), Error(false)
			{
			}
			void ResponseFrame::PutHeader(const Core::String& Key, const Core::String& Value)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				Headers[Key].push_back(Value);
			}
			void ResponseFrame::SetHeader(const Core::String& Key, const Core::String& Value)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto& Range = Headers[Key];
				Range.clear();
				Range.push_back(Value);
			}
			void ResponseFrame::SetCookie(const Cookie& Value)
			{
				for (auto& Cookie : Cookies)
				{
					if (Core::Stringify::CaseCompare(Cookie.Name.c_str(), Value.Name.c_str()) == 0)
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
					if (Core::Stringify::CaseCompare(Cookie.Name.c_str(), Value.Name.c_str()) == 0)
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
			Core::String ResponseFrame::ComposeHeader(const Core::String& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Label);
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
			Cookie* ResponseFrame::GetCookie(const char* Key)
			{
				VI_ASSERT(Key != nullptr, "key should be set");
				for (size_t i = 0; i < Cookies.size(); i++)
				{
					Cookie* Result = &Cookies[i];
					if (!Core::Stringify::CaseCompare(Result->Name.c_str(), Key))
						return Result;
				}

				return nullptr;
			}
			RangePayload* ResponseFrame::GetHeaderRanges(const Core::String& Key)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				return (RangePayload*)&Headers[Key];
			}
			const Core::String* ResponseFrame::GetHeaderBlob(const Core::String& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Headers.find(Key);
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				const Core::String& Result = It->second.back();
				return &Result;
			}
			const char* ResponseFrame::GetHeader(const Core::String& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Headers.find(Key);
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				return It->second.back().c_str();
			}
			bool ResponseFrame::IsUndefined() const
			{
				return StatusCode <= 0;
			}
			bool ResponseFrame::IsOK() const
			{
				return StatusCode >= 200 && StatusCode < 400;
			}

			ContentFrame::ContentFrame() : Length(0), Offset(0), Exceeds(false), Limited(false)
			{
			}
			void ContentFrame::Append(const Core::String& Text)
			{
				TextAppend(Data, Text);
			}
			void ContentFrame::Append(const char* Buffer, size_t Size)
			{
				TextAppend(Data, Buffer, Size);
			}
			void ContentFrame::Assign(const Core::String& Text)
			{
				TextAssign(Data, Text);
			}
			void ContentFrame::Assign(const char* Buffer, size_t Size)
			{
				TextAssign(Data, Buffer, Size);
			}
			void ContentFrame::Prepare(const char* ContentLength)
			{
				Limited = (ContentLength != nullptr);
				if (Limited)
					Length = strtoull(ContentLength, nullptr, 10);
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

					for (auto& Path : Paths)
						Core::Schedule::Get()->SetTask([Path]() { Core::OS::File::Remove(Path.c_str()); });
				}

				Data.clear();
				Resources.clear();
				Length = 0;
				Offset = 0;
				Limited = false;
				Exceeds = false;
			}
			Core::Expects<Core::Schema*, Core::Exceptions::ParserException> ContentFrame::GetJSON() const
			{
				return Core::Schema::FromJSON(GetText());
			}
			Core::Expects<Core::Schema*, Core::Exceptions::ParserException> ContentFrame::GetXML() const
			{
				return Core::Schema::FromXML(GetText());
			}
			Core::String ContentFrame::GetText() const
			{
				return Core::String(Data.data(), Data.size());
			}
			bool ContentFrame::IsFinalized() const
			{
				if (!Limited)
					return Offset >= Length && Offset > 0;

				return Offset >= Length || Data.size() >= Length;
			}

			FetchFrame::FetchFrame() : Timeout(10000), VerifyPeers(9), MaxSize(PAYLOAD_SIZE)
			{
			}
			void FetchFrame::PutHeader(const Core::String& Key, const Core::String& Value)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				Headers[Key].push_back(Value);
			}
			void FetchFrame::SetHeader(const Core::String& Key, const Core::String& Value)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto& Range = Headers[Key];
				Range.clear();
				Range.push_back(Value);
			}
			void FetchFrame::Cleanup()
			{
				Content.Cleanup();
				Headers.clear();
				Cookies.clear();
			}
			Core::String FetchFrame::ComposeHeader(const Core::String& Label) const
			{
				VI_ASSERT(!Label.empty(), "label should not be empty");
				auto It = Headers.find(Label);
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
			RangePayload* FetchFrame::GetCookieRanges(const Core::String& Key)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				return (RangePayload*)&Cookies[Key];
			}
			const Core::String* FetchFrame::GetCookieBlob(const Core::String& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Cookies.find(Key);
				if (It == Cookies.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				const Core::String& Result = It->second.back();
				return &Result;
			}
			const char* FetchFrame::GetCookie(const Core::String& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Cookies.find(Key);
				if (It == Cookies.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				return It->second.back().c_str();
			}
			RangePayload* FetchFrame::GetHeaderRanges(const Core::String& Key)
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				return (RangePayload*)&Headers[Key];
			}
			const Core::String* FetchFrame::GetHeaderBlob(const Core::String& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Headers.find(Key);
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				const Core::String& Result = It->second.back();
				return &Result;
			}
			const char* FetchFrame::GetHeader(const Core::String& Key) const
			{
				VI_ASSERT(!Key.empty(), "key should not be empty");
				auto It = Headers.find(Key);
				if (It == Headers.end())
					return nullptr;

				if (It->second.empty())
					return nullptr;

				return It->second.back().c_str();
			}
			Core::Vector<std::pair<size_t, size_t>> FetchFrame::GetRanges() const
			{
				const char* Range = GetHeader("Range");
				if (Range == nullptr)
					return Core::Vector<std::pair<size_t, size_t>>();

				Core::Vector<Core::String> Bases = Core::Stringify::Split(Range, ',');
				Core::Vector<std::pair<size_t, size_t>> Ranges;

				for (auto& Item : Bases)
				{
					Core::TextSettle Result = Core::Stringify::Find(Item, '-');
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

					int64_t From = std::stoll(std::string(Start, (size_t)StartLength));
					if (From == -1)
						break;

					int64_t To = std::stoll(std::string(End, (size_t)EndLength));
					if (To == -1 || To < From)
						break;

					Ranges.emplace_back(std::make_pair((size_t)From, (size_t)To));
				}

				return Ranges;
			}
			std::pair<size_t, size_t> FetchFrame::GetRange(Core::Vector<std::pair<size_t, size_t>>::iterator Range, size_t ContenLength) const
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

			Connection::Connection(Server* Source) noexcept : Root(Source)
			{
				Parsers.Request = new HTTP::Parser();
				Parsers.Request->OnMethodValue = Parsing::ParseMethodValue;
				Parsers.Request->OnPathValue = Parsing::ParsePathValue;
				Parsers.Request->OnQueryValue = Parsing::ParseQueryValue;
				Parsers.Request->OnVersion = Parsing::ParseVersion;
				Parsers.Request->OnHeaderField = Parsing::ParseHeaderField;
				Parsers.Request->OnHeaderValue = Parsing::ParseHeaderValue;

				Parsers.Multipart = new HTTP::Parser();
				Parsers.Multipart->OnContentData = Parsing::ParseMultipartContentData;
				Parsers.Multipart->OnHeaderField = Parsing::ParseMultipartHeaderField;
				Parsers.Multipart->OnHeaderValue = Parsing::ParseMultipartHeaderValue;
				Parsers.Multipart->OnResourceBegin = Parsing::ParseMultipartResourceBegin;
				Parsers.Multipart->OnResourceEnd = Parsing::ParseMultipartResourceEnd;
			}
			Connection::~Connection() noexcept
			{
				VI_CLEAR(Parsers.Request);
				VI_CLEAR(Parsers.Multipart);
			}
			void Connection::Reset(bool Fully)
			{
				if (!Fully)
					Info.Close = (Info.Close || Response.StatusCode < 0);

				Route = nullptr;
				Request.Cleanup();
				Response.Cleanup();
				SocketConnection::Reset(Fully);
			}
			bool Connection::Consume(const ContentCallback& Callback, bool Eat)
			{
				if (!Request.Content.Resources.empty())
				{
					if (Callback)
						Callback(this, SocketPoll::Timeout, nullptr, 0);

					return false;
				}
				else if (Request.Content.IsFinalized())
				{
					if (!Eat && Callback && !Request.Content.Data.empty())
						Callback(this, SocketPoll::Next, Request.Content.Data.data(), (int)Request.Content.Data.size());

					if (Callback)
						Callback(this, SocketPoll::FinishSync, nullptr, 0);

					return true;
				}
				else if (Request.Content.Exceeds)
				{
					if (Callback)
						Callback(this, SocketPoll::FinishSync, nullptr, 0);

					return false;
				}
				else if (!Stream->IsValid())
				{
					if (Callback)
						Callback(this, SocketPoll::Reset, nullptr, 0);

					return false;
				}

				const char* ContentType = Request.GetHeader("Content-Type");
				if (ContentType && !strncmp(ContentType, "multipart/form-data", 19))
				{
					Request.Content.Exceeds = true;
					if (Callback)
						Callback(this, SocketPoll::FinishSync, nullptr, 0);

					return true;
				}

				const char* TransferEncoding = Request.GetHeader("Transfer-Encoding");
				if (!Request.Content.Limited && TransferEncoding && !Core::Stringify::CaseCompare(TransferEncoding, "chunked"))
				{
					Parser* Parser = new HTTP::Parser();
					return !!Stream->ReadAsync(Root->Router->PayloadMaxLength, [this, Parser, Eat, Callback](SocketPoll Event, const char* Buffer, size_t Recv)
					{
						if (Packet::IsData(Event))
						{
							int64_t Result = Parser->ParseDecodeChunked((char*)Buffer, &Recv);
							if (Result >= 0 || Result == -2)
							{
								Request.Content.Offset += Recv;
								if (Eat)
									return Result == -2;

								if (Callback)
									Callback(this, SocketPoll::Next, Buffer, Recv);

								if (!Route || Request.Content.Data.size() < Route->MaxCacheLength)
									Request.Content.Append(Buffer, Recv);
							}
							else if (Result == -1)
							{
								VI_RELEASE(Parser);
								if (Callback)
									Callback(this, SocketPoll::Timeout, nullptr, 0);

								return false;
							}

							return Result == -2;
						}
						else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
						{
							Request.Content.Finalize();
							if (Callback)
								Callback(this, Event, nullptr, 0);
						}

						VI_RELEASE(Parser);
						return true;
					});
				}
				else if (!Request.Content.Limited)
				{
					Request.Content.Finalize();
					if (Callback)
						Callback(this, SocketPoll::FinishSync, nullptr, 0);

					return true;
				}
				else if (!Route || Request.Content.Length > Route->MaxCacheLength || Request.Content.Length > Root->Router->PayloadMaxLength)
				{
					Request.Content.Exceeds = true;
					if (Callback)
						Callback(this, SocketPoll::Timeout, nullptr, 0);

					return true;
				}

				return !!Stream->ReadAsync(Request.Content.Length, [this, Eat, Callback](SocketPoll Event, const char* Buffer, size_t Recv)
				{
					if (Packet::IsData(Event))
					{
						Request.Content.Offset += Recv;
						if (Eat)
							return true;

						if (Callback)
							Callback(this, SocketPoll::Next, Buffer, Recv);

						if (!Route || Request.Content.Data.size() < Route->MaxCacheLength)
							Request.Content.Append(Buffer, Recv);
					}
					else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
					{
						Request.Content.Finalize();
						if (Callback)
							Callback(this, Event, nullptr, 0);

						return false;
					}

					return true;
				});
			}
			bool Connection::Store(const ResourceCallback& Callback, bool Eat)
			{
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

				const char* ContentType = Request.GetHeader("Content-Type");
				const char* BoundaryName;
				Request.Content.Exceeds = true;

				if (ContentType != nullptr && (BoundaryName = strstr(ContentType, "boundary=")))
				{
					if (Route->Site->ResourceRoot.empty())
						Eat = true;

					Core::String Boundary("--");
					Boundary.append(BoundaryName + 9);

					Parsers.Multipart->PrepareForNextParsing(this, true);
					Parsers.Multipart->Frame.Callback = Callback;
					Parsers.Multipart->Frame.Ignore = Eat;

					return !!Stream->ReadAsync(Request.Content.Length, [this, Boundary](SocketPoll Event, const char* Buffer, size_t Recv)
					{
						if (Packet::IsData(Event))
						{
							Request.Content.Offset += Recv;
							if (Parsers.Multipart->MultipartParse(Boundary.c_str(), Buffer, Recv) != -1 && !Parsers.Multipart->Frame.Close)
								return true;

							if (Parsers.Multipart->Frame.Callback)
								Parsers.Multipart->Frame.Callback(nullptr);

							return false;
						}
						else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
						{
							Request.Content.Finalize();
							if (Parsers.Multipart->Frame.Callback)
								Parsers.Multipart->Frame.Callback(nullptr);
						}

						return true;
					});
				}
				else if (Eat)
				{
					return !!Stream->ReadAsync(Request.Content.Length, [this, Callback](SocketPoll Event, const char* Buffer, size_t Recv)
					{
						if (!Packet::IsDone(Event) && !Packet::IsErrorOrSkip(Event))
						{
							Request.Content.Offset += Recv;
							return true;
						}

						Request.Content.Finalize();
						if (Callback)
							Callback(nullptr);

						return true;
					});
				}

				HTTP::Resource Subresource;
				Subresource.Length = Request.Content.Length;
				Subresource.Type = (ContentType ? ContentType : "application/octet-stream");

				auto Directory = Core::OS::Directory::GetWorking();
				if (!Directory)
					return false;

				auto Random = Compute::Crypto::RandomBytes(16);
				if (!Random)
					return false;

				auto Hash = Compute::Crypto::Hash(Compute::Digests::MD5(), *Random);
				if (!Hash)
					return false;

				Subresource.Path = *Directory + *Hash;
				auto Source = Core::OS::File::Open(Subresource.Path.c_str(), "wb");
				if (!Source)
					return false;

				FILE* File = *Source;
				return !!Stream->ReadAsync(Request.Content.Length, [this, File, Subresource, Callback](SocketPoll Event, const char* Buffer, size_t Recv)
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
			bool Connection::Skip(const SuccessCallback& Callback)
			{
				VI_ASSERT(Callback != nullptr, "callback should be set");
				if (!Request.Content.Resources.empty())
					return true;
				else if (Request.Content.IsFinalized())
					return true;

				Consume([Callback](HTTP::Connection* Base, SocketPoll Event, const char*, size_t)
				{
					if (!Packet::IsDone(Event) && !Packet::IsErrorOrSkip(Event))
						return true;

					if (!Base->Request.Content.Exceeds)
					{
						Callback(Base);
						return true;
					}

					return Base->Store([Base, Callback](HTTP::Resource* Resource)
					{
						Callback(Base);
						return true;
					}, true);
				}, true);

				return false;
			}
			bool Connection::Finish()
			{
				Core::UMutex<std::mutex> Unique(Info.Sync);
				if (WebSocket != nullptr)
				{
					if (!WebSocket->IsFinished())
					{
						Unique.Negate();
						WebSocket->Finish();
						return false;
					}

					VI_CLEAR(WebSocket);
				}

				Unique.Negate();
				if (Response.StatusCode < 0 || Stream->Outcome > 0 || !Stream->IsValid())
					return !!Root->Continue(this);

				if (Response.StatusCode >= 400 && !Response.Error && Response.Content.Data.empty())
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
							return Routing::RouteGET(this);
						}
					}

					const char* StatusText = Utils::StatusMessage(Response.StatusCode);
					Core::String Content;
					Content.append(Request.Version).append(" ");
					Content.append(Core::ToString(Response.StatusCode)).append(" ");
					Content.append(StatusText).append("\r\n");

					Paths::ConstructHeadUncache(this, Content);
					if (Route && Route->Callbacks.Headers)
						Route->Callbacks.Headers(this, Content);

					char Date[64];
					Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Info.Start / 1000);

					Core::String Auth;
					if (Route && Request.User.Type == Auth::Denied)
						Auth = "WWW-Authenticate: " + Route->Auth.Type + " realm=\"" + Route->Auth.Realm + "\"\r\n";

					int HasContents = (Response.StatusCode > 199 && Response.StatusCode != 204 && Response.StatusCode != 304);
					if (HasContents)
					{
						char Buffer[Core::BLOB_SIZE];
						Core::String Reason = TextHTML(Info.Message);
						snprintf(Buffer, sizeof(Buffer), "<html><head><title>%d %s</title><style>" CSS_MESSAGE_STYLE "%s</style></head><body><div><h1>%d %s</h1></div></body></html>\n", Response.StatusCode, StatusText, Reason.size() <= 128 ? CSS_NORMAL_FONT : CSS_SMALL_FONT, Response.StatusCode, Reason.empty() ? StatusText : Reason.c_str());

						if (Route && Route->Callbacks.Headers)
							Route->Callbacks.Headers(this, Content);

						size_t BufferSize = strlen(Buffer);
						Content.append("Date: ").append(Date).append("\r\n");
						Content.append(Utils::ConnectionResolve(this));
						Content.append("Content-Type: text/html; charset=").append(Route ? Route->CharSet.c_str() : "utf-8").append("\r\n");
						Content.append("Accept-Ranges: bytes\r\nContent-Length: ").append(Core::ToString(BufferSize)).append("\r\n");
						Content.append(Auth).append("\r\n").append(Buffer, BufferSize);
					}
					else
					{
						if (Route && Route->Callbacks.Headers)
							Route->Callbacks.Headers(this, Content);

						Content.append("Date: ").append(Date).append("\r\n");
						Content.append(Utils::ConnectionResolve(this));
						Content.append("Accept-Ranges: bytes\r\n");
						Content.append(Auth).append("\r\n");
					}

					return !!Stream->WriteAsync(Content.c_str(), Content.size(), [this](SocketPoll Event)
					{
						if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
							Root->Continue(this);
					});
				}

				Core::String Content;
				Content.append(Request.Version).append(" ");
				Content.append(Core::ToString(Response.StatusCode)).append(" ");
				Content.append(Utils::StatusMessage(Response.StatusCode)).append("\r\n");

				if (!Response.GetHeader("Date"))
				{
					char Buffer[64];
					Core::DateTime::FetchWebDateGMT(Buffer, sizeof(Buffer), Info.Start / 1000);
					Content.append("Date: ").append(Buffer).append("\r\n");
				}

				if (!Response.GetHeader("Connection"))
					Content.append(Utils::ConnectionResolve(this));

				const char* ContentType;
				if (!Response.GetHeader("Accept-Ranges"))
					Content.append("Accept-Ranges: bytes\r\n", 22);

				Core::String Boundary;
				if (!Response.GetHeader("Content-Type"))
				{
					if (Route != nullptr)
						ContentType = Utils::ContentType(Request.Path, &Route->MimeTypes);
					else
						ContentType = "application/octet-stream";

					if (Request.GetHeader("Range") != nullptr)
					{
						Boundary = Parsing::ParseMultipartDataBoundary();
						Content.append("Content-Type: multipart/byteranges; boundary=").append(Boundary).append("; charset=").append(Route ? Route->CharSet.c_str() : "utf-8").append("\r\n");
					}
					else
						Content.append("Content-Type: ").append(ContentType).append("; charset=").append(Route ? Route->CharSet.c_str() : "utf-8").append("\r\n");
				}
				else
					ContentType = Response.GetHeader("Content-Type");

				if (!Response.Content.Data.empty())
				{
#ifdef VI_ZLIB
					bool Deflate = false, Gzip = false;
					if (Resources::ResourceCompressed(this, Response.Content.Data.size()))
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
							fStream.avail_in = (uInt)Response.Content.Data.size();
							fStream.next_in = (Bytef*)Response.Content.Data.data();

							if (deflateInit2(&fStream, Route ? Route->Compression.QualityLevel : 8, Z_DEFLATED, (Gzip ? 15 | 16 : 15), Route ? Route->Compression.MemoryLevel : 8, Route ? (int)Route->Compression.Tune : 0) == Z_OK)
							{
								Core::String Buffer(Response.Content.Data.size(), '\0');
								fStream.avail_out = (uInt)Buffer.size();
								fStream.next_out = (Bytef*)Buffer.c_str();
								bool Compress = (deflate(&fStream, Z_FINISH) == Z_STREAM_END);
								bool Flush = (deflateEnd(&fStream) == Z_OK);

								if (Compress && Flush)
								{
									Response.Content.Assign(Buffer.c_str(), (size_t)fStream.total_out);
									if (!Response.GetHeader("Content-Encoding"))
									{
										if (Gzip)
											Content.append("Content-Encoding: gzip\r\n", 24);
										else
											Content.append("Content-Encoding: deflate\r\n", 27);
									}
								}
							}
						}
					}
#endif
					if (Request.GetHeader("Range") != nullptr)
					{
						Core::Vector<std::pair<size_t, size_t>> Ranges = Request.GetRanges();
						if (Ranges.size() > 1)
						{
							Core::String Data;
							for (auto It = Ranges.begin(); It != Ranges.end(); ++It)
							{
								std::pair<size_t, size_t> Offset = Request.GetRange(It, Response.Content.Data.size());
								Core::String ContentRange = Paths::ConstructContentRange(Offset.first, Offset.second, Response.Content.Data.size());

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
								Data.append(TextSubstring(Response.Content.Data, Offset.first, Offset.second));
								Data.append("\r\n", 2);
							}

							Data.append("--", 2);
							Data.append(Boundary);
							Data.append("--\r\n", 4);
							Response.Content.Assign(Data);
						}
						else
						{
							std::pair<size_t, size_t> Offset = Request.GetRange(Ranges.begin(), Response.Content.Data.size());
							if (!Response.GetHeader("Content-Range"))
								Content.append("Content-Range: ").append(Paths::ConstructContentRange(Offset.first, Offset.second, Response.Content.Data.size())).append("\r\n");

							Response.Content.Assign(TextSubstring(Response.Content.Data, Offset.first, Offset.second));
						}
					}

					if (!Response.GetHeader("Content-Length"))
						Content.append("Content-Length: ").append(Core::ToString(Response.Content.Data.size())).append("\r\n");
				}
				else if (!Response.GetHeader("Content-Length"))
					Content.append("Content-Length: 0\r\n", 19);

				Paths::ConstructHeadFull(&Request, &Response, false, Content);
				if (Route && Route->Callbacks.Headers)
					Route->Callbacks.Headers(this, Content);

				Content.append("\r\n", 2);
				return !!Stream->WriteAsync(Content.c_str(), Content.size(), [this](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
					{
						if (memcmp(Request.Method, "HEAD", 4) != 0 && !Response.Content.Data.empty())
						{
							Stream->WriteAsync(Response.Content.Data.data(), Response.Content.Data.size(), [this](SocketPoll Event)
							{
								if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
									Root->Continue(this);
							});
						}
						else
							Root->Continue(this);
					}
					else if (Packet::IsErrorOrSkip(Event))
						Root->Continue(this);
				});
			}
			bool Connection::Finish(int StatusCode)
			{
				Response.StatusCode = StatusCode;
				return Finish();
			}

			Query::Query() : Object(Core::Var::Set::Object())
			{
			}
			Query::~Query() noexcept
			{
				VI_RELEASE(Object);
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

				VI_RELEASE(*Output);
				*Output = Object;
				Object = nullptr;
			}
			void Query::NewParameter(Core::Vector<QueryToken>* Tokens, const QueryToken& Name, const QueryToken& Value)
			{
				Core::String URI = Compute::Codec::URIDecode(Name.Value, Name.Length);
				char* Data = (char*)URI.c_str();

				size_t Offset = 0, Length = URI.size();
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
					Parameter->Value.Deserialize(Compute::Codec::URIDecode(Value.Value, Value.Length));
			}
			void Query::Decode(const char* Type, const Core::String& URI)
			{
				if (!Type || URI.empty())
					return;

				if (!Core::Stringify::CaseCompare(Type, "application/x-www-form-urlencoded"))
					DecodeAXWFD(URI);
				else if (!Core::Stringify::CaseCompare(Type, "application/json"))
					DecodeAJSON(URI);
			}
			void Query::DecodeAXWFD(const Core::String& URI)
			{
				Core::Vector<QueryToken> Tokens;
				char* Data = (char*)URI.c_str();

				size_t Offset = 0, Length = URI.size();
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
			void Query::DecodeAJSON(const Core::String& URI)
			{
				VI_CLEAR(Object);
				auto Result = Core::Schema::ConvertFromJSON(URI.c_str(), URI.size());
				if (Result)
					Object = *Result;
			}
			Core::String Query::Encode(const char* Type) const
			{
				if (Type != nullptr)
				{
					if (!Core::Stringify::CaseCompare(Type, "application/x-www-form-urlencoded"))
						return EncodeAXWFD();

					if (!Core::Stringify::CaseCompare(Type, "application/json"))
						return EncodeAJSON();
				}

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
				Core::Schema::ConvertToJSON(Object, [&Stream](Core::VarForm, const char* Buffer, size_t Length)
				{
					if (Buffer != nullptr && Length > 0)
						Stream.append(Buffer, Length);
				});

				return Stream;
			}
			Core::Schema* Query::Get(const char* Name) const
			{
				return (Core::Schema*)Object->Get(Name);
			}
			Core::Schema* Query::Set(const char* Name)
			{
				return (Core::Schema*)Object->Set(Name, Core::Var::String("", 0));
			}
			Core::Schema* Query::Set(const char* Name, const char* Value)
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

				New->Value = Core::Var::String("", 0);
				Object->Push(New);

				return New;
			}
			Core::String Query::Build(Core::Schema* Base)
			{
				Core::String Output, Label = Compute::Codec::URIEncode(Base->GetParent() != nullptr ? ('[' + Base->Key + ']') : Base->Key);
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
						Output.append(Label).append(1, '=').append(Compute::Codec::URIEncode(V));
					else
						Output.append(Label);
				}

				return Output;
			}
			Core::String Query::BuildFromBase(Core::Schema* Base)
			{
				Core::String Output, Label = Compute::Codec::URIEncode(Base->Key);
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
						Output.append(Label).append(1, '=').append(Compute::Codec::URIEncode(V));
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

				return Base->Set(Key, Core::Var::String("", 0));
			}

			Session::Session()
			{
				Query = Core::Var::Set::Object();
			}
			Session::~Session() noexcept
			{
				VI_RELEASE(Query);
			}
			void Session::Clear()
			{
				if (Query != nullptr)
					Query->Clear();
			}
			bool Session::Write(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				Core::String Path = Base->Route->Site->Session.DocumentRoot + FindSessionId(Base);
				auto Stream = Core::OS::File::Open(Path.c_str(), "wb");
				if (!Stream)
					return false;

				SessionExpires = time(nullptr) + Base->Route->Site->Session.Expires;
				fwrite(&SessionExpires, sizeof(int64_t), 1, *Stream);
				Query->ConvertToJSONB(Query, [&Stream](Core::VarForm, const char* Buffer, size_t Size)
				{
					if (Buffer != nullptr && Size > 0)
						fwrite(Buffer, Size, 1, *Stream);
				});
				Core::OS::File::Close(*Stream);
				return true;
			}
			bool Session::Read(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				Core::String Path = Base->Route->Site->Session.DocumentRoot + FindSessionId(Base);
				auto Stream = Core::OS::File::Open(Path.c_str(), "rb");
				if (!Stream)
					return false;

				if (fread(&SessionExpires, 1, sizeof(int64_t), *Stream) != sizeof(int64_t))
				{
					Core::OS::File::Close(*Stream);
					return false;
				}

				if (SessionExpires <= time(nullptr))
				{
					SessionId.clear();
					Core::OS::File::Close(*Stream);
					if (!Core::OS::File::Remove(Path.c_str()))
						VI_ERR("[http] session file %s cannot be deleted", Path.c_str());

					return false;
				}

				VI_RELEASE(Query);
				auto Result = Core::Schema::ConvertFromJSONB([&Stream](char* Buffer, size_t Size)
				{
					if (!Buffer || !Size)
						return true;

					return fread(Buffer, sizeof(char), Size, *Stream) == Size;
				});
				if (Result)
					Query = *Result;

				Core::OS::File::Close(*Stream);
				return true;
			}
			Core::String& Session::FindSessionId(Connection* Base)
			{
				if (!SessionId.empty())
					return SessionId;

				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				const char* Value = Base->Request.GetCookie(Base->Route->Site->Session.Cookie.Name.c_str());
				if (!Value)
					return GenerateSessionId(Base);

				return SessionId.assign(Value);
			}
			Core::String& Session::GenerateSessionId(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				int64_t Time = time(nullptr);
				auto Hash = Compute::Crypto::Hash(Compute::Digests::MD5(), Base->Request.URI + Core::ToString(Time));
				if (Hash)
					SessionId = *Hash;
				else
					SessionId = Core::ToString(Time);

				if (SessionExpires == 0)
					SessionExpires = Time + Base->Route->Site->Session.Expires;

				Cookie Result;
				Result.Value = SessionId;
				Result.Name = Base->Route->Site->Session.Cookie.Name;
				Result.Domain = Base->Route->Site->Session.Cookie.Domain;
				Result.Path = Base->Route->Site->Session.Cookie.Path;
				Result.SameSite = Base->Route->Site->Session.Cookie.SameSite;
				Result.Secure = Base->Route->Site->Session.Cookie.Secure;
				Result.HttpOnly = Base->Route->Site->Session.Cookie.HttpOnly;
				Result.SetExpires(Time + (int64_t)Base->Route->Site->Session.Cookie.Expires);
				Base->Response.SetCookie(std::move(Result));

				return SessionId;
			}
			Core::ExpectsIO<void> Session::InvalidateCache(const Core::String& Path)
			{
				Core::Vector<std::pair<Core::String, Core::FileEntry>> Entries;
				auto Status = Core::OS::Directory::Scan(Path, &Entries);
				if (!Status)
					return Status;

				bool Split = (Path.back() != '\\' && Path.back() != '/');
				for (auto& Item : Entries)
				{
					if (Item.second.IsDirectory)
						continue;

					Core::String Filename = (Split ? Path + '/' : Path) + Item.first;
					Status = Core::OS::File::Remove(Filename.c_str());
					if (!Status)
					{
						VI_ERR("[http] cannot invalidate session: %s", Item.first.c_str());
						return Status;
					}
				}

				return Core::Optional::OK;
			}

			Parser::Parser()
			{
			}
			Parser::~Parser() noexcept
			{
				VI_FREE(Multipart.Boundary);
			}
			void Parser::PrepareForNextParsing(Connection* Base, bool ForMultipart)
			{
				VI_ASSERT(Base != nullptr, "base should be set");
				Multipart = MultipartData();
				Chunked = ChunkedData();
				Frame.Request = &Base->Request;
				Frame.Response = nullptr;
				Frame.Route = nullptr;
				Frame.Stream = nullptr;
				Frame.Header.clear();
				Frame.Source = HTTP::Resource();
				Frame.Callback = nullptr;
				Frame.Close = false;
				Frame.Ignore = false;

				if (!ForMultipart)
					return;

				Frame.Response = &Base->Response;
				Frame.Route = Base->Route;
			}
			int64_t Parser::MultipartParse(const char* Boundary, const char* Buffer, size_t Length)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				VI_ASSERT(Boundary != nullptr, "boundary should be set");

				if (!Multipart.Boundary || !Multipart.LookBehind)
				{
					if (Multipart.Boundary)
						VI_FREE(Multipart.Boundary);

					Multipart.Length = strlen(Boundary);
					Multipart.Boundary = VI_MALLOC(char, sizeof(char) * (size_t)(Multipart.Length * 2 + 9));
					memcpy(Multipart.Boundary, Boundary, sizeof(char) * (size_t)Multipart.Length);
					Multipart.Boundary[Multipart.Length] = '\0';
					Multipart.LookBehind = (Multipart.Boundary + Multipart.Length + 1);
					Multipart.Index = 0;
					Multipart.State = MultipartState_Start;
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
								if (OnContentData && !OnContentData(this, Multipart.LookBehind, 2 + (size_t)Multipart.Index))
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
			int64_t Parser::ParseRequest(const char* BufferStart, size_t Length, size_t Offset)
			{
				VI_ASSERT(BufferStart != nullptr, "buffer start should be set");
				const char* Buffer = BufferStart;
				const char* BufferEnd = BufferStart + Length;
				int Result;
				
				if (IsCompleted(Buffer, BufferEnd, Offset, &Result) == nullptr)
					return (int64_t)Result;

				if ((Buffer = ProcessRequest(Buffer, BufferEnd, &Result)) == nullptr)
					return (int64_t)Result;

				return (int64_t)(Buffer - BufferStart);
			}
			int64_t Parser::ParseResponse(const char* BufferStart, size_t Length, size_t Offset)
			{
				VI_ASSERT(BufferStart != nullptr, "buffer start should be set");
				const char* Buffer = BufferStart;
				const char* BufferEnd = Buffer + Length;
				int Result;

				if (IsCompleted(Buffer, BufferEnd, Offset, &Result) == nullptr)
					return (int64_t)Result;

				if ((Buffer = ProcessResponse(Buffer, BufferEnd, &Result)) == nullptr)
					return (int64_t)Result;

				return (int64_t)(Buffer - BufferStart);
			}
			int64_t Parser::ParseDecodeChunked(char* Buffer, size_t* Length)
			{
				VI_ASSERT(Buffer != nullptr && Length != nullptr, "buffer should be set");
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
			const char* Parser::Tokenize(const char* Buffer, const char* BufferEnd, const char** Token, size_t* TokenLength, int* Out)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				VI_ASSERT(BufferEnd != nullptr, "buffer end should be set");
				VI_ASSERT(Token != nullptr, "token should be set");
				VI_ASSERT(TokenLength != nullptr, "token length should be set");
				VI_ASSERT(Out != nullptr, "output should be set");

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
			const char* Parser::IsCompleted(const char* Buffer, const char* BufferEnd, size_t Offset, int* Out)
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
			const char* Parser::ProcessVersion(const char* Buffer, const char* BufferEnd, int* Out)
			{
				VI_ASSERT(Buffer != nullptr, "buffer should be set");
				VI_ASSERT(BufferEnd != nullptr, "buffer end should be set");
				VI_ASSERT(Out != nullptr, "output should be set");

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

						if (OnHeaderField && !OnHeaderField(this, Name, (size_t)Length))
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

					const char* Value; size_t ValueLength;
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
					if (OnQueryValue && !OnQueryValue(this, Path + PL + 1, (size_t)QL))
					{
						*Out = -1;
						return nullptr;
					}
				}

				if (OnPathValue && !OnPathValue(this, Path, (size_t)PL))
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
				if (OnStatusCode && !OnStatusCode(this, Status))
				{
					*Out = -1;
					return nullptr;
				}

				const char* Message; size_t MessageLength;
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

			WebCodec::WebCodec() : State(Bytecode::Begin), Fragment(0)
			{
			}
			bool WebCodec::ParseFrame(const char* Buffer, size_t Size)
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
							TextAssign(Message, Data, Length);
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

			Core::String Utils::ConnectionResolve(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Root != nullptr && Base->Root->Router != nullptr, "connection should be set");
				if (Base->Info.KeepAlive <= 0)
					return "Connection: Close\r\n";

				if (Base->Response.StatusCode == 401)
					return "Connection: Close\r\n";

				if (Base->Root->Router->KeepAliveMaxCount == 0)
					return "Connection: Close\r\n";

				const char* Connection = Base->Request.GetHeader("Connection");
				if (Connection != nullptr && Core::Stringify::CaseCompare(Connection, "keep-alive"))
				{
					Base->Info.KeepAlive = 0;
					return "Connection: Close\r\n";
				}

				if (!Connection && strcmp(Base->Request.Version, "1.1") != 0)
					return "Connection: Close\r\n";

				if (Base->Root->Router->KeepAliveMaxCount < 0)
					return "Connection: Keep-Alive\r\nKeep-Alive: timeout=" + Core::ToString(Base->Root->Router->SocketTimeout / 1000) + "\r\n";

				return "Connection: Keep-Alive\r\nKeep-Alive: timeout=" + Core::ToString(Base->Root->Router->SocketTimeout / 1000) + ", max=" + Core::ToString(Base->Root->Router->KeepAliveMaxCount) + "\r\n";
			}
			const char* Utils::ContentType(const Core::String& Path, Core::Vector<MimeType>* Types)
			{
				VI_ASSERT(Types != nullptr, "types should be set");
				static MimeStatic MimeTypes[] = { MimeStatic(".3dm", "x-world/x-3dmf"), MimeStatic(".3dmf", "x-world/x-3dmf"), MimeStatic(".a", "application/octet-stream"), MimeStatic(".aab", "application/x-authorware-bin"), MimeStatic(".aac", "audio/aac"), MimeStatic(".aam", "application/x-authorware-map"), MimeStatic(".aas", "application/x-authorware-seg"), MimeStatic(".aat", "application/font-sfnt"), MimeStatic(".abc", "text/vnd.abc"), MimeStatic(".acgi", "text/html"), MimeStatic(".afl", "video/animaflex"), MimeStatic(".ai", "application/postscript"), MimeStatic(".aif", "audio/x-aiff"), MimeStatic(".aifc", "audio/x-aiff"), MimeStatic(".aiff", "audio/x-aiff"), MimeStatic(".aim", "application/x-aim"), MimeStatic(".aip", "text/x-audiosoft-intra"), MimeStatic(".ani", "application/x-navi-animation"), MimeStatic(".aos", "application/x-nokia-9000-communicator-add-on-software"), MimeStatic(".aps", "application/mime"), MimeStatic(".arc", "application/octet-stream"), MimeStatic(".arj", "application/arj"), MimeStatic(".art", "image/x-jg"), MimeStatic(".asf", "video/x-ms-asf"), MimeStatic(".asm", "text/x-asm"), MimeStatic(".asp", "text/asp"), MimeStatic(".asx", "video/x-ms-asf"), MimeStatic(".au", "audio/x-au"), MimeStatic(".avi", "video/x-msvideo"), MimeStatic(".avs", "video/avs-video"), MimeStatic(".bcpio", "application/x-bcpio"), MimeStatic(".bin", "application/x-binary"), MimeStatic(".bm", "image/bmp"), MimeStatic(".bmp", "image/bmp"), MimeStatic(".boo", "application/book"), MimeStatic(".book", "application/book"), MimeStatic(".boz", "application/x-bzip2"), MimeStatic(".bsh", "application/x-bsh"), MimeStatic(".bz", "application/x-bzip"), MimeStatic(".bz2", "application/x-bzip2"), MimeStatic(".c", "text/x-c"), MimeStatic(".c++", "text/x-c"), MimeStatic(".cat", "application/vnd.ms-pki.seccat"), MimeStatic(".cc", "text/x-c"), MimeStatic(".ccad", "application/clariscad"), MimeStatic(".cco", "application/x-cocoa"), MimeStatic(".cdf", "application/x-cdf"), MimeStatic(".cer", "application/pkix-cert"), MimeStatic(".cff", "application/font-sfnt"), MimeStatic(".cha", "application/x-chat"), MimeStatic(".chat", "application/x-chat"), MimeStatic(".class", "application/x-java-class"), MimeStatic(".com", "application/octet-stream"), MimeStatic(".conf", "text/plain"), MimeStatic(".cpio", "application/x-cpio"), MimeStatic(".cpp", "text/x-c"), MimeStatic(".cpt", "application/x-compactpro"), MimeStatic(".crl", "application/pkcs-crl"), MimeStatic(".crt", "application/x-x509-user-cert"), MimeStatic(".csh", "text/x-script.csh"), MimeStatic(".css", "text/css"), MimeStatic(".csv", "text/csv"), MimeStatic(".cxx", "text/plain"), MimeStatic(".dcr", "application/x-director"), MimeStatic(".deepv", "application/x-deepv"), MimeStatic(".def", "text/plain"), MimeStatic(".der", "application/x-x509-ca-cert"), MimeStatic(".dif", "video/x-dv"), MimeStatic(".dir", "application/x-director"), MimeStatic(".dl", "video/x-dl"), MimeStatic(".dll", "application/octet-stream"), MimeStatic(".doc", "application/msword"), MimeStatic(".dot", "application/msword"), MimeStatic(".dp", "application/commonground"), MimeStatic(".drw", "application/drafting"), MimeStatic(".dump", "application/octet-stream"), MimeStatic(".dv", "video/x-dv"), MimeStatic(".dvi", "application/x-dvi"), MimeStatic(".dwf", "model/vnd.dwf"), MimeStatic(".dwg", "image/vnd.dwg"), MimeStatic(".dxf", "image/vnd.dwg"), MimeStatic(".dxr", "application/x-director"), MimeStatic(".el", "text/x-script.elisp"), MimeStatic(".elc", "application/x-bytecode.elisp"), MimeStatic(".env", "application/x-envoy"), MimeStatic(".eps", "application/postscript"), MimeStatic(".es", "application/x-esrehber"), MimeStatic(".etx", "text/x-setext"), MimeStatic(".evy", "application/x-envoy"), MimeStatic(".exe", "application/octet-stream"), MimeStatic(".f", "text/x-fortran"), MimeStatic(".f77", "text/x-fortran"), MimeStatic(".f90", "text/x-fortran"), MimeStatic(".fdf", "application/vnd.fdf"), MimeStatic(".fif", "image/fif"), MimeStatic(".fli", "video/x-fli"), MimeStatic(".flo", "image/florian"), MimeStatic(".flx", "text/vnd.fmi.flexstor"), MimeStatic(".fmf", "video/x-atomic3d-feature"), MimeStatic(".for", "text/x-fortran"), MimeStatic(".fpx", "image/vnd.fpx"), MimeStatic(".frl", "application/freeloader"), MimeStatic(".funk", "audio/make"), MimeStatic(".g", "text/plain"), MimeStatic(".g3", "image/g3fax"), MimeStatic(".gif", "image/gif"), MimeStatic(".gl", "video/x-gl"), MimeStatic(".gsd", "audio/x-gsm"), MimeStatic(".gsm", "audio/x-gsm"), MimeStatic(".gsp", "application/x-gsp"), MimeStatic(".gss", "application/x-gss"), MimeStatic(".gtar", "application/x-gtar"), MimeStatic(".gz", "application/x-gzip"), MimeStatic(".h", "text/x-h"), MimeStatic(".hdf", "application/x-hdf"), MimeStatic(".help", "application/x-helpfile"), MimeStatic(".hgl", "application/vnd.hp-hpgl"), MimeStatic(".hh", "text/x-h"), MimeStatic(".hlb", "text/x-script"), MimeStatic(".hlp", "application/x-helpfile"), MimeStatic(".hpg", "application/vnd.hp-hpgl"), MimeStatic(".hpgl", "application/vnd.hp-hpgl"), MimeStatic(".hqx", "application/binhex"), MimeStatic(".hta", "application/hta"), MimeStatic(".htc", "text/x-component"), MimeStatic(".htm", "text/html"), MimeStatic(".html", "text/html"), MimeStatic(".htmls", "text/html"), MimeStatic(".htt", "text/webviewhtml"), MimeStatic(".htx", "text/html"), MimeStatic(".ice", "x-conference/x-cooltalk"), MimeStatic(".ico", "image/x-icon"), MimeStatic(".idc", "text/plain"), MimeStatic(".ief", "image/ief"), MimeStatic(".iefs", "image/ief"), MimeStatic(".iges", "model/iges"), MimeStatic(".igs", "model/iges"), MimeStatic(".ima", "application/x-ima"), MimeStatic(".imap", "application/x-httpd-imap"), MimeStatic(".inf", "application/inf"), MimeStatic(".ins", "application/x-internett-signup"), MimeStatic(".ip", "application/x-ip2"), MimeStatic(".isu", "video/x-isvideo"), MimeStatic(".it", "audio/it"), MimeStatic(".iv", "application/x-inventor"), MimeStatic(".ivr", "i-world/i-vrml"), MimeStatic(".ivy", "application/x-livescreen"), MimeStatic(".jam", "audio/x-jam"), MimeStatic(".jav", "text/x-java-source"), MimeStatic(".java", "text/x-java-source"), MimeStatic(".jcm", "application/x-java-commerce"), MimeStatic(".jfif", "image/jpeg"), MimeStatic(".jfif-tbnl", "image/jpeg"), MimeStatic(".jpe", "image/jpeg"), MimeStatic(".jpeg", "image/jpeg"), MimeStatic(".jpg", "image/jpeg"), MimeStatic(".jpm", "image/jpm"), MimeStatic(".jps", "image/x-jps"), MimeStatic(".jpx", "image/jpx"), MimeStatic(".js", "application/x-javascript"), MimeStatic(".json", "application/json"), MimeStatic(".jut", "image/jutvision"), MimeStatic(".kar", "music/x-karaoke"), MimeStatic(".kml", "application/vnd.google-earth.kml+xml"), MimeStatic(".kmz", "application/vnd.google-earth.kmz"), MimeStatic(".ksh", "text/x-script.ksh"), MimeStatic(".la", "audio/x-nspaudio"), MimeStatic(".lam", "audio/x-liveaudio"), MimeStatic(".latex", "application/x-latex"), MimeStatic(".lha", "application/x-lha"), MimeStatic(".lhx", "application/octet-stream"), MimeStatic(".lib", "application/octet-stream"), MimeStatic(".list", "text/plain"), MimeStatic(".lma", "audio/x-nspaudio"), MimeStatic(".log", "text/plain"), MimeStatic(".lsp", "text/x-script.lisp"), MimeStatic(".lst", "text/plain"), MimeStatic(".lsx", "text/x-la-asf"), MimeStatic(".ltx", "application/x-latex"), MimeStatic(".lzh", "application/x-lzh"), MimeStatic(".lzx", "application/x-lzx"), MimeStatic(".m", "text/x-m"), MimeStatic(".m1v", "video/mpeg"), MimeStatic(".m2a", "audio/mpeg"), MimeStatic(".m2v", "video/mpeg"), MimeStatic(".m3u", "audio/x-mpegurl"), MimeStatic(".m4v", "video/x-m4v"), MimeStatic(".man", "application/x-troff-man"), MimeStatic(".map", "application/x-navimap"), MimeStatic(".mar", "text/plain"), MimeStatic(".mbd", "application/mbedlet"), MimeStatic(".mc$", "application/x-magic-cap-package-1.0"), MimeStatic(".mcd", "application/x-mathcad"), MimeStatic(".mcf", "text/mcf"), MimeStatic(".mcp", "application/netmc"), MimeStatic(".me", "application/x-troff-me"), MimeStatic(".mht", "message/rfc822"), MimeStatic(".mhtml", "message/rfc822"), MimeStatic(".mid", "audio/x-midi"), MimeStatic(".midi", "audio/x-midi"), MimeStatic(".mif", "application/x-mif"), MimeStatic(".mime", "www/mime"), MimeStatic(".mjf", "audio/x-vnd.audioexplosion.mjuicemediafile"), MimeStatic(".mjpg", "video/x-motion-jpeg"), MimeStatic(".mm", "application/base64"), MimeStatic(".mme", "application/base64"), MimeStatic(".mod", "audio/x-mod"), MimeStatic(".moov", "video/quicktime"), MimeStatic(".mov", "video/quicktime"), MimeStatic(".movie", "video/x-sgi-movie"), MimeStatic(".mp2", "video/x-mpeg"), MimeStatic(".mp3", "audio/x-mpeg-3"), MimeStatic(".mp4", "video/mp4"), MimeStatic(".mpa", "audio/mpeg"), MimeStatic(".mpc", "application/x-project"), MimeStatic(".mpeg", "video/mpeg"), MimeStatic(".mpg", "video/mpeg"), MimeStatic(".mpga", "audio/mpeg"), MimeStatic(".mpp", "application/vnd.ms-project"), MimeStatic(".mpt", "application/x-project"), MimeStatic(".mpv", "application/x-project"), MimeStatic(".mpx", "application/x-project"), MimeStatic(".mrc", "application/marc"), MimeStatic(".ms", "application/x-troff-ms"), MimeStatic(".mv", "video/x-sgi-movie"), MimeStatic(".my", "audio/make"), MimeStatic(".mzz", "application/x-vnd.audioexplosion.mzz"), MimeStatic(".nap", "image/naplps"), MimeStatic(".naplps", "image/naplps"), MimeStatic(".nc", "application/x-netcdf"), MimeStatic(".ncm", "application/vnd.nokia.configuration-message"), MimeStatic(".nif", "image/x-niff"), MimeStatic(".niff", "image/x-niff"), MimeStatic(".nix", "application/x-mix-transfer"), MimeStatic(".nsc", "application/x-conference"), MimeStatic(".nvd", "application/x-navidoc"), MimeStatic(".o", "application/octet-stream"), MimeStatic(".obj", "application/octet-stream"), MimeStatic(".oda", "application/oda"), MimeStatic(".oga", "audio/ogg"), MimeStatic(".ogg", "audio/ogg"), MimeStatic(".ogv", "video/ogg"), MimeStatic(".omc", "application/x-omc"), MimeStatic(".omcd", "application/x-omcdatamaker"), MimeStatic(".omcr", "application/x-omcregerator"), MimeStatic(".otf", "application/font-sfnt"), MimeStatic(".p", "text/x-pascal"), MimeStatic(".p10", "application/x-pkcs10"), MimeStatic(".p12", "application/x-pkcs12"), MimeStatic(".p7a", "application/x-pkcs7-signature"), MimeStatic(".p7c", "application/x-pkcs7-mime"), MimeStatic(".p7m", "application/x-pkcs7-mime"), MimeStatic(".p7r", "application/x-pkcs7-certreqresp"), MimeStatic(".p7s", "application/pkcs7-signature"), MimeStatic(".part", "application/pro_eng"), MimeStatic(".pas", "text/x-pascal"), MimeStatic(".pbm", "image/x-portable-bitmap"), MimeStatic(".pcl", "application/vnd.hp-pcl"), MimeStatic(".pct", "image/x-pct"), MimeStatic(".pcx", "image/x-pcx"), MimeStatic(".pdb", "chemical/x-pdb"), MimeStatic(".pdf", "application/pdf"), MimeStatic(".pfr", "application/font-tdpfr"), MimeStatic(".pfunk", "audio/make"), MimeStatic(".pgm", "image/x-portable-greymap"), MimeStatic(".pic", "image/pict"), MimeStatic(".pict", "image/pict"), MimeStatic(".pkg", "application/x-newton-compatible-pkg"), MimeStatic(".pko", "application/vnd.ms-pki.pko"), MimeStatic(".pl", "text/x-script.perl"), MimeStatic(".plx", "application/x-pixelscript"), MimeStatic(".pm", "text/x-script.perl-module"), MimeStatic(".pm4", "application/x-pagemaker"), MimeStatic(".pm5", "application/x-pagemaker"), MimeStatic(".png", "image/png"), MimeStatic(".pnm", "image/x-portable-anymap"), MimeStatic(".pot", "application/vnd.ms-powerpoint"), MimeStatic(".pov", "model/x-pov"), MimeStatic(".ppa", "application/vnd.ms-powerpoint"), MimeStatic(".ppm", "image/x-portable-pixmap"), MimeStatic(".pps", "application/vnd.ms-powerpoint"), MimeStatic(".ppt", "application/vnd.ms-powerpoint"), MimeStatic(".ppz", "application/vnd.ms-powerpoint"), MimeStatic(".pre", "application/x-freelance"), MimeStatic(".prt", "application/pro_eng"), MimeStatic(".ps", "application/postscript"), MimeStatic(".psd", "application/octet-stream"), MimeStatic(".pvu", "paleovu/x-pv"), MimeStatic(".pwz", "application/vnd.ms-powerpoint"), MimeStatic(".py", "text/x-script.python"), MimeStatic(".pyc", "application/x-bytecode.python"), MimeStatic(".qcp", "audio/vnd.qcelp"), MimeStatic(".qd3", "x-world/x-3dmf"), MimeStatic(".qd3d", "x-world/x-3dmf"), MimeStatic(".qif", "image/x-quicktime"), MimeStatic(".qt", "video/quicktime"), MimeStatic(".qtc", "video/x-qtc"), MimeStatic(".qti", "image/x-quicktime"), MimeStatic(".qtif", "image/x-quicktime"), MimeStatic(".ra", "audio/x-pn-realaudio"), MimeStatic(".ram", "audio/x-pn-realaudio"), MimeStatic(".rar", "application/x-arj-compressed"), MimeStatic(".ras", "image/x-cmu-raster"), MimeStatic(".rast", "image/cmu-raster"), MimeStatic(".rexx", "text/x-script.rexx"), MimeStatic(".rf", "image/vnd.rn-realflash"), MimeStatic(".rgb", "image/x-rgb"), MimeStatic(".rm", "audio/x-pn-realaudio"), MimeStatic(".rmi", "audio/mid"), MimeStatic(".rmm", "audio/x-pn-realaudio"), MimeStatic(".rmp", "audio/x-pn-realaudio"), MimeStatic(".rng", "application/vnd.nokia.ringing-tone"), MimeStatic(".rnx", "application/vnd.rn-realplayer"), MimeStatic(".roff", "application/x-troff"), MimeStatic(".rp", "image/vnd.rn-realpix"), MimeStatic(".rpm", "audio/x-pn-realaudio-plugin"), MimeStatic(".rt", "text/vnd.rn-realtext"), MimeStatic(".rtf", "application/x-rtf"), MimeStatic(".rtx", "application/x-rtf"), MimeStatic(".rv", "video/vnd.rn-realvideo"), MimeStatic(".s", "text/x-asm"), MimeStatic(".s3m", "audio/s3m"), MimeStatic(".saveme", "application/octet-stream"), MimeStatic(".sbk", "application/x-tbook"), MimeStatic(".scm", "text/x-script.scheme"), MimeStatic(".sdml", "text/plain"), MimeStatic(".sdp", "application/x-sdp"), MimeStatic(".sdr", "application/sounder"), MimeStatic(".sea", "application/x-sea"), MimeStatic(".set", "application/set"), MimeStatic(".sgm", "text/x-sgml"), MimeStatic(".sgml", "text/x-sgml"), MimeStatic(".sh", "text/x-script.sh"), MimeStatic(".shar", "application/x-shar"), MimeStatic(".shtm", "text/html"), MimeStatic(".shtml", "text/html"), MimeStatic(".sid", "audio/x-psid"), MimeStatic(".sil", "application/font-sfnt"), MimeStatic(".sit", "application/x-sit"), MimeStatic(".skd", "application/x-koan"), MimeStatic(".skm", "application/x-koan"), MimeStatic(".skp", "application/x-koan"), MimeStatic(".skt", "application/x-koan"), MimeStatic(".sl", "application/x-seelogo"), MimeStatic(".smi", "application/smil"), MimeStatic(".smil", "application/smil"), MimeStatic(".snd", "audio/x-adpcm"), MimeStatic(".so", "application/octet-stream"), MimeStatic(".sol", "application/solids"), MimeStatic(".spc", "text/x-speech"), MimeStatic(".spl", "application/futuresplash"), MimeStatic(".spr", "application/x-sprite"), MimeStatic(".sprite", "application/x-sprite"), MimeStatic(".src", "application/x-wais-source"), MimeStatic(".ssi", "text/x-server-parsed-html"), MimeStatic(".ssm", "application/streamingmedia"), MimeStatic(".sst", "application/vnd.ms-pki.certstore"), MimeStatic(".step", "application/step"), MimeStatic(".stl", "application/vnd.ms-pki.stl"), MimeStatic(".stp", "application/step"), MimeStatic(".sv4cpio", "application/x-sv4cpio"), MimeStatic(".sv4crc", "application/x-sv4crc"), MimeStatic(".svf", "image/x-dwg"), MimeStatic(".svg", "image/svg+xml"), MimeStatic(".svr", "x-world/x-svr"), MimeStatic(".swf", "application/x-shockwave-flash"), MimeStatic(".t", "application/x-troff"), MimeStatic(".talk", "text/x-speech"), MimeStatic(".tar", "application/x-tar"), MimeStatic(".tbk", "application/x-tbook"), MimeStatic(".tcl", "text/x-script.tcl"), MimeStatic(".tcsh", "text/x-script.tcsh"), MimeStatic(".tex", "application/x-tex"), MimeStatic(".texi", "application/x-texinfo"), MimeStatic(".texinfo", "application/x-texinfo"), MimeStatic(".text", "text/plain"), MimeStatic(".tgz", "application/x-compressed"), MimeStatic(".tif", "image/x-tiff"), MimeStatic(".tiff", "image/x-tiff"), MimeStatic(".torrent", "application/x-bittorrent"), MimeStatic(".tr", "application/x-troff"), MimeStatic(".tsi", "audio/tsp-audio"), MimeStatic(".tsp", "audio/tsplayer"), MimeStatic(".tsv", "text/tab-separated-values"), MimeStatic(".ttf", "application/font-sfnt"), MimeStatic(".turbot", "image/florian"), MimeStatic(".txt", "text/plain"), MimeStatic(".uil", "text/x-uil"), MimeStatic(".uni", "text/uri-list"), MimeStatic(".unis", "text/uri-list"), MimeStatic(".unv", "application/i-deas"), MimeStatic(".uri", "text/uri-list"), MimeStatic(".uris", "text/uri-list"), MimeStatic(".ustar", "application/x-ustar"), MimeStatic(".uu", "text/x-uuencode"), MimeStatic(".uue", "text/x-uuencode"), MimeStatic(".vcd", "application/x-cdlink"), MimeStatic(".vcs", "text/x-vcalendar"), MimeStatic(".vda", "application/vda"), MimeStatic(".vdo", "video/vdo"), MimeStatic(".vew", "application/groupwise"), MimeStatic(".viv", "video/vnd.vivo"), MimeStatic(".vivo", "video/vnd.vivo"), MimeStatic(".vmd", "application/vocaltec-media-desc"), MimeStatic(".vmf", "application/vocaltec-media-resource"), MimeStatic(".voc", "audio/x-voc"), MimeStatic(".vos", "video/vosaic"), MimeStatic(".vox", "audio/voxware"), MimeStatic(".vqe", "audio/x-twinvq-plugin"), MimeStatic(".vqf", "audio/x-twinvq"), MimeStatic(".vql", "audio/x-twinvq-plugin"), MimeStatic(".vrml", "model/vrml"), MimeStatic(".vrt", "x-world/x-vrt"), MimeStatic(".vsd", "application/x-visio"), MimeStatic(".vst", "application/x-visio"), MimeStatic(".vsw", "application/x-visio"), MimeStatic(".w60", "application/wordperfect6.0"), MimeStatic(".w61", "application/wordperfect6.1"), MimeStatic(".w6w", "application/msword"), MimeStatic(".wav", "audio/x-wav"), MimeStatic(".wb1", "application/x-qpro"), MimeStatic(".wbmp", "image/vnd.wap.wbmp"), MimeStatic(".web", "application/vnd.xara"), MimeStatic(".webm", "video/webm"), MimeStatic(".wiz", "application/msword"), MimeStatic(".wk1", "application/x-123"), MimeStatic(".wmf", "windows/metafile"), MimeStatic(".wml", "text/vnd.wap.wml"), MimeStatic(".wmlc", "application/vnd.wap.wmlc"), MimeStatic(".wmls", "text/vnd.wap.wmlscript"), MimeStatic(".wmlsc", "application/vnd.wap.wmlscriptc"), MimeStatic(".woff", "application/font-woff"), MimeStatic(".word", "application/msword"), MimeStatic(".wp", "application/wordperfect"), MimeStatic(".wp5", "application/wordperfect"), MimeStatic(".wp6", "application/wordperfect"), MimeStatic(".wpd", "application/wordperfect"), MimeStatic(".wq1", "application/x-lotus"), MimeStatic(".wri", "application/x-wri"), MimeStatic(".wrl", "model/vrml"), MimeStatic(".wrz", "model/vrml"), MimeStatic(".wsc", "text/scriplet"), MimeStatic(".wsrc", "application/x-wais-source"), MimeStatic(".wtk", "application/x-wintalk"), MimeStatic(".x-png", "image/png"), MimeStatic(".xbm", "image/x-xbm"), MimeStatic(".xdr", "video/x-amt-demorun"), MimeStatic(".xgz", "xgl/drawing"), MimeStatic(".xhtml", "application/xhtml+xml"), MimeStatic(".xif", "image/vnd.xiff"), MimeStatic(".xl", "application/vnd.ms-excel"), MimeStatic(".xla", "application/vnd.ms-excel"), MimeStatic(".xlb", "application/vnd.ms-excel"), MimeStatic(".xlc", "application/vnd.ms-excel"), MimeStatic(".xld", "application/vnd.ms-excel"), MimeStatic(".xlk", "application/vnd.ms-excel"), MimeStatic(".xll", "application/vnd.ms-excel"), MimeStatic(".xlm", "application/vnd.ms-excel"), MimeStatic(".xls", "application/vnd.ms-excel"), MimeStatic(".xlt", "application/vnd.ms-excel"), MimeStatic(".xlv", "application/vnd.ms-excel"), MimeStatic(".xlw", "application/vnd.ms-excel"), MimeStatic(".xm", "audio/xm"), MimeStatic(".xml", "text/xml"), MimeStatic(".xmz", "xgl/movie"), MimeStatic(".xpix", "application/x-vnd.ls-xpix"), MimeStatic(".xpm", "image/x-xpixmap"), MimeStatic(".xsl", "application/xml"), MimeStatic(".xslt", "application/xml"), MimeStatic(".xsr", "video/x-amt-showrun"), MimeStatic(".xwd", "image/x-xwd"), MimeStatic(".xyz", "chemical/x-pdb"), MimeStatic(".z", "application/x-compressed"), MimeStatic(".zip", "application/x-zip-compressed"), MimeStatic(".zoo", "application/octet-stream"), MimeStatic(".zsh", "text/x-script.zsh") };

				size_t PathLength = Path.size();
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
					if ((Result = Core::Stringify::CaseCompare(Ext, MimeTypes[Index].Extension)) == 0)
						return MimeTypes[Index].Type;

					if (Result < 0)
						End = Index;
					else
						Start = Index;
				}

				if (!Core::Stringify::CaseCompare(Ext, MimeTypes[Start].Extension))
					return MimeTypes[Start].Type;

				if (!Types->empty())
				{
					for (auto& Item : *Types)
					{
						if (!Core::Stringify::CaseCompare(Ext, Item.Extension.c_str()))
							return Item.Type.c_str();
					}
				}

				return "application/octet-stream";
			}
			const char* Utils::StatusMessage(int StatusCode)
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
						return "Request URI Too Large";
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

				return "Stateless";
			}

			void Paths::ConstructPath(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				if (!Base->Route->Override.empty())
				{
					Base->Request.Path.assign(Base->Route->Override);
					if (Base->Route->Site->Callbacks.OnRewriteURL)
						Base->Route->Site->Callbacks.OnRewriteURL(Base);
					return;
				}

				for (size_t i = 0; i < Base->Request.URI.size(); i++)
				{
					if (Base->Request.URI[i] == '%' && i + 1 < Base->Request.URI.size())
					{
						if (Base->Request.URI[i + 1] == 'u')
						{
							int Value = 0;
							if (Compute::Codec::HexToDecimal(Base->Request.URI, i + 2, 4, Value))
							{
								char Buffer[4];
								size_t LCount = Compute::Codec::Utf8(Value, Buffer);
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
							if (Compute::Codec::HexToDecimal(Base->Request.URI, i + 1, 2, Value))
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
					size_t Start = std::min<size_t>(Base->Request.Path.size(), (size_t)Match.Start);
					size_t End = std::min<size_t>(Base->Request.Path.size(), (size_t)Match.End);
					Core::Stringify::RemovePart(Base->Request.Path, Start, End);
				}
				
				if (!Base->Route->DocumentRoot.empty())
				{
					Base->Request.Path = Base->Route->DocumentRoot + Base->Request.Path;
					auto Path = Core::OS::Path::Resolve(Base->Request.Path.c_str());
					if (Path)
						Base->Request.Path = *Path;
				}

				if (Core::Stringify::EndsOf(Base->Request.Path, "/\\"))
				{
					if (!Core::Stringify::EndsOf(Base->Request.URI, "/\\"))
						Base->Request.Path.erase(Base->Request.Path.size() - 1, 1);
				}
				else if (Core::Stringify::EndsOf(Base->Request.URI, "/\\"))
					Base->Request.Path.append(1, '/');

				if (Base->Route->Site->Callbacks.OnRewriteURL)
					Base->Route->Site->Callbacks.OnRewriteURL(Base);
			}
			void Paths::ConstructHeadFull(RequestFrame* Request, ResponseFrame* Response, bool IsRequest, Core::String& Buffer)
			{
				VI_ASSERT(Request != nullptr, "connection should be set");
				VI_ASSERT(Response != nullptr, "response should be set");

				HeaderMapping& Headers = (IsRequest ? Request->Headers : Response->Headers);
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
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				if (!Base->Route->StaticFileMaxAge)
					return ConstructHeadUncache(Base, Buffer);

				Buffer.append("Cache-Control: max-age=");
				Buffer.append(Core::ToString(Base->Route->StaticFileMaxAge));
				Buffer.append("\r\n");
			}
			void Paths::ConstructHeadUncache(Connection* Base, Core::String& Buffer)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				Buffer.append(
					"Cache-Control: no-cache, no-store, must-revalidate, private, max-age=0\r\n"
					"Pragma: no-cache\r\n"
					"Expires: 0\r\n", 102);
			}
			bool Paths::ConstructRoute(MapRouter* Router, Connection* Base)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				VI_ASSERT(Router != nullptr, "router should be set");

				if (Router->Sites.empty())
					return false;

				auto* Host = Base->Request.GetHeaderBlob("Host");
				if (!Host)
					return false;

				if (Base->Request.URI.empty() || Base->Request.URI.front() != '/')
					return false;

				Core::UnorderedMap<Core::String, SiteEntry*>::iterator It;
				if (Router->Listeners.size() > 1)
				{
					auto Listen = Router->Listeners.find(*Host);
					if (Listen == Router->Listeners.end())
					{
						Listen = Router->Listeners.find("*");
						if (Listen == Router->Listeners.end())
							return false;
					}

					It = Router->Sites.find(Listen->first);
					if (It == Router->Sites.end())
						return false;
				}
				else
				{
					auto Listen = Router->Listeners.begin();
					if (Listen->first != "*" && Listen->first != *Host)
						return false;

					It = Router->Sites.begin();
				}

				Base->Request.Where = Base->Request.URI;
				for (auto& Group : It->second->Groups)
				{
					if (!Group->Match.empty())
					{
						Core::String& URI = Base->Request.URI;
						if (Group->Mode == RouteMode::Start)
						{
							if (!Core::Stringify::StartsWith(URI, Group->Match))
								continue;

							URI = URI.substr(Group->Match.size(), URI.size());
						}
						else if (Group->Mode == RouteMode::Match)
						{
							if (!Core::Stringify::Find(URI, Group->Match).Found)
								continue;
						}
						else if (Group->Mode == RouteMode::End)
						{
							if (!Core::Stringify::EndsWith(URI, Group->Match))
								continue;

							Core::Stringify::Clip(URI, URI.size() - Group->Match.size());
						}

						if (URI.empty())
							URI.append(1, '/');

						for (auto* Basis : Group->Routes)
						{
							if (Compute::Regex::Match(&Basis->URI, Base->Request.Match, Base->Request.URI))
							{
								Base->Route = Basis;
								return true;
							}
						}

						URI.assign(Base->Request.Where);
					}
					else
					{
						for (auto* Basis : Group->Routes)
						{
							if (Compute::Regex::Match(&Basis->URI, Base->Request.Match, Base->Request.URI))
							{
								Base->Route = Basis;
								return true;
							}
						}
					}
				}

				Base->Route = It->second->Base;
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
				Field += Core::ToString(Offset + Length - 1);
				Field += '/';
				Field += Core::ToString(ContentLength);

				return Field;
			}

			bool Parsing::ParseMultipartHeaderField(Parser* Parser, const char* Name, size_t Length)
			{
				return ParseHeaderField(Parser, Name, Length);
			}
			bool Parsing::ParseMultipartHeaderValue(Parser* Parser, const char* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");

				if (!Length || Parser->Frame.Ignore)
					return true;

				if (Parser->Frame.Header.empty())
					return true;

				Core::String Value(Data, Length);
				if (Parser->Frame.Header == "Content-Disposition")
				{
					Core::TextSettle Start = Core::Stringify::Find(Value, "name=\"");
					if (Start.Found)
					{
						Core::TextSettle End = Core::Stringify::Find(Value, '\"', Start.End);
						if (End.Found)
							Parser->Frame.Source.Key = Value.substr(Start.End, End.End - Start.End - 1);
					}

					Start = Core::Stringify::Find(Value, "filename=\"");
					if (Start.Found)
					{
						Core::TextSettle End = Core::Stringify::Find(Value, '\"', Start.End);
						if (End.Found)
						{
							auto Name = Value.substr(Start.End, End.End - Start.End - 1);
							Parser->Frame.Source.Name = Core::OS::Path::GetFilename(Name.c_str());
						}
					}
				}
				else if (Parser->Frame.Header == "Content-Type")
					Parser->Frame.Source.Type = Value;

				Parser->Frame.Source.SetHeader(Parser->Frame.Header.c_str(), Value);
				Parser->Frame.Header.clear();

				return true;
			}
			bool Parsing::ParseMultipartContentData(Parser* Parser, const char* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");

				if (!Length)
					return true;

				if (Parser->Frame.Ignore || !Parser->Frame.Stream)
					return false;

				if (fwrite(Data, 1, (size_t)Length, Parser->Frame.Stream) != (size_t)Length)
					return false;

				Parser->Frame.Source.Length += Length;
				return true;
			}
			bool Parsing::ParseMultipartResourceBegin(Parser* Parser)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				if (Parser->Frame.Ignore || !Parser->Frame.Request)
					return true;

				if (Parser->Frame.Stream != nullptr)
				{
					Core::OS::File::Close(Parser->Frame.Stream);
					Parser->Frame.Stream = nullptr;
					return false;
				}

				if (Parser->Frame.Route && Parser->Frame.Request->Content.Resources.size() >= Parser->Frame.Route->Site->MaxResources)
				{
					Parser->Frame.Close = true;
					return false;
				}

				Parser->Frame.Header.clear();
				Parser->Frame.Source.Headers.clear();
				Parser->Frame.Source.Name.clear();
				Parser->Frame.Source.Type = "application/octet-stream";
				Parser->Frame.Source.IsInMemory = false;
				Parser->Frame.Source.Length = 0;

				if (Parser->Frame.Route)
				{
					Parser->Frame.Source.Path = Parser->Frame.Route->Site->ResourceRoot;
					if (Parser->Frame.Source.Path.back() != '/' && Parser->Frame.Source.Path.back() != '\\')
						Parser->Frame.Source.Path.append(1, '/');

					auto Random = Compute::Crypto::RandomBytes(16);
					if (Random)
					{
						auto Hash = Compute::Crypto::Hash(Compute::Digests::MD5(), *Random);
						if (Hash)
							Parser->Frame.Source.Path.append(*Hash);
					}
				}

				auto File = Core::OS::File::Open(Parser->Frame.Source.Path.c_str(), "wb");
				if (!File)
					return false;

				Parser->Frame.Stream = *File;
				return true;
			}
			bool Parsing::ParseMultipartResourceEnd(Parser* Parser)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				if (Parser->Frame.Ignore || !Parser->Frame.Stream || !Parser->Frame.Request)
					return true;

				Core::OS::File::Close(Parser->Frame.Stream);
				Parser->Frame.Stream = nullptr;
				Parser->Frame.Request->Content.Resources.push_back(Parser->Frame.Source);

				if (Parser->Frame.Callback)
					Parser->Frame.Callback(&Parser->Frame.Request->Content.Resources.back());

				return true;
			}
			bool Parsing::ParseHeaderField(Parser* Parser, const char* Name, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Name != nullptr, "name should be set");

				if (!Length || Parser->Frame.Ignore)
					return true;

				Parser->Frame.Header.assign(Name, Length);
				return true;
			}
			bool Parsing::ParseHeaderValue(Parser* Parser, const char* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");

				if (!Length || Parser->Frame.Ignore || Parser->Frame.Header.empty())
					return true;

				if (Core::Stringify::CaseCompare(Parser->Frame.Header.c_str(), "cookie") == 0)
				{
					if (!Parser->Frame.Request)
						goto Success;

					Core::Vector<std::pair<Core::String, Core::String>> Cookies;
					const char* Offset = Data;

					for (size_t i = 0; i < Length; i++)
					{
						if (Data[i] != '=')
							continue;

						Core::String Name(Offset, (size_t)((Data + i) - Offset));
						size_t Set = i;

						while (i + 1 < Length && Data[i] != ';')
							i++;

						if (Data[i] == ';')
							i--;

						Cookies.emplace_back(std::make_pair(std::move(Name), Core::String(Data + Set + 1, i - Set)));
						Offset = Data + (i + 3);
					}

					for (auto&& Item : Cookies)
					{
						auto& Cookie = Parser->Frame.Request->Cookies[Item.first];
						Cookie.emplace_back(std::move(Item.second));
					}
				}
				else if (Parser->Frame.Request != nullptr || Parser->Frame.Response != nullptr)
				{
					Core::Vector<Core::String> Keys;
					if (Core::Stringify::CaseCompare(Parser->Frame.Header.c_str(), "user-agent") != 0)
					{
						Keys = Core::Stringify::Split(Core::String(Data, Length), ',');
						for (auto& Item : Keys)
							Core::Stringify::Trim(Item);
					}
					else
						Keys.emplace_back(Data, Length);

					if (Parser->Frame.Request)
					{
						auto& Source = Parser->Frame.Request->Headers[Parser->Frame.Header];
						for (auto& Item : Keys)
							Source.push_back(Item);
					}

					if (Parser->Frame.Response)
					{
						auto& Source = Parser->Frame.Response->Headers[Parser->Frame.Header];
						for (auto& Item : Keys)
							Source.push_back(Item);
					}
				}

			Success:
				Parser->Frame.Header.clear();
				return true;
			}
			bool Parsing::ParseVersion(Parser* Parser, const char* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");

				if (!Length || Parser->Frame.Ignore || !Parser->Frame.Request)
					return true;

				memset(Parser->Frame.Request->Version, 0, sizeof(Parser->Frame.Request->Version));
				memcpy((void*)Parser->Frame.Request->Version, (void*)Data, std::min<size_t>(Length, sizeof(Parser->Frame.Request->Version)));
				return true;
			}
			bool Parsing::ParseStatusCode(Parser* Parser, size_t Value)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				if (Parser->Frame.Ignore || !Parser->Frame.Response)
					return true;

				Parser->Frame.Response->StatusCode = (int)Value;
				return true;
			}
			bool Parsing::ParseMethodValue(Parser* Parser, const char* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");

				if (!Length || Parser->Frame.Ignore || !Parser->Frame.Request)
					return true;

				memset(Parser->Frame.Request->Method, 0, sizeof(Parser->Frame.Request->Method));
				memcpy((void*)Parser->Frame.Request->Method, (void*)Data, std::min<size_t>(Length, sizeof(Parser->Frame.Request->Method)));
				return true;
			}
			bool Parsing::ParsePathValue(Parser* Parser, const char* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");

				if (!Length || Parser->Frame.Ignore || !Parser->Frame.Request)
					return true;

				Parser->Frame.Request->URI.assign(Data, Length);
				return true;
			}
			bool Parsing::ParseQueryValue(Parser* Parser, const char* Data, size_t Length)
			{
				VI_ASSERT(Parser != nullptr, "parser should be set");
				VI_ASSERT(Data != nullptr, "data should be set");

				if (!Length || Parser->Frame.Ignore || !Parser->Frame.Request)
					return true;

				Parser->Frame.Request->Query.assign(Data, Length);
				return true;
			}
			int Parsing::ParseContentRange(const char* ContentRange, int64_t* Range1, int64_t* Range2)
			{
				VI_ASSERT(ContentRange != nullptr, "content range should be set");
				VI_ASSERT(Range1 != nullptr, "range 1 should be set");
				VI_ASSERT(Range2 != nullptr, "range 2 should be set");

				return sscanf(ContentRange, "bytes=%" PRId64 "-%" PRId64, Range1, Range2);
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

			Core::String Permissions::Authorize(const Core::String& Username, const Core::String& Password, const Core::String& Type)
			{
				return Type + ' ' + Compute::Codec::Base64Encode(Username + ':' + Password);
			}
			bool Permissions::Authorize(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				if (!Base->Route->Callbacks.Authorize || Base->Route->Auth.Type.empty())
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

				size_t Index = 0;
				while (Authorization[Index] != ' ' && Authorization[Index] != '\0')
					Index++;

				Core::String Type(Authorization, Index);
				if (Type != Base->Route->Auth.Type)
				{
					Base->Request.User.Type = Auth::Denied;
					Base->Error(401, "Authorization type \"%s\" is not allowed.", Type.c_str());
					return false;
				}

				Base->Request.User.Token = Authorization + Index + 1;
				if (Base->Route->Callbacks.Authorize(Base, &Base->Request.User))
				{
					Base->Request.User.Type = Auth::Granted;
					return true;
				}

				Base->Request.User.Type = Auth::Denied;
				Base->Error(401, "Invalid user access credentials were provided. Access denied.");
				return false;
			}
			bool Permissions::MethodAllowed(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
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
				const char* Upgrade = Base->Request.GetHeader("Upgrade");
				if (!Upgrade)
					return false;

				if (Core::Stringify::CaseCompare(Upgrade, "websocket") != 0)
					return false;

				const char* Connection = Base->Request.GetHeader("Connection");
				if (!Connection)
					return false;

				if (Core::Stringify::CaseCompare(Connection, "upgrade") != 0)
					return false;

				return true;
			}

			bool Resources::ResourceHasAlternative(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
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
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				if (Base->Route->HiddenFiles.empty())
					return false;

				const Core::String& Value = (Path ? *Path : Base->Request.Path);
				Compute::RegexResult Result;

				for (auto& Item : Base->Route->HiddenFiles)
				{
					if (Compute::Regex::Match(&Item, Result, Value))
						return true;
				}

				return false;
			}
			bool Resources::ResourceIndexed(Connection* Base, Core::FileEntry* Resource)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				VI_ASSERT(Resource != nullptr, "resource should be set");

				if (Base->Route->IndexFiles.empty())
					return false;

				Core::String Path = Base->Request.Path;
				if (!Core::Stringify::EndsOf(Path, "/\\"))
				{
#ifdef VI_MICROSOFT
					Path.append(1, '\\');
#else
					Path.append(1, '/');
#endif
				}

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
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				VI_ASSERT(Resource != nullptr, "resource should be set");

				const char* CacheControl = Base->Request.GetHeader("Cache-Control");
				if (CacheControl != nullptr && (!Core::Stringify::CaseCompare("no-cache", CacheControl) || !Core::Stringify::CaseCompare("max-age=0", CacheControl)))
					return true;

				const char* IfNoneMatch = Base->Request.GetHeader("If-None-Match");
				if (IfNoneMatch != nullptr)
				{
					char ETag[64];
					Core::OS::Net::GetETag(ETag, sizeof(ETag), Resource);
					if (!Core::Stringify::CaseCompare(ETag, IfNoneMatch))
						return false;
				}

				const char* IfModifiedSince = Base->Request.GetHeader("If-Modified-Since");
				return !(IfModifiedSince != nullptr && Resource->LastModified <= Core::DateTime::ParseWebDate(IfModifiedSince));

			}
			bool Resources::ResourceCompressed(Connection* Base, size_t Size)
			{
#ifdef VI_ZLIB
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
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

			bool Routing::RouteWEBSOCKET(Connection* Base)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				if (!Base->Route->AllowWebSocket)
					return Base->Error(404, "Websocket protocol is not allowed on this server.");

				const char* WebSocketKey = Base->Request.GetHeader("Sec-WebSocket-Key");
				if (WebSocketKey != nullptr)
					return Logical::ProcessWebSocket(Base, WebSocketKey);

				const char* WebSocketKey1 = Base->Request.GetHeader("Sec-WebSocket-Key1");
				if (!WebSocketKey1)
					return Base->Error(400, "Malformed websocket request. Provide first key.");

				const char* WebSocketKey2 = Base->Request.GetHeader("Sec-WebSocket-Key2");
				if (!WebSocketKey2)
					return Base->Error(400, "Malformed websocket request. Provide second key.");

				return !!Base->Stream->ReadAsync(8, [Base](SocketPoll Event, const char* Buffer, size_t Recv)
				{
					if (Packet::IsData(Event))
						Base->Request.Content.Append(Buffer, Recv);
					else if (Packet::IsDone(Event))
						Logical::ProcessWebSocket(Base, Base->Request.Content.Data.data());
					else if (Packet::IsError(Event))
						Base->Break();

					return true;
				});
			}
			bool Routing::RouteGET(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				if (Base->Route->DocumentRoot.empty() || !Core::OS::File::GetState(Base->Request.Path, &Base->Resource))
				{
					if (Permissions::WebSocketUpgradeAllowed(Base))
						return Core::Schedule::Get()->SetTask([Base]() { RouteWEBSOCKET(Base); });

					if (!Resources::ResourceHasAlternative(Base))
						return Base->Error(404, "Requested resource was not found.");
				}

				if (Permissions::WebSocketUpgradeAllowed(Base))
					return Core::Schedule::Get()->SetTask([Base]() { RouteWEBSOCKET(Base); });

				if (Resources::ResourceHidden(Base, nullptr))
					return Base->Error(404, "Requested resource was not found.");

				if (Base->Resource.IsDirectory && !Resources::ResourceIndexed(Base, &Base->Resource))
				{
					if (Base->Route->AllowDirectoryListing)
						return Core::Schedule::Get()->SetTask([Base]() { Logical::ProcessDirectory(Base); });

					return Base->Error(403, "Directory listing denied.");
				}

				if (Base->Route->StaticFileMaxAge > 0 && !Resources::ResourceModified(Base, &Base->Resource))
					return Core::Schedule::Get()->SetTask([Base]() { Logical::ProcessResourceCache(Base); });

				return Core::Schedule::Get()->SetTask([Base]() { Logical::ProcessResource(Base); });
			}
			bool Routing::RoutePOST(Connection* Base)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				if (Base->Route->DocumentRoot.empty() || Resources::ResourceHidden(Base, nullptr))
					return Base->Error(404, "Requested resource was not found.");

				if (!Core::OS::File::GetState(Base->Request.Path, &Base->Resource))
					return Base->Error(404, "Requested resource was not found.");

				if (Base->Resource.IsDirectory && !Resources::ResourceIndexed(Base, &Base->Resource))
					return Base->Error(404, "Requested resource was not found.");

				if (Base->Route->StaticFileMaxAge > 0 && !Resources::ResourceModified(Base, &Base->Resource))
					return Core::Schedule::Get()->SetTask([Base]() { Logical::ProcessResourceCache(Base); });

				return Core::Schedule::Get()->SetTask([Base]() { Logical::ProcessResource(Base); });
			}
			bool Routing::RoutePUT(Connection* Base)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				if (Base->Route->DocumentRoot.empty() || Resources::ResourceHidden(Base, nullptr))
					return Base->Error(403, "Resource overwrite denied.");

				if (!Core::OS::File::GetState(Base->Request.Path, &Base->Resource))
					return Base->Error(403, "Directory overwrite denied.");

				if (!Base->Resource.IsDirectory)
					return Base->Error(403, "Directory overwrite denied.");

				const char* Range = Base->Request.GetHeader("Range");
				int64_t Range1 = 0, Range2 = 0;

				auto File = Core::OS::File::Open(Base->Request.Path.c_str(), "wb");
				if (!File)
					return Base->Error(422, "Resource stream cannot be opened.");

				FILE* Stream = *File;
				if (Range != nullptr && HTTP::Parsing::ParseContentRange(Range, &Range1, &Range2))
				{
					if (Base->Response.StatusCode <= 0)
						Base->Response.StatusCode = 206;

					if (!Core::OS::File::Seek64(Stream, Range1, Core::FileSeek::Begin))
						return Base->Error(416, "Invalid content range offset (%" PRId64 ") was specified.", Range1);
				}
				else
					Base->Response.StatusCode = 204;

				return Base->Consume([=](Connection* Base, SocketPoll Event, const char* Buffer, size_t Size)
				{
					if (Packet::IsData(Event))
					{
						fwrite(Buffer, sizeof(char) * (size_t)Size, 1, Stream);
						return true;
					}
					else if (Packet::IsDone(Event))
					{
						char Date[64];
						Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

						Core::String Content;
						Content.append(Base->Request.Version);
						Content.append(" 204 No Content\r\nDate: ").append(Date).append("\r\n");
						Content.append(Utils::ConnectionResolve(Base));
						Content.append("Content-Location: ").append(Base->Request.URI).append("\r\n");

						Core::OS::File::Close(Stream);
						if (Base->Route->Callbacks.Headers)
							Base->Route->Callbacks.Headers(Base, Content);

						Content.append("\r\n", 2);
						return !Base->Stream->WriteAsync(Content.c_str(), Content.size(), [Base](SocketPoll Event)
						{
							if (Packet::IsDone(Event))
								Base->Finish();
							else if (Packet::IsError(Event))
								Base->Break();
						});
					}
					else if (Packet::IsError(Event))
					{
						Core::OS::File::Close(Stream);
						return Base->Break();
					}
					else if (Packet::IsSkip(Event))
						Core::OS::File::Close(Stream);

					return true;
				});
			}
			bool Routing::RoutePATCH(Connection* Base)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				if (Base->Route->DocumentRoot.empty() || Resources::ResourceHidden(Base, nullptr))
					return Base->Error(404, "Requested resource was not found.");

				if (!Core::OS::File::GetState(Base->Request.Path, &Base->Resource))
					return Base->Error(404, "Requested resource was not found.");

				if (Base->Resource.IsDirectory && !Resources::ResourceIndexed(Base, &Base->Resource))
					return Base->Error(404, "Requested resource cannot be directory.");

				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				Core::String Content;
				Content.append(Base->Request.Version);
				Content.append(" 204 No Content\r\nDate: ").append(Date).append("\r\n");
				Content.append(Utils::ConnectionResolve(Base));
				Content.append("Content-Location: ").append(Base->Request.URI).append("\r\n");

				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, Content);

				Content.append("\r\n", 2);
				return !!Base->Stream->WriteAsync(Content.c_str(), Content.size(), [Base](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
						Base->Finish(204);
					else if (Packet::IsError(Event))
						Base->Break();
				});
			}
			bool Routing::RouteDELETE(Connection* Base)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				if (Base->Route->DocumentRoot.empty() || Resources::ResourceHidden(Base, nullptr))
					return Base->Error(404, "Requested resource was not found.");

				if (!Core::OS::File::GetState(Base->Request.Path, &Base->Resource))
					return Base->Error(404, "Requested resource was not found.");

				if (!Base->Resource.IsDirectory)
				{
					if (!Core::OS::File::Remove(Base->Request.Path.c_str()))
						return Base->Error(403, "Operation denied by system.");
				}
				else if (!Core::OS::Directory::Remove(Base->Request.Path.c_str()))
					return Base->Error(403, "Operation denied by system.");

				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				Core::String Content;
				Content.append(Base->Request.Version);
				Content.append(" 204 No Content\r\nDate: ").append(Date).append("\r\n");
				Content.append(Utils::ConnectionResolve(Base));

				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, Content);

				Content.append("\r\n", 2);
				return !!Base->Stream->WriteAsync(Content.c_str(), Content.size(), [Base](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
						Base->Finish(204);
					else if (Packet::IsError(Event))
						Base->Break();
				});
			}
			bool Routing::RouteOPTIONS(Connection* Base)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				Core::String Content;
				Content.append(Base->Request.Version);
				Content.append(" 204 No Content\r\nDate: ").append(Date).append("\r\n");
				Content.append(Utils::ConnectionResolve(Base));
				Content.append("Allow: GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD\r\n");

				if (Base->Route && Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, Content);

				Content.append("\r\n", 2);
				return !!Base->Stream->WriteAsync(Content.c_str(), Content.size(), [Base](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
						Base->Finish(204);
					else if (Packet::IsError(Event))
						Base->Break();
				});
			}

			bool Logical::ProcessDirectory(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				Core::Vector<std::pair<Core::String, Core::FileEntry>> Entries;
				if (!Core::OS::Directory::Scan(Base->Request.Path, &Entries))
					return Base->Error(500, "System denied to directory listing.");

				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				Core::String Content;
				Content.append(Base->Request.Version);
				Content.append(" 200 OK\r\nDate: ").append(Date).append("\r\n");
				Content.append(Utils::ConnectionResolve(Base));
				Content.append("Content-Type: text/html; charset=").append(Base->Route->CharSet);
				Content.append("\r\nAccept-Ranges: bytes\r\n");

				Paths::ConstructHeadCache(Base, Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, Content);

				const char* Message = Base->Response.GetHeader("X-Error");
				if (Message != nullptr)
					Content.append("X-Error: ").append(Message).append("\r\n");

				size_t Size = Base->Request.URI.size() - 1;
				while (Base->Request.URI[Size] != '/')
					Size--;

				char Direction = (!Base->Request.Query.empty() && Base->Request.Query[1] == 'd') ? 'a' : 'd';
				Core::String Name = Compute::Codec::URIDecode(Base->Request.URI);
				Core::String Parent(1, '/');
				if (Base->Request.URI.size() > 1)
				{
					Parent = Base->Request.URI.substr(0, Base->Request.URI.size() - 1);
					Parent = Core::OS::Path::GetDirectory(Parent.c_str());
				}

				Base->Response.Content.Assign(
					"<html><head><title>Index of " + Name + "</title>"
					"<style>" CSS_DIRECTORY_STYLE "</style></head>"
					"<body><h1>Index of " + Name + "</h1><pre><table cellpadding=\"0\">"
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

					Core::String URI = Compute::Codec::URIEncode(Item.first);
					Core::String HREF = (Base->Request.URI + ((*(Base->Request.URI.c_str() + 1) != '\0' && Base->Request.URI[Base->Request.URI.size() - 1] != '/') ? "/" : "") + URI);
					if (Item.second.IsDirectory && !Core::Stringify::EndsOf(HREF, "/\\"))
						HREF.append(1, '/');

					Base->Response.Content.Append("<tr><td><a href=\"" + HREF + "\">" + Item.first + "</a></td><td>&nbsp;" + dDate + "</td><td>&nbsp;&nbsp;" + dSize + "</td></tr>\n");
				}
				Base->Response.Content.Append("</table></pre></body></html>");

#ifdef VI_ZLIB
				bool Deflate = false, Gzip = false;
				if (Resources::ResourceCompressed(Base, Base->Response.Content.Data.size()))
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
								Base->Response.Content.Assign(Buffer.c_str(), (size_t)Stream.total_out);
								if (!Base->Response.GetHeader("Content-Encoding"))
								{
									if (Gzip)
										Content.append("Content-Encoding: gzip\r\n", 24);
									else
										Content.append("Content-Encoding: deflate\r\n", 27);
								}
							}
						}
					}
				}
#endif
				Content.append("Content-Length: ").append(Core::ToString(Base->Response.Content.Data.size())).append("\r\n\r\n");
				return !!Base->Stream->WriteAsync(Content.c_str(), Content.size(), [Base](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
					{
						if (memcmp(Base->Request.Method, "HEAD", 4) == 0)
							return (void)Base->Finish(200);

						Base->Stream->WriteAsync(Base->Response.Content.Data.data(), Base->Response.Content.Data.size(), [Base](SocketPoll Event)
						{
							if (Packet::IsDone(Event))
								Base->Finish(200);
							else if (Packet::IsError(Event))
								Base->Break();
						});
					}
					else if (Packet::IsError(Event))
						Base->Break();
				});
			}
			bool Logical::ProcessResource(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				const char* ContentType = Utils::ContentType(Base->Request.Path, &Base->Route->MimeTypes);
				const char* Range = Base->Request.GetHeader("Range");
				const char* StatusMessage = Utils::StatusMessage(Base->Response.StatusCode = (Base->Response.Error && Base->Response.StatusCode > 0 ? Base->Response.StatusCode : 200));
				int64_t Range1 = 0, Range2 = 0, Count = 0;
				int64_t ContentLength = (int64_t)Base->Resource.Size;

				char ContentRange[128] = { };
				if (Range != nullptr && (Count = Parsing::ParseContentRange(Range, &Range1, &Range2)) > 0 && Range1 >= 0 && Range2 >= 0)
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
					const char* AcceptEncoding = Base->Request.GetHeader("Accept-Encoding");
					if (AcceptEncoding != nullptr)
					{
						bool Deflate = strstr(AcceptEncoding, "deflate") != nullptr;
						bool Gzip = strstr(AcceptEncoding, "gzip") != nullptr;

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

				Core::String Content;
				Content.append(Base->Request.Version).append(" ");
				Content.append(Core::ToString(Base->Response.StatusCode)).append(" ");
				Content.append(StatusMessage).append("\r\n");
				Content.append("Date: ").append(Date).append("\r\n");
				Content.append(Utils::ConnectionResolve(Base));

				const char* Origin = Base->Request.GetHeader("Origin");
				if (Origin != nullptr)
					Content.append("Access-Control-Allow-Origin: ").append(Base->Route->AccessControlAllowOrigin).append("\r\n");

				Paths::ConstructHeadCache(Base, Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, Content);

				const char* Message = Base->Response.GetHeader("X-Error");
				if (Message != nullptr)
					Content.append("X-Error: ").append(Message).append("\r\n");

				Content.append("Accept-Ranges: bytes\r\nLast-Modified: ").append(LastModified).append("\r\n");
				Content.append("Etag: ").append(ETag).append("\r\n");
				Content.append("Content-Type: ").append(ContentType).append("; charset=").append(Base->Route->CharSet).append("\r\n");
				Content.append("Content-Length: ").append(Core::ToString(ContentLength)).append("\r\n");
				Content.append(ContentRange).append("\r\n");

				if (!ContentLength || !strcmp(Base->Request.Method, "HEAD"))
				{
					return !!Base->Stream->WriteAsync(Content.c_str(), Content.size(), [Base](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
							Base->Finish(200);
						else if (Packet::IsError(Event))
							Base->Break();
					});
				}

				return !!Base->Stream->WriteAsync(Content.c_str(), Content.size(), [Base, ContentLength, Range1](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
						Core::Schedule::Get()->SetTask([Base, ContentLength, Range1]() { Logical::ProcessFile(Base, (size_t)ContentLength, (size_t)Range1); });
					else if (Packet::IsError(Event))
						Base->Break();
				});
			}
			bool Logical::ProcessResourceCompress(Connection* Base, bool Deflate, bool Gzip, const char* ContentRange, size_t Range)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				VI_ASSERT(ContentRange != nullptr, "content tange should be set");
				VI_ASSERT(Deflate || Gzip, "uncompressable resource");

				const char* ContentType = Utils::ContentType(Base->Request.Path, &Base->Route->MimeTypes);
				const char* StatusMessage = Utils::StatusMessage(Base->Response.StatusCode = (Base->Response.Error && Base->Response.StatusCode > 0 ? Base->Response.StatusCode : 200));
				int64_t ContentLength = (int64_t)Base->Resource.Size;

				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				char LastModified[64];
				Core::DateTime::FetchWebDateGMT(LastModified, sizeof(LastModified), Base->Resource.LastModified);

				char ETag[64];
				Core::OS::Net::GetETag(ETag, sizeof(ETag), &Base->Resource);

				Core::String Content;
				Content.append(Base->Request.Version).append(" ");
				Content.append(Core::ToString(Base->Response.StatusCode)).append(" ");
				Content.append(StatusMessage).append("\r\n");
				Content.append("Date: ").append(Date).append("\r\n");
				Content.append(Utils::ConnectionResolve(Base));

				const char* Origin = Base->Request.GetHeader("Origin");
				if (Origin != nullptr)
					Content.append("Access-Control-Allow-Origin: ").append(Base->Route->AccessControlAllowOrigin).append("\r\n");

				Paths::ConstructHeadCache(Base, Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, Content);

				const char* Message = Base->Response.GetHeader("X-Error");
				if (Message != nullptr)
					Content.append("X-Error: ").append(Message).append("\r\n");

				Content.append("Accept-Ranges: bytes\r\nLast-Modified: ").append(LastModified).append("\r\n");
				Content.append("Etag: ").append(ETag).append("\r\n");
				Content.append("Content-Type: ").append(ContentType).append("; charset=").append(Base->Route->CharSet).append("\r\n");
				Content.append("Content-Encoding: ").append(Gzip ? "gzip" : "deflate").append("\r\n");
				Content.append("Transfer-Encoding: chunked\r\n");
				Content.append(ContentRange).append("\r\n");

				if (!ContentLength || !strcmp(Base->Request.Method, "HEAD"))
				{
					return !!Base->Stream->WriteAsync(Content.c_str(), Content.size(), [Base](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
							Base->Finish();
						else if (Packet::IsError(Event))
							Base->Break();
					});
				}

				return !!Base->Stream->WriteAsync(Content.c_str(), Content.size(), [Base, Range, ContentLength, Gzip](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
						Core::Schedule::Get()->SetTask([Base, Range, ContentLength, Gzip]() { Logical::ProcessFileCompress(Base, (size_t)ContentLength, (size_t)Range, Gzip); });
					else if (Packet::IsError(Event))
						Base->Break();
				});
			}
			bool Logical::ProcessResourceCache(Connection* Base)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				char Date[64];
				Core::DateTime::FetchWebDateGMT(Date, sizeof(Date), Base->Info.Start / 1000);

				char LastModified[64];
				Core::DateTime::FetchWebDateGMT(LastModified, sizeof(LastModified), Base->Resource.LastModified);

				char ETag[64];
				Core::OS::Net::GetETag(ETag, sizeof(ETag), &Base->Resource);

				Core::String Content;
				Content.append(Base->Request.Version);
				Content.append(" 304 Not Modified\r\nDate: ");
				Content.append(Date).append("\r\n");

				Paths::ConstructHeadCache(Base, Content);
				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, Content);

				Content.append("Accept-Ranges: bytes\r\nLast-Modified: ").append(LastModified).append("\r\n");
				Content.append("Etag: ").append(ETag).append("\r\n");
				Content.append(Utils::ConnectionResolve(Base)).append("\r\n");

				return !!Base->Stream->WriteAsync(Content.c_str(), Content.size(), [Base](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
						Base->Finish(304);
					else if (Packet::IsError(Event))
						Base->Break();
				});
			}
			bool Logical::ProcessFile(Connection* Base, size_t ContentLength, size_t Range)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				Range = (Range > Base->Resource.Size ? Base->Resource.Size : Range);
				if (ContentLength > 0 && Base->Resource.IsReferenced && Base->Resource.Size > 0)
				{
					size_t Limit = Base->Resource.Size - Range;
					if (ContentLength > Limit)
						ContentLength = Limit;

					if (Base->Response.Content.Data.size() >= ContentLength)
					{
						return !!Base->Stream->WriteAsync(Base->Response.Content.Data.data() + Range, ContentLength, [Base](SocketPoll Event)
						{
							if (Packet::IsDone(Event))
								Base->Finish();
							else if (Packet::IsError(Event))
								Base->Break();
						});
					}
				}

				auto File = Core::OS::File::Open(Base->Request.Path.c_str(), "rb");
				if (!File)
					return Base->Error(500, "System denied to open resource stream.");

				FILE* Stream = *File;
                if (Base->Route->AllowSendFile)
                {
                    auto Result = Base->Stream->SendFileAsync(Stream, Range, ContentLength, [Base, Stream, ContentLength, Range](SocketPoll Event)
                    {
                        if (Packet::IsDone(Event))
                        {
                            Core::OS::File::Close(Stream);
                            Base->Finish();
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
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				VI_ASSERT(Stream != nullptr, "stream should be set");
				if (!Core::OS::File::Seek64(Stream, Range, Core::FileSeek::Begin))
				{
					Core::OS::File::Close(Stream);
					return Base->Error(400, "Provided content range offset (%" PRIu64 ") is invalid", Range);
				}

                return ProcessFileChunk(Base, Base->Root, Stream, ContentLength);
			}
			bool Logical::ProcessFileChunk(Connection* Base, Server* Router, FILE* Stream, size_t ContentLength)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				VI_ASSERT(Router != nullptr, "router should be set");
				VI_ASSERT(Stream != nullptr, "stream should be set");

            Retry:
                char Buffer[Core::BLOB_SIZE];
				if (!ContentLength || Router->State != ServerState::Working)
				{
				Cleanup:
					Core::OS::File::Close(Stream);
					if (Router->State != ServerState::Working)
						return Base->Break();

					return Base->Finish() || true;
				}
                
				size_t Read = sizeof(Buffer);
				if ((Read = (size_t)fread(Buffer, 1, Read > ContentLength ? ContentLength : Read, Stream)) <= 0)
					goto Cleanup;
                
				ContentLength -= Read;
                auto Written = Base->Stream->WriteAsync(Buffer, Read, [Base, Router, Stream, ContentLength](SocketPoll Event)
				{
					if (Packet::IsDoneAsync(Event))
					{
                        Core::Schedule::Get()->SetTask([Base, Router, Stream, ContentLength]()
						{
							ProcessFileChunk(Base, Router, Stream, ContentLength);
						});
					}
					else if (Packet::IsError(Event))
					{
						Core::OS::File::Close(Stream);
						Base->Break();
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
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
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
								Base->Response.Content.Assign(Buffer.c_str(), (size_t)ZStream.total_out);
						}
#endif
						return !!Base->Stream->WriteAsync(Base->Response.Content.Data.data(), ContentLength, [Base](SocketPoll Event)
						{
							if (Packet::IsDone(Event))
								Base->Finish();
							else if (Packet::IsError(Event))
								Base->Break();
						});
					}
				}

				auto File = Core::OS::File::Open(Base->Request.Path.c_str(), "rb");
				if (!File)
					return Base->Error(500, "System denied to open resource stream.");

				FILE* Stream = *File;
				if (Range > 0 && !Core::OS::File::Seek64(Stream, Range, Core::FileSeek::Begin))
				{
					Core::OS::File::Close(Stream);
					return Base->Error(400, "Provided content range offset (%" PRIu64 ") is invalid", Range);
				}
#ifdef VI_ZLIB
				Server* Server = Base->Root;
				z_stream* ZStream = VI_MALLOC(z_stream, sizeof(z_stream));
				ZStream->zalloc = Z_NULL;
				ZStream->zfree = Z_NULL;
				ZStream->opaque = Z_NULL;

				if (deflateInit2(ZStream, Base->Route->Compression.QualityLevel, Z_DEFLATED, (Gzip ? MAX_WBITS + 16 : MAX_WBITS), Base->Route->Compression.MemoryLevel, (int)Base->Route->Compression.Tune) != Z_OK)
				{
					Core::OS::File::Close(Stream);
					VI_FREE(ZStream);
					return Base->Break();
				}

				return ProcessFileCompressChunk(Base, Server, Stream, ZStream, ContentLength);
#else
				Core::OS::File::Close(Stream);
				return Base->Error(500, "Cannot process gzip stream.");
#endif
			}
			bool Logical::ProcessFileCompressChunk(Connection* Base, Server* Router, FILE* Stream, void* CStream, size_t ContentLength)
			{
				VI_ASSERT(Base != nullptr && Base->Route != nullptr, "connection should be set");
				VI_ASSERT(Router != nullptr, "router should be set");
				VI_ASSERT(Stream != nullptr, "stream should be set");
				VI_ASSERT(CStream != nullptr, "cstream should be set");
#ifdef VI_ZLIB
#define FREE_STREAMING { Core::OS::File::Close(Stream); deflateEnd(ZStream); VI_FREE(ZStream); }
				z_stream* ZStream = (z_stream*)CStream;
            Retry:
                char Buffer[Core::BLOB_SIZE + GZ_HEADER_SIZE], Deflate[Core::BLOB_SIZE];
				if (!ContentLength || Router->State != ServerState::Working)
				{
				Cleanup:
					FREE_STREAMING;
					if (Router->State != ServerState::Working)
						return Base->Break();

					return !!Base->Stream->WriteAsync("0\r\n\r\n", 5, [Base](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
							Base->Finish();
						else if (Packet::IsError(Event))
							Base->Break();
					});
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

				int Next = snprintf(Buffer, sizeof(Buffer), "%X\r\n", (unsigned int)Read);
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

				auto Written = Base->Stream->WriteAsync(Buffer, Read, [Base, Router, Stream, ZStream, ContentLength](SocketPoll Event)
				{
					if (Packet::IsDoneAsync(Event))
					{
						if (ContentLength > 0)
						{
                            Core::Schedule::Get()->SetTask([Base, Router, Stream, ZStream, ContentLength]()
                            {
                                ProcessFileCompressChunk(Base, Router, Stream, ZStream, ContentLength);
                            });
						}
						else
						{
							FREE_STREAMING;
							Base->Finish();
						}
					}
					else if (Packet::IsError(Event))
					{
						FREE_STREAMING;
						Base->Break();
					}
					else if (Packet::IsSkip(Event))
						FREE_STREAMING;
				});
                if (Written && *Written > 0)
                    goto Retry;
                
				return false;
#undef FREE_STREAMING
#else
				return Base->Finish();
#endif
			}
			bool Logical::ProcessWebSocket(Connection* Base, const char* Key)
			{
				VI_ASSERT(Base != nullptr, "connection should be set");
				VI_ASSERT(Key != nullptr, "key should be set");

				const char* Version = Base->Request.GetHeader("Sec-WebSocket-Version");
				if (!Version || strcmp(Version, "13") != 0)
					return Base->Error(426, "Protocol upgrade required. Version \"%s\" is not allowed", Version);

				char Buffer[100];
				snprintf(Buffer, sizeof(Buffer), "%s%s", Key, WEBSOCKET_KEY);
				Base->Request.Content.Data.clear();

				char Encoded20[20];
				Compute::Crypto::Sha1Compute(Buffer, (int)strlen(Buffer), Encoded20);

				Core::String Content;
				Content.append(
					"HTTP/1.1 101 Switching Protocols\r\n"
					"Upgrade: websocket\r\n"
					"Connection: Upgrade\r\n"
					"Sec-WebSocket-Accept: ");
				Content.append(Compute::Codec::Base64Encode((const unsigned char*)Encoded20, 20));
				Content.append("\r\n");

				const char* Protocol = Base->Request.GetHeader("Sec-WebSocket-Protocol");
				if (Protocol != nullptr)
				{
					const char* Offset = strchr(Protocol, ',');
					if (Offset != nullptr)
						Content.append("Sec-WebSocket-Protocol: ").append(Protocol, (size_t)(Offset - Protocol)).append("\r\n");
					else
						Content.append("Sec-WebSocket-Protocol: ").append(Protocol).append("\r\n");
				}

				if (Base->Route->Callbacks.Headers)
					Base->Route->Callbacks.Headers(Base, Content);

				Content.append("\r\n", 2);
				return !!Base->Stream->WriteAsync(Content.c_str(), Content.size(), [Base](SocketPoll Event)
				{
					if (Packet::IsDone(Event))
					{
						Base->WebSocket = new WebSocketFrame(Base->Stream);
						Base->WebSocket->Connect = Base->Route->Callbacks.WebSocket.Connect;
						Base->WebSocket->Receive = Base->Route->Callbacks.WebSocket.Receive;
						Base->WebSocket->Disconnect = Base->Route->Callbacks.WebSocket.Disconnect;
						Base->WebSocket->Lifetime.Dead = [Base](WebSocketFrame*)
						{
							return Base->Info.Close;
						};
						Base->WebSocket->Lifetime.Close = [Base](WebSocketFrame*, bool Successful)
						{
							if (Successful)
							{
								Base->Info.KeepAlive = 0;
								if (Base->Response.StatusCode <= 0)
									Base->Response.StatusCode = 101;
								Base->Finish();
							}
							else
								Base->Break();
						};

						Base->Stream->Timeout = Base->Route->WebSocketTimeout;
						if (!Base->Route->Callbacks.WebSocket.Initiate || !Base->Route->Callbacks.WebSocket.Initiate(Base))
							Base->WebSocket->Next();
					}
					else if (Packet::IsError(Event))
						Base->Break();
				});
			}

			Server::Server() : SocketServer()
			{
			}
			Core::ExpectsIO<void> Server::Update()
			{
				auto* Root = (MapRouter*)Router;
				for (auto& Site : Root->Sites)
				{
					auto* Entry = Site.second;
					Entry->Base->URI = Compute::RegexSource("/");
					Entry->Base->Site = Entry;
					Entry->Router = Root;

					if (!Entry->Session.DocumentRoot.empty())
					{
						auto Directory = Core::OS::Path::Resolve(Entry->Session.DocumentRoot.c_str());
						if (Directory)
							Entry->Session.DocumentRoot = *Directory;

						auto Status = Core::OS::Directory::Patch(Entry->Session.DocumentRoot);
						if (!Status)
							return Status;
					}

					if (!Entry->ResourceRoot.empty())
					{
						auto Directory = Core::OS::Path::Resolve(Entry->ResourceRoot.c_str());
						if (Directory)
							Entry->ResourceRoot = *Directory;

						auto Status = Core::OS::Directory::Patch(Entry->ResourceRoot);
						if (!Status)
							return Status;
					}

					if (!Entry->Base->Override.empty())
					{
						auto Directory = Core::OS::Path::Resolve(Entry->Base->Override, Entry->Base->DocumentRoot, false);
						if (Directory)
							Entry->Base->Override = *Directory;
					}

					for (auto& Group : Entry->Groups)
					{
						for (auto* Route : Group->Routes)
						{
							Route->Site = Entry;
							if (Route->Override.empty())
								continue;

							auto Directory = Core::OS::Path::Resolve(Route->Override, Route->DocumentRoot, false);
							if (Directory)
								Route->Override = *Directory;
						}
					}

					Entry->Sort();
				}
				return Core::Optional::OK;
			}
			Core::ExpectsIO<void> Server::OnConfigure(SocketRouter* NewRouter)
			{
				VI_ASSERT(NewRouter != nullptr, "router should be set");
				return Update();
			}
			Core::ExpectsIO<void> Server::OnUnlisten()
			{
				VI_ASSERT(Router != nullptr, "router should be set");
				MapRouter* Root = (MapRouter*)Router;

				for (auto& Site : Root->Sites)
				{
					auto* Entry = Site.second;
					if (!Entry->ResourceRoot.empty())
					{
						auto Status = Core::OS::Directory::Remove(Entry->ResourceRoot.c_str());
						if (!Status)
						{
							VI_ERR("[http] resource directory %s cannot be deleted", Entry->ResourceRoot.c_str());
							return Status;
						}
					}

					if (!Entry->Session.DocumentRoot.empty())
					{
						auto Status = Session::InvalidateCache(Entry->Session.DocumentRoot);
						if (!Status)
							return Status;
					}
				}

				return Core::Optional::OK;
			}
			void Server::OnRequestOpen(SocketConnection* Source)
			{
				VI_ASSERT(Source != nullptr, "connection should be set");
				auto* Conf = (MapRouter*)Router;
				auto* Base = (Connection*)Source;

				Base->Parsers.Request->PrepareForNextParsing(Base, false);
				Base->Stream->ReadUntilAsync("\r\n\r\n", [Base, Conf](SocketPoll Event, const char* Buffer, size_t Size)
				{
					if (Packet::IsData(Event))
					{
						size_t LastLength = Base->Request.Content.Data.size();
						Base->Request.Content.Append(Buffer, Size);

						int64_t Offset = Base->Parsers.Request->ParseRequest(Base->Request.Content.Data.data(), Base->Request.Content.Data.size(), LastLength);
						if (Offset >= 0 || Offset == -2)
							return true;

						Base->Error(400, "Invalid request was provided by client");
						return false;
					}
					else if (Packet::IsDone(Event))
					{
						uint32_t Redirects = 0;
						Base->Request.Content.Data.clear();
						Base->Info.Start = Network::Utils::Clock();
					Redirect:
						if (!Paths::ConstructRoute(Conf, Base))
							return Base->Error(400, "Request cannot be resolved");

						if (!Base->Route->Redirect.empty())
						{
							if (Redirects++ > MAX_REDIRECTS)
								return Base->Error(500, "Infinite redirects loop detected");

							Base->Request.URI = Base->Route->Redirect;
							goto Redirect;
						}

						if (!Base->Route->ProxyIpAddress.empty())
						{
							const char* Address = Base->Request.GetHeader(Base->Route->ProxyIpAddress.c_str());
							if (Address != nullptr)
								strncpy(Base->RemoteAddress, Address, sizeof(Base->RemoteAddress));
						}

						Base->Request.Content.Prepare(Base->Request.GetHeader("Content-Length"));
						Paths::ConstructPath(Base);

						if (!Permissions::MethodAllowed(Base))
							return Base->Error(405, "Requested method \"%s\" is not allowed on this server", Base->Request.Method);

						if (!memcmp(Base->Request.Method, "GET", 3) || !memcmp(Base->Request.Method, "HEAD", 4))
						{
							if (!Permissions::Authorize(Base))
								return false;

							if (Base->Route->Callbacks.Get && Base->Route->Callbacks.Get(Base))
								return true;

							return Routing::RouteGET(Base);
						}
						else if (!memcmp(Base->Request.Method, "POST", 4))
						{
							if (!Permissions::Authorize(Base))
								return false;

							if (Base->Route->Callbacks.Post && Base->Route->Callbacks.Post(Base))
								return true;

							return Routing::RoutePOST(Base);
						}
						else if (!memcmp(Base->Request.Method, "PUT", 3))
						{
							if (!Permissions::Authorize(Base))
								return false;

							if (Base->Route->Callbacks.Put && Base->Route->Callbacks.Put(Base))
								return true;

							return Routing::RoutePUT(Base);
						}
						else if (!memcmp(Base->Request.Method, "PATCH", 5))
						{
							if (!Permissions::Authorize(Base))
								return false;

							if (Base->Route->Callbacks.Patch && Base->Route->Callbacks.Patch(Base))
								return true;

							return Routing::RoutePATCH(Base);
						}
						else if (!memcmp(Base->Request.Method, "DELETE", 6))
						{
							if (!Permissions::Authorize(Base))
								return false;

							if (Base->Route->Callbacks.Delete && Base->Route->Callbacks.Delete(Base))
								return true;

							return Routing::RouteDELETE(Base);
						}
						else if (!memcmp(Base->Request.Method, "OPTIONS", 7))
						{
							if (Base->Route->Callbacks.Options && Base->Route->Callbacks.Options(Base))
								return true;

							return Routing::RouteOPTIONS(Base);
						}

						if (!Permissions::Authorize(Base))
							return false;

						return Base->Error(405, "Request method \"%s\" is not allowed", Base->Request.Method);
					}
					else if (Packet::IsError(Event))
						Base->Break();

					return true;
				});
			}
			bool Server::OnRequestCleanup(SocketConnection* Root)
			{
				VI_ASSERT(Root != nullptr, "connection should be set");
				auto Base = (HTTP::Connection*)Root;
				return Base->Skip([](HTTP::Connection* Base)
				{
					Base->Root->Finalize(Base);
					return true;
				});
			}
			void Server::OnRequestStall(SocketConnection* Root)
			{
				auto Base = (HTTP::Connection*)Root;
				Core::String Status = ", pathname: " + Base->Request.URI;
				if (Base->WebSocket != nullptr)
					Status += ", websocket: " + Core::String(Base->WebSocket->IsFinished() ? "alive" : "dead");
				VI_DEBUG("[stall] connection on fd %i%s", (int)Base->Stream->GetFd(), Status.c_str());
			}
			void Server::OnRequestClose(SocketConnection* Root)
			{
				VI_ASSERT(Root != nullptr, "connection should be set");
				auto Base = (HTTP::Connection*)Root;
				if (Base->Info.KeepAlive >= -1 && Base->Response.StatusCode >= 0 && Base->Route && Base->Route->Callbacks.Access)
					Base->Route->Callbacks.Access(Base);
				Base->Reset(false);
			}
			SocketConnection* Server::OnAllocate(SocketListener* Host)
			{
				VI_ASSERT(Host != nullptr, "host should be set");
				return new HTTP::Connection(this);
			}
			SocketRouter* Server::OnAllocateRouter()
			{
				return new MapRouter();
			}

			Client::Client(int64_t ReadTimeout) : SocketClient(ReadTimeout), WebSocket(nullptr)
			{
			}
			Client::~Client()
			{
				VI_CLEAR(WebSocket);
			}
			Core::ExpectsPromiseIO<void> Client::Consume(size_t MaxSize)
			{
				VI_ASSERT(!WebSocket, "cannot read http over websocket");
				if (Response.Content.IsFinalized())
					return Core::ExpectsPromiseIO<void>(Core::Optional::OK);
				else if (Response.Content.Exceeds)
					return Core::ExpectsPromiseIO<void>(std::make_error_condition(std::errc::value_too_large));

				Response.Content.Data.clear();
				if (!Stream || !Stream->IsValid())
					return Core::ExpectsPromiseIO<void>(std::make_error_condition(std::errc::bad_file_descriptor));

				const char* ContentType = Response.GetHeader("Content-Type");
				if (ContentType && !strncmp(ContentType, "multipart/form-data", 19))
				{
					Response.Content.Exceeds = true;
					return Core::ExpectsPromiseIO<void>(std::make_error_condition(std::errc::value_too_large));
				}
                
				const char* TransferEncoding = Response.GetHeader("Transfer-Encoding");
				if (!Response.Content.Limited && TransferEncoding && !Core::Stringify::CaseCompare(TransferEncoding, "chunked"))
				{
					Core::ExpectsPromiseIO<void> Result;
					Parser* Parser = new HTTP::Parser();
					Stream->ReadAsync(MaxSize, [this, Parser, Result, MaxSize](SocketPoll Event, const char* Buffer, size_t Recv) mutable
					{
						if (Packet::IsData(Event))
						{
							int64_t Subresult = Parser->ParseDecodeChunked((char*)Buffer, &Recv);
							if (Subresult == -1)
							{
								VI_RELEASE(Parser);
								Result.Set(std::make_error_condition(std::errc::protocol_error));

								return false;
							}
							else if (Subresult >= 0 || Subresult == -2)
							{
								Response.Content.Offset += Recv;
								Response.Content.Append(Buffer, Recv);
							}

							return Subresult == -2;
						}
						else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
						{
							if (!Response.Content.Limited)
							{
								Response.Content.Length += Response.Content.Offset;
								Response.Content.Limited = true;
							}

							if (!Response.Content.Data.empty())
								VI_DEBUG("[http] %i responded\n%.*s", (int)Stream->GetFd(), (int)Response.Content.Data.size(), Response.Content.Data.data());

							if (Packet::IsErrorOrSkip(Event))
								Result.Set(Packet::ToCondition(Event));
							else
								Result.Set(Core::Optional::OK);
							VI_RELEASE(Parser);
						}

						return true;
					});
					return Result;
				}
				else if (!Response.Content.Limited)
				{
					const char* Connection = Response.GetHeader("Connection");
					if (!Connection || Core::Stringify::CaseCompare(Connection, "close"))
						return Core::ExpectsPromiseIO<void>(std::make_error_condition(std::errc::no_protocol_option));

					Core::ExpectsPromiseIO<void> Result;
					Stream->ReadAsync(MaxSize, [this, Result, MaxSize](SocketPoll Event, const char* Buffer, size_t Recv) mutable
					{
						if (Packet::IsData(Event))
						{
							Response.Content.Offset += Recv;
							Response.Content.Append(Buffer, Recv);
							return true;
						}
						else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
						{
							Response.Content.Length += Response.Content.Offset;
							if (!Response.Content.Data.empty())
								VI_DEBUG("[http] %i responded\n%.*s", (int)Stream->GetFd(), (int)Response.Content.Data.size(), Response.Content.Data.data());

							if (Packet::IsErrorOrSkip(Event))
								Result.Set(Packet::ToCondition(Event));
							else
								Result.Set(Core::Optional::OK);
						}

						return true;
					});
					return Result;
				}
                
				MaxSize = std::min(MaxSize, Response.Content.Length - Response.Content.Offset);
				if (!MaxSize || Response.Content.Offset > Response.Content.Length)
					return Core::ExpectsPromiseIO<void>(std::make_error_condition(std::errc::result_out_of_range));

				Core::ExpectsPromiseIO<void> Result;
				Stream->ReadAsync(MaxSize, [this, Result, MaxSize](SocketPoll Event, const char* Buffer, size_t Recv) mutable
				{
					if (Packet::IsData(Event))
					{
						Response.Content.Offset += Recv;
						Response.Content.Append(Buffer, Recv);
						return true;
					}
					else if (Packet::IsDone(Event) || Packet::IsErrorOrSkip(Event))
					{
						if (!Response.Content.Data.empty())
							VI_DEBUG("[http] %i responded\n%.*s", (int)Stream->GetFd(), (int)Response.Content.Data.size(), Response.Content.Data.data());

						if (Packet::IsErrorOrSkip(Event))
							Result.Set(Packet::ToCondition(Event));
						else
							Result.Set(Core::Optional::OK);
					}

					return true;
				});
				return Result;
			}
			Core::ExpectsPromiseIO<void> Client::Fetch(HTTP::RequestFrame&& Root, size_t MaxSize)
			{
				return Send(std::move(Root)).Then<Core::ExpectsPromiseIO<void>>([this, MaxSize](Core::ExpectsIO<ResponseFrame*>&& Response) -> Core::ExpectsPromiseIO<void>
				{
					if (!Response)
						return Core::ExpectsPromiseIO<void>(Response.Error());

					return Consume(MaxSize);
				});
			}
			Core::ExpectsPromiseIO<void> Client::Upgrade(HTTP::RequestFrame&& Root)
			{
				VI_ASSERT(WebSocket != nullptr, "websocket should be opened");
				if (!Stream || !Stream->IsValid())
					return Core::ExpectsPromiseIO<void>(std::make_error_condition(std::errc::bad_file_descriptor));

				Root.SetHeader("Pragma", "no-cache");
				Root.SetHeader("Upgrade", "WebSocket");
				Root.SetHeader("Connection", "Upgrade");
				Root.SetHeader("Sec-WebSocket-Version", "13");

				auto Random = Compute::Crypto::RandomBytes(16);
				if (Random)
					Root.SetHeader("Sec-WebSocket-Key", Compute::Codec::Base64Encode(*Random));
				else
					Root.SetHeader("Sec-WebSocket-Key", WEBSOCKET_KEY);
	
				return Send(std::move(Root)).Then<Core::ExpectsPromiseIO<void>>([this](Core::ExpectsIO<ResponseFrame*>&& Response) -> Core::ExpectsPromiseIO<void>
				{
					VI_DEBUG("[ws] handshake %s", Request.URI.c_str());
					if (!Response)
						return Core::ExpectsPromiseIO<void>(Response.Error());

					if (Response->StatusCode != 101)
					{
						Error("ws handshake error");
						return Core::ExpectsPromiseIO<void>(std::make_error_condition(std::errc::protocol_error));
					}

					if (!Response->GetHeader("Sec-WebSocket-Accept"))
					{
						Error("ws handshake was not accepted");
						return Core::ExpectsPromiseIO<void>(std::make_error_condition(std::errc::bad_message));
					}

					Future = Core::ExpectsPromiseIO<void>();
					WebSocket->Next();
					return Future;
				});
			}
			Core::ExpectsPromiseIO<ResponseFrame*> Client::Send(HTTP::RequestFrame&& Root)
			{
				VI_ASSERT(!WebSocket || Root.GetHeader("Sec-WebSocket-Key") != nullptr, "cannot send http request over websocket");
				if (!Stream || !Stream->IsValid())
					return Core::ExpectsPromiseIO<ResponseFrame*>(std::make_error_condition(std::errc::bad_file_descriptor));

				VI_DEBUG("[http] %s %s", Root.Method, Root.URI.c_str());
				Core::ExpectsPromiseIO<ResponseFrame*> Result;
				Request = std::move(Root);
				Response.Cleanup();
				State.Done = [Result](SocketClient* Client, const Core::Option<std::error_condition>& ErrorCode) mutable
				{
					HTTP::Client* Base = (HTTP::Client*)Client;
					if (ErrorCode)
					{
						Base->GetResponse()->StatusCode = -1;
						Result.Set(*ErrorCode);
					}
					else
						Result.Set(Base->GetResponse());
				};
				Stage("request delivery");

				if (!Request.GetHeader("Host"))
				{
					if (Context != nullptr)
					{
						if (State.Hostname.Port == 443)
							Request.SetHeader("Host", State.Hostname.Hostname.c_str());
						else
							Request.SetHeader("Host", (State.Hostname.Hostname + ':' + Core::ToString(State.Hostname.Port)));
					}
					else
					{
						if (State.Hostname.Port == 80)
							Request.SetHeader("Host", State.Hostname.Hostname);
						else
							Request.SetHeader("Host", (State.Hostname.Hostname + ':' + Core::ToString(State.Hostname.Port)));
					}
				}

				if (!Request.GetHeader("Accept"))
					Request.SetHeader("Accept", "*/*");

				if (!Request.GetHeader("User-Agent"))
					Request.SetHeader("User-Agent", "Lynx/1.1");

				if (!Request.GetHeader("Content-Length"))
				{
					Request.Content.Length = Request.Content.Data.size();
					Request.SetHeader("Content-Length", Core::ToString(Request.Content.Data.size()));
				}

				if (!Request.GetHeader("Connection"))
					Request.SetHeader("Connection", "Keep-Alive");

				Core::String Content;
				if (Request.URI.empty())
					Request.URI = "/";

				if (!Request.Query.empty())
				{
					Content.append(Request.Method).append(" ");
					Content.append(Request.URI).append("?");
					Content.append(Request.Query).append(" ");
					Content.append(Request.Version).append("\r\n");
				}
				else
				{
					Content.append(Request.Method).append(" ");
					Content.append(Request.URI).append(" ");
					Content.append(Request.Version).append("\r\n");
				}

				if (Request.Content.Resources.empty())
				{
					if (!Request.Content.Data.empty())
					{
						if (!Request.GetHeader("Content-Type"))
							Request.SetHeader("Content-Type", "application/octet-stream");

						if (!Request.GetHeader("Content-Length"))
							Request.SetHeader("Content-Length", Core::ToString(Request.Content.Data.size()).c_str());
					}
					else if (!memcmp(Request.Method, "POST", 4) || !memcmp(Request.Method, "PUT", 3) || !memcmp(Request.Method, "PATCH", 5))
						Request.SetHeader("Content-Length", "0");

					Paths::ConstructHeadFull(&Request, &Response, true, Content);
					Content.append("\r\n");

					Stream->WriteAsync(Content.c_str(), Content.size(), [this](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
						{
							if (!Request.Content.Data.empty())
							{
								Stream->WriteAsync(Request.Content.Data.data(), Request.Content.Data.size(), [this](SocketPoll Event)
								{
									if (Packet::IsDone(Event))
									{
										Stream->ReadUntilAsync("\r\n\r\n", [this](SocketPoll Event, const char* Buffer, size_t Recv)
										{
											if (Packet::IsData(Event))
												Response.Content.Append(Buffer, Recv);
											else if (Packet::IsDone(Event))
												Receive();
											else if (Packet::IsErrorOrSkip(Event))
												Error("http socket read %s", (Event == SocketPoll::Timeout ? "timeout" : "error"));

											return true;
										});
									}
									else if (Packet::IsErrorOrSkip(Event))
										Error("http socket write %s", (Event == SocketPoll::Timeout ? "timeout" : "error"));
								});
							}
							else
							{
								Stream->ReadUntilAsync("\r\n\r\n", [this](SocketPoll Event, const char* Buffer, size_t Recv)
								{
									if (Packet::IsData(Event))
										Response.Content.Append(Buffer, Recv);
									else if (Packet::IsDone(Event))
										Receive();
									else if (Packet::IsErrorOrSkip(Event))
										Error("http socket read %s", (Event == SocketPoll::Timeout ? "timeout" : "error"));

									return true;
								});
							}
						}
						else if (Packet::IsErrorOrSkip(Event))
							Error("http socket write %s", (Event == SocketPoll::Timeout ? "timeout" : "error"));
					});
				}
				else
				{
					auto RandomBytes = Compute::Crypto::RandomBytes(24);
					if (!RandomBytes)
					{
						Error("cannot generate request boundary");
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

						Core::String FormBody;
						for (auto& Header : Item.Headers)
						{
							FormBody.append(Boundary).append("\r\n");
							FormBody.append("Content-Disposition: form-data; name=\"").append(Header.first).append("\"\r\n\r\n");
							for (auto& Value : Header.second)
								FormBody.append(Value);
							FormBody.append("\r\n\r\n").append(Boundary).append("\r\n");
						}

						Core::String FormFinish = "\r\n";
						FormFinish.append(Boundary);

						bool IsFinal = (Boundaries.size() + 1 == Request.Content.Resources.size());
						if (IsFinal)
							FormFinish.append("--\r\n");
						else
							FormFinish.append("\r\n");

						FormBody.append(Boundary).append("\r\n");
						FormBody.append("Content-Disposition: form-data; name=\"").append(Item.Key).append("\"; filename=\"").append(Item.Name).append("\"\r\n");
						FormBody.append("Content-Type: ").append(Item.Type).append("\r\n\r\n");

						if (Item.IsInMemory)
						{
							Item.Length = Item.GetInMemoryContents().size();
							FormBody.append(Item.GetInMemoryContents());
							FormBody.append(FormFinish);
						}
						else
						{
							auto State = Core::OS::File::GetState(Item.Path);
							if (State && !State->IsDirectory)
								Item.Length = State->Size;
							else
								Item.Length = 0;
						}

						BoundaryBlock Block;
						Block.Data = std::move(FormBody);
						Block.Finish = std::move(FormFinish);
						Block.File = &Item;
						Block.IsFinal = IsFinal;

						ContentSize += Block.Data.size() + Item.Length + Block.Finish.size();
						Boundaries.emplace_back(std::move(Block));
					}

					Request.SetHeader("Content-Length", Core::ToString(ContentSize));
					Paths::ConstructHeadFull(&Request, &Response, true, Content);
					Content.append("\r\n");

					Stream->WriteAsync(Content.c_str(), Content.size(), [this](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
							Upload(0);
						else if (Packet::IsErrorOrSkip(Event))
							Error("http socket write %s", (Event == SocketPoll::Timeout ? "timeout" : "error"));
					});
				}

				return Result;
			}
			Core::ExpectsPromiseIO<Core::Schema*> Client::JSON(HTTP::RequestFrame&& Root, size_t MaxSize)
			{
				return Fetch(std::move(Root), MaxSize).Then<Core::ExpectsIO<Core::Schema*>>([this](Core::ExpectsIO<void>&& Result) -> Core::ExpectsIO<Core::Schema*>
				{
					if (!Result)
						return Result.Error();

					auto Data = Core::Schema::ConvertFromJSON(Response.Content.Data.data(), Response.Content.Data.size());
					if (!Data)
						return std::make_error_condition(std::errc::bad_message);

					return *Data;
				});
			}
			Core::ExpectsPromiseIO<Core::Schema*> Client::XML(HTTP::RequestFrame&& Root, size_t MaxSize)
			{
				return Fetch(std::move(Root), MaxSize).Then<Core::ExpectsIO<Core::Schema*>>([this](Core::ExpectsIO<void>&& Result) -> Core::ExpectsIO<Core::Schema*>
				{
					if (!Result)
						return Result.Error();

					auto Data = Core::Schema::ConvertFromXML(Response.Content.Data.data(), Response.Content.Data.size());
					if (!Data)
						return std::make_error_condition(std::errc::bad_message);

					return *Data;
				});
			}
			void Client::Downgrade()
			{
				VI_ASSERT(WebSocket != nullptr, "websocket should be opened");
				VI_ASSERT(WebSocket->IsFinished(), "websocket connection should be finished");
				VI_CLEAR(WebSocket);
			}
			WebSocketFrame* Client::GetWebSocket()
			{
				if (WebSocket != nullptr)
					return WebSocket;

				WebSocket = new WebSocketFrame(Stream);
				WebSocket->Lifetime.Dead = [](WebSocketFrame*)
				{
					return false;
				};
				WebSocket->Lifetime.Close = [this](WebSocketFrame*, bool Successful)
				{
					if (!Successful)
					{
						Stream->Close(false);
						Future.Set(std::make_error_condition(std::errc::connection_reset));
					}
					else
						Future.Set(Core::Optional::OK);
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
			void Client::UploadFile(BoundaryBlock* Boundary, std::function<void(bool)>&& Callback)
			{
				auto File = Core::OS::File::Open(Boundary->File->Path.c_str(), "rb");
				if (!File)
					return Callback(false);

				FILE* FileStream = *File;
                auto Result = Stream->SendFileAsync(FileStream, 0, Boundary->File->Length, [this, FileStream, Callback](SocketPoll Event)
                {
                    if (Packet::IsDone(Event))
                    {
                        Core::OS::File::Close(FileStream);
						Callback(true);
                    }
                    else if (Packet::IsErrorOrSkip(Event))
					{
						Core::OS::File::Close(FileStream);
						Callback(false);
					}
				});         
                if (Result || Result.Error() != std::errc::not_supported)
					return Callback(true);

				if (Config.IsAsync)
					return UploadFileChunkAsync(FileStream, Boundary->File->Length, std::move(Callback));

				return UploadFileChunk(FileStream, Boundary->File->Length, std::move(Callback));
			}
			void Client::UploadFileChunk(FILE* FileStream, size_t ContentLength, std::function<void(bool)>&& Callback)
			{
				if (!ContentLength)
				{
				Cleanup:
					Core::OS::File::Close(FileStream);
					return Callback(true);
				}

				char Buffer[Core::BLOB_SIZE];
				while (ContentLength > 0)
				{
					size_t Read = sizeof(Buffer);
					if ((Read = (size_t)fread(Buffer, 1, Read > ContentLength ? ContentLength : Read, FileStream)) <= 0)
						goto Cleanup;

					ContentLength -= Read;
					auto Written = Stream->Write(Buffer, Read);
					if (!Written || !*Written)
						break;
				}

				Core::OS::File::Close(FileStream);
				return Callback(!ContentLength);
			}
			void Client::UploadFileChunkAsync(FILE* FileStream, size_t ContentLength, std::function<void(bool)>&& Callback)
			{
				if (!ContentLength)
				{
				Cleanup:
					Core::OS::File::Close(FileStream);
					return Callback(true);
				}

			Retry:
				char Buffer[Core::BLOB_SIZE];
				size_t Read = sizeof(Buffer);
				if ((Read = (size_t)fread(Buffer, 1, Read > ContentLength ? ContentLength : Read, FileStream)) <= 0)
					goto Cleanup;

				ContentLength -= Read;
				auto Written = Stream->WriteAsync(Buffer, Read, [this, FileStream, ContentLength, Callback](SocketPoll Event) mutable
				{
					if (Packet::IsDoneAsync(Event))
					{
						Core::Schedule::Get()->SetTask([this, FileStream, ContentLength, Callback = std::move(Callback)]() mutable
						{
							UploadFileChunk(FileStream, ContentLength, std::move(Callback));
						});
					}
					else if (Packet::IsErrorOrSkip(Event))
					{
						Core::OS::File::Close(FileStream);
						Callback(false);
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
					Stream->WriteAsync(Boundary->Data.c_str(), Boundary->Data.size(), [this, Boundary, FileId](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
						{
							if (!Boundary->File->IsInMemory)
							{
								BoundaryBlock& Target = *Boundary;
								UploadFile(&Target, [this, Boundary, FileId](bool Success)
								{
									if (Success)
									{
										Stream->WriteAsync(Boundary->Finish.c_str(), Boundary->Finish.size(), [this, Boundary, FileId](SocketPoll Event)
										{
											if (Packet::IsDone(Event))
												Upload(FileId + 1);
											else if (Packet::IsErrorOrSkip(Event))
												Error("http socket write %s", (Event == SocketPoll::Timeout ? "timeout" : "error"));
										});
									}
									else
										Error("io socker file write error");
								});
							}
							else
								Upload(FileId + 1);
						}
						else if (Packet::IsErrorOrSkip(Event))
							Error("http socket file write %s", (Event == SocketPoll::Timeout ? "timeout" : "error"));
					});
				}
				else
				{
					Stream->ReadUntilAsync("\r\n\r\n", [this](SocketPoll Event, const char* Buffer, size_t Recv)
					{
						if (Packet::IsData(Event))
							Response.Content.Append(Buffer, Recv);
						else if (Packet::IsDone(Event))
							Receive();
						else if (Packet::IsErrorOrSkip(Event))
							Error("http socket read %s", (Event == SocketPoll::Timeout ? "timeout" : "error"));

						return true;
					});
				}
			}
			void Client::ManageKeepAlive()
			{
				auto Connection = Response.Headers.find("Connection");
				if (Connection == Response.Headers.end())
					return DisableReusability();

				if (Connection->second.size() != 1 || Core::Stringify::CaseCompare(Connection->second.front().c_str(), "keep-alive") != 0)
					return DisableReusability();

				auto KeepAlive = Response.Headers.find("Keep-Alive");
				if (KeepAlive == Response.Headers.end())
					return;

				for (auto& Item : KeepAlive->second)
				{
					if (Item.find("timeout") == std::string::npos)
						continue;

					auto NewTimeout = Core::FromString<int64_t>(Item.substr(Item.find('=') + 1));
					if (NewTimeout)
						SetReusability(*NewTimeout * 1000);
				}
			}
			void Client::Receive()
			{
				Stage("http response receive");
				auto Address = Stream->GetRemoteAddress();
				if (Address)
					strncpy(RemoteAddress, Address->c_str(), std::min(Address->size(), sizeof(RemoteAddress)));

				Parser* Parser = new HTTP::Parser();
				Parser->OnMethodValue = Parsing::ParseMethodValue;
				Parser->OnPathValue = Parsing::ParsePathValue;
				Parser->OnQueryValue = Parsing::ParseQueryValue;
				Parser->OnVersion = Parsing::ParseVersion;
				Parser->OnStatusCode = Parsing::ParseStatusCode;
				Parser->OnHeaderField = Parsing::ParseHeaderField;
				Parser->OnHeaderValue = Parsing::ParseHeaderValue;
				Parser->Frame.Response = &Response;

				if (Parser->ParseResponse(Response.Content.Data.data(), Response.Content.Data.size(), 0) >= 0)
				{
					Response.Content.Prepare(Response.GetHeader("Content-Length"));
					Response.Content.Data.clear();
					ManageKeepAlive();

					VI_RELEASE(Parser);
					Report(Core::Optional::None);
				}
				else
				{
					VI_RELEASE(Parser);
					Error("cannot parse http response");
				}
			}

			Core::ExpectsPromiseIO<ResponseFrame> Fetch(const Core::String& Target, const Core::String& Method, const FetchFrame& Options)
			{
				return Core::Coasync<Core::ExpectsIO<ResponseFrame>>([Target, Method, Options]() -> Core::ExpectsPromiseIO<ResponseFrame>
				{
					Network::Location URL(Target);
					if (URL.Protocol != "http" && URL.Protocol != "https")
						Coreturn std::make_error_condition(std::errc::address_family_not_supported);

					Network::RemoteHost Address;
					Address.Hostname = URL.Hostname;
					Address.Secure = (URL.Protocol == "https");
					Address.Port = (URL.Port < 0 ? (Address.Secure ? 443 : 80) : URL.Port);

					HTTP::RequestFrame Request;
					Request.Cookies = Options.Cookies;
					Request.Headers = Options.Headers;
					Request.Content = Options.Content;
					Request.SetMethod(Method.c_str());
					Request.URI.assign(URL.Path);

					if (!URL.Username.empty() || !URL.Password.empty())
						Request.SetHeader("Authorization", Permissions::Authorize(URL.Username, URL.Password));

					size_t MaxSize = Options.MaxSize;
					HTTP::Client* Client = new HTTP::Client(Options.Timeout);
					auto Status = VI_AWAIT(Client->Connect(&Address, true, Options.VerifyPeers));
					if (!Status)
					{
						VI_AWAIT(Client->Disconnect());
						VI_RELEASE(Client);
						Coreturn Status.Error();
					}

					for (auto& Item : URL.Query)
						Request.Query += Item.first + "=" + Item.second + "&";

					if (!Request.Query.empty())
						Request.Query.pop_back();

					Status = VI_AWAIT(Client->Fetch(std::move(Request), MaxSize));
					ResponseFrame Response = std::move(*Client->GetResponse());
					VI_AWAIT(Client->Disconnect());
					VI_RELEASE(Client);

					if (!Status)
						Coreturn Status.Error();

					Coreturn Response;
				});
			}
		}
	}
}
#pragma warning(pop)