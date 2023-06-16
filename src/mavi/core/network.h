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

		typedef std::function<void(class SocketClient*, const Core::Option<std::error_condition>&)> SocketClientCallback;
		typedef std::function<void(SocketPoll)> PollEventCallback;
		typedef std::function<void(SocketPoll)> SocketWrittenCallback;
		typedef std::function<void(SocketAddress*)> SocketOpenedCallback;
		typedef std::function<void(const Core::Option<std::error_condition>&)> SocketStatusCallback;
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
			EpollHandle(const EpollHandle& Other) noexcept;
			EpollHandle(EpollHandle&& Other) noexcept;
			~EpollHandle() noexcept;
			EpollHandle& operator= (const EpollHandle& Other) noexcept;
			EpollHandle& operator= (EpollHandle&& Other) noexcept;
			bool Add(Socket* Fd, bool Readable, bool Writeable) noexcept;
			bool Update(Socket* Fd, bool Readable, bool Writeable) noexcept;
			bool Remove(Socket* Fd, bool Readable, bool Writeable) noexcept;
			int Wait(EpollFd* Data, size_t DataSize, uint64_t Timeout) noexcept;
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
			static std::error_condition ToCondition(SocketPoll Event)
			{
				switch (Event)
				{
					case SocketPoll::Next:
						return std::make_error_condition(std::errc::operation_in_progress);
					case SocketPoll::Reset:
						return std::make_error_condition(std::errc::connection_reset);
					case SocketPoll::Timeout:
						return std::make_error_condition(std::errc::timed_out);
					case SocketPoll::Cancel:
						return std::make_error_condition(std::errc::operation_canceled);
					case SocketPoll::Finish:
					case SocketPoll::FinishSync:
					default:
						return std::make_error_condition(std::errc());
				}
			}
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
			static int Poll(pollfd* Fd, int FdCount, int Timeout) noexcept;
			static int Poll(PollFd* Fd, int FdCount, int Timeout) noexcept;
			static Core::String GetLocalAddress() noexcept;
			static Core::String GetAddress(addrinfo* Info) noexcept;
			static Core::String GetAddress(sockaddr* Info) noexcept;
			static std::error_condition GetLastError(ssl_st* Device, int ErrorCode) noexcept;
			static int GetAddressFamily(const char* Address) noexcept;
			static int64_t Clock() noexcept;
		};

		class VI_OUT_TS DNS final : public Core::Singleton<DNS>
		{
		private:
			Core::UnorderedMap<Core::String, std::pair<int64_t, SocketAddress*>> Names;
			std::mutex Exclusive;

		public:
			DNS() noexcept;
			virtual ~DNS() noexcept override;
			Core::Expected<Core::String> FindNameFromAddress(const Core::String& Host, const Core::String& Service);
			Core::Expected<SocketAddress*> FindAddressFromName(const Core::String& Host, const Core::String& Service, DNSType TestType, SocketProtocol Proto, SocketType Type);
		};

		class VI_OUT_TS Multiplexer final : public Core::Singleton<Multiplexer>
		{
		private:
			Core::OrderedMap<std::chrono::microseconds, Socket*> Timeouts;
			Core::Vector<EpollFd> Fds;
			std::atomic<size_t> Activations;
			std::recursive_mutex Exclusive;
			EpollHandle Handle;
			uint64_t DefaultTimeout;

		public:
			Multiplexer() noexcept;
			Multiplexer(uint64_t DispatchTimeout, size_t MaxEvents) noexcept;
			virtual ~Multiplexer() noexcept override;
			void Rescale(uint64_t DispatchTimeout, size_t MaxEvents) noexcept;
			void Activate() noexcept;
			void Deactivate() noexcept;
			int Dispatch(uint64_t Timeout) noexcept;
			bool WhenReadable(Socket* Value, PollEventCallback&& WhenReady) noexcept;
			bool WhenWriteable(Socket* Value, PollEventCallback&& WhenReady) noexcept;
			bool CancelEvents(Socket* Value, SocketPoll Event = SocketPoll::Cancel, bool EraseTimeout = true) noexcept;
			bool ClearEvents(Socket* Value) noexcept;
			bool IsAwaitingEvents(Socket* Value) noexcept;
			bool IsAwaitingReadable(Socket* Value) noexcept;
			bool IsAwaitingWriteable(Socket* Value) noexcept;
			bool IsListening() noexcept;
			size_t GetActivations() noexcept;

		private:
			bool WhenEvents(Socket* Value, bool Readable, bool Writeable, PollEventCallback&& WhenReadable, PollEventCallback&& WhenWriteable) noexcept;
			bool DispatchEvents(EpollFd& Fd, const std::chrono::microseconds& Time) noexcept;
			void AddTimeout(Socket* Value, const std::chrono::microseconds& Time) noexcept;
			void UpdateTimeout(Socket* Value, const std::chrono::microseconds& Time) noexcept;
			void RemoveTimeout(Socket* Value) noexcept;
			void TryDispatch() noexcept;
			void TryEnqueue() noexcept;
			void TryListen() noexcept;
			void TryUnlisten() noexcept;
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
			size_t Income;
			size_t Outcome;
			void* UserData;

		public:
			Socket() noexcept;
			Socket(socket_t FromFd) noexcept;
			Core::Expected<void> Accept(Socket* OutConnection, char* OutAddress);
			Core::Expected<void> Accept(socket_t* OutFd, char* OutAddress);
			Core::Expected<void> AcceptAsync(bool WithAddress, SocketAcceptedCallback&& Callback);
			Core::Expected<void> Close(bool Gracefully = true);
			Core::Expected<void> CloseAsync(bool Gracefully, SocketStatusCallback&& Callback);
			Core::Expected<size_t> SendFile(FILE* Stream, size_t Offset, size_t Size);
			Core::Expected<size_t> SendFileAsync(FILE* Stream, size_t Offset, size_t Size, SocketWrittenCallback&& Callback, size_t TempBuffer = 0);
			Core::Expected<size_t> Write(const char* Buffer, size_t Size);
			Core::Expected<size_t> WriteAsync(const char* Buffer, size_t Size, SocketWrittenCallback&& Callback, char* TempBuffer = nullptr, size_t TempOffset = 0);
			Core::Expected<size_t> Read(char* Buffer, size_t Size);
			Core::Expected<size_t> Read(char* Buffer, size_t Size, SocketReadCallback&& Callback);
			Core::Expected<size_t> ReadAsync(size_t Size, SocketReadCallback&& Callback, size_t TempBuffer = 0);
			Core::Expected<size_t> ReadUntil(const char* Match, SocketReadCallback&& Callback);
			Core::Expected<size_t> ReadUntilAsync(const char* Match, SocketReadCallback&& Callback, char* TempBuffer = nullptr, size_t TempIndex = 0);
			Core::Expected<void> Connect(SocketAddress* Address, uint64_t Timeout);
			Core::Expected<void> ConnectAsync(SocketAddress* Address, SocketStatusCallback&& Callback);
			Core::Expected<void> Open(SocketAddress* Address);
			Core::Expected<void> Secure(ssl_ctx_st* Context, const char* Hostname);
			Core::Expected<void> Bind(SocketAddress* Address);
			Core::Expected<void> Listen(int Backlog);
			Core::Expected<void> ClearEvents(bool Gracefully);
			Core::Expected<void> MigrateTo(socket_t Fd, bool Gracefully = true);
			Core::Expected<void> SetCloseOnExec();
			Core::Expected<void> SetTimeWait(int Timeout);
			Core::Expected<void> SetSocket(int Option, void* Value, size_t Size);
			Core::Expected<void> SetAny(int Level, int Option, void* Value, size_t Size);
			Core::Expected<void> SetSocketFlag(int Option, int Value);
			Core::Expected<void> SetAnyFlag(int Level, int Option, int Value);
			Core::Expected<void> SetBlocking(bool Enabled);
			Core::Expected<void> SetNoDelay(bool Enabled);
			Core::Expected<void> SetKeepAlive(bool Enabled);
			Core::Expected<void> SetTimeout(int Timeout);
			Core::Expected<void> GetSocket(int Option, void* Value, size_t* Size);
			Core::Expected<void> GetAny(int Level, int Option, void* Value, size_t* Size);
			Core::Expected<void> GetSocketFlag(int Option, int* Value);
			Core::Expected<void> GetAnyFlag(int Level, int Option, int* Value);
			Core::Expected<Core::String> GetRemoteAddress();
			Core::Expected<int> GetPort();
			socket_t GetFd();
			ssl_st* GetDevice();
			bool IsPendingForRead();
			bool IsPendingForWrite();
			bool IsPending();
			bool IsValid();

		private:
			Core::Expected<void> TryCloseAsync(SocketStatusCallback&& Callback, bool KeepTrying);

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
			std::mutex Exclusive;
			size_t Backlog;
			
		public:
			SocketServer() noexcept;
			virtual ~SocketServer() noexcept;
			Core::Expected<void> Configure(SocketRouter* New);
			Core::Expected<void> Unlisten(uint64_t TimeoutSeconds = 5);
			Core::Expected<void> Listen();
			void SetRouter(SocketRouter* New);
			void SetBacklog(size_t Value);
			size_t GetBacklog() const;
			ServerState GetState() const;
			SocketRouter* GetRouter();
			const Core::UnorderedSet<SocketConnection*>& GetActiveClients();
			const Core::UnorderedSet<SocketConnection*>& GetPooledClients();
			const Core::Vector<SocketListener*>& GetListeners();

		protected:
			virtual Core::Expected<void> OnConfigure(SocketRouter* New);
			virtual Core::Expected<void> OnListen();
			virtual Core::Expected<void> OnUnlisten();
			virtual void OnRequestOpen(SocketConnection* Base);
			virtual bool OnRequestCleanup(SocketConnection* Base);
			virtual void OnRequestStall(SocketConnection* Base);
			virtual void OnRequestClose(SocketConnection* Base);
			virtual SocketConnection* OnAllocate(SocketListener* Host);
			virtual SocketRouter* OnAllocateRouter();

		private:
			Core::Expected<void> TryEncryptThenBegin(SocketConnection* Base);

		protected:
			Core::Expected<void> Refuse(SocketConnection* Base);
			Core::Expected<void> Accept(SocketListener* Host, socket_t Fd, const Core::String& Address);
			Core::Expected<void> EncryptThenOpen(SocketConnection* Fd, SocketListener* Host);
			Core::Expected<void> Continue(SocketConnection* Base);
			SocketConnection* Pop(SocketListener* Host);
			void Push(SocketConnection* Base);
			bool FreeAll();
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
			Core::ExpectedPromise<void> Connect(RemoteHost* Address, bool Async);
			Core::ExpectedPromise<void> Close();
			Socket* GetStream();

		protected:
			virtual bool OnResolveHost(RemoteHost* Address);
			virtual bool OnConnect();
			virtual bool OnClose();

		private:
			void TryEncrypt(std::function<void(const Core::Option<std::error_condition>&)>&& Callback);

		protected:
			void Encrypt(std::function<void(const Core::Option<std::error_condition>&)>&& Callback);
			bool Stage(const Core::String& Name);
			bool Error(const char* Message, ...);
			bool Report(const Core::Option<std::error_condition>& ErrorCode);
		};
	}
}
#endif
