#ifndef VI_LAYER_H
#define VI_LAYER_H
#include "scripting.h"
#include <atomic>
#include <cstdarg>

namespace Vitex
{
	namespace Layer
	{
		typedef std::function<void(class ContentManager*, bool)> SaveCallback;

		class Application;

		class ContentManager;

		class Processor;

		enum
		{
			USE_SCRIPTING = 1 << 0,
			USE_PROCESSING = 1 << 1,
			USE_NETWORKING = 1 << 2,
			EXIT_JUMP = 9600,
			EXIT_RESTART = EXIT_JUMP * 2
		};

		enum class ApplicationState
		{
			Terminated,
			Staging,
			Active,
			Restart
		};

		struct VI_OUT AssetCache
		{
			Core::String Path;
			void* Resource = nullptr;
		};

		struct VI_OUT AssetArchive
		{
			Core::Stream* Stream = nullptr;
			Core::String Path;
			size_t Length = 0;
			size_t Offset = 0;
		};

		class ContentException : public Core::SystemException
		{
		public:
			VI_OUT ContentException();
			VI_OUT ContentException(const std::string_view& Message);
			VI_OUT const char* type() const noexcept override;
		};

		template <typename V>
		using ExpectsContent = Core::Expects<V, ContentException>;

		template <typename T, typename Executor = Core::ParallelExecutor>
		using ExpectsPromiseContent = Core::BasicPromise<ExpectsContent<T>, Executor>;

		class VI_OUT_TS Parallel
		{
		public:
			static Core::Promise<void> Enqueue(Core::TaskCallback&& Callback);
			static Core::Vector<Core::Promise<void>> EnqueueAll(Core::Vector<Core::TaskCallback>&& Callbacks);
			static void Wait(Core::Promise<void>&& Value);
			static void WailAll(Core::Vector<Core::Promise<void>>&& Values);
			static size_t GetThreadIndex();
			static size_t GetThreads();

		public:
			template <typename T>
			static Core::Vector<T> InlineWaitAll(Core::Vector<Core::Promise<T>>&& Values)
			{
				Core::Vector<T> Results;
				Results.reserve(Values.size());
				for (auto& Value : Values)
					Results.emplace_back(std::move(Value.Get()));
				return Results;
			}
			template <typename Iterator, typename Function>
			static Core::Vector<Core::Promise<void>> ForEach(Iterator Begin, Iterator End, Function&& Callback)
			{
				Core::Vector<Core::Promise<void>> Tasks;
				size_t Size = End - Begin;

				if (!Size)
					return Tasks;

				size_t Threads = std::max<size_t>(1, GetThreads());
				size_t Step = Size / Threads;
				size_t Remains = Size % Threads;
				Tasks.reserve(Threads);

				while (Begin != End)
				{
					auto Offset = Begin;
					Begin += Remains > 0 ? --Remains, Step + 1 : Step;
					Tasks.emplace_back(Enqueue(std::bind(std::for_each<Iterator, Function>, Offset, Begin, Callback)));
				}

				return Tasks;
			}
			template <typename Iterator, typename InitFunction, typename ElementFunction>
			static Core::Vector<Core::Promise<void>> Distribute(Iterator Begin, Iterator End, InitFunction&& InitCallback, ElementFunction&& ElementCallback)
			{
				Core::Vector<Core::Promise<void>> Tasks;
				size_t Size = End - Begin;
				if (!Size)
				{
					InitCallback((size_t)0);
					return Tasks;
				}

				size_t Threads = std::max<size_t>(1, GetThreads());
				size_t Step = Size / Threads;
				size_t Remains = Size % Threads;
				size_t Remainder = Remains;
				size_t Index = 0, Counting = 0;
				auto Start = Begin;

				while (Start != End)
				{
					Start += Remainder > 0 ? --Remainder, Step + 1 : Step;
					++Counting;
				}

				Tasks.reserve(Counting);
				InitCallback(Counting);

				while (Begin != End)
				{
					auto Offset = Begin;
					auto Bound = std::bind(ElementCallback, Index++, std::placeholders::_1);
					Begin += Remains > 0 ? --Remains, Step + 1 : Step;
					Tasks.emplace_back(Enqueue([Offset, Begin, Bound]()
					{
						std::for_each<Iterator, decltype(Bound)>(Offset, Begin, Bound);
					}));
				}

				return Tasks;
			}
		};

		class VI_OUT_TS Series
		{
		public:
			static void Pack(Core::Schema* V, bool Value);
			static void Pack(Core::Schema* V, int32_t Value);
			static void Pack(Core::Schema* V, int64_t Value);
			static void Pack(Core::Schema* V, uint32_t Value);
			static void Pack(Core::Schema* V, uint64_t Value);
			static void Pack(Core::Schema* V, float Value);
			static void Pack(Core::Schema* V, double Value);
			static void Pack(Core::Schema* V, const std::string_view& Value);
			static void Pack(Core::Schema* V, const Core::Vector<bool>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<int32_t>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<int64_t>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<uint32_t>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<uint64_t>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<float>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<double>& Value);
			static void Pack(Core::Schema* V, const Core::Vector<Core::String>& Value);
			static void Pack(Core::Schema* V, const Core::UnorderedMap<size_t, size_t>& Value);
			static bool Unpack(Core::Schema* V, bool* O);
			static bool Unpack(Core::Schema* V, int32_t* O);
			static bool Unpack(Core::Schema* V, int64_t* O);
			static bool Unpack(Core::Schema* V, uint32_t* O);
			static bool Unpack(Core::Schema* V, uint64_t* O);
			static bool UnpackA(Core::Schema* V, size_t* O);
			static bool Unpack(Core::Schema* V, float* O);
			static bool Unpack(Core::Schema* V, double* O);
			static bool Unpack(Core::Schema* V, Core::String* O);
			static bool Unpack(Core::Schema* V, Core::Vector<bool>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<int32_t>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<int64_t>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<uint32_t>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<uint64_t>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<float>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<double>* O);
			static bool Unpack(Core::Schema* V, Core::Vector<Core::String>* O);
			static bool Unpack(Core::Schema* V, Core::UnorderedMap<size_t, size_t>* O);
		};

		class VI_OUT AssetFile final : public Core::Reference<AssetFile>
		{
		private:
			char* Buffer;
			size_t Length;

		public:
			AssetFile(char* SrcBuffer, size_t SrcSize) noexcept;
			~AssetFile() noexcept;
			char* GetBuffer();
			size_t Size();
		};

		class VI_OUT Processor : public Core::Reference<Processor>
		{
			friend ContentManager;

		protected:
			ContentManager* Content;

		public:
			Processor(ContentManager* NewContent) noexcept;
			virtual ~Processor() noexcept;
			virtual void Free(AssetCache* Asset);
			virtual ExpectsContent<Core::Unique<void>> Duplicate(AssetCache* Asset, const Core::VariantArgs& Keys);
			virtual ExpectsContent<Core::Unique<void>> Deserialize(Core::Stream* Stream, size_t Offset, const Core::VariantArgs& Keys);
			virtual ExpectsContent<void> Serialize(Core::Stream* Stream, void* Instance, const Core::VariantArgs& Keys);
			ContentManager* GetContent() const;
		};

		class VI_OUT_TS ContentManager : public Core::Reference<ContentManager>
		{
		private:
			Core::UnorderedMap<Core::String, Core::UnorderedMap<Processor*, AssetCache*>> Assets;
			Core::UnorderedMap<Core::String, AssetArchive*> Archives;
			Core::UnorderedMap<uint64_t, Processor*> Processors;
			Core::UnorderedMap<Core::Stream*, size_t> Streams;
			Core::String Environment, Base;
			std::mutex Exclusive;
			size_t Queue;

		public:
			ContentManager() noexcept;
			virtual ~ContentManager() noexcept;
			void ClearCache();
			void ClearArchives();
			void ClearStreams();
			void ClearProcessors();
			void ClearPath(const std::string_view& Path);
			void SetEnvironment(const std::string_view& Path);
			ExpectsContent<void> ImportArchive(const std::string_view& Path, bool ValidateChecksum);
			ExpectsContent<void> ExportArchive(const std::string_view& Path, const std::string_view& PhysicalDirectory, const std::string_view& VirtualDirectory = std::string_view());
			ExpectsContent<Core::Unique<void>> Load(Processor* Processor, const std::string_view& Path, const Core::VariantArgs& Keys);
			ExpectsContent<void> Save(Processor* Processor, const std::string_view& Path, void* Object, const Core::VariantArgs& Keys);
			ExpectsPromiseContent<Core::Unique<void>> LoadDeferred(Processor* Processor, const std::string_view& Path, const Core::VariantArgs& Keys);
			ExpectsPromiseContent<void> SaveDeferred(Processor* Processor, const std::string_view& Path, void* Object, const Core::VariantArgs& Keys);
			Processor* AddProcessor(Processor* Value, uint64_t Id);
			Processor* GetProcessor(uint64_t Id);
			AssetCache* FindCache(Processor* Target, const std::string_view& Path);
			AssetCache* FindCache(Processor* Target, void* Resource);
			const Core::UnorderedMap<uint64_t, Processor*>& GetProcessors() const;
			bool RemoveProcessor(uint64_t Id);
			void* TryToCache(Processor* Root, const std::string_view& Path, void* Resource);
			bool IsBusy();
			const Core::String& GetEnvironment() const;

		private:
			ExpectsContent<void*> LoadFromArchive(Processor* Processor, const std::string_view& Path, const Core::VariantArgs& Keys);
			void Enqueue();
			void Dequeue();

		public:
			template <typename T>
			ExpectsContent<Core::Unique<T>> Load(const std::string_view& Path, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				auto Result = Load(GetProcessor<T>(), Path, Keys);
				if (!Result)
					return ExpectsContent<T*>(Result.Error());

				return ExpectsContent<T*>((T*)*Result);
			}
			template <typename T>
			ExpectsPromiseContent<Core::Unique<T>> LoadDeferred(const std::string_view& Path, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				Enqueue();
				Core::String TargetPath = Core::String(Path);
				return Core::Cotask<ExpectsContent<T*>>([this, TargetPath, Keys]()
				{
					auto Result = Load(GetProcessor<T>(), TargetPath, Keys);
					Dequeue();
					if (!Result)
						return ExpectsContent<T*>(Result.Error());

					return ExpectsContent<T*>((T*)*Result);
				});
			}
			template <typename T>
			ExpectsContent<void> Save(const std::string_view& Path, T* Object, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				return Save(GetProcessor<T>(), Path, Object, Keys);
			}
			template <typename T>
			ExpectsPromiseContent<void> SaveDeferred(const std::string_view& Path, T* Object, const Core::VariantArgs& Keys = Core::VariantArgs())
			{
				return SaveDeferred(GetProcessor<T>(), Path, (void*)Object, Keys);
			}
			template <typename T>
			bool RemoveProcessor()
			{
				return RemoveProcessor(typeid(T).hash_code());
			}
			template <typename V, typename T>
			V* AddProcessor()
			{
				return (V*)AddProcessor(new V(this), typeid(T).hash_code());
			}
			template <typename T>
			Processor* GetProcessor()
			{
				return GetProcessor(typeid(T).hash_code());
			}
			template <typename T>
			AssetCache* FindCache(const std::string_view& Path)
			{
				return FindCache(GetProcessor<T>(), Path);
			}
			template <typename T>
			AssetCache* FindCache(void* Resource)
			{
				return FindCache(GetProcessor<T>(), Resource);
			}
		};

		class VI_OUT_TS AppData final : public Core::Reference<AppData>
		{
		private:
			ContentManager* Content;
			Core::Schema* Data;
			Core::String Path;
			std::mutex Exclusive;

		public:
			AppData(ContentManager* Manager, const std::string_view& Path) noexcept;
			~AppData() noexcept;
			void Migrate(const std::string_view& Path);
			void SetKey(const std::string_view& Name, Core::Unique<Core::Schema> Value);
			void SetText(const std::string_view& Name, const std::string_view& Value);
			Core::Unique<Core::Schema> GetKey(const std::string_view& Name);
			Core::String GetText(const std::string_view& Name);
			bool Has(const std::string_view& Name);
			Core::Schema* GetSnapshot() const;

		private:
			bool ReadAppData(const std::string_view& Path);
			bool WriteAppData(const std::string_view& Path);
		};

		class VI_OUT Application : public Core::Singleton<Application>
		{
		public:
			struct Desc
			{
				struct FramesInfo
				{
					float Stable = 120.0f;
					float Limit = 0.0f;
				} Refreshrate;

				Core::Schedule::Desc Scheduler;
				Core::String Preferences;
				Core::String Environment;
				Core::String Directory;
				size_t PollingTimeout = 100;
				size_t PollingEvents = 256;
				size_t Threads = 0;
				size_t Usage =
					(size_t)USE_PROCESSING |
					(size_t)USE_NETWORKING |
					(size_t)USE_SCRIPTING;
				bool Daemon = false;
			};

		private:
			Core::Timer* Clock = nullptr;
			ApplicationState State = ApplicationState::Terminated;
			int ExitCode = 0;

		public:
			Scripting::VirtualMachine* VM = nullptr;
			ContentManager* Content = nullptr;
			AppData* Database = nullptr;
			Desc Control;

		public:
			Application(Desc* I) noexcept;
			virtual ~Application() noexcept;
			virtual void Dispatch(Core::Timer* Time);
			virtual void Publish(Core::Timer* Time);
			virtual void Composition();
			virtual void ScriptHook();
			virtual void Initialize();
			virtual Core::Promise<void> Startup();
			virtual Core::Promise<void> Shutdown();
			ApplicationState GetState() const;
			int Start();
			void Restart();
			void Stop(int ExitCode = 0);

		private:
			void LoopTrigger();

		private:
			static bool Status(Application* App);

		public:
			template <typename T, typename ...A>
			static int StartApp(A... Args)
			{
				Core::UPtr<T> App = new T(Args...);
				int ExitCode = App->Start();
				VI_ASSERT(ExitCode != EXIT_RESTART, "application cannot be restarted");
				return ExitCode;
			}
			template <typename T, typename ...A>
			static int StartAppWithRestart(A... Args)
			{
			RestartApp:
				Core::UPtr<T> App = new T(Args...);
				int ExitCode = App->Start();
				if (ExitCode == EXIT_RESTART)
					goto RestartApp;

				return ExitCode;
			}
		};
	}
}
#endif
