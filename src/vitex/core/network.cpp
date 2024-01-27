#include "network.h"
#include "../network/http.h"
#ifdef VI_MICROSOFT
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <io.h>
#ifdef VI_WEPOLL
#include <wepoll.h>
#endif
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
#ifndef VI_APPLE
#include <sys/epoll.h>
#include <sys/sendfile.h>
#else
#include <sys/event.h>
#endif
#include <fcntl.h>
#include <poll.h>
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
#ifdef VI_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
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
#if defined(VI_MICROSOFT) && !defined(VI_WEPOLL)
		struct epoll_socket
		{
			pollfd Fd;
			void* Data = nullptr;
		};

		struct epoll_array
		{
			Core::UnorderedMap<socket_t, epoll_socket> Fds;
			Core::Vector<pollfd> Events;
			std::mutex Mutex;
			bool Dirty = true;

			void Upsert(socket_t Fd, bool Readable, bool Writeable, void* Data)
			{
				Core::UMutex<std::mutex> Unique(Mutex);
				auto& Target = Fds[Fd];
				Target.Fd.fd = Fd;
				Target.Fd.revents = 0;

				if (Readable)
				{
					if (!(Target.Fd.events & POLLIN))
					{
						Target.Fd.events |= POLLIN;
						Dirty = true;
					}
				}
				else if (Target.Fd.events & POLLIN)
				{
					Target.Fd.events &= ~(POLLIN);
					Dirty = true;
				}

				if (Writeable)
				{
					if (!(Target.Fd.events & POLLOUT))
					{
						Target.Fd.events |= POLLOUT;
						Dirty = true;
					}
				}
				else if (Target.Fd.events & POLLOUT)
				{
					Target.Fd.events &= ~(POLLOUT);
					Dirty = true;
				}

				if (Target.Data != Data)
				{
					Target.Data = Data;
					Dirty = true;
				}
			}
			void* Find(socket_t Fd)
			{
				Core::UMutex<std::mutex> Unique(Mutex);
				auto It = Fds.find(Fd);
				return It != Fds.end() ?  It->second.Data : nullptr;
			}
			Core::Vector<pollfd>& Compile()
			{
				Core::UMutex<std::mutex> Unique(Mutex);
				if (!Dirty)
					return Events;

				Events.clear();
				Events.reserve(Fds.size());
				for (auto It = Fds.begin(); It != Fds.end();)
				{
					if (It->second.Fd.events > 0)
					{
						Events.emplace_back(It->second.Fd);
						++It;
					}
					else
						Fds.erase(It++);
				}

				Dirty = false;
				return Events;
			}
		};
#endif
#ifdef VI_OPENSSL
		static std::pair<Core::String, time_t> ASN1_GetTime(ASN1_TIME* Time)
		{
			if (!Time)
				return std::make_pair<Core::String, time_t>(Core::String(), time_t(0));

			struct tm Date; size_t i = 0;
			memset(&Date, 0, sizeof(Date));
			ASN1_TIME_to_tm(Time, &Date);

			time_t TimeStamp = mktime(&Date);
			return std::make_pair(Core::DateTime::FetchWebDateGMT(TimeStamp), TimeStamp);
		}
#endif
		static addrinfo* TryConnectDNS(const Core::UnorderedMap<socket_t, addrinfo*>& Hosts, uint64_t Timeout)
		{
			VI_MEASURE(Core::Timings::Networking);

			Core::Vector<pollfd> Sockets4, Sockets6;
			for (auto& Host : Hosts)
			{
				Socket Stream(Host.first);
				Stream.SetBlocking(false);
				Stream.MigrateTo(INVALID_SOCKET, false);

				VI_DEBUG("[net] resolve dns on fd %i", (int)Host.first);
				int Status = connect(Host.first, Host.second->ai_addr, (int)Host.second->ai_addrlen);
				if (Status != 0 && Utils::GetLastError(nullptr, Status) != std::errc::operation_would_block)
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

		Location::Location(const Core::String& Src) noexcept : URL(Src), Protocol("file"), Port(-1)
		{
			VI_ASSERT(!URL.empty(), "url should not be empty");
			Core::Stringify::Replace(URL, '\\', '/');

			const char* ParametersBegin = nullptr;
			const char* PathBegin = nullptr;
			const char* HostBegin = strchr(URL.c_str(), ':');
			if (HostBegin != nullptr)
			{
				if (strncmp(HostBegin, "://", 3) != 0)
				{
					while (*HostBegin != '\0' && *HostBegin != '/' && *HostBegin != ':' && *HostBegin != '?' && *HostBegin != '#')
						++HostBegin;

					PathBegin = *HostBegin == '\0' || *HostBegin == '/' ? HostBegin : HostBegin + 1;
					Protocol = Core::String(URL.c_str(), HostBegin);
					goto InlineURL;
				}
				else
				{
					Protocol = Core::String(URL.c_str(), HostBegin);
					HostBegin += 3;
				}
			}
			else
				HostBegin = URL.c_str();

			if (HostBegin != URL.c_str())
			{
				const char* AtSymbol = strchr(HostBegin, '@');
				PathBegin = strchr(HostBegin, '/');

				if (AtSymbol != nullptr && (PathBegin == nullptr || AtSymbol < PathBegin))
				{
					Core::String LoginPassword;
					LoginPassword = Core::String(HostBegin, AtSymbol);
					HostBegin = AtSymbol + 1;
					PathBegin = strchr(HostBegin, '/');

					const char* PasswordPtr = strchr(LoginPassword.c_str(), ':');
					if (PasswordPtr)
					{
						Username = Compute::Codec::URIDecode(Core::String(LoginPassword.c_str(), PasswordPtr));
						Password = Compute::Codec::URIDecode(Core::String(PasswordPtr + 1));
					}
					else
						Username = Compute::Codec::URIDecode(LoginPassword);
				}

				const char* PortBegin = strchr(HostBegin, ':');
				if (PortBegin != nullptr && (PathBegin == nullptr || PortBegin < PathBegin))
				{
					if (1 != sscanf(PortBegin, ":%d", &Port))
						goto FinalizeURL;

					Hostname = Core::String(HostBegin, PortBegin);
					if (!PathBegin)
						goto FinalizeURL;
				}
				else
				{
					if (PathBegin == nullptr)
					{
						Hostname = HostBegin;
						goto FinalizeURL;
					}

					Hostname = Core::String(HostBegin, PathBegin);
				}
			}
			else
				PathBegin = URL.c_str();

		InlineURL:
			ParametersBegin = strchr(PathBegin, '?');
			if (ParametersBegin != nullptr)
			{
				const char* ParametersEnd = strchr(++ParametersBegin, '#');
				Core::String Parameters(ParametersBegin, ParametersEnd ? ParametersEnd - ParametersBegin : strlen(ParametersBegin));
				Path = Core::String(PathBegin, ParametersBegin - 1);

				if (!ParametersEnd)
				{
					ParametersEnd = strchr(Path.c_str(), '#');
					if (ParametersEnd != nullptr && ParametersEnd > Path.c_str())
					{
						Fragment = ParametersEnd + 1;
						Path = Core::String(Path.c_str(), ParametersEnd);
					}
				}
				else
					Fragment = ParametersEnd + 1;

				for (auto& Item : Core::Stringify::Split(Parameters, '&'))
				{
					Core::Vector<Core::String> KeyValue = Core::Stringify::Split(Item, '=');
					KeyValue[0] = Compute::Codec::URIDecode(KeyValue[0]);

					if (KeyValue.size() >= 2)
						Query[KeyValue[0]] = Compute::Codec::URIDecode(KeyValue[1]);
					else
						Query[KeyValue[0]] = "";
				}
			}
			else
			{
				const char* ParametersEnd = strchr(PathBegin, '#');
				if (ParametersEnd != nullptr)
				{
					Fragment = ParametersEnd + 1;
					Path = Core::String(PathBegin, ParametersEnd);
				}
				else
					Path = PathBegin;
			}

		FinalizeURL:
			if (Protocol.size() == 1)
			{
				Path = Protocol + ':' + Path;
				Protocol = "file";
			}

			if (Path.empty())
				Path = "/";

			if (Port != -1)
				return;

			if (Protocol == "http" || Protocol == "ws")
				Port = 80;
			else if (Protocol == "https" || Protocol == "wss")
				Port = 443;
			else if (Protocol == "ftp")
				Port = 21;
			else if (Protocol == "gopher")
				Port = 70;
			else if (Protocol == "imap")
				Port = 143;
			else if (Protocol == "idap")
				Port = 389;
			else if (Protocol == "nfs")
				Port = 2049;
			else if (Protocol == "nntp")
				Port = 119;
			else if (Protocol == "pop")
				Port = 110;
			else if (Protocol == "smtp")
				Port = 25;
			else if (Protocol == "telnet")
				Port = 23;
		}
		Location::Location(const Location& Other) noexcept :
			Query(Other.Query), URL(Other.URL), Protocol(Other.Protocol),
			Username(Other.Username), Password(Other.Password), Hostname(Other.Hostname),
			Fragment(Other.Fragment), Path(Other.Path), Port(Other.Port)
		{
		}
		Location::Location(Location&& Other) noexcept :
			Query(std::move(Other.Query)), URL(std::move(Other.URL)), Protocol(std::move(Other.Protocol)),
			Username(std::move(Other.Username)), Password(std::move(Other.Password)), Hostname(std::move(Other.Hostname)),
			Fragment(std::move(Other.Fragment)), Path(std::move(Other.Path)), Port(Other.Port)
		{
		}
		Location& Location::operator= (const Location& Other) noexcept
		{
			if (this == &Other)
				return *this;

			Query = Other.Query;
			URL = Other.URL;
			Protocol = Other.Protocol;
			Username = Other.Username;
			Password = Other.Password;
			Hostname = Other.Hostname;
			Path = Other.Path;
			Fragment = Other.Fragment;
			Port = Other.Port;

			return *this;
		}
		Location& Location::operator= (Location&& Other) noexcept
		{
			if (this == &Other)
				return *this;

			Query = std::move(Other.Query);
			URL = std::move(Other.URL);
			Protocol = std::move(Other.Protocol);
			Username = std::move(Other.Username);
			Password = std::move(Other.Password);
			Hostname = std::move(Other.Hostname);
			Path = std::move(Other.Path);
			Fragment = std::move(Other.Fragment);
			Port = Other.Port;

			return *this;
		}

		X509Blob::X509Blob(void* NewPointer) noexcept : Pointer(NewPointer)
		{
		}
		X509Blob::X509Blob(const X509Blob& Other) noexcept : Pointer(nullptr)
		{
#ifdef VI_OPENSSL
			if (Other.Pointer != nullptr)
				Pointer = X509_dup((X509*)Other.Pointer);
#endif
		}
		X509Blob::X509Blob(X509Blob&& Other) noexcept : Pointer(Other.Pointer)
		{
			Other.Pointer = nullptr;
		}
		X509Blob::~X509Blob() noexcept
		{
#ifdef VI_OPENSSL
			if (Pointer != nullptr)
				X509_free((X509*)Pointer);
#endif
		}
		X509Blob& X509Blob::operator= (const X509Blob& Other) noexcept
		{
			if (this == &Other)
				return *this;

#ifdef VI_OPENSSL
			if (Pointer != nullptr)
				X509_free((X509*)Pointer);
			if (Other.Pointer != nullptr)
				Pointer = X509_dup((X509*)Other.Pointer);
#endif
			return *this;
		}
		X509Blob& X509Blob::operator= (X509Blob&& Other) noexcept
		{
			if (this == &Other)
				return *this;

			Pointer = Other.Pointer;
			Other.Pointer = nullptr;
			return *this;
		}

		Core::UnorderedMap<Core::String, Core::Vector<Core::String>> Certificate::GetFullExtensions() const
		{
			Core::UnorderedMap<Core::String, Core::Vector<Core::String>> Data;
			Data.reserve(Extensions.size());

			for (auto& Item : Extensions)
				Data[Item.first] = Core::Stringify::Split(Item.second, '\n');

			return Data;
		}

		DataFrame& DataFrame::operator= (const DataFrame& Other)
		{
			VI_ASSERT(this != &Other, "this should not be other");
			Message = Other.Message;
			Start = Other.Start;
			Finish = Other.Finish;
			Timeout = Other.Timeout;
			KeepAlive = Other.KeepAlive;
			Close = Other.Close;

			return *this;
		}

		EpollHandle::EpollHandle(size_t NewArraySize) noexcept : ArraySize(NewArraySize)
		{
			VI_ASSERT(ArraySize > 0, "array size should be greater than zero");
#if defined(VI_MICROSOFT) && !defined(VI_WEPOLL)
			Handle = (epoll_handle)(uintptr_t)std::numeric_limits<size_t>::max();
			Array = (epoll_event*)VI_NEW(epoll_array);
#elif defined(VI_APPLE)
			Handle = kqueue();
			Array = VI_MALLOC(struct kevent, sizeof(struct kevent) * ArraySize);
#else
			Handle = epoll_create(1);
			Array = VI_MALLOC(epoll_event, sizeof(epoll_event) * ArraySize);
#endif
		}
		EpollHandle::EpollHandle(const EpollHandle& Other) noexcept : ArraySize(Other.ArraySize)
		{
			VI_ASSERT(ArraySize > 0, "array size should be greater than zero");
#if defined(VI_MICROSOFT) && !defined(VI_WEPOLL)
			Handle = (epoll_handle)(uintptr_t)std::numeric_limits<size_t>::max();
			Array = (epoll_event*)VI_NEW(epoll_array);
#elif defined(VI_APPLE)
			Handle = kqueue();
			Array = VI_MALLOC(struct kevent, sizeof(struct kevent) * ArraySize);
#else
			Handle = epoll_create(1);
			Array = VI_MALLOC(epoll_event, sizeof(epoll_event) * ArraySize);
#endif
		}
		EpollHandle::EpollHandle(EpollHandle&& Other) noexcept : Array(Other.Array), Handle(Other.Handle), ArraySize(Other.ArraySize)
		{
			Other.Handle = INVALID_EPOLL;
			Other.Array = nullptr;
		}
		EpollHandle::~EpollHandle() noexcept
		{
#if defined(VI_MICROSOFT) && !defined(VI_WEPOLL)
			Handle = INVALID_EPOLL;
			if (Array != nullptr)
			{
				epoll_array* Handler = (epoll_array*)Array;
				VI_DELETE(epoll_array, Handler);
				Array = nullptr;
			}
#else
			if (Handle != INVALID_EPOLL)
			{
				epoll_close(Handle);
				Handle = INVALID_EPOLL;
			}

			if (Array != nullptr)
			{
				VI_FREE(Array);
				Array = nullptr;
			}
#endif
		}
		EpollHandle& EpollHandle::operator= (const EpollHandle& Other) noexcept
		{
			if (this == &Other)
				return *this;

			this->~EpollHandle();
			ArraySize = Other.ArraySize;
#if defined(VI_MICROSOFT) && !defined(VI_WEPOLL)
			Handle = (epoll_handle)(uintptr_t)std::numeric_limits<size_t>::max();
			Array = (epoll_event*)VI_NEW(epoll_array);
#elif defined(VI_APPLE)
			Handle = kqueue();
			Array = VI_MALLOC(struct kevent, sizeof(struct kevent) * ArraySize);
#else
			Handle = epoll_create(1);
			Array = VI_MALLOC(epoll_event, sizeof(epoll_event) * ArraySize);
#endif
			return *this;
		}
		EpollHandle& EpollHandle::operator= (EpollHandle&& Other) noexcept
		{
			if (this == &Other)
				return *this;

			this->~EpollHandle();
			Array = Other.Array;
			Handle = Other.Handle;
			ArraySize = Other.ArraySize;
			Other.Handle = INVALID_EPOLL;
			Other.Array = nullptr;
			return *this;
		}
		bool EpollHandle::Add(Socket* Fd, bool Readable, bool Writeable) noexcept
		{
			VI_ASSERT(Handle != INVALID_EPOLL, "epoll should be initialized");
			VI_ASSERT(Fd != nullptr && Fd->Fd != INVALID_SOCKET, "socket should be set and valid");
			VI_TRACE("[net] epoll add fd %i %s%s", (int)Fd->Fd, Readable ? "r" : "", Writeable ? "w" : "");
#if defined(VI_MICROSOFT) && !defined(VI_WEPOLL)
			epoll_array* Handler = (epoll_array*)Array;
			Handler->Upsert(Fd->Fd, Readable, Writeable, (void*)Fd);
			return true;
#elif defined(VI_APPLE)
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
		bool EpollHandle::Update(Socket* Fd, bool Readable, bool Writeable) noexcept
		{
			VI_ASSERT(Handle != INVALID_EPOLL, "epoll should be initialized");
			VI_ASSERT(Fd != nullptr && Fd->Fd != INVALID_SOCKET, "socket should be set and valid");
			VI_TRACE("[net] epoll update fd %i %s%s", (int)Fd->Fd, Readable ? "r" : "", Writeable ? "w" : "");
#if defined(VI_MICROSOFT) && !defined(VI_WEPOLL)
			epoll_array* Handler = (epoll_array*)Array;
			Handler->Upsert(Fd->Fd, Readable, Writeable, (void*)Fd);
			return true;
#elif defined(VI_APPLE)
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
		bool EpollHandle::Remove(Socket* Fd, bool Readable, bool Writeable) noexcept
		{
			VI_ASSERT(Handle != INVALID_EPOLL, "epoll should be initialized");
			VI_ASSERT(Fd != nullptr && Fd->Fd != INVALID_SOCKET, "socket should be set and valid");
			VI_TRACE("[net] epoll remove fd %i %s%s", (int)Fd->Fd, Readable ? "r" : "", Writeable ? "w" : "");
#if defined(VI_MICROSOFT) && !defined(VI_WEPOLL)
			epoll_array* Handler = (epoll_array*)Array;
			Handler->Upsert(Fd->Fd, !Readable, !Writeable, (void*)Fd);
			return true;
#elif defined(VI_APPLE)
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
		int EpollHandle::Wait(EpollFd* Data, size_t DataSize, uint64_t Timeout) noexcept
		{
			VI_ASSERT(ArraySize <= DataSize, "epollfd array should be less than or equal to internal events buffer");
#if defined(VI_MICROSOFT) && !defined(VI_WEPOLL)
			epoll_array* Handler = (epoll_array*)Array;
			auto& Events = Handler->Compile();
			if (Events.empty())
				return 0;

			int Count = Utils::Poll(Events.data(), (int)Events.size(), (int)Timeout);
			if (Count <= 0)
				return Count;

			size_t Incoming = 0;
			for (auto& Event : Events)
			{
				if (!Event.revents)
					continue;
				
				Socket* Target = (Socket*)Handler->Find(Event.fd);
				if (!Target)
					continue;

				auto& Fd = Data[Incoming++];
				Fd.Base = Target;
				Fd.Readable = (Event.revents & POLLIN);
				Fd.Writeable = (Event.revents & POLLOUT);
				Fd.Closed = (Event.revents & POLLHUP || Event.revents & POLLNVAL || Event.revents & POLLERR);
			}
#elif defined(VI_APPLE)
			VI_TRACE("[net] kqueue wait %i fds (%" PRIu64 " ms)", (int)DataSize, Timeout);
			struct timespec Wait;
			Wait.tv_sec = (int)Timeout / 1000;
			Wait.tv_nsec = ((int)Timeout % 1000) * 1000000;

			struct kevent* Events = (struct kevent*)Array;
			int Count = kevent(Handle, nullptr, 0, Events, (int)DataSize, &Wait);
			if (Count <= 0)
				return Count;

			size_t Incoming = 0;
			for (auto It = Events; It != Events + Count; It++)
			{
				auto& Fd = Data[Incoming++];
				Fd.Base = (Socket*)It->udata;
				Fd.Readable = (It->filter == EVFILT_READ);
				Fd.Writeable = (It->filter == EVFILT_WRITE);
				Fd.Closed = (It->flags & EV_EOF);
			}
			VI_TRACE("[net] kqueue recv %i events", (int)Incoming);
#else
			VI_TRACE("[net] epoll wait %i fds (%" PRIu64 " ms)", (int)DataSize, Timeout);
			epoll_event* Events = (epoll_event*)Array;
			int Count = epoll_wait(Handle, Events, (int)DataSize, (int)Timeout);
			if (Count <= 0)
				return Count;

			size_t Incoming = 0;
			for (auto It = Events; It != Events + Count; It++)
			{
				auto& Fd = Data[Incoming++];
				Fd.Base = (Socket*)It->data.ptr;
				Fd.Readable = (It->events & EPOLLIN);
				Fd.Writeable = (It->events & EPOLLOUT);
				Fd.Closed = (It->events & EPOLLHUP || It->events & EPOLLRDHUP || It->events & EPOLLERR);
			}
			VI_TRACE("[net] epoll recv %i events", (int)Incoming);
#endif
			return (int)Incoming;
		}

		Core::ExpectsIO<CertificateBlob> Utils::GenerateSelfSignedCertificate(uint32_t Days, const Core::String& AddressesCommaSeparated, const Core::String& DomainsCommaSeparated) noexcept
		{
			CertificateBuilder* Builder = new CertificateBuilder();
			Builder->SetSerialNumber();
			Builder->SetVersion(2);
			Builder->SetNotBefore();
			Builder->SetNotAfter(86400 * Days);
			Builder->SetPublicKey();
			Builder->AddSubjectInfo("CN", "Self-signed certificate");
			Builder->AddIssuerInfo("CN", "CA for self-signed certificates");

			Core::String AlternativeNames;
			for (auto& Domain : Core::Stringify::Split(DomainsCommaSeparated, ','))
				AlternativeNames += "DNS: " + Domain + ", ";

			for (auto& Address : Core::Stringify::Split(AddressesCommaSeparated, ','))
				AlternativeNames += "IP: " + Address + ", ";

			if (!AlternativeNames.empty())
			{
				AlternativeNames.erase(AlternativeNames.size() - 2, 2);
				Builder->AddStandardExtension(nullptr, "subjectAltName", AlternativeNames.c_str());
			}

			Builder->Sign(Compute::Digests::SHA256());
			auto Blob = Builder->Build();
			VI_RELEASE(Builder);

			return Blob;
		}
		Core::ExpectsIO<Certificate> Utils::GetCertificateFromX509(void* CertificateX509) noexcept
		{
			VI_ASSERT(CertificateX509 != nullptr, "certificate should be set");
			VI_MEASURE(Core::Timings::Networking);
#ifdef VI_OPENSSL
			X509* Blob = (X509*)CertificateX509;
			X509_NAME* Subject = X509_get_subject_name(Blob);
			X509_NAME* Issuer = X509_get_issuer_name(Blob);
			ASN1_INTEGER* Serial = X509_get_serialNumber(Blob);
			auto NotBefore = ASN1_GetTime(X509_get_notBefore(Blob));
			auto NotAfter = ASN1_GetTime(X509_get_notAfter(Blob));

			Certificate Output;
			Output.Version = X509_get_version(Blob);
			Output.Signature = X509_get_signature_type(Blob);
			Output.NotBeforeDate = NotBefore.first;
			Output.NotBeforeTime = NotBefore.second;
			Output.NotAfterDate = NotAfter.first;
			Output.NotAfterTime = NotAfter.second;
			Output.Blob.Pointer = Blob;

			int Extensions = X509_get_ext_count(Blob);
			Output.Extensions.reserve((size_t)Extensions);

			for (int i = 0; i < Extensions; i++)
			{
				Core::String Name, Value;
				X509_EXTENSION* Extension = X509_get_ext(Blob, i);
				ASN1_OBJECT* Object = X509_EXTENSION_get_object(Extension);

				BIO* ExtensionBio = BIO_new(BIO_s_mem());
				if (ExtensionBio != nullptr)
				{
					BUF_MEM* ExtensionMemory = nullptr;
					i2a_ASN1_OBJECT(ExtensionBio, Object);
					if (BIO_get_mem_ptr(ExtensionBio, &ExtensionMemory) != 0 && ExtensionMemory->data != nullptr && ExtensionMemory->length > 0)
						Name = Core::String(ExtensionMemory->data, (size_t)ExtensionMemory->length);
					BIO_free(ExtensionBio);
				}

				ExtensionBio = BIO_new(BIO_s_mem());
				if (ExtensionBio != nullptr)
				{
					BUF_MEM* ExtensionMemory = nullptr;
					X509V3_EXT_print(ExtensionBio, Extension, 0, 0);
					if (BIO_get_mem_ptr(ExtensionBio, &ExtensionMemory) != 0 && ExtensionMemory->data != nullptr && ExtensionMemory->length > 0)
						Value = Core::String(ExtensionMemory->data, (size_t)ExtensionMemory->length);
					BIO_free(ExtensionBio);
				}

				Output.Extensions[Name] = std::move(Value);
			}

			char SubjectBuffer[Core::CHUNK_SIZE];
			X509_NAME_oneline(Subject, SubjectBuffer, (int)sizeof(SubjectBuffer));
			Output.SubjectName = SubjectBuffer;

			char IssuerBuffer[Core::CHUNK_SIZE], SerialBuffer[Core::CHUNK_SIZE];
			X509_NAME_oneline(Issuer, IssuerBuffer, (int)sizeof(IssuerBuffer));
			Output.IssuerName = IssuerBuffer;

			unsigned char Buffer[256];
			int Length = i2d_ASN1_INTEGER(Serial, nullptr);
			if (Length > 0 && (unsigned)Length < (unsigned)sizeof(Buffer))
			{
				unsigned char* Pointer = Buffer;
				if (!Compute::Codec::HexToString(Buffer, (uint64_t)i2d_ASN1_INTEGER(Serial, &Pointer), SerialBuffer, sizeof(SerialBuffer)))
					*SerialBuffer = '\0';
			}
			else
				*SerialBuffer = '\0';
			Output.SerialNumber = SerialBuffer;

			const EVP_MD* Digest = EVP_get_digestbyname("sha256");
			unsigned int BufferLength = sizeof(Buffer) - 1;
			X509_digest(Blob, Digest, Buffer, &BufferLength);
			Output.Fingerprint = Compute::Codec::HexEncode(Core::String((const char*)Buffer, BufferLength));

			X509_pubkey_digest(Blob, Digest, Buffer, &BufferLength);
			Output.PublicKey = Compute::Codec::HexEncode(Core::String((const char*)Buffer, BufferLength));

			return Output;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		int Utils::Poll(pollfd* Fd, int FdCount, int Timeout) noexcept
		{
			VI_ASSERT(Fd != nullptr, "poll should be set");
			VI_TRACE("[net] poll wait %i fds (%i ms)", FdCount, Timeout);
#if defined(VI_MICROSOFT)
			int Count = WSAPoll(Fd, FdCount, Timeout);
#else
			int Count = poll(Fd, FdCount, Timeout);
#endif
			if (Count > 0)
				VI_TRACE("[net] poll recv %i events", Count);
			return Count;
		}
		int Utils::Poll(PollFd* Fd, int FdCount, int Timeout) noexcept
		{
			VI_ASSERT(Fd != nullptr, "poll should be set");
			Core::Vector<pollfd> Fds;
			Fds.resize(FdCount);

			for (size_t i = 0; i < (size_t)FdCount; i++)
			{
				auto& Next = Fds[i];
				Next.revents = 0;
				Next.events = 0;

				auto& Base = Fd[i];
				Next.fd = Base.Fd;

				if (Base.Events & InputNormal)
					Next.events |= POLLRDNORM;
				if (Base.Events & InputBand)
					Next.events |= POLLRDBAND;
				if (Base.Events & InputPriority)
					Next.events |= POLLPRI;
				if (Base.Events & Input)
					Next.events |= POLLIN;
				if (Base.Events & OutputNormal)
					Next.events |= POLLWRNORM;
				if (Base.Events & OutputBand)
					Next.events |= POLLWRBAND;
				if (Base.Events & Output)
					Next.events |= POLLOUT;
				if (Base.Events & Error)
					Next.events |= POLLERR;
				if (Base.Events & Hangup)
					Next.events |= POLLHUP;
			}

			int Size = Poll(Fds.data(), FdCount, Timeout);
			for (size_t i = 0; i < (size_t)FdCount; i++)
			{
				auto& Next = Fd[i];
				Next.Returns = 0;

				auto& Base = Fds[i];
				if (Base.revents & POLLRDNORM)
					Next.Returns |= InputNormal;
				if (Base.revents & POLLRDBAND)
					Next.Returns |= InputBand;
				if (Base.revents & POLLPRI)
					Next.Returns |= InputPriority;
				if (Base.revents & POLLIN)
					Next.Returns |= Input;
				if (Base.revents & POLLWRNORM)
					Next.Returns |= OutputNormal;
				if (Base.revents & POLLWRBAND)
					Next.Returns |= OutputBand;
				if (Base.revents & POLLOUT)
					Next.Returns |= Output;
				if (Base.revents & POLLERR)
					Next.Returns |= Error;
				if (Base.revents & POLLHUP)
					Next.Returns |= Hangup;
			}

			return Size;
		}
		Core::String Utils::GetLocalAddress() noexcept
		{
			char Buffer[Core::CHUNK_SIZE];
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
		Core::String Utils::GetAddress(addrinfo* Info) noexcept
		{
			VI_ASSERT(Info != nullptr, "address info should be set");
			char Buffer[INET6_ADDRSTRLEN];
			inet_ntop(Info->ai_family, GetAddressStorage(Info->ai_addr), Buffer, sizeof(Buffer));
			VI_TRACE("[net] inet ntop addrinfo 0x%" PRIXPTR ": %s", (void*)Info, Buffer);
			return Buffer;
		}
		Core::String Utils::GetAddress(sockaddr* Info) noexcept
		{
			VI_ASSERT(Info != nullptr, "socket address should be set");
			char Buffer[INET6_ADDRSTRLEN];
			inet_ntop(Info->sa_family, GetAddressStorage(Info), Buffer, sizeof(Buffer));
			VI_TRACE("[net] inet ntop sockaddr 0x%" PRIXPTR ": %s", (void*)Info, Buffer);
			return Buffer;
		}
		std::error_condition Utils::GetLastError(ssl_st* Device, int ErrorCode) noexcept
		{
#ifdef VI_OPENSSL
			if (Device != nullptr)
			{
				ErrorCode = SSL_get_error(Device, ErrorCode);
				switch (ErrorCode)
				{
					case SSL_ERROR_WANT_READ:
					case SSL_ERROR_WANT_WRITE:
					case SSL_ERROR_WANT_CONNECT:
					case SSL_ERROR_WANT_ACCEPT:
					case SSL_ERROR_WANT_ASYNC:
					case SSL_ERROR_WANT_ASYNC_JOB:
					case SSL_ERROR_WANT_X509_LOOKUP:
					case SSL_ERROR_WANT_CLIENT_HELLO_CB:
					case SSL_ERROR_WANT_RETRY_VERIFY:
						return std::make_error_condition(std::errc::operation_would_block);
					case SSL_ERROR_SSL:
						Utils::DisplayTransportLog();
						return std::make_error_condition(std::errc::protocol_error);
					case SSL_ERROR_ZERO_RETURN:
						Utils::DisplayTransportLog();
						return std::make_error_condition(std::errc::bad_message);
					case SSL_ERROR_SYSCALL:
						Utils::DisplayTransportLog();
						return std::make_error_condition(std::errc::io_error);
					case SSL_ERROR_NONE:
					default:
						Utils::DisplayTransportLog();
						return std::make_error_condition(std::errc());
				}
			}
#endif
#ifdef VI_MICROSOFT
			ErrorCode = WSAGetLastError();
#else
			ErrorCode = errno;
#endif
			switch (ErrorCode)
			{
#ifdef VI_MICROSOFT
				case WSAEWOULDBLOCK:
#endif
				case EWOULDBLOCK:
				case EINPROGRESS:
					return std::make_error_condition(std::errc::operation_would_block);
				case 0:
					return std::make_error_condition(std::errc());
				default:
					return Core::OS::Error::GetCondition(ErrorCode);
			}
		}
		int Utils::GetAddressFamily(const char* Address) noexcept
		{
			VI_ASSERT(Address != nullptr, "address should be set");
			VI_TRACE("[net] fetch addr family %s", Address);

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
		int64_t Utils::Clock() noexcept
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}
		void Utils::DisplayTransportLog() noexcept
		{
			Compute::Crypto::DisplayCryptoLog();
		}

		TransportLayer::TransportLayer() noexcept : IsInstalled(false)
		{
		}
		TransportLayer::~TransportLayer() noexcept
		{
#ifdef VI_OPENSSL
			for (auto& Context : Servers)
				SSL_CTX_free(Context);
			Servers.clear();

			for (auto& Context : Clients)
				SSL_CTX_free(Context);
			Clients.clear();
#endif
		}
		Core::ExpectsIO<ssl_ctx_st*> TransportLayer::CreateServerContext(size_t VerifyDepth, SecureLayerOptions Options, const Core::String& CiphersList) noexcept
		{
#ifdef VI_OPENSSL
			Core::UMutex<std::mutex> Unique(Exclusive);
			SSL_CTX* Context; bool LoadCertificates = false;
			if (Servers.empty())
			{
				Context = SSL_CTX_new(TLS_server_method());
				if (!Context)
				{
					Utils::DisplayTransportLog();
					return std::make_error_condition(std::errc::protocol_not_supported);
				}
				VI_DEBUG("[net] OK create client 0x%" PRIuPTR " TLS context", Context);
				LoadCertificates = VerifyDepth > 0;
			}
			else
			{
				Context = *Servers.begin();
				Servers.erase(Servers.begin());
				VI_DEBUG("[net] pop client 0x%" PRIuPTR " TLS context from cache", Context);
				LoadCertificates = VerifyDepth > 0 && SSL_CTX_get_verify_depth(Context) <= 0;
			}
			Unique.Negate();

			auto Status = InitializeContext(Context, LoadCertificates);
			if (!Status)
			{
				SSL_CTX_free(Context);
				return Status.Error();
			}

			long Flags = SSL_OP_ALL;
			if ((size_t)Options & (size_t)SecureLayerOptions::NoSSL_V2)
				Flags |= SSL_OP_NO_SSLv2;
			if ((size_t)Options & (size_t)SecureLayerOptions::NoSSL_V3)
				Flags |= SSL_OP_NO_SSLv3;
			if ((size_t)Options & (size_t)SecureLayerOptions::NoTLS_V1)
				Flags |= SSL_OP_NO_TLSv1;
			if ((size_t)Options & (size_t)SecureLayerOptions::NoTLS_V1_1)
				Flags |= SSL_OP_NO_TLSv1_1;
#ifdef SSL_OP_NO_TLSv1_2
			if ((size_t)Options & (size_t)SecureLayerOptions::NoTLS_V1_2)
				Flags |= SSL_OP_NO_TLSv1_2;
#endif
#ifdef SSL_OP_NO_TLSv1_2
			if ((size_t)Options & (size_t)SecureLayerOptions::NoTLS_V1_3)
				Flags |= SSL_OP_NO_TLSv1_3;
#endif
			SSL_CTX_set_options(Context, Flags);
			SSL_CTX_set_verify_depth(Context, (int)VerifyDepth);
			if (!CiphersList.empty() && SSL_CTX_set_cipher_list(Context, CiphersList.c_str()) != 1)
			{
				SSL_CTX_free(Context);
				Utils::DisplayTransportLog();
				return std::make_error_condition(std::errc::protocol_not_supported);
			}

			VI_DEBUG("[net] OK create server 0x%" PRIuPTR " TLS context", Context);
			return Context;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<ssl_ctx_st*> TransportLayer::CreateClientContext(size_t VerifyDepth) noexcept
		{
#ifdef VI_OPENSSL
			Core::UMutex<std::mutex> Unique(Exclusive);
			SSL_CTX* Context; bool LoadCertificates = false;
			if (Clients.empty())
			{
				Context = SSL_CTX_new(TLS_client_method());
				if (!Context)
				{
					Utils::DisplayTransportLog();
					return std::make_error_condition(std::errc::protocol_not_supported);
				}
				VI_DEBUG("[net] OK create client 0x%" PRIuPTR " TLS context", Context);
				LoadCertificates = VerifyDepth > 0;
			}
			else
			{
				Context = *Clients.begin();
				Clients.erase(Clients.begin());
				VI_DEBUG("[net] pop client 0x%" PRIuPTR " TLS context from cache", Context);
				LoadCertificates = VerifyDepth > 0 && SSL_CTX_get_verify_depth(Context) <= 0;
			}
			Unique.Negate();

			auto Status = InitializeContext(Context, LoadCertificates);
			if (!Status)
			{
				SSL_CTX_free(Context);
				return Status.Error();
			}

			SSL_CTX_set_verify_depth(Context, (int)VerifyDepth);
			return Context;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		void TransportLayer::FreeServerContext(ssl_ctx_st* Context) noexcept
		{
			if (!Context)
				return;

			Core::UMutex<std::mutex> Unique(Exclusive);
			Servers.insert(Context);
		}
		void TransportLayer::FreeClientContext(ssl_ctx_st* Context) noexcept
		{
			if (!Context)
				return;

			Core::UMutex<std::mutex> Unique(Exclusive);
			Clients.insert(Context);
		}
		Core::ExpectsIO<void> TransportLayer::InitializeContext(ssl_ctx_st* Context, bool LoadCertificates) noexcept
		{
#ifdef VI_OPENSSL
			SSL_CTX_set_options(Context, SSL_OP_SINGLE_DH_USE);
			SSL_CTX_set_options(Context, SSL_OP_CIPHER_SERVER_PREFERENCE);
			SSL_CTX_set_mode(Context, SSL_MODE_ENABLE_PARTIAL_WRITE);
			SSL_CTX_set_mode(Context, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
#ifdef SSL_CTX_set_ecdh_auto
			SSL_CTX_set_ecdh_auto(Context, 1);
#endif
			auto SessionId = Compute::Crypto::RandomBytes(12);
			if (SessionId)
			{
				auto ContextId = Compute::Codec::HexEncode(*SessionId);
				SSL_CTX_set_session_id_context(Context, (const unsigned char*)ContextId.c_str(), (unsigned int)ContextId.size());
			}

			if (LoadCertificates)
			{
	#ifdef VI_MICROSOFT
				HCERTSTORE Store = CertOpenSystemStore(0, "ROOT");
				if (!Store)
				{
					Utils::DisplayTransportLog();
					return std::make_error_condition(std::errc::permission_denied);
				}

				X509_STORE* Storage = SSL_CTX_get_cert_store(Context);
				if (!Storage)
				{
					CertCloseStore(Store, 0);
					Utils::DisplayTransportLog();
					return std::make_error_condition(std::errc::bad_file_descriptor);
				}

				PCCERT_CONTEXT Next = nullptr; uint32_t Count = 0;
				while (Next = CertEnumCertificatesInStore(Store, Next))
				{
					X509* Certificate = d2i_X509(nullptr, (const unsigned char**)&Next->pbCertEncoded, Next->cbCertEncoded);
					if (!Certificate)
						continue;

					if (X509_STORE_add_cert(Storage, Certificate) && !IsInstalled)
						VI_TRACE("[net] OK load root level certificate: %s", Compute::Codec::HexEncode((const char*)Next->pCertInfo->SerialNumber.pbData, (size_t)Next->pCertInfo->SerialNumber.cbData).c_str());
					X509_free(Certificate);
					++Count;
				}

				VI_DEBUG("[net] OK load %i root level certificates from ROOT registry", Count);
				CertCloseStore(Store, 0);
				Utils::DisplayTransportLog();
	#else
				if (!SSL_CTX_set_default_verify_paths(Context))
				{
					Utils::DisplayTransportLog();
					return std::make_error_condition(std::errc::no_such_file_or_directory);
				}
	#endif
			}

			IsInstalled = true;
			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}

		DNS::DNS() noexcept
		{
			VI_TRACE("[dns] OK initialize cache");
		}
		DNS::~DNS() noexcept
		{
			VI_TRACE("[dns] cleanup cache");
			for (auto& Item : Names)
				VI_RELEASE(Item.second.second);
			Names.clear();
		}
		Core::ExpectsSystem<Core::String> DNS::FindNameFromAddress(const Core::String& Host, const Core::String& Service)
		{
			VI_ASSERT(!Host.empty(), "ip address should not be empty");
			VI_ASSERT(!Service.empty(), "port should be greater than zero");
			VI_MEASURE((uint64_t)Core::Timings::Networking * 3);

			struct sockaddr_storage Storage;
			auto Port = Core::FromString<int>(Service);
			int Family = Utils::GetAddressFamily(Host.c_str());
			int Result = -1;

			if (Family == AF_INET)
			{
				auto* Base = reinterpret_cast<struct sockaddr_in*>(&Storage);
				Result = inet_pton(Family, Host.c_str(), &Base->sin_addr.s_addr);
				Base->sin_family = Family;
				Base->sin_port = Port ? htons(*Port) : 0;
			}
			else if (Family == AF_INET6)
			{
				auto* Base = reinterpret_cast<struct sockaddr_in6*>(&Storage);
				Result = inet_pton(Family, Host.c_str(), &Base->sin6_addr);
				Base->sin6_family = Family;
				Base->sin6_port = Port ? htons(*Port) : 0;
			}

			if (Result == -1)
				return Core::SystemException(Core::Stringify::Text("dns resolve %s:%i address: invalid address", Host.c_str(), Service.c_str()));

			char Hostname[NI_MAXHOST], ServiceName[NI_MAXSERV];
			if (getnameinfo((struct sockaddr*)&Storage, sizeof(struct sockaddr), Hostname, NI_MAXHOST, ServiceName, NI_MAXSERV, NI_NUMERICSERV) != 0)
				return Core::SystemException(Core::Stringify::Text("dns reverse resolve %s:%i address: invalid address", Host.c_str(), Service.c_str()));

			VI_DEBUG("[net] dns reverse resolved for identity %s:%i (host %s:%s is used)", Host.c_str(), Service.c_str(), Hostname, ServiceName);
			return Core::String(Hostname, strnlen(Hostname, sizeof(Hostname) - 1));
		}
		Core::ExpectsSystem<SocketAddress*> DNS::FindAddressFromName(const Core::String& Host, const Core::String& Service, DNSType DNS, SocketProtocol Proto, SocketType Type)
		{
			VI_ASSERT(!Host.empty(), "host should not be empty");
			VI_ASSERT(!Service.empty(), "service should not be empty");
			VI_MEASURE((uint64_t)Core::Timings::Networking * 3);

			struct addrinfo Hints;
			memset(&Hints, 0, sizeof(struct addrinfo));
			Hints.ai_family = AF_UNSPEC;

			Core::String XProto;
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

			Core::String XType;
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
			Core::String Identity = XProto + '_' + XType + '@' + Host + ':' + Service;
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto It = Names.find(Identity);
				if (It != Names.end() && It->second.first > Time)
					return It->second.second;
			}

			struct addrinfo* Addresses = nullptr;
			if (getaddrinfo(Host.c_str(), Service.c_str(), &Hints, &Addresses) != 0)
				return Core::SystemException(Core::Stringify::Text("dns resolve %s address: invalid address", Identity.c_str()));

			struct addrinfo* Good = nullptr;
			Core::UnorderedMap<socket_t, addrinfo*> Hosts;
			for (auto It = Addresses; It != nullptr; It = It->ai_next)
			{
				socket_t Connection = socket(It->ai_family, It->ai_socktype, It->ai_protocol);
				if (Connection == INVALID_SOCKET)
					continue;

				VI_DEBUG("[net] open dns fd %i", (int)Connection);
				if (DNS == DNSType::Connect)
				{
					Hosts[Connection] = It;
					continue;
				}

				Good = It;
				closesocket(Connection);
				VI_DEBUG("[net] close dns fd %i", (int)Connection);
				break;
			}

			if (DNS == DNSType::Connect)
				Good = TryConnectDNS(Hosts, CONNECT_TIMEOUT);

			for (auto& Host : Hosts)
			{
				closesocket(Host.first);
				VI_DEBUG("[net] close dns fd %i", (int)Host.first);
			}

			if (!Good)
			{
				freeaddrinfo(Addresses);
				return Core::SystemException(Core::Stringify::Text("dns resolve %s address: invalid address", Identity.c_str()), std::make_error_condition(std::errc::host_unreachable));
			}

			SocketAddress* Result = new SocketAddress(Addresses, Good);
			VI_DEBUG("[net] dns resolved for identity %s (address %s is used)", Identity.c_str(), Utils::GetAddress(Good).c_str());

			Core::UMutex<std::mutex> Unique(Exclusive);
			auto It = Names.find(Identity);
			if (It != Names.end())
			{
				VI_RELEASE(Result);
				It->second.first = Time + DNS_TIMEOUT;
				Result = It->second.second;
			}
			else
				Names[Identity] = std::make_pair(Time + DNS_TIMEOUT, Result);

			return Result;
		}

		Multiplexer::Multiplexer() noexcept : Multiplexer(50, 256)
		{
		}
		Multiplexer::Multiplexer(uint64_t DispatchTimeout, size_t MaxEvents) noexcept : Handle(MaxEvents), Activations(0), DefaultTimeout(DispatchTimeout)
		{
			VI_TRACE("[net] OK initialize multiplexer (%" PRIu64 " events)", (uint64_t)MaxEvents);
			Fds.resize(MaxEvents);
		}
		Multiplexer::~Multiplexer() noexcept
		{
			VI_TRACE("[net] free multiplexer");
		}
		void Multiplexer::Rescale(uint64_t DispatchTimeout, size_t MaxEvents) noexcept
		{
			DefaultTimeout = DispatchTimeout;
			Handle = EpollHandle(MaxEvents);
		}
		void Multiplexer::Activate() noexcept
		{
			TryListen();
		}
		void Multiplexer::Deactivate() noexcept
		{
			TryUnlisten();
		}
		int Multiplexer::Dispatch(uint64_t EventTimeout) noexcept
		{
			int Count = Handle.Wait(Fds.data(), Fds.size(), EventTimeout);
			if (Count < 0)
				return Count;

			VI_MEASURE(EventTimeout + (uint64_t)Core::Timings::FileSystem);
			size_t Size = (size_t)Count;
			auto Time = Core::Schedule::GetClock();

			for (size_t i = 0; i < Size; i++)
				DispatchEvents(Fds.at(i), Time);

			VI_MEASURE(Core::Timings::FileSystem);
			if (Timeouts.empty())
				return Count;

			Core::UMutex<std::recursive_mutex> Unique(Exclusive);
			while (!Timeouts.empty())
			{
				auto It = Timeouts.begin();
				if (It->first > Time)
					break;

				VI_DEBUG("[net] sock timeout on fd %i", (int)It->second->Fd);
				CancelEvents(It->second, SocketPoll::Timeout, false);
				Timeouts.erase(It);
			}

			return Count;
		}
		bool Multiplexer::DispatchEvents(EpollFd& Fd, const std::chrono::microseconds& Time) noexcept
		{
			VI_ASSERT(Fd.Base != nullptr, "no socket is connected to epoll fd");
			if (Fd.Closed)
			{
				VI_DEBUG("[net] sock reset on fd %i", (int)Fd.Base->Fd);
				CancelEvents(Fd.Base, SocketPoll::Reset, true);
				return false;
			}

			if (!Fd.Readable && !Fd.Writeable)
			{
				ClearEvents(Fd.Base);
				return false;
			}

			Core::UMutex<std::recursive_mutex> Unique(Exclusive);
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

			Unique.Negate();
			if (Fd.Readable && Fd.Writeable)
			{
				Core::Schedule::Get()->SetTask([WhenReadable = std::move(WhenReadable), WhenWriteable = std::move(WhenWriteable)]() mutable
				{
					if (WhenWriteable)
						WhenWriteable(SocketPoll::Finish);

					if (WhenReadable)
						WhenReadable(SocketPoll::Finish);
				});
			}
			else if (Fd.Readable)
			{
				Core::Schedule::Get()->SetTask([WhenReadable = std::move(WhenReadable)]() mutable
				{
					if (WhenReadable)
						WhenReadable(SocketPoll::Finish);
				});
			}
			else if (Fd.Writeable)
			{
				Core::Schedule::Get()->SetTask([WhenWriteable = std::move(WhenWriteable)]() mutable
				{
					if (WhenWriteable)
						WhenWriteable(SocketPoll::Finish);
				});
			}

			return Exists;
		}
		bool Multiplexer::WhenReadable(Socket* Value, PollEventCallback&& WhenReady) noexcept
		{
			VI_ASSERT(Value != nullptr && Value->Fd != INVALID_SOCKET, "socket should be set and valid");
			return WhenEvents(Value, true, false, std::move(WhenReady), nullptr);
		}
		bool Multiplexer::WhenWriteable(Socket* Value, PollEventCallback&& WhenReady) noexcept
		{
			VI_ASSERT(Value != nullptr && Value->Fd != INVALID_SOCKET, "socket should be set and valid");
			return WhenEvents(Value, false, true, nullptr, std::move(WhenReady));
		}
		bool Multiplexer::CancelEvents(Socket* Value, SocketPoll Event, bool EraseTimeout) noexcept
		{
			VI_ASSERT(Value != nullptr && Value->Fd != INVALID_SOCKET, "socket should be set and valid");
			Core::UMutex<std::recursive_mutex> Unique(Exclusive);
			auto WhenReadable = std::move(Value->Events.ReadCallback);
			auto WhenWriteable = std::move(Value->Events.WriteCallback);
			bool Success = Handle.Remove(Value, true, true);
			Value->Events.Readable = false;
			Value->Events.Writeable = false;
			if (EraseTimeout && Value->Timeout > 0 && Value->Events.ExpiresAt > std::chrono::microseconds(0))
				RemoveTimeout(Value);

			Unique.Negate();
			if (!Packet::IsDone(Event) && (WhenWriteable || WhenReadable))
			{
				Core::Schedule::Get()->SetTask([Event, WhenReadable = std::move(WhenReadable), WhenWriteable = std::move(WhenWriteable)]() mutable
				{
					if (WhenWriteable)
						WhenWriteable(Event);

					if (WhenReadable)
						WhenReadable(Event);
				});
			}

			return Success;
		}
		bool Multiplexer::ClearEvents(Socket* Value) noexcept
		{
			return CancelEvents(Value, SocketPoll::Finish, true);
		}
		bool Multiplexer::IsAwaitingEvents(Socket* Value) noexcept
		{
			VI_ASSERT(Value != nullptr, "socket should be set");
			Core::UMutex<std::recursive_mutex> Unique(Exclusive);
			return Value->Events.Readable || Value->Events.Writeable;
		}
		bool Multiplexer::IsAwaitingReadable(Socket* Value) noexcept
		{
			VI_ASSERT(Value != nullptr, "socket should be set");
			Core::UMutex<std::recursive_mutex> Unique(Exclusive);
			return Value->Events.Readable;
		}
		bool Multiplexer::IsAwaitingWriteable(Socket* Value) noexcept
		{
			VI_ASSERT(Value != nullptr, "socket should be set");
			Core::UMutex<std::recursive_mutex> Unique(Exclusive);
			return Value->Events.Writeable;
		}
		bool Multiplexer::IsListening() noexcept
		{
			return Activations > 0;
		}
		void Multiplexer::TryDispatch() noexcept
		{
			auto* Queue = Core::Schedule::Get();
			Dispatch(Queue->GetThreads(Core::Difficulty::Normal) > 1 ? DefaultTimeout : 10);
			TryEnqueue();
		}
		void Multiplexer::TryEnqueue() noexcept
		{
			auto* Queue = Core::Schedule::Get();
			if (Queue->CanEnqueue() && Activations > 0)
				Queue->SetTask(std::bind(&Multiplexer::TryDispatch, this));
		}
		void Multiplexer::TryListen() noexcept
		{
			if (!Activations++)
			{
				VI_DEBUG("[net] start events polling");
				TryEnqueue();
			}
		}
		void Multiplexer::TryUnlisten() noexcept
		{
			VI_ASSERT(Activations > 0, "events poller is already inactive");
			if (!--Activations)
				VI_DEBUG("[net] stop events polling");
		}
		bool Multiplexer::WhenEvents(Socket* Value, bool Readable, bool Writeable, PollEventCallback&& WhenReadable, PollEventCallback&& WhenWriteable) noexcept
		{
			bool Update = false;
			Core::UMutex<std::recursive_mutex> Unique(Exclusive);
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

			bool Success = (Update ? Handle.Update(Value, Value->Events.Readable, Value->Events.Writeable) : Handle.Add(Value, Value->Events.Readable, Value->Events.Writeable));
			Unique.Negate();

			if (WhenWriteable || WhenReadable)
			{
				Core::Schedule::Get()->SetTask([WhenReadable = std::move(WhenReadable), WhenWriteable = std::move(WhenWriteable)]() mutable
				{
					if (WhenWriteable)
						WhenWriteable(SocketPoll::Cancel);

					if (WhenReadable)
						WhenReadable(SocketPoll::Cancel);
				});
			}

			return Success;
		}
		void Multiplexer::AddTimeout(Socket* Value, const std::chrono::microseconds& Time) noexcept
		{
			Value->Events.ExpiresAt = Time;
			Timeouts.insert(std::make_pair(Time, Value));
		}
		void Multiplexer::UpdateTimeout(Socket* Value, const std::chrono::microseconds& Time) noexcept
		{
			RemoveTimeout(Value);
			AddTimeout(Value, Time);
		}
		void Multiplexer::RemoveTimeout(Socket* Value) noexcept
		{
			auto It = Timeouts.find(Value->Events.ExpiresAt);
			if (It == Timeouts.end())
			{
				for (auto I = Timeouts.begin(); I != Timeouts.end(); ++I)
				{
					if (I->second == Value)
					{
						It = I;
						break;
					}
				}
			}

			if (It != Timeouts.end())
				Timeouts.erase(It);
		}
		size_t Multiplexer::GetActivations() noexcept
		{
			return Activations;
		}

		Uplinks::Uplinks() noexcept
		{
			Multiplexer::Get()->Activate();
		}
		Uplinks::~Uplinks() noexcept
		{
			Core::UMutex<std::mutex> Unique(Exclusive);
			for (auto& Targets : Connections)
			{
				for (auto& Stream : Targets.second)
					VI_RELEASE(Stream);
			}
			Connections.clear();
			Multiplexer::Get()->Deactivate();
		}
		void Uplinks::ExpireConnection(RemoteHost* Address, Socket* Target)
		{
			VI_ASSERT(Address != nullptr, "address should be set");
			ExpireConnectionURL(GetURL(Address), Target);;
		}
		void Uplinks::ExpireConnectionURL(const Core::String& URL, Socket* Stream)
		{
			VI_ASSERT(Stream != nullptr, "socket should be set");
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto Targets = Connections.find(URL);
				if (Targets == Connections.end())
					return;

				auto It = Targets->second.find(Stream);
				if (It == Targets->second.end())
					return;

				Targets->second.erase(It);
			}
			UnlistenConnection(Stream);
			VI_DEBUG("[uplink] expire fd %i of %s", (int)Stream->GetFd(), URL.c_str());
			VI_RELEASE(Stream);
		}
		void Uplinks::ListenConnectionURL(const Core::String& URL, Socket* Target)
		{
			Multiplexer::Get()->WhenReadable(Target, [this, URL, Target](SocketPoll Event)
			{
				if (Packet::IsError(Event))
					ExpireConnectionURL(URL, Target);
				else if (!Packet::IsSkip(Event))
					ListenConnectionURL(URL, Target);
			});
		}
		void Uplinks::UnlistenConnection(Socket* Target)
		{
			Target->ClearEvents(false);
		}
		bool Uplinks::PushConnection(RemoteHost* Address, Socket* Stream)
		{
			VI_ASSERT(Stream != nullptr, "socket should be set");
			VI_ASSERT(Address != nullptr, "address should be set");
			if (!Stream->IsValid())
				return false;

			Core::String URL = GetURL(Address);
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				Connections[URL].insert(Stream);
			}
			Stream->Timeout = 0;
			ListenConnectionURL(URL, Stream);
			VI_DEBUG("[uplink] store fd %i of %s", (int)Stream->GetFd(), URL.c_str());
			return true;
		}
		Socket* Uplinks::PopConnection(RemoteHost* Address)
		{
			VI_ASSERT(Address != nullptr, "address should be set");
			Core::String URL = GetURL(Address);
			Socket* Stream = nullptr;
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				auto Targets = Connections.find(URL);
				if (Targets == Connections.end() || Targets->second.empty())
					return nullptr;

				auto It = Targets->second.begin();
				Stream = *It;
				Targets->second.erase(It);
			}
			UnlistenConnection(Stream);
			VI_DEBUG("[uplink] reuse fd %i of %s", (int)Stream->GetFd(), URL.c_str());
			return Stream;
		}
		size_t Uplinks::GetSize()
		{
			size_t Size = 0;
			Core::UMutex<std::mutex> Unique(Exclusive);
			for (auto& Targets : Connections)
				Size += Targets.second.size();
			return Size;
		}
		Core::String Uplinks::GetURL(RemoteHost* Address)
		{
			return Address->Hostname + ':' + Core::ToString(Address->Port) + (Address->Secure ? "@secure" : "@default");
		}

		CertificateBuilder::CertificateBuilder() noexcept
		{
#ifdef VI_OPENSSL
			Certificate = X509_new();
			PrivateKey = EVP_PKEY_new();
#endif
		}
		CertificateBuilder::~CertificateBuilder() noexcept
		{
#ifdef VI_OPENSSL
			if (Certificate != nullptr)
				X509_free((X509*)Certificate);
			if (PrivateKey != nullptr)
				EVP_PKEY_free((EVP_PKEY*)PrivateKey);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::SetSerialNumber(uint32_t Bits)
		{
#ifdef VI_OPENSSL
			VI_ASSERT(Bits > 0, "bits should be greater than zero");
			ASN1_STRING* SerialNumber = X509_get_serialNumber((X509*)Certificate);
			if (!SerialNumber)
				return std::make_error_condition(std::errc::bad_message);

			auto Data = Compute::Crypto::RandomBytes(Bits / 8);
			if (!Data)
				return std::make_error_condition(std::errc::bad_message);

			if (!ASN1_STRING_set(SerialNumber, Data->data(), static_cast<int>(Data->size())))
				return std::make_error_condition(std::errc::invalid_argument);

			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::SetVersion(uint8_t Version)
		{
#ifdef VI_OPENSSL
			if (!X509_set_version((X509*)Certificate, Version))
				return std::make_error_condition(std::errc::invalid_argument);

			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::SetNotAfter(int64_t OffsetSeconds)
		{
#ifdef VI_OPENSSL
			if (!X509_gmtime_adj(X509_get_notAfter((X509*)Certificate), (long)OffsetSeconds))
				return std::make_error_condition(std::errc::invalid_argument);

			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::SetNotBefore(int64_t OffsetSeconds)
		{
#ifdef VI_OPENSSL
			if (!X509_gmtime_adj(X509_get_notBefore((X509*)Certificate), (long)OffsetSeconds))
				return std::make_error_condition(std::errc::invalid_argument);

			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::SetPublicKey(uint32_t Bits)
		{
#ifdef VI_OPENSSL
			VI_ASSERT(Bits > 0, "bits should be greater than zero");
			EVP_PKEY* NewPrivateKey = EVP_RSA_gen(Bits);
			if (!NewPrivateKey)
				return std::make_error_condition(std::errc::function_not_supported);

			EVP_PKEY_free((EVP_PKEY*)PrivateKey);
			PrivateKey = NewPrivateKey;

			if (!X509_set_pubkey((X509*)Certificate, (EVP_PKEY*)PrivateKey))
				return std::make_error_condition(std::errc::bad_message);

			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::SetIssuer(const X509Blob& Issuer)
		{
#ifdef VI_OPENSSL
			if (!Issuer.Pointer)
				return std::make_error_condition(std::errc::invalid_argument);

			X509_NAME* SubjectName = X509_get_subject_name((X509*)Issuer.Pointer);
			if (!SubjectName)
				return std::make_error_condition(std::errc::invalid_argument);
	
			if (!X509_set_issuer_name((X509*)Certificate, SubjectName))
				return std::make_error_condition(std::errc::invalid_argument);

			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::AddCustomExtension(bool Critical, const Core::String& Key, const Core::String& Value, const Core::String& Description)
		{
#ifdef VI_OPENSSL
			VI_ASSERT(!Key.empty(), "key should not be empty");
			const int NID = OBJ_create(Key.c_str(), Value.c_str(), Description.empty() ? nullptr : Description.c_str());
			ASN1_OCTET_STRING* Data = ASN1_OCTET_STRING_new();
			if (!Data)
				return std::make_error_condition(std::errc::not_enough_memory);

			if (!ASN1_OCTET_STRING_set(Data, reinterpret_cast<unsigned const char*>(Value.data()), (int)Value.size()))
			{
				ASN1_OCTET_STRING_free(Data);
				return std::make_error_condition(std::errc::invalid_argument);
			}

			X509_EXTENSION* Extension = X509_EXTENSION_create_by_NID(nullptr, NID, Critical, Data);
			if (!Extension)
			{
				ASN1_OCTET_STRING_free(Data);
				return std::make_error_condition(std::errc::invalid_argument);
			}

			bool Success = X509_add_ext((X509*)Certificate, Extension, -1) == 1;
			X509_EXTENSION_free(Extension);
			ASN1_OCTET_STRING_free(Data);

			if (!Success)
				return std::make_error_condition(std::errc::invalid_argument);

			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::AddStandardExtension(const X509Blob& Issuer, int NID, const Core::String& Value)
		{
#ifdef VI_OPENSSL
			X509V3_CTX Context;
			X509V3_set_ctx_nodb(&Context);
			X509V3_set_ctx(&Context, (X509*)(Issuer.Pointer ? Issuer.Pointer : Certificate), (X509*)Certificate, nullptr, nullptr, 0);

			X509_EXTENSION* Extension = X509V3_EXT_conf_nid(nullptr, &Context, NID, Value.c_str());
			if (!Extension)
				return std::make_error_condition(std::errc::invalid_argument);

			bool Success = X509_add_ext((X509*)Certificate, Extension, -1) == 1;
			X509_EXTENSION_free(Extension);
			if (!Success)
				return std::make_error_condition(std::errc::invalid_argument);

			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::AddStandardExtension(const X509Blob& Issuer, const Core::String& NameOfNID, const Core::String& Value)
		{
#ifdef VI_OPENSSL
			VI_ASSERT(!NameOfNID.empty(), "name of nid should not be empty");
			int NID = OBJ_txt2nid(NameOfNID.c_str());
			if (NID == NID_undef)
				return std::make_error_condition(std::errc::invalid_argument);

			return AddStandardExtension(Issuer, NID, Value);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::AddSubjectInfo(const Core::String& Key, const Core::String& Value)
		{
#ifdef VI_OPENSSL
			VI_ASSERT(!Key.empty(), "key should not be empty");
			X509_NAME* SubjectName = X509_get_subject_name((X509*)Certificate);
			if (!SubjectName)
				return std::make_error_condition(std::errc::bad_message);

			if (!X509_NAME_add_entry_by_txt(SubjectName, Key.c_str(), MBSTRING_ASC, (unsigned char*)Value.c_str(), (int)Value.size(), -1, 0))
				return std::make_error_condition(std::errc::invalid_argument);

			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::AddIssuerInfo(const Core::String& Key, const Core::String& Value)
		{
#ifdef VI_OPENSSL
			VI_ASSERT(!Key.empty(), "key should not be empty");
			X509_NAME* IssuerName = X509_get_issuer_name((X509*)Certificate);
			if (!IssuerName)
				return std::make_error_condition(std::errc::invalid_argument);

			if (!X509_NAME_add_entry_by_txt(IssuerName, Key.c_str(), MBSTRING_ASC, (unsigned char*)Value.c_str(), (int)Value.size(), -1, 0))
				return std::make_error_condition(std::errc::invalid_argument);

			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> CertificateBuilder::Sign(Compute::Digest Algorithm)
		{
#ifdef VI_OPENSSL
			VI_ASSERT(Algorithm != nullptr, "algorithm should be set");
			if (X509_sign((X509*)Certificate, (EVP_PKEY*)PrivateKey, (EVP_MD*)Algorithm) != 0)
				return std::make_error_condition(std::errc::invalid_argument);

			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<CertificateBlob> CertificateBuilder::Build()
		{
#ifdef VI_OPENSSL
			CertificateBlob Blob;
			BIO* Bio = BIO_new(BIO_s_mem());
			PEM_write_bio_X509(Bio, (X509*)Certificate);

			BUF_MEM* BioMemory = nullptr;
			if (BIO_get_mem_ptr(Bio, &BioMemory) != 0 && BioMemory->data != nullptr && BioMemory->length > 0)
				Blob.Certificate = Core::String(BioMemory->data, (size_t)BioMemory->length);

			BIO_free(Bio);
			Bio = BIO_new(BIO_s_mem());
			PEM_write_bio_PrivateKey(Bio, (EVP_PKEY*)PrivateKey, nullptr, nullptr, 0, nullptr, nullptr);

			BioMemory = nullptr;
			if (BIO_get_mem_ptr(Bio, &BioMemory) != 0 && BioMemory->data != nullptr && BioMemory->length > 0)
				Blob.PrivateKey = Core::String(BioMemory->data, (size_t)BioMemory->length);

			BIO_free(Bio);
			return Blob;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		void* CertificateBuilder::GetCertificateX509()
		{
			return Certificate;
		}
		void* CertificateBuilder::GetPrivateKeyEVP_PKEY()
		{
			return PrivateKey;
		}

		SocketAddress::SocketAddress(addrinfo* NewNames, addrinfo* NewUsable) noexcept : Names(NewNames), Usable(NewUsable)
		{
		}
		SocketAddress::~SocketAddress() noexcept
		{
			if (Names != nullptr)
				freeaddrinfo(Names);
		}
		bool SocketAddress::IsUsable() const
		{
			return Usable != nullptr;
		}
		addrinfo* SocketAddress::Get() const
		{
			return Usable;
		}
		addrinfo* SocketAddress::GetAlternatives() const
		{
			return Names;
		}
		Core::String SocketAddress::GetAddress() const
		{
			if (!Usable)
				return Core::String();

			return Utils::GetAddress(Usable);
		}

		Socket::Socket() noexcept : Device(nullptr), Fd(INVALID_SOCKET), Timeout(0), Income(0), Outcome(0), UserData(nullptr)
		{
		}
		Socket::Socket(socket_t FromFd) noexcept : Socket()
		{
			Fd = FromFd;
		}
		Socket::Socket(Socket&& Other) noexcept : Device(Other.Device), Fd(Other.Fd), Timeout(Other.Timeout), Income(Other.Income), Outcome(Other.Outcome), UserData(Other.UserData)
		{
			Events = std::move(Other.Events);
			Other.Device = nullptr;
			Other.Fd = INVALID_SOCKET;
		}
		Socket::~Socket() noexcept
		{
			Close(false);
		}
		Socket& Socket::operator= (Socket&& Other) noexcept
		{
			if (this == &Other)
				return *this;

			Close(false);
			Device = Other.Device;
			Fd = Other.Fd;
			Timeout = Other.Timeout;
			Income = Other.Income;
			Outcome = Other.Outcome;
			UserData = Other.UserData;
			Events = std::move(Other.Events);
			Other.Device = nullptr;
			Other.Fd = INVALID_SOCKET;
			return *this;
		}
		Core::ExpectsIO<void> Socket::Accept(Socket* OutConnection, char* OutAddr)
		{
			VI_ASSERT(OutConnection != nullptr, "socket should be set");
			return Accept(&OutConnection->Fd, OutAddr);
		}
		Core::ExpectsIO<void> Socket::Accept(socket_t* OutFd, char* OutAddr)
		{
			VI_ASSERT(OutFd != nullptr, "socket should be set");

			sockaddr Address;
			socket_size_t Length = sizeof(sockaddr);
			*OutFd = accept(Fd, &Address, &Length);
			if (*OutFd == INVALID_SOCKET)
			{
				VI_TRACE("[net] fd %i: not acceptable", (int)Fd);
				return Core::OS::Error::GetConditionOr();
			}

			VI_DEBUG("[net] accept fd %i on %i fd", (int)*OutFd, (int)Fd);
			if (OutAddr != nullptr)
				inet_ntop(Address.sa_family, GetAddressStorage(&Address), OutAddr, INET6_ADDRSTRLEN);

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::AcceptAsync(bool WithAddress, SocketAcceptedCallback&& Callback)
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			if (!Multiplexer::Get()->WhenReadable(this, [this, WithAddress, Callback = std::move(Callback)](SocketPoll Event) mutable
			{
				if (!Packet::IsDone(Event))
					return;

				socket_t OutFd = INVALID_SOCKET;
				char OutAddr[INET6_ADDRSTRLEN] = { };
				char* RemoteAddr = (WithAddress ? OutAddr : nullptr);
				while (Accept(&OutFd, RemoteAddr))
				{
					if (!Callback(OutFd, RemoteAddr))
						break;
				}

				AcceptAsync(WithAddress, std::move(Callback));
			}))
				return Core::OS::Error::GetConditionOr();

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::Close(bool Gracefully)
		{
#ifdef VI_OPENSSL
			if (Device != nullptr)
			{
				VI_TRACE("[net] fd %i free ssl device", (int)Fd);
				SSL_free(Device);
				Device = nullptr;
			}
#endif
			if (Fd == INVALID_SOCKET)
				return std::make_error_condition(std::errc::bad_file_descriptor);
			else
				ClearEvents(false);

			int Error = 1;
			socklen_t Size = sizeof(Error);
			getsockopt(Fd, SOL_SOCKET, SO_ERROR, (char*)&Error, &Size);
			VI_TRACE("[net] fd %i fetch errors: %i", (int)Fd, Error);

			if (Gracefully)
			{
				VI_TRACE("[net] fd %i graceful shutdown", (int)Fd);
				VI_MEASURE(Core::Timings::Networking);
				int Timeout = 100;
				SetBlocking(true);
				SetSocket(SO_RCVTIMEO, &Timeout, sizeof(int));
				shutdown(Fd, SD_SEND);

				while (recv(Fd, (char*)&Error, 1, 0) > 0)
					VI_MEASURE_LOOP();
			}
			else
			{
				VI_TRACE("[net] fd %i shutdown", (int)Fd);
				shutdown(Fd, SD_SEND);
			}

			closesocket(Fd);
			VI_DEBUG("[net] sock fd %i closed", (int)Fd);
			Fd = INVALID_SOCKET;
			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::CloseAsync(bool Gracefully, SocketStatusCallback&& Callback)
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
#ifdef VI_OPENSSL
			if (Device != nullptr)
			{
				VI_TRACE("[net] fd %i free ssl device", (int)Fd);
				SSL_free(Device);
				Device = nullptr;
			}
#endif
			if (Fd == INVALID_SOCKET)
			{
				auto Condition = std::make_error_condition(std::errc::bad_file_descriptor);
				Callback(Condition);
				return Condition;
			}
			else
				ClearEvents(false);

			int Error = 1;
			socklen_t Size = sizeof(Error);
			getsockopt(Fd, SOL_SOCKET, SO_ERROR, (char*)&Error, &Size);
			shutdown(Fd, SD_SEND);
			VI_TRACE("[net] fd %i fetch errors: %i", (int)Fd, Error);

			if (Gracefully)
			{
				VI_TRACE("[net] fd %i graceful shutdown", (int)Fd);
				return TryCloseAsync(std::move(Callback), true);
			}

			closesocket(Fd);
			VI_TRACE("[net] fd %i shutdown", (int)Fd);
			VI_DEBUG("[net] sock fd %i closed", (int)Fd);
			Fd = INVALID_SOCKET;

			Callback(Core::Optional::None);
			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::TryCloseAsync(SocketStatusCallback&& Callback, bool KeepTrying)
		{
			while (KeepTrying)
			{
				char Buffer;
				auto Status = Read(&Buffer, 1);
				if (Status)
					continue;
				else if (Status.Error() != std::errc::operation_would_block)
					break;

				Timeout = 500;
				Multiplexer::Get()->WhenReadable(this, [this, Callback = std::move(Callback)](SocketPoll Event) mutable
				{
					if (!Packet::IsSkip(Event))
						TryCloseAsync(std::move(Callback), Packet::IsDone(Event));
					else
						Callback(Core::Optional::None);
				});
				return Status.Error();
			}

			ClearEvents(false);
			closesocket(Fd);
			VI_DEBUG("[net] sock fd %i closed", (int)Fd);
			Fd = INVALID_SOCKET;

			Callback(Core::Optional::None);
			return Core::Expectation::Met;
		}
		Core::ExpectsIO<size_t> Socket::SendFile(FILE* Stream, size_t Offset, size_t Size)
		{
			VI_ASSERT(Stream != nullptr, "stream should be set");
			VI_ASSERT(Offset >= 0, "offset should be set and positive");
			VI_ASSERT(Size > 0, "size should be set and greater than zero");
			VI_MEASURE(Core::Timings::Networking);
			VI_TRACE("[net] fd %i sendfile %" PRId64 " off, %" PRId64 " bytes", (int)Fd, Offset, Size);
			off_t Seek = (off_t)Offset, Length = (off_t)Size;
#ifdef VI_OPENSSL
			if (Device != nullptr)
			{
				static bool IsUnsupported = false;
				if (IsUnsupported)
					return std::make_error_condition(std::errc::not_supported);

				ossl_ssize_t Value = SSL_sendfile(Device, VI_FILENO(Stream), Seek, Length, 0);
				if (Value < 0)
				{
					auto Condition = Utils::GetLastError(Device, (int)Value);
					IsUnsupported = (Condition == std::errc::protocol_error);
					return IsUnsupported ? std::make_error_condition(std::errc::not_supported) : Condition;
				}

				size_t Written = (size_t)Value;
				Outcome += Written;
				return Written;
			}
#endif
#ifdef VI_APPLE
			int Value = sendfile(VI_FILENO(Stream), Fd, Seek, &Length, nullptr, 0);
			if (Value < 0)
				return Utils::GetLastError(Device, Value);

			size_t Written = (size_t)Length;
			Outcome += Written;
			return Written;
#elif defined(VI_LINUX)
			ssize_t Value = sendfile(Fd, VI_FILENO(Stream), &Seek, Size);
			if (Value < 0)
				return Utils::GetLastError(Device, (int)Value);

			size_t Written = (size_t)Value;
			Outcome += Written;
			return Written;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<size_t> Socket::SendFileAsync(FILE* Stream, size_t Offset, size_t Size, SocketWrittenCallback&& Callback, size_t TempBuffer)
		{
			VI_ASSERT(Stream != nullptr, "stream should be set");
			VI_ASSERT(Callback != nullptr, "callback should be set");
			VI_ASSERT(Offset >= 0, "offset should be set and positive");
			VI_ASSERT(Size > 0, "size should be set and greater than zero");

			size_t Written = 0;
			while (Size > 0)
			{
				auto Status = SendFile(Stream, Offset, Size);
				if (!Status)
				{
					if (Status.Error() == std::errc::operation_would_block)
					{
						Multiplexer::Get()->WhenWriteable(this, [this, TempBuffer, Stream, Offset, Size, Callback = std::move(Callback)](SocketPoll Event) mutable
						{
							if (Packet::IsDone(Event))
								SendFileAsync(Stream, Offset, Size, std::move(Callback), ++TempBuffer);
							else
								Callback(Event);
						});
					}
					else if (Status.Error() != std::errc::not_supported)
						Callback(SocketPoll::Reset);

					return Status;
				}
				else
				{
					size_t WrittenSize = *Status;
					Size -= WrittenSize;
					Offset += WrittenSize;
					Written += WrittenSize;
				}
			}

			Callback(TempBuffer != 0 ? SocketPoll::Finish : SocketPoll::FinishSync);
			return Written;
		}
		Core::ExpectsIO<size_t> Socket::Write(const char* Buffer, size_t Size)
		{
			VI_ASSERT(Fd != INVALID_SOCKET, "socket should be valid");
			VI_MEASURE(Core::Timings::Networking);
			VI_TRACE("[net] fd %i write %i bytes", (int)Fd, (int)Size);
#ifdef VI_OPENSSL
			if (Device != nullptr)
			{
				int Value = SSL_write(Device, Buffer, (int)Size);
				if (Value <= 0)
				{
					ERR_print_errors_fp(stderr);
					return Utils::GetLastError(Device, Value);
				}

				size_t Written = (size_t)Value;
				Outcome += Written;
				return Written;
			}
#endif
			int Value = send(Fd, Buffer, (int)Size, 0);
			if (Value <= 0)
				return Utils::GetLastError(Device, Value);

			size_t Written = (size_t)Value;
			Outcome += Written;
			return Written;
		}
		Core::ExpectsIO<size_t> Socket::WriteAsync(const char* Buffer, size_t Size, SocketWrittenCallback&& Callback, char* TempBuffer, size_t TempOffset)
		{
			VI_ASSERT(Buffer != nullptr && Size > 0, "buffer should be set");
			VI_ASSERT(Callback != nullptr, "callback should be set");
			size_t Payload = Size;
			size_t Written = 0;

			while (Size > 0)
			{
				auto Status = Write(Buffer + TempOffset, (int)Size);
				if (!Status)
				{
					if (Status.Error() == std::errc::operation_would_block)
					{
						if (!TempBuffer)
						{
							TempBuffer = VI_MALLOC(char, Payload);
							memcpy(TempBuffer, Buffer, Payload);
						}

						Multiplexer::Get()->WhenWriteable(this, [this, TempBuffer, TempOffset, Size, Callback = std::move(Callback)](SocketPoll Event) mutable
						{
							if (!Packet::IsDone(Event))
							{
								VI_FREE(TempBuffer);
								Callback(Event);
							}
							else
								WriteAsync(TempBuffer, Size, std::move(Callback), TempBuffer, TempOffset);
						});
					}
					else
					{
						if (TempBuffer != nullptr)
							VI_FREE(TempBuffer);
						Callback(SocketPoll::Reset);
					}

					return Status;
				}
				else
				{
					size_t WrittenSize = *Status;
					Size -= WrittenSize;
					TempOffset += WrittenSize;
					Written += WrittenSize;
				}
			}

			if (TempBuffer != nullptr)
				VI_FREE(TempBuffer);

			Callback(TempBuffer ? SocketPoll::Finish : SocketPoll::FinishSync);
			return Written;
		}
		Core::ExpectsIO<size_t> Socket::Read(char* Buffer, size_t Size)
		{
			VI_ASSERT(Fd != INVALID_SOCKET, "socket should be valid");
			VI_ASSERT(Buffer != nullptr && Size > 0, "buffer should be set");
			VI_MEASURE(Core::Timings::Networking);
			VI_TRACE("[net] fd %i read %i bytes", (int)Fd, (int)Size);
#ifdef VI_OPENSSL
			if (Device != nullptr)
			{
				int Value = SSL_read(Device, Buffer, (int)Size);
				if (Value <= 0)
					return Utils::GetLastError(Device, Value);

				size_t Received = (size_t)Value;
				Income += Received;
				return Received;
			}
#endif
			int Value = recv(Fd, Buffer, (int)Size, 0);
			if (Value <= 0)
				return Utils::GetLastError(Device, Value);

			size_t Received = (size_t)Value;
			Income += Received;
			return Received;
		}
		Core::ExpectsIO<size_t> Socket::Read(char* Buffer, size_t Size, SocketReadCallback&& Callback)
		{
			VI_ASSERT(Fd != INVALID_SOCKET, "socket should be valid");
			VI_ASSERT(Buffer != nullptr && Size > 0, "buffer should be set");
			VI_ASSERT(Callback != nullptr, "callback should be set");

			size_t Offset = 0;
			while (Size > 0)
			{
				auto Status = Read(Buffer + Offset, Size);
				if (!Status)
				{
					if (Status.Error() == std::errc::operation_would_block)
						Callback(SocketPoll::Timeout, nullptr, 0);
					else
						Callback(SocketPoll::Reset, nullptr, 0);

					return Status;
				}

				size_t Received = *Status;
				if (!Callback(SocketPoll::Next, Buffer + Offset, Received))
					break;

				Size -= Received;
				Offset += Received;
			}

			Callback(SocketPoll::FinishSync, nullptr, 0);
			return Offset;
		}
		Core::ExpectsIO<size_t> Socket::ReadAsync(size_t Size, SocketReadCallback&& Callback, size_t TempBuffer)
		{
			VI_ASSERT(Fd != INVALID_SOCKET, "socket should be valid");
			VI_ASSERT(Callback != nullptr, "callback should be set");
			VI_ASSERT(Size > 0, "size should be greater than zero");

			char Buffer[Core::BLOB_SIZE];
			size_t Offset = 0;

			while (Size > 0)
			{
				auto Status = Read(Buffer, Size > sizeof(Buffer) ? sizeof(Buffer) : Size);
				if (!Status)
				{
					if (Status.Error() == std::errc::operation_would_block)
					{
						Multiplexer::Get()->WhenReadable(this, [this, Size, TempBuffer, Callback = std::move(Callback)](SocketPoll Event) mutable
						{
							if (Packet::IsDone(Event))
								ReadAsync(Size, std::move(Callback), ++TempBuffer);
							else
								Callback(Event, nullptr, 0);
						});
					}
					else
						Callback(SocketPoll::Reset, nullptr, 0);

					return Status;
				}

				size_t Received = *Status;
				if (!Callback(SocketPoll::Next, Buffer, Received))
					break;

				Size -= Received;
				Offset += Received;
			}

			Callback(TempBuffer != 0 ? SocketPoll::Finish : SocketPoll::FinishSync, nullptr, 0);
			return Offset;
		}
		Core::ExpectsIO<size_t> Socket::ReadUntil(const char* Match, SocketReadCallback&& Callback)
		{
			VI_ASSERT(Fd != INVALID_SOCKET, "socket should be valid");
			VI_ASSERT(Match != nullptr && Match[0] != '\0', "match should be set");
			VI_ASSERT(Callback != nullptr, "callback should be set");

			char Buffer[MAX_READ_UNTIL];
			size_t Size = strlen(Match), Cache = 0, Receiving = 0, Index = 0;
			auto Publish = [&Callback, &Buffer, &Cache, &Receiving](size_t Size)
			{
				Receiving += Cache; Cache = 0;
				return Callback(SocketPoll::Next, Buffer, Size);
			};

			VI_ASSERT(Size > 0, "match should not be empty");
			while (Index < Size)
			{
				auto Status = Read(Buffer + Cache, 1);
				if (!Status)
				{
					if (!Cache || Publish(Cache))
						Callback(SocketPoll::Reset, nullptr, 0);
					return Status;
				}

				size_t Offset = Cache;
				if (++Cache >= sizeof(Buffer) && !Publish(Cache))
					break;

				if (Match[Index] == Buffer[Offset])
				{
					if (++Index >= Size)
					{
						Publish(Cache);
						break;
					}
				}
				else
					Index = 0;
			}

			Callback(SocketPoll::FinishSync, nullptr, 0);
			return Receiving;
		}
		Core::ExpectsIO<size_t> Socket::ReadUntilAsync(Core::String&& Match, SocketReadCallback&& Callback, size_t TempIndex, bool TempBuffer)
		{
			VI_ASSERT(Fd != INVALID_SOCKET, "socket should be valid");
			VI_ASSERT(!Match.empty(), "match should be set");
			VI_ASSERT(Callback != nullptr, "callback should be set");

			char Buffer[MAX_READ_UNTIL];
			size_t Size = Match.size(), Cache = 0, Receiving = 0;
			auto Publish = [&Callback, &Buffer, &Cache, &Receiving](size_t Size)
			{
				Receiving += Cache; Cache = 0;
				return Callback(SocketPoll::Next, Buffer, Size);
			};

			VI_ASSERT(Size > 0, "match should not be empty");
			while (TempIndex < Size)
			{
				auto Status = Read(Buffer + Cache, 1);
				if (!Status)
				{
					if (Status.Error() == std::errc::operation_would_block)
					{
						Multiplexer::Get()->WhenReadable(this, [this, TempIndex, Match = std::move(Match), Callback = std::move(Callback)](SocketPoll Event) mutable
						{
							if (Packet::IsDone(Event))
								ReadUntilAsync(std::move(Match), std::move(Callback), TempIndex, true);
							else
								Callback(Event, nullptr, 0);
						});
					}
					else if (!Cache || Publish(Cache))
						Callback(SocketPoll::Reset, nullptr, 0);

					return Status;
				}

				size_t Offset = Cache++;
				if (Cache >= sizeof(Buffer) && !Publish(Cache))
					break;

				if (Match[TempIndex] == Buffer[Offset])
				{
					if (++TempIndex >= Size)
					{
						Publish(Cache);
						break;
					}
				}
				else
					TempIndex = 0;
			}

			Callback(TempBuffer ? SocketPoll::Finish : SocketPoll::FinishSync, nullptr, 0);
			return Receiving;
		}
		Core::ExpectsIO<void> Socket::Connect(SocketAddress* Address, uint64_t Timeout)
		{
			VI_ASSERT(Address && Address->IsUsable(), "address should be set and usable");
			VI_MEASURE(Core::Timings::Networking);
			VI_DEBUG("[net] connect fd %i", (int)Fd);
			addrinfo* Source = Address->Get();
			if (connect(Fd, Source->ai_addr, (int)Source->ai_addrlen) != 0)
				return Core::OS::Error::GetConditionOr();

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::ConnectAsync(SocketAddress* Address, SocketStatusCallback&& Callback)
		{
			VI_ASSERT(Address && Address->IsUsable(), "address should be set and usable");
			VI_ASSERT(Callback != nullptr, "callback should be set");
			VI_MEASURE(Core::Timings::Networking);
			VI_DEBUG("[net] connect fd %i", (int)Fd);

			addrinfo* Source = Address->Get();
			int Status = connect(Fd, Source->ai_addr, (int)Source->ai_addrlen);
			if (Status == 0)
			{
				Callback(Core::Optional::None);
				return Core::Expectation::Met;
			}
			else if (Utils::GetLastError(Device, Status) != std::errc::operation_would_block)
			{
				auto Condition = Core::OS::Error::GetConditionOr();
				Callback(Condition);
				return Condition;
			}

			Multiplexer::Get()->WhenWriteable(this, [Callback = std::move(Callback)](SocketPoll Event) mutable
			{
				if (Packet::IsDone(Event))
					Callback(Core::Optional::None);
				else if (Packet::IsTimeout(Event))
					Callback(std::make_error_condition(std::errc::timed_out));
				else
					Callback(std::make_error_condition(std::errc::connection_refused));
			});
			return std::make_error_condition(std::errc::operation_would_block);
		}
		Core::ExpectsIO<void> Socket::Open(SocketAddress* Address)
		{
			VI_ASSERT(Address && Address->IsUsable(), "address should be set and usable");
			VI_MEASURE(Core::Timings::Networking);
			VI_DEBUG("[net] assign fd");

			addrinfo* Source = Address->Get();
			Fd = socket(Source->ai_family, Source->ai_socktype, Source->ai_protocol);
			if (Fd == INVALID_SOCKET)
				return Core::OS::Error::GetConditionOr();

			int Option = 1;
			setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, (char*)&Option, sizeof(Option));
			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::Secure(ssl_ctx_st* Context, const char* Hostname)
		{
#ifdef VI_OPENSSL
			VI_MEASURE(Core::Timings::Networking);
			VI_TRACE("[net] fd %i create ssl device on %s", (int)Fd, Hostname);
			if (Device != nullptr)
				SSL_free(Device);

			Device = SSL_new(Context);
			if (!Device)
				return std::make_error_condition(std::errc::broken_pipe);

			SSL_set_mode(Device, SSL_MODE_ENABLE_PARTIAL_WRITE);
			SSL_set_mode(Device, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
#ifndef OPENSSL_NO_TLSEXT
			if (Hostname != nullptr)
				SSL_set_tlsext_host_name(Device, Hostname);
#endif
#endif
			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::Bind(SocketAddress* Address)
		{
			VI_ASSERT(Address && Address->IsUsable(), "address should be set and usable");
			VI_MEASURE(Core::Timings::Networking);
			VI_DEBUG("[net] bind fd %i", (int)Fd);

			addrinfo* Source = Address->Get();
			if (bind(Fd, Source->ai_addr, (int)Source->ai_addrlen) != 0)
				return Core::OS::Error::GetConditionOr();

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::Listen(int Backlog)
		{
			VI_MEASURE(Core::Timings::Networking);
			VI_DEBUG("[net] listen fd %i", (int)Fd);
			if (listen(Fd, Backlog) != 0)
				return Core::OS::Error::GetConditionOr();

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::ClearEvents(bool Gracefully)
		{
			VI_MEASURE(Core::Timings::Networking);
			VI_TRACE("[net] fd %i clear events %s", (int)Fd, Gracefully ? "gracefully" : "forcefully");
			bool Success;
			if (Gracefully)
				Success = Multiplexer::Get()->CancelEvents(this, SocketPoll::Reset, true);
			else
				Success = Multiplexer::Get()->ClearEvents(this);

			if (!Success)
				return std::make_error_condition(std::errc::not_connected);

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::MigrateTo(socket_t NewFd, bool Gracefully)
		{
			VI_MEASURE(Core::Timings::Networking);
			VI_TRACE("[net] migrate fd %i to fd %i", (int)Fd, (int)NewFd);
			if (!Gracefully)
			{
				Fd = NewFd;
				return Core::Expectation::Met;
			}

			auto Status = ClearEvents(false);
			Fd = NewFd;
			return Status;
		}
		Core::ExpectsIO<void> Socket::SetCloseOnExec()
		{
			VI_TRACE("[net] fd %i setopt: cloexec", (int)Fd);
#if defined(_WIN32)
			return std::make_error_condition(std::errc::not_supported);
#else
			if (fcntl(Fd, F_SETFD, FD_CLOEXEC) == -1)
				return Core::OS::Error::GetConditionOr();

			return Core::Expectation::Met;
#endif
		}
		Core::ExpectsIO<void> Socket::SetTimeWait(int Timeout)
		{
			linger Linger;
			Linger.l_onoff = (Timeout >= 0 ? 1 : 0);
			Linger.l_linger = Timeout;

			VI_TRACE("[net] fd %i setopt: timewait %i", (int)Fd, Timeout);
			if (setsockopt(Fd, SOL_SOCKET, SO_LINGER, (char*)&Linger, sizeof(Linger)) != 0)
				return Core::OS::Error::GetConditionOr();

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::SetSocket(int Option, void* Value, size_t Size)
		{
			VI_TRACE("[net] fd %i setsockopt: opt%i %i bytes", (int)Fd, Option, (int)Size);
			if (::setsockopt(Fd, SOL_SOCKET, Option, (const char*)Value, (int)Size) != 0)
				return Core::OS::Error::GetConditionOr();

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::SetAny(int Level, int Option, void* Value, size_t Size)
		{
			VI_TRACE("[net] fd %i setopt: l%i opt%i %i bytes", (int)Fd, Level, Option, (int)Size);
			if (::setsockopt(Fd, Level, Option, (const char*)Value, (int)Size) != 0)
				return Core::OS::Error::GetConditionOr();

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::SetSocketFlag(int Option, int Value)
		{
			return SetSocket(Option, (void*)&Value, sizeof(int));
		}
		Core::ExpectsIO<void> Socket::SetAnyFlag(int Level, int Option, int Value)
		{
			VI_TRACE("[net] fd %i setopt: l%i opt%i %i", (int)Fd, Level, Option, Value);
			return SetAny(Level, Option, (void*)&Value, sizeof(int));
		}
		Core::ExpectsIO<void> Socket::SetBlocking(bool Enabled)
		{
			VI_TRACE("[net] fd %i setopt: blocking %s", (int)Fd, Enabled ? "on" : "off");
#ifdef VI_MICROSOFT
			unsigned long Mode = (Enabled ? 0 : 1);
			if (ioctlsocket(Fd, (long)FIONBIO, &Mode) != 0)
				return Core::OS::Error::GetConditionOr();
#else
			int Flags = fcntl(Fd, F_GETFL, 0);
			if (Flags == -1)
				return Core::OS::Error::GetConditionOr();

			Flags = Enabled ? (Flags & ~O_NONBLOCK) : (Flags | O_NONBLOCK);
			if (fcntl(Fd, F_SETFL, Flags) == -1)
				return Core::OS::Error::GetConditionOr();
#endif
			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::SetNoDelay(bool Enabled)
		{
			return SetAnyFlag(IPPROTO_TCP, TCP_NODELAY, (Enabled ? 1 : 0));
		}
		Core::ExpectsIO<void> Socket::SetKeepAlive(bool Enabled)
		{
			return SetSocketFlag(SO_KEEPALIVE, (Enabled ? 1 : 0));
		}
		Core::ExpectsIO<void> Socket::SetTimeout(int Timeout)
		{
			VI_TRACE("[net] fd %i setopt: rwtimeout %i", (int)Fd, Timeout);
#ifdef VI_MICROSOFT
			DWORD Time = (DWORD)Timeout;
#else
			struct timeval Time;
			memset(&Time, 0, sizeof(Time));
			Time.tv_sec = Timeout / 1000;
			Time.tv_usec = (Timeout * 1000) % 1000000;
#endif
			if (setsockopt(Fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&Time, sizeof(Time)) != 0)
				return Core::OS::Error::GetConditionOr();

			if (setsockopt(Fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&Time, sizeof(Time)) != 0)
				return Core::OS::Error::GetConditionOr();

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::GetSocket(int Option, void* Value, size_t* Size)
		{
			return GetAny(SOL_SOCKET, Option, Value, Size);
		}
		Core::ExpectsIO<void> Socket::GetAny(int Level, int Option, void* Value, size_t* Size)
		{
			socket_size_t Length = (socket_size_t)Level;
			if (::getsockopt(Fd, Level, Option, (char*)Value, &Length) == -1)
				return Core::OS::Error::GetConditionOr();

			if (Size != nullptr)
				*Size = (size_t)Length;

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> Socket::GetSocketFlag(int Option, int* Value)
		{
			size_t Size = sizeof(int);
			return GetSocket(Option, (void*)Value, &Size);
		}
		Core::ExpectsIO<void> Socket::GetAnyFlag(int Level, int Option, int* Value)
		{
			return GetAny(Level, Option, (char*)Value, nullptr);
		}
		Core::ExpectsIO<Core::String> Socket::GetRemoteAddress()
		{
			struct sockaddr_storage Address;
			socklen_t Size = sizeof(Address);

			if (getpeername(Fd, (struct sockaddr*)&Address, &Size) == -1)
				return Core::OS::Error::GetConditionOr();

			char Buffer[NI_MAXHOST];
			if (getnameinfo((struct sockaddr*)&Address, Size, Buffer, sizeof(Buffer), nullptr, 0, NI_NUMERICHOST) != 0)
				return Core::OS::Error::GetConditionOr();

			VI_TRACE("[net] fd %i remote address: %s", (int)Fd, Buffer);
			return Core::String(Buffer, strnlen(Buffer, sizeof(Buffer) - 1));
		}
		Core::ExpectsIO<int> Socket::GetPort()
		{
			struct sockaddr_storage Address;
			socket_size_t Size = sizeof(Address);
			if (getsockname(Fd, reinterpret_cast<struct sockaddr*>(&Address), &Size) == -1)
				return Core::OS::Error::GetConditionOr();

			if (Address.ss_family == AF_INET)
				return ntohs(reinterpret_cast<struct sockaddr_in*>(&Address)->sin_port);

			if (Address.ss_family == AF_INET6)
				return ntohs(reinterpret_cast<struct sockaddr_in6*>(&Address)->sin6_port);

			return std::make_error_condition(std::errc::bad_address);
		}
		Core::ExpectsIO<Certificate> Socket::GetCertificate()
		{
			if (!Device)
				return std::make_error_condition(std::errc::not_supported);
#ifdef VI_OPENSSL
			X509* Blob = SSL_get_peer_certificate(Device);
			if (!Blob)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			return Utils::GetCertificateFromX509(Blob);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
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
			return Multiplexer::Get()->IsAwaitingReadable(this);
		}
		bool Socket::IsPendingForWrite()
		{
			return Multiplexer::Get()->IsAwaitingWriteable(this);
		}
		bool Socket::IsPending()
		{
			return Multiplexer::Get()->IsAwaitingEvents(this);
		}
		bool Socket::IsSecure()
		{
			return Device != nullptr;
		}

		SocketListener::SocketListener(const Core::String& NewName, const RemoteHost& NewHost, SocketAddress* NewAddress) : Name(NewName), Hostname(NewHost), Source(NewAddress), Base(new Socket())
		{
		}
		SocketListener::~SocketListener() noexcept
		{
			VI_CLEAR(Base);
		}

		SocketRouter::~SocketRouter() noexcept
		{
#ifdef VI_OPENSSL
			auto* Transporter = TransportLayer::Get();
			for (auto& Item : Certificates)
				Transporter->FreeServerContext(Item.second.Context);
			Certificates.clear();
#endif
		}
		RemoteHost& SocketRouter::Listen(const Core::String& Hostname, int Port, bool Secure)
		{
			return Listen("*", Hostname, Port, Secure);
		}
		RemoteHost& SocketRouter::Listen(const Core::String& Pattern, const Core::String& Hostname, int Port, bool Secure)
		{
			RemoteHost& Listener = Listeners[Pattern.empty() ? "*" : Pattern];
			Listener.Hostname = Hostname;
			Listener.Port = Port;
			Listener.Secure = Secure;
			return Listener;
		}

		SocketConnection::~SocketConnection() noexcept
		{
			VI_CLEAR(Stream);
			VI_CLEAR(Host);
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
			char Buffer[Core::BLOB_SIZE];
			if (Info.Close)
				return Finish();

			va_list Args;
			va_start(Args, ErrorMessage);
			int Count = vsnprintf(Buffer, sizeof(Buffer), ErrorMessage, Args);
			va_end(Args);

			Info.Message.assign(Buffer, Count);
			return Finish(StatusCode);
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

		SocketServer::SocketServer() noexcept : Backlog(1024)
		{
			Multiplexer::Get()->Activate();
		}
		SocketServer::~SocketServer() noexcept
		{
			Unlisten();
			Multiplexer::Get()->Deactivate();
		}
		void SocketServer::SetRouter(SocketRouter* New)
		{
			VI_RELEASE(Router);
			Router = New;
		}
		void SocketServer::SetBacklog(size_t Value)
		{
			VI_ASSERT(Value > 0, "backlog must be greater than zero");
			Backlog = Value;
		}
		Core::ExpectsSystem<void> SocketServer::Configure(SocketRouter* NewRouter)
		{
			VI_ASSERT(State == ServerState::Idle, "server should not be running");
			if (NewRouter != nullptr)
			{
				VI_RELEASE(Router);
				Router = NewRouter;
			}
			else if (!Router && !(Router = OnAllocateRouter()))
			{
				VI_ASSERT(false, "router is not valid");
				return Core::SystemException("configure server: invalid router", std::make_error_condition(std::errc::invalid_argument));
			}

			auto ConfigureStatus = OnConfigure(Router);
			if (!ConfigureStatus)
				return ConfigureStatus;

			if (Router->Listeners.empty())
			{
				VI_ASSERT(false, "there are no listeners provided");
				return Core::SystemException("configure server: invalid listeners", std::make_error_condition(std::errc::invalid_argument));
			}

			DNS* Service = DNS::Get();
			for (auto&& It : Router->Listeners)
			{
				if (It.second.Hostname.empty())
					continue;

				auto Source = Service->FindAddressFromName(It.second.Hostname, Core::ToString(It.second.Port), DNSType::Listen, SocketProtocol::TCP, SocketType::Stream);
				if (!Source)
					return Core::SystemException(Core::Stringify::Text("resolve %s:%i listener error", It.second.Hostname.c_str(), (int)It.second.Port), std::make_error_condition(std::errc::host_unreachable));

				SocketListener* Value = new SocketListener(It.first, It.second, *Source);
				auto Status = Value->Base->Open(*Source);
				if (!Status)
					return Core::SystemException(Core::Stringify::Text("open %s:%i listener error", It.second.Hostname.c_str(), (int)It.second.Port), std::move(Status.Error()));

				Status = Value->Base->Bind(*Source);
				if (!Status)
					return Core::SystemException(Core::Stringify::Text("bind %s:%i listener error", It.second.Hostname.c_str(), (int)It.second.Port), std::move(Status.Error()));

				Status = Value->Base->Listen((int)Router->BacklogQueue);
				if (!Status)
					return Core::SystemException(Core::Stringify::Text("listen %s:%i listener error", It.second.Hostname.c_str(), (int)It.second.Port), std::move(Status.Error()));

				Value->Base->SetCloseOnExec();
				Value->Base->SetBlocking(false);
				Listeners.push_back(Value);

				if (It.second.Port <= 0)
				{
					auto Port = Value->Base->GetPort();
					if (!Port)
						return Core::SystemException(Core::Stringify::Text("fetch port of %s:%i listener error", It.second.Hostname.c_str(), (int)It.second.Port), std::move(Port.Error()));
					else
						It.second.Port = *Port;
				}
			}
#ifdef VI_OPENSSL
			for (auto&& It : Router->Certificates)
			{
				auto Context = TransportLayer::Get()->CreateServerContext(It.second.VerifyPeers, It.second.Options, It.second.Ciphers);
				if (!Context)
					return Core::SystemException("create server transport layer error: " + It.first, std::move(Context.Error()));

				It.second.Context = *Context;
				if (It.second.VerifyPeers > 0)
					SSL_CTX_set_verify(It.second.Context, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE, nullptr);
				else
					SSL_CTX_set_verify(It.second.Context, SSL_VERIFY_NONE, nullptr);

				if (It.second.Blob.Certificate.empty() || It.second.Blob.PrivateKey.empty())
					return Core::SystemException("invalid server transport layer private key or certificate error: " + It.first, std::make_error_condition(std::errc::invalid_argument));

				BIO* CertificateBio = BIO_new_mem_buf((void*)It.second.Blob.Certificate.c_str(), (int)It.second.Blob.Certificate.size());
				if (!CertificateBio)
					return Core::SystemException(Core::Stringify::Text("server transport layer certificate %s memory error: %s", It.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::not_enough_memory));

				X509* Certificate = PEM_read_bio_X509(CertificateBio, nullptr, 0, nullptr);
				BIO_free(CertificateBio);
				if (!Certificate)
					return Core::SystemException(Core::Stringify::Text("server transport layer certificate %s parse error: %s", It.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::invalid_argument));

				if (SSL_CTX_use_certificate(It.second.Context, Certificate) != 1)
					return Core::SystemException(Core::Stringify::Text("server transport layer certificate %s import error: %s", It.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::bad_message));
	
				BIO* PrivateKeyBio = BIO_new_mem_buf((void*)It.second.Blob.PrivateKey.c_str(), (int)It.second.Blob.PrivateKey.size());
				if (!PrivateKeyBio)
					return Core::SystemException(Core::Stringify::Text("server transport layer private key %s memory error: %s", It.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::not_enough_memory));

				EVP_PKEY* PrivateKey = PEM_read_bio_PrivateKey(PrivateKeyBio, nullptr, 0, nullptr);
				BIO_free(PrivateKeyBio);
				if (!PrivateKey)
					return Core::SystemException(Core::Stringify::Text("server transport layer private key %s parse error: %s", It.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::invalid_argument));

				if (SSL_CTX_use_PrivateKey(It.second.Context, PrivateKey) != 1)
					return Core::SystemException(Core::Stringify::Text("server transport layer private key %s import error: %s", It.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::bad_message));

				if (!SSL_CTX_check_private_key(It.second.Context))
					return Core::SystemException(Core::Stringify::Text("server transport layer private key %s verify error: %s", It.first.c_str(), ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::bad_message));
			}
#endif
			return Core::Expectation::Met;
		}
		Core::ExpectsSystem<void> SocketServer::Unlisten(uint64_t TimeoutSeconds)
		{
			VI_MEASURE(Core::Timings::Hangup);
			if (!Router && State == ServerState::Idle)
				return Core::Expectation::Met;

			State = ServerState::Stopping;
			int64_t Timeout = time(nullptr);

			do
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				if (TimeoutSeconds > 0 && time(nullptr) - Timeout > (int64_t)TimeoutSeconds)
				{
					VI_DEBUG("[stall] server has stalled connections: %i (these connections will be ignored)", (int)Active.size());
					for (auto* Next : Active)
						OnRequestStall(Next);
					break;
				}

				auto Copy = Active;
				Unique.Negate();

				for (auto* Base : Copy)
				{
					Base->Info.Close = true;
					Base->Info.KeepAlive = 0;
					if (Base->Stream->IsPending())
						Base->Stream->ClearEvents(true);
				}

				FreeAll();
				VI_MEASURE_LOOP();
			} while (Core::Schedule::Get()->IsActive() && (!Inactive.empty() || !Active.empty()));

			auto UnlistenStatus = OnUnlisten();
			if (!UnlistenStatus)
				return UnlistenStatus;

			for (auto It : Listeners)
			{
				if (It->Base != nullptr)
					It->Base->Close();
			}

			if (!Core::Schedule::Get()->IsActive())
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				if (!Active.empty())
				{
					Inactive.insert(Active.begin(), Active.end());
					Active.clear();
				}
			}
			FreeAll();
			for (auto It : Listeners)
				VI_RELEASE(It);

			VI_CLEAR(Router);
			Listeners.clear();
			State = ServerState::Idle;
			Router = nullptr;
			return Core::Expectation::Met;
		}
		Core::ExpectsSystem<void> SocketServer::Listen()
		{
			if (Listeners.empty() || State != ServerState::Idle)
				return Core::Expectation::Met;

			State = ServerState::Working;
			auto ListenStatus = OnListen();
			if (!ListenStatus)
				return ListenStatus;

			for (auto&& Source : Listeners)
			{
				Source->Base->AcceptAsync(true, [this, Source](socket_t Fd, char* RemoteAddr)
				{
					if (State != ServerState::Working)
						return false;

					Core::String IpAddress = RemoteAddr;
					Core::Schedule::Get()->SetTask([this, Source, Fd, IpAddress = std::move(IpAddress)]() mutable
					{
						Accept(Source, Fd, IpAddress);
					});
					return State == ServerState::Working;
				});
			}

			return Core::Expectation::Met;
		}
		bool SocketServer::FreeAll()
		{
			VI_MEASURE(Core::Timings::Pass);
			if (Inactive.empty())
				return false;

			Core::UMutex<std::mutex> Unique(Exclusive);
			for (auto* Item : Inactive)
				VI_RELEASE(Item);
			Inactive.clear();
			return true;
		}
		Core::ExpectsIO<void> SocketServer::Refuse(SocketConnection* Base)
		{
			return Base->Stream->CloseAsync(false, [this, Base](const Core::Option<std::error_condition>&) { Push(Base); });
		}
		Core::ExpectsIO<void> SocketServer::Accept(SocketListener* Host, socket_t Fd, const Core::String& Address)
		{
			auto* Base = Pop(Host);
			if (!Base)
				return std::make_error_condition(std::errc::not_enough_memory);

			strncpy(Base->RemoteAddress, Address.c_str(), std::min(Address.size(), sizeof(Base->RemoteAddress)));
			Base->Stream->Timeout = Router->SocketTimeout;
			Base->Stream->MigrateTo(Fd, false);
			Base->Stream->SetCloseOnExec();
			Base->Stream->SetNoDelay(Router->EnableNoDelay);
			Base->Stream->SetKeepAlive(true);
			Base->Stream->SetBlocking(false);

			if (Router->GracefulTimeWait >= 0)
				Base->Stream->SetTimeWait((int)Router->GracefulTimeWait);

			if (Router->MaxConnections > 0 && Active.size() >= Router->MaxConnections)
			{
				Refuse(Base);
				return std::make_error_condition(std::errc::too_many_files_open);
			}

			if (!Host->Hostname.Secure)
			{
				OnRequestOpen(Base);
				return Core::Expectation::Met;
			}

			if (!HandshakeThenOpen(Base, Host))
			{
				Refuse(Base);
				return std::make_error_condition(std::errc::protocol_error);
			}

			return Core::Expectation::Met;
		}
		Core::ExpectsIO<void> SocketServer::HandshakeThenOpen(SocketConnection* Base, SocketListener* Host)
		{
			VI_ASSERT(Base != nullptr, "socket should be set");
			VI_ASSERT(Base->Stream != nullptr, "socket should be set");
			VI_ASSERT(Host != nullptr, "host should be set");

			if (Router->Certificates.empty())
				return std::make_error_condition(std::errc::not_supported);

			auto Source = Router->Certificates.find(Host->Name);
			if (Source == Router->Certificates.end())
				return std::make_error_condition(std::errc::not_supported);

			ssl_ctx_st* Context = Source->second.Context;
			if (!Context)
				return std::make_error_condition(std::errc::host_unreachable);

			auto Status = Base->Stream->Secure(Context, nullptr);
			if (!Status)
				return Status;
#ifdef VI_OPENSSL
			if (SSL_set_fd(Base->Stream->GetDevice(), (int)Base->Stream->GetFd()) != 1)
				return std::make_error_condition(std::errc::connection_aborted);

			TryHandshakeThenBegin(Base);
			return Core::Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> SocketServer::TryHandshakeThenBegin(SocketConnection* Base)
		{
#ifdef VI_OPENSSL
			int ErrorCode = SSL_accept(Base->Stream->GetDevice());
			if (ErrorCode != -1)
			{
				OnRequestOpen(Base);
				return Core::Expectation::Met;
			}

			switch (SSL_get_error(Base->Stream->GetDevice(), ErrorCode))
			{
				case SSL_ERROR_WANT_ACCEPT:
				case SSL_ERROR_WANT_READ:
				{
					Multiplexer::Get()->WhenReadable(Base->Stream, [this, Base](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
							TryHandshakeThenBegin(Base);
						else
							Refuse(Base);
					});
					return std::make_error_condition(std::errc::operation_would_block);
				}
				case SSL_ERROR_WANT_WRITE:
				{
					Multiplexer::Get()->WhenWriteable(Base->Stream, [this, Base](SocketPoll Event)
					{
						if (Packet::IsDone(Event))
							TryHandshakeThenBegin(Base);
						else
							Refuse(Base);
					});
					return std::make_error_condition(std::errc::operation_would_block);
				}
				default:
				{
					Utils::DisplayTransportLog();
					return Refuse(Base);
				}
			}
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		Core::ExpectsIO<void> SocketServer::Continue(SocketConnection* Base)
		{
			VI_ASSERT(Base != nullptr, "socket should be set");
			if (Base->Info.KeepAlive < -1)
				return std::make_error_condition(std::errc::connection_aborted);

			if (!OnRequestCleanup(Base))
				return std::make_error_condition(std::errc::operation_in_progress);

			return Finalize(Base);
		}
		Core::ExpectsIO<void> SocketServer::Finalize(SocketConnection* Base)
		{
			VI_ASSERT(Base != nullptr, "socket should be set");
			if (Router->KeepAliveMaxCount >= 0)
				Base->Info.KeepAlive--;

			if (Base->Info.KeepAlive >= -1)
				Base->Info.Finish = Utils::Clock();

			OnRequestClose(Base);
			Base->Stream->Timeout = Router->SocketTimeout;
			Base->Stream->Income = 0;
			Base->Stream->Outcome = 0;

			if (Base->Info.Close || Base->Info.KeepAlive <= -1)
			{
				Base->Info.KeepAlive = -2;
				return Base->Stream->CloseAsync(true, [this, Base](const Core::Option<std::error_condition>&) { Push(Base); });
			}
			else if (Base->Stream->IsValid())
				OnRequestOpen(Base);
			else
				Push(Base);

			return Core::Expectation::Met;
		}
		Core::ExpectsSystem<void> SocketServer::OnConfigure(SocketRouter*)
		{
			return Core::Expectation::Met;
		}
		Core::ExpectsSystem<void> SocketServer::OnListen()
		{
			return Core::Expectation::Met;
		}
		Core::ExpectsSystem<void> SocketServer::OnUnlisten()
		{
			return Core::Expectation::Met;
		}
		void SocketServer::OnRequestStall(SocketConnection* Base)
		{
		}
		void SocketServer::OnRequestOpen(SocketConnection*)
		{
		}
		void SocketServer::OnRequestClose(SocketConnection*)
		{
		}
		bool SocketServer::OnRequestCleanup(SocketConnection*)
		{
			return true;
		}
		void SocketServer::Push(SocketConnection* Base)
		{
			VI_MEASURE(Core::Timings::Pass);
			Core::UMutex<std::mutex> Unique(Exclusive);
			auto It = Active.find(Base);
			if (It != Active.end())
				Active.erase(It);

			Base->Reset(true);
			if (Inactive.size() < Backlog)
				Inactive.insert(Base);
			else
				VI_RELEASE(Base);
		}
		SocketConnection* SocketServer::Pop(SocketListener* Host)
		{
			VI_ASSERT(Host != nullptr, "host should be set");
			SocketConnection* Result = nullptr;
			if (!Inactive.empty())
			{
				Core::UMutex<std::mutex> Unique(Exclusive);
				if (!Inactive.empty())
				{
					auto It = Inactive.begin();
					Result = *It;
					Inactive.erase(It);
					Active.insert(Result);
				}
			}

			if (!Result)
			{
				Result = OnAllocate(Host);
				if (!Result)
					return nullptr;

				Result->Stream = new Socket();
				Result->Stream->UserData = Result;

				Core::UMutex<std::mutex> Unique(Exclusive);
				Active.insert(Result);
			}

			VI_RELEASE(Result->Host);
			Result->Info.KeepAlive = (Router->KeepAliveMaxCount > 0 ? Router->KeepAliveMaxCount - 1 : 0);
			Result->Host = Host;
			Result->Host->AddRef();

			return Result;
		}
		SocketConnection* SocketServer::OnAllocate(SocketListener*)
		{
			return new SocketConnection();
		}
		SocketRouter* SocketServer::OnAllocateRouter()
		{
			return new SocketRouter();
		}
		const Core::UnorderedSet<SocketConnection*>& SocketServer::GetActiveClients()
		{
			return Active;
		}
		const Core::UnorderedSet<SocketConnection*>& SocketServer::GetPooledClients()
		{
			return Inactive;
		}
		const Core::Vector<SocketListener*>& SocketServer::GetListeners()
		{
			return Listeners;
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

		SocketClient::SocketClient(int64_t RequestTimeout) noexcept
		{
			Timeout.Idle = RequestTimeout;
		}
		SocketClient::~SocketClient() noexcept
		{
			VI_CLEAR(Net.Stream);
#ifdef VI_OPENSSL
			if (Net.Context != nullptr)
			{
				TransportLayer::Get()->FreeClientContext(Net.Context);
				Net.Context = nullptr;
			}
#endif
			if (Config.IsAsync)
				Multiplexer::Get()->Deactivate();
		}
		Core::ExpectsSystem<void> SocketClient::OnResolveHost(RemoteHost* Address)
		{
			if (!Address)
				return Core::SystemException("resolve host address: unreachable", std::make_error_condition(std::errc::host_unreachable));

			return Core::Expectation::Met;
		}
		Core::ExpectsSystem<void> SocketClient::OnConnect()
		{
			Report(Core::Expectation::Met);
			return Core::Expectation::Met;
		}
		Core::ExpectsSystem<void> SocketClient::OnDisconnect()
		{
			Report(Core::Expectation::Met);
			return Core::Expectation::Met;
		}
		Core::ExpectsPromiseSystem<void> SocketClient::Connect(RemoteHost* Source, bool Async, uint32_t VerifyPeers)
		{
			VI_ASSERT(Source != nullptr && !Source->Hostname.empty(), "address should be set");
			if (!Async && Config.IsAsync)
				Multiplexer::Get()->Deactivate();

			bool IsReusing = TryReuseStream(Source, Async);
			Config.VerifyPeers = VerifyPeers;
			Config.IsAsync = Async;
			State.Hostname = *Source;
			if (IsReusing)
			{
				if (Config.IsAsync)
					Multiplexer::Get()->Activate();
				return Core::ExpectsPromiseSystem<void>(Core::Expectation::Met);
			}

			auto ResolveStatus = OnResolveHost(Source);
			if (!ResolveStatus)
			{
				Report(ResolveStatus.Error());
				return ResolveStatus;
			}

			Core::ExpectsPromiseSystem<void> Future;
			State.Done = [Future](SocketClient*, Core::ExpectsSystem<void>&& Status) mutable { Future.Set(std::move(Status)); };
			if (!Async)
			{
				TryConnect(DNS::Get()->FindAddressFromName(State.Hostname.Hostname, Core::ToString(State.Hostname.Port), DNSType::Connect, SocketProtocol::TCP, SocketType::Stream));
				return Future;
			}

			auto* Context = this;
			Multiplexer::Get()->Activate();
			Core::Cotask<Core::ExpectsSystem<SocketAddress*>>([Context]()
			{
				return DNS::Get()->FindAddressFromName(Context->State.Hostname.Hostname, Core::ToString(Context->State.Hostname.Port), DNSType::Connect, SocketProtocol::TCP, SocketType::Stream);
			}).When([Context](Core::ExpectsSystem<SocketAddress*>&& Host)
			{
				Context->TryConnect(std::move(Host));
			});
			return Future;
		}
		Core::ExpectsPromiseSystem<void> SocketClient::Disconnect()
		{
			if (!HasStream())
				return Core::ExpectsPromiseSystem<void>(Core::SystemException("socket: not connected", std::make_error_condition(std::errc::bad_file_descriptor)));

			Uplinks* Cache = (Uplinks::HasInstance() ? Uplinks::Get() : nullptr);
			if (Timeout.Cache && Cache != nullptr && Cache->PushConnection(&State.Hostname, Net.Stream))
			{
				Net.Stream = nullptr;
				Timeout.Cache = false;
				return Core::ExpectsPromiseSystem<void>(Core::Expectation::Met);
			}

			Core::ExpectsPromiseSystem<void> Result;
			State.Done = [this, Result](SocketClient*, Core::ExpectsSystem<void>&& Status) mutable { Result.Set(std::move(Status)); };
			Timeout.Cache = false;
			OnDisconnect();
			return Result;
		}
		void SocketClient::Handshake(std::function<void(Core::ExpectsSystem<void>&&)>&& Callback)
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			if (!HasStream())
				return Callback(Core::SystemException("ssl handshake error: bad fd", std::make_error_condition(std::errc::bad_file_descriptor)));
#ifdef VI_OPENSSL
			if (Net.Stream->GetDevice() || !Net.Context)
				return Callback(Core::SystemException("ssl handshake error: invalid operation", std::make_error_condition(std::errc::bad_file_descriptor)));

			auto Status = Net.Stream->Secure(Net.Context, State.Hostname.Hostname.c_str());
			if (!Status)
				return Callback(Core::SystemException("ssl handshake establish error", std::move(Status.Error())));

			if (SSL_set_fd(Net.Stream->GetDevice(), (int)Net.Stream->GetFd()) != 1)
				return Callback(Core::SystemException("ssl handshake set error", std::make_error_condition(std::errc::bad_file_descriptor)));

			if (Config.VerifyPeers > 0)
				SSL_set_verify(Net.Stream->GetDevice(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
			else
				SSL_set_verify(Net.Stream->GetDevice(), SSL_VERIFY_NONE, nullptr);

			TryHandshake(std::move(Callback));
#else
			Callback(Core::SystemException("ssl handshake error: unsupported", std::make_error_condition(std::errc::not_supported)));
#endif
		}
		void SocketClient::TryHandshake(std::function<void(Core::ExpectsSystem<void>&&)>&& Callback)
		{
#ifdef VI_OPENSSL
			int ErrorCode = SSL_connect(Net.Stream->GetDevice());
			if (ErrorCode != -1)
				return Callback(Core::Expectation::Met);

			switch (SSL_get_error(Net.Stream->GetDevice(), ErrorCode))
			{
				case SSL_ERROR_WANT_READ:
				{
					Multiplexer::Get()->WhenReadable(Net.Stream, [this, Callback = std::move(Callback)](SocketPoll Event) mutable
					{
						if (!Packet::IsDone(Event))
							Callback(Core::SystemException(Core::Stringify::Text("ssl connect read error: %s", ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::timed_out)));
						else
							TryHandshake(std::move(Callback));
					});
					break;
				}
				case SSL_ERROR_WANT_CONNECT:
				case SSL_ERROR_WANT_WRITE:
				{
					Multiplexer::Get()->WhenWriteable(Net.Stream, [this, Callback = std::move(Callback)](SocketPoll Event) mutable
					{
						if (!Packet::IsDone(Event))
							Callback(Core::SystemException(Core::Stringify::Text("ssl connect write error: %s", ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::timed_out)));
						else
							TryHandshake(std::move(Callback));
					});
					break;
				}
				default:
				{
					Utils::DisplayTransportLog();
					return Callback(Core::SystemException(Core::Stringify::Text("ssl connection aborted error: %s", ERR_error_string(ERR_get_error(), nullptr)), std::make_error_condition(std::errc::connection_aborted)));
				}
			}
#else
			Callback(Core::SystemException("ssl connect error: unsupported", std::make_error_condition(std::errc::not_supported)));
#endif
		}
		void SocketClient::TryConnect(Core::ExpectsSystem<SocketAddress*>&& Host)
		{
			if (!Host)
				return Report(std::move(Host.Error()));

			auto Status = Net.Stream->Open(*Host);
			if (!Status)
				return Report(Core::SystemException(Core::Stringify::Text("connect to %s:%i error", State.Hostname.Hostname.c_str(), (int)State.Hostname.Port), std::move(Status.Error())));

			auto* Context = this;
			Net.Stream->ConnectAsync(*Host, [Context](const Core::Option<std::error_condition>& ErrorCode)
			{
				Context->DispatchConnection(ErrorCode);
			});
		}
		void SocketClient::DispatchConnection(const Core::Option<std::error_condition>& ErrorCode)
		{
			if (ErrorCode)
				return Report(Core::SystemException(Core::Stringify::Text("connect to %s:%i error", State.Hostname.Hostname.c_str(), (int)State.Hostname.Port), std::error_condition(*ErrorCode)));
#ifdef VI_OPENSSL
			if (!State.Hostname.Secure)
				return DispatchSimpleHandshake();

			if (!Net.Context || (Config.VerifyPeers > 0 && SSL_CTX_get_verify_depth(Net.Context) <= 0))
			{
				auto* Transporter = TransportLayer::Get();
				if (Net.Context != nullptr)
				{
					Transporter->FreeClientContext(Net.Context);
					Net.Context = nullptr;
				}

				auto NewContext = Transporter->CreateClientContext(Config.VerifyPeers);
				if (!NewContext)
					return Report(Core::SystemException(Core::Stringify::Text("ssl connect to %s:%i error: initialization failed", State.Hostname.Hostname.c_str(), (int)State.Hostname.Port), std::move(NewContext.Error())));
				else
					Net.Context = *NewContext;
			}

			if (!Config.IsAutoEncrypted)
				return DispatchSimpleHandshake();

			auto* Context = this;
			Handshake([Context](Core::ExpectsSystem<void>&& ErrorCode) mutable
			{
				Context->DispatchSecureHandshake(std::move(ErrorCode));
			});
#else
			DispatchSimpleHandshake();
#endif
		}
		void SocketClient::DispatchSecureHandshake(Core::ExpectsSystem<void>&& Status)
		{
			if (!Status)
				Report(std::move(Status));
			else
				DispatchSimpleHandshake();
		}
		void SocketClient::DispatchSimpleHandshake()
		{
			OnConnect();
		}
		bool SocketClient::TryReuseStream(RemoteHost* NewAddress, bool Async)
		{
			Uplinks* Cache = (Uplinks::HasInstance() ? Uplinks::Get() : nullptr);
			Socket* ReusingStream = Cache ? Cache->PopConnection(NewAddress) : nullptr;
			if (ReusingStream != nullptr)
			{
				VI_RELEASE(Net.Stream);
				Net.Stream = ReusingStream;
			}
			else if (!Net.Stream)
				Net.Stream = new Socket();

			Net.Stream->UserData = this;
			Net.Stream->Timeout = Timeout.Idle;
			Net.Stream->SetBlocking(!Async);
			Net.Stream->SetCloseOnExec();
			return ReusingStream != nullptr;
		}
		void SocketClient::EnableReusability()
		{
			Timeout.Cache = true;
		}
		void SocketClient::DisableReusability()
		{
			Timeout.Cache = false;
		}
		void SocketClient::Report(Core::ExpectsSystem<void>&& Status)
		{
			if (!Status && HasStream())
			{
				Net.Stream->CloseAsync(true, [this, Status = std::move(Status)](const Core::Option<std::error_condition>&) mutable
				{
					SocketClientCallback Callback(std::move(State.Done));
					if (Callback)
						Callback(this, std::move(Status));
				});
			}
			else
			{
				SocketClientCallback Callback(std::move(State.Done));
				if (Callback)
					Callback(this, std::move(Status));
			}
		}
		bool SocketClient::HasStream() const
		{
			return Net.Stream != nullptr && Net.Stream->IsValid();
		}
		Socket* SocketClient::GetStream()
		{
			return Net.Stream;
		}
	}
}
#pragma warning(pop)