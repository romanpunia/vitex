#include "pq.h"
#ifdef VI_POSTGRESQL
#include <libpq-fe.h>
#endif
#ifdef VI_OPENSSL
extern "C"
{
#include <openssl/opensslv.h>
}
#endif

namespace vitex
{
	namespace network
	{
		namespace pq
		{
			class array_filter;

			typedef std::function<void(void*, array_filter*, char*, size_t)> filter_callback;
			typedef core::vector<std::function<void(bool)>> futures_list;
#ifdef VI_POSTGRESQL
			class array_filter
			{
			private:
				core::vector<char> records;
				const char* source;
				size_t size;
				size_t position;
				size_t dimension;
				bool subroutine;

			public:
				array_filter(const char* new_source, size_t new_size) : source(new_source), size(new_size), position(0), dimension(0), subroutine(false)
				{
				}
				bool parse(void* context, const filter_callback& callback)
				{
					VI_ASSERT(callback, "callback should be set");
					if (source[0] == '[')
					{
						while (position < size)
						{
							if (next().first == '=')
								break;
						}
					}

					bool quote = false;
					while (position < size)
					{
						auto sym = next();
						if (sym.first == '{' && !quote)
						{
							dimension++;
							if (dimension > 1)
							{
								array_filter subparser(source + (position - 1), size - (position - 1));
								subparser.subroutine = true;
								callback(context, &subparser, nullptr, 0);
								position += subparser.position - 2;
							}
						}
						else if (sym.first == '}' && !quote)
						{
							dimension--;
							if (!dimension)
							{
								emplace(context, callback, false);
								if (subroutine)
									return true;
							}
						}
						else if (sym.first == '"' && !sym.second)
						{
							if (quote)
								emplace(context, callback, true);
							quote = !quote;
						}
						else if (sym.first == ',' && !quote)
							emplace(context, callback, false);
						else
							records.push_back(sym.first);
					}

					return dimension == 0;
				}

			private:
				void emplace(void* context, const filter_callback& callback, bool empties)
				{
					VI_ASSERT(callback, "callback should be set");
					if (records.empty() && !empties)
						return;

					callback(context, nullptr, records.data(), records.size());
					records.clear();
				}
				std::pair<char, bool> next()
				{
					char v = source[position++];
					if (v != '\\')
						return std::make_pair(v, false);

					return std::make_pair(source[position++], true);
				}
			};

			static void PQlogNotice(void*, const char* message)
			{
				if (!message || message[0] == '\0')
					return;

				core::string buffer(message);
				for (auto& item : core::stringify::split(buffer, '\n'))
				{
					VI_DEBUG("[pq] %s", item.c_str());
					(void)item;
				}
			}
			static void PQlogNoticeOf(tconnection* connection)
			{
#ifdef VI_SQLITE
				VI_ASSERT(connection != nullptr, "base should be set");
				char* message = PQerrorMessage(connection);
				if (!message || message[0] == '\0')
					return;

				core::string buffer(message);
				buffer.erase(buffer.size() - 1);
				for (auto& item : core::stringify::split(buffer, '\n'))
				{
					VI_DEBUG("[pq] %s", item.c_str());
					(void)item;
				}
#endif
			}
			static core::schema* to_schema(const char* data, int size, uint32_t id);
			static void to_array_field(void* context, array_filter* subdata, char* data, size_t size)
			{
				VI_ASSERT(context != nullptr, "context should be set");
				std::pair<core::schema*, Oid>* base = (std::pair<core::schema*, Oid>*)context;
				if (subdata != nullptr)
				{
					std::pair<core::schema*, Oid> next;
					next.first = core::var::set::array();
					next.second = base->second;

					if (!subdata->parse(&next, to_array_field))
					{
						base->first->push(new core::schema(core::var::null()));
						core::memory::release(next.first);
					}
					else
						base->first->push(next.first);
				}
				else if (data != nullptr && size > 0)
				{
					core::string result(data, size);
					core::stringify::trim(result);

					if (result.empty())
						return;

					if (result != "NULL")
						base->first->push(to_schema(result.c_str(), (int)result.size(), base->second));
					else
						base->first->push(new core::schema(core::var::null()));
				}
			}
			static core::variant to_variant(const char* data, int size, uint32_t id)
			{
				if (!data)
					return core::var::null();

				oid_type type = (oid_type)id;
				switch (type)
				{
					case oid_type::symbol:
					case oid_type::int2:
					case oid_type::int4:
					case oid_type::int8:
					{
						core::string source(data, (size_t)size);
						if (core::stringify::has_integer(source))
							return core::var::integer(*core::from_string<int64_t>(source));

						return core::var::string(std::string_view(data, (size_t)size));
					}
					case oid_type::boolf:
					{
						if (data[0] == 't')
							return core::var::boolean(true);
						else if (data[0] == 'f')
							return core::var::boolean(false);

						core::string source(data, (size_t)size);
						core::stringify::to_lower(source);

						bool value = (source == "true" || source == "yes" || source == "on" || source == "1");
						return core::var::boolean(value);
					}
					case oid_type::float4:
					case oid_type::float8:
					{
						core::string source(data, (size_t)size);
						if (core::stringify::has_number(source))
							return core::var::number(*core::from_string<double>(source));

						return core::var::string(std::string_view(data, (size_t)size));
					}
					case oid_type::money:
					case oid_type::numeric:
						return core::var::decimal_string(core::string(data, (size_t)size));
					case oid_type::bytea:
					{
						size_t length = 0;
						uint8_t* buffer = PQunescapeBytea((const uint8_t*)data, &length);
						core::variant result = core::var::binary(buffer, length);

						PQfreemem(buffer);
						return result;
					}
					case oid_type::JSON:
					case oid_type::JSONB:
					case oid_type::any_array:
					case oid_type::name_array:
					case oid_type::text_array:
					case oid_type::date_array:
					case oid_type::time_array:
					case oid_type::uuid_array:
					case oid_type::cstring_array:
					case oid_type::bp_char_array:
					case oid_type::var_char_array:
					case oid_type::bit_array:
					case oid_type::var_bit_array:
					case oid_type::char_array:
					case oid_type::int2_array:
					case oid_type::int4_array:
					case oid_type::int8_array:
					case oid_type::bool_array:
					case oid_type::float4_array:
					case oid_type::float8_array:
					case oid_type::money_array:
					case oid_type::numeric_array:
					case oid_type::bytea_array:
					case oid_type::any:
					case oid_type::name:
					case oid_type::text:
					case oid_type::date:
					case oid_type::time:
					case oid_type::UUID:
					case oid_type::cstring:
					case oid_type::bp_char:
					case oid_type::var_char:
					case oid_type::bit:
					case oid_type::var_bit:
					default:
						return core::var::string(std::string_view(data, (size_t)size));
				}
			}
			static core::schema* to_array(const char* data, int size, uint32_t id)
			{
				std::pair<core::schema*, Oid> context;
				context.first = core::var::set::array();
				context.second = id;

				array_filter filter(data, (size_t)size);
				if (!filter.parse(&context, to_array_field))
				{
					core::memory::release(context.first);
					return new core::schema(core::var::string(std::string_view(data, (size_t)size)));
				}

				return context.first;
			}
			core::schema* to_schema(const char* data, int size, uint32_t id)
			{
				if (!data)
					return nullptr;

				oid_type type = (oid_type)id;
				switch (type)
				{
					case oid_type::JSON:
					case oid_type::JSONB:
					{
						auto result = core::schema::convert_from_json(std::string_view(data, (size_t)size));
						if (result)
							return *result;

						return new core::schema(core::var::string(std::string_view(data, (size_t)size)));
					}
					case oid_type::any_array:
						return to_array(data, size, (Oid)oid_type::any);
					case oid_type::name_array:
						return to_array(data, size, (Oid)oid_type::name);
						break;
					case oid_type::text_array:
						return to_array(data, size, (Oid)oid_type::name);
						break;
					case oid_type::date_array:
						return to_array(data, size, (Oid)oid_type::name);
						break;
					case oid_type::time_array:
						return to_array(data, size, (Oid)oid_type::name);
						break;
					case oid_type::uuid_array:
						return to_array(data, size, (Oid)oid_type::UUID);
						break;
					case oid_type::cstring_array:
						return to_array(data, size, (Oid)oid_type::cstring);
						break;
					case oid_type::bp_char_array:
						return to_array(data, size, (Oid)oid_type::bp_char);
						break;
					case oid_type::var_char_array:
						return to_array(data, size, (Oid)oid_type::var_char);
						break;
					case oid_type::bit_array:
						return to_array(data, size, (Oid)oid_type::bit);
						break;
					case oid_type::var_bit_array:
						return to_array(data, size, (Oid)oid_type::var_bit);
						break;
					case oid_type::char_array:
						return to_array(data, size, (Oid)oid_type::symbol);
						break;
					case oid_type::int2_array:
						return to_array(data, size, (Oid)oid_type::int2);
						break;
					case oid_type::int4_array:
						return to_array(data, size, (Oid)oid_type::int4);
						break;
					case oid_type::int8_array:
						return to_array(data, size, (Oid)oid_type::int8);
						break;
					case oid_type::bool_array:
						return to_array(data, size, (Oid)oid_type::boolf);
						break;
					case oid_type::float4_array:
						return to_array(data, size, (Oid)oid_type::float4);
						break;
					case oid_type::float8_array:
						return to_array(data, size, (Oid)oid_type::float8);
						break;
					case oid_type::money_array:
						return to_array(data, size, (Oid)oid_type::money);
						break;
					case oid_type::numeric_array:
						return to_array(data, size, (Oid)oid_type::numeric);
						break;
					case oid_type::bytea_array:
						return to_array(data, size, (Oid)oid_type::bytea);
						break;
					default:
						return new core::schema(to_variant(data, size, id));
				}
			}
#endif
			database_exception::database_exception(tconnection* connection)
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(connection != nullptr, "base should be set");
				char* new_message = PQerrorMessage(connection);
				if (!new_message || new_message[0] == '\0')
					return;

				core::string buffer(new_message);
				buffer.erase(buffer.size() - 1);
				for (auto& item : core::stringify::split(buffer, '\n'))
				{
					if (item.empty())
						continue;
					if (error_message.empty())
						error_message += item;
					else
						error_message += ", " + item;
				}
#else
				error_message = "not supported";
#endif
			}
			database_exception::database_exception(core::string&& new_message)
			{
				error_message = std::move(new_message);
			}
			const char* database_exception::type() const noexcept
			{
				return "postgresql_error";
			}

			address::address()
			{
			}
			void address::override(const std::string_view& key, const std::string_view& value)
			{
				params[core::string(key)] = value;
			}
			bool address::set(address_op key, const std::string_view& value)
			{
				core::string name = core::string(get_key_name(key));
				if (name.empty())
					return false;

				params[name] = value;
				return true;
			}
			core::string address::get(address_op key) const
			{
				auto it = params.find(core::key_lookup_cast(get_key_name(key)));
				if (it == params.end())
					return "";

				return it->second;
			}
			core::string address::get_address() const
			{
				core::string hostname = get(address_op::ip);
				if (hostname.empty())
					hostname = get(address_op::host);

				return "postgresql://" + hostname + ':' + get(address_op::port) + '/';
			}
			const core::unordered_map<core::string, core::string>& address::get() const
			{
				return params;
			}
			std::string_view address::get_key_name(address_op key)
			{
				switch (key)
				{
					case address_op::host:
						return "host";
					case address_op::ip:
						return "hostaddr";
					case address_op::port:
						return "port";
					case address_op::database:
						return "dbname";
					case address_op::user:
						return "user";
					case address_op::password:
						return "password";
					case address_op::timeout:
						return "connect_timeout";
					case address_op::encoding:
						return "client_encoding";
					case address_op::options:
						return "options";
					case address_op::profile:
						return "application_name";
					case address_op::fallback_profile:
						return "fallback_application_name";
					case address_op::keep_alive:
						return "keepalives";
					case address_op::keep_alive_idle:
						return "keepalives_idle";
					case address_op::keep_alive_interval:
						return "keepalives_interval";
					case address_op::keep_alive_count:
						return "keepalives_count";
					case address_op::TTY:
						return "tty";
					case address_op::SSL:
						return "sslmode";
					case address_op::SSL_Compression:
						return "sslcompression";
					case address_op::SSL_Cert:
						return "sslcert";
					case address_op::SSL_Root_Cert:
						return "sslrootcert";
					case address_op::SSL_Key:
						return "sslkey";
					case address_op::SSL_CRL:
						return "sslcrl";
					case address_op::require_peer:
						return "requirepeer";
					case address_op::require_ssl:
						return "requiressl";
					case address_op::krb_server_name:
						return "krbservname";
					case address_op::service:
						return "service";
					default:
						return "";
				}
			}
			const char** address::create_keys() const
			{
				const char** result = core::memory::allocate<const char*>(sizeof(const char*) * (params.size() + 1));
				size_t index = 0;

				for (auto& key : params)
					result[index++] = key.first.c_str();

				result[index] = nullptr;
				return result;
			}
			const char** address::create_values() const
			{
				const char** result = core::memory::allocate<const char*>(sizeof(const char*) * (params.size() + 1));
				size_t index = 0;

				for (auto& key : params)
					result[index++] = key.second.c_str();

				result[index] = nullptr;
				return result;
			}
			expects_db<address> address::from_url(const std::string_view& location)
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(core::stringify::is_cstring(location), "location should be set");
				char* message = nullptr;
				PQconninfoOption* result = PQconninfoParse(location.data(), &message);
				if (!result)
				{
					database_exception exception("address parser error: " + core::string(message));
					PQfreemem(message);
					return exception;
				}

				address target;
				size_t index = 0;
				while (result[index].keyword != nullptr)
				{
					auto& info = result[index];
					target.params[info.keyword] = info.val ? info.val : "";
					index++;
				}
				PQconninfoFree(result);
				return target;
#else
				return database_exception("address parser: not supported");
#endif
			}

			notify::notify(tnotify* base) : pid(0)
			{
#ifdef VI_POSTGRESQL
				if (!base)
					return;

				if (base->relname != nullptr)
					name = base->relname;

				if (base->extra != nullptr)
					data = base->extra;

				pid = base->be_pid;
#endif
			}
			core::schema* notify::get_schema() const
			{
#ifdef VI_POSTGRESQL
				if (data.empty())
					return nullptr;

				auto result = core::schema::convert_from_json(data);
				return result ? *result : nullptr;
#else
				return nullptr;
#endif
			}
			core::string notify::get_name() const
			{
				return name;
			}
			core::string notify::get_data() const
			{
				return data;
			}
			int notify::get_pid() const
			{
				return pid;
			}

			column::column(tresponse* new_base, size_t fRowIndex, size_t fColumnIndex) : base(new_base), row_index(fRowIndex), column_index(fColumnIndex)
			{
			}
			core::string column::get_name() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(column_index != std::numeric_limits<size_t>::max(), "column should be valid");

				char* text = PQfname(base, (int)column_index);
				if (!text)
					return core::string();

				return text;
#else
				return core::string();
#endif
			}
			core::string column::get_value_text() const
			{
				return core::string(get_raw(), raw_size());
			}
			core::variant column::get() const
			{
#ifdef VI_POSTGRESQL
				if (!base || row_index == std::numeric_limits<size_t>::max() || column_index == std::numeric_limits<size_t>::max())
					return core::var::undefined();

				if (PQgetisnull(base, (int)row_index, (int)column_index) == 1)
					return core::var::null();

				char* data = PQgetvalue(base, (int)row_index, (int)column_index);
				int size = PQgetlength(base, (int)row_index, (int)column_index);
				Oid type = PQftype(base, (int)column_index);

				return to_variant(data, size, type);
#else
				return core::var::undefined();
#endif
			}
			core::schema* column::get_inline() const
			{
#ifdef VI_POSTGRESQL
				if (!base || row_index == std::numeric_limits<size_t>::max() || column_index == std::numeric_limits<size_t>::max())
					return nullptr;

				if (PQgetisnull(base, (int)row_index, (int)column_index) == 1)
					return nullptr;

				char* data = PQgetvalue(base, (int)row_index, (int)column_index);
				int size = PQgetlength(base, (int)row_index, (int)column_index);
				Oid type = PQftype(base, (int)column_index);

				return to_schema(data, size, type);
#else
				return nullptr;
#endif
			}
			char* column::get_raw() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(column_index != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQgetvalue(base, (int)row_index, (int)column_index);
#else
				return nullptr;
#endif
			}
			int column::get_format_id() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(column_index != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQfformat(base, (int)column_index);
#else
				return 0;
#endif
			}
			int column::get_mod_id() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(column_index != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQfmod(base, (int)column_index);
#else
				return -1;
#endif
			}
			int column::get_table_index() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(column_index != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQftablecol(base, (int)column_index);
#else
				return -1;
#endif
			}
			object_id column::get_table_id() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(column_index != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQftable(base, (int)column_index);
#else
				return 0;
#endif
			}
			object_id column::get_type_id() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(column_index != std::numeric_limits<size_t>::max(), "column should be valid");

				return PQftype(base, (int)column_index);
#else
				return 0;
#endif
			}
			size_t column::index() const
			{
				return column_index;
			}
			size_t column::size() const
			{
#ifdef VI_POSTGRESQL
				if (!base || row_index == std::numeric_limits<size_t>::max() || column_index == std::numeric_limits<size_t>::max())
					return 0;

				int size = PQnfields(base);
				if (size < 0)
					return 0;

				return (size_t)size;
#else
				return 0;
#endif
			}
			size_t column::raw_size() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");
				VI_ASSERT(column_index != std::numeric_limits<size_t>::max(), "column should be valid");

				int size = PQgetlength(base, (int)row_index, (int)column_index);
				if (size < 0)
					return 0;

				return (size_t)size;
#else
				return 0;
#endif
			}
			row column::get_row() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");

				return row(base, row_index);
#else
				return row(nullptr, std::numeric_limits<size_t>::max());
#endif
			}
			bool column::nullable() const
			{
#ifdef VI_POSTGRESQL
				if (!base || row_index == std::numeric_limits<size_t>::max() || column_index == std::numeric_limits<size_t>::max())
					return true;

				return PQgetisnull(base, (int)row_index, (int)column_index) == 1;
#else
				return true;
#endif
			}

			row::row(tresponse* new_base, size_t fRowIndex) : base(new_base), row_index(fRowIndex)
			{
			}
			core::schema* row::get_object() const
			{
#ifdef VI_POSTGRESQL
				if (!base || row_index == std::numeric_limits<size_t>::max())
					return nullptr;

				int size = PQnfields(base);
				if (size <= 0)
					return core::var::set::object();

				core::schema* result = core::var::set::object();
				result->get_childs().reserve((size_t)size);

				for (int j = 0; j < size; j++)
				{
					char* name = PQfname(base, j);
					char* data = PQgetvalue(base, (int)row_index, j);
					int count = PQgetlength(base, (int)row_index, j);
					bool null = PQgetisnull(base, (int)row_index, j) == 1;
					Oid type = PQftype(base, j);

					if (!null)
						result->set(name ? name : core::to_string(j), to_schema(data, count, type));
					else
						result->set(name ? name : core::to_string(j), core::var::null());
				}

				return result;
#else
				return nullptr;
#endif
			}
			core::schema* row::get_array() const
			{
#ifdef VI_POSTGRESQL
				if (!base || row_index == std::numeric_limits<size_t>::max())
					return nullptr;

				int size = PQnfields(base);
				if (size <= 0)
					return core::var::set::array();

				core::schema* result = core::var::set::array();
				result->get_childs().reserve((size_t)size);

				for (int j = 0; j < size; j++)
				{
					char* data = PQgetvalue(base, (int)row_index, j);
					int count = PQgetlength(base, (int)row_index, j);
					bool null = PQgetisnull(base, (int)row_index, j) == 1;
					Oid type = PQftype(base, j);
					result->push(null ? core::var::set::null() : to_schema(data, count, type));
				}

				return result;
#else
				return nullptr;
#endif
			}
			size_t row::index() const
			{
				return row_index;
			}
			size_t row::size() const
			{
#ifdef VI_POSTGRESQL
				if (!base || row_index == std::numeric_limits<size_t>::max())
					return 0;

				int size = PQnfields(base);
				if (size < 0)
					return 0;

				return (size_t)size;
#else
				return 0;
#endif
			}
			column row::get_column(size_t index) const
			{
#ifdef VI_POSTGRESQL
				if (!base || row_index == std::numeric_limits<size_t>::max() || (int)index >= PQnfields(base))
					return column(base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				return column(base, row_index, index);
#else
				return column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());
#endif
			}
			column row::get_column(const std::string_view& name) const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");
				if (!base || row_index == std::numeric_limits<size_t>::max())
					return column(base, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				int index = PQfnumber(base, name.data());
				if (index < 0)
					return column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());

				return column(base, row_index, (size_t)index);
#else
				return column(nullptr, std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max());
#endif
			}
			bool row::get_columns(column* output, size_t output_size) const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(output != nullptr && output_size > 0, "output should be valid");
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(row_index != std::numeric_limits<size_t>::max(), "row should be valid");

				output_size = std::min(size(), output_size);
				for (size_t i = 0; i < output_size; i++)
					output[i] = column(base, row_index, i);

				return true;
#else
				return false;
#endif
			}

			response::response() : response(nullptr)
			{

			}
			response::response(tresponse* new_base) : base(new_base), failure(false)
			{
			}
			response::response(response&& other) : base(other.base), failure(other.failure)
			{
				other.base = nullptr;
				other.failure = false;
			}
			response::~response()
			{
#ifdef VI_POSTGRESQL
				if (base != nullptr)
					PQclear(base);
				base = nullptr;
#endif
			}
			response& response::operator =(response&& other)
			{
				if (&other == this)
					return *this;

#ifdef VI_POSTGRESQL
				if (base != nullptr)
					PQclear(base);
#endif
				base = other.base;
				failure = other.failure;
				other.base = nullptr;
				other.failure = false;
				return *this;
			}
			core::schema* response::get_array_of_objects() const
			{
#ifdef VI_POSTGRESQL
				core::schema* result = core::var::set::array();
				if (!base)
					return result;

				int rows_size = PQntuples(base);
				if (rows_size <= 0)
					return result;

				int columns_size = PQnfields(base);
				if (columns_size <= 0)
					return result;

				core::vector<std::pair<core::string, Oid>> meta;
				meta.reserve((size_t)columns_size);

				for (int j = 0; j < columns_size; j++)
				{
					char* name = PQfname(base, j);
					meta.emplace_back(std::make_pair(name ? name : core::to_string(j), PQftype(base, j)));
				}

				result->reserve((size_t)rows_size);
				for (int i = 0; i < rows_size; i++)
				{
					core::schema* subresult = core::var::set::object();
					subresult->get_childs().reserve((size_t)columns_size);

					for (int j = 0; j < columns_size; j++)
					{
						char* data = PQgetvalue(base, i, j);
						int size = PQgetlength(base, i, j);
						bool null = PQgetisnull(base, i, j) == 1;
						auto& field = meta[j];

						if (!null)
							subresult->set(field.first, to_schema(data, size, field.second));
						else
							subresult->set(field.first, core::var::null());
					}

					result->push(subresult);
				}

				return result;
#else
				return core::var::set::array();
#endif
			}
			core::schema* response::get_array_of_arrays() const
			{
#ifdef VI_POSTGRESQL
				core::schema* result = core::var::set::array();
				if (!base)
					return result;

				int rows_size = PQntuples(base);
				if (rows_size <= 0)
					return result;

				int columns_size = PQnfields(base);
				if (columns_size <= 0)
					return result;

				core::vector<Oid> meta;
				meta.reserve((size_t)columns_size);

				for (int j = 0; j < columns_size; j++)
					meta.emplace_back(PQftype(base, j));

				result->reserve((size_t)rows_size);
				for (int i = 0; i < rows_size; i++)
				{
					core::schema* subresult = core::var::set::array();
					subresult->get_childs().reserve((size_t)columns_size);

					for (int j = 0; j < columns_size; j++)
					{
						char* data = PQgetvalue(base, i, j);
						int size = PQgetlength(base, i, j);
						bool null = PQgetisnull(base, i, j) == 1;
						subresult->push(null ? core::var::set::null() : to_schema(data, size, meta[j]));
					}

					result->push(subresult);
				}

				return result;
#else
				return core::var::set::array();
#endif
			}
			core::schema* response::get_object(size_t index) const
			{
				return get_row(index).get_object();
			}
			core::schema* response::get_array(size_t index) const
			{
				return get_row(index).get_array();
			}
			core::vector<core::string> response::get_columns() const
			{
				core::vector<core::string> columns;
#ifdef VI_POSTGRESQL
				if (!base)
					return columns;

				int columns_size = PQnfields(base);
				if (columns_size <= 0)
					return columns;

				columns.reserve((size_t)columns_size);
				for (int j = 0; j < columns_size; j++)
				{
					char* name = PQfname(base, j);
					columns.emplace_back(name ? name : core::to_string(j));
				}
#endif
				return columns;
			}
			core::string response::get_command_status_text() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				char* text = PQcmdStatus(base);
				if (!text)
					return core::string();

				return text;
#else
				return core::string();
#endif
			}
			core::string response::get_status_text() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				char* text = PQresStatus(PQresultStatus(base));
				if (!text)
					return core::string();

				return text;
#else
				return core::string();
#endif
			}
			core::string response::get_error_text() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				char* text = PQresultErrorMessage(base);
				if (!text)
					return core::string();

				return text;
#else
				return core::string();
#endif
			}
			core::string response::get_error_field(field_code field) const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				int code;
				switch (field)
				{
					case field_code::severity:
						code = PG_DIAG_SEVERITY;
						break;
					case field_code::severity_nonlocalized:
						code = PG_DIAG_SEVERITY_NONLOCALIZED;
						break;
					case field_code::sql_state:
						code = PG_DIAG_SQLSTATE;
						break;
					case field_code::message_primary:
						code = PG_DIAG_MESSAGE_PRIMARY;
						break;
					case field_code::message_detail:
						code = PG_DIAG_MESSAGE_DETAIL;
						break;
					case field_code::message_hint:
						code = PG_DIAG_MESSAGE_HINT;
						break;
					case field_code::statement_position:
						code = PG_DIAG_STATEMENT_POSITION;
						break;
					case field_code::internal_position:
						code = PG_DIAG_INTERNAL_POSITION;
						break;
					case field_code::internal_query:
						code = PG_DIAG_INTERNAL_QUERY;
						break;
					case field_code::context:
						code = PG_DIAG_CONTEXT;
						break;
					case field_code::schema_name:
						code = PG_DIAG_SCHEMA_NAME;
						break;
					case field_code::table_name:
						code = PG_DIAG_TABLE_NAME;
						break;
					case field_code::column_name:
						code = PG_DIAG_COLUMN_NAME;
						break;
					case field_code::data_type_name:
						code = PG_DIAG_DATATYPE_NAME;
						break;
					case field_code::constraint_name:
						code = PG_DIAG_CONSTRAINT_NAME;
						break;
					case field_code::source_file:
						code = PG_DIAG_SOURCE_FILE;
						break;
					case field_code::source_line:
						code = PG_DIAG_SOURCE_LINE;
						break;
					case field_code::source_function:
						code = PG_DIAG_SOURCE_FUNCTION;
						break;
					default:
						return core::string();
				}

				char* text = PQresultErrorField(base, code);
				if (!text)
					return core::string();

				return text;
#else
				return core::string();
#endif
			}
			int response::get_name_index(const std::string_view& name) const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				VI_ASSERT(core::stringify::is_cstring(name), "name should be set");

				return PQfnumber(base, name.data());
#else
				return 0;
#endif
			}
			query_exec response::get_status() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				return (query_exec)PQresultStatus(base);
#else
				return query_exec::empty_query;
#endif
			}
			object_id response::get_value_id() const
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(base != nullptr, "context should be valid");
				return PQoidValue(base);
#else
				return 0;
#endif
			}
			size_t response::affected_rows() const
			{
#ifdef VI_POSTGRESQL
				if (!base)
					return 0;

				char* count = PQcmdTuples(base);
				if (!count || count[0] == '\0')
					return 0;

				auto numeric = core::from_string<uint64_t>(count);
				if (!numeric)
					return 0;

				return (size_t)*numeric;
#else
				return 0;
#endif
			}
			size_t response::size() const
			{
#ifdef VI_POSTGRESQL
				if (!base)
					return 0;

				int size = PQntuples(base);
				if (size < 0)
					return 0;

				return (size_t)size;
#else
				return 0;
#endif
			}
			row response::get_row(size_t index) const
			{
#ifdef VI_POSTGRESQL
				if (!base || (int)index >= PQntuples(base))
					return row(base, std::numeric_limits<size_t>::max());

				return row(base, index);
#else
				return row(nullptr, std::numeric_limits<size_t>::max());
#endif
			}
			row response::front() const
			{
				if (empty())
					return row(nullptr, std::numeric_limits<size_t>::max());

				return get_row(0);
			}
			row response::back() const
			{
				if (empty())
					return row(nullptr, std::numeric_limits<size_t>::max());

				return get_row(size() - 1);
			}
			response response::copy() const
			{
#ifdef VI_POSTGRESQL
				response result;
				if (!base)
					return result;

				result.failure = error();
				result.base = PQcopyResult(base, PG_COPYRES_ATTRS | PG_COPYRES_TUPLES | PG_COPYRES_EVENTS | PG_COPYRES_NOTICEHOOKS);

				return result;
#else
				return response();
#endif
			}
			tresponse* response::get() const
			{
				return base;
			}
			bool response::empty() const
			{
#ifdef VI_POSTGRESQL
				if (!base)
					return true;

				return PQntuples(base) <= 0;
#else
				return true;
#endif
			}
			bool response::error() const
			{
				if (failure)
					return true;
#ifdef VI_POSTGRESQL
				query_exec state = (base ? get_status() : query_exec::non_fatal_error);
				return state == query_exec::fatal_error || state == query_exec::non_fatal_error || state == query_exec::bad_response;
#else
				return false;
#endif
			}

			cursor::cursor() : cursor(nullptr, caching::never)
			{
			}
			cursor::cursor(connection* new_executor, caching status) : executor(new_executor), cache(status)
			{
				VI_WATCH(this, "pq-result cursor");
			}
			cursor::cursor(cursor&& other) : base(std::move(other.base)), executor(other.executor), cache(other.cache)
			{
				VI_WATCH(this, "pq-result cursor (moved)");
				other.executor = nullptr;
			}
			cursor::~cursor()
			{
				VI_UNWATCH(this);
			}
			cursor& cursor::operator =(cursor&& other)
			{
				if (&other == this)
					return *this;

				base = std::move(other.base);
				executor = other.executor;
				cache = other.cache;
				other.executor = nullptr;

				return *this;
			}
			bool cursor::success() const
			{
				return !error();
			}
			bool cursor::empty() const
			{
				if (base.empty())
					return true;

				for (auto& item : base)
				{
					if (!item.empty())
						return false;
				}

				return true;
			}
			bool cursor::error() const
			{
				if (base.empty())
					return true;

				for (auto& item : base)
				{
					if (item.error())
						return true;
				}

				return false;
			}
			size_t cursor::size() const
			{
				return base.size();
			}
			size_t cursor::affected_rows() const
			{
				size_t size = 0;
				for (auto& item : base)
					size += item.affected_rows();
				return size;
			}
			cursor cursor::copy() const
			{
				cursor result(executor, caching::cached);
				if (base.empty())
					return result;

				result.base.clear();
				result.base.reserve(base.size());

				for (auto& item : base)
					result.base.emplace_back(item.copy());

				return result;
			}
			const response& cursor::first() const
			{
				VI_ASSERT(!base.empty(), "index outside of range");
				return base.front();
			}
			const response& cursor::last() const
			{
				VI_ASSERT(!base.empty(), "index outside of range");
				return base.back();
			}
			const response& cursor::at(size_t index) const
			{
				VI_ASSERT(index < base.size(), "index outside of range");
				return base[index];
			}
			connection* cursor::get_executor() const
			{
				return executor;
			}
			caching cursor::get_cache_status() const
			{
				return cache;
			}
			core::schema* cursor::get_array_of_objects(size_t response_index) const
			{
				if (response_index >= base.size())
					return core::var::set::array();

				return base[response_index].get_array_of_objects();
			}
			core::schema* cursor::get_array_of_arrays(size_t response_index) const
			{
				if (response_index >= base.size())
					return core::var::set::array();

				return base[response_index].get_array_of_arrays();
			}
			core::schema* cursor::get_object(size_t response_index, size_t index) const
			{
				if (response_index >= base.size())
					return nullptr;

				return base[response_index].get_object(index);
			}
			core::schema* cursor::get_array(size_t response_index, size_t index) const
			{
				if (response_index >= base.size())
					return nullptr;

				return base[response_index].get_array(index);
			}

			connection::connection(tconnection* new_base, socket_t fd) : base(new_base), stream(new socket(fd)), current(nullptr), status(query_state::idle)
			{
			}
			connection::~connection() noexcept
			{
				core::memory::release(stream);
			}
			tconnection* connection::get_base() const
			{
				return base;
			}
			socket* connection::get_stream() const
			{
				return stream;
			}
			request* connection::get_current() const
			{
				return current;
			}
			query_state connection::get_state() const
			{
				return status;
			}
			transaction_state connection::get_tx_state() const
			{
#ifdef VI_POSTGRESQL
				switch (PQtransactionStatus(base))
				{
					case PQTRANS_IDLE:
						return transaction_state::idle;
					case PQTRANS_ACTIVE:
						return transaction_state::active;
					case PQTRANS_INTRANS:
						return transaction_state::in_transaction;
					case PQTRANS_INERROR:
						return transaction_state::in_error;
					case PQTRANS_UNKNOWN:
					default:
						return transaction_state::none;
				}
#else
				return transaction_state::none;
#endif
			}
			bool connection::in_transaction() const
			{
#ifdef VI_POSTGRESQL
				return PQtransactionStatus(base) != PQTRANS_IDLE;
#else
				return false;
#endif
			}
			bool connection::busy() const
			{
				return current != nullptr || status == query_state::busy || status == query_state::busy_in_transaction;
			}
			void connection::make_busy(request* data)
			{
				status = in_transaction() ? query_state::busy_in_transaction : query_state::busy;
				current = data;
			}
			request* connection::make_idle()
			{
				request* copy = current;
				status = in_transaction() ? query_state::idle_in_transaction : query_state::idle;
				current = nullptr;
				return copy;
			}
			request* connection::make_lost()
			{
				request* copy = current;
				status = query_state::lost;
				current = nullptr;
				return copy;
			}

			request::request(const std::string_view& commands, session_id new_session, caching status, uint64_t rid, size_t new_options) : command(commands.begin(), commands.end()), time(core::schedule::get_clock()), session(new_session), result(nullptr, status), id(rid), options(new_options)
			{
				command.emplace_back('\0');
			}
			void request::report_cursor()
			{
				if (callback)
					callback(result);
				if (future.is_pending())
					future.set(std::move(result));
			}
			void request::report_failure()
			{
				if (future.is_pending())
					future.set(cursor());
			}
			cursor&& request::get_result()
			{
				return std::move(result);
			}
			const core::vector<char>& request::get_command() const
			{
				return command;
			}
			session_id request::get_session() const
			{
				return session;
			}
			uint64_t request::get_timing() const
			{
				return (uint64_t)((core::schedule::get_clock() - time).count() / 1000);
			}
			uint64_t request::get_id() const
			{
				return id;
			}
			bool request::pending() const
			{
				return future.is_pending();
			}

			cluster::cluster()
			{
				multiplexer::get()->activate();
			}
			cluster::~cluster() noexcept
			{
				core::umutex<std::recursive_mutex> unique(update);
#ifdef VI_POSTGRESQL
				for (auto& item : pool)
				{
					item.first->clear_events(false);
					PQfinish(item.second->base);
					core::memory::release(item.second);
				}
#endif
				for (auto* item : requests)
				{
					item->report_failure();
					core::memory::release(item);
				}
				if (network::multiplexer::has_instance())
					multiplexer::get()->deactivate();
			}
			void cluster::clear_cache()
			{
				core::umutex<std::mutex> unique(cache.context);
				cache.objects.clear();
			}
			void cluster::set_cache_cleanup(uint64_t interval)
			{
				cache.cleanup_duration = interval;
			}
			void cluster::set_cache_duration(query_op cache_id, uint64_t duration)
			{
				switch (cache_id)
				{
					case query_op::cache_short:
						cache.short_duration = duration;
						break;
					case query_op::cache_mid:
						cache.mid_duration = duration;
						break;
					case query_op::cache_long:
						cache.long_duration = duration;
						break;
					default:
						VI_ASSERT(false, "cache id should be valid cache category");
						break;
				}
			}
			void cluster::set_when_reconnected(const on_reconnect& new_callback)
			{
				core::umutex<std::recursive_mutex> unique(update);
				reconnected = new_callback;
			}
			uint64_t cluster::add_channel(const std::string_view& name, const on_notification& new_callback)
			{
				VI_ASSERT(new_callback != nullptr, "callback should be set");

				uint64_t id = channel++;
				core::umutex<std::recursive_mutex> unique(update);
				listeners[core::string(name)][id] = new_callback;
				return id;
			}
			bool cluster::remove_channel(const std::string_view& name, uint64_t id)
			{
				core::umutex<std::recursive_mutex> unique(update);
				auto& base = listeners[core::string(name)];
				auto it = base.find(id);
				if (it == base.end())
					return false;

				base.erase(it);
				return true;
			}
			expects_promise_db<session_id> cluster::tx_begin(isolation type)
			{
				switch (type)
				{
					case isolation::serializable:
						return tx_start("BEGIN ISOLATION LEVEL SERIALIZABLE");
					case isolation::repeatable_read:
						return tx_start("BEGIN ISOLATION LEVEL REPEATABLE READ");
					case isolation::read_uncommited:
						return tx_start("BEGIN ISOLATION LEVEL READ UNCOMMITTED");
					case isolation::read_commited:
					default:
						return tx_start("BEGIN");
				}
			}
			expects_promise_db<session_id> cluster::tx_start(const std::string_view& command)
			{
				return query(command).then<expects_db<session_id>>([](expects_db<cursor>&& result) -> expects_db<session_id>
				{
					if (!result)
						return result.error();
					else if (result->success())
						return result->get_executor();

					return database_exception("transaction start error");
				});
			}
			expects_promise_db<void> cluster::tx_end(const std::string_view& command, session_id session)
			{
				return query(command, 0, session).then<expects_db<void>>([](expects_db<cursor>&& result) -> expects_db<void>
				{
					if (!result)
						return result.error();
					else if (result->success())
						return core::expectation::met;

					return database_exception("transaction end error");
				});
			}
			expects_promise_db<void> cluster::tx_commit(session_id session)
			{
				return tx_end("COMMIT", session);
			}
			expects_promise_db<void> cluster::tx_rollback(session_id session)
			{
				return tx_end("ROLLBACK", session);
			}
			expects_promise_db<void> cluster::connect(const address& location, size_t connections)
			{
#ifdef VI_POSTGRESQL
				VI_ASSERT(connections > 0, "connections count should be at least 1");
				if (!core::os::control::has(core::access_option::net))
					return expects_promise_db<void>(database_exception("connect failed: permission denied"));
#if OPENSSL_VERSION_MAJOR >= 3 && OPENSSL_VERSION_MINOR >= 2
				int version = PQlibVersion();
				if (version < 160002)
					return expects_promise_db<void>(database_exception(core::stringify::text("connect failed: libpq <= %.2f will cause a segfault starting with openssl 3.2", (double)version / 10000.0)));
#endif
				core::umutex<std::recursive_mutex> unique(update);
				source = location;
				if (!pool.empty())
					return disconnect().then<expects_promise_db<void>>([this, location, connections](expects_db<void>&&) { return this->connect(location, connections); });

				return core::cotask<expects_db<void>>([this, connections]() -> expects_db<void>
				{
					VI_MEASURE(core::timings::intensive);
					const char** keys = source.create_keys();
					const char** values = source.create_values();
					core::umutex<std::recursive_mutex> unique(update);
					core::unordered_map<socket_t, tconnection*> queue;
					core::vector<network::utils::poll_fd> sockets;
					tconnection* error = nullptr;

					VI_DEBUG("[pq] try connect using %i connections", (int)connections);
					queue.reserve(connections);

					auto& props = source.get();
					auto hostname = props.find("host");
					auto port = props.find("port");
					auto connect_timeout_value = props.find("connect_timeout");
					auto connect_timeout_seconds = connect_timeout_value == props.end() ? core::expects_io<uint64_t>(std::make_error_condition(std::errc::invalid_argument)) : core::from_string<uint64_t>(connect_timeout_value->second);
					core::string address = core::stringify::text("%s:%s", hostname != props.end() ? hostname->second.c_str() : "0.0.0.0", port != props.end() ? port->second.c_str() : "5432");
					uint64_t connect_timeout = (connect_timeout_seconds ? *connect_timeout_seconds : 10);
					time_t connect_initiated = time(nullptr);

					for (size_t i = 0; i < connections; i++)
					{
						tconnection* base = PQconnectStartParams(keys, values, 0);
						if (!base)
							goto failure;

						network::utils::poll_fd fd;
						fd.fd = (socket_t)PQsocket(base);
						queue[fd.fd] = base;
						sockets.emplace_back(std::move(fd));

						VI_DEBUG("[pq] try connect to %s on 0x%" PRIXPTR, address.c_str(), base);
						if (PQstatus(base) == ConnStatusType::CONNECTION_BAD)
						{
							error = base;
							goto failure;
						}
					}

					do
					{
						for (auto it = sockets.begin(); it != sockets.end(); it++)
						{
							network::utils::poll_fd& fd = *it; tconnection* base = queue[fd.fd];
							if (fd.events == 0 || fd.returns & network::utils::input || fd.returns & network::utils::output)
							{
								bool ready = false;
								switch (PQconnectPoll(base))
								{
									case PGRES_POLLING_ACTIVE:
										break;
									case PGRES_POLLING_FAILED:
										error = base;
										goto failure;
									case PGRES_POLLING_WRITING:
										fd.events = network::utils::output;
										break;
									case PGRES_POLLING_READING:
										fd.events = network::utils::input;
										break;
									case PGRES_POLLING_OK:
									{
										VI_DEBUG("[pq] OK connect on 0x%" PRIXPTR, (uintptr_t)base);
										PQsetNoticeProcessor(base, PQlogNotice, nullptr);
										PQsetnonblocking(base, 1);

										connection* next = new connection(base, fd.fd);
										pool.insert(std::make_pair(next->stream, next));
										queue.erase(fd.fd);
										sockets.erase(it);
										reprocess(next);
										ready = true;
										break;
									}
								}

								if (ready)
									break;
							}
							else if (fd.returns & network::utils::error || fd.returns & network::utils::hangup)
							{
								error = base;
								goto failure;
							}
						}

						if (time(nullptr) - connect_initiated >= (time_t)connect_timeout)
						{
							for (auto& base : queue)
								PQfinish(base.second);
							core::memory::deallocate(keys);
							core::memory::deallocate(values);
							return database_exception(core::stringify::text("connection to %s has timed out (took %" PRIu64 " seconds)", address.c_str(), connect_timeout));
						}
					} while (!sockets.empty() && network::utils::poll(sockets.data(), (int)sockets.size(), 50) >= 0);

					core::memory::deallocate(keys);
					core::memory::deallocate(values);
					return core::expectation::met;
				failure:
					database_exception exception = database_exception(error);
					for (auto& base : queue)
						PQfinish(base.second);

					core::memory::deallocate(keys);
					core::memory::deallocate(values);
					return exception;
				});
#else
				return expects_promise_db<void>(database_exception("connect: not supported"));
#endif
			}
			expects_promise_db<void> cluster::disconnect()
			{
#ifdef VI_POSTGRESQL
				if (pool.empty())
					return expects_promise_db<void>(database_exception("disconnect: not connected"));

				return core::cotask<expects_db<void>>([this]() -> expects_db<void>
				{
					core::umutex<std::recursive_mutex> unique(update);
					for (auto& item : pool)
					{
						item.first->clear_events(false);
						PQfinish(item.second->base);
						core::memory::release(item.second);
					}

					pool.clear();
					return core::expectation::met;
				});
#else
				return expects_promise_db<void>(database_exception("disconnect: not supported"));
#endif
			}
			expects_promise_db<void> cluster::listen(const core::vector<core::string>& channels)
			{
				VI_ASSERT(!channels.empty(), "channels should not be empty");
				core::vector<core::string> actual;
				{
					core::umutex<std::recursive_mutex> unique(update);
					actual.reserve(channels.size());
					for (auto& item : channels)
					{
						if (!is_listens(item))
							actual.push_back(item);
					}
				}

				if (actual.empty())
					return expects_promise_db<void>(core::expectation::met);

				core::string command;
				for (auto& item : actual)
					command += "LISTEN " + item + ';';

				return query(command).then<expects_db<void>>([this, actual = std::move(actual)](expects_db<cursor>&& result) mutable -> expects_db<void>
				{
					if (!result)
						return result.error();

					connection* base = result->get_executor();
					if (!base || !result->success())
						return database_exception("listen error");

					core::umutex<std::recursive_mutex> unique(update);
					for (auto& item : actual)
						base->listens.insert(item);
					return core::expectation::met;
				});
			}
			expects_promise_db<void> cluster::unlisten(const core::vector<core::string>& channels)
			{
				VI_ASSERT(!channels.empty(), "channels should not be empty");
				core::unordered_map<connection*, core::string> commands;
				{
					core::umutex<std::recursive_mutex> unique(update);
					for (auto& item : channels)
					{
						connection* next = is_listens(item);
						if (next != nullptr)
						{
							commands[next] += "UNLISTEN " + item + ';';
							next->listens.erase(item);
						}
					}
				}

				if (commands.empty())
					return expects_promise_db<void>(core::expectation::met);

				return core::coasync<expects_db<void>>([this, commands = std::move(commands)]() mutable -> expects_promise_db<void>
				{
					size_t count = 0;
					for (auto& next : commands)
					{
						auto status = VI_AWAIT(query(next.second, 0, next.first));
						count += status && status->success() ? 1 : 0;
					}
					if (!count)
						coreturn expects_db<void>(database_exception("unlisten error"));

					coreturn expects_db<void>(core::expectation::met);
				});
			}
			expects_promise_db<cursor> cluster::emplace_query(const std::string_view& command, core::schema_list* map, size_t opts, session_id session)
			{
				auto pattern = driver::get()->emplace(this, command, map);
				if (!pattern)
					return expects_promise_db<cursor>(pattern.error());

				return query(*pattern, opts, session);
			}
			expects_promise_db<cursor> cluster::template_query(const std::string_view& name, core::schema_args* map, size_t opts, session_id session)
			{
				VI_DEBUG("[pq] template query %s", name.empty() ? "empty-query-name" : core::string(name).c_str());
				auto pattern = driver::get()->get_query(this, name, map);
				if (!pattern)
					return expects_promise_db<cursor>(pattern.error());

				return query(*pattern, opts, session);
			}
			expects_promise_db<cursor> cluster::query(const std::string_view& command, size_t opts, session_id session)
			{
				VI_ASSERT(!command.empty(), "command should not be empty");
				core::string reference;
				bool may_cache = opts & (size_t)query_op::cache_short || opts & (size_t)query_op::cache_mid || opts & (size_t)query_op::cache_long;
				if (may_cache)
				{
					cursor result(nullptr, caching::cached);
					reference = get_cache_oid(command, opts);
					if (get_cache(reference, &result))
					{
						driver::get()->log_query(command);
						VI_DEBUG("[pq] OK execute on NULL (memory-cache)");
						return expects_promise_db<cursor>(std::move(result));
					}
				}
				else if (!is_managing(session))
					return expects_promise_db<cursor>(database_exception("supplied transaction id does not exist"));
				else
					driver::get()->log_query(command);

				request* next = new request(command, session, may_cache ? caching::miss : caching::never, ++counter, opts);
				if (!reference.empty())
					next->callback = [this, reference, opts](cursor& data) { set_cache(reference, &data, opts); };

				auto future = next->future;
				core::umutex<std::recursive_mutex> unique(update);
				requests.push_back(next);
				for (auto& item : pool)
				{
					if (consume(item.second))
						return future;
				}

				auto time = core::schedule::get_clock();
				auto timeout = std::chrono::milliseconds((uint64_t)core::timings::hangup);
				for (auto& item : pool)
				{
					if (item.second->busy() && item.second->current != nullptr && time - item.second->current->time > timeout)
						VI_WARN("[pq] stuck%s on 0x%" PRIXPTR " while executing query (rid: %" PRIu64 "):\n  %s", item.second->in_transaction() ? " in transaction" : "", (uintptr_t)item.second, item.second->current->id, item.second->current->command.data());
				}

				return future;
			}
			connection* cluster::get_connection(query_state state)
			{
				core::umutex<std::recursive_mutex> unique(update);
				for (auto& item : pool)
				{
					if (item.second->status == state)
						return item.second;
				}

				return nullptr;
			}
			connection* cluster::get_any_connection() const
			{
				for (auto& item : pool)
					return item.second;

				return nullptr;
			}
			bool cluster::is_managing(session_id session)
			{
				if (!session)
					return true;

				core::umutex<std::recursive_mutex> unique(update);
				for (auto& item : pool)
				{
					if (item.second == session)
						return true;
				}

				return false;
			}
			connection* cluster::is_listens(const std::string_view& name)
			{
				core::umutex<std::recursive_mutex> unique(update);
				auto copy = core::key_lookup_cast(name);
				for (auto& item : pool)
				{
					if (item.second->listens.count(copy) > 0)
						return item.second;
				}

				return nullptr;
			}
			core::string cluster::get_cache_oid(const std::string_view& payload, size_t opts)
			{
				auto hash = compute::crypto::hash_hex(compute::digests::sha256(), payload);
				core::string reference = hash ? compute::codec::hex_encode(*hash) : compute::codec::hex_encode(payload.substr(0, 32));
				if (opts & (size_t)query_op::cache_short)
					reference.append(".s");
				else if (opts & (size_t)query_op::cache_mid)
					reference.append(".m");
				else if (opts & (size_t)query_op::cache_long)
					reference.append(".l");

				return reference;
			}
			bool cluster::is_connected() const
			{
				return !pool.empty();
			}
			bool cluster::get_cache(const std::string_view& cache_oid, cursor* data)
			{
				VI_ASSERT(!cache_oid.empty(), "cache Oid should not be empty");
				VI_ASSERT(data != nullptr, "cursor should be set");

				core::umutex<std::mutex> unique(cache.context);
				auto it = cache.objects.find(core::key_lookup_cast(cache_oid));
				if (it == cache.objects.end())
					return false;

				int64_t time = ::time(nullptr);
				if (it->second.first < time)
				{
					*data = std::move(it->second.second);
					cache.objects.erase(it);
				}
				else
					*data = it->second.second.copy();

				return true;
			}
			void cluster::set_cache(const std::string_view& cache_oid, cursor* data, size_t opts)
			{
				VI_ASSERT(!cache_oid.empty(), "cache Oid should not be empty");
				VI_ASSERT(data != nullptr, "cursor should be set");

				int64_t time = ::time(nullptr);
				int64_t timeout = time;
				if (opts & (size_t)query_op::cache_short)
					timeout += cache.short_duration;
				else if (opts & (size_t)query_op::cache_mid)
					timeout += cache.mid_duration;
				else if (opts & (size_t)query_op::cache_long)
					timeout += cache.long_duration;

				core::umutex<std::mutex> unique(cache.context);
				if (cache.next_cleanup < time)
				{
					cache.next_cleanup = time + cache.cleanup_duration;
					for (auto it = cache.objects.begin(); it != cache.objects.end();)
					{
						if (it->second.first < time)
							it = cache.objects.erase(it);
						else
							++it;
					}
				}

				auto it = cache.objects.find(core::key_lookup_cast(cache_oid));
				if (it != cache.objects.end())
				{
					it->second.second = data->copy();
					it->second.first = timeout;
				}
				else
					cache.objects[core::string(cache_oid)] = std::make_pair(timeout, data->copy());
			}
			bool cluster::reestablish(connection* target)
			{
#ifdef VI_POSTGRESQL
				const char** keys = source.create_keys();
				const char** values = source.create_values();
				core::umutex<std::recursive_mutex> unique(update);
				auto* broken_request = target->make_lost();
				if (broken_request != nullptr)
				{
					VI_DEBUG("[pqerr] query reset on 0x%" PRIXPTR ": connection lost", (uintptr_t)target->base);
					core::codefer([broken_request]()
					{
						core::uptr<request> item = broken_request;
						item->report_failure();
					});
				}

				target->stream->clear_events(false);
				PQlogNoticeOf(target->base);
				PQfinish(target->base);

				VI_DEBUG("[pq] try reconnect on 0x%" PRIXPTR, (uintptr_t)target->base);
				target->base = PQconnectdbParams(keys, values, 0);
				core::memory::deallocate(keys);
				core::memory::deallocate(values);

				if (!target->base || PQstatus(target->base) != ConnStatusType::CONNECTION_OK)
				{
					if (target != nullptr)
						PQlogNoticeOf(target->base);

					core::codefer([this, target]() { reestablish(target); });
					return false;
				}

				core::vector<core::string> channels;
				channels.reserve(target->listens.size());
				for (auto& item : target->listens)
					channels.push_back(item);
				target->listens.clear();

				VI_DEBUG("[pq] OK reconnect on 0x%" PRIXPTR, (uintptr_t)target->base);
				target->make_idle();
				target->stream->migrate_to((socket_t)PQsocket(target->base));
				PQsetnonblocking(target->base, 1);
				PQsetNoticeProcessor(target->base, PQlogNotice, nullptr);
				consume(target);
				unique.negate();

				bool success = reprocess(target);
				if (!channels.empty())
				{
					if (reconnected)
					{
						reconnected(channels).when([this, channels](bool&& listen_to_channels)
						{
							if (listen_to_channels)
								listen(channels);
						});
					}
					else
						listen(channels);
				}
				else if (reconnected)
					reconnected(channels);

				return success;
#else
				return false;
#endif
			}
			bool cluster::consume(connection* base)
			{
#ifdef VI_POSTGRESQL
				core::umutex<std::recursive_mutex> unique(update);
				if (base->busy())
					return false;

				for (auto it = requests.begin(); it != requests.end(); ++it)
				{
					request* context = *it;
					if (context->session != nullptr)
					{
						if (context->session != base)
							continue;
					}
					else if (base->in_transaction())
						continue;

					context->result.executor = base;
					base->make_busy(context);
					requests.erase(it);
					break;
				}

				if (!base->busy())
					return false;

				VI_MEASURE(core::timings::intensive);
				VI_DEBUG("[pq] execute query on 0x%" PRIXPTR "%s (rid: %" PRIu64 "): %.64s%s", (uintptr_t)base, base->in_transaction() ? " (transaction)" : "", base->current->id, base->current->command.data(), base->current->command.size() > 64 ? " ..." : "");
				if (PQsendQuery(base->base, base->current->command.data()) == 1)
				{
					flush(base, false);
					return true;
				}

				auto* broken_request = base->make_idle();
				PQlogNoticeOf(base->base);
				core::codefer([broken_request]()
				{
					core::uptr<request> item = broken_request;
					item->report_failure();
				});
				return true;
#else
				return false;
#endif
			}
			bool cluster::reprocess(connection* source)
			{
				return multiplexer::get()->when_readable(source->stream, [this, source](socket_poll event)
				{
					if (packet::is_error(event))
						reestablish(source);
					else if (!packet::is_skip(event))
						dispatch(source);
				});
			}
			bool cluster::flush(connection* source, bool listen_for_results)
			{
#ifdef VI_POSTGRESQL
				if (PQflush(source->base) == 1)
				{
					source->stream->clear_events(false);
					return multiplexer::get()->when_writeable(source->stream, [this, source](socket_poll event)
					{
						if (!packet::is_skip(event))
							flush(source, true);
					});
				}
#endif
				if (listen_for_results)
					return reprocess(source);

				return true;
			}
			bool cluster::dispatch(connection* source)
			{
#ifdef VI_POSTGRESQL
				VI_MEASURE(core::timings::intensive);
				consume(source);
			retry:
				if (PQconsumeInput(source->base) != 1)
					return reestablish(source);
				else if (PQisBusy(source->base) != 0)
					return reprocess(source);

				core::umutex<std::recursive_mutex> unique(update);
				pgNotify* notification = nullptr;
				while ((notification = PQnotifies(source->base)) != nullptr)
				{
					if (notification != nullptr && notification->relname != nullptr)
					{
						auto it = listeners.find(notification->relname);
						if (it != listeners.end() && !it->second.empty())
						{
							notify event(notification);
							for (auto& item : it->second)
							{
								on_notification callback = item.second;
								core::codefer([event, callback = std::move(callback)]() { callback(event); });
							}
						}
						VI_DEBUG("[pq] notification on channel @%s: %s", notification->relname, notification->extra ? notification->extra : "[payload]");
						PQfreeNotify(notification);
					}
				}

				response chunk(PQgetResult(source->base));
				if (chunk.exists())
				{
					if (source->current != nullptr)
						source->current->result.base.emplace_back(std::move(chunk));
					goto retry;
				}

				PQlogNoticeOf(source->base);
				if (source->current != nullptr && !source->current->result.error())
					VI_DEBUG("[pq] OK execute on 0x%" PRIXPTR " (%" PRIu64 " ms, rid: %" PRIu64 ")", (uintptr_t)source, source->current->get_timing(), source->current->id);

				auto* ready_request = source->make_idle();
				if (ready_request != nullptr)
				{
					core::codefer([ready_request]()
					{
						core::uptr<request> item = ready_request;
						item->report_cursor();
					});
				}
				if (consume(source))
					goto retry;

				return reprocess(source);
#else
				return false;
#endif
			}

			expects_db<core::string> utils::inline_array(cluster* client, core::uptr<core::schema>&& array)
			{
				VI_ASSERT(client != nullptr, "cluster should be set");
				VI_ASSERT(array, "array should be set");

				core::schema_list map;
				core::string def;
				for (auto* item : array->get_childs())
				{
					if (item->value.is_object())
					{
						if (item->empty())
							continue;

						def += '(';
						for (auto* sub : item->get_childs())
						{
							sub->add_ref();
							map.emplace_back(sub);
							def += "?,";
						}
						def.erase(def.end() - 1);
						def += "),";
					}
					else
					{
						item->add_ref();
						map.emplace_back(item);
						def += "?,";
					}
				}

				auto result = pq::driver::get()->emplace(client, def, &map);
				if (result && !result->empty() && result->back() == ',')
					result->erase(result->end() - 1);

				return result;
			}
			expects_db<core::string> utils::inline_query(cluster* client, core::uptr<core::schema>&& where, const core::unordered_map<core::string, core::string>& whitelist, const std::string_view& placeholder)
			{
				VI_ASSERT(client != nullptr, "cluster should be set");
				VI_ASSERT(where, "array should be set");

				core::schema_list map;
				core::string allow = "abcdefghijklmnopqrstuvwxyz._", def;
				for (auto* statement : where->get_childs())
				{
					core::string op = statement->value.get_blob();
					if (op == "=" || op == "<>" || op == "<=" || op == "<" || op == ">" || op == ">=" || op == "+" || op == "-" || op == "*" || op == "/" || op == "(" || op == ")" || op == "TRUE" || op == "FALSE")
						def += op;
					else if (op == "~==")
						def += " LIKE ";
					else if (op == "~=")
						def += " ILIKE ";
					else if (op == "[")
						def += " ANY(";
					else if (op == "]")
						def += ")";
					else if (op == "&")
						def += " AND ";
					else if (op == "|")
						def += " OR ";
					else if (op == "TRUE")
						def += " TRUE ";
					else if (op == "FALSE")
						def += " FALSE ";
					else if (!op.empty())
					{
						if (op.front() == '@')
						{
							op = op.substr(1);
							if (!whitelist.empty())
							{
								auto it = whitelist.find(op);
								if (it != whitelist.end() && op.find_first_not_of(allow) == core::string::npos)
									def += it->second.empty() ? op : it->second;
							}
							else if (op.find_first_not_of(allow) == core::string::npos)
								def += op;
						}
						else if (!core::stringify::has_number(op))
						{
							statement->add_ref();
							map.emplace_back(statement);
							def += "?";
						}
						else
							def += op;
					}
				}

				auto result = pq::driver::get()->emplace(client, def, &map);
				if (result && result->empty())
					result = core::string(placeholder);

				return result;
			}
			core::string utils::get_char_array(connection* base, const std::string_view& src) noexcept
			{
#ifdef VI_POSTGRESQL
				if (src.empty())
					return "''";

				if (!base)
				{
					core::string dest(src);
					core::stringify::replace(dest, "'", "''");
					dest.insert(dest.begin(), '\'');
					dest.append(1, '\'');
					return dest;
				}

				char* subresult = PQescapeLiteral(base->get_base(), src.data(), src.size());
				core::string result(subresult);
				PQfreemem(subresult);

				return result;
#else
				core::string dest(src);
				core::stringify::replace(dest, "'", "''");
				dest.insert(dest.begin(), '\'');
				dest.append(1, '\'');
				return dest;
#endif
			}
			core::string utils::get_byte_array(connection* base, const std::string_view& src) noexcept
			{
#ifdef VI_POSTGRESQL
				if (src.empty())
					return "'\\x'::bytea";

				if (!base)
					return "'\\x" + compute::codec::hex_encode(src) + "'::bytea";

				size_t length = 0;
				char* subresult = (char*)PQescapeByteaConn(base->get_base(), (uint8_t*)src.data(), src.size(), &length);
				core::string result(subresult, length > 1 ? length - 1 : length);
				PQfreemem((uint8_t*)subresult);

				result.insert(result.begin(), '\'');
				result.append("'::bytea");
				return result;
#else
				return "'\\x" + compute::codec::hex_encode(src) + "'::bytea";
#endif
			}
			core::string utils::get_sql(connection* base, core::schema* source, bool escape, bool negate) noexcept
			{
				if (!source)
					return "NULL";

				core::schema* parent = source->get_parent();
				switch (source->value.get_type())
				{
					case core::var_type::object:
					{
						core::string result;
						core::schema::convert_to_json(source, [&result](core::var_form, const std::string_view& buffer) { result.append(buffer); });
						return escape ? get_char_array(base, result) : result;
					}
					case core::var_type::array:
					{
						core::string result = (parent && parent->value.get_type() == core::var_type::array ? "[" : "ARRAY[");
						for (auto* node : source->get_childs())
							result.append(get_sql(base, node, escape, negate)).append(1, ',');

						if (!source->empty())
							result = result.substr(0, result.size() - 1);

						return result + "]";
					}
					case core::var_type::string:
					{
						if (escape)
							return get_char_array(base, source->value.get_blob());

						return source->value.get_blob();
					}
					case core::var_type::integer:
						return core::to_string(negate ? -source->value.get_integer() : source->value.get_integer());
					case core::var_type::number:
						return core::to_string(negate ? -source->value.get_number() : source->value.get_number());
					case core::var_type::boolean:
						return (negate ? !source->value.get_boolean() : source->value.get_boolean()) ? "TRUE" : "FALSE";
					case core::var_type::decimal:
					{
						core::decimal value = source->value.get_decimal();
						if (value.is_nan())
							return "NULL";

						core::string result = (negate ? '-' + value.to_string() : value.to_string());
						return result.find('.') != core::string::npos ? result : result + ".0";
					}
					case core::var_type::binary:
						return get_byte_array(base, source->value.get_string());
					case core::var_type::null:
					case core::var_type::undefined:
						return "NULL";
					default:
						break;
				}

				return "NULL";
			}

			driver::driver() noexcept : active(false), logger(nullptr)
			{
				network::multiplexer::get()->activate();
			}
			driver::~driver() noexcept
			{
				network::multiplexer::get()->deactivate();
			}
			void driver::set_query_log(const on_query_log& callback) noexcept
			{
				logger = callback;
			}
			void driver::log_query(const std::string_view& command) noexcept
			{
				if (logger)
					logger(command);
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
					return database_exception("import empty query error: " + core::string(name));

				sequence result;
				result.request.assign(buffer);

				core::string lines = "\r\n";
				core::string enums = " \r\n\t\'\"()<>=%&^*/+-,!?:;";
				core::string erasable = " \r\n\t\'\"()<>=%&^*/+-,.!?:;";
				core::string quotes = "\"'`";

				core::string& base = result.request;
				core::stringify::replace_in_between(base, "/*", "*/", "", false);
				core::stringify::replace_starts_with_ends_of(base, "--", lines.c_str(), "");
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
							return database_exception("query expects @" + item.first + " constant: " + core::string(name));

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
					if (core::stringify::is_preceded_by(base, item.second.start, "-"))
					{
						item.first += ";negate";
						--item.second.start;
					}

					variables.emplace_back(std::move(item));
				}

				for (auto& item : core::stringify::find_in_between(base, "@<", ">", quotes.c_str()))
				{
					item.first += ";unsafe";
					if (core::stringify::is_preceded_by(base, item.second.start, "-"))
					{
						item.first += ";negate";
						--item.second.start;
					}

					variables.emplace_back(std::move(item));
				}

				core::stringify::replace_parts(base, variables, "", [&erasable](const std::string_view& name, char left, int side)
				{
					if (side < 0 && name.find(";negate") != core::string::npos)
						return '\0';

					return erasable.find(left) == core::string::npos ? ' ' : '\0';
				});

				for (auto& item : variables)
				{
					pose position;
					position.negate = item.first.find(";negate") != core::string::npos;
					position.escape = item.first.find(";escape") != core::string::npos;
					position.offset = item.second.start;
					position.key = item.first.substr(0, item.first.find(';'));
					result.positions.emplace_back(std::move(position));
				}

				if (variables.empty())
					result.cache = result.request;

				core::umutex<std::mutex> unique(exclusive);
				queries[core::string(name)] = std::move(result);
				return core::expectation::met;
			}
			expects_db<void> driver::add_directory(const std::string_view& directory, const std::string_view& origin)
			{
				core::vector<std::pair<core::string, core::file_entry>> entries;
				if (!core::os::directory::scan(directory, entries))
					return database_exception("import directory scan error: " + core::string(directory));

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

					if (!core::stringify::ends_with(base, ".sql"))
						continue;

					auto buffer = core::os::file::read_all(base, &size);
					if (!buffer)
						continue;

					core::stringify::replace(base, origin.empty() ? directory : origin, "");
					core::stringify::replace(base, "\\", "/");
					core::stringify::replace(base, ".sql", "");
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
					result.cache = data->get_var("cache").get_blob();
					result.request = data->get_var("request").get_blob();

					if (result.request.empty())
						result.request = result.cache;

					core::schema* positions = data->get("positions");
					if (positions != nullptr)
					{
						for (auto* position : positions->get_childs())
						{
							pose next;
							next.key = position->get_var(0).get_blob();
							next.offset = (size_t)position->get_var(1).get_integer();
							next.escape = position->get_var(2).get_boolean();
							next.negate = position->get_var(3).get_boolean();
							result.positions.emplace_back(std::move(next));
						}
					}

					core::string name = data->get_var("name").get_blob();
					queries[name] = std::move(result);
					++count;
				}

				if (count > 0)
					VI_DEBUG("[pq] OK load %" PRIu64 " parsed query templates", (uint64_t)count);

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

					if (query.second.cache.empty())
						data->set("request", core::var::string(query.second.request));
					else
						data->set("cache", core::var::string(query.second.cache));

					auto* positions = data->set("positions", core::var::set::array());
					for (auto& position : query.second.positions)
					{
						auto* next = positions->push(core::var::set::array());
						next->push(core::var::string(position.key));
						next->push(core::var::integer(position.offset));
						next->push(core::var::boolean(position.escape));
						next->push(core::var::boolean(position.negate));
					}
				}

				VI_DEBUG("[pq] OK save %" PRIu64 " parsed query templates", (uint64_t)queries.size());
				return result;
			}
			expects_db<core::string> driver::emplace(cluster* base, const std::string_view& SQL, core::schema_list* map) noexcept
			{
				if (!map || map->empty())
					return core::string(SQL);

				connection* remote = base->get_any_connection();
				core::string buffer(SQL);
				core::text_settle set;
				core::string& src = buffer;
				size_t offset = 0;
				size_t next = 0;

				while ((set = core::stringify::find(buffer, '?', offset)).found)
				{
					if (next >= map->size())
						return database_exception("query expects at least " + core::to_string(next + 1) + " arguments: " + core::string(buffer.substr(set.start, 64)));

					bool escape = true, negate = false;
					if (set.start > 0)
					{
						if (src[set.start - 1] == '\\')
						{
							offset = set.start;
							buffer.erase(set.start - 1, 1);
							continue;
						}
						else if (src[set.start - 1] == '$')
						{
							if (set.start > 1 && src[set.start - 2] == '-')
							{
								negate = true;
								set.start--;
							}

							escape = false;
							set.start--;
						}
						else if (src[set.start - 1] == '-')
						{
							negate = true;
							set.start--;
						}
					}
					core::string value = utils::get_sql(remote, *(*map)[next++], escape, negate);
					buffer.erase(set.start, (escape ? 1 : 2));
					buffer.insert(set.start, value);
					offset = set.start + value.size();
				}

				return src;
			}
			expects_db<core::string> driver::get_query(cluster* base, const std::string_view& name, core::schema_args* map) noexcept
			{
				core::umutex<std::mutex> unique(exclusive);
				auto it = queries.find(core::key_lookup_cast(name));
				if (it == queries.end())
					return database_exception("query not found: " + core::string(name));

				if (!it->second.cache.empty())
					return it->second.cache;

				if (!map || map->empty())
					return it->second.request;

				connection* remote = base->get_any_connection();
				sequence origin = it->second;
				size_t offset = 0;
				unique.negate();

				core::string& result = origin.request;
				for (auto& word : origin.positions)
				{
					auto it = map->find(word.key);
					if (it == map->end())
						return database_exception("query expects @" + word.key + " constant: " + core::string(name));

					core::string value = utils::get_sql(remote, *it->second, word.escape, word.negate);
					if (value.empty())
						continue;

					result.insert(word.offset + offset, value);
					offset += value.size();
				}

				if (result.empty())
					return database_exception("query construction error: " + core::string(name));

				return result;
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
