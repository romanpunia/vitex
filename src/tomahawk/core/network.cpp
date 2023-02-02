#include "network.h"
#include "../network/http.h"
#ifdef TH_MICROSOFT
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <io.h>
#ifdef TH_WITH_WEPOLL
#include <wepoll.h>
#endif
#define ERRNO WSAGetLastError()
#define ERRWOULDBLOCK WSAEWOULDBLOCK
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
#ifndef TH_APPLE
#include <sys/epoll.h>
#include <sys/sendfile.h>
#else
#include <sys/event.h>
#endif
#include <fcntl.h>
#include <poll.h>
#define ERRNO errno
#define ERRWOULDBLOCK EWOULDBLOCK
#define INVALID_SOCKET -1
#define INVALID_EPOLL -1
#define SOCKET_ERROR -1
#define closesocket close
#define epoll_close close
#define SD_SEND SHUT_WR
#endif
#define DNS_TIMEOUT 21600
#define CONNECT_TIMEOUT 2000
#define MAX_READ_UNTIL 512
#pragma warning(push)
#pragma warning(disable: 4996)

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
		static addrinfo* TryConnectDNS(const std::unordered_map<socket_t, addrinfo*>& Hosts, uint64_t Timeout)
		{
			TH_MEASURE(TH_TIMING_NET);

			std::vector<pollfd> Sockets4, Sockets6;
			for (auto& Host : Hosts)
			{
				Socket Stream(Host.first);
				Stream.SetBlocking(false);

				TH_DEBUG("[net] resolve dns on fd %i", (int)Host.first);
				int Status = connect(Host.first, Host.second->ai_addr, (int)Host.second->ai_addrlen);
				if (Status != 0)
				{
					int Code = Stream.GetError(Status);
					if (Code != EINPROGRESS && Code != ERRWOULDBLOCK)
						continue;
				}

				pollfd Fd;
				Fd.fd = Host.first;
				Fd.events = POLLOUT;
				if (Host.second->ai_family == AF_INET6)
					Sockets6.push_back(Fd);
				else
					Sockets4.push_back(Fd);
			}

			if (!Sockets4.empty() && Utils::Poll(Sockets4.data(), (int)Sockets4.size(), (int)Timeout) > 0)
			{
				for (auto& Fd : Sockets4)
				{
					if (Fd.revents & POLLOUT)
					{
						auto It = Hosts.find(Fd.fd);
						if (It != Hosts.end())
							return It->second;
					}
				}
			}

			if (!Sockets6.empty() && Utils::Poll(Sockets6.data(), (int)Sockets6.size(), (int)Timeout) > 0)
			{
				for (auto& Fd : Sockets6)
				{
					if (Fd.revents & POLLOUT)
					{
						auto It = Hosts.find(Fd.fd);
						if (It != Hosts.end())
							return It->second;
					}
				}
			}

			return nullptr;
		}
		static void* GetAddressStorage(struct sockaddr* Info)
		{
			if (Info->sa_family == AF_INET)
				return &(((struct sockaddr_in*)Info)->sin_addr);

			return &(((struct sockaddr_in6*)Info)->sin6_addr);
		}
    
        void Address::Use(Address& Other)
        {
            Free();
            Pool = Other.Pool;
            Usable = Other.Usable;
            Other.Pool = nullptr;
            Other.Usable = nullptr;
        }
        void Address::Use()
        {
            Usable = Pool;
        }
        void Address::Free()
        {
            if (Pool != nullptr)
                freeaddrinfo(Pool);
            
            Pool = nullptr;
            Usable = nullptr;
        }
        bool Address::IsUsable() const
        {
            return Usable != nullptr;
        }
        std::string Address::GetAddress() const
        {
            if (!Usable)
                return "127.0.0.1";
            
            return Socket::GetAddress(Usable);
        }
    
		SourceURL::SourceURL(const std::string& Src) noexcept : URL(Src), Protocol("file"), Port(0)
		{
			TH_ASSERT_V(!URL.empty(), "url should not be empty");
			Core::Parser fURL(&URL);
			fURL.Replace('\\', '/');

			const char* HostBegin = strchr(URL.c_str(), ':');
			if (HostBegin != nullptr)
			{
				if (strncmp(HostBegin, "://", 3) != 0)
				{
					char Malformed[4] = { 0, 0, 0, 0 };
					strncpy(Malformed, HostBegin, 3);
					return;
				}
				else
				{
					Protocol = std::string(URL.c_str(), HostBegin);
					HostBegin += 3;
				}
			}
			else
				HostBegin = URL.c_str();

			const char* PathBegin;
			if (HostBegin != URL.c_str())
			{
				const char* AtSymbol = strchr(HostBegin, '@');
				if (AtSymbol)
				{
					std::string LoginPassword;
					LoginPassword = std::string(HostBegin, AtSymbol);
					HostBegin = AtSymbol + 1;

					const char* PasswordPtr = strchr(LoginPassword.c_str(), ':');
					if (PasswordPtr)
					{
						Login = std::string(LoginPassword.c_str(), PasswordPtr);
						Password = std::string(PasswordPtr + 1);
					}
					else
						Login = LoginPassword;
				}

				PathBegin = strchr(HostBegin, '/');
				const char* PortBegin = strchr(HostBegin, ':');
				if (PortBegin != nullptr && (PathBegin == nullptr || PortBegin < PathBegin))
				{
					if (1 != sscanf(PortBegin, ":%d", &Port))
					{
						MakePath();
						return;
					}

					Host = std::string(HostBegin, PortBegin);
					if (nullptr == PathBegin)
					{
						MakePath();
						return;
					}

					++PathBegin;
				}
				else
				{
					Port = -1;
					if (PathBegin == nullptr)
					{
						Host = HostBegin;
						MakePath();
						return;
					}

					Host = std::string(HostBegin, PathBegin);
					++PathBegin;
				}
			}
			else
				PathBegin = URL.c_str();

			std::string PathSegment;
			const char* Parameters = strchr(PathBegin, '?');
			if (Parameters != nullptr)
			{
				PathSegment = std::string(PathBegin, Parameters);
				PathBegin = PathSegment.c_str();

				Core::Parser fParameters(Parameters + 1);
				std::vector<std::string> Array = fParameters.Split('&');
				for (size_t i = 0; i < Array.size(); i++)
				{
					Core::Parser fParameter(Array[i]);
					std::vector<std::string> KeyValue = fParameters.Split('=');
					KeyValue[0] = Compute::Codec::URIDecode(KeyValue[0]);

					if (KeyValue.size() >= 2)
						Query[KeyValue[0]] = Compute::Codec::URIDecode(KeyValue[1]);
					else
						Query[KeyValue[0]] = "";
				}
			}

			const char* FilenameBegin = strrchr(PathBegin, '/');
			if (FilenameBegin != nullptr)
			{
				Path = std::string(PathBegin, ++FilenameBegin);
				size_t ParentPos = std::string::npos;

				while ((ParentPos = Path.find("/../")) != std::string::npos && ParentPos != 0)
				{
					size_t ParentStartPos = Path.rfind('/', ParentPos - 1);
					if (ParentStartPos == std::string::npos)
						ParentStartPos = 0;
					else
						ParentStartPos += 1;

					Path.erase(ParentStartPos, ParentPos - ParentStartPos + 4);
				}
			}
			else
				FilenameBegin = PathBegin;

			const char* ExtensionBegin = strrchr(FilenameBegin, '.');
			if (nullptr != ExtensionBegin)
			{
				Filename = std::string(FilenameBegin, ExtensionBegin);
				Extension = ExtensionBegin + 1;
			}
			else
				Filename = FilenameBegin;

			if (Path.empty() && !Filename.empty())
				MakePath();
			else if (!Path.empty() && Path.front() != '/')
				Path = '/' + Path;
		}
		SourceURL::SourceURL(const SourceURL& Other) noexcept :
			Query(Other.Query), URL(Other.URL), Protocol(Other.Protocol),
			Login(Other.Login), Password(Other.Password), Host(Other.Host),
			Path(Other.Path), Filename(Other.Filename), Extension(Other.Extension), Port(Other.Port)
		{
		}
		SourceURL::SourceURL(SourceURL&& Other) noexcept :
			Query(std::move(Other.Query)), URL(std::move(Other.URL)), Protocol(std::move(Other.Protocol)),
			Login(std::move(Other.Login)), Password(std::move(Other.Password)), Host(std::move(Other.Host)),
			Path(std::move(Other.Path)), Filename(std::move(Other.Filename)), Extension(std::move(Other.Extension)), Port(Other.Port)
		{
		}
		SourceURL& SourceURL::operator= (const SourceURL& Other) noexcept
		{
			if (this == &Other)
				return *this;

			Query = Other.Query;
			URL = Other.URL;
			Protocol = Other.Protocol;
			Login = Other.Login;
			Password = Other.Password;
			Host = Other.Host;
			Path = Other.Path;
			Filename = Other.Filename;
			Extension = Other.Extension;
			Port = Other.Port;

			return *this;
		}
		SourceURL& SourceURL::operator= (SourceURL&& Other) noexcept
		{
			if (this == &Other)
				return *this;

			Query = std::move(Other.Query);
			URL = std::move(Other.URL);
			Protocol = std::move(Other.Protocol);
			Login = std::move(Other.Login);
			Password = std::move(Other.Password);
			Host = std::move(Other.Host);
			Path = std::move(Other.Path);
			Filename = std::move(Other.Filename);
			Extension = std::move(Other.Extension);
			Port = Other.Port;

			return *this;
		}
		void SourceURL::MakePath()
		{
			if (!Path.empty() || Filename.empty())
				return;

			if (Extension.empty())
				Path = Filename.front() == '/' ? Filename : '/' + Filename;
			else
				Path = (Filename.front() == '/' ? Filename : '/' + Filename) + '.' + Extension;
		}

		Socket::Socket() : Device(nullptr), Fd(INVALID_SOCKET), Timeout(0), Income(0), Outcome(0), UserData(nullptr)
		{
		}
		Socket::Socket(socket_t FromFd) : Socket()
		{
			Fd = FromFd;
		}
		int Socket::Accept(Socket* OutConnection, char* OutAddr)
		{
			TH_ASSERT(OutConnection != nullptr, -1, "socket should be set");
            return Accept(&OutConnection->Fd, OutAddr);
		}
		int Socket::Accept(socket_t* OutFd, char* OutAddr)
		{
            TH_ASSERT(OutFd != nullptr, -1, "socket should be set");

            sockaddr Address;
            socket_size_t Length = sizeof(sockaddr);
            *OutFd = accept(Fd, &Address, &Length);
            if (*OutFd == INVALID_SOCKET)
                return -1;

            TH_DEBUG("[net] accept fd %i on %i fd", (int)*OutFd, (int)Fd);
            if (OutAddr != nullptr)
                inet_ntop(Address.sa_family, GetAddressStorage(&Address), OutAddr, INET6_ADDRSTRLEN);
            
            return 0;
		}
		int Socket::AcceptAsync(bool WithAddress, SocketAcceptedCallback&& Callback)
		{
			TH_ASSERT(Callback != nullptr, -1, "callback should be set");

			bool Success = Driver::WhenReadable(this, [this, WithAddress, Callback = std::move(Callback)](SocketPoll Event) mutable
			{
				if (!Packet::IsDone(Event))
					return;
                
                socket_t OutFd = INVALID_SOCKET;
                char OutAddr[INET6_ADDRSTRLEN] = { 0 };
                char* RemoteAddr = (WithAddress ? OutAddr : nullptr);
                
				while (Accept(&OutFd, RemoteAddr) == 0)
				{
					if (!Callback(OutFd, RemoteAddr))
						break;
				}

				AcceptAsync(WithAddress, std::move(Callback));
			});

			return Success ? 0 : -1;
		}
		int Socket::Close(bool Gracefully)
		{
			ClearEvents(false);
			if (Fd == INVALID_SOCKET)
				return -1;
#ifdef TH_HAS_OPENSSL
			if (Device != nullptr)
			{
				SSL_free(Device);
				Device = nullptr;
			}
#endif
			int Error = 1;
			socklen_t Size = sizeof(Error);
			getsockopt(Fd, SOL_SOCKET, SO_ERROR, (char*)&Error, &Size);

			if (Gracefully)
			{
				TH_MEASURE(TH_TIMING_NET);
				int Timeout = 100;
				SetBlocking(true);
				SetSocket(SO_RCVTIMEO, &Timeout, sizeof(int));
				shutdown(Fd, SD_SEND);
				while (recv(Fd, (char*)&Error, 1, 0) > 0)
					TH_MEASURE_LOOP();
			}
			else
				shutdown(Fd, SD_SEND);

			closesocket(Fd);
			TH_DEBUG("[net] sock fd %i closed", (int)Fd);
			Fd = INVALID_SOCKET;
			return 0;
		}
		int Socket::CloseAsync(bool Gracefully, SocketClosedCallback&& Callback)
		{
			if (Fd == INVALID_SOCKET)
			{
				if (Callback)
					Callback();

				return -1;
			}

			ClearEvents(false);
#ifdef TH_HAS_OPENSSL
			if (Device != nullptr)
			{
				SSL_free(Device);
				Device = nullptr;
			}
#endif
			int Error = 1;
			socklen_t Size = sizeof(Error);
			getsockopt(Fd, SOL_SOCKET, SO_ERROR, (char*)&Error, &Size);
			shutdown(Fd, SD_SEND);

			if (Gracefully)
				return TryCloseAsync(std::move(Callback), true);

			closesocket(Fd);
			TH_DEBUG("[net] sock fd %i closed", (int)Fd);
			Fd = INVALID_SOCKET;

			if (Callback)
				Callback();

			return 0;
		}
		int Socket::TryCloseAsync(SocketClosedCallback&& Callback, bool KeepTrying)
		{
			while (KeepTrying)
			{
				char Buffer;
				int Length = Read(&Buffer, 1);
				if (Length == -2)
				{
					Timeout = 500;
					Driver::WhenReadable(this, [this, Callback = std::move(Callback)](SocketPoll Event) mutable
					{
						if (!Packet::IsSkip(Event))
							TryCloseAsync(std::move(Callback), Packet::IsDone(Event));
						else if (Callback)
							Callback();
					});

					return 1;
				}
				else if (Length == -1)
					break;
			}

			ClearEvents(false);
			closesocket(Fd);
			TH_DEBUG("[net] sock fd %i closed", (int)Fd);
			Fd = INVALID_SOCKET;

			if (Callback)
				Callback();
			return 0;
		}
        int64_t Socket::SendFile(FILE* Stream, int64_t Offset, int64_t Size)
        {
            TH_ASSERT(Stream != nullptr, -1, "stream should be set");
            TH_ASSERT(Offset >= 0, -1, "offset should be set and positive");
            TH_ASSERT(Size > 0, -1, "size should be set and greater than zero");
            TH_MEASURE(TH_TIMING_NET);
            auto FromFd = TH_FILENO(Stream);
#ifdef TH_APPLE
            off_t Seek = (off_t)Offset, Length = (off_t)Size;
            int64_t Value = (int64_t)sendfile(FromFd, Fd, Seek, &Length, nullptr, 0);
            Size = Length;
#elif defined(TH_UNIX)
            off_t Seek = (off_t)Offset;
            int64_t Value = (int64_t)sendfile(Fd, FromFd, &Seek, (size_t)Size);
            Size = Value;
#else
			int64_t Value = -3;
			return Value;
#endif
            if (Value < 0 && Size <= 0)
                return GetError((int)Value) == ERRWOULDBLOCK ? -2 : -1;
            
            if (Value != Size)
                Value = Size;
            
            Outcome += Value;
            return Value;
        }
        int64_t Socket::SendFileAsync(FILE* Stream, int64_t Offset, int64_t Size, SocketWrittenCallback&& Callback, int TempBuffer)
        {
            TH_ASSERT(Stream != nullptr, -1, "stream should be set");
            TH_ASSERT(Offset >= 0, -1, "offset should be set and positive");
            TH_ASSERT(Size > 0, -1, "size should be set and greater than zero");
            
            while (Size > 0)
            {
                int64_t Length = SendFile(Stream, Offset, Size);
                if (Length == -2)
                {
                    Driver::WhenWriteable(this, [this, TempBuffer, Stream, Offset, Size, Callback = std::move(Callback)](SocketPoll Event) mutable
                    {
                        if (Packet::IsDone(Event))
                            SendFileAsync(Stream, Offset, Size, std::move(Callback), ++TempBuffer);
                        else if (Callback)
                            Callback(Event);
                    });

                    return -2;
                }
                else if (Length == -1)
                {
                    if (Callback)
                        Callback(SocketPoll::Reset);

                    return -1;
                }
                else if (Length == -3)
                    return -3;
                
                Size -= (int64_t)Length;
                Offset += (int64_t)Length;
            }
            
            if (Callback)
                Callback(TempBuffer != 0 ? SocketPoll::Finish : SocketPoll::FinishSync);

            return (int)Offset;
        }
		int Socket::Write(const char* Buffer, int Size)
		{
			TH_ASSERT(Fd != INVALID_SOCKET, -1, "socket should be valid");
			TH_MEASURE(TH_TIMING_NET);
#ifdef TH_HAS_OPENSSL
			if (Device != nullptr)
			{
				int Value = SSL_write(Device, Buffer, Size);
				if (Value <= 0)
					return GetError(Value) == SSL_ERROR_WANT_WRITE ? -2 : -1;

				Outcome += (int64_t)Value;
				return Value;
			}
#endif
			int Value = (int)send(Fd, Buffer, Size, 0);
			if (Value <= 0)
				return GetError(Value) == ERRWOULDBLOCK ? -2 : -1;

			Outcome += (int64_t)Value;
			return Value;
		}
		int Socket::WriteAsync(const char* Buffer, size_t Size, SocketWrittenCallback&& Callback, char* TempBuffer, size_t TempOffset)
		{
			TH_ASSERT(Buffer != nullptr && Size > 0, -1, "buffer should be set");

            size_t Payload = Size;
            size_t Written = 0;
            
			while (Size > 0)
			{
				int Length = Write(Buffer + TempOffset, (int)Size);
				if (Length == -2)
				{
					if (!TempBuffer)
					{
						TempBuffer = TH_MALLOC(char, Payload);
                        memcpy(TempBuffer, Buffer, Payload);
					}

					Driver::WhenWriteable(this, [this, TempBuffer, TempOffset, Size, Callback = std::move(Callback)](SocketPoll Event) mutable
					{
						if (!Packet::IsDone(Event))
						{
							TH_FREE(TempBuffer);
							if (Callback)
								Callback(Event);
						}
						else
							WriteAsync(TempBuffer, Size, std::move(Callback), TempBuffer, TempOffset);
					});

					return -2;
				}
				else if (Length == -1)
				{
					if (TempBuffer != nullptr)
						TH_FREE(TempBuffer);

					if (Callback)
						Callback(SocketPoll::Reset);

					return -1;
				}

				Size -= (int64_t)Length;
                TempOffset += (int64_t)Length;
                Written += (size_t)Length;
			}

			if (TempBuffer != nullptr)
				TH_FREE(TempBuffer);

			if (Callback)
				Callback(TempBuffer ? SocketPoll::Finish : SocketPoll::FinishSync);

			return (int)Written;
		}
		int Socket::Read(char* Buffer, int Size)
		{
			TH_ASSERT(Fd != INVALID_SOCKET, -1, "socket should be valid");
			TH_ASSERT(Buffer != nullptr && Size > 0, -1, "buffer should be set");
			TH_MEASURE(TH_TIMING_NET);
#ifdef TH_HAS_OPENSSL
			if (Device != nullptr)
			{
				int Value = SSL_read(Device, Buffer, Size);
				if (Value <= 0)
					return GetError(Value) == SSL_ERROR_WANT_READ ? -2 : -1;

				Income += (int64_t)Value;
				return Value;
			}
#endif
			int Value = (int)recv(Fd, Buffer, Size, 0);
			if (Value <= 0)
				return GetError(Value) == ERRWOULDBLOCK ? -2 : -1;

			Income += (int64_t)Value;
			return Value;
		}
		int Socket::Read(char* Buffer, int Size, const SocketReadCallback& Callback)
		{
			TH_ASSERT(Fd != INVALID_SOCKET, -1, "socket should be valid");
			TH_ASSERT(Buffer != nullptr && Size > 0, -1, "buffer should be set");

			int Offset = 0;
			while (Size > 0)
			{
				int Length = Read(Buffer + Offset, Size);
				if (Length == -2)
				{
					if (Callback)
						Callback(SocketPoll::Timeout, nullptr, 0);

					return -2;
				}
				else if (Length == -1)
				{
					if (Callback)
						Callback(SocketPoll::Reset, nullptr, 0);

					return -1;
				}

				if (Callback && !Callback(SocketPoll::Next, Buffer + Offset, (size_t)Length))
					break;

				Size -= Length;
				Offset += Length;
			}

			if (Callback)
				Callback(SocketPoll::FinishSync, nullptr, 0);

			return Offset;
		}
		int Socket::ReadAsync(size_t Size, SocketReadCallback&& Callback, int TempBuffer)
		{
			TH_ASSERT(Fd != INVALID_SOCKET, -1, "socket should be valid");
			TH_ASSERT(Size > 0, -1, "size should be greater than zero");

			char Buffer[TH_BIG_CHUNK_SIZE];
            int Offset = 0;
            
			while (Size > 0)
			{
				int Length = Read(Buffer, (int)(Size > sizeof(Buffer) ? sizeof(Buffer) : Size));
				if (Length == -2)
				{
					Driver::WhenReadable(this, [this, Size, TempBuffer, Callback = std::move(Callback)](SocketPoll Event) mutable
					{
						if (Packet::IsDone(Event))
							ReadAsync(Size, std::move(Callback), ++TempBuffer);
						else if (Callback)
							Callback(Event, nullptr, 0);
					});

					return -2;
				}
				else if (Length == -1)
				{
					if (Callback)
						Callback(SocketPoll::Reset, nullptr, 0);

					return -1;
				}

				Size -= (size_t)Length;
                Offset += Length;
                
				if (Callback && !Callback(SocketPoll::Next, Buffer, (size_t)Length))
					break;
			}

			if (Callback)
				Callback(TempBuffer != 0 ? SocketPoll::Finish : SocketPoll::FinishSync, nullptr, 0);

			return Offset;
		}
		int Socket::ReadUntil(const char* Match, SocketReadCallback&& Callback)
		{
			TH_ASSERT(Fd != INVALID_SOCKET, -1, "socket should be valid");
			TH_ASSERT(Match != nullptr, -1, "match should be set");

			char Buffer[MAX_READ_UNTIL];
			size_t Size = strlen(Match), Offset = 0, Index = 0;
			auto Publish = [&Callback, &Buffer, &Offset](size_t Size)
			{
				Offset = 0;
				return !Callback || Callback(SocketPoll::Next, Buffer, Size);
			};

			TH_ASSERT(Size > 0, -1, "match should not be empty");
			while (true)
			{
				int Length = Read(Buffer + Offset, 1);
				if (Length <= 0)
				{
					if (Offset > 0 && !Publish(Offset))
						return -1;

					if (Callback)
						Callback(SocketPoll::Reset, nullptr, 0);

					return -1;
				}

				if (++Offset >= MAX_READ_UNTIL && !Publish(Offset))
					break;

				if (Match[Index] == Buffer[Offset - 1])
				{
					if (++Index >= Size)
					{
						if (Offset > 0)
							Publish(Offset);
						break;
					}
				}
				else
					Index = 0;
			}

			if (Callback)
				Callback(SocketPoll::FinishSync, nullptr, 0);

			return 0;
		}
		int Socket::ReadUntilAsync(const char* Match, SocketReadCallback&& Callback, char* TempBuffer, size_t TempIndex)
		{
			TH_ASSERT(Fd != INVALID_SOCKET, -1, "socket should be valid");
			TH_ASSERT(Match != nullptr, -1, "match should be set");

			char Buffer[MAX_READ_UNTIL];
			size_t Size = strlen(Match), Offset = 0;
			auto Publish = [&Callback, &Buffer, &Offset](size_t Size)
			{
				Offset = 0;
				return !Callback || Callback(SocketPoll::Next, Buffer, Size);
			};

			TH_ASSERT(Size > 0, -1, "match should not be empty");
			while (true)
			{
				int Length = Read(Buffer + Offset, 1);
				if (Length == -2)
				{
					if (!TempBuffer)
					{
						TempBuffer = TH_MALLOC(char, Size + 1);
						memcpy(TempBuffer, Match, Size);
						TempBuffer[Size] = '\0';
					}

					Driver::WhenReadable(this, [this, TempBuffer, TempIndex, Callback = std::move(Callback)](SocketPoll Event) mutable
					{
						if (!Packet::IsDone(Event))
						{
							TH_FREE(TempBuffer);
							if (Callback)
								Callback(Event, nullptr, 0);
						}
						else
							ReadUntilAsync(TempBuffer, std::move(Callback), TempBuffer, TempIndex);
					});

					return -2;
				}
				else if (Length == -1)
				{
					if (TempBuffer != nullptr)
						TH_FREE(TempBuffer);

					if (Offset > 0 && !Publish(Offset))
						return -1;

					if (Callback)
						Callback(SocketPoll::Reset, nullptr, 0);

					return -1;
				}

				if (++Offset >= MAX_READ_UNTIL && !Publish(Offset))
					break;

				if (Match[TempIndex] == Buffer[Offset - 1])
				{
					if (++TempIndex >= Size)
					{
						if (Offset > 0)
							Publish(Offset);
						break;
					}
				}
				else
					TempIndex = 0;
			}

			if (TempBuffer != nullptr)
				TH_FREE(TempBuffer);

			if (Callback)
				Callback(TempBuffer ? SocketPoll::Finish : SocketPoll::FinishSync, nullptr, 0);

			return 0;
		}
		int Socket::Connect(Address* Address, uint64_t Timeout)
		{
			TH_ASSERT(Address && Address->Usable, -1, "address should be set and usable");
			TH_MEASURE(TH_TIMING_NET);
			TH_DEBUG("[net] connect fd %i", (int)Fd);
			return connect(Fd, Address->Usable->ai_addr, (int)Address->Usable->ai_addrlen);
		}
		int Socket::ConnectAsync(Address* Address, SocketConnectedCallback&& Callback)
		{
			TH_ASSERT(Address && Address->Usable, -1, "address should be set and usable");
			TH_MEASURE(TH_TIMING_NET);
			TH_DEBUG("[net] connect fd %i", (int)Fd);
			int Value = connect(Fd, Address->Usable->ai_addr, (int)Address->Usable->ai_addrlen);
			if (Value == 0)
			{
				if (Callback)
					Callback(0);

				return 0;
			}

			int Code = GetError(Value);
			if (Code != ERRWOULDBLOCK && Code != EINPROGRESS)
			{
				if (Callback)
					Callback(Value);

				return Value;
			}

			if (!Callback)
				return -2;

			Driver::WhenWriteable(this, [this, Callback = std::move(Callback)](SocketPoll Event) mutable
			{
				if (Packet::IsDone(Event))
					Callback(0);
				else if (Packet::IsTimeout(Event))
					Callback(ETIMEDOUT);
				else
					Callback(ECONNREFUSED);
			});

			return -2;
		}
		int Socket::Open(addrinfo* Good)
		{
			TH_ASSERT(Good != nullptr, -1, "addrinfo should be set");
			TH_MEASURE(TH_TIMING_NET);
			TH_DEBUG("[net] assign fd");

			Fd = socket(Good->ai_family, Good->ai_socktype, Good->ai_protocol);
			if (Fd == INVALID_SOCKET)
				return -1;

			int Option = 1;
			setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, (char*)&Option, sizeof(Option));
			return 0;
		}
		int Socket::Secure(ssl_ctx_st* Context, const char* Hostname)
		{
#ifdef TH_HAS_OPENSSL
			TH_MEASURE(TH_TIMING_NET);
			if (Device != nullptr)
				SSL_free(Device);

			Device = SSL_new(Context);
#ifndef OPENSSL_NO_TLSEXT
			if (Device != nullptr && Hostname != nullptr)
				SSL_set_tlsext_host_name(Device, Hostname);
#endif
#endif
			if (!Device)
				return -1;

			return 0;
		}
		int Socket::Bind(Address* Address)
		{
			TH_ASSERT(Address && Address->Usable, -1, "address should be set and usable");
			TH_MEASURE(TH_TIMING_NET);
			TH_DEBUG("[net] bind fd %i", (int)Fd);
			return bind(Fd, Address->Usable->ai_addr, (int)Address->Usable->ai_addrlen);
		}
		int Socket::Listen(int Backlog)
		{
			TH_MEASURE(TH_TIMING_NET);
			TH_DEBUG("[net] listen fd %i", (int)Fd);
			return listen(Fd, Backlog);
		}
		int Socket::ClearEvents(bool Gracefully)
		{
			TH_MEASURE(TH_TIMING_NET);
			if (Gracefully)
				Driver::CancelEvents(this, SocketPoll::Reset);
			else
				Driver::ClearEvents(this);
			return 0;
		}
		int Socket::SetFd(socket_t NewFd, bool Gracefully)
		{
			TH_MEASURE(TH_TIMING_NET);
			int Result = Gracefully ? ClearEvents(false) : 0;
			Fd = NewFd;
			return Result;
		}
		int Socket::SetCloseOnExec()
		{
#if defined(_WIN32)
			return 0;
#else
			return fcntl(Fd, F_SETFD, FD_CLOEXEC);
#endif
		}
		int Socket::SetTimeWait(int Timeout)
		{
			linger Linger;
			Linger.l_onoff = (Timeout >= 0 ? 1 : 0);
			Linger.l_linger = Timeout;

			return setsockopt(Fd, SOL_SOCKET, SO_LINGER, (char*)&Linger, sizeof(Linger));
		}
		int Socket::SetSocket(int Option, void* Value, int Size)
		{
			return ::setsockopt(Fd, SOL_SOCKET, Option, (const char*)Value, Size);
		}
		int Socket::SetAny(int Level, int Option, void* Value, int Size)
		{
			return ::setsockopt(Fd, Level, Option, (const char*)Value, Size);
		}
		int Socket::SetAnyFlag(int Level, int Option, int Value)
		{
			return SetAny(Level, Option, (void*)&Value, sizeof(int));
		}
		int Socket::SetSocketFlag(int Option, int Value)
		{
			return SetSocket(Option, (void*)&Value, sizeof(int));
		}
		int Socket::SetBlocking(bool Enabled)
		{
#ifdef TH_MICROSOFT
			unsigned long Mode = (Enabled ? 0 : 1);
			return ioctlsocket(Fd, (long)FIONBIO, &Mode);
#else
			int Flags = fcntl(Fd, F_GETFL, 0);
			if (Flags == -1)
				return -1;

			Flags = Enabled ? (Flags & ~O_NONBLOCK) : (Flags | O_NONBLOCK);
			return (fcntl(Fd, F_SETFL, Flags) == 0);
#endif
		}
		int Socket::SetNoDelay(bool Enabled)
		{
			return SetAnyFlag(IPPROTO_TCP, TCP_NODELAY, (Enabled ? 1 : 0));
		}
		int Socket::SetKeepAlive(bool Enabled)
		{
			return SetSocketFlag(SO_KEEPALIVE, (Enabled ? 1 : 0));
		}
		int Socket::SetTimeout(int Timeout)
		{
#ifdef TH_MICROSOFT
			DWORD Time = (DWORD)Timeout;
#else
			struct timeval Time;
			memset(&Time, 0, sizeof(Time));
			Time.tv_sec = Timeout / 1000;
			Time.tv_usec = (Timeout * 1000) % 1000000;
#endif
			int Range1 = setsockopt(Fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&Time, sizeof(Time));
			int Range2 = setsockopt(Fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&Time, sizeof(Time));
			return (Range1 == 0 || Range2 == 0);
		}
		int Socket::GetError(int Result)
		{
#ifdef TH_HAS_OPENSSL
			if (!Device)
				return ERRNO;

			return SSL_get_error(Device, Result);
#else
			return ERRNO;
#endif
		}
		int Socket::GetAnyFlag(int Level, int Option, int* Value)
		{
			return GetAny(Level, Option, (char*)Value, nullptr);
		}
		int Socket::GetAny(int Level, int Option, void* Value, int* Size)
		{
			socket_size_t Length = (socket_size_t)Level;
			int Result = ::getsockopt(Fd, Level, Option, (char*)Value, &Length);
			if (Size != nullptr)
				*Size = (int)Length;

			return Result;
		}
		int Socket::GetSocket(int Option, void* Value, int* Size)
		{
			return GetAny(SOL_SOCKET, Option, Value, Size);
		}
		int Socket::GetSocketFlag(int Option, int* Value)
		{
			int Size = sizeof(int);
			return GetSocket(Option, (void*)Value, &Size);
		}
		int Socket::GetPort()
		{
			struct sockaddr_storage Address;
			socket_size_t Size = sizeof(Address);
			if (getsockname(Fd, reinterpret_cast<struct sockaddr*>(&Address), &Size) == -1)
				return -1;

			if (Address.ss_family == AF_INET)
				return ntohs(reinterpret_cast<struct sockaddr_in*>(&Address)->sin_port);

			if (Address.ss_family == AF_INET6)
				return ntohs(reinterpret_cast<struct sockaddr_in6*>(&Address)->sin6_port);

			return -1;
		}
		socket_t Socket::GetFd()
		{
			return Fd;
		}
		ssl_st* Socket::GetDevice()
		{
			return Device;
		}
		bool Socket::IsValid()
		{
			return Fd != INVALID_SOCKET;
		}
		bool Socket::IsPendingForRead()
		{
			return Driver::IsAwaitingReadable(this);
		}
		bool Socket::IsPendingForWrite()
		{
			return Driver::IsAwaitingWriteable(this);
		}
		bool Socket::IsPending()
		{
			return Driver::IsAwaitingEvents(this);
		}
		std::string Socket::GetRemoteAddress()
		{
			struct sockaddr_storage Address;
			socklen_t Size = sizeof(Address);

			if (!getpeername(Fd, (struct sockaddr*)&Address, &Size))
			{
				char Buffer[NI_MAXHOST];
				if (!getnameinfo((struct sockaddr*)&Address, Size, Buffer, sizeof(Buffer), nullptr, 0, NI_NUMERICHOST))
					return Buffer;
			}

			return std::string();
		}
		std::string Socket::GetLocalAddress()
		{
			char Buffer[TH_CHUNK_SIZE];
			if (gethostname(Buffer, sizeof(Buffer)) == SOCKET_ERROR)
				return "127.0.0.1";

			struct addrinfo Hints = { };
			Hints.ai_family = AF_INET;
			Hints.ai_socktype = SOCK_STREAM;

			struct sockaddr_in Address = { };
			struct addrinfo* Results = nullptr;

			bool Success = (getaddrinfo(Buffer, nullptr, &Hints, &Results) == 0);
			if (Success)
			{
				if (Results != nullptr)
					memcpy(&Address, Results->ai_addr, sizeof(Address));
			}

			if (Results != nullptr)
				freeaddrinfo(Results);

			if (!Success)
				return "127.0.0.1";

			char Result[INET_ADDRSTRLEN];
			if (!inet_ntop(AF_INET, &(Address.sin_addr), Result, INET_ADDRSTRLEN))
				return "127.0.0.1";

			return Result;
		}
		std::string Socket::GetAddress(addrinfo* Info)
		{
            TH_ASSERT(Info != nullptr, std::string(), "address info should be set");
			char Buffer[INET6_ADDRSTRLEN];
			inet_ntop(Info->ai_family, GetAddressStorage(Info->ai_addr), Buffer, sizeof(Buffer));
			return Buffer;
		}
		std::string Socket::GetAddress(sockaddr* Info)
		{
            TH_ASSERT(Info != nullptr, std::string(), "socket address should be set");
			char Buffer[INET6_ADDRSTRLEN];
			inet_ntop(Info->sa_family, GetAddressStorage(Info), Buffer, sizeof(Buffer));
			return Buffer;
		}
		int Socket::GetAddressFamily(const char* Address)
		{
			TH_ASSERT(Address != nullptr, AF_UNSPEC, "address should be set");

			struct addrinfo Hints;
			memset(&Hints, 0, sizeof(Hints));
			Hints.ai_family = AF_UNSPEC;
			Hints.ai_socktype = SOCK_STREAM;
			Hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;

			struct addrinfo* Result;
			if (getaddrinfo(Address, 0, &Hints, &Result) != 0)
				return AF_UNSPEC;

			int Family = Result->ai_family;
			freeaddrinfo(Result);

			return Family;
		}

		SocketConnection::~SocketConnection()
		{
			TH_DELETE(Socket, Stream);
		}
		bool SocketConnection::Finish()
		{
			return false;
		}
		bool SocketConnection::Finish(int)
		{
			return Finish();
		}
		bool SocketConnection::Error(int StatusCode, const char* ErrorMessage, ...)
		{
			char Buffer[TH_BIG_CHUNK_SIZE];
			if (Info.Close)
				return Finish();

			va_list Args;
			va_start(Args, ErrorMessage);
			int Count = vsnprintf(Buffer, sizeof(Buffer), ErrorMessage, Args);
			va_end(Args);

			Info.Message.assign(Buffer, Count);
			return Finish(StatusCode);
		}
		bool SocketConnection::EncryptionInfo(Certificate* Output)
		{
#ifdef TH_HAS_OPENSSL
			TH_ASSERT(Output != nullptr, false, "certificate should be set");
			TH_MEASURE(TH_TIMING_NET);

			X509* Certificate = SSL_get_peer_certificate(Stream->GetDevice());
			if (!Certificate)
				return false;

			const EVP_MD* Digest = EVP_get_digestbyname("sha1");
			X509_NAME* Subject = X509_get_subject_name(Certificate);
			X509_NAME* Issuer = X509_get_issuer_name(Certificate);
			ASN1_INTEGER* Serial = X509_get_serialNumber(Certificate);
            
			char SubjectBuffer[TH_CHUNK_SIZE];
			X509_NAME_oneline(Subject, SubjectBuffer, (int)sizeof(SubjectBuffer));

			char IssuerBuffer[TH_CHUNK_SIZE], SerialBuffer[TH_CHUNK_SIZE];
			X509_NAME_oneline(Issuer, IssuerBuffer, (int)sizeof(IssuerBuffer));

			unsigned char Buffer[256];
			int Length = i2d_ASN1_INTEGER(Serial, nullptr);

			if (Length > 0 && (unsigned)Length < (unsigned)sizeof(Buffer))
			{
				unsigned char* Pointer = Buffer;
				int Size = i2d_ASN1_INTEGER(Serial, &Pointer);

				if (!Compute::Codec::HexToString(Buffer, (uint64_t)Size, SerialBuffer, sizeof(SerialBuffer)))
					*SerialBuffer = '\0';
			}
			else
				*SerialBuffer = '\0';
#if OPENSSL_VERSION_MAJOR < 3
			unsigned int Size = 0;
			ASN1_digest((int (*)(void*, unsigned char**))i2d_X509, Digest, (char*)Certificate, Buffer, &Size);
            
			char FingerBuffer[TH_CHUNK_SIZE];
			if (!Compute::Codec::HexToString(Buffer, (uint64_t)Size, FingerBuffer, sizeof(FingerBuffer)))
				*FingerBuffer = '\0';
            
            Output->Finger = FingerBuffer;
#endif
			Output->Subject = SubjectBuffer;
			Output->Issuer = IssuerBuffer;
			Output->Serial = SerialBuffer;

			X509_free(Certificate);
			return true;
#else
			return false;
#endif
		}
		bool SocketConnection::Break()
		{
			return Finish(-1);
		}
		void SocketConnection::Reset(bool Fully)
		{
			if (Fully)
			{
				Info.Message.clear();
				Info.Start = 0;
				Info.Finish = 0;
				Info.Timeout = 0;
				Info.KeepAlive = 1;
				Info.Close = false;
			}

			if (Stream != nullptr)
			{
				Stream->Income = 0;
				Stream->Outcome = 0;
			}
		}

		Host& SocketRouter::Listen(const std::string& Hostname, int Port, bool Secure)
		{
			return Listen("*", Hostname, Port, Secure);
		}
		Host& SocketRouter::Listen(const std::string& Pattern, const std::string& Hostname, int Port, bool Secure)
		{
			Host& Listener = Listeners[Pattern.empty() ? "*" : Pattern];
			Listener.Hostname = Hostname;
			Listener.Port = Port;
			Listener.Secure = Secure;
			return Listener;
		}
		SocketRouter::~SocketRouter()
		{
#ifdef TH_HAS_OPENSSL
			for (auto It = Certificates.begin(); It != Certificates.end(); It++)
			{
				if (!It->second.Context)
					continue;

				SSL_CTX_free(It->second.Context);
				It->second.Context = nullptr;
			}
#endif
		}

		EpollHandle::EpollHandle(size_t NewArraySize) : ArraySize(NewArraySize)
		{
			TH_ASSERT_V(ArraySize > 0, "array size should be greater than zero");
#ifdef TH_APPLE
			Handle = kqueue();
			Array = TH_MALLOC(struct kevent, sizeof(struct kevent) * ArraySize);
#else
			Handle = epoll_create(1);
			Array = TH_MALLOC(epoll_event, sizeof(epoll_event) * ArraySize);
#endif
		}
		EpollHandle::~EpollHandle()
		{
			if (Handle != INVALID_EPOLL)
				epoll_close(Handle);

			if (Array != nullptr)
				TH_FREE(Array);
		}
		bool EpollHandle::Add(Socket* Fd, bool Readable, bool Writeable)
		{
			TH_ASSERT(Handle != INVALID_EPOLL, false, "epoll should be initialized");
			TH_ASSERT(Fd != nullptr && Fd->Fd != INVALID_SOCKET, false, "socket should be set and valid");

#ifdef TH_APPLE
			struct kevent Event;
			int Result1 = 1;
			if (Readable)
			{
				EV_SET(&Event, Fd->Fd, EVFILT_READ, EV_ADD, 0, 0, (void*)Fd);
				Result1 = kevent(Handle, &Event, 1, nullptr, 0, nullptr);
			}

			int Result2 = 1;
			if (Writeable)
			{
				EV_SET(&Event, Fd->Fd, EVFILT_WRITE, EV_ADD, 0, 0, (void*)Fd);
				Result2 = kevent(Handle, &Event, 1, nullptr, 0, nullptr);
			}

			return Result1 == 1 && Result2 == 1;
#else
			epoll_event Event;
			Event.data.ptr = (void*)Fd;
			Event.events = EPOLLRDHUP;

			if (Readable)
				Event.events |= EPOLLIN;

			if (Writeable)
				Event.events |= EPOLLOUT;

			return epoll_ctl(Handle, EPOLL_CTL_ADD, Fd->Fd, &Event) == 0;
#endif
		}
		bool EpollHandle::Update(Socket* Fd, bool Readable, bool Writeable)
		{
			TH_ASSERT(Handle != INVALID_EPOLL, false, "epoll should be initialized");
			TH_ASSERT(Fd != nullptr && Fd->Fd != INVALID_SOCKET, false, "socket should be set and valid");

#ifdef TH_APPLE
			struct kevent Event;
			int Result1 = 1;
			if (Readable)
			{
				EV_SET(&Event, Fd->Fd, EVFILT_READ, EV_ADD, 0, 0, (void*)Fd);
				Result1 = kevent(Handle, &Event, 1, nullptr, 0, nullptr);
			}

			int Result2 = 1;
			if (Writeable)
			{
				EV_SET(&Event, Fd->Fd, EVFILT_WRITE, EV_ADD, 0, 0, (void*)Fd);
				Result2 = kevent(Handle, &Event, 1, nullptr, 0, nullptr);
			}

			return Result1 == 1 && Result2 == 1;
#else
			epoll_event Event;
			Event.data.ptr = (void*)Fd;
			Event.events = EPOLLRDHUP;

			if (Readable)
				Event.events |= EPOLLIN;

			if (Writeable)
				Event.events |= EPOLLOUT;

			return epoll_ctl(Handle, EPOLL_CTL_MOD, Fd->Fd, &Event) == 0;
#endif
		}
		bool EpollHandle::Remove(Socket* Fd, bool Readable, bool Writeable)
		{
			TH_ASSERT(Handle != INVALID_EPOLL, false, "epoll should be initialized");
			TH_ASSERT(Fd != nullptr && Fd->Fd != INVALID_SOCKET, false, "socket should be set and valid");

#ifdef TH_APPLE
			struct kevent Event;
			int Result1 = 1;
			if (Readable)
			{
				EV_SET(&Event, Fd->Fd, EVFILT_READ, EV_DELETE, 0, 0, (void*)nullptr);
				Result1 = kevent(Handle, &Event, 1, nullptr, 0, nullptr);
			}

			int Result2 = 1;
			if (Writeable)
			{
				EV_SET(&Event, Fd->Fd, EVFILT_WRITE, EV_DELETE, 0, 0, (void*)nullptr);
				Result2 = kevent(Handle, &Event, 1, nullptr, 0, nullptr);
			}

			return Result1 == 1 && Result2 == 1;
#else
			epoll_event Event;
			Event.data.ptr = (void*)Fd;
			Event.events = EPOLLRDHUP;

			if (Readable)
				Event.events |= EPOLLIN;

			if (Writeable)
				Event.events |= EPOLLOUT;

			return epoll_ctl(Handle, EPOLL_CTL_DEL, Fd->Fd, &Event) == 0;
#endif
		}
		int EpollHandle::Wait(EpollFd* Data, size_t DataSize, uint64_t Timeout)
		{
			TH_ASSERT(ArraySize <= DataSize, -1, "epollfd array should be less than or equal to internal events buffer");
#ifdef TH_APPLE
			struct timespec Wait;
			Wait.tv_sec = (int)Timeout / 1000;
			Wait.tv_nsec = ((int)Timeout % 1000) * 1000000;

			struct kevent* Events = (struct kevent*)Array;
			int Count = kevent(Handle, nullptr, 0, Events, (int)DataSize, &Wait);
#else
			epoll_event* Events = (epoll_event*)Array;
			int Count = epoll_wait(Handle, Events, (int)DataSize, (int)Timeout);
#endif
			if (Count <= 0)
				return Count;

			size_t Offset = 0;
			for (auto It = Events; It != Events + Count; It++)
			{
				auto& Fd = Data[Offset++];
#ifdef TH_APPLE
				Fd.Base = (Socket*)It->udata;
				Fd.Readable = (It->filter == EVFILT_READ);
				Fd.Writeable = (It->filter == EVFILT_WRITE);
				Fd.Closed = (It->flags & EV_EOF);
#else
				Fd.Base = (Socket*)It->data.ptr;
				Fd.Readable = (It->events & EPOLLIN);
				Fd.Writeable = (It->events & EPOLLOUT);
				Fd.Closed = (It->events & EPOLLHUP || It->events & EPOLLRDHUP || It->events & EPOLLERR);
#endif
			}

			return Count;
		}

		void DNS::Release()
		{
			Exclusive.lock();
			for (auto& Item : Names)
			{
				Item.second.second->Free();
				TH_DELETE(Address, Item.second.second);
			}
			Names.clear();
			Exclusive.unlock();
		}
		std::string DNS::FindNameFromAddress(const std::string& Host, const std::string& Service)
		{
			TH_ASSERT(!Host.empty(), std::string(), "ip address should not be empty");
			TH_ASSERT(!Service.empty(), std::string(), "port should be greater than zero");
			TH_MEASURE(TH_TIMING_NET * 3);

			struct sockaddr_storage Storage;
			int Port = Core::Parser(&Service).ToInt();
			int Family = Socket::GetAddressFamily(Host.c_str());
			int Result = -1;

			if (Family == AF_INET)
			{
				auto* Base = reinterpret_cast<struct sockaddr_in*>(&Storage);
				Result = inet_pton(Family, Host.c_str(), &Base->sin_addr.s_addr);
				Base->sin_family = Family;
				Base->sin_port = htons(Port);
			}
			else if (Family == AF_INET6)
			{
				auto* Base = reinterpret_cast<struct sockaddr_in6*>(&Storage);
				Result = inet_pton(Family, Host.c_str(), &Base->sin6_addr);
				Base->sin6_family = Family;
				Base->sin6_port = htons(Port);
			}

			if (Result == -1)
			{
				TH_ERR("[dns] cannot reverse resolve dns for identity %s:%i\n\tinvalid address", Host.c_str(), Service.c_str());
				return std::string();
			}

			char Hostname[NI_MAXHOST], ServiceName[NI_MAXSERV];
			if (getnameinfo((struct sockaddr*)&Storage, sizeof(struct sockaddr), Hostname, NI_MAXHOST, ServiceName, NI_MAXSERV, NI_NUMERICSERV) != 0)
			{
				TH_ERR("[dns] cannot reverse resolve dns for identity %s:%i", Host.c_str(), Service.c_str());
				return std::string();
			}

			TH_DEBUG("[net] dns reverse resolved for identity %s:%i\n\thost %s:%s is used", Host.c_str(), Service.c_str(), Hostname, ServiceName);
			return Hostname;
		}
		Address* DNS::FindAddressFromName(const std::string& Host, const std::string& Service, DNSType DNS, SocketProtocol Proto, SocketType Type)
		{
			TH_ASSERT(!Host.empty(), nullptr, "host should not be empty");
			TH_ASSERT(!Service.empty(), nullptr, "service should not be empty");
			TH_MEASURE(TH_TIMING_NET * 3);

			struct addrinfo Hints;
			memset(&Hints, 0, sizeof(struct addrinfo));
			Hints.ai_family = AF_UNSPEC;

			std::string XProto;
			switch (Proto)
			{
				case SocketProtocol::IP:
					Hints.ai_protocol = IPPROTO_IP;
					XProto = "ip";
					break;
				case SocketProtocol::ICMP:
					Hints.ai_protocol = IPPROTO_ICMP;
					XProto = "icmp";
					break;
				case SocketProtocol::TCP:
					Hints.ai_protocol = IPPROTO_TCP;
					XProto = "tcp";
					break;
				case SocketProtocol::UDP:
					Hints.ai_protocol = IPPROTO_UDP;
					XProto = "udp";
					break;
				case SocketProtocol::RAW:
					Hints.ai_protocol = IPPROTO_RAW;
					XProto = "raw";
					break;
				default:
					XProto = "none";
					break;
			}

			std::string XType;
			switch (Type)
			{
				case SocketType::Datagram:
					Hints.ai_socktype = SOCK_DGRAM;
					XType = "dgram";
					break;
				case SocketType::Raw:
					Hints.ai_socktype = SOCK_RAW;
					XType = "raw";
					break;
				case SocketType::Reliably_Delivered_Message:
					Hints.ai_socktype = SOCK_RDM;
					XType = "rdm";
					break;
				case SocketType::Sequence_Packet_Stream:
					Hints.ai_socktype = SOCK_SEQPACKET;
					XType = "seqpacket";
					break;
				case SocketType::Stream:
				default:
					Hints.ai_socktype = SOCK_STREAM;
					XType = "stream";
					break;
			}

			int64_t Time = time(nullptr);
			std::string Identity = XProto + '_' + XType + '@' + Host + ':' + Service;
			{
				std::unique_lock Unique(Exclusive);
				auto It = Names.find(Identity);
				if (It != Names.end() && It->second.first > Time)
				{
					Address* Result = It->second.second;
					Exclusive.unlock();
					return Result;
				}
			}

			struct addrinfo* Addresses = nullptr;
			if (getaddrinfo(Host.c_str(), Service.c_str(), &Hints, &Addresses) != 0)
			{
				TH_ERR("[dns] cannot resolve dns for identity %s", Identity.c_str());
				return nullptr;
			}

			struct addrinfo* Good = nullptr;
			std::unordered_map<socket_t, addrinfo*> Hosts;
			for (auto It = Addresses; It != nullptr; It = It->ai_next)
			{
				socket_t Connection = socket(It->ai_family, It->ai_socktype, It->ai_protocol);
				if (Connection == INVALID_SOCKET)
					continue;

				TH_DEBUG("[net] open dns fd %i", (int)Connection);
				if (DNS == DNSType::Connect)
				{
					Hosts[Connection] = It;
					continue;
				}

				Good = It;
				closesocket(Connection);
				TH_DEBUG("[net] close dns fd %i", (int)Connection);
				break;
			}

			if (DNS == DNSType::Connect)
				Good = TryConnectDNS(Hosts, CONNECT_TIMEOUT);

			for (auto& Host : Hosts)
			{
				closesocket(Host.first);
				TH_DEBUG("[net] close dns fd %i", (int)Host.first);
			}

			if (!Good)
			{
				freeaddrinfo(Addresses);
				TH_ERR("[dns] cannot resolve dns for identity %s", Identity.c_str());
				return nullptr;
			}

			Address* Result = TH_NEW(Address);
			Result->Pool = Addresses;
			Result->Usable = Good;

			TH_DEBUG("[net] dns resolved for identity %s\n\taddress %s is used", Identity.c_str(), Socket::GetAddress(Good).c_str());

			std::unique_lock Unique(Exclusive);
			auto It = Names.find(Identity);
			if (It != Names.end())
			{
				Result->Free();
				TH_DELETE(Address, Result);
				It->second.first = Time + DNS_TIMEOUT;
				Result = It->second.second;
			}
			else
				Names[Identity] = std::make_pair(Time + DNS_TIMEOUT, Result);

			return Result;
		}
		std::unordered_map<std::string, std::pair<int64_t, Address*>> DNS::Names;
		std::mutex DNS::Exclusive;

		int Utils::Poll(pollfd* Fd, int FdCount, int Timeout)
		{
			TH_ASSERT(Fd != nullptr, -1, "poll should be set");
#if defined(TH_MICROSOFT)
			return WSAPoll(Fd, FdCount, Timeout);
#else
			return poll(Fd, FdCount, Timeout);
#endif
		}
		int64_t Utils::Clock()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}

		void Driver::Create(uint64_t DispatchTimeout, size_t MaxEvents)
		{
			TH_ASSERT_V(MaxEvents > 0, "array size should be greater than zero");
			TH_DELETE(EpollHandle, Handle);

			using Map = Core::Mapping<std::map<std::chrono::microseconds, Socket*>>;
			if (!Timeouts)
				Timeouts = TH_NEW(Map);

			if (!Fds)
				Fds = TH_NEW(std::vector<EpollFd>);
			
			DefaultTimeout = DispatchTimeout;
			Handle = TH_NEW(EpollHandle, (int)MaxEvents);
			Fds->resize(MaxEvents);
		}
		void Driver::Release()
		{
			if (Timeouts != nullptr)
			{
				TH_DELETE(Mapping, Timeouts);
				Timeouts = nullptr;
			}

			if (Fds != nullptr)
			{
				TH_DELETE(vector, Fds);
				Fds = nullptr;
			}

			if (Handle != nullptr)
			{
				TH_DELETE(EpollHandle, Handle);
				Handle = nullptr;
			}

			DNS::Release();
		}
		void Driver::SetActive(bool Active)
		{
			if (Active)
				TryListen();
			else
				TryUnlisten();
		}
		int Driver::Dispatch(uint64_t EventTimeout)
		{
			TH_ASSERT(Handle != nullptr && Timeouts != nullptr, -1, "driver should be initialized");
			int Count = Handle->Wait(Fds->data(), Fds->size(), EventTimeout);
			if (Count < 0)
				return Count;

			TH_MEASURE(EventTimeout + TH_TIMING_IO);
			size_t Size = (size_t)Count, Cleanups = 0;
			auto Time = Core::Schedule::GetClock();

			for (size_t i = 0; i < Size; i++)
			{
				if (!DispatchEvents(Fds->at(i), Time))
					++Cleanups;
			}

			TH_MEASURE(TH_TIMING_IO);
			if (Timeouts->Map.empty())
				return Count;

			std::unique_lock Unique(Exclusive);
			while (!Timeouts->Map.empty())
			{
				auto It = Timeouts->Map.begin();
				if (It->first > Time)
					break;

				TH_DEBUG("[net] sock timeout on fd %i", (int)It->second->Fd);
				CancelEvents(It->second, SocketPoll::Timeout, false);
				Timeouts->Map.erase(It);
			}

			return Count;
		}
		bool Driver::DispatchEvents(EpollFd& Fd, const std::chrono::microseconds& Time)
		{
			TH_ASSERT(Fd.Base != nullptr, false, "no socket is connected to epoll fd");
			if (Fd.Closed)
			{
				TH_DEBUG("[net] sock reset on fd %i", (int)Fd.Base->Fd);
				CancelEvents(Fd.Base, SocketPoll::Reset);
				return false;
			}

			if (!Fd.Readable && !Fd.Writeable)
			{
				ClearEvents(Fd.Base);
				return false;
			}

			Exclusive.lock();
			bool Exists = ((!Fd.Readable && Fd.Base->Events.Readable) || (!Fd.Writeable && Fd.Base->Events.Writeable));
			auto WhenReadable = std::move(Fd.Base->Events.ReadCallback);
			auto WhenWriteable = std::move(Fd.Base->Events.WriteCallback);

			if (Exists)
			{
				if (Fd.Base->Timeout > 0)
					UpdateTimeout(Fd.Base, Time + std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::milliseconds(Fd.Base->Timeout)));

				if (!Fd.Readable)
					Fd.Base->Events.ReadCallback = std::move(WhenReadable);
				else if (!Fd.Writeable)
					Fd.Base->Events.WriteCallback = std::move(WhenWriteable);
			}
			else
			{
				if (Fd.Base->Timeout > 0 && Fd.Base->Events.ExpiresAt > std::chrono::microseconds(0))
					RemoveTimeout(Fd.Base);

				CancelEvents(Fd.Base, SocketPoll::Finish, false);
			}
			Exclusive.unlock();

			if (Fd.Readable && Fd.Writeable)
			{
				Core::Schedule::Get()->SetTask([WhenReadable = std::move(WhenReadable), WhenWriteable = std::move(WhenWriteable)]() mutable
				{
					if (WhenWriteable)
						WhenWriteable(SocketPoll::Finish);

					if (WhenReadable)
						WhenReadable(SocketPoll::Finish);
				}, Core::Difficulty::Light);
			}
			else if (Fd.Readable)
			{
				Core::Schedule::Get()->SetTask([WhenReadable = std::move(WhenReadable)]() mutable
				{
					if (WhenReadable)
						WhenReadable(SocketPoll::Finish);
				}, Core::Difficulty::Light);
			}
			else if (Fd.Writeable)
			{
				Core::Schedule::Get()->SetTask([WhenWriteable = std::move(WhenWriteable)]() mutable
				{
					if (WhenWriteable)
						WhenWriteable(SocketPoll::Finish);
				}, Core::Difficulty::Light);
			}

			return Exists;
		}
		bool Driver::WhenReadable(Socket* Value, PollEventCallback&& WhenReady)
		{
			TH_ASSERT(Handle != nullptr, false, "driver should be initialized");
			TH_ASSERT(Value != nullptr && Value->Fd != INVALID_SOCKET, false, "socket should be set and valid");

			return WhenEvents(Value, true, false, std::move(WhenReady), nullptr);
		}
		bool Driver::WhenWriteable(Socket* Value, PollEventCallback&& WhenReady)
		{
			TH_ASSERT(Handle != nullptr, false, "driver should be initialized");
			TH_ASSERT(Value != nullptr && Value->Fd != INVALID_SOCKET, false, "socket should be set and valid");

			return WhenEvents(Value, false, true, nullptr, std::move(WhenReady));
		}
		bool Driver::CancelEvents(Socket* Value, SocketPoll Event, bool Safely)
		{
			TH_ASSERT(Handle != nullptr, false, "driver should be initialized");
			TH_ASSERT(Value != nullptr && Value->Fd != INVALID_SOCKET, false, "socket should be set and valid");

			if (Safely)
				Exclusive.lock();

			auto WhenReadable = std::move(Value->Events.ReadCallback);
			auto WhenWriteable = std::move(Value->Events.WriteCallback);
			bool Success = Handle->Remove(Value, true, true);
			Value->Events.Readable = false;
			Value->Events.Writeable = false;
			
			if (Safely)
			{
				if (Value->Timeout > 0 && Value->Events.ExpiresAt > std::chrono::microseconds(0))
					RemoveTimeout(Value);
				Exclusive.unlock();
			}

			if (!Packet::IsDone(Event) && (WhenWriteable || WhenReadable))
			{
				Core::Schedule::Get()->SetTask([Event, WhenReadable = std::move(WhenReadable), WhenWriteable = std::move(WhenWriteable)]() mutable
				{
					if (WhenWriteable)
						WhenWriteable(Event);

					if (WhenReadable)
						WhenReadable(Event);
				}, Core::Difficulty::Light);
			}

			return Success;
		}
		bool Driver::ClearEvents(Socket* Value)
		{
			return CancelEvents(Value, SocketPoll::Finish);
		}
		bool Driver::IsAwaitingEvents(Socket* Value)
		{
			TH_ASSERT(Value != nullptr, false, "socket should be set");

			Exclusive.lock();
			bool Awaits = Value->Events.Readable || Value->Events.Writeable;
			Exclusive.unlock();
			return Awaits;
		}
		bool Driver::IsAwaitingReadable(Socket* Value)
		{
			TH_ASSERT(Value != nullptr, false, "socket should be set");

			Exclusive.lock();
			bool Awaits = Value->Events.Readable;
			Exclusive.unlock();
			return Awaits;
		}
		bool Driver::IsAwaitingWriteable(Socket* Value)
		{
			TH_ASSERT(Value != nullptr, false, "socket should be set");

			Exclusive.lock();
			bool Awaits = Value->Events.Writeable;
			Exclusive.unlock();
			return Awaits;
		}
		bool Driver::IsListening()
		{
			return Activations > 0;
		}
		bool Driver::IsActive()
		{
			return Handle != nullptr;
		}
		void Driver::TryDispatch()
		{
			Dispatch(DefaultTimeout);
			TryEnqueue();
		}
		void Driver::TryEnqueue()
		{
			auto* Queue = Core::Schedule::Get();
			if (Queue->CanEnqueue() && Activations > 0)
				Queue->SetTask(&Driver::TryDispatch);
		}
		void Driver::TryListen()
		{
			if (!Activations++)
			{
				TH_DEBUG("[net] start events polling");
				TryEnqueue();
			}
		}
		void Driver::TryUnlisten()
		{
			TH_ASSERT_V(Activations > 0, "events poller is already inactive");
			if (!--Activations)
				TH_DEBUG("[net] stop events polling");
		}
		bool Driver::WhenEvents(Socket* Value, bool Readable, bool Writeable, PollEventCallback&& WhenReadable, PollEventCallback&& WhenWriteable)
		{
			bool Success = false;
			bool Update = false;
			{
				std::unique_lock<std::mutex> Unique(Exclusive);
				if (Readable)
				{
					if (!Value->Events.Readable)
					{
						Value->Events.ReadCallback = std::move(WhenReadable);
						Value->Events.Readable = true;
					}
					else
					{
						Value->Events.ReadCallback.swap(WhenReadable);
						Update = true;
					}
				}

				if (Writeable)
				{
					if (!Value->Events.Writeable)
					{
						Value->Events.WriteCallback = std::move(WhenWriteable);
						Value->Events.Writeable = true;
					}
					else
					{
						Value->Events.WriteCallback.swap(WhenWriteable);
						Update = true;
					}
				}

				if (Value->Timeout > 0)
					AddTimeout(Value, Core::Schedule::GetClock() + std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::milliseconds(Value->Timeout)));

				if (Update)
					Success = Handle->Update(Value, Value->Events.Readable, Value->Events.Writeable);
				else
					Success = Handle->Add(Value, Value->Events.Readable, Value->Events.Writeable);
			}

			if (WhenWriteable || WhenReadable)
			{
				Core::Schedule::Get()->SetTask([WhenReadable = std::move(WhenReadable), WhenWriteable = std::move(WhenWriteable)]() mutable
				{
					if (WhenWriteable)
						WhenWriteable(SocketPoll::Cancel);

					if (WhenReadable)
						WhenReadable(SocketPoll::Cancel);
				}, Core::Difficulty::Light);
			}

			return Success;
		}
		void Driver::AddTimeout(Socket* Value, const std::chrono::microseconds& Time)
		{
			Value->Events.ExpiresAt = Time;
			Timeouts->Map.insert(std::make_pair(Time, Value));
		}
		void Driver::UpdateTimeout(Socket* Value, const std::chrono::microseconds& Time)
		{
			RemoveTimeout(Value);
			AddTimeout(Value, Time);
		}
		void Driver::RemoveTimeout(Socket* Value)
		{
			auto It = Timeouts->Map.find(Value->Events.ExpiresAt);
			if (It == Timeouts->Map.end())
			{
				for (auto I = Timeouts->Map.begin(); I != Timeouts->Map.end(); ++I)
				{
					if (I->second == Value)
					{
						It = I;
						break;
					}
				}
			}

			if (It != Timeouts->Map.end())
				Timeouts->Map.erase(It);
		}
		size_t Driver::GetActivations()
		{
			return Activations;
		}
		EpollHandle* Driver::Handle = nullptr;
		Core::Mapping<std::map<std::chrono::microseconds, Socket*>>* Driver::Timeouts = nullptr;
		std::vector<EpollFd>* Driver::Fds = nullptr;
		std::atomic<size_t> Driver::Activations(0);
		std::mutex Driver::Exclusive;
		uint64_t Driver::DefaultTimeout = 50;

		SocketServer::SocketServer() : Backlog(1024)
		{
			Driver::SetActive(true);
#ifndef TH_MICROSOFT
			signal(SIGPIPE, SIG_IGN);
#endif
		}
		SocketServer::~SocketServer()
		{
			Unlisten();
			Driver::SetActive(false);
		}
		void SocketServer::SetRouter(SocketRouter* New)
		{
			OnDeallocateRouter(Router);
			Router = New;
		}
		void SocketServer::SetBacklog(size_t Value)
		{
			TH_ASSERT_V(Value > 0, "backlog must be greater than zero");
			Backlog = Value;
		}
		void SocketServer::Lock()
		{
			Sync.lock();
		}
		void SocketServer::Unlock()
		{
			Sync.unlock();
		}
		bool SocketServer::Configure(SocketRouter* NewRouter)
		{
			TH_ASSERT(State == ServerState::Idle, false, "server should not be running");
			if (NewRouter != nullptr)
			{
				OnDeallocateRouter(Router);
				Router = NewRouter;
			}
			else if (!Router && !(Router = OnAllocateRouter()))
			{
				TH_ERR("[net] cannot allocate router");
				return false;
			}

			if (!OnConfigure(Router))
				return false;

			if (Router->Listeners.empty())
			{
				TH_ERR("[net] there are no listeners provided");
				return false;
			}

			for (auto&& It : Router->Listeners)
			{
				if (It.second.Hostname.empty())
					continue;

				Listener* Value = TH_NEW(Listener);
				Value->Hostname = &It.second;
				Value->Name = It.first;
				Value->Base = TH_NEW(Socket);
				Value->Base->UserData = this;

				Value->Source = DNS::FindAddressFromName(It.second.Hostname, std::to_string(It.second.Port), DNSType::Listen, SocketProtocol::TCP, SocketType::Stream);
				if (!Value->Source)
				{
					TH_ERR("[net] cannot resolve %s:%i", It.second.Hostname.c_str(), (int)It.second.Port);
					return false;
				}

				if (Value->Base->Open(Value->Source->Usable) < 0)
				{
					TH_ERR("[net] cannot open %s:%i", It.second.Hostname.c_str(), (int)It.second.Port);
					return false;
				}

				if (Value->Base->Bind(Value->Source))
				{
					TH_ERR("[net] cannot bind %s:%i", It.second.Hostname.c_str(), (int)It.second.Port);
					return false;
				}

				if (Value->Base->Listen((int)Router->BacklogQueue))
				{
					TH_ERR("[net] cannot listen %s:%i", It.second.Hostname.c_str(), (int)It.second.Port);
					return false;
				}

				Value->Base->SetCloseOnExec();
				Value->Base->SetBlocking(false);
				Listeners.push_back(Value);

				if (It.second.Port <= 0 && (It.second.Port = Value->Base->GetPort()) < 0)
				{
					TH_ERR("[net] cannot determine listener's port number");
					return false;
				}
			}
#ifdef TH_HAS_OPENSSL
			for (auto&& It : Router->Certificates)
			{
				long Protocol = SSL_OP_ALL;
				switch (It.second.Protocol)
				{
					case Secure::SSL_V2:
						Protocol = SSL_OP_ALL | SSL_OP_NO_SSLv2;
						break;
					case Secure::SSL_V3:
						Protocol = SSL_OP_ALL | SSL_OP_NO_SSLv3;
						break;
					case Secure::TLS_V1:
						Protocol = SSL_OP_ALL | SSL_OP_NO_TLSv1;
						break;
					case Secure::TLS_V1_1:
						Protocol = SSL_OP_ALL | SSL_OP_NO_TLSv1_1;
						break;
					case Secure::Any:
					default:
						Protocol = SSL_OP_ALL;
						break;
				}

				if (!(It.second.Context = SSL_CTX_new(SSLv23_server_method())))
				{
					TH_ERR("[net] cannot create server's SSL context");
					return false;
				}

				SSL_CTX_clear_options(It.second.Context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
				SSL_CTX_set_options(It.second.Context, Protocol);
				SSL_CTX_set_options(It.second.Context, SSL_OP_SINGLE_DH_USE);
				SSL_CTX_set_options(It.second.Context, SSL_OP_CIPHER_SERVER_PREFERENCE);
#ifdef SSL_CTX_set_ecdh_auto
				SSL_CTX_set_ecdh_auto(It.second.Context, 1);
#endif
				std::string ContextId = Compute::Crypto::Hash(Compute::Digests::MD5(), Core::Parser((int64_t)time(nullptr)).R());
				SSL_CTX_set_session_id_context(It.second.Context, (const unsigned char*)ContextId.c_str(), (unsigned int)ContextId.size());

				if (!It.second.Chain.empty() && !It.second.Key.empty())
				{
					if (SSL_CTX_load_verify_locations(It.second.Context, It.second.Chain.c_str(), It.second.Key.c_str()) != 1)
					{
						TH_ERR("[net] cannot load verification locations:\n\t%s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (SSL_CTX_set_default_verify_paths(It.second.Context) != 1)
					{
						TH_ERR("[net] cannot set default verification paths:\n\t%s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (SSL_CTX_use_certificate_file(It.second.Context, It.second.Chain.c_str(), SSL_FILETYPE_PEM) <= 0)
					{
						TH_ERR("[net] cannot use this certificate file:\n\t%s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (SSL_CTX_use_PrivateKey_file(It.second.Context, It.second.Key.c_str(), SSL_FILETYPE_PEM) <= 0)
					{
						TH_ERR("[net] cannot use this private key:\n\t%s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (!SSL_CTX_check_private_key(It.second.Context))
					{
						TH_ERR("[net] cannot verify this private key:\n\t%s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (It.second.VerifyPeers)
					{
						SSL_CTX_set_verify(It.second.Context, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
						SSL_CTX_set_verify_depth(It.second.Context, (int)It.second.Depth);
					}
				}

				if (!It.second.Ciphers.empty())
				{
					if (SSL_CTX_set_cipher_list(It.second.Context, It.second.Ciphers.c_str()) != 1)
					{
						TH_ERR("[net] cannot set ciphers list:\n\t%s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}
				}
			}
#endif
			return true;
		}
		bool SocketServer::Unlisten()
		{
			TH_MEASURE(TH_TIMING_HANG);
			if (!Router && State == ServerState::Idle)
				return false;

			State = ServerState::Stopping;
			int64_t Timeout = time(nullptr);

			do
			{
				if (time(nullptr) - Timeout > 5)
				{
					TH_ERR("[stall] server has stalled connections: %i", (int)Active.size());
					Sync.lock();
					OnStall(Active);
					Sync.unlock();
					break;
				}

				Sync.lock();
				auto Copy = Active;
				Sync.unlock();

				for (auto* Base : Copy)
				{
					Base->Info.Close = true;
					Base->Info.KeepAlive = 0;
					if (Base->Stream->IsPending())
						Base->Stream->ClearEvents(true);
				}

				FreeAll();
				TH_MEASURE_LOOP();
			} while (!Inactive.empty() || !Active.empty());

			if (!OnUnlisten())
				return false;

			for (auto It : Listeners)
			{
				if (It->Base != nullptr)
					It->Base->Close();
			}

			FreeAll();
			for (auto It : Listeners)
			{
				TH_DELETE(Socket, It->Base);
				TH_DELETE(Listener, It);
			}

			OnDeallocateRouter(Router);
			State = ServerState::Idle;
			Router = nullptr;

			return true;
		}
		bool SocketServer::Listen()
		{
			if (Listeners.empty() || State != ServerState::Idle)
				return false;

			State = ServerState::Working;
			if (!OnListen())
				return false;

			for (auto&& Source : Listeners)
			{
				Source->Base->AcceptAsync(true, [this, Source](socket_t Fd, char* RemoteAddr)
				{
					if (State != ServerState::Working)
						return false;
				
					std::string IpAddress = RemoteAddr;
					Core::Schedule::Get()->SetTask([this, Source, Fd, IpAddress = std::move(IpAddress)]() mutable
					{
						Accept(Source, Fd, IpAddress);
					}, Core::Difficulty::Light);
					return State == ServerState::Working;
				});
			}

			return true;
		}
		bool SocketServer::FreeAll()
		{
			TH_MEASURE(TH_TIMING_FRAME);
			if (Inactive.empty())
				return false;

			Sync.lock();
			for (auto* Item : Inactive)
				OnDeallocate(Item);
			Inactive.clear();
			Sync.unlock();

			return true;
		}
		bool SocketServer::Refuse(SocketConnection* Base)
		{
			Base->Stream->CloseAsync(false, [this, Base]()
			{
				Push(Base);
			});
			return false;
		}
		bool SocketServer::Accept(Listener* Host, socket_t Fd, const std::string& Address)
		{
			auto* Base = Pop(Host);
			if (!Base)
				return false;
            
			strncpy(Base->RemoteAddress, Address.c_str(), std::min(Address.size(), sizeof(Base->RemoteAddress)));
			Base->Stream->Timeout = Router->SocketTimeout;
			Base->Stream->SetFd(Fd, false);
			Base->Stream->SetCloseOnExec();
			Base->Stream->SetNoDelay(Router->EnableNoDelay);
			Base->Stream->SetKeepAlive(true);
			Base->Stream->SetBlocking(false);

			if (Router->GracefulTimeWait >= 0)
				Base->Stream->SetTimeWait((int)Router->GracefulTimeWait);

			if (Router->MaxConnections > 0 && Active.size() >= Router->MaxConnections)
				return Refuse(Base);

			if (!Host->Hostname->Secure)
				return OnRequestBegin(Base);

			if (!EncryptThenBegin(Base, Host))
				return Refuse(Base);
			
			return true;
		}
		bool SocketServer::EncryptThenBegin(SocketConnection* Base, Listener* Host)
		{
			TH_ASSERT(Base != nullptr, false, "socket should be set");
			TH_ASSERT(Base->Stream != nullptr, false, "socket should be set");
			TH_ASSERT(Host != nullptr, false, "host should be set");

			if (Router->Certificates.empty())
				return false;

			auto Source = Router->Certificates.find(Host->Name);
			if (Source == Router->Certificates.end())
				return false;

			ssl_ctx_st* Context = Source->second.Context;
			if (!Context || Base->Stream->Secure(Context, nullptr) == -1)
				return false;

#ifdef TH_HAS_OPENSSL
			int Result = SSL_set_fd(Base->Stream->GetDevice(), (int)Base->Stream->GetFd());
			if (Result != 1)
				return false;

			TryEncryptThenBegin(Base);
			return true;
#else
			return false;
#endif
		}
		bool SocketServer::TryEncryptThenBegin(SocketConnection* Base)
		{
#ifdef TH_HAS_OPENSSL
			int ErrorCode = SSL_accept(Base->Stream->GetDevice());
			if (ErrorCode != -1)
				return OnRequestBegin(Base);

			switch (SSL_get_error(Base->Stream->GetDevice(), ErrorCode))
			{
				case SSL_ERROR_WANT_ACCEPT:
				case SSL_ERROR_WANT_READ:
				{
					return Driver::WhenReadable(Base->Stream, [this, Base](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
							OnRequestBegin(Base);
						else
							Refuse(Base);
					});
				}
				case SSL_ERROR_WANT_WRITE:
				{
					return Driver::WhenWriteable(Base->Stream, [this, Base](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
							OnRequestBegin(Base);
						else
							Refuse(Base);
					});
				}
				default:
					return Refuse(Base);
			}
#else
			return false;
#endif
		}
		bool SocketServer::Manage(SocketConnection* Base)
		{
			TH_ASSERT(Base != nullptr, false, "socket should be set");
			if (Base->Info.KeepAlive < -1)
				return false;

			if (!OnRequestEnded(Base, true))
				return false;

			if (Router->KeepAliveMaxCount >= 0)
				Base->Info.KeepAlive--;

			if (Base->Info.KeepAlive >= -1)
				Base->Info.Finish = Utils::Clock();

			if (!OnRequestEnded(Base, false))
				return false;

			Base->Stream->Timeout = Router->SocketTimeout;
			Base->Stream->Income = 0;
			Base->Stream->Outcome = 0;

			if (!Base->Info.Close && Base->Info.KeepAlive > -1 && Base->Stream->IsValid())
				return OnRequestBegin(Base);

			Base->Info.KeepAlive = -2;
			Base->Stream->CloseAsync(true, [this, Base]
			{
				Push(Base);
			});

			return true;
		}
		bool SocketServer::OnConfigure(SocketRouter*)
		{
			return true;
		}
		bool SocketServer::OnRequestEnded(SocketConnection*, bool)
		{
			return true;
		}
		bool SocketServer::OnRequestBegin(SocketConnection*)
		{
			return true;
		}
		bool SocketServer::OnDeallocate(SocketConnection* Base)
		{
			if (!Base)
				return false;

			TH_DELETE(SocketConnection, Base);
			return true;
		}
		bool SocketServer::OnDeallocateRouter(SocketRouter* Base)
		{
			if (!Base)
				return false;

			TH_DELETE(SocketRouter, Base);
			return true;
		}
		bool SocketServer::OnStall(std::unordered_set<SocketConnection*>& Base)
		{
			return true;
		}
		bool SocketServer::OnListen()
		{
			return true;
		}
		bool SocketServer::OnUnlisten()
		{
			return true;
		}
		void SocketServer::Push(SocketConnection* Base)
		{
			TH_MEASURE(TH_TIMING_FRAME);
			Sync.lock();
			auto It = Active.find(Base);
			if (It != Active.end())
				Active.erase(It);

			Base->Reset(true);
			if (Inactive.size() < Backlog)
				Inactive.insert(Base);
			else
				OnDeallocate(Base);
			Sync.unlock();
		}
		SocketConnection* SocketServer::Pop(Listener* Host)
		{
			SocketConnection* Result = nullptr;
			if (!Inactive.empty())
			{
				Sync.lock();
				if (!Inactive.empty())
				{
					auto It = Inactive.begin();
					Result = *It;
					Inactive.erase(It);
					Active.insert(Result);
				}
				Sync.unlock();
			}

			if (!Result)
			{
				Result = OnAllocate(Host);
				if (!Result)
					return nullptr;

				Socket* Base = TH_NEW(Socket);
				Result->Stream = Base;
				Result->Stream->UserData = Result;

				Sync.lock();
				Active.insert(Result);
				Sync.unlock();
			}

			Result->Host = Host;
			Result->Info.KeepAlive = (Router->KeepAliveMaxCount > 0 ? Router->KeepAliveMaxCount - 1 : 0);

			return Result;
		}
		SocketConnection* SocketServer::OnAllocate(Listener*)
		{
			return TH_NEW(SocketConnection);
		}
		SocketRouter* SocketServer::OnAllocateRouter()
		{
			return TH_NEW(SocketRouter);
		}
		std::unordered_set<SocketConnection*>* SocketServer::GetClients()
		{
			return &Active;
		}
		ServerState SocketServer::GetState() const
		{
			return State;
		}
		SocketRouter* SocketServer::GetRouter()
		{
			return Router;
		}
		size_t SocketServer::GetBacklog() const
		{
			return Backlog;
		}

		SocketClient::SocketClient(int64_t RequestTimeout) : Context(nullptr), Timeout(RequestTimeout), AutoEncrypt(true)
		{
			Driver::SetActive(true);
			Stream.UserData = this;
		}
		SocketClient::~SocketClient()
		{
			if (Stream.IsValid())
			{
				int Result = TH_AWAIT(Close());
				TH_WARN("[net:%i] socket client leaking\n\tconsider manual termination", Result);
			}
#ifdef TH_HAS_OPENSSL
			if (Context != nullptr)
			{
				SSL_CTX_free(Context);
				Context = nullptr;
			}
#endif
			Driver::SetActive(false);
		}
		Core::Promise<int> SocketClient::Connect(Host* Source, bool Async)
		{
			TH_ASSERT(Source != nullptr && !Source->Hostname.empty(), -2, "address should be set");
			TH_ASSERT(!Stream.IsValid(), -2, "stream should not be connected");

			Stage("dns resolve");
			if (!OnResolveHost(Source))
			{
				Error("cannot resolve host %s:%i", Source->Hostname.c_str(), (int)Source->Port);
				return Core::Promise<int>(-2);
			}

			Hostname = *Source;
			Stage("socket open");
			
			Core::Promise<int> Future;
			auto RemoteConnect = [this, Future, Async](Address*&& Host) mutable
			{
				if (Host != nullptr && Stream.Open(Host->Usable) == 0)
				{
					Stage("socket connect");
					Stream.Timeout = Timeout;
					Stream.SetCloseOnExec();
					Stream.SetBlocking(!Async);
					Stream.ConnectAsync(Host, [this, Future](int Code) mutable
					{
						if (Code == 0)
						{
							auto Finalize = [this, Future]() mutable
							{
								Stage("socket proto-connect");
								Done = [Future](SocketClient*, int Code) mutable { Future = Code; };
								OnConnect();
							};
#ifdef TH_HAS_OPENSSL
							if (Hostname.Secure)
							{
								Stage("socket ssl handshake");
								if (Context != nullptr || (Context = SSL_CTX_new(SSLv23_client_method())) != nullptr)
								{
									if (AutoEncrypt)
									{
										Encrypt([this, Future, Finalize = std::move(Finalize)](bool Success) mutable
										{
											if (!Success)
											{
												Error("cannot connect ssl context");
												Future = -1;
											}
											else
												Finalize();
										});
									}
									else
										Finalize();
								}
								else
								{
									Error("cannot create ssl context");
									Future = -1;
								}
							}
							else
								Finalize();
#else
							Finalize();
#endif
						}
						else
						{
							Error("cannot connect to %s:%i", Hostname.Hostname.c_str(), (int)Hostname.Port);
							Future = -1;
						}
					});
				}
				else
				{
					Error("cannot open %s:%i", Hostname.Hostname.c_str(), (int)Hostname.Port);
					Future = -2;
				}
			};

			if (Async)
			{
				Core::Cotask<Address*>([this]()
				{
					return DNS::FindAddressFromName(Hostname.Hostname, std::to_string(Hostname.Port), DNSType::Connect, SocketProtocol::TCP, SocketType::Stream);
				}).Await(std::move(RemoteConnect));
			}
			else
				RemoteConnect(DNS::FindAddressFromName(Hostname.Hostname, std::to_string(Hostname.Port), DNSType::Connect, SocketProtocol::TCP, SocketType::Stream));

			return Future;
		}
		Core::Promise<int> SocketClient::Close()
		{
			if (!Stream.IsValid())
				return -2;

			Core::Promise<int> Result;
			Done = [Result](SocketClient*, int Code) mutable
			{
				Result = Code;
			};

			OnClose();
			return Result;
		}
		void SocketClient::Encrypt(std::function<void(bool)>&& Callback)
		{
			TH_ASSERT_V(Callback != nullptr, "callback should be set");
#ifdef TH_HAS_OPENSSL
			Stage("ssl handshake");
			if (Stream.GetDevice() || !Context)
			{
				Error("client does not use ssl");
				return Callback(false);
			}

			if (Stream.Secure(Context, Hostname.Hostname.c_str()) == -1)
			{
				Error("cannot establish handshake");
				return Callback(false);
			}

			int Result = SSL_set_fd(Stream.GetDevice(), (int)Stream.GetFd());
			if (Result != 1)
			{
				Error("cannot set fd");
				return Callback(false);
			}

			TryEncrypt(std::move(Callback));
#else
			Error("ssl is not supported for clients");
			Callback(false);
#endif
		}
		void SocketClient::TryEncrypt(std::function<void(bool)>&& Callback)
		{
			int ErrorCode = SSL_connect(Stream.GetDevice());
			if (ErrorCode != -1)
				return Callback(true);

			switch (SSL_get_error(Stream.GetDevice(), ErrorCode))
			{
				case SSL_ERROR_WANT_READ:
				{
					Driver::WhenReadable(&Stream, [this, Callback = std::move(Callback)](SocketPoll Event) mutable
					{
						if (!Packet::IsDone(Event))
						{
							Error("ssl connection timeout\n\t%s", ERR_error_string(ERR_get_error(), nullptr));
							Callback(false);
						}
						else
							TryEncrypt(std::move(Callback));
					});
					break;
				}
				case SSL_ERROR_WANT_CONNECT:
				case SSL_ERROR_WANT_WRITE:
				{
					Driver::WhenWriteable(&Stream, [this, Callback = std::move(Callback)](SocketPoll Event) mutable
					{
						if (!Packet::IsDone(Event))
						{
							Error("ssl connection timeout\n\t%s", ERR_error_string(ERR_get_error(), nullptr));
							Callback(false);
						}
						else
							TryEncrypt(std::move(Callback));
					});
					break;
				}
				default:
				{
					Error("%s", ERR_error_string(ERR_get_error(), nullptr));
					return Callback(false);
				}
			}
		}
		bool SocketClient::OnResolveHost(Host* Address)
		{
			return Address != nullptr;
		}
		bool SocketClient::OnConnect()
		{
			return Success(0);
		}
		bool SocketClient::OnClose()
		{
			return Stream.CloseAsync(true, [this]()
			{
				Success(0);
			}) == 0;
		}
		bool SocketClient::Stage(const std::string& Name)
		{
			Action = Name;
			return !Action.empty();
		}
		bool SocketClient::Error(const char* Format, ...)
		{
			char Buffer[TH_BIG_CHUNK_SIZE];
			va_list Args;
			va_start(Args, Format);
			int Size = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			TH_ERR("[net] %.*s\n\tat %s", Size, Buffer, Action.empty() ? "request" : Action.c_str());
			Stream.CloseAsync(true, [this]()
			{
				Success(-1);
			});

			return false;
	}
		bool SocketClient::Success(int Code)
		{
			SocketClientCallback Callback(std::move(Done));
			if (Callback)
				Callback(this, Code);

			return true;
		}
		Socket* SocketClient::GetStream()
		{
			return &Stream;
		}
	}
}
#pragma warning(pop)
