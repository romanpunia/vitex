#include "mongo.h"
#ifdef VI_MONGOC
#include <mongoc/mongoc.h>
#endif
#define mongo_execute_query(function, context, ...) execute_query(#function, function, context, ##__VA_ARGS__)
#define mongo_execute_cursor(function, context, ...) execute_cursor(#function, function, context, ##__VA_ARGS__)

namespace vitex
{
	namespace network
	{
		namespace mongo
		{
#ifdef VI_MONGOC
			typedef enum
			{
				BSON_FLAG_NONE = 0,
				BSON_FLAG_INLINE = (1 << 0),
				BSON_FLAG_STATIC = (1 << 1),
				BSON_FLAG_RDONLY = (1 << 2),
				BSON_FLAG_CHILD = (1 << 3),
				BSON_FLAG_IN_CHILD = (1 << 4),
				BSON_FLAG_NO_FREE = (1 << 5),
			} BSON_FLAG;

			template <typename r, typename t, typename... args>
			expects_db<void> execute_query(const char* name, r&& function, t* base, args&&... data)
			{
				VI_ASSERT(base != nullptr, "context should be set");
				VI_MEASURE(core::timings::intensive);
				VI_DEBUG("mongoc execute query schema on 0x%" PRIXPTR ": %s", (uintptr_t)base, name + 1);

				bson_error_t error;
				memset(&error, 0, sizeof(bson_error_t));

				auto time = core::schedule::get_clock();
				if (!function(base, data..., &error))
					return database_exception(error.code, error.message);

				VI_DEBUG("mongoc OK execute on 0x%" PRIXPTR " (%" PRIu64 " ms)", (uintptr_t)base, (uint64_t)((core::schedule::get_clock() - time).count() / 1000));
				(void)time;
				return core::expectation::met;
			}
			template <typename r, typename t, typename... args>
			expects_db<cursor> execute_cursor(const char* name, r&& function, t* base, args&&... data)
			{
				VI_ASSERT(base != nullptr, "context should be set");
				VI_MEASURE(core::timings::intensive);
				VI_DEBUG("mongoc execute query cursor on 0x%" PRIXPTR ": %s", (uintptr_t)base, name + 1);

				bson_error_t error;
				memset(&error, 0, sizeof(bson_error_t));

				auto time = core::schedule::get_clock();
				tcursor* result = function(base, data...);
				if (!result || mongoc_cursor_error(result, &error))
					return database_exception(error.code, error.message);

				VI_DEBUG("mongoc OK execute on 0x%" PRIXPTR " (%" PRIu64 " ms)", (uintptr_t)base, (uint64_t)((core::schedule::get_clock() - time).count() / 1000));
				(void)time;
				return cursor(result);
			}
#endif
			property::property() noexcept : source(nullptr), integer(0), high(0), low(0), number(0.0), boolean(false), mod(type::unknown), is_valid(false)
			{
			}
			property::property(property&& other) : name(std::move(other.name)), string(std::move(other.string)), source(other.source), integer(other.integer), high(other.high), low(other.low), number(other.number), boolean(other.boolean), mod(other.mod), is_valid(other.is_valid)
			{
				if (mod == type::object_id)
					memcpy(object_id, other.object_id, sizeof(object_id));

				other.source = nullptr;
			}
			property::~property()
			{
				document deleted(source);
				source = nullptr;
				name.clear();
				string.clear();
				integer = 0;
				number = 0;
				boolean = false;
				mod = type::unknown;
				is_valid = false;
			}
			property& property::operator =(const property& other)
			{
				if (&other == this)
					return *this;

				document deleted(source);
				name = std::move(other.name);
				string = std::move(other.string);
				mod = other.mod;
				integer = other.integer;
				high = other.high;
				low = other.low;
				number = other.number;
				boolean = other.boolean;
				is_valid = other.is_valid;
#ifdef VI_MONGOC
				if (other.source != nullptr)
					source = bson_copy(other.source);
				else
					source = nullptr;
#endif
				if (mod == type::object_id)
					memcpy(object_id, other.object_id, sizeof(object_id));

				return *this;
			}
			property& property::operator =(property&& other)
			{
				if (&other == this)
					return *this;

				document deleted(source);
				name = std::move(other.name);
				string = std::move(other.string);
				source = other.source;
				mod = other.mod;
				integer = other.integer;
				high = other.high;
				low = other.low;
				number = other.number;
				boolean = other.boolean;
				is_valid = other.is_valid;
				other.source = nullptr;

				if (mod == type::object_id)
					memcpy(object_id, other.object_id, sizeof(object_id));

				return *this;
			}
			tdocument* property::reset()
			{
				if (!source)
					return nullptr;

				tdocument* result = source;
				source = nullptr;
				return result;
			}
			core::string& property::to_string()
			{
				switch (mod)
				{
					case type::document:
						return string.assign("{}");
					case type::array:
						return string.assign("[]");
					case type::string:
						return string;
					case type::boolean:
						return string.assign(boolean ? "true" : "false");
					case type::number:
						return string.assign(core::to_string(number));
					case type::integer:
						return string.assign(core::to_string(integer));
					case type::object_id:
						return string.assign(compute::codec::bep45_encode(core::string((const char*)object_id, 12)));
					case type::null:
						return string.assign("null");
					case type::unknown:
					case type::uncastable:
						return string.assign("undefined");
					case type::decimal:
					{
#ifdef VI_MONGOC
						bson_decimal128_t decimal;
						decimal.high = (uint64_t)high;
						decimal.low = (uint64_t)low;

						char buffer[BSON_DECIMAL128_STRING];
						bson_decimal128_to_string(&decimal, buffer);
						return string.assign(buffer);
#else
						break;
#endif
					}
				}

				return string;
			}
			core::string property::to_object_id()
			{
				return utils::id_to_string(object_id);
			}
			document property::as_document() const
			{
				return document(source);
			}
			property property::at(const std::string_view& label) const
			{
				property result;
				as_document().get_property(label, &result);
				return result;
			}
			property property::operator [](const std::string_view& label)
			{
				property result;
				as_document().get_property(label, &result);
				return result;
			}
			property property::operator [](const std::string_view& label) const
			{
				property result;
				as_document().get_property(label, &result);
				return result;
			}

			database_exception::database_exception(int new_error_code, core::string&& new_message) : error_code(new_error_code)
			{
				error_message = std::move(new_message);
				if (error_code == 0)
					return;

				error_message += " (error = ";
				error_message += core::to_string(error_code);
				error_message += ")";
			}
			const char* database_exception::type() const noexcept
			{
				return "mongodb_error";
			}
			int database_exception::code() const noexcept
			{
				return error_code;
			}

			document::document() : base(nullptr), store(false)
			{
			}
			document::document(tdocument* new_base) : base(new_base), store(false)
			{
			}
			document::document(const document& other) : base(nullptr), store(false)
			{
#ifdef VI_MONGOC
				if (other.base)
					base = bson_copy(other.base);
#endif
			}
			document::document(document&& other) : base(other.base), store(other.store)
			{
				other.base = nullptr;
				other.store = false;
			}
			document::~document()
			{
				cleanup();
			}
			document& document::operator =(const document& other)
			{
#ifdef VI_MONGOC
				if (&other == this)
					return *this;

				cleanup();
				base = other.base ? bson_copy(other.base) : nullptr;
				store = false;
				return *this;
#else
				return *this;
#endif
			}
			document& document::operator =(document&& other)
			{
				if (&other == this)
					return *this;

				cleanup();
				base = other.base;
				store = other.store;
				other.base = nullptr;
				other.store = false;
				return *this;
			}
			void document::cleanup()
			{
#ifdef VI_MONGOC
				if (!base || store)
					return;

				if (!(base->flags & BSON_FLAG_STATIC) && !(base->flags & BSON_FLAG_RDONLY) && !(base->flags & BSON_FLAG_INLINE) && !(base->flags & BSON_FLAG_NO_FREE))
					bson_destroy((bson_t*)base);

				base = nullptr;
				store = false;
#endif
			}
			void document::join(const document& value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");
				VI_ASSERT(value.base != nullptr, "other schema should be set");
				bson_concat((bson_t*)base, (bson_t*)value.base);
#endif
			}
			void document::loop(const std::function<bool(property*)>& callback) const
			{
				VI_ASSERT(base != nullptr, "schema should be set");
				VI_ASSERT(callback, "callback should be set");
#ifdef VI_MONGOC
				bson_iter_t it;
				if (!bson_iter_init(&it, base))
					return;

				property output;
				while (bson_iter_next(&it))
				{
					clone(&it, &output);
					if (!callback(&output))
						break;
				}
#endif
			}
			bool document::set_schema(const std::string_view& key, const document& value, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");
				VI_ASSERT(value.base != nullptr, "other schema should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				return bson_append_document((bson_t*)base, value_key.data(), (int)value_key.size(), (bson_t*)value.base);
#else
				return false;
#endif
			}
			bool document::set_array(const std::string_view& key, const document& value, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");
				VI_ASSERT(value.base != nullptr, "other schema should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				return bson_append_array((bson_t*)base, value_key.data(), (int)value_key.size(), (bson_t*)value.base);
#else
				return false;
#endif
			}
			bool document::set_string(const std::string_view& key, const std::string_view& value, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				return bson_append_utf8((bson_t*)base, value_key.data(), (int)value_key.size(), value.data(), (int)value.size());
#else
				return false;
#endif
			}
			bool document::set_integer(const std::string_view& key, int64_t value, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				return bson_append_int64((bson_t*)base, value_key.data(), (int)value_key.size(), value);
#else
				return false;
#endif
			}
			bool document::set_number(const std::string_view& key, double value, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				return bson_append_double((bson_t*)base, value_key.data(), (int)value_key.size(), value);
#else
				return false;
#endif
			}
			bool document::set_decimal(const std::string_view& key, uint64_t high, uint64_t low, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				bson_decimal128_t decimal;
				decimal.high = (uint64_t)high;
				decimal.low = (uint64_t)low;

				return bson_append_decimal128((bson_t*)base, value_key.data(), (int)value_key.size(), &decimal);
#else
				return false;
#endif
			}
			bool document::set_decimal_string(const std::string_view& key, const std::string_view& value, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");
				VI_ASSERT(core::stringify::is_cstring(value), "value should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				bson_decimal128_t decimal;
				bson_decimal128_from_string(value.data(), &decimal);

				return bson_append_decimal128((bson_t*)base, value_key.data(), (int)value_key.size(), &decimal);
#else
				return false;
#endif
			}
			bool document::set_decimal_integer(const std::string_view& key, int64_t value, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				char data[64];
				snprintf(data, 64, "%" PRId64 "", value);

				bson_decimal128_t decimal;
				bson_decimal128_from_string(data, &decimal);

				return bson_append_decimal128((bson_t*)base, value_key.data(), (int)value_key.size(), &decimal);
#else
				return false;
#endif
			}
			bool document::set_decimal_number(const std::string_view& key, double value, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				char data[64];
				snprintf(data, 64, "%f", value);

				for (size_t i = 0; i < 64; i++)
					data[i] = (data[i] == ',' ? '.' : data[i]);

				bson_decimal128_t decimal;
				bson_decimal128_from_string(data, &decimal);

				return bson_append_decimal128((bson_t*)base, value_key.data(), (int)value_key.size(), &decimal);
#else
				return false;
#endif
			}
			bool document::set_boolean(const std::string_view& key, bool value, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				return bson_append_bool((bson_t*)base, value_key.data(), (int)value_key.size(), value);
#else
				return false;
#endif
			}
			bool document::set_object_id(const std::string_view& key, uint8_t value[12], size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");

				bson_oid_t object_id;
				memcpy(object_id.bytes, value, sizeof(uint8_t) * 12);

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				return bson_append_oid((bson_t*)base, value_key.data(), (int)value_key.size(), &object_id);
#else
				return false;
#endif
			}
			bool document::set_null(const std::string_view& key, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				return bson_append_null((bson_t*)base, value_key.data(), (int)value_key.size());
#else
				return false;
#endif
			}
			bool document::set_property(const std::string_view& key, property* value, size_t array_id)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");
				VI_ASSERT(value != nullptr, "property should be set");

				char index[16];
				std::string_view value_key = key;
				if (value_key.empty())
				{
					const char* index_key = index;
					bson_uint32_to_string((uint32_t)array_id, &index_key, index, sizeof(index));
					value_key = index_key;
				}

				switch (value->mod)
				{
					case type::document:
						return set_schema(key, value->as_document().copy());
					case type::array:
						return set_array(key, value->as_document().copy());
					case type::string:
						return set_string(key, value->string.c_str());
					case type::boolean:
						return set_boolean(key, value->boolean);
					case type::number:
						return set_number(key, value->number);
					case type::integer:
						return set_integer(key, value->integer);
					case type::decimal:
						return set_decimal(key, value->high, value->low);
					case type::object_id:
						return set_object_id(key, value->object_id);
					default:
						return false;
				}
#else
				return false;
#endif
			}
			bool document::has_property(const std::string_view& key) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");
				VI_ASSERT(core::stringify::is_cstring(key), "key should be set");
				return bson_has_field(base, key.data());
#else
				return false;
#endif
			}
			bool document::get_property(const std::string_view& key, property* output) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");
				VI_ASSERT(core::stringify::is_cstring(key), "key should be set");
				VI_ASSERT(output != nullptr, "property should be set");

				bson_iter_t it;
				if (!bson_iter_init_find(&it, base, key.data()))
					return false;

				return clone(&it, output);
#else
				return false;
#endif
			}
			bool document::clone(void* it, property* output)
			{
#ifdef VI_MONGOC
				VI_ASSERT(it != nullptr, "iterator should be set");
				VI_ASSERT(output != nullptr, "property should be set");

				const bson_value_t* value = bson_iter_value((bson_iter_t*)it);
				if (!value)
					return false;

				property init;
				switch (value->value_type)
				{
					case BSON_TYPE_DOCUMENT:
					{
						const uint8_t* buffer; uint32_t length;
						bson_iter_document((bson_iter_t*)it, &length, &buffer);
						init.mod = type::document;
						init.source = bson_new_from_data(buffer, length);
						break;
					}
					case BSON_TYPE_ARRAY:
					{
						const uint8_t* buffer; uint32_t length;
						bson_iter_array((bson_iter_t*)it, &length, &buffer);
						init.mod = type::array;
						init.source = bson_new_from_data(buffer, length);
						break;
					}
					case BSON_TYPE_BOOL:
						init.mod = type::boolean;
						init.boolean = value->value.v_bool;
						break;
					case BSON_TYPE_INT32:
						init.mod = type::integer;
						init.integer = value->value.v_int32;
						break;
					case BSON_TYPE_INT64:
						init.mod = type::integer;
						init.integer = value->value.v_int64;
						break;
					case BSON_TYPE_DOUBLE:
						init.mod = type::number;
						init.number = value->value.v_double;
						break;
					case BSON_TYPE_DECIMAL128:
						init.mod = type::decimal;
						init.high = (uint64_t)value->value.v_decimal128.high;
						init.low = (uint64_t)value->value.v_decimal128.low;
						break;
					case BSON_TYPE_UTF8:
						init.mod = type::string;
						init.string.assign(value->value.v_utf8.str, (uint64_t)value->value.v_utf8.len);
						break;
					case BSON_TYPE_TIMESTAMP:
						init.mod = type::integer;
						init.integer = (int64_t)value->value.v_timestamp.timestamp;
						init.number = (double)value->value.v_timestamp.increment;
						break;
					case BSON_TYPE_DATE_TIME:
						init.mod = type::integer;
						init.integer = value->value.v_datetime;
						break;
					case BSON_TYPE_REGEX:
						init.mod = type::string;
						init.string.assign(value->value.v_regex.regex).append(1, '\n').append(value->value.v_regex.options);
						break;
					case BSON_TYPE_CODE:
						init.mod = type::string;
						init.string.assign(value->value.v_code.code, (uint64_t)value->value.v_code.code_len);
						break;
					case BSON_TYPE_SYMBOL:
						init.mod = type::string;
						init.string.assign(value->value.v_symbol.symbol, (uint64_t)value->value.v_symbol.len);
						break;
					case BSON_TYPE_CODEWSCOPE:
						init.mod = type::string;
						init.string.assign(value->value.v_codewscope.code, (uint64_t)value->value.v_codewscope.code_len);
						break;
					case BSON_TYPE_UNDEFINED:
					case BSON_TYPE_NULL:
						init.mod = type::null;
						break;
					case BSON_TYPE_OID:
						init.mod = type::object_id;
						memcpy(init.object_id, value->value.v_oid.bytes, sizeof(uint8_t) * 12);
						break;
					case BSON_TYPE_EOD:
					case BSON_TYPE_BINARY:
					case BSON_TYPE_DBPOINTER:
					case BSON_TYPE_MAXKEY:
					case BSON_TYPE_MINKEY:
						init.mod = type::uncastable;
						break;
					default:
						break;
				}

				init.name.assign(bson_iter_key((const bson_iter_t*)it));
				init.is_valid = true;
				*output = std::move(init);

				return true;
#else
				return false;
#endif
			}
			size_t document::count() const
			{
#ifdef VI_MONGOC
				if (!base)
					return 0;

				return bson_count_keys(base);
#else
				return 0;
#endif
			}
			core::string document::to_relaxed_json() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");
				size_t length = 0;
				char* value = bson_as_relaxed_extended_json(base, &length);

				core::string output;
				output.assign(value, (uint64_t)length);
				bson_free(value);

				return output;
#else
				return "";
#endif
			}
			core::string document::to_extended_json() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");
				size_t length = 0;
				char* value = bson_as_canonical_extended_json(base, &length);

				core::string output;
				output.assign(value, (uint64_t)length);
				bson_free(value);

				return output;
#else
				return "";
#endif
			}
			core::string document::to_json() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");
				size_t length = 0;
				char* value = bson_as_json(base, &length);

				core::string output;
				output.assign(value, (uint64_t)length);
				bson_free(value);

				return output;
#else
				return "";
#endif
			}
			core::string document::to_indices() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "schema should be set");
				char* value = mongoc_collection_keys_to_index_string(base);
				if (!value)
					return core::string();

				core::string output(value);
				bson_free(value);

				return output;
#else
				return "";
#endif
			}
			core::schema* document::to_schema(bool is_array) const
			{
#ifdef VI_MONGOC
				if (!base)
					return nullptr;

				core::schema* node = (is_array ? core::var::set::array() : core::var::set::object());
				loop([node](property* key) -> bool
				{
					core::string name = (node->value.get_type() == core::var_type::array ? "" : key->name);
					switch (key->mod)
					{
						case type::document:
						{
							node->set(name, key->as_document().to_schema(false));
							break;
						}
						case type::array:
						{
							node->set(name, key->as_document().to_schema(true));
							break;
						}
						case type::string:
							node->set(name, core::var::string(key->string));
							break;
						case type::boolean:
							node->set(name, core::var::boolean(key->boolean));
							break;
						case type::number:
							node->set(name, core::var::number(key->number));
							break;
						case type::decimal:
							node->set(name, core::var::decimal_string(key->to_string()));
							break;
						case type::integer:
							node->set(name, core::var::integer(key->integer));
							break;
						case type::object_id:
							node->set(name, core::var::binary(key->object_id, 12));
							break;
						case type::null:
							node->set(name, core::var::null());
							break;
						default:
							break;
					}

					return true;
				});

				return node;
#else
				return nullptr;
#endif
			}
			tdocument* document::get() const
			{
#ifdef VI_MONGOC
				return base;
#else
				return nullptr;
#endif
			}
			document document::copy() const
			{
#ifdef VI_MONGOC
				if (!base)
					return nullptr;

				return document(bson_copy(base));
#else
				return nullptr;
#endif
			}
			document& document::persist(bool keep)
			{
				store = keep;
				return *this;
			}
			property document::at(const std::string_view& name) const
			{
				property result;
				get_property(name, &result);
				return result;
			}
			document document::from_schema(core::schema* src)
			{
#ifdef VI_MONGOC
				VI_ASSERT(src != nullptr && src->value.is_object(), "schema should be set");
				bool array = (src->value.get_type() == core::var_type::array);
				document result = bson_new();
				size_t index = 0;

				for (auto&& node : src->get_childs())
				{
					switch (node->value.get_type())
					{
						case core::var_type::object:
							result.set_schema(array ? std::string_view() : node->key.c_str(), document::from_schema(node), index);
							break;
						case core::var_type::array:
							result.set_array(array ? std::string_view() : node->key.c_str(), document::from_schema(node), index);
							break;
						case core::var_type::string:
							result.set_string(array ? std::string_view() : node->key.c_str(), node->value.get_string(), index);
							break;
						case core::var_type::boolean:
							result.set_boolean(array ? std::string_view() : node->key.c_str(), node->value.get_boolean(), index);
							break;
						case core::var_type::decimal:
							result.set_decimal_string(array ? std::string_view() : node->key.c_str(), node->value.get_decimal().to_string(), index);
							break;
						case core::var_type::number:
							result.set_number(array ? std::string_view() : node->key.c_str(), node->value.get_number(), index);
							break;
						case core::var_type::integer:
							result.set_integer(array ? std::string_view() : node->key.c_str(), node->value.get_integer(), index);
							break;
						case core::var_type::null:
							result.set_null(array ? std::string_view() : node->key.c_str(), index);
							break;
						case core::var_type::binary:
						{
							if (node->value.size() != 12)
							{
								core::string base = compute::codec::bep45_encode(node->value.get_blob());
								result.set_string(array ? std::string_view() : node->key.c_str(), base, index);
							}
							else
								result.set_object_id(array ? std::string_view() : node->key.c_str(), node->value.get_binary(), index);
							break;
						}
						default:
							break;
					}
					index++;
				}

				return result;
#else
				return nullptr;
#endif
			}
			document document::from_empty()
			{
#ifdef VI_MONGOC
				return bson_new();
#else
				return nullptr;
#endif
			}
			expects_db<document> document::from_json(const std::string_view& JSON)
			{
#ifdef VI_MONGOC
				bson_error_t error;
				memset(&error, 0, sizeof(bson_error_t));

				tdocument* result = bson_new_from_json((uint8_t*)JSON.data(), (ssize_t)JSON.size(), &error);
				if (result != nullptr && error.code == 0)
					return document(result);
				else if (result != nullptr)
					bson_destroy((bson_t*)result);

				return database_exception(error.code, error.message);
#else
				return database_exception(0, "not supported");
#endif
			}
			document document::from_buffer(const uint8_t* buffer, size_t length)
			{
#ifdef VI_MONGOC
				VI_ASSERT(buffer != nullptr, "buffer should be set");
				return bson_new_from_data(buffer, length);
#else
				return nullptr;
#endif
			}
			document document::from_source(tdocument* src)
			{
#ifdef VI_MONGOC
				VI_ASSERT(src != nullptr, "src should be set");
				tdocument* dest = bson_new();
				bson_steal((bson_t*)dest, (bson_t*)src);
				return dest;
#else
				return nullptr;
#endif
			}

			address::address() : base(nullptr)
			{
			}
			address::address(taddress* new_base) : base(new_base)
			{
			}
			address::address(const address& other) : address(std::move(*(address*)&other))
			{
			}
			address::address(address&& other) : base(other.base)
			{
				other.base = nullptr;
			}
			address::~address()
			{
#ifdef VI_MONGOC
				if (base != nullptr)
					mongoc_uri_destroy(base);
				base = nullptr;
#endif
			}
			address& address::operator =(const address& other)
			{
				return *this = std::move(*(address*)&other);
			}
			address& address::operator =(address&& other)
			{
				if (&other == this)
					return *this;
#ifdef VI_MONGOC
				if (base != nullptr)
					mongoc_uri_destroy(base);
#endif
				base = other.base;
				other.base = nullptr;
				return *this;
			}
			void address::set_option(const std::string_view& name, int64_t value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				mongoc_uri_set_option_as_int32(base, name.data(), (int32_t)value);
#endif
			}
			void address::set_option(const std::string_view& name, bool value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				mongoc_uri_set_option_as_bool(base, name.data(), value);
#endif
			}
			void address::set_option(const std::string_view& name, const std::string_view& value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				VI_ASSERT(core::stringify::is_cstring(value), "value should be set");
				mongoc_uri_set_option_as_utf8(base, name.data(), value.data());
#endif
			}
			void address::set_auth_mechanism(const std::string_view& value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(value), "value should be set");
				mongoc_uri_set_auth_mechanism(base, value.data());
#endif
			}
			void address::set_auth_source(const std::string_view& value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(value), "value should be set");
				mongoc_uri_set_auth_source(base, value.data());
#endif
			}
			void address::set_compressors(const std::string_view& value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(value), "value should be set");
				mongoc_uri_set_compressors(base, value.data());
#endif
			}
			void address::set_database(const std::string_view& value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(value), "value should be set");
				mongoc_uri_set_database(base, value.data());
#endif
			}
			void address::set_username(const std::string_view& value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(value), "value should be set");
				mongoc_uri_set_username(base, value.data());
#endif
			}
			void address::set_password(const std::string_view& value)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(value), "value should be set");
				mongoc_uri_set_password(base, value.data());
#endif
			}
			taddress* address::get() const
			{
#ifdef VI_MONGOC
				return base;
#else
				return nullptr;
#endif
			}
			expects_db<address> address::from_url(const std::string_view& location)
			{
#ifdef VI_MONGOC
				VI_ASSERT(core::stringify::is_cstring(location), "location should be set");
				taddress* result = mongoc_uri_new(location.data());
				if (!strstr(location.data(), MONGOC_URI_SOCKETTIMEOUTMS))
					mongoc_uri_set_option_as_int32(result, MONGOC_URI_SOCKETTIMEOUTMS, 10000);

				return address(result);
#else
				return database_exception(0, "not supported");
#endif
			}

			stream::stream() : net_options(nullptr), source(nullptr), base(nullptr), count(0)
			{
			}
			stream::stream(tcollection* new_source, tstream* new_base, document&& new_options) : net_options(std::move(new_options)), source(new_source), base(new_base), count(0)
			{
			}
			stream::stream(const stream& other) : stream(std::move(*(stream*)&other))
			{
			}
			stream::stream(stream&& other) : net_options(std::move(other.net_options)), source(other.source), base(other.base), count(other.count)
			{
				other.source = nullptr;
				other.base = nullptr;
				other.count = 0;
			}
			stream::~stream()
			{
#ifdef VI_MONGOC
				if (base != nullptr)
					mongoc_bulk_operation_destroy(base);

				source = nullptr;
				base = nullptr;
				count = 0;
#endif
			}
			stream& stream::operator =(const stream& other)
			{
				return *this = std::move(*(stream*)&other);
			}
			stream& stream::operator =(stream&& other)
			{
				if (&other == this)
					return *this;

#ifdef VI_MONGOC
				if (base != nullptr)
					mongoc_bulk_operation_destroy(base);
#endif
				net_options = std::move(other.net_options);
				source = other.source;
				base = other.base;
				count = other.count;
				other.source = nullptr;
				other.base = nullptr;
				other.count = 0;
				return *this;
			}
			expects_db<void> stream::remove_many(const document& match, const document& options)
			{
#ifdef VI_MONGOC
				auto status = next_operation();
				if (!status)
					return status;

				return mongo_execute_query(&mongoc_bulk_operation_remove_many_with_opts, base, match.get(), options.get());
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_db<void> stream::remove_one(const document& match, const document& options)
			{
#ifdef VI_MONGOC
				auto status = next_operation();
				if (!status)
					return status;

				return mongo_execute_query(&mongoc_bulk_operation_remove_one_with_opts, base, match.get(), options.get());
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_db<void> stream::replace_one(const document& match, const document& replacement, const document& options)
			{
#ifdef VI_MONGOC
				auto status = next_operation();
				if (!status)
					return status;

				return mongo_execute_query(&mongoc_bulk_operation_replace_one_with_opts, base, match.get(), replacement.get(), options.get());
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_db<void> stream::insert_one(const document& result, const document& options)
			{
#ifdef VI_MONGOC
				auto status = next_operation();
				if (!status)
					return status;

				return mongo_execute_query(&mongoc_bulk_operation_insert_with_opts, base, result.get(), options.get());
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_db<void> stream::update_one(const document& match, const document& result, const document& options)
			{
#ifdef VI_MONGOC
				auto status = next_operation();
				if (!status)
					return status;

				return mongo_execute_query(&mongoc_bulk_operation_update_one_with_opts, base, match.get(), result.get(), options.get());
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_db<void> stream::update_many(const document& match, const document& result, const document& options)
			{
#ifdef VI_MONGOC
				auto status = next_operation();
				if (!status)
					return status;

				return mongo_execute_query(&mongoc_bulk_operation_update_many_with_opts, base, match.get(), result.get(), options.get());
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_db<void> stream::template_query(const std::string_view& name, core::schema_args* map)
			{
				VI_DEBUG("mongoc template query %s", name.empty() ? "empty-query-name" : core::string(name).c_str());
				auto pattern = driver::get()->get_query(name, map);
				if (!pattern)
					return pattern.error();

				return query(*pattern);
			}
			expects_db<void> stream::query(const document& command)
			{
#ifdef VI_MONGOC
				if (!command.get())
					return database_exception(0, "execute query: empty command");

				property type;
				if (!command.get_property("type", &type) || type.mod != type::string)
					return database_exception(0, "execute query: no type field");

				if (type.string == "update")
				{
					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return database_exception(0, "execute query: no match field");

					property update;
					if (!command.get_property("update", &update) || update.mod != type::document)
						return database_exception(0, "execute query: no update field");

					property options = command["options"];
					return update_one(match.reset(), update.reset(), options.reset());
				}
				else if (type.string == "update-many")
				{
					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return database_exception(0, "execute query: no match field");

					property update;
					if (!command.get_property("update", &update) || update.mod != type::document)
						return database_exception(0, "execute query: no update field");

					property options = command["options"];
					return update_many(match.reset(), update.reset(), options.reset());
				}
				else if (type.string == "insert")
				{
					property value;
					if (!command.get_property("value", &value) || value.mod != type::document)
						return database_exception(0, "execute query: no value field");

					property options = command["options"];
					return insert_one(value.reset(), options.reset());
				}
				else if (type.string == "replace")
				{
					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return database_exception(0, "execute query: no match field");

					property value;
					if (!command.get_property("value", &value) || value.mod != type::document)
						return database_exception(0, "execute query: no value field");

					property options = command["options"];
					return replace_one(match.reset(), value.reset(), options.reset());
				}
				else if (type.string == "remove")
				{
					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return database_exception(0, "execute query: no match field");

					property options = command["options"];
					return remove_one(match.reset(), options.reset());
				}
				else if (type.string == "remove-many")
				{
					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return database_exception(0, "execute query: no match field");

					property options = command["options"];
					return remove_many(match.reset(), options.reset());
				}

				return database_exception(0, "invalid query type: " + type.string);
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_db<void> stream::next_operation()
			{
#ifdef VI_MONGOC
				if (!base || !source)
					return database_exception(0, "invalid operation");

				if (count++ <= 768)
					return core::expectation::met;

				tdocument result;
				auto status = mongo_execute_query(&mongoc_bulk_operation_execute, base, &result);
				bson_destroy((bson_t*)&result);
				if (source != nullptr)
				{
					auto subresult = collection(source).create_stream(net_options);
					if (subresult)
						*this = std::move(*subresult);
				}
				return status;
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_promise_db<document> stream::execute_with_reply()
			{
#ifdef VI_MONGOC
				if (!base)
					return expects_promise_db<document>(database_exception(0, "invalid operation"));

				auto* context = base;
				return core::cotask<expects_db<document>>([context]() mutable -> expects_db<document>
				{
					tdocument subresult;
					auto status = mongo_execute_query(&mongoc_bulk_operation_execute, context, &subresult);
					if (!status)
					{
						bson_destroy((bson_t*)&subresult);
						return status.error();
					}

					document result = document::from_source(&subresult);
					bson_destroy((bson_t*)&subresult);
					return result;
				});
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> stream::execute()
			{
#ifdef VI_MONGOC
				if (!base)
					return expects_promise_db<void>(database_exception(0, "invalid operation"));

				auto* context = base;
				return core::cotask<expects_db<void>>([context]() mutable -> expects_db<void>
				{
					tdocument result;
					auto status = mongo_execute_query(&mongoc_bulk_operation_execute, context, &result);
					bson_destroy((bson_t*)&result);
					return status;
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			size_t stream::get_hint() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
#if MONGOC_CHECK_VERSION(1, 28, 0)
				return (size_t)mongoc_bulk_operation_get_server_id(base);
#else
				return (size_t)mongoc_bulk_operation_get_hint(base);
#endif
#else
				return 0;
#endif
			}
			tstream* stream::get() const
			{
#ifdef VI_MONGOC
				return base;
#else
				return nullptr;
#endif
			}

			cursor::cursor() : base(nullptr)
			{
			}
			cursor::cursor(tcursor* new_base) : base(new_base)
			{
			}
			cursor::cursor(cursor&& other) : base(other.base)
			{
				other.base = nullptr;
			}
			cursor::~cursor()
			{
				cleanup();
			}
			cursor& cursor::operator =(cursor&& other)
			{
				if (&other == this)
					return *this;

				cleanup();
				base = other.base;
				other.base = nullptr;
				return *this;
			}
			void cursor::cleanup()
			{
#ifdef VI_MONGOC
				if (!base)
					return;

				mongoc_cursor_destroy(base);
				base = nullptr;
#endif
			}
			void cursor::set_max_await_time(size_t max_await_time)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				mongoc_cursor_set_max_await_time_ms(base, (uint32_t)max_await_time);
#endif
			}
			void cursor::set_batch_size(size_t batch_size)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				mongoc_cursor_set_batch_size(base, (uint32_t)batch_size);
#endif
			}
			bool cursor::set_limit(int64_t limit)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				return mongoc_cursor_set_limit(base, limit);
#else
				return false;
#endif
			}
			bool cursor::set_hint(size_t hint)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
#if MONGOC_CHECK_VERSION(1, 28, 0)
				return mongoc_cursor_set_server_id(base, (uint32_t)hint);
#else
				return mongoc_cursor_set_hint(base, (uint32_t)hint);
#endif
#else
				return false;
#endif
			}
			bool cursor::empty() const
			{
#ifdef VI_MONGOC
				if (!base)
					return true;

				return !mongoc_cursor_more(base);
#else
				return true;
#endif
			}
			core::option<database_exception> cursor::error() const
			{
#ifdef VI_MONGOC
				if (!base)
					return database_exception(0, "invalid operation");

				bson_error_t error;
				memset(&error, 0, sizeof(bson_error_t));
				if (!mongoc_cursor_error(base, &error))
					return core::optional::none;

				return database_exception(error.code, error.message);
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_promise_db<void> cursor::next()
			{
#ifdef VI_MONGOC
				if (!base)
					return expects_promise_db<void>(database_exception(0, "invalid operation"));

				auto* context = base;
				return core::cotask<expects_db<void>>([context]() -> expects_db<void>
				{
					VI_MEASURE(core::timings::intensive);
					tdocument* query = nullptr;
					if (mongoc_cursor_next(context, (const tdocument**)&query))
						return core::expectation::met;

					bson_error_t error;
					memset(&error, 0, sizeof(bson_error_t));
					if (!mongoc_cursor_error(context, &error))
						return database_exception(0, "end of stream");

					return database_exception(error.code, error.message);
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			int64_t cursor::get_id() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				return (int64_t)mongoc_cursor_get_id(base);
#else
				return 0;
#endif
			}
			int64_t cursor::get_limit() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				return (int64_t)mongoc_cursor_get_limit(base);
#else
				return 0;
#endif
			}
			size_t cursor::get_max_await_time() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				return (size_t)mongoc_cursor_get_max_await_time_ms(base);
#else
				return 0;
#endif
			}
			size_t cursor::get_batch_size() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				return (size_t)mongoc_cursor_get_batch_size(base);
#else
				return 0;
#endif
			}
			size_t cursor::get_hint() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
#if MONGOC_CHECK_VERSION(1, 28, 0)
				return (size_t)mongoc_cursor_get_server_id(base);
#else
				return (size_t)mongoc_cursor_get_hint(base);
#endif
#else
				return 0;
#endif
			}
			document cursor::current() const
			{
#ifdef VI_MONGOC
				if (!base)
					return nullptr;

				return (tdocument*)mongoc_cursor_current(base);
#else
				return nullptr;
#endif
			}
			cursor cursor::clone()
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				return mongoc_cursor_clone(base);
#else
				return nullptr;
#endif
			}
			tcursor* cursor::get() const
			{
#ifdef VI_MONGOC
				return base;
#else
				return nullptr;
#endif
			}

			response::response() : net_success(false)
			{
			}
			response::response(bool new_success) : net_success(new_success)
			{
			}
			response::response(cursor& new_cursor) : net_cursor(std::move(new_cursor))
			{
				net_success = net_cursor && !net_cursor.error_or_empty();
			}
			response::response(document& new_document) : net_document(std::move(new_document))
			{
				net_success = net_document.get() != nullptr;
			}
			response::response(response&& other) : net_cursor(std::move(other.net_cursor)), net_document(std::move(other.net_document)), net_success(other.net_success)
			{
				other.net_success = false;
			}
			response& response::operator =(response&& other)
			{
				if (&other == this)
					return *this;

				net_cursor = std::move(other.net_cursor);
				net_document = std::move(other.net_document);
				net_success = other.net_success;
				other.net_success = false;
				return *this;
			}
			expects_promise_db<core::schema*> response::fetch()
			{
				if (net_document)
					return expects_promise_db<core::schema*>(net_document.to_schema());

				if (!net_cursor)
					return expects_promise_db<core::schema*>(database_exception(0, "fetch: no cursor"));

				return net_cursor.next().then<expects_db<core::schema*>>([this](expects_db<void>&& result) -> expects_db<core::schema*>
				{
					if (!result)
						return result.error();

					return net_cursor.current().to_schema();
				});
			}
			expects_promise_db<core::schema*> response::fetch_all()
			{
				if (net_document)
				{
					core::schema* result = net_document.to_schema();
					return expects_promise_db<core::schema*>(result ? result : core::var::set::array());
				}

				if (!net_cursor)
					return expects_promise_db<core::schema*>(core::var::set::array());

				return core::coasync<expects_db<core::schema*>>([this]() -> expects_promise_db<core::schema*>
				{
					core::uptr<core::schema> result = core::var::set::array();
					while (true)
					{
						auto status = VI_AWAIT(net_cursor.next());
						if (status)
						{
							result->push(net_cursor.current().to_schema());
							continue;
						}
						else if (status.error().code() != 0 && result->empty())
							coreturn expects_db<core::schema*>(std::move(status.error()));
						break;
					}
					coreturn expects_db<core::schema*>(result.reset());
				});
			}
			expects_promise_db<property> response::get_property(const std::string_view& name)
			{
				if (net_document)
				{
					property result;
					net_document.get_property(name, &result);
					return expects_promise_db<property>(std::move(result));
				}

				if (!net_cursor)
					return expects_promise_db<property>(property());

				document source = net_cursor.current();
				if (source)
				{
					property result;
					source.get_property(name, &result);
					return expects_promise_db<property>(std::move(result));
				}

				core::string copy = core::string(name);
				return net_cursor.next().then<expects_db<property>>([this, copy](expects_db<void>&& status) mutable -> expects_db<property>
				{
					property result;
					if (!status)
						return status.error();

					document source = net_cursor.current();
					if (!source)
						return database_exception(0, "property not found: " + copy);

					source.get_property(copy, &result);
					return result;
				});
			}
			cursor&& response::get_cursor()
			{
				return std::move(net_cursor);
			}
			document&& response::get_document()
			{
				return std::move(net_document);
			}
			bool response::success() const
			{
				return net_success;
			}

			transaction::transaction() : base(nullptr)
			{
			}
			transaction::transaction(ttransaction* new_base) : base(new_base)
			{
			}
			expects_db<void> transaction::push(const document& query_options_wrapper)
			{
#ifdef VI_MONGOC
				document& query_options = *(document*)&query_options_wrapper;
				if (!query_options.get())
					query_options = bson_new();

				bson_error_t error;
				memset(&error, 0, sizeof(bson_error_t));
				if (!mongoc_client_session_append(base, (bson_t*)query_options.get(), &error) && error.code != 0)
					return database_exception(error.code, error.message);

				return core::expectation::met;
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_db<void> transaction::put(tdocument** query_options) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(query_options != nullptr, "query options should be set");
				if (!*query_options)
					*query_options = bson_new();

				bson_error_t error;
				memset(&error, 0, sizeof(bson_error_t));
				if (!mongoc_client_session_append(base, (bson_t*)*query_options, &error) && error.code != 0)
					return database_exception(error.code, error.message);

				return core::expectation::met;
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_promise_db<void> transaction::begin()
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<void>>([context]()
				{
					return mongo_execute_query(&mongoc_client_session_start_transaction, context, nullptr);
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> transaction::rollback()
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<void>>([context]()
				{
					return mongo_execute_query(&mongoc_client_session_abort_transaction, context);
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> transaction::remove_many(collection& source, const document& match, const document& options)
			{
#ifdef VI_MONGOC
				if (!push(options))
					return expects_promise_db<document>(database_exception(0, "invalid operation"));

				return source.remove_many(match, options);
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> transaction::remove_one(collection& source, const document& match, const document& options)
			{
#ifdef VI_MONGOC
				if (!push(options))
					return expects_promise_db<document>(database_exception(0, "invalid operation"));

				return source.remove_one(match, options);
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> transaction::replace_one(collection& source, const document& match, const document& replacement, const document& options)
			{
#ifdef VI_MONGOC
				if (!push(options))
					return expects_promise_db<document>(database_exception(0, "invalid operation"));

				return source.replace_one(match, replacement, options);
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> transaction::insert_many(collection& source, core::vector<document>& list, const document& options)
			{
#ifdef VI_MONGOC
				if (!push(options))
					return expects_promise_db<document>(database_exception(0, "invalid operation"));

				return source.insert_many(list, options);
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> transaction::insert_one(collection& source, const document& result, const document& options)
			{
#ifdef VI_MONGOC
				if (!push(options))
					return expects_promise_db<document>(database_exception(0, "invalid operation"));

				return source.insert_one(result, options);
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> transaction::update_many(collection& source, const document& match, const document& update, const document& options)
			{
#ifdef VI_MONGOC
				if (!push(options))
					return expects_promise_db<document>(database_exception(0, "invalid operation"));

				return source.update_many(match, update, options);
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> transaction::update_one(collection& source, const document& match, const document& update, const document& options)
			{
#ifdef VI_MONGOC
				if (!push(options))
					return expects_promise_db<document>(database_exception(0, "invalid operation"));

				return source.update_one(match, update, options);
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<cursor> transaction::find_many(collection& source, const document& match, const document& options)
			{
#ifdef VI_MONGOC
				if (!push(options))
					return expects_promise_db<cursor>(database_exception(0, "invalid operation"));

				return source.find_many(match, options);
#else
				return expects_promise_db<cursor>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<cursor> transaction::find_one(collection& source, const document& match, const document& options)
			{
#ifdef VI_MONGOC
				if (!push(options))
					return expects_promise_db<cursor>(database_exception(0, "invalid operation"));

				return source.find_one(match, options);
#else
				return expects_promise_db<cursor>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<cursor> transaction::aggregate(collection& source, query_flags flags, const document& pipeline, const document& options)
			{
#ifdef VI_MONGOC
				if (!push(options))
					return expects_promise_db<cursor>(database_exception(0, "invalid operation"));

				return source.aggregate(flags, pipeline, options);
#else
				return expects_promise_db<cursor>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<response> transaction::template_query(collection& source, const std::string_view& name, core::schema_args* map)
			{
				auto pattern = driver::get()->get_query(name, map);
				if (!pattern)
					return expects_promise_db<response>(std::move(pattern.error()));

				return query(source, *pattern);
			}
			expects_promise_db<response> transaction::query(collection& source, const document& command)
			{
#ifdef VI_MONGOC
				return source.query(command, *this);
#else
				return expects_promise_db<response>(database_exception(0, "not supported"));
#endif
			}
			core::promise<transaction_state> transaction::commit()
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<transaction_state>([context]()
				{
					tdocument subresult; transaction_state result;
					if (mongo_execute_query(&mongoc_client_session_commit_transaction, context, &subresult))
						result = transaction_state::OK;
					else if (mongoc_error_has_label(&subresult, "TransientTransactionError"))
						result = transaction_state::retry;
					else if (mongoc_error_has_label(&subresult, "UnknownTransactionCommitResult"))
						result = transaction_state::retry_commit;
					else
						result = transaction_state::fatal;

					bson_free(&subresult);
					return result;
				});
#else
				return core::promise<transaction_state>(transaction_state::fatal);
#endif
			}
			ttransaction* transaction::get() const
			{
#ifdef VI_MONGOC
				return base;
#else
				return nullptr;
#endif
			}

			collection::collection() : base(nullptr)
			{
			}
			collection::collection(tcollection* new_base) : base(new_base)
			{
			}
			collection::collection(collection&& other) : base(other.base)
			{
				other.base = nullptr;
			}
			collection::~collection()
			{
#ifdef VI_MONGOC
				if (base != nullptr)
					mongoc_collection_destroy(base);
				base = nullptr;
#endif
			}
			collection& collection::operator =(collection&& other)
			{
				if (&other == this)
					return *this;

#ifdef VI_MONGOC
				if (base != nullptr)
					mongoc_collection_destroy(base);
#endif
				base = other.base;
				other.base = nullptr;
				return *this;
			}
			expects_promise_db<void> collection::rename(const std::string_view& new_database_name, const std::string_view& new_collection_name)
			{
#ifdef VI_MONGOC
				auto* context = base;
				auto new_database_name_copy = core::string(new_database_name);
				auto new_collection_name_copy = core::string(new_database_name);
				return core::cotask<expects_db<void>>([context, new_database_name_copy = std::move(new_database_name_copy), new_collection_name_copy = std::move(new_collection_name_copy)]() mutable
				{
					return mongo_execute_query(&mongoc_collection_rename, context, new_database_name_copy.data(), new_collection_name_copy.data(), false);
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> collection::rename_with_options(const std::string_view& new_database_name, const std::string_view& new_collection_name, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				auto new_database_name_copy = core::string(new_database_name);
				auto new_collection_name_copy = core::string(new_database_name);
				return core::cotask<expects_db<void>>([context, new_database_name_copy = std::move(new_database_name_copy), new_collection_name_copy = std::move(new_collection_name_copy), &options]() mutable
				{
					return mongo_execute_query(&mongoc_collection_rename_with_opts, context, new_database_name_copy.c_str(), new_collection_name_copy.c_str(), false, options.get());
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> collection::rename_with_remove(const std::string_view& new_database_name, const std::string_view& new_collection_name)
			{
#ifdef VI_MONGOC
				auto* context = base;
				auto new_database_name_copy = core::string(new_database_name);
				auto new_collection_name_copy = core::string(new_database_name);
				return core::cotask<expects_db<void>>([context, new_database_name_copy = std::move(new_database_name_copy), new_collection_name_copy = std::move(new_collection_name_copy)]() mutable
				{
					return mongo_execute_query(&mongoc_collection_rename, context, new_database_name_copy.c_str(), new_collection_name_copy.c_str(), true);
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> collection::rename_with_options_and_remove(const std::string_view& new_database_name, const std::string_view& new_collection_name, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				auto new_database_name_copy = core::string(new_database_name);
				auto new_collection_name_copy = core::string(new_database_name);
				return core::cotask<expects_db<void>>([context, new_database_name_copy = std::move(new_database_name_copy), new_collection_name_copy = std::move(new_collection_name_copy), &options]() mutable
				{
					return mongo_execute_query(&mongoc_collection_rename_with_opts, context, new_database_name_copy.c_str(), new_collection_name_copy.c_str(), true, options.get());
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> collection::remove(const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<void>>([context, &options]()
				{
					return mongo_execute_query(&mongoc_collection_drop_with_opts, context, options.get());
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> collection::remove_index(const std::string_view& name, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				auto name_copy = core::string(name);
				return core::cotask<expects_db<void>>([context, name_copy = std::move(name_copy), &options]() mutable
				{
					return mongo_execute_query(&mongoc_collection_drop_index_with_opts, context, name_copy.c_str(), options.get());
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> collection::remove_many(const document& match, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<document>>([context, &match, &options]() -> expects_db<document>
				{
					tdocument subresult;
					auto status = mongo_execute_query(&mongoc_collection_delete_many, context, match.get(), options.get(), &subresult);
					if (!status)
					{
						bson_destroy((bson_t*)&subresult);
						return status.error();
					}

					document result = document::from_source(&subresult);
					bson_destroy((bson_t*)&subresult);
					return result;
				});
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> collection::remove_one(const document& match, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<document>>([context, &match, &options]() -> expects_db<document>
				{
					tdocument subresult;
					auto status = mongo_execute_query(&mongoc_collection_delete_one, context, match.get(), options.get(), &subresult);
					if (!status)
					{
						bson_destroy((bson_t*)&subresult);
						return status.error();
					}

					document result = document::from_source(&subresult);
					bson_destroy((bson_t*)&subresult);
					return result;
				});
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> collection::replace_one(const document& match, const document& replacement, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<document>>([context, &match, &replacement, &options]() -> expects_db<document>
				{
					tdocument subresult;
					auto status = mongo_execute_query(&mongoc_collection_replace_one, context, match.get(), replacement.get(), options.get(), &subresult);
					if (!status)
					{
						bson_destroy((bson_t*)&subresult);
						return status.error();
					}

					document result = document::from_source(&subresult);
					bson_destroy((bson_t*)&subresult);
					return result;
				});
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> collection::insert_many(core::vector<document>& list, const document& options)
			{
#ifdef VI_MONGOC
				VI_ASSERT(!list.empty(), "insert array should not be empty");
				core::vector<document> array(std::move(list));
				auto* context = base;

				return core::cotask<expects_db<document>>([context, &array, &options]() -> expects_db<document>
				{
					core::vector<tdocument*> subarray;
					subarray.reserve(array.size());
					for (auto& item : array)
						subarray.push_back(item.get());

					tdocument subresult;
					auto status = mongo_execute_query(&mongoc_collection_insert_many, context, (const tdocument**)subarray.data(), (size_t)array.size(), options.get(), &subresult);
					if (!status)
					{
						bson_destroy((bson_t*)&subresult);
						return status.error();
					}

					document result = document::from_source(&subresult);
					bson_destroy((bson_t*)&subresult);
					return result;
				});
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> collection::insert_one(const document& result, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<document>>([context, &result, &options]() -> expects_db<document>
				{
					tdocument subresult;
					auto status = mongo_execute_query(&mongoc_collection_insert_one, context, result.get(), options.get(), &subresult);
					if (!status)
					{
						bson_destroy((bson_t*)&subresult);
						return status.error();
					}

					document result = document::from_source(&subresult);
					bson_destroy((bson_t*)&subresult);
					return result;
				});
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> collection::update_many(const document& match, const document& update, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<document>>([context, &match, &update, &options]() -> expects_db<document>
				{
					tdocument subresult;
					auto status = mongo_execute_query(&mongoc_collection_update_many, context, match.get(), update.get(), options.get(), &subresult);
					if (!status)
					{
						bson_destroy((bson_t*)&subresult);
						return status.error();
					}

					document result = document::from_source(&subresult);
					bson_destroy((bson_t*)&subresult);
					return result;
				});
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> collection::update_one(const document& match, const document& update, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<document>>([context, &match, &update, &options]() -> expects_db<document>
				{
					tdocument subresult;
					auto status = mongo_execute_query(&mongoc_collection_update_one, context, match.get(), update.get(), options.get(), &subresult);
					if (!status)
					{
						bson_destroy((bson_t*)&subresult);
						return status.error();
					}

					document result = document::from_source(&subresult);
					bson_destroy((bson_t*)&subresult);
					return result;
				});
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<document> collection::find_and_modify(const document& query, const document& sort, const document& update, const document& fields, bool remove_at, bool upsert, bool init)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<document>>([context, &query, &sort, &update, &fields, remove_at, upsert, init]() -> expects_db<document>
				{
					tdocument subresult;
					auto status = mongo_execute_query(&mongoc_collection_find_and_modify, context, query.get(), sort.get(), update.get(), fields.get(), remove_at, upsert, init, &subresult);
					if (!status)
					{
						bson_destroy((bson_t*)&subresult);
						return status.error();
					}

					document result = document::from_source(&subresult);
					bson_destroy((bson_t*)&subresult);
					return result;
				});
#else
				return expects_promise_db<document>(database_exception(0, "not supported"));
#endif
			}
			core::promise<size_t> collection::count_documents(const document& match, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<size_t>([context, &match, &options]()
				{
					int64_t count = mongoc_collection_count_documents(context, match.get(), options.get(), nullptr, nullptr, nullptr);
					return count > 0 ? (size_t)count : (size_t)0;
				});
#else
				return core::promise<size_t>(0);
#endif
			}
			core::promise<size_t> collection::count_documents_estimated(const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<size_t>([context, &options]()
				{
					int64_t count = mongoc_collection_estimated_document_count(context, options.get(), nullptr, nullptr, nullptr);
					return count > 0 ? (size_t)count : (size_t)0;
				});
#else
				return core::promise<size_t>(0);
#endif
			}
			expects_promise_db<cursor> collection::find_indexes(const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<cursor>>([context, &options]()
				{
					return mongo_execute_cursor(&mongoc_collection_find_indexes_with_opts, context, options.get());
				});
#else
				return expects_promise_db<cursor>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<cursor> collection::find_many(const document& match, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<cursor>>([context, &match, &options]()
				{
					return mongo_execute_cursor(&mongoc_collection_find_with_opts, context, match.get(), options.get(), nullptr);
				});
#else
				return expects_promise_db<cursor>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<cursor> collection::find_one(const document& match, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<cursor>>([context, &match, &options]()
				{
					document settings;
					if (options.get() != nullptr)
					{
						settings = options.copy();
						settings.set_integer("limit", 1);
					}
					else
						settings = document(BCON_NEW("limit", BCON_INT32(1)));

					return mongo_execute_cursor(&mongoc_collection_find_with_opts, context, match.get(), settings.get(), nullptr);
				});
#else
				return expects_promise_db<cursor>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<cursor> collection::aggregate(query_flags flags, const document& pipeline, const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<cursor>>([context, flags, &pipeline, &options]()
				{
					return mongo_execute_cursor(&mongoc_collection_aggregate, context, (mongoc_query_flags_t)flags, pipeline.get(), options.get(), nullptr);
				});
#else
				return expects_promise_db<cursor>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<response> collection::template_query(const std::string_view& name, core::schema_args* map, const transaction& session)
			{
				VI_DEBUG("mongoc template query %s", name.empty() ? "empty-query-name" : core::string(name).c_str());
				auto pattern = driver::get()->get_query(name, map);
				if (!pattern)
					return expects_promise_db<response>(pattern.error());

				return query(*pattern, session);
			}
			expects_promise_db<response> collection::query(const document& command, const transaction& session)
			{
#ifdef VI_MONGOC
				if (!command.get())
					return expects_promise_db<response>(database_exception(0, "execute query: empty command"));

				property type;
				if (!command.get_property("type", &type) || type.mod != type::string)
					return expects_promise_db<response>(database_exception(0, "execute query: no type field"));

				if (type.string == "aggregate")
				{
					query_flags flags = query_flags::none; property qflags;
					if (command.get_property("flags", &qflags) && qflags.mod == type::string)
					{
						for (auto& item : core::stringify::split(qflags.string, ','))
						{
							if (item == "tailable-cursor")
								flags = flags | query_flags::tailable_cursor;
							else if (item == "slave-ok")
								flags = flags | query_flags::slave_ok;
							else if (item == "oplog-replay")
								flags = flags | query_flags::oplog_replay;
							else if (item == "no-cursor-timeout")
								flags = flags | query_flags::no_cursor_timeout;
							else if (item == "await-data")
								flags = flags | query_flags::await_data;
							else if (item == "exhaust")
								flags = flags | query_flags::exhaust;
							else if (item == "partial")
								flags = flags | query_flags::partial;
						}
					}

					property options = command["options"];
					if (session && !session.put(&options.source))
						return expects_promise_db<response>(database_exception(0, "execute query: in transaction"));

					property pipeline;
					if (!command.get_property("pipeline", &pipeline) || pipeline.mod != type::array)
						return expects_promise_db<response>(database_exception(0, "execute query: no pipeline field"));

					return aggregate(flags, pipeline.reset(), options.reset()).then<expects_db<response>>([](expects_db<cursor>&& result) -> expects_db<response>
					{
						if (!result)
							return result.error();

						return response(*result);
					});
				}
				else if (type.string == "find")
				{
					property options = command["options"];
					if (session && !session.put(&options.source))
						return expects_promise_db<response>(database_exception(0, "execute query: in transaction"));

					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no match field"));

					return find_one(match.reset(), options.reset()).then<expects_db<response>>([](expects_db<cursor>&& result) -> expects_db<response>
					{
						if (!result)
							return result.error();

						return response(*result);
					});
				}
				else if (type.string == "find-many")
				{
					property options = command["options"];
					if (session && !session.put(&options.source))
						return expects_promise_db<response>(database_exception(0, "execute query: in transaction"));

					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no match field"));

					return find_many(match.reset(), options.reset()).then<expects_db<response>>([](expects_db<cursor>&& result) -> expects_db<response>
					{
						if (!result)
							return result.error();

						return response(*result);
					});
				}
				else if (type.string == "update")
				{
					property options = command["options"];
					if (session && !session.put(&options.source))
						return expects_promise_db<response>(database_exception(0, "execute query: in transaction"));

					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no match field"));

					property update;
					if (!command.get_property("update", &update) || update.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no update field"));

					return update_one(match.reset(), update.reset(), options.reset()).then<expects_db<response>>([](expects_db<document>&& result) -> expects_db<response>
					{
						if (!result)
							return result.error();

						return response(*result);
					});
				}
				else if (type.string == "update-many")
				{
					property options = command["options"];
					if (session && !session.put(&options.source))
						return expects_promise_db<response>(database_exception(0, "execute query: in transaction"));

					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no match field"));

					property update;
					if (!command.get_property("update", &update) || update.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no update field"));

					return update_many(match.reset(), update.reset(), options.reset()).then<expects_db<response>>([](expects_db<document>&& result) -> expects_db<response>
					{
						if (!result)
							return result.error();

						return response(*result);
					});
				}
				else if (type.string == "insert")
				{
					property options = command["options"];
					if (session && !session.put(&options.source))
						return expects_promise_db<response>(database_exception(0, "execute query: in transaction"));

					property value;
					if (!command.get_property("value", &value) || value.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no value field"));

					return insert_one(value.reset(), options.reset()).then<expects_db<response>>([](expects_db<document>&& result) -> expects_db<response>
					{
						if (!result)
							return result.error();

						return response(*result);
					});
				}
				else if (type.string == "insert-many")
				{
					property options = command["options"];
					if (session && !session.put(&options.source))
						return expects_promise_db<response>(database_exception(0, "execute query: in transaction"));

					property values;
					if (!command.get_property("values", &values) || values.mod != type::array)
						return expects_promise_db<response>(database_exception(0, "execute query: no values field"));

					core::vector<document> data;
					values.as_document().loop([&data](property* value)
					{
						data.push_back(value->reset());
						return true;
					});

					return insert_many(data, options.reset()).then<expects_db<response>>([](expects_db<document>&& result) -> expects_db<response>
					{
						if (!result)
							return result.error();

						return response(*result);
					});
				}
				else if (type.string == "find-update")
				{
					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no match field"));

					property sort = command["sort"];
					property update = command["update"];
					property fields = command["fields"];
					property remove = command["remove"];
					property upsert = command["upsert"];
					property init = command["new"];

					return find_and_modify(match.reset(), sort.reset(), update.reset(), fields.reset(), remove.boolean, upsert.boolean, init.boolean).then<expects_db<response>>([](expects_db<document>&& result) -> expects_db<response>
					{
						if (!result)
							return result.error();

						return response(*result);
					});
				}
				else if (type.string == "replace")
				{
					property options = command["options"];
					if (session && !session.put(&options.source))
						return expects_promise_db<response>(database_exception(0, "execute query: in transaction"));

					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no match field"));

					property value;
					if (!command.get_property("value", &value) || value.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no value field"));

					return replace_one(match.reset(), value.reset(), options.reset()).then<expects_db<response>>([](expects_db<document>&& result) -> expects_db<response>
					{
						if (!result)
							return result.error();

						return response(*result);
					});
				}
				else if (type.string == "remove")
				{
					property options = command["options"];
					if (session && !session.put(&options.source))
						return expects_promise_db<response>(database_exception(0, "execute query: in transaction"));

					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no match field"));

					return remove_one(match.reset(), options.reset()).then<expects_db<response>>([](expects_db<document>&& result) -> expects_db<response>
					{
						if (!result)
							return result.error();

						return response(*result);
					});
				}
				else if (type.string == "remove-many")
				{
					property options = command["options"];
					if (session && !session.put(&options.source))
						return expects_promise_db<response>(database_exception(0, "execute query: in transaction"));

					property match;
					if (!command.get_property("match", &match) || match.mod != type::document)
						return expects_promise_db<response>(database_exception(0, "execute query: no match field"));

					return remove_many(match.reset(), options.reset()).then<expects_db<response>>([](expects_db<document>&& result) -> expects_db<response>
					{
						if (!result)
							return result.error();

						return response(*result);
					});
				}

				return expects_promise_db<response>(database_exception(0, "invalid query type: " + type.string));
#else
				return expects_promise_db<response>(database_exception(0, "not supported"));
#endif
			}
			core::string collection::get_name() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				return mongoc_collection_get_name(base);
#else
				return core::string();
#endif
			}
			expects_db<stream> collection::create_stream(document& options)
			{
#ifdef VI_MONGOC
				if (!base)
					return database_exception(0, "invalid operation");

				tstream* operation = mongoc_collection_create_bulk_operation_with_opts(base, options.get());
				return stream(base, operation, std::move(options));
#else
				return database_exception(0, "not supported");
#endif
			}
			tcollection* collection::get() const
			{
#ifdef VI_MONGOC
				return base;
#else
				return nullptr;
#endif
			}

			database::database() : base(nullptr)
			{
			}
			database::database(tdatabase* new_base) : base(new_base)
			{
			}
			database::database(database&& other) : base(other.base)
			{
				other.base = nullptr;
			}
			database::~database()
			{
#ifdef VI_MONGOC
				if (base != nullptr)
					mongoc_database_destroy(base);
				base = nullptr;
#endif
			}
			database& database::operator =(database&& other)
			{
				if (&other == this)
					return *this;

#ifdef VI_MONGOC
				if (base != nullptr)
					mongoc_database_destroy(base);
#endif
				base = other.base;
				other.base = nullptr;
				return *this;
			}
			expects_promise_db<void> database::remove_all_users()
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<void>>([context]()
				{
					return mongo_execute_query(&mongoc_database_remove_all_users, context);
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> database::remove_user(const std::string_view& name)
			{
#ifdef VI_MONGOC
				auto* context = base;
				auto name_copy = core::string(name);
				return core::cotask<expects_db<void>>([context, name_copy = std::move(name_copy)]() mutable
				{
					return mongo_execute_query(&mongoc_database_remove_user, context, name_copy.c_str());
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> database::remove()
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<void>>([context]()
				{
					return mongo_execute_query(&mongoc_database_drop, context);
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> database::remove_with_options(const document& options)
			{
#ifdef VI_MONGOC
				if (!options.get())
					return remove();

				auto* context = base;
				return core::cotask<expects_db<void>>([context, &options]()
				{
					return mongo_execute_query(&mongoc_database_drop_with_opts, context, options.get());
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> database::add_user(const std::string_view& username, const std::string_view& password, const document& roles, const document& custom)
			{
#ifdef VI_MONGOC
				auto* context = base;
				auto username_copy = core::string(username);
				auto password_copy = core::string(password);
				return core::cotask<expects_db<void>>([context, username_copy = std::move(username_copy), password_copy = std::move(password_copy), &roles, &custom]() mutable
				{
					return mongo_execute_query(&mongoc_database_add_user, context, username_copy.c_str(), password_copy.c_str(), roles.get(), custom.get());
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> database::has_collection(const std::string_view& name)
			{
#ifdef VI_MONGOC
				auto* context = base;
				auto name_copy = core::string(name);
				return core::cotask<expects_db<void>>([context, name_copy = std::move(name_copy)]() mutable -> expects_db<void>
				{
					bson_error_t error;
					memset(&error, 0, sizeof(bson_error_t));
					if (!mongoc_database_has_collection(context, name_copy.c_str(), &error))
						return database_exception(error.code, error.message);

					return core::expectation::met;
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<cursor> database::find_collections(const document& options)
			{
#ifdef VI_MONGOC
				auto* context = base;
				return core::cotask<expects_db<cursor>>([context, &options]()
				{
					return mongo_execute_cursor(&mongoc_database_find_collections_with_opts, context, options.get());
				});
#else
				return expects_promise_db<cursor>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<collection> database::create_collection(const std::string_view& name, const document& options)
			{
#ifdef VI_MONGOC
				if (!base)
					return expects_promise_db<collection>(database_exception(0, "invalid operation"));

				auto* context = base;
				auto name_copy = core::string(name);
				return core::cotask<expects_db<collection>>([context, name_copy = std::move(name_copy), &options]() mutable -> expects_db<collection>
				{
					bson_error_t error;
					memset(&error, 0, sizeof(bson_error_t));
					tcollection* result = mongoc_database_create_collection(context, name_copy.c_str(), options.get(), &error);
					if (!result)
						return database_exception(error.code, error.message);

					return collection(result);
				});
#else
				return expects_promise_db<collection>(database_exception(0, "not supported"));
#endif
			}
			expects_db<core::vector<core::string>> database::get_collection_names(const document& options) const
			{
#ifdef VI_MONGOC
				bson_error_t error;
				memset(&error, 0, sizeof(bson_error_t));
				if (!base)
					return database_exception(0, "invalid operation");

				char** names = mongoc_database_get_collection_names_with_opts(base, options.get(), &error);
				if (!names)
					return database_exception(error.code, error.message);

				core::vector<core::string> output;
				for (unsigned i = 0; names[i]; i++)
					output.push_back(names[i]);

				bson_strfreev(names);
				return output;
#else
				return database_exception(0, "not supported");
#endif
			}
			core::string database::get_name() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				auto* name = mongoc_database_get_name(base);
				return name ? name : "";
#else
				return core::string();
#endif
			}
			collection database::get_collection(const std::string_view& name)
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				return mongoc_database_get_collection(base, name.data());
#else
				return nullptr;
#endif
			}
			tdatabase* database::get() const
			{
#ifdef VI_MONGOC
				return base;
#else
				return nullptr;
#endif
			}

			watcher::watcher()
			{
			}
			watcher::watcher(twatcher* new_base) : base(new_base)
			{
			}
			watcher::watcher(const watcher& other) : watcher(std::move(*(watcher*)&other))
			{
			}
			watcher::watcher(watcher&& other) : base(other.base)
			{
				other.base = nullptr;
			}
			watcher::~watcher()
			{
#ifdef VI_MONGOC
				if (base != nullptr)
					mongoc_change_stream_destroy(base);
				base = nullptr;
#endif
			}
			watcher& watcher::operator =(const watcher& other)
			{
				return *this = std::move(*(watcher*)&other);
			}
			watcher& watcher::operator =(watcher&& other)
			{
				if (&other == this)
					return *this;
#ifdef VI_MONGOC
				if (base != nullptr)
					mongoc_change_stream_destroy(base);
#endif
				base = other.base;
				other.base = nullptr;
				return *this;
			}
			expects_promise_db<void> watcher::next(document& result)
			{
#ifdef VI_MONGOC
				if (!base || !result.get())
					return expects_promise_db<void>(database_exception(0, "invalid operation"));

				auto* context = base;
				return core::cotask<expects_db<void>>([context, &result]() -> expects_db<void>
				{
					tdocument* ptr = result.get();
					if (!mongoc_change_stream_next(context, (const tdocument**)&ptr))
						return database_exception(0, "end of stream");

					return core::expectation::met;
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> watcher::error(document& result)
			{
#ifdef VI_MONGOC
				if (!base || !result.get())
					return expects_promise_db<void>(database_exception(0, "invalid operation"));

				auto* context = base;
				return core::cotask<expects_db<void>>([context, &result]() -> expects_db<void>
				{
					tdocument* ptr = result.get();
					if (!mongoc_change_stream_error_document(context, nullptr, (const tdocument**)&ptr))
						return database_exception(0, "end of stream");

					return core::expectation::met;
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			twatcher* watcher::get() const
			{
#ifdef VI_MONGOC
				return base;
#else
				return nullptr;
#endif
			}
			expects_db<watcher> watcher::from_connection(connection* connection, const document& pipeline, const document& options)
			{
#ifdef VI_MONGOC
				if (!connection)
					return database_exception(0, "invalid operation");

				return watcher(mongoc_client_watch(connection->get(), pipeline.get(), options.get()));
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_db<watcher> watcher::from_database(const database& database, const document& pipeline, const document& options)
			{
#ifdef VI_MONGOC
				if (!database.get())
					return database_exception(0, "invalid operation");

				return watcher(mongoc_database_watch(database.get(), pipeline.get(), options.get()));
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_db<watcher> watcher::from_collection(const collection& collection, const document& pipeline, const document& options)
			{
#ifdef VI_MONGOC
				if (!collection.get())
					return database_exception(0, "invalid operation");

				return watcher(mongoc_collection_watch(collection.get(), pipeline.get(), options.get()));
#else
				return database_exception(0, "not supported");
#endif
			}

			connection::connection() : connected(false), session(nullptr), base(nullptr), master(nullptr)
			{
			}
			connection::~connection() noexcept
			{
#ifdef VI_MONGOC
				ttransaction* context = session.get();
				if (context != nullptr)
					mongoc_client_session_destroy(context);
#endif
				if (connected && base != nullptr)
					disconnect();
			}
			expects_promise_db<void> connection::connect(address* location)
			{
#ifdef VI_MONGOC
				VI_ASSERT(master != nullptr, "connection should be created outside of cluster");
				VI_ASSERT(location && location->get(), "location should be valid");
				if (!core::os::control::has(core::access_option::net))
					return expects_promise_db<void>(database_exception(-1, "connect failed: permission denied"));
				else if (connected)
					return disconnect().then<expects_promise_db<void>>([this, location](expects_db<void>&&) { return this->connect(location); });

				taddress* address = location->get(); *location = nullptr;
				return core::cotask<expects_db<void>>([this, address]() -> expects_db<void>
				{
					VI_MEASURE(core::timings::intensive);
					base = mongoc_client_new_from_uri(address);
					if (!base)
						return database_exception(0, "connect: invalid address");

					driver::get()->attach_query_log(base);
					connected = true;
					return core::expectation::met;
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> connection::disconnect()
			{
#ifdef VI_MONGOC
				VI_ASSERT(connected && base, "connection should be established");
				return core::cotask<expects_db<void>>([this]() -> expects_db<void>
				{
					connected = false;
					if (master != nullptr)
					{
						if (base != nullptr)
						{
							mongoc_client_pool_push(master->get(), base);
							base = nullptr;
						}

						master = nullptr;
					}
					else if (base != nullptr)
					{
						mongoc_client_destroy(base);
						base = nullptr;
					}
					return core::expectation::met;
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> connection::make_transaction(std::function<core::promise<bool>(transaction*)>&& callback)
			{
#ifdef VI_MONGOC
				VI_ASSERT(callback, "callback should not be empty");
				return core::coasync<expects_db<void>>([this, callback = std::move(callback)]() mutable -> expects_promise_db<void>
				{
					auto context = get_session();
					if (!context)
						coreturn expects_db<void>(std::move(context.error()));
					else if (!*context)
						coreturn expects_db<void>(database_exception(0, "make transaction: no session"));

					while (true)
					{
						auto status = VI_AWAIT(context->begin());
						if (!status)
							coreturn expects_db<void>(std::move(status));

						if (!VI_AWAIT(callback(*context)))
							break;

						while (true)
						{
							transaction_state state = VI_AWAIT(context->commit());
							if (state == transaction_state::fatal)
								coreturn expects_db<void>(database_exception(0, "make transaction: fatal error"));
							else if (state == transaction_state::OK)
								coreturn expects_db<void>(core::expectation::met);

							if (state == transaction_state::retry_commit)
							{
								VI_DEBUG("mongoc retrying transaction commit");
								continue;
							}

							if (state == transaction_state::retry)
							{
								VI_DEBUG("mongoc retrying full transaction");
								break;
							}
						}
					}

					VI_AWAIT(context->rollback());
					coreturn expects_db<void>(database_exception(0, "make transaction: aborted"));
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<cursor> connection::find_databases(const document& options)
			{
#ifdef VI_MONGOC
				return core::cotask<expects_db<cursor>>([this, &options]() -> expects_db<cursor>
				{
					mongoc_cursor_t* result = mongoc_client_find_databases_with_opts(base, options.get());
					if (!result)
						return database_exception(0, "databases not found");

					return cursor(result);
				});
#else
				return expects_promise_db<cursor>(database_exception(0, "not supported"));
#endif
			}
			void connection::set_profile(const std::string_view& name)
			{
#ifdef VI_MONGOC
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				mongoc_client_set_appname(base, name.data());
#endif
			}
			expects_db<void> connection::set_server(bool for_writes)
			{
#ifdef VI_MONGOC
				bson_error_t error;
				memset(&error, 0, sizeof(bson_error_t));

				mongoc_server_description_t* server = mongoc_client_select_server(base, for_writes, nullptr, &error);
				if (!server)
					return database_exception(error.code, error.message);

				mongoc_server_description_destroy(server);
				return core::expectation::met;
#else
				return database_exception(0, "not supported");
#endif
			}
			expects_db<transaction*> connection::get_session()
			{
#ifdef VI_MONGOC
				if (session.get() != nullptr)
					return &session;

				bson_error_t error;
				session = mongoc_client_start_session(base, nullptr, &error);
				if (!session.get())
					return database_exception(error.code, error.message);

				return &session;
#else
				return &session;
#endif
			}
			database connection::get_database(const std::string_view& name) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				return mongoc_client_get_database(base, name.data());
#else
				return nullptr;
#endif
			}
			database connection::get_default_database() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				return mongoc_client_get_default_database(base);
#else
				return nullptr;
#endif
			}
			collection connection::get_collection(const std::string_view& database_name, const std::string_view& name) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				VI_ASSERT(core::stringify::is_cstring(database_name), "db name should be set");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				return mongoc_client_get_collection(base, database_name.data(), name.data());
#else
				return nullptr;
#endif
			}
			address connection::get_address() const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				return (taddress*)mongoc_client_get_uri(base);
#else
				return nullptr;
#endif
			}
			cluster* connection::get_master() const
			{
				return master;
			}
			tconnection* connection::get() const
			{
				return base;
			}
			expects_db<core::vector<core::string>> connection::get_database_names(const document& options) const
			{
#ifdef VI_MONGOC
				VI_ASSERT(base != nullptr, "context should be set");
				bson_error_t error;
				memset(&error, 0, sizeof(bson_error_t));
				char** names = mongoc_client_get_database_names_with_opts(base, options.get(), &error);
				if (!names)
					return database_exception(error.code, error.message);

				core::vector<core::string> output;
				for (unsigned i = 0; names[i]; i++)
					output.push_back(names[i]);

				bson_strfreev(names);
				return output;
#else
				return database_exception(0, "not supported");
#endif
			}
			bool connection::is_connected() const
			{
				return connected;
			}

			cluster::cluster() : connected(false), pool(nullptr), src_address(nullptr)
			{
			}
			cluster::~cluster() noexcept
			{
#ifdef VI_MONGOC
				if (pool != nullptr)
				{
					mongoc_client_pool_destroy(pool);
					pool = nullptr;
				}
				src_address.~address();
				connected = false;
#endif
			}
			expects_promise_db<void> cluster::connect(address* location)
			{
#ifdef VI_MONGOC
				VI_ASSERT(location && location->get(), "location should be set");
				if (!core::os::control::has(core::access_option::net))
					return expects_promise_db<void>(database_exception(-1, "connect failed: permission denied"));
				else if (connected)
					return disconnect().then<expects_promise_db<void>>([this, location](expects_db<void>&&) { return this->connect(location); });

				taddress* context = location->get(); *location = nullptr;
				return core::cotask<expects_db<void>>([this, context]() -> expects_db<void>
				{
					VI_MEASURE(core::timings::intensive);
					src_address = context;
					pool = mongoc_client_pool_new(src_address.get());
					if (!pool)
						return database_exception(0, "connect: invalid address");

					driver::get()->attach_query_log(pool);
					connected = true;
					return core::expectation::met;
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			expects_promise_db<void> cluster::disconnect()
			{
#ifdef VI_MONGOC
				VI_ASSERT(connected && pool, "connection should be established");
				return core::cotask<expects_db<void>>([this]() -> expects_db<void>
				{
					if (pool != nullptr)
					{
						mongoc_client_pool_destroy(pool);
						pool = nullptr;
					}

					src_address.~address();
					connected = false;
					return core::expectation::met;
				});
#else
				return expects_promise_db<void>(database_exception(0, "not supported"));
#endif
			}
			void cluster::set_profile(const std::string_view& name)
			{
#ifdef VI_MONGOC
				VI_ASSERT(pool != nullptr, "connection should be established");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				mongoc_client_pool_set_appname(pool, name.data());
#endif
			}
			void cluster::push(connection** client)
			{
#ifdef VI_MONGOC
				VI_ASSERT(client && *client, "client should be set");
				mongoc_client_pool_push(pool, (*client)->base);
				(*client)->base = nullptr;
				(*client)->connected = false;
				(*client)->master = nullptr;
				core::memory::release(*client);
#endif
			}
			connection* cluster::pop()
			{
#ifdef VI_MONGOC
				tconnection* base = mongoc_client_pool_pop(pool);
				if (!base)
					return nullptr;

				connection* result = new connection();
				result->base = base;
				result->connected = true;
				result->master = this;

				return result;
#else
				return nullptr;
#endif
			}
			tconnection_pool* cluster::get() const
			{
				return pool;
			}
			address& cluster::get_address()
			{
				return src_address;
			}

			bool utils::get_id(uint8_t* id12) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(id12 != nullptr, "id should be set");
				bson_oid_t object_id;
				bson_oid_init(&object_id, nullptr);

				memcpy((void*)id12, (void*)object_id.bytes, sizeof(char) * 12);
				return true;
#else
				return false;
#endif
			}
			bool utils::get_decimal(const std::string_view& value, int64_t* high, int64_t* low) noexcept
			{
				VI_ASSERT(core::stringify::is_cstring(value), "value should be set");
				VI_ASSERT(high != nullptr, "high should be set");
				VI_ASSERT(low != nullptr, "low should be set");
#ifdef VI_MONGOC
				bson_decimal128_t decimal;
				if (!bson_decimal128_from_string(value.data(), &decimal))
					return false;

				*high = decimal.high;
				*low = decimal.low;
				return true;
#else
				return false;
#endif
			}
			uint32_t utils::get_hash_id(uint8_t* id12) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(id12 != nullptr, "id should be set");

				bson_oid_t id;
				memcpy((void*)id.bytes, (void*)id12, sizeof(char) * 12);
				return bson_oid_hash(&id);
#else
				return 0;
#endif
			}
			int64_t utils::get_time_id(uint8_t* id12) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(id12 != nullptr, "id should be set");

				bson_oid_t id;
				memcpy((void*)id.bytes, (void*)id12, sizeof(char) * 12);

				return bson_oid_get_time_t(&id);
#else
				return 0;
#endif
			}
			core::string utils::id_to_string(uint8_t* id12) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(id12 != nullptr, "id should be set");

				bson_oid_t id;
				memcpy(id.bytes, id12, sizeof(uint8_t) * 12);

				char result[25];
				bson_oid_to_string(&id, result);

				return core::string(result, 24);
#else
				return "";
#endif
			}
			core::string utils::string_to_id(const std::string_view& id24) noexcept
			{
				VI_ASSERT(id24.size() == 24, "id should be 24 chars long");
				VI_ASSERT(core::stringify::is_cstring(id24), "id should be set");
#ifdef VI_MONGOC
				bson_oid_t id;
				bson_oid_init_from_string(&id, id24.data());

				return core::string((const char*)id.bytes, 12);
#else
				return "";
#endif
			}
			core::string utils::get_json(core::schema* source, bool escape) noexcept
			{
				VI_ASSERT(source != nullptr, "source should be set");
				switch (source->value.get_type())
				{
					case core::var_type::object:
					{
						core::string result = "{";
						for (auto* node : source->get_childs())
						{
							result.append(1, '\"').append(node->key).append("\":");
							result.append(get_json(node, true)).append(1, ',');
						}

						if (!source->empty())
							result = result.substr(0, result.size() - 1);

						return result + "}";
					}
					case core::var_type::array:
					{
						core::string result = "[";
						for (auto* node : source->get_childs())
							result.append(get_json(node, true)).append(1, ',');

						if (!source->empty())
							result = result.substr(0, result.size() - 1);

						return result + "]";
					}
					case core::var_type::string:
					{
						core::string result = source->value.get_blob();
						if (!escape)
							return result;

						result.insert(result.begin(), '\"');
						result.append(1, '\"');
						return result;
					}
					case core::var_type::integer:
						return core::to_string(source->value.get_integer());
					case core::var_type::number:
						return core::to_string(source->value.get_number());
					case core::var_type::boolean:
						return source->value.get_boolean() ? "true" : "false";
					case core::var_type::decimal:
						return "{\"$numberDouble\":\"" + source->value.get_decimal().to_string() + "\"}";
					case core::var_type::binary:
					{
						if (source->value.size() != 12)
						{
							core::string base = compute::codec::bep45_encode(source->value.get_blob());
							return "\"" + base + "\"";
						}

						return "{\"$oid\":\"" + utils::id_to_string(source->value.get_binary()) + "\"}";
					}
					case core::var_type::null:
					case core::var_type::undefined:
						return "null";
					default:
						break;
				}

				return "";
			}

			driver::sequence::sequence() : cache(nullptr)
			{
			}
			driver::sequence::sequence(const sequence& other) : positions(other.positions), request(other.request), cache(other.cache.copy())
			{
			}

			driver::driver() noexcept : logger(nullptr), APM(nullptr)
			{
#ifdef VI_MONGOC
				VI_TRACE("mongo OK initialize driver");
				mongoc_log_set_handler([](mongoc_log_level_t level, const char* domain, const char* message, void*)
				{
					switch (level)
					{
						case MONGOC_LOG_LEVEL_WARNING:
							VI_WARN("mongoc %s %s", domain, message);
							break;
						case MONGOC_LOG_LEVEL_INFO:
							VI_INFO("mongoc %s %s", domain, message);
							break;
						case MONGOC_LOG_LEVEL_ERROR:
						case MONGOC_LOG_LEVEL_CRITICAL:
							VI_ERR("mongoc %s %s", domain, message);
							break;
						case MONGOC_LOG_LEVEL_MESSAGE:
							VI_DEBUG("mongoc %s %s", domain, message);
							break;
						case MONGOC_LOG_LEVEL_TRACE:
							VI_TRACE("mongoc %s %s", domain, message);
							break;
						default:
							break;
					}
				}, nullptr);
				mongoc_init();
#endif
			}
			driver::~driver() noexcept
			{
#ifdef VI_MONGOC
				VI_TRACE("mongo cleanup driver");
				if (APM != nullptr)
				{
					mongoc_apm_callbacks_destroy((mongoc_apm_callbacks_t*)APM);
					APM = nullptr;
				}
				mongoc_cleanup();
#endif
			}
			void driver::set_query_log(const on_query_log& callback) noexcept
			{
				logger = callback;
				if (!logger || APM)
					return;
#ifdef VI_MONGOC
				mongoc_apm_callbacks_t* callbacks = mongoc_apm_callbacks_new();
				mongoc_apm_set_command_started_cb(callbacks, [](const mongoc_apm_command_started_t* event)
				{
					const char* name = mongoc_apm_command_started_get_command_name(event);
					char* command = bson_as_relaxed_extended_json(mongoc_apm_command_started_get_command(event), nullptr);
					core::string buffer = core::stringify::text("%s:\n%s\n", name, command);
					bson_free(command);

					auto* base = driver::get();
					if (base->logger)
						base->logger(buffer);
				});
				APM = (void*)callbacks;
#endif
			}
			void driver::attach_query_log(tconnection* connection) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(connection != nullptr, "connection should be set");
				mongoc_client_set_apm_callbacks(connection, (mongoc_apm_callbacks_t*)APM, nullptr);
#endif
			}
			void driver::attach_query_log(tconnection_pool* connection) noexcept
			{
#ifdef VI_MONGOC
				VI_ASSERT(connection != nullptr, "connection pool should be set");
				mongoc_client_pool_set_apm_callbacks(connection, (mongoc_apm_callbacks_t*)APM, nullptr);
#endif
			}
			void driver::add_constant(const std::string_view& name, const std::string_view& value) noexcept
			{
				VI_ASSERT(!name.empty(), "name should not be empty");
				core::umutex<std::mutex> unique(exclusive);
				constants[core::string(name)] = value;
			}
			expects_db<void> driver::add_query(const std::string_view& name, const std::string_view& buffer)
			{
				VI_ASSERT(!name.empty(), "name should not be empty");
				if (buffer.empty())
					return database_exception(0, "import empty query error: " + core::string(name));

				sequence result;
				result.request.assign(buffer);

				core::string enums = " \r\n\t\'\"()<>=%&^*/+-,!?:;";
				core::string erasable = " \r\n\t\'\"()<>=%&^*/+-,.!?:;";
				core::string quotes = "\"'`";

				core::string& base = result.request;
				core::stringify::replace_in_between(base, "/*", "*/", "", false);
				core::stringify::trim(base);
				core::stringify::compress(base, erasable.c_str(), quotes.c_str());

				auto enumerations = core::stringify::find_starts_with_ends_of(base, "#", enums.c_str(), quotes.c_str());
				if (!enumerations.empty())
				{
					int64_t offset = 0;
					core::umutex<std::mutex> unique(exclusive);
					for (auto& item : enumerations)
					{
						size_t size = item.second.end - item.second.start;
						item.second.start = (size_t)((int64_t)item.second.start + offset);
						item.second.end = (size_t)((int64_t)item.second.end + offset);

						auto it = constants.find(item.first);
						if (it == constants.end())
							return database_exception(0, "query expects @" + item.first + " constant: " + core::string(name));

						core::stringify::replace_part(base, item.second.start, item.second.end, it->second);
						size_t new_size = it->second.size();
						offset += (int64_t)new_size - (int64_t)size;
						item.second.end = item.second.start + new_size;
					}
				}

				core::vector<std::pair<core::string, core::text_settle>> variables;
				for (auto& item : core::stringify::find_in_between(base, "$<", ">", quotes.c_str()))
				{
					item.first += ";escape";
					variables.emplace_back(std::move(item));
				}

				for (auto& item : core::stringify::find_in_between(base, "@<", ">", quotes.c_str()))
				{
					item.first += ";unsafe";
					variables.emplace_back(std::move(item));
				}

				core::stringify::replace_parts(base, variables, "", [&erasable](const std::string_view&, char left, int)
				{
					return erasable.find(left) == core::string::npos ? ' ' : '\0';
				});

				for (auto& item : variables)
				{
					pose position;
					position.escape = item.first.find(";escape") != core::string::npos;
					position.offset = item.second.start;
					position.key = item.first.substr(0, item.first.find(';'));
					result.positions.emplace_back(std::move(position));
				}

				if (variables.empty())
				{
					auto subresult = document::from_json(result.request);
					if (!subresult)
						return subresult.error();
					result.cache = std::move(*subresult);
				}

				core::umutex<std::mutex> unique(exclusive);
				queries[core::string(name)] = std::move(result);
				return core::expectation::met;
			}
			expects_db<void> driver::add_directory(const std::string_view& directory, const std::string_view& origin)
			{
				core::vector<std::pair<core::string, core::file_entry>> entries;
				if (!core::os::directory::scan(directory, entries))
					return database_exception(0, "import directory scan error: " + core::string(directory));

				core::string path = core::string(directory);
				if (path.back() != '/' && path.back() != '\\')
					path.append(1, '/');

				size_t size = 0;
				for (auto& file : entries)
				{
					core::string base(path + file.first);
					if (file.second.is_directory)
					{
						auto status = add_directory(base, origin.empty() ? directory : origin);
						if (!status)
							return status;
						else
							continue;
					}

					if (!core::stringify::ends_with(base, ".json"))
						continue;

					auto buffer = core::os::file::read_all(base, &size);
					if (!buffer)
						continue;

					core::stringify::replace(base, origin.empty() ? directory : origin, "");
					core::stringify::replace(base, "\\", "/");
					core::stringify::replace(base, ".json", "");
					if (core::stringify::starts_of(base, "\\/"))
						base.erase(0, 1);

					auto status = add_query(base, std::string_view((char*)*buffer, size));
					core::memory::deallocate(*buffer);
					if (!status)
						return status;
				}

				return core::expectation::met;
			}
			bool driver::remove_constant(const std::string_view& name) noexcept
			{
				core::umutex<std::mutex> unique(exclusive);
				auto it = constants.find(core::key_lookup_cast(name));
				if (it == constants.end())
					return false;

				constants.erase(it);
				return true;
			}
			bool driver::remove_query(const std::string_view& name) noexcept
			{
				core::umutex<std::mutex> unique(exclusive);
				auto it = queries.find(core::key_lookup_cast(name));
				if (it == queries.end())
					return false;

				queries.erase(it);
				return true;
			}
			bool driver::load_cache_dump(core::schema* dump) noexcept
			{
				VI_ASSERT(dump != nullptr, "dump should be set");
				size_t count = 0;
				core::umutex<std::mutex> unique(exclusive);
				queries.clear();

				for (auto* data : dump->get_childs())
				{
					sequence result;
					result.request = data->get_var("request").get_blob();

					auto* cache = data->get("cache");
					if (cache != nullptr && cache->value.is_object())
					{
						result.cache = document::from_schema(cache);
						result.request = result.cache.to_json();
					}

					core::schema* positions = data->get("positions");
					if (positions != nullptr)
					{
						for (auto* position : positions->get_childs())
						{
							pose next;
							next.key = position->get_var(0).get_blob();
							next.offset = (size_t)position->get_var(1).get_integer();
							next.escape = position->get_var(2).get_boolean();
							result.positions.emplace_back(std::move(next));
						}
					}

					core::string name = data->get_var("name").get_blob();
					queries[name] = std::move(result);
					++count;
				}

				if (count > 0)
					VI_DEBUG("mongo OK load %" PRIu64 " parsed query templates", (uint64_t)count);

				return count > 0;
			}
			core::schema* driver::get_cache_dump() noexcept
			{
				core::umutex<std::mutex> unique(exclusive);
				core::schema* result = core::var::set::array();
				for (auto& query : queries)
				{
					core::schema* data = result->push(core::var::set::object());
					data->set("name", core::var::string(query.first));

					auto* cache = query.second.cache.to_schema();
					if (cache != nullptr)
						data->set("cache", cache);
					else
						data->set("request", core::var::string(query.second.request));

					auto* positions = data->set("positions", core::var::set::array());
					for (auto& position : query.second.positions)
					{
						auto* next = positions->push(core::var::set::array());
						next->push(core::var::string(position.key));
						next->push(core::var::integer(position.offset));
						next->push(core::var::boolean(position.escape));
					}
				}

				VI_DEBUG("mongo OK save %" PRIu64 " parsed query templates", (uint64_t)queries.size());
				return result;
			}
			expects_db<document> driver::get_query(const std::string_view& name, core::schema_args* map) noexcept
			{
				core::umutex<std::mutex> unique(exclusive);
				auto it = queries.find(core::key_lookup_cast(name));
				if (it == queries.end())
					return database_exception(0, "query not found: " + core::string(name));

				if (it->second.cache.get() != nullptr)
					return it->second.cache.copy();

				if (!map || map->empty())
					return document::from_json(it->second.request);

				sequence origin = it->second;
				size_t offset = 0;
				unique.negate();

				core::string& result = origin.request;
				for (auto& word : origin.positions)
				{
					auto it = map->find(word.key);
					if (it == map->end())
						return database_exception(0, "query expects @" + word.key + " constant: " + core::string(name));

					core::string value = utils::get_json(*it->second, word.escape);
					if (value.empty())
						continue;

					result.insert(word.offset + offset, value);
					offset += value.size();
				}

				auto data = document::from_json(origin.request);
				if (!data)
					return data.error();
				else if (!data->get())
					return database_exception(0, "query construction error: " + core::string(name));

				return data;
			}
			core::vector<core::string> driver::get_queries() noexcept
			{
				core::vector<core::string> result;
				core::umutex<std::mutex> unique(exclusive);
				result.reserve(queries.size());
				for (auto& item : queries)
					result.push_back(item.first);

				return result;
			}
		}
	}
}
