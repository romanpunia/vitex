#ifndef VI_CORE_H
#define VI_CORE_H
#include "config.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <inttypes.h>
#include <limits>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <charconv>
#include <cstring>
#include <list>
#include <system_error>
#include <algorithm>
#ifdef VI_CXX20
#include <coroutine>
#ifndef NDEBUG
#include <source_location>
#endif
#endif

namespace Vitex
{
	namespace Core
	{
		struct ConcurrentAsyncQueue;

		struct ConcurrentSyncQueue;

		struct Decimal;

		struct Cocontext;

		class Console;

		class Costate;

		class Schema;

		class Stream;

		class ProcessStream;

		class Var;

		typedef uint64_t TaskId;

		enum
		{
			INVALID_TASK_ID = (TaskId)0,
			NUMSTR_SIZE = (size_t)32,
			CHUNK_SIZE = (size_t)2048,
			BLOB_SIZE = (size_t)8192,
			STACK_SIZE = (size_t)(512 * 1024)
		};

		enum class Timings : uint64_t
		{
			Atomic = 1,
			Pass = 5,
			Frame = 16,
			Mixed = 50,
			FileSystem = 50,
			Networking = 150,
			Intensive = 350,
			Hangup = 5000,
			Infinite = 0
		};

		enum class Deferred : uint8_t
		{
			Pending = 0,
			Waiting = 1,
			Ready = 2
		};

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
			Binary,
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

		enum class Coexecution
		{
			Active,
			Suspended,
			Resumable,
			Finished
		};

		enum class Difficulty
		{
			Async,
			Sync,
			Timeout,
			Count
		};

		enum class LogLevel
		{
			Error = 1,
			Warning = 2,
			Info = 3,
			Debug = 4,
			Trace = 5
		};

		enum class LogOption
		{
			Active = 1 << 0,
			Pretty = 1 << 1,
			Async = 1 << 2,
			Dated = 1 << 3,
			ReportSysErrors = 1 << 4
		};

		enum class Optional : char
		{
			None = 0,
			Value = 1
		};

		enum class Expectation : char
		{
			Error = -1,
			Met = 1
		};

		enum class Signal
		{
			SIG_INT,
			SIG_ILL,
			SIG_FPE,
			SIG_SEGV,
			SIG_TERM,
			SIG_BREAK,
			SIG_ABRT,
			SIG_BUS,
			SIG_ALRM,
			SIG_HUP,
			SIG_QUIT,
			SIG_TRAP,
			SIG_CONT,
			SIG_STOP,
			SIG_PIPE,
			SIG_CHLD,
			SIG_USR1,
			SIG_USR2
		};

		enum class AccessOption : uint64_t
		{
			Mem = (1 << 0),
			Fs = (1 << 1),
			Gz = (1 << 2),
			Net = (1 << 3),
			Lib = (1 << 4),
			Http = (1 << 5),
			Https = (1 << 6),
			Shell = (1 << 7),
			Env = (1 << 8),
			Addons = (1 << 9),
			All = (uint64_t)(Mem | Fs | Gz | Net | Lib | Http | Https | Shell | Env | Addons)
		};

		enum class ArgsFormat
		{
			KeyValue = (1 << 0),
			FlagValue = (1 << 1),
			Key = (1 << 2),
			Flag = (1 << 3),
			StopIfNoMatch = (1 << 4)
		};

		enum class ParserError
		{
			NotSupported,
			BadVersion,
			BadDictionary,
			BadNameIndex,
			BadName,
			BadKeyName,
			BadKeyType,
			BadValue,
			BadString,
			BadInteger,
			BadDouble,
			BadBoolean,
			XMLOutOfMemory,
			XMLInternalError,
			XMLUnrecognizedTag,
			XMLBadPi,
			XMLBadComment,
			XMLBadCData,
			XMLBadDocType,
			XMLBadPCData,
			XMLBadStartElement,
			XMLBadAttribute,
			XMLBadEndElement,
			XMLEndElementMismatch,
			XMLAppendInvalidRoot,
			XMLNoDocumentElement,
			JSONDocumentEmpty,
			JSONDocumentRootNotSingular,
			JSONValueInvalid,
			JSONObjectMissName,
			JSONObjectMissColon,
			JSONObjectMissCommaOrCurlyBracket,
			JSONArrayMissCommaOrSquareBracket,
			JSONStringUnicodeEscapeInvalidHex,
			JSONStringUnicodeSurrogateInvalid,
			JSONStringEscapeInvalid,
			JSONStringMissQuotationMark,
			JSONStringInvalidEncoding,
			JSONNumberTooBig,
			JSONNumberMissFraction,
			JSONNumberMissExponent,
			JSONTermination,
			JSONUnspecificSyntaxError
		};

		inline AccessOption operator |(AccessOption A, AccessOption B)
		{
			return static_cast<AccessOption>(static_cast<uint64_t>(A) | static_cast<uint64_t>(B));
		}

		template <typename T, typename = void>
		struct IsIterable : std::false_type { };

		template <typename T>
		struct IsIterable<T, std::void_t<decltype(std::begin(std::declval<T&>())), decltype(std::end(std::declval<T&>()))>> : std::true_type { };

		template <typename T>
		struct IsAddRefer
		{
			template <typename U>
			static constexpr std::conditional_t<false, decltype(std::declval<U>().AddRef()), std::true_type> Signature(int);

			template <typename U>
			static constexpr std::false_type Signature(char);

			static constexpr bool value = decltype(Signature<T>(0))::value;
		};

		template <typename T>
		struct IsReleaser
		{
			template <typename U>
			static constexpr std::conditional_t<false, decltype(std::declval<U>().Release()), std::true_type> Signature(int);

			template <typename U>
			static constexpr std::false_type Signature(char);

			static constexpr bool value = decltype(Signature<T>(0))::value;
		};

		template <typename T>
		using Unique = T*;

		struct Singletonish { };

		struct VI_OUT MemoryLocation
		{
			const char* Source;
			const char* Function;
			const char* TypeName;
			int Line;

			MemoryLocation();
			MemoryLocation(const char* NewSource, const char* NewFunction, const char* NewTypeName, int NewLine);
		};

		class VI_OUT_TS GlobalAllocator
		{
		public:
			virtual ~GlobalAllocator() = default;
			virtual Unique<void> Allocate(size_t Size) noexcept = 0;
			virtual Unique<void> Allocate(MemoryLocation&& Location, size_t Size) noexcept = 0;
			virtual void Transfer(Unique<void> Address, size_t Size) noexcept = 0;
			virtual void Transfer(Unique<void> Address, MemoryLocation&& Location, size_t Size) noexcept = 0;
			virtual void Free(Unique<void> Address) noexcept = 0;
			virtual void Watch(MemoryLocation&& Location, void* Address) noexcept = 0;
			virtual void Unwatch(void* Address) noexcept = 0;
			virtual void Finalize() noexcept = 0;
			virtual bool IsValid(void* Address) noexcept = 0;
			virtual bool IsFinalizable() noexcept = 0;
		};

		class VI_OUT LocalAllocator
		{
		public:
			virtual ~LocalAllocator() = default;
			virtual Unique<void> Allocate(size_t Size) noexcept = 0;
			virtual void Free(Unique<void> Address) noexcept = 0;
			virtual void Reset() noexcept = 0;
			virtual bool IsValid(void* Address) noexcept = 0;
		};

		class VI_OUT_TS Memory final : public Singletonish
		{
		private:
			struct State
			{
				std::unordered_map<void*, std::pair<MemoryLocation, size_t>> Allocations;
				std::mutex Mutex;
			};

		private:
			static GlobalAllocator* Global;
			static State* Context;

		public:
			static Unique<void> DefaultAllocate(size_t Size) noexcept;
			static Unique<void> TracingAllocate(size_t Size, MemoryLocation&& Location) noexcept;
			static void DefaultDeallocate(Unique<void> Address) noexcept;
			static void Watch(void* Address, MemoryLocation&& Location) noexcept;
			static void Unwatch(void* Address) noexcept;
			static void Cleanup() noexcept;
			static void SetGlobalAllocator(GlobalAllocator* NewAllocator) noexcept;
			static void SetLocalAllocator(LocalAllocator* NewAllocator) noexcept;
			static bool IsValidAddress(void* Address) noexcept;
			static GlobalAllocator* GetGlobalAllocator() noexcept;
			static LocalAllocator* GetLocalAllocator() noexcept;

		public:
			template <typename T>
			static inline void Delete(T*& Value)
			{
				if (Value != nullptr)
				{
					Value->~T();
					DefaultDeallocate(static_cast<void*>(Value));
					Value = nullptr;
				}
			}
			template <typename T>
			static inline void Delete(T* const& Value)
			{
				if (Value != nullptr)
				{
					Value->~T();
					DefaultDeallocate(static_cast<void*>(Value));
				}
			}
			template <typename T>
			static inline void Deallocate(T*& Value)
			{
				if (Value != nullptr)
				{
					DefaultDeallocate(static_cast<void*>(Value));
					Value = nullptr;
				}
			}
			template <typename T>
			static inline void Deallocate(T* const& Value)
			{
				DefaultDeallocate(static_cast<void*>(Value));
			}
			template <typename T>
			static inline void Release(T*& Value)
			{
				if (Value != nullptr)
				{
					Value->Release();
					Value = nullptr;
				}
			}
			template <typename T>
			static inline void Release(T* const& Value)
			{
				if (Value != nullptr)
					Value->Release();
			}

		public:
#ifndef NDEBUG
#ifdef VI_CXX20
			template <typename T>
			static T* Allocate(size_t Size, const std::source_location& Location = std::source_location::current())
			{
				return static_cast<T*>(TracingAllocate(Size, MemoryLocation(Location.file_name(), Location.function_name(), typeid(T).name(), Location.line())));
			}
			template <typename T>
			static T* New(const std::source_location& Location = std::source_location::current())
			{
				return new(Allocate<T>(sizeof(T), Location)) T();
			}
			template <typename T, typename P1>
			static T* New(P1&& Ph1, const std::source_location& Location = std::source_location::current())
			{
				return new(Allocate<T>(sizeof(T), Location)) T(std::forward<P1>(Ph1));
			}
			template <typename T, typename P1, typename P2>
			static T* New(P1&& Ph1, P2&& Ph2, const std::source_location& Location = std::source_location::current())
			{
				return new(Allocate<T>(sizeof(T), Location)) T(std::forward<P1>(Ph1), std::forward<P2>(Ph2));
			}
			template <typename T, typename P1, typename P2, typename P3>
			static T* New(P1&& Ph1, P2&& Ph2, P3&& Ph3, const std::source_location& Location = std::source_location::current())
			{
				return new(Allocate<T>(sizeof(T), Location)) T(std::forward<P1>(Ph1), std::forward<P2>(Ph2), std::forward<P3>(Ph3));
			}
			template <typename T, typename P1, typename P2, typename P3, typename P4>
			static T* New(P1&& Ph1, P2&& Ph2, P3&& Ph3, P4&& Ph4, const std::source_location& Location = std::source_location::current())
			{
				return new(Allocate<T>(sizeof(T), Location)) T(std::forward<P1>(Ph1), std::forward<P2>(Ph2), std::forward<P3>(Ph3), std::forward<P4>(Ph4));
			}
			template <typename T, typename P1, typename P2, typename P3, typename P4, typename P5>
			static T* New(P1&& Ph1, P2&& Ph2, P3&& Ph3, P4&& Ph4, P5&& Ph5, const std::source_location& Location = std::source_location::current())
			{
				return new(Allocate<T>(sizeof(T), Location)) T(std::forward<P1>(Ph1), std::forward<P2>(Ph2), std::forward<P3>(Ph3), std::forward<P4>(Ph4), std::forward<P5>(Ph5));
			}
			template <typename T, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
			static T* New(P1&& Ph1, P2&& Ph2, P3&& Ph3, P4&& Ph4, P5&& Ph5, P6&& Ph6, const std::source_location& Location = std::source_location::current())
			{
				return new(Allocate<T>(sizeof(T), Location)) T(std::forward<P1>(Ph1), std::forward<P2>(Ph2), std::forward<P3>(Ph3), std::forward<P4>(Ph4), std::forward<P5>(Ph5), std::forward<P6>(Ph6));
			}
			template <typename T, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
			static T* New(P1&& Ph1, P2&& Ph2, P3&& Ph3, P4&& Ph4, P5&& Ph5, P6&& Ph6, P7&& Ph7, const std::source_location& Location = std::source_location::current())
			{
				return new(Allocate<T>(sizeof(T), Location)) T(std::forward<P1>(Ph1), std::forward<P2>(Ph2), std::forward<P3>(Ph3), std::forward<P4>(Ph4), std::forward<P5>(Ph5), std::forward<P6>(Ph6), std::forward<P7>(Ph7));
			}
#else
			template <typename T>
			static T* Allocate(size_t Size)
			{
				return static_cast<T*>(TracingAllocate(Size, MemoryLocation(__FILE__, __func__, typeid(T).name(), __LINE__)));
			}
			template <typename T, typename... Args>
			static T* New(Args&&... Values)
			{
				void* Address = static_cast<T*>(TracingAllocate(sizeof(T), MemoryLocation(__FILE__, __func__, typeid(T).name(), __LINE__)));
				return new(Address) T(std::forward<Args>(Values)...);
			}
#endif
#else
			template <typename T>
			static T* Allocate(size_t Size)
			{
				return static_cast<T*>(DefaultAllocate(Size));
			}
			template <typename T, typename... Args>
			static T* New(Args&&... Values)
			{
				return new(Allocate<T>(sizeof(T))) T(std::forward<Args>(Values)...);
			}
#endif
		};

		template <typename T>
		class StandardAllocator
		{
		public:
			typedef T value_type;

		public:
			template <typename U>
			struct rebind
			{
				typedef StandardAllocator<U> other;
			};

		public:
			StandardAllocator() = default;
			~StandardAllocator() = default;
			StandardAllocator(const StandardAllocator&) = default;
			value_type* allocate(size_t Count)
			{
				return Memory::Allocate<T>(Count * sizeof(T));
			}
			value_type* allocate(size_t Count, const void*)
			{
				return Memory::Allocate<T>(Count * sizeof(T));
			}
			void deallocate(value_type* Address, size_t Count)
			{
				Memory::Deallocate<value_type>(Address);
			}
			size_t max_size() const
			{
				return std::numeric_limits<size_t>::max() / sizeof(T);
			}
			bool operator== (const StandardAllocator&)
			{
				return true;
			}
			bool operator!=(const StandardAllocator&)
			{
				return false;
			}

		public:
			template <typename U>
			StandardAllocator(const StandardAllocator<U>&)
			{
			}
		};

		template <class A, class B>
		bool operator== (const StandardAllocator<A>&, const StandardAllocator<B>&) noexcept
		{
			return true;
		}

		template <typename T>
		struct AllocationType
		{
#ifdef VI_ALLOCATOR
			using type = StandardAllocator<T>;
#else
			using type = std::allocator<T>;
#endif
		};

		template <typename T, T OffsetBasis, T Prime>
		struct FNV1AHash
		{
			static_assert(std::is_unsigned<T>::value, "Q needs to be uint32_teger");

			inline T operator()(const void* Address, size_t Size) const noexcept
			{
				const auto Data = static_cast<const uint8_t*>(Address);
				auto State = OffsetBasis;
				for (size_t i = 0; i < Size; ++i)
					State = (State ^ (size_t)Data[i]) * Prime;
				return State;
			}
		};

		template <size_t Bits>
		struct FNV1ABits;

		template <>
		struct FNV1ABits<32> { using type = FNV1AHash<uint32_t, UINT32_C(2166136261), UINT32_C(16777619)>; };

		template <>
		struct FNV1ABits<64> { using type = FNV1AHash<uint64_t, UINT64_C(14695981039346656037), UINT64_C(1099511628211)>; };

		template <size_t Bits>
		using FNV1A = typename FNV1ABits<Bits>::type;

		template <typename T>
		struct KeyHasher
		{
			typedef float argument_type;
			typedef size_t result_type;

			inline result_type operator()(const T& Value) const noexcept
			{
				return std::hash<T>()(Value);
			}
		};

		template <typename T>
		struct EqualTo
		{
			typedef T first_argument_type;
			typedef T second_argument_type;
			typedef bool result_type;

			inline result_type operator()(const T& Left, const T& Right) const noexcept
			{
				return std::equal_to<T>()(Left, Right);
			}
		};

		using String = std::basic_string<std::string::value_type, std::string::traits_type, typename AllocationType<typename std::string::value_type>::type>;
		using WideString = std::basic_string<std::wstring::value_type, std::wstring::traits_type, typename AllocationType<typename std::wstring::value_type>::type>;
		using StringStream = std::basic_stringstream<std::string::value_type, std::string::traits_type, typename AllocationType<typename std::string::value_type>::type>;
		using WideStringStream = std::basic_stringstream<std::wstring::value_type, std::wstring::traits_type, typename AllocationType<typename std::wstring::value_type>::type>;

		template <>
		struct EqualTo<String>
		{
			typedef String first_argument_type;
			typedef String second_argument_type;
			typedef bool result_type;
			using is_transparent = void;

			inline result_type operator()(const String& Left, const String& Right) const noexcept
			{
				return Left == Right;
			}
			inline result_type operator()(const String& Left, const char* Right) const noexcept
			{
				return Left.compare(Right) == 0;
			}
			inline result_type operator()(const String& Left, const std::string_view& Right) const noexcept
			{
				return Left == Right;
			}
			inline result_type operator()(const char* Left, const String& Right) const noexcept
			{
				return Right.compare(Left) == 0;
			}
			inline result_type operator()(const std::string_view& Left, const String& Right) const noexcept
			{
				return Left == Right;
			}
		};

		template <>
		struct KeyHasher<String>
		{
			typedef float argument_type;
			typedef size_t result_type;
			using is_transparent = void;

			inline result_type operator()(const char* Value) const noexcept
			{
				return FNV1A<8 * sizeof(size_t)>()(Value, strlen(Value));
			}
			inline result_type operator()(const std::string_view& Value) const noexcept
			{
				return FNV1A<8 * sizeof(size_t)>()(Value.data(), Value.size());
			}
			inline result_type operator()(const String& Value) const noexcept
			{
				return FNV1A<8 * sizeof(size_t)>()(Value.data(), Value.size());
			}
		};

		template <>
		struct KeyHasher<WideString>
		{
			typedef float argument_type;
			typedef size_t result_type;

			inline result_type operator()(const WideString& Value) const noexcept
			{
				return FNV1A<8 * sizeof(std::size_t)>()(Value.c_str(), Value.size());
			}
			inline result_type operator()(const std::wstring_view& Value) const noexcept
			{
				return FNV1A<8 * sizeof(std::size_t)>()(Value.data(), Value.size());
			}
		};

		template <typename T>
		using Vector = std::vector<T, typename AllocationType<T>::type>;

		template <typename T>
		using LinkedList = std::list<T, typename AllocationType<T>::type>;

		template <typename T>
		using SingleQueue = std::queue<T, std::deque<T, typename AllocationType<T>::type>>;

		template <typename T>
		using DoubleQueue = std::deque<T, typename AllocationType<T>::type>;

		template <typename K, typename V, typename Comparator = typename std::map<K, V>::key_compare>
		using OrderedMap = std::map<K, V, Comparator, typename AllocationType<typename std::map<K, V>::value_type>::type>;

		template <typename K, typename Hash = KeyHasher<K>, typename KeyEqual = EqualTo<K>>
		using UnorderedSet = std::unordered_set<K, Hash, KeyEqual, typename AllocationType<typename std::unordered_set<K>::value_type>::type>;

		template <typename K, typename V, typename Hash = KeyHasher<K>, typename KeyEqual = EqualTo<K>>
		using UnorderedMap = std::unordered_map<K, V, Hash, KeyEqual, typename AllocationType<typename std::unordered_map<K, V>::value_type>::type>;

		template <typename K, typename V, typename Hash = KeyHasher<K>, typename KeyEqual = EqualTo<K>>
		using UnorderedMultiMap = std::unordered_multimap<K, V, Hash, KeyEqual, typename AllocationType<typename std::unordered_multimap<K, V>::value_type>::type>;

		typedef std::function<void()> TaskCallback;
		typedef std::function<bool(const std::string_view&)> ProcessCallback;
		typedef std::function<String(const std::string_view&)> SchemaNameCallback;
		typedef std::function<void(VarForm, const std::string_view&)> SchemaWriteCallback;
		typedef std::function<bool(uint8_t*, size_t)> SchemaReadCallback;
		typedef std::function<bool()> ActivityCallback;
		typedef std::function<void(TaskCallback&&)> SpawnerCallback;
		typedef void(*SignalCallback)(int);

		namespace Allocators
		{
			class VI_OUT_TS DebugAllocator final : public GlobalAllocator
			{
			public:
				struct VI_OUT_TS TracingInfo
				{
					std::thread::id Thread;
					std::string TypeName;
					MemoryLocation Location;
					time_t Time;
					size_t Size;
					bool Active;
					bool Static;

					TracingInfo();
					TracingInfo(const char* NewTypeName, MemoryLocation&& NewLocation, time_t NewTime, size_t NewSize, bool IsActive, bool IsStatic);
				};

			private:
				std::unordered_map<void*, TracingInfo> Blocks;
				std::unordered_map<void*, TracingInfo> Watchers;
				std::recursive_mutex Mutex;

			public:
				~DebugAllocator() override = default;
				Unique<void> Allocate(size_t Size) noexcept override;
				Unique<void> Allocate(MemoryLocation&& Origin, size_t Size) noexcept override;
				void Free(Unique<void> Address) noexcept override;
				void Transfer(Unique<void> Address, size_t Size) noexcept override;
				void Transfer(Unique<void> Address, MemoryLocation&& Origin, size_t Size) noexcept override;
				void Watch(MemoryLocation&& Origin, void* Address) noexcept override;
				void Unwatch(void* Address) noexcept override;
				void Finalize() noexcept override;
				bool IsValid(void* Address) noexcept override;
				bool IsFinalizable() noexcept override;
				bool Dump(void* Address);
				bool FindBlock(void* Address, TracingInfo* Output);
				const std::unordered_map<void*, TracingInfo>& GetBlocks() const;
				const std::unordered_map<void*, TracingInfo>& GetWatchers() const;
			};

			class VI_OUT_TS DefaultAllocator final : public GlobalAllocator
			{
			public:
				~DefaultAllocator() override = default;
				Unique<void> Allocate(size_t Size) noexcept override;
				Unique<void> Allocate(MemoryLocation&& Origin, size_t Size) noexcept override;
				void Free(Unique<void> Address) noexcept override;
				void Transfer(Unique<void> Address, size_t Size) noexcept override;
				void Transfer(Unique<void> Address, MemoryLocation&& Origin, size_t Size) noexcept override;
				void Watch(MemoryLocation&& Origin, void* Address) noexcept override;
				void Unwatch(void* Address) noexcept override;
				void Finalize() noexcept override;
				bool IsValid(void* Address) noexcept override;
				bool IsFinalizable() noexcept override;
			};

			class VI_OUT_TS CachedAllocator final : public GlobalAllocator
			{
			private:
				struct PageCache;

				using PageGroup = std::vector<PageCache*>;

				struct PageAddress
				{
					PageCache* Cache;
					void* Address;
				};

				struct PageCache
				{
					std::vector<PageAddress*> Addresses;
					PageGroup& Page;
					int64_t Timing;
					size_t Capacity;

					inline PageCache(PageGroup& NewPage, int64_t Time, size_t Size) : Page(NewPage), Timing(Time), Capacity(Size)
					{
						Addresses.resize(Capacity);
					}
					~PageCache() = default;
				};

			private:
				std::unordered_map<size_t, PageGroup> Pages;
				std::recursive_mutex Mutex;
				uint64_t MinimalLifeTime;
				double ElementsReducingFactor;
				size_t ElementsReducingBase;
				size_t ElementsPerAllocation;

			public:
				CachedAllocator(uint64_t MinimalLifeTimeMs = 2000, size_t MaxElementsPerAllocation = 1024, size_t ElementsReducingBaseBytes = 300, double ElementsReducingFactorRate = 5.0);
				~CachedAllocator() noexcept override;
				Unique<void> Allocate(size_t Size) noexcept override;
				Unique<void> Allocate(MemoryLocation&& Origin, size_t Size) noexcept override;
				void Free(Unique<void> Address) noexcept override;
				void Transfer(Unique<void> Address, size_t Size) noexcept override;
				void Transfer(Unique<void> Address, MemoryLocation&& Origin, size_t Size) noexcept override;
				void Watch(MemoryLocation&& Origin, void* Address) noexcept override;
				void Unwatch(void* Address) noexcept override;
				void Finalize() noexcept override;
				bool IsValid(void* Address) noexcept override;
				bool IsFinalizable() noexcept override;

			private:
				PageCache* GetPageCache(size_t Size);
				int64_t GetClock();
				size_t GetElementsCount(PageGroup& Page, size_t Size);
			};

			class VI_OUT LinearAllocator final : public LocalAllocator
			{
			private:
				struct Region
				{
					char* FreeAddress;
					char* BaseAddress;
					Region* UpperAddress;
					Region* LowerAddress;
					size_t Size;
				};

			private:
				Region* Top;
				Region* Bottom;
				size_t LatestSize;
				size_t Sizing;

			public:
				LinearAllocator(size_t Size);
				~LinearAllocator() noexcept override;
				Unique<void> Allocate(size_t Size) noexcept override;
				void Free(Unique<void> Address) noexcept override;
				void Reset() noexcept override;
				bool IsValid(void* Address) noexcept override;
				size_t GetLeftovers() const noexcept;

			private:
				void NextRegion(size_t Size) noexcept;
				void FlushRegions() noexcept;
			};

			class VI_OUT StackAllocator final : public LocalAllocator
			{
			private:
				struct Region
				{
					char* FreeAddress;
					char* BaseAddress;
					Region* UpperAddress;
					Region* LowerAddress;
					size_t Size;
				};

			private:
				Region* Top;
				Region* Bottom;
				size_t Sizing;

			public:
				StackAllocator(size_t Size);
				~StackAllocator() noexcept override;
				Unique<void> Allocate(size_t Size) noexcept override;
				void Free(Unique<void> Address) noexcept override;
				void Reset() noexcept override;
				bool IsValid(void* Address) noexcept override;
				size_t GetLeftovers() const noexcept;

			private:
				void NextRegion(size_t Size) noexcept;
				void FlushRegions() noexcept;
			};
		}

		class VI_OUT StackTrace
		{
		public:
			struct VI_OUT Frame
			{
				String Code;
				String Function;
				String File;
				uint32_t Line;
				uint32_t Column;
				void* Handle;
				bool Native;
			};

		public:
			typedef Vector<Frame> StackPtr;

		private:
			StackPtr Frames;

		public:
			StackTrace(size_t Skips = 0, size_t MaxDepth = 64);
			StackPtr::const_iterator begin() const;
			StackPtr::const_iterator end() const;
			StackPtr::const_reverse_iterator rbegin() const;
			StackPtr::const_reverse_iterator rend() const;
			explicit operator bool() const;
			const StackPtr& Range() const;
			bool Empty() const;
			size_t Size() const;
		};

		class VI_OUT_TS ErrorHandling final : public Singletonish
		{
		public:
			struct VI_OUT Details
			{
				struct
				{
					const char* File = nullptr;
					int Line = 0;
				} Origin;

				struct
				{
					LogLevel Level = LogLevel::Info;
					bool Fatal = false;
				} Type;

				struct
				{
					char Data[BLOB_SIZE] = { '\0' };
					char Date[64] = { '\0' };
					size_t Size = 0;
				} Message;
			};

			struct VI_OUT Tick
			{
				bool IsCounting;

				Tick(bool Active) noexcept;
				Tick(const Tick& Other) = delete;
				Tick(Tick&& Other) noexcept;
				~Tick() noexcept;
				Tick& operator =(const Tick& Other) = delete;
				Tick& operator =(Tick&& Other) noexcept;
			};

		private:
			struct State
			{
				std::function<void(Details&)> Callback;
				uint32_t Flags = (uint32_t)LogOption::Pretty;
			};

		private:
			static State* Context;

		public:
			static void Panic(int Line, const char* Source, const char* Function, const char* Condition, const char* Format, ...) noexcept;
			static void Assert(int Line, const char* Source, const char* Function, const char* Condition, const char* Format, ...) noexcept;
			static void Message(LogLevel Level, int Line, const char* Source, const char* Format, ...) noexcept;
			static void Pause() noexcept;
			static void Cleanup() noexcept;
			static void SetCallback(std::function<void(Details&)>&& Callback) noexcept;
			static void SetFlag(LogOption Option, bool Active) noexcept;
			static bool HasFlag(LogOption Option) noexcept;
			static bool HasCallback() noexcept;
			static String GetStackTrace(size_t Skips, size_t MaxFrames = 64) noexcept;
			static String GetMeasureTrace() noexcept;
			static Tick Measure(const char* File, const char* Function, int Line, uint64_t ThresholdMS) noexcept;
			static void MeasureLoop() noexcept;
			static std::string_view GetMessageType(const Details& Base) noexcept;
			static StdColor GetMessageColor(const Details& Base) noexcept;
			static String GetMessageText(const Details& Base) noexcept;

		private:
			static void Enqueue(Details&& Data) noexcept;
			static void Dispatch(Details& Data) noexcept;
			static void Colorify(Console* Terminal, Details& Data) noexcept;
		};

		class OptionUtils
		{
		public:
			template <typename Q>
			static typename std::enable_if<std::is_trivial<Q>::value, void>::type MoveBuffer(char* Dest, char* Src, size_t Size)
			{
				memcpy(Dest, Src, Size);
			}
			template <typename Q>
			static typename std::enable_if<!std::is_trivial<Q>::value, void>::type MoveBuffer(char* Dest, char* Src, size_t)
			{
				new(Dest) Q(std::move(*(Q*)Src));
			}
			template <typename Q>
			static typename std::enable_if<std::is_trivial<Q>::value, void>::type CopyBuffer(char* Dest, const char* Src, size_t Size)
			{
				memcpy(Dest, Src, Size);
			}
			template <typename Q>
			static typename std::enable_if<!std::is_trivial<Q>::value, void>::type CopyBuffer(char* Dest, const char* Src, size_t)
			{
				new(Dest) Q(*(Q*)Src);
			}
			template <typename Q>
			static typename std::enable_if<std::is_pointer<Q>::value, const Q>::type ToConstPointer(const char* Src)
			{
				Q& Reference = *(Q*)Src;
				return Reference;
			}
			template <typename Q>
			static typename std::enable_if<!std::is_pointer<Q>::value, const Q*>::type ToConstPointer(const char* Src)
			{
				Q& Reference = *(Q*)Src;
				return &Reference;
			}
			template <typename Q>
			static typename std::enable_if<std::is_pointer<Q>::value, Q>::type ToPointer(char* Src)
			{
				Q& Reference = *(Q*)Src;
				return Reference;
			}
			template <typename Q>
			static typename std::enable_if<!std::is_pointer<Q>::value, Q*>::type ToPointer(char* Src)
			{
				Q& Reference = *(Q*)Src;
				return &Reference;
			}
			template <typename Q>
			static typename std::enable_if<std::is_base_of<std::exception, Q>::value, String>::type ToErrorText(const char* Src, bool IsError)
			{
				return String(IsError ? ((Q*)Src)->what() : "accessing a none value");
			}
			template <typename Q>
			static typename std::enable_if<std::is_same<std::error_code, Q>::value, std::string>::type ToErrorText(const char* Src, bool IsError)
			{
				return IsError ? ((Q*)Src)->message() : std::string("accessing a none value");
			}
			template <typename Q>
			static typename std::enable_if<std::is_same<std::error_condition, Q>::value, std::string>::type ToErrorText(const char* Src, bool IsError)
			{
				return IsError ? ((Q*)Src)->message() : std::string("accessing a none value");
			}
			template <typename Q>
			static typename std::enable_if<std::is_same<std::string_view, Q>::value, String>::type ToErrorText(const char* Src, bool IsError)
			{
				return String(IsError ? *(Q*)Src : Q("accessing a none value"));
			}
			template <typename Q>
			static typename std::enable_if<std::is_same<std::string, Q>::value, std::string>::type ToErrorText(const char* Src, bool IsError)
			{
				return IsError ? *(Q*)Src : Q("accessing a none value");
			}
#ifdef VI_ALLOCATOR
			template <typename Q>
			static typename std::enable_if<std::is_same<String, Q>::value, String>::type ToErrorText(const char* Src, bool IsError)
			{
				return IsError ? *(Q*)Src : Q("accessing a none value");
			}
#endif
			template <typename Q>
			static typename std::enable_if<!std::is_base_of<std::exception, Q>::value && !std::is_same<std::error_code, Q>::value && !std::is_same<std::error_condition, Q>::value && !std::is_same<std::string, Q>::value && !std::is_same<String, Q>::value, String>::type ToErrorText(const char* Src, bool IsError)
			{
				return String(IsError ? "*unknown error value*" : "accessing a none value");
			}
		};

		class BasicException : public std::exception
		{
		protected:
			String Message;

		public:
			BasicException() = default;
			VI_OUT BasicException(const std::string_view& NewMessage) noexcept;
			VI_OUT BasicException(String&& NewMessage) noexcept;
			BasicException(const BasicException&) = default;
			BasicException(BasicException&&) = default;
			BasicException& operator= (const BasicException&) = default;
			BasicException& operator= (BasicException&&) = default;
			virtual ~BasicException() = default;
			virtual const char* type() const noexcept = 0;
			virtual VI_OUT const char* what() const noexcept;
			virtual VI_OUT const String& message() const& noexcept;
			virtual VI_OUT const String&& message() const&& noexcept;
			virtual VI_OUT String& message() & noexcept;
			virtual VI_OUT String&& message() && noexcept;
		};

		class ParserException final : public BasicException
		{
		private:
			ParserError Type;
			size_t Offset;

		public:
			VI_OUT ParserException(ParserError NewType);
			VI_OUT ParserException(ParserError NewType, size_t NewOffset);
			VI_OUT ParserException(ParserError NewType, size_t NewOffset, const std::string_view& NewMessage);
			VI_OUT const char* type() const noexcept override;
			VI_OUT ParserError status() const noexcept;
			VI_OUT size_t offset() const noexcept;
		};

		class SystemException : public BasicException
		{
		private:
			std::error_condition Error;

		public:
			VI_OUT SystemException();
			VI_OUT SystemException(const std::string_view& Message);
			VI_OUT SystemException(const std::string_view& Message, std::error_condition&& Condition);
			virtual VI_OUT const char* type() const noexcept override;
			virtual VI_OUT const std::error_condition& error() const& noexcept;
			virtual VI_OUT const std::error_condition&& error() const&& noexcept;
			virtual VI_OUT std::error_condition& error() & noexcept;
			virtual VI_OUT std::error_condition&& error() && noexcept;
		};

		template <typename T>
		class Bitmask
		{
			static_assert(std::is_integral<T>::value, "value should be of integral type to have bitmask applications");

		public:
			static T Mark(T Other)
			{
				return Other | (T)Highbit<T>();
			}
			static T Unmark(T Other)
			{
				return Other & (T)Lowbit<T>();
			}
			static T Value(T Other)
			{
				return Other & (T)Lowbit<T>();
			}
			static bool IsMarked(T Other)
			{
				return (bool)(Other & (T)Highbit<T>());
			}

		private:
			template <typename Q>
			static typename std::enable_if<sizeof(Q) == sizeof(uint64_t), uint64_t>::type Highbit()
			{
				return 0x8000000000000000;
			}
			template <typename Q>
			static typename std::enable_if < sizeof(Q) < sizeof(uint64_t), uint32_t > ::type Highbit()
			{
				return 0x80000000;
			}
			template <typename Q>
			static typename std::enable_if<sizeof(Q) == sizeof(uint64_t), uint64_t>::type Lowbit()
			{
				return 0x7FFFFFFFFFFFFFFF;
			}
			template <typename Q>
			static typename std::enable_if < sizeof(Q) < sizeof(uint64_t), uint32_t > ::type Lowbit()
			{
				return 0x7FFFFFFF;
			}
		};

		template <typename V>
		class Option
		{
			static_assert(!std::is_same<V, void>::value, "value type should not be void");

		private:
			alignas(V) char Value[sizeof(V)];
			int8_t Status;

		public:
			Option(Optional Type) : Status((int8_t)Type)
			{
				VI_ASSERT(Type == Optional::None, "only none is accepted for this constructor");
			}
			Option(const V& Other) : Status(1)
			{
				OptionUtils::CopyBuffer<V>(Value, (const char*)&Other, sizeof(V));
			}
			Option(V&& Other) noexcept : Status(1)
			{
				OptionUtils::MoveBuffer<V>(Value, (char*)&Other, sizeof(V));
			}
			Option(const Option& Other) : Status(Other.Status)
			{
				if (Status > 0)
					OptionUtils::CopyBuffer<V>(Value, Other.Value, sizeof(Value));
			}
			Option(Option&& Other) noexcept : Status(Other.Status)
			{
				if (Status > 0)
					OptionUtils::MoveBuffer<V>(Value, Other.Value, sizeof(Value));
			}
			~Option()
			{
				if (Status > 0)
					((V*)Value)->~V();
				Status = 0;
			}
			Option& operator= (Optional Type)
			{
				VI_ASSERT(Type == Optional::None, "only none is accepted for this operator");
				this->~Option();
				Status = (int8_t)Type;
				return *this;
			}
			Option& operator= (const V& Other)
			{
				this->~Option();
				OptionUtils::CopyBuffer<V>(Value, (const char*)&Other, sizeof(V));
				Status = 1;
				return *this;
			}
			Option& operator= (V&& Other) noexcept
			{
				this->~Option();
				OptionUtils::MoveBuffer<V>(Value, (char*)&Other, sizeof(V));
				Status = 1;
				return *this;
			}
			Option& operator= (const Option& Other)
			{
				if (this == &Other)
					return *this;

				this->~Option();
				Status = Other.Status;
				if (Status > 0)
					OptionUtils::CopyBuffer<V>(Value, Other.Value, sizeof(Value));

				return *this;
			}
			Option& operator= (Option&& Other) noexcept
			{
				if (this == &Other)
					return *this;

				this->~Option();
				Status = Other.Status;
				if (Status > 0)
					OptionUtils::MoveBuffer<V>(Value, Other.Value, sizeof(Value));

				return *this;
			}
			const V& Expect(const std::string_view& Message) const&
			{
				VI_PANIC(IsValue(), "%.*s CAUSING unwrapping an empty value", (int)Message.size(), Message.data());
				const V& Reference = *(V*)Value;
				return Reference;
			}
			const V&& Expect(const std::string_view& Message) const&&
			{
				VI_PANIC(IsValue(), "%.*s CAUSING unwrapping an empty value", (int)Message.size(), Message.data());
				V& Reference = *(V*)Value;
				return std::move(Reference);
			}
			V& Expect(const std::string_view& Message)&
			{
				VI_PANIC(IsValue(), "%.*s CAUSING unwrapping an empty value", (int)Message.size(), Message.data());
				V& Reference = *(V*)Value;
				return Reference;
			}
			V&& Expect(const std::string_view& Message)&&
			{
				VI_PANIC(IsValue(), "%.*s CAUSING unwrapping an empty value", (int)Message.size(), Message.data());
				V& Reference = *(V*)Value;
				return std::move(Reference);
			}
			const V& Or(const V& IfNone) const&
			{
				if (!IsValue())
					return IfNone;

				const V& Reference = *(V*)Value;
				return Reference;
			}
			const V&& Or(const V&& IfNone) const&&
			{
				if (!IsValue())
					return std::move(IfNone);

				V& Reference = *(V*)Value;
				return std::move(Reference);
			}
			V& Or(V& IfNone)&
			{
				if (!IsValue())
					return IfNone;

				V& Reference = *(V*)Value;
				return Reference;
			}
			V&& Or(V&& IfNone)&&
			{
				if (!IsValue())
					return std::move(IfNone);

				V& Reference = *(V*)Value;
				return std::move(Reference);
			}
			const V& Unwrap() const&
			{
				VI_PANIC(IsValue(), "unwrapping an empty value");
				const V& Reference = *(V*)Value;
				return Reference;
			}
			const V&& Unwrap() const&&
			{
				VI_PANIC(IsValue(), "unwrapping an empty value");
				V& Reference = *(V*)Value;
				return std::move(Reference);
			}
			V& Unwrap()&
			{
				VI_PANIC(IsValue(), "unwrapping an empty value");
				V& Reference = *(V*)Value;
				return Reference;
			}
			V&& Unwrap()&&
			{
				VI_PANIC(IsValue(), "unwrapping an empty value");
				V& Reference = *(V*)Value;
				return std::move(Reference);
			}
			const V* Address() const
			{
				const V* Reference = IsValue() ? (V*)Value : nullptr;
				return Reference;
			}
			V* Address()
			{
				V* Reference = IsValue() ? (V*)Value : nullptr;
				return Reference;
			}
			void Reset()
			{
				this->~Option();
				Status = 0;
			}
			bool IsNone() const
			{
				return Status == 0;
			}
			bool IsValue() const
			{
				return Status > 0;
			}
			explicit operator bool() const
			{
				return IsValue();
			}
			explicit operator Optional() const
			{
				return (Optional)Status;
			}
			const V& operator* () const&
			{
				VI_PANIC(IsValue(), "unwrapping an empty value");
				const V& Reference = *(V*)Value;
				return Reference;
			}
			const V&& operator* () const&&
			{
				VI_PANIC(IsValue(), "unwrapping an empty value");
				V& Reference = *(V*)Value;
				return std::move(Reference);
			}
			V& operator* ()&
			{
				VI_PANIC(IsValue(), "unwrapping an empty value");
				V& Reference = *(V*)Value;
				return Reference;
			}
			V&& operator* ()&&
			{
				VI_PANIC(IsValue(), "unwrapping an empty value");
				V& Reference = *(V*)Value;
				return std::move(Reference);
			}
			const typename std::remove_pointer<V>::type* operator-> () const
			{
				VI_ASSERT(IsValue(), "unwrapping an empty value");
				const auto* Reference = OptionUtils::ToConstPointer<V>(Value);
				return Reference;
			}
			typename std::remove_pointer<V>::type* operator-> ()
			{
				VI_ASSERT(IsValue(), "unwrapping an empty value");
				auto* Reference = OptionUtils::ToPointer<V>(Value);
				return Reference;
			}
		};

		template <>
		class Option<void>
		{
		private:
			int8_t Status;

		public:
			Option(Optional Type) : Status((int8_t)Type)
			{
			}
			Option(const Option&) = default;
			Option(Option&&) = default;
			~Option() = default;
			Option& operator= (Optional Type)
			{
				Status = (int8_t)Type;
				return *this;
			}
			Option& operator= (const Option&) = default;
			Option& operator= (Option&&) = default;
			void Expect(const std::string_view& Message) const
			{
				VI_PANIC(IsValue(), "%.*s CAUSING unwrapping an empty value", (int)Message.size(), Message.data());
			}
			void Unwrap() const
			{
				VI_PANIC(IsValue(), "unwrapping an empty value");
			}
			void Address() const
			{
			}
			void Reset()
			{
				Status = 0;
			}
			bool IsNone() const
			{
				return Status == 0;
			}
			bool IsValue() const
			{
				return Status > 0;
			}
			explicit operator bool() const
			{
				return IsValue();
			}
			explicit operator Optional() const
			{
				return (Optional)Status;
			}
		};

		template <typename V, typename E>
		class Expects
		{
			static_assert(!std::is_same<E, void>::value, "error type should not be void");
			static_assert(!std::is_same<E, V>::value, "error type should not be value type");

		private:
			template <typename... Types>
			struct Storage
			{
				alignas(Types...) char Buffer[std::max({ sizeof(Types)... })];
			};

		private:
			Storage<V, E> Value;
			int8_t Status;

		public:
			Expects(const V& Other) : Status(1)
			{
				OptionUtils::CopyBuffer<V>(Value.Buffer, (const char*)&Other, sizeof(V));
			}
			Expects(V&& Other) noexcept : Status(1)
			{
				OptionUtils::MoveBuffer<V>(Value.Buffer, (char*)&Other, sizeof(V));
			}
			Expects(const E& Other) noexcept : Status(-1)
			{
				OptionUtils::CopyBuffer<E>(Value.Buffer, (const char*)&Other, sizeof(E));
			}
			Expects(E&& Other) noexcept : Status(-1)
			{
				OptionUtils::MoveBuffer<E>(Value.Buffer, (char*)&Other, sizeof(E));
			}
			Expects(const Expects& Other) : Status(Other.Status)
			{
				if (Status > 0)
					OptionUtils::CopyBuffer<V>(Value.Buffer, Other.Value.Buffer, sizeof(Value.Buffer));
				else if (Status < 0)
					OptionUtils::CopyBuffer<E>(Value.Buffer, Other.Value.Buffer, sizeof(Value.Buffer));
			}
			Expects(Expects&& Other) noexcept : Status(Other.Status)
			{
				if (Status > 0)
					OptionUtils::MoveBuffer<V>(Value.Buffer, Other.Value.Buffer, sizeof(Value.Buffer));
				else if (Status < 0)
					OptionUtils::MoveBuffer<E>(Value.Buffer, Other.Value.Buffer, sizeof(Value.Buffer));
			}
			~Expects()
			{
				if (Status > 0)
					((V*)Value.Buffer)->~V();
				else if (Status < 0)
					((E*)Value.Buffer)->~E();
				Status = 0;
			}
			Expects& operator= (const V& Other)
			{
				this->~Expects();
				OptionUtils::CopyBuffer<V>(Value.Buffer, (const char*)&Other, sizeof(V));
				Status = 1;
				return *this;
			}
			Expects& operator= (V&& Other) noexcept
			{
				this->~Expects();
				OptionUtils::MoveBuffer<V>(Value.Buffer, (char*)&Other, sizeof(V));
				Status = 1;
				return *this;
			}
			Expects& operator= (const E& Other)
			{
				this->~Expects();
				OptionUtils::CopyBuffer<E>(Value.Buffer, (const char*)&Other, sizeof(E));
				Status = -1;
				return *this;
			}
			Expects& operator= (E&& Other) noexcept
			{
				this->~Expects();
				OptionUtils::MoveBuffer<E>(Value.Buffer, (char*)&Other, sizeof(E));
				Status = -1;
				return *this;
			}
			Expects& operator= (const Expects& Other)
			{
				if (this == &Other)
					return *this;

				this->~Expects();
				Status = Other.Status;
				if (Status > 0)
					OptionUtils::CopyBuffer<V>(Value.Buffer, Other.Value.Buffer, sizeof(Value.Buffer));
				else if (Status < 0)
					OptionUtils::CopyBuffer<E>(Value.Buffer, Other.Value.Buffer, sizeof(Value.Buffer));

				return *this;
			}
			Expects& operator= (Expects&& Other) noexcept
			{
				if (this == &Other)
					return *this;

				this->~Expects();
				Status = Other.Status;
				if (Status > 0)
					OptionUtils::MoveBuffer<V>(Value.Buffer, Other.Value.Buffer, sizeof(Value.Buffer));
				else if (Status < 0)
					OptionUtils::MoveBuffer<E>(Value.Buffer, Other.Value.Buffer, sizeof(Value.Buffer));

				return *this;
			}
			const V& Expect(const std::string_view& Message) const&
			{
				VI_PANIC(IsValue(), "%s CAUSING %.*s", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str(), (int)Message.size(), Message.data());
				const V& Reference = *(V*)Value.Buffer;
				return Reference;
			}
			const V&& Expect(const std::string_view& Message) const&&
			{
				VI_PANIC(IsValue(), "%s CAUSING %.*s", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str(), (int)Message.size(), Message.data());
				V& Reference = *(V*)Value.Buffer;
				return std::move(Reference);
			}
			V& Expect(const std::string_view& Message)&
			{
				VI_PANIC(IsValue(), "%s CAUSING %.*s", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str(), (int)Message.size(), Message.data());
				V& Reference = *(V*)Value.Buffer;
				return Reference;
			}
			V&& Expect(const std::string_view& Message)&&
			{
				VI_PANIC(IsValue(), "%s CAUSING %.*s", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str(), (int)Message.size(), Message.data());
				V& Reference = *(V*)Value.Buffer;
				return std::move(Reference);
			}
			const V& Or(const V& IfNone) const&
			{
				if (!IsValue())
					return IfNone;

				const V& Reference = *(V*)Value.Buffer;
				return Reference;
			}
			const V&& Or(const V&& IfNone) const&&
			{
				if (!IsValue())
					return std::move(IfNone);

				V& Reference = *(V*)Value.Buffer;
				return std::move(Reference);
			}
			V& Or(V& IfNone)&
			{
				if (!IsValue())
					return IfNone;

				V& Reference = *(V*)Value.Buffer;
				return Reference;
			}
			V&& Or(V&& IfNone)&&
			{
				if (!IsValue())
					return std::move(IfNone);

				V& Reference = *(V*)Value.Buffer;
				return std::move(Reference);
			}
			const V& Unwrap() const&
			{
				VI_PANIC(IsValue(), "%s CAUSING unwrapping an empty value", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str());
				const V& Reference = *(V*)Value.Buffer;
				return Reference;
			}
			const V&& Unwrap() const&&
			{
				VI_PANIC(IsValue(), "%s CAUSING unwrapping an empty value", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str());
				V& Reference = *(V*)Value.Buffer;
				return std::move(Reference);
			}
			V& Unwrap()&
			{
				VI_PANIC(IsValue(), "%s CAUSING unwrapping an empty value", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str());
				V& Reference = *(V*)Value.Buffer;
				return Reference;
			}
			V&& Unwrap()&&
			{
				VI_PANIC(IsValue(), "%s CAUSING unwrapping an empty value", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str());
				V& Reference = *(V*)Value.Buffer;
				return std::move(Reference);
			}
			const V* Address() const
			{
				const V* Reference = IsValue() ? (V*)Value.Buffer : nullptr;
				return Reference;
			}
			V* Address()
			{
				V* Reference = IsValue() ? (V*)Value.Buffer : nullptr;
				return Reference;
			}
			const E& Error() const&
			{
				VI_PANIC(IsError(), "value does not contain any errors");
				const E& Reference = *(E*)Value.Buffer;
				return Reference;
			}
			const E&& Error() const&&
			{
				VI_PANIC(IsError(), "value does not contain an error");
				E& Reference = *(E*)Value.Buffer;
				return std::move(Reference);
			}
			E& Error()&
			{
				VI_PANIC(IsError(), "value does not contain an error");
				E& Reference = *(E*)Value.Buffer;
				return Reference;
			}
			E&& Error()&&
			{
				VI_PANIC(IsError(), "value does not contain an error");
				E& Reference = *(E*)Value.Buffer;
				return std::move(Reference);
			}
			String What() const
			{
				VI_ASSERT(!IsValue(), "value does not contain an error");
				auto Reason = OptionUtils::ToErrorText<E>(Value.Buffer, IsError());
				return String(Reason.begin(), Reason.end());
			}
			void Report(const std::string_view& Message) const
			{
				if (IsError())
					VI_ERR("%s CAUSING %.*s", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str(), (int)Message.size(), Message.data());
			}
			void Reset()
			{
				this->~Expects();
				Status = 0;
			}
			bool IsNone() const
			{
				return Status == 0;
			}
			bool IsValue() const
			{
				return Status > 0;
			}
			bool IsError() const
			{
				return Status < 0;
			}
			explicit operator bool() const
			{
				return IsValue();
			}
			explicit operator Expectation() const
			{
				return (Expectation)Status;
			}
			const V& operator* () const&
			{
				VI_PANIC(IsValue(), "%s CAUSING unwrapping an empty value", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str());
				const V& Reference = *(V*)Value.Buffer;
				return Reference;
			}
			const V&& operator* () const&&
			{
				VI_PANIC(IsValue(), "%s CAUSING unwrapping an empty value", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str());
				V& Reference = *(V*)Value.Buffer;
				return std::move(Reference);
			}
			V& operator* ()&
			{
				VI_PANIC(IsValue(), "%s CAUSING unwrapping an empty value", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str());
				V& Reference = *(V*)Value.Buffer;
				return Reference;
			}
			V&& operator* ()&&
			{
				VI_PANIC(IsValue(), "%s CAUSING unwrapping an empty value", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str());
				V& Reference = *(V*)Value.Buffer;
				return std::move(Reference);
			}
			const typename std::remove_pointer<V>::type* operator-> () const
			{
				VI_ASSERT(IsValue(), "%s CAUSING unwrapping an empty value", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str());
				auto* Reference = OptionUtils::ToConstPointer<V>(Value.Buffer);
				return Reference;
			}
			typename std::remove_pointer<V>::type* operator-> ()
			{
				VI_ASSERT(IsValue(), "%s CAUSING unwrapping an empty value", OptionUtils::ToErrorText<E>(Value.Buffer, IsError()).c_str());
				auto* Reference = OptionUtils::ToPointer<V>(Value.Buffer);
				return Reference;
			}
		};

		template <typename E>
		class Expects<void, E>
		{
			static_assert(!std::is_same<E, void>::value, "error type should not be void");

		private:
			alignas(E) char Value[sizeof(E)];
			int8_t Status;

		public:
			Expects(Expectation Type) : Status((int8_t)Type)
			{
				VI_ASSERT(Type == Expectation::Met, "only met is accepted for this constructor");
			}
			Expects(const E& Other) noexcept : Status(-1)
			{
				OptionUtils::CopyBuffer<E>(Value, (const char*)&Other, sizeof(E));
			}
			Expects(E&& Other) noexcept : Status(-1)
			{
				OptionUtils::MoveBuffer<E>(Value, (char*)&Other, sizeof(E));
			}
			Expects(const Expects& Other) : Status(Other.Status)
			{
				if (Status < 0)
					OptionUtils::CopyBuffer<E>(Value, Other.Value, sizeof(Value));
			}
			Expects(Expects&& Other) noexcept : Status(Other.Status)
			{
				if (Status < 0)
					OptionUtils::MoveBuffer<E>(Value, Other.Value, sizeof(Value));
			}
			~Expects()
			{
				if (Status < 0)
					((E*)Value)->~E();
				Status = 0;
			}
			Expects& operator= (Expectation Type)
			{
				VI_ASSERT(Type == Expectation::Met, "only met is accepted for this constructor");
				this->~Expects();
				Status = (int8_t)Type;
				return *this;
			}
			Expects& operator= (const E& Other)
			{
				this->~Expects();
				OptionUtils::CopyBuffer<E>(Value, (const char*)&Other, sizeof(E));
				Status = -1;
				return *this;
			}
			Expects& operator= (E&& Other) noexcept
			{
				this->~Expects();
				OptionUtils::MoveBuffer<E>(Value, (char*)&Other, sizeof(E));
				Status = -1;
				return *this;
			}
			Expects& operator= (const Expects& Other)
			{
				if (this == &Other)
					return *this;

				this->~Expects();
				Status = Other.Status;
				if (Status < 0)
					OptionUtils::CopyBuffer<E>(Value, Other.Value, sizeof(Value));

				return *this;
			}
			Expects& operator= (Expects&& Other) noexcept
			{
				if (this == &Other)
					return *this;

				this->~Expects();
				Status = Other.Status;
				if (Status < 0)
					OptionUtils::MoveBuffer<E>(Value, Other.Value, sizeof(Value));

				return *this;
			}
			void Expect(const std::string_view& Message) const
			{
				VI_PANIC(IsValue(), "%s CAUSING %.*s", OptionUtils::ToErrorText<E>(Value, IsError()).c_str(), (int)Message.size(), Message.data());
			}
			void Unwrap() const
			{
				VI_PANIC(IsValue(), "%s CAUSING unwrapping an empty value", OptionUtils::ToErrorText<E>(Value, IsError()).c_str());
			}
			void Address() const
			{
			}
			const E& Error() const&
			{
				VI_PANIC(IsError(), "value does not contain any errors");
				const E& Reference = *(E*)Value;
				return Reference;
			}
			const E&& Error() const&&
			{
				VI_PANIC(IsError(), "value does not contain an error");
				E& Reference = *(E*)Value;
				return std::move(Reference);
			}
			E& Error()&
			{
				VI_PANIC(IsError(), "value does not contain an error");
				E& Reference = *(E*)Value;
				return Reference;
			}
			E&& Error()&&
			{
				VI_PANIC(IsError(), "value does not contain an error");
				E& Reference = *(E*)Value;
				return std::move(Reference);
			}
			String What() const
			{
				VI_ASSERT(!IsValue(), "value does not contain an error");
				auto Reason = OptionUtils::ToErrorText<E>(Value, IsError());
				return String(Reason.begin(), Reason.end());
			}
			void Report(const std::string_view& Message) const
			{
				if (IsError())
					VI_ERR("%s CAUSING %.*s", OptionUtils::ToErrorText<E>(Value, IsError()).c_str(), (int)Message.size(), Message.data());
			}
			void Reset()
			{
				this->~Expects();
				Status = 0;
			}
			bool IsNone() const
			{
				return Status == 0;
			}
			bool IsValue() const
			{
				return Status > 0;
			}
			bool IsError() const
			{
				return Status < 0;
			}
			explicit operator bool() const
			{
				return !IsError();
			}
			explicit operator Expectation() const
			{
				return (Expectation)Status;
			}
		};

		template <typename V>
		using ExpectsIO = Expects<V, std::error_condition>;

		template <typename V>
		using ExpectsParser = Expects<V, ParserException>;

		template <typename V>
		using ExpectsSystem = Expects<V, SystemException>;

		struct VI_OUT Coroutine
		{
			friend Costate;

		private:
			std::atomic<Coexecution> State;
			TaskCallback Callback;
			Cocontext* Slave;
			Costate* Master;

		public:
			TaskCallback Return;
			void* UserData;

		public:
			Coroutine(Costate* Base, TaskCallback&& Procedure) noexcept;
			~Coroutine() noexcept;
		};

		struct VI_OUT Decimal
		{
		private:
			String Source;
			int32_t Length;
			int8_t Sign;

		public:
			Decimal() noexcept;
			Decimal(const std::string_view& Value) noexcept;
			Decimal(const Decimal& Value) noexcept;
			Decimal(Decimal&& Value) noexcept;
			Decimal& Truncate(uint32_t Value);
			Decimal& Round(uint32_t Value);
			Decimal& Trim();
			Decimal& Unlead();
			Decimal& Untrail();
			bool IsNaN() const;
			bool IsZero() const;
			bool IsZeroOrNaN() const;
			bool IsPositive() const;
			bool IsNegative() const;
			double ToDouble() const;
			float ToFloat() const;
			int8_t ToInt8() const;
			uint8_t ToUInt8() const;
			int16_t ToInt16() const;
			uint16_t ToUInt16() const;
			int32_t ToInt32() const;
			uint32_t ToUInt32() const;
			int64_t ToInt64() const;
			uint64_t ToUInt64() const;
			String ToString() const;
			String ToExponent() const;
			const String& Numeric() const;
			uint32_t Decimals() const;
			uint32_t Ints() const;
			uint32_t Size() const;
			int8_t Position() const;
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
			explicit operator double() const
			{
				return ToDouble();
			}
			explicit operator float() const
			{
				return ToFloat();
			}
			explicit operator int64_t() const
			{
				return ToInt64();
			}
			explicit operator uint64_t() const
			{
				return ToUInt64();
			}

		private:
			void ApplyBase10(const std::string_view& Text);
			void ApplyZero();

		public:
			template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
			Decimal(const T& Right) noexcept : Decimal()
			{
				if (Right != T(0))
					ApplyBase10(std::to_string(Right));
				else
					ApplyZero();
			}

		public:
			VI_OUT friend Decimal operator + (const Decimal& Left, const Decimal& Right);
			VI_OUT friend Decimal operator - (const Decimal& Left, const Decimal& Right);
			VI_OUT friend Decimal operator * (const Decimal& Left, const Decimal& Right);
			VI_OUT friend Decimal operator / (const Decimal& Left, const Decimal& Right);
			VI_OUT friend Decimal operator % (const Decimal& Left, const Decimal& Right);

		public:
			static Decimal From(const std::string_view& Data, uint8_t Base);
			static Decimal Zero();
			static Decimal NaN();

		private:
			static Decimal Sum(const Decimal& Left, const Decimal& Right);
			static Decimal Subtract(const Decimal& Left, const Decimal& Right);
			static Decimal Multiply(const Decimal& Left, const Decimal& Right);
			static int CompareNum(const Decimal& Left, const Decimal& Right);
			static int CharToInt(char Value);
			static char IntToChar(const int& Value);
		};

		struct VI_OUT Variant
		{
			friend Schema;
			friend Var;

		private:
			union Tag
			{
				char String[32];
				char* Pointer;
				int64_t Integer;
				double Number;
				bool Boolean;
			} Value;

		private:
			VarType Type;
			uint32_t Length;

		public:
			Variant() noexcept;
			Variant(const Variant& Other) noexcept;
			Variant(Variant&& Other) noexcept;
			~Variant() noexcept;
			bool Deserialize(const std::string_view& Value, bool Strict = false);
			String Serialize() const;
			String GetBlob() const;
			Decimal GetDecimal() const;
			void* GetPointer() const;
			void* GetContainer();
			std::string_view GetString() const;
			uint8_t* GetBinary() const;
			int64_t GetInteger() const;
			double GetNumber() const;
			bool GetBoolean() const;
			VarType GetType() const;
			size_t Size() const;
			Variant& operator= (const Variant& Other) noexcept;
			Variant& operator= (Variant&& Other) noexcept;
			bool operator== (const Variant& Other) const;
			bool operator!= (const Variant& Other) const;
			explicit operator bool() const;
			bool IsString(const std::string_view& Value) const;
			bool IsObject() const;
			bool Empty() const;
			bool Is(VarType Value) const;

		private:
			Variant(VarType NewType) noexcept;
			bool Same(const Variant& Value) const;
			void Copy(const Variant& Other);
			void Move(Variant&& Other);
			void Free();

		private:
			static size_t GetMaxSmallStringSize();
		};

		typedef Vector<Variant> VariantList;
		typedef UnorderedMap<String, Variant> VariantArgs;

		struct VI_OUT TextSettle
		{
			size_t Start = 0;
			size_t End = 0;
			bool Found = false;
		};

		struct VI_OUT FileState
		{
			size_t Size = 0;
			size_t Links = 0;
			size_t Permissions = 0;
			size_t Document = 0;
			size_t Device = 0;
			size_t UserId = 0;
			size_t GroupId = 0;
			int64_t LastAccess = 0;
			int64_t LastPermissionChange = 0;
			int64_t LastModified = 0;
			bool Exists = false;
		};

		struct VI_OUT Timeout
		{
			std::chrono::microseconds Expires;
			TaskCallback Callback;
			TaskId Id;
			bool Alive;

			Timeout(TaskCallback&& NewCallback, const std::chrono::microseconds& NewTimeout, TaskId NewId, bool NewAlive) noexcept;
			Timeout(const Timeout& Other) noexcept;
			Timeout(Timeout&& Other) noexcept;
			Timeout& operator= (const Timeout& Other) noexcept;
			Timeout& operator= (Timeout&& Other) noexcept;
		};

		struct VI_OUT FileEntry
		{
			size_t Size = 0;
			int64_t LastModified = 0;
			int64_t CreationTime = 0;
			bool IsReferenced = false;
			bool IsDirectory = false;
			bool IsExists = false;
		};

		struct VI_OUT DateTime
		{
		private:
			std::chrono::system_clock::duration Offset;
			struct tm Timepoint;
			bool Synchronized;
			bool Globalized;

		public:
			DateTime() noexcept;
			DateTime(const struct tm& Duration) noexcept;
			DateTime(std::chrono::system_clock::duration&& Duration) noexcept;
			DateTime(const DateTime&) = default;
			DateTime(DateTime&&) noexcept = default;
			DateTime& operator= (const DateTime&) = default;
			DateTime& operator= (DateTime&&) noexcept = default;
			DateTime& operator +=(const DateTime& Right);
			DateTime& operator -=(const DateTime& Right);
			bool operator >=(const DateTime& Right);
			bool operator <=(const DateTime& Right);
			bool operator >(const DateTime& Right);
			bool operator <(const DateTime& Right);
			bool operator ==(const DateTime& Right);
			DateTime operator +(const DateTime& Right) const;
			DateTime operator -(const DateTime& Right) const;
			DateTime& ApplyOffset(bool Always = false);
			DateTime& ApplyTimepoint(bool Always = false);
			DateTime& UseGlobalTime();
			DateTime& UseLocalTime();
			DateTime& SetSecond(uint8_t Value);
			DateTime& SetMinute(uint8_t Value);
			DateTime& SetHour(uint8_t Value);
			DateTime& SetDay(uint8_t Value);
			DateTime& SetWeek(uint8_t Value);
			DateTime& SetMonth(uint8_t Value);
			DateTime& SetYear(uint32_t Value);
			String Serialize(const std::string_view& Format) const;
			uint8_t Second() const;
			uint8_t Minute() const;
			uint8_t Hour() const;
			uint8_t Day() const;
			uint8_t Week() const;
			uint8_t Month() const;
			uint32_t Year() const;
			int64_t Nanoseconds() const;
			int64_t Microseconds() const;
			int64_t Milliseconds() const;
			int64_t Seconds() const;
			const struct tm& CurrentTimepoint() const;
			const std::chrono::system_clock::duration& CurrentOffset() const;

		public:
			template <typename Rep, typename Period>
			DateTime(const std::chrono::duration<Rep, Period>& Duration) noexcept : DateTime(std::chrono::duration_cast<std::chrono::system_clock::duration>(Duration))
			{
			}

		public:
			static std::chrono::system_clock::duration Now();
			static DateTime FromNanoseconds(int64_t Value);
			static DateTime FromMicroseconds(int64_t Value);
			static DateTime FromMilliseconds(int64_t Value);
			static DateTime FromSeconds(int64_t Value);
			static DateTime FromSerialized(const std::string_view& Text, const std::string_view& Format);
			static String SerializeGlobal(const std::chrono::system_clock::duration& Time, const std::string_view& Format);
			static String SerializeLocal(const std::chrono::system_clock::duration& Time, const std::string_view& Format);
			static std::string_view SerializeGlobal(char* Buffer, size_t Length, const std::chrono::system_clock::duration& Duration, const std::string_view& Format);
			static std::string_view SerializeLocal(char* Buffer, size_t Length, const std::chrono::system_clock::duration& Duration, const std::string_view& Format);
			static std::string_view FormatIso8601Time();
			static std::string_view FormatWebTime();
			static std::string_view FormatWebLocalTime();
			static std::string_view FormatCompactTime();
			static int64_t SecondsFromSerialized(const std::string_view& Text, const std::string_view& Format);
			static bool MakeGlobalTime(time_t Time, struct tm* Timepoint);
			static bool MakeLocalTime(time_t Time, struct tm* Timepoint);
		};

		struct VI_OUT Stringify
		{
		public:
			static String& EscapePrint(String& Other);
			static String& Escape(String& Other);
			static String& Unescape(String& Other);
			static String& ToUpper(String& Other);
			static String& ToLower(String& Other);
			static String& Clip(String& Other, size_t Length);
			static String& Compress(String& Other, const std::string_view& SpaceIfNotFollowedOrPrecededByOf, const std::string_view& NotInBetweenOf, size_t Start = 0U);
			static String& ReplaceOf(String& Other, const std::string_view& Chars, const std::string_view& To, size_t Start = 0U);
			static String& ReplaceNotOf(String& Other, const std::string_view& Chars, const std::string_view& To, size_t Start = 0U);
			static String& ReplaceGroups(String& Other, const std::string_view& FromRegex, const std::string_view& To);
			static String& Replace(String& Other, const std::string_view& From, const std::string_view& To, size_t Start = 0U);
			static String& Replace(String& Other, char From, char To, size_t Position = 0U);
			static String& Replace(String& Other, char From, char To, size_t Position, size_t Count);
			static String& ReplacePart(String& Other, size_t Start, size_t End, const std::string_view& Value);
			static String& ReplaceStartsWithEndsOf(String& Other, const std::string_view& Begins, const std::string_view& EndsOf, const std::string_view& With, size_t Start = 0U);
			static String& ReplaceInBetween(String& Other, const std::string_view& Begins, const std::string_view& Ends, const std::string_view& With, bool Recursive, size_t Start = 0U);
			static String& ReplaceNotInBetween(String& Other, const std::string_view& Begins, const std::string_view& Ends, const std::string_view& With, bool Recursive, size_t Start = 0U);
			static String& ReplaceParts(String& Other, Vector<std::pair<String, TextSettle>>& Inout, const std::string_view& With, const std::function<char(const std::string_view&, char, int)>& Surrounding = nullptr);
			static String& ReplaceParts(String& Other, Vector<TextSettle>& Inout, const std::string_view& With, const std::function<char(char, int)>& Surrounding = nullptr);
			static String& RemovePart(String& Other, size_t Start, size_t End);
			static String& Reverse(String& Other);
			static String& Reverse(String& Other, size_t Start, size_t End);
			static String& Substring(String& Other, const TextSettle& Result);
			static String& Splice(String& Other, size_t Start, size_t End);
			static String& Trim(String& Other);
			static String& TrimStart(String& Other);
			static String& TrimEnd(String& Other);
			static String& Fill(String& Other, char Char);
			static String& Fill(String& Other, char Char, size_t Count);
			static String& Fill(String& Other, char Char, size_t Start, size_t Count);
			static String& Append(String& Other, const char* Format, ...);
			static String& Erase(String& Other, size_t Position);
			static String& Erase(String& Other, size_t Position, size_t Count);
			static String& EraseOffsets(String& Other, size_t Start, size_t End);
			static ExpectsSystem<void> EvalEnvs(String& Other, const std::string_view& Directory, const Vector<String>& Tokens, const std::string_view& Token = ":net");
			static Vector<std::pair<String, TextSettle>> FindInBetween(const std::string_view& Other, const std::string_view& Begins, const std::string_view& Ends, const std::string_view& NotInSubBetweenOf, size_t Offset = 0U);
			static Vector<std::pair<String, TextSettle>> FindInBetweenInCode(const std::string_view& Other, const std::string_view& Begins, const std::string_view& Ends, size_t Offset = 0U);
			static Vector<std::pair<String, TextSettle>> FindStartsWithEndsOf(const std::string_view& Other, const std::string_view& Begins, const std::string_view& EndsOf, const std::string_view& NotInSubBetweenOf, size_t Offset = 0U);
			static void PmFindInBetween(Vector<std::pair<String, TextSettle>>& Data, const std::string_view& Other, const std::string_view& Begins, const std::string_view& Ends, const std::string_view& NotInSubBetweenOf, size_t Offset = 0U);
			static void PmFindInBetweenInCode(Vector<std::pair<String, TextSettle>>& Data, const std::string_view& Other, const std::string_view& Begins, const std::string_view& Ends, size_t Offset = 0U);
			static void PmFindStartsWithEndsOf(Vector<std::pair<String, TextSettle>>& Data, const std::string_view& Other, const std::string_view& Begins, const std::string_view& EndsOf, const std::string_view& NotInSubBetweenOf, size_t Offset = 0U);
			static TextSettle ReverseFind(const std::string_view& Other, const std::string_view& Needle, size_t Offset = 0U);
			static TextSettle ReverseFind(const std::string_view& Other, char Needle, size_t Offset = 0U);
			static TextSettle ReverseFindUnescaped(const std::string_view& Other, char Needle, size_t Offset = 0U);
			static TextSettle ReverseFindOf(const std::string_view& Other, const std::string_view& Needle, size_t Offset = 0U);
			static TextSettle Find(const std::string_view& Other, const std::string_view& Needle, size_t Offset = 0U);
			static TextSettle Find(const std::string_view& Other, char Needle, size_t Offset = 0U);
			static TextSettle FindUnescaped(const std::string_view& Other, char Needle, size_t Offset = 0U);
			static TextSettle FindOf(const std::string_view& Other, const std::string_view& Needle, size_t Offset = 0U);
			static TextSettle FindNotOf(const std::string_view& Other, const std::string_view& Needle, size_t Offset = 0U);
			static size_t CountLines(const std::string_view& Other);
			static bool IsCString(const std::string_view& Other);
			static bool IsNotPrecededByEscape(const std::string_view& Other, size_t Offset, char Escape = '\\');
			static bool IsEmptyOrWhitespace(const std::string_view& Other);
			static bool IsPrecededBy(const std::string_view& Other, size_t At, const std::string_view& Of);
			static bool IsFollowedBy(const std::string_view& Other, size_t At, const std::string_view& Of);
			static bool StartsWith(const std::string_view& Other, const std::string_view& Value, size_t Offset = 0U);
			static bool StartsOf(const std::string_view& Other, const std::string_view& Value, size_t Offset = 0U);
			static bool StartsNotOf(const std::string_view& Other, const std::string_view& Value, size_t Offset = 0U);
			static bool EndsWith(const std::string_view& Other, const std::string_view& Value);
			static bool EndsWith(const std::string_view& Other, char Value);
			static bool EndsOf(const std::string_view& Other, const std::string_view& Value);
			static bool EndsNotOf(const std::string_view& Other, const std::string_view& Value);
			static bool HasInteger(const std::string_view& Other);
			static bool HasNumber(const std::string_view& Other);
			static bool HasDecimal(const std::string_view& Other);
			static String Text(const char* Format, ...);
			static WideString ToWide(const std::string_view& Other);
			static Vector<String> Split(const std::string_view& Other, const std::string_view& With, size_t Start = 0U);
			static Vector<String> Split(const std::string_view& Other, char With, size_t Start = 0U);
			static Vector<String> SplitMax(const std::string_view& Other, char With, size_t MaxCount, size_t Start = 0U);
			static Vector<String> SplitOf(const std::string_view& Other, const std::string_view& With, size_t Start = 0U);
			static Vector<String> SplitNotOf(const std::string_view& Other, const std::string_view& With, size_t Start = 0U);
			static void PmSplit(Vector<String>& Data, const std::string_view& Other, const std::string_view& With, size_t Start = 0U);
			static void PmSplit(Vector<String>& Data, const std::string_view& Other, char With, size_t Start = 0U);
			static void PmSplitMax(Vector<String>& Data, const std::string_view& Other, char With, size_t MaxCount, size_t Start = 0U);
			static void PmSplitOf(Vector<String>& Data, const std::string_view& Other, const std::string_view& With, size_t Start = 0U);
			static void PmSplitNotOf(Vector<String>& Data, const std::string_view& Other, const std::string_view& With, size_t Start = 0U);
			static bool IsNumericOrDot(char Char);
			static bool IsNumericOrDotOrWhitespace(char Char);
			static bool IsHex(char Char);
			static bool IsHexOrDot(char Char);
			static bool IsHexOrDotOrWhitespace(char Char);
			static bool IsAlphabetic(char Char);
			static bool IsNumeric(char Char);
			static bool IsAlphanum(char Char);
			static bool IsWhitespace(char Char);
			static char ToLowerLiteral(char Char);
			static char ToUpperLiteral(char Char);
			static bool CaseEquals(const std::string_view& Value1, const std::string_view& Value2);
			static int CaseCompare(const std::string_view& Value1, const std::string_view& Value2);
			static int Match(const char* Pattern, const std::string_view& Text);
			static int Match(const char* Pattern, size_t Length, const std::string_view& Text);
			static void ConvertToWide(const std::string_view& Input, wchar_t* Output, size_t OutputSize);
		};

		struct VI_OUT ConcurrentTimeoutQueue
		{
			OrderedMap<std::chrono::microseconds, Timeout> Queue;
			std::condition_variable Notify;
			std::mutex Update;
			bool Resync = true;
		};

		struct VI_OUT InlineArgs
		{
		public:
			UnorderedMap<String, String> Args;
			Vector<String> Params;
			String Path;

		public:
			InlineArgs() noexcept;
			bool IsEnabled(const std::string_view& Option, const std::string_view& Shortcut = "") const;
			bool IsDisabled(const std::string_view& Option, const std::string_view& Shortcut = "") const;
			bool Has(const std::string_view& Option, const std::string_view& Shortcut = "") const;
			String& Get(const std::string_view& Option, const std::string_view& Shortcut = "");
			String& GetIf(const std::string_view& Option, const std::string_view& Shortcut, const std::string_view& WhenEmpty);

		private:
			bool IsTrue(const std::string_view& Value) const;
			bool IsFalse(const std::string_view& Value) const;
		};

		class VI_OUT_TS Var
		{
		public:
			class VI_OUT Set
			{
			public:
				static Unique<Schema> Auto(Variant&& Value);
				static Unique<Schema> Auto(const Variant& Value);
				static Unique<Schema> Auto(const std::string_view& Value, bool Strict = false);
				static Unique<Schema> Null();
				static Unique<Schema> Undefined();
				static Unique<Schema> Object();
				static Unique<Schema> Array();
				static Unique<Schema> Pointer(void* Value);
				static Unique<Schema> String(const std::string_view& Value);
				static Unique<Schema> Binary(const std::string_view& Value);
				static Unique<Schema> Binary(const uint8_t* Value, size_t Size);
				static Unique<Schema> Integer(int64_t Value);
				static Unique<Schema> Number(double Value);
				static Unique<Schema> Decimal(const Core::Decimal& Value);
				static Unique<Schema> Decimal(Core::Decimal&& Value);
				static Unique<Schema> DecimalString(const std::string_view& Value);
				static Unique<Schema> Boolean(bool Value);
			};

		public:
			static Variant Auto(const std::string_view& Value, bool Strict = false);
			static Variant Null();
			static Variant Undefined();
			static Variant Object();
			static Variant Array();
			static Variant Pointer(void* Value);
			static Variant String(const std::string_view& Value);
			static Variant Binary(const std::string_view& Value);
			static Variant Binary(const uint8_t* Value, size_t Size);
			static Variant Integer(int64_t Value);
			static Variant Number(double Value);
			static Variant Decimal(const Core::Decimal& Value);
			static Variant Decimal(Core::Decimal&& Value);
			static Variant DecimalString(const std::string_view& Value);
			static Variant Boolean(bool Value);
		};

		class VI_OUT_TS OS
		{
		public:
			class VI_OUT CPU
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

			public:
				static QuantityInfo GetQuantityInfo();
				static CacheInfo GetCacheInfo(uint32_t level);
				static Arch GetArch() noexcept;
				static Endian GetEndianness() noexcept;
				static size_t GetFrequency() noexcept;

			public:
				template <typename T>
				static typename std::enable_if<std::is_arithmetic<T>::value, T>::type SwapEndianness(T Value)
				{
					union U
					{
						T Numeric;
						uint8_t Buffer[sizeof(T)];
					} From, To;

					From.Numeric = Value;
					std::reverse_copy(std::begin(From.Buffer), std::end(From.Buffer), std::begin(To.Buffer));
					return To.Numeric;
				}
				template <typename T>
				static typename std::enable_if<std::is_arithmetic<T>::value, T>::type ToEndianness(Endian Type, T Value)
				{
					return GetEndianness() == Type ? Value : SwapEndianness(Value);
				}
			};

			class VI_OUT Directory
			{
			public:
				static bool IsExists(const std::string_view& Path);
				static bool IsEmpty(const std::string_view& Path);
				static ExpectsIO<void> SetWorking(const std::string_view& Path);
				static ExpectsIO<void> Patch(const std::string_view& Path);
				static ExpectsIO<void> Scan(const std::string_view& Path, Vector<std::pair<String, FileEntry>>& Entries);
				static ExpectsIO<void> Create(const std::string_view& Path);
				static ExpectsIO<void> Remove(const std::string_view& Path);
				static ExpectsIO<String> GetModule();
				static ExpectsIO<String> GetWorking();
				static Vector<String> GetMounts();
			};

			class VI_OUT File
			{
			public:
				static bool IsExists(const std::string_view& Path);
				static int Compare(const std::string_view& FirstPath, const std::string_view& SecondPath);
				static uint64_t GetHash(const std::string_view& Data);
				static uint64_t GetIndex(const std::string_view& Data);
				static ExpectsIO<size_t> Write(const std::string_view& Path, const uint8_t* Data, size_t Size);
				static ExpectsIO<void> Move(const std::string_view& From, const std::string_view& To);
				static ExpectsIO<void> Copy(const std::string_view& From, const std::string_view& To);
				static ExpectsIO<void> Remove(const std::string_view& Path);
				static ExpectsIO<void> Close(Unique<FILE> Stream);
				static ExpectsIO<void> GetState(const std::string_view& Path, FileEntry* Output);
				static ExpectsIO<void> Seek64(FILE* Stream, int64_t Offset, FileSeek Mode);
				static ExpectsIO<uint64_t> Tell64(FILE* Stream);
				static ExpectsIO<size_t> Join(const std::string_view& To, const Vector<String>& Paths);
				static ExpectsIO<FileState> GetProperties(const std::string_view& Path);
				static ExpectsIO<FileEntry> GetState(const std::string_view& Path);
				static ExpectsIO<Unique<Stream>> OpenJoin(const std::string_view& Path, const Vector<String>& Paths);
				static ExpectsIO<Unique<Stream>> OpenArchive(const std::string_view& Path, size_t UnarchivedMaxSize = 128 * 1024 * 1024);
				static ExpectsIO<Unique<Stream>> Open(const std::string_view& Path, FileMode Mode, bool Async = false);
				static ExpectsIO<Unique<FILE>> Open(const std::string_view& Path, const std::string_view& Mode);
				static ExpectsIO<Unique<uint8_t>> ReadChunk(Stream* Stream, size_t Length);
				static ExpectsIO<Unique<uint8_t>> ReadAll(const std::string_view& Path, size_t* ByteLength);
				static ExpectsIO<Unique<uint8_t>> ReadAll(Stream* Stream, size_t* ByteLength);
				static ExpectsIO<String> ReadAsString(const std::string_view& Path);
				static ExpectsIO<Vector<String>> ReadAsArray(const std::string_view& Path);

			public:
				template <size_t Size>
				static constexpr uint64_t GetIndex(const char Source[Size]) noexcept
				{
					uint64_t Result = 0xcbf29ce484222325;
					for (size_t i = 0; i < Size; i++)
					{
						Result ^= Source[i];
						Result *= 1099511628211;
					}

					return Result;
				}
			};

			class VI_OUT Path
			{
			public:
				static bool IsRemote(const std::string_view& Path);
				static bool IsRelative(const std::string_view& Path);
				static bool IsAbsolute(const std::string_view& Path);
				static std::string_view GetFilename(const std::string_view& Path);
				static std::string_view GetExtension(const std::string_view& Path);
				static String GetDirectory(const std::string_view& Path, size_t Level = 0);
				static String GetNonExistant(const std::string_view& Path);
				static ExpectsIO<String> Resolve(const std::string_view& Path);
				static ExpectsIO<String> Resolve(const std::string_view& Path, const std::string_view& Directory, bool EvenIfExists);
				static ExpectsIO<String> ResolveDirectory(const std::string_view& Path);
				static ExpectsIO<String> ResolveDirectory(const std::string_view& Path, const std::string_view& Directory, bool EvenIfExists);
			};

			class VI_OUT Net
			{
			public:
				static bool GetETag(char* Buffer, size_t Length, FileEntry* Resource);
				static bool GetETag(char* Buffer, size_t Length, int64_t LastModified, size_t ContentLength);
				static socket_t GetFd(FILE* Stream);
			};

			class VI_OUT Process
			{
			public:
				static void Abort();
				static void Interrupt();
				static void Exit(int Code);
				static bool RaiseSignal(Signal Type);
				static bool BindSignal(Signal Type, SignalCallback Callback);
				static bool RebindSignal(Signal Type);
				static void* BindCheckpoint(Signal Type, SignalCallback Callback);
				static bool RaiseCheckpoint(void** Checkpoint);
				static bool ClearCheckpoint(void** Checkpoint);
				static int GetSignalId(Signal Type);
				static ExpectsIO<int> Execute(const std::string_view& Command, FileMode Mode, ProcessCallback&& Callback);
				static ExpectsIO<Unique<ProcessStream>> Spawn(const std::string_view& Command, FileMode Mode);
				static ExpectsIO<String> GetEnv(const std::string_view& Name);
				static ExpectsIO<String> GetShell();
				static String GetThreadId(const std::thread::id& Id);
				static InlineArgs ParseArgs(int Argc, char** Argv, size_t FormatOpts, const UnorderedSet<String>& Flags = { });
			};

			class VI_OUT Symbol
			{
			public:
				static ExpectsIO<Unique<void>> Load(const std::string_view& Path = "");
				static ExpectsIO<Unique<void>> LoadFunction(void* Handle, const std::string_view& Name);
				static ExpectsIO<void> Unload(void* Handle);
			};

			class VI_OUT Error
			{
			public:
				static int Get(bool Clear = true);
				static void Clear();
				static bool Occurred();
				static bool IsError(int Code);
				static std::error_condition GetCondition();
				static std::error_condition GetCondition(int Code);
				static std::error_condition GetConditionOr(std::errc Code = std::errc::invalid_argument);
				static String GetName(int Code);
			};

			class VI_OUT Control
			{
			private:
				static std::atomic<uint64_t> Options;

			public:
				static void Set(AccessOption Option, bool Enabled);
				static bool Has(AccessOption Option);
				static Option<AccessOption> ToOption(const std::string_view& Option);
				static std::string_view ToString(AccessOption Option);
				static std::string_view ToOptions();
			};
		};

		class VI_OUT_TS Composer : public Singletonish
		{
		private:
			struct State
			{
				UnorderedMap<uint64_t, std::pair<uint64_t, void*>> Factory;
				std::mutex Mutex;
			};

		private:
			static State* Context;

		public:
			static UnorderedSet<uint64_t> Fetch(uint64_t Id) noexcept;
			static bool Pop(const std::string_view& Hash) noexcept;
			static void Cleanup() noexcept;

		private:
			static void Push(uint64_t TypeId, uint64_t Tag, void* Callback) noexcept;
			static void* Find(uint64_t TypeId) noexcept;

		public:
			template <typename T, typename... Args>
			static Unique<T> Create(const std::string_view& Hash, Args... Data) noexcept
			{
				return Create<T, Args...>(VI_HASH(Hash), Data...);
			}
			template <typename T, typename... Args>
			static Unique<T> Create(uint64_t Id, Args... Data) noexcept
			{
				void* (*Callable)(Args...) = nullptr;
				reinterpret_cast<void*&>(Callable) = Find(Id);

				if (!Callable)
					return nullptr;

				return (T*)Callable(Data...);
			}
			template <typename T, typename... Args>
			static void Push(uint64_t Tag) noexcept
			{
				auto Callable = &Composer::Callee<T, Args...>;
				void* Result = reinterpret_cast<void*&>(Callable);
				Push(T::GetTypeId(), Tag, Result);
			}

		private:
			template <typename T, typename... Args>
			static Unique<void> Callee(Args... Data) noexcept
			{
				return (void*)new T(Data...);
			}
		};

		template <typename T>
		class Reference
		{
		private:
			std::atomic<uint32_t> __vcnt;
			std::atomic<uint32_t> __vmrk;

		public:
			Reference() noexcept : __vcnt(1), __vmrk(0)
			{
			}
			Reference(const Reference&) noexcept : Reference()
			{
			}
			Reference(Reference&&) noexcept : Reference()
			{
			}
			Reference& operator=(const Reference&) noexcept
			{
				return *this;
			}
			Reference& operator=(Reference&&) noexcept
			{
				return *this;
			}
			~Reference() = default;
			bool IsMarkedRef() const noexcept
			{
				return __vmrk.load() > 0;
			}
			uint32_t GetRefCount() const noexcept
			{
				return __vcnt.load();
			}
			void MarkRef() noexcept
			{
				__vmrk = 1;
			}
			void AddRef() noexcept
			{
				VI_ASSERT(__vcnt < std::numeric_limits<uint32_t>::max(), "too many references to an object at address 0x%" PRIXPTR " as %s at %s()", (void*)this, typeid(T).name(), __func__);
				++__vcnt;
			}
			void Release() noexcept
			{
				VI_ASSERT(__vcnt > 0, "address at 0x%" PRIXPTR " has already been released as %s at %s()", (void*)this, typeid(T).name(), __func__);
				if (!--__vcnt)
				{
					VI_ASSERT(Memory::IsValidAddress((void*)(T*)this), "address at 0x%" PRIXPTR " is not a valid heap address as %s at %s()", (void*)this, typeid(T).name(), __func__);
					delete (T*)this;
				}
				else
					__vmrk = 0;
			}

		public:
#ifndef NDEBUG
#ifdef VI_CXX20
			void* operator new(size_t Size, const std::source_location& Location = std::source_location::current()) noexcept
			{
				return static_cast<void*>(Memory::Allocate<T>(Size, Location));
			}
			void operator delete(void* Address, const std::source_location& Location) noexcept
			{
				VI_ASSERT(false, "illegal usage of no-op delete operator usable only by compiler");
			}
#else
			void* operator new(size_t Size) noexcept
			{
				return Memory::TracingAllocate(Size, MemoryLocation(__FILE__, __func__, typeid(T).name(), __LINE__));
			}
#endif
			void operator delete(void* Address) noexcept
			{
				auto* Handle = static_cast<T*>(Address);
				VI_ASSERT(!Handle || Handle->__vcnt <= 1, "address at 0x%" PRIXPTR " is still in use but destructor has been called by delete as %s at %s()", Address, typeid(T).name(), __func__);
				Memory::Deallocate<T>(Handle);
			}
#else
			void* operator new(size_t Size) noexcept
			{
				return Memory::DefaultAllocate(Size);
			}
			void operator delete(void* Address) noexcept
			{
				Memory::Deallocate<T>(static_cast<T*>(Address));
			}
#endif
		};

		template <typename T>
		class Singleton : public Reference<T>
		{
		private:
			enum class Action
			{
				Destroy,
				Restore,
				Create,
				Store,
				Fetch
			};

		public:
			Singleton() noexcept
			{
				UpdateInstance((T*)this, Action::Store);
			}
			virtual ~Singleton() noexcept
			{
				if (UpdateInstance(nullptr, Action::Fetch) == (T*)this)
					UpdateInstance(nullptr, Action::Restore);
			}

		public:
			static bool UnlinkInstance(T* Unlinkable) noexcept
			{
				return (UpdateInstance(nullptr, Action::Fetch) == Unlinkable) && !UpdateInstance(nullptr, Action::Restore);
			}
			static bool LinkInstance()
			{
				return UpdateInstance(nullptr, Action::Create) != nullptr;
			}
			static bool CleanupInstance() noexcept
			{
				return !UpdateInstance(nullptr, Action::Destroy);
			}
			static bool HasInstance() noexcept
			{
				return UpdateInstance(nullptr, Action::Fetch) != nullptr;
			}
			static T* Get() noexcept
			{
				return UpdateInstance(nullptr, Action::Create);
			}

		private:
			template <typename Q>
			static typename std::enable_if<std::is_default_constructible<Q>::value, void>::type CreateInstance(Q*& Instance) noexcept
			{
				if (!Instance)
					Instance = new Q();
			}
			template <typename Q>
			static typename std::enable_if<!std::is_default_constructible<Q>::value, void>::type CreateInstance(Q*& Instance) noexcept
			{
			}
			static T* UpdateInstance(T* Other, Action Type) noexcept
			{
				static T* Instance = nullptr;
				switch (Type)
				{
					case Action::Destroy:
					{
						Memory::Release(Instance);
						Instance = Other;
						return nullptr;
					}
					case Action::Restore:
						Instance = nullptr;
						return nullptr;
					case Action::Create:
						CreateInstance<T>(Instance);
						return Instance;
					case Action::Fetch:
						return Instance;
					case Action::Store:
					{
						if (Instance != Other)
							Memory::Release(Instance);
						Instance = Other;
						return Instance;
					}
					default:
						return nullptr;
				}
			}
		};

		template <typename T>
		class UPtr
		{
		private:
			T* Address;

		public:
			UPtr() noexcept : Address(nullptr)
			{
			}
			UPtr(T* NewAddress) noexcept : Address(NewAddress)
			{
			}
			template <typename E>
			explicit UPtr(const Expects<T*, E>& MayBeAddress) noexcept : Address(MayBeAddress ? *MayBeAddress : nullptr)
			{
			}
			explicit UPtr(const Option<T*>& MayBeAddress) noexcept : Address(MayBeAddress ? *MayBeAddress : nullptr)
			{
			}
			UPtr(const UPtr&) noexcept = delete;
			UPtr(UPtr&& Other) noexcept : Address(Other.Address)
			{
				Other.Address = nullptr;
			}
			~UPtr()
			{
				Destroy();
			}
			UPtr& operator= (const UPtr&) noexcept = delete;
			UPtr& operator= (UPtr&& Other) noexcept
			{
				if (this == &Other)
					return *this;

				Destroy();
				Address = Other.Address;
				Other.Address = nullptr;
				return *this;
			}
			explicit operator bool() const
			{
				return Address != nullptr;
			}
			inline T* operator-> ()
			{
				VI_ASSERT(Address != nullptr, "unique null pointer access");
				return Address;
			}
			inline T* operator-> () const
			{
				VI_ASSERT(Address != nullptr, "unique null pointer access");
				return Address;
			}
			inline T* operator* ()
			{
				return Address;
			}
			inline T* operator* () const
			{
				return Address;
			}
			inline T** Out()
			{
				VI_ASSERT(!Address, "pointer should be null when performing output update");
				return &Address;
			}
			inline T* const* In() const
			{
				return &Address;
			}
			inline T* Expect(const std::string_view& Message)
			{
				VI_PANIC(Address != nullptr, "%.*s CAUSING panic", (int)Message.size(), Message.data());
				return Address;
			}
			inline T* Expect(const std::string_view& Message) const
			{
				VI_PANIC(Address != nullptr, "%.*s CAUSING panic", (int)Message.size(), Message.data());
				return Address;
			}
			inline Unique<T> Reset()
			{
				T* Result = Address;
				Address = nullptr;
				return Result;
			}
			inline void Destroy()
			{
				if constexpr (!IsReleaser<T>::value)
				{
					if constexpr (std::is_trivially_default_constructible<T>::value)
						Memory::Deallocate<T>(Address);
					else
						Memory::Delete<T>(Address);
				}
				else
					Memory::Release<T>(Address);
			}
		};

		template <typename T>
		class URef
		{
			static_assert(IsAddRefer<T>::value, "unique reference type should have AddRef() method");
			static_assert(IsReleaser<T>::value, "unique reference type should have Release() method");

		private:
			T* Address;

		public:
			URef() noexcept : Address(nullptr)
			{
			}
			URef(T* NewAddress) noexcept : Address(NewAddress)
			{
			}
			template <typename E>
			explicit URef(const Expects<T*, E>& MayBeAddress) noexcept : Address(MayBeAddress ? *MayBeAddress : nullptr)
			{
			}
			explicit URef(const Option<T*>& MayBeAddress) noexcept : Address(MayBeAddress ? *MayBeAddress : nullptr)
			{
			}
			URef(const URef& Other) noexcept : Address(Other.Address)
			{
				if (Address != nullptr)
					Address->AddRef();
			}
			URef(URef&& Other) noexcept : Address(Other.Address)
			{
				Other.Address = nullptr;
			}
			~URef()
			{
				Destroy();
			}
			URef& operator= (const URef& Other) noexcept
			{
				if (this == &Other)
					return *this;

				Destroy();
				Address = Other.Address;
				if (Address != nullptr)
					Address->AddRef();
				return *this;
			}
			URef& operator= (URef&& Other) noexcept
			{
				if (this == &Other)
					return *this;

				Destroy();
				Address = Other.Address;
				Other.Address = nullptr;
				return *this;
			}
			explicit operator bool() const
			{
				return Address != nullptr;
			}
			inline T* operator-> ()
			{
				VI_ASSERT(Address != nullptr, "unique null pointer access");
				return Address;
			}
			inline T* operator-> () const
			{
				VI_ASSERT(Address != nullptr, "unique null pointer access");
				return Address;
			}
			inline T* operator* ()
			{
				return Address;
			}
			inline T* operator* () const
			{
				return Address;
			}
			inline T** Out()
			{
				VI_ASSERT(!Address, "pointer should be null when performing output update");
				return &Address;
			}
			inline T* const* In() const
			{
				return &Address;
			}
			inline T* Expect(const std::string_view& Message)
			{
				VI_PANIC(Address != nullptr, "%.*s CAUSING panic", (int)Message.size(), Message.data());
				return Address;
			}
			inline T* Expect(const std::string_view& Message) const
			{
				VI_PANIC(Address != nullptr, "%.*s CAUSING panic", (int)Message.size(), Message.data());
				return Address;
			}
			inline Unique<T> Reset()
			{
				T* Result = Address;
				Address = nullptr;
				return Result;
			}
			inline void Destroy()
			{
				Memory::Release<T>(Address);
			}
		};

		template <typename T>
		class UMutex
		{
		private:
#ifndef NDEBUG
#ifdef VI_CXX20
			std::source_location Location;
#endif
			int64_t Diff;
#endif
			T& Mutex;
			bool Owns;

		public:
			UMutex(const UMutex&) noexcept = delete;
			UMutex(UMutex&&) noexcept = delete;
			~UMutex()
			{
#ifndef NDEBUG
				Unlock();
#else
				if (Owns)
					Mutex.unlock();
#endif
			}
			UMutex& operator= (const UMutex&) noexcept = delete;
			UMutex& operator= (UMutex&&) noexcept = delete;
#ifndef NDEBUG
#ifdef VI_CXX20
			UMutex(T& NewMutex, std::source_location&& NewLocation = std::source_location::current()) noexcept : Location(std::move(NewLocation)), Diff(0), Mutex(NewMutex), Owns(false)
			{
				Lock();
			}
#else
			UMutex(T& NewMutex) noexcept : Diff(0), Mutex(NewMutex), Owns(false)
			{
				Lock();
			}
#endif
			inline void Negate()
			{
				if (Owns)
					Unlock();
				else
					Lock();
			}
			inline void Lock()
			{
				if (Owns)
					return;

				auto Time = std::chrono::system_clock::now().time_since_epoch();
				Owns = true;
				Mutex.lock();
				Diff = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch() - Time).count();
			}
			inline void Unlock()
			{
				if (!Owns)
					return;

				Mutex.unlock();
				Owns = false;
				if (Diff <= (int64_t)Timings::Pass * 1000000)
					return;
#ifdef VI_CXX20
				VI_WARN("[stall] %s:%i mutex lock() took %" PRIu64 " ms (%" PRIu64 " ns, expected < %" PRIu64 " ms)", OS::Path::GetFilename(Location.file_name()).data(), (int)Location.line(), Diff / 1000000, Diff, (uint64_t)Timings::Pass);
#else
				VI_WARN("[stall] mutex lock() took %" PRIu64 " ms (%" PRIu64 " ns, expected < %" PRIu64 " ms)", Diff / 1000000, Diff, (uint64_t)Timings::Pass);
#endif
			}
#else
			UMutex(T& NewMutex) noexcept : Mutex(NewMutex), Owns(true)
			{
				Mutex.lock();
			}
			inline void Negate()
			{
				Owns = !Owns;
				if (Owns)
					Mutex.lock();
				else
					Mutex.unlock();
			}
			inline void Lock()
			{
				if (!Owns)
				{
					Owns = true;
					Mutex.lock();
				}
			}
			inline void Unlock()
			{
				if (Owns)
				{
					Mutex.unlock();
					Owns = false;
				}
			}
#endif
		};

		template <typename T>
		class UAlloc
		{
			static_assert(std::is_base_of<LocalAllocator, T>::value, "unique allocator type should be based on local allocator");

		private:
			T& Allocator;

		public:
			UAlloc(T& NewAllocator) noexcept : Allocator(NewAllocator)
			{
				Memory::SetLocalAllocator(&NewAllocator);
			}
			UAlloc(const UAlloc&) noexcept = delete;
			UAlloc(UAlloc&&) noexcept = delete;
			~UAlloc()
			{
				Memory::SetLocalAllocator(nullptr);
				Allocator.Reset();
			}
			UAlloc& operator= (const UAlloc&) noexcept = delete;
			UAlloc& operator= (UAlloc&&) noexcept = delete;
		};

		class VI_OUT_TS Console final : public Singleton<Console>
		{
		public:
			struct ColorToken
			{
				StdColor Color;
				const char* Token;
				char First;
				size_t Size;

				ColorToken(StdColor BaseColor, const char* Name) : Color(BaseColor), Token(Name), First(Name ? *Name : '\0'), Size(Name ? strlen(Name) : 0)
				{
				}
			};

		private:
			enum class Mode
			{
				Attached,
				Allocated,
				Detached
			};

			struct ElementState
			{
				uint64_t State;
				uint32_t X, Y;
			};

			struct WindowState
			{
				Vector<std::pair<uint64_t, String>> Elements;
				size_t Position = 0;
			};

		private:
			struct
			{
				FILE* Input = nullptr;
				FILE* Output = nullptr;
				FILE* Errors = nullptr;
			} Streams;

			struct
			{
				UnorderedMap<uint64_t, ElementState> Elements;
				UnorderedMap<uint64_t, WindowState> Windows;
				std::recursive_mutex Session;
				unsigned short Attributes = 0;
				Mode Status = Mode::Detached;
				bool Colors = true;
				double Time = 0.0;
				uint64_t Id = 0;
			} State;

			struct
			{
				Vector<ColorToken> Default;
				Vector<ColorToken> Custom;
			} Tokens;

		public:
			Console() noexcept;
			virtual ~Console() noexcept override;
			void Hide();
			void Show();
			void Clear();
			void Attach();
			void Detach();
			void Allocate();
			void Deallocate();
			void Trace(uint32_t MaxFrames = 32);
			void SetColoring(bool Enabled);
			void SetColorTokens(Vector<ColorToken>&& AdditionalTokens);
			void ColorBegin(StdColor Text, StdColor Background = StdColor::Zero);
			void ColorEnd();
			void ColorPrint(StdColor BaseColor, const std::string_view& Buffer);
			void CaptureTime();
			uint64_t CaptureWindow(uint32_t Height);
			void FreeWindow(uint64_t Id, bool RestorePosition);
			void EmplaceWindow(uint64_t Id, const std::string_view& Text);
			uint64_t CaptureElement();
			void ResizeElement(uint64_t Id, uint32_t X);
			void MoveElement(uint64_t Id, uint32_t Y);
			void ReadElement(uint64_t Id, uint32_t* X, uint32_t* Y);
			void FreeElement(uint64_t Id);
			void ReplaceElement(uint64_t Id, const std::string_view& Text);
			void SpinningElement(uint64_t Id, const std::string_view& Label);
			void ProgressElement(uint64_t Id, double Value, double Coverage = 0.8);
			void SpinningProgressElement(uint64_t Id, double Value, double Coverage = 0.8);
			void ClearElement(uint64_t Id);
			void Flush();
			void FlushWrite();
			void WriteSize(uint32_t Width, uint32_t Height);
			void WritePosition(uint32_t X, uint32_t Y);
			void WriteLine(const std::string_view& Line);
			void WriteChar(char Value);
			void Write(const std::string_view& Text);
			void jWrite(Schema* Data);
			void jWriteLine(Schema* Data);
			void fWriteLine(const char* Format, ...);
			void fWrite(const char* Format, ...);
			double GetCapturedTime() const;
			bool ReadScreen(uint32_t* Width, uint32_t* Height, uint32_t* X, uint32_t* Y);
			bool ReadLine(String& Data, size_t Size);
			String Read(size_t Size);
			char ReadChar();

		public:
			template <typename F>
			void Synced(F&& Callback)
			{
				UMutex<std::recursive_mutex> Unique(State.Session);
				Callback(this);
			}

		public:
			static bool IsAvailable();
		};

		class VI_OUT Timer final : public Reference<Timer>
		{
		public:
			typedef std::chrono::microseconds Units;

		public:
			struct Capture
			{
				const char* Name;
				Units Begin = Units(0);
				Units End = Units(0);
				Units Delta = Units(0);
				float Step = 0.0;
			};

		private:
			struct
			{
				Units Begin = Units(0);
				Units When = Units(0);
				Units Delta = Units(0);
				size_t Frame = 0;
			} Timing;

			struct
			{
				Units When = Units(0);
				Units Delta = Units(0);
				Units Sum = Units(0);
				size_t Frame = 0;
				bool InFrame = false;
			} Fixed;

		private:
			SingleQueue<Capture> Captures;
			Units MinDelta = Units(0);
			Units MaxDelta = Units(0);
			float FixedFrames = 0.0f;
			float MaxFrames = 0.0f;

		public:
			Timer() noexcept;
			~Timer() noexcept = default;
			void SetFixedFrames(float Value);
			void SetMaxFrames(float Value);
			void Reset();
			void Begin();
			void Finish();
			void Push(const char* Name = nullptr);
			bool PopIf(float GreaterThan, Capture* Out = nullptr);
			Capture Pop();
			size_t GetFrameIndex() const;
			size_t GetFixedFrameIndex() const;
			float GetMaxFrames() const;
			float GetMinStep() const;
			float GetFrames() const;
			float GetElapsed() const;
			float GetElapsedMills() const;
			float GetStep() const;
			float GetFixedStep() const;
			float GetFixedFrames() const;
			bool IsFixed() const;

		public:
			static float ToSeconds(const Units& Value);
			static float ToMills(const Units& Value);
			static Units ToUnits(float Value);
			static Units Clock();
		};

		class VI_OUT Stream : public Reference<Stream>
		{
		private:
			String VName;
			size_t VSize;

		public:
			Stream() noexcept;
			virtual ~Stream() noexcept = default;
			virtual ExpectsIO<void> Clear() = 0;
			virtual ExpectsIO<void> Open(const std::string_view& File, FileMode Mode) = 0;
			virtual ExpectsIO<void> Close() = 0;
			virtual ExpectsIO<void> Seek(FileSeek Mode, int64_t Offset) = 0;
			virtual ExpectsIO<void> Move(int64_t Offset);
			virtual ExpectsIO<void> Flush() = 0;
			virtual ExpectsIO<size_t> ReadScan(const char* Format, ...) = 0;
			virtual ExpectsIO<size_t> ReadLine(char* Buffer, size_t Length) = 0;
			virtual ExpectsIO<size_t> Read(uint8_t* Buffer, size_t Length) = 0;
			virtual ExpectsIO<size_t> WriteFormat(const char* Format, ...) = 0;
			virtual ExpectsIO<size_t> Write(const uint8_t* Buffer, size_t Length) = 0;
			virtual ExpectsIO<size_t> Tell() = 0;
			virtual socket_t GetReadableFd() const = 0;
			virtual socket_t GetWriteableFd() const = 0;
			virtual void* GetReadable() const = 0;
			virtual void* GetWriteable() const = 0;
			virtual bool IsSized() const = 0;
			void SetVirtualSize(size_t Size);
			void SetVirtualName(const std::string_view& File);
			ExpectsIO<size_t> ReadAll(const std::function<void(uint8_t*, size_t)>& Callback);
			ExpectsIO<size_t> Size();
			size_t VirtualSize() const;
			std::string_view VirtualName() const;

		protected:
			void OpenVirtual(String&& Path);
			void CloseVirtual();
		};

		class VI_OUT MemoryStream final : public Stream
		{
		protected:
			Vector<char> Buffer;
			size_t Offset;
			bool Readable;
			bool Writeable;

		public:
			MemoryStream() noexcept;
			~MemoryStream() noexcept override;
			ExpectsIO<void> Clear() override;
			ExpectsIO<void> Open(const std::string_view& File, FileMode Mode) override;
			ExpectsIO<void> Close() override;
			ExpectsIO<void> Seek(FileSeek Mode, int64_t Offset) override;
			ExpectsIO<void> Flush() override;
			ExpectsIO<size_t> ReadScan(const char* Format, ...) override;
			ExpectsIO<size_t> ReadLine(char* Buffer, size_t Length) override;
			ExpectsIO<size_t> Read(uint8_t* Buffer, size_t Length) override;
			ExpectsIO<size_t> WriteFormat(const char* Format, ...) override;
			ExpectsIO<size_t> Write(const uint8_t* Buffer, size_t Length) override;
			ExpectsIO<size_t> Tell() override;
			socket_t GetReadableFd() const override;
			socket_t GetWriteableFd() const override;
			void* GetReadable() const override;
			void* GetWriteable() const override;
			bool IsSized() const override;

		private:
			char* PrepareBuffer(size_t Size);
		};

		class VI_OUT FileStream final : public Stream
		{
		protected:
			FILE* IoStream;

		public:
			FileStream() noexcept;
			~FileStream() noexcept override;
			ExpectsIO<void> Clear() override;
			ExpectsIO<void> Open(const std::string_view& File, FileMode Mode) override;
			ExpectsIO<void> Close() override;
			ExpectsIO<void> Seek(FileSeek Mode, int64_t Offset) override;
			ExpectsIO<void> Flush() override;
			ExpectsIO<size_t> ReadScan(const char* Format, ...) override;
			ExpectsIO<size_t> ReadLine(char* Buffer, size_t Length) override;
			ExpectsIO<size_t> Read(uint8_t* Buffer, size_t Length) override;
			ExpectsIO<size_t> WriteFormat(const char* Format, ...) override;
			ExpectsIO<size_t> Write(const uint8_t* Buffer, size_t Length) override;
			ExpectsIO<size_t> Tell() override;
			socket_t GetReadableFd() const override;
			socket_t GetWriteableFd() const override;
			void* GetReadable() const override;
			void* GetWriteable() const override;
			bool IsSized() const override;
		};

		class VI_OUT GzStream final : public Stream
		{
		protected:
			void* IoStream;

		public:
			GzStream() noexcept;
			~GzStream() noexcept override;
			ExpectsIO<void> Clear() override;
			ExpectsIO<void> Open(const std::string_view& File, FileMode Mode) override;
			ExpectsIO<void> Close() override;
			ExpectsIO<void> Seek(FileSeek Mode, int64_t Offset) override;
			ExpectsIO<void> Flush() override;
			ExpectsIO<size_t> ReadScan(const char* Format, ...) override;
			ExpectsIO<size_t> ReadLine(char* Buffer, size_t Length) override;
			ExpectsIO<size_t> Read(uint8_t* Buffer, size_t Length) override;
			ExpectsIO<size_t> WriteFormat(const char* Format, ...) override;
			ExpectsIO<size_t> Write(const uint8_t* Buffer, size_t Length) override;
			ExpectsIO<size_t> Tell() override;
			socket_t GetReadableFd() const override;
			socket_t GetWriteableFd() const override;
			void* GetReadable() const override;
			void* GetWriteable() const override;
			bool IsSized() const override;
		};

		class VI_OUT WebStream final : public Stream
		{
		protected:
			UnorderedMap<String, String> Headers;
			Vector<char> Chunk;
			void* OutputStream;
			size_t Offset;
			size_t Length;
			bool Async;

		public:
			WebStream(bool IsAsync) noexcept;
			WebStream(bool IsAsync, UnorderedMap<String, String>&& NewHeaders) noexcept;
			~WebStream() noexcept override;
			ExpectsIO<void> Clear() override;
			ExpectsIO<void> Open(const std::string_view& File, FileMode Mode) override;
			ExpectsIO<void> Close() override;
			ExpectsIO<void> Seek(FileSeek Mode, int64_t Offset) override;
			ExpectsIO<void> Flush() override;
			ExpectsIO<size_t> ReadScan(const char* Format, ...) override;
			ExpectsIO<size_t> ReadLine(char* Buffer, size_t Length) override;
			ExpectsIO<size_t> Read(uint8_t* Buffer, size_t Length) override;
			ExpectsIO<size_t> WriteFormat(const char* Format, ...) override;
			ExpectsIO<size_t> Write(const uint8_t* Buffer, size_t Length) override;
			ExpectsIO<size_t> Tell() override;
			socket_t GetReadableFd() const override;
			socket_t GetWriteableFd() const override;
			void* GetReadable() const override;
			void* GetWriteable() const override;
			bool IsSized() const override;
		};

		class VI_OUT ProcessStream final : public Stream
		{
		private:
			struct
			{
				void* OutputPipe = nullptr;
				void* InputPipe = nullptr;
				void* Process = nullptr;
				void* Thread = nullptr;
				socket_t ProcessId = 0;
				socket_t ThreadId = 0;
				int ErrorExitCode = 0;
			} Internal;

		private:
			FILE* OutputStream;
			int InputFd;
			int ExitCode;

		public:
			ProcessStream() noexcept;
			~ProcessStream() noexcept override;
			ExpectsIO<void> Clear() override;
			ExpectsIO<void> Open(const std::string_view& File, FileMode Mode) override;
			ExpectsIO<void> Close() override;
			ExpectsIO<void> Seek(FileSeek Mode, int64_t Offset) override;
			ExpectsIO<void> Flush() override;
			ExpectsIO<size_t> ReadLine(char* Buffer, size_t Length) override;
			ExpectsIO<size_t> ReadScan(const char* Format, ...) override;
			ExpectsIO<size_t> Read(uint8_t* Buffer, size_t Length) override;
			ExpectsIO<size_t> WriteFormat(const char* Format, ...) override;
			ExpectsIO<size_t> Write(const uint8_t* Buffer, size_t Length) override;
			ExpectsIO<size_t> Tell() override;
			socket_t GetProcessId() const;
			socket_t GetThreadId() const;
			socket_t GetReadableFd() const override;
			socket_t GetWriteableFd() const override;
			void* GetReadable() const override;
			void* GetWriteable() const override;
			bool IsSized() const override;
			bool IsAlive();
			int GetExitCode() const;
		};

		class VI_OUT FileTree final : public Reference<FileTree>
		{
		public:
			Vector<FileTree*> Directories;
			Vector<String> Files;
			String Path;

		public:
			FileTree(const std::string_view& Path) noexcept;
			~FileTree() noexcept;
			void Loop(const std::function<bool(const FileTree*)>& Callback) const;
			const FileTree* Find(const std::string_view& Path) const;
			size_t GetFiles() const;
		};

		class VI_OUT Costate final : public Reference<Costate>
		{
			friend Cocontext;

		private:
			UnorderedSet<Coroutine*> Cached;
			UnorderedSet<Coroutine*> Used;
			std::thread::id Thread;
			Coroutine* Current;
			Cocontext* Master;
			size_t Size;

		public:
			std::condition_variable* ExternalCondition;
			std::mutex* ExternalMutex;

		public:
			Costate(size_t StackSize = STACK_SIZE) noexcept;
			~Costate() noexcept;
			Costate(const Costate&) = delete;
			Costate(Costate&&) = delete;
			Costate& operator= (const Costate&) = delete;
			Costate& operator= (Costate&&) = delete;
			Coroutine* Pop(TaskCallback&& Procedure);
			Coexecution Resume(Coroutine* Routine);
			void Reuse(Coroutine* Routine);
			void Push(Coroutine* Routine);
			void Activate(Coroutine* Routine);
			void Deactivate(Coroutine* Routine);
			void Deactivate(Coroutine* Routine, TaskCallback&& AfterSuspend);
			void Clear();
			bool Dispatch();
			bool Suspend();
			bool HasResumableCoroutines() const;
			bool HasAliveCoroutines() const;
			bool HasCoroutines() const;
			Coroutine* GetCurrent() const;
			size_t GetCount() const;

		private:
			Coexecution Execute(Coroutine* Routine);

		public:
			static Costate* Get();
			static Coroutine* GetCoroutine();
			static bool GetState(Costate** State, Coroutine** Routine);
			static bool IsCoroutine();

		public:
			static void VI_COCALL ExecutionEntry(VI_CODATA);
		};

		class VI_OUT Schema final : public Reference<Schema>
		{
		protected:
			Vector<Schema*>* Nodes;
			Schema* Parent;
			bool Saved;

		public:
			String Key;
			Variant Value;

		public:
			Schema(const Variant& Base) noexcept;
			Schema(Variant&& Base) noexcept;
			~Schema() noexcept;
			UnorderedMap<String, size_t> GetNames() const;
			Vector<Schema*> FindCollection(const std::string_view& Name, bool Deep = false) const;
			Vector<Schema*> FetchCollection(const std::string_view& Notation, bool Deep = false) const;
			Vector<Schema*> GetAttributes() const;
			Vector<Schema*>& GetChilds();
			Schema* Find(const std::string_view& Name, bool Deep = false) const;
			Schema* Fetch(const std::string_view& Notation, bool Deep = false) const;
			Variant FetchVar(const std::string_view& Key, bool Deep = false) const;
			Variant GetVar(size_t Index) const;
			Variant GetVar(const std::string_view& Key) const;
			Variant GetAttributeVar(const std::string_view& Key) const;
			Schema* GetParent() const;
			Schema* GetAttribute(const std::string_view& Key) const;
			Schema* Get(size_t Index) const;
			Schema* Get(const std::string_view& Key) const;
			Schema* Set(const std::string_view& Key);
			Schema* Set(const std::string_view& Key, const Variant& Value);
			Schema* Set(const std::string_view& Key, Variant&& Value);
			Schema* Set(const std::string_view& Key, Unique<Schema> Value);
			Schema* SetAttribute(const std::string_view& Key, const Variant& Value);
			Schema* SetAttribute(const std::string_view& Key, Variant&& Value);
			Schema* Push(const Variant& Value);
			Schema* Push(Variant&& Value);
			Schema* Push(Unique<Schema> Value);
			Schema* Pop(size_t Index);
			Schema* Pop(const std::string_view& Name);
			Unique<Schema> Copy() const;
			bool Rename(const std::string_view& Name, const std::string_view& NewName);
			bool Has(const std::string_view& Name) const;
			bool HasAttribute(const std::string_view& Name) const;
			bool Empty() const;
			bool IsAttribute() const;
			bool IsSaved() const;
			size_t Size() const;
			String GetName() const;
			void Join(Schema* Other, bool AppendOnly);
			void Reserve(size_t Size);
			void Unlink();
			void Clear();
			void Save();

		protected:
			void Allocate();
			void Allocate(const Vector<Schema*>& Other);

		private:
			void Attach(Schema* Root);

		public:
			static void Transform(Schema* Value, const SchemaNameCallback& Callback);
			static void ConvertToXML(Schema* Value, const SchemaWriteCallback& Callback);
			static void ConvertToJSON(Schema* Value, const SchemaWriteCallback& Callback);
			static void ConvertToJSONB(Schema* Value, const SchemaWriteCallback& Callback);
			static String ToXML(Schema* Value);
			static String ToJSON(Schema* Value);
			static Vector<char> ToJSONB(Schema* Value);
			static ExpectsParser<Unique<Schema>> ConvertFromXML(const std::string_view& Buffer);
			static ExpectsParser<Unique<Schema>> ConvertFromJSON(const std::string_view& Buffer);
			static ExpectsParser<Unique<Schema>> ConvertFromJSONB(const SchemaReadCallback& Callback);
			static ExpectsParser<Unique<Schema>> FromXML(const std::string_view& Text);
			static ExpectsParser<Unique<Schema>> FromJSON(const std::string_view& Text);
			static ExpectsParser<Unique<Schema>> FromJSONB(const Vector<char>& Binary);

		private:
			static ExpectsParser<void> ProcessConvertionFromJSONB(Schema* Current, UnorderedMap<size_t, String>* Map, const SchemaReadCallback& Callback);
			static Schema* ProcessConversionFromJSONStringOrNumber(void* Base, bool IsDocument);
			static void ProcessConvertionFromXML(void* Base, Schema* Current);
			static void ProcessConvertionFromJSON(void* Base, Schema* Current);
			static void ProcessConvertionToJSONB(Schema* Current, UnorderedMap<String, size_t>* Map, const SchemaWriteCallback& Callback);
			static void GenerateNamingTable(const Schema* Current, UnorderedMap<String, size_t>* Map, size_t& Index);
		};

		class VI_OUT_TS Schedule final : public Singleton<Schedule>
		{
		public:
			enum class ThreadTask
			{
				Spawn,
				EnqueueTimer,
				EnqueueCoroutine,
				EnqueueTask,
				ConsumeCoroutine,
				ProcessTimer,
				ProcessCoroutine,
				ProcessTask,
				Awake,
				Sleep,
				Despawn
			};

			struct VI_OUT ThreadData
			{
				SingleQueue<TaskCallback> Queue;
				std::condition_variable Notify;
				std::mutex Update;
				std::thread Handle;
				std::thread::id Id;
				Allocators::LinearAllocator Allocator;
				Difficulty Type;
				size_t GlobalIndex;
				size_t LocalIndex;
				bool Daemon;

				ThreadData(Difficulty NewType, size_t PreallocatedSize, size_t NewGlobalIndex, size_t NewLocalIndex, bool IsDaemon) : Allocator(PreallocatedSize), Type(NewType), GlobalIndex(NewGlobalIndex), LocalIndex(NewLocalIndex), Daemon(IsDaemon)
				{
				}
				~ThreadData() = default;
			};

			struct VI_OUT ThreadMessage
			{
				const ThreadData* Thread;
				ThreadTask State;
				size_t Tasks;

				ThreadMessage() : Thread(nullptr), State(ThreadTask::Sleep), Tasks(0)
				{
				}
				ThreadMessage(const ThreadData* NewThread, ThreadTask NewState, size_t NewTasks) : Thread(NewThread), State(NewState), Tasks(NewTasks)
				{
				}
			};

			struct VI_OUT Desc
			{
				size_t Threads[(size_t)Difficulty::Count];
				size_t PreallocatedSize;
				size_t StackSize;
				size_t MaxCoroutines;
				size_t MaxRecycles;
				std::chrono::milliseconds IdleTimeout;
				std::chrono::milliseconds ClockTimeout;
				SpawnerCallback Initialize;
				ActivityCallback Ping;
				bool Parallel;

				Desc();
				Desc(size_t Cores);
			};

		public:
			typedef std::function<void(const ThreadMessage&)> ThreadDebugCallback;

		private:
			struct
			{
				TaskCallback Event;
				Costate* State = nullptr;
			} Dispatcher;

		private:
			Vector<ThreadData*> Threads[(size_t)Difficulty::Count];
			ConcurrentTimeoutQueue* Timeouts = nullptr;
			ConcurrentAsyncQueue* Async = nullptr;
			ConcurrentSyncQueue* Sync = nullptr;
			std::atomic<TaskId> Generation;
			std::mutex Exclusive;
			ThreadDebugCallback Debug;
			Desc Policy;
			bool Terminate;
			bool Enqueue;
			bool Suspended;
			bool Active;

		public:
			Schedule() noexcept;
			virtual ~Schedule() noexcept override;
			TaskId SetInterval(uint64_t Milliseconds, TaskCallback&& Callback);
			TaskId SetTimeout(uint64_t Milliseconds, TaskCallback&& Callback);
			bool SetTask(TaskCallback&& Callback, bool Recyclable = true);
			bool SetCoroutine(TaskCallback&& Callback, bool Recyclable = true);
			bool SetDebugCallback(ThreadDebugCallback&& Callback);
			bool ClearTimeout(TaskId WorkId);
			bool TriggerTimers();
			bool Trigger(Difficulty Type);
			bool Start(const Desc& NewPolicy);
			bool Stop();
			bool Wakeup();
			bool Dispatch();
			bool IsActive() const;
			bool CanEnqueue() const;
			bool HasTasks(Difficulty Type) const;
			bool HasAnyTasks() const;
			bool IsSuspended() const;
			void Suspend();
			void Resume();
			size_t GetThreadGlobalIndex();
			size_t GetThreadLocalIndex();
			size_t GetTotalThreads() const;
			size_t GetThreads(Difficulty Type) const;
			bool HasParallelThreads(Difficulty Type) const;
			const ThreadData* GetThread() const;
			const Desc& GetPolicy() const;

		private:
			const ThreadData* InitializeThread(ThreadData* Source, bool Update) const;
			void InitializeSpawnTrigger();
			bool FastBypassEnqueue(Difficulty Type, TaskCallback&& Callback);
			bool ReportThread(ThreadTask State, size_t Tasks, const ThreadData* Thread);
			bool TriggerThread(Difficulty Type, ThreadData* Thread);
			bool SleepThread(Difficulty Type, ThreadData* Thread);
			bool ThreadActive(ThreadData* Thread);
			bool ChunkCleanup();
			bool PushThread(Difficulty Type, size_t GlobalIndex, size_t LocalIndex, bool IsDaemon);
			bool PopThread(ThreadData* Thread);
			std::chrono::microseconds GetTimeout(std::chrono::microseconds Clock);
			TaskId GetTaskId();

		public:
			static bool IsAvailable(Difficulty Type = Difficulty::Count);
			static std::chrono::microseconds GetClock();
		};

		typedef Vector<UPtr<Schema>> SchemaList;
		typedef UnorderedMap<String, UPtr<Schema>> SchemaArgs;

		template <>
		class UAlloc<Schedule>
		{
		private:
			Schedule::ThreadData* Thread;

		public:
			UAlloc() noexcept : Thread((Schedule::ThreadData*)Schedule::Get()->GetThread())
			{
				VI_PANIC(!Thread, "this thread is not a scheduler thread");
				Memory::SetLocalAllocator(&Thread->Allocator);
			}
			UAlloc(const UAlloc& Other) noexcept = delete;
			UAlloc(UAlloc&& Other) noexcept = delete;
			~UAlloc()
			{
				Memory::SetLocalAllocator(nullptr);
				Thread->Allocator.Reset();
			}
			UAlloc& operator= (const UAlloc& Other) noexcept = delete;
			UAlloc& operator= (UAlloc&& Other) noexcept = delete;
		};

		template <typename T>
		class Pool
		{
			static_assert(std::is_pointer<T>::value, "pool can be used only for pointer types");

		public:
			typedef T* Iterator;

		protected:
			size_t Count, Volume;
			T* Data;

		public:
			Pool() noexcept : Count(0), Volume(0), Data(nullptr)
			{
			}
			Pool(const Pool<T>& Ref) noexcept : Count(0), Volume(0), Data(nullptr)
			{
				if (Ref.Data != nullptr)
					Copy(Ref);
			}
			Pool(Pool<T>&& Ref) noexcept : Count(Ref.Count), Volume(Ref.Volume), Data(Ref.Data)
			{
				Ref.Count = 0;
				Ref.Volume = 0;
				Ref.Data = nullptr;
			}
			~Pool() noexcept
			{
				Release();
			}
			void Release()
			{
				Memory::Deallocate(Data);
				Data = nullptr;
				Count = 0;
				Volume = 0;
			}
			void Reserve(size_t Capacity)
			{
				if (Capacity <= Volume)
					return;

				size_t Size = Capacity * sizeof(T);
				T* NewData = Memory::Allocate<T>(Size);
				memset(NewData, 0, Size);

				if (Data != nullptr)
				{
					memcpy(NewData, Data, Count * sizeof(T));
					Memory::Deallocate(Data);
				}

				Volume = Capacity;
				Data = NewData;
			}
			void Resize(size_t NewSize)
			{
				if (NewSize > Volume)
					Reserve(IncreaseCapacity(NewSize));

				Count = NewSize;
			}
			void Copy(const Pool<T>& Ref)
			{
				if (!Ref.Data)
					return;

				if (!Data || Ref.Count > Count)
				{
					Memory::Deallocate<T>(Data);
					Data = Memory::Allocate<T>(Ref.Volume * sizeof(T));
					memset(Data, 0, Ref.Volume * sizeof(T));
				}

				memcpy(Data, Ref.Data, (size_t)(Ref.Count * sizeof(T)));
				Count = Ref.Count;
				Volume = Ref.Volume;
			}
			void Clear()
			{
				Count = 0;
			}
			Iterator Add(const T& Ref)
			{
				VI_ASSERT(Count < Volume, "pool capacity overflow");
				Data[Count++] = Ref;
				return End() - 1;
			}
			Iterator AddUnbounded(const T& Ref)
			{
				if (Count + 1 >= Volume)
					Reserve(IncreaseCapacity(Count + 1));

				return Add(Ref);
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
				VI_ASSERT(Count > 0, "pool is empty");
				VI_ASSERT((size_t)(It - Data) >= 0 && (size_t)(It - Data) < Count, "iterator ranges out of pool");

				Count--;
				Data[It - Data] = Data[Count];
				return It;
			}
			Iterator Remove(const T& Value)
			{
				Iterator It = Find(Value);
				if (It != End())
					RemoveAt(It);

				return It;
			}
			Iterator At(size_t Index) const
			{
				VI_ASSERT(Index < Count, "index ranges out of pool");
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
			Iterator Begin() const
			{
				return Data;
			}
			Iterator End() const
			{
				return Data + Count;
			}
			Iterator begin() const
			{
				return Data;
			}
			Iterator end() const
			{
				return Data + Count;
			}
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
			T& operator [](size_t Index)
			{
				return *(Data + Index);
			}
			Pool<T>& operator =(const Pool<T>& Ref) noexcept
			{
				if (this == &Ref)
					return *this;

				Copy(Ref);
				return *this;
			}
			Pool<T>& operator =(Pool<T>&& Ref) noexcept
			{
				if (this == &Ref)
					return *this;

				Release();
				Count = Ref.Count;
				Volume = Ref.Volume;
				Data = Ref.Data;
				Ref.Count = 0;
				Ref.Volume = 0;
				Ref.Data = nullptr;
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

		private:
			size_t IncreaseCapacity(size_t NewSize)
			{
				size_t Alpha = Volume > 4 ? (Volume + Volume / 2) : 8;
				return Alpha > NewSize ? Alpha : NewSize;
			}
		};

		struct VI_OUT_TS ParallelExecutor
		{
			inline void operator()(TaskCallback&& Callback, bool Async)
			{
				if (Async && Schedule::IsAvailable())
					Schedule::Get()->SetTask(std::move(Callback));
				else
					Callback();
			}
		};

		template <typename T>
		struct PromiseState
		{
			std::mutex Update;
			TaskCallback Event;
			alignas(T) char Value[sizeof(T)];
			std::atomic<uint32_t> Count;
			std::atomic<Deferred> Code;

			PromiseState() noexcept : Count(1), Code(Deferred::Pending)
			{
			}
			PromiseState(const T& NewValue) noexcept : Count(1), Code(Deferred::Ready)
			{
				OptionUtils::CopyBuffer<T>(Value, (const char*)&NewValue, sizeof(T));
			}
			PromiseState(T&& NewValue) noexcept : Count(1), Code(Deferred::Ready)
			{
				OptionUtils::MoveBuffer<T>(Value, (char*)&NewValue, sizeof(T));
			}
			~PromiseState()
			{
				if (Code == Deferred::Ready)
					((T*)Value)->~T();
			}
			void Emplace(const T& NewValue)
			{
				VI_ASSERT(Code != Deferred::Ready, "emplacing to already initialized memory is not desired");
				OptionUtils::CopyBuffer<T>(Value, (const char*)&NewValue, sizeof(T));
			}
			void Emplace(T&& NewValue)
			{
				VI_ASSERT(Code != Deferred::Ready, "emplacing to already initialized memory is not desired");
				OptionUtils::MoveBuffer<T>(Value, (char*)&NewValue, sizeof(T));
			}
			T& Unwrap()
			{
				VI_PANIC(Code == Deferred::Ready, "unwrapping uninitialized memory will result in an undefined behaviour");
				return *(T*)Value;
			}
		};

		template <>
		struct PromiseState<void>
		{
			std::mutex Update;
			TaskCallback Event;
			std::atomic<uint32_t> Count;
			std::atomic<Deferred> Code;

			PromiseState() noexcept : Count(1), Code(Deferred::Pending)
			{
			}
		};

		template <typename T, typename Executor>
		class BasicPromise
		{
		public:
			typedef PromiseState<T> Status;
			typedef T Type;

		private:
			template <typename F>
			struct Unwrap
			{
				typedef typename std::remove_reference<F>::type type;
			};

			template <typename F>
			struct Unwrap<BasicPromise<F, Executor>>
			{
				typedef typename std::remove_reference<F>::type type;
			};

		private:
			Status* Data;

		public:
			BasicPromise() noexcept : Data(Memory::New<Status>())
			{
			}
			BasicPromise(const T& Value) noexcept : Data(Memory::New<Status>(Value))
			{
			}
			BasicPromise(T&& Value) noexcept : Data(Memory::New<Status>(std::move(Value)))
			{
			}
			BasicPromise(const BasicPromise& Other) noexcept : Data(Other.Data)
			{
				AddRef();
			}
			BasicPromise(BasicPromise&& Other) noexcept : Data(Other.Data)
			{
				Other.Data = nullptr;
			}
			~BasicPromise() noexcept
			{
				Release(Data);
			}
			BasicPromise& operator= (const BasicPromise& Other) = delete;
			BasicPromise& operator= (BasicPromise&& Other) noexcept
			{
				if (&Other == this)
					return *this;

				Release(Data);
				Data = Other.Data;
				Other.Data = nullptr;
				return *this;
			}
			void Set(const T& Other) noexcept
			{
				VI_ASSERT(Data != nullptr && Data->Code != Deferred::Ready, "async should be pending");
				UMutex<std::mutex> Unique(Data->Update);
				bool Async = (Data->Code != Deferred::Waiting);
				Data->Emplace(Other);
				Data->Code = Deferred::Ready;
				Execute(Data, Unique, Async);
			}
			void Set(T&& Other) noexcept
			{
				VI_ASSERT(Data != nullptr && Data->Code != Deferred::Ready, "async should be pending");
				UMutex<std::mutex> Unique(Data->Update);
				bool Async = (Data->Code != Deferred::Waiting);
				Data->Emplace(std::move(Other));
				Data->Code = Deferred::Ready;
				Execute(Data, Unique, Async);
			}
			void Set(const BasicPromise& Other) noexcept
			{
				VI_ASSERT(Data != nullptr && Data->Code != Deferred::Ready, "async should be pending");
				Status* Copy = AddRef();
				Other.When([Copy](T&& Value) mutable
				{
					{
						UMutex<std::mutex> Unique(Copy->Update);
						bool Async = (Copy->Code != Deferred::Waiting);
						Copy->Emplace(std::move(Value));
						Copy->Code = Deferred::Ready;
						Execute(Copy, Unique, Async);
					}
					Release(Copy);
				});
			}
			void When(std::function<void(T&&)>&& Callback) const noexcept
			{
				VI_ASSERT(Data != nullptr && Callback, "callback should be set");
				if (!IsPending())
					return Callback(std::move(Data->Unwrap()));

				Status* Copy = AddRef();
				Store([Copy, Callback = std::move(Callback)]() mutable
				{
					Callback(std::move(Copy->Unwrap()));
					Release(Copy);
				});
			}
			void Wait() noexcept
			{
				if (!IsPending())
					return;

				Status* Copy;
				{
					std::unique_lock<std::mutex> Unique(Data->Update);
					if (Data->Code == Deferred::Ready)
						return;

					std::condition_variable Ready;
					Copy = AddRef();
					Copy->Code = Deferred::Waiting;
					Copy->Event = [&Ready]()
					{
						Ready.notify_all();
					};
					Ready.wait(Unique, [this]()
					{
						return !IsPending();
					});
				}
				Release(Copy);
			}
			T&& Get() noexcept
			{
				Wait();
				return Load();
			}
			Deferred GetStatus() const noexcept
			{
				return Data ? Data->Code.load() : Deferred::Ready;
			}
			bool IsPending() const noexcept
			{
				return Data ? Data->Code != Deferred::Ready : false;
			}
			bool IsNull() const noexcept
			{
				return !Data;
			}
			template <typename R>
			BasicPromise<R, Executor> Then(std::function<void(BasicPromise<R, Executor>&, T&&)>&& Callback) const noexcept
			{
				VI_ASSERT(Data != nullptr && Callback, "async should be pending");
				BasicPromise<R, Executor> Result; Status* Copy = AddRef();
				Store([Copy, Result, Callback = std::move(Callback)]() mutable
				{
					Callback(Result, std::move(Copy->Unwrap()));
					Release(Copy);
				});

				return Result;
			}
			BasicPromise<void, Executor> Then(std::function<void(BasicPromise<void, Executor>&, T&&)>&& Callback) const noexcept
			{
				VI_ASSERT(Data != nullptr && Callback, "async should be pending");
				BasicPromise<void, Executor> Result; Status* Copy = AddRef();
				Store([Copy, Result, Callback = std::move(Callback)]() mutable
				{
					Callback(Result, std::move(Copy->Unwrap()));
					Release(Copy);
				});

				return Result;
			}
			template <typename R>
			BasicPromise<typename Unwrap<R>::type, Executor> Then(std::function<R(T&&)>&& Callback) const noexcept
			{
				VI_ASSERT(Data != nullptr && Callback, "async should be pending");
				BasicPromise<typename Unwrap<R>::type, Executor> Result; Status* Copy = AddRef();
				Store([Copy, Result, Callback = std::move(Callback)]() mutable
				{
					Result.Set(std::move(Callback(std::move(Copy->Unwrap()))));
					Release(Copy);
				});

				return Result;
			}
			BasicPromise<void, Executor> Then(std::function<void(T&&)>&& Callback) const noexcept
			{
				VI_ASSERT(Data != nullptr, "async should be pending");
				BasicPromise<void, Executor> Result; Status* Copy = AddRef();
				Store([Copy, Result, Callback = std::move(Callback)]() mutable
				{
					if (Callback)
						Callback(std::move(Copy->Unwrap()));
					Result.Set();
					Release(Copy);
				});

				return Result;
			}

		private:
			BasicPromise(Status* Context, bool) noexcept : Data(Context)
			{
			}
			Status* AddRef() const noexcept
			{
				if (Data != nullptr)
					++Data->Count;
				return Data;
			}
			T&& Load() noexcept
			{
				if (!Data)
					Data = Memory::New<Status>();

				return std::move(Data->Unwrap());
			}
			void Store(TaskCallback&& Callback) const noexcept
			{
				UMutex<std::mutex> Unique(Data->Update);
				Data->Event = std::move(Callback);
				if (Data->Code == Deferred::Ready)
					Execute(Data, Unique, false);
			}

		public:
			static BasicPromise Null() noexcept
			{
				return BasicPromise((Status*)nullptr, false);
			}

		private:
			static void Execute(Status* State, UMutex<std::mutex>& Unique, bool Async) noexcept
			{
				if (State->Event)
				{
					TaskCallback Callback = std::move(State->Event);
					Unique.Negate();
					Executor()(std::move(Callback), Async);
				}
			}
			static void Release(Status* State) noexcept
			{
				if (State != nullptr && !--State->Count)
					Memory::Delete(State);
			}
#ifdef VI_CXX20
		public:
			struct promise_type
			{
				BasicPromise State;

				void return_value(const BasicPromise& NewValue) noexcept
				{
					State.Set(NewValue);
				}
				void return_value(const T& NewValue) noexcept
				{
					State.Set(NewValue);
				}
				void return_value(T&& NewValue) noexcept
				{
					State.Set(std::move(NewValue));
				}
				void unhandled_exception() noexcept
				{
					VI_PANIC(false, "a coroutine has thrown an exception (invalid behaviour)");
				}
				auto get_return_object() noexcept
				{
					return State;
				}
				std::suspend_never initial_suspend() const noexcept
				{
					return { };
				}
				std::suspend_never final_suspend() const noexcept
				{
					return { };
				}
#if !defined(_MSC_VER) || defined(NDEBUG)
				void* operator new(size_t Size) noexcept
				{
					return Memory::Allocate<promise_type>(Size);
				}
				void operator delete(void* Address) noexcept
				{
					Memory::Deallocate<void>(Address);
				}
				static auto get_return_object_on_allocation_failure() noexcept
				{
					return BasicPromise::Null();
				}
#endif // E3394 Intellisense false positive
			};

		public:
			bool await_ready() const noexcept
			{
				return !IsPending();
			}
			T&& await_resume() noexcept
			{
				return Load();
			}
			void await_suspend(std::coroutine_handle<> Handle) noexcept
			{
				if (!IsPending())
					return Handle.resume();

				Status* Copy = AddRef();
#ifndef NDEBUG
				std::chrono::microseconds Time = Schedule::GetClock();
				VI_WATCH(Handle.address(), typeid(Handle).name());
				Store([Copy, Time, Handle]()
				{
					int64_t Diff = (Schedule::GetClock() - Time).count();
					if (Diff > (int64_t)Timings::Hangup * 1000)
						VI_WARN("[stall] async operation took %" PRIu64 " ms (%" PRIu64 " us, expected < %" PRIu64 " ms)", Diff / 1000, Diff, (uint64_t)Timings::Hangup);

					VI_UNWATCH(Handle.address());
					Handle.resume();
					Release(Copy);
				});
#else
				Store([Copy, Handle]()
				{
					Handle.resume();
					Release(Copy);
				});
#endif
			}
#endif
		};

		template <typename Executor>
		class BasicPromise<void, Executor>
		{
		public:
			typedef PromiseState<void> Status;
			typedef void Type;

		private:
			template <typename F>
			struct Unwrap
			{
				typedef F type;
			};

			template <typename F>
			struct Unwrap<BasicPromise<F, Executor>>
			{
				typedef F type;
			};

		private:
			Status* Data;

		public:
			BasicPromise() noexcept : Data(Memory::New<Status>())
			{
			}
			BasicPromise(const BasicPromise& Other) noexcept : Data(Other.Data)
			{
				AddRef();
			}
			BasicPromise(BasicPromise&& Other) noexcept : Data(Other.Data)
			{
				Other.Data = nullptr;
			}
			~BasicPromise() noexcept
			{
				Release(Data);
			}
			BasicPromise& operator= (const BasicPromise& Other) = delete;
			BasicPromise& operator= (BasicPromise&& Other) noexcept
			{
				if (&Other == this)
					return *this;

				Release(Data);
				Data = Other.Data;
				Other.Data = nullptr;
				return *this;
			}
			void Set() noexcept
			{
				VI_ASSERT(Data != nullptr && Data->Code != Deferred::Ready, "async should be pending");
				UMutex<std::mutex> Unique(Data->Update);
				bool Async = (Data->Code != Deferred::Waiting);
				Data->Code = Deferred::Ready;
				Execute(Data, Unique, Async);
			}
			void Set(const BasicPromise& Other) noexcept
			{
				VI_ASSERT(Data != nullptr && Data->Code != Deferred::Ready, "async should be pending");
				Status* Copy = AddRef();
				Other.When([Copy]() mutable
				{
					{
						UMutex<std::mutex> Unique(Copy->Update);
						bool Async = (Copy->Code != Deferred::Waiting);
						Copy->Code = Deferred::Ready;
						Execute(Copy, Unique, Async);
					}
					Release(Copy);
				});
			}
			void When(std::function<void()>&& Callback) const noexcept
			{
				VI_ASSERT(Callback, "callback should be set");
				if (!IsPending())
					return Callback();

				Status* Copy = AddRef();
				Store([Copy, Callback = std::move(Callback)]() mutable
				{
					Callback();
					Release(Copy);
				});
			}
			void Wait() noexcept
			{
				if (!IsPending())
					return;

				Status* Copy;
				{
					std::unique_lock<std::mutex> Lock(Data->Update);
					if (Data->Code == Deferred::Ready)
						return;

					std::condition_variable Ready;
					Copy = AddRef();
					Copy->Code = Deferred::Waiting;
					Copy->Event = [&Ready]()
					{
						Ready.notify_all();
					};
					Ready.wait(Lock, [this]()
					{
						return !IsPending();
					});
				}
				Release(Copy);
			}
			void Get() noexcept
			{
				Wait();
			}
			Deferred GetStatus() const noexcept
			{
				return Data ? Data->Code.load() : Deferred::Ready;
			}
			bool IsPending() const noexcept
			{
				return Data ? Data->Code != Deferred::Ready : false;
			}
			bool IsNull() const noexcept
			{
				return !Data;
			}
			template <typename R>
			BasicPromise<R, Executor> Then(std::function<void(BasicPromise<R, Executor>&)>&& Callback) const noexcept
			{
				VI_ASSERT(Callback, "callback should be set");
				BasicPromise<R, Executor> Result;
				if (Data != nullptr)
				{
					Status* Copy = AddRef();
					Store([Copy, Result, Callback = std::move(Callback)]() mutable
					{
						Callback(Result);
						Release(Copy);
					});
				}
				else
					Callback(Result);

				return Result;
			}
			BasicPromise<void, Executor> Then(std::function<void(BasicPromise<void, Executor>&)>&& Callback) const noexcept
			{
				VI_ASSERT(Callback, "callback should be set");
				BasicPromise<void, Executor> Result;
				if (Data != nullptr)
				{
					Status* Copy = AddRef();
					Store([Copy, Result, Callback = std::move(Callback)]() mutable
					{
						Callback(Result);
						Release(Copy);
					});
				}
				else
					Callback(Result);

				return Result;
			}
			template <typename R>
			BasicPromise<typename Unwrap<R>::type, Executor> Then(std::function<R()>&& Callback) const noexcept
			{
				VI_ASSERT(Callback, "callback should be set");
				BasicPromise<typename Unwrap<R>::type, Executor> Result;
				if (Data != nullptr)
				{
					Status* Copy = AddRef();
					Store([Copy, Result, Callback = std::move(Callback)]() mutable
					{
						Result.Set(std::move(Callback()));
						Release(Copy);
					});
				}
				else
					Result.Set(std::move(Callback()));

				return Result;
			}
			BasicPromise<void, Executor> Then(std::function<void()>&& Callback) const noexcept
			{
				BasicPromise<void, Executor> Result;
				if (!IsPending())
				{
					if (Callback)
						Callback();
					Result.Set();
					return Result;
				}

				Status* Copy = AddRef();
				Store([Copy, Result, Callback = std::move(Callback)]() mutable
				{
					if (Callback)
						Callback();
					Result.Set();
					Release(Copy);
				});

				return Result;
			}

		private:
			BasicPromise(Status* Context, bool) noexcept : Data(Context)
			{
			}
			Status* AddRef() const noexcept
			{
				if (Data != nullptr)
					++Data->Count;
				return Data;
			}
			void Load() noexcept
			{
				if (!Data)
					Data = Memory::New<Status>();
			}
			void Store(TaskCallback&& Callback) const noexcept
			{
				UMutex<std::mutex> Unique(Data->Update);
				Data->Event = std::move(Callback);
				if (Data->Code == Deferred::Ready)
					Execute(Data, Unique, false);
			}

		public:
			static BasicPromise Null() noexcept
			{
				return BasicPromise((Status*)nullptr, false);
			}

		private:
			static void Execute(Status* State, UMutex<std::mutex>& Unique, bool Async) noexcept
			{
				if (State->Event)
				{
					TaskCallback Callback = std::move(State->Event);
					Unique.Negate();
					Executor()(std::move(Callback), Async);
				}
			}
			static void Release(Status* State) noexcept
			{
				if (State != nullptr && !--State->Count)
					Memory::Delete(State);
			}
#ifdef VI_CXX20
		public:
			struct promise_type
			{
				BasicPromise State;

				void return_void() noexcept
				{
					State.Set();
				}
				void unhandled_exception() noexcept
				{
					VI_PANIC(false, "a coroutine has thrown an exception (invalid behaviour)");
				}
				auto get_return_object() noexcept
				{
					return State;
				}
				std::suspend_never initial_suspend() const noexcept
				{
					return { };
				}
				std::suspend_never final_suspend() const noexcept
				{
					return { };
				}
#if !defined(_MSC_VER) || defined(NDEBUG)
				void* operator new(size_t Size) noexcept
				{
					return Memory::Allocate<BasicPromise>(Size);
				}
				void operator delete(void* Address) noexcept
				{
					Memory::Deallocate<void>(Address);
				}
				static auto get_return_object_on_allocation_failure() noexcept
				{
					return BasicPromise::Null();
				}
#endif // E3394 Intellisense false positive
			};

		public:
			bool await_ready() const noexcept
			{
				return !IsPending();
			}
			void await_resume() noexcept
			{
			}
			void await_suspend(std::coroutine_handle<> Handle) noexcept
			{
				if (!IsPending())
					return Handle.resume();

				Status* Copy = AddRef();
#ifndef NDEBUG
				std::chrono::microseconds Time = Schedule::GetClock();
				VI_WATCH(Handle.address(), typeid(Handle).name());
				Store([Copy, Time, Handle]()
				{
					int64_t Diff = (Schedule::GetClock() - Time).count();
					if (Diff > (int64_t)Timings::Hangup * 1000)
						VI_WARN("[stall] async operation took %" PRIu64 " ms (%" PRIu64 " us, expected < %" PRIu64 " ms)", Diff / 1000, Diff, (uint64_t)Timings::Hangup);

					VI_UNWATCH(Handle.address());
					Handle.resume();
					Release(Copy);
				});
#else
				Store([Copy, Handle]()
				{
					Handle.resume();
					Release(Copy);
				});
#endif
			}
#endif
		};

		template <typename T, typename Executor>
		class BasicGenerator
		{
			static_assert(!std::is_same<T, void>::value, "value type should not be void");

		public:
			typedef std::function<BasicPromise<void, Executor>(BasicGenerator&)> ExecutorCallback;

		public:
			struct State
			{
				alignas(T) char Value[sizeof(T)];
				BasicPromise<void, Executor> Input = BasicPromise<void, Executor>::Null();
				BasicPromise<void, Executor> Output = BasicPromise<void, Executor>::Null();
				ExecutorCallback Callback = nullptr;
				std::atomic<uint32_t> Count = 1;
				std::atomic<bool> Exit = false;
				std::atomic<bool> Next = false;
			};

		private:
			State* Status;

		public:
			BasicGenerator(ExecutorCallback&& Callback) noexcept : Status(Memory::New<State>())
			{
				Status->Callback = std::move(Callback);
			}
			BasicGenerator(const BasicGenerator& Other) noexcept : Status(Other.Status)
			{
				if (Status != nullptr)
					++Status->Count;
			}
			BasicGenerator(BasicGenerator&& Other) noexcept : Status(Other.Status)
			{
				Other.Status = nullptr;
			}
			~BasicGenerator() noexcept
			{
				if (Status != nullptr && !--Status->Count)
					Memory::Delete(Status);
			}
			BasicGenerator& operator= (const BasicGenerator& Other) noexcept
			{
				if (this == &Other)
					return *this;

				this->~BasicGenerator();
				Status = Other.Status;
				if (Status != nullptr)
					++Status->Count;

				return *this;
			}
			BasicGenerator& operator= (BasicGenerator&& Other) noexcept
			{
				if (this == &Other)
					return *this;

				this->~BasicGenerator();
				Status = Other.Status;
				Other.Status = nullptr;

				return *this;
			}
			BasicPromise<bool, Executor> Next()
			{
				VI_ASSERT(Status != nullptr, "generator is not valid");
				bool IsEntrypoint = Entrypoint();
				if (Status->Output.IsNull())
					return BasicPromise<bool, Executor>(false);

				if (!IsEntrypoint && Status->Next)
				{
					Status->Next = false;
					Status->Output = BasicPromise<void, Executor>();
					Status->Input.Set();
				}

				return Status->Output.template Then<bool>([this]() -> bool
				{
					return !Status->Exit.load() && Status->Next.load();
				});
			}
			T&& operator()()
			{
				VI_ASSERT(Status != nullptr, "generator is not valid");
				VI_ASSERT(!Status->Exit.load() && Status->Next.load(), "generator does not have a value");
				T& Value = *(T*)Status->Value;
				return std::move(Value);
			}
			BasicPromise<void, Executor> operator<< (const T& Value)
			{
				VI_ASSERT(Status != nullptr, "generator is not valid");
				VI_ASSERT(!Status->Next, "generator already has a value");
				OptionUtils::CopyBuffer<T>(Status->Value, (const char*)&Value, sizeof(T));
				Status->Next = true;
				Status->Input = BasicPromise<void, Executor>();
				Status->Output.Set();
				return Status->Input;
			}
			BasicPromise<void, Executor> operator<< (T&& Value)
			{
				VI_ASSERT(Status != nullptr, "generator is not valid");
				VI_ASSERT(!Status->Next, "generator already has a value");
				OptionUtils::MoveBuffer<T>(Status->Value, (char*)&Value, sizeof(T));
				Status->Next = true;
				Status->Input = BasicPromise<void, Executor>();
				Status->Output.Set();
				return Status->Input;
			}

		private:
			bool Entrypoint()
			{
				if (!Status->Callback)
					return false;

				Status->Output = BasicPromise<void, Executor>();
				auto Generate = [this]()
				{
					ExecutorCallback Callback = std::move(Status->Callback);
					Callback(*this).When([this]()
					{
						Status->Exit = true;
						if (Status->Output.IsPending())
							Status->Output.Set();
					});
				};
				if (Schedule::IsAvailable())
					Schedule::Get()->SetTask(Generate);
				else
					Generate();
				return true;
			}
		};

		template <typename T, typename Executor = ParallelExecutor>
		using Generator = BasicGenerator<T, Executor>;

		template <typename T, typename Executor = ParallelExecutor>
		using Promise = BasicPromise<T, Executor>;

		template <typename T, typename E, typename Executor = ParallelExecutor>
		using ExpectsPromise = BasicPromise<Expects<T, E>, Executor>;

		template <typename T, typename Executor = ParallelExecutor>
		using ExpectsPromiseIO = BasicPromise<ExpectsIO<T>, Executor>;

		template <typename T, typename Executor = ParallelExecutor>
		using ExpectsPromiseSystem = BasicPromise<ExpectsSystem<T>, Executor>;

		template <typename T>
		inline Promise<T> Cotask(std::function<T()>&& Callback, bool Recyclable = true) noexcept
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Schedule::IsAvailable(Difficulty::Sync))
				return Promise<T>(std::move(Callback()));

			Promise<T> Result;
			Schedule::Get()->SetTask([Result, Callback = std::move(Callback)]() mutable { Result.Set(std::move(Callback())); }, Recyclable);
			return Result;
		}
		template <>
		inline Promise<void> Cotask(std::function<void()>&& Callback, bool Recyclable) noexcept
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Schedule::IsAvailable(Difficulty::Sync))
			{
				Callback();
				return Promise<void>::Null();
			}

			Promise<void> Result;
			Schedule::Get()->SetTask([Result, Callback = std::move(Callback)]() mutable
			{
				Callback();
				Result.Set();
			}, Recyclable);
			return Result;
		}
		inline void Codefer(TaskCallback&& Callback, bool AlwaysExecute = true) noexcept
		{
			VI_ASSERT(Callback, "callback should be set");
			if (Schedule::IsAvailable())
				Schedule::Get()->SetTask(std::move(Callback), true);
			else if (AlwaysExecute)
				Callback();
		}
		inline void Cospawn(TaskCallback&& Callback, bool AlwaysExecute = true) noexcept
		{
			VI_ASSERT(Callback, "callback should be set");
			if (Schedule::IsAvailable())
				Schedule::Get()->SetTask(std::move(Callback), false);
			else if (AlwaysExecute)
				Callback();
		}
#ifdef VI_CXX20
		template <typename T>
		inline Promise<T> Coasync(std::function<Promise<T>()>&& Callback, bool AlwaysEnqueue = false) noexcept
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			auto* Callable = Memory::New<std::function<Promise<T>()>>(std::move(Callback));
			auto Finalize = [Callable](T&& Temp) -> T&& { Memory::Delete(Callable); return std::move(Temp); };
			if (!AlwaysEnqueue)
				return (*Callable)().template Then<T>(Finalize);

			Promise<T> Value;
			Schedule::Get()->SetTask([Value, Callable]() mutable { Value.Set((*Callable)()); }, !AlwaysEnqueue);
			return Value.template Then<T>(Finalize);
		}
		template <>
		inline Promise<void> Coasync(std::function<Promise<void>()>&& Callback, bool AlwaysEnqueue) noexcept
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			auto* Callable = Memory::New<std::function<Promise<void>()>>(std::move(Callback));
			auto Finalize = [Callable]() { Memory::Delete(Callable); };
			if (!AlwaysEnqueue)
				return (*Callable)().Then(Finalize);

			Promise<void> Value;
			Schedule::Get()->SetTask([Value, Callable]() mutable { Value.Set((*Callable)()); }, !AlwaysEnqueue);
			return Value.Then(Finalize);
		}
#else
		template <typename T>
		inline T&& Coawait(Promise<T>&& Future, const char* Function = nullptr, const char* Expression = nullptr) noexcept
		{
			if (!Future.IsPending())
				return Future.Get();

			Costate* State; Coroutine* Base;
			Costate::GetState(&State, &Base);
			VI_ASSERT(State != nullptr && Base != nullptr, "cannot call await outside coroutine");
#ifndef NDEBUG
			std::chrono::microseconds Time = Schedule::GetClock();
			if (Function != nullptr && Expression != nullptr)
				VI_WATCH_AT((void*)&Future, Function, Expression);
#endif
			State->Deactivate(Base, [&Future, &State, &Base]()
			{
				Future.When([&State, &Base](T&&) { State->Activate(Base); });
			});
#ifndef NDEBUG
			if (Function != nullptr && Expression != nullptr)
			{
				int64_t Diff = (Schedule::GetClock() - Time).count();
				if (Diff > (int64_t)Timings::Hangup * 1000)
					VI_WARN("[stall] %s(): \"%s\" operation took %" PRIu64 " ms (%" PRIu64 " us, expected < %" PRIu64 " ms)", Function, Expression, Diff / 1000, Diff, (uint64_t)Timings::Hangup);
				VI_UNWATCH((void*)&Future);
			}
#endif
			return Future.Get();
		}
		inline void Coawait(Promise<void>&& Future, const char* Function = nullptr, const char* Expression = nullptr) noexcept
		{
			if (!Future.IsPending())
				return Future.Get();

			Costate* State; Coroutine* Base;
			Costate::GetState(&State, &Base);
			VI_ASSERT(State != nullptr && Base != nullptr, "cannot call await outside coroutine");
#ifndef NDEBUG
			std::chrono::microseconds Time = Schedule::GetClock();
			if (Function != nullptr && Expression != nullptr)
				VI_WATCH_AT((void*)&Future, Function, Expression);
#endif
			State->Deactivate(Base, [&Future, &State, &Base]()
			{
				Future.When([&State, &Base]() { State->Activate(Base); });
			});
#ifndef NDEBUG
			if (Function != nullptr && Expression != nullptr)
			{
				int64_t Diff = (Schedule::GetClock() - Time).count();
				if (Diff > (int64_t)Timings::Hangup * 1000)
					VI_WARN("[stall] %s(): \"%s\" operation took %" PRIu64 " ms (%" PRIu64 " us, expected < %" PRIu64 " ms)", Function, Expression, Diff / 1000, Diff, (uint64_t)Timings::Hangup);
				VI_UNWATCH((void*)&Future);
			}
#endif
			return Future.Get();
		}
		template <typename T>
		inline Promise<T> Coasync(std::function<Promise<T>()>&& Callback, bool AlwaysEnqueue = false) noexcept
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!AlwaysEnqueue && Costate::IsCoroutine())
				return Callback();

			Promise<T> Result;
			Schedule::Get()->SetCoroutine([Result, Callback = std::move(Callback)]() mutable
			{
				Callback().When([Result](T&& Value) mutable { Result.Set(std::move(Value)); });
			}, !AlwaysEnqueue);
			return Result;
		}
		template <>
		inline Promise<void> Coasync(std::function<Promise<void>()>&& Callback, bool AlwaysEnqueue) noexcept
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!AlwaysEnqueue && Costate::IsCoroutine())
				return Callback();

			Promise<void> Result;
			Schedule::Get()->SetCoroutine([Result, Callback = std::move(Callback)]() mutable
			{
				Callback().When([Result]() mutable { Result.Set(); });
			}, !AlwaysEnqueue);
			return Result;
		}
#endif
		template <typename T>
		inline Generator<T> Cogenerate(std::function<Promise<void>(Generator<T>&)>&& Callback)
		{
			return Generator<T>([Callback = std::move(Callback)](Generator<T>& Results) mutable -> Promise<void>
			{
				return Coasync<void>([Results, Callback = std::move(Callback)]() mutable -> Promise<void> { return Callback(Results); });
			});
		}
#ifdef VI_ALLOCATOR
		template <typename O, typename I>
		inline O Copy(const I& Other)
		{
			static_assert(!std::is_same<I, O>::value, "copy should be used to copy object of the same type with different allocator");
			static_assert(IsIterable<I>::value, "input type should be iterable");
			static_assert(IsIterable<O>::value, "output type should be iterable");
			return O(Other.begin(), Other.end());
		}
#else
		template <typename O, typename I>
		inline O&& Copy(I&& Other)
		{
			static_assert(IsIterable<I>::value, "input type should be iterable");
			static_assert(IsIterable<O>::value, "output type should be iterable");
			return std::move(Other);
		}
		template <typename O, typename I>
		inline const O& Copy(const I& Other)
		{
			static_assert(IsIterable<I>::value, "input type should be iterable");
			static_assert(IsIterable<O>::value, "output type should be iterable");
			return Other;
		}
#endif
#ifdef VI_CXX20
		inline const std::string_view& KeyLookupCast(const std::string_view& Value)
		{
			return Value;
		}
#else
		inline String KeyLookupCast(const std::string_view& Value)
		{
			return String(Value);
		}
#endif
		template <typename T>
		inline ExpectsIO<T> FromString(const std::string_view& Other, int Base = 10)
		{
			static_assert(std::is_arithmetic<T>::value, "conversion can be done only to arithmetic types");
			T Value;
			if constexpr (!std::is_integral<T>::value)
			{
#if defined(__clang__) && !defined(VI_CXX20)
				OS::Error::Clear();
				char* End = nullptr;
				if constexpr (!std::is_same<T, long double>::value)
				{
					if constexpr (!std::is_same<T, float>::value)
						Value = strtod(Other.data(), &End);
					else
						Value = strtof(Other.data(), &End);
				}
				else
					Value = strtold(Other.data(), &End);
				if (Other.data() == End || OS::Error::Occurred())
					return OS::Error::GetConditionOr();
#else
				std::from_chars_result Result = std::from_chars(Other.data(), Other.data() + Other.size(), Value, Base == 16 ? std::chars_format::hex : std::chars_format::general);
				if (Result.ec != std::errc())
					return std::make_error_condition(Result.ec);
#endif
			}
			else
			{
				std::from_chars_result Result = std::from_chars(Other.data(), Other.data() + Other.size(), Value, Base);
				if (Result.ec != std::errc())
					return std::make_error_condition(Result.ec);
			}
			return Value;
		}
		template <typename T>
		inline std::string_view ToStringView(char* Buffer, size_t Size, T Other, int Base = 10)
		{
			static_assert(std::is_arithmetic<T>::value, "conversion can be done only from arithmetic types");
			if constexpr (std::is_integral<T>::value)
			{
				std::to_chars_result Result = std::to_chars(Buffer, Buffer + Size, Other, Base);
				return Result.ec == std::errc() ? std::string_view(Buffer, Result.ptr - Buffer) : std::string_view();
			}
			else
			{
				std::to_chars_result Result = std::to_chars(Buffer, Buffer + Size, Other, Base == 16 ? std::chars_format::hex : std::chars_format::fixed);
				return Result.ec == std::errc() ? std::string_view(Buffer, Result.ptr - Buffer) : std::string_view();
			}
		}
		template <typename T>
		inline String ToString(T Other, int Base = 10)
		{
			char Buffer[32];
			return String(ToStringView<T>(Buffer, sizeof(Buffer), Other, Base));
		}
	}
}

using decimal_t = Vitex::Core::Decimal;
#endif