#ifndef VI_SCRIPTING_H
#define VI_SCRIPTING_H
#include "compute.h"
#include <type_traits>
#include <random>
#include <queue>

class asIScriptEngine;
class asIScriptContext;
class asIScriptModule;
class asIScriptFunction;
class asIScriptGeneric;
class asIScriptObject;
class asIStringFactory;
class asILockableSharedBool;
class asITypeInfo;
struct asSFuncPtr;
struct asSMessageInfo;

namespace vitex
{
	namespace scripting
	{
		struct library;

		struct function;

		struct function_delegate;

		struct type_info;

		class virtual_machine;

		class immediate_context;

		class dummy_ptr
		{
		};

		enum class debug_type
		{
			suspended,
			attach,
			detach
		};

		enum class log_category
		{
			err = 0,
			warning = 1,
			information = 2
		};

		enum class features
		{
			allow_unsafe_references = 1,
			optimize_bytecode = 2,
			copy_script_sections = 3,
			max_stack_size = 4,
			use_character_literals = 5,
			allow_multiline_strings = 6,
			allow_implicit_handle_types = 7,
			build_without_line_cues = 8,
			init_global_vars_after_build = 9,
			require_enum_scope = 10,
			script_scanner = 11,
			include_jit_instructions = 12,
			string_encoding = 13,
			property_accessor_mode = 14,
			expand_def_array_to_impl = 15,
			auto_garbage_collect = 16,
			disallow_global_vars = 17,
			always_impl_default_construct = 18,
			compiler_warnings = 19,
			disallow_value_assign_for_ref_type = 20,
			alter_syntax_named_args = 21,
			disable_integer_division = 22,
			disallow_empty_list_elements = 23,
			private_prop_as_protected = 24,
			allow_unicode_identifiers = 25,
			heredoc_trim_mode = 26,
			max_nested_calls = 27,
			generic_call_mode = 28,
			init_stack_size = 29,
			init_call_stack_size = 30,
			max_call_stack_size = 31,
			ignore_duplicate_shared_int = 32,
			no_debug_output = 33,
			disable_script_class_gc = 34,
			jit_interface_version = 35,
			always_impl_default_copy = 36,
			always_impl_default_copy_construct = 37,
			member_init_mode = 38,
			bool_conversion_mode = 39,
			foreach_support = 40,
		};

		enum class library_features
		{
			promise_no_callbacks = 0,
			promise_no_constructor = 1,
			os_expose_control = 2,
			ctypes_no_pointer_cast = 3
		};

		enum class modifiers
		{
			none = 0,
			in_ref = 1,
			out_ref = 2,
			in_out_ref = 3,
			constant = 4
		};

		enum class compile_flags
		{
			add_to_module = 1
		};

		enum class function_type
		{
			dummy_function = -1,
			system_function = 0,
			script_function = 1,
			interface_function = 2,
			virtual_function = 3,
			definition_function = 4,
			imported_function = 5,
			delegate_function = 6
		};

		enum class behaviours
		{
			construct,
			list_construct,
			destruct,
			factory,
			list_factory,
			add_ref,
			release,
			get_weak_ref_flag,
			template_callback,
			first_gc,
			get_ref_count = first_gc,
			set_gc_flag,
			get_gc_flag,
			enum_refs,
			release_refs,
			last_gc = release_refs,
			max
		};

		enum class execution
		{
			finished = 0,
			suspended = 1,
			aborted = 2,
			exception = 3,
			prepared = 4,
			uninitialized = 5,
			active = 6,
			error = 7,
			deserialization = 8
		};

		enum class convention
		{
			cdecl_call = 0,
			std_call = 1,
			this_call_as_global = 2,
			this_call = 3,
			cdecl_call_obj_last = 4,
			cdecl_call_obj_first = 5,
			generic_call = 6,
			this_call_obj_last = 7,
			this_call_obj_first = 8
		};

		enum class virtual_error
		{
			success = 0,
			err = -1,
			context_active = -2,
			context_not_finished = -3,
			context_not_prepared = -4,
			invalid_arg = -5,
			no_function = -6,
			not_supported = -7,
			invalid_name = -8,
			name_taken = -9,
			invalid_declaration = -10,
			invalid_object = -11,
			invalid_type = -12,
			already_registered = -13,
			multiple_functions = -14,
			no_module = -15,
			no_global_var = -16,
			invalid_configuration = -17,
			invalid_interface = -18,
			cant_bind_all_functions = -19,
			lower_array_dimension_not_registered = -20,
			wrong_config_group = -21,
			config_group_is_in_use = -22,
			illegal_behaviour_for_type = -23,
			wrong_calling_conv = -24,
			build_in_progress = -25,
			init_global_vars_failed = -26,
			out_of_memory = -27,
			module_is_in_use = -28
		};

		enum class type_id
		{
			void_t = 0,
			bool_t = 1,
			int8_t = 2,
			int16_t = 3,
			int32_t = 4,
			int64_t = 5,
			uint8_t = 6,
			uint16_t = 7,
			uint32_t = 8,
			uint64_t = 9,
			float_t = 10,
			double_t = 11,
			handle_t = 0x40000000,
			const_handle_t = 0x20000000,
			mask_object_t = 0x1C000000,
			app_object_t = 0x04000000,
			script_object_t = 0x08000000,
			pattern_t = 0x10000000,
			mask_seqnbr_t = 0x03FFFFFF
		};

		enum class object_behaviours
		{
			ref = (1 << 0),
			value = (1 << 1),
			gc = (1 << 2),
			pod = (1 << 3),
			nohandle = (1 << 4),
			scoped = (1 << 5),
			pattern = (1 << 6),
			as_handle = (1 << 7),
			app_class = (1 << 8),
			app_class_constructor = (1 << 9),
			app_class_destructor = (1 << 10),
			app_class_assignment = (1 << 11),
			app_class_copy_constructor = (1 << 12),
			app_class_c = (app_class + app_class_constructor),
			app_class_cd = (app_class + app_class_constructor + app_class_destructor),
			app_class_ca = (app_class + app_class_constructor + app_class_assignment),
			app_class_ck = (app_class + app_class_constructor + app_class_copy_constructor),
			app_class_cda = (app_class + app_class_constructor + app_class_destructor + app_class_assignment),
			app_class_cdk = (app_class + app_class_constructor + app_class_destructor + app_class_copy_constructor),
			app_class_cak = (app_class + app_class_constructor + app_class_assignment + app_class_copy_constructor),
			app_class_cdak = (app_class + app_class_constructor + app_class_destructor + app_class_assignment + app_class_copy_constructor),
			app_class_d = (app_class + app_class_destructor),
			app_class_da = (app_class + app_class_destructor + app_class_assignment),
			app_class_dk = (app_class + app_class_destructor + app_class_copy_constructor),
			app_class_dak = (app_class + app_class_destructor + app_class_assignment + app_class_copy_constructor),
			app_class_a = (app_class + app_class_assignment),
			app_class_ak = (app_class + app_class_assignment + app_class_copy_constructor),
			app_class_k = (app_class + app_class_copy_constructor),
			app_primitive = (1 << 13),
			app_float = (1 << 14),
			app_array = (1 << 15),
			app_class_allints = (1 << 16),
			app_class_allfloats = (1 << 17),
			nocount = (1 << 18),
			app_class_align8 = (1 << 19),
			implicit_handle = (1 << 20),
			mask_valid_flags = 0x1FFFFF,
			script_object = (1 << 21),
			shared = (1 << 22),
			noinherit = (1 << 23),
			funcdef = (1 << 24),
			list_pattern = (1 << 25),
			enumerator = (1 << 26),
			template_subtype = (1 << 27),
			type_def = (1 << 28),
			abstraction = (1 << 29),
			app_align16 = (1 << 30)
		};

		enum class operators
		{
			neg_t,
			com_t,
			pre_inc_t,
			pre_dec_t,
			post_inc_t,
			post_dec_t,
			equals_t,
			cmp_t,
			assign_t,
			add_assign_t,
			sub_assign_t,
			mul_assign_t,
			div_assign_t,
			mod_assign_t,
			pow_assign_t,
			and_assign_t,
			or_assign_t,
			xor_assign_t,
			shl_assign_t,
			shr_assign_t,
			ushr_assign_t,
			add_t,
			sub_t,
			mul_t,
			div_t,
			mod_t,
			pow_t,
			and_t,
			or_t,
			xor_t,
			shl_t,
			shr_t,
			ushr_t,
			index_t,
			call_t,
			cast_t,
			impl_cast_t
		};

		enum class position
		{
			left = 0,
			right = 1,
			constant = 2
		};

		enum class garbage_collector
		{
			full_cycle = 1,
			one_step = 2,
			destroy_garbage = 4,
			detect_garbage = 8
		};

		inline object_behaviours operator |(object_behaviours a, object_behaviours b)
		{
			return static_cast<object_behaviours>(static_cast<size_t>(a) | static_cast<size_t>(b));
		}
		inline position operator |(position a, position b)
		{
			return static_cast<position>(static_cast<size_t>(a) | static_cast<size_t>(b));
		}
		inline garbage_collector operator |(garbage_collector a, garbage_collector b)
		{
			return static_cast<garbage_collector>(static_cast<size_t>(a) | static_cast<size_t>(b));
		}

		typedef void(dummy_ptr::* dummy_method_ptr)();
		typedef void(*function_ptr)();
		typedef std::function<void(type_info*, struct function_info*)> property_callback;
		typedef std::function<void(type_info*, struct function*)> method_callback;
		typedef std::function<void(class virtual_machine*)> addon_callback;
		typedef std::function<void(class immediate_context*)> args_callback;

		class virtual_exception final : public core::basic_exception
		{
		private:
			virtual_error error_code;

		public:
			virtual_exception(virtual_error error_code);
			virtual_exception(virtual_error error_code, core::string&& message);
			virtual_exception(core::string&& message);
			const char* type() const noexcept override;
			virtual_error code() const noexcept;
		};

		template <typename t>
		using expects_vm = core::expects<t, virtual_exception>;

		template <typename t>
		using expects_promise_vm = core::promise<expects_vm<t>>;

		class type_cache : public core::singletonish
		{
		private:
			static core::unordered_map<uint64_t, std::pair<core::string, int>>* names;

		public:
			static uint64_t set(uint64_t id, const std::string_view& name);
			static int get_type_id(uint64_t id);
			static void cleanup();
		};

		class parser
		{
		public:
			static expects_vm<void> replace_inline_preconditions(const std::string_view& keyword, core::string& data, const std::function<expects_vm<core::string>(const std::string_view& expression)>& replacer);
			static expects_vm<void> replace_directive_preconditions(const std::string_view& keyword, core::string& data, const std::function<expects_vm<core::string>(const std::string_view& expression)>& replacer);

		private:
			static expects_vm<void> replace_preconditions(bool is_directive, const std::string_view& keyword, core::string& data, const std::function<expects_vm<core::string>(const std::string_view& expression)>& replacer);
		};

		class function_factory
		{
		public:
			static asSFuncPtr* create_function_base(void(*base)(), int type);
			static asSFuncPtr* create_method_base(const void* base, size_t size, int type);
			static asSFuncPtr* create_dummy_base();
			static expects_vm<void> atomic_notify_gc(const std::string_view& type_name, void* object);
			static expects_vm<void> atomic_notify_gc_by_id(int type_id, void* object);
			static void release_functor(asSFuncPtr** ptr);
			static void gc_enum_callback(asIScriptEngine* engine, void* reference);
			static void gc_enum_callback(asIScriptEngine* engine, asIScriptFunction* reference);
			static void gc_enum_callback(asIScriptEngine* engine, function_delegate* reference);

		public:
			template <typename t>
			static expects_vm<t> to_return(int code, t&& value)
			{
				if (code < 0)
					return virtual_exception((virtual_error)code);

				return value;
			}
			static expects_vm<void> to_return(int code)
			{
				if (code < 0)
					return virtual_exception((virtual_error)code);

				return core::expectation::met;
			}
		};

		template <int n>
		struct function_binding
		{
			template <class m>
			static asSFuncPtr* bind(m value)
			{
				return function_factory::create_dummy_base();
			}
		};

		template <>
		struct function_binding<sizeof(dummy_method_ptr)>
		{
			template <class m>
			static asSFuncPtr* bind(m value)
			{
				return function_factory::create_method_base(&value, sizeof(dummy_method_ptr), 3);
			}
		};
#if defined(_MSC_VER) && !defined(__MWERKS__)
		template <>
		struct function_binding<sizeof(dummy_method_ptr) + 1 * sizeof(int)>
		{
			template <class m>
			static asSFuncPtr* bind(m value)
			{
				return function_factory::create_method_base(&value, sizeof(dummy_method_ptr) + sizeof(int), 3);
			}
		};

		template <>
		struct function_binding<sizeof(dummy_method_ptr) + 2 * sizeof(int)>
		{
			template <class m>
			static asSFuncPtr* bind(m value)
			{
				asSFuncPtr* ptr = function_factory::create_method_base(&value, sizeof(dummy_method_ptr) + 2 * sizeof(int), 3);
#if defined(_MSC_VER) && !defined(VI_64)
				* (reinterpret_cast<unsigned long*>(ptr) + 3) = *(reinterpret_cast<unsigned long*>(ptr) + 2);
#endif
				return ptr;
			}
		};

		template <>
		struct function_binding<sizeof(dummy_method_ptr) + 3 * sizeof(int)>
		{
			template <class m>
			static asSFuncPtr* bind(m value)
			{
				return function_factory::create_method_base(&value, sizeof(dummy_method_ptr) + 3 * sizeof(int), 3);
			}
		};

		template <>
		struct function_binding<sizeof(dummy_method_ptr) + 4 * sizeof(int)>
		{
			template <class m>
			static asSFuncPtr* bind(m value)
			{
				return function_factory::create_method_base(&value, sizeof(dummy_method_ptr) + 4 * sizeof(int), 3);
			}
		};
#endif
		class generic_context
		{
		private:
			virtual_machine* vm;
			asIScriptGeneric* genericf;

		public:
			generic_context(asIScriptGeneric* base) noexcept;
			void* get_object_address();
			int get_object_type_id() const;
			size_t get_args_count() const;
			int get_arg_type_id(size_t argument, size_t* flags = 0) const;
			uint8_t get_arg_byte(size_t argument);
			uint16_t get_arg_word(size_t argument);
			size_t get_arg_dword(size_t argument);
			uint64_t get_arg_qword(size_t argument);
			float get_arg_float(size_t argument);
			double get_arg_double(size_t argument);
			void* get_arg_address(size_t argument);
			void* get_arg_object_address(size_t argument);
			void* get_address_of_arg(size_t argument);
			int get_return_type_id(size_t* flags = 0) const;
			expects_vm<void> set_return_byte(uint8_t value);
			expects_vm<void> set_return_word(uint16_t value);
			expects_vm<void> set_return_dword(size_t value);
			expects_vm<void> set_return_qword(uint64_t value);
			expects_vm<void> set_return_float(float value);
			expects_vm<void> set_return_double(double value);
			expects_vm<void> set_return_address(void* address);
			expects_vm<void> set_return_object_address(void* object);
			void* get_address_of_return_location();
			bool is_valid() const;
			asIScriptGeneric* get_generic() const;
			virtual_machine* get_vm() const;

		public:
			template <typename t>
			expects_vm<void> set_return_object(t* object)
			{
				return set_return_object_address((void*)object);
			}
			template <typename t>
			t* get_arg_object(size_t arg)
			{
				return (t*)get_arg_object_address(arg);
			}
		};

		class bridge
		{
		public:
			template <typename t>
			static asSFuncPtr* function_call(t value)
			{
#ifdef VI_64
				void(*address)() = reinterpret_cast<void(*)()>(size_t(value));
#else
				void (*address)() = reinterpret_cast<void (*)()>(value);
#endif
				return function_factory::create_function_base(address, 2);
			}
			template <typename t>
			static asSFuncPtr* function_generic_call(t value)
			{
#ifdef VI_64
				void(*address)() = reinterpret_cast<void(*)()>(size_t(value));
#else
				void(*address)() = reinterpret_cast<void(*)()>(value);
#endif
				return function_factory::create_function_base(address, 1);
			}
			template <typename t, typename r, typename... args>
			static asSFuncPtr* method_call(r(t::* value)(args...))
			{
				return function_binding<sizeof(void (t::*)())>::bind((void (t::*)())(value));
			}
			template <typename t, typename r, typename... args>
			static asSFuncPtr* method_call(r(t::* value)(args...) const)
			{
				return function_binding<sizeof(void (t::*)())>::bind((void (t::*)())(value));
			}
			template <typename t, typename r, typename... args>
			static asSFuncPtr* operator_call(r(t::* value)(args...))
			{
				return function_binding<sizeof(void (t::*)())>::bind(static_cast<r(t::*)(args...)>(value));
			}
			template <typename t, typename r, typename... args>
			static asSFuncPtr* operator_call(r(t::* value)(args...) const)
			{
				return function_binding<sizeof(void (t::*)())>::bind(static_cast<r(t::*)(args...)>(value));
			}
			template <typename t, typename... args>
			static void constructor_call(void* memory, args... data)
			{
				new(memory) t(data...);
			}
			template <typename t>
			static void constructor_list_call(asIScriptGeneric* genericf)
			{
				generic_context args(genericf);
				*reinterpret_cast<t**>(args.get_address_of_return_location()) = new t((uint8_t*)args.get_arg_address(0));
			}
			template <typename t>
			static void destructor_call(void* memory)
			{
				((t*)memory)->~t();
			}
			template <typename t, uint64_t type_name, typename... args>
			static t* managed_constructor_call(args... data)
			{
				auto* result = new t(data...);
				function_factory::atomic_notify_gc_by_id(type_cache::get_type_id(type_name), (void*)result);

				return result;
			}
			template <typename t, uint64_t type_name>
			static void managed_constructor_list_call(asIScriptGeneric* genericf)
			{
				generic_context args(genericf);
				t* result = new t((uint8_t*)args.get_arg_address(0));
				*reinterpret_cast<t**>(args.get_address_of_return_location()) = result;
				function_factory::atomic_notify_gc_by_id(type_cache::get_type_id(type_name), (void*)result);
			}
			template <typename t, typename... args>
			static t* unmanaged_constructor_call(args... data)
			{
				return new t(data...);
			}
			template <typename t>
			static void unmanaged_constructor_list_call(asIScriptGeneric* genericf)
			{
				generic_context args(genericf);
				*reinterpret_cast<t**>(args.get_address_of_return_location()) = new t((uint8_t*)args.get_arg_address(0));
			}
			template <typename t>
			static constexpr size_t type_traits_of()
			{
				if (std::is_floating_point<t>::value)
					return (size_t)object_behaviours::app_float;

				if (std::is_integral<t>::value || std::is_pointer<t>::value || std::is_enum<t>::value)
					return (size_t)object_behaviours::app_primitive;

				if (std::is_class<t>::value)
				{
					return
						(size_t)object_behaviours::app_class |
						(std::is_default_constructible<t>::value && !std::is_trivially_default_constructible<t>::value ? (size_t)object_behaviours::app_class_constructor : 0) |
						(std::is_destructible<t>::value && !std::is_trivially_destructible<t>::value ? (size_t)object_behaviours::app_class_destructor : 0) |
						(std::is_copy_assignable<t>::value && !std::is_trivially_copy_assignable<t>::value ? (size_t)object_behaviours::app_class_assignment : 0) |
						(std::is_copy_constructible<t>::value && !std::is_trivially_copy_constructible<t>::value ? (size_t)object_behaviours::app_class_copy_constructor : 0);
				}

				if (std::is_array<t>::value)
					return (size_t)object_behaviours::app_array;

				return 0;
			}
		};

		struct byte_code_info
		{
			core::vector<uint8_t> data;
			core::string name;
			bool valid = false;
			bool debug = true;
		};

		struct byte_code_label
		{
			std::string_view name;
			size_t offset_of_arg0;
			size_t offset_of_arg1;
			size_t offset_of_arg2;
			int32_t offset_of_stack;
			uint8_t code;
			uint8_t size;
			uint8_t size_of_arg0;
			uint8_t size_of_arg1;
			uint8_t size_of_arg2;
		};

		struct property_info
		{
			std::string_view name;
			std::string_view name_space;
			int type_id;
			bool is_const;
			std::string_view config_group;
			void* pointer;
			size_t access_mask;
		};

		struct function_info
		{
			std::string_view name;
			size_t access_mask;
			int type_id;
			int offset;
			bool is_private;
			bool is_protected;
			bool is_reference;
		};

		struct message_info
		{
		private:
			asSMessageInfo* info;

		public:
			message_info(asSMessageInfo* info) noexcept;
			std::string_view get_section() const;
			std::string_view get_text() const;
			log_category get_type() const;
			int get_row() const;
			int get_column() const;
			asSMessageInfo* get_message_info() const;
			bool is_valid() const;
		};

		struct type_info
		{
		private:
			virtual_machine* vm;
			asITypeInfo* info;

		public:
			type_info(asITypeInfo* type_info) noexcept;
			void for_each_property(const property_callback& callback);
			void for_each_method(const method_callback& callback);
			std::string_view get_group() const;
			size_t get_access_mask() const;
			library get_module() const;
			void add_ref() const;
			void release();
			std::string_view get_name() const;
			std::string_view get_namespace() const;
			type_info get_base_type() const;
			bool derives_from(const type_info& type) const;
			size_t flags() const;
			size_t size() const;
			int get_type_id() const;
			int get_sub_type_id(size_t sub_type_index = 0) const;
			type_info get_sub_type(size_t sub_type_index = 0) const;
			size_t get_sub_type_count() const;
			size_t get_interface_count() const;
			type_info get_interface(size_t index) const;
			bool implements(const type_info& type) const;
			size_t get_factories_count() const;
			function get_factory_by_index(size_t index) const;
			function get_factory_by_decl(const std::string_view& decl) const;
			size_t get_methods_count() const;
			function get_method_by_index(size_t index, bool get_virtual = true) const;
			function get_method_by_name(const std::string_view& name, bool get_virtual = true) const;
			function get_method_by_decl(const std::string_view& decl, bool get_virtual = true) const;
			size_t get_properties_count() const;
			expects_vm<void> get_property(size_t index, function_info* out) const;
			std::string_view get_property_declaration(size_t index, bool include_namespace = false) const;
			size_t get_behaviour_count() const;
			function get_behaviour_by_index(size_t index, behaviours* out_behaviour) const;
			size_t get_child_function_def_count() const;
			type_info get_child_function_def(size_t index) const;
			type_info get_parent_type() const;
			size_t get_enum_value_count() const;
			std::string_view get_enum_value_by_index(size_t index, int* out_value) const;
			function get_function_def_signature() const;
			void* set_user_data(void* data, size_t type = 0);
			void* get_user_data(size_t type = 0) const;
			bool is_handle() const;
			bool is_valid() const;
			asITypeInfo* get_type_info() const;
			virtual_machine* get_vm() const;

		public:
			template <typename t>
			t* get_instance(void* object)
			{
				VI_ASSERT(object != nullptr, "object should be set");
				return is_handle() ? *(t**)object : (t*)object;
			}
			template <typename t>
			t* get_property(void* object, int offset)
			{
				VI_ASSERT(object != nullptr, "object should be set");
				if (!is_handle())
					return reinterpret_cast<t*>(reinterpret_cast<char*>(object) + offset);

				if (!(*(void**)object))
					return nullptr;

				return reinterpret_cast<t*>(reinterpret_cast<char*>(*(void**)object) + offset);
			}

		public:
			template <typename t>
			static t* get_instance(void* object, int type_id)
			{
				VI_ASSERT(object != nullptr, "object should be set");
				return is_handle(type_id) ? *(t**)object : (t*)object;
			}
			template <typename t>
			static t* get_property(void* object, int offset, int type_id)
			{
				VI_ASSERT(object != nullptr, "object should be set");
				if (!is_handle(type_id))
					return reinterpret_cast<t*>(reinterpret_cast<char*>(object) + offset);

				if (!(*(void**)object))
					return nullptr;

				return reinterpret_cast<t*>(reinterpret_cast<char*>(*(void**)object) + offset);
			}

		public:
			static bool is_handle(int type_id);
			static bool is_script_object(int type_id);
		};

		struct function
		{
		private:
			virtual_machine* vm;
			asIScriptFunction* ptr;

		public:
			function(asIScriptFunction* base) noexcept;
			function(const function& base) noexcept;
			void add_ref() const;
			void release();
			int get_id() const;
			function_type get_type() const;
			uint32_t* get_byte_code(size_t* size = nullptr) const;
			std::string_view get_module_name() const;
			library get_module() const;
			std::string_view get_section_name() const;
			std::string_view get_group() const;
			size_t get_access_mask() const;
			type_info get_object_type() const;
			std::string_view get_object_name() const;
			std::string_view get_name() const;
			std::string_view get_namespace() const;
			std::string_view get_decl(bool include_object_name = true, bool include_namespace = false, bool include_arg_names = false) const;
			bool is_read_only() const;
			bool is_private() const;
			bool is_protected() const;
			bool is_final() const;
			bool is_override() const;
			bool is_shared() const;
			bool is_explicit() const;
			bool is_property() const;
			size_t get_args_count() const;
			expects_vm<void> get_arg(size_t index, int* type_id, size_t* flags = nullptr, std::string_view* name = nullptr, std::string_view* default_arg = nullptr) const;
			int get_return_type_id(size_t* flags = nullptr) const;
			int get_type_id() const;
			bool is_compatible_with_type_id(int type_id) const;
			void* get_delegate_object() const;
			type_info get_delegate_object_type() const;
			function get_delegate_function() const;
			size_t get_properties_count() const;
			expects_vm<void> get_property(size_t index, std::string_view* name, int* type_id = nullptr) const;
			std::string_view get_property_decl(size_t index, bool include_namespace = false) const;
			int find_next_line_with_code(int line) const;
			void* set_user_data(void* user_data, size_t type = 0);
			void* get_user_data(size_t type = 0) const;
			bool is_valid() const;
			asIScriptFunction* get_function() const;
			virtual_machine* get_vm() const;
		};

		struct script_object
		{
		private:
			asIScriptObject* object;

		public:
			script_object(asIScriptObject* base) noexcept;
			void add_ref() const;
			void release();
			type_info get_object_type();
			int get_type_id();
			int get_property_type_id(size_t id) const;
			size_t get_properties_count() const;
			std::string_view get_property_name(size_t id) const;
			void* get_address_of_property(size_t id);
			virtual_machine* get_vm() const;
			int copy_from(const script_object& other);
			void* set_user_data(void* data, size_t type = 0);
			void* get_user_data(size_t type = 0) const;
			bool is_valid() const;
			asIScriptObject* get_object() const;
		};

		struct base_class
		{
		protected:
			virtual_machine* vm;
			asITypeInfo* type;
			int type_id;

		public:
			base_class(virtual_machine* engine, asITypeInfo* source, int type) noexcept;
			base_class(const base_class&) = default;
			base_class(base_class&&) = default;
			expects_vm<void> set_function_def(const std::string_view& decl);
			expects_vm<void> set_operator_copy_address(asSFuncPtr* value, convention = convention::this_call);
			expects_vm<void> set_behaviour_address(const std::string_view& decl, behaviours behave, asSFuncPtr* value, convention = convention::this_call);
			expects_vm<void> set_property_address(const std::string_view& decl, int offset);
			expects_vm<void> set_property_static_address(const std::string_view& decl, void* value);
			expects_vm<void> set_operator_address(const std::string_view& decl, asSFuncPtr* value, convention type = convention::this_call);
			expects_vm<void> set_method_address(const std::string_view& decl, asSFuncPtr* value, convention type = convention::this_call);
			expects_vm<void> set_method_static_address(const std::string_view& decl, asSFuncPtr* value, convention type = convention::cdecl_call);
			expects_vm<void> set_constructor_address(const std::string_view& decl, asSFuncPtr* value, convention type = convention::cdecl_call_obj_first);
			expects_vm<void> set_constructor_list_address(const std::string_view& decl, asSFuncPtr* value, convention type = convention::cdecl_call_obj_first);
			expects_vm<void> set_destructor_address(const std::string_view& decl, asSFuncPtr* value);
			asITypeInfo* get_type_info() const;
			int get_type_id() const;
			virtual bool is_valid() const;
			virtual std::string_view get_type_name() const;
			virtual core::string get_name() const;
			virtual_machine* get_vm() const;

		private:
			static core::string get_operator(operators op, const std::string_view& out, const std::string_view& args, bool constant, bool right);

		public:
			expects_vm<void> set_template_callback(bool(*value)(asITypeInfo*, bool&))
			{
				asSFuncPtr* template_callback = bridge::function_call<bool(*)(asITypeInfo*, bool&)>(value);
				auto result = set_behaviour_address("bool f(int&in, bool&out)", behaviours::template_callback, template_callback, convention::cdecl_call);
				function_factory::release_functor(&template_callback);
				return result;
			}
			template <typename t>
			expects_vm<void> set_enum_refs(void(t::* value)(asIScriptEngine*))
			{
				asSFuncPtr* enum_refs = bridge::method_call<t>(value);
				auto result = set_behaviour_address("void f(int &in)", behaviours::enum_refs, enum_refs, convention::this_call);
				function_factory::release_functor(&enum_refs);
				return result;
			}
			template <typename t>
			expects_vm<void> set_release_refs(void(t::* value)(asIScriptEngine*))
			{
				asSFuncPtr* release_refs = bridge::method_call<t>(value);
				auto result = set_behaviour_address("void f(int &in)", behaviours::release_refs, release_refs, convention::this_call);
				function_factory::release_functor(&release_refs);
				return result;
			}
			template <typename t>
			expects_vm<void> set_enum_refs_extern(void(*value)(t*, asIScriptEngine*))
			{
				asSFuncPtr* enum_refs = bridge::function_call<void(*)(t*, asIScriptEngine*)>(value);
				auto result = set_behaviour_address("void f(int &in)", behaviours::enum_refs, enum_refs, convention::cdecl_call_obj_first);
				function_factory::release_functor(&enum_refs);
				return result;
			}
			template <typename t>
			expects_vm<void> set_release_refs_extern(void(*value)(t*, asIScriptEngine*))
			{
				asSFuncPtr* release_refs = bridge::function_call<void(*)(t*, asIScriptEngine*)>(value);
				auto result = set_behaviour_address("void f(int &in)", behaviours::release_refs, release_refs, convention::cdecl_call_obj_first);
				function_factory::release_functor(&release_refs);
				return result;
			}
			template <typename t, typename r>
			expects_vm<void> set_property(const std::string_view& decl, r t::* value)
			{
				return set_property_address(decl, (int)reinterpret_cast<size_t>(&(((t*)0)->*value)));
			}
			template <typename t, typename r>
			expects_vm<void> set_property_array(const std::string_view& decl, r t::* value, size_t elements_count)
			{
				for (size_t i = 0; i < elements_count; i++)
				{
					core::string element_decl = core::string(decl) + core::to_string(i);
					auto result = set_property_address(element_decl.c_str(), (int)reinterpret_cast<size_t>(&(((t*)0)->*value)) + (int)(i * sizeof(r) / elements_count));
					if (!result)
						return result;
				}
				return core::expectation::met;
			}
			template <typename t>
			expects_vm<void> set_property_static(const std::string_view& decl, t* value)
			{
				return set_property_static_address(decl, (void*)value);
			}
			template <typename t, typename r>
			expects_vm<void> set_getter(const std::string_view& type, const std::string_view& name, r(t::* value)())
			{
				asSFuncPtr* ptr = bridge::method_call<t, r>(value);
				auto result = set_method_address(core::stringify::text("%s get_%s()", (int)type.size(), type.data(), (int)name.size(), name.data()).c_str(), ptr, convention::this_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename r>
			expects_vm<void> set_getter_extern(const std::string_view& type, const std::string_view& name, r(*value)(t*))
			{
				asSFuncPtr* ptr = bridge::function_call(value);
				auto result = set_method_address(core::stringify::text("%s get_%s()", (int)type.size(), type.data(), (int)name.size(), name.data()).c_str(), ptr, convention::cdecl_call_obj_first);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename r>
			expects_vm<void> set_setter(const std::string_view& type, const std::string_view& name, void(t::* value)(r))
			{
				asSFuncPtr* ptr = bridge::method_call<t, void, r>(value);
				auto result = set_method_address(core::stringify::text("void set_%s(%s)", (int)name.size(), name.data(), (int)type.size(), type.data()).c_str(), ptr, convention::this_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename r>
			expects_vm<void> set_setter_extern(const std::string_view& type, const std::string_view& name, void(*value)(t*, r))
			{
				asSFuncPtr* ptr = bridge::function_call(value);
				auto result = set_method_address(core::stringify::text("void set_%s(%s)", (int)name.size(), name.data(), (int)type.size(), type.data()).c_str(), ptr, convention::cdecl_call_obj_first);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename r>
			expects_vm<void> set_array_getter(const std::string_view& type, const std::string_view& name, r(t::* value)(uint32_t))
			{
				asSFuncPtr* ptr = bridge::method_call<t, r, uint32_t>(value);
				auto result = set_method_address(core::stringify::text("%s get_%s(uint)", (int)type.size(), type.data(), (int)name.size(), name.data()).c_str(), ptr, convention::this_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename r>
			expects_vm<void> set_array_getter_extern(const std::string_view& type, const std::string_view& name, r(*value)(t*, uint32_t))
			{
				asSFuncPtr* ptr = bridge::function_call(value);
				auto result = set_method_address(core::stringify::text("%s get_%s(uint)", (int)type.size(), type.data(), (int)name.size(), name.data()).c_str(), ptr, convention::cdecl_call_obj_first);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename r>
			expects_vm<void> set_array_setter(const std::string_view& type, const std::string_view& name, void(t::* value)(uint32_t, r))
			{
				asSFuncPtr* ptr = bridge::method_call<t, void, uint32_t, r>(value);
				auto result = set_method_address(core::stringify::text("void set_%s(uint, %s)", (int)name.size(), name.data(), (int)type.size(), type.data()).c_str(), ptr, convention::this_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename r>
			expects_vm<void> set_array_setter_extern(const std::string_view& type, const std::string_view& name, void(*value)(t*, uint32_t, r))
			{
				asSFuncPtr* ptr = bridge::function_call(value);
				auto result = set_method_address(core::stringify::text("void set_%s(uint, %s)", (int)name.size(), name.data(), (int)type.size(), type.data()).c_str(), ptr, convention::cdecl_call_obj_first);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename r, typename... a>
			expects_vm<void> set_operator(operators type, uint32_t opts, const std::string_view& out, const std::string_view& args, r(t::* value)(a...))
			{
				core::string op = get_operator(type, out, args, opts & (uint32_t)position::constant, opts & (uint32_t)position::right);
				VI_ASSERT(!op.empty(), "resulting operator should not be empty");
				asSFuncPtr* ptr = bridge::method_call<t, r, a...>(value);
				auto result = set_operator_address(op.c_str(), ptr, convention::this_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename r, typename... a>
			expects_vm<void> set_operator_extern(operators type, uint32_t opts, const std::string_view& out, const std::string_view& args, r(*value)(a...))
			{
				core::string op = get_operator(type, out, args, opts & (uint32_t)position::constant, opts & (uint32_t)position::right);
				VI_ASSERT(!op.empty(), "resulting operator should not be empty");
				asSFuncPtr* ptr = bridge::function_call(value);
				auto result = set_operator_address(op.c_str(), ptr, convention::cdecl_call_obj_first);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t>
			expects_vm<void> set_operator_copy()
			{
				asSFuncPtr* ptr = bridge::operator_call<t, t&, const t&>(&t::operator =);
				auto result = set_operator_copy_address(ptr, convention::this_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t>
			expects_vm<void> set_operator_move_copy()
			{
				asSFuncPtr* ptr = bridge::operator_call<t, t&, t&&>(&t::operator =);
				auto result = set_operator_copy_address(ptr, convention::this_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename r, typename... args>
			expects_vm<void> set_operator_copy_static(r(*value)(args...), convention type = convention::cdecl_call)
			{
				asSFuncPtr* ptr = (type == convention::generic_call ? bridge::function_generic_call<r(*)(args...)>(value) : bridge::function_call<r(*)(args...)>(value));
				auto result = set_operator_copy_address(ptr, convention::cdecl_call_obj_first);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename r, typename... args>
			expects_vm<void> set_method(const std::string_view& decl, r(t::* value)(args...))
			{
				asSFuncPtr* ptr = bridge::method_call<t, r, args...>(value);
				auto result = set_method_address(decl, ptr, convention::this_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename r, typename... args>
			expects_vm<void> set_method(const std::string_view& decl, r(t::* value)(args...) const)
			{
				asSFuncPtr* ptr = bridge::method_call<t, r, args...>(value);
				auto result = set_method_address(decl, ptr, convention::this_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename r, typename... args>
			expects_vm<void> set_method_extern(const std::string_view& decl, r(*value)(args...))
			{
				asSFuncPtr* ptr = bridge::function_call<r(*)(args...)>(value);
				auto result = set_method_address(decl, ptr, convention::cdecl_call_obj_first);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename r, typename... args>
			expects_vm<void> set_method_static(const std::string_view& decl, r(*value)(args...), convention type = convention::cdecl_call)
			{
				asSFuncPtr* ptr = (type == convention::generic_call ? bridge::function_generic_call<r(*)(args...)>(value) : bridge::function_call<r(*)(args...)>(value));
				auto result = set_method_static_address(decl, ptr, type);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename to>
			expects_vm<void> set_dynamic_cast(const std::string_view& to_decl, bool implicit = false)
			{
				auto type = implicit ? operators::impl_cast_t : operators::cast_t;
				auto result1 = set_operator_extern(type, 0, to_decl, "", &base_class::dynamic_cast_function<t, to>);
				if (!result1)
					return result1;

				auto result2 = set_operator_extern(type, (uint32_t)position::constant, to_decl, "", &base_class::dynamic_cast_function<t, to>);
				return result2;
			}

		private:
			template <typename t, typename to>
			static to* dynamic_cast_function(t* base)
			{
				return dynamic_cast<to*>(base);
			}
		};

		struct ref_base_class : public base_class
		{
		public:
			ref_base_class(virtual_machine* engine, asITypeInfo* source, int type) noexcept : base_class(engine, source, type)
			{
			}
			ref_base_class(const ref_base_class&) = default;
			ref_base_class(ref_base_class&&) = default;

		public:
			template <typename t, typename... args>
			expects_vm<void> set_constructor_extern(const std::string_view& decl, t* (*value)(args...))
			{
				asSFuncPtr* functor = bridge::function_call<t * (*)(args...)>(value);
				auto result = set_behaviour_address(decl, behaviours::factory, functor, convention::cdecl_call);
				function_factory::release_functor(&functor);
				return result;
			}
			expects_vm<void> set_constructor_extern(const std::string_view& decl, void(*value)(asIScriptGeneric*))
			{
				asSFuncPtr* functor = bridge::function_generic_call<void(*)(asIScriptGeneric*)>(value);
				auto result = set_behaviour_address(decl, behaviours::factory, functor, convention::generic_call);
				function_factory::release_functor(&functor);
				return result;
			}
			template <typename t, typename... args>
			expects_vm<void> set_constructor(const std::string_view& decl)
			{
				asSFuncPtr* functor = bridge::function_call(&bridge::unmanaged_constructor_call<t, args...>);
				auto result = set_behaviour_address(decl, behaviours::factory, functor, convention::cdecl_call);
				function_factory::release_functor(&functor);
				return result;
			}
			template <typename t, asIScriptGeneric*>
			expects_vm<void> set_constructor(const std::string_view& decl)
			{
				asSFuncPtr* functor = bridge::function_generic_call(&bridge::unmanaged_constructor_call<t, asIScriptGeneric*>);
				auto result = set_behaviour_address(decl, behaviours::factory, functor, convention::generic_call);
				function_factory::release_functor(&functor);
				return result;
			}
			template <typename t>
			expects_vm<void> set_constructor_list(const std::string_view& decl)
			{
				asSFuncPtr* functor = bridge::function_generic_call(&bridge::unmanaged_constructor_list_call<t>);
				auto result = set_behaviour_address(decl, behaviours::list_factory, functor, convention::generic_call);
				function_factory::release_functor(&functor);
				return result;
			}
			template <typename t>
			expects_vm<void> set_constructor_list_extern(const std::string_view& decl, void(*value)(asIScriptGeneric*))
			{
				asSFuncPtr* functor = bridge::function_generic_call(value);
				auto result = set_behaviour_address(decl, behaviours::list_factory, functor, convention::generic_call);
				function_factory::release_functor(&functor);
				return result;
			}
			template <typename t, uint64_t type_name, typename... args>
			expects_vm<void> set_gc_constructor(const std::string_view& decl)
			{
				asSFuncPtr* functor = bridge::function_call(&bridge::managed_constructor_call<t, type_name, args...>);
				auto result = set_behaviour_address(decl, behaviours::factory, functor, convention::cdecl_call);
				function_factory::release_functor(&functor);
				return result;
			}
			template <typename t, uint64_t type_name, asIScriptGeneric*>
			expects_vm<void> set_gc_constructor(const std::string_view& decl)
			{
				asSFuncPtr* functor = bridge::function_generic_call(&bridge::managed_constructor_call<t, type_name, asIScriptGeneric*>);
				auto result = set_behaviour_address(decl, behaviours::factory, functor, convention::generic_call);
				function_factory::release_functor(&functor);
				return result;
			}
			template <typename t, uint64_t type_name>
			expects_vm<void> set_gc_constructor_list(const std::string_view& decl)
			{
				asSFuncPtr* functor = bridge::function_generic_call(&bridge::managed_constructor_list_call<t, type_name>);
				auto result = set_behaviour_address(decl, behaviours::list_factory, functor, convention::generic_call);
				function_factory::release_functor(&functor);
				return result;
			}
			template <typename t>
			expects_vm<void> set_gc_constructor_list_extern(const std::string_view& decl, void(*value)(asIScriptGeneric*))
			{
				asSFuncPtr* functor = bridge::function_generic_call(value);
				auto result = set_behaviour_address(decl, behaviours::list_factory, functor, convention::generic_call);
				function_factory::release_functor(&functor);
				return result;
			}
			template <typename f>
			expects_vm<void> set_add_ref()
			{
				auto factory_ptr = &ref_base_class::gc_add_ref<f>;
				asSFuncPtr* add_ref = bridge::function_call<decltype(factory_ptr)>(factory_ptr);
				auto result = set_behaviour_address("void f()", behaviours::add_ref, add_ref, convention::cdecl_call_obj_first);
				function_factory::release_functor(&add_ref);
				return result;
			}
			template <typename f>
			expects_vm<void> set_release()
			{
				auto factory_ptr = &ref_base_class::gc_release<f>;
				asSFuncPtr* release = bridge::function_call<decltype(factory_ptr)>(factory_ptr);
				auto result = set_behaviour_address("void f()", behaviours::release, release, convention::cdecl_call_obj_first);
				function_factory::release_functor(&release);
				return result;
			}
			template <typename f>
			expects_vm<void> set_mark_ref()
			{
				auto factory_ptr = &ref_base_class::gc_mark_ref<f>;
				asSFuncPtr* release = bridge::function_call<decltype(factory_ptr)>(factory_ptr);
				auto result = set_behaviour_address("void f()", behaviours::set_gc_flag, release, convention::cdecl_call_obj_first);
				function_factory::release_functor(&release);
				return result;
			}
			template <typename f>
			expects_vm<void> set_is_marked_ref()
			{
				auto factory_ptr = &ref_base_class::gc_is_marked_ref<f>;
				asSFuncPtr* release = bridge::function_call<decltype(factory_ptr)>(factory_ptr);
				auto result = set_behaviour_address("bool f()", behaviours::get_gc_flag, release, convention::cdecl_call_obj_first);
				function_factory::release_functor(&release);
				return result;
			}
			template <typename f>
			expects_vm<void> set_ref_count()
			{
				auto factory_ptr = &ref_base_class::gc_get_ref_count<f>;
				asSFuncPtr* release = bridge::function_call<decltype(factory_ptr)>(factory_ptr);
				auto result = set_behaviour_address("int f()", behaviours::get_ref_count, release, convention::cdecl_call_obj_first);
				function_factory::release_functor(&release);
				return result;
			}

		private:
			template <typename u>
			static void gc_add_ref(u* base)
			{
				base->add_ref();
			}
			template <typename u>
			static void gc_release(u* base)
			{
				base->release();
			}
			template <typename u>
			static void gc_mark_ref(u* base)
			{
				base->mark_ref();
			}
			template <typename u>
			static bool gc_is_marked_ref(u* base)
			{
				return base->is_marked_ref();
			}
			template <typename u>
			static int gc_get_ref_count(u* base)
			{
				return (int)base->get_ref_count();
			}
		};

		struct ref_class final : public ref_base_class
		{
		public:
			ref_class(virtual_machine* engine, asITypeInfo* source, int type) noexcept : ref_base_class(engine, source, type)
			{
			}
		};

		struct template_class final : public ref_base_class
		{
		private:
			core::string name;

		public:
			template_class(virtual_machine* engine, const std::string_view& new_name) noexcept : ref_base_class(engine, nullptr, 0), name(core::string(new_name))
			{
			}
			template_class(const template_class&) = default;
			template_class(template_class&&) = default;
			bool is_valid() const override
			{
				return vm && !name.empty();
			}
			std::string_view get_type_name() const override
			{
				return name;
			}
			core::string get_name() const override
			{
				return name;
			}
		};

		struct type_class final : public base_class
		{
		public:
			type_class(virtual_machine* engine, asITypeInfo* source, int type) noexcept : base_class(engine, source, type)
			{
			}
			type_class(const type_class&) = default;
			type_class(type_class&&) = default;

		public:
			template <typename t, typename... args>
			expects_vm<void> set_constructor_extern(const std::string_view& decl, void(*value)(t, args...))
			{
				asSFuncPtr* functor = bridge::function_call<void(*)(t, args...)>(value);
				auto result = set_behaviour_address(decl, behaviours::construct, functor, convention::cdecl_call_obj_first);
				function_factory::release_functor(&functor);
				return result;
			}
			template <typename t, typename... args>
			expects_vm<void> set_constructor(const std::string_view& decl)
			{
				asSFuncPtr* ptr = bridge::function_call(&bridge::constructor_call<t, args...>);
				auto result = set_constructor_address(decl, ptr, convention::cdecl_call_obj_first);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, asIScriptGeneric*>
			expects_vm<void> set_constructor(const std::string_view& decl)
			{
				asSFuncPtr* ptr = bridge::function_generic_call(&bridge::constructor_call<t, asIScriptGeneric*>);
				auto result = set_constructor_address(decl, ptr, convention::generic_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t>
			expects_vm<void> set_constructor_list(const std::string_view& decl)
			{
				asSFuncPtr* ptr = bridge::function_generic_call(&bridge::constructor_list_call<t>);
				auto result = set_constructor_list_address(decl, ptr, convention::generic_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t>
			expects_vm<void> set_constructor_list_extern(const std::string_view& decl, void(*value)(asIScriptGeneric*))
			{
				asSFuncPtr* ptr = bridge::function_generic_call(value);
				auto result = set_constructor_list_address(decl, ptr, convention::generic_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t>
			expects_vm<void> set_destructor(const std::string_view& decl)
			{
				asSFuncPtr* ptr = bridge::function_call(&bridge::destructor_call<t>);
				auto result = set_destructor_address(decl, ptr);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t, typename... args>
			expects_vm<void> set_destructor_extern(const std::string_view& decl, void(*value)(t, args...))
			{
				asSFuncPtr* ptr = bridge::function_call<void(*)(t, args...)>(value);
				auto result = set_destructor_address(decl, ptr);
				function_factory::release_functor(&ptr);
				return result;
			}
		};

		struct type_interface
		{
		private:
			virtual_machine* vm;
			asITypeInfo* type;
			int type_id;

		public:
			type_interface(virtual_machine* engine, asITypeInfo* source, int type) noexcept;
			type_interface(const type_interface&) = default;
			type_interface(type_interface&&) = default;
			expects_vm<void> set_method(const std::string_view& decl);
			asITypeInfo* get_type_info() const;
			int get_type_id() const;
			bool is_valid() const;
			std::string_view get_type_name() const;
			core::string get_name() const;
			virtual_machine* get_vm() const;
		};

		struct enumeration
		{
		private:
			virtual_machine* vm;
			asITypeInfo* type;
			int type_id;

		public:
			enumeration(virtual_machine* engine, asITypeInfo* source, int type) noexcept;
			enumeration(const enumeration&) = default;
			enumeration(enumeration&&) = default;
			expects_vm<void> set_value(const std::string_view& name, int value);
			asITypeInfo* get_type_info() const;
			int get_type_id() const;
			bool is_valid() const;
			std::string_view get_type_name() const;
			core::string get_name() const;
			virtual_machine* get_vm() const;
		};

		struct library
		{
		private:
			virtual_machine* vm;
			asIScriptModule* mod;

		public:
			library(asIScriptModule* type) noexcept;
			void set_name(const std::string_view& name);
			expects_vm<void> add_section(const std::string_view& name, const std::string_view& code, int line_offset = 0);
			expects_vm<void> remove_function(const function& function);
			expects_vm<void> reset_properties(asIScriptContext* context = nullptr);
			expects_vm<void> build();
			expects_vm<void> load_byte_code(byte_code_info* info);
			expects_vm<void> bind_imported_function(size_t import_index, const function& function);
			expects_vm<void> unbind_imported_function(size_t import_index);
			expects_vm<void> bind_all_imported_functions();
			expects_vm<void> unbind_all_imported_functions();
			expects_vm<void> compile_function(const std::string_view& section_name, const std::string_view& code, int line_offset, size_t compile_flags, function* out_function);
			expects_vm<void> compile_property(const std::string_view& section_name, const std::string_view& code, int line_offset);
			expects_vm<void> set_default_namespace(const std::string_view& name_space);
			expects_vm<void> remove_property(size_t index);
			void discard();
			void* get_address_of_property(size_t index);
			size_t set_access_mask(size_t access_mask);
			size_t get_function_count() const;
			function get_function_by_index(size_t index) const;
			function get_function_by_decl(const std::string_view& decl) const;
			function get_function_by_name(const std::string_view& name) const;
			int get_type_id_by_decl(const std::string_view& decl) const;
			expects_vm<size_t> get_imported_function_index_by_decl(const std::string_view& decl) const;
			expects_vm<void> save_byte_code(byte_code_info* info) const;
			expects_vm<size_t> get_property_index_by_name(const std::string_view& name) const;
			expects_vm<size_t> get_property_index_by_decl(const std::string_view& decl) const;
			expects_vm<void> get_property(size_t index, property_info* out) const;
			size_t get_access_mask() const;
			size_t get_objects_count() const;
			size_t get_properties_count() const;
			size_t get_enums_count() const;
			size_t get_imported_function_count() const;
			type_info get_object_by_index(size_t index) const;
			type_info get_type_info_by_name(const std::string_view& name) const;
			type_info get_type_info_by_decl(const std::string_view& decl) const;
			type_info get_enum_by_index(size_t index) const;
			std::string_view get_property_decl(size_t index, bool include_namespace = false) const;
			std::string_view get_default_namespace() const;
			std::string_view get_imported_function_decl(size_t import_index) const;
			std::string_view get_imported_function_module(size_t import_index) const;
			std::string_view get_name() const;
			bool is_valid() const;
			asIScriptModule* get_module() const;
			virtual_machine* get_vm() const;

		public:
			template <typename t>
			expects_vm<void> set_type_property(const std::string_view& name, t* value)
			{
				auto index = get_property_index_by_name(name);
				if (!index)
					return index.error();

				t** address = (t**)get_address_of_property(*index);
				if (!address)
					return virtual_exception(virtual_error::invalid_object);

				*address = value;
				return core::expectation::met;
			}
			template <typename t>
			expects_vm<void> set_type_property(const std::string_view& name, const t& value)
			{
				auto index = get_property_index_by_name(name);
				if (!index)
					return index.error();

				t* address = (t*)get_address_of_property(*index);
				if (!address)
					return virtual_exception(virtual_error::invalid_object);

				*address = value;
				return core::expectation::met;
			}
			template <typename t>
			expects_vm<void> set_ref_property(const std::string_view& name, t* value)
			{
				auto index = get_property_index_by_name(name);
				if (!index)
					return index.error();

				t** address = (t**)get_address_of_property(*index);
				if (!address)
					return virtual_exception(virtual_error::invalid_object);

				core::memory::release(*address);
				*address = (t*)value;
				if (*address != nullptr)
					(*address)->add_ref();
				return core::expectation::met;
			}
		};

		struct function_delegate
		{
			immediate_context* context;
			asIScriptFunction* callback;
			asITypeInfo* delegate_type;
			void* delegate_object;

			function_delegate();
			function_delegate(const function& function);
			function_delegate(const function& function, immediate_context* wanted_context);
			function_delegate(const function_delegate& other);
			function_delegate(function_delegate&& other);
			~function_delegate();
			function_delegate& operator= (const function_delegate& other);
			function_delegate& operator= (function_delegate&& other);
			expects_promise_vm<execution> operator()(args_callback&& on_args, args_callback&& on_return = nullptr);
			function callable() const;
			bool is_valid() const;
			void add_ref();
			void release();

		private:
			void add_ref_and_initialize(bool is_first_time);
		};

		class compiler final : public core::reference<compiler>
		{
		private:
			compute::proc_include_callback include;
			compute::proc_pragma_callback pragma;
			compute::preprocessor* processor;
			asIScriptModule* scope;
			virtual_machine* vm;
			byte_code_info vcache;
			size_t counter;
			bool built;

		public:
			compiler(virtual_machine* engine) noexcept;
			~compiler() noexcept;
			void set_include_callback(compute::proc_include_callback&& callback);
			void set_pragma_callback(compute::proc_pragma_callback&& callback);
			void define(const std::string_view& word);
			void undefine(const std::string_view& word);
			library unlink_module();
			bool clear();
			bool is_defined(const std::string_view& word) const;
			bool is_built() const;
			bool is_cached() const;
			expects_vm<void> prepare(byte_code_info* info);
			expects_vm<void> prepare(const library& info);
			expects_vm<void> prepare(const std::string_view& module_name, bool scoped = false);
			expects_vm<void> prepare(const std::string_view& module_name, const std::string_view& cache, bool debug = true, bool scoped = false);
			expects_vm<void> save_byte_code(byte_code_info* info);
			expects_vm<void> load_file(const std::string_view& path);
			expects_vm<void> load_code(const std::string_view& name, const std::string_view& buffer);
			expects_vm<void> load_byte_code_sync(byte_code_info* info);
			expects_vm<void> compile_sync();
			expects_promise_vm<void> load_byte_code(byte_code_info* info);
			expects_promise_vm<void> compile();
			expects_promise_vm<void> compile_file(const std::string_view& name, const std::string_view& module_name, const std::string_view& entry_name);
			expects_promise_vm<void> compile_memory(const std::string_view& buffer, const std::string_view& module_name, const std::string_view& entry_name);
			expects_promise_vm<function> compile_function(const std::string_view& code, const std::string_view& returns = "", const std::string_view& args = "", core::option<size_t>&& function_id = core::optional::none);
			library get_module() const;
			virtual_machine* get_vm() const;
			compute::preprocessor* get_processor() const;
		};

		class debugger_context final : public core::reference<debugger_context>
		{
		public:
			typedef std::function<core::string(core::string& indent, int depth, void* object)> to_string_callback;
			typedef std::function<core::string(core::string& indent, int depth, void* object, int type_id)> to_string_type_callback;
			typedef std::function<bool(immediate_context*, const core::vector<core::string>&)> command_callback;
			typedef std::function<void(const std::string_view&)> output_callback;
			typedef std::function<bool(core::string&)> input_callback;
			typedef std::function<void(bool)> interrupt_callback;
			typedef std::function<void()> exit_callback;

		public:
			enum class debug_action
			{
				trigger,
				interrupt,
				match,
				next,
				step_into,
				step_over,
				step_out
			};

		protected:
			enum class args_type
			{
				no_args,
				expression,
				array
			};

			struct break_point
			{
				core::string name;
				bool needs_adjusting;
				bool function;
				int line;

				break_point(const std::string_view& n, int l, bool f) noexcept : name(core::string(n)), needs_adjusting(true), function(f), line(l)
				{
				}
			};

			struct command_data
			{
				command_callback callback;
				core::string description;
				args_type arguments;
			};

			struct thread_data
			{
				immediate_context* context;
				std::thread::id id;
			};

		protected:
			core::unordered_map<const asITypeInfo*, to_string_callback> fast_to_string_callbacks;
			core::unordered_map<core::string, to_string_type_callback> slow_to_string_callbacks;
			core::unordered_map<core::string, command_data> commands;
			core::unordered_map<core::string, core::string> descriptions;
			core::vector<thread_data> threads;
			core::vector<break_point> break_points;
			std::recursive_mutex thread_barrier;
			std::recursive_mutex mutex;
			immediate_context* last_context;
			uint32_t last_command_at_stack_level;
			std::atomic<int32_t> force_switch_threads;
			asIScriptFunction* last_function;
			virtual_machine* vm;
			interrupt_callback on_interrupt;
			output_callback on_output;
			input_callback on_input;
			exit_callback on_exit;
			debug_action action;
			bool input_error;
			bool attachable;

		public:
			debugger_context(debug_type type = debug_type::suspended) noexcept;
			~debugger_context() noexcept;
			expects_vm<void> execute_expression(immediate_context* context, const std::string_view& code, const std::string_view& args, args_callback&& on_args);
			void set_interrupt_callback(interrupt_callback&& callback);
			void set_output_callback(output_callback&& callback);
			void set_input_callback(input_callback&& callback);
			void set_exit_callback(exit_callback&& callback);
			void add_to_string_callback(const type_info& type, to_string_callback&& callback);
			void add_to_string_callback(const std::string_view& type, to_string_type_callback&& callback);
			void throw_internal_exception(const std::string_view& message);
			void allow_input_after_failure();
			void input(immediate_context* context);
			void output(const std::string_view& data);
			void line_callback(asIScriptContext* context);
			void exception_callback(asIScriptContext* context);
			void trigger(immediate_context* context, uint64_t timeout_ms = 0);
			void add_file_break_point(const std::string_view& file, int line_number);
			void add_func_break_point(const std::string_view& function);
			void show_exception(immediate_context* context);
			void list_break_points();
			void list_threads();
			void list_addons();
			void list_stack_registers(immediate_context* context, uint32_t level);
			void list_local_variables(immediate_context* context);
			void list_global_variables(immediate_context* context);
			void list_member_properties(immediate_context* context);
			void list_source_code(immediate_context* context);
			void list_definitions(immediate_context* context);
			void list_statistics(immediate_context* context);
			void print_callstack(immediate_context* context);
			void print_value(const std::string_view& expression, immediate_context* context);
			void print_byte_code(const std::string_view& function_decl, immediate_context* context);
			void set_engine(virtual_machine* engine);
			bool interpret_command(const std::string_view& command, immediate_context* context);
			bool check_break_point(immediate_context* context);
			bool interrupt();
			bool is_error();
			bool is_attached();
			debug_action get_state();
			core::string to_string(int max_depth, void* value, uint32_t type_id);
			core::string to_string(core::string& indent, int max_depth, void* value, uint32_t type_id);
			virtual_machine* get_engine();

		public:
			static size_t byte_code_label_to_text(core::string_stream& stream, virtual_machine* vm, void* program, size_t program_pointer, bool selection, bool uppercase);

		private:
			void add_command(const std::string_view& name, const std::string_view& description, args_type type, command_callback&& callback);
			void add_default_commands();
			void clear_thread(immediate_context* context);
			thread_data get_thread(immediate_context* context);
		};

		class immediate_context final : public core::reference<immediate_context>
		{
			friend function_delegate;
			friend virtual_machine;

		public:
			typedef std::function<void()> stop_execution_callback;

		private:
			static int context_ud;

		private:
			struct events
			{
				core::vector<stop_execution_callback> stop_executions;
				std::function<void(immediate_context*)> notification_resolver;
				std::function<void(immediate_context*, function_delegate&&, args_callback&&, args_callback&&)> callback_resolver;
				std::function<void(immediate_context*)> exception;
				std::function<void(immediate_context*)> line;
			} callbacks;

			struct frame
			{
				struct
				{
					core::string info;
					bool allow_catch;
				} deferred_exception;

				expects_promise_vm<execution> future = expects_promise_vm<execution>::null();
				core::string stacktrace;
				size_t deny_suspends = 0;
				size_t deferred_exceptions = 0;
			} executor;

		private:
			core::linked_list<core::string> strings;
			asIScriptContext* context;
			virtual_machine* vm;
			std::recursive_mutex exchange;

		public:
			~immediate_context() noexcept;
			expects_promise_vm<execution> execute_call(const function& function, args_callback&& on_args);
			expects_vm<execution> execute_inline_call(const function& function, args_callback&& on_args);
			expects_vm<execution> execute_subcall(const function& function, args_callback&& on_args, args_callback&& on_return = nullptr);
			expects_vm<execution> execute_next();
			expects_vm<execution> resume();
			expects_promise_vm<execution> resolve_callback(function_delegate&& delegatef, args_callback&& on_args, args_callback&& on_return);
			expects_vm<execution> resolve_notification();
			expects_vm<void> prepare(const function& function);
			expects_vm<void> unprepare();
			expects_vm<void> abort();
			expects_vm<void> suspend();
			expects_vm<void> push_state();
			expects_vm<void> pop_state();
			expects_vm<void> set_object(void* object);
			expects_vm<void> set_arg8(size_t arg, uint8_t value);
			expects_vm<void> set_arg16(size_t arg, uint16_t value);
			expects_vm<void> set_arg32(size_t arg, int value);
			expects_vm<void> set_arg64(size_t arg, int64_t value);
			expects_vm<void> set_arg_float(size_t arg, float value);
			expects_vm<void> set_arg_double(size_t arg, double value);
			expects_vm<void> set_arg_address(size_t arg, void* address);
			expects_vm<void> set_arg_object(size_t arg, void* object);
			expects_vm<void> set_arg_any(size_t arg, void* ptr, int type_id);
			expects_vm<void> get_returnable_by_type(void* defer, asITypeInfo* return_type_id);
			expects_vm<void> get_returnable_by_decl(void* defer, const std::string_view& return_type_decl);
			expects_vm<void> get_returnable_by_id(void* defer, int return_type_id);
			expects_vm<void> set_exception(const std::string_view& info, bool allow_catch = true);
			expects_vm<void> set_exception_callback(void(*callback)(asIScriptContext* context, void* object), void* object);
			expects_vm<void> set_line_callback(void(*callback)(asIScriptContext* context, void* object), void* object);
			expects_vm<void> start_deserialization();
			expects_vm<void> finish_deserialization();
			expects_vm<void> push_function(const function& func, void* object);
			expects_vm<void> get_state_registers(size_t stack_level, function* calling_system_function, function* initial_function, uint32_t* orig_stack_pointer, uint32_t* arguments_size, uint64_t* value_register, void** object_register, type_info* object_type_register);
			expects_vm<void> get_call_state_registers(size_t stack_level, uint32_t* stack_frame_pointer, function* current_function, uint32_t* program_pointer, uint32_t* stack_pointer, uint32_t* stack_index);
			expects_vm<void> set_state_registers(size_t stack_level, function calling_system_function, const function& initial_function, uint32_t orig_stack_pointer, uint32_t arguments_size, uint64_t value_register, void* object_register, const type_info& object_type_register);
			expects_vm<void> set_call_state_registers(size_t stack_level, uint32_t stack_frame_pointer, const function& current_function, uint32_t program_pointer, uint32_t stack_pointer, uint32_t stack_index);
			expects_vm<size_t> get_args_on_stack_count(size_t stack_level);
			expects_vm<void> get_arg_on_stack(size_t stack_level, size_t argument, int* type_id, size_t* flags, void** address);
			expects_vm<size_t> get_properties_count(size_t stack_level = 0);
			expects_vm<void> get_property(size_t index, size_t stack_level, std::string_view* name, int* type_id = 0, modifiers* type_modifiers = 0, bool* is_var_on_heap = 0, int* stack_offset = 0);
			function get_function(size_t stack_level = 0);
			void set_notification_resolver_callback(std::function<void(immediate_context*)>&& callback);
			void set_callback_resolver_callback(std::function<void(immediate_context*, function_delegate&&, args_callback&&, args_callback&&)>&& callback);
			void set_line_callback(std::function<void(immediate_context*)>&& callback);
			void set_exception_callback(std::function<void(immediate_context*)>&& callback);
			void append_stop_execution_callback(stop_execution_callback&& callback);
			core::string& copy_string(core::string& value);
			void invalidate_string(const std::string_view& value);
			void invalidate_strings();
			void reset();
			void disable_suspends();
			void enable_suspends();
			void enable_deferred_exceptions();
			void disable_deferred_exceptions();
			bool is_nested(size_t* nest_count = 0) const;
			bool is_thrown() const;
			bool is_pending();
			execution get_state() const;
			void* get_address_of_arg(size_t arg);
			uint8_t get_return_byte();
			uint16_t get_return_word();
			size_t get_return_dword();
			uint64_t get_return_qword();
			float get_return_float();
			double get_return_double();
			void* get_return_address();
			void* get_return_object_address();
			void* get_address_of_return_value();
			int get_exception_line_number(int* column = 0, std::string_view* section_name = 0);
			int get_line_number(size_t stack_level = 0, int* column = 0, std::string_view* section_name = 0);
			function get_exception_function();
			std::string_view get_exception_string();
			bool will_exception_be_caught();
			bool has_deferred_exception();
			bool rethrow_deferred_exception();
			void clear_exception_callback();
			void clear_line_callback();
			size_t get_callstack_size() const;
			core::option<core::string> get_exception_stack_trace();
			std::string_view get_property_name(size_t index, size_t stack_level = 0);
			std::string_view get_property_decl(size_t index, size_t stack_level = 0, bool include_namespace = false);
			int get_property_type_id(size_t index, size_t stack_level = 0);
			void* get_address_of_property(size_t index, size_t stack_level = 0, bool dont_dereference = false, bool return_address_of_unitialized_objects = false);
			bool is_property_in_scope(size_t index, size_t stack_level = 0);
			int get_this_type_id(size_t stack_level = 0);
			void* get_this_pointer(size_t stack_level = 0);
			function get_system_function();
			bool is_suspended() const;
			bool is_suspendable() const;
			bool is_sync_locked() const;
			bool can_execute_call() const;
			bool can_execute_subcall() const;
			void* set_user_data(void* data, size_t type = 0);
			void* get_user_data(size_t type = 0) const;
			asIScriptContext* get_context();
			virtual_machine* get_vm();

		private:
			immediate_context(asIScriptContext* base) noexcept;

		public:
			template <typename t>
			t* get_return_object()
			{
				return (t*)get_return_object_address();
			}

		public:
			static immediate_context* get(asIScriptContext* context);
			static immediate_context* get();
		};

		class virtual_machine final : public core::reference<virtual_machine>
		{
		public:
			typedef std::function<expects_vm<void>(compute::preprocessor* base, const std::string_view& path, core::string& buffer)> generator_callback;
			typedef std::function<void(const std::string_view&)> compile_callback;
			typedef std::function<void()> when_error_callback;

		public:
			struct cfunction
			{
				core::string declaration;
				void* handle;
			};

			struct clibrary
			{
				core::unordered_map<core::string, cfunction> functions;
				void* handle;
				bool is_addon;
			};

			struct addon
			{
				core::vector<core::string> dependencies;
				addon_callback callback;
				bool exposed = false;
			};

		private:
			struct
			{
				std::recursive_mutex general;
				std::recursive_mutex pool;
			} sync;

		private:
			static int manager_ud;

		private:
			core::unordered_map<library_features, size_t> library_settings;
			core::unordered_map<core::string, core::string> files;
			core::unordered_map<core::string, core::string> sections;
			core::unordered_map<core::string, core::schema*> datas;
			core::unordered_map<core::string, byte_code_info> opcodes;
			core::unordered_map<core::string, clibrary> clibraries;
			core::unordered_map<core::string, addon> addons;
			core::unordered_map<core::string, compile_callback> callbacks;
			core::unordered_map<core::string, generator_callback> generators;
			core::vector<immediate_context*> threads;
			core::vector<asIScriptContext*> stacks;
			core::string default_namespace;
			compute::preprocessor::desc proc;
			compute::include_desc include;
			std::function<void(immediate_context*)> global_exception;
			when_error_callback when_error;
			std::atomic<int64_t> last_major_gc;
			uint64_t scope;
			debugger_context* debugger;
			asIScriptEngine* engine;
			bool save_sources;
			bool save_cache;

		public:
			virtual_machine() noexcept;
			~virtual_machine() noexcept;
			expects_vm<void> write_message(const std::string_view& section, int row, int column, log_category type, const std::string_view& message);
			expects_vm<void> garbage_collect(garbage_collector flags, size_t num_iterations = 1);
			expects_vm<void> perform_periodic_garbage_collection(uint64_t interval_ms);
			expects_vm<void> perform_full_garbage_collection();
			expects_vm<void> notify_of_new_object(void* object, const type_info& type);
			expects_vm<void> get_object_address(size_t index, size_t* sequence_number = nullptr, void** object = nullptr, type_info* type = nullptr);
			expects_vm<void> assign_object(void* dest_object, void* src_object, const type_info& type);
			expects_vm<void> ref_cast_object(void* object, const type_info& from_type, const type_info& to_type, void** new_ptr, bool use_only_implicit_cast = false);
			expects_vm<void> begin_group(const std::string_view& group_name);
			expects_vm<void> end_group();
			expects_vm<void> remove_group(const std::string_view& group_name);
			expects_vm<void> begin_namespace(const std::string_view& name_space);
			expects_vm<void> begin_namespace_isolated(const std::string_view& name_space, size_t default_mask);
			expects_vm<void> end_namespace();
			expects_vm<void> end_namespace_isolated();
			expects_vm<void> add_script_section(asIScriptModule* library, const std::string_view& name, const std::string_view& code, int line_offset = 0);
			expects_vm<void> get_type_name_scope(std::string_view* type_name, std::string_view* name_space) const;
			expects_vm<void> set_function_def(const std::string_view& decl);
			expects_vm<void> set_type_def(const std::string_view& type, const std::string_view& decl);
			expects_vm<void> set_function_address(const std::string_view& decl, asSFuncPtr* value, convention type = convention::cdecl_call);
			expects_vm<void> set_property_address(const std::string_view& decl, void* value);
			expects_vm<void> set_string_factory_address(const std::string_view& type, asIStringFactory* factory);
			expects_vm<void> set_log_callback(void(*callback)(const asSMessageInfo* message, void* object), void* object);
			expects_vm<void> log(const std::string_view& section, int row, int column, log_category type, const std::string_view& message);
			expects_vm<void> set_property(features property, size_t value);
			expects_vm<void> get_property_by_index(size_t index, property_info* info) const;
			expects_vm<size_t> get_property_index_by_name(const std::string_view& name) const;
			expects_vm<size_t> get_property_index_by_decl(const std::string_view& decl) const;
			expects_vm<size_t> get_size_of_primitive_type(int type_id) const;
			expects_vm<type_class> set_struct_address(const std::string_view& name, size_t size, uint64_t flags = (uint64_t)object_behaviours::value);
			expects_vm<type_class> set_pod_address(const std::string_view& name, size_t size, uint64_t flags = (uint64_t)(object_behaviours::value | object_behaviours::pod));
			expects_vm<ref_class> set_class_address(const std::string_view& name, size_t size, uint64_t flags = (uint64_t)object_behaviours::ref);
			expects_vm<template_class> set_template_class_address(const std::string_view& decl, const std::string_view& name, size_t size, uint64_t flags = (uint64_t)object_behaviours::ref);
			expects_vm<type_interface> set_interface(const std::string_view& name);
			expects_vm<enumeration> set_enum(const std::string_view& name);
			void set_code_generator(const std::string_view& name, generator_callback&& callback);
			void set_preserve_source_code(bool enabled);
			void set_ts_imports(bool enabled, const std::string_view& import_syntax = "import from");
			void set_cache(bool enabled);
			void set_exception_callback(std::function<void(immediate_context*)>&& callback);
			void set_debugger(debugger_context* context);
			void set_compiler_error_callback(when_error_callback&& callback);
			void set_compiler_include_options(const compute::include_desc& new_desc);
			void set_compiler_features(const compute::preprocessor::desc& new_desc);
			void set_processor_options(compute::preprocessor* processor);
			void set_compile_callback(const std::string_view& section, compile_callback&& callback);
			void set_default_array_type(const std::string_view& type);
			void set_type_info_user_data_cleanup_callback(void(*callback)(asITypeInfo*), size_t type = 0);
			void set_engine_user_data_cleanup_callback(void(*callback)(asIScriptEngine*), size_t type = 0);
			void* set_user_data(void* data, size_t type = 0);
			void* get_user_data(size_t type = 0) const;
			void clear_cache();
			void clear_sections();
			void attach_debugger_to_context(asIScriptContext* context);
			void detach_debugger_from_context(asIScriptContext* context);
			void get_statistics(uint32_t* current_size, uint32_t* total_destroyed, uint32_t* total_detected, uint32_t* new_objects, uint32_t* total_new_destroyed) const;
			void forward_enum_references(void* reference, const type_info& type);
			void forward_release_references(void* reference, const type_info& type);
			void gc_enum_callback(void* reference);
			void gc_enum_callback(asIScriptFunction* reference);
			void gc_enum_callback(function_delegate* reference);
			bool trigger_debugger(immediate_context* context, uint64_t timeout_ms = 0);
			compute::expects_preprocessor<void> generate_code(compute::preprocessor* processor, const std::string_view& path, core::string& inout_buffer);
			core::unordered_map<core::string, core::string> generate_definitions(immediate_context* context);
			compiler* create_compiler();
			asIScriptModule* create_scoped_module(const std::string_view& name);
			asIScriptModule* create_module(const std::string_view& name);
			immediate_context* request_context();
			void return_context(immediate_context* context);
			bool get_byte_code_cache(byte_code_info* info);
			void set_byte_code_cache(byte_code_info* info);
			void* create_object(const type_info& type);
			void* create_object_copy(void* object, const type_info& type);
			void* create_empty_object(const type_info& type);
			function create_delegate(const function& function, void* object);
			void release_object(void* object, const type_info& type);
			void add_ref_object(void* object, const type_info& type);
			size_t begin_access_mask(size_t default_mask);
			size_t end_access_mask();
			std::string_view get_namespace() const;
			library get_module(const std::string_view& name);
			void set_library_property(library_features property, size_t value);
			size_t get_library_property(library_features property);
			size_t get_property(features property);
			void set_module_directory(const std::string_view& root);
			size_t get_property(features property) const;
			asIScriptEngine* get_engine() const;
			debugger_context* get_debugger() const;
			const core::string& get_module_directory() const;
			core::vector<core::string> get_exposed_addons();
			const core::unordered_map<core::string, addon>& get_system_addons() const;
			const core::unordered_map<core::string, clibrary>& get_clibraries() const;
			const compute::include_desc& get_compile_include_options() const;
			bool has_library(const std::string_view& name, bool is_addon = false);
			bool has_system_addon(const std::string_view& name);
			bool has_addon(const std::string_view& name);
			bool is_nullable(int type_id);
			bool has_debugger();
			bool add_system_addon(const std::string_view& name, const core::vector<core::string>& dependencies, addon_callback&& callback);
			expects_vm<void> import_file(const std::string_view& path, bool is_remote, core::string& output);
			expects_vm<void> import_cfunction(const core::vector<core::string>& sources, const std::string_view& name, const std::string_view& decl);
			expects_vm<void> import_clibrary(const std::string_view& path, bool iaddon = false);
			expects_vm<void> import_addon(const std::string_view& path);
			expects_vm<void> import_system_addon(const std::string_view& name);
			core::option<core::string> get_source_code_appendix(const std::string_view& label, const std::string_view& code, uint32_t line_number, uint32_t column_number, size_t max_lines);
			core::option<core::string> get_source_code_appendix_by_path(const std::string_view& label, const std::string_view& path, uint32_t line_number, uint32_t column_number, size_t max_lines);
			core::option<core::string> get_script_section(const std::string_view& section_name);
			size_t get_functions_count() const;
			function get_function_by_id(int id) const;
			function get_function_by_index(size_t index) const;
			function get_function_by_decl(const std::string_view& decl) const;
			size_t get_properties_count() const;
			size_t get_objects_count() const;
			type_info get_object_by_index(size_t index) const;
			size_t get_enum_count() const;
			type_info get_enum_by_index(size_t index) const;
			size_t get_function_defs_count() const;
			type_info get_function_def_by_index(size_t index) const;
			size_t get_modules_count() const;
			asIScriptModule* get_module_by_id(int id) const;
			int get_type_id_by_decl(const std::string_view& decl) const;
			std::string_view get_type_id_decl(int type_id, bool include_namespace = false) const;
			type_info get_type_info_by_id(int type_id) const;
			type_info get_type_info_by_name(const std::string_view& name);
			type_info get_type_info_by_decl(const std::string_view& decl) const;

		private:
			immediate_context* create_context();
			expects_vm<void> initialize_addon(const std::string_view& name, clibrary& library);
			void uninitialize_addon(const std::string_view& name, clibrary& library);

		public:
			static void line_handler(asIScriptContext* context, void* object);
			static void exception_handler(asIScriptContext* context, void* object);
			static void set_memory_functions(void* (*alloc)(size_t), void(*free)(void*));
			static void cleanup_this_thread();
			static std::string_view get_error_name_info(virtual_error code);
			static byte_code_label get_byte_code_info(uint8_t code);
			static virtual_machine* get(asIScriptEngine* engine);
			static virtual_machine* get();
			static size_t get_default_access_mask();
			static void cleanup();

		private:
			static std::string_view get_library_name(const std::string_view& path);
			static asIScriptContext* request_raw_context(asIScriptEngine* engine, void* data);
			static void return_raw_context(asIScriptEngine* engine, asIScriptContext* context, void* data);
			static void message_logger(asSMessageInfo* info, void* object);
			static void* get_nullable();

		public:
			template <typename t>
			expects_vm<void> set_function(const std::string_view& decl, t value)
			{
				asSFuncPtr* ptr = bridge::function_call<t>(value);
				auto result = set_function_address(decl, ptr, convention::cdecl_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <void(*)(asIScriptGeneric*)>
			expects_vm<void> set_function(const std::string_view& decl, void(*value)(asIScriptGeneric*))
			{
				asSFuncPtr* ptr = bridge::function_generic_call<void (*)(asIScriptGeneric*)>(value);
				auto result = set_function_address(decl, ptr, convention::generic_call);
				function_factory::release_functor(&ptr);
				return result;
			}
			template <typename t>
			expects_vm<void> set_property(const std::string_view& decl, t* value)
			{
				return set_property_address(decl, (void*)value);
			}
			template <typename t>
			expects_vm<ref_class> set_class(const std::string_view& name, bool gc)
			{
				auto ref_type = get_type_info_by_name(name);
				if (ref_type.is_valid())
					return ref_class(this, ref_type.get_type_info(), ref_type.get_type_id());

				auto data_class = set_class_address(name, sizeof(t), gc ? (size_t)object_behaviours::ref | (size_t)object_behaviours::gc : (size_t)object_behaviours::ref);
				if (!data_class)
					return data_class;

				auto status = data_class->template set_add_ref<t>();
				if (!status)
					return status.error();

				status = data_class->template set_release<t>();
				if (!status)
					return status.error();

				if (!gc)
					return data_class;

				status = data_class->template set_mark_ref<t>();
				if (!status)
					return status.error();

				status = data_class->template set_is_marked_ref<t>();
				if (!status)
					return status.error();

				status = data_class->template set_ref_count<t>();
				if (!status)
					return status.error();

				return data_class;
			}
			template <typename t>
			expects_vm<ref_class> set_interface_class(const std::string_view& name)
			{
				auto ref_type = get_type_info_by_name(name);
				if (ref_type.is_valid())
					return ref_class(this, ref_type.get_type_info(), ref_type.get_type_id());

				return set_class_address(name, sizeof(t), (size_t)object_behaviours::ref | (size_t)object_behaviours::nocount);
			}
			template <typename t>
			expects_vm<template_class> set_template_class(const std::string_view& decl, const std::string_view& name, bool gc)
			{
				auto ref_type = get_type_info_by_decl(decl);
				if (ref_type.is_valid())
					return template_class(this, name);

				auto data_class = set_template_class_address(decl, name, sizeof(t), gc ? (size_t)object_behaviours::pattern | (size_t)object_behaviours::ref | (size_t)object_behaviours::gc : (size_t)object_behaviours::pattern | (size_t)object_behaviours::ref);
				if (!data_class)
					return data_class;

				auto status = data_class->template set_add_ref<t>();
				if (!status)
					return status.error();

				status = data_class->template set_release<t>();
				if (!status)
					return status.error();

				if (!gc)
					return data_class;

				status = data_class->template set_mark_ref<t>();
				if (!status)
					return status.error();

				status = data_class->template set_is_marked_ref<t>();
				if (!status)
					return status.error();

				status = data_class->template set_ref_count<t>();
				if (!status)
					return status.error();

				return data_class;
			}
			template <typename t>
			expects_vm<template_class> set_template_specialization_class(const std::string_view& name, bool gc)
			{
				auto data_class = set_template_class_address(name, name, sizeof(t), gc ? (size_t)object_behaviours::ref | (size_t)object_behaviours::gc : (size_t)object_behaviours::ref);
				if (!data_class)
					return data_class;

				auto status = data_class->template set_add_ref<t>();
				if (!status)
					return status.error();

				status = data_class->template set_release<t>();
				if (!status)
					return status.error();

				if (!gc)
					return data_class;

				status = data_class->template set_mark_ref<t>();
				if (!status)
					return status.error();

				status = data_class->template set_is_marked_ref<t>();
				if (!status)
					return status.error();

				status = data_class->template set_ref_count<t>();
				if (!status)
					return status.error();

				return data_class;
			}
			template <typename t>
			expects_vm<type_class> set_struct_trivial(const std::string_view& name, size_t traits = 0)
			{
				auto ref_type = get_type_info_by_name(name);
				if (ref_type.is_valid())
					return type_class(this, ref_type.get_type_info(), ref_type.get_type_id());

				auto data_struct = set_struct_address(name, sizeof(t), (size_t)object_behaviours::value | bridge::type_traits_of<t>() | traits);
				if (!data_struct)
					return data_struct;

				auto status = data_struct->template set_operator_copy<t>();
				if (!status)
					return status.error();

				status = data_struct->template set_destructor<t>("void f()");
				if (!status)
					return status.error();

				return data_struct;
			}
			template <typename t>
			expects_vm<type_class> set_struct(const std::string_view& name, size_t traits = 0)
			{
				auto ref_type = get_type_info_by_name(name);
				if (ref_type.is_valid())
					return type_class(this, ref_type.get_type_info(), ref_type.get_type_id());

				return set_struct_address(name, sizeof(t), (size_t)object_behaviours::value | bridge::type_traits_of<t>() | traits);
			}
			template <typename t>
			expects_vm<type_class> set_pod(const std::string_view& name, size_t traits = 0)
			{
				auto ref_type = get_type_info_by_name(name);
				if (ref_type.is_valid())
					return type_class(this, ref_type.get_type_info(), ref_type.get_type_id());

				return set_pod_address(name, sizeof(t), (size_t)object_behaviours::value | (size_t)object_behaviours::pod | bridge::type_traits_of<t>() | traits);
			}
		};

		class event_loop final : public core::reference<event_loop>
		{
		public:
			struct callable
			{
				function_delegate delegation;
				args_callback on_args;
				args_callback on_return;
				immediate_context* context;

				callable(immediate_context* new_context) noexcept;
				callable(immediate_context* new_context, function_delegate&& new_delegate, args_callback&& new_on_args, args_callback&& new_on_return) noexcept;
				callable(const callable& other) noexcept;
				callable(callable&& other) noexcept;
				~callable() = default;
				callable& operator= (const callable& other) noexcept;
				callable& operator= (callable&& other) noexcept;
				bool is_notification() const;
				bool is_callback() const;
			};

		private:
			core::single_queue<callable> queue;
			std::condition_variable waitable;
			std::mutex mutex;
			args_callback on_enqueue;
			bool aborts;
			bool wake;

		public:
			event_loop() noexcept;
			~event_loop() = default;
			void wakeup();
			void restore();
			void abort();
			void when(args_callback&& callback);
			void listen(immediate_context* context);
			void unlisten(immediate_context* context);
			void enqueue(immediate_context* context);
			void enqueue(function_delegate&& delegatef, args_callback&& on_args, args_callback&& on_return);
			bool poll(immediate_context* context, uint64_t timeout_ms);
			bool poll_extended(immediate_context* context, uint64_t timeout_ms);
			size_t dequeue(virtual_machine* vm, size_t max_executions = 0);
			bool is_aborted();

		private:
			void on_notification(immediate_context* context);
			void on_callback(immediate_context* context, function_delegate&& delegatef, args_callback&& on_args, args_callback&& on_return);
			void abort_if(expects_vm<execution>&& status);

		public:
			static void set(event_loop* for_current_thread);
			static event_loop* get();
		};
	}
}
#endif