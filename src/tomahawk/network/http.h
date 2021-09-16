#ifndef TH_NETWORK_HTTP_H
#define TH_NETWORK_HTTP_H
#define TH_HTTP_PAYLOAD (1024 * 64)
#include "../core/network.h"
#include "../core/script.h"

namespace Tomahawk
{
	namespace Network
	{
		namespace HTTP
		{
			enum class Auth
			{
				Granted,
				Denied,
				Unverified
			};

			enum class Content
			{
				Not_Loaded,
				Lost,
				Cached,
				Empty,
				Corrupted,
				Payload_Exceeded,
				Wants_Save,
				Saved,
				Save_Exception
			};

			enum class QueryValue
			{
				Unknown,
				Number,
				String,
				Boolean,
				Object
			};

			enum class WebSocketOp
			{
				Continue = 0x00,
				Text = 0x01,
				Binary = 0x02,
				Close = 0x08,
				Ping = 0x09,
				Pong = 0x0A
			};

			enum class WebSocketState
			{
				Open,
				Receive,
				Process,
				Close
			};

			enum class CompressionTune
			{
				Filtered = 1,
				Huffman = 2,
				Rle = 3,
				Fixed = 4,
				Default = 0
			};

			enum class RouteMode
			{
				Start,
				Match,
				End
			};

			typedef std::vector<std::string> RangePayload;
			typedef std::map<std::string, RangePayload, struct HeaderComparator> HeaderMapping;
			typedef std::function<bool(struct Connection*)> SuccessCallback;
			typedef std::function<void(struct Connection*, const char*)> MessageCallback;
			typedef std::function<bool(struct Connection*, const char*, int64_t)> ContentCallback;
			typedef std::function<bool(struct Connection*, struct Resource*, int64_t)> ResourceCallback;
			typedef std::function<bool(struct Connection*, struct Credentials*, const std::string&)> AuthorizeCallback;
			typedef std::function<bool(struct Connection*, Core::Parser*)> HeaderCallback;
			typedef std::function<bool(struct Connection*, Script::VMCompiler*)> CompilerCallback;
			typedef std::function<void(struct WebSocketFrame*)> WebSocketCallback;
			typedef std::function<void(struct WebSocketFrame*, const char*, int64_t, enum WebSocketOp)> WebSocketReadCallback;
			typedef std::function<bool(struct WebSocketFrame*)> WebSocketCheckCallback;
			typedef std::function<void(struct GatewayFrame*)> GatewayCallback;
			typedef std::function<void(struct GatewayFrame*, int, const char*)> GatewayStatusCallback;
			typedef std::function<bool(struct GatewayFrame*)> GatewayCloseCallback;
			typedef std::function<bool(class Parser*, int64_t)> ParserCodeCallback;
			typedef std::function<bool(class Parser*, const char*, int64_t)> ParserDataCallback;
			typedef std::function<bool(class Parser*)> ParserNotifyCallback;

			struct Connection;

			struct RouteEntry;

			struct SiteEntry;

			struct MapRouter;

			class Server;

			class Query;

			class WebCodec;

			struct TH_OUT HeaderComparator : std::binary_function<std::string, std::string, bool>
			{
				struct Insensitive : public std::binary_function<unsigned char, unsigned char, bool>
				{
					bool operator() (const unsigned char& c1, const unsigned char& c2) const
					{
						return tolower(c1) < tolower(c2);
					}
				};

				bool operator() (const std::string& s1, const std::string& s2) const
				{
					return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), Insensitive());
				}
			};

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
				Auth Type = Auth::Unverified;
			};

			struct TH_OUT Resource
			{
				HeaderMapping Headers;
				std::string Path;
				std::string Type;
				std::string Name;
				std::string Key;
				uint64_t Length = 0;
				bool Memory = false;

				void PutHeader(const std::string& Key, const std::string& Value);
				void SetHeader(const std::string& Key, const std::string& Value);
				RangePayload* GetHeaderRanges(const std::string& Key);
				const std::string* GetHeaderBlob(const std::string& Key) const;
				const char* GetHeader(const std::string& Key) const;
			};

			struct TH_OUT Cookie
			{
				std::string Name;
				std::string Value;
				std::string Domain;
				std::string Path;
				uint64_t Expires = 0;
				bool Secure = false;
				bool HTTPOnly = false;
			};

			struct TH_OUT RequestFrame
			{
				HeaderMapping Cookies;
				HeaderMapping Headers;
				std::vector<Resource> Resources;
				std::string Buffer;
				std::string Query;
				std::string Path;
				std::string URI;
				std::string Where;
				Compute::RegexResult Match;
				Credentials User;
				char RemoteAddress[48] = { 0 };
				char Method[10] = { 'G', 'E', 'T' };
				char Version[10] = { 'H', 'T', 'T', 'P', '/', '1', '.', '1' };
				uint64_t ContentLength = 0;

				void SetMethod(const char* Value);
				void SetVersion(unsigned int Major, unsigned int Minor);
				void PutHeader(const std::string& Key, const std::string& Value);
				void SetHeader(const std::string& Key, const std::string& Value);
				RangePayload* GetCookieRanges(const std::string& Key);
				std::string* GetCookieBlob(const std::string& Key) const;
				const char* GetCookie(const std::string& Key) const;
				RangePayload* GetHeaderRanges(const std::string& Key);
				const std::string* GetHeaderBlob(const std::string& Key) const;
				const char* GetHeader(const std::string& Key) const;
				std::vector<std::pair<int64_t, int64_t>> GetRanges() const;
				std::pair<uint64_t, uint64_t> GetRange(std::vector<std::pair<int64_t, int64_t>>::iterator Range, uint64_t ContentLength) const;
			};

			struct TH_OUT ResponseFrame
			{
				HeaderMapping Headers;
				std::vector<Cookie> Cookies;
				std::vector<char> Buffer;
				Content Data = Content::Not_Loaded;
				int StatusCode = -1;
				bool Error = false;

				void PutBuffer(const std::string& Data);
				void SetBuffer(const std::string& Data);
				void PutHeader(const std::string& Key, const std::string& Value);
				void SetHeader(const std::string& Key, const std::string& Value);
				void SetCookie(const char* Key, const char* Value, uint64_t Expires = 0, const char* Domain = nullptr, const char* Path = nullptr, bool Secure = false, bool HTTPOnly = false);
				Cookie* GetCookie(const char* Key);
				RangePayload* GetHeaderRanges(const std::string& Key);
				const std::string* GetHeaderBlob(const std::string& Key) const;
				const char* GetHeader(const std::string& Key) const;
				std::string GetBuffer() const;
				bool HasBody() const;
				bool IsOK() const;
			};

			struct TH_OUT WebSocketFrame
			{
				friend struct GatewayFrame;
				friend struct Connection;
				friend class Util;

			public:
				struct
				{
					WebSocketCallback Reset;
					WebSocketCallback Close;
					WebSocketCheckCallback Dead;
				} E;

			private:
				std::atomic<uint32_t> State;
				std::atomic<bool> Active;
				std::atomic<bool> Reset;
				std::mutex Section;
				Socket* Stream;
				WebCodec* Codec;

			public:
				WebSocketCallback Connect;
				WebSocketCallback Disconnect;
				WebSocketCallback Notification;
				WebSocketReadCallback Receive;

			public:
				WebSocketFrame(Socket* NewStream);
				~WebSocketFrame();
				void Send(const char* Buffer, int64_t Length, WebSocketOp OpCode, const WebSocketCallback& Callback);
				void Send(unsigned int Mask, const char* Buffer, int64_t Length, WebSocketOp OpCode, const WebSocketCallback& Callback);
				void Finish();
				void Next();
				bool IsFinished();
			};

			struct TH_OUT GatewayFrame
			{
				friend WebSocketFrame;
				friend class Util;

			public:
				struct
				{
					GatewayStatusCallback Status;
					GatewayCloseCallback Close;
					GatewayCallback Exception;
					GatewayCallback Finish;
				} E;

			private:
				Script::VMCompiler* Compiler;
				std::atomic<bool> Active;

			public:
				GatewayFrame(Script::VMCompiler* NewCompiler);
				void Execute(Script::VMContext*, Script::VMPoll State);
				bool Start(const std::string& Path, const char* Method, char* Buffer, int64_t Size);
				bool Error(int StatusCode, const char* Text);
				bool Finish();
				bool IsFinished();
				bool GetException(const char** Exception, const char** Function, int* Line, int* Column);
				Script::VMContext* GetContext();
				Script::VMCompiler* GetCompiler();
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
					CompilerCallback Compiler;
					AuthorizeCallback Authorize;
				} Callbacks;
				struct
				{
					std::vector<Compute::RegexSource> Files;
					std::vector<std::string> Methods;
					bool ReportErrors = false;
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
					std::vector<Compute::RegexSource> Files;
					CompressionTune Tune = CompressionTune::Default;
					int QualityLevel = 8;
					int MemoryLevel = 8;
					uint64_t MinLength = 16384;
					bool Enabled = false;
				} Compression;

				std::vector<Compute::RegexSource> HiddenFiles;
				std::vector<ErrorFile> ErrorFiles;
				std::vector<MimeType> MimeTypes;
				std::vector<std::string> IndexFiles;
				std::vector<std::string> DisallowedMethods;
				std::string DocumentRoot = "./";
				std::string CharSet = "utf-8";
				std::string ProxyIpAddress = "";
				std::string AccessControlAllowOrigin;
				std::string Redirect;
				std::string Override;
				uint64_t WebSocketTimeout = 30000;
				uint64_t StaticFileMaxAge = 604800;
				uint64_t MaxCacheLength = 16384;
				uint64_t Level = 0;
				bool AllowDirectoryListing = true;
				bool AllowWebSocket = false;
				bool AllowSendFile = false;
				Compute::RegexSource URI;
				SiteEntry* Site = nullptr;
			};

			struct TH_OUT RouteGroup
			{
				std::vector<RouteEntry*> Routes;
				std::string Match;
				RouteMode Mode = RouteMode::Start;
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

					bool Verify = false;
					bool Enabled = false;
				} Gateway;
				struct
				{
					SuccessCallback OnRewriteURL;
				} Callbacks;

				std::vector<RouteGroup> Groups;
				std::string ResourceRoot = "./files/";
				uint64_t MaxResources = 5;
				RouteEntry* Base = nullptr;
				MapRouter* Router = nullptr;

				SiteEntry();
				SiteEntry(const SiteEntry&) = delete;
				SiteEntry(SiteEntry&&) = delete;
				~SiteEntry();
				void Sort();
				RouteGroup* Group(const std::string& Match, RouteMode Mode);
				RouteEntry* Route(const std::string& Match, RouteMode Mode, const std::string& Pattern);
				RouteEntry* Route(const std::string& Pattern, RouteGroup* Group, RouteEntry* From);
				bool Remove(RouteEntry* Source);
				bool Get(const char* Pattern, const SuccessCallback& Callback);
				bool Get(const std::string& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Post(const char* Pattern, const SuccessCallback& Callback);
				bool Post(const std::string& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Put(const char* Pattern, const SuccessCallback& Callback);
				bool Put(const std::string& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Patch(const char* Pattern, const SuccessCallback& Callback);
				bool Patch(const std::string& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Delete(const char* Pattern, const SuccessCallback& Callback);
				bool Delete(const std::string& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Options(const char* Pattern, const SuccessCallback& Callback);
				bool Options(const std::string& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Access(const char* Pattern, const SuccessCallback& Callback);
				bool Access(const std::string& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool WebSocketConnect(const char* Pattern, const WebSocketCallback& Callback);
				bool WebSocketConnect(const std::string& Match, RouteMode Mode, const char* Pattern, const WebSocketCallback& Callback);
				bool WebSocketDisconnect(const char* Pattern, const WebSocketCallback& Callback);
				bool WebSocketDisconnect(const std::string& Match, RouteMode Mode, const char* Pattern, const WebSocketCallback& Callback);
				bool WebSocketReceive(const char* Pattern, const WebSocketReadCallback& Callback);
				bool WebSocketReceive(const std::string& Match, RouteMode Mode, const char* Pattern, const WebSocketReadCallback& Callback);
			};

			struct TH_OUT MapRouter final : public SocketRouter
			{
				std::unordered_map<std::string, SiteEntry*> Sites;
				std::string ModuleRoot;
				Script::VMManager* VM;

				MapRouter();
				virtual ~MapRouter() override;
				SiteEntry* Site(const char* Host);
			};

			struct TH_OUT Connection final : public SocketConnection
			{
				Core::Resource Resource;
				WebSocketFrame* WebSocket = nullptr;
				GatewayFrame* Gateway = nullptr;
				RouteEntry* Route = nullptr;
				Server* Root = nullptr;
				RequestFrame Request;
				ResponseFrame Response;

				virtual ~Connection() = default;
				bool Finish() override;
				bool Finish(int StatusCode) override;
				bool Certify(Certificate* Output) override;
				bool Consume(const ContentCallback& Callback = nullptr);
				bool Store(const ResourceCallback& Callback = nullptr);
			};

			class TH_OUT QueryParameter final : public Core::Document
			{
			public:
				QueryParameter();
				virtual ~QueryParameter() = default;
				std::string Build();
				std::string BuildFromBase();
				QueryParameter* Find(QueryToken* Name);
			};

			class TH_OUT Query : public Core::Object
			{
			public:
				QueryParameter* Object;

			public:
				Query();
				virtual ~Query() override;
				void Clear();
				void Steal(Core::Document** Output);
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

			class TH_OUT Session : public Core::Object
			{
			public:
				Core::Document* Query = nullptr;
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

			class TH_OUT Parser : public Core::Object
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

			class TH_OUT WebCodec : public Core::Object
			{
			public:
				typedef std::queue<std::pair<WebSocketOp, std::vector<char>>> MessageQueue;

			private:
				enum class Bytecode
				{
					Begin = 0,
					Length,
					Length_16_0,
					Length_16_1,
					Length_64_0,
					Length_64_1,
					Length_64_2,
					Length_64_3,
					Length_64_4,
					Length_64_5,
					Length_64_6,
					Length_64_7,
					Mask_0,
					Mask_1,
					Mask_2,
					Mask_3,
					End
				};

			private:
				std::vector<char> Payload;
				WebSocketOp Opcode;
				MessageQueue Queue;
				Bytecode State;
				uint64_t Remains;
				uint8_t Mask[4];
				uint8_t Fragment;
				uint8_t Final;
				uint8_t Control;
				uint8_t Masked;
				uint8_t Masks;

			public:
				std::vector<char> Data;

			public:
				WebCodec();
				bool ParseFrame(const char* Data, size_t Size);
				bool GetFrame(WebSocketOp* Op, std::vector<char>* Message);
			};

			class TH_OUT Util
			{
			public:
				static void ConstructPath(Connection* Base);
				static void ConstructHeadFull(RequestFrame* Request, ResponseFrame* Response, bool IsRequest, Core::Parser* Buffer);
				static void ConstructHeadCache(Connection* Base, Core::Parser* Buffer);
				static void ConstructHeadUncache(Connection* Base, Core::Parser* Buffer);
				static bool ConstructRoute(MapRouter* Router, Connection* Base);
				static bool ConstructDirectoryEntries(const Core::ResourceEntry& A, const Core::ResourceEntry& B);
				static bool ContentOK(Content State);
				static std::string ConnectionResolve(Connection* Base);
				static std::string ConstructContentRange(uint64_t Offset, uint64_t Length, uint64_t ContenLength);
				static const char* ContentType(const std::string& Path, std::vector<MimeType>* MimeTypes);
				static const char* StatusMessage(int StatusCode);

			public:
				static void ParseCookie(const std::string& Value);
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
				static bool ResourceIndexed(Connection* Base, Core::Resource* Resource);
				static bool ResourceProvided(Connection* Base, Core::Resource* Resource);
				static bool ResourceModified(Connection* Base, Core::Resource* Resource);
				static bool ResourceCompressed(Connection* Base, uint64_t Size);

			public:
				static bool RouteWEBSOCKET(Connection* Base);
				static bool RouteGET(Connection* Base);
				static bool RoutePOST(Connection* Base);
				static bool RoutePUT(Connection* Base);
				static bool RoutePATCH(Connection* Base);
				static bool RouteDELETE(Connection* Base);
				static bool RouteOPTIONS(Connection* Base);

			public:
				static bool ProcessDirectory(Connection* Base);
				static bool ProcessResource(Connection* Base);
				static bool ProcessResourceCompress(Connection* Base, bool Deflate, bool Gzip, const char* ContentRange, uint64_t Range);
				static bool ProcessResourceCache(Connection* Base);
				static bool ProcessFile(Connection* Base, uint64_t ContentLength, uint64_t Range);
				static bool ProcessFileChunk(Connection* Base, Server* Router, FILE* Stream, uint64_t ContentLength);
				static bool ProcessFileCompress(Connection* Base, uint64_t ContentLength, uint64_t Range, bool Gzip);
				static bool ProcessFileCompressChunk(Connection* Base, Server* Router, FILE* Stream, void* CStream, uint64_t ContentLength);
				static bool ProcessGateway(Connection* Base);
				static bool ProcessWebSocket(Connection* Base, const char* Key);
			};

			class TH_OUT Server final : public SocketServer
			{
				friend GatewayFrame;
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
				bool OnListen() override;
				bool OnUnlisten() override;
				SocketConnection* OnAllocate(Listener* Host, Socket* Stream) override;
				SocketRouter* OnAllocateRouter() override;
			};

			class TH_OUT Client final : public SocketClient
			{
			private:
				WebSocketFrame * WebSocket;
				RequestFrame Request;
				ResponseFrame Response;
				Core::Async<bool> Future;

			public:
				Client(int64_t ReadTimeout);
				virtual ~Client() override;
				bool Downgrade();
				Core::Async<bool> Consume(int64_t MaxSize = TH_HTTP_PAYLOAD);
				Core::Async<bool> Fetch(HTTP::RequestFrame&& Root, int64_t MaxSize = TH_HTTP_PAYLOAD);
				Core::Async<bool> Upgrade(HTTP::RequestFrame&& Root);
				Core::Async<ResponseFrame*> Send(HTTP::RequestFrame&& Root);
				Core::Async<Core::Document*> JSON(HTTP::RequestFrame&& Root, int64_t MaxSize = TH_HTTP_PAYLOAD);
				Core::Async<Core::Document*> XML(HTTP::RequestFrame&& Root, int64_t MaxSize = TH_HTTP_PAYLOAD);
				WebSocketFrame* GetWebSocket();
				RequestFrame* GetRequest();
				ResponseFrame* GetResponse();

			private:
				bool Receive();
			};
		}
	}
}
#endif