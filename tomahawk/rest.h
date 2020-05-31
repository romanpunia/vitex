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

typedef long long Int64;
typedef unsigned long long UInt64;
typedef double Float64;
typedef long double LFloat64;

#define THAWK_VA_ARGS(...) , ##__VA_ARGS__
#ifdef THAWK_MICROSOFT
#ifdef THAWK_64
typedef UInt64 socket_t;
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
        typedef std::function<void(DocumentPretty, const char*, Int64)> NWriteCallback;
        typedef std::function<bool(char*, Int64)> NReadCallback;

        struct THAWK_OUT FileState
        {
            UInt64 Size = 0;
            UInt64 Links = 0;
            UInt64 Permissions = 0;
            UInt64 IDocument = 0;
            UInt64 Device = 0;
            UInt64 UserId = 0;
            UInt64 GroupId = 0;
            Int64 LastAccess = 0;
            Int64 LastPermissionChange = 0;
            Int64 LastModified = 0;
            bool Exists = false;
        };

        struct THAWK_OUT EventArgs
        {
            friend EventQueue;

        private:
            EventWorker* Worker = nullptr;

        public:
            void* Data = nullptr;
            UInt64 Hash = 0;
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
            UInt64 Type = 0;
            EventArgs Args;
            BaseCallback Callback;
        };

        struct THAWK_OUT EventTimer
        {
            BaseCallback Callback;
            EventArgs Args;
            UInt64 Timeout = 0;
            UInt64 Id = 0;
            Int64 Time = 0;
        };

        struct THAWK_OUT EventListener
        {
            UInt64 Type = 0;
            BaseCallback Callback;
        };

        struct THAWK_OUT Resource
        {
            UInt64 Size = 0;
            Int64 LastModified = 0;
            Int64 CreationTime = 0;
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
            UInt64 Length = 0;
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
            DateTime FromNanoseconds(UInt64 Value);
            DateTime FromMicroseconds(UInt64 Value);
            DateTime FromMilliseconds(UInt64 Value);
            DateTime FromSeconds(UInt64 Value);
            DateTime FromMinutes(UInt64 Value);
            DateTime FromHours(UInt64 Value);
            DateTime FromDays(UInt64 Value);
            DateTime FromWeeks(UInt64 Value);
            DateTime FromMonths(UInt64 Value);
            DateTime FromYears(UInt64 Value);
            DateTime operator +(const DateTime& Right);
            DateTime operator -(const DateTime& Right);
            DateTime& SetDateSeconds(UInt64 Value, bool NoFlush = false);
            DateTime& SetDateMinutes(UInt64 Value, bool NoFlush = false);
            DateTime& SetDateHours(UInt64 Value, bool NoFlush = false);
            DateTime& SetDateDay(UInt64 Value, bool NoFlush = false);
            DateTime& SetDateWeek(UInt64 Value, bool NoFlush = false);
            DateTime& SetDateMonth(UInt64 Value, bool NoFlush = false);
            DateTime& SetDateYear(UInt64 Value, bool NoFlush = false);
            UInt64 Nanoseconds();
            UInt64 Microseconds();
            UInt64 Milliseconds();
            UInt64 Seconds();
            UInt64 Minutes();
            UInt64 Hours();
            UInt64 Days();
            UInt64 Weeks();
            UInt64 Months();
            UInt64 Years();

        private:
            void Rebuild();

        public:
            static std::string GetGMTBasedString(Int64 TimeStamp);
            static bool TimeFormatGMT(char* Buffer, UInt64 Length, Int64 Time);
            static bool TimeFormatLCL(char* Buffer, UInt64 Length, Int64 Time);
            static Int64 ReadGMTBasedString(const char* Date);
        };

        struct THAWK_OUT Stroke
        {
        public:
            struct Settle
            {
                UInt64 Start;
                UInt64 End;
                bool Found;
            };

        private:
            std::string* L;
            bool Safe;

        public:
            Stroke();
            Stroke(int Value);
            Stroke(unsigned int Value);
            Stroke(Int64 Value);
            Stroke(UInt64 Value);
            Stroke(float Value);
            Stroke(Float64 Value);
            Stroke(LFloat64 Value);
            Stroke(const std::string& Buffer);
            Stroke(std::string* Buffer);
            Stroke(const std::string* Buffer);
            Stroke(const char* Buffer);
            Stroke(const char* Buffer, Int64 Length);
            Stroke(const Stroke& Value);
            ~Stroke();
            Stroke& EscapePrint();
            Stroke& Reserve(UInt64 Count = 1);
            Stroke& Resize(UInt64 Count);
            Stroke& Resize(UInt64 Count, char Char);
            Stroke& Clear();
            Stroke& ToUtf8();
            Stroke& ToUpper();
            Stroke& ToLower();
            Stroke& Clip(UInt64 Length);
            Stroke& ReplaceOf(const char* Chars, const char* To, UInt64 Start = 0U);
            Stroke& Replace(const std::string& From, const std::string& To, UInt64 Start = 0U);
            Stroke& Replace(const char* From, const char* To, UInt64 Start = 0U);
            Stroke& Replace(const char& From, const char& To, UInt64 Position = 0U);
            Stroke& Replace(const char& From, const char& To, UInt64 Position, UInt64 Count);
            Stroke& ReplacePart(UInt64 Start, UInt64 End, const std::string& Value);
            Stroke& ReplacePart(UInt64 Start, UInt64 End, const char* Value);
            Stroke& RemovePart(UInt64 Start, UInt64 End);
            Stroke& Reverse();
            Stroke& Reverse(UInt64 Start, UInt64 End);
            Stroke& Substring(UInt64 Start);
            Stroke& Substring(UInt64 Start, UInt64 Count);
            Stroke& Substring(const Stroke::Settle& Result);
            Stroke& Splice(UInt64 Start, UInt64 End);
            Stroke& Trim();
            Stroke& Fill(const char& Char);
            Stroke& Fill(const char& Char, UInt64 Count);
            Stroke& Fill(const char& Char, UInt64 Start, UInt64 Count);
            Stroke& Assign(const char* Raw);
            Stroke& Assign(const char* Raw, UInt64 Length);
            Stroke& Assign(const std::string& Raw);
            Stroke& Assign(const std::string& Raw, UInt64 Start, UInt64 Count);
            Stroke& Assign(const char* Raw, UInt64 Start, UInt64 Count);
            Stroke& Append(const char* Raw);
            Stroke& Append(const char& Char);
            Stroke& Append(const char& Char, UInt64 Count);
            Stroke& Append(const std::string& Raw);
            Stroke& Append(const char* Raw, UInt64 Count);
            Stroke& Append(const char* Raw, UInt64 Start, UInt64 Count);
            Stroke& Append(const std::string& Raw, UInt64 Start, UInt64 Count);
            Stroke& fAppend(const char* Format, ...);
            Stroke& Insert(const std::string& Raw, UInt64 Position);
            Stroke& Insert(const std::string& Raw, UInt64 Position, UInt64 Start, UInt64 Count);
            Stroke& Insert(const std::string& Raw, UInt64 Position, UInt64 Count);
            Stroke& Insert(const char& Char, UInt64 Position, UInt64 Count);
            Stroke& Insert(const char& Char, UInt64 Position);
            Stroke& Erase(UInt64 Position);
            Stroke& Erase(UInt64 Position, UInt64 Count);
            Stroke& EraseOffsets(UInt64 Start, UInt64 End);
            Stroke& Path(const std::string& Net, const std::string& Dir);
            Stroke::Settle ReverseFind(const std::string& Needle, UInt64 Offset = 0U) const;
            Stroke::Settle ReverseFind(const char* Needle, UInt64 Offset = 0U) const;
            Stroke::Settle ReverseFind(const char& Needle, UInt64 Offset = 0U) const;
            Stroke::Settle ReverseFindUnescaped(const char& Needle, UInt64 Offset = 0U) const;
            Stroke::Settle ReverseFindOf(const std::string& Needle, UInt64 Offset = 0U) const;
            Stroke::Settle ReverseFindOf(const char* Needle, UInt64 Offset = 0U) const;
            Stroke::Settle Find(const std::string& Needle, UInt64 Offset = 0U) const;
            Stroke::Settle Find(const char* Needle, UInt64 Offset = 0U) const;
            Stroke::Settle Find(const char& Needle, UInt64 Offset = 0U) const;
            Stroke::Settle FindUnescaped(const char& Needle, UInt64 Offset = 0U) const;
            Stroke::Settle FindOf(const std::string& Needle, UInt64 Offset = 0U) const;
            Stroke::Settle FindOf(const char* Needle, UInt64 Offset = 0U) const;
            bool StartsWith(const std::string& Value, UInt64 Offset = 0U) const;
            bool StartsWith(const char* Value, UInt64 Offset = 0U) const;
            bool StartsOf(const char* Value, UInt64 Offset = 0U) const;
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
            Int64 ToInt64() const;
            Float64 ToFloat64() const;
            LFloat64 ToLFloat64() const;
            UInt64 ToUInt64() const;
            UInt64 Size() const;
            UInt64 Capacity() const;
            char* Value() const;
            const char* Get() const;
            std::string& R();
            std::basic_string<wchar_t> ToUnicode() const;
            std::vector<std::string> Split(const std::string& With, UInt64 Start = 0U) const;
            std::vector<std::string> Split(char With, UInt64 Start = 0U) const;
            std::vector<std::string> SplitOf(const char* With, UInt64 Start = 0U) const;

        public:
            static bool IsDigit(char Char);
            static int CaseCompare(const char* Value1, const char* Value2);
            static int Match(const char* Pattern, const char* Text);
            static int Match(const char* Pattern, UInt64 Length, const char* Text);
        };

        struct THAWK_OUT TickTimer
        {
        private:
            Float64 Time;

        public:
            Float64 Delay;

        public:
            TickTimer();
            bool OnTickEvent(Float64 ElapsedTime);
            Float64 GetTime();
        };

        template <class T>
        class Pool
        {
        public:
            typedef T* Iterator;

        protected:
            UInt64 Count, Volume;
            T* Data;

        public:
            Pool() : Count(0), Volume(0), Data(nullptr)
            {
                Reserve(1);
            }
            Pool(UInt64 _Size, UInt64 _Capacity, T* _Data) : Count(_Size), Volume(_Capacity), Data(_Data)
            {
                if (!Data && Volume > 0)
                    Reserve(Volume);
                else if (!Data || !Volume)
                    Reserve(1);
            }
            Pool(UInt64 _Capacity) : Count(0), Volume(0), Data(nullptr)
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
                UInt64 _Size = Raw.Count;
                Raw.Count = Count;
                Count = _Size;

                UInt64 _Capacity = Raw.Volume;
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
            void Reserve(UInt64 NewCount)
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
            void Resize(UInt64 NewSize)
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
            Iterator AddUnique(const T& Ref)
            {
                Iterator It = std::find(Data, Data + Count, Ref);
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
                while ((It = std::find(Data, Data + Count, Value)) != End())
                    RemoveAt(It);

                return It;
            }
            Iterator At(UInt64 Index) const
            {
                if (Index < 0 || Index >= Count)
                    return End();

                return Data + Index;
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
            T& operator [](UInt64 Index) const
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
            UInt64 Size() const
            {
                return Count;
            }
            UInt64 Capacity() const
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
            UInt64 SizeOf(Iterator A)
            {
                if (!std::is_pointer<T>::value)
                    return sizeof(T);

                return sizeof(UInt64);
            }
            UInt64 IncreaseCapacity(UInt64 NewSize)
            {
                UInt64 Alpha = Volume ? (Volume + Volume / 2) : 8;
                return Alpha > NewSize ? Alpha : NewSize;
            }
        };

        class THAWK_OUT LT
        {
            friend class Object;

        private:
#ifndef NDEBUG
            static UInt64 Memory;
            static std::unordered_map<void*, UInt64>* Objects;
            static std::mutex* Safe;
#endif
            static std::function<void(const char*, int)> Callback;
            static bool Enabled;

        public:
            static void AttachCallback(const std::function<void(const char*, int)>& Callback);
            static void AttachStream();
            static void DetachCallback();
            static void DetachStream();
            static void Inform(int Level, const char* Source, const char* Format, ...);
            static void* GetPtr(void* Ptr);
            static UInt64 GetSize(void* Ptr);
            static UInt64 GetCount();
            static UInt64 GetMemory();
            static void Free(void* Ptr);
            static void* Alloc(UInt64 Size);
        };

        class THAWK_OUT Object
        {
        public:
            Object();
            virtual ~Object() = default;
            void* operator new(size_t Size);
            void operator delete(void* Data);

        public:
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
            Float64 Time;
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
            Float64 GetCapturedTime();
            std::string Read(UInt64 Size);

        public:
            static Console*& Get();
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
            Float64 TimeIncrement;
            Float64 TickCounter;
            Float64 FrameCount;
            Float64 CapturedTime;
            Float64 DeltaTimeLimit;
            Float64 TimeStepLimit;
            void* TimeLimit;
            void* Frequency;
            void* PastTime;

        public:
            Float64 FrameRelation;
            Float64 FrameLimit;

        public:
            Timer();
            virtual ~Timer() override;
            void SetStepLimitation(Float64 MaxFrames, Float64 MinFrames);
            void Synchronize();
            void CaptureTime();
            void Sleep(UInt64 MilliSecs);
            Float64 GetTimeIncrement();
            Float64 GetTickCounter();
            Float64 GetFrameCount();
            Float64 GetElapsedTime();
            Float64 GetCapturedTime();
            Float64 GetDeltaTime();
            Float64 GetTimeStep();
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
            bool Seek(FileSeek Mode, Int64 Offset);
            bool Move(Int64 Offset);
            int Error();
            int Flush();
            int Fd();
            unsigned char Get();
            unsigned char Put(unsigned char Value);
            UInt64 WriteAny(const char* Format, ...);
            UInt64 Write(const char* Buffer, UInt64 Length);
            UInt64 ReadAny(const char* Format, ...);
            UInt64 Read(char* Buffer, UInt64 Length);
            UInt64 Tell();
            UInt64 Size();
            std::string& Filename();
            FILE* Stream();
            void* StreamZ();
        };

        class THAWK_OUT FileLogger : public Object
        {
        private:
            std::string LastValue;
            UInt64 Offset;

        public:
            FileStream* Stream = nullptr;
            std::string Path, Name;

        public:
            FileLogger(const std::string& Root);
            virtual ~FileLogger() override;
            void Process(const std::function<bool(FileLogger*, const char*, Int64)>& Callback);
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
            UInt64 GetFiles();
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
            static bool Write(const char* Path, const char* Data, UInt64 Length);
            static bool Write(const char* Path, const std::string& Data);
            static bool RemoveFile(const char* Path);
            static bool RemoveDir(const char* Path);
            static bool Move(const char* From, const char* To);
            static bool StateResource(const std::string& Path, Resource* Resource);
            static bool ScanDir(const std::string& Path, std::vector<ResourceEntry>* Entries);
            static bool ConstructETag(char* Buffer, UInt64 Length, Resource* Resource);
            static bool ConstructETagManually(char* Buffer, UInt64 Length, Int64 LastModified, UInt64 ContentLength);
            static bool SpawnProcess(const std::string& Path, const std::vector<std::string>& Params, ChildProcess* Result);
            static bool FreeProcess(ChildProcess* Process);
            static bool AwaitProcess(ChildProcess* Process, int* ExitCode);
            static bool UnloadObject(void* Handle);
            static bool SendFile(FILE* Stream, socket_t Socket, Int64 Size);
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
            static unsigned char* ReadAllBytes(const char* Path, UInt64* ByteLength);
            static unsigned char* ReadAllBytes(FileStream* Stream, UInt64* ByteLength);
            static unsigned char* ReadByteChunk(FileStream* Stream, UInt64 Length);
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
            std::unordered_map<UInt64, EventListener*> Listeners;
            std::deque<EventBase*> Events;
            std::deque<EventBase*> Tasks;
            std::vector<EventTimer*> Timers;
            EventState State = EventState_Terminated;
            UInt64 Timer = 0, Idle = 1;
            int Synchronize = 0;

        public:
            void* UserData = nullptr;

        public:
            EventQueue();
            virtual ~EventQueue() override;
            void SetIdleTime(UInt64 IdleTime);
            void SetState(EventState NewState);
            bool Tick();
            bool Start(EventWorkflow Workflow, UInt64 TaskWorkers, UInt64 EventWorkers);
            bool Stop();
            bool Expire(UInt64 TimerId);
            EventState GetState();

        private:
            bool MixedLoop();
            bool DefaultLoop();
            bool TimerLoop();
            bool QueueEvent(EventWorker* Worker);
            bool QueueTask(EventWorker* Worker);
            bool QueueTimer(Int64 Time);
            bool AddEvent(EventBase* Value);
            bool AddTask(EventBase* Value);
            bool AddTimer(EventTimer* Value);
            bool AddListener(EventListener* Value);
            bool RemoveCallback(EventBase* Value);
            bool RemoveListener(UInt64 Value);
            bool RemoveAny(EventType Type, UInt64 Hash, const PullCallback& Callback, bool NoCall);
            Int64 Clock();

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
            bool Interval(T* Args, UInt64 Milliseconds, const BaseCallback& Callback, EventTimer** Event = nullptr)
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
            bool Timeout(T* Args, UInt64 Milliseconds, const BaseCallback& Callback, EventTimer** Event = nullptr)
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
            Int64 Low;
            Int64 Integer;
            Float64 Number;
            bool Boolean;
            bool Saved;

        public:
            Document();
            virtual ~Document() override;
            void Clear();
            void Save();
            Document* GetIndex(Int64 Index);
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
            Document* SetString(const std::string& Name, const char* Value, Int64 Size);
            Document* SetString(const std::string& Name, const std::string& Value);
            Document* SetInteger(const std::string& Name, Int64 Value);
            Document* SetNumber(const std::string& Name, Float64 Value);
            Document* SetDecimal(const std::string& Name, Int64 High, Int64 Low);
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
            Int64 Size();
            Int64 GetDecimal(const std::string& Name, Int64* Low);
            Int64 GetInteger(const std::string& Name);
            Float64 GetNumber(const std::string& Name);
            unsigned char* GetId(const std::string& Name);
            const char* GetString(const std::string& Name);
            std::string& GetStringBlob(const std::string& Name);
            std::string Serialize();
            Document* Find(const std::string& Name, bool Here = false);
            Document* FindPath(const std::string& Notation, bool Here = false);
            std::vector<Document*> FindCollection(const std::string& Name, bool Here = false);
            std::vector<Document*> FindCollectionPath(const std::string& Notation, bool Here = false);
            std::vector<Document*> GetAttributes();
            std::vector<Document*>* GetNodes();
            std::unordered_map<std::string, UInt64> CreateMapping();

        public:
            static std::string Serialize(Document* Value);
            static bool Deserialize(const std::string& Value, Document* Output);
            static bool WriteBIN(Document* Value, const NWriteCallback& Callback);
            static bool WriteXML(Document* Value, const NWriteCallback& Callback);
            static bool WriteJSON(Document* Value, const NWriteCallback& Callback);
            static Document* ReadBIN(const NReadCallback& Callback);
            static Document* ReadXML(Int64 Size, const NReadCallback& Callback);
            static Document* ReadJSON(Int64 Size, const NReadCallback& Callback);

        private:
            static void ProcessBINWrite(Document* Current, std::unordered_map<std::string, UInt64>* Map, const NWriteCallback& Callback);
            static bool ProcessBINRead(Document* Current, std::unordered_map<UInt64, std::string>* Map, const NReadCallback& Callback);
            static bool ProcessMAPRead(Document* Current, std::unordered_map<std::string, UInt64>* Map, UInt64& Index);
            static bool ProcessXMLRead(void* Base, Document* Current);
            static bool ProcessJSONRead(Document* Current);
        };

        THAWK_OUT Stroke Form(const char* Format, ...);
    }
}
#endif