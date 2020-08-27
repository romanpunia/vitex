#ifndef THAWK_REST_H
#define THAWK_REST_H
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
#ifdef THAWK_LIBRARY
#define THAWK_OUT __declspec(dllimport)
#else
#define THAWK_OUT __declspec(dllexport)
#endif
#define THAWK_MICROSOFT
#define THAWK_FUNCTION __FUNCTION__
#ifdef _WIN64
#define THAWK_64
#else
#define THAWK_32
#endif
#define THAWK_MAX_PATH MAX_PATH
#define THAWK_STRINGIFY(X) #X
#define THAWK_FUNCTION __FUNCTION__
#define THAWK_CDECL __cdecl
#undef THAWK_UNIX
#elif defined __linux__ && defined __GNUC__
#define THAWK_OUT
#define THAWK_UNIX
#define THAWK_MAX_PATH _POSIX_PATH_MAX
#define THAWK_STRINGIFY(X) #X
#define THAWK_FUNCTION __FUNCTION__
#define THAWK_CDECL
#undef THAWK_MICROSOFT
#if __x86_64__ || __ppc64__
#define THAWK_64
#else
#define THAWK_32
#endif
#elif __APPLE__
#define THAWK_OUT
#define THAWK_UNIX
#define THAWK_MAX_PATH _POSIX_PATH_MAX
#define THAWK_STRINGIFY(X) #X
#define THAWK_FUNCTION __FUNCTION__
#define THAWK_CDECL
#define THAWK_APPLE
#undef THAWK_MICROSOFT
#if __x86_64__ || __ppc64__
#define THAWK_64
#else
#define THAWK_32
#endif
#endif
#ifndef THAWK_MICROSOFT
#include <sys/types.h>
#endif
#ifdef max
#undef max
#endif
#define THAWK_VA_ARGS(...) , ##__VA_ARGS__
#ifdef THAWK_MICROSOFT
#ifdef THAWK_64
typedef uint64_t socket_t;
#elif defined(THAWK_32)
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
#if THAWK_DLEVEL >= 3
#define THAWK_INFO(Format, ...) Tomahawk::Rest::LT::Inform(3, THAWK_FUNCTION, Format THAWK_VA_ARGS(__VA_ARGS__))
#define THAWK_WARN(Format, ...) Tomahawk::Rest::LT::Inform(2, THAWK_FUNCTION, Format THAWK_VA_ARGS(__VA_ARGS__))
#define THAWK_ERROR(Format, ...) Tomahawk::Rest::LT::Inform(1, THAWK_FUNCTION, Format THAWK_VA_ARGS(__VA_ARGS__))
#elif THAWK_DLEVEL >= 2
#define THAWK_INFO(Format, ...)
#define THAWK_WARN(Format, ...) Tomahawk::Rest::LT::Inform(2, THAWK_FUNCTION, Format THAWK_VA_ARGS(__VA_ARGS__))
#define THAWK_ERROR(Format, ...) Tomahawk::Rest::LT::Inform(1, THAWK_FUNCTION, Format THAWK_VA_ARGS(__VA_ARGS__))
#elif THAWK_DLEVEL >= 1
#define THAWK_INFO(Format, ...)
#define THAWK_WARN(Format, ...)
#define THAWK_ERROR(Format, ...) Tomahawk::Rest::LT::Inform(1, THAWK_FUNCTION, Format THAWK_VA_ARGS(__VA_ARGS__))
#else
#define THAWK_INFO(...)
#define THAWK_WARN(...)
#define THAWK_ERROR(...)
#endif
#define THAWK_LOG(Format, ...) Tomahawk::Rest::LT::Inform(0, THAWK_FUNCTION, Format THAWK_VA_ARGS(__VA_ARGS__))
#define THAWK_COMPONENT_ID(ClassName) (uint64_t)std::hash<std::string>()(#ClassName)
#define THAWK_COMPONENT_HASH(ClassName) (uint64_t)std::hash<std::string>()(ClassName)
#define THAWK_COMPONENT(ClassName) \
virtual const char* Name() override { static const char* V = #ClassName; return V; } \
virtual uint64_t Id() override { static uint64_t V = THAWK_COMPONENT_ID(ClassName); return V; } \
static const char* BaseName() { static const char* V = #ClassName; return V; } \
static uint64_t BaseId() { static uint64_t V = THAWK_COMPONENT_ID(ClassName); return V; }
#define THAWK_COMPONENT_BASIS(ClassName) \
virtual const char* Name() { static const char* V = #ClassName; return V; } \
virtual uint64_t Id() { static uint64_t V = THAWK_COMPONENT_ID(ClassName); return V; } \
static const char* BaseName() { static const char* V = #ClassName; return V; } \
static uint64_t BaseId() { static uint64_t V = THAWK_COMPONENT_ID(ClassName); return V; }

namespace Tomahawk
{
	namespace Rest
	{
		class EventWorker;

		class EventQueue;

		class Document;

		class Object;

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

		enum NodeType
		{
			NodeType_Undefined,
			NodeType_Object,
			NodeType_Array,
			NodeType_String,
			NodeType_Integer,
			NodeType_Number,
			NodeType_Boolean,
			NodeType_Null,
			NodeType_Id,
			NodeType_Decimal
		};

		enum DocumentPretty
		{
			DocumentPretty_Dummy,
			DocumentPretty_Tab_Decrease,
			DocumentPretty_Tab_Increase,
			DocumentPretty_Write_Space,
			DocumentPretty_Write_Line,
			DocumentPretty_Write_Tab,
		};

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

		typedef std::function<void(class EventQueue*, struct EventArgs*)> BaseCallback;
		typedef std::function<bool(class EventQueue*, struct EventArgs*)> PullCallback;
		typedef std::function<void(DocumentPretty, const char*, int64_t)> NWriteCallback;
		typedef std::function<bool(char*, int64_t)> NReadCallback;

		struct THAWK_OUT FileState
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

		struct THAWK_OUT EventArgs
		{
			friend EventQueue;

		private:
			EventWorker* Worker = nullptr;

		public:
			void* Data = nullptr;
			uint64_t Hash = 0;
			bool Alive = true;

		public:
			bool Blockable();
			template <typename T>
			void Free()
			{
				if (Data != nullptr)
				{
					delete (T*)Data;
					Data = nullptr;
				}
			}
			template <typename T>
			bool Is()
			{
				return typeid(T).hash_code() == Hash;
			}
			template <typename T>
			T* Get()
			{
				return (T*)Data;
			}
		};

		struct THAWK_OUT EventBase
		{
			uint64_t Type = 0;
			EventArgs Args;
			BaseCallback Callback;
		};

		struct THAWK_OUT EventTimer
		{
			BaseCallback Callback;
			EventArgs Args;
			uint64_t Timeout = 0;
			uint64_t Id = 0;
			int64_t Time = 0;
		};

		struct THAWK_OUT EventListener
		{
			uint64_t Type = 0;
			BaseCallback Callback;
		};

		struct THAWK_OUT Resource
		{
			uint64_t Size = 0;
			int64_t LastModified = 0;
			int64_t CreationTime = 0;
			bool IsReferenced = false;
			bool IsDirectory = false;
		};

		struct THAWK_OUT ResourceEntry
		{
			std::string Path;
			Resource Source;
			void* UserData = nullptr;
		};

		struct THAWK_OUT DirectoryEntry
		{
			std::string Path;
			bool IsDirectory = false;
			bool IsGood = false;
			uint64_t Length = 0;
		};

		struct THAWK_OUT ChildProcess
		{
			friend class OS;

		private:
			bool Valid = false;

		public:
#ifdef THAWK_MICROSOFT
			void* Process = nullptr;
			void* Thread = nullptr;
			void* Job = nullptr;
#else
			pid_t Process;
#endif
		};

		struct THAWK_OUT DateTime
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

		struct THAWK_OUT Stroke
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
			Stroke& Reserve(uint64_t Count = 1);
			Stroke& Resize(uint64_t Count);
			Stroke& Resize(uint64_t Count, char Char);
			Stroke& Clear();
			Stroke& ToUtf8();
			Stroke& ToUpper();
			Stroke& ToLower();
			Stroke& Clip(uint64_t Length);
			Stroke& ReplaceOf(const char* Chars, const char* To, uint64_t Start = 0U);
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
			bool StartsWith(const std::string& Value, uint64_t Offset = 0U) const;
			bool StartsWith(const char* Value, uint64_t Offset = 0U) const;
			bool StartsOf(const char* Value, uint64_t Offset = 0U) const;
			bool EndsWith(const std::string& Value) const;
			bool EndsOf(const char* Value) const;
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
			double ToFloat64() const;
			long double ToLFloat64() const;
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
			Stroke& operator = (const Stroke& New);

		public:
			static bool IsDigit(char Char);
			static int CaseCompare(const char* Value1, const char* Value2);
			static int Match(const char* Pattern, const char* Text);
			static int Match(const char* Pattern, uint64_t Length, const char* Text);
		};

		struct THAWK_OUT TickTimer
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
					Reserve(Capacity);
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
				T* Raw = (T*)malloc((size_t)(Volume * SizeOf(Data)));
				memset(Raw, 0, (size_t)(Volume * SizeOf(Data)));

				if (!Assign(Begin(), End(), Raw))
					memcpy(Raw, Data, (size_t)(Count * SizeOf(Data)));

				if (Data != nullptr)
				{
					for (auto It = Begin(); It != End(); It++)
						Dispose(It);

					free(Data);
				}
				Data = Raw;
			}
			void Release()
			{
				if (Data != nullptr)
				{
					for (auto It = Begin(); It != End(); It++)
						Dispose(It);

					free(Data);
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
					Data = (T*)malloc((size_t)(Raw.Count * SizeOf(Data)));
					memset(Data, 0, (size_t)(Raw.Count * SizeOf(Data)));
				}
				else
					Data = (T*)realloc(Data, (size_t)(Raw.Volume * SizeOf(Data)));

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

		class THAWK_OUT LT
		{
			friend class Object;

		private:
#ifndef NDEBUG
			static uint64_t Memory;
			static std::unordered_map<void*, uint64_t>* Objects;
			static std::mutex* Safe;
#endif
			static std::function<void(const char*, int)> Callback;
			static bool Enabled;

		public:
			static void AddRef(Object* Value);
			static void SetFlag(Object* Value);
			static void Release(Object* Value);
			static bool GetFlag(Object* Value);
			static int GetRefCount(Object* Value);
			static void AttachCallback(const std::function<void(const char*, int)>& Callback);
			static void AttachStream();
			static void DetachCallback();
			static void DetachStream();
			static void Inform(int Level, const char* Source, const char* Format, ...);
			static void* GetPtr(void* Ptr);
			static uint64_t GetSize(void* Ptr);
			static uint64_t GetCount();
			static uint64_t GetMemory();
			static void Free(void* Ptr);
			static void* Alloc(uint64_t Size);
		};

		class THAWK_OUT Composer
		{
		private:
			static std::unordered_map<uint64_t, void*>* Factory;

		public:
			static bool Clear();
			static bool Pop(const std::string& Hash);

		public:
			template <typename T, typename... Args>
			static T* Create(const std::string& Hash, Args... Data)
			{
				return Create<T, Args...>(THAWK_COMPONENT_HASH(Hash), Data...);
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
			static void Push(const std::string& Hash)
			{
				if (!Factory)
					Factory = new std::unordered_map<uint64_t, void*>();

				auto Callable = &Composer::Callee<T, Args...>;
				void* Result = reinterpret_cast<void*&>(Callable);
				(*Factory)[THAWK_COMPONENT_HASH(Hash)] = Result;
			}

		private:
			template <typename T, typename... Args>
			static void* Callee(Args... Data)
			{
				return (void*)new T(Data...);
			}
		};

		class THAWK_OUT Object
		{
			friend class LT;

		private:
			std::atomic<int> __vcnt;
			std::atomic<bool> __vflg;

		public:
			Object();
			virtual ~Object() = default;
			void* operator new(size_t Size);
			void operator delete(void* Data);
			void SetFlag();
			bool GetFlag();
			int GetRefCount();
			Object* AddRef();
			Object* Release();

		public:
			template <typename T>
			T* AddRefAs()
			{
				return (T*)AddRef();
			}
			template <typename T>
			T* ReleaseAs()
			{
				return (T*)Release();
			}
			template <typename T>
			T* As()
			{
				return (T*)this;
			}
		};

		class THAWK_OUT Console : public Object
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

		class THAWK_OUT Timer : public Object
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

		class THAWK_OUT FileStream : public Object
		{
		protected:
			void* Compress = nullptr;
			FILE* Buffer = nullptr;
			std::string Path;

		public:
			FileStream();
			virtual ~FileStream() override;
			void Clear();
			bool Open(const char* File, FileMode Mode);
			bool OpenZ(const char* File, FileMode Mode);
			bool Close();
			bool Seek(FileSeek Mode, int64_t Offset);
			bool Move(int64_t Offset);
			int Error();
			int Flush();
			int Fd();
			unsigned char Get();
			unsigned char Put(unsigned char Value);
			uint64_t ReadAny(const char* Format, ...);
			uint64_t Read(char* Buffer, uint64_t Length);
			uint64_t WriteAny(const char* Format, ...);
			uint64_t Write(const char* Buffer, uint64_t Length);
			uint64_t Tell();
			uint64_t Size();
			std::string& Filename();
			FILE* Stream();
			void* StreamZ();
		};

		class THAWK_OUT FileLogger : public Object
		{
		private:
			std::string LastValue;
			uint64_t Offset;

		public:
			FileStream* Stream = nullptr;
			std::string Path, Name;

		public:
			FileLogger(const std::string& Root);
			virtual ~FileLogger() override;
			void Process(const std::function<bool(FileLogger*, const char*, int64_t)>& Callback);
		};

		class THAWK_OUT FileTree : public Object
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

		class THAWK_OUT OS
		{
		public:
			static void SetDirectory(const char* Path);
			static void SaveBitmap(const char* Path, int Width, int Height, unsigned char* Ptr);
			static bool Iterate(const char* Path, const std::function<bool(DirectoryEntry*)>& Callback);
			static bool FileExists(const char* Path);
			static bool ExecExists(const char* Path);
			static bool DirExists(const char* Path);
			static bool Write(const char* Path, const char* Data, uint64_t Length);
			static bool Write(const char* Path, const std::string& Data);
			static bool RemoveFile(const char* Path);
			static bool RemoveDir(const char* Path);
			static bool Move(const char* From, const char* To);
			static bool StateResource(const std::string& Path, Resource* Resource);
			static bool ScanDir(const std::string& Path, std::vector<ResourceEntry>* Entries);
			static bool ConstructETag(char* Buffer, uint64_t Length, Resource* Resource);
			static bool ConstructETagManually(char* Buffer, uint64_t Length, int64_t LastModified, uint64_t ContentLength);
			static bool SpawnProcess(const std::string& Path, const std::vector<std::string>& Params, ChildProcess* Result);
			static bool FreeProcess(ChildProcess* Process);
			static bool AwaitProcess(ChildProcess* Process, int* ExitCode);
			static bool UnloadObject(void* Handle);
			static bool SendFile(FILE* Stream, socket_t Socket, int64_t Size);
			static bool CreateDir(const char* Path);
			static socket_t GetFD(FILE* Stream);
			static int GetError();
			static std::string GetErrorName(int Code);
			static void Run(const char* Format, ...);
			static void* LoadObject(const char* Path);
			static void* LoadObjectFunction(void* Handle, const char* Name);
			static void* Open(const char* Path, const char* Mode);
			static std::string Resolve(const char* Path);
			static std::string Resolve(const std::string& Path, const std::string& Directory);
			static std::string ResolveDir(const char* Path);
			static std::string ResolveDir(const std::string& Path, const std::string& Directory);
			static std::string GetDirectory();
			static std::string Read(const char* Path);
			static std::string FileDirectory(const std::string& Path, int Level = 0);
			static FileState GetState(const char* Path);
			static std::vector<std::string> ReadAllLines(const char* Path);
			static std::vector<std::string> GetDiskDrives();
			static const char* FileExtention(const char* Path);
			static unsigned char* ReadAllBytes(const char* Path, uint64_t* ByteLength);
			static unsigned char* ReadAllBytes(FileStream* Stream, uint64_t* ByteLength);
			static unsigned char* ReadByteChunk(FileStream* Stream, uint64_t Length);
			static bool WantTextInput(const std::string& Title, const std::string& Message, const std::string& DefaultInput, std::string* Result);
			static bool WantPasswordInput(const std::string& Title, const std::string& Message, std::string* Result);
			static bool WantFileSave(const std::string& Title, const std::string& DefaultPath, const std::string& Filter, const std::string& FilterDescription, std::string* Result);
			static bool WantFileOpen(const std::string& Title, const std::string& DefaultPath, const std::string& Filter, const std::string& FilterDescription, bool Multiple, std::string* Result);
			static bool WantFolder(const std::string& Title, const std::string& DefaultPath, std::string* Result);
			static bool WantColor(const std::string& Title, const std::string& DefaultHexRGB, std::string* Result);
		};

		class THAWK_OUT EventWorker : public Object
		{
			friend EventArgs;

		private:
			EventQueue* Queue;
			std::thread Thread;
			bool Extended;

		public:
			EventWorker(EventQueue* Value, bool IsTask);
			virtual ~EventWorker() override;

		private:
			bool QueueTask();
			bool QueueEvent();
		};

		class THAWK_OUT EventQueue : public Object
		{
			friend EventArgs;
			friend EventWorker;

		private:
			struct
			{
				std::vector<EventWorker*> Workers[2];
				std::condition_variable Condition[2];
				std::thread Thread[2];
				std::mutex Safe[2];
			} Async;

			struct
			{
				std::mutex Events;
				std::mutex Tasks;
				std::mutex Timers;
				std::mutex Listeners;
			} Sync;

		private:
			std::unordered_map<uint64_t, EventListener*> Listeners;
			std::deque<EventBase*> Events;
			std::deque<EventBase*> Tasks;
			std::vector<EventTimer*> Timers;
			EventState State = EventState_Terminated;
			uint64_t Timer = 0, Idle = 1;
			int Synchronize = 0;

		public:
			void* UserData = nullptr;

		public:
			EventQueue();
			virtual ~EventQueue() override;
			void SetIdleTime(uint64_t IdleTime);
			void SetState(EventState NewState);
			bool Tick();
			bool Start(EventWorkflow Workflow, uint64_t TaskWorkers, uint64_t EventWorkers);
			bool Stop();
			bool Expire(uint64_t TimerId);
			EventState GetState();

		private:
			bool MixedLoop();
			bool DefaultLoop();
			bool TimerLoop();
			bool QueueEvent(EventWorker* Worker);
			bool QueueTask(EventWorker* Worker);
			bool QueueTimer(int64_t Time);
			bool AddEvent(EventBase* Value);
			bool AddTask(EventBase* Value);
			bool AddTimer(EventTimer* Value);
			bool AddListener(EventListener* Value);
			bool RemoveCallback(EventBase* Value);
			bool RemoveListener(uint64_t Value);
			bool RemoveAny(EventType Type, uint64_t Hash, const PullCallback& Callback, bool NoCall);
			int64_t Clock();

		public:
			template <typename T>
			bool Task(T* Args, const BaseCallback& Callback, bool Keep = false, EventBase** Event = nullptr)
			{
				if (!Callback)
					return false;

				auto* Value = new EventBase();
				Value->Type = typeid(T).hash_code();
				Value->Args.Hash = Value->Type;
				Value->Args.Data = (void*)Args;
				Value->Args.Alive = Keep;
				Value->Args.Worker = nullptr;
				Value->Callback = Callback;

				if (Event != nullptr)
					*Event = Value;

				return AddTask(Value);
			}
			template <typename T>
			bool Event(T* Args, bool Keep = false, EventBase** Event = nullptr)
			{
				auto* Value = new EventBase();
				Value->Type = typeid(T).hash_code();
				Value->Args.Hash = Value->Type;
				Value->Args.Data = (void*)Args;
				Value->Args.Worker = nullptr;
				Value->Args.Alive = Keep;

				if (!RemoveCallback(Value))
				{
					delete Value;
					if (Event != nullptr)
						*Event = nullptr;

					return false;
				}
				else if (Event != nullptr)
					*Event = Value;

				return AddEvent(Value);
			}
			template <typename T>
			bool Callback(T* Args, const BaseCallback& Callback, bool Keep = false, EventBase** Event = nullptr)
			{
				if (!Callback)
					return false;

				auto* Value = new EventBase();
				Value->Type = typeid(EventBase).hash_code();
				Value->Args.Hash = typeid(T).hash_code();
				Value->Args.Data = (void*)Args;
				Value->Args.Alive = Keep;
				Value->Args.Worker = nullptr;
				Value->Callback = Callback;

				if (Event != nullptr)
					*Event = Value;

				return AddEvent(Value);
			}
			template <typename T>
			bool Interval(T* Args, uint64_t Milliseconds, const BaseCallback& Callback, EventTimer** Event = nullptr)
			{
				if (!Callback)
					return false;

				auto* Value = new EventTimer();
				Value->Args.Hash = typeid(T).hash_code();
				Value->Args.Data = (void*)Args;
				Value->Args.Alive = true;
				Value->Args.Worker = nullptr;
				Value->Callback = Callback;
				Value->Timeout = Milliseconds;
				Value->Time = Clock();
				Value->Id = Timer++;

				if (Event != nullptr)
					*Event = Value;

				return AddTimer(Value);
			}
			template <typename T>
			bool Timeout(T* Args, uint64_t Milliseconds, const BaseCallback& Callback, EventTimer** Event = nullptr)
			{
				if (!Callback)
					return false;

				auto* Value = new EventTimer();
				Value->Args.Hash = typeid(T).hash_code();
				Value->Args.Data = (void*)Args;
				Value->Args.Alive = false;
				Value->Args.Worker = nullptr;
				Value->Callback = Callback;
				Value->Timeout = Milliseconds;
				Value->Time = Clock();
				Value->Id = Timer++;

				if (Event != nullptr)
					*Event = Value;

				return AddTimer(Value);
			}
			template <typename T>
			bool Subscribe(const BaseCallback& Callback, EventListener** Event = nullptr)
			{
				if (!Callback)
					return false;

				auto* Value = new EventListener();
				Value->Type = typeid(T).hash_code();
				Value->Callback = Callback;

				return AddListener(Value);
			}
			template <typename T>
			bool Unsubscribe()
			{
				return RemoveListener(typeid(T).hash_code());
			}
			template <typename T>
			bool Pull(EventType Type, const PullCallback& Callback = nullptr, bool NoCall = false)
			{
				return RemoveAny(Type, typeid(T).hash_code(), Callback, NoCall);
			}
		};

		class THAWK_OUT Document : public Object
		{
		protected:
			std::vector<Document*> Nodes;
			Document* Parent;

		public:
			std::string Name;
			std::string String;
			NodeType Type;
			int64_t Low;
			int64_t Integer;
			double Number;
			bool Boolean;
			bool Saved;

		public:
			Document();
			virtual ~Document() override;
			void Join(Document* Other);
			void Clear();
			void Save();
			Document* GetIndex(int64_t Index);
			Document* Get(const std::string& Name);
			Document* SetCast(const std::string& Name, const std::string& Prop);
			Document* SetUndefined(const std::string& Name);
			Document* SetNull(const std::string& Name);
			Document* SetId(const std::string& Name, unsigned char Value[12]);
			Document* SetDocument(const std::string& Name, Document* Value);
			Document* SetDocument(const std::string& Name);
			Document* SetArray(const std::string& Name, Document* Value);
			Document* SetArray(const std::string& Name);
			Document* SetAttribute(const std::string& Name, const std::string& Value);
			Document* SetString(const std::string& Name, const char* Value, int64_t Size);
			Document* SetString(const std::string& Name, const std::string& Value);
			Document* SetInteger(const std::string& Name, int64_t Value);
			Document* SetNumber(const std::string& Name, double Value);
			Document* SetDecimal(const std::string& Name, int64_t High, int64_t Low);
			Document* SetDecimal(const std::string& Name, const std::string& Value);
			Document* SetBoolean(const std::string& Name, bool Value);
			Document* Copy();
			Document* GetParent();
			Document* GetAttribute(const std::string& Name);
			bool IsAttribute();
			bool IsObject();
			bool Deserialize(const std::string& Value);
			bool GetBoolean(const std::string& Name);
			bool GetNull(const std::string& Name);
			bool GetUndefined(const std::string& Name);
			int64_t Size();
			int64_t GetDecimal(const std::string& Name, int64_t* Low);
			int64_t GetInteger(const std::string& Name);
			double GetNumber(const std::string& Name);
			unsigned char* GetId(const std::string& Name);
			const char* GetString(const std::string& Name);
			std::string& GetStringBlob(const std::string& Name);
			std::string GetName();
			std::string Serialize();
			Document* Find(const std::string& Name, bool Here = false);
			Document* FindPath(const std::string& Notation, bool Here = false);
			std::vector<Document*> FindCollection(const std::string& Name, bool Here = false);
			std::vector<Document*> FindCollectionPath(const std::string& Notation, bool Here = false);
			std::vector<Document*> GetAttributes();
			std::vector<Document*>* GetNodes();
			std::unordered_map<std::string, uint64_t> CreateMapping();

		public:
			static bool WriteBIN(Document* Value, const NWriteCallback& Callback);
			static bool WriteXML(Document* Value, const NWriteCallback& Callback);
			static bool WriteJSON(Document* Value, const NWriteCallback& Callback);
			static Document* ReadBIN(const NReadCallback& Callback);
			static Document* ReadXML(int64_t Size, const NReadCallback& Callback);
			static Document* ReadJSON(int64_t Size, const NReadCallback& Callback);
			static Document* NewArray();
			static Document* NewUndefined();
			static Document* NewNull();
			static Document* NewId(unsigned char Value[12]);
			static Document* NewString(const char* Value, int64_t Size);
			static Document* NewString(const std::string& Value);
			static Document* NewInteger(int64_t Value);
			static Document* NewNumber(double Value);
			static Document* NewDecimal(int64_t High, int64_t Low);
			static Document* NewDecimal(const std::string& Value);
			static Document* NewBoolean(bool Value);

		private:
			static void ProcessBINWrite(Document* Current, std::unordered_map<std::string, uint64_t>* Map, const NWriteCallback& Callback);
			static bool ProcessBINRead(Document* Current, std::unordered_map<uint64_t, std::string>* Map, const NReadCallback& Callback);
			static bool ProcessMAPRead(Document* Current, std::unordered_map<std::string, uint64_t>* Map, uint64_t& Index);
			static bool ProcessXMLRead(void* Base, Document* Current);
			static bool ProcessJSONRead(Document* Current);
			static bool Deserialize(const std::string& Value, Document* Output);
			static std::string Serialize(Document* Value);
		};

		THAWK_OUT Stroke Form(const char* Format, ...);
	}
}
#endif