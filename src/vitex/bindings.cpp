#include "bindings.h"
#include "internal/types.hpp"
#include <sstream>
#ifdef VI_BINDINGS
#include "network.h"
#include "network/http.h"
#include "network/smtp.h"
#include "network/sqlite.h"
#include "network/pq.h"
#include "network/mongo.h"
#include "layer/processors.h"
#endif
#ifdef VI_ANGELSCRIPT
#include <angelscript.h>
#endif

namespace
{
#ifdef VI_ANGELSCRIPT
	class string_factory : public asIStringFactory
	{
	private:
		static string_factory* base;

	public:
		vitex::core::unordered_map<vitex::core::string, std::atomic<int32_t>> cache;

	public:
		string_factory()
		{
		}
		~string_factory()
		{
			VI_ASSERT(cache.size() == 0, "some strings are still in use");
		}
		const void* GetStringConstant(const char* buffer, asUINT length) override
		{
			VI_ASSERT(buffer != nullptr, "buffer must not be null");

			asAcquireSharedLock();
			std::string_view source(buffer, length);
			auto it = cache.find(vitex::core::key_lookup_cast(source));
			if (it != cache.end())
			{
				it->second++;
				asReleaseSharedLock();
				return reinterpret_cast<const void*>(&it->first);
			}

			asReleaseSharedLock();
			asAcquireExclusiveLock();
			it = cache.insert(std::make_pair(std::move(source), 1)).first;
			asReleaseExclusiveLock();
			return reinterpret_cast<const void*>(&it->first);
		}
		int ReleaseStringConstant(const void* source) override
		{
			VI_ASSERT(source != nullptr, "source must not be null");
			asAcquireSharedLock();
			auto it = cache.find(*reinterpret_cast<const vitex::core::string*>(source));
			if (it == cache.end())
			{
				asReleaseSharedLock();
				return asERROR;
			}
			else if (--it->second > 0)
			{
				asReleaseSharedLock();
				return asSUCCESS;
			}

			asReleaseSharedLock();
			asAcquireExclusiveLock();
			cache.erase(it);
			asReleaseExclusiveLock();
			return asSUCCESS;
		}
		int GetRawStringData(const void* source, char* buffer, asUINT* length) const override
		{
			VI_ASSERT(source != nullptr, "source must not be null");
			if (length != nullptr)
				*length = (asUINT)reinterpret_cast<const vitex::core::string*>(source)->length();

			if (buffer != nullptr)
				memcpy(buffer, reinterpret_cast<const vitex::core::string*>(source)->c_str(), reinterpret_cast<const vitex::core::string*>(source)->length());

			return asSUCCESS;
		}

	public:
		static string_factory* get()
		{
			if (!base)
				base = vitex::core::memory::init<string_factory>();

			return base;
		}
		static void free()
		{
			if (base != nullptr && base->cache.empty())
				vitex::core::memory::deinit(base);
		}
	};
	string_factory* string_factory::base = nullptr;
#endif
	struct file_link : public vitex::core::file_entry
	{
		vitex::core::string path;
	};
}

namespace vitex
{
	namespace scripting
	{
		namespace bindings
		{
			void pointer_to_handle_cast(void* from, void** to, int type_id)
			{
				if (!(type_id & (size_t)type_id::handle_t))
					return;

				if (!(type_id & (size_t)type_id::mask_object_t))
					return;

				*to = from;
			}
			void handle_to_handle_cast(void* from, void** to, int type_id)
			{
				if (!(type_id & (size_t)type_id::handle_t))
					return;

				if (!(type_id & (size_t)type_id::mask_object_t))
					return;

				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return;

				auto type_info = vm->get_type_info_by_id(type_id);
				vm->ref_cast_object(from, type_info, type_info, to);
			}
			void* handle_to_pointer_cast(void* from, int type_id)
			{
				if (!(type_id & (size_t)type_id::handle_t))
					return nullptr;

				if (!(type_id & (size_t)type_id::mask_object_t))
					return nullptr;

				return *reinterpret_cast<void**>(from);
			}

			std::string_view string::impl_cast_string_view(core::string& base)
			{
				immediate_context* context = immediate_context::get();
				return std::string_view(context ? context->copy_string(base) : base);
			}
			void string::create(core::string* base)
			{
				new(base) core::string();
			}
			void string::create_copy1(core::string* base, const core::string& other)
			{
				new(base) core::string(other);
			}
			void string::create_copy2(core::string* base, const std::string_view& other)
			{
				new(base) core::string(other);
			}
			void string::destroy(core::string* base)
			{
				base->~basic_string();
			}
			void string::pop_back(core::string& base)
			{
				if (base.empty())
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
				else
					base.pop_back();
			}
			core::string& string::replace1(core::string& other, const core::string& from, const core::string& to, size_t start)
			{
				return core::stringify::replace(other, from, to, start);
			}
			core::string& string::replace_part1(core::string& other, size_t start, size_t end, const core::string& to)
			{
				return core::stringify::replace_part(other, start, end, to);
			}
			bool string::starts_with1(const core::string& other, const core::string& value, size_t offset)
			{
				return core::stringify::starts_with(other, value, offset);
			}
			bool string::ends_with1(const core::string& other, const core::string& value)
			{
				return core::stringify::ends_with(other, value);
			}
			core::string& string::replace2(core::string& other, const std::string_view& from, const std::string_view& to, size_t start)
			{
				return core::stringify::replace(other, from, to, start);
			}
			core::string& string::replace_part2(core::string& other, size_t start, size_t end, const std::string_view& to)
			{
				return core::stringify::replace_part(other, start, end, to);
			}
			bool string::starts_with2(const core::string& other, const std::string_view& value, size_t offset)
			{
				return core::stringify::starts_with(other, value, offset);
			}
			bool string::ends_with2(const core::string& other, const std::string_view& value)
			{
				return core::stringify::ends_with(other, value);
			}
			core::string string::substring1(core::string& base, size_t offset)
			{
				return base.substr(offset);
			}
			core::string string::substring2(core::string& base, size_t offset, size_t size)
			{
				return base.substr(offset, size);
			}
			core::string string::from_pointer(void* pointer)
			{
				return core::stringify::text("0x%" PRIXPTR, pointer);
			}
			core::string string::from_buffer(const char* buffer, size_t max_size)
			{
				if (!buffer)
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_NULLPOINTER));
					return core::string();
				}

				size_t size = strnlen(buffer, max_size);
				return core::string(buffer, size);
			}
			char* string::index(core::string& base, size_t offset)
			{
				if (offset >= base.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return &base[offset];
			}
			char* string::front(core::string& base)
			{
				if (base.empty())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				char& target = base.front();
				return &target;
			}
			char* string::back(core::string& base)
			{
				if (base.empty())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				char& target = base.back();
				return &target;
			}
			array* string::split(core::string& base, const std::string_view& delimiter)
			{
				virtual_machine* vm = virtual_machine::get();
				asITypeInfo* array_type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@").get_type_info();
				array* array = array::create(array_type);

				int offset = 0, prev = 0, count = 0;
				while ((offset = (int)base.find(delimiter, prev)) != (int)core::string::npos)
				{
					array->resize(array->size() + 1);
					((core::string*)array->at(count))->assign(&base[prev], offset - prev);
					prev = offset + (int)delimiter.size();
					count++;
				}

				array->resize(array->size() + 1);
				((core::string*)array->at(count))->assign(&base[prev]);
				return array;
			}

			core::string string_view::impl_cast_string(std::string_view& base)
			{
				return core::string(base);
			}
			void string_view::create(std::string_view* base)
			{
				new(base) std::string_view();
			}
			void string_view::create_copy(std::string_view* base, core::string& other)
			{
				immediate_context* context = immediate_context::get();
				new(base) std::string_view(context ? context->copy_string(other) : other);
			}
			void string_view::assign(std::string_view* base, core::string& other)
			{
				immediate_context* context = immediate_context::get();
				*base = (context ? context->copy_string(other) : other);
			}
			void string_view::destroy(std::string_view* base)
			{
				immediate_context* context = immediate_context::get();
				if (context != nullptr)
					context->invalidate_string(*base);
				base->~basic_string_view();
			}
			bool string_view::starts_with(const std::string_view& other, const std::string_view& value, size_t offset)
			{
				return core::stringify::starts_with(other, value, offset);
			}
			bool string_view::ends_with(const std::string_view& other, const std::string_view& value)
			{
				return core::stringify::ends_with(other, value);
			}
			int string_view::compare1(std::string_view& base, const core::string& other)
			{
				return base.compare(other);
			}
			int string_view::compare2(std::string_view& base, const std::string_view& other)
			{
				return base.compare(other);
			}
			core::string string_view::append1(const std::string_view& base, const std::string_view& other)
			{
				core::string result = core::string(base);
				result.append(other);
				return result;
			}
			core::string string_view::append2(const std::string_view& base, const core::string& other)
			{
				core::string result = core::string(base);
				result.append(other);
				return result;
			}
			core::string string_view::append3(const core::string& other, const std::string_view& base)
			{
				core::string result = other;
				result.append(base);
				return result;
			}
			core::string string_view::append4(const std::string_view& base, char other)
			{
				core::string result = core::string(base);
				result.append(1, other);
				return result;
			}
			core::string string_view::append5(char other, const std::string_view& base)
			{
				core::string result = core::string(1, other);
				result.append(base);
				return result;
			}
			core::string string_view::substring1(std::string_view& base, size_t offset)
			{
				return core::string(base.substr(offset));
			}
			core::string string_view::substring2(std::string_view& base, size_t offset, size_t size)
			{
				return core::string(base.substr(offset, size));
			}
			size_t string_view::reverse_find1(std::string_view& base, const std::string_view& other, size_t offset)
			{
				return base.rfind(other, offset);
			}
			size_t string_view::reverse_find2(std::string_view& base, char other, size_t offset)
			{
				return base.rfind(other, offset);
			}
			size_t string_view::find1(std::string_view& base, const std::string_view& other, size_t offset)
			{
				return base.find(other, offset);
			}
			size_t string_view::find2(std::string_view& base, char other, size_t offset)
			{
				return base.find(other, offset);
			}
			size_t string_view::find_first_of(std::string_view& base, const std::string_view& other, size_t offset)
			{
				return base.find_first_of(other, offset);
			}
			size_t string_view::find_first_not_of(std::string_view& base, const std::string_view& other, size_t offset)
			{
				return base.find_first_not_of(other, offset);
			}
			size_t string_view::find_last_of(std::string_view& base, const std::string_view& other, size_t offset)
			{
				return base.find_last_of(other, offset);
			}
			size_t string_view::find_last_not_of(std::string_view& base, const std::string_view& other, size_t offset)
			{
				return base.find_last_not_of(other, offset);
			}
			std::string_view string_view::from_buffer(const char* buffer, size_t max_size)
			{
				if (!buffer)
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_NULLPOINTER));
					return std::string_view();
				}

				size_t size = strnlen(buffer, max_size);
				return std::string_view(buffer, size);
			}
			char* string_view::index(std::string_view& base, size_t offset)
			{
				if (offset >= base.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				char* data = (char*)base.data();
				return &data[offset];
			}
			char* string_view::front(std::string_view& base)
			{
				if (base.empty())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				const char& target = base.front();
				return (char*)&target;
			}
			char* string_view::back(std::string_view& base)
			{
				if (base.empty())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				const char& target = base.back();
				return (char*)&target;
			}
			array* string_view::split(std::string_view& base, const std::string_view& delimiter)
			{
				virtual_machine* vm = virtual_machine::get();
				asITypeInfo* array_type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@").get_type_info();
				array* array = array::create(array_type);

				int offset = 0, prev = 0, count = 0;
				while ((offset = (int)base.find(delimiter, prev)) != (int)std::string::npos)
				{
					array->resize(array->size() + 1);
					((core::string*)array->at(count))->assign(&base[prev], offset - prev);
					prev = offset + (int)delimiter.size();
					count++;
				}

				array->resize(array->size() + 1);
				((core::string*)array->at(count))->assign(&base[prev]);
				return array;
			}

			float math::fp_from_ieee(uint32_t raw)
			{
				return *reinterpret_cast<float*>(&raw);
			}
			uint32_t math::fp_to_ieee(float value)
			{
				return *reinterpret_cast<uint32_t*>(&value);
			}
			double math::fp_from_ieee(as_uint64_t raw)
			{
				return *reinterpret_cast<double*>(&raw);
			}
			as_uint64_t math::fp_to_ieee(double value)
			{
				return *reinterpret_cast<as_uint64_t*>(&value);
			}
			bool math::close_to(float a, float b, float epsilon)
			{
				if (a == b)
					return true;

				float diff = fabsf(a - b);
				if ((a == 0 || b == 0) && (diff < epsilon))
					return true;

				return diff / (fabs(a) + fabs(b)) < epsilon;
			}
			bool math::close_to(double a, double b, double epsilon)
			{
				if (a == b)
					return true;

				double diff = fabs(a - b);
				if ((a == 0 || b == 0) && (diff < epsilon))
					return true;

				return diff / (fabs(a) + fabs(b)) < epsilon;
			}

			void imports::bind_syntax(virtual_machine* vm, bool enabled, const std::string_view& syntax)
			{
				VI_ASSERT(vm != nullptr, "vm should be set");
				if (enabled)
					vm->set_code_generator("import-syntax", std::bind(&bindings::imports::generator_callback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, syntax));
				else
					vm->set_code_generator("import-syntax", nullptr);
			}
			expects_vm<void> imports::generator_callback(compute::preprocessor* base, const std::string_view& path, core::string& code, const std::string_view& syntax)
			{
				return parser::replace_directive_preconditions(syntax, code, [base, &path](const std::string_view& text) -> expects_vm<core::string>
				{
					core::vector<std::pair<core::string, core::text_settle>> includes = core::stringify::find_in_between_in_code(text, "\"", "\"");
					core::string output;

					for (auto& include : includes)
					{
						auto result = base->resolve_file(path, core::stringify::trim(include.first));
						if (!result)
							return virtual_exception(virtual_error::invalid_declaration, std::move(result.error().message()));
						output += *result;
					}

					size_t prev = core::stringify::count_lines(text);
					size_t next = core::stringify::count_lines(output);
					size_t diff = (next < prev ? prev - next : 0);
					output.append(diff, '\n');
					return output;
				});
			}

			void tags::bind_syntax(virtual_machine* vm, bool enabled, const tag_callback& callback)
			{
				VI_ASSERT(vm != nullptr, "vm should be set");
				if (enabled)
					vm->set_code_generator("tags-syntax", std::bind(&bindings::tags::generator_callback, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, vm, callback));
				else
					vm->set_code_generator("tags-syntax", nullptr);
			}
			expects_vm<void> tags::generator_callback(compute::preprocessor* base, const std::string_view& path, core::string& code, virtual_machine* vm, const tag_callback& callback)
			{
#ifdef VI_ANGELSCRIPT
				if (!callback)
					return core::expectation::met;

				asIScriptEngine* engine = vm->get_engine();
				core::vector<tag_declaration> declarations;
				tag_declaration tag;
				size_t offset = 0;

				while (offset < code.size())
				{
					asUINT length = 0;
					asETokenClass type = engine->ParseToken(&code[offset], code.size() - offset, &length);
					if (type == asTC_COMMENT || type == asTC_WHITESPACE)
					{
						offset += length;
						continue;
					}

					std::string_view token = std::string_view(&code[offset], length);
					if (token == "shared" || token == "abstract" || token == "mixin" || token == "external")
					{
						offset += length;
						continue;
					}

					if (tag.class_name.empty() && (token == "class" || token == "interface"))
					{
						do
						{
							offset += length;
							if (offset >= code.size())
							{
								type = asTC_UNKNOWN;
								break;
							}
							type = engine->ParseToken(&code[offset], code.size() - offset, &length);
						} while (type == asTC_COMMENT || type == asTC_WHITESPACE);

						if (type == asTC_IDENTIFIER)
						{
							tag.class_name = std::string_view(code).substr(offset, length);
							while (offset < code.length())
							{
								engine->ParseToken(&code[offset], code.size() - offset, &length);
								if (code[offset] == '{')
								{
									offset += length;
									break;
								}
								else if (code[offset] == ';')
								{
									tag.class_name = std::string_view();
									offset += length;
									break;
								}
								offset += length;
							}
						}

						continue;
					}

					if (!tag.class_name.empty() && token == "}")
					{
						tag.class_name = std::string_view();
						offset += length;
						continue;
					}

					if (token == "namespace")
					{
						do
						{
							offset += length;
							type = engine->ParseToken(&code[offset], code.size() - offset, &length);
						} while (type == asTC_COMMENT || type == asTC_WHITESPACE);

						if (!tag.name_space.empty())
							tag.name_space += "::";

						tag.name_space += code.substr(offset, length);
						while (offset < code.length())
						{
							engine->ParseToken(&code[offset], code.size() - offset, &length);
							if (code[offset] == '{')
							{
								offset += length;
								break;
							}
							offset += length;
						}
						continue;
					}

					if (!tag.name_space.empty() && token == "}")
					{
						size_t index = tag.name_space.rfind("::");
						if (index != std::string::npos)
							tag.name_space.erase(index);
						else
							tag.name_space.clear();
						offset += length;
						continue;
					}

					if (token == "[")
					{
						offset = extract_field(vm, code, offset, tag);
						extract_declaration(vm, code, offset, tag);
					}

					length = 0;
					while (offset < code.length() && code[offset] != ';' && code[offset] != '{')
					{
						engine->ParseToken(&code[offset], code.size() - offset, &length);
						offset += length;
					}

					if (offset < code.length() && code[offset] == '{')
					{
						int level = 1; ++offset;
						while (level > 0 && offset < code.size())
						{
							if (engine->ParseToken(&code[offset], code.size() - offset, &length) == asTC_KEYWORD)
							{
								if (code[offset] == '{')
									level++;
								else if (code[offset] == '}')
									level--;
							}
							offset += length;
						}
					}
					else
						++offset;

					if (tag.type == tag_type::unknown)
						continue;

					auto& result = declarations.emplace_back();
					result.class_name = tag.class_name;
					result.name = tag.name;
					result.declaration = std::move(tag.declaration);
					result.name_space = tag.name_space;
					result.directives = std::move(tag.directives);
					result.type = tag.type;

					core::stringify::trim(result.declaration);
					tag.name = std::string_view();
					tag.type = tag_type::unknown;
				}

				callback(vm, std::move(declarations));
				return core::expectation::met;
#else
				return virtual_exception(virtual_error::not_supported, "tag generator requires engine support");
#endif
			}
			size_t tags::extract_field(virtual_machine* vm, core::string& code, size_t offset, tag_declaration& tag)
			{
#ifdef VI_ANGELSCRIPT
				asIScriptEngine* engine = vm->get_engine();
				for (;;)
				{
					core::string declaration;
					code[offset++] = ' ';

					int level = 1; asUINT length = 0;
					while (level > 0 && offset < code.size())
					{
						asETokenClass type = engine->ParseToken(&code[offset], code.size() - offset, &length);
						if (type == asTC_KEYWORD)
						{
							if (code[offset] == '[')
								level++;
							else if (code[offset] == ']')
								level--;
						}

						if (level > 0)
							declaration.append(&code[offset], length);

						if (type != asTC_WHITESPACE)
						{
							char* buffer = &code[offset];
							for (asUINT i = 0; i < length; i++)
							{
								if (!core::stringify::is_whitespace(buffer[i]))
									buffer[i] = ' ';
							}
						}
						offset += length;
					}
					append_directive(tag, core::stringify::trim(declaration));

					asETokenClass type = engine->ParseToken(&code[offset], code.size() - offset, &length);
					while (type == asTC_COMMENT || type == asTC_WHITESPACE)
					{
						offset += length;
						type = engine->ParseToken(&code[offset], code.size() - offset, &length);
					}

					if (code[offset] != '[')
						break;
				}
#endif
				return offset;
			}
			void tags::extract_declaration(virtual_machine* vm, core::string& code, size_t offset, tag_declaration& tag)
			{
#ifdef VI_ANGELSCRIPT
				asIScriptEngine* engine = vm->get_engine();
				std::string_view token;
				asUINT length = 0;
				asETokenClass type = asTC_WHITESPACE;

				do
				{
					offset += length;
					type = engine->ParseToken(&code[offset], code.size() - offset, &length);
					token = std::string_view(&code[offset], length);
				} while (type == asTC_WHITESPACE || type == asTC_COMMENT || token == "private" || token == "protected" || token == "shared" || token == "external" || token == "final" || token == "abstract");

				if (type != asTC_KEYWORD && type != asTC_IDENTIFIER)
					return;

				token = std::string_view(&code[offset], length);
				if (token != "interface" && token != "class" && token != "enum")
				{
					int nested_parenthesis = 0;
					bool has_parenthesis = false;
					tag.declaration.append(&code[offset], length);
					offset += length;
					for (; offset < code.size();)
					{
						type = engine->ParseToken(&code[offset], code.size() - offset, &length);
						token = std::string_view(&code[offset], length);
						if (type == asTC_KEYWORD)
						{
							if (token == "{" && nested_parenthesis == 0)
							{
								if (!has_parenthesis)
								{
									tag.declaration = tag.name;
									tag.type = tag_type::property_function;
								}
								else
									tag.type = tag_type::function;
								return;
							}
							if ((token == "=" && !has_parenthesis) || token == ";")
							{
								if (!has_parenthesis)
								{
									tag.declaration = tag.name;
									tag.type = tag_type::variable;
								}
								else
									tag.type = tag_type::not_type;
								return;
							}
							else if (token == "(")
							{
								nested_parenthesis++;
								has_parenthesis = true;
							}
							else if (token == ")")
								nested_parenthesis--;
						}
						else if (tag.name.empty() && type == asTC_IDENTIFIER)
							tag.name = token;

						if (!has_parenthesis || nested_parenthesis > 0 || type != asTC_IDENTIFIER || (token != "final" && token != "override"))
							tag.declaration += token;
						offset += length;
					}
				}
				else
				{
					do
					{
						offset += length;
						type = engine->ParseToken(&code[offset], code.size() - offset, &length);
					} while (type == asTC_WHITESPACE || type == asTC_COMMENT);

					if (type == asTC_IDENTIFIER)
					{
						tag.type = tag_type::type;
						tag.declaration = std::string_view(&code[offset], length);
						offset += length;
						return;
					}
				}
#endif
			}
			void tags::append_directive(tag_declaration& tag, core::string& directive)
			{
				tag_directive result;
				size_t where = directive.find('(');
				if (where == core::string::npos || directive.back() != ')')
				{
					result.name = std::move(directive);
					tag.directives.emplace_back(std::move(result));
					return;
				}

				core::vector<core::string> args;
				std::string_view data = std::string_view(directive).substr(where + 1, directive.size() - where - 2);
				result.name = directive.substr(0, where);
				where = 0;

				size_t last = 0;
				while (where < data.size())
				{
					char v = data[where];
					if (v == '\"' || v == '\'')
					{
						while (where < data.size() && data[++where] != v);
						if (where + 1 >= data.size())
						{
							++where;
							goto add_value;
						}
					}
					else if (v == ',' || where + 1 >= data.size())
					{
					add_value:
						core::string subvalue = core::string(data.substr(last, where + 1 >= data.size() ? core::string::npos : where - last));
						core::stringify::trim(subvalue);
						if (subvalue.size() >= 2 && subvalue.front() == subvalue.back() && (subvalue.front() == '\"' || subvalue.front() == '\''))
							subvalue.erase(subvalue.size() - 1, 1).erase(0, 1);
						args.push_back(std::move(subvalue));
						last = where + 1;
					}
					++where;
				}

				for (auto& key_value : args)
				{
					size_t key_size = 0;
					char v = key_value.empty() ? 0 : key_value.front();
					if (v == '\"' || v == '\'')
						while (key_size < key_value.size() && key_value[++key_size] != v);

					key_size = key_value.find('=', key_size);
					if (key_size != std::string::npos)
					{
						core::string key = key_value.substr(0, key_size - 1);
						core::string value = key_value.substr(key_size + 1);
						core::stringify::trim(key);
						core::stringify::trim(value);
						result.args[key] = std::move(value);
					}
					else
						result.args[core::to_string(result.args.size())] = std::move(key_value);
				}
				tag.directives.emplace_back(std::move(result));
			}

			exception::pointer::pointer() : context(nullptr)
			{
			}
			exception::pointer::pointer(immediate_context* new_context) : context(new_context)
			{
				auto value = (context ? context->get_exception_string() : std::string_view());
				if (!value.empty() && (context ? !context->will_exception_be_caught() : false))
				{
					load_exception_data(value);
					origin = load_stack_here();
				}
			}
			exception::pointer::pointer(const std::string_view& value) : context(immediate_context::get())
			{
				load_exception_data(value);
				origin = load_stack_here();
			}
			exception::pointer::pointer(const std::string_view& new_type, const std::string_view& new_text) : type(new_type), text(new_text), context(immediate_context::get())
			{
				origin = load_stack_here();
			}
			void exception::pointer::load_exception_data(const std::string_view& value)
			{
				size_t offset = value.find(':');
				if (offset != std::string::npos)
				{
					type = value.substr(0, offset);
					text = value.substr(offset + 1);
				}
				else if (!value.empty())
				{
					type = EXCEPTIONCAT_GENERIC;
					text = value;
				}
			}
			const core::string& exception::pointer::get_type() const
			{
				return type;
			}
			const core::string& exception::pointer::get_text() const
			{
				return text;
			}
			core::string exception::pointer::to_exception_string() const
			{
				return empty() ? "" : type + ":" + text;
			}
			core::string exception::pointer::what() const
			{
				core::string data = type;
				if (!text.empty())
				{
					data.append(": ");
					data.append(text);
				}

				data.append(" ");
				data.append(origin.empty() ? load_stack_here() : origin);
				return data;
			}
			core::string exception::pointer::load_stack_here() const
			{
				core::string data;
				if (!context)
					return data;

				core::string_stream stream;
				stream << '\n';

				scripting::virtual_machine* vm = context->get_vm();
				size_t callstack_size = context->get_callstack_size();
				size_t top_callstack_size = callstack_size;
				for (size_t i = 0; i < callstack_size; i++)
				{
					int column_number = 0;
					int line_number = context->get_line_number(i, &column_number);
					scripting::function next = context->get_function(i);
					auto section_name = next.get_section_name();
					stream << "  #" << --top_callstack_size << " at " << core::os::path::get_filename(section_name);
					if (line_number > 0)
						stream << ":" << line_number;
					if (column_number > 0)
						stream << "," << column_number;
					stream << " in " << (next.get_decl().empty() ? "[optimized]" : next.get_decl());
					if (top_callstack_size > 0)
						stream << "\n";
				}
				data = std::move(stream.str());
				return data;
			}
			bool exception::pointer::empty() const
			{
				return type.empty() && text.empty();
			}

			void exception::throw_ptr_at(immediate_context* context, const pointer& data)
			{
				if (context != nullptr)
					context->set_exception(data.to_exception_string().c_str());
			}
			void exception::throw_ptr(const pointer& data)
			{
				throw_ptr_at(immediate_context::get(), data);
			}
			void exception::rethrow_at(immediate_context* context)
			{
				if (context != nullptr)
					context->set_exception(context->get_exception_string());
			}
			void exception::rethrow()
			{
				rethrow_at(immediate_context::get());
			}
			bool exception::has_exception_at(immediate_context* context)
			{
				return context ? !context->get_exception_string().empty() : false;
			}
			bool exception::has_exception()
			{
				return has_exception_at(immediate_context::get());
			}
			exception::pointer exception::get_exception_at(immediate_context* context)
			{
				return pointer(context);
			}
			exception::pointer exception::get_exception()
			{
				return get_exception_at(immediate_context::get());
			}
			expects_vm<void> exception::generator_callback(compute::preprocessor*, const std::string_view& path, core::string& code)
			{
				return parser::replace_inline_preconditions("throw", code, [](const std::string_view& expression) -> expects_vm<core::string>
				{
					core::string result = "exception::throw(";
					result.append(expression);
					result.append(1, ')');
					return result;
				});
			}

			exception::pointer expects_wrapper::translate_throw(const std::exception& error)
			{
				return exception::pointer(EXCEPTIONCAT_STANDARD, error.what());
			}
			exception::pointer expects_wrapper::translate_throw(const std::error_code& error)
			{
				return exception::pointer(EXCEPTIONCAT_SYSTEM, core::copy<core::string>(error.message()));
			}
			exception::pointer expects_wrapper::translate_throw(const std::error_condition& error)
			{
				return exception::pointer(EXCEPTIONCAT_SYSTEM, core::copy<core::string>(error.message()));
			}
			exception::pointer expects_wrapper::translate_throw(const std::string_view& error)
			{
				return exception::pointer(EXCEPTIONCAT_SYSTEM, error);
			}
			exception::pointer expects_wrapper::translate_throw(const core::string& error)
			{
				return exception::pointer(EXCEPTIONCAT_SYSTEM, error);
			}
			exception::pointer expects_wrapper::translate_throw(const core::basic_exception& error)
			{
				return exception::pointer(error.type(), error.message());
			}

			exception::pointer option_wrapper::translate_throw()
			{
				return exception::pointer(EXCEPTION_ACCESSINVALID);
			}

			any::any(virtual_machine* _Engine) noexcept : engine(_Engine)
			{
				value.type_id = 0;
				value.integer = 0;
				engine->notify_of_new_object(this, engine->get_type_info_by_name(TYPENAME_ANY));
			}
			any::any(void* ref, int ref_type_id, virtual_machine* _Engine) noexcept : any(_Engine)
			{
				store(ref, ref_type_id);
			}
			any::any(const any& other) noexcept : any(other.engine)
			{
				if ((other.value.type_id & (size_t)type_id::mask_object_t))
				{
					auto info = engine->get_type_info_by_id(other.value.type_id);
					if (info.is_valid())
						info.add_ref();
				}

				value.type_id = other.value.type_id;
				if (value.type_id & (size_t)type_id::handle_t)
				{
					value.object = other.value.object;
					engine->add_ref_object(value.object, engine->get_type_info_by_id(value.type_id));
				}
				else if (value.type_id & (size_t)type_id::mask_object_t)
					value.object = engine->create_object_copy(other.value.object, engine->get_type_info_by_id(value.type_id));
				else
					value.integer = other.value.integer;
			}
			any::~any() noexcept
			{
				free_object();
			}
			any& any::operator=(const any& other) noexcept
			{
				if ((other.value.type_id & (size_t)type_id::mask_object_t))
				{
					auto info = engine->get_type_info_by_id(other.value.type_id);
					if (info.is_valid())
						info.add_ref();
				}

				free_object();
				value.type_id = other.value.type_id;

				if (value.type_id & (size_t)type_id::handle_t)
				{
					value.object = other.value.object;
					engine->add_ref_object(value.object, engine->get_type_info_by_id(value.type_id));
				}
				else if (value.type_id & (size_t)type_id::mask_object_t)
					value.object = engine->create_object_copy(other.value.object, engine->get_type_info_by_id(value.type_id));
				else
					value.integer = other.value.integer;

				return *this;
			}
			int any::copy_from(const any* other)
			{
				if (other == 0)
					return (int)virtual_error::invalid_arg;

				*this = *(any*)other;
				return 0;
			}
			void any::store(void* ref, int ref_type_id)
			{
				if ((ref_type_id & (size_t)type_id::mask_object_t))
				{
					auto info = engine->get_type_info_by_id(ref_type_id);
					if (info.is_valid())
						info.add_ref();
				}

				free_object();
				value.type_id = ref_type_id;

				if (value.type_id & (size_t)type_id::handle_t)
				{
					value.object = *(void**)ref;
					engine->add_ref_object(value.object, engine->get_type_info_by_id(value.type_id));
				}
				else if (value.type_id & (size_t)type_id::mask_object_t)
				{
					value.object = engine->create_object_copy(ref, engine->get_type_info_by_id(value.type_id));
				}
				else
				{
					value.integer = 0;
					auto size = engine->get_size_of_primitive_type(value.type_id);
					if (size)
						memcpy(&value.integer, ref, *size);
				}
			}
			bool any::retrieve(void* ref, int ref_type_id) const
			{
				if (ref_type_id & (size_t)type_id::handle_t)
				{
					if ((value.type_id & (size_t)type_id::mask_object_t))
					{
						if ((value.type_id & (size_t)type_id::const_handle_t) && !(ref_type_id & (size_t)type_id::const_handle_t))
							return false;

						engine->ref_cast_object(value.object, engine->get_type_info_by_id(value.type_id), engine->get_type_info_by_id(ref_type_id), reinterpret_cast<void**>(ref));
#ifdef VI_ANGELSCRIPT
						if (*(asPWORD*)ref == 0)
							return false;
#endif

						return true;
					}
				}
				else if (ref_type_id & (size_t)type_id::mask_object_t)
				{
					if (value.type_id == ref_type_id)
					{
						engine->assign_object(ref, value.object, engine->get_type_info_by_id(value.type_id));
						return true;
					}
				}
				else
				{
					auto size1 = engine->get_size_of_primitive_type(value.type_id);
					auto size2 = engine->get_size_of_primitive_type(ref_type_id);
					if (size1 && size2 && *size1 == *size2)
					{
						memcpy(ref, &value.integer, *size1);
						return true;
					}
				}

				return false;
			}
			void* any::get_address_of_object()
			{
				if (value.type_id & (size_t)type_id::handle_t)
					return &value.object;
				else if (value.type_id & (size_t)type_id::mask_object_t)
					return value.object;
				else if (value.type_id <= (size_t)type_id::double_t || value.type_id & (size_t)type_id::mask_seqnbr_t)
					return &value.integer;

				return nullptr;
			}
			int any::get_type_id() const
			{
				return value.type_id;
			}
			void any::free_object()
			{
				if (value.type_id & (size_t)type_id::mask_object_t)
				{
					auto type = engine->get_type_info_by_id(value.type_id);
					engine->release_object(value.object, type);
					type.release();
					value.object = 0;
					value.type_id = 0;
				}
			}
			void any::enum_references(asIScriptEngine* in_engine)
			{
				if (value.object && (value.type_id & (size_t)type_id::mask_object_t))
				{
					auto sub_type = engine->get_type_info_by_id(value.type_id);
					if ((sub_type.flags() & (size_t)object_behaviours::ref))
						function_factory::gc_enum_callback(in_engine, value.object);
					else if ((sub_type.flags() & (size_t)object_behaviours::value) && (sub_type.flags() & (size_t)object_behaviours::gc))
						engine->forward_enum_references(value.object, sub_type);

					auto type = virtual_machine::get(in_engine)->get_type_info_by_id(value.type_id);
					function_factory::gc_enum_callback(in_engine, type.get_type_info());
				}
			}
			void any::release_references(asIScriptEngine*)
			{
				free_object();
			}
			any* any::create()
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				any* result = new any(vm);
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}
			any* any::create(int type_id, void* ref)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				any* result = new any(ref, type_id, vm);
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}
			any* any::create(const char* decl, void* ref)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				any* result = new any(ref, vm->get_type_id_by_decl(decl), vm);
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}
			any* any::factory1()
			{
				any* result = new any(virtual_machine::get());
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));
				return result;
			}
			void any::factory2(asIScriptGeneric* genericf)
			{
				generic_context args = genericf;
				any* result = new any(args.get_arg_address(0), args.get_arg_type_id(0), args.get_vm());
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));
				else
					*(any**)args.get_address_of_return_location() = result;
			}
			any& any::assignment(any* base, any* other)
			{
				*base = *other;
				return *base;
			}

			array::array(asITypeInfo* info, void* buffer_ptr) noexcept : obj_type(info), buffer(nullptr), element_size(0), sub_type_id(-1)
			{
				VI_ASSERT(info && core::string(obj_type.get_name()) == TYPENAME_ARRAY, "array type is invalid");
#ifdef VI_ANGELSCRIPT
				obj_type.add_ref();
				precache();

				virtual_machine* engine = obj_type.get_vm();
				if (sub_type_id & (size_t)type_id::mask_object_t)
					element_size = sizeof(asPWORD);
				else
					element_size = engine->get_size_of_primitive_type(sub_type_id).or_else(0);

				size_t length = *(asUINT*)buffer_ptr;
				if (!check_max_size(length))
					return;

				if ((obj_type.get_sub_type_id() & (size_t)type_id::mask_object_t) == 0)
				{
					create_buffer(&buffer, length);
					if (length > 0)
						memcpy(at(0), (((asUINT*)buffer_ptr) + 1), (size_t)length * (size_t)element_size);
				}
				else if (obj_type.get_sub_type_id() & (size_t)type_id::handle_t)
				{
					create_buffer(&buffer, length);
					if (length > 0)
						memcpy(at(0), (((asUINT*)buffer_ptr) + 1), (size_t)length * (size_t)element_size);

					memset((((asUINT*)buffer_ptr) + 1), 0, (size_t)length * (size_t)element_size);
				}
				else if (obj_type.get_sub_type().flags() & (size_t)object_behaviours::ref)
				{
					sub_type_id |= (size_t)type_id::handle_t;
					create_buffer(&buffer, length);
					sub_type_id &= ~(size_t)type_id::handle_t;

					if (length > 0)
						memcpy(buffer->data, (((asUINT*)buffer_ptr) + 1), (size_t)length * (size_t)element_size);

					memset((((asUINT*)buffer_ptr) + 1), 0, (size_t)length * (size_t)element_size);
				}
				else
				{
					create_buffer(&buffer, length);
					for (size_t n = 0; n < length; n++)
					{
						unsigned char* source_obj = (unsigned char*)buffer_ptr;
						source_obj += 4 + n * obj_type.get_sub_type().size();
						engine->assign_object(at(n), source_obj, obj_type.get_sub_type());
					}
				}

				if (obj_type.flags() & (size_t)object_behaviours::gc)
					obj_type.get_vm()->notify_of_new_object(this, obj_type);
#endif
			}
			array::array(size_t length, asITypeInfo* info) noexcept : obj_type(info), buffer(nullptr), element_size(0), sub_type_id(-1)
			{
				VI_ASSERT(info && core::string(obj_type.get_name()) == TYPENAME_ARRAY, "array type is invalid");
#ifdef VI_ANGELSCRIPT
				obj_type.add_ref();
				precache();

				if (sub_type_id & (size_t)type_id::mask_object_t)
					element_size = sizeof(asPWORD);
				else
					element_size = obj_type.get_vm()->get_size_of_primitive_type(sub_type_id).or_else(0);

				if (!check_max_size(length))
					return;

				create_buffer(&buffer, length);
				if (obj_type.flags() & (size_t)object_behaviours::gc)
					obj_type.get_vm()->notify_of_new_object(this, obj_type);
#endif
			}
			array::array(const array& other) noexcept : obj_type(other.obj_type), buffer(nullptr), element_size(0), sub_type_id(-1)
			{
				VI_ASSERT(obj_type.is_valid() && core::string(obj_type.get_name()) == TYPENAME_ARRAY, "array type is invalid");
				obj_type.add_ref();
				precache();

				element_size = other.element_size;
				if (obj_type.flags() & (size_t)object_behaviours::gc)
					obj_type.get_vm()->notify_of_new_object(this, obj_type);

				create_buffer(&buffer, 0);
				*this = other;
			}
			array::array(size_t length, void* default_value, asITypeInfo* info) noexcept : obj_type(info), buffer(nullptr), element_size(0), sub_type_id(-1)
			{
#ifdef VI_ANGELSCRIPT
				VI_ASSERT(info && core::string(info->GetName()) == TYPENAME_ARRAY, "array type is invalid");
				obj_type.add_ref();
				precache();

				if (sub_type_id & (size_t)type_id::mask_object_t)
					element_size = sizeof(asPWORD);
				else
					element_size = obj_type.get_vm()->get_size_of_primitive_type(sub_type_id).or_else(0);

				if (!check_max_size(length))
					return;

				create_buffer(&buffer, length);
				if (obj_type.flags() & (size_t)object_behaviours::gc)
					obj_type.get_vm()->notify_of_new_object(this, obj_type);

				for (size_t i = 0; i < size(); i++)
					set_value(i, default_value);
#endif
			}
			array::~array() noexcept
			{
				if (buffer)
				{
					delete_buffer(buffer);
					buffer = nullptr;
				}
				obj_type.release();
			}
			array& array::operator=(const array& other) noexcept
			{
				if (&other != this && other.get_array_object_type() == get_array_object_type())
				{
					resize(other.buffer->num_elements);
					copy_buffer(buffer, other.buffer);
				}

				return *this;
			}
			void array::set_value(size_t index, void* value)
			{
				void* ptr = at(index);
				if (ptr == 0)
					return;

				if ((sub_type_id & ~(size_t)type_id::mask_seqnbr_t) && !(sub_type_id & (size_t)type_id::handle_t))
					obj_type.get_vm()->assign_object(ptr, value, obj_type.get_sub_type());
				else if (sub_type_id & (size_t)type_id::handle_t)
				{
					void* swap = *(void**)ptr;
					*(void**)ptr = *(void**)value;
					obj_type.get_vm()->add_ref_object(*(void**)value, obj_type.get_sub_type());
					if (swap)
						obj_type.get_vm()->release_object(swap, obj_type.get_sub_type());
				}
				else if (sub_type_id == (size_t)type_id::bool_t || sub_type_id == (size_t)type_id::int8_t || sub_type_id == (size_t)type_id::uint8_t)
					*(char*)ptr = *(char*)value;
				else if (sub_type_id == (size_t)type_id::int16_t || sub_type_id == (size_t)type_id::uint16_t)
					*(short*)ptr = *(short*)value;
				else if (sub_type_id == (size_t)type_id::int32_t || sub_type_id == (size_t)type_id::uint32_t || sub_type_id == (size_t)type_id::float_t || sub_type_id > (size_t)type_id::double_t)
					*(int*)ptr = *(int*)value;
				else if (sub_type_id == (size_t)type_id::int64_t || sub_type_id == (size_t)type_id::uint64_t || sub_type_id == (size_t)type_id::double_t)
					*(double*)ptr = *(double*)value;
			}
			size_t array::size() const
			{
				return buffer->num_elements;
			}
			size_t array::capacity() const
			{
				return buffer->max_elements;
			}
			bool array::empty() const
			{
				return buffer->num_elements == 0;
			}
			void array::reserve(size_t max_elements)
			{
				if (max_elements <= buffer->max_elements)
					return;

				if (!check_max_size(max_elements))
					return;

				sbuffer* new_buffer = core::memory::allocate<sbuffer>(sizeof(sbuffer) - 1 + (size_t)element_size * (size_t)max_elements);
				if (!new_buffer)
					return bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				new_buffer->num_elements = buffer->num_elements;
				new_buffer->max_elements = max_elements;
				memcpy(new_buffer->data, buffer->data, (size_t)buffer->num_elements * (size_t)element_size);
				core::memory::deallocate(buffer);
				buffer = new_buffer;
			}
			void array::resize(size_t num_elements)
			{
				if (!check_max_size(num_elements))
					return;

				resize((int64_t)num_elements - (int64_t)buffer->num_elements, (size_t)-1);
			}
			void array::remove_range(size_t start, size_t count)
			{
				if (count == 0)
					return;

				if (buffer == 0 || start > buffer->num_elements)
					return bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));

				if (start + count > buffer->num_elements)
					count = buffer->num_elements - start;

				destroy(buffer, start, start + count);
				memmove(buffer->data + start * (size_t)element_size, buffer->data + (start + count) * (size_t)element_size, (size_t)(buffer->num_elements - start - count) * (size_t)element_size);
				buffer->num_elements -= count;
			}
			void array::remove_if(void* value, size_t start_at)
			{
				scache* cache; size_t count = size();
				if (!is_eligible_for_find(&cache) || !count)
					return;

				immediate_context* context = immediate_context::get();
				for (size_t i = start_at; i < count; i++)
				{
					if (equals(at(i), value, context, cache))
					{
						remove_at(i--);
						--count;
					}
				}
			}
			void array::resize(int64_t delta, size_t where)
			{
				if (delta < 0)
				{
					if (-delta > (int64_t)buffer->num_elements)
						delta = -(int64_t)buffer->num_elements;

					if (where > buffer->num_elements + delta)
						where = buffer->num_elements + delta;
				}
				else if (delta > 0)
				{
					if (!check_max_size(buffer->num_elements + delta))
						return;

					if (where > buffer->num_elements)
						where = buffer->num_elements;
				}

				if (delta == 0)
					return;

				if (buffer->max_elements < buffer->num_elements + delta)
				{
					size_t count = (size_t)buffer->num_elements + (size_t)delta, size = (size_t)element_size;
					sbuffer* new_buffer = core::memory::allocate<sbuffer>(sizeof(sbuffer) - 1 + size * count);
					if (!new_buffer)
						return bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

					new_buffer->num_elements = buffer->num_elements + delta;
					new_buffer->max_elements = new_buffer->num_elements;
					memcpy(new_buffer->data, buffer->data, (size_t)where * (size_t)element_size);
					if (where < buffer->num_elements)
						memcpy(new_buffer->data + (where + delta) * (size_t)element_size, buffer->data + where * (size_t)element_size, (size_t)(buffer->num_elements - where) * (size_t)element_size);

					create(new_buffer, where, where + delta);
					core::memory::deallocate(buffer);
					buffer = new_buffer;
				}
				else if (delta < 0)
				{
					destroy(buffer, where, where - delta);
					memmove(buffer->data + where * (size_t)element_size, buffer->data + (where - delta) * (size_t)element_size, (size_t)(buffer->num_elements - (where - delta)) * (size_t)element_size);
					buffer->num_elements += delta;
				}
				else
				{
					memmove(buffer->data + (where + delta) * (size_t)element_size, buffer->data + where * (size_t)element_size, (size_t)(buffer->num_elements - where) * (size_t)element_size);
					create(buffer, where, where + delta);
					buffer->num_elements += delta;
				}
			}
			bool array::check_max_size(size_t num_elements)
			{
				size_t max_size = 0xFFFFFFFFul - sizeof(sbuffer) + 1;
				if (element_size > 0)
					max_size /= (size_t)element_size;

				if (num_elements <= max_size)
					return true;

				bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_TOOLARGESIZE));
				return false;
			}
			asITypeInfo* array::get_array_object_type() const
			{
				return obj_type.get_type_info();
			}
			int array::get_array_type_id() const
			{
				return obj_type.get_type_id();
			}
			int array::get_element_type_id() const
			{
				return sub_type_id;
			}
			void array::insert_at(size_t index, void* value)
			{
				if (index > buffer->num_elements)
					return bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));

				resize(1, index);
				set_value(index, value);
			}
			void array::insert_at(size_t index, const array& array)
			{
				if (index > buffer->num_elements)
					return bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));

				if (obj_type.get_type_info() != array.obj_type.get_type_info())
					return bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_TEMPLATEMISMATCH));

				size_t new_size = array.size();
				resize((int)new_size, index);

				if (&array != this)
				{
					for (size_t i = 0; i < array.size(); i++)
					{
						void* value = const_cast<void*>(array.at(i));
						set_value(index + i, value);
					}
				}
				else
				{
					for (size_t i = 0; i < index; i++)
					{
						void* value = const_cast<void*>(array.at(i));
						set_value(index + i, value);
					}

					for (size_t i = index + new_size, k = 0; i < array.size(); i++, k++)
					{
						void* value = const_cast<void*>(array.at(i));
						set_value(index + index + k, value);
					}
				}
			}
			void array::insert_last(void* value)
			{
				insert_at(buffer->num_elements, value);
			}
			void array::remove_at(size_t index)
			{
				if (index >= buffer->num_elements)
					return bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
				resize(-1, index);
			}
			void array::remove_last()
			{
				remove_at(buffer->num_elements - 1);
			}
			const void* array::at(size_t index) const
			{
				if (buffer == 0 || index >= buffer->num_elements)
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}
				else if ((sub_type_id & (size_t)type_id::mask_object_t) && !(sub_type_id & (size_t)type_id::handle_t))
					return *(void**)(buffer->data + (size_t)element_size * index);

				return buffer->data + (size_t)element_size * index;
			}
			void* array::at(size_t index)
			{
				return const_cast<void*>(const_cast<const array*>(this)->at(index));
			}
			void* array::front()
			{
				if (empty())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return at(0);
			}
			const void* array::front() const
			{
				if (empty())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return at(0);
			}
			void* array::back()
			{
				if (empty())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return at(size() - 1);
			}
			const void* array::back() const
			{
				if (empty())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return at(size() - 1);
			}
			void* array::get_buffer()
			{
				return buffer->data;
			}
			void array::create_buffer(sbuffer** buffer_ptr, size_t num_elements)
			{
				*buffer_ptr = core::memory::allocate<sbuffer>(sizeof(sbuffer) - 1 + (size_t)element_size * (size_t)num_elements);
				if (!*buffer_ptr)
					return bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				(*buffer_ptr)->num_elements = num_elements;
				(*buffer_ptr)->max_elements = num_elements;
				create(*buffer_ptr, 0, num_elements);
			}
			void array::delete_buffer(sbuffer* buffer_ptr)
			{
				destroy(buffer_ptr, 0, buffer_ptr->num_elements);
				core::memory::deallocate(buffer_ptr);
			}
			void array::create(sbuffer* buffer_ptr, size_t start, size_t end)
			{
				if ((sub_type_id & (size_t)type_id::mask_object_t) && !(sub_type_id & (size_t)type_id::handle_t))
				{
					void** max = (void**)(buffer_ptr->data + end * sizeof(void*));
					void** d = (void**)(buffer_ptr->data + start * sizeof(void*));

					virtual_machine* engine = obj_type.get_vm();
					type_info sub_type = obj_type.get_sub_type();

					for (; d < max; d++)
					{
						*d = (void*)engine->create_object(sub_type);
						if (*d == 0)
						{
							memset(d, 0, sizeof(void*) * (max - d));
							return;
						}
					}
				}
				else
				{
					void* d = (void*)(buffer_ptr->data + start * (size_t)element_size);
					memset(d, 0, (size_t)(end - start) * (size_t)element_size);
				}
			}
			void array::destroy(sbuffer* buffer_ptr, size_t start, size_t end)
			{
				if (sub_type_id & (size_t)type_id::mask_object_t)
				{
					virtual_machine* engine = obj_type.get_vm();
					type_info sub_type = obj_type.get_sub_type();
					void** max = (void**)(buffer_ptr->data + end * sizeof(void*));
					void** d = (void**)(buffer_ptr->data + start * sizeof(void*));

					for (; d < max; d++)
					{
						if (*d)
							engine->release_object(*d, sub_type);
					}
				}
			}
			void array::reverse()
			{
				size_t length = size();
				if (length >= 2)
				{
					unsigned char temp[16];
					for (size_t i = 0; i < length / 2; i++)
					{
						copy(temp, get_array_item_pointer((int)i));
						copy(get_array_item_pointer((int)i), get_array_item_pointer((int)(length - i - 1)));
						copy(get_array_item_pointer((int)(length - i - 1)), temp);
					}
				}
			}
			void array::clear()
			{
				resize(0);
			}
			bool array::operator==(const array& other) const
			{
				if (obj_type.get_type_info() != other.obj_type.get_type_info())
					return false;

				if (size() != other.size())
					return false;

				immediate_context* cmp_context = 0;
				bool is_nested = false;

				if (sub_type_id & ~(size_t)type_id::mask_seqnbr_t)
				{
					cmp_context = immediate_context::get();
					if (cmp_context)
					{
						if (cmp_context->get_vm() == obj_type.get_vm() && cmp_context->push_state())
							is_nested = true;
						else
							cmp_context = 0;
					}

					if (cmp_context == 0)
						cmp_context = obj_type.get_vm()->request_context();
				}

				bool is_equal = true;
				scache* cache = reinterpret_cast<scache*>(obj_type.get_user_data(array_id));
				for (size_t n = 0; n < size(); n++)
				{
					if (!equals(at(n), other.at(n), cmp_context, cache))
					{
						is_equal = false;
						break;
					}
				}

				if (cmp_context)
				{
					if (is_nested)
					{
						auto state = cmp_context->get_state();
						cmp_context->pop_state();
						if (state == execution::aborted)
							cmp_context->abort();
					}
					else
						obj_type.get_vm()->return_context(cmp_context);
				}

				return is_equal;
			}
			bool array::less(const void* a, const void* b, immediate_context* context, scache* cache)
			{
				if (sub_type_id & ~(size_t)type_id::mask_seqnbr_t)
				{
					if (sub_type_id & (size_t)type_id::handle_t)
					{
						if (*(void**)a == 0)
							return true;

						if (*(void**)b == 0)
							return false;
					}

					if (!cache || !cache->comparator)
						return false;

					bool is_less = false;
					context->execute_subcall(cache->comparator, [a, b](immediate_context* context)
					{
						context->set_object((void*)a);
						context->set_arg_object(0, (void*)b);
					}, [&is_less](immediate_context* context) { is_less = (context->get_return_dword() < 0); });
					return is_less;
				}

				switch (sub_type_id)
				{
#define COMPARE(t) *((t*)a) < *((t*)b)
					case (size_t)type_id::bool_t: return COMPARE(bool);
					case (size_t)type_id::int8_t: return COMPARE(signed char);
					case (size_t)type_id::uint8_t: return COMPARE(unsigned char);
					case (size_t)type_id::int16_t: return COMPARE(signed short);
					case (size_t)type_id::uint16_t: return COMPARE(unsigned short);
					case (size_t)type_id::int32_t: return COMPARE(signed int);
					case (size_t)type_id::uint32_t: return COMPARE(uint32_t);
					case (size_t)type_id::float_t: return COMPARE(float);
					case (size_t)type_id::double_t: return COMPARE(double);
					default: return COMPARE(signed int);
#undef COMPARE
				}

				return false;
			}
			bool array::equals(const void* a, const void* b, immediate_context* context, scache* cache) const
			{
				if (sub_type_id & ~(size_t)type_id::mask_seqnbr_t)
				{
					if (sub_type_id & (size_t)type_id::handle_t)
					{
						if (*(void**)a == *(void**)b)
							return true;
					}

					if (cache && cache->equals)
					{
						bool is_matched = false;
						context->execute_subcall(cache->equals, [a, b](immediate_context* context)
						{
							context->set_object((void*)a);
							context->set_arg_object(0, (void*)b);
						}, [&is_matched](immediate_context* context) { is_matched = (context->get_return_byte() != 0); });
						return is_matched;
					}

					if (cache && cache->comparator)
					{
						bool is_matched = false;
						context->execute_subcall(cache->comparator, [a, b](immediate_context* context)
						{
							context->set_object((void*)a);
							context->set_arg_object(0, (void*)b);
						}, [&is_matched](immediate_context* context) { is_matched = (context->get_return_dword() == 0); });
						return is_matched;
					}

					return false;
				}

				switch (sub_type_id)
				{
#define COMPARE(t) *((t*)a) == *((t*)b)
					case (size_t)type_id::bool_t: return COMPARE(bool);
					case (size_t)type_id::int8_t: return COMPARE(signed char);
					case (size_t)type_id::uint8_t: return COMPARE(unsigned char);
					case (size_t)type_id::int16_t: return COMPARE(signed short);
					case (size_t)type_id::uint16_t: return COMPARE(unsigned short);
					case (size_t)type_id::int32_t: return COMPARE(signed int);
					case (size_t)type_id::uint32_t: return COMPARE(uint32_t);
					case (size_t)type_id::float_t: return COMPARE(float);
					case (size_t)type_id::double_t: return COMPARE(double);
					default: return COMPARE(signed int);
#undef COMPARE
				}
			}
			size_t array::find_by_ref(void* value, size_t start_at) const
			{
				size_t length = size();
				if (sub_type_id & (size_t)type_id::handle_t)
				{
					value = *(void**)value;
					for (size_t i = start_at; i < length; i++)
					{
						if (*(void**)at(i) == value)
							return i;
					}
				}
				else
				{
					for (size_t i = start_at; i < length; i++)
					{
						if (at(i) == value)
							return i;
					}
				}

				return std::string::npos;
			}
			size_t array::find(void* value, size_t start_at) const
			{
				scache* cache; size_t count = size();
				if (!is_eligible_for_find(&cache) || !count)
					return std::string::npos;

				immediate_context* context = immediate_context::get();
				for (size_t i = start_at; i < count; i++)
				{
					if (equals(at(i), value, context, cache))
						return i;
				}

				return std::string::npos;
			}
			void array::copy(void* dest, void* src)
			{
				memcpy(dest, src, element_size);
			}
			void* array::get_array_item_pointer(size_t index)
			{
				return buffer->data + index * element_size;
			}
			void* array::get_data_pointer(void* buffer_ptr)
			{
				if ((sub_type_id & (size_t)type_id::mask_object_t) && !(sub_type_id & (size_t)type_id::handle_t))
					return reinterpret_cast<void*>(*(size_t*)buffer_ptr);
				else
					return buffer_ptr;
			}
			void array::swap(size_t index1, size_t index2)
			{
				if (index1 >= size() || index2 >= size())
					return bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));

				unsigned char swap[16];
				copy(swap, get_array_item_pointer(index1));
				copy(get_array_item_pointer(index1), get_array_item_pointer(index2));
				copy(get_array_item_pointer(index2), swap);
			}
			void array::sort(asIScriptFunction* callback)
			{
				scache* cache; size_t count = size();
				if (!is_eligible_for_sort(&cache) || count < 2)
					return;

				unsigned char swap[16];
				immediate_context* context = immediate_context::get();
				if (callback != nullptr)
				{
					function_delegate delegatef(callback);
					for (size_t i = 1; i < count; i++)
					{
						int64_t j = (int64_t)(i - 1);
						copy(swap, get_array_item_pointer(i));
						while (j >= 0)
						{
							void* a = get_data_pointer(swap), * b = at(j); bool is_less = false;
							context->execute_subcall(delegatef.callable(), [a, b](immediate_context* context)
							{
								context->set_arg_address(0, a);
								context->set_arg_address(1, b);
							}, [&is_less](immediate_context* context) { is_less = (context->get_return_byte() > 0); });
							if (!is_less)
								break;

							copy(get_array_item_pointer(j + 1), get_array_item_pointer(j));
							j--;
						}
						copy(get_array_item_pointer(j + 1), swap);
					}
				}
				else
				{
					for (size_t i = 1; i < count; i++)
					{
						int64_t j = (int64_t)(i - 1);
						copy(swap, get_array_item_pointer(i));
						while (j >= 0 && less(get_data_pointer(swap), at(j), context, cache))
						{
							copy(get_array_item_pointer(j + 1), get_array_item_pointer(j));
							j--;
						}
						copy(get_array_item_pointer(j + 1), swap);
					}
				}
			}
			void array::copy_buffer(sbuffer* dest, sbuffer* src)
			{
				virtual_machine* engine = obj_type.get_vm();
				if (sub_type_id & (size_t)type_id::handle_t)
				{
					if (dest->num_elements > 0 && src->num_elements > 0)
					{
						int count = (int)(dest->num_elements > src->num_elements ? src->num_elements : dest->num_elements);
						void** max = (void**)(dest->data + count * sizeof(void*));
						void** d = (void**)dest->data;
						void** s = (void**)src->data;

						for (; d < max; d++, s++)
						{
							void* swap = *d;
							*d = *s;

							if (*d)
								engine->add_ref_object(*d, obj_type.get_sub_type());

							if (swap)
								engine->release_object(swap, obj_type.get_sub_type());
						}
					}
				}
				else
				{
					if (dest->num_elements > 0 && src->num_elements > 0)
					{
						int count = (int)(dest->num_elements > src->num_elements ? src->num_elements : dest->num_elements);
						if (sub_type_id & (size_t)type_id::mask_object_t)
						{
							void** max = (void**)(dest->data + count * sizeof(void*));
							void** d = (void**)dest->data;
							void** s = (void**)src->data;

							auto sub_type = obj_type.get_sub_type();
							for (; d < max; d++, s++)
								engine->assign_object(*d, *s, sub_type);
						}
						else
							memcpy(dest->data, src->data, (size_t)count * (size_t)element_size);
					}
				}
			}
			void array::precache()
			{
#ifdef VI_ANGELSCRIPT
				sub_type_id = obj_type.get_sub_type_id();
				if (!(sub_type_id & ~(size_t)type_id::mask_seqnbr_t))
					return;

				scache* cache = reinterpret_cast<scache*>(obj_type.get_user_data(array_id));
				if (cache)
					return;

				asAcquireExclusiveLock();
				cache = reinterpret_cast<scache*>(obj_type.get_user_data(array_id));
				if (cache)
					return asReleaseExclusiveLock();

				cache = core::memory::allocate<scache>(sizeof(scache));
				if (!cache)
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));
					return asReleaseExclusiveLock();
				}

				memset(cache, 0, sizeof(scache));
				bool must_be_const = (sub_type_id & (size_t)type_id::const_handle_t) ? true : false;

				auto sub_type = obj_type.get_vm()->get_type_info_by_id(sub_type_id);
				if (sub_type.is_valid())
				{
					for (size_t i = 0; i < sub_type.get_methods_count(); i++)
					{
						auto function = sub_type.get_method_by_index((int)i);
						if (function.get_args_count() == 1 && (!must_be_const || function.is_read_only()))
						{
							size_t flags = 0;
							int return_type_id = function.get_return_type_id(&flags);
							if (flags != (size_t)modifiers::none)
								continue;

							bool is_cmp = false, is_equals = false;
							if (return_type_id == (size_t)type_id::int32_t && function.get_name() == "opCmp")
								is_cmp = true;
							if (return_type_id == (size_t)type_id::bool_t && function.get_name() == "opEquals")
								is_equals = true;

							if (!is_cmp && !is_equals)
								continue;

							int param_type_id;
							function.get_arg(0, &param_type_id, &flags);

							if ((param_type_id & ~((size_t)type_id::handle_t | (size_t)type_id::const_handle_t)) != (sub_type_id & ~((size_t)type_id::handle_t | (size_t)type_id::const_handle_t)))
								continue;

							if ((flags & (size_t)modifiers::in_ref))
							{
								if ((param_type_id & (size_t)type_id::handle_t) || (must_be_const && !(flags & (size_t)modifiers::constant)))
									continue;
							}
							else if (param_type_id & (size_t)type_id::handle_t)
							{
								if (must_be_const && !(param_type_id & (size_t)type_id::const_handle_t))
									continue;
							}
							else
								continue;

							if (is_cmp)
							{
								if (cache->comparator || cache->comparator_return_code)
								{
									cache->comparator = 0;
									cache->comparator_return_code = (int)virtual_error::multiple_functions;
								}
								else
									cache->comparator = function.get_function();
							}
							else if (is_equals)
							{
								if (cache->equals || cache->equals_return_code)
								{
									cache->equals = 0;
									cache->equals_return_code = (int)virtual_error::multiple_functions;
								}
								else
									cache->equals = function.get_function();
							}
						}
					}
				}

				if (cache->equals == 0 && cache->equals_return_code == 0)
					cache->equals_return_code = (int)virtual_error::no_function;
				if (cache->comparator == 0 && cache->comparator_return_code == 0)
					cache->comparator_return_code = (int)virtual_error::no_function;

				obj_type.set_user_data(cache, array_id);
				asReleaseExclusiveLock();
#endif
			}
			void array::enum_references(asIScriptEngine* engine)
			{
				if (sub_type_id & (size_t)type_id::mask_object_t)
				{
					void** data = (void**)buffer->data;
					virtual_machine* vm = virtual_machine::get(engine);
					auto sub_type = vm->get_type_info_by_id(sub_type_id);
					if ((sub_type.flags() & (size_t)object_behaviours::ref))
					{
						for (size_t i = 0; i < buffer->num_elements; i++)
							function_factory::gc_enum_callback(engine, data[i]);
					}
					else if ((sub_type.flags() & (size_t)object_behaviours::value) && (sub_type.flags() & (size_t)object_behaviours::gc))
					{
						for (size_t i = 0; i < buffer->num_elements; i++)
						{
							if (data[i])
								vm->forward_enum_references(data[i], sub_type);
						}
					}
				}
			}
			void array::release_references(asIScriptEngine*)
			{
				resize(0);
			}
			array* array::create(asITypeInfo* info, size_t length)
			{
				array* result = new array(length, info);
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}
			array* array::create(asITypeInfo* info, size_t length, void* default_value)
			{
				array* result = new array(length, default_value, info);
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}
			array* array::create(asITypeInfo* info)
			{
				return array::create(info, (size_t)0);
			}
			array* array::factory(asITypeInfo* info, void* init_list)
			{
				array* result = new array(info, init_list);
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}
			void array::cleanup_type_info_cache(asITypeInfo* type_context)
			{
				type_info type(type_context);
				array::scache* cache = reinterpret_cast<array::scache*>(type.get_user_data(array_id));
				if (cache != nullptr)
				{
					cache->~scache();
					core::memory::deallocate(cache);
				}
			}
			bool array::template_callback(asITypeInfo* info_context, bool& dont_garbage_collect)
			{
				type_info info(info_context);
				int type_id = info.get_sub_type_id();
				if (type_id == (size_t)type_id::void_t)
					return false;

				if ((type_id & (size_t)type_id::mask_object_t) && !(type_id & (size_t)type_id::handle_t))
				{
					virtual_machine* engine = info.get_vm();
					auto sub_type = engine->get_type_info_by_id(type_id);
					size_t flags = sub_type.flags();

					if ((flags & (size_t)object_behaviours::value) && !(flags & (size_t)object_behaviours::pod))
					{
						bool found = false;
						for (size_t i = 0; i < sub_type.get_behaviour_count(); i++)
						{
							behaviours properties;
							function func = sub_type.get_behaviour_by_index(i, &properties);
							if (properties != behaviours::construct)
								continue;

							if (func.get_args_count() == 0)
							{
								found = true;
								break;
							}
						}

						if (!found)
						{
							engine->write_message(TYPENAME_ARRAY, 0, 0, log_category::err, "The subtype has no default constructor");
							return false;
						}
					}
					else if ((flags & (size_t)object_behaviours::ref))
					{
						bool found = false;
						if (!engine->get_property(features::disallow_value_assign_for_ref_type))
						{
							for (size_t i = 0; i < sub_type.get_factories_count(); i++)
							{
								function func = sub_type.get_factory_by_index(i);
								if (func.get_args_count() == 0)
								{
									found = true;
									break;
								}
							}
						}

						if (!found)
						{
							engine->write_message(TYPENAME_ARRAY, 0, 0, log_category::err, "The subtype has no default factory");
							return false;
						}
					}

					if (!(flags & (size_t)object_behaviours::gc))
						dont_garbage_collect = true;
				}
				else if (!(type_id & (size_t)type_id::handle_t))
				{
					dont_garbage_collect = true;
				}
				else
				{
					auto sub_type = info.get_vm()->get_type_info_by_id(type_id);
					size_t flags = sub_type.flags();

					if (!(flags & (size_t)object_behaviours::gc))
					{
						if ((flags & (size_t)object_behaviours::script_object))
						{
							if ((flags & (size_t)object_behaviours::noinherit))
								dont_garbage_collect = true;
						}
						else
							dont_garbage_collect = true;
					}
				}

				return true;
			}
			bool array::is_eligible_for_find(scache** output) const
			{
#ifdef VI_ANGELSCRIPT
				scache* cache = reinterpret_cast<scache*>(obj_type.get_user_data(get_id()));
				if (!(sub_type_id & ~asTYPEID_MASK_SEQNBR))
				{
					*output = cache;
					return true;
				}

				if (cache != nullptr && cache->equals != nullptr)
				{
					*output = cache;
					return true;
				}

				immediate_context* context = immediate_context::get();
				if (context != nullptr)
				{
					if (cache && cache->equals_return_code == asMULTIPLE_FUNCTIONS)
						bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_TEMPLATEMULTIPLEEQCOMPARATORS));
					else
						bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_TEMPLATENOEQCOMPARATORS));
				}
#endif
				* output = nullptr;
				return false;
			}
			bool array::is_eligible_for_sort(scache** output) const
			{
#ifdef VI_ANGELSCRIPT
				scache* cache = reinterpret_cast<scache*>(obj_type.get_user_data(get_id()));
				if (!(sub_type_id & ~asTYPEID_MASK_SEQNBR))
				{
					*output = cache;
					return true;
				}

				if (cache != nullptr && cache->comparator != nullptr)
				{
					*output = cache;
					return true;
				}

				immediate_context* context = immediate_context::get();
				if (context != nullptr)
				{
					if (cache && cache->comparator_return_code == asMULTIPLE_FUNCTIONS)
						bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_TEMPLATEMULTIPLECOMPARATORS));
					else
						bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_TEMPLATENOCOMPARATORS));
				}
#endif
				* output = nullptr;
				return false;
			}
			int array::get_id()
			{
				return array_id;
			}
			int array::array_id = 1431;

			storable::storable() noexcept
			{
			}
			storable::storable(virtual_machine* engine, void* value, int _TypeId) noexcept
			{
				set(engine, value, _TypeId);
			}
			storable::~storable() noexcept
			{
				if (value.object && value.type_id)
				{
					virtual_machine* vm = virtual_machine::get();
					if (vm != nullptr)
						release_references(vm->get_engine());
				}
			}
			void storable::release_references(asIScriptEngine* engine)
			{
				if (value.type_id & (size_t)type_id::mask_object_t)
				{
					virtual_machine* vm = virtual_machine::get(engine);
					vm->release_object(value.object, vm->get_type_info_by_id(value.type_id));
					value.clean();
				}
			}
			void storable::enum_references(asIScriptEngine* _Engine)
			{
				function_factory::gc_enum_callback(_Engine, value.object);
				if (value.type_id)
					function_factory::gc_enum_callback(_Engine, virtual_machine::get(_Engine)->get_type_info_by_id(value.type_id).get_type_info());
			}
			void storable::set(virtual_machine* engine, void* pointer, int _TypeId)
			{
				release_references(engine->get_engine());
				value.type_id = _TypeId;

				if (value.type_id & (size_t)type_id::handle_t)
				{
					value.object = *(void**)pointer;
					engine->add_ref_object(value.object, engine->get_type_info_by_id(value.type_id));
				}
				else if (value.type_id & (size_t)type_id::mask_object_t)
				{
					value.object = engine->create_object_copy(pointer, engine->get_type_info_by_id(value.type_id));
					if (value.object == 0)
						bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_COPYFAIL));
				}
				else
				{
					auto size = engine->get_size_of_primitive_type(value.type_id);
					if (size)
						memcpy(&value.integer, pointer, *size);
				}
			}
			void storable::set(virtual_machine* engine, storable& other)
			{
				if (other.value.type_id & (size_t)type_id::handle_t)
					set(engine, (void*)&other.value.object, other.value.type_id);
				else if (other.value.type_id & (size_t)type_id::mask_object_t)
					set(engine, (void*)other.value.object, other.value.type_id);
				else
					set(engine, (void*)&other.value.integer, other.value.type_id);
			}
			bool storable::get(virtual_machine* engine, void* pointer, int _TypeId) const
			{
				if (_TypeId & (size_t)type_id::handle_t)
				{
					if ((_TypeId & (size_t)type_id::mask_object_t))
					{
						if ((value.type_id & (size_t)type_id::const_handle_t) && !(_TypeId & (size_t)type_id::const_handle_t))
							return false;

						engine->ref_cast_object(value.object, engine->get_type_info_by_id(value.type_id), engine->get_type_info_by_id(_TypeId), reinterpret_cast<void**>(pointer));
						return true;
					}
				}
				else if (_TypeId & (size_t)type_id::mask_object_t)
				{
					bool isCompatible = false;
					if ((value.type_id & ~((size_t)type_id::handle_t | (size_t)type_id::const_handle_t)) == _TypeId && value.object != 0)
						isCompatible = true;

					if (isCompatible)
					{
						engine->assign_object(pointer, value.object, engine->get_type_info_by_id(value.type_id));
						return true;
					}
				}
				else
				{
					if (value.type_id == _TypeId)
					{
						auto size = engine->get_size_of_primitive_type(_TypeId);
						if (size)
							memcpy(pointer, &value.integer, *size);
						return true;
					}

					if (_TypeId == (size_t)type_id::double_t)
					{
						if (value.type_id == (size_t)type_id::int64_t)
						{
							*(double*)pointer = double(value.integer);
						}
						else if (value.type_id == (size_t)type_id::bool_t)
						{
							char local;
							memcpy(&local, &value.integer, sizeof(char));
							*(double*)pointer = local ? 1.0 : 0.0;
						}
						else if (value.type_id > (size_t)type_id::double_t && (value.type_id & (size_t)type_id::mask_object_t) == 0)
						{
							int local;
							memcpy(&local, &value.integer, sizeof(int));
							*(double*)pointer = double(local);
						}
						else
						{
							*(double*)pointer = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId == (size_t)type_id::int64_t)
					{
						if (value.type_id == (size_t)type_id::double_t)
						{
							*(as_int64_t*)pointer = as_int64_t(value.number);
						}
						else if (value.type_id == (size_t)type_id::bool_t)
						{
							char local;
							memcpy(&local, &value.integer, sizeof(char));
							*(as_int64_t*)pointer = local ? 1 : 0;
						}
						else if (value.type_id > (size_t)type_id::double_t && (value.type_id & (size_t)type_id::mask_object_t) == 0)
						{
							int local;
							memcpy(&local, &value.integer, sizeof(int));
							*(as_int64_t*)pointer = local;
						}
						else
						{
							*(as_int64_t*)pointer = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId > (size_t)type_id::double_t && (value.type_id & (size_t)type_id::mask_object_t) == 0)
					{
						if (value.type_id == (size_t)type_id::double_t)
						{
							*(int*)pointer = int(value.number);
						}
						else if (value.type_id == (size_t)type_id::int64_t)
						{
							*(int*)pointer = int(value.integer);
						}
						else if (value.type_id == (size_t)type_id::bool_t)
						{
							char local;
							memcpy(&local, &value.integer, sizeof(char));
							*(int*)pointer = local ? 1 : 0;
						}
						else if (value.type_id > (size_t)type_id::double_t && (value.type_id & (size_t)type_id::mask_object_t) == 0)
						{
							int local;
							memcpy(&local, &value.integer, sizeof(int));
							*(int*)pointer = local;
						}
						else
						{
							*(int*)pointer = 0;
							return false;
						}

						return true;
					}
					else if (_TypeId == (size_t)type_id::bool_t)
					{
						if (value.type_id & (size_t)type_id::handle_t)
						{
							*(bool*)pointer = value.object ? true : false;
						}
						else if (value.type_id & (size_t)type_id::mask_object_t)
						{
							*(bool*)pointer = true;
						}
						else
						{
							as_uint64_t zero = 0;
							auto size = engine->get_size_of_primitive_type(value.type_id);
							if (size)
								*(bool*)pointer = memcmp(&value.integer, &zero, *size) == 0 ? false : true;
						}

						return true;
					}
				}

				return false;
			}
			const void* storable::get_address_of_value() const
			{
				if ((value.type_id & (size_t)type_id::mask_object_t) && !(value.type_id & (size_t)type_id::handle_t))
					return value.object;

				return reinterpret_cast<const void*>(&value.object);
			}
			int storable::get_type_id() const
			{
				return value.type_id;
			}

			dictionary::local_iterator::local_iterator(const dictionary& from, internal_map::const_iterator it) noexcept : it(it), base(from)
			{
			}
			void dictionary::local_iterator::operator++()
			{
				++it;
			}
			void dictionary::local_iterator::operator++(int)
			{
				++it;
			}
			dictionary::local_iterator& dictionary::local_iterator::operator*()
			{
				return *this;
			}
			bool dictionary::local_iterator::operator==(const local_iterator& other) const
			{
				return it == other.it;
			}
			bool dictionary::local_iterator::operator!=(const local_iterator& other) const
			{
				return it != other.it;
			}
			const core::string& dictionary::local_iterator::get_key() const
			{
				return it->first;
			}
			int dictionary::local_iterator::get_type_id() const
			{
				return it->second.value.type_id;
			}
			bool dictionary::local_iterator::get_value(void* pointer, int type_id) const
			{
				return it->second.get(base.engine, pointer, type_id);
			}
			const void* dictionary::local_iterator::get_address_of_value() const
			{
				return it->second.get_address_of_value();
			}

			dictionary::dictionary(virtual_machine* _Engine) noexcept : engine(_Engine)
			{
				dictionary::scache* cache = reinterpret_cast<dictionary::scache*>(engine->get_user_data(dictionary_id));
				engine->notify_of_new_object(this, cache->dictionary_type);
			}
			dictionary::dictionary(unsigned char* buffer) noexcept : dictionary(virtual_machine::get())
			{
#ifdef VI_ANGELSCRIPT
				dictionary::scache& cache = *reinterpret_cast<dictionary::scache*>(engine->get_user_data(dictionary_id));
				bool key_as_ref = cache.key_type.flags() & (size_t)object_behaviours::ref ? true : false;
				size_t length = *(asUINT*)buffer;
				buffer += 4;

				while (length--)
				{
					if (asPWORD(buffer) & 0x3)
						buffer += 4 - (asPWORD(buffer) & 0x3);

					core::string name;
					if (key_as_ref)
					{
						name = **(core::string**)buffer;
						buffer += sizeof(core::string*);
					}
					else
					{
						name = *(core::string*)buffer;
						buffer += sizeof(core::string);
					}

					int type_id = *(int*)buffer;
					buffer += sizeof(int);

					void* ref_ptr = (void*)buffer;
					if (type_id >= (size_t)type_id::int8_t && type_id <= (size_t)type_id::double_t)
					{
						as_int64_t integer64;
						double double64;

						switch (type_id)
						{
							case (size_t)type_id::int8_t:
								integer64 = *(char*)ref_ptr;
								break;
							case (size_t)type_id::int16_t:
								integer64 = *(short*)ref_ptr;
								break;
							case (size_t)type_id::int32_t:
								integer64 = *(int*)ref_ptr;
								break;
							case (size_t)type_id::uint8_t:
								integer64 = *(unsigned char*)ref_ptr;
								break;
							case (size_t)type_id::uint16_t:
								integer64 = *(unsigned short*)ref_ptr;
								break;
							case (size_t)type_id::uint32_t:
								integer64 = *(uint32_t*)ref_ptr;
								break;
							case (size_t)type_id::int64_t:
							case (size_t)type_id::uint64_t:
								integer64 = *(as_int64_t*)ref_ptr;
								break;
							case (size_t)type_id::float_t:
								double64 = *(float*)ref_ptr;
								break;
							case (size_t)type_id::double_t:
								double64 = *(double*)ref_ptr;
								break;
						}

						if (type_id >= (size_t)type_id::float_t)
							set(name, &double64, (size_t)type_id::double_t);
						else
							set(name, &integer64, (size_t)type_id::int64_t);
					}
					else
					{
						if ((type_id & (size_t)type_id::mask_object_t) && !(type_id & (size_t)type_id::handle_t) && (engine->get_type_info_by_id(type_id).flags() & (size_t)object_behaviours::ref))
							ref_ptr = *(void**)ref_ptr;

						set(name, ref_ptr, engine->is_nullable(type_id) ? 0 : type_id);
					}

					if (type_id & (size_t)type_id::mask_object_t)
					{
						auto info = engine->get_type_info_by_id(type_id);
						if (info.flags() & (size_t)object_behaviours::value)
							buffer += info.size();
						else
							buffer += sizeof(void*);
					}
					else if (type_id == 0)
						buffer += sizeof(void*);
					else
						buffer += engine->get_size_of_primitive_type(type_id).or_else(0);
				}
#endif
			}
			dictionary::dictionary(const dictionary& other) noexcept : dictionary(other.engine)
			{
				for (auto it = other.data.begin(); it != other.data.end(); ++it)
				{
					auto& key = it->second;
					if (key.value.type_id & (size_t)type_id::handle_t)
						set(it->first, (void*)&key.value.object, key.value.type_id);
					else if (key.value.type_id & (size_t)type_id::mask_object_t)
						set(it->first, (void*)key.value.object, key.value.type_id);
					else
						set(it->first, (void*)&key.value.integer, key.value.type_id);
				}
			}
			dictionary::~dictionary() noexcept
			{
				clear();
			}
			void dictionary::enum_references(asIScriptEngine* _Engine)
			{
				for (auto it = data.begin(); it != data.end(); ++it)
				{
					auto& key = it->second;
					if (key.value.type_id & (size_t)type_id::mask_object_t)
					{
						auto sub_type = engine->get_type_info_by_id(key.value.type_id);
						if ((sub_type.flags() & (size_t)object_behaviours::value) && (sub_type.flags() & (size_t)object_behaviours::gc))
							engine->forward_enum_references(key.value.object, sub_type);
						else
							function_factory::gc_enum_callback(_Engine, key.value.object);
					}
				}
			}
			void dictionary::release_references(asIScriptEngine*)
			{
				clear();
			}
			dictionary& dictionary::operator =(const dictionary& other) noexcept
			{
				clear();
				for (auto it = other.data.begin(); it != other.data.end(); ++it)
				{
					auto& key = it->second;
					if (key.value.type_id & (size_t)type_id::handle_t)
						set(it->first, (void*)&key.value.object, key.value.type_id);
					else if (key.value.type_id & (size_t)type_id::mask_object_t)
						set(it->first, (void*)key.value.object, key.value.type_id);
					else
						set(it->first, (void*)&key.value.integer, key.value.type_id);
				}

				return *this;
			}
			storable* dictionary::operator[](const std::string_view& key)
			{
				auto it = data.find(core::key_lookup_cast(key));
				if (it != data.end())
					return &it->second;

				return &data[core::string(key)];
			}
			const storable* dictionary::operator[](const std::string_view& key) const
			{
				auto it = data.find(core::key_lookup_cast(key));
				if (it != data.end())
					return &it->second;

				bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_ACCESSINVALID));
				return 0;
			}
			storable* dictionary::operator[](size_t index)
			{
				if (index < data.size())
				{
					auto it = data.begin();
					while (index--)
						it++;
					return &it->second;
				}

				bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_ACCESSINVALID));
				return 0;
			}
			const storable* dictionary::operator[](size_t index) const
			{
				if (index < data.size())
				{
					auto it = data.begin();
					while (index--)
						it++;
					return &it->second;
				}

				bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_ACCESSINVALID));
				return 0;
			}
			void dictionary::set(const std::string_view& key, void* value, int type_id)
			{
				auto it = data.find(core::key_lookup_cast(key));
				if (it == data.end())
					it = data.insert(internal_map::value_type(key, storable())).first;

				it->second.set(engine, value, type_id);
			}
			bool dictionary::get(const std::string_view& key, void* value, int type_id) const
			{
				auto it = data.find(core::key_lookup_cast(key));
				if (it != data.end())
					return it->second.get(engine, value, type_id);

				return false;
			}
			bool dictionary::get_index(size_t index, core::string* key, void** value, int* type_id) const
			{
				if (index >= data.size())
					return false;

				auto it = begin();
				size_t offset = 0;

				while (offset != index)
				{
					++offset;
					++it;
				}

				if (key != nullptr)
					*key = it.get_key();

				if (value != nullptr)
					*value = (void*)it.get_address_of_value();

				if (type_id != nullptr)
					*type_id = it.get_type_id();

				return true;
			}
			bool dictionary::try_get_index(size_t index, core::string* key, void* value, int type_id) const
			{
				if (index >= data.size())
					return false;

				auto it = begin();
				size_t offset = 0;

				while (offset != index)
				{
					++offset;
					++it;
				}

				return it.get_value(value, type_id);
			}
			int dictionary::get_type_id(const std::string_view& key) const
			{
				auto it = data.find(core::key_lookup_cast(key));
				if (it != data.end())
					return it->second.value.type_id;

				return -1;
			}
			bool dictionary::exists(const std::string_view& key) const
			{
				auto it = data.find(core::key_lookup_cast(key));
				if (it != data.end())
					return true;

				return false;
			}
			bool dictionary::empty() const
			{
				if (data.size() == 0)
					return true;

				return false;
			}
			size_t dictionary::size() const
			{
				return size_t(data.size());
			}
			bool dictionary::erase(const std::string_view& key)
			{
				auto it = data.find(core::key_lookup_cast(key));
				if (it != data.end())
				{
					it->second.release_references(engine->get_engine());
					data.erase(it);
					return true;
				}

				return false;
			}
			void dictionary::clear()
			{
				asIScriptEngine* target = engine->get_engine();
				for (auto it = data.begin(); it != data.end(); ++it)
					it->second.release_references(target);
				data.clear();
			}
			array* dictionary::get_keys() const
			{
				dictionary::scache* cache = reinterpret_cast<dictionary::scache*>(engine->get_user_data(dictionary_id));
				array* array = array::create(cache->array_type.get_type_info(), size_t(data.size()));
				size_t current = 0;

				for (auto it = data.begin(); it != data.end(); ++it)
					*(core::string*)array->at(current++) = it->first;

				return array;
			}
			dictionary::local_iterator dictionary::begin() const
			{
				return local_iterator(*this, data.begin());
			}
			dictionary::local_iterator dictionary::end() const
			{
				return local_iterator(*this, data.end());
			}
			dictionary::local_iterator dictionary::find(const std::string_view& key) const
			{
				return local_iterator(*this, data.find(core::key_lookup_cast(key)));
			}
			dictionary* dictionary::create(virtual_machine* engine)
			{
				dictionary* result = new dictionary(engine);
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}
			dictionary* dictionary::create(unsigned char* buffer)
			{
				dictionary* result = new dictionary(buffer);
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}
			void dictionary::setup(virtual_machine* engine)
			{
				dictionary::scache* cache = reinterpret_cast<dictionary::scache*>(engine->get_user_data(dictionary_id));
				if (cache == 0)
				{
					cache = core::memory::init<dictionary::scache>();
					engine->set_user_data(cache, dictionary_id);
					engine->set_engine_user_data_cleanup_callback([](asIScriptEngine* engine)
					{
						virtual_machine* vm = virtual_machine::get(engine);
						dictionary::scache* cache = reinterpret_cast<dictionary::scache*>(vm->get_user_data(dictionary_id));
						core::memory::deinit(cache);
					}, dictionary_id);

					cache->dictionary_type = engine->get_type_info_by_name(TYPENAME_DICTIONARY).get_type_info();
					cache->array_type = engine->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">").get_type_info();
					cache->key_type = engine->get_type_info_by_decl(TYPENAME_STRING).get_type_info();
				}
			}
			void dictionary::factory(asIScriptGeneric* genericf)
			{
				generic_context args = genericf;
				*(dictionary**)args.get_address_of_return_location() = dictionary::create(args.get_vm());
			}
			void dictionary::list_factory(asIScriptGeneric* genericf)
			{
				generic_context args = genericf;
				unsigned char* buffer = (unsigned char*)args.get_arg_address(0);
				*(dictionary**)args.get_address_of_return_location() = dictionary::create(buffer);
			}
			void dictionary::key_create(void* memory)
			{
				new(memory) storable();
			}
			void dictionary::key_destroy(storable* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (vm != nullptr)
					base->release_references(vm->get_engine());

				base->~storable();
			}
			storable& dictionary::keyop_assign(storable* base, void* ref_ptr, int type_id)
			{
				virtual_machine* vm = virtual_machine::get();
				if (vm != nullptr)
					base->set(vm, ref_ptr, type_id);

				return *base;
			}
			storable& dictionary::keyop_assign(storable* base, const storable& other)
			{
				virtual_machine* vm = virtual_machine::get();
				if (vm != nullptr)
					base->set(vm, const_cast<storable&>(other));

				return *base;
			}
			storable& dictionary::keyop_assign(storable* base, double value)
			{
				return keyop_assign(base, &value, (size_t)type_id::double_t);
			}
			storable& dictionary::keyop_assign(storable* base, as_int64_t value)
			{
				return dictionary::keyop_assign(base, &value, (size_t)type_id::int64_t);
			}
			void dictionary::keyop_cast(storable* base, void* ref_ptr, int type_id)
			{
				virtual_machine* vm = virtual_machine::get();
				if (vm != nullptr)
					base->get(vm, ref_ptr, type_id);
			}
			as_int64_t dictionary::keyop_conv_int(storable* base)
			{
				as_int64_t value;
				keyop_cast(base, &value, (size_t)type_id::int64_t);
				return value;
			}
			double dictionary::keyop_conv_double(storable* base)
			{
				double value;
				keyop_cast(base, &value, (size_t)type_id::double_t);
				return value;
			}
			int dictionary::dictionary_id = 2348;

			core::string random::getb(uint64_t size)
			{
				return compute::codec::hex_encode(*compute::crypto::random_bytes((size_t)size)).substr(0, (size_t)size);
			}
			double random::betweend(double min, double max)
			{
				return compute::mathd::random(min, max);
			}
			double random::magd()
			{
				return compute::mathd::random_mag();
			}
			double random::getd()
			{
				return compute::mathd::random();
			}
			float random::betweenf(float min, float max)
			{
				return compute::mathf::random(min, max);
			}
			float random::magf()
			{
				return compute::mathf::random_mag();
			}
			float random::getf()
			{
				return compute::mathf::random();
			}
			uint64_t random::betweeni(uint64_t min, uint64_t max)
			{
				return compute::math<uint64_t>::random(min, max);
			}

			promise::promise(immediate_context* new_context) noexcept : engine(nullptr), context(new_context), value(-1)
			{
				VI_ASSERT(context != nullptr, "context should not be null");
				context->add_ref();
				engine = context->get_vm();
				engine->notify_of_new_object(this, engine->get_type_info_by_name(TYPENAME_PROMISE));
			}
			promise::~promise() noexcept
			{
				release_references(nullptr);
			}
			void promise::enum_references(asIScriptEngine* other_engine)
			{
				if (value.object != nullptr && (value.type_id & (size_t)type_id::mask_object_t))
				{
					auto sub_type = engine->get_type_info_by_id(value.type_id);
					if ((sub_type.flags() & (size_t)object_behaviours::ref))
						function_factory::gc_enum_callback(other_engine, value.object);
					else if ((sub_type.flags() & (size_t)object_behaviours::value) && (sub_type.flags() & (size_t)object_behaviours::gc))
						engine->forward_enum_references(value.object, sub_type);

					auto type = virtual_machine::get(other_engine)->get_type_info_by_id(value.type_id);
					function_factory::gc_enum_callback(other_engine, type.get_type_info());
				}
				function_factory::gc_enum_callback(other_engine, &delegatef);
				function_factory::gc_enum_callback(other_engine, context);
			}
			void promise::release_references(asIScriptEngine*)
			{
				if (value.type_id & (size_t)type_id::mask_object_t)
				{
					auto type = engine->get_type_info_by_id(value.type_id);
					engine->release_object(value.object, type);
					type.release();
					value.clean();
				}
				if (delegatef.is_valid())
					delegatef.release();
				core::memory::release(context);
			}
			int promise::get_type_id()
			{
				return value.type_id;
			}
			void* promise::get_address_of_object()
			{
				return retrieve();
			}
			void promise::when(asIScriptFunction* new_callback)
			{
				core::umutex<std::mutex> unique(update);
				delegatef = function_delegate(new_callback);
				if (delegatef.is_valid() && !is_pending())
				{
					auto delegate_return = std::move(delegatef);
					unique.negate();
					context->resolve_callback(std::move(delegate_return), [this](immediate_context* context) { context->set_arg_object(0, this); }, nullptr);
				}
			}
			void promise::when(std::function<void(promise*)>&& new_callback)
			{
				core::umutex<std::mutex> unique(update);
				bounce = std::move(new_callback);
				if (bounce && !is_pending())
				{
					auto native_return = std::move(bounce);
					unique.negate();
					native_return(this);
				}
			}
			void promise::store(void* ref_pointer, int ref_type_id)
			{
				VI_ASSERT(value.type_id == promise_null, "promise should be settled only once");
				VI_ASSERT(ref_pointer != nullptr || ref_type_id == (size_t)type_id::void_t, "input pointer should not be null");
				VI_ASSERT(engine != nullptr, "promise is malformed (engine is null)");
				VI_ASSERT(context != nullptr, "promise is malformed (context is null)");

				core::umutex<std::mutex> unique(update);
				if (value.type_id != promise_null)
					return bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_PROMISEREADY));

				if ((ref_type_id & (size_t)type_id::mask_object_t))
				{
					auto type = engine->get_type_info_by_id(ref_type_id);
					if (type.is_valid())
						type.add_ref();
				}

				value.type_id = ref_type_id;
				if (value.type_id & (size_t)type_id::handle_t)
				{
					value.object = *(void**)ref_pointer;
				}
				else if (value.type_id & (size_t)type_id::mask_object_t)
				{
					value.object = engine->create_object_copy(ref_pointer, engine->get_type_info_by_id(value.type_id));
				}
				else if (ref_pointer != nullptr)
				{
					value.integer = 0;
					auto size = engine->get_size_of_primitive_type(value.type_id);
					if (size)
						memcpy(&value.integer, ref_pointer, *size);
				}

				bool suspend_owned = context->get_user_data(promise_ud) == (void*)this;
				if (suspend_owned)
					context->set_user_data(nullptr, promise_ud);

				bool wants_resume = (context->is_suspended() && suspend_owned);
				auto native_return = std::move(bounce);
				auto delegate_return = std::move(delegatef);
				unique.negate();

				if (native_return)
					native_return(this);

				if (delegate_return.is_valid())
					context->resolve_callback(std::move(delegate_return), [this](immediate_context* context) { context->set_arg_object(0, this); }, nullptr);

				if (wants_resume)
					context->resolve_notification();
			}
			void promise::store(void* ref_pointer, const char* type_name)
			{
				VI_ASSERT(engine != nullptr, "promise is malformed (engine is null)");
				VI_ASSERT(type_name != nullptr, "typename should not be null");
				store(ref_pointer, engine->get_type_id_by_decl(type_name));
			}
			void promise::store_exception(const exception::pointer& ref_value)
			{
				VI_ASSERT(context != nullptr, "promise is malformed (context is null)");
				context->enable_deferred_exceptions();
				exception::throw_ptr_at(context, ref_value);
				context->disable_deferred_exceptions();
				store(nullptr, (int)type_id::void_t);
			}
			void promise::store_void()
			{
				store(nullptr, (int)type_id::void_t);
			}
			bool promise::retrieve(void* ref_pointer, int ref_type_id)
			{
				VI_ASSERT(engine != nullptr, "promise is malformed (engine is null)");
				VI_ASSERT(ref_pointer != nullptr, "output pointer should not be null");
				if (value.type_id == promise_null)
					return false;

				if (ref_type_id & (size_t)type_id::handle_t)
				{
					if ((value.type_id & (size_t)type_id::mask_object_t))
					{
						if ((value.type_id & (size_t)type_id::const_handle_t) && !(ref_type_id & (size_t)type_id::const_handle_t))
							return false;

						engine->ref_cast_object(value.object, engine->get_type_info_by_id(value.type_id), engine->get_type_info_by_id(ref_type_id), reinterpret_cast<void**>(ref_pointer));
#ifdef VI_ANGELSCRIPT
						if (*(asPWORD*)ref_pointer == 0)
							return false;
#endif
						return true;
					}
				}
				else if (ref_type_id & (size_t)type_id::mask_object_t)
				{
					if (value.type_id == ref_type_id)
					{
						engine->assign_object(ref_pointer, value.object, engine->get_type_info_by_id(value.type_id));
						return true;
					}
				}
				else
				{
					auto size1 = engine->get_size_of_primitive_type(value.type_id);
					auto size2 = engine->get_size_of_primitive_type(ref_type_id);
					if (size1 && size2 && *size1 == *size2)
					{
						memcpy(ref_pointer, &value.integer, *size1);
						return true;
					}
				}

				return false;
			}
			void promise::retrieve_void()
			{
				core::umutex<std::mutex> unique(update);
				immediate_context* context = immediate_context::get();
				if (context != nullptr && !context->rethrow_deferred_exception())
				{
					if (value.type_id == promise_null)
						bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_PROMISENOTREADY));
				}
			}
			void* promise::retrieve()
			{
				retrieve_void();
				if (value.type_id == promise_null)
					return nullptr;

				if (value.type_id & (size_t)type_id::handle_t)
					return &value.object;
				else if (value.type_id & (size_t)type_id::mask_object_t)
					return value.object;
				else if (value.type_id <= (size_t)type_id::double_t || value.type_id & (size_t)type_id::mask_seqnbr_t)
					return &value.integer;

				return nullptr;
			}
			bool promise::is_pending()
			{
				return value.type_id == promise_null;
			}
			promise* promise::yield_if()
			{
				core::umutex<std::mutex> unique(update);
				if (value.type_id == promise_null && context != nullptr && context->suspend())
					context->set_user_data(this, promise_ud);

				return this;
			}
			promise* promise::create_factory(void* _Ref, int type_id)
			{
				promise* future = new promise(immediate_context::get());
				if (!future)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				if (type_id != (size_t)type_id::void_t)
					future->store(_Ref, type_id);
				return future;
			}
			promise* promise::create_factory_type(asITypeInfo*)
			{
				promise* future = new promise(immediate_context::get());
				if (!future)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));
				return future;
			}
			promise* promise::create_factory_void()
			{
				promise* future = new promise(immediate_context::get());
				if (!future)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));
				return future;
			}
			bool promise::template_callback(asITypeInfo* info_context, bool& dont_garbage_collect)
			{
				type_info info(info_context);
				int type_id = info.get_sub_type_id();
				if (type_id == (size_t)type_id::void_t)
					return false;

				if ((type_id & (size_t)type_id::mask_object_t) && !(type_id & (size_t)type_id::handle_t))
				{
					virtual_machine* engine = info.get_vm();
					auto sub_type = engine->get_type_info_by_id(type_id);
					size_t flags = sub_type.flags();

					if (!(flags & (size_t)object_behaviours::gc))
						dont_garbage_collect = true;
				}
				else if (!(type_id & (size_t)type_id::handle_t))
				{
					dont_garbage_collect = true;
				}
				else
				{
					auto sub_type = info.get_vm()->get_type_info_by_id(type_id);
					size_t flags = sub_type.flags();

					if (!(flags & (size_t)object_behaviours::gc))
					{
						if ((flags & (size_t)object_behaviours::script_object))
						{
							if ((flags & (size_t)object_behaviours::noinherit))
								dont_garbage_collect = true;
						}
						else
							dont_garbage_collect = true;
					}
				}

				return true;
			}
			expects_vm<void> promise::generator_callback(compute::preprocessor*, const std::string_view&, core::string& code)
			{
				core::stringify::replace(code, "promise<void>", "promise_v");
				return parser::replace_inline_preconditions("co_await", code, [](const std::string_view& expression) -> expects_vm<core::string>
				{
					const char generator[] = ").yield().unwrap()";
					core::string result = "(";
					result.reserve(sizeof(generator) + expression.size());
					result.append(expression);
					result.append(generator, sizeof(generator) - 1);
					return result;
				});
			}
			bool promise::is_context_pending(immediate_context* context)
			{
				return context->is_pending() || context->get_user_data(promise_ud) != nullptr || context->is_suspended();
			}
			bool promise::is_context_busy(immediate_context* context)
			{
				return is_context_pending(context) || context->get_state() == execution::active;
			}
			int promise::promise_null = -1;
			int promise::promise_ud = 559;

			template <typename t>
			void decimal_custom_constructor_arithmetic(core::decimal* base, t value)
			{
				auto* vm = virtual_machine::get();
				uint32_t precision = vm ? (uint32_t)vm->get_library_property(library_features::decimal_default_precision) : 0;
				new(base) core::decimal(value);
				if (precision > 0)
					base->truncate(precision);
			}
			template <typename t>
			void decimal_custom_constructor_auto(core::decimal* base, const t& value)
			{
				auto* vm = virtual_machine::get();
				uint32_t precision = vm ? (uint32_t)vm->get_library_property(library_features::decimal_default_precision) : 0;
				new(base) core::decimal(value);
				if (precision > 0)
					base->truncate(precision);
			}
			void decimal_custom_constructor(core::decimal* base)
			{
				auto* vm = virtual_machine::get();
				uint32_t precision = vm ? (uint32_t)vm->get_library_property(library_features::decimal_default_precision) : 0;
				new(base) core::decimal();
				if (precision > 0)
					base->truncate(precision);
			}
			core::decimal decimal_negate(core::decimal& base)
			{
				return -base;
			}
			core::decimal& decimal_mul_eq(core::decimal& base, const core::decimal& v)
			{
				base *= v;
				return base;
			}
			core::decimal& decimal_div_eq(core::decimal& base, const core::decimal& v)
			{
				base /= v;
				return base;
			}
			core::decimal& decimal_add_eq(core::decimal& base, const core::decimal& v)
			{
				base += v;
				return base;
			}
			core::decimal& decimal_sub_eq(core::decimal& base, const core::decimal& v)
			{
				base -= v;
				return base;
			}
			core::decimal& decimal_fpp(core::decimal& base)
			{
				return ++base;
			}
			core::decimal& decimal_fmm(core::decimal& base)
			{
				return --base;
			}
			core::decimal& decimal_pp(core::decimal& base)
			{
				base++;
				return base;
			}
			core::decimal& decimal_mm(core::decimal& base)
			{
				base--;
				return base;
			}
			bool decimal_eq(core::decimal& base, const core::decimal& right)
			{
				return base == right;
			}
			int decimal_cmp(core::decimal& base, const core::decimal& right)
			{
				if (base == right)
					return 0;

				return base > right ? 1 : -1;
			}
			core::decimal decimal_add(const core::decimal& left, const core::decimal& right)
			{
				return left + right;
			}
			core::decimal decimal_sub(const core::decimal& left, const core::decimal& right)
			{
				return left - right;
			}
			core::decimal decimal_mul(const core::decimal& left, const core::decimal& right)
			{
				return left * right;
			}
			core::decimal decimal_div(const core::decimal& left, const core::decimal& right)
			{
				return left / right;
			}
			core::decimal decimal_per(const core::decimal& left, const core::decimal& right)
			{
				return left % right;
			}

			void uint128_default_construct(compute::uint128* base)
			{
				new(base) compute::uint128();
				memset(base, 0, sizeof(compute::uint128));
			}
			bool uint128_to_bool(compute::uint128& value)
			{
				return !value;
			}
			int8_t uint128_to_int8(compute::uint128& value)
			{
				return (int8_t)(uint8_t)value;
			}
			uint8_t uint128_to_uint8(compute::uint128& value)
			{
				return (uint8_t)value;
			}
			int16_t uint128_to_int16(compute::uint128& value)
			{
				return (int16_t)(uint16_t)value;
			}
			uint16_t uint128_to_uint16(compute::uint128& value)
			{
				return (uint16_t)value;
			}
			int32_t uint128_to_int32(compute::uint128& value)
			{
				return (int32_t)(uint32_t)value;
			}
			uint32_t uint128_to_uint32(compute::uint128& value)
			{
				return (uint32_t)value;
			}
			int64_t uint128_to_int64(compute::uint128& value)
			{
				return (int64_t)(uint64_t)value;
			}
			uint64_t uint128_to_uint64(compute::uint128& value)
			{
				return (uint64_t)value;
			}
			compute::uint128& uint128_mul_eq(compute::uint128& base, const compute::uint128& v)
			{
				base *= v;
				return base;
			}
			compute::uint128& uint128_div_eq(compute::uint128& base, const compute::uint128& v)
			{
				base /= v;
				return base;
			}
			compute::uint128& uint128_add_eq(compute::uint128& base, const compute::uint128& v)
			{
				base += v;
				return base;
			}
			compute::uint128& uint128_sub_eq(compute::uint128& base, const compute::uint128& v)
			{
				base -= v;
				return base;
			}
			compute::uint128& uint128_fpp(compute::uint128& base)
			{
				return ++base;
			}
			compute::uint128& uint128_fmm(compute::uint128& base)
			{
				return --base;
			}
			compute::uint128& uint128_pp(compute::uint128& base)
			{
				base++;
				return base;
			}
			compute::uint128& uint128_mm(compute::uint128& base)
			{
				base--;
				return base;
			}
			bool uint128_eq(compute::uint128& base, const compute::uint128& right)
			{
				return base == right;
			}
			int uint128_cmp(compute::uint128& base, const compute::uint128& right)
			{
				if (base == right)
					return 0;

				return base > right ? 1 : -1;
			}
			compute::uint128 uint128_add(const compute::uint128& left, const compute::uint128& right)
			{
				return left + right;
			}
			compute::uint128 uint128_sub(const compute::uint128& left, const compute::uint128& right)
			{
				return left - right;
			}
			compute::uint128 uint128_mul(const compute::uint128& left, const compute::uint128& right)
			{
				return left * right;
			}
			compute::uint128 uint128_div(const compute::uint128& left, const compute::uint128& right)
			{
				return left / right;
			}
			compute::uint128 uint128_per(const compute::uint128& left, const compute::uint128& right)
			{
				return left % right;
			}

			void uint256_default_construct(compute::uint256* base)
			{
				new(base) compute::uint256();
				memset(base, 0, sizeof(compute::uint256));
			}
			bool uint256_to_bool(compute::uint256& value)
			{
				return !value;
			}
			int8_t uint256_to_int8(compute::uint256& value)
			{
				return (int8_t)(uint8_t)value;
			}
			uint8_t uint256_to_uint8(compute::uint256& value)
			{
				return (uint8_t)value;
			}
			int16_t uint256_to_int16(compute::uint256& value)
			{
				return (int16_t)(uint16_t)value;
			}
			uint16_t uint256_to_uint16(compute::uint256& value)
			{
				return (uint16_t)value;
			}
			int32_t uint256_to_int32(compute::uint256& value)
			{
				return (int32_t)(uint32_t)value;
			}
			uint32_t uint256_to_uint32(compute::uint256& value)
			{
				return (uint32_t)value;
			}
			int64_t uint256_to_int64(compute::uint256& value)
			{
				return (int64_t)(uint64_t)value;
			}
			uint64_t uint256_to_uint64(compute::uint256& value)
			{
				return (uint64_t)value;
			}
			compute::uint256& uint256_mul_eq(compute::uint256& base, const compute::uint256& v)
			{
				base *= v;
				return base;
			}
			compute::uint256& uint256_div_eq(compute::uint256& base, const compute::uint256& v)
			{
				base /= v;
				return base;
			}
			compute::uint256& uint256_add_eq(compute::uint256& base, const compute::uint256& v)
			{
				base += v;
				return base;
			}
			compute::uint256& uint256_sub_eq(compute::uint256& base, const compute::uint256& v)
			{
				base -= v;
				return base;
			}
			compute::uint256& uint256_fpp(compute::uint256& base)
			{
				return ++base;
			}
			compute::uint256& uint256_fmm(compute::uint256& base)
			{
				return --base;
			}
			compute::uint256& uint256_pp(compute::uint256& base)
			{
				base++;
				return base;
			}
			compute::uint256& uint256_mm(compute::uint256& base)
			{
				base--;
				return base;
			}
			bool uint256_eq(compute::uint256& base, const compute::uint256& right)
			{
				return base == right;
			}
			int uint256_cmp(compute::uint256& base, const compute::uint256& right)
			{
				if (base == right)
					return 0;

				return base > right ? 1 : -1;
			}
			compute::uint256 uint256_add(const compute::uint256& left, const compute::uint256& right)
			{
				return left + right;
			}
			compute::uint256 uint256_sub(const compute::uint256& left, const compute::uint256& right)
			{
				return left - right;
			}
			compute::uint256 uint256_mul(const compute::uint256& left, const compute::uint256& right)
			{
				return left * right;
			}
			compute::uint256 uint256_div(const compute::uint256& left, const compute::uint256& right)
			{
				return left / right;
			}
			compute::uint256 uint256_per(const compute::uint256& left, const compute::uint256& right)
			{
				return left % right;
			}

			core::date_time& date_time_add_eq(core::date_time& base, const core::date_time& v)
			{
				base += v;
				return base;
			}
			core::date_time& date_time_sub_eq(core::date_time& base, const core::date_time& v)
			{
				base -= v;
				return base;
			}
			core::date_time& date_time_set(core::date_time& base, const core::date_time& v)
			{
				base = v;
				return base;
			}
			bool date_time_eq(core::date_time& base, const core::date_time& right)
			{
				return base == right;
			}
			int date_time_cmp(core::date_time& base, const core::date_time& right)
			{
				if (base == right)
					return 0;

				return base > right ? 1 : -1;
			}
			core::date_time date_time_add(const core::date_time& left, const core::date_time& right)
			{
				return left + right;
			}
			core::date_time date_time_sub(const core::date_time& left, const core::date_time& right)
			{
				return left - right;
			}

			size_t variant_get_size(core::variant& base)
			{
				return base.size();
			}
			bool variant_equals(core::variant& base, const core::variant& other)
			{
				return base == other;
			}
			bool variant_impl_cast(core::variant& base)
			{
				return (bool)base;
			}
			int8_t variant_to_int8(core::variant& base)
			{
				return (int8_t)base.get_integer();
			}
			int16_t variant_to_int16(core::variant& base)
			{
				return (int16_t)base.get_integer();
			}
			int32_t variant_to_int32(core::variant& base)
			{
				return (int32_t)base.get_integer();
			}
			int64_t variant_to_int64(core::variant& base)
			{
				return (int64_t)base.get_integer();
			}
			uint8_t variant_to_uint8(core::variant& base)
			{
				return (uint8_t)base.get_integer();
			}
			uint16_t variant_to_uint16(core::variant& base)
			{
				return (int16_t)base.get_integer();
			}
			uint32_t variant_to_uint32(core::variant& base)
			{
				return (uint32_t)base.get_integer();
			}
			uint64_t variant_to_uint64(core::variant& base)
			{
				return (uint64_t)base.get_integer();
			}

			int8_t schema_to_int8(core::schema* base)
			{
				return (int8_t)base->value.get_integer();
			}
			int16_t schema_to_int16(core::schema* base)
			{
				return (int16_t)base->value.get_integer();
			}
			int32_t schema_to_int32(core::schema* base)
			{
				return (int32_t)base->value.get_integer();
			}
			int64_t schema_to_int64(core::schema* base)
			{
				return (int64_t)base->value.get_integer();
			}
			uint8_t schema_to_uint8(core::schema* base)
			{
				return (uint8_t)base->value.get_integer();
			}
			uint16_t schema_to_uint16(core::schema* base)
			{
				return (int16_t)base->value.get_integer();
			}
			uint32_t schema_to_uint32(core::schema* base)
			{
				return (uint32_t)base->value.get_integer();
			}
			uint64_t schema_to_uint64(core::schema* base)
			{
				return (uint64_t)base->value.get_integer();
			}
			void schema_notify_all_references(core::schema* base, virtual_machine* engine, asITypeInfo* type)
			{
				engine->notify_of_new_object(base, type);
				for (auto* item : base->get_childs())
					schema_notify_all_references(item, engine, type);
			}
			core::schema* schema_init(core::schema* base)
			{
				return base;
			}
			core::schema* schema_construct_copy(core::schema* base)
			{
				virtual_machine* vm = virtual_machine::get();
				auto* init = base->copy();
				schema_notify_all_references(init, vm, vm->get_type_info_by_name(TYPENAME_SCHEMA).get_type_info());
				return init;
			}
			core::schema* schema_construct_buffer(unsigned char* buffer)
			{
#ifdef VI_ANGELSCRIPT
				if (!buffer)
					return nullptr;

				core::schema* result = core::var::set::object();
				virtual_machine* vm = virtual_machine::get();
				size_t length = *(asUINT*)buffer;
				buffer += 4;

				while (length--)
				{
					if (asPWORD(buffer) & 0x3)
						buffer += 4 - (asPWORD(buffer) & 0x3);

					core::string name = *(core::string*)buffer;
					buffer += sizeof(core::string);

					int type_id = *(int*)buffer;
					buffer += sizeof(int);

					void* ref = (void*)buffer;
					if (type_id >= (size_t)type_id::bool_t && type_id <= (size_t)type_id::double_t)
					{
						switch (type_id)
						{
							case (size_t)type_id::bool_t:
								result->set(name, core::var::boolean(*(bool*)ref));
								break;
							case (size_t)type_id::int8_t:
								result->set(name, core::var::integer(*(char*)ref));
								break;
							case (size_t)type_id::int16_t:
								result->set(name, core::var::integer(*(short*)ref));
								break;
							case (size_t)type_id::int32_t:
								result->set(name, core::var::integer(*(int*)ref));
								break;
							case (size_t)type_id::uint8_t:
								result->set(name, core::var::integer(*(unsigned char*)ref));
								break;
							case (size_t)type_id::uint16_t:
								result->set(name, core::var::integer(*(unsigned short*)ref));
								break;
							case (size_t)type_id::uint32_t:
								result->set(name, core::var::integer(*(uint32_t*)ref));
								break;
							case (size_t)type_id::int64_t:
							case (size_t)type_id::uint64_t:
								result->set(name, core::var::integer(*(int64_t*)ref));
								break;
							case (size_t)type_id::float_t:
								result->set(name, core::var::number(*(float*)ref));
								break;
							case (size_t)type_id::double_t:
								result->set(name, core::var::number(*(double*)ref));
								break;
						}
					}
					else
					{
						auto type = vm->get_type_info_by_id(type_id);
						if ((type_id & (size_t)type_id::mask_object_t) && !(type_id & (size_t)type_id::handle_t) && (type.is_valid() && type.flags() & (size_t)object_behaviours::ref))
							ref = *(void**)ref;

						if (type_id & (size_t)type_id::handle_t)
							ref = *(void**)ref;

						if (vm->is_nullable(type_id) || !ref)
						{
							result->set(name, core::var::null());
						}
						else if (type.is_valid() && type.get_name() == TYPENAME_SCHEMA)
						{
							core::schema* base = (core::schema*)ref;
							if (base->get_parent() != result)
								base->add_ref();

							result->set(name, base);
						}
						else if (type.is_valid() && type.get_name() == TYPENAME_STRING)
							result->set(name, core::var::string(*(core::string*)ref));
						else if (type.is_valid() && type.get_name() == TYPENAME_DECIMAL)
							result->set(name, core::var::decimal(*(core::decimal*)ref));
					}

					if (type_id & (size_t)type_id::mask_object_t)
					{
						auto type = vm->get_type_info_by_id(type_id);
						if (type.flags() & (size_t)object_behaviours::value)
							buffer += type.size();
						else
							buffer += sizeof(void*);
					}
					else if (type_id != 0)
						buffer += vm->get_size_of_primitive_type(type_id).or_else(0);
					else
						buffer += sizeof(void*);
				}

				return result;
#else
				return nullptr;
#endif
			}
			void schema_construct(asIScriptGeneric* genericf)
			{
				generic_context args = genericf;
				virtual_machine* vm = args.get_vm();
				unsigned char* buffer = (unsigned char*)args.get_arg_address(0);
				core::schema* init = schema_construct_buffer(buffer);
				schema_notify_all_references(init, vm, vm->get_type_info_by_name(TYPENAME_SCHEMA).get_type_info());
				*(core::schema**)args.get_address_of_return_location() = init;
			}
			core::schema* schema_get_index(core::schema* base, const std::string_view& name)
			{
				core::schema* result = base->fetch(name);
				if (result != nullptr)
					return result;

				return base->set(name, core::var::undefined());
			}
			core::schema* schema_get_index_offset(core::schema* base, size_t offset)
			{
				return base->get(offset);
			}
			core::schema* schema_set(core::schema* base, const std::string_view& name, core::schema* value)
			{
				if (value != nullptr && value->get_parent() != base)
					value->add_ref();

				return base->set(name, value);
			}
			core::schema* schema_push(core::schema* base, core::schema* value)
			{
				if (value != nullptr)
					value->add_ref();

				return base->push(value);
			}
			core::schema* schema_first(core::schema* base)
			{
				auto& childs = base->get_childs();
				return childs.empty() ? nullptr : childs.front();
			}
			core::schema* schema_last(core::schema* base)
			{
				auto& childs = base->get_childs();
				return childs.empty() ? nullptr : childs.back();
			}
			core::variant schema_first_var(core::schema* base)
			{
				auto& childs = base->get_childs();
				return childs.empty() ? core::var::undefined() : childs.front()->value;
			}
			core::variant schema_last_var(core::schema* base)
			{
				auto& childs = base->get_childs();
				return childs.empty() ? core::var::undefined() : childs.back()->value;
			}
			array* schema_get_collection(core::schema* base, const std::string_view& name, bool deep)
			{
				immediate_context* context = immediate_context::get();
				if (!context)
					return nullptr;

				virtual_machine* vm = context->get_vm();
				if (!vm)
					return nullptr;

				core::vector<core::schema*> nodes = base->fetch_collection(name, deep);
				for (auto& node : nodes)
					node->add_ref();

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return array::compose(type.get_type_info(), nodes);
			}
			array* schema_get_childs(core::schema* base)
			{
				immediate_context* context = immediate_context::get();
				if (!context)
					return nullptr;

				virtual_machine* vm = context->get_vm();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return array::compose(type.get_type_info(), base->get_childs());
			}
			array* schema_get_attributes(core::schema* base)
			{
				immediate_context* context = immediate_context::get();
				if (!context)
					return nullptr;

				virtual_machine* vm = context->get_vm();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_SCHEMA "@>@");
				return array::compose(type.get_type_info(), base->get_attributes());
			}
			dictionary* schema_get_names(core::schema* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				core::unordered_map<core::string, size_t> mapping = base->get_names();
				dictionary* map = dictionary::create(vm);

				for (auto& item : mapping)
				{
					int64_t index = (int64_t)item.second;
					map->set(item.first, &index, (int)type_id::int64_t);
				}

				return map;
			}
			size_t schema_get_size(core::schema* base)
			{
				return base->size();
			}
			core::string schema_to_json(core::schema* base)
			{
				core::string stream;
				core::schema::convert_to_json(base, [&stream](core::var_form, const std::string_view& buffer) { stream.append(buffer); });
				return stream;
			}
			core::string schema_to_xml(core::schema* base)
			{
				core::string stream;
				core::schema::convert_to_xml(base, [&stream](core::var_form, const std::string_view& buffer) { stream.append(buffer); });
				return stream;
			}
			core::string schema_to_string(core::schema* base)
			{
				switch (base->value.get_type())
				{
					case core::var_type::null:
					case core::var_type::undefined:
					case core::var_type::object:
					case core::var_type::array:
					case core::var_type::pointer:
						break;
					case core::var_type::string:
					case core::var_type::binary:
						return base->value.get_blob();
					case core::var_type::integer:
						return core::to_string(base->value.get_integer());
					case core::var_type::number:
						return core::to_string(base->value.get_number());
					case core::var_type::decimal:
						return base->value.get_decimal().to_string();
					case core::var_type::boolean:
						return base->value.get_boolean() ? "1" : "0";
				}

				return "";
			}
			core::string schema_to_binary(core::schema* base)
			{
				return base->value.get_blob();
			}
			int64_t schema_to_integer(core::schema* base)
			{
				return base->value.get_integer();
			}
			double schema_to_number(core::schema* base)
			{
				return base->value.get_number();
			}
			core::decimal schema_to_decimal(core::schema* base)
			{
				return base->value.get_decimal();
			}
			bool schema_to_boolean(core::schema* base)
			{
				return base->value.get_boolean();
			}
			core::schema* schema_from_json(const std::string_view& value)
			{
				return expects_wrapper::unwrap(core::schema::convert_from_json(value), (core::schema*)nullptr);
			}
			core::schema* schema_from_xml(const std::string_view& value)
			{
				return expects_wrapper::unwrap(core::schema::convert_from_xml(value), (core::schema*)nullptr);
			}
			core::schema* schema_import(const std::string_view& value)
			{
				auto data = core::os::file::read_as_string(value);
				if (!data)
					return expects_wrapper::unwrap_void(std::move(data)) ? nullptr : nullptr;

				auto output = core::schema::from_json(*data);
				if (output)
					return *output;

				output = core::schema::from_xml(value);
				if (output)
					return *output;

				return expects_wrapper::unwrap(core::schema::from_jsonb(value), (core::schema*)nullptr);
			}
			core::schema* schema_copy_assign1(core::schema* base, const core::variant& other)
			{
				base->value = other;
				return base;
			}
			core::schema* schema_copy_assign2(core::schema* base, core::schema* other)
			{
				base->clear();
				if (!other)
					return base;

				auto* copy = other->copy();
				base->join(copy, true);
				copy->release();
				return base;
			}
			bool schema_equals(core::schema* base, core::schema* other)
			{
				if (other != nullptr)
					return base->value == other->value;

				core::var_type type = base->value.get_type();
				return (type == core::var_type::null || type == core::var_type::undefined);
			}
#ifdef VI_BINDINGS
			core::expects_system<void> from_socket_poll(network::socket_poll poll)
			{
				switch (poll)
				{
					case network::socket_poll::reset:
						return core::expects_system<void>(core::system_exception("connection reset"));
					case network::socket_poll::timeout:
						return core::expects_system<void>(core::system_exception("connection timed out"));
					case network::socket_poll::cancel:
						return core::expects_system<void>(core::system_exception("connection aborted"));
					case network::socket_poll::finish:
					case network::socket_poll::finish_sync:
					case network::socket_poll::next:
					default:
						return core::expectation::met;
				}
			}
			template <typename t>
			core::string get_component_name(t* base)
			{
				return base->get_name();
			}
			template <typename t>
			void populate_component(ref_class& class_name)
			{
				class_name.set_method_extern("string get_name() const", &get_component_name<t>);
				class_name.set_method("uint64 get_id() const", &t::get_id);
			}
			core::variant_args to_variant_keys(core::schema* args)
			{
				core::variant_args keys;
				if (!args || args->empty())
					return keys;

				keys.reserve(args->size());
				for (auto* item : args->get_childs())
					keys[item->key] = item->value;

				return keys;
			}

			mutex::mutex() noexcept
			{
			}
			bool mutex::try_lock()
			{
				if (!base.try_lock())
					return false;

				immediate_context* context = immediate_context::get();
				if (context != nullptr)
					context->set_user_data((void*)(uintptr_t)((size_t)(uintptr_t)context->get_user_data(mutex_ud) + (size_t)1), mutex_ud);

				return true;
			}
			void mutex::lock()
			{
				immediate_context* context = immediate_context::get();
				if (!context)
					return base.lock();

				virtual_machine* vm = context->get_vm();
				context->set_user_data((void*)(uintptr_t)((size_t)(uintptr_t)context->get_user_data(mutex_ud) + (size_t)1), mutex_ud);
				if (!vm->has_debugger())
					return base.lock();

				while (!try_lock())
					vm->trigger_debugger(context, 50);
			}
			void mutex::unlock()
			{
				immediate_context* context = immediate_context::get();
				if (context != nullptr)
				{
					size_t size = (size_t)(uintptr_t)context->get_user_data(mutex_ud);
					if (!size)
						bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_MUTEXNOTOWNED));
					else
						context->set_user_data((void*)(uintptr_t)(size - 1), mutex_ud);
				}
				base.unlock();
			}
			mutex* mutex::factory()
			{
				mutex* result = new mutex();
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}
			bool mutex::is_any_locked(immediate_context* context)
			{
				VI_ASSERT(context != nullptr, "context should be set");
				return (size_t)(uintptr_t)context->get_user_data(mutex_ud) != (size_t)0;
			}
			int mutex::mutex_ud = 538;

			thread::thread(virtual_machine* engine, asIScriptFunction* callback) noexcept : vm(engine), loop(nullptr), function(callback, vm ? vm->request_context() : nullptr)
			{
				VI_ASSERT(vm != nullptr, "virtual matchine should be set");
				engine->notify_of_new_object(this, engine->get_type_info_by_name(TYPENAME_THREAD));
			}
			thread::~thread() noexcept
			{
				release_references(nullptr);
			}
			void thread::execution_loop()
			{
				thread* base = this;
				if (!function.is_valid())
					return release();

				function_delegate copy = function;
				function.context->set_user_data(this, thread_ud);

				loop = new event_loop();
				loop->listen(function.context);
				loop->enqueue(std::move(copy), [base](immediate_context* context)
				{
					context->set_arg_object(0, base);
				}, nullptr);

				event_loop::set(loop);
				while (loop->poll(function.context, 1000))
					loop->dequeue(vm);

				core::umutex<std::recursive_mutex> unique(mutex);
				raise = exception::pointer(function.context);
				vm->return_context(function.context);
				function.release();
				core::memory::release(loop);
				release();
				virtual_machine::cleanup_this_thread();
			}
			bool thread::suspend()
			{
				if (!is_active())
					return false;

				core::umutex<std::recursive_mutex> unique(mutex);
				return function.context->is_suspended() || !!function.context->suspend();
			}
			bool thread::resume()
			{
				core::umutex<std::recursive_mutex> unique(mutex);
				loop->enqueue(function.context);
				VI_DEBUG("vm resume thread at %s", core::os::process::get_thread_id(procedure.get_id()).c_str());
				return true;
			}
			void thread::enum_references(asIScriptEngine* engine)
			{
				for (int i = 0; i < 2; i++)
				{
					for (auto any : pipe[i].queue)
						function_factory::gc_enum_callback(engine, any);
				}
				function_factory::gc_enum_callback(engine, &function);
			}
			void thread::release_references(asIScriptEngine*)
			{
				join();
				for (int i = 0; i < 2; i++)
				{
					auto& source = pipe[i];
					core::umutex<std::mutex> unique(source.mutex);
					for (auto any : source.queue)
						core::memory::release(any);
					source.queue.clear();
				}

				core::umutex<std::recursive_mutex> unique(mutex);
				if (function.context != nullptr)
					vm->return_context(function.context);
				function.release();
			}
			int thread::join(uint64_t timeout)
			{
				if (std::this_thread::get_id() == procedure.get_id())
					return -1;

				core::umutex<std::recursive_mutex> unique(mutex);
				if (!procedure.joinable())
					return -1;

				VI_DEBUG("vm join thread %s", core::os::process::get_thread_id(procedure.get_id()).c_str());
				unique.negate();
				if (vm != nullptr && vm->has_debugger())
				{
					while (procedure.joinable() && is_active())
						vm->trigger_debugger(nullptr, 50);
				}
				procedure.join();
				unique.negate();
				if (!raise.empty())
					exception::throw_ptr(raise);
				return 1;
			}
			int thread::join()
			{
				if (std::this_thread::get_id() == procedure.get_id())
					return -1;

				while (true)
				{
					int r = join(1000);
					if (r == -1 || r == 1)
						return r;
				}

				return 0;
			}
			void thread::push(void* ref, int type_id)
			{
				any* next = new any(ref, type_id, virtual_machine::get());
				if (!next)
					return bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				auto& source = pipe[(get_thread() == this ? 1 : 0)];
				core::umutex<std::mutex> unique(source.mutex);
				source.queue.push_back(next);
				source.CV.notify_one();
			}
			bool thread::pop(void* ref, int type_id)
			{
				bool resolved = false;
				while (!resolved)
					resolved = pop(ref, type_id, 1000);

				return true;
			}
			bool thread::pop(void* ref, int type_id, uint64_t timeout)
			{
				auto& source = pipe[(get_thread() == this ? 0 : 1)];
				std::unique_lock<std::mutex> guard(source.mutex);
				if (!source.CV.wait_for(guard, std::chrono::milliseconds(timeout), [&]
				{
					return source.queue.size() != 0;
				}))
					return false;

				any* result = source.queue.front();
				if (!result->retrieve(ref, type_id))
					return false;

				source.queue.erase(source.queue.begin());
				core::memory::release(result);
				return true;
			}
			bool thread::is_active()
			{
				core::umutex<std::recursive_mutex> unique(mutex);
				return loop != nullptr && function.context != nullptr && function.context->get_state() == execution::active;
			}
			bool thread::start()
			{
				core::umutex<std::recursive_mutex> unique(mutex);
				if (!function.is_valid())
					return false;

				join();
				add_ref();

				procedure = std::thread(&thread::execution_loop, this);
				VI_DEBUG("vm spawn thread %s", core::os::process::get_thread_id(procedure.get_id()).c_str());
				return true;
			}
			thread* thread::create(asIScriptFunction* callback)
			{
				thread* result = new thread(virtual_machine::get(), callback);
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));
				return result;
			}
			thread* thread::get_thread()
			{
				auto* context = immediate_context::get();
				if (!context)
					return nullptr;

				return static_cast<thread*>(context->get_user_data(thread_ud));
			}
			core::string thread::get_id() const
			{
				return core::os::process::get_thread_id(procedure.get_id());
			}
			void thread::thread_sleep(uint64_t timeout)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
			}
			bool thread::thread_suspend()
			{
				immediate_context* context = immediate_context::get();
				if (!context)
					return false;

				if (context->is_suspended())
					return true;

				return !!context->suspend();
			}
			core::string thread::get_thread_id()
			{
				return core::os::process::get_thread_id(std::this_thread::get_id());
			}
			int thread::thread_ud = 431;

			char_buffer::char_buffer() noexcept : char_buffer(nullptr)
			{
			}
			char_buffer::char_buffer(size_t new_size) noexcept : char_buffer(nullptr)
			{
				if (new_size > 0)
					allocate(new_size);
			}
			char_buffer::char_buffer(char* pointer) noexcept : buffer(pointer), length(0)
			{
			}
			char_buffer::~char_buffer() noexcept
			{
				deallocate();
			}
			bool char_buffer::allocate(size_t new_size)
			{
				deallocate();
				if (!new_size)
					return false;

				buffer = core::memory::allocate<char>(new_size);
				if (!buffer)
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));
					return false;
				}

				memset(buffer, 0, new_size);
				length = new_size;
				return true;
			}
			void char_buffer::deallocate()
			{
				if (length > 0)
					core::memory::deallocate(buffer);

				buffer = nullptr;
				length = 0;
			}
			bool char_buffer::set_int8(size_t offset, int8_t value, size_t set_size)
			{
				if (!buffer || offset + set_size > length)
					return false;

				memset(buffer + offset, (int32_t)value, set_size);
				return true;
			}
			bool char_buffer::set_uint8(size_t offset, uint8_t value, size_t set_size)
			{
				if (!buffer || offset + set_size > length)
					return false;

				memset(buffer + offset, (int32_t)value, set_size);
				return true;
			}
			bool char_buffer::store_bytes(size_t offset, const std::string_view& value)
			{
				return store(offset, value.data(), sizeof(char) * value.size());
			}
			bool char_buffer::store_int8(size_t offset, int8_t value)
			{
				return store(offset, (const char*)&value, sizeof(value));
			}
			bool char_buffer::store_uint8(size_t offset, uint8_t value)
			{
				return store(offset, (const char*)&value, sizeof(value));
			}
			bool char_buffer::store_int16(size_t offset, int16_t value)
			{
				return store(offset, (const char*)&value, sizeof(value));
			}
			bool char_buffer::store_uint16(size_t offset, uint16_t value)
			{
				return store(offset, (const char*)&value, sizeof(value));
			}
			bool char_buffer::store_int32(size_t offset, int32_t value)
			{
				return store(offset, (const char*)&value, sizeof(value));
			}
			bool char_buffer::store_uint32(size_t offset, uint32_t value)
			{
				return store(offset, (const char*)&value, sizeof(value));
			}
			bool char_buffer::store_int64(size_t offset, int64_t value)
			{
				return store(offset, (const char*)&value, sizeof(value));
			}
			bool char_buffer::store_uint64(size_t offset, uint64_t value)
			{
				return store(offset, (const char*)&value, sizeof(value));
			}
			bool char_buffer::store_float(size_t offset, float value)
			{
				return store(offset, (const char*)&value, sizeof(value));
			}
			bool char_buffer::store_double(size_t offset, double value)
			{
				return store(offset, (const char*)&value, sizeof(value));
			}
			bool char_buffer::interpret(size_t offset, core::string& value, size_t max_size) const
			{
				size_t data_size = 0;
				if (!buffer || offset + sizeof(&data_size) > length)
					return false;

				char* data = buffer + offset;
				char* next = data;
				while (*(next++) != '\0')
				{
					if (++data_size > max_size)
						return false;
				}

				value.assign(data, data_size);
				return true;
			}
			bool char_buffer::load_bytes(size_t offset, core::string& value, size_t value_size) const
			{
				value.resize(value_size);
				if (load(offset, (char*)value.c_str(), sizeof(char) * value_size))
					return true;

				value.clear();
				return false;
			}
			bool char_buffer::load_int8(size_t offset, int8_t& value) const
			{
				return load(offset, (char*)&value, sizeof(value));
			}
			bool char_buffer::load_uint8(size_t offset, uint8_t& value) const
			{
				return load(offset, (char*)&value, sizeof(value));
			}
			bool char_buffer::load_int16(size_t offset, int16_t& value) const
			{
				return load(offset, (char*)&value, sizeof(value));
			}
			bool char_buffer::load_uint16(size_t offset, uint16_t& value) const
			{
				return load(offset, (char*)&value, sizeof(value));
			}
			bool char_buffer::load_int32(size_t offset, int32_t& value) const
			{
				return load(offset, (char*)&value, sizeof(value));
			}
			bool char_buffer::load_uint32(size_t offset, uint32_t& value) const
			{
				return load(offset, (char*)&value, sizeof(value));
			}
			bool char_buffer::load_int64(size_t offset, int64_t& value) const
			{
				return load(offset, (char*)&value, sizeof(value));
			}
			bool char_buffer::load_uint64(size_t offset, uint64_t& value) const
			{
				return load(offset, (char*)&value, sizeof(value));
			}
			bool char_buffer::load_float(size_t offset, float& value) const
			{
				return load(offset, (char*)&value, sizeof(value));
			}
			bool char_buffer::load_double(size_t offset, double& value) const
			{
				return load(offset, (char*)&value, sizeof(value));
			}
			bool char_buffer::store(size_t offset, const char* data, size_t data_size)
			{
				if (!buffer || offset + data_size > length)
					return false;

				memcpy(buffer + offset, data, data_size);
				return true;
			}
			bool char_buffer::load(size_t offset, char* data, size_t data_size) const
			{
				if (!buffer || offset + data_size > length)
					return false;

				memcpy(data, buffer + offset, data_size);
				return true;
			}
			void* char_buffer::get_pointer(size_t offset) const
			{
				if (length > 0 && offset >= length)
					return nullptr;

				return buffer + offset;
			}
			bool char_buffer::exists(size_t offset) const
			{
				return !length || offset < length;
			}
			bool char_buffer::empty() const
			{
				return !buffer;
			}
			size_t char_buffer::size() const
			{
				return length;
			}
			core::string char_buffer::to_string(size_t max_size) const
			{
				core::string data;
				if (!buffer)
					return data;

				if (length > 0)
				{
					data.assign(buffer, length > max_size ? max_size : length);
					return data;
				}

				size_t data_size = 0;
				char* next = buffer;
				while (*(next++) != '\0')
				{
					if (++data_size > max_size)
					{
						data.assign(buffer, data_size - 1);
						return data;
					}
				}

				data.assign(buffer, data_size);
				return data;
			}
			char_buffer* char_buffer::create()
			{
				char_buffer* result = new char_buffer();
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}
			char_buffer* char_buffer::create(size_t size)
			{
				char_buffer* result = new char_buffer(size);
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}
			char_buffer* char_buffer::create(char* pointer)
			{
				char_buffer* result = new char_buffer(pointer);
				if (!result)
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFMEMORY));

				return result;
			}

			complex::complex() noexcept
			{
				r = 0;
				i = 0;
			}
			complex::complex(const complex& other) noexcept
			{
				r = other.r;
				i = other.i;
			}
			complex::complex(float _R, float _I) noexcept
			{
				r = _R;
				i = _I;
			}
			bool complex::operator==(const complex& other) const
			{
				return (r == other.r) && (i == other.i);
			}
			bool complex::operator!=(const complex& other) const
			{
				return !(*this == other);
			}
			complex& complex::operator=(const complex& other) noexcept
			{
				r = other.r;
				i = other.i;
				return *this;
			}
			complex& complex::operator+=(const complex& other)
			{
				r += other.r;
				i += other.i;
				return *this;
			}
			complex& complex::operator-=(const complex& other)
			{
				r -= other.r;
				i -= other.i;
				return *this;
			}
			complex& complex::operator*=(const complex& other)
			{
				*this = *this * other;
				return *this;
			}
			complex& complex::operator/=(const complex& other)
			{
				*this = *this / other;
				return *this;
			}
			float complex::squared_length() const
			{
				return r * r + i * i;
			}
			float complex::length() const
			{
				return sqrtf(squared_length());
			}
			complex complex::operator+(const complex& other) const
			{
				return complex(r + other.r, i + other.i);
			}
			complex complex::operator-(const complex& other) const
			{
				return complex(r - other.r, i + other.i);
			}
			complex complex::operator*(const complex& other) const
			{
				return complex(r * other.r - i * other.i, r * other.i + i * other.r);
			}
			complex complex::operator/(const complex& other) const
			{
				float sq = other.squared_length();
				if (sq == 0)
					return complex(0, 0);

				return complex((r * other.r + i * other.i) / sq, (i * other.r - r * other.i) / sq);
			}
			complex complex::get_ri() const
			{
				return *this;
			}
			complex complex::get_ir() const
			{
				return complex(r, i);
			}
			void complex::set_ri(const complex& other)
			{
				*this = other;
			}
			void complex::set_ir(const complex& other)
			{
				r = other.i;
				i = other.r;
			}
			void complex::default_constructor(complex* base)
			{
				new(base) complex();
			}
			void complex::copy_constructor(complex* base, const complex& other)
			{
				new(base) complex(other);
			}
			void complex::conv_constructor(complex* base, float result)
			{
				new(base) complex(result);
			}
			void complex::init_constructor(complex* base, float result, float _I)
			{
				new(base) complex(result, _I);
			}
			void complex::list_constructor(complex* base, float* init_list)
			{
				new(base) complex(init_list[0], init_list[1]);
			}

			void console_trace(core::console* base, uint32_t frames)
			{
				base->write_line(core::error_handling::get_stack_trace(1, (size_t)frames));
			}

			application::application(desc& i, void* new_object, int new_type_id) noexcept : layer::application(&i), processed_events(0), initiator_type(nullptr), initiator_object(new_object)
			{
				virtual_machine* current_vm = virtual_machine::get();
				if (current_vm != nullptr && initiator_object != nullptr && ((new_type_id & (int)type_id::handle_t) || (new_type_id & (int)type_id::mask_object_t)))
				{
					initiator_type = current_vm->get_type_info_by_id(new_type_id).get_type_info();
					if (new_type_id & (int)type_id::handle_t)
						initiator_object = *(void**)initiator_object;
					current_vm->add_ref_object(initiator_object, initiator_type);
				}
				else if (current_vm != nullptr && initiator_object != nullptr)
				{
					if (new_type_id != (int)type_id::void_t)
						exception::throw_ptr(exception::pointer(EXCEPTION_INVALIDINITIATOR));
					initiator_object = nullptr;
				}

				if (i.usage & (size_t)layer::USE_SCRIPTING)
					vm = current_vm;

				if (i.usage & (size_t)layer::USE_PROCESSING)
				{
					content = new layer::content_manager();
					content->add_processor(new layer::processors::asset_processor(content), VI_HASH(TYPENAME_ASSETFILE));
					content->add_processor(new layer::processors::schema_processor(content), VI_HASH(TYPENAME_SCHEMA));
					content->add_processor(new layer::processors::server_processor(content), VI_HASH(TYPENAME_HTTPSERVER));
				}
			}
			application::~application() noexcept
			{
				virtual_machine* current_vm = vm ? vm : virtual_machine::get();
				if (current_vm != nullptr && initiator_object != nullptr && initiator_type != nullptr)
					current_vm->release_object(initiator_object, initiator_type);

				on_dispatch.release();
				on_publish.release();
				on_composition.release();
				on_script_hook.release();
				on_initialize.release();
				on_startup.release();
				on_shutdown.release();
				initiator_object = nullptr;
				initiator_type = nullptr;
				vm = nullptr;
			}
			void application::set_on_dispatch(asIScriptFunction* callback)
			{
				on_dispatch = function_delegate(callback);
			}
			void application::set_on_publish(asIScriptFunction* callback)
			{
				on_publish = function_delegate(callback);
			}
			void application::set_on_composition(asIScriptFunction* callback)
			{
				on_composition = function_delegate(callback);
			}
			void application::set_on_script_hook(asIScriptFunction* callback)
			{
				on_script_hook = function_delegate(callback);
			}
			void application::set_on_initialize(asIScriptFunction* callback)
			{
				on_initialize = function_delegate(callback);
			}
			void application::set_on_startup(asIScriptFunction* callback)
			{
				on_startup = function_delegate(callback);
			}
			void application::set_on_shutdown(asIScriptFunction* callback)
			{
				on_shutdown = function_delegate(callback);
			}
			void application::script_hook()
			{
				if (!on_script_hook.is_valid())
					return;

				auto* context = immediate_context::get();
				VI_ASSERT(context != nullptr, "application method cannot be called outside of script context");
				context->execute_subcall(on_script_hook.callable(), nullptr);
			}
			void application::composition()
			{
				if (!on_composition.is_valid())
					return;

				auto* context = immediate_context::get();
				VI_ASSERT(context != nullptr, "application method cannot be called outside of script context");
				context->execute_subcall(on_composition.callable(), nullptr);
			}
			void application::dispatch(core::timer* time)
			{
				auto* loop = event_loop::get();
				if (loop != nullptr)
					processed_events = loop->dequeue(vm);
				else
					processed_events = 0;

				if (on_dispatch.is_valid())
				{
					auto* context = immediate_context::get();
					VI_ASSERT(context != nullptr, "application method cannot be called outside of script context");
					context->execute_subcall(on_dispatch.callable(), [time](immediate_context* context)
					{
						context->set_arg_object(0, (void*)time);
					});
				}
			}
			void application::publish(core::timer* time)
			{
				if (!on_publish.is_valid())
					return;

				auto* context = immediate_context::get();
				VI_ASSERT(context != nullptr, "application method cannot be called outside of script context");
				context->execute_subcall(on_publish.callable(), [time](immediate_context* context)
				{
					context->set_arg_object(0, (void*)time);
				});
			}
			void application::initialize()
			{
				if (!on_initialize.is_valid())
					return;

				auto* context = immediate_context::get();
				VI_ASSERT(context != nullptr, "application method cannot be called outside of script context");
				context->execute_subcall(on_initialize.callable(), nullptr);
			}
			core::promise<void> application::startup()
			{
				if (!on_startup.is_valid())
					return core::promise<void>::null();

				VI_ASSERT(immediate_context::get() != nullptr, "application method cannot be called outside of script context");
				auto future = on_startup(nullptr);
				if (!future.is_pending())
					return core::promise<void>::null();

				return future.then(std::function<void(expects_vm<execution>&&)>(nullptr));
			}
			core::promise<void> application::shutdown()
			{
				if (!on_shutdown.is_valid())
					return core::promise<void>::null();

				VI_ASSERT(immediate_context::get() != nullptr, "application method cannot be called outside of script context");
				auto future = on_shutdown(nullptr);
				if (!future.is_pending())
					return core::promise<void>::null();

				return future.then(std::function<void(expects_vm<execution>&&)>(nullptr));
			}
			size_t application::get_processed_events() const
			{
				return processed_events;
			}
			bool application::has_processed_events() const
			{
				return processed_events > 0;
			}
			bool application::retrieve_initiator_object(void* ref_pointer, int ref_type_id) const
			{
				virtual_machine* current_vm = vm ? vm : virtual_machine::get();
				if (!initiator_object || !initiator_type || !ref_pointer || !current_vm)
					return false;

				if (ref_type_id & (size_t)type_id::handle_t)
				{
					current_vm->ref_cast_object(initiator_object, initiator_type, current_vm->get_type_info_by_id(ref_type_id), reinterpret_cast<void**>(ref_pointer));
#ifdef VI_ANGELSCRIPT
					if (*(asPWORD*)ref_pointer == 0)
						return false;
#endif
					return true;
				}
				else if (ref_type_id & (size_t)type_id::mask_object_t)
				{
					auto ref_type_info = current_vm->get_type_info_by_id(ref_type_id);
					if (initiator_type == ref_type_info.get_type_info())
					{
						current_vm->assign_object(ref_pointer, initiator_object, initiator_type);
						return true;
					}
				}

				return false;
			}
			void* application::get_initiator_object() const
			{
				return initiator_object;
			}
			bool application::wants_restart(int exit_code)
			{
				return exit_code == layer::EXIT_RESTART;
			}

			bool stream_open(core::stream* base, const std::string_view& path, core::file_mode mode)
			{
				return expects_wrapper::unwrap_void(base->open(path, mode));
			}
			core::string stream_read(core::stream* base, size_t size)
			{
				core::string result;
				result.resize(size);

				size_t count = expects_wrapper::unwrap(base->read((uint8_t*)result.data(), size), (size_t)0);
				if (count < size)
					result.resize(count);

				return result;
			}
			core::string stream_read_line(core::stream* base, size_t size)
			{
				core::string result;
				result.resize(size);

				size_t count = expects_wrapper::unwrap(base->read_line((char*)result.data(), size), (size_t)0);
				if (count < size)
					result.resize(count);

				return result;
			}
			size_t stream_write(core::stream* base, const std::string_view& data)
			{
				return expects_wrapper::unwrap(base->write((uint8_t*)data.data(), data.size()), (size_t)0);
			}

			core::task_id schedule_set_interval(core::schedule* base, uint64_t mills, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
					return core::INVALID_TASK_ID;

				return base->set_interval(mills, [delegatef]() mutable { delegatef(nullptr); });
			}
			core::task_id schedule_set_timeout(core::schedule* base, uint64_t mills, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
					return core::INVALID_TASK_ID;

				return base->set_timeout(mills, [delegatef]() mutable { delegatef(nullptr); });
			}
			bool schedule_set_immediate(core::schedule* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
					return false;

				return base->set_task([delegatef]() mutable { delegatef(nullptr); });
			}
			bool schedule_spawn(core::schedule* base, asIScriptFunction* callback)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return false;

				immediate_context* context = vm->request_context();
				if (!context)
					return false;

				function_delegate delegatef(callback, context);
				if (!delegatef.is_valid())
					return false;

				return base->set_task([delegatef, context]() mutable
				{
					delegatef(nullptr, [context](immediate_context*)
					{
						context->get_vm()->return_context(context);
					});
				});
			}

			dictionary* inline_args_get_args(core::inline_args& base)
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_DICTIONARY "@");
				return dictionary::compose<core::string>(type.get_type_id(), base.args);
			}
			array* inline_args_get_params(core::inline_args& base)
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose<core::string>(type.get_type_info(), base.params);
			}

			array* os_directory_scan(const std::string_view& path)
			{
				core::vector<std::pair<core::string, core::file_entry>> entries;
				auto status = core::os::directory::scan(path, entries);
				if (!status)
					return expects_wrapper::unwrap_void(std::move(status)) ? nullptr : nullptr;

				core::vector<file_link> results;
				results.reserve(entries.size());
				for (auto& item : entries)
				{
					file_link next;
					(*(core::file_entry*)&next) = item.second;
					next.path = std::move(item.first);
					results.emplace_back(std::move(next));
				}

				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_FILELINK ">@");
				return array::compose<file_link>(type.get_type_info(), results);
			}
			array* os_directory_get_mounts()
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose<core::string>(type.get_type_info(), core::os::directory::get_mounts());
			}
			bool os_directory_create(const std::string_view& path)
			{
				return expects_wrapper::unwrap_void(core::os::directory::create(path));
			}
			bool os_directory_remove(const std::string_view& path)
			{
				return expects_wrapper::unwrap_void(core::os::directory::remove(path));
			}
			bool os_directory_is_exists(const std::string_view& path)
			{
				return core::os::directory::is_exists(path);
			}
			bool os_directory_is_empty(const std::string_view& path)
			{
				return core::os::directory::is_empty(path);
			}
			bool os_directory_set_working(const std::string_view& path)
			{
				return expects_wrapper::unwrap_void(core::os::directory::set_working(path));
			}
			bool os_directory_patch(const std::string_view& path)
			{
				return expects_wrapper::unwrap_void(core::os::directory::patch(path));
			}

			bool os_file_state(const std::string_view& path, core::file_entry& data)
			{
				return expects_wrapper::unwrap_void(core::os::file::get_state(path, &data));
			}
			bool os_file_move(const std::string_view& from, const std::string_view& to)
			{
				return expects_wrapper::unwrap_void(core::os::file::move(from, to));
			}
			bool os_file_copy(const std::string_view& from, const std::string_view& to)
			{
				return expects_wrapper::unwrap_void(core::os::file::copy(from, to));
			}
			bool os_file_remove(const std::string_view& path)
			{
				return expects_wrapper::unwrap_void(core::os::file::remove(path));
			}
			bool os_file_is_exists(const std::string_view& path)
			{
				return core::os::file::is_exists(path);
			}
			bool os_file_write(const std::string_view& path, const std::string_view& data)
			{
				return expects_wrapper::unwrap_void(core::os::file::write(path, (uint8_t*)data.data(), data.size()));
			}
			size_t os_file_join(const std::string_view& from, array* paths)
			{
				return expects_wrapper::unwrap(core::os::file::join(from, array::decompose<core::string>(paths)), (size_t)0);
			}
			core::file_state os_file_get_properties(const std::string_view& path)
			{
				return expects_wrapper::unwrap(core::os::file::get_properties(path), core::file_state());
			}
			core::stream* os_file_open_join(const std::string_view& from, array* paths)
			{
				return expects_wrapper::unwrap(core::os::file::open_join(from, array::decompose<core::string>(paths)), (core::stream*)nullptr);
			}
			core::stream* os_file_open_archive(const std::string_view& path, size_t unarchived_max_size)
			{
				return expects_wrapper::unwrap(core::os::file::open_archive(path, unarchived_max_size), (core::stream*)nullptr);
			}
			core::stream* os_file_open(const std::string_view& path, core::file_mode mode, bool async)
			{
				return expects_wrapper::unwrap(core::os::file::open(path, mode, async), (core::stream*)nullptr);
			}
			array* os_file_read_as_array(const std::string_view& path)
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				auto data = core::os::file::read_as_array(path);
				if (!data)
					return expects_wrapper::unwrap_void(std::move(data)) ? nullptr : nullptr;

				return array::compose<core::string>(type.get_type_info(), *data);
			}

			bool os_path_is_remote(const std::string_view& path)
			{
				return core::os::path::is_remote(path);
			}
			bool os_path_is_relative(const std::string_view& path)
			{
				return core::os::path::is_relative(path);
			}
			bool os_path_is_absolute(const std::string_view& path)
			{
				return core::os::path::is_absolute(path);
			}
			core::string os_path_resolve1(const std::string_view& path)
			{
				return expects_wrapper::unwrap(core::os::path::resolve(path), core::string());
			}
			core::string os_path_resolve_directory1(const std::string_view& path)
			{
				return expects_wrapper::unwrap(core::os::path::resolve_directory(path), core::string());
			}
			core::string os_path_resolve2(const std::string_view& path, const std::string_view& directory, bool even_if_exists)
			{
				return expects_wrapper::unwrap(core::os::path::resolve(path, directory, even_if_exists), core::string());
			}
			core::string os_path_resolve_directory2(const std::string_view& path, const std::string_view& directory, bool even_if_exists)
			{
				return expects_wrapper::unwrap(core::os::path::resolve_directory(path, directory, even_if_exists), core::string());
			}
			core::string os_path_get_directory(const std::string_view& path, size_t level)
			{
				return core::os::path::get_directory(path, level);
			}
			core::string os_path_get_filename(const std::string_view& path)
			{
				return core::string(core::os::path::get_filename(path));
			}
			core::string os_path_get_extension(const std::string_view& path)
			{
				return core::string(core::os::path::get_extension(path));
			}

			int os_process_execute(const std::string_view& command, core::file_mode mode, asIScriptFunction* callback)
			{
				immediate_context* context = immediate_context::get();
				function_delegate delegatef(callback);
				auto exit_code = core::os::process::execute(command, mode, [context, &delegatef](const std::string_view& value) -> bool
				{
					if (!context || !delegatef.is_valid())
						return true;

					core::string copy = core::string(value); bool next = true;
					context->execute_subcall(delegatef.callable(), [&copy](immediate_context* context)
					{
						context->set_arg_object(0, (void*)&copy);
					}, [&next](immediate_context* context)
					{
						next = (bool)context->get_return_byte();
					});
					return next;
				});
				return expects_wrapper::unwrap(std::move(exit_code), -1, context);
			}
			core::inline_args os_process_parse_args(array* args_array, size_t opts, array* flags_array)
			{
				core::unordered_set<core::string> flags;
				core::vector<core::string> inline_flags = array::decompose<core::string>(args_array);
				flags.reserve(inline_flags.size());

				core::vector<core::string> inline_args = array::decompose<core::string>(args_array);
				core::vector<char*> args;
				args.reserve(inline_args.size());

				for (auto& item : inline_flags)
					flags.insert(item);

				for (auto& item : inline_args)
					args.push_back((char*)item.c_str());

				return core::os::process::parse_args((int)args.size(), args.data(), opts, flags);
			}

			void* os_symbol_load(const std::string_view& path)
			{
				return expects_wrapper::unwrap(core::os::symbol::load(path), (void*)nullptr);
			}
			void* os_symbol_load_function(void* lib_handle, const std::string_view& path)
			{
				return expects_wrapper::unwrap(core::os::symbol::load_function(lib_handle, path), (void*)nullptr);
			}
			bool os_symbol_unload(void* handle)
			{
				return expects_wrapper::unwrap_void(core::os::symbol::unload(handle));
			}

			core::string regex_match_target(compute::regex_match& base)
			{
				return base.pointer ? base.pointer : core::string();
			}

			array* regex_result_get(compute::regex_result& base)
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_REGEXMATCH ">@");
				return array::compose<compute::regex_match>(type.get_type_info(), base.get());
			}
			array* regex_result_to_array(compute::regex_result& base)
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose<core::string>(type.get_type_info(), base.to_array());
			}

			compute::regex_result regex_source_match(compute::regex_source& base, const std::string_view& text)
			{
				compute::regex_result result;
				compute::regex::match(&base, result, text);
				return result;
			}
			core::string regex_source_replace(compute::regex_source& base, const std::string_view& text, const std::string_view& to)
			{
				core::string copy = core::string(text);
				compute::regex_result result;
				compute::regex::replace(&base, to, copy);
				return copy;
			}

			void web_token_set_audience(compute::web_token* base, array* data)
			{
				base->set_audience(array::decompose<core::string>(data));
			}

			bool crypto_ec_scalar_is_on_curve(compute::proofs::curve curve, const compute::secret_box& scalar)
			{
				return expects_wrapper::unwrap_void(compute::crypto::ec_scalar_is_on_curve(curve, scalar));
			}
			bool crypto_ec_point_is_on_curve(compute::proofs::curve curve, const std::string_view& point)
			{
				return expects_wrapper::unwrap_void(compute::crypto::ec_point_is_on_curve(curve, point));
			}
			bool crypto_verify(compute::digest type, compute::proof key_type, const std::string_view& data, const std::string_view& signature, const std::string_view& key)
			{
				return compute::crypto::verify(type, key_type, data, signature, key).or_else(false);
			}
			bool crypto_ec_verify(compute::digest type, compute::proofs::curve curve, const std::string_view& data, const std::string_view& signature, const std::string_view& key)
			{
				return compute::crypto::ec_verify(type, curve, data, signature, key).or_else(false);
			}
			size_t crypto_file_encrypt(compute::cipher type, core::stream* from, core::stream* to, const compute::secret_box& key, const compute::secret_box& salt, asIScriptFunction* transform, size_t read_interval, int complexity)
			{
				core::string intermediate;
				immediate_context* context = immediate_context::get();
				function_delegate delegatef(transform);
				auto callback = [context, &delegatef, &intermediate](uint8_t** buffer, size_t* size) -> void
				{
					if (!context || !delegatef.is_valid())
						return;

					std::string_view input((char*)*buffer, *size);
					context->execute_subcall(delegatef.callable(), [&input](immediate_context* context)
					{
						context->set_arg_object(0, (void*)&input);
					}, [&intermediate](immediate_context* context)
					{
						auto* result = context->get_return_object<core::string>();
						if (result != nullptr)
							intermediate = *result;
					});
					*buffer = (uint8_t*)intermediate.data();
					*size = intermediate.size();
				};
				return expects_wrapper::unwrap(compute::crypto::file_encrypt(type, from, to, key, salt, std::move(callback), read_interval, complexity), (size_t)0);
			}
			size_t crypto_file_decrypt(compute::cipher type, core::stream* from, core::stream* to, const compute::secret_box& key, const compute::secret_box& salt, asIScriptFunction* transform, size_t read_interval, int complexity)
			{
				core::string intermediate;
				immediate_context* context = immediate_context::get();
				function_delegate delegatef(transform);
				auto callback = [context, &delegatef, &intermediate](uint8_t** buffer, size_t* size) -> void
				{
					if (!context || !delegatef.is_valid())
						return;

					std::string_view input((char*)*buffer, *size);
					context->execute_subcall(delegatef.callable(), [&input](immediate_context* context)
					{
						context->set_arg_object(0, (void*)&input);
					}, [&intermediate](immediate_context* context)
					{
						auto* result = context->get_return_object<core::string>();
						if (result != nullptr)
							intermediate = *result;
					});
					*buffer = (uint8_t*)intermediate.data();
					*size = intermediate.size();
				};
				return expects_wrapper::unwrap(compute::crypto::file_decrypt(type, from, to, key, salt, std::move(callback), read_interval, complexity), (size_t)0);
			}

			void include_desc_add_ext(compute::include_desc& base, const std::string_view& value)
			{
				base.exts.push_back(core::string(value));
			}
			void include_desc_remove_ext(compute::include_desc& base, const std::string_view& value)
			{
				auto it = std::find(base.exts.begin(), base.exts.end(), value);
				if (it != base.exts.end())
					base.exts.erase(it);
			}

			void preprocessor_set_include_callback(compute::preprocessor* base, asIScriptFunction* callback)
			{
				immediate_context* context = immediate_context::get();
				function_delegate delegatef(callback);
				if (!context || !delegatef.is_valid())
					return base->set_include_callback(nullptr);

				base->set_include_callback([context, delegatef](compute::preprocessor* base, const compute::include_result& file, core::string& output) -> compute::expects_preprocessor<compute::include_type>
				{
					compute::include_type result;
					context->execute_subcall(delegatef.callable(), [base, &file, &output](immediate_context* context)
					{
						context->set_arg_object(0, base);
						context->set_arg_object(1, (void*)&file);
						context->set_arg_object(2, &output);
					}, [&result](immediate_context* context)
					{
						result = (compute::include_type)context->get_return_dword();
					});
					return result;
				});
			}
			void preprocessor_set_pragma_callback(compute::preprocessor* base, asIScriptFunction* callback)
			{
				immediate_context* context = immediate_context::get();
				function_delegate delegatef(callback);
				if (!context || !delegatef.is_valid())
					return base->set_pragma_callback(nullptr);

				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				base->set_pragma_callback([type, context, delegatef](compute::preprocessor* base, const std::string_view& name, const core::vector<core::string>& args) -> compute::expects_preprocessor<void>
				{
					bool success = false;
					core::uptr<array> params = array::compose<core::string>(type.get_type_info(), args);
					context->execute_subcall(delegatef.callable(), [base, &name, &params](immediate_context* context)
					{
						context->set_arg_object(0, base);
						context->set_arg_object(1, (void*)&name);
						context->set_arg_object(2, *params);
					}, [&success](immediate_context* context)
					{
						success = (bool)context->get_return_byte();
					});
					if (!success)
						return compute::preprocessor_exception(compute::preprocessor_error::pragma_not_found, 0, "pragma name: " + core::string(name));

					return core::expectation::met;
				});
			}
			void preprocessor_set_directive_callback(compute::preprocessor* base, const std::string_view& name, asIScriptFunction* callback)
			{
				immediate_context* context = immediate_context::get();
				function_delegate delegatef(callback);
				if (!context || !delegatef.is_valid())
					return base->set_directive_callback(name, nullptr);

				base->set_directive_callback(name, [context, delegatef](compute::preprocessor* base, const compute::proc_directive& token, core::string& result) -> compute::expects_preprocessor<void>
				{
					bool success = false;
					context->execute_subcall(delegatef.callable(), [base, &token, &result](immediate_context* context)
					{
						context->set_arg_object(0, base);
						context->set_arg_object(1, (void*)&token);
						context->set_arg_object(1, (void*)&result);
					}, [&success](immediate_context* context)
					{
						success = context->get_return_byte() > 0;
					});
					if (!success)
						return compute::preprocessor_exception(compute::preprocessor_error::directive_not_found, 0, "directive name: " + token.name);

					return core::expectation::met;
				});
			}
			bool preprocessor_is_defined(compute::preprocessor* base, const std::string_view& name)
			{
				return base->is_defined(name);
			}

			core::string socket_address_get_hostname(network::socket_address& base)
			{
				return expects_wrapper::unwrap(base.get_hostname(), core::string());
			}
			core::string socket_address_get_ip_address(network::socket_address& base)
			{
				return expects_wrapper::unwrap(base.get_ip_address(), core::string());
			}
			uint16_t socket_address_get_ip_port(network::socket_address& base)
			{
				return expects_wrapper::unwrap(base.get_ip_port(), uint16_t());
			}

			dictionary* location_get_query(network::location& base)
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_DICTIONARY "@");
				return dictionary::compose<core::string>(type.get_type_id(), base.query);
			}

			dictionary* certificate_get_extensions(network::certificate& base)
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_DICTIONARY "@");
				return dictionary::compose<core::string>(type.get_type_id(), base.extensions);
			}

			core::promise<network::socket_accept> socket_accept_deferred(network::socket* base)
			{
				immediate_context* context = immediate_context::get(); base->add_ref();
				return base->accept_deferred().then<network::socket_accept>([context, base](core::expects_io<network::socket_accept>&& incoming)
				{
					base->release();
					return expects_wrapper::unwrap(std::move(incoming), network::socket_accept(), context);
				});
			}
			core::promise<bool> socket_connect_deferred(network::socket* base, const network::socket_address& address)
			{
				immediate_context* context = immediate_context::get(); base->add_ref();
				return base->connect_deferred(address).then<bool>([context, base](core::expects_io<void>&& status)
				{
					base->release();
					return expects_wrapper::unwrap_void(std::move(status), context);
				});
			}
			core::promise<bool> socket_close_deferred(network::socket* base)
			{
				immediate_context* context = immediate_context::get(); base->add_ref();
				return base->close_deferred().then<bool>([context, base](core::expects_io<void>&& status)
				{
					base->release();
					return expects_wrapper::unwrap_void(std::move(status), context);
				});
			}
			core::promise<bool> socket_write_file_deferred(network::socket* base, core::file_stream* stream, size_t offset, size_t size)
			{
				immediate_context* context = immediate_context::get(); base->add_ref();
				return base->write_file_deferred(stream ? (FILE*)stream->get_readable() : nullptr, offset, size).then<bool>([context, base](core::expects_io<size_t>&& status)
				{
					base->release();
					return expects_wrapper::unwrap_void(std::move(status), context);
				});
			}
			core::promise<bool> socket_write_deferred(network::socket* base, const std::string_view& data)
			{
				immediate_context* context = immediate_context::get(); base->add_ref();
				return base->write_deferred((uint8_t*)data.data(), data.size()).then<bool>([context, base](core::expects_io<size_t>&& status)
				{
					base->release();
					return expects_wrapper::unwrap_void(std::move(status), context);
				});
			}
			core::promise<core::string> socket_read_deferred(network::socket* base, size_t size)
			{
				immediate_context* context = immediate_context::get(); base->add_ref();
				return base->read_deferred(size).then<core::string>([context, base](core::expects_io<core::string>&& data)
				{
					base->release();
					return expects_wrapper::unwrap(std::move(data), core::string(), context);
				});
			}
			core::promise<core::string> socket_read_until_deferred(network::socket* base, const std::string_view& match, size_t max_size)
			{
				immediate_context* context = immediate_context::get(); base->add_ref();
				return base->read_until_deferred(core::string(match), max_size).then<core::string>([context, base](core::expects_io<core::string>&& data)
				{
					base->release();
					return expects_wrapper::unwrap(std::move(data), core::string(), context);
				});
			}
			core::promise<core::string> socket_read_until_chunked_deferred(network::socket* base, const std::string_view& match, size_t max_size)
			{
				immediate_context* context = immediate_context::get(); base->add_ref();
				return base->read_until_chunked_deferred(core::string(match), max_size).then<core::string>([context, base](core::expects_io<core::string>&& data)
				{
					base->release();
					return expects_wrapper::unwrap(std::move(data), core::string(), context);
				});
			}
			size_t socket_write_file(network::socket* base, core::file_stream* stream, size_t offset, size_t size)
			{
				return expects_wrapper::unwrap(base->write_file((FILE*)stream->get_readable(), offset, size), (size_t)0);
			}
			size_t socket_write(network::socket* base, const std::string_view& data)
			{
				return expects_wrapper::unwrap(base->write((uint8_t*)data.data(), (int)data.size()), (size_t)0);
			}
			size_t socket_read(network::socket* base, core::string& data, size_t size)
			{
				data.resize(size);
				return expects_wrapper::unwrap(base->read((uint8_t*)data.data(), size), (size_t)0);
			}
			size_t socket_read_until(network::socket* base, const std::string_view& data, asIScriptFunction* callback)
			{
				immediate_context* context = immediate_context::get();
				function_delegate delegatef(callback);
				if (!context || !delegatef.is_valid())
					return expects_wrapper::unwrap_void<void, std::error_condition>(std::make_error_condition(std::errc::invalid_argument)) ? 0 : 0;

				base->add_ref();
				return expects_wrapper::unwrap(base->read_until(data, [base, context, delegatef](network::socket_poll poll, const uint8_t* data, size_t size)
				{
					bool result = false;
					std::string_view source = std::string_view((char*)data, size);
					context->execute_subcall(delegatef.callable(), [poll, source](immediate_context* context)
					{
						context->set_arg32(0, (int)poll);
						context->set_arg_object(1, (void*)&source);
					}, [&result](immediate_context* context)
					{
						result = (bool)context->get_return_byte();
					});

					base->release();
					return result;
				}), (size_t)0);
			}
			size_t socket_read_until_chunked(network::socket* base, const std::string_view& data, asIScriptFunction* callback)
			{
				immediate_context* context = immediate_context::get();
				function_delegate delegatef(callback);
				if (!context || !delegatef.is_valid())
					return expects_wrapper::unwrap_void<void, std::error_condition>(std::make_error_condition(std::errc::invalid_argument)) ? 0 : 0;

				base->add_ref();
				return expects_wrapper::unwrap(base->read_until_chunked(data, [base, context, delegatef](network::socket_poll poll, const uint8_t* data, size_t size)
				{
					bool result = false;
					std::string_view source = std::string_view((char*)data, size);
					context->execute_subcall(delegatef.callable(), [poll, source](immediate_context* context)
					{
						context->set_arg32(0, (int)poll);
						context->set_arg_object(1, (void*)&source);
					}, [&result](immediate_context* context)
					{
						result = (bool)context->get_return_byte();
					});

					base->release();
					return result;
				}), (size_t)0);
			}

			core::promise<core::string> dns_reverse_lookup_deferred(network::dns* base, const std::string_view& hostname, const std::string_view& service)
			{
				immediate_context* context = immediate_context::get();
				return base->reverse_lookup_deferred(hostname, service).then<core::string>([context](core::expects_system<core::string>&& address)
				{
					return expects_wrapper::unwrap(std::move(address), core::string(), context);
				});
			}
			core::promise<core::string> dns_reverse_address_lookup_deferred(network::dns* base, const network::socket_address& address)
			{
				immediate_context* context = immediate_context::get();
				return base->reverse_address_lookup_deferred(address).then<core::string>([context](core::expects_system<core::string>&& address)
				{
					return expects_wrapper::unwrap(std::move(address), core::string(), context);
				});
			}
			core::promise<network::socket_address> dns_lookup_deferred(network::dns* base, const std::string_view& hostname, const std::string_view& service, network::dns_type mode, network::socket_protocol protocol, network::socket_type type)
			{
				immediate_context* context = immediate_context::get();
				return base->lookup_deferred(hostname, service, mode, protocol, type).then<network::socket_address>([context](core::expects_system<network::socket_address>&& address)
				{
					return expects_wrapper::unwrap(std::move(address), network::socket_address(), context);
				});
			}

			network::epoll_fd& epoll_fd_copy(network::epoll_fd& base, network::epoll_fd& other)
			{
				if (&base == &other)
					return base;

				core::memory::release(base.base);
				base = other;
				if (base.base != nullptr)
					base.base->add_ref();

				return base;
			}
			void epoll_fd_destructor(network::epoll_fd& base)
			{
				core::memory::release(base.base);
			}

			int epoll_interface_wait(network::epoll_interface& base, array* data, uint64_t timeout)
			{
				core::vector<network::epoll_fd> fds = array::decompose<network::epoll_fd>(data);
				return base.wait(fds.data(), fds.size(), timeout);
			}

			bool multiplexer_when_readable(network::socket* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
					return false;

				return network::multiplexer::get()->when_readable(base, [base, delegatef](network::socket_poll poll) mutable
				{
					delegatef([base, poll](immediate_context* context)
					{
						context->set_arg_object(0, base);
						context->set_arg32(1, (int)poll);
					});
				});
			}
			bool multiplexer_when_writeable(network::socket* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
					return false;

				return network::multiplexer::get()->when_writeable(base, [base, delegatef](network::socket_poll poll) mutable
				{
					delegatef([base, poll](immediate_context* context)
					{
						context->set_arg_object(0, base);
						context->set_arg32(1, (int)poll);
					});
				});
			}

			void socket_router_listen1(network::socket_router* base, const std::string_view& hostname, const std::string_view& service, bool secure)
			{
				expects_wrapper::unwrap(base->listen(hostname, service, secure), (network::router_listener*)nullptr);
			}
			void socket_router_listen2(network::socket_router* base, const std::string_view& pattern, const std::string_view& hostname, const std::string_view& service, bool secure)
			{
				expects_wrapper::unwrap(base->listen(pattern, hostname, service, secure), (network::router_listener*)nullptr);
			}
			void socket_router_set_listeners(network::socket_router* base, dictionary* data)
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_DICTIONARY "@");
				base->listeners = dictionary::decompose<network::router_listener>(type.get_type_id(), data);
			}
			dictionary* socket_router_get_listeners(network::socket_router* base)
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_DICTIONARY "@");
				return dictionary::compose<network::router_listener>(type.get_type_id(), base->listeners);
			}
			void socket_router_set_certificates(network::socket_router* base, dictionary* data)
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_DICTIONARY "@");
				base->certificates = dictionary::decompose<network::socket_certificate>(type.get_type_id(), data);
			}
			dictionary* socket_router_get_certificates(network::socket_router* base)
			{
				type_info type = virtual_machine::get()->get_type_info_by_decl(TYPENAME_DICTIONARY "@");
				return dictionary::compose<network::socket_certificate>(type.get_type_id(), base->certificates);
			}

			void socket_server_set_router(network::socket_server* server, network::socket_router* router)
			{
				if (router != nullptr)
					router->add_ref();
				server->set_router(router);
			}
			bool socket_server_configure(network::socket_server* server, network::socket_router* router)
			{
				if (router != nullptr)
					router->add_ref();
				return expects_wrapper::unwrap_void(server->configure(router));
			}
			bool socket_server_listen(network::socket_server* server)
			{
				return expects_wrapper::unwrap_void(server->listen());
			}
			bool socket_server_unlisten(network::socket_server* server, bool gracefully)
			{
				return expects_wrapper::unwrap_void(server->unlisten(gracefully));
			}

			core::promise<bool> socket_client_connect_sync(network::socket_client* client, const network::socket_address& address, int32_t verify_peers)
			{
				immediate_context* context = immediate_context::get();
				return client->connect_sync(address, verify_peers).then<bool>([context](core::expects_system<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> socket_client_connect_async(network::socket_client* client, const network::socket_address& address, int32_t verify_peers)
			{
				immediate_context* context = immediate_context::get();
				return client->connect_async(address, verify_peers).then<bool>([context](core::expects_system<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> socket_client_disconnect(network::socket_client* client)
			{
				immediate_context* context = immediate_context::get();
				return client->disconnect().then<bool>([context](core::expects_system<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}

			bool socket_connection_abort(network::socket_connection* base, int code, const std::string_view& message)
			{
				return base->abort(code, "%.*s", (int)message.size(), message.data());
			}

			void vm_collect_garbage(uint64_t interval_ms)
			{
				immediate_context* context = immediate_context::get();
				virtual_machine* vm = virtual_machine::get();
				if (vm != nullptr)
					vm->perform_periodic_garbage_collection(interval_ms);
			}

			template <typename t>
			void* processor_duplicate(t* base, layer::asset_cache* cache, core::schema* args)
			{
				return expects_wrapper::unwrap(base->duplicate(cache, to_variant_keys(args)), (void*)nullptr);
			}
			template <typename t>
			void* processor_deserialize(t* base, core::stream* stream, size_t offset, core::schema* args)
			{
				return expects_wrapper::unwrap(base->deserialize(stream, offset, to_variant_keys(args)), (void*)nullptr);
			}
			template <typename t>
			bool processor_serialize(t* base, core::stream* stream, void* instance, core::schema* args)
			{
				return expects_wrapper::unwrap_void(base->serialize(stream, instance, to_variant_keys(args)));
			}
			template <typename t>
			void populate_processor_base(ref_class& class_name, bool base_cast = true)
			{
				if (base_cast)
					class_name.set_operator_extern(operators::cast_t, 0, "void", "?&out", &handle_to_handle_cast);

				class_name.set_method("void free(uptr@)", &layer::processor::free);
				class_name.set_method_extern("uptr@ duplicate(uptr@, schema@+)", &processor_duplicate<t>);
				class_name.set_method_extern("uptr@ deserialize(base_stream@+, usize, schema@+)", &processor_deserialize<t>);
				class_name.set_method_extern("bool serialize(base_stream@+, uptr@, schema@+)", &processor_serialize<t>);
				class_name.set_method("content_manager@+ get_content() const", &layer::processor::get_content);
			}
			template <typename t, typename... args>
			void populate_processor_interface(ref_class& class_name, const char* Constructor)
			{
				class_name.set_constructor<t, args...>(Constructor);
				class_name.set_dynamic_cast<t, layer::processor>("base_processor@+", true);
				populate_processor_base<t>(class_name, false);
			}

			void* content_manager_load(layer::content_manager* base, layer::processor* source, const std::string_view& path, core::schema* args)
			{
				return expects_wrapper::unwrap(base->load(source, path, to_variant_keys(args)), (void*)nullptr);
			}
			bool content_manager_save(layer::content_manager* base, layer::processor* source, const std::string_view& path, void* object, core::schema* args)
			{
				return expects_wrapper::unwrap_void(base->save(source, path, object, to_variant_keys(args)));
			}
			void content_manager_load_deferred2(layer::content_manager* base, layer::processor* source, const std::string_view& path, core::schema* args, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
					return;

				base->load_deferred(source, path, to_variant_keys(args)).when([delegatef](layer::expects_content<void*>&& object) mutable
				{
					void* address = object.or_else(nullptr);
					delegatef([address](immediate_context* context)
					{
						context->set_arg_address(0, address);
					});
				});
			}
			void content_manager_load_deferred1(layer::content_manager* base, layer::processor* source, const std::string_view& path, asIScriptFunction* callback)
			{
				content_manager_load_deferred2(base, source, path, nullptr, callback);
			}
			void content_manager_save_deferred2(layer::content_manager* base, layer::processor* source, const std::string_view& path, void* object, core::schema* args, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
					return;

				base->save_deferred(source, path, object, to_variant_keys(args)).when([delegatef](layer::expects_content<void>&& status) mutable
				{
					bool success = !!status;
					delegatef([success](immediate_context* context)
					{
						context->set_arg8(0, success);
					});
				});
			}
			void content_manager_save_deferred1(layer::content_manager* base, layer::processor* source, const std::string_view& path, void* object, asIScriptFunction* callback)
			{
				content_manager_save_deferred2(base, source, path, object, nullptr, callback);
			}
			bool content_manager_find_cache1(layer::content_manager* base, layer::processor* source, layer::asset_cache& output, const std::string_view& path)
			{
				auto* cache = base->find_cache(source, path);
				if (!cache)
					return false;

				output = *cache;
				return true;
			}
			bool content_manager_find_cache2(layer::content_manager* base, layer::processor* source, layer::asset_cache& output, void* object)
			{
				auto* cache = base->find_cache(source, object);
				if (!cache)
					return false;

				output = *cache;
				return true;
			}
			layer::processor* content_manager_add_processor(layer::content_manager* base, layer::processor* source, uint64_t id)
			{
				if (!source)
					return nullptr;

				return base->add_processor(source, id);
			}

			core::string resource_get_header_blob(network::http::resource* base, const std::string_view& name)
			{
				auto* value = base->get_header_blob(name);
				return value ? *value : core::string();
			}

			core::schema* content_frame_get_json(network::http::content_frame& base)
			{
				return expects_wrapper::unwrap(base.get_json(), (core::schema*)nullptr);
			}
			core::schema* content_frame_get_xml(network::http::content_frame& base)
			{
				return expects_wrapper::unwrap(base.get_xml(), (core::schema*)nullptr);
			}
			void content_frame_prepare1(network::http::content_frame& base, const network::http::request_frame& target, const std::string_view& leftover_content)
			{
				base.prepare(target.headers, (uint8_t*)leftover_content.data(), leftover_content.size());
			}
			void content_frame_prepare2(network::http::content_frame& base, const network::http::response_frame& target, const std::string_view& leftover_content)
			{
				base.prepare(target.headers, (uint8_t*)leftover_content.data(), leftover_content.size());
			}
			void content_frame_prepare3(network::http::content_frame& base, const network::http::fetch_frame& target, const std::string_view& leftover_content)
			{
				base.prepare(target.headers, (uint8_t*)leftover_content.data(), leftover_content.size());
			}
			void content_frame_add_resource(network::http::content_frame& base, const network::http::resource& item)
			{
				base.resources.push_back(item);
			}
			void content_frame_clear_resources(network::http::content_frame& base)
			{
				base.resources.clear();
			}
			size_t content_frame_get_resources_size(network::http::content_frame& base)
			{
				return base.resources.size();
			}
			network::http::resource content_frame_get_resource(network::http::content_frame& base, size_t index)
			{
				if (index >= base.resources.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return network::http::resource();
				}

				return base.resources[index];
			}

			core::string request_frame_get_header_blob(network::http::request_frame& base, const std::string_view& name)
			{
				auto* value = base.get_header_blob(name);
				return value ? *value : core::string();
			}
			core::string request_frame_get_header(network::http::request_frame& base, size_t index, size_t subindex)
			{
				if (index >= base.headers.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return core::string();
				}

				auto it = base.headers.begin();
				while (index-- > 0)
					++it;

				if (subindex >= it->second.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return core::string();
				}

				return it->second[subindex];
			}
			size_t request_frame_get_header_size(network::http::request_frame& base, size_t index)
			{
				if (index >= base.headers.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return 0;
				}

				auto it = base.headers.begin();
				while (index-- > 0)
					++it;

				return it->second.size();
			}
			size_t request_frame_get_headers_size(network::http::request_frame& base)
			{
				return base.headers.size();
			}
			core::string request_frame_get_cookie_blob(network::http::request_frame& base, const std::string_view& name)
			{
				auto* value = base.get_cookie_blob(name);
				return value ? *value : core::string();
			}
			core::string request_frame_get_cookie(network::http::request_frame& base, size_t index, size_t subindex)
			{
				if (index >= base.cookies.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return core::string();
				}

				auto it = base.cookies.begin();
				while (index-- > 0)
					++it;

				if (subindex >= it->second.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return core::string();
				}

				return it->second[subindex];
			}
			size_t request_frame_get_cookie_size(network::http::request_frame& base, size_t index)
			{
				if (index >= base.cookies.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return 0;
				}

				auto it = base.cookies.begin();
				while (index-- > 0)
					++it;

				return it->second.size();
			}
			size_t request_frame_get_cookies_size(network::http::request_frame& base)
			{
				return base.cookies.size();
			}
			void request_frame_set_method(network::http::request_frame& base, const std::string_view& value)
			{
				base.set_method(value);
			}
			core::string request_frame_get_method(network::http::request_frame& base)
			{
				return base.method;
			}
			core::string request_frame_get_version(network::http::request_frame& base)
			{
				return base.version;
			}

			core::string response_frame_get_header_blob(network::http::response_frame& base, const std::string_view& name)
			{
				auto* value = base.get_header_blob(name);
				return value ? *value : core::string();
			}
			core::string response_frame_get_header(network::http::response_frame& base, size_t index, size_t subindex)
			{
				if (index >= base.headers.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return core::string();
				}

				auto it = base.headers.begin();
				while (index-- > 0)
					++it;

				if (subindex >= it->second.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return core::string();
				}

				return it->second[subindex];
			}
			size_t response_frame_get_header_size(network::http::response_frame& base, size_t index)
			{
				if (index >= base.headers.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return 0;
				}

				auto it = base.headers.begin();
				while (index-- > 0)
					++it;

				return it->second.size();
			}
			size_t response_frame_get_headers_size(network::http::response_frame& base)
			{
				return base.headers.size();
			}
			network::http::cookie response_frame_get_cookie1(network::http::response_frame& base, const std::string_view& name)
			{
				auto value = base.get_cookie(name);
				return value ? *value : network::http::cookie();
			}
			network::http::cookie response_frame_get_cookie2(network::http::response_frame& base, size_t index)
			{
				if (index >= base.cookies.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return network::http::cookie();
				}

				return base.cookies[index];
			}
			size_t response_frame_get_cookies_size(network::http::response_frame& base)
			{
				return base.cookies.size();
			}

			core::string fetch_frame_get_header_blob(network::http::fetch_frame& base, const std::string_view& name)
			{
				auto* value = base.get_header_blob(name);
				return value ? *value : core::string();
			}
			core::string fetch_frame_get_header(network::http::fetch_frame& base, size_t index, size_t subindex)
			{
				if (index >= base.headers.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return core::string();
				}

				auto it = base.headers.begin();
				while (index-- > 0)
					++it;

				if (subindex >= it->second.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return core::string();
				}

				return it->second[subindex];
			}
			size_t fetch_frame_get_header_size(network::http::fetch_frame& base, size_t index)
			{
				if (index >= base.headers.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return 0;
				}

				auto it = base.headers.begin();
				while (index-- > 0)
					++it;

				return it->second.size();
			}
			size_t fetch_frame_get_headers_size(network::http::fetch_frame& base)
			{
				return base.headers.size();
			}
			core::string fetch_frame_get_cookie_blob(network::http::fetch_frame& base, const std::string_view& name)
			{
				auto* value = base.get_cookie_blob(name);
				return value ? *value : core::string();
			}
			core::string fetch_frame_get_cookie(network::http::fetch_frame& base, size_t index, size_t subindex)
			{
				if (index >= base.cookies.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return core::string();
				}

				auto it = base.cookies.begin();
				while (index-- > 0)
					++it;

				if (subindex >= it->second.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return core::string();
				}

				return it->second[subindex];
			}
			size_t fetch_frame_get_cookie_size(network::http::fetch_frame& base, size_t index)
			{
				if (index >= base.cookies.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return 0;
				}

				auto it = base.cookies.begin();
				while (index-- > 0)
					++it;

				return it->second.size();
			}
			size_t fetch_frame_get_cookies_size(network::http::fetch_frame& base)
			{
				return base.cookies.size();
			}

			network::http::router_entry* router_group_get_route(network::http::router_group* base, size_t index)
			{
				if (index >= base->routes.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return base->routes[index];
			}
			size_t router_group_get_routes_size(network::http::router_group* base)
			{
				return base->routes.size();
			}

			void router_entry_set_hidden_files(network::http::router_entry* base, array* data)
			{
				if (data != nullptr)
					base->hidden_files = array::decompose<compute::regex_source>(data);
				else
					base->hidden_files.clear();
			}
			array* router_entry_get_hidden_files(network::http::router_entry* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_REGEXSOURCE ">@");
				return array::compose<compute::regex_source>(type.get_type_info(), base->hidden_files);
			}
			void router_entry_set_error_files(network::http::router_entry* base, array* data)
			{
				if (data != nullptr)
					base->error_files = array::decompose<network::http::error_file>(data);
				else
					base->error_files.clear();
			}
			array* router_entry_get_error_files(network::http::router_entry* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_HTTPERRORFILE ">@");
				return array::compose<network::http::error_file>(type.get_type_info(), base->error_files);
			}
			void router_entry_set_mime_types(network::http::router_entry* base, array* data)
			{
				if (data != nullptr)
					base->mime_types = array::decompose<network::http::mime_type>(data);
				else
					base->mime_types.clear();
			}
			array* router_entry_get_mime_types(network::http::router_entry* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_HTTPMIMETYPE ">@");
				return array::compose<network::http::mime_type>(type.get_type_info(), base->mime_types);
			}
			void router_entry_set_index_files(network::http::router_entry* base, array* data)
			{
				if (data != nullptr)
					base->index_files = array::decompose<core::string>(data);
				else
					base->index_files.clear();
			}
			array* router_entry_get_index_files(network::http::router_entry* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose<core::string>(type.get_type_info(), base->index_files);
			}
			void router_entry_set_try_files(network::http::router_entry* base, array* data)
			{
				if (data != nullptr)
					base->try_files = array::decompose<core::string>(data);
				else
					base->try_files.clear();
			}
			array* router_entry_get_try_files(network::http::router_entry* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose<core::string>(type.get_type_info(), base->try_files);
			}
			void router_entry_set_disallowed_methods_files(network::http::router_entry* base, array* data)
			{
				if (data != nullptr)
					base->disallowed_methods = array::decompose<core::string>(data);
				else
					base->disallowed_methods.clear();
			}
			array* router_entry_get_disallowed_methods_files(network::http::router_entry* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose<core::string>(type.get_type_info(), base->disallowed_methods);
			}
			network::http::map_router* router_entry_get_router(network::http::router_entry* base)
			{
				return base->router;
			}

			void route_auth_set_methods(network::http::router_entry::entry_auth& base, array* data)
			{
				if (data != nullptr)
					base.methods = array::decompose<core::string>(data);
				else
					base.methods.clear();
			}
			array* route_auth_get_methods(network::http::router_entry::entry_auth& base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose<core::string>(type.get_type_info(), base.methods);
			}

			void route_compression_set_files(network::http::router_entry::entry_compression& base, array* data)
			{
				if (data != nullptr)
					base.files = array::decompose<compute::regex_source>(data);
				else
					base.files.clear();
			}
			array* route_compression_get_files(network::http::router_entry::entry_compression& base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_REGEXSOURCE ">@");
				return array::compose<compute::regex_source>(type.get_type_info(), base.files);
			}

			network::http::router_entry* map_router_get_base(network::http::map_router* base)
			{
				return base->base;
			}
			network::http::router_group* map_router_get_group(network::http::map_router* base, size_t index)
			{
				if (index >= base->groups.size())
				{
					bindings::exception::throw_ptr(bindings::exception::pointer(EXCEPTION_OUTOFBOUNDS));
					return nullptr;
				}

				return base->groups[index];
			}
			size_t map_router_get_groups_size(network::http::map_router* base)
			{
				return base->groups.size();
			}
			network::http::router_entry* map_router_fetch_route(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern)
			{
				return base->route(match, mode, pattern, false);
			}
			bool map_router_get2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.get = nullptr;
					return false;
				}

				route->callbacks.get = [delegatef](network::http::connection* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
					return true;
				};
				return true;
			}
			bool map_router_get1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_get2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool map_router_post2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.post = nullptr;
					return false;
				}

				route->callbacks.post = [delegatef](network::http::connection* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
					return true;
				};
				return true;
			}
			bool map_router_post1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_post2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool map_router_patch2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.patch = nullptr;
					return false;
				}

				route->callbacks.patch = [delegatef](network::http::connection* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
					return true;
				};
				return true;
			}
			bool map_router_patch1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_patch2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool map_router_delete2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.deinit = nullptr;
					return false;
				}

				route->callbacks.deinit = [delegatef](network::http::connection* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
					return true;
				};
				return true;
			}
			bool map_router_delete1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_delete2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool map_router_options2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.options = nullptr;
					return false;
				}

				route->callbacks.options = [delegatef](network::http::connection* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
					return true;
				};
				return true;
			}
			bool map_router_options1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_options2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool map_router_access2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.access = nullptr;
					return false;
				}

				route->callbacks.access = [delegatef](network::http::connection* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
					return true;
				};
				return true;
			}
			bool map_router_access1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_access2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool map_router_headers2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.headers = nullptr;
					return false;
				}

				route->callbacks.headers = [delegatef](network::http::connection* base, core::string& source) mutable
				{
					delegatef([base, &source](immediate_context* context)
					{
						context->set_arg_object(0, base);
						context->set_arg_object(1, &source);
					}).wait();
					return true;
				};
				return true;
			}
			bool map_router_headers1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_headers2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool map_router_authorize2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.authorize = nullptr;
					return false;
				}

				route->callbacks.authorize = [delegatef](network::http::connection* base, network::http::credentials* source) mutable
				{
					delegatef([base, source](immediate_context* context)
					{
						context->set_arg_object(0, base);
						context->set_arg_object(1, source);
					}).wait();
					return true;
				};
				return true;
			}
			bool map_router_authorize1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_authorize2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool map_router_websocket_initiate2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.web_socket.initiate = nullptr;
					return false;
				}

				route->callbacks.web_socket.initiate = [delegatef](network::http::connection* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
					return true;
				};
				return true;
			}
			bool map_router_websocket_initiate1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_websocket_initiate2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool map_router_websocket_connect2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.web_socket.connect = nullptr;
					return false;
				}

				route->callbacks.web_socket.connect = [delegatef](network::http::web_socket_frame* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
					return true;
				};
				return true;
			}
			bool map_router_websocket_connect1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_websocket_connect2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool map_router_websocket_disconnect2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.web_socket.disconnect = nullptr;
					return false;
				}

				route->callbacks.web_socket.disconnect = [delegatef](network::http::web_socket_frame* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
					return true;
				};
				return true;
			}
			bool map_router_websocket_disconnect1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_websocket_disconnect2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool map_router_websocket_receive2(network::http::map_router* base, const std::string_view& match, network::http::route_mode mode, const std::string_view& pattern, asIScriptFunction* callback)
			{
				auto* route = map_router_fetch_route(base, match, mode, pattern);
				if (!route)
					return false;

				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					route->callbacks.web_socket.receive = nullptr;
					return false;
				}

				route->callbacks.web_socket.receive = [delegatef](network::http::web_socket_frame* base, network::http::web_socket_op opcode, const std::string_view& data) mutable
				{
					core::string buffer = core::string(data);
					delegatef([base, opcode, buffer](immediate_context* context)
					{
						context->set_arg_object(0, base);
						context->set_arg32(1, (int)opcode);
						context->set_arg_object(2, (void*)&buffer);
					});
					return true;
				};
				return true;
			}
			bool map_router_websocket_receive1(network::http::map_router* base, const std::string_view& pattern, asIScriptFunction* callback)
			{
				return map_router_websocket_receive2(base, "", network::http::route_mode::start, pattern, callback);
			}
			bool web_socket_frame_set_on_connect(network::http::web_socket_frame* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					base->connect = nullptr;
					return false;
				}

				base->connect = [delegatef](network::http::web_socket_frame* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
				};
				return true;
			}
			bool web_socket_frame_set_on_before_disconnect(network::http::web_socket_frame* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					base->before_disconnect = nullptr;
					return false;
				}

				base->before_disconnect = [delegatef](network::http::web_socket_frame* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
				};
				return true;
			}
			bool web_socket_frame_set_on_disconnect(network::http::web_socket_frame* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					base->disconnect = nullptr;
					return false;
				}

				base->disconnect = [delegatef](network::http::web_socket_frame* base) mutable
				{
					delegatef([base](immediate_context* context) { context->set_arg_object(0, base); });
				};
				return true;
			}
			bool web_socket_frame_set_on_receive(network::http::web_socket_frame* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
				{
					base->receive = nullptr;
					return false;
				}

				base->receive = [delegatef](network::http::web_socket_frame* base, network::http::web_socket_op opcode, const std::string_view& data) mutable
				{
					core::string buffer = core::string(data);
					delegatef([base, opcode, buffer](immediate_context* context)
					{
						context->set_arg_object(0, base);
						context->set_arg32(1, (int)opcode);
						context->set_arg_object(2, (void*)&buffer);
					});
					return true;
				};
				return true;
			}
			core::promise<bool> web_socket_frame_send2(network::http::web_socket_frame* base, uint32_t mask, const std::string_view& data, network::http::web_socket_op opcode)
			{
				core::promise<bool> result;
				expects_wrapper::unwrap_void(base->send(mask, data, opcode, [result](network::http::web_socket_frame* base) mutable { result.set(true); }));
				return result;
			}
			core::promise<bool> web_socket_frame_send1(network::http::web_socket_frame* base, const std::string_view& data, network::http::web_socket_op opcode)
			{
				return web_socket_frame_send2(base, 0, data, opcode);
			}
			core::promise<bool> web_socket_frame_send_close(network::http::web_socket_frame* base, uint32_t mask, const std::string_view& data, network::http::web_socket_op opcode)
			{
				core::promise<bool> result;
				expects_wrapper::unwrap_void(base->send_close([result](network::http::web_socket_frame* base) mutable { result.set(true); }));
				return result;
			}

			core::promise<bool> connection_send_headers(network::http::connection* base, int status_code, bool specify_transfer_encoding)
			{
				core::promise<bool> result; immediate_context* context = immediate_context::get();
				bool sending = base->send_headers(status_code, specify_transfer_encoding, [result, context](network::http::connection*, network::socket_poll event) mutable
				{
					result.set(expects_wrapper::unwrap_void(from_socket_poll(event), context));
				});
				if (!sending)
					result.set(expects_wrapper::unwrap_void(core::expects_system<void>(core::system_exception("cannot send headers: illegal operation")), context));
				return result;
			}
			core::promise<bool> connection_send_chunk(network::http::connection* base, const std::string_view& chunk)
			{
				core::promise<bool> result; immediate_context* context = immediate_context::get();
				bool sending = base->send_chunk(chunk, [result, context](network::http::connection*, network::socket_poll event) mutable
				{
					result.set(expects_wrapper::unwrap_void(from_socket_poll(event), context));
				});
				if (!sending)
					result.set(expects_wrapper::unwrap_void(core::expects_system<void>(core::system_exception("cannot send chunk: illegal operation")), context));
				return result;
			}
			core::promise<bool> connection_skip(network::http::connection* base)
			{
				core::promise<bool> result;
				base->skip([result](network::http::connection*) mutable { result.set(true); return true; });
				return result;
			}
			core::promise<array*> connection_store(network::http::connection* base, bool eat)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return core::promise<array*>((array*)nullptr);

				core::promise<array*> result;
				core::vector<network::http::resource>* resources = core::memory::init<core::vector<network::http::resource>>();
				asITypeInfo* type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_HTTPRESOURCEINFO ">@").get_type_info();
				base->store([result, resources, type](network::http::resource* next) mutable
				{
					if (next != nullptr)
					{
						resources->push_back(*next);
						return true;
					}

					result.set(array::compose<network::http::resource>(type, *resources));
					core::memory::deinit(resources);
					return true;
				}, eat);
				return result;
			}
			core::promise<core::string> connection_fetch(network::http::connection* base, bool eat)
			{
				core::promise<core::string> result;
				core::string* data = core::memory::init<core::string>();
				base->fetch([result, data](network::http::connection*, network::socket_poll poll, const std::string_view& buffer) mutable
				{
					if (buffer.empty())
					{
						result.set(std::move(*data));
						core::memory::deinit(data);
					}
					else
						data->append(buffer);
					return true;
				}, eat);
				return result;
			}
			core::string connection_get_peer_ip_address(network::http::connection* base)
			{
				return expects_wrapper::unwrap(base->get_peer_ip_address(), core::string());
			}
			network::http::web_socket_frame* connection_get_web_socket(network::http::connection* base)
			{
				return base->web_socket;
			}
			network::http::router_entry* connection_get_route(network::http::connection* base)
			{
				return base->route;
			}
			network::http::server* connection_get_server(network::http::connection* base)
			{
				return base->root;
			}

			void query_decode(network::http::query* base, const std::string_view& content_type, const std::string_view& data)
			{
				base->decode(content_type, data);
			}
			core::string query_encode(network::http::query* base, const std::string_view& content_type)
			{
				return base->encode(content_type);
			}
			void query_set_data(network::http::query* base, core::schema* data)
			{
				core::memory::release(base->object);
				base->object = data;
				if (base->object != nullptr)
					base->object->add_ref();
			}
			core::schema* query_get_data(network::http::query* base)
			{
				return base->object;
			}

			void session_set_data(network::http::session* base, core::schema* data)
			{
				core::memory::release(base->query);
				base->query = data;
				if (base->query != nullptr)
					base->query->add_ref();
			}
			core::schema* session_get_data(network::http::session* base)
			{
				return base->query;
			}

			core::promise<bool> client_skip(network::http::client* base)
			{
				immediate_context* context = immediate_context::get();
				return base->skip().then<bool>([context](core::expects_system<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> client_fetch(network::http::client* base, size_t max_size, bool eat)
			{
				immediate_context* context = immediate_context::get();
				return base->fetch(max_size, eat).then<bool>([context](core::expects_system<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> client_upgrade(network::http::client* base, const network::http::request_frame& frame)
			{
				immediate_context* context = immediate_context::get();
				return base->upgrade(network::http::request_frame(frame)).then<bool>([context](core::expects_system<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> client_send(network::http::client* base, const network::http::request_frame& frame)
			{
				immediate_context* context = immediate_context::get();
				return base->send(network::http::request_frame(frame)).then<bool>([context](core::expects_system<void>&& result) -> bool
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> client_send_fetch(network::http::client* base, const network::http::request_frame& frame, size_t max_size)
			{
				immediate_context* context = immediate_context::get();
				return base->send_fetch(network::http::request_frame(frame), max_size).then<bool>([context](core::expects_system<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<core::schema*> client_json(network::http::client* base, const network::http::request_frame& frame, size_t max_size)
			{
				immediate_context* context = immediate_context::get();
				return base->json(network::http::request_frame(frame), max_size).then<core::schema*>([context](core::expects_system<core::schema*>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), (core::schema*)nullptr, context);
				});
			}
			core::promise<core::schema*> client_xml(network::http::client* base, const network::http::request_frame& frame, size_t max_size)
			{
				immediate_context* context = immediate_context::get();
				return base->xml(network::http::request_frame(frame), max_size).then<core::schema*>([context](core::expects_system<core::schema*>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), (core::schema*)nullptr, context);
				});
			}

			core::promise<network::http::response_frame> http_fetch(const std::string_view& location, const std::string_view& method, const network::http::fetch_frame& options)
			{
				immediate_context* context = immediate_context::get();
				return network::http::fetch(location, method, options).then<network::http::response_frame>([context](core::expects_system<network::http::response_frame>&& response) -> network::http::response_frame
				{
					return expects_wrapper::unwrap(std::move(response), network::http::response_frame(), context);
				});
			}

			array* smtp_request_get_recipients(network::smtp::request_frame* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_SMTPRECIPIENT ">@");
				return array::compose<network::smtp::recipient>(type.get_type_info(), base->recipients);
			}
			void smtp_request_set_recipients(network::smtp::request_frame* base, array* data)
			{
				if (data != nullptr)
					base->recipients = array::decompose<network::smtp::recipient>(data);
				else
					base->recipients.clear();
			}
			array* smtp_request_get_cc_recipients(network::smtp::request_frame* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_SMTPRECIPIENT ">@");
				return array::compose<network::smtp::recipient>(type.get_type_info(), base->cc_recipients);
			}
			void smtp_request_set_cc_recipients(network::smtp::request_frame* base, array* data)
			{
				if (data != nullptr)
					base->cc_recipients = array::decompose<network::smtp::recipient>(data);
				else
					base->cc_recipients.clear();
			}
			array* smtp_request_get_bcc_recipients(network::smtp::request_frame* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_SMTPRECIPIENT ">@");
				return array::compose<network::smtp::recipient>(type.get_type_info(), base->bcc_recipients);
			}
			void smtp_request_set_bcc_recipients(network::smtp::request_frame* base, array* data)
			{
				if (data != nullptr)
					base->bcc_recipients = array::decompose<network::smtp::recipient>(data);
				else
					base->bcc_recipients.clear();
			}
			array* smtp_request_get_attachments(network::smtp::request_frame* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_SMTPATTACHMENT ">@");
				return array::compose<network::smtp::attachment>(type.get_type_info(), base->attachments);
			}
			void smtp_request_set_attachments(network::smtp::request_frame* base, array* data)
			{
				if (data != nullptr)
					base->attachments = array::decompose<network::smtp::attachment>(data);
				else
					base->attachments.clear();
			}
			array* smtp_request_get_messages(network::smtp::request_frame* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose<core::string>(type.get_type_info(), base->messages);
			}
			void smtp_request_set_messages(network::smtp::request_frame* base, array* data)
			{
				if (data != nullptr)
					base->messages = array::decompose<core::string>(data);
				else
					base->messages.clear();
			}
			void smtp_request_set_header(network::smtp::request_frame* base, const std::string_view& name, const std::string_view& value)
			{
				if (value.empty())
					base->headers.erase(core::string(name));
				else
					base->headers[core::string(name)] = value;
			}
			core::string smtp_request_get_header(network::smtp::request_frame* base, const std::string_view& name)
			{
				auto it = base->headers.find(core::key_lookup_cast(name));
				return it == base->headers.end() ? core::string() : it->second;
			}

			core::promise<bool> smtp_client_send(network::smtp::client* base, const network::smtp::request_frame& frame)
			{
				immediate_context* context = immediate_context::get();
				return base->send(network::smtp::request_frame(frame)).then<bool>([context](core::expects_system<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}

			network::sqlite::response& ldb_response_copy(network::sqlite::response& base, network::sqlite::response&& other)
			{
				if (&base == &other)
					return base;

				base = other.copy();
				return base;
			}
			array* ldb_response_get_columns(network::sqlite::response& base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose(type.get_type_info(), base.get_columns());
			}

			network::sqlite::response ldb_cursor_first(network::sqlite::cursor& base)
			{
				return base.size() > 0 ? base.first().copy() : network::sqlite::response();
			}
			network::sqlite::response ldb_cursor_last(network::sqlite::cursor& base)
			{
				return base.size() > 0 ? base.last().copy() : network::sqlite::response();
			}
			network::sqlite::response ldb_cursor_at(network::sqlite::cursor& base, size_t index)
			{
				return index < base.size() ? base.at(index).copy() : network::sqlite::response();
			}
			network::sqlite::cursor& ldb_cursor_copy(network::sqlite::cursor& base, network::sqlite::cursor&& other)
			{
				if (&base == &other)
					return base;

				base = other.copy();
				return base;
			}

			class ldb_aggregate final : public network::sqlite::aggregate
			{
			public:
				function_delegate step_delegate = function_delegate(nullptr);
				function_delegate finalize_delegate = function_delegate(nullptr);

			public:
				virtual ~ldb_aggregate() = default;
				void step(const core::variant_list& args) override
				{
					step_delegate.context->execute_subcall(step_delegate.callable(), [&args](immediate_context* context)
					{
						type_info type = context->get_vm()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_VARIANT ">@");
						core::uptr<array> data = array::compose<core::variant>(type, args);
						context->set_arg_object(0, *data);
					});
				}
				core::variant finalize() override
				{
					core::variant result = core::var::undefined();
					finalize_delegate.context->execute_subcall(finalize_delegate.callable(), nullptr, [&result](immediate_context* context) mutable
					{
						core::variant* target = context->get_return_object<core::variant>();
						if (target != nullptr)
							result = std::move(*target);
					});
					return result;
				}
			};

			class ldb_window final : public network::sqlite::window
			{
			public:
				function_delegate step_delegate = function_delegate(nullptr);
				function_delegate inverse_delegate = function_delegate(nullptr);
				function_delegate value_delegate = function_delegate(nullptr);
				function_delegate finalize_delegate = function_delegate(nullptr);

			public:
				virtual ~ldb_window() = default;
				void step(const core::variant_list& args) override
				{
					step_delegate.context->execute_subcall(step_delegate.callable(), [&args](immediate_context* context)
					{
						type_info type = context->get_vm()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_VARIANT ">@");
						core::uptr<array> data = array::compose<core::variant>(type, args);
						context->set_arg_object(0, *data);
					});
				}
				void inverse(const core::variant_list& args) override
				{
					inverse_delegate.context->execute_subcall(inverse_delegate.callable(), [&args](immediate_context* context)
					{
						type_info type = context->get_vm()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_VARIANT ">@");
						core::uptr<array> data = array::compose<core::variant>(type, args);
						context->set_arg_object(0, *data);
					});
				}
				core::variant value() override
				{
					core::variant result = core::var::undefined();
					value_delegate.context->execute_subcall(value_delegate.callable(), nullptr, [&result](immediate_context* context) mutable
					{
						core::variant* target = context->get_return_object<core::variant>();
						if (target != nullptr)
							result = std::move(*target);
					});
					return result;
				}
				core::variant finalize() override
				{
					core::variant result = core::var::undefined();
					finalize_delegate.context->execute_subcall(finalize_delegate.callable(), nullptr, [&result](immediate_context* context) mutable
					{
						core::variant* target = context->get_return_object<core::variant>();
						if (target != nullptr)
							result = std::move(*target);
					});
					return result;
				}
			};

			void ldb_cluster_set_function(network::sqlite::cluster* base, const std::string_view& name, uint8_t args, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
					return;

				base->set_function(name, args, [delegatef](const core::variant_list& args) mutable -> core::variant
				{
					core::variant result = core::var::undefined();
					delegatef.context->execute_subcall(delegatef.callable(), [&args](immediate_context* context)
					{
						type_info type = context->get_vm()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_VARIANT ">@");
						core::uptr<array> data = array::compose<core::variant>(type, args);
						context->set_arg_object(0, *data);
					}, [&result](immediate_context* context) mutable
					{
						core::variant* target = context->get_return_object<core::variant>();
						if (target != nullptr)
							result = std::move(*target);
					});
					return result;
				});
			}
			void ldb_cluster_set_aggregate_function(network::sqlite::cluster* base, const std::string_view& name, uint8_t args, asIScriptFunction* step_callback, asIScriptFunction* finalize_callback)
			{
				function_delegate step_delegate(step_callback);
				function_delegate finalize_delegate(finalize_callback);
				if (!step_delegate.is_valid() || !finalize_delegate.is_valid())
					return;

				ldb_aggregate* aggregate = new ldb_aggregate();
				aggregate->step_delegate = std::move(step_delegate);
				aggregate->finalize_delegate = std::move(finalize_delegate);
				base->set_aggregate_function(name, args, aggregate);
			}
			void ldb_cluster_set_window_function(network::sqlite::cluster* base, const std::string_view& name, uint8_t args, asIScriptFunction* step_callback, asIScriptFunction* inverse_callback, asIScriptFunction* value_callback, asIScriptFunction* finalize_callback)
			{
				function_delegate step_delegate(step_callback);
				function_delegate inverse_delegate(inverse_callback);
				function_delegate value_delegate(value_callback);
				function_delegate finalize_delegate(finalize_callback);
				if (!step_delegate.is_valid() || !inverse_delegate.is_valid() || !value_delegate.is_valid() || !finalize_delegate.is_valid())
					return;

				ldb_window* window = new ldb_window();
				window->step_delegate = std::move(step_delegate);
				window->inverse_delegate = std::move(inverse_delegate);
				window->value_delegate = std::move(value_delegate);
				window->finalize_delegate = std::move(finalize_delegate);
				base->set_window_function(name, args, window);
			}
			core::promise<network::sqlite::session_id> ldb_cluster_tx_begin(network::sqlite::cluster* base, network::sqlite::isolation level)
			{
				immediate_context* context = immediate_context::get();
				return base->tx_begin(level).then<network::sqlite::session_id>([context](network::sqlite::expects_db<network::sqlite::session_id>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), (network::sqlite::session_id)nullptr, context);
				});
			}
			core::promise<network::sqlite::session_id> ldb_cluster_tx_start(network::sqlite::cluster* base, const std::string_view& command)
			{
				immediate_context* context = immediate_context::get();
				return base->tx_start(command).then<network::sqlite::session_id>([context](network::sqlite::expects_db<network::sqlite::session_id>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), (network::sqlite::session_id)nullptr, context);
				});
			}
			core::promise<bool> ldb_cluster_tx_end(network::sqlite::cluster* base, const std::string_view& command, network::sqlite::session_id session)
			{
				immediate_context* context = immediate_context::get();
				return base->tx_end(command, session).then<bool>([context](network::sqlite::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> ldb_cluster_tx_commit(network::sqlite::cluster* base, network::sqlite::session_id session)
			{
				immediate_context* context = immediate_context::get();
				return base->tx_commit(session).then<bool>([context](network::sqlite::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> ldb_cluster_tx_rollback(network::sqlite::cluster* base, network::sqlite::session_id session)
			{
				immediate_context* context = immediate_context::get();
				return base->tx_rollback(session).then<bool>([context](network::sqlite::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> ldb_cluster_connect(network::sqlite::cluster* base, const std::string_view& address, size_t connections)
			{
				immediate_context* context = immediate_context::get();
				return base->connect(address, connections).then<bool>([context](network::sqlite::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> ldb_cluster_disconnect(network::sqlite::cluster* base)
			{
				immediate_context* context = immediate_context::get();
				return base->disconnect().then<bool>([context](network::sqlite::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> ldb_cluster_flush(network::sqlite::cluster* base)
			{
				immediate_context* context = immediate_context::get();
				return base->flush().then<bool>([context](network::sqlite::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<network::sqlite::cursor> ldb_cluster_query(network::sqlite::cluster* base, const std::string_view& command, size_t args, network::sqlite::session_id session)
			{
				immediate_context* context = immediate_context::get();
				return base->query(command, args, session).then<network::sqlite::cursor>([context](network::sqlite::expects_db<network::sqlite::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::sqlite::cursor(nullptr), context);
				});
			}
			core::promise<network::sqlite::cursor> ldb_cluster_emplace_query(network::sqlite::cluster* base, const std::string_view& command, array* data, size_t options, network::sqlite::session_id session)
			{
				core::schema_list args;
				for (auto& item : array::decompose<core::schema*>(data))
				{
					args.emplace_back(item);
					item->add_ref();
				}

				immediate_context* context = immediate_context::get();
				return base->emplace_query(command, &args, options, session).then<network::sqlite::cursor>([context](network::sqlite::expects_db<network::sqlite::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::sqlite::cursor(nullptr), context);
				});
			}
			core::promise<network::sqlite::cursor> ldb_cluster_template_query(network::sqlite::cluster* base, const std::string_view& command, dictionary* data, size_t options, network::sqlite::session_id session)
			{
				core::schema_args args;
				if (data != nullptr)
				{
					virtual_machine* vm = virtual_machine::get();
					if (vm != nullptr)
					{
						int type_id = vm->get_type_id_by_decl("schema@");
						args.reserve(data->size());

						for (auto it = data->begin(); it != data->end(); ++it)
						{
							core::schema* value = nullptr;
							if (it.get_value(&value, type_id))
							{
								args[it.get_key()] = value;
								value->add_ref();
							}
						}
					}
				}

				immediate_context* context = immediate_context::get();
				return base->template_query(command, &args, options, session).then<network::sqlite::cursor>([context](network::sqlite::expects_db<network::sqlite::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::sqlite::cursor(nullptr), context);
				});
			}
			array* ldb_cluster_wal_checkpoint(network::sqlite::cluster* base, network::sqlite::checkpoint_mode mode, const std::string_view& database)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_LDBCHECKPOINT ">@");
				return array::compose(type.get_type_info(), base->wal_checkpoint(mode, database));
			}

			core::string ldb_utils_inline_query(core::schema* where, dictionary* whitelist_data, const std::string_view& default_value)
			{
				virtual_machine* vm = virtual_machine::get();
				int type_id = vm ? vm->get_type_id_by_decl("string") : -1;
				core::unordered_map<core::string, core::string> whitelist = dictionary::decompose<core::string>(type_id, whitelist_data);
				return expects_wrapper::unwrap(network::sqlite::utils::inline_query(where, whitelist, default_value), core::string());
			}

			void ldb_driver_set_query_log(network::sqlite::driver* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (delegatef.is_valid())
				{
					base->set_query_log([delegatef](const std::string_view& data) mutable
					{
						core::string copy = core::string(data);
						delegatef([copy](immediate_context* context)
						{
							context->set_arg_object(0, (void*)&copy);
						});
					});
				}
				else
					base->set_query_log(nullptr);
			}
			core::string ldb_driver_emplace(network::sqlite::driver* base, const std::string_view& SQL, array* data)
			{
				core::schema_list args;
				for (auto& item : array::decompose<core::schema*>(data))
				{
					args.emplace_back(item);
					item->add_ref();
				}

				return expects_wrapper::unwrap(base->emplace(SQL, &args), core::string());
			}
			core::string ldb_driver_get_query(network::sqlite::driver* base, const std::string_view& SQL, dictionary* data)
			{
				core::schema_args args;
				if (data != nullptr)
				{
					virtual_machine* vm = virtual_machine::get();
					if (vm != nullptr)
					{
						int type_id = vm->get_type_id_by_decl("schema@");
						args.reserve(data->size());

						for (auto it = data->begin(); it != data->end(); ++it)
						{
							core::schema* value = nullptr;
							if (it.get_value(&value, type_id))
							{
								args[it.get_key()] = value;
								value->add_ref();
							}
						}
					}
				}

				return expects_wrapper::unwrap(base->get_query(SQL, &args), core::string());
			}
			array* ldb_driver_get_queries(network::sqlite::driver* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose(type.get_type_info(), base->get_queries());
			}

			network::pq::response& pdb_response_copy(network::pq::response& base, network::pq::response&& other)
			{
				if (&base == &other)
					return base;

				base = other.copy();
				return base;
			}
			array* pdb_response_get_columns(network::pq::response& base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose(type.get_type_info(), base.get_columns());
			}

			network::pq::response pdb_cursor_first(network::pq::cursor& base)
			{
				return base.size() > 0 ? base.first().copy() : network::pq::response();
			}
			network::pq::response pdb_cursor_last(network::pq::cursor& base)
			{
				return base.size() > 0 ? base.last().copy() : network::pq::response();
			}
			network::pq::response pdb_cursor_at(network::pq::cursor& base, size_t index)
			{
				return index < base.size() ? base.at(index).copy() : network::pq::response();
			}
			network::pq::cursor& pdb_cursor_copy(network::pq::cursor& base, network::pq::cursor&& other)
			{
				if (&base == &other)
					return base;

				base = other.copy();
				return base;
			}

			void pdb_cluster_set_when_reconnected(network::pq::cluster* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (delegatef.is_valid())
				{
					base->set_when_reconnected([base, delegatef](const core::vector<core::string>& data) mutable -> core::promise<bool>
					{
						core::promise<bool> future;
						delegatef([base, data](immediate_context* context)
						{
							type_info type = context->get_vm()->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
							context->set_arg_object(0, base);
							context->set_arg_object(1, array::compose(type.get_type_info(), data));
						}, [future](immediate_context* context) mutable
						{
							promise* target = context->get_return_object<promise>();
							if (!target)
								return future.set(true);

							target->when([future](promise* target) mutable
							{
								bool value = true;
								target->retrieve(&value, (int)type_id::bool_t);
								future.set(value);
							});
						});
						return future;
					});
				}
				else
					base->set_when_reconnected(nullptr);
			}
			uint64_t pdb_cluster_add_channel(network::pq::cluster* base, const std::string_view& name, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
					return 0;

				return base->add_channel(name, [base, delegatef](const network::pq::notify& event) mutable
				{
					delegatef([base, &event](immediate_context* context)
					{
						context->set_arg_object(0, base);
						context->set_arg_object(1, (void*)&event);
					});
				});
			}
			core::promise<bool> pdb_cluster_listen(network::pq::cluster* base, array* data)
			{
				immediate_context* context = immediate_context::get();
				core::vector<core::string> names = array::decompose<core::string>(data);
				return base->listen(names).then<bool>([context](network::pq::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> pdb_cluster_unlisten(network::pq::cluster* base, array* data)
			{
				immediate_context* context = immediate_context::get();
				core::vector<core::string> names = array::decompose<core::string>(data);
				return base->unlisten(names).then<bool>([context](network::pq::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<network::pq::session_id> pdb_cluster_tx_begin(network::pq::cluster* base, network::pq::isolation level)
			{
				immediate_context* context = immediate_context::get();
				return base->tx_begin(level).then<network::pq::session_id>([context](network::pq::expects_db<network::pq::session_id>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), (network::pq::session_id)nullptr, context);
				});
			}
			core::promise<network::pq::session_id> pdb_cluster_tx_start(network::pq::cluster* base, const std::string_view& command)
			{
				immediate_context* context = immediate_context::get();
				return base->tx_start(command).then<network::pq::session_id>([context](network::pq::expects_db<network::pq::session_id>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), (network::pq::session_id)nullptr, context);
				});
			}
			core::promise<bool> pdb_cluster_tx_end(network::pq::cluster* base, const std::string_view& command, network::pq::session_id session)
			{
				immediate_context* context = immediate_context::get();
				return base->tx_end(command, session).then<bool>([context](network::pq::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> pdb_cluster_tx_commit(network::pq::cluster* base, network::pq::session_id session)
			{
				immediate_context* context = immediate_context::get();
				return base->tx_commit(session).then<bool>([context](network::pq::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> pdb_cluster_tx_rollback(network::pq::cluster* base, network::pq::session_id session)
			{
				immediate_context* context = immediate_context::get();
				return base->tx_rollback(session).then<bool>([context](network::pq::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> pdb_cluster_connect(network::pq::cluster* base, const network::pq::address& address, size_t connections)
			{
				immediate_context* context = immediate_context::get();
				return base->connect(address, connections).then<bool>([context](network::pq::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> pdb_cluster_disconnect(network::pq::cluster* base)
			{
				immediate_context* context = immediate_context::get();
				return base->disconnect().then<bool>([context](network::pq::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<network::pq::cursor> pdb_cluster_query(network::pq::cluster* base, const std::string_view& command, size_t args, network::pq::session_id session)
			{
				immediate_context* context = immediate_context::get();
				return base->query(command, args, session).then<network::pq::cursor>([context](network::pq::expects_db<network::pq::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::pq::cursor(), context);
				});
			}
			core::promise<network::pq::cursor> pdb_cluster_emplace_query(network::pq::cluster* base, const std::string_view& command, array* data, size_t options, network::pq::connection* session)
			{
				core::schema_list args;
				for (auto& item : array::decompose<core::schema*>(data))
				{
					args.emplace_back(item);
					item->add_ref();
				}

				immediate_context* context = immediate_context::get();
				return base->emplace_query(command, &args, options, session).then<network::pq::cursor>([context](network::pq::expects_db<network::pq::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::pq::cursor(), context);
				});
			}
			core::promise<network::pq::cursor> pdb_cluster_template_query(network::pq::cluster* base, const std::string_view& command, dictionary* data, size_t options, network::pq::connection* session)
			{
				core::schema_args args;
				if (data != nullptr)
				{
					virtual_machine* vm = virtual_machine::get();
					if (vm != nullptr)
					{
						int type_id = vm->get_type_id_by_decl("schema@");
						args.reserve(data->size());

						for (auto it = data->begin(); it != data->end(); ++it)
						{
							core::schema* value = nullptr;
							if (it.get_value(&value, type_id))
							{
								args[it.get_key()] = value;
								value->add_ref();
							}
						}
					}
				}

				immediate_context* context = immediate_context::get();
				return base->template_query(command, &args, options, session).then<network::pq::cursor>([context](network::pq::expects_db<network::pq::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::pq::cursor(), context);
				});
			}

			core::string pdb_utils_inline_query(network::pq::cluster* client, core::schema* where, dictionary* whitelist_data, const std::string_view& default_value)
			{
				virtual_machine* vm = virtual_machine::get();
				int type_id = vm ? vm->get_type_id_by_decl("string") : -1;
				core::unordered_map<core::string, core::string> whitelist = dictionary::decompose<core::string>(type_id, whitelist_data);
				return expects_wrapper::unwrap(network::pq::utils::inline_query(client, where, whitelist, default_value), core::string());
			}

			void pdb_driver_set_query_log(network::pq::driver* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (delegatef.is_valid())
				{
					base->set_query_log([delegatef](const std::string_view& data) mutable
					{
						core::string copy = core::string(data);
						delegatef([copy](immediate_context* context)
						{
							context->set_arg_object(0, (void*)&copy);
						});
					});
				}
				else
					base->set_query_log(nullptr);
			}
			core::string pdb_driver_emplace(network::pq::driver* base, network::pq::cluster* cluster, const std::string_view& SQL, array* data)
			{
				core::schema_list args;
				for (auto& item : array::decompose<core::schema*>(data))
				{
					args.emplace_back(item);
					item->add_ref();
				}

				return expects_wrapper::unwrap(base->emplace(cluster, SQL, &args), core::string());
			}
			core::string pdb_driver_get_query(network::pq::driver* base, network::pq::cluster* cluster, const std::string_view& SQL, dictionary* data)
			{
				core::schema_args args;
				if (data != nullptr)
				{
					virtual_machine* vm = virtual_machine::get();
					if (vm != nullptr)
					{
						int type_id = vm->get_type_id_by_decl("schema@");
						args.reserve(data->size());

						for (auto it = data->begin(); it != data->end(); ++it)
						{
							core::schema* value = nullptr;
							if (it.get_value(&value, type_id))
							{
								args[it.get_key()] = value;
								value->add_ref();
							}
						}
					}
				}

				return expects_wrapper::unwrap(base->get_query(cluster, SQL, &args), core::string());
			}
			array* pdb_driver_get_queries(network::pq::driver* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose(type.get_type_info(), base->get_queries());
			}

			network::mongo::document mdb_document_construct_buffer(unsigned char* buffer)
			{
#ifdef VI_ANGELSCRIPT
				if (!buffer)
					return network::mongo::document::from_empty();

				network::mongo::document result = network::mongo::document::from_empty();
				virtual_machine* vm = virtual_machine::get();
				size_t length = *(asUINT*)buffer;
				buffer += 4;

				while (length--)
				{
					if (asPWORD(buffer) & 0x3)
						buffer += 4 - (asPWORD(buffer) & 0x3);

					core::string name = *(core::string*)buffer;
					buffer += sizeof(core::string);

					int type_id = *(int*)buffer;
					buffer += sizeof(int);

					void* ref = (void*)buffer;
					if (type_id >= (size_t)type_id::bool_t && type_id <= (size_t)type_id::double_t)
					{
						switch (type_id)
						{
							case (size_t)type_id::bool_t:
								result.set_boolean(name, *(bool*)ref);
								break;
							case (size_t)type_id::int8_t:
								result.set_integer(name, *(char*)ref);
								break;
							case (size_t)type_id::int16_t:
								result.set_integer(name, *(short*)ref);
								break;
							case (size_t)type_id::int32_t:
								result.set_integer(name, *(int*)ref);
								break;
							case (size_t)type_id::uint8_t:
								result.set_integer(name, *(unsigned char*)ref);
								break;
							case (size_t)type_id::uint16_t:
								result.set_integer(name, *(unsigned short*)ref);
								break;
							case (size_t)type_id::uint32_t:
								result.set_integer(name, *(uint32_t*)ref);
								break;
							case (size_t)type_id::int64_t:
							case (size_t)type_id::uint64_t:
								result.set_integer(name, *(int64_t*)ref);
								break;
							case (size_t)type_id::float_t:
								result.set_number(name, *(float*)ref);
								break;
							case (size_t)type_id::double_t:
								result.set_number(name, *(double*)ref);
								break;
						}
					}
					else
					{
						auto type = vm->get_type_info_by_id(type_id);
						if ((type_id & (size_t)type_id::mask_object_t) && !(type_id & (size_t)type_id::handle_t) && (type.is_valid() && type.flags() & (size_t)object_behaviours::ref))
							ref = *(void**)ref;

						if (type_id & (size_t)type_id::handle_t)
							ref = *(void**)ref;

						if (vm->is_nullable(type_id) || !ref)
						{
							result.set_null(name);
						}
						else if (type.is_valid() && type.get_name() == TYPENAME_SCHEMA)
						{
							core::schema* base = (core::schema*)ref;
							result.set_schema(name, network::mongo::document::from_schema(base));
						}
						else if (type.is_valid() && type.get_name() == TYPENAME_STRING)
							result.set_string(name, *(core::string*)ref);
						else if (type.is_valid() && type.get_name() == TYPENAME_DECIMAL)
							result.set_decimal_string(name, ((core::decimal*)ref)->to_string());
					}

					if (type_id & (size_t)type_id::mask_object_t)
					{
						auto type = vm->get_type_info_by_id(type_id);
						if (type.flags() & (size_t)object_behaviours::value)
							buffer += type.size();
						else
							buffer += sizeof(void*);
					}
					else if (type_id != 0)
						buffer += vm->get_size_of_primitive_type(type_id).or_else(0);
					else
						buffer += sizeof(void*);
				}

				return result;
#else
				return nullptr;
#endif
			}
			void mdb_document_construct(asIScriptGeneric* genericf)
			{
				generic_context args = genericf;
				unsigned char* buffer = (unsigned char*)args.get_arg_address(0);
				*(network::mongo::document*)args.get_address_of_return_location() = mdb_document_construct_buffer(buffer);
			}

			bool mdb_stream_template_query(network::mongo::stream& base, const std::string_view& command, dictionary* data)
			{
				core::schema_args args;
				if (data != nullptr)
				{
					virtual_machine* vm = virtual_machine::get();
					if (vm != nullptr)
					{
						int type_id = vm->get_type_id_by_decl("schema@");
						args.reserve(data->size());

						for (auto it = data->begin(); it != data->end(); ++it)
						{
							core::schema* value = nullptr;
							if (it.get_value(&value, type_id))
							{
								args[it.get_key()] = value;
								value->add_ref();
							}
						}
					}
				}

				return expects_wrapper::unwrap_void(base.template_query(command, &args));
			}
			core::promise<network::mongo::document> mdb_stream_execute_with_reply(network::mongo::stream& base)
			{
				immediate_context* context = immediate_context::get();
				return base.execute_with_reply().then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<bool> mdb_stream_execute(network::mongo::stream& base)
			{
				immediate_context* context = immediate_context::get();
				return base.execute().then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}

			core::string mdb_cursor_error(network::mongo::cursor& base)
			{
				auto error = base.error();
				return error ? error->message() : core::string();
			}
			core::promise<bool> mdb_cursor_next(network::mongo::cursor& base)
			{
				immediate_context* context = immediate_context::get();
				return base.next().then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}

			core::promise<core::schema*> mdb_response_fetch(network::mongo::response& base)
			{
				immediate_context* context = immediate_context::get();
				return base.fetch().then<core::schema*>([context](network::mongo::expects_db<core::schema*>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), (core::schema*)nullptr, context);
				});
			}
			core::promise<core::schema*> mdb_response_fetch_all(network::mongo::response& base)
			{
				immediate_context* context = immediate_context::get();
				return base.fetch_all().then<core::schema*>([context](network::mongo::expects_db<core::schema*>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), (core::schema*)nullptr, context);
				});
			}
			core::promise<network::mongo::property> mdb_response_get_property(network::mongo::response& base, const std::string_view& name)
			{
				immediate_context* context = immediate_context::get();
				return base.get_property(name).then<network::mongo::property>([context](network::mongo::expects_db<network::mongo::property>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::property(), context);
				});
			}

			core::promise<bool> mdb_transaction_begin(network::mongo::transaction& base)
			{
				immediate_context* context = immediate_context::get();
				return base.begin().then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_transaction_rollback(network::mongo::transaction& base)
			{
				immediate_context* context = immediate_context::get();
				return base.rollback().then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<network::mongo::document> mdb_transaction_remove_many(network::mongo::transaction& base, network::mongo::collection& collection, const network::mongo::document& match, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.remove_many(collection, match, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_transaction_remove_one(network::mongo::transaction& base, network::mongo::collection& collection, const network::mongo::document& match, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.remove_one(collection, match, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_transaction_replace_one(network::mongo::transaction& base, network::mongo::collection& collection, const network::mongo::document& match, const network::mongo::document& replacement, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.replace_one(collection, match, replacement, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_transaction_insert_many(network::mongo::transaction& base, network::mongo::collection& collection, array* data, const network::mongo::document& options)
			{
				core::vector<network::mongo::document> list;
				if (data != nullptr)
				{
					size_t size = data->size();
					list.reserve(size);
					for (size_t i = 0; i < size; i++)
						list.emplace_back(((network::mongo::document*)data->at(i))->copy());
				}

				immediate_context* context = immediate_context::get();
				return base.insert_many(collection, list, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_transaction_insert_one(network::mongo::transaction& base, network::mongo::collection& collection, const network::mongo::document& result, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.insert_one(collection, result, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_transaction_update_many(network::mongo::transaction& base, network::mongo::collection& collection, const network::mongo::document& match, const network::mongo::document& update, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.update_many(collection, match, update, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_transaction_update_one(network::mongo::transaction& base, network::mongo::collection& collection, const network::mongo::document& match, const network::mongo::document& update, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.update_one(collection, match, update, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::cursor> mdb_transaction_find_many(network::mongo::transaction& base, network::mongo::collection& collection, const network::mongo::document& match, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.find_many(collection, match, options).then<network::mongo::cursor>([context](network::mongo::expects_db<network::mongo::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::cursor(nullptr), context);
				});
			}
			core::promise<network::mongo::cursor> mdb_transaction_find_one(network::mongo::transaction& base, network::mongo::collection& collection, const network::mongo::document& match, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.find_one(collection, match, options).then<network::mongo::cursor>([context](network::mongo::expects_db<network::mongo::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::cursor(nullptr), context);
				});
			}
			core::promise<network::mongo::cursor> mdb_transaction_aggregate(network::mongo::transaction& base, network::mongo::collection& collection, network::mongo::query_flags flags, const network::mongo::document& pipeline, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.aggregate(collection, flags, pipeline, options).then<network::mongo::cursor>([context](network::mongo::expects_db<network::mongo::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::cursor(nullptr), context);
				});
			}
			core::promise<network::mongo::response> mdb_transaction_template_query(network::mongo::transaction& base, network::mongo::collection& collection, const std::string_view& name, dictionary* data)
			{
				core::schema_args args;
				if (data != nullptr)
				{
					virtual_machine* vm = virtual_machine::get();
					if (vm != nullptr)
					{
						int type_id = vm->get_type_id_by_decl("schema@");
						args.reserve(data->size());

						for (auto it = data->begin(); it != data->end(); ++it)
						{
							core::schema* value = nullptr;
							if (it.get_value(&value, type_id))
							{
								args[it.get_key()] = value;
								value->add_ref();
							}
						}
					}
				}

				immediate_context* context = immediate_context::get();
				return base.template_query(collection, name, &args).then<network::mongo::response>([context](network::mongo::expects_db<network::mongo::response>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::response(), context);
				});
			}
			core::promise<network::mongo::response> mdb_transaction_query(network::mongo::transaction& base, network::mongo::collection& collection, const network::mongo::document& command)
			{
				immediate_context* context = immediate_context::get();
				return base.query(collection, command).then<network::mongo::response>([context](network::mongo::expects_db<network::mongo::response>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::response(), context);
				});
			}
			core::promise<network::mongo::transaction_state> mdb_transaction_commit(network::mongo::transaction& base)
			{
				immediate_context* context = immediate_context::get();
				return base.commit().then<network::mongo::transaction_state>([context](network::mongo::expects_db<network::mongo::transaction_state>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::transaction_state::fatal, context);
				});
			}

			core::promise<bool> mdb_collection_rename(network::mongo::collection& base, const std::string_view& database_name, const std::string_view& collection_name, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.rename(database_name, collection_name).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_collection_rename_with_options(network::mongo::collection& base, const std::string_view& database_name, const std::string_view& collection_name, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.rename_with_options(database_name, collection_name, options).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_collection_rename_with_remove(network::mongo::collection& base, const std::string_view& database_name, const std::string_view& collection_name, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.rename_with_remove(database_name, collection_name).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_collection_rename_with_options_and_remove(network::mongo::collection& base, const std::string_view& database_name, const std::string_view& collection_name, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.rename_with_options_and_remove(database_name, collection_name, options).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_collection_remove(network::mongo::collection& base, const std::string_view& name, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.remove(options).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_collection_remove_index(network::mongo::collection& base, const std::string_view& name, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.remove_index(name, options).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<network::mongo::document> mdb_collection_remove_many(network::mongo::collection& base, const network::mongo::document& match, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.remove_many(match, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_collection_remove_one(network::mongo::collection& base, const network::mongo::document& match, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.remove_one(match, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_collection_replace_one(network::mongo::collection& base, const network::mongo::document& match, const network::mongo::document& replacement, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.replace_one(match, replacement, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_collection_insert_many(network::mongo::collection& base, array* data, const network::mongo::document& options)
			{
				core::vector<network::mongo::document> list;
				if (data != nullptr)
				{
					size_t size = data->size();
					list.reserve(size);
					for (size_t i = 0; i < size; i++)
						list.emplace_back(((network::mongo::document*)data->at(i))->copy());
				}

				immediate_context* context = immediate_context::get();
				return base.insert_many(list, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_collection_insert_one(network::mongo::collection& base, const network::mongo::document& result, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.insert_one(result, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_collection_update_many(network::mongo::collection& base, const network::mongo::document& match, const network::mongo::document& update, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.update_many(match, update, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_collection_update_one(network::mongo::collection& base, const network::mongo::document& match, const network::mongo::document& update, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.update_one(match, update, options).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::document> mdb_collection_find_and_modify(network::mongo::collection& base, const network::mongo::document& match, const network::mongo::document& sort, const network::mongo::document& update, const network::mongo::document& fields, bool remove, bool upsert, bool init)
			{
				immediate_context* context = immediate_context::get();
				return base.find_and_modify(match, sort, update, fields, remove, upsert, init).then<network::mongo::document>([context](network::mongo::expects_db<network::mongo::document>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::document(nullptr), context);
				});
			}
			core::promise<network::mongo::cursor> mdb_collection_find_many(network::mongo::collection& base, const network::mongo::document& match, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.find_many(match, options).then<network::mongo::cursor>([context](network::mongo::expects_db<network::mongo::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::cursor(nullptr), context);
				});
			}
			core::promise<network::mongo::cursor> mdb_collection_find_one(network::mongo::collection& base, const network::mongo::document& match, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.find_one(match, options).then<network::mongo::cursor>([context](network::mongo::expects_db<network::mongo::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::cursor(nullptr), context);
				});
			}
			core::promise<network::mongo::cursor> mdb_collection_find_indexes(network::mongo::collection& base, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.find_indexes(options).then<network::mongo::cursor>([context](network::mongo::expects_db<network::mongo::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::cursor(), context);
				});
			}
			core::promise<network::mongo::cursor> mdb_collection_aggregate(network::mongo::collection& base, network::mongo::query_flags flags, const network::mongo::document& pipeline, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.aggregate(flags, pipeline, options).then<network::mongo::cursor>([context](network::mongo::expects_db<network::mongo::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::cursor(), context);
				});
			}
			core::promise<network::mongo::response> mdb_collection_template_query(network::mongo::collection& base, const std::string_view& name, dictionary* data, network::mongo::transaction& session)
			{
				core::schema_args args;
				if (data != nullptr)
				{
					virtual_machine* vm = virtual_machine::get();
					if (vm != nullptr)
					{
						int type_id = vm->get_type_id_by_decl("schema@");
						args.reserve(data->size());

						for (auto it = data->begin(); it != data->end(); ++it)
						{
							core::schema* value = nullptr;
							if (it.get_value(&value, type_id))
							{
								args[it.get_key()] = value;
								value->add_ref();
							}
						}
					}
				}

				immediate_context* context = immediate_context::get();
				return base.template_query(name, &args, session).then<network::mongo::response>([context](network::mongo::expects_db<network::mongo::response>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::response(), context);
				});
			}
			core::promise<network::mongo::response> mdb_collection_query(network::mongo::collection& base, const network::mongo::document& command, network::mongo::transaction& session)
			{
				immediate_context* context = immediate_context::get();
				return base.query(command, session).then<network::mongo::response>([context](network::mongo::expects_db<network::mongo::response>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::response(), context);
				});
			}

			core::promise<bool> mdb_database_remove_all_users(network::mongo::database& base, const std::string_view& name)
			{
				immediate_context* context = immediate_context::get();
				return base.remove_all_users().then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_database_remove_user(network::mongo::database& base, const std::string_view& name)
			{
				immediate_context* context = immediate_context::get();
				return base.remove_user(name).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_database_remove(network::mongo::database& base)
			{
				immediate_context* context = immediate_context::get();
				return base.remove().then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_database_remove_with_options(network::mongo::database& base, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.remove_with_options(options).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_database_add_user(network::mongo::database& base, const std::string_view& username, const std::string_view& password, const network::mongo::document& roles, const network::mongo::document& custom)
			{
				immediate_context* context = immediate_context::get();
				return base.add_user(username, password, roles, custom).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_database_has_collection(network::mongo::database& base, const std::string_view& name)
			{
				return base.has_collection(name).then<bool>([](network::mongo::expects_db<void>&& result) { return !!result; });
			}
			core::promise<network::mongo::collection> mdb_database_create_collection(network::mongo::database& base, const std::string_view& name, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.create_collection(name, options).then<network::mongo::collection>([context](network::mongo::expects_db<network::mongo::collection>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::collection(nullptr), context);
				});
			}
			core::promise<network::mongo::cursor> mdb_database_find_collections(network::mongo::database& base, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base.find_collections(options).then<network::mongo::cursor>([context](network::mongo::expects_db<network::mongo::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::cursor(nullptr), context);
				});
			}
			array* mdb_database_get_collection_names(network::mongo::database& base, const network::mongo::document& options)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose<core::string>(type, expects_wrapper::unwrap(base.get_collection_names(options), core::vector<core::string>()));
			}

			core::promise<bool> mdb_watcher_next(network::mongo::watcher& base, network::mongo::document& result)
			{
				immediate_context* context = immediate_context::get();
				return base.next(result).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_watcher_error(network::mongo::watcher& base, network::mongo::document& result)
			{
				immediate_context* context = immediate_context::get();
				return base.error(result).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}

			array* mdb_connection_get_database_names(network::mongo::connection* base, const network::mongo::document& options)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose<core::string>(type, expects_wrapper::unwrap(base->get_database_names(options), core::vector<core::string>()));
			}
			core::promise<bool> mdb_connection_connect(network::mongo::connection* base, network::mongo::address* address)
			{
				immediate_context* context = immediate_context::get();
				return base->connect(address).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_connection_disconnect(network::mongo::connection* base)
			{
				immediate_context* context = immediate_context::get();
				return base->disconnect().then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_connection_make_transaction(network::mongo::connection* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (!delegatef.is_valid())
					return core::promise<bool>(false);

				immediate_context* context = immediate_context::get();
				return base->make_transaction([base, delegatef](network::mongo::transaction* session) mutable -> core::promise<bool>
				{
					core::promise<bool> future;
					delegatef([base, session](immediate_context* context)
					{
						context->set_arg_object(0, (void*)&base);
						context->set_arg_object(1, (void*)session);
					}, [future](immediate_context* context) mutable
					{
						promise* target = context->get_return_object<promise>();
						if (!target)
							return future.set(true);

						target->when([future](promise* target) mutable
						{
							bool value = true;
							target->retrieve(&value, (int)type_id::bool_t);
							future.set(value);
						});
					});
					return future;
				}).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<network::mongo::cursor> mdb_connection_find_databases(network::mongo::connection* base, const network::mongo::document& options)
			{
				immediate_context* context = immediate_context::get();
				return base->find_databases(options).then<network::mongo::cursor>([context](network::mongo::expects_db<network::mongo::cursor>&& result)
				{
					return expects_wrapper::unwrap(std::move(result), network::mongo::cursor(nullptr), context);
				});
			}

			void mdb_cluster_push(network::mongo::cluster* base, network::mongo::connection* target)
			{
				base->push(&target);
			}
			core::promise<bool> mdb_cluster_connect(network::mongo::cluster* base, network::mongo::address* address)
			{
				immediate_context* context = immediate_context::get();
				return base->connect(address).then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}
			core::promise<bool> mdb_cluster_disconnect(network::mongo::cluster* base)
			{
				immediate_context* context = immediate_context::get();
				return base->disconnect().then<bool>([context](network::mongo::expects_db<void>&& result)
				{
					return expects_wrapper::unwrap_void(std::move(result), context);
				});
			}

			void mdb_driver_set_query_log(network::mongo::driver* base, asIScriptFunction* callback)
			{
				function_delegate delegatef(callback);
				if (delegatef.is_valid())
				{
					base->set_query_log([delegatef](const std::string_view& data) mutable
					{
						core::string copy = core::string(data);
						delegatef([copy](immediate_context* context)
						{
							context->set_arg_object(0, (void*)&copy);
						});
					});
				}
				else
					base->set_query_log(nullptr);
			}
			core::string mdb_driver_get_query(network::pq::driver* base, network::pq::cluster* cluster, const std::string_view& SQL, dictionary* data)
			{
				core::schema_args args;
				if (data != nullptr)
				{
					virtual_machine* vm = virtual_machine::get();
					if (vm != nullptr)
					{
						int type_id = vm->get_type_id_by_decl("schema@");
						args.reserve(data->size());

						for (auto it = data->begin(); it != data->end(); ++it)
						{
							core::schema* value = nullptr;
							if (it.get_value(&value, type_id))
							{
								args[it.get_key()] = value;
								value->add_ref();
							}
						}
					}
				}

				return expects_wrapper::unwrap(base->get_query(cluster, SQL, &args), core::string());
			}
			array* mdb_driver_get_queries(network::mongo::driver* base)
			{
				virtual_machine* vm = virtual_machine::get();
				if (!vm)
					return nullptr;

				type_info type = vm->get_type_info_by_decl(TYPENAME_ARRAY "<" TYPENAME_STRING ">@");
				return array::compose(type.get_type_info(), base->get_queries());
			}
#endif
			void* registry::fetch_string_factory() noexcept
			{
#ifdef VI_ANGELSCRIPT
				return string_factory::get();
#else
				return nullptr;
#endif
			}
			bool registry::cleanup() noexcept
			{
#ifdef VI_ANGELSCRIPT
				string_factory::free();
#endif
				return true;
			}
			bool registry::bind_addons(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");
				vm->add_system_addon("ctypes", { }, &import_ctypes);
				vm->add_system_addon("any", { }, &import_any);
				vm->add_system_addon("array", { "ctypes" }, &import_array);
				vm->add_system_addon("complex", { }, &import_complex);
				vm->add_system_addon("math", { }, &import_math);
				vm->add_system_addon("string", { "array" }, &import_string);
				vm->add_system_addon("random", { "string" }, &import_random);
				vm->add_system_addon("dictionary", { "array", "string" }, &import_dictionary);
				vm->add_system_addon("exception", { "string" }, &import_exception);
				vm->add_system_addon("mutex", { }, &import_mutex);
				vm->add_system_addon("thread", { "any", "string" }, &import_thread);
				vm->add_system_addon("buffers", { "string" }, &import_buffers);
				vm->add_system_addon("promise", { "exception" }, &import_promise);
				vm->add_system_addon("decimal", { "string" }, &import_decimal);
				vm->add_system_addon("uint128", { "decimal" }, &import_uint128);
				vm->add_system_addon("uint256", { "uint128" }, &import_uint256);
				vm->add_system_addon("variant", { "string", "decimal" }, &import_variant);
				vm->add_system_addon("timestamp", { "string" }, &import_timestamp);
				vm->add_system_addon("console", { "string", "schema" }, &import_console);
				vm->add_system_addon("schema", { "array", "string", "dictionary", "variant" }, &import_schema);
				vm->add_system_addon("schedule", { "ctypes" }, &import_schedule);
				vm->add_system_addon("clock", { }, &import_clock_timer);
				vm->add_system_addon("fs", { "string" }, &import_file_system);
				vm->add_system_addon("os", { "fs", "array", "dictionary" }, &import_os);
				vm->add_system_addon("regex", { "string" }, &import_regex);
				vm->add_system_addon("crypto", { "string", "schema" }, &import_crypto);
				vm->add_system_addon("codec", { "string" }, &import_codec);
				vm->add_system_addon("preprocessor", { "string" }, &import_preprocessor);
				vm->add_system_addon("network", { "string", "array", "dictionary", "promise" }, &import_network);
				vm->add_system_addon("http", { "schema", "fs", "promise", "regex", "network" }, &import_http);
				vm->add_system_addon("smtp", { "promise", "network" }, &import_smtp);
				vm->add_system_addon("sqlite", { "network", "schema" }, &import_sqlite);
				vm->add_system_addon("pq", { "network", "schema" }, &import_pq);
				vm->add_system_addon("mongo", { "network", "schema" }, &import_mongo);
				vm->add_system_addon("vm", { }, &import_vm);
				vm->add_system_addon("layer", { "schema", "schedule", "fs", "clock" }, &import_layer);
				return true;
			}
			bool registry::bind_stringifiers(debugger_context* context) noexcept
			{
				VI_ASSERT(context != nullptr, "context should be set");
				context->add_to_string_callback("string", [](core::string& indent, int depth, void* object, int type_id)
				{
					core::string& source = *(core::string*)object;
					core::string_stream stream;
					stream << "\"" << source << "\"";
					stream << " (string, " << source.size() << " chars)";
					return stream.str();
				});
				context->add_to_string_callback("string_view", [](core::string& indent, int depth, void* object, int type_id)
				{
					std::string_view& source = *(std::string_view*)object;
					core::string_stream stream;
					stream << "\"" << source << "\"";
					stream << " (string_view, " << source.size() << " chars)";
					return stream.str();
				});
				context->add_to_string_callback("decimal", [](core::string& indent, int depth, void* object, int type_id)
				{
					core::decimal& source = *(core::decimal*)object;
					return source.to_string() + " (decimal)";
				});
				context->add_to_string_callback("uint128", [](core::string& indent, int depth, void* object, int type_id)
				{
					compute::uint128& source = *(compute::uint128*)object;
					return source.to_string() + " (uint128)";
				});
				context->add_to_string_callback("uint256", [](core::string& indent, int depth, void* object, int type_id)
				{
					compute::uint256& source = *(compute::uint256*)object;
					return source.to_string() + " (uint256)";
				});
				context->add_to_string_callback("variant", [](core::string& indent, int depth, void* object, int type_id)
				{
					core::variant& source = *(core::variant*)object;
					return "\"" + source.serialize() + "\" (variant)";
				});
				context->add_to_string_callback("any", [context](core::string& indent, int depth, void* object, int type_id)
				{
					bindings::any* source = (bindings::any*)object;
					return context->to_string(indent, depth - 1, source->get_address_of_object(), source->get_type_id());
				});
				context->add_to_string_callback("promise, promise_v", [context](core::string& indent, int depth, void* object, int type_id)
				{
					bindings::promise* source = (bindings::promise*)object;
					core::string_stream stream;
					stream << "(promise<t>)\n";
					stream << indent << "  state = " << (source->is_pending() ? "pending\n" : "fulfilled\n");
					stream << indent << "  data = " << context->to_string(indent, depth - 1, source->get_address_of_object(), source->get_type_id());
					return stream.str();
				});
				context->add_to_string_callback("array", [context](core::string& indent, int depth, void* object, int type_id)
				{
					bindings::array* source = (bindings::array*)object;
					int base_type_id = source->get_element_type_id();
					size_t size = source->size();
					core::string_stream stream;
					stream << "0x" << (void*)source << " (array<t>, " << size << " elements)";

					if (!depth || !size)
						return stream.str();

					if (size > 128)
					{
						stream << "\n";
						indent.append("  ");
						for (size_t i = 0; i < size; i++)
						{
							stream << indent << "[" << i << "]: " << context->to_string(indent, depth - 1, source->at(i), base_type_id);
							if (i + 1 < size)
								stream << "\n";
						}
						indent.erase(indent.end() - 2, indent.end());
					}
					else
					{
						stream << " [";
						for (size_t i = 0; i < size; i++)
						{
							stream << context->to_string(indent, depth - 1, source->at(i), base_type_id);
							if (i + 1 < size)
								stream << ", ";
						}
						stream << "]";
					}

					return stream.str();
				});
				context->add_to_string_callback("dictionary", [context](core::string& indent, int depth, void* object, int type_id)
				{
					bindings::dictionary* source = (bindings::dictionary*)object;
					size_t size = source->size();
					core::string_stream stream;
					stream << "0x" << (void*)source << " (dictionary, " << size << " elements)";

					if (!depth || !size)
						return stream.str();

					stream << "\n";
					indent.append("  ");
					for (size_t i = 0; i < size; i++)
					{
						core::string name; void* value; int type_id;
						if (!source->get_index(i, &name, &value, &type_id))
							continue;

						type_info type = context->get_engine()->get_type_info_by_id(type_id);
						core::string type_name = core::string(type.is_valid() ? type.get_name() : "?");
						stream << indent << core::stringify::trim_end(type_name) << " \"" << name << "\": " << context->to_string(indent, depth - 1, value, type_id);
						if (i + 1 < size)
							stream << "\n";
					}

					indent.erase(indent.end() - 2, indent.end());
					return stream.str();
				});
				context->add_to_string_callback("schema", [](core::string& indent, int depth, void* object, int type_id)
				{
					core::string_stream stream;
					core::schema* source = (core::schema*)object;
					if (source->value.is_object())
						stream << "0x" << (void*)source << "(schema)\n";

					core::schema::convert_to_json(source, [&indent, &stream](core::var_form type, const std::string_view& buffer)
					{
						if (!buffer.empty())
							stream << buffer;

						switch (type)
						{
							case core::var_form::tab_decrease:
								indent.erase(indent.end() - 2, indent.end());
								break;
							case core::var_form::tab_increase:
								indent.append("  ");
								break;
							case core::var_form::write_space:
								stream << " ";
								break;
							case core::var_form::write_line:
								stream << "\n";
								break;
							case core::var_form::write_tab:
								stream << indent;
								break;
							default:
								break;
						}
					});
					return stream.str();
				});
#ifdef VI_BINDINGS
				context->add_to_string_callback("thread", [](core::string& indent, int depth, void* object, int type_id)
				{
					bindings::thread* source = (bindings::thread*)object;
					core::string_stream stream;
					stream << "(thread)\n";
					stream << indent << "  id = " << source->get_id() << "\n";
					stream << indent << "  state = " << (source->is_active() ? "active" : "suspended");
					return stream.str();
				});
				context->add_to_string_callback("char_buffer", [](core::string& indent, int depth, void* object, int type_id)
				{
					bindings::char_buffer* source = (bindings::char_buffer*)object;
					size_t size = source->size();

					core::string_stream stream;
					stream << "0x" << (void*)source << " (char_buffer, " << size << " bytes) [";

					char* buffer = (char*)source->get_pointer(0);
					size_t count = (size > 256 ? 256 : size);
					core::string next = "0";

					for (size_t i = 0; i < count; i++)
					{
						next[0] = buffer[i];
						stream << "0x" << compute::codec::hex_encode(next);
						if (i + 1 < count)
							stream << ", ";
					}

					if (count < size)
						stream << ", ...";

					stream << "]";
					return stream.str();
				});
#endif
				return true;
			}
			bool registry::import_ctypes(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");
#ifdef VI_64
				vm->set_type_def("usize", "uint64");
#else
				vm->set_type_def("usize", "uint32");
#endif
				auto vpointer = vm->set_class_address("uptr", 0, (uint64_t)object_behaviours::ref | (uint64_t)object_behaviours::nocount);
				bool has_pointer_cast = (!vm->get_library_property(library_features::ctypes_no_pointer_cast));
				if (has_pointer_cast)
				{
					vpointer->set_method_extern("void opCast(?&out)", &pointer_to_handle_cast);
					vm->set_function("uptr@ to_ptr(?&in)", &handle_to_pointer_cast);
				}

				return true;
			}
			bool registry::import_any(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");

				auto vany = vm->set_class<any>("any", true);
				vany->set_constructor_extern("any@ f()", &any::factory1);
				vany->set_constructor_extern("any@ f(?&in) explicit", &any::factory2);
				vany->set_constructor_extern("any@ f(const int64&in) explicit", &any::factory2);
				vany->set_constructor_extern("any@ f(const double&in) explicit", &any::factory2);
				vany->set_enum_refs(&any::enum_references);
				vany->set_release_refs(&any::release_references);
				vany->set_method_extern("any &opAssign(any&in)", &any::assignment);
				vany->set_method("void store(?&in)", &any::store);
				vany->set_method("bool retrieve(?&out)", &any::retrieve);

				return true;
			}
			bool registry::import_array(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");
				vm->set_type_info_user_data_cleanup_callback(array::cleanup_type_info_cache, (size_t)array::get_id());

				auto list_factory_call = bridge::function_call(&array::factory);
				{
					auto varray = vm->set_template_class<array>("array<class t>", "array<t>", true);
					varray->set_template_callback(&array::template_callback);
					varray->set_function_def("bool array<t>::less_sync(const t&in a, const t&in b)");
					varray->set_constructor_extern<array, asITypeInfo*>("array<t>@ f(int&in)", &array::create);
					varray->set_constructor_extern<array, asITypeInfo*, size_t>("array<t>@ f(int&in, usize) explicit", &array::create);
					varray->set_constructor_extern<array, asITypeInfo*, size_t, void*>("array<t>@ f(int&in, usize, const t&in)", &array::create);
					varray->set_behaviour_address("array<t>@ f(int&in, int&in) {repeat t}", behaviours::list_factory, list_factory_call, convention::cdecl_call);
					varray->set_enum_refs(&array::enum_references);
					varray->set_release_refs(&array::release_references);
					varray->set_method("bool opEquals(const array<t>&in) const", &array::operator==);
					varray->set_method("array<t>& opAssign(const array<t>&in)", &array::operator=);
					varray->set_method<array, void*, size_t>("t& opIndex(usize)", &array::at);
					varray->set_method<array, const void*, size_t>("const t& opIndex(usize) const", &array::at);
					varray->set_method<array, void*>("t& front()", &array::front);
					varray->set_method<array, const void*>("const t& front() const", &array::front);
					varray->set_method<array, void*>("t& back()", &array::back);
					varray->set_method<array, const void*>("const t& back() const", &array::back);
					varray->set_method("bool empty() const", &array::empty);
					varray->set_method("usize size() const", &array::size);
					varray->set_method("usize capacity() const", &array::capacity);
					varray->set_method("void reserve(usize)", &array::reserve);
					varray->set_method<array, void, size_t>("void resize(usize)", &array::resize);
					varray->set_method("void clear()", &array::clear);
					varray->set_method("void push(const t&in)", &array::insert_last);
					varray->set_method("void pop()", &array::remove_last);
					varray->set_method<array, void, size_t, void*>("void insert(usize, const t&in)", &array::insert_at);
					varray->set_method<array, void, size_t, const array&>("void insert(usize, const array<t>&)", &array::insert_at);
					varray->set_method("void erase_if(const t&in if_handle_then_const, usize = 0)", &array::remove_if);
					varray->set_method("void erase(usize)", &array::remove_at);
					varray->set_method("void erase(usize, usize)", &array::remove_range);
					varray->set_method("void reverse()", &array::reverse);
					varray->set_method("void swap(usize, usize)", &array::swap);
					varray->set_method("void sort(less_sync@ = null)", &array::sort);
					varray->set_method<array, size_t, void*, size_t>("usize find(const t&in if_handle_then_const, usize = 0) const", &array::find);
					varray->set_method<array, size_t, void*, size_t>("usize find_ref(const t&in if_handle_then_const, usize = 0) const", &array::find_by_ref);
				}
				function_factory::release_functor(&list_factory_call);

				vm->set_default_array_type("array<t>");
				vm->begin_namespace("array");
				vm->set_property("const usize npos", &core::string::npos);
				vm->end_namespace();

				return true;
			}
			bool registry::import_complex(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");
				auto list_constructor_call = bridge::function_call(&complex::list_constructor);
				{
					auto vcomplex = vm->set_struct_address("complex", sizeof(complex), (size_t)object_behaviours::value | (size_t)object_behaviours::pod | bridge::type_traits_of<complex>() | (size_t)object_behaviours::app_class_allfloats);
					vcomplex->set_property("float R", &complex::r);
					vcomplex->set_property("float I", &complex::i);
					vcomplex->set_constructor_extern("void f()", &complex::default_constructor);
					vcomplex->set_constructor_extern("void f(const complex &in)", &complex::copy_constructor);
					vcomplex->set_constructor_extern("void f(float)", &complex::conv_constructor);
					vcomplex->set_constructor_extern("void f(float, float)", &complex::init_constructor);
					vcomplex->set_behaviour_address("void f(const int &in) {float, float}", behaviours::list_construct, list_constructor_call, convention::cdecl_call_obj_first);
					vcomplex->set_method("complex &opAddAssign(const complex &in)", &complex::operator+=);
					vcomplex->set_method("complex &opSubAssign(const complex &in)", &complex::operator-=);
					vcomplex->set_method("complex &opMulAssign(const complex &in)", &complex::operator*=);
					vcomplex->set_method("complex &opDivAssign(const complex &in)", &complex::operator/=);
					vcomplex->set_method("bool opEquals(const complex &in) const", &complex::operator==);
					vcomplex->set_method("complex opAdd(const complex &in) const", &complex::operator+);
					vcomplex->set_method("complex opSub(const complex &in) const", &complex::operator-);
					vcomplex->set_method("complex opMul(const complex &in) const", &complex::operator*);
					vcomplex->set_method("complex opDiv(const complex &in) const", &complex::operator/);
					vcomplex->set_method("float abs() const", &complex::length);
					vcomplex->set_method("complex get_ri() const property", &complex::get_ri);
					vcomplex->set_method("complex get_ir() const property", &complex::get_ir);
					vcomplex->set_method("void set_ri(const complex &in) property", &complex::set_ri);
					vcomplex->set_method("void set_ir(const complex &in) property", &complex::set_ir);
				}
				function_factory::release_functor(&list_constructor_call);
				return true;
#else
				VI_ASSERT(false, "<complex> is not loaded");
				return false;
#endif
			}
			bool registry::import_dictionary(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");

				auto vstorable = vm->set_struct_address("storable", sizeof(storable), (size_t)object_behaviours::value | (size_t)object_behaviours::as_handle | (size_t)object_behaviours::gc | bridge::type_traits_of<storable>());
				vstorable->set_constructor_extern("void f()", &dictionary::key_create);
				vstorable->set_destructor_extern("void f()", &dictionary::key_destroy);
				vstorable->set_enum_refs(&storable::enum_references);
				vstorable->set_release_refs(&storable::release_references);
				vstorable->set_method_extern<storable&, storable*, const storable&>("storable &opAssign(const storable&in)", &dictionary::keyop_assign);
				vstorable->set_method_extern<storable&, storable*, void*, int>("storable &opHndlAssign(const ?&in)", &dictionary::keyop_assign);
				vstorable->set_method_extern<storable&, storable*, const storable&>("storable &opHndlAssign(const storable&in)", &dictionary::keyop_assign);
				vstorable->set_method_extern<storable&, storable*, void*, int>("storable &opAssign(const ?&in)", &dictionary::keyop_assign);
				vstorable->set_method_extern<storable&, storable*, double>("storable &opAssign(double)", &dictionary::keyop_assign);
				vstorable->set_method_extern<storable&, storable*, as_int64_t>("storable &opAssign(int64)", &dictionary::keyop_assign);
				vstorable->set_method_extern("void opCast(?&out)", &dictionary::keyop_cast);
				vstorable->set_method_extern("void opConv(?&out)", &dictionary::keyop_cast);
				vstorable->set_method_extern("int64 opConv()", &dictionary::keyop_conv_int);
				vstorable->set_method_extern("double opConv()", &dictionary::keyop_conv_double);

				auto factory_call = bridge::function_generic_call(&dictionary::factory);
				auto list_factory_call = bridge::function_generic_call(&dictionary::list_factory);
				{
					auto vdictionary = vm->set_class<dictionary>("dictionary", true);
					vdictionary->set_behaviour_address("dictionary@ f()", behaviours::factory, factory_call, convention::generic_call);
					vdictionary->set_behaviour_address("dictionary @f(int &in) {repeat {string, ?}}", behaviours::list_factory, list_factory_call, convention::generic_call);
					vdictionary->set_enum_refs(&dictionary::enum_references);
					vdictionary->set_release_refs(&dictionary::release_references);
					vdictionary->set_method("dictionary &opAssign(const dictionary &in)", &dictionary::operator=);
					vdictionary->set_method<dictionary, storable*, const std::string_view&>("storable& opIndex(const string_view&in)", &dictionary::operator[]);
					vdictionary->set_method<dictionary, const storable*, const std::string_view&>("const storable& opIndex(const string_view&in) const", &dictionary::operator[]);
					vdictionary->set_method<dictionary, storable*, size_t>("storable& opIndex(usize)", &dictionary::operator[]);
					vdictionary->set_method<dictionary, const storable*, size_t>("const storable& opIndex(usize) const", &dictionary::operator[]);
					vdictionary->set_method("void set(const string_view&in, const ?&in)", &dictionary::set);
					vdictionary->set_method("bool get(const string_view&in, ?&out) const", &dictionary::get);
					vdictionary->set_method("bool exists(const string_view&in) const", &dictionary::exists);
					vdictionary->set_method("bool empty() const", &dictionary::empty);
					vdictionary->set_method("bool at(usize, string&out, ?&out) const", &dictionary::try_get_index);
					vdictionary->set_method("usize size() const", &dictionary::size);
					vdictionary->set_method("bool erase(const string_view&in)", &dictionary::erase);
					vdictionary->set_method("void clear()", &dictionary::clear);
					vdictionary->set_method("array<string>@ get_keys() const", &dictionary::get_keys);
					dictionary::setup(vm);
				}
				function_factory::release_functor(&factory_call);
				function_factory::release_functor(&list_factory_call);
				return true;
			}
			bool registry::import_math(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");
				vm->set_function<float(*)(uint32_t)>("float fp_from_ieee(uint)", &math::fp_from_ieee);
				vm->set_function<uint32_t(*)(float)>("uint fp_to_ieee(float)", &math::fp_to_ieee);
				vm->set_function<double(*)(as_uint64_t)>("double fp_from_ieee(uint64)", &math::fp_from_ieee);
				vm->set_function<as_uint64_t(*)(double)>("uint64 fp_to_ieee(double)", &math::fp_to_ieee);
				vm->set_function<bool(*)(float, float, float)>("bool close_to(float, float, float = 0.00001f)", &math::close_to);
				vm->set_function<bool(*)(double, double, double)>("bool close_to(double, double, double = 0.0000000001)", &math::close_to);
				vm->set_function("float rad2degf()", &compute::mathf::rad2deg);
				vm->set_function("float deg2radf()", &compute::mathf::deg2rad);
				vm->set_function("float pi_valuef()", &compute::mathf::pi);
				vm->set_function("float clampf(float, float, float)", &compute::mathf::clamp);
				vm->set_function("float acotanf(float)", &compute::mathf::acotan);
				vm->set_function("float cotanf(float)", &compute::mathf::cotan);
				vm->set_function("float minf(float, float)", &compute::mathf::min);
				vm->set_function("float maxf(float, float)", &compute::mathf::max);
				vm->set_function("float saturatef(float)", &compute::mathf::saturate);
				vm->set_function("float lerpf(float, float, float)", &compute::mathf::lerp);
				vm->set_function("float strong_lerpf(float, float, float)", &compute::mathf::strong_lerp);
				vm->set_function("float angle_saturatef(float)", &compute::mathf::saturate_angle);
				vm->set_function<float(*)()>("float randomf()", &compute::mathf::random);
				vm->set_function("float random_magf()", &compute::mathf::random_mag);
				vm->set_function<float(*)(float, float)>("float random_rangef(float, float)", &compute::mathf::random);
				vm->set_function<float(*)(float, float, float, float, float)>("float mapf(float, float, float, float, float)", &compute::mathf::map);
				vm->set_function("float cosf(float)", &compute::mathf::cos);
				vm->set_function("float sinf(float)", &compute::mathf::sin);
				vm->set_function("float tanf(float)", &compute::mathf::tan);
				vm->set_function("float acosf(float)", &compute::mathf::acos);
				vm->set_function("float asinf(float)", &compute::mathf::asin);
				vm->set_function("float atanf(float)", &compute::mathf::atan);
				vm->set_function("float atan2f(float, float)", &compute::mathf::atan2);
				vm->set_function("float expf(float)", &compute::mathf::exp);
				vm->set_function("float logf(float)", &compute::mathf::log);
				vm->set_function("float log2f(float)", &compute::mathf::log2);
				vm->set_function("float log10f(float)", &compute::mathf::log10);
				vm->set_function("float powf(float, float)", &compute::mathf::pow);
				vm->set_function("float sqrtf(float)", &compute::mathf::sqrt);
				vm->set_function("float ceilf(float)", &compute::mathf::ceil);
				vm->set_function("float absf(float)", &compute::mathf::abs);
				vm->set_function("float floorf(float)", &compute::mathf::floor);
				vm->set_function("double rad2degd()", &compute::mathd::rad2deg);
				vm->set_function("double deg2radd()", &compute::mathd::deg2rad);
				vm->set_function("double pi_valued()", &compute::mathd::pi);
				vm->set_function("double clampd(double, double, double)", &compute::mathd::clamp);
				vm->set_function("double acotand(double)", &compute::mathd::acotan);
				vm->set_function("double cotand(double)", &compute::mathd::cotan);
				vm->set_function("double mind(double, double)", &compute::mathd::min);
				vm->set_function("double maxd(double, double)", &compute::mathd::max);
				vm->set_function("double saturated(double)", &compute::mathd::saturate);
				vm->set_function("double lerpd(double, double, double)", &compute::mathd::lerp);
				vm->set_function("double strong_lerpd(double, double, double)", &compute::mathd::strong_lerp);
				vm->set_function("double angle_saturated(double)", &compute::mathd::saturate_angle);
				vm->set_function<double(*)()>("double randomd()", &compute::mathd::random);
				vm->set_function("double random_magd()", &compute::mathd::random_mag);
				vm->set_function<double(*)(double, double)>("double random_ranged(double, double)", &compute::mathd::random);
				vm->set_function("double mapd(double, double, double, double, double)", &compute::mathd::map);
				vm->set_function("double cosd(double)", &compute::mathd::cos);
				vm->set_function("double sind(double)", &compute::mathd::sin);
				vm->set_function("double tand(double)", &compute::mathd::tan);
				vm->set_function("double acosd(double)", &compute::mathd::acos);
				vm->set_function("double asind(double)", &compute::mathd::asin);
				vm->set_function("double atand(double)", &compute::mathd::atan);
				vm->set_function("double atan2d(double, double)", &compute::mathd::atan2);
				vm->set_function("double expd(double)", &compute::mathd::exp);
				vm->set_function("double logd(double)", &compute::mathd::log);
				vm->set_function("double log2d(double)", &compute::mathd::log2);
				vm->set_function("double log10d(double)", &compute::mathd::log10);
				vm->set_function("double powd(double, double)", &compute::mathd::pow);
				vm->set_function("double sqrtd(double)", &compute::mathd::sqrt);
				vm->set_function("double ceild(double)", &compute::mathd::ceil);
				vm->set_function("double absd(double)", &compute::mathd::abs);
				vm->set_function("double floord(double)", &compute::mathd::floor);

				return true;
			}
			bool registry::import_string(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");
				auto vstring_view = vm->set_struct_address("string_view", sizeof(std::string_view), (size_t)object_behaviours::value | bridge::type_traits_of<std::string_view>() | (size_t)object_behaviours::app_class_allints);
				auto vstring = vm->set_struct_address("string", sizeof(core::string), (size_t)object_behaviours::value | bridge::type_traits_of<core::string>());
#ifdef VI_ANGELSCRIPT
				vm->set_string_factory_address("string", string_factory::get());
#endif
				vstring->set_constructor_extern("void f()", &string::create);
				vstring->set_constructor_extern("void f(const string&in)", &string::create_copy1);
				vstring->set_constructor_extern("void f(const string_view&in)", &string::create_copy2);
				vstring->set_destructor_extern("void f()", &string::destroy);
				vstring->set_method<core::string, core::string&, const core::string&>("string& opAssign(const string&in)", &core::string::operator=);
				vstring->set_method<core::string, core::string&, const std::string_view&>("string& opAssign(const string_view&in)", &core::string::operator=);
				vstring->set_method<core::string, core::string&, const core::string&>("string& opAddAssign(const string&in)", &core::string::operator+=);
				vstring->set_method<core::string, core::string&, const std::string_view&>("string& opAddAssign(const string_view&in)", &core::string::operator+=);
				vstring->set_method_extern<core::string, const core::string&, const core::string&>("string opAdd(const string&in) const", &std::operator+);
				vstring->set_method<core::string, core::string&, char>("string& opAddAssign(uint8)", &core::string::operator+=);
				vstring->set_method_extern<core::string, const core::string&, char>("string opAdd(uint8) const", &std::operator+);
				vstring->set_method_extern<core::string, char, const core::string&>("string opAdd_r(uint8) const", &std::operator+);
				vstring->set_method<core::string, int, const core::string&>("int opCmp(const string&in) const", &core::string::compare);
				vstring->set_method<core::string, int, const std::string_view&>("int opCmp(const string_view&in) const", &core::string::compare);
				vstring->set_method_extern("uint8& opIndex(usize)", &string::index);
				vstring->set_method_extern("const uint8& opIndex(usize) const", &string::index);
				vstring->set_method_extern("uint8& at(usize)", &string::index);
				vstring->set_method_extern("const uint8& at(usize) const", &string::index);
				vstring->set_method_extern("uint8& front()", &string::front);
				vstring->set_method_extern("const uint8& front() const", &string::front);
				vstring->set_method_extern("uint8& back()", &string::back);
				vstring->set_method_extern("const uint8& back() const", &string::back);
				vstring->set_method("uptr@ data() const", &core::string::c_str);
				vstring->set_method("uptr@ c_str() const", &core::string::c_str);
				vstring->set_method("bool empty() const", &core::string::empty);
				vstring->set_method("usize size() const", &core::string::size);
				vstring->set_method("usize max_size() const", &core::string::max_size);
				vstring->set_method("usize capacity() const", &core::string::capacity);
				vstring->set_method<core::string, void, size_t>("void reserve(usize)", &core::string::reserve);
				vstring->set_method<core::string, void, size_t, char>("void resize(usize, uint8 = 0)", &core::string::resize);
				vstring->set_method("void shrink_to_fit()", &core::string::shrink_to_fit);
				vstring->set_method("void clear()", &core::string::clear);
				vstring->set_method<core::string, core::string&, size_t, size_t, char>("string& insert(usize, usize, uint8)", &core::string::insert);
				vstring->set_method<core::string, core::string&, size_t, const core::string&>("string& insert(usize, const string&in)", &core::string::insert);
				vstring->set_method<core::string, core::string&, size_t, const std::string_view&>("string& insert(usize, const string_view&in)", &core::string::insert);
				vstring->set_method<core::string, core::string&, size_t, size_t>("string& erase(usize, usize)", &core::string::erase);
				vstring->set_method<core::string, core::string&, size_t, char>("string& append(usize, uint8)", &core::string::append);
				vstring->set_method<core::string, core::string&, const core::string&>("string& append(const string&in)", &core::string::append);
				vstring->set_method<core::string, core::string&, const std::string_view&>("string& append(const string_view&in)", &core::string::append);
				vstring->set_method("void push(uint8)", &core::string::push_back);
				vstring->set_method_extern("void pop()", &string::pop_back);
				vstring->set_method_extern("bool starts_with(const string&in, usize = 0) const", &string::starts_with1);
				vstring->set_method_extern("bool ends_with(const string&in) const", &string::ends_with1);
				vstring->set_method_extern("string& replace(usize, usize, const string&in)", &string::replace_part1);
				vstring->set_method_extern("string& replace_all(const string&in, const string&in, usize)", &string::replace1);
				vstring->set_method_extern("bool starts_with(const string_view&in, usize = 0) const", &string::starts_with2);
				vstring->set_method_extern("bool ends_with(const string_view&in) const", &string::ends_with2);
				vstring->set_method_extern("string& replace(usize, usize, const string_view&in)", &string::replace_part2);
				vstring->set_method_extern("string& replace_all(const string_view&in, const string_view&in, usize)", &string::replace2);
				vstring->set_method_extern("string substring(usize) const", &string::substring1);
				vstring->set_method_extern("string substring(usize, usize) const", &string::substring2);
				vstring->set_method_extern("string substr(usize) const", &string::substring1);
				vstring->set_method_extern("string substr(usize, usize) const", &string::substring2);
				vstring->set_method_extern("string& trim()", &core::stringify::trim);
				vstring->set_method_extern("string& trim_start()", &core::stringify::trim_start);
				vstring->set_method_extern("string& trim_end()", &core::stringify::trim_end);
				vstring->set_method_extern("string& to_lower()", &core::stringify::to_lower);
				vstring->set_method_extern("string& to_upper()", &core::stringify::to_upper);
				vstring->set_method_extern<core::string&, core::string&>("string& reverse()", &core::stringify::reverse);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize rfind(const string&in, usize = 0) const", &core::string::rfind);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize rfind(const string_view&in, usize = 0) const", &core::string::rfind);
				vstring->set_method<core::string, size_t, char, size_t>("usize rfind(uint8, usize = 0) const", &core::string::rfind);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize find(const string&in, usize = 0) const", &core::string::find);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize find(const string_view&in, usize = 0) const", &core::string::find);
				vstring->set_method<core::string, size_t, char, size_t>("usize find(uint8, usize = 0) const", &core::string::find);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize find_first_of(const string&in, usize = 0) const", &core::string::find_first_of);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize find_first_not_of(const string&in, usize = 0) const", &core::string::find_first_not_of);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize find_last_of(const string&in, usize = 0) const", &core::string::find_last_of);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize find_last_not_of(const string&in, usize = 0) const", &core::string::find_last_not_of);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize find_first_of(const string_view&in, usize = 0) const", &core::string::find_first_of);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize find_first_not_of(const string_view&in, usize = 0) const", &core::string::find_first_not_of);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize find_last_of(const string_view&in, usize = 0) const", &core::string::find_last_of);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize find_last_not_of(const string_view&in, usize = 0) const", &core::string::find_last_not_of);
				vstring->set_method_extern("array<string>@ split(const string&in) const", &string::split);
				vstring->set_operator_extern(operators::impl_cast_t, (uint32_t)position::constant, "string_view", "", &string::impl_cast_string_view);
				vm->set_function("int8 to_int8(const string&in, int = 10)", &string::from_string<int8_t>);
				vm->set_function("int16 to_int16(const string&in, int = 10)", &string::from_string<int16_t>);
				vm->set_function("int32 to_int32(const string&in, int = 10)", &string::from_string<int32_t>);
				vm->set_function("int64 to_int64(const string&in, int = 10)", &string::from_string<int64_t>);
				vm->set_function("uint8 to_uint8(const string&in, int = 10)", &string::from_string<uint8_t>);
				vm->set_function("uint16 to_uint16(const string&in, int = 10)", &string::from_string<uint16_t>);
				vm->set_function("uint32 to_uint32(const string&in, int = 10)", &string::from_string<uint32_t>);
				vm->set_function("uint64 to_uint64(const string&in, int = 10)", &string::from_string<uint64_t>);
				vm->set_function("float to_float(const string&in, int = 10)", &string::from_string<float>);
				vm->set_function("double to_double(const string&in, int = 10)", &string::from_string<double>);
				vm->set_function("string to_string(int8, int = 10)", &core::to_string<int8_t>);
				vm->set_function("string to_string(int16, int = 10)", &core::to_string<int16_t>);
				vm->set_function("string to_string(int32, int = 10)", &core::to_string<int32_t>);
				vm->set_function("string to_string(int64, int = 10)", &core::to_string<int64_t>);
				vm->set_function("string to_string(uint8, int = 10)", &core::to_string<uint8_t>);
				vm->set_function("string to_string(uint16, int = 10)", &core::to_string<uint16_t>);
				vm->set_function("string to_string(uint32, int = 10)", &core::to_string<uint32_t>);
				vm->set_function("string to_string(uint64, int = 10)", &core::to_string<uint64_t>);
				vm->set_function("string to_string(float, int = 10)", &core::to_string<float>);
				vm->set_function("string to_string(double, int = 10)", &core::to_string<double>);
				vm->set_function("string to_string(uptr@, usize)", &string::from_buffer);
				vm->set_function("string handle_id(uptr@)", &string::from_pointer);
				vm->begin_namespace("string");
				vm->set_property("const usize npos", &core::string::npos);
				vm->end_namespace();

				vstring_view->set_constructor_extern("void f()", &string_view::create);
				vstring_view->set_constructor_extern("void f(const string&in)", &string_view::create_copy);
				vstring_view->set_destructor_extern("void f()", &string_view::destroy);
				vstring_view->set_method_extern("string_view& opAssign(const string&in)", &string_view::assign);
				vstring_view->set_method_extern<core::string, const std::string_view&, const std::string_view&>("string opAdd(const string_view&in) const", &string_view::append1);
				vstring_view->set_method_extern<core::string, const std::string_view&, const core::string&>("string opAdd(const string&in) const", &string_view::append2);
				vstring_view->set_method_extern<core::string, const core::string&, const std::string_view&>("string opAdd_r(const string&in) const", &string_view::append3);
				vstring_view->set_method_extern<core::string, const std::string_view&, char>("string opAdd(uint8) const", &string_view::append4);
				vstring_view->set_method_extern<core::string, char, const std::string_view&>("string opAdd_r(uint8) const", &string_view::append5);
				vstring_view->set_method_extern("int opCmp(const string&in) const", &string_view::compare1);
				vstring_view->set_method_extern("int opCmp(const string_view&in) const", &string_view::compare2);
				vstring_view->set_method_extern("const uint8& opIndex(usize) const", &string_view::index);
				vstring_view->set_method_extern("const uint8& at(usize) const", &string_view::index);
				vstring_view->set_method_extern("const uint8& front() const", &string_view::front);
				vstring_view->set_method_extern("const uint8& back() const", &string_view::back);
				vstring_view->set_method("uptr@ data() const", &std::string_view::data);
				vstring_view->set_method("bool empty() const", &std::string_view::empty);
				vstring_view->set_method("usize size() const", &std::string_view::size);
				vstring_view->set_method("usize max_size() const", &std::string_view::max_size);
				vstring_view->set_method_extern("bool starts_with(const string_view&in, usize = 0) const", &string_view::starts_with);
				vstring_view->set_method_extern("bool ends_with(const string_view&in) const", &string_view::ends_with);
				vstring_view->set_method_extern("string substring(usize) const", &string_view::substring1);
				vstring_view->set_method_extern("string substring(usize, usize) const", &string_view::substring2);
				vstring_view->set_method_extern("string substr(usize) const", &string_view::substring1);
				vstring_view->set_method_extern("string substr(usize, usize) const", &string_view::substring2);
				vstring_view->set_method_extern("usize rfind(const string_view&in, usize = 0) const", &string_view::reverse_find1);
				vstring_view->set_method_extern("usize rfind(uint8, usize = 0) const", &string_view::reverse_find2);
				vstring_view->set_method_extern("usize find(const string_view&in, usize = 0) const", &string_view::find1);
				vstring_view->set_method_extern("usize find(uint8, usize = 0) const", &string_view::find2);
				vstring_view->set_method_extern("usize find_first_of(const string_view&in, usize = 0) const", &string_view::find_first_of);
				vstring_view->set_method_extern("usize find_first_not_of(const string_view&in, usize = 0) const", &string_view::find_first_not_of);
				vstring_view->set_method_extern("usize find_last_of(const string_view&in, usize = 0) const", &string_view::find_last_of);
				vstring_view->set_method_extern("usize find_last_not_of(const string_view&in, usize = 0) const", &string_view::find_last_not_of);
				vstring_view->set_method_extern("array<string>@ split(const string_view&in) const", &string_view::split);
				vstring_view->set_operator_extern(operators::impl_cast_t, (uint32_t)position::constant, "string", "", &string_view::impl_cast_string);
				vm->set_function("int8 to_int8(const string_view&in)", &string_view::from_string<int8_t>);
				vm->set_function("int16 to_int16(const string_view&in)", &string_view::from_string<int16_t>);
				vm->set_function("int32 to_int32(const string_view&in)", &string_view::from_string<int32_t>);
				vm->set_function("int64 to_int64(const string_view&in)", &string_view::from_string<int64_t>);
				vm->set_function("uint8 to_uint8(const string_view&in)", &string_view::from_string<uint8_t>);
				vm->set_function("uint16 to_uint16(const string_view&in)", &string_view::from_string<uint16_t>);
				vm->set_function("uint32 to_uint32(const string_view&in)", &string_view::from_string<uint32_t>);
				vm->set_function("uint64 to_uint64(const string_view&in)", &string_view::from_string<uint64_t>);
				vm->set_function("float to_float(const string_view&in)", &string_view::from_string<float>);
				vm->set_function("double to_double(const string_view&in)", &string_view::from_string<double>);
				vm->set_function("uint64 component_id(const string_view&in)", &core::os::file::get_hash);
				vm->begin_namespace("string_view");
				vm->set_property("const usize npos", &std::string_view::npos);
				vm->end_namespace();

				return true;
			}
			bool registry::import_safe_string(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");
				auto vstring_view = vm->set_struct_address("string_view", sizeof(std::string_view), (size_t)object_behaviours::value | bridge::type_traits_of<std::string_view>() | (size_t)object_behaviours::app_class_allints);
				auto vstring = vm->set_struct_address("string", sizeof(core::string), (size_t)object_behaviours::value | bridge::type_traits_of<core::string>());
#ifdef VI_ANGELSCRIPT
				vm->set_string_factory_address("string", string_factory::get());
#endif
				vstring->set_constructor_extern("void f()", &string::create);
				vstring->set_constructor_extern("void f(const string&in)", &string::create_copy1);
				vstring->set_constructor_extern("void f(const string_view&in)", &string::create_copy2);
				vstring->set_destructor_extern("void f()", &string::destroy);
				vstring->set_method<core::string, core::string&, const core::string&>("string& opAssign(const string&in)", &core::string::operator=);
				vstring->set_method<core::string, core::string&, const std::string_view&>("string& opAssign(const string_view&in)", &core::string::operator=);
				vstring->set_method<core::string, core::string&, const core::string&>("string& opAddAssign(const string&in)", &core::string::operator+=);
				vstring->set_method<core::string, core::string&, const std::string_view&>("string& opAddAssign(const string_view&in)", &core::string::operator+=);
				vstring->set_method_extern<core::string, const core::string&, const core::string&>("string opAdd(const string&in) const", &std::operator+);
				vstring->set_method<core::string, core::string&, char>("string& opAddAssign(uint8)", &core::string::operator+=);
				vstring->set_method_extern<core::string, const core::string&, char>("string opAdd(uint8) const", &std::operator+);
				vstring->set_method_extern<core::string, char, const core::string&>("string opAdd_r(uint8) const", &std::operator+);
				vstring->set_method<core::string, int, const core::string&>("int opCmp(const string&in) const", &core::string::compare);
				vstring->set_method<core::string, int, const std::string_view&>("int opCmp(const string_view&in) const", &core::string::compare);
				vstring->set_method_extern("uint8& opIndex(usize)", &string::index);
				vstring->set_method_extern("const uint8& opIndex(usize) const", &string::index);
				vstring->set_method_extern("uint8& at(usize)", &string::index);
				vstring->set_method_extern("const uint8& at(usize) const", &string::index);
				vstring->set_method_extern("uint8& front()", &string::front);
				vstring->set_method_extern("const uint8& front() const", &string::front);
				vstring->set_method_extern("uint8& back()", &string::back);
				vstring->set_method_extern("const uint8& back() const", &string::back);
				vstring->set_method("bool empty() const", &core::string::empty);
				vstring->set_method("usize size() const", &core::string::size);
				vstring->set_method("void clear()", &core::string::clear);
				vstring->set_method<core::string, core::string&, const core::string&>("string& append(const string&in)", &core::string::append);
				vstring->set_method<core::string, core::string&, const std::string_view&>("string& append(const string_view&in)", &core::string::append);
				vstring->set_method("void push(uint8)", &core::string::push_back);
				vstring->set_method_extern("void pop()", &string::pop_back);
				vstring->set_method_extern("bool starts_with(const string&in, usize = 0) const", &string::starts_with1);
				vstring->set_method_extern("bool ends_with(const string&in) const", &string::ends_with1);
				vstring->set_method_extern("bool starts_with(const string_view&in, usize = 0) const", &string::starts_with2);
				vstring->set_method_extern("bool ends_with(const string_view&in) const", &string::ends_with2);
				vstring->set_method_extern("string substring(usize) const", &string::substring1);
				vstring->set_method_extern("string substring(usize, usize) const", &string::substring2);
				vstring->set_method_extern("string substr(usize) const", &string::substring1);
				vstring->set_method_extern("string substr(usize, usize) const", &string::substring2);
				vstring->set_method_extern("string& trim()", &core::stringify::trim);
				vstring->set_method_extern("string& trim_start()", &core::stringify::trim_start);
				vstring->set_method_extern("string& trim_end()", &core::stringify::trim_end);
				vstring->set_method_extern("string& to_lower()", &core::stringify::to_lower);
				vstring->set_method_extern("string& to_upper()", &core::stringify::to_upper);
				vstring->set_method_extern<core::string&, core::string&>("string& reverse()", &core::stringify::reverse);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize rfind(const string&in, usize = 0) const", &core::string::rfind);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize rfind(const string_view&in, usize = 0) const", &core::string::rfind);
				vstring->set_method<core::string, size_t, char, size_t>("usize rfind(uint8, usize = 0) const", &core::string::rfind);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize find(const string&in, usize = 0) const", &core::string::find);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize find(const string_view&in, usize = 0) const", &core::string::find);
				vstring->set_method<core::string, size_t, char, size_t>("usize find(uint8, usize = 0) const", &core::string::find);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize find_first_of(const string&in, usize = 0) const", &core::string::find_first_of);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize find_first_not_of(const string&in, usize = 0) const", &core::string::find_first_not_of);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize find_last_of(const string&in, usize = 0) const", &core::string::find_last_of);
				vstring->set_method<core::string, size_t, const core::string&, size_t>("usize find_last_not_of(const string&in, usize = 0) const", &core::string::find_last_not_of);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize find_first_of(const string_view&in, usize = 0) const", &core::string::find_first_of);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize find_first_not_of(const string_view&in, usize = 0) const", &core::string::find_first_not_of);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize find_last_of(const string_view&in, usize = 0) const", &core::string::find_last_of);
				vstring->set_method<core::string, size_t, const std::string_view&, size_t>("usize find_last_not_of(const string_view&in, usize = 0) const", &core::string::find_last_not_of);
				vstring->set_method_extern("array<string>@ split(const string&in) const", &string::split);
				vstring->set_operator_extern(operators::impl_cast_t, (uint32_t)position::constant, "string_view", "", &string::impl_cast_string_view);
				vm->set_function("int8 to_int8(const string&in, int = 10)", &string::from_string<int8_t>);
				vm->set_function("int16 to_int16(const string&in, int = 10)", &string::from_string<int16_t>);
				vm->set_function("int32 to_int32(const string&in, int = 10)", &string::from_string<int32_t>);
				vm->set_function("int64 to_int64(const string&in, int = 10)", &string::from_string<int64_t>);
				vm->set_function("uint8 to_uint8(const string&in, int = 10)", &string::from_string<uint8_t>);
				vm->set_function("uint16 to_uint16(const string&in, int = 10)", &string::from_string<uint16_t>);
				vm->set_function("uint32 to_uint32(const string&in, int = 10)", &string::from_string<uint32_t>);
				vm->set_function("uint64 to_uint64(const string&in, int = 10)", &string::from_string<uint64_t>);
				vm->set_function("float to_float(const string&in, int = 10)", &string::from_string<float>);
				vm->set_function("double to_double(const string&in, int = 10)", &string::from_string<double>);
				vm->set_function("string to_string(int8, int = 10)", &core::to_string<int8_t>);
				vm->set_function("string to_string(int16, int = 10)", &core::to_string<int16_t>);
				vm->set_function("string to_string(int32, int = 10)", &core::to_string<int32_t>);
				vm->set_function("string to_string(int64, int = 10)", &core::to_string<int64_t>);
				vm->set_function("string to_string(uint8, int = 10)", &core::to_string<uint8_t>);
				vm->set_function("string to_string(uint16, int = 10)", &core::to_string<uint16_t>);
				vm->set_function("string to_string(uint32, int = 10)", &core::to_string<uint32_t>);
				vm->set_function("string to_string(uint64, int = 10)", &core::to_string<uint64_t>);
				vm->set_function("string to_string(float, int = 10)", &core::to_string<float>);
				vm->set_function("string to_string(double, int = 10)", &core::to_string<double>);
				vm->begin_namespace("string");
				vm->set_property("const usize npos", &core::string::npos);
				vm->end_namespace();

				vstring_view->set_constructor_extern("void f()", &string_view::create);
				vstring_view->set_constructor_extern("void f(const string&in)", &string_view::create_copy);
				vstring_view->set_destructor_extern("void f()", &string_view::destroy);
				vstring_view->set_method_extern("string_view& opAssign(const string&in)", &string_view::assign);
				vstring_view->set_method_extern<core::string, const std::string_view&, const std::string_view&>("string opAdd(const string_view&in) const", &string_view::append1);
				vstring_view->set_method_extern<core::string, const std::string_view&, const core::string&>("string opAdd(const string&in) const", &string_view::append2);
				vstring_view->set_method_extern<core::string, const core::string&, const std::string_view&>("string opAdd_r(const string&in) const", &string_view::append3);
				vstring_view->set_method_extern<core::string, const std::string_view&, char>("string opAdd(uint8) const", &string_view::append4);
				vstring_view->set_method_extern<core::string, char, const std::string_view&>("string opAdd_r(uint8) const", &string_view::append5);
				vstring_view->set_method_extern("int opCmp(const string&in) const", &string_view::compare1);
				vstring_view->set_method_extern("int opCmp(const string_view&in) const", &string_view::compare2);
				vstring_view->set_method_extern("const uint8& opIndex(usize) const", &string_view::index);
				vstring_view->set_method_extern("const uint8& at(usize) const", &string_view::index);
				vstring_view->set_method_extern("const uint8& front() const", &string_view::front);
				vstring_view->set_method_extern("const uint8& back() const", &string_view::back);
				vstring_view->set_method("uptr@ data() const", &std::string_view::data);
				vstring_view->set_method("bool empty() const", &std::string_view::empty);
				vstring_view->set_method("usize size() const", &std::string_view::size);
				vstring_view->set_method("usize max_size() const", &std::string_view::max_size);
				vstring_view->set_method_extern("bool starts_with(const string_view&in, usize = 0) const", &string_view::starts_with);
				vstring_view->set_method_extern("bool ends_with(const string_view&in) const", &string_view::ends_with);
				vstring_view->set_method_extern("string substring(usize) const", &string_view::substring1);
				vstring_view->set_method_extern("string substring(usize, usize) const", &string_view::substring2);
				vstring_view->set_method_extern("string substr(usize) const", &string_view::substring1);
				vstring_view->set_method_extern("string substr(usize, usize) const", &string_view::substring2);
				vstring_view->set_method_extern("usize rfind(const string_view&in, usize = 0) const", &string_view::reverse_find1);
				vstring_view->set_method_extern("usize rfind(uint8, usize = 0) const", &string_view::reverse_find2);
				vstring_view->set_method_extern("usize find(const string_view&in, usize = 0) const", &string_view::find1);
				vstring_view->set_method_extern("usize find(uint8, usize = 0) const", &string_view::find2);
				vstring_view->set_method_extern("usize find_first_of(const string_view&in, usize = 0) const", &string_view::find_first_of);
				vstring_view->set_method_extern("usize find_first_not_of(const string_view&in, usize = 0) const", &string_view::find_first_not_of);
				vstring_view->set_method_extern("usize find_last_of(const string_view&in, usize = 0) const", &string_view::find_last_of);
				vstring_view->set_method_extern("usize find_last_not_of(const string_view&in, usize = 0) const", &string_view::find_last_not_of);
				vstring_view->set_method_extern("array<string>@ split(const string_view&in) const", &string_view::split);
				vstring_view->set_operator_extern(operators::impl_cast_t, (uint32_t)position::constant, "string", "", &string_view::impl_cast_string);
				vm->set_function("int8 to_int8(const string_view&in)", &string_view::from_string<int8_t>);
				vm->set_function("int16 to_int16(const string_view&in)", &string_view::from_string<int16_t>);
				vm->set_function("int32 to_int32(const string_view&in)", &string_view::from_string<int32_t>);
				vm->set_function("int64 to_int64(const string_view&in)", &string_view::from_string<int64_t>);
				vm->set_function("uint8 to_uint8(const string_view&in)", &string_view::from_string<uint8_t>);
				vm->set_function("uint16 to_uint16(const string_view&in)", &string_view::from_string<uint16_t>);
				vm->set_function("uint32 to_uint32(const string_view&in)", &string_view::from_string<uint32_t>);
				vm->set_function("uint64 to_uint64(const string_view&in)", &string_view::from_string<uint64_t>);
				vm->set_function("float to_float(const string_view&in)", &string_view::from_string<float>);
				vm->set_function("double to_double(const string_view&in)", &string_view::from_string<double>);
				vm->set_function("uint64 component_id(const string_view&in)", &core::os::file::get_hash);
				vm->begin_namespace("string_view");
				vm->set_property("const usize npos", &std::string_view::npos);
				vm->end_namespace();

				return true;
			}
			bool registry::import_exception(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");
				auto vexception_ptr = vm->set_struct_trivial<exception::pointer>("exception_ptr");
				vexception_ptr->set_property("string type", &exception::pointer::type);
				vexception_ptr->set_property("string text", &exception::pointer::text);
				vexception_ptr->set_property("string origin", &exception::pointer::origin);
				vexception_ptr->set_constructor<exception::pointer>("void f()");
				vexception_ptr->set_constructor<exception::pointer, const std::string_view&>("void f(const string_view&in)");
				vexception_ptr->set_constructor<exception::pointer, const std::string_view&, const std::string_view&>("void f(const string_view&in, const string_view&in)");
				vexception_ptr->set_method("const string& get_type() const", &exception::pointer::get_type);
				vexception_ptr->set_method("const string& get_text() const", &exception::pointer::get_text);
				vexception_ptr->set_method("string what() const", &exception::pointer::what);
				vexception_ptr->set_method("bool empty() const", &exception::pointer::empty);

				vm->set_code_generator("throw-syntax", &exception::generator_callback);
				vm->begin_namespace("exception");
				vm->set_function("void throw(const exception_ptr&in)", &exception::throw_ptr);
				vm->set_function("void rethrow()", &exception::rethrow);
				vm->set_function("exception_ptr unwrap()", &exception::get_exception);
				vm->end_namespace();
				return true;
			}
			bool registry::import_mutex(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");
				auto vmutex = vm->set_class<mutex>("mutex", false);
				vmutex->set_constructor_extern("mutex@ f()", &mutex::factory);
				vmutex->set_method("bool try_lock()", &mutex::try_lock);
				vmutex->set_method("void lock()", &mutex::lock);
				vmutex->set_method("void unlock()", &mutex::unlock);
				return true;
#else
				VI_ASSERT(false, "<mutex> is not loaded");
				return false;
#endif
			}
			bool registry::import_thread(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");
				auto vthread = vm->set_class<thread>("thread", true);
				vthread->set_function_def("void thread_parallel(thread@+)");
				vthread->set_constructor_extern("thread@ f(thread_parallel@)", &thread::create);
				vthread->set_enum_refs(&thread::enum_references);
				vthread->set_release_refs(&thread::release_references);
				vthread->set_method("bool is_active()", &thread::is_active);
				vthread->set_method("bool invoke()", &thread::start);
				vthread->set_method("bool suspend()", &thread::suspend);
				vthread->set_method("bool resume()", &thread::resume);
				vthread->set_method("void push(const ?&in)", &thread::push);
				vthread->set_method<thread, bool, void*, int>("bool pop(?&out)", &thread::pop);
				vthread->set_method<thread, bool, void*, int, uint64_t>("bool pop(?&out, uint64)", &thread::pop);
				vthread->set_method<thread, int, uint64_t>("int join(uint64)", &thread::join);
				vthread->set_method<thread, int>("int join()", &thread::join);
				vthread->set_method("string get_id() const", &thread::get_id);

				vm->begin_namespace("this_thread");
				vm->set_function("thread@+ get_routine()", &thread::get_thread);
				vm->set_function("string get_id()", &thread::get_thread_id);
				vm->set_function("void sleep(uint64)", &thread::thread_sleep);
				vm->set_function("bool suspend()", &thread::thread_suspend);
				vm->end_namespace();
				return true;
#else
				VI_ASSERT(false, "<thread> is not loaded");
				return false;
#endif
			}
			bool registry::import_buffers(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");
				auto vchar_buffer = vm->set_class<char_buffer>("char_buffer", false);
				vchar_buffer->set_constructor_extern<char_buffer>("char_buffer@ f()", &char_buffer::create);
				vchar_buffer->set_constructor_extern<char_buffer, size_t>("char_buffer@ f(usize)", &char_buffer::create);
				vchar_buffer->set_constructor_extern<char_buffer, char*>("char_buffer@ f(uptr@)", &char_buffer::create);
				vchar_buffer->set_method("uptr@ get_ptr(usize = 0) const", &char_buffer::get_pointer);
				vchar_buffer->set_method("bool allocate(usize)", &char_buffer::allocate);
				vchar_buffer->set_method("bool deallocate()", &char_buffer::deallocate);
				vchar_buffer->set_method("bool exists(usize) const", &char_buffer::exists);
				vchar_buffer->set_method("bool empty() const", &char_buffer::empty);
				vchar_buffer->set_method("bool set(usize, int8, usize)", &char_buffer::set_int8);
				vchar_buffer->set_method("bool set(usize, uint8, usize)", &char_buffer::set_uint8);
				vchar_buffer->set_method("bool store(usize, const string_view&in)", &char_buffer::store_bytes);
				vchar_buffer->set_method("bool store(usize, int8)", &char_buffer::store_int8);
				vchar_buffer->set_method("bool store(usize, uint8)", &char_buffer::store_uint8);
				vchar_buffer->set_method("bool store(usize, int16)", &char_buffer::store_int16);
				vchar_buffer->set_method("bool store(usize, uint16)", &char_buffer::store_uint16);
				vchar_buffer->set_method("bool store(usize, int32)", &char_buffer::store_int32);
				vchar_buffer->set_method("bool store(usize, uint32)", &char_buffer::store_uint32);
				vchar_buffer->set_method("bool store(usize, int64)", &char_buffer::store_int64);
				vchar_buffer->set_method("bool store(usize, uint64)", &char_buffer::store_uint64);
				vchar_buffer->set_method("bool store(usize, float)", &char_buffer::store_float);
				vchar_buffer->set_method("bool store(usize, double)", &char_buffer::store_double);
				vchar_buffer->set_method("bool interpret(usize, string&out, usize) const", &char_buffer::interpret);
				vchar_buffer->set_method("bool load(usize, string&out, usize) const", &char_buffer::load_bytes);
				vchar_buffer->set_method("bool load(usize, int8&out) const", &char_buffer::load_int8);
				vchar_buffer->set_method("bool load(usize, uint8&out) const", &char_buffer::load_uint8);
				vchar_buffer->set_method("bool load(usize, int16&out) const", &char_buffer::load_int16);
				vchar_buffer->set_method("bool load(usize, uint16&out) const", &char_buffer::load_uint16);
				vchar_buffer->set_method("bool load(usize, int32&out) const", &char_buffer::load_int32);
				vchar_buffer->set_method("bool load(usize, uint32&out) const", &char_buffer::load_uint32);
				vchar_buffer->set_method("bool load(usize, int64&out) const", &char_buffer::load_int64);
				vchar_buffer->set_method("bool load(usize, uint64&out) const", &char_buffer::load_uint64);
				vchar_buffer->set_method("bool load(usize, float&out) const", &char_buffer::load_float);
				vchar_buffer->set_method("bool load(usize, double&out) const", &char_buffer::load_double);
				vchar_buffer->set_method("usize size() const", &char_buffer::size);
				vchar_buffer->set_method("string to_string(usize) const", &char_buffer::to_string);
				return true;
#else
				VI_ASSERT(false, "<buffers> is not loaded");
				return false;
#endif
			}
			bool registry::import_random(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr && vm->get_engine() != nullptr, "manager should be set");

				vm->begin_namespace("random");
				vm->set_function("string bytes(uint64)", &random::getb);
				vm->set_function("double betweend(double, double)", &random::betweend);
				vm->set_function("double magd()", &random::magd);
				vm->set_function("double getd()", &random::getd);
				vm->set_function("float betweenf(float, float)", &random::betweenf);
				vm->set_function("float magf()", &random::magf);
				vm->set_function("float getf()", &random::getf);
				vm->set_function("uint64 betweeni(uint64, uint64)", &random::betweeni);
				vm->end_namespace();

				return true;
			}
			bool registry::import_promise(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr, "manager should be set");
				auto vpromise = vm->set_template_class<promise>("promise<class t>", "promise<t>", true);
				vpromise->set_template_callback(&promise::template_callback);
				vpromise->set_enum_refs(&promise::enum_references);
				vpromise->set_release_refs(&promise::release_references);
				vpromise->set_method<promise, void, void*, int>("void wrap(?&in)", &promise::store);
				vpromise->set_method("void except(const exception_ptr&in)", &promise::store_exception);
				vpromise->set_method<promise, void*>("t& unwrap()", &promise::retrieve);
				vpromise->set_method("promise<t>@+ yield()", &promise::yield_if);
				vpromise->set_method("bool pending()", &promise::is_pending);

				auto vpromise_void = vm->set_class<promise>("promise_v", true);
				vpromise_void->set_enum_refs(&promise::enum_references);
				vpromise_void->set_release_refs(&promise::release_references);
				vpromise_void->set_method("void wrap()", &promise::store_void);
				vpromise_void->set_method("void except(const exception_ptr&in)", &promise::store_exception);
				vpromise_void->set_method("void unwrap()", &promise::retrieve_void);
				vpromise_void->set_method("promise_v@+ yield()", &promise::yield_if);
				vpromise_void->set_method("bool pending()", &promise::is_pending);

				bool has_constructor = (!vm->get_library_property(library_features::promise_no_constructor));
				if (has_constructor)
				{
					vpromise->set_constructor_extern("promise<t>@ f(int&in)", &promise::create_factory_type);
					vpromise_void->set_constructor_extern("promise_v@ f()", &promise::create_factory_void);
				}

				bool has_callbacks = (!vm->get_library_property(library_features::promise_no_callbacks));
				if (has_callbacks)
				{
					vpromise->set_function_def("void promise<t>::when_async(promise<t>@+)");
					vpromise->set_method<promise, void, asIScriptFunction*>("void when(when_async@)", &promise::when);
					vpromise_void->set_function_def("void promise_v::when_async(promise_v@+)");
					vpromise_void->set_method<promise, void, asIScriptFunction*>("void when(when_async@)", &promise::when);
				}

				vm->set_code_generator("await-syntax", &promise::generator_callback);
				return true;
			}
			bool registry::import_decimal(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr, "manager should be set");

				auto vdecimal = vm->set_struct_trivial<core::decimal>("decimal");
				if (vm->get_library_property(library_features::decimal_default_precision) > 0)
				{
					vdecimal->set_constructor_extern<core::decimal*>("void f()", &decimal_custom_constructor);
					vdecimal->set_constructor_extern<core::decimal*, int16_t>("void f(int16)", &decimal_custom_constructor_arithmetic<int16_t>);
					vdecimal->set_constructor_extern<core::decimal*, uint16_t>("void f(uint16)", &decimal_custom_constructor_arithmetic<uint16_t>);
					vdecimal->set_constructor_extern<core::decimal*, int32_t>("void f(int32)", &decimal_custom_constructor_arithmetic<int32_t>);
					vdecimal->set_constructor_extern<core::decimal*, uint32_t>("void f(uint32)", &decimal_custom_constructor_arithmetic<uint32_t>);
					vdecimal->set_constructor_extern<core::decimal*, int64_t>("void f(int64)", &decimal_custom_constructor_arithmetic<int64_t>);
					vdecimal->set_constructor_extern<core::decimal*, uint64_t>("void f(uint64)", &decimal_custom_constructor_arithmetic<uint64_t>);
					vdecimal->set_constructor_extern<core::decimal*, float>("void f(float)", &decimal_custom_constructor_arithmetic<float>);
					vdecimal->set_constructor_extern<core::decimal*, double>("void f(double)", &decimal_custom_constructor_arithmetic<double>);
					vdecimal->set_constructor_extern<core::decimal*, const std::string_view&>("void f(const string_view&in)", &decimal_custom_constructor_auto<std::string_view>);
					vdecimal->set_constructor_extern<core::decimal*, const core::decimal&>("void f(const decimal &in)", &decimal_custom_constructor_auto<core::decimal>);
				}
				else
				{
					vdecimal->set_constructor<core::decimal>("void f()");
					vdecimal->set_constructor<core::decimal, int16_t>("void f(int16)");
					vdecimal->set_constructor<core::decimal, uint16_t>("void f(uint16)");
					vdecimal->set_constructor<core::decimal, int32_t>("void f(int32)");
					vdecimal->set_constructor<core::decimal, uint32_t>("void f(uint32)");
					vdecimal->set_constructor<core::decimal, int64_t>("void f(int64)");
					vdecimal->set_constructor<core::decimal, uint64_t>("void f(uint64)");
					vdecimal->set_constructor<core::decimal, float>("void f(float)");
					vdecimal->set_constructor<core::decimal, double>("void f(double)");
					vdecimal->set_constructor<core::decimal, const std::string_view&>("void f(const string_view&in)");
					vdecimal->set_constructor<core::decimal, const core::decimal&>("void f(const decimal &in)");
				}
				vdecimal->set_method("decimal& truncate(int)", &core::decimal::truncate);
				vdecimal->set_method("decimal& round(int)", &core::decimal::round);
				vdecimal->set_method("decimal& trim()", &core::decimal::trim);
				vdecimal->set_method("decimal& unlead()", &core::decimal::unlead);
				vdecimal->set_method("decimal& untrail()", &core::decimal::untrail);
				vdecimal->set_method("bool is_nan() const", &core::decimal::is_nan);
				vdecimal->set_method("bool is_zero() const", &core::decimal::is_zero);
				vdecimal->set_method("bool is_zero_or_nan() const", &core::decimal::is_zero_or_nan);
				vdecimal->set_method("bool is_positive() const", &core::decimal::is_positive);
				vdecimal->set_method("bool is_negative() const", &core::decimal::is_negative);
				vdecimal->set_method("bool is_integer() const", &core::decimal::is_integer);
				vdecimal->set_method("bool is_fractional() const", &core::decimal::is_fractional);
				vdecimal->set_method("float to_float() const", &core::decimal::to_float);
				vdecimal->set_method("double to_double() const", &core::decimal::to_double);
				vdecimal->set_method("int8 to_int8() const", &core::decimal::to_int8);
				vdecimal->set_method("uint8 to_uint8() const", &core::decimal::to_uint8);
				vdecimal->set_method("int16 to_int16() const", &core::decimal::to_int16);
				vdecimal->set_method("uint16 to_uint16() const", &core::decimal::to_uint16);
				vdecimal->set_method("int32 to_int32() const", &core::decimal::to_int32);
				vdecimal->set_method("uint32 to_uint32() const", &core::decimal::to_uint32);
				vdecimal->set_method("int64 to_int64() const", &core::decimal::to_int64);
				vdecimal->set_method("uint64 to_uint64() const", &core::decimal::to_uint64);
				vdecimal->set_method("string to_string() const", &core::decimal::to_string);
				vdecimal->set_method("string to_exponent() const", &core::decimal::to_exponent);
				vdecimal->set_method("uint32 decimal_places() const", &core::decimal::decimal_places);
				vdecimal->set_method("uint32 whole_places() const", &core::decimal::integer_places);
				vdecimal->set_method("uint32 size() const", &core::decimal::size);
				vdecimal->set_operator_extern(operators::neg_t, (uint32_t)position::constant, "decimal", "", &decimal_negate);
				vdecimal->set_operator_extern(operators::mul_assign_t, (uint32_t)position::left, "decimal&", "const decimal &in", &decimal_mul_eq);
				vdecimal->set_operator_extern(operators::div_assign_t, (uint32_t)position::left, "decimal&", "const decimal &in", &decimal_div_eq);
				vdecimal->set_operator_extern(operators::add_assign_t, (uint32_t)position::left, "decimal&", "const decimal &in", &decimal_add_eq);
				vdecimal->set_operator_extern(operators::sub_assign_t, (uint32_t)position::left, "decimal&", "const decimal &in", &decimal_sub_eq);
				vdecimal->set_operator_extern(operators::pre_inc_t, (uint32_t)position::left, "decimal&", "", &decimal_fpp);
				vdecimal->set_operator_extern(operators::pre_dec_t, (uint32_t)position::left, "decimal&", "", &decimal_fmm);
				vdecimal->set_operator_extern(operators::post_inc_t, (uint32_t)position::left, "decimal&", "", &decimal_pp);
				vdecimal->set_operator_extern(operators::post_dec_t, (uint32_t)position::left, "decimal&", "", &decimal_mm);
				vdecimal->set_operator_extern(operators::equals_t, (uint32_t)position::constant, "bool", "const decimal &in", &decimal_eq);
				vdecimal->set_operator_extern(operators::cmp_t, (uint32_t)position::constant, "int", "const decimal &in", &decimal_cmp);
				vdecimal->set_operator_extern(operators::add_t, (uint32_t)position::constant, "decimal", "const decimal &in", &decimal_add);
				vdecimal->set_operator_extern(operators::sub_t, (uint32_t)position::constant, "decimal", "const decimal &in", &decimal_sub);
				vdecimal->set_operator_extern(operators::mul_t, (uint32_t)position::constant, "decimal", "const decimal &in", &decimal_mul);
				vdecimal->set_operator_extern(operators::div_t, (uint32_t)position::constant, "decimal", "const decimal &in", &decimal_div);
				vdecimal->set_operator_extern(operators::mod_t, (uint32_t)position::constant, "decimal", "const decimal &in", &decimal_per);
				vdecimal->set_method_static("decimal nan()", &core::decimal::nan);
				vdecimal->set_method_static("decimal zero()", &core::decimal::zero);
				vdecimal->set_method_static("decimal from(const string_view&in, uint8)", &core::decimal::from);

				return true;
			}
			bool registry::import_uint128(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr, "manager should be set");

				auto vuint128 = vm->set_struct_trivial<compute::uint128>("uint128", (size_t)object_behaviours::app_class_allints);
				vuint128->set_constructor_extern("void f()", &uint128_default_construct);
				vuint128->set_constructor<compute::uint128, int16_t>("void f(int16)");
				vuint128->set_constructor<compute::uint128, uint16_t>("void f(uint16)");
				vuint128->set_constructor<compute::uint128, int32_t>("void f(int32)");
				vuint128->set_constructor<compute::uint128, uint32_t>("void f(uint32)");
				vuint128->set_constructor<compute::uint128, int64_t>("void f(int64)");
				vuint128->set_constructor<compute::uint128, uint64_t>("void f(uint64)");
				vuint128->set_constructor<compute::uint128, const std::string_view&>("void f(const string_view&in)");
				vuint128->set_constructor<compute::uint128, const compute::uint128&>("void f(const uint128 &in)");
				vuint128->set_method_extern("bool opImplConv() const", &uint128_to_bool);
				vuint128->set_method_extern("int8 to_int8() const", &uint128_to_int8);
				vuint128->set_method_extern("uint8 to_uint8() const", &uint128_to_uint8);
				vuint128->set_method_extern("int16 to_int16() const", &uint128_to_int16);
				vuint128->set_method_extern("uint16 to_uint16() const", &uint128_to_uint16);
				vuint128->set_method_extern("int32 to_int32() const", &uint128_to_int32);
				vuint128->set_method_extern("uint32 to_uint32() const", &uint128_to_uint32);
				vuint128->set_method_extern("int64 to_int64() const", &uint128_to_int64);
				vuint128->set_method_extern("uint64 to_uint64() const", &uint128_to_uint64);
				vuint128->set_method("decimal to_decimal() const", &compute::uint128::to_decimal);
				vuint128->set_method("string to_string(uint8 = 10, uint32 = 0) const", &compute::uint128::to_string);
				vuint128->set_method<compute::uint128, const uint64_t&>("const uint64& low() const", &compute::uint128::low);
				vuint128->set_method<compute::uint128, const uint64_t&>("const uint64& high() const", &compute::uint128::high);
				vuint128->set_method("uint8 bits() const", &compute::uint128::bits);
				vuint128->set_method("uint8 bytes() const", &compute::uint128::bits);
				vuint128->set_operator_extern(operators::mul_assign_t, (uint32_t)position::left, "uint128&", "const uint128 &in", &uint128_mul_eq);
				vuint128->set_operator_extern(operators::div_assign_t, (uint32_t)position::left, "uint128&", "const uint128 &in", &uint128_div_eq);
				vuint128->set_operator_extern(operators::add_assign_t, (uint32_t)position::left, "uint128&", "const uint128 &in", &uint128_add_eq);
				vuint128->set_operator_extern(operators::sub_assign_t, (uint32_t)position::left, "uint128&", "const uint128 &in", &uint128_sub_eq);
				vuint128->set_operator_extern(operators::pre_inc_t, (uint32_t)position::left, "uint128&", "", &uint128_fpp);
				vuint128->set_operator_extern(operators::pre_dec_t, (uint32_t)position::left, "uint128&", "", &uint128_fmm);
				vuint128->set_operator_extern(operators::post_inc_t, (uint32_t)position::left, "uint128&", "", &uint128_pp);
				vuint128->set_operator_extern(operators::post_dec_t, (uint32_t)position::left, "uint128&", "", &uint128_mm);
				vuint128->set_operator_extern(operators::equals_t, (uint32_t)position::constant, "bool", "const uint128 &in", &uint128_eq);
				vuint128->set_operator_extern(operators::cmp_t, (uint32_t)position::constant, "int", "const uint128 &in", &uint128_cmp);
				vuint128->set_operator_extern(operators::add_t, (uint32_t)position::constant, "uint128", "const uint128 &in", &uint128_add);
				vuint128->set_operator_extern(operators::sub_t, (uint32_t)position::constant, "uint128", "const uint128 &in", &uint128_sub);
				vuint128->set_operator_extern(operators::mul_t, (uint32_t)position::constant, "uint128", "const uint128 &in", &uint128_mul);
				vuint128->set_operator_extern(operators::div_t, (uint32_t)position::constant, "uint128", "const uint128 &in", &uint128_div);
				vuint128->set_operator_extern(operators::mod_t, (uint32_t)position::constant, "uint128", "const uint128 &in", &uint128_per);
				vuint128->set_method_static("uint128 min_value()", &compute::uint128::min);
				vuint128->set_method_static("uint128 max_value()", &compute::uint128::max);

				return true;
			}
			bool registry::import_uint256(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr, "manager should be set");

				auto vuint256 = vm->set_struct_trivial<compute::uint256>("uint256", (size_t)object_behaviours::app_class_allints);
				vuint256->set_constructor_extern("void f()", &uint256_default_construct);
				vuint256->set_constructor<compute::uint256, int16_t>("void f(int16)");
				vuint256->set_constructor<compute::uint256, uint16_t>("void f(uint16)");
				vuint256->set_constructor<compute::uint256, int32_t>("void f(int32)");
				vuint256->set_constructor<compute::uint256, uint32_t>("void f(uint32)");
				vuint256->set_constructor<compute::uint256, int64_t>("void f(int64)");
				vuint256->set_constructor<compute::uint256, uint64_t>("void f(uint64)");
				vuint256->set_constructor<compute::uint256, const compute::uint128&>("void f(const uint128&in)");
				vuint256->set_constructor<compute::uint256, const compute::uint128&, const compute::uint128&>("void f(const uint128&in, const uint128&in)");
				vuint256->set_constructor<compute::uint256, const std::string_view&>("void f(const string_view&in)");
				vuint256->set_constructor<compute::uint256, const compute::uint256&>("void f(const uint256 &in)");
				vuint256->set_method_extern("bool opImplConv() const", &uint256_to_bool);
				vuint256->set_method_extern("int8 to_int8() const", &uint256_to_int8);
				vuint256->set_method_extern("uint8 to_uint8() const", &uint256_to_uint8);
				vuint256->set_method_extern("int16 to_int16() const", &uint256_to_int16);
				vuint256->set_method_extern("uint16 to_uint16() const", &uint256_to_uint16);
				vuint256->set_method_extern("int32 to_int32() const", &uint256_to_int32);
				vuint256->set_method_extern("uint32 to_uint32() const", &uint256_to_uint32);
				vuint256->set_method_extern("int64 to_int64() const", &uint256_to_int64);
				vuint256->set_method_extern("uint64 to_uint64() const", &uint256_to_uint64);
				vuint256->set_method("decimal to_decimal() const", &compute::uint256::to_decimal);
				vuint256->set_method("string to_string(uint8 = 10, uint32 = 0) const", &compute::uint256::to_string);
				vuint256->set_method<compute::uint256, const compute::uint128&>("const uint128& low() const", &compute::uint256::low);
				vuint256->set_method<compute::uint256, const compute::uint128&>("const uint128& high() const", &compute::uint256::high);
				vuint256->set_method("uint16 bits() const", &compute::uint256::bits);
				vuint256->set_method("uint16 bytes() const", &compute::uint256::bytes);
				vuint256->set_operator_extern(operators::mul_assign_t, (uint32_t)position::left, "uint256&", "const uint256 &in", &uint256_mul_eq);
				vuint256->set_operator_extern(operators::div_assign_t, (uint32_t)position::left, "uint256&", "const uint256 &in", &uint256_div_eq);
				vuint256->set_operator_extern(operators::add_assign_t, (uint32_t)position::left, "uint256&", "const uint256 &in", &uint256_add_eq);
				vuint256->set_operator_extern(operators::sub_assign_t, (uint32_t)position::left, "uint256&", "const uint256 &in", &uint256_sub_eq);
				vuint256->set_operator_extern(operators::pre_inc_t, (uint32_t)position::left, "uint256&", "", &uint256_fpp);
				vuint256->set_operator_extern(operators::pre_dec_t, (uint32_t)position::left, "uint256&", "", &uint256_fmm);
				vuint256->set_operator_extern(operators::post_inc_t, (uint32_t)position::left, "uint256&", "", &uint256_pp);
				vuint256->set_operator_extern(operators::post_dec_t, (uint32_t)position::left, "uint256&", "", &uint256_mm);
				vuint256->set_operator_extern(operators::equals_t, (uint32_t)position::constant, "bool", "const uint256 &in", &uint256_eq);
				vuint256->set_operator_extern(operators::cmp_t, (uint32_t)position::constant, "int", "const uint256 &in", &uint256_cmp);
				vuint256->set_operator_extern(operators::add_t, (uint32_t)position::constant, "uint256", "const uint256 &in", &uint256_add);
				vuint256->set_operator_extern(operators::sub_t, (uint32_t)position::constant, "uint256", "const uint256 &in", &uint256_sub);
				vuint256->set_operator_extern(operators::mul_t, (uint32_t)position::constant, "uint256", "const uint256 &in", &uint256_mul);
				vuint256->set_operator_extern(operators::div_t, (uint32_t)position::constant, "uint256", "const uint256 &in", &uint256_div);
				vuint256->set_operator_extern(operators::mod_t, (uint32_t)position::constant, "uint256", "const uint256 &in", &uint256_per);
				vuint256->set_method_static("uint256 min_value()", &compute::uint256::min);
				vuint256->set_method_static("uint256 max_value()", &compute::uint256::max);

				return true;
			}
			bool registry::import_variant(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr, "manager should be set");
				auto vvar_type = vm->set_enum("var_type");
				vvar_type->set_value("null_t", (int)core::var_type::null);
				vvar_type->set_value("undefined_t", (int)core::var_type::undefined);
				vvar_type->set_value("object_t", (int)core::var_type::object);
				vvar_type->set_value("array_t", (int)core::var_type::array);
				vvar_type->set_value("string_t", (int)core::var_type::string);
				vvar_type->set_value("binary_t", (int)core::var_type::binary);
				vvar_type->set_value("integer_t", (int)core::var_type::integer);
				vvar_type->set_value("number_t", (int)core::var_type::number);
				vvar_type->set_value("decimal_t", (int)core::var_type::decimal);
				vvar_type->set_value("boolean_t", (int)core::var_type::boolean);

				auto vvariant = vm->set_struct_trivial<core::variant>("variant");
				vvariant->set_constructor<core::variant>("void f()");
				vvariant->set_constructor<core::variant, const core::variant&>("void f(const variant &in)");
				vvariant->set_method("bool deserialize(const string_view&in, bool = false)", &core::variant::deserialize);
				vvariant->set_method("string serialize() const", &core::variant::serialize);
				vvariant->set_method("decimal to_decimal() const", &core::variant::get_decimal);
				vvariant->set_method("string to_string() const", &core::variant::get_blob);
				vvariant->set_method("uptr@ to_pointer() const", &core::variant::get_pointer);
				vvariant->set_method_extern("int8 to_int8() const", &variant_to_int8);
				vvariant->set_method_extern("uint8 to_uint8() const", &variant_to_uint8);
				vvariant->set_method_extern("int16 to_int16() const", &variant_to_int16);
				vvariant->set_method_extern("uint16 to_uint16() const", &variant_to_uint16);
				vvariant->set_method_extern("int32 to_int32() const", &variant_to_int32);
				vvariant->set_method_extern("uint32 to_uint32() const", &variant_to_uint32);
				vvariant->set_method_extern("int64 to_int64() const", &variant_to_int64);
				vvariant->set_method_extern("uint64 to_uint64() const", &variant_to_uint64);
				vvariant->set_method("double to_number() const", &core::variant::get_number);
				vvariant->set_method("bool to_boolean() const", &core::variant::get_boolean);
				vvariant->set_method("var_type get_type() const", &core::variant::get_type);
				vvariant->set_method("bool is_object() const", &core::variant::is_object);
				vvariant->set_method("bool empty() const", &core::variant::empty);
				vvariant->set_method_extern("usize size() const", &variant_get_size);
				vvariant->set_operator_extern(operators::equals_t, (uint32_t)position::constant, "bool", "const variant &in", &variant_equals);
				vvariant->set_operator_extern(operators::impl_cast_t, (uint32_t)position::constant, "bool", "", &variant_impl_cast);

				vm->begin_namespace("var");
				vm->set_function("variant auto_t(const string_view&in, bool = false)", &core::var::any);
				vm->set_function("variant null_t()", &core::var::null);
				vm->set_function("variant undefined_t()", &core::var::undefined);
				vm->set_function("variant object_t()", &core::var::object);
				vm->set_function("variant array_t()", &core::var::array);
				vm->set_function("variant pointer_t(uptr@)", &core::var::pointer);
				vm->set_function("variant integer_t(int64)", &core::var::integer);
				vm->set_function("variant number_t(double)", &core::var::number);
				vm->set_function("variant boolean_t(bool)", &core::var::boolean);
				vm->set_function<core::variant(const std::string_view&)>("variant string_t(const string_view&in)", &core::var::string);
				vm->set_function<core::variant(const std::string_view&)>("variant binary_t(const string_view&in)", &core::var::binary);
				vm->set_function<core::variant(const std::string_view&)>("variant decimal_t(const string_view&in)", &core::var::decimal_string);
				vm->set_function<core::variant(const core::decimal&)>("variant decimal_t(const decimal &in)", &core::var::decimal);
				vm->end_namespace();

				return true;
			}
			bool registry::import_timestamp(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr, "manager should be set");
				auto vdate_time = vm->set_struct_trivial<core::date_time>("timestamp");
				vdate_time->set_constructor<core::date_time>("void f()");
				vdate_time->set_constructor<core::date_time, const core::date_time&>("void f(const timestamp&in)");
				vdate_time->set_method("timestamp& apply_offset(bool = false)", &core::date_time::apply_offset);
				vdate_time->set_method("timestamp& apply_timepoint(bool = false)", &core::date_time::apply_timepoint);
				vdate_time->set_method("timestamp& use_global_time()", &core::date_time::use_global_time);
				vdate_time->set_method("timestamp& use_local_time()", &core::date_time::use_local_time);
				vdate_time->set_method("timestamp& set_second(uint8)", &core::date_time::set_second);
				vdate_time->set_method("timestamp& set_minute(uint8)", &core::date_time::set_minute);
				vdate_time->set_method("timestamp& set_hour(uint8)", &core::date_time::set_hour);
				vdate_time->set_method("timestamp& set_day(uint8)", &core::date_time::set_day);
				vdate_time->set_method("timestamp& set_week(uint8)", &core::date_time::set_week);
				vdate_time->set_method("timestamp& set_month(uint8)", &core::date_time::set_month);
				vdate_time->set_method("timestamp& set_year(uint32)", &core::date_time::set_year);
				vdate_time->set_method("string serialize(const string_view&in)", &core::date_time::serialize);
				vdate_time->set_method("uint8 second() const", &core::date_time::second);
				vdate_time->set_method("uint8 minute() const", &core::date_time::minute);
				vdate_time->set_method("uint8 hour() const", &core::date_time::hour);
				vdate_time->set_method("uint8 day() const", &core::date_time::day);
				vdate_time->set_method("uint8 week() const", &core::date_time::week);
				vdate_time->set_method("uint8 month() const", &core::date_time::month);
				vdate_time->set_method("uint32 year() const", &core::date_time::year);
				vdate_time->set_method("int64 nanoseconds() const", &core::date_time::nanoseconds);
				vdate_time->set_method("int64 microseconds() const", &core::date_time::microseconds);
				vdate_time->set_method("int64 milliseconds() const", &core::date_time::milliseconds);
				vdate_time->set_method("int64 seconds() const", &core::date_time::seconds);
				vdate_time->set_operator_extern(operators::add_assign_t, (uint32_t)position::left, "timestamp&", "const timestamp &in", &date_time_add_eq);
				vdate_time->set_operator_extern(operators::sub_assign_t, (uint32_t)position::left, "timestamp&", "const timestamp &in", &date_time_sub_eq);
				vdate_time->set_operator_extern(operators::equals_t, (uint32_t)position::constant, "bool", "const timestamp &in", &date_time_eq);
				vdate_time->set_operator_extern(operators::cmp_t, (uint32_t)position::constant, "int", "const timestamp &in", &date_time_cmp);
				vdate_time->set_operator_extern(operators::add_t, (uint32_t)position::constant, "timestamp", "const timestamp &in", &date_time_add);
				vdate_time->set_operator_extern(operators::sub_t, (uint32_t)position::constant, "timestamp", "const timestamp &in", &date_time_sub);
				vdate_time->set_method_static("timestamp from_nanoseconds(int64)", &core::date_time::from_nanoseconds);
				vdate_time->set_method_static("timestamp from_microseconds(int64)", &core::date_time::from_microseconds);
				vdate_time->set_method_static("timestamp from_milliseconds(int64)", &core::date_time::from_milliseconds);
				vdate_time->set_method_static("timestamp from_seconds(int64)", &core::date_time::from_seconds);
				vdate_time->set_method_static("timestamp from_serialized(const string_view&in, const string_view&in)", &core::date_time::from_serialized);
				vdate_time->set_method_static("string_view format_iso8601_time()", &core::date_time::format_iso8601_time);
				vdate_time->set_method_static("string_view format_web_time()", &core::date_time::format_web_time);
				vdate_time->set_method_static("string_view format_web_local_time()", &core::date_time::format_web_local_time);
				vdate_time->set_method_static("string_view format_compact_time()", &core::date_time::format_compact_time);
				return true;
			}
			bool registry::import_console(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				auto vstd_color = vm->set_enum("std_color");
				vstd_color->set_value("black", (int)core::std_color::black);
				vstd_color->set_value("dark_blue", (int)core::std_color::dark_blue);
				vstd_color->set_value("dark_green", (int)core::std_color::dark_green);
				vstd_color->set_value("light_blue", (int)core::std_color::light_blue);
				vstd_color->set_value("dark_red", (int)core::std_color::dark_red);
				vstd_color->set_value("magenta", (int)core::std_color::magenta);
				vstd_color->set_value("orange", (int)core::std_color::orange);
				vstd_color->set_value("light_gray", (int)core::std_color::light_gray);
				vstd_color->set_value("gray", (int)core::std_color::gray);
				vstd_color->set_value("blue", (int)core::std_color::blue);
				vstd_color->set_value("green", (int)core::std_color::green);
				vstd_color->set_value("cyan", (int)core::std_color::cyan);
				vstd_color->set_value("red", (int)core::std_color::red);
				vstd_color->set_value("pink", (int)core::std_color::pink);
				vstd_color->set_value("yellow", (int)core::std_color::yellow);
				vstd_color->set_value("white", (int)core::std_color::white);
				vstd_color->set_value("zero", (int)core::std_color::zero);

				auto vconsole = vm->set_class<core::console>("console", false);
				vconsole->set_method("void hide()", &core::console::hide);
				vconsole->set_method("void show()", &core::console::show);
				vconsole->set_method("void clear()", &core::console::clear);
				vconsole->set_method("void allocate()", &core::console::allocate);
				vconsole->set_method("void deallocate()", &core::console::deallocate);
				vconsole->set_method("void attach()", &core::console::attach);
				vconsole->set_method("void detach()", &core::console::detach);
				vconsole->set_method("void set_colorization(bool)", &core::console::set_colorization);
				vconsole->set_method("void write_color(std_color, std_color = std_color::zero)", &core::console::write_color);
				vconsole->set_method("void clear_color()", &core::console::clear_color);
				vconsole->set_method("void capture_time()", &core::console::capture_time);
				vconsole->set_method("uint64 capture_window(uint32)", &core::console::capture_window);
				vconsole->set_method("void free_window(uint64, bool)", &core::console::free_window);
				vconsole->set_method("void emplace_window(uint64, const string_view&in)", &core::console::emplace_window);
				vconsole->set_method("uint64 capture_element()", &core::console::capture_element);
				vconsole->set_method("void free_element(uint64)", &core::console::free_element);
				vconsole->set_method("void resize_element(uint64, uint32)", &core::console::resize_element);
				vconsole->set_method("void move_element(uint64, uint32)", &core::console::move_element);
				vconsole->set_method("void read_element(uint64, uint32&out, uint32&out)", &core::console::read_element);
				vconsole->set_method("void replace_element(uint64, const string_view&in)", &core::console::replace_element);
				vconsole->set_method("void spinning_element(uint64, const string_view&in)", &core::console::spinning_element);
				vconsole->set_method("void progress_element(uint64, double, double = 0.8)", &core::console::progress_element);
				vconsole->set_method("void spinning_progress_element(uint64, double, double = 0.8)", &core::console::spinning_progress_element);
				vconsole->set_method("void clear_element(uint64)", &core::console::clear_element);
				vconsole->set_method("void flush()", &core::console::flush);
				vconsole->set_method("void flush_write()", &core::console::flush_write);
				vconsole->set_method("void write_size(uint32, uint32)", &core::console::write_size);
				vconsole->set_method("void write_position(uint32, uint32)", &core::console::write_position);
				vconsole->set_method("void write_line(const string_view&in)", &core::console::write_line);
				vconsole->set_method("void write_char(uint8)", &core::console::write_char);
				vconsole->set_method("void write(const string_view&in)", &core::console::write);
				vconsole->set_method("void jwrite_line(schema@+)", &core::console::jwrite_line);
				vconsole->set_method("void jwrite(schema@+)", &core::console::jwrite);
				vconsole->set_method("double get_captured_time()", &core::console::get_captured_time);
				vconsole->set_method("string read(usize)", &core::console::read);
				vconsole->set_method("bool read_screen(uint32 &out, uint32 &out, uint32 &out, uint32 &out)", &core::console::read_screen);
				vconsole->set_method("bool read_line(string&out, usize)", &core::console::read_line);
				vconsole->set_method("uint8 read_char()", &core::console::read_char);
				vconsole->set_method_static("console@+ get()", &core::console::get);
				vconsole->set_method_extern("void trace(uint32 = 32)", &console_trace);

				return true;
#else
				VI_ASSERT(false, "<console> is not loaded");
				return false;
#endif
			}
			bool registry::import_schema(virtual_machine* vm) noexcept
			{
				VI_ASSERT(vm != nullptr, "manager should be set");
				VI_TYPEREF(schema, "schema");

				auto vschema = vm->set_class<core::schema>("schema", true);
				vschema->set_property<core::schema>("string key", &core::schema::key);
				vschema->set_property<core::schema>("variant value", &core::schema::value);
				vschema->set_constructor_extern<core::schema, core::schema*>("schema@ f(schema@+)", &schema_construct_copy);
				vschema->set_gc_constructor<core::schema, schema, const core::variant&>("schema@ f(const variant &in)");
				vschema->set_gc_constructor_list_extern<core::schema>("schema@ f(int &in) { repeat { string, ? } }", &schema_construct);
				vschema->set_method<core::schema, core::variant, size_t>("variant get_var(usize) const", &core::schema::get_var);
				vschema->set_method<core::schema, core::variant, const std::string_view&>("variant get_var(const string_view&in) const", &core::schema::get_var);
				vschema->set_method<core::schema, core::variant, const std::string_view&>("variant get_attribute_var(const string_view&in) const", &core::schema::get_attribute_var);
				vschema->set_method("schema@+ get_parent() const", &core::schema::get_parent);
				vschema->set_method("schema@+ get_attribute(const string_view&in) const", &core::schema::get_attribute);
				vschema->set_method<core::schema, core::schema*, size_t>("schema@+ get(usize) const", &core::schema::get);
				vschema->set_method<core::schema, core::schema*, const std::string_view&, bool>("schema@+ get(const string_view&in, bool = false) const", &core::schema::fetch);
				vschema->set_method<core::schema, core::schema*, const std::string_view&>("schema@+ set(const string_view&in)", &core::schema::set);
				vschema->set_method<core::schema, core::schema*, const std::string_view&, const core::variant&>("schema@+ set(const string_view&in, const variant &in)", &core::schema::set);
				vschema->set_method<core::schema, core::schema*, const std::string_view&, const core::variant&>("schema@+ set_attribute(const string& in, const variant &in)", &core::schema::set_attribute);
				vschema->set_method<core::schema, core::schema*, const core::variant&>("schema@+ push(const variant &in)", &core::schema::push);
				vschema->set_method<core::schema, core::schema*, size_t>("schema@+ pop(usize)", &core::schema::pop);
				vschema->set_method<core::schema, core::schema*, const std::string_view&>("schema@+ pop(const string_view&in)", &core::schema::pop);
				vschema->set_method("schema@ copy() const", &core::schema::copy);
				vschema->set_method("bool has(const string_view&in) const", &core::schema::has);
				vschema->set_method("bool has_attribute(const string_view&in) const", &core::schema::has_attribute);
				vschema->set_method("bool empty() const", &core::schema::empty);
				vschema->set_method("bool is_attribute() const", &core::schema::is_attribute);
				vschema->set_method("bool is_saved() const", &core::schema::is_attribute);
				vschema->set_method("string get_name() const", &core::schema::get_name);
				vschema->set_method("void join(schema@+, bool)", &core::schema::join);
				vschema->set_method("void unlink()", &core::schema::unlink);
				vschema->set_method("void clear()", &core::schema::clear);
				vschema->set_method("void save()", &core::schema::save);
				vschema->set_method_extern("variant first_var() const", &schema_first_var);
				vschema->set_method_extern("variant last_var() const", &schema_last_var);
				vschema->set_method_extern("schema@+ first() const", &schema_first);
				vschema->set_method_extern("schema@+ last() const", &schema_last);
				vschema->set_method_extern("schema@+ set(const string_view&in, schema@+)", &schema_set);
				vschema->set_method_extern("schema@+ push(schema@+)", &schema_push);
				vschema->set_method_extern("array<schema@>@ get_collection(const string_view&in, bool = false) const", &schema_get_collection);
				vschema->set_method_extern("array<schema@>@ get_attributes() const", &schema_get_attributes);
				vschema->set_method_extern("array<schema@>@ get_childs() const", &schema_get_childs);
				vschema->set_method_extern("dictionary@ get_names() const", &schema_get_names);
				vschema->set_method_extern("usize size() const", &schema_get_size);
				vschema->set_method_extern("string to_json() const", &schema_to_json);
				vschema->set_method_extern("string to_xml() const", &schema_to_xml);
				vschema->set_method_extern("string to_string() const", &schema_to_string);
				vschema->set_method_extern("string to_binary() const", &schema_to_binary);
				vschema->set_method_extern("int8 to_int8() const", &schema_to_int8);
				vschema->set_method_extern("uint8 to_uint8() const", &schema_to_uint8);
				vschema->set_method_extern("int16 to_int16() const", &schema_to_int16);
				vschema->set_method_extern("uint16 to_uint16() const", &schema_to_uint16);
				vschema->set_method_extern("int32 to_int32() const", &schema_to_int32);
				vschema->set_method_extern("uint32 to_uint32() const", &schema_to_uint32);
				vschema->set_method_extern("int64 to_int64() const", &schema_to_int64);
				vschema->set_method_extern("uint64 to_uint64() const", &schema_to_uint64);
				vschema->set_method_extern("double to_number() const", &schema_to_number);
				vschema->set_method_extern("decimal to_decimal() const", &schema_to_decimal);
				vschema->set_method_extern("bool to_boolean() const", &schema_to_boolean);
				vschema->set_method_static("schema@ from_json(const string_view&in)", &schema_from_json);
				vschema->set_method_static("schema@ from_xml(const string_view&in)", &schema_from_xml);
				vschema->set_method_static("schema@ import_json(const string_view&in)", &schema_import);
				vschema->set_operator_extern(operators::assign_t, (uint32_t)position::left, "schema@+", "const variant &in", &schema_copy_assign1);
				vschema->set_operator_extern(operators::assign_t, (uint32_t)position::left, "schema@+", "schema@+", &schema_copy_assign2);
				vschema->set_operator_extern(operators::equals_t, (uint32_t)(position::left | position::constant), "bool", "schema@+", &schema_equals);
				vschema->set_operator_extern(operators::index_t, (uint32_t)position::left, "schema@+", "const string_view&in", &schema_get_index);
				vschema->set_operator_extern(operators::index_t, (uint32_t)position::left, "schema@+", "usize", &schema_get_index_offset);
				vschema->set_enum_refs_extern<core::schema>([](core::schema* base, asIScriptEngine*) { });
				vschema->set_release_refs_extern<core::schema>([](core::schema* base, asIScriptEngine*) { });

				vm->begin_namespace("var::set");
				vm->set_function("schema@ auto_t(const string_view&in, bool = false)", &core::var::any);
				vm->set_function("schema@ null_t()", &core::var::set::null);
				vm->set_function("schema@ undefined_t()", &core::var::set::undefined);
				vm->set_function("schema@ object_t()", &core::var::set::object);
				vm->set_function("schema@ array_t()", &core::var::set::array);
				vm->set_function("schema@ pointer_t(uptr@)", &core::var::set::pointer);
				vm->set_function("schema@ integer_t(int64)", &core::var::set::integer);
				vm->set_function("schema@ number_t(double)", &core::var::set::number);
				vm->set_function("schema@ boolean_t(bool)", &core::var::set::boolean);
				vm->set_function<core::schema* (const std::string_view&)>("schema@ string_t(const string_view&in)", &core::var::set::string);
				vm->set_function<core::schema* (const std::string_view&)>("schema@ binary_t(const string_view&in)", &core::var::set::binary);
				vm->set_function<core::schema* (const std::string_view&)>("schema@ decimal_t(const string_view&in)", &core::var::set::decimal_string);
				vm->set_function<core::schema* (const core::decimal&)>("schema@ decimal_t(const decimal &in)", &core::var::set::decimal);
				vm->set_function("schema@+ as(schema@+)", &schema_init);
				vm->end_namespace();

				return true;
			}
			bool registry::import_clock_timer(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");

				auto vtimer = vm->set_class<core::timer>("clock_timer", false);
				vtimer->set_constructor<core::timer>("clock_timer@ f()");
				vtimer->set_method("void set_fixed_frames(float)", &core::timer::set_fixed_frames);
				vtimer->set_method("void begin()", &core::timer::begin);
				vtimer->set_method("void finish()", &core::timer::finish);
				vtimer->set_method("usize get_frame_index() const", &core::timer::get_frame_index);
				vtimer->set_method("usize get_fixed_frame_index() const", &core::timer::get_fixed_frame_index);
				vtimer->set_method("float get_max_frames() const", &core::timer::get_max_frames);
				vtimer->set_method("float get_min_step() const", &core::timer::get_min_step);
				vtimer->set_method("float get_frames() const", &core::timer::get_frames);
				vtimer->set_method("float get_elapsed() const", &core::timer::get_elapsed);
				vtimer->set_method("float get_elapsed_mills() const", &core::timer::get_elapsed_mills);
				vtimer->set_method("float get_step() const", &core::timer::get_step);
				vtimer->set_method("float get_fixed_step() const", &core::timer::get_fixed_step);
				vtimer->set_method("bool is_fixed() const", &core::timer::is_fixed);

				return true;
#else
				VI_ASSERT(false, "<clock_timer> is not loaded");
				return false;
#endif
			}
			bool registry::import_file_system(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				auto vfile_mode = vm->set_enum("file_mode");
				vfile_mode->set_value("read_only", (int)core::file_mode::read_only);
				vfile_mode->set_value("write_only", (int)core::file_mode::write_only);
				vfile_mode->set_value("append_only", (int)core::file_mode::append_only);
				vfile_mode->set_value("read_write", (int)core::file_mode::read_write);
				vfile_mode->set_value("write_read", (int)core::file_mode::write_read);
				vfile_mode->set_value("read_append_write", (int)core::file_mode::read_append_write);
				vfile_mode->set_value("binary_read_only", (int)core::file_mode::binary_read_only);
				vfile_mode->set_value("binary_write_only", (int)core::file_mode::binary_write_only);
				vfile_mode->set_value("binary_append_only", (int)core::file_mode::binary_append_only);
				vfile_mode->set_value("binary_read_write", (int)core::file_mode::binary_read_write);
				vfile_mode->set_value("binary_write_read", (int)core::file_mode::binary_write_read);
				vfile_mode->set_value("binary_read_append_write", (int)core::file_mode::binary_read_append_write);

				auto vfile_seek = vm->set_enum("file_seek");
				vfile_seek->set_value("begin", (int)core::file_seek::begin);
				vfile_seek->set_value("current", (int)core::file_seek::current);
				vfile_seek->set_value("end", (int)core::file_seek::end);

				auto vfile_state = vm->set_pod<core::file_state>("file_state");
				vfile_state->set_property("usize size", &core::file_state::size);
				vfile_state->set_property("usize links", &core::file_state::links);
				vfile_state->set_property("usize permissions", &core::file_state::permissions);
				vfile_state->set_property("usize document", &core::file_state::document);
				vfile_state->set_property("usize device", &core::file_state::device);
				vfile_state->set_property("usize userId", &core::file_state::user_id);
				vfile_state->set_property("usize groupId", &core::file_state::group_id);
				vfile_state->set_property("int64 last_access", &core::file_state::last_access);
				vfile_state->set_property("int64 last_permission_change", &core::file_state::last_permission_change);
				vfile_state->set_property("int64 last_modified", &core::file_state::last_modified);
				vfile_state->set_property("bool exists", &core::file_state::exists);
				vfile_state->set_constructor<core::file_state>("void f()");

				auto vfile_entry = vm->set_struct_trivial<core::file_entry>("file_entry");
				vfile_entry->set_constructor<core::file_entry>("void f()");
				vfile_entry->set_property("usize size", &core::file_entry::size);
				vfile_entry->set_property("int64 last_modified", &core::file_entry::last_modified);
				vfile_entry->set_property("int64 creation_time", &core::file_entry::creation_time);
				vfile_entry->set_property("bool is_referenced", &core::file_entry::is_referenced);
				vfile_entry->set_property("bool is_directory", &core::file_entry::is_directory);
				vfile_entry->set_property("bool is_exists", &core::file_entry::is_exists);

				auto vfile_link = vm->set_struct_trivial<file_link>("file_link");
				vfile_link->set_constructor<file_link>("void f()");
				vfile_link->set_property("string path", &file_link::path);
				vfile_link->set_property("usize size", &file_link::size);
				vfile_link->set_property("int64 last_modified", &file_link::last_modified);
				vfile_link->set_property("int64 creation_time", &file_link::creation_time);
				vfile_link->set_property("bool is_referenced", &file_link::is_referenced);
				vfile_link->set_property("bool is_directory", &file_link::is_directory);
				vfile_link->set_property("bool is_exists", &file_link::is_exists);

				auto vstream = vm->set_class<core::stream>("base_stream", false);
				vstream->set_method_extern("bool clear()", &VI_EXPECTIFY_VOID(core::stream::clear));
				vstream->set_method_extern("bool close()", &VI_EXPECTIFY_VOID(core::stream::close));
				vstream->set_method_extern("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(core::stream::seek));
				vstream->set_method_extern("bool move(int64)", &VI_EXPECTIFY_VOID(core::stream::move));
				vstream->set_method_extern("bool flush()", &VI_EXPECTIFY_VOID(core::stream::flush));
				vstream->set_method_extern("usize size()", &VI_EXPECTIFY(core::stream::size));
				vstream->set_method_extern("usize tell()", &VI_EXPECTIFY(core::stream::tell));
				vstream->set_method("usize virtual_size() const", &core::stream::virtual_size);
				vstream->set_method("const string& virtual_name() const", &core::stream::virtual_name);
				vstream->set_method("void set_virtual_size(usize) const", &core::stream::set_virtual_size);
				vstream->set_method("void set_virtual_name(const string_view&in) const", &core::stream::set_virtual_name);
				vstream->set_method("int32 get_readable_fd() const", &core::stream::get_readable_fd);
				vstream->set_method("int32 get_writeable_fd() const", &core::stream::get_writeable_fd);
				vstream->set_method("uptr@ get_readable() const", &core::stream::get_readable);
				vstream->set_method("uptr@ get_writeable() const", &core::stream::get_writeable);
				vstream->set_method("bool is_sized() const", &core::stream::is_sized);
				vstream->set_method_extern("bool open(const string_view&in, file_mode)", &stream_open);
				vstream->set_method_extern("usize write(const string_view&in)", &stream_write);
				vstream->set_method_extern("string read(usize)", &stream_read);
				vstream->set_method_extern("string read_line(usize)", &stream_read_line);

				auto vmemory_stream = vm->set_class<core::memory_stream>("memory_stream", false);
				vmemory_stream->set_constructor<core::memory_stream>("memory_stream@ f()");
				vmemory_stream->set_method_extern("bool clear()", &VI_EXPECTIFY_VOID(core::memory_stream::clear));
				vmemory_stream->set_method_extern("bool close()", &VI_EXPECTIFY_VOID(core::memory_stream::close));
				vmemory_stream->set_method_extern("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(core::memory_stream::seek));
				vmemory_stream->set_method_extern("bool move(int64)", &VI_EXPECTIFY_VOID(core::memory_stream::move));
				vmemory_stream->set_method_extern("bool flush()", &VI_EXPECTIFY_VOID(core::memory_stream::flush));
				vmemory_stream->set_method_extern("usize size()", &VI_EXPECTIFY(core::memory_stream::size));
				vmemory_stream->set_method_extern("usize tell()", &VI_EXPECTIFY(core::memory_stream::tell));
				vmemory_stream->set_method("usize virtual_size() const", &core::memory_stream::virtual_size);
				vmemory_stream->set_method("const string& virtual_name() const", &core::memory_stream::virtual_name);
				vmemory_stream->set_method("void set_virtual_size(usize) const", &core::memory_stream::set_virtual_size);
				vmemory_stream->set_method("void set_virtual_name(const string_view&in) const", &core::memory_stream::set_virtual_name);
				vmemory_stream->set_method("int32 get_readable_fd() const", &core::memory_stream::get_readable_fd);
				vmemory_stream->set_method("int32 get_writeable_fd() const", &core::memory_stream::get_writeable_fd);
				vmemory_stream->set_method("uptr@ get_readable() const", &core::memory_stream::get_readable);
				vmemory_stream->set_method("uptr@ get_writeable() const", &core::memory_stream::get_writeable);
				vmemory_stream->set_method("bool is_sized() const", &core::memory_stream::is_sized);
				vmemory_stream->set_method_extern("bool open(const string_view&in, file_mode)", &stream_open);
				vmemory_stream->set_method_extern("usize write(const string_view&in)", &stream_write);
				vmemory_stream->set_method_extern("string read(usize)", &stream_read);
				vmemory_stream->set_method_extern("string read_line(usize)", &stream_read_line);

				auto vfile_stream = vm->set_class<core::file_stream>("file_stream", false);
				vfile_stream->set_constructor<core::file_stream>("file_stream@ f()");
				vfile_stream->set_method_extern("bool clear()", &VI_EXPECTIFY_VOID(core::file_stream::clear));
				vfile_stream->set_method_extern("bool close()", &VI_EXPECTIFY_VOID(core::file_stream::close));
				vfile_stream->set_method_extern("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(core::file_stream::seek));
				vfile_stream->set_method_extern("bool move(int64)", &VI_EXPECTIFY_VOID(core::file_stream::move));
				vfile_stream->set_method_extern("bool flush()", &VI_EXPECTIFY_VOID(core::file_stream::flush));
				vfile_stream->set_method_extern("usize size()", &VI_EXPECTIFY(core::file_stream::size));
				vfile_stream->set_method_extern("usize tell()", &VI_EXPECTIFY(core::file_stream::tell));
				vfile_stream->set_method("usize virtual_size() const", &core::file_stream::virtual_size);
				vfile_stream->set_method("const string& virtual_name() const", &core::file_stream::virtual_name);
				vfile_stream->set_method("void set_virtual_size(usize) const", &core::file_stream::set_virtual_size);
				vfile_stream->set_method("void set_virtual_name(const string_view&in) const", &core::file_stream::set_virtual_name);
				vfile_stream->set_method("int32 get_readable_fd() const", &core::file_stream::get_readable_fd);
				vfile_stream->set_method("int32 get_writeable_fd() const", &core::file_stream::get_writeable_fd);
				vfile_stream->set_method("uptr@ get_readable() const", &core::file_stream::get_readable);
				vfile_stream->set_method("uptr@ get_writeable() const", &core::file_stream::get_writeable);
				vfile_stream->set_method("bool is_sized() const", &core::file_stream::is_sized);
				vfile_stream->set_method_extern("bool open(const string_view&in, file_mode)", &stream_open);
				vfile_stream->set_method_extern("usize write(const string_view&in)", &stream_write);
				vfile_stream->set_method_extern("string read(usize)", &stream_read);
				vfile_stream->set_method_extern("string read_line(usize)", &stream_read_line);

				auto vgz_stream = vm->set_class<core::gz_stream>("gz_stream", false);
				vgz_stream->set_constructor<core::gz_stream>("gz_stream@ f()");
				vgz_stream->set_method_extern("bool clear()", &VI_EXPECTIFY_VOID(core::gz_stream::clear));
				vgz_stream->set_method_extern("bool close()", &VI_EXPECTIFY_VOID(core::gz_stream::close));
				vgz_stream->set_method_extern("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(core::gz_stream::seek));
				vgz_stream->set_method_extern("bool move(int64)", &VI_EXPECTIFY_VOID(core::gz_stream::move));
				vgz_stream->set_method_extern("bool flush()", &VI_EXPECTIFY_VOID(core::gz_stream::flush));
				vgz_stream->set_method_extern("usize size()", &VI_EXPECTIFY(core::gz_stream::size));
				vgz_stream->set_method_extern("usize tell()", &VI_EXPECTIFY(core::gz_stream::tell));
				vgz_stream->set_method("usize virtual_size() const", &core::gz_stream::virtual_size);
				vgz_stream->set_method("const string& virtual_name() const", &core::gz_stream::virtual_name);
				vgz_stream->set_method("void set_virtual_size(usize) const", &core::gz_stream::set_virtual_size);
				vgz_stream->set_method("void set_virtual_name(const string_view&in) const", &core::gz_stream::set_virtual_name);
				vgz_stream->set_method("int32 get_readable_fd() const", &core::gz_stream::get_readable_fd);
				vgz_stream->set_method("int32 get_writeable_fd() const", &core::gz_stream::get_writeable_fd);
				vgz_stream->set_method("uptr@ get_readable() const", &core::gz_stream::get_readable);
				vgz_stream->set_method("uptr@ get_writeable() const", &core::gz_stream::get_writeable);
				vgz_stream->set_method("bool is_sized() const", &core::gz_stream::is_sized);
				vgz_stream->set_method_extern("bool open(const string_view&in, file_mode)", &stream_open);
				vgz_stream->set_method_extern("usize write(const string_view&in)", &stream_write);
				vgz_stream->set_method_extern("string read(usize)", &stream_read);
				vgz_stream->set_method_extern("string read_line(usize)", &stream_read_line);

				auto vweb_stream = vm->set_class<core::web_stream>("web_stream", false);
				vweb_stream->set_constructor<core::web_stream, bool>("web_stream@ f(bool)");
				vweb_stream->set_method_extern("bool clear()", &VI_EXPECTIFY_VOID(core::web_stream::clear));
				vweb_stream->set_method_extern("bool close()", &VI_EXPECTIFY_VOID(core::web_stream::close));
				vweb_stream->set_method_extern("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(core::web_stream::seek));
				vweb_stream->set_method_extern("bool move(int64)", &VI_EXPECTIFY_VOID(core::web_stream::move));
				vweb_stream->set_method_extern("bool flush()", &VI_EXPECTIFY_VOID(core::web_stream::flush));
				vweb_stream->set_method_extern("usize size()", &VI_EXPECTIFY(core::web_stream::size));
				vweb_stream->set_method_extern("usize tell()", &VI_EXPECTIFY(core::web_stream::tell));
				vweb_stream->set_method("usize virtual_size() const", &core::web_stream::virtual_size);
				vweb_stream->set_method("const string& virtual_name() const", &core::web_stream::virtual_name);
				vweb_stream->set_method("void set_virtual_size(usize) const", &core::web_stream::set_virtual_size);
				vweb_stream->set_method("void set_virtual_name(const string_view&in) const", &core::web_stream::set_virtual_name);
				vweb_stream->set_method("int32 get_readable_fd() const", &core::web_stream::get_readable_fd);
				vweb_stream->set_method("int32 get_writeable_fd() const", &core::web_stream::get_writeable_fd);
				vweb_stream->set_method("uptr@ get_readable() const", &core::web_stream::get_readable);
				vweb_stream->set_method("uptr@ get_writeable() const", &core::web_stream::get_writeable);
				vweb_stream->set_method("bool is_sized() const", &core::web_stream::is_sized);
				vweb_stream->set_method_extern("bool open(const string_view&in, file_mode)", &stream_open);
				vweb_stream->set_method_extern("usize write(const string_view&in)", &stream_write);
				vweb_stream->set_method_extern("string read(usize)", &stream_read);
				vweb_stream->set_method_extern("string read_line(usize)", &stream_read_line);

				auto vprocess_stream = vm->set_class<core::process_stream>("process_stream", false);
				vprocess_stream->set_constructor<core::process_stream>("process_stream@ f()");
				vprocess_stream->set_method_extern("bool clear()", &VI_EXPECTIFY_VOID(core::process_stream::clear));
				vprocess_stream->set_method_extern("bool close()", &VI_EXPECTIFY_VOID(core::process_stream::close));
				vprocess_stream->set_method_extern("bool seek(file_seek, int64)", &VI_EXPECTIFY_VOID(core::process_stream::seek));
				vprocess_stream->set_method_extern("bool move(int64)", &VI_EXPECTIFY_VOID(core::process_stream::move));
				vprocess_stream->set_method_extern("bool flush()", &VI_EXPECTIFY_VOID(core::process_stream::flush));
				vprocess_stream->set_method_extern("usize size()", &VI_EXPECTIFY(core::process_stream::size));
				vprocess_stream->set_method_extern("usize tell()", &VI_EXPECTIFY(core::process_stream::tell));
				vprocess_stream->set_method("usize virtual_size() const", &core::process_stream::virtual_size);
				vprocess_stream->set_method("const string& virtual_name() const", &core::process_stream::virtual_name);
				vprocess_stream->set_method("void set_virtual_size(usize) const", &core::process_stream::set_virtual_size);
				vprocess_stream->set_method("void set_virtual_name(const string_view&in) const", &core::process_stream::set_virtual_name);
				vprocess_stream->set_method("int32 get_readable_fd() const", &core::process_stream::get_readable_fd);
				vprocess_stream->set_method("int32 get_writeable_fd() const", &core::process_stream::get_writeable_fd);
				vprocess_stream->set_method("uptr@ get_readable() const", &core::process_stream::get_readable);
				vprocess_stream->set_method("uptr@ get_writeable() const", &core::process_stream::get_writeable);
				vprocess_stream->set_method("usize get_process_id() const", &core::process_stream::get_process_id);
				vprocess_stream->set_method("usize get_thread_id() const", &core::process_stream::get_thread_id);
				vprocess_stream->set_method("int32 get_exit_code() const", &core::process_stream::get_exit_code);
				vprocess_stream->set_method("bool is_sized() const", &core::process_stream::is_sized);
				vprocess_stream->set_method("bool is_alive()", &core::process_stream::is_alive);
				vprocess_stream->set_method_extern("bool open(const string_view&in, file_mode)", &stream_open);
				vprocess_stream->set_method_extern("usize write(const string_view&in)", &stream_write);
				vprocess_stream->set_method_extern("string read(usize)", &stream_read);
				vprocess_stream->set_method_extern("string read_line(usize)", &stream_read_line);

				vstream->set_dynamic_cast<core::stream, core::memory_stream>("memory_stream@+");
				vstream->set_dynamic_cast<core::stream, core::file_stream>("file_stream@+");
				vstream->set_dynamic_cast<core::stream, core::gz_stream>("gz_stream@+");
				vstream->set_dynamic_cast<core::stream, core::web_stream>("web_stream@+");
				vstream->set_dynamic_cast<core::stream, core::process_stream>("process_stream@+");
				vmemory_stream->set_dynamic_cast<core::memory_stream, core::stream>("base_stream@+", true);
				vfile_stream->set_dynamic_cast<core::file_stream, core::stream>("base_stream@+", true);
				vgz_stream->set_dynamic_cast<core::gz_stream, core::stream>("base_stream@+", true);
				vweb_stream->set_dynamic_cast<core::web_stream, core::stream>("base_stream@+", true);
				vprocess_stream->set_dynamic_cast<core::process_stream, core::stream>("base_stream@+", true);
				return true;
#else
				VI_ASSERT(false, "<file_system> is not loaded");
				return false;
#endif
			}
			bool registry::import_os(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				auto vargs_format = vm->set_enum("args_format");
				vargs_format->set_value("key_value", (int)core::args_format::key_value);
				vargs_format->set_value("flag_value", (int)core::args_format::flag_value);
				vargs_format->set_value("key", (int)core::args_format::key);
				vargs_format->set_value("flag", (int)core::args_format::flag);
				vargs_format->set_value("stop_if_no_match", (int)core::args_format::stop_if_no_match);

				auto vinline_args = vm->set_struct_trivial<core::inline_args>("inline_args");
				vinline_args->set_property<core::inline_args>("string path", &core::inline_args::path);
				vinline_args->set_constructor<core::inline_args>("void f()");
				vinline_args->set_method("bool is_enabled(const string_view&in, const string_view&in = string_view()) const", &core::inline_args::is_enabled);
				vinline_args->set_method("bool is_disabled(const string_view&in, const string_view&in = string_view()) const", &core::inline_args::is_disabled);
				vinline_args->set_method("bool has(const string_view&in, const string_view&in = string_view()) const", &core::inline_args::has);
				vinline_args->set_method("string& get(const string_view&in, const string_view&in = string_view()) const", &core::inline_args::get);
				vinline_args->set_method("string& get_if(const string_view&in, const string_view&in, const string_view&in) const", &core::inline_args::get_if);
				vinline_args->set_method_extern("dictionary@ get_args() const", &inline_args_get_args);
				vinline_args->set_method_extern("array<string>@ get_params() const", &inline_args_get_params);

				vm->begin_namespace("os::hw");
				auto varch = vm->set_enum("arch");
				varch->set_value("x64", (int)core::os::hw::arch::x64);
				varch->set_value("arm", (int)core::os::hw::arch::arm);
				varch->set_value("itanium", (int)core::os::hw::arch::itanium);
				varch->set_value("x86", (int)core::os::hw::arch::x86);
				varch->set_value("unknown", (int)core::os::hw::arch::unknown);

				auto vendian = vm->set_enum("endian");
				vendian->set_value("little", (int)core::os::hw::endian::little);
				vendian->set_value("big", (int)core::os::hw::endian::big);

				auto vcache = vm->set_enum("cache");
				vcache->set_value("unified", (int)core::os::hw::cache::unified);
				vcache->set_value("instruction", (int)core::os::hw::cache::instruction);
				vcache->set_value("data", (int)core::os::hw::cache::data);
				vcache->set_value("trace", (int)core::os::hw::cache::trace);

				auto vquantity_info = vm->set_pod<core::os::hw::quantity_info>("quantity_info", (size_t)object_behaviours::app_class_allints);
				vquantity_info->set_property("uint32 logical", &core::os::hw::quantity_info::logical);
				vquantity_info->set_property("uint32 physical", &core::os::hw::quantity_info::physical);
				vquantity_info->set_property("uint32 packages", &core::os::hw::quantity_info::packages);
				vquantity_info->set_constructor<core::os::hw::quantity_info>("void f()");

				auto vcache_info = vm->set_pod<core::os::hw::cache_info>("cache_info");
				vcache_info->set_property("usize size", &core::os::hw::cache_info::size);
				vcache_info->set_property("usize line_size", &core::os::hw::cache_info::line_size);
				vcache_info->set_property("uint8 associativity", &core::os::hw::cache_info::associativity);
				vcache_info->set_property("cache type", &core::os::hw::cache_info::type);
				vcache_info->set_constructor<core::os::hw::cache_info>("void f()");

				vm->set_function("quantity_info get_quantity_info()", &core::os::hw::get_quantity_info);
				vm->set_function("cache_info get_cache_info(uint32)", &core::os::hw::get_cache_info);
				vm->set_function("arch get_arch()", &core::os::hw::get_arch);
				vm->set_function("endian get_endianness()", &core::os::hw::get_endianness);
				vm->set_function("usize get_frequency()", &core::os::hw::get_frequency);
				vm->end_namespace();

				vm->begin_namespace("os::directory");
				vm->set_function("array<file_state>@ scan(const string_view&in)", &os_directory_scan);
				vm->set_function("array<string>@ get_mounts(const string_view&in)", &os_directory_get_mounts);
				vm->set_function("bool create(const string_view&in)", &os_directory_create);
				vm->set_function("bool remove(const string_view&in)", &os_directory_remove);
				vm->set_function("bool is_exists(const string_view&in)", &os_directory_is_exists);
				vm->set_function("bool is_empty(const string_view&in)", &os_directory_is_empty);
				vm->set_function("bool patch(const string_view&in)", &os_directory_patch);
				vm->set_function("bool set_working(const string_view&in)", &os_directory_set_working);
				vm->set_function("string get_module()", &VI_SEXPECTIFY(core::os::directory::get_module));
				vm->set_function("string get_working()", &VI_SEXPECTIFY(core::os::directory::get_working));
				vm->end_namespace();

				vm->begin_namespace("os::file");
				vm->set_function("bool write(const string_view&in, const string_view&in)", &os_file_write);
				vm->set_function("bool state(const string_view&in, file_entry &out)", &os_file_state);
				vm->set_function("bool move(const string_view&in, const string_view&in)", &os_file_move);
				vm->set_function("bool copy(const string_view&in, const string_view&in)", &os_file_copy);
				vm->set_function("bool remove(const string_view&in)", &os_file_remove);
				vm->set_function("bool is_exists(const string_view&in)", &os_file_is_exists);
				vm->set_function("file_state get_properties(const string_view&in)", &os_file_get_properties);
				vm->set_function("array<string>@ read_as_array(const string_view&in)", &os_file_read_as_array);
				vm->set_function("usize join(const string_view&in, array<string>@+)", &os_file_join);
				vm->set_function("int32 compare(const string_view&in, const string_view&in)", &core::os::file::compare);
				vm->set_function("int64 get_hash(const string_view&in)", &core::os::file::get_hash);
				vm->set_function("base_stream@ open_join(const string_view&in, array<string>@+)", &os_file_open_join);
				vm->set_function("base_stream@ open_archive(const string_view&in, usize = 134217728)", &os_file_open_archive);
				vm->set_function("base_stream@ open(const string_view&in, file_mode, bool = false)", &os_file_open);
				vm->set_function("string read_as_string(const string_view&in)", &VI_SEXPECTIFY(core::os::file::read_as_string));
				vm->end_namespace();

				vm->begin_namespace("os::path");
				vm->set_function("bool is_remote(const string_view&in)", &os_path_is_remote);
				vm->set_function("bool is_relative(const string_view&in)", &os_path_is_relative);
				vm->set_function("bool is_absolute(const string_view&in)", &os_path_is_absolute);
				vm->set_function("string resolve(const string_view&in)", &os_path_resolve1);
				vm->set_function("string resolve_directory(const string_view&in)", &os_path_resolve_directory1);
				vm->set_function("string get_directory(const string_view&in, usize = 0)", &os_path_get_directory);
				vm->set_function("string get_filename(const string_view&in)", &os_path_get_filename);
				vm->set_function("string get_extension(const string_view&in)", &os_path_get_extension);
				vm->set_function("string get_non_existant(const string_view&in)", &core::os::path::get_non_existant);
				vm->set_function("string resolve(const string_view&in, const string_view&in, bool)", &os_path_resolve2);
				vm->set_function("string resolve_directory(const string_view&in, const string_view&in, bool)", &os_path_resolve_directory2);
				vm->end_namespace();

				vm->begin_namespace("os::process");
				vm->set_function_def("void process_async(const string_view&in)");
				vm->set_function("void abort()", &core::os::process::abort);
				vm->set_function("void exit(int)", &core::os::process::exit);
				vm->set_function("void interrupt()", &core::os::process::interrupt);
				vm->set_function("string get_env(const string_view&in)", &VI_SEXPECTIFY(core::os::process::get_env));
				vm->set_function("string get_shell()", &VI_SEXPECTIFY(core::os::process::get_shell));
				vm->set_function("process_stream@+ spawn(const string_view&in, file_mode)", &VI_SEXPECTIFY(core::os::process::spawn));
				vm->set_function("int execute(const string_view&in, file_mode, process_async@)", &os_process_execute);
				vm->set_function("inline_args parse_args(array<string>@+, usize, array<string>@+ = null)", &os_process_parse_args);
				vm->end_namespace();

				vm->begin_namespace("os::symbol");
				vm->set_function("uptr@ load(const string_view&in)", &os_symbol_load);
				vm->set_function("uptr@ load_function(uptr@, const string_view&in)", &os_symbol_load_function);
				vm->set_function("bool unload(uptr@)", &os_symbol_unload);
				vm->end_namespace();

				vm->begin_namespace("os::error");
				vm->set_function("int32 get(bool = true)", &core::os::error::get);
				vm->set_function("void clear()", &core::os::error::clear);
				vm->set_function("bool occurred()", &core::os::error::occurred);
				vm->set_function("bool is_error(int32)", &core::os::error::is_error);
				vm->set_function("string get_name(int32)", &core::os::error::get_name);
				vm->end_namespace();

				if (vm->get_library_property(library_features::os_expose_control) > 0)
				{
					auto vfs_option = vm->set_enum("access_option");
					vfs_option->set_value("allow_mem", (int)core::access_option::mem);
					vfs_option->set_value("allow_fs", (int)core::access_option::fs);
					vfs_option->set_value("allow_gz", (int)core::access_option::gz);
					vfs_option->set_value("allow_net", (int)core::access_option::net);
					vfs_option->set_value("allow_lib", (int)core::access_option::lib);
					vfs_option->set_value("allow_http", (int)core::access_option::http);
					vfs_option->set_value("allow_https", (int)core::access_option::https);
					vfs_option->set_value("allow_shell", (int)core::access_option::shell);
					vfs_option->set_value("allow_env", (int)core::access_option::env);
					vfs_option->set_value("allow_addons", (int)core::access_option::addons);
					vfs_option->set_value("all", (int)core::access_option::all);

					vm->begin_namespace("os::control");
					vm->set_function("bool set(access_option, bool)", &core::os::control::set);
					vm->set_function("bool has(access_option)", &core::os::control::has);
					vm->end_namespace();
				}

				return true;
#else
				VI_ASSERT(false, "<os> is not loaded");
				return false;
#endif
			}
			bool registry::import_schedule(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				vm->set_type_def("task_id", "uint64");

				auto vdifficulty = vm->set_enum("difficulty");
				vdifficulty->set_value("async", (int)core::difficulty::async);
				vdifficulty->set_value("sync", (int)core::difficulty::sync);
				vdifficulty->set_value("timeout", (int)core::difficulty::timeout);

				auto vdesc = vm->set_struct_trivial<core::schedule::desc>("schedule_policy");
				vdesc->set_property("usize preallocated_size", &core::schedule::desc::preallocated_size);
				vdesc->set_property("usize stack_size", &core::schedule::desc::stack_size);
				vdesc->set_property("usize max_coroutines", &core::schedule::desc::max_coroutines);
				vdesc->set_property("usize max_recycles", &core::schedule::desc::max_recycles);
				vdesc->set_property("bool parallel", &core::schedule::desc::parallel);
				vdesc->set_constructor<core::schedule::desc>("void f()");
				vdesc->set_constructor<core::schedule::desc, size_t>("void f(usize)");

				auto vschedule = vm->set_class<core::schedule>("schedule", false);
				vschedule->set_function_def("void task_async()");
				vschedule->set_function_def("void task_parallel()");
				vschedule->set_method_extern("task_id set_interval(uint64, task_async@)", &schedule_set_interval);
				vschedule->set_method_extern("task_id set_timeout(uint64, task_async@)", &schedule_set_timeout);
				vschedule->set_method_extern("bool set_immediate(task_async@)", &schedule_set_immediate);
				vschedule->set_method_extern("bool spawn(task_parallel@)", &schedule_spawn);
				vschedule->set_method("bool clear_timeout(task_id)", &core::schedule::clear_timeout);
				vschedule->set_method("bool trigger_timers()", &core::schedule::trigger_timers);
				vschedule->set_method("bool trigger(difficulty)", &core::schedule::trigger);
				vschedule->set_method("bool dispatch()", &core::schedule::dispatch);
				vschedule->set_method("bool start(const schedule_policy &in)", &core::schedule::start);
				vschedule->set_method("bool stop()", &core::schedule::stop);
				vschedule->set_method("bool wakeup()", &core::schedule::wakeup);
				vschedule->set_method("bool is_active() const", &core::schedule::is_active);
				vschedule->set_method("bool can_enqueue() const", &core::schedule::can_enqueue);
				vschedule->set_method("bool has_tasks(difficulty) const", &core::schedule::has_tasks);
				vschedule->set_method("bool has_any_tasks() const", &core::schedule::has_any_tasks);
				vschedule->set_method("bool is_suspended() const", &core::schedule::is_suspended);
				vschedule->set_method("void suspend()", &core::schedule::suspend);
				vschedule->set_method("void resume()", &core::schedule::resume);
				vschedule->set_method("usize get_total_threads() const", &core::schedule::get_total_threads);
				vschedule->set_method("usize get_thread_global_index()", &core::schedule::get_thread_global_index);
				vschedule->set_method("usize get_thread_local_index()", &core::schedule::get_thread_local_index);
				vschedule->set_method("usize get_threads(difficulty) const", &core::schedule::get_threads);
				vschedule->set_method("bool has_parallel_threads(difficulty) const", &core::schedule::has_parallel_threads);
				vschedule->set_method("const schedule_policy& get_policy() const", &core::schedule::get_policy);
				vschedule->set_method_static("schedule@+ get()", &core::schedule::get);

				return true;
#else
				VI_ASSERT(false, "<schedule> is not loaded");
				return false;
#endif
			}
			bool registry::import_regex(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				auto vregex_state = vm->set_enum("regex_state");
				vregex_state->set_value("preprocessed", (int)compute::regex_state::preprocessed);
				vregex_state->set_value("match_found", (int)compute::regex_state::match_found);
				vregex_state->set_value("no_match", (int)compute::regex_state::no_match);
				vregex_state->set_value("unexpected_quantifier", (int)compute::regex_state::unexpected_quantifier);
				vregex_state->set_value("unbalanced_brackets", (int)compute::regex_state::unbalanced_brackets);
				vregex_state->set_value("internal_error", (int)compute::regex_state::internal_error);
				vregex_state->set_value("invalid_character_set", (int)compute::regex_state::invalid_character_set);
				vregex_state->set_value("invalid_metacharacter", (int)compute::regex_state::invalid_metacharacter);
				vregex_state->set_value("sumatch_array_too_small", (int)compute::regex_state::sumatch_array_too_small);
				vregex_state->set_value("too_many_branches", (int)compute::regex_state::too_many_branches);
				vregex_state->set_value("too_many_brackets", (int)compute::regex_state::too_many_brackets);

				auto vregex_match = vm->set_pod<compute::regex_match>("regex_match");
				vregex_match->set_property<compute::regex_match>("int64 start", &compute::regex_match::start);
				vregex_match->set_property<compute::regex_match>("int64 end", &compute::regex_match::end);
				vregex_match->set_property<compute::regex_match>("int64 length", &compute::regex_match::length);
				vregex_match->set_property<compute::regex_match>("int64 steps", &compute::regex_match::steps);
				vregex_match->set_constructor<compute::regex_match>("void f()");
				vregex_match->set_method_extern("string target() const", &regex_match_target);

				auto vregex_result = vm->set_struct_trivial<compute::regex_result>("regex_result");
				vregex_result->set_constructor<compute::regex_result>("void f()");
				vregex_result->set_method("bool empty() const", &compute::regex_result::empty);
				vregex_result->set_method("int64 get_steps() const", &compute::regex_result::get_steps);
				vregex_result->set_method("regex_state get_state() const", &compute::regex_result::get_state);
				vregex_result->set_method_extern("array<regex_match>@ get() const", &regex_result_get);
				vregex_result->set_method_extern("array<string>@ to_array() const", &regex_result_to_array);

				auto vregex_source = vm->set_struct_trivial<compute::regex_source>("regex_source");
				vregex_source->set_property<compute::regex_source>("bool ignoreCase", &compute::regex_source::ignore_case);
				vregex_source->set_constructor<compute::regex_source>("void f()");
				vregex_source->set_constructor<compute::regex_source, const std::string_view&, bool, int64_t, int64_t, int64_t>("void f(const string_view&in, bool = false, int64 = -1, int64 = -1, int64 = -1)");
				vregex_source->set_method("const string& get_regex() const", &compute::regex_source::get_regex);
				vregex_source->set_method("int64 get_max_branches() const", &compute::regex_source::get_max_branches);
				vregex_source->set_method("int64 get_max_brackets() const", &compute::regex_source::get_max_brackets);
				vregex_source->set_method("int64 get_max_matches() const", &compute::regex_source::get_max_matches);
				vregex_source->set_method("int64 get_complexity() const", &compute::regex_source::get_complexity);
				vregex_source->set_method("regex_state get_state() const", &compute::regex_source::get_state);
				vregex_source->set_method("bool is_simple() const", &compute::regex_source::is_simple);
				vregex_source->set_method_extern("regex_result match(const string_view&in) const", &regex_source_match);
				vregex_source->set_method_extern("string replace(const string_view&in, const string_view&in) const", &regex_source_replace);

				return true;
#else
				VI_ASSERT(false, "<regex> is not loaded");
				return false;
#endif
			}
			bool registry::import_crypto(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				VI_TYPEREF(web_token, "web_token");
				vm->set_class<core::stream>("base_stream", false);

				auto vsecret_box = vm->set_struct_trivial<compute::secret_box>("secret_box");
				vsecret_box->set_constructor<compute::secret_box>("void f()");
				vsecret_box->set_method("void clear()", &compute::secret_box::clear);
				vsecret_box->set_method("string heap() const", &compute::secret_box::heap);
				vsecret_box->set_method("usize size() const", &compute::secret_box::clear);
				vsecret_box->set_method_static<compute::secret_box, const std::string_view&>("secret_box secure(const string_view&in)", &compute::secret_box::secure);
				vsecret_box->set_method_static<compute::secret_box, const std::string_view&>("secret_box insecure(const string_view&in)", &compute::secret_box::insecure);
				vsecret_box->set_method_static<compute::secret_box, const std::string_view&>("secret_box view(const string_view&in)", &compute::secret_box::view);

				auto vweb_token = vm->set_class<compute::web_token>("web_token", true);
				vweb_token->set_property<compute::web_token>("schema@ header", &compute::web_token::header);
				vweb_token->set_property<compute::web_token>("schema@ payload", &compute::web_token::payload);
				vweb_token->set_property<compute::web_token>("schema@ token", &compute::web_token::token);
				vweb_token->set_property<compute::web_token>("string refresher", &compute::web_token::refresher);
				vweb_token->set_property<compute::web_token>("string signature", &compute::web_token::signature);
				vweb_token->set_property<compute::web_token>("string data", &compute::web_token::data);
				vweb_token->set_gc_constructor<compute::web_token, web_token>("web_token@ f()");
				vweb_token->set_gc_constructor<compute::web_token, web_token, const std::string_view&, const std::string_view&, int64_t>("web_token@ f(const string_view&in, const string_view&in, int64)");
				vweb_token->set_method("void unsign()", &compute::web_token::unsign);
				vweb_token->set_method("void set_algorithm(const string_view&in)", &compute::web_token::set_algorithm);
				vweb_token->set_method("void set_type(const string_view&in)", &compute::web_token::set_type);
				vweb_token->set_method("void set_content_type(const string_view&in)", &compute::web_token::set_content_type);
				vweb_token->set_method("void set_issuer(const string_view&in)", &compute::web_token::set_issuer);
				vweb_token->set_method("void set_subject(const string_view&in)", &compute::web_token::set_subject);
				vweb_token->set_method("void set_id(const string_view&in)", &compute::web_token::set_id);
				vweb_token->set_method("void set_expiration(int64)", &compute::web_token::set_expiration);
				vweb_token->set_method("void set_not_before(int64)", &compute::web_token::set_not_before);
				vweb_token->set_method("void set_created(int64)", &compute::web_token::set_created);
				vweb_token->set_method("void set_refresh_token(const string_view&in, const secret_box &in, const secret_box &in)", &compute::web_token::set_refresh_token);
				vweb_token->set_method("bool sign(const secret_box &in)", &compute::web_token::sign);
				vweb_token->set_method("bool is_valid()", &compute::web_token::is_valid);
				vweb_token->set_method_extern("string get_refresh_token(const secret_box &in, const secret_box &in)", &VI_EXPECTIFY(compute::web_token::get_refresh_token));
				vweb_token->set_method_extern("void set_audience(array<string>@+)", &web_token_set_audience);
				vweb_token->set_enum_refs_extern<compute::web_token>([](compute::web_token* base, asIScriptEngine* vm) { });
				vweb_token->set_release_refs_extern<compute::web_token>([](compute::web_token* base, asIScriptEngine*) { });

				vm->begin_namespace("ciphers");
				vm->set_function("uptr@ des_ecb()", &compute::ciphers::des_ecb);
				vm->set_function("uptr@ des_ede()", &compute::ciphers::des_ede);
				vm->set_function("uptr@ des_ede3()", &compute::ciphers::des_ede3);
				vm->set_function("uptr@ des_ede_ecb()", &compute::ciphers::des_ede_ecb);
				vm->set_function("uptr@ des_ede3_ecb()", &compute::ciphers::des_ede3_ecb);
				vm->set_function("uptr@ des_cfb64()", &compute::ciphers::des_cfb64);
				vm->set_function("uptr@ des_cfb1()", &compute::ciphers::des_cfb1);
				vm->set_function("uptr@ des_cfb8()", &compute::ciphers::des_cfb8);
				vm->set_function("uptr@ des_ede_cfb64()", &compute::ciphers::des_ede_cfb64);
				vm->set_function("uptr@ des_ede3_cfb64()", &compute::ciphers::des_ede3_cfb64);
				vm->set_function("uptr@ des_ede3_cfb1()", &compute::ciphers::des_ede3_cfb1);
				vm->set_function("uptr@ des_ede3_cfb8()", &compute::ciphers::des_ede3_cfb8);
				vm->set_function("uptr@ des_ofb()", &compute::ciphers::des_ofb);
				vm->set_function("uptr@ des_ede_ofb()", &compute::ciphers::des_ede_ofb);
				vm->set_function("uptr@ des_ede3_ofb()", &compute::ciphers::des_ede3_ofb);
				vm->set_function("uptr@ des_cbc()", &compute::ciphers::des_cbc);
				vm->set_function("uptr@ des_ede_cbc()", &compute::ciphers::des_ede_cbc);
				vm->set_function("uptr@ des_ede3_cbc()", &compute::ciphers::des_ede3_cbc);
				vm->set_function("uptr@ des_ede3_wrap()", &compute::ciphers::desede3_wrap);
				vm->set_function("uptr@ desx_cbc()", &compute::ciphers::desx_cbc);
				vm->set_function("uptr@ rc4()", &compute::ciphers::rc4);
				vm->set_function("uptr@ rc4_40()", &compute::ciphers::rc4_40);
				vm->set_function("uptr@ rc4_hmac_md5()", &compute::ciphers::rc4_hmac_md5);
				vm->set_function("uptr@ idea_ecb()", &compute::ciphers::idea_ecb);
				vm->set_function("uptr@ idea_cfb64()", &compute::ciphers::idea_cfb64);
				vm->set_function("uptr@ idea_ofb()", &compute::ciphers::idea_ofb);
				vm->set_function("uptr@ idea_cbc()", &compute::ciphers::idea_cbc);
				vm->set_function("uptr@ rc2_ecb()", &compute::ciphers::rc2_ecb);
				vm->set_function("uptr@ rc2_cbc()", &compute::ciphers::rc2_cbc);
				vm->set_function("uptr@ rc2_40_cbc()", &compute::ciphers::rc2_40_cbc);
				vm->set_function("uptr@ rc2_64_cbc()", &compute::ciphers::rc2_64_cbc);
				vm->set_function("uptr@ rc2_cfb64()", &compute::ciphers::rc2_cfb64);
				vm->set_function("uptr@ rc2_ofb()", &compute::ciphers::rc2_ofb);
				vm->set_function("uptr@ bf_ecb()", &compute::ciphers::bf_ecb);
				vm->set_function("uptr@ bf_cbc()", &compute::ciphers::bf_cbc);
				vm->set_function("uptr@ bf_cfb64()", &compute::ciphers::bf_cfb64);
				vm->set_function("uptr@ bf_ofb()", &compute::ciphers::bf_ofb);
				vm->set_function("uptr@ cast5_ecb()", &compute::ciphers::cast5_ecb);
				vm->set_function("uptr@ cast5_cbc()", &compute::ciphers::cast5_cbc);
				vm->set_function("uptr@ cast5_cfb64()", &compute::ciphers::cast5_cfb64);
				vm->set_function("uptr@ cast5_ofb()", &compute::ciphers::cast5_ofb);
				vm->set_function("uptr@ rc5_32_12_16_cbc()", &compute::ciphers::rc5_32_12_16_cbc);
				vm->set_function("uptr@ rc5_32_12_16_ecb()", &compute::ciphers::rc5_32_12_16_ecb);
				vm->set_function("uptr@ rc5_32_12_16_cfb64()", &compute::ciphers::rc5_32_12_16_cfb64);
				vm->set_function("uptr@ rc5_32_12_16_ofb()", &compute::ciphers::rc5_32_12_16_ofb);
				vm->set_function("uptr@ aes_128_ecb()", &compute::ciphers::aes_128_ecb);
				vm->set_function("uptr@ aes_128_cbc()", &compute::ciphers::aes_128_cbc);
				vm->set_function("uptr@ aes_128_cfb1()", &compute::ciphers::aes_128_cfb1);
				vm->set_function("uptr@ aes_128_cfb8()", &compute::ciphers::aes_128_cfb8);
				vm->set_function("uptr@ aes_128_cfb128()", &compute::ciphers::aes_128_cfb128);
				vm->set_function("uptr@ aes_128_ofb()", &compute::ciphers::aes_128_ofb);
				vm->set_function("uptr@ aes_128_ctr()", &compute::ciphers::aes_128_ctr);
				vm->set_function("uptr@ aes_128_ccm()", &compute::ciphers::aes_128_ccm);
				vm->set_function("uptr@ aes_128_gcm()", &compute::ciphers::aes_128_gcm);
				vm->set_function("uptr@ aes_128_xts()", &compute::ciphers::aes_128_xts);
				vm->set_function("uptr@ aes_128_wrap()", &compute::ciphers::aes128_wrap);
				vm->set_function("uptr@ aes_128_wrap_pad()", &compute::ciphers::aes128_wrap_pad);
				vm->set_function("uptr@ aes_128_ocb()", &compute::ciphers::aes_128_ocb);
				vm->set_function("uptr@ aes_192_ecb()", &compute::ciphers::aes_192_ecb);
				vm->set_function("uptr@ aes_192_cbc()", &compute::ciphers::aes_192_cbc);
				vm->set_function("uptr@ aes_192_cfb1()", &compute::ciphers::aes_192_cfb1);
				vm->set_function("uptr@ aes_192_cfb8()", &compute::ciphers::aes_192_cfb8);
				vm->set_function("uptr@ aes_192_cfb128()", &compute::ciphers::aes_192_cfb128);
				vm->set_function("uptr@ aes_192_ofb()", &compute::ciphers::aes_192_ofb);
				vm->set_function("uptr@ aes_192_ctr()", &compute::ciphers::aes_192_ctr);
				vm->set_function("uptr@ aes_192_ccm()", &compute::ciphers::aes_192_ccm);
				vm->set_function("uptr@ aes_192_gcm()", &compute::ciphers::aes_192_gcm);
				vm->set_function("uptr@ aes_192_wrap()", &compute::ciphers::aes192_wrap);
				vm->set_function("uptr@ aes_192_wrap_pad()", &compute::ciphers::aes192_wrap_pad);
				vm->set_function("uptr@ aes_192_ocb()", &compute::ciphers::aes_192_ocb);
				vm->set_function("uptr@ aes_256_ecb()", &compute::ciphers::aes_256_ecb);
				vm->set_function("uptr@ aes_256_cbc()", &compute::ciphers::aes_256_cbc);
				vm->set_function("uptr@ aes_256_cfb1()", &compute::ciphers::aes_256_cfb1);
				vm->set_function("uptr@ aes_256_cfb8()", &compute::ciphers::aes_256_cfb8);
				vm->set_function("uptr@ aes_256_cfb128()", &compute::ciphers::aes_256_cfb128);
				vm->set_function("uptr@ aes_256_ofb()", &compute::ciphers::aes_256_ofb);
				vm->set_function("uptr@ aes_256_ctr()", &compute::ciphers::aes_256_ctr);
				vm->set_function("uptr@ aes_256_ccm()", &compute::ciphers::aes_256_ccm);
				vm->set_function("uptr@ aes_256_gcm()", &compute::ciphers::aes_256_gcm);
				vm->set_function("uptr@ aes_256_xts()", &compute::ciphers::aes_256_xts);
				vm->set_function("uptr@ aes_256_wrap()", &compute::ciphers::aes256_wrap);
				vm->set_function("uptr@ aes_256_wrap_pad()", &compute::ciphers::aes256_wrap_pad);
				vm->set_function("uptr@ aes_256_ocb()", &compute::ciphers::aes_256_ocb);
				vm->set_function("uptr@ aes_128_cbc_hmac_SHA1()", &compute::ciphers::aes_128_cbc_hmac_sha1);
				vm->set_function("uptr@ aes_256_cbc_hmac_SHA1()", &compute::ciphers::aes_256_cbc_hmac_sha1);
				vm->set_function("uptr@ aes_128_cbc_hmac_SHA256()", &compute::ciphers::aes_128_cbc_hmac_sha256);
				vm->set_function("uptr@ aes_256_cbc_hmac_SHA256()", &compute::ciphers::aes_256_cbc_hmac_sha256);
				vm->set_function("uptr@ aria_128_ecb()", &compute::ciphers::aria_128_ecb);
				vm->set_function("uptr@ aria_128_cbc()", &compute::ciphers::aria_128_cbc);
				vm->set_function("uptr@ aria_128_cfb1()", &compute::ciphers::aria_128_cfb1);
				vm->set_function("uptr@ aria_128_cfb8()", &compute::ciphers::aria_128_cfb8);
				vm->set_function("uptr@ aria_128_cfb128()", &compute::ciphers::aria_128_cfb128);
				vm->set_function("uptr@ aria_128_ctr()", &compute::ciphers::aria_128_ctr);
				vm->set_function("uptr@ aria_128_ofb()", &compute::ciphers::aria_128_ofb);
				vm->set_function("uptr@ aria_128_gcm()", &compute::ciphers::aria_128_gcm);
				vm->set_function("uptr@ aria_128_ccm()", &compute::ciphers::aria_128_ccm);
				vm->set_function("uptr@ aria_192_ecb()", &compute::ciphers::aria_192_ecb);
				vm->set_function("uptr@ aria_192_cbc()", &compute::ciphers::aria_192_cbc);
				vm->set_function("uptr@ aria_192_cfb1()", &compute::ciphers::aria_192_cfb1);
				vm->set_function("uptr@ aria_192_cfb8()", &compute::ciphers::aria_192_cfb8);
				vm->set_function("uptr@ aria_192_cfb128()", &compute::ciphers::aria_192_cfb128);
				vm->set_function("uptr@ aria_192_ctr()", &compute::ciphers::aria_192_ctr);
				vm->set_function("uptr@ aria_192_ofb()", &compute::ciphers::aria_192_ofb);
				vm->set_function("uptr@ aria_192_gcm()", &compute::ciphers::aria_192_gcm);
				vm->set_function("uptr@ aria_192_ccm()", &compute::ciphers::aria_192_ccm);
				vm->set_function("uptr@ aria_256_ecb()", &compute::ciphers::aria_256_ecb);
				vm->set_function("uptr@ aria_256_cbc()", &compute::ciphers::aria_256_cbc);
				vm->set_function("uptr@ aria_256_cfb1()", &compute::ciphers::aria_256_cfb1);
				vm->set_function("uptr@ aria_256_cfb8()", &compute::ciphers::aria_256_cfb8);
				vm->set_function("uptr@ aria_256_cfb128()", &compute::ciphers::aria_256_cfb128);
				vm->set_function("uptr@ aria_256_ctr()", &compute::ciphers::aria_256_ctr);
				vm->set_function("uptr@ aria_256_ofb()", &compute::ciphers::aria_256_ofb);
				vm->set_function("uptr@ aria_256_gcm()", &compute::ciphers::aria_256_gcm);
				vm->set_function("uptr@ aria_256_ccm()", &compute::ciphers::aria_256_ccm);
				vm->set_function("uptr@ camellia_128_ecb()", &compute::ciphers::camellia128_ecb);
				vm->set_function("uptr@ camellia_128_cbc()", &compute::ciphers::camellia128_cbc);
				vm->set_function("uptr@ camellia_128_cfb1()", &compute::ciphers::camellia128_cfb1);
				vm->set_function("uptr@ camellia_128_cfb8()", &compute::ciphers::camellia128_cfb8);
				vm->set_function("uptr@ camellia_128_cfb128()", &compute::ciphers::camellia128_cfb128);
				vm->set_function("uptr@ camellia_128_ofb()", &compute::ciphers::camellia128_ofb);
				vm->set_function("uptr@ camellia_128_ctr()", &compute::ciphers::camellia128_ctr);
				vm->set_function("uptr@ camellia_192_ecb()", &compute::ciphers::camellia192_ecb);
				vm->set_function("uptr@ camellia_192_cbc()", &compute::ciphers::camellia192_cbc);
				vm->set_function("uptr@ camellia_192_cfb1()", &compute::ciphers::camellia192_cfb1);
				vm->set_function("uptr@ camellia_192_cfb8()", &compute::ciphers::camellia192_cfb8);
				vm->set_function("uptr@ camellia_192_cfb128()", &compute::ciphers::camellia192_cfb128);
				vm->set_function("uptr@ camellia_192_ofb()", &compute::ciphers::camellia192_ofb);
				vm->set_function("uptr@ camellia_192_ctr()", &compute::ciphers::camellia192_ctr);
				vm->set_function("uptr@ camellia_256_ecb()", &compute::ciphers::camellia256_ecb);
				vm->set_function("uptr@ camellia_256_cbc()", &compute::ciphers::camellia256_cbc);
				vm->set_function("uptr@ camellia_256_cfb1()", &compute::ciphers::camellia256_cfb1);
				vm->set_function("uptr@ camellia_256_cfb8()", &compute::ciphers::camellia256_cfb8);
				vm->set_function("uptr@ camellia_256_cfb128()", &compute::ciphers::camellia256_cfb128);
				vm->set_function("uptr@ camellia_256_ofb()", &compute::ciphers::camellia256_ofb);
				vm->set_function("uptr@ camellia_256_ctr()", &compute::ciphers::camellia256_ctr);
				vm->set_function("uptr@ chacha20()", &compute::ciphers::chacha20);
				vm->set_function("uptr@ chacha20_poly1305()", &compute::ciphers::chacha20_poly1305);
				vm->set_function("uptr@ seed_ecb()", &compute::ciphers::seed_ecb);
				vm->set_function("uptr@ seed_cbc()", &compute::ciphers::seed_cbc);
				vm->set_function("uptr@ seed_cfb128()", &compute::ciphers::seed_cfb128);
				vm->set_function("uptr@ seed_ofb()", &compute::ciphers::seed_ofb);
				vm->set_function("uptr@ sm4_ecb()", &compute::ciphers::sm4_ecb);
				vm->set_function("uptr@ sm4_cbc()", &compute::ciphers::sm4_cbc);
				vm->set_function("uptr@ sm4_cfb128()", &compute::ciphers::sm4_cfb128);
				vm->set_function("uptr@ sm4_ofb()", &compute::ciphers::sm4_ofb);
				vm->set_function("uptr@ sm4_ctr()", &compute::ciphers::sm4_ctr);
				vm->end_namespace();

				vm->begin_namespace("digests");
				vm->set_function("uptr@ md2()", &compute::digests::md2);
				vm->set_function("uptr@ md4()", &compute::digests::md4);
				vm->set_function("uptr@ md5()", &compute::digests::md5);
				vm->set_function("uptr@ md5_sha1()", &compute::digests::md5_sha1);
				vm->set_function("uptr@ blake2b512()", &compute::digests::blake2_b512);
				vm->set_function("uptr@ blake2s256()", &compute::digests::blake2_s256);
				vm->set_function("uptr@ sha1()", &compute::digests::sha1);
				vm->set_function("uptr@ sha224()", &compute::digests::sha224);
				vm->set_function("uptr@ sha256()", &compute::digests::sha256);
				vm->set_function("uptr@ sha384()", &compute::digests::sha384);
				vm->set_function("uptr@ sha512()", &compute::digests::sha512);
				vm->set_function("uptr@ sha512_224()", &compute::digests::sha512_224);
				vm->set_function("uptr@ sha512_256()", &compute::digests::sha512_256);
				vm->set_function("uptr@ sha3_224()", &compute::digests::sha3_224);
				vm->set_function("uptr@ sha3_256()", &compute::digests::sha3_256);
				vm->set_function("uptr@ sha3_384()", &compute::digests::sha3_384);
				vm->set_function("uptr@ sha3_512()", &compute::digests::sha3_512);
				vm->set_function("uptr@ shake128()", &compute::digests::shake128);
				vm->set_function("uptr@ shake256()", &compute::digests::shake256);
				vm->set_function("uptr@ mdc2()", &compute::digests::mdc2);
				vm->set_function("uptr@ ripemd160()", &compute::digests::ripe_md160);
				vm->set_function("uptr@ whirlpool()", &compute::digests::whirlpool);
				vm->set_function("uptr@ sm3()", &compute::digests::sm3);
				vm->end_namespace();

				vm->begin_namespace("proofs");
				auto vformat = vm->set_enum("format");
				vformat->set_value("uncompressed", (int)compute::proofs::format::uncompressed);
				vformat->set_value("compressed", (int)compute::proofs::format::compressed);
				vformat->set_value("hybrid", (int)compute::proofs::format::hybrid);
				vm->set_function("int32 pk_rsa()", &compute::proofs::pk_rsa);
				vm->set_function("int32 pk_dsa()", &compute::proofs::pk_dsa);
				vm->set_function("int32 pk_dh()", &compute::proofs::pk_dh);
				vm->set_function("int32 pk_ec()", &compute::proofs::pk_ec);
				vm->set_function("int32 pkt_sign()", &compute::proofs::pkt_sign);
				vm->set_function("int32 pkt_enc()", &compute::proofs::pkt_enc);
				vm->set_function("int32 pkt_exch()", &compute::proofs::pkt_exch);
				vm->set_function("int32 pks_rsa()", &compute::proofs::pks_rsa);
				vm->set_function("int32 pks_dsa()", &compute::proofs::pks_dsa);
				vm->set_function("int32 pks_ec()", &compute::proofs::pks_ec);
				vm->set_function("int32 rsa()", &compute::proofs::rsa);
				vm->set_function("int32 rsa2()", &compute::proofs::rsa2);
				vm->set_function("int32 rsa_pss()", &compute::proofs::rsa_pss);
				vm->set_function("int32 dsa()", &compute::proofs::dsa);
				vm->set_function("int32 dsa1()", &compute::proofs::dsa1);
				vm->set_function("int32 dsa2()", &compute::proofs::dsa2);
				vm->set_function("int32 dsa3()", &compute::proofs::dsa3);
				vm->set_function("int32 dsa4()", &compute::proofs::dsa4);
				vm->set_function("int32 dh()", &compute::proofs::dh);
				vm->set_function("int32 dhx()", &compute::proofs::dhx);
				vm->set_function("int32 ec()", &compute::proofs::ec);
				vm->set_function("int32 sm2()", &compute::proofs::sm2);
				vm->set_function("int32 hmac()", &compute::proofs::hmac);
				vm->set_function("int32 cmac()", &compute::proofs::cmac);
				vm->set_function("int32 scrypt()", &compute::proofs::scrypt);
				vm->set_function("int32 tls1_prf()", &compute::proofs::tls1_prf);
				vm->set_function("int32 hkdf()", &compute::proofs::hkdf);
				vm->set_function("int32 poly1305()", &compute::proofs::poly1305);
				vm->set_function("int32 siphash()", &compute::proofs::siphash);
				vm->set_function("int32 x25519()", &compute::proofs::x25519);
				vm->set_function("int32 ed25519()", &compute::proofs::ed25519);
				vm->set_function("int32 x448()", &compute::proofs::x448);
				vm->set_function("int32 ed448()", &compute::proofs::ed448);
				vm->end_namespace();

				vm->begin_namespace("proofs::curves");
				vm->set_function("int32 c2pnb163v1()", &compute::proofs::curves::c2pnb163v1);
				vm->set_function("int32 c2pnb163v2()", &compute::proofs::curves::c2pnb163v2);
				vm->set_function("int32 c2pnb163v3()", &compute::proofs::curves::c2pnb163v3);
				vm->set_function("int32 c2pnb176v1()", &compute::proofs::curves::c2pnb176v1);
				vm->set_function("int32 c2tnb191v1()", &compute::proofs::curves::c2tnb191v1);
				vm->set_function("int32 c2tnb191v2()", &compute::proofs::curves::c2tnb191v2);
				vm->set_function("int32 c2tnb191v3()", &compute::proofs::curves::c2tnb191v3);
				vm->set_function("int32 c2onb191v4()", &compute::proofs::curves::c2onb191v4);
				vm->set_function("int32 c2onb191v5()", &compute::proofs::curves::c2onb191v5);
				vm->set_function("int32 c2pnb208w1()", &compute::proofs::curves::c2pnb208w1);
				vm->set_function("int32 c2tnb239v1()", &compute::proofs::curves::c2tnb239v1);
				vm->set_function("int32 c2tnb239v2()", &compute::proofs::curves::c2tnb239v2);
				vm->set_function("int32 c2tnb239v3()", &compute::proofs::curves::c2tnb239v3);
				vm->set_function("int32 c2onb239v4()", &compute::proofs::curves::c2onb239v4);
				vm->set_function("int32 c2onb239v5()", &compute::proofs::curves::c2onb239v5);
				vm->set_function("int32 c2pnb272w1()", &compute::proofs::curves::c2pnb272w1);
				vm->set_function("int32 c2pnb304w1()", &compute::proofs::curves::c2pnb304w1);
				vm->set_function("int32 c2tnb359v1()", &compute::proofs::curves::c2tnb359v1);
				vm->set_function("int32 c2pnb368w1()", &compute::proofs::curves::c2pnb368w1);
				vm->set_function("int32 c2tnb431r1()", &compute::proofs::curves::c2tnb431r1);
				vm->set_function("int32 prime192v1()", &compute::proofs::curves::prime192v1);
				vm->set_function("int32 prime192v2()", &compute::proofs::curves::prime192v2);
				vm->set_function("int32 prime192v3()", &compute::proofs::curves::prime192v3);
				vm->set_function("int32 prime239v1()", &compute::proofs::curves::prime239v1);
				vm->set_function("int32 prime239v2()", &compute::proofs::curves::prime239v2);
				vm->set_function("int32 prime239v3()", &compute::proofs::curves::prime239v3);
				vm->set_function("int32 prime256v1()", &compute::proofs::curves::prime256v1);
				vm->set_function("int32 ecdsa_sha1()", &compute::proofs::curves::ecdsa_sha1);
				vm->set_function("int32 ecdsa_sha224()", &compute::proofs::curves::ecdsa_sha224);
				vm->set_function("int32 ecdsa_sha256()", &compute::proofs::curves::ecdsa_sha256);
				vm->set_function("int32 ecdsa_sha384()", &compute::proofs::curves::ecdsa_sha384);
				vm->set_function("int32 ecdsa_sha512()", &compute::proofs::curves::ecdsa_sha512);
				vm->set_function("int32 secp112r1()", &compute::proofs::curves::secp112r1);
				vm->set_function("int32 secp112r2()", &compute::proofs::curves::secp112r2);
				vm->set_function("int32 secp128r1()", &compute::proofs::curves::secp128r1);
				vm->set_function("int32 secp128r2()", &compute::proofs::curves::secp128r2);
				vm->set_function("int32 secp160k1()", &compute::proofs::curves::secp160k1);
				vm->set_function("int32 secp160r1()", &compute::proofs::curves::secp160r1);
				vm->set_function("int32 secp160r2()", &compute::proofs::curves::secp160r2);
				vm->set_function("int32 secp192k1()", &compute::proofs::curves::secp192k1);
				vm->set_function("int32 secp224k1()", &compute::proofs::curves::secp224k1);
				vm->set_function("int32 secp224r1()", &compute::proofs::curves::secp224r1);
				vm->set_function("int32 secp256k1()", &compute::proofs::curves::secp256k1);
				vm->set_function("int32 secp384r1()", &compute::proofs::curves::secp384r1);
				vm->set_function("int32 secp521r1()", &compute::proofs::curves::secp521r1);
				vm->set_function("int32 sect113r1()", &compute::proofs::curves::sect113r1);
				vm->set_function("int32 sect113r2()", &compute::proofs::curves::sect113r2);
				vm->set_function("int32 sect131r1()", &compute::proofs::curves::sect131r1);
				vm->set_function("int32 sect131r2()", &compute::proofs::curves::sect131r2);
				vm->set_function("int32 sect163k1()", &compute::proofs::curves::sect163k1);
				vm->set_function("int32 sect163r1()", &compute::proofs::curves::sect163r1);
				vm->set_function("int32 sect163r2()", &compute::proofs::curves::sect163r2);
				vm->set_function("int32 sect193r1()", &compute::proofs::curves::sect193r1);
				vm->set_function("int32 sect193r2()", &compute::proofs::curves::sect193r2);
				vm->set_function("int32 sect233k1()", &compute::proofs::curves::sect233k1);
				vm->set_function("int32 sect233r1()", &compute::proofs::curves::sect233r1);
				vm->set_function("int32 sect239k1()", &compute::proofs::curves::sect239k1);
				vm->set_function("int32 sect283k1()", &compute::proofs::curves::sect283k1);
				vm->set_function("int32 sect283r1()", &compute::proofs::curves::sect283r1);
				vm->set_function("int32 sect409k1()", &compute::proofs::curves::sect409k1);
				vm->set_function("int32 sect409r1()", &compute::proofs::curves::sect409r1);
				vm->set_function("int32 sect571k1()", &compute::proofs::curves::sect571k1);
				vm->set_function("int32 sect571r1()", &compute::proofs::curves::sect571r1);
				vm->end_namespace();

				vm->begin_namespace("crypto");
				vm->set_function_def("string block_transform_sync(const string_view&in)");
				vm->set_function("uptr@ get_digest_by_name(const string_view&in)", &compute::crypto::get_digest_by_name);
				vm->set_function("uptr@ get_cipher_by_name(const string_view&in)", &compute::crypto::get_cipher_by_name);
				vm->set_function("int32 get_proof_by_name(const string_view&in)", &compute::crypto::get_proof_by_name);
				vm->set_function("int32 get_curve_by_name(const string_view&in)", &compute::crypto::get_curve_by_name);
				vm->set_function("string_view get_digest_name(uptr@)", &compute::crypto::get_digest_name);
				vm->set_function("string_view get_cipher_name(uptr@)", &compute::crypto::get_cipher_name);
				vm->set_function("string_view get_proof_name(int32)", &compute::crypto::get_proof_name);
				vm->set_function("string_view get_curve_name(int32)", &compute::crypto::get_curve_name);
				vm->set_function("uint8 random_uc()", &compute::crypto::random_uc);
				vm->set_function<uint64_t()>("uint64 random()", &compute::crypto::random);
				vm->set_function<uint64_t(uint64_t, uint64_t)>("uint64 random(uint64, uint64)", &compute::crypto::random);
				vm->set_function("uint32 crc32(const string_view&in)", &compute::crypto::crc32);
				vm->set_function("uint64 crc64(const string_view&in)", &compute::crypto::crc64);
				vm->set_function("void display_crypto_log()", &compute::crypto::display_crypto_log);
				vm->set_function("string random_bytes(usize)", &VI_SEXPECTIFY(compute::crypto::random_bytes));
				vm->set_function("string checksum_hex(base_stream@, const string_view&in)", &VI_SEXPECTIFY(compute::crypto::checksum_hex));
				vm->set_function("string checksum(base_stream@, const string_view&in)", &VI_SEXPECTIFY(compute::crypto::checksum));
				vm->set_function("string hash_hex(uptr@, const string_view&in)", &VI_SEXPECTIFY(compute::crypto::hash_hex));
				vm->set_function("string hash(uptr@, const string_view&in)", &VI_SEXPECTIFY(compute::crypto::hash));
				vm->set_function("string jwt_sign(const string_view&in, const string_view&in, const secret_box &in)", &VI_SEXPECTIFY(compute::crypto::jwt_sign));
				vm->set_function("string jwt_encode(web_token@+, const secret_box &in)", &VI_SEXPECTIFY(compute::crypto::jwt_encode));
				vm->set_function("web_token@ jwt_decode(const string_view&in, const secret_box &in)", &VI_SEXPECTIFY(compute::crypto::jwt_decode));
				vm->set_function("string schema_encrypt(schema@+, const secret_box &in, const secret_box &in)", &VI_SEXPECTIFY(compute::crypto::schema_encrypt));
				vm->set_function("schema@ schema_decrypt(const string_view&in, const secret_box &in, const secret_box &in)", &VI_SEXPECTIFY(compute::crypto::schema_decrypt));
				vm->set_function("secret_box private_key_gen(int32, usize = 2048)", &VI_SEXPECTIFY(compute::crypto::private_key_gen));
				vm->set_function("string to_public_key(int32, const secret_box&in)", &VI_SEXPECTIFY(compute::crypto::to_public_key));
				vm->set_function("string sign(uptr@, int32, const string_view&in, const secret_box &in)", &VI_SEXPECTIFY(compute::crypto::sign));
				vm->set_function("bool verify(uptr@, int32, const string_view&in, const string_view &in, const string_view &in)", &crypto_verify);
				vm->set_function("secret_box ec_private_key_gen(int32)", &VI_SEXPECTIFY(compute::crypto::ec_private_key_gen));
				vm->set_function("string ec_to_public_key(int32, proofs::format, const secret_box&in)", &VI_SEXPECTIFY(compute::crypto::ec_to_public_key));
				vm->set_function("secret_box ec_scalar_add(int32, const secret_box&in, const secret_box&in)", &VI_SEXPECTIFY(compute::crypto::ec_scalar_add));
				vm->set_function("secret_box ec_scalar_sub(int32, const secret_box&in, const secret_box&in)", &VI_SEXPECTIFY(compute::crypto::ec_scalar_sub));
				vm->set_function("secret_box ec_scalar_mul(int32, const secret_box&in, const secret_box&in)", &VI_SEXPECTIFY(compute::crypto::ec_scalar_mul));
				vm->set_function("secret_box ec_scalar_inv(int32, const secret_box&in)", &VI_SEXPECTIFY(compute::crypto::ec_scalar_inv));
				vm->set_function("bool ec_scalar_is_on_curve(int32, const secret_box&in)", &crypto_ec_scalar_is_on_curve);
				vm->set_function("string ec_point_mul(int32, proofs::format, const string_view&in, const secret_box&in)", &VI_SEXPECTIFY(compute::crypto::ec_point_mul));
				vm->set_function("string ec_point_add(int32, proofs::format, const string_view&in, const string_view&in)", &VI_SEXPECTIFY(compute::crypto::ec_point_add));
				vm->set_function("string ec_point_inv(int32, proofs::format, const string_view&in)", &VI_SEXPECTIFY(compute::crypto::ec_point_inv));
				vm->set_function("bool ec_point_is_on_curve(int32, const string_view&in)", &crypto_ec_point_is_on_curve);
				vm->set_function("string ec_sign(uptr@, int32, const string_view&in, const secret_box &in)", &VI_SEXPECTIFY(compute::crypto::ec_sign));
				vm->set_function("bool ec_verify(uptr@, int32, const string_view&in, const string_view&in, const string_view &in)", &crypto_ec_verify);
				vm->set_function("string ec_der_to_rs(const string_view&in)", &VI_SEXPECTIFY(compute::crypto::ec_der_to_rs));
				vm->set_function("string ec_rs_to_der(const string_view&in)", &VI_SEXPECTIFY(compute::crypto::ec_rs_to_der));
				vm->set_function("string hmac(uptr@, const string_view&in, const secret_box &in)", &VI_SEXPECTIFY(compute::crypto::hmac));
				vm->set_function("string encrypt(uptr@, const string_view&in, const secret_box &in, const secret_box &in, int = -1)", &VI_SEXPECTIFY(compute::crypto::encrypt));
				vm->set_function("string decrypt(uptr@, const string_view&in, const secret_box &in, const secret_box &in, int = -1)", &VI_SEXPECTIFY(compute::crypto::decrypt));
				vm->set_function("usize file_encrypt(uptr@, base_stream@, const secret_box &in, const secret_box &in, block_transform_sync@ = null, usize = 1, int = -1)", &crypto_file_encrypt);
				vm->set_function("usize file_decrypt(uptr@, base_stream@, const secret_box &in, const secret_box &in, block_transform_sync@ = null, usize = 1, int = -1)", &crypto_file_decrypt);
				vm->end_namespace();

				return true;
#else
				VI_ASSERT(false, "<crypto> is not loaded");
				return false;
#endif
			}
			bool registry::import_codec(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				auto vcompression = vm->set_enum("compression_cdc");
				vcompression->set_value("none", (int)compute::compression::none);
				vcompression->set_value("best_speed", (int)compute::compression::best_speed);
				vcompression->set_value("best_compression", (int)compute::compression::best_compression);
				vcompression->set_value("default_compression", (int)compute::compression::default_compression);

				vm->begin_namespace("codec");
				vm->set_function("string rotate(const string_view&in, uint64, int8)", &compute::codec::rotate);
				vm->set_function("string bep45_encode(const string_view&in)", &compute::codec::bep45_encode);
				vm->set_function("string bep45_decode(const string_view&in)", &compute::codec::bep45_decode);
				vm->set_function("string base32_encode(const string_view&in)", &compute::codec::base32_encode);
				vm->set_function("string base32_decode(const string_view&in)", &compute::codec::base32_decode);
				vm->set_function("string base45_encode(const string_view&in)", &compute::codec::base45_encode);
				vm->set_function("string base45_decode(const string_view&in)", &compute::codec::base45_decode);
				vm->set_function("string compress(const string_view&in, compression_cdc)", &VI_SEXPECTIFY(compute::codec::compress));
				vm->set_function("string decompress(const string_view&in)", &VI_SEXPECTIFY(compute::codec::decompress));
				vm->set_function<core::string(const std::string_view&)>("string base64_encode(const string_view&in)", &compute::codec::base64_encode);
				vm->set_function<core::string(const std::string_view&)>("string base64_decode(const string_view&in)", &compute::codec::base64_decode);
				vm->set_function<core::string(const std::string_view&)>("string base64_url_encode(const string_view&in)", &compute::codec::base64_url_encode);
				vm->set_function<core::string(const std::string_view&)>("string base64_url_decode(const string_view&in)", &compute::codec::base64_url_decode);
				vm->set_function<core::string(const std::string_view&, bool)>("string hex_encode(const string_view&in, bool = false)", &compute::codec::hex_encode);
				vm->set_function<core::string(const std::string_view&)>("string hex_decode(const string_view&in)", &compute::codec::hex_decode);
				vm->set_function<core::string(const std::string_view&)>("string url_encode(const string_view&in)", &compute::codec::url_encode);
				vm->set_function<core::string(const std::string_view&)>("string url_decode(const string_view&in)", &compute::codec::url_decode);
				vm->set_function("string decimal_to_hex(uint64)", &compute::codec::decimal_to_hex);
				vm->set_function("string base10_to_base_n(uint64, uint8)", &compute::codec::base10_to_base_n);
				vm->end_namespace();

				return true;
#else
				VI_ASSERT(false, "<codec> is not loaded");
				return false;
#endif
			}
			bool registry::import_preprocessor(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				auto vinclude_type = vm->set_enum("include_type");
				vinclude_type->set_value("error_t", (int)compute::include_type::error);
				vinclude_type->set_value("preprocess_t", (int)compute::include_type::preprocess);
				vinclude_type->set_value("unchaned_t", (int)compute::include_type::unchanged);
				vinclude_type->set_value("virtual_t", (int)compute::include_type::computed);

				auto vproc_directive = vm->set_struct_trivial<compute::proc_directive>("proc_directive");
				vproc_directive->set_property<compute::proc_directive>("string name", &compute::proc_directive::name);
				vproc_directive->set_property<compute::proc_directive>("string value", &compute::proc_directive::value);
				vproc_directive->set_property<compute::proc_directive>("usize start", &compute::proc_directive::start);
				vproc_directive->set_property<compute::proc_directive>("usize end", &compute::proc_directive::end);
				vproc_directive->set_property<compute::proc_directive>("bool found", &compute::proc_directive::found);
				vproc_directive->set_property<compute::proc_directive>("bool as_global", &compute::proc_directive::as_global);
				vproc_directive->set_property<compute::proc_directive>("bool as_scope", &compute::proc_directive::as_scope);
				vproc_directive->set_constructor<compute::proc_directive>("void f()");

				auto vinclude_desc = vm->set_struct_trivial<compute::include_desc>("include_desc");
				vinclude_desc->set_property<compute::include_desc>("string from", &compute::include_desc::from);
				vinclude_desc->set_property<compute::include_desc>("string path", &compute::include_desc::path);
				vinclude_desc->set_property<compute::include_desc>("string root", &compute::include_desc::root);
				vinclude_desc->set_constructor<compute::include_desc>("void f()");
				vinclude_desc->set_method_extern("void add_ext(const string_view&in)", &include_desc_add_ext);
				vinclude_desc->set_method_extern("void remove_ext(const string_view&in)", &include_desc_remove_ext);

				auto vinclude_result = vm->set_struct_trivial<compute::include_result>("include_result");
				vinclude_result->set_property<compute::include_result>("string module", &compute::include_result::library);
				vinclude_result->set_property<compute::include_result>("bool is_abstract", &compute::include_result::is_abstract);
				vinclude_result->set_property<compute::include_result>("bool is_remote", &compute::include_result::is_remote);
				vinclude_result->set_property<compute::include_result>("bool is_file", &compute::include_result::is_file);
				vinclude_result->set_constructor<compute::include_result>("void f()");

				auto vdesc = vm->set_pod<compute::preprocessor::desc>("preprocessor_desc");
				vdesc->set_property<compute::preprocessor::desc>("string multiline_comment_begin", &compute::preprocessor::desc::multiline_comment_begin);
				vdesc->set_property<compute::preprocessor::desc>("string multiline_comment_end", &compute::preprocessor::desc::multiline_comment_end);
				vdesc->set_property<compute::preprocessor::desc>("string comment_begin", &compute::preprocessor::desc::comment_begin);
				vdesc->set_property<compute::preprocessor::desc>("bool pragmas", &compute::preprocessor::desc::pragmas);
				vdesc->set_property<compute::preprocessor::desc>("bool includes", &compute::preprocessor::desc::includes);
				vdesc->set_property<compute::preprocessor::desc>("bool defines", &compute::preprocessor::desc::defines);
				vdesc->set_property<compute::preprocessor::desc>("bool conditions", &compute::preprocessor::desc::conditions);
				vdesc->set_constructor<compute::preprocessor::desc>("void f()");

				auto vpreprocessor = vm->set_class<compute::preprocessor>("preprocessor", false);
				vpreprocessor->set_function_def("include_type proc_include_sync(preprocessor@+, const include_result &in, string &out)");
				vpreprocessor->set_function_def("bool proc_pragma_sync(preprocessor@+, const string_view&in, array<string>@+)");
				vpreprocessor->set_function_def("bool proc_directive_sync(preprocessor@+, const proc_directive&in, string&out)");
				vpreprocessor->set_constructor<compute::preprocessor>("preprocessor@ f(uptr@)");
				vpreprocessor->set_method("void set_include_options(const include_desc &in)", &compute::preprocessor::set_include_options);
				vpreprocessor->set_method("void set_features(const preprocessor_desc &in)", &compute::preprocessor::set_features);
				vpreprocessor->set_method("void add_default_definitions()", &compute::preprocessor::add_default_definitions);
				vpreprocessor->set_method_extern("bool define(const string_view&in)", &VI_EXPECTIFY_VOID(compute::preprocessor::define));
				vpreprocessor->set_method("void undefine(const string_view&in)", &compute::preprocessor::undefine);
				vpreprocessor->set_method("void clear()", &compute::preprocessor::clear);
				vpreprocessor->set_method_extern("bool process(const string_view&in, string &out)", &VI_EXPECTIFY_VOID(compute::preprocessor::process));
				vpreprocessor->set_method_extern("string resolve_file(const string_view&in, const string_view&in)", &VI_EXPECTIFY(compute::preprocessor::resolve_file));
				vpreprocessor->set_method("const string& get_current_file_path() const", &compute::preprocessor::get_current_file_path);
				vpreprocessor->set_method_extern("void set_include_callback(proc_include_sync@)", &preprocessor_set_include_callback);
				vpreprocessor->set_method_extern("void set_pragma_callback(proc_pragma_sync@)", &preprocessor_set_pragma_callback);
				vpreprocessor->set_method_extern("void set_directive_callback(const string_view&in, proc_directive_sync@)", &preprocessor_set_directive_callback);
				vpreprocessor->set_method_extern("bool is_defined(const string_view&in) const", &preprocessor_is_defined);

				return true;
#else
				VI_ASSERT(false, "<preprocessor> is not loaded");
				return false;
#endif
			}
			bool registry::import_network(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				VI_TYPEREF(socket_address, "socket_address");
				VI_TYPEREF(socket_accept, "socket_accept");
				VI_TYPEREF(socket_listener, "socket_listener");
				VI_TYPEREF(socket_connection, "socket_connection");
				VI_TYPEREF(socket_server, "socket_server");
				VI_TYPEREF(socket, "socket@");
				VI_TYPEREF(string, "string");

				auto vsecure_layer_options = vm->set_enum("secure_layer_options");
				vsecure_layer_options->set_value("all", (int)network::secure_layer_options::all);
				vsecure_layer_options->set_value("ssl_v2", (int)network::secure_layer_options::no_sslv2);
				vsecure_layer_options->set_value("ssl_v3", (int)network::secure_layer_options::no_sslv3);
				vsecure_layer_options->set_value("tls_v1", (int)network::secure_layer_options::no_tlsv1);
				vsecure_layer_options->set_value("tls_v1_1", (int)network::secure_layer_options::no_tlsv11);
				vsecure_layer_options->set_value("tls_v1_2", (int)network::secure_layer_options::no_tlsv11);
				vsecure_layer_options->set_value("tls_v1_3", (int)network::secure_layer_options::no_tlsv11);

				auto vserver_state = vm->set_enum("server_state");
				vserver_state->set_value("working", (int)network::server_state::working);
				vserver_state->set_value("stopping", (int)network::server_state::stopping);
				vserver_state->set_value("idle", (int)network::server_state::idle);

				auto vsocket_poll = vm->set_enum("socket_poll");
				vsocket_poll->set_value("next", (int)network::socket_poll::next);
				vsocket_poll->set_value("reset", (int)network::socket_poll::reset);
				vsocket_poll->set_value("timeout", (int)network::socket_poll::timeout);
				vsocket_poll->set_value("cancel", (int)network::socket_poll::cancel);
				vsocket_poll->set_value("finish", (int)network::socket_poll::finish);
				vsocket_poll->set_value("finish_sync", (int)network::socket_poll::finish_sync);

				auto vsocket_protocol = vm->set_enum("socket_protocol");
				vsocket_protocol->set_value("ip", (int)network::socket_protocol::IP);
				vsocket_protocol->set_value("icmp", (int)network::socket_protocol::ICMP);
				vsocket_protocol->set_value("tcp", (int)network::socket_protocol::TCP);
				vsocket_protocol->set_value("udp", (int)network::socket_protocol::UDP);
				vsocket_protocol->set_value("raw", (int)network::socket_protocol::RAW);

				auto vsocket_type = vm->set_enum("socket_type");
				vsocket_type->set_value("stream", (int)network::socket_type::stream);
				vsocket_type->set_value("datagram", (int)network::socket_type::datagram);
				vsocket_type->set_value("raw", (int)network::socket_type::raw);
				vsocket_type->set_value("reliably_delivered_message", (int)network::socket_type::reliably_delivered_message);
				vsocket_type->set_value("sequence_packet_stream", (int)network::socket_type::sequence_packet_stream);

				auto vdns_type = vm->set_enum("dns_type");
				vdns_type->set_value("connect", (int)network::dns_type::connect);
				vdns_type->set_value("listen", (int)network::dns_type::listen);

				auto vsocket_address = vm->set_struct_trivial<network::socket_address>("socket_address");
				vsocket_address->set_constructor<network::socket_address>("void f()");
				vsocket_address->set_constructor<network::socket_address, const std::string_view&, uint16_t>("void f(const string_view&in, uint16)");
				vsocket_address->set_method("uptr@ get_address4_ptr() const", &network::socket_address::get_address4);
				vsocket_address->set_method("uptr@ get_address6_ptr() const", &network::socket_address::get_address6);
				vsocket_address->set_method("uptr@ get_address_ptr() const", &network::socket_address::get_raw_address);
				vsocket_address->set_method("usize get_address_ptr_size() const", &network::socket_address::get_address_size);
				vsocket_address->set_method("int32 get_flags() const", &network::socket_address::get_flags);
				vsocket_address->set_method("int32 get_family() const", &network::socket_address::get_family);
				vsocket_address->set_method("int32 get_type() const", &network::socket_address::get_type);
				vsocket_address->set_method("int32 get_protocol() const", &network::socket_address::get_protocol);
				vsocket_address->set_method("dns_type get_resolver_type() const", &network::socket_address::get_resolver_type);
				vsocket_address->set_method("socket_protocol get_protocol_type() const", &network::socket_address::get_protocol_type);
				vsocket_address->set_method("socket_type get_socket_type() const", &network::socket_address::get_socket_type);
				vsocket_address->set_method("bool is_valid() const", &network::socket_address::is_valid);
				vsocket_address->set_method_extern("string get_hostname() const", &socket_address_get_hostname);
				vsocket_address->set_method_extern("string get_ip_address() const", &socket_address_get_ip_address);
				vsocket_address->set_method_extern("uint16 get_ip_port() const", &socket_address_get_ip_port);

				auto vsocket_accept = vm->set_struct_trivial<network::socket_accept>("socket_accept");
				vsocket_accept->set_property<network::socket_accept>("socket_address address", &network::socket_accept::address);
				vsocket_accept->set_property<network::socket_accept>("usize fd", &network::socket_accept::fd);
				vsocket_accept->set_constructor<network::socket_accept>("void f()");

				auto vrouter_listener = vm->set_struct_trivial<network::router_listener>("router_listener");
				vrouter_listener->set_property<network::router_listener>("socket_address address", &network::router_listener::address);
				vrouter_listener->set_property<network::router_listener>("bool is_secure", &network::router_listener::is_secure);
				vrouter_listener->set_constructor<network::router_listener>("void f()");

				auto vlocation = vm->set_struct_trivial<network::location>("url_location");
				vlocation->set_property<network::location>("string protocol", &network::location::protocol);
				vlocation->set_property<network::location>("string username", &network::location::username);
				vlocation->set_property<network::location>("string password", &network::location::password);
				vlocation->set_property<network::location>("string hostname", &network::location::hostname);
				vlocation->set_property<network::location>("string fragment", &network::location::fragment);
				vlocation->set_property<network::location>("string path", &network::location::path);
				vlocation->set_property<network::location>("string body", &network::location::body);
				vlocation->set_property<network::location>("int32 port", &network::location::port);
				vlocation->set_constructor<network::location, const std::string_view&>("void f(const string_view&in)");
				vlocation->set_method_extern("dictionary@ get_query() const", &location_get_query);

				auto vcertificate = vm->set_struct_trivial<network::certificate>("certificate");
				vcertificate->set_property<network::certificate>("string subject_name", &network::certificate::subject_name);
				vcertificate->set_property<network::certificate>("string issuer_name", &network::certificate::issuer_name);
				vcertificate->set_property<network::certificate>("string serial_number", &network::certificate::serial_number);
				vcertificate->set_property<network::certificate>("string fingerprint", &network::certificate::fingerprint);
				vcertificate->set_property<network::certificate>("string public_key", &network::certificate::public_key);
				vcertificate->set_property<network::certificate>("string not_before_date", &network::certificate::not_before_date);
				vcertificate->set_property<network::certificate>("string not_after_date", &network::certificate::not_after_date);
				vcertificate->set_property<network::certificate>("int64 not_before_time", &network::certificate::not_before_time);
				vcertificate->set_property<network::certificate>("int64 not_after_time", &network::certificate::not_after_time);
				vcertificate->set_property<network::certificate>("int32 version", &network::certificate::version);
				vcertificate->set_property<network::certificate>("int32 signature", &network::certificate::signature);
				vcertificate->set_method_extern("dictionary@ get_extensions() const", &certificate_get_extensions);
				vcertificate->set_constructor<network::certificate>("void f()");

				auto vcertificate_blob = vm->set_struct_trivial<network::certificate_blob>("certificate_blob");
				vcertificate_blob->set_property<network::certificate_blob>("string certificate", &network::certificate_blob::certificate);
				vcertificate_blob->set_property<network::certificate_blob>("string private_key", &network::certificate_blob::private_key);
				vcertificate_blob->set_constructor<network::certificate_blob>("void f()");

				auto vsocket_certificate = vm->set_struct_trivial<network::socket_certificate>("socket_certificate");
				vsocket_certificate->set_property<network::socket_certificate>("certificate_blob blob", &network::socket_certificate::blob);
				vsocket_certificate->set_property<network::socket_certificate>("string ciphers", &network::socket_certificate::ciphers);
				vsocket_certificate->set_property<network::socket_certificate>("uptr@ context", &network::socket_certificate::context);
				vsocket_certificate->set_property<network::socket_certificate>("secure_layer_options options", &network::socket_certificate::options);
				vsocket_certificate->set_property<network::socket_certificate>("uint32 verify_peers", &network::socket_certificate::verify_peers);
				vsocket_certificate->set_constructor<network::socket_certificate>("void f()");

				auto vdata_frame = vm->set_struct_trivial<network::data_frame>("socket_data_frame");
				vdata_frame->set_property<network::data_frame>("string message", &network::data_frame::message);
				vdata_frame->set_property<network::data_frame>("usize reuses", &network::data_frame::reuses);
				vdata_frame->set_property<network::data_frame>("int64 start", &network::data_frame::start);
				vdata_frame->set_property<network::data_frame>("int64 finish", &network::data_frame::finish);
				vdata_frame->set_property<network::data_frame>("int64 timeout", &network::data_frame::timeout);
				vdata_frame->set_property<network::data_frame>("bool abort", &network::data_frame::abort);
				vdata_frame->set_constructor<network::data_frame>("void f()");

				auto vsocket = vm->set_class<network::socket>("socket", false);
				auto vepoll_fd = vm->set_struct<network::epoll_fd>("epoll_fd");
				vepoll_fd->set_property<network::epoll_fd>("socket@ base", &network::epoll_fd::base);
				vepoll_fd->set_property<network::epoll_fd>("bool readable", &network::epoll_fd::readable);
				vepoll_fd->set_property<network::epoll_fd>("bool writeable", &network::epoll_fd::writeable);
				vepoll_fd->set_property<network::epoll_fd>("bool closeable", &network::epoll_fd::closeable);
				vepoll_fd->set_constructor<network::epoll_fd>("void f()");
				vepoll_fd->set_operator_copy_static(&epoll_fd_copy);
				vepoll_fd->set_destructor_extern("void f()", &epoll_fd_destructor);

				auto vepoll_interface = vm->set_struct<network::epoll_interface>("epoll_interface");
				vepoll_interface->set_constructor<network::epoll_interface, size_t>("void f(usize)");
				vepoll_interface->set_method("bool add(socket@+, bool, bool)", &network::epoll_interface::add);
				vepoll_interface->set_method("bool update(socket@+, bool, bool)", &network::epoll_interface::update);
				vepoll_interface->set_method("bool remove(socket@+)", &network::epoll_interface::remove);
				vepoll_interface->set_method("usize capacity() const", &network::epoll_interface::capacity);
				vepoll_interface->set_method_extern("int wait(array<epoll_fd>@+, uint64)", &epoll_interface_wait);
				vepoll_interface->set_operator_move_copy<network::epoll_interface>();
				vepoll_interface->set_destructor<network::epoll_interface>("void f()");

				auto vstream = vm->set_class<core::file_stream>("file_stream", false);
				vsocket->set_function_def("void socket_accept_async(socket_accept&in)");
				vsocket->set_function_def("void socket_status_async(int)");
				vsocket->set_function_def("void socket_send_async(socket_poll)");
				vsocket->set_function_def("bool socket_recv_async(socket_poll, const string_view&in)");
				vsocket->set_property<network::socket>("int64 income", &network::socket::income);
				vsocket->set_property<network::socket>("int64 outcome", &network::socket::outcome);
				vsocket->set_constructor<network::socket>("socket@ f()");
				vsocket->set_constructor<network::socket, socket_t>("socket@ f(usize)");
				vsocket->set_method("usize get_fd() const", &network::socket::get_fd);
				vsocket->set_method("uptr@ get_device() const", &network::socket::get_device);
				vsocket->set_method("bool is_awaiting_readable() const", &network::socket::is_awaiting_readable);
				vsocket->set_method("bool is_awaiting_writeable() const", &network::socket::is_awaiting_writeable);
				vsocket->set_method("bool is_awaiting_events() const", &network::socket::is_awaiting_events);
				vsocket->set_method("bool is_valid() const", &network::socket::is_valid);
				vsocket->set_method("bool is_secure() const", &network::socket::is_secure);
				vsocket->set_method("void set_io_timeout(uint64)", &network::socket::set_io_timeout);
				vsocket->set_method_extern("promise<socket_accept>@ accept_deferred()", &VI_SPROMISIFY_REF(socket_accept_deferred, socket_accept));
				vsocket->set_method_extern("promise<bool>@ connect_deferred(const socket_address&in)", &VI_SPROMISIFY(socket_connect_deferred, type_id::bool_t));
				vsocket->set_method_extern("promise<bool>@ close_deferred()", &VI_SPROMISIFY(socket_close_deferred, type_id::bool_t));
				vsocket->set_method_extern("promise<bool>@ write_file_deferred(file_stream@+, usize, usize)", &VI_SPROMISIFY(socket_write_file_deferred, type_id::bool_t));
				vsocket->set_method_extern("promise<bool>@ write_deferred(const string_view&in)", &VI_SPROMISIFY(socket_write_deferred, type_id::bool_t));
				vsocket->set_method_extern("promise<string>@ read_deferred(usize)", &VI_SPROMISIFY_REF(socket_read_deferred, string));
				vsocket->set_method_extern("promise<string>@ read_until_deferred(const string_view&in, usize)", &VI_SPROMISIFY_REF(socket_read_until_deferred, string));
				vsocket->set_method_extern("promise<string>@ read_until_chunked_deferred(const string_view&in, usize)", &VI_SPROMISIFY_REF(socket_read_until_chunked_deferred, string));
				vsocket->set_method_extern("bool accept(socket_accept&out)", &VI_EXPECTIFY_VOID(network::socket::accept));
				vsocket->set_method_extern("bool connect(const socket_address&in, uint64)", &VI_EXPECTIFY_VOID(network::socket::connect));
				vsocket->set_method_extern("bool close()", &VI_EXPECTIFY_VOID(network::socket::close));
				vsocket->set_method_extern("usize write_file(file_stream@+, usize, usize)", &socket_write_file);
				vsocket->set_method_extern("usize write(const string_view&in)", &socket_write);
				vsocket->set_method_extern("usize read(string &out, usize)", &socket_read);
				vsocket->set_method_extern("usize read_until(const string_view&in, socket_recv_async@)", &socket_read_until);
				vsocket->set_method_extern("usize read_until_chunked(const string_view&in, socket_recv_async@)", &socket_read_until_chunked);
				vsocket->set_method_extern("socket_address get_peer_address() const", &VI_EXPECTIFY(network::socket::get_peer_address));
				vsocket->set_method_extern("socket_address get_this_address() const", &VI_EXPECTIFY(network::socket::get_this_address));
				vsocket->set_method_extern("bool get_any_flag(int, int, int &out) const", &VI_EXPECTIFY_VOID(network::socket::get_any_flag));
				vsocket->set_method_extern("bool get_socket_flag(int, int &out) const", &VI_EXPECTIFY_VOID(network::socket::get_socket_flag));
				vsocket->set_method_extern("bool set_close_on_exec()", VI_EXPECTIFY_VOID(network::socket::set_close_on_exec));
				vsocket->set_method_extern("bool set_time_wait(int)", VI_EXPECTIFY_VOID(network::socket::set_time_wait));
				vsocket->set_method_extern("bool set_any_flag(int, int, int)", VI_EXPECTIFY_VOID(network::socket::set_any_flag));
				vsocket->set_method_extern("bool set_socket_flag(int, int)", VI_EXPECTIFY_VOID(network::socket::set_socket_flag));
				vsocket->set_method_extern("bool set_blocking(bool)", VI_EXPECTIFY_VOID(network::socket::set_blocking));
				vsocket->set_method_extern("bool set_no_delay(bool)", VI_EXPECTIFY_VOID(network::socket::set_no_delay));
				vsocket->set_method_extern("bool set_keep_alive(bool)", VI_EXPECTIFY_VOID(network::socket::set_keep_alive));
				vsocket->set_method_extern("bool set_timeout(int)", VI_EXPECTIFY_VOID(network::socket::set_timeout));
				vsocket->set_method_extern("bool shutdown(bool = false)", &VI_EXPECTIFY_VOID(network::socket::shutdown));
				vsocket->set_method_extern("bool open(const socket_address&in)", &VI_EXPECTIFY_VOID(network::socket::open));
				vsocket->set_method_extern("bool secure(uptr@, const string_view&in)", &VI_EXPECTIFY_VOID(network::socket::secure));
				vsocket->set_method_extern("bool bind(const socket_address&in)", &VI_EXPECTIFY_VOID(network::socket::bind));
				vsocket->set_method_extern("bool listen(int)", &VI_EXPECTIFY_VOID(network::socket::listen));
				vsocket->set_method_extern("bool clear_events(bool)", &VI_EXPECTIFY_VOID(network::socket::clear_events));
				vsocket->set_method_extern("bool migrate_to(usize, bool = true)", &VI_EXPECTIFY_VOID(network::socket::migrate_to));

				vm->begin_namespace("net_packet");
				vm->set_function("bool is_data(socket_poll)", &network::packet::is_data);
				vm->set_function("bool is_skip(socket_poll)", &network::packet::is_skip);
				vm->set_function("bool is_done(socket_poll)", &network::packet::is_done);
				vm->set_function("bool is_done_sync(socket_poll)", &network::packet::is_done_sync);
				vm->set_function("bool is_done_async(socket_poll)", &network::packet::is_done_async);
				vm->set_function("bool is_timeout(socket_poll)", &network::packet::is_timeout);
				vm->set_function("bool is_error(socket_poll)", &network::packet::is_error);
				vm->set_function("bool is_error_or_skip(socket_poll)", &network::packet::is_error_or_skip);
				vm->end_namespace();

				auto VDNS = vm->set_class<network::dns>("dns", false);
				VDNS->set_constructor<network::dns>("dns@ f()");
				VDNS->set_method_extern("string reverse_lookup(const string_view&in, const string_view&in = string_view())", &VI_EXPECTIFY(network::dns::reverse_lookup));
				VDNS->set_method_extern("promise<string>@ reverse_lookup_deferred(const string_view&in, const string_view&in = string_view())", &VI_SPROMISIFY_REF(dns_reverse_lookup_deferred, string));
				VDNS->set_method_extern("string reverse_address_lookup(const socket_address&in)", &VI_EXPECTIFY(network::dns::reverse_address_lookup));
				VDNS->set_method_extern("promise<string>@ reverse_address_lookup_deferred(const socket_address&in)", &VI_SPROMISIFY_REF(dns_reverse_address_lookup_deferred, string));
				VDNS->set_method_extern("socket_address lookup(const string_view&in, const string_view&in, dns_type, socket_protocol = socket_protocol::tcp, socket_type = socket_type::stream)", &VI_EXPECTIFY(network::dns::lookup));
				VDNS->set_method_extern("promise<socket_address>@ lookup_deferred(const string_view&in, const string_view&in, dns_type, socket_protocol = socket_protocol::tcp, socket_type = socket_type::stream)", &VI_SPROMISIFY_REF(dns_lookup_deferred, socket_address));
				VDNS->set_method_static("dns@+ get()", &network::dns::get);

				auto vmultiplexer = vm->set_class<network::multiplexer>("multiplexer", false);
				vmultiplexer->set_function_def("void poll_async(socket@+, socket_poll)");
				vmultiplexer->set_constructor<network::multiplexer>("multiplexer@ f()");
				vmultiplexer->set_constructor<network::multiplexer, uint64_t, size_t>("multiplexer@ f(uint64, usize)");
				vmultiplexer->set_method("void rescale(uint64, usize)", &network::multiplexer::rescale);
				vmultiplexer->set_method("void activate()", &network::multiplexer::activate);
				vmultiplexer->set_method("void deactivate()", &network::multiplexer::deactivate);
				vmultiplexer->set_method("int dispatch(uint64)", &network::multiplexer::dispatch);
				vmultiplexer->set_method("bool shutdown()", &network::multiplexer::shutdown);
				vmultiplexer->set_method_extern("bool when_readable(socket@+, poll_async@)", &multiplexer_when_readable);
				vmultiplexer->set_method_extern("bool when_writeable(socket@+, poll_async@)", &multiplexer_when_writeable);
				vmultiplexer->set_method("bool cancel_events(socket@+, socket_poll = socket_poll::cancel, bool = true)", &network::multiplexer::cancel_events);
				vmultiplexer->set_method("bool clear_events(socket@+)", &network::multiplexer::clear_events);
				vmultiplexer->set_method("bool is_listening()", &network::multiplexer::is_listening);
				vmultiplexer->set_method("usize get_activations()", &network::multiplexer::get_activations);
				vmultiplexer->set_method_static("multiplexer@+ get()", &network::multiplexer::get);

				auto vuplinks = vm->set_class<network::uplinks>("uplinks", false);
				vuplinks->set_constructor<network::uplinks>("uplinks@ f()");
				vuplinks->set_method("void set_max_duplicates(usize)", &network::uplinks::set_max_duplicates);
				vuplinks->set_method("bool push_connection(const socket_address&in, socket@+)", &network::uplinks::push_connection);
				vuplinks->set_method_extern("promise<socket@>@ pop_connection(const socket_address&in)", &VI_PROMISIFY_REF(network::uplinks::pop_connection, socket));
				vuplinks->set_method("usize max_duplicates() const", &network::uplinks::get_max_duplicates);
				vuplinks->set_method("usize size() const", &network::uplinks::get_size);
				vuplinks->set_method_static("uplinks@+ get()", &network::uplinks::get);

				auto vsocket_listener = vm->set_class<network::socket_listener>("socket_listener", true);
				vsocket_listener->set_property<network::socket_listener>("string name", &network::socket_listener::name);
				vsocket_listener->set_property<network::socket_listener>("socket_address source", &network::socket_listener::address);
				vsocket_listener->set_property<network::socket_listener>("socket@ stream", &network::socket_listener::stream);
				vsocket_listener->set_gc_constructor<network::socket_listener, socket_listener, const std::string_view&, const network::socket_address&, bool>("socket_listener@ f(const string_view&in, const socket_address&in, bool)");
				vsocket_listener->set_enum_refs_extern<network::socket_listener>([](network::socket_listener* base, asIScriptEngine* vm)
				{
					function_factory::gc_enum_callback(vm, base->stream);
				});
				vsocket_listener->set_release_refs_extern<network::socket_listener>([](network::socket_listener* base, asIScriptEngine*)
				{
					base->~socket_listener();
				});

				auto vsocket_router = vm->set_class<network::socket_router>("socket_router", false);
				vsocket_router->set_property<network::socket_router>("usize max_heap_buffer", &network::socket_router::max_heap_buffer);
				vsocket_router->set_property<network::socket_router>("usize max_net_buffer", &network::socket_router::max_net_buffer);
				vsocket_router->set_property<network::socket_router>("usize backlog_queue", &network::socket_router::backlog_queue);
				vsocket_router->set_property<network::socket_router>("usize socket_timeout", &network::socket_router::socket_timeout);
				vsocket_router->set_property<network::socket_router>("usize max_connections", &network::socket_router::max_connections);
				vsocket_router->set_property<network::socket_router>("int64 keep_alive_max_count", &network::socket_router::keep_alive_max_count);
				vsocket_router->set_property<network::socket_router>("int64 graceful_time_wait", &network::socket_router::graceful_time_wait);
				vsocket_router->set_property<network::socket_router>("bool enable_no_delay", &network::socket_router::enable_no_delay);
				vsocket_router->set_constructor<network::socket_router>("socket_router@ f()");
				vsocket_router->set_method_extern("void listen(const string_view&in, const string_view&in, bool = false)", &socket_router_listen1);
				vsocket_router->set_method_extern("void listen(const string_view&in, const string_view&in, const string_view&in, bool = false)", &socket_router_listen2);
				vsocket_router->set_method_extern("void set_listeners(dictionary@ data)", &socket_router_set_listeners);
				vsocket_router->set_method_extern("dictionary@ get_listeners() const", &socket_router_get_listeners);
				vsocket_router->set_method_extern("void set_certificates(dictionary@ data)", &socket_router_set_certificates);
				vsocket_router->set_method_extern("dictionary@ get_certificates() const", &socket_router_get_certificates);

				auto vsocket_connection = vm->set_class<network::socket_connection>("socket_connection", true);
				vsocket_connection->set_property<network::socket_connection>("socket@ stream", &network::socket_connection::stream);
				vsocket_connection->set_property<network::socket_connection>("socket_listener@ host", &network::socket_connection::host);
				vsocket_connection->set_property<network::socket_connection>("socket_address address", &network::socket_connection::address);
				vsocket_connection->set_property<network::socket_connection>("socket_data_frame info", &network::socket_connection::info);
				vsocket_connection->set_gc_constructor<network::socket_connection, socket_connection>("socket_connection@ f()");
				vsocket_connection->set_method("void reset(bool)", &network::socket_connection::reset);
				vsocket_connection->set_method<network::socket_connection, bool>("bool next()", &network::socket_connection::next);
				vsocket_connection->set_method<network::socket_connection, bool, int>("bool next(int32)", &network::socket_connection::next);
				vsocket_connection->set_method<network::socket_connection, bool>("bool abort()", &network::socket_connection::abort);
				vsocket_connection->set_method_extern("bool abort(int, const string_view&in)", &socket_connection_abort);
				vsocket_connection->set_enum_refs_extern<network::socket_connection>([](network::socket_connection* base, asIScriptEngine* vm)
				{
					function_factory::gc_enum_callback(vm, base->stream);
					function_factory::gc_enum_callback(vm, base->host);
				});
				vsocket_connection->set_release_refs_extern<network::socket_connection>([](network::socket_connection* base, asIScriptEngine*)
				{
					base->~socket_connection();
				});

				auto vsocket_server = vm->set_class<network::socket_server>("socket_server", true);
				vsocket_server->set_gc_constructor<network::socket_server, socket_server>("socket_server@ f()");
				vsocket_server->set_method_extern("void set_router(socket_router@+)", &socket_server_set_router);
				vsocket_server->set_method_extern("bool configure(socket_router@+)", &socket_server_configure);
				vsocket_server->set_method_extern("bool listen()", &socket_server_listen);
				vsocket_server->set_method_extern("bool unlisten(bool)", &socket_server_unlisten);
				vsocket_server->set_method("void set_backlog(usize)", &network::socket_server::set_backlog);
				vsocket_server->set_method("usize get_backlog() const", &network::socket_server::get_backlog);
				vsocket_server->set_method("server_state get_state() const", &network::socket_server::get_state);
				vsocket_server->set_method("socket_router@+ get_router() const", &network::socket_server::get_router);
				vsocket_server->set_enum_refs_extern<network::socket_server>([](network::socket_server* base, asIScriptEngine* vm)
				{
					function_factory::gc_enum_callback(vm, base->get_router());
					for (auto* item : base->get_active_clients())
						function_factory::gc_enum_callback(vm, item);

					for (auto* item : base->get_pooled_clients())
						function_factory::gc_enum_callback(vm, item);
				});
				vsocket_server->set_release_refs_extern<network::socket_server>([](network::socket_server* base, asIScriptEngine*)
				{
					base->~socket_server();
				});

				auto vsocket_client = vm->set_class<network::socket_client>("socket_client", false);
				vsocket_client->set_constructor<network::socket_client, int64_t>("socket_client@ f(int64)");
				vsocket_client->set_method_extern("promise<bool>@ connect_sync(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(socket_client_connect_sync, type_id::bool_t));
				vsocket_client->set_method_extern("promise<bool>@ connect_async(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(socket_client_connect_async, type_id::bool_t));
				vsocket_client->set_method_extern("promise<bool>@ disconnect()", &VI_SPROMISIFY(socket_client_disconnect, type_id::bool_t));
				vsocket_client->set_method("void apply_reusability(bool)", &network::socket_client::apply_reusability);
				vsocket_client->set_method("bool has_stream() const", &network::socket_client::has_stream);
				vsocket_client->set_method("bool is_secure() const", &network::socket_client::is_secure);
				vsocket_client->set_method("bool is_verified() const", &network::socket_client::is_verified);
				vsocket_client->set_method("const socket_address& get_peer_address() const", &network::socket_client::get_peer_address);
				vsocket_client->set_method("socket@+ get_stream() const", &network::socket_client::get_stream);

				return true;
#else
				VI_ASSERT(false, "<network> is not loaded");
				return false;
#endif
			}
			bool registry::import_http(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				VI_TYPEREF(router_group, "http::route_group");
				VI_TYPEREF(site_entry, "http::site_entry");
				VI_TYPEREF(map_router, "http::map_router");
				VI_TYPEREF(server, "http::server");
				VI_TYPEREF(connection, "http::connection");
				VI_TYPEREF(response_frame, "http::response_frame");
				VI_TYPEREF(array_resource_info, "array<http::resource_info>@");
				VI_TYPEREF(string, "string");
				VI_TYPEREF(schema, "schema");

				vm->begin_namespace("http");
				auto vauth = vm->set_enum("auth");
				vauth->set_value("granted", (int)network::http::auth::granted);
				vauth->set_value("denied", (int)network::http::auth::denied);
				vauth->set_value("unverified", (int)network::http::auth::unverified);

				auto vweb_socket_op = vm->set_enum("websocket_op");
				vweb_socket_op->set_value("next", (int)network::http::web_socket_op::next);
				vweb_socket_op->set_value("text", (int)network::http::web_socket_op::text);
				vweb_socket_op->set_value("binary", (int)network::http::web_socket_op::binary);
				vweb_socket_op->set_value("close", (int)network::http::web_socket_op::close);
				vweb_socket_op->set_value("ping", (int)network::http::web_socket_op::ping);
				vweb_socket_op->set_value("pong", (int)network::http::web_socket_op::pong);

				auto vweb_socket_state = vm->set_enum("websocket_state");
				vweb_socket_state->set_value("open", (int)network::http::web_socket_state::open);
				vweb_socket_state->set_value("receive", (int)network::http::web_socket_state::receive);
				vweb_socket_state->set_value("process", (int)network::http::web_socket_state::process);
				vweb_socket_state->set_value("close", (int)network::http::web_socket_state::close);

				auto vcompression_tune = vm->set_enum("compression_tune");
				vcompression_tune->set_value("filtered", (int)network::http::compression_tune::filtered);
				vcompression_tune->set_value("huffman", (int)network::http::compression_tune::huffman);
				vcompression_tune->set_value("rle", (int)network::http::compression_tune::rle);
				vcompression_tune->set_value("fixed", (int)network::http::compression_tune::fixed);
				vcompression_tune->set_value("default_tune", (int)network::http::compression_tune::default_tune);

				auto vroute_mode = vm->set_enum("route_mode");
				vroute_mode->set_value("start", (int)network::http::route_mode::start);
				vroute_mode->set_value("match", (int)network::http::route_mode::match);
				vroute_mode->set_value("end", (int)network::http::route_mode::end);

				auto verror_file = vm->set_struct_trivial<network::http::error_file>("error_file");
				verror_file->set_property<network::http::error_file>("string pattern", &network::http::error_file::pattern);
				verror_file->set_property<network::http::error_file>("int32 status_code", &network::http::error_file::status_code);
				verror_file->set_constructor<network::http::error_file>("void f()");

				auto vmime_type = vm->set_struct_trivial<network::http::mime_type>("mime_type");
				vmime_type->set_property<network::http::mime_type>("string extension", &network::http::mime_type::extension);
				vmime_type->set_property<network::http::mime_type>("string type", &network::http::mime_type::type);
				vmime_type->set_constructor<network::http::mime_type>("void f()");

				auto vcredentials = vm->set_struct_trivial<network::http::credentials>("credentials");
				vcredentials->set_property<network::http::credentials>("string token", &network::http::credentials::token);
				vcredentials->set_property<network::http::credentials>("auth type", &network::http::credentials::type);
				vcredentials->set_constructor<network::http::credentials>("void f()");

				auto vresource = vm->set_struct_trivial<network::http::resource>("resource_info");
				vresource->set_property<network::http::resource>("string path", &network::http::resource::path);
				vresource->set_property<network::http::resource>("string type", &network::http::resource::type);
				vresource->set_property<network::http::resource>("string name", &network::http::resource::name);
				vresource->set_property<network::http::resource>("string key", &network::http::resource::key);
				vresource->set_property<network::http::resource>("usize length", &network::http::resource::length);
				vresource->set_property<network::http::resource>("bool is_in_memory", &network::http::resource::is_in_memory);
				vresource->set_constructor<network::http::resource>("void f()");
				vresource->set_method("string& put_header(const string_view&in, const string_view&in)", &network::http::resource::put_header);
				vresource->set_method("string& set_header(const string_view&in, const string_view&in)", &network::http::resource::set_header);
				vresource->set_method("string compose_header(const string_view&in) const", &network::http::resource::compose_header);
				vresource->set_method_extern("string get_header(const string_view&in) const", &resource_get_header_blob);
				vresource->set_method("const string& get_in_memory_contents() const", &network::http::resource::get_in_memory_contents);

				auto vcookie = vm->set_struct_trivial<network::http::cookie>("cookie");
				vcookie->set_property<network::http::cookie>("string name", &network::http::cookie::name);
				vcookie->set_property<network::http::cookie>("string value", &network::http::cookie::value);
				vcookie->set_property<network::http::cookie>("string domain", &network::http::cookie::domain);
				vcookie->set_property<network::http::cookie>("string same_site", &network::http::cookie::same_site);
				vcookie->set_property<network::http::cookie>("string expires", &network::http::cookie::expires);
				vcookie->set_property<network::http::cookie>("bool secure", &network::http::cookie::secure);
				vcookie->set_property<network::http::cookie>("bool http_only", &network::http::cookie::http_only);
				vcookie->set_constructor<network::http::cookie>("void f()");
				vcookie->set_method("void set_expires(int64)", &network::http::cookie::set_expires);
				vcookie->set_method("void set_expired()", &network::http::cookie::set_expired);

				auto vrequest_frame = vm->set_struct_trivial<network::http::request_frame>("request_frame");
				auto vresponse_frame = vm->set_struct_trivial<network::http::response_frame>("response_frame");
				auto vfetch_frame = vm->set_struct_trivial<network::http::fetch_frame>("fetch_frame");
				auto vcontent_frame = vm->set_struct_trivial<network::http::content_frame>("content_frame");
				vcontent_frame->set_property<network::http::content_frame>("usize length", &network::http::content_frame::length);
				vcontent_frame->set_property<network::http::content_frame>("usize offset", &network::http::content_frame::offset);
				vcontent_frame->set_property<network::http::content_frame>("bool exceeds", &network::http::content_frame::exceeds);
				vcontent_frame->set_property<network::http::content_frame>("bool limited", &network::http::content_frame::limited);
				vcontent_frame->set_constructor<network::http::content_frame>("void f()");
				vcontent_frame->set_method<network::http::content_frame, void, const std::string_view&>("void append(const string_view&in)", &network::http::content_frame::append);
				vcontent_frame->set_method<network::http::content_frame, void, const std::string_view&>("void assign(const string_view&in)", &network::http::content_frame::assign);
				vcontent_frame->set_method("void finalize()", &network::http::content_frame::finalize);
				vcontent_frame->set_method("void cleanup()", &network::http::content_frame::cleanup);
				vcontent_frame->set_method("bool is_finalized() const", &network::http::content_frame::is_finalized);
				vcontent_frame->set_method("string get_text() const", &network::http::content_frame::get_text);
				vcontent_frame->set_method_extern("schema@ get_json() const", &content_frame_get_json);
				vcontent_frame->set_method_extern("schema@ get_xml() const", &content_frame_get_xml);
				vcontent_frame->set_method_extern("void prepare(const request_frame&in, const string_view&in)", &content_frame_prepare1);
				vcontent_frame->set_method_extern("void prepare(const response_frame&in, const string_view&in)", &content_frame_prepare2);
				vcontent_frame->set_method_extern("void prepare(const fetch_frame&in, const string_view&in)", &content_frame_prepare3);
				vcontent_frame->set_method_extern("void add_resource(const resource_info&in)", &content_frame_add_resource);
				vcontent_frame->set_method_extern("void clear_resources()", &content_frame_clear_resources);
				vcontent_frame->set_method_extern("resource_info get_resource(usize) const", &content_frame_get_resource);
				vcontent_frame->set_method_extern("usize get_resources_size() const", &content_frame_get_resources_size);

				vrequest_frame->set_property<network::http::request_frame>("content_frame content", &network::http::request_frame::content);
				vrequest_frame->set_property<network::http::request_frame>("string query", &network::http::request_frame::query);
				vrequest_frame->set_property<network::http::request_frame>("string path", &network::http::request_frame::path);
				vrequest_frame->set_property<network::http::request_frame>("string location", &network::http::request_frame::location);
				vrequest_frame->set_property<network::http::request_frame>("string referrer", &network::http::request_frame::referrer);
				vrequest_frame->set_property<network::http::request_frame>("regex_result match", &network::http::request_frame::match);
				vrequest_frame->set_property<network::http::request_frame>("credentials user", &network::http::request_frame::user);
				vrequest_frame->set_constructor<network::http::request_frame>("void f()");
				vrequest_frame->set_method("string& put_header(const string_view&in, const string_view&in)", &network::http::request_frame::put_header);
				vrequest_frame->set_method("string& set_header(const string_view&in, const string_view&in)", &network::http::request_frame::set_header);
				vrequest_frame->set_method("void set_version(uint32, uint32)", &network::http::request_frame::set_version);
				vrequest_frame->set_method("void cleanup()", &network::http::request_frame::cleanup);
				vrequest_frame->set_method("string compose_header(const string_view&in) const", &network::http::request_frame::compose_header);
				vrequest_frame->set_method_extern("string get_cookie(const string_view&in) const", &request_frame_get_cookie_blob);
				vrequest_frame->set_method_extern("string get_header(const string_view&in) const", &request_frame_get_header_blob);
				vrequest_frame->set_method_extern("string get_header(usize, usize = 0) const", &request_frame_get_header);
				vrequest_frame->set_method_extern("string get_cookie(usize, usize = 0) const", &request_frame_get_cookie);
				vrequest_frame->set_method_extern("usize get_headers_size() const", &request_frame_get_headers_size);
				vrequest_frame->set_method_extern("usize get_header_size(usize) const", &request_frame_get_header_size);
				vrequest_frame->set_method_extern("usize get_cookies_size() const", &request_frame_get_cookies_size);
				vrequest_frame->set_method_extern("usize get_cookie_size(usize) const", &request_frame_get_cookie_size);
				vrequest_frame->set_method_extern("void set_method(const string_view&in)", &request_frame_set_method);
				vrequest_frame->set_method_extern("string get_method() const", &request_frame_get_method);
				vrequest_frame->set_method_extern("string get_version() const", &request_frame_get_version);

				vresponse_frame->set_property<network::http::response_frame>("content_frame content", &network::http::response_frame::content);
				vresponse_frame->set_property<network::http::response_frame>("int32 status_code", &network::http::response_frame::status_code);
				vresponse_frame->set_property<network::http::response_frame>("bool error", &network::http::response_frame::error);
				vresponse_frame->set_constructor<network::http::response_frame>("void f()");
				vresponse_frame->set_method("string& put_header(const string_view&in, const string_view&in)", &network::http::response_frame::put_header);
				vresponse_frame->set_method("string& set_header(const string_view&in, const string_view&in)", &network::http::response_frame::set_header);
				vresponse_frame->set_method<network::http::response_frame, void, const network::http::cookie&>("void set_cookie(const cookie&in)", &network::http::response_frame::set_cookie);
				vresponse_frame->set_method("void cleanup()", &network::http::response_frame::cleanup);
				vresponse_frame->set_method("string compose_header(const string_view&in) const", &network::http::response_frame::compose_header);
				vresponse_frame->set_method_extern("string get_header(const string_view&in) const", &response_frame_get_header_blob);
				vresponse_frame->set_method("bool is_undefined() const", &network::http::response_frame::is_undefined);
				vresponse_frame->set_method("bool is_ok() const", &network::http::response_frame::is_ok);
				vresponse_frame->set_method_extern("string get_header(usize, usize) const", &response_frame_get_header);
				vresponse_frame->set_method_extern("cookie get_cookie(const string_view&in) const", &response_frame_get_cookie1);
				vresponse_frame->set_method_extern("cookie get_cookie(usize) const", &response_frame_get_cookie2);
				vresponse_frame->set_method_extern("usize get_headers_size() const", &response_frame_get_headers_size);
				vresponse_frame->set_method_extern("usize get_header_size(usize) const", &response_frame_get_header_size);
				vresponse_frame->set_method_extern("usize get_cookies_size() const", &response_frame_get_cookies_size);

				vfetch_frame->set_property<network::http::fetch_frame>("content_frame content", &network::http::fetch_frame::content);
				vfetch_frame->set_property<network::http::fetch_frame>("uint64 timeout", &network::http::fetch_frame::timeout);
				vfetch_frame->set_property<network::http::fetch_frame>("uint32 verify_peers", &network::http::fetch_frame::verify_peers);
				vfetch_frame->set_property<network::http::fetch_frame>("usize max_size", &network::http::fetch_frame::max_size);
				vfetch_frame->set_constructor<network::http::fetch_frame>("void f()");
				vfetch_frame->set_method("void put_header(const string_view&in, const string_view&in)", &network::http::fetch_frame::put_header);
				vfetch_frame->set_method("void set_header(const string_view&in, const string_view&in)", &network::http::fetch_frame::set_header);
				vfetch_frame->set_method("void cleanup()", &network::http::fetch_frame::cleanup);
				vfetch_frame->set_method("string compose_header(const string_view&in) const", &network::http::fetch_frame::compose_header);
				vfetch_frame->set_method_extern("string get_cookie(const string_view&in) const", &fetch_frame_get_cookie_blob);
				vfetch_frame->set_method_extern("string get_header(const string_view&in) const", &fetch_frame_get_header_blob);
				vfetch_frame->set_method_extern("string get_header(usize, usize = 0) const", &fetch_frame_get_header);
				vfetch_frame->set_method_extern("string get_cookie(usize, usize = 0) const", &fetch_frame_get_cookie);
				vfetch_frame->set_method_extern("usize get_headers_size() const", &fetch_frame_get_headers_size);
				vfetch_frame->set_method_extern("usize get_header_size(usize) const", &fetch_frame_get_header_size);
				vfetch_frame->set_method_extern("usize get_cookies_size() const", &fetch_frame_get_cookies_size);
				vfetch_frame->set_method_extern("usize get_cookie_size(usize) const", &fetch_frame_get_cookie_size);

				auto vroute_auth = vm->set_struct_trivial<network::http::router_entry::entry_auth>("route_auth");
				vroute_auth->set_property<network::http::router_entry::entry_auth>("string type", &network::http::router_entry::entry_auth::type);
				vroute_auth->set_property<network::http::router_entry::entry_auth>("string realm", &network::http::router_entry::entry_auth::realm);
				vroute_auth->set_constructor<network::http::router_entry::entry_auth>("void f()");
				vroute_auth->set_method_extern("void set_methods(array<string>@+)", &route_auth_set_methods);
				vroute_auth->set_method_extern("array<string>@ get_methods() const", &route_auth_get_methods);

				auto vroute_compression = vm->set_struct_trivial<network::http::router_entry::entry_compression>("route_compression");
				vroute_compression->set_property<network::http::router_entry::entry_compression>("compression_tune tune", &network::http::router_entry::entry_compression::tune);
				vroute_compression->set_property<network::http::router_entry::entry_compression>("int32 quality_level", &network::http::router_entry::entry_compression::quality_level);
				vroute_compression->set_property<network::http::router_entry::entry_compression>("int32 memory_level", &network::http::router_entry::entry_compression::memory_level);
				vroute_compression->set_property<network::http::router_entry::entry_compression>("usize min_length", &network::http::router_entry::entry_compression::min_length);
				vroute_compression->set_property<network::http::router_entry::entry_compression>("bool enabled", &network::http::router_entry::entry_compression::enabled);
				vroute_compression->set_constructor<network::http::router_entry::entry_compression>("void f()");
				vroute_compression->set_method_extern("void set_files(array<regex_source>@+)", &route_compression_set_files);
				vroute_compression->set_method_extern("array<regex_source>@ get_files() const", &route_compression_get_files);

				auto vmap_router = vm->set_class<network::http::map_router>("map_router", true);
				auto vrouter_entry = vm->set_class<network::http::router_entry>("route_entry", false);
				vrouter_entry->set_property<network::http::router_entry>("route_auth auths", &network::http::router_entry::auth);
				vrouter_entry->set_property<network::http::router_entry>("route_compression compressions", &network::http::router_entry::compression);
				vrouter_entry->set_property<network::http::router_entry>("string files_directory", &network::http::router_entry::files_directory);
				vrouter_entry->set_property<network::http::router_entry>("string char_set", &network::http::router_entry::char_set);
				vrouter_entry->set_property<network::http::router_entry>("string proxy_ip_address", &network::http::router_entry::proxy_ip_address);
				vrouter_entry->set_property<network::http::router_entry>("string access_control_allow_origin", &network::http::router_entry::access_control_allow_origin);
				vrouter_entry->set_property<network::http::router_entry>("string redirect", &network::http::router_entry::redirect);
				vrouter_entry->set_property<network::http::router_entry>("string alias", &network::http::router_entry::alias);
				vrouter_entry->set_property<network::http::router_entry>("usize websocket_timeout", &network::http::router_entry::web_socket_timeout);
				vrouter_entry->set_property<network::http::router_entry>("usize static_file_max_age", &network::http::router_entry::static_file_max_age);
				vrouter_entry->set_property<network::http::router_entry>("usize level", &network::http::router_entry::level);
				vrouter_entry->set_property<network::http::router_entry>("bool allow_directory_listing", &network::http::router_entry::allow_directory_listing);
				vrouter_entry->set_property<network::http::router_entry>("bool allow_websocket", &network::http::router_entry::allow_web_socket);
				vrouter_entry->set_property<network::http::router_entry>("bool allow_send_file", &network::http::router_entry::allow_send_file);
				vrouter_entry->set_property<network::http::router_entry>("regex_source location", &network::http::router_entry::location);
				vrouter_entry->set_constructor<network::http::router_entry>("route_entry@ f()");
				vrouter_entry->set_method_extern("void set_hidden_files(array<regex_source>@+)", &router_entry_set_hidden_files);
				vrouter_entry->set_method_extern("array<regex_source>@ get_hidden_files() const", &router_entry_get_hidden_files);
				vrouter_entry->set_method_extern("void set_error_files(array<error_file>@+)", &router_entry_set_error_files);
				vrouter_entry->set_method_extern("array<error_file>@ get_error_files() const", &router_entry_get_error_files);
				vrouter_entry->set_method_extern("void set_mime_types(array<mime_type>@+)", &router_entry_set_mime_types);
				vrouter_entry->set_method_extern("array<mime_type>@ get_mime_types() const", &router_entry_get_mime_types);
				vrouter_entry->set_method_extern("void set_index_files(array<string>@+)", &router_entry_set_index_files);
				vrouter_entry->set_method_extern("array<string>@ get_index_files() const", &router_entry_get_index_files);
				vrouter_entry->set_method_extern("void set_try_files(array<string>@+)", &router_entry_set_try_files);
				vrouter_entry->set_method_extern("array<string>@ get_try_files() const", &router_entry_get_try_files);
				vrouter_entry->set_method_extern("void set_disallowed_methods_files(array<string>@+)", &router_entry_set_disallowed_methods_files);
				vrouter_entry->set_method_extern("array<string>@ get_disallowed_methods_files() const", &router_entry_get_disallowed_methods_files);
				vrouter_entry->set_method_extern("map_router@+ get_router() const", &router_entry_get_router);

				auto vrouter_group = vm->set_class<network::http::router_group>("route_group", false);
				vrouter_group->set_property<network::http::router_group>("string match", &network::http::router_group::match);
				vrouter_group->set_property<network::http::router_group>("route_mode mode", &network::http::router_group::mode);
				vrouter_group->set_gc_constructor<network::http::router_group, router_group, const std::string_view&, network::http::route_mode>("route_group@ f(const string_view&in, route_mode)");
				vrouter_group->set_method_extern("route_entry@+ get_route(usize) const", &router_group_get_route);
				vrouter_group->set_method_extern("usize get_routes_size() const", &router_group_get_routes_size);

				auto vrouter_cookie = vm->set_struct_trivial<network::http::map_router::router_session::router_cookie>("router_cookie");
				vrouter_cookie->set_property<network::http::map_router::router_session::router_cookie>("string name", &network::http::map_router::router_session::router_cookie::name);
				vrouter_cookie->set_property<network::http::map_router::router_session::router_cookie>("string domain", &network::http::map_router::router_session::router_cookie::domain);
				vrouter_cookie->set_property<network::http::map_router::router_session::router_cookie>("string path", &network::http::map_router::router_session::router_cookie::path);
				vrouter_cookie->set_property<network::http::map_router::router_session::router_cookie>("string same_site", &network::http::map_router::router_session::router_cookie::same_site);
				vrouter_cookie->set_property<network::http::map_router::router_session::router_cookie>("uint64 expires", &network::http::map_router::router_session::router_cookie::expires);
				vrouter_cookie->set_property<network::http::map_router::router_session::router_cookie>("bool secure", &network::http::map_router::router_session::router_cookie::secure);
				vrouter_cookie->set_property<network::http::map_router::router_session::router_cookie>("bool http_only", &network::http::map_router::router_session::router_cookie::http_only);
				vrouter_cookie->set_constructor<network::http::map_router::router_session::router_cookie>("void f()");

				auto vrouter_session = vm->set_struct_trivial<network::http::map_router::router_session>("router_session");
				vrouter_session->set_property<network::http::map_router::router_session>("router_cookie cookie", &network::http::map_router::router_session::cookie);
				vrouter_session->set_property<network::http::map_router::router_session>("string directory", &network::http::map_router::router_session::directory);
				vrouter_session->set_property<network::http::map_router::router_session>("uint64 expires", &network::http::map_router::router_session::expires);
				vrouter_session->set_constructor<network::http::map_router::router_session>("void f()");

				auto vconnection = vm->set_class<network::http::connection>("connection", false);
				auto vweb_socket_frame = vm->set_class<network::http::web_socket_frame>("websocket_frame", false);
				vweb_socket_frame->set_function_def("void status_async(websocket_frame@+)");
				vweb_socket_frame->set_function_def("void recv_async(websocket_frame@+, websocket_op, const string&in)");
				vweb_socket_frame->set_method_extern("bool set_on_connect(status_async@+)", &web_socket_frame_set_on_connect);
				vweb_socket_frame->set_method_extern("bool set_on_before_disconnect(status_async@+)", &web_socket_frame_set_on_before_disconnect);
				vweb_socket_frame->set_method_extern("bool set_on_disconnect(status_async@+)", &web_socket_frame_set_on_disconnect);
				vweb_socket_frame->set_method_extern("bool set_on_receive(recv_async@+)", &web_socket_frame_set_on_receive);
				vweb_socket_frame->set_method_extern("promise<bool>@ send(const string_view&in, websocket_op)", &VI_SPROMISIFY(web_socket_frame_send1, type_id::bool_t));
				vweb_socket_frame->set_method_extern("promise<bool>@ send(uint32, const string_view&in, websocket_op)", &VI_SPROMISIFY(web_socket_frame_send2, type_id::bool_t));
				vweb_socket_frame->set_method_extern("promise<bool>@ send_close()", &VI_SPROMISIFY(web_socket_frame_send_close, type_id::bool_t));
				vweb_socket_frame->set_method("void next()", &network::http::web_socket_frame::next);
				vweb_socket_frame->set_method("bool is_finished() const", &network::http::web_socket_frame::is_finished);
				vweb_socket_frame->set_method("socket@+ get_stream() const", &network::http::web_socket_frame::get_stream);
				vweb_socket_frame->set_method("connection@+ get_connection() const", &network::http::web_socket_frame::get_connection);

				vmap_router->set_property<network::socket_router>("usize max_heap_buffer", &network::socket_router::max_heap_buffer);
				vmap_router->set_property<network::socket_router>("usize max_net_buffer", &network::socket_router::max_net_buffer);
				vmap_router->set_property<network::socket_router>("usize backlog_queue", &network::socket_router::backlog_queue);
				vmap_router->set_property<network::socket_router>("usize socket_timeout", &network::socket_router::socket_timeout);
				vmap_router->set_property<network::socket_router>("usize max_connections", &network::socket_router::max_connections);
				vmap_router->set_property<network::socket_router>("int64 keep_alive_max_count", &network::socket_router::keep_alive_max_count);
				vmap_router->set_property<network::socket_router>("int64 graceful_time_wait", &network::socket_router::graceful_time_wait);
				vmap_router->set_property<network::socket_router>("bool enable_no_delay", &network::socket_router::enable_no_delay);
				vmap_router->set_property<network::http::map_router>("router_session session", &network::http::map_router::session);
				vmap_router->set_property<network::http::map_router>("string temporary_directory", &network::http::map_router::temporary_directory);
				vmap_router->set_property<network::http::map_router>("usize max_uploadable_resources", &network::http::map_router::max_uploadable_resources);
				vmap_router->set_gc_constructor<network::http::map_router, map_router>("map_router@ f()");
				vmap_router->set_method_extern("void listen(const string_view&in, const string_view&in, bool = false)", &socket_router_listen1);
				vmap_router->set_method_extern("void listen(const string_view&in, const string_view&in, const string_view&in, bool = false)", &socket_router_listen2);
				vmap_router->set_method_extern("void set_listeners(dictionary@ data)", &socket_router_set_listeners);
				vmap_router->set_method_extern("dictionary@ get_listeners() const", &socket_router_get_listeners);
				vmap_router->set_method_extern("void set_certificates(dictionary@ data)", &socket_router_set_certificates);
				vmap_router->set_method_extern("dictionary@ get_certificates() const", &socket_router_get_certificates);
				vmap_router->set_function_def("void route_async(connection@+)");
				vmap_router->set_function_def("void authorize_sync(connection@+, credentials&in)");
				vmap_router->set_function_def("void headers_sync(connection@+, string&out)");
				vmap_router->set_function_def("void websocket_status_async(websocket_frame@+)");
				vmap_router->set_function_def("void websocket_recv_async(websocket_frame@+, websocket_op, const string&in)");
				vmap_router->set_method("void sort()", &network::http::map_router::sort);
				vmap_router->set_method("route_group@+ group(const string_view&in, route_mode)", &network::http::map_router::group);
				vmap_router->set_method<network::http::map_router, network::http::router_entry*, const std::string_view&, network::http::route_mode, const std::string_view&, bool>("route_entry@+ route(const string_view&in, route_mode, const string_view&in, bool)", &network::http::map_router::route);
				vmap_router->set_method<network::http::map_router, network::http::router_entry*, const std::string_view&, network::http::router_group*, network::http::router_entry*>("route_entry@+ route(const string_view&in, route_group@+, route_entry@+)", &network::http::map_router::route);
				vmap_router->set_method("bool remove(route_entry@+)", &network::http::map_router::remove);
				vmap_router->set_method_extern("bool get(const string_view&in, route_async@) const", &map_router_get1);
				vmap_router->set_method_extern("bool get(const string_view&in, route_mode, const string_view&in, route_async@) const", &map_router_get2);
				vmap_router->set_method_extern("bool post(const string_view&in, route_async@) const", &map_router_post1);
				vmap_router->set_method_extern("bool post(const string_view&in, route_mode, const string_view&in, route_async@) const", &map_router_post2);
				vmap_router->set_method_extern("bool patch(const string_view&in, route_async@) const", &map_router_patch1);
				vmap_router->set_method_extern("bool patch(const string_view&in, route_mode, const string_view&in, route_async@) const", &map_router_patch2);
				vmap_router->set_method_extern("bool delete(const string_view&in, route_async@) const", &map_router_delete1);
				vmap_router->set_method_extern("bool delete(const string_view&in, route_mode, const string_view&in, route_async@) const", &map_router_delete2);
				vmap_router->set_method_extern("bool options(const string_view&in, route_async@) const", &map_router_options1);
				vmap_router->set_method_extern("bool options(const string_view&in, route_mode, const string_view&in, route_async@) const", &map_router_options2);
				vmap_router->set_method_extern("bool access(const string_view&in, route_async@) const", &map_router_access1);
				vmap_router->set_method_extern("bool access(const string_view&in, route_mode, const string_view&in, route_async@) const", &map_router_access2);
				vmap_router->set_method_extern("bool headers(const string_view&in, headers_sync@) const", &map_router_headers1);
				vmap_router->set_method_extern("bool headers(const string_view&in, route_mode, const string_view&in, headers_sync@) const", &map_router_headers2);
				vmap_router->set_method_extern("bool authorize(const string_view&in, authorize_sync@) const", &map_router_authorize1);
				vmap_router->set_method_extern("bool authorize(const string_view&in, route_mode, const string_view&in, authorize_sync@) const", &map_router_authorize2);
				vmap_router->set_method_extern("bool websocket_initiate(const string_view&in, route_async@) const", &map_router_websocket_initiate1);
				vmap_router->set_method_extern("bool websocket_initiate(const string_view&in, route_mode, const string_view&in, route_async@) const", &map_router_websocket_initiate2);
				vmap_router->set_method_extern("bool websocket_connect(const string_view&in, websocket_status_async@) const", &map_router_websocket_connect1);
				vmap_router->set_method_extern("bool websocket_connect(const string_view&in, route_mode, const string_view&in, websocket_status_async@) const", &map_router_websocket_connect2);
				vmap_router->set_method_extern("bool websocket_disconnect(const string_view&in, websocket_status_async@) const", &map_router_websocket_disconnect1);
				vmap_router->set_method_extern("bool websocket_disconnect(const string_view&in, route_mode, const string_view&in, websocket_status_async@) const", &map_router_websocket_disconnect2);
				vmap_router->set_method_extern("bool websocket_receive(const string_view&in, websocket_recv_async@) const", &map_router_websocket_receive1);
				vmap_router->set_method_extern("bool websocket_receive(const string_view&in, route_mode, const string_view&in, websocket_recv_async@) const", &map_router_websocket_receive2);
				vmap_router->set_method_extern("route_entry@+ get_base() const", &map_router_get_base);
				vmap_router->set_method_extern("route_group@+ get_group(usize) const", &map_router_get_group);
				vmap_router->set_method_extern("usize get_groups_size() const", &map_router_get_groups_size);
				vmap_router->set_enum_refs_extern<network::http::map_router>([](network::http::map_router* base, asIScriptEngine* vm)
				{
					function_factory::gc_enum_callback(vm, base->base);
					for (auto& group : base->groups)
					{
						for (auto& route : group->routes)
							function_factory::gc_enum_callback(vm, route);
					}
				});
				vmap_router->set_release_refs_extern<network::http::map_router>([](network::http::map_router* base, asIScriptEngine*)
				{
					base->~map_router();
				});
				vmap_router->set_dynamic_cast<network::http::map_router, network::socket_router>("socket_router@+", true);

				auto vserver = vm->set_class<network::http::server>("server", true);
				vconnection->set_property<network::http::connection>("file_entry target", &network::http::connection::resource);
				vconnection->set_property<network::http::connection>("request_frame request", &network::http::connection::request);
				vconnection->set_property<network::http::connection>("response_frame response", &network::http::connection::response);
				vconnection->set_property<network::socket_connection>("socket@ stream", &network::socket_connection::stream);
				vconnection->set_property<network::socket_connection>("socket_listener@ host", &network::socket_connection::host);
				vconnection->set_property<network::socket_connection>("socket_address address", &network::socket_connection::address);
				vconnection->set_property<network::socket_connection>("socket_data_frame info", &network::socket_connection::info);
				vconnection->set_method<network::http::connection, bool>("bool next()", &network::http::connection::next);
				vconnection->set_method<network::http::connection, bool, int>("bool next(int32)", &network::http::connection::next);
				vconnection->set_method<network::socket_connection, bool>("bool abort()", &network::socket_connection::abort);
				vconnection->set_method_extern("bool abort(int32, const string_view&in)", &socket_connection_abort);
				vconnection->set_method("void reset(bool)", &network::http::connection::reset);
				vconnection->set_method("bool is_skip_required() const", &network::http::connection::is_skip_required);
				vconnection->set_method_extern("promise<bool>@ send_headers(int32, bool = true) const", &VI_SPROMISIFY(connection_send_headers, type_id::bool_t));
				vconnection->set_method_extern("promise<bool>@ send_chunk(const string_view&in) const", &VI_SPROMISIFY(connection_send_chunk, type_id::bool_t));
				vconnection->set_method_extern("promise<array<resource_info>@>@ store(bool = false) const", &VI_SPROMISIFY_REF(connection_store, array_resource_info));
				vconnection->set_method_extern("promise<string>@ fetch(bool = false) const", &VI_SPROMISIFY_REF(connection_fetch, string));
				vconnection->set_method_extern("promise<bool>@ skip() const", &VI_SPROMISIFY(connection_skip, type_id::bool_t));
				vconnection->set_method_extern("string get_peer_ip_address() const", &connection_get_peer_ip_address);
				vconnection->set_method_extern("websocket_frame@+ get_websocket() const", &connection_get_web_socket);
				vconnection->set_method_extern("route_entry@+ get_route() const", &connection_get_route);
				vconnection->set_method_extern("server@+ get_root() const", &connection_get_server);

				auto vquery = vm->set_class<network::http::query>("query", false);
				vquery->set_constructor<network::http::query>("query@ f()");
				vquery->set_method("void clear()", &network::http::query::clear);
				vquery->set_method_extern("void decode(const string_view&in, const string_view&in)", &query_decode);
				vquery->set_method_extern("string encode(const string_view&in)", &query_encode);
				vquery->set_method_extern("void set_data(schema@+)", &query_set_data);
				vquery->set_method_extern("schema@+ get_data()", &query_get_data);

				auto vsession = vm->set_class<network::http::session>("session", false);
				vsession->set_property<network::http::session>("string session_id", &network::http::session::session_id);
				vsession->set_property<network::http::session>("int64 session_expires", &network::http::session::session_expires);
				vsession->set_constructor<network::http::session>("session@ f()");
				vsession->set_method("void clear()", &network::http::session::clear);
				vsession->set_method_extern("bool write(connection@+)", &VI_EXPECTIFY_VOID(network::http::session::write));
				vsession->set_method_extern("bool read(connection@+)", &VI_EXPECTIFY_VOID(network::http::session::read));
				vsession->set_method_static("bool invalidate_cache(const string_view&in)", &VI_SEXPECTIFY_VOID(network::http::session::invalidate_cache));
				vsession->set_method_extern("void set_data(schema@+)", &session_set_data);
				vsession->set_method_extern("schema@+ get_data()", &session_get_data);

				vserver->set_gc_constructor<network::http::server, server>("server@ f()");
				vserver->set_method("void update()", &network::http::server::update);
				vserver->set_method_extern("void set_router(map_router@+)", &socket_server_set_router);
				vserver->set_method_extern("bool configure(map_router@+)", &socket_server_configure);
				vserver->set_method_extern("bool listen()", &socket_server_listen);
				vserver->set_method_extern("bool unlisten(bool)", &socket_server_unlisten);
				vserver->set_method("void set_backlog(usize)", &network::socket_server::set_backlog);
				vserver->set_method("usize get_backlog() const", &network::socket_server::get_backlog);
				vserver->set_method("server_state get_state() const", &network::socket_server::get_state);
				vserver->set_method("map_router@+ get_router() const", &network::socket_server::get_router);
				vserver->set_enum_refs_extern<network::http::server>([](network::http::server* base, asIScriptEngine* vm)
				{
					function_factory::gc_enum_callback(vm, base->get_router());
					for (auto* item : base->get_active_clients())
						function_factory::gc_enum_callback(vm, item);

					for (auto* item : base->get_pooled_clients())
						function_factory::gc_enum_callback(vm, item);
				});
				vserver->set_release_refs_extern<network::http::server>([](network::http::server* base, asIScriptEngine*)
				{
					base->~server();
				});
				vserver->set_dynamic_cast<network::http::server, network::socket_server>("socket_server@+", true);

				auto vclient = vm->set_class<network::http::client>("client", false);
				vclient->set_constructor<network::http::client, int64_t>("client@ f(int64)");
				vclient->set_method("bool downgrade()", &network::http::client::downgrade);
				vclient->set_method_extern("promise<schema@>@ json(const request_frame&in, usize = 65536)", &VI_SPROMISIFY_REF(client_json, schema));
				vclient->set_method_extern("promise<schema@>@ xml(const request_frame&in, usize = 65536)", &VI_SPROMISIFY_REF(client_xml, schema));
				vclient->set_method_extern("promise<bool>@ skip()", &VI_SPROMISIFY(client_skip, type_id::bool_t));
				vclient->set_method_extern("promise<bool>@ fetch(usize = 65536, bool = false)", &VI_SPROMISIFY(client_fetch, type_id::bool_t));
				vclient->set_method_extern("promise<bool>@ upgrade(const request_frame&in)", &VI_SPROMISIFY(client_upgrade, type_id::bool_t));
				vclient->set_method_extern("promise<bool>@ send(const request_frame&in)", &VI_SPROMISIFY(client_send, type_id::bool_t));
				vclient->set_method_extern("promise<bool>@ send_fetch(const request_frame&in, usize = 65536)", &VI_SPROMISIFY(client_send_fetch, type_id::bool_t));
				vclient->set_method_extern("promise<bool>@ connect_sync(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(socket_client_connect_sync, type_id::bool_t));
				vclient->set_method_extern("promise<bool>@ connect_async(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(socket_client_connect_async, type_id::bool_t));
				vclient->set_method_extern("promise<bool>@ disconnect()", &VI_SPROMISIFY(socket_client_disconnect, type_id::bool_t));
				vclient->set_method("bool has_stream() const", &network::socket_client::has_stream);
				vclient->set_method("bool is_secure() const", &network::socket_client::is_secure);
				vclient->set_method("bool is_verified() const", &network::socket_client::is_verified);
				vclient->set_method("void apply_reusability(bool)", &network::socket_client::apply_reusability);
				vclient->set_method("const socket_address& get_peer_address() const", &network::socket_client::get_peer_address);
				vclient->set_method("socket@+ get_stream() const", &network::socket_client::get_stream);
				vclient->set_method("websocket_frame@+ get_websocket() const", &network::http::client::get_web_socket);
				vclient->set_method("request_frame& get_request() property", &network::http::client::get_request);
				vclient->set_method("response_frame& get_response() property", &network::http::client::get_response);
				vclient->set_dynamic_cast<network::http::client, network::socket_client>("socket_client@+", true);

				auto vhrm_cache = vm->set_class<network::http::hrm_cache>("hrm_cache", false);
				vhrm_cache->set_constructor<network::http::hrm_cache>("hrm_cache@ f()");
				vhrm_cache->set_constructor<network::http::hrm_cache, size_t>("hrm_cache@ f(usize)");
				vhrm_cache->set_method("void rescale(usize)", &network::http::hrm_cache::rescale);
				vhrm_cache->set_method("void shrink()", &network::http::hrm_cache::shrink);
				vhrm_cache->set_method_static("hrm_cache@+ get()", &network::http::hrm_cache::get);

				vm->set_function("promise<response_frame>@ fetch(const string_view&in, const string_view&in = \"GET\", const fetch_frame&in = fetch_frame())", &VI_SPROMISIFY_REF(http_fetch, response_frame));
				vm->end_namespace();

				return true;
#else
				VI_ASSERT(false, "<http> is not loaded");
				return false;
#endif
			}
			bool registry::import_smtp(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");

				vm->begin_namespace("smtp");
				auto vpriority = vm->set_enum("priority");
				vpriority->set_value("high", (int)network::smtp::priority::high);
				vpriority->set_value("normal", (int)network::smtp::priority::normal);
				vpriority->set_value("low", (int)network::smtp::priority::low);

				auto vrecipient = vm->set_struct_trivial<network::smtp::recipient>("recipient");
				vrecipient->set_property<network::smtp::recipient>("string name", &network::smtp::recipient::name);
				vrecipient->set_property<network::smtp::recipient>("string address", &network::smtp::recipient::address);
				vrecipient->set_constructor<network::smtp::recipient>("void f()");

				auto vattachment = vm->set_struct_trivial<network::smtp::attachment>("attachment");
				vattachment->set_property<network::smtp::attachment>("string path", &network::smtp::attachment::path);
				vattachment->set_property<network::smtp::attachment>("usize length", &network::smtp::attachment::length);
				vattachment->set_constructor<network::smtp::attachment>("void f()");

				auto vrequest_frame = vm->set_struct_trivial<network::smtp::request_frame>("request_frame");
				vrequest_frame->set_property<network::smtp::request_frame>("string sender_name", &network::smtp::request_frame::sender_name);
				vrequest_frame->set_property<network::smtp::request_frame>("string sender_address", &network::smtp::request_frame::sender_address);
				vrequest_frame->set_property<network::smtp::request_frame>("string subject", &network::smtp::request_frame::subject);
				vrequest_frame->set_property<network::smtp::request_frame>("string charset", &network::smtp::request_frame::charset);
				vrequest_frame->set_property<network::smtp::request_frame>("string mailer", &network::smtp::request_frame::mailer);
				vrequest_frame->set_property<network::smtp::request_frame>("string receiver", &network::smtp::request_frame::receiver);
				vrequest_frame->set_property<network::smtp::request_frame>("string login", &network::smtp::request_frame::login);
				vrequest_frame->set_property<network::smtp::request_frame>("string password", &network::smtp::request_frame::password);
				vrequest_frame->set_property<network::smtp::request_frame>("priority prior", &network::smtp::request_frame::prior);
				vrequest_frame->set_property<network::smtp::request_frame>("bool authenticate", &network::smtp::request_frame::authenticate);
				vrequest_frame->set_property<network::smtp::request_frame>("bool no_notification", &network::smtp::request_frame::no_notification);
				vrequest_frame->set_property<network::smtp::request_frame>("bool allow_html", &network::smtp::request_frame::allow_html);
				vrequest_frame->set_constructor<network::smtp::request_frame>("void f()");
				vrequest_frame->set_method_extern("void set_header(const string_view&in, const string_view&in)", &smtp_request_set_header);
				vrequest_frame->set_method_extern("string get_header(const string_view&in)", &smtp_request_get_header);
				vrequest_frame->set_method_extern("void set_recipients(array<recipient>@+)", &smtp_request_set_recipients);
				vrequest_frame->set_method_extern("array<string>@ get_recipients() const", &smtp_request_get_recipients);
				vrequest_frame->set_method_extern("void set_cc_recipients(array<recipient>@+)", &smtp_request_set_cc_recipients);
				vrequest_frame->set_method_extern("array<string>@ get_cc_recipients() const", &smtp_request_get_cc_recipients);
				vrequest_frame->set_method_extern("void set_bcc_recipients(array<recipient>@+)", &smtp_request_set_bcc_recipients);
				vrequest_frame->set_method_extern("array<string>@ get_bcc_recipients() const", &smtp_request_get_bcc_recipients);
				vrequest_frame->set_method_extern("void set_attachments(array<attachment>@+)", &smtp_request_set_attachments);
				vrequest_frame->set_method_extern("array<attachment>@ get_attachments() const", &smtp_request_get_attachments);
				vrequest_frame->set_method_extern("void set_messages(array<string>@+)", &smtp_request_set_messages);
				vrequest_frame->set_method_extern("array<string>@ get_messages() const", &smtp_request_get_messages);

				auto vclient = vm->set_class<network::smtp::client>("client", false);
				vclient->set_constructor<network::smtp::client, const std::string_view&, int64_t>("client@ f(const string_view&in, int64)");
				vclient->set_method_extern("promise<bool>@ send(const request_frame&in)", &VI_SPROMISIFY(smtp_client_send, type_id::bool_t));
				vclient->set_method_extern("promise<bool>@ connect_sync(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(socket_client_connect_sync, type_id::bool_t));
				vclient->set_method_extern("promise<bool>@ connect_async(const socket_address&in, int32 = -1)", &VI_SPROMISIFY(socket_client_connect_async, type_id::bool_t));
				vclient->set_method_extern("promise<bool>@ disconnect()", &VI_SPROMISIFY(socket_client_disconnect, type_id::bool_t));
				vclient->set_method("bool has_stream() const", &network::socket_client::has_stream);
				vclient->set_method("bool is_secure() const", &network::socket_client::is_secure);
				vclient->set_method("bool is_verified() const", &network::socket_client::is_verified);
				vclient->set_method("void apply_reusability(bool)", &network::socket_client::apply_reusability);
				vclient->set_method("const socket_address& get_peer_address() const", &network::socket_client::get_peer_address);
				vclient->set_method("socket@+ get_stream() const", &network::socket_client::get_stream);
				vclient->set_method("request_frame& get_request() property", &network::smtp::client::get_request);
				vclient->set_dynamic_cast<network::smtp::client, network::socket_client>("socket_client@+", true);
				vm->end_namespace();

				return true;
#else
				VI_ASSERT(false, "<smtp> is not loaded");
				return false;
#endif
			}
			bool registry::import_sqlite(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				VI_TYPEREF(cursor, "sqlite::cursor");
				VI_TYPEREF(connection, "uptr");

				vm->begin_namespace("sqlite");
				auto visolation = vm->set_enum("isolation");
				visolation->set_value("deferred", (int)network::sqlite::isolation::deferred);
				visolation->set_value("immediate", (int)network::sqlite::isolation::immediate);
				visolation->set_value("exclusive", (int)network::sqlite::isolation::exclusive);
				visolation->set_value("default_isolation", (int)network::sqlite::isolation::default_isolation);

				auto vquery_op = vm->set_enum("query_op");
				vquery_op->set_value("transaction_start", (int)network::sqlite::query_op::transaction_start);
				vquery_op->set_value("transaction_end", (int)network::sqlite::query_op::transaction_end);

				auto vcheckpoint_mode = vm->set_enum("checkpoint_mode");
				vcheckpoint_mode->set_value("passive", (int)network::sqlite::checkpoint_mode::passive);
				vcheckpoint_mode->set_value("full", (int)network::sqlite::checkpoint_mode::full);
				vcheckpoint_mode->set_value("restart", (int)network::sqlite::checkpoint_mode::restart);
				vcheckpoint_mode->set_value("truncate", (int)network::sqlite::checkpoint_mode::truncate);

				auto vcheckpoint = vm->set_struct_trivial<network::sqlite::checkpoint>("checkpoint");
				vcheckpoint->set_property<network::sqlite::checkpoint>("string database", &network::sqlite::checkpoint::database);
				vcheckpoint->set_property<network::sqlite::checkpoint>("uint32 frames_size", &network::sqlite::checkpoint::frames_size);
				vcheckpoint->set_property<network::sqlite::checkpoint>("uint32 frames_count", &network::sqlite::checkpoint::frames_count);
				vcheckpoint->set_property<network::sqlite::checkpoint>("int32 status", &network::sqlite::checkpoint::status);
				vcheckpoint->set_constructor<network::sqlite::checkpoint>("void f()");

				auto vrow = vm->set_struct_trivial<network::sqlite::row>("row", (size_t)object_behaviours::app_class_allints);
				auto vcolumn = vm->set_struct_trivial<network::sqlite::column>("column");
				vcolumn->set_constructor<network::sqlite::column, const network::sqlite::column&>("void f(const column&in)");
				vcolumn->set_method("string get_name() const", &network::sqlite::column::get_name);
				vcolumn->set_method("variant get() const", &network::sqlite::column::get);
				vcolumn->set_method("schema@ get_inline() const", &network::sqlite::column::get_inline);
				vcolumn->set_method("usize index() const", &network::sqlite::column::index);
				vcolumn->set_method("usize size() const", &network::sqlite::column::size);
				vcolumn->set_method("row get_row() const", &network::sqlite::column::get_row);
				vcolumn->set_method("bool nullable() const", &network::sqlite::column::nullable);
				vcolumn->set_method("bool exists() const", &network::sqlite::column::exists);

				auto vresponse = vm->set_struct<network::sqlite::response>("response");
				vrow->set_constructor<network::sqlite::row, const network::sqlite::row&>("void f(const row&in)");
				vrow->set_method("schema@ get_object() const", &network::sqlite::row::get_object);
				vrow->set_method("schema@ get_array() const", &network::sqlite::row::get_array);
				vrow->set_method("usize index() const", &network::sqlite::row::index);
				vrow->set_method("usize size() const", &network::sqlite::row::size);
				vrow->set_method<network::sqlite::row, network::sqlite::column, size_t>("column get_column(usize) const", &network::sqlite::row::get_column);
				vrow->set_method<network::sqlite::row, network::sqlite::column, const std::string_view&>("column get_column(const string_view&in) const", &network::sqlite::row::get_column);
				vrow->set_method("bool exists() const", &network::sqlite::row::exists);
				vrow->set_method<network::sqlite::row, network::sqlite::column, size_t>("column opIndex(usize)", &network::sqlite::row::get_column);
				vrow->set_method<network::sqlite::row, network::sqlite::column, size_t>("column opIndex(usize) const", &network::sqlite::row::get_column);
				vrow->set_method<network::sqlite::row, network::sqlite::column, const std::string_view&>("column opIndex(const string_view&in)", &network::sqlite::row::get_column);
				vrow->set_method<network::sqlite::row, network::sqlite::column, const std::string_view&>("column opIndex(const string_view&in) const", &network::sqlite::row::get_column);

				vresponse->set_constructor<network::sqlite::response>("void f()");
				vresponse->set_operator_copy_static(&ldb_response_copy);
				vresponse->set_destructor<network::sqlite::response>("void f()");
				vresponse->set_method<network::sqlite::response, network::sqlite::row, size_t>("row opIndex(usize)", &network::sqlite::response::get_row);
				vresponse->set_method<network::sqlite::response, network::sqlite::row, size_t>("row opIndex(usize) const", &network::sqlite::response::get_row);
				vresponse->set_method("schema@ get_array_of_objects() const", &network::sqlite::response::get_array_of_objects);
				vresponse->set_method("schema@ get_array_of_arrays() const", &network::sqlite::response::get_array_of_arrays);
				vresponse->set_method("schema@ get_object(usize = 0) const", &network::sqlite::response::get_object);
				vresponse->set_method("schema@ get_array(usize = 0) const", &network::sqlite::response::get_array);
				vresponse->set_method_extern("array<string>@ get_columns() const", &ldb_response_get_columns);
				vresponse->set_method("string get_status_text() const", &network::sqlite::response::get_status_text);
				vresponse->set_method("int32 get_status_code() const", &network::sqlite::response::get_status_code);
				vresponse->set_method("usize affected_rows() const", &network::sqlite::response::affected_rows);
				vresponse->set_method("usize last_inserted_row_id() const", &network::sqlite::response::last_inserted_row_id);
				vresponse->set_method("usize size() const", &network::sqlite::response::size);
				vresponse->set_method("row get_row(usize) const", &network::sqlite::response::get_row);
				vresponse->set_method("row front() const", &network::sqlite::response::front);
				vresponse->set_method("row back() const", &network::sqlite::response::back);
				vresponse->set_method("response copy() const", &network::sqlite::response::copy);
				vresponse->set_method("bool empty() const", &network::sqlite::response::empty);
				vresponse->set_method("bool error() const", &network::sqlite::response::error);
				vresponse->set_method("bool error_or_empty() const", &network::sqlite::response::error_or_empty);
				vresponse->set_method("bool exists() const", &network::sqlite::response::exists);

				auto vcursor = vm->set_struct<network::sqlite::cursor>("cursor");
				vcursor->set_constructor<network::sqlite::cursor>("void f()");
				vcursor->set_operator_copy_static(&ldb_cursor_copy);
				vcursor->set_destructor<network::sqlite::cursor>("void f()");
				vcursor->set_method("column opIndex(const string_view&in)", &network::sqlite::cursor::get_column);
				vcursor->set_method("column opIndex(const string_view&in) const", &network::sqlite::cursor::get_column);
				vcursor->set_method("bool success() const", &network::sqlite::cursor::success);
				vcursor->set_method("bool empty() const", &network::sqlite::cursor::empty);
				vcursor->set_method("bool error() const", &network::sqlite::cursor::error);
				vcursor->set_method("bool error_or_empty() const", &network::sqlite::cursor::error_or_empty);
				vcursor->set_method("usize size() const", &network::sqlite::cursor::size);
				vcursor->set_method("usize affected_rows() const", &network::sqlite::cursor::affected_rows);
				vcursor->set_method("cursor copy() const", &network::sqlite::cursor::copy);
				vcursor->set_method_extern("response first() const", &ldb_cursor_first);
				vcursor->set_method_extern("response last() const", &ldb_cursor_last);
				vcursor->set_method_extern("response at(usize) const", &ldb_cursor_at);
				vcursor->set_method("uptr@ get_executor() const", &network::sqlite::cursor::get_executor);
				vcursor->set_method("schema@ get_array_of_objects(usize = 0) const", &network::sqlite::cursor::get_array_of_objects);
				vcursor->set_method("schema@ get_array_of_arrays(usize = 0) const", &network::sqlite::cursor::get_array_of_arrays);
				vcursor->set_method("schema@ get_object(usize = 0, usize = 0) const", &network::sqlite::cursor::get_object);
				vcursor->set_method("schema@ get_array(usize = 0, usize = 0) const", &network::sqlite::cursor::get_array);

				auto vcluster = vm->set_class<network::sqlite::cluster>("cluster", false);
				vcluster->set_constructor<network::sqlite::cluster>("cluster@ f()");
				vcluster->set_function_def("variant constant_sync(array<variant>@+)");
				vcluster->set_function_def("variant finalize_sync()");
				vcluster->set_function_def("variant value_sync()");
				vcluster->set_function_def("void step_sync(array<variant>@+)");
				vcluster->set_function_def("void inverse_sync(array<variant>@+)");
				vcluster->set_method("void set_wal_autocheckpoint(uint32)", &network::sqlite::cluster::set_wal_autocheckpoint);
				vcluster->set_method("void set_soft_heap_limit(uint64)", &network::sqlite::cluster::set_soft_heap_limit);
				vcluster->set_method("void set_hard_heap_limit(uint64)", &network::sqlite::cluster::set_hard_heap_limit);
				vcluster->set_method("void set_shared_cache(bool)", &network::sqlite::cluster::set_shared_cache);
				vcluster->set_method("void set_extensions(bool)", &network::sqlite::cluster::set_extensions);
				vcluster->set_method("void overload_function(const string_view&in, uint8)", &network::sqlite::cluster::overload_function);
				vcluster->set_method_extern("array<checkpoint>@ wal_checkpoint(checkpoint_mode, const string_view&in = string_view())", &ldb_cluster_wal_checkpoint);
				vcluster->set_method("usize free_memory_used(usize)", &network::sqlite::cluster::free_memory_used);
				vcluster->set_method("usize get_memory_used()", &network::sqlite::cluster::get_memory_used);
				vcluster->set_method("uptr@ get_idle_connection()", &network::sqlite::cluster::get_idle_connection);
				vcluster->set_method("uptr@ get_busy_connection()", &network::sqlite::cluster::get_busy_connection);
				vcluster->set_method("uptr@ get_any_connection()", &network::sqlite::cluster::get_any_connection);
				vcluster->set_method("const string& get_address() const", &network::sqlite::cluster::get_address);
				vcluster->set_method("bool is_connected() const", &network::sqlite::cluster::is_connected);
				vcluster->set_method_extern("void set_function(const string_view&in, uint8, constant_sync@)", &ldb_cluster_set_function);
				vcluster->set_method_extern("void set_aggregate_function(const string_view&in, uint8, step_sync@, finalize_sync@)", &ldb_cluster_set_aggregate_function);
				vcluster->set_method_extern("void set_window_function(const string_view&in, uint8, step_sync@, inverse_sync@, value_sync@, finalize_sync@)", &ldb_cluster_set_window_function);
				vcluster->set_method_extern("promise<uptr@>@ tx_begin(isolation)", &VI_SPROMISIFY_REF(ldb_cluster_tx_begin, connection));
				vcluster->set_method_extern("promise<uptr@>@ tx_start(const string_view&in)", &VI_SPROMISIFY_REF(ldb_cluster_tx_start, connection));
				vcluster->set_method_extern("promise<bool>@ tx_end(const string_view&in, uptr@)", &VI_SPROMISIFY(ldb_cluster_tx_end, type_id::bool_t));
				vcluster->set_method_extern("promise<bool>@ tx_commit(uptr@)", &VI_SPROMISIFY(ldb_cluster_tx_commit, type_id::bool_t));
				vcluster->set_method_extern("promise<bool>@ tx_rollback(uptr@)", &VI_SPROMISIFY(ldb_cluster_tx_rollback, type_id::bool_t));
				vcluster->set_method_extern("promise<bool>@ connect(const string_view&in, usize = 1)", &VI_SPROMISIFY(ldb_cluster_connect, type_id::bool_t));
				vcluster->set_method_extern("promise<bool>@ disconnect()", &VI_SPROMISIFY(ldb_cluster_disconnect, type_id::bool_t));
				vcluster->set_method_extern("promise<bool>@ flush()", &VI_SPROMISIFY(ldb_cluster_flush, type_id::bool_t));
				vcluster->set_method_extern("promise<cursor>@ query(const string_view&in, usize = 0, uptr@ = null)", &VI_SPROMISIFY_REF(ldb_cluster_query, cursor));
				vcluster->set_method_extern("promise<cursor>@ emplace_query(const string_view&in, array<schema@>@+, usize = 0, uptr@ = null)", &VI_SPROMISIFY_REF(ldb_cluster_emplace_query, cursor));
				vcluster->set_method_extern("promise<cursor>@ template_query(const string_view&in, dictionary@+, usize = 0, uptr@ = null)", &VI_SPROMISIFY_REF(ldb_cluster_template_query, cursor));

				auto vdriver = vm->set_class<network::sqlite::driver>("driver", false);
				vdriver->set_function_def("void query_log_async(const string&in)");
				vdriver->set_constructor<network::sqlite::driver>("driver@ f()");
				vdriver->set_method("void log_query(const string_view&in)", &network::sqlite::driver::log_query);
				vdriver->set_method("void add_constant(const string_view&in, const string_view&in)", &network::sqlite::driver::add_constant);
				vdriver->set_method_extern("bool add_query(const string_view&in, const string_view&in)", &VI_EXPECTIFY_VOID(network::sqlite::driver::add_query));
				vdriver->set_method_extern("bool add_directory(const string_view&in, const string_view&in = string_view())", &VI_EXPECTIFY_VOID(network::sqlite::driver::add_directory));
				vdriver->set_method("bool remove_constant(const string_view&in)", &network::sqlite::driver::remove_constant);
				vdriver->set_method("bool remove_query(const string_view&in)", &network::sqlite::driver::remove_query);
				vdriver->set_method("bool load_cache_dump(schema@+)", &network::sqlite::driver::load_cache_dump);
				vdriver->set_method("schema@ get_cache_dump()", &network::sqlite::driver::get_cache_dump);
				vdriver->set_method_extern("void set_query_log(query_log_async@)", &ldb_driver_set_query_log);
				vdriver->set_method_extern("string emplace(const string_view&in, array<schema@>@+)", &ldb_driver_emplace);
				vdriver->set_method_extern("string get_query(const string_view&in, dictionary@+)", &ldb_driver_get_query);
				vdriver->set_method_extern("array<string>@ get_queries()", &ldb_driver_get_queries);
				vdriver->set_method_static("driver@+ get()", &network::sqlite::driver::get);

				vm->end_namespace();
				return true;
#else
				VI_ASSERT(false, "<sqlite> is not loaded");
				return false;
#endif
			}
			bool registry::import_pq(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				VI_TYPEREF(connection, "pq::connection");
				VI_TYPEREF(cursor, "pq::cursor");

				vm->begin_namespace("pq");
				auto visolation = vm->set_enum("isolation");
				visolation->set_value("serializable", (int)network::pq::isolation::serializable);
				visolation->set_value("repeatable_read", (int)network::pq::isolation::repeatable_read);
				visolation->set_value("read_commited", (int)network::pq::isolation::read_commited);
				visolation->set_value("read_uncommited", (int)network::pq::isolation::read_uncommited);
				visolation->set_value("default_isolation", (int)network::pq::isolation::default_isolation);

				auto vquery_op = vm->set_enum("query_op");
				vquery_op->set_value("cache_short", (int)network::pq::query_op::cache_short);
				vquery_op->set_value("cache_mid", (int)network::pq::query_op::cache_mid);
				vquery_op->set_value("cache_long", (int)network::pq::query_op::cache_long);

				auto vaddress_op = vm->set_enum("address_op");
				vaddress_op->set_value("host", (int)network::pq::address_op::host);
				vaddress_op->set_value("ip", (int)network::pq::address_op::ip);
				vaddress_op->set_value("port", (int)network::pq::address_op::port);
				vaddress_op->set_value("database", (int)network::pq::address_op::database);
				vaddress_op->set_value("user", (int)network::pq::address_op::user);
				vaddress_op->set_value("password", (int)network::pq::address_op::password);
				vaddress_op->set_value("timeout", (int)network::pq::address_op::timeout);
				vaddress_op->set_value("encoding", (int)network::pq::address_op::encoding);
				vaddress_op->set_value("options", (int)network::pq::address_op::options);
				vaddress_op->set_value("profile", (int)network::pq::address_op::profile);
				vaddress_op->set_value("fallback_profile", (int)network::pq::address_op::fallback_profile);
				vaddress_op->set_value("keep_alive", (int)network::pq::address_op::keep_alive);
				vaddress_op->set_value("keep_alive_idle", (int)network::pq::address_op::keep_alive_idle);
				vaddress_op->set_value("keep_alive_interval", (int)network::pq::address_op::keep_alive_interval);
				vaddress_op->set_value("keep_alive_count", (int)network::pq::address_op::keep_alive_count);
				vaddress_op->set_value("tty", (int)network::pq::address_op::TTY);
				vaddress_op->set_value("ssl", (int)network::pq::address_op::SSL);
				vaddress_op->set_value("ssl_compression", (int)network::pq::address_op::SSL_Compression);
				vaddress_op->set_value("ssl_cert", (int)network::pq::address_op::SSL_Cert);
				vaddress_op->set_value("ssl_root_cert", (int)network::pq::address_op::SSL_Root_Cert);
				vaddress_op->set_value("ssl_key", (int)network::pq::address_op::SSL_Key);
				vaddress_op->set_value("ssl_crl", (int)network::pq::address_op::SSL_CRL);
				vaddress_op->set_value("require_peer", (int)network::pq::address_op::require_peer);
				vaddress_op->set_value("require_ssl", (int)network::pq::address_op::require_ssl);
				vaddress_op->set_value("krb_server_name", (int)network::pq::address_op::krb_server_name);
				vaddress_op->set_value("service", (int)network::pq::address_op::service);

				auto vquery_exec = vm->set_enum("query_exec");
				vquery_exec->set_value("empty_query", (int)network::pq::query_exec::empty_query);
				vquery_exec->set_value("command_ok", (int)network::pq::query_exec::command_ok);
				vquery_exec->set_value("tuples_ok", (int)network::pq::query_exec::tuples_ok);
				vquery_exec->set_value("copy_out", (int)network::pq::query_exec::copy_out);
				vquery_exec->set_value("copy_in", (int)network::pq::query_exec::copy_in);
				vquery_exec->set_value("bad_response", (int)network::pq::query_exec::bad_response);
				vquery_exec->set_value("non_fatal_error", (int)network::pq::query_exec::non_fatal_error);
				vquery_exec->set_value("fatal_error", (int)network::pq::query_exec::fatal_error);
				vquery_exec->set_value("copy_both", (int)network::pq::query_exec::copy_both);
				vquery_exec->set_value("single_tuple", (int)network::pq::query_exec::single_tuple);

				auto vfield_code = vm->set_enum("field_code");
				vfield_code->set_value("severity", (int)network::pq::field_code::severity);
				vfield_code->set_value("severity_nonlocalized", (int)network::pq::field_code::severity_nonlocalized);
				vfield_code->set_value("sql_state", (int)network::pq::field_code::sql_state);
				vfield_code->set_value("messagep_rimary", (int)network::pq::field_code::message_primary);
				vfield_code->set_value("message_detail", (int)network::pq::field_code::message_detail);
				vfield_code->set_value("message_hint", (int)network::pq::field_code::message_hint);
				vfield_code->set_value("statement_position", (int)network::pq::field_code::statement_position);
				vfield_code->set_value("internal_position", (int)network::pq::field_code::internal_position);
				vfield_code->set_value("internal_query", (int)network::pq::field_code::internal_query);
				vfield_code->set_value("context", (int)network::pq::field_code::context);
				vfield_code->set_value("schema_name", (int)network::pq::field_code::schema_name);
				vfield_code->set_value("table_name", (int)network::pq::field_code::table_name);
				vfield_code->set_value("column_name", (int)network::pq::field_code::column_name);
				vfield_code->set_value("data_type_name", (int)network::pq::field_code::data_type_name);
				vfield_code->set_value("constraint_name", (int)network::pq::field_code::constraint_name);
				vfield_code->set_value("source_file", (int)network::pq::field_code::source_file);
				vfield_code->set_value("source_line", (int)network::pq::field_code::source_line);
				vfield_code->set_value("source_function", (int)network::pq::field_code::source_function);

				auto vtransaction_state = vm->set_enum("transaction_state");
				vtransaction_state->set_value("idle", (int)network::pq::transaction_state::idle);
				vtransaction_state->set_value("active", (int)network::pq::transaction_state::active);
				vtransaction_state->set_value("in_transaction", (int)network::pq::transaction_state::in_transaction);
				vtransaction_state->set_value("in_error", (int)network::pq::transaction_state::in_error);
				vtransaction_state->set_value("none", (int)network::pq::transaction_state::none);

				auto vcaching = vm->set_enum("caching");
				vcaching->set_value("never", (int)network::pq::caching::never);
				vcaching->set_value("miss", (int)network::pq::caching::miss);
				vcaching->set_value("cached", (int)network::pq::caching::cached);

				auto void_type = vm->set_enum("oid_type");
				void_type->set_value("json", (int)network::pq::oid_type::JSON);
				void_type->set_value("jsonb", (int)network::pq::oid_type::JSONB);
				void_type->set_value("any_array", (int)network::pq::oid_type::any_array);
				void_type->set_value("name_array", (int)network::pq::oid_type::name_array);
				void_type->set_value("text_array", (int)network::pq::oid_type::text_array);
				void_type->set_value("date_array", (int)network::pq::oid_type::date_array);
				void_type->set_value("time_array", (int)network::pq::oid_type::time_array);
				void_type->set_value("uuid_array", (int)network::pq::oid_type::uuid_array);
				void_type->set_value("cstring_array", (int)network::pq::oid_type::cstring_array);
				void_type->set_value("bp_char_array", (int)network::pq::oid_type::bp_char_array);
				void_type->set_value("var_char_array", (int)network::pq::oid_type::var_char_array);
				void_type->set_value("bit_array", (int)network::pq::oid_type::bit_array);
				void_type->set_value("var_bit_array", (int)network::pq::oid_type::var_bit_array);
				void_type->set_value("char_array", (int)network::pq::oid_type::char_array);
				void_type->set_value("int2_array", (int)network::pq::oid_type::int2_array);
				void_type->set_value("int4_array", (int)network::pq::oid_type::int4_array);
				void_type->set_value("int8_array", (int)network::pq::oid_type::int8_array);
				void_type->set_value("bool_array", (int)network::pq::oid_type::bool_array);
				void_type->set_value("float4_array", (int)network::pq::oid_type::float4_array);
				void_type->set_value("float8_array", (int)network::pq::oid_type::float8_array);
				void_type->set_value("money_array", (int)network::pq::oid_type::money_array);
				void_type->set_value("numeric_array", (int)network::pq::oid_type::numeric_array);
				void_type->set_value("bytea_array", (int)network::pq::oid_type::bytea_array);
				void_type->set_value("any_t", (int)network::pq::oid_type::any);
				void_type->set_value("name_t", (int)network::pq::oid_type::name);
				void_type->set_value("text_t", (int)network::pq::oid_type::text);
				void_type->set_value("date_t", (int)network::pq::oid_type::date);
				void_type->set_value("time_t", (int)network::pq::oid_type::time);
				void_type->set_value("uuid_t", (int)network::pq::oid_type::UUID);
				void_type->set_value("cstring_t", (int)network::pq::oid_type::cstring);
				void_type->set_value("bp_char_t", (int)network::pq::oid_type::bp_char);
				void_type->set_value("var_char_t", (int)network::pq::oid_type::var_char);
				void_type->set_value("bit_t", (int)network::pq::oid_type::bit);
				void_type->set_value("var_bit_t", (int)network::pq::oid_type::var_bit);
				void_type->set_value("char_t", (int)network::pq::oid_type::symbol);
				void_type->set_value("int2_t", (int)network::pq::oid_type::int2);
				void_type->set_value("int4_t", (int)network::pq::oid_type::int4);
				void_type->set_value("int8_t", (int)network::pq::oid_type::int8);
				void_type->set_value("bool_t", (int)network::pq::oid_type::boolf);
				void_type->set_value("float4_t", (int)network::pq::oid_type::float4);
				void_type->set_value("float8_t", (int)network::pq::oid_type::float8);
				void_type->set_value("money_t", (int)network::pq::oid_type::money);
				void_type->set_value("numeric_t", (int)network::pq::oid_type::numeric);
				void_type->set_value("bytea_t", (int)network::pq::oid_type::bytea);

				auto vquery_state = vm->set_enum("query_state");
				vquery_state->set_value("lost", (int)network::pq::query_state::lost);
				vquery_state->set_value("idle", (int)network::pq::query_state::idle);
				vquery_state->set_value("idle_in_transaction", (int)network::pq::query_state::idle_in_transaction);
				vquery_state->set_value("busy", (int)network::pq::query_state::busy);
				vquery_state->set_value("busy_in_transaction", (int)network::pq::query_state::busy_in_transaction);

				auto vaddress = vm->set_struct_trivial<network::pq::address>("host_address");
				vaddress->set_constructor<network::pq::address>("void f()");
				vaddress->set_method("void override(const string_view&in, const string_view&in)", &network::pq::address::override);
				vaddress->set_method("bool set(address_op, const string_view&in)", &network::pq::address::set);
				vaddress->set_method<network::pq::address, core::string, network::pq::address_op>("string get(address_op) const", &network::pq::address::get);
				vaddress->set_method("string get_address() const", &network::pq::address::get_address);
				vaddress->set_method_static("host_address from_url(const string_view&in)", &VI_SEXPECTIFY(network::pq::address::from_url));

				auto vnotify = vm->set_struct_trivial<network::pq::notify>("notify");
				vnotify->set_constructor<network::pq::notify, const network::pq::notify&>("void f(const notify&in)");
				vnotify->set_method("schema@ get_schema() const", &network::pq::notify::get_schema);
				vnotify->set_method("string get_name() const", &network::pq::notify::get_name);
				vnotify->set_method("string get_data() const", &network::pq::notify::get_data);
				vnotify->set_method("int32 get_pid() const", &network::pq::notify::get_pid);

				auto vrow = vm->set_struct_trivial<network::pq::row>("row", (size_t)object_behaviours::app_class_allints);
				auto vcolumn = vm->set_struct_trivial<network::pq::column>("column");
				vcolumn->set_constructor<network::pq::column, const network::pq::column&>("void f(const column&in)");
				vcolumn->set_method("string get_name() const", &network::pq::column::get_name);
				vcolumn->set_method("string get_value_text() const", &network::pq::column::get_value_text);
				vcolumn->set_method("variant get() const", &network::pq::column::get);
				vcolumn->set_method("schema@ get_inline() const", &network::pq::column::get_inline);
				vcolumn->set_method("int32 get_format_id() const", &network::pq::column::get_format_id);
				vcolumn->set_method("int32 get_mod_id() const", &network::pq::column::get_mod_id);
				vcolumn->set_method("uint64 get_table_id() const", &network::pq::column::get_table_id);
				vcolumn->set_method("uint64 get_type_id() const", &network::pq::column::get_type_id);
				vcolumn->set_method("usize index() const", &network::pq::column::index);
				vcolumn->set_method("usize size() const", &network::pq::column::size);
				vcolumn->set_method("row get_row() const", &network::pq::column::get_row);
				vcolumn->set_method("bool nullable() const", &network::pq::column::nullable);
				vcolumn->set_method("bool exists() const", &network::pq::column::exists);

				auto vresponse = vm->set_struct<network::pq::response>("response");
				vrow->set_constructor<network::pq::row, const network::pq::row&>("void f(const row&in)");
				vrow->set_method("schema@ get_object() const", &network::pq::row::get_object);
				vrow->set_method("schema@ get_array() const", &network::pq::row::get_array);
				vrow->set_method("usize index() const", &network::pq::row::index);
				vrow->set_method("usize size() const", &network::pq::row::size);
				vrow->set_method<network::pq::row, network::pq::column, size_t>("column get_column(usize) const", &network::pq::row::get_column);
				vrow->set_method<network::pq::row, network::pq::column, const std::string_view&>("column get_column(const string_view&in) const", &network::pq::row::get_column);
				vrow->set_method("bool exists() const", &network::pq::row::exists);
				vrow->set_method<network::pq::row, network::pq::column, size_t>("column opIndex(usize)", &network::pq::row::get_column);
				vrow->set_method<network::pq::row, network::pq::column, size_t>("column opIndex(usize) const", &network::pq::row::get_column);
				vrow->set_method<network::pq::row, network::pq::column, const std::string_view&>("column opIndex(const string_view&in)", &network::pq::row::get_column);
				vrow->set_method<network::pq::row, network::pq::column, const std::string_view&>("column opIndex(const string_view&in) const", &network::pq::row::get_column);

				vresponse->set_constructor<network::pq::response>("void f()");
				vresponse->set_operator_copy_static(&pdb_response_copy);
				vresponse->set_destructor<network::pq::response>("void f()");
				vresponse->set_method<network::pq::response, network::pq::row, size_t>("row opIndex(usize)", &network::pq::response::get_row);
				vresponse->set_method<network::pq::response, network::pq::row, size_t>("row opIndex(usize) const", &network::pq::response::get_row);
				vresponse->set_method("schema@ get_array_of_objects() const", &network::pq::response::get_array_of_objects);
				vresponse->set_method("schema@ get_array_of_arrays() const", &network::pq::response::get_array_of_arrays);
				vresponse->set_method("schema@ get_object(usize = 0) const", &network::pq::response::get_object);
				vresponse->set_method("schema@ get_array(usize = 0) const", &network::pq::response::get_array);
				vresponse->set_method_extern("array<string>@ get_columns() const", &pdb_response_get_columns);
				vresponse->set_method("string get_command_status_text() const", &network::pq::response::get_command_status_text);
				vresponse->set_method("string get_status_text() const", &network::pq::response::get_status_text);
				vresponse->set_method("string get_error_text() const", &network::pq::response::get_error_text);
				vresponse->set_method("string get_error_field(field_code) const", &network::pq::response::get_error_field);
				vresponse->set_method("int32 get_name_index(const string_view&in) const", &network::pq::response::get_name_index);
				vresponse->set_method("query_exec get_status() const", &network::pq::response::get_status);
				vresponse->set_method("uint64 get_value_id() const", &network::pq::response::get_value_id);
				vresponse->set_method("usize affected_rows() const", &network::pq::response::affected_rows);
				vresponse->set_method("usize size() const", &network::pq::response::size);
				vresponse->set_method("row get_row(usize) const", &network::pq::response::get_row);
				vresponse->set_method("row front() const", &network::pq::response::front);
				vresponse->set_method("row back() const", &network::pq::response::back);
				vresponse->set_method("response copy() const", &network::pq::response::copy);
				vresponse->set_method("uptr@ get() const", &network::pq::response::get);
				vresponse->set_method("bool empty() const", &network::pq::response::empty);
				vresponse->set_method("bool error() const", &network::pq::response::error);
				vresponse->set_method("bool error_or_empty() const", &network::pq::response::error_or_empty);
				vresponse->set_method("bool exists() const", &network::pq::response::exists);

				auto vconnection = vm->set_class<network::pq::connection>("connection", false);
				auto vcursor = vm->set_struct<network::pq::cursor>("cursor");
				vcursor->set_constructor<network::pq::cursor>("void f()");
				vcursor->set_operator_copy_static(&pdb_cursor_copy);
				vcursor->set_destructor<network::pq::cursor>("void f()");
				vcursor->set_method("column opIndex(const string_view&in)", &network::pq::cursor::get_column);
				vcursor->set_method("column opIndex(const string_view&in) const", &network::pq::cursor::get_column);
				vcursor->set_method("bool success() const", &network::pq::cursor::success);
				vcursor->set_method("bool empty() const", &network::pq::cursor::empty);
				vcursor->set_method("bool error() const", &network::pq::cursor::error);
				vcursor->set_method("bool error_or_empty() const", &network::pq::cursor::error_or_empty);
				vcursor->set_method("usize size() const", &network::pq::cursor::size);
				vcursor->set_method("usize affected_rows() const", &network::pq::cursor::affected_rows);
				vcursor->set_method("cursor copy() const", &network::pq::cursor::copy);
				vcursor->set_method_extern("response first() const", &pdb_cursor_first);
				vcursor->set_method_extern("response last() const", &pdb_cursor_last);
				vcursor->set_method_extern("response at(usize) const", &pdb_cursor_at);
				vcursor->set_method("connection@+ get_executor() const", &network::pq::cursor::get_executor);
				vcursor->set_method("caching get_cache_status() const", &network::pq::cursor::get_cache_status);
				vcursor->set_method("schema@ get_array_of_objects(usize = 0) const", &network::pq::cursor::get_array_of_objects);
				vcursor->set_method("schema@ get_array_of_arrays(usize = 0) const", &network::pq::cursor::get_array_of_arrays);
				vcursor->set_method("schema@ get_object(usize = 0, usize = 0) const", &network::pq::cursor::get_object);
				vcursor->set_method("schema@ get_array(usize = 0, usize = 0) const", &network::pq::cursor::get_array);

				auto vrequest = vm->set_class<network::pq::request>("request", false);
				vconnection->set_method("uptr@ get_base() const", &network::pq::connection::get_base);
				vconnection->set_method("socket@+ get_stream() const", &network::pq::connection::get_stream);
				vconnection->set_method("request@+ get_current() const", &network::pq::connection::get_current);
				vconnection->set_method("query_state get_state() const", &network::pq::connection::get_state);
				vconnection->set_method("transaction_state get_tx_state() const", &network::pq::connection::get_tx_state);
				vconnection->set_method("bool in_transaction() const", &network::pq::connection::in_transaction);
				vconnection->set_method("bool busy() const", &network::pq::connection::busy);

				vrequest->set_method("cursor& get_result()", &network::pq::request::get_result);
				vrequest->set_method("connection@+ get_session() const", &network::pq::request::get_session);
				vrequest->set_method("uint64 get_timing() const", &network::pq::request::get_timing);
				vrequest->set_method("bool pending() const", &network::pq::request::pending);

				auto vcluster = vm->set_class<network::pq::cluster>("cluster", false);
				vcluster->set_function_def("promise<bool>@ reconnect_async(cluster@+, array<string>@+)");
				vcluster->set_function_def("void notification_async(cluster@+, const notify&in)");
				vcluster->set_constructor<network::pq::cluster>("cluster@ f()");
				vcluster->set_method("void clear_cache()", &network::pq::cluster::clear_cache);
				vcluster->set_method("void set_cache_cleanup(uint64)", &network::pq::cluster::set_cache_cleanup);
				vcluster->set_method("void set_cache_duration(query_op, uint64)", &network::pq::cluster::set_cache_duration);
				vcluster->set_method("bool remove_channel(const string_view&in, uint64)", &network::pq::cluster::remove_channel);
				vcluster->set_method("connection@+ get_connection(query_state)", &network::pq::cluster::get_connection);
				vcluster->set_method("connection@+ get_any_connection()", &network::pq::cluster::get_any_connection);
				vcluster->set_method("bool is_connected() const", &network::pq::cluster::is_connected);
				vcluster->set_method_extern("promise<connection@>@ tx_begin(isolation)", &VI_SPROMISIFY_REF(pdb_cluster_tx_begin, connection));
				vcluster->set_method_extern("promise<connection@>@ tx_start(const string_view&in)", &VI_SPROMISIFY_REF(pdb_cluster_tx_start, connection));
				vcluster->set_method_extern("promise<bool>@ tx_end(const string_view&in, connection@+)", &VI_SPROMISIFY(pdb_cluster_tx_end, type_id::bool_t));
				vcluster->set_method_extern("promise<bool>@ tx_commit(connection@+)", &VI_SPROMISIFY(pdb_cluster_tx_commit, type_id::bool_t));
				vcluster->set_method_extern("promise<bool>@ tx_rollback(connection@+)", &VI_SPROMISIFY(pdb_cluster_tx_rollback, type_id::bool_t));
				vcluster->set_method_extern("promise<bool>@ connect(const host_address&in, usize = 1)", &VI_SPROMISIFY(pdb_cluster_connect, type_id::bool_t));
				vcluster->set_method_extern("promise<bool>@ disconnect()", &VI_SPROMISIFY(pdb_cluster_disconnect, type_id::bool_t));
				vcluster->set_method_extern("promise<cursor>@ query(const string_view&in, usize = 0, connection@+ = null)", &VI_SPROMISIFY_REF(pdb_cluster_query, cursor));
				vcluster->set_method_extern("void set_when_reconnected(reconnect_async@)", &pdb_cluster_set_when_reconnected);
				vcluster->set_method_extern("uint64 add_channel(const string_view&in, notification_async@)", &pdb_cluster_add_channel);
				vcluster->set_method_extern("promise<bool>@ listen(array<string>@+)", &VI_SPROMISIFY(pdb_cluster_listen, type_id::bool_t));
				vcluster->set_method_extern("promise<bool>@ unlisten(array<string>@+)", &VI_SPROMISIFY(pdb_cluster_unlisten, type_id::bool_t));
				vcluster->set_method_extern("promise<cursor>@ emplace_query(const string_view&in, array<schema@>@+, usize = 0, connection@+ = null)", &VI_SPROMISIFY_REF(pdb_cluster_emplace_query, cursor));
				vcluster->set_method_extern("promise<cursor>@ template_query(const string_view&in, dictionary@+, usize = 0, connection@+ = null)", &VI_SPROMISIFY_REF(pdb_cluster_template_query, cursor));

				auto vdriver = vm->set_class<network::pq::driver>("driver", false);
				vdriver->set_function_def("void query_log_async(const string_view&in)");
				vdriver->set_constructor<network::pq::driver>("driver@ f()");
				vdriver->set_method("void log_query(const string_view&in)", &network::pq::driver::log_query);
				vdriver->set_method("void add_constant(const string_view&in, const string_view&in)", &network::pq::driver::add_constant);
				vdriver->set_method_extern("bool add_query(const string_view&in, const string_view&in)", &VI_EXPECTIFY_VOID(network::pq::driver::add_query));
				vdriver->set_method_extern("bool add_directory(const string_view&in, const string_view&in = string_view())", &VI_EXPECTIFY_VOID(network::pq::driver::add_directory));
				vdriver->set_method("bool remove_constant(const string_view&in)", &network::pq::driver::remove_constant);
				vdriver->set_method("bool remove_query(const string_view&in)", &network::pq::driver::remove_query);
				vdriver->set_method("bool load_cache_dump(schema@+)", &network::pq::driver::load_cache_dump);
				vdriver->set_method("schema@ get_cache_dump()", &network::pq::driver::get_cache_dump);
				vdriver->set_method_extern("void set_query_log(query_log_async@)", &pdb_driver_set_query_log);
				vdriver->set_method_extern("string emplace(cluster@+, const string_view&in, array<schema@>@+)", &pdb_driver_emplace);
				vdriver->set_method_extern("string get_query(cluster@+, const string_view&in, dictionary@+)", &pdb_driver_get_query);
				vdriver->set_method_extern("array<string>@ get_queries()", &pdb_driver_get_queries);
				vdriver->set_method_static("driver@+ get()", &network::pq::driver::get);

				vm->end_namespace();
				return true;
#else
				VI_ASSERT(false, "<postgresql> is not loaded");
				return false;
#endif
			}
			bool registry::import_mongo(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				VI_TYPEREF(property, "mongo::doc_property");
				VI_TYPEREF(document, "mongo::document");
				VI_TYPEREF(cursor, "mongo::cursor");
				VI_TYPEREF(response, "mongo::response");
				VI_TYPEREF(collection, "mongo::collection");
				VI_TYPEREF(transaction_state, "mongo::transaction_state");
				VI_TYPEREF(schema, "schema@");

				vm->begin_namespace("mongo");
				auto vquery_flags = vm->set_enum("query_flags");
				vquery_flags->set_value("none", (int)network::mongo::query_flags::none);
				vquery_flags->set_value("tailable_cursor", (int)network::mongo::query_flags::tailable_cursor);
				vquery_flags->set_value("slave_ok", (int)network::mongo::query_flags::slave_ok);
				vquery_flags->set_value("oplog_replay", (int)network::mongo::query_flags::oplog_replay);
				vquery_flags->set_value("no_cursor_timeout", (int)network::mongo::query_flags::no_cursor_timeout);
				vquery_flags->set_value("await_data", (int)network::mongo::query_flags::await_data);
				vquery_flags->set_value("exhaust", (int)network::mongo::query_flags::exhaust);
				vquery_flags->set_value("partial", (int)network::mongo::query_flags::partial);

				auto vtype = vm->set_enum("doc_type");
				vtype->set_value("unknown_t", (int)network::mongo::type::unknown);
				vtype->set_value("uncastable_t", (int)network::mongo::type::uncastable);
				vtype->set_value("null_t", (int)network::mongo::type::null);
				vtype->set_value("document_t", (int)network::mongo::type::document);
				vtype->set_value("array_t", (int)network::mongo::type::array);
				vtype->set_value("string_t", (int)network::mongo::type::string);
				vtype->set_value("integer_t", (int)network::mongo::type::integer);
				vtype->set_value("number_t", (int)network::mongo::type::number);
				vtype->set_value("boolean_t", (int)network::mongo::type::boolean);
				vtype->set_value("object_id_t", (int)network::mongo::type::object_id);
				vtype->set_value("decimal_t", (int)network::mongo::type::decimal);

				auto vtransaction_state = vm->set_enum("transaction_state");
				vtransaction_state->set_value("ok", (int)network::mongo::transaction_state::OK);
				vtransaction_state->set_value("retry_commit", (int)network::mongo::transaction_state::retry_commit);
				vtransaction_state->set_value("retry", (int)network::mongo::transaction_state::retry);
				vtransaction_state->set_value("fatal", (int)network::mongo::transaction_state::fatal);

				auto vdocument = vm->set_struct<network::mongo::document>("document");
				auto vproperty = vm->set_struct<network::mongo::property>("doc_property");
				vproperty->set_property<network::mongo::property>("string name", &network::mongo::property::name);
				vproperty->set_property<network::mongo::property>("string blob", &network::mongo::property::string);
				vproperty->set_property<network::mongo::property>("doc_type mod", &network::mongo::property::mod);
				vproperty->set_property<network::mongo::property>("int64 integer", &network::mongo::property::integer);
				vproperty->set_property<network::mongo::property>("uint64 high", &network::mongo::property::high);
				vproperty->set_property<network::mongo::property>("uint64 low", &network::mongo::property::low);
				vproperty->set_property<network::mongo::property>("bool boolean", &network::mongo::property::boolean);
				vproperty->set_property<network::mongo::property>("bool is_valid", &network::mongo::property::is_valid);
				vproperty->set_constructor<network::mongo::property>("void f()");
				vproperty->set_operator_move_copy<network::mongo::property>();
				vproperty->set_destructor<network::mongo::property>("void f()");
				vproperty->set_method("string& to_string()", &network::mongo::property::to_string);
				vproperty->set_method("document as_document() const", &network::mongo::property::as_document);
				vproperty->set_method("doc_property opIndex(const string_view&in) const", &network::mongo::property::at);

				vdocument->set_constructor<network::mongo::document>("void f()");
				vdocument->set_constructor_list_extern<network::mongo::document>("void f(int &in) { repeat { string, ? } }", &mdb_document_construct);
				vdocument->set_operator_move_copy<network::mongo::document>();
				vdocument->set_destructor<network::mongo::document>("void f()");
				vdocument->set_method("bool is_valid() const", &network::mongo::document::operator bool);
				vdocument->set_method("void cleanup()", &network::mongo::document::cleanup);
				vdocument->set_method("void join(const document&in)", &network::mongo::document::join);
				vdocument->set_method("bool set_schema(const string_view&in, const document&in, usize = 0)", &network::mongo::document::set_schema);
				vdocument->set_method("bool set_array(const string_view&in, const document&in, usize = 0)", &network::mongo::document::set_array);
				vdocument->set_method("bool set_string(const string_view&in, const string_view&in, usize = 0)", &network::mongo::document::set_string);
				vdocument->set_method("bool set_integer(const string_view&in, int64, usize = 0)", &network::mongo::document::set_integer);
				vdocument->set_method("bool set_number(const string_view&in, double, usize = 0)", &network::mongo::document::set_number);
				vdocument->set_method("bool set_decimal(const string_view&in, uint64, uint64, usize = 0)", &network::mongo::document::set_decimal);
				vdocument->set_method("bool set_decimal_string(const string_view&in, const string_view&in, usize = 0)", &network::mongo::document::set_decimal_string);
				vdocument->set_method("bool set_decimal_integer(const string_view&in, int64, usize = 0)", &network::mongo::document::set_decimal_integer);
				vdocument->set_method("bool set_decimal_number(const string_view&in, double, usize = 0)", &network::mongo::document::set_decimal_number);
				vdocument->set_method("bool set_boolean(const string_view&in, bool, usize = 0)", &network::mongo::document::set_boolean);
				vdocument->set_method("bool set_object_id(const string_view&in, const string_view&in, usize = 0)", &network::mongo::document::set_object_id);
				vdocument->set_method("bool set_null(const string_view&in, usize = 0)", &network::mongo::document::set_null);
				vdocument->set_method("bool set_property(const string_view&in, doc_property&in, usize = 0)", &network::mongo::document::set_property);
				vdocument->set_method("bool has_property(const string_view&in) const", &network::mongo::document::has_property);
				vdocument->set_method("bool get_property(const string_view&in, doc_property&out) const", &network::mongo::document::get_property);
				vdocument->set_method("usize count() const", &network::mongo::document::count);
				vdocument->set_method("string to_relaxed_json() const", &network::mongo::document::to_relaxed_json);
				vdocument->set_method("string to_extended_json() const", &network::mongo::document::to_extended_json);
				vdocument->set_method("string to_json() const", &network::mongo::document::to_json);
				vdocument->set_method("string to_indices() const", &network::mongo::document::to_indices);
				vdocument->set_method("schema@ to_schema(bool = false) const", &network::mongo::document::to_schema);
				vdocument->set_method("document copy() const", &network::mongo::document::copy);
				vdocument->set_method("document& persist(bool = true) const", &network::mongo::document::persist);
				vdocument->set_method("doc_property opIndex(const string_view&in) const", &network::mongo::document::at);
				vdocument->set_method_static("document from_empty()", &network::mongo::document::from_empty);
				vdocument->set_method_static("document from_schema(schema@+)", &network::mongo::document::from_schema);
				vdocument->set_method_static("document from_json(const string_view&in)", &VI_SEXPECTIFY(network::mongo::document::from_json));

				auto vaddress = vm->set_struct<network::mongo::address>("host_address");
				vaddress->set_constructor<network::mongo::address>("void f()");
				vaddress->set_operator_move_copy<network::mongo::address>();
				vaddress->set_destructor<network::mongo::address>("void f()");
				vaddress->set_method("bool is_valid() const", &network::mongo::address::operator bool);
				vaddress->set_method<network::mongo::address, void, const std::string_view&, int64_t>("void set_option(const string_view&in, int64)", &network::mongo::address::set_option);
				vaddress->set_method<network::mongo::address, void, const std::string_view&, bool>("void set_option(const string_view&in, bool)", &network::mongo::address::set_option);
				vaddress->set_method<network::mongo::address, void, const std::string_view&, const std::string_view&>("void set_option(const string_view&in, const string_view&in)", &network::mongo::address::set_option);
				vaddress->set_method("void set_auth_mechanism(const string_view&in)", &network::mongo::address::set_auth_mechanism);
				vaddress->set_method("void set_auth_source(const string_view&in)", &network::mongo::address::set_auth_source);
				vaddress->set_method("void set_compressors(const string_view&in)", &network::mongo::address::set_compressors);
				vaddress->set_method("void set_database(const string_view&in)", &network::mongo::address::set_database);
				vaddress->set_method("void set_username(const string_view&in)", &network::mongo::address::set_username);
				vaddress->set_method("void set_password(const string_view&in)", &network::mongo::address::set_password);
				vaddress->set_method_static("host_address from_url(const string_view&in)", &VI_SEXPECTIFY(network::mongo::address::from_url));

				auto vstream = vm->set_struct<network::mongo::stream>("stream");
				vstream->set_constructor<network::mongo::stream>("void f()");
				vstream->set_operator_move_copy<network::mongo::stream>();
				vstream->set_destructor<network::mongo::stream>("void f()");
				vstream->set_method("bool is_valid() const", &network::mongo::stream::operator bool);
				vstream->set_method_extern("bool remove_many(const document&in, const document&in)", &VI_EXPECTIFY_VOID(network::mongo::stream::remove_many));
				vstream->set_method_extern("bool remove_one(const document&in, const document&in)", &VI_EXPECTIFY_VOID(network::mongo::stream::remove_one));
				vstream->set_method_extern("bool replace_one(const document&in, const document&in, const document&in)", &VI_EXPECTIFY_VOID(network::mongo::stream::replace_one));
				vstream->set_method_extern("bool insert_one(const document&in, const document&in)", &VI_EXPECTIFY_VOID(network::mongo::stream::insert_one));
				vstream->set_method_extern("bool update_one(const document&in, const document&in, const document&in)", &VI_EXPECTIFY_VOID(network::mongo::stream::update_one));
				vstream->set_method_extern("bool update_many(const document&in, const document&in, const document&in)", &VI_EXPECTIFY_VOID(network::mongo::stream::update_many));
				vstream->set_method_extern("bool query(const document&in)", &VI_EXPECTIFY_VOID(network::mongo::stream::query));
				vstream->set_method("usize get_hint() const", &network::mongo::stream::get_hint);
				vstream->set_method_extern("bool template_query(const string_view&in, dictionary@+)", &mdb_stream_template_query);
				vstream->set_method_extern("promise<document>@ execute_with_reply()", &VI_SPROMISIFY_REF(mdb_stream_execute_with_reply, document));
				vstream->set_method_extern("promise<bool>@ execute()", &VI_SPROMISIFY(mdb_stream_execute, type_id::bool_t));

				auto vcursor = vm->set_struct<network::mongo::cursor>("cursor");
				vcursor->set_constructor<network::mongo::cursor>("void f()");
				vcursor->set_operator_move_copy<network::mongo::cursor>();
				vcursor->set_destructor<network::mongo::cursor>("void f()");
				vcursor->set_method("bool is_valid() const", &network::mongo::cursor::operator bool);
				vcursor->set_method("void set_max_await_time(usize)", &network::mongo::cursor::set_max_await_time);
				vcursor->set_method("void set_batch_size(usize)", &network::mongo::cursor::set_batch_size);
				vcursor->set_method("void set_limit(int64)", &network::mongo::cursor::set_limit);
				vcursor->set_method("void set_hint(usize)", &network::mongo::cursor::set_hint);
				vcursor->set_method_extern("string error() const", &mdb_cursor_error);
				vcursor->set_method("bool empty() const", &network::mongo::cursor::empty);
				vcursor->set_method("bool error_or_empty() const", &network::mongo::cursor::error_or_empty);
				vcursor->set_method("int64 get_id() const", &network::mongo::cursor::get_id);
				vcursor->set_method("int64 get_limit() const", &network::mongo::cursor::get_limit);
				vcursor->set_method("usize get_max_await_time() const", &network::mongo::cursor::get_max_await_time);
				vcursor->set_method("usize get_batch_size() const", &network::mongo::cursor::get_batch_size);
				vcursor->set_method("usize get_hint() const", &network::mongo::cursor::get_hint);
				vcursor->set_method("usize current() const", &network::mongo::cursor::current);
				vcursor->set_method("cursor clone() const", &network::mongo::cursor::clone);
				vcursor->set_method_extern("promise<bool>@ next() const", &VI_SPROMISIFY(mdb_cursor_next, type_id::bool_t));

				auto vresponse = vm->set_struct<network::mongo::response>("response");
				vresponse->set_constructor<network::mongo::response>("void f()");
				vresponse->set_constructor<network::mongo::response, bool>("void f(bool)");
				vresponse->set_constructor<network::mongo::response, network::mongo::cursor&>("void f(cursor&in)");
				vresponse->set_constructor<network::mongo::response, network::mongo::document&>("void f(document&in)");
				vresponse->set_operator_move_copy<network::mongo::response>();
				vresponse->set_destructor<network::mongo::response>("void f()");
				vresponse->set_method("bool is_valid() const", &network::mongo::response::operator bool);
				vresponse->set_method("bool success() const", &network::mongo::response::success);
				vresponse->set_method("cursor& get_cursor() const", &network::mongo::response::get_cursor);
				vresponse->set_method("document& get_document() const", &network::mongo::response::get_document);
				vresponse->set_method_extern("promise<schema@>@ fetch() const", &VI_SPROMISIFY_REF(mdb_response_fetch, schema));
				vresponse->set_method_extern("promise<schema@>@ fetch_all() const", &VI_SPROMISIFY_REF(mdb_response_fetch_all, schema));
				vresponse->set_method_extern("promise<doc_property>@ get_property(const string_view&in) const", &VI_SPROMISIFY_REF(mdb_response_get_property, property));
				vresponse->set_method_extern("promise<doc_property>@ opIndex(const string_view&in) const", &VI_SPROMISIFY_REF(mdb_response_get_property, property));

				auto vcollection = vm->set_struct<network::mongo::collection>("collection");
				auto vtransaction = vm->set_struct<network::mongo::transaction>("transaction");
				vtransaction->set_constructor<network::mongo::transaction>("void f()");
				vtransaction->set_constructor<network::mongo::transaction, const network::mongo::transaction&>("void f(const transaction&in)");
				vtransaction->set_operator_copy<network::mongo::transaction>();
				vtransaction->set_destructor<network::mongo::transaction>("void f()");
				vtransaction->set_method("bool is_valid() const", &network::mongo::transaction::operator bool);
				vtransaction->set_method_extern("bool push(document&in) const", &VI_EXPECTIFY_VOID(network::mongo::transaction::push));
				vtransaction->set_method_extern("promise<bool>@ begin()", &VI_SPROMISIFY(mdb_transaction_begin, type_id::bool_t));
				vtransaction->set_method_extern("promise<bool>@ rollback()", &VI_SPROMISIFY(mdb_transaction_rollback, type_id::bool_t));
				vtransaction->set_method_extern("promise<document>@ remove_many(collection&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(mdb_transaction_remove_many, document));
				vtransaction->set_method_extern("promise<document>@ remove_one(collection&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(mdb_transaction_remove_one, document));
				vtransaction->set_method_extern("promise<document>@ replace_one(collection&in, const document&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(mdb_transaction_replace_one, document));
				vtransaction->set_method_extern("promise<document>@ insert_many(collection&in, array<document>@+, const document&in)", &VI_SPROMISIFY_REF(mdb_transaction_insert_many, document));
				vtransaction->set_method_extern("promise<document>@ insert_one(collection&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(mdb_transaction_insert_one, document));
				vtransaction->set_method_extern("promise<document>@ update_many(collection&in, const document&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(mdb_transaction_update_many, document));
				vtransaction->set_method_extern("promise<document>@ update_one(collection&in, const document&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(mdb_transaction_update_one, document));
				vtransaction->set_method_extern("promise<cursor>@ find_many(collection&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(mdb_transaction_find_many, cursor));
				vtransaction->set_method_extern("promise<cursor>@ find_one(collection&in, const document&in, const document&in)", &VI_SPROMISIFY_REF(mdb_transaction_find_one, cursor));
				vtransaction->set_method_extern("promise<cursor>@ aggregate(collection&in, query_flags, const document&in, const document&in)", &VI_SPROMISIFY_REF(mdb_transaction_aggregate, cursor));
				vtransaction->set_method_extern("promise<response>@ template_query(collection&in, const string_view&in, dictionary@+)", &VI_SPROMISIFY_REF(mdb_transaction_template_query, response));
				vtransaction->set_method_extern("promise<response>@ query(collection&in, const document&in)", &VI_SPROMISIFY_REF(mdb_transaction_query, response));
				vtransaction->set_method_extern("promise<transaction_state>@ commit()", &VI_SPROMISIFY_REF(mdb_transaction_commit, transaction_state));

				vcollection->set_constructor<network::mongo::collection>("void f()");
				vcollection->set_operator_move_copy<network::mongo::collection>();
				vcollection->set_destructor<network::mongo::collection>("void f()");
				vcollection->set_method("bool is_valid() const", &network::mongo::collection::operator bool);
				vcollection->set_method("string get_name() const", &network::mongo::collection::get_name);
				vcollection->set_method_extern("stream create_stream(document&in) const", &VI_EXPECTIFY(network::mongo::collection::create_stream));
				vcollection->set_method_extern("promise<bool>@ rename(const string_view&in, const string_view&in) const", &VI_SPROMISIFY(mdb_collection_rename, type_id::bool_t));
				vcollection->set_method_extern("promise<bool>@ rename_with_options(const string_view&in, const string_view&in, const document&in) const", &VI_SPROMISIFY(mdb_collection_rename_with_options, type_id::bool_t));
				vcollection->set_method_extern("promise<bool>@ rename_with_remove(const string_view&in, const string_view&in) const", &VI_SPROMISIFY(mdb_collection_rename_with_remove, type_id::bool_t));
				vcollection->set_method_extern("promise<bool>@ rename_with_options_and_remove(const string_view&in, const string_view&in, const document&in) const", &VI_SPROMISIFY(mdb_collection_rename_with_options_and_remove, type_id::bool_t));
				vcollection->set_method_extern("promise<bool>@ remove(const document&in) const", &VI_SPROMISIFY(mdb_collection_remove, type_id::bool_t));
				vcollection->set_method_extern("promise<bool>@ remove_index(const string_view&in, const document&in) const", &VI_SPROMISIFY(mdb_collection_remove_index, type_id::bool_t));
				vcollection->set_method_extern("promise<document>@ remove_many(const document&in, const document&in) const", &VI_SPROMISIFY_REF(mdb_collection_remove_many, document));
				vcollection->set_method_extern("promise<document>@ remove_one(const document&in, const document&in) const", &VI_SPROMISIFY_REF(mdb_collection_remove_one, document));
				vcollection->set_method_extern("promise<document>@ replace_one(const document&in, const document&in, const document&in) const", &VI_SPROMISIFY_REF(mdb_collection_replace_one, document));
				vcollection->set_method_extern("promise<document>@ insert_many(array<document>@+, const document&in) const", &VI_SPROMISIFY_REF(mdb_collection_insert_many, document));
				vcollection->set_method_extern("promise<document>@ insert_one(const document&in, const document&in) const", &VI_SPROMISIFY_REF(mdb_collection_insert_one, document));
				vcollection->set_method_extern("promise<document>@ update_many(const document&in, const document&in, const document&in) const", &VI_SPROMISIFY_REF(mdb_collection_update_many, document));
				vcollection->set_method_extern("promise<document>@ update_one(const document&in, const document&in, const document&in) const", &VI_SPROMISIFY_REF(mdb_collection_update_one, document));
				vcollection->set_method_extern("promise<document>@ find_and_modify(const document&in, const document&in, const document&in, const document&in, bool, bool, bool) const", &VI_SPROMISIFY_REF(mdb_collection_find_and_modify, document));
#ifdef VI_64
				vcollection->set_method_extern("promise<usize>@ count_documents(const document&in, const document&in) const", &VI_PROMISIFY(network::mongo::collection::count_documents, type_id::int64_t));
				vcollection->set_method_extern("promise<usize>@ count_documents_estimated(const document&in) const", &VI_PROMISIFY(network::mongo::collection::count_documents_estimated, type_id::int64_t));
#else
				vcollection->set_method_extern("promise<usize>@ count_documents(const document&in, const document&in) const", &VI_PROMISIFY(network::mongo::collection::count_documents, type_id::int32_t));
				vcollection->set_method_extern("promise<usize>@ count_documents_estimated(const document&in) const", &VI_PROMISIFY(network::mongo::collection::count_documents_estimated, type_id::int32_t));
#endif
				vcollection->set_method_extern("promise<cursor>@ find_indexes(const document&in) const", &VI_SPROMISIFY_REF(mdb_collection_find_indexes, cursor));
				vcollection->set_method_extern("promise<cursor>@ find_many(const document&in, const document&in) const", &VI_SPROMISIFY_REF(mdb_collection_find_many, cursor));
				vcollection->set_method_extern("promise<cursor>@ find_one(const document&in, const document&in) const", &VI_SPROMISIFY_REF(mdb_collection_find_one, cursor));
				vcollection->set_method_extern("promise<cursor>@ aggregate(query_flags, const document&in, const document&in) const", &VI_SPROMISIFY_REF(mdb_collection_aggregate, cursor));
				vcollection->set_method_extern("promise<response>@ template_query(const string_view&in, dictionary@+, bool = true, const transaction&in = transaction()) const", &VI_SPROMISIFY_REF(mdb_collection_template_query, response));
				vcollection->set_method_extern("promise<response>@ query(const document&in, const transaction&in = transaction()) const", &VI_SPROMISIFY_REF(mdb_collection_query, response));

				auto vdatabase = vm->set_struct<network::mongo::database>("database");
				vdatabase->set_constructor<network::mongo::database>("void f()");
				vdatabase->set_operator_move_copy<network::mongo::database>();
				vdatabase->set_destructor<network::mongo::database>("void f()");
				vdatabase->set_method("bool is_valid() const", &network::mongo::database::operator bool);
				vdatabase->set_method("string get_name() const", &network::mongo::database::get_name);
				vdatabase->set_method("collection get_collection(const string_view&in) const", &network::mongo::database::get_collection);
				vdatabase->set_method_extern("promise<bool>@ remove_all_users()", &VI_SPROMISIFY(mdb_database_remove_all_users, type_id::bool_t));
				vdatabase->set_method_extern("promise<bool>@ remove_user(const string_view&in)", &VI_SPROMISIFY(mdb_database_remove_user, type_id::bool_t));
				vdatabase->set_method_extern("promise<bool>@ remove()", &VI_SPROMISIFY(mdb_database_remove, type_id::bool_t));
				vdatabase->set_method_extern("promise<bool>@ remove_with_options(const document&in)", &VI_SPROMISIFY(mdb_database_remove_with_options, type_id::bool_t));
				vdatabase->set_method_extern("promise<bool>@ add_user(const string_view&in, const string_view&in, const document&in, const document&in)", &VI_SPROMISIFY(mdb_database_add_user, type_id::bool_t));
				vdatabase->set_method_extern("promise<bool>@ has_collection(const string_view&in)", &VI_SPROMISIFY(mdb_database_has_collection, type_id::bool_t));
				vdatabase->set_method_extern("promise<collection>@ create_collection(const string_view&in, const document&in)", &VI_SPROMISIFY_REF(mdb_database_create_collection, collection));
				vdatabase->set_method_extern("promise<cursor>@ find_collections(const document&in)", &VI_SPROMISIFY_REF(mdb_database_find_collections, cursor));
				vdatabase->set_method_extern("array<string>@ get_collection_names(const document&in)", &mdb_database_get_collection_names);

				auto vconnection = vm->set_class<network::mongo::connection>("connection", false);
				auto vwatcher = vm->set_struct<network::mongo::watcher>("watcher");
				vwatcher->set_constructor<network::mongo::watcher>("void f()");
				vwatcher->set_operator_move_copy<network::mongo::watcher>();
				vwatcher->set_destructor<network::mongo::watcher>("void f()");
				vwatcher->set_method("bool is_valid() const", &network::mongo::watcher::operator bool);
				vwatcher->set_method_extern("promise<bool>@ next(document&out)", &VI_SPROMISIFY(mdb_watcher_next, type_id::bool_t));
				vwatcher->set_method_extern("promise<bool>@ error(document&out)", &VI_SPROMISIFY(mdb_watcher_error, type_id::bool_t));
				vwatcher->set_method_static("watcher from_connection(connection@+, const document&in, const document&in)", &VI_SEXPECTIFY(network::mongo::watcher::from_connection));
				vwatcher->set_method_static("watcher from_database(const database&in, const document&in, const document&in)", &VI_SEXPECTIFY(network::mongo::watcher::from_database));
				vwatcher->set_method_static("watcher from_collection(const collection&in, const document&in, const document&in)", &VI_SEXPECTIFY(network::mongo::watcher::from_collection));

				auto vcluster = vm->set_class<network::mongo::cluster>("cluster", false);
				vconnection->set_function_def("promise<bool>@ transaction_async(connection@+, transaction&in)");
				vconnection->set_constructor<network::mongo::connection>("connection@ f()");
				vconnection->set_method("void set_profile(const string_view&in)", &network::mongo::connection::set_profile);
				vconnection->set_method_extern("bool set_server(bool)", &VI_EXPECTIFY_VOID(network::mongo::connection::set_server));
				vconnection->set_method_extern("transaction& get_session()", &VI_EXPECTIFY(network::mongo::connection::get_session));
				vconnection->set_method("database get_database(const string_view&in) const", &network::mongo::connection::get_database);
				vconnection->set_method("database get_default_database() const", &network::mongo::connection::get_default_database);
				vconnection->set_method("collection get_collection(const string_view&in, const string_view&in) const", &network::mongo::connection::get_collection);
				vconnection->set_method("host_address get_address() const", &network::mongo::connection::get_address);
				vconnection->set_method("cluster@+ get_master() const", &network::mongo::connection::get_master);
				vconnection->set_method("bool connected() const", &network::mongo::connection::is_connected);
				vconnection->set_method_extern("array<string>@ get_database_names(const document&in) const", &mdb_connection_get_database_names);
				vconnection->set_method_extern("promise<bool>@ connect(host_address&in)", &VI_SPROMISIFY(mdb_connection_connect, type_id::bool_t));
				vconnection->set_method_extern("promise<bool>@ disconnect()", &VI_SPROMISIFY(mdb_connection_disconnect, type_id::bool_t));
				vconnection->set_method_extern("promise<bool>@ make_transaction(transaction_async@)", &VI_SPROMISIFY(mdb_connection_make_transaction, type_id::bool_t));
				vconnection->set_method_extern("promise<cursor>@ find_databases(const document&in)", &VI_SPROMISIFY_REF(mdb_connection_find_databases, cursor));

				vcluster->set_constructor<network::mongo::cluster>("cluster@ f()");
				vcluster->set_method("void set_profile(const string_view&in)", &network::mongo::cluster::set_profile);
				vcluster->set_method("host_address& get_address()", &network::mongo::cluster::get_address);
				vcluster->set_method("connection@+ pop()", &network::mongo::cluster::pop);
				vcluster->set_method_extern("void push(connection@+)", &mdb_cluster_push);
				vcluster->set_method_extern("promise<bool>@ connect(host_address&in)", &VI_SPROMISIFY(mdb_cluster_connect, type_id::bool_t));
				vcluster->set_method_extern("promise<bool>@ disconnect()", &VI_SPROMISIFY(mdb_cluster_disconnect, type_id::bool_t));

				auto vdriver = vm->set_class<network::mongo::driver>("driver", false);
				vdriver->set_function_def("void query_log_async(const string&in)");
				vdriver->set_constructor<network::mongo::driver>("driver@ f()");
				vdriver->set_method("void add_constant(const string_view&in, const string_view&in)", &network::mongo::driver::add_constant);
				vdriver->set_method_extern("bool add_query(const string_view&in, const string_view&in)", &VI_EXPECTIFY_VOID(network::mongo::driver::add_query));
				vdriver->set_method_extern("bool add_directory(const string_view&in, const string_view&in = string_view())", &VI_EXPECTIFY_VOID(network::mongo::driver::add_directory));
				vdriver->set_method("bool remove_constant(const string_view&in)", &network::mongo::driver::remove_constant);
				vdriver->set_method("bool remove_query(const string_view&in)", &network::mongo::driver::remove_query);
				vdriver->set_method("bool load_cache_dump(schema@+)", &network::mongo::driver::load_cache_dump);
				vdriver->set_method("schema@ get_cache_dump()", &network::mongo::driver::get_cache_dump);
				vdriver->set_method_extern("void set_query_log(query_log_async@)", &mdb_driver_set_query_log);
				vdriver->set_method_extern("string get_query(cluster@+, const string_view&in, dictionary@+)", &mdb_driver_get_query);
				vdriver->set_method_extern("array<string>@ get_queries()", &mdb_driver_get_queries);
				vdriver->set_method_static("driver@+ get()", &network::mongo::driver::get);

				vm->end_namespace();
				return true;
#else
				VI_ASSERT(false, "<mongodb> is not loaded");
				return false;
#endif
			}
			bool registry::import_vm(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				vm->begin_namespace("this_vm");
				vm->set_function("void collect_garbage(uint64)", &vm_collect_garbage);
				vm->end_namespace();

				return true;
#else
				VI_ASSERT(false, "<vm> is not loaded");
				return false;
#endif
			}
			bool registry::import_layer(virtual_machine* vm) noexcept
			{
#ifdef VI_BINDINGS
				VI_ASSERT(vm != nullptr, "manager should be set");
				VI_TYPEREF(content_manager, "content_manager");
				VI_TYPEREF(app_data, "app_data");
				VI_TYPEREF(application_name, "application");

				auto vapplication_use = vm->set_enum("application_use");
				vapplication_use->set_value("scripting", (int)layer::USE_SCRIPTING);
				vapplication_use->set_value("processing", (int)layer::USE_PROCESSING);
				vapplication_use->set_value("networking", (int)layer::USE_NETWORKING);

				auto vapplication_state = vm->set_enum("application_state");
				vapplication_state->set_value("terminated_t", (int)layer::application_state::terminated);
				vapplication_state->set_value("staging_t", (int)layer::application_state::staging);
				vapplication_state->set_value("active_t", (int)layer::application_state::active);
				vapplication_state->set_value("restart_t", (int)layer::application_state::restart);

				auto vasset_cache = vm->set_struct_trivial<layer::asset_cache>("asset_cache");
				vasset_cache->set_property<layer::asset_cache>("string path", &layer::asset_cache::path);
				vasset_cache->set_property<layer::asset_cache>("uptr@ resource", &layer::asset_cache::resource);
				vasset_cache->set_constructor<layer::asset_cache>("void f()");

				auto vasset_archive = vm->set_struct_trivial<layer::asset_archive>("asset_archive");
				vasset_archive->set_property<layer::asset_archive>("base_stream@ stream", &layer::asset_archive::stream);
				vasset_archive->set_property<layer::asset_archive>("string path", &layer::asset_archive::path);
				vasset_archive->set_property<layer::asset_archive>("usize length", &layer::asset_archive::length);
				vasset_archive->set_property<layer::asset_archive>("usize offset", &layer::asset_archive::offset);
				vasset_archive->set_constructor<layer::asset_archive>("void f()");

				auto vasset_file = vm->set_class<layer::asset_file>("asset_file", false);
				vasset_file->set_constructor<layer::asset_file, char*, size_t>("asset_file@ f(uptr@, usize)");
				vasset_file->set_method("uptr@ get_buffer() const", &layer::asset_file::get_buffer);
				vasset_file->set_method("usize get_size() const", &layer::asset_file::size);

				vm->begin_namespace("content_series");
				vm->set_function<void(core::schema*, bool)>("void pack(schema@+, bool)", &layer::series::pack);
				vm->set_function<void(core::schema*, int32_t)>("void pack(schema@+, int32)", &layer::series::pack);
				vm->set_function<void(core::schema*, int64_t)>("void pack(schema@+, int64)", &layer::series::pack);
				vm->set_function<void(core::schema*, uint32_t)>("void pack(schema@+, uint32)", &layer::series::pack);
				vm->set_function<void(core::schema*, uint64_t)>("void pack(schema@+, uint64)", &layer::series::pack);
				vm->set_function<void(core::schema*, float)>("void pack(schema@+, float)", &layer::series::pack);
				vm->set_function<void(core::schema*, double)>("void pack(schema@+, double)", &layer::series::pack);
				vm->set_function<void(core::schema*, const std::string_view&)>("void pack(schema@+, const string_view&in)", &layer::series::pack);
				vm->set_function<bool(core::schema*, bool*)>("bool unpack(schema@+, bool &out)", &layer::series::unpack);
				vm->set_function<bool(core::schema*, int32_t*)>("bool unpack(schema@+, int32 &out)", &layer::series::unpack);
				vm->set_function<bool(core::schema*, int64_t*)>("bool unpack(schema@+, int64 &out)", &layer::series::unpack);
				vm->set_function<bool(core::schema*, uint64_t*)>("bool unpack(schema@+, uint64 &out)", &layer::series::unpack);
				vm->set_function<bool(core::schema*, uint32_t*)>("bool unpack(schema@+, uint32 &out)", &layer::series::unpack);
				vm->set_function<bool(core::schema*, float*)>("bool unpack(schema@+, float &out)", &layer::series::unpack);
				vm->set_function<bool(core::schema*, double*)>("bool unpack(schema@+, double &out)", &layer::series::unpack);
				vm->set_function<bool(core::schema*, core::string*)>("bool unpack(schema@+, string &out)", &layer::series::unpack);
				vm->end_namespace();

				auto vcontent_manager = vm->set_class<layer::content_manager>("content_manager", true);
				auto vprocessor = vm->set_class<layer::processor>("base_processor", false);
				populate_processor_base<layer::processor>(*vprocessor);

				vcontent_manager->set_function_def("void load_result_async(uptr@)");
				vcontent_manager->set_function_def("void save_result_async(bool)");
				vcontent_manager->set_gc_constructor<layer::content_manager, content_manager>("content_manager@ f()");
				vcontent_manager->set_method("void clear_cache()", &layer::content_manager::clear_cache);
				vcontent_manager->set_method("void clear_archives()", &layer::content_manager::clear_archives);
				vcontent_manager->set_method("void clear_streams()", &layer::content_manager::clear_streams);
				vcontent_manager->set_method("void clear_processors()", &layer::content_manager::clear_processors);
				vcontent_manager->set_method("void clear_path(const string_view&in)", &layer::content_manager::clear_path);
				vcontent_manager->set_method("void set_environment(const string_view&in)", &layer::content_manager::set_environment);
				vcontent_manager->set_method_extern("uptr@ load(base_processor@+, const string_view&in, schema@+ = null)", &content_manager_load);
				vcontent_manager->set_method_extern("bool save(base_processor@+, const string_view&in, uptr@, schema@+ = null)", &content_manager_save);
				vcontent_manager->set_method_extern("void load_deferred(base_processor@+, const string_view&in, load_result_async@)", &content_manager_load_deferred1);
				vcontent_manager->set_method_extern("void load_deferred(base_processor@+, const string_view&in, schema@+, load_result_async@)", &content_manager_load_deferred2);
				vcontent_manager->set_method_extern("void save_deferred(base_processor@+, const string_view&in, uptr@, save_result_async@)", &content_manager_save_deferred1);
				vcontent_manager->set_method_extern("void save_deferred(base_processor@+, const string_view&in, uptr@, schema@+, save_result_async@)", &content_manager_save_deferred2);
				vcontent_manager->set_method_extern("bool find_cache_info(base_processor@+, asset_cache &out, const string_view&in)", &content_manager_find_cache1);
				vcontent_manager->set_method_extern("bool find_cache_info(base_processor@+, asset_cache &out, uptr@)", &content_manager_find_cache2);
				vcontent_manager->set_method<layer::content_manager, layer::asset_cache*, layer::processor*, const std::string_view&>("uptr@ find_cache(base_processor@+, const string_view&in)", &layer::content_manager::find_cache);
				vcontent_manager->set_method<layer::content_manager, layer::asset_cache*, layer::processor*, void*>("uptr@ find_cache(base_processor@+, uptr@)", &layer::content_manager::find_cache);
				vcontent_manager->set_method_extern("base_processor@+ add_processor(base_processor@+, uint64)", &content_manager_add_processor);
				vcontent_manager->set_method<layer::content_manager, layer::processor*, uint64_t>("base_processor@+ get_processor(uint64)", &layer::content_manager::get_processor);
				vcontent_manager->set_method<layer::content_manager, bool, uint64_t>("bool remove_processor(uint64)", &layer::content_manager::remove_processor);
				vcontent_manager->set_method_extern("bool import_archive(const string_view&in, bool)", &VI_EXPECTIFY_VOID(layer::content_manager::import_archive));
				vcontent_manager->set_method_extern("bool export_archive(const string_view&in, const string_view&in, const string_view&in = string_view())", &VI_EXPECTIFY_VOID(layer::content_manager::export_archive));
				vcontent_manager->set_method("uptr@ try_to_cache(base_processor@+, const string_view&in, uptr@)", &layer::content_manager::try_to_cache);
				vcontent_manager->set_method("bool is_busy() const", &layer::content_manager::is_busy);
				vcontent_manager->set_method("const string& get_environment() const", &layer::content_manager::get_environment);
				vcontent_manager->set_enum_refs_extern<layer::content_manager>([](layer::content_manager* base, asIScriptEngine* vm)
				{
					for (auto& item : base->get_processors())
						function_factory::gc_enum_callback(vm, item.second);
				});
				vcontent_manager->set_release_refs_extern<layer::content_manager>([](layer::content_manager* base, asIScriptEngine*)
				{
					base->clear_processors();
				});

				auto vapp_data = vm->set_class<layer::app_data>("app_data", true);
				vapp_data->set_gc_constructor<layer::app_data, app_data, layer::content_manager*, const std::string_view&>("app_data@ f(content_manager@+, const string_view&in)");
				vapp_data->set_method("void migrate(const string_view&in)", &layer::app_data::migrate);
				vapp_data->set_method("void set_key(const string_view&in, schema@+)", &layer::app_data::set_key);
				vapp_data->set_method("void set_text(const string_view&in, const string_view&in)", &layer::app_data::set_text);
				vapp_data->set_method("schema@ get_key(const string_view&in)", &layer::app_data::get_key);
				vapp_data->set_method("string get_text(const string_view&in)", &layer::app_data::get_text);
				vapp_data->set_method("bool has(const string_view&in)", &layer::app_data::has);
				vapp_data->set_enum_refs_extern<layer::app_data>([](layer::app_data* base, asIScriptEngine* vm) { });
				vapp_data->set_release_refs_extern<layer::app_data>([](layer::app_data* base, asIScriptEngine*) { });

				auto vapplication = vm->set_class<application>("application", true);
				auto vapplication_frame_info = vm->set_struct_trivial<application::desc::frames_info>("application_frame_info");
				vapplication_frame_info->set_property<application::desc::frames_info>("float stable", &application::desc::frames_info::stable);
				vapplication_frame_info->set_property<application::desc::frames_info>("float limit", &application::desc::frames_info::limit);
				vapplication_frame_info->set_constructor<application::desc::frames_info>("void f()");

				auto vapplication_desc = vm->set_struct_trivial<application::desc>("application_desc");
				vapplication_desc->set_property<application::desc>("application_frame_info refreshrate", &application::desc::refreshrate);
				vapplication_desc->set_property<application::desc>("schedule_policy scheduler", &application::desc::scheduler);
				vapplication_desc->set_property<application::desc>("string preferences", &application::desc::preferences);
				vapplication_desc->set_property<application::desc>("string environment", &application::desc::environment);
				vapplication_desc->set_property<application::desc>("string directory", &application::desc::directory);
				vapplication_desc->set_property<application::desc>("usize polling_timeout", &application::desc::polling_timeout);
				vapplication_desc->set_property<application::desc>("usize polling_events", &application::desc::polling_events);
				vapplication_desc->set_property<application::desc>("usize threads", &application::desc::threads);
				vapplication_desc->set_property<application::desc>("usize usage", &application::desc::usage);
				vapplication_desc->set_property<application::desc>("bool daemon", &application::desc::daemon);
				vapplication_desc->set_constructor<application::desc>("void f()");

				vapplication->set_function_def("void dispatch_sync(clock_timer@+)");
				vapplication->set_function_def("void publish_sync(clock_timer@+)");
				vapplication->set_function_def("void composition_sync()");
				vapplication->set_function_def("void script_hook_sync()");
				vapplication->set_function_def("void initialize_sync()");
				vapplication->set_function_def("void startup_sync()");
				vapplication->set_function_def("void shutdown_sync()");
				vapplication->set_property<layer::application>("content_manager@ content", &layer::application::content);
				vapplication->set_property<layer::application>("app_data@ database", &layer::application::database);
				vapplication->set_property<layer::application>("application_desc control", &layer::application::control);
				vapplication->set_gc_constructor<application, application_name, application::desc&, void*, int>("application@ f(application_desc &in, ?&in)");
				vapplication->set_method("void set_on_dispatch(dispatch_sync@)", &application::set_on_dispatch);
				vapplication->set_method("void set_on_publish(publish_sync@)", &application::set_on_publish);
				vapplication->set_method("void set_on_composition(composition_sync@)", &application::set_on_composition);
				vapplication->set_method("void set_on_script_hook(script_hook_sync@)", &application::set_on_script_hook);
				vapplication->set_method("void set_on_initialize(initialize_sync@)", &application::set_on_initialize);
				vapplication->set_method("void set_on_startup(startup_sync@)", &application::set_on_startup);
				vapplication->set_method("void set_on_shutdown(shutdown_sync@)", &application::set_on_shutdown);
				vapplication->set_method("int start()", &layer::application::start);
				vapplication->set_method("int restart()", &layer::application::restart);
				vapplication->set_method("void stop(int = 0)", &layer::application::stop);
				vapplication->set_method("application_state get_state() const", &layer::application::get_state);
				vapplication->set_method("uptr@ get_initiator() const", &application::get_initiator_object);
				vapplication->set_method("usize get_processed_events() const", &application::get_processed_events);
				vapplication->set_method("bool has_processed_events() const", &application::has_processed_events);
				vapplication->set_method("bool retrieve_initiator(?&out) const", &application::retrieve_initiator_object);
				vapplication->set_method_static("application@+ get()", &layer::application::get);
				vapplication->set_method_static("bool wants_restart(int)", &application::wants_restart);
				vapplication->set_enum_refs_extern<application>([](application* base, asIScriptEngine* vm)
				{
					function_factory::gc_enum_callback(vm, base->vm);
					function_factory::gc_enum_callback(vm, base->content);
					function_factory::gc_enum_callback(vm, base->database);
					function_factory::gc_enum_callback(vm, base->get_initiator_object());
					function_factory::gc_enum_callback(vm, &base->on_dispatch);
					function_factory::gc_enum_callback(vm, &base->on_publish);
					function_factory::gc_enum_callback(vm, &base->on_composition);
					function_factory::gc_enum_callback(vm, &base->on_script_hook);
					function_factory::gc_enum_callback(vm, &base->on_initialize);
					function_factory::gc_enum_callback(vm, &base->on_startup);
					function_factory::gc_enum_callback(vm, &base->on_shutdown);
				});
				vapplication->set_release_refs_extern<application>([](application* base, asIScriptEngine*)
				{
					base->~application();
				});

				return true;
#else
				VI_ASSERT(false, "<layer> is not loaded");
				return false;
#endif
			}
		}
	}
}