#include "network.h"
#include "../network/http.h"
#ifdef THAWK_MICROSOFT
#include <WS2tcpip.h>
#include <io.h>
#include <wepoll.h>
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
#ifndef THAWK_APPLE
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
#define SD_BOTH SHUT_RDWR
#endif

extern "C"
{
#ifdef THAWK_HAS_OPENSSL
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
		bool Address::Free(Network::Address* Address)
		{
			if (!Address)
				return false;

			if (Address->Host != nullptr)
				freeaddrinfo(Address->Host);

			Address->Host = nullptr;
			return true;
		}

		Socket::Socket() : Input(nullptr), Output(nullptr), Device(nullptr), Fd(INVALID_SOCKET), Income(0), Outcome(0), UserData(nullptr)
		{
			Sync.Await = false;
			Sync.Timeout = 0;
			Sync.Time = 0;
		}
		int Socket::Open(const char* Host, int Port, SocketType Type, Address* Result)
		{
			struct addrinfo Hints;
			memset(&Hints, 0, sizeof(struct addrinfo));
			Hints.ai_family = AF_UNSPEC;

			switch (Type)
			{
				case SocketType_Stream:
					Hints.ai_socktype = SOCK_STREAM;
					break;
				case SocketType_Datagram:
					Hints.ai_socktype = SOCK_DGRAM;
					break;
				case SocketType_Raw:
					Hints.ai_socktype = SOCK_RAW;
					break;
				case SocketType_Reliably_Delivered_Message:
					Hints.ai_socktype = SOCK_RDM;
					break;
				case SocketType_Sequence_Packet_Stream:
					Hints.ai_socktype = SOCK_SEQPACKET;
					break;
				default:
					Hints.ai_socktype = SOCK_STREAM;
					break;
			}

			struct addrinfo* Address;
			if (getaddrinfo(Host, Rest::Stroke(Port).Get(), &Hints, &Address))
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
			return Open(Host, Port, SocketType_Stream, Result);
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
		int Socket::Secure(ssl_ctx_st* Context)
		{
#ifdef THAWK_HAS_OPENSSL
			Sync.Device.lock();
			if (Device != nullptr)
				SSL_free(Device);

			Device = SSL_new(Context);
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
		int Socket::Accept(Socket* Connection, Address* Output)
		{
			if (!Connection)
				return -1;

			sockaddr Address;
			socket_size_t Length = sizeof(sockaddr);
			Connection->Fd = accept(Fd, &Address, &Length);
			if (Connection->Fd == INVALID_SOCKET)
				return -1;

			if (!Output)
				return 0;

			struct addrinfo Hints;
			memset(&Hints, 0, sizeof(struct addrinfo));
			Hints.ai_family = AF_UNSPEC;
			Hints.ai_socktype = SOCK_STREAM;

			if (getaddrinfo(Address.sa_data, Rest::Stroke((((struct sockaddr_in*)&Address)->sin_port)).Get(), &Hints, &Output->Host))
				return -1;

			Output->Active = Output->Host;
			return 0;
		}
		int Socket::AcceptAsync(const SocketAcceptCallback& Callback)
		{
			Sync.IO.lock();
			Listener = Callback;
			Multiplexer::Listen(this);
			Sync.IO.unlock();

			return 0;
		}
		int Socket::Clear()
		{
			Sync.IO.lock();
			while (Input != nullptr)
				ReadPop();

			while (Output != nullptr)
				WritePop();

			Multiplexer::Unlisten(this);
			Sync.IO.unlock();

			return 0;
		}
		int Socket::Close(bool Detach)
		{
			if (Detach)
				Clear();

			if (Fd == INVALID_SOCKET)
				return -1;

#ifdef THAWK_HAS_OPENSSL
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

			linger Linger;
			Linger.l_onoff = 1;
			Linger.l_linger = (TimeWait > 0 ? TimeWait : 0);
			setsockopt(Fd, SOL_SOCKET, SO_LINGER, (char*)&Linger, sizeof(Linger));

			shutdown(Fd, SD_BOTH);
			closesocket(Fd);
			return (int)(Fd = INVALID_SOCKET);
		}
		int Socket::CloseOnExec()
		{
#if defined(_WIN32)
			return (int)SetHandleInformation((HANDLE)(intptr_t)Fd, HANDLE_FLAG_INHERIT, 0);
#else
			return fcntl(Fd, F_SETFD, FD_CLOEXEC);
#endif
		}
		int Socket::Write(const char* Buffer, int Size)
		{
			if (Fd == INVALID_SOCKET)
				return -1;

#ifdef THAWK_HAS_OPENSSL
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
		int Socket::WriteAsync(const char* Buffer, int64_t Size, const SocketWriteCallback& Callback)
		{
			if (Listener)
				return 0;

			if (Output != nullptr)
			{
				Sync.IO.lock();
				WritePush(Callback, Buffer, Size);
				Sync.IO.unlock();

				return 0;
			}

			int64_t Offset = 0;
			while (Size > 0)
			{
				int Length = Write(Buffer + Offset, (int)Size);
				if (Length == -2)
				{
					Sync.IO.lock();
					WritePush(Callback, Buffer + Offset, Size);
					Multiplexer::Listen(this);
					Sync.IO.unlock();

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
		int Socket::fWriteAsync(const SocketWriteCallback& Callback, const char* Format, ...)
		{
			char Buffer[8192];

			va_list Args;
					va_start(Args, Format);
			int Count = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
					va_end(Args);

			return WriteAsync(Buffer, (int64_t)Count, Callback);
		}
		int Socket::Read(char* Buffer, int Size)
		{
			if (Fd == INVALID_SOCKET)
				return -1;

#ifdef THAWK_HAS_OPENSSL
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
		int Socket::ReadAsync(int64_t Size, const SocketReadCallback& Callback)
		{
			if (Input != nullptr)
			{
				Sync.IO.lock();
				ReadPush(Callback, nullptr, Size, 0);
				Sync.IO.unlock();

				return 0;
			}

			char Buffer[8192];
			while (Size > 0)
			{
				int Length = Read(Buffer, (int)(Size > sizeof(Buffer) ? sizeof(Buffer) : Size));
				if (Length == -2)
				{
					Sync.IO.lock();
					ReadPush(Callback, nullptr, Size, 0);
					Multiplexer::Listen(this);
					Sync.IO.unlock();

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
		int Socket::ReadUntilAsync(const char* Match, const SocketReadCallback& Callback)
		{
			int64_t Size = (int64_t)(Match ? strlen(Match) : 0);
			if (!Size)
				return 0;

			char Buffer = 0;
			if (Input != nullptr)
			{
				Sync.IO.lock();
				ReadPush(Callback, Match, Size, 0);
				Sync.IO.unlock();

				return 0;
			}

			int64_t Index = 0;
			while (true)
			{
				int Length = Read(&Buffer, 1);
				if (Length == -2)
				{
					Sync.IO.lock();
					ReadPush(Callback, Match, Size, Index);
					Multiplexer::Listen(this);
					Sync.IO.unlock();

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
		int Socket::Skip(int IO, int Code)
		{
			if (Code > 0)
				return -1;

			Sync.IO.lock();
			if (IO & SocketEvent_Read && Input != nullptr)
			{
				auto Callback = Input->Callback;
				ReadPop();
				Sync.IO.unlock();
				if (Callback)
					Callback(this, nullptr, Code);
				Sync.IO.lock();
			}

			if (IO & SocketEvent_Write && Output != nullptr)
			{
				auto Callback = Output->Callback;
				WritePop();
				Sync.IO.unlock();
				if (Callback)
					Callback(this, Code);
				Sync.IO.lock();
			}

			Sync.IO.unlock();
			return ((IO & SocketEvent_Write && Output) || (IO & SocketEvent_Read && Input) ? 1 : 0);
		}
		int Socket::SetTimeWait(int Timeout)
		{
			TimeWait = Timeout;
			return 0;
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
#ifdef THAWK_MICROSOFT
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
#ifdef THAWK_MICROSOFT
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
#ifdef THAWK_HAS_OPENSSL
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
			return Sync.Await;
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
		void Socket::ReadPush(const SocketReadCallback& Callback, const char* Match, int64_t Size, int64_t Index)
		{
			ReadEvent* Event = new ReadEvent();
			Event->Callback = Callback;
			Event->Size = Size;
			Event->Index = Index;
			Event->Match = (Match ? strdup(Match) : nullptr);

			if (Input != nullptr)
			{
				ReadEvent* It = Input;
				while (It != nullptr && It->Next != nullptr)
					It = It->Next;

				Event->Prev = It;
				Event->Next = nullptr;
				It->Next = Event;
			}
			else
			{
				Event->Prev = nullptr;
				Event->Next = nullptr;
				Input = Event;
			}

			Sync.Time = Multiplexer::Clock();
		}
		void Socket::ReadPop()
		{
			if (!Input)
				return;

			ReadEvent* It = Input;
			Input = It->Next;

			if (Input != nullptr)
				Input->Prev = nullptr;

			if (It->Match)
				free((void*)It->Match);

			delete It;
		}
		void Socket::WritePush(const SocketWriteCallback& Callback, const char* Buffer, int64_t Size)
		{
			WriteEvent* Event = new WriteEvent();
			Event->Callback = Callback;
			Event->Size = Size;
			Event->Buffer = (char*)malloc((size_t)Size);
			memcpy(Event->Buffer, Buffer, (size_t)Size);

			if (Output != nullptr)
			{
				WriteEvent* It = Output;
				while (It != nullptr && It->Next != nullptr)
					It = It->Next;

				Event->Prev = It;
				Event->Next = nullptr;
				It->Next = Event;
			}
			else
			{
				Event->Prev = nullptr;
				Event->Next = nullptr;
				Output = Event;
			}

			Sync.Time = Multiplexer::Clock();
		}
		void Socket::WritePop()
		{
			if (!Output)
				return;

			WriteEvent* It = Output;
			Output = It->Next;

			if (Output != nullptr)
				Input->Prev = nullptr;

			if (It->Buffer)
				free((void*)It->Buffer);

			delete It;
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

#ifdef THAWK_MICROSOFT
			return Rest::Form("%i.%i.%i.%i", (int)((struct in_addr*)(Host->h_addr))->S_un.S_un_b.s_b1, (int)((struct in_addr*)(Host->h_addr))->S_un.S_un_b.s_b2, (int)((struct in_addr*)(Host->h_addr))->S_un.S_un_b.s_b3, (int)((struct in_addr*)(Host->h_addr))->S_un.S_un_b.s_b4).R();
#else
			return inet_ntoa(*(struct in_addr *)Host->h_addr_list[0]);
#endif
		}
		int64_t Socket::GetAsyncTimeout()
		{
			return Sync.Timeout;
		}

		SocketConnection::~SocketConnection()
		{
			if (Stream != nullptr)
			{
				delete Stream;
				Stream = nullptr;
			}
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
			if (Info.Error)
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
#ifdef THAWK_HAS_OPENSSL
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

				if (!Compute::MathCommon::HexToString(Buffer, (uint64_t)Size, SerialBuffer, sizeof(SerialBuffer)))
					*SerialBuffer = '\0';
			}
			else
				*SerialBuffer = '\0';

			unsigned int Size = 0;
			ASN1_digest((int (*)(void*, unsigned char**))i2d_X509, Digest, (char*)Certificate, Buffer, &Size);

			char FingerBuffer[1024];
			if (!Compute::MathCommon::HexToString(Buffer, (uint64_t)Size, FingerBuffer, sizeof(FingerBuffer)))
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
#ifdef THAWK_HAS_OPENSSL
			for (auto It = Certificates.begin(); It != Certificates.end(); It++)
			{
				if (!It->second.Context)
					continue;

				SSL_CTX_free(It->second.Context);
				It->second.Context = nullptr;
			}
#endif
		}

		void Multiplexer::Create(int Length)
		{
			if (Array != nullptr || Handle != INVALID_EPOLL)
				return;

			ArraySize = Length;
#ifdef THAWK_APPLE
			Handle = kqueue();
			Array = (struct kevent*)malloc(sizeof(struct kevent) * ArraySize);
#else
			Handle = epoll_create(1);
			Array = (epoll_event*)malloc(sizeof(epoll_event) * ArraySize);
#endif
		}
		void Multiplexer::Release()
		{
			if (Handle != INVALID_EPOLL)
			{
				epoll_close(Handle);
				Handle = INVALID_EPOLL;
			}

			if (Array != nullptr)
			{
				free(Array);
				Array = nullptr;
			}

			if (Loop != nullptr)
				Loop = nullptr;
		}
		void Multiplexer::Dispatch()
		{
			if (!Loop)
				Worker(nullptr, nullptr);
		}
		void Multiplexer::Worker(Rest::EventQueue* Queue, Rest::EventArgs* Args)
		{
#ifdef THAWK_APPLE
			struct kevent* Events = (struct kevent*)Array;
#else
			epoll_event* Events = (epoll_event*)Array;
#endif
			if (!Events)
				return;

			do
			{
#ifdef THAWK_APPLE
				struct timespec Wait;
				Wait.tv_sec = (int)PipeTimeout / 1000;
				Wait.tv_nsec = ((int)PipeTimeout % 1000) * 1000000;

				int Count = kevent(Handle, nullptr, 0, Events, ArraySize, &Wait);
#else
				int Count = epoll_wait(Handle, Events, ArraySize, (int)PipeTimeout);
#endif
				if (Count <= 0)
					continue;

				int64_t Time = Clock();
				int Size = 0;
				for (auto It = Events; It != Events + Count; It++)
				{
					int Flags = 0;
#ifdef THAWK_APPLE
					if (It->filter == EVFILT_READ)
						Flags |= SocketEvent_Read;

					if (It->filter == EVFILT_WRITE)
						Flags |= SocketEvent_Write;

					if (It->flags & EV_EOF)
						Flags |= SocketEvent_Close;

					Socket* Value = (Socket*)It->udata;
#else
					if (It->events & EPOLLIN)
						Flags |= SocketEvent_Read;

					if (It->events & EPOLLOUT)
						Flags |= SocketEvent_Write;

					if (It->events & EPOLLHUP)
						Flags |= SocketEvent_Close;

					if (It->events & EPOLLRDHUP)
						Flags |= SocketEvent_Close;

					if (It->events & EPOLLERR)
						Flags |= SocketEvent_Close;

					Socket* Value = (Socket*)It->data.ptr;
#endif
					if (Dispatch(Value, &Flags, Time) != 0)
						Size++;
				}

				if (!Size)
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}while (Queue == Loop && Args);
		}
		bool Multiplexer::Create(int Length, int64_t Timeout, Rest::EventQueue* Queue)
		{
			Create(Length);
			return Bind(Timeout, Queue);
		}
		bool Multiplexer::Bind(int64_t Timeout, Rest::EventQueue* Queue)
		{
			if (Loop != nullptr && Queue != nullptr)
				return false;

			PipeTimeout = Timeout;
			if ((Loop = Queue) != nullptr)
				Loop->Task<Multiplexer>(nullptr, Multiplexer::Worker);

			return true;
		}
		int Multiplexer::Listen(Socket* Value)
		{
			if (!Handle || !Value || Value->Sync.Await || Value->Fd == INVALID_SOCKET)
				return -1;

			Value->Sync.Time = Clock();
			Value->Sync.Await = true;

#ifdef THAWK_APPLE
			struct kevent Event;
			EV_SET(&Event, Value->Fd, EVFILT_READ, EV_ADD, 0, 0, (void*)Value);
			kevent(Handle, &Event, 1, nullptr, 0, nullptr);

			EV_SET(&Event, Value->Fd, EVFILT_WRITE, EV_ADD, 0, 0, (void*)Value);
			return kevent(Handle, &Event, 1, nullptr, 0, nullptr);
#else
			epoll_event Event;
			Event.data.ptr = (void*)Value;
			if (!Value->Listener)
				Event.events = EPOLLRDHUP | EPOLLIN | EPOLLOUT;
			else
				Event.events = EPOLLIN;

			return epoll_ctl(Handle, EPOLL_CTL_ADD, Value->Fd, &Event);
#endif
		}
		int Multiplexer::Unlisten(Socket* Value)
		{
			if (!Handle || !Value || Value->Fd == INVALID_SOCKET || !Value->Sync.Await)
				return -1;

			Value->Sync.Await = false;

#ifdef THAWK_APPLE
			struct kevent Event;
			EV_SET(&Event, Value->Fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
			kevent(Handle, &Event, 1, nullptr, 0, nullptr);
			EV_SET(&Event, Value->Fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
			kevent(Handle, &Event, 1, nullptr, 0, nullptr);

			return 0;
#else
			return epoll_ctl(Handle, EPOLL_CTL_DEL, Value->Fd, nullptr);
#endif
		}
		int Multiplexer::Dispatch(Socket* Fd, int* Events, int64_t Time)
		{
			if (!Fd || Fd->Fd == INVALID_SOCKET)
				return 1;

			if (*Events & SocketEvent_Read && !(*Events & SocketEvent_Close))
			{
				if (Fd->Input != nullptr)
				{
					char Buffer[8192];
					int Result = 0;
					while (Result != -1)
					{
						Fd->Sync.IO.lock();
						if (!Fd->Input)
						{
							Fd->Sync.IO.unlock();
							break;
						}

						auto Callback = Fd->Input->Callback;
						ReadEvent* Event = Fd->Input;
						while (Event->Size > 0)
						{
							int Size = Fd->Read(Buffer, Event->Match ? 1 : (int)std::min(Event->Size, (int64_t)sizeof(Buffer)));
							if (Size == -1)
							{
								while (Fd->Input != nullptr)
									Fd->ReadPop();

								Fd->Sync.IO.unlock();
								if (Callback)
									Callback(Fd, nullptr, -1);

								*Events = SocketEvent_Close;
								Result = -1;
								break;
							}
							else if (Size == -2)
							{
								Fd->Sync.IO.unlock();
								Result = -1;
								break;
							}

							Fd->Sync.IO.unlock();
							bool Done = (Callback && !Callback(Fd, Buffer, (int64_t)Size));
							Fd->Sync.IO.lock();

							if (!Fd->Input || Fd->Input != Event)
							{
								Fd->Sync.IO.unlock();
								Result = -1;
								break;
							}

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

						if (Result == -1)
							break;

						Fd->ReadPop();
						Fd->Sync.IO.unlock();
						if (Callback)
							Callback(Fd, nullptr, 0);
					}
				}
				else if (Fd->Listener)
				{
					Fd->Sync.IO.lock();
					SocketAcceptCallback Callback = Fd->Listener;
					Fd->Sync.IO.unlock();

					if (!Callback(Fd))
					{
						Fd->Sync.IO.lock();
						Multiplexer::Unlisten(Fd);
						Fd->Listener = nullptr;
						Fd->Sync.IO.unlock();
					}
				}
			}

			if (*Events & SocketEvent_Write && !(*Events & SocketEvent_Close))
			{
				if (Fd->Output != nullptr)
				{
					int Result = 0;
					while (Result != -1)
					{
						Fd->Sync.IO.lock();
						if (!Fd->Output)
						{
							Fd->Sync.IO.unlock();
							break;
						}

						auto Callback = Fd->Output->Callback;
						WriteEvent* Event = Fd->Output;
						int64_t Offset = 0;
						while (Event->Buffer && Event->Size > 0)
						{
							int Size = Fd->Write(Event->Buffer + Offset, (int)Event->Size);
							if (Size == -1)
							{
								while (Fd->Output != nullptr)
									Fd->WritePop();

								Fd->Sync.IO.unlock();
								if (Callback)
									Callback(Fd, -1);

								*Events = SocketEvent_Close;
								Result = -1;
								break;
							}
							else if (Size == -2)
							{
								Fd->Sync.IO.unlock();
								Result = -1;
								break;
							}

							Fd->Sync.IO.unlock();
							bool Done = (Callback && !Callback(Fd, (int64_t)Size));
							Fd->Sync.IO.lock();

							if (!Fd->Output || Fd->Output != Event)
							{
								Fd->Sync.IO.unlock();
								Result = -1;
								break;
							}

							Event->Size -= (int64_t)Size;
							if (Done)
								break;
						}

						Fd->WritePop();
						Fd->Sync.IO.unlock();
						if (Callback)
							Callback(Fd, 0);
					}
				}
			}

			Fd->Sync.IO.lock();
			if (!Fd->Input && !Fd->Output && !Fd->Listener)
			{
				Multiplexer::Unlisten(Fd);
				Fd->Sync.IO.unlock();
				return 1;
			}

			if (!(*Events & SocketEvent_Close))
			{
				if (Fd->Sync.Timeout <= 0 || Time - Fd->Sync.Time <= Fd->Sync.Timeout)
				{
					Fd->Sync.IO.unlock();
					return 0;
				}

				*Events = SocketEvent_Timeout;
			}

			Multiplexer::Unlisten(Fd);
			Fd->Sync.IO.unlock();
			while (Fd->Skip(SocketEvent_Read | SocketEvent_Write, -2) == 1);

			return 1;
		}
		int Multiplexer::Poll(pollfd* Fd, int FdCount, int Timeout)
		{
#if defined(THAWK_MICROSOFT)
			return WSAPoll(Fd, FdCount, Timeout);
#else
			return poll(Fd, FdCount, Timeout);
#endif
		}
		Rest::EventQueue* Multiplexer::GetQueue()
		{
			return Loop;
		}
		int64_t Multiplexer::Clock()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}
#ifdef THAWK_APPLE
		struct kevent* Multiplexer::Array = nullptr;
#else
		epoll_event* Multiplexer::Array = nullptr;
#endif
		Rest::EventQueue* Multiplexer::Loop = nullptr;
		epoll_handle Multiplexer::Handle = INVALID_EPOLL;
		int64_t Multiplexer::PipeTimeout = 200;
		int Multiplexer::ArraySize = 0;

		SocketServer::SocketServer()
		{
#ifndef THAWK_MICROSOFT
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
			}while (!Bad.empty() || !Good.empty());
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
			if (State != ServerState_Idle)
			{
				THAWK_ERROR("cannot configure while running");
				return false;
			}

			if (NewRouter != nullptr)
			{
				OnDeallocateRouter(Router);
				Router = NewRouter;
			}
			else if (!(Router = OnAllocateRouter()))
			{
				THAWK_ERROR("cannot allocate router");
				return false;
			}

			if (!OnConfigure(Router))
				return false;

			if (Router->MaxEvents < 1)
				Router->MaxEvents = 256;

			if (Router->Listeners.empty())
			{
				THAWK_ERROR("there are no listeners provided");
				return false;
			}

			for (auto&& It : Router->Listeners)
			{
				if (It.second.Hostname.empty())
					continue;

				Listener* Value = new Listener();
				Value->Hostname = &It.second;
				Value->Name = It.first;
				Value->Base = new Socket();
				Value->Base->UserData = this;

				if (Value->Base->Open(It.second.Hostname.c_str(), It.second.Port, &Value->Source))
				{
					THAWK_ERROR("cannot open %s:%i", It.second.Hostname.c_str(), (int)It.second.Port);
					return false;
				}

				if (Value->Base->Bind(&Value->Source))
				{
					THAWK_ERROR("cannot bind %s:%i", It.second.Hostname.c_str(), (int)It.second.Port);
					return false;
				}
				if (Value->Base->Listen((int)Router->BacklogQueue))
				{
					THAWK_ERROR("cannot listen %s:%i", It.second.Hostname.c_str(), (int)It.second.Port);
					return false;
				}

				Value->Base->CloseOnExec();
				Value->Base->SetBlocking(false);
				Listeners.push_back(Value);

				if (It.second.Port <= 0 && (It.second.Port = Value->Base->GetPort()) < 0)
				{
					THAWK_ERROR("cannot determine listener's port number");
					return false;
				}
			}

#ifdef THAWK_HAS_OPENSSL
			for (auto&& It : Router->Certificates)
			{
				long Protocol = SSL_OP_ALL;
				switch (It.second.Protocol)
				{
					case Secure_SSL_V2:
						Protocol = SSL_OP_ALL | SSL_OP_NO_SSLv2;
					case Secure_SSL_V3:
						Protocol = SSL_OP_ALL | SSL_OP_NO_SSLv3;
					case Secure_TLS_V1:
						Protocol = SSL_OP_ALL | SSL_OP_NO_TLSv1;
					case Secure_TLS_V1_1:
						Protocol = SSL_OP_ALL | SSL_OP_NO_TLSv1_1;
					case Secure_Any:
					default:
						Protocol = SSL_OP_ALL;
						break;
				}

				if (!(It.second.Context = SSL_CTX_new(SSLv23_server_method())))
				{
					THAWK_ERROR("cannot create server's SSL context");
					return false;
				}

				SSL_CTX_clear_options(It.second.Context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
				SSL_CTX_set_options(It.second.Context, Protocol);
				SSL_CTX_set_options(It.second.Context, SSL_OP_SINGLE_DH_USE);
				SSL_CTX_set_options(It.second.Context, SSL_OP_CIPHER_SERVER_PREFERENCE);
#ifdef SSL_CTX_set_ecdh_auto
				SSL_CTX_set_ecdh_auto(It.second.Context, 1);
#endif
				std::string ContextId = Compute::MathCommon::MD5Hash(Rest::Stroke((int64_t)time(nullptr)).R());
				SSL_CTX_set_session_id_context(It.second.Context, (const unsigned char*)ContextId.c_str(), (unsigned int)ContextId.size());

				if (!It.second.Chain.empty() && !It.second.Key.empty())
				{
					if (SSL_CTX_load_verify_locations(It.second.Context, It.second.Chain.c_str(), It.second.Key.c_str()) != 1)
					{
						THAWK_ERROR("SSL_CTX_load_verify_locations(): %s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (SSL_CTX_set_default_verify_paths(It.second.Context) != 1)
					{
						THAWK_ERROR("SSL_CTX_set_default_verify_paths(): %s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (SSL_CTX_use_certificate_file(It.second.Context, It.second.Chain.c_str(), SSL_FILETYPE_PEM) <= 0)
					{
						THAWK_ERROR("SSL_CTX_use_certificate_file(): %s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (SSL_CTX_use_PrivateKey_file(It.second.Context, It.second.Key.c_str(), SSL_FILETYPE_PEM) <= 0)
					{
						THAWK_ERROR("SSL_CTX_use_PrivateKey_file(): %s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}

					if (!SSL_CTX_check_private_key(It.second.Context))
					{
						THAWK_ERROR("SSL_CTX_check_private_key(): %s", ERR_error_string(ERR_get_error(), nullptr));
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
						THAWK_ERROR("SSL_CTX_set_cipher_list(): %s", ERR_error_string(ERR_get_error(), nullptr));
						return false;
					}
				}
			}
#endif
			return true;
		}
		bool SocketServer::Unlisten()
		{
			if (!Router && State == ServerState_Idle)
				return false;

			if (Queue != nullptr && Worker != nullptr)
			{
				Queue->Expire(Worker->Id);
				Worker = nullptr;
			}

			State = ServerState_Stopping;
			Sync.lock();
			for (auto It = Good.begin(); It != Good.end(); It++)
			{
				SocketConnection* Base = *It;
				Base->Info.KeepAlive = 0;
				Base->Info.Error = true;
				Base->Stream->SetAsyncTimeout(1);
				Base->Stream->SetTimeWait(2);
			}
			Sync.unlock();

			do
			{
				FreeQueued();
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}while (!Bad.empty() || !Good.empty());

			if (!OnUnlisten())
				return false;

			for (auto It : Listeners)
			{
				if (It->Base != nullptr)
				{
					It->Base->Close(true);
					delete It->Base;
				}

				Address::Free(&It->Source);
				delete It;
			}

			OnDeallocateRouter(Router);
			Router = nullptr;
			State = ServerState_Idle;

			if (Multiplexer::GetQueue() == Queue)
				Multiplexer::Bind(0, nullptr);

			Queue = nullptr;
			return true;
		}
		bool SocketServer::Listen(Rest::EventQueue* Ref)
		{
			if (Listeners.empty() || State != ServerState_Idle || !(Queue = Ref))
				return false;

			State = ServerState_Working;
			if (!OnListen(Ref))
				return false;

			Multiplexer::Create((int)Router->MaxEvents, Router->MasterTimeout, Queue);
			Queue->Interval<SocketServer>(this, Router->CloseTimeout, [this](Rest::EventQueue*, Rest::EventArgs* Args)
			{
				FreeQueued();
				if (State == ServerState_Working)
					return;

				Sync.lock();
				State = ServerState_Idle;
				Args->Alive = false;
				Sync.unlock();
			}, &Worker);

			for (auto&& It : Listeners)
			{
				It->Base->AcceptAsync([this, It](Socket*)
				{
					if (State != ServerState_Working)
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
			auto Connection = new Socket();
			if (Host->Base->Accept(Connection, nullptr) == -1)
			{
				delete Connection;
				return false;
			}

			if (Router->MaxConnections > 0 && Good.size() >= Router->MaxConnections)
			{
				Connection->SetTimeWait(0);
				Connection->Close(true);
				delete Connection;

				return false;
			}

			Connection->CloseOnExec();
			Connection->SetAsyncTimeout(Router->SocketTimeout);
			Connection->SetNodelay(Router->EnableNoDelay);
			Connection->SetKeepAlive(true);
			Connection->SetBlocking(false);

			if (Host->Hostname->Secure && !Protect(Connection, Host))
			{
				Connection->Close(true);
				delete Connection;

				return false;
			}

			auto Base = OnAllocate(Host, Connection);
			if (!Base)
			{
				Connection->Close(true);
				delete Connection;

				return false;
			}

			Base->Info.KeepAlive = (Router->KeepAliveMaxCount > 0 ? (int)Router->KeepAliveMaxCount : 1) - 1;
			Base->Host = Host;
			Base->Stream = Connection;
			Base->Stream->UserData = Base;

			Sync.lock();
			Good.insert(Base);
			Sync.unlock();

			return Queue->Callback<SocketConnection>(Base, [this](Rest::EventQueue*, Rest::EventArgs* Args)
			{
				OnRequestBegin(Args->Get<SocketConnection>());
			});
		}
		bool SocketServer::Protect(Socket* Fd, Listener* Host)
		{
			ssl_ctx_st* Context = nullptr;
			if (!OnProtect(Fd, Host, &Context) || !Context)
				return false;

			if (!Fd || Fd->Secure(Context) == -1)
				return false;

#ifdef THAWK_HAS_OPENSSL
			int Result = SSL_set_fd(Fd->GetDevice(), (int)Fd->GetFd());
			if (Result != 1)
				return false;

			pollfd SFd{ };
			SFd.fd = Fd->GetFd();
			SFd.events = POLLIN | POLLOUT;

			int64_t Timeout = Multiplexer::Clock();
			bool OK = true;

			while (OK)
			{
				Result = SSL_accept(Fd->GetDevice());
				if (Result >= 0)
					break;

				if (Multiplexer::Clock() - Timeout > (int64_t)Router->SocketTimeout)
					return false;

				int Code = SSL_get_error(Fd->GetDevice(), Result);
				switch (Code)
				{
					case SSL_ERROR_WANT_CONNECT:
					case SSL_ERROR_WANT_ACCEPT:
					case SSL_ERROR_WANT_WRITE:
					case SSL_ERROR_WANT_READ:
						Multiplexer::Poll(&SFd, 1, Router->SocketTimeout);
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

			Base->Info.KeepAlive--;
			if (Base->Info.KeepAlive >= -1)
				Base->Info.Finish = Multiplexer::Clock();

			if (!OnRequestEnded(Base, false))
				return false;

			Base->Stream->Income = 0;
			Base->Stream->Outcome = 0;

			if (!Base->Info.Error && Base->Info.KeepAlive > -1 && Base->Stream->IsValid())
				return OnRequestBegin(Base);

			Base->Stream->Close(true);
			Base->Info.KeepAlive = -2;

			Sync.lock();
			auto It = Good.find(Base);
			if (It != Good.end())
				Good.erase(It);

			Bad.insert(Base);
			Sync.unlock();

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

			delete Base;
			return true;
		}
		bool SocketServer::OnDeallocateRouter(SocketRouter* Base)
		{
			if (!Base)
				return false;

			delete Base;
			return true;
		}
		bool SocketServer::OnListen(Rest::EventQueue*)
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
			return new SocketConnection();
		}
		SocketRouter* SocketServer::OnAllocateRouter()
		{
			return new SocketRouter();
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
		Rest::EventQueue* SocketServer::GetQueue()
		{
			return Queue;
		}

		SocketClient::SocketClient(int64_t RequestTimeout) : Timeout(RequestTimeout), Context(nullptr), AutoCertify(true)
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

#ifdef THAWK_HAS_OPENSSL
			if (Context != nullptr)
			{
				SSL_CTX_free(Context);
				Context = nullptr;
			}
#endif
		}
		bool SocketClient::Connect(Host* Address, const SocketClientCallback& Callback)
		{
			if (!Address || Address->Hostname.empty() || Stream.IsValid())
			{
				if (Callback)
					Callback(this, -2);

				return false;
			}

			Stage("socket dns resolve");
			Done = Callback;

			if (!OnResolveHost(Address))
				return Error("cannot resolve host %s:%i", Address->Hostname.c_str(), (int)Address->Port);

			Hostname = *Address;
			Stage("socket open");

			Tomahawk::Network::Address Host;
			if (Stream.Open(Hostname.Hostname.c_str(), Hostname.Port, &Host) == -1)
				return Error("cannot open %s:%i", Hostname.Hostname.c_str(), (int)Hostname.Port);

			Stage("socket connect");
			if (Stream.Connect(&Host) == -1)
			{
				Address::Free(&Host);
				return Error("cannot connect to %s:%i", Hostname.Hostname.c_str(), (int)Hostname.Port);
			}

			Address::Free(&Host);
			Stream.CloseOnExec();
			Stream.SetBlocking(false);
			Stream.SetAsyncTimeout(Timeout);

#ifdef THAWK_HAS_OPENSSL
			if (Hostname.Secure)
			{
				Stage("socket ssl handshake");
				if (!Context && !(Context = SSL_CTX_new(SSLv23_client_method())))
					return Error("cannot create ssl context");

				if (AutoCertify && !Certify())
					return false;
			}
#endif

			return OnConnect();
		}
		bool SocketClient::Close(const SocketClientCallback& Callback)
		{
			if (!Stream.IsValid())
			{
				if (Callback)
					Callback(this, -2);

				return false;
			}

			Done = Callback;
			return OnClose();
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
			Stream.Close(true);
			return Success(0);
		}
		bool SocketClient::Certify()
		{
			Stage("ssl handshake");
			if (Stream.GetDevice() || !Context)
				return Error("client does not use ssl");

			if (Stream.Secure(Context) == -1)
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
						Multiplexer::Poll(&Fd, 1, Timeout);
						break;
					default:
						OK = false;
						break;
				}
			}

			return Result != 1 ? Error("%s", ERR_error_string(ERR_get_error(), nullptr)) : true;
		}
		bool SocketClient::Stage(const std::string& Name)
		{
			Action = Name;
			return !Action.empty();
		}
		bool SocketClient::Error(const char* Format, ...)
		{
			char Buffer[16384];
			va_list Args;
					va_start(Args, Format);
			int Size = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
					va_end(Args);

			THAWK_ERROR("%.*s (at %s)", Size, Buffer, Action.empty() ? "request" : Action.c_str());
			Stream.Close(true);

			return !Success(-1);
		}
		bool SocketClient::Success(int Code)
		{
			auto Callback = Done;
			Done = nullptr;

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