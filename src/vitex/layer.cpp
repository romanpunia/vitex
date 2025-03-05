#include "layer.h"
#include "layer/processors.h"
#include "network/http.h"
#include "network/mongo.h"
#include "network/pq.h"
#include "vitex.h"
#include <sstream>
#define CONTENT_BLOCKED_WAIT_MS 50
#define CONTENT_ARCHIVE_HEADER_MAGIC "2a20c37fc5ea31f2057d1bc3aa2ad7b8"
#define CONTENT_ARCHIVE_HASH_MAGIC "a6ef0892ed9fc12442a3ed68b0266b5a"
#define CONTENT_ARCHIVE_MAX_FILES 1024 * 1024 * 16
#define CONTENT_ARCHIVE_MAX_PATH 1024
#define CONTENT_ARCHIVE_MAX_SIZE 1024llu * 1024 * 1024 * 4

namespace vitex
{
	namespace layer
	{
		asset_file::asset_file(char* src_buffer, size_t src_size) noexcept : buffer(src_buffer), length(src_size)
		{
		}
		asset_file::~asset_file() noexcept
		{
			if (buffer != nullptr)
				core::memory::deallocate(buffer);
		}
		char* asset_file::get_buffer()
		{
			return buffer;
		}
		size_t asset_file::size()
		{
			return length;
		}

		content_exception::content_exception() : core::system_exception()
		{
		}
		content_exception::content_exception(const std::string_view& message) : core::system_exception(message)
		{
		}
		const char* content_exception::type() const noexcept
		{
			return "content_error";
		}

		void series::pack(core::schema* v, bool value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			v->set_attribute("b", core::var::boolean(value));
		}
		void series::pack(core::schema* v, int32_t value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			v->set_attribute("i", core::var::integer(value));
		}
		void series::pack(core::schema* v, int64_t value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			v->set_attribute("i", core::var::integer(value));
		}
		void series::pack(core::schema* v, uint32_t value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			v->set_attribute("i", core::var::integer(value));
		}
		void series::pack(core::schema* v, uint64_t value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			v->set_attribute("i", core::var::integer(value));
		}
		void series::pack(core::schema* v, float value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			v->set_attribute("n", core::var::number(value));
		}
		void series::pack(core::schema* v, double value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			v->set_attribute("n", core::var::number(value));
		}
		void series::pack(core::schema* v, const std::string_view& value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			v->set_attribute("s", core::var::string(value));
		}
		void series::pack(core::schema* v, const core::vector<bool>& value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			core::string_stream stream;
			for (auto&& it : value)
				stream << it << " ";

			v->set("b-array", core::var::string(stream.str().substr(0, stream.str().size() - 1)));
			v->set("size", core::var::integer((int64_t)value.size()));
		}
		void series::pack(core::schema* v, const core::vector<int32_t>& value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			core::string_stream stream;
			for (auto&& it : value)
				stream << it << " ";

			v->set("i-array", core::var::string(stream.str().substr(0, stream.str().size() - 1)));
			v->set("size", core::var::integer((int64_t)value.size()));
		}
		void series::pack(core::schema* v, const core::vector<int64_t>& value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			core::string_stream stream;
			for (auto&& it : value)
				stream << it << " ";

			v->set("i-array", core::var::string(stream.str().substr(0, stream.str().size() - 1)));
			v->set("size", core::var::integer((int64_t)value.size()));
		}
		void series::pack(core::schema* v, const core::vector<uint32_t>& value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			core::string_stream stream;
			for (auto&& it : value)
				stream << it << " ";

			v->set("i-array", core::var::string(stream.str().substr(0, stream.str().size() - 1)));
			v->set("size", core::var::integer((int64_t)value.size()));
		}
		void series::pack(core::schema* v, const core::vector<uint64_t>& value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			core::string_stream stream;
			for (auto&& it : value)
				stream << it << " ";

			v->set("i-array", core::var::string(stream.str().substr(0, stream.str().size() - 1)));
			v->set("size", core::var::integer((int64_t)value.size()));
		}
		void series::pack(core::schema* v, const core::vector<float>& value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			core::string_stream stream;
			for (auto&& it : value)
				stream << it << " ";

			v->set("n-array", core::var::string(stream.str().substr(0, stream.str().size() - 1)));
			v->set("size", core::var::integer((int64_t)value.size()));
		}
		void series::pack(core::schema* v, const core::vector<double>& value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			core::string_stream stream;
			for (auto&& it : value)
				stream << it << " ";

			v->set("n-array", core::var::string(stream.str().substr(0, stream.str().size() - 1)));
			v->set("size", core::var::integer((int64_t)value.size()));
		}
		void series::pack(core::schema* v, const core::vector<core::string>& value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			core::schema* array = v->set("s-array", core::var::array());
			for (auto&& it : value)
				array->set("s", core::var::string(it));

			v->set("size", core::var::integer((int64_t)value.size()));
		}
		void series::pack(core::schema* v, const core::unordered_map<size_t, size_t>& value)
		{
			VI_ASSERT(v != nullptr, "schema should be set");
			core::string_stream stream;
			for (auto&& it : value)
				stream << (uint64_t)it.first << " " << (uint64_t)it.second << " ";

			v->set("gl-array", core::var::string(stream.str().substr(0, stream.str().size() - 1)));
			v->set("size", core::var::integer((int64_t)value.size()));
		}
		bool series::unpack(core::schema* v, bool* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			*o = v->get_attribute_var("b").get_boolean();
			return true;
		}
		bool series::unpack(core::schema* v, int32_t* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			*o = (int)v->get_attribute_var("i").get_integer();
			return true;
		}
		bool series::unpack(core::schema* v, int64_t* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			*o = v->get_attribute_var("i").get_integer();
			return true;
		}
		bool series::unpack(core::schema* v, uint32_t* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			*o = (uint32_t)v->get_attribute_var("i").get_integer();
			return true;
		}
		bool series::unpack(core::schema* v, uint64_t* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			*o = (uint64_t)v->get_attribute_var("i").get_integer();
			return true;
		}
		bool series::unpack_a(core::schema* v, size_t* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			*o = (size_t)v->get_attribute_var("i").get_integer();
			return true;
		}
		bool series::unpack(core::schema* v, float* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			*o = (float)v->get_attribute_var("n").get_number();
			return true;
		}
		bool series::unpack(core::schema* v, double* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			*o = (int)v->get_attribute_var("n").get_number();
			return true;
		}
		bool series::unpack(core::schema* v, core::string* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			*o = v->get_attribute_var("s").get_blob();
			return true;
		}
		bool series::unpack(core::schema* v, core::vector<bool>* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			core::string array(v->get_var("b-array").get_blob());
			int64_t size = v->get_var("size").get_integer();
			if (array.empty() || !size)
				return false;

			core::string_stream stream(array);
			o->resize((size_t)size);

			for (auto it = o->begin(); it != o->end(); ++it)
			{
				bool item;
				stream >> item;
				*it = item;
			}

			return true;
		}
		bool series::unpack(core::schema* v, core::vector<int32_t>* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			core::string array(v->get_var("i-array").get_blob());
			int64_t size = v->get_var("size").get_integer();
			if (array.empty() || !size)
				return false;

			core::string_stream stream(array);
			o->resize((size_t)size);

			for (auto it = o->begin(); it != o->end(); ++it)
			{
				int item;
				stream >> item;
				*it = item;
			}

			return true;
		}
		bool series::unpack(core::schema* v, core::vector<int64_t>* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			core::string array(v->get_var("i-array").get_blob());
			int64_t size = v->get_var("size").get_integer();
			if (array.empty() || !size)
				return false;

			core::string_stream stream(array);
			o->resize((size_t)size);

			for (auto it = o->begin(); it != o->end(); ++it)
			{
				int64_t item;
				stream >> item;
				*it = item;
			}

			return true;
		}
		bool series::unpack(core::schema* v, core::vector<uint32_t>* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			core::string array(v->get_var("i-array").get_blob());
			int64_t size = v->get_var("size").get_integer();
			if (array.empty() || !size)
				return false;

			core::string_stream stream(array);
			o->resize((size_t)size);

			for (auto it = o->begin(); it != o->end(); ++it)
			{
				uint32_t item;
				stream >> item;
				*it = item;
			}

			return true;
		}
		bool series::unpack(core::schema* v, core::vector<uint64_t>* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			core::string array(v->get_var("i-array").get_blob());
			int64_t size = v->get_var("size").get_integer();
			if (array.empty() || !size)
				return false;

			core::string_stream stream(array);
			o->resize((size_t)size);

			for (auto it = o->begin(); it != o->end(); ++it)
			{
				uint64_t item;
				stream >> item;
				*it = item;
			}

			return true;
		}
		bool series::unpack(core::schema* v, core::vector<float>* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			core::string array(v->get_var("n-array").get_blob());
			int64_t size = v->get_var("size").get_integer();
			if (array.empty() || !size)
				return false;

			core::string_stream stream(array);
			o->resize((size_t)size);

			for (auto it = o->begin(); it != o->end(); ++it)
			{
				float item;
				stream >> item;
				*it = item;
			}

			return true;
		}
		bool series::unpack(core::schema* v, core::vector<double>* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			core::string array(v->get_var("n-array").get_blob());
			int64_t size = v->get_var("size").get_integer();
			if (array.empty() || !size)
				return false;

			core::string_stream stream(array);
			o->resize((size_t)size);

			for (auto it = o->begin(); it != o->end(); ++it)
			{
				double item;
				stream >> item;
				*it = item;
			}

			return true;
		}
		bool series::unpack(core::schema* v, core::vector<core::string>* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			core::schema* array = v->get("s-array");
			if (!array)
				return false;

			for (auto&& it : array->get_childs())
			{
				if (it->key == "s" && it->value.get_type() == core::var_type::string)
					o->push_back(it->value.get_blob());
			}

			return true;
		}
		bool series::unpack(core::schema* v, core::unordered_map<size_t, size_t>* o)
		{
			VI_ASSERT(o != nullptr, "output should be set");
			if (!v)
				return false;

			core::string array(v->get_var("gl-array").get_blob());
			int64_t size = v->get_var("size").get_integer();
			if (array.empty() || !size)
				return false;

			core::string_stream stream(array);
			o->reserve((size_t)size);

			for (size_t i = 0; i < (size_t)size; i++)
			{
				uint64_t global_index = 0;
				uint64_t local_index = 0;
				stream >> global_index;
				stream >> local_index;
				(*o)[global_index] = local_index;
			}

			return true;
		}

		core::promise<void> parallel::enqueue(core::task_callback&& callback)
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			return core::cotask<void>(std::move(callback));
		}
		core::vector<core::promise<void>> parallel::enqueue_all(core::vector<core::task_callback>&& callbacks)
		{
			VI_ASSERT(!callbacks.empty(), "callbacks should not be empty");
			core::vector<core::promise<void>> result;
			result.reserve(callbacks.size());

			for (auto& callback : callbacks)
				result.emplace_back(enqueue(std::move(callback)));

			return result;
		}
		void parallel::wait(core::promise<void>&& value)
		{
			value.wait();
		}
		void parallel::wail_all(core::vector<core::promise<void>>&& values)
		{
			for (auto& value : values)
				value.wait();
		}
		size_t parallel::get_thread_index()
		{
			return core::schedule::get()->get_thread_local_index();
		}
		size_t parallel::get_threads()
		{
			return core::schedule::get()->get_threads(core::difficulty::sync);
		}

		processor::processor(content_manager* new_content) noexcept : content(new_content)
		{
		}
		processor::~processor() noexcept
		{
		}
		void processor::free(asset_cache* asset)
		{
		}
		expects_content<void*> processor::duplicate(asset_cache* asset, const core::variant_args& args)
		{
			return content_exception("not implemented");
		}
		expects_content<void*> processor::deserialize(core::stream* stream, size_t offset, const core::variant_args& args)
		{
			return content_exception("not implemented");
		}
		expects_content<void> processor::serialize(core::stream* stream, void* object, const core::variant_args& args)
		{
			return content_exception("not implemented");
		}
		content_manager* processor::get_content() const
		{
			return content;
		}

		content_manager::content_manager() noexcept : queue(0)
		{
			auto directory = core::os::directory::get_working();
			if (!directory)
				return;

			directory = core::os::path::resolve_directory(directory->c_str());
			if (!directory)
				return;

			base = std::move(*directory);
			set_environment(base);
		}
		content_manager::~content_manager() noexcept
		{
			clear_cache();
			clear_archives();
			clear_streams();
			clear_processors();
		}
		void content_manager::clear_cache()
		{
			VI_TRACE("[content] clear cache on 0x%" PRIXPTR, (void*)this);
			core::umutex<std::mutex> unique(exclusive);
			for (auto& entries : assets)
			{
				for (auto& entry : entries.second)
				{
					if (!entry.second)
						continue;

					unique.negate();
					if (entry.first != nullptr)
						entry.first->free(entry.second);
					core::memory::deinit(entry.second);
					unique.negate();
				}
			}
			assets.clear();
		}
		void content_manager::clear_archives()
		{
			VI_TRACE("[content] clear archives on 0x%" PRIXPTR, (void*)this);
			core::umutex<std::mutex> unique(exclusive);
			for (auto it = archives.begin(); it != archives.end(); ++it)
				core::memory::deinit(it->second);
			archives.clear();
		}
		void content_manager::clear_streams()
		{
			VI_TRACE("[content] clear streams on 0x%" PRIXPTR, (void*)this);
			core::umutex<std::mutex> unique(exclusive);
			for (auto it = streams.begin(); it != streams.end(); ++it)
				it->first->release();
			streams.clear();
		}
		void content_manager::clear_processors()
		{
			VI_TRACE("[content] clear processors on 0x%" PRIXPTR, (void*)this);
			core::umutex<std::mutex> unique(exclusive);
			for (auto it = processors.begin(); it != processors.end(); ++it)
				core::memory::release(it->second);
			processors.clear();
		}
		void content_manager::clear_path(const std::string_view& path)
		{
			VI_TRACE("[content] clear path %.*s on 0x%" PRIXPTR, (int)path.size(), path.data(), (void*)this);
			auto file = core::os::path::resolve(path, environment, true);
			core::umutex<std::mutex> unique(exclusive);
			if (file)
			{
				auto it = assets.find(*file);
				if (it != assets.end())
					assets.erase(it);
			}

			auto it = assets.find(core::key_lookup_cast(path));
			if (it != assets.end())
				assets.erase(it);
		}
		void content_manager::set_environment(const std::string_view& path)
		{
			auto target = core::os::path::resolve_directory(path);
			if (!target)
				return;

			core::string new_path = *target;
			core::stringify::replace(new_path, '\\', '/');
			core::os::directory::set_working(new_path.c_str());
			core::umutex<std::mutex> unique(exclusive);
			environment = new_path;
		}
		expects_content<void> content_manager::import_archive(const std::string_view& path, bool validate_checksum)
		{
			auto target_path = core::os::path::resolve(path, environment, true);
			if (!target_path)
				return content_exception("archive was not found: " + core::string(path));

			core::uptr<core::stream> stream = core::os::file::open(*target_path, core::file_mode::binary_read_only).otherwise(nullptr);
			if (!stream)
				return content_exception("cannot open archive: " + core::string(path));

			uint8_t header[32] = { 0 }; size_t header_size = sizeof(header);
			if (stream->read(header, header_size).otherwise(0) != header_size || memcmp(header, CONTENT_ARCHIVE_HEADER_MAGIC, header_size) != 0)
				return content_exception("invalid archive header: " + core::string(path));

			uint64_t content_elements = 0;
			if (stream->read((uint8_t*)&content_elements, sizeof(uint64_t)).otherwise(0) != sizeof(uint64_t))
				return content_exception("invalid archive size: " + core::string(path) + " (size = " + core::to_string(content_elements) + ")");

			content_elements = core::os::hw::to_endianness<uint64_t>(core::os::hw::endian::little, content_elements);
			if (!content_elements || content_elements > CONTENT_ARCHIVE_MAX_FILES)
				return content_exception("invalid archive size: " + core::string(path) + " (size = " + core::to_string(content_elements) + ")");

			uint64_t content_offset = 0;
			core::vector<core::uptr<asset_archive>> content_files;
			content_files.reserve((size_t)content_elements);
			for (uint64_t i = 0; i < content_elements; i++)
			{
				uint64_t content_length = 0;
				if (stream->read((uint8_t*)&content_length, sizeof(uint64_t)).otherwise(0) != sizeof(uint64_t))
					return content_exception("invalid archive chunk length: " + core::string(path) + " (chunk = " + core::to_string(i) + ")");

				content_length = core::os::hw::to_endianness<uint64_t>(core::os::hw::endian::little, content_length);
				if (content_length > CONTENT_ARCHIVE_MAX_SIZE)
					return content_exception("invalid archive chunk length: " + core::string(path) + " (chunk = " + core::to_string(i) + ")");

				uint64_t path_length = 0;
				if (stream->read((uint8_t*)&path_length, sizeof(uint64_t)).otherwise(0) != sizeof(uint64_t))
					return content_exception("invalid archive chunk path size: " + core::string(path) + " (chunk = " + core::to_string(i) + ")");

				path_length = core::os::hw::to_endianness<uint64_t>(core::os::hw::endian::little, path_length);
				if (path_length > CONTENT_ARCHIVE_MAX_PATH)
					return content_exception("invalid archive chunk path size: " + core::string(path) + " (chunk = " + core::to_string(i) + ")");

				core::string path_value = core::string((size_t)path_length, '\0');
				if (stream->read((uint8_t*)path_value.c_str(), sizeof(char) * (size_t)path_length).otherwise(0) != (size_t)path_length)
					return content_exception("invalid archive chunk path data: " + core::string(path) + " (chunk = " + core::to_string(i) + ")");

				asset_archive* archive = core::memory::init<asset_archive>();
				archive->path = std::move(path_value);
				archive->offset = content_offset;
				archive->length = content_length;
				archive->stream = *stream;
				content_files.push_back(archive);
				content_offset += content_length;
			}

			size_t headers_offset = stream->tell().otherwise(0);
			if (validate_checksum)
			{
				size_t calculated_chunk = 0;
				core::string calculated_hash = CONTENT_ARCHIVE_HASH_MAGIC;
				for (auto& item : content_files)
				{
					size_t data_length = item->length;
					while (data_length > 0)
					{
						uint8_t data_buffer[core::CHUNK_SIZE];
						size_t data_size = std::min<size_t>(sizeof(data_buffer), data_length);
						size_t value_size = stream->read(data_buffer, data_size).otherwise(0);
						data_length -= value_size;
						if (!value_size)
							break;

						calculated_hash.append((char*)data_buffer, value_size);
						calculated_hash = *compute::crypto::hash_raw(compute::digests::SHA256(), calculated_hash);
						if (value_size < data_size)
							break;
					}

					if (data_length > 0)
						return content_exception("invalid archive chunk content data: " + core::string(path) + " (chunk = " + core::to_string(calculated_chunk) + ")");

					++calculated_chunk;
				}

				uint8_t requested_hash[64] = { 0 };
				calculated_hash = compute::codec::hex_encode(calculated_hash);
				if (stream->read(requested_hash, sizeof(requested_hash)).otherwise(0) != sizeof(requested_hash) || calculated_hash.size() != sizeof(requested_hash) || memcmp(requested_hash, calculated_hash.c_str(), sizeof(requested_hash)) != 0)
					return content_exception("invalid archive checksum: " + core::string(path) + " (calculated = " + calculated_hash + ")");
				stream->seek(core::file_seek::begin, (int64_t)headers_offset);
			}

			core::umutex<std::mutex> unique(exclusive);
			for (auto& item : content_files)
			{
				auto it = archives.find(item->path);
				if (it != archives.end())
				{
					core::memory::deinit(it->second);
					it->second = item.reset();
				}
				else
					archives[item->path] = item.reset();
			}
			streams[stream.reset()] = headers_offset;
			return core::expectation::met;
		}
		expects_content<void> content_manager::export_archive(const std::string_view& path, const std::string_view& physical_directory, const std::string_view& virtual_directory)
		{
			VI_ASSERT(!path.empty() && !physical_directory.empty(), "path and directory should not be empty");
			auto target_path = core::os::path::resolve(path, environment, true);
			if (!target_path)
				return content_exception("cannot resolve archive path: " + core::string(path));

			auto physical_volume = core::os::path::resolve(physical_directory, environment, true);
			if (!physical_volume)
				return content_exception("cannot resolve archive target path: " + core::string(physical_directory));

			size_t path_index = target_path->find(*physical_volume);
			if (path_index != std::string::npos)
			{
				auto copy = target_path->substr(path_index + physical_volume->size());
				if (copy.empty() || copy.front() != '.' || copy.find_first_of("\\/") != std::string::npos)
					return content_exception("export path overlaps physical directory: " + *target_path);
			}

			core::string virtual_volume = core::string(virtual_directory);
			core::stringify::replace(virtual_volume, '\\', '/');
			while (!virtual_volume.empty() && virtual_volume.front() == '/')
				virtual_volume.erase(virtual_volume.begin());

			core::uptr<core::stream> stream = core::os::file::open(*target_path, core::file_mode::binary_write_only).otherwise(nullptr);
			if (!stream)
				return content_exception("cannot open archive: " + core::string(path));

			uint8_t header[] = CONTENT_ARCHIVE_HEADER_MAGIC;
			if (stream->write(header, sizeof(header) - 1).otherwise(0) != sizeof(header) - 1)
				return content_exception("cannot write header: " + *physical_volume);

			core::uptr<core::file_tree> scanner = new core::file_tree(*physical_volume);
			size_t content_count = scanner->get_files();
			uint64_t content_elements = core::os::hw::to_endianness<uint64_t>(core::os::hw::endian::little, (uint64_t)content_count);
			if (stream->write((uint8_t*)&content_elements, sizeof(uint64_t)).otherwise(0) != sizeof(uint64_t) || content_elements > CONTENT_ARCHIVE_MAX_FILES)
				return content_exception("too many files: " + *physical_volume);

			size_t calculated_chunk = 0;
			core::string calculated_hash = CONTENT_ARCHIVE_HASH_MAGIC;
			expects_content<void> content_error = core::expectation::met;
			core::vector<std::pair<core::uptr<core::stream>, size_t>> content_files;
			core::stringify::replace(*physical_volume, '\\', '/');
			content_files.reserve(content_count);
			scanner->loop([&stream, &content_files, &content_error, &physical_volume, &virtual_volume](const core::file_tree* tree) mutable
			{
				core::stringify::replace(((core::file_tree*)tree)->path, '\\', '/');
				for (auto& physical_path : tree->files)
				{
					core::string virtual_path = tree->path + '/' + physical_path;
					core::uptr<core::stream> file = core::os::file::open(virtual_path, core::file_mode::binary_read_only).otherwise(nullptr);
					if (!file)
					{
						content_error = content_exception("cannot open content path: " + physical_path);
						return false;
					}

					size_t content_size = file->size().otherwise(0);
					uint64_t content_length = core::os::hw::to_endianness<uint64_t>(core::os::hw::endian::little, (uint64_t)content_size);
					if (content_length > CONTENT_ARCHIVE_MAX_SIZE || stream->write((uint8_t*)&content_length, sizeof(uint64_t)).otherwise(0) != sizeof(uint64_t))
					{
						content_error = content_exception("cannot write content length: " + physical_path);
						return false;
					}

					core::stringify::replace(virtual_path, *physical_volume, virtual_volume);
					core::stringify::replace(virtual_path, "//", "/");
					while (virtual_volume.empty() && !virtual_path.empty() && virtual_path.front() == '/')
						virtual_path.erase(virtual_path.begin());

					uint64_t path_length = core::os::hw::to_endianness<uint64_t>(core::os::hw::endian::little, (uint64_t)virtual_path.size());
					if (path_length > CONTENT_ARCHIVE_MAX_PATH || stream->write((uint8_t*)&path_length, sizeof(uint64_t)).otherwise(0) != sizeof(uint64_t))
					{
						content_error = content_exception("cannot write content path length: " + physical_path);
						return false;
					}
					else if (!virtual_path.empty() && stream->write((uint8_t*)virtual_path.c_str(), sizeof(char) * virtual_path.size()).otherwise(0) != virtual_path.size())
					{
						content_error = content_exception("cannot write content path data: " + physical_path);
						return false;
					}

					content_files.push_back(std::make_pair(*file, content_size));
				}
				return true;
			});

			if (!content_error)
				return content_error;

			for (auto& item : content_files)
			{
				size_t data_length = item.second;
				while (data_length > 0)
				{
					uint8_t data_buffer[core::CHUNK_SIZE];
					size_t data_size = std::min<size_t>(sizeof(data_buffer), data_length);
					size_t value_size = item.first->read(data_buffer, data_size).otherwise(0);
					data_length -= value_size;
					if (!value_size)
						break;

					if (stream->write(data_buffer, value_size).otherwise(0) != value_size)
						return content_exception("cannot write content data: " + core::string(item.first->virtual_name()) + " (chunk = " + core::to_string(calculated_chunk) + ")");

					calculated_hash.append((char*)data_buffer, value_size);
					calculated_hash = *compute::crypto::hash_raw(compute::digests::SHA256(), calculated_hash);
					if (value_size < data_size)
						break;
				}

				if (data_length > 0)
					return content_exception("cannot read content data: " + core::string(item.first->virtual_name()) + " (chunk = " + core::to_string(calculated_chunk) + ")");

				++calculated_chunk;
			}

			calculated_hash = compute::codec::hex_encode(calculated_hash);
			if (stream->write((uint8_t*)calculated_hash.data(), calculated_hash.size()).otherwise(0) != calculated_hash.size())
				return content_exception("cannot write archive checksum: " + core::string(path) + " (calculated = " + calculated_hash + ")");

			return core::expectation::met;
		}
		expects_content<void*> content_manager::load_from_archive(processor* processor, const std::string_view& path, const core::variant_args& map)
		{
			core::string file(path);
			core::stringify::replace(file, '\\', '/');
			core::stringify::replace(file, "./", "");

			core::umutex<std::mutex> unique(exclusive);
			auto archive = archives.find(file);
			if (archive == archives.end() || !archive->second || !archive->second->stream)
			{
				VI_TRACE("[content] archive was not found: %.*s", (int)path.size(), path.data());
				return content_exception("archive was not found: " + core::string(path));
			}

			unique.negate();
			{
				asset_cache* asset = find_cache(processor, file);
				if (asset != nullptr)
				{
					VI_TRACE("[content] load archived %.*s: cached", (int)path.size(), path.data());
					return processor->duplicate(asset, map);
				}
			}
			unique.negate();

			auto it = streams.find(archive->second->stream);
			if (it == streams.end())
				return content_exception("archived content does not contain: " + core::string(path));

			auto* stream = archive->second->stream;
			stream->set_virtual_name(file);
			stream->set_virtual_size(archive->second->length);
			stream->seek(core::file_seek::begin, it->second + archive->second->offset);
			unique.negate();

			VI_TRACE("[content] load archived: %.*s", (int)path.size(), path.data());
			return processor->deserialize(stream, it->second + archive->second->offset, map);
		}
		expects_content<void*> content_manager::load(processor* processor, const std::string_view& path, const core::variant_args& map)
		{
			if (path.empty())
			{
				VI_TRACE("[content] load from archive: no path provided");
				return content_exception("content path is empty");
			}

			if (!processor)
				return content_exception("content processor was not found: " + core::string(path));

			auto object = load_from_archive(processor, path, map);
			if (object && *object != nullptr)
			{
				VI_TRACE("[content] load from archive %.*s: OK", (int)path.size(), path.data());
				return object;
			}

			core::string file = core::string(path);
			if (!core::os::path::is_remote(file.c_str()))
			{
				auto subfile = core::os::path::resolve(file, environment, true);
				if (subfile && core::os::file::is_exists(subfile->c_str()))
				{
					file = *subfile;
					auto subtarget = environment + file;
					auto subpath = core::os::path::resolve(subtarget.c_str());
					if (subpath && core::os::file::is_exists(subpath->c_str()))
						file = *subpath;
				}

				if (file.empty())
					return content_exception("content was not found: " + core::string(path));
			}

			asset_cache* asset = find_cache(processor, file);
			if (asset != nullptr)
			{
				VI_TRACE("[content] load from archive %.*s: cached", (int)path.size(), path.data());
				return processor->duplicate(asset, map);
			}

			core::uptr<core::stream> stream = core::os::file::open(file, core::file_mode::binary_read_only).otherwise(nullptr);
			if (!stream)
			{
				VI_TRACE("[content] load from archive %.*s: non-existant", (int)path.size(), path.data());
				return content_exception("content was not found: " + core::string(path));
			}

			object = processor->deserialize(*stream, 0, map);
			VI_TRACE("[content] load from archive %.*s: %s", (int)path.size(), path.data(), object ? "OK" : object.error().what());
			return object;
		}
		expects_content<void> content_manager::save(processor* processor, const std::string_view& path, void* object, const core::variant_args& map)
		{
			VI_ASSERT(object != nullptr, "object should be set");
			if (path.empty())
			{
				VI_TRACE("[content] save forward: no path provided");
				return content_exception("content path is empty");
			}

			if (!processor)
				return content_exception("content processor was not found: " + core::string(path));

			core::string directory = core::os::path::get_directory(path);
			auto file = core::os::path::resolve(directory, environment, true);
			if (!file)
				return content_exception("cannot resolve saving path: " + core::string(path));

			core::string target = *file;
			target.append(path.substr(directory.size()));

			if (!target.empty())
				core::os::directory::patch(core::os::path::get_directory(target.c_str()));
			else
				core::os::directory::patch(core::os::path::get_directory(path));

			core::uptr<core::stream> stream = core::os::file::open(target, core::file_mode::binary_write_only).otherwise(nullptr);
			if (!stream)
			{
				stream = core::os::file::open(path, core::file_mode::binary_write_only).otherwise(nullptr);
				if (!stream)
					return content_exception("cannot open saving stream: " + core::string(path) + " or " + target);
			}

			auto result = processor->serialize(*stream, object, map);
			VI_TRACE("[content] save forward %.*s: %s", (int)path.size(), path.data(), result ? "OK" : result.error().what());
			return result;
		}
		expects_promise_content<void*> content_manager::load_deferred(processor* processor, const std::string_view& path, const core::variant_args& keys)
		{
			enqueue();
			core::string target_path = core::string(path);
			return core::cotask<expects_content<void*>>([this, processor, target_path, keys]()
			{
				auto result = load(processor, target_path, keys);
				dequeue();
				return result;
			});
		}
		expects_promise_content<void> content_manager::save_deferred(processor* processor, const std::string_view& path, void* object, const core::variant_args& keys)
		{
			enqueue();
			core::string target_path = core::string(path);
			return core::cotask<expects_content<void>>([this, processor, target_path, object, keys]()
			{
				auto result = save(processor, target_path, object, keys);
				dequeue();
				return result;
			});
		}
		processor* content_manager::add_processor(processor* value, uint64_t id)
		{
			core::umutex<std::mutex> unique(exclusive);
			auto it = processors.find(id);
			if (it != processors.end())
			{
				core::memory::release(it->second);
				it->second = value;
			}
			else
				processors[id] = value;

			return value;
		}
		processor* content_manager::get_processor(uint64_t id)
		{
			core::umutex<std::mutex> unique(exclusive);
			auto it = processors.find(id);
			if (it != processors.end())
				return it->second;

			return nullptr;
		}
		bool content_manager::remove_processor(uint64_t id)
		{
			core::umutex<std::mutex> unique(exclusive);
			auto it = processors.find(id);
			if (it == processors.end())
				return false;

			core::memory::release(it->second);
			processors.erase(it);
			return true;
		}
		void* content_manager::try_to_cache(processor* root, const std::string_view& path, void* resource)
		{
			VI_TRACE("[content] save 0x%" PRIXPTR " to cache", resource);
			core::string target = core::string(path);
			core::stringify::replace(target, '\\', '/');
			core::stringify::replace(target, environment, "./");
			core::umutex<std::mutex> unique(exclusive);
			auto& entries = assets[target];
			auto& entry = entries[root];
			if (entry != nullptr)
				return entry->resource;

			asset_cache* asset = core::memory::init<asset_cache>();
			asset->path = target;
			asset->resource = resource;
			entry = asset;
			return nullptr;
		}
		bool content_manager::is_busy()
		{
			core::umutex<std::mutex> unique(exclusive);
			return queue > 0;
		}
		void content_manager::enqueue()
		{
			core::umutex<std::mutex> unique(exclusive);
			++queue;
		}
		void content_manager::dequeue()
		{
			core::umutex<std::mutex> unique(exclusive);
			--queue;
		}
		const core::unordered_map<uint64_t, processor*>& content_manager::get_processors() const
		{
			return processors;
		}
		asset_cache* content_manager::find_cache(processor* target, const std::string_view& path)
		{
			if (path.empty())
				return nullptr;

			core::string rel_path = core::string(path);
			core::stringify::replace(rel_path, '\\', '/');
			core::stringify::replace(rel_path, environment, "./");
			core::umutex<std::mutex> unique(exclusive);
			auto it = assets.find(rel_path);
			if (it != assets.end())
			{
				auto kit = it->second.find(target);
				if (kit != it->second.end())
					return kit->second;
			}

			return nullptr;
		}
		asset_cache* content_manager::find_cache(processor* target, void* resource)
		{
			if (!resource)
				return nullptr;

			core::umutex<std::mutex> unique(exclusive);
			for (auto& it : assets)
			{
				auto kit = it.second.find(target);
				if (kit == it.second.end())
					continue;

				if (kit->second && kit->second->resource == resource)
					return kit->second;
			}

			return nullptr;
		}
		const core::string& content_manager::get_environment() const
		{
			return environment;
		}

		app_data::app_data(content_manager* manager, const std::string_view& new_path) noexcept : content(manager), data(nullptr)
		{
			VI_ASSERT(manager != nullptr, "content manager should be set");
			migrate(new_path);
		}
		app_data::~app_data() noexcept
		{
			core::memory::release(data);
		}
		void app_data::migrate(const std::string_view& next)
		{
			VI_ASSERT(!next.empty(), "path should not be empty");
			VI_TRACE("[appd] migrate %s to %.*s", path.c_str(), (int)next.size(), next.data());

			core::umutex<std::mutex> unique(exclusive);
			if (data != nullptr)
			{
				if (!path.empty())
					core::os::file::remove(path.c_str());
				write_app_data(next);
			}
			else
				read_app_data(next);
			path = next;
		}
		void app_data::set_key(const std::string_view& name, core::schema* value)
		{
			VI_TRACE("[appd] apply %.*s = %s", (int)name.size(), name.data(), value ? core::schema::to_json(value).c_str() : "NULL");
			core::umutex<std::mutex> unique(exclusive);
			if (!data)
				data = core::var::set::object();
			data->set(name, value);
			write_app_data(path);
		}
		void app_data::set_text(const std::string_view& name, const std::string_view& value)
		{
			set_key(name, core::var::set::string(value));
		}
		core::schema* app_data::get_key(const std::string_view& name)
		{
			core::umutex<std::mutex> unique(exclusive);
			if (!read_app_data(path))
				return nullptr;

			core::schema* result = data->get(name);
			if (result != nullptr)
				result = result->copy();
			return result;
		}
		core::string app_data::get_text(const std::string_view& name)
		{
			core::umutex<std::mutex> unique(exclusive);
			if (!read_app_data(path))
				return core::string();

			return data->get_var(name).get_blob();
		}
		bool app_data::has(const std::string_view& name)
		{
			core::umutex<std::mutex> unique(exclusive);
			if (!read_app_data(path))
				return false;

			return data->has(name);
		}
		bool app_data::read_app_data(const std::string_view& next)
		{
			if (data != nullptr)
				return true;

			if (next.empty())
				return false;

			data = content->load<core::schema>(next).otherwise(nullptr);
			return data != nullptr;
		}
		bool app_data::write_app_data(const std::string_view& next)
		{
			if (next.empty() || !data)
				return false;

			core::string type = "JSONB";
			auto type_id = core::os::path::get_extension(next);
			if (!type_id.empty())
			{
				type.assign(type_id);
				core::stringify::to_upper(type);
				if (type != "JSON" && type != "JSONB" && type != "XML")
					type = "JSONB";
			}

			core::variant_args args;
			args["type"] = core::var::string(type);

			return !!content->save<core::schema>(next, data, args);
		}
		core::schema* app_data::get_snapshot() const
		{
			return data;
		}

		application::application(desc* i) noexcept : control(i ? *i : desc())
		{
			VI_ASSERT(i != nullptr, "desc should be set");
			state = application_state::staging;
		}
		application::~application() noexcept
		{
			core::memory::release(vm);
			core::memory::release(content);
			core::memory::release(clock);
			application::unlink_instance(this);
			vitex::runtime::cleanup_instances();
		}
		core::promise<void> application::startup()
		{
			return core::promise<void>::null();
		}
		core::promise<void> application::shutdown()
		{
			return core::promise<void>::null();
		}
		void application::script_hook()
		{
		}
		void application::composition()
		{
		}
		void application::dispatch(core::timer* time)
		{
		}
		void application::publish(core::timer* time)
		{
		}
		void application::initialize()
		{
		}
		void application::loop_trigger()
		{
			VI_MEASURE(core::timings::infinite);
			core::schedule::desc& policy = control.scheduler;
			core::schedule* queue = core::schedule::get();
			if (policy.parallel)
			{
				while (state == application_state::active)
				{
					clock->begin();
					dispatch(clock);

					clock->finish();
					publish(clock);
				}

				while (content && content->is_busy())
					std::this_thread::sleep_for(std::chrono::milliseconds(CONTENT_BLOCKED_WAIT_MS));
			}
			else
			{
				while (state == application_state::active)
				{
					queue->dispatch();
					clock->begin();
					dispatch(clock);

					clock->finish();
					publish(clock);
				}
			}
		}
		int application::start()
		{
			composition();
			if (control.usage & (size_t)USE_PROCESSING)
			{
				if (!content)
					content = new content_manager();

				content->add_processor<processors::asset_processor, asset_file>();
				content->add_processor<processors::schema_processor, core::schema>();
				content->add_processor<processors::server_processor, network::http::server>();
				if (control.environment.empty())
				{
					auto directory = core::os::directory::get_working();
					if (directory)
						content->set_environment(*directory + control.directory);
				}
				else
					content->set_environment(control.environment + control.directory);

				if (!control.preferences.empty() && !database)
				{
					auto path = core::os::path::resolve(control.preferences, content->get_environment(), true);
					database = new app_data(content, path ? *path : control.preferences);
				}
			}

			if (control.usage & (size_t)USE_SCRIPTING && !vm)
				vm = new scripting::virtual_machine();

			clock = new core::timer();
			clock->set_fixed_frames(control.refreshrate.stable);
			clock->set_max_frames(control.refreshrate.limit);

			if (control.usage & (size_t)USE_NETWORKING)
			{
				if (network::multiplexer::has_instance())
					network::multiplexer::get()->rescale(control.polling_timeout, control.polling_events);
				else
					new network::multiplexer(control.polling_timeout, control.polling_events);
			}

			if (control.usage & (size_t)USE_SCRIPTING)
				script_hook();

			initialize();
			if (state == application_state::terminated)
				return exit_code != 0 ? exit_code : EXIT_JUMP + 6;

			state = application_state::active;
			core::schedule::desc& policy = control.scheduler;
			policy.initialize = [this](core::task_callback&& callback) { this->startup().when(std::move(callback)); };
			policy.ping = control.daemon ? std::bind(&application::status, this) : (core::activity_callback)nullptr;

			if (control.threads > 0)
			{
				core::schedule::desc launch = core::schedule::desc(control.threads);
				memcpy(policy.threads, launch.threads, sizeof(policy.threads));
			}

			auto* queue = core::schedule::get();
			queue->start(policy);
			clock->reset();
			loop_trigger();
			shutdown().wait();
			queue->stop();

			exit_code = (state == application_state::restart ? EXIT_RESTART : exit_code);
			state = application_state::terminated;
			return exit_code;
		}
		void application::stop(int code)
		{
			core::schedule* queue = core::schedule::get();
			state = application_state::terminated;
			exit_code = code;
			queue->wakeup();
		}
		void application::restart()
		{
			core::schedule* queue = core::schedule::get();
			state = application_state::restart;
			queue->wakeup();
		}
		application_state application::get_state() const
		{
			return state;
		}
		bool application::status(application* app)
		{
			return app->state == application_state::active;
		}
	}
}
