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
struct pollfd;

namespace Tomahawk
{
	namespace Network
	{
		typedef std::function<bool(struct Socket*, const char*, int64_t)> SocketReadCallback;
		typedef std::function<void(struct Socket*, int64_t)> SocketWriteCallback;
		typedef std::function<bool(struct Socket*)> SocketAcceptCallback;
		typedef std::function<void(class SocketClient*, int)> SocketClientCallback;

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

		enum class SocketType
		{
			Stream,
			Datagram,
			Raw,
			Reliably_Delivered_Message,
			Sequence_Packet_Stream
		};

		inline SocketEvent operator |(SocketEvent A, SocketEvent B)
		{
			return static_cast<SocketEvent>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}

		struct TH_OUT Address
		{
			addrinfo* Host = nullptr;
			addrinfo* Active = nullptr;
			bool Resolved = false;

			static bool Free(Network::Address* Address);
		};

		struct TH_OUT WriteEvent
		{
			char* Buffer = nullptr;
			int64_t Size = 0;
			SocketWriteCallback Callback;
		};

		struct TH_OUT ReadEvent
		{
			const char* Match = nullptr;
			int64_t Size = 0, Index = 0;
			SocketReadCallback Callback;
		};

		struct TH_OUT Socket
		{
			friend Driver;

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
			int Open(const char* Host, int Port, SocketType Type, Address* Result);
			int Open(const char* Host, int Port, Address* Result);
			int Open(addrinfo* Info, Address* Result);
			int Secure(ssl_ctx_st* Context, const char* Hostname);
			int Bind(Address* Address);
			int Connect(Address* Address);
			int Listen(int Backlog);
			int Accept(Socket* Connection, Address* Output);
			int AcceptAsync(SocketAcceptCallback&& Callback);
			int Close(bool Gracefully = true);
			int CloseAsync(bool Gracefully, const SocketAcceptCallback& Callback);
			int CloseOnExec();
			int Skip(unsigned int IO, int Code);
			int Clear(bool Gracefully);
			int Write(const char* Buffer, int Size);
			int Write(const char* Buffer, int Size, const SocketWriteCallback& Callback);
			int Write(const std::string& Buffer);
			int WriteAsync(const char* Buffer, int64_t Size, SocketWriteCallback&& Callback);
			int fWrite(const char* Format, ...);
			int fWriteAsync(SocketWriteCallback&& Callback, const char* Format, ...);
			int Read(char* Buffer, int Size);
			int Read(char* Buffer, int Size, const SocketReadCallback& Callback);
			int ReadAsync(int64_t Size, SocketReadCallback&& Callback);
			int ReadUntil(const char* Match, const SocketReadCallback& Callback);
			int ReadUntilAsync(const char* Match, SocketReadCallback&& Callback);
			int SetFd(socket_t Fd);
			int SetReadNotify(SocketReadCallback&& Callback);
			int SetWriteNotify(SocketWriteCallback&& Callback);
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
			bool ReadSet(SocketReadCallback&& Callback, const char* Match, int64_t Size, int64_t Index);
			bool ReadFlush();
			bool WriteSet(SocketWriteCallback&& Callback, const char* Buffer, int64_t Size);
			bool WriteFlush();

		public:
			template <typename T>
			T* Context()
			{
				return (T*)UserData;
			}

		public:
			static std::string LocalIpAddress();
		};

		struct TH_OUT Host
		{
			std::string Hostname;
			int Port = 0;
			bool Secure = false;
		};

		struct TH_OUT Listener
		{
			Address Source;
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
			uint64_t PayloadMaxLength = std::numeric_limits<uint64_t>::max();
			uint64_t BacklogQueue = 20;
			uint64_t SocketTimeout = 5000;
			uint64_t CloseTimeout = 500;
			uint64_t MaxConnections = 0;
			int64_t KeepAliveMaxCount = 10;
			int64_t GracefulTimeWait = -1;
			bool EnableNoDelay = false;

			virtual ~SocketRouter();
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
			static std::mutex* fSources;
			static epoll_handle Handle;
			static int64_t PipeTimeout;
			static int ArraySize;

		public:
			static void Create(int MaxEvents = 256, int64_t Timeout = 100);
			static void Release();
			static void Multiplex();
			static int Dispatch();
			static int Listen(Socket* Value, bool Always);
			static int Unlisten(Socket* Value, bool Always);
			static int Poll(pollfd* Fd, int FdCount, int Timeout);
			static int64_t Clock();
			static bool IsActive();

		private:
			static int Dispatch(Socket* Value, uint32_t Events, int64_t Time);
		};

		class TH_OUT SocketServer : public Core::Object
		{
			friend SocketConnection;

		protected:
			std::unordered_set<SocketConnection*> Good;
			std::unordered_set<SocketConnection*> Bad;
			std::vector<Listener*> Listeners;
			SocketRouter* Router = nullptr;
			ServerState State = ServerState::Idle;
			Core::TimerId Timer = -1;
			std::mutex Sync;

		public:
			SocketServer();
			virtual ~SocketServer() override;
			void Lock();
			void Unlock();
			bool Configure(SocketRouter* New);
			bool Unlisten();
			bool Listen();
			ServerState GetState();
			SocketRouter* GetRouter();
			std::unordered_set<SocketConnection*>* GetClients();

		protected:
			virtual bool OnConfigure(SocketRouter* New);
			virtual bool OnRequestEnded(SocketConnection* Base, bool Check);
			virtual bool OnRequestBegin(SocketConnection* Base);
			virtual bool OnDeallocate(SocketConnection* Base);
			virtual bool OnDeallocateRouter(SocketRouter* Base);
			virtual bool OnListen();
			virtual bool OnUnlisten();
			virtual bool OnProtect(Socket* Fd, Listener* Host, ssl_ctx_st** Context);
			virtual SocketConnection* OnAllocate(Listener* Host, Socket* Stream);
			virtual SocketRouter* OnAllocateRouter();

		protected:
			bool FreeQueued();
			bool Accept(Listener* Host);
			bool Protect(Socket* Fd, Listener* Host);
			bool Manage(SocketConnection* Base);
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
