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

		enum class SocketEvent
		{
			Read = (1 << 0),
			Write = (1 << 1),
			Close = (1 << 2),
			Timeout = (1 << 3),
			None = (1 << 4)
		};

		enum class DNSType
		{
			Connect,
			Listen
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

		enum class NetEvent
		{
			Packet,
			Timeout,
			Finished,
			Cancelled,
			Closed
		};

		typedef std::function<bool(NetEvent, const char*, size_t)> NetReadCallback;
		typedef std::function<void(NetEvent, size_t)> NetWriteCallback;
		typedef std::function<bool(struct Socket*)> SocketAcceptCallback;
		typedef std::function<void(class SocketClient*, int)> SocketClientCallback;

		inline SocketEvent operator |(SocketEvent A, SocketEvent B)
		{
			return static_cast<SocketEvent>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}

		struct TH_OUT Address
		{
			addrinfo* Addresses = nullptr;
			addrinfo* Good = nullptr;
		};

		struct TH_OUT WriteEvent
		{
			char* Buffer = nullptr;
			size_t Size = 0;
			NetWriteCallback Callback;
		};

		struct TH_OUT ReadEvent
		{
			const char* Match = nullptr;
			size_t Size = 0, Index = 0;
			NetReadCallback Callback;
		};

		struct TH_OUT Socket
		{
			friend Driver;
			friend SocketConnection;

		private:
			struct
			{
				std::mutex IO;
				std::mutex Device;
				std::atomic<bool> Poll;
				int64_t Time, Timeout;
			} Sync;

		private:
			SocketAcceptCallback* Listener;
			ReadEvent* Input;
			WriteEvent* Output;
			ssl_st* Device;
			socket_t Fd;

		public:
			int64_t Income, Outcome;
			void* UserData;

		public:
			Socket();
			Socket(socket_t FromFd);
			~Socket();
			int Open(const std::string& Host, const std::string& Port, SocketProtocol Proto, SocketType Type, DNSType DNS, Address** Result);
			int Open(const std::string& Host, const std::string& Port, DNSType DNS, Address** Result);
			int Open(addrinfo* Good);
			int Secure(ssl_ctx_st* Context, const char* Hostname);
			int Bind(Address* Address);
			int Connect(Address* Address, uint64_t Timeout);
			int Listen(int Backlog);
			int Accept(Socket* Connection, Address* Output);
			int AcceptAsync(SocketAcceptCallback&& Callback);
			int Close(bool Gracefully = true);
			int CloseAsync(bool Gracefully, const SocketAcceptCallback& Callback);
			int CloseOnExec();
			int Skip(unsigned int IO, NetEvent Reason);
			int Clear(bool Gracefully);
			int Write(const char* Buffer, int Size);
			int Write(const char* Buffer, int Size, const NetWriteCallback& Callback);
			int Write(const std::string& Buffer);
			int WriteAsync(const char* Buffer, size_t Size, NetWriteCallback&& Callback);
			int fWrite(const char* Format, ...);
			int fWriteAsync(NetWriteCallback&& Callback, const char* Format, ...);
			int Read(char* Buffer, int Size);
			int Read(char* Buffer, int Size, const NetReadCallback& Callback);
			int ReadAsync(size_t Size, NetReadCallback&& Callback);
			int ReadUntil(const char* Match, const NetReadCallback& Callback);
			int ReadUntilAsync(const char* Match, NetReadCallback&& Callback);
			int SetFd(socket_t Fd);
			int SetReadNotify(NetReadCallback&& Callback);
			int SetWriteNotify(NetWriteCallback&& Callback);
			int SetTimeWait(int Timeout);
			int SetSocket(int Option, void* Value, int Size);
			int SetAny(int Level, int Option, void* Value, int Size);
			int SetAnyFlag(int Level, int Option, int Value);
			int SetSocketFlag(int Option, int Value);
			int SetBlocking(bool Enabled);
			int SetNodelay(bool Enabled);
			int SetKeepAlive(bool Enabled);
			int SetTimeout(int Timeout);
			int SetAsyncTimeout(int64_t Timeout);
			int GetError(int Result);
			int GetAnyFlag(int Level, int Option, int* Value);
			int GetAny(int Level, int Option, void* Value, int* Size);
			int GetSocket(int Option, void* Value, int* Size);
			int GetSocketFlag(int Option, int* Value);
			int GetPort();
			socket_t GetFd();
			ssl_st* GetDevice();
			bool IsValid();
			bool HasIncomingData();
			bool HasOutcomingData();
			bool HasPendingData();
			std::string GetRemoteAddress();
			int64_t GetAsyncTimeout();

		private:
			bool CloseSet(const SocketAcceptCallback& Callback, bool OK);
			bool ReadSet(NetReadCallback&& Callback, const char* Match, size_t Size, size_t Index);
			bool ReadFlush();
			bool WriteSet(NetWriteCallback&& Callback, const char* Buffer, size_t Size);
			bool WriteFlush();

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

			virtual ~SocketRouter();
		};

		class TH_OUT Packet
		{
		public:
			static bool IsData(NetEvent Event)
			{
				return Event == NetEvent::Packet;
			}
			static bool IsSkip(NetEvent Event)
			{
				return Event == NetEvent::Cancelled;
			}
			static bool IsDone(NetEvent Event)
			{
				return Event == NetEvent::Finished;
			}
			static bool IsError(NetEvent Event)
			{
				return Event == NetEvent::Closed || Event == NetEvent::Timeout;
			}
			static bool IsErrorOrSkip(NetEvent Event)
			{
				return Event == NetEvent::Closed || Event == NetEvent::Timeout || Event == NetEvent::Cancelled;
			}
		};

		class TH_OUT Driver
		{
			friend struct Socket;

		private:
#ifdef TH_APPLE
			static kevent* Array;
#else
			static epoll_event* Array;
#endif
			static std::unordered_set<Socket*>* Sources;
			static std::unordered_map<std::string, std::pair<int64_t, Address*>> Names;
			static std::mutex Exclusive;
			static epoll_handle Handle;
			static int ArraySize;

		public:
			static void Create(int MaxEvents = 256);
			static void Release();
			static int Dispatch(int64_t Timeout);
			static int Listen(Socket* Value, bool Always);
			static int Unlisten(Socket* Value, bool Always);
			static int Poll(pollfd* Fd, int FdCount, int Timeout);
			static int64_t Clock();
			static bool IsActive();
			static std::string ResolveDNSReverse(const std::string& IpAddress, uint32_t Port);
			static Address* ResolveDNS(const std::string& Host, const std::string& Service, SocketProtocol Proto, SocketType Type, DNSType DNS);

		private:
			static int Dispatch(Socket* Value, uint32_t Events, int64_t Time);
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
			bool Accept(Listener* Host);
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
