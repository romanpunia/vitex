#ifndef TH_CORE_H
#define TH_CORE_H
#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif
#pragma warning(disable: 4251)
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
#include <limits>
#if defined(_WIN32) || defined(_WIN64)
#ifndef TH_EXPORT
#define TH_OUT __declspec(dllimport)
#else
#define TH_OUT __declspec(dllexport)
#endif
#define TH_MICROSOFT 1
#define TH_FUNCTION __FUNCTION__
#ifdef _WIN64
#define TH_64 1
#else
#define TH_32 1
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
#define TH_UNIX 1
#define TH_MAX_PATH _POSIX_PATH_MAX
#define TH_STRINGIFY(X) #X
#define TH_FUNCTION __FUNCTION__
#define TH_CDECL
#define TH_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define TH_LINE __LINE__
#undef TH_MICROSOFT
#if __x86_64__ || __ppc64__
#define TH_64 1
#else
#define TH_32 1
#endif
#elif __APPLE__
#define TH_OUT
#define TH_UNIX 1
#define TH_MAX_PATH _POSIX_PATH_MAX
#define TH_STRINGIFY(X) #X
#define TH_FUNCTION __FUNCTION__
#define TH_CDECL
#define TH_FILE (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define TH_LINE __LINE__
#define TH_APPLE 1
#undef TH_MICROSOFT
#if __x86_64__ || __ppc64__
#define TH_64 1
#else
#define TH_32 1
#endif
#endif
#ifndef TH_MICROSOFT
#include <sys/types.h>
#endif
#ifdef max
#undef max
#endif
#ifdef TH_MICROSOFT
#ifdef TH_WITH_FCTX
#define TH_COCALL
#define TH_CODATA void* Context
#else
#define TH_COCALL __stdcall
#define TH_CODATA void* Context
#endif
#define TH_FILENO _fileno
#ifdef TH_64
typedef uint64_t socket_t;
#else
typedef int socket_t;
#endif
typedef int socket_size_t;
typedef void* epoll_handle;
#else
#ifdef TH_WITH_FCTX
#define TH_COCALL
#define TH_CODATA void* Context
#else
#define TH_COCALL
#define TH_CODATA int X, int Y
#endif
#define TH_FILENO fileno
#include <sys/socket.h>
typedef int epoll_handle;
typedef int socket_t;
typedef socklen_t socket_size_t;
#endif
#if TH_DLEVEL >= 4
#ifndef _DEBUG
#define TH_TRACE(Format, ...) Tomahawk::Core::OS::Log(4, 0, nullptr, Format, ##__VA_ARGS__)
#define TH_INFO(Format, ...) Tomahawk::Core::OS::Log(3, 0, nullptr, Format, ##__VA_ARGS__)
#define TH_WARN(Format, ...) Tomahawk::Core::OS::Log(2, 0, nullptr, Format, ##__VA_ARGS__)
#define TH_ERR(Format, ...) Tomahawk::Core::OS::Log(1, 0, nullptr, Format, ##__VA_ARGS__)
#else
#define TH_TRACE(Format, ...) Tomahawk::Core::OS::Log(4, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_INFO(Format, ...) Tomahawk::Core::OS::Log(3, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_WARN(Format, ...) Tomahawk::Core::OS::Log(2, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_ERR(Format, ...) Tomahawk::Core::OS::Log(1, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#endif
#elif TH_DLEVEL >= 3
#ifndef _DEBUG
#define TH_TRACE(Format, ...) ((void)0)
#define TH_INFO(Format, ...) Tomahawk::Core::OS::Log(3, 0, nullptr, Format, ##__VA_ARGS__)
#define TH_WARN(Format, ...) Tomahawk::Core::OS::Log(2, 0, nullptr, Format, ##__VA_ARGS__)
#define TH_ERR(Format, ...) Tomahawk::Core::OS::Log(1, 0, nullptr, Format, ##__VA_ARGS__)
#else
#define TH_TRACE(Format, ...) ((void)0)
#define TH_INFO(Format, ...) Tomahawk::Core::OS::Log(3, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_WARN(Format, ...) Tomahawk::Core::OS::Log(2, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_ERR(Format, ...) Tomahawk::Core::OS::Log(1, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#endif
#elif TH_DLEVEL >= 2
#define TH_TRACE(Format, ...) ((void)0)
#define TH_INFO(Format, ...) ((void)0)
#ifndef _DEBUG
#define TH_WARN(Format, ...) Tomahawk::Core::OS::Log(2, 0, nullptr, Format, ##__VA_ARGS__)
#define TH_ERR(Format, ...) Tomahawk::Core::OS::Log(1, 0, nullptr, Format, ##__VA_ARGS__)
#else
#define TH_WARN(Format, ...) Tomahawk::Core::OS::Log(2, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_ERR(Format, ...) Tomahawk::Core::OS::Log(1, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#endif
#elif TH_DLEVEL >= 1
#define TH_TRACE(Format, ...) ((void)0)
#define TH_INFO(Format, ...) ((void)0)
#define TH_WARN(Format, ...) ((void)0)
#ifndef _DEBUG
#define TH_ERR(Format, ...) Tomahawk::Core::OS::Log(1, 0, nullptr, Format, ##__VA_ARGS__)
#else
#define TH_ERR(Format, ...) Tomahawk::Core::OS::Log(1, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#endif
#else
#define TH_TRACE(Format, ...) ((void)0)
#define TH_INFO(...) ((void)0)
#define TH_WARN(...) ((void)0)
#define TH_ERR(...) ((void)0)
#endif
#ifdef _DEBUG
#if TH_DLEVEL >= 1
#define TH_ASSERT(Condition, Returnable, Format, ...) if (!(Condition)) { Tomahawk::Core::OS::Assert(true, TH_LINE, TH_FILE, TH_FUNCTION, TH_STRINGIFY(Condition), Format, ##__VA_ARGS__); return Returnable; }
#define TH_ASSERT_V(Condition, Format, ...) if (!(Condition)) { Tomahawk::Core::OS::Assert(true, TH_LINE, TH_FILE, TH_FUNCTION, TH_STRINGIFY(Condition), Format, ##__VA_ARGS__); return; }
#else
#define TH_ASSERT(Condition, Returnable, Format, ...) if (!(Condition)) { Tomahawk::Core::OS::Process::Interrupt(); return Returnable; }
#define TH_ASSERT_V(Condition, Format, ...) if (!(Condition)) { Tomahawk::Core::OS::Process::Interrupt(); return; }
#endif
#define TH_PERF_ATOM (1)
#define TH_PERF_FRAME (5)
#define TH_PERF_CORE (16)
#define TH_PERF_MIX (50)
#define TH_PERF_IO (80)
#define TH_PERF_NET (150)
#define TH_PERF_MAX (350)
#define TH_PERF_HANG (5000)
#define TH_PPUSH(Section, Threshold) Tomahawk::Core::OS::PerfPush(TH_FILE, Section, TH_FUNCTION, TH_LINE, Threshold)
#define TH_PSIG() Tomahawk::Core::OS::PerfSignal()
#define TH_PPOP() Tomahawk::Core::OS::PerfPop()
#define TH_PRET(Value) { auto __vfbuf = (Value); Tomahawk::Core::OS::PerfPop(); return __vfbuf; }
#define TH_SPUSH(Section, Threshold, Id) Tomahawk::Core::OS::SpecPush(TH_FILE, Section, TH_FUNCTION, TH_LINE, Threshold, (void*)(Id))
#define TH_SSIG() Tomahawk::Core::OS::SpecSignal()
#define TH_SPOP(Id) Tomahawk::Core::OS::SpecPop((void*)(Id))
#define TH_ORET(Id, Value) { auto __vfbuf = (Value); Tomahawk::Core::OS::SpecPop((void*)(Id)); return __vfbuf; }
#define TH_AWAIT(Value) Tomahawk::Core::Coawait(Value, TH_FILE, TH_FUNCTION, "coawait of [ " TH_STRINGIFY(Value) " ]", TH_LINE)
#define TH_CLOSE(Stream) { TH_TRACE("[io] close fs %i", (int)TH_FILENO(Stream)); fclose(Stream); }
#else
#define TH_ASSERT(Condition, Returnable, Format, ...) ((void)0)
#define TH_ASSERT_V(Condition, Format, ...) ((void)0)
#define TH_PPUSH(Section, Threshold) ((void)0)
#define TH_PSIG() ((void)0)
#define TH_PPOP() ((void)0)
#define TH_PRET(Value) return Value
#define TH_SPUSH(Section, Threshold, Id) ((void)0)
#define TH_SSIG() ((void)0)
#define TH_SPOP(Id) ((void)0)
#define TH_ORET(Id, Value) return Value
#define TH_AWAIT(Value) Tomahawk::Core::Coawait(Value)
#define TH_CLOSE(Stream) fclose(Stream)
#endif
#define TH_LOG(Format, ...) Tomahawk::Core::OS::Log(0, TH_LINE, TH_FILE, Format, ##__VA_ARGS__)
#define TH_STACKSIZE (512 * 1024)
#define TH_COUT extern "C" TH_OUT
#define TH_MALLOC(Size) Tomahawk::Core::Mem::Malloc(Size)
#define TH_NEW(Type, ...) new(TH_MALLOC(sizeof(Type))) Type(__VA_ARGS__)
#define TH_DELETE(Destructor, Var) { if (Var != nullptr) { (Var)->~Destructor(); TH_FREE((void*)Var); } }
#define TH_DELETE_THIS(Destructor) { (this)->~Destructor(); TH_FREE((void*)this); }
#define TH_REALLOC(Ptr, Size) Tomahawk::Core::Mem::Realloc(Ptr, Size)
#define TH_FREE(Ptr) Tomahawk::Core::Mem::Free(Ptr)
#define TH_RELEASE(Ptr) { if (Ptr != nullptr) (Ptr)->Release(); }
#define TH_CLEAR(Ptr) { if (Ptr != nullptr) { (Ptr)->Release(); Ptr = nullptr; } }
#define TH_INVALID_TASK_ID 0
#define TH_MAX_EVENTS 32
#define TH_PREFIX_CHAR '`'
#define TH_PREFIX_STR "`"
#define TH_SHUFFLE(Name) Tomahawk::Core::Shuffle<sizeof(Name)>(Name)
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
		struct ConcurrentQueuePtr;

		struct Decimal;

		struct Cocontext;

		class Costate;

		class Schema;

		class Object;

		class Stream;

		class Var;

		enum class StdColor
		{
			Black = 0,
			DarkBlue = 1,
			DarkGreen = 2,
			LightBlue = 3,
			DarkRed = 4,
			Magenta = 5,
			Orange = 6,
			LightGray = 7,
			Gray = 8,
			Blue = 9,
			Green = 10,
			Cyan = 11,
			Red = 12,
			Pink = 13,
			Yellow = 14,
			White = 15,
			Zero = 16
		};

		enum class FileMode
		{
			Read_Only,
			Write_Only,
			Append_Only,
			Read_Write,
			Write_Read,
			Read_Append_Write,
			Binary_Read_Only,
			Binary_Write_Only,
			Binary_Append_Only,
			Binary_Read_Write,
			Binary_Write_Read,
			Binary_Read_Append_Write
		};

		enum class FileSeek
		{
			Begin,
			Current,
			End
		};

		enum class VarType : uint8_t
		{
			Null,
			Undefined,
			Object,
			Array,
			Pointer,
			String,
			Base64,
			Integer,
			Number,
			Decimal,
			Boolean
		};

		enum class VarForm
		{
			Dummy,
			Tab_Decrease,
			Tab_Increase,
			Write_Space,
			Write_Line,
			Write_Tab,
		};

		enum class Coactive
		{
			Active,
			Inactive,
			Resumable
		};

		enum class Difficulty
		{
			Chain,
			Light,
			Heavy,
			Clock,
			Count
		};

		typedef std::vector<struct Variant> VariantList;
		typedef std::vector<Schema*> SchemaList;
		typedef std::unordered_map<std::string, struct Variant> VariantArgs;
		typedef std::unordered_map<std::string, Schema*> SchemaArgs;
		typedef std::function<void()> TaskCallback;
		typedef std::function<std::string(const std::string&)> SchemaNameCallback;
		typedef std::function<void(VarForm, const char*, int64_t)> SchemaWriteCallback;
		typedef std::function<bool(char*, int64_t)> SchemaReadCallback;
		typedef std::function<void* (size_t)> AllocCallback;
		typedef std::function<void* (void*, size_t)> ReallocCallback;
		typedef std::function<void(void*)> FreeCallback;
		typedef std::function<bool()> ActivityCallback;
		typedef uint64_t TaskId;
		typedef Decimal BigNumber;

		struct TH_OUT Coroutine
		{
			friend Costate;

		private:
			std::atomic<Coactive> State;
			std::atomic<int> Dead;
			TaskCallback Callback;
			Cocontext* Slave;
			Costate* Master;

		public:
			TaskCallback Return;

		private:
			Coroutine(Costate* Base, const TaskCallback& Procedure);
			Coroutine(Costate* Base, TaskCallback&& Procedure);
			~Coroutine();
		};

		struct TH_OUT Decimal
		{
		private:
			std::deque<char> Source;
			int Length;
			char Sign;
			bool Invalid;

		public:
			Decimal();
			Decimal(const char* Value);
			Decimal(const std::string& Value);
			Decimal(int32_t Value);
			Decimal(uint32_t Value);
			Decimal(int64_t Value);
			Decimal(uint64_t Value);
			Decimal(float Value);
			Decimal(double Value);
			Decimal(const Decimal& Value) noexcept;
			Decimal(Decimal&& Value) noexcept;
			Decimal& Precise(int Value);
			Decimal& Trim();
			Decimal& Unlead();
			Decimal& Untrail();
			bool IsNaN() const;
			bool IsZero() const;
			bool IsZeroOrNaN() const;
			double ToDouble() const;
			int64_t ToInt64() const;
			uint64_t ToUInt64() const;
			std::string ToString() const;
			std::string Exp() const;
			int Decimals() const;
			int Ints() const;
			int Size() const;
			Decimal operator -() const;
			Decimal& operator *=(const Decimal& V);
			Decimal& operator /=(const Decimal& V);
			Decimal& operator +=(const Decimal& V);
			Decimal& operator -=(const Decimal& V);
			Decimal& operator= (const Decimal& Value) noexcept;
			Decimal& operator= (Decimal&& Value) noexcept;
			Decimal& operator++ (int);
			Decimal& operator++ ();
			Decimal& operator-- (int);
			Decimal& operator-- ();
			bool operator== (const Decimal& Right) const;
			bool operator!= (const Decimal& Right) const;
			bool operator> (const Decimal& Right) const;
			bool operator>= (const Decimal& Right) const;
			bool operator< (const Decimal& Right) const;
			bool operator<= (const Decimal& Right) const;

		public:
			TH_OUT friend Decimal operator + (const Decimal& Left, const Decimal& Right);
			TH_OUT friend Decimal operator - (const Decimal& Left, const Decimal& Right);
			TH_OUT friend Decimal operator * (const Decimal& Left, const Decimal& Right);
			TH_OUT friend Decimal operator / (const Decimal& Left, const Decimal& Right);
			TH_OUT friend Decimal operator % (const Decimal& Left, const Decimal& Right);

		public:
			static Decimal Divide(const Decimal& Left, const Decimal& Right, int Precision);
			static Decimal NaN();

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
			friend Schema;
			friend Var;

		private:
			struct String
			{
				char* Buffer;
				uint32_t Size;
			};

		private:
			union Tag
			{
				char* Data;
				int64_t Integer;
				double Number;
				bool Boolean;
			} Value;

		private:
			VarType Type;

		public:
			Variant() noexcept;
			Variant(const Variant& Other) noexcept;
			Variant(Variant&& Other) noexcept;
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
			Variant& operator= (const Variant& Other) noexcept;
			Variant& operator= (Variant&& Other) noexcept;
			bool operator== (const Variant& Other) const;
			bool operator!= (const Variant& Other) const;
			operator bool() const;
			bool IsString(const char* Value) const;
			bool IsObject() const;
			bool IsEmpty() const;

		private:
			Variant(VarType NewType) noexcept;
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

		struct TH_OUT Timeout
		{
			std::chrono::microseconds Expires;
			TaskCallback Callback;
			Difficulty Type;
			TaskId Id;
			bool Alive;

			Timeout(const TaskCallback& NewCallback, const std::chrono::microseconds& NewTimeout, TaskId NewId, bool NewAlive, Difficulty NewType) noexcept;
			Timeout(TaskCallback&& NewCallback, const std::chrono::microseconds& NewTimeout, TaskId NewId, bool NewAlive, Difficulty NewType) noexcept;
			Timeout(const Timeout& Other) noexcept;
			Timeout(Timeout&& Other) noexcept;
			Timeout& operator= (const Timeout& Other) noexcept;
			Timeout& operator= (Timeout&& Other) noexcept;
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
			DateTime(uint64_t Seconds);
			DateTime(const DateTime& Value);
			DateTime& operator= (const DateTime& Other);
			void operator +=(const DateTime& Right);
			void operator -=(const DateTime& Right);
			bool operator >=(const DateTime& Right);
			bool operator <=(const DateTime& Right);
			bool operator >(const DateTime& Right);
			bool operator <(const DateTime& Right);
			bool operator ==(const DateTime& Right);
			std::string Format(const std::string& Value);
			std::string Date(const std::string& Value);
			std::string Iso8601();
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
			DateTime operator +(const DateTime& Right) const;
			DateTime operator -(const DateTime& Right) const;
			DateTime& SetDateSeconds(uint64_t Value, bool NoFlush = true);
			DateTime& SetDateMinutes(uint64_t Value, bool NoFlush = true);
			DateTime& SetDateHours(uint64_t Value, bool NoFlush = true);
			DateTime& SetDateDay(uint64_t Value, bool NoFlush = true);
			DateTime& SetDateWeek(uint64_t Value, bool NoFlush = true);
			DateTime& SetDateMonth(uint64_t Value, bool NoFlush = true);
			DateTime& SetDateYear(uint64_t Value, bool NoFlush = true);
			uint64_t DateSecond();
			uint64_t DateMinute();
			uint64_t DateHour();
			uint64_t DateDay();
			uint64_t DateWeek();
			uint64_t DateMonth();
			uint64_t DateYear();
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
				uint64_t Start = 0;
				uint64_t End = 0;
				bool Found = false;
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
			Parser& ReplaceGroups(const std::string& FromRegex, const std::string& To);
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
			Parser& Eval(const std::string& Net, const std::string& Dir);
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
			std::wstring ToWide() const;
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
			static std::string ToString(float Number);
			static std::string ToString(double Number);
		};

		struct TH_OUT Ticker
		{
		private:
			double Time;

		public:
			double Delay;

		public:
			Ticker();
			bool TickEvent(double ElapsedTime);
			double GetTime();
		};

		struct TH_OUT Spin
		{
		private:
			std::atomic_flag Atom;

		public:
			Spin();
			void Lock();
			void Unlock();
		};

		class TH_OUT Var
		{
		public:
			class TH_OUT Set
			{
			public:
				static Schema* Auto(Variant&& Value);
				static Schema* Auto(const Variant& Value);
				static Schema* Auto(const std::string& Value, bool Strict = false);
				static Schema* Null();
				static Schema* Undefined();
				static Schema* Object();
				static Schema* Array();
				static Schema* Pointer(void* Value);
				static Schema* String(const std::string& Value);
				static Schema* String(const char* Value, size_t Size);
				static Schema* Base64(const std::string& Value);
				static Schema* Base64(const unsigned char* Value, size_t Size);
				static Schema* Base64(const char* Value, size_t Size);
				static Schema* Integer(int64_t Value);
				static Schema* Number(double Value);
				static Schema* Decimal(const BigNumber& Value);
				static Schema* Decimal(BigNumber&& Value);
				static Schema* DecimalString(const std::string& Value);
				static Schema* Boolean(bool Value);
			};

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
			static AllocCallback OnAlloc;
			static ReallocCallback OnRealloc;
			static FreeCallback OnFree;

		public:
			static void SetAlloc(const AllocCallback& Callback);
			static void SetRealloc(const ReallocCallback& Callback);
			static void SetFree(const FreeCallback& Callback);
			static void* Malloc(size_t Size);
			static void* Realloc(void* Ptr, size_t Size);
			static void Free(void* Ptr);
		};

		class TH_OUT OS
		{
		public:
			class TH_OUT CPU
			{
			public:
				enum class Arch
				{
					X64,
					ARM,
					Itanium,
					X86,
					Unknown,
				};

				enum class Endian
				{
					Little,
					Big,
				};

				enum class Cache
				{
					Unified,
					Instruction,
					Data,
					Trace,
				};

				struct QuantityInfo
				{
					uint32_t Logical;
					uint32_t Physical;
					uint32_t Packages;
				};

				struct CacheInfo
				{
					size_t Size;
					size_t LineSize;
					uint8_t Associativity;
					Cache Type;
				};

				static QuantityInfo GetQuantityInfo();
				static CacheInfo GetCacheInfo(unsigned int level);
				static Arch GetArch() noexcept;
				static Endian GetEndianness() noexcept;
				static uint64_t GetFrequency() noexcept;
			};

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
				static int Compare(const std::string& FirstPath, const std::string& SecondPath);
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
				static void Interrupt();
				static int Execute(const char* Format, ...);
				static bool Spawn(const std::string& Path, const std::vector<std::string>& Params, ChildProcess* Result);
				static bool Await(ChildProcess* Process, int* ExitCode);
				static bool Free(ChildProcess* Process);
                static std::string GetThreadId(const std::thread::id& Id);
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
#ifdef _DEBUG
		public:
			struct DbgContext
			{
				const char* File = nullptr;
				const char* Section = nullptr;
				const char* Function = nullptr;
				void* Id = nullptr;
				uint64_t Threshold = 0;
				uint64_t Time = 0;
				int Line = 0;
			};

		private:
			static std::vector<DbgContext> SpecFrame;
#endif
		private:
			static std::function<void(const char*, int)> Callback;
			static std::mutex Buffer;
			static bool Active;

		public:
#ifdef _DEBUG
			static void SpecPush(const char* File, const char* Section, const char* Function, int Line, uint64_t ThresholdMS, void* Id);
			static void SpecSignal();
			static void SpecPop(void* Id);
			static void PerfPush(const char* File, const char* Section, const char* Function, int Line, uint64_t ThresholdMS);
			static void PerfSignal();
			static void PerfPop();
#endif
			static void Assert(bool Fatal, int Line, const char* Source, const char* Function, const char* Condition, const char* Format, ...);
			static void Log(int Level, int Line, const char* Source, const char* Format, ...);
			static void Pause();
			static void SetLogCallback(const std::function<void(const char*, int)>& Callback);
			static void SetLogActive(bool Enabled);
			static std::string GetStackTrace(size_t Skips, size_t MaxFrames = 16);
		};

		class TH_OUT Composer
		{
		private:
			static std::unordered_map<uint64_t, std::pair<uint64_t, void*>>* Factory;

		public:
			static void AddRef(Object* Value);
			static void Release(Object* Value);
			static int GetRefCount(Object* Value);
			static bool Clear();
			static bool Pop(const std::string& Hash);
			static std::unordered_set<uint64_t> Fetch(uint64_t Id);

		private:
			static void Push(uint64_t TypeId, uint64_t Tag, void* Callback);
			static void* Find(uint64_t TypeId);

		public:
			template <typename T, typename... Args>
			static T* Create(const std::string& Hash, Args... Data)
			{
				return Create<T, Args...>(TH_COMPONENT_HASH(Hash), Data...);
			}
			template <typename T, typename... Args>
			static T* Create(uint64_t Id, Args... Data)
			{
				void* (*Callable)(Args...) = nullptr;
				reinterpret_cast<void*&>(Callable) = Find(Id);

				if (!Callable)
					return nullptr;

				return (T*)Callable(Data...);
			}
			template <typename T, typename... Args>
			static void Push(uint64_t Tag)
			{
				auto Callable = &Composer::Callee<T, Args...>;
				void* Result = reinterpret_cast<void*&>(Callable);
				Push(T::GetTypeId(), Tag, Result);
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

		public:
			Object() noexcept;
			virtual ~Object() noexcept;
			void operator delete(void* Data) noexcept;
			void* operator new(size_t Size) noexcept;
			int GetRefCount() noexcept;
			Object* AddRef() noexcept;
			Object* Release() noexcept;
		};

		class TH_OUT Console : public Object
		{
		protected:
#ifdef TH_MICROSOFT
			unsigned short Attributes;
			FILE* Conin;
			FILE* Conout;
			FILE* Conerr;
#endif
			std::mutex Lock;
			bool Coloring;
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
			void Trace(uint32_t MaxFrames = 32);
			void CaptureTime();
			void SetColoring(bool Enabled);
			void ColorBegin(StdColor Text, StdColor Background);
			void ColorEnd();
			void WriteBuffer(const char* Buffer);
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
			static bool Reset();
			static bool IsPresent();

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
			int64_t Time;

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

		class TH_OUT Costate : public Object
		{
		private:
			std::unordered_set<Coroutine*> Cached;
			std::unordered_set<Coroutine*> Used;
			std::thread::id Thread;
			Coroutine* Current;
			Cocontext* Master;
			size_t Size;

		public:
			std::function<void()> NotifyLock;
			std::function<void()> NotifyUnlock;

		public:
			Costate(size_t StackSize = TH_STACKSIZE);
			virtual ~Costate() override;
			Costate(const Costate&) = delete;
			Costate(Costate&&) = delete;
			Costate& operator= (const Costate&) = delete;
			Costate& operator= (Costate&&) = delete;
			Coroutine* Pop(const TaskCallback& Procedure);
			Coroutine* Pop(TaskCallback&& Procedure);
			int Reuse(Coroutine* Routine, const TaskCallback& Procedure);
			int Reuse(Coroutine* Routine, TaskCallback&& Procedure);
			int Reuse(Coroutine* Routine);
			int Push(Coroutine* Routine);
			int Activate(Coroutine* Routine);
			int Deactivate(Coroutine* Routine);
			int Deactivate(Coroutine* Routine, const TaskCallback& AfterSuspend);
			int Deactivate(Coroutine* Routine, TaskCallback&& AfterSuspend);
			int Resume(Coroutine* Routine);
			int Dispatch();
			int Suspend();
			void Clear();
			bool HasActive();
			Coroutine* GetCurrent() const;
			uint64_t GetCount() const;

		private:
			int Swap(Coroutine* Routine);

		public:
			static Costate* Get();
			static Coroutine* GetCoroutine();
			static bool GetState(Costate** State, Coroutine** Routine);
			static bool IsCoroutine();

		public:
			static void TH_COCALL ExecutionEntry(TH_CODATA);
		};

		class TH_OUT Schema : public Object
		{
		protected:
			std::vector<Schema*>* Nodes;
			Schema* Parent;
			bool Saved;

		public:
			std::string Key;
			Variant Value;

		public:
			Schema(const Variant& Base) noexcept;
			Schema(Variant&& Base) noexcept;
			virtual ~Schema() override;
			std::unordered_map<std::string, uint64_t> GetNames() const;
			std::vector<Schema*> FindCollection(const std::string& Name, bool Deep = false) const;
			std::vector<Schema*> FetchCollection(const std::string& Notation, bool Deep = false) const;
			std::vector<Schema*> GetAttributes() const;
			std::vector<Schema*>& GetChilds();
			Schema* Find(const std::string& Name, bool Deep = false) const;
			Schema* Fetch(const std::string& Notation, bool Deep = false) const;
			Variant FetchVar(const std::string& Key, bool Deep = false) const;
			Variant GetVar(size_t Index) const;
			Variant GetVar(const std::string& Key) const;
			Schema* GetParent() const;
			Schema* GetAttribute(const std::string& Key) const;
			Schema* Get(size_t Index) const;
			Schema* Get(const std::string& Key) const;
			Schema* Set(const std::string& Key);
			Schema* Set(const std::string& Key, const Variant& Value);
			Schema* Set(const std::string& Key, Variant&& Value);
			Schema* Set(const std::string& Key, Schema* Value);
			Schema* SetAttribute(const std::string& Key, const Variant& Value);
			Schema* SetAttribute(const std::string& Key, Variant&& Value);
			Schema* Push(const Variant& Value);
			Schema* Push(Variant&& Value);
			Schema* Push(Schema* Value);
			Schema* Pop(size_t Index);
			Schema* Pop(const std::string& Name);
			Schema* Copy() const;
			bool Rename(const std::string& Name, const std::string& NewName);
			bool Has(const std::string& Name) const;
			bool Has64(const std::string& Name, size_t Size = 12) const;
			bool IsEmpty() const;
			bool IsAttribute() const;
			bool IsSaved() const;
			size_t Size() const;
			std::string GetName() const;
			void Join(Schema* Other, bool Copy = true, bool Fast = true);
			void Reserve(size_t Size);
			void Clear();
			void Save();

		protected:
			void Allocate();
			void Allocate(const std::vector<Schema*>& Other);

		private:
			void Attach(Schema* Root);

		public:
			static bool Transform(Schema* Value, const SchemaNameCallback& Callback);
			static bool WriteXML(Schema* Value, const SchemaWriteCallback& Callback);
			static bool WriteJSON(Schema* Value, const SchemaWriteCallback& Callback);
			static bool WriteJSONB(Schema* Value, const SchemaWriteCallback& Callback);
			static Schema* ReadXML(const char* Buffer, bool Assert = true);
			static Schema* ReadJSON(const char* Buffer, size_t Size, bool Assert = true);
			static Schema* ReadJSONB(const SchemaReadCallback& Callback, bool Assert = true);

		private:
			static bool ProcessXMLRead(void* Base, Schema* Current);
			static bool ProcessJSONRead(void* Base, Schema* Current);
			static bool ProcessJSONBWrite(Schema* Current, std::unordered_map<std::string, uint64_t>* Map, const SchemaWriteCallback& Callback);
			static bool ProcessJSONBRead(Schema* Current, std::unordered_map<uint64_t, std::string>* Map, const SchemaReadCallback& Callback);
			static bool ProcessNames(const Schema* Current, std::unordered_map<std::string, uint64_t>* Map, uint64_t& Index);
		};

		template <typename T>
		class Pool
		{
		public:
			typedef T* Iterator;

		protected:
			size_t Count, Volume;
			T* Data;

		public:
			Pool() : Count(0), Volume(0), Data(nullptr)
			{
				Reserve(1);
			}
			Pool(size_t _Size, size_t _Capacity, T* _Data) : Count(_Size), Volume(_Capacity), Data(_Data)
			{
				if (!Data && Volume > 0)
					Reserve(Volume);
				else if (!Data || !Volume)
					Reserve(1);
			}
			Pool(size_t _Capacity) : Count(0), Volume(0), Data(nullptr)
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
			void Swap(Pool<T>& Raw)
			{
				size_t _Size = Raw.Count;
				Raw.Count = Count;
				Count = _Size;

				size_t _Capacity = Raw.Volume;
				Raw.Volume = Volume;
				Volume = _Capacity;

				T* _Data = Raw.Data;
				Raw.Data = Data;
				Data = _Data;
			}
			void Reserve(size_t NewCount)
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
			void Resize(size_t NewSize)
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
			void PopFront()
			{
				TH_ASSERT_V(Count > 0, "pool is empty");

				Count--;
				Data[0] = Data[Count];
				Dispose(Data);
			}
			void PopBack()
			{
				TH_ASSERT_V(Count > 0, "pool is empty");

				Count--;
				Dispose(Data + Count);
			}
			Iterator Add(const T& Ref)
			{
				TH_ASSERT(Count < Volume, End(), "pool capacity overflow");
				Data[Count++] = Ref;
				return End() - 1;
			}
			Iterator AddUnbounded(const T& Ref)
			{
				if (Count >= Volume)
					Reserve(IncreaseCapacity(Count + 1));

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
				TH_ASSERT(Count > 0, End(), "pool is empty");
				TH_ASSERT(It - Data >= 0 && It - Data < Count, End(), "iterator ranges out of pool");

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
			Iterator At(size_t Index) const
			{
				TH_ASSERT(Index < Count, End(), "index ranges out of pool");
				return Data + Index;
			}
			Iterator Find(const T& Ref)
			{
				auto End = Data + Count;
				for (auto It = Data; It != End; ++It)
				{
					if (*It == Ref)
						return It;
				}

				return End;
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
			T& operator [](size_t Index) const
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
			size_t Size() const
			{
				return Count;
			}
			size_t Capacity() const
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
			size_t SizeOf(Iterator A)
			{
				if (!std::is_pointer<T>::value)
					return sizeof(T);

				return sizeof(size_t);
			}
			size_t IncreaseCapacity(size_t NewSize)
			{
				size_t Alpha = Volume ? (Volume + Volume / 2) : 8;
				return Alpha > NewSize ? Alpha : NewSize;
			}
		};

		class TH_OUT Schedule : public Object
		{
		public:
			struct TH_OUT Desc
			{
				std::chrono::milliseconds Timeout = std::chrono::milliseconds(2000);
				uint64_t Threads[(size_t)Difficulty::Count] = { 1, 1, 1, 1 };
				uint64_t Memory = TH_STACKSIZE;
				uint64_t Coroutines = 16;
				ActivityCallback Ping = nullptr;
				bool Async = true;

				void SetThreads(uint64_t Cores);
			};

		private:
			struct ThreadPtr
			{
				std::condition_variable Notify;
				std::mutex Update;
				std::thread Handle;
				std::thread::id Id;
				bool Daemon = false;
			};

		private:
			struct
			{
				std::vector<TaskCallback*> Events;
				TaskCallback* Tasks[TH_MAX_EVENTS];
				Costate* State = nullptr;
			} Dispatcher;

		private:
			ConcurrentQueuePtr* Queues[(size_t)Difficulty::Count];
			std::vector<ThreadPtr*> Threads[(size_t)Difficulty::Count];
			std::atomic<TaskId> Generation;
			std::mutex Exclusive;
			Desc Policy;
			bool Enqueue;
			bool Terminate;
			bool Active;

		private:
			Schedule();

		public:
			virtual ~Schedule() override;
			TaskId SetInterval(uint64_t Milliseconds, const TaskCallback& Callback, Difficulty Type = Difficulty::Light);
			TaskId SetInterval(uint64_t Milliseconds, TaskCallback&& Callback, Difficulty Type = Difficulty::Light);
			TaskId SetTimeout(uint64_t Milliseconds, const TaskCallback& Callback, Difficulty Type = Difficulty::Light);
			TaskId SetTimeout(uint64_t Milliseconds, TaskCallback&& Callback, Difficulty Type = Difficulty::Light);
			bool SetTask(const TaskCallback& Callback, Difficulty Type = Difficulty::Heavy);
			bool SetTask(TaskCallback&& Callback, Difficulty Type = Difficulty::Heavy);
			bool SetChain(const TaskCallback& Callback);
			bool SetChain(TaskCallback&& Callback);
			bool ClearTimeout(TaskId WorkId);
			bool Start(const Desc& NewPolicy);
			bool Stop();
			bool Wakeup();
			bool Dispatch();
			bool IsActive();
			bool HasTasks(Difficulty Type);
			uint64_t GetTotalThreads();
			uint64_t GetThreads(Difficulty Type);
			const Desc& GetPolicy();

		private:
			bool ProcessTick(Difficulty Type);
			bool ProcessLoop(Difficulty Type, ThreadPtr* Thread);
			bool ThreadActive(ThreadPtr* Thread);
			bool ChunkCleanup();
			bool PushThread(Difficulty Type, bool IsDaemon);
			bool PopThread(ThreadPtr* Thread);
			std::chrono::microseconds GetTimeout(std::chrono::microseconds Clock);
			TaskId GetTaskId();

		public:
			static std::chrono::microseconds GetClock();
			static Schedule* Get();
			static bool Reset();

		private:
			static Schedule* Singleton;
		};

		template <typename T>
		class Awaitable
		{
		public:
			std::function<void()> Event;
			std::atomic<uint32_t> Count;
			std::atomic<short> Set;
			std::mutex Update;
			T Result;

		public:
			Awaitable() noexcept : Count(1), Set(-1), Result()
			{
			}
			Awaitable(const T& Value) noexcept : Count(1), Set(1), Result(Value)
			{
			}
			Awaitable(T&& Value) noexcept : Count(1), Set(1), Result(std::move(Value))
			{
			}
			Awaitable* Copy() noexcept
			{
				Count++;
				return this;
			}
			void Put(std::function<void()>&& Callback) noexcept
			{
				Update.lock();
				if (Set > 0)
				{
					Update.unlock();
					return Callback();
				}

				Event = std::move(Callback);
				Update.unlock();
			}
			void React() noexcept
			{
				Update.lock();
				Notify();
				Update.unlock();
			}
			void React(T&& Value) noexcept
			{
				Update.lock();
				Set = 1;
				Result = std::move(Value);
				Notify();
				Update.unlock();
			}
			void React(const T& Value) noexcept
			{
				Update.lock();
				Set = 1;
				Result = Value;
				Notify();
				Update.unlock();
			}
			void Free() noexcept
			{
				if (!--Count)
					TH_DELETE_THIS(Awaitable);
			}

		private:
			void Notify()
			{
				if (Event)
					Event();
			}
		};

		template <typename T>
		class Async
		{
			static_assert(!std::is_same<T, void>::value, "async cannot be used with void type");
			static_assert(std::is_default_constructible<T>::value, "async cannot be used with non default constructible type");
			typedef Awaitable<T> context_type;
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
			context_type* Next;

		private:
			Async(context_type* Context, bool) noexcept : Next(Context)
			{
				if (Next != nullptr)
					Next->Count++;
			}

		public:
			Async() noexcept : Next(TH_NEW(context_type))
			{
			}
			Async(const T& Value) noexcept : Next(TH_NEW(context_type, Value))
			{
			}
			Async(T&& Value) noexcept : Next(TH_NEW(context_type, std::move(Value)))
			{
			}
			Async(const Async& Other) noexcept : Next(Other.Next)
			{
				if (Next != nullptr)
					Next->Count++;
			}
			Async(Async&& Other) noexcept : Next(Other.Next)
			{
				Other.Next = nullptr;
			}
			~Async()
			{
				if (Next != nullptr)
					Next->Free();
			}
			Async& operator= (const T& Other)
			{
				Set(Other);
				return *this;
			}
			Async& operator= (T&& Other) noexcept
			{
				Set(std::move(Other));
				return *this;
			}
			Async& operator= (const Async& Other)
			{
				if (&Other != this)
					Set(Other);

				return *this;
			}
			Async& operator= (Async&& Other) noexcept
			{
				if (&Other == this)
					return *this;

				if (Next != nullptr)
					Next->Free();

				Next = Other.Next;
				Other.Next = nullptr;
				return *this;
			}
			void Set(const T& Other)
			{
				TH_ASSERT_V(Next != nullptr && Next->Set == -1, "async should be pending");
				Next->React(Other);
			}
			void Set(T&& Other)
			{
				TH_ASSERT_V(Next != nullptr && Next->Set == -1, "async should be pending");
				Next->React(std::move(Other));
			}
			void Set(const Async& Other)
			{
				TH_ASSERT_V(Next != nullptr && Next->Set == -1, "async should be pending");
				context_type* Subresult = Next->Copy();
				Subresult->Update.lock();
				Subresult->Set = 0;
				Subresult->Update.unlock();

				Other.Await([Subresult](T&& Value) mutable
				{
					Subresult->React(std::move(Value));
					Subresult->Free();
				});
			}
			void Set(Async&& Other)
			{
				TH_ASSERT_V(Next != nullptr && Next->Set == -1, "async should be pending");
				if (!Other.IsPending())
					return Next->React(std::move(Other.GetIfAny()));

				context_type* Subresult = Next->Copy();
				Subresult->Update.lock();
				Subresult->Set = 0;
				Subresult->Update.unlock();

				Other.Await([Subresult](T&& Value) mutable
				{
					Subresult->React(std::move(Value));
					Subresult->Free();
				});
			}
			void Await(std::function<void(T&&)>&& Callback) const noexcept
			{
				TH_ASSERT_V(Next != nullptr && Callback, "async should be pending");

				context_type* Subresult = Next->Copy();
				Next->Put([Subresult, Callback = std::move(Callback)]()
				{
					Schedule::Get()->SetTask([Subresult, Callback = std::move(Callback)]()
					{
						Callback(std::move(Subresult->Result));
						Subresult->Free();
					}, Difficulty::Light);
				});
			}
			bool IsPending() const noexcept
			{
				if (!Next)
					return false;

				Next->Update.lock();
				bool Result = (Next->Set != 1);
				Next->Update.unlock();
				return Result;
			}
			T&& GetIfAny() noexcept
			{
				if (Next != nullptr)
					return std::move(Next->Result);

				Next = TH_NEW(context_type);
				Next->Update.lock();
				Next->Set = 1;
				Next->Update.unlock();
				return std::move(Next->Result);
			}
			T&& Get() noexcept
			{
				if (!IsPending())
					return GetIfAny();

				std::mutex Waitable;
				std::condition_variable Ready;

				Await([&](T&&)
				{
					std::unique_lock<std::mutex> Lock(Waitable);
					Ready.notify_all();
				});

				std::unique_lock<std::mutex> Lock(Waitable);
				Ready.wait(Lock, [&]()
				{
					return !IsPending();
				});

				return GetIfAny();
			}

		public:
			template <typename R>
			Async<R> Then(std::function<void(Async<R>&, T&&)>&& Callback) const noexcept
			{
				TH_ASSERT(Next != nullptr && Callback, Async<R>::Move(), "async should be pending");

				Async<R> Result; context_type* Subresult = Next->Copy();
				Next->Put([Subresult, Result, Callback = std::move(Callback)]() mutable
				{
					Schedule::Get()->SetTask([Subresult, Result = std::move(Result), Callback = std::move(Callback)]() mutable
					{
						Callback(Result, std::move(Subresult->Result));
						Subresult->Free();
					}, Difficulty::Light);
				});

				return Result;
			}
			template <typename R>
			Async<typename Future<R>::type> Then(std::function<R(T&&)>&& Callback) const noexcept
			{
				using F = typename Future<R>::type;
				TH_ASSERT(Next != nullptr && Callback, Async<F>::Move(), "async should be pending");

				Async<F> Result; context_type* Subresult = Next->Copy();
				Next->Put([Subresult, Result, Callback = std::move(Callback)]() mutable
				{
					Schedule::Get()->SetTask([Subresult, Result = std::move(Result), Callback = std::move(Callback)]() mutable
					{
						Result.Set(std::move(Callback(std::move(Subresult->Result))));
						Subresult->Free();
					}, Difficulty::Light);
				});

				return Result;
			}

		public:
			static Async Move(context_type* Base = nullptr) noexcept
			{
				return Async(Base, true);
			}
			static Async Execute(std::function<void(Async&)>&& Callback) noexcept
			{
				if (!Callback)
					return Move();

				Async Result;
				Schedule::Get()->SetTask([Result, Callback = std::move(Callback)]() mutable
				{
					Callback(Result);
				}, Difficulty::Heavy);

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
					Value = Match;
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
				TH_ASSERT(Callback && !Array.empty(), false, "array and callback should not be empty");
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
#ifdef _DEBUG
		template <typename T>
		inline T&& Coawait(Async<T>&& Future, const char* File = nullptr, const char* Function = nullptr, const char* Expression = nullptr, int Line = 0) noexcept
		{
			Costate* State; Coroutine* Base;
			if (!Costate::GetState(&State, &Base) || !Future.IsPending())
				return Future.Get();

			bool Tracking = (File && Function && Expression && Line >= 0);
			if (Tracking)
				OS::SpecPush(File, Expression, Function, Line, TH_PERF_HANG, (void*)&Future);

			State->Deactivate(Base, [&Future, &State, &Base]()
			{
				Future.Await([&State, &Base](T&&)
				{
					State->Activate(Base);
				});
			});

			if (Tracking)
				OS::SpecPop((void*)&Future);

			return Future.GetIfAny();
		}
#else
		template <typename T>
		inline T&& Coawait(Async<T>&& Future) noexcept
		{
			Costate* State; Coroutine* Base;
			if (!Costate::GetState(&State, &Base) || !Future.IsPending())
				return Future.Get();

			State->Deactivate(Base, [&Future, &State, &Base]()
			{
				Future.Await([&State, &Base](T&&)
				{
					State->Activate(Base);
				});
			});

			return Future.GetIfAny();
		}
#endif
		template <typename T>
		inline Async<T> Cotask(const std::function<T()>& Callback, Difficulty Type = Difficulty::Heavy) noexcept
		{
			TH_ASSERT(Callback, Async<T>::Move(), "callback should not be empty");

			Async<T> Result;
			Schedule::Get()->SetTask([Result, Callback]() mutable
			{
				Result = std::move(Callback());
			}, Type);

			return Result;
		}
		template <typename T>
		inline Async<T> Cotask(std::function<T()>&& Callback, Difficulty Type = Difficulty::Heavy) noexcept
		{
			TH_ASSERT(Callback, Async<T>::Move(), "callback should not be empty");

			Async<T> Result;
			Schedule::Get()->SetTask([Result, Callback = std::move(Callback)]() mutable
			{
				Result = std::move(Callback());
			}, Type);

			return Result;
		}
		template <typename T>
		inline Async<T> Coasync(const std::function<T()>& Callback) noexcept
		{
			TH_ASSERT(Callback, Async<T>::Move(), "callback should not be empty");

			Async<T> Result;
			Schedule::Get()->SetChain([Result, Callback]() mutable
			{
				Result = std::move(Callback());
			});

			return Result;
		}
		template <typename T>
		inline Async<T> Coasync(std::function<T()>&& Callback) noexcept
		{
			TH_ASSERT(Callback, Async<T>::Move(), "callback should not be empty");

			Async<T> Result;
			Schedule::Get()->SetChain([Result, Callback = std::move(Callback)]() mutable
			{
				Result = std::move(Callback());
			});

			return Result;
		}
		inline Async<bool> Cosleep(uint64_t Ms) noexcept
		{
			Async<bool> Result;
			Schedule::Get()->SetTimeout(Ms, [Result]() mutable
			{
				Result = true;
			}, Difficulty::Light);

			return Result;
		}
		inline bool Coasync(const TaskCallback& Callback) noexcept
		{
			return Schedule::Get()->SetChain(Callback);
		}
		inline bool Coasync(TaskCallback&& Callback) noexcept
		{
			return Schedule::Get()->SetChain(std::move(Callback));
		}
		inline bool Cosuspend() noexcept
		{
			Costate* State = Costate::Get();
			TH_ASSERT(State != nullptr, false, "cannot call suspend outside coroutine");

			return State->Suspend();
		}
		inline Parser Form(const char* Format, ...)
		{
			TH_ASSERT(Format != nullptr, Parser(), "format should be set");

			va_list Args;
			va_start(Args, Format);

			char Buffer[10240];
			int Size = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			return Parser(Buffer, Size > 16384 ? 16384 : (size_t)Size);
		}
		template <size_t Size>
		constexpr uint64_t Shuffle(const char Source[Size])
		{
			uint64_t Result = 0xcbf29ce484222325;
			for (size_t i = 0; i < Size; i++)
			{
				Result ^= Source[i];
				Result *= 1099511628211;
			}

			return Result;
		}
	}
}
#endif