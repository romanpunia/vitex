#ifndef ED_NETWORK_HTTP_H
#define ED_NETWORK_HTTP_H
#define ED_HTTP_PAYLOAD (1024 * 64)
#include "../core/network.h"
#include "../core/script.h"

namespace Edge
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
			typedef std::function<bool(class Connection*)> SuccessCallback;
			typedef std::function<bool(class Connection*, SocketPoll, const char*, size_t)> ContentCallback;
			typedef std::function<bool(class Connection*, struct Credentials*)> AuthorizeCallback;
			typedef std::function<bool(class Connection*, Core::Parser*)> HeaderCallback;
			typedef std::function<bool(class Connection*, Script::VMCompiler*)> CompilerCallback;
			typedef std::function<bool(struct Resource*)> ResourceCallback;
			typedef std::function<void(class WebSocketFrame*)> WebSocketCallback;
			typedef std::function<bool(class WebSocketFrame*, WebSocketOp, const char*, size_t)> WebSocketReadCallback;
			typedef std::function<bool(class WebSocketFrame*)> WebSocketCheckCallback;
			typedef std::function<void(class GatewayFrame*)> GatewayCallback;
			typedef std::function<void(class GatewayFrame*, int, const char*)> GatewayStatusCallback;
			typedef std::function<bool(class GatewayFrame*)> GatewayCloseCallback;
			typedef std::function<bool(class Parser*, size_t)> ParserCodeCallback;
			typedef std::function<bool(class Parser*, const char*, size_t)> ParserDataCallback;
			typedef std::function<bool(class Parser*)> ParserNotifyCallback;

			class Connection;

			class RouteEntry;

			class SiteEntry;

			class MapRouter;

			class Server;

			class Query;

			class WebCodec;

			struct ED_OUT HeaderComparator
			{
				struct Insensitive
				{
					bool operator() (const unsigned char& A, const unsigned char& B) const
					{
						return tolower(A) < tolower(B);
					}
				};

				bool operator() (const std::string& Left, const std::string& Right) const
				{
					return std::lexicographical_compare(Left.begin(), Left.end(), Right.begin(), Right.end(), Insensitive());
				}
			};

			struct ED_OUT ErrorFile
			{
				std::string Pattern;
				int StatusCode = 0;
			};

			struct ED_OUT MimeType
			{
				std::string Extension;
				std::string Type;
			};

			struct ED_OUT MimeStatic
			{
				const char* Extension = nullptr;
				const char* Type = nullptr;

				MimeStatic(const char* Ext, const char* T);
			};

			struct ED_OUT Credentials
			{
				std::string Token;
				Auth Type = Auth::Unverified;
			};

			struct ED_OUT Resource
			{
				HeaderMapping Headers;
				std::string Path;
				std::string Type;
				std::string Name;
				std::string Key;
				size_t Length = 0;
				bool Memory = false;

				void PutHeader(const std::string& Key, const std::string& Value);
				void SetHeader(const std::string& Key, const std::string& Value);
				std::string ComposeHeader(const std::string& Key) const;
				RangePayload* GetHeaderRanges(const std::string& Key);
				const std::string* GetHeaderBlob(const std::string& Key) const;
				const char* GetHeader(const std::string& Key) const;
			};

			struct ED_OUT Cookie
			{
				std::string Name;
				std::string Value;
				std::string Domain;
				std::string Path = "/";
				std::string SameSite;
				std::string Expires;
				bool Secure = false;
				bool HttpOnly = false;

				void SetExpires(int64_t Time);
				void SetExpired();
			};

			struct ED_OUT RequestFrame
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
				char Method[10] = { 'G', 'E', 'T' };
				char Version[10] = { 'H', 'T', 'T', 'P', '/', '1', '.', '1' };
				size_t ContentLength = 0;

				void SetMethod(const char* Value);
				void SetVersion(unsigned int Major, unsigned int Minor);
				void PutHeader(const std::string& Key, const std::string& Value);
				void SetHeader(const std::string& Key, const std::string& Value);
				std::string ComposeHeader(const std::string& Key) const;
				RangePayload* GetCookieRanges(const std::string& Key);
				std::string* GetCookieBlob(const std::string& Key) const;
				const char* GetCookie(const std::string& Key) const;
				RangePayload* GetHeaderRanges(const std::string& Key);
				const std::string* GetHeaderBlob(const std::string& Key) const;
				const char* GetHeader(const std::string& Key) const;
				std::vector<std::pair<size_t, size_t>> GetRanges() const;
				std::pair<size_t, size_t> GetRange(std::vector<std::pair<size_t, size_t>>::iterator Range, size_t ContentLength) const;
			};

			struct ED_OUT ResponseFrame
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
				void SetCookie(const Cookie& Value);
				void SetCookie(Cookie&& Value);
				std::string ComposeHeader(const std::string& Key) const;
				Cookie* GetCookie(const char* Key);
				RangePayload* GetHeaderRanges(const std::string& Key);
				const std::string* GetHeaderBlob(const std::string& Key) const;
				const char* GetHeader(const std::string& Key) const;
				std::string GetBuffer() const;
				bool HasBody() const;
				bool IsOK() const;
			};

			class ED_OUT RouteGroup : public Core::Object
			{
			public:
				std::vector<RouteEntry*> Routes;
				std::string Match;
				RouteMode Mode;

			public:
				RouteGroup(const std::string& NewMatch, RouteMode NewMode) noexcept;
				virtual ~RouteGroup() noexcept override;
			};

			class ED_OUT WebSocketFrame : public Core::Object
			{
				friend class GatewayFrame;
				friend class Connection;
				friend class Util;

			private:
				struct Message
				{
					unsigned int Mask;
					char* Buffer;
					size_t Size;
					WebSocketOp Opcode;
					WebSocketCallback Callback;
				};

			public:
				struct
				{
					WebSocketCallback Reset;
					WebSocketCallback Close;
					WebSocketCheckCallback Dead;
				} Lifetime;

			private:
				std::queue<Message> Messages;
				std::atomic<uint32_t> State;
				std::atomic<bool> Active;
				std::atomic<bool> Reset;
				std::atomic<bool> Deadly;
				std::atomic<bool> Busy;
				std::mutex Section;
				Socket* Stream;
				WebCodec* Codec;

			public:
				WebSocketCallback Connect;
				WebSocketCallback BeforeDisconnect;
				WebSocketCallback Disconnect;
				WebSocketReadCallback Receive;

			public:
				WebSocketFrame(Socket* NewStream);
				virtual ~WebSocketFrame() override;
				void Send(const char* Buffer, size_t Length, WebSocketOp OpCode, const WebSocketCallback& Callback);
				void Send(unsigned int Mask, const char* Buffer, size_t Length, WebSocketOp OpCode, const WebSocketCallback& Callback);
				void Finish();
				void Next();
				bool IsFinished();

			private:
				void Finalize();
				void Dequeue();
				void Writeable(bool IsBusy = false, bool Lockup = true);
				bool Enqueue(unsigned int Mask, const char* Buffer, size_t Length, WebSocketOp OpCode, const WebSocketCallback& Callback);
				bool IsWriteable();
				bool IsIgnore();
			};

			class ED_OUT GatewayFrame : public Core::Object
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
				} Lifetime;

			private:
				HTTP::Connection* Base;
				Script::VMCompiler* Compiler;
				std::atomic<bool> Active;

			public:
				GatewayFrame(HTTP::Connection* NewBase, Script::VMCompiler* NewCompiler);
				virtual ~GatewayFrame() = default;
				bool Start(const std::string& Path, const char* Method, char* Buffer, size_t Size);
				bool Error(int StatusCode, const char* Text);
				bool Finish();
				bool IsFinished();
				bool GetException(const char** Exception, const char** Function, int* Line, int* Column);
				Script::VMContext* GetContext();
				Script::VMCompiler* GetCompiler();
				HTTP::Connection* GetBase();
			};

			class ED_OUT RouteEntry : public Core::Object
			{
			public:
				struct
				{
					struct
					{
						SuccessCallback Initiate;
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
					bool ReportStack = false;
					bool ReportErrors = false;
				} Gateway;
				struct
				{
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
					size_t MinLength = 16384;
					bool Enabled = false;
				} Compression;

			public:
				std::vector<Compute::RegexSource> HiddenFiles;
				std::vector<ErrorFile> ErrorFiles;
				std::vector<MimeType> MimeTypes;
				std::vector<std::string> IndexFiles;
				std::vector<std::string> TryFiles;
				std::vector<std::string> DisallowedMethods;
				std::string DocumentRoot = "./";
				std::string CharSet = "utf-8";
				std::string ProxyIpAddress;
				std::string AccessControlAllowOrigin;
				std::string Redirect;
				std::string Override;
				size_t WebSocketTimeout = 30000;
				size_t StaticFileMaxAge = 604800;
				size_t MaxCacheLength = 16384;
				size_t Level = 0;
				bool AllowDirectoryListing = true;
				bool AllowWebSocket = false;
				bool AllowSendFile = true;
				Compute::RegexSource URI;
				SiteEntry* Site = nullptr;

			public:
				RouteEntry() = default;
				RouteEntry(RouteEntry* Other, const Compute::RegexSource& Source);
				virtual ~RouteEntry() = default;
			};

			class ED_OUT SiteEntry : public Core::Object
			{
			public:
				struct
				{
					struct
					{
						struct
						{
							std::string Name = "sid";
							std::string Domain;
							std::string Path = "/";
							std::string SameSite = "Strict";
							uint64_t Expires = 31536000;
							bool Secure = false;
							bool HttpOnly = true;
						} Cookie;

						std::string DocumentRoot;
						uint64_t Expires = 604800;
					} Session;

					bool Verify = false;
					bool Enabled = false;
				} Gateway;
				struct
				{
					SuccessCallback OnRewriteURL;
				} Callbacks;

			public:
				std::vector<RouteGroup*> Groups;
				std::string ResourceRoot = "./temp";
				size_t MaxResources = 5;
				RouteEntry* Base = nullptr;
				MapRouter* Router = nullptr;

			public:
				SiteEntry();
				virtual ~SiteEntry() override;
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

			class ED_OUT MapRouter final : public SocketRouter
			{
			public:
				std::unordered_map<std::string, SiteEntry*> Sites;
				std::string ModuleRoot;
				Script::VMManager* VM;

			public:
				MapRouter();
				virtual ~MapRouter() override;
				SiteEntry* Site();
				SiteEntry* Site(const char* Host);
			};

			class ED_OUT Connection final : public SocketConnection
			{
			public:
				struct
				{
					HTTP::Parser* Multipart = nullptr;
					HTTP::Parser* Request = nullptr;
				} Parsers;

			public:
				Core::FileEntry Resource;
				WebSocketFrame* WebSocket = nullptr;
				GatewayFrame* Gateway = nullptr;
				RouteEntry* Route = nullptr;
				Server* Root = nullptr;
				RequestFrame Request;
				ResponseFrame Response;

			public:
				Connection(Server* Source) noexcept;
				virtual ~Connection() override;
				void Reset(bool Fully) override;
				bool Finish() override;
				bool Finish(int StatusCode) override;
				bool EncryptionInfo(Certificate* Output) override;
				bool Consume(const ContentCallback& Callback = nullptr, bool Eat = false);
				bool Store(const ResourceCallback& Callback = nullptr, bool Eat = false);
				bool Skip(const SuccessCallback& Callback);
			};

			class ED_OUT Query : public Core::Object
			{
			private:
				struct QueryToken
				{
					char* Value = nullptr;
					size_t Length = 0;
				};

			public:
				Core::Schema* Object;

			public:
				Query();
				virtual ~Query() override;
				void Clear();
				void Steal(Core::Schema** Output);
				void Decode(const char* ContentType, const std::string& URI);
				std::string Encode(const char* ContentType) const;
				Core::Schema* Get(const char* Name) const;
				Core::Schema* Set(const char* Name);
				Core::Schema* Set(const char* Name, const char* Value);

			private:
				void NewParameter(std::vector<QueryToken>* Tokens, const QueryToken& Name, const QueryToken& Value);
				void DecodeAXWFD(const std::string& URI);
				void DecodeAJSON(const std::string& URI);
				std::string EncodeAXWFD() const;
				std::string EncodeAJSON() const;
				Core::Schema* GetParameter(QueryToken* Name);

			private:
				static std::string Build(Core::Schema* Base);
				static std::string BuildFromBase(Core::Schema* Base);
				static Core::Schema* FindParameter(Core::Schema* Base, QueryToken* Name);
			};

			class ED_OUT Session : public Core::Object
			{
			public:
				Core::Schema* Query = nullptr;
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

			class ED_OUT Parser : public Core::Object
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
				struct MultipartData
				{
					char* LookBehind = nullptr;
					char* Boundary = nullptr;
					unsigned char State = MultipartState_Start;
					int64_t Index = 0, Length = 0;
				} Multipart;

				struct ChunkedData
				{
					size_t Length = 0;
					char ConsumeTrailer = 1;
					char HexCount = 0;
					char State = 0;
				} Chunked;

			public:
				struct
				{
					RequestFrame* Request = nullptr;
					ResponseFrame* Response = nullptr;
					RouteEntry* Route = nullptr;
					FILE* Stream = nullptr;
					std::string Header;
					Resource Source;
					ResourceCallback Callback;
					bool Close = false;
					bool Ignore = false;
				} Frame;

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

			public:
				Parser();
				virtual ~Parser() override;
				void PrepareForNextParsing(Connection* Base, bool ForMultipart);
				int64_t MultipartParse(const char* Boundary, const char* Buffer, size_t Length);
				int64_t ParseRequest(const char* BufferStart, size_t Length, size_t LengthLastTime);
				int64_t ParseResponse(const char* BufferStart, size_t Length, size_t LengthLastTime);
				int64_t ParseDecodeChunked(char* Buffer, size_t* BufferLength);

			private:
				const char* IsCompleted(const char* Buffer, const char* BufferEnd, size_t Offset, int* Out);
				const char* Tokenize(const char* Buffer, const char* BufferEnd, const char** Token, size_t* TokenLength, int* Out);
				const char* ProcessVersion(const char* Buffer, const char* BufferEnd, int* Out);
				const char* ProcessHeaders(const char* Buffer, const char* BufferEnd, int* Out);
				const char* ProcessRequest(const char* Buffer, const char* BufferEnd, int* Out);
				const char* ProcessResponse(const char* Buffer, const char* BufferEnd, int* Out);
			};

			class ED_OUT WebCodec : public Core::Object
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

			class ED_OUT_TS Util
			{
			public:
				static std::string ConnectionResolve(Connection* Base);
				static const char* StatusMessage(int StatusCode);
				static const char* ContentType(const std::string& Path, std::vector<MimeType>* MimeTypes);
				static bool ContentOK(Content State);
			};

			class ED_OUT_TS Paths
			{
			public:
				static void ConstructPath(Connection* Base);
				static void ConstructHeadFull(RequestFrame* Request, ResponseFrame* Response, bool IsRequest, Core::Parser* Buffer);
				static void ConstructHeadCache(Connection* Base, Core::Parser* Buffer);
				static void ConstructHeadUncache(Connection* Base, Core::Parser* Buffer);
				static bool ConstructRoute(MapRouter* Router, Connection* Base);
				static bool ConstructDirectoryEntries(Connection* Base, const Core::FileEntry& A, const Core::FileEntry& B);
				static std::string ConstructContentRange(size_t Offset, size_t Length, size_t ContentLength);
			};

			class ED_OUT_TS Parsing
			{
			public:
				static bool ParseMultipartHeaderField(Parser* Parser, const char* Name, size_t Length);
				static bool ParseMultipartHeaderValue(Parser* Parser, const char* Name, size_t Length);
				static bool ParseMultipartContentData(Parser* Parser, const char* Name, size_t Length);
				static bool ParseMultipartResourceBegin(Parser* Parser);
				static bool ParseMultipartResourceEnd(Parser* Parser);
				static bool ParseHeaderField(Parser* Parser, const char* Name, size_t Length);
				static bool ParseHeaderValue(Parser* Parser, const char* Name, size_t Length);
				static bool ParseVersion(Parser* Parser, const char* Name, size_t Length);
				static bool ParseStatusCode(Parser* Parser, size_t Length);
				static bool ParseMethodValue(Parser* Parser, const char* Name, size_t Length);
				static bool ParsePathValue(Parser* Parser, const char* Name, size_t Length);
				static bool ParseQueryValue(Parser* Parser, const char* Name, size_t Length);
				static int ParseContentRange(const char* ContentRange, int64_t* Range1, int64_t* Range2);
				static std::string ParseMultipartDataBoundary();
				static void ParseCookie(const std::string& Value);
			};

			class ED_OUT_TS Permissions
			{
			public:
				static std::string Authorize(const std::string& Username, const std::string& Password, const std::string& Type = "Basic");
				static bool Authorize(Connection* Base);
				static bool MethodAllowed(Connection* Base);
				static bool WebSocketUpgradeAllowed(Connection* Base);
			};

			class ED_OUT_TS Resources
			{
			public:
				static bool ResourceHasAlternative(Connection* Base);
				static bool ResourceHidden(Connection* Base, std::string* Path);
				static bool ResourceIndexed(Connection* Base, Core::FileEntry* Resource);
				static bool ResourceProvided(Connection* Base, Core::FileEntry* Resource);
				static bool ResourceModified(Connection* Base, Core::FileEntry* Resource);
				static bool ResourceCompressed(Connection* Base, size_t Size);
			};

			class ED_OUT_TS Routing
			{
			public:
				static bool RouteWEBSOCKET(Connection* Base);
				static bool RouteGET(Connection* Base);
				static bool RoutePOST(Connection* Base);
				static bool RoutePUT(Connection* Base);
				static bool RoutePATCH(Connection* Base);
				static bool RouteDELETE(Connection* Base);
				static bool RouteOPTIONS(Connection* Base);
			};

			class ED_OUT_TS Logical
			{
			public:
				static bool ProcessDirectory(Connection* Base);
				static bool ProcessResource(Connection* Base);
				static bool ProcessResourceCompress(Connection* Base, bool Deflate, bool Gzip, const char* ContentRange, size_t Range);
				static bool ProcessResourceCache(Connection* Base);
				static bool ProcessFile(Connection* Base, size_t ContentLength, size_t Range);
				static bool ProcessFileChunk(Connection* Base, Server* Router, FILE* Stream, size_t ContentLength);
				static bool ProcessFileCompress(Connection* Base, size_t ContentLength, size_t Range, bool Gzip);
				static bool ProcessFileCompressChunk(Connection* Base, Server* Router, FILE* Stream, void* CStream, size_t ContentLength);
				static bool ProcessGateway(Connection* Base);
				static bool ProcessWebSocket(Connection* Base, const char* Key);
			};

			class ED_OUT_TS Server final : public SocketServer
			{
				friend GatewayFrame;
				friend Connection;
				friend Logical;
				friend Util;

			public:
				Server();
				virtual ~Server() override;
				bool Update();

			private:
				bool OnConfigure(SocketRouter* New) override;
				bool OnRequestEnded(SocketConnection* Base, bool Check) override;
				bool OnRequestBegin(SocketConnection* Base) override;
				bool OnStall(std::unordered_set<SocketConnection*>& Data) override;
				bool OnListen() override;
				bool OnUnlisten() override;
				SocketConnection* OnAllocate(SocketListener* Host) override;
				SocketRouter* OnAllocateRouter() override;
			};

			class ED_OUT Client final : public SocketClient
			{
			private:
				WebSocketFrame * WebSocket;
				RequestFrame Request;
				ResponseFrame Response;
				Core::Promise<bool> Future;

            public:
                char RemoteAddress[48] = { 0 };
                
			public:
				Client(int64_t ReadTimeout);
				virtual ~Client() override;
				bool Downgrade();
				Core::Promise<bool> Consume(size_t MaxSize = ED_HTTP_PAYLOAD);
				Core::Promise<bool> Fetch(HTTP::RequestFrame&& Root, size_t MaxSize = ED_HTTP_PAYLOAD);
				Core::Promise<bool> Upgrade(HTTP::RequestFrame&& Root);
				Core::Promise<ResponseFrame*> Send(HTTP::RequestFrame&& Root);
				Core::Promise<Core::Unique<Core::Schema>> JSON(HTTP::RequestFrame&& Root, size_t MaxSize = ED_HTTP_PAYLOAD);
				Core::Promise<Core::Unique<Core::Schema>> XML(HTTP::RequestFrame&& Root, size_t MaxSize = ED_HTTP_PAYLOAD);
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