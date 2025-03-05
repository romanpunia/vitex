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

namespace vitex
{
	namespace core
	{
		struct concurrent_async_queue;

		struct concurrent_sync_queue;

		struct decimal;

		struct cocontext;

		class console;

		class costate;

		class schema;

		class stream;

		class process_stream;

		class var;

		typedef uint64_t task_id;

		enum
		{
			INVALID_TASK_ID = (task_id)0,
			NUMSTR_SIZE = (size_t)32,
			CHUNK_SIZE = (size_t)2048,
			BLOB_SIZE = (size_t)8192,
			STACK_SIZE = (size_t)(512 * 1024)
		};

		enum class timings : uint64_t
		{
			atomic = 1,
			pass = 5,
			frame = 16,
			mixed = 50,
			file_system = 50,
			networking = 150,
			intensive = 350,
			hangup = 5000,
			infinite = 0
		};

		enum class deferred : uint8_t
		{
			pending = 0,
			waiting = 1,
			ready = 2
		};

		enum class std_color
		{
			black = 0,
			dark_blue = 1,
			dark_green = 2,
			light_blue = 3,
			dark_red = 4,
			magenta = 5,
			orange = 6,
			light_gray = 7,
			gray = 8,
			blue = 9,
			green = 10,
			cyan = 11,
			red = 12,
			pink = 13,
			yellow = 14,
			white = 15,
			zero = 16
		};

		enum class file_mode
		{
			read_only,
			write_only,
			append_only,
			read_write,
			write_read,
			read_append_write,
			binary_read_only,
			binary_write_only,
			binary_append_only,
			binary_read_write,
			binary_write_read,
			binary_read_append_write
		};

		enum class file_seek
		{
			begin,
			current,
			end
		};

		enum class var_type : uint8_t
		{
			null,
			undefined,
			object,
			array,
			pointer,
			string,
			binary,
			integer,
			number,
			decimal,
			boolean
		};

		enum class var_form
		{
			dummy,
			tab_decrease,
			tab_increase,
			write_space,
			write_line,
			write_tab,
		};

		enum class coexecution
		{
			active,
			suspended,
			resumable,
			finished
		};

		enum class difficulty
		{
			async,
			sync,
			timeout,
			count
		};

		enum class log_level
		{
			error = 1,
			warning = 2,
			info = 3,
			debug = 4,
			trace = 5
		};

		enum class log_option
		{
			active = 1 << 0,
			pretty = 1 << 1,
			async = 1 << 2,
			dated = 1 << 3,
			report_sys_errors = 1 << 4
		};

		enum class optional : int8_t
		{
			none = 0,
			value = 1
		};

		enum class expectation : int8_t
		{
			error = -1,
			met = 1
		};

		enum class signal_code
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

		enum class access_option : uint64_t
		{
			mem = (1 << 0),
			fs = (1 << 1),
			gz = (1 << 2),
			net = (1 << 3),
			lib = (1 << 4),
			http = (1 << 5),
			https = (1 << 6),
			shell = (1 << 7),
			env = (1 << 8),
			addons = (1 << 9),
			all = (uint64_t)(mem | fs | gz | net | lib | http | https | shell | env | addons)
		};

		enum class args_format
		{
			key_value = (1 << 0),
			flag_value = (1 << 1),
			key = (1 << 2),
			flag = (1 << 3),
			stop_if_no_match = (1 << 4)
		};

		enum class parser_error
		{
			not_supported,
			bad_version,
			bad_dictionary,
			bad_name_index,
			bad_name,
			bad_key_name,
			bad_key_type,
			bad_value,
			bad_string,
			bad_integer,
			bad_double,
			bad_boolean,
			xml_out_of_memory,
			xml_internal_error,
			xml_unrecognized_tag,
			xml_bad_pi,
			xml_bad_comment,
			xml_bad_cdata,
			xml_bad_doc_type,
			xml_bad_pc_data,
			xml_bad_start_element,
			xml_bad_attribute,
			xml_bad_end_element,
			xml_end_element_mismatch,
			xml_append_invalid_root,
			xml_no_document_element,
			json_document_empty,
			json_document_root_not_singular,
			json_value_invalid,
			json_object_miss_name,
			json_object_miss_colon,
			json_object_miss_comma_or_curly_bracket,
			json_array_miss_comma_or_square_bracket,
			json_string_unicode_escape_invalid_hex,
			json_string_unicode_surrogate_invalid,
			json_string_escape_invalid,
			json_string_miss_quotation_mark,
			json_string_invalid_encoding,
			json_number_too_big,
			json_number_miss_fraction,
			json_number_miss_exponent,
			json_termination,
			json_unspecific_syntax_error
		};

		inline access_option operator |(access_option a, access_option b)
		{
			return static_cast<access_option>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
		}

		template <typename t, typename = void>
		struct is_iterable : std::false_type { };

		template <typename t>
		struct is_iterable<t, std::void_t<decltype(std::begin(std::declval<t&>())), decltype(std::end(std::declval<t&>()))>> : std::true_type { };

		template <typename t>
		struct is_add_refer
		{
			template <typename u>
			static constexpr std::conditional_t<false, decltype(std::declval<u>().add_ref()), std::true_type> signature(int);

			template <typename u>
			static constexpr std::false_type signature(char);

			static constexpr bool value = decltype(signature<t>(0))::value;
		};

		template <typename t>
		struct is_releaser
		{
			template <typename u>
			static constexpr std::conditional_t<false, decltype(std::declval<u>().release()), std::true_type> signature(int);

			template <typename u>
			static constexpr std::false_type signature(char);

			static constexpr bool value = decltype(signature<t>(0))::value;
		};

		template <typename t>
		using unique = t*;

		struct singletonish { };

		struct memory_location
		{
			const char* source;
			const char* function;
			const char* type_name;
			int line;

			memory_location();
			memory_location(const char* new_source, const char* new_function, const char* new_type_name, int new_line);
		};

		class global_allocator
		{
		public:
			virtual ~global_allocator() = default;
			virtual unique<void> allocate(size_t size) noexcept = 0;
			virtual unique<void> allocate(memory_location&& location, size_t size) noexcept = 0;
			virtual void transfer(unique<void> address, size_t size) noexcept = 0;
			virtual void transfer(unique<void> address, memory_location&& location, size_t size) noexcept = 0;
			virtual void free(unique<void> address) noexcept = 0;
			virtual void watch(memory_location&& location, void* address) noexcept = 0;
			virtual void unwatch(void* address) noexcept = 0;
			virtual void finalize() noexcept = 0;
			virtual bool is_valid(void* address) noexcept = 0;
			virtual bool is_finalizable() noexcept = 0;
		};

		class local_allocator
		{
		public:
			virtual ~local_allocator() = default;
			virtual unique<void> allocate(size_t size) noexcept = 0;
			virtual void free(unique<void> address) noexcept = 0;
			virtual void reset() noexcept = 0;
			virtual bool is_valid(void* address) noexcept = 0;
		};

		class memory final : public singletonish
		{
		private:
			struct state
			{
				std::unordered_map<void*, std::pair<memory_location, size_t>> allocations;
				std::mutex mutex;
			};

		private:
			static global_allocator* global;
			static state* context;

		public:
			static unique<void> default_allocate(size_t size) noexcept;
			static unique<void> tracing_allocate(size_t size, memory_location&& location) noexcept;
			static void default_deallocate(unique<void> address) noexcept;
			static void watch(void* address, memory_location&& location) noexcept;
			static void unwatch(void* address) noexcept;
			static void cleanup() noexcept;
			static void set_global_allocator(global_allocator* new_allocator) noexcept;
			static void set_local_allocator(local_allocator* new_allocator) noexcept;
			static bool is_valid_address(void* address) noexcept;
			static global_allocator* get_global_allocator() noexcept;
			static local_allocator* get_local_allocator() noexcept;

		public:
			template <typename t>
			static inline void deinit(t*& value)
			{
				if (value != nullptr)
				{
					value->~t();
					default_deallocate(static_cast<void*>(value));
					value = nullptr;
				}
			}
			template <typename t>
			static inline void deinit(t* const& value)
			{
				if (value != nullptr)
				{
					value->~t();
					default_deallocate(static_cast<void*>(value));
				}
			}
			template <typename t>
			static inline void deallocate(t*& value)
			{
				if (value != nullptr)
				{
					default_deallocate(static_cast<void*>(value));
					value = nullptr;
				}
			}
			template <typename t>
			static inline void deallocate(t* const& value)
			{
				default_deallocate(static_cast<void*>(value));
			}
			template <typename t>
			static inline void release(t*& value)
			{
				if (value != nullptr)
				{
					value->release();
					value = nullptr;
				}
			}
			template <typename t>
			static inline void release(t* const& value)
			{
				if (value != nullptr)
					value->release();
			}

		public:
#ifndef NDEBUG
#ifdef VI_CXX20
			template <typename t>
			static t* allocate(size_t size, const std::source_location& location = std::source_location::current())
			{
				return static_cast<t*>(tracing_allocate(size, memory_location(location.file_name(), location.function_name(), typeid(t).name(), location.line())));
			}
			template <typename t>
			static t* init(const std::source_location& location = std::source_location::current())
			{
				return new(allocate<t>(sizeof(t), location)) t();
			}
			template <typename t, typename P1>
			static t* init(P1&& ph1, const std::source_location& location = std::source_location::current())
			{
				return new(allocate<t>(sizeof(t), location)) t(std::forward<P1>(ph1));
			}
			template <typename t, typename P1, typename P2>
			static t* init(P1&& ph1, P2&& ph2, const std::source_location& location = std::source_location::current())
			{
				return new(allocate<t>(sizeof(t), location)) t(std::forward<P1>(ph1), std::forward<P2>(ph2));
			}
			template <typename t, typename P1, typename P2, typename P3>
			static t* init(P1&& ph1, P2&& ph2, P3&& ph3, const std::source_location& location = std::source_location::current())
			{
				return new(allocate<t>(sizeof(t), location)) t(std::forward<P1>(ph1), std::forward<P2>(ph2), std::forward<P3>(ph3));
			}
			template <typename t, typename P1, typename P2, typename P3, typename P4>
			static t* init(P1&& ph1, P2&& ph2, P3&& ph3, P4&& ph4, const std::source_location& location = std::source_location::current())
			{
				return new(allocate<t>(sizeof(t), location)) t(std::forward<P1>(ph1), std::forward<P2>(ph2), std::forward<P3>(ph3), std::forward<P4>(ph4));
			}
			template <typename t, typename P1, typename P2, typename P3, typename P4, typename P5>
			static t* init(P1&& ph1, P2&& ph2, P3&& ph3, P4&& ph4, P5&& ph5, const std::source_location& location = std::source_location::current())
			{
				return new(allocate<t>(sizeof(t), location)) t(std::forward<P1>(ph1), std::forward<P2>(ph2), std::forward<P3>(ph3), std::forward<P4>(ph4), std::forward<P5>(ph5));
			}
			template <typename t, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
			static t* init(P1&& ph1, P2&& ph2, P3&& ph3, P4&& ph4, P5&& ph5, P6&& ph6, const std::source_location& location = std::source_location::current())
			{
				return new(allocate<t>(sizeof(t), location)) t(std::forward<P1>(ph1), std::forward<P2>(ph2), std::forward<P3>(ph3), std::forward<P4>(ph4), std::forward<P5>(ph5), std::forward<P6>(ph6));
			}
			template <typename t, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6, typename P7>
			static t* init(P1&& ph1, P2&& ph2, P3&& ph3, P4&& ph4, P5&& ph5, P6&& ph6, P7&& ph7, const std::source_location& location = std::source_location::current())
			{
				return new(allocate<t>(sizeof(t), location)) t(std::forward<P1>(ph1), std::forward<P2>(ph2), std::forward<P3>(ph3), std::forward<P4>(ph4), std::forward<P5>(ph5), std::forward<P6>(ph6), std::forward<P7>(ph7));
			}
#else
			template <typename t>
			static t* allocate(size_t size)
			{
				return static_cast<t*>(tracing_allocate(size, memory_location(__FILE__, __func__, typeid(t).name(), __LINE__)));
			}
			template <typename t, typename... args>
			static t* init(args&&... values)
			{
				void* address = static_cast<t*>(tracing_allocate(sizeof(t), memory_location(__FILE__, __func__, typeid(t).name(), __LINE__)));
				return new(address) t(std::forward<args>(values)...);
			}
#endif
#else
			template <typename t>
			static t* allocate(size_t size)
			{
				return static_cast<t*>(default_allocate(size));
			}
			template <typename t, typename... args>
			static t* init(args&&... values)
			{
				return new(allocate<t>(sizeof(t))) t(std::forward<args>(values)...);
			}
#endif
		};

		template <typename t>
		class standard_allocator
		{
		public:
			typedef t value_type;

		public:
			template <typename u>
			struct rebind
			{
				typedef standard_allocator<u> other;
			};

		public:
			standard_allocator() = default;
			~standard_allocator() = default;
			standard_allocator(const standard_allocator&) = default;
			value_type* allocate(size_t count)
			{
				return memory::allocate<t>(count * sizeof(t));
			}
			value_type* allocate(size_t count, const void*)
			{
				return memory::allocate<t>(count * sizeof(t));
			}
			void deallocate(value_type* address, size_t count)
			{
				memory::deallocate<value_type>(address);
			}
			size_t max_size() const
			{
				return std::numeric_limits<size_t>::max() / sizeof(t);
			}
			bool operator== (const standard_allocator&)
			{
				return true;
			}
			bool operator!=(const standard_allocator&)
			{
				return false;
			}

		public:
			template <typename u>
			standard_allocator(const standard_allocator<u>&)
			{
			}
		};

		template <class a, class b>
		bool operator== (const standard_allocator<a>&, const standard_allocator<b>&) noexcept
		{
			return true;
		}

		template <typename t>
		struct allocation_type
		{
#ifdef VI_ALLOCATOR
			using type = standard_allocator<t>;
#else
			using type = std::allocator<t>;
#endif
		};

		template <typename t, t offset_basis, t prime>
		struct fnv1a_hash
		{
			static_assert(std::is_unsigned<t>::value, "Q needs to be uint32_teger");

			inline t operator()(const void* address, size_t size) const noexcept
			{
				const auto data = static_cast<const uint8_t*>(address);
				auto state = offset_basis;
				for (size_t i = 0; i < size; ++i)
					state = (state ^ (size_t)data[i]) * prime;
				return state;
			}
		};

		template <size_t bits>
		struct fnv1a_bits;

		template <>
		struct fnv1a_bits<32> { using type = fnv1a_hash<uint32_t, UINT32_C(2166136261), UINT32_C(16777619)>; };

		template <>
		struct fnv1a_bits<64> { using type = fnv1a_hash<uint64_t, UINT64_C(14695981039346656037), UINT64_C(1099511628211)>; };

		template <size_t bits>
		using FNV1A = typename fnv1a_bits<bits>::type;

		template <typename t>
		struct key_hasher
		{
			typedef float argument_type;
			typedef size_t result_type;

			inline result_type operator()(const t& value) const noexcept
			{
				return std::hash<t>()(value);
			}
		};

		template <typename t>
		struct equal_to
		{
			typedef t first_argument_type;
			typedef t second_argument_type;
			typedef bool result_type;

			inline result_type operator()(const t& left, const t& right) const noexcept
			{
				return std::equal_to<t>()(left, right);
			}
		};

		using string = std::basic_string<std::string::value_type, std::string::traits_type, typename allocation_type<typename std::string::value_type>::type>;
		using wide_string = std::basic_string<std::wstring::value_type, std::wstring::traits_type, typename allocation_type<typename std::wstring::value_type>::type>;
		using string_stream = std::basic_stringstream<std::string::value_type, std::string::traits_type, typename allocation_type<typename std::string::value_type>::type>;
		using wide_string_stream = std::basic_stringstream<std::wstring::value_type, std::wstring::traits_type, typename allocation_type<typename std::wstring::value_type>::type>;

		template <>
		struct equal_to<string>
		{
			typedef string first_argument_type;
			typedef string second_argument_type;
			typedef bool result_type;
			using is_transparent = void;

			inline result_type operator()(const string& left, const string& right) const noexcept
			{
				return left == right;
			}
			inline result_type operator()(const string& left, const char* right) const noexcept
			{
				return left.compare(right) == 0;
			}
			inline result_type operator()(const string& left, const std::string_view& right) const noexcept
			{
				return left == right;
			}
			inline result_type operator()(const char* left, const string& right) const noexcept
			{
				return right.compare(left) == 0;
			}
			inline result_type operator()(const std::string_view& left, const string& right) const noexcept
			{
				return left == right;
			}
		};

		template <>
		struct key_hasher<string>
		{
			typedef float argument_type;
			typedef size_t result_type;
			using is_transparent = void;

			inline result_type operator()(const char* value) const noexcept
			{
				return FNV1A<8 * sizeof(size_t)>()(value, strlen(value));
			}
			inline result_type operator()(const std::string_view& value) const noexcept
			{
				return FNV1A<8 * sizeof(size_t)>()(value.data(), value.size());
			}
			inline result_type operator()(const string& value) const noexcept
			{
				return FNV1A<8 * sizeof(size_t)>()(value.data(), value.size());
			}
		};

		template <>
		struct key_hasher<wide_string>
		{
			typedef float argument_type;
			typedef size_t result_type;

			inline result_type operator()(const wide_string& value) const noexcept
			{
				return FNV1A<8 * sizeof(std::size_t)>()(value.c_str(), value.size());
			}
			inline result_type operator()(const std::wstring_view& value) const noexcept
			{
				return FNV1A<8 * sizeof(std::size_t)>()(value.data(), value.size());
			}
		};

		template <typename t>
		using vector = std::vector<t, typename allocation_type<t>::type>;

		template <typename t>
		using linked_list = std::list<t, typename allocation_type<t>::type>;

		template <typename t>
		using single_queue = std::queue<t, std::deque<t, typename allocation_type<t>::type>>;

		template <typename t>
		using double_queue = std::deque<t, typename allocation_type<t>::type>;

		template <typename k, typename v, typename comparator = typename std::map<k, v>::key_compare>
		using ordered_map = std::map<k, v, comparator, typename allocation_type<typename std::map<k, v>::value_type>::type>;

		template <typename k, typename hash = key_hasher<k>, typename key_equal = equal_to<k>>
		using unordered_set = std::unordered_set<k, hash, key_equal, typename allocation_type<typename std::unordered_set<k>::value_type>::type>;

		template <typename k, typename v, typename hash = key_hasher<k>, typename key_equal = equal_to<k>>
		using unordered_map = std::unordered_map<k, v, hash, key_equal, typename allocation_type<typename std::unordered_map<k, v>::value_type>::type>;

		template <typename k, typename v, typename hash = key_hasher<k>, typename key_equal = equal_to<k>>
		using unordered_multi_map = std::unordered_multimap<k, v, hash, key_equal, typename allocation_type<typename std::unordered_multimap<k, v>::value_type>::type>;

		typedef std::function<void()> task_callback;
		typedef std::function<bool(const std::string_view&)> process_callback;
		typedef std::function<string(const std::string_view&)> schema_name_callback;
		typedef std::function<void(var_form, const std::string_view&)> schema_write_callback;
		typedef std::function<bool(uint8_t*, size_t)> schema_read_callback;
		typedef std::function<bool()> activity_callback;
		typedef std::function<void(task_callback&&)> spawner_callback;
		typedef void(*signal_callback)(int);

		namespace allocators
		{
			class debug_allocator final : public global_allocator
			{
			public:
				struct tracing_info
				{
					std::thread::id thread;
					std::string type_name;
					memory_location location;
					time_t time;
					size_t size;
					bool active;
					bool constant;

					tracing_info();
					tracing_info(const char* new_type_name, memory_location&& new_location, time_t new_time, size_t new_size, bool is_active, bool is_static);
				};

			private:
				std::unordered_map<void*, tracing_info> blocks;
				std::unordered_map<void*, tracing_info> watchers;
				std::recursive_mutex mutex;

			public:
				~debug_allocator() override = default;
				unique<void> allocate(size_t size) noexcept override;
				unique<void> allocate(memory_location&& origin, size_t size) noexcept override;
				void free(unique<void> address) noexcept override;
				void transfer(unique<void> address, size_t size) noexcept override;
				void transfer(unique<void> address, memory_location&& origin, size_t size) noexcept override;
				void watch(memory_location&& origin, void* address) noexcept override;
				void unwatch(void* address) noexcept override;
				void finalize() noexcept override;
				bool is_valid(void* address) noexcept override;
				bool is_finalizable() noexcept override;
				bool dump(void* address);
				bool find_block(void* address, tracing_info* output);
				const std::unordered_map<void*, tracing_info>& get_blocks() const;
				const std::unordered_map<void*, tracing_info>& get_watchers() const;
			};

			class default_allocator final : public global_allocator
			{
			public:
				~default_allocator() override = default;
				unique<void> allocate(size_t size) noexcept override;
				unique<void> allocate(memory_location&& origin, size_t size) noexcept override;
				void free(unique<void> address) noexcept override;
				void transfer(unique<void> address, size_t size) noexcept override;
				void transfer(unique<void> address, memory_location&& origin, size_t size) noexcept override;
				void watch(memory_location&& origin, void* address) noexcept override;
				void unwatch(void* address) noexcept override;
				void finalize() noexcept override;
				bool is_valid(void* address) noexcept override;
				bool is_finalizable() noexcept override;
			};

			class cached_allocator final : public global_allocator
			{
			private:
				struct page_cache;

				using page_group = std::vector<page_cache*>;

				struct page_address
				{
					page_cache* cache;
					void* address;
				};

				struct page_cache
				{
					std::vector<page_address*> addresses;
					page_group& page;
					int64_t timing;
					size_t capacity;

					inline page_cache(page_group& new_page, int64_t time, size_t size) : page(new_page), timing(time), capacity(size)
					{
						addresses.resize(capacity);
					}
					~page_cache() = default;
				};

			private:
				std::unordered_map<size_t, page_group> pages;
				std::recursive_mutex mutex;
				uint64_t minimal_life_time;
				double elements_reducing_factor;
				size_t elements_reducing_base;
				size_t elements_per_allocation;

			public:
				cached_allocator(uint64_t minimal_life_time_ms = 2000, size_t max_elements_per_allocation = 1024, size_t elements_reducing_base_bytes = 300, double elements_reducing_factor_rate = 5.0);
				~cached_allocator() noexcept override;
				unique<void> allocate(size_t size) noexcept override;
				unique<void> allocate(memory_location&& origin, size_t size) noexcept override;
				void free(unique<void> address) noexcept override;
				void transfer(unique<void> address, size_t size) noexcept override;
				void transfer(unique<void> address, memory_location&& origin, size_t size) noexcept override;
				void watch(memory_location&& origin, void* address) noexcept override;
				void unwatch(void* address) noexcept override;
				void finalize() noexcept override;
				bool is_valid(void* address) noexcept override;
				bool is_finalizable() noexcept override;

			private:
				page_cache* get_page_cache(size_t size);
				int64_t get_clock();
				size_t get_elements_count(page_group& page, size_t size);
			};

			class linear_allocator final : public local_allocator
			{
			private:
				struct region
				{
					char* free_address;
					char* base_address;
					region* upper_address;
					region* lower_address;
					size_t size;
				};

			private:
				region* top;
				region* bottom;
				size_t latest_size;
				size_t sizing;

			public:
				linear_allocator(size_t size);
				~linear_allocator() noexcept override;
				unique<void> allocate(size_t size) noexcept override;
				void free(unique<void> address) noexcept override;
				void reset() noexcept override;
				bool is_valid(void* address) noexcept override;
				size_t get_leftovers() const noexcept;

			private:
				void next_region(size_t size) noexcept;
				void flush_regions() noexcept;
			};

			class stack_allocator final : public local_allocator
			{
			private:
				struct region
				{
					char* free_address;
					char* base_address;
					region* upper_address;
					region* lower_address;
					size_t size;
				};

			private:
				region* top;
				region* bottom;
				size_t sizing;

			public:
				stack_allocator(size_t size);
				~stack_allocator() noexcept override;
				unique<void> allocate(size_t size) noexcept override;
				void free(unique<void> address) noexcept override;
				void reset() noexcept override;
				bool is_valid(void* address) noexcept override;
				size_t get_leftovers() const noexcept;

			private:
				void next_region(size_t size) noexcept;
				void flush_regions() noexcept;
			};
		}

		class stack_trace
		{
		public:
			struct frame
			{
				string code;
				string function;
				string file;
				uint32_t line;
				uint32_t column;
				void* handle;
				bool native;
			};

		public:
			typedef vector<frame> stack_ptr;

		private:
			stack_ptr frames;

		public:
			stack_trace(size_t skips = 0, size_t max_depth = 64);
			stack_ptr::const_iterator begin() const;
			stack_ptr::const_iterator end() const;
			stack_ptr::const_reverse_iterator rbegin() const;
			stack_ptr::const_reverse_iterator rend() const;
			explicit operator bool() const;
			const stack_ptr& range() const;
			bool empty() const;
			size_t size() const;
		};

		class error_handling final : public singletonish
		{
		public:
			struct details
			{
				struct
				{
					const char* file = nullptr;
					int line = 0;
				} origin;

				struct
				{
					log_level level = log_level::info;
					bool fatal = false;
				} type;

				struct
				{
					char data[BLOB_SIZE] = { '\0' };
					char date[64] = { '\0' };
					size_t size = 0;
				} message;
			};

			struct tick
			{
				bool is_counting;

				tick(bool active) noexcept;
				tick(const tick& other) = delete;
				tick(tick&& other) noexcept;
				~tick() noexcept;
				tick& operator =(const tick& other) = delete;
				tick& operator =(tick&& other) noexcept;
			};

		private:
			struct state
			{
				std::function<void(details&)> callback;
				uint32_t flags = (uint32_t)log_option::pretty;
			};

		private:
			static state* context;

		public:
			static void panic(int line, const char* source, const char* function, const char* condition, const char* format, ...) noexcept;
			static void assertion(int line, const char* source, const char* function, const char* condition, const char* format, ...) noexcept;
			static void message(log_level level, int line, const char* source, const char* format, ...) noexcept;
			static void pause() noexcept;
			static void cleanup() noexcept;
			static void set_callback(std::function<void(details&)>&& callback) noexcept;
			static void set_flag(log_option option, bool active) noexcept;
			static bool has_flag(log_option option) noexcept;
			static bool has_callback() noexcept;
			static string get_stack_trace(size_t skips, size_t max_frames = 64) noexcept;
			static string get_measure_trace() noexcept;
			static tick measure(const char* file, const char* function, int line, uint64_t threshold_ms) noexcept;
			static void measure_loop() noexcept;
			static std::string_view get_message_type(const details& base) noexcept;
			static std_color get_message_color(const details& base) noexcept;
			static string get_message_text(const details& base) noexcept;

		private:
			static void enqueue(details&& data) noexcept;
			static void dispatch(details& data) noexcept;
			static void colorify(console* terminal, details& data) noexcept;
		};

		class option_utils
		{
		public:
			template <typename q>
			static typename std::enable_if<std::is_trivial<q>::value, void>::type move_buffer(char* dest, char* src, size_t size)
			{
				memcpy(dest, src, size);
			}
			template <typename q>
			static typename std::enable_if<!std::is_trivial<q>::value, void>::type move_buffer(char* dest, char* src, size_t)
			{
				new(dest) q(std::move(*(q*)src));
			}
			template <typename q>
			static typename std::enable_if<std::is_trivial<q>::value, void>::type copy_buffer(char* dest, const char* src, size_t size)
			{
				memcpy(dest, src, size);
			}
			template <typename q>
			static typename std::enable_if<!std::is_trivial<q>::value, void>::type copy_buffer(char* dest, const char* src, size_t)
			{
				new(dest) q(*(q*)src);
			}
			template <typename q>
			static typename std::enable_if<std::is_pointer<q>::value, const q>::type to_const_pointer(const char* src)
			{
				q& reference = *(q*)src;
				return reference;
			}
			template <typename q>
			static typename std::enable_if<!std::is_pointer<q>::value, const q*>::type to_const_pointer(const char* src)
			{
				q& reference = *(q*)src;
				return &reference;
			}
			template <typename q>
			static typename std::enable_if<std::is_pointer<q>::value, q>::type to_pointer(char* src)
			{
				q& reference = *(q*)src;
				return reference;
			}
			template <typename q>
			static typename std::enable_if<!std::is_pointer<q>::value, q*>::type to_pointer(char* src)
			{
				q& reference = *(q*)src;
				return &reference;
			}
			template <typename q>
			static typename std::enable_if<std::is_base_of<std::exception, q>::value, string>::type to_error_text(const char* src, bool is_error)
			{
				return string(is_error ? ((q*)src)->what() : "accessing a none value");
			}
			template <typename q>
			static typename std::enable_if<std::is_same<std::error_code, q>::value, std::string>::type to_error_text(const char* src, bool is_error)
			{
				return is_error ? ((q*)src)->message() : std::string("accessing a none value");
			}
			template <typename q>
			static typename std::enable_if<std::is_same<std::error_condition, q>::value, std::string>::type to_error_text(const char* src, bool is_error)
			{
				return is_error ? ((q*)src)->message() : std::string("accessing a none value");
			}
			template <typename q>
			static typename std::enable_if<std::is_same<std::string_view, q>::value, string>::type to_error_text(const char* src, bool is_error)
			{
				return string(is_error ? *(q*)src : q("accessing a none value"));
			}
			template <typename q>
			static typename std::enable_if<std::is_same<std::string, q>::value, std::string>::type to_error_text(const char* src, bool is_error)
			{
				return is_error ? *(q*)src : q("accessing a none value");
			}
#ifdef VI_ALLOCATOR
			template <typename q>
			static typename std::enable_if<std::is_same<string, q>::value, string>::type to_error_text(const char* src, bool is_error)
			{
				return is_error ? *(q*)src : q("accessing a none value");
			}
#endif
			template <typename q>
			static typename std::enable_if<!std::is_base_of<std::exception, q>::value && !std::is_same<std::error_code, q>::value && !std::is_same<std::error_condition, q>::value && !std::is_same<std::string, q>::value && !std::is_same<string, q>::value, string>::type to_error_text(const char* src, bool is_error)
			{
				return string(is_error ? "*unknown error value*" : "accessing a none value");
			}
		};

		class basic_exception : public std::exception
		{
		protected:
			string error_message;

		public:
			basic_exception() = default;
			basic_exception(const std::string_view& new_message) noexcept;
			basic_exception(string&& new_message) noexcept;
			basic_exception(const basic_exception&) = default;
			basic_exception(basic_exception&&) = default;
			basic_exception& operator= (const basic_exception&) = default;
			basic_exception& operator= (basic_exception&&) = default;
			virtual ~basic_exception() = default;
			virtual const char* type() const noexcept = 0;
			virtual const char* what() const noexcept;
			virtual const string& message() const& noexcept;
			virtual const string&& message() const&& noexcept;
			virtual string& message() & noexcept;
			virtual string&& message() && noexcept;
		};

		class parser_exception final : public basic_exception
		{
		private:
			parser_error error_type;
			size_t error_offset;

		public:
			parser_exception(parser_error new_type);
			parser_exception(parser_error new_type, size_t new_offset);
			parser_exception(parser_error new_type, size_t new_offset, const std::string_view& new_message);
			const char* type() const noexcept override;
			parser_error status() const noexcept;
			size_t offset() const noexcept;
		};

		class system_exception : public basic_exception
		{
		private:
			std::error_condition error_condition;

		public:
			system_exception();
			system_exception(const std::string_view& message);
			system_exception(const std::string_view& message, std::error_condition&& condition);
			virtual const char* type() const noexcept override;
			virtual const std::error_condition& error() const& noexcept;
			virtual const std::error_condition&& error() const&& noexcept;
			virtual std::error_condition& error() & noexcept;
			virtual std::error_condition&& error() && noexcept;
		};

		template <typename t>
		class bitmask
		{
			static_assert(std::is_integral<t>::value, "value should be of integral type to have bitmask applications");

		public:
			static t mark(t other)
			{
				return other | (t)highbit<t>();
			}
			static t unmark(t other)
			{
				return other & (t)lowbit<t>();
			}
			static t value(t other)
			{
				return other & (t)lowbit<t>();
			}
			static bool is_marked(t other)
			{
				return (bool)(other & (t)highbit<t>());
			}

		private:
			template <typename q>
			static typename std::enable_if<sizeof(q) == sizeof(uint64_t), uint64_t>::type highbit()
			{
				return 0x8000000000000000;
			}
			template <typename q>
			static typename std::enable_if < sizeof(q) < sizeof(uint64_t), uint32_t > ::type highbit()
			{
				return 0x80000000;
			}
			template <typename q>
			static typename std::enable_if<sizeof(q) == sizeof(uint64_t), uint64_t>::type lowbit()
			{
				return 0x7FFFFFFFFFFFFFFF;
			}
			template <typename q>
			static typename std::enable_if < sizeof(q) < sizeof(uint64_t), uint32_t > ::type lowbit()
			{
				return 0x7FFFFFFF;
			}
		};

		template <typename v>
		class option
		{
			static_assert(!std::is_same<v, void>::value, "value type should not be void");

		private:
			alignas(v) char value[sizeof(v)];
			int8_t status;
#ifndef NDEBUG
			const v* hidden_value = (const v*)value;
#endif
		public:
			option(optional type) : status((int8_t)type)
			{
				VI_ASSERT(type == optional::none, "only none is accepted for this constructor");
			}
			option(const v& other) : status(1)
			{
				option_utils::copy_buffer<v>(value, (const char*)&other, sizeof(v));
			}
			option(v&& other) noexcept : status(1)
			{
				option_utils::move_buffer<v>(value, (char*)&other, sizeof(v));
			}
			option(const option& other) : status(other.status)
			{
				if (status > 0)
					option_utils::copy_buffer<v>(value, other.value, sizeof(value));
			}
			option(option&& other) noexcept : status(other.status)
			{
				if (status > 0)
					option_utils::move_buffer<v>(value, other.value, sizeof(value));
			}
			~option()
			{
				if (status > 0)
					((v*)value)->~v();
				status = 0;
			}
			option& operator= (optional type)
			{
				VI_ASSERT(type == optional::none, "only none is accepted for this operator");
				this->~option();
				status = (int8_t)type;
				return *this;
			}
			option& operator= (const v& other)
			{
				this->~option();
				option_utils::copy_buffer<v>(value, (const char*)&other, sizeof(v));
				status = 1;
				return *this;
			}
			option& operator= (v&& other) noexcept
			{
				this->~option();
				option_utils::move_buffer<v>(value, (char*)&other, sizeof(v));
				status = 1;
				return *this;
			}
			option& operator= (const option& other)
			{
				if (this == &other)
					return *this;

				this->~option();
				status = other.status;
				if (status > 0)
					option_utils::copy_buffer<v>(value, other.value, sizeof(value));

				return *this;
			}
			option& operator= (option&& other) noexcept
			{
				if (this == &other)
					return *this;

				this->~option();
				status = other.status;
				if (status > 0)
					option_utils::move_buffer<v>(value, other.value, sizeof(value));

				return *this;
			}
			const v& expect(const std::string_view& message) const&
			{
				VI_PANIC(is_value(), "%.*s CAUSING unwrapping an empty value", (int)message.size(), message.data());
				const v& reference = *(v*)value;
				return reference;
			}
			const v&& expect(const std::string_view& message) const&&
			{
				VI_PANIC(is_value(), "%.*s CAUSING unwrapping an empty value", (int)message.size(), message.data());
				v& reference = *(v*)value;
				return std::move(reference);
			}
			v& expect(const std::string_view& message)&
			{
				VI_PANIC(is_value(), "%.*s CAUSING unwrapping an empty value", (int)message.size(), message.data());
				v& reference = *(v*)value;
				return reference;
			}
			v&& expect(const std::string_view& message)&&
			{
				VI_PANIC(is_value(), "%.*s CAUSING unwrapping an empty value", (int)message.size(), message.data());
				v& reference = *(v*)value;
				return std::move(reference);
			}
			const v& otherwise(const v & if_none) const&
			{
				if (!is_value())
					return if_none;

				const v& reference = *(v*)value;
				return reference;
			}
			const v&& otherwise(const v && if_none) const&&
			{
				if (!is_value())
					return std::move(if_none);

				v& reference = *(v*)value;
				return std::move(reference);
			}
			v& otherwise(v & if_none)&
			{
				if (!is_value())
					return if_none;

				v& reference = *(v*)value;
				return reference;
			}
			v&& otherwise(v && if_none)&&
			{
				if (!is_value())
					return std::move(if_none);

				v& reference = *(v*)value;
				return std::move(reference);
			}
			const v& unwrap() const&
			{
				VI_PANIC(is_value(), "unwrapping an empty value");
				const v& reference = *(v*)value;
				return reference;
			}
			const v&& unwrap() const&&
			{
				VI_PANIC(is_value(), "unwrapping an empty value");
				v& reference = *(v*)value;
				return std::move(reference);
			}
			v& unwrap()&
			{
				VI_PANIC(is_value(), "unwrapping an empty value");
				v& reference = *(v*)value;
				return reference;
			}
			v&& unwrap()&&
			{
				VI_PANIC(is_value(), "unwrapping an empty value");
				v& reference = *(v*)value;
				return std::move(reference);
			}
			const v* address() const
			{
				const v* reference = is_value() ? (v*)value : nullptr;
				return reference;
			}
			v* address()
			{
				v* reference = is_value() ? (v*)value : nullptr;
				return reference;
			}
			void reset()
			{
				this->~option();
				status = 0;
			}
			bool is_none() const
			{
				return status == 0;
			}
			bool is_value() const
			{
				return status > 0;
			}
			explicit operator bool() const
			{
				return is_value();
			}
			explicit operator optional() const
			{
				return (optional)status;
			}
			const v& operator* () const&
			{
				VI_PANIC(is_value(), "unwrapping an empty value");
				const v& reference = *(v*)value;
				return reference;
			}
			const v&& operator* () const&&
			{
				VI_PANIC(is_value(), "unwrapping an empty value");
				v& reference = *(v*)value;
				return std::move(reference);
			}
			v& operator* ()&
			{
				VI_PANIC(is_value(), "unwrapping an empty value");
				v& reference = *(v*)value;
				return reference;
			}
			v&& operator* ()&&
			{
				VI_PANIC(is_value(), "unwrapping an empty value");
				v& reference = *(v*)value;
				return std::move(reference);
			}
			const typename std::remove_pointer<v>::type* operator-> () const
			{
				VI_ASSERT(is_value(), "unwrapping an empty value");
				const auto* reference = option_utils::to_const_pointer<v>(value);
				return reference;
			}
			typename std::remove_pointer<v>::type* operator-> ()
			{
				VI_ASSERT(is_value(), "unwrapping an empty value");
				auto* reference = option_utils::to_pointer<v>(value);
				return reference;
			}
		};

		template <>
		class option<void>
		{
		private:
			int8_t status;

		public:
			option(optional type) : status((int8_t)type)
			{
			}
			option(const option&) = default;
			option(option&&) = default;
			~option() = default;
			option& operator= (optional type)
			{
				status = (int8_t)type;
				return *this;
			}
			option& operator= (const option&) = default;
			option& operator= (option&&) = default;
			void expect(const std::string_view& message) const
			{
				VI_PANIC(is_value(), "%.*s CAUSING unwrapping an empty value", (int)message.size(), message.data());
			}
			void unwrap() const
			{
				VI_PANIC(is_value(), "unwrapping an empty value");
			}
			void address() const
			{
			}
			void reset()
			{
				status = 0;
			}
			bool is_none() const
			{
				return status == 0;
			}
			bool is_value() const
			{
				return status > 0;
			}
			explicit operator bool() const
			{
				return is_value();
			}
			explicit operator optional() const
			{
				return (optional)status;
			}
		};

		template <typename v, typename e>
		class expects
		{
			static_assert(!std::is_same<e, void>::value, "error type should not be void");
			static_assert(!std::is_same<e, v>::value, "error type should not be value type");

		private:
			template <typename... types>
			struct storage
			{
				alignas(types...) char buffer[std::max({ sizeof(types)... })];
			};

		private:
			storage<v, e> value;
			int8_t status;
#ifndef NDEBUG
			const v* hidden_value = (const v*)&value;
			const e* hidden_error = (const e*)&value;
#endif
		public:
			expects(const v& other) : status(1)
			{
				option_utils::copy_buffer<v>(value.buffer, (const char*)&other, sizeof(v));
			}
			expects(v&& other) noexcept : status(1)
			{
				option_utils::move_buffer<v>(value.buffer, (char*)&other, sizeof(v));
			}
			expects(const e& other) noexcept : status(-1)
			{
				option_utils::copy_buffer<e>(value.buffer, (const char*)&other, sizeof(e));
			}
			expects(e&& other) noexcept : status(-1)
			{
				option_utils::move_buffer<e>(value.buffer, (char*)&other, sizeof(e));
			}
			expects(const expects& other) : status(other.status)
			{
				if (status > 0)
					option_utils::copy_buffer<v>(value.buffer, other.value.buffer, sizeof(value.buffer));
				else if (status < 0)
					option_utils::copy_buffer<e>(value.buffer, other.value.buffer, sizeof(value.buffer));
			}
			expects(expects&& other) noexcept : status(other.status)
			{
				if (status > 0)
					option_utils::move_buffer<v>(value.buffer, other.value.buffer, sizeof(value.buffer));
				else if (status < 0)
					option_utils::move_buffer<e>(value.buffer, other.value.buffer, sizeof(value.buffer));
			}
			~expects()
			{
				if (status > 0)
					((v*)value.buffer)->~v();
				else if (status < 0)
					((e*)value.buffer)->~e();
				status = 0;
			}
			expects& operator= (const v& other)
			{
				this->~expects();
				option_utils::copy_buffer<v>(value.buffer, (const char*)&other, sizeof(v));
				status = 1;
				return *this;
			}
			expects& operator= (v&& other) noexcept
			{
				this->~expects();
				option_utils::move_buffer<v>(value.buffer, (char*)&other, sizeof(v));
				status = 1;
				return *this;
			}
			expects& operator= (const e& other)
			{
				this->~expects();
				option_utils::copy_buffer<e>(value.buffer, (const char*)&other, sizeof(e));
				status = -1;
				return *this;
			}
			expects& operator= (e&& other) noexcept
			{
				this->~expects();
				option_utils::move_buffer<e>(value.buffer, (char*)&other, sizeof(e));
				status = -1;
				return *this;
			}
			expects& operator= (const expects& other)
			{
				if (this == &other)
					return *this;

				this->~expects();
				status = other.status;
				if (status > 0)
					option_utils::copy_buffer<v>(value.buffer, other.value.buffer, sizeof(value.buffer));
				else if (status < 0)
					option_utils::copy_buffer<e>(value.buffer, other.value.buffer, sizeof(value.buffer));

				return *this;
			}
			expects& operator= (expects&& other) noexcept
			{
				if (this == &other)
					return *this;

				this->~expects();
				status = other.status;
				if (status > 0)
					option_utils::move_buffer<v>(value.buffer, other.value.buffer, sizeof(value.buffer));
				else if (status < 0)
					option_utils::move_buffer<e>(value.buffer, other.value.buffer, sizeof(value.buffer));

				return *this;
			}
			const v& expect(const std::string_view& message) const&
			{
				VI_PANIC(is_value(), "%s CAUSING %.*s", option_utils::to_error_text<e>(value.buffer, is_error()).c_str(), (int)message.size(), message.data());
				const v& reference = *(v*)value.buffer;
				return reference;
			}
			const v&& expect(const std::string_view& message) const&&
			{
				VI_PANIC(is_value(), "%s CAUSING %.*s", option_utils::to_error_text<e>(value.buffer, is_error()).c_str(), (int)message.size(), message.data());
				v& reference = *(v*)value.buffer;
				return std::move(reference);
			}
			v& expect(const std::string_view& message)&
			{
				VI_PANIC(is_value(), "%s CAUSING %.*s", option_utils::to_error_text<e>(value.buffer, is_error()).c_str(), (int)message.size(), message.data());
				v& reference = *(v*)value.buffer;
				return reference;
			}
			v&& expect(const std::string_view& message)&&
			{
				VI_PANIC(is_value(), "%s CAUSING %.*s", option_utils::to_error_text<e>(value.buffer, is_error()).c_str(), (int)message.size(), message.data());
				v& reference = *(v*)value.buffer;
				return std::move(reference);
			}
			const v& otherwise(const v & if_none) const&
			{
				if (!is_value())
					return if_none;

				const v& reference = *(v*)value.buffer;
				return reference;
			}
			const v&& otherwise(const v && if_none) const&&
			{
				if (!is_value())
					return std::move(if_none);

				v& reference = *(v*)value.buffer;
				return std::move(reference);
			}
			v& otherwise(v & if_none)&
			{
				if (!is_value())
					return if_none;

				v& reference = *(v*)value.buffer;
				return reference;
			}
			v&& otherwise(v && if_none)&&
			{
				if (!is_value())
					return std::move(if_none);

				v& reference = *(v*)value.buffer;
				return std::move(reference);
			}
			const v& unwrap() const&
			{
				VI_PANIC(is_value(), "%s CAUSING unwrapping an empty value", option_utils::to_error_text<e>(value.buffer, is_error()).c_str());
				const v& reference = *(v*)value.buffer;
				return reference;
			}
			const v&& unwrap() const&&
			{
				VI_PANIC(is_value(), "%s CAUSING unwrapping an empty value", option_utils::to_error_text<e>(value.buffer, is_error()).c_str());
				v& reference = *(v*)value.buffer;
				return std::move(reference);
			}
			v& unwrap()&
			{
				VI_PANIC(is_value(), "%s CAUSING unwrapping an empty value", option_utils::to_error_text<e>(value.buffer, is_error()).c_str());
				v& reference = *(v*)value.buffer;
				return reference;
			}
			v&& unwrap()&&
			{
				VI_PANIC(is_value(), "%s CAUSING unwrapping an empty value", option_utils::to_error_text<e>(value.buffer, is_error()).c_str());
				v& reference = *(v*)value.buffer;
				return std::move(reference);
			}
			const v* address() const
			{
				const v* reference = is_value() ? (v*)value.buffer : nullptr;
				return reference;
			}
			v* address()
			{
				v* reference = is_value() ? (v*)value.buffer : nullptr;
				return reference;
			}
			const e& error() const&
			{
				VI_PANIC(is_error(), "value does not contain any errors");
				const e& reference = *(e*)value.buffer;
				return reference;
			}
			const e&& error() const&&
			{
				VI_PANIC(is_error(), "value does not contain an error");
				e& reference = *(e*)value.buffer;
				return std::move(reference);
			}
			e& error()&
			{
				VI_PANIC(is_error(), "value does not contain an error");
				e& reference = *(e*)value.buffer;
				return reference;
			}
			e&& error()&&
			{
				VI_PANIC(is_error(), "value does not contain an error");
				e& reference = *(e*)value.buffer;
				return std::move(reference);
			}
			string what() const
			{
				VI_ASSERT(!is_value(), "value does not contain an error");
				auto reason = option_utils::to_error_text<e>(value.buffer, is_error());
				return string(reason.begin(), reason.end());
			}
			void report(const std::string_view& message) const
			{
				if (is_error())
					VI_ERR("%s CAUSING %.*s", option_utils::to_error_text<e>(value.buffer, is_error()).c_str(), (int)message.size(), message.data());
			}
			void reset()
			{
				this->~expects();
				status = 0;
			}
			bool is_none() const
			{
				return status == 0;
			}
			bool is_value() const
			{
				return status > 0;
			}
			bool is_error() const
			{
				return status < 0;
			}
			explicit operator bool() const
			{
				return is_value();
			}
			explicit operator expectation() const
			{
				return (expectation)status;
			}
			const v& operator* () const&
			{
				VI_PANIC(is_value(), "%s CAUSING unwrapping an empty value", option_utils::to_error_text<e>(value.buffer, is_error()).c_str());
				const v& reference = *(v*)value.buffer;
				return reference;
			}
			const v&& operator* () const&&
			{
				VI_PANIC(is_value(), "%s CAUSING unwrapping an empty value", option_utils::to_error_text<e>(value.buffer, is_error()).c_str());
				v& reference = *(v*)value.buffer;
				return std::move(reference);
			}
			v& operator* ()&
			{
				VI_PANIC(is_value(), "%s CAUSING unwrapping an empty value", option_utils::to_error_text<e>(value.buffer, is_error()).c_str());
				v& reference = *(v*)value.buffer;
				return reference;
			}
			v&& operator* ()&&
			{
				VI_PANIC(is_value(), "%s CAUSING unwrapping an empty value", option_utils::to_error_text<e>(value.buffer, is_error()).c_str());
				v& reference = *(v*)value.buffer;
				return std::move(reference);
			}
			const typename std::remove_pointer<v>::type* operator-> () const
			{
				VI_ASSERT(is_value(), "%s CAUSING unwrapping an empty value", option_utils::to_error_text<e>(value.buffer, is_error()).c_str());
				auto* reference = option_utils::to_const_pointer<v>(value.buffer);
				return reference;
			}
			typename std::remove_pointer<v>::type* operator-> ()
			{
				VI_ASSERT(is_value(), "%s CAUSING unwrapping an empty value", option_utils::to_error_text<e>(value.buffer, is_error()).c_str());
				auto* reference = option_utils::to_pointer<v>(value.buffer);
				return reference;
			}
		};

		template <typename e>
		class expects<void, e>
		{
			static_assert(!std::is_same<e, void>::value, "error type should not be void");

		private:
			alignas(e) char value[sizeof(e)];
			int8_t status;
#ifndef NDEBUG
			const e* hidden_error = (const e*)&value;
#endif
		public:
			expects(expectation type) : status((int8_t)type)
			{
				VI_ASSERT(type == expectation::met, "only met is accepted for this constructor");
			}
			expects(const e& other) noexcept : status(-1)
			{
				option_utils::copy_buffer<e>(value, (const char*)&other, sizeof(e));
			}
			expects(e&& other) noexcept : status(-1)
			{
				option_utils::move_buffer<e>(value, (char*)&other, sizeof(e));
			}
			expects(const expects& other) : status(other.status)
			{
				if (status < 0)
					option_utils::copy_buffer<e>(value, other.value, sizeof(value));
			}
			expects(expects&& other) noexcept : status(other.status)
			{
				if (status < 0)
					option_utils::move_buffer<e>(value, other.value, sizeof(value));
			}
			~expects()
			{
				if (status < 0)
					((e*)value)->~e();
				status = 0;
			}
			expects& operator= (expectation type)
			{
				VI_ASSERT(type == expectation::met, "only met is accepted for this constructor");
				this->~expects();
				status = (int8_t)type;
				return *this;
			}
			expects& operator= (const e& other)
			{
				this->~expects();
				option_utils::copy_buffer<e>(value, (const char*)&other, sizeof(e));
				status = -1;
				return *this;
			}
			expects& operator= (e&& other) noexcept
			{
				this->~expects();
				option_utils::move_buffer<e>(value, (char*)&other, sizeof(e));
				status = -1;
				return *this;
			}
			expects& operator= (const expects& other)
			{
				if (this == &other)
					return *this;

				this->~expects();
				status = other.status;
				if (status < 0)
					option_utils::copy_buffer<e>(value, other.value, sizeof(value));

				return *this;
			}
			expects& operator= (expects&& other) noexcept
			{
				if (this == &other)
					return *this;

				this->~expects();
				status = other.status;
				if (status < 0)
					option_utils::move_buffer<e>(value, other.value, sizeof(value));

				return *this;
			}
			void expect(const std::string_view& message) const
			{
				VI_PANIC(is_value(), "%s CAUSING %.*s", option_utils::to_error_text<e>(value, is_error()).c_str(), (int)message.size(), message.data());
			}
			void unwrap() const
			{
				VI_PANIC(is_value(), "%s CAUSING unwrapping an empty value", option_utils::to_error_text<e>(value, is_error()).c_str());
			}
			void address() const
			{
			}
			const e& error() const&
			{
				VI_PANIC(is_error(), "value does not contain any errors");
				const e& reference = *(e*)value;
				return reference;
			}
			const e&& error() const&&
			{
				VI_PANIC(is_error(), "value does not contain an error");
				e& reference = *(e*)value;
				return std::move(reference);
			}
			e& error()&
			{
				VI_PANIC(is_error(), "value does not contain an error");
				e& reference = *(e*)value;
				return reference;
			}
			e&& error()&&
			{
				VI_PANIC(is_error(), "value does not contain an error");
				e& reference = *(e*)value;
				return std::move(reference);
			}
			string what() const
			{
				VI_ASSERT(!is_value(), "value does not contain an error");
				auto reason = option_utils::to_error_text<e>(value, is_error());
				return string(reason.begin(), reason.end());
			}
			void report(const std::string_view& message) const
			{
				if (is_error())
					VI_ERR("%s CAUSING %.*s", option_utils::to_error_text<e>(value, is_error()).c_str(), (int)message.size(), message.data());
			}
			void reset()
			{
				this->~expects();
				status = 0;
			}
			bool is_none() const
			{
				return status == 0;
			}
			bool is_value() const
			{
				return status > 0;
			}
			bool is_error() const
			{
				return status < 0;
			}
			explicit operator bool() const
			{
				return !is_error();
			}
			explicit operator expectation() const
			{
				return (expectation)status;
			}
		};

		template <typename v>
		using expects_io = expects<v, std::error_condition>;

		template <typename v>
		using expects_parser = expects<v, parser_exception>;

		template <typename v>
		using expects_system = expects<v, system_exception>;

		struct coroutine
		{
			friend costate;

		private:
			std::atomic<coexecution> state;
			task_callback callback;
			cocontext* slave;
			costate* master;

		public:
			task_callback defer;
			void* user_data;

		public:
			coroutine(costate* base, task_callback&& procedure) noexcept;
			~coroutine() noexcept;
		};

		struct decimal
		{
		private:
			string source;
			int32_t length;
			int8_t sign;

		public:
			decimal() noexcept;
			decimal(const std::string_view& value) noexcept;
			decimal(const decimal& value) noexcept;
			decimal(decimal&& value) noexcept;
			decimal& truncate(uint32_t value);
			decimal& round(uint32_t value);
			decimal& trim();
			decimal& unlead();
			decimal& untrail();
			bool is_nan() const;
			bool is_zero() const;
			bool is_zero_or_nan() const;
			bool is_positive() const;
			bool is_negative() const;
			bool is_integer() const;
			bool is_fractional() const;
			bool is_safe_number() const;
			double to_double() const;
			float to_float() const;
			int8_t to_int8() const;
			uint8_t to_uint8() const;
			int16_t to_int16() const;
			uint16_t to_uint16() const;
			int32_t to_int32() const;
			uint32_t to_uint32() const;
			int64_t to_int64() const;
			uint64_t to_uint64() const;
			string to_string() const;
			string to_exponent() const;
			const string& numeric() const;
			uint32_t decimal_places() const;
			uint32_t integer_places() const;
			uint32_t size() const;
			int8_t position() const;
			decimal operator -() const;
			decimal& operator *=(const decimal& v);
			decimal& operator /=(const decimal& v);
			decimal& operator +=(const decimal& v);
			decimal& operator -=(const decimal& v);
			decimal& operator= (const decimal& value) noexcept;
			decimal& operator= (decimal&& value) noexcept;
			decimal& operator++ (int);
			decimal& operator++ ();
			decimal& operator-- (int);
			decimal& operator-- ();
			bool operator== (const decimal& right) const;
			bool operator!= (const decimal& right) const;
			bool operator> (const decimal& right) const;
			bool operator>= (const decimal& right) const;
			bool operator< (const decimal& right) const;
			bool operator<= (const decimal& right) const;
			explicit operator double() const
			{
				return to_double();
			}
			explicit operator float() const
			{
				return to_float();
			}
			explicit operator int64_t() const
			{
				return to_int64();
			}
			explicit operator uint64_t() const
			{
				return to_uint64();
			}

		private:
			void apply_base10(const std::string_view& text);
			void apply_zero();

		public:
			template <typename t, typename = typename std::enable_if<std::is_arithmetic<t>::value, t>::type>
			decimal(const t& right) noexcept : decimal()
			{
				if (right != t(0))
					apply_base10(std::to_string(right));
				else
					apply_zero();
			}

		public:
			friend decimal operator + (const decimal& left, const decimal& right);
			friend decimal operator - (const decimal& left, const decimal& right);
			friend decimal operator * (const decimal& left, const decimal& right);
			friend decimal operator / (const decimal& left, const decimal& right);
			friend decimal operator % (const decimal& left, const decimal& right);

		public:
			static decimal from(const std::string_view& data, uint8_t base);
			static decimal zero();
			static decimal nan();

		private:
			static decimal sum(const decimal& left, const decimal& right);
			static decimal subtract(const decimal& left, const decimal& right);
			static decimal multiply(const decimal& left, const decimal& right);
			static int compare_num(const decimal& left, const decimal& right);
			static int char_to_int(char value);
			static char int_to_char(const int& value);
		};

		struct variant
		{
			friend schema;
			friend var;

		private:
			union tag
			{
				char string[32];
				char* pointer;
				int64_t integer;
				double number;
				bool boolean;
			} value;

		private:
			var_type type;
			uint32_t length;

		public:
			variant() noexcept;
			variant(const variant& other) noexcept;
			variant(variant&& other) noexcept;
			~variant() noexcept;
			bool deserialize(const std::string_view& value, bool strict = false);
			string serialize() const;
			string get_blob() const;
			decimal get_decimal() const;
			void* get_pointer() const;
			void* get_container();
			std::string_view get_string() const;
			uint8_t* get_binary() const;
			int64_t get_integer() const;
			double get_number() const;
			bool get_boolean() const;
			var_type get_type() const;
			size_t size() const;
			variant& operator= (const variant& other) noexcept;
			variant& operator= (variant&& other) noexcept;
			bool operator== (const variant& other) const;
			bool operator!= (const variant& other) const;
			explicit operator bool() const;
			bool is_string(const std::string_view& value) const;
			bool is_object() const;
			bool empty() const;
			bool is(var_type value) const;

		private:
			variant(var_type new_type) noexcept;
			bool same(const variant& value) const;
			void copy(const variant& other);
			void move(variant&& other);
			void free();

		private:
			static size_t get_max_small_string_size();
		};

		typedef vector<variant> variant_list;
		typedef unordered_map<string, variant> variant_args;

		struct text_settle
		{
			size_t start = 0;
			size_t end = 0;
			bool found = false;
		};

		struct file_state
		{
			size_t size = 0;
			size_t links = 0;
			size_t permissions = 0;
			size_t document = 0;
			size_t device = 0;
			size_t user_id = 0;
			size_t group_id = 0;
			int64_t last_access = 0;
			int64_t last_permission_change = 0;
			int64_t last_modified = 0;
			bool exists = false;
		};

		struct timeout
		{
			std::chrono::microseconds expires;
			task_callback callback;
			task_id id;
			bool alive;

			timeout(task_callback&& new_callback, const std::chrono::microseconds& new_timeout, task_id new_id, bool new_alive) noexcept;
			timeout(const timeout& other) noexcept;
			timeout(timeout&& other) noexcept;
			timeout& operator= (const timeout& other) noexcept;
			timeout& operator= (timeout&& other) noexcept;
		};

		struct file_entry
		{
			size_t size = 0;
			int64_t last_modified = 0;
			int64_t creation_time = 0;
			bool is_referenced = false;
			bool is_directory = false;
			bool is_exists = false;
		};

		struct date_time
		{
		private:
			std::chrono::system_clock::duration offset;
			struct tm timepoint;
			bool synchronized;
			bool globalized;

		public:
			date_time() noexcept;
			date_time(const struct tm& duration) noexcept;
			date_time(std::chrono::system_clock::duration&& duration) noexcept;
			date_time(const date_time&) = default;
			date_time(date_time&&) noexcept = default;
			date_time& operator= (const date_time&) = default;
			date_time& operator= (date_time&&) noexcept = default;
			date_time& operator +=(const date_time& right);
			date_time& operator -=(const date_time& right);
			bool operator >=(const date_time& right);
			bool operator <=(const date_time& right);
			bool operator >(const date_time& right);
			bool operator <(const date_time& right);
			bool operator ==(const date_time& right);
			date_time operator +(const date_time& right) const;
			date_time operator -(const date_time& right) const;
			date_time& apply_offset(bool always = false);
			date_time& apply_timepoint(bool always = false);
			date_time& use_global_time();
			date_time& use_local_time();
			date_time& set_second(uint8_t value);
			date_time& set_minute(uint8_t value);
			date_time& set_hour(uint8_t value);
			date_time& set_day(uint8_t value);
			date_time& set_week(uint8_t value);
			date_time& set_month(uint8_t value);
			date_time& set_year(uint32_t value);
			string serialize(const std::string_view& format) const;
			uint8_t second() const;
			uint8_t minute() const;
			uint8_t hour() const;
			uint8_t day() const;
			uint8_t week() const;
			uint8_t month() const;
			uint32_t year() const;
			int64_t nanoseconds() const;
			int64_t microseconds() const;
			int64_t milliseconds() const;
			int64_t seconds() const;
			const struct tm& current_timepoint() const;
			const std::chrono::system_clock::duration& current_offset() const;

		public:
			template <typename rep, typename period>
			date_time(const std::chrono::duration<rep, period>& duration) noexcept : date_time(std::chrono::duration_cast<std::chrono::system_clock::duration>(duration))
			{
			}

		public:
			static std::chrono::system_clock::duration now();
			static date_time from_nanoseconds(int64_t value);
			static date_time from_microseconds(int64_t value);
			static date_time from_milliseconds(int64_t value);
			static date_time from_seconds(int64_t value);
			static date_time from_serialized(const std::string_view& text, const std::string_view& format);
			static string serialize_global(const std::chrono::system_clock::duration& time, const std::string_view& format);
			static string serialize_local(const std::chrono::system_clock::duration& time, const std::string_view& format);
			static std::string_view serialize_global(char* buffer, size_t length, const std::chrono::system_clock::duration& duration, const std::string_view& format);
			static std::string_view serialize_local(char* buffer, size_t length, const std::chrono::system_clock::duration& duration, const std::string_view& format);
			static std::string_view format_iso8601_time();
			static std::string_view format_web_time();
			static std::string_view format_web_local_time();
			static std::string_view format_compact_time();
			static int64_t seconds_from_serialized(const std::string_view& text, const std::string_view& format);
			static bool make_global_time(time_t time, struct tm* timepoint);
			static bool make_local_time(time_t time, struct tm* timepoint);
		};

		struct stringify
		{
		public:
			static string& escape_print(string& other);
			static string& escape(string& other);
			static string& unescape(string& other);
			static string& to_upper(string& other);
			static string& to_lower(string& other);
			static string& clip(string& other, size_t length);
			static string& compress(string& other, const std::string_view& space_if_not_followed_or_preceded_by_of, const std::string_view& not_in_between_of, size_t start = 0U);
			static string& replace_of(string& other, const std::string_view& chars, const std::string_view& to, size_t start = 0U);
			static string& replace_not_of(string& other, const std::string_view& chars, const std::string_view& to, size_t start = 0U);
			static string& replace_groups(string& other, const std::string_view& from_regex, const std::string_view& to);
			static string& replace(string& other, const std::string_view& from, const std::string_view& to, size_t start = 0U);
			static string& replace(string& other, char from, char to, size_t position = 0U);
			static string& replace(string& other, char from, char to, size_t position, size_t count);
			static string& replace_part(string& other, size_t start, size_t end, const std::string_view& value);
			static string& replace_starts_with_ends_of(string& other, const std::string_view& begins, const std::string_view& ends_of, const std::string_view& with, size_t start = 0U);
			static string& replace_in_between(string& other, const std::string_view& begins, const std::string_view& ends, const std::string_view& with, bool recursive, size_t start = 0U);
			static string& replace_not_in_between(string& other, const std::string_view& begins, const std::string_view& ends, const std::string_view& with, bool recursive, size_t start = 0U);
			static string& replace_parts(string& other, vector<std::pair<string, text_settle>>& inout, const std::string_view& with, const std::function<char(const std::string_view&, char, int)>& surrounding = nullptr);
			static string& replace_parts(string& other, vector<text_settle>& inout, const std::string_view& with, const std::function<char(char, int)>& surrounding = nullptr);
			static string& remove_part(string& other, size_t start, size_t end);
			static string& reverse(string& other);
			static string& reverse(string& other, size_t start, size_t end);
			static string& substring(string& other, const text_settle& result);
			static string& splice(string& other, size_t start, size_t end);
			static string& trim(string& other);
			static string& trim_start(string& other);
			static string& trim_end(string& other);
			static string& fill(string& other, char symbol);
			static string& fill(string& other, char symbol, size_t count);
			static string& fill(string& other, char symbol, size_t start, size_t count);
			static string& append(string& other, const char* format, ...);
			static string& erase(string& other, size_t position);
			static string& erase(string& other, size_t position, size_t count);
			static string& erase_offsets(string& other, size_t start, size_t end);
			static expects_system<void> eval_envs(string& other, const std::string_view& directory, const vector<string>& tokens, const std::string_view& token = ":net");
			static vector<std::pair<string, text_settle>> find_in_between(const std::string_view& other, const std::string_view& begins, const std::string_view& ends, const std::string_view& not_in_sub_between_of, size_t offset = 0U);
			static vector<std::pair<string, text_settle>> find_in_between_in_code(const std::string_view& other, const std::string_view& begins, const std::string_view& ends, size_t offset = 0U);
			static vector<std::pair<string, text_settle>> find_starts_with_ends_of(const std::string_view& other, const std::string_view& begins, const std::string_view& ends_of, const std::string_view& not_in_sub_between_of, size_t offset = 0U);
			static void pm_find_in_between(vector<std::pair<string, text_settle>>& data, const std::string_view& other, const std::string_view& begins, const std::string_view& ends, const std::string_view& not_in_sub_between_of, size_t offset = 0U);
			static void pm_find_in_between_in_code(vector<std::pair<string, text_settle>>& data, const std::string_view& other, const std::string_view& begins, const std::string_view& ends, size_t offset = 0U);
			static void pm_find_starts_with_ends_of(vector<std::pair<string, text_settle>>& data, const std::string_view& other, const std::string_view& begins, const std::string_view& ends_of, const std::string_view& not_in_sub_between_of, size_t offset = 0U);
			static text_settle reverse_find(const std::string_view& other, const std::string_view& needle, size_t offset = 0U);
			static text_settle reverse_find(const std::string_view& other, char needle, size_t offset = 0U);
			static text_settle reverse_find_unescaped(const std::string_view& other, char needle, size_t offset = 0U);
			static text_settle reverse_find_of(const std::string_view& other, const std::string_view& needle, size_t offset = 0U);
			static text_settle find(const std::string_view& other, const std::string_view& needle, size_t offset = 0U);
			static text_settle find(const std::string_view& other, char needle, size_t offset = 0U);
			static text_settle find_unescaped(const std::string_view& other, char needle, size_t offset = 0U);
			static text_settle find_of(const std::string_view& other, const std::string_view& needle, size_t offset = 0U);
			static text_settle find_not_of(const std::string_view& other, const std::string_view& needle, size_t offset = 0U);
			static size_t count_lines(const std::string_view& other);
			static bool is_cstring(const std::string_view& other);
			static bool is_not_preceded_by_escape(const std::string_view& other, size_t offset, char escape = '\\');
			static bool is_empty_or_whitespace(const std::string_view& other);
			static bool is_preceded_by(const std::string_view& other, size_t at, const std::string_view& of);
			static bool is_followed_by(const std::string_view& other, size_t at, const std::string_view& of);
			static bool starts_with(const std::string_view& other, const std::string_view& value, size_t offset = 0U);
			static bool starts_of(const std::string_view& other, const std::string_view& value, size_t offset = 0U);
			static bool starts_not_of(const std::string_view& other, const std::string_view& value, size_t offset = 0U);
			static bool ends_with(const std::string_view& other, const std::string_view& value);
			static bool ends_with(const std::string_view& other, char value);
			static bool ends_of(const std::string_view& other, const std::string_view& value);
			static bool ends_not_of(const std::string_view& other, const std::string_view& value);
			static bool has_integer(const std::string_view& other);
			static bool has_number(const std::string_view& other);
			static bool has_decimal(const std::string_view& other);
			static string text(const char* format, ...);
			static wide_string to_wide(const std::string_view& other);
			static vector<string> split(const std::string_view& other, const std::string_view& with, size_t start = 0U);
			static vector<string> split(const std::string_view& other, char with, size_t start = 0U);
			static vector<string> split_max(const std::string_view& other, char with, size_t max_count, size_t start = 0U);
			static vector<string> split_of(const std::string_view& other, const std::string_view& with, size_t start = 0U);
			static vector<string> split_not_of(const std::string_view& other, const std::string_view& with, size_t start = 0U);
			static void pm_split(vector<string>& data, const std::string_view& other, const std::string_view& with, size_t start = 0U);
			static void pm_split(vector<string>& data, const std::string_view& other, char with, size_t start = 0U);
			static void pm_split_max(vector<string>& data, const std::string_view& other, char with, size_t max_count, size_t start = 0U);
			static void pm_split_of(vector<string>& data, const std::string_view& other, const std::string_view& with, size_t start = 0U);
			static void pm_split_not_of(vector<string>& data, const std::string_view& other, const std::string_view& with, size_t start = 0U);
			static bool is_numeric_or_dot(char symbol);
			static bool is_numeric_or_dot_or_whitespace(char symbol);
			static bool is_hex(char symbol);
			static bool is_hex_or_dot(char symbol);
			static bool is_hex_or_dot_or_whitespace(char symbol);
			static bool is_alphabetic(char symbol);
			static bool is_numeric(char symbol);
			static bool is_alphanum(char symbol);
			static bool is_whitespace(char symbol);
			static char to_lower_literal(char symbol);
			static char to_upper_literal(char symbol);
			static bool case_equals(const std::string_view& value1, const std::string_view& value2);
			static int case_compare(const std::string_view& value1, const std::string_view& value2);
			static int match(const char* pattern, const std::string_view& text);
			static int match(const char* pattern, size_t length, const std::string_view& text);
			static void convert_to_wide(const std::string_view& input, wchar_t* output, size_t output_size);
		};

		struct concurrent_timeout_queue
		{
			ordered_map<std::chrono::microseconds, timeout> queue;
			std::condition_variable notify;
			std::mutex update;
			bool resync = true;
		};

		struct inline_args
		{
		public:
			unordered_map<string, string> args;
			vector<string> params;
			string path;

		public:
			inline_args() noexcept;
			bool is_enabled(const std::string_view& option, const std::string_view& shortcut = "") const;
			bool is_disabled(const std::string_view& option, const std::string_view& shortcut = "") const;
			bool has(const std::string_view& option, const std::string_view& shortcut = "") const;
			string& get(const std::string_view& option, const std::string_view& shortcut = "");
			string& get_if(const std::string_view& option, const std::string_view& shortcut, const std::string_view& when_empty);

		private:
			bool is_true(const std::string_view& value) const;
			bool is_false(const std::string_view& value) const;
		};

		class var
		{
		public:
			class set
			{
			public:
				static unique<schema> any(variant&& value);
				static unique<schema> any(const variant& value);
				static unique<schema> any(const std::string_view& value, bool strict = false);
				static unique<schema> null();
				static unique<schema> undefined();
				static unique<schema> object();
				static unique<schema> array();
				static unique<schema> pointer(void* value);
				static unique<schema> string(const std::string_view& value);
				static unique<schema> binary(const std::string_view& value);
				static unique<schema> binary(const uint8_t* value, size_t size);
				static unique<schema> integer(int64_t value);
				static unique<schema> number(double value);
				static unique<schema> decimal(const core::decimal& value);
				static unique<schema> decimal(core::decimal&& value);
				static unique<schema> decimal_string(const std::string_view& value);
				static unique<schema> boolean(bool value);
			};

		public:
			static variant any(const std::string_view& value, bool strict = false);
			static variant null();
			static variant undefined();
			static variant object();
			static variant array();
			static variant pointer(void* value);
			static variant string(const std::string_view& value);
			static variant binary(const std::string_view& value);
			static variant binary(const uint8_t* value, size_t size);
			static variant integer(int64_t value);
			static variant number(double value);
			static variant decimal(const core::decimal& value);
			static variant decimal(core::decimal&& value);
			static variant decimal_string(const std::string_view& value);
			static variant boolean(bool value);
		};

		class os
		{
		public:
			class hw
			{
			public:
				enum class arch
				{
					x64,
					arm,
					itanium,
					x86,
					unknown,
				};

				enum class endian
				{
					little,
					big,
				};

				enum class cache
				{
					unified,
					instruction,
					data,
					trace,
				};

				struct quantity_info
				{
					uint32_t logical;
					uint32_t physical;
					uint32_t packages;
				};

				struct cache_info
				{
					size_t size;
					size_t line_size;
					uint8_t associativity;
					cache type;
				};

			public:
				static quantity_info get_quantity_info();
				static cache_info get_cache_info(uint32_t level);
				static arch get_arch() noexcept;
				static endian get_endianness() noexcept;
				static size_t get_frequency() noexcept;

			public:
				template <typename t>
				static typename std::enable_if<std::is_arithmetic<t>::value, t>::type swap_endianness(t value)
				{
					union u
					{
						t numeric;
						uint8_t buffer[sizeof(t)];
					} from, to;

					from.numeric = value;
					std::reverse_copy(std::begin(from.buffer), std::end(from.buffer), std::begin(to.buffer));
					return to.numeric;
				}
				template <typename t>
				static typename std::enable_if<std::is_arithmetic<t>::value, t>::type to_endianness(endian type, t value)
				{
					return get_endianness() == type ? value : swap_endianness(value);
				}
			};

			class directory
			{
			public:
				static bool is_exists(const std::string_view& path);
				static bool is_empty(const std::string_view& path);
				static expects_io<void> set_working(const std::string_view& path);
				static expects_io<void> patch(const std::string_view& path);
				static expects_io<void> scan(const std::string_view& path, vector<std::pair<string, file_entry>>& entries);
				static expects_io<void> create(const std::string_view& path);
				static expects_io<void> remove(const std::string_view& path);
				static expects_io<string> get_module();
				static expects_io<string> get_working();
				static vector<string> get_mounts();
			};

			class file
			{
			public:
				static bool is_exists(const std::string_view& path);
				static int compare(const std::string_view& first_path, const std::string_view& second_path);
				static uint64_t get_hash(const std::string_view& data);
				static uint64_t get_index(const std::string_view& data);
				static expects_io<size_t> write(const std::string_view& path, const uint8_t* data, size_t size);
				static expects_io<void> move(const std::string_view& from, const std::string_view& to);
				static expects_io<void> copy(const std::string_view& from, const std::string_view& to);
				static expects_io<void> remove(const std::string_view& path);
				static expects_io<void> close(unique<FILE> stream);
				static expects_io<void> get_state(const std::string_view& path, file_entry* output);
				static expects_io<void> seek64(FILE* stream, int64_t offset, file_seek mode);
				static expects_io<size_t> tell64(FILE* stream);
				static expects_io<size_t> join(const std::string_view& to, const vector<string>& paths);
				static expects_io<file_state> get_properties(const std::string_view& path);
				static expects_io<file_entry> get_state(const std::string_view& path);
				static expects_io<unique<stream>> open_join(const std::string_view& path, const vector<string>& paths);
				static expects_io<unique<stream>> open_archive(const std::string_view& path, size_t unarchived_max_size = 128 * 1024 * 1024);
				static expects_io<unique<stream>> open(const std::string_view& path, file_mode mode, bool async = false);
				static expects_io<unique<FILE>> open(const std::string_view& path, const std::string_view& mode);
				static expects_io<unique<uint8_t>> read_chunk(stream* stream, size_t length);
				static expects_io<unique<uint8_t>> read_all(const std::string_view& path, size_t* byte_length);
				static expects_io<unique<uint8_t>> read_all(stream* stream, size_t* byte_length);
				static expects_io<string> read_as_string(const std::string_view& path);
				static expects_io<vector<string>> read_as_array(const std::string_view& path);

			public:
				template <size_t size>
				static constexpr uint64_t get_index(const char source[size]) noexcept
				{
					uint64_t result = 0xcbf29ce484222325;
					for (size_t i = 0; i < size; i++)
					{
						result ^= source[i];
						result *= 1099511628211;
					}

					return result;
				}
			};

			class path
			{
			public:
				static bool is_remote(const std::string_view& path);
				static bool is_relative(const std::string_view& path);
				static bool is_absolute(const std::string_view& path);
				static std::string_view get_filename(const std::string_view& path);
				static std::string_view get_extension(const std::string_view& path);
				static string get_directory(const std::string_view& path, size_t level = 0);
				static string get_non_existant(const std::string_view& path);
				static expects_io<string> resolve(const std::string_view& path);
				static expects_io<string> resolve(const std::string_view& path, const std::string_view& directory, bool even_if_exists);
				static expects_io<string> resolve_directory(const std::string_view& path);
				static expects_io<string> resolve_directory(const std::string_view& path, const std::string_view& directory, bool even_if_exists);
			};

			class net
			{
			public:
				static bool get_etag(char* buffer, size_t length, file_entry* resource);
				static bool get_etag(char* buffer, size_t length, int64_t last_modified, size_t content_length);
				static socket_t get_fd(FILE* stream);
			};

			class process
			{
			public:
				static void abort();
				static void interrupt();
				static void exit(int code);
				static bool raise_signal(signal_code type);
				static bool bind_signal(signal_code type, signal_callback callback);
				static bool rebind_signal(signal_code type);
				static bool has_debugger();
				static int get_signal_id(signal_code type);
				static expects_io<int> execute(const std::string_view& command, file_mode mode, process_callback&& callback);
				static expects_io<unique<process_stream>> spawn(const std::string_view& command, file_mode mode);
				static expects_io<string> get_env(const std::string_view& name);
				static expects_io<string> get_shell();
				static string get_thread_id(const std::thread::id& id);
				static inline_args parse_args(int argc, char** argv, size_t format_opts, const unordered_set<string>& flags = { });
			};

			class symbol
			{
			public:
				static expects_io<unique<void>> load(const std::string_view& path = "");
				static expects_io<unique<void>> load_function(void* handle, const std::string_view& name);
				static expects_io<void> unload(void* handle);
			};

			class error
			{
			public:
				static int get(bool clear = true);
				static void clear();
				static bool occurred();
				static bool is_error(int code);
				static std::error_condition get_condition();
				static std::error_condition get_condition(int code);
				static std::error_condition get_condition_or(std::errc code = std::errc::invalid_argument);
				static string get_name(int code);
			};

			class control
			{
			private:
				static std::atomic<uint64_t> options;

			public:
				static void set(access_option option, bool enabled);
				static bool has(access_option option);
				static option<access_option> to_option(const std::string_view& option);
				static std::string_view to_string(access_option option);
				static std::string_view to_options();
			};
		};

		class composer : public singletonish
		{
		private:
			struct state
			{
				unordered_map<uint64_t, std::pair<uint64_t, void*>> factory;
				std::mutex mutex;
			};

		private:
			static state* context;

		public:
			static unordered_set<uint64_t> fetch(uint64_t id) noexcept;
			static bool pop(const std::string_view& hash) noexcept;
			static void cleanup() noexcept;

		private:
			static void push(uint64_t type_id, uint64_t tag, void* callback) noexcept;
			static void* find(uint64_t type_id) noexcept;

		public:
			template <typename t, typename... args>
			static unique<t> create(const std::string_view& hash, args... data) noexcept
			{
				return create<t, args...>(VI_HASH(hash), data...);
			}
			template <typename t, typename... args>
			static unique<t> create(uint64_t id, args... data) noexcept
			{
				void* (*callable)(args...) = nullptr;
				reinterpret_cast<void*&>(callable) = find(id);

				if (!callable)
					return nullptr;

				return (t*)callable(data...);
			}
			template <typename t, typename... args>
			static void push(uint64_t tag) noexcept
			{
				auto callable = &composer::callee<t, args...>;
				void* result = reinterpret_cast<void*&>(callable);
				push(t::get_type_id(), tag, result);
			}

		private:
			template <typename t, typename... args>
			static unique<void> callee(args... data) noexcept
			{
				return (void*)new t(data...);
			}
		};

		template <typename t>
		class reference
		{
		private:
			std::atomic<uint32_t> __vcnt;
			std::atomic<uint32_t> __vmrk;

		public:
			reference() noexcept : __vcnt(1), __vmrk(0)
			{
			}
			reference(const reference&) noexcept : reference()
			{
			}
			reference(reference&&) noexcept : reference()
			{
			}
			reference& operator=(const reference&) noexcept
			{
				return *this;
			}
			reference& operator=(reference&&) noexcept
			{
				return *this;
			}
			~reference() = default;
			bool is_marked_ref() const noexcept
			{
				return __vmrk.load() > 0;
			}
			uint32_t get_ref_count() const noexcept
			{
				return __vcnt.load();
			}
			void mark_ref() noexcept
			{
				__vmrk = 1;
			}
			void add_ref() noexcept
			{
				VI_ASSERT(__vcnt < std::numeric_limits<uint32_t>::max(), "too many references to an object at address 0x%" PRIXPTR " as %s at %s()", (void*)this, typeid(t).name(), __func__);
				++__vcnt;
			}
			void release() noexcept
			{
				VI_ASSERT(__vcnt > 0, "address at 0x%" PRIXPTR " has already been released as %s at %s()", (void*)this, typeid(t).name(), __func__);
				if (!--__vcnt)
				{
					VI_ASSERT(memory::is_valid_address((void*)(t*)this), "address at 0x%" PRIXPTR " is not a valid heap address as %s at %s()", (void*)this, typeid(t).name(), __func__);
					delete (t*)this;
				}
				else
					__vmrk = 0;
			}

		public:
#ifndef NDEBUG
#ifdef VI_CXX20
			void* operator new(size_t size, const std::source_location& location = std::source_location::current()) noexcept
			{
				return static_cast<void*>(memory::allocate<t>(size, location));
			}
			void operator delete(void* address, const std::source_location& location) noexcept
			{
				VI_ASSERT(false, "illegal usage of no-op delete operator usable only by compiler");
			}
#else
			void* operator new(size_t size) noexcept
			{
				return memory::tracing_allocate(size, memory_location(__FILE__, __func__, typeid(t).name(), __LINE__));
			}
#endif
			void operator delete(void* address) noexcept
			{
				auto* handle = static_cast<t*>(address);
				VI_ASSERT(!handle || handle->__vcnt <= 1, "address at 0x%" PRIXPTR " is still in use but destructor has been called by delete as %s at %s()", address, typeid(t).name(), __func__);
				memory::deallocate<t>(handle);
			}
#else
			void* operator new(size_t size) noexcept
			{
				return memory::default_allocate(size);
			}
			void operator delete(void* address) noexcept
			{
				memory::deallocate<t>(static_cast<t*>(address));
			}
#endif
		};

		template <typename t>
		class singleton : public reference<t>
		{
		private:
			enum class action
			{
				destroy,
				restore,
				create,
				store,
				fetch
			};

		public:
			singleton() noexcept
			{
				update_instance((t*)this, action::store);
			}
			virtual ~singleton() noexcept
			{
				if (update_instance(nullptr, action::fetch) == (t*)this)
					update_instance(nullptr, action::restore);
			}

		public:
			static bool unlink_instance(t* unlinkable) noexcept
			{
				return (update_instance(nullptr, action::fetch) == unlinkable) && !update_instance(nullptr, action::restore);
			}
			static bool link_instance()
			{
				return update_instance(nullptr, action::create) != nullptr;
			}
			static bool cleanup_instance() noexcept
			{
				return !update_instance(nullptr, action::destroy);
			}
			static bool has_instance() noexcept
			{
				return update_instance(nullptr, action::fetch) != nullptr;
			}
			static t* get() noexcept
			{
				return update_instance(nullptr, action::create);
			}

		private:
			template <typename q>
			static typename std::enable_if<std::is_default_constructible<q>::value, void>::type create_instance(q*& instance) noexcept
			{
				if (!instance)
					instance = new q();
			}
			template <typename q>
			static typename std::enable_if<!std::is_default_constructible<q>::value, void>::type create_instance(q*& instance) noexcept
			{
			}
			static t* update_instance(t* other, action type) noexcept
			{
				static t* instance = nullptr;
				switch (type)
				{
					case action::destroy:
					{
						memory::release(instance);
						instance = other;
						return nullptr;
					}
					case action::restore:
						instance = nullptr;
						return nullptr;
					case action::create:
						create_instance<t>(instance);
						return instance;
					case action::fetch:
						return instance;
					case action::store:
					{
						if (instance != other)
							memory::release(instance);
						instance = other;
						return instance;
					}
					default:
						return nullptr;
				}
			}
		};

		template <typename t>
		class uptr
		{
		private:
			t* address;

		public:
			uptr() noexcept : address(nullptr)
			{
			}
			uptr(t* new_address) noexcept : address(new_address)
			{
			}
			template <typename e>
			explicit uptr(const expects<t*, e>& may_be_address) noexcept : address(may_be_address ? *may_be_address : nullptr)
			{
			}
			explicit uptr(const option<t*>& may_be_address) noexcept : address(may_be_address ? *may_be_address : nullptr)
			{
			}
			uptr(const uptr&) noexcept = delete;
			uptr(uptr&& other) noexcept : address(other.address)
			{
				other.address = nullptr;
			}
			~uptr()
			{
				destroy();
			}
			uptr& operator= (const uptr&) noexcept = delete;
			uptr& operator= (uptr&& other) noexcept
			{
				if (this == &other)
					return *this;

				destroy();
				address = other.address;
				other.address = nullptr;
				return *this;
			}
			explicit operator bool() const
			{
				return address != nullptr;
			}
			inline t* operator-> ()
			{
				VI_ASSERT(address != nullptr, "unique null pointer access");
				return address;
			}
			inline t* operator-> () const
			{
				VI_ASSERT(address != nullptr, "unique null pointer access");
				return address;
			}
			inline t* operator* ()
			{
				return address;
			}
			inline t* operator* () const
			{
				return address;
			}
			inline t** out()
			{
				VI_ASSERT(!address, "pointer should be null when performing output update");
				return &address;
			}
			inline t* const* in() const
			{
				return &address;
			}
			inline t* expect(const std::string_view& message)
			{
				VI_PANIC(address != nullptr, "%.*s CAUSING panic", (int)message.size(), message.data());
				return address;
			}
			inline t* expect(const std::string_view& message) const
			{
				VI_PANIC(address != nullptr, "%.*s CAUSING panic", (int)message.size(), message.data());
				return address;
			}
			inline unique<t> reset()
			{
				t* result = address;
				address = nullptr;
				return result;
			}
			inline void destroy()
			{
				if constexpr (!is_releaser<t>::value)
				{
					if constexpr (std::is_trivially_default_constructible<t>::value)
						memory::deallocate<t>(address);
					else
						memory::deinit<t>(address);
				}
				else
					memory::release<t>(address);
			}
		};

		template <typename t>
		class uref
		{
			static_assert(is_add_refer<t>::value, "unique reference type should have add_ref() method");
			static_assert(is_releaser<t>::value, "unique reference type should have release() method");

		private:
			t* address;

		public:
			uref() noexcept : address(nullptr)
			{
			}
			uref(t* new_address) noexcept : address(new_address)
			{
			}
			template <typename e>
			explicit uref(const expects<t*, e>& may_be_address) noexcept : address(may_be_address ? *may_be_address : nullptr)
			{
			}
			explicit uref(const option<t*>& may_be_address) noexcept : address(may_be_address ? *may_be_address : nullptr)
			{
			}
			uref(const uref& other) noexcept : address(other.address)
			{
				if (address != nullptr)
					address->add_ref();
			}
			uref(uref&& other) noexcept : address(other.address)
			{
				other.address = nullptr;
			}
			~uref()
			{
				destroy();
			}
			uref& operator= (const uref& other) noexcept
			{
				if (this == &other)
					return *this;

				destroy();
				address = other.address;
				if (address != nullptr)
					address->add_ref();
				return *this;
			}
			uref& operator= (uref&& other) noexcept
			{
				if (this == &other)
					return *this;

				destroy();
				address = other.address;
				other.address = nullptr;
				return *this;
			}
			explicit operator bool() const
			{
				return address != nullptr;
			}
			inline t* operator-> ()
			{
				VI_ASSERT(address != nullptr, "unique null pointer access");
				return address;
			}
			inline t* operator-> () const
			{
				VI_ASSERT(address != nullptr, "unique null pointer access");
				return address;
			}
			inline t* operator* ()
			{
				return address;
			}
			inline t* operator* () const
			{
				return address;
			}
			inline t** out()
			{
				VI_ASSERT(!address, "pointer should be null when performing output update");
				return &address;
			}
			inline t* const* in() const
			{
				return &address;
			}
			inline t* expect(const std::string_view& message)
			{
				VI_PANIC(address != nullptr, "%.*s CAUSING panic", (int)message.size(), message.data());
				return address;
			}
			inline t* expect(const std::string_view& message) const
			{
				VI_PANIC(address != nullptr, "%.*s CAUSING panic", (int)message.size(), message.data());
				return address;
			}
			inline unique<t> reset()
			{
				t* result = address;
				address = nullptr;
				return result;
			}
			inline void destroy()
			{
				memory::release<t>(address);
			}
		};

		template <typename t>
		class umutex
		{
		private:
#ifndef NDEBUG
#ifdef VI_CXX20
			std::source_location location;
#endif
			int64_t diff;
#endif
			t& mutex;
			bool owns;

		public:
			umutex(const umutex&) noexcept = delete;
			umutex(umutex&&) noexcept = delete;
			~umutex()
			{
#ifndef NDEBUG
				unlock();
#else
				if (owns)
					mutex.unlock();
#endif
			}
			umutex& operator= (const umutex&) noexcept = delete;
			umutex& operator= (umutex&&) noexcept = delete;
#ifndef NDEBUG
#ifdef VI_CXX20
			umutex(t& new_mutex, std::source_location&& new_location = std::source_location::current()) noexcept : location(std::move(new_location)), diff(0), mutex(new_mutex), owns(false)
			{
				lock();
			}
#else
			umutex(t& new_mutex) noexcept : diff(0), mutex(new_mutex), owns(false)
			{
				lock();
			}
#endif
			inline void negate()
			{
				if (owns)
					unlock();
				else
					lock();
			}
			inline void lock()
			{
				if (owns)
					return;

				auto time = std::chrono::system_clock::now().time_since_epoch();
				owns = true;
				mutex.lock();
				diff = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch() - time).count();
			}
			inline void unlock()
			{
				if (!owns)
					return;

				mutex.unlock();
				owns = false;
				if (diff <= (int64_t)timings::pass * 1000000)
					return;
#ifdef VI_CXX20
				VI_WARN("[stall] %s:%i mutex lock() took %" PRIu64 " ms (%" PRIu64 " ns, expected < %" PRIu64 " ms)", os::path::get_filename(location.file_name()).data(), (int)location.line(), diff / 1000000, diff, (uint64_t)timings::pass);
#else
				VI_WARN("[stall] mutex lock() took %" PRIu64 " ms (%" PRIu64 " ns, expected < %" PRIu64 " ms)", diff / 1000000, diff, (uint64_t)timings::pass);
#endif
			}
#else
			umutex(t& new_mutex) noexcept : mutex(new_mutex), owns(true)
			{
				mutex.lock();
			}
			inline void negate()
			{
				owns = !owns;
				if (owns)
					mutex.lock();
				else
					mutex.unlock();
			}
			inline void lock()
			{
				if (!owns)
				{
					owns = true;
					mutex.lock();
				}
			}
			inline void unlock()
			{
				if (owns)
				{
					mutex.unlock();
					owns = false;
				}
			}
#endif
		};

		template <typename t>
		class ualloc
		{
			static_assert(std::is_base_of<local_allocator, t>::value, "unique allocator type should be based on local allocator");

		private:
			t& allocator;

		public:
			ualloc(t& new_allocator) noexcept : allocator(new_allocator)
			{
				memory::set_local_allocator(&new_allocator);
			}
			ualloc(const ualloc&) noexcept = delete;
			ualloc(ualloc&&) noexcept = delete;
			~ualloc()
			{
				memory::set_local_allocator(nullptr);
				allocator.reset();
			}
			ualloc& operator= (const ualloc&) noexcept = delete;
			ualloc& operator= (ualloc&&) noexcept = delete;
		};

		class console final : public singleton<console>
		{
		public:
			struct color_token
			{
				std::string_view text;
				std_color background;
				std_color foreground;

				color_token(const std::string_view& name, std_color foreground_color, std_color background_color = std_color::zero) : text(name), foreground(foreground_color), background(background_color)
				{
				}
			};

		private:
			enum class mode
			{
				attached,
				allocated,
				detached
			};

			struct element_state
			{
				uint64_t state;
				uint32_t x, y;
			};

			struct window_state
			{
				vector<std::pair<uint64_t, string>> elements;
				size_t position = 0;
			};

		private:
			struct
			{
				FILE* input = nullptr;
				FILE* output = nullptr;
				FILE* errors = nullptr;
			} streams;

			struct
			{
				unordered_map<uint64_t, element_state> elements;
				unordered_map<uint64_t, window_state> windows;
				std::recursive_mutex session;
				unsigned short attributes = 0;
				mode status = mode::detached;
				bool colors = true;
				double time = 0.0;
				uint64_t id = 0;
			} state;

		private:
			vector<color_token> color_tokens;

		public:
			console() noexcept;
			virtual ~console() noexcept override;
			void hide();
			void show();
			void clear();
			void attach();
			void detach();
			void allocate();
			void deallocate();
			void trace(uint32_t max_frames = 32);
			void set_coloring(bool enabled);
			void add_color_tokens(const vector<color_token>& additional_tokens);
			void clear_color_tokens();
			void color_begin(std_color text, std_color background = std_color::zero);
			void color_end();
			void color_print(std_color base_color, const std::string_view& buffer);
			void capture_time();
			uint64_t capture_window(uint32_t height);
			void free_window(uint64_t id, bool restore_position);
			void emplace_window(uint64_t id, const std::string_view& text);
			uint64_t capture_element();
			void resize_element(uint64_t id, uint32_t x);
			void move_element(uint64_t id, uint32_t y);
			void read_element(uint64_t id, uint32_t* x, uint32_t* y);
			void free_element(uint64_t id);
			void replace_element(uint64_t id, const std::string_view& text);
			void spinning_element(uint64_t id, const std::string_view& label);
			void progress_element(uint64_t id, double value, double coverage = 0.8);
			void spinning_progress_element(uint64_t id, double value, double coverage = 0.8);
			void clear_element(uint64_t id);
			void flush();
			void flush_write();
			void write_size(uint32_t width, uint32_t height);
			void write_position(uint32_t x, uint32_t y);
			void write_line(const std::string_view& line);
			void write_char(char value);
			void write(const std::string_view& text);
			void jwrite(schema* data);
			void jwrite_line(schema* data);
			void fwrite_line(const char* format, ...);
			void fwrite(const char* format, ...);
			double get_captured_time() const;
			bool read_screen(uint32_t* width, uint32_t* height, uint32_t* x, uint32_t* y);
			bool read_line(string& data, size_t size);
			string read(size_t size);
			char read_char();

		public:
			template <typename f>
			void synced(f&& callback)
			{
				umutex<std::recursive_mutex> unique(state.session);
				callback(this);
			}

		public:
			static bool is_available();
		};

		class timer final : public reference<timer>
		{
		public:
			typedef std::chrono::microseconds units;

		public:
			struct capture
			{
				const char* name;
				units begin = units(0);
				units end = units(0);
				units delta = units(0);
				float step = 0.0;
			};

		private:
			struct
			{
				units begin = units(0);
				units when = units(0);
				units delta = units(0);
				size_t frame = 0;
			} timing;

			struct
			{
				units when = units(0);
				units delta = units(0);
				units sum = units(0);
				size_t frame = 0;
				bool in_frame = false;
			} fixed;

		private:
			single_queue<capture> captures;
			units min_delta = units(0);
			units max_delta = units(0);
			float fixed_frames = 0.0f;
			float max_frames = 0.0f;

		public:
			timer() noexcept;
			~timer() noexcept = default;
			void set_fixed_frames(float value);
			void set_max_frames(float value);
			void reset();
			void begin();
			void finish();
			void push(const char* name = nullptr);
			bool pop_if(float greater_than, capture* out = nullptr);
			capture pop();
			size_t get_frame_index() const;
			size_t get_fixed_frame_index() const;
			float get_max_frames() const;
			float get_min_step() const;
			float get_frames() const;
			float get_elapsed() const;
			float get_elapsed_mills() const;
			float get_step() const;
			float get_fixed_step() const;
			float get_fixed_frames() const;
			bool is_fixed() const;

		public:
			static float to_seconds(const units& value);
			static float to_mills(const units& value);
			static units to_units(float value);
			static units clock();
		};

		class stream : public reference<stream>
		{
		private:
			string vname;
			size_t vsize;

		public:
			stream() noexcept;
			virtual ~stream() noexcept = default;
			virtual expects_io<void> clear() = 0;
			virtual expects_io<void> open(const std::string_view& file, file_mode mode) = 0;
			virtual expects_io<void> close() = 0;
			virtual expects_io<void> seek(file_seek mode, int64_t offset) = 0;
			virtual expects_io<void> move(int64_t offset);
			virtual expects_io<void> flush() = 0;
			virtual expects_io<size_t> read_scan(const char* format, ...) = 0;
			virtual expects_io<size_t> read_line(char* buffer, size_t length) = 0;
			virtual expects_io<size_t> read(uint8_t* buffer, size_t length) = 0;
			virtual expects_io<size_t> write_format(const char* format, ...) = 0;
			virtual expects_io<size_t> write(const uint8_t* buffer, size_t length) = 0;
			virtual expects_io<size_t> tell() = 0;
			virtual socket_t get_readable_fd() const = 0;
			virtual socket_t get_writeable_fd() const = 0;
			virtual void* get_readable() const = 0;
			virtual void* get_writeable() const = 0;
			virtual bool is_sized() const = 0;
			void set_virtual_size(size_t size);
			void set_virtual_name(const std::string_view& file);
			expects_io<size_t> read_all(const std::function<void(uint8_t*, size_t)>& callback);
			expects_io<size_t> size();
			size_t virtual_size() const;
			std::string_view virtual_name() const;

		protected:
			void open_virtual(string&& path);
			void close_virtual();
		};

		class memory_stream final : public stream
		{
		protected:
			vector<char> buffer;
			size_t offset;
			bool readable;
			bool writeable;

		public:
			memory_stream() noexcept;
			~memory_stream() noexcept override;
			expects_io<void> clear() override;
			expects_io<void> open(const std::string_view& file, file_mode mode) override;
			expects_io<void> close() override;
			expects_io<void> seek(file_seek mode, int64_t offset) override;
			expects_io<void> flush() override;
			expects_io<size_t> read_scan(const char* format, ...) override;
			expects_io<size_t> read_line(char* buffer, size_t length) override;
			expects_io<size_t> read(uint8_t* buffer, size_t length) override;
			expects_io<size_t> write_format(const char* format, ...) override;
			expects_io<size_t> write(const uint8_t* buffer, size_t length) override;
			expects_io<size_t> tell() override;
			socket_t get_readable_fd() const override;
			socket_t get_writeable_fd() const override;
			void* get_readable() const override;
			void* get_writeable() const override;
			bool is_sized() const override;

		private:
			char* prepare_buffer(size_t size);
		};

		class file_stream final : public stream
		{
		protected:
			FILE* io_stream;

		public:
			file_stream() noexcept;
			~file_stream() noexcept override;
			expects_io<void> clear() override;
			expects_io<void> open(const std::string_view& file, file_mode mode) override;
			expects_io<void> close() override;
			expects_io<void> seek(file_seek mode, int64_t offset) override;
			expects_io<void> flush() override;
			expects_io<size_t> read_scan(const char* format, ...) override;
			expects_io<size_t> read_line(char* buffer, size_t length) override;
			expects_io<size_t> read(uint8_t* buffer, size_t length) override;
			expects_io<size_t> write_format(const char* format, ...) override;
			expects_io<size_t> write(const uint8_t* buffer, size_t length) override;
			expects_io<size_t> tell() override;
			socket_t get_readable_fd() const override;
			socket_t get_writeable_fd() const override;
			void* get_readable() const override;
			void* get_writeable() const override;
			bool is_sized() const override;
		};

		class gz_stream final : public stream
		{
		protected:
			void* io_stream;

		public:
			gz_stream() noexcept;
			~gz_stream() noexcept override;
			expects_io<void> clear() override;
			expects_io<void> open(const std::string_view& file, file_mode mode) override;
			expects_io<void> close() override;
			expects_io<void> seek(file_seek mode, int64_t offset) override;
			expects_io<void> flush() override;
			expects_io<size_t> read_scan(const char* format, ...) override;
			expects_io<size_t> read_line(char* buffer, size_t length) override;
			expects_io<size_t> read(uint8_t* buffer, size_t length) override;
			expects_io<size_t> write_format(const char* format, ...) override;
			expects_io<size_t> write(const uint8_t* buffer, size_t length) override;
			expects_io<size_t> tell() override;
			socket_t get_readable_fd() const override;
			socket_t get_writeable_fd() const override;
			void* get_readable() const override;
			void* get_writeable() const override;
			bool is_sized() const override;
		};

		class web_stream final : public stream
		{
		protected:
			unordered_map<string, string> headers;
			vector<char> chunk;
			void* output_stream;
			size_t offset;
			size_t length;
			bool async;

		public:
			web_stream(bool is_async) noexcept;
			web_stream(bool is_async, unordered_map<string, string>&& new_headers) noexcept;
			~web_stream() noexcept override;
			expects_io<void> clear() override;
			expects_io<void> open(const std::string_view& file, file_mode mode) override;
			expects_io<void> close() override;
			expects_io<void> seek(file_seek mode, int64_t offset) override;
			expects_io<void> flush() override;
			expects_io<size_t> read_scan(const char* format, ...) override;
			expects_io<size_t> read_line(char* buffer, size_t length) override;
			expects_io<size_t> read(uint8_t* buffer, size_t length) override;
			expects_io<size_t> write_format(const char* format, ...) override;
			expects_io<size_t> write(const uint8_t* buffer, size_t length) override;
			expects_io<size_t> tell() override;
			socket_t get_readable_fd() const override;
			socket_t get_writeable_fd() const override;
			void* get_readable() const override;
			void* get_writeable() const override;
			bool is_sized() const override;
		};

		class process_stream final : public stream
		{
		private:
			struct
			{
				void* output_pipe = nullptr;
				void* input_pipe = nullptr;
				void* process = nullptr;
				void* thread = nullptr;
				socket_t process_id = 0;
				socket_t thread_id = 0;
				int error_exit_code = 0;
			} internal;

		private:
			FILE* output_stream;
			int input_fd;
			int exit_code;

		public:
			process_stream() noexcept;
			~process_stream() noexcept override;
			expects_io<void> clear() override;
			expects_io<void> open(const std::string_view& file, file_mode mode) override;
			expects_io<void> close() override;
			expects_io<void> seek(file_seek mode, int64_t offset) override;
			expects_io<void> flush() override;
			expects_io<size_t> read_line(char* buffer, size_t length) override;
			expects_io<size_t> read_scan(const char* format, ...) override;
			expects_io<size_t> read(uint8_t* buffer, size_t length) override;
			expects_io<size_t> write_format(const char* format, ...) override;
			expects_io<size_t> write(const uint8_t* buffer, size_t length) override;
			expects_io<size_t> tell() override;
			socket_t get_process_id() const;
			socket_t get_thread_id() const;
			socket_t get_readable_fd() const override;
			socket_t get_writeable_fd() const override;
			void* get_readable() const override;
			void* get_writeable() const override;
			bool is_sized() const override;
			bool is_alive();
			int get_exit_code() const;
		};

		class file_tree final : public reference<file_tree>
		{
		public:
			vector<file_tree*> directories;
			vector<string> files;
			string path;

		public:
			file_tree(const std::string_view& path) noexcept;
			~file_tree() noexcept;
			void loop(const std::function<bool(const file_tree*)>& callback) const;
			const file_tree* find(const std::string_view& path) const;
			size_t get_files() const;
		};

		class costate final : public reference<costate>
		{
			friend cocontext;

		private:
			unordered_set<coroutine*> cached;
			unordered_set<coroutine*> used;
			std::thread::id thread;
			coroutine* current;
			cocontext* master;
			size_t size;

		public:
			std::condition_variable* external_condition;
			std::mutex* external_mutex;

		public:
			costate(size_t stack_size = STACK_SIZE) noexcept;
			~costate() noexcept;
			costate(const costate&) = delete;
			costate(costate&&) = delete;
			costate& operator= (const costate&) = delete;
			costate& operator= (costate&&) = delete;
			coroutine* pop(task_callback&& procedure);
			coexecution resume(coroutine* routine);
			void reuse(coroutine* routine);
			void push(coroutine* routine);
			void activate(coroutine* routine);
			void deactivate(coroutine* routine);
			void deactivate(coroutine* routine, task_callback&& after_suspend);
			void clear();
			bool dispatch();
			bool suspend();
			bool has_resumable_coroutines() const;
			bool has_alive_coroutines() const;
			bool has_coroutines() const;
			coroutine* get_current() const;
			size_t get_count() const;

		private:
			coexecution execute(coroutine* routine);

		public:
			static costate* get();
			static coroutine* get_coroutine();
			static bool get_state(costate** state, coroutine** routine);
			static bool is_coroutine();

		public:
			static void VI_COCALL execution_entry(VI_CODATA);
		};

		class schema final : public reference<schema>
		{
		protected:
			vector<schema*>* nodes;
			schema* parent;
			bool saved;

		public:
			string key;
			variant value;

		public:
			schema(const variant& base) noexcept;
			schema(variant&& base) noexcept;
			~schema() noexcept;
			unordered_map<string, size_t> get_names() const;
			vector<schema*> find_collection(const std::string_view& name, bool deep = false) const;
			vector<schema*> fetch_collection(const std::string_view& notation, bool deep = false) const;
			vector<schema*> get_attributes() const;
			vector<schema*>& get_childs();
			schema* find(const std::string_view& name, bool deep = false) const;
			schema* fetch(const std::string_view& notation, bool deep = false) const;
			variant fetch_var(const std::string_view& key, bool deep = false) const;
			variant get_var(size_t index) const;
			variant get_var(const std::string_view& key) const;
			variant get_attribute_var(const std::string_view& key) const;
			schema* get_parent() const;
			schema* get_attribute(const std::string_view& key) const;
			schema* get(size_t index) const;
			schema* get(const std::string_view& key) const;
			schema* set(const std::string_view& key);
			schema* set(const std::string_view& key, const variant& value);
			schema* set(const std::string_view& key, variant&& value);
			schema* set(const std::string_view& key, unique<schema> value);
			schema* set_attribute(const std::string_view& key, const variant& value);
			schema* set_attribute(const std::string_view& key, variant&& value);
			schema* push(const variant& value);
			schema* push(variant&& value);
			schema* push(unique<schema> value);
			schema* pop(size_t index);
			schema* pop(const std::string_view& name);
			unique<schema> copy() const;
			bool rename(const std::string_view& name, const std::string_view& new_name);
			bool has(const std::string_view& name) const;
			bool has_attribute(const std::string_view& name) const;
			bool empty() const;
			bool is_attribute() const;
			bool is_saved() const;
			size_t size() const;
			string get_name() const;
			void join(schema* other, bool append_only);
			void reserve(size_t size);
			void unlink();
			void clear();
			void save();

		protected:
			void allocate();
			void allocate(const vector<schema*>& other);

		private:
			void attach(schema* root);

		public:
			static void transform(schema* value, const schema_name_callback& callback);
			static void convert_to_xml(schema* value, const schema_write_callback& callback);
			static void convert_to_json(schema* value, const schema_write_callback& callback);
			static void convert_to_jsonb(schema* value, const schema_write_callback& callback);
			static string to_xml(schema* value);
			static string to_json(schema* value);
			static vector<char> to_jsonb(schema* value);
			static expects_parser<unique<schema>> convert_from_xml(const std::string_view& buffer);
			static expects_parser<unique<schema>> convert_from_json(const std::string_view& buffer);
			static expects_parser<unique<schema>> convert_from_jsonb(const schema_read_callback& callback);
			static expects_parser<unique<schema>> from_xml(const std::string_view& text);
			static expects_parser<unique<schema>> from_json(const std::string_view& text);
			static expects_parser<unique<schema>> from_jsonb(const std::string_view& binary);

		private:
			static expects_parser<void> process_convertion_from_jsonb(schema* current, unordered_map<size_t, string>* map, const schema_read_callback& callback);
			static schema* process_conversion_from_json_string_or_number(void* base, bool is_document);
			static void process_convertion_from_xml(void* base, schema* current);
			static void process_convertion_from_json(void* base, schema* current);
			static void process_convertion_to_jsonb(schema* current, unordered_map<string, size_t>* map, const schema_write_callback& callback);
			static void generate_naming_table(const schema* current, unordered_map<string, size_t>* map, size_t& index);
		};

		class schedule final : public singleton<schedule>
		{
		public:
			enum class thread_task
			{
				spawn,
				enqueue_timer,
				enqueue_coroutine,
				enqueue_task,
				consume_coroutine,
				process_timer,
				process_coroutine,
				process_task,
				awake,
				sleep,
				despawn
			};

			struct thread_data
			{
				single_queue<task_callback> queue;
				std::condition_variable notify;
				std::mutex update;
				std::thread handle;
				std::thread::id id;
				allocators::linear_allocator allocator;
				difficulty type;
				size_t global_index;
				size_t local_index;
				bool daemon;

				thread_data(difficulty new_type, size_t preallocated_size, size_t new_global_index, size_t new_local_index, bool is_daemon) : allocator(preallocated_size), type(new_type), global_index(new_global_index), local_index(new_local_index), daemon(is_daemon)
				{
				}
				~thread_data() = default;
			};

			struct thread_message
			{
				const thread_data* thread;
				thread_task state;
				size_t tasks;

				thread_message() : thread(nullptr), state(thread_task::sleep), tasks(0)
				{
				}
				thread_message(const thread_data* new_thread, thread_task new_state, size_t new_tasks) : thread(new_thread), state(new_state), tasks(new_tasks)
				{
				}
			};

			struct desc
			{
				size_t threads[(size_t)difficulty::count];
				size_t preallocated_size;
				size_t stack_size;
				size_t max_coroutines;
				size_t max_recycles;
				std::chrono::milliseconds idle_timeout;
				std::chrono::milliseconds clock_timeout;
				spawner_callback initialize;
				activity_callback ping;
				bool parallel;

				desc();
				desc(size_t cores);
			};

		public:
			typedef std::function<void(const thread_message&)> thread_debug_callback;

		private:
			struct
			{
				task_callback event;
				costate* state = nullptr;
			} dispatcher;

		private:
			vector<thread_data*> threads[(size_t)difficulty::count];
			concurrent_timeout_queue* timeouts = nullptr;
			concurrent_async_queue* async = nullptr;
			concurrent_sync_queue* sync = nullptr;
			std::atomic<task_id> generation;
			std::mutex exclusive;
			thread_debug_callback debug;
			desc policy;
			bool terminate;
			bool enqueue;
			bool suspended;
			bool active;

		public:
			schedule() noexcept;
			virtual ~schedule() noexcept override;
			task_id set_interval(uint64_t milliseconds, task_callback&& callback);
			task_id set_timeout(uint64_t milliseconds, task_callback&& callback);
			bool set_task(task_callback&& callback, bool recyclable = true);
			bool set_coroutine(task_callback&& callback, bool recyclable = true);
			bool set_debug_callback(thread_debug_callback&& callback);
			bool clear_timeout(task_id work_id);
			bool trigger_timers();
			bool trigger(difficulty type);
			bool start(const desc& new_policy);
			bool stop();
			bool wakeup();
			bool dispatch();
			bool is_active() const;
			bool can_enqueue() const;
			bool has_tasks(difficulty type) const;
			bool has_any_tasks() const;
			bool is_suspended() const;
			void suspend();
			void resume();
			size_t get_thread_global_index();
			size_t get_thread_local_index();
			size_t get_total_threads() const;
			size_t get_threads(difficulty type) const;
			bool has_parallel_threads(difficulty type) const;
			const thread_data* get_thread() const;
			const desc& get_policy() const;

		private:
			const thread_data* initialize_thread(thread_data* source, bool update) const;
			void initialize_spawn_trigger();
			bool fast_bypass_enqueue(difficulty type, task_callback&& callback);
			bool report_thread(thread_task state, size_t tasks, const thread_data* thread);
			bool trigger_thread(difficulty type, thread_data* thread);
			bool sleep_thread(difficulty type, thread_data* thread);
			bool thread_active(thread_data* thread);
			bool chunk_cleanup();
			bool push_thread(difficulty type, size_t global_index, size_t local_index, bool is_daemon);
			bool pop_thread(thread_data* thread);
			std::chrono::microseconds get_timeout(std::chrono::microseconds clock);
			task_id get_task_id();

		public:
			static bool is_available(difficulty type = difficulty::count);
			static std::chrono::microseconds get_clock();
		};

		typedef vector<uptr<schema>> schema_list;
		typedef unordered_map<string, uptr<schema>> schema_args;

		template <>
		class ualloc<schedule>
		{
		private:
			schedule::thread_data* thread;

		public:
			ualloc() noexcept : thread((schedule::thread_data*)schedule::get()->get_thread())
			{
				VI_PANIC(!thread, "this thread is not a scheduler thread");
				memory::set_local_allocator(&thread->allocator);
			}
			ualloc(const ualloc& other) noexcept = delete;
			ualloc(ualloc&& other) noexcept = delete;
			~ualloc()
			{
				memory::set_local_allocator(nullptr);
				thread->allocator.reset();
			}
			ualloc& operator= (const ualloc& other) noexcept = delete;
			ualloc& operator= (ualloc&& other) noexcept = delete;
		};

		template <typename t>
		class pool
		{
			static_assert(std::is_pointer<t>::value, "pool can be used only for pointer types");

		public:
			typedef t* iterator;

		protected:
			size_t count, volume;
			t* data;

		public:
			pool() noexcept : count(0), volume(0), data(nullptr)
			{
			}
			pool(const pool<t>& ref) noexcept : count(0), volume(0), data(nullptr)
			{
				if (ref.data != nullptr)
					copy(ref);
			}
			pool(pool<t>&& ref) noexcept : count(ref.count), volume(ref.volume), data(ref.data)
			{
				ref.count = 0;
				ref.volume = 0;
				ref.data = nullptr;
			}
			~pool() noexcept
			{
				release();
			}
			void release()
			{
				memory::deallocate(data);
				data = nullptr;
				count = 0;
				volume = 0;
			}
			void reserve(size_t capacity)
			{
				if (capacity <= volume)
					return;

				size_t size = capacity * sizeof(t);
				t* new_data = memory::allocate<t>(size);
				memset(new_data, 0, size);

				if (data != nullptr)
				{
					memcpy(new_data, data, count * sizeof(t));
					memory::deallocate(data);
				}

				volume = capacity;
				data = new_data;
			}
			void resize(size_t new_size)
			{
				if (new_size > volume)
					reserve(increase_capacity(new_size));

				count = new_size;
			}
			void copy(const pool<t>& ref)
			{
				if (!ref.data)
					return;

				if (!data || ref.count > count)
				{
					memory::deallocate<t>(data);
					data = memory::allocate<t>(ref.volume * sizeof(t));
					memset(data, 0, ref.volume * sizeof(t));
				}

				memcpy(data, ref.data, (size_t)(ref.count * sizeof(t)));
				count = ref.count;
				volume = ref.volume;
			}
			void clear()
			{
				count = 0;
			}
			iterator add(const t& ref)
			{
				VI_ASSERT(count < volume, "pool capacity overflow");
				data[count++] = ref;
				return end() - 1;
			}
			iterator add_unbounded(const t& ref)
			{
				if (count + 1 >= volume)
					reserve(increase_capacity(count + 1));

				return add(ref);
			}
			iterator add_if_not_exists(const t& ref)
			{
				iterator it = find(ref);
				if (it != end())
					return it;

				return add(ref);
			}
			iterator remove_at(iterator it)
			{
				VI_ASSERT(count > 0, "pool is empty");
				VI_ASSERT((size_t)(it - data) >= 0 && (size_t)(it - data) < count, "iterator ranges out of pool");

				count--;
				data[it - data] = data[count];
				return it;
			}
			iterator remove(const t& value)
			{
				iterator it = find(value);
				if (it != end())
					remove_at(it);

				return it;
			}
			iterator at(size_t index) const
			{
				VI_ASSERT(index < count, "index ranges out of pool");
				return data + index;
			}
			iterator find(const t& ref)
			{
				auto end = data + count;
				for (auto it = data; it != end; ++it)
				{
					if (*it == ref)
						return it;
				}

				return end;
			}
			iterator begin() const
			{
				return data;
			}
			iterator end() const
			{
				return data + count;
			}
			size_t size() const
			{
				return count;
			}
			size_t capacity() const
			{
				return volume;
			}
			bool empty() const
			{
				return !count;
			}
			t* get() const
			{
				return data;
			}
			t& front() const
			{
				return *begin();
			}
			t& back() const
			{
				return *(end() - 1);
			}
			t& operator [](size_t index) const
			{
				return *(data + index);
			}
			t& operator [](size_t index)
			{
				return *(data + index);
			}
			pool<t>& operator =(const pool<t>& ref) noexcept
			{
				if (this == &ref)
					return *this;

				copy(ref);
				return *this;
			}
			pool<t>& operator =(pool<t>&& ref) noexcept
			{
				if (this == &ref)
					return *this;

				release();
				count = ref.count;
				volume = ref.volume;
				data = ref.data;
				ref.count = 0;
				ref.volume = 0;
				ref.data = nullptr;
				return *this;
			}
			bool operator ==(const pool<t>& raw)
			{
				return compare(raw) == 0;
			}
			bool operator !=(const pool<t>& raw)
			{
				return compare(raw) != 0;
			}
			bool operator <(const pool<t>& raw)
			{
				return compare(raw) < 0;
			}
			bool operator <=(const pool<t>& raw)
			{
				return compare(raw) <= 0;
			}
			bool operator >(const pool<t>& raw)
			{
				return compare(raw) > 0;
			}
			bool operator >=(const pool<t>& raw)
			{
				return compare(raw) >= 0;
			}

		private:
			size_t increase_capacity(size_t new_size)
			{
				size_t alpha = volume > 4 ? (volume + volume / 2) : 8;
				return alpha > new_size ? alpha : new_size;
			}
		};

		struct parallel_executor
		{
			inline void operator()(task_callback&& callback, bool async)
			{
				if (async && schedule::is_available())
					schedule::get()->set_task(std::move(callback));
				else
					callback();
			}
		};

		template <typename t>
		struct promise_state
		{
			std::mutex update;
			task_callback event;
			alignas(t) char value[sizeof(t)];
			std::atomic<uint32_t> count;
			std::atomic<deferred> code;
#ifndef NDEBUG
			const t* hidden_value = (const t*)value;
#endif
			promise_state() noexcept : count(1), code(deferred::pending)
			{
			}
			promise_state(const t& new_value) noexcept : count(1), code(deferred::ready)
			{
				option_utils::copy_buffer<t>(value, (const char*)&new_value, sizeof(t));
			}
			promise_state(t&& new_value) noexcept : count(1), code(deferred::ready)
			{
				option_utils::move_buffer<t>(value, (char*)&new_value, sizeof(t));
			}
			~promise_state()
			{
				if (code == deferred::ready)
					((t*)value)->~t();
			}
			void emplace(const t& new_value)
			{
				VI_ASSERT(code != deferred::ready, "emplacing to already initialized memory is not desired");
				option_utils::copy_buffer<t>(value, (const char*)&new_value, sizeof(t));
			}
			void emplace(t&& new_value)
			{
				VI_ASSERT(code != deferred::ready, "emplacing to already initialized memory is not desired");
				option_utils::move_buffer<t>(value, (char*)&new_value, sizeof(t));
			}
			t& unwrap()
			{
				VI_PANIC(code == deferred::ready, "unwrapping uninitialized memory will result in an undefined behaviour");
				return *(t*)value;
			}
		};

		template <>
		struct promise_state<void>
		{
			std::mutex update;
			task_callback event;
			std::atomic<uint32_t> count;
			std::atomic<deferred> code;

			promise_state() noexcept : count(1), code(deferred::pending)
			{
			}
		};

		template <typename t, typename executor>
		class basic_promise
		{
		public:
			typedef promise_state<t> status;
			typedef t type;

		private:
			template <typename f>
			struct unwrap
			{
				typedef typename std::remove_reference<f>::type type;
			};

			template <typename f>
			struct unwrap<basic_promise<f, executor>>
			{
				typedef typename std::remove_reference<f>::type type;
			};

		private:
			status* data;

		public:
			basic_promise() noexcept : data(memory::init<status>())
			{
			}
			basic_promise(const t& value) noexcept : data(memory::init<status>(value))
			{
			}
			basic_promise(t&& value) noexcept : data(memory::init<status>(std::move(value)))
			{
			}
			basic_promise(const basic_promise& other) noexcept : data(other.data)
			{
				add_ref();
			}
			basic_promise(basic_promise&& other) noexcept : data(other.data)
			{
				other.data = nullptr;
			}
			~basic_promise() noexcept
			{
				release(data);
			}
			basic_promise& operator= (const basic_promise& other) = delete;
			basic_promise& operator= (basic_promise&& other) noexcept
			{
				if (&other == this)
					return *this;

				release(data);
				data = other.data;
				other.data = nullptr;
				return *this;
			}
			void set(const t& other) noexcept
			{
				VI_ASSERT(data != nullptr && data->code != deferred::ready, "async should be pending");
				umutex<std::mutex> unique(data->update);
				bool async = (data->code != deferred::waiting);
				data->emplace(other);
				data->code = deferred::ready;
				execute(data, unique, async);
			}
			void set(t&& other) noexcept
			{
				VI_ASSERT(data != nullptr && data->code != deferred::ready, "async should be pending");
				umutex<std::mutex> unique(data->update);
				bool async = (data->code != deferred::waiting);
				data->emplace(std::move(other));
				data->code = deferred::ready;
				execute(data, unique, async);
			}
			void set(const basic_promise& other) noexcept
			{
				VI_ASSERT(data != nullptr && data->code != deferred::ready, "async should be pending");
				status* copy = add_ref();
				other.when([copy](t&& value) mutable
				{
					{
						umutex<std::mutex> unique(copy->update);
						bool async = (copy->code != deferred::waiting);
						copy->emplace(std::move(value));
						copy->code = deferred::ready;
						execute(copy, unique, async);
					}
					release(copy);
				});
			}
			void when(std::function<void(t&&)>&& callback) const noexcept
			{
				VI_ASSERT(data != nullptr && callback, "callback should be set");
				if (!is_pending())
					return callback(std::move(data->unwrap()));

				status* copy = add_ref();
				store([copy, callback = std::move(callback)]() mutable
				{
					callback(std::move(copy->unwrap()));
					release(copy);
				});
			}
			void wait() noexcept
			{
				if (!is_pending())
					return;

				status* copy;
				{
					std::unique_lock<std::mutex> unique(data->update);
					if (data->code == deferred::ready)
						return;

					std::condition_variable ready;
					copy = add_ref();
					copy->code = deferred::waiting;
					copy->event = [&ready]()
					{
						ready.notify_all();
					};
					ready.wait(unique, [this]()
					{
						return !is_pending();
					});
				}
				release(copy);
			}
			t&& get() noexcept
			{
				wait();
				return load();
			}
			deferred get_status() const noexcept
			{
				return data ? data->code.load() : deferred::ready;
			}
			bool is_pending() const noexcept
			{
				return data ? data->code != deferred::ready : false;
			}
			bool is_null() const noexcept
			{
				return !data;
			}
			template <typename r>
			basic_promise<r, executor> then(std::function<void(basic_promise<r, executor>&, t&&)>&& callback) const noexcept
			{
				VI_ASSERT(data != nullptr && callback, "async should be pending");
				basic_promise<r, executor> result; status* copy = add_ref();
				store([copy, result, callback = std::move(callback)]() mutable
				{
					callback(result, std::move(copy->unwrap()));
					release(copy);
				});

				return result;
			}
			basic_promise<void, executor> then(std::function<void(basic_promise<void, executor>&, t&&)>&& callback) const noexcept
			{
				VI_ASSERT(data != nullptr && callback, "async should be pending");
				basic_promise<void, executor> result; status* copy = add_ref();
				store([copy, result, callback = std::move(callback)]() mutable
				{
					callback(result, std::move(copy->unwrap()));
					release(copy);
				});

				return result;
			}
			template <typename r>
			basic_promise<typename unwrap<r>::type, executor> then(std::function<r(t&&)>&& callback) const noexcept
			{
				VI_ASSERT(data != nullptr && callback, "async should be pending");
				basic_promise<typename unwrap<r>::type, executor> result; status* copy = add_ref();
				store([copy, result, callback = std::move(callback)]() mutable
				{
					result.set(std::move(callback(std::move(copy->unwrap()))));
					release(copy);
				});

				return result;
			}
			basic_promise<void, executor> then(std::function<void(t&&)>&& callback) const noexcept
			{
				VI_ASSERT(data != nullptr, "async should be pending");
				basic_promise<void, executor> result; status* copy = add_ref();
				store([copy, result, callback = std::move(callback)]() mutable
				{
					if (callback)
						callback(std::move(copy->unwrap()));
					result.set();
					release(copy);
				});

				return result;
			}

		private:
			basic_promise(status* context, bool) noexcept : data(context)
			{
			}
			status* add_ref() const noexcept
			{
				if (data != nullptr)
					++data->count;
				return data;
			}
			t&& load() noexcept
			{
				if (!data)
					data = memory::init<status>();

				return std::move(data->unwrap());
			}
			void store(task_callback&& callback) const noexcept
			{
				umutex<std::mutex> unique(data->update);
				data->event = std::move(callback);
				if (data->code == deferred::ready)
					execute(data, unique, false);
			}

		public:
			static basic_promise null() noexcept
			{
				return basic_promise((status*)nullptr, false);
			}

		private:
			static void execute(status* state, umutex<std::mutex>& unique, bool async) noexcept
			{
				if (state->event)
				{
					task_callback callback = std::move(state->event);
					unique.negate();
					executor()(std::move(callback), async);
				}
			}
			static void release(status* state) noexcept
			{
				if (state != nullptr && !--state->count)
					memory::deinit(state);
			}
#ifdef VI_CXX20
		public:
			struct promise_type
			{
				basic_promise state;

				void return_value(const basic_promise& new_value) noexcept
				{
					state.set(new_value);
				}
				void return_value(const t& new_value) noexcept
				{
					state.set(new_value);
				}
				void return_value(t&& new_value) noexcept
				{
					state.set(std::move(new_value));
				}
				void unhandled_exception() noexcept
				{
					VI_PANIC(false, "a coroutine has thrown an exception (invalid behaviour)");
				}
				auto get_return_object() noexcept
				{
					return state;
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
				void* operator new(size_t size) noexcept
				{
					return memory::allocate<promise_type>(size);
				}
				void operator delete(void* address) noexcept
				{
					memory::deallocate<void>(address);
				}
				static auto get_return_object_on_allocation_failure() noexcept
				{
					return basic_promise::null();
				}
#endif // E3394 intellisense false positive
			};

		public:
			bool await_ready() const noexcept
			{
				return !is_pending();
			}
			t&& await_resume() noexcept
			{
				return load();
			}
			void await_suspend(std::coroutine_handle<> handle) noexcept
			{
				if (!is_pending())
					return handle.resume();

				status* copy = add_ref();
#ifndef NDEBUG
				std::chrono::microseconds time = schedule::get_clock();
				VI_WATCH(handle.address(), typeid(handle).name());
				store([copy, time, handle]()
				{
					int64_t diff = (schedule::get_clock() - time).count();
					if (diff > (int64_t)timings::hangup * 1000)
						VI_WARN("[stall] async operation took %" PRIu64 " ms (%" PRIu64 " us, expected < %" PRIu64 " ms)", diff / 1000, diff, (uint64_t)timings::hangup);

					VI_UNWATCH(handle.address());
					handle.resume();
					release(copy);
				});
#else
				store([copy, handle]()
				{
					handle.resume();
					release(copy);
				});
#endif
			}
#endif
		};

		template <typename executor>
		class basic_promise<void, executor>
		{
		public:
			typedef promise_state<void> status;
			typedef void type;

		private:
			template <typename f>
			struct unwrap
			{
				typedef f type;
			};

			template <typename f>
			struct unwrap<basic_promise<f, executor>>
			{
				typedef f type;
			};

		private:
			status* data;

		public:
			basic_promise() noexcept : data(memory::init<status>())
			{
			}
			basic_promise(const basic_promise& other) noexcept : data(other.data)
			{
				add_ref();
			}
			basic_promise(basic_promise&& other) noexcept : data(other.data)
			{
				other.data = nullptr;
			}
			~basic_promise() noexcept
			{
				release(data);
			}
			basic_promise& operator= (const basic_promise& other) = delete;
			basic_promise& operator= (basic_promise&& other) noexcept
			{
				if (&other == this)
					return *this;

				release(data);
				data = other.data;
				other.data = nullptr;
				return *this;
			}
			void set() noexcept
			{
				VI_ASSERT(data != nullptr && data->code != deferred::ready, "async should be pending");
				umutex<std::mutex> unique(data->update);
				bool async = (data->code != deferred::waiting);
				data->code = deferred::ready;
				execute(data, unique, async);
			}
			void set(const basic_promise& other) noexcept
			{
				VI_ASSERT(data != nullptr && data->code != deferred::ready, "async should be pending");
				status* copy = add_ref();
				other.when([copy]() mutable
				{
					{
						umutex<std::mutex> unique(copy->update);
						bool async = (copy->code != deferred::waiting);
						copy->code = deferred::ready;
						execute(copy, unique, async);
					}
					release(copy);
				});
			}
			void when(std::function<void()>&& callback) const noexcept
			{
				VI_ASSERT(callback, "callback should be set");
				if (!is_pending())
					return callback();

				status* copy = add_ref();
				store([copy, callback = std::move(callback)]() mutable
				{
					callback();
					release(copy);
				});
			}
			void wait() noexcept
			{
				if (!is_pending())
					return;

				status* copy;
				{
					std::unique_lock<std::mutex> lock(data->update);
					if (data->code == deferred::ready)
						return;

					std::condition_variable ready;
					copy = add_ref();
					copy->code = deferred::waiting;
					copy->event = [&ready]()
					{
						ready.notify_all();
					};
					ready.wait(lock, [this]()
					{
						return !is_pending();
					});
				}
				release(copy);
			}
			void get() noexcept
			{
				wait();
			}
			deferred get_status() const noexcept
			{
				return data ? data->code.load() : deferred::ready;
			}
			bool is_pending() const noexcept
			{
				return data ? data->code != deferred::ready : false;
			}
			bool is_null() const noexcept
			{
				return !data;
			}
			template <typename r>
			basic_promise<r, executor> then(std::function<void(basic_promise<r, executor>&)>&& callback) const noexcept
			{
				VI_ASSERT(callback, "callback should be set");
				basic_promise<r, executor> result;
				if (data != nullptr)
				{
					status* copy = add_ref();
					store([copy, result, callback = std::move(callback)]() mutable
					{
						callback(result);
						release(copy);
					});
				}
				else
					callback(result);

				return result;
			}
			basic_promise<void, executor> then(std::function<void(basic_promise<void, executor>&)>&& callback) const noexcept
			{
				VI_ASSERT(callback, "callback should be set");
				basic_promise<void, executor> result;
				if (data != nullptr)
				{
					status* copy = add_ref();
					store([copy, result, callback = std::move(callback)]() mutable
					{
						callback(result);
						release(copy);
					});
				}
				else
					callback(result);

				return result;
			}
			template <typename r>
			basic_promise<typename unwrap<r>::type, executor> then(std::function<r()>&& callback) const noexcept
			{
				VI_ASSERT(callback, "callback should be set");
				basic_promise<typename unwrap<r>::type, executor> result;
				if (data != nullptr)
				{
					status* copy = add_ref();
					store([copy, result, callback = std::move(callback)]() mutable
					{
						result.set(std::move(callback()));
						release(copy);
					});
				}
				else
					result.set(std::move(callback()));

				return result;
			}
			basic_promise<void, executor> then(std::function<void()>&& callback) const noexcept
			{
				basic_promise<void, executor> result;
				if (!is_pending())
				{
					if (callback)
						callback();
					result.set();
					return result;
				}

				status* copy = add_ref();
				store([copy, result, callback = std::move(callback)]() mutable
				{
					if (callback)
						callback();
					result.set();
					release(copy);
				});

				return result;
			}

		private:
			basic_promise(status* context, bool) noexcept : data(context)
			{
			}
			status* add_ref() const noexcept
			{
				if (data != nullptr)
					++data->count;
				return data;
			}
			void load() noexcept
			{
				if (!data)
					data = memory::init<status>();
			}
			void store(task_callback&& callback) const noexcept
			{
				umutex<std::mutex> unique(data->update);
				data->event = std::move(callback);
				if (data->code == deferred::ready)
					execute(data, unique, false);
			}

		public:
			static basic_promise null() noexcept
			{
				return basic_promise((status*)nullptr, false);
			}

		private:
			static void execute(status* state, umutex<std::mutex>& unique, bool async) noexcept
			{
				if (state->event)
				{
					task_callback callback = std::move(state->event);
					unique.negate();
					executor()(std::move(callback), async);
				}
			}
			static void release(status* state) noexcept
			{
				if (state != nullptr && !--state->count)
					memory::deinit(state);
			}
#ifdef VI_CXX20
		public:
			struct promise_type
			{
				basic_promise state;

				void return_void() noexcept
				{
					state.set();
				}
				void unhandled_exception() noexcept
				{
					VI_PANIC(false, "a coroutine has thrown an exception (invalid behaviour)");
				}
				auto get_return_object() noexcept
				{
					return state;
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
				void* operator new(size_t size) noexcept
				{
					return memory::allocate<basic_promise>(size);
				}
				void operator delete(void* address) noexcept
				{
					memory::deallocate<void>(address);
				}
				static auto get_return_object_on_allocation_failure() noexcept
				{
					return basic_promise::null();
				}
#endif // E3394 intellisense false positive
			};

		public:
			bool await_ready() const noexcept
			{
				return !is_pending();
			}
			void await_resume() noexcept
			{
			}
			void await_suspend(std::coroutine_handle<> handle) noexcept
			{
				if (!is_pending())
					return handle.resume();

				status* copy = add_ref();
#ifndef NDEBUG
				std::chrono::microseconds time = schedule::get_clock();
				VI_WATCH(handle.address(), typeid(handle).name());
				store([copy, time, handle]()
				{
					int64_t diff = (schedule::get_clock() - time).count();
					if (diff > (int64_t)timings::hangup * 1000)
						VI_WARN("[stall] async operation took %" PRIu64 " ms (%" PRIu64 " us, expected < %" PRIu64 " ms)", diff / 1000, diff, (uint64_t)timings::hangup);

					VI_UNWATCH(handle.address());
					handle.resume();
					release(copy);
				});
#else
				store([copy, handle]()
				{
					handle.resume();
					release(copy);
				});
#endif
			}
#endif
		};

		template <typename t, typename executor>
		class basic_generator
		{
			static_assert(!std::is_same<t, void>::value, "value type should not be void");

		public:
			typedef std::function<basic_promise<void, executor>(basic_generator&)> executor_callback;

		public:
			struct state
			{
				alignas(t) char value[sizeof(t)];
				basic_promise<void, executor> input = basic_promise<void, executor>::null();
				basic_promise<void, executor> output = basic_promise<void, executor>::null();
				executor_callback callback = nullptr;
				std::atomic<uint32_t> count = 1;
				std::atomic<bool> exit = false;
				std::atomic<bool> next = false;
#ifndef NDEBUG
				const t* hidden_value = (const t*)value;
#endif
			};

		private:
			state* status;

		public:
			basic_generator(executor_callback&& callback) noexcept : status(memory::init<state>())
			{
				status->callback = std::move(callback);
			}
			basic_generator(const basic_generator& other) noexcept : status(other.status)
			{
				if (status != nullptr)
					++status->count;
			}
			basic_generator(basic_generator&& other) noexcept : status(other.status)
			{
				other.status = nullptr;
			}
			~basic_generator() noexcept
			{
				if (status != nullptr && !--status->count)
					memory::deinit(status);
			}
			basic_generator& operator= (const basic_generator& other) noexcept
			{
				if (this == &other)
					return *this;

				this->~basic_generator();
				status = other.status;
				if (status != nullptr)
					++status->count;

				return *this;
			}
			basic_generator& operator= (basic_generator&& other) noexcept
			{
				if (this == &other)
					return *this;

				this->~basic_generator();
				status = other.status;
				other.status = nullptr;

				return *this;
			}
			basic_promise<bool, executor> next()
			{
				VI_ASSERT(status != nullptr, "generator is not valid");
				bool is_entrypoint = entrypoint();
				if (status->output.is_null())
					return basic_promise<bool, executor>(false);

				if (!is_entrypoint && status->next)
				{
					status->next = false;
					status->output = basic_promise<void, executor>();
					status->input.set();
				}

				return status->output.template then<bool>([this]() -> bool
				{
					return !status->exit.load() && status->next.load();
				});
			}
			t&& operator()()
			{
				VI_ASSERT(status != nullptr, "generator is not valid");
				VI_ASSERT(!status->exit.load() && status->next.load(), "generator does not have a value");
				t& value = *(t*)status->value;
				return std::move(value);
			}
			basic_promise<void, executor> operator<< (const t& value)
			{
				VI_ASSERT(status != nullptr, "generator is not valid");
				VI_ASSERT(!status->next, "generator already has a value");
				option_utils::copy_buffer<t>(status->value, (const char*)&value, sizeof(t));
				status->next = true;
				status->input = basic_promise<void, executor>();
				status->output.set();
				return status->input;
			}
			basic_promise<void, executor> operator<< (t&& value)
			{
				VI_ASSERT(status != nullptr, "generator is not valid");
				VI_ASSERT(!status->next, "generator already has a value");
				option_utils::move_buffer<t>(status->value, (char*)&value, sizeof(t));
				status->next = true;
				status->input = basic_promise<void, executor>();
				status->output.set();
				return status->input;
			}

		private:
			bool entrypoint()
			{
				if (!status->callback)
					return false;

				status->output = basic_promise<void, executor>();
				auto generate = [this]()
				{
					executor_callback callback = std::move(status->callback);
					callback(*this).when([this]()
					{
						status->exit = true;
						if (status->output.is_pending())
							status->output.set();
					});
				};
				if (schedule::is_available())
					schedule::get()->set_task(generate);
				else
					generate();
				return true;
			}
		};

		template <typename t, typename executor = parallel_executor>
		using generator = basic_generator<t, executor>;

		template <typename t, typename executor = parallel_executor>
		using promise = basic_promise<t, executor>;

		template <typename t, typename e, typename executor = parallel_executor>
		using expects_promise = basic_promise<expects<t, e>, executor>;

		template <typename t, typename executor = parallel_executor>
		using expects_promise_io = basic_promise<expects_io<t>, executor>;

		template <typename t, typename executor = parallel_executor>
		using expects_promise_system = basic_promise<expects_system<t>, executor>;

		template <typename t>
		inline promise<t> cotask(std::function<t()>&& callback, bool recyclable = true) noexcept
		{
			VI_ASSERT(callback, "callback should not be empty");
			if (!schedule::is_available(difficulty::sync))
				return promise<t>(std::move(callback()));

			promise<t> result;
			schedule::get()->set_task([result, callback = std::move(callback)]() mutable { result.set(std::move(callback())); }, recyclable);
			return result;
		}
		template <>
		inline promise<void> cotask(std::function<void()>&& callback, bool recyclable) noexcept
		{
			VI_ASSERT(callback, "callback should not be empty");
			if (!schedule::is_available(difficulty::sync))
			{
				callback();
				return promise<void>::null();
			}

			promise<void> result;
			schedule::get()->set_task([result, callback = std::move(callback)]() mutable
			{
				callback();
				result.set();
			}, recyclable);
			return result;
		}
		inline void codefer(task_callback&& callback, bool always_execute = true) noexcept
		{
			VI_ASSERT(callback, "callback should be set");
			if (schedule::is_available())
				schedule::get()->set_task(std::move(callback), true);
			else if (always_execute)
				callback();
		}
		inline void cospawn(task_callback&& callback, bool always_execute = true) noexcept
		{
			VI_ASSERT(callback, "callback should be set");
			if (schedule::is_available())
				schedule::get()->set_task(std::move(callback), false);
			else if (always_execute)
				callback();
		}
#ifdef VI_CXX20
		template <typename t>
		inline promise<t> coasync(std::function<promise<t>()>&& callback, bool always_enqueue = false) noexcept
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			auto* callable = memory::init<std::function<promise<t>()>>(std::move(callback));
			auto finalize = [callable](t&& temp) -> t&& { memory::deinit(callable); return std::move(temp); };
			if (!always_enqueue)
				return (*callable)().template then<t>(finalize);

			promise<t> value;
			schedule::get()->set_task([value, callable]() mutable { value.set((*callable)()); }, !always_enqueue);
			return value.template then<t>(finalize);
		}
		template <>
		inline promise<void> coasync(std::function<promise<void>()>&& callback, bool always_enqueue) noexcept
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			auto* callable = memory::init<std::function<promise<void>()>>(std::move(callback));
			auto finalize = [callable]() { memory::deinit(callable); };
			if (!always_enqueue)
				return (*callable)().then(finalize);

			promise<void> value;
			schedule::get()->set_task([value, callable]() mutable { value.set((*callable)()); }, !always_enqueue);
			return value.then(finalize);
		}
#else
		template <typename t>
		inline t&& coawait(promise<t>&& future, const char* function = nullptr, const char* expression = nullptr) noexcept
		{
			if (!future.is_pending())
				return future.get();

			costate* state; coroutine* base;
			costate::get_state(&state, &base);
			VI_ASSERT(state != nullptr && base != nullptr, "cannot call await outside coroutine");
#ifndef NDEBUG
			std::chrono::microseconds time = schedule::get_clock();
			if (function != nullptr && expression != nullptr)
				VI_WATCH_AT((void*)&future, function, expression);
#endif
			state->deactivate(base, [&future, &state, &base]()
			{
				future.when([&state, &base](t&&) { state->activate(base); });
			});
#ifndef NDEBUG
			if (function != nullptr && expression != nullptr)
			{
				int64_t diff = (schedule::get_clock() - time).count();
				if (diff > (int64_t)timings::hangup * 1000)
					VI_WARN("[stall] %s(): \"%s\" operation took %" PRIu64 " ms (%" PRIu64 " us, expected < %" PRIu64 " ms)", function, expression, diff / 1000, diff, (uint64_t)timings::hangup);
				VI_UNWATCH((void*)&future);
			}
#endif
			return future.get();
		}
		inline void coawait(promise<void>&& future, const char* function = nullptr, const char* expression = nullptr) noexcept
		{
			if (!future.is_pending())
				return future.get();

			costate* state; coroutine* base;
			costate::get_state(&state, &base);
			VI_ASSERT(state != nullptr && base != nullptr, "cannot call await outside coroutine");
#ifndef NDEBUG
			std::chrono::microseconds time = schedule::get_clock();
			if (function != nullptr && expression != nullptr)
				VI_WATCH_AT((void*)&future, function, expression);
#endif
			state->deactivate(base, [&future, &state, &base]()
			{
				future.when([&state, &base]() { state->activate(base); });
			});
#ifndef NDEBUG
			if (function != nullptr && expression != nullptr)
			{
				int64_t diff = (schedule::get_clock() - time).count();
				if (diff > (int64_t)timings::hangup * 1000)
					VI_WARN("[stall] %s(): \"%s\" operation took %" PRIu64 " ms (%" PRIu64 " us, expected < %" PRIu64 " ms)", function, expression, diff / 1000, diff, (uint64_t)timings::hangup);
				VI_UNWATCH((void*)&future);
			}
#endif
			return future.get();
		}
		template <typename t>
		inline promise<t> coasync(std::function<promise<t>()>&& callback, bool always_enqueue = false) noexcept
		{
			VI_ASSERT(callback, "callback should not be empty");
			if (!always_enqueue && costate::is_coroutine())
				return callback();

			promise<t> result;
			schedule::get()->set_coroutine([result, callback = std::move(callback)]() mutable
			{
				callback().when([result](t&& value) mutable { result.set(std::move(value)); });
			}, !always_enqueue);
			return result;
		}
		template <>
		inline promise<void> coasync(std::function<promise<void>()>&& callback, bool always_enqueue) noexcept
		{
			VI_ASSERT(callback, "callback should not be empty");
			if (!always_enqueue && costate::is_coroutine())
				return callback();

			promise<void> result;
			schedule::get()->set_coroutine([result, callback = std::move(callback)]() mutable
			{
				callback().when([result]() mutable { result.set(); });
			}, !always_enqueue);
			return result;
		}
#endif
		template <typename t>
		inline generator<t> cogenerate(std::function<promise<void>(generator<t>&)>&& callback)
		{
			return generator<t>([callback = std::move(callback)](generator<t>& results) mutable -> promise<void>
			{
				return coasync<void>([results, callback = std::move(callback)]() mutable -> promise<void> { return callback(results); });
			});
		}
#ifdef VI_ALLOCATOR
		template <typename o, typename i>
		inline o copy(const i& other)
		{
			static_assert(!std::is_same<i, o>::value, "copy should be used to copy object of the same type with different allocator");
			static_assert(is_iterable<i>::value, "input type should be iterable");
			static_assert(is_iterable<o>::value, "output type should be iterable");
			return o(other.begin(), other.end());
		}
#else
		template <typename o, typename i>
		inline o&& copy(i&& other)
		{
			static_assert(is_iterable<i>::value, "input type should be iterable");
			static_assert(is_iterable<o>::value, "output type should be iterable");
			return std::move(other);
		}
		template <typename o, typename i>
		inline const o& copy(const i& other)
		{
			static_assert(is_iterable<i>::value, "input type should be iterable");
			static_assert(is_iterable<o>::value, "output type should be iterable");
			return other;
		}
#endif
#ifdef VI_CXX20
		inline const std::string_view& key_lookup_cast(const std::string_view& value)
		{
			return value;
		}
#else
		inline string key_lookup_cast(const std::string_view& value)
		{
			return string(value);
		}
#endif
		template <typename t>
		inline expects_io<t> from_string(const std::string_view& other, int base = 10)
		{
			static_assert(std::is_arithmetic<t>::value, "conversion can be done only to arithmetic types");
			t value;
			if constexpr (!std::is_integral<t>::value)
			{
#if defined(__clang__) && (!defined(VI_CXX20) || defined(VI_APPLE))
				os::error::clear();
				char* end = nullptr;
				if constexpr (!std::is_same<t, long double>::value)
				{
					if constexpr (!std::is_same<t, float>::value)
						value = strtod(other.data(), &end);
					else
						value = strtof(other.data(), &end);
				}
				else
					value = strtold(other.data(), &end);
				if (other.data() == end || os::error::occurred())
					return os::error::get_condition_or();
#else
				std::from_chars_result result = std::from_chars(other.data(), other.data() + other.size(), value, base == 16 ? std::chars_format::hex : std::chars_format::general);
				if (result.ec != std::errc())
					return std::make_error_condition(result.ec);
#endif
			}
			else
			{
				std::from_chars_result result = std::from_chars(other.data(), other.data() + other.size(), value, base);
				if (result.ec != std::errc())
					return std::make_error_condition(result.ec);
			}
			return value;
		}
		template <typename t>
		inline std::string_view to_string_view(char* buffer, size_t size, t other, int base = 10)
		{
			static_assert(std::is_arithmetic<t>::value, "conversion can be done only from arithmetic types");
			if constexpr (std::is_integral<t>::value)
			{
				std::to_chars_result result = std::to_chars(buffer, buffer + size, other, base);
				return result.ec == std::errc() ? std::string_view(buffer, result.ptr - buffer) : std::string_view();
			}
			else
			{
				std::to_chars_result result = std::to_chars(buffer, buffer + size, other, base == 16 ? std::chars_format::hex : std::chars_format::fixed);
				return result.ec == std::errc() ? std::string_view(buffer, result.ptr - buffer) : std::string_view();
			}
		}
		template <typename t>
		inline string to_string(t other, int base = 10)
		{
			char buffer[32];
			return string(to_string_view<t>(buffer, sizeof(buffer), other, base));
		}
	}
}
#endif
