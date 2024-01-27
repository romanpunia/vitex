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

namespace Vitex
{
	namespace Network
	{
		struct EpollHandle;

		class Multiplexer;

		class Socket;

		class SocketAddress;

		class SocketConnection;

		enum class SecureLayerOptions
		{
			All = 0,
			NoSSL_V2 = 1 << 1,
			NoSSL_V3 = 1 << 2,
			NoTLS_V1 = 1 << 3,
			NoTLS_V1_1 = 1 << 4,
			NoTLS_V1_2 = 1 << 5,
			NoTLS_V1_3 = 1 << 6
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

		typedef std::function<void(class SocketClient*, Core::ExpectsSystem<void>&&)> SocketClientCallback;
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

		struct VI_OUT X509Blob
		{
			void* Pointer;

			X509Blob(void* NewPointer) noexcept;
			X509Blob(const X509Blob& Other) noexcept;
			X509Blob(X509Blob&& Other) noexcept;
			~X509Blob() noexcept;
			X509Blob& operator= (const X509Blob& Other) noexcept;
			X509Blob& operator= (X509Blob&& Other) noexcept;
		};

		struct VI_OUT CertificateBlob
		{
			Core::String PrivateKey;
			Core::String Certificate;
		};

		struct VI_OUT Certificate
		{
			Core::UnorderedMap<Core::String, Core::String> Extensions;
			Core::String SubjectName;
			Core::String IssuerName;
			Core::String SerialNumber;
			Core::String Fingerprint;
			Core::String PublicKey;
			Core::String NotBeforeDate;
			Core::String NotAfterDate;
			int64_t NotBeforeTime = 0;
			int64_t NotAfterTime = 0;
			int32_t Version = 0;
			int32_t Signature = 0;
			X509Blob Blob = X509Blob(nullptr);

			Core::UnorderedMap<Core::String, Core::Vector<Core::String>> GetFullExtensions() const;
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
			CertificateBlob Blob;
			Core::String Ciphers = "ALL";
			ssl_ctx_st* Context = nullptr;
			SecureLayerOptions Options = SecureLayerOptions::All;
			uint32_t VerifyPeers = 100;
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
			static Core::ExpectsIO<CertificateBlob> GenerateSelfSignedCertificate(uint32_t Days = 365 * 4, const Core::String& AddressesCommaSeparated = "127.0.0.1", const Core::String& DomainsCommaSeparated = Core::String()) noexcept;
			static Core::ExpectsIO<Certificate> GetCertificateFromX509(void* CertificateX509) noexcept;
			static int Poll(pollfd* Fd, int FdCount, int Timeout) noexcept;
			static int Poll(PollFd* Fd, int FdCount, int Timeout) noexcept;
			static Core::String GetLocalAddress() noexcept;
			static Core::String GetAddress(addrinfo* Info) noexcept;
			static Core::String GetAddress(sockaddr* Info) noexcept;
			static std::error_condition GetLastError(ssl_st* Device, int ErrorCode) noexcept;
			static int GetAddressFamily(const char* Address) noexcept;
			static int64_t Clock() noexcept;
			static void DisplayTransportLog() noexcept;
		};

		class VI_OUT_TS TransportLayer final : public Core::Singleton<TransportLayer>
		{
		private:
			Core::UnorderedSet<ssl_ctx_st*> Servers;
			Core::UnorderedSet<ssl_ctx_st*> Clients;
			std::mutex Exclusive;
			bool IsInstalled;

		public:
			TransportLayer() noexcept;
			virtual ~TransportLayer() noexcept override;
			Core::ExpectsIO<ssl_ctx_st*> CreateServerContext(size_t VerifyDepth, SecureLayerOptions Options, const Core::String& CiphersList) noexcept;
			Core::ExpectsIO<ssl_ctx_st*> CreateClientContext(size_t VerifyDepth) noexcept;
			void FreeServerContext(ssl_ctx_st* Context) noexcept;
			void FreeClientContext(ssl_ctx_st* Context) noexcept;

		private:
			Core::ExpectsIO<void> InitializeContext(ssl_ctx_st* Context, bool LoadCertificates) noexcept;
		};

		class VI_OUT_TS DNS final : public Core::Singleton<DNS>
		{
		private:
			Core::UnorderedMap<Core::String, std::pair<int64_t, SocketAddress*>> Names;
			std::mutex Exclusive;

		public:
			DNS() noexcept;
			virtual ~DNS() noexcept override;
			Core::ExpectsSystem<Core::String> FindNameFromAddress(const Core::String& Host, const Core::String& Service);
			Core::ExpectsSystem<SocketAddress*> FindAddressFromName(const Core::String& Host, const Core::String& Service, DNSType TestType, SocketProtocol Proto, SocketType Type);
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

		class VI_OUT_TS Uplinks final : public Core::Singleton<Uplinks>
		{
		private:
			Core::UnorderedMap<Core::String, Core::UnorderedSet<Socket*>> Connections;
			std::mutex Exclusive;

		public:
			Uplinks() noexcept;
			virtual ~Uplinks() noexcept override;
			void ExpireConnection(RemoteHost* Address, Socket* Target);
			bool PushConnection(RemoteHost* Address, Socket* Target);
			Socket* PopConnection(RemoteHost* Address);
			size_t GetSize();

		private:
			void ExpireConnectionURL(const Core::String& URL, Socket* Target);
			void ListenConnectionURL(const Core::String& URL, Socket* Target);
			void UnlistenConnection(Socket* Target);
			Core::String GetURL(RemoteHost* Address);
		};

		class VI_OUT CertificateBuilder final : public Core::Reference<CertificateBuilder>
		{
		private:
			void* Certificate;
			void* PrivateKey;

		public:
			CertificateBuilder() noexcept;
			~CertificateBuilder() noexcept;
			Core::ExpectsIO<void> SetSerialNumber(uint32_t Bits = 160);
			Core::ExpectsIO<void> SetVersion(uint8_t Version);
			Core::ExpectsIO<void> SetNotAfter(int64_t OffsetSeconds);
			Core::ExpectsIO<void> SetNotBefore(int64_t OffsetSeconds = 0);
			Core::ExpectsIO<void> SetPublicKey(uint32_t Bits = 4096);
			Core::ExpectsIO<void> SetIssuer(const X509Blob& Issuer);
			Core::ExpectsIO<void> AddCustomExtension(bool Critical, const Core::String& Key, const Core::String& Value, const Core::String& Description = Core::String());
			Core::ExpectsIO<void> AddStandardExtension(const X509Blob& Issuer, int NID, const Core::String& Value);
			Core::ExpectsIO<void> AddStandardExtension(const X509Blob& Issuer, const Core::String& NameOfNID, const Core::String& Value);
			Core::ExpectsIO<void> AddSubjectInfo(const Core::String& Key, const Core::String& Value);
			Core::ExpectsIO<void> AddIssuerInfo(const Core::String& Key, const Core::String& Value);
			Core::ExpectsIO<void> Sign(Compute::Digest Algorithm);
			Core::ExpectsIO<CertificateBlob> Build();
			void* GetCertificateX509();
			void* GetPrivateKeyEVP_PKEY();
		};

		class VI_OUT SocketAddress final : public Core::Reference<SocketAddress>
		{
			friend DNS;

		private:
			addrinfo* Names;
			addrinfo* Usable;

		public:
			SocketAddress(addrinfo* NewNames, addrinfo* NewUsables) noexcept;
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
			Socket(const Socket& Other) = delete;
			Socket(Socket&& Other) noexcept;
			~Socket() noexcept;
			Socket& operator =(const Socket& Other) = delete;
			Socket& operator =(Socket&& Other) noexcept;
			Core::ExpectsIO<void> Accept(Socket* OutConnection, char* OutAddress);
			Core::ExpectsIO<void> Accept(socket_t* OutFd, char* OutAddress);
			Core::ExpectsIO<void> AcceptAsync(bool WithAddress, SocketAcceptedCallback&& Callback);
			Core::ExpectsIO<void> Close(bool Gracefully = true);
			Core::ExpectsIO<void> CloseAsync(bool Gracefully, SocketStatusCallback&& Callback);
			Core::ExpectsIO<size_t> SendFile(FILE* Stream, size_t Offset, size_t Size);
			Core::ExpectsIO<size_t> SendFileAsync(FILE* Stream, size_t Offset, size_t Size, SocketWrittenCallback&& Callback, size_t TempBuffer = 0);
			Core::ExpectsIO<size_t> Write(const char* Buffer, size_t Size);
			Core::ExpectsIO<size_t> WriteAsync(const char* Buffer, size_t Size, SocketWrittenCallback&& Callback, char* TempBuffer = nullptr, size_t TempOffset = 0);
			Core::ExpectsIO<size_t> Read(char* Buffer, size_t Size);
			Core::ExpectsIO<size_t> Read(char* Buffer, size_t Size, SocketReadCallback&& Callback);
			Core::ExpectsIO<size_t> ReadAsync(size_t Size, SocketReadCallback&& Callback, size_t TempBuffer = 0);
			Core::ExpectsIO<size_t> ReadUntil(const char* Match, SocketReadCallback&& Callback);
			Core::ExpectsIO<size_t> ReadUntilAsync(Core::String&& Match, SocketReadCallback&& Callback, size_t TempIndex = 0, bool TempBuffer = false);
			Core::ExpectsIO<void> Connect(SocketAddress* Address, uint64_t Timeout);
			Core::ExpectsIO<void> ConnectAsync(SocketAddress* Address, SocketStatusCallback&& Callback);
			Core::ExpectsIO<void> Open(SocketAddress* Address);
			Core::ExpectsIO<void> Secure(ssl_ctx_st* Context, const char* Hostname);
			Core::ExpectsIO<void> Bind(SocketAddress* Address);
			Core::ExpectsIO<void> Listen(int Backlog);
			Core::ExpectsIO<void> ClearEvents(bool Gracefully);
			Core::ExpectsIO<void> MigrateTo(socket_t Fd, bool Gracefully = true);
			Core::ExpectsIO<void> SetCloseOnExec();
			Core::ExpectsIO<void> SetTimeWait(int Timeout);
			Core::ExpectsIO<void> SetSocket(int Option, void* Value, size_t Size);
			Core::ExpectsIO<void> SetAny(int Level, int Option, void* Value, size_t Size);
			Core::ExpectsIO<void> SetSocketFlag(int Option, int Value);
			Core::ExpectsIO<void> SetAnyFlag(int Level, int Option, int Value);
			Core::ExpectsIO<void> SetBlocking(bool Enabled);
			Core::ExpectsIO<void> SetNoDelay(bool Enabled);
			Core::ExpectsIO<void> SetKeepAlive(bool Enabled);
			Core::ExpectsIO<void> SetTimeout(int Timeout);
			Core::ExpectsIO<void> GetSocket(int Option, void* Value, size_t* Size);
			Core::ExpectsIO<void> GetAny(int Level, int Option, void* Value, size_t* Size);
			Core::ExpectsIO<void> GetSocketFlag(int Option, int* Value);
			Core::ExpectsIO<void> GetAnyFlag(int Level, int Option, int* Value);
			Core::ExpectsIO<Core::String> GetRemoteAddress();
			Core::ExpectsIO<int> GetPort();
			Core::ExpectsIO<Certificate> GetCertificate();
			socket_t GetFd();
			ssl_st* GetDevice();
			bool IsPendingForRead();
			bool IsPendingForWrite();
			bool IsPending();
			bool IsValid();
			bool IsSecure();

		private:
			Core::ExpectsIO<void> TryCloseAsync(SocketStatusCallback&& Callback, bool KeepTrying);

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
			size_t MaxHeapBuffer = 1024 * 1024 * 4;
			size_t MaxNetBuffer = 1024 * 1024 * 32;
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
			Core::ExpectsSystem<void> Configure(SocketRouter* New);
			Core::ExpectsSystem<void> Unlisten(uint64_t TimeoutSeconds = 5);
			Core::ExpectsSystem<void> Listen();
			void SetRouter(SocketRouter* New);
			void SetBacklog(size_t Value);
			size_t GetBacklog() const;
			ServerState GetState() const;
			SocketRouter* GetRouter();
			const Core::UnorderedSet<SocketConnection*>& GetActiveClients();
			const Core::UnorderedSet<SocketConnection*>& GetPooledClients();
			const Core::Vector<SocketListener*>& GetListeners();

		protected:
			virtual Core::ExpectsSystem<void> OnConfigure(SocketRouter* New);
			virtual Core::ExpectsSystem<void> OnListen();
			virtual Core::ExpectsSystem<void> OnUnlisten();
			virtual void OnRequestOpen(SocketConnection* Base);
			virtual bool OnRequestCleanup(SocketConnection* Base);
			virtual void OnRequestStall(SocketConnection* Base);
			virtual void OnRequestClose(SocketConnection* Base);
			virtual SocketConnection* OnAllocate(SocketListener* Host);
			virtual SocketRouter* OnAllocateRouter();

		private:
			Core::ExpectsIO<void> TryHandshakeThenBegin(SocketConnection* Base);

		protected:
			Core::ExpectsIO<void> Refuse(SocketConnection* Base);
			Core::ExpectsIO<void> Accept(SocketListener* Host, socket_t Fd, const Core::String& Address);
			Core::ExpectsIO<void> HandshakeThenOpen(SocketConnection* Fd, SocketListener* Host);
			Core::ExpectsIO<void> Continue(SocketConnection* Base);
			Core::ExpectsIO<void> Finalize(SocketConnection* Base);
			SocketConnection* Pop(SocketListener* Host);
			void Push(SocketConnection* Base);
			bool FreeAll();
		};

		class VI_OUT SocketClient : public Core::Reference<SocketClient>
		{
		protected:
			struct
			{
				int64_t Idle = 0;
				bool Cache = false;
			} Timeout;

			struct
			{
				SocketClientCallback Done;
				RemoteHost Hostname;
			} State;

			struct
			{
				bool IsAutoEncrypted = true;
				bool IsAsync = false;
				uint32_t VerifyPeers = 100;
			} Config;

			struct
			{
				ssl_ctx_st* Context = nullptr;
				Socket* Stream = nullptr;
			} Net;

		public:
			SocketClient(int64_t RequestTimeout) noexcept;
			virtual ~SocketClient() noexcept;
			Core::ExpectsPromiseSystem<void> Connect(RemoteHost* Address, bool Async, uint32_t VerifyPeers = 100);
			Core::ExpectsPromiseSystem<void> Disconnect();
			bool HasStream() const;
			Socket* GetStream();

		protected:
			virtual Core::ExpectsSystem<void> OnResolveHost(RemoteHost* Address);
			virtual Core::ExpectsSystem<void> OnConnect();
			virtual Core::ExpectsSystem<void> OnDisconnect();

		private:
			bool TryReuseStream(RemoteHost* Address, bool IsAsync);
			void TryHandshake(std::function<void(Core::ExpectsSystem<void>&&)>&& Callback);
			void TryConnect(Core::ExpectsSystem<SocketAddress*>&& Host);
			void DispatchConnection(const Core::Option<std::error_condition>& ErrorCode);
			void DispatchSecureHandshake(Core::ExpectsSystem<void>&& Status);
			void DispatchSimpleHandshake();

		protected:
			void EnableReusability();
			void DisableReusability();
			void Handshake(std::function<void(Core::ExpectsSystem<void>&&)>&& Callback);
			void Report(Core::ExpectsSystem<void>&& Status);
		};
	}
}
#endif
