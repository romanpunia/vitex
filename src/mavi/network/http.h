#ifndef VI_NETWORK_HTTP_H
#define VI_NETWORK_HTTP_H
#include "../core/network.h"
#include "../core/scripting.h"

namespace Mavi
{
	namespace Network
	{
		namespace HTTP
		{
			enum
			{
				PAYLOAD_SIZE = (size_t)(1024 * 64)
			};

			enum class Auth
			{
				Granted,
				Denied,
				Unverified
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

			typedef Core::Vector<Core::String> RangePayload;
			typedef Core::OrderedMap<Core::String, RangePayload, struct HeaderComparator> HeaderMapping;
			typedef std::function<bool(class Connection*)> SuccessCallback;
			typedef std::function<bool(class Connection*, SocketPoll, const char*, size_t)> ContentCallback;
			typedef std::function<bool(class Connection*, struct Credentials*)> AuthorizeCallback;
			typedef std::function<bool(class Connection*, Core::Stringify*)> HeaderCallback;
			typedef std::function<bool(class Connection*, Scripting::Compiler*)> CompilerCallback;
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

			struct VI_OUT HeaderComparator
			{
				struct Insensitive
				{
					bool operator() (const unsigned char& A, const unsigned char& B) const
					{
						return tolower(A) < tolower(B);
					}
				};

				bool operator() (const Core::String& Left, const Core::String& Right) const
				{
					return std::lexicographical_compare(Left.begin(), Left.end(), Right.begin(), Right.end(), Insensitive());
				}
			};

			struct VI_OUT ErrorFile
			{
				Core::String Pattern;
				int StatusCode = 0;
			};

			struct VI_OUT MimeType
			{
				Core::String Extension;
				Core::String Type;
			};

			struct VI_OUT MimeStatic
			{
				const char* Extension = nullptr;
				const char* Type = nullptr;

				MimeStatic(const char* Ext, const char* T);
			};

			struct VI_OUT Credentials
			{
				Core::String Token;
				Auth Type = Auth::Unverified;
			};

			struct VI_OUT Resource
			{
				HeaderMapping Headers;
				Core::String Path;
				Core::String Type;
				Core::String Name;
				Core::String Key;
				size_t Length = 0;
				bool Memory = false;

				void PutHeader(const Core::String& Key, const Core::String& Value);
				void SetHeader(const Core::String& Key, const Core::String& Value);
				Core::String ComposeHeader(const Core::String& Key) const;
				RangePayload* GetHeaderRanges(const Core::String& Key);
				const Core::String* GetHeaderBlob(const Core::String& Key) const;
				const char* GetHeader(const Core::String& Key) const;
			};

			struct VI_OUT Cookie
			{
				Core::String Name;
				Core::String Value;
				Core::String Domain;
				Core::String Path = "/";
				Core::String SameSite;
				Core::String Expires;
				bool Secure = false;
				bool HttpOnly = false;

				void SetExpires(int64_t Time);
				void SetExpired();
			};

			struct VI_OUT ContentFrame
			{
				Core::Vector<Resource> Resources;
				Core::Vector<char> Data;
				size_t Length = 0;
				size_t Offset = 0;
				bool Exceeds = false;
				bool Limited = false;

				void Append(const Core::String& Data);
				void Append(const char* Data, size_t Size);
				void Assign(const Core::String& Data);
				void Assign(const char* Data, size_t Size);
				void Prepare(const char* ContentLength);
				void Finalize();
				void Cleanup();
				Core::String GetText() const;
				bool IsFinalized() const;
			};

			struct VI_OUT RequestFrame
			{
				HeaderMapping Cookies;
				HeaderMapping Headers;
				ContentFrame Content;
				Core::String Query;
				Core::String Path;
				Core::String URI;
				Core::String Where;
				Compute::RegexResult Match;
				Credentials User;
				char Method[10] = { 'G', 'E', 'T' };
				char Version[10] = { 'H', 'T', 'T', 'P', '/', '1', '.', '1' };

				void SetMethod(const char* Value);
				void SetVersion(unsigned int Major, unsigned int Minor);
				void PutHeader(const Core::String& Key, const Core::String& Value);
				void SetHeader(const Core::String& Key, const Core::String& Value);
				void Cleanup();
				Core::String ComposeHeader(const Core::String& Key) const;
				RangePayload* GetCookieRanges(const Core::String& Key);
				Core::String* GetCookieBlob(const Core::String& Key) const;
				const char* GetCookie(const Core::String& Key) const;
				RangePayload* GetHeaderRanges(const Core::String& Key);
				const Core::String* GetHeaderBlob(const Core::String& Key) const;
				const char* GetHeader(const Core::String& Key) const;
				Core::Vector<std::pair<size_t, size_t>> GetRanges() const;
				std::pair<size_t, size_t> GetRange(Core::Vector<std::pair<size_t, size_t>>::iterator Range, size_t ContentLength) const;
			};

			struct VI_OUT ResponseFrame
			{
				HeaderMapping Headers;
				ContentFrame Content;
				Core::Vector<Cookie> Cookies;
				int StatusCode = -1;
				bool Error = false;

				void PutHeader(const Core::String& Key, const Core::String& Value);
				void SetHeader(const Core::String& Key, const Core::String& Value);
				void SetCookie(const Cookie& Value);
				void SetCookie(Cookie&& Value);
				void Cleanup();
				Core::String ComposeHeader(const Core::String& Key) const;
				Cookie* GetCookie(const char* Key);
				RangePayload* GetHeaderRanges(const Core::String& Key);
				const Core::String* GetHeaderBlob(const Core::String& Key) const;
				const char* GetHeader(const Core::String& Key) const;
				bool IsOK() const;
			};

			class VI_OUT RouteGroup final : public Core::Reference<RouteGroup>
			{
			public:
				Core::Vector<RouteEntry*> Routes;
				Core::String Match;
				RouteMode Mode;

			public:
				RouteGroup(const Core::String& NewMatch, RouteMode NewMode) noexcept;
				~RouteGroup() noexcept;
			};

			class VI_OUT WebSocketFrame final : public Core::Reference<WebSocketFrame>
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
				Core::SingleQueue<Message> Messages;
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
				~WebSocketFrame() noexcept;
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

			class VI_OUT GatewayFrame final : public Core::Reference<GatewayFrame>
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
				Scripting::Compiler* Compiler;
				std::atomic<bool> Active;

			public:
				GatewayFrame(HTTP::Connection* NewBase, Scripting::Compiler* NewCompiler);
				bool Start(const Core::String& Path, const char* Method, char* Buffer, size_t Size);
				bool Error(int StatusCode, const char* Text);
				bool Finish();
				bool IsFinished();
				bool GetException(const char** Exception, const char** Function, int* Line, int* Column);
				Scripting::ImmediateContext* GetContext();
				Scripting::Compiler* GetCompiler();
				HTTP::Connection* GetBase();
			};

			class VI_OUT RouteEntry final : public Core::Reference<RouteEntry>
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
					Core::Vector<Compute::RegexSource> Files;
					Core::Vector<Core::String> Methods;
					bool ReportStack = false;
					bool ReportErrors = false;
				} Gateway;
				struct
				{
					Core::Vector<Core::String> Methods;
					Core::String Type;
					Core::String Realm;
				} Auth;
				struct
				{
					Core::Vector<Compute::RegexSource> Files;
					CompressionTune Tune = CompressionTune::Default;
					int QualityLevel = 8;
					int MemoryLevel = 8;
					size_t MinLength = 16384;
					bool Enabled = false;
				} Compression;

			public:
				Core::Vector<Compute::RegexSource> HiddenFiles;
				Core::Vector<ErrorFile> ErrorFiles;
				Core::Vector<MimeType> MimeTypes;
				Core::Vector<Core::String> IndexFiles;
				Core::Vector<Core::String> TryFiles;
				Core::Vector<Core::String> DisallowedMethods;
				Core::String DocumentRoot = "./";
				Core::String CharSet = "utf-8";
				Core::String ProxyIpAddress;
				Core::String AccessControlAllowOrigin;
				Core::String Redirect;
				Core::String Override;
				size_t WebSocketTimeout = 30000;
				size_t StaticFileMaxAge = 604800;
				size_t MaxCacheLength = 1024 * 64;
				size_t Level = 0;
				bool AllowDirectoryListing = false;
				bool AllowWebSocket = false;
				bool AllowSendFile = true;
				Compute::RegexSource URI;
				SiteEntry* Site = nullptr;

			public:
				RouteEntry() = default;
				RouteEntry(RouteEntry* Other, const Compute::RegexSource& Source);
			};

			class VI_OUT SiteEntry final : public Core::Reference<SiteEntry>
			{
			public:
				struct
				{
					struct
					{
						struct
						{
							Core::String Name = "sid";
							Core::String Domain;
							Core::String Path = "/";
							Core::String SameSite = "Strict";
							uint64_t Expires = 31536000;
							bool Secure = false;
							bool HttpOnly = true;
						} Cookie;

						Core::String DocumentRoot;
						uint64_t Expires = 604800;
					} Session;

					bool Enabled = false;
				} Gateway;
				struct
				{
					SuccessCallback OnRewriteURL;
				} Callbacks;

			public:
				Core::Vector<RouteGroup*> Groups;
				Core::String ResourceRoot = "./temp";
				size_t MaxResources = 5;
				RouteEntry* Base = nullptr;
				MapRouter* Router = nullptr;

			public:
				SiteEntry();
				~SiteEntry() noexcept;
				void Sort();
				RouteGroup* Group(const Core::String& Match, RouteMode Mode);
				RouteEntry* Route(const Core::String& Match, RouteMode Mode, const Core::String& Pattern);
				RouteEntry* Route(const Core::String& Pattern, RouteGroup* Group, RouteEntry* From);
				bool Remove(RouteEntry* Source);
				bool Get(const char* Pattern, const SuccessCallback& Callback);
				bool Get(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Post(const char* Pattern, const SuccessCallback& Callback);
				bool Post(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Put(const char* Pattern, const SuccessCallback& Callback);
				bool Put(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Patch(const char* Pattern, const SuccessCallback& Callback);
				bool Patch(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Delete(const char* Pattern, const SuccessCallback& Callback);
				bool Delete(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Options(const char* Pattern, const SuccessCallback& Callback);
				bool Options(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool Access(const char* Pattern, const SuccessCallback& Callback);
				bool Access(const Core::String& Match, RouteMode Mode, const char* Pattern, const SuccessCallback& Callback);
				bool WebSocketConnect(const char* Pattern, const WebSocketCallback& Callback);
				bool WebSocketConnect(const Core::String& Match, RouteMode Mode, const char* Pattern, const WebSocketCallback& Callback);
				bool WebSocketDisconnect(const char* Pattern, const WebSocketCallback& Callback);
				bool WebSocketDisconnect(const Core::String& Match, RouteMode Mode, const char* Pattern, const WebSocketCallback& Callback);
				bool WebSocketReceive(const char* Pattern, const WebSocketReadCallback& Callback);
				bool WebSocketReceive(const Core::String& Match, RouteMode Mode, const char* Pattern, const WebSocketReadCallback& Callback);
			};

			class VI_OUT MapRouter final : public SocketRouter
			{
			public:
				Core::UnorderedMap<Core::String, SiteEntry*> Sites;
				Core::String ModuleRoot;
				Scripting::VirtualMachine* VM;

			public:
				MapRouter();
				~MapRouter() override;
				SiteEntry* Site();
				SiteEntry* Site(const char* Host);
			};

			class VI_OUT Connection final : public SocketConnection
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
				~Connection() noexcept override;
				void Reset(bool Fully) override;
				bool Finish() override;
				bool Finish(int StatusCode) override;
				bool EncryptionInfo(Certificate* Output) override;
				bool Consume(const ContentCallback& Callback = nullptr, bool Eat = false);
				bool Store(const ResourceCallback& Callback = nullptr, bool Eat = false);
				bool Skip(const SuccessCallback& Callback);
			};

			class VI_OUT Query final : public Core::Reference<Query>
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
				~Query() noexcept;
				void Clear();
				void Steal(Core::Schema** Output);
				void Decode(const char* ContentType, const Core::String& URI);
				Core::String Encode(const char* ContentType) const;
				Core::Schema* Get(const char* Name) const;
				Core::Schema* Set(const char* Name);
				Core::Schema* Set(const char* Name, const char* Value);

			private:
				void NewParameter(Core::Vector<QueryToken>* Tokens, const QueryToken& Name, const QueryToken& Value);
				void DecodeAXWFD(const Core::String& URI);
				void DecodeAJSON(const Core::String& URI);
				Core::String EncodeAXWFD() const;
				Core::String EncodeAJSON() const;
				Core::Schema* GetParameter(QueryToken* Name);

			private:
				static Core::String Build(Core::Schema* Base);
				static Core::String BuildFromBase(Core::Schema* Base);
				static Core::Schema* FindParameter(Core::Schema* Base, QueryToken* Name);
			};

			class VI_OUT Session final : public Core::Reference<Session>
			{
			public:
				Core::Schema* Query = nullptr;
				Core::String SessionId;
				int64_t SessionExpires = 0;
				bool IsNewSession = false;

			public:
				Session();
				~Session() noexcept;
				void Clear();
				bool Write(Connection* Base);
				bool Read(Connection* Base);

			private:
				Core::String& FindSessionId(Connection* Base);
				Core::String& GenerateSessionId(Connection* Base);

			public:
				static bool InvalidateCache(const Core::String& Path);
			};

			class VI_OUT Parser final : public Core::Reference<Parser>
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
					Core::String Header;
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
				~Parser() noexcept;
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

			class VI_OUT WebCodec final : public Core::Reference<WebCodec>
			{
			public:
				typedef Core::SingleQueue<std::pair<WebSocketOp, Core::Vector<char>>> MessageQueue;

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
				Core::Vector<char> Payload;
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
				Core::Vector<char> Data;

			public:
				WebCodec();
				bool ParseFrame(const char* Data, size_t Size);
				bool GetFrame(WebSocketOp* Op, Core::Vector<char>* Message);
			};

			class VI_OUT_TS Util
			{
			public:
				static Core::String ConnectionResolve(Connection* Base);
				static const char* StatusMessage(int StatusCode);
				static const char* ContentType(const Core::String& Path, Core::Vector<MimeType>* MimeTypes);
			};

			class VI_OUT_TS Paths
			{
			public:
				static void ConstructPath(Connection* Base);
				static void ConstructHeadFull(RequestFrame* Request, ResponseFrame* Response, bool IsRequest, Core::Stringify* Buffer);
				static void ConstructHeadCache(Connection* Base, Core::Stringify* Buffer);
				static void ConstructHeadUncache(Connection* Base, Core::Stringify* Buffer);
				static bool ConstructRoute(MapRouter* Router, Connection* Base);
				static bool ConstructDirectoryEntries(Connection* Base, const Core::FileEntry& A, const Core::FileEntry& B);
				static Core::String ConstructContentRange(size_t Offset, size_t Length, size_t ContentLength);
			};

			class VI_OUT_TS Parsing
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
				static Core::String ParseMultipartDataBoundary();
				static void ParseCookie(const Core::String& Value);
			};

			class VI_OUT_TS Permissions
			{
			public:
				static Core::String Authorize(const Core::String& Username, const Core::String& Password, const Core::String& Type = "Basic");
				static bool Authorize(Connection* Base);
				static bool MethodAllowed(Connection* Base);
				static bool WebSocketUpgradeAllowed(Connection* Base);
			};

			class VI_OUT_TS Resources
			{
			public:
				static bool ResourceHasAlternative(Connection* Base);
				static bool ResourceHidden(Connection* Base, Core::String* Path);
				static bool ResourceIndexed(Connection* Base, Core::FileEntry* Resource);
				static bool ResourceProvided(Connection* Base, Core::FileEntry* Resource);
				static bool ResourceModified(Connection* Base, Core::FileEntry* Resource);
				static bool ResourceCompressed(Connection* Base, size_t Size);
			};

			class VI_OUT_TS Routing
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

			class VI_OUT_TS Logical
			{
			public:
				static bool ProcessDirectory(Connection* Base);
				static bool ProcessResource(Connection* Base);
				static bool ProcessResourceCompress(Connection* Base, bool Deflate, bool Gzip, const char* ContentRange, size_t Range);
				static bool ProcessResourceCache(Connection* Base);
				static bool ProcessFile(Connection* Base, size_t ContentLength, size_t Range);
				static bool ProcessFileStream(Connection* Base, FILE* Stream, size_t ContentLength, size_t Range);
				static bool ProcessFileChunk(Connection* Base, Server* Router, FILE* Stream, size_t ContentLength);
				static bool ProcessFileCompress(Connection* Base, size_t ContentLength, size_t Range, bool Gzip);
				static bool ProcessFileCompressChunk(Connection* Base, Server* Router, FILE* Stream, void* CStream, size_t ContentLength);
				static bool ProcessGateway(Connection* Base);
				static bool ProcessWebSocket(Connection* Base, const char* Key);
			};

			class VI_OUT_TS Server final : public SocketServer
			{
				friend GatewayFrame;
				friend Connection;
				friend Logical;
				friend Util;

			public:
				Server();
				~Server() override;
				bool Update();

			private:
				bool OnConfigure(SocketRouter* New) override;
				bool OnRequestEnded(SocketConnection* Base, bool Check) override;
				bool OnRequestBegin(SocketConnection* Base) override;
				bool OnStall(Core::UnorderedSet<SocketConnection*>& Data) override;
				bool OnListen() override;
				bool OnUnlisten() override;
				SocketConnection* OnAllocate(SocketListener* Host) override;
				SocketRouter* OnAllocateRouter() override;
			};

			class VI_OUT Client final : public SocketClient
			{
			private:
				WebSocketFrame* WebSocket;
				RequestFrame Request;
				ResponseFrame Response;
				Core::Promise<bool> Future;

            public:
                char RemoteAddress[48] = { };
                
			public:
				Client(int64_t ReadTimeout);
				~Client() override;
				bool Downgrade();
				Core::Promise<bool> Consume(size_t MaxSize = PAYLOAD_SIZE);
				Core::Promise<bool> Fetch(HTTP::RequestFrame&& Root, size_t MaxSize = PAYLOAD_SIZE);
				Core::Promise<bool> Upgrade(HTTP::RequestFrame&& Root);
				Core::Promise<ResponseFrame*> Send(HTTP::RequestFrame&& Root);
				Core::Promise<Core::Unique<Core::Schema>> JSON(HTTP::RequestFrame&& Root, size_t MaxSize = PAYLOAD_SIZE);
				Core::Promise<Core::Unique<Core::Schema>> XML(HTTP::RequestFrame&& Root, size_t MaxSize = PAYLOAD_SIZE);
				WebSocketFrame* GetWebSocket();
				RequestFrame* GetRequest();
				ResponseFrame* GetResponse();
				ContentFrame* GetContent();

			private:
				bool Receive();
			};
		}
	}
}
#endif