#ifndef THAWK_NETWORK_HTTP_H
#define THAWK_NETWORK_HTTP_H

#include "../network.h"
#include "../script.h"

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
            typedef std::function<bool(struct Connection*, const char*, Int64)> ContentCallback;
            typedef std::function<bool(struct Connection*, struct Resource*, Int64)> ResourceCallback;
            typedef std::function<bool(struct Connection*, struct Credentials*, const std::string&)> AuthorizeCallback;
            typedef std::function<bool(struct Connection*, Rest::Stroke*)> HeaderCallback;
            typedef std::function<void(struct WebSocketFrame*)> WebSocketCallback;
            typedef std::function<void(struct WebSocketFrame*, const char*, Int64, enum WebSocketOp)> WebSocketReadCallback;
            typedef std::function<bool(struct GatewayFrame*, const char*, UInt64, char)> GatewayNextCallback;
            typedef std::function<bool(struct GatewayFrame*, void*)> GatewayFreeCallback;
            typedef std::function<bool(void**, struct SiteEntry*)> GatewayCreateCallback;
            typedef std::function<bool(void**, struct SiteEntry*)> GatewayReleaseCallback;
            typedef std::function<bool(class Parser*, Int64)> ParserCodeCallback;
            typedef std::function<bool(class Parser*, const char*, Int64)> ParserDataCallback;
            typedef std::function<bool(class Parser*)> ParserNotifyCallback;
            typedef std::function<void(class Client*, struct RequestFrame*, struct ResponseFrame*)> ResponseCallback;

            struct Connection;

            struct RouteEntry;

			struct SiteEntry;

			struct MapRouter;

            class Server;

            class Query;

            struct THAWK_OUT GatewayFile
            {
                Compute::RegExp Value;
                bool Core = false;
            };

            struct THAWK_OUT ErrorFile
            {
                std::string Pattern;
                int StatusCode = 0;
            };

            struct THAWK_OUT MimeType
            {
                std::string Extension;
                std::string Type;
            };

            struct THAWK_OUT MimeStatic
            {
                const char* Extension = nullptr;
                const char* Type = nullptr;

                MimeStatic(const char* Ext, const char* T);
            };

            struct THAWK_OUT Credentials
            {
                std::string Username;
                std::string Password;
                Auth Type = Auth_Unverified;
            };

            struct THAWK_OUT Header
            {
                std::string Key;
                std::string Value;
            };

            struct THAWK_OUT Resource
            {
                std::vector<Header> Headers;
                std::string Path;
                std::string Type;
                std::string Name;
                std::string Key;
                UInt64 Length = 0;
                bool Memory = false;

                void SetHeader(const char* Key, const char* Value);
                void SetHeader(const char* Key, const std::string& Value);
                const char* GetHeader(const char* Key);
            };

            struct THAWK_OUT Cookie
            {
                std::string Name;
                std::string Value;
                std::string Domain;
                std::string Path;
                UInt64 Expires = 0;
                bool Secure = false;
            };

            struct THAWK_OUT RequestFrame
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
                UInt64 ContentLength = 0;

                void SetHeader(const char* Key, const char* Value);
                void SetHeader(const char* Key, const std::string& Value);
                const char* GetCookie(const char* Key);
                const char* GetHeader(const char* Key);
                std::vector<std::pair<Int64, Int64>> GetRanges();
                std::pair<UInt64, UInt64> GetRange(std::vector<std::pair<Int64, Int64>>::iterator Range, UInt64 ContentLength);
            };

            struct THAWK_OUT ResponseFrame
            {
                std::vector<Cookie> Cookies;
                std::vector<Header> Headers;
                std::string Buffer;
                int StatusCode = -1;

                void SetHeader(const char* Key, const char* Value);
                void SetHeader(const char* Key, const std::string& Value);
                void SetCookie(const char* Key, const char* Value, const char* Domain, const char* Path, UInt64 Expires, bool Secure);
                const char* GetHeader(const char* Key);
                Cookie* GetCookie(const char* Key);
            };

            struct THAWK_OUT WebSocketFrame
            {
                friend struct GatewayFrame;
                friend class Util;

            private:
                std::string Buffer;
                UInt64 BodyLength = 0;
                UInt64 MaskLength = 0;
                UInt64 HeaderLength = 0;
                UInt64 DataLength = 0;
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
                void Write(const char* Buffer, Int64 Length, WebSocketOp OpCode, const SuccessCallback& Callback);
                void Finish();
                void Next();
                void Notify();
				bool IsFinished();
            };

            struct THAWK_OUT GatewayFrame
            {
                friend WebSocketFrame;
                friend class Util;

            private:
                GatewayNextCallback Callback;
                GatewayFreeCallback Destroy;
                char* Buffer = nullptr;
                UInt64 Size = 0, i = 0;
                UInt64 Offset = 0;
                bool EoF = false;
                bool Core = false;
				bool Save = false;
				char Type = '?';

            public:
                Connection* Base = nullptr;
                void* Manager = nullptr;
                void* Device = nullptr;

            public:
                bool Next();
                bool Finish(const GatewayFreeCallback& Callback);
                bool Error(const GatewayFreeCallback& Callback);
				bool IsDone();

            private:
                bool Done(bool Normal);
            };

            struct THAWK_OUT ParserFrame
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

            struct THAWK_OUT QueryToken
            {
                char* Value = nullptr;
                UInt64 Length = 0;
            };

            struct THAWK_OUT RouteEntry
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
                    GatewayNextCallback Gateway;
                    AuthorizeCallback Authorize;
                } Callbacks;
                struct
                {
                    std::vector<GatewayFile> Files;
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
                    UInt64 MinLength = 16384;
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
                UInt64 WebSocketTimeout = 30000;
                UInt64 StaticFileMaxAge = 604800;
                UInt64 MaxCacheLength = 16384;
                UInt64 GracefulTimeWait = 1;
                bool AllowDirectoryListing = true;
                bool AllowWebSocket = false;
                bool AllowSendFile = false;
                Compute::RegExp URI;
                SiteEntry* Site = nullptr;
            };

			struct THAWK_OUT SiteEntry
			{
				struct
				{
					struct
					{
						std::string DocumentRoot = "./sessions/";
						std::string Name = "Sid";
						std::string Domain;
						std::string Path = "/";
						UInt64 Expires = 604800;
						UInt64 CookieExpires = 31536000;
					} Session;

					void* Manager = nullptr;
                    std::string ModuleRoot;
					bool Enabled = false;
				} Gateway;
				struct
				{
					SuccessCallback OnRewriteURL;
					GatewayCreateCallback OnGatewayCreate;
					GatewayReleaseCallback OnGatewayRelease;
				} Callbacks;

				std::unordered_set<std::string> Hosts;
				std::vector<RouteEntry*> Routes;
				std::string ResourceRoot = "./files/";
				std::string SiteName;
				UInt64 MaxResources = 5;
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

            struct THAWK_OUT MapRouter : public SocketRouter
            {
                std::vector<SiteEntry*> Sites;

                MapRouter();
                ~MapRouter();
				SiteEntry* Site(const char* Host);
            };

            struct THAWK_OUT Connection : public SocketConnection
            {
                Rest::Resource Resource;
                WebSocketFrame* WebSocket = nullptr;
                GatewayFrame* Gateway = nullptr;
                RouteEntry* Route = nullptr;
				Server* Root = nullptr;
                RequestFrame Request;
                ResponseFrame Response;

                virtual ~Connection();
                bool Finish();
                bool Finish(int StatusCode);
                bool Certify(Certificate* Output);
                bool Consume(const ContentCallback& Callback = nullptr);
                bool Store(const ResourceCallback& Callback = nullptr);
            };

            class THAWK_OUT QueryParameter : public Rest::Document
            {
            public:
                std::string Build();
                std::string BuildFromBase();
                QueryParameter* Find(QueryToken* Name);
            };

            class THAWK_OUT Query : public Rest::Object
            {
            public:
                QueryParameter* Object;

            public:
                Query();
                ~Query();
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

            class THAWK_OUT Session : public Rest::Object
            {
            public:
                Rest::Document* Query = nullptr;
                std::string SessionId;
                Int64 SessionExpires = 0;
                bool IsNewSession = false;

            public:
                Session();
                ~Session();
                void Clear();
                bool Write(Connection* Base);
                bool Read(Connection* Base);

            private:
                std::string& FindSessionId(Connection* Base);
                std::string& GenerateSessionId(Connection* Base);

            public:
                static bool InvalidateCache(const std::string& Path);
            };

            class THAWK_OUT Parser : public Rest::Object
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
                    Int64 Index = 0, Length = 0;
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
                ~Parser();
                Int64 MultipartParse(const char* Boundary, const char* Buffer, Int64 Length);
                Int64 ParseRequest(const char* BufferStart, UInt64 Length, UInt64 LastLength);
                Int64 ParseResponse(const char* BufferStart, UInt64 Length, UInt64 LastLength);
                Int64 ParseDecodeChunked(char* Buffer, Int64* BufferLength);

            private:
                const char* Tokenize(const char* Buffer, const char* BufferEnd, const char** Token, UInt64* TokenLength, int* Out);
                const char* Complete(const char* Buffer, const char* BufferEnd, UInt64 LastLength, int* Out);
                const char* ProcessVersion(const char* Buffer, const char* BufferEnd, int* Out);
                const char* ProcessHeaders(const char* Buffer, const char* BufferEnd, int* Out);
                const char* ProcessRequest(const char* Buffer, const char* BufferEnd, int* Out);
                const char* ProcessResponse(const char* Buffer, const char* BufferEnd, int* Out);
            };

            class THAWK_OUT Util
            {
            public:
                static void ConstructPath(Connection* Base);
                static void ConstructHeadFull(RequestFrame* Request, ResponseFrame* Response, bool IsRequest, Rest::Stroke* Buffer);
                static void ConstructHeadCache(Connection* Base, Rest::Stroke* Buffer);
                static void ConstructHeadUncache(Connection* Base, Rest::Stroke* Buffer);
                static bool ConstructRoute(MapRouter* Router, Connection* Base);
                static bool WebSocketWrite(Connection* Base, const char* Buffer, Int64 Length, WebSocketOp Type, const SuccessCallback& Callback);
                static bool WebSocketWriteMask(Connection* Base, const char* Buffer, Int64 Length, WebSocketOp Type, unsigned int Mask, const SuccessCallback& Callback);
                static bool ConstructDirectoryEntries(const Rest::ResourceEntry& A, const Rest::ResourceEntry& B);
                static std::string ConnectionResolve(Connection* Base);
                static std::string ConstructContentRange(UInt64 Offset, UInt64 Length, UInt64 ContenLength);
                static const char* ContentType(const std::string& Path, std::vector<MimeType>* MimeTypes);
                static const char* StatusMessage(int StatusCode);

            public:
                static bool ParseMultipartHeaderField(Parser* Parser, const char* Name, UInt64 Length);
                static bool ParseMultipartHeaderValue(Parser* Parser, const char* Name, UInt64 Length);
                static bool ParseMultipartContentData(Parser* Parser, const char* Name, UInt64 Length);
                static bool ParseMultipartResourceBegin(Parser* Parser);
                static bool ParseMultipartResourceEnd(Parser* Parser);
                static bool ParseHeaderField(Parser* Parser, const char* Name, UInt64 Length);
                static bool ParseHeaderValue(Parser* Parser, const char* Name, UInt64 Length);
                static bool ParseVersion(Parser* Parser, const char* Name, UInt64 Length);
                static bool ParseStatusCode(Parser* Parser, UInt64 Length);
                static bool ParseMethodValue(Parser* Parser, const char* Name, UInt64 Length);
                static bool ParsePathValue(Parser* Parser, const char* Name, UInt64 Length);
                static bool ParseQueryValue(Parser* Parser, const char* Name, UInt64 Length);
                static int ParseContentRange(const char* ContentRange, Int64* Range1, Int64* Range2);
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
                static bool ResourceCompressed(Connection* Base, UInt64 Size);

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
                static bool ProcessResourceCompress(Connection* Base, bool Deflate, bool Gzip, const char* ContentRange, UInt64 Range);
                static bool ProcessResourceCache(Connection* Base);
                static bool ProcessFile(Connection* Base, UInt64 ContentLength, UInt64 Range);
                static bool ProcessFileCompress(Connection* Base, UInt64 ContentLength, UInt64 Range, bool Gzip);
                static bool ProcessGateway(Connection* Base);
                static bool ProcessWebSocket(Connection* Base, const char* Key);
                static bool ProcessWebSocketPass(Connection* Base);
            };

            class THAWK_OUT Server : public SocketServer
            {
                friend Connection;
                friend Util;

            public:
                Server();
                virtual ~Server();

            private:
                bool OnConfigure(SocketRouter* New);
                bool OnRequestEnded(SocketConnection* Base, bool Check);
                bool OnRequestBegin(SocketConnection* Base);
                bool OnDeallocate(SocketConnection* Base);
                bool OnDeallocateRouter(SocketRouter* Base);
                bool OnListen(Rest::EventQueue* Loop);
                bool OnUnlisten();
                SocketConnection* OnAllocate(Listener* Host, Socket* Stream);
                SocketRouter* OnAllocateRouter();

            private:
                static bool Handle(Socket* Connection, const char* Buffer, Int64 Size);
            };

            class THAWK_OUT Client : public SocketClient
            {
            private:
                RequestFrame Request;
                ResponseFrame Response;

            public:
                Client(Int64 ReadTimeout);
                ~Client();
                bool Send(HTTP::RequestFrame* Root, const ResponseCallback& Callback);
				bool Consume(Int64 MaxSize, const ResponseCallback& Callback);
				RequestFrame* GetRequest();
				ResponseFrame* GetResponse();

            private:
                bool Receive();
            };
        }
    }
}
#endif
