#ifndef VI_NETWORK_H
#define VI_NETWORK_H
#include "compute.h"
#include <atomic>
struct ssl_ctx_st;
struct ssl_st;
struct addrinfo;
struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;
struct pollfd;

namespace Vitex
{
	namespace Network
	{
		struct epoll_queue;

		struct SocketAccept;

		struct EpollHandle;

		class Multiplexer;

		class Socket;

		class SocketConnection;

		enum
		{
			ADDRESS_SIZE = 64,
			PEER_NOT_SECURE = -1,
			PEER_NOT_VERIFIED = 0,
			PEER_VERITY_DEFAULT = 100,
		};

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

		typedef std::function<void(Core::ExpectsSystem<void>&&)> SocketClientCallback;
		typedef std::function<void(SocketPoll)> PollEventCallback;
		typedef std::function<void(SocketPoll)> SocketWrittenCallback;
		typedef std::function<void(const Core::Option<std::error_condition>&)> SocketStatusCallback;
		typedef std::function<bool(SocketPoll, const uint8_t*, size_t)> SocketReadCallback;
		typedef std::function<bool(SocketAccept&)> SocketAcceptedCallback;

		struct Location
		{
		public:
			Core::UnorderedMap<Core::String, Core::String> Query;
			Core::String Protocol;
			Core::String Username;
			Core::String Password;
			Core::String Hostname;
			Core::String Fragment;
			Core::String Path;
			Core::String Body;
			uint16_t Port;

		public:
			Location(const std::string_view& From) noexcept;
			Location(const Location&) = default;
			Location(Location&&) noexcept = default;
			Location& operator= (const Location&) = default;
			Location& operator= (Location&&) noexcept = default;
		};

		struct X509Blob
		{
			void* Pointer;

			X509Blob(void* NewPointer) noexcept;
			X509Blob(const X509Blob& Other) noexcept;
			X509Blob(X509Blob&& Other) noexcept;
			~X509Blob() noexcept;
			X509Blob& operator= (const X509Blob& Other) noexcept;
			X509Blob& operator= (X509Blob&& Other) noexcept;
		};

		struct CertificateBlob
		{
			Core::String PrivateKey;
			Core::String Certificate;
		};

		struct Certificate
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

		struct DataFrame
		{
			Core::String Message;
			size_t Reuses = 1;
			int64_t Start = 0;
			int64_t Finish = 0;
			int64_t Timeout = 0;
			bool Abort = false;

			DataFrame() = default;
			DataFrame& operator= (const DataFrame& Other);
		};

		struct SocketCertificate
		{
			CertificateBlob Blob;
			Core::String Ciphers = "ALL";
			ssl_ctx_st* Context = nullptr;
			SecureLayerOptions Options = SecureLayerOptions::All;
			uint32_t VerifyPeers = 100;
		};

		struct SocketCidr
		{
			Compute::UInt128 MinValue;
			Compute::UInt128 MaxValue;
			Compute::UInt128 Value;
			uint8_t Mask;

			bool IsMatching(const Compute::UInt128& Value);
		};

		struct SocketAddress
		{
		private:
			struct
			{
				Core::String Hostname;
				uint16_t Port;
				uint16_t Padding;
				int32_t Flags;
				int32_t Family;
				int32_t Type;
				int32_t Protocol;
			} Info;

		private:
			char AddressBuffer[ADDRESS_SIZE];
			size_t AddressSize;

		public:
			SocketAddress() noexcept;
			SocketAddress(const std::string_view& IpAddress, uint16_t Port) noexcept;
			SocketAddress(const std::string_view& Hostname, uint16_t Port, addrinfo* AddressInfo) noexcept;
			SocketAddress(const std::string_view& Hostname, uint16_t Port, sockaddr* Address, size_t AddressSize) noexcept;
			SocketAddress(SocketAddress&&) = default;
			SocketAddress(const SocketAddress&) = default;
			~SocketAddress() = default;
			SocketAddress& operator= (const SocketAddress&) = default;
			SocketAddress& operator= (SocketAddress&&) = default;
			const sockaddr_in* GetAddress4() const noexcept;
			const sockaddr_in6* GetAddress6() const noexcept;
			const sockaddr* GetRawAddress() const noexcept;
			size_t GetAddressSize() const noexcept;
			int32_t GetFlags() const noexcept;
			int32_t GetFamily() const noexcept;
			int32_t GetType() const noexcept;
			int32_t GetProtocol() const noexcept;
			DNSType GetResolverType() const noexcept;
			SocketProtocol GetProtocolType() const noexcept;
			SocketType GetSocketType() const noexcept;
			bool IsValid() const noexcept;
			Core::ExpectsIO<Core::String> GetHostname() const noexcept;
			Core::ExpectsIO<Core::String> GetIpAddress() const noexcept;
			Core::ExpectsIO<uint16_t> GetIpPort() const noexcept;
			Core::ExpectsIO<Compute::UInt128> GetIpValue() const noexcept;
		};

		struct SocketAccept
		{
			SocketAddress Address;
			socket_t Fd = 0;
		};
		
		struct RouterListener
		{
			SocketAddress Address;
			bool IsSecure;
		};

		struct EpollFd
		{
			Socket* Base;
			bool Readable;
			bool Writeable;
			bool Closeable;
		};

		struct EpollHandle
		{
		private:
			epoll_queue* Queue;
			epoll_handle Handle;

		public:
			EpollHandle(size_t MaxEvents) noexcept;
			EpollHandle(const EpollHandle&) = delete;
			EpollHandle(EpollHandle&& Other) noexcept;
			~EpollHandle() noexcept;
			EpollHandle& operator= (const EpollHandle&) = delete;
			EpollHandle& operator= (EpollHandle&& Other) noexcept;
			bool Add(Socket* Fd, bool Readable, bool Writeable) noexcept;
			bool Update(Socket* Fd, bool Readable, bool Writeable) noexcept;
			bool Remove(Socket* Fd) noexcept;
			int Wait(EpollFd* Data, size_t DataSize, uint64_t Timeout) noexcept;
			size_t Capacity() noexcept;
		};

		class Packet
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

		class Utils
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
			static Core::ExpectsIO<CertificateBlob> GenerateSelfSignedCertificate(uint32_t Days = 365 * 4, const std::string_view& AddressesCommaSeparated = "127.0.0.1", const std::string_view& DomainsCommaSeparated = std::string_view()) noexcept;
			static Core::ExpectsIO<Certificate> GetCertificateFromX509(void* CertificateX509) noexcept;
			static Core::Vector<Core::String> GetHostIpAddresses() noexcept;
			static int Poll(pollfd* Fd, int FdCount, int Timeout) noexcept;
			static int Poll(PollFd* Fd, int FdCount, int Timeout) noexcept;
			static std::error_condition GetLastError(ssl_st* Device, int ErrorCode) noexcept;
			static Core::Option<SocketCidr> ParseAddressMask(const std::string_view& Mask) noexcept;
			static bool IsInvalid(socket_t Fd) noexcept;
			static int64_t Clock() noexcept;
			static void DisplayTransportLog() noexcept;
		};

		class TransportLayer final : public Core::Singleton<TransportLayer>
		{
		private:
			std::mutex Exclusive;
			Core::UnorderedSet<ssl_ctx_st*> Servers;
			Core::UnorderedSet<ssl_ctx_st*> Clients;
			bool IsInstalled;

		public:
			TransportLayer() noexcept;
			virtual ~TransportLayer() noexcept override;
			Core::ExpectsIO<ssl_ctx_st*> CreateServerContext(size_t VerifyDepth, SecureLayerOptions Options, const std::string_view& CiphersList) noexcept;
			Core::ExpectsIO<ssl_ctx_st*> CreateClientContext(size_t VerifyDepth) noexcept;
			void FreeServerContext(ssl_ctx_st* Context) noexcept;
			void FreeClientContext(ssl_ctx_st* Context) noexcept;

		private:
			Core::ExpectsIO<void> InitializeContext(ssl_ctx_st* Context, bool LoadCertificates) noexcept;
		};

		class DNS final : public Core::Singleton<DNS>
		{
		private:
			std::mutex Exclusive;
			Core::UnorderedMap<size_t, std::pair<int64_t, SocketAddress>> Names;

		public:
			DNS() noexcept;
			virtual ~DNS() noexcept override;
			Core::ExpectsSystem<Core::String> ReverseLookup(const std::string_view& Hostname, const std::string_view& Service = std::string_view());
			Core::ExpectsPromiseSystem<Core::String> ReverseLookupDeferred(const std::string_view& Hostname, const std::string_view& Service = std::string_view());
			Core::ExpectsSystem<Core::String> ReverseAddressLookup(const SocketAddress& Address);
			Core::ExpectsPromiseSystem<Core::String> ReverseAddressLookupDeferred(const SocketAddress& Address);
			Core::ExpectsSystem<SocketAddress> Lookup(const std::string_view& Hostname, const std::string_view& Service, DNSType Resolver, SocketProtocol Proto = SocketProtocol::TCP, SocketType Type = SocketType::Stream);
			Core::ExpectsPromiseSystem<SocketAddress> LookupDeferred(const std::string_view& Hostname, const std::string_view& Service, DNSType Resolver, SocketProtocol Proto = SocketProtocol::TCP, SocketType Type = SocketType::Stream);
		};

		class Multiplexer final : public Core::Singleton<Multiplexer>
		{
		private:
			std::mutex Exclusive;
			Core::UnorderedSet<Socket*> Trackers;
			Core::Vector<EpollFd> Fds;
			Core::OrderedMap<std::chrono::microseconds, Socket*> Timers;
			EpollHandle Handle;
			std::atomic<size_t> Activations;
			uint64_t DefaultTimeout;

		public:
			Multiplexer() noexcept;
			Multiplexer(uint64_t DispatchTimeout, size_t MaxEvents) noexcept;
			virtual ~Multiplexer() noexcept override;
			void Rescale(uint64_t DispatchTimeout, size_t MaxEvents) noexcept;
			void Activate() noexcept;
			void Deactivate() noexcept;
			void Shutdown() noexcept;
			int Dispatch(uint64_t Timeout) noexcept;
			bool WhenReadable(Socket* Value, PollEventCallback&& WhenReady) noexcept;
			bool WhenWriteable(Socket* Value, PollEventCallback&& WhenReady) noexcept;
			bool CancelEvents(Socket* Value, SocketPoll Event = SocketPoll::Cancel) noexcept;
			bool ClearEvents(Socket* Value) noexcept;
			bool IsListening() noexcept;
			size_t GetActivations() noexcept;

		private:
			void DispatchTimers(const std::chrono::microseconds& Time) noexcept;
			bool DispatchEvents(const EpollFd& Fd, const std::chrono::microseconds& Time) noexcept;
			void TryDispatch() noexcept;
			void TryEnqueue() noexcept;
			void TryListen() noexcept;
			void TryUnlisten() noexcept;
			void AddTimeout(Socket* Value, const std::chrono::microseconds& Time) noexcept;
			void UpdateTimeout(Socket* Value, const std::chrono::microseconds& Time) noexcept;
			void RemoveTimeout(Socket* Value) noexcept;
		};

		class Uplinks final : public Core::Singleton<Uplinks>
		{
		public:
			typedef std::function<void(Socket*)> AcquireCallback;

		private:
			struct ConnectionQueue
			{
				Core::SingleQueue<AcquireCallback> Requests;
				Core::UnorderedSet<Socket*> Streams;
				size_t Duplicates = 0;
			};

		private:
			std::recursive_mutex Exclusive;
			Core::UnorderedMap<Core::String, ConnectionQueue> Connections;
			size_t MaxDuplicates;

		public:
			Uplinks() noexcept;
			virtual ~Uplinks() noexcept override;
			void SetMaxDuplicates(size_t Max);
			bool PushConnection(const SocketAddress& Address, Socket* Target);
			bool PopConnectionQueued(const SocketAddress& Address, AcquireCallback&& Callback);
			Core::Promise<Socket*> PopConnection(const SocketAddress& Address);
			size_t GetMaxDuplicates() const;
			size_t GetSize();

		private:
			void ListenConnection(Core::String&& Id, Socket* Target);
		};

		class CertificateBuilder final : public Core::Reference<CertificateBuilder>
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
			Core::ExpectsIO<void> AddCustomExtension(bool Critical, const std::string_view& Key, const std::string_view& Value, const std::string_view& Description = std::string_view());
			Core::ExpectsIO<void> AddStandardExtension(const X509Blob& Issuer, int NID, const std::string_view& Value);
			Core::ExpectsIO<void> AddStandardExtension(const X509Blob& Issuer, const std::string_view& NameOfNID, const std::string_view& Value);
			Core::ExpectsIO<void> AddSubjectInfo(const std::string_view& Key, const std::string_view& Value);
			Core::ExpectsIO<void> AddIssuerInfo(const std::string_view& Key, const std::string_view& Value);
			Core::ExpectsIO<void> Sign(Compute::Digest Algorithm);
			Core::ExpectsIO<CertificateBlob> Build();
			void* GetCertificateX509();
			void* GetPrivateKeyEVP_PKEY();
		};

		class Socket final : public Core::Reference<Socket>
		{
			friend EpollHandle;
			friend Multiplexer;

		private:
			struct IEvents
			{
				std::mutex Mutex;
				PollEventCallback ReadCallback = nullptr;
				PollEventCallback WriteCallback = nullptr;
				std::chrono::microseconds Expiration = std::chrono::microseconds(0);
				uint64_t Timeout = 0;

				IEvents() = default;
				IEvents(IEvents&& Other) noexcept;
				IEvents& operator=(IEvents&& Other) noexcept;
				~IEvents() = default;
			} Events;

		private:
			ssl_st* Device;
			socket_t Fd;

		public:
			size_t Income;
			size_t Outcome;

		public:
			Socket() noexcept;
			Socket(socket_t FromFd) noexcept;
			Socket(const Socket& Other) = delete;
			Socket(Socket&& Other) noexcept;
			~Socket() noexcept;
			Socket& operator =(const Socket& Other) = delete;
			Socket& operator =(Socket&& Other) noexcept;
			Core::ExpectsIO<void> Accept(SocketAccept* Incoming);
			Core::ExpectsIO<void> AcceptQueued(SocketAcceptedCallback&& Callback);
			Core::ExpectsPromiseIO<SocketAccept> AcceptDeferred();
			Core::ExpectsIO<void> Shutdown(bool Gracefully = false);
			Core::ExpectsIO<void> Close();
			Core::ExpectsIO<void> CloseQueued(SocketStatusCallback&& Callback);
			Core::ExpectsPromiseIO<void> CloseDeferred();
			Core::ExpectsIO<size_t> WriteFile(FILE* Stream, size_t Offset, size_t Size);
			Core::ExpectsIO<size_t> WriteFileQueued(FILE* Stream, size_t Offset, size_t Size, SocketWrittenCallback&& Callback, size_t TempBuffer = 0);
			Core::ExpectsPromiseIO<size_t> WriteFileDeferred(FILE* Stream, size_t Offset, size_t Size);
			Core::ExpectsIO<size_t> Write(const uint8_t* Buffer, size_t Size);
			Core::ExpectsIO<size_t> WriteQueued(const uint8_t* Buffer, size_t Size, SocketWrittenCallback&& Callback, bool CopyBufferWhenAsync = true, uint8_t* TempBuffer = nullptr, size_t TempOffset = 0);
			Core::ExpectsPromiseIO<size_t> WriteDeferred(const uint8_t* Buffer, size_t Size, bool CopyBufferWhenAsync = true);
			Core::ExpectsIO<size_t> Read(uint8_t* Buffer, size_t Size);
			Core::ExpectsIO<size_t> ReadQueued(size_t Size, SocketReadCallback&& Callback, size_t TempBuffer = 0);
			Core::ExpectsPromiseIO<Core::String> ReadDeferred(size_t Size);
			Core::ExpectsIO<size_t> ReadUntil(const std::string_view& Match, SocketReadCallback&& Callback);
			Core::ExpectsIO<size_t> ReadUntilQueued(Core::String&& Match, SocketReadCallback&& Callback, size_t TempIndex = 0, bool TempBuffer = false);
			Core::ExpectsPromiseIO<Core::String> ReadUntilDeferred(Core::String&& Match, size_t MaxSize);
			Core::ExpectsIO<size_t> ReadUntilChunked(const std::string_view& Match, SocketReadCallback&& Callback);
			Core::ExpectsIO<size_t> ReadUntilChunkedQueued(Core::String&& Match, SocketReadCallback&& Callback, size_t TempIndex = 0, bool TempBuffer = false);
			Core::ExpectsPromiseIO<Core::String> ReadUntilChunkedDeferred(Core::String&& Match, size_t MaxSize);
			Core::ExpectsIO<void> Connect(const SocketAddress& Address, uint64_t Timeout);
			Core::ExpectsIO<void> ConnectQueued(const SocketAddress& Address, SocketStatusCallback&& Callback);
			Core::ExpectsPromiseIO<void> ConnectDeferred(const SocketAddress& Address);
			Core::ExpectsIO<void> Open(const SocketAddress& Address);
			Core::ExpectsIO<void> Secure(ssl_ctx_st* Context, const std::string_view& ServerName);
			Core::ExpectsIO<void> Bind(const SocketAddress& Address);
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
			Core::ExpectsIO<SocketAddress> GetPeerAddress();
			Core::ExpectsIO<SocketAddress> GetThisAddress();
			Core::ExpectsIO<Certificate> GetCertificate();
			void SetIoTimeout(uint64_t TimeoutMs);
			socket_t GetFd() const;
			ssl_st* GetDevice() const;
			bool IsAwaitingReadable();
			bool IsAwaitingWriteable();
			bool IsAwaitingEvents();
			bool IsSecure() const;
			bool IsValid() const;

		private:
			Core::ExpectsIO<void> TryCloseQueued(SocketStatusCallback&& Callback, const std::chrono::microseconds& Time, bool KeepTrying);
		};

		class SocketListener final : public Core::Reference<SocketListener>
		{
		public:
			Core::String Name;
			SocketAddress Address;
			Socket* Stream;
			bool IsSecure;

		public:
			SocketListener(const std::string_view& NewName, const SocketAddress& NewAddress, bool Secure);
			~SocketListener() noexcept;
		};

		class SocketRouter : public Core::Reference<SocketRouter>
		{
		public:
			Core::UnorderedMap<Core::String, SocketCertificate> Certificates;
			Core::UnorderedMap<Core::String, RouterListener> Listeners;
			size_t MaxHeapBuffer = 1024 * 1024 * 4;
			size_t MaxNetBuffer = 1024 * 1024 * 32;
			size_t BacklogQueue = 20;
			size_t SocketTimeout = 10000;
			size_t MaxConnections = 0;
			int64_t KeepAliveMaxCount = 0;
			int64_t GracefulTimeWait = -1;
			bool EnableNoDelay = false;

		public:
			SocketRouter() = default;
			virtual ~SocketRouter() noexcept;
			Core::ExpectsSystem<RouterListener*> Listen(const std::string_view& Hostname, const std::string_view& Service, bool Secure = false);
			Core::ExpectsSystem<RouterListener*> Listen(const std::string_view& Pattern, const std::string_view& Hostname, const std::string_view& Service, bool Secure = false);
		};

		class SocketConnection : public Core::Reference<SocketConnection>
		{
		public:
			SocketAddress Address;
			DataFrame Info;
			Socket* Stream = nullptr;
			SocketListener* Host = nullptr;

		public:
			SocketConnection() = default;
			virtual ~SocketConnection() noexcept;
			virtual void Reset(bool Fully);
			virtual bool Abort();
			virtual bool Abort(int, const char* Format, ...);
			virtual bool Next();
			virtual bool Next(int);
			virtual bool Closable(SocketRouter* Router);
		};

		class SocketServer : public Core::Reference<SocketServer>
		{
			friend SocketConnection;

		protected:
			std::recursive_mutex Exclusive;
			Core::UnorderedSet<SocketConnection*> Active;
			Core::UnorderedSet<SocketConnection*> Inactive;
			Core::Vector<SocketListener*> Listeners;
			SocketRouter* Router;
			uint64_t ShutdownTimeout;
			size_t Backlog;
			std::atomic<ServerState> State;
			
		public:
			SocketServer() noexcept;
			virtual ~SocketServer() noexcept;
			Core::ExpectsSystem<void> Configure(SocketRouter* New);
			Core::ExpectsSystem<void> Unlisten(bool Gracefully);
			Core::ExpectsSystem<void> Listen();
			void SetRouter(SocketRouter* New);
			void SetBacklog(size_t Value);
			void SetShutdownTimeout(uint64_t TimeoutMs);
			size_t GetBacklog() const;
			uint64_t GetShutdownTimeout() const;
			ServerState GetState() const;
			SocketRouter* GetRouter();
			const Core::UnorderedSet<SocketConnection*>& GetActiveClients();
			const Core::UnorderedSet<SocketConnection*>& GetPooledClients();
			const Core::Vector<SocketListener*>& GetListeners();

		protected:
			virtual Core::ExpectsSystem<void> OnConfigure(SocketRouter* New);
			virtual Core::ExpectsSystem<void> OnListen();
			virtual Core::ExpectsSystem<void> OnUnlisten();
			virtual Core::ExpectsSystem<void> OnAfterUnlisten();
			virtual void OnRequestOpen(SocketConnection* Base);
			virtual bool OnRequestCleanup(SocketConnection* Base);
			virtual void OnRequestStall(SocketConnection* Base);
			virtual void OnRequestClose(SocketConnection* Base);
			virtual SocketConnection* OnAllocate(SocketListener* Host);
			virtual SocketRouter* OnAllocateRouter();
			virtual Core::ExpectsIO<void> TryHandshakeThenBegin(SocketConnection* Base);
			virtual Core::ExpectsIO<void> Refuse(SocketConnection* Base);
			virtual Core::ExpectsIO<void> Accept(SocketListener* Host, SocketAccept&& Incoming);
			virtual Core::ExpectsIO<void> HandshakeThenOpen(SocketConnection* Fd, SocketListener* Host);
			virtual Core::ExpectsIO<void> Continue(SocketConnection* Base);
			virtual Core::ExpectsIO<void> Finalize(SocketConnection* Base);
			virtual SocketConnection* Pop(SocketListener* Host);
			virtual void Push(SocketConnection* Base);
			virtual bool FreeAll();
		};

		class SocketClient : public Core::Reference<SocketClient>
		{
		protected:
			struct
			{
				int64_t Idle = 0;
				int8_t Cache = -1;
			} Timeout;

			struct
			{
				SocketClientCallback Resolver;
				SocketAddress Address;
			} State;

			struct
			{
				bool IsAutoHandshake = true;
				bool IsNonBlocking = false;
				int32_t VerifyPeers = 100;
			} Config;

			struct
			{
				ssl_ctx_st* Context = nullptr;
				Socket* Stream = nullptr;
			} Net;

		public:
			SocketClient(int64_t RequestTimeout) noexcept;
			virtual ~SocketClient() noexcept;
			Core::ExpectsSystem<void> ConnectQueued(const SocketAddress& Address, bool AsNonBlocking, int32_t VerifyPeers, SocketClientCallback&& Callback);
			Core::ExpectsSystem<void> DisconnectQueued(SocketClientCallback&& Callback);
			Core::ExpectsPromiseSystem<void> ConnectSync(const SocketAddress& Address, int32_t VerifyPeers = PEER_NOT_SECURE);
			Core::ExpectsPromiseSystem<void> ConnectAsync(const SocketAddress& Address, int32_t VerifyPeers = PEER_NOT_SECURE);
			Core::ExpectsPromiseSystem<void> Disconnect();
			void ApplyReusability(bool KeepAlive);
			const SocketAddress& GetPeerAddress() const;
			bool HasStream() const;
			bool IsSecure() const;
			bool IsVerified() const;
			Socket* GetStream();

		protected:
			virtual Core::ExpectsSystem<void> OnConnect();
			virtual Core::ExpectsSystem<void> OnReuse();
			virtual Core::ExpectsSystem<void> OnDisconnect();
			virtual bool TryReuseStream(const SocketAddress& Address, std::function<void(bool)>&& Callback);
			virtual bool TryStoreStream();
			virtual void TryHandshake(std::function<void(Core::ExpectsSystem<void>&&)>&& Callback);
			virtual void DispatchConnection(const Core::Option<std::error_condition>& ErrorCode);
			virtual void DispatchSecureHandshake(Core::ExpectsSystem<void>&& Status);
			virtual void DispatchSimpleHandshake();
			virtual void CreateStream();
			virtual void ConfigureStream();
			virtual void DestroyStream();
			virtual void EnableReusability();
			virtual void DisableReusability();
			virtual void Handshake(std::function<void(Core::ExpectsSystem<void>&&)>&& Callback);
			virtual void Report(Core::ExpectsSystem<void>&& Status);
		};
	}
}
#endif
