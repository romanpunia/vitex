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
		static int TryConnect(socket_t Base, const sockaddr* Address, int Length, uint64_t Timeout, bool IsBlocking)
		{
			Socket Stream(Base);
			if (IsBlocking)
				Stream.SetBlocking(false);

			int Status = connect(Base, Address, Length);
#ifndef TH_MICROSOFT
			if (Status != 0 && errno == EINPROGRESS)
				Status = 0;
#endif
			if (Status != 0 && Stream.GetError(Status) != ERRWOULDBLOCK)
			{
				if (IsBlocking)
					Stream.SetBlocking(true);

				return Status;
			}

			if (IsBlocking)
				Stream.SetBlocking(true);

			pollfd Fd;
			Fd.fd = Base;
			Fd.events = POLLOUT;

			return Utils::Poll(&Fd, 1, (int)Timeout) > 0;
		}
		static addrinfo* TryConnectDNS(const std::unordered_map<socket_t, addrinfo*>& Hosts, uint64_t Timeout)
		{
			TH_PPUSH(TH_PERF_NET);

			std::vector<pollfd> Sockets4, Sockets6;
			for (auto& Host : Hosts)
			{
				Socket Stream(Host.first);
				Stream.SetBlocking(false);

				TH_DEBUG("[net] resolve dns on fd %i", (int)Host.first);
				int Status = connect(Host.first, Host.second->ai_addr, Host.second->ai_addrlen);
#ifndef TH_MICROSOFT
				if (Status != 0 && errno == EINPROGRESS)
					Status = 0;
#endif
				if (Status != 0 && Stream.GetError(Status) != ERRWOULDBLOCK)
					continue;

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
						{
							TH_PPOP();
							return It->second;
						}
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
						{
							TH_PPOP();
							return It->second;
						}
					}
				}
			}

			TH_PPOP();
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
			if (Filename.empty() && Extension.empty())
				return;

			Path += Filename + (Extension.empty() ? "" : '.' + Extension);
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
				TH_PPUSH(TH_PERF_NET);
				int Timeout = 100;
				SetBlocking(true);
				SetSocket(SO_RCVTIMEO, &Timeout, sizeof(int));
				shutdown(Fd, SD_SEND);

				while (recv(Fd, (char*)&Error, 1, 0) > 0)
					TH_PSIG();

				closesocket(Fd);
				TH_PPOP();
			}
			else
			{
				shutdown(Fd, SD_SEND);
				closesocket(Fd);
			}

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
            TH_PPUSH(TH_PERF_NET);
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
                TH_PRET(GetError((int)Value) == ERRWOULDBLOCK ? -2 : -1);
            
            if (Value != Size)
                Value = Size;
            
            Outcome += Value;
            TH_PPOP();
            
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
			TH_PPUSH(TH_PERF_NET);
#ifdef TH_HAS_OPENSSL
			if (Device != nullptr)
			{
				int Value = SSL_write(Device, Buffer, Size);
				if (Value <= 0)
					TH_PRET(GetError(Value) == SSL_ERROR_WANT_WRITE ? -2 : -1);

				Outcome += (int64_t)Value;
				TH_PPOP();

				return Value;
			}
#endif
			int Value = (int)send(Fd, Buffer, Size, 0);
			if (Value <= 0)
				TH_PRET(GetError(Value) == ERRWOULDBLOCK ? -2 : -1);

			Outcome += (int64_t)Value;
			TH_PPOP();

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
			TH_PPUSH(TH_PERF_NET);
#ifdef TH_HAS_OPENSSL
			if (Device != nullptr)
			{
				int Value = SSL_read(Device, Buffer, Size);
				if (Value <= 0)
					TH_PRET(GetError(Value) == SSL_ERROR_WANT_READ ? -2 : -1);

				Income += (int64_t)Value;
				TH_PPOP();

				return Value;
			}
#endif
			int Value = (int)recv(Fd, Buffer, Size, 0);
			if (Value <= 0)
				TH_PRET(GetError(Value) == ERRWOULDBLOCK ? -2 : -1);

			Income += (int64_t)Value;
			TH_PPOP();

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

			char Buffer[8192];
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

			char Buffer = 0;
			int Size = (int)strlen(Match);
			int Offset = 0, Index = 0;

			TH_ASSERT(Size > 0, -1, "match should not be empty");
			while (true)
			{
				int Length = Read(&Buffer, 1);
				if (Length <= 0)
				{
					if (Callback)
						Callback(SocketPoll::Reset, nullptr, 0);

					return -1;
				}

                Offset += Length;
				if (Callback && !Callback(SocketPoll::Next, &Buffer, 1))
					break;

				if (Match[Index] == Buffer)
				{
					if (++Index >= Size)
						break;
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

			int64_t Size = (int64_t)strlen(Match);
			TH_ASSERT(Size > 0, -1, "match should not be empty");

			char Buffer = 0;
            int Offset = 0;
            
			while (true)
			{
				int Length = Read(&Buffer, 1);
				if (Length == -2)
				{
					if (!TempBuffer)
					{
						TempBuffer = TH_MALLOC(char, (size_t)Size + 1);
						memcpy(TempBuffer, Match, (size_t)Size);
						TempBuffer[Size] = '\0';
					}

					Driver::WhenReadable(this, [this, TempBuffer, TempIndex, Size, Callback = std::move(Callback)](SocketPoll Event) mutable
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

					if (Callback)
						Callback(SocketPoll::Reset, nullptr, 0);

					return -1;
				}

                Offset += Length;
				if (Callback && !Callback(SocketPoll::Next, &Buffer, 1))
					break;

				if (Match[TempIndex] == Buffer)
				{
					if (++TempIndex >= Size)
						break;
				}
				else
					TempIndex = 0;
			}

			if (TempBuffer != nullptr)
				TH_FREE(TempBuffer);

			if (Callback)
				Callback(TempBuffer ? SocketPoll::Finish : SocketPoll::FinishSync, nullptr, 0);

			return Offset;
		}
		int Socket::Open(const std::string& Host, const std::string& Port, SocketProtocol Proto, SocketType Type, DNSType DNS, Address** Subresult)
		{
			TH_PPUSH(TH_PERF_NET);
			TH_DEBUG("[net] open fd %s:%s", Host.c_str(), Port.c_str());

			Address* Result = DNS::FindAddressFromName(Host, Port, Proto, Type, DNS);
			if (!Result)
			{
				TH_PPOP();
				return -1;
			}

			if (Subresult != nullptr)
				*Subresult = Result;

			TH_PRET(Open(Result->Usable));
		}
		int Socket::Open(const std::string& Host, const std::string& Port, DNSType DNS, Address** Result)
		{
			return Open(Host, Port, SocketProtocol::TCP, SocketType::Stream, DNS, Result);
		}
		int Socket::Open(addrinfo* Good)
		{
			TH_ASSERT(Good != nullptr, -1, "addrinfo should be set");
			TH_PPUSH(TH_PERF_NET);
			TH_DEBUG("[net] assign fd");

			Fd = socket(Good->ai_family, Good->ai_socktype, Good->ai_protocol);
			if (Fd == INVALID_SOCKET)
			{
				TH_PPOP();
				return -1;
			}

			int Option = 1;
			setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, (char*)&Option, sizeof(Option));
			TH_PPOP();
			return 0;
		}
		int Socket::Secure(ssl_ctx_st* Context, const char* Hostname)
		{
#ifdef TH_HAS_OPENSSL
			TH_PPUSH(TH_PERF_NET);
			if (Device != nullptr)
				SSL_free(Device);

			Device = SSL_new(Context);
#ifndef OPENSSL_NO_TLSEXT
			if (Device != nullptr && Hostname != nullptr)
				SSL_set_tlsext_host_name(Device, Hostname);
#endif
			TH_PPOP();
#endif
			if (!Device)
				return -1;

			return 0;
		}
		int Socket::Bind(Address* Address)
		{
			TH_ASSERT(Address && Address->Usable, -1, "address should be set and usable");
			TH_DEBUG("[net] bind fd %i", (int)Fd);
			return bind(Fd, Address->Usable->ai_addr, (int)Address->Usable->ai_addrlen);
		}
		int Socket::Connect(Address* Address, uint64_t Timeout)
		{
			TH_ASSERT(Address && Address->Usable, -1, "address should be set and usable");
			TH_PPUSH(TH_PERF_NET);
			TH_DEBUG("[net] connect fd %i", (int)Fd);
			TH_PRET(TryConnect(Fd, Address->Usable->ai_addr, (int)Address->Usable->ai_addrlen, Timeout, true));
		}
		int Socket::Listen(int Backlog)
		{
			TH_DEBUG("[net] listen fd %i", (int)Fd);
			return listen(Fd, Backlog);
		}
		int Socket::ClearEvents(bool Gracefully)
		{
			TH_PPUSH(TH_PERF_NET);
			if (Gracefully)
				Driver::CancelEvents(this, SocketPoll::Reset);
			else
				Driver::ClearEvents(this);
			TH_PPOP();
			return 0;
		}
		int Socket::SetFd(socket_t NewFd, bool Gracefully)
		{
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
			return (Range1 || Range2);
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
			char Buffer[1024];
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
			char Buffer[8192];
			if (Info.Close)
				return Finish();

			va_list Args;
			va_start(Args, ErrorMessage);
			int Count = vsnprintf(Buffer, sizeof(Buffer), ErrorMessage, Args);
			va_end(Args);

			Info.Message.assign(Buffer, Count);
			return Finish(StatusCode);
		}
		bool SocketConnection::Certify(Certificate* Output)
		{
#ifdef TH_HAS_OPENSSL
			TH_ASSERT(Output != nullptr, false, "certificate should be set");
			TH_PPUSH(TH_PERF_NET);

			X509* Certificate = SSL_get_peer_certificate(Stream->GetDevice());
			if (!Certificate)
			{
				TH_PPOP();
				return false;
			}

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

				if (!Compute::Codec::HexToString(Buffer, (uint64_t)Size, SerialBuffer, sizeof(SerialBuffer)))
					*SerialBuffer = '\0';
			}
			else
				*SerialBuffer = '\0';
#if OPENSSL_VERSION_MAJOR < 3
			unsigned int Size = 0;
			ASN1_digest((int (*)(void*, unsigned char**))i2d_X509, Digest, (char*)Certificate, Buffer, &Size);
            
			char FingerBuffer[1024];
			if (!Compute::Codec::HexToString(Buffer, (uint64_t)Size, FingerBuffer, sizeof(FingerBuffer)))
				*FingerBuffer = '\0';
            
            Output->Finger = FingerBuffer;
#endif
			Output->Subject = SubjectBuffer;
			Output->Issuer = IssuerBuffer;
			Output->Serial = SerialBuffer;

			X509_free(Certificate);
			TH_PPOP();

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

		EpollHandle::EpollHandle(int _ArraySize) : ArraySize(_ArraySize)
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
		std::string DNS::FindNameFromAddress(const std::string& IpAddress, uint32_t Port)
		{
			TH_ASSERT(!IpAddress.empty(), std::string(), "ip address should not be empty");
			TH_ASSERT(Port > 0, std::string(), "port should be greater than zero");
			TH_PPUSH(TH_PERF_NET * 3);

			struct sockaddr_storage Storage;
			int Family = Socket::GetAddressFamily(IpAddress.c_str());
			int Result = -1;

			if (Family == AF_INET)
			{
				auto* Base = reinterpret_cast<struct sockaddr_in*>(&Storage);
				Result = inet_pton(Family, IpAddress.c_str(), &Base->sin_addr.s_addr);
				Base->sin_family = Family;
				Base->sin_port = htons(Port);
			}
			else if (Family == AF_INET6)
			{
				auto* Base = reinterpret_cast<struct sockaddr_in6*>(&Storage);
				Result = inet_pton(Family, IpAddress.c_str(), &Base->sin6_addr);
				Base->sin6_family = Family;
				Base->sin6_port = htons(Port);
			}

			if (Result == -1)
			{
				TH_ERR("[dns] cannot reverse resolve dns for identity %s:%i\n\tinvalid address", IpAddress.c_str(), Port);
				TH_PPOP();
				return std::string();
			}

			char Host[NI_MAXHOST], Service[NI_MAXSERV];
			if (getnameinfo((struct sockaddr*)&Storage, sizeof(struct sockaddr), Host, NI_MAXHOST, Service, NI_MAXSERV, NI_NUMERICSERV) != 0)
			{
				TH_ERR("[dns] cannot reverse resolve dns for identity %s:%i", IpAddress.c_str(), Port);
				TH_PPOP();
				return std::string();
			}

			TH_DEBUG("[net] dns reverse resolved for identity %s:%i\n\thost %s:%s is used", IpAddress.c_str(), Port, Host, Service);
			TH_PPOP();

			return Host;
		}
		Address* DNS::FindAddressFromName(const std::string& Host, const std::string& Service, SocketProtocol Proto, SocketType Type, DNSType DNS)
		{
			TH_ASSERT(!Host.empty(), nullptr, "host should not be empty");
			TH_ASSERT(!Service.empty(), nullptr, "service should not be empty");
			TH_PPUSH(TH_PERF_NET * 3);

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
			Exclusive.lock();
			{
				auto It = Names.find(Identity);
				if (It != Names.end() && It->second.first > Time)
				{
					Address* Result = It->second.second;
					Exclusive.unlock();
					TH_PRET(Result);
				}
			}
			Exclusive.unlock();

			struct addrinfo* Addresses = nullptr;
			if (getaddrinfo(Host.c_str(), Service.c_str(), &Hints, &Addresses) != 0)
			{
				TH_ERR("[dns] cannot resolve dns for identity %s", Identity.c_str());
				TH_PPOP();
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
				TH_PPOP();
				return nullptr;
			}

			Address* Result = TH_NEW(Address);
			Result->Pool = Addresses;
			Result->Usable = Good;

			TH_DEBUG("[net] dns resolved for identity %s\n\taddress %s is used", Identity.c_str(), Socket::GetAddress(Good).c_str());
			Exclusive.lock();
			{
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
			}
			Exclusive.unlock();

			TH_PPOP();
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

		void Driver::Create(size_t MaxEvents)
		{
			TH_ASSERT_V(MaxEvents > 0, "array size should be greater than zero");
			TH_DELETE(EpollHandle, Handle);

			if (!Timeouts)
				Timeouts = new std::map<std::chrono::microseconds, Socket*>();

			if (!Fds)
				Fds = TH_NEW(std::vector<EpollFd>);
			
			Handle = TH_NEW(EpollHandle, (int)MaxEvents);
			Fds->resize(MaxEvents);
		}
		void Driver::Release()
		{
			if (Timeouts != nullptr)
			{
				delete Timeouts;
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
		int Driver::Dispatch(uint64_t EventTimeout)
		{
			TH_ASSERT(Handle != nullptr && Timeouts != nullptr, -1, "driver should be initialized");

			int Count = Handle->Wait(Fds->data(), Fds->size(), EventTimeout);
			if (Count < 0)
				return Count;

			TH_PPUSH(EventTimeout + TH_PERF_IO);
			size_t Size = (size_t)Count, Cleanups = 0;
			auto Time = Core::Schedule::GetClock();

			for (size_t i = 0; i < Size; i++)
			{
				if (!DispatchEvents(Fds->at(i), Time))
					++Cleanups;
			}

			TH_PPOP();
			if (Timeouts->empty())
				return Count;

			TH_PPUSH(TH_PERF_IO);
			Exclusive.lock();
			while (!Timeouts->empty())
			{
				auto It = Timeouts->begin();
				if (It->first > Time)
					break;

				TH_DEBUG("[net] sock timeout on fd %i", (int)It->second->Fd);
				CancelEvents(It->second, SocketPoll::Timeout, false);
				Timeouts->erase(It);
			}
			Exclusive.unlock();

			TH_PPOP();
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
		bool Driver::IsActive()
		{
			return Handle != nullptr;
		}
		bool Driver::WhenEvents(Socket* Value, bool Readable, bool Writeable, PollEventCallback&& WhenReadable, PollEventCallback&& WhenWriteable)
		{
			bool Success = false;
			Exclusive.lock();
			{
				bool Update = false;
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
			Exclusive.unlock();

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
			Timeouts->insert(std::make_pair(Time, Value));
		}
		void Driver::UpdateTimeout(Socket* Value, const std::chrono::microseconds& Time)
		{
			RemoveTimeout(Value);
			AddTimeout(Value, Time);
		}
		void Driver::RemoveTimeout(Socket* Value)
		{
			auto It = Timeouts->find(Value->Events.ExpiresAt);
			if (It == Timeouts->end())
			{
				for (auto I = Timeouts->begin(); I != Timeouts->end(); ++I)
				{
					if (I->second == Value)
					{
						It = I;
						break;
					}
				}
			}

			if (It != Timeouts->end())
				Timeouts->erase(It);
		}
		EpollHandle* Driver::Handle = nullptr;
		std::map<std::chrono::microseconds, Socket*>* Driver::Timeouts = nullptr;
		std::vector<EpollFd>* Driver::Fds = nullptr;
		std::mutex Driver::Exclusive;

		SocketServer::SocketServer() : Backlog(1024)
		{
#ifndef TH_MICROSOFT
			signal(SIGPIPE, SIG_IGN);
#endif
		}
		SocketServer::~SocketServer()
		{
			Unlisten();
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

				if (Value->Base->Open(It.second.Hostname.c_str(), std::to_string(It.second.Port), DNSType::Listen, &Value->Source))
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
			if (!Router && State == ServerState::Idle)
				return false;

			State = ServerState::Stopping;
			int64_t Timeout = time(nullptr);
			TH_PPUSH(TH_PERF_HANG);
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
				TH_PSIG();
			} while (!Inactive.empty() || !Active.empty());
			TH_PPOP();

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

			for (auto&& It : Listeners)
			{
				It->Base->AcceptAsync(true, [this, It](socket_t Fd, char* RemoteAddr)
				{
					if (State != ServerState::Working)
						return false;

					Accept(It, RemoteAddr, Fd);
					return State == ServerState::Working;
				});
			}

			return true;
		}
		bool SocketServer::FreeAll()
		{
			if (Inactive.empty())
				return false;

			TH_PPUSH(TH_PERF_FRAME);
			Sync.lock();
			for (auto* Item : Inactive)
				OnDeallocate(Item);
			Inactive.clear();
			Sync.unlock();

			TH_PPOP();
			return true;
		}
		bool SocketServer::Accept(Listener* Host, char* RemoteAddr, socket_t Fd)
		{
			TH_PPUSH(TH_PERF_FRAME);
			auto* Base = Pop(Host);
			if (!Base)
			{
				TH_PPOP();
				return false;
			}
            
			strncpy(Base->RemoteAddress, RemoteAddr, sizeof(Base->RemoteAddress));
			Base->Stream->Timeout = Router->SocketTimeout;
			Base->Stream->SetFd(Fd, false);
			Base->Stream->SetCloseOnExec();
			Base->Stream->SetNoDelay(Router->EnableNoDelay);
			Base->Stream->SetKeepAlive(true);
			Base->Stream->SetBlocking(false);

			if (Router->GracefulTimeWait >= 0)
				Base->Stream->SetTimeWait((int)Router->GracefulTimeWait);

			if (Router->MaxConnections > 0 && Active.size() >= Router->MaxConnections)
				goto Refuse;

			if (Host->Hostname->Secure && !Protect(Base->Stream, Host))
				goto Refuse;

			TH_PPOP();
			return Core::Schedule::Get()->SetTask([this, Base]()
			{
				OnRequestBegin(Base);
			}, Core::Difficulty::Light);
		Refuse:
			Base->Stream->CloseAsync(false, [this, Base]()
			{
				Push(Base);
			});

			TH_PPOP();
			return false;
		}
		bool SocketServer::Protect(Socket* Fd, Listener* Host)
		{
			TH_ASSERT(Fd != nullptr, false, "socket should be set");

			ssl_ctx_st* Context = nullptr;
			if (!OnProtect(Fd, Host, &Context) || !Context)
				return false;

			if (Fd->Secure(Context, nullptr) == -1)
				return false;

#ifdef TH_HAS_OPENSSL
			TH_PPUSH(TH_PERF_NET);
			int Result = SSL_set_fd(Fd->GetDevice(), (int)Fd->GetFd());
			if (Result != 1)
			{
				TH_PPOP();
				return false;
			}

			pollfd SFd { };
			SFd.fd = Fd->GetFd();
			SFd.events = POLLIN | POLLOUT;

			int64_t Timeout = Utils::Clock();
			bool OK = true;

			while (OK)
			{
				Result = SSL_accept(Fd->GetDevice());
				if (Result >= 0)
					break;

				if (Utils::Clock() - Timeout > (int64_t)Router->SocketTimeout)
				{
					TH_PPOP();
					return false;
				}

				int Code = SSL_get_error(Fd->GetDevice(), Result);
				switch (Code)
				{
					case SSL_ERROR_WANT_CONNECT:
					case SSL_ERROR_WANT_ACCEPT:
					case SSL_ERROR_WANT_WRITE:
					case SSL_ERROR_WANT_READ:
						Utils::Poll(&SFd, 1, (int)Router->SocketTimeout);
						break;
					default:
						OK = false;
						break;
				}
			}

			TH_PPOP();
			return Result == 1;
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
		bool SocketServer::OnProtect(Socket* Fd, Listener* Host, ssl_ctx_st** Context)
		{
			TH_ASSERT(Fd != nullptr, false, "socket should be set");
			TH_ASSERT(Host != nullptr, false, "host should be set");
			TH_ASSERT(Context != nullptr, false, "context should be set");

			if (Router->Certificates.empty())
				return false;

			auto&& It = Router->Certificates.find(Host->Name);
			if (It == Router->Certificates.end())
				return false;

			*Context = It->second.Context;
			return true;
		}
		void SocketServer::Push(SocketConnection* Base)
		{
			TH_PPUSH(TH_PERF_FRAME);
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
			TH_PPOP();
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
		ServerState SocketServer::GetState()
		{
			return State;
		}
		SocketRouter* SocketServer::GetRouter()
		{
			return Router;
		}
		size_t SocketServer::GetBacklog()
		{
			return Backlog;
		}

		SocketClient::SocketClient(int64_t RequestTimeout) : Context(nullptr), Timeout(RequestTimeout), AutoCertify(true)
		{
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
		}
		Core::Async<int> SocketClient::Connect(Host* Address, bool Async)
		{
			TH_ASSERT(Address != nullptr && !Address->Hostname.empty(), -2, "address should be set");
			TH_ASSERT(!Stream.IsValid(), -2, "stream should not be connected");

			Stage("dns resolve");
			if (!OnResolveHost(Address))
			{
				Error("cannot resolve host %s:%i", Address->Hostname.c_str(), (int)Address->Port);
				return Core::Async<int>(-2);
			}

			Stage("socket open");
			Hostname = *Address;

			return Core::Coasync<int>([this, Async]()
			{
				Tomahawk::Network::Address* Host;
				if (TH_AWAIT(Core::Cotask<int>([this, &Host]()
				{
					return Stream.Open(Hostname.Hostname.c_str(), std::to_string(Hostname.Port), DNSType::Connect, &Host);
				})) == -1)
				{
					Error("cannot open %s:%i", Hostname.Hostname.c_str(), (int)Hostname.Port);
					return -2;
				}

				Stage("socket connect");
				if (TH_AWAIT(Core::Cotask<int>([this, Host]()
				{
					return Stream.Connect(Host, Timeout);
				})) == -1)
				{
					Error("cannot connect to %s:%i", Hostname.Hostname.c_str(), (int)Hostname.Port);
					return -1;
				}

				Stream.Timeout = Timeout;
				Stream.SetCloseOnExec();
				Stream.SetBlocking(!Async);
#ifdef TH_HAS_OPENSSL
				if (Hostname.Secure)
				{
					Stage("socket ssl handshake");
					if (!Context && !(Context = SSL_CTX_new(SSLv23_client_method())))
					{
						Error("cannot create ssl context");
						return -1;
					}

					if (AutoCertify && !TH_AWAIT(Core::Cotask<bool>([this]()
					{
						return Certify();
					})))
						return -1;
				}
#endif
				Core::Async<int> Result;
				Done = [Result](SocketClient*, int Code) mutable
				{
					Result = Code;
				};

				OnConnect();
				return TH_AWAIT(std::move(Result));
			});
		}
		Core::Async<int> SocketClient::Close()
		{
			if (!Stream.IsValid())
				return -2;

			Core::Async<int> Result;
			Done = [Result](SocketClient*, int Code) mutable
			{
				Result = Code;
			};

			OnClose();
			return Result;
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
		bool SocketClient::Certify()
		{
#ifdef TH_HAS_OPENSSL
			Stage("ssl handshake");
			if (Stream.GetDevice() || !Context)
				return Error("client does not use ssl");

			if (Stream.Secure(Context, Hostname.Hostname.c_str()) == -1)
				return Error("cannot establish handshake");

			int Result = SSL_set_fd(Stream.GetDevice(), (int)Stream.GetFd());
			if (Result != 1)
				return Error("cannot set fd");

			pollfd Fd;
			Fd.fd = Stream.GetFd();

			auto Time = std::chrono::system_clock::now();
			auto Max = std::chrono::milliseconds(Timeout);

			while ((Result = SSL_connect(Stream.GetDevice())) == -1)
			{
				auto Diff = std::chrono::system_clock::now() - Time;
				if (std::chrono::duration_cast<std::chrono::milliseconds>(Diff) > Max)
					return Error("ssl connection timeout\n\t%s", ERR_error_string(ERR_get_error(), nullptr));

				switch (SSL_get_error(Stream.GetDevice(), Result))
				{
					case SSL_ERROR_WANT_READ:
						Fd.events = POLLIN;
						Utils::Poll(&Fd, 1, 5);
						break;
					case SSL_ERROR_WANT_WRITE:
						Fd.events = POLLOUT;
						Utils::Poll(&Fd, 1, 5);
						break;
					default:
						return Error("%s", ERR_error_string(ERR_get_error(), nullptr));
				}
			}

			return true;
#else
			return Error("ssl is not supported for clients");
#endif
		}
		bool SocketClient::Stage(const std::string& Name)
		{
			Action = Name;
			return !Action.empty();
		}
		bool SocketClient::Error(const char* Format, ...)
		{
			char Buffer[10240];
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
