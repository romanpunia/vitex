#ifndef TH_CORE_H
#define TH_CORE_H
#pragma warning(disable: 4251)
#pragma warning(disable: 4996)
#include <thread>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <cstdarg>
#include <cstring>
#include <functional>
#include <vector>
#include <queue>
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
#define TH_INFO(Format, ...) Tomahawk::Core::Debug::Log(3, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_WARN(Format, ...) Tomahawk::Core::Debug::Log(2, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_ERROR(Format, ...) Tomahawk::Core::Debug::Log(1, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#elif TH_DLEVEL >= 2
#define TH_INFO(Format, ...)
#define TH_WARN(Format, ...) Tomahawk::Core::Debug::Log(2, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_ERROR(Format, ...) Tomahawk::Core::Debug::Log(1, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#elif TH_DLEVEL >= 1
#define TH_INFO(Format, ...)
#define TH_WARN(Format, ...)
#define TH_ERROR(Format, ...) Tomahawk::Core::Debug::Log(1, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#else
#define TH_INFO(...)
#define TH_WARN(...)
#define TH_ERROR(...)
#endif
#define TH_LOG(Format, ...) Tomahawk::Core::Debug::Log(0, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_COUT extern "C" TH_OUT
#define TH_MALLOC(Size) Tomahawk::Core::Mem::Malloc(Size)
#define TH_NEW(Type, ...) new(TH_MALLOC(sizeof(Type))) Type(__VA_ARGS__)
#define TH_DELETE(Destructor, Var) { if (Var != nullptr) { (Var)->~Destructor(); TH_FREE((void*)Var); } }
#define TH_DELETE_THIS(Destructor) { (this)->~Destructor(); TH_FREE((void*)this); }
#define TH_REALLOC(Ptr, Size) Tomahawk::Core::Mem::Realloc(Ptr, Size)
#define TH_FREE(Ptr) Tomahawk::Core::Mem::Free(Ptr)
#define TH_RELEASE(Ptr) { if (Ptr != nullptr) (Ptr)->Release(); }
#define TH_CLEAR(Ptr) { if (Ptr != nullptr) { (Ptr)->Release(); Ptr = nullptr; } }
#define TH_PREFIX_CHAR '@'
#define TH_PREFIX_STR "@"
#define TH_COMPONENT_HASH(Name) Tomahawk::Core::OS::File::GetCheckSum(Name)
#define TH_COMPONENT_IS(Source, Name) (Source->GetId() == TH_COMPONENT_HASH(Name))
#define TH_COMPONENT_ROOT(Name) \
virtual const char* GetName() { return Name; } \
virtual uint64_t GetId() { static uint64_t V = TH_COMPONENT_HASH(Name); return V; } \
static const char* GetTypeName() { return Name; } \
static uint64_t GetTypeId() { static uint64_t V = TH_COMPONENT_HASH(Name); return V; }

#define TH_COMPONENT(Name) \
virtual const char* GetName() override { return Name; } \
virtual uint64_t GetId() override { static uint64_t V = TH_COMPONENT_HASH(Name); return V; } \
static const char* GetTypeName() { return Name; } \
static uint64_t GetTypeId() { static uint64_t V = TH_COMPONENT_HASH(Name); return V; }

namespace Tomahawk
{
	namespace Core
	{
		class Document;

		class Object;

		class Stream;

		class Var;

        class Decimal;
    
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

		enum EventType
		{
			EventType_Events = (1 << 1),
			EventType_Tasks = (1 << 2),
			EventType_Timers = (1 << 3)
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
		typedef std::unordered_map<std::string, Document*> DocumentArgs;
		typedef std::function<void(VariantArgs&)> EventCallback;
		typedef std::function<void()> TaskCallback;
		typedef std::function<void()> TimerCallback;
		typedef std::function<void(VarForm, const char*, int64_t)> NWriteCallback;
		typedef std::function<bool(char*, int64_t)> NReadCallback;
		typedef TaskCallback EventTask;
		typedef uint64_t EventId;
        typedef Decimal BigNumber;

        class TH_OUT Decimal
        {
        private:
            std::deque<char> Source;
            int Length;
            char Sign;
            bool NaN;
            
        public:
            Decimal();
            Decimal(const char* Value);
            Decimal(const std::string& Value);
            Decimal(int Value);
            Decimal(double Value);
            Decimal(const Decimal& Value);
            Decimal(Decimal&& Value);
            void SetPrecision(int Value);
            void TrimLead();
            void TrimTrail();
			bool IsNaN() const;
            double ToDouble() const;
            float ToFloat() const;
            std::string ToString() const;
            std::string Exp() const;
            int Decimals() const;
            int Ints() const;
            int Size() const;
            Decimal& operator= (const char* Value);
            Decimal& operator= (const std::string& Value);
            Decimal& operator= (int Value);
            Decimal& operator= (double Value);
            Decimal& operator= (const Decimal& Value);
            Decimal& operator= (Decimal&& Value);
            Decimal& operator++ (int);
            Decimal& operator++ ();
            Decimal& operator-- (int);
            Decimal& operator-- ();
            bool operator== (const Decimal& Right) const;
            bool operator== (const int& Right) const;
            bool operator== (const double& Right) const;
            bool operator!= (const Decimal& Right) const;
            bool operator!= (const int& Right) const;
            bool operator!= (const double& Right) const;
            bool operator> (const Decimal& Right) const;
            bool operator> (const int& Right) const;
            bool operator> (const double& Right) const;
            bool operator>= (const Decimal& Right) const;
            bool operator>= (const int& Right) const;
            bool operator>= (const double& Right) const;
            bool operator< (const Decimal& Right) const;
            bool operator< (const int& Right) const;
            bool operator< (const double& Right) const;
            bool operator<= (const Decimal& Right) const;
            bool operator<= (const int& Right) const;
            bool operator<= (const double& Right) const;
        
		public:
			TH_OUT friend std::ostream& operator << (std::ostream& Left, const Decimal& Right);
			TH_OUT friend std::istream& operator >> (std::istream& Left, Decimal& Right);
			TH_OUT friend Decimal operator + (const Decimal& Left, const Decimal& Right);
			TH_OUT friend Decimal operator + (const Decimal& Left, const int& Right);
			TH_OUT friend Decimal operator + (const Decimal& Left, const double& Right);
			TH_OUT friend Decimal operator - (const Decimal& Left, const Decimal& Right);
			TH_OUT friend Decimal operator - (const Decimal& Left, const int& Right);
			TH_OUT friend Decimal operator - (const Decimal& Left, const double& Right);
			TH_OUT friend Decimal operator * (const Decimal& Left, const Decimal& Right);
			TH_OUT friend Decimal operator * (const Decimal& Left, const int& Right);
			TH_OUT friend Decimal operator * (const Decimal& Left, const double& Right);
			TH_OUT friend Decimal operator / (const Decimal& Left, const Decimal& Right);
			TH_OUT friend Decimal operator / (const Decimal& Left, const int& Right);
			TH_OUT friend Decimal operator / (const Decimal& Left, const double& Right);
			TH_OUT friend Decimal operator % (const Decimal& Left, const Decimal& Right);
			TH_OUT friend Decimal operator % (const Decimal& Left, const int& Right);

        public:
            static Decimal PrecDiv(const Decimal& Left, const Decimal& Right, int Precision);
            static Decimal PrecDiv(const Decimal& Left, const int& Right, int Precision);
            static Decimal PrecDiv(const Decimal& Left, const double& Right, int Precision);
            static Decimal Empty();
            
        private:
            static Decimal Sum(const Decimal& Left, const Decimal& Right);
            static Decimal Subtract(const Decimal& Left, const Decimal& Right);
            static Decimal Multiply(const Decimal& Left, const Decimal& Right);
            static int CompareNum(const Decimal& Left, const Decimal& Right);
            static int CharToInt(const char& Value);
            static char IntToChar(const int& Value);
        };

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
			std::string GetBlob() const;
            Decimal GetDecimal() const;
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

		struct TH_OUT EventTimer
		{
			TimerCallback Callback;
			uint64_t Timeout;
			EventId Id;
			bool Alive;

			EventTimer(const TimerCallback& NewCallback, uint64_t NewTimeout, EventId NewId, bool NewAlive);
			EventTimer(TimerCallback&& NewCallback, uint64_t NewTimeout, EventId NewId, bool NewAlive);
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
			bool DateRebuild;

		public:
			DateTime();
			DateTime(const DateTime& Value);
			DateTime& operator= (const DateTime& Other);
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

		struct TH_OUT Parser
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
			Parser();
			Parser(int Value);
			Parser(unsigned int Value);
			Parser(int64_t Value);
			Parser(uint64_t Value);
			Parser(float Value);
			Parser(double Value);
			Parser(long double Value);
			Parser(const std::string& Buffer);
			Parser(std::string* Buffer);
			Parser(const std::string* Buffer);
			Parser(const char* Buffer);
			Parser(const char* Buffer, int64_t Length);
			Parser(const Parser& Value);
			~Parser();
			Parser& EscapePrint();
			Parser& Escape();
			Parser& Unescape();
			Parser& Reserve(uint64_t Count = 1);
			Parser& Resize(uint64_t Count);
			Parser& Resize(uint64_t Count, char Char);
			Parser& Clear();
			Parser& ToUpper();
			Parser& ToLower();
			Parser& Clip(uint64_t Length);
			Parser& ReplaceOf(const char* Chars, const char* To, uint64_t Start = 0U);
			Parser& ReplaceNotOf(const char* Chars, const char* To, uint64_t Start = 0U);
			Parser& Replace(const std::string& From, const std::string& To, uint64_t Start = 0U);
			Parser& Replace(const char* From, const char* To, uint64_t Start = 0U);
			Parser& Replace(const char& From, const char& To, uint64_t Position = 0U);
			Parser& Replace(const char& From, const char& To, uint64_t Position, uint64_t Count);
			Parser& ReplacePart(uint64_t Start, uint64_t End, const std::string& Value);
			Parser& ReplacePart(uint64_t Start, uint64_t End, const char* Value);
			Parser& RemovePart(uint64_t Start, uint64_t End);
			Parser& Reverse();
			Parser& Reverse(uint64_t Start, uint64_t End);
			Parser& Substring(uint64_t Start);
			Parser& Substring(uint64_t Start, uint64_t Count);
			Parser& Substring(const Parser::Settle& Result);
			Parser& Splice(uint64_t Start, uint64_t End);
			Parser& Trim();
			Parser& Fill(const char& Char);
			Parser& Fill(const char& Char, uint64_t Count);
			Parser& Fill(const char& Char, uint64_t Start, uint64_t Count);
			Parser& Assign(const char* Raw);
			Parser& Assign(const char* Raw, uint64_t Length);
			Parser& Assign(const std::string& Raw);
			Parser& Assign(const std::string& Raw, uint64_t Start, uint64_t Count);
			Parser& Assign(const char* Raw, uint64_t Start, uint64_t Count);
			Parser& Append(const char* Raw);
			Parser& Append(const char& Char);
			Parser& Append(const char& Char, uint64_t Count);
			Parser& Append(const std::string& Raw);
			Parser& Append(const char* Raw, uint64_t Count);
			Parser& Append(const char* Raw, uint64_t Start, uint64_t Count);
			Parser& Append(const std::string& Raw, uint64_t Start, uint64_t Count);
			Parser& fAppend(const char* Format, ...);
			Parser& Insert(const std::string& Raw, uint64_t Position);
			Parser& Insert(const std::string& Raw, uint64_t Position, uint64_t Start, uint64_t Count);
			Parser& Insert(const std::string& Raw, uint64_t Position, uint64_t Count);
			Parser& Insert(const char& Char, uint64_t Position, uint64_t Count);
			Parser& Insert(const char& Char, uint64_t Position);
			Parser& Erase(uint64_t Position);
			Parser& Erase(uint64_t Position, uint64_t Count);
			Parser& EraseOffsets(uint64_t Start, uint64_t End);
			Parser& Path(const std::string& Net, const std::string& Dir);
			Parser::Settle ReverseFind(const std::string& Needle, uint64_t Offset = 0U) const;
			Parser::Settle ReverseFind(const char* Needle, uint64_t Offset = 0U) const;
			Parser::Settle ReverseFind(const char& Needle, uint64_t Offset = 0U) const;
			Parser::Settle ReverseFindUnescaped(const char& Needle, uint64_t Offset = 0U) const;
			Parser::Settle ReverseFindOf(const std::string& Needle, uint64_t Offset = 0U) const;
			Parser::Settle ReverseFindOf(const char* Needle, uint64_t Offset = 0U) const;
			Parser::Settle Find(const std::string& Needle, uint64_t Offset = 0U) const;
			Parser::Settle Find(const char* Needle, uint64_t Offset = 0U) const;
			Parser::Settle Find(const char& Needle, uint64_t Offset = 0U) const;
			Parser::Settle FindUnescaped(const char& Needle, uint64_t Offset = 0U) const;
			Parser::Settle FindOf(const std::string& Needle, uint64_t Offset = 0U) const;
			Parser::Settle FindOf(const char* Needle, uint64_t Offset = 0U) const;
			Parser::Settle FindNotOf(const std::string& Needle, uint64_t Offset = 0U) const;
			Parser::Settle FindNotOf(const char* Needle, uint64_t Offset = 0U) const;
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
			Parser& operator = (const Parser& New);

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
			static Variant Decimal(const BigNumber& Value);
            static Variant Decimal(BigNumber&& Value);
            static Variant DecimalString(const std::string& Value);
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
				static std::string ResolveResource(const std::string& Path);
				static std::string ResolveResource(const std::string& Path, const std::string& Directory);
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
#ifdef TH_MICROSOFT
			FILE* Conin;
			FILE* Conout;
			FILE* Conerr;
#endif
			std::mutex Lock;
			bool Handle;
			double Time;

		private:
			Console();

		public:
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
			virtual void Clear() override;
			virtual bool Open(const char* File, FileMode Mode) override;
			virtual bool Close() override;
			virtual bool Seek(FileSeek Mode, int64_t Offset) override;
			virtual bool Move(int64_t Offset) override;
			virtual int Flush() override;
			virtual uint64_t ReadAny(const char* Format, ...) override;
			virtual uint64_t Read(char* Buffer, uint64_t Length) override;
			virtual uint64_t WriteAny(const char* Format, ...) override;
			virtual uint64_t Write(const char* Buffer, uint64_t Length) override;
			virtual uint64_t Tell() override;
			virtual int GetFd() override;
			virtual void* GetBuffer() override;
		};

		class TH_OUT GzStream : public Stream
		{
		protected:
			void* Resource;

		public:
			GzStream();
			virtual ~GzStream() override;
			virtual void Clear() override;
			virtual bool Open(const char* File, FileMode Mode) override;
			virtual bool Close() override;
			virtual bool Seek(FileSeek Mode, int64_t Offset) override;
			virtual bool Move(int64_t Offset) override;
			virtual int Flush() override;
			virtual uint64_t ReadAny(const char* Format, ...) override;
			virtual uint64_t Read(char* Buffer, uint64_t Length) override;
			virtual uint64_t WriteAny(const char* Format, ...) override;
			virtual uint64_t Write(const char* Buffer, uint64_t Length) override;
			virtual uint64_t Tell() override;
			virtual int GetFd() override;
			virtual void* GetBuffer() override;
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
			virtual void Clear() override;
			virtual bool Open(const char* File, FileMode Mode) override;
			virtual bool Close() override;
			virtual bool Seek(FileSeek Mode, int64_t Offset) override;
			virtual bool Move(int64_t Offset) override;
			virtual int Flush() override;
			virtual uint64_t ReadAny(const char* Format, ...) override;
			virtual uint64_t Read(char* Buffer, uint64_t Length) override;
			virtual uint64_t WriteAny(const char* Format, ...) override;
			virtual uint64_t Write(const char* Buffer, uint64_t Length) override;
			virtual uint64_t Tell() override;
			virtual int GetFd() override;
			virtual void* GetBuffer() override;
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

		class TH_OUT Schedule : public Object
		{
		private:
			struct
			{
				std::vector<std::thread> Childs;
				std::condition_variable Condition;
				std::mutex Manage;
				std::mutex Child;
			} Async;

			struct
			{
				std::mutex Listeners;
				std::mutex Events;
				std::mutex Tasks;
				std::mutex Timers;
			} Sync;

		private:
			std::unordered_map<std::string, EventListener> Listeners;
			std::map<int64_t, EventTimer> Timers;
			std::queue<EventTask> Tasks;
			std::queue<EventBase> Events;
			EventId Timer;
			uint64_t Workers;
			bool Terminate;
			bool Active;

		private:
			Schedule();

		public:
			virtual ~Schedule() override;
			EventId SetInterval(uint64_t Milliseconds, const TimerCallback& Callback);
			EventId SetInterval(uint64_t Milliseconds, TimerCallback&& Callback);
			EventId SetTimeout(uint64_t Milliseconds, const TimerCallback& Callback);
			EventId SetTimeout(uint64_t Milliseconds, TimerCallback&& Callback);
			EventId SetListener(const std::string& Name, const EventCallback& Callback);
			EventId SetListener(const std::string& Name, EventCallback&& Callback);
			bool SetTask(const TaskCallback& Callback);
			bool SetTask(TaskCallback&& Callback);
			bool SetEvent(const std::string& Name, const VariantArgs& Args);
			bool SetEvent(const std::string& Name, VariantArgs&& Args);
			bool SetEvent(const std::string& Name);
			bool ClearListener(const std::string& Name, EventId ListenerId);
			bool ClearTimeout(EventId TimerId);
			bool Clear(EventType Type, bool NoCall);
			bool Start(bool Async, uint64_t Workers);
			bool Dispatch();
			bool Stop();
			bool IsBlockable();
			bool IsActive();

		private:
			bool LoopIncome();
			bool LoopCycle();
			bool DispatchTask();
			bool DispatchEvent();
			bool DispatchTimer();
			int64_t GetTimeout(int64_t Clock);
			int64_t GetClock();

		public:
			static Schedule* Get();

		private:
			static Schedule* Singleton;
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
            bool Rename(const std::string& Name, const std::string& NewName);
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

				if (!Data)
				{
					Data = Raw;
					return;
				}

				if (!Assign(Begin(), End(), Raw))
					memcpy(Raw, Data, (size_t)(Count * SizeOf(Data)));

				for (auto It = Begin(); It != End(); It++)
					Dispose(It);

				TH_FREE(Data);
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
				if (Data == nullptr || Volume >= Raw.Volume)
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
				if (Index >= Count)
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
				return !Count;
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

		template <typename T>
		class Async
		{
			static_assert(!std::is_same<T, void>::value, "async cannot be used with void type");
			typedef T value_type;

		private:
			template <typename F>
			struct Future
			{
				typedef F type;
			};

			template <typename F>
			struct Future<Async<F>>
			{
				typedef F type;
			};

		private:
			struct Base
			{
				std::atomic<uint32_t> Count;
				std::atomic<int32_t> Set;
				std::atomic<bool> Deferred;
				std::function<void()> Resolve;
				std::mutex RW;
				T Result;

				Base() : Count(1), Set(-1), Deferred(true)
				{
				}
				Base* Copy()
				{
					Count++;
					return this;
				}
				void Put(std::function<void()>&& Callback)
				{
					if (Set > 0)
						return Callback();

					RW.lock();
					Resolve = std::move(Callback);
					RW.unlock();
				}
				void React()
				{
					RW.lock();
					if (Resolve)
						Resolve();
					RW.unlock();
				}
				void React(T&& Value)
				{
					Set = 1;
					RW.lock();
					Result = std::move(Value);
					if (Resolve)
						Resolve();
					RW.unlock();
				}
				void React(const T& Value)
				{
					Set = 1;
					RW.lock();
					Result = Value;
					if (Resolve)
						Resolve();
					RW.unlock();
				}
				void Free()
				{
					if (!--Count)
						TH_DELETE_THIS(Base);
				}
			}* Next;

		public:
			Async() : Next(TH_NEW(Base))
			{
			}
			Async(std::function<void(Async&)>&& Executor) : Async()
			{
                if (!Executor)
                    return;

                Schedule* Queue = Schedule::Get();
                if (Queue->IsActive())
                {
                    Base* Subresult = Next->Copy();
                    Queue->SetTask([Subresult, Executor = std::move(Executor)]() mutable
                    {
                        Async Copy(Subresult);
                        Subresult->Free();
                        Executor(Copy);
                    });
                }
                else
                    Executor(*this);
			}
			Async(Base* Context) : Next(Context)
			{
				if (Next != nullptr)
					Next->Count++;
			}
			Async(const Async& Other) : Next(Other.Next)
			{
				if (Next != nullptr)
					Next->Count++;
			}
			Async(Async&& Other) : Next(Other.Next)
			{
				Other.Next = nullptr;
			}
			~Async()
			{
				if (Next != nullptr)
					Next->Free();
			}
			Async& operator= (const Async&) = delete;
			Async& operator= (Async&&) = delete;
			Async& operator= (const T& Other)
			{
				Set(Other);
				return *this;
			}
			Async& operator= (T&& Other)
			{
				Set(Other);
				return *this;
			}
			Async& Sync(bool Blocking = true)
			{
				if (Next != nullptr)
					Next->Deferred = !Blocking;

				return *this;
			}
			void Set(const T& Value)
			{
				if (Next != nullptr && Next->Set == -1)
					Next->React(Value);
			}
			void Set(T&& Value)
			{
				if (Next != nullptr && Next->Set == -1)
					Next->React(std::move(Value));
			}
			void Set(Async&& Other)
			{
				if (!Next || Next->Set != -1)
					return;

				Base* Subresult = Next->Copy();
				Subresult->Set = 0;

				Other.Sync(!Subresult->Deferred).Await([Subresult](T&& Value) mutable
				{
					Subresult->React(std::move(Value));
					Subresult->Free();
				});
			}
			void Await(std::function<void(T&&)>&& Callback) const
			{
				if (!Callback || !Next)
					return;

				Base* Subresult = Next->Copy();
				Next->Put([Subresult, Callback = std::move(Callback)]()
				{
					Schedule* Queue = Schedule::Get();
					if (Queue->IsActive() && Subresult->Deferred)
					{
						Queue->SetTask([Subresult, Callback = std::move(Callback)]()
						{
							Callback(std::move(Subresult->Result));
							Subresult->Free();
						});
					}
					else
					{
						Callback(std::move(Subresult->Result));
						Subresult->Free();
					}
				});
			}
			bool IsPending()
			{
				return (Next != nullptr && Next->Set != 1);
			}
			T& Get()
			{
				while (IsPending())
					std::this_thread::sleep_for(std::chrono::microseconds(100));

				if (Next != nullptr)
					return Next->Result;

				Next = TH_NEW(Base);
				Next->Set = 1;
				return Next->Result;
			}

		public:
			template <typename R>
			Async<R> Then(std::function<void(Async<R>&, T&&)>&& Callback) const
			{
				if (!Callback || !Next)
					return Async<R>(nullptr);

				Async<R> Result; Base* Subresult = Next->Copy();
				Next->Put([Subresult, Result, Callback = std::move(Callback)]() mutable
				{
					Schedule* Queue = Schedule::Get();
					if (Queue->IsActive() && Subresult->Deferred)
					{
						Queue->SetTask([Subresult, Result = std::move(Result), Callback = std::move(Callback)]() mutable
						{
							Callback(Result, std::move(Subresult->Result));
							Subresult->Free();
						});
					}
					else
					{
						Callback(Result, std::move(Subresult->Result));
						Subresult->Free();
					}
				});

				return Result;
			}
			template <typename R>
			Async<typename Future<R>::type> Then(std::function<R(T&&)>&& Callback) const
			{
				using F = typename Future<R>::type;
				if (!Callback || !Next)
					return Async<F>(nullptr);

				Async<F> Result; Base* Subresult = Next->Copy();
				Next->Put([Subresult, Result, Callback = std::move(Callback)]() mutable
				{
					Schedule* Queue = Schedule::Get();
					if (Queue->IsActive() && Subresult->Deferred)
					{
						Queue->SetTask([Subresult, Result = std::move(Result), Callback = std::move(Callback)]() mutable
						{
							Result.Set(std::move(Callback(std::move(Subresult->Result))));
							Subresult->Free();
						});
					}
					else
					{
						Result.Set(std::move(Callback(std::move(Subresult->Result))));
						Subresult->Free();
					}
				});

				return Result;
			}

		public:
			static Async Store(const T& Value)
			{
				Async Result;
				Result.Next->Result = Value;
				Result.Next->Set = 1;

				return Result;
			}
			static Async Store(T&& Value)
			{
				Async Result;
				Result.Next->Result = std::move(Value);
				Result.Next->Set = 1;

				return Result;
			}
		};

		class Conditional
		{
		private:
			struct Output
			{
				std::atomic<size_t> Count;
				std::atomic<bool> Match;
				Async<bool> Value;
				bool Reverse;

				Output(bool Default, size_t Size) : Count(Size + 1), Match(Default), Reverse(Default)
				{
				}
				void Free()
				{
					Value.Set(Match);
					TH_DELETE_THIS(Output);
				}
				void Next(bool Statement)
				{
					if (Statement != Reverse)
						Match = !Reverse;

					if (!--Count)
						Free();
				}
				Async<bool> Get()
				{
					Async<bool> Result = Value;
					if (!--Count)
						Free();

					return Result;
				}
			};

		public:
			template <typename T>
			static bool ForEach(const std::vector<Async<T>>& Array, std::function<void(T&&)>&& Callback)
			{
				if (!Callback || Array.empty())
					return false;

				for (auto& Item : Array)
				{
					std::function<void(T&&)> Actor = Callback;
					Item.Await(std::move(Actor));
				}

				return true;
			}
			template <typename T>
			static Async<bool> And(T&& Value, const std::vector<Async<T>>& Array)
			{
				Output* State = TH_NEW(Output, true, Array.size());
				ForEach<T>(Array, [State, Value = std::move(Value)](T&& Result)
				{
					State->Next(Result == Value);
				});

				return State->Get();
			}
			template <typename T>
			static Async<bool> Or(T&& Value, const std::vector<Async<T>>& Array)
			{
				Output* State = TH_NEW(Output, false, Array.size());
				ForEach<T>(Array, [State, Value = std::move(Value)](T&& Result)
				{
					State->Next(Result == Value);
				});

				return State->Get();
			}
		};

		TH_OUT Parser Form(const char* Format, ...);

		inline EventType operator |(EventType A, EventType B)
		{
			return static_cast<EventType>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}
	}
}
#endif
