#include "network.h"
#include "../network/http.h"
#ifdef TH_MICROSOFT
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
		SourceURL::SourceURL(const std::string& Src) noexcept : URL(Src), Port(0), Protocol("file")
		{
			if (URL.empty())
				return;

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
					KeyValue[0] = Compute::Common::URIDecode(KeyValue[0]);

					if (KeyValue.size() >= 2)
						Query[KeyValue[0]] = Compute::Common::URIDecode(KeyValue[1]);
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
		void SourceURL::MakePath()
		{
			if (Filename.empty() && Extension.empty())
				return;

			Path += Filename + (Extension.empty() ? "" : '.' + Extension);
		}

		bool Address::Free(Network::Address* Address)
		{
			if (!Address)
				return false;

			if (Address->Host != nullptr)
				freeaddrinfo(Address->Host);

			Address->Host = nullptr;
			return true;
		}

		Socket::Socket() : Listener(nullptr), Input(nullptr), Output(nullptr), Device(nullptr), Fd(INVALID_SOCKET), Income(0), Outcome(0), UserData(nullptr)
		{
			Sync.Poll = false;
			Sync.Timeout = 0;
			Sync.Time = 0;
		}
		Socket::Socket(socket_t FromFd) : Socket()
		{
			Fd = FromFd;
		}
		Socket::~Socket()
		{
			TH_DELETE(SocketAcceptCallback, Listener);
		}
		int Socket::Open(const char* Host, int Port, SocketType Type, Address* Result)
		{
			struct addrinfo Hints;
			memset(&Hints, 0, sizeof(struct addrinfo));
			Hints.ai_family = AF_UNSPEC;

			switch (Type)
			{
				case SocketType::Stream:
					Hints.ai_socktype = SOCK_STREAM;
					break;
				case SocketType::Datagram:
					Hints.ai_socktype = SOCK_DGRAM;
					break;
				case SocketType::Raw:
					Hints.ai_socktype = SOCK_RAW;
					break;
				case SocketType::Reliably_Delivered_Message:
					Hints.ai_socktype = SOCK_RDM;
					break;
				case SocketType::Sequence_Packet_Stream:
					Hints.ai_socktype = SOCK_SEQPACKET;
					break;
				default:
					Hints.ai_socktype = SOCK_STREAM;
					break;
			}

			struct addrinfo* Address;
			if (getaddrinfo(Host, Core::Parser(Port).Get(), &Hints, &Address))
				return -1;

			for (auto It = Address; It; It = It->ai_next)
			{
				Fd = socket(It->ai_family, It->ai_socktype, It->ai_protocol);
				if (Fd == INVALID_SOCKET)
					continue;

				int Option = 1;
				setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, (char*)&Option, sizeof(Option));
				if (Result != nullptr)
					Result->Active = It;

				break;
			}

			if (Result != nullptr)
			{
				sockaddr_in IpAddress;
				Result->Resolved = (inet_pton(AF_INET, Host, &IpAddress.sin_addr) != 1);
				Result->Host = Address;

				if (!Result->Resolved)
					Result->Resolved = (inet_pton(AF_INET6, Host, &IpAddress.sin_addr) != 1);
			}
			else
				freeaddrinfo(Address);

			return 0;
		}
		int Socket::Open(const char* Host, int Port, Address* Result)
		{
			return Open(Host, Port, SocketType::Stream, Result);
		}
		int Socket::Open(addrinfo* Info, Address* Result)
		{
			if (!Info)
				return -1;

			for (auto It = Info; It; It = It->ai_next)
			{
				Fd = socket(It->ai_family, It->ai_socktype, It->ai_protocol);
				if (Fd == INVALID_SOCKET)
					continue;

				if (Result != nullptr)
					Result->Active = It;

				return 0;
			}

			return -1;
		}
		int Socket::Secure(ssl_ctx_st* Context, const char* Hostname)
		{
#ifdef TH_HAS_OPENSSL
			Sync.Device.lock();
			if (Device != nullptr)
				SSL_free(Device);

			Device = SSL_new(Context);
#ifndef OPENSSL_NO_TLSEXT
			if (Device != nullptr && Hostname != nullptr)
				SSL_set_tlsext_host_name(Device, Hostname);
#endif
			Sync.Device.unlock();
#endif
			if (!Device)
				return -1;

			return 0;
		}
		int Socket::Bind(Address* Address)
		{
			if (!Address || !Address->Active)
				return -1;

			return bind(Fd, Address->Active->ai_addr, (int)Address->Active->ai_addrlen);
		}
		int Socket::Connect(Address* Address)
		{
			if (!Address || !Address->Active)
				return -1;

			return connect(Fd, Address->Active->ai_addr, (int)Address->Active->ai_addrlen);
		}
		int Socket::Listen(int Backlog)
		{
			return listen(Fd, Backlog);
		}
		int Socket::Accept(Socket* Connection, Address* OutAddr)
		{
			if (!Connection)
				return -1;

			sockaddr Address;
			socket_size_t Length = sizeof(sockaddr);
			Connection->Fd = accept(Fd, &Address, &Length);
			if (Connection->Fd == INVALID_SOCKET)
				return -1;

			if (!OutAddr)
				return 0;

			struct addrinfo Hints;
			memset(&Hints, 0, sizeof(struct addrinfo));
			Hints.ai_family = AF_UNSPEC;
			Hints.ai_socktype = SOCK_STREAM;

			if (getaddrinfo(Address.sa_data, Core::Parser((((struct sockaddr_in*)&Address)->sin_port)).Get(), &Hints, &OutAddr->Host))
				return -1;

			OutAddr->Active = OutAddr->Host;
			return 0;
		}
		int Socket::AcceptAsync(SocketAcceptCallback&& Callback)
		{
			Sync.IO.lock();
			TH_DELETE(SocketAcceptCallback, Listener);
			if (Callback)
			{
				Listener = TH_NEW(SocketAcceptCallback, std::move(Callback));
				Driver::Listen(this, false);
			}
			else
			{
				Listener = nullptr;
				Driver::Unlisten(this, true);
			}
			Sync.IO.unlock();

			return 0;
		}
		int Socket::Clear(bool Gracefully)
		{
			if (!Gracefully)
			{
				Sync.IO.lock();
				ReadFlush();
				WriteFlush();
				Driver::Unlisten(this, true);
				Sync.IO.unlock();
			}
			else
			{
				Sync.IO.lock();
				Driver::Unlisten(this, true);
				Sync.IO.unlock();
				while (Skip((uint32_t)(SocketEvent::Read | SocketEvent::Write), -2) == 1);
			}

			return 0;
		}
		int Socket::Close(bool Gracefully)
		{
			Clear(false);
			if (Fd == INVALID_SOCKET)
				return -1;
#ifdef TH_HAS_OPENSSL
			if (Device != nullptr)
			{
				Sync.Device.lock();
				SSL_free(Device);
				Device = nullptr;
				Sync.Device.unlock();
			}
#endif
			int Error = 1;
			socklen_t Size = sizeof(Error);
			getsockopt(Fd, SOL_SOCKET, SO_ERROR, (char*)&Error, &Size);

			if (Gracefully)
			{
				int Timeout = 100;
				SetBlocking(true);
				SetSocket(SO_RCVTIMEO, &Timeout, sizeof(int));
				shutdown(Fd, SD_SEND);

				while (recv(Fd, (char*)&Error, 1, 0) > 0);
				closesocket(Fd);
			}
			else
			{
				shutdown(Fd, SD_SEND);
				closesocket(Fd);
			}

			Fd = INVALID_SOCKET;
			return 0;
		}
		int Socket::CloseAsync(bool Gracefully, const SocketAcceptCallback& Callback)
		{
			Clear(false);
			if (Fd == INVALID_SOCKET)
				return -1;
#ifdef TH_HAS_OPENSSL
			if (Device != nullptr)
			{
				Sync.Device.lock();
				SSL_free(Device);
				Device = nullptr;
				Sync.Device.unlock();
			}
#endif
			int Error = 1;
			socklen_t Size = sizeof(Error);
			getsockopt(Fd, SOL_SOCKET, SO_ERROR, (char*)&Error, &Size);
			shutdown(Fd, SD_SEND);

			if (Gracefully)
				return TryClose(this, nullptr, 0, Callback) ? 0 : -1;

			closesocket(Fd);
			Fd = INVALID_SOCKET;
			if (Callback)
				Callback(this);

			return 0;
		}
		int Socket::CloseOnExec()
		{
#if defined(_WIN32)
			return 0;
#else
			return fcntl(Fd, F_SETFD, FD_CLOEXEC);
#endif
		}
		int Socket::Write(const char* Buffer, int Size)
		{
			if (Fd == INVALID_SOCKET)
				return -1;

#ifdef TH_HAS_OPENSSL
			if (Device != nullptr)
			{
				Sync.Device.lock();
				int Value = SSL_write(Device, Buffer, Size);
				Sync.Device.unlock();

				if (Value <= 0)
					return (GetError(Value) == SSL_ERROR_WANT_WRITE ? -2 : -1);

				Outcome += (int64_t)Value;
				return Value;
			}
#endif
			int Value = send(Fd, Buffer, Size, 0);
			if (Value <= 0)
				return (GetError(Value) == ERRWOULDBLOCK ? -2 : -1);

			Outcome += (int64_t)Value;
			return Value;
		}
		int Socket::Write(const char* Buffer, int Size, const SocketWriteCallback& Callback)
		{
			int Offset = 0;
			while (Size > 0)
			{
				int Length = Write(Buffer + Offset, Size);
				if (Length <= 0)
				{
					if (Callback)
						Callback(this, -1);

					return -1;
				}

				Size -= Length;
				Offset += Length;

				if (Callback && !Callback(this, Length))
					break;
			}

			if (Callback)
				Callback(this, 0);

			return Size;
		}
		int Socket::Write(const std::string& Buffer)
		{
			return Write(Buffer.c_str(), (int)Buffer.size());
		}
		int Socket::WriteAsync(const char* Buffer, int64_t Size, SocketWriteCallback&& Callback)
		{
			if (Listener != nullptr)
				return 0;

			if (Output != nullptr)
			{
				if (Callback)
					Callback(this, -1);

				return -1;
			}

			int64_t Offset = 0;
			while (Size > 0)
			{
				int Length = Write(Buffer + Offset, (int)Size);
				if (Length == -2)
				{
					Sync.IO.lock();
					bool OK = WriteSet(std::move(Callback), Buffer + Offset, Size);
					Driver::Listen(this, false);
					Sync.IO.unlock();

					if (!OK && Callback)
						Callback(this, -1);

					return -2;
				}
				else if (Length == -1)
				{
					if (Callback)
						Callback(this, -1);

					return -1;
				}

				Size -= (int64_t)Length;
				Offset += (int64_t)Length;

				if (Callback && !Callback(this, (int64_t)Length))
					break;
			}

			if (Callback)
				Callback(this, 0);

			return (int)Size;
		}
		int Socket::fWrite(const char* Format, ...)
		{
			char Buffer[8192];

			va_list Args;
			va_start(Args, Format);
			int Count = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			return Write(Buffer, (uint64_t)Count);
		}
		int Socket::fWriteAsync(SocketWriteCallback&& Callback, const char* Format, ...)
		{
			char Buffer[8192];

			va_list Args;
			va_start(Args, Format);
			int Count = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			return WriteAsync(Buffer, (int64_t)Count, std::move(Callback));
		}
		int Socket::Read(char* Buffer, int Size)
		{
			if (Fd == INVALID_SOCKET)
				return -1;

#ifdef TH_HAS_OPENSSL
			if (Device != nullptr)
			{
				Sync.Device.lock();
				int Value = SSL_read(Device, Buffer, Size);
				Sync.Device.unlock();

				if (Value <= 0)
					return (GetError(Value) == SSL_ERROR_WANT_READ ? -2 : -1);

				Income += (int64_t)Value;
				return Value;
			}
#endif
			int Value = recv(Fd, Buffer, Size, 0);
			if (Value <= 0)
				return (GetError(Value) == ERRWOULDBLOCK ? -2 : -1);

			Income += (int64_t)Value;
			return Value;
		}
		int Socket::Read(char* Buffer, int Size, const SocketReadCallback& Callback)
		{
			while (Size > 0)
			{
				int Length = Read(Buffer, Size > sizeof(Buffer) ? sizeof(Buffer) : Size);
				if (Length <= 0)
				{
					if (Callback)
						Callback(this, nullptr, -1);

					return -1;
				}

				Size -= Length;
				if (Callback && !Callback(this, Buffer, Length))
					break;
			}

			if (Callback)
				Callback(this, nullptr, 0);

			return Size;
		}
		int Socket::ReadAsync(int64_t Size, SocketReadCallback&& Callback)
		{
			if (Input != nullptr)
			{
				if (!Callback)
					Callback(this, nullptr, -1);

				return -1;
			}

			char Buffer[8192];
			while (Size > 0)
			{
				int Length = Read(Buffer, (int)(Size > sizeof(Buffer) ? sizeof(Buffer) : Size));
				if (Length == -2)
				{
					Sync.IO.lock();
					bool OK = ReadSet(std::move(Callback), nullptr, Size, 0);
					Driver::Listen(this, false);
					Sync.IO.unlock();

					if (!OK && Callback)
						Callback(this, nullptr, -1);

					return -2;
				}
				else if (Length == -1)
				{
					if (Callback)
						Callback(this, nullptr, -1);

					return -1;
				}

				Size -= (int64_t)Length;
				if (Callback && !Callback(this, Buffer, (int64_t)Length))
					break;
			}

			if (Callback)
				Callback(this, nullptr, 0);

			return (int)Size;
		}
		int Socket::ReadUntil(const char* Match, const SocketReadCallback& Callback)
		{
			if (!Match)
				return 0;

			int Index = 0, Size = (int)strlen(Match);
			if (!Size)
				return 0;

			char Buffer = 0;
			while (true)
			{
				int Length = Read(&Buffer, 1);
				if (Length <= 0)
				{
					if (Callback)
						Callback(this, nullptr, -1);

					return -1;
				}

				if (Callback && !Callback(this, &Buffer, 1))
					break;

				if (Match[Index] == Buffer)
				{
					Index++;
					if (Index < Size)
						continue;

					break;
				}
				else
					Index = 0;
			}

			if (Callback)
				Callback(this, nullptr, 0);

			return 0;
		}
		int Socket::ReadUntilAsync(const char* Match, SocketReadCallback&& Callback)
		{
			int64_t Size = (int64_t)(Match ? strlen(Match) : 0);
			if (!Size)
				return 0;

			if (Input != nullptr)
			{
				if (!Callback)
					Callback(this, nullptr, -1);

				return -1;
			}

			char Buffer = 0;
			int64_t Index = 0;
			while (true)
			{
				int Length = Read(&Buffer, 1);
				if (Length == -2)
				{
					Sync.IO.lock();
					bool OK = ReadSet(std::move(Callback), Match, Size, Index);
					Driver::Listen(this, false);
					Sync.IO.unlock();

					if (!OK && Callback)
						Callback(this, nullptr, -1);

					return -2;
				}
				else if (Length == -1)
				{
					if (Callback)
						Callback(this, nullptr, -1);

					return -1;
				}

				if (Callback && !Callback(this, &Buffer, 1))
					break;

				if (Match[Index] == Buffer)
				{
					Index++;
					if (Index < Size)
						continue;

					break;
				}
				else
					Index = 0;
			}

			if (Callback)
				Callback(this, nullptr, 0);

			return 0;
		}
		int Socket::Skip(unsigned int IO, int Code)
		{
			if (Code > 0)
				return -1;

			Sync.IO.lock();
			if (IO & (uint32_t)SocketEvent::Read && Input != nullptr)
			{
				auto Callback = Input->Callback;
				ReadFlush();
				Sync.IO.unlock();
				if (Callback)
					Callback(this, nullptr, Code);
				Sync.IO.lock();
			}

			if (IO & (uint32_t)SocketEvent::Write && Output != nullptr)
			{
				auto Callback = Output->Callback;
				WriteFlush();
				Sync.IO.unlock();
				if (Callback)
					Callback(this, Code);
				Sync.IO.lock();
			}

			Sync.IO.unlock();
			return ((IO & (uint32_t)SocketEvent::Write && Output) || (IO & (size_t)SocketEvent::Read && Input) ? 1 : 0);
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
		int Socket::SetNodelay(bool Enabled)
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
		int Socket::SetAsyncTimeout(int64_t Timeout)
		{
			Sync.Timeout = Timeout;
			return 0;
		}
		int Socket::GetError(int Result)
		{
#ifdef TH_HAS_OPENSSL
			if (!Device)
				return ERRNO;

			Sync.Device.lock();
			int Value = SSL_get_error(Device, Result);
			Sync.Device.unlock();

			return Value;
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
		bool Socket::IsAwaiting()
		{
			return Sync.Poll;
		}
		bool Socket::HasIncomingData()
		{
			return Input != nullptr;
		}
		bool Socket::HasOutcomingData()
		{
			return Output != nullptr;
		}
		bool Socket::HasPendingData()
		{
			return Output || Input;
		}
		bool Socket::ReadSet(SocketReadCallback&& Callback, const char* Match, int64_t Size, int64_t Index)
		{
			if (Input != nullptr)
			{
				TH_ERROR("cannot request read op while another already bound");
				return false;
			}

			Input = TH_NEW(ReadEvent);
			Input->Callback = std::move(Callback);
			Input->Size = Size;
			Input->Index = Index;
			Input->Match = (Match ? strdup(Match) : nullptr);
			Sync.Time = Driver::Clock();

			return true;
		}
		bool Socket::ReadFlush()
		{
			if (!Input)
				return false;

			ReadEvent* It = Input;
			Input = nullptr;
			if (It->Match != nullptr)
				free((void*)It->Match);

			TH_DELETE(ReadEvent, It);
			return true;
		}
		bool Socket::WriteSet(SocketWriteCallback&& Callback, const char* Buffer, int64_t Size)
		{
			if (Output != nullptr)
			{
				TH_ERROR("cannot request write op while another already bound");
				return false;
			}

			Output = TH_NEW(WriteEvent);
			Output->Callback = std::move(Callback);
			Output->Size = Size;
			Output->Buffer = (char*)TH_MALLOC((size_t)Size);
			memcpy(Output->Buffer, Buffer, (size_t)Size);
			Sync.Time = Driver::Clock();

			return true;
		}
		bool Socket::WriteFlush()
		{
			if (!Output)
				return false;

			WriteEvent* It = Output;
			Output = nullptr;
			TH_FREE((void*)It->Buffer);

			TH_DELETE(WriteEvent, It);
			return true;
		}
		bool Socket::TryClose(Socket* Base, const char* Buffer, int64_t Size, const SocketAcceptCallback& Callback)
		{
			if (Size > 0)
				return true;

			if (Size < 0)
			{
				Base->Clear(false);
				closesocket(Base->Fd);
				Base->Fd = INVALID_SOCKET;

				if (Callback)
					Callback(Base);
			}
			else
			{
				Base->ReadAsync(1, [Callback](Socket* Base, const char* Buffer, int64_t Size)
				{
					return TryClose(Base, Buffer, Size, Callback);
				});
			}

			return false;
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
		std::string Socket::LocalIpAddress()
		{
			char Buffer[1024];
			if (gethostname(Buffer, sizeof(Buffer)) == SOCKET_ERROR)
				return std::string();

			struct hostent* Host = gethostbyname(Buffer);
			if (Host == nullptr)
				return std::string();

#ifdef TH_MICROSOFT
			return Core::Form("%i.%i.%i.%i", (int)((struct in_addr*)(Host->h_addr))->S_un.S_un_b.s_b1, (int)((struct in_addr*)(Host->h_addr))->S_un.S_un_b.s_b2, (int)((struct in_addr*)(Host->h_addr))->S_un.S_un_b.s_b3, (int)((struct in_addr*)(Host->h_addr))->S_un.S_un_b.s_b4).R();
#else
			return inet_ntoa(*(struct in_addr*)Host->h_addr_list[0]);
#endif
		}
		int64_t Socket::GetAsyncTimeout()
		{
			return Sync.Timeout;
		}

		SocketConnection::~SocketConnection()
		{
			TH_DELETE(Socket, Stream);
		}
		void SocketConnection::Lock()
		{
			Info.Sync.lock();
			Info.Await = true;
			Info.Sync.unlock();
		}
		void SocketConnection::Unlock()
		{
			Info.Sync.lock();
			Info.Await = false;
			Info.Sync.unlock();
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
			ASN1_digest((int (*)(void*, unsigned char**))i2d_X509, Digest, (char*)Certificate, Buffer, &Size);

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
		bool SocketConnection::Break()
		{
			return Finish(-1);
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

		void Driver::Create(int Length, int64_t Timeout)
		{
			PipeTimeout = Timeout;
			if (Array != nullptr || Handle != INVALID_EPOLL)
				return;

			ArraySize = Length;
#ifdef TH_APPLE
			Handle = kqueue();
			Array = (struct kevent*)TH_MALLOC(sizeof(struct kevent) * ArraySize);
#else
			Handle = epoll_create(1);
			Array = (epoll_event*)TH_MALLOC(sizeof(epoll_event) * ArraySize);
#endif
			Assign(Core::Schedule::Get());
		}
		void Driver::Release()
		{
			if (Handle != INVALID_EPOLL)
			{
				epoll_close(Handle);
				Handle = INVALID_EPOLL;
			}

			if (Array != nullptr)
			{
				TH_FREE(Array);
				Array = nullptr;
			}
		}
		void Driver::Assign(Core::Schedule* Queue)
		{
			if (Queue != nullptr)
			{
				if (!Active)
				{
					Queue->SetTask(Driver::Resolve);
					Active = true;
				}
			}
			else if (Active)
				Active = false;
		}
		void Driver::Resolve()
		{
			Core::Schedule* Queue = Core::Schedule::Get();
			if (!Queue)
				return;

			Dispatch();
			if (Queue->IsActive() && Active)
				Queue->SetTask(Driver::Resolve);
		}
		int Driver::Dispatch()
		{
#ifdef TH_APPLE
			struct kevent* Events = (struct kevent*)Array;
#else
			epoll_event* Events = (epoll_event*)Array;
#endif
			if (!Events)
				return -1;

#ifdef TH_APPLE
			struct timespec Wait;
			Wait.tv_sec = (int)PipeTimeout / 1000;
			Wait.tv_nsec = ((int)PipeTimeout % 1000) * 1000000;

			int Count = kevent(Handle, nullptr, 0, Events, ArraySize, &Wait);
#else
			int Count = epoll_wait(Handle, Events, ArraySize, (int)PipeTimeout);
#endif
			if (Count <= 0)
				return 0;

			int64_t Time = Clock();
			for (auto It = Events; It != Events + Count; It++)
			{
				uint32_t Flags = 0;
#ifdef TH_APPLE
				if (It->filter == EVFILT_READ)
					Flags |= (uint32_t)SocketEvent::Read;

				if (It->filter == EVFILT_WRITE)
					Flags |= (uint32_t)SocketEvent::Write;

				if (It->flags & EV_EOF)
					Flags |= (uint32_t)SocketEvent::Close;

				Socket* Value = (Socket*)It->udata;
#else
				if (It->events & EPOLLIN)
					Flags |= (uint32_t)SocketEvent::Read;

				if (It->events & EPOLLOUT)
					Flags |= (uint32_t)SocketEvent::Write;

				if (It->events & EPOLLHUP || It->events & EPOLLRDHUP || It->events & EPOLLERR)
					Flags |= (uint32_t)SocketEvent::Close;

				Socket* Value = (Socket*)It->data.ptr;
#endif
				Driver::Dispatch(Value, Flags, Time);
			}

			return Count;
		}
		int Driver::Dispatch(Socket* Fd, uint32_t Events, int64_t Time)
		{
			if (!Fd || Fd->Fd == INVALID_SOCKET)
				return 1;

			if (Events & (uint32_t)SocketEvent::Close)
				return Fd->Clear(true) ? 0 : 1;

			Fd->Sync.IO.lock();
			if (Events & (uint32_t)SocketEvent::Read)
			{
				if (Fd->Input != nullptr)
				{
					ReadEvent* Event = Fd->Input;
					auto Callback = std::move(Event->Callback);
					char Buffer[8192];

					while (Event->Size > 0)
					{
						int Size = Fd->Read(Buffer, Event->Match ? 1 : (int)std::min(Event->Size, (int64_t)sizeof(Buffer)));
						if (Size == -1)
						{
							Fd->ReadFlush();
							Fd->Sync.IO.unlock();
							if (Callback)
								Callback(Fd, nullptr, -1);
							Fd->Sync.IO.lock();
							goto ReadEOF;
						}
						else if (Size == -2)
							goto ReadEOF;

						Fd->Sync.IO.unlock();
						bool Done = (Callback && !Callback(Fd, Buffer, (int64_t)Size));
						Fd->Sync.IO.lock();

						if (!Fd->Input || Fd->Input != Event)
							goto ReadEOF;

						if (Done)
							break;

						if (!Event->Match)
						{
							Event->Size -= (int64_t)Size;
							continue;
						}

						if (Event->Match[Event->Index] != Buffer[0])
						{
							Event->Index = 0;
							continue;
						}

						Event->Index++;
						if (Event->Index >= Event->Size)
							break;
					}

					Fd->ReadFlush();
					Fd->Sync.IO.unlock();
					if (Callback)
						Callback(Fd, nullptr, 0);
					Fd->Sync.IO.lock();
				ReadEOF:
					Event = nullptr;
				}
				else if (Fd->Listener != nullptr)
				{
					SocketAcceptCallback Callback = *Fd->Listener;
					Fd->Sync.IO.unlock();
					bool Stop = !Callback(Fd);
					Fd->Sync.IO.lock();

					if (Stop)
					{
						TH_DELETE(SocketAcceptCallback, Fd->Listener);
						Fd->Listener = nullptr;
						Driver::Unlisten(Fd, true);
					}
				}
			}
			else if (Events & (uint32_t)SocketEvent::Write && Fd->Output != nullptr)
			{
				WriteEvent* Event = Fd->Output;
				auto Callback = std::move(Event->Callback);
				int64_t Offset = 0;

				while (Event->Buffer && Event->Size > 0)
				{
					int Size = Fd->Write(Event->Buffer + Offset, (int)Event->Size);
					if (Size == -1)
					{
						Fd->WriteFlush();
						Fd->Sync.IO.unlock();
						if (Callback)
							Callback(Fd, -1);
						Fd->Sync.IO.lock();
						goto WriteEOF;
					}
					else if (Size == -2)
						goto WriteEOF;

					Fd->Sync.IO.unlock();
					bool Done = (Callback && !Callback(Fd, (int64_t)Size));
					Fd->Sync.IO.lock();

					if (!Fd->Output || Fd->Output != Event)
						goto WriteEOF;

					Event->Size -= (int64_t)Size;
					if (Done)
						break;
				}

				Fd->WriteFlush();
				Fd->Sync.IO.unlock();
				if (Callback)
					Callback(Fd, 0);
				Fd->Sync.IO.lock();
			WriteEOF:
				Event = nullptr;
			}

			bool Timeout = (Fd->Sync.Timeout > 0 && Time - Fd->Sync.Time > Fd->Sync.Timeout);
			if (!Fd->Input && !Fd->Output && !Fd->Listener)
				Driver::Unlisten(Fd, false);

			Fd->Sync.IO.unlock();
			if (Timeout)
				Fd->Clear(true);

			return Timeout ? 0 : 1;
		}
		int Driver::Listen(Socket* Value, bool Always)
		{
			if (!Handle || !Value || Value->Fd == INVALID_SOCKET)
				return -1;

			bool Set = Value->Sync.Poll;
			Value->Sync.Time = Clock();
			Value->Sync.Poll = true;
#ifdef TH_APPLE
			struct kevent Event;
			int Result1, Result2;

			if (Always || Value->Input != nullptr || Value->Listener)
			{
				EV_SET(&Event, Value->Fd, EVFILT_READ, EV_ADD, 0, 0, (void*)Value);
				Result1 = kevent(Handle, &Event, 1, nullptr, 0, nullptr);
			}

			if (Always || Value->Output != nullptr)
			{
				EV_SET(&Event, Value->Fd, EVFILT_WRITE, EV_ADD, 0, 0, (void*)Value);
				Result2 = kevent(Handle, &Event, 1, nullptr, 0, nullptr);
			}

			return Result1 == 1 && Result2 == 1 ? 0 : -1;
#else
			epoll_event Event;
			Event.data.ptr = (void*)Value;
			Event.events = EPOLLRDHUP;

			if (Always || Value->Input != nullptr || Value->Listener)
				Event.events |= EPOLLIN;

			if (Always || Value->Output != nullptr)
				Event.events |= EPOLLOUT;

			return epoll_ctl(Handle, Set ? EPOLL_CTL_MOD : EPOLL_CTL_ADD, Value->Fd, &Event);
#endif
		}
		int Driver::Unlisten(Socket* Value, bool Always)
		{
			if (!Handle || !Value || Value->Fd == INVALID_SOCKET || !Value->Sync.Poll)
				return -1;

#ifdef TH_APPLE
			struct kevent Event;
			int Result1, Result2;

			if (Always || Value->Input != nullptr || Value->Listener)
			{
				EV_SET(&Event, Value->Fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
				Result1 = kevent(Handle, &Event, 1, nullptr, 0, nullptr);
			}

			if (Always || Value->Output != nullptr)
			{
				EV_SET(&Event, Value->Fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
				Result2 = kevent(Handle, &Event, 1, nullptr, 0, nullptr);
			}

			return Result1 == 1 && Result2 == 1 ? 0 : -1;
#else
			epoll_event Event;
			Event.data.ptr = (void*)Value;
			Event.events = EPOLLRDHUP;

			if (Always || (!Value->Input && !Value->Output))
			{
				Value->Sync.Poll = false;
				return epoll_ctl(Handle, EPOLL_CTL_DEL, Value->Fd, &Event);
			}

			if (Value->Input != nullptr || Value->Listener)
				Event.events |= EPOLLIN;

			if (Value->Output != nullptr)
				Event.events |= EPOLLOUT;

			return epoll_ctl(Handle, EPOLL_CTL_MOD, Value->Fd, &Event);
#endif
		}
		int Driver::Poll(pollfd* Fd, int FdCount, int Timeout)
		{
#if defined(TH_MICROSOFT)
			return WSAPoll(Fd, FdCount, Timeout);
#else
			return poll(Fd, FdCount, Timeout);
#endif
		}
		int64_t Driver::Clock()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}
#ifdef TH_APPLE
		struct kevent* Driver::Array = nullptr;
#else
		epoll_event* Driver::Array = nullptr;
#endif
		epoll_handle Driver::Handle = INVALID_EPOLL;
		int64_t Driver::PipeTimeout = 200;
		int Driver::ArraySize = 0;
		std::atomic<bool> Driver::Active(false);

		SocketServer::SocketServer()
		{
#ifndef TH_MICROSOFT
			signal(SIGPIPE, SIG_IGN);
#endif
		}
		SocketServer::~SocketServer()
		{
			Unlisten();

			do
			{
				FreeQueued();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			} while (!Bad.empty() || !Good.empty());
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
			if (State != ServerState::Idle)
			{
				TH_ERROR("cannot configure while running");
				return false;
			}

			if (NewRouter != nullptr)
			{
				OnDeallocateRouter(Router);
				Router = NewRouter;
			}
			else if (!(Router = OnAllocateRouter()))
			{
				TH_ERROR("cannot allocate router");
				return false;
			}

			if (!OnConfigure(Router))
				return false;

			if (Router->MaxEvents < 1)
				Router->MaxEvents = 256;

			if (Router->Listeners.empty())
			{
				TH_ERROR("there are no listeners provided");
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

				if (Value->Base->Open(It.second.Hostname.c_str(), It.second.Port, &Value->Source))
				{
					TH_ERROR("cannot open %s:%i", It.second.Hostname.c_str(), (int)It.second.Port);
					return false;
				}

				if (Value->Base->Bind(&Value->Source))
				{
					TH_ERROR("cannot bind %s:%i", It.second.Hostname.c_str(), (int)It.second.Port);
					return false;
				}
				if (Value->Base->Listen((int)Router->BacklogQueue))
				{
					TH_ERROR("cannot listen %s:%i", It.second.Hostname.c_str(), (int)It.second.Port);
					return false;
				}

				Value->Base->CloseOnExec();
				Value->Base->SetBlocking(false);
				Listeners.push_back(Value);

				if (It.second.Port <= 0 && (It.second.Port = Value->Base->GetPort()) < 0)
				{
					TH_ERROR("cannot determine listener's port number");
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
					TH_ERROR("cannot create server's SSL context");
					return false;
				}

				SSL_CTX_clear_options(It.second.Context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
				SSL_CTX_set_options(It.second.Context, Protocol);
				SSL_CTX_set_options(It.second.Context, SSL_OP_SINGLE_DH_USE);
				SSL_CTX_set_options(It.second.Context, SSL_OP_CIPHER_SERVER_PREFERENCE);
#ifdef SSL_CTX_set_ecdh_auto
				SSL_CTX_set_ecdh_auto(It.second.Context, 1);
#endif
				std::string ContextId = Compute::Common::MD5Hash(Core::Parser((int64_t)time(nullptr)).R());
				SSL_CTX_set_session_id_context(It.second.Context, (const unsigned char*)ContextId.c_str(), (unsigned int)ContextId.size());

				if (!It.second.Chain.empty() && !It.second.Key.empty())
				{
					if (SSL_CTX_load_verify_locations(It.second.Context, It.second.Chain.c_str(), It.second.Key.c_str()) != 1)
					{
						TH_ERROR("SSL_CTX_load_verify_locations(): %s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (SSL_CTX_set_default_verify_paths(It.second.Context) != 1)
					{
						TH_ERROR("SSL_CTX_set_default_verify_paths(): %s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (SSL_CTX_use_certificate_file(It.second.Context, It.second.Chain.c_str(), SSL_FILETYPE_PEM) <= 0)
					{
						TH_ERROR("SSL_CTX_use_certificate_file(): %s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (SSL_CTX_use_PrivateKey_file(It.second.Context, It.second.Key.c_str(), SSL_FILETYPE_PEM) <= 0)
					{
						TH_ERROR("SSL_CTX_use_PrivateKey_file(): %s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (!SSL_CTX_check_private_key(It.second.Context))
					{
						TH_ERROR("SSL_CTX_check_private_key(): %s", ERR_error_string(ERR_get_error(), nullptr));
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
						TH_ERROR("SSL_CTX_set_cipher_list(): %s", ERR_error_string(ERR_get_error(), nullptr));
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

			auto* Queue = Core::Schedule::Get();
			Queue->ClearTimeout(Timer);
			Timer = -1;

			Sync.lock();
			State = ServerState::Stopping;
			for (auto It = Good.begin(); It != Good.end(); It++)
			{
				SocketConnection* Base = *It;
				Base->Info.KeepAlive = 0;
				Base->Info.Close = true;
				Base->Stream->SetAsyncTimeout(1);
			}
			Sync.unlock();

			do
			{
				FreeQueued();
				if (!Queue->IsActive())
					Driver::Dispatch();
			} while (!Bad.empty() || !Good.empty());

			if (!OnUnlisten())
				return false;

			for (auto It : Listeners)
			{
				if (It->Base != nullptr)
				{
					It->Base->Close();
					TH_DELETE(Socket, It->Base);
				}

				Address::Free(&It->Source);
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

			Driver::Create((int)Router->MaxEvents, Router->PollTimeout);
			Timer = Core::Schedule::Get()->SetInterval(Router->CloseTimeout, [this]()
				{
					FreeQueued();
					if (State == ServerState::Stopping)
					{
						Sync.lock();
						State = ServerState::Idle;
						Sync.unlock();
					}
				});

			for (auto&& It : Listeners)
			{
				It->Base->AcceptAsync([this, It](Socket*)
					{
						if (State != ServerState::Working)
							return false;

						Accept(It);
						return true;
					});
			}

			return true;
		}
		bool SocketServer::FreeQueued()
		{
			if (Bad.empty())
				return false;

			Sync.lock();
			for (auto It = Bad.begin(); It != Bad.end(); It++)
				OnDeallocate(*It);

			Bad.clear();
			Sync.unlock();

			return true;
		}
		bool SocketServer::Accept(Listener* Host)
		{
			auto Connection = TH_NEW(Socket);
			if (Host->Base->Accept(Connection, nullptr) == -1)
			{
				TH_DELETE(Socket, Connection);
				return false;
			}

			if (Router->MaxConnections > 0 && Good.size() >= Router->MaxConnections)
			{
				Connection->CloseAsync(false, [](Socket* Base)
				{
					TH_DELETE(Socket, Base);
					return true;
				});
				return false;
			}

			if (Router->GracefulTimeWait >= 0)
				Connection->SetTimeWait((int)Router->GracefulTimeWait);

			Connection->CloseOnExec();
			Connection->SetAsyncTimeout(Router->SocketTimeout);
			Connection->SetNodelay(Router->EnableNoDelay);
			Connection->SetKeepAlive(true);
			Connection->SetBlocking(false);

			if (Host->Hostname->Secure && !Protect(Connection, Host))
			{
				Connection->CloseAsync(false, [](Socket* Base)
				{
					TH_DELETE(Socket, Base);
					return true;
				});
				return false;
			}

			auto Base = OnAllocate(Host, Connection);
			if (!Base)
			{
				Connection->CloseAsync(false, [](Socket* Base)
				{
					TH_DELETE(Socket, Base);
					return true;
				});
				return false;
			}

			Base->Info.KeepAlive = (Router->KeepAliveMaxCount > 0 ? Router->KeepAliveMaxCount - 1 : 0);
			Base->Host = Host;
			Base->Stream = Connection;
			Base->Stream->UserData = Base;

			Sync.lock();
			Good.insert(Base);
			Sync.unlock();

			return Core::Schedule::Get()->SetTask([this, Base]()
				{
					OnRequestBegin(Base);
				});
		}
		bool SocketServer::Protect(Socket* Fd, Listener* Host)
		{
			ssl_ctx_st* Context = nullptr;
			if (!OnProtect(Fd, Host, &Context) || !Context)
				return false;

			if (!Fd || Fd->Secure(Context, nullptr) == -1)
				return false;

#ifdef TH_HAS_OPENSSL
			int Result = SSL_set_fd(Fd->GetDevice(), (int)Fd->GetFd());
			if (Result != 1)
				return false;

			pollfd SFd{ };
			SFd.fd = Fd->GetFd();
			SFd.events = POLLIN | POLLOUT;

			int64_t Timeout = Driver::Clock();
			bool OK = true;

			while (OK)
			{
				Result = SSL_accept(Fd->GetDevice());
				if (Result >= 0)
					break;

				if (Driver::Clock() - Timeout > (int64_t)Router->SocketTimeout)
					return false;

				int Code = SSL_get_error(Fd->GetDevice(), Result);
				switch (Code)
				{
					case SSL_ERROR_WANT_CONNECT:
					case SSL_ERROR_WANT_ACCEPT:
					case SSL_ERROR_WANT_WRITE:
					case SSL_ERROR_WANT_READ:
						Driver::Poll(&SFd, 1, Router->SocketTimeout);
						break;
					default:
						OK = false;
						break;
				}
			}

			return Result == 1;
#else
			return false;
#endif
		}
		bool SocketServer::Manage(SocketConnection* Base)
		{
			if (!Base || Base->Info.KeepAlive < -1)
				return false;

			if (!OnRequestEnded(Base, true))
				return false;

			if (Router->KeepAliveMaxCount >= 0)
				Base->Info.KeepAlive--;

			if (Base->Info.KeepAlive >= -1)
				Base->Info.Finish = Driver::Clock();

			if (!OnRequestEnded(Base, false))
				return false;

			Base->Stream->Income = 0;
			Base->Stream->Outcome = 0;

			if (!Base->Info.Close && Base->Info.KeepAlive > -1 && Base->Stream->IsValid())
				return OnRequestBegin(Base);

			Base->Info.KeepAlive = -2;
			Base->Stream->CloseAsync(true, [this, Base](Socket*)
			{
				Sync.lock();
				auto It = Good.find(Base);
				if (It != Good.end())
					Good.erase(It);

				Bad.insert(Base);
				Sync.unlock();
				return true;
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
			if (Router->Certificates.empty())
				return false;

			auto&& It = Router->Certificates.find(Host->Name);
			if (It == Router->Certificates.end())
				return false;

			*Context = It->second.Context;
			return true;
		}
		SocketConnection* SocketServer::OnAllocate(Listener*, Socket*)
		{
			return TH_NEW(SocketConnection);
		}
		SocketRouter* SocketServer::OnAllocateRouter()
		{
			return TH_NEW(SocketRouter);
		}
		std::unordered_set<SocketConnection*>* SocketServer::GetClients()
		{
			return &Good;
		}
		ServerState SocketServer::GetState()
		{
			return State;
		}
		SocketRouter* SocketServer::GetRouter()
		{
			return Router;
		}

		SocketClient::SocketClient(int64_t RequestTimeout) : Context(nullptr), Timeout(RequestTimeout), AutoCertify(true)
		{
			Stream.UserData = this;
		}
		SocketClient::~SocketClient()
		{
			if (Stream.IsValid())
			{
				Stream.SetBlocking(true);
				OnClose();
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
			if (!Address || Address->Hostname.empty() || Stream.IsValid())
				return Core::Async<int>::Store(-2);

			Core::Async<int> Result;
			Done = [Result](SocketClient*, int Code) mutable
			{
				Result.Set(Code);
			};

			Stage("socket dns resolve");
			if (!OnResolveHost(Address))
			{
				Error("cannot resolve host %s:%i", Address->Hostname.c_str(), (int)Address->Port);
				return Result;
			}

			Hostname = *Address;
			Stage("socket open");

			Tomahawk::Network::Address Host;
			if (Stream.Open(Hostname.Hostname.c_str(), Hostname.Port, &Host) == -1)
			{
				Error("cannot open %s:%i", Hostname.Hostname.c_str(), (int)Hostname.Port);
				return Result;
			}

			Stage("socket connect");
			if (Stream.Connect(&Host) == -1)
			{
				Address::Free(&Host);
				Error("cannot connect to %s:%i", Hostname.Hostname.c_str(), (int)Hostname.Port);
				return Result;
			}

			Address::Free(&Host);
			Stream.CloseOnExec();
			Stream.SetBlocking(!Async);
			Stream.SetAsyncTimeout(Timeout);

#ifdef TH_HAS_OPENSSL
			if (Hostname.Secure)
			{
				Stage("socket ssl handshake");
				if (!Context && !(Context = SSL_CTX_new(SSLv23_client_method())))
				{
					Error("cannot create ssl context");
					return Result;
				}

				if (AutoCertify && !Certify())
					return Result;
			}
#endif
			OnConnect();
			return Result;
		}
		Core::Async<int> SocketClient::Close()
		{
			if (!Stream.IsValid())
				return Core::Async<int>::Store(-2);

			Core::Async<int> Result;
			Done = [Result](SocketClient*, int Code) mutable
			{
				Result.Set(Code);
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
			return Stream.CloseAsync(true, [this](Socket*)
			{
				return Success(0);
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
			Fd.events = POLLIN | POLLOUT;

			bool OK = true;
			while (OK)
			{
				Result = SSL_connect(Stream.GetDevice());
				if (Result >= 0)
					break;

				int Code = SSL_get_error(Stream.GetDevice(), Result);
				switch (Code)
				{
					case SSL_ERROR_WANT_CONNECT:
					case SSL_ERROR_WANT_ACCEPT:
					case SSL_ERROR_WANT_WRITE:
					case SSL_ERROR_WANT_READ:
						Driver::Poll(&Fd, 1, Timeout);
						break;
					default:
						OK = false;
						break;
				}
			}

			return Result != 1 ? Error("%s", ERR_error_string(ERR_get_error(), nullptr)) : true;
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

			TH_ERROR("%.*s (at %s)", Size, Buffer, Action.empty() ? "request" : Action.c_str());
			return Stream.CloseAsync(true, [this](Socket*)
			{
				return Success(-1);
			}) == 0;
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
