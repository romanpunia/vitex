#ifndef TH_NETWORK_H
#define TH_NETWORK_H
#include "compute.h"
#include <atomic>
#ifdef TH_APPLE
struct kevent;
#else
struct epoll_event;
#endif

struct ssl_ctx_st;
struct ssl_st;
struct addrinfo;
struct sockaddr;
struct pollfd;

namespace Tomahawk
{
	namespace Network
	{
		struct SocketConnection;

		struct EpollHandle;

		class Driver;

		enum class Secure
		{
			Any,
			SSL_V2,
			SSL_V3,
			TLS_V1,
			TLS_V1_1,
		};

		enum class ServerState
		{
			Working,
			Stopping,
			Idle
		};

		enum class SocketPoll
		{
			Next = 0,
			Reset = 1,
			Timeout = 2,
			Cancel = 3,
			Finish = 4
		};

		enum class SocketProtocol
		{
			IP,
			ICMP,
			TCP,
			UDP,
			RAW
		};

		enum class SocketType
		{
			Stream,
			Datagram,
			Raw,
			Reliably_Delivered_Message,
			Sequence_Packet_Stream
		};

		enum class DNSType
		{
			Connect,
			Listen
		};

		typedef std::function<void(class SocketClient*, int)> SocketClientCallback;
		typedef std::function<void(SocketPoll)> PollEventCallback;
		typedef std::function<void(SocketPoll)> SocketWrittenCallback;
		typedef std::function<void()> SocketClosedCallback;
		typedef std::function<bool(SocketPoll, const char*, size_t)> SocketReadCallback;
		typedef std::function<bool(socket_t)> SocketAcceptedCallback;

		struct TH_OUT Address
		{
			addrinfo* Addresses = nullptr;
			addrinfo* Good = nullptr;
		};

		struct TH_OUT Socket
		{
			friend EpollHandle;
			friend Driver;
			
		private:
			struct
			{
				PollEventCallback ReadCallback = nullptr;
				PollEventCallback WriteCallback = nullptr;
				std::chrono::microseconds ExpiresAt;
				bool Readable = false;
				bool Writeable = false;
			} Events;

		private:
			ssl_st* Device;
			socket_t Fd;

		public:
			uint64_t Timeout;
			int64_t Income;
			int64_t Outcome;
			void* UserData;

		public:
			Socket();
			Socket(socket_t FromFd);
			int Accept(Socket* Connection, Address* Output);
			int Accept(socket_t* NewFd);
			int AcceptAsync(SocketAcceptedCallback&& Callback);
			int Close(bool Gracefully = true);
			int CloseAsync(bool Gracefully, SocketClosedCallback&& Callback);
			int Write(const char* Buffer, int Size);
			int WriteAsync(const char* Buffer, size_t Size, SocketWrittenCallback&& Callback, char* TempBuffer = nullptr);
			int Read(char* Buffer, int Size);
			int Read(char* Buffer, int Size, const SocketReadCallback& Callback);
			int ReadAsync(size_t Size, SocketReadCallback&& Callback);
			int ReadUntil(const char* Match, SocketReadCallback&& Callback);
			int ReadUntilAsync(const char* Match, SocketReadCallback&& Callback, char* TempBuffer = nullptr, size_t TempIndex = 0);
			int Open(const std::string& Host, const std::string& Port, SocketProtocol Proto, SocketType Type, DNSType DNS, Address** Result);
			int Open(const std::string& Host, const std::string& Port, DNSType DNS, Address** Result);
			int Open(addrinfo* Good);
			int Secure(ssl_ctx_st* Context, const char* Hostname);
			int Bind(Address* Address);
			int Connect(Address* Address, uint64_t Timeout);
			int Listen(int Backlog);
			int ClearEvents(bool Gracefully);
			int SetFd(socket_t Fd, bool Gracefully = true);
			int SetCloseOnExec();
			int SetTimeWait(int Timeout);
			int SetSocket(int Option, void* Value, int Size);
			int SetAny(int Level, int Option, void* Value, int Size);
			int SetAnyFlag(int Level, int Option, int Value);
			int SetSocketFlag(int Option, int Value);
			int SetBlocking(bool Enabled);
			int SetNodelay(bool Enabled);
			int SetKeepAlive(bool Enabled);
			int SetTimeout(int Timeout);
			int GetError(int Result);
			int GetAnyFlag(int Level, int Option, int* Value);
			int GetAny(int Level, int Option, void* Value, int* Size);
			int GetSocket(int Option, void* Value, int* Size);
			int GetSocketFlag(int Option, int* Value);
			int GetPort();
			socket_t GetFd();
			ssl_st* GetDevice();
			bool IsPendingForRead();
			bool IsPendingForWrite();
			bool IsPending();
			bool IsValid();
			std::string GetRemoteAddress();

		private:
			int TryCloseAsync(SocketClosedCallback&& Callback, bool KeepTrying);

		public:
			template <typename T>
			T* Context()
			{
				return (T*)UserData;
			}

		public:
			static std::string GetLocalAddress();
			static std::string GetAddress(addrinfo* Info);
			static std::string GetAddress(sockaddr* Info);
			static int GetAddressFamily(const char* Address);
		};

		struct TH_OUT Host
		{
			std::string Hostname;
			int Port = 0;
			bool Secure = false;
		};

		struct TH_OUT Listener
		{
			Address* Source;
			std::string Name;
			Host* Hostname = nullptr;
			Socket* Base = nullptr;
		};

		struct TH_OUT SourceURL
		{
		public:
			std::unordered_map<std::string, std::string> Query;
			std::string URL;
			std::string Protocol;
			std::string Login;
			std::string Password;
			std::string Host;
			std::string Path;
			std::string Filename;
			std::string Extension;
			int Port;

		public:
			SourceURL(const std::string& Src) noexcept;
			SourceURL(const SourceURL& Other) noexcept;
			SourceURL(SourceURL&& Other) noexcept;
			SourceURL& operator= (const SourceURL& Other) noexcept;
			SourceURL& operator= (SourceURL&& Other) noexcept;

		private:
			void MakePath();
		};

		struct TH_OUT Certificate
		{
			std::string Subject;
			std::string Issuer;
			std::string Serial;
			std::string Finger;
		};

		struct TH_OUT DataFrame
		{
			std::string Message;
			std::mutex Sync;
			int64_t Start = 0;
			int64_t Finish = 0;
			int64_t Timeout = 0;
			int64_t KeepAlive = 1;
			bool Close = false;
		};

		struct TH_OUT SocketCertificate
		{
			ssl_ctx_st* Context = nullptr;
			std::string Key;
			std::string Chain;
			std::string Ciphers = "ALL";
			Secure Protocol = Secure::Any;
			bool VerifyPeers = true;
			uint64_t Depth = 9;
		};

		struct TH_OUT SocketConnection
		{
			Socket* Stream = nullptr;
			Listener* Host = nullptr;
			DataFrame Info;

			virtual ~SocketConnection();
			virtual void Reset(bool Fully);
			virtual bool Finish();
			virtual bool Finish(int);
			virtual bool Error(int, const char* ErrorMessage, ...);
			virtual bool Certify(Certificate* Output);
			virtual bool Break();
		};

		struct TH_OUT SocketRouter
		{
			std::unordered_map<std::string, SocketCertificate> Certificates;
			std::unordered_map<std::string, Host> Listeners;
			uint64_t PayloadMaxLength = 12582912;
			uint64_t BacklogQueue = 20;
			uint64_t SocketTimeout = 10000;
			uint64_t MaxConnections = 0;
			int64_t KeepAliveMaxCount = 50;
			int64_t GracefulTimeWait = -1;
			bool EnableNoDelay = false;

			Host& Listen(const std::string& Hostname, int Port, bool Secure = false);
			Host& Listen(const std::string& Pattern, const std::string& Hostname, int Port, bool Secure = false);
			virtual ~SocketRouter();
		};

		struct TH_OUT EpollFd
		{
			Socket* Base;
			bool Readable;
			bool Writeable;
			bool Closed;
		};

		struct TH_OUT EpollHandle
		{
#ifdef TH_APPLE
			kevent* Array;
#else
			epoll_event* Array;
#endif
			epoll_handle Handle;
			int ArraySize;

			EpollHandle(int ArraySize);
			~EpollHandle();
			bool Add(Socket* Fd, bool Readable, bool Writeable);
			bool Update(Socket* Fd, bool Readable, bool Writeable);
			bool Remove(Socket* Fd, bool Readable, bool Writeable);
			int Wait(EpollFd* Data, size_t DataSize, uint64_t Timeout);
		};

		class TH_OUT Packet
		{
		public:
			static bool IsData(SocketPoll Event)
			{
				return Event == SocketPoll::Next;
			}
			static bool IsSkip(SocketPoll Event)
			{
				return Event == SocketPoll::Cancel;
			}
			static bool IsDone(SocketPoll Event)
			{
				return Event == SocketPoll::Finish;
			}
			static bool IsTimeout(SocketPoll Event)
			{
				return Event == SocketPoll::Timeout;
			}
			static bool IsError(SocketPoll Event)
			{
				return Event == SocketPoll::Reset || Event == SocketPoll::Timeout;
			}
			static bool IsErrorOrSkip(SocketPoll Event)
			{
				return IsError(Event) || Event == SocketPoll::Cancel;
			}
			static bool WillContinue(SocketPoll Event)
			{
				return !IsData(Event);
			}
		};

		class TH_OUT DNS
		{
		private:
			static std::unordered_map<std::string, std::pair<int64_t, Address*>> Names;
			static std::mutex Exclusive;

		public:
			static void Release();
			static std::string FindNameFromAddress(const std::string& IpAddress, uint32_t Port);
			static Address* FindAddressFromName(const std::string& Host, const std::string& Service, SocketProtocol Proto, SocketType Type, DNSType DNS);
		};

		class TH_OUT Utils
		{
		public:
			static int Poll(pollfd* Fd, int FdCount, int Timeout);
			static int64_t Clock();
		};

		class TH_OUT Driver
		{
		private:
			static std::map<std::chrono::microseconds, Socket*>* Timeouts;
			static std::vector<EpollFd>* Fds;
			static std::mutex Exclusive;
			static EpollHandle* Handle;

		public:
			static void Create(size_t MaxEvents = 256);
			static void Release();
			static int Dispatch(uint64_t Timeout);
			static bool WhenReadable(Socket* Value, PollEventCallback&& WhenReady);
			static bool WhenWriteable(Socket* Value, PollEventCallback&& WhenReady);
			static bool CancelEvents(Socket* Value, SocketPoll Event = SocketPoll::Cancel, bool Safely = true);
			static bool ClearEvents(Socket* Value);
			static bool IsAwaitingEvents(Socket* Value);
			static bool IsAwaitingReadable(Socket* Value);
			static bool IsAwaitingWriteable(Socket* Value);
			static bool IsActive();

		private:
			static bool WhenEvents(Socket* Value, bool Readable, bool Writeable, PollEventCallback&& WhenReadable, PollEventCallback&& WhenWriteable);
			static bool DispatchEvents(EpollFd& Fd, const std::chrono::microseconds& Time);
			static void AddTimeout(Socket* Value, const std::chrono::microseconds& Time);
			static void UpdateTimeout(Socket* Value, const std::chrono::microseconds& Time);
			static void RemoveTimeout(Socket* Value);
		};

		class TH_OUT SocketServer : public Core::Object
		{
			friend SocketConnection;

		protected:
			std::unordered_set<SocketConnection*> Active;
			std::unordered_set<SocketConnection*> Inactive;
			std::vector<Listener*> Listeners;
			SocketRouter* Router = nullptr;
			ServerState State = ServerState::Idle;
			std::mutex Sync;
			size_t Backlog;
			
		public:
			SocketServer();
			virtual ~SocketServer() override;
			void SetRouter(SocketRouter* New);
			void SetBacklog(size_t Value);
			void Lock();
			void Unlock();
			bool Configure(SocketRouter* New);
			bool Unlisten();
			bool Listen();
			size_t GetBacklog();
			ServerState GetState();
			SocketRouter* GetRouter();
			std::unordered_set<SocketConnection*>* GetClients();

		protected:
			virtual bool OnConfigure(SocketRouter* New);
			virtual bool OnRequestEnded(SocketConnection* Base, bool Check);
			virtual bool OnRequestBegin(SocketConnection* Base);
			virtual bool OnDeallocate(SocketConnection* Base);
			virtual bool OnDeallocateRouter(SocketRouter* Base);
			virtual bool OnStall(std::unordered_set<SocketConnection*>& Base);
			virtual bool OnListen();
			virtual bool OnUnlisten();
			virtual bool OnProtect(Socket* Fd, Listener* Host, ssl_ctx_st** Context);
			virtual SocketConnection* OnAllocate(Listener* Host);
			virtual SocketRouter* OnAllocateRouter();

		protected:
			bool FreeAll();
			bool Accept(Listener* Host, socket_t Fd);
			bool Protect(Socket* Fd, Listener* Host);
			bool Manage(SocketConnection* Base);
			void Push(SocketConnection* Base);
			SocketConnection* Pop(Listener* Host);
		};

		class TH_OUT SocketClient : public Core::Object
		{
		protected:
			SocketClientCallback Done;
			ssl_ctx_st* Context;
			std::string Action;
			Socket Stream;
			Host Hostname;
			int64_t Timeout;
			bool AutoCertify;

		public:
			SocketClient(int64_t RequestTimeout);
			virtual ~SocketClient() override;
			Core::Async<int> Connect(Host* Address, bool Async);
			Core::Async<int> Close();
			Socket* GetStream();

		protected:
			virtual bool OnResolveHost(Host* Address);
			virtual bool OnConnect();
			virtual bool OnClose();

		protected:
			bool Certify();
			bool Stage(const std::string& Name);
			bool Error(const char* Message, ...);
			bool Success(int Code);
		};
	}
}
#endif
