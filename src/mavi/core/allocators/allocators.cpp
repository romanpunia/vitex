#include "allocators.h"

namespace Mavi
{
	namespace Core
	{
		namespace Allocators
		{
			DebugAllocator::TracingBlock::TracingBlock() : Thread(std::this_thread::get_id()), Time(0), Size(0), Active(false)
			{
			}
			DebugAllocator::TracingBlock::TracingBlock(const char* NewTypeName, MemoryContext&& NewOrigin, time_t NewTime, size_t NewSize, bool IsActive, bool IsStatic) : Thread(std::this_thread::get_id()), TypeName(NewTypeName ? NewTypeName : "void"), Origin(std::move(NewOrigin)), Time(NewTime), Size(NewSize), Active(IsActive), Static(IsStatic)
			{
			}

			void* DebugAllocator::Allocate(size_t Size) noexcept
			{
				return Allocate(MemoryContext("[unknown]", "[external]", "void", 0), Size);
			}
			void* DebugAllocator::Allocate(MemoryContext&& Origin, size_t Size) noexcept
			{
				void* Address = malloc(Size);
				VI_ASSERT(Address != nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)Size);

				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				Blocks[Address] = TracingBlock(Origin.TypeName, std::move(Origin), time(nullptr), Size, true, false);
				return Address;
			}
			void DebugAllocator::Free(void* Address) noexcept
			{
				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				auto It = Blocks.find(Address);
				VI_ASSERT(It != Blocks.end() && It->second.Active, "cannot free memory that was not allocated by this allocator at 0x%" PRIXPTR, Address);

				Blocks.erase(It);
				free(Address);
			}
			void DebugAllocator::Transfer(Unique<void> Address, size_t Size) noexcept
			{
				VI_ASSERT(false, "invalid allocator transfer call without memory context");
			}
			void DebugAllocator::Transfer(Unique<void> Address, MemoryContext&& Origin, size_t Size) noexcept
			{
				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				Blocks[Address] = TracingBlock(Origin.TypeName, std::move(Origin), time(nullptr), Size, true, true);
			}
			void DebugAllocator::Watch(MemoryContext&& Origin, void* Address) noexcept
			{
				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				auto It = Blocks.find(Address);

				VI_ASSERT(It == Blocks.end() || !It->second.Active, "cannot watch memory that is already being tracked at 0x%" PRIXPTR, Address);
				Blocks[Address] = TracingBlock(Origin.TypeName, std::move(Origin), time(nullptr), sizeof(void*), false, false);
			}
			void DebugAllocator::Unwatch(void* Address) noexcept
			{
				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				auto It = Blocks.find(Address);

				VI_ASSERT(It != Blocks.end() && !It->second.Active, "address at 0x%" PRIXPTR " cannot be cleared from tracking because it was not allocated by this allocator", Address);
				Blocks.erase(It);
			}
			void DebugAllocator::Finalize() noexcept
			{
				Dump(nullptr);
			}
			bool DebugAllocator::IsValid(void* Address) noexcept
			{
				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				auto It = Blocks.find(Address);

				VI_ASSERT(It != Blocks.end(), "address at 0x%" PRIXPTR " cannot be used as it was already freed");
				return It != Blocks.end();
			}
			bool DebugAllocator::IsFinalizable() noexcept
			{
				return true;
			}
			bool DebugAllocator::Dump(void* Address)
			{
#if VI_DLEVEL >= 4
				VI_TRACE("[mem] dump internal memory state on 0x%" PRIXPTR, Address);
				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				if (Address != nullptr)
				{
					bool LogActive = ErrorHandling::HasFlag(LogOption::Active);
					if (!LogActive)
						ErrorHandling::SetFlag(LogOption::Active, true);

					auto It = Blocks.find(Address);
					if (It != Blocks.end())
					{
						char Date[64];
						DateTime::FetchDateTime(Date, sizeof(Date), It->second.Time);
						ErrorHandling::Message(LogLevel::Debug, It->second.Origin.Line, It->second.Origin.Source, "[mem] %saddress at 0x%" PRIXPTR " is active since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
							It->second.Static ? "static " : "",
							It->first, Date, It->second.TypeName.c_str(),
							(uint64_t)It->second.Size,
							It->second.Origin.Function,
							OS::Process::GetThreadId(It->second.Thread).c_str());
					}

					ErrorHandling::SetFlag(LogOption::Active, LogActive);
					return It != Blocks.end();
				}
				else if (!Blocks.empty())
				{
					size_t StaticAddresses = 0;
					for (auto& Item : Blocks)
					{
						if (Item.second.Static || Item.second.TypeName.find("ontainer_proxy") != std::string::npos || Item.second.TypeName.find("ist_node") != std::string::npos)
							++StaticAddresses;
					}

					if (StaticAddresses == Blocks.size())
						return false;

					bool LogActive = ErrorHandling::HasFlag(LogOption::Active);
					if (!LogActive)
						ErrorHandling::SetFlag(LogOption::Active, true);

					size_t TotalMemory = 0;
					for (auto& Item : Blocks)
						TotalMemory += Item.second.Size;

					VI_DEBUG("[mem] %" PRIu64 " addresses are still used (%" PRIu64 " bytes)", (uint64_t)(Blocks.size() - StaticAddresses), (uint64_t)TotalMemory);
					for (auto& Item : Blocks)
					{
						if (Item.second.Static || Item.second.TypeName.find("ontainer_proxy") != std::string::npos || Item.second.TypeName.find("ist_node") != std::string::npos)
							continue;

						char Date[64];
						DateTime::FetchDateTime(Date, sizeof(Date), Item.second.Time);
						ErrorHandling::Message(LogLevel::Debug, Item.second.Origin.Line, Item.second.Origin.Source, "[mem] address at 0x%" PRIXPTR " is active since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
							Item.first,
							Date,
							Item.second.TypeName.c_str(),
							(uint64_t)Item.second.Size,
							Item.second.Origin.Function,
							OS::Process::GetThreadId(Item.second.Thread).c_str());
					}

					ErrorHandling::SetFlag(LogOption::Active, LogActive);
					return true;
				}
#endif
				return false;
			}
			bool DebugAllocator::FindBlock(void* Address, TracingBlock* Output)
			{
				VI_ASSERT(Address != nullptr, "address should not be null");
				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				auto It = Blocks.find(Address);
				if (It == Blocks.end())
					return false;

				if (Output != nullptr)
					*Output = It->second;

				return true;
			}
			const std::unordered_map<void*, DebugAllocator::TracingBlock>& DebugAllocator::GetBlocks() const
			{
				return Blocks;
			}

			void* DefaultAllocator::Allocate(size_t Size) noexcept
			{
				void* Address = malloc(Size);
				VI_ASSERT(Address != nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)Size);
				return Address;
			}
			void* DefaultAllocator::Allocate(MemoryContext&& Origin, size_t Size) noexcept
			{
				void* Address = malloc(Size);
				VI_ASSERT(Address != nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)Size);
				return Address;
			}
			void DefaultAllocator::Free(void* Address) noexcept
			{
				free(Address);
			}
			void DefaultAllocator::Transfer(Unique<void> Address, size_t Size) noexcept
			{
			}
			void DefaultAllocator::Transfer(Unique<void> Address, MemoryContext&& Origin, size_t Size) noexcept
			{
			}
			void DefaultAllocator::Watch(MemoryContext&& Origin, void* Address) noexcept
			{
			}
			void DefaultAllocator::Unwatch(void* Address) noexcept
			{
			}
			void DefaultAllocator::Finalize() noexcept
			{
			}
			bool DefaultAllocator::IsValid(void* Address) noexcept
			{
				return true;
			}
			bool DefaultAllocator::IsFinalizable() noexcept
			{
				return true;
			}

			CachedAllocator::CachedAllocator(uint64_t MinimalLifeTimeMs, size_t MaxElementsPerAllocation, size_t ElementsReducingBaseBytes, double ElementsReducingFactorRate) : MinimalLifeTime(MinimalLifeTimeMs), ElementsReducingFactor(ElementsReducingFactorRate), ElementsReducingBase(ElementsReducingBaseBytes), ElementsPerAllocation(MaxElementsPerAllocation)
			{
				VI_ASSERT(ElementsPerAllocation > 0, "elements count per allocation should be greater then zero");
				VI_ASSERT(ElementsReducingFactor > 1.0, "elements reducing factor should be greater then zero");
				VI_ASSERT(ElementsReducingBase > 0, "elements reducing base should be greater then zero");
			}
			CachedAllocator::~CachedAllocator() noexcept
			{
				for (auto& Page : Pages)
				{
					for (auto* Cache : Page.second)
					{
						Cache->~PageCache();
						free(Cache);
					}
				}
				Pages.clear();
			}
			void* CachedAllocator::Allocate(size_t Size) noexcept
			{
				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				auto* Cache = GetPageCache(Size);
				if (!Cache)
					return nullptr;

				PageAddress* Address = Cache->Addresses.back();
				Cache->Addresses.pop_back();
				return Address->Address;
			}
			void* CachedAllocator::Allocate(MemoryContext&&, size_t Size) noexcept
			{
				return Allocate(Size);
			}
			void CachedAllocator::Free(void* Address) noexcept
			{
				char* SourceAddress = nullptr;
				PageAddress* Source = (PageAddress*)((char*)Address - sizeof(PageAddress));
				memcpy(&SourceAddress, (char*)Source + sizeof(void*), sizeof(void*));
				if (SourceAddress != Address)
					return free(Address);

				PageCache* Cache = nullptr;
				memcpy(&Cache, Source, sizeof(void*));

				std::unique_lock<std::recursive_mutex> Unique(Mutex);
				Cache->Addresses.push_back(Source);

				if (Cache->Addresses.size() >= Cache->Capacity && (Cache->Capacity == 1 || GetClock() - Cache->Timing > (int64_t)MinimalLifeTime))
				{
					Cache->Page.erase(std::find(Cache->Page.begin(), Cache->Page.end(), Cache));
					Cache->~PageCache();
					free(Cache);
				}
			}
			void CachedAllocator::Transfer(Unique<void> Address, size_t Size) noexcept
			{
			}
			void CachedAllocator::Transfer(Unique<void> Address, MemoryContext&& Origin, size_t Size) noexcept
			{
			}
			void CachedAllocator::Watch(MemoryContext&& Origin, void* Address) noexcept
			{
			}
			void CachedAllocator::Unwatch(void* Address) noexcept
			{
			}
			void CachedAllocator::Finalize() noexcept
			{
			}
			bool CachedAllocator::IsValid(void* Address) noexcept
			{
				return true;
			}
			bool CachedAllocator::IsFinalizable() noexcept
			{
				return false;
			}
			CachedAllocator::PageCache* CachedAllocator::GetPageCache(size_t Size)
			{
				auto& Page = Pages[Size];
				for (auto* Cache : Page)
				{
					if (!Cache->Addresses.empty())
						return Cache;
				}

				size_t AddressSize = sizeof(PageAddress) + Size;
				size_t PageElements = GetElementsCount(Page, Size);
				size_t PageSize = sizeof(PageCache) + AddressSize * PageElements;
				PageCache* Cache = (PageCache*)malloc(PageSize);
				VI_ASSERT(Cache != nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)PageSize);

				if (!Cache)
					return nullptr;

				char* BaseAddress = (char*)Cache + sizeof(PageCache);
				new(Cache) PageCache(Page, GetClock(), PageElements);
				for (size_t i = 0; i < PageElements; i++)
				{
					PageAddress* Next = (PageAddress*)(BaseAddress + AddressSize * i);
					Next->Address = (void*)(BaseAddress + AddressSize * i + sizeof(PageAddress));
					Next->Cache = Cache;
					Cache->Addresses[i] = Next;
				}

				Page.push_back(Cache);
				return Cache;
			}
			int64_t CachedAllocator::GetClock()
			{
				return (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			}
			size_t CachedAllocator::GetElementsCount(PageGroup& Page, size_t Size)
			{
				double Frequency;
				if (Pages.size() > 1)
				{
					Frequency = 0.0;
					for (auto& Next : Pages)
					{
						if (Next.second != Page)
							Frequency += (double)Next.second.size();
					}

					Frequency /= (double)Pages.size() - 1;
					if (Frequency < 1.0)
						Frequency = std::max(1.0, (double)Page.size());
					else
						Frequency = std::max(1.0, (double)Page.size() / Frequency);
				}
				else
					Frequency = 1.0;

				double Total = (double)ElementsPerAllocation;
				double Reducing = (double)Size / (double)ElementsReducingBase;
				if (Reducing > 1.0)
				{
					Total /= ElementsReducingFactor * Reducing;
					if (Total < 1.0)
						Total = 1.0;
				}
				else if (Frequency > 1.0)
					Total *= Frequency;

				return (size_t)Total;
			}

			ArenaAllocator::ArenaAllocator(size_t Size) : Top(nullptr), Bottom(nullptr), Sizing(Size)
			{
				NextRegion(Sizing);
			}
			ArenaAllocator::~ArenaAllocator() noexcept
			{
				FlushRegions();
			}
			void* ArenaAllocator::Allocate(size_t Size) noexcept
			{
			Retry:
				char* MaxAddress = Bottom->BaseAddress + Bottom->Size;
				char* OffsetAddress = Bottom->FreeAddress;
				size_t Leftovers = MaxAddress - OffsetAddress;
				if (Leftovers < Size)
				{
					NextRegion(Size);
					goto Retry;
				}

				char* Address = OffsetAddress;
				Bottom->FreeAddress = Address + Size;
				return Address;
			}
			void ArenaAllocator::Free(void* Address) noexcept
			{
				VI_ASSERT(IsValid(Address), "address is not valid");
			}
			void ArenaAllocator::Reset() noexcept
			{
				size_t TotalSize = 0;
				Region* Next = Top;
				while (Next != nullptr)
				{
					TotalSize += Next->Size;
					Next->FreeAddress = Next->BaseAddress;
					Next = Next->LowerAddress;
				}

				if (TotalSize > Sizing)
				{
					Sizing = TotalSize;
					FlushRegions();
					NextRegion(Sizing);
				}
			}
			bool ArenaAllocator::IsValid(void* Address) noexcept
			{
				char* Target = (char*)Address;
				Region* Next = Top;
				while (Next != nullptr)
				{
					if (Target >= Next->BaseAddress && Target <= Next->BaseAddress + Next->Size)
						return true;

					Next = Next->LowerAddress;
				}

				return false;
			}
			void ArenaAllocator::NextRegion(size_t Size) noexcept
			{
				LocalAllocator* Current = Memory::GetLocalAllocator();
				Memory::SetLocalAllocator(nullptr);

				Region* Next = VI_MALLOC(Region, sizeof(Region) + Size);
				Next->BaseAddress = (char*)Next + sizeof(Region);
				Next->FreeAddress = Next->BaseAddress;
				Next->UpperAddress = Bottom;
				Next->LowerAddress = nullptr;
				Next->Size = Size;

				if (!Top)
					Top = Next;
				if (Bottom != nullptr)
					Bottom->LowerAddress = Next;

				Bottom = Next;
				Memory::SetLocalAllocator(Current);
			}
			void ArenaAllocator::FlushRegions() noexcept
			{
				LocalAllocator* Current = Memory::GetLocalAllocator();
				Memory::SetLocalAllocator(nullptr);

				Region* Next = Bottom;
				Bottom = nullptr;
				Top = nullptr;

				while (Next != nullptr)
				{
					void* Address = (void*)Next;
					Next = Next->UpperAddress;
					VI_FREE(Address);
				}

				Memory::SetLocalAllocator(Current);
			}
		}
	}
}