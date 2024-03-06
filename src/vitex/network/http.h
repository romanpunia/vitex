#ifndef VI_NETWORK_HTTP_H
#define VI_NETWORK_HTTP_H
#include "../network.h"

namespace Vitex
{
	namespace Network
	{
		namespace HTTP
		{
			enum
			{
				LABEL_SIZE = 16,
				INLINING_SIZE = 768,
				PAYLOAD_SIZE = (size_t)(1024 * 64)
			};

			enum class Auth
			{
				Granted,
				Denied,
				Unverified
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
				Exact,
				Start,
				Match,
				End
			};

			template <typename T, T OffsetBasis, T Prime>
			struct LFNV1AHash
			{
				static_assert(std::is_unsigned<T>::value, "Q needs to be unsigned integer");

				inline T operator()(const void* Address, size_t Size) const noexcept
				{
					const auto Data = static_cast<const uint8_t*>(Address);
					auto State = OffsetBasis;
					for (size_t i = 0; i < Size; ++i)
						State = (State ^ (size_t)tolower(Data[i])) * Prime;
					return State;
				}
			};

			template <size_t Bits>
			struct LFNV1ABits;

			template <>
			struct LFNV1ABits<32> { using type = LFNV1AHash<uint32_t, UINT32_C(2166136261), UINT32_C(16777619)>; };

			template <>
			struct LFNV1ABits<64> { using type = LFNV1AHash<uint64_t, UINT64_C(14695981039346656037), UINT64_C(1099511628211)>; };

			template <size_t Bits>
			using LFNV1A = typename LFNV1ABits<Bits>::type;

			struct KimvEqualTo
			{
				typedef Core::String first_argument_type;
				typedef Core::String second_argument_type;
				typedef bool result_type;
				using is_transparent = void;

				inline result_type operator()(const Core::String& Left, const Core::String& Right) const noexcept
				{
					return Left.size() == Right.size() && std::equal(Left.begin(), Left.end(), Right.begin(), [](uint8_t A, uint8_t B) { return tolower(A) == tolower(B); });
				}
				inline result_type operator()(const Core::String& Left, const char* Right) const noexcept
				{
					size_t Size = strlen(Right);
					return Left.size() == Size && std::equal(Left.begin(), Left.end(), Right, [](uint8_t A, uint8_t B) { return tolower(A) == tolower(B); });
				}
				inline result_type operator()(const Core::String& Left, const std::string_view& Right) const noexcept
				{
					return Left.size() == Right.size() && std::equal(Left.begin(), Left.end(), Right.begin(), [](uint8_t A, uint8_t B) { return tolower(A) == tolower(B); });
				}
				inline result_type operator()(const char* Left, const Core::String& Right) const noexcept
				{
					size_t Size = strlen(Left);
					return Size == Right.size() && std::equal(Left, Left + Size, Right.begin(), [](uint8_t A, uint8_t B) { return tolower(A) == tolower(B); });
				}
				inline result_type operator()(const std::string_view& Left, const Core::String& Right) const noexcept
				{
					return Left.size() == Right.size() && std::equal(Left.begin(), Left.end(), Right.begin(), [](uint8_t A, uint8_t B) { return tolower(A) == tolower(B); });
				}
			};

			struct KimvKeyHasher
			{
				typedef float argument_type;
				typedef size_t result_type;
				using is_transparent = void;

				inline result_type operator()(const char* Value) const noexcept
				{
					return LFNV1A<8 * sizeof(size_t)>()(Value, strlen(Value));
				}
				inline result_type operator()(const std::string_view& Value) const noexcept
				{
					return LFNV1A<8 * sizeof(size_t)>()(Value.data(), Value.size());
				}
				inline result_type operator()(const Core::String& Value) const noexcept
				{
					return LFNV1A<8 * sizeof(size_t)>()(Value.c_str(), Value.size());
				}
			};

			typedef Core::UnorderedMap<Core::String, Core::Vector<Core::String>, KimvKeyHasher, KimvEqualTo> KimvUnorderedMap;
			typedef std::function<bool(class Connection*)> SuccessCallback;
			typedef std::function<void(class Connection*, SocketPoll)> HeadersCallback;
			typedef std::function<bool(class Connection*, SocketPoll, const std::string_view&)> ContentCallback;
			typedef std::function<bool(class Connection*, struct Credentials*)> AuthorizeCallback;
			typedef std::function<bool(class Connection*, Core::String&)> HeaderCallback;
			typedef std::function<bool(struct Resource*)> ResourceCallback;
			typedef std::function<void(class WebSocketFrame*)> WebSocketCallback;
			typedef std::function<bool(class WebSocketFrame*, WebSocketOp, const std::string_view&)> WebSocketReadCallback;
			typedef std::function<void(class WebSocketFrame*, bool)> WebSocketStatusCallback;
			typedef std::function<bool(class WebSocketFrame*)> WebSocketCheckCallback;

			class Parser;

			class Connection;

			class Client;

			class RouterEntry;

			class MapRouter;

			class Server;

			class Query;

			class WebCodec;

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
				std::string_view Extension = "";
				std::string_view Type = "";

				MimeStatic(const std::string_view& Ext, const std::string_view& T);
			};

			struct VI_OUT Credentials
			{
				Core::String Token;
				Auth Type = Auth::Unverified;
			};

			struct VI_OUT Resource
			{
				KimvUnorderedMap Headers;
				Core::String Path;
				Core::String Type;
				Core::String Name;
				Core::String Key;
				size_t Length = 0;
				bool IsInMemory = false;

				Core::String& PutHeader(const std::string_view& Key, const std::string_view& Value);
				Core::String& SetHeader(const std::string_view& Key, const std::string_view& Value);
				Core::String ComposeHeader(const std::string_view& Key) const;
				Core::Vector<Core::String>* GetHeaderRanges(const std::string_view& Key);
				Core::String* GetHeaderBlob(const std::string_view& Key);
				std::string_view GetHeader(const std::string_view& Key) const;
				const Core::String& GetInMemoryContents() const;
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
				size_t Length;
				size_t Offset;
				size_t Prefetch;
				bool Exceeds;
				bool Limited;

				ContentFrame();
				ContentFrame(const ContentFrame&) = default;
				ContentFrame(ContentFrame&&) noexcept = default;
				ContentFrame& operator= (const ContentFrame&) = default;
				ContentFrame& operator= (ContentFrame&&) noexcept = default;
				void Append(const std::string_view& Data);
				void Assign(const std::string_view& Data);
				void Prepare(const KimvUnorderedMap& Headers, const uint8_t* Buffer, size_t Size);
				void Finalize();
				void Cleanup();
				Core::ExpectsParser<Core::Unique<Core::Schema>> GetJSON() const;
				Core::ExpectsParser<Core::Unique<Core::Schema>> GetXML() const;
				Core::String GetText() const;
				bool IsFinalized() const;
			};

			struct VI_OUT RequestFrame
			{
				ContentFrame Content;
				KimvUnorderedMap Cookies;
				KimvUnorderedMap Headers;
				Compute::RegexResult Match;
				Credentials User;
				Core::String Query;
				Core::String Path;
				Core::String Location;
				Core::String Referrer;
				char Method[LABEL_SIZE];
				char Version[LABEL_SIZE];

				RequestFrame();
				RequestFrame(const RequestFrame&) = default;
				RequestFrame(RequestFrame&&) noexcept = default;
				RequestFrame& operator= (const RequestFrame&) = default;
				RequestFrame& operator= (RequestFrame&&) noexcept = default;
				void SetMethod(const std::string_view& Value);
				void SetVersion(uint32_t Major, uint32_t Minor);
				void Cleanup();
				Core::String& PutHeader(const std::string_view& Key, const std::string_view& Value);
				Core::String& SetHeader(const std::string_view& Key, const std::string_view& Value);
				Core::String ComposeHeader(const std::string_view& Key) const;
				Core::Vector<Core::String>* GetHeaderRanges(const std::string_view& Key);
				Core::String* GetHeaderBlob(const std::string_view& Key);
				std::string_view GetHeader(const std::string_view& Key) const;
				Core::Vector<Core::String>* GetCookieRanges(const std::string_view& Key);
				Core::String* GetCookieBlob(const std::string_view& Key);
				std::string_view GetCookie(const std::string_view& Key) const;
				Core::Vector<std::pair<size_t, size_t>> GetRanges() const;
				std::pair<size_t, size_t> GetRange(Core::Vector<std::pair<size_t, size_t>>::iterator Range, size_t ContentLength) const;
			};

			struct VI_OUT ResponseFrame
			{
				ContentFrame Content;
				KimvUnorderedMap Headers;
				Core::Vector<Cookie> Cookies;
				int StatusCode;
				bool Error;

				ResponseFrame();
				ResponseFrame(const ResponseFrame&) = default;
				ResponseFrame(ResponseFrame&&) noexcept = default;
				ResponseFrame& operator= (const ResponseFrame&) = default;
				ResponseFrame& operator= (ResponseFrame&&) noexcept = default;
				void SetCookie(const Cookie& Value);
				void SetCookie(Cookie&& Value);
				void Cleanup();
				Core::String& PutHeader(const std::string_view& Key, const std::string_view& Value);
				Core::String& SetHeader(const std::string_view& Key, const std::string_view& Value);
				Core::String ComposeHeader(const std::string_view& Key) const;
				Core::Vector<Core::String>* GetHeaderRanges(const std::string_view& Key);
				Core::String* GetHeaderBlob(const std::string_view& Key);
				std::string_view GetHeader(const std::string_view& Key) const;
				Cookie* GetCookie(const std::string_view& Key);
				bool IsUndefined() const;
				bool IsOK() const;
			};

			struct VI_OUT FetchFrame
			{
				ContentFrame Content;
				KimvUnorderedMap Cookies;
				KimvUnorderedMap Headers;
				uint64_t Timeout;
				size_t MaxSize;
				uint32_t VerifyPeers;

				FetchFrame();
				FetchFrame(const FetchFrame&) = default;
				FetchFrame(FetchFrame&&) noexcept = default;
				FetchFrame& operator= (const FetchFrame&) = default;
				FetchFrame& operator= (FetchFrame&&) noexcept = default;
				void Cleanup();
				Core::String& PutHeader(const std::string_view& Key, const std::string_view& Value);
				Core::String& SetHeader(const std::string_view& Key, const std::string_view& Value);
				Core::String ComposeHeader(const std::string_view& Key) const;
				Core::Vector<Core::String>* GetHeaderRanges(const std::string_view& Key);
				Core::String* GetHeaderBlob(const std::string_view& Key);
				std::string_view GetHeader(const std::string_view& Key) const;
				Core::Vector<Core::String>* GetCookieRanges(const std::string_view& Key);
				Core::String* GetCookieBlob(const std::string_view& Key);
				std::string_view GetCookie(const std::string_view& Key) const;
				Core::Vector<std::pair<size_t, size_t>> GetRanges() const;
				std::pair<size_t, size_t> GetRange(Core::Vector<std::pair<size_t, size_t>>::iterator Range, size_t ContentLength) const;
			};

			class VI_OUT WebSocketFrame final : public Core::Reference<WebSocketFrame>
			{
				friend class Connection;

			private:
				enum class Tunnel
				{
					Healthy,
					Closing,
					Gone
				};

			private:
				struct Message
				{
					uint32_t Mask;
					char* Buffer;
					size_t Size;
					WebSocketOp Opcode;
					WebSocketCallback Callback;
				};

			public:
				struct
				{
					WebSocketCallback Destroy;
					WebSocketStatusCallback Close;
					WebSocketCheckCallback Dead;
				} Lifetime;

			private:
				std::mutex Section;
				Core::SingleQueue<Message> Messages;
				Socket* Stream;
				WebCodec* Codec;
				std::atomic<uint32_t> State;
				std::atomic<uint32_t> Tunneling;
				std::atomic<bool> Active;
				std::atomic<bool> Deadly;
				std::atomic<bool> Busy;

			public:
				WebSocketCallback Connect;
				WebSocketCallback BeforeDisconnect;
				WebSocketCallback Disconnect;
				WebSocketReadCallback Receive;
				void* UserData;

			public:
				WebSocketFrame(Socket* NewStream, void* NewUserData);
				~WebSocketFrame() noexcept;
				Core::ExpectsSystem<size_t> Send(const std::string_view& Buffer, WebSocketOp OpCode, WebSocketCallback&& Callback);
				Core::ExpectsSystem<size_t> Send(uint32_t Mask, const std::string_view& Buffer, WebSocketOp OpCode, WebSocketCallback&& Callback);
				Core::ExpectsSystem<void> SendClose(WebSocketCallback&& Callback);
				void Next();
				bool IsFinished();
				Socket* GetStream();
				Connection* GetConnection();
				Client* GetClient();

			private:
				void Update();
				void Finalize();
				void Dequeue();
				bool Enqueue(uint32_t Mask, const std::string_view& Buffer, WebSocketOp OpCode, WebSocketCallback&& Callback);
				bool IsWriteable();
				bool IsIgnore();
			};

			class VI_OUT RouterGroup final : public Core::Reference<RouterGroup>
			{
			public:
				Core::String Match;
				Core::Vector<RouterEntry*> Routes;
				RouteMode Mode;

			public:
				RouterGroup(const std::string_view& NewMatch, RouteMode NewMode) noexcept;
				~RouterGroup() noexcept;
			};

			class VI_OUT RouterEntry final : public Core::Reference<RouterEntry>
			{
				friend MapRouter;

			public:
				struct EntryCallbacks
				{
					struct WebSocketCallbacks
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
					HeaderCallback Headers;
					AuthorizeCallback Authorize;
				} Callbacks;

				struct EntryAuth
				{
					Core::String Type;
					Core::String Realm;
					Core::Vector<Core::String> Methods;
				} Auth;

				struct EntryCompression
				{
					Core::Vector<Compute::RegexSource> Files;
					CompressionTune Tune = CompressionTune::Default;
					size_t MinLength = 16384;
					int QualityLevel = 8;
					int MemoryLevel = 8;
					bool Enabled = false;
				} Compression;

			public:
				Compute::RegexSource Location;
				Core::String FilesDirectory;
				Core::String CharSet = "utf-8";
				Core::String ProxyIpAddress;
				Core::String AccessControlAllowOrigin;
				Core::String Redirect;
				Core::String Alias;
				Core::Vector<Compute::RegexSource> HiddenFiles;
				Core::Vector<ErrorFile> ErrorFiles;
				Core::Vector<MimeType> MimeTypes;
				Core::Vector<Core::String> IndexFiles;
				Core::Vector<Core::String> TryFiles;
				Core::Vector<Core::String> DisallowedMethods;
				MapRouter* Router = nullptr;
				size_t WebSocketTimeout = 30000;
				size_t StaticFileMaxAge = 604800;
				size_t Level = 0;
				bool AllowDirectoryListing = false;
				bool AllowWebSocket = false;
				bool AllowSendFile = true;

			private:
				static RouterEntry* From(const RouterEntry& Other, const Compute::RegexSource& Source);
			};

			class VI_OUT MapRouter final : public SocketRouter
			{
			public:
				struct RouterSession
				{
					struct RouterCookie
					{
						Core::String Name = "sid";
						Core::String Domain;
						Core::String Path = "/";
						Core::String SameSite = "Strict";
						uint64_t Expires = 31536000;
						bool Secure = false;
						bool HttpOnly = true;
					} Cookie;

					Core::String Directory;
					uint64_t Expires = 604800;
				} Session;

				struct RouterCallbacks
				{
					std::function<void(MapRouter*)> OnDestroy;
					SuccessCallback OnLocation;
				} Callbacks;

			public:
				Core::String TemporaryDirectory = "./temp";
				Core::Vector<RouterGroup*> Groups;
				size_t MaxUploadableResources = 10;
				RouterEntry* Base = nullptr;

			public:
				MapRouter();
				~MapRouter() override;
				void Sort();
				RouterGroup* Group(const std::string_view& Match, RouteMode Mode);
				RouterEntry* Route(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, bool InheritProps);
				RouterEntry* Route(const std::string_view& Pattern, RouterGroup* Group, RouterEntry* From);
				bool Remove(RouterEntry* Source);
				bool Get(const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Get(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Post(const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Post(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Put(const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Put(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Patch(const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Patch(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Delete(const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Delete(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Options(const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Options(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Access(const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Access(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback);
				bool Headers(const std::string_view& Pattern, HeaderCallback&& Callback);
				bool Headers(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, HeaderCallback&& Callback);
				bool Authorize(const std::string_view& Pattern, AuthorizeCallback&& Callback);
				bool Authorize(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, AuthorizeCallback&& Callback);
				bool WebSocketInitiate(const std::string_view& Pattern, SuccessCallback&& Callback);
				bool WebSocketInitiate(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, SuccessCallback&& Callback);
				bool WebSocketConnect(const std::string_view& Pattern, WebSocketCallback&& Callback);
				bool WebSocketConnect(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, WebSocketCallback&& Callback);
				bool WebSocketDisconnect(const std::string_view& Pattern, WebSocketCallback&& Callback);
				bool WebSocketDisconnect(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, WebSocketCallback&& Callback);
				bool WebSocketReceive(const std::string_view& Pattern, WebSocketReadCallback&& Callback);
				bool WebSocketReceive(const std::string_view& Match, RouteMode Mode, const std::string_view& Pattern, WebSocketReadCallback&& Callback);
			};

			class VI_OUT Connection final : public SocketConnection
			{
			public:
				RequestFrame Request;
				ResponseFrame Response;
				Core::FileEntry Resource;
				Parser* Resolver = nullptr;
				WebSocketFrame* WebSocket = nullptr;
				RouterEntry* Route = nullptr;
				Server* Root = nullptr;

			public:
				Connection(Server* Source) noexcept;
				~Connection() noexcept override;
				void Reset(bool Fully) override;
				bool Next() override;
				bool Next(int StatusCode) override;
				bool SendHeaders(int StatusCode, bool SpecifyTransferEncoding, HeadersCallback&& Callback);
				bool SendChunk(const std::string_view& Chunk, HeadersCallback&& Callback);
				bool Fetch(ContentCallback&& Callback = nullptr, bool Eat = false);
				bool Store(ResourceCallback&& Callback = nullptr, bool Eat = false);
				bool Skip(SuccessCallback&& Callback);
				Core::ExpectsIO<Core::String> GetPeerIpAddress() const;

			private:
				bool ComposeResponse(bool ApplyErrorResponse, bool ApplyBodyInline, HeadersCallback&& Callback);
				bool ErrorResponseRequested();
				bool BodyInliningRequested();
				bool WaitingForWebSocket();
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
				void Decode(const std::string_view& ContentType, const std::string_view& Body);
				Core::String Encode(const std::string_view& ContentType) const;
				Core::Schema* Get(const std::string_view& Name) const;
				Core::Schema* Set(const std::string_view& Name);
				Core::Schema* Set(const std::string_view& Name, const std::string_view& Value);

			private:
				void NewParameter(Core::Vector<QueryToken>* Tokens, const QueryToken& Name, const QueryToken& Value);
				void DecodeAXWFD(const std::string_view& Body);
				void DecodeAJSON(const std::string_view& Body);
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
				Core::String SessionId;
				Core::Schema* Query = nullptr;
				int64_t SessionExpires = 0;

			public:
				Session();
				~Session() noexcept;
				Core::ExpectsSystem<void> Write(Connection* Base);
				Core::ExpectsSystem<void> Read(Connection* Base);
				void Clear();

			private:
				Core::String& FindSessionId(Connection* Base);
				Core::String& GenerateSessionId(Connection* Base);

			public:
				static Core::ExpectsSystem<void> InvalidateCache(const std::string_view& Path);
			};

			class VI_OUT Parser final : public Core::Reference<Parser>
			{
			private:
				enum class MultipartStatus : uint8_t
				{
					Uninitialized = 1,
					Start,
					Start_Boundary,
					Header_Field_Start,
					Header_Field,
					Header_Field_Waiting,
					Header_Value_Start,
					Header_Value,
					Header_Value_Waiting,
					Resource_Start,
					Resource,
					Resource_Boundary_Waiting,
					Resource_Boundary,
					Resource_Waiting,
					Resource_End,
					Resource_Hyphen,
					End
				};

				enum class ChunkedStatus : int8_t
				{
					Size,
					Ext,
					Data,
					End,
					Head,
					Middle
				};

			public:
				struct MessageState
				{
					Core::String Header;
					char* Version = nullptr;
					char* Method = nullptr;
					int* StatusCode = nullptr;
					Core::String* Location = nullptr;
					Core::String* Query = nullptr;
					KimvUnorderedMap* Cookies = nullptr;
					KimvUnorderedMap* Headers = nullptr;
					ContentFrame* Content = nullptr;
				} Message;

				struct ChunkedState
				{
					size_t Length = 0;
					ChunkedStatus State = ChunkedStatus::Size;
					int8_t ConsumeTrailer = 1;
					int8_t HexCount = 0;
				} Chunked;

				struct MultipartState
				{
					Resource Data;
					ResourceCallback Callback;
					Core::String* TemporaryDirectory = nullptr;
					FILE* Stream = nullptr;
					uint8_t* LookBehind = nullptr;
					uint8_t* Boundary = nullptr;
					int64_t Index = 0;
					int64_t Length = 0;
					size_t MaxResources = 0;
					bool Skip = false;
					bool Finish = false;
					MultipartStatus State = MultipartStatus::Start;
				} Multipart;

			public:
				Parser();
				~Parser() noexcept;
				void PrepareForRequestParsing(RequestFrame* Request);
				void PrepareForResponseParsing(ResponseFrame* Response);
				void PrepareForChunkedParsing();
				void PrepareForMultipartParsing(ContentFrame* Content, Core::String* TemporaryDirectory, size_t MaxResources, bool Skip, ResourceCallback&& Callback);
				int64_t MultipartParse(const std::string_view& Boundary, const uint8_t* Buffer, size_t Length);
				int64_t ParseRequest(const uint8_t* BufferStart, size_t Length, size_t LengthLastTime);
				int64_t ParseResponse(const uint8_t* BufferStart, size_t Length, size_t LengthLastTime);
				int64_t ParseDecodeChunked(uint8_t* Buffer, size_t* BufferLength);

			private:
				const uint8_t* IsCompleted(const uint8_t* Buffer, const uint8_t* BufferEnd, size_t Offset, int* Out);
				const uint8_t* Tokenize(const uint8_t* Buffer, const uint8_t* BufferEnd, const uint8_t** Token, size_t* TokenLength, int* Out);
				const uint8_t* ProcessVersion(const uint8_t* Buffer, const uint8_t* BufferEnd, int* Out);
				const uint8_t* ProcessHeaders(const uint8_t* Buffer, const uint8_t* BufferEnd, int* Out);
				const uint8_t* ProcessRequest(const uint8_t* Buffer, const uint8_t* BufferEnd, int* Out);
				const uint8_t* ProcessResponse(const uint8_t* Buffer, const uint8_t* BufferEnd, int* Out);
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
				MessageQueue Queue;
				Core::Vector<char> Payload;
				uint64_t Remains;
				WebSocketOp Opcode;
				Bytecode State;
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
				bool ParseFrame(const uint8_t* Buffer, size_t Size);
				bool GetFrame(WebSocketOp* Op, Core::Vector<char>* Message);
			};

			class VI_OUT_TS HrmCache final : public Core::Singleton<HrmCache>
			{
			private:
				std::mutex Mutex;
				Core::SingleQueue<Core::String*> Queue;
				size_t Capacity;
				size_t Size;

			public:
				HrmCache() noexcept;
				HrmCache(size_t MaxBytesStorage) noexcept;
				virtual ~HrmCache() noexcept override;
				void Rescale(size_t MaxBytesStorage) noexcept;
				void Shrink() noexcept;
				void Push(Core::String* Entry);
				Core::String* Pop() noexcept;

			private:
				void ShrinkToFit() noexcept;
			};

			class VI_OUT_TS Utils
			{
			public:
				static void UpdateKeepAliveHeaders(Connection* Base, Core::String& Content);
				static std::string_view StatusMessage(int StatusCode);
				static std::string_view ContentType(const std::string_view& Path, Core::Vector<MimeType>* MimeTypes);
			};

			class VI_OUT_TS Paths
			{
			public:
				static void ConstructPath(Connection* Base);
				static void ConstructHeadFull(RequestFrame* Request, ResponseFrame* Response, bool IsRequest, Core::String& Buffer);
				static void ConstructHeadCache(Connection* Base, Core::String& Buffer);
				static void ConstructHeadUncache(Core::String& Buffer);
				static bool ConstructRoute(MapRouter* Router, Connection* Base);
				static bool ConstructDirectoryEntries(Connection* Base, const Core::String& NameA, const Core::FileEntry& A, const Core::String& NameB, const Core::FileEntry& B);
				static Core::String ConstructContentRange(size_t Offset, size_t Length, size_t ContentLength);
			};

			class VI_OUT_TS Parsing
			{
			public:
				static bool ParseMultipartHeaderField(Parser* Target, const uint8_t* Name, size_t Length);
				static bool ParseMultipartHeaderValue(Parser* Target, const uint8_t* Name, size_t Length);
				static bool ParseMultipartContentData(Parser* Target, const uint8_t* Name, size_t Length);
				static bool ParseMultipartResourceBegin(Parser* Target);
				static bool ParseMultipartResourceEnd(Parser* Target);
				static bool ParseHeaderField(Parser* Target, const uint8_t* Name, size_t Length);
				static bool ParseHeaderValue(Parser* Target, const uint8_t* Name, size_t Length);
				static bool ParseVersion(Parser* Target, const uint8_t* Name, size_t Length);
				static bool ParseStatusCode(Parser* Target, size_t Length);
				static bool ParseStatusMessage(Parser* Target, const uint8_t* Name, size_t Length);
				static bool ParseMethodValue(Parser* Target, const uint8_t* Name, size_t Length);
				static bool ParsePathValue(Parser* Target, const uint8_t* Name, size_t Length);
				static bool ParseQueryValue(Parser* Target, const uint8_t* Name, size_t Length);
				static int ParseContentRange(const std::string_view& ContentRange, int64_t* Range1, int64_t* Range2);
				static Core::String ParseMultipartDataBoundary();
				static void ParseCookie(const std::string_view& Value);
			};

			class VI_OUT_TS Permissions
			{
			public:
				static Core::String Authorize(const std::string_view& Username, const std::string_view& Password, const std::string_view& Type = "Basic");
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
				static bool ResourceModified(Connection* Base, Core::FileEntry* Resource);
				static bool ResourceCompressed(Connection* Base, size_t Size);
			};

			class VI_OUT_TS Routing
			{
			public:
				static bool RouteWebSocket(Connection* Base);
				static bool RouteGet(Connection* Base);
				static bool RoutePost(Connection* Base);
				static bool RoutePut(Connection* Base);
				static bool RoutePatch(Connection* Base);
				static bool RouteDelete(Connection* Base);
				static bool RouteOptions(Connection* Base);
			};

			class VI_OUT_TS Logical
			{
			public:
				static bool ProcessDirectory(Connection* Base);
				static bool ProcessResource(Connection* Base);
				static bool ProcessResourceCompress(Connection* Base, bool Deflate, bool Gzip, const std::string_view& ContentRange, size_t Range);
				static bool ProcessResourceCache(Connection* Base);
				static bool ProcessFile(Connection* Base, size_t ContentLength, size_t Range);
				static bool ProcessFileStream(Connection* Base, FILE* Stream, size_t ContentLength, size_t Range);
				static bool ProcessFileChunk(Connection* Base, FILE* Stream, size_t ContentLength);
				static bool ProcessFileCompress(Connection* Base, size_t ContentLength, size_t Range, bool Gzip);
				static bool ProcessFileCompressChunk(Connection* Base, FILE* Stream, void* CStream, size_t ContentLength);
				static bool ProcessWebSocket(Connection* Base, const uint8_t* Key, size_t KeySize);
			};

			class VI_OUT_TS Server final : public SocketServer
			{
				friend Connection;
				friend Logical;
				friend Utils;

			public:
				Server();
				~Server() override;
				Core::ExpectsSystem<void> Update();

			private:
				Core::ExpectsSystem<void> UpdateRoute(RouterEntry* Route);
				Core::ExpectsSystem<void> OnConfigure(SocketRouter* New) override;
				Core::ExpectsSystem<void> OnUnlisten() override;
				void OnRequestOpen(SocketConnection* Base) override;
				bool OnRequestCleanup(SocketConnection* Base) override;
				void OnRequestStall(SocketConnection* Data) override;
				void OnRequestClose(SocketConnection* Base) override;
				SocketConnection* OnAllocate(SocketListener* Host) override;
				SocketRouter* OnAllocateRouter() override;
			};

			class VI_OUT Client final : public SocketClient
			{
			private:
				struct BoundaryBlock
				{
					Resource* File;
					Core::String Data;
					Core::String Finish;
					bool IsFinal;
				};

			private:
				Parser* Resolver;
				WebSocketFrame* WebSocket;
				RequestFrame Request;
				ResponseFrame Response;
				Core::Vector<BoundaryBlock> Boundaries;
				Core::ExpectsPromiseSystem<void> Future;

			public:
				Client(int64_t ReadTimeout);
				~Client() override;
				Core::ExpectsPromiseSystem<void> Skip();
				Core::ExpectsPromiseSystem<void> Fetch(size_t MaxSize = PAYLOAD_SIZE, bool Eat = false);
				Core::ExpectsPromiseSystem<void> Upgrade(RequestFrame&& Root);
				Core::ExpectsPromiseSystem<void> Send(RequestFrame&& Root);
				Core::ExpectsPromiseSystem<void> SendFetch(RequestFrame&& Root, size_t MaxSize = PAYLOAD_SIZE);
				Core::ExpectsPromiseSystem<Core::Unique<Core::Schema>> JSON(RequestFrame&& Root, size_t MaxSize = PAYLOAD_SIZE);
				Core::ExpectsPromiseSystem<Core::Unique<Core::Schema>> XML(RequestFrame&& Root, size_t MaxSize = PAYLOAD_SIZE);
				void Downgrade();
				WebSocketFrame* GetWebSocket();
				RequestFrame* GetRequest();
				ResponseFrame* GetResponse();

			private:
				Core::ExpectsSystem<void> OnReuse() override;
				Core::ExpectsSystem<void> OnDisconnect() override;
				void UploadFile(BoundaryBlock* Boundary, std::function<void(Core::ExpectsSystem<void>&&)>&& Callback);
				void UploadFileChunk(FILE* Stream, size_t ContentLength, std::function<void(Core::ExpectsSystem<void>&&)>&& Callback);
				void UploadFileChunkQueued(FILE* Stream, size_t ContentLength, std::function<void(Core::ExpectsSystem<void>&&)>&& Callback);
				void Upload(size_t FileId);
				void ManageKeepAlive();
				void Receive(const uint8_t* LeftoverBuffer, size_t LeftoverSize);
			};

			VI_OUT Core::ExpectsPromiseSystem<ResponseFrame> Fetch(const std::string_view& Location, const std::string_view& Method = "GET", const FetchFrame& Options = FetchFrame());
		}
	}
}
#endif
