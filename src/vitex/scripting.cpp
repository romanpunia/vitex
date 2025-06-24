#include "scripting.h"
#include "bindings.h"
#include "vitex.h"
#include <sstream>
#define ADDON_ANY "any"
#ifdef VI_ANGELSCRIPT
#include <angelscript.h>
#include <as_texts.h>
#define COMPILER_BLOCKED_WAIT_US 100
#define THREAD_BLOCKED_WAIT_MS 50

namespace
{
	class cbyte_code_stream : public asIBinaryStream
	{
	private:
		vitex::core::vector<asBYTE> code;
		int read_pos, write_pos;

	public:
		cbyte_code_stream() : read_pos(0), write_pos(0)
		{
		}
		cbyte_code_stream(const vitex::core::vector<asBYTE>& data) : code(data), read_pos(0), write_pos(0)
		{
		}
		int Read(void* ptr, asUINT size) override
		{
			VI_ASSERT(ptr && size, "corrupted read");
			memcpy(ptr, &code[read_pos], size);
			read_pos += size;

			return 0;
		}
		int Write(const void* ptr, asUINT size) override
		{
			VI_ASSERT(ptr && size, "corrupted write");
			code.resize(code.size() + size);
			memcpy(&code[write_pos], ptr, size);
			write_pos += size;

			return 0;
		}
		vitex::core::vector<asBYTE>& GetCode()
		{
			return code;
		}
		asUINT GetSize()
		{
			return (asUINT)code.size();
		}
	};

	struct denum
	{
		vitex::core::vector<vitex::core::string> values;
	};

	struct dclass
	{
		vitex::core::vector<vitex::core::string> props;
		vitex::core::vector<vitex::core::string> interfaces;
		vitex::core::vector<vitex::core::string> types;
		vitex::core::vector<vitex::core::string> funcdefs;
		vitex::core::vector<vitex::core::string> methods;
		vitex::core::vector<vitex::core::string> functions;
	};

	struct dnamespace
	{
		vitex::core::unordered_map<vitex::core::string, denum> enums;
		vitex::core::unordered_map<vitex::core::string, dclass> classes;
		vitex::core::vector<vitex::core::string> funcdefs;
		vitex::core::vector<vitex::core::string> functions;
	};

	vitex::core::string get_combination(const vitex::core::vector<vitex::core::string>& names, const vitex::core::string& by)
	{
		vitex::core::string result;
		for (size_t i = 0; i < names.size(); i++)
		{
			result.append(names[i]);
			if (i + 1 < names.size())
				result.append(by);
		}

		return result;
	}
	vitex::core::string get_combination_all(const vitex::core::vector<vitex::core::string>& names, const vitex::core::string& by, const vitex::core::string& end_by)
	{
		vitex::core::string result;
		for (size_t i = 0; i < names.size(); i++)
		{
			result.append(names[i]);
			if (i + 1 < names.size())
				result.append(by);
			else
				result.append(end_by);
		}

		return result;
	}
	vitex::core::string get_type_naming(asITypeInfo* type)
	{
		const char* name_space = type->GetNamespace();
		return (name_space ? name_space + vitex::core::string("::") : vitex::core::string("")) + type->GetName();
	}
	asITypeInfo* get_type_namespacing(asIScriptEngine* engine, const vitex::core::string& name)
	{
		asITypeInfo* result = engine->GetTypeInfoByName(name.c_str());
		if (result != nullptr)
			return result;

		return engine->GetTypeInfoByName((name + "@").c_str());
	}
	void dump_namespace(vitex::core::string& source, const vitex::core::string& naming, dnamespace& name_space, vitex::core::string& offset)
	{
		if (!naming.empty())
		{
			offset.append("\t");
			source += vitex::core::stringify::text("namespace %s\n{\n", naming.c_str());
		}

		for (auto it = name_space.enums.begin(); it != name_space.enums.end(); it++)
		{
			auto copy = it;
			source += vitex::core::stringify::text("%senum %s\n%s{\n\t%s", offset.c_str(), it->first.c_str(), offset.c_str(), offset.c_str());
			source += vitex::core::stringify::text("%s", get_combination(it->second.values, ",\n\t" + offset).c_str());
			source += vitex::core::stringify::text("\n%s}\n%s", offset.c_str(), ++copy != name_space.enums.end() ? "\n" : "");
		}

		if (!name_space.enums.empty() && (!name_space.classes.empty() || !name_space.funcdefs.empty() || !name_space.functions.empty()))
			source += vitex::core::stringify::text("\n");

		for (auto it = name_space.classes.begin(); it != name_space.classes.end(); it++)
		{
			auto copy = it;
			source += vitex::core::stringify::text("%sclass %s%s%s%s%s%s\n%s{\n\t%s",
				offset.c_str(),
				it->first.c_str(),
				it->second.types.empty() ? "" : "<",
				it->second.types.empty() ? "" : get_combination(it->second.types, ", ").c_str(),
				it->second.types.empty() ? "" : ">",
				it->second.interfaces.empty() ? "" : " : ",
				it->second.interfaces.empty() ? "" : get_combination(it->second.interfaces, ", ").c_str(),
				offset.c_str(), offset.c_str());
			source += vitex::core::stringify::text("%s", get_combination_all(it->second.funcdefs, ";\n\t" + offset, it->second.props.empty() && it->second.methods.empty() ? ";" : ";\n\n\t" + offset).c_str());
			source += vitex::core::stringify::text("%s", get_combination_all(it->second.props, ";\n\t" + offset, it->second.methods.empty() ? ";" : ";\n\n\t" + offset).c_str());
			source += vitex::core::stringify::text("%s", get_combination_all(it->second.methods, ";\n\t" + offset, ";").c_str());
			source += vitex::core::stringify::text("\n%s}\n%s", offset.c_str(), !it->second.functions.empty() || ++copy != name_space.classes.end() ? "\n" : "");

			if (it->second.functions.empty())
				continue;

			source += vitex::core::stringify::text("%snamespace %s\n%s{\n\t%s", offset.c_str(), it->first.c_str(), offset.c_str(), offset.c_str());
			source += vitex::core::stringify::text("%s", get_combination_all(it->second.functions, ";\n\t" + offset, ";").c_str());
			source += vitex::core::stringify::text("\n%s}\n%s", offset.c_str(), ++copy != name_space.classes.end() ? "\n" : "");
		}

		if (!name_space.funcdefs.empty())
		{
			if (!name_space.enums.empty() || !name_space.classes.empty())
				source += vitex::core::stringify::text("\n%s", offset.c_str());
			else
				source += vitex::core::stringify::text("%s", offset.c_str());
		}

		source += vitex::core::stringify::text("%s", get_combination_all(name_space.funcdefs, ";\n" + offset, name_space.functions.empty() ? ";\n" : "\n\n" + offset).c_str());
		if (!name_space.functions.empty() && name_space.funcdefs.empty())
		{
			if (!name_space.enums.empty() || !name_space.classes.empty())
				source += vitex::core::stringify::text("\n");
			else
				source += vitex::core::stringify::text("%s", offset.c_str());
		}

		source += vitex::core::stringify::text("%s", get_combination_all(name_space.functions, ";\n" + offset, ";\n").c_str());
		if (!naming.empty())
		{
			source += vitex::core::stringify::text("}");
			offset.erase(offset.begin());
		}
		else
			source.erase(source.end() - 1);
	}
}
#endif

namespace vitex
{
	namespace scripting
	{
		static core::vector<core::string> extract_lines_of_code(const std::string_view& code, int line, int max)
		{
			core::vector<core::string> total;
			size_t start = 0, size = code.size();
			size_t offset = 0, lines = 0;
			size_t left_side = (max - 1) / 2;
			size_t right_side = (max - 1) - left_side;
			size_t base_right_side = (right_side > 0 ? right_side - 1 : 0);

			VI_ASSERT(max > 0, "max lines count should be at least one");
			while (offset < size)
			{
				if (code[offset++] != '\n')
				{
					if (offset != size)
						continue;
				}

				++lines;
				if (lines >= line - left_side && left_side > 0)
				{
					core::string copy = core::string(code.substr(start, offset - start));
					core::stringify::replace_of(copy, "\r\n\t\v", " ");
					total.push_back(std::move(copy));
					--left_side; --max;
				}
				else if (lines == line)
				{
					core::string copy = core::string(code.substr(start, offset - start));
					core::stringify::replace_of(copy, "\r\n\t\v", " ");
					total.insert(total.begin(), std::move(copy));
					--max;
				}
				else if (lines >= line + (right_side - base_right_side) && right_side > 0)
				{
					core::string copy = core::string(code.substr(start, offset - start));
					core::stringify::replace_of(copy, "\r\n\t\v", " ");
					total.push_back(std::move(copy));
					--right_side; --max;
				}

				start = offset;
				if (!max)
					break;
			}

			for (auto& item : total)
			{
				if (!core::stringify::is_empty_or_whitespace(item))
					return total;
			}

			total.clear();
			return total;
		}
		static core::string char_trim_end(const std::string_view& value)
		{
			core::string copy = core::string(value);
			core::stringify::trim_end(copy);
			return copy;
		}
		static const char* or_empty(const char* value)
		{
			return value ? value : "";
		}

		virtual_exception::virtual_exception(virtual_error new_error_code) : error_code(new_error_code)
		{
			if (error_code != virtual_error::success)
				error_message += virtual_machine::get_error_name_info(error_code);
		}
		virtual_exception::virtual_exception(virtual_error new_error_code, core::string&& new_message) : error_code(new_error_code)
		{
			error_message = std::move(new_message);
			if (error_code == virtual_error::success)
				return;

			error_message += " CAUSING ";
			error_message += virtual_machine::get_error_name_info(error_code);
		}
		virtual_exception::virtual_exception(core::string&& new_message)
		{
			error_message = std::move(new_message);
		}
		const char* virtual_exception::type() const noexcept
		{
			return "virtual_error";
		}
		virtual_error virtual_exception::code() const noexcept
		{
			return error_code;
		}

		uint64_t type_cache::set(uint64_t id, const std::string_view& name)
		{
			VI_ASSERT(id > 0 && !name.empty(), "id should be greater than zero and name should not be empty");

			using map = core::unordered_map<uint64_t, std::pair<core::string, int>>;
			if (!names)
				names = core::memory::init<map>();

			(*names)[id] = std::make_pair(name, (int)-1);
			return id;
		}
		int type_cache::get_type_id(uint64_t id)
		{
			auto it = names->find(id);
			if (it == names->end())
				return -1;

			if (it->second.second < 0)
			{
				virtual_machine* engine = virtual_machine::get();
				VI_ASSERT(engine != nullptr, "engine should be set");
				it->second.second = engine->get_type_id_by_decl(it->second.first.c_str());
			}

			return it->second.second;
		}
		void type_cache::cleanup()
		{
			core::memory::deinit(names);
		}
		core::unordered_map<uint64_t, std::pair<core::string, int>>* type_cache::names = nullptr;

		expects_vm<void> parser::replace_inline_preconditions(const std::string_view& keyword, core::string& data, const std::function<expects_vm<core::string>(const std::string_view& expression)>& replacer)
		{
			return replace_preconditions(false, core::string(keyword) + ' ', data, replacer);
		}
		expects_vm<void> parser::replace_directive_preconditions(const std::string_view& keyword, core::string& data, const std::function<expects_vm<core::string>(const std::string_view& expression)>& replacer)
		{
			return replace_preconditions(true, keyword, data, replacer);
		}
		expects_vm<void> parser::replace_preconditions(bool is_directive, const std::string_view& match, core::string& code, const std::function<expects_vm<core::string>(const std::string_view& expression)>& replacer)
		{
			VI_ASSERT(!match.empty(), "keyword should not be empty");
			VI_ASSERT(replacer != nullptr, "replacer callback should not be empty");
			size_t match_size = match.size();
			size_t offset = 0;

			while (offset < code.size())
			{
				char u = code[offset];
				if (u == '/' && offset + 1 < code.size() && (code[offset + 1] == '/' || code[offset + 1] == '*'))
				{
					if (code[++offset] == '*')
					{
						while (offset + 1 < code.size())
						{
							char n = code[offset++];
							if (n == '*' && code[offset++] == '/')
								break;
						}
					}
					else
					{
						while (offset < code.size())
						{
							char n = code[offset++];
							if (n == '\r' || n == '\n')
								break;
						}
					}

					continue;
				}
				else if (u == '\"' || u == '\'')
				{
					++offset;
					while (offset < code.size())
					{
						if (code[offset++] == u && core::stringify::is_not_preceded_by_escape(code, offset - 1))
							break;
					}

					continue;
				}
				else if (code.size() - offset < match_size || memcmp(code.c_str() + offset, match.data(), match_size) != 0)
				{
					++offset;
					continue;
				}

				size_t start = offset + match_size;
				while (start < code.size())
				{
					if (!core::stringify::is_whitespace(code[start]))
						break;
					++start;
				}

				int32_t brackets = 0;
				size_t end = start;
				while (end < code.size())
				{
					char v = code[end];
					if (v == ')')
					{
						if (--brackets < 0)
							break;
					}
					else if (v == '"' || v == '\'')
					{
						++end;
						while (end < code.size() && (code[end++] != v || !core::stringify::is_not_preceded_by_escape(code, end - 1)));
						--end;
					}
					else if (v == ';')
					{
						if (is_directive)
							++end;
						break;
					}
					else if (v == '(')
						++brackets;
					end++;
				}

				if (brackets > 0)
					return virtual_exception(core::stringify::text("unexpected end of file, expected closing ')' symbol, offset %" PRIu64 ":\n%.*s <<<", (uint64_t)end, (int)(end - start), code.c_str() + start));

				if (end == start)
				{
					offset = end;
					continue;
				}

				auto expression = replacer(code.substr(start, end - start));
				if (!expression)
					return expression.error();

				core::stringify::replace_part(code, offset, end, *expression);
				if (expression->find(match) == std::string::npos)
					offset += expression->size();
			}

			return core::expectation::met;
		}

		expects_vm<void> function_factory::atomic_notify_gc(const std::string_view& type_name, void* object)
		{
			VI_ASSERT(object != nullptr, "object should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* context = asGetActiveContext();
			VI_ASSERT(context != nullptr, "context should be set");

			virtual_machine* engine = virtual_machine::get(context->GetEngine());
			VI_ASSERT(engine != nullptr, "engine should be set");

			type_info type = engine->get_type_info_by_name(type_name);
			return engine->notify_of_new_object(object, type.get_type_info());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> function_factory::atomic_notify_gc_by_id(int type_id, void* object)
		{
			VI_ASSERT(object != nullptr, "object should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* context = asGetActiveContext();
			VI_ASSERT(context != nullptr, "context should be set");

			virtual_machine* engine = virtual_machine::get(context->GetEngine());
			VI_ASSERT(engine != nullptr, "engine should be set");

			type_info type = engine->get_type_info_by_id(type_id);
			return engine->notify_of_new_object(object, type.get_type_info());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		asSFuncPtr* function_factory::create_function_base(void(*base)(), int type)
		{
			VI_ASSERT(base != nullptr, "function pointer should be set");
#ifdef VI_ANGELSCRIPT
			asSFuncPtr* ptr = core::memory::init<asSFuncPtr>(type);
			ptr->ptr.f.func = reinterpret_cast<asFUNCTION_t>(base);
			return ptr;
#else
			return nullptr;
#endif
		}
		asSFuncPtr* function_factory::create_method_base(const void* base, size_t size, int type)
		{
			VI_ASSERT(base != nullptr, "function pointer should be set");
#ifdef VI_ANGELSCRIPT
			asSFuncPtr* ptr = core::memory::init<asSFuncPtr>(type);
			ptr->CopyMethodPtr(base, size);
			return ptr;
#else
			return nullptr;
#endif
		}
		asSFuncPtr* function_factory::create_dummy_base()
		{
#ifdef VI_ANGELSCRIPT
			return core::memory::init<asSFuncPtr>(0);
#else
			return nullptr;
#endif
		}
		void function_factory::release_functor(asSFuncPtr** ptr)
		{
			if (!ptr || !*ptr)
				return;
#ifdef VI_ANGELSCRIPT
			core::memory::deinit(*ptr);
#endif
		}
		void function_factory::gc_enum_callback(asIScriptEngine* engine, void* reference)
		{
#ifdef VI_ANGELSCRIPT
			if (reference != nullptr)
				engine->GCEnumCallback(reference);
#endif
		}
		void function_factory::gc_enum_callback(asIScriptEngine* engine, asIScriptFunction* reference)
		{
			if (!reference)
				return;
#ifdef VI_ANGELSCRIPT
			engine->GCEnumCallback(reference);
			gc_enum_callback(engine, reference->GetDelegateFunction());
			gc_enum_callback(engine, reference->GetDelegateObject());
			gc_enum_callback(engine, reference->GetDelegateObjectType());
#endif
		}
		void function_factory::gc_enum_callback(asIScriptEngine* engine, function_delegate* reference)
		{
			if (reference && reference->is_valid())
				gc_enum_callback(engine, reference->callback);
		}

		message_info::message_info(asSMessageInfo* msg) noexcept : info(msg)
		{
		}
		std::string_view message_info::get_section() const
		{
			VI_ASSERT(is_valid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(info->section);
#else
			return or_empty("");
#endif
		}
		std::string_view message_info::get_text() const
		{
			VI_ASSERT(is_valid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(info->message);
#else
			return "";
#endif
		}
		log_category message_info::get_type() const
		{
			VI_ASSERT(is_valid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return (log_category)info->type;
#else
			return log_category::err;
#endif
		}
		int message_info::get_row() const
		{
			VI_ASSERT(is_valid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return info->row;
#else
			return 0;
#endif
		}
		int message_info::get_column() const
		{
			VI_ASSERT(is_valid(), "message should be valid");
#ifdef VI_ANGELSCRIPT
			return info->col;
#else
			return 0;
#endif
		}
		asSMessageInfo* message_info::get_message_info() const
		{
			return info;
		}
		bool message_info::is_valid() const
		{
			return info != nullptr;
		}

		type_info::type_info(asITypeInfo* type_info) noexcept : info(type_info)
		{
#ifdef VI_ANGELSCRIPT
			vm = (info ? virtual_machine::get(info->GetEngine()) : nullptr);
#endif
		}
		void type_info::for_each_property(const property_callback& callback)
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
			VI_ASSERT(callback, "type_info should not be empty");
#ifdef VI_ANGELSCRIPT
			unsigned int count = info->GetPropertyCount();
			for (unsigned int i = 0; i < count; i++)
			{
				function_info prop;
				if (get_property(i, &prop))
					callback(this, &prop);
			}
#endif
		}
		void type_info::for_each_method(const method_callback& callback)
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
			VI_ASSERT(callback, "type_info should not be empty");
#ifdef VI_ANGELSCRIPT
			unsigned int count = info->GetMethodCount();
			for (unsigned int i = 0; i < count; i++)
			{
				function method = info->GetMethodByIndex(i);
				if (method.is_valid())
					callback(this, &method);
			}
#endif
		}
		std::string_view type_info::get_group() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(info->GetConfigGroup());
#else
			return "";
#endif
		}
		size_t type_info::get_access_mask() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetAccessMask();
#else
			return 0;
#endif
		}
		library type_info::get_module() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetModule();
#else
			return library(nullptr);
#endif
		}
		void type_info::add_ref() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			info->AddRef();
#endif
		}
		void type_info::release()
		{
			if (!is_valid())
				return;
#ifdef VI_ANGELSCRIPT
			info->Release();
#endif
		}
		std::string_view type_info::get_name() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(info->GetName());
#else
			return "";
#endif
		}
		std::string_view type_info::get_namespace() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(info->GetNamespace());
#else
			return "";
#endif
		}
		type_info type_info::get_base_type() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetBaseType();
#else
			return type_info(nullptr);
#endif
		}
		bool type_info::derives_from(const type_info& type) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->DerivesFrom(type.get_type_info());
#else
			return false;
#endif
		}
		size_t type_info::flags() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetFlags();
#else
			return 0;
#endif
		}
		size_t type_info::size() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)info->GetSize();
#else
			return 0;
#endif
		}
		int type_info::get_type_id() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetTypeId();
#else
			return -1;
#endif
		}
		int type_info::get_sub_type_id(size_t sub_type_index) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetSubTypeId((asUINT)sub_type_index);
#else
			return -1;
#endif
		}
		type_info type_info::get_sub_type(size_t sub_type_index) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetSubType((asUINT)sub_type_index);
#else
			return type_info(nullptr);
#endif
		}
		size_t type_info::get_sub_type_count() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)info->GetSubTypeCount();
#else
			return 0;
#endif
		}
		size_t type_info::get_interface_count() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)info->GetInterfaceCount();
#else
			return 0;
#endif
		}
		type_info type_info::get_interface(size_t index) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetInterface((asUINT)index);
#else
			return type_info(nullptr);
#endif
		}
		bool type_info::implements(const type_info& type) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->Implements(type.get_type_info());
#else
			return false;
#endif
		}
		size_t type_info::get_factories_count() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)info->GetFactoryCount();
#else
			return 0;
#endif
		}
		function type_info::get_factory_by_index(size_t index) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetFactoryByIndex((asUINT)index);
#else
			return function(nullptr);
#endif
		}
		function type_info::get_factory_by_decl(const std::string_view& decl) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return info->GetFactoryByDecl(decl.data());
#else
			return function(nullptr);
#endif
		}
		size_t type_info::get_methods_count() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)info->GetMethodCount();
#else
			return 0;
#endif
		}
		function type_info::get_method_by_index(size_t index, bool get_virtual) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetMethodByIndex((asUINT)index, get_virtual);
#else
			return function(nullptr);
#endif
		}
		function type_info::get_method_by_name(const std::string_view& name, bool get_virtual) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
#ifdef VI_ANGELSCRIPT
			return info->GetMethodByName(name.data(), get_virtual);
#else
			return function(nullptr);
#endif
		}
		function type_info::get_method_by_decl(const std::string_view& decl, bool get_virtual) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return info->GetMethodByDecl(decl.data(), get_virtual);
#else
			return function(nullptr);
#endif
		}
		size_t type_info::get_properties_count() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)info->GetPropertyCount();
#else
			return 0;
#endif
		}
		expects_vm<void> type_info::get_property(size_t index, function_info* out) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			const char* name;
			asDWORD access_mask;
			int type_id, offset;
			bool is_private;
			bool is_protected;
			bool is_reference;
			int r = info->GetProperty((asUINT)index, &name, &type_id, &is_private, &is_protected, &offset, &is_reference, &access_mask);
			if (out != nullptr)
			{
				out->name = name;
				out->access_mask = (size_t)access_mask;
				out->type_id = type_id;
				out->offset = offset;
				out->is_private = is_private;
				out->is_protected = is_protected;
				out->is_reference = is_reference;
			}
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		std::string_view type_info::get_property_declaration(size_t index, bool include_namespace) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(info->GetPropertyDeclaration((asUINT)index, include_namespace));
#else
			return "";
#endif
		}
		size_t type_info::get_behaviour_count() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)info->GetBehaviourCount();
#else
			return 0;
#endif
		}
		function type_info::get_behaviour_by_index(size_t index, behaviours* out_behaviour) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			asEBehaviours out;
			asIScriptFunction* result = info->GetBehaviourByIndex((asUINT)index, &out);
			if (out_behaviour != nullptr)
				*out_behaviour = (behaviours)out;

			return result;
#else
			return function(nullptr);
#endif
		}
		size_t type_info::get_child_function_def_count() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)info->GetChildFuncdefCount();
#else
			return 0;
#endif
		}
		type_info type_info::get_child_function_def(size_t index) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetChildFuncdef((asUINT)index);
#else
			return type_info(nullptr);
#endif
		}
		type_info type_info::get_parent_type() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetParentType();
#else
			return type_info(nullptr);
#endif
		}
		size_t type_info::get_enum_value_count() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)info->GetEnumValueCount();
#else
			return 0;
#endif
		}
		std::string_view type_info::get_enum_value_by_index(size_t index, int* out_value) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(info->GetEnumValueByIndex((asUINT)index, out_value));
#else
			return "";
#endif
		}
		function type_info::get_function_def_signature() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetFuncdefSignature();
#else
			return function(nullptr);
#endif
		}
		void* type_info::set_user_data(void* data, size_t type)
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->SetUserData(data, type);
#else
			return nullptr;
#endif
		}
		void* type_info::get_user_data(size_t type) const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return info->GetUserData(type);
#else
			return nullptr;
#endif
		}
		bool type_info::is_handle() const
		{
			VI_ASSERT(is_valid(), "type_info should be valid");
#ifdef VI_ANGELSCRIPT
			return is_handle(info->GetTypeId());
#else
			return false;
#endif
		}
		bool type_info::is_valid() const
		{
#ifdef VI_ANGELSCRIPT
			return vm != nullptr && info != nullptr;
#else
			return false;
#endif
		}
		asITypeInfo* type_info::get_type_info() const
		{
#ifdef VI_ANGELSCRIPT
			return info;
#else
			return nullptr;
#endif
		}
		virtual_machine* type_info::get_vm() const
		{
#ifdef VI_ANGELSCRIPT
			return vm;
#else
			return nullptr;
#endif
		}
		bool type_info::is_handle(int type_id)
		{
#ifdef VI_ANGELSCRIPT
			return (type_id & asTYPEID_OBJHANDLE || type_id & asTYPEID_HANDLETOCONST);
#else
			return false;
#endif
		}
		bool type_info::is_script_object(int type_id)
		{
#ifdef VI_ANGELSCRIPT
			return (type_id & asTYPEID_SCRIPTOBJECT);
#else
			return false;
#endif
		}

		function::function(asIScriptFunction* base) noexcept : ptr(base)
		{
#ifdef VI_ANGELSCRIPT
			vm = (base ? virtual_machine::get(base->GetEngine()) : nullptr);
#endif
		}
		function::function(const function& base) noexcept : vm(base.vm), ptr(base.ptr)
		{
		}
		void function::add_ref() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			ptr->AddRef();
#endif
		}
		void function::release()
		{
			if (!is_valid())
				return;
#ifdef VI_ANGELSCRIPT
			ptr->Release();
#endif
		}
		int function::get_id() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->GetId();
#else
			return -1;
#endif
		}
		function_type function::get_type() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return (function_type)ptr->GetFuncType();
#else
			return function_type::dummy_function;
#endif
		}
		uint32_t* function::get_byte_code(size_t* size) const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			asUINT data_size = 0;
			asDWORD* data = ptr->GetByteCode(&data_size);
			if (size != nullptr)
				*size = data_size;
			return (uint32_t*)data;
#else
			return nullptr;
#endif
		}
		std::string_view function::get_module_name() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(ptr->GetModuleName());
#else
			return "";
#endif
		}
		library function::get_module() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->GetModule();
#else
			return library(nullptr);
#endif
		}
		std::string_view function::get_section_name() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			const char* script_section = nullptr;
			ptr->GetDeclaredAt(&script_section, nullptr, nullptr);
			return or_empty(script_section);
#else
			return "";
#endif
		}
		std::string_view function::get_group() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(ptr->GetConfigGroup());
#else
			return "";
#endif
		}
		size_t function::get_access_mask() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->GetAccessMask();
#else
			return 0;
#endif
		}
		type_info function::get_object_type() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->GetObjectType();
#else
			return type_info(nullptr);
#endif
		}
		std::string_view function::get_object_name() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(ptr->GetObjectName());
#else
			return "";
#endif
		}
		std::string_view function::get_name() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(ptr->GetName());
#else
			return "";
#endif
		}
		std::string_view function::get_namespace() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(ptr->GetNamespace());
#else
			return "";
#endif
		}
		std::string_view function::get_decl(bool include_object_name, bool include_namespace, bool include_arg_names) const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(ptr->GetDeclaration(include_object_name, include_namespace, include_arg_names));
#else
			return "";
#endif
		}
		bool function::is_read_only() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->IsReadOnly();
#else
			return false;
#endif
		}
		bool function::is_private() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->IsPrivate();
#else
			return false;
#endif
		}
		bool function::is_protected() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->IsProtected();
#else
			return false;
#endif
		}
		bool function::is_final() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->IsFinal();
#else
			return false;
#endif
		}
		bool function::is_override() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->IsOverride();
#else
			return false;
#endif
		}
		bool function::is_shared() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->IsShared();
#else
			return false;
#endif
		}
		bool function::is_explicit() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->IsExplicit();
#else
			return false;
#endif
		}
		bool function::is_property() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->IsProperty();
#else
			return false;
#endif
		}
		size_t function::get_args_count() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)ptr->GetParamCount();
#else
			return 0;
#endif
		}
		expects_vm<void> function::get_arg(size_t index, int* type_id, size_t* flags, std::string_view* name, std::string_view* default_arg) const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			asDWORD asFlags;
			const char* asName = "";
			const char* asDefaultArg = "";
			int r = ptr->GetParam((asUINT)index, type_id, &asFlags, &asName, &asDefaultArg);
			if (flags != nullptr)
				*flags = (size_t)asFlags;
			if (name != nullptr)
				*name = asName;
			if (default_arg != nullptr)
				*default_arg = asDefaultArg;
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		int function::get_return_type_id(size_t* flags) const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			asDWORD asFlags;
			int r = ptr->GetReturnTypeId(&asFlags);
			if (flags != nullptr)
				*flags = (size_t)asFlags;

			return r;
#else
			return -1;
#endif
		}
		int function::get_type_id() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->GetTypeId();
#else
			return -1;
#endif
		}
		bool function::is_compatible_with_type_id(int type_id) const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->IsCompatibleWithTypeId(type_id);
#else
			return false;
#endif
		}
		void* function::get_delegate_object() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->GetDelegateObject();
#else
			return nullptr;
#endif
		}
		type_info function::get_delegate_object_type() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->GetDelegateObjectType();
#else
			return type_info(nullptr);
#endif
		}
		function function::get_delegate_function() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->GetDelegateFunction();
#else
			return function(nullptr);
#endif
		}
		size_t function::get_properties_count() const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)ptr->GetVarCount();
#else
			return 0;
#endif
		}
		expects_vm<void> function::get_property(size_t index, std::string_view* name, int* type_id) const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			const char* asName = "";
			auto result = ptr->GetVar((asUINT)index, &asName, type_id);
			if (name != nullptr)
				*name = asName;
			return function_factory::to_return(result);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		std::string_view function::get_property_decl(size_t index, bool include_namespace) const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(ptr->GetVarDecl((asUINT)index, include_namespace));
#else
			return "";
#endif
		}
		int function::find_next_line_with_code(int line) const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->FindNextLineWithCode(line);
#else
			return -1;
#endif
		}
		void* function::set_user_data(void* user_data, size_t type)
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->SetUserData(user_data, type);
#else
			return nullptr;
#endif
		}
		void* function::get_user_data(size_t type) const
		{
			VI_ASSERT(is_valid(), "function should be valid");
#ifdef VI_ANGELSCRIPT
			return ptr->GetUserData(type);
#else
			return nullptr;
#endif
		}
		bool function::is_valid() const
		{
#ifdef VI_ANGELSCRIPT
			return vm != nullptr && ptr != nullptr;
#else
			return false;
#endif
		}
		asIScriptFunction* function::get_function() const
		{
#ifdef VI_ANGELSCRIPT
			return ptr;
#else
			return nullptr;
#endif
		}
		virtual_machine* function::get_vm() const
		{
#ifdef VI_ANGELSCRIPT
			return vm;
#else
			return nullptr;
#endif
		}

		script_object::script_object(asIScriptObject* base) noexcept : object(base)
		{
		}
		void script_object::add_ref() const
		{
			VI_ASSERT(is_valid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			object->AddRef();
#endif
		}
		void script_object::release()
		{
			if (!is_valid())
				return;
#ifdef VI_ANGELSCRIPT
			object->Release();
#endif
		}
		type_info script_object::get_object_type()
		{
			VI_ASSERT(is_valid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return object->GetObjectType();
#else
			return type_info(nullptr);
#endif
		}
		int script_object::get_type_id()
		{
			VI_ASSERT(is_valid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return object->GetTypeId();
#else
			return -1;
#endif
		}
		int script_object::get_property_type_id(size_t id) const
		{
			VI_ASSERT(is_valid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return object->GetPropertyTypeId((asUINT)id);
#else
			return -1;
#endif
		}
		size_t script_object::get_properties_count() const
		{
			VI_ASSERT(is_valid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)object->GetPropertyCount();
#else
			return 0;
#endif
		}
		std::string_view script_object::get_property_name(size_t id) const
		{
			VI_ASSERT(is_valid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(object->GetPropertyName((asUINT)id));
#else
			return "";
#endif
		}
		void* script_object::get_address_of_property(size_t id)
		{
			VI_ASSERT(is_valid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return object->GetAddressOfProperty((asUINT)id);
#else
			return nullptr;
#endif
		}
		virtual_machine* script_object::get_vm() const
		{
			VI_ASSERT(is_valid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return virtual_machine::get(object->GetEngine());
#else
			return nullptr;
#endif
		}
		int script_object::copy_from(const script_object& other)
		{
			VI_ASSERT(is_valid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return object->CopyFrom(other.get_object());
#else
			return -1;
#endif
		}
		void* script_object::set_user_data(void* data, size_t type)
		{
			VI_ASSERT(is_valid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return object->SetUserData(data, (asPWORD)type);
#else
			return nullptr;
#endif
		}
		void* script_object::get_user_data(size_t type) const
		{
			VI_ASSERT(is_valid(), "object should be valid");
#ifdef VI_ANGELSCRIPT
			return object->GetUserData((asPWORD)type);
#else
			return nullptr;
#endif
		}
		bool script_object::is_valid() const
		{
#ifdef VI_ANGELSCRIPT
			return object != nullptr;
#else
			return false;
#endif
		}
		asIScriptObject* script_object::get_object() const
		{
#ifdef VI_ANGELSCRIPT
			return object;
#else
			return nullptr;
#endif
		}

		generic_context::generic_context(asIScriptGeneric* base) noexcept : genericf(base)
		{
#ifdef VI_ANGELSCRIPT
			vm = (genericf ? virtual_machine::get(genericf->GetEngine()) : nullptr);
#endif
		}
		void* generic_context::get_object_address()
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetObject();
#else
			return nullptr;
#endif
		}
		int generic_context::get_object_type_id() const
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetObjectTypeId();
#else
			return -1;
#endif
		}
		size_t generic_context::get_args_count() const
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return (size_t)genericf->GetArgCount();
#else
			return 0;
#endif
		}
		int generic_context::get_arg_type_id(size_t argument, size_t* flags) const
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			asDWORD asFlags;
			int r = genericf->GetArgTypeId((asUINT)argument, &asFlags);
			if (flags != nullptr)
				*flags = (size_t)asFlags;
			return r;
#else
			return -1;
#endif
		}
		unsigned char generic_context::get_arg_byte(size_t argument)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetArgByte((asUINT)argument);
#else
			return 0;
#endif
		}
		unsigned short generic_context::get_arg_word(size_t argument)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetArgWord((asUINT)argument);
#else
			return 0;
#endif
		}
		size_t generic_context::get_arg_dword(size_t argument)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetArgDWord((asUINT)argument);
#else
			return 0;
#endif
		}
		uint64_t generic_context::get_arg_qword(size_t argument)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetArgQWord((asUINT)argument);
#else
			return 0;
#endif
		}
		float generic_context::get_arg_float(size_t argument)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetArgFloat((asUINT)argument);
#else
			return 0.0f;
#endif
		}
		double generic_context::get_arg_double(size_t argument)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetArgDouble((asUINT)argument);
#else
			return 0.0;
#endif
		}
		void* generic_context::get_arg_address(size_t argument)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetArgAddress((asUINT)argument);
#else
			return nullptr;
#endif
		}
		void* generic_context::get_arg_object_address(size_t argument)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetArgObject((asUINT)argument);
#else
			return nullptr;
#endif
		}
		void* generic_context::get_address_of_arg(size_t argument)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetAddressOfArg((asUINT)argument);
#else
			return nullptr;
#endif
		}
		int generic_context::get_return_type_id(size_t* flags) const
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			asDWORD asFlags;
			int type_id = genericf->GetReturnTypeId(&asFlags);
			if (flags != nullptr)
				*flags = (size_t)asFlags;

			return type_id;
#else
			return -1;
#endif
		}
		int generic_context::get_return_addressable_type_id(size_t* flags) const
		{
			int result = get_return_type_id(flags);
			if (result & (int)type_id::script_object_t)
				result |= (int)type_id::handle_t;
			return result;
		}
		expects_vm<void> generic_context::set_return_byte(unsigned char value)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(genericf->SetReturnByte(value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> generic_context::set_return_word(unsigned short value)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(genericf->SetReturnWord(value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> generic_context::set_return_dword(size_t value)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(genericf->SetReturnDWord((asDWORD)value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> generic_context::set_return_qword(uint64_t value)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(genericf->SetReturnQWord(value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> generic_context::set_return_float(float value)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(genericf->SetReturnFloat(value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> generic_context::set_return_double(double value)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(genericf->SetReturnDouble(value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> generic_context::set_return_address(void* address)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(genericf->SetReturnAddress(address));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> generic_context::set_return_object_address(void* object)
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(genericf->SetReturnObject(object));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		void* generic_context::get_address_of_return_location()
		{
			VI_ASSERT(is_valid(), "generic should be valid");
#ifdef VI_ANGELSCRIPT
			return genericf->GetAddressOfReturnLocation();
#else
			return nullptr;
#endif
		}
		bool generic_context::is_valid() const
		{
#ifdef VI_ANGELSCRIPT
			return vm != nullptr && genericf != nullptr;
#else
			return false;
#endif
		}
		asIScriptGeneric* generic_context::get_generic() const
		{
#ifdef VI_ANGELSCRIPT
			return genericf;
#else
			return nullptr;
#endif
		}
		virtual_machine* generic_context::get_vm() const
		{
#ifdef VI_ANGELSCRIPT
			return vm;
#else
			return nullptr;
#endif
		}

		base_class::base_class(virtual_machine* engine, asITypeInfo* source, int type) noexcept : vm(engine), type(source), type_id(type)
		{
		}
		expects_vm<void> base_class::set_function_def(const std::string_view& decl)
		{
			VI_ASSERT(is_valid(), "class should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " funcdef %i bytes", (void*)this, (int)decl.size());
			return function_factory::to_return(engine->RegisterFuncdef(decl.data()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> base_class::set_operator_copy_address(asSFuncPtr* value, convention type)
		{
			VI_ASSERT(is_valid(), "class should be valid");
			VI_ASSERT(core::stringify::is_cstring(get_type_name()), "typename should be set");
			VI_ASSERT(value != nullptr, "value should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");

			core::string decl = core::stringify::text("%s& opAssign(const %s &in)", get_type_name().data(), get_type_name().data());
			VI_TRACE("[vm] register class 0x%" PRIXPTR " op-copy funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)type, (int)decl.size(), (void*)value);
			return function_factory::to_return(engine->RegisterObjectMethod(get_type_name().data(), decl.c_str(), *value, (asECallConvTypes)type));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> base_class::set_behaviour_address(const std::string_view& decl, behaviours behave, asSFuncPtr* value, convention type)
		{
			VI_ASSERT(is_valid(), "class should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(core::stringify::is_cstring(get_type_name()), "typename should be set");
			VI_ASSERT(value != nullptr, "value should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " behaviour funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)type, (int)decl.size(), (void*)value);
			return function_factory::to_return(engine->RegisterObjectBehaviour(get_type_name().data(), (asEBehaviours)behave, decl.data(), *value, (asECallConvTypes)type));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> base_class::set_property_address(const std::string_view& decl, int offset)
		{
			VI_ASSERT(is_valid(), "class should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(core::stringify::is_cstring(get_type_name()), "typename should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " property %i bytes at 0x0+%i", (void*)this, (int)decl.size(), offset);
			return function_factory::to_return(engine->RegisterObjectProperty(get_type_name().data(), decl.data(), offset));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> base_class::set_property_static_address(const std::string_view& decl, void* value)
		{
			VI_ASSERT(is_valid(), "class should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(core::stringify::is_cstring(get_type_name()), "typename should be set");
			VI_ASSERT(value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " static property %i bytes at 0x%" PRIXPTR, (void*)this, (int)decl.size(), (void*)value);
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");
			const char* name_space = engine->GetDefaultNamespace();
			const char* scope = type->GetNamespace();

			engine->SetDefaultNamespace(scope[0] == '\0' ? get_type_name().data() : core::string(scope).append("::").append(get_name()).c_str());
			int r = engine->RegisterGlobalProperty(decl.data(), value);
			engine->SetDefaultNamespace(name_space);

			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> base_class::set_operator_address(const std::string_view& decl, asSFuncPtr* value, convention type)
		{
			return set_method_address(decl, value, type);
		}
		expects_vm<void> base_class::set_method_address(const std::string_view& decl, asSFuncPtr* value, convention type)
		{
			VI_ASSERT(is_valid(), "class should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(core::stringify::is_cstring(get_type_name()), "typename should be set");
			VI_ASSERT(value != nullptr, "value should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)type, (int)decl.size(), (void*)value);
			return function_factory::to_return(engine->RegisterObjectMethod(get_type_name().data(), decl.data(), *value, (asECallConvTypes)type));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> base_class::set_method_static_address(const std::string_view& decl, asSFuncPtr* value, convention type)
		{
			VI_ASSERT(is_valid(), "class should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(core::stringify::is_cstring(get_type_name()), "typename should be set");
			VI_ASSERT(value != nullptr, "value should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " static funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)type, (int)decl.size(), (void*)value);

			const char* name_space = engine->GetDefaultNamespace();
			const char* scope = this->type->GetNamespace();
			engine->SetDefaultNamespace(scope[0] == '\0' ? get_type_name().data() : core::string(scope).append("::").append(get_name()).c_str());
			int r = engine->RegisterGlobalFunction(decl.data(), *value, (asECallConvTypes)type);
			engine->SetDefaultNamespace(name_space);

			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> base_class::set_constructor_address(const std::string_view& decl, asSFuncPtr* value, convention type)
		{
			VI_ASSERT(is_valid(), "class should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(core::stringify::is_cstring(get_type_name()), "typename should be set");
			VI_ASSERT(value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " constructor funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)type, (int)decl.size(), (void*)value);
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");
			return function_factory::to_return(engine->RegisterObjectBehaviour(get_type_name().data(), asBEHAVE_CONSTRUCT, decl.data(), *value, (asECallConvTypes)type));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> base_class::set_constructor_list_address(const std::string_view& decl, asSFuncPtr* value, convention type)
		{
			VI_ASSERT(is_valid(), "class should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(core::stringify::is_cstring(get_type_name()), "typename should be set");
			VI_ASSERT(value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " list-constructor funcaddr(%i) %i bytes at 0x%" PRIXPTR, (void*)this, (int)type, (int)decl.size(), (void*)value);
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");
			return function_factory::to_return(engine->RegisterObjectBehaviour(get_type_name().data(), asBEHAVE_LIST_CONSTRUCT, decl.data(), *value, (asECallConvTypes)type));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> base_class::set_destructor_address(const std::string_view& decl, asSFuncPtr* value)
		{
			VI_ASSERT(is_valid(), "class should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(core::stringify::is_cstring(get_type_name()), "typename should be set");
			VI_ASSERT(value != nullptr, "value should be set");
			VI_TRACE("[vm] register class 0x%" PRIXPTR " destructor funcaddr %i bytes at 0x%" PRIXPTR, (void*)this, (int)decl.size(), (void*)value);
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");
			return function_factory::to_return(engine->RegisterObjectBehaviour(get_type_name().data(), asBEHAVE_DESTRUCT, decl.data(), *value, asCALL_CDECL_OBJFIRST));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		asITypeInfo* base_class::get_type_info() const
		{
			return type;
		}
		int base_class::get_type_id() const
		{
			return type_id;
		}
		bool base_class::is_valid() const
		{
			return vm != nullptr && type_id >= 0 && type != nullptr;
		}
		std::string_view base_class::get_type_name() const
		{
#ifdef VI_ANGELSCRIPT
			return or_empty(type->GetName());
#else
			return "";
#endif
		}
		core::string base_class::get_name() const
		{
			return core::string(get_type_name());
		}
		virtual_machine* base_class::get_vm() const
		{
			return vm;
		}
		core::string base_class::get_operator(operators op, const std::string_view& out, const std::string_view& args, bool constant, bool right)
		{
			VI_ASSERT(core::stringify::is_cstring(out), "out should be set");
			VI_ASSERT(core::stringify::is_cstring(args), "args should be set");
			switch (op)
			{
				case operators::neg_t:
					if (right)
						return "";

					return core::stringify::text("%s opNeg()%s", out.data(), constant ? " const" : "");
				case operators::com_t:
					if (right)
						return "";

					return core::stringify::text("%s opCom()%s", out.data(), constant ? " const" : "");
				case operators::pre_inc_t:
					if (right)
						return "";

					return core::stringify::text("%s opPreInc()%s", out.data(), constant ? " const" : "");
				case operators::pre_dec_t:
					if (right)
						return "";

					return core::stringify::text("%s opPreDec()%s", out.data(), constant ? " const" : "");
				case operators::post_inc_t:
					if (right)
						return "";

					return core::stringify::text("%s opPostInc()%s", out.data(), constant ? " const" : "");
				case operators::post_dec_t:
					if (right)
						return "";

					return core::stringify::text("%s opPostDec()%s", out.data(), constant ? " const" : "");
				case operators::equals_t:
					if (right)
						return "";

					return core::stringify::text("%s opEquals(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::cmp_t:
					if (right)
						return "";

					return core::stringify::text("%s opCmp(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::add_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opAddAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::sub_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opSubAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::mul_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opMulAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::div_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opDivAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::mod_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opModAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::pow_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opPowAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::and_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opAndAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::or_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opOrAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::xor_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opXorAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::shl_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opShlAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::shr_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opShrAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::ushr_assign_t:
					if (right)
						return "";

					return core::stringify::text("%s opUshrAssign(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::add_t:
					return core::stringify::text("%s opAdd%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::sub_t:
					return core::stringify::text("%s opSub%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::mul_t:
					return core::stringify::text("%s opMul%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::div_t:
					return core::stringify::text("%s opDiv%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::mod_t:
					return core::stringify::text("%s opMod%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::pow_t:
					return core::stringify::text("%s opPow%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::and_t:
					return core::stringify::text("%s opAnd%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::or_t:
					return core::stringify::text("%s opOr%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::xor_t:
					return core::stringify::text("%s opXor%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::shl_t:
					return core::stringify::text("%s opShl%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::shr_t:
					return core::stringify::text("%s opShr%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::ushr_t:
					return core::stringify::text("%s opUshr%s(%s)%s", out.data(), right ? "_r" : "", args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::index_t:
					if (right)
						return "";

					return core::stringify::text("%s opIndex(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::call_t:
					if (right)
						return "";

					return core::stringify::text("%s opCall(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::cast_t:
					if (right)
						return "";

					return core::stringify::text("%s opCast(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				case operators::impl_cast_t:
					if (right)
						return "";

					return core::stringify::text("%s opImplCast(%s)%s", out.data(), args.empty() ? "" : args.data(), constant ? " const" : "");
				default:
					return "";
			}
		}

		type_interface::type_interface(virtual_machine* engine, asITypeInfo* source, int type) noexcept : vm(engine), type(source), type_id(type)
		{
		}
		expects_vm<void> type_interface::set_method(const std::string_view& decl)
		{
			VI_ASSERT(is_valid(), "interface should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(core::stringify::is_cstring(get_type_name()), "typename should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register interface 0x%" PRIXPTR " method %i bytes", (void*)this, (int)decl.size());
			return function_factory::to_return(engine->RegisterInterfaceMethod(get_type_name().data(), decl.data()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		asITypeInfo* type_interface::get_type_info() const
		{
			return type;
		}
		int type_interface::get_type_id() const
		{
			return type_id;
		}
		bool type_interface::is_valid() const
		{
			return vm != nullptr && type_id >= 0 && type != nullptr;
		}
		std::string_view type_interface::get_type_name() const
		{
#ifdef VI_ANGELSCRIPT
			return or_empty(type->GetName());
#else
			return "";
#endif
		}
		core::string type_interface::get_name() const
		{
			return core::string(get_type_name());
		}
		virtual_machine* type_interface::get_vm() const
		{
			return vm;
		}

		enumeration::enumeration(virtual_machine* engine, asITypeInfo* source, int type) noexcept : vm(engine), type(source), type_id(type)
		{
		}
		expects_vm<void> enumeration::set_value(const std::string_view& name, int value)
		{
			VI_ASSERT(is_valid(), "enum should be valid");
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
			VI_ASSERT(core::stringify::is_cstring(get_type_name()), "typename should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register enum 0x%" PRIXPTR " value %i bytes = %i", (void*)this, (int)name.size(), value);
			return function_factory::to_return(engine->RegisterEnumValue(get_type_name().data(), name.data(), value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		asITypeInfo* enumeration::get_type_info() const
		{
			return type;
		}
		int enumeration::get_type_id() const
		{
			return type_id;
		}
		bool enumeration::is_valid() const
		{
			return vm != nullptr && type_id >= 0 && type != nullptr;
		}
		std::string_view enumeration::get_type_name() const
		{
#ifdef VI_ANGELSCRIPT
			return or_empty(type->GetName());
#else
			return "";
#endif
		}
		core::string enumeration::get_name() const
		{
			return core::string(get_type_name());
		}
		virtual_machine* enumeration::get_vm() const
		{
			return vm;
		}

		library::library(asIScriptModule* type) noexcept : mod(type)
		{
#ifdef VI_ANGELSCRIPT
			vm = (mod ? virtual_machine::get(mod->GetEngine()) : nullptr);
#endif
		}
		void library::set_name(const std::string_view& name)
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
#ifdef VI_ANGELSCRIPT
			mod->SetName(name.data());
#endif
		}
		expects_vm<void> library::add_section(const std::string_view& name, const std::string_view& code, int line_offset)
		{
			VI_ASSERT(is_valid(), "module should be valid");
			return vm->add_script_section(mod, name, code, line_offset);
		}
		expects_vm<void> library::remove_function(const function& function)
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(mod->RemoveFunction(function.get_function()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::reset_properties(asIScriptContext* context)
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(mod->ResetGlobalVars(context));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::build()
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			int r = mod->Build();
			if (r != asBUILD_IN_PROGRESS)
				vm->clear_sections();
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::load_byte_code(byte_code_info* info)
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			cbyte_code_stream* stream = core::memory::init<cbyte_code_stream>(info->data);
			int r = mod->LoadByteCode(stream, &info->debug);
			core::memory::deinit(stream);
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::bind_imported_function(size_t import_index, const function& function)
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(mod->BindImportedFunction((asUINT)import_index, function.get_function()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::unbind_imported_function(size_t import_index)
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(mod->UnbindImportedFunction((asUINT)import_index));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::bind_all_imported_functions()
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(mod->BindAllImportedFunctions());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::unbind_all_imported_functions()
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(mod->UnbindAllImportedFunctions());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::compile_function(const std::string_view& section_name, const std::string_view& code, int line_offset, size_t compile_flags, function* out_function)
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(code), "code should be set");
			VI_ASSERT(core::stringify::is_cstring(section_name), "section name should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptFunction* out_func = nullptr;
			int r = mod->CompileFunction(section_name.data(), code.data(), line_offset, (asDWORD)compile_flags, &out_func);
			if (out_function != nullptr)
				*out_function = function(out_func);
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::compile_property(const std::string_view& section_name, const std::string_view& code, int line_offset)
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(code), "code should be set");
			VI_ASSERT(core::stringify::is_cstring(section_name), "section name should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(mod->CompileGlobalVar(section_name.data(), code.data(), line_offset));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::set_default_namespace(const std::string_view& name_space)
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(name_space), "namespace should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(mod->SetDefaultNamespace(name_space.data()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::remove_property(size_t index)
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(mod->RemoveGlobalVar((asUINT)index));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		void library::discard()
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			mod->Discard();
			mod = nullptr;
#endif
		}
		void* library::get_address_of_property(size_t index)
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return mod->GetAddressOfGlobalVar((asUINT)index);
#else
			return nullptr;
#endif
		}
		size_t library::set_access_mask(size_t access_mask)
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return mod->SetAccessMask((asDWORD)access_mask);
#else
			return 0;
#endif
		}
		size_t library::get_function_count() const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return mod->GetFunctionCount();
#else
			return 0;
#endif
		}
		function library::get_function_by_index(size_t index) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return mod->GetFunctionByIndex((asUINT)index);
#else
			return function(nullptr);
#endif
		}
		function library::get_function_by_decl(const std::string_view& decl) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return mod->GetFunctionByDecl(decl.data());
#else
			return function(nullptr);
#endif
		}
		function library::get_function_by_name(const std::string_view& name) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
#ifdef VI_ANGELSCRIPT
			return mod->GetFunctionByName(name.data());
#else
			return function(nullptr);
#endif
		}
		int library::get_type_id_by_decl(const std::string_view& decl) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return mod->GetTypeIdByDecl(decl.data());
#else
			return -1;
#endif
		}
		expects_vm<size_t> library::get_imported_function_index_by_decl(const std::string_view& decl) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			int r = mod->GetImportedFunctionIndexByDecl(decl.data());
			return function_factory::to_return<size_t>(r, (size_t)r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::save_byte_code(byte_code_info* info) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			cbyte_code_stream* stream = core::memory::init<cbyte_code_stream>();
			int r = mod->SaveByteCode(stream, info->debug);
			info->data = stream->GetCode();
			core::memory::deinit(stream);
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<size_t> library::get_property_index_by_name(const std::string_view& name) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
#ifdef VI_ANGELSCRIPT
			int r = mod->GetGlobalVarIndexByName(name.data());
			return function_factory::to_return<size_t>(r, (size_t)r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<size_t> library::get_property_index_by_decl(const std::string_view& decl) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			int r = mod->GetGlobalVarIndexByDecl(decl.data());
			return function_factory::to_return<size_t>(r, (size_t)r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> library::get_property(size_t index, property_info* info) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			const char* name = "";
			const char* name_space = "";
			bool is_const = false;
			int type_id = 0;
			int result = mod->GetGlobalVar((asUINT)index, &name, &name_space, &type_id, &is_const);

			if (info != nullptr)
			{
				info->name = or_empty(name);
				info->name_space = or_empty(name_space);
				info->type_id = type_id;
				info->is_const = is_const;
				info->config_group = "";
				info->pointer = mod->GetAddressOfGlobalVar((asUINT)index);
				info->access_mask = get_access_mask();
			}

			return function_factory::to_return(result);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		size_t library::get_access_mask() const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			asDWORD access_mask = mod->SetAccessMask(0);
			mod->SetAccessMask(access_mask);
			return (size_t)access_mask;
#else
			return 0;
#endif
		}
		size_t library::get_objects_count() const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return mod->GetObjectTypeCount();
#else
			return 0;
#endif
		}
		size_t library::get_properties_count() const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return mod->GetGlobalVarCount();
#else
			return 0;
#endif
		}
		size_t library::get_enums_count() const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return mod->GetEnumCount();
#else
			return 0;
#endif
		}
		size_t library::get_imported_function_count() const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return mod->GetImportedFunctionCount();
#else
			return 0;
#endif
		}
		type_info library::get_object_by_index(size_t index) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return mod->GetObjectTypeByIndex((asUINT)index);
#else
			return type_info(nullptr);
#endif
		}
		type_info library::get_type_info_by_name(const std::string_view& name) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
#ifdef VI_ANGELSCRIPT
			std::string_view type_name = name;
			std::string_view name_space = "";
			if (!vm->get_type_name_scope(&type_name, &name_space))
				return mod->GetTypeInfoByName(name.data());

			VI_ASSERT(core::stringify::is_cstring(type_name), "typename should be set");
			vm->begin_namespace(core::string(name_space));
			asITypeInfo* info = mod->GetTypeInfoByName(type_name.data());
			vm->end_namespace();

			return info;
#else
			return type_info(nullptr);
#endif
		}
		type_info library::get_type_info_by_decl(const std::string_view& decl) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return mod->GetTypeInfoByDecl(decl.data());
#else
			return type_info(nullptr);
#endif
		}
		type_info library::get_enum_by_index(size_t index) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return mod->GetEnumByIndex((asUINT)index);
#else
			return type_info(nullptr);
#endif
		}
		std::string_view library::get_property_decl(size_t index, bool include_namespace) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(mod->GetGlobalVarDeclaration((asUINT)index, include_namespace));
#else
			return "";
#endif
		}
		std::string_view library::get_default_namespace() const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(mod->GetDefaultNamespace());
#else
			return "";
#endif
		}
		std::string_view library::get_imported_function_decl(size_t import_index) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(mod->GetImportedFunctionDeclaration((asUINT)import_index));
#else
			return "";
#endif
		}
		std::string_view library::get_imported_function_module(size_t import_index) const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(mod->GetImportedFunctionSourceModule((asUINT)import_index));
#else
			return "";
#endif
		}
		std::string_view library::get_name() const
		{
			VI_ASSERT(is_valid(), "module should be valid");
#ifdef VI_ANGELSCRIPT
			return or_empty(mod->GetName());
#else
			return "";
#endif
		}
		bool library::is_valid() const
		{
#ifdef VI_ANGELSCRIPT
			return vm != nullptr && mod != nullptr;
#else
			return false;
#endif
		}
		asIScriptModule* library::get_module() const
		{
#ifdef VI_ANGELSCRIPT
			return mod;
#else
			return nullptr;
#endif
		}
		virtual_machine* library::get_vm() const
		{
#ifdef VI_ANGELSCRIPT
			return vm;
#else
			return nullptr;
#endif
		}

		function_delegate::function_delegate() : context(nullptr), callback(nullptr), delegate_type(nullptr), delegate_object(nullptr)
		{
		}
		function_delegate::function_delegate(const function& function) : context(nullptr), callback(nullptr), delegate_type(nullptr), delegate_object(nullptr)
		{
			if (!function.is_valid())
				return;
#ifdef VI_ANGELSCRIPT
			context = immediate_context::get();
			callback = function.get_function();
			delegate_type = callback->GetDelegateObjectType();
			delegate_object = callback->GetDelegateObject();
			add_ref_and_initialize(true);
#endif
		}
		function_delegate::function_delegate(const function& function, immediate_context* wanted_context) : context(wanted_context), callback(nullptr), delegate_type(nullptr), delegate_object(nullptr)
		{
			if (!function.is_valid())
				return;
#ifdef VI_ANGELSCRIPT
			if (!context)
				context = immediate_context::get();

			callback = function.get_function();
			delegate_type = callback->GetDelegateObjectType();
			delegate_object = callback->GetDelegateObject();
			add_ref_and_initialize(true);
#endif
		}
		function_delegate::function_delegate(const function_delegate& other) : context(other.context), callback(other.callback), delegate_type(other.delegate_type), delegate_object(other.delegate_object)
		{
			add_ref();
		}
		function_delegate::function_delegate(function_delegate&& other) : context(other.context), callback(other.callback), delegate_type(other.delegate_type), delegate_object(other.delegate_object)
		{
			other.context = nullptr;
			other.callback = nullptr;
			other.delegate_type = nullptr;
			other.delegate_object = nullptr;
		}
		function_delegate::~function_delegate()
		{
			release();
		}
		function_delegate& function_delegate::operator= (const function_delegate& other)
		{
			if (this == &other)
				return *this;

			release();
			context = other.context;
			callback = other.callback;
			delegate_type = other.delegate_type;
			delegate_object = other.delegate_object;
			add_ref();

			return *this;
		}
		function_delegate& function_delegate::operator= (function_delegate&& other)
		{
			if (this == &other)
				return *this;

			context = other.context;
			callback = other.callback;
			delegate_type = other.delegate_type;
			delegate_object = other.delegate_object;
			other.context = nullptr;
			other.callback = nullptr;
			other.delegate_type = nullptr;
			other.delegate_object = nullptr;
			return *this;
		}
		expects_promise_vm<execution> function_delegate::operator()(args_callback&& on_args, args_callback&& on_return)
		{
			if (!is_valid())
				return expects_vm<execution>(virtual_error::invalid_configuration);

			function_delegate copy = *this;
			return context->resolve_callback(std::move(copy), std::move(on_args), std::move(on_return));
		}
		void function_delegate::add_ref_and_initialize(bool is_first_time)
		{
			if (!is_valid())
				return;
#ifdef VI_ANGELSCRIPT
			if (delegate_object != nullptr)
				context->get_vm()->add_ref_object(delegate_object, delegate_type);

			if (!is_first_time)
				callback->AddRef();
			context->add_ref();
#endif
		}
		void function_delegate::add_ref()
		{
			add_ref_and_initialize(false);
		}
		void function_delegate::release()
		{
			if (!is_valid())
				return;
#ifdef VI_ANGELSCRIPT
			if (delegate_object != nullptr)
				context->get_vm()->release_object(delegate_object, delegate_type);

			delegate_object = nullptr;
			delegate_type = nullptr;
			if (callback != nullptr)
				callback->Release();
			core::memory::release(context);
#endif
		}
		bool function_delegate::is_valid() const
		{
			return context && callback;
		}
		function function_delegate::callable() const
		{
			return callback;
		}

		compiler::compiler(virtual_machine* engine) noexcept : scope(nullptr), vm(engine), built(false)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");

			processor = new compute::preprocessor();
			processor->set_include_callback([this](compute::preprocessor* processor, const compute::include_result& file, core::string& output) -> compute::expects_preprocessor<compute::include_type>
			{
				VI_ASSERT(vm != nullptr, "engine should be set");
				if (include)
				{
					auto status = include(processor, file, output);
					switch (status.or_else(compute::include_type::unchanged))
					{
						case compute::include_type::preprocess:
							goto preprocess;
						case compute::include_type::computed:
							return compute::include_type::computed;
						case compute::include_type::error:
							return compute::include_type::error;
						case compute::include_type::unchanged:
						default:
							break;
					}
				}

				if (file.library.empty() || !scope)
					return compute::include_type::error;

				if (!file.is_file && file.is_abstract)
					return vm->import_system_addon(file.library) ? compute::include_type::computed : compute::include_type::error;

				if (!vm->import_file(file.library, file.is_remote, output))
					return compute::include_type::error;

			preprocess:
				if (output.empty())
					return compute::include_type::computed;

				auto status = vm->generate_code(processor, file.library, output);
				if (!status)
					return status.error();
				else if (output.empty())
					return compute::include_type::computed;

				return vm->add_script_section(scope, file.library, output) ? compute::include_type::computed : compute::include_type::error;
			});
			processor->set_pragma_callback([this](compute::preprocessor* processor, const std::string_view& name, const core::vector<core::string>& args) -> compute::expects_preprocessor<void>
			{
				VI_ASSERT(vm != nullptr, "engine should be set");
				if (pragma)
				{
					auto status = pragma(processor, name, args);
					if (status)
						return status;
				}

				if (name == "compile" && args.size() == 2)
				{
					const core::string& key = args[0];
					const core::string& value = args[1];
					auto numeric = core::from_string<uint64_t>(value);
					size_t result = numeric ? (size_t)*numeric : 0;
					if (key == "allow_unsafe_references")
						vm->set_property(features::allow_unsafe_references, result);
					else if (key == "optimize_bytecode")
						vm->set_property(features::optimize_bytecode, result);
					else if (key == "copy_script_sections")
						vm->set_property(features::copy_script_sections, result);
					else if (key == "max_stack_size")
						vm->set_property(features::max_stack_size, result);
					else if (key == "use_character_literals")
						vm->set_property(features::use_character_literals, result);
					else if (key == "allow_multiline_strings")
						vm->set_property(features::allow_multiline_strings, result);
					else if (key == "allow_implicit_handle_types")
						vm->set_property(features::allow_implicit_handle_types, result);
					else if (key == "build_without_line_cues")
						vm->set_property(features::build_without_line_cues, result);
					else if (key == "init_global_vars_after_build")
						vm->set_property(features::init_global_vars_after_build, result);
					else if (key == "require_enum_scope")
						vm->set_property(features::require_enum_scope, result);
					else if (key == "script_scanner")
						vm->set_property(features::script_scanner, result);
					else if (key == "include_jit_instructions")
						vm->set_property(features::include_jit_instructions, result);
					else if (key == "string_encoding")
						vm->set_property(features::string_encoding, result);
					else if (key == "property_accessor_mode")
						vm->set_property(features::property_accessor_mode, result);
					else if (key == "expand_def_array_to_impl")
						vm->set_property(features::expand_def_array_to_impl, result);
					else if (key == "auto_garbage_collect")
						vm->set_property(features::auto_garbage_collect, result);
					else if (key == "always_impl_default_construct")
						vm->set_property(features::always_impl_default_construct, result);
					else if (key == "always_impl_default_construct")
						vm->set_property(features::always_impl_default_construct, result);
					else if (key == "compiler_warnings")
						vm->set_property(features::compiler_warnings, result);
					else if (key == "disallow_value_assign_for_ref_type")
						vm->set_property(features::disallow_value_assign_for_ref_type, result);
					else if (key == "alter_syntax_named_args")
						vm->set_property(features::alter_syntax_named_args, result);
					else if (key == "disable_integer_division")
						vm->set_property(features::disable_integer_division, result);
					else if (key == "disallow_empty_list_elements")
						vm->set_property(features::disallow_empty_list_elements, result);
					else if (key == "private_prop_as_protected")
						vm->set_property(features::private_prop_as_protected, result);
					else if (key == "allow_unicode_identifiers")
						vm->set_property(features::allow_unicode_identifiers, result);
					else if (key == "heredoc_trim_mode")
						vm->set_property(features::heredoc_trim_mode, result);
					else if (key == "max_nested_calls")
						vm->set_property(features::max_nested_calls, result);
					else if (key == "generic_call_mode")
						vm->set_property(features::generic_call_mode, result);
					else if (key == "init_stack_size")
						vm->set_property(features::init_stack_size, result);
					else if (key == "init_call_stack_size")
						vm->set_property(features::init_call_stack_size, result);
					else if (key == "max_call_stack_size")
						vm->set_property(features::max_call_stack_size, result);
					else if (key == "ignore_duplicate_shared_int")
						vm->set_property(features::ignore_duplicate_shared_int, result);
					else if (key == "no_debug_output")
						vm->set_property(features::no_debug_output, result);
					else if (key == "disable_script_class_gc")
						vm->set_property(features::disable_script_class_gc, result);
					else if (key == "jit_interface_version")
						vm->set_property(features::jit_interface_version, result);
					else if (key == "always_impl_default_copy")
						vm->set_property(features::always_impl_default_copy, result);
					else if (key == "always_impl_default_copy_construct")
						vm->set_property(features::always_impl_default_copy_construct, result);
					else if (key == "member_init_mode")
						vm->set_property(features::member_init_mode, result);
					else if (key == "bool_conversion_mode")
						vm->set_property(features::bool_conversion_mode, result);
					else if (key == "foreach_support")
						vm->set_property(features::foreach_support, result);
				}
				else if (name == "comment" && args.size() == 2)
				{
					const core::string& key = args[0];
					if (key == "info")
						VI_INFO("[asc] %s", args[1].c_str());
					else if (key == "trace")
						VI_DEBUG("[asc] %s", args[1].c_str());
					else if (key == "warn")
						VI_WARN("[asc] %s", args[1].c_str());
					else if (key == "err")
						VI_ERR("[asc] %s", args[1].c_str());
				}
				else if (name == "modify" && args.size() == 2)
				{
#ifdef VI_ANGELSCRIPT
					const core::string& key = args[0];
					const core::string& value = args[1];
					auto numeric = core::from_string<uint64_t>(value);
					size_t result = numeric ? (size_t)*numeric : 0;
					if (key == "name")
						scope->SetName(value.c_str());
					else if (key == "namespace")
						scope->SetDefaultNamespace(value.c_str());
					else if (key == "access_mask")
						scope->SetAccessMask((asDWORD)result);
#endif
				}
				else if (name == "cimport" && args.size() >= 2)
				{
					if (args.size() == 3)
					{
						auto status = vm->import_clibrary(args[0]);
						if (status)
						{
							status = vm->import_cfunction({ args[0] }, args[1], args[2]);
							if (status)
								define("SOF_" + args[1]);
							else
								VI_ERR("[asc] %s", status.error().what());
						}
						else
							VI_ERR("[asc] %s", status.error().what());
					}
					else
					{
						auto status = vm->import_cfunction({ }, args[0], args[1]);
						if (status)
							define("SOF_" + args[1]);
						else
							VI_ERR("[asc] %s", status.error().what());
					}
				}
				else if (name == "clibrary" && args.size() >= 1)
				{
					core::string directory = core::os::path::get_directory(processor->get_current_file_path().c_str());
					if (directory.empty())
					{
						auto working = core::os::directory::get_working();
						if (working)
							directory = *working;
					}

					if (!vm->import_clibrary(args[0]))
					{
						auto path = core::os::path::resolve(args[0], directory, false);
						if (path && vm->import_clibrary(*path))
						{
							if (args.size() == 2 && !args[1].empty())
								define("SOL_" + args[1]);
						}
					}
					else if (args.size() == 2 && !args[1].empty())
						define("SOL_" + args[1]);

				}
				else if (name == "define" && args.size() == 1)
					define(args[0]);

				return core::expectation::met;
			});
			processor->define("VI_VERSION " + core::to_string((size_t)vitex::version));
#ifdef VI_MICROSOFT
			processor->define("OS_MICROSOFT");
#elif defined(VI_ANDROID)
			processor->define("OS_ANDROID");
			processor->define("OS_LINUX");
#elif defined(VI_APPLE)
			processor->define("OS_APPLE");
			processor->define("OS_LINUX");
#ifdef VI_IOS
			processor->define("OS_IOS");
#elif defined(VI_OSX)
			processor->define("OS_OSX");
#endif
#elif defined(VI_LINUX)
			processor->define("OS_LINUX");
#endif
			vm->set_processor_options(processor);
		}
		compiler::~compiler() noexcept
		{
#ifdef VI_ANGELSCRIPT
			if (scope != nullptr)
				scope->Discard();
#endif
			core::memory::release(processor);
		}
		void compiler::set_include_callback(compute::proc_include_callback&& callback)
		{
			include = std::move(callback);
		}
		void compiler::set_pragma_callback(compute::proc_pragma_callback&& callback)
		{
			pragma = std::move(callback);
		}
		void compiler::define(const std::string_view& word)
		{
			processor->define(word);
		}
		void compiler::undefine(const std::string_view& word)
		{
			processor->undefine(word);
		}
		library compiler::unlink_module()
		{
			library result(scope);
			scope = nullptr;
			return result;
		}
		bool compiler::clear()
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			if (scope != nullptr)
				scope->Discard();
#endif
			if (processor != nullptr)
				processor->clear();

			scope = nullptr;
			built = false;
			return true;
		}
		bool compiler::is_defined(const std::string_view& word) const
		{
			return processor->is_defined(word);
		}
		bool compiler::is_built() const
		{
			return built;
		}
		bool compiler::is_cached() const
		{
			return vcache.valid;
		}
		expects_vm<void> compiler::prepare(byte_code_info* info)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			if (!info->valid || info->data.empty())
				return virtual_exception(virtual_error::invalid_arg);

			auto result = prepare(info->name, true);
			if (result)
				vcache = *info;
			return result;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> compiler::prepare(const library& info)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(info.is_valid(), "module should be set");
#ifdef VI_ANGELSCRIPT
			if (!info.is_valid())
				return virtual_exception(virtual_error::invalid_arg);

			built = true;
			vcache.valid = false;
			vcache.name.clear();
			if (scope != nullptr)
				scope->Discard();

			scope = info.get_module();
			vm->set_processor_options(processor);
			return core::expectation::met;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> compiler::prepare(const std::string_view& module_name, bool scoped)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(!module_name.empty(), "module name should not be empty");
			VI_DEBUG("[vm] prepare %.*s on 0x%" PRIXPTR, (int)module_name.size(), module_name.data(), (uintptr_t)this);
#ifdef VI_ANGELSCRIPT
			built = false;
			vcache.valid = false;
			vcache.name.clear();
			if (scope != nullptr)
				scope->Discard();

			scope = scoped ? vm->create_scoped_module(module_name) : vm->create_module(module_name);
			if (!scope)
				return virtual_exception(virtual_error::invalid_name);

			vm->set_processor_options(processor);
			return core::expectation::met;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> compiler::prepare(const std::string_view& module_name, const std::string_view& name, bool debug, bool scoped)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			auto result = prepare(module_name, scoped);
			if (!result)
				return result;

			vcache.name = name;
			if (!vm->get_byte_code_cache(&vcache))
			{
				vcache.data.clear();
				vcache.debug = debug;
				vcache.valid = false;
			}

			return result;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> compiler::save_byte_code(byte_code_info* info)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(scope != nullptr, "module should not be empty");
			VI_ASSERT(info != nullptr, "bytecode should be set");
			VI_ASSERT(built, "module should be built");
#ifdef VI_ANGELSCRIPT
			cbyte_code_stream* stream = core::memory::init<cbyte_code_stream>();
			int r = scope->SaveByteCode(stream, !info->debug);
			info->data = stream->GetCode();
			core::memory::deinit(stream);
			if (r >= 0)
				VI_DEBUG("[vm] OK save bytecode on 0x%" PRIXPTR, (uintptr_t)this);
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> compiler::load_file(const std::string_view& path)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(scope != nullptr, "module should not be empty");
#ifdef VI_ANGELSCRIPT
			if (vcache.valid)
				return core::expectation::met;

			auto source = core::os::path::resolve(path);
			if (!source)
				return virtual_exception("path not found: " + core::string(path));

			if (!core::os::file::is_exists(source->c_str()))
				return virtual_exception("file not found: " + core::string(path));

			auto buffer = core::os::file::read_as_string(*source);
			if (!buffer)
				return virtual_exception("open file error: " + core::string(path));

			core::string code = *buffer;
			auto status = vm->generate_code(processor, *source, code);
			if (!status)
				return virtual_exception(std::move(status.error().message()));
			else if (code.empty())
				return core::expectation::met;

			auto result = vm->add_script_section(scope, *source, code);
			if (result)
				VI_DEBUG("[vm] OK load program on 0x%" PRIXPTR " (file)", (uintptr_t)this);
			return result;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> compiler::load_code(const std::string_view& name, const std::string_view& data)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(scope != nullptr, "module should not be empty");
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
#ifdef VI_ANGELSCRIPT
			if (vcache.valid)
				return core::expectation::met;

			core::string buffer(data);
			auto status = vm->generate_code(processor, name, buffer);
			if (!status)
				return virtual_exception(std::move(status.error().message()));
			else if (buffer.empty())
				return core::expectation::met;

			auto result = vm->add_script_section(scope, name, buffer);
			if (result)
				VI_DEBUG("[vm] OK load program on 0x%" PRIXPTR, (uintptr_t)this);
			return result;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> compiler::load_byte_code_sync(byte_code_info* info)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(scope != nullptr, "module should not be empty");
			VI_ASSERT(info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			int r = 0;
			cbyte_code_stream* stream = core::memory::init<cbyte_code_stream>(info->data);
			while ((r = scope->LoadByteCode(stream, &info->debug)) == asBUILD_IN_PROGRESS)
				std::this_thread::sleep_for(std::chrono::microseconds(COMPILER_BLOCKED_WAIT_US));
			core::memory::deinit(stream);
			if (r >= 0)
				VI_DEBUG("[vm] OK load bytecode on 0x%" PRIXPTR, (uintptr_t)this);
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> compiler::compile_sync()
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(scope != nullptr, "module should not be empty");
#ifdef VI_ANGELSCRIPT
			if (vcache.valid)
				return load_byte_code_sync(&vcache);

			int r = 0;
			while ((r = scope->Build()) == asBUILD_IN_PROGRESS)
				std::this_thread::sleep_for(std::chrono::microseconds(COMPILER_BLOCKED_WAIT_US));

			vm->clear_sections();
			built = (r >= 0);
			if (!built)
				return function_factory::to_return(r);

			VI_DEBUG("[vm] OK compile on 0x%" PRIXPTR, (uintptr_t)this);
			if (vcache.name.empty())
				return function_factory::to_return(r);

			auto status = save_byte_code(&vcache);
			if (status)
				vm->set_byte_code_cache(&vcache);
			return status;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_promise_vm<void> compiler::load_byte_code(byte_code_info* info)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(scope != nullptr, "module should not be empty");
			VI_ASSERT(info != nullptr, "bytecode should be set");
#ifdef VI_ANGELSCRIPT
			return core::cotask<expects_vm<void>>(std::bind(&compiler::load_byte_code_sync, this, info));
#else
			return expects_promise_vm<void>(virtual_exception(virtual_error::not_supported));
#endif
		}
		expects_promise_vm<void> compiler::compile()
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(scope != nullptr, "module should not be empty");
#ifdef VI_ANGELSCRIPT
			return core::cotask<expects_vm<void>>(std::bind(&compiler::compile_sync, this));
#else
			return expects_promise_vm<void>(virtual_exception(virtual_error::not_supported));
#endif
		}
		expects_promise_vm<void> compiler::compile_file(const std::string_view& name, const std::string_view& module_name, const std::string_view& entry_name)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			auto status = prepare(module_name, name);
			if (!status)
				return expects_promise_vm<void>(status);

			status = load_file(name);
			if (!status)
				return expects_promise_vm<void>(status);

			return compile();
#else
			return expects_promise_vm<void>(virtual_exception(virtual_error::not_supported));
#endif
		}
		expects_promise_vm<void> compiler::compile_memory(const std::string_view& buffer, const std::string_view& module_name, const std::string_view& entry_name)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(!buffer.empty(), "buffer should not be empty");
#ifdef VI_ANGELSCRIPT
			core::string name = "anonymous:" + core::to_string(counter++);
			auto status = prepare(module_name, "anonymous");
			if (!status)
				return expects_promise_vm<void>(status);

			status = load_code("anonymous", buffer);
			if (!status)
				return expects_promise_vm<void>(status);

			return compile();
#else
			return expects_promise_vm<void>(virtual_exception(virtual_error::not_supported));
#endif
		}
		expects_promise_vm<function> compiler::compile_function(const std::string_view& buffer, const std::string_view& returns, const std::string_view& args, core::option<size_t>&& function_id)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(!buffer.empty(), "buffer should not be empty");
			VI_ASSERT(scope != nullptr, "module should not be empty");
			VI_ASSERT(built, "module should be built");
#ifdef VI_ANGELSCRIPT
			core::string code = core::string(buffer);
			core::string name = " __vfunc" + core::to_string(function_id ? *function_id : (counter + 1));
			auto status = vm->generate_code(processor, name, code);
			if (!status)
				return expects_promise_vm<function>(virtual_exception(std::move(status.error().message())));

			core::string eval;
			eval.append(returns.empty() ? "void" : returns);
			eval.append(name);
			eval.append("(");
			eval.append(args);
			eval.append("){");

			if (!function_id)
				++counter;

			if (!returns.empty() && returns != "void")
			{
				size_t offset = buffer.size();
				while (offset > 0)
				{
					char u = buffer[offset - 1];
					if (u == '\"' || u == '\'')
					{
						--offset;
						while (offset > 0)
						{
							if (buffer[--offset] == u)
								break;
						}

						continue;
					}
					else if (u == ';' && offset < buffer.size())
						break;
					--offset;
				}

				if (offset > 0)
					eval.append(buffer.substr(0, offset));

				size_t size = returns.size();
				eval.append("return ");
				if (returns[size - 1] == '@')
				{
					eval.append("@");
					eval.append(returns.data(), size - 1);
				}
				else
					eval.append(returns);

				eval.append("(");
				eval.append(buffer.substr(offset));
				if (eval.back() == ';')
					eval.erase(eval.end() - 1);
				eval.append(");}");
			}
			else
			{
				eval.append(buffer);
				if (eval.back() == ';')
					eval.erase(eval.end() - 1);
				eval.append(";}");
			}

			asIScriptModule* source = get_module().get_module();
			return core::cotask<expects_vm<function>>([source, eval = std::move(eval)]() mutable
			{
				asIScriptFunction* function_pointer = nullptr; int r = 0;
				while ((r = source->CompileFunction("__vfunc", eval.c_str(), -1, asCOMP_ADD_TO_MODULE, &function_pointer)) == asBUILD_IN_PROGRESS)
					std::this_thread::sleep_for(std::chrono::microseconds(COMPILER_BLOCKED_WAIT_US));
				return function_factory::to_return<function>(r, function(function_pointer));
			});
#else
			return expects_promise_vm<function>(virtual_exception(virtual_error::not_supported));
#endif
		}
		virtual_machine* compiler::get_vm() const
		{
			return vm;
		}
		library compiler::get_module() const
		{
			return scope;
		}
		compute::preprocessor* compiler::get_processor() const
		{
			return processor;
		}

		debugger_context::debugger_context(debug_type type) noexcept : last_context(nullptr), force_switch_threads(0), last_function(nullptr), vm(nullptr), action(type == debug_type::suspended ? debug_action::trigger : debug_action::next), input_error(false), attachable(type != debug_type::detach)
		{
			last_command_at_stack_level = 0;
			add_default_commands();
		}
		debugger_context::~debugger_context() noexcept
		{
#ifdef VI_ANGELSCRIPT
			if (vm != nullptr)
			{
				auto engine = vm->get_engine();
				if (engine != nullptr)
					engine->Release();
			}
#endif
		}
		void debugger_context::add_default_commands()
		{
			add_command("h, help", "show debugger commands", args_type::no_args, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				size_t max = 0;
				for (auto& next : descriptions)
				{
					if (next.first.size() > max)
						max = next.first.size();
				}

				for (auto& next : descriptions)
				{
					size_t spaces = max - next.first.size();
					output("  ");
					output(next.first);
					for (size_t i = 0; i < spaces; i++)
						output(" ");
					output(" - ");
					output(next.second);
					output("\n");
				}
				return false;
			});
			add_command("ed, exportdef", "load global system addon", args_type::expression, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				if (!args.empty())
				{
					core::string result;
					for (auto& definitions : vm->generate_definitions(context))
						result.append(definitions.second);
					
					auto path = core::os::path::resolve(args[0]);
					if (path)
						core::os::file::write(args[0], (uint8_t*)result.data(), result.size()).report("file write error");
					else
						path.report("invalid path");
				}
				else
					output("  exportdef <result file name>\n");
				return false;
			});
			add_command("ls, loadsys", "load global system addon", args_type::expression, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				if (!args.empty() && !args[0].empty())
					vm->import_system_addon(args[0]);
				return false;
			});
			add_command("le, loadext", "load global external addon", args_type::expression, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				if (!args.empty() && !args[0].empty())
					vm->import_addon(args[0]);
				return false;
			});
			add_command("e, eval", "evaluate script expression", args_type::expression, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				if (!args.empty() && !args[0].empty())
					execute_expression(context, args[0], core::string(), nullptr);

				return false;
			});
			add_command("b, break", "add a break point", args_type::array, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				if (args.size() > 1)
				{
					if (args[0].empty())
						goto break_failure;

					auto numeric = core::from_string<int>(args[1]);
					if (!numeric)
						goto break_failure;

					add_file_break_point(args[0], *numeric);
					return false;
				}
				else if (args.size() == 1)
				{
					if (args[0].empty())
						goto break_failure;

					auto numeric = core::from_string<int>(args[0]);
					if (!numeric)
					{
						add_func_break_point(args[0]);
						return false;
					}

					std::string_view file = "";
					context->get_line_number(0, 0, &file);
					if (file.empty())
						goto break_failure;

					add_file_break_point(file, *numeric);
					return false;
				}

			break_failure:
				output(
					"  break <file name> <line number>\n"
					"  break <function name | line number>\n");
				return false;
			});
			add_command("del, clear", "remove a break point", args_type::expression, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				if (args.empty())
				{
				clear_failure:
					output("  clear <all | breakpoint number>\n");
					return false;
				}

				if (args[0] == "all")
				{
					break_points.clear();
					output("  all break points have been removed\n");
					return false;
				}

				auto numeric = core::from_string<uint64_t>(args[0]);
				if (!numeric)
					goto clear_failure;

				size_t offset = (size_t)*numeric;
				if (offset >= break_points.size())
					goto clear_failure;

				break_points.erase(break_points.begin() + offset);
				list_break_points();
				return false;
			});
			add_command("p, print", "print variable value", args_type::expression, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				if (args[0].empty())
				{
					output("  print <expression>\n");
					return false;
				}

				print_value(args[0], context);
				return false;
			});
			add_command("d, dump", "dump compiled function bytecode by name or declaration", args_type::expression, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				if (args[0].empty())
				{
					output("  dump <function declaration | function name>\n");
					return false;
				}

				print_byte_code(args[0], context);
				return false;
			});
			add_command("i, info", "show info about of specific topic", args_type::array, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				if (args.empty())
					goto info_failure;

				if (args[0] == "b" || args[0] == "break")
				{
					list_break_points();
					return false;
				}
				if (args[0] == "e" || args[0] == "exception")
				{
					show_exception(context);
					return false;
				}
				else if (args[0] == "s" || args[0] == "stack")
				{
					if (args.size() > 1)
					{
						auto numeric = core::from_string<uint32_t>(args[1]);
						if (numeric)
							list_stack_registers(context, *numeric);
						else
							output("  invalid stack level");
					}
					else
						list_stack_registers(context, 0);

					return false;
				}
				else if (args[0] == "l" || args[0] == "locals")
				{
					list_local_variables(context);
					return false;
				}
				else if (args[0] == "g" || args[0] == "globals")
				{
					list_global_variables(context);
					return false;
				}
				else if (args[0] == "m" || args[0] == "members")
				{
					list_member_properties(context);
					return false;
				}
				else if (args[0] == "t" || args[0] == "threads")
				{
					list_threads();
					return false;
				}
				else if (args[0] == "c" || args[0] == "code")
				{
					list_source_code(context);
					return false;
				}
				else if (args[0] == "a" || args[0] == "addons")
				{
					list_addons();
					return false;
				}
				else if (args[0] == "rd" || args[0] == "definitions")
				{
					list_definitions(context);
					return false;
				}
				else if (args[0] == "gc" || args[0] == "garbage")
				{
					list_statistics(context);
					return false;
				}

			info_failure:
				output(
					"  info b, info break - show breakpoints\n"
					"  info s, info stack <level?> - show stack registers\n"
					"  info e, info exception - show current exception\n"
					"  info l, info locals - show local variables\n"
					"  info m, info members - show member properties\n"
					"  info g, info globals - show global variables\n"
					"  info t, info threads - show suspended threads\n"
					"  info c, info code - show source code section\n"
					"  info a, info addons - show imported addons\n"
					"  info ri, info interfaces - show registered script virtual interfaces\n"
					"  info gc, info garbage - show gc statistics\n");
				return false;
			});
			add_command("t, thread", "switch to thread by it's number", args_type::array, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				if (args.empty())
				{
				thread_failure:
					output("  thread <thread number>\n");
					return false;
				}

				auto numeric = core::from_string<uint64_t>(args[0]);
				if (!numeric)
					goto thread_failure;

				size_t index = (size_t)*numeric;
				if (index >= threads.size())
					goto thread_failure;

				auto& thread = threads[index];
				if (thread.context == context)
					return false;

				action = debug_action::match;
				last_context = thread.context;
				return true;
			});
			add_command("c, continue", "continue execution", args_type::no_args, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				action = debug_action::next;
				return true;
			});
			add_command("s, step", "step into subroutine", args_type::no_args, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				action = debug_action::step_into;
				return true;
			});
			add_command("n, next", "step over subroutine", args_type::no_args, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				action = debug_action::step_over;
				last_command_at_stack_level = (unsigned int)context->get_callstack_size();
				return true;
			});
			add_command("fin, finish", "step out of subroutine", args_type::no_args, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				action = debug_action::step_out;
				last_command_at_stack_level = (unsigned int)context->get_callstack_size();
				return true;
			});
			add_command("a, abort", "abort current execution", args_type::no_args, [](immediate_context* context, const core::vector<core::string>& args)
			{
				context->abort();
				return false;
			});
			add_command("bt, backtrace", "show current callstack", args_type::no_args, [this](immediate_context* context, const core::vector<core::string>& args)
			{
				print_callstack(context);
				return false;
			});
		}
		void debugger_context::add_command(const std::string_view& name, const std::string_view& description, args_type type, command_callback&& callback)
		{
			descriptions[core::string(name)] = description;
			auto types = core::stringify::split(name, ',');
			if (types.size() > 1)
			{
				for (auto& command : types)
				{
					core::stringify::trim(command);
					auto& data = commands[command];
					data.callback = callback;
					data.description = description;
					data.arguments = type;
				}
			}
			else
			{
				auto& command = types.front();
				core::stringify::trim(command);
				auto& data = commands[command];
				data.callback = std::move(callback);
				data.description = description;
				data.arguments = type;
			}
		}
		void debugger_context::set_interrupt_callback(interrupt_callback&& callback)
		{
			on_interrupt = std::move(callback);
		}
		void debugger_context::set_output_callback(output_callback&& callback)
		{
			on_output = std::move(callback);
		}
		void debugger_context::set_input_callback(input_callback&& callback)
		{
			on_input = std::move(callback);
		}
		void debugger_context::set_exit_callback(exit_callback&& callback)
		{
			on_exit = std::move(callback);
		}
		void debugger_context::add_to_string_callback(const type_info& type, to_string_callback&& callback)
		{
			fast_to_string_callbacks[type.get_type_info()] = std::move(callback);
		}
		void debugger_context::add_to_string_callback(const std::string_view& type, to_string_type_callback&& callback)
		{
			for (auto& item : core::stringify::split(type, ','))
			{
				core::stringify::trim(item);
				slow_to_string_callbacks[item] = std::move(callback);
			}
		}
		void debugger_context::line_callback(asIScriptContext* base)
		{
#ifdef VI_ANGELSCRIPT
			immediate_context* context = immediate_context::get(base);
			VI_ASSERT(context != nullptr, "context should be set");
			auto state = base->GetState();
			if (state != asEXECUTION_ACTIVE && state != asEXECUTION_EXCEPTION)
			{
				if (state != asEXECUTION_SUSPENDED)
					clear_thread(context);
				return;
			}

			thread_data thread = get_thread(context);
			if (state == asEXECUTION_EXCEPTION)
				force_switch_threads = 1;

		retry:
			core::umutex<std::recursive_mutex> unique(thread_barrier);
			if (force_switch_threads > 0)
			{
				if (state == asEXECUTION_EXCEPTION)
				{
					action = debug_action::trigger;
					force_switch_threads = 0;
					if (last_context != thread.context)
					{
						last_context = context;
						output("  exception handler caused switch to thread " + core::os::process::get_thread_id(thread.id) + " after last continuation\n");
					}
					show_exception(context);
				}
				else
				{
					unique.negate();
					std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_BLOCKED_WAIT_MS));
					goto retry;
				}
			}
			else if (action == debug_action::next)
			{
				last_context = context;
				if (!check_break_point(context))
					return;
			}
			else if (action == debug_action::match)
			{
				if (last_context != thread.context)
				{
					unique.negate();
					std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_BLOCKED_WAIT_MS));
					goto retry;
				}
				else
				{
					last_context = context;
					action = debug_action::trigger;
					output("  switched to thread " + core::os::process::get_thread_id(thread.id) + "\n");
				}
			}
			else if (action == debug_action::step_over)
			{
				last_context = context;
				if (base->GetCallstackSize() > last_command_at_stack_level && !check_break_point(context))
					return;
			}
			else if (action == debug_action::step_out)
			{
				last_context = context;
				if (base->GetCallstackSize() >= last_command_at_stack_level && !check_break_point(context))
					return;
			}
			else if (action == debug_action::step_into)
			{
				last_context = context;
				check_break_point(context);
			}
			else if (action == debug_action::interrupt)
			{
				if (on_interrupt)
					on_interrupt(true);

				last_context = context;
				action = debug_action::trigger;
				output("  execution interrupt signal has been raised, moving to input trigger\n");
				print_callstack(context);
			}
			else if (action == debug_action::trigger)
				last_context = context;

			input(context);
			if (action == debug_action::match || force_switch_threads > 0)
			{
				unique.negate();
				std::this_thread::sleep_for(std::chrono::milliseconds(THREAD_BLOCKED_WAIT_MS));
				goto retry;
			}

			if (on_interrupt)
				on_interrupt(false);
#endif
		}
		void debugger_context::exception_callback(asIScriptContext* base)
		{
#ifdef VI_ANGELSCRIPT
			if (!base->WillExceptionBeCaught())
				line_callback(base);
#endif
		}
		void debugger_context::trigger(immediate_context* context, uint64_t timeout_ms)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			line_callback(context->get_context());
			if (timeout_ms > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));
		}
		void debugger_context::throw_internal_exception(const std::string_view& message)
		{
			if (message.empty())
				return;

			auto exception = bindings::exception::pointer(core::string(message));
			output("  " + exception.get_type() + ": " + exception.get_text() + "\n");
		}
		void debugger_context::allow_input_after_failure()
		{
			input_error = false;
		}
		void debugger_context::input(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(context->get_context() != nullptr, "context should be set");
			if (input_error)
				return;


			for (;;)
			{
				core::string data;
				output("(dbg) ");
				if (on_input)
				{
					if (!on_input(data))
					{
						input_error = true;
						break;
					}
				}
				else if (!core::console::get()->read_line(data, 1024))
				{
					input_error = true;
					break;
				}

				if (interpret_command(data, context))
					break;
			}
			if (!input_error)
				return;

			for (auto& thread : threads)
				thread.context->abort();
		}
		void debugger_context::print_value(const std::string_view& expression, immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(context->get_function().is_valid(), "context current function should be set");
#ifdef VI_ANGELSCRIPT
			auto is_global_search_only = [](const core::string& name) -> bool
			{
				return core::stringify::starts_with(name, "::");
			};
			auto get_index_invocation = [](const core::string& name) -> core::option<core::string>
			{
				if (name.empty() || name.front() != '[' || name.back() != ']')
					return core::optional::none;

				return name.substr(1, name.size() - 2);
			};
			auto get_method_invocation = [](const core::string& name) -> core::option<core::string>
			{
				if (name.empty() || name.front() != '(' || name.back() != ')')
					return core::optional::none;

				return name.substr(1, name.size() - 2);
			};
			auto get_namespace_scope = [](core::string& name) -> core::string
			{
				size_t position = name.rfind("::");
				if (position == std::string::npos)
					return core::string();

				auto name_space = name.substr(0, position);
				name.erase(0, name_space.size() + 2);
				return name_space;
			};
			auto parse_expression = [](const std::string_view& expression) -> core::vector<core::string>
			{
				core::vector<core::string> stack;
				size_t start = 0, end = 0;
				while (end < expression.size())
				{
					char v = expression[end];
					if (v == '(' || v == '[')
					{
						size_t offset = end + 1, nesting = 1;
						while (nesting > 0 && offset < expression.size())
						{
							char n = expression[offset++];
							if (n == (v == '(' ? ')' : ']'))
								--nesting;
							else if (n == v)
								++nesting;
						}
						end = offset;
					}
					else if (v == '.')
					{
						stack.push_back(core::string(expression.substr(start, end - start)));
						start = ++end;
					}
					else
						++end;
				}

				if (start < end)
					stack.push_back(core::string(expression.substr(start, end - start)));

				return stack;
			};
			auto sparsify_stack = [](core::vector<core::string>& stack) -> bool
			{
			retry:
				size_t offset = 0;
				for (auto& item : stack)
				{
					auto& name = core::stringify::trim(item);
					if (name.empty() || name.front() == '[' || name.front() == '(')
					{
					next:
						++offset;
						continue;
					}

					if (name.back() == ']')
					{
						size_t position = name.find('[');
						if (position == std::string::npos)
							goto next;

						core::string index = name.substr(position);
						name.erase(position);
						stack.insert(stack.begin() + offset + 1, std::move(index));
						goto retry;
					}
					else if (name.back() == ')')
					{
						size_t position = name.find('(');
						if (position == std::string::npos)
							goto next;

						core::string index = name.substr(position);
						name.erase(position);
						stack.insert(stack.begin() + offset + 1, std::move(index));
						goto retry;
					}
					goto next;
				}

				return true;
			};

			asIScriptEngine* engine = context->get_vm()->get_engine();
			asIScriptContext* base = context->get_context();
			VI_ASSERT(base != nullptr, "context should be set");

			auto stack = parse_expression(expression);
			if (stack.empty() || !sparsify_stack(stack))
				return output("  no tokens found in the expression");

			int top_type_id = asTYPEID_VOID;
			asIScriptFunction* this_function = nullptr;
			void* this_pointer = nullptr;
			int this_type_id = asTYPEID_VOID;
			core::string callable;
			core::string last;
			size_t top = 0;

			while (!stack.empty())
			{
				auto& name = stack.front();
				if (top > 0)
				{
					auto index = get_index_invocation(name);
					auto method = get_method_invocation(name);
					if (index)
					{
						asITypeInfo* type = engine->GetTypeInfoById(this_type_id);
						if (!type)
							return output("  symbol <" + last + "> type is not iterable\n");

						asIScriptFunction* index_operator = nullptr;
						for (asUINT i = 0; i < type->GetMethodCount(); i++)
						{
							asIScriptFunction* next = type->GetMethodByIndex(i);
							if (!strcmp(next->GetName(), "opIndex"))
							{
								index_operator = next;
								break;
							}
						}

						if (!index_operator)
							return output("  symbol <" + last + "> does not have opIndex method\n");

						const char* type_declaration = nullptr;
						asITypeInfo* top_type = engine->GetTypeInfoById(top_type_id);
						if (top_type != nullptr)
						{
							for (asUINT i = 0; i < top_type->GetPropertyCount(); i++)
							{
								const char* var_name = nullptr;
								top_type->GetProperty(i, &var_name);
								if (last == var_name)
								{
									type_declaration = top_type->GetPropertyDeclaration(i, true);
									break;
								}
							}
						}

						if (!type_declaration)
						{
							asIScriptModule* library = base->GetFunction()->GetModule();
							for (asUINT i = 0; i < library->GetGlobalVarCount(); i++)
							{
								const char* var_name = nullptr;
								library->GetGlobalVar(i, &var_name);
								if (last == var_name)
								{
									type_declaration = library->GetGlobalVarDeclaration(i, true);
									break;
								}
							}
						}

						if (!type_declaration)
						{
							int var_count = base->GetVarCount();
							if (var_count > 0)
							{
								for (asUINT n = 0; n < (asUINT)var_count; n++)
								{
									const char* var_name = nullptr;
									base->GetVar(n, 0, &var_name);
									if (base->IsVarInScope(n) && var_name != 0 && name == var_name)
									{
										type_declaration = base->GetVarDeclaration(n, 0, true);
										break;
									}
								}
							}
						}

						if (!type_declaration)
							return output("  symbol <" + last + "> does not have type decl for <" + name + "> to call operator method\n");

						core::string args = type_declaration;
						if (this_type_id & asTYPEID_OBJHANDLE)
						{
							this_pointer = *(void**)this_pointer;
							core::stringify::replace(args, "& ", "@ ");
						}
						else
							core::stringify::replace(args, "& ", "&in ");
						execute_expression(context, last + name, args, [this_pointer](immediate_context* context)
						{
							context->set_arg_object(0, this_pointer);
						});
						return;
					}
					else if (method)
					{
						if (!this_function)
							return output("  symbol <" + last + "> is not a function\n");

						core::string call = this_function->GetName();
						if (!this_pointer)
						{
							execute_expression(context, core::stringify::text("%s(%s)", call.c_str(), method->c_str()), core::string(), nullptr);
							return;
						}

						const char* type_declaration = nullptr;
						asITypeInfo* top_type = engine->GetTypeInfoById(top_type_id);
						if (top_type != nullptr)
						{
							for (asUINT i = 0; i < top_type->GetPropertyCount(); i++)
							{
								const char* var_name = nullptr;
								top_type->GetProperty(i, &var_name);
								if (callable == var_name)
								{
									type_declaration = top_type->GetPropertyDeclaration(i, true);
									break;
								}
							}
						}

						if (!type_declaration)
						{
							asIScriptModule* library = base->GetFunction()->GetModule();
							for (asUINT i = 0; i < library->GetGlobalVarCount(); i++)
							{
								const char* var_name = nullptr;
								library->GetGlobalVar(i, &var_name);
								if (callable == var_name)
								{
									type_declaration = library->GetGlobalVarDeclaration(i, true);
									break;
								}
							}
						}

						if (!type_declaration)
						{
							int var_count = base->GetVarCount();
							if (var_count > 0)
							{
								for (asUINT n = 0; n < (asUINT)var_count; n++)
								{
									const char* var_name = nullptr;
									base->GetVar(n, 0, &var_name);
									if (base->IsVarInScope(n) && var_name != 0 && name == var_name)
									{
										type_declaration = base->GetVarDeclaration(n, 0, true);
										break;
									}
								}
							}
						}

						if (!type_declaration)
							return output("  symbol <" + callable + "> does not have type decl for <" + last + "> to call method\n");

						core::string args = type_declaration;
						if (this_type_id & asTYPEID_OBJHANDLE)
						{
							this_pointer = *(void**)this_pointer;
							core::stringify::replace(args, "& ", "@ ");
						}
						else
							core::stringify::replace(args, "& ", "&in ");
						execute_expression(context, core::stringify::text("%s.%s(%s)", callable.c_str(), call.c_str(), method->c_str()), args, [this_pointer](immediate_context* context)
						{
							context->set_arg_object(0, this_pointer);
						});
						return;
					}
					else
					{
						asITypeInfo* type = engine->GetTypeInfoById(this_type_id);
						if (!type)
							return output("  symbol <" + last + "> type is not iterable\n");

						for (asUINT n = 0; n < type->GetPropertyCount(); n++)
						{
							const char* prop_name = 0;
							int offset = 0;
							bool is_reference = 0;
							int composite_offset = 0;
							bool is_composite_indirect = false;
							int type_id = 0;

							type->GetProperty(n, &prop_name, &type_id, 0, 0, &offset, &is_reference, 0, &composite_offset, &is_composite_indirect);
							if (name != prop_name)
								continue;

							if (this_type_id & asTYPEID_OBJHANDLE)
								this_pointer = *(void**)this_pointer;

							this_pointer = (void*)(((asBYTE*)this_pointer) + composite_offset);
							if (is_composite_indirect)
								this_pointer = *(void**)this_pointer;

							this_pointer = (void*)(((asBYTE*)this_pointer) + offset);
							if (is_reference)
								this_pointer = *(void**)this_pointer;

							this_type_id = type_id;
							goto next_iteration;
						}

						for (asUINT n = 0; n < type->GetMethodCount(); n++)
						{
							asIScriptFunction* method_function = type->GetMethodByIndex(n);
							if (!strcmp(method_function->GetName(), name.c_str()))
							{
								this_function = method_function;
								callable = last;
								goto next_iteration;
							}
						}

						return output("  symbol <" + name + "> was not found in <" + last + ">\n");
					}
				}
				else
				{
					if (name == "this")
					{
						asIScriptFunction* function = base->GetFunction();
						if (!function)
							return output("  no function to be observed\n");

						this_pointer = base->GetThisPointer();
						this_type_id = base->GetThisTypeId();
						if (this_pointer != nullptr && this_type_id > 0 && function->GetObjectType() != nullptr)
							goto next_iteration;

						return output("  this function is not a method\n");
					}

					bool global_only = is_global_search_only(name);
					if (!global_only)
					{
						int var_count = base->GetVarCount();
						if (var_count > 0)
						{
							for (asUINT n = 0; n < (asUINT)var_count; n++)
							{
								const char* var_name = nullptr;
								base->GetVar(n, 0, &var_name, &this_type_id);
								if (base->IsVarInScope(n) && var_name != 0 && name == var_name)
								{
									this_pointer = base->GetAddressOfVar(n);
									goto next_iteration;
								}
							}
						}
					}

					asIScriptFunction* function = base->GetFunction();
					if (!function)
						return output("  no function to be observed\n");

					asIScriptModule* library = function->GetModule();
					if (!library)
						return output("  no module to be observed\n");

					auto name_space = get_namespace_scope(name);
					for (asUINT n = 0; n < library->GetGlobalVarCount(); n++)
					{
						const char* var_name = nullptr, * var_namespace = nullptr;
						library->GetGlobalVar(n, &var_name, &var_namespace, &this_type_id);
						if (name == var_name && name_space == var_namespace)
						{
							this_pointer = library->GetAddressOfGlobalVar(n);
							goto next_iteration;
						}
					}

					for (asUINT n = 0; n < engine->GetGlobalFunctionCount(); n++)
					{
						asIScriptFunction* global_function = engine->GetGlobalFunctionByIndex(n);
						if (!strcmp(global_function->GetName(), name.c_str()) && (!global_function->GetNamespace() || name_space == global_function->GetNamespace()))
						{
							this_function = global_function;
							callable = last;
							goto next_iteration;
						}
					}

					name_space = (name_space.empty() ? "" : name_space + "::");
					return output("  symbol <" + name_space + name + "> was not found in " + core::string(global_only ? "global" : "global or local") + " scope\n");
				}
			next_iteration:
				last = name;
				stack.erase(stack.begin());
				if (top % 2 == 0)
					top_type_id = this_type_id;
				++top;
			}

			if (this_function != nullptr)
			{
				output("  ");
				output(this_function->GetDeclaration(true, true, true));
				output("\n");
			}
			else if (this_pointer != nullptr)
			{
				core::string indent = "  ";
				core::string_stream stream;
				stream << indent << to_string(indent, 3, this_pointer, this_type_id) << std::endl;
				output(stream.str());
			}
#endif
		}
		void debugger_context::print_byte_code(const std::string_view& function_decl, immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(core::stringify::is_cstring(function_decl), "fndecl should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* base = context->get_context();
			VI_ASSERT(base != nullptr, "context should be set");

			asIScriptFunction* function = context->get_context()->GetFunction();
			if (!function)
				return output("  context was not found\n");

			asIScriptModule* library = function->GetModule();
			if (!library)
				return output("  module was not found\n");

			function = library->GetFunctionByName(function_decl.data());
			if (!function)
			{
				function = library->GetFunctionByDecl(function_decl.data());
				if (!function)
				{
					auto keys = core::stringify::split(function_decl, "::");
					if (keys.size() >= 2)
					{
						size_t length = keys.size() - 1;
						core::string type_name = keys.front();
						for (size_t i = 1; i < length; i++)
						{
							type_name += "::";
							type_name += keys[i];
						}

						auto* type = library->GetTypeInfoByName(type_name.c_str());
						if (type != nullptr)
						{
							function = type->GetMethodByName(keys.back().c_str());
							if (!function)
								function = type->GetMethodByDecl(keys.back().c_str());
						}
					}
				}
			}

			if (!function)
				return output("  function was not found\n");

			core::string_stream stream;
			stream << "  function <" << function->GetName() << "> disassembly:\n";

			asUINT offset = 0, size = 0, instructions = 0;
			asDWORD* byte_code = function->GetByteCode(&size);
			while (offset < size)
			{
				stream << "  ";
				offset = (asUINT)byte_code_label_to_text(stream, vm, byte_code, offset, false, false);
				++instructions;
			}

			stream << "  " << instructions << " instructions (" << size << " bytes)\n";
			output(stream.str());
#endif
		}
		void debugger_context::show_exception(immediate_context* context)
		{
			core::string_stream stream;
			auto exception = bindings::exception::get_exception();
			if (exception.empty())
				return;

			core::string data = exception.what();
			auto exception_lines = core::stringify::split(data, '\n');
			if (!context->will_exception_be_caught() && !exception_lines.empty())
				exception_lines[0] = "uncaught " + exception_lines[0];

			for (auto& line : exception_lines)
			{
				if (line.empty())
					continue;

				if (line.front() == '#')
					stream << "    " << line << "\n";
				else
					stream << "  " << line << "\n";
			}

			std::string_view file = "";
			int column_number = 0;
			int line_number = context->get_exception_line_number(&column_number, &file);
			if (!file.empty() && line_number > 0)
			{
				auto code = vm->get_source_code_appendix_by_path("exception origin", file, line_number, column_number, 5);
				if (code)
					stream << *code << "\n";
			}
			output(stream.str());
		}
		void debugger_context::list_break_points()
		{
			core::string_stream stream;
			for (size_t b = 0; b < break_points.size(); b++)
			{
				if (break_points[b].function)
					stream << "  " << b << " - " << break_points[b].name << std::endl;
				else
					stream << "  " << b << " - " << break_points[b].name << ":" << break_points[b].line << std::endl;
			}
			output(stream.str());
		}
		void debugger_context::list_threads()
		{
#ifdef VI_ANGELSCRIPT
			core::unordered_set<core::string> ids;
			core::string_stream stream;
			size_t index = 0;
			for (auto& item : threads)
			{
				asIScriptContext* context = item.context->get_context();
				asIScriptFunction* function = context->GetFunction();
				if (last_context == item.context)
					stream << "  * ";
				else
					stream << "  ";

				core::string thread_id = core::os::process::get_thread_id(item.id);
				stream << "#" << index++ << " " << (ids.find(thread_id) != ids.end() ? "coroutine" : "thread") << " " << thread_id << ", ";
				ids.insert(thread_id);

				if (function != nullptr)
				{
					if (function->GetFuncType() == asFUNC_SCRIPT)
					{
						const char* script_section = nullptr;
						function->GetDeclaredAt(&script_section, nullptr, nullptr);
						stream << "source \"" << (script_section ? script_section : "") << "\", line " << context->GetLineNumber() << ", in " << function->GetDeclaration();
					}
					else
						stream << "source { native code }, in " << function->GetDeclaration();
					stream << " 0x" << function;
				}
				else
					stream << "source { native code } [nullptr]";
				stream << "\n";
			}
			output(stream.str());
#endif
		}
		void debugger_context::list_addons()
		{
			for (auto& name : vm->get_exposed_addons())
				output("  " + name + "\n");
		}
		void debugger_context::list_stack_registers(immediate_context* context, uint32_t level)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* base = context->get_context();
			VI_ASSERT(base != nullptr, "context should be set");

			asUINT stack_size = base->GetCallstackSize();
			if (level >= stack_size)
				return output("  there are only " + core::to_string(stack_size) + " stack frames\n");

			core::string_stream stream;
			stream << "  stack frame #" << level << " values:\n";
			int properties_count = base->GetVarCount(level);
			if (properties_count > 0)
			{
				core::string indent = "    ";
				for (asUINT n = properties_count; n != (asUINT)-1; n--)
				{
					int type_id, offset; bool heap, active = base->IsVarInScope(n, level);
					if (base->GetVar(n, level, nullptr, &type_id, nullptr, &heap, &offset) < 0)
						continue;

					stream << indent << "#" << n << " [sp:" << offset << ";hp:" << heap << "] " << char_trim_end(base->GetVarDeclaration(n, level)) << ": ";
					if (active)
						stream << to_string(indent, 3, base->GetAddressOfVar(n), type_id) << std::endl;
					else
						stream << "<uninitialized>" << std::endl;
				}
			}

			bool has_base_call_state = false;
			if (properties_count <= 0)
				stream << "  stack arguments are empty\n";

			stream << "  stack frame #" << level << " registers:\n";
			asIScriptFunction* current_function = nullptr;
			asDWORD stack_frame_pointer, program_pointer, stack_pointer, stack_index;
			if (base->GetCallStateRegisters(level, &stack_frame_pointer, &current_function, &program_pointer, &stack_pointer, &stack_index) >= 0)
			{
				void* this_pointer = base->GetThisPointer(level);
				asITypeInfo* this_type = vm->get_engine()->GetTypeInfoById(base->GetThisTypeId(level));
				const char* section_name = ""; int column_number = 0;
				int line_number = base->GetLineNumber(level, &column_number, &section_name);
				stream << "    [sfp] stack frame pointer: " << stack_frame_pointer << "\n";
				stream << "    [pp] program pointer: " << program_pointer << "\n";
				stream << "    [sp] stack pointer: " << stack_pointer << "\n";
				stream << "    [si] stack index: " << stack_index << "\n";
				stream << "    [sd] stack depth: " << stack_size - (level + 1) << "\n";
				stream << "    [tp] this pointer: 0x" << this_pointer << "\n";
				stream << "    [ttp] this type pointer: 0x" << this_type << " (" << (this_type ? this_type->GetName() : "null") << ")" << "\n";
				stream << "    [sn] section name: " << (section_name ? section_name : "?") << "\n";
				stream << "    [ln] line number: " << line_number << "\n";
				stream << "    [cn] column number: " << column_number << "\n";
				stream << "    [ces] context execution state: ";
				switch (base->GetState())
				{
					case asEXECUTION_FINISHED:
						stream << "finished\n";
						break;
					case asEXECUTION_SUSPENDED:
						stream << "suspended\n";
						break;
					case asEXECUTION_ABORTED:
						stream << "aborted\n";
						break;
					case asEXECUTION_EXCEPTION:
						stream << "exception\n";
						break;
					case asEXECUTION_PREPARED:
						stream << "prepared\n";
						break;
					case asEXECUTION_UNINITIALIZED:
						stream << "uninitialized\n";
						break;
					case asEXECUTION_ACTIVE:
						stream << "active\n";
						break;
					case asEXECUTION_ERROR:
						stream << "error\n";
						break;
					case asEXECUTION_DESERIALIZATION:
						stream << "deserialization\n";
						break;
					default:
						stream << "invalid\n";
						break;
				}
				has_base_call_state = true;
			}

			void* object_register = nullptr; asITypeInfo* object_type_register = nullptr;
			asIScriptFunction* calling_system_function = nullptr, * initial_function = nullptr;
			asDWORD original_stack_pointer, initial_arguments_size; asQWORD value_register;
			if (base->GetStateRegisters(level, &calling_system_function, &initial_function, &original_stack_pointer, &initial_arguments_size, &value_register, &object_register, &object_type_register) >= 0)
			{
				stream << "    [osp] original stack pointer: " << original_stack_pointer << "\n";
				stream << "    [vc] value content: " << value_register << "\n";
				stream << "    [op] object pointer: 0x" << object_register << "\n";
				stream << "    [otp] object type pointer: 0x" << object_type_register << " (" << (object_type_register ? object_type_register->GetName() : "null") << ")" << "\n";
				stream << "    [ias] initial arguments size: " << initial_arguments_size << "\n";

				if (initial_function != nullptr)
					stream << "    [if] initial function: " << initial_function->GetDeclaration(true, true, true) << "\n";

				if (calling_system_function != nullptr)
					stream << "    [csf] calling system function: " << calling_system_function->GetDeclaration(true, true, true) << "\n";
			}
			else if (!has_base_call_state)
				stream << "  stack registers are empty\n";

			if (has_base_call_state && current_function != nullptr)
			{
				asUINT size = 0; size_t preview_size = 40;
				asDWORD* byte_code = current_function->GetByteCode(&size);
				stream << "    [cf] current function: " << current_function->GetDeclaration(true, true, true) << "\n";
				if (byte_code != nullptr && program_pointer < size)
				{
					asUINT preview_program_pointer_begin = program_pointer - std::min<asUINT>((asUINT)preview_size, (asUINT)program_pointer);
					asUINT preview_program_pointer_end = program_pointer + std::min<asUINT>((asUINT)preview_size, (asUINT)size - (asUINT)program_pointer);
					stream << "  stack frame #" << level << " disassembly:\n";
					if (preview_program_pointer_begin < program_pointer)
						stream << "    ... " << program_pointer - preview_program_pointer_begin << " bytes of assembly data skipped\n";

					while (preview_program_pointer_begin < preview_program_pointer_end)
					{
						stream << "  ";
						preview_program_pointer_begin = (asUINT)byte_code_label_to_text(stream, vm, byte_code, preview_program_pointer_begin, preview_program_pointer_begin == program_pointer, false);
					}

					if (program_pointer < preview_program_pointer_end)
						stream << "    ... " << preview_program_pointer_end - program_pointer << " more bytes of assembly data\n";
				}
				else
					stream << "  stack frame #" << level << " disassembly:\n    ... assembly data is empty\n";
			}
			else
				stream << "  stack frame #" << level << " disassembly:\n    ... assembly data is empty\n";

			output(stream.str());
#endif
		}
		void debugger_context::list_member_properties(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* base = context->get_context();
			VI_ASSERT(base != nullptr, "context should be set");

			void* pointer = base->GetThisPointer();
			if (pointer != nullptr)
			{
				core::string indent = "  ";
				core::string_stream stream;
				stream << indent << "this = " << to_string(indent, 3, pointer, base->GetThisTypeId()) << std::endl;
				output(stream.str());
			}
#endif
		}
		void debugger_context::list_local_variables(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* base = context->get_context();
			VI_ASSERT(base != nullptr, "context should be set");

			asIScriptFunction* function = base->GetFunction();
			if (!function)
				return;

			core::string indent = "  ";
			core::string_stream stream;
			for (asUINT n = 0; n < function->GetVarCount(); n++)
			{
				if (!base->IsVarInScope(n))
					continue;

				int type_id;
				base->GetVar(n, 0, 0, &type_id);
				stream << indent << char_trim_end(function->GetVarDecl(n)) << ": " << to_string(indent, 3, base->GetAddressOfVar(n), type_id) << std::endl;
			}
			output(stream.str());
#endif
		}
		void debugger_context::list_global_variables(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* base = context->get_context();
			VI_ASSERT(base != nullptr, "context should be set");

			asIScriptFunction* function = base->GetFunction();
			if (!function)
				return;

			asIScriptModule* mod = function->GetModule();
			if (!mod)
				return;

			core::string indent = "  ";
			core::string_stream stream;
			for (asUINT n = 0; n < mod->GetGlobalVarCount(); n++)
			{
				int type_id = 0;
				mod->GetGlobalVar(n, nullptr, nullptr, &type_id);
				stream << indent << char_trim_end(mod->GetGlobalVarDeclaration(n)) << ": " << to_string(indent, 3, mod->GetAddressOfGlobalVar(n), type_id) << std::endl;
			}
			output(stream.str());
#endif
		}
		void debugger_context::list_source_code(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			std::string_view file = "";
			context->get_line_number(0, 0, &file);
			if (file.empty())
				return output("source code is not available");

			auto code = vm->get_script_section(file);
			if (!code)
				return output("source code is not available");

			auto lines = core::stringify::split(*code, '\n');
			size_t max_line_size = core::to_string(lines.size()).size(), line_number = 0;
			for (auto& line : lines)
			{
				size_t line_size = core::to_string(++line_number).size();
				size_t spaces = 1 + (max_line_size - line_size);
				output("  ");
				output(core::to_string(line_number));
				for (size_t i = 0; i < spaces; i++)
					output(" ");
				output(line + '\n');
			}
		}
		void debugger_context::list_definitions(immediate_context* context)
		{
			for (auto& interfacef : vm->generate_definitions(context))
			{
				output("  listing generated <" + interfacef.first + ">:\n");
				core::stringify::replace(interfacef.second, "\t", "  ");
				for (auto& line : core::stringify::split(interfacef.second, '\n'))
					output("    " + line + "\n");
				output("\n");
			}
		}
		void debugger_context::list_statistics(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* base = context->get_context();
			VI_ASSERT(base != nullptr, "context should be set");

			asIScriptEngine* engine = base->GetEngine();
			asUINT gc_curr_size, gc_total_destr, gc_total_det, gc_new_objects, gc_total_new_destr;
			engine->GetGCStatistics(&gc_curr_size, &gc_total_destr, &gc_total_det, &gc_new_objects, &gc_total_new_destr);

			core::string_stream stream;
			stream << "  garbage collector" << std::endl;
			stream << "    current size:          " << gc_curr_size << std::endl;
			stream << "    total destroyed:       " << gc_total_destr << std::endl;
			stream << "    total detected:        " << gc_total_det << std::endl;
			stream << "    new objects:           " << gc_new_objects << std::endl;
			stream << "    new objects destroyed: " << gc_total_new_destr << std::endl;
			output(stream.str());
#endif
		}
		void debugger_context::print_callstack(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(context->get_context() != nullptr, "context should be set");

			core::string_stream stream;
			stream << core::error_handling::get_stack_trace(1) << "\n";
			output(stream.str());
		}
		void debugger_context::add_func_break_point(const std::string_view& function)
		{
			size_t b = function.find_first_not_of(" \t"), e = function.find_last_not_of(" \t");
			core::string actual = core::string(function.substr(b, e != core::string::npos ? e - b + 1 : core::string::npos));

			core::string_stream stream;
			stream << "  adding deferred break point for function '" << actual << "'" << std::endl;
			output(stream.str());

			break_point point(actual, 0, true);
			break_points.push_back(point);
		}
		void debugger_context::add_file_break_point(const std::string_view& file, int line_number)
		{
			size_t r = file.find_last_of("\\/");
			core::string actual;

			if (r != core::string::npos)
				actual = file.substr(r + 1);
			else
				actual = file;

			size_t b = actual.find_first_not_of(" \t");
			size_t e = actual.find_last_not_of(" \t");
			actual = actual.substr(b, e != core::string::npos ? e - b + 1 : core::string::npos);

			core::string_stream stream;
			stream << "  setting break point in file '" << actual << "' at line " << line_number << std::endl;
			output(stream.str());

			break_point point(actual, line_number, false);
			break_points.push_back(point);
		}
		void debugger_context::output(const std::string_view& data)
		{
			if (on_output)
				on_output(data);
			else
				core::console::get()->write(data);
		}
		void debugger_context::set_engine(virtual_machine* engine)
		{
#ifdef VI_ANGELSCRIPT
			if (engine != nullptr && engine != vm)
			{
				if (vm != nullptr)
				{
					auto* engine = vm->get_engine();
					if (engine != nullptr)
						engine->Release();
				}

				vm = engine;
				vm->get_engine()->AddRef();
			}
#endif
		}
		bool debugger_context::check_break_point(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptContext* base = context->get_context();
			VI_ASSERT(base != nullptr, "context should be set");

			const char* temp = 0;
			int line = base->GetLineNumber(0, 0, &temp);

			core::string file = temp ? temp : "";
			size_t r = file.find_last_of("\\/");
			if (r != core::string::npos)
				file = file.substr(r + 1);

			asIScriptFunction* function = base->GetFunction();
			if (last_function != function)
			{
				for (size_t n = 0; n < break_points.size(); n++)
				{
					if (break_points[n].function)
					{
						if (break_points[n].name == function->GetName())
						{
							core::string_stream stream;
							stream << "  entering function '" << break_points[n].name << "', transforming it into break point" << std::endl;
							output(stream.str());

							break_points[n].name = file;
							break_points[n].line = line;
							break_points[n].function = false;
							break_points[n].needs_adjusting = false;
						}
					}
					else if (break_points[n].needs_adjusting && break_points[n].name == file)
					{
						int number = function->FindNextLineWithCode(break_points[n].line);
						if (number >= 0)
						{
							break_points[n].needs_adjusting = false;
							if (number != break_points[n].line)
							{
								core::string_stream stream;
								stream << "  moving break point " << n << " in file '" << file << "' to next line with code at line " << number << std::endl;
								output(stream.str());

								break_points[n].line = number;
							}
						}
					}
				}
			}

			last_function = function;
			for (size_t n = 0; n < break_points.size(); n++)
			{
				if (!break_points[n].function && break_points[n].line == line && break_points[n].name == file)
				{
					core::string_stream stream;
					stream << "  reached break point " << n << " in file '" << file << "' at line " << line << std::endl;
					output(stream.str());
					return true;
				}
			}
#endif
			return false;
		}
		bool debugger_context::interrupt()
		{
			core::umutex<std::recursive_mutex> unique(thread_barrier);
			if (action != debug_action::next && action != debug_action::step_into && action != debug_action::step_out)
				return false;

			action = debug_action::interrupt;
			return true;
		}
		bool debugger_context::interpret_command(const std::string_view& command, immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(context->get_context() != nullptr, "context should be set");

			for (auto& item : core::stringify::split(command, "&&"))
			{
				core::stringify::trim(item);
				core::string name = item.substr(0, item.find(' '));
				auto it = commands.find(name);
				if (it == commands.end())
				{
					output("  command <" + name + "> is not a known operation\n");
					return false;
				}

				core::string data = (name.size() == item.size() ? core::string() : item.substr(name.size()));
				core::vector<core::string> args;
				switch (it->second.arguments)
				{
					case args_type::array:
					{
						size_t offset = 0;
						while (offset < data.size())
						{
							char v = data[offset];
							if (core::stringify::is_whitespace(v))
							{
								size_t start = offset;
								while (++start < data.size() && core::stringify::is_whitespace(data[start]));

								size_t end = start;
								while (++end < data.size() && !core::stringify::is_whitespace(data[end]) && data[end] != '\"' && data[end] != '\'');

								auto value = data.substr(start, end - start);
								core::stringify::trim(value);

								if (!value.empty())
									args.push_back(value);
								offset = end;
							}
							else if (v == '\"' || v == '\'')
								while (++offset < data.size() && data[offset] != v);
							else
								++offset;
						}
						break;
					}
					case args_type::expression:
					{
						core::stringify::trim(data);
						args.emplace_back(data);
						break;
					}
					case args_type::no_args:
					default:
						if (data.empty())
							break;

						output("  command <" + name + "> requires no arguments\n");
						return false;
				}

				if (it->second.callback(context, args))
					return true;
			}

			return false;
		}
		bool debugger_context::is_error()
		{
			return input_error;
		}
		bool debugger_context::is_attached()
		{
			return attachable;
		}
		debugger_context::debug_action debugger_context::get_state()
		{
			return action;
		}
		size_t debugger_context::byte_code_label_to_text(core::string_stream& stream, virtual_machine* vm, void* program, size_t program_pointer, bool selection, bool uppercase)
		{
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm ? vm->get_engine() : nullptr;
			asDWORD* byte_code = ((asDWORD*)program) + program_pointer;
			asBYTE* base_code = (asBYTE*)byte_code;
			byte_code_label label = virtual_machine::get_byte_code_info((uint8_t)*base_code);
			auto print_argument = [&stream](asBYTE* offset, uint8_t size, bool last)
			{
				switch (size)
				{
					case sizeof(asBYTE) :
						stream << " %spl:" << *(asBYTE*)offset;
						if (!last)
							stream << ",";
						break;
						case sizeof(asWORD) :
							stream << " %sp:" << *(asWORD*)offset;
							if (!last)
								stream << ",";
							break;
							case sizeof(asDWORD) :
								stream << " %esp:" << *(asDWORD*)offset;
								if (!last)
									stream << ",";
								break;
								case sizeof(asQWORD) :
									stream << " %rdx:" << *(asQWORD*)offset;
									if (!last)
										stream << ",";
									break;
								default:
									break;
				}
			};

			stream << (selection ? "> 0x" : "  0x") << (void*)(uintptr_t)program_pointer << ": ";
			if (uppercase)
			{
				core::string name = core::string(label.name);
				stream << core::stringify::to_upper(name);
			}
			else
				stream << label.name;
			if (!vm)
				goto default_print;

			switch (*base_code)
			{
				case asBC_CALL:
				case asBC_CALLSYS:
				case asBC_CALLBND:
				case asBC_CALLINTF:
				case asBC_Thiscall1:
				{
					auto* function = engine->GetFunctionById(asBC_INTARG(byte_code));
					if (!function)
						goto default_print;

					auto* declaration = function->GetDeclaration();
					if (!declaration)
						goto default_print;

					stream << " %edi:[" << declaration << "]";
					break;
				}
				default:
				default_print:
					print_argument(base_code + label.offset_of_arg0, label.size_of_arg0, !label.size_of_arg1);
					print_argument(base_code + label.offset_of_arg1, label.size_of_arg1, !label.size_of_arg2);
					print_argument(base_code + label.offset_of_arg2, label.size_of_arg2, true);
					break;
			}

			stream << "\n";
			return program_pointer + label.size;
#else
			return program_pointer + 1;
#endif
		}
		core::string debugger_context::to_string(int depth, void* value, unsigned int type_id)
		{
			core::string indent;
			return to_string(indent, depth, value, type_id);
		}
		core::string debugger_context::to_string(core::string& indent, int depth, void* value, unsigned int type_id)
		{
#ifdef VI_ANGELSCRIPT
			if (value == 0 || !vm)
				return "null";

			asIScriptEngine* base = vm->get_engine();
			if (!base)
				return "null";

			core::string_stream stream;
			if (type_id == asTYPEID_VOID)
				return "void";
			else if (type_id == asTYPEID_BOOL)
				return *(bool*)value ? "true" : "false";
			else if (type_id == asTYPEID_INT8)
				stream << (int)*(signed char*)value;
			else if (type_id == asTYPEID_INT16)
				stream << (int)*(signed short*)value;
			else if (type_id == asTYPEID_INT32)
				stream << *(signed int*)value;
			else if (type_id == asTYPEID_INT64)
				stream << *(asINT64*)value;
			else if (type_id == asTYPEID_UINT8)
				stream << (unsigned int)*(unsigned char*)value;
			else if (type_id == asTYPEID_UINT16)
				stream << (unsigned int)*(unsigned short*)value;
			else if (type_id == asTYPEID_UINT32)
				stream << *(unsigned int*)value;
			else if (type_id == asTYPEID_UINT64)
				stream << *(asQWORD*)value;
			else if (type_id == asTYPEID_FLOAT)
				stream << *(float*)value;
			else if (type_id == asTYPEID_DOUBLE)
				stream << *(double*)value;
			else if ((type_id & asTYPEID_MASK_OBJECT) == 0)
			{
				asITypeInfo* t = base->GetTypeInfoById(type_id);
				stream << *(asUINT*)value;

				for (int n = t->GetEnumValueCount(); n-- > 0;)
				{
					int enum_val;
					const char* enum_name = t->GetEnumValueByIndex(n, &enum_val);
					if (enum_val == *(int*)value)
					{
						stream << " (" << enum_name << ")";
						break;
					}
				}
			}
			else if (type_id & asTYPEID_SCRIPTOBJECT)
			{
				if (type_id & asTYPEID_OBJHANDLE)
					value = *(void**)value;

				if (!value || !depth)
				{
					if (value != nullptr)
						stream << "0x" << value;
					else
						stream << "null";
					goto finalize;
				}

				asIScriptObject* object = (asIScriptObject*)value;
				asITypeInfo* type = object->GetObjectType();
				size_t size = object->GetPropertyCount();
				stream << "0x" << value << " (" << type->GetName() << ")";
				if (!size)
				{
					stream << " { }";
					goto finalize;
				}

				stream << "\n";
				indent.append("  ");
				for (asUINT n = 0; n < size; n++)
				{
					stream << indent << char_trim_end(type->GetPropertyDeclaration(n)) << ": " << to_string(indent, depth - 1, object->GetAddressOfProperty(n), object->GetPropertyTypeId(n));
					if (n + 1 < size)
						stream << "\n";
				}
				indent.erase(indent.end() - 2, indent.end());
			}
			else
			{
				if (type_id & asTYPEID_OBJHANDLE)
					value = *(void**)value;

				asITypeInfo* type = base->GetTypeInfoById(type_id);
				if (!value || !depth)
				{
					if (value != nullptr)
						stream << "0x" << value << " (" << type->GetName() << ")";
					else
						stream << "null (" << type->GetName() << ")";
					goto finalize;
				}

				auto it1 = fast_to_string_callbacks.find(type);
				if (it1 == fast_to_string_callbacks.end())
				{
					if (type->GetFlags() & asOBJ_TEMPLATE)
						it1 = fast_to_string_callbacks.find(base->GetTypeInfoByName(type->GetName()));
				}

				if (it1 != fast_to_string_callbacks.end())
				{
					indent.append("  ");
					stream << it1->second(indent, depth, value);
					indent.erase(indent.end() - 2, indent.end());
					goto finalize;
				}

				auto it2 = slow_to_string_callbacks.find(type->GetName());
				if (it2 != slow_to_string_callbacks.end())
				{
					indent.append("  ");
					stream << it2->second(indent, depth, value, type_id);
					indent.erase(indent.end() - 2, indent.end());
					goto finalize;
				}

				size_t size = type->GetPropertyCount();
				stream << "0x" << value << " (" << type->GetName() << ")";
				if (!size)
				{
					stream << " { }";
					goto finalize;
				}

				stream << "\n";
				indent.append("  ");
				for (asUINT n = 0; n < type->GetPropertyCount(); n++)
				{
					int prop_type_id, prop_offset;
					if (type->GetProperty(n, nullptr, &prop_type_id, nullptr, nullptr, &prop_offset, nullptr, nullptr, nullptr, nullptr) != 0)
						continue;

					stream << indent << char_trim_end(type->GetPropertyDeclaration(n)) << ": " << to_string(indent, depth - 1, (char*)value + prop_offset, prop_type_id);
					if (n + 1 < size)
						stream << "\n";
				}
				indent.erase(indent.end() - 2, indent.end());
			}

		finalize:
			return stream.str();
#else
			return "null";
#endif
		}
		void debugger_context::clear_thread(immediate_context* context)
		{
			core::umutex<std::recursive_mutex> unique(mutex);
			for (auto it = threads.begin(); it != threads.end(); it++)
			{
				if (it->context == context)
				{
					threads.erase(it);
					break;
				}
			}
		}
		expects_vm<void> debugger_context::execute_expression(immediate_context* context, const std::string_view& code, const std::string_view& args, args_callback&& on_args)
		{
			VI_ASSERT(vm != nullptr, "engine should be set");
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			core::string indent = "  ";
			core::string eval = "any@ __vfdbgfunc(" + core::string(args) + "){return any(" + core::string(code.empty() || code.back() != ';' ? code : code.substr(0, code.size() - 1)) + ");}";
			asIScriptModule* library = context->get_function().get_module().get_module();
			asIScriptFunction* function = nullptr;
			bindings::any* data = nullptr;
			vm->detach_debugger_from_context(context->get_context());
			vm->import_system_addon(ADDON_ANY);

			int result = 0;
			while ((result = library->CompileFunction("__vfdbgfunc", eval.c_str(), -1, asCOMP_ADD_TO_MODULE, &function)) == asBUILD_IN_PROGRESS)
				std::this_thread::sleep_for(std::chrono::microseconds(COMPILER_BLOCKED_WAIT_US));

			if (result < 0)
			{
				vm->attach_debugger_to_context(context->get_context());
				if (function != nullptr)
					function->Release();
				return virtual_exception((virtual_error)result);
			}

			context->disable_suspends();
			context->push_state();
			auto status1 = context->prepare(function);
			if (!status1)
			{
				context->pop_state();
				context->enable_suspends();
				vm->attach_debugger_to_context(context->get_context());
				if (function != nullptr)
					function->Release();
				return status1;
			}

			if (on_args)
				on_args(context);

			auto status2 = context->execute_next();
			if (status2 && *status2 == execution::finished)
			{
				data = context->get_return_object<bindings::any>();
				output(indent + to_string(indent, 3, data, vm->get_type_info_by_name("any").get_type_id()) + "\n");
			}

			context->pop_state();
			context->enable_suspends();
			vm->attach_debugger_to_context(context->get_context());
			if (function != nullptr)
				function->Release();

			if (!status2)
				return status2.error();

			return core::expectation::met;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		debugger_context::thread_data debugger_context::get_thread(immediate_context* context)
		{
			core::umutex<std::recursive_mutex> unique(mutex);
			for (auto& thread : threads)
			{
				if (thread.context == context)
				{
					thread.id = std::this_thread::get_id();
					return thread;
				}
			}

			thread_data thread;
			thread.context = context;
			thread.id = std::this_thread::get_id();
			threads.push_back(thread);
			return thread;
		}
		virtual_machine* debugger_context::get_engine()
		{
			return vm;
		}

		immediate_context::immediate_context(asIScriptContext* base) noexcept : context(base), vm(nullptr)
		{
			VI_ASSERT(base != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			context->SetUserData(this, context_ud);
			vm = virtual_machine::get(base->GetEngine());
#endif
		}
		immediate_context::~immediate_context() noexcept
		{
			if (executor.future.is_pending())
				executor.future.set(virtual_exception(virtual_error::context_not_prepared));
#ifdef VI_ANGELSCRIPT
			vm->get_engine()->ReturnContext(context);
#endif
		}
		expects_promise_vm<execution> immediate_context::execute_call(const function& function, args_callback&& on_args)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(function.is_valid(), "function should be set");
			VI_ASSERT(!core::costate::is_coroutine(), "cannot safely execute in coroutine");
#ifdef VI_ANGELSCRIPT
			core::umutex<std::recursive_mutex> unique(exchange);
			if (!can_execute_call())
				return expects_promise_vm<execution>(virtual_exception(virtual_error::context_active));

			int result = context->Prepare(function.get_function());
			if (result < 0)
				return expects_promise_vm<execution>(virtual_exception((virtual_error)result));

			if (on_args)
				on_args(this);

			executor.future = expects_promise_vm<execution>();
			resume();
			return executor.future;
#else
			return expects_promise_vm<execution>(virtual_exception(virtual_error::not_supported));
#endif
		}
		expects_vm<execution> immediate_context::execute_inline_call(const function& function, args_callback&& on_args)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(function.is_valid(), "function should be set");
			VI_ASSERT(!core::costate::is_coroutine(), "cannot safely execute in coroutine");
#ifdef VI_ANGELSCRIPT
			core::umutex<std::recursive_mutex> unique(exchange);
			if (!can_execute_call())
				return virtual_exception(virtual_error::context_active);

			disable_suspends();
			int result = context->Prepare(function.get_function());
			if (result < 0)
			{
				enable_suspends();
				return virtual_exception((virtual_error)result);
			}
			else if (on_args)
				on_args(this);

			auto status = execute_next();
			enable_suspends();
			return status;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<execution> immediate_context::execute_subcall(const function& function, args_callback&& on_args, args_callback&& on_return)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(function.is_valid(), "function should be set");
			VI_ASSERT(!core::costate::is_coroutine(), "cannot safely execute in coroutine");
#ifdef VI_ANGELSCRIPT
			core::umutex<std::recursive_mutex> unique(exchange);
			if (!can_execute_subcall())
			{
				VI_ASSERT(false, "context should be active");
				return virtual_exception(virtual_error::context_not_prepared);
			}

			disable_suspends();
			context->PushState();
			int result = context->Prepare(function.get_function());
			if (result < 0)
			{
				context->PopState();
				enable_suspends();
				return virtual_exception((virtual_error)result);
			}
			else if (on_args)
				on_args(this);

			auto status = execute_next();
			if (on_return)
				on_return(this);
			context->PopState();
			enable_suspends();
			return status;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<execution> immediate_context::execute_next()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			int r = context->Execute();
			if (callbacks.stop_executions.empty())
				return function_factory::to_return<execution>(r, (execution)r);

			core::umutex<std::recursive_mutex> unique(exchange);
			core::vector<stop_execution_callback> queue;
			queue.swap(callbacks.stop_executions);
			unique.negate();
			for (auto& callback : queue)
				callback();

			return function_factory::to_return<execution>(r, (execution)r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<execution> immediate_context::resume()
		{
#ifdef VI_ANGELSCRIPT
			auto status = execute_next();
			if (status && *status == execution::suspended)
				return status;

			core::umutex<std::recursive_mutex> unique(exchange);
			if (!executor.future.is_pending())
				return status;

			if (status)
				executor.future.set(*status);
			else
				executor.future.set(status.error());
			return status;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_promise_vm<execution> immediate_context::resolve_callback(function_delegate&& delegatef, args_callback&& on_args, args_callback&& on_return)
		{
			VI_ASSERT(delegatef.is_valid(), "callback should be valid");
#ifdef VI_ANGELSCRIPT
			if (callbacks.callback_resolver)
			{
				callbacks.callback_resolver(this, std::move(delegatef), std::move(on_args), std::move(on_return));
				return expects_promise_vm<execution>(execution::active);
			}

			core::umutex<std::recursive_mutex> unique(exchange);
			if (can_execute_call())
			{
				return execute_call(delegatef.callable(), std::move(on_args)).then<expects_vm<execution>>([this, on_return = std::move(on_return)](expects_vm<execution>&& result) mutable
				{
					if (on_return)
						on_return(this);
					return result;
				});
			}

			immediate_context* target = vm->request_context();
			return target->execute_call(delegatef.callable(), std::move(on_args)).then<expects_vm<execution>>([target, on_return = std::move(on_return)](expects_vm<execution>&& result) mutable
			{
				if (on_return)
					on_return(target);

				target->vm->return_context(target);
				return result;
			});
#else
			return expects_promise_vm<execution>(virtual_exception(virtual_error::not_supported));
#endif
		}
		expects_vm<execution> immediate_context::resolve_notification()
		{
#ifdef VI_ANGELSCRIPT
			if (callbacks.notification_resolver)
			{
				callbacks.notification_resolver(this);
				return execution::active;
			}

			immediate_context* context = this;
			core::codefer([context]()
			{
				if (context->is_suspended())
					context->resume();
			});
			return execution::active;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::prepare(const function& function)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->Prepare(function.get_function()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::unprepare()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->Unprepare());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::abort()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->Abort());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::suspend()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			if (!is_suspendable())
			{
				bindings::exception::throw_ptr_at(this, bindings::exception::pointer("async_error", "yield is not allowed in this function call"));
				return virtual_exception(virtual_error::context_not_prepared);
			}

			return function_factory::to_return(context->Suspend());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::push_state()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->PushState());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::pop_state()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->PopState());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_object(void* object)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetObject(object));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_arg8(size_t arg, unsigned char value)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetArgByte((asUINT)arg, value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_arg16(size_t arg, unsigned short value)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetArgWord((asUINT)arg, value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_arg32(size_t arg, int value)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetArgDWord((asUINT)arg, value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_arg64(size_t arg, int64_t value)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetArgQWord((asUINT)arg, value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_arg_float(size_t arg, float value)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetArgFloat((asUINT)arg, value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_arg_double(size_t arg, double value)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetArgDouble((asUINT)arg, value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_arg_address(size_t arg, void* address)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetArgAddress((asUINT)arg, address));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_arg_object(size_t arg, void* object)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetArgObject((asUINT)arg, object));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_arg_any(size_t arg, void* ptr, int type_id)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetArgVarType((asUINT)arg, ptr, type_id));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::get_returnable_by_type(void* defer, asITypeInfo* return_type_info)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(defer != nullptr, "return value should be set");
			VI_ASSERT(return_type_info != nullptr, "return type info should be set");
#ifdef VI_ANGELSCRIPT
			VI_ASSERT(return_type_info->GetTypeId() != (int)type_id::void_t, "return value type should not be void");
			void* address = context->GetAddressOfReturnValue();
			if (!address)
				return virtual_exception(virtual_error::invalid_object);

			int type_id = return_type_info->GetTypeId();
			asIScriptEngine* engine = vm->get_engine();
			if (type_id & asTYPEID_OBJHANDLE)
			{
				if (*reinterpret_cast<void**>(defer) == nullptr)
				{
					*reinterpret_cast<void**>(defer) = *reinterpret_cast<void**>(address);
					engine->AddRefScriptObject(*reinterpret_cast<void**>(defer), return_type_info);
					return core::expectation::met;
				}
			}
			else if (type_id & asTYPEID_MASK_OBJECT)
				return function_factory::to_return(engine->AssignScriptObject(defer, address, return_type_info));

			size_t size = engine->GetSizeOfPrimitiveType(return_type_info->GetTypeId());
			if (!size)
				return virtual_exception(virtual_error::invalid_type);

			memcpy(defer, address, size);
			return core::expectation::met;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::get_returnable_by_decl(void* defer, const std::string_view& return_type_decl)
		{
			VI_ASSERT(core::stringify::is_cstring(return_type_decl), "rtdecl should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			return get_returnable_by_type(defer, engine->GetTypeInfoByDecl(return_type_decl.data()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::get_returnable_by_id(void* defer, int return_type_id)
		{
			VI_ASSERT(return_type_id != (int)type_id::void_t, "return value type should not be void");
#ifdef VI_ANGELSCRIPT
			asIScriptEngine* engine = vm->get_engine();
			return get_returnable_by_type(defer, engine->GetTypeInfoById(return_type_id));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_exception(const std::string_view& info, bool allow_catch)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(core::stringify::is_cstring(info), "info should be set");
#ifdef VI_ANGELSCRIPT
			if (!is_suspended() && !executor.deferred_exceptions)
				return function_factory::to_return(context->SetException(info.data(), allow_catch));

			executor.deferred_exception.info = info;
			executor.deferred_exception.allow_catch = allow_catch;
			return core::expectation::met;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_exception_callback(void(*callback)(asIScriptContext* context, void* object), void* object)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetExceptionCallback(asFUNCTION(callback), object, asCALL_CDECL));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_line_callback(void(*callback)(asIScriptContext* context, void* object), void* object)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(callback != nullptr, "callback should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetLineCallback(asFUNCTION(callback), object, asCALL_CDECL));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::start_deserialization()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->StartDeserialization());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::finish_deserialization()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->FinishDeserialization());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::push_function(const function& func, void* object)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->PushFunction(func.get_function(), object));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::get_state_registers(size_t stack_level, function* calling_system_function, function* initial_function, uint32_t* orig_stack_pointer, uint32_t* arguments_size, uint64_t* value_register, void** object_register, type_info* object_type_register)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asITypeInfo* object_type_register1 = nullptr;
			asIScriptFunction* calling_system_function1 = nullptr, * initial_function1 = nullptr;
			asDWORD orig_stack_pointer1 = 0, arguments_size1 = 0; asQWORD value_register1 = 0;
			int r = context->GetStateRegisters((asUINT)stack_level, &calling_system_function1, &initial_function1, &orig_stack_pointer1, &arguments_size1, &value_register1, object_register, &object_type_register1);
			if (calling_system_function != nullptr) *calling_system_function = calling_system_function1;
			if (initial_function != nullptr) *initial_function = initial_function1;
			if (orig_stack_pointer != nullptr) *orig_stack_pointer = (uint32_t)orig_stack_pointer1;
			if (arguments_size != nullptr) *arguments_size = (uint32_t)arguments_size1;
			if (value_register != nullptr) *value_register = (uint64_t)value_register1;
			if (object_type_register != nullptr) *object_type_register = object_type_register1;
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::get_call_state_registers(size_t stack_level, uint32_t* stack_frame_pointer, function* current_function, uint32_t* program_pointer, uint32_t* stack_pointer, uint32_t* stack_index)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asIScriptFunction* current_function1 = nullptr;
			asDWORD stack_frame_pointer1 = 0, program_pointer1 = 0, stack_pointer1 = 0, stack_index1 = 0;
			int r = context->GetCallStateRegisters((asUINT)stack_level, &stack_frame_pointer1, &current_function1, &program_pointer1, &stack_pointer1, &stack_index1);
			if (current_function != nullptr) *current_function = current_function1;
			if (stack_frame_pointer != nullptr) *stack_frame_pointer = (uint32_t)stack_frame_pointer1;
			if (program_pointer != nullptr) *program_pointer = (uint32_t)program_pointer1;
			if (stack_pointer != nullptr) *stack_pointer = (uint32_t)stack_pointer1;
			if (stack_index != nullptr) *stack_index = (uint32_t)stack_index1;
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_state_registers(size_t stack_level, function calling_system_function, const function& initial_function, uint32_t orig_stack_pointer, uint32_t arguments_size, uint64_t value_register, void* object_register, const type_info& object_type_register)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetStateRegisters((asUINT)stack_level, calling_system_function.get_function(), initial_function.get_function(), (asDWORD)orig_stack_pointer, (asDWORD)arguments_size, (asQWORD)value_register, object_register, object_type_register.get_type_info()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::set_call_state_registers(size_t stack_level, uint32_t stack_frame_pointer, const function& current_function, uint32_t program_pointer, uint32_t stack_pointer, uint32_t stack_index)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(context->SetCallStateRegisters((asUINT)stack_level, (asDWORD)stack_frame_pointer, current_function.get_function(), (asDWORD)program_pointer, (asDWORD)stack_pointer, (asDWORD)stack_index));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<size_t> immediate_context::get_args_on_stack_count(size_t stack_level)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			int result = context->GetArgsOnStackCount((asUINT)stack_level);
			return function_factory::to_return<size_t>(result, (size_t)result);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<size_t> immediate_context::get_properties_count(size_t stack_level)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			int result = context->GetVarCount((asUINT)stack_level);
			return function_factory::to_return<size_t>(result, (size_t)result);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> immediate_context::get_property(size_t index, size_t stack_level, std::string_view* name, int* type_id, modifiers* type_modifiers, bool* is_var_on_heap, int* stack_offset)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			const char* asName = "";
			asETypeModifiers type_modifiers1 = asTM_NONE;
			int r = context->GetVar((asUINT)index, (asUINT)stack_level, &asName, type_id, &type_modifiers1, is_var_on_heap, stack_offset);
			if (type_modifiers != nullptr)
				*type_modifiers = (modifiers)type_modifiers1;
			if (name != nullptr)
				*name = asName;
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		function immediate_context::get_function(size_t stack_level)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetFunction((asUINT)stack_level);
#else
			return function(nullptr);
#endif
		}
		int immediate_context::get_line_number(size_t stack_level, int* column, std::string_view* section_name)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			const char* asSectionName = "";
			int r = context->GetLineNumber((asUINT)stack_level, column, &asSectionName);
			if (section_name != nullptr)
				*section_name = asSectionName;
			return r;
#else
			return 0;
#endif
		}
		void immediate_context::set_notification_resolver_callback(std::function<void(immediate_context*)>&& callback)
		{
			callbacks.notification_resolver = std::move(callback);
		}
		void immediate_context::set_callback_resolver_callback(std::function<void(immediate_context*, function_delegate&&, args_callback&&, args_callback&&)>&& callback)
		{
			callbacks.callback_resolver = std::move(callback);
		}
		void immediate_context::set_exception_callback(std::function<void(immediate_context*)>&& callback)
		{
			callbacks.exception = std::move(callback);
		}
		void immediate_context::set_line_callback(std::function<void(immediate_context*)>&& callback)
		{
			callbacks.line = std::move(callback);
			set_line_callback(&virtual_machine::line_handler, this);
		}
		void immediate_context::append_stop_execution_callback(stop_execution_callback&& callback)
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			core::umutex<std::recursive_mutex> unique(exchange);
			callbacks.stop_executions.push_back(std::move(callback));
		}
		void immediate_context::reset()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			context->Unprepare();
			callbacks = events();
			executor = frame();
			invalidate_strings();
#endif
		}
		core::string& immediate_context::copy_string(core::string& value)
		{
			auto& copy = strings.emplace_front(value);
			return copy;
		}
		void immediate_context::invalidate_string(const std::string_view& value)
		{
			auto it = strings.begin();
			while (it != strings.end())
			{
				if (it->data() == value.data())
				{
					strings.erase(it);
					break;
				}
				++it;
			}
		}
		void immediate_context::invalidate_strings()
		{
			strings.clear();
		}
		void immediate_context::disable_suspends()
		{
			++executor.deny_suspends;
		}
		void immediate_context::enable_suspends()
		{
			VI_ASSERT(executor.deny_suspends > 0, "suspends are already enabled");
			--executor.deny_suspends;
		}
		void immediate_context::enable_deferred_exceptions()
		{
			++executor.deferred_exceptions;
		}
		void immediate_context::disable_deferred_exceptions()
		{
			VI_ASSERT(executor.deferred_exceptions > 0, "deferred exceptions are already disabled");
			--executor.deferred_exceptions;
		}
		bool immediate_context::is_nested(size_t* nest_count) const
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			asUINT nest_count1 = 0;
			bool value = context->IsNested(&nest_count1);
			if (nest_count != nullptr) *nest_count = (size_t)nest_count1;
			return value;
#else
			return false;
#endif
		}
		bool immediate_context::is_thrown() const
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			const char* message = context->GetExceptionString();
			return message != nullptr && message[0] != '\0';
#else
			return false;
#endif
		}
		bool immediate_context::is_pending()
		{
			core::umutex<std::recursive_mutex> unique(exchange);
			return executor.future.is_pending();
		}
		execution immediate_context::get_state() const
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return (execution)context->GetState();
#else
			return execution::uninitialized;
#endif
		}
		void* immediate_context::get_address_of_arg(size_t arg)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetAddressOfArg((asUINT)arg);
#else
			return nullptr;
#endif
		}
		unsigned char immediate_context::get_return_byte()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetReturnByte();
#else
			return 0;
#endif
		}
		unsigned short immediate_context::get_return_word()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetReturnWord();
#else
			return 0;
#endif
		}
		size_t immediate_context::get_return_dword()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetReturnDWord();
#else
			return 0;
#endif
		}
		uint64_t immediate_context::get_return_qword()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetReturnQWord();
#else
			return 0;
#endif
		}
		float immediate_context::get_return_float()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetReturnFloat();
#else
			return 0.0f;
#endif
		}
		double immediate_context::get_return_double()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetReturnDouble();
#else
			return 0.0;
#endif
		}
		void* immediate_context::get_return_address()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetReturnAddress();
#else
			return nullptr;
#endif
		}
		void* immediate_context::get_return_object_address()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetReturnObject();
#else
			return nullptr;
#endif
		}
		void* immediate_context::get_address_of_return_value()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetAddressOfReturnValue();
#else
			return nullptr;
#endif
		}
		int immediate_context::get_exception_line_number(int* column, std::string_view* section_name)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			const char* asSectionName = "";
			int r = context->GetExceptionLineNumber(column, &asSectionName);
			if (section_name != nullptr)
				*section_name = asSectionName;
			return r;
#else
			return 0;
#endif
		}
		function immediate_context::get_exception_function()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetExceptionFunction();
#else
			return function(nullptr);
#endif
		}
		std::string_view immediate_context::get_exception_string()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return or_empty(context->GetExceptionString());
#else
			return "";
#endif
		}
		bool immediate_context::will_exception_be_caught()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->WillExceptionBeCaught();
#else
			return false;
#endif
		}
		bool immediate_context::has_deferred_exception()
		{
			return !executor.deferred_exception.info.empty();
		}
		bool immediate_context::rethrow_deferred_exception()
		{
			VI_ASSERT(context != nullptr, "context should be set");
			if (!has_deferred_exception())
				return false;
#ifdef VI_ANGELSCRIPT
			if (context->SetException(executor.deferred_exception.info.c_str(), executor.deferred_exception.allow_catch) != 0)
				return false;

			executor.deferred_exception.info.clear();
			return true;
#else
			return false;
#endif
		}
		void immediate_context::clear_line_callback()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			context->ClearExceptionCallback();
#endif
			callbacks.line = nullptr;
		}
		void immediate_context::clear_exception_callback()
		{
			callbacks.exception = nullptr;
		}
		size_t immediate_context::get_callstack_size() const
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return (size_t)context->GetCallstackSize();
#else
			return 0;
#endif
		}
		std::string_view immediate_context::get_property_name(size_t index, size_t stack_level)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			const char* name = "";
			context->GetVar((asUINT)index, (asUINT)stack_level, &name);
			return or_empty(name);
#else
			return "";
#endif
		}
		std::string_view immediate_context::get_property_decl(size_t index, size_t stack_level, bool include_namespace)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return or_empty(context->GetVarDeclaration((asUINT)index, (asUINT)stack_level, include_namespace));
#else
			return "";
#endif
		}
		int immediate_context::get_property_type_id(size_t index, size_t stack_level)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			int type_id = -1;
			context->GetVar((asUINT)index, (asUINT)stack_level, nullptr, &type_id);
			return type_id;
#else
			return -1;
#endif
		}
		void* immediate_context::get_address_of_property(size_t index, size_t stack_level, bool dont_dereference, bool return_address_of_unitialized_objects)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetAddressOfVar((asUINT)index, (asUINT)stack_level, dont_dereference, return_address_of_unitialized_objects);
#else
			return nullptr;
#endif
		}
		bool immediate_context::is_property_in_scope(size_t index, size_t stack_level)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->IsVarInScope((asUINT)index, (asUINT)stack_level);
#else
			return false;
#endif
		}
		int immediate_context::get_this_type_id(size_t stack_level)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetThisTypeId((asUINT)stack_level);
#else
			return -1;
#endif
		}
		void* immediate_context::get_this_pointer(size_t stack_level)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetThisPointer((asUINT)stack_level);
#else
			return nullptr;
#endif
		}
		core::option<core::string> immediate_context::get_exception_stack_trace()
		{
			core::umutex<std::recursive_mutex> unique(exchange);
			if (executor.stacktrace.empty())
				return core::optional::none;

			return executor.stacktrace;
		}
		function immediate_context::get_system_function()
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetSystemFunction();
#else
			return function(nullptr);
#endif
		}
		bool immediate_context::is_suspended() const
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetState() == asEXECUTION_SUSPENDED;
#else
			return false;
#endif
		}
		bool immediate_context::is_suspendable() const
		{
			return !is_suspended() && !is_sync_locked() && !executor.deny_suspends;
		}
		bool immediate_context::is_sync_locked() const
		{
#ifdef VI_BINDINGS
			VI_ASSERT(context != nullptr, "context should be set");
			return bindings::mutex::is_any_locked((immediate_context*)this);
#else
			return false;
#endif
		}
		bool immediate_context::can_execute_call() const
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			auto state = context->GetState();
			return state != asEXECUTION_SUSPENDED && state != asEXECUTION_ACTIVE && !context->IsNested() && !executor.future.is_pending();
#else
			return false;
#endif
		}
		bool immediate_context::can_execute_subcall() const
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetState() == asEXECUTION_ACTIVE;
#else
			return false;
#endif
		}
		void* immediate_context::set_user_data(void* data, size_t type)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->SetUserData(data, type);
#else
			return nullptr;
#endif
		}
		void* immediate_context::get_user_data(size_t type) const
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			return context->GetUserData(type);
#else
			return nullptr;
#endif
		}
		asIScriptContext* immediate_context::get_context()
		{
			return context;
		}
		virtual_machine* immediate_context::get_vm()
		{
			return vm;
		}
		immediate_context* immediate_context::get(asIScriptContext* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			void* vm = context->GetUserData(context_ud);
			VI_ASSERT(vm != nullptr, "context should be created by virtual machine");
			return (immediate_context*)vm;
#else
			return nullptr;
#endif
		}
		immediate_context* immediate_context::get()
		{
#ifdef VI_ANGELSCRIPT
			asIScriptContext* context = asGetActiveContext();
			if (!context)
				return nullptr;

			return get(context);
#else
			return nullptr;
#endif
		}
		int immediate_context::context_ud = 151;

		virtual_machine::virtual_machine() noexcept : last_major_gc(0), scope(0), debugger(nullptr), engine(nullptr), save_sources(false), save_cache(true)
		{
			auto directory = core::os::directory::get_working();
			if (directory)
				include.root = *directory;

			include.exts.push_back(".as");
			include.exts.push_back(".so");
			include.exts.push_back(".dylib");
			include.exts.push_back(".dll");
#ifdef VI_ANGELSCRIPT
			engine = asCreateScriptEngine();
			engine->SetUserData(this, manager_ud);
			engine->SetContextCallbacks(request_raw_context, return_raw_context, nullptr);
			engine->SetMessageCallback(asFUNCTION(message_logger), this, asCALL_CDECL);
			engine->SetEngineProperty(asEP_INIT_GLOBAL_VARS_AFTER_BUILD, 1);
			engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, 1);
			engine->SetEngineProperty(asEP_DISALLOW_EMPTY_LIST_ELEMENTS, 1);
			engine->SetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, 0);
			engine->SetEngineProperty(asEP_COMPILER_WARNINGS, 1);
			engine->SetEngineProperty(asEP_INCLUDE_JIT_INSTRUCTIONS, 0);
#endif
			set_ts_imports(true);
		}
		virtual_machine::~virtual_machine() noexcept
		{
			for (auto& next : clibraries)
			{
				if (next.second.is_addon)
					uninitialize_addon(next.first, next.second);
				core::os::symbol::unload(next.second.handle);
			}

			for (auto& context : threads)
				core::memory::release(context);
#ifdef VI_ANGELSCRIPT
			for (auto& context : stacks)
			{
				if (context != nullptr)
					context->Release();
			}
#endif
			core::memory::release(debugger);
			cleanup_this_thread();
#ifdef VI_ANGELSCRIPT
			if (engine != nullptr)
			{
				engine->ShutDownAndRelease();
				engine = nullptr;
			}
#endif
			clear_cache();
		}
		expects_vm<type_interface> virtual_machine::set_interface(const std::string_view& name)
		{
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register interface %i bytes", (int)name.size());
#ifdef VI_ANGELSCRIPT
			int type_id = engine->RegisterInterface(name.data());
			return function_factory::to_return<type_interface>(type_id, type_interface(this, engine->GetTypeInfoById(type_id), type_id));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<type_class> virtual_machine::set_struct_address(const std::string_view& name, size_t size, uint64_t flags)
		{
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register struct(%i) %i bytes sizeof %i", (int)flags, (int)name.size(), (int)size);
#ifdef VI_ANGELSCRIPT
			int type_id = engine->RegisterObjectType(name.data(), (asUINT)size, (asDWORD)flags);
			return function_factory::to_return<type_class>(type_id, type_class(this, engine->GetTypeInfoById(type_id), type_id));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<type_class> virtual_machine::set_pod_address(const std::string_view& name, size_t size, uint64_t flags)
		{
			return set_struct_address(name, size, flags);
		}
		expects_vm<ref_class> virtual_machine::set_class_address(const std::string_view& name, size_t size, uint64_t flags)
		{
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class(%i) %i bytes", (int)flags, (int)name.size());
#ifdef VI_ANGELSCRIPT
			int type_id = engine->RegisterObjectType(name.data(), (asUINT)size, (asDWORD)flags);
			return function_factory::to_return<ref_class>(type_id, ref_class(this, engine->GetTypeInfoById(type_id), type_id));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<template_class> virtual_machine::set_template_class_address(const std::string_view& decl, const std::string_view& name, size_t size, uint64_t flags)
		{
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register class(%i) %i bytes", (int)flags, (int)decl.size());
#ifdef VI_ANGELSCRIPT
			int type_id = engine->RegisterObjectType(decl.data(), (asUINT)size, (asDWORD)flags);
			return function_factory::to_return<template_class>(type_id, template_class(this, name));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<enumeration> virtual_machine::set_enum(const std::string_view& name)
		{
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register enum %i bytes", (int)name.size());
#ifdef VI_ANGELSCRIPT
			int type_id = engine->RegisterEnum(name.data());
			return function_factory::to_return<enumeration>(type_id, enumeration(this, engine->GetTypeInfoById(type_id), type_id));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::set_function_def(const std::string_view& decl)
		{
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register funcdef %i bytes", (int)decl.size());
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->RegisterFuncdef(decl.data()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::set_type_def(const std::string_view& type, const std::string_view& decl)
		{
			VI_ASSERT(core::stringify::is_cstring(type), "type should be set");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register funcdef %i bytes", (int)decl.size());
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->RegisterTypedef(type.data(), decl.data()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::set_function_address(const std::string_view& decl, asSFuncPtr* value, convention type)
		{
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(value != nullptr, "value should be set");
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register funcaddr(%i) %i bytes at 0x%" PRIXPTR, (int)type, (int)decl.size(), (void*)value);
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->RegisterGlobalFunction(decl.data(), *value, (asECallConvTypes)type));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::set_property_address(const std::string_view& decl, void* value)
		{
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			VI_ASSERT(value != nullptr, "value should be set");
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_TRACE("[vm] register global %i bytes at 0x%" PRIXPTR, (int)decl.size(), (void*)value);
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->RegisterGlobalProperty(decl.data(), value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::set_string_factory_address(const std::string_view& type, asIStringFactory* factory)
		{
			VI_ASSERT(core::stringify::is_cstring(type), "typename should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->RegisterStringFactory(type.data(), factory));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::get_property_by_index(size_t index, property_info* info) const
		{
#ifdef VI_ANGELSCRIPT
			const char* name = "", * name_space = "";
			const char* config_group = "";
			void* pointer = nullptr;
			bool is_const = false;
			asDWORD access_mask = 0;
			int type_id = 0;
			int result = engine->GetGlobalPropertyByIndex((asUINT)index, &name, &name_space, &type_id, &is_const, &config_group, &pointer, &access_mask);
			if (info != nullptr)
			{
				info->name = or_empty(name);
				info->name_space = or_empty(name_space);
				info->type_id = type_id;
				info->is_const = is_const;
				info->config_group = or_empty(config_group);
				info->pointer = pointer;
				info->access_mask = access_mask;
			}
			return function_factory::to_return(result);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<size_t> virtual_machine::get_property_index_by_name(const std::string_view& name) const
		{
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
#ifdef VI_ANGELSCRIPT
			int r = engine->GetGlobalPropertyIndexByName(name.data());
			return function_factory::to_return<size_t>(r, (size_t)r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<size_t> virtual_machine::get_property_index_by_decl(const std::string_view& decl) const
		{
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			int r = engine->GetGlobalPropertyIndexByDecl(decl.data());
			return function_factory::to_return<size_t>(r, (size_t)r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::set_log_callback(void(*callback)(const asSMessageInfo* message, void* object), void* object)
		{
#ifdef VI_ANGELSCRIPT
			if (!callback)
				return function_factory::to_return(engine->ClearMessageCallback());

			return function_factory::to_return(engine->SetMessageCallback(asFUNCTION(callback), object, asCALL_CDECL));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::log(const std::string_view& section, int row, int column, log_category type, const std::string_view& message)
		{
#ifdef VI_ANGELSCRIPT
			VI_ASSERT(core::stringify::is_cstring(section), "section should be set");
			VI_ASSERT(core::stringify::is_cstring(message), "message should be set");
			return function_factory::to_return(engine->WriteMessage(section.data(), row, column, (asEMsgType)type, message.data()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::assign_object(void* dest_object, void* src_object, const type_info& type)
		{
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->AssignScriptObject(dest_object, src_object, type.get_type_info()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::ref_cast_object(void* object, const type_info& from_type, const type_info& to_type, void** new_ptr, bool use_only_implicit_cast)
		{
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->RefCastObject(object, from_type.get_type_info(), to_type.get_type_info(), new_ptr, use_only_implicit_cast));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::write_message(const std::string_view& section, int row, int column, log_category type, const std::string_view& message)
		{
#ifdef VI_ANGELSCRIPT
			VI_ASSERT(core::stringify::is_cstring(section), "section should be set");
			VI_ASSERT(core::stringify::is_cstring(message), "message should be set");
			return function_factory::to_return(engine->WriteMessage(section.data(), row, column, (asEMsgType)type, message.data()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::garbage_collect(garbage_collector flags, size_t num_iterations)
		{
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->GarbageCollect((asDWORD)flags, (asUINT)num_iterations));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::perform_periodic_garbage_collection(uint64_t interval_ms)
		{
			int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			if (last_major_gc + (int64_t)interval_ms > time)
				return core::expectation::met;

			core::umutex<std::recursive_mutex> unique(sync.general);
			int64_t prev_time = last_major_gc.load() + (int64_t)interval_ms;
			if (prev_time > time)
				return core::expectation::met;

			last_major_gc = time;
			return perform_full_garbage_collection();
		}
		expects_vm<void> virtual_machine::perform_full_garbage_collection()
		{
#ifdef VI_ANGELSCRIPT
			int r = engine->GarbageCollect(asGC_FULL_CYCLE | asGC_DESTROY_GARBAGE | asGC_DETECT_GARBAGE, 1);
			return function_factory::to_return(r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::notify_of_new_object(void* object, const type_info& type)
		{
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->NotifyGarbageCollectorOfNewObject(object, type.get_type_info()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::get_object_address(size_t index, size_t* sequence_number, void** object, type_info* type)
		{
#ifdef VI_ANGELSCRIPT
			asUINT asSequenceNumber;
			asITypeInfo* out_type = nullptr;
			int result = engine->GetObjectInGC((asUINT)index, &asSequenceNumber, object, &out_type);
			if (sequence_number != nullptr)
				*sequence_number = (size_t)asSequenceNumber;
			if (type != nullptr)
				*type = type_info(out_type);
			return function_factory::to_return(result);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::add_script_section(asIScriptModule* library, const std::string_view& name, const std::string_view& code, int line_offset)
		{
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
			VI_ASSERT(core::stringify::is_cstring(code), "code should be set");
			VI_ASSERT(library != nullptr, "module should be set");
#ifdef VI_ANGELSCRIPT
			core::umutex<std::recursive_mutex> unique(sync.general);
			sections[core::string(name)] = core::string(code);
			unique.negate();

			return function_factory::to_return(library->AddScriptSection(name.data(), code.data(), code.size(), line_offset));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::get_type_name_scope(std::string_view* type_name, std::string_view* name_space) const
		{
			VI_ASSERT(type_name != nullptr && core::stringify::is_cstring(*type_name), "typename should be set");
#ifdef VI_ANGELSCRIPT
			const char* value = type_name->data();
			size_t size = strlen(value);
			size_t index = size - 1;

			while (index > 0 && value[index] != ':' && value[index - 1] != ':')
				index--;

			if (index < 1)
			{
				if (name_space != nullptr)
					*name_space = "";
				return virtual_exception(virtual_error::already_registered);
			}

			if (name_space != nullptr)
				*name_space = std::string_view(value, index - 1);

			*type_name = std::string_view(value + index + 1);
			return core::expectation::met;
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::begin_group(const std::string_view& group_name)
		{
			VI_ASSERT(core::stringify::is_cstring(group_name), "group name should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->BeginConfigGroup(group_name.data()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::end_group()
		{
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->EndConfigGroup());
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::remove_group(const std::string_view& group_name)
		{
			VI_ASSERT(core::stringify::is_cstring(group_name), "group name should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->RemoveConfigGroup(group_name.data()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::begin_namespace(const std::string_view& name_space)
		{
			VI_ASSERT(core::stringify::is_cstring(name_space), "namespace should be set");
#ifdef VI_ANGELSCRIPT
			const char* prev = engine->GetDefaultNamespace();
			core::umutex<std::recursive_mutex> unique(sync.general);
			if (prev != nullptr)
				default_namespace = prev;
			else
				default_namespace.clear();

			unique.negate();
			return function_factory::to_return(engine->SetDefaultNamespace(name_space.data()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::begin_namespace_isolated(const std::string_view& name_space, size_t default_mask)
		{
			begin_access_mask(default_mask);
			return begin_namespace(name_space);
		}
		expects_vm<void> virtual_machine::end_namespace_isolated()
		{
			end_access_mask();
			return end_namespace();
		}
		expects_vm<void> virtual_machine::end_namespace()
		{
#ifdef VI_ANGELSCRIPT
			core::umutex<std::recursive_mutex> unique(sync.general);
			return function_factory::to_return(engine->SetDefaultNamespace(default_namespace.c_str()));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<void> virtual_machine::set_property(features property, size_t value)
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return function_factory::to_return(engine->SetEngineProperty((asEEngineProp)property, (asPWORD)value));
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		expects_vm<size_t> virtual_machine::get_size_of_primitive_type(int type_id) const
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			int r = engine->GetSizeOfPrimitiveType(type_id);
			return function_factory::to_return<size_t>(r, (size_t)r);
#else
			return virtual_exception(virtual_error::not_supported);
#endif
		}
		size_t virtual_machine::get_functions_count() const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetGlobalFunctionCount();
#else
			return 0;
#endif
		}
		function virtual_machine::get_function_by_id(int id) const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetFunctionById(id);
#else
			return function(nullptr);
#endif
		}
		function virtual_machine::get_function_by_index(size_t index) const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetGlobalFunctionByIndex((asUINT)index);
#else
			return function(nullptr);
#endif
		}
		function virtual_machine::get_function_by_decl(const std::string_view& decl) const
		{
#ifdef VI_ANGELSCRIPT
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			return engine->GetGlobalFunctionByDecl(decl.data());
#else
			return function(nullptr);
#endif
		}
		size_t virtual_machine::get_properties_count() const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetGlobalPropertyCount();
#else
			return 0;
#endif
		}
		size_t virtual_machine::get_objects_count() const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetObjectTypeCount();
#else
			return 0;
#endif
		}
		type_info virtual_machine::get_object_by_index(size_t index) const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetObjectTypeByIndex((asUINT)index);
#else
			return type_info(nullptr);
#endif
		}
		size_t virtual_machine::get_enum_count() const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetEnumCount();
#else
			return 0;
#endif
		}
		type_info virtual_machine::get_enum_by_index(size_t index) const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetEnumByIndex((asUINT)index);
#else
			return type_info(nullptr);
#endif
		}
		size_t virtual_machine::get_function_defs_count() const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetFuncdefCount();
#else
			return 0;
#endif
		}
		type_info virtual_machine::get_function_def_by_index(size_t index) const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetFuncdefByIndex((asUINT)index);
#else
			return type_info(nullptr);
#endif
		}
		size_t virtual_machine::get_modules_count() const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetModuleCount();
#else
			return 0;
#endif
		}
		asIScriptModule* virtual_machine::get_module_by_id(int id) const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetModuleByIndex(id);
#else
			return nullptr;
#endif
		}
		int virtual_machine::get_type_id_by_decl(const std::string_view& decl) const
		{
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return engine->GetTypeIdByDecl(decl.data());
#else
			return -1;
#endif
		}
		std::string_view virtual_machine::get_type_id_decl(int type_id, bool include_namespace) const
		{
#ifdef VI_ANGELSCRIPT
			return or_empty(engine->GetTypeDeclaration(type_id, include_namespace));
#else
			return "";
#endif
		}
		core::option<core::string> virtual_machine::get_script_section(const std::string_view& section)
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			auto it = sections.find(core::key_lookup_cast(section));
			if (it == sections.end())
				return core::optional::none;

			return it->second;
		}
		type_info virtual_machine::get_type_info_by_id(int type_id) const
		{
#ifdef VI_ANGELSCRIPT
			return engine->GetTypeInfoById(type_id);
#else
			return type_info(nullptr);
#endif
		}
		type_info virtual_machine::get_type_info_by_name(const std::string_view& name)
		{
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
#ifdef VI_ANGELSCRIPT
			std::string_view type_name = name;
			std::string_view name_space = "";
			if (!get_type_name_scope(&type_name, &name_space))
				return engine->GetTypeInfoByName(name.data());

			begin_namespace(core::string(name_space));
			asITypeInfo* info = engine->GetTypeInfoByName(type_name.data());
			end_namespace();

			return info;
#else
			return type_info(nullptr);
#endif
		}
		type_info virtual_machine::get_type_info_by_decl(const std::string_view& decl) const
		{
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
#ifdef VI_ANGELSCRIPT
			return engine->GetTypeInfoByDecl(decl.data());
#else
			return type_info(nullptr);
#endif
		}
		void virtual_machine::set_library_property(library_features property, size_t value)
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			library_settings[property] = value;
		}
		void virtual_machine::set_code_generator(const std::string_view& name, generator_callback&& callback)
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			auto it = generators.find(core::key_lookup_cast(name));
			if (it != generators.end())
			{
				if (callback != nullptr)
					it->second = std::move(callback);
				else
					generators.erase(it);
			}
			else if (callback != nullptr)
				generators[core::string(name)] = std::move(callback);
			else
				generators.erase(core::string(name));
		}
		void virtual_machine::set_preserve_source_code(bool enabled)
		{
			save_sources = enabled;
		}
		void virtual_machine::set_ts_imports(bool enabled, const std::string_view& import_syntax)
		{
			bindings::imports::bind_syntax(this, enabled, import_syntax);
		}
		void virtual_machine::set_cache(bool enabled)
		{
			save_cache = enabled;
		}
		void virtual_machine::set_exception_callback(std::function<void(immediate_context*)>&& callback)
		{
			global_exception = std::move(callback);
		}
		void virtual_machine::set_debugger(debugger_context* context)
		{
			core::umutex<std::recursive_mutex> unique1(sync.general);
			core::memory::release(debugger);
			debugger = context;
			if (debugger != nullptr)
				debugger->set_engine(this);

			core::umutex<std::recursive_mutex> unique2(sync.pool);
			for (auto* next : stacks)
			{
				if (debugger != nullptr)
					attach_debugger_to_context(next);
				else
					detach_debugger_from_context(next);
			}
		}
		void virtual_machine::set_default_array_type(const std::string_view& type)
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_ASSERT(core::stringify::is_cstring(type), "type should be set");
#ifdef VI_ANGELSCRIPT
			engine->RegisterDefaultArrayType(type.data());
#endif
		}
		void virtual_machine::set_type_info_user_data_cleanup_callback(void(*callback)(asITypeInfo*), size_t type)
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			engine->SetTypeInfoUserDataCleanupCallback(callback, (asPWORD)type);
#endif
		}
		void virtual_machine::set_engine_user_data_cleanup_callback(void(*callback)(asIScriptEngine*), size_t type)
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			engine->SetEngineUserDataCleanupCallback(callback, (asPWORD)type);
#endif
		}
		void* virtual_machine::set_user_data(void* data, size_t type)
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return engine->SetUserData(data, (asPWORD)type);
#else
			return nullptr;
#endif
		}
		void* virtual_machine::get_user_data(size_t type) const
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return engine->GetUserData((asPWORD)type);
#else
			return nullptr;
#endif
		}
		void virtual_machine::clear_cache()
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			for (auto data : datas)
				core::memory::release(data.second);

			opcodes.clear();
			datas.clear();
			files.clear();
		}
		void virtual_machine::clear_sections()
		{
			if (debugger != nullptr || save_sources)
				return;

			core::umutex<std::recursive_mutex> unique(sync.general);
			sections.clear();
		}
		void virtual_machine::set_compiler_error_callback(when_error_callback&& callback)
		{
			when_error = std::move(callback);
		}
		void virtual_machine::set_compiler_include_options(const compute::include_desc& new_desc)
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			include = new_desc;
		}
		void virtual_machine::set_compiler_features(const compute::preprocessor::desc& new_desc)
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			proc = new_desc;
		}
		void virtual_machine::set_processor_options(compute::preprocessor* processor)
		{
			VI_ASSERT(processor != nullptr, "preprocessor should be set");
			core::umutex<std::recursive_mutex> unique(sync.general);
			processor->set_include_options(include);
			processor->set_features(proc);
		}
		void virtual_machine::set_compile_callback(const std::string_view& section, compile_callback&& callback)
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			auto it = callbacks.find(core::key_lookup_cast(section));
			if (it != callbacks.end())
			{
				if (callback != nullptr)
					it->second = std::move(callback);
				else
					callbacks.erase(it);
			}
			else if (callback != nullptr)
				callbacks[core::string(section)] = std::move(callback);
			else
				callbacks.erase(core::string(section));
		}
		void virtual_machine::attach_debugger_to_context(asIScriptContext* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			if (!debugger || !debugger->is_attached())
				return detach_debugger_from_context(context);
#ifdef VI_ANGELSCRIPT
			context->SetLineCallback(asMETHOD(debugger_context, line_callback), debugger, asCALL_THISCALL);
			context->SetExceptionCallback(asMETHOD(debugger_context, exception_callback), debugger, asCALL_THISCALL);
#endif
		}
		void virtual_machine::detach_debugger_from_context(asIScriptContext* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
#ifdef VI_ANGELSCRIPT
			context->ClearLineCallback();
			context->SetExceptionCallback(asFUNCTION(virtual_machine::exception_handler), context, asCALL_CDECL);
#endif
		}
		bool virtual_machine::get_byte_code_cache(byte_code_info* info)
		{
			VI_ASSERT(info != nullptr, "bytecode should be set");
			if (!save_cache)
				return false;

			core::umutex<std::recursive_mutex> unique(sync.general);
			auto it = opcodes.find(info->name);
			if (it == opcodes.end())
				return false;

			info->data = it->second.data;
			info->debug = it->second.debug;
			info->valid = true;
			return true;
		}
		void virtual_machine::set_byte_code_cache(byte_code_info* info)
		{
			VI_ASSERT(info != nullptr, "bytecode should be set");
			info->valid = true;
			if (!save_cache)
				return;

			core::umutex<std::recursive_mutex> unique(sync.general);
			opcodes[info->name] = *info;
		}
		immediate_context* virtual_machine::create_context()
		{
#ifdef VI_ANGELSCRIPT
			asIScriptContext* context = engine->RequestContext();
			VI_ASSERT(context != nullptr, "cannot create script context");
			attach_debugger_to_context(context);
			return new immediate_context(context);
#else
			return nullptr;
#endif
		}
		compiler* virtual_machine::create_compiler()
		{
			return new compiler(this);
		}
		asIScriptModule* virtual_machine::create_scoped_module(const std::string_view& name)
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
#ifdef VI_ANGELSCRIPT
			core::umutex<std::recursive_mutex> unique(sync.general);
			if (!engine->GetModule(name.data()))
				return engine->GetModule(name.data(), asGM_ALWAYS_CREATE);

			core::string result;
			while (result.size() < 1024)
			{
				result.assign(name);
				result.append(core::to_string(scope++));
				if (!engine->GetModule(result.c_str()))
					return engine->GetModule(result.c_str(), asGM_ALWAYS_CREATE);
			}
#endif
			return nullptr;
		}
		asIScriptModule* virtual_machine::create_module(const std::string_view& name)
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
#ifdef VI_ANGELSCRIPT
			core::umutex<std::recursive_mutex> unique(sync.general);
			return engine->GetModule(name.data(), asGM_ALWAYS_CREATE);
#else
			return nullptr;
#endif
		}
		void* virtual_machine::create_object(const type_info& type)
		{
#ifdef VI_ANGELSCRIPT
			return engine->CreateScriptObject(type.get_type_info());
#else
			return nullptr;
#endif
		}
		void* virtual_machine::create_object_copy(void* object, const type_info& type)
		{
#ifdef VI_ANGELSCRIPT
			return engine->CreateScriptObjectCopy(object, type.get_type_info());
#else
			return nullptr;
#endif
		}
		void* virtual_machine::create_empty_object(const type_info& type)
		{
#ifdef VI_ANGELSCRIPT
			return engine->CreateUninitializedScriptObject(type.get_type_info());
#else
			return nullptr;
#endif
		}
		function virtual_machine::create_delegate(const function& function, void* object)
		{
#ifdef VI_ANGELSCRIPT
			return engine->CreateDelegate(function.get_function(), object);
#else
			return function;
#endif
		}
		void virtual_machine::release_object(void* object, const type_info& type)
		{
#ifdef VI_ANGELSCRIPT
			engine->ReleaseScriptObject(object, type.get_type_info());
#endif
		}
		void virtual_machine::add_ref_object(void* object, const type_info& type)
		{
#ifdef VI_ANGELSCRIPT
			engine->AddRefScriptObject(object, type.get_type_info());
#endif
		}
		void virtual_machine::get_statistics(unsigned int* current_size, unsigned int* total_destroyed, unsigned int* total_detected, unsigned int* new_objects, unsigned int* total_new_destroyed) const
		{
#ifdef VI_ANGELSCRIPT
			unsigned int asCurrentSize, asTotalDestroyed, asTotalDetected, asNewObjects, asTotalNewDestroyed;
			engine->GetGCStatistics(&asCurrentSize, &asTotalDestroyed, &asTotalDetected, &asNewObjects, &asTotalNewDestroyed);
			if (current_size != nullptr)
				*current_size = (unsigned int)asCurrentSize;

			if (total_destroyed != nullptr)
				*total_destroyed = (unsigned int)asTotalDestroyed;

			if (total_detected != nullptr)
				*total_detected = (unsigned int)asTotalDetected;

			if (new_objects != nullptr)
				*new_objects = (unsigned int)asNewObjects;

			if (total_new_destroyed != nullptr)
				*total_new_destroyed = (unsigned int)asTotalNewDestroyed;
#endif
		}
		void virtual_machine::forward_enum_references(void* reference, const type_info& type)
		{
#ifdef VI_ANGELSCRIPT
			engine->ForwardGCEnumReferences(reference, type.get_type_info());
#endif
		}
		void virtual_machine::forward_release_references(void* reference, const type_info& type)
		{
#ifdef VI_ANGELSCRIPT
			engine->ForwardGCReleaseReferences(reference, type.get_type_info());
#endif
		}
		void virtual_machine::gc_enum_callback(void* reference)
		{
			function_factory::gc_enum_callback(engine, reference);
		}
		void virtual_machine::gc_enum_callback(asIScriptFunction* reference)
		{
			function_factory::gc_enum_callback(engine, reference);
		}
		void virtual_machine::gc_enum_callback(function_delegate* reference)
		{
			function_factory::gc_enum_callback(engine, reference);
		}
		bool virtual_machine::trigger_debugger(immediate_context* context, uint64_t timeout_ms)
		{
			if (!debugger)
				return false;
#ifdef VI_ANGELSCRIPT
			asIScriptContext* target = (context ? context->get_context() : asGetActiveContext());
			if (!target)
				return false;

			debugger->line_callback(target);
			if (timeout_ms > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));

			return true;
#else
			return false;
#endif
		}
		compute::expects_preprocessor<void> virtual_machine::generate_code(compute::preprocessor* processor, const std::string_view& path, core::string& inout_buffer)
		{
			VI_ASSERT(processor != nullptr, "preprocessor should be set");
			if (inout_buffer.empty())
				return core::expectation::met;

			auto status = processor->process(path, inout_buffer);
			if (!status)
				return status;

			std::string_view target_path = path.empty() ? "<anonymous>" : path;
			VI_TRACE("[vm] preprocessor source code generation at %.*s (%" PRIu64 " bytes, %" PRIu64 " generators)", (int)target_path.size(), target_path.data(), (uint64_t)inout_buffer.size(), (uint64_t)generators.size());
			{
				core::unordered_set<core::string> applied_generators;
				core::umutex<std::recursive_mutex> unique(sync.general);
			retry:
				size_t current_generators = generators.size();
				for (auto& item : generators)
				{
					if (applied_generators.find(item.first) != applied_generators.end())
						continue;

					VI_TRACE("[vm] generate source code for %s generator at %.*s (%" PRIu64 " bytes)", item.first.c_str(), (int)target_path.size(), target_path.data(), (uint64_t)inout_buffer.size());
					auto status = item.second(processor, path, inout_buffer);
					if (!status)
						return compute::preprocessor_exception(compute::preprocessor_error::extension_error, 0, status.error().message());

					applied_generators.insert(item.first);
					if (generators.size() != current_generators)
						goto retry;
				}
			}

			(void)target_path;
			return status;
		}
		core::unordered_map<core::string, core::string> virtual_machine::generate_definitions(immediate_context* context)
		{
#ifdef VI_ANGELSCRIPT
			core::unordered_set<core::string> grouping;
			core::unordered_map<core::string, core::string> sources;
			core::unordered_map<core::string, dnamespace> namespaces;
			core::unordered_map<core::string, core::vector<std::pair<core::string, dnamespace*>>> groups;
			auto add_group = [&grouping, &namespaces, &groups](const core::string& current_name)
			{
				auto& group = groups[current_name];
				for (auto& name_space : namespaces)
				{
					if (grouping.find(name_space.first) == grouping.end())
					{
						group.push_back(std::make_pair(name_space.first, &name_space.second));
						grouping.insert(name_space.first);
					}
				}
			};
			auto add_enum = [&namespaces](asITypeInfo* etype)
			{
				const char* enamespace = etype->GetNamespace();
				dnamespace& name_space = namespaces[enamespace ? enamespace : ""];
				denum& enumerator = name_space.enums[etype->GetName()];
				asUINT values_count = etype->GetEnumValueCount();

				for (asUINT j = 0; j < values_count; j++)
				{
					int evalue;
					const char* ename = etype->GetEnumValueByIndex(j, &evalue);
					enumerator.values.push_back(core::stringify::text("%s = %i", ename ? ename : core::to_string(j).c_str(), evalue));
				}
			};
			auto add_object = [this, &namespaces](asITypeInfo* etype)
			{
				asITypeInfo* ebase = etype->GetBaseType();
				const char* cnamespace = etype->GetNamespace();
				const char* cname = etype->GetName();
				dnamespace& name_space = namespaces[cnamespace ? cnamespace : ""];
				dclass& data_class = name_space.classes[cname];
				asUINT types_count = etype->GetSubTypeCount();
				asUINT interfaces_count = etype->GetInterfaceCount();
				asUINT funcdefs_count = etype->GetChildFuncdefCount();
				asUINT props_count = etype->GetPropertyCount();
				asUINT factories_count = etype->GetFactoryCount();
				asUINT methods_count = etype->GetMethodCount();

				if (ebase != nullptr)
					data_class.interfaces.push_back(get_type_naming(ebase));

				for (asUINT j = 0; j < interfaces_count; j++)
				{
					asITypeInfo* itype = etype->GetInterface(j);
					data_class.interfaces.push_back(get_type_naming(itype));
				}

				for (asUINT j = 0; j < types_count; j++)
				{
					int stype_id = etype->GetSubTypeId(j);
					const char* sdecl = engine->GetTypeDeclaration(stype_id, true);
					data_class.types.push_back(core::string("class ") + (sdecl ? sdecl : "__type__"));
				}

				for (asUINT j = 0; j < funcdefs_count; j++)
				{
					asITypeInfo* ftype = etype->GetChildFuncdef(j);
					asIScriptFunction* ffunction = ftype->GetFuncdefSignature();
					const char* fdecl = ffunction->GetDeclaration(false, false, true);
					data_class.funcdefs.push_back(core::string("funcdef ") + (fdecl ? fdecl : "void __unnamed" + core::to_string(j) + "__()"));
				}

				for (asUINT j = 0; j < props_count; j++)
				{
					const char* pname; int ptype_id; bool pprivate, pprotected;
					if (etype->GetProperty(j, &pname, &ptype_id, &pprivate, &pprotected) != 0)
						continue;

					const char* pdecl = engine->GetTypeDeclaration(ptype_id, true);
					const char* pmod = (pprivate ? "private " : (pprotected ? "protected " : nullptr));
					data_class.props.push_back(core::stringify::text("%s%s %s", pmod ? pmod : "", pdecl ? pdecl : "__type__", pname ? pname : ("__unnamed" + core::to_string(j) + "__").c_str()));
				}

				for (asUINT j = 0; j < factories_count; j++)
				{
					asIScriptFunction* ffunction = etype->GetFactoryByIndex(j);
					const char* fdecl = ffunction->GetDeclaration(false, false, true);
					data_class.methods.push_back(fdecl ? core::string(fdecl) : "void " + core::string(cname) + "()");
				}

				for (asUINT j = 0; j < methods_count; j++)
				{
					asIScriptFunction* ffunction = etype->GetMethodByIndex(j);
					const char* fdecl = ffunction->GetDeclaration(false, false, true);
					data_class.methods.push_back(fdecl ? fdecl : "void __unnamed" + core::to_string(j) + "__()");
				}
			};
			auto add_function = [this, &namespaces](asIScriptFunction* ffunction, asUINT index)
			{
				const char* fnamespace = ffunction->GetNamespace();
				const char* fdecl = ffunction->GetDeclaration(false, false, true);

				if (fnamespace != nullptr && *fnamespace != '\0')
				{
					asITypeInfo* ftype = get_type_namespacing(engine, fnamespace);
					if (ftype != nullptr)
					{
						const char* cnamespace = ftype->GetNamespace();
						const char* cname = ftype->GetName();
						dnamespace& name_space = namespaces[cnamespace ? cnamespace : ""];
						dclass& data_class = name_space.classes[cname];
						const char* fdecl = ffunction->GetDeclaration(false, false, true);
						data_class.functions.push_back(fdecl ? fdecl : "void __unnamed" + core::to_string(index) + "__()");
						return;
					}
				}

				dnamespace& name_space = namespaces[fnamespace ? fnamespace : ""];
				name_space.functions.push_back(fdecl ? fdecl : "void __unnamed" + core::to_string(index) + "__()");
			};
			auto add_funcdef = [&namespaces](asITypeInfo* ftype, asUINT index)
			{
				if (ftype->GetParentType() != nullptr)
					return;

				asIScriptFunction* ffunction = ftype->GetFuncdefSignature();
				const char* fnamespace = ftype->GetNamespace();
				dnamespace& name_space = namespaces[fnamespace ? fnamespace : ""];
				const char* fdecl = ffunction->GetDeclaration(false, false, true);
				name_space.funcdefs.push_back(core::string("funcdef ") + (fdecl ? fdecl : "void __unnamed" + core::to_string(index) + "__()"));
			};

			asUINT enums_count = engine->GetEnumCount();
			for (asUINT i = 0; i < enums_count; i++)
				add_enum(engine->GetEnumByIndex(i));

			asUINT objects_count = engine->GetObjectTypeCount();
			for (asUINT i = 0; i < objects_count; i++)
				add_object(engine->GetObjectTypeByIndex(i));

			asUINT functions_count = engine->GetGlobalFunctionCount();
			for (asUINT i = 0; i < functions_count; i++)
				add_function(engine->GetGlobalFunctionByIndex(i), i);

			asUINT funcdefs_count = engine->GetFuncdefCount();
			for (asUINT i = 0; i < funcdefs_count; i++)
				add_funcdef(engine->GetFuncdefByIndex(i), i);

			core::string module_name = "anonymous.as.predefined";
			if (context != nullptr)
			{
				asIScriptFunction* function = context->get_function().get_function();
				if (function != nullptr)
				{
					asIScriptModule* library = function->GetModule();
					if (library != nullptr)
					{
						asUINT enums_count = library->GetEnumCount();
						for (asUINT i = 0; i < enums_count; i++)
							add_enum(library->GetEnumByIndex(i));

						asUINT objects_count = library->GetObjectTypeCount();
						for (asUINT i = 0; i < objects_count; i++)
							add_object(library->GetObjectTypeByIndex(i));

						asUINT functions_count = library->GetFunctionCount();
						for (asUINT i = 0; i < functions_count; i++)
							add_function(library->GetFunctionByIndex(i), i);

						module_name = library->GetName();
						if (core::stringify::ends_with(module_name, ".as"))
							module_name.append(".predefined");
						else
							module_name.append(".as.predefined");
					}
				}
			}

			add_group(module_name);
			for (auto& group : groups)
			{
				core::string offset;
				VI_SORT(group.second.begin(), group.second.end(), [](const auto& a, const auto& b)
				{
					return a.first.size() < b.first.size();
				});

				auto& source = sources[group.first];
				auto& list = group.second;
				for (auto it = list.begin(); it != list.end(); it++)
				{
					auto copy = it;
					dump_namespace(source, it->first, *it->second, offset);
					if (++copy != list.end())
						source += "\n\n";
				}
			}

			return sources;
#else
			return core::unordered_map<core::string, core::string>();
#endif
		}
		size_t virtual_machine::begin_access_mask(size_t default_mask)
		{
#ifdef VI_ANGELSCRIPT
			return engine->SetDefaultAccessMask((asDWORD)default_mask);
#else
			return 0;
#endif
		}
		size_t virtual_machine::end_access_mask()
		{
#ifdef VI_ANGELSCRIPT
			return engine->SetDefaultAccessMask((asDWORD)virtual_machine::get_default_access_mask());
#else
			return 0;
#endif
		}
		std::string_view virtual_machine::get_namespace() const
		{
#ifdef VI_ANGELSCRIPT
			return or_empty(engine->GetDefaultNamespace());
#else
			return "";
#endif
		}
		library virtual_machine::get_module(const std::string_view& name)
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
			VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
#ifdef VI_ANGELSCRIPT
			return library(engine->GetModule(name.data(), asGM_CREATE_IF_NOT_EXISTS));
#else
			return library(nullptr);
#endif
		}
		size_t virtual_machine::get_library_property(library_features property)
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			return library_settings[property];
		}
		size_t virtual_machine::get_property(features property)
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return (size_t)engine->GetEngineProperty((asEEngineProp)property);
#else
			return 0;
#endif
		}
		void virtual_machine::set_module_directory(const std::string_view& value)
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			include.root = value;
			if (include.root.empty())
				return;

			if (!core::stringify::ends_of(include.root, "/\\"))
				include.root.append(1, VI_SPLITTER);
		}
		const core::string& virtual_machine::get_module_directory() const
		{
			return include.root;
		}
		core::vector<core::string> virtual_machine::get_exposed_addons()
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			core::vector<core::string> result;
			result.reserve(addons.size() + clibraries.size());
			for (auto& library : addons)
			{
				if (library.second.exposed)
					result.push_back(core::stringify::text("system(0x%" PRIXPTR "):%s", (void*)&library.second, library.first.c_str()));
			}

			for (auto& library : clibraries)
				result.push_back(core::stringify::text("%s(0x%" PRIXPTR "):%s", library.second.is_addon ? "addon" : "clibrary", library.second.handle, library.first.c_str()));

			return result;
		}
		const core::unordered_map<core::string, virtual_machine::addon>& virtual_machine::get_system_addons() const
		{
			return addons;
		}
		const core::unordered_map<core::string, virtual_machine::clibrary>& virtual_machine::get_clibraries() const
		{
			return clibraries;
		}
		const compute::include_desc& virtual_machine::get_compile_include_options() const
		{
			return include;
		}
		bool virtual_machine::has_library(const std::string_view& name, bool is_addon)
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			auto it = clibraries.find(core::key_lookup_cast(name));
			if (it == clibraries.end())
				return false;

			return it->second.is_addon == is_addon;
		}
		bool virtual_machine::has_system_addon(const std::string_view& name)
		{
			core::umutex<std::recursive_mutex> unique(sync.general);
			auto it = addons.find(core::key_lookup_cast(name));
			if (it == addons.end())
				return false;

			return it->second.exposed;
		}
		bool virtual_machine::has_addon(const std::string_view& name)
		{
			return has_library(name, true);
		}
		bool virtual_machine::is_nullable(int type_id)
		{
			return type_id == 0;
		}
		bool virtual_machine::has_debugger()
		{
			return debugger != nullptr;
		}
		bool virtual_machine::add_system_addon(const std::string_view& name, const core::vector<core::string>& dependencies, addon_callback&& callback)
		{
			VI_ASSERT(!name.empty(), "name should not be empty");
			core::umutex<std::recursive_mutex> unique(sync.general);
			auto it = addons.find(core::key_lookup_cast(name));
			if (it != addons.end())
			{
				if (callback || !dependencies.empty())
				{
					it->second.dependencies = dependencies;
					it->second.callback = std::move(callback);
					it->second.exposed = false;
				}
				else
					addons.erase(it);
			}
			else
			{
				addon result;
				result.dependencies = dependencies;
				result.callback = std::move(callback);
				addons.insert({ core::string(name), std::move(result) });
			}

			if (name == "*")
				return true;

			auto& lists = addons["*"];
			lists.dependencies.push_back(core::string(name));
			lists.exposed = false;
			return true;
		}
		expects_vm<void> virtual_machine::import_file(const std::string_view& path, bool is_remote, core::string& output)
		{
			if (!is_remote)
			{
				if (!core::os::file::is_exists(path))
					return virtual_exception("file not found: " + core::string(path));

				if (!core::stringify::ends_with(path, ".as"))
					return import_addon(path);
			}

			if (!save_cache)
			{
				auto data = core::os::file::read_as_string(path);
				if (!data)
					return virtual_exception(core::copy<core::string>(data.error().message()) + ": " + core::string(path));

				output.assign(*data);
				return core::expectation::met;
			}

			core::umutex<std::recursive_mutex> unique(sync.general);
			auto it = files.find(core::key_lookup_cast(path));
			if (it != files.end())
			{
				output.assign(it->second);
				return core::expectation::met;
			}

			unique.negate();
			auto data = core::os::file::read_as_string(path);
			if (!data)
				return virtual_exception(core::copy<core::string>(data.error().message()) + ": " + core::string(path));

			output.assign(*data);
			unique.negate();
			files.insert(std::make_pair(path, output));
			return core::expectation::met;
		}
		expects_vm<void> virtual_machine::import_cfunction(const core::vector<core::string>& sources, const std::string_view& func, const std::string_view& decl)
		{
			VI_ASSERT(core::stringify::is_cstring(func), "func should be set");
			VI_ASSERT(core::stringify::is_cstring(decl), "decl should be set");
			if (!engine || decl.empty() || func.empty())
				return virtual_exception("import cfunction: invalid argument");

			auto load_function = [this, &func, &decl](clibrary& context) -> expects_vm<void>
			{
				auto handle = context.functions.find(core::key_lookup_cast(func));
				if (handle != context.functions.end())
					return core::expectation::met;

				auto function_handle = core::os::symbol::load_function(context.handle, func);
				if (!function_handle)
					return virtual_exception(core::copy<core::string>(function_handle.error().message()) + ": " + core::string(func));

				function_ptr function = (function_ptr)*function_handle;
				if (!function)
					return virtual_exception("cfunction not found: " + core::string(func));
#ifdef VI_ANGELSCRIPT
				VI_TRACE("[vm] register global funcaddr(%i) %i bytes at 0x%" PRIXPTR, (int)asCALL_CDECL, (int)decl.size(), (void*)function);
				int result = engine->RegisterGlobalFunction(decl.data(), asFUNCTION(function), asCALL_CDECL);
				if (result < 0)
					return virtual_exception((virtual_error)result, "register cfunction error: " + core::string(func));

				context.functions.insert({ core::string(func), { core::string(decl), (void*)function } });
				VI_DEBUG("[vm] load function %.*s", (int)func.size(), func.data());
				return core::expectation::met;
#else
				(void)this;
				(void)decl;
				return virtual_exception("cfunction not found: not supported");
#endif
			};

			core::umutex<std::recursive_mutex> unique(sync.general);
			auto it = clibraries.end();
			for (auto& item : sources)
			{
				it = clibraries.find(item);
				if (it != clibraries.end())
					break;
			}

			if (it != clibraries.end())
				return load_function(it->second);

			for (auto& item : clibraries)
			{
				auto status = load_function(item.second);
				if (status)
					return status;
			}

			return virtual_exception("cfunction not found: " + core::string(func));
		}
		expects_vm<void> virtual_machine::import_clibrary(const std::string_view& path, bool is_addon)
		{
			auto name = get_library_name(path);
			if (!engine)
				return virtual_exception("import clibrary: invalid argument");

			core::umutex<std::recursive_mutex> unique(sync.general);
			auto core = clibraries.find(core::key_lookup_cast(name));
			if (core != clibraries.end())
				return core::expectation::met;

			unique.negate();
			auto handle = core::os::symbol::load(path);
			if (!handle)
			{
				core::string directory = core::os::path::get_directory(path);
				std::string_view file = core::os::path::get_filename(path);
				handle = core::os::symbol::load(core::stringify::text("%slib%.*s", directory.c_str(), (int)file.size(), file.data()));
				if (!handle)
				{
					handle = core::os::symbol::load(core::stringify::text("%s%.*slib", directory.c_str(), (int)file.size(), file.data()));
					 if (!handle)
						return virtual_exception(core::copy<core::string>(handle.error().message()) + ": " + core::string(path));
				}
			}
			
			clibrary library;
			library.handle = *handle;
			library.is_addon = is_addon;

			if (library.is_addon)
			{
				auto status = initialize_addon(name, library);
				if (!status)
				{
					uninitialize_addon(name, library);
					core::os::symbol::unload(*handle);
					return status;
				}
			}

			unique.negate();
			clibraries.insert({ core::string(name), std::move(library) });
			VI_DEBUG("[vm] load library %.*s", (int)path.size(), path.data());
			return core::expectation::met;
		}
		expects_vm<void> virtual_machine::import_system_addon(const std::string_view& name)
		{
			if (!core::os::control::has(core::access_option::addons))
				return virtual_exception("import system addon: denied");

			core::string target = core::string(name);
			if (core::stringify::ends_with(target, ".as"))
				target = target.substr(0, target.size() - 3);

			core::umutex<std::recursive_mutex> unique(sync.general);
			auto it = addons.find(target);
			if (it == addons.end())
				return virtual_exception("system addon not found: " + core::string(name));

			if (it->second.exposed)
				return core::expectation::met;

			addon base = it->second;
			it->second.exposed = true;
			unique.negate();

			for (auto& item : base.dependencies)
			{
				auto status = import_system_addon(item);
				if (!status)
					return status;
			}

			if (base.callback)
				base.callback(this);

			VI_DEBUG("[vm] load system addon %.*s", (int)name.size(), name.data());
			return core::expectation::met;
		}
		expects_vm<void> virtual_machine::import_addon(const std::string_view& name)
		{
			return import_clibrary(name, true);
		}
		expects_vm<void> virtual_machine::initialize_addon(const std::string_view& path, clibrary& library)
		{
			auto addon_import_handle = core::os::symbol::load_function(library.handle, "addon_import");
			if (!addon_import_handle)
				return virtual_exception("import user addon: no initialization routine (path = " + core::string(path) + ")");

			auto addon_import = (int(*)())*addon_import_handle;
			if (!addon_import)
				return virtual_exception("import user addon: no initialization routine (path = " + core::string(path) + ")");

			int code = addon_import();
			if (code != 0)
				return virtual_exception("import user addon: initialization failed (path = " + core::string(path) + ", exit = " + core::to_string(code) + ")");

			VI_DEBUG("[vm] addon library %.*s initializated", (int)path.size(), path.data());
			library.functions.insert({ "addon_import", { core::string(), (void*)addon_import } });
			return core::expectation::met;
		}
		void virtual_machine::uninitialize_addon(const std::string_view& name, clibrary& library)
		{
			auto addon_cleanup_handle = core::os::symbol::load_function(library.handle, "addon_cleanup");
			if (!addon_cleanup_handle)
				return;

			auto addon_cleanup = (void(*)()) * addon_cleanup_handle;
			if (addon_cleanup != nullptr)
			{
				library.functions.insert({ "addon_cleanup", { core::string(), (void*)addon_cleanup } });
				addon_cleanup();
			}
		}
		core::option<core::string> virtual_machine::get_source_code_appendix(const std::string_view& label, const std::string_view& code, uint32_t line_number, uint32_t column_number, size_t max_lines)
		{
			if (max_lines % 2 == 0)
				++max_lines;

			core::vector<core::string> lines = extract_lines_of_code(code, (int)line_number, (int)max_lines);
			if (lines.empty())
				return core::optional::none;

			core::string line = lines.front();
			lines.erase(lines.begin());
			if (line.empty())
				return core::optional::none;

			core::string_stream stream;
			size_t top_lines = (lines.size() % 2 != 0 ? 1 : 0) + lines.size() / 2;
			size_t top_line = top_lines < line_number ? line_number - top_lines - 1 : line_number;
			stream << "\n  last " << (lines.size() + 1) << " lines of " << label << " code\n";

			for (size_t i = 0; i < top_lines; i++)
				stream << "  " << top_line++ << "  " << core::stringify::trim_end(lines[i]) << "\n";
			stream << "  " << top_line++ << "  " << core::stringify::trim_end(line) << "\n  ";

			column_number += 1 + (uint32_t)core::to_string(line_number).size();
			for (uint32_t i = 0; i < column_number; i++)
				stream << " ";
			stream << "^";

			for (size_t i = top_lines; i < lines.size(); i++)
				stream << "\n  " << top_line++ << "  " << core::stringify::trim_end(lines[i]);

			return stream.str();
		}
		core::option<core::string> virtual_machine::get_source_code_appendix_by_path(const std::string_view& label, const std::string_view& path, uint32_t line_number, uint32_t column_number, size_t max_lines)
		{
			auto code = get_script_section(path);
			if (!code)
				return code;

			return get_source_code_appendix(label, *code, line_number, column_number, max_lines);
		}
		size_t virtual_machine::get_property(features property) const
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			return (size_t)engine->GetEngineProperty((asEEngineProp)property);
#else
			return 0;
#endif
		}
		asIScriptEngine* virtual_machine::get_engine() const
		{
			return engine;
		}
		debugger_context* virtual_machine::get_debugger() const
		{
			return debugger;
		}
		void virtual_machine::cleanup()
		{
			type_cache::cleanup();
			cleanup_this_thread();
		}
		virtual_machine* virtual_machine::get(asIScriptEngine* engine)
		{
			VI_ASSERT(engine != nullptr, "engine should be set");
#ifdef VI_ANGELSCRIPT
			void* vm = engine->GetUserData(manager_ud);
			VI_ASSERT(vm != nullptr, "engine should be created by virtual machine");
			return (virtual_machine*)vm;
#else
			return nullptr;
#endif
		}
		virtual_machine* virtual_machine::get()
		{
#ifdef VI_ANGELSCRIPT
			asIScriptContext* context = asGetActiveContext();
			if (!context)
				return nullptr;

			return get(context->GetEngine());
#else
			return nullptr;
#endif
		}
		std::string_view virtual_machine::get_library_name(const std::string_view& path)
		{
			if (path.empty())
				return path;

			core::text_settle start = core::stringify::reverse_find_of(path, "\\/");
			if (!start.found)
				return path;

			return path.substr(start.end);
		}
		immediate_context* virtual_machine::request_context()
		{
			core::umutex<std::recursive_mutex> unique(sync.pool);
			if (threads.empty())
			{
				unique.negate();
				return create_context();
			}

			immediate_context* context = *threads.rbegin();
			threads.pop_back();
			return context;
		}
		void virtual_machine::return_context(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			core::umutex<std::recursive_mutex> unique(sync.pool);
			threads.push_back(context);
			context->reset();
		}
		asIScriptContext* virtual_machine::request_raw_context(asIScriptEngine* engine, void* data)
		{
#ifdef VI_ANGELSCRIPT
			virtual_machine* vm = virtual_machine::get(engine);
			if (!vm || vm->stacks.empty())
				return engine->CreateContext();

			core::umutex<std::recursive_mutex> unique(vm->sync.pool);
			if (vm->stacks.empty())
			{
				unique.negate();
				return engine->CreateContext();
			}

			asIScriptContext* context = *vm->stacks.rbegin();
			vm->stacks.pop_back();
			return context;
#else
			return nullptr;
#endif
		}
		void virtual_machine::return_raw_context(asIScriptEngine* engine, asIScriptContext* context, void* data)
		{
#ifdef VI_ANGELSCRIPT
			virtual_machine* vm = virtual_machine::get(engine);
			VI_ASSERT(vm != nullptr, "engine should be set");

			core::umutex<std::recursive_mutex> unique(vm->sync.pool);
			vm->stacks.push_back(context);
			context->Unprepare();
#endif
		}
		void virtual_machine::line_handler(asIScriptContext* context, void*)
		{
			immediate_context* base = immediate_context::get(context);
			VI_ASSERT(base != nullptr, "context should be set");
			VI_ASSERT(base->callbacks.line != nullptr, "callback should be set");
			base->callbacks.line(base);
		}
		void virtual_machine::exception_handler(asIScriptContext* context, void*)
		{
#ifdef VI_ANGELSCRIPT
			immediate_context* base = immediate_context::get(context);
			VI_ASSERT(base != nullptr, "context should be set");

			virtual_machine* vm = base->get_vm();
			if (vm->debugger != nullptr)
				return vm->debugger->throw_internal_exception(context->GetExceptionString());

			const char* message = context->GetExceptionString();
			if (message && message[0] != '\0' && !context->WillExceptionBeCaught())
			{
				core::string details = bindings::exception::pointer(core::string(message)).what();
				core::string trace = core::error_handling::get_stack_trace(1, 64);
				VI_ERR("[vm] uncaught exception %s, callstack:\n%.*s", details.empty() ? "unknown" : details.c_str(), (int)trace.size(), trace.c_str());
				core::umutex<std::recursive_mutex> unique(base->exchange);
				base->executor.stacktrace = trace;
				unique.negate();

				if (base->callbacks.exception)
					base->callbacks.exception(base);
				else if (vm->global_exception)
					vm->global_exception(base);
			}
			else if (base->callbacks.exception)
				base->callbacks.exception(base);
			else if (vm->global_exception)
				vm->global_exception(base);
#endif
		}
		void virtual_machine::set_memory_functions(void* (*alloc)(size_t), void(*free)(void*))
		{
#ifdef VI_ANGELSCRIPT
			asSetGlobalMemoryFunctions(alloc, free);
#endif
		}
		void virtual_machine::cleanup_this_thread()
		{
#ifdef VI_ANGELSCRIPT
			asThreadCleanup();
#endif
		}
		std::string_view virtual_machine::get_error_name_info(virtual_error code)
		{
			switch (code)
			{
				case virtual_error::success:
					return "OK";
				case virtual_error::err:
					return "invalid operation";
				case virtual_error::context_active:
					return "context is in use";
				case virtual_error::context_not_finished:
					return "context is not finished";
				case virtual_error::context_not_prepared:
					return "context is not prepared";
				case virtual_error::invalid_arg:
					return "invalid argument";
				case virtual_error::no_function:
					return "function not found";
				case virtual_error::not_supported:
					return "operation not supported";
				case virtual_error::invalid_name:
					return "invalid name argument";
				case virtual_error::name_taken:
					return "name is in use";
				case virtual_error::invalid_declaration:
					return "invalid code declaration";
				case virtual_error::invalid_object:
					return "invalid object argument";
				case virtual_error::invalid_type:
					return "invalid type argument";
				case virtual_error::already_registered:
					return "type is already registered";
				case virtual_error::multiple_functions:
					return "function overload is not deducible";
				case virtual_error::no_module:
					return "module not found";
				case virtual_error::no_global_var:
					return "global variable not found";
				case virtual_error::invalid_configuration:
					return "invalid configuration state";
				case virtual_error::invalid_interface:
					return "invalid interface type";
				case virtual_error::cant_bind_all_functions:
					return "function binding failed";
				case virtual_error::lower_array_dimension_not_registered:
					return "lower array dimension not registered";
				case virtual_error::wrong_config_group:
					return "invalid configuration group";
				case virtual_error::config_group_is_in_use:
					return "configuration group is in use";
				case virtual_error::illegal_behaviour_for_type:
					return "illegal type behaviour configuration";
				case virtual_error::wrong_calling_conv:
					return "illegal function calling convention";
				case virtual_error::build_in_progress:
					return "program compiler is in use";
				case virtual_error::init_global_vars_failed:
					return "global variable initialization failed";
				case virtual_error::out_of_memory:
					return "out of memory";
				case virtual_error::module_is_in_use:
					return "module is in use";
				default:
					return "internal operation failed";
			}
		}
		byte_code_label virtual_machine::get_byte_code_info(uint8_t code)
		{
#ifdef VI_ANGELSCRIPT
			auto& source = asBCInfo[code];
			byte_code_label label;
			label.name = source.name;
			label.code = (uint8_t)source.bc;
			label.size = asBCTypeSize[source.type];
			label.offset_of_stack = source.stackInc;
			label.size_of_arg0 = 0;
			label.size_of_arg1 = 0;
			label.size_of_arg2 = 0;

			switch (source.type)
			{
				case asBCTYPE_W_ARG:
				case asBCTYPE_wW_ARG:
				case asBCTYPE_rW_ARG:
					label.size_of_arg0 = sizeof(asWORD);
					break;
				case asBCTYPE_DW_ARG:
					label.size_of_arg0 = sizeof(asDWORD);
					break;
				case asBCTYPE_wW_DW_ARG:
				case asBCTYPE_rW_DW_ARG:
					label.size_of_arg0 = sizeof(asWORD);
					label.size_of_arg1 = sizeof(asDWORD);
					break;
				case asBCTYPE_QW_ARG:
					label.size_of_arg0 = sizeof(asQWORD);
					break;
				case asBCTYPE_DW_DW_ARG:
					label.size_of_arg0 = sizeof(asDWORD);
					label.size_of_arg1 = sizeof(asDWORD);
					break;
				case asBCTYPE_wW_rW_rW_ARG:
					label.size_of_arg0 = sizeof(asWORD);
					label.size_of_arg1 = sizeof(asWORD);
					label.size_of_arg2 = sizeof(asWORD);
					break;
				case asBCTYPE_wW_QW_ARG:
					label.size_of_arg0 = sizeof(asWORD);
					label.size_of_arg1 = sizeof(asQWORD);
					break;
				case asBCTYPE_wW_rW_ARG:
				case asBCTYPE_rW_rW_ARG:
					label.size_of_arg0 = sizeof(asWORD);
					label.size_of_arg0 = sizeof(asWORD);
					break;
				case asBCTYPE_wW_rW_DW_ARG:
					label.size_of_arg0 = sizeof(asWORD);
					label.size_of_arg1 = sizeof(asWORD);
					label.size_of_arg2 = sizeof(asDWORD);
					break;
				case asBCTYPE_QW_DW_ARG:
					label.size_of_arg0 = sizeof(asQWORD);
					label.size_of_arg1 = sizeof(asWORD);
					break;
				case asBCTYPE_rW_QW_ARG:
					label.size_of_arg0 = sizeof(asWORD);
					label.size_of_arg1 = sizeof(asQWORD);
					break;
				case asBCTYPE_W_DW_ARG:
					label.size_of_arg0 = sizeof(asWORD);
					break;
				case asBCTYPE_rW_W_DW_ARG:
					label.size_of_arg0 = sizeof(asWORD);
					label.size_of_arg1 = sizeof(asWORD);
					break;
				case asBCTYPE_rW_DW_DW_ARG:
					label.size_of_arg0 = sizeof(asWORD);
					label.size_of_arg1 = sizeof(asDWORD);
					label.size_of_arg2 = sizeof(asDWORD);
					break;
				default:
					break;
			}

			label.offset_of_arg0 = 1;
			label.offset_of_arg1 = label.offset_of_arg0 + label.size_of_arg0;
			label.offset_of_arg2 = label.offset_of_arg1 + label.size_of_arg1;
			return label;
#else
			return byte_code_label();
#endif
		}
		void virtual_machine::message_logger(asSMessageInfo* info, void* this_engine)
		{
#ifdef VI_ANGELSCRIPT
			virtual_machine* engine = (virtual_machine*)this_engine;
			const char* section = (info->section && info->section[0] != '\0' ? info->section : "?");
			if (engine->when_error)
				engine->when_error();

			auto source_code = engine->get_source_code_appendix_by_path("error", section, info->row, info->col, 5);
			if (engine != nullptr && !engine->callbacks.empty())
			{
				auto it = engine->callbacks.find(section);
				if (it != engine->callbacks.end())
				{
					if (info->type == asMSGTYPE_WARNING)
						return it->second(core::stringify::text("WARN %i: %s%s", info->row, info->message, source_code ? source_code->c_str() : ""));
					else if (info->type == asMSGTYPE_INFORMATION)
						return it->second(core::stringify::text("INFO %s", info->message));

					return it->second(core::stringify::text("ERR %i: %s%s", info->row, info->message, source_code ? source_code->c_str() : ""));
				}
			}

			if (info->type == asMSGTYPE_WARNING)
				VI_WARN("[asc] %s:%i: %s%s", section, info->row, info->message, source_code ? source_code->c_str() : "");
			else if (info->type == asMSGTYPE_INFORMATION)
				VI_INFO("[asc] %s", info->message);
			else if (info->type == asMSGTYPE_ERROR)
				VI_ERR("[asc] %s: %i: %s%s", section, info->row, info->message, source_code ? source_code->c_str() : "");
#endif
		}
		size_t virtual_machine::get_default_access_mask()
		{
			return 0xFFFFFFFF;
		}
		void* virtual_machine::get_nullable()
		{
			return nullptr;
		}
		int virtual_machine::manager_ud = 553;

		event_loop::callable::callable(immediate_context* new_context) noexcept : context(new_context)
		{
		}
		event_loop::callable::callable(immediate_context* new_context, function_delegate&& new_delegate, args_callback&& new_on_args, args_callback&& new_on_return) noexcept : delegation(std::move(new_delegate)), on_args(std::move(new_on_args)), on_return(std::move(new_on_return)), context(new_context)
		{
		}
		event_loop::callable::callable(const callable& other) noexcept : delegation(other.delegation), on_args(other.on_args), on_return(other.on_return), context(other.context)
		{
		}
		event_loop::callable::callable(callable&& other) noexcept : delegation(std::move(other.delegation)), on_args(std::move(other.on_args)), on_return(std::move(other.on_return)), context(other.context)
		{
			other.context = nullptr;
		}
		event_loop::callable& event_loop::callable::operator= (const callable& other) noexcept
		{
			if (this == &other)
				return *this;

			delegation = other.delegation;
			on_args = other.on_args;
			on_return = other.on_return;
			context = other.context;
			return *this;
		}
		event_loop::callable& event_loop::callable::operator= (callable&& other) noexcept
		{
			if (this == &other)
				return *this;

			delegation = std::move(other.delegation);
			on_args = std::move(other.on_args);
			on_return = std::move(other.on_return);
			context = other.context;
			other.context = nullptr;
			return *this;
		}
		bool event_loop::callable::is_notification() const
		{
			return !delegation.is_valid();
		}
		bool event_loop::callable::is_callback() const
		{
			return delegation.is_valid();
		}

		static thread_local event_loop* internal_loop = nullptr;
		event_loop::event_loop() noexcept : aborts(false), wake(false)
		{
		}
		void event_loop::on_notification(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			core::umutex<std::mutex> unique(mutex);
			queue.push(callable(context));
			waitable.notify_one();

			auto ready = std::move(on_enqueue);
			unique.negate();
			if (ready)
				ready(context);
		}
		void event_loop::on_callback(immediate_context* context, function_delegate&& delegatef, args_callback&& on_args, args_callback&& on_return)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			VI_ASSERT(delegatef.is_valid(), "delegate should be valid");
			core::umutex<std::mutex> unique(mutex);
			queue.push(callable(context, std::move(delegatef), std::move(on_args), std::move(on_return)));
			waitable.notify_one();

			auto ready = std::move(on_enqueue);
			unique.negate();
			if (ready)
				ready(context);
		}
		void event_loop::wakeup()
		{
			core::umutex<std::mutex> unique(mutex);
			wake = true;
			waitable.notify_all();
		}
		void event_loop::restore()
		{
			core::umutex<std::mutex> unique(mutex);
			aborts = false;
		}
		void event_loop::abort()
		{
			core::umutex<std::mutex> unique(mutex);
			aborts = true;
		}
		void event_loop::when(args_callback&& callback)
		{
			on_enqueue = std::move(callback);
		}
		void event_loop::listen(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			context->set_notification_resolver_callback(std::bind(&event_loop::on_notification, this, std::placeholders::_1));
			context->set_callback_resolver_callback(std::bind(&event_loop::on_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		}
		void event_loop::unlisten(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			context->set_notification_resolver_callback(nullptr);
			context->set_callback_resolver_callback(nullptr);
		}
		void event_loop::enqueue(immediate_context* context)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			core::umutex<std::mutex> unique(mutex);
			queue.push(callable(context));
			waitable.notify_one();

			auto ready = std::move(on_enqueue);
			unique.negate();
			if (ready)
				ready(context);
		}
		void event_loop::enqueue(function_delegate&& delegatef, args_callback&& on_args, args_callback&& on_return)
		{
			VI_ASSERT(delegatef.is_valid(), "delegate should be valid");
			immediate_context* context = delegatef.context;
			core::umutex<std::mutex> unique(mutex);
			queue.push(callable(delegatef.context, std::move(delegatef), std::move(on_args), std::move(on_return)));
			waitable.notify_one();

			auto ready = std::move(on_enqueue);
			unique.negate();
			if (ready)
				ready(context);
		}
		bool event_loop::poll(immediate_context* context, uint64_t timeout_ms)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			if (!bindings::promise::is_context_busy(context) && queue.empty())
				return false;
			else if (!timeout_ms)
				return true;

			std::unique_lock<std::mutex> unique(mutex);
			waitable.wait_for(unique, std::chrono::milliseconds(timeout_ms), [this]() { return !queue.empty() || wake; });
			wake = false;
			return true;
		}
		bool event_loop::poll_extended(immediate_context* context, uint64_t timeout_ms)
		{
			VI_ASSERT(context != nullptr, "context should be set");
			if (!bindings::promise::is_context_busy(context) && queue.empty())
			{
				if (!core::schedule::is_available() || !core::schedule::get()->get_policy().parallel)
					return false;
			}
			else if (!timeout_ms)
				return true;

			std::unique_lock<std::mutex> unique(mutex);
			waitable.wait_for(unique, std::chrono::milliseconds(timeout_ms), [this]() { return !queue.empty() || wake; });
			wake = false;
			return true;
		}
		size_t event_loop::dequeue(virtual_machine* vm, size_t max_executions)
		{
			VI_ASSERT(vm != nullptr, "virtual machine should be set");
			core::umutex<std::mutex> unique(mutex);
			size_t executions = 0;

			while (!queue.empty())
			{
				callable next = std::move(queue.front());
				queue.pop();
				if (aborts)
					continue;

				unique.negate();
				immediate_context* initiator_context = next.context;
				if (next.is_callback())
				{
					immediate_context* executing_context = next.context;
					if (!next.context->can_execute_call())
					{
						executing_context = vm->request_context();
						listen(executing_context);
					}

					if (next.on_return)
					{
						args_callback on_return = std::move(next.on_return);
						executing_context->execute_call(next.delegation.callable(), std::move(next.on_args)).when([this, vm, initiator_context, executing_context, on_return = std::move(on_return)](expects_vm<execution>&& status) mutable
						{
							on_return(executing_context);
							if (executing_context != initiator_context)
								vm->return_context(executing_context);
							abort_if(std::move(status));
						});
					}
					else
					{
						executing_context->execute_call(next.delegation.callable(), std::move(next.on_args)).when([this, vm, initiator_context, executing_context](expects_vm<execution>&& status) mutable
						{
							if (executing_context != initiator_context)
								vm->return_context(executing_context);
							abort_if(std::move(status));
						});
					}
					++executions;
				}
				else if (next.is_notification())
				{
					if (initiator_context->is_suspended())
						abort_if(initiator_context->resume());
					++executions;
				}
				unique.negate();
				if (max_executions > 0 && executions >= max_executions)
					break;
			}

			if (executions > 0)
				VI_TRACE("[vm] loop process %" PRIu64 " events", (uint64_t)executions);
			return executions;
		}
		void event_loop::abort_if(expects_vm<execution>&& status)
		{
			if (status && *status == execution::aborted)
				abort();
		}
		bool event_loop::is_aborted()
		{
			return aborts;
		}
		void event_loop::set(event_loop* for_current_thread)
		{
			internal_loop = for_current_thread;
		}
		event_loop* event_loop::get()
		{
			return internal_loop;
		}
	}
}
