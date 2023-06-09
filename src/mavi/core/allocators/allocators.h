#include "../core.h"

namespace Mavi
{
	namespace Core
	{
		namespace Allocators
		{
			class VI_OUT_TS DebugAllocator final : public GlobalAllocator
			{
			public:
				struct VI_OUT_TS TracingBlock
				{
					std::thread::id Thread;
					std::string TypeName;
					MemoryContext Origin;
					time_t Time;
					size_t Size;
					bool Active;
					bool Static;

					TracingBlock();
					TracingBlock(const char* NewTypeName, MemoryContext&& NewOrigin, time_t NewTime, size_t NewSize, bool IsActive, bool IsStatic);
				};

			private:
				std::unordered_map<void*, TracingBlock> Blocks;
				std::recursive_mutex Mutex;

			public:
				~DebugAllocator() override = default;
				Unique<void> Allocate(size_t Size) noexcept override;
				Unique<void> Allocate(MemoryContext&& Origin, size_t Size) noexcept override;
				void Free(Unique<void> Address) noexcept override;
				void Transfer(Unique<void> Address, size_t Size) noexcept override;
				void Transfer(Unique<void> Address, MemoryContext&& Origin, size_t Size) noexcept override;
				void Watch(MemoryContext&& Origin, void* Address) noexcept override;
				void Unwatch(void* Address) noexcept override;
				void Finalize() noexcept override;
				bool IsValid(void* Address) noexcept override;
				bool IsFinalizable() noexcept override;
				bool Dump(void* Address);
				bool FindBlock(void* Address, TracingBlock* Output);
				const std::unordered_map<void*, TracingBlock>& GetBlocks() const;
			};

			class VI_OUT_TS DefaultAllocator final : public GlobalAllocator
			{
			public:
				~DefaultAllocator() override = default;
				Unique<void> Allocate(size_t Size) noexcept override;
				Unique<void> Allocate(MemoryContext&& Origin, size_t Size) noexcept override;
				void Free(Unique<void> Address) noexcept override;
				void Transfer(Unique<void> Address, size_t Size) noexcept override;
				void Transfer(Unique<void> Address, MemoryContext&& Origin, size_t Size) noexcept override;
				void Watch(MemoryContext&& Origin, void* Address) noexcept override;
				void Unwatch(void* Address) noexcept override;
				void Finalize() noexcept override;
				bool IsValid(void* Address) noexcept override;
				bool IsFinalizable() noexcept override;
			};

			class VI_OUT_TS CachedAllocator final : public GlobalAllocator
			{
			public:
				struct PageCache;

				using PageGroup = std::vector<PageCache*>;

				struct PageAddress
				{
					PageCache* Cache;
					void* Address;
				};

				struct PageCache
				{
					std::vector<PageAddress*> Addresses;
					PageGroup& Page;
					int64_t Timing;
					size_t Capacity;

					inline PageCache(PageGroup& NewPage, int64_t Time, size_t Size) : Page(NewPage), Timing(Time), Capacity(Size)
					{
						Addresses.resize(Capacity);
					}
					~PageCache() = default;
				};

			private:
				std::unordered_map<size_t, PageGroup> Pages;
				std::recursive_mutex Mutex;
				uint64_t MinimalLifeTime;
				double ElementsReducingFactor;
				size_t ElementsReducingBase;
				size_t ElementsPerAllocation;

			public:
				CachedAllocator(uint64_t MinimalLifeTimeMs = 2000, size_t MaxElementsPerAllocation = 1024, size_t ElementsReducingBaseBytes = 300, double ElementsReducingFactorRate = 5.0);
				~CachedAllocator() noexcept override;
				Unique<void> Allocate(size_t Size) noexcept override;
				Unique<void> Allocate(MemoryContext&& Origin, size_t Size) noexcept override;
				void Free(Unique<void> Address) noexcept override;
				void Transfer(Unique<void> Address, size_t Size) noexcept override;
				void Transfer(Unique<void> Address, MemoryContext&& Origin, size_t Size) noexcept override;
				void Watch(MemoryContext&& Origin, void* Address) noexcept override;
				void Unwatch(void* Address) noexcept override;
				void Finalize() noexcept override;
				bool IsValid(void* Address) noexcept override;
				bool IsFinalizable() noexcept override;

			private:
				PageCache* GetPageCache(size_t Size);
				int64_t GetClock();
				size_t GetElementsCount(PageGroup& Page, size_t Size);
			};

			class VI_OUT ArenaAllocator final : public LocalAllocator
			{
			private:
				struct Region
				{
					char* FreeAddress;
					char* BaseAddress;
					Region* UpperAddress;
					Region* LowerAddress;
					size_t Size;
				};

			private:
				Region* Top;
				Region* Bottom;
				size_t Sizing;

			public:
				ArenaAllocator(size_t Size);
				~ArenaAllocator() noexcept override;
				Unique<void> Allocate(size_t Size) noexcept override;
				void Free(Unique<void> Address) noexcept override;
				void Reset() noexcept override;
				bool IsValid(void* Address) noexcept override;

			private:
				void NextRegion(size_t Size) noexcept;
				void FlushRegions() noexcept;
			};
		}
	}
}