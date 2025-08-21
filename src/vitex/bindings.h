#define VI_BINDINGS_H
#include "scripting.h"
#include "layer.h"
#include <tuple>
#define VI_TYPEREF(name, type_name) static const uint64_t name = vitex::core::os::file::get_index<sizeof(type_name)>(type_name); vitex::scripting::type_cache::set(name, type_name)
#define VI_PROMISIFY(member_function, type_id) vitex::scripting::bindings::promise::ify<decltype(&member_function), &member_function>::id<type_id>
#define VI_PROMISIFY_REF(member_function, type_ref) vitex::scripting::bindings::promise::ify<decltype(&member_function), &member_function>::decl<type_ref>
#define VI_SPROMISIFY(function, type_id) vitex::scripting::bindings::promise::ify_static<decltype(&function), &function>::id<type_id>
#define VI_SPROMISIFY_REF(function, type_ref) vitex::scripting::bindings::promise::ify_static<decltype(&function), &function>::decl<type_ref>
#define VI_ARRAYIFY(member_function, type_id) vitex::scripting::bindings::array::ify<decltype(&member_function), &member_function>::id<type_id>
#define VI_ARRAYIFY_REF(member_function, type_ref) vitex::scripting::bindings::array::ify<decltype(&member_function), &member_function>::decl<type_ref>
#define VI_SARRAYIFY(function, type_id) vitex::scripting::bindings::array::ify_static<decltype(&function), &function>::id<type_id>
#define VI_SARRAYIFY_REF(function, type_ref) vitex::scripting::bindings::array::ify_static<decltype(&function), &function>::decl<type_ref>
#define VI_ANYIFY(member_function, type_id) vitex::scripting::bindings::any::ify<decltype(&member_function), &member_function>::id<type_id>
#define VI_ANYIFY_REF(member_function, type_ref) vitex::scripting::bindings::any::ify<decltype(&member_function), &member_function>::decl<type_ref>
#define VI_SANYIFY(function, type_id) vitex::scripting::bindings::any::ify_static<decltype(&function), &function>::id<type_id>
#define VI_SANYIFY_REF(function, type_ref) vitex::scripting::bindings::any::ify_static<decltype(&function), &function>::decl<type_ref>
#define VI_EXPECTIFY(member_function) vitex::scripting::bindings::expects_wrapper::ify<decltype(&member_function), &member_function>::throws
#define VI_EXPECTIFY_VOID(member_function) vitex::scripting::bindings::expects_wrapper::ify_void<decltype(&member_function), &member_function>::throws
#define VI_SEXPECTIFY(function) vitex::scripting::bindings::expects_wrapper::ify_static<decltype(&function), &function>::throws
#define VI_SEXPECTIFY_VOID(function) vitex::scripting::bindings::expects_wrapper::ify_static_void<decltype(&function), &function>::throws
#define VI_OPTIONIFY(member_function) vitex::scripting::bindings::option_wrapper::ify<decltype(&member_function), &member_function>::throws
#define VI_OPTIONIFY_VOID(member_function) vitex::scripting::bindings::option_wrapper::ify_void<decltype(&member_function), &member_function>::throws
#define VI_SOPTIONIFY(function) vitex::scripting::bindings::option_wrapper::ify_static<decltype(&function), &function>::throws
#define VI_SOPTIONIFY_VOID(function) vitex::scripting::bindings::option_wrapper::ify_static_void<decltype(&function), &function>::throws
#ifdef __LP64__
typedef unsigned int as_uint32_t;
typedef unsigned long as_uint64_t;
typedef long as_int64_t;
#else
typedef unsigned long as_uint32_t;
#if !defined(_MSC_VER) && (defined(__GNUC__) || defined(__MWERKS__) || defined(__SUNPRO_CC) || defined(__psp2__))
typedef uint64_t as_uint64_t;
typedef int64_t as_int64_t;
#else
typedef unsigned __int64 as_uint64_t;
typedef __int64 as_int64_t;
#endif
#endif
typedef unsigned int as_size_t;

namespace vitex
{
	namespace scripting
	{
		namespace bindings
		{
			class promise;

			class array;

			struct dynamic
			{
				union
				{
					as_int64_t integer;
					double number;
					void* object;
				};

				int type_id;

				dynamic()
				{
					clean();
				}
				dynamic(int new_type_id)
				{
					clean();
					type_id = new_type_id;
				}
				void clean()
				{
					memset((void*)this, 0, sizeof(*this));
				}
			};

			class utils
			{
			public:
				template <typename function, typename tuple, std::size_t... i>
				static constexpr decltype(auto) apply_pack(function&& f, tuple&& t, std::index_sequence<i...>)
				{
					return static_cast<function&&>(f)(std::get<i>(static_cast<tuple&&>(t))...);
				}
				template <typename function, typename tuple>
				static constexpr decltype(auto) apply(function&& f, tuple&& t)
				{
					return utils::apply_pack(static_cast<function&&>(f), static_cast<tuple&&>(t), std::make_index_sequence<std::tuple_size<std::remove_reference_t<tuple>>::value>{});
				}
				template <typename lambda_type, typename... args_type>
				static auto capture_call(lambda_type&& lambda, args_type&&... args)
				{
					return [lambda = std::forward<lambda>(lambda), capture_args = std::forward_as_tuple(args...)](auto&&... original_args) mutable
					{
						return utils::apply([&lambda](auto&&... args)
						{
							lambda(std::forward<decltype(args)>(args)...);
						}, std::tuple_cat(std::forward_as_tuple(original_args...), utils::apply([](auto&&... args)
						{
							return std::forward_as_tuple(std::move(args)...);
						}, std::move(capture_args))));
					};
				}
			};

			class imports
			{
			public:
				static void bind_syntax(virtual_machine* vm, bool enabled, const std::string_view& syntax);
				static expects_vm<void> generator_callback(compute::preprocessor* base, const std::string_view& path, core::string& code, const std::string_view& syntax);
			};

			class tags
			{
			public:
				enum class tag_type
				{
					unknown,
					type,
					function,
					property_function,
					variable,
					not_type
				};

				struct tag_directive
				{
					core::unordered_map<core::string, core::string> args;
					core::string name;
				};

				struct tag_declaration
				{
					std::string_view class_name;
					std::string_view name;
					core::string declaration;
					core::string name_space;
					core::vector<tag_directive> directives;
					tag_type type = tag_type::unknown;
				};

			public:
				typedef core::vector<tag_declaration> tag_info;
				typedef std::function<void(virtual_machine*, tag_info&&)> tag_callback;

			public:
				static void bind_syntax(virtual_machine* vm, bool enabled, const tag_callback& callback);
				static expects_vm<void> generator_callback(compute::preprocessor* base, const std::string_view& path, core::string& code, virtual_machine* vm, const tag_callback& callback);

			private:
				static size_t extract_field(virtual_machine* vm, core::string& code, size_t offset, tag_declaration& tag);
				static void extract_declaration(virtual_machine* vm, core::string& code, size_t offset, tag_declaration& tag);
				static void append_directive(tag_declaration& tag, core::string& directive);
			};

			class exception
			{
			public:
				struct pointer
				{
					core::string type;
					core::string text;
					core::string origin;
					immediate_context* context;

					pointer();
					pointer(immediate_context* context);
					pointer(const std::string_view& data);
					pointer(const std::string_view& type, const std::string_view& text);
					void load_exception_data(const std::string_view& data);
					const core::string& get_type() const;
					const core::string& get_text() const;
					core::string to_exception_string() const;
					core::string what() const;
					core::string load_stack_here() const;
					bool empty() const;
				};

			public:
				static void throw_ptr_at(immediate_context* context, const pointer& data);
				static void throw_ptr(const pointer& data);
				static void rethrow_at(immediate_context* context);
				static void rethrow();
				static bool has_exception_at(immediate_context* context);
				static bool has_exception();
				static pointer get_exception_at(immediate_context* context);
				static pointer get_exception();
				static expects_vm<void> generator_callback(compute::preprocessor* base, const std::string_view& path, core::string& code);
			};

			class expects_wrapper
			{
			public:
				static exception::pointer translate_throw(const std::exception& error);
				static exception::pointer translate_throw(const std::error_code& error);
				static exception::pointer translate_throw(const std::string_view& error);
				static exception::pointer translate_throw(const std::error_condition& error);
				static exception::pointer translate_throw(const core::basic_exception& error);
				static exception::pointer translate_throw(const core::string& error);

			public:
				template <typename v, typename e>
				static v&& unwrap(core::expects<v, e>&& subresult, v&& default_value, immediate_context* context = immediate_context::get())
				{
					if (subresult)
						return std::move(*subresult);

					exception::throw_ptr_at(context, translate_throw(subresult.error()));
					return std::move(default_value);
				}
				template <typename v, typename e>
				static bool unwrap_void(core::expects<v, e>&& subresult, immediate_context* context = immediate_context::get())
				{
					if (!subresult)
						exception::throw_ptr_at(context, translate_throw(subresult.error()));
					return !!subresult;
				}

			public:
				template <typename t, t>
				struct ify;

				template <typename t, t>
				struct ify_void;

				template <typename t, t>
				struct ify_static;

				template <typename t, t>
				struct ify_static_void;

				template <typename t, typename v, typename e, typename ...args, core::expects<v, e>(t::* f)(args...)>
				struct ify<core::expects<v, e>(t::*)(args...), f>
				{
					static v throws(t* base, args... data)
					{
						core::expects<v, e> subresult((base->*f)(data...));
						if (subresult)
							return *subresult;

						exception::throw_ptr(translate_throw(subresult.error()));
						throw false;
					}
				};

				template <typename t, typename v, typename e, typename ...args, core::expects<v, e>(t::* f)(args...)>
				struct ify_void<core::expects<v, e>(t::*)(args...), f>
				{
					static bool throws(t* base, args... data)
					{
						core::expects<v, e> subresult((base->*f)(data...));
						if (!subresult)
							exception::throw_ptr(translate_throw(subresult.error()));
						return !!subresult;
					}
				};

				template <typename v, typename e, typename ...args, core::expects<v, e>(*f)(args...)>
				struct ify_static<core::expects<v, e>(*)(args...), f>
				{
					static v throws(args... data)
					{
						core::expects<v, e> subresult((*f)(data...));
						if (subresult)
							return *subresult;

						exception::throw_ptr(translate_throw(subresult.error()));
						throw false;
					}
				};

				template <typename v, typename e, typename ...args, core::expects<v, e>(*f)(args...)>
				struct ify_static_void<core::expects<v, e>(*)(args...), f>
				{
					static bool throws(args... data)
					{
						core::expects<v, e> subresult((*f)(data...));
						if (!subresult)
							exception::throw_ptr(translate_throw(subresult.error()));
						return !!subresult;
					}
				};
			};

			class option_wrapper
			{
			public:
				static exception::pointer translate_throw();

			public:
				template <typename v>
				static v&& unwrap(core::option<v>&& subresult, v&& default_value, immediate_context* context = immediate_context::get())
				{
					if (subresult)
						return std::move(*subresult);

					exception::throw_ptr_at(context, translate_throw());
					return std::move(default_value);
				}
				template <typename v>
				static bool unwrap_void(core::option<v>&& subresult, immediate_context* context = immediate_context::get())
				{
					if (!subresult)
						exception::throw_ptr_at(context, translate_throw());
					return !!subresult;
				}

			public:
				template <typename t, t>
				struct ify;

				template <typename t, t>
				struct ify_void;

				template <typename t, t>
				struct ify_static;

				template <typename t, t>
				struct ify_static_void;

				template <typename t, typename v, typename ...args, core::option<v>(t::* f)(args...)>
				struct ify<core::option<v>(t::*)(args...), f>
				{
					static v throws(t* base, args... data)
					{
						core::option<v> subresult((base->*f)(data...));
						if (subresult)
							return *subresult;

						exception::throw_ptr(translate_throw());
						throw false;
					}
				};

				template <typename t, typename v, typename ...args, core::option<v>(t::* f)(args...)>
				struct ify_void<core::option<v>(t::*)(args...), f>
				{
					static bool throws(t* base, args... data)
					{
						core::option<v> subresult((base->*f)(data...));
						if (!subresult)
							exception::throw_ptr(translate_throw());
						return !!subresult;
					}
				};

				template <typename v, typename ...args, core::option<v>(*f)(args...)>
				struct ify_static<core::option<v>(*)(args...), f>
				{
					static v throws(args... data)
					{
						core::option<v> subresult((*f)(data...));
						if (subresult)
							return *subresult;

						exception::throw_ptr(translate_throw());
						throw false;
					}
				};

				template <typename v, typename ...args, core::option<v>(*f)(args...)>
				struct ify_static_void<core::option<v>(*)(args...), f>
				{
					static bool throws(args... data)
					{
						core::option<v> subresult((*f)(data...));
						if (!subresult)
							exception::throw_ptr(translate_throw());
						return !!subresult;
					}
				};
			};

			class string
			{
			public:
				typedef vitex::core::unordered_map<vitex::core::string, std::atomic<int32_t>> factory_context;

			public:
				static std::string_view impl_cast_string_view(core::string& base);
				static void create(core::string* base);
				static void create_copy1(core::string* base, const core::string& other);
				static void create_copy2(core::string* base, const std::string_view& other);
				static void destroy(core::string* base);
				static void pop_back(core::string& base);
				static core::string& replace1(core::string& other, const core::string& from, const core::string& to, size_t start);
				static core::string& replace_part1(core::string& other, size_t start, size_t end, const core::string& to);
				static bool starts_with1(const core::string& other, const core::string& value, size_t offset);
				static bool ends_with1(const core::string& other, const core::string& value);
				static core::string& replace2(core::string& other, const std::string_view& from, const std::string_view& to, size_t start);
				static core::string& replace_part2(core::string& other, size_t start, size_t end, const std::string_view& to);
				static bool starts_with2(const core::string& other, const std::string_view& value, size_t offset);
				static bool ends_with2(const core::string& other, const std::string_view& value);
				static core::string substring1(core::string& base, size_t offset);
				static core::string substring2(core::string& base, size_t offset, size_t size);
				static core::string from_pointer(void* pointer);
				static core::string from_buffer(const char* buffer, size_t max_size);
				static char* index(core::string& base, size_t offset);
				static char* front(core::string& base);
				static char* back(core::string& base);
				static array* split(core::string& base, const std::string_view& delimiter);

			public:
				template <typename t>
				static t from_string(const core::string& text, int base = 10)
				{
					auto value = core::from_string<t>(text, base);
					return value ? *value : (t)0;
				}
			};

			class string_view
			{
			public:
				static core::string impl_cast_string(std::string_view& base);
				static void create(std::string_view* base);
				static void create_copy(std::string_view* base, core::string& other);
				static void assign(std::string_view* base, core::string& other);
				static void destroy(std::string_view* base);
				static bool starts_with(const std::string_view& other, const std::string_view& value, size_t offset);
				static bool ends_with(const std::string_view& other, const std::string_view& value);
				static int compare1(std::string_view& base, const core::string& other);
				static int compare2(std::string_view& base, const std::string_view& other);
				static core::string append1(const std::string_view& base, const std::string_view& other);
				static core::string append2(const std::string_view& base, const core::string& other);
				static core::string append3(const core::string& other, const std::string_view& base);
				static core::string append4(const std::string_view& base, char other);
				static core::string append5(char other, const std::string_view& base);
				static core::string substring1(std::string_view& base, size_t offset);
				static core::string substring2(std::string_view& base, size_t offset, size_t size);
				static size_t reverse_find1(std::string_view& base, const std::string_view& other, size_t offset);
				static size_t reverse_find2(std::string_view& base, char other, size_t offset);
				static size_t find1(std::string_view& base, const std::string_view& other, size_t offset);
				static size_t find2(std::string_view& base, char other, size_t offset);
				static size_t find_first_of(std::string_view& base, const std::string_view& other, size_t offset);
				static size_t find_first_not_of(std::string_view& base, const std::string_view& other, size_t offset);
				static size_t find_last_of(std::string_view& base, const std::string_view& other, size_t offset);
				static size_t find_last_not_of(std::string_view& base, const std::string_view& other, size_t offset);
				static std::string_view from_buffer(const char* buffer, size_t max_size);
				static char* index(std::string_view& base, size_t offset);
				static char* front(std::string_view& base);
				static char* back(std::string_view& base);
				static array* split(std::string_view& base, const std::string_view& delimiter);

			public:
				template <typename t>
				static t from_string(const std::string_view& text, int base = 10)
				{
					auto value = core::from_string<t>(text, base);
					return value ? *value : (t)0;
				}
			};

			class math
			{
			public:
				static float fp_from_ieee(uint32_t raw);
				static uint32_t fp_to_ieee(float fp);
				static double fp_from_ieee(as_uint64_t raw);
				static as_uint64_t fp_to_ieee(double fp);
				static bool close_to(float a, float b, float epsilon);
				static bool close_to(double a, double b, double epsilon);
			};

			class storable
			{
			private:
				friend class dictionary;

			private:
				dynamic value;

			public:
				storable() noexcept;
				storable(virtual_machine* engine, void* pointer, int type_id) noexcept;
				~storable() noexcept;
				void set(virtual_machine* engine, void* pointer, int type_id);
				void set(virtual_machine* engine, storable& other);
				bool get(virtual_machine* engine, void* pointer, int type_id) const;
				const void* get_address_of_value() const;
				int get_type_id() const;
				void release_references(asIScriptEngine* engine);
				void enum_references(asIScriptEngine* engine);
			};

			class random
			{
			public:
				static core::string getb(uint64_t size);
				static double betweend(double min, double max);
				static double magd();
				static double getd();
				static float betweenf(float min, float max);
				static float magf();
				static float getf();
				static uint64_t betweeni(uint64_t min, uint64_t max);
			};

			class any : public core::reference<any>
			{
				friend promise;

			private:
				virtual_machine* engine;
				dynamic value;

			public:
				any(virtual_machine* engine) noexcept;
				any(void* ref, int ref_type_id, virtual_machine* engine) noexcept;
				any(const any&) noexcept;
				~any() noexcept;
				any& operator= (const any&) noexcept;
				int copy_from(const any* other);
				void store(void* ref, int ref_type_id);
				bool retrieve(void* ref, int ref_type_id) const;
				void* get_address_of_object();
				int get_type_id() const;
				void enum_references(asIScriptEngine* engine);
				void release_references(asIScriptEngine* engine);

			private:
				void free_object();

			public:
				static any* create();
				static any* create(int type_id, void* ref);
				static any* create(const char* decl, void* ref);
				static any* factory1();
				static void factory2(asIScriptGeneric* genericf);
				static any& assignment(any* base, any* other);

			public:
				template <typename t, t>
				struct ify;

				template <typename t, t>
				struct ify_static;

				template <typename t, typename r, typename ...args, r(t::* f)(args...)>
				struct ify<r(t::*)(args...), f>
				{
					template <type_id type_id>
					static any* id(t* base, args... data)
					{
						r subresult((base->*f)(data...));
						return any::create((int)type_id, &subresult);
					}
					template <uint64_t type_ref>
					static any* decl(t* base, args... data)
					{
						r subresult((base->*f)(data...));
						return any::create(type_cache::get_type_id(type_ref), &subresult);
					}
				};

				template <typename r, typename ...args, r(*f)(args...)>
				struct ify_static<r(*)(args...), f>
				{
					template <type_id type_id>
					static any* id(args... data)
					{
						r subresult((*f)(data...));
						return any::create((int)type_id, &subresult);
					}
					template <uint64_t type_ref>
					static any* decl(args... data)
					{
						r subresult((*f)(data...));
						return any::create(type_cache::get_type_id(type_ref), &subresult);
					}
				};
			};

			class array : public core::reference<array>
			{
			public:
				struct sbuffer
				{
					size_t max_elements;
					size_t num_elements;
					unsigned char data[1];
				};

				struct scache
				{
					asIScriptFunction* comparator;
					asIScriptFunction* equals;
					int comparator_return_code;
					int equals_return_code;
				};

			private:
				static int array_id;

			private:
				type_info obj_type;
				sbuffer* buffer;
				size_t element_size;
				int sub_type_id;

			public:
				array(asITypeInfo* t, void* init_buf) noexcept;
				array(size_t length, asITypeInfo* t) noexcept;
				array(size_t length, void* def_val, asITypeInfo* t) noexcept;
				array(const array& other) noexcept;
				~array() noexcept;
				asITypeInfo* get_array_object_type() const;
				int get_array_type_id() const;
				int get_element_type_id() const;
				size_t size() const;
				size_t capacity() const;
				bool empty() const;
				void reserve(size_t max_elements);
				void resize(size_t num_elements);
				void* front();
				const void* front() const;
				void* back();
				const void* back() const;
				void* at(size_t index);
				const void* at(size_t index) const;
				void set_value(size_t index, void* value);
				array& operator= (const array&) noexcept;
				bool operator== (const array&) const;
				void insert_at(size_t index, void* value);
				void insert_at(size_t index, const array& other);
				void insert_last(void* value);
				void remove_at(size_t index);
				void remove_last();
				void remove_range(size_t start, size_t count);
				void remove_if(void* value, size_t start_at);
				void swap(size_t index1, size_t index2);
				void sort(asIScriptFunction* callback);
				void reverse();
				void clear();
				size_t find(void* value, size_t start_at) const;
				size_t find_by_ref(void* value, size_t start_at) const;
				void* get_buffer();
				void enum_references(asIScriptEngine* engine);
				void release_references(asIScriptEngine* engine);

			private:
				void* get_array_item_pointer(size_t index);
				void* get_data_pointer(void* buffer);
				void copy(void* dst, void* src);
				void precache();
				bool check_max_size(size_t num_elements);
				void resize(int64_t delta, size_t at);
				void create_buffer(sbuffer** buf, size_t num_elements);
				void delete_buffer(sbuffer* buf);
				void copy_buffer(sbuffer* dst, sbuffer* src);
				void create(sbuffer* buf, size_t start, size_t end);
				void destroy(sbuffer* buf, size_t start, size_t end);
				bool less(const void* a, const void* b, immediate_context* ctx, scache* cache);
				bool equals(const void* a, const void* b, immediate_context* ctx, scache* cache) const;
				bool is_eligible_for_find(scache** output) const;
				bool is_eligible_for_sort(scache** output) const;

			public:
				static array* create(asITypeInfo* t);
				static array* create(asITypeInfo* t, size_t length);
				static array* create(asITypeInfo* t, size_t length, void* default_value);
				static array* factory(asITypeInfo* t, void* list_buffer);
				static void cleanup_type_info_cache(asITypeInfo* type);
				static bool template_callback(asITypeInfo* t, bool& dont_garbage_collect);
				static int get_id();

			public:
				template <typename t>
				static array* compose(const type_info& array_type, const core::vector<t>& objects)
				{
					array* array = create(array_type.get_type_info(), objects.size());
					for (size_t i = 0; i < objects.size(); i++)
						array->set_value((size_t)i, (void*)&objects[i]);

					return array;
				}
				template <typename t>
				static typename std::enable_if<std::is_pointer<t>::value, core::vector<t>>::type decompose(array* array)
				{
					core::vector<t> result;
					if (!array)
						return result;

					size_t size = array->size();
					result.reserve(size);

					for (size_t i = 0; i < size; i++)
						result.push_back((t)array->at(i));

					return result;
				}
				template <typename t>
				static typename std::enable_if<!std::is_pointer<t>::value, core::vector<t>>::type decompose(array* array)
				{
					core::vector<t> result;
					if (!array)
						return result;

					size_t size = array->size();
					result.reserve(size);

					for (size_t i = 0; i < size; i++)
						result.push_back(*((t*)array->at(i)));

					return result;
				}

			public:
				template <typename t, t>
				struct ify;

				template <typename t, t>
				struct ify_static;

				template <typename t, typename r, typename ...args, core::vector<r>(t::* f)(args...)>
				struct ify<core::vector<r>(t::*)(args...), f>
				{
					template <type_id type_id>
					static array* id(t* base, args... data)
					{
						virtual_machine* vm = virtual_machine::get();
						VI_ASSERT(vm != nullptr, "manager should be present");

						auto info = vm->get_type_info_by_id((int)type_id);
						VI_ASSERT(info.is_valid(), "type_info should be valid");

						core::vector<r> source((base->*f)(data...));
						return array::compose(info, source);
					}
					template <uint64_t type_ref>
					static array* decl(t* base, args... data)
					{
						virtual_machine* vm = virtual_machine::get();
						VI_ASSERT(vm != nullptr, "manager should be present");

						auto info = vm->get_type_info_by_id(type_cache::get_type_id(type_ref));
						VI_ASSERT(info.is_valid(), "type_info should be valid");

						core::vector<r> source((base->*f)(data...));
						return array::compose(info, source);
					}
				};

				template <typename r, typename ...args, core::vector<r>(*f)(args...)>
				struct ify_static<core::vector<r>(*)(args...), f>
				{
					template <type_id type_id>
					static array* id(args... data)
					{
						virtual_machine* vm = virtual_machine::get();
						VI_ASSERT(vm != nullptr, "manager should be present");

						auto info = vm->get_type_info_by_id((int)type_id);
						VI_ASSERT(info.is_valid(), "type_info should be valid");

						core::vector<r> source((*f)(data...));
						return array::compose(info, source);
					}
					template <uint64_t type_ref>
					static array* decl(args... data)
					{
						virtual_machine* vm = virtual_machine::get();
						VI_ASSERT(vm != nullptr, "manager should be present");

						auto info = vm->get_type_info_by_id(type_cache::get_type_id(type_ref));
						VI_ASSERT(info.is_valid(), "type_info should be valid");

						core::vector<r> source((*f)(data...));
						return array::compose(info, source);
					}
				};
			};

			class dictionary : public core::reference<dictionary>
			{
			public:
				typedef core::unordered_map<core::string, storable> internal_map;

			public:
				class local_iterator
				{
				private:
					friend class dictionary;

				private:
					internal_map::const_iterator it;
					const dictionary& base;

				public:
					void operator++();
					void operator++(int);
					local_iterator& operator*();
					bool operator==(const local_iterator& other) const;
					bool operator!=(const local_iterator& other) const;
					const core::string& get_key() const;
					int get_type_id() const;
					bool get_value(void* value, int type_id) const;
					const void* get_address_of_value() const;

				private:
					local_iterator() noexcept;
					local_iterator(const dictionary& from, internal_map::const_iterator it) noexcept;
					local_iterator& operator= (const local_iterator&) noexcept
					{
						return *this;
					}
				};

				struct scache
				{
					type_info dictionary_type = type_info(nullptr);
					type_info array_type = type_info(nullptr);
					type_info key_type = type_info(nullptr);
				};

			private:
				static int dictionary_id;

			private:
				virtual_machine* engine;
				internal_map data;

			public:
				dictionary(virtual_machine* engine) noexcept;
				dictionary(unsigned char* buffer) noexcept;
				dictionary(const dictionary&) noexcept;
				~dictionary() noexcept;
				dictionary& operator= (const dictionary& other) noexcept;
				void set(const std::string_view& key, void* value, int type_id);
				bool get(const std::string_view& key, void* value, int type_id) const;
				bool get_index(size_t index, core::string* key, void** value, int* type_id) const;
				bool try_get_index(size_t index, core::string* key, void* value, int type_id) const;
				storable* operator[](const std::string_view& key);
				const storable* operator[](const std::string_view& key) const;
				storable* operator[](size_t);
				const storable* operator[](size_t) const;
				int get_type_id(const std::string_view& key) const;
				bool exists(const std::string_view& key) const;
				bool empty() const;
				size_t size() const;
				bool erase(const std::string_view& key);
				void clear();
				array* get_keys() const;
				local_iterator begin() const;
				local_iterator end() const;
				local_iterator find(const std::string_view& key) const;
				void enum_references(asIScriptEngine* engine);
				void release_references(asIScriptEngine* engine);

			public:
				static dictionary* create(virtual_machine* engine);
				static dictionary* create(unsigned char* buffer);
				static void setup(virtual_machine* vm);
				static void factory(asIScriptGeneric* genericf);
				static void list_factory(asIScriptGeneric* genericf);
				static void key_create(void* memory);
				static void key_destroy(storable* base);
				static storable& keyop_assign(storable* base, void* ref, int type_id);
				static storable& keyop_assign(storable* base, const storable& other);
				static storable& keyop_assign(storable* base, double value);
				static storable& keyop_assign(storable* base, as_int64_t value);
				static void keyop_cast(storable* base, void* ref, int type_id);
				static as_int64_t keyop_conv_int(storable* base);
				static double keyop_conv_double(storable* base);

			public:
				template <typename t>
				static dictionary* compose(int type_id, const core::unordered_map<core::string, t>& objects)
				{
					auto* engine = virtual_machine::get();
					dictionary* data = create(engine);
					for (auto& item : objects)
						data->set(item.first, (void*)&item.second, type_id);

					return data;
				}
				template <typename t>
				static typename std::enable_if<std::is_pointer<t>::value, core::unordered_map<core::string, t>>::type decompose(int type_id, dictionary* array)
				{
					core::unordered_map<core::string, t> result;
					result.reserve(array->size());

					int sub_type_id = 0;
					size_t size = array->size();
					for (size_t i = 0; i < size; i++)
					{
						core::string key; void* value = nullptr;
						if (array->get_index(i, &key, &value, &sub_type_id) && sub_type_id == type_id)
							result[key] = (t*)value;
					}

					return result;
				}
				template <typename t>
				static typename std::enable_if<!std::is_pointer<t>::value, core::unordered_map<core::string, t>>::type decompose(int type_id, dictionary* array)
				{
					core::unordered_map<core::string, t> result;
					result.reserve(array->size());

					int sub_type_id = 0;
					size_t size = array->size();
					for (size_t i = 0; i < size; i++)
					{
						core::string key; void* value = nullptr;
						if (array->get_index(i, &key, &value, &sub_type_id) && sub_type_id == type_id)
							result[key] = *(t*)value;
					}

					return result;
				}
			};

			class promise : public core::reference<promise>
			{
			private:
				static int promise_null;
				static int promise_ud;

			private:
				virtual_machine* engine;
				immediate_context* context;
				function_delegate delegatef;
				std::function<void(promise*)> bounce;
				std::mutex update;
				dynamic value;

			public:
				promise(immediate_context* new_context) noexcept;
				~promise() noexcept;
				void enum_references(asIScriptEngine* other_engine);
				void release_references(asIScriptEngine*);
				int get_type_id();
				void* get_address_of_object();
				void when(asIScriptFunction* new_callback);
				void when(std::function<void(promise*)>&& new_callback);
				void store(void* ref_pointer, int ref_type_id);
				void store(void* ref_pointer, const char* type_name);
				void store_exception(const exception::pointer& ref_value);
				void store_void();
				bool retrieve(void* ref_pointer, int ref_type_id);
				void retrieve_void();
				void* retrieve();
				bool is_pending();
				promise* yield_if();

			public:
				static promise* create_factory(void* _Ref, int type_id);
				static promise* create_factory_type(asITypeInfo* type);
				static promise* create_factory_void();
				static bool template_callback(asITypeInfo* info, bool& dont_garbage_collect);
				static expects_vm<void> generator_callback(compute::preprocessor* base, const std::string_view& path, core::string& code);
				static bool is_context_pending(immediate_context* context);
				static bool is_context_busy(immediate_context* context);

			public:
				template <typename t>
				static void dispatch_awaitable(promise* future, int type_id, core::promise<t>&& awaitable)
				{
					future->add_ref();
					future->context->append_stop_execution_callback([future, type_id, awaitable]()
					{
						awaitable.when([future, type_id](t&& result)
						{
							future->store((void*)&result, type_id);
							future->release();
						});
					});
				}
				template <typename t>
				static promise* compose(core::promise<t>&& value, type_id id)
				{
					promise* future = promise::create_factory_void();
					return promise::dispatch_awaitable<t>(future, (int)id, std::move(value));
				}
				template <typename t>
				static promise* compose(core::promise<t>&& value, const char* type_name)
				{
					virtual_machine* engine = virtual_machine::get();
					VI_ASSERT(engine != nullptr, "engine should be set");
					promise* future = promise::create_factory_void();
					int type_id = engine->get_type_id_by_decl(type_name);
					return promise::dispatch_awaitable<t>(type_id, type_id, std::move(value));
				}

			public:
				template <typename t, t>
				struct ify;

				template <typename t, t>
				struct ify_static;

				template <typename t, typename r, typename ...args, core::promise<r>(t::* f)(args...)>
				struct ify<core::promise<r>(t::*)(args...), f>
				{
					template <type_id type_id>
					static promise* id(t* base, args... data)
					{
						promise* future = promise::create_factory_void();
						if (!future)
							return future;

						future->context->enable_deferred_exceptions();
						promise::dispatch_awaitable<r>(future, (int)type_id, ((base->*f)(data...)));
						future->context->disable_deferred_exceptions();
						return future;
					}
					template <uint64_t type_ref>
					static promise* decl(t* base, args... data)
					{
						promise* future = promise::create_factory_void();
						if (!future)
							return future;

						int type_id = type_cache::get_type_id(type_ref);
						future->context->enable_deferred_exceptions();
						promise::dispatch_awaitable<r>(future, type_id, ((base->*f)(data...)));
						future->context->disable_deferred_exceptions();
						return future;
					}
				};

				template <typename r, typename ...args, core::promise<r>(*f)(args...)>
				struct ify_static<core::promise<r>(*)(args...), f>
				{
					template <type_id type_id>
					static promise* id(args... data)
					{
						promise* future = promise::create_factory_void();
						if (!future)
							return future;

						future->context->enable_deferred_exceptions();
						promise::dispatch_awaitable<r>(future, (int)type_id, ((*f)(data...)));
						future->context->disable_deferred_exceptions();
						return future;
					}
					template <uint64_t type_ref>
					static promise* decl(args... data)
					{
						promise* future = promise::create_factory_void();
						if (!future)
							return future;

						int type_id = type_cache::get_type_id(type_ref);
						future->context->enable_deferred_exceptions();
						promise::dispatch_awaitable<r>(future, type_id, ((*f)(data...)));
						future->context->disable_deferred_exceptions();
						return future;
					}
				};
			};
#ifdef VI_BINDINGS
			class complex
			{
			public:
				float r;
				float i;

			public:
				complex() noexcept;
				complex(const complex& other) noexcept;
				complex(float r, float i = 0) noexcept;
				complex& operator= (const complex& other) noexcept;
				complex& operator+= (const complex& other);
				complex& operator-= (const complex& other);
				complex& operator*= (const complex& other);
				complex& operator/= (const complex& other);
				float length() const;
				float squared_length() const;
				complex get_ri() const;
				void set_ri(const complex& in);
				complex get_ir() const;
				void set_ir(const complex& in);
				bool operator== (const complex& other) const;
				bool operator!= (const complex& other) const;
				complex operator+ (const complex& other) const;
				complex operator- (const complex& other) const;
				complex operator* (const complex& other) const;
				complex operator/ (const complex& other) const;

			public:
				static void default_constructor(complex* base);
				static void copy_constructor(complex* base, const complex& other);
				static void conv_constructor(complex* base, float new_r);
				static void init_constructor(complex* base, float new_r, float new_i);
				static void list_constructor(complex* base, float* list);
			};

			class mutex : public core::reference<mutex>
			{
			private:
				static int mutex_ud;

			private:
				std::recursive_mutex base;

			public:
				mutex() noexcept;
				~mutex() = default;
				bool try_lock();
				void lock();
				void unlock();

			public:
				static mutex* factory();
				static bool is_any_locked(immediate_context* context);
			};

			class thread : public core::reference<thread>
			{
			private:
				static int thread_ud;

			private:
				struct
				{
					core::vector<any*> queue;
					std::condition_variable CV;
					std::mutex mutex;
				} pipe[2];

			private:
				std::thread procedure;
				std::recursive_mutex mutex;
				exception::pointer raise;
				virtual_machine* vm;
				event_loop* loop;
				function_delegate function;

			public:
				thread(virtual_machine* engine, asIScriptFunction* function) noexcept;
				~thread() noexcept;
				void enum_references(asIScriptEngine* engine);
				void release_references(asIScriptEngine* engine);
				bool suspend();
				bool resume();
				void push(void* ref, int type_id);
				bool pop(void* ref, int type_id);
				bool pop(void* ref, int type_id, uint64_t timeout);
				bool is_active();
				bool start();
				int join(uint64_t timeout);
				int join();
				core::string get_id() const;

			private:
				void execution_loop();

			public:
				static thread* create(asIScriptFunction* callback);
				static thread* get_thread();
				static core::string get_thread_id();
				static void thread_sleep(uint64_t mills);
				static bool thread_suspend();
			};

			class char_buffer : public core::reference<char_buffer>
			{
			private:
				char* buffer;
				size_t length;

			public:
				char_buffer() noexcept;
				char_buffer(size_t size) noexcept;
				char_buffer(char* pointer) noexcept;
				~char_buffer() noexcept;
				bool allocate(size_t size);
				void deallocate();
				bool set_int8(size_t offset, int8_t value, size_t size);
				bool set_uint8(size_t offset, uint8_t value, size_t size);
				bool store_bytes(size_t offset, const std::string_view& value);
				bool store_int8(size_t offset, int8_t value);
				bool store_uint8(size_t offset, uint8_t value);
				bool store_int16(size_t offset, int16_t value);
				bool store_uint16(size_t offset, uint16_t value);
				bool store_int32(size_t offset, int32_t value);
				bool store_uint32(size_t offset, uint32_t value);
				bool store_int64(size_t offset, int64_t value);
				bool store_uint64(size_t offset, uint64_t value);
				bool store_float(size_t offset, float value);
				bool store_double(size_t offset, double value);
				bool interpret(size_t offset, core::string& value, size_t max_size) const;
				bool load_bytes(size_t offset, core::string& value, size_t size) const;
				bool load_int8(size_t offset, int8_t& value) const;
				bool load_uint8(size_t offset, uint8_t& value) const;
				bool load_int16(size_t offset, int16_t& value) const;
				bool load_uint16(size_t offset, uint16_t& value) const;
				bool load_int32(size_t offset, int32_t& value) const;
				bool load_uint32(size_t offset, uint32_t& value) const;
				bool load_int64(size_t offset, int64_t& value) const;
				bool load_uint64(size_t offset, uint64_t& value) const;
				bool load_float(size_t offset, float& value) const;
				bool load_double(size_t offset, double& value) const;
				void* get_pointer(size_t offset) const;
				bool exists(size_t offset) const;
				bool empty() const;
				size_t size() const;
				core::string to_string(size_t max_size) const;

			private:
				bool store(size_t offset, const char* data, size_t size);
				bool load(size_t offset, char* data, size_t size) const;

			public:
				static char_buffer* create();
				static char_buffer* create(size_t size);
				static char_buffer* create(char* pointer);
			};

			class application final : public layer::application
			{
			public:
				function_delegate on_dispatch;
				function_delegate on_publish;
				function_delegate on_composition;
				function_delegate on_script_hook;
				function_delegate on_initialize;
				function_delegate on_startup;
				function_delegate on_shutdown;

			private:
				size_t processed_events;
				asITypeInfo* initiator_type;
				void* initiator_object;

			public:
				application(desc& i, void* object, int type_id) noexcept;
				virtual ~application() noexcept override;
				void set_on_dispatch(asIScriptFunction* callback);
				void set_on_publish(asIScriptFunction* callback);
				void set_on_composition(asIScriptFunction* callback);
				void set_on_script_hook(asIScriptFunction* callback);
				void set_on_initialize(asIScriptFunction* callback);
				void set_on_startup(asIScriptFunction* callback);
				void set_on_shutdown(asIScriptFunction* callback);
				void dispatch(core::timer* time) override;
				void publish(core::timer* time) override;
				void composition() override;
				void script_hook() override;
				void initialize() override;
				core::promise<void> startup() override;
				core::promise<void> shutdown() override;
				size_t get_processed_events() const;
				bool has_processed_events() const;
				bool retrieve_initiator_object(void* ref_pointer, int ref_type_id) const;
				void* get_initiator_object() const;

			public:
				static bool wants_restart(int exit_code);
			};
#endif
			class registry
			{
			public:
				registry() = default;
				virtual bool bind_addons(virtual_machine* vm) noexcept;
				virtual bool bind_stringifiers(debugger_context* context) noexcept;
				static bool bind_string_factory(virtual_machine* vm) noexcept;
				static bool import_ctypes(virtual_machine* vm) noexcept;
				static bool import_any(virtual_machine* vm) noexcept;
				static bool import_array(virtual_machine* vm) noexcept;
				static bool import_complex(virtual_machine* vm) noexcept;
				static bool import_dictionary(virtual_machine* vm) noexcept;
				static bool import_math(virtual_machine* vm) noexcept;
				static bool import_string(virtual_machine* vm) noexcept;
				static bool import_safe_string(virtual_machine* vm) noexcept;
				static bool import_exception(virtual_machine* vm) noexcept;
				static bool import_mutex(virtual_machine* vm) noexcept;
				static bool import_thread(virtual_machine* vm) noexcept;
				static bool import_buffers(virtual_machine* vm) noexcept;
				static bool import_random(virtual_machine* vm) noexcept;
				static bool import_promise(virtual_machine* vm) noexcept;
				static bool import_decimal(virtual_machine* vm) noexcept;
				static bool import_uint128(virtual_machine* vm) noexcept;
				static bool import_uint256(virtual_machine* vm) noexcept;
				static bool import_variant(virtual_machine* vm) noexcept;
				static bool import_timestamp(virtual_machine* vm) noexcept;
				static bool import_console(virtual_machine* vm) noexcept;
				static bool import_schema(virtual_machine* vm) noexcept;
				static bool import_clock_timer(virtual_machine* vm) noexcept;
				static bool import_file_system(virtual_machine* vm) noexcept;
				static bool import_os(virtual_machine* vm) noexcept;
				static bool import_schedule(virtual_machine* vm) noexcept;
				static bool import_regex(virtual_machine* vm) noexcept;
				static bool import_crypto(virtual_machine* vm) noexcept;
				static bool import_codec(virtual_machine* vm) noexcept;
				static bool import_preprocessor(virtual_machine* vm) noexcept;
				static bool import_network(virtual_machine* vm) noexcept;
				static bool import_http(virtual_machine* vm) noexcept;
				static bool import_smtp(virtual_machine* vm) noexcept;
				static bool import_sqlite(virtual_machine* vm) noexcept;
				static bool import_pq(virtual_machine* vm) noexcept;
				static bool import_mongo(virtual_machine* vm) noexcept;
				static bool import_vm(virtual_machine* vm) noexcept;
				static bool import_layer(virtual_machine* vm) noexcept;
			};
		}
	}
}