#ifndef TH_REST_H
#define TH_REST_H
#pragma warning(disable: 4251)
#pragma warning(disable: 4996)
#include <thread>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <cstdarg>
#include <cstring>
#include <functional>
#include <vector>
#include <deque>
#include <string>
#include <condition_variable>
#include <atomic>
#if defined(_WIN32) || defined(_WIN64)
#ifndef TH_EXPORT
#define TH_OUT __declspec(dllimport)
#else
#define TH_OUT __declspec(dllexport)
#endif
#define TH_MICROSOFT
#define TH_FUNCTION __FUNCTION__
#ifdef _WIN64
#define TH_64
#else
#define TH_32
#endif
#define TH_MAX_PATH MAX_PATH
#define TH_STRINGIFY(X) #X
#define TH_FUNCTION __FUNCTION__
#define TH_CDECL __cdecl
#define TH_FILE (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define TH_LINE __LINE__
#undef TH_UNIX
#elif defined __linux__ && defined __GNUC__
#define TH_OUT
#define TH_UNIX
#define TH_MAX_PATH _POSIX_PATH_MAX
#define TH_STRINGIFY(X) #X
#define TH_FUNCTION __FUNCTION__
#define TH_CDECL
#define TH_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define TH_LINE __LINE__
#undef TH_MICROSOFT
#if __x86_64__ || __ppc64__
#define TH_64
#else
#define TH_32
#endif
#elif __APPLE__
#define TH_OUT
#define TH_UNIX
#define TH_MAX_PATH _POSIX_PATH_MAX
#define TH_STRINGIFY(X) #X
#define TH_FUNCTION __FUNCTION__
#define TH_CDECL
#define TH_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define TH_LINE __LINE__
#define TH_APPLE
#undef TH_MICROSOFT
#if __x86_64__ || __ppc64__
#define TH_64
#else
#define TH_32
#endif
#endif
#ifndef TH_MICROSOFT
#include <sys/types.h>
#endif
#ifdef max
#undef max
#endif
#ifdef TH_MICROSOFT
#ifdef TH_64
typedef uint64_t socket_t;
#elif defined(TH_32)
typedef int socket_t;
#endif
typedef int socket_size_t;
typedef void* epoll_handle;
#else
#include <sys/socket.h>
typedef int epoll_handle;
typedef int socket_t;
typedef socklen_t socket_size_t;
#endif
#if TH_DLEVEL >= 3
#define TH_INFO(Format, ...) Tomahawk::Rest::Debug::Log(3, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_WARN(Format, ...) Tomahawk::Rest::Debug::Log(2, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_ERROR(Format, ...) Tomahawk::Rest::Debug::Log(1, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#elif TH_DLEVEL >= 2
#define TH_INFO(Format, ...)
#define TH_WARN(Format, ...) Tomahawk::Rest::Debug::Log(2, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_ERROR(Format, ...) Tomahawk::Rest::Debug::Log(1, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#elif TH_DLEVEL >= 1
#define TH_INFO(Format, ...)
#define TH_WARN(Format, ...)
#define TH_ERROR(Format, ...) Tomahawk::Rest::Debug::Log(1, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#else
#define TH_INFO(...)
#define TH_WARN(...)
#define TH_ERROR(...)
#endif
#define TH_COUT extern "C" TH_OUT
#define TH_MALLOC(Size) Tomahawk::Rest::Mem::Malloc(Size)
#define TH_REALLOC(Ptr, Size) Tomahawk::Rest::Mem::Realloc(Ptr, Size)
#define TH_FREE(Ptr) Tomahawk::Rest::Mem::Free(Ptr)
#define TH_RELEASE(Ptr) { if (Ptr != nullptr) (Ptr)->Release(); }
#define TH_CLEAR(Ptr) { if (Ptr != nullptr) { (Ptr)->Release(); Ptr = nullptr; } }
#define TH_PREFIX_CHAR '@'
#define TH_PREFIX_STR "@"
#define TH_LOG(Format, ...) Tomahawk::Rest::Debug::Log(0, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_COMPONENT_HASH(Name) Tomahawk::Rest::OS::File::GetCheckSum(Name)
#define TH_COMPONENT_IS(Source, Name) (Source->GetId() == TH_COMPONENT_HASH(Name))
#define TH_COMPONENT(Name) \
virtual const char* GetName() { return Name; } \
virtual uint64_t GetId() { static uint64_t V = TH_COMPONENT_HASH(Name); return V; } \
static const char* GetTypeName() { return Name; } \
static uint64_t GetTypeId() { static uint64_t V = TH_COMPONENT_HASH(Name); return V; }

namespace Tomahawk
{
	namespace Rest
	{
		class EventWorker;

		class EventQueue;

		class Document;

		class Object;

		class Stream;

		class Var;

		enum FileMode
		{
			FileMode_Read_Only,
			FileMode_Write_Only,
			FileMode_Append_Only,
			FileMode_Read_Write,
			FileMode_Write_Read,
			FileMode_Read_Append_Write,
			FileMode_Binary_Read_Only,
			FileMode_Binary_Write_Only,
			FileMode_Binary_Append_Only,
			FileMode_Binary_Read_Write,
			FileMode_Binary_Write_Read,
			FileMode_Binary_Read_Append_Write
		};

		enum FileSeek
		{
			FileSeek_Begin,
			FileSeek_Current,
			FileSeek_End
		};

		enum EventState
		{
			EventState_Working,
			EventState_Idle,
			EventState_Terminated
		};

		enum EventWorkflow
		{
			EventWorkflow_Multithreaded,
			EventWorkflow_Singlethreaded,
			EventWorkflow_Ticked,
			EventWorkflow_Mixed
		};

		enum EventType
		{
			EventType_Events = (1 << 1),
			EventType_Tasks = (1 << 2),
			EventType_Timers = (1 << 3),
			EventType_Subscribe = (1 << 4),
			EventType_Unsubscribe = (1 << 5),
			EventType_Pull = (1 << 6)
		};

		enum VarType
		{
			VarType_Null,
			VarType_Undefined,
			VarType_Object,
			VarType_Array,
			VarType_Pointer,
			VarType_String,
			VarType_Base64,
			VarType_Integer,
			VarType_Number,
			VarType_Decimal,
			VarType_Boolean
		};

		enum VarForm
		{
			VarForm_Dummy,
			VarForm_Tab_Decrease,
			VarForm_Tab_Increase,
			VarForm_Write_Space,
			VarForm_Write_Line,
			VarForm_Write_Tab,
		};

		typedef std::vector<struct Variant> VariantList;
		typedef std::unordered_map<std::string, struct Variant> VariantArgs;
		typedef std::function<void(class EventQueue*, VariantArgs&)> EventCallback;
		typedef std::function<void(class EventQueue*)> TaskCallback;
		typedef std::function<void(class EventQueue*)> TimerCallback;
		typedef std::function<void(VarForm, const char*, int64_t)> NWriteCallback;
		typedef std::function<bool(char*, int64_t)> NReadCallback;
		typedef uint64_t EventId;

		struct TH_OUT Variant
		{
			friend Document;
			friend Var;

		private:
			struct String
			{
				char* Buffer;
				size_t Size;
			};

		private:
			VarType Type;
			char* Data;

		public:
			Variant();
			Variant(const Variant& Other);
			Variant(Variant&& Other);
			~Variant();
			bool Deserialize(const std::string& Value, bool Strict = false);
			std::string Serialize() const;
			std::string GetDecimal() const;
			std::string GetBlob() const;
			void* GetPointer() const;
			const char* GetString() const;
			unsigned char* GetBase64() const;
			int64_t GetInteger() const;
			double GetNumber() const;
			bool GetBoolean() const;
			VarType GetType() const;
			size_t GetSize() const;
			Variant& operator= (const Variant& Other);
			Variant& operator= (Variant&& Other);
			bool operator== (const Variant& Other) const;
			bool operator!= (const Variant& Other) const;
			operator bool() const;
			bool IsObject() const;
			bool IsEmpty() const;

		private:
			Variant(VarType NewType, char* NewData);
			bool Is(const Variant& Value) const;
			void Copy(const Variant& Other);
			void Copy(Variant&& Other);
			void Free();
		};

		struct TH_OUT FileState
		{
			uint64_t Size = 0;
			uint64_t Links = 0;
			uint64_t Permissions = 0;
			uint64_t IDocument = 0;
			uint64_t Device = 0;
			uint64_t UserId = 0;
			uint64_t GroupId = 0;
			int64_t LastAccess = 0;
			int64_t LastPermissionChange = 0;
			int64_t LastModified = 0;
			bool Exists = false;
		};

		struct TH_OUT EventBase
		{
			std::string Name;
			VariantArgs Args;

			EventBase(const std::string& NewName);
			EventBase(const std::string& NewName, const VariantArgs& NewArgs);
			EventBase(const std::string& NewName, VariantArgs&& NewArgs);
			EventBase(const EventBase& Other);
			EventBase(EventBase&& Other);
			EventBase& operator= (const EventBase& Other);
			EventBase& operator= (EventBase&& Other);
		};

		struct TH_OUT EventTask
		{
			TaskCallback Callback;
			bool Alive;

			EventTask(const TaskCallback& NewCallback, bool NewAlive);
			EventTask(TaskCallback&& NewCallback, bool NewAlive);
			EventTask(const EventTask& Other);
			EventTask(EventTask&& Other);
			EventTask& operator= (const EventTask& Other);
			EventTask& operator= (EventTask&& Other);
		};

		struct TH_OUT EventTimer
		{
			TimerCallback Callback;
			uint64_t Timeout;
			int64_t Time;
			EventId Id;
			bool Alive;

			EventTimer(const TimerCallback& NewCallback, uint64_t NewTimeout, int64_t NewTime, EventId NewId, bool NewAlive);
			EventTimer(TimerCallback&& NewCallback, uint64_t NewTimeout, int64_t NewTime, EventId NewId, bool NewAlive);
			EventTimer(const EventTimer& Other);
			EventTimer(EventTimer&& Other);
			EventTimer& operator= (const EventTimer& Other);
			EventTimer& operator= (EventTimer&& Other);
		};

		struct TH_OUT EventListener
		{
			std::unordered_map<EventId, EventCallback> Callbacks;
			EventId Counter = 0;
		};

		struct TH_OUT Resource
		{
			uint64_t Size = 0;
			int64_t LastModified = 0;
			int64_t CreationTime = 0;
			bool IsReferenced = false;
			bool IsDirectory = false;
		};

		struct TH_OUT ResourceEntry
		{
			std::string Path;
			Resource Source;
			void* UserData = nullptr;
		};

		struct TH_OUT DirectoryEntry
		{
			std::string Path;
			bool IsDirectory = false;
			bool IsGood = false;
			uint64_t Length = 0;
		};

		struct TH_OUT ChildProcess
		{
			friend class OS;

		private:
			bool Valid = false;

		public:
#ifdef TH_MICROSOFT
			void* Process = nullptr;
			void* Thread = nullptr;
			void* Job = nullptr;
#else
			pid_t Process;
#endif
		};

		struct TH_OUT DateTime
		{
		private:
			std::chrono::system_clock::duration Time;
			struct tm DateValue;
			bool DateRebuild = false;

		public:
			DateTime();
			DateTime(const DateTime& Value);
			void operator +=(const DateTime& Right);
			void operator -=(const DateTime& Right);
			bool operator >=(const DateTime& Right);
			bool operator <=(const DateTime& Right);
			bool operator >(const DateTime& Right);
			bool operator <(const DateTime& Right);
			std::string Format(const std::string& Value);
			std::string Date(const std::string& Value);
			DateTime Now();
			DateTime FromNanoseconds(uint64_t Value);
			DateTime FromMicroseconds(uint64_t Value);
			DateTime FromMilliseconds(uint64_t Value);
			DateTime FromSeconds(uint64_t Value);
			DateTime FromMinutes(uint64_t Value);
			DateTime FromHours(uint64_t Value);
			DateTime FromDays(uint64_t Value);
			DateTime FromWeeks(uint64_t Value);
			DateTime FromMonths(uint64_t Value);
			DateTime FromYears(uint64_t Value);
			DateTime operator +(const DateTime& Right);
			DateTime operator -(const DateTime& Right);
			DateTime& SetDateSeconds(uint64_t Value, bool NoFlush = false);
			DateTime& SetDateMinutes(uint64_t Value, bool NoFlush = false);
			DateTime& SetDateHours(uint64_t Value, bool NoFlush = false);
			DateTime& SetDateDay(uint64_t Value, bool NoFlush = false);
			DateTime& SetDateWeek(uint64_t Value, bool NoFlush = false);
			DateTime& SetDateMonth(uint64_t Value, bool NoFlush = false);
			DateTime& SetDateYear(uint64_t Value, bool NoFlush = false);
			uint64_t Nanoseconds();
			uint64_t Microseconds();
			uint64_t Milliseconds();
			uint64_t Seconds();
			uint64_t Minutes();
			uint64_t Hours();
			uint64_t Days();
			uint64_t Weeks();
			uint64_t Months();
			uint64_t Years();

		private:
			void Rebuild();

		public:
			static std::string GetGMTBasedString(int64_t TimeStamp);
			static bool TimeFormatGMT(char* Buffer, uint64_t Length, int64_t Time);
			static bool TimeFormatLCL(char* Buffer, uint64_t Length, int64_t Time);
			static int64_t ReadGMTBasedString(const char* Date);
		};

		struct TH_OUT Stroke
		{
		public:
			struct Settle
			{
				uint64_t Start;
				uint64_t End;
				bool Found;
			};

		private:
			std::string* L;
			bool Safe;

		public:
			Stroke();
			Stroke(int Value);
			Stroke(unsigned int Value);
			Stroke(int64_t Value);
			Stroke(uint64_t Value);
			Stroke(float Value);
			Stroke(double Value);
			Stroke(long double Value);
			Stroke(const std::string& Buffer);
			Stroke(std::string* Buffer);
			Stroke(const std::string* Buffer);
			Stroke(const char* Buffer);
			Stroke(const char* Buffer, int64_t Length);
			Stroke(const Stroke& Value);
			~Stroke();
			Stroke& EscapePrint();
			Stroke& Escape();
			Stroke& Unescape();
			Stroke& Reserve(uint64_t Count = 1);
			Stroke& Resize(uint64_t Count);
			Stroke& Resize(uint64_t Count, char Char);
			Stroke& Clear();
			Stroke& ToUtf8();
			Stroke& ToUpper();
			Stroke& ToLower();
			Stroke& Clip(uint64_t Length);
			Stroke& ReplaceOf(const char* Chars, const char* To, uint64_t Start = 0U);
			Stroke& ReplaceNotOf(const char* Chars, const char* To, uint64_t Start = 0U);
			Stroke& Replace(const std::string& From, const std::string& To, uint64_t Start = 0U);
			Stroke& Replace(const char* From, const char* To, uint64_t Start = 0U);
			Stroke& Replace(const char& From, const char& To, uint64_t Position = 0U);
			Stroke& Replace(const char& From, const char& To, uint64_t Position, uint64_t Count);
			Stroke& ReplacePart(uint64_t Start, uint64_t End, const std::string& Value);
			Stroke& ReplacePart(uint64_t Start, uint64_t End, const char* Value);
			Stroke& RemovePart(uint64_t Start, uint64_t End);
			Stroke& Reverse();
			Stroke& Reverse(uint64_t Start, uint64_t End);
			Stroke& Substring(uint64_t Start);
			Stroke& Substring(uint64_t Start, uint64_t Count);
			Stroke& Substring(const Stroke::Settle& Result);
			Stroke& Splice(uint64_t Start, uint64_t End);
			Stroke& Trim();
			Stroke& Fill(const char& Char);
			Stroke& Fill(const char& Char, uint64_t Count);
			Stroke& Fill(const char& Char, uint64_t Start, uint64_t Count);
			Stroke& Assign(const char* Raw);
			Stroke& Assign(const char* Raw, uint64_t Length);
			Stroke& Assign(const std::string& Raw);
			Stroke& Assign(const std::string& Raw, uint64_t Start, uint64_t Count);
			Stroke& Assign(const char* Raw, uint64_t Start, uint64_t Count);
			Stroke& Append(const char* Raw);
			Stroke& Append(const char& Char);
			Stroke& Append(const char& Char, uint64_t Count);
			Stroke& Append(const std::string& Raw);
			Stroke& Append(const char* Raw, uint64_t Count);
			Stroke& Append(const char* Raw, uint64_t Start, uint64_t Count);
			Stroke& Append(const std::string& Raw, uint64_t Start, uint64_t Count);
			Stroke& fAppend(const char* Format, ...);
			Stroke& Insert(const std::string& Raw, uint64_t Position);
			Stroke& Insert(const std::string& Raw, uint64_t Position, uint64_t Start, uint64_t Count);
			Stroke& Insert(const std::string& Raw, uint64_t Position, uint64_t Count);
			Stroke& Insert(const char& Char, uint64_t Position, uint64_t Count);
			Stroke& Insert(const char& Char, uint64_t Position);
			Stroke& Erase(uint64_t Position);
			Stroke& Erase(uint64_t Position, uint64_t Count);
			Stroke& EraseOffsets(uint64_t Start, uint64_t End);
			Stroke& Path(const std::string& Net, const std::string& Dir);
			Stroke::Settle ReverseFind(const std::string& Needle, uint64_t Offset = 0U) const;
			Stroke::Settle ReverseFind(const char* Needle, uint64_t Offset = 0U) const;
			Stroke::Settle ReverseFind(const char& Needle, uint64_t Offset = 0U) const;
			Stroke::Settle ReverseFindUnescaped(const char& Needle, uint64_t Offset = 0U) const;
			Stroke::Settle ReverseFindOf(const std::string& Needle, uint64_t Offset = 0U) const;
			Stroke::Settle ReverseFindOf(const char* Needle, uint64_t Offset = 0U) const;
			Stroke::Settle Find(const std::string& Needle, uint64_t Offset = 0U) const;
			Stroke::Settle Find(const char* Needle, uint64_t Offset = 0U) const;
			Stroke::Settle Find(const char& Needle, uint64_t Offset = 0U) const;
			Stroke::Settle FindUnescaped(const char& Needle, uint64_t Offset = 0U) const;
			Stroke::Settle FindOf(const std::string& Needle, uint64_t Offset = 0U) const;
			Stroke::Settle FindOf(const char* Needle, uint64_t Offset = 0U) const;
			Stroke::Settle FindNotOf(const std::string& Needle, uint64_t Offset = 0U) const;
			Stroke::Settle FindNotOf(const char* Needle, uint64_t Offset = 0U) const;
			bool StartsWith(const std::string& Value, uint64_t Offset = 0U) const;
			bool StartsWith(const char* Value, uint64_t Offset = 0U) const;
			bool StartsOf(const char* Value, uint64_t Offset = 0U) const;
			bool StartsNotOf(const char* Value, uint64_t Offset = 0U) const;
			bool EndsWith(const std::string& Value) const;
			bool EndsOf(const char* Value) const;
			bool EndsNotOf(const char* Value) const;
			bool EndsWith(const char* Value) const;
			bool EndsWith(const char& Value) const;
			bool Empty() const;
			bool HasInteger() const;
			bool HasNumber() const;
			bool HasDecimal() const;
			bool ToBoolean() const;
			int ToInt() const;
			long ToLong() const;
			float ToFloat() const;
			unsigned int ToUInt() const;
			unsigned long ToULong() const;
			int64_t ToInt64() const;
			double ToDouble() const;
			long double ToLDouble() const;
			uint64_t ToUInt64() const;
			uint64_t Size() const;
			uint64_t Capacity() const;
			char* Value() const;
			const char* Get() const;
			std::string& R();
			std::basic_string<wchar_t> ToUnicode() const;
			std::vector<std::string> Split(const std::string& With, uint64_t Start = 0U) const;
			std::vector<std::string> Split(char With, uint64_t Start = 0U) const;
			std::vector<std::string> SplitMax(char With, uint64_t MaxCount, uint64_t Start = 0U) const;
			std::vector<std::string> SplitOf(const char* With, uint64_t Start = 0U) const;
			std::vector<std::string> SplitNotOf(const char* With, uint64_t Start = 0U) const;
			Stroke& operator = (const Stroke& New);

		public:
			static bool IsDigit(char Char);
			static int CaseCompare(const char* Value1, const char* Value2);
			static int Match(const char* Pattern, const char* Text);
			static int Match(const char* Pattern, uint64_t Length, const char* Text);
			static std::string ToStringAutoPrec(float Number);
			static std::string ToStringAutoPrec(double Number);
		};

		struct TH_OUT TickTimer
		{
		private:
			double Time;

		public:
			double Delay;

		public:
			TickTimer();
			bool TickEvent(double ElapsedTime);
			double GetTime();
		};

		struct TH_OUT SpinLock
		{
		private:
			std::atomic_flag Atom;

		public:
			SpinLock();
			void Acquire();
			void Release();
		};

		class TH_OUT Var
		{
		public:
			static Variant Auto(const std::string& Value, bool Strict = false);
			static Variant Null();
			static Variant Undefined();
			static Variant Object();
			static Variant Array();
			static Variant Pointer(void* Value);
			static Variant String(const std::string& Value);
			static Variant String(const char* Value, size_t Size);
			static Variant Base64(const std::string& Value);
			static Variant Base64(const unsigned char* Value, size_t Size);
			static Variant Base64(const char* Value, size_t Size);
			static Variant Integer(int64_t Value);
			static Variant Number(double Value);
			static Variant Decimal(const std::string& Value);
			static Variant Decimal(const char* Value, size_t Size);
			static Variant Boolean(bool Value);
		};

		class TH_OUT Mem
		{
		private:
			struct MemoryPage
			{
				uint64_t Size;
				bool Allocated;
				char Data;
			};

		private:
			static MemoryPage* Heap;
			static std::mutex* Mutex;
			static SpinLock Atom;
			static uint64_t HeadSize;
			static uint64_t HeapSize;

		public:
			static void Create(size_t InitialSize);
			static void Release();
			static void* Malloc(size_t Size);
			static void* Realloc(void* Ptr, size_t Size);
			static void Free(void* Ptr);
			static void Interrupt();

		private:
			static void ConcatSequentialPages(MemoryPage* Block, bool IsAllocated);
			static MemoryPage* FindFirstPage(uint64_t MinSize);
			static void SplitPage(MemoryPage* Block, uint64_t Size);
		};

		class TH_OUT OS
		{
		public:
			class TH_OUT Directory
			{
			public:
				static void Set(const char* Path);
				static void Patch(const std::string& Path);
				static bool Scan(const std::string& Path, std::vector<ResourceEntry>* Entries);
				static bool Each(const char* Path, const std::function<bool(DirectoryEntry*)>& Callback);
				static bool Create(const char* Path);
				static bool Remove(const char* Path);
				static bool IsExists(const char* Path);
				static std::string Get();
				static std::vector<std::string> GetMounts();
			};

			class TH_OUT File
			{
			public:
				static bool Write(const char* Path, const char* Data, uint64_t Length);
				static bool Write(const char* Path, const std::string& Data);
				static bool State(const std::string& Path, Resource* Resource);
				static bool Move(const char* From, const char* To);
				static bool Remove(const char* Path);
				static bool IsExists(const char* Path);
				static uint64_t GetCheckSum(const std::string& Data);
				static FileState GetState(const char* Path);
				static Stream* Open(const std::string& Path, FileMode Mode);
				static void* Open(const char* Path, const char* Mode);
				static unsigned char* ReadChunk(Stream* Stream, uint64_t Length);
				static unsigned char* ReadAll(const char* Path, uint64_t* ByteLength);
				static unsigned char* ReadAll(Stream* Stream, uint64_t* ByteLength);
				static std::string ReadAsString(const char* Path);
				static std::vector<std::string> ReadAsArray(const char* Path);
			};

			class TH_OUT Path
			{
			public:
				static bool IsRemote(const char* Path);
				static std::string Resolve(const char* Path);
				static std::string Resolve(const std::string& Path, const std::string& Directory);
				static std::string ResolveDirectory(const char* Path);
				static std::string ResolveDirectory(const std::string& Path, const std::string& Directory);
				static std::string GetDirectory(const char* Path, uint32_t Level = 0);
				static const char* GetFilename(const char* Path);
				static const char* GetExtension(const char* Path);
			};

			class TH_OUT Net
			{
			public:
				static bool SendFile(FILE* Stream, socket_t Socket, int64_t Size);
				static bool GetETag(char* Buffer, uint64_t Length, Resource* Resource);
				static bool GetETag(char* Buffer, uint64_t Length, int64_t LastModified, uint64_t ContentLength);
				static socket_t GetFd(FILE* Stream);
			};

			class TH_OUT Process
			{
			public:
				static void Execute(const char* Format, ...);
				static bool Spawn(const std::string& Path, const std::vector<std::string>& Params, ChildProcess* Result);
				static bool Await(ChildProcess* Process, int* ExitCode);
				static bool Free(ChildProcess* Process);
			};

			class TH_OUT Symbol
			{
			public:
				static void* Load(const std::string& Path = "");
				static void* LoadFunction(void* Handle, const std::string& Name);
				static bool Unload(void* Handle);
			};

			class TH_OUT Input
			{
			public:
				static bool Text(const std::string& Title, const std::string& Message, const std::string& DefaultInput, std::string* Result);
				static bool Password(const std::string& Title, const std::string& Message, std::string* Result);
				static bool Save(const std::string& Title, const std::string& DefaultPath, const std::string& Filter, const std::string& FilterDescription, std::string* Result);
				static bool Open(const std::string& Title, const std::string& DefaultPath, const std::string& Filter, const std::string& FilterDescription, bool Multiple, std::string* Result);
				static bool Folder(const std::string& Title, const std::string& DefaultPath, std::string* Result);
				static bool Color(const std::string& Title, const std::string& DefaultHexRGB, std::string* Result);
			};

			class TH_OUT Error
			{
			public:
				static int Get();
				static std::string GetName(int Code);
			};
		};

		class TH_OUT Debug
		{
		private:
			static std::function<void(const char*, int)> Callback;
			static bool Enabled;

		public:
			static void Log(int Level, int Line, const char* Source, const char* Format, ...);
			static void AttachCallback(const std::function<void(const char*, int)>& Callback);
			static void AttachStream();
			static void DetachCallback();
			static void DetachStream();
		};

		class TH_OUT Composer
		{
		private:
			static std::unordered_map<uint64_t, void*>* Factory;

		public:
			static void AddRef(Object* Value);
			static void SetFlag(Object* Value);
			static void Release(Object* Value);
			static int GetRefCount(Object* Value);
			static bool GetFlag(Object* Value);
			static bool Clear();
			static bool Pop(const std::string& Hash);

		public:
			template <typename T, typename... Args>
			static T* Create(const std::string& Hash, Args... Data)
			{
				return Create<T, Args...>(TH_COMPONENT_HASH(Hash), Data...);
			}
			template <typename T, typename... Args>
			static T* Create(uint64_t Id, Args... Data)
			{
				if (!Factory)
					return nullptr;

				auto It = Factory->find(Id);
				if (It == Factory->end() || !It->second)
					return nullptr;

				void*(*Callable)(Args...) = nullptr;
				reinterpret_cast<void*&>(Callable) = It->second;

				if (!Callable)
					return nullptr;

				return (T*)Callable(Data...);
			}
			template <typename T, typename... Args>
			static void Push()
			{
				if (!Factory)
					Factory = new std::unordered_map<uint64_t, void*>();

				if (Factory->find(T::GetTypeId()) != Factory->end())
					return TH_ERROR("type \"%s\" already exists in composer's table", T::GetTypeName());

				auto Callable = &Composer::Callee<T, Args...>;
				void* Result = reinterpret_cast<void*&>(Callable);
				(*Factory)[T::GetTypeId()] = Result;
			}

		private:
			template <typename T, typename... Args>
			static void* Callee(Args... Data)
			{
				return (void*)new T(Data...);
			}
		};

		class TH_OUT Object
		{
			friend class Mem;

		private:
			std::atomic<int> __vcnt;
			std::atomic<bool> __vflg;

		public:
			Object() noexcept;
			virtual ~Object() noexcept;
			void operator delete(void* Data) noexcept;
			void* operator new(size_t Size) noexcept;
			void SetFlag() noexcept;
			bool GetFlag() noexcept;
			int GetRefCount() noexcept;
			Object* AddRef() noexcept;
			Object* Release() noexcept;

		public:
			template <typename T>
			T* As() noexcept
			{
				return (T*)this;
			}
		};

		class TH_OUT Console : public Object
		{
		protected:
			bool Handle;
			double Time;
			std::mutex Lock;

		public:
			Console();
			virtual ~Console() override;
			void Hide();
			void Show();
			void Clear();
			void Flush();
			void FlushWrite();
			void CaptureTime();
			void WriteLine(const std::string& Line);
			void Write(const std::string& Line);
			void fWriteLine(const char* Format, ...);
			void fWrite(const char* Format, ...);
			void sWriteLine(const std::string& Line);
			void sWrite(const std::string& Line);
			void sfWriteLine(const char* Format, ...);
			void sfWrite(const char* Format, ...);
			double GetCapturedTime();
			std::string Read(uint64_t Size);

		public:
			static Console* Get();
			static void Trace(const char* Format, ...);

		public:
			template <typename T>
			T* GetHandle()
			{
				return Handle;
			}

		private:
			static Console* Singleton;
		};

		class TH_OUT Timer : public Object
		{
		private:
			double TimeIncrement;
			double TickCounter;
			double FrameCount;
			double CapturedTime;
			double DeltaTimeLimit;
			double TimeStepLimit;
			void* TimeLimit;
			void* Frequency;
			void* PastTime;

		public:
			double FrameRelation;
			double FrameLimit;

		public:
			Timer();
			virtual ~Timer() override;
			void SetStepLimitation(double MaxFrames, double MinFrames);
			void Synchronize();
			void CaptureTime();
			void Sleep(uint64_t MilliSecs);
			double GetTimeIncrement();
			double GetTickCounter();
			double GetFrameCount();
			double GetElapsedTime();
			double GetCapturedTime();
			double GetDeltaTime();
			double GetTimeStep();
		};

		class TH_OUT Stream : public Object
		{
		protected:
			std::string Path;

		public:
			Stream();
			virtual ~Stream() = default;
			virtual void Clear() = 0;
			virtual bool Open(const char* File, FileMode Mode) = 0;
			virtual bool Close() = 0;
			virtual bool Seek(FileSeek Mode, int64_t Offset) = 0;
			virtual bool Move(int64_t Offset) = 0;
			virtual int Flush() = 0;
			virtual uint64_t ReadAny(const char* Format, ...) = 0;
			virtual uint64_t Read(char* Buffer, uint64_t Length) = 0;
			virtual uint64_t WriteAny(const char* Format, ...) = 0;
			virtual uint64_t Write(const char* Buffer, uint64_t Length) = 0;
			virtual uint64_t Tell() = 0;
			virtual int GetFd() = 0;
			virtual void* GetBuffer() = 0;
			uint64_t GetSize();
			std::string& GetSource();
		};

		class TH_OUT FileStream : public Stream
		{
		protected:
			FILE* Resource;

		public:
			FileStream();
			virtual ~FileStream() override;
			virtual void Clear();
			virtual bool Open(const char* File, FileMode Mode);
			virtual bool Close();
			virtual bool Seek(FileSeek Mode, int64_t Offset);
			virtual bool Move(int64_t Offset);
			virtual int Flush();
			virtual uint64_t ReadAny(const char* Format, ...);
			virtual uint64_t Read(char* Buffer, uint64_t Length);
			virtual uint64_t WriteAny(const char* Format, ...);
			virtual uint64_t Write(const char* Buffer, uint64_t Length);
			virtual uint64_t Tell();
			virtual int GetFd();
			virtual void* GetBuffer();
		};

		class TH_OUT GzStream : public Stream
		{
		protected:
			void* Resource;

		public:
			GzStream();
			virtual ~GzStream() override;
			virtual void Clear();
			virtual bool Open(const char* File, FileMode Mode);
			virtual bool Close();
			virtual bool Seek(FileSeek Mode, int64_t Offset);
			virtual bool Move(int64_t Offset);
			virtual int Flush();
			virtual uint64_t ReadAny(const char* Format, ...);
			virtual uint64_t Read(char* Buffer, uint64_t Length);
			virtual uint64_t WriteAny(const char* Format, ...);
			virtual uint64_t Write(const char* Buffer, uint64_t Length);
			virtual uint64_t Tell();
			virtual int GetFd();
			virtual void* GetBuffer();
		};

		class TH_OUT WebStream : public Stream
		{
		protected:
			void* Resource;
			std::string Buffer;
			uint64_t Offset;
			uint64_t Size;

		public:
			WebStream();
			virtual ~WebStream() override;
			virtual void Clear();
			virtual bool Open(const char* File, FileMode Mode);
			virtual bool Close();
			virtual bool Seek(FileSeek Mode, int64_t Offset);
			virtual bool Move(int64_t Offset);
			virtual int Flush();
			virtual uint64_t ReadAny(const char* Format, ...);
			virtual uint64_t Read(char* Buffer, uint64_t Length);
			virtual uint64_t WriteAny(const char* Format, ...);
			virtual uint64_t Write(const char* Buffer, uint64_t Length);
			virtual uint64_t Tell();
			virtual int GetFd();
			virtual void* GetBuffer();
		};

		class TH_OUT ChangeLog : public Object
		{
		private:
			std::string LastValue;
			uint64_t Offset;

		public:
			Stream* Source = nullptr;
			std::string Path, Name;

		public:
			ChangeLog(const std::string& Root);
			virtual ~ChangeLog() override;
			void Process(const std::function<bool(ChangeLog*, const char*, int64_t)>& Callback);
		};

		class TH_OUT FileTree : public Object
		{
		public:
			std::vector<FileTree*> Directories;
			std::vector<std::string> Files;
			std::string Path;

		public:
			FileTree(const std::string& Path);
			virtual ~FileTree() override;
			void Loop(const std::function<bool(FileTree*)>& Callback);
			FileTree* Find(const std::string& Path);
			uint64_t GetFiles();
		};

		class TH_OUT EventWorker : public Object
		{
			friend EventQueue;

		private:
			EventQueue* Queue;
			std::thread Thread;

		private:
			EventWorker(EventQueue* Value);
			virtual ~EventWorker() override;
			bool Loop();
		};

		class TH_OUT EventQueue : public Object
		{
			friend EventWorker;

		private:
			struct
			{
				std::vector<EventWorker*> Workers;
				std::condition_variable Condition;
				std::thread Thread[2];
				std::mutex Safe;
			} Async;

			struct
			{
				std::mutex Events;
				std::mutex Tasks;
				std::mutex Timers;
				std::mutex Listeners;
			} Sync;

		private:
			std::unordered_map<std::string, EventListener> Listeners;
			std::vector<EventTimer> Timers;
			std::deque<EventTask> Tasks;
			std::deque<EventBase> Events;
			EventState State = EventState_Terminated;
			EventId Timer = 0;
			int Synchronize = 0;

		public:
			void* UserData = nullptr;

		public:
			EventQueue();
			virtual ~EventQueue() override;
			void SetState(EventState NewState);
			EventId SetInterval(uint64_t Milliseconds, const TimerCallback& Callback);
			EventId SetInterval(uint64_t Milliseconds, TimerCallback&& Callback);
			EventId SetTimeout(uint64_t Milliseconds, const TimerCallback& Callback);
			EventId SetTimeout(uint64_t Milliseconds, TimerCallback&& Callback);
			EventId SetListener(const std::string& Name, const EventCallback& Callback);
			EventId SetListener(const std::string& Name, EventCallback&& Callback);
			bool SetTask(const TaskCallback& Callback, bool Looped = false);
			bool SetTask(TaskCallback&& Callback, bool Looped = false);
			bool SetEvent(const std::string& Name, const VariantArgs& Args);
			bool SetEvent(const std::string& Name, VariantArgs&& Args);
			bool SetEvent(const std::string& Name);
			bool ClearListener(const std::string& Name, EventId ListenerId);
			bool ClearTimeout(EventId TimerId);
			bool Clear(EventType Type, bool NoCall);
			bool Start(EventWorkflow Workflow, uint64_t Workers);
			bool Tick();
			bool Stop();
			bool IsBlockable();
			EventState GetState();

		private:
			bool LoopMixed();
			bool LoopTasks();
			bool LoopCalls();
			bool CallTask();
			bool CallEvent();
			bool CallTimer(int64_t Time);
			int64_t GetClock();
		};

		class TH_OUT Document : public Object
		{
		protected:
			std::vector<Document*> Nodes;
			Document* Parent;
			bool Saved;

		public:
			std::string Key;
			Variant Value;

		public:
			Document(const Variant& Base);
			Document(Variant&& Base);
			virtual ~Document() override;
			std::unordered_map<std::string, uint64_t> GetNames() const;
			std::vector<Document*> FindCollection(const std::string& Name, bool Deep = false) const;
			std::vector<Document*> FetchCollection(const std::string& Notation, bool Deep = false) const;
			std::vector<Document*> GetAttributes() const;
			std::vector<Document*>* GetNodes();
			Document* Find(const std::string& Name, bool Deep = false) const;
			Document* Fetch(const std::string& Notation, bool Deep = false) const;
			Variant GetVar(size_t Index) const;
			Variant GetVar(const std::string& Key) const;
			Document* GetParent() const;
			Document* GetAttribute(const std::string& Key) const;
			Document* Get(size_t Index) const;
			Document* Get(const std::string& Key) const;
			Document* Set(const std::string& Key);
			Document* Set(const std::string& Key, const Variant& Value);
			Document* Set(const std::string& Key, Variant&& Value);
			Document* Set(const std::string& Key, Document* Value);
			Document* SetAttribute(const std::string& Key, const Variant& Value);
			Document* SetAttribute(const std::string& Key, Variant&& Value);
			Document* Push(const Variant& Value);
			Document* Push(Variant&& Value);
			Document* Push(Document* Value);
			Document* Pop(size_t Index);
			Document* Pop(const std::string& Name);
			Document* Copy() const;
			bool Has(const std::string& Name) const;
			bool Has64(const std::string& Name, size_t Size = 12) const;
			bool IsAttribute() const;
			bool IsSaved() const;
			int64_t Size() const;
			std::string GetName() const;
			void Join(Document* Other);
			void Clear();
			void Save();

		public:
			static Document* Object();
			static Document* Array();
			static bool WriteXML(Document* Value, const NWriteCallback& Callback);
			static bool WriteJSON(Document* Value, const NWriteCallback& Callback);
			static bool WriteJSONB(Document* Value, const NWriteCallback& Callback);
			static Document* ReadXML(int64_t Size, const NReadCallback& Callback, bool Assert = true);
			static Document* ReadJSON(int64_t Size, const NReadCallback& Callback, bool Assert = true);
			static Document* ReadJSONB(const NReadCallback& Callback, bool Assert = true);

		private:
			static bool ProcessXMLRead(void* Base, Document* Current);
			static bool ProcessJSONRead(void* Base, Document* Current);
			static bool ProcessJSONBWrite(Document* Current, std::unordered_map<std::string, uint64_t>* Map, const NWriteCallback& Callback);
			static bool ProcessJSONBRead(Document* Current, std::unordered_map<uint64_t, std::string>* Map, const NReadCallback& Callback);
			static bool ProcessNames(const Document* Current, std::unordered_map<std::string, uint64_t>* Map, uint64_t& Index);
		};

		template <class T>
		class Pool
		{
		public:
			typedef T* Iterator;

		protected:
			uint64_t Count, Volume;
			T* Data;

		public:
			Pool() : Count(0), Volume(0), Data(nullptr)
			{
				Reserve(1);
			}
			Pool(uint64_t _Size, uint64_t _Capacity, T* _Data) : Count(_Size), Volume(_Capacity), Data(_Data)
			{
				if (!Data && Volume > 0)
					Reserve(Volume);
				else if (!Data || !Volume)
					Reserve(1);
			}
			Pool(uint64_t _Capacity) : Count(0), Volume(0), Data(nullptr)
			{
				if (_Capacity > 0)
					Reserve(_Capacity);
				else
					Reserve(1);
			}
			Pool(const Pool<T>& Ref) : Count(0), Volume(0), Data(nullptr)
			{
				if (Ref.Data != nullptr)
					Copy(Ref);
				else
					Reserve(1);
			}
			~Pool()
			{
				Release();
			}
			void Swap(const Pool<T>& Raw)
			{
				uint64_t _Size = Raw.Count;
				Raw.Count = Count;
				Count = _Size;

				uint64_t _Capacity = Raw.Volume;
				Raw.Volume = Volume;
				Volume = _Capacity;

				T* _Data = Raw.Data;
				if (!Raw.Data)
					Release();
				else if (!Data)
					Raw.Release();

				Raw.Data = Data;
				Data = _Data;
			}
			void Reserve(uint64_t NewCount)
			{
				if (NewCount <= Volume)
					return;

				Volume = NewCount;
				T* Raw = (T*)TH_MALLOC((size_t)(Volume * SizeOf(Data)));
				memset(Raw, 0, (size_t)(Volume * SizeOf(Data)));

				if (!Assign(Begin(), End(), Raw))
					memcpy(Raw, Data, (size_t)(Count * SizeOf(Data)));

				if (Data != nullptr)
				{
					for (auto It = Begin(); It != End(); It++)
						Dispose(It);

					TH_FREE(Data);
				}
				Data = Raw;
			}
			void Release()
			{
				if (Data != nullptr)
				{
					for (auto It = Begin(); It != End(); It++)
						Dispose(It);

					TH_FREE(Data);
					Data = nullptr;
				}

				Count = Volume = 0;
			}
			void Resize(uint64_t NewSize)
			{
				if (NewSize > Volume)
					Reserve(IncreaseCapacity(NewSize));

				Count = NewSize;
			}
			void Copy(const Pool<T>& Raw)
			{
				if (Data == nullptr || (Data != nullptr && Volume >= Raw.Volume))
				{
					Data = (T*)TH_MALLOC((size_t)(Raw.Count * SizeOf(Data)));
					memset(Data, 0, (size_t)(Raw.Count * SizeOf(Data)));
				}
				else
					Data = (T*)TH_REALLOC(Data, (size_t)(Raw.Volume * SizeOf(Data)));

				Count = Raw.Count;
				Volume = Raw.Volume;
				if (!Assign(Raw.Begin(), Raw.End(), Data))
					memcpy(Data, Raw.Data, (size_t)(Count * SizeOf(Data)));
			}
			void Clear()
			{
				Count = 0;
			}
			Iterator Add(const T& Ref)
			{
				if (Count >= Volume)
					return End();

				Data[Count++] = Ref;
				return End() - 1;
			}
			Iterator AddIfNotExists(const T& Ref)
			{
				Iterator It = Find(Ref);
				if (It != End())
					return It;

				return Add(Ref);
			}
			Iterator RemoveAt(Iterator It)
			{
				Count--;
				Data[It - Data] = Data[Count];
				Dispose(It);
				return It;
			}
			Iterator Remove(const T& Value)
			{
				Iterator It;
				while ((It = Find(Value)) != End())
					RemoveAt(It);

				return It;
			}
			Iterator At(uint64_t Index) const
			{
				if (Index < 0 || Index >= Count)
					return End();

				return Data + Index;
			}
			Iterator Find(const T& Ref)
			{
				for (auto It = Data; It != Data + Count; It++)
				{
					if (*It == Ref)
						return It;
				}

				return Data + Count;
			}

		public:
			Iterator Begin() const
			{
				return Data;
			}
			Iterator End() const
			{
				return Data + Count;
			}
			T* Get() const
			{
				return Data;
			}
			T& Front() const
			{
				return *Begin();
			}
			T& Back() const
			{
				return *(End() - 1);
			}
			T& operator [](uint64_t Index) const
			{
				return *(Data + Index);
			}
			Pool<T>& operator =(const Pool<T>& Raw)
			{
				Copy(Raw);
				return *this;
			}
			bool operator ==(const Pool<T>& Raw)
			{
				return Compare(Raw) == 0;
			}
			bool operator !=(const Pool<T>& Raw)
			{
				return Compare(Raw) != 0;
			}
			bool operator <(const Pool<T>& Raw)
			{
				return Compare(Raw) < 0;
			}
			bool operator <=(const Pool<T>& Raw)
			{
				return Compare(Raw) <= 0;
			}
			bool operator >(const Pool<T>& Raw)
			{
				return Compare(Raw) > 0;
			}
			bool operator >=(const Pool<T>& Raw)
			{
				return Compare(Raw) >= 0;
			}

		public:
			uint64_t Size() const
			{
				return Count;
			}
			uint64_t Capacity() const
			{
				return Volume;
			}
			bool Empty() const
			{
				return Count <= 0;
			}

		protected:
			bool Assign(Iterator A, Iterator B, Iterator C)
			{
				if (std::is_pointer<T>::value)
					return false;

				std::copy(A, B, C);
				return true;
			}
			void Dispose(Iterator It)
			{
				if (!std::is_pointer<T>::value)
					It->~T();
			}
			uint64_t SizeOf(Iterator A)
			{
				if (!std::is_pointer<T>::value)
					return sizeof(T);

				return sizeof(uint64_t);
			}
			uint64_t IncreaseCapacity(uint64_t NewSize)
			{
				uint64_t Alpha = Volume ? (Volume + Volume / 2) : 8;
				return Alpha > NewSize ? Alpha : NewSize;
			}
		};

		TH_OUT Stroke Form(const char* Format, ...);
	}
}
#endif
