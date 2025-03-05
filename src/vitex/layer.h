#ifndef VI_LAYER_H
#define VI_LAYER_H
#include "scripting.h"
#include <atomic>
#include <cstdarg>

namespace vitex
{
	namespace layer
	{
		typedef std::function<void(class content_manager*, bool)> save_callback;

		class application;

		class content_manager;

		class processor;

		enum
		{
			USE_SCRIPTING = 1 << 0,
			USE_PROCESSING = 1 << 1,
			USE_NETWORKING = 1 << 2,
			EXIT_JUMP = 9600,
			EXIT_RESTART = EXIT_JUMP * 2
		};

		enum class application_state
		{
			terminated,
			staging,
			active,
			restart
		};

		struct asset_cache
		{
			core::string path;
			void* resource = nullptr;
		};

		struct asset_archive
		{
			core::stream* stream = nullptr;
			core::string path;
			size_t length = 0;
			size_t offset = 0;
		};

		class content_exception : public core::system_exception
		{
		public:
			content_exception();
			content_exception(const std::string_view& message);
			const char* type() const noexcept override;
		};

		template <typename v>
		using expects_content = core::expects<v, content_exception>;

		template <typename t, typename executor = core::parallel_executor>
		using expects_promise_content = core::basic_promise<expects_content<t>, executor>;

		class parallel
		{
		public:
			static core::promise<void> enqueue(core::task_callback&& callback);
			static core::vector<core::promise<void>> enqueue_all(core::vector<core::task_callback>&& callbacks);
			static void wait(core::promise<void>&& value);
			static void wail_all(core::vector<core::promise<void>>&& values);
			static size_t get_thread_index();
			static size_t get_threads();

		public:
			template <typename t>
			static core::vector<t> inline_wait_all(core::vector<core::promise<t>>&& values)
			{
				core::vector<t> results;
				results.reserve(values.size());
				for (auto& value : values)
					results.emplace_back(std::move(value.get()));
				return results;
			}
			template <typename function>
			static core::vector<core::promise<void>> for_loop(size_t size, size_t threshold_size, function callback)
			{
				core::vector<core::promise<void>> tasks;
				if (!size)
					return tasks;

				size_t threads = std::max<size_t>(1, get_threads());
				if (core::schedule::is_available() && threads > 1 && size > threshold_size)
				{
					size_t begin = 0, end = size;
					size_t step = size / threads;
					size_t remains = size % threads;
					tasks.reserve(threads);
					while (begin != end)
					{
						auto offset = begin;
						begin += remains > 0 ? --remains, step + 1 : step;
						tasks.emplace_back(core::cotask<void>([offset, begin, &callback]()
						{
							for (size_t i = offset; i < begin; i++)
								callback(i);
						}, false));
					}
				}
				else
				{
					for (size_t i = 0; i < size; i++)
						callback(i);
				}
				return tasks;
			}
			template <typename iterator, typename function>
			static core::vector<core::promise<void>> for_each(iterator begin, iterator end, size_t threshold_size, function callback)
			{
				core::vector<core::promise<void>> tasks;
				size_t size = end - begin;
				if (!size)
					return tasks;

				size_t threads = std::max<size_t>(1, get_threads());
				if (core::schedule::is_available() && threads > 1 && size > threshold_size)
				{
					size_t step = size / threads;
					size_t remains = size % threads;
					tasks.reserve(threads);
					while (begin != end)
					{
						auto offset = begin;
						begin += remains > 0 ? --remains, step + 1 : step;
						tasks.emplace_back(core::cotask<void>(std::bind(std::for_each<iterator, function>, offset, begin, callback), false));
					}
				}
				else
					std::for_each(begin, end, callback);
				return tasks;
			}
			template <typename iterator, typename function>
			static core::vector<core::promise<void>> for_each_sequential(iterator begin, iterator end, size_t size, size_t threshold_size, function callback)
			{
				core::vector<core::promise<void>> tasks;
				if (!size)
					return tasks;

				size_t threads = std::max<size_t>(1, get_threads());
				if (core::schedule::is_available() && threads > 1 && size > threshold_size)
				{
					size_t step = size / threads;
					size_t remains = size % threads;
					tasks.reserve(threads);
					while (begin != end)
					{
						auto offset = begin;
						size_t count = remains > 0 ? --remains, step + 1 : step;
						while (count-- > 0)
							++begin;
						tasks.emplace_back(core::cotask<void>(std::bind(std::for_each<iterator, function>, offset, begin, callback), false));
					}
				}
				else
					std::for_each(begin, end, callback);
				return tasks;
			}
			template <typename iterator, typename init_function, typename element_function>
			static core::vector<core::promise<void>> distribute(iterator begin, iterator end, size_t threshold_size, init_function&& init_callback, element_function&& element_callback)
			{
				core::vector<core::promise<void>> tasks;
				size_t size = end - begin;
				if (!size)
				{
					init_callback((size_t)0);
					return tasks;
				}

				size_t threads = std::max<size_t>(1, get_threads());
				if (core::schedule::is_available() && threads > 1 && size > threshold_size)
				{
					size_t step = size / threads;
					size_t remains = size % threads;
					size_t remainder = remains;
					size_t index = 0, counting = 0;
					auto start = begin;

					while (start != end)
					{
						start += remainder > 0 ? --remainder, step + 1 : step;
						++counting;
					}

					tasks.reserve(counting);
					init_callback(counting);

					while (begin != end)
					{
						auto offset = begin;
						auto bound = std::bind(element_callback, index++, std::placeholders::_1);
						begin += remains > 0 ? --remains, step + 1 : step;
						tasks.emplace_back(enqueue([offset, begin, bound]()
						{
							std::for_each<iterator, decltype(bound)>(offset, begin, bound);
						}));
					}
				}
				else
				{
					init_callback((size_t)1);
					auto bound = std::bind(element_callback, (size_t)0, std::placeholders::_1);
					std::for_each<iterator, decltype(bound)>(begin, end, bound);
				}

				return tasks;
			}
		};

		class series
		{
		public:
			static void pack(core::schema* v, bool value);
			static void pack(core::schema* v, int32_t value);
			static void pack(core::schema* v, int64_t value);
			static void pack(core::schema* v, uint32_t value);
			static void pack(core::schema* v, uint64_t value);
			static void pack(core::schema* v, float value);
			static void pack(core::schema* v, double value);
			static void pack(core::schema* v, const std::string_view& value);
			static void pack(core::schema* v, const core::vector<bool>& value);
			static void pack(core::schema* v, const core::vector<int32_t>& value);
			static void pack(core::schema* v, const core::vector<int64_t>& value);
			static void pack(core::schema* v, const core::vector<uint32_t>& value);
			static void pack(core::schema* v, const core::vector<uint64_t>& value);
			static void pack(core::schema* v, const core::vector<float>& value);
			static void pack(core::schema* v, const core::vector<double>& value);
			static void pack(core::schema* v, const core::vector<core::string>& value);
			static void pack(core::schema* v, const core::unordered_map<size_t, size_t>& value);
			static bool unpack(core::schema* v, bool* o);
			static bool unpack(core::schema* v, int32_t* o);
			static bool unpack(core::schema* v, int64_t* o);
			static bool unpack(core::schema* v, uint32_t* o);
			static bool unpack(core::schema* v, uint64_t* o);
			static bool unpack_a(core::schema* v, size_t* o);
			static bool unpack(core::schema* v, float* o);
			static bool unpack(core::schema* v, double* o);
			static bool unpack(core::schema* v, core::string* o);
			static bool unpack(core::schema* v, core::vector<bool>* o);
			static bool unpack(core::schema* v, core::vector<int32_t>* o);
			static bool unpack(core::schema* v, core::vector<int64_t>* o);
			static bool unpack(core::schema* v, core::vector<uint32_t>* o);
			static bool unpack(core::schema* v, core::vector<uint64_t>* o);
			static bool unpack(core::schema* v, core::vector<float>* o);
			static bool unpack(core::schema* v, core::vector<double>* o);
			static bool unpack(core::schema* v, core::vector<core::string>* o);
			static bool unpack(core::schema* v, core::unordered_map<size_t, size_t>* o);
		};

		class asset_file final : public core::reference<asset_file>
		{
		private:
			char* buffer;
			size_t length;

		public:
			asset_file(char* src_buffer, size_t src_size) noexcept;
			~asset_file() noexcept;
			char* get_buffer();
			size_t size();
		};

		class processor : public core::reference<processor>
		{
			friend content_manager;

		protected:
			content_manager* content;

		public:
			processor(content_manager* new_content) noexcept;
			virtual ~processor() noexcept;
			virtual void free(asset_cache* asset);
			virtual expects_content<core::unique<void>> duplicate(asset_cache* asset, const core::variant_args& keys);
			virtual expects_content<core::unique<void>> deserialize(core::stream* stream, size_t offset, const core::variant_args& keys);
			virtual expects_content<void> serialize(core::stream* stream, void* instance, const core::variant_args& keys);
			content_manager* get_content() const;
		};

		class content_manager : public core::reference<content_manager>
		{
		private:
			core::unordered_map<core::string, core::unordered_map<processor*, asset_cache*>> assets;
			core::unordered_map<core::string, asset_archive*> archives;
			core::unordered_map<uint64_t, processor*> processors;
			core::unordered_map<core::stream*, size_t> streams;
			core::string environment, base;
			std::mutex exclusive;
			size_t queue;

		public:
			content_manager() noexcept;
			virtual ~content_manager() noexcept;
			void clear_cache();
			void clear_archives();
			void clear_streams();
			void clear_processors();
			void clear_path(const std::string_view& path);
			void set_environment(const std::string_view& path);
			expects_content<void> import_archive(const std::string_view& path, bool validate_checksum);
			expects_content<void> export_archive(const std::string_view& path, const std::string_view& physical_directory, const std::string_view& virtual_directory = std::string_view());
			expects_content<core::unique<void>> load(processor* processor, const std::string_view& path, const core::variant_args& keys);
			expects_content<void> save(processor* processor, const std::string_view& path, void* object, const core::variant_args& keys);
			expects_promise_content<core::unique<void>> load_deferred(processor* processor, const std::string_view& path, const core::variant_args& keys);
			expects_promise_content<void> save_deferred(processor* processor, const std::string_view& path, void* object, const core::variant_args& keys);
			processor* add_processor(processor* value, uint64_t id);
			processor* get_processor(uint64_t id);
			asset_cache* find_cache(processor* target, const std::string_view& path);
			asset_cache* find_cache(processor* target, void* resource);
			const core::unordered_map<uint64_t, processor*>& get_processors() const;
			bool remove_processor(uint64_t id);
			void* try_to_cache(processor* root, const std::string_view& path, void* resource);
			bool is_busy();
			const core::string& get_environment() const;

		private:
			expects_content<void*> load_from_archive(processor* processor, const std::string_view& path, const core::variant_args& keys);
			void enqueue();
			void dequeue();

		public:
			template <typename t>
			expects_content<core::unique<t>> load(const std::string_view& path, const core::variant_args& keys = core::variant_args())
			{
				auto result = load(get_processor<t>(), path, keys);
				if (!result)
					return expects_content<t*>(result.error());

				return expects_content<t*>((t*)*result);
			}
			template <typename t>
			expects_promise_content<core::unique<t>> load_deferred(const std::string_view& path, const core::variant_args& keys = core::variant_args())
			{
				enqueue();
				core::string target_path = core::string(path);
				return core::cotask<expects_content<t*>>([this, target_path, keys]()
				{
					auto result = load(get_processor<t>(), target_path, keys);
					dequeue();
					if (!result)
						return expects_content<t*>(result.error());

					return expects_content<t*>((t*)*result);
				});
			}
			template <typename t>
			expects_content<void> save(const std::string_view& path, t* object, const core::variant_args& keys = core::variant_args())
			{
				return save(get_processor<t>(), path, object, keys);
			}
			template <typename t>
			expects_promise_content<void> save_deferred(const std::string_view& path, t* object, const core::variant_args& keys = core::variant_args())
			{
				return save_deferred(get_processor<t>(), path, (void*)object, keys);
			}
			template <typename t>
			bool remove_processor()
			{
				return remove_processor(typeid(t).hash_code());
			}
			template <typename v, typename t>
			v* add_processor()
			{
				return (v*)add_processor(new v(this), typeid(t).hash_code());
			}
			template <typename t>
			processor* get_processor()
			{
				return get_processor(typeid(t).hash_code());
			}
			template <typename t>
			asset_cache* find_cache(const std::string_view& path)
			{
				return find_cache(get_processor<t>(), path);
			}
			template <typename t>
			asset_cache* find_cache(void* resource)
			{
				return find_cache(get_processor<t>(), resource);
			}
		};

		class app_data final : public core::reference<app_data>
		{
		private:
			content_manager* content;
			core::schema* data;
			core::string path;
			std::mutex exclusive;

		public:
			app_data(content_manager* manager, const std::string_view& path) noexcept;
			~app_data() noexcept;
			void migrate(const std::string_view& path);
			void set_key(const std::string_view& name, core::unique<core::schema> value);
			void set_text(const std::string_view& name, const std::string_view& value);
			core::unique<core::schema> get_key(const std::string_view& name);
			core::string get_text(const std::string_view& name);
			bool has(const std::string_view& name);
			core::schema* get_snapshot() const;

		private:
			bool read_app_data(const std::string_view& path);
			bool write_app_data(const std::string_view& path);
		};

		class application : public core::singleton<application>
		{
		public:
			struct desc
			{
				struct frames_info
				{
					float stable = 120.0f;
					float limit = 0.0f;
				} refreshrate;

				core::schedule::desc scheduler;
				core::string preferences;
				core::string environment;
				core::string directory;
				size_t polling_timeout = 100;
				size_t polling_events = 256;
				size_t threads = 0;
				size_t usage =
					(size_t)USE_PROCESSING |
					(size_t)USE_NETWORKING |
					(size_t)USE_SCRIPTING;
				bool daemon = false;
			};

		private:
			core::timer* clock = nullptr;
			application_state state = application_state::terminated;
			int exit_code = 0;

		public:
			scripting::virtual_machine* vm = nullptr;
			content_manager* content = nullptr;
			app_data* database = nullptr;
			desc control;

		public:
			application(desc* i) noexcept;
			virtual ~application() noexcept;
			virtual void dispatch(core::timer* time);
			virtual void publish(core::timer* time);
			virtual void composition();
			virtual void script_hook();
			virtual void initialize();
			virtual core::promise<void> startup();
			virtual core::promise<void> shutdown();
			application_state get_state() const;
			int start();
			void restart();
			void stop(int exit_code = 0);

		private:
			void loop_trigger();

		private:
			static bool status(application* app);

		public:
			template <typename t, typename ...a>
			static int start_app(a... args)
			{
				core::uptr<t> app = new t(args...);
				int exit_code = app->start();
				VI_ASSERT(exit_code != EXIT_RESTART, "application cannot be restarted");
				return exit_code;
			}
			template <typename t, typename ...a>
			static int start_app_with_restart(a... args)
			{
			restart_app:
				core::uptr<t> app = new t(args...);
				int exit_code = app->start();
				if (exit_code == EXIT_RESTART)
					goto restart_app;

				return exit_code;
			}
		};
	}
}
#endif
