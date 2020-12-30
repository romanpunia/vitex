#ifndef TH_NETWORK_H
#define TH_NETWORK_H

#include "compute.h"
#include <atomic>

struct ssl_ctx_st;
struct ssl_st;
struct addrinfo;
struct pollfd;
#ifdef TH_APPLE
struct kevent;
#else
struct epoll_event;
#endif

namespace Tomahawk
{
	namespace Network
	{
		typedef std::function<bool(struct Socket*, const char*, int64_t)> SocketReadCallback;
		typedef std::function<bool(struct Socket*, int64_t)> SocketWriteCallback;
		typedef std::function<bool(struct Socket*)> SocketAcceptCallback;
		typedef std::function<void(class SocketClient*, int)> SocketClientCallback;

		class Multiplexer;

		enum Secure
		{
			Secure_Any,
			Secure_SSL_V2,
			Secure_SSL_V3,
			Secure_TLS_V1,
			Secure_TLS_V1_1,
		};

		enum ServerState
		{
			ServerState_Working,
			ServerState_Stopping,
			ServerState_Idle
		};

		enum SocketEvent
		{
			SocketEvent_Read = (1 << 0),
			SocketEvent_Write = (1 << 1),
			SocketEvent_Close = (1 << 2),
			SocketEvent_Timeout = (1 << 3),
			SocketEvent_None = (1 << 4)
		};

		enum SocketType
		{
			SocketType_Stream,
			SocketType_Datagram,
			SocketType_Raw,
			SocketType_Reliably_Delivered_Message,
			SocketType_Sequence_Packet_Stream
		};

		struct TH_OUT Address
		{
			addrinfo* Host = nullptr;
			addrinfo* Active = nullptr;
			bool Resolved = false;

			static bool Free(Network::Address* Address);
		};

		struct TH_OUT WriteEvent
		{
			WriteEvent* Prev = nullptr;
			WriteEvent* Next = nullptr;
			char* Buffer = nullptr;
			int64_t Size = 0;
			SocketWriteCallback Callback;
		};

		struct TH_OUT ReadEvent
		{
			ReadEvent* Prev = nullptr;
			ReadEvent* Next = nullptr;
			const char* Match = nullptr;
			int64_t Size = 0, Index = 0;
			SocketReadCallback Callback;
		};

		struct TH_OUT Socket
		{
			friend Multiplexer;

		private:
			struct
			{
				std::mutex IO;
				std::mutex Device;
				int64_t Time, Timeout;
				bool Await;
			} Sync;

		private:
			SocketAcceptCallback Listener;
			ReadEvent* Input;
			WriteEvent* Output;
			ssl_st* Device;
			socket_t Fd;
			int TimeWait = 1;

		public:
			int64_t Income, Outcome;
			void* UserData;

		public:
			Socket();
			int Open(const char* Host, int Port, SocketType Type, Address* Result);
			int Open(const char* Host, int Port, Address* Result);
			int Open(addrinfo* Info, Address* Result);
			int Secure(ssl_ctx_st* Context, const char* Hostname);
			int Bind(Address* Address);
			int Connect(Address* Address);
			int Listen(int Backlog);
			int Accept(Socket* Connection, Address* Output);
			int AcceptAsync(const SocketAcceptCallback& Callback);
			int Close(bool Detach);
			int CloseOnExec();
			int Skip(int IO, int Code);
			int Clear();
			int Write(const char* Buffer, int Size);
			int Write(const char* Buffer, int Size, const SocketWriteCallback& Callback);
			int Write(const std::string& Buffer);
			int WriteAsync(const char* Buffer, int64_t Size, const SocketWriteCallback& Callback);
			int fWrite(const char* Format, ...);
			int fWriteAsync(const SocketWriteCallback& Callback, const char* Format, ...);
			int Read(char* Buffer, int Size);
			int Read(char* Buffer, int Size, const SocketReadCallback& Callback);
			int ReadAsync(int64_t Size, const SocketReadCallback& Callback);
			int ReadUntil(const char* Match, const SocketReadCallback& Callback);
			int ReadUntilAsync(const char* Match, const SocketReadCallback& Callback);
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
			bool IsAwaiting();
			bool HasIncomingData();
			bool HasOutcomingData();
			bool HasPendingData();
			std::string GetRemoteAddress();
			int64_t GetAsyncTimeout();

		private:
			void ReadPush(const SocketReadCallback& Callback, const char* Match, int64_t Size, int64_t Index);
			void ReadPop();
			void WritePush(const SocketWriteCallback& Callback, const char* Buffer, int64_t Size);
			void WritePop();

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
			std::unordered_map<std::string, std::string> Query;
			std::string URL;
			std::string Protocol;
			std::string Login;
			std::string Password;
			std::string Host;
			std::string Path;
			int Port;

			SourceURL(const std::string& Src);
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
			int KeepAlive = 1;
			bool Error = false;
			bool Await = false;
		};

		struct TH_OUT SocketCertificate
		{
			ssl_ctx_st* Context = nullptr;
			std::string Key;
			std::string Chain;
			std::string Ciphers = "ALL";
			Secure Protocol = Secure_Any;
			bool VerifyPeers = true;
			uint64_t Depth = 9;
		};

		struct TH_OUT SocketConnection
		{
			Socket* Stream = nullptr;
			Listener* Host = nullptr;
			DataFrame Info;

			virtual ~SocketConnection();
			virtual void Lock();
			virtual void Unlock();
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
			uint64_t KeepAliveMaxCount = 10;
			uint64_t PayloadMaxLength = std::numeric_limits<uint64_t>::max();
			uint64_t BacklogQueue = 20;
			uint64_t SocketTimeout = 5000;
			uint64_t MasterTimeout = 200;
			uint64_t CloseTimeout = 500;
			uint64_t MaxEvents = 256;
			uint64_t MaxConnections = 0;
			bool EnableNoDelay = false;

			virtual ~SocketRouter();
		};

		class TH_OUT Multiplexer
		{
			friend struct Socket;

		private:
#ifdef TH_APPLE
			static kevent* Array;
#else
			static epoll_event* Array;
#endif
			static Rest::EventQueue* Loop;
			static epoll_handle Handle;
			static int64_t PipeTimeout;
			static int ArraySize;

		public:
			static void Create(int MaxEvents);
			static void Release();
			static void Dispatch();
			static bool Create(int MaxEvents, int64_t Timeout, Rest::EventQueue* Queue);
			static bool Bind(int64_t Timeout, Rest::EventQueue* Queue);
			static int Listen(Socket* Value);
			static int Unlisten(Socket* Value);
			static int Dispatch(Socket* Value, int* Events, int64_t Time);
			static int Poll(pollfd* Fd, int FdCount, int Timeout);
			static int64_t Clock();
			static Rest::EventQueue* GetQueue();

		private:
			static void Worker(Rest::EventQueue* Queue, Rest::EventArgs* Args);
		};

		class TH_OUT SocketServer : public Rest::Object
		{
			friend SocketConnection;

		protected:
			std::unordered_set<SocketConnection*> Good;
			std::unordered_set<SocketConnection*> Bad;
			std::vector<Listener*> Listeners;
			Rest::EventQueue* Queue = nullptr;
			Rest::EventTimer* Worker = nullptr;
			SocketRouter* Router = nullptr;
			ServerState State = ServerState_Idle;
			std::mutex Sync;

		public:
			SocketServer();
			virtual ~SocketServer() override;
			void Lock();
			void Unlock();
			bool Configure(SocketRouter* New);
			bool Unlisten();
			bool Listen(Rest::EventQueue* Loop);
			ServerState GetState();
			SocketRouter* GetRouter();
			Rest::EventQueue* GetQueue();
			std::unordered_set<SocketConnection*>* GetClients();

		protected:
			virtual bool OnConfigure(SocketRouter* New);
			virtual bool OnRequestEnded(SocketConnection* Base, bool Check);
			virtual bool OnRequestBegin(SocketConnection* Base);
			virtual bool OnDeallocate(SocketConnection* Base);
			virtual bool OnDeallocateRouter(SocketRouter* Base);
			virtual bool OnListen(Rest::EventQueue* Loop);
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

		class TH_OUT SocketClient : public Rest::Object
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
			bool Connect(Host* Address, bool Async, const SocketClientCallback& Callback);
			bool Close(const SocketClientCallback& Callback);
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