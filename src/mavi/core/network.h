#ifndef VI_NETWORK_H
#define VI_NETWORK_H
#include "compute.h"
#include <atomic>
#ifdef VI_APPLE
struct kevent;
#else
struct epoll_event;
#endif
struct ssl_ctx_st;
struct ssl_st;
struct addrinfo;
struct sockaddr;
struct pollfd;

namespace Mavi
{
	namespace Network
	{
		struct EpollHandle;

		class Multiplexer;

		class Socket;

		class SocketAddress;

		class SocketConnection;

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
			Finish = 4,
            FinishSync = 5
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
		typedef std::function<void(SocketAddress*)> SocketOpenedCallback;
		typedef std::function<void(int)> SocketConnectedCallback;
		typedef std::function<void()> SocketClosedCallback;
		typedef std::function<bool(SocketPoll, const char*, size_t)> SocketReadCallback;
		typedef std::function<bool(socket_t, char*)> SocketAcceptedCallback;

		struct VI_OUT RemoteHost
		{
			Core::String Hostname;
			int Port = 0;
			bool Secure = false;
		};

		struct VI_OUT Location
		{
		public:
			Core::UnorderedMap<Core::String, Core::String> Query;
			Core::String URL;
			Core::String Protocol;
			Core::String Username;
			Core::String Password;
			Core::String Hostname;
			Core::String Fragment;
			Core::String Path;
			int Port;

		public:
			Location(const Core::String& Src) noexcept;
			Location(const Location& Other) noexcept;
			Location(Location&& Other) noexcept;
			Location& operator= (const Location& Other) noexcept;
			Location& operator= (Location&& Other) noexcept;
		};

		struct VI_OUT Certificate
		{
			Core::String Subject;
			Core::String Issuer;
			Core::String Serial;
			Core::String Finger;
		};

		struct VI_OUT DataFrame
		{
			Core::String Message;
			std::mutex Sync;
			int64_t Start = 0;
			int64_t Finish = 0;
			int64_t Timeout = 0;
			int64_t KeepAlive = 1;
			bool Close = false;

			DataFrame() = default;
			DataFrame& operator= (const DataFrame& Other);
		};

		struct VI_OUT SocketCertificate
		{
			ssl_ctx_st* Context = nullptr;
			Core::String Key;
			Core::String Chain;
			Core::String Ciphers = "ALL";
			Secure Protocol = Secure::Any;
			bool VerifyPeers = true;
			size_t Depth = 9;
		};

		struct VI_OUT EpollFd
		{
			Socket* Base;
			bool Readable;
			bool Writeable;
			bool Closed;
		};

		struct VI_OUT_TS EpollHandle
		{
#ifdef VI_APPLE
			kevent* Array;
#else
			epoll_event* Array;
#endif
			epoll_handle Handle;
			size_t ArraySize;

			EpollHandle(size_t NewArraySize) noexcept;
			~EpollHandle() noexcept;
			bool Add(Socket* Fd, bool Readable, bool Writeable);
			bool Update(Socket* Fd, bool Readable, bool Writeable);
			bool Remove(Socket* Fd, bool Readable, bool Writeable);
			int Wait(EpollFd* Data, size_t DataSize, uint64_t Timeout);
		};

		class VI_OUT_TS Packet
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
				return Event == SocketPoll::Finish || Event == SocketPoll::FinishSync;
			}
            static bool IsDoneSync(SocketPoll Event)
            {
                return Event == SocketPoll::FinishSync;
            }
            static bool IsDoneAsync(SocketPoll Event)
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

		class VI_OUT_TS DNS
		{
		private:
			static Core::Mapping<Core::UnorderedMap<Core::String, std::pair<int64_t, SocketAddress*>>>* Names;
			static std::mutex Exclusive;

		public:
			static Core::String FindNameFromAddress(const Core::String& Host, const Core::String& Service);
			static SocketAddress* FindAddressFromName(const Core::String& Host, const Core::String& Service, DNSType DNS, SocketProtocol Proto, SocketType Type);
			static void Release();
		};

		class VI_OUT_TS Utils
		{
		public:
			enum PollEvent : uint32_t
			{
				InputNormal = (1 << 0),
				InputBand = (1 << 1),
				InputPriority = (1 << 2),
				Input = (1 << 3),
				OutputNormal = (1 << 4),
				OutputBand = (1 << 5),
				Output = (1 << 6),
				Error = (1 << 7),
				Hangup = (1 << 8)
			};

			struct PollFd
			{
				socket_t Fd = 0;
				uint32_t Events = 0;
				uint32_t Returns = 0;
			};

		public:
			static int Poll(pollfd* Fd, int FdCount, int Timeout);
			static int Poll(PollFd* Fd, int FdCount, int Timeout);
			static int64_t Clock();
		};

		class VI_OUT_TS Multiplexer
		{
		private:
			static Core::Mapping<Core::OrderedMap<std::chrono::microseconds, Socket*>>* Timeouts;
			static Core::Vector<EpollFd>* Fds;
			static std::atomic<size_t> Activations;
			static std::mutex Exclusive;
			static EpollHandle* Handle;
			static uint64_t DefaultTimeout;

		public:
			static void Create(uint64_t DispatchTimeout = 50, size_t MaxEvents = 256);
			static void Release();
			static void SetActive(bool Active);
			static int Dispatch(uint64_t Timeout);
			static bool WhenReadable(Socket* Value, PollEventCallback&& WhenReady);
			static bool WhenWriteable(Socket* Value, PollEventCallback&& WhenReady);
			static bool CancelEvents(Socket* Value, SocketPoll Event = SocketPoll::Cancel, bool Safely = true);
			static bool ClearEvents(Socket* Value);
			static bool IsAwaitingEvents(Socket* Value);
			static bool IsAwaitingReadable(Socket* Value);
			static bool IsAwaitingWriteable(Socket* Value);
			static bool IsListening();
			static bool IsActive();
			static size_t GetActivations();
			static Core::String GetLocalAddress();
			static Core::String GetAddress(addrinfo* Info);
			static Core::String GetAddress(sockaddr* Info);
			static int GetAddressFamily(const char* Address);

		private:
			static bool WhenEvents(Socket* Value, bool Readable, bool Writeable, PollEventCallback&& WhenReadable, PollEventCallback&& WhenWriteable);
			static bool DispatchEvents(EpollFd& Fd, const std::chrono::microseconds& Time);
			static void AddTimeout(Socket* Value, const std::chrono::microseconds& Time);
			static void UpdateTimeout(Socket* Value, const std::chrono::microseconds& Time);
			static void RemoveTimeout(Socket* Value);
			static void TryDispatch();
			static void TryEnqueue();
			static void TryListen();
			static void TryUnlisten();
		};

		class VI_OUT SocketAddress final : public Core::Reference<SocketAddress>
		{
			friend DNS;

		private:
			addrinfo* Names;
			addrinfo* Usable;

		public:
			SocketAddress(addrinfo* NewNames, addrinfo* NewUsables);
			~SocketAddress() noexcept;
			bool IsUsable() const;
			addrinfo* Get() const;
			addrinfo* GetAlternatives() const;
			Core::String GetAddress() const;
		};

		class VI_OUT Socket final : public Core::Reference<Socket>
		{
			friend EpollHandle;
			friend Multiplexer;

		private:
			struct
			{
				PollEventCallback ReadCallback = nullptr;
				PollEventCallback WriteCallback = nullptr;
				std::chrono::microseconds ExpiresAt = std::chrono::microseconds(0);
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
			Socket() noexcept;
			Socket(socket_t FromFd) noexcept;
			int Accept(Socket* OutConnection, char* OutAddress);
			int Accept(socket_t* OutFd, char* OutAddress);
			int AcceptAsync(bool WithAddress, SocketAcceptedCallback&& Callback);
			int Close(bool Gracefully = true);
			int CloseAsync(bool Gracefully, SocketClosedCallback&& Callback);
			int64_t SendFile(FILE* Stream, int64_t Offset, int64_t Size);
			int64_t SendFileAsync(FILE* Stream, int64_t Offset, int64_t Size, SocketWrittenCallback&& Callback, int TempBuffer = 0);
			int Write(const char* Buffer, int Size);
			int WriteAsync(const char* Buffer, size_t Size, SocketWrittenCallback&& Callback, char* TempBuffer = nullptr, size_t TempOffset = 0);
			int Read(char* Buffer, int Size);
			int Read(char* Buffer, int Size, const SocketReadCallback& Callback);
			int ReadAsync(size_t Size, SocketReadCallback&& Callback, int TempBuffer = 0);
			int ReadUntil(const char* Match, SocketReadCallback&& Callback);
			int ReadUntilAsync(const char* Match, SocketReadCallback&& Callback, char* TempBuffer = nullptr, size_t TempIndex = 0);
			int Connect(SocketAddress* Address, uint64_t Timeout);
			int ConnectAsync(SocketAddress* Address, SocketConnectedCallback&& Callback);
			int Open(SocketAddress* Address);
			int Secure(ssl_ctx_st* Context, const char* Hostname);
			int Bind(SocketAddress* Address);
			int Listen(int Backlog);
			int ClearEvents(bool Gracefully);
			int MigrateTo(socket_t Fd, bool Gracefully = true);
			int SetCloseOnExec();
			int SetTimeWait(int Timeout);
			int SetSocket(int Option, void* Value, int Size);
			int SetAny(int Level, int Option, void* Value, int Size);
			int SetAnyFlag(int Level, int Option, int Value);
			int SetSocketFlag(int Option, int Value);
			int SetBlocking(bool Enabled);
			int SetNoDelay(bool Enabled);
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
			Core::String GetRemoteAddress();

		private:
			int TryCloseAsync(SocketClosedCallback&& Callback, bool KeepTrying);

		public:
			template <typename T>
			T* Context()
			{
				return (T*)UserData;
			}
		};

		class VI_OUT SocketListener final : public Core::Reference<SocketListener>
		{
		public:
			Core::String Name;
			RemoteHost Hostname;
			SocketAddress* Source;
			Socket* Base;

		public:
			SocketListener(const Core::String& NewName, const RemoteHost& NewHost, SocketAddress* NewAddress);
			~SocketListener() noexcept;
		};

		class VI_OUT SocketRouter : public Core::Reference<SocketRouter>
		{
		public:
			Core::UnorderedMap<Core::String, SocketCertificate> Certificates;
			Core::UnorderedMap<Core::String, RemoteHost> Listeners;
			size_t PayloadMaxLength = 12582912;
			size_t BacklogQueue = 20;
			size_t SocketTimeout = 10000;
			size_t MaxConnections = 0;
			int64_t KeepAliveMaxCount = 50;
			int64_t GracefulTimeWait = -1;
			bool EnableNoDelay = false;

		public:
			SocketRouter() = default;
			virtual ~SocketRouter() noexcept;
			RemoteHost& Listen(const Core::String& Hostname, int Port, bool Secure = false);
			RemoteHost& Listen(const Core::String& Pattern, const Core::String& Hostname, int Port, bool Secure = false);
		};

		class VI_OUT SocketConnection : public Core::Reference<SocketConnection>
		{
		public:
			Socket* Stream = nullptr;
			SocketListener* Host = nullptr;
			char RemoteAddress[48] = { };
			DataFrame Info;

		public:
			SocketConnection() = default;
			virtual ~SocketConnection() noexcept;
			virtual void Reset(bool Fully);
			virtual bool Finish();
			virtual bool Finish(int);
			virtual bool Error(int, const char* ErrorMessage, ...);
			virtual bool EncryptionInfo(Certificate* Output);
			virtual bool Break();
		};

		class VI_OUT SocketServer : public Core::Reference<SocketServer>
		{
			friend SocketConnection;

		protected:
			Core::UnorderedSet<SocketConnection*> Active;
			Core::UnorderedSet<SocketConnection*> Inactive;
			Core::Vector<SocketListener*> Listeners;
			SocketRouter* Router = nullptr;
			ServerState State = ServerState::Idle;
			std::mutex Sync;
			size_t Backlog;
			
		public:
			SocketServer() noexcept;
			virtual ~SocketServer() noexcept;
			void SetRouter(SocketRouter* New);
			void SetBacklog(size_t Value);
			void Lock();
			void Unlock();
			bool Configure(SocketRouter* New);
			bool Unlisten(uint64_t TimeoutSeconds = 5);
			bool Listen();
			size_t GetBacklog() const;
			ServerState GetState() const;
			SocketRouter* GetRouter();
			const Core::UnorderedSet<SocketConnection*>& GetActiveClients();
			const Core::UnorderedSet<SocketConnection*>& GetPooledClients();
			const Core::Vector<SocketListener*>& GetListeners();

		protected:
			virtual bool OnConfigure(SocketRouter* New);
			virtual bool OnRequestEnded(SocketConnection* Base, bool Check);
			virtual bool OnRequestBegin(SocketConnection* Base);
			virtual bool OnStall(Core::UnorderedSet<SocketConnection*>& Base);
			virtual bool OnListen();
			virtual bool OnUnlisten();
			virtual SocketConnection* OnAllocate(SocketListener* Host);
			virtual SocketRouter* OnAllocateRouter();

		private:
			bool TryEncryptThenBegin(SocketConnection* Base);

		protected:
			bool FreeAll();
			bool Refuse(SocketConnection* Base);
			bool Accept(SocketListener* Host, socket_t Fd, const Core::String& Address);
			bool EncryptThenBegin(SocketConnection* Fd, SocketListener* Host);
			bool Manage(SocketConnection* Base);
			void Push(SocketConnection* Base);
			SocketConnection* Pop(SocketListener* Host);
		};

		class VI_OUT SocketClient : public Core::Reference<SocketClient>
		{
		protected:
			SocketClientCallback Done;
			ssl_ctx_st* Context;
			Core::String Action;
			Socket Stream;
			RemoteHost Hostname;
			int64_t Timeout;
			bool AutoEncrypt;
			bool IsAsync;

		public:
			SocketClient(int64_t RequestTimeout) noexcept;
			virtual ~SocketClient() noexcept;
			Core::Promise<int> Connect(RemoteHost* Address, bool Async);
			Core::Promise<int> Close();
			Socket* GetStream();

		protected:
			virtual bool OnResolveHost(RemoteHost* Address);
			virtual bool OnConnect();
			virtual bool OnClose();

		private:
			void TryEncrypt(std::function<void(bool)>&& Callback);

		protected:
			void Encrypt(std::function<void(bool)>&& Callback);
			bool Stage(const Core::String& Name);
			bool Error(const char* Message, ...);
			bool Success(int Code);
		};
	}
}
#endif
