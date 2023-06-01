#ifndef VI_CORE_H
#define VI_CORE_H
#include "../config.hpp"
#include <inttypes.h>
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
#include <array>
#include <list>
#include <sstream>
#ifdef VI_CXX17
#include <charconv>
#endif
#ifdef VI_CXX20
#include <coroutine>
#endif

namespace Mavi
{
	namespace Core
	{
		struct ConcurrentQueue;

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
			STACK_SIZE = (size_t)(512 * 1024),
			CHUNK_SIZE = (size_t)2048,
			BLOB_SIZE = (size_t)8192,
			EVENTS_SIZE = (size_t)32
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

		enum class Coactive
		{
			Active,
			Inactive,
			Resumable
		};

		enum class Difficulty
		{
			Coroutine,
			Light,
			Heavy,
			Clock,
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
			Dated = 1 << 3
		};

		enum class Optional : char
		{
			Error = -1,
			None = 0,
			Value = 1
		};

		template <typename T, typename = void>
		struct IsIterable : std::false_type { };

		template <typename T>
		struct IsIterable<T, std::void_t<decltype(std::begin(std::declval<T&>())), decltype(std::end(std::declval<T&>()))>> : std::true_type
		{
		};

		template <typename T>
		using Unique = T*;

		template <typename T>
		struct Mapping
		{
			T Map;

			~Mapping() = default;
		};

		struct VI_OUT MemoryContext
		{
			const char* Source;
			const char* Function;
			const char* TypeName;
			int Line;

			MemoryContext();
			MemoryContext(const char* NewSource, const char* NewFunction, const char* NewTypeName, int NewLine);
		};

		class VI_OUT_TS Allocator
		{
		public:
			virtual ~Allocator() = default;
			virtual Unique<void> Allocate(MemoryContext&& Origin, size_t Size) noexcept = 0;
			virtual void Transfer(Unique<void> Address, MemoryContext&& Origin, size_t Size) noexcept = 0;
			virtual void Free(Unique<void> Address) noexcept = 0;
			virtual void Watch(MemoryContext&& Origin, void* Address) noexcept = 0;
			virtual void Unwatch(void* Address) noexcept = 0;
			virtual void Finalize() noexcept = 0;
			virtual bool IsValid(void* Address) noexcept = 0;
			virtual bool IsFinalizable() noexcept = 0;
		};

		class VI_OUT_TS DebugAllocator final : public Allocator
		{
		public:
			struct VI_OUT_TS TracingBlock
			{
				std::string TypeName;
				MemoryContext Origin;
				time_t Time;
				size_t Size;
				bool Active;
				bool Static;

				TracingBlock();
				TracingBlock(const char* NewTypeName, MemoryContext&& NewOrigin, time_t NewTime, size_t NewSize, bool IsActive, bool IsStatic);
			};

		private:
			std::unordered_map<void*, TracingBlock> Blocks;
			std::recursive_mutex Mutex;

		public:
			~DebugAllocator() override = default;
			Unique<void> Allocate(MemoryContext&& Origin, size_t Size) noexcept override;
			void Free(Unique<void> Address) noexcept override;
			void Transfer(Unique<void> Address, MemoryContext&& Origin, size_t Size) noexcept override;
			void Watch(MemoryContext&& Origin, void* Address) noexcept override;
			void Unwatch(void* Address) noexcept override;
			void Finalize() noexcept override;
			bool IsValid(void* Address) noexcept override;
			bool IsFinalizable() noexcept override;
			bool Dump(void* Address);
			bool FindBlock(void* Address, TracingBlock* Output);
			const std::unordered_map<void*, TracingBlock>& GetBlocks() const;
		};
		
		class VI_OUT_TS DefaultAllocator final : public Allocator
		{
		public:
			~DefaultAllocator() override = default;
			Unique<void> Allocate(MemoryContext&& Origin, size_t Size) noexcept override;
			void Free(Unique<void> Address) noexcept override;
			void Transfer(Unique<void> Address, MemoryContext&& Origin, size_t Size) noexcept override;
			void Watch(MemoryContext&& Origin, void* Address) noexcept override;
			void Unwatch(void* Address) noexcept override;
			void Finalize() noexcept override;
			bool IsValid(void* Address) noexcept override;
			bool IsFinalizable() noexcept override;
		};

		class VI_OUT_TS PoolAllocator final : public Allocator
		{
		public:
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
			PoolAllocator(uint64_t MinimalLifeTimeMs = 2000, size_t MaxElementsPerAllocation = 1024, size_t ElementsReducingBaseBytes = 300, double ElementsReducingFactorRate = 5.0);
			~PoolAllocator() noexcept override;
			Unique<void> Allocate(MemoryContext&& Origin, size_t Size) noexcept override;
			void Free(Unique<void> Address) noexcept override;
			void Transfer(Unique<void> Address, MemoryContext&& Origin, size_t Size) noexcept override;
			void Watch(MemoryContext&& Origin, void* Address) noexcept override;
			void Unwatch(void* Address) noexcept override;
			void Finalize() noexcept override;
			bool IsValid(void* Address) noexcept override;
			bool IsFinalizable() noexcept override;

		private:
			PageCache* GetPageCache(size_t Size);
			int64_t GetClock();
			size_t GetElementsCount(PageGroup& Page, size_t Size);
		};

		class VI_OUT_TS Memory
		{
		private:
			static std::unordered_map<void*, std::pair<MemoryContext, size_t>>* Allocations;
			static std::mutex* Mutex;
			static Allocator* Base;

		public:
			static Unique<void> Malloc(size_t Size) noexcept;
			static Unique<void> MallocContext(size_t Size, MemoryContext&& Origin) noexcept;
			static void Free(Unique<void> Address) noexcept;
			static void Watch(void* Address, MemoryContext&& Origin) noexcept;
			static void Unwatch(void* Address) noexcept;
			static void SetAllocator(Allocator* NewAllocator);
			static bool IsValidAddress(void* Address);
			static Allocator* GetAllocator();

		private:
			static void Initialize();
			static void Uninitialize();
		};

		template <typename T>
		class AllocationInvoker
		{
		public:
			typedef T value_type;

		public:
			template <typename U>
			struct rebind
			{
				typedef AllocationInvoker<U> other;
			};

		public:
			AllocationInvoker() = default;
			~AllocationInvoker() = default;
			AllocationInvoker(const AllocationInvoker&) = default;
			value_type* allocate(size_t Count)
			{
				return VI_MALLOC(T, Count * sizeof(T));
			}
			value_type* allocate(size_t Count, const void*)
			{
				return VI_MALLOC(T, Count * sizeof(T));
			}
			void deallocate(value_type* Address, size_t Count)
			{
				VI_FREE((void*)Address);
			}
			size_t max_size() const
			{
				return std::numeric_limits<size_t>::max() / sizeof(T);
			}
			bool operator== (const AllocationInvoker&)
			{
				return true;
			}
			bool operator!=(const AllocationInvoker&)
			{
				return false;
			}

		public:
			template <typename U>
			AllocationInvoker(const AllocationInvoker<U>&)
			{
			}
		};

		template <typename T>
		struct AllocationType
		{
#ifdef VI_HAS_FAST_MEMORY
			using type = AllocationInvoker<T>;
#else
			using type = std::allocator<T>;
#endif
		};

		template <typename T, T OffsetBasis, T Prime>
		struct FNV1AHash
		{
			static_assert(std::is_unsigned<T>::value, "Q needs to be unsigned integer");

			T operator()(const void* Address, size_t Size) const noexcept
			{
				const auto Data = static_cast<const unsigned char*>(Address);
				auto State = OffsetBasis;
				for (size_t i = 0; i < Size; ++i)
					State = (State ^ (size_t)Data[i]) * Prime;

				return State;
			}
		};

		template <size_t Bits>
		struct FNV1ABits;

		template <>
		struct FNV1ABits<32>
		{
			using type = FNV1AHash<uint32_t, UINT32_C(2166136261), UINT32_C(16777619)>;
		};

		template <>
		struct FNV1ABits<64>
		{
			using type = FNV1AHash<uint64_t, UINT64_C(14695981039346656037), UINT64_C(1099511628211)>;
		};

		template <size_t Bits>
		using FNV1A = typename FNV1ABits<Bits>::type;

		template <typename T>
		struct Hasher
		{
			typedef float argument_type;
			typedef size_t result_type;

			result_type operator()(const T& Value) const noexcept
			{
				auto Hasher = std::hash<T>();
				return Hasher(Value);
			}
		};

		template <typename T>
		struct EqualTo
		{
			typedef T first_argument_type;
			typedef T second_argument_type;
			typedef bool result_type;

			result_type operator()(const T& Left, const T& Right) const noexcept
			{
				auto Comparator = std::equal_to<T>();
				return Comparator(Left, Right);
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

			result_type operator()(const String& Left, const String& Right) const noexcept
			{
				return Left == Right;
			}
			result_type operator()(const char* Left, const String& Right) const noexcept
			{
				return Right.compare(Left) == 0;
			}
			result_type operator()(const String& Left, const char* Right) const noexcept
			{
				return Left.compare(Right) == 0;
			}
#ifdef VI_CXX17
			result_type operator()(const std::string_view& Left, const String& Right) const noexcept
			{
				return Left == Right;
			}
			result_type operator()(const String& Left, const std::string_view& Right) const noexcept
			{
				return Left == Right;
			}
#endif
		};

		template <>
		struct Hasher<String>
		{
			typedef float argument_type;
			typedef size_t result_type;
			using is_transparent = void;

			result_type operator()(const char* Value) const noexcept
			{
				auto Hasher = FNV1A<8 * sizeof(size_t)>();
				return Hasher(Value, strlen(Value));
			}
			result_type operator()(const String& Value) const noexcept
			{
				auto Hasher = FNV1A<8 * sizeof(size_t)>();
				return Hasher(Value.c_str(), Value.size());
			}
#ifdef VI_CXX17
			result_type operator()(const std::string_view& Value) const noexcept
			{
				auto Hasher = FNV1A<8 * sizeof(size_t)>();
				return Hasher(Value.data(), Value.size());
			}
#endif
		};

		template <>
		struct Hasher<WideString>
		{
			typedef float argument_type;
			typedef size_t result_type;

			result_type operator()(const WideString& Value) const noexcept
			{
				auto Hasher = FNV1A<8 * sizeof(std::size_t)>();
				return Hasher(Value.c_str(), Value.size());
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

		template <typename K, typename Hash = Hasher<K>, typename KeyEqual = EqualTo<K>>
		using UnorderedSet = std::unordered_set<K, Hash, KeyEqual, typename AllocationType<typename std::unordered_set<K>::value_type>::type>;

		template <typename K, typename V, typename Hash = Hasher<K>, typename KeyEqual = EqualTo<K>>
		using UnorderedMap = std::unordered_map<K, V, Hash, KeyEqual, typename AllocationType<typename std::unordered_map<K, V>::value_type>::type>;

		template <typename K, typename V, typename Hash = Hasher<K>, typename KeyEqual = EqualTo<K>>
		using UnorderedMultiMap = std::unordered_multimap<K, V, Hash, KeyEqual, typename AllocationType<typename std::unordered_multimap<K, V>::value_type>::type>;

		template <typename K, typename V, typename Comparator = typename std::map<K, V>::key_compare>
		using OrderedMap = std::map<K, V, Comparator, typename AllocationType<typename std::map<K, V>::value_type>::type>;

		typedef std::function<void()> TaskCallback;
		typedef std::function<void(size_t)> SeqTaskCallback;
		typedef std::function<Core::String(const Core::String&)> SchemaNameCallback;
		typedef std::function<void(VarForm, const char*, size_t)> SchemaWriteCallback;
		typedef std::function<bool(char*, size_t)> SchemaReadCallback;
		typedef std::function<bool()> ActivityCallback;

		class VI_OUT_TS ErrorHandling
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
			static std::function<void(Details&)> Callback;
			static std::mutex Buffer;
			static uint32_t Flags;

		public:
			static void Panic(int Line, const char* Source, const char* Function, const char* Condition, const char* Format, ...);
			static void Assert(int Line, const char* Source, const char* Function, const char* Condition, const char* Format, ...);
			static void Message(LogLevel Level, int Line, const char* Source, const char* Format, ...);
			static void Pause();
			static void SetCallback(const std::function<void(Details&)>& Callback);
			static void SetFlag(LogOption Option, bool Active);
			static bool HasFlag(LogOption Option);
			static Core::String GetStackTrace(size_t Skips, size_t MaxFrames = 16);
			static Core::String GetMeasureTrace();
			static Tick Measure(const char* File, const char* Function, int Line, uint64_t ThresholdMS);
			static void MeasureLoop();
			static const char* GetMessageType(const Details& Base);
			static StdColor GetMessageColor(const Details& Base);
			static Core::String GetMessageText(const Details& Base);

		private:
			static void Enqueue(Details&& Data);
			static void Dispatch(Details& Data);
			static void Colorify(Console* Base, Details& Data);
			static void ColorifyTokens(Console* Base, const char* Buffer, StdColor BaseColor);
		};

		template <typename V>
		class Option
		{
		private:
			char Value[sizeof(V)];
			bool Empty;

		public:
			Option(Optional Other) : Empty(true)
			{
				VI_ASSERT(Other == Optional::None, "only none is accepted for this constructor");
			}
			Option(const V& Other) : Empty(false)
			{
				new (Value) V(Other);
			}
			Option(V&& Other) noexcept : Empty(false)
			{
				new (Value) V(std::move(Other));
			}
			Option(const Option& Other) : Empty(Other.Empty)
			{
				if (!Empty)
					new (Value) V(*(V*)Other.Value);
			}
			Option(Option&& Other) noexcept : Empty(Other.Empty)
			{
				memcpy(Value, Other.Value, sizeof(Value));
				Other.Empty = true;
			}
			~Option()
			{
				if (!Empty)
					((V*)Value)->~V();
			}
			Option& operator= (const V& Other)
			{
				~Option();
				new (Value) V(Other);
				Empty = false;
				return *this;
			}
			Option& operator= (V&& Other) noexcept
			{
				~Option();
				new (Value) V(std::move(Other));
				Empty = false;
				return *this;
			}
			Option& operator= (const Option& Other)
			{
				if (this == &Other)
					return *this;

				~Option();
				Empty = Other.Empty;
				if (!Empty)
					new (Value) V(*(V*)Other.Value);

				return *this;
			}
			Option& operator= (Option&& Other) noexcept
			{
				if (this == &Other)
					return *this;

				~Option();
				memcpy(Value, Other.Value, sizeof(Value));
				Empty = Other.Empty;
				Other.Empty = true;
				return *this;
			}
			const V& OrPanic(const char* Message) const
			{
				VI_ASSERT(Message != nullptr && Message[0] != '\0', "panic case message should be set");
				VI_PANIC(IsValue(), "panic is caused by %s", Message);
				return *(V*)Value;
			}
			V&& OrPanic(const char* Message)
			{
				VI_ASSERT(Message != nullptr && Message[0] != '\0', "panic case message should be set");
				VI_PANIC(IsValue(), "panic is caused by %s", Message);
				return std::move(*(V*)Value);
			}
			const V& operator* () const
			{
				VI_ASSERT(IsValue(), "option does not contain any value");
				return *(V*)Value;
			}
			V&& operator* ()
			{
				VI_ASSERT(IsValue(), "option does not contain any value");
				return std::move(*(V*)Value);
			}
			typename std::remove_pointer<V>::type* operator-> ()
			{
				VI_ASSERT(IsValue(), "option does not contain any value");
				return (typename std::remove_pointer<V>::type*)Value;
			}
			explicit operator bool() const
			{
				return IsValue();
			}
			explicit operator Optional() const
			{
				return IsValue() ? Optional::Value : Optional::None;
			}
			void Reset()
			{
				~Option();
				Empty = true;
			}
			bool IsNone() const
			{
				return Empty;
			}
			bool IsValue() const
			{
				return !Empty;
			}
		};

		template <>
		class Option<void>
		{
		private:
			bool Empty;

		public:
			Option(Optional Value) : Empty(Value == Optional::None)
			{
				VI_ASSERT(Value != Optional::Error, "only none and value are accepted for this constructor");
			}
			Option(const Option&) = default;
			Option(Option&& Other) = default;
			~Option() = default;
			Option& operator= (Optional Value)
			{
				VI_ASSERT(Value != Optional::Error, "only none and value are accepted for this constructor");
				Empty = (Value == Optional::None);
				return *this;
			}
			Option& operator= (const Option&) = default;
			Option& operator= (Option&& Other) = default;
			bool OrPanic(const char* Message) const
			{
				VI_ASSERT(Message != nullptr, "panic case message should be set");
				VI_PANIC(IsValue(), "%s", Message);
				return true;
			}
			bool OrPanic(const char* Message)
			{
				VI_ASSERT(Message != nullptr, "panic case message should be set");
				VI_PANIC(IsValue(), "%s", Message);
				return true;
			}
			explicit operator bool() const
			{
				return IsValue();
			}
			explicit operator Optional() const
			{
				return IsValue() ? Optional::Value : Optional::None;
			}
			void Reset()
			{
				Empty = true;
			}
			bool IsNone() const
			{
				return Empty;
			}
			bool IsValue() const
			{
				return !Empty;
			}
		};

		template <typename V, typename E>
		class Expects
		{
			static_assert(!std::is_same<V, void>::value, "value type should not be void");
			static_assert(!std::is_same<E, void>::value, "error type should not be void");

		private:
			using ValueSize = std::integral_constant<std::size_t, std::max(sizeof(V), sizeof(E))>;
			char Value[ValueSize::value];
			char Status;

		public:
			Expects(const V& Other) : Status(1)
			{
				new (Value) V(Other);
			}
			Expects(V&& Other) noexcept : Status(1)
			{
				new (Value) V(std::move(Other));
			}
			Expects(const E& Other) noexcept : Status(-1)
			{
				new (Value) E(Other);
			}
			Expects(E&& Other) noexcept : Status(-1)
			{
				new (Value) E(std::move(Other));
			}
			Expects(const Expects& Other) : Status(Other.Status)
			{
				if (Status > 0)
					new (Value) V(*(V*)Other.Value);
				else if (Status < 0)
					new (Value) E(*(E*)Other.Value);
			}
			Expects(Expects&& Other) noexcept : Status(Other.Status)
			{
				Other.Status = 0;
				if (Status > 0)
					memcpy(Value, Other.Value, sizeof(V));
				else if (Status < 0)
					memcpy(Value, Other.Value, sizeof(E));
			}
			~Expects()
			{
				if (Status > 0)
					((V*)Value)->~V();
				else if (Status < 0)
					((E*)Value)->~E();
			}
			Expects& operator= (const V& Other)
			{
				~Expects();
				new (Value) V(Other);
				Status = 1;
				return **this;
			}
			Expects& operator= (V&& Other) noexcept
			{
				~Expects();
				new (Value) V(std::move(Other));
				Status = 1;
				return *this;
			}
			Expects& operator= (const Expects& Other)
			{
				if (this == &Other)
					return *this;

				~Expects();
				Status = Other.Status;
				if (Status > 0)
					new (Value) V(*(V*)Other.Value);
				else if (Status < 0)
					new (Value) E(*(E*)Other.Value);

				return *this;
			}
			Expects& operator= (Expects&& Other) noexcept
			{
				if (this == &Other)
					return *this;

				~Expects();
				Status = Other.Status;
				Other.Status = 0;
				if (Status > 0)
					memcpy(Value, Other.Value, sizeof(V));
				else if (Status < 0)
					memcpy(Value, Other.Value, sizeof(E));
				return *this;
			}
			const V& OrPanic(const char* Message) const
			{
				VI_ASSERT(Message != nullptr && Message[0] != '\0', "panic case message should be set");
				VI_PANIC(IsValue(), "%s caused by %s", Message, GetErrorText<E>().c_str());
				return *(V*)Value;
			}
			V&& OrPanic(const char* Message)
			{
				VI_ASSERT(Message != nullptr && Message[0] != '\0', "panic case message should be set");
				VI_PANIC(IsValue(), "%s caused by %s", Message, GetErrorText<E>().c_str());
				return std::move(*(V*)Value);
			}
			const E& Error() const
			{
				VI_ASSERT(IsError(), "outcome does not contain any errors");
				return *(E*)Value;
			}
			E&& Error()
			{
				VI_ASSERT(IsError(), "outcome does not contain any errors");
				return std::move(*(E*)Value);
			}
			Core::String What() const
			{
				VI_ASSERT(!IsValue(), "outcome does not contain any errors");
				return GetErrorText<E>();
			}
			explicit operator bool() const
			{
				return IsValue();
			}
			explicit operator Optional() const
			{
				return (Optional)Status;
			}
			const V& operator* () const
			{
				VI_ASSERT(IsValue(), "error caused by %s", GetErrorText<E>().c_str());
				return *(V*)Value;
			}
			V&& operator* ()
			{
				VI_ASSERT(IsValue(), "error caused by %s", GetErrorText<E>().c_str());
				return std::move(*(V*)Value);
			}
			typename std::remove_pointer<V>::type* operator-> ()
			{
				VI_ASSERT(IsValue(), "error caused by %s", GetErrorText<E>().c_str());
				return (typename std::remove_pointer<V>::type*)Value;
			}
			void Reset()
			{
				~Expects();
				Status = 0;
			}
			bool IsNone() const
			{
				return !Status;
			}
			bool IsValue() const
			{
				return Status > 0;
			}
			bool IsError() const
			{
				return Status < 0;
			}

		private:
			template <typename Q>
			inline typename std::enable_if<std::is_base_of<std::exception, Q>::value, Core::String>::type GetErrorText() const
			{
				return String(IsError() ?((Q*)Value)->what() : "*no value stored*");
			}
			template <typename Q>
			inline typename std::enable_if<std::is_same<std::error_code, Q>::value, std::string>::type GetErrorText() const
			{
				return IsError() ? std::string(((Q*)Value)->message()) : std::string("*no value stored*");
			}
			template <typename Q>
			inline typename std::enable_if<std::is_same<std::error_condition, Q>::value, std::string>::type GetErrorText() const
			{
				return IsError() ? std::string(((Q*)Value)->message()) : std::string("*no value stored*");
			}
			template <typename Q>
			inline typename std::enable_if<std::is_same<std::string, Q>::value, std::string>::type GetErrorText() const
			{
				return IsError() ? *(Q*)Value : Q("*no value stored*");
			}
#ifdef VI_HAS_FAST_MEMORY
			template <typename Q>
			inline typename std::enable_if<std::is_same<Core::String, Q>::value, Core::String>::type GetErrorText() const
			{
				return IsError() ? *(Q*)Value : Q("*no value stored*");
			}
#endif
			template <typename Q>
			inline typename std::enable_if<!std::is_base_of<std::exception, Q>::value && !std::is_same<std::error_code, Q>::value && !std::is_same<std::error_condition, Q>::value && !std::is_same<std::string, Q>::value && !std::is_same<Core::String, Q>::value, Core::String>::type GetErrorText() const
			{
				return String(IsError() ? "*error cannot be shown*" : "*no value stored*");
			}
		};

		struct VI_OUT Coroutine
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
			Coroutine(Costate* Base, const TaskCallback& Procedure) noexcept;
			Coroutine(Costate* Base, TaskCallback&& Procedure) noexcept;
			~Coroutine() noexcept;
		};

		struct VI_OUT Decimal
		{
		private:
			Core::DoubleQueue<char> Source;
			int Length;
			char Sign;
			bool Invalid;

		public:
			Decimal() noexcept;
			Decimal(const char* Value) noexcept;
			Decimal(const Core::String& Value) noexcept;
			Decimal(int32_t Value) noexcept;
			Decimal(uint32_t Value) noexcept;
			Decimal(int64_t Value) noexcept;
			Decimal(uint64_t Value) noexcept;
			Decimal(float Value) noexcept;
			Decimal(double Value) noexcept;
			Decimal(const Decimal& Value) noexcept;
			Decimal(Decimal&& Value) noexcept;
			Decimal& Truncate(int Value);
			Decimal& Round(int Value);
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
			int64_t ToInt64() const;
			uint64_t ToUInt64() const;
			Core::String ToString() const;
			Core::String Exp() const;
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
			explicit operator double () const
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
			explicit operator Core::String() const
			{
				return ToString();
			}

		public:
			VI_OUT friend Decimal operator + (const Decimal& Left, const Decimal& Right);
			VI_OUT friend Decimal operator - (const Decimal& Left, const Decimal& Right);
			VI_OUT friend Decimal operator * (const Decimal& Left, const Decimal& Right);
			VI_OUT friend Decimal operator / (const Decimal& Left, const Decimal& Right);
			VI_OUT friend Decimal operator % (const Decimal& Left, const Decimal& Right);

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
			uint32_t Size;

		public:
			Variant() noexcept;
			Variant(const Variant& Other) noexcept;
			Variant(Variant&& Other) noexcept;
			~Variant() noexcept;
			bool Deserialize(const Core::String& Value, bool Strict = false);
			Core::String Serialize() const;
			Core::String GetBlob() const;
			Decimal GetDecimal() const;
			void* GetPointer() const;
			const char* GetString() const;
			unsigned char* GetBinary() const;
			int64_t GetInteger() const;
			double GetNumber() const;
			bool GetBoolean() const;
			VarType GetType() const;
			size_t GetSize() const;
			Variant& operator= (const Variant& Other) noexcept;
			Variant& operator= (Variant&& Other) noexcept;
			bool operator== (const Variant& Other) const;
			bool operator!= (const Variant& Other) const;
			explicit operator bool() const;
			bool IsString(const char* Value) const;
			bool IsObject() const;
			bool IsEmpty() const;
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

		typedef Core::Vector<Variant> VariantList;
		typedef Core::Vector<Schema*> SchemaList;
		typedef Core::UnorderedMap<Core::String, Variant> VariantArgs;
		typedef Core::UnorderedMap<Core::String, Schema*> SchemaArgs;

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
			SeqTaskCallback Callback;
			Difficulty Type;
			TaskId Id;
			size_t Invocations;
			bool Alive;

			Timeout(const SeqTaskCallback& NewCallback, const std::chrono::microseconds& NewTimeout, TaskId NewId, bool NewAlive, Difficulty NewType) noexcept;
			Timeout(SeqTaskCallback&& NewCallback, const std::chrono::microseconds& NewTimeout, TaskId NewId, bool NewAlive, Difficulty NewType) noexcept;
			Timeout(const Timeout& Other) noexcept;
			Timeout(Timeout&& Other) noexcept;
			Timeout& operator= (const Timeout& Other) noexcept;
			Timeout& operator= (Timeout&& Other) noexcept;
		};

		struct VI_OUT FileEntry
		{
			Core::String Path;
			size_t Size = 0;
			int64_t LastModified = 0;
			int64_t CreationTime = 0;
			bool IsReferenced = false;
			bool IsDirectory = false;
			bool IsExists = false;
		};

		struct VI_OUT ChildProcess
		{
			friend class OS;

		private:
			bool Valid = false;
#ifdef VI_MICROSOFT
			void* Process = nullptr;
			void* Thread = nullptr;
			void* Job = nullptr;
#else
			pid_t Process;
#endif

		public:
			int64_t GetPid();
		};

		struct VI_OUT DateTime
		{
		private:
			std::chrono::system_clock::duration Time;
			struct tm DateValue;
			bool DateRebuild;

		public:
			DateTime() noexcept;
			DateTime(uint64_t Seconds) noexcept;
			DateTime(const DateTime& Value) noexcept;
			DateTime& operator= (const DateTime& Other) noexcept;
			void operator +=(const DateTime& Right);
			void operator -=(const DateTime& Right);
			bool operator >=(const DateTime& Right);
			bool operator <=(const DateTime& Right);
			bool operator >(const DateTime& Right);
			bool operator <(const DateTime& Right);
			bool operator ==(const DateTime& Right);
			Core::String Format(const Core::String& Value);
			Core::String Date(const Core::String& Value);
			Core::String Iso8601();
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
			static Core::String FetchWebDateGMT(int64_t TimeStamp);
			static Core::String FetchWebDateTime(int64_t TimeStamp);
			static bool FetchWebDateGMT(char* Buffer, size_t Length, int64_t Time);
			static bool FetchWebDateTime(char* Buffer, size_t Length, int64_t Time);
			static int64_t ParseWebDate(const char* Date);
		};

		struct VI_OUT Stringify
		{
		public:
			static Core::String& EscapePrint(Core::String& Other);
			static Core::String& Escape(Core::String& Other);
			static Core::String& Unescape(Core::String& Other);
			static Core::String& ToUpper(Core::String& Other);
			static Core::String& ToLower(Core::String& Other);
			static Core::String& Clip(Core::String& Other, size_t Length);
			static Core::String& Compress(Core::String& Other, const char* SpaceIfNotFollowedOrPrecededByOf, const char* NotInBetweenOf, size_t Start = 0U);
			static Core::String& ReplaceOf(Core::String& Other, const char* Chars, const char* To, size_t Start = 0U);
			static Core::String& ReplaceNotOf(Core::String& Other, const char* Chars, const char* To, size_t Start = 0U);
			static Core::String& ReplaceGroups(Core::String& Other, const Core::String& FromRegex, const Core::String& To);
			static Core::String& Replace(Core::String& Other, const Core::String& From, const Core::String& To, size_t Start = 0U);
			static Core::String& Replace(Core::String& Other, const char* From, const char* To, size_t Start = 0U);
			static Core::String& Replace(Core::String& Other, const char& From, const char& To, size_t Position = 0U);
			static Core::String& Replace(Core::String& Other, const char& From, const char& To, size_t Position, size_t Count);
			static Core::String& ReplacePart(Core::String& Other, size_t Start, size_t End, const Core::String& Value);
			static Core::String& ReplacePart(Core::String& Other, size_t Start, size_t End, const char* Value);
			static Core::String& ReplaceStartsWithEndsOf(Core::String& Other, const char* Begins, const char* EndsOf, const Core::String& With, size_t Start = 0U);
			static Core::String& ReplaceInBetween(Core::String& Other, const char* Begins, const char* Ends, const Core::String& With, bool Recursive, size_t Start = 0U);
			static Core::String& ReplaceNotInBetween(Core::String& Other, const char* Begins, const char* Ends, const Core::String& With, bool Recursive, size_t Start = 0U);
			static Core::String& ReplaceParts(Core::String& Other, Core::Vector<std::pair<Core::String, TextSettle>>& Inout, const Core::String& With, const std::function<char(const Core::String&, char, int)>& Surrounding = nullptr);
			static Core::String& ReplaceParts(Core::String& Other, Core::Vector<TextSettle>& Inout, const Core::String& With, const std::function<char(char, int)>& Surrounding = nullptr);
			static Core::String& RemovePart(Core::String& Other, size_t Start, size_t End);
			static Core::String& Reverse(Core::String& Other);
			static Core::String& Reverse(Core::String& Other, size_t Start, size_t End);
			static Core::String& Substring(Core::String& Other, const TextSettle& Result);
			static Core::String& Splice(Core::String& Other, size_t Start, size_t End);
			static Core::String& Trim(Core::String& Other);
			static Core::String& TrimStart(Core::String& Other);
			static Core::String& TrimEnd(Core::String& Other);
			static Core::String& Fill(Core::String& Other, const char& Char);
			static Core::String& Fill(Core::String& Other, const char& Char, size_t Count);
			static Core::String& Fill(Core::String& Other, const char& Char, size_t Start, size_t Count);
			static Core::String& Append(Core::String& Other, const char* Format, ...);
			static Core::String& Erase(Core::String& Other, size_t Position);
			static Core::String& Erase(Core::String& Other, size_t Position, size_t Count);
			static Core::String& EraseOffsets(Core::String& Other, size_t Start, size_t End);
			static Core::String& EvalEnvs(Core::String& Other, const Core::String& Net, const Core::String& Dir);
			static Core::Vector<std::pair<Core::String, TextSettle>> FindInBetween(const Core::String& Other, const char* Begins, const char* Ends, const char* NotInSubBetweenOf, size_t Offset = 0U);
			static Core::Vector<std::pair<Core::String, TextSettle>> FindStartsWithEndsOf(const Core::String& Other, const char* Begins, const char* EndsOf, const char* NotInSubBetweenOf, size_t Offset = 0U);
			static TextSettle ReverseFind(const Core::String& Other, const Core::String& Needle, size_t Offset = 0U);
			static TextSettle ReverseFind(const Core::String& Other, const char* Needle, size_t Offset = 0U);
			static TextSettle ReverseFind(const Core::String& Other, const char& Needle, size_t Offset = 0U);
			static TextSettle ReverseFindUnescaped(const Core::String& Other, const char& Needle, size_t Offset = 0U);
			static TextSettle ReverseFindOf(const Core::String& Other, const Core::String& Needle, size_t Offset = 0U);
			static TextSettle ReverseFindOf(const Core::String& Other, const char* Needle, size_t Offset = 0U);
			static TextSettle Find(const Core::String& Other, const Core::String& Needle, size_t Offset = 0U);
			static TextSettle Find(const Core::String& Other, const char* Needle, size_t Offset = 0U);
			static TextSettle Find(const Core::String& Other, const char& Needle, size_t Offset = 0U);
			static TextSettle FindUnescaped(const Core::String& Other, const char& Needle, size_t Offset = 0U);
			static TextSettle FindOf(const Core::String& Other, const Core::String& Needle, size_t Offset = 0U);
			static TextSettle FindOf(const Core::String& Other, const char* Needle, size_t Offset = 0U);
			static TextSettle FindNotOf(const Core::String& Other, const Core::String& Needle, size_t Offset = 0U);
			static TextSettle FindNotOf(const Core::String& Other, const char* Needle, size_t Offset = 0U);
			static bool IsPrecededBy(const Core::String& Other, size_t At, const char* Of);
			static bool IsFollowedBy(const Core::String& Other, size_t At, const char* Of);
			static bool StartsWith(const Core::String& Other, const Core::String& Value, size_t Offset = 0U);
			static bool StartsWith(const Core::String& Other, const char* Value, size_t Offset = 0U);
			static bool StartsOf(const Core::String& Other, const char* Value, size_t Offset = 0U);
			static bool StartsNotOf(const Core::String& Other, const char* Value, size_t Offset = 0U);
			static bool EndsWith(const Core::String& Other, const Core::String& Value);
			static bool EndsOf(const Core::String& Other, const char* Value);
			static bool EndsNotOf(const Core::String& Other, const char* Value);
			static bool EndsWith(const Core::String& Other, const char* Value);
			static bool EndsWith(const Core::String& Other, const char& Value);
			static bool HasInteger(const Core::String& Other);
			static bool HasNumber(const Core::String& Other);
			static bool HasDecimal(const Core::String& Other);
			static Core::String Text(const char* Format, ...);
			static Core::WideString ToWide(const Core::String& Other);
			static Core::Vector<Core::String> Split(const Core::String& Other, const Core::String& With, size_t Start = 0U);
			static Core::Vector<Core::String> Split(const Core::String& Other, char With, size_t Start = 0U);
			static Core::Vector<Core::String> SplitMax(const Core::String& Other, char With, size_t MaxCount, size_t Start = 0U);
			static Core::Vector<Core::String> SplitOf(const Core::String& Other, const char* With, size_t Start = 0U);
			static Core::Vector<Core::String> SplitNotOf(const Core::String& Other, const char* With, size_t Start = 0U);
			static bool IsDigit(char Char);
			static bool IsAlphabetic(char Char);
			static int CaseCompare(const char* Value1, const char* Value2);
			static int Match(const char* Pattern, const char* Text);
			static int Match(const char* Pattern, size_t Length, const char* Text);
			static void ConvertToWide(const char* Input, size_t InputSize, wchar_t* Output, size_t OutputSize);
		};

		class VI_OUT_TS Guard
		{
		public:
			class VI_OUT Loaded
			{
			public:
				friend Guard;

			private:
				Guard* Base;

			public:
				Loaded(Loaded&& Other) noexcept;
				Loaded& operator =(Loaded&& Other) noexcept;
				~Loaded() noexcept;
				void Close();
				explicit operator bool() const;

			private:
				Loaded(Guard* NewBase) noexcept;
			};

			class VI_OUT Stored
			{
			public:
				friend Guard;

			private:
				Guard* Base;

			public:
				Stored(Stored&& Other) noexcept;
				Stored& operator =(Stored&& Other) noexcept;
				~Stored() noexcept;
				void Close();
				explicit operator bool() const;

			private:
				Stored(Guard* NewBase) noexcept;
			};

		public:
			friend Loaded;
			friend Stored;

		private:
			std::condition_variable Condition;
			std::mutex Mutex;
			uint32_t Readers;
			uint32_t Writers;

		public:
			Guard() noexcept;
			Guard(const Guard& Other) = delete;
			Guard(Guard&& Other) = delete;
			~Guard() noexcept = default;
			Guard& operator =(const Guard& Other) = delete;
			Guard& operator =(Guard&& Other) = delete;
			Loaded TryLoad();
			Loaded Load();
			Stored TryStore();
			Stored Store();
			bool TryLoadLock();
			void LoadLock();
			void LoadUnlock();
			bool TryStoreLock();
			void StoreLock();
			void StoreUnlock();
		};

		class VI_OUT_TS Var
		{
		public:
			class VI_OUT Set
			{
			public:
				static Unique<Schema> Auto(Variant&& Value);
				static Unique<Schema> Auto(const Variant& Value);
				static Unique<Schema> Auto(const Core::String& Value, bool Strict = false);
				static Unique<Schema> Null();
				static Unique<Schema> Undefined();
				static Unique<Schema> Object();
				static Unique<Schema> Array();
				static Unique<Schema> Pointer(void* Value);
				static Unique<Schema> String(const Core::String& Value);
				static Unique<Schema> String(const char* Value, size_t Size);
				static Unique<Schema> Binary(const Core::String& Value);
				static Unique<Schema> Binary(const unsigned char* Value, size_t Size);
				static Unique<Schema> Binary(const char* Value, size_t Size);
				static Unique<Schema> Integer(int64_t Value);
				static Unique<Schema> Number(double Value);
				static Unique<Schema> Decimal(const Core::Decimal& Value);
				static Unique<Schema> Decimal(Core::Decimal&& Value);
				static Unique<Schema> DecimalString(const Core::String& Value);
				static Unique<Schema> Boolean(bool Value);
			};

		public:
			static Variant Auto(const Core::String& Value, bool Strict = false);
			static Variant Null();
			static Variant Undefined();
			static Variant Object();
			static Variant Array();
			static Variant Pointer(void* Value);
			static Variant String(const Core::String& Value);
			static Variant String(const char* Value, size_t Size);
			static Variant Binary(const Core::String& Value);
			static Variant Binary(const unsigned char* Value, size_t Size);
			static Variant Binary(const char* Value, size_t Size);
			static Variant Integer(int64_t Value);
			static Variant Number(double Value);
			static Variant Decimal(const Core::Decimal& Value);
			static Variant Decimal(Core::Decimal&& Value);
			static Variant DecimalString(const Core::String& Value);
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
				static CacheInfo GetCacheInfo(unsigned int level);
				static Arch GetArch() noexcept;
				static Endian GetEndianness() noexcept;
				static size_t GetFrequency() noexcept;

			public:
				template <typename T>
				static typename std::enable_if<std::is_arithmetic<T>::value, T>::type SwapEndianness(T Value)
				{
					union U
					{
						T Value;
						std::array<std::uint8_t, sizeof(T)> Data;
					} From, To;

					From.Value = Value;
					std::reverse_copy(From.Data.begin(), From.Data.end(), To.Data.begin());
					return To.Value;
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
				static void SetWorking(const char* Path);
				static bool Patch(const Core::String& Path);
				static bool Scan(const Core::String& Path, Core::Vector<FileEntry>* Entries);
				static bool Each(const char* Path, const std::function<bool(FileEntry*)>& Callback);
				static bool Create(const char* Path);
				static bool Remove(const char* Path);
				static bool IsExists(const char* Path);
				static Core::String GetModule();
				static Core::String GetWorking();
				static Core::Vector<Core::String> GetMounts();
			};

			class VI_OUT File
			{
			public:
				static bool Write(const Core::String& Path, const char* Data, size_t Length);
				static bool Write(const Core::String& Path, const Core::String& Data);
				static bool State(const Core::String& Path, FileEntry* Resource);
				static bool Move(const char* From, const char* To);
				static bool Copy(const char* From, const char* To);
				static bool Remove(const char* Path);
				static bool IsExists(const char* Path);
				static void Close(Unique<void> Stream);
				static int Compare(const Core::String& FirstPath, const Core::String& SecondPath);
				static size_t Join(const Core::String& To, const Core::Vector<Core::String>& Paths);
				static uint64_t GetHash(const Core::String& Data);
				static uint64_t GetIndex(const char* Data, size_t Size);
				static FileState GetProperties(const char* Path);
				static Unique<Stream> OpenJoin(const Core::String& Path, const Core::Vector<Core::String>& Paths);
				static Unique<Stream> OpenArchive(const Core::String& Path, size_t UnarchivedMaxSize = 128 * 1024 * 1024);
				static Unique<Stream> Open(const Core::String& Path, FileMode Mode, bool Async = false);
				static Unique<void> Open(const char* Path, const char* Mode);
				static Unique<unsigned char> ReadChunk(Stream* Stream, size_t Length);
				static Unique<unsigned char> ReadAll(const Core::String& Path, size_t* ByteLength);
				static Unique<unsigned char> ReadAll(Stream* Stream, size_t* ByteLength);
				static Core::String ReadAsString(const Core::String& Path);
				static Core::Vector<Core::String> ReadAsArray(const Core::String& Path);

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
				static bool IsRemote(const char* Path);
				static bool IsRelative(const char* Path);
				static bool IsAbsolute(const char* Path);
				static Core::String Resolve(const char* Path);
				static Core::String Resolve(const Core::String& Path, const Core::String& Directory, bool EvenIfExists);
				static Core::String ResolveDirectory(const char* Path);
				static Core::String ResolveDirectory(const Core::String& Path, const Core::String& Directory, bool EvenIfExists);
				static Core::String GetDirectory(const char* Path, size_t Level = 0);
				static Core::String GetNonExistant(const Core::String& Path);
				static const char* GetFilename(const char* Path);
				static const char* GetExtension(const char* Path);
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
                struct VI_OUT ArgsContext
                {
				public:
                    Core::UnorderedMap<Core::String, Core::String> Base;
                    
				public:
					ArgsContext(int Argc, char** Argv, const Core::String& WhenNoValue = "1") noexcept;
					void ForEach(const std::function<void(const Core::String&, const Core::String&)>& Callback) const;
					bool IsEnabled(const Core::String& Option, const Core::String& Shortcut = "") const;
					bool IsDisabled(const Core::String& Option, const Core::String& Shortcut = "") const;
					bool Has(const Core::String& Option, const Core::String& Shortcut = "") const;
					Core::String& Get(const Core::String& Option, const Core::String& Shortcut = "");
					Core::String& GetIf(const Core::String& Option, const Core::String& Shortcut, const Core::String& WhenEmpty);
					Core::String& GetAppPath();
                    
                private:
					bool IsTrue(const Core::String& Value) const;
					bool IsFalse(const Core::String& Value) const;
                };
                
			public:
				static void Interrupt();
				static int ExecutePlain(const String& Command);
				static ProcessStream* ExecuteWriteOnly(const String& Command);
				static ProcessStream* ExecuteReadOnly(const String& Command);
				static bool Spawn(const Core::String& Path, const Core::Vector<Core::String>& Params, ChildProcess* Result);
				static bool Await(ChildProcess* Process, int* ExitCode);
				static bool Free(ChildProcess* Process);
                static Core::String GetThreadId(const std::thread::id& Id);
                static Core::UnorderedMap<Core::String, Core::String> GetArgs(int Argc, char** Argv, const Core::String& WhenNoValue = "1");
			};

			class VI_OUT Symbol
			{
			public:
				static Unique<void> Load(const Core::String& Path = "");
				static Unique<void> LoadFunction(void* Handle, const Core::String& Name);
				static bool Unload(void* Handle);
			};

			class VI_OUT Input
			{
			public:
				static bool Text(const Core::String& Title, const Core::String& Message, const Core::String& DefaultInput, Core::String* Result);
				static bool Password(const Core::String& Title, const Core::String& Message, Core::String* Result);
				static bool Save(const Core::String& Title, const Core::String& DefaultPath, const Core::String& Filter, const Core::String& FilterDescription, Core::String* Result);
				static bool Open(const Core::String& Title, const Core::String& DefaultPath, const Core::String& Filter, const Core::String& FilterDescription, bool Multiple, Core::String* Result);
				static bool Folder(const Core::String& Title, const Core::String& DefaultPath, Core::String* Result);
				static bool Color(const Core::String& Title, const Core::String& DefaultHexRGB, Core::String* Result);
			};

			class VI_OUT Error
			{
			public:
				static int Get(bool Clear = true);
				static void Clear();
				static bool Occurred();
				static bool IsError(int Code);
				static std::error_code GetCode();
				static std::error_code GetCode(int Code);
				static std::error_condition GetCondition();
				static std::error_condition GetCondition(int Code);
				static Core::String GetName(int Code);
			};
		};

		class VI_OUT_TS Composer
		{
		private:
			static Mapping<Core::UnorderedMap<uint64_t, std::pair<uint64_t, void*>>>* Factory;

		public:
			static bool Clear();
			static bool Pop(const Core::String& Hash);
			static Core::UnorderedSet<uint64_t> Fetch(uint64_t Id);

		private:
			static void Push(uint64_t TypeId, uint64_t Tag, void* Callback);
			static void* Find(uint64_t TypeId);

		public:
			template <typename T, typename... Args>
			static Unique<T> Create(const Core::String& Hash, Args... Data)
			{
				return Create<T, Args...>(VI_HASH(Hash), Data...);
			}
			template <typename T, typename... Args>
			static Unique<T> Create(uint64_t Id, Args... Data)
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
			static Unique<void> Callee(Args... Data)
			{
				return (void*)new T(Data...);
			}
		};

		template <typename T>
		class Reference
		{
		private:
			std::atomic<uint32_t> __vcnt = 1;

		public:
			void operator delete(void* Ptr) noexcept
			{
				if (Ptr != nullptr)
				{
					auto Handle = (T*)Ptr;
					VI_ASSERT(Handle->__vcnt <= 1, "address at 0x%" PRIXPTR " is still in use but destructor has been called by delete as %s at %s()", Ptr, typeid(T).name(), __func__);
					VI_FREE(Ptr);
				}
			}
			void* operator new(size_t Size) noexcept
			{
				return (void*)VI_MALLOC(T, Size);
			}
			bool IsMarkedRef() const noexcept
			{
				return (bool)(__vcnt.load() & 0x80000000);
			}
			uint32_t GetRefCount() const noexcept
			{
				return __vcnt.load() & 0x7FFFFFFF;
			}
			void MarkRef() noexcept
			{
				__vcnt |= 0x80000000;
			}
			void AddRef() noexcept
			{
				__vcnt = (__vcnt.load() & 0x7FFFFFFF) + 1;
			}
			void Release() noexcept
			{
				__vcnt &= 0x7FFFFFFF;
				VI_ASSERT(__vcnt > 0 && Memory::IsValidAddress((void*)(T*)this), "address at 0x%" PRIXPTR " has already been released as %s at %s()", (void*)this, typeid(T).name(), __func__);
				if (!--__vcnt)
					delete (T*)this;
			}
		};

		template <typename T>
		class UPtr
		{
		private:
			T* Pointer;

		public:
			UPtr(T* NewPointer) noexcept : Pointer(NewPointer)
			{
			}
			UPtr(const UPtr& Other) noexcept = delete;
			UPtr(UPtr&& Other) noexcept : Pointer(Other.Pointer)
			{
				Other.Pointer = nullptr;
			}
			~UPtr()
			{
				Cleanup<T>();
			}
			UPtr& operator= (const UPtr& Other) noexcept = delete;
			UPtr& operator= (UPtr&& Other) noexcept
			{
				if (this == &Other)
					return *this;

				Cleanup<T>();
				Pointer = Other.Pointer;
				Other.Pointer = nullptr;
				return *this;
			}
			explicit operator bool() const
			{
				return Pointer != nullptr;
			}
			T* operator-> ()
			{
				VI_ASSERT(Pointer != nullptr, "unique pointer invalid access");
				return Pointer;
			}
			T* operator* ()
			{
				return Pointer;
			}
			T* OrPanic(const char* Message)
			{
				VI_ASSERT(Message != nullptr && Message[0] != '\0', "panic case message should be set");
				VI_PANIC(Pointer != nullptr, "panic is caused by %s", Message);
				return Pointer;
			}
			Unique<T> Reset()
			{
				T* Result = Pointer;
				Pointer = nullptr;
				return Result;
			}

		private:
			template <typename Q>
			inline typename std::enable_if<std::is_trivially_default_constructible<Q>::value && !std::is_base_of<Reference<Q>, Q>::value, void>::type Cleanup()
			{
				VI_FREE(Pointer);
				Pointer = nullptr;
			}
			template <typename Q>
			inline typename std::enable_if<!std::is_trivially_default_constructible<Q>::value && !std::is_base_of<Reference<Q>, Q>::value, void>::type Cleanup()
			{
				VI_DELETE(T, Pointer);
				Pointer = nullptr;
			}
			template <typename Q>
			inline typename std::enable_if<!std::is_trivially_default_constructible<Q>::value && std::is_base_of<Reference<Q>, Q>::value, void>::type Cleanup()
			{
				VI_CLEAR(Pointer);
			}
		};

		template <typename T>
		class Reactive
		{
		public:
			typedef std::function<void(T&)> SimpleSetterCallback;
			typedef std::function<void(T&, const T&)> ComplexSetterCallback;
			typedef std::function<T()> SimpleGetterCallback;
			typedef std::function<T(bool&)> ComplexGetterCallback;

		private:
			Core::UnorderedMap<size_t, ComplexSetterCallback> Setters;
			ComplexGetterCallback Getter;
			size_t Index;
			bool IsSet;

		private:
			T Value;

		public:
			Reactive() : Index(0), IsSet(false)
			{
			}
			Reactive(const T& NewValue) noexcept : Index(0), IsSet(true), Value(NewValue)
			{
			}
			Reactive(T&& NewValue) noexcept : Index(0), IsSet(true), Value(std::move(NewValue))
			{
			}
			Reactive(const Reactive& Other) = delete;
			Reactive(Reactive&& Other) = default;
			~Reactive() = default;
			Reactive& operator= (const T& Other)
			{
				SetValue(Other);
				return *this;
			}
			Reactive& operator= (T&& Other)
			{
				SetValue(std::move(Other));
				return *this;
			}
			Reactive& operator= (const Reactive& Other) = delete;
			Reactive& operator= (Reactive&& Other) = default;
			T* operator-> ()
			{
				T& Current = GetValue();
				return &Current;
			}
			operator const T& () const
			{
				return GetValue();
			}
			size_t WhenSet(ComplexSetterCallback&& NewCallback)
			{
				size_t Id = Index++;
				Setters[Id] = std::move(NewCallback);
				return Id;
			}
			size_t WhenSet(const ComplexSetterCallback& NewCallback)
			{
				size_t Id = Index++;
				Setters[Id] = NewCallback;
				return Id;
			}
			size_t WhenSet(SimpleSetterCallback&& NewCallback)
			{
				return WhenSet([NewCallback = std::move(NewCallback)](T& New, const T&) { NewCallback(New); });
			}
			size_t WhenSet(const SimpleSetterCallback& NewCallback)
			{
				return WhenSet([NewCallback](T& New, const T&) { NewCallback(New); });
			}
			void WhenGet(ComplexGetterCallback&& NewCallback)
			{
				Getter = std::move(NewCallback);
			}
			void WhenGet(const ComplexGetterCallback& NewCallback)
			{
				Getter = NewCallback;
			}
			void WhenGet(SimpleGetterCallback&& NewCallback)
			{
				Getter = [NewCallback = std::move(NewCallback)](bool& IsSet)->T
				{
					IsSet = true;
					return NewCallback();
				};
			}
			void WhenGet(const SimpleGetterCallback& NewCallback)
			{
				Getter = [NewCallback](bool& IsSet) -> T
				{
					IsSet = true;
					return NewCallback();
				};
			}
			void ClearWhenSet(size_t Id)
			{
				Setters.erase(Id);
			}
			void ClearWhenGet(size_t Id)
			{
				Getter = nullptr;
			}
			void Notify(const T& Other)
			{
				T OldValue = GetValue();
				Value = Other;

				for (auto& Item : Setters)
					Item.second(Value, OldValue);
			}
			void Notify()
			{
				Notify(GetValue());
			}

		public:
			inline typename std::enable_if<std::is_arithmetic<T>::value || std::is_same<T, Core::String>::value || std::is_same<T, Core::String>::value, Reactive&>::type operator+= (const T& Other)
			{
				T Temporary = GetValue() + Other;
				SetValue(std::move(Temporary));
				return *this;
			}

		private:
			void SetValue(const T& Other)
			{
				if (&Value == &Other)
					return;

				Notify(Other);
				Value = Other;
			}
			void SetValue(T&& Other)
			{
				if (&Value == &Other)
					return;

				Notify(Other);
				Value = std::move(Other);
			}
			T& GetValue()
			{
				if (!IsSet && Getter)
					Value = std::move(Getter(IsSet));

				return Value;
			}
		};

		template <typename T>
		class Reactive<T*>
		{
		public:
			typedef std::function<void(T*&)> SimpleSetterCallback;
			typedef std::function<void(T*&, const T*)> ComplexSetterCallback;
			typedef std::function<T* ()> SimpleGetterCallback;
			typedef std::function<T* (bool&)> ComplexGetterCallback;

		private:
			Core::UnorderedMap<size_t, ComplexSetterCallback> Setters;
			ComplexGetterCallback Getter;
			size_t Index;
			bool IsSet;

		private:
			UPtr<T> Value;

		public:
			Reactive() : Index(0), IsSet(false), Value(nullptr)
			{
			}
			Reactive(T* NewValue) noexcept : Index(0), IsSet(NewValue != nullptr), Value(NewValue)
			{
			}
			Reactive(const Reactive& Other) = delete;
			Reactive(Reactive&& Other) = default;
			~Reactive() = default;
			Reactive& operator= (T* Other)
			{
				SetValue(Other);
				return *this;
			}
			Reactive& operator= (UPtr<T>&& Other)
			{
				SetValue(std::move(Other));
				return *this;
			}
			Reactive& operator= (const Reactive& Other) = delete;
			Reactive& operator= (Reactive&& Other) = default;
			T* operator-> ()
			{
				T*& Current = GetValue();
				VI_ASSERT(Current != nullptr, "null pointer access");
				return Current;
			}
			operator T* ()
			{
				return GetValue();
			}
			Unique<T> Reset()
			{
				return Value.Reset();
			}
			size_t WhenSet(ComplexSetterCallback&& NewCallback)
			{
				size_t Id = Index++;
				Setters[Id] = std::move(NewCallback);
				return Id;
			}
			size_t WhenSet(const ComplexSetterCallback& NewCallback)
			{
				size_t Id = Index++;
				Setters[Id] = NewCallback;
				return Id;
			}
			size_t WhenSet(SimpleSetterCallback&& NewCallback)
			{
				return WhenSet([NewCallback = std::move(NewCallback)](T*& New, const T*) { NewCallback(New); });
			}
			size_t WhenSet(const SimpleSetterCallback& NewCallback)
			{
				return WhenSet([NewCallback](T*& New, const T*) { NewCallback(New); });
			}
			void WhenGet(ComplexGetterCallback&& NewCallback)
			{
				Getter = std::move(NewCallback);
			}
			void WhenGet(const ComplexGetterCallback& NewCallback)
			{
				Getter = NewCallback;
			}
			void WhenGet(SimpleGetterCallback&& NewCallback)
			{
				Getter = [NewCallback = std::move(NewCallback)](bool& IsSet)->T
				{
					IsSet = true;
					return NewCallback();
				};
			}
			void WhenGet(const SimpleGetterCallback& NewCallback)
			{
				Getter = [NewCallback](bool& IsSet) -> T
				{
					IsSet = true;
					return NewCallback();
				};
			}
			void ClearWhenSet(size_t Id)
			{
				Setters.erase(Id);
			}
			void ClearWhenGet(size_t Id)
			{
				Getter = nullptr;
			}
			void Notify(T* Other)
			{
				const T* OldValue = GetValue();
				Value = Other;

				for (auto& Item : Setters)
					Item.second(Value, OldValue);
			}
			void Notify()
			{
				Notify(Value);
			}

		private:
			void SetValue(T* Other)
			{
				if (Value == Other)
					return;

				Notify(Other);
				Value = Other;
			}
			void SetValue(UPtr<T>&& Other)
			{
				if (&Value == &Other)
					return;

				Notify(Other);
				Value = std::move(Other);
			}
			T*& GetValue()
			{
				if (!IsSet && Getter)
					Value = Getter(IsSet);

				return Value;
			}
		};

		class VI_OUT_TS Console final : public Reference<Console>
		{
		private:
			enum class Mode
			{
				Attached,
				Allocated,
				Detached
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
				unsigned short Attributes = 0;
				double Time = 0.0;
			} Cache;

		protected:
			std::recursive_mutex Session;
			Mode Status;
			bool Colors;

		private:
			Console() noexcept;

		public:
			~Console() noexcept;
			void Begin();
			void End();
			void Hide();
			void Show();
			void Clear();
			void Attach();
			void Detach();
			void Allocate();
			void Deallocate();
			void Flush();
			void FlushWrite();
			void Trace(uint32_t MaxFrames = 32);
			void CaptureTime();
			void SetColoring(bool Enabled);
			void SetCursor(uint32_t X, uint32_t Y);
			void ColorBegin(StdColor Text, StdColor Background = StdColor::Black);
			void ColorEnd();
			void WriteBuffer(const char* Buffer);
			void WriteLine(const Core::String& Line);
			void WriteChar(char Value);
			void Write(const Core::String& Line);
			void fWriteLine(const char* Format, ...);
			void fWrite(const char* Format, ...);
			void sWriteLine(const Core::String& Line);
			void sWrite(const Core::String& Line);
			void sfWriteLine(const char* Format, ...);
			void sfWrite(const char* Format, ...);
			void GetSize(uint32_t* Width, uint32_t* Height);
			double GetCapturedTime() const;
			bool ReadLine(Core::String& Data, size_t Size);
			Core::String Read(size_t Size);
			char ReadChar();

		public:
			static Console* Get();
			static bool Reset();
			static bool IsPresent();

		private:
			static Console* Singleton;
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
			Core::SingleQueue<Capture> Captures;
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
		protected:
			Core::String Path;
			size_t VirtualSize;

		public:
			Stream() noexcept;
			virtual ~Stream() noexcept = default;
			virtual bool Clear() = 0;
			virtual bool Open(const char* File, FileMode Mode) = 0;
			virtual bool Close() = 0;
			virtual bool Seek(FileSeek Mode, int64_t Offset) = 0;
			virtual bool Move(int64_t Offset) = 0;
			virtual int Flush() = 0;
			virtual size_t ReadAny(const char* Format, ...) = 0;
			virtual size_t Read(char* Buffer, size_t Length) = 0;
			virtual size_t WriteAny(const char* Format, ...) = 0;
			virtual size_t Write(const char* Buffer, size_t Length) = 0;
			virtual size_t Tell() = 0;
			virtual int GetFd() const = 0;
			virtual void* GetBuffer() const = 0;
			virtual bool IsSized() const = 0;
			void SetVirtualSize(size_t Size);
			size_t ReadAll(const std::function<void(char*, size_t)>& Callback);
			size_t GetVirtualSize() const;
			size_t GetSize();
			Core::String& GetSource();
		};

		class VI_OUT FileStream : public Stream
		{
		protected:
			FILE* Resource;

		public:
			FileStream() noexcept;
			~FileStream() noexcept override;
			virtual bool Clear() override;
			virtual bool Open(const char* File, FileMode Mode) override;
			virtual bool Close() override;
			bool Seek(FileSeek Mode, int64_t Offset) override;
			bool Move(int64_t Offset) override;
			int Flush() override;
			size_t ReadAny(const char* Format, ...) override;
			size_t Read(char* Buffer, size_t Length) override;
			size_t WriteAny(const char* Format, ...) override;
			size_t Write(const char* Buffer, size_t Length) override;
			size_t Tell() override;
			int GetFd() const override;
			void* GetBuffer() const override;
			virtual bool IsSized() const override;
		};

		class VI_OUT GzStream final : public Stream
		{
		protected:
			void* Resource;

		public:
			GzStream() noexcept;
			~GzStream() noexcept override;
			bool Clear() override;
			bool Open(const char* File, FileMode Mode) override;
			bool Close() override;
			bool Seek(FileSeek Mode, int64_t Offset) override;
			bool Move(int64_t Offset) override;
			int Flush() override;
			size_t ReadAny(const char* Format, ...) override;
			size_t Read(char* Buffer, size_t Length) override;
			size_t WriteAny(const char* Format, ...) override;
			size_t Write(const char* Buffer, size_t Length) override;
			size_t Tell() override;
			int GetFd() const override;
			void* GetBuffer() const override;
			bool IsSized() const override;
		};

		class VI_OUT WebStream final : public Stream
		{
		protected:
			void* Resource;
			Core::UnorderedMap<Core::String, Core::String> Headers;
			Core::Vector<char> Chunk;
			size_t Offset;
			size_t Size;
			bool Async;

		public:
			WebStream(bool IsAsync) noexcept;
			WebStream(bool IsAsync, Core::UnorderedMap<Core::String, Core::String>&& NewHeaders) noexcept;
			~WebStream() noexcept override;
			bool Clear() override;
			bool Open(const char* File, FileMode Mode) override;
			bool Close() override;
			bool Seek(FileSeek Mode, int64_t Offset) override;
			bool Move(int64_t Offset) override;
			int Flush() override;
			size_t ReadAny(const char* Format, ...) override;
			size_t Read(char* Buffer, size_t Length) override;
			size_t WriteAny(const char* Format, ...) override;
			size_t Write(const char* Buffer, size_t Length) override;
			size_t Tell() override;
			int GetFd() const override;
			void* GetBuffer() const override;
			bool IsSized() const override;
		};

		class VI_OUT ProcessStream final : public FileStream
		{
		private:
			int ExitCode;

		public:
			ProcessStream() noexcept;
			~ProcessStream() noexcept = default;
			bool Clear() override;
			bool Open(const char* File, FileMode Mode) override;
			bool Close() override;
			bool IsSized() const override;
			int GetExitCode() const;

		private:
			static void* OpenPipe(const char* File, const char* Mode);
			static int ClosePipe(void* Fd);
		};

		class VI_OUT FileLog final : public Reference<FileLog>
		{
		private:
			Core::String LastValue;
			size_t Offset;
			int64_t Time;

		public:
			Stream* Source = nullptr;
			Core::String Path, Name;

		public:
			FileLog(const Core::String& Root) noexcept;
			~FileLog() noexcept;
			void Process(const std::function<bool(FileLog*, const char*, int64_t)>& Callback);
		};

		class VI_OUT FileTree final : public Reference<FileTree>
		{
		public:
			Core::Vector<FileTree*> Directories;
			Core::Vector<Core::String> Files;
			Core::String Path;

		public:
			FileTree(const Core::String& Path) noexcept;
			~FileTree() noexcept;
			void Loop(const std::function<bool(const FileTree*)>& Callback) const;
			const FileTree* Find(const Core::String& Path) const;
			size_t GetFiles() const;
		};

		class VI_OUT Costate final : public Reference<Costate>
		{
			friend Cocontext;

		private:
			Core::UnorderedSet<Coroutine*> Cached;
			Core::UnorderedSet<Coroutine*> Used;
			std::thread::id Thread;
			Coroutine* Current;
			Cocontext* Master;
			size_t Size;

		public:
			std::function<void()> NotifyLock;
			std::function<void()> NotifyUnlock;

		public:
			Costate(size_t StackSize = STACK_SIZE) noexcept;
			~Costate() noexcept;
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
			bool HasActive() const;
			Coroutine* GetCurrent() const;
			size_t GetCount() const;

		private:
			int Swap(Coroutine* Routine);

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
			Core::Vector<Schema*>* Nodes;
			Schema* Parent;
			bool Saved;

		public:
			Core::String Key;
			Variant Value;

		public:
			Schema(const Variant& Base) noexcept;
			Schema(Variant&& Base) noexcept;
			~Schema() noexcept;
			Core::UnorderedMap<Core::String, size_t> GetNames() const;
			Core::Vector<Schema*> FindCollection(const Core::String& Name, bool Deep = false) const;
			Core::Vector<Schema*> FetchCollection(const Core::String& Notation, bool Deep = false) const;
			Core::Vector<Schema*> GetAttributes() const;
			Core::Vector<Schema*>& GetChilds();
			Schema* Find(const Core::String& Name, bool Deep = false) const;
			Schema* Fetch(const Core::String& Notation, bool Deep = false) const;
			Variant FetchVar(const Core::String& Key, bool Deep = false) const;
			Variant GetVar(size_t Index) const;
			Variant GetVar(const Core::String& Key) const;
			Variant GetAttributeVar(const Core::String& Key) const;
			Schema* GetParent() const;
			Schema* GetAttribute(const Core::String& Key) const;
			Schema* Get(size_t Index) const;
			Schema* Get(const Core::String& Key) const;
			Schema* Set(const Core::String& Key);
			Schema* Set(const Core::String& Key, const Variant& Value);
			Schema* Set(const Core::String& Key, Variant&& Value);
			Schema* Set(const Core::String& Key, Unique<Schema> Value);
			Schema* SetAttribute(const Core::String& Key, const Variant& Value);
			Schema* SetAttribute(const Core::String& Key, Variant&& Value);
			Schema* Push(const Variant& Value);
			Schema* Push(Variant&& Value);
			Schema* Push(Unique<Schema> Value);
			Schema* Pop(size_t Index);
			Schema* Pop(const Core::String& Name);
			Unique<Schema> Copy() const;
			bool Rename(const Core::String& Name, const Core::String& NewName);
			bool Has(const Core::String& Name) const;
			bool HasAttribute(const Core::String& Name) const;
			bool IsEmpty() const;
			bool IsAttribute() const;
			bool IsSaved() const;
			size_t Size() const;
			Core::String GetName() const;
			void Join(Schema* Other, bool AppendOnly);
			void Reserve(size_t Size);
			void Unlink();
			void Clear();
			void Save();

		protected:
			void Allocate();
			void Allocate(const Core::Vector<Schema*>& Other);

		private:
			void Attach(Schema* Root);

		public:
			static bool Transform(Schema* Value, const SchemaNameCallback& Callback);
			static bool ConvertToXML(Schema* Value, const SchemaWriteCallback& Callback);
			static bool ConvertToJSON(Schema* Value, const SchemaWriteCallback& Callback);
			static bool ConvertToJSONB(Schema* Value, const SchemaWriteCallback& Callback);
			static Core::String ToXML(Schema* Value);
			static Core::String ToJSON(Schema* Value);
			static Core::Vector<char> ToJSONB(Schema* Value);
			static Unique<Schema> ConvertFromXML(const char* Buffer, bool Assert = true);
			static Unique<Schema> ConvertFromJSON(const char* Buffer, size_t Size, bool Assert = true);
			static Unique<Schema> ConvertFromJSONB(const SchemaReadCallback& Callback, bool Assert = true);
			static Unique<Schema> FromXML(const Core::String& Text, bool Assert = true);
			static Unique<Schema> FromJSON(const Core::String& Text, bool Assert = true);
			static Unique<Schema> FromJSONB(const Core::Vector<char>& Binary, bool Assert = true);

		private:
			static bool ProcessConvertionFromXML(void* Base, Schema* Current);
			static bool ProcessConvertionFromJSON(void* Base, Schema* Current);
			static bool ProcessConvertionFromJSONB(Schema* Current, Core::UnorderedMap<size_t, Core::String>* Map, const SchemaReadCallback& Callback);
			static bool ProcessConvertionToJSONB(Schema* Current, Core::UnorderedMap<Core::String, size_t>* Map, const SchemaWriteCallback& Callback);
			static bool GenerateNamingTable(const Schema* Current, Core::UnorderedMap<Core::String, size_t>* Map, size_t& Index);
		};

		class VI_OUT_TS Schedule final : public Reference<Schedule>
		{
		private:
			struct ThreadPtr
			{
				std::condition_variable Notify;
				std::mutex Update;
				std::thread Handle;
				std::thread::id Id;
				Difficulty Type = Difficulty::Count;
				size_t GlobalIndex = 0;
				size_t LocalIndex = 0;
				bool Daemon = false;
			};

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

			struct VI_OUT ThreadDebug
			{
				std::thread::id Id = std::thread::id();
				Difficulty Type = Difficulty::Count;
				ThreadTask State = ThreadTask::Spawn;
				size_t Tasks = 0;
			};

			typedef std::function<void(const ThreadDebug&)> ThreadDebugCallback;

		public:
			struct VI_OUT Desc
			{
				std::chrono::milliseconds Timeout = std::chrono::milliseconds(2000);
				size_t Threads[(size_t)Difficulty::Count] = { 1, 1, 1, 1 };
				size_t Memory = STACK_SIZE;
				size_t Coroutines = 16;
				ActivityCallback Ping = nullptr;
				bool Parallel = true;

				Desc();
				void SetThreads(size_t Cores);
			};

		private:
			struct
			{
				Core::Vector<TaskCallback> Events;
				TaskCallback Tasks[EVENTS_SIZE];
				Costate* State = nullptr;
			} Dispatcher;

		private:
			ConcurrentQueue* Queues[(size_t)Difficulty::Count];
			Core::Vector<ThreadPtr*> Threads[(size_t)Difficulty::Count];
			std::atomic<TaskId> Generation;
			std::mutex Exclusive;
			ThreadDebugCallback Debug;
			Desc Policy;
			bool Enqueue;
			bool Terminate;
			bool Active;
			bool Immediate;

		private:
			Schedule() noexcept;

		public:
			~Schedule() noexcept;
			TaskId SetInterval(uint64_t Milliseconds, const TaskCallback& Callback, Difficulty Type = Difficulty::Light);
			TaskId SetInterval(uint64_t Milliseconds, TaskCallback&& Callback, Difficulty Type = Difficulty::Light);
			TaskId SetSeqInterval(uint64_t Milliseconds, const SeqTaskCallback& Callback, Difficulty Type = Difficulty::Light);
			TaskId SetSeqInterval(uint64_t Milliseconds, SeqTaskCallback&& Callback, Difficulty Type = Difficulty::Light);
			TaskId SetTimeout(uint64_t Milliseconds, const TaskCallback& Callback, Difficulty Type = Difficulty::Light);
			TaskId SetTimeout(uint64_t Milliseconds, TaskCallback&& Callback, Difficulty Type = Difficulty::Light);
			TaskId SetSeqTimeout(uint64_t Milliseconds, const SeqTaskCallback& Callback, Difficulty Type = Difficulty::Light);
			TaskId SetSeqTimeout(uint64_t Milliseconds, SeqTaskCallback&& Callback, Difficulty Type = Difficulty::Light);
			bool SetTask(const TaskCallback& Callback, Difficulty Type = Difficulty::Heavy);
			bool SetTask(TaskCallback&& Callback, Difficulty Type = Difficulty::Heavy);
			bool SetCoroutine(const TaskCallback& Callback);
			bool SetCoroutine(TaskCallback&& Callback);
			bool SetDebugCallback(const ThreadDebugCallback& Callback);
			bool SetImmediate(bool Enabled);
			bool ClearTimeout(TaskId WorkId);
			bool Start(const Desc& NewPolicy);
			bool Stop();
			bool Wakeup();
			bool Dispatch();
			bool IsActive() const;
			bool CanEnqueue() const;
			bool HasTasks(Difficulty Type) const;
			bool HasAnyTasks() const;
			size_t GetThreadGlobalIndex();
			size_t GetThreadLocalIndex();
			size_t GetTotalThreads() const;
			size_t GetThreads(Difficulty Type) const;
			const Desc& GetPolicy() const;

		private:
			void InitializeThread(size_t GlobalIndex, size_t LocalIndex);
			size_t UpdateThreadGlobalCounter(int64_t Index);
			size_t UpdateThreadLocalCounter(int64_t Index);

		private:
			bool PostDebug(Difficulty Type, ThreadTask State, size_t Tasks);
			bool PostDebug(ThreadPtr* Ptr, ThreadTask State, size_t Tasks);
			bool ProcessTick(Difficulty Type);
			bool ProcessLoop(Difficulty Type, ThreadPtr* Thread);
			bool ThreadActive(ThreadPtr* Thread);
			bool ChunkCleanup();
			bool PushThread(Difficulty Type, size_t GlobalIndex, size_t LocalIndex, bool IsDaemon);
			bool PopThread(ThreadPtr* Thread);
			std::chrono::microseconds GetTimeout(std::chrono::microseconds Clock);
			TaskId GetTaskId();

		public:
			static std::chrono::microseconds GetClock();
			static Schedule* Get();
			static bool Reset();
			static bool IsPresentAndActive();

		private:
			static Schedule* Singleton;
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
				VI_FREE(Data);
				Data = nullptr;
				Count = 0;
				Volume = 0;
			}
			void Reserve(size_t Capacity)
			{
				if (Capacity <= Volume)
					return;

				size_t Size = Capacity * sizeof(T);
				T* NewData = VI_MALLOC(T, Size);
				memset(NewData, 0, Size);
				
				if (Data != nullptr)
				{
					memcpy(NewData, Data, Count * sizeof(T));
					VI_FREE(Data);
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
					VI_FREE(Data);
					Data = VI_MALLOC(T, Ref.Volume * sizeof(T));
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

		template <typename T>
		class Guarded
		{
		public:
			class Loaded
			{
			public:
				friend Guarded;

			private:
				Guarded* Base;

			public:
				Loaded(Loaded&& Other) noexcept : Base(Other.Base)
				{
					Other.Base = nullptr;
				}
				Loaded& operator =(Loaded&& Other) noexcept
				{
					if (&Other == this)
						return *this;

					Base = Other.Base;
					Other.Base = nullptr;
					return *this;
				}
				~Loaded() noexcept
				{
					Close();
				}
				void Close()
				{
					if (Base != nullptr)
					{
						Base->Mutex.LoadUnlock();
						Base = nullptr;
					}
				}
				explicit operator bool() const
				{
					return Base != nullptr;
				}
				const T& operator*() const
				{
					VI_ASSERT(Base != nullptr, "value was not loaded");
					return Base->Value;
				}
				const T& Unwrap() const
				{
					VI_ASSERT(Base != nullptr, "value was not loaded");
					return Base->Value;
				}

			private:
				Loaded(Guarded* NewBase) noexcept : Base(NewBase)
				{
				}
			};

			class Stored
			{
			public:
				friend Guarded;

			private:
				Guarded* Base;

			public:
				Stored(Stored&& Other) noexcept : Base(Other.Base)
				{
					Other.Base = nullptr;
				}
				Stored& operator =(Stored&& Other) noexcept
				{
					if (&Other == this)
						return *this;

					Base = Other.Base;
					Other.Base = nullptr;
					return *this;
				}
				~Stored() noexcept
				{
					Close();
				}
				void Close()
				{
					if (Base != nullptr)
					{
						Base->Mutex.StoreUnlock();
						Base = nullptr;
					}
				}
				explicit operator bool() const
				{
					return Base != nullptr;
				}
				T& operator*()
				{
					VI_ASSERT(Base != nullptr, "value was not stored");
					return Base->Value;
				}
				T& Unwrap()
				{
					VI_ASSERT(Base != nullptr, "value was not stored");
					return Base->Value;
				}

			private:
				Stored(Guarded* NewBase) noexcept : Base(NewBase)
				{
				}
			};

		public:
			friend Loaded;
			friend Stored;

		private:
			Guard Mutex;
			T Value;

		public:
			Guarded(const T& NewValue) noexcept : Value(NewValue)
			{
			}
			Guarded(const Guarded& Other) = delete;
			Guarded(Guarded&& Other) = delete;
			~Guarded() noexcept = default;
			Guarded& operator =(const Guarded& Other) = delete;
			Guarded& operator =(Guarded&& Other) = delete;
			Loaded TryLoad()
			{
				return Mutex.TryLoadLock() ? Loaded(this) : Loaded(nullptr);
			}
			Loaded Load()
			{
				Mutex.LoadLock();
				return Loaded(this);
			}
			Stored TryStore()
			{
				return Mutex.TryStoreLock() ? Stored(this) : Stored(nullptr);
			}
			Stored Store()
			{
				Mutex.StoreLock();
				return Stored(this);
			}
		};

		struct VI_OUT_TS ParallelExecutor
		{
			inline void operator()(TaskCallback&& Callback)
			{
				if (Schedule::IsPresentAndActive())
					Schedule::Get()->SetTask(std::move(Callback), Difficulty::Light);
				else
					Callback();
			}
		};

		template <typename T>
		class DeferredStorage
		{
		public:
			TaskCallback Event;
			char Value[sizeof(T)];
			std::atomic<uint32_t> Count;
			std::atomic<Deferred> Code;
			std::mutex Update;

		public:
			DeferredStorage() noexcept : Count(1), Code(Deferred::Pending)
			{
			}
			DeferredStorage(const T& NewValue) noexcept : Count(1), Code(Deferred::Ready)
			{
				new(Value) T(NewValue);
			}
			DeferredStorage(T&& NewValue) noexcept : Count(1), Code(Deferred::Ready)
			{
				new(Value) T(std::move(NewValue));
			}
			~DeferredStorage()
			{
				if (Code == Deferred::Ready)
					((T*)Value)->~T();
			}
			void Emplace(const T& NewValue)
			{
				VI_ASSERT(Code != Deferred::Ready, "emplacing to already initialized memory is not desired");
				new(Value) T(NewValue);
			}
			void Emplace(T&& NewValue)
			{
				VI_ASSERT(Code != Deferred::Ready, "emplacing to already initialized memory is not desired");
				new(Value) T(std::move(NewValue));
			}
			T& Unwrap()
			{
				VI_ASSERT(Code == Deferred::Ready, "unwrapping uninitialized memory will result in undefined behaviour");
				return *(T*)Value;
			}
		};

		template <>
		class DeferredStorage<void>
		{
		public:
			TaskCallback Event;
			std::mutex Update;
			std::atomic<uint32_t> Count;
			std::atomic<Deferred> Code;

		public:
			DeferredStorage() noexcept : Count(1), Code(Deferred::Pending)
			{
			}
		};

		template <typename T, typename Executor>
		class BasicPromise
		{
		public:
			typedef DeferredStorage<T> Status;
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
			BasicPromise() noexcept : Data(VI_NEW(Status))
			{
			}
			BasicPromise(const T& Value) noexcept : Data(VI_NEW(Status, Value))
			{
			}
			BasicPromise(T&& Value) noexcept : Data(VI_NEW(Status, std::move(Value)))
			{
			}
			BasicPromise(const BasicPromise& Other) : Data(Other.Data)
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
			void Set(const T& Other)
			{
				VI_ASSERT(Data != nullptr && Data->Code != Deferred::Ready, "async should be pending");
				std::unique_lock<std::mutex> Unique(Data->Update);
				bool Async = (Data->Code != Deferred::Waiting);
				Data->Emplace(Other);
				Data->Code = Deferred::Ready;
				Execute(Data, Async);
			}
			void Set(T&& Other)
			{
				VI_ASSERT(Data != nullptr && Data->Code != Deferred::Ready, "async should be pending");
				std::unique_lock<std::mutex> Unique(Data->Update);
				bool Async = (Data->Code != Deferred::Waiting);
				Data->Emplace(std::move(Other));
				Data->Code = Deferred::Ready;
				Execute(Data, Async);
			}
			void Set(const BasicPromise& Other)
			{
				VI_ASSERT(Data != nullptr && Data->Code != Deferred::Ready, "async should be pending");
				Status* Copy = AddRef();
				Other.When([Copy](T&& Value) mutable
				{
					{
						std::unique_lock<std::mutex> Unique(Copy->Update);
						bool Async = (Copy->Code != Deferred::Waiting);
						Copy->Emplace(std::move(Value));
						Copy->Code = Deferred::Ready;
						Execute(Copy, Async);
					}
					Release(Copy);
				});
			}
			void When(std::function<void(T&&)>&& Callback) const noexcept
			{
				VI_ASSERT(Callback, "callback should be set");
				if (!IsPending())
					return Callback(std::move(Data->Unwrap()));

				Status* Copy = AddRef();
				Store([Copy, Callback = std::move(Callback)]() mutable
				{
					Callback(std::move(Copy->Unwrap()));
					Release(Copy);
				});
			}
			void Wait()
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
			template <typename R>
			BasicPromise<R, Executor> Then(std::function<void(BasicPromise<R, Executor>&, T&&)>&& Callback) const noexcept
			{
				using OtherPromise = BasicPromise<R, Executor>;
				VI_ASSERT(Data != nullptr && Callback, "async should be pending");

				BasicPromise<R, Executor> Result; Status* Copy = AddRef();
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
				using F = typename Unwrap<R>::type;
				using OtherPromise = BasicPromise<F, Executor>;
				VI_ASSERT(Data != nullptr && Callback, "async should be pending");

				BasicPromise<F, Executor> Result; Status* Copy = AddRef();
				Store([Copy, Result, Callback = std::move(Callback)]() mutable
				{
					Result.Set(std::move(Callback(std::move(Copy->Unwrap()))));
					Release(Copy);
				});

				return Result;
			}

		private:
			BasicPromise(bool Unused1, bool Unused2) noexcept : Data(nullptr)
			{
			}
			Status* AddRef() const
			{
				if (Data != nullptr)
					++Data->Count;
				return Data;
			}
			T&& Load()
			{
				if (!Data)
					Data = VI_NEW(Status);

				return std::move(Data->Unwrap());
			}
			void Store(TaskCallback&& Callback) const noexcept
			{
				std::unique_lock<std::mutex> Unique(Data->Update);
				Data->Event = std::move(Callback);
				if (Data->Code == Deferred::Ready)
					Execute(Data, false);
			}

		public:
			static BasicPromise Null() noexcept
			{
				return BasicPromise(false, false);
			}

		private:
			static void Execute(Status* State, bool Async) noexcept
			{
				if (!State->Event)
					return;

				if (Async)
					Executor()(std::move(State->Event));
				else
					State->Event();
			}
			static void Release(Status* State) noexcept
			{
				if (State != nullptr && !--State->Count)
					VI_DELETE(Status, State);
			}
#ifdef VI_CXX20
		public:
			struct awaitable
			{
				BasicPromise Value;

				explicit awaitable(const BasicPromise& NewValue) : Value(NewValue)
				{
				}
				explicit awaitable(BasicPromise&& NewValue) : Value(std::move(NewValue))
				{
				}
				awaitable(const awaitable&) = default;
				awaitable(awaitable&&) = default;
				bool await_ready() const noexcept
				{
					return !Value.IsPending();
				}
				T&& await_resume() noexcept
				{
					return Value.Get();
				}
				void await_suspend(std::coroutine_handle<> Handle)
				{
					Value.When([Handle](T&&)
					{
						Handle.resume();
					});
				}
			};

			struct promise_type
			{
				BasicPromise Value;
#ifndef NDEBUG
				std::chrono::microseconds Time;
#endif
				promise_type()
				{
#ifndef NDEBUG
					Time = Schedule::GetClock();
					VI_WATCH((void*)&Value, "coroutine20-frame");
#endif
				}
				std::suspend_never initial_suspend() const noexcept
				{
					return {};
				}
				std::suspend_never final_suspend() const noexcept
				{
					return {};
				}
				BasicPromise get_return_object()
				{
#ifndef NDEBUG
					int64_t Diff = (Schedule::GetClock() - Time).count();
					if (Diff > (int64_t)Core::Timings::Hangup * 1000)
						VI_WARN("[stall] async operation took %" PRIu64 " ms (%" PRIu64 " us)\t\nexpected: %" PRIu64 " ms at most", Diff / 1000, Diff, (uint64_t)Core::Timings::Hangup);
					VI_UNWATCH((void*)&Value);
#endif
					return Value;
				}
				void return_value(const BasicPromise& NewValue)
				{
					Value.Set(NewValue);
				}
				void return_value(const T& NewValue)
				{
					Value.Set(NewValue);
				}
				void return_value(T&& NewValue)
				{
					Value.Set(std::move(NewValue));
				}
				void unhandled_exception()
				{
				}
				void* operator new(size_t Size) noexcept
				{
					return VI_MALLOC(promise_type, Size);
				}
				void operator delete(void* Ptr) noexcept
				{
					VI_FREE(Ptr);
				}
				static BasicPromise get_return_object_on_allocation_failure()
				{
					return BasicPromise::Null();
				}
			};
#endif
		};

		template <typename Executor>
		class BasicPromise<void, Executor>
		{
		public:
			typedef DeferredStorage<void> Status;
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
			BasicPromise() noexcept : Data(VI_NEW(Status))
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
			void Set()
			{
				VI_ASSERT(Data != nullptr && Data->Code != Deferred::Ready, "async should be pending");
				std::unique_lock<std::mutex> Unique(Data->Update);
				bool Async = (Data->Code != Deferred::Waiting);
				Data->Code = Deferred::Ready;
				Execute(Data, Async);
			}
			void Set(const BasicPromise& Other)
			{
				VI_ASSERT(Data != nullptr && Data->Code != Deferred::Ready, "async should be pending");
				Status* Copy = AddRef();
				Other.When([Copy]() mutable
				{
					{
						std::unique_lock<std::mutex> Unique(Copy->Update);
						bool Async = (Copy->Code != Deferred::Waiting);
						Copy->Code = Deferred::Ready;
						Execute(Copy, Async);
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
			void Wait()
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
			template <typename R>
			BasicPromise<R, Executor> Then(std::function<void(BasicPromise<R, Executor>&)>&& Callback) const noexcept
			{
				using OtherPromise = BasicPromise<R, Executor>;
				VI_ASSERT(Data != nullptr && Callback, "async should be pending");

				BasicPromise<R, Executor> Result; Status* Copy = AddRef();
				Store([Copy, Result, Callback = std::move(Callback)]() mutable
				{
					Callback(Result);
					Release(Copy);
				});

				return Result;
			}
			template <typename R>
			BasicPromise<typename Unwrap<R>::type, Executor> Then(std::function<R()>&& Callback) const noexcept
			{
				using F = typename Unwrap<R>::type;
				using OtherPromise = BasicPromise<F, Executor>;
				VI_ASSERT(Data != nullptr && Callback, "async should be pending");

				BasicPromise<F, Executor> Result; Status* Copy = AddRef();
				Store([Copy, Result, Callback = std::move(Callback)]() mutable
				{
					Callback();
					Result.Set();
					Release(Copy);
				});

				return Result;
			}

		private:
			BasicPromise(Status* Context) noexcept : Data(Context)
			{
				AddRef();
			}
			Status* AddRef() const
			{
				if (Data != nullptr)
					++Data->Count;
				return Data;
			}
			void Load() noexcept
			{
				if (!Data)
					Data = VI_NEW(Status);
			}
			void Store(TaskCallback&& Callback) const noexcept
			{
				std::unique_lock<std::mutex> Unique(Data->Update);
				Data->Event = std::move(Callback);
				if (Data->Code == Deferred::Ready)
					Execute(Data, false);
			}

		public:
			static BasicPromise Null() noexcept
			{
				return BasicPromise((Status*)nullptr);
			}

		private:
			static void Execute(Status* State, bool Async) noexcept
			{
				if (!State->Event)
					return;

				if (Async)
					Executor()(std::move(State->Event));
				else
					State->Event();
			}
			static void Release(Status* State) noexcept
			{
				if (State != nullptr && !--State->Count)
					VI_DELETE(Status, State);
			}
#ifdef VI_CXX20
		public:
			struct awaitable
			{
				BasicPromise Value;

				explicit awaitable(const BasicPromise& NewValue) : Value(NewValue)
				{
				}
				explicit awaitable(BasicPromise&& NewValue) : Value(std::move(NewValue))
				{
				}
				awaitable(const awaitable&) = default;
				awaitable(awaitable&&) = default;
				bool await_ready() const noexcept
				{
					return !Value.IsPending();
				}
				void await_resume() noexcept
				{
				}
				void await_suspend(std::coroutine_handle<> Handle)
				{
					Value.When([Handle]()
					{
						Handle.resume();
					});
				}
			};

			struct promise_type
			{
				BasicPromise Value;
#ifndef NDEBUG
				std::chrono::microseconds Time;
#endif
				promise_type()
				{
#ifndef NDEBUG
					Time = Schedule::GetClock();
					VI_WATCH((void*)&Value, "coroutine20-frame");
#endif
				}
				std::suspend_never initial_suspend() const noexcept
				{
					return {};
				}
				std::suspend_never final_suspend() const noexcept
				{
					return {};
				}
				BasicPromise get_return_object()
				{
#ifndef NDEBUG
					int64_t Diff = (Schedule::GetClock() - Time).count();
					if (Diff > (int64_t)Core::Timings::Hangup * 1000)
						VI_WARN("[stall] async operation took %" PRIu64 " ms (%" PRIu64 " us)\t\nexpected: %" PRIu64 " ms at most", Diff / 1000, Diff, (uint64_t)Core::Timings::Hangup);
					VI_UNWATCH((void*)&Value);
#endif
					return Value;
				}
				void return_void()
				{
					Value.Set();
				}
				void unhandled_exception()
				{
				}
				void* operator new(size_t Size) noexcept
				{
					return VI_MALLOC(promise_type, Size);
				}
				void operator delete(void* Ptr) noexcept
				{
					VI_FREE(Ptr);
				}
				static BasicPromise get_return_object_on_allocation_failure()
				{
					return BasicPromise::Null();
				}
			};
#endif
		};

		template <typename T, typename Executor = ParallelExecutor>
		using Promise = BasicPromise<T, Executor>;
		
		template <typename T>
		struct PromiseContext
		{
			std::function<Promise<T>()> Callback;

			PromiseContext(std::function<Promise<T>()>&& NewCallback) : Callback(std::move(NewCallback))
			{
			}
			~PromiseContext() = default;
			PromiseContext(const PromiseContext& Other) = delete;
			PromiseContext(PromiseContext&& Other) = delete;
			PromiseContext& operator= (const PromiseContext& Other) = delete;
			PromiseContext& operator= (PromiseContext&& Other) = delete;
		};

		inline bool Cosuspend() noexcept
		{
			VI_ASSERT(Costate::Get() != nullptr, "cannot call suspend outside coroutine");
			return Costate::Get()->Suspend();
		}
		template <typename T, typename Executor = ParallelExecutor>
		inline Promise<T> Cotask(std::function<T()>&& Callback, Difficulty Type = Difficulty::Heavy) noexcept
		{
			VI_ASSERT(Callback, "callback should not be empty");

			Promise<T> Result;
			Schedule::Get()->SetTask([Result, Callback = std::move(Callback)]() mutable
			{
				Result.Set(std::move(Callback()));
			}, Type);

			return Result;
		}
#ifdef VI_CXX20
		template <typename T, typename Executor = ParallelExecutor>
		auto operator co_await(BasicPromise<T, Executor>&& Value) noexcept
		{
			using Awaitable = typename BasicPromise<T, Executor>::awaitable;
			return Awaitable(std::move(Value));
		}
		template <typename T, typename Executor = ParallelExecutor>
		auto operator co_await(const BasicPromise<T, Executor>& Value) noexcept
		{
			using Awaitable = typename BasicPromise<T, Executor>::awaitable;
			return Awaitable(Value);
		}
		template <typename T>
		void Coforward1(Promise<T> Value, PromiseContext<T>* Context)
		{
			Promise<T> Wrapper = Context->Callback();
			auto Deleter = [Context](T&& Result) -> T&&
			{
				VI_DELETE(PromiseContext, Context);
				return std::move(Result);
			};
			Value.Set(Wrapper.template Then<T>(Deleter));
		}
		template <typename T>
		Promise<T> Coforward2(Promise<T>&& Value, PromiseContext<T>* Context)
		{
			auto Deleter = [Context](T&& Result) -> T&&
			{
				VI_DELETE(PromiseContext, Context);
				return std::move(Result);
			};
			return Value.template Then<T>(Deleter);
		}
		template <typename T>
		inline Promise<T> Coasync(std::function<Promise<T>()>&& Callback, bool AlwaysEnqueue = false) noexcept
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			PromiseContext<T>* Context = VI_NEW(PromiseContext<T>, std::move(Callback));
			if (AlwaysEnqueue)
			{
				Promise<T> Value;
				Schedule::Get()->SetTask(std::bind(&Coforward1<T>, Value, Context), Difficulty::Light);
				return Value;
			}
			else
			{
				Promise<T> Value = Context->Callback();
				return Coforward2<T>(std::move(Value), Context);
			}
		}
		template <>
		inline Promise<void> Coasync(std::function<Promise<void>()>&& Callback, bool AlwaysEnqueue) noexcept
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			PromiseContext<void>* Context = VI_NEW(PromiseContext<void>, std::move(Callback));
			if (AlwaysEnqueue)
			{
				Promise<void> Value;
				Schedule::Get()->SetTask([Value, Context]() mutable
				{
					Promise<void> Wrapper = Context->Callback();
					Value.Set(Wrapper.Then<void>([Context]()
					{
						VI_DELETE(PromiseContext, Context);
					}));
				}, Difficulty::Light);

				return Value;
			}
			else
			{
				Promise<void> Value = Context->Callback();
				return Value.Then<void>([Context]()
				{
					VI_DELETE(PromiseContext, Context);
				});
			}
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
				Future.When([&State, &Base](T&&)
				{
					State->Activate(Base);
				});
			});
#ifndef NDEBUG
			if (Function != nullptr && Expression != nullptr)
			{
				int64_t Diff = (Schedule::GetClock() - Time).count();
				if (Diff > (int64_t)Core::Timings::Hangup * 1000)
					VI_WARN("[stall] %s(): \"%s\" operation took %" PRIu64 " ms (%" PRIu64 " us) out of % " PRIu64 "ms budget", Function, Expression, Diff / 1000, Diff, (uint64_t)Core::Timings::Hangup);
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
				Result.Set(Callback().Get());
			});

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
				Callback();
				Result.Set();
			});

			return Result;
		}
#endif
#ifdef VI_HAS_FAST_MEMORY
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
#ifdef VI_CXX17
		template <typename T>
		inline Expects<T, std::error_condition> FromStringRadix(const Core::String& Other, int Base)
		{
			static_assert(std::is_integral<T>::value, "base can be specified only for integral types");
			T Value;
			std::from_chars_result Result = std::from_chars(Other.data(), Other.data() + Other.size(), Value, Base);
			if (Result.ec != std::errc())
				return std::make_error_condition(Result.ec);
			return Value;
		}
		template <typename T>
		inline Expects<T, std::error_condition> FromString(const Core::String& Other)
		{
			static_assert(std::is_integral<T>::value, "conversion can be done only for integral types");
			T Value;
			std::from_chars_result Result = std::from_chars(Other.data(), Other.data() + Other.size(), Value);
			if (Result.ec != std::errc())
				return std::make_error_condition(Result.ec);
			return Value;
		}
#else
		template <typename T>
		inline Expects<T, std::error_condition> FromStringRadix(const Core::String& Other, int Base)
		{
			static_assert(false, "conversion can be done only to arithmetic types");
			return std::make_error_condition(std::errc::not_supported);
		}
		template <>
		inline Expects<int8_t, std::error_condition> FromStringRadix<int8_t>(const Core::String& Other, int Base)
		{
			OS::Error::Clear();
			char* End = nullptr;
			const char* Start = Other.c_str();
			long Value = strtol(Start, &End, Base);
			if (Start == End)
				return std::make_error_condition(std::errc::invalid_argument);
			else if (Value < (long)std::numeric_limits<int8_t>::min())
				return std::make_error_condition(std::errc::result_out_of_range);
			else if (Value > (long)std::numeric_limits<int8_t>::max())
				return std::make_error_condition(std::errc::result_out_of_range);
			else if (OS::Error::Occurred())
				return OS::Error::GetCondition();
			return (int8_t)Value;
		}
		template <>
		inline Expects<int16_t, std::error_condition> FromStringRadix<int16_t>(const Core::String& Other, int Base)
		{
			OS::Error::Clear();
			char* End = nullptr;
			const char* Start = Other.c_str();
			long Value = strtol(Start, &End, Base);
			if (Start == End)
				return std::make_error_condition(std::errc::invalid_argument);
			else if (Value < (long)std::numeric_limits<int16_t>::min())
				return std::make_error_condition(std::errc::result_out_of_range);
			else if (Value > (long)std::numeric_limits<int16_t>::max())
				return std::make_error_condition(std::errc::result_out_of_range);
			else if (OS::Error::Occurred())
				return OS::Error::GetCondition();
			return (int16_t)Value;
		}
		template <>
		inline Expects<int32_t, std::error_condition> FromStringRadix<int32_t>(const Core::String& Other, int Base)
		{
			OS::Error::Clear();
			char* End = nullptr;
			const char* Start = Other.c_str();
			long Value = strtol(Start, &End, Base);
			if (Start == End)
				return std::make_error_condition(std::errc::invalid_argument);
			else if (OS::Error::Occurred())
				return OS::Error::GetCondition();
			return (int32_t)Value;
		}
		template <>
		inline Expects<int64_t, std::error_condition> FromStringRadix<int64_t>(const Core::String& Other, int Base)
		{
			OS::Error::Clear();
			char* End = nullptr;
			const char* Start = Other.c_str();
			long long Value = strtoll(Start, &End, Base);
			if (Start == End)
				return std::make_error_condition(std::errc::invalid_argument);
			else if (OS::Error::Occurred())
				return OS::Error::GetCondition();
			return (int64_t)Value;
		}
		template <>
		inline Expects<uint8_t, std::error_condition> FromStringRadix<uint8_t>(const Core::String& Other, int Base)
		{
			OS::Error::Clear();
			char* End = nullptr;
			const char* Start = Other.c_str();
			unsigned long Value = strtoul(Start, &End, Base);
			if (Start == End)
				return std::make_error_condition(std::errc::invalid_argument);
			else if (Value > (unsigned long)std::numeric_limits<uint8_t>::max())
				return std::make_error_condition(std::errc::result_out_of_range);
			else if (OS::Error::Occurred())
				return OS::Error::GetCondition();
			return (uint8_t)Value;
		}
		template <>
		inline Expects<uint16_t, std::error_condition> FromStringRadix<uint16_t>(const Core::String& Other, int Base)
		{
			OS::Error::Clear();
			char* End = nullptr;
			const char* Start = Other.c_str();
			unsigned long Value = strtoul(Start, &End, Base);
			if (Start == End)
				return std::make_error_condition(std::errc::invalid_argument);
			else if (Value > (unsigned long)std::numeric_limits<uint16_t>::max())
				return std::make_error_condition(std::errc::result_out_of_range);
			else if (OS::Error::Occurred())
				return OS::Error::GetCondition();
			return (uint16_t)Value;
		}
		template <>
		inline Expects<uint32_t, std::error_condition> FromStringRadix<uint32_t>(const Core::String& Other, int Base)
		{
			OS::Error::Clear();
			char* End = nullptr;
			const char* Start = Other.c_str();
			unsigned long Value = strtoul(Start, &End, Base);
			if (Start == End)
				return std::make_error_condition(std::errc::invalid_argument);
			else if (OS::Error::Occurred())
				return OS::Error::GetCondition();
			return (uint32_t)Value;
		}
		template <>
		inline Expects<uint64_t, std::error_condition> FromStringRadix<uint64_t>(const Core::String& Other, int Base)
		{
			OS::Error::Clear();
			char* End = nullptr;
			const char* Start = Other.c_str();
			unsigned long long Value = strtoull(Start, &End, Base);
			if (Start == End)
				return std::make_error_condition(std::errc::invalid_argument);
			else if (OS::Error::Occurred())
				return OS::Error::GetCondition();
			return (uint64_t)Value;
		}
		template <typename T>
		inline Expects<T, std::error_condition> FromString(const Core::String& Other)
		{
			static_assert(false, "conversion can be done only to arithmetic types");
			return std::make_error_condition(std::errc::not_supported);
		}
		template <>
		inline Expects<int8_t, std::error_condition> FromString<int8_t>(const Core::String& Other)
		{
			return FromStringRadix<int8_t>(Other, 10);
		}
		template <>
		inline Expects<int16_t, std::error_condition> FromString<int16_t>(const Core::String& Other)
		{
			return FromStringRadix<int16_t>(Other, 10);
		}
		template <>
		inline Expects<int32_t, std::error_condition> FromString<int32_t>(const Core::String& Other)
		{
			return FromStringRadix<int32_t>(Other, 10);
		}
		template <>
		inline Expects<int64_t, std::error_condition> FromString<int64_t>(const Core::String& Other)
		{
			return FromStringRadix<int64_t>(Other, 10);
		}
		template <>
		inline Expects<uint8_t, std::error_condition> FromString<uint8_t>(const Core::String& Other)
		{
			return FromStringRadix<uint8_t>(Other, 10);
		}
		template <>
		inline Expects<uint16_t, std::error_condition> FromString<uint16_t>(const Core::String& Other)
		{
			return FromStringRadix<uint16_t>(Other, 10);
		}
		template <>
		inline Expects<uint32_t, std::error_condition> FromString<uint32_t>(const Core::String& Other)
		{
			return FromStringRadix<uint32_t>(Other, 10);
		}
		template <>
		inline Expects<uint64_t, std::error_condition> FromString<uint64_t>(const Core::String& Other)
		{
			return FromStringRadix<uint64_t>(Other, 10);
		}
#endif
		template <>
		inline Expects<float, std::error_condition> FromString<float>(const Core::String& Other)
		{
			OS::Error::Clear();
			char* End = nullptr;
			const char* Start = Other.c_str();
			float Value = strtof(Start, &End);
			if (Start == End)
				return std::make_error_condition(std::errc::invalid_argument);
			else if (OS::Error::Occurred())
				return OS::Error::GetCondition();
			return Value;
		}
		template <>
		inline Expects<double, std::error_condition> FromString<double>(const Core::String& Other)
		{
			OS::Error::Clear();
			char* End = nullptr;
			const char* Start = Other.c_str();
			double Value = strtod(Start, &End);
			if (Start == End)
				return std::make_error_condition(std::errc::invalid_argument);
			else if (OS::Error::Occurred())
				return OS::Error::GetCondition();
			return Value;
		}
		template <>
		inline Expects<long double, std::error_condition> FromString<long double>(const Core::String& Other)
		{
			OS::Error::Clear();
			char* End = nullptr;
			const char* Start = Other.c_str();
			long double Value = strtold(Start, &End);
			if (Start == End)
				return std::make_error_condition(std::errc::invalid_argument);
			else if (OS::Error::Occurred())
				return OS::Error::GetCondition();
			return Value;
		}
		template <typename T>
		inline Core::String ToString(T Other)
		{
			static_assert(std::is_arithmetic<T>::value, "conversion can be done only to arithmetic types");
			return Core::Copy<Core::String, std::string>(std::to_string(Other));
		}
	}
}
#endif