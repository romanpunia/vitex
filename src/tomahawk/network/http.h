#ifndef TH_NETWORK_HTTP_H
#define TH_NETWORK_HTTP_H

#include "../core/network.h"
#include "../core/script.h"

namespace Tomahawk
{
	namespace Network
	{
		namespace HTTP
		{
			enum Auth
			{
				Auth_Granted,
				Auth_Denied,
				Auth_Unverified
			};

			enum Content
			{
				Content_Not_Loaded,
				Content_Lost,
				Content_Cached,
				Content_Empty,
				Content_Corrupted,
				Content_Payload_Exceeded,
				Content_Wants_Save,
				Content_Saved,
				Content_Save_Exception
			};

			enum QueryValue
			{
				QueryValue_Unknown,
				QueryValue_Number,
				QueryValue_String,
				QueryValue_Boolean,
				QueryValue_Object
			};

			enum WebSocketOp
			{
				WebSocketOp_Continue = 0x00,
				WebSocketOp_Text = 0x01,
				WebSocketOp_Binary = 0x02,
				WebSocketOp_Close = 0x08,
				WebSocketOp_Ping = 0x09,
				WebSocketOp_Pong = 0x0A
			};

			enum WebSocketState
			{
				WebSocketState_Active = (1 << 0),
				WebSocketState_Handshake = (1 << 1),
				WebSocketState_Reset = (1 << 2),
				WebSocketState_Close = (1 << 3),
				WebSocketState_Free = (1 << 4)
			};

			enum CompressionTune
			{
				CompressionTune_Filtered = 1,
				CompressionTune_Huffman = 2,
				CompressionTune_Rle = 3,
				CompressionTune_Fixed = 4,
				CompressionTune_Default = 0
			};

			typedef std::function<bool(struct Connection*)> SuccessCallback;
			typedef std::function<void(struct Connection*, const char*)> MessageCallback;
			typedef std::function<bool(struct Connection*, const char*, int64_t)> ContentCallback;
			typedef std::function<bool(struct Connection*, struct Resource*, int64_t)> ResourceCallback;
			typedef std::function<bool(struct Connection*, struct Credentials*, const std::string&)> AuthorizeCallback;
			typedef std::function<bool(struct Connection*, Rest::Stroke*)> HeaderCallback;
			typedef std::function<void(struct WebSocketFrame*)> WebSocketCallback;
			typedef std::function<void(struct WebSocketFrame*, const char*, int64_t, enum WebSocketOp)> WebSocketReadCallback;
			typedef std::function<bool(struct Connection*, Script::VMCompiler*)> GatewayCallback;
			typedef std::function<bool(class Parser*, int64_t)> ParserCodeCallback;
			typedef std::function<bool(class Parser*, const char*, int64_t)> ParserDataCallback;
			typedef std::function<bool(class Parser*)> ParserNotifyCallback;
			typedef std::function<void(class Client*, struct RequestFrame*, struct ResponseFrame*)> ResponseCallback;

			struct Connection;

			struct RouteEntry;

			struct SiteEntry;

			struct MapRouter;

			class Server;

			class Query;

			struct TH_OUT ErrorFile
			{
				std::string Pattern;
				int StatusCode = 0;
			};

			struct TH_OUT MimeType
			{
				std::string Extension;
				std::string Type;
			};

			struct TH_OUT MimeStatic
			{
				const char* Extension = nullptr;
				const char* Type = nullptr;

				MimeStatic(const char* Ext, const char* T);
			};

			struct TH_OUT Credentials
			{
				std::string Username;
				std::string Password;
				Auth Type = Auth_Unverified;
			};

			struct TH_OUT Header
			{
				std::string Key;
				std::string Value;
			};

			struct TH_OUT Resource
			{
				std::vector<Header> Headers;
				std::string Path;
				std::string Type;
				std::string Name;
				std::string Key;
				uint64_t Length = 0;
				bool Memory = false;

				void SetHeader(const char* Key, const char* Value);
				void SetHeader(const char* Key, const std::string& Value);
				const char* GetHeader(const char* Key);
			};

			struct TH_OUT Cookie
			{
				std::string Name;
				std::string Value;
				std::string Domain;
				std::string Path;
				uint64_t Expires = 0;
				bool Secure = false;
			};

			struct TH_OUT RequestFrame
			{
				std::vector<Resource> Resources;
				std::vector<Header> Cookies;
				std::vector<Header> Headers;
				std::string Buffer;
				std::string Query;
				std::string Path;
				std::string URI;
				Compute::RegexResult Match;
				Credentials User;
				Content ContentState = Content_Not_Loaded;
				char RemoteAddress[48] = { 0 };
				char Method[10] = { 0 };
				char Version[10] = { 0 };
				uint64_t ContentLength = 0;

				void SetHeader(const char* Key, const char* Value);
				void SetHeader(const char* Key, const std::string& Value);
				const char* GetCookie(const char* Key);
				const char* GetHeader(const char* Key);
				std::vector<std::pair<int64_t, int64_t>> GetRanges();
				std::pair<uint64_t, uint64_t> GetRange(std::vector<std::pair<int64_t, int64_t>>::iterator Range, uint64_t ContentLength);
			};

			struct TH_OUT ResponseFrame
			{
				std::vector<Cookie> Cookies;
				std::vector<Header> Headers;
				std::vector<char> Buffer;
				int StatusCode = -1;

				void SetHeader(const char* Key, const char* Value);
				void SetHeader(const char* Key, const std::string& Value);
				void SetCookie(const char* Key, const char* Value, const char* Domain, const char* Path, uint64_t Expires, bool Secure);
				const char* GetHeader(const char* Key);
				Cookie* GetCookie(const char* Key);
			};

			struct TH_OUT WebSocketFrame
			{
				friend struct GatewayFrame;

				friend class Util;

			private:
				std::string Buffer;
				uint64_t BodyLength = 0;
				uint64_t MaskLength = 0;
				uint64_t HeaderLength = 0;
				uint64_t DataLength = 0;
				int State = WebSocketState_Handshake;
				unsigned char Mask[4] = { 0 };
				unsigned char Opcode = 0;
				bool Clear = false;
				bool Error = false;
				bool Notified = false;
				bool Save = false;

			public:
				HTTP::Connection* Base = nullptr;
				WebSocketCallback Connect;
				WebSocketCallback Disconnect;
				WebSocketCallback Notification;
				WebSocketReadCallback Receive;

			public:
				void Write(const char* Buffer, int64_t Length, WebSocketOp OpCode, const SuccessCallback& Callback);
				void Finish();
				void Next();
				void Execute(Script::VMContext* Context);
				void Notify();
				bool IsFinished();
			};

			struct TH_OUT GatewayFrame
			{
				friend WebSocketFrame;

				friend class Util;

			private:
				char* Buffer;
				int64_t Size;
				bool Save;

			public:
				Script::VMCompiler* Compiler;
				Connection* Base;

			public:
				GatewayFrame(char* Data, int64_t DataSize);
				bool Finish();
				bool Error();
				bool Start();
				bool Execute(Script::VMContext* Context);
				bool IsDone();
				Script::VMContext* GetContext();

			private:
				bool Done(bool Normal);
			};

			struct TH_OUT ParserFrame
			{
				RequestFrame* Request = nullptr;
				ResponseFrame* Response = nullptr;
				RouteEntry* Route = nullptr;
				FILE* Stream = nullptr;
				std::string Header;
				Resource Source;
				ResourceCallback Callback;
				bool Close = false;
			};

			struct TH_OUT QueryToken
			{
				char* Value = nullptr;
				uint64_t Length = 0;
			};

			struct TH_OUT RouteEntry
			{
				struct
				{
					struct
					{
						WebSocketCallback Connect;
						WebSocketCallback Disconnect;
						WebSocketReadCallback Receive;
					} WebSocket;

					SuccessCallback Get;
					SuccessCallback Post;
					SuccessCallback Put;
					SuccessCallback Patch;
					SuccessCallback Delete;
					SuccessCallback Options;
					SuccessCallback Access;
					SuccessCallback Proxy;
					HeaderCallback Headers;
					GatewayCallback Gateway;
					AuthorizeCallback Authorize;
				} Callbacks;
				struct
				{
					std::vector<Compute::RegExp> Files;
					std::vector<std::string> Methods;
				} Gateway;
				struct
				{
					std::vector<Credentials> Users;
					std::vector<std::string> Methods;
					std::string Type;
					std::string Realm;
				} Auth;
				struct
				{
					std::vector<Compute::RegExp> Files;
					CompressionTune Tune = CompressionTune_Default;
					int QualityLevel = 8;
					int MemoryLevel = 8;
					uint64_t MinLength = 16384;
					bool Enabled = false;
				} Compression;

				std::vector<Compute::RegExp> HiddenFiles;
				std::vector<ErrorFile> ErrorFiles;
				std::vector<MimeType> MimeTypes;
				std::vector<std::string> IndexFiles;
				std::vector<std::string> DisallowedMethods;
				std::string DocumentRoot = "./";
				std::string CharSet = "utf-8";
				std::string ProxyIpAddress = "";
				std::string AccessControlAllowOrigin;
				std::string Refer;
				std::string Default;
				uint64_t WebSocketTimeout = 30000;
				uint64_t StaticFileMaxAge = 604800;
				uint64_t MaxCacheLength = 16384;
				uint64_t GracefulTimeWait = 1;
				bool AllowDirectoryListing = true;
				bool AllowWebSocket = false;
				bool AllowSendFile = false;
				Compute::RegExp URI;
				SiteEntry* Site = nullptr;
			};

			struct TH_OUT SiteEntry
			{
				struct
				{
					struct
					{
						std::string DocumentRoot = "./sessions/";
						std::string Name = "Sid";
						std::string Domain;
						std::string Path = "/";
						uint64_t Expires = 604800;
						uint64_t CookieExpires = 31536000;
					} Session;

					bool Enabled = false;
				} Gateway;
				struct
				{
					SuccessCallback OnRewriteURL;
				} Callbacks;

				std::unordered_set<std::string> Hosts;
				std::vector<RouteEntry*> Routes;
				std::string ResourceRoot = "./files/";
				std::string SiteName;
				uint64_t MaxResources = 5;
				RouteEntry* Base = nullptr;
				MapRouter* Router = nullptr;

				SiteEntry();
				~SiteEntry();
				RouteEntry* Route(const char* Pattern);
				bool Get(const char* Pattern, SuccessCallback Callback);
				bool Post(const char* Pattern, SuccessCallback Callback);
				bool Put(const char* Pattern, SuccessCallback Callback);
				bool Patch(const char* Pattern, SuccessCallback Callback);
				bool Delete(const char* Pattern, SuccessCallback Callback);
				bool Options(const char* Pattern, SuccessCallback Callback);
				bool Access(const char* Pattern, SuccessCallback Callback);
				bool WebSocketConnect(const char* Pattern, WebSocketCallback Callback);
				bool WebSocketDisconnect(const char* Pattern, WebSocketCallback Callback);
				bool WebSocketReceive(const char* Pattern, WebSocketReadCallback Callback);
			};

			struct TH_OUT MapRouter : public SocketRouter
			{
				std::vector<SiteEntry*> Sites;
				std::string ModuleRoot;
				Script::VMManager* VM;

				MapRouter();
				virtual ~MapRouter() override;
				SiteEntry* Site(const char* Host);
			};

			struct TH_OUT Connection : public SocketConnection
			{
				Rest::Resource Resource;
				WebSocketFrame* WebSocket = nullptr;
				GatewayFrame* Gateway = nullptr;
				RouteEntry* Route = nullptr;
				Server* Root = nullptr;
				RequestFrame Request;
				ResponseFrame Response;

				virtual ~Connection() override;
				bool Finish() override;
				bool Finish(int StatusCode) override;
				bool Certify(Certificate* Output) override;
				bool Consume(const ContentCallback& Callback = nullptr);
				bool Store(const ResourceCallback& Callback = nullptr);
			};

			class TH_OUT QueryParameter : public Rest::Document
			{
			public:
				std::string Build();
				std::string BuildFromBase();
				QueryParameter* Find(QueryToken* Name);
			};

			class TH_OUT Query : public Rest::Object
			{
			public:
				QueryParameter* Object;

			public:
				Query();
				virtual ~Query() override;
				void Clear();
				void Decode(const char* ContentType, const std::string& URI);
				std::string Encode(const char* ContentType);
				QueryParameter* Get(const char* Name);
				QueryParameter* Set(const char* Name);
				QueryParameter* Set(const char* Name, const char* Value);

			private:
				void NewParameter(std::vector<QueryToken>* Tokens, const QueryToken& Name, const QueryToken& Value);
				void DecodeAXWFD(const std::string& URI);
				void DecodeAJSON(const std::string& URI);
				std::string EncodeAXWFD();
				std::string EncodeAJSON();
				QueryParameter* GetParameter(QueryToken* Name);
			};

			class TH_OUT Session : public Rest::Object
			{
			public:
				Rest::Document* Query = nullptr;
				std::string SessionId;
				int64_t SessionExpires = 0;
				bool IsNewSession = false;

			public:
				Session();
				virtual ~Session() override;
				void Clear();
				bool Write(Connection* Base);
				bool Read(Connection* Base);

			private:
				std::string& FindSessionId(Connection* Base);
				std::string& GenerateSessionId(Connection* Base);

			public:
				static bool InvalidateCache(const std::string& Path);
			};

			class TH_OUT Parser : public Rest::Object
			{
			private:
				enum MultipartState
				{
					MultipartState_Uninitialized = 1,
					MultipartState_Start,
					MultipartState_Start_Boundary,
					MultipartState_Header_Field_Start,
					MultipartState_Header_Field,
					MultipartState_Header_Field_Waiting,
					MultipartState_Header_Value_Start,
					MultipartState_Header_Value,
					MultipartState_Header_Value_Waiting,
					MultipartState_Resource_Start,
					MultipartState_Resource,
					MultipartState_Resource_Boundary_Waiting,
					MultipartState_Resource_Boundary,
					MultipartState_Resource_Waiting,
					MultipartState_Resource_End,
					MultipartState_Resource_Hyphen,
					MultipartState_End
				};

				enum ChunkedState
				{
					ChunkedState_Size,
					ChunkedState_Ext,
					ChunkedState_Data,
					ChunkedState_End,
					ChunkedState_Head,
					ChunkedState_Middle
				};

			private:
				struct
				{
					char* LookBehind = nullptr;
					char* Boundary = nullptr;
					unsigned char State = MultipartState_Start;
					int64_t Index = 0, Length = 0;
				} Multipart;

				struct
				{
					size_t Length = 0;
					char ConsumeTrailer = 1;
					char HexCount = 0;
					char State = 0;
				} Chunked;

			public:
				ParserCodeCallback OnStatusCode;
				ParserDataCallback OnStatusMessage;
				ParserDataCallback OnQueryValue;
				ParserDataCallback OnVersion;
				ParserDataCallback OnPathValue;
				ParserDataCallback OnMethodValue;
				ParserDataCallback OnHeaderField;
				ParserDataCallback OnHeaderValue;
				ParserDataCallback OnContentData;
				ParserNotifyCallback OnResourceBegin;
				ParserNotifyCallback OnResourceEnd;
				void* UserPointer = nullptr;

			public:
				Parser();
				virtual ~Parser() override;
				int64_t MultipartParse(const char* Boundary, const char* Buffer, int64_t Length);
				int64_t ParseRequest(const char* BufferStart, uint64_t Length, uint64_t LastLength);
				int64_t ParseResponse(const char* BufferStart, uint64_t Length, uint64_t LastLength);
				int64_t ParseDecodeChunked(char* Buffer, int64_t* BufferLength);

			private:
				const char* Tokenize(const char* Buffer, const char* BufferEnd, const char** Token, uint64_t* TokenLength, int* Out);
				const char* Complete(const char* Buffer, const char* BufferEnd, uint64_t LastLength, int* Out);
				const char* ProcessVersion(const char* Buffer, const char* BufferEnd, int* Out);
				const char* ProcessHeaders(const char* Buffer, const char* BufferEnd, int* Out);
				const char* ProcessRequest(const char* Buffer, const char* BufferEnd, int* Out);
				const char* ProcessResponse(const char* Buffer, const char* BufferEnd, int* Out);
			};

			class TH_OUT Util
			{
			public:
				static void ConstructPath(Connection* Base);
				static void ConstructHeadFull(RequestFrame* Request, ResponseFrame* Response, bool IsRequest, Rest::Stroke* Buffer);
				static void ConstructHeadCache(Connection* Base, Rest::Stroke* Buffer);
				static void ConstructHeadUncache(Connection* Base, Rest::Stroke* Buffer);
				static bool ConstructRoute(MapRouter* Router, Connection* Base);
				static bool WebSocketWrite(Connection* Base, const char* Buffer, int64_t Length, WebSocketOp Type, const SuccessCallback& Callback);
				static bool WebSocketWriteMask(Connection* Base, const char* Buffer, int64_t Length, WebSocketOp Type, unsigned int Mask, const SuccessCallback& Callback);
				static bool ConstructDirectoryEntries(const Rest::ResourceEntry& A, const Rest::ResourceEntry& B);
				static std::string ConnectionResolve(Connection* Base);
				static std::string ConstructContentRange(uint64_t Offset, uint64_t Length, uint64_t ContenLength);
				static const char* ContentType(const std::string& Path, std::vector<MimeType>* MimeTypes);
				static const char* StatusMessage(int StatusCode);

			public:
				static bool ParseMultipartHeaderField(Parser* Parser, const char* Name, uint64_t Length);
				static bool ParseMultipartHeaderValue(Parser* Parser, const char* Name, uint64_t Length);
				static bool ParseMultipartContentData(Parser* Parser, const char* Name, uint64_t Length);
				static bool ParseMultipartResourceBegin(Parser* Parser);
				static bool ParseMultipartResourceEnd(Parser* Parser);
				static bool ParseHeaderField(Parser* Parser, const char* Name, uint64_t Length);
				static bool ParseHeaderValue(Parser* Parser, const char* Name, uint64_t Length);
				static bool ParseVersion(Parser* Parser, const char* Name, uint64_t Length);
				static bool ParseStatusCode(Parser* Parser, uint64_t Length);
				static bool ParseMethodValue(Parser* Parser, const char* Name, uint64_t Length);
				static bool ParsePathValue(Parser* Parser, const char* Name, uint64_t Length);
				static bool ParseQueryValue(Parser* Parser, const char* Name, uint64_t Length);
				static int ParseContentRange(const char* ContentRange, int64_t* Range1, int64_t* Range2);
				static std::string ParseMultipartDataBoundary();

			public:
				static bool Authorize(Connection* Base);
				static bool MethodAllowed(Connection* Base);
				static bool WebSocketUpgradeAllowed(Connection* Base);

			public:
				static bool ResourceHidden(Connection* Base, std::string* Path);
				static bool ResourceIndexed(Connection* Base, Rest::Resource* Resource);
				static bool ResourceProvided(Connection* Base, Rest::Resource* Resource);
				static bool ResourceModified(Connection* Base, Rest::Resource* Resource);
				static bool ResourceCompressed(Connection* Base, uint64_t Size);

			public:
				static bool RouteWEBSOCKET(Connection* Base);
				static bool RouteGET(Connection* Base);
				static bool RoutePOST(Connection* Base);
				static bool RoutePUT(Connection* Base);
				static bool RouteDELETE(Connection* Base);
				static bool RoutePATCH(Connection* Base);
				static bool RouteOPTIONS(Connection* Base);

			public:
				static bool ProcessDirectory(Connection* Base);
				static bool ProcessResource(Connection* Base);
				static bool ProcessResourceCompress(Connection* Base, bool Deflate, bool Gzip, const char* ContentRange, uint64_t Range);
				static bool ProcessResourceCache(Connection* Base);
				static bool ProcessFile(Connection* Base, uint64_t ContentLength, uint64_t Range);
				static bool ProcessFileCompress(Connection* Base, uint64_t ContentLength, uint64_t Range, bool Gzip);
				static bool ProcessGateway(Connection* Base);
				static bool ProcessWebSocket(Connection* Base, const char* Key);
				static bool ProcessWebSocketPass(Connection* Base);
			};

			class TH_OUT Server : public SocketServer
			{
				friend Connection;
				friend Util;

			public:
				Server();
				virtual ~Server() override;

			private:
				bool OnConfigure(SocketRouter* New) override;
				bool OnRequestEnded(SocketConnection* Base, bool Check) override;
				bool OnRequestBegin(SocketConnection* Base) override;
				bool OnDeallocate(SocketConnection* Base) override;
				bool OnDeallocateRouter(SocketRouter* Base) override;
				bool OnListen(Rest::EventQueue* Loop) override;
				bool OnUnlisten() override;
				SocketConnection* OnAllocate(Listener* Host, Socket* Stream) override;
				SocketRouter* OnAllocateRouter() override;
			};

			class TH_OUT Client : public SocketClient
			{
			private:
				RequestFrame Request;
				ResponseFrame Response;

			public:
				Client(int64_t ReadTimeout);
				virtual ~Client() override;
				bool Send(HTTP::RequestFrame* Root, const ResponseCallback& Callback);
				bool Consume(int64_t MaxSize, const ResponseCallback& Callback);
				RequestFrame* GetRequest();
				ResponseFrame* GetResponse();

			private:
				bool Receive();
			};
		}
	}
}
#endif