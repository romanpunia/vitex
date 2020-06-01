#ifndef THAWK_NETWORK_H
#define THAWK_NETWORK_H

#include "compute.h"
#include <atomic>

struct ssl_ctx_st;
struct ssl_st;
struct addrinfo;
struct pollfd;
#ifdef THAWK_APPLE
struct kevent;
#else
struct epoll_event;
#endif

namespace Tomahawk
{
	namespace Network
	{
		typedef std::function<bool(struct Socket*, const char*, Int64)> SocketReadCallback;
		typedef std::function<bool(struct Socket*, Int64)> SocketWriteCallback;
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

		struct THAWK_OUT Address
		{
			addrinfo* Host = nullptr;
			addrinfo* Active = nullptr;
			bool Resolved = false;

			static bool Free(Network::Address* Address);
		};

		struct THAWK_OUT WriteEvent
		{
			WriteEvent* Prev = nullptr;
			WriteEvent* Next = nullptr;
			char* Buffer = nullptr;
			Int64 Size = 0;
			SocketWriteCallback Callback;
		};

		struct THAWK_OUT ReadEvent
		{
			ReadEvent* Prev = nullptr;
			ReadEvent* Next = nullptr;
			const char* Match = nullptr;
			Int64 Size = 0, Index = 0;
			SocketReadCallback Callback;
		};

		struct THAWK_OUT Socket
		{
			friend Multiplexer;

		private:
			struct
			{
				std::mutex IO;
				std::mutex Device;
				Int64 Time, Timeout;
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
			Int64 Income, Outcome;
			void* UserData;

		public:
			Socket();
			int Open(const char* Host, int Port, SocketType Type, Address* Result);
			int Open(const char* Host, int Port, Address* Result);
			int Open(addrinfo* Info, Address* Result);
			int Secure(ssl_ctx_st* Context);
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
			int WriteAsync(const char* Buffer, Int64 Size, const SocketWriteCallback& Callback);
			int fWrite(const char* Format, ...);
			int fWriteAsync(const SocketWriteCallback& Callback, const char* Format, ...);
			int Read(char* Buffer, int Size);
			int Read(char* Buffer, int Size, const SocketReadCallback& Callback);
			int ReadAsync(Int64 Size, const SocketReadCallback& Callback);
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
			int SetAsyncTimeout(Int64 Timeout);
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
			Int64 GetAsyncTimeout();

		private:
			void ReadPush(const SocketReadCallback& Callback, const char* Match, Int64 Size, Int64 Index);
			void ReadPop();
			void WritePush(const SocketWriteCallback& Callback, const char* Buffer, Int64 Size);
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

		struct THAWK_OUT Host
		{
			std::string Hostname;
			int Port = 0;
			bool Secure = false;
		};

		struct THAWK_OUT Listener
		{
			Address Source;
			std::string Name;
			Host* Hostname = nullptr;
			Socket* Base = nullptr;
		};

		struct THAWK_OUT Certificate
		{
			std::string Subject;
			std::string Issuer;
			std::string Serial;
			std::string Finger;
		};

		struct THAWK_OUT DataFrame
		{
			std::string Message;
			std::mutex Sync;
			Int64 Start = 0;
			Int64 Finish = 0;
			Int64 Timeout = 0;
			int KeepAlive = 1;
			bool Error = false;
			bool Await = false;
		};

		struct THAWK_OUT SocketCertificate
		{
			ssl_ctx_st* Context = nullptr;
			std::string Key;
			std::string Chain;
			std::string Ciphers = "ALL";
			Secure Protocol = Secure_Any;
			bool VerifyPeers = true;
			UInt64 Depth = 9;
		};

		struct THAWK_OUT SocketConnection
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

		struct THAWK_OUT SocketRouter
		{
			std::unordered_map<std::string, SocketCertificate> Certificates;
			std::unordered_map<std::string, Host> Listeners;
			UInt64 KeepAliveMaxCount = 10;
			UInt64 PayloadMaxLength = std::numeric_limits<UInt64>::max();
			UInt64 BacklogQueue = 20;
			UInt64 SocketTimeout = 5000;
			UInt64 MasterTimeout = 200;
			UInt64 CloseTimeout = 500;
			UInt64 MaxEvents = 256;
			UInt64 MaxConnections = 0;
			bool EnableNoDelay = false;

			virtual ~SocketRouter();
		};

		class THAWK_OUT Multiplexer
		{
			friend struct Socket;

		private:
#ifdef THAWK_APPLE
			static kevent* Array;
#else
			static epoll_event* Array;
#endif
			static Rest::EventQueue* Loop;
			static epoll_handle Handle;
			static Int64 PipeTimeout;
			static int ArraySize;

		public:
			static void Create(int MaxEvents);
			static void Release();
			static void Dispatch();
			static bool Create(int MaxEvents, Int64 Timeout, Rest::EventQueue* Queue);
			static bool Bind(Int64 Timeout, Rest::EventQueue* Queue);
			static int Listen(Socket* Value);
			static int Unlisten(Socket* Value);
			static int Dispatch(Socket* Value, int* Events, Int64 Time);
			static int Poll(pollfd* Fd, int FdCount, int Timeout);
			static Int64 Clock();
			static Rest::EventQueue* GetQueue();

		private:
			static void Worker(Rest::EventQueue* Queue, Rest::EventArgs* Args);
		};

		class THAWK_OUT SocketServer : public Rest::Object
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

		class THAWK_OUT SocketClient : public Rest::Object
		{
		protected:
			SocketClientCallback Done;
			ssl_ctx_st* Context;
			std::string Action;
			Socket Stream;
			Host Hostname;
			Int64 Timeout;
			bool AutoCertify;

		public:
			SocketClient(Int64 RequestTimeout);
			virtual ~SocketClient() override;
			bool Connect(Host* Address, const SocketClientCallback& Callback);
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