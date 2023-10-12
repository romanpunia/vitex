#include "core.h"
#include "scripting.h"
#include "../network/http.h"
#include <cctype>
#include <ctime>
#include <thread>
#include <functional>
#include <iostream>
#include <fstream>
#include <bitset>
#include <signal.h>
#include <sys/stat.h>
#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4267)
#pragma warning(disable: 4554)
#include <blockingconcurrentqueue.h>
#ifdef VI_PUGIXML
#include <pugixml.hpp>
#endif
#ifdef VI_RAPIDJSON
#include <rapidjson/document.h>
#endif
#ifdef VI_TINYFILEDIALOGS
#include <tinyfiledialogs.h>
#endif
#ifdef VI_CXX23
#include <stacktrace>
#elif defined(VI_BACKTRACE)
#include <backward.hpp>
#endif
#ifdef VI_FCTX
#include <fcontext/fcontext.h>
#endif
#ifdef VI_MICROSOFT
#include <Windows.h>
#include <io.h>
#else
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#ifdef VI_SOLARIS
#include <stdlib.h>
#endif
#ifdef VI_APPLE
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <mach-o/dyld.h>
#endif
#include <sys/utsname.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <dlfcn.h>
#ifndef VI_FCTX
#include <ucontext.h>
#endif
#endif
#ifdef VI_SDL2
#include <SDL2/SDL.h>
#endif
#ifdef VI_ZLIB
extern "C"
{
#include <zlib.h>
}
#endif
#define PREFIX_ENUM "$"
#define PREFIX_BINARY "`"
#define JSONB_VERSION 0xef1033dd
#define MAKEUQUAD(L, H) ((uint64_t)(((uint32_t)(L)) | ((uint64_t)((uint32_t)(H))) << 32))
#define RATE_DIFF (10000000)
#define EPOCH_DIFF (MAKEUQUAD(0xd53e8000, 0x019db1de))
#define SYS2UNIX_TIME(L, H) ((int64_t)((MAKEUQUAD((L), (H)) - EPOCH_DIFF) / RATE_DIFF))
#define LEAP_YEAR(X) (((X) % 4 == 0) && (((X) % 100) != 0 || ((X) % 400) == 0))
#ifdef VI_MICROSOFT
namespace
{
	BOOL WINAPI ConsoleEventHandler(DWORD Event)
	{
		switch (Event)
		{
			case CTRL_C_EVENT:
			case CTRL_BREAK_EVENT:
				raise(SIGINT);
				break;
			case CTRL_CLOSE_EVENT:
			case CTRL_SHUTDOWN_EVENT:
			case CTRL_LOGOFF_EVENT:
				raise(SIGTERM);
				break;
			default:
				break;
		}

		return TRUE;
	}
	bool LocalTime(time_t const* const A, struct tm* const B)
	{
		return localtime_s(B, A) == 0;
	}
}
#else
namespace
{
	void Pack2_64(void* Value, int* X, int* Y)
	{
		uint64_t Subvalue = (uint64_t)Value;
		*X = (int)(uint32_t)((Subvalue & 0xFFFFFFFF00000000LL) >> 32);
		*Y = (int)(uint32_t)(Subvalue & 0xFFFFFFFFLL);
	}
	void* Unpack2_64(int X, int Y)
	{
		uint64_t Subvalue = ((uint64_t)(uint32_t)X) << 32 | (uint32_t)Y;
		return (void*)Subvalue;
	}
	bool LocalTime(time_t const* const A, struct tm* const B)
	{
		return localtime_r(A, B) != nullptr;
	}
	const char* GetColorId(Mavi::Core::StdColor Color, bool Background)
	{
		switch (Color)
		{
			case Mavi::Core::StdColor::Black:
				return Background ? "40" : "30";
			case Mavi::Core::StdColor::DarkBlue:
				return Background ? "44" : "34";
			case Mavi::Core::StdColor::DarkGreen:
				return Background ? "42" : "32";
			case Mavi::Core::StdColor::DarkRed:
				return Background ? "41" : "31";
			case Mavi::Core::StdColor::Magenta:
				return Background ? "45" : "35";
			case Mavi::Core::StdColor::Orange:
				return Background ? "43" : "93";
			case Mavi::Core::StdColor::LightGray:
				return Background ? "47" : "97";
			case Mavi::Core::StdColor::LightBlue:
				return Background ? "46" : "94";
			case Mavi::Core::StdColor::Gray:
				return Background ? "100" : "90";
			case Mavi::Core::StdColor::Blue:
				return Background ? "104" : "94";
			case Mavi::Core::StdColor::Green:
				return Background ? "102" : "92";
			case Mavi::Core::StdColor::Cyan:
				return Background ? "106" : "36";
			case Mavi::Core::StdColor::Red:
				return Background ? "101" : "91";
			case Mavi::Core::StdColor::Pink:
				return Background ? "105" : "95";
			case Mavi::Core::StdColor::Yellow:
				return Background ? "103" : "33";
			case Mavi::Core::StdColor::White:
				return Background ? "107" : "37";
			case Mavi::Core::StdColor::Zero:
				return Background ? "49" : "39";
			default:
				return Background ? "40" : "107";
		}
	}
}
#endif
namespace
{
	bool IsPathExists(const char* Path)
	{
		struct stat Buffer;
		return stat(Path, &Buffer) == 0;
	}
	void EscapeText(char* Text, size_t Size)
	{
		size_t Index = 0;
		while (Size-- > 0)
		{
			char& V = Text[Index++];
			if (V == '\a' || V == '\b' || V == '\f' || V == '\v' || V == '\0')
				V = '\?';
		}
	}
#ifdef VI_APPLE
#define SYSCTL(fname, ...) std::size_t Size{};if(fname(__VA_ARGS__,nullptr,&Size,nullptr,0))return{};Mavi::Core::Vector<char> Result(Size);if(fname(__VA_ARGS__,Result.data(),&Size,nullptr,0))return{};return Result
	template <class T>
	static std::pair<bool, T> SysDecompose(const Mavi::Core::Vector<char>& Data)
	{
		std::pair<bool, T> Out { true, {} };
		std::memcpy(&Out.second, Data.data(), sizeof(Out.second));
		return Out;
	}
	Mavi::Core::Vector<char> SysControl(const char* Name)
	{
		SYSCTL(::sysctlbyname, Name);
	}
	Mavi::Core::Vector<char> SysControl(int M1, int M2)
	{
		int Name[2]{ M1, M2 };
		SYSCTL(::sysctl, Name, sizeof(Name) / sizeof(*Name));
	}
	std::pair<bool, uint64_t> SysExtract(const Mavi::Core::Vector<char>& Data)
	{
		switch (Data.size())
		{
			case sizeof(uint16_t) :
				return SysDecompose<uint16_t>(Data);
				case sizeof(uint32_t) :
					return SysDecompose<uint32_t>(Data);
					case sizeof(uint64_t) :
						return SysDecompose<uint64_t>(Data);
					default:
						return {};
		}
	}
#endif
#ifdef VI_MICROSOFT
	static Mavi::Core::Vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> CPUInfoBuffer()
	{
		DWORD ByteCount = 0;
		Mavi::Core::Vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> Buffer;
		GetLogicalProcessorInformation(nullptr, &ByteCount);
		Buffer.resize(ByteCount / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
		GetLogicalProcessorInformation(Buffer.data(), &ByteCount);

		return Buffer;
	}
#endif
}

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

				UMutex<std::recursive_mutex> Unique(Mutex);
				Blocks[Address] = TracingBlock(Origin.TypeName, std::move(Origin), time(nullptr), Size, true, false);
				return Address;
			}
			void DebugAllocator::Free(void* Address) noexcept
			{
				UMutex<std::recursive_mutex> Unique(Mutex);
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
				UMutex<std::recursive_mutex> Unique(Mutex);
				Blocks[Address] = TracingBlock(Origin.TypeName, std::move(Origin), time(nullptr), Size, true, true);
			}
			void DebugAllocator::Watch(MemoryContext&& Origin, void* Address) noexcept
			{
				UMutex<std::recursive_mutex> Unique(Mutex);
				auto It = Watchers.find(Address);

				VI_ASSERT(It == Watchers.end() || !It->second.Active, "cannot watch memory that is already being tracked at 0x%" PRIXPTR, Address);
				Watchers[Address] = TracingBlock(Origin.TypeName, std::move(Origin), time(nullptr), sizeof(void*), false, false);
			}
			void DebugAllocator::Unwatch(void* Address) noexcept
			{
				UMutex<std::recursive_mutex> Unique(Mutex);
				auto It = Watchers.find(Address);

				VI_ASSERT(It != Watchers.end() && !It->second.Active, "address at 0x%" PRIXPTR " cannot be cleared from tracking because it was not allocated by this allocator", Address);
				Watchers.erase(It);
			}
			void DebugAllocator::Finalize() noexcept
			{
				Dump(nullptr);
			}
			bool DebugAllocator::IsValid(void* Address) noexcept
			{
				UMutex<std::recursive_mutex> Unique(Mutex);
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
				UMutex<std::recursive_mutex> Unique(Mutex);
				if (Address != nullptr)
				{
					bool LogActive = ErrorHandling::HasFlag(LogOption::Active), Exists = false;
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
						Exists = true;
					}

					It = Watchers.find(Address);
					if (It != Watchers.end())
					{
						char Date[64];
						DateTime::FetchDateTime(Date, sizeof(Date), It->second.Time);
						ErrorHandling::Message(LogLevel::Debug, It->second.Origin.Line, It->second.Origin.Source, "[mem-watch] %saddress at 0x%" PRIXPTR " is being watched since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
							It->second.Static ? "static " : "",
							It->first, Date, It->second.TypeName.c_str(),
							(uint64_t)It->second.Size,
							It->second.Origin.Function,
							OS::Process::GetThreadId(It->second.Thread).c_str());
						Exists = true;
					}

					ErrorHandling::SetFlag(LogOption::Active, LogActive);
					return Exists;
				}
				else if (!Blocks.empty() || !Watchers.empty())
				{
					size_t StaticAddresses = 0;
					for (auto& Item : Blocks)
					{
						if (Item.second.Static || Item.second.TypeName.find("ontainer_proxy") != std::string::npos || Item.second.TypeName.find("ist_node") != std::string::npos)
							++StaticAddresses;
					}
					for (auto& Item : Watchers)
					{
						if (Item.second.Static || Item.second.TypeName.find("ontainer_proxy") != std::string::npos || Item.second.TypeName.find("ist_node") != std::string::npos)
							++StaticAddresses;
					}

					if (StaticAddresses == Blocks.size() + Watchers.size())
						return false;

					bool LogActive = ErrorHandling::HasFlag(LogOption::Active);
					if (!LogActive)
						ErrorHandling::SetFlag(LogOption::Active, true);

					size_t TotalMemory = 0;
					for (auto& Item : Blocks)
						TotalMemory += Item.second.Size;
					for (auto& Item : Watchers)
						TotalMemory += Item.second.Size;

					VI_DEBUG("[mem] %" PRIu64 " addresses are still used (%" PRIu64 " bytes)", (uint64_t)(Blocks.size() + Watchers.size() - StaticAddresses), (uint64_t)TotalMemory);
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
					for (auto& Item : Watchers)
					{
						if (Item.second.Static || Item.second.TypeName.find("ontainer_proxy") != std::string::npos || Item.second.TypeName.find("ist_node") != std::string::npos)
							continue;

						char Date[64];
						DateTime::FetchDateTime(Date, sizeof(Date), Item.second.Time);
						ErrorHandling::Message(LogLevel::Debug, Item.second.Origin.Line, Item.second.Origin.Source, "[mem-watch] address at 0x%" PRIXPTR " is being watched since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
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
				UMutex<std::recursive_mutex> Unique(Mutex);
				auto It = Blocks.find(Address);
				if (It == Blocks.end())
				{
					It = Watchers.find(Address);
					if (It == Watchers.end())
						return false;
				}

				if (Output != nullptr)
					*Output = It->second;

				return true;
			}
			const std::unordered_map<void*, DebugAllocator::TracingBlock>& DebugAllocator::GetBlocks() const
			{
				return Blocks;
			}
			const std::unordered_map<void*, DebugAllocator::TracingBlock>& DebugAllocator::GetWatchers() const
			{
				return Watchers;
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
				UMutex<std::recursive_mutex> Unique(Mutex);
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

				UMutex<std::recursive_mutex> Unique(Mutex);
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

			LinearAllocator::LinearAllocator(size_t Size) : Top(nullptr), Bottom(nullptr), LatestSize(0), Sizing(Size)
			{
				if (Sizing > 0)
					NextRegion(Sizing);
			}
			LinearAllocator::~LinearAllocator() noexcept
			{
				FlushRegions();
			}
			void* LinearAllocator::Allocate(size_t Size) noexcept
			{
				if (!Bottom)
					NextRegion(Size);
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
				LatestSize = Size;
				return Address;
			}
			void LinearAllocator::Free(void* Address) noexcept
			{
				VI_ASSERT(IsValid(Address), "address is not valid");
				char* OriginAddress = Bottom->FreeAddress - LatestSize;
				if (OriginAddress == Address)
				{
					Bottom->FreeAddress = OriginAddress;
					LatestSize = 0;
				}
			}
			void LinearAllocator::Reset() noexcept
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
			bool LinearAllocator::IsValid(void* Address) noexcept
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
			void LinearAllocator::NextRegion(size_t Size) noexcept
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
			void LinearAllocator::FlushRegions() noexcept
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
			size_t LinearAllocator::GetLeftovers() const noexcept
			{
				if (!Bottom)
					return 0;

				char* MaxAddress = Bottom->BaseAddress + Bottom->Size;
				char* OffsetAddress = Bottom->FreeAddress;
				return MaxAddress - OffsetAddress;
			}

			StackAllocator::StackAllocator(size_t Size) : Top(nullptr), Bottom(nullptr), Sizing(Size)
			{
				if (Sizing > 0)
					NextRegion(Sizing);
			}
			StackAllocator::~StackAllocator() noexcept
			{
				FlushRegions();
			}
			void* StackAllocator::Allocate(size_t Size) noexcept
			{
				Size = Size + sizeof(size_t);
				if (!Bottom)
					NextRegion(Size);
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
				*(size_t*)Address = Size;
				return Address + sizeof(size_t);
			}
			void StackAllocator::Free(void* Address) noexcept
			{
				VI_ASSERT(IsValid(Address), "address is not valid");
				char* OffsetAddress = (char*)Address - sizeof(size_t);
				char* OriginAddress = Bottom->FreeAddress - *(size_t*)OffsetAddress;
				if (OriginAddress == OffsetAddress)
					Bottom->FreeAddress = OriginAddress;
			}
			void StackAllocator::Reset() noexcept
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
			bool StackAllocator::IsValid(void* Address) noexcept
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
			void StackAllocator::NextRegion(size_t Size) noexcept
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
			void StackAllocator::FlushRegions() noexcept
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
			size_t StackAllocator::GetLeftovers() const noexcept
			{
				if (!Bottom)
					return 0;

				char* MaxAddress = Bottom->BaseAddress + Bottom->Size;
				char* OffsetAddress = Bottom->FreeAddress;
				return MaxAddress - OffsetAddress;
			}
		}

		namespace Exceptions
		{
			ParserException::ParserException(ParserError NewType) : ParserException(NewType, -1, nullptr)
			{
			}
			ParserException::ParserException(ParserError NewType, int NewOffset) : ParserException(NewType, NewOffset, nullptr)
			{
			}
			ParserException::ParserException(ParserError NewType, int NewOffset, const char* NewMessage) : std::exception(), Type(NewType), Offset(NewOffset)
			{
				if (!NewMessage || NewMessage[0] == '\0')
				{ 
					switch (Type)
					{
						case ParserError::NotSupported:
							Info = "required libraries are not loaded";
							break;
						case ParserError::BadVersion:
							Info = "corrupted JSONB version header";
							break;
						case ParserError::BadDictionary:
							Info = "corrupted JSONB dictionary body";
							break;
						case ParserError::BadNameIndex:
							Info = "invalid JSONB dictionary name index";
							break;
						case ParserError::BadName:
							Info = "invalid JSONB name";
							break;
						case ParserError::BadKeyName:
							Info = "invalid JSONB key name";
							break;
						case ParserError::BadKeyType:
							Info = "invalid JSONB key type";
							break;
						case ParserError::BadValue:
							Info = "invalid JSONB value for specified key";
							break;
						case ParserError::BadString:
							Info = "invalid JSONB value string";
							break;
						case ParserError::BadInteger:
							Info = "invalid JSONB value integer";
							break;
						case ParserError::BadDouble:
							Info = "invalid JSONB value double";
							break;
						case ParserError::BadBoolean:
							Info = "invalid JSONB value boolean";
							break;
						case ParserError::XMLOutOfMemory:
							Info = "XML out of memory";
							break;
						case ParserError::XMLInternalError:
							Info = "XML internal error";
							break;
						case ParserError::XMLUnrecognizedTag:
							Info = "XML unrecognized tag";
							break;
						case ParserError::XMLBadPi:
							Info = "XML bad pi";
							break;
						case ParserError::XMLBadComment:
							Info = "XML bad comment";
							break;
						case ParserError::XMLBadCData:
							Info = "XML bad cdata";
							break;
						case ParserError::XMLBadDocType:
							Info = "XML bad doctype";
							break;
						case ParserError::XMLBadPCData:
							Info = "XML bad pcdata";
							break;
						case ParserError::XMLBadStartElement:
							Info = "XML bad start element";
							break;
						case ParserError::XMLBadAttribute:
							Info = "XML bad attribute";
							break;
						case ParserError::XMLBadEndElement:
							Info = "XML bad end element";
							break;
						case ParserError::XMLEndElementMismatch:
							Info = "XML end element mismatch";
							break;
						case ParserError::XMLAppendInvalidRoot:
							Info = "XML append invalid root";
							break;
						case ParserError::XMLNoDocumentElement:
							Info = "XML no document element";
							break;
						case ParserError::JSONDocumentEmpty:
							Info = "the JSON document is empty";
							break;
						case ParserError::JSONDocumentRootNotSingular:
							Info = "the JSON document root must not follow by other values";
							break;
						case ParserError::JSONValueInvalid:
							Info = "the JSON document contains an invalid value";
							break;
						case ParserError::JSONObjectMissName:
							Info = "missing a name for a JSON object member";
							break;
						case ParserError::JSONObjectMissColon:
							Info = "missing a colon after a name of a JSON object member";
							break;
						case ParserError::JSONObjectMissCommaOrCurlyBracket:
							Info = "missing a comma or '}' after a JSON object member";
							break;
						case ParserError::JSONArrayMissCommaOrSquareBracket:
							Info = "missing a comma or ']' after a JSON array element";
							break;
						case ParserError::JSONStringUnicodeEscapeInvalidHex:
							Info = "incorrect hex digit after \\u escape in a JSON string";
							break;
						case ParserError::JSONStringUnicodeSurrogateInvalid:
							Info = "the surrogate pair in a JSON string is invalid";
							break;
						case ParserError::JSONStringEscapeInvalid:
							Info = "invalid escape character in a JSON string";
							break;
						case ParserError::JSONStringMissQuotationMark:
							Info = "missing a closing quotation mark in a JSON string";
							break;
						case ParserError::JSONStringInvalidEncoding:
							Info = "invalid encoding in a JSON string";
							break;
						case ParserError::JSONNumberTooBig:
							Info = "JSON number too big to be stored in double";
							break;
						case ParserError::JSONNumberMissFraction:
							Info = "missing fraction part in a JSON number";
							break;
						case ParserError::JSONNumberMissExponent:
							Info = "missing exponent in a JSON number";
							break;
						case ParserError::JSONTermination:
							Info = "unexpected end of file while parsing a JSON document";
							break;
						case ParserError::JSONUnspecificSyntaxError:
							Info = "unspecified JSON syntax error";
							break;
						default:
							Info = "(unrecognized condition)";
							break;
					}
				}
				else
					Info = NewMessage;

				if (Offset >= 0)
				{
					Info += " at offset ";
					Info += ToString(Offset);
				}
			}
			const char* ParserException::what() const noexcept
			{
				return Info.c_str();
			}
		}

		typedef moodycamel::BlockingConcurrentQueue<TaskCallback> FastQueue;
		typedef moodycamel::ConsumerToken ReceiveToken;

		struct Measurement
		{
#ifndef NDEBUG
			const char* File = nullptr;
			const char* Function = nullptr;
			void* Id = nullptr;
			uint64_t Threshold = 0;
			uint64_t Time = 0;
			int Line = 0;

			void NotifyOfOverConsumption()
			{
				if (Threshold == (uint64_t)Timings::Infinite)
					return;

				uint64_t Delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - Time;
				if (Delta <= Threshold)
					return;

				String BackTrace = ErrorHandling::GetMeasureTrace();
				VI_WARN("[stall] operation took %" PRIu64 " ms (%" PRIu64 " us), back trace %s", Delta / 1000, Delta, BackTrace.c_str());
			}
#endif
		};

		struct ConcurrentQueue
		{
			OrderedMap<std::chrono::microseconds, Timeout> Timers;
			std::condition_variable Notify;
			std::mutex Update;
			FastQueue Tasks;
		};

		struct Cocontext
		{
#ifdef VI_FCTX
			fcontext_t Context = nullptr;
			char* Stack = nullptr;
#elif VI_MICROSOFT
			LPVOID Context = nullptr;
			bool Main = false;
#else
			ucontext_t Context = nullptr;
			char* Stack = nullptr;
#endif
			Cocontext()
			{
#ifndef VI_FCTX
#ifdef VI_MICROSOFT
				Context = ConvertThreadToFiber(nullptr);
				Main = true;
#endif
#endif
			}
			Cocontext(Costate* State)
			{
#ifdef VI_FCTX
				Stack = VI_MALLOC(char, sizeof(char) * State->Size);
				Context = make_fcontext(Stack + State->Size, State->Size, [](transfer_t Transfer)
				{
					Costate::ExecutionEntry(&Transfer);
				});
#elif VI_MICROSOFT
				Context = CreateFiber(State->Size, &Costate::ExecutionEntry, (LPVOID)State);
#else
				getcontext(&Context);
				Stack = VI_MALLOC(char, sizeof(char) * State->Size);
				Context.uc_stack.ss_sp = Stack;
				Context.uc_stack.ss_size = State->Size;
				Context.uc_stack.ss_flags = 0;
				Context.uc_link = &State->Master->Context;

				int X, Y;
				Pack2_64((void*)State, &X, &Y);
				makecontext(&Context, (void(*)()) & Costate::ExecutionEntry, 2, X, Y);
#endif
			}
			~Cocontext()
			{
#ifdef VI_FCTX
				VI_FREE(Stack);
#elif VI_MICROSOFT
				if (Main)
					ConvertFiberToThread();
				else if (Context != nullptr)
					DeleteFiber(Context);
#else
				VI_FREE(Stack);
#endif
			}
		};

		MemoryContext::MemoryContext() : Source("?.cpp"), Function("?"), TypeName("void"), Line(0)
		{
		}
		MemoryContext::MemoryContext(const char* NewSource, const char* NewFunction, const char* NewTypeName, int NewLine) : Source(NewSource ? NewSource : "?.cpp"), Function(NewFunction ? NewFunction : "?"), TypeName(NewTypeName ? NewTypeName : "void"), Line(NewLine >= 0 ? NewLine : 0)
		{
		}

		static thread_local LocalAllocator* Local = nullptr;
		void* Memory::Malloc(size_t Size) noexcept
		{
			VI_ASSERT(Size > 0, "cannot allocate zero bytes");
			if (Local != nullptr)
			{
				void* Address = Local->Allocate(Size);
				VI_PANIC(Address != nullptr, "application is out of local memory allocating %" PRIu64 " bytes", (uint64_t)Size);
				return Address;
			}
			else if (Global != nullptr)
			{
				void* Address = Global->Allocate(Size);
				VI_PANIC(Address != nullptr, "application is out of global memory allocating %" PRIu64 " bytes", (uint64_t)Size);
				return Address;
			}
			else if (!Context)
				Context = new State();

			void* Address = malloc(Size);
			VI_PANIC(Address != nullptr, "application is out of system memory allocating %" PRIu64 " bytes", (uint64_t)Size);
			UMutex<std::mutex> Unique(Context->Mutex);
			Context->Allocations[Address].second = Size;
			return Address;
		}
		void* Memory::MallocContext(size_t Size, MemoryContext&& Origin) noexcept
		{
			VI_ASSERT(Size > 0, "cannot allocate zero bytes");
			if (Local != nullptr)
			{
				void* Address = Local->Allocate(Size);
				VI_PANIC(Address != nullptr, "application is out of global memory allocating %" PRIu64 " bytes", (uint64_t)Size);
				return Address;
			}
			else if (Global != nullptr)
			{
				void* Address = Global->Allocate(std::move(Origin), Size);
				VI_PANIC(Address != nullptr, "application is out of global memory allocating %" PRIu64 " bytes", (uint64_t)Size);
				return Address;
			}
			else if (!Context)
				Context = new State();

			void* Address = malloc(Size);
			VI_PANIC(Address != nullptr, "application is out of system memory allocating %" PRIu64 " bytes", (uint64_t)Size);
			UMutex<std::mutex> Unique(Context->Mutex);
			auto& Item = Context->Allocations[Address];
			Item.first = std::move(Origin);
			Item.second = Size;
			return Address;
		}
		void Memory::Free(void* Address) noexcept
		{
			if (!Address)
				return;

			if (Local != nullptr)
				return Local->Free(Address);
			else if (Global != nullptr)
				return Global->Free(Address);
			else if (!Context)
				Context = new State();

			UMutex<std::mutex> Unique(Context->Mutex);
			Context->Allocations.erase(Address);
			free(Address);
		}
		void Memory::Watch(void* Address, MemoryContext&& Origin) noexcept
		{
			VI_ASSERT(Global != nullptr, "allocator should be set");
			VI_ASSERT(Address != nullptr, "address should be set");
			Global->Watch(std::move(Origin), Address);
		}
		void Memory::Unwatch(void* Address) noexcept
		{
			VI_ASSERT(Global != nullptr, "allocator should be set");
			VI_ASSERT(Address != nullptr, "address should be set");
			Global->Unwatch(Address);
		}
		void Memory::Cleanup() noexcept
		{
			SetGlobalAllocator(nullptr);
		}
		void Memory::SetGlobalAllocator(GlobalAllocator* NewAllocator) noexcept
		{
			if (Global != nullptr)
				Global->Finalize();

			Global = NewAllocator;
			if (Global != nullptr && Context != nullptr)
			{
				for (auto& Item : Context->Allocations)
				{
#ifndef NDEBUG
					Global->Transfer(Item.first, MemoryContext(Item.second.first), Item.second.second);
#else
					Global->Transfer(Item.first, Item.second.second);
#endif
				}
			}

			delete Context;
			Context = nullptr;
		}
		void Memory::SetLocalAllocator(LocalAllocator* NewAllocator) noexcept
		{
			Local = NewAllocator;
		}
		bool Memory::IsValidAddress(void* Address) noexcept
		{
			VI_ASSERT(Global != nullptr, "allocator should be set");
			VI_ASSERT(Address != nullptr, "address should be set");
			if (Local != nullptr && Local->IsValid(Address))
				return true;

			return Global->IsValid(Address);
		}
		GlobalAllocator* Memory::GetGlobalAllocator() noexcept
		{
			return Global;
		}
		LocalAllocator* Memory::GetLocalAllocator() noexcept
		{
			return Local;
		}
		GlobalAllocator* Memory::Global = nullptr;
		Memory::State* Memory::Context = nullptr;

		StackTrace::StackTrace(size_t Skips, size_t MaxDepth)
		{
			Scripting::ImmediateContext* Context = Scripting::ImmediateContext::Get();
			if (Context != nullptr)
			{
				Scripting::VirtualMachine* VM = Context->GetVM();
				size_t CallstackSize = Context->GetCallstackSize();
				Frames.reserve(CallstackSize);
				for (size_t i = 0; i < CallstackSize; i++)
				{
					int ColumnNumber = 0;
					int LineNumber = Context->GetLineNumber(i, &ColumnNumber);
					Scripting::Function Next = Context->GetFunction(i);
					Frame Target;
					if (Next.GetSectionName())
					{
						const char* SectionName = Next.GetSectionName();
						auto SourceCode = VM->GetSourceCodeAppendixByPath("source", SectionName, (uint32_t)LineNumber, (uint32_t)ColumnNumber, 5);
						if (SourceCode)
							Target.Code = *SourceCode;
						Target.File = SectionName;
					}
					else
						Target.File = "[native]";
					Target.Function = (Next.GetDecl() ? Next.GetDecl() : "[optimized]");
					Target.Line = (uint32_t)LineNumber;
					Target.Column = (uint32_t)ColumnNumber;
					Target.Handle = (void*)Next.GetFunction();
					Target.Native = false;
					Frames.emplace_back(std::move(Target));
				}
			}
#ifdef VI_CXX23
			using StackTraceContainer = std::basic_stacktrace<StandardAllocator<std::stacktrace_entry>>;
			StackTraceContainer Stack = StackTraceContainer::current(Skips + 2, MaxDepth + Skips + 2);
			Frames.reserve((size_t)Stack.size());

			for (auto& Next : Stack)
			{
				Frame Target;
				Target.File = Copy<String>(Next.source_file());
				Target.Function = Copy<String>(Next.description());
				Target.Line = (uint32_t)Next.source_line();
				Target.Column = 0;
				Target.Handle = (void*)Next.native_handle();
				Target.Native = true;
				if (Target.File.empty())
					Target.File = "[external]";
				if (Target.Function.empty())
					Target.Function = "[optimized]";
				Frames.emplace_back(std::move(Target));
			}
#elif defined(VI_BACKTRACE)
			static bool IsPreloaded = false;
			if (!IsPreloaded)
			{
				backward::StackTrace EmptyStack;
				EmptyStack.load_here();
				backward::TraceResolver EmptyResolver;
				EmptyResolver.load_stacktrace(EmptyStack);
				IsPreloaded = true;
			}

			backward::StackTrace Stack;
			Stack.load_here(MaxDepth + Skips + 3);
			Stack.skip_n_firsts(Skips + 3);
			Frames.reserve(Stack.size());

			backward::TraceResolver Resolver;
			Resolver.load_stacktrace(Stack);

			size_t Size = Stack.size();
			for (size_t i = 0; i < Size; i++)
			{
				backward::ResolvedTrace Next = Resolver.resolve(Stack[i]);
				Frame Target;
				Target.File = Next.source.filename.empty() ? (Next.object_filename.empty() ? "[external]" : Next.object_filename.c_str()) : Next.source.filename.c_str();
				Target.Function = Next.source.function.empty() ? "[optimized]" : Next.source.function.c_str();
				Target.Line = (uint32_t)Next.source.line;
				Target.Column = 0;
				Target.Handle = Next.addr;
				Target.Native = true;
				if (!Next.source.function.empty() && Target.Function.back() != ')')
					Target.Function += "()";
				Frames.emplace_back(std::move(Target));
			}
#endif
		}
		StackTrace::StackPtr::const_iterator StackTrace::begin() const
		{
			return Frames.begin();
		}
		StackTrace::StackPtr::const_iterator StackTrace::end() const
		{
			return Frames.end();
		}
		StackTrace::StackPtr::const_reverse_iterator StackTrace::rbegin() const
		{
			return Frames.rbegin();
		}
		StackTrace::StackPtr::const_reverse_iterator StackTrace::rend() const
		{
			return Frames.rend();
		}
		StackTrace::operator bool() const
		{
			return !Frames.empty();
		}
		const StackTrace::StackPtr& StackTrace::Range() const
		{
			return Frames;
		}
		bool StackTrace::Empty() const
		{
			return Frames.empty();
		}
		size_t StackTrace::Size() const
		{
			return Frames.size();
		}
#ifndef NDEBUG
		static thread_local std::stack<Measurement> MeasuringTree;
#endif
		static thread_local bool IgnoreLogging = false;
		ErrorHandling::Tick::Tick(bool Active) noexcept : IsCounting(Active)
		{
		}
		ErrorHandling::Tick::Tick(Tick&& Other) noexcept : IsCounting(Other.IsCounting)
		{
			Other.IsCounting = false;
		}
		ErrorHandling::Tick::~Tick() noexcept
		{
#ifndef NDEBUG
			if (IgnoreLogging || !IsCounting)
				return;

			VI_ASSERT(!MeasuringTree.empty(), "debug frame should be set");
			auto& Next = MeasuringTree.top();
			Next.NotifyOfOverConsumption();
			MeasuringTree.pop();
#endif
		}
		ErrorHandling::Tick& ErrorHandling::Tick::operator =(Tick&& Other) noexcept
		{
			if (&Other == this)
				return *this;

			IsCounting = Other.IsCounting;
			Other.IsCounting = false;
			return *this;
		}

		void ErrorHandling::Panic(int Line, const char* Source, const char* Function, const char* Condition, const char* Format, ...) noexcept
		{
			Details Data;
			Data.Origin.File = Source ? OS::Path::GetFilename(Source) : nullptr;
			Data.Origin.Line = Line;
			Data.Type.Level = LogLevel::Error;
			Data.Type.Fatal = true;
			if (HasFlag(LogOption::ReportSysErrors) || true)
			{
				int ErrorCode = OS::Error::Get();
				if (!OS::Error::IsError(ErrorCode))
					goto DefaultFormat;
				
				Data.Message.Size = snprintf(Data.Message.Data, sizeof(Data.Message.Data), "thread %s PANIC! %s(): %s on \"!(%s)\", latest system error [%s]\n%s",
					OS::Process::GetThreadId(std::this_thread::get_id()).c_str(),
					Function ? Function : "?",
					Format ? Format : "check failed",
					Condition ? Condition : "?",
					OS::Error::GetName(ErrorCode).c_str(),
					ErrorHandling::GetStackTrace(1).c_str());
			}
			else
			{
			DefaultFormat:
				Data.Message.Size = snprintf(Data.Message.Data, sizeof(Data.Message.Data), "thread %s PANIC! %s(): %s on \"!(%s)\"\n%s",
					OS::Process::GetThreadId(std::this_thread::get_id()).c_str(),
					Function ? Function : "?",
					Format ? Format : "check failed",
					Condition ? Condition : "?",
					ErrorHandling::GetStackTrace(1).c_str());
			}
			if (HasFlag(LogOption::Dated))
				DateTime::FetchDateTime(Data.Message.Date, sizeof(Data.Message.Date), time(nullptr));

			if (Format != nullptr)
			{
				va_list Args;
				va_start(Args, Format);
				char Buffer[BLOB_SIZE] = { '\0' };
				Data.Message.Size = vsnprintf(Buffer, sizeof(Buffer), Data.Message.Data, Args);
				if (Data.Message.Size > sizeof(Data.Message.Data))
					Data.Message.Size = sizeof(Data.Message.Data);
				memcpy(Data.Message.Data, Buffer, sizeof(Buffer));
				va_end(Args);
			}

			if (Data.Message.Size > 0)
			{
#ifndef NDEBUG
				Console::Get()->Show();
#endif
				EscapeText(Data.Message.Data, (size_t)Data.Message.Size);
				Dispatch(Data);
			}

			ErrorHandling::Pause();
			OS::Process::Abort();
		}
		void ErrorHandling::Assert(int Line, const char* Source, const char* Function, const char* Condition, const char* Format, ...) noexcept
		{
			Details Data;
			Data.Origin.File = Source ? OS::Path::GetFilename(Source) : nullptr;
			Data.Origin.Line = Line;
			Data.Type.Level = LogLevel::Error;
			Data.Type.Fatal = true;
			Data.Message.Size = snprintf(Data.Message.Data, sizeof(Data.Message.Data), "thread %s ASSERT %s(): %s on \"!(%s)\"\n%s",
				OS::Process::GetThreadId(std::this_thread::get_id()).c_str(),
				Function ? Function : "?",
				Format ? Format : "assertion failed",
				Condition ? Condition : "?",
				ErrorHandling::GetStackTrace(1).c_str());
			if (HasFlag(LogOption::Dated))
				DateTime::FetchDateTime(Data.Message.Date, sizeof(Data.Message.Date), time(nullptr));

			if (Format != nullptr)
			{
				va_list Args;
				va_start(Args, Format);
				char Buffer[BLOB_SIZE] = { '\0' };
				Data.Message.Size = vsnprintf(Buffer, sizeof(Buffer), Data.Message.Data, Args);
				if (Data.Message.Size > sizeof(Data.Message.Data))
					Data.Message.Size = sizeof(Data.Message.Data);
				memcpy(Data.Message.Data, Buffer, sizeof(Buffer));
				va_end(Args);
			}

			if (Data.Message.Size > 0)
			{
#ifndef NDEBUG
				Console::Get()->Show();
#endif
				EscapeText(Data.Message.Data, (size_t)Data.Message.Size);
				Dispatch(Data);
			}

			ErrorHandling::Pause();
			OS::Process::Abort();
		}
		void ErrorHandling::Message(LogLevel Level, int Line, const char* Source, const char* Format, ...) noexcept
		{
			VI_ASSERT(Format != nullptr, "format string should be set");
			if (IgnoreLogging || (!HasFlag(LogOption::Active) && !HasCallback()))
				return;

			Details Data;
			Data.Origin.File = Source ? OS::Path::GetFilename(Source) : nullptr;
			Data.Origin.Line = Line;
			Data.Type.Level = Level;
			Data.Type.Fatal = false;
			if (HasFlag(LogOption::Dated))
				DateTime::FetchDateTime(Data.Message.Date, sizeof(Data.Message.Date), time(nullptr));

			char Buffer[512] = { '\0' };
			if (Level == LogLevel::Error && HasFlag(LogOption::ReportSysErrors))
			{
				int ErrorCode = OS::Error::Get();
				if (!OS::Error::IsError(ErrorCode))
					goto DefaultFormat;

				snprintf(Buffer, sizeof(Buffer), "%s, latest system error [%s]", Format, OS::Error::GetName(ErrorCode).c_str());
			}
			else
			{
			DefaultFormat:
				memcpy(Buffer, Format, std::min(sizeof(Buffer), strlen(Format)));
			}

			va_list Args;
			va_start(Args, Format);
			Data.Message.Size = vsnprintf(Data.Message.Data, sizeof(Data.Message.Data), Buffer, Args);
			if (Data.Message.Size > sizeof(Data.Message.Data))
				Data.Message.Size = sizeof(Data.Message.Data);
			va_end(Args);

			if (Data.Message.Size > 0)
			{
				EscapeText(Data.Message.Data, (size_t)Data.Message.Size);
				Enqueue(std::move(Data));
			}
		}
		void ErrorHandling::Enqueue(Details&& Data) noexcept
		{
			if (HasFlag(LogOption::Async) && Schedule::IsAvailable())
				Schedule::Get()->SetTask([Data = std::move(Data)]() mutable { Dispatch(Data); });
			else
				Dispatch(Data);
		}
		void ErrorHandling::Dispatch(Details& Data) noexcept
		{
			if (HasCallback())
			{
				IgnoreLogging = true;
				Context->Callback(Data);
				IgnoreLogging = false;
			}
#if defined(VI_MICROSOFT) && !defined(NDEBUG)
			OutputDebugStringA(GetMessageText(Data).c_str());
#endif
			if (!Console::IsAvailable())
				return;

			Console::Get()->Synced([&Data](Console* Base)
			{
				if (!HasFlag(LogOption::Pretty))
					Base->Write(GetMessageText(Data));
				else
					Colorify(Base, Data);
			});
		}
		void ErrorHandling::Colorify(Console* Base, Details& Data) noexcept
		{
#if VI_DLEVEL < 5
			bool ParseTokens = Data.Type.Level != LogLevel::Trace && Data.Type.Level != LogLevel::Debug;
#else
			bool ParseTokens = Data.Type.Level != LogLevel::Trace;
#endif
			Base->ColorBegin(ParseTokens ? StdColor::Cyan : StdColor::Gray);
			if (HasFlag(LogOption::Dated))
			{
				Base->WriteBuffer(Data.Message.Date);
				Base->WriteBuffer(" ");
#ifndef NDEBUG
				if (Data.Origin.File != nullptr)
				{
					Base->ColorBegin(StdColor::Gray);
					Base->WriteBuffer(Data.Origin.File);
					Base->WriteBuffer(":");
					if (Data.Origin.Line > 0)
					{
						Base->Write(Core::ToString(Data.Origin.Line));
						Base->WriteBuffer(" ");
					}
				}
#endif
			}
			Base->ColorBegin(GetMessageColor(Data));
			Base->WriteBuffer(GetMessageType(Data));
			Base->WriteBuffer(" ");
			if (ParseTokens)
				Base->ColorPrintBuffer(StdColor::LightGray, Data.Message.Data, Data.Message.Size);
			else
				Base->WriteBuffer(Data.Message.Data);
			Base->WriteBuffer("\n");
			Base->ColorEnd();
		}
		void ErrorHandling::Pause() noexcept
		{
			OS::Process::Interrupt();
		}
		void ErrorHandling::Cleanup() noexcept
		{
			delete Context;
			Context = nullptr;
		}
		void ErrorHandling::SetCallback(const std::function<void(Details&)>& _Callback) noexcept
		{
			if (!Context)
				Context = new State();

			Context->Callback = _Callback;
		}
		void ErrorHandling::SetFlag(LogOption Option, bool Active) noexcept
		{
			if (!Context)
				Context = new State();

			if (Active)
				Context->Flags = Context->Flags | (uint32_t)Option;
			else
				Context->Flags = Context->Flags & ~(uint32_t)Option;
		}
		bool ErrorHandling::HasFlag(LogOption Option) noexcept
		{
			if (!Context)
				return ((uint32_t)LogOption::Pretty) & (uint32_t)Option;

			return Context->Flags & (uint32_t)Option;
		}
		bool ErrorHandling::HasCallback() noexcept
		{
			return Context != nullptr && Context->Callback != nullptr;
		}
		ErrorHandling::Tick ErrorHandling::Measure(const char* File, const char* Function, int Line, uint64_t ThresholdMS) noexcept
		{
#ifndef NDEBUG
			VI_ASSERT(File != nullptr, "file should be set");
			VI_ASSERT(Function != nullptr, "function should be set");
			VI_ASSERT(ThresholdMS > 0 || ThresholdMS == (uint64_t)Timings::Infinite, "threshold time should be greater than zero");
			if (IgnoreLogging)
				return ErrorHandling::Tick(false);

			Measurement Next;
			Next.File = File;
			Next.Function = Function;
			Next.Threshold = ThresholdMS * 1000;
			Next.Time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			Next.Line = Line;

			MeasuringTree.emplace(std::move(Next));
			return ErrorHandling::Tick(true);
#else
			return ErrorHandling::Tick(false);
#endif
		}
		void ErrorHandling::MeasureLoop() noexcept
		{
#ifndef NDEBUG
			if (!IgnoreLogging)
			{
				VI_ASSERT(!MeasuringTree.empty(), "debug frame should be set");
				auto& Next = MeasuringTree.top();
				Next.NotifyOfOverConsumption();
			}
#endif
		}
		String ErrorHandling::GetMeasureTrace() noexcept
		{
#ifndef NDEBUG
			auto Source = MeasuringTree;
			StringStream Stream;
			Stream << "in thread " << std::this_thread::get_id() << ":\n";

			size_t Size = Source.size();
			for (size_t TraceIdx = Source.size(); TraceIdx > 0; --TraceIdx)
			{
				auto& Frame = Source.top();
				Stream << "  #" << (Size - TraceIdx) + 1 << " at " << OS::Path::GetFilename(Frame.File);
				if (Frame.Line > 0)
					Stream << ":" << Frame.Line;
				Stream << " in " << Frame.Function << "()";
				if (Frame.Id != nullptr)
					Stream << " pointed here 0x" << Frame.Id;
				Stream << ", at most " << Frame.Threshold / 1000 << " ms\n";
				Source.pop();
			}

			String Out(Stream.str());
			return Out.substr(0, Out.size() - 1);
#else
			return String();
#endif
		}
		String ErrorHandling::GetStackTrace(size_t Skips, size_t MaxFrames) noexcept
		{
			StackTrace Stack(Skips, MaxFrames);
			if (!Stack)
				return "  * #0 [unavailable]";

			StringStream Stream;
			size_t Size = Stack.Size();
			for (auto& Frame : Stack)
			{
				Stream << "  #" << --Size << " at " << OS::Path::GetFilename(Frame.File.c_str());
				if (Frame.Line > 0)
					Stream << ":" << Frame.Line;
				if (Frame.Column > 0)
					Stream << "," << Frame.Column;
				Stream << " in " << Frame.Function;
				if (!Frame.Native)
					Stream << " (vcall)";
				if (!Frame.Code.empty())
				{
					auto Lines = Stringify::Split(Frame.Code, '\n');
					for (size_t i = 0; i < Lines.size(); i++)
					{
						Stream << "  " << Lines[i];
						if (i + 1 < Lines.size())
							Stream << "\n";
					}
				}
				if (Size > 0)
					Stream << "\n";
			}

			return Stream.str();
		}
		const char* ErrorHandling::GetMessageType(const Details& Base) noexcept
		{
			switch (Base.Type.Level)
			{
				case LogLevel::Error:
					return "ERROR";
				case LogLevel::Warning:
					return "WARN";
				case LogLevel::Info:
					return "INFO";
				case LogLevel::Debug:
					return "DEBUG";
				case LogLevel::Trace:
					return "TRACE";
				default:
					return "LOG";
			}
		}
		StdColor ErrorHandling::GetMessageColor(const Details& Base) noexcept
		{
			switch (Base.Type.Level)
			{
				case LogLevel::Error:
					return StdColor::DarkRed;
				case LogLevel::Warning:
					return StdColor::Orange;
				case LogLevel::Info:
					return StdColor::LightBlue;
				case LogLevel::Debug:
					return StdColor::Gray;
				case LogLevel::Trace:
					return StdColor::Gray;
				default:
					return StdColor::LightGray;
			}
		}
		String ErrorHandling::GetMessageText(const Details& Base) noexcept
		{
			StringStream Stream;
			if (HasFlag(LogOption::Dated))
			{
				Stream << Base.Message.Date;
#ifndef NDEBUG
				if (Base.Origin.File != nullptr)
				{
					Stream << ' ' << Base.Origin.File << ':';
					if (Base.Origin.Line > 0)
						Stream << Base.Origin.Line;
				}
#endif
				Stream << ' ';
			}
			Stream << GetMessageType(Base) << ' ';
			Stream << Base.Message.Data << '\n';
			return Stream.str();
		}
		ErrorHandling::State* ErrorHandling::Context = nullptr;

		Coroutine::Coroutine(Costate* Base, const TaskCallback& Procedure) noexcept : State(Coexecution::Active), Callback(Procedure), Slave(VI_NEW(Cocontext, Base)), Master(Base), UserData(nullptr)
		{
		}
		Coroutine::Coroutine(Costate* Base, TaskCallback&& Procedure) noexcept : State(Coexecution::Active), Callback(std::move(Procedure)), Slave(VI_NEW(Cocontext, Base)), Master(Base), UserData(nullptr)
		{
		}
		Coroutine::~Coroutine() noexcept
		{
			VI_DELETE(Cocontext, Slave);
		}

		Decimal::Decimal() noexcept : Length(0), Sign('\0')
		{
		}
		Decimal::Decimal(const String& Value) noexcept : Length(0)
		{
			InitializeFromText(Value.c_str(), Value.size());
		}
		Decimal::Decimal(const Decimal& Value) noexcept : Source(Value.Source), Length(Value.Length), Sign(Value.Sign)
		{
		}
		Decimal::Decimal(Decimal&& Value) noexcept : Source(std::move(Value.Source)), Length(Value.Length), Sign(Value.Sign)
		{
		}
		void Decimal::InitializeFromText(const char* Text, size_t Size) noexcept
		{
			if (!Size)
			{
			InvalidNumber:
				Source.clear();
				Length = 0;
				Sign = '\0';
				return;
			}

			bool Points = false;
			size_t Index = 0;
			int8_t Direction = Text[0];
			if (Direction != '+' && Direction != '-')
			{
				if (!isdigit(Direction))
					goto InvalidNumber;
				Direction = '+';
			}
			else
				Index = 1;

			Sign = Direction;
			while (Text[Index] != '\0')
			{
				if (!Points && Text[Index] == '.')
				{
					if (Source.empty())
						goto InvalidNumber;

					Points = true;
					++Index;
				}
				else if (!isdigit(Text[Index]))
					goto InvalidNumber;

				Source.insert(0, 1, Text[Index++]);
				if (Points)
					Length++;
			}

			Trim();
		}
		void Decimal::InitializeFromZero() noexcept
		{
			Source.push_back('0');
			Sign = '+';
			Length = 0;
		}
		Decimal& Decimal::Truncate(uint32_t Precision)
		{
			if (IsNaN())
				return *this;

			int32_t NewPrecision = Precision;
			if (Length < NewPrecision)
			{
				while (Length < NewPrecision)
				{
					Length++;
					Source.insert(0, 1, '0');
				}
			}
			else if (Length > NewPrecision)
			{
				while (Length > NewPrecision)
				{
					Length--;
					Source.erase(0, 1);
				}
			}

			return *this;
		}
		Decimal& Decimal::Round(uint32_t Precision)
		{
			if (IsNaN())
				return *this;

			int32_t NewPrecision = Precision;
			if (Length < NewPrecision)
			{
				while (Length < NewPrecision)
				{
					Length++;
					Source.insert(0, 1, '0');
				}
			}
			else if (Length > NewPrecision)
			{
				char Last;
				while (Length > NewPrecision)
				{
					Last = Source[0];
					Length--;
					Source.erase(0, 1);
				}

				if (CharToInt(Last) >= 5)
				{
					if (NewPrecision != 0)
					{
						String Result = "0.";
						Result.reserve(3 + (size_t)NewPrecision);
						for (int32_t i = 1; i < NewPrecision; i++)
							Result += '0';
						Result += '1';

						Decimal Temp(Result);
						*this = *this + Temp;
					}
					else
						++(*this);
				}
			}

			return *this;
		}
		Decimal& Decimal::Trim()
		{
			return Unlead().Untrail();
		}
		Decimal& Decimal::Unlead()
		{
			for (int32_t i = (int32_t)Source.size() - 1; i > Length; --i)
			{
				if (Source[i] != '0')
					break;

				Source.pop_back();
			}

			return *this;
		}
		Decimal& Decimal::Untrail()
		{
			if (IsNaN() || Source.empty())
				return *this;

			while ((Source[0] == '0') && (Length > 0))
			{
				Source.erase(0, 1);
				Length--;
			}

			return *this;
		}
		bool Decimal::IsNaN() const
		{
			return Sign == '\0';
		}
		bool Decimal::IsZero() const
		{
			for (char Item : Source)
			{
				if (Item != '0')
					return false;
			}

			return true;
		}
		bool Decimal::IsZeroOrNaN() const
		{
			return IsNaN() || IsZero();
		}
		bool Decimal::IsPositive() const
		{
			return !IsNaN() && *this > 0.0;
		}
		bool Decimal::IsNegative() const
		{
			return !IsNaN() && *this < 0.0;
		}
		double Decimal::ToDouble() const
		{
			if (IsNaN())
				return std::nan("");

			double Dec = 1;
			if (Length > 0)
			{
				int32_t Aus = Length;
				while (Aus != 0)
				{
					Dec /= 10;
					Aus--;
				}
			}

			double Var = 0;
			for (char Char : Source)
			{
				Var += CharToInt(Char) * Dec;
				Dec *= 10;
			}

			if (Sign == '-')
				Var *= -1;

			return Var;
		}
		float Decimal::ToFloat() const
		{
			return (float)ToDouble();
		}
		int8_t Decimal::ToInt8() const
		{
			return (int8_t)ToInt16();
		}
		uint8_t Decimal::ToUInt8() const
		{
			return (uint8_t)ToUInt16();
		}
		int16_t Decimal::ToInt16() const
		{
			return (int16_t)ToInt32();
		}
		uint16_t Decimal::ToUInt16() const
		{
			return (uint16_t)ToUInt32();
		}
		int32_t Decimal::ToInt32() const
		{
			return (int32_t)ToInt64();
		}
		uint32_t Decimal::ToUInt32() const
		{
			return (uint32_t)ToUInt64();
		}
		int64_t Decimal::ToInt64() const
		{
			if (IsNaN() || Source.empty())
				return 0;

			String Result;
			if (Sign == '-')
				Result += Sign;

			int32_t Offset = 0, Size = Length;
			while ((Source[Offset] == '0') && (Size > 0))
			{
				Offset++;
				Size--;
			}

			for (int32_t i = (int32_t)Source.size() - 1; i >= Offset; i--)
			{
				Result += Source[i];
				if ((i == Length) && (i != 0) && Offset != Length)
					break;
			}

			return strtoll(Result.c_str(), nullptr, 10);
		}
		uint64_t Decimal::ToUInt64() const
		{
			if (IsNaN() || Source.empty())
				return 0;

			String Result;
			int32_t Offset = 0, Size = Length;
			while ((Source[Offset] == '0') && (Size > 0))
			{
				Offset++;
				Size--;
			}

			for (int32_t i = (int32_t)Source.size() - 1; i >= Offset; i--)
			{
				Result += Source[i];
				if ((i == Length) && (i != 0) && Offset != Length)
					break;
			}

			return strtoull(Result.c_str(), nullptr, 10);
		}
		String Decimal::ToString() const
		{
			if (IsNaN() || Source.empty())
				return "NaN";

			String Result;
			if (Sign == '-')
				Result += Sign;

			int32_t Offset = 0, Size = Length;
			while ((Source[Offset] == '0') && (Size > 0))
			{
				Offset++;
				Size--;
			}

			for (int32_t i = (int32_t)Source.size() - 1; i >= Offset; i--)
			{
				Result += Source[i];
				if ((i == Length) && (i != 0) && Offset != Length)
					Result += '.';
			}

			return Result;
		}
		String Decimal::Exp() const
		{
			if (IsNaN())
				return "NaN";

			String Result;
			int Compare = Decimal::CompareNum(*this, Decimal(1));
			if (Compare == 0)
			{
				Result += Sign;
				Result += "1e+0";
			}
			else if (Compare == 1)
			{
				Result += Sign;
				int32_t i = (int32_t)Source.size() - 1;
				Result += Source[i];
				i--;

				if (i > 0)
				{
					Result += '.';
					for (; (i >= (int32_t)Source.size() - 6) && (i >= 0); --i)
						Result += Source[i];
				}
				Result += "e+";
				Result += Core::ToString(Ints() - 1);
			}
			else if (Compare == 2)
			{
				int32_t Exp = 0, Count = (int32_t)Source.size() - 1;
				while (Count > 0 && Source[Count] == '0')
				{
					Count--;
					Exp++;
				}

				if (Count == 0)
				{
					if (Source[Count] != '0')
					{
						Result += Sign;
						Result += Source[Count];
						Result += "e-";
						Result += Core::ToString(Exp);
					}
					else
						Result += "+0";
				}
				else
				{
					Result += Sign;
					Result += Source[Count];
					Result += '.';

					for (int32_t i = Count - 1; (i >= Count - 5) && (i >= 0); --i)
						Result += Source[i];

					Result += "e-";
					Result += Core::ToString(Exp);
				}
			}

			return Result;
		}
		uint32_t Decimal::Decimals() const
		{
			return Length;
		}
		uint32_t Decimal::Ints() const
		{
			return (int32_t)Source.size() - Length;
		}
		uint32_t Decimal::Size() const
		{
			return (int32_t)(sizeof(*this) + Source.size() * sizeof(char));
		}
		Decimal Decimal::operator -() const
		{
			Decimal Result = *this;
			if (Result.Sign == '+')
				Result.Sign = '-';
			else if (Result.Sign == '-')
				Result.Sign = '+';

			return Result;
		}
		Decimal& Decimal::operator *=(const Decimal& V)
		{
			*this = *this * V;
			return *this;
		}
		Decimal& Decimal::operator /=(const Decimal& V)
		{
			*this = *this / V;
			return *this;
		}
		Decimal& Decimal::operator +=(const Decimal& V)
		{
			*this = *this + V;
			return *this;
		}
		Decimal& Decimal::operator -=(const Decimal& V)
		{
			*this = *this - V;
			return *this;
		}
		Decimal& Decimal::operator=(const Decimal& Value) noexcept
		{
			Source = Value.Source;
			Length = Value.Length;
			Sign = Value.Sign;
			return *this;
		}
		Decimal& Decimal::operator=(Decimal&& Value) noexcept
		{
			Source = std::move(Value.Source);
			Length = Value.Length;
			Sign = Value.Sign;
			return *this;
		}
		Decimal& Decimal::operator++(int)
		{
			*this = *this + 1;
			return *this;
		}
		Decimal& Decimal::operator++()
		{
			*this = *this + 1;
			return *this;
		}
		Decimal& Decimal::operator--(int)
		{
			*this = *this - 1;
			return *this;
		}
		Decimal& Decimal::operator--()
		{
			*this = *this - 1;
			return *this;
		}
		bool Decimal::operator==(const Decimal& Right) const
		{
			if (IsNaN() || Right.IsNaN())
				return false;

			int Check = CompareNum(*this, Right);
			if ((Check == 0) && (Sign == Right.Sign))
				return true;

			return false;
		}
		bool Decimal::operator!=(const Decimal& Right) const
		{
			if (IsNaN() || Right.IsNaN())
				return false;

			return !(*this == Right);
		}
		bool Decimal::operator>(const Decimal& Right) const
		{
			if (IsNaN() || Right.IsNaN())
				return false;

			if (((Sign == '+') && (Right.Sign == '+')))
			{
				int Check = CompareNum(*this, Right);
				return Check == 1;
			}

			if (((Sign == '-') && (Right.Sign == '-')))
			{
				int Check = CompareNum(*this, Right);
				return Check == 2;
			}

			if (((Sign == '-') && (Right.Sign == '+')))
				return false;

			if (((Sign == '+') && (Right.Sign == '-')))
				return true;

			return false;
		}
		bool Decimal::operator>=(const Decimal& Right) const
		{
			if (IsNaN() || Right.IsNaN())
				return false;

			return !(*this < Right);
		}
		bool Decimal::operator<(const Decimal& Right) const
		{
			if (IsNaN() || Right.IsNaN())
				return false;

			if (((Sign == '+') && (Right.Sign == '+')))
			{
				int Check = CompareNum(*this, Right);
				return Check == 2;
			}

			if (((Sign == '-') && (Right.Sign == '-')))
			{
				int Check = CompareNum(*this, Right);
				return Check == 1;
			}

			if (((Sign == '-') && (Right.Sign == '+')))
				return true;

			if (((Sign == '+') && (Right.Sign == '-')))
				return false;

			return false;
		}
		bool Decimal::operator<=(const Decimal& Right) const
		{
			if (IsNaN() || Right.IsNaN())
				return false;

			return !(*this > Right);
		}
		Decimal operator+(const Decimal& _Left, const Decimal& _Right)
		{
			Decimal Temp;
			if (_Left.IsNaN() || _Right.IsNaN())
				return Temp;

			Decimal Left, Right;
			Left = _Left;
			Right = _Right;

			if (Left.Length > Right.Length)
			{
				while (Left.Length > Right.Length)
				{
					Right.Length++;
					Right.Source.insert(0, 1, '0');
				}
			}
			else if (Left.Length < Right.Length)
			{
				while (Left.Length < Right.Length)
				{
					Left.Length++;
					Left.Source.insert(0, 1, '0');
				}
			}

			if ((Left.Sign == '+') && (Right.Sign == '-'))
			{
				int Check = Decimal::CompareNum(Left, Right);
				if (Check == 0)
				{
					Temp = 0;
					return Temp;
				}

				if (Check == 1)
				{
					Temp = Decimal::Subtract(Left, Right);
					Temp.Sign = '+';
					Temp.Length = Left.Length;
					Temp.Unlead();
					return Temp;
				}

				if (Check == 2)
				{
					Temp = Decimal::Subtract(Right, Left);
					Temp.Sign = '-';
					Temp.Length = Left.Length;
					Temp.Unlead();
					return Temp;
				}
			}

			if ((Left.Sign == '-') && (Right.Sign == '+'))
			{
				int Check = Decimal::CompareNum(Left, Right);
				if (Check == 0)
				{
					Temp = 0;
					return Temp;
				}

				if (Check == 1)
				{
					Temp = Decimal::Subtract(Left, Right);
					Temp.Sign = '-';
					Temp.Length = Left.Length;
					Temp.Unlead();
					return Temp;
				}

				if (Check == 2)
				{
					Temp = Decimal::Subtract(Right, Left);
					Temp.Sign = '+';
					Temp.Length = Left.Length;
					Temp.Unlead();
					return Temp;
				}
			}

			if ((Left.Sign == '+') && (Right.Sign == '+'))
			{
				Temp = Decimal::Sum(Left, Right);
				Temp.Sign = '+';
				Temp.Length = Left.Length;
				return Temp;
			}

			if ((Left.Sign == '-') && (Right.Sign == '-'))
			{
				Temp = Decimal::Sum(Left, Right);
				Temp.Sign = '-';
				Temp.Length = Left.Length;
				return Temp;
			}

			return Temp;
		}
		Decimal operator+(const Decimal& Left, const int& VRight)
		{
			Decimal Right;
			Right = VRight;
			return Left + Right;
		}
		Decimal operator+(const Decimal& Left, const double& VRight)
		{
			Decimal Right;
			Right = VRight;
			return Left + Right;
		}
		Decimal operator-(const Decimal& _Left, const Decimal& _Right)
		{
			Decimal Temp;
			if (_Left.IsNaN() || _Right.IsNaN())
				return Temp;

			Decimal Left, Right;
			Left = _Left;
			Right = _Right;

			if (Left.Length > Right.Length)
			{
				while (Left.Length > Right.Length)
				{
					Right.Length++;
					Right.Source.insert(0, 1, '0');
				}
			}
			else if (Left.Length < Right.Length)
			{
				while (Left.Length < Right.Length)
				{
					Left.Length++;
					Left.Source.insert(0, 1, '0');
				}
			}

			if ((Left.Sign == '+') && (Right.Sign == '-'))
			{
				Temp = Decimal::Sum(Left, Right);
				Temp.Sign = '+';
				Temp.Length = Left.Length;
				return Temp;
			}
			if ((Left.Sign == '-') && (Right.Sign == '+'))
			{
				Temp = Decimal::Sum(Left, Right);
				Temp.Sign = '-';
				Temp.Length = Left.Length;
				return Temp;
			}

			if ((Left.Sign == '+') && (Right.Sign == '+'))
			{
				int Check = Decimal::CompareNum(Left, Right);
				if (Check == 0)
				{
					Temp = 0;
					return Temp;
				}

				if (Check == 1)
				{
					Temp = Decimal::Subtract(Left, Right);
					Temp.Sign = '+';
					Temp.Length = Left.Length;
					Temp.Unlead();
					return Temp;
				}

				if (Check == 2)
				{
					Temp = Decimal::Subtract(Right, Left);
					Temp.Sign = '-';
					Temp.Length = Left.Length;
					Temp.Unlead();
					return Temp;
				}
			}

			if ((Left.Sign == '-') && (Right.Sign == '-'))
			{
				int Check = Decimal::CompareNum(Left, Right);
				if (Check == 0)
				{
					Temp = 0;
					return Temp;
				}

				if (Check == 1)
				{
					Temp = Decimal::Subtract(Left, Right);
					Temp.Sign = '-';
					Temp.Length = Left.Length;
					Temp.Unlead();
					return Temp;
				}

				if (Check == 2)
				{
					Temp = Decimal::Subtract(Right, Left);
					Temp.Sign = '+';
					Temp.Length = Left.Length;
					Temp.Unlead();
					return Temp;
				}
			}

			return Temp;
		}
		Decimal operator-(const Decimal& Left, const int& VRight)
		{
			Decimal Right;
			Right = VRight;
			return Left - Right;
		}
		Decimal operator-(const Decimal& Left, const double& VRight)
		{
			Decimal Right;
			Right = VRight;
			return Left - Right;
		}
		Decimal operator*(const Decimal& Left, const Decimal& Right)
		{
			Decimal Temp;
			if (Left.IsNaN() || Right.IsNaN())
				return Temp;

			Temp = Decimal::Multiply(Left, Right);
			if (((Left.Sign == '-') && (Right.Sign == '-')) || ((Left.Sign == '+') && (Right.Sign == '+')))
				Temp.Sign = '+';
			else
				Temp.Sign = '-';

			Temp.Length = Left.Length + Right.Length;
			Temp.Unlead();

			return Temp;
		}
		Decimal operator*(const Decimal& Left, const int& VRight)
		{
			Decimal Right;
			Right = VRight;
			return Left * Right;
		}
		Decimal operator*(const Decimal& Left, const double& VRight)
		{
			Decimal Right;
			Right = VRight;
			return Left * Right;
		}
		Decimal operator/(const Decimal& Left, const Decimal& Right)
		{
			Decimal Temp;
			if (Left.IsNaN() || Right.IsNaN())
				return Temp;

			Decimal Q, R, D, N, Zero;
			Zero = 0;

			if (Right == Zero)
				return Temp;

			N = (Left > Zero) ? (Left) : (Left * (-1));
			D = (Right > Zero) ? (Right) : (Right * (-1));
			R.Sign = '+';

			while ((N.Length != 0) || (D.Length != 0))
			{
				if (N.Length == 0)
					N.Source.insert(0, 1, '0');
				else
					N.Length--;

				if (D.Length == 0)
					D.Source.insert(0, 1, '0');
				else
					D.Length--;
			}

			N.Unlead();
			D.Unlead();

			int32_t DivPrecision = (Left.Length > Right.Length) ? (Left.Length) : (Right.Length);
			for (int32_t i = 0; i < DivPrecision; i++)
				N.Source.insert(0, 1, '0');

			int Check = Decimal::CompareNum(N, D);
			if (Check == 0)
				Temp.Source.insert(0, 1, '1');

			if (Check == 2)
				return Zero;

			while (!N.Source.empty())
			{
				R.Source.insert(0, 1, *(N.Source.rbegin()));
				N.Source.pop_back();

				bool IsZero = true;
				auto ZeroIt = R.Source.begin();
				for (; ZeroIt != R.Source.end(); ++ZeroIt)
				{
					if (*ZeroIt != '0')
						IsZero = false;
				}

				if ((R >= D) && (!IsZero))
				{
					int32_t QSub = 0;
					int32_t Min = 0;
					int32_t Max = 9;

					while (R >= D)
					{
						int32_t Avg = Max - Min;
						int32_t ModAvg = Avg / 2;
						Avg = (Avg - ModAvg * 2) ? (ModAvg + 1) : (ModAvg);

						int DivCheck = Decimal::CompareNum(R, D * Avg);
						if (DivCheck != 2)
						{
							QSub = QSub + Avg;
							R = R - D * Avg;

							Max = 9;
						}
						else
							Max = Avg;
					}

					Q.Source.insert(0, 1, Decimal::IntToChar(QSub));

					bool IsZero = true;
					auto ZeroIt = R.Source.begin();
					for (; ZeroIt != R.Source.end(); ++ZeroIt)
					{
						if (*ZeroIt != '0')
							IsZero = false;
					}

					if (IsZero)
						R.Source.clear();
				}
				else
					Q.Source.insert(0, 1, '0');
			}

			Temp = Q;
			if (((Left.Sign == '-') && (Right.Sign == '-')) || ((Left.Sign == '+') && (Right.Sign == '+')))
				Temp.Sign = '+';
			else
				Temp.Sign = '-';
			Temp.Length = DivPrecision;
			Temp.Unlead();
			return Temp;
		}
		Decimal operator/(const Decimal& Left, const int& VRight)
		{
			Decimal Right;
			Right = VRight;
			return Left / Right;
		}
		Decimal operator/(const Decimal& Left, const double& VRight)
		{
			Decimal Right;
			Right = VRight;
			return Left / Right;
		}
		Decimal operator%(const Decimal& Left, const Decimal& Right)
		{
			Decimal Temp;
			if (Left.IsNaN() || Right.IsNaN())
				return Temp;

			if ((Left.Length != 0) || (Right.Length != 0))
				return Temp;

			Decimal Q, R, D, N, Zero, Result;
			Zero = 0;

			if (Right == Zero)
				return Temp;

			N = (Left > Zero) ? (Left) : (Left * (-1));
			D = (Right > Zero) ? (Right) : (Right * (-1));
			R.Sign = '+';

			int Check = Decimal::CompareNum(N, D);
			if (Check == 0)
				return Zero;

			if (Check == 2)
				return Left;

			while (!N.Source.empty())
			{
				R.Source.insert(0, 1, *(N.Source.rbegin()));
				N.Source.pop_back();

				bool IsZero = true;
				auto ZeroIt = R.Source.begin();
				for (; ZeroIt != R.Source.end(); ++ZeroIt)
				{
					if (*ZeroIt != '0')
						IsZero = false;
				}

				if ((R >= D) && (!IsZero))
				{
					int32_t QSub = 0;
					int32_t Min = 0;
					int32_t Max = 9;

					while (R >= D)
					{
						int32_t Avg = Max - Min;
						int32_t ModAvg = Avg / 2;
						Avg = (Avg - ModAvg * 2) ? (ModAvg + 1) : (ModAvg);

						int DivCheck = Decimal::CompareNum(R, D * Avg);
						if (DivCheck != 2)
						{
							QSub = QSub + Avg;
							R = R - D * Avg;

							Max = 9;
						}
						else
							Max = Avg;
					}

					Q.Source.insert(0, 1, Decimal::IntToChar(QSub));
					Result = R;

					bool IsZero = true;
					auto ZeroIt = R.Source.begin();
					for (; ZeroIt != R.Source.end(); ++ZeroIt)
					{
						if (*ZeroIt != '0')
							IsZero = false;
					}

					if (IsZero)
						R.Source.clear();
				}
				else
				{
					Result = R;
					Q.Source.insert(0, 1, '0');
				}
			}

			Q.Unlead();
			Result.Unlead();
			Temp = Result;
			if (((Left.Sign == '-') && (Right.Sign == '-')) || ((Left.Sign == '+') && (Right.Sign == '+')))
				Temp.Sign = '+';
			else
				Temp.Sign = '-';
			if (!Decimal::CompareNum(Temp, Zero))
				Temp.Sign = '+';
			return Temp;
		}
		Decimal operator%(const Decimal& Left, const int& VRight)
		{
			Decimal Right;
			Right = VRight;
			return Left % Right;
		}
		Decimal Decimal::Zero()
		{
			Decimal Result;
			Result.InitializeFromZero();
			return Result;
		}
		Decimal Decimal::NaN()
		{
			Decimal Result;
			return Result;
		}
		Decimal Decimal::Sum(const Decimal& Left, const Decimal& Right)
		{
			Decimal Temp;
			size_t LoopSize = (Left.Source.size() > Right.Source.size() ? Left.Source.size() : Right.Source.size());
			int32_t Carry = 0;

			for (size_t i = 0; i < LoopSize; ++i)
			{
				int32_t Val1, Val2;
				Val1 = (i > Left.Source.size() - 1) ? 0 : CharToInt(Left.Source[i]);
				Val2 = (i > Right.Source.size() - 1) ? 0 : CharToInt(Right.Source[i]);

				int32_t Aus = Val1 + Val2 + Carry;
				Carry = 0;

				if (Aus > 9)
				{
					Carry = 1;
					Aus = Aus - 10;
				}

				Temp.Source.push_back(IntToChar(Aus));
			}

			if (Carry != 0)
				Temp.Source.push_back(IntToChar(Carry));

			return Temp;
		}
		Decimal Decimal::Subtract(const Decimal& Left, const Decimal& Right)
		{
			Decimal Temp;
			int32_t Carry = 0;
			int32_t Aus;

			for (size_t i = 0; i < Left.Source.size(); ++i)
			{
				int32_t Val1, Val2;
				Val1 = CharToInt(Left.Source[i]);
				Val2 = (i > Right.Source.size() - 1) ? 0 : CharToInt(Right.Source[i]);
				Val1 -= Carry;

				if (Val1 < Val2)
				{
					Aus = 10 + Val1 - Val2;
					Carry = 1;
				}
				else
				{
					Aus = Val1 - Val2;
					Carry = 0;
				}

				Temp.Source.push_back(IntToChar(Aus));
			}

			return Temp;
		}
		Decimal Decimal::Multiply(const Decimal& Left, const Decimal& Right)
		{
			Decimal Result;
			Decimal Temp;
			Result.Source.push_back('0');
			int32_t Carry = 0;

			for (size_t i = 0; i < Right.Source.size(); ++i)
			{
				for (size_t k = 0; k < i; ++k)
					Temp.Source.insert(0, 1, '0');

				for (size_t j = 0; j < Left.Source.size(); ++j)
				{
					int32_t Aus = CharToInt(Right.Source[i]) * CharToInt(Left.Source[j]) + Carry;
					Carry = 0;
					if (Aus > 9)
					{
						while (Aus > 9)
						{
							Carry++;
							Aus -= 10;
						}
					}

					Temp.Source.push_back(IntToChar(Aus));
				}

				if (Carry != 0)
					Temp.Source.push_back(IntToChar(Carry));

				Carry = 0;
				Result = Sum(Result, Temp);
				Temp.Source.clear();
			}

			return Result;
		}
		int Decimal::CompareNum(const Decimal& Left, const Decimal& Right)
		{
			if ((Left.Source.size() - Left.Length) > (Right.Source.size() - Right.Length))
				return 1;

			if ((Left.Source.size() - Left.Length) < (Right.Source.size() - Right.Length))
				return 2;

			if (Left.Length > Right.Length)
			{
				Decimal Temp;
				Temp = Right;
				while (Left.Length > Temp.Length)
				{
					Temp.Length++;
					Temp.Source.insert(0, 1, '0');
				}

				for (int32_t i = (int32_t)Left.Source.size() - 1; i >= 0; i--)
				{
					if (Left.Source[i] > Temp.Source[i])
						return 1;

					if (Left.Source[i] < Temp.Source[i])
						return 2;
				}

				return 0;
			}
			else if (Left.Length < Right.Length)
			{
				Decimal Temp;
				Temp = Left;
				while (Temp.Length < Right.Length)
				{
					Temp.Length++;
					Temp.Source.insert(0, 1, '0');
				}

				for (int32_t i = (int32_t)Temp.Source.size() - 1; i >= 0; i--)
				{
					if (Temp.Source[i] > Right.Source[i])
						return 1;

					if (Temp.Source[i] < Right.Source[i])
						return 2;
				}

				return 0;
			}
			else
			{
				for (int32_t i = (int32_t)Left.Source.size() - 1; i >= 0; i--)
				{
					if (Left.Source[i] > Right.Source[i])
						return 1;
					else if (Left.Source[i] < Right.Source[i])
						return 2;
				}

				return 0;
			}
		}
		int Decimal::CharToInt(const char& Value)
		{
			return Value - '0';
		}
		char Decimal::IntToChar(const int& Value)
		{
			return Value + '0';
		}

		Variant::Variant() noexcept : Type(VarType::Undefined), Length(0)
		{
			Value.Pointer = nullptr;
		}
		Variant::Variant(VarType NewType) noexcept : Type(NewType), Length(0)
		{
			Value.Pointer = nullptr;
		}
		Variant::Variant(const Variant& Other) noexcept
		{
			Copy(Other);
		}
		Variant::Variant(Variant&& Other) noexcept
		{
			Move(std::move(Other));
		}
		Variant::~Variant() noexcept
		{
			Free();
		}
		bool Variant::Deserialize(const String& Text, bool Strict)
		{
			Free();
			if (!Strict)
			{
				if (Text == PREFIX_ENUM "null" PREFIX_ENUM)
				{
					Type = VarType::Null;
					return true;
				}

				if (Text == PREFIX_ENUM "undefined" PREFIX_ENUM)
				{
					Type = VarType::Undefined;
					return true;
				}

				if (Text == PREFIX_ENUM "{}" PREFIX_ENUM)
				{
					Type = VarType::Object;
					return true;
				}

				if (Text == PREFIX_ENUM "[]" PREFIX_ENUM)
				{
					Type = VarType::Array;
					return true;
				}

				if (Text == PREFIX_ENUM "void*" PREFIX_ENUM)
				{
					Type = VarType::Pointer;
					return true;
				}

				if (Text == "true")
				{
					Move(Var::Boolean(true));
					return true;
				}

				if (Text == "false")
				{
					Move(Var::Boolean(false));
					return true;
				}

				if (Stringify::HasNumber(Text) && (Text.size() == 1 || Text.front() != '0'))
				{
					if (Stringify::HasDecimal(Text))
					{
						Move(Var::DecimalString(Text));
						return true;
					}
					else if (Stringify::HasInteger(Text))
					{
						auto Number = FromString<int64_t>(Text);
						if (Number)
						{
							Move(Var::Integer(*Number));
							return true;
						}
					}
					else
					{
						auto Number = FromString<double>(Text);
						if (Number)
						{
							Move(Var::Number(*Number));
							return true;
						}
					}
				}
			}

			if (Text.size() > 2 && Text.front() == PREFIX_BINARY[0] && Text.back() == PREFIX_BINARY[0])
			{
				Move(Var::Binary(Compute::Codec::Bep45Decode(String(Text.substr(1).c_str(), Text.size() - 2))));
				return true;
			}

			Move(Var::String(Text));
			return true;
		}
		String Variant::Serialize() const
		{
			switch (Type)
			{
				case VarType::Null:
					return PREFIX_ENUM "null" PREFIX_ENUM;
				case VarType::Undefined:
					return PREFIX_ENUM "undefined" PREFIX_ENUM;
				case VarType::Object:
					return PREFIX_ENUM "{}" PREFIX_ENUM;
				case VarType::Array:
					return PREFIX_ENUM "[]" PREFIX_ENUM;
				case VarType::Pointer:
					return PREFIX_ENUM "void*" PREFIX_ENUM;
				case VarType::String:
					return String(GetString(), Size());
				case VarType::Binary:
					return PREFIX_BINARY + Compute::Codec::Bep45Encode(String(GetString(), Size())) + PREFIX_BINARY;
				case VarType::Decimal:
				{
					auto* Data = ((Decimal*)Value.Pointer);
					if (Data->IsNaN())
						return PREFIX_ENUM "null" PREFIX_ENUM;

					return Data->ToString();
				}
				case VarType::Integer:
					return Core::ToString(Value.Integer);
				case VarType::Number:
					return Core::ToString(Value.Number);
				case VarType::Boolean:
					return Value.Boolean ? "true" : "false";
				default:
					return "";
			}
		}
		String Variant::GetBlob() const
		{
			if (Type == VarType::String || Type == VarType::Binary)
				return String(GetString(), Length);

			if (Type == VarType::Decimal)
				return ((Decimal*)Value.Pointer)->ToString();

			if (Type == VarType::Integer)
				return Core::ToString(GetInteger());

			if (Type == VarType::Number)
				return Core::ToString(GetNumber());

			if (Type == VarType::Boolean)
				return Value.Boolean ? "1" : "0";

			return "";
		}
		Decimal Variant::GetDecimal() const
		{
			if (Type == VarType::Decimal)
				return *(Decimal*)Value.Pointer;

			if (Type == VarType::Integer)
				return Decimal(Core::ToString(Value.Integer));

			if (Type == VarType::Number)
				return Decimal(Core::ToString(Value.Number));

			if (Type == VarType::Boolean)
				return Decimal(Value.Boolean ? "1" : "0");

			if (Type == VarType::String)
				return Decimal(GetString());

			return Decimal::NaN();
		}
		void* Variant::GetPointer() const
		{
			if (Type == VarType::Pointer)
				return (void*)Value.Pointer;

			return nullptr;
		}
		const char* Variant::GetString() const
		{
			if (Type != VarType::String && Type != VarType::Binary)
				return nullptr;

			return Length <= GetMaxSmallStringSize() ? Value.String : Value.Pointer;
		}
		unsigned char* Variant::GetBinary() const
		{
			return (unsigned char*)GetString();
		}
		int64_t Variant::GetInteger() const
		{
			if (Type == VarType::Integer)
				return Value.Integer;

			if (Type == VarType::Number)
				return (int64_t)Value.Number;

			if (Type == VarType::Decimal)
				return (int64_t)((Decimal*)Value.Pointer)->ToDouble();

			if (Type == VarType::Boolean)
				return Value.Boolean ? 1 : 0;

			if (Type == VarType::String)
			{
				auto Result = FromString<int64_t>(String(GetString(), Size()));
				if (Result)
					return *Result;
			}

			return 0;
		}
		double Variant::GetNumber() const
		{
			if (Type == VarType::Number)
				return Value.Number;

			if (Type == VarType::Integer)
				return (double)Value.Integer;

			if (Type == VarType::Decimal)
				return ((Decimal*)Value.Pointer)->ToDouble();

			if (Type == VarType::Boolean)
				return Value.Boolean ? 1.0 : 0.0;

			if (Type == VarType::String)
			{
				auto Result = FromString<double>(String(GetString(), Size()));
				if (Result)
					return *Result;
			}

			return 0.0;
		}
		bool Variant::GetBoolean() const
		{
			if (Type == VarType::Boolean)
				return Value.Boolean;

			if (Type == VarType::Number)
				return Value.Number > 0.0;

			if (Type == VarType::Integer)
				return Value.Integer > 0;

			if (Type == VarType::Decimal)
				return ((Decimal*)Value.Pointer)->ToDouble() > 0.0;

			return Size() > 0;
		}
		VarType Variant::GetType() const
		{
			return Type;
		}
		size_t Variant::Size() const
		{
			switch (Type)
			{
				case VarType::Null:
				case VarType::Undefined:
				case VarType::Object:
				case VarType::Array:
					return 0;
				case VarType::Pointer:
					return sizeof(void*);
				case VarType::String:
				case VarType::Binary:
					return Length;
				case VarType::Decimal:
					return ((Decimal*)Value.Pointer)->Size();
				case VarType::Integer:
					return sizeof(int64_t);
				case VarType::Number:
					return sizeof(double);
				case VarType::Boolean:
					return sizeof(bool);
			}

			return 0;
		}
		bool Variant::operator== (const Variant& Other) const
		{
			return Same(Other);
		}
		bool Variant::operator!= (const Variant& Other) const
		{
			return !Same(Other);
		}
		Variant& Variant::operator= (const Variant& Other) noexcept
		{
			Free();
			Copy(Other);

			return *this;
		}
		Variant& Variant::operator= (Variant&& Other) noexcept
		{
			Free();
			Move(std::move(Other));

			return *this;
		}
		Variant::operator bool() const
		{
			return !Empty();
		}
		bool Variant::IsString(const char* Text) const
		{
			VI_ASSERT(Text != nullptr, "text should be set");
			const char* Other = GetString();
			if (Other == Text)
				return true;

			return strcmp(Other, Text) == 0;
		}
		bool Variant::IsObject() const
		{
			return Type == VarType::Object || Type == VarType::Array;
		}
		bool Variant::Empty() const
		{
			switch (Type)
			{
				case VarType::Null:
				case VarType::Undefined:
					return true;
				case VarType::Object:
				case VarType::Array:
					return false;
				case VarType::Pointer:
					return Value.Pointer == nullptr;
				case VarType::String:
				case VarType::Binary:
					return Length == 0;
				case VarType::Decimal:
					return ((Decimal*)Value.Pointer)->ToDouble() == 0.0;
				case VarType::Integer:
					return Value.Integer == 0;
				case VarType::Number:
					return Value.Number == 0.0;
				case VarType::Boolean:
					return Value.Boolean == false;
				default:
					return true;
			}
		}
		bool Variant::Is(VarType Value) const
		{
			return Type == Value;
		}
		bool Variant::Same(const Variant& Other) const
		{
			if (Type != Other.Type)
				return false;

			switch (Type)
			{
				case VarType::Null:
				case VarType::Undefined:
					return true;
				case VarType::Pointer:
					return GetPointer() == Other.GetPointer();
				case VarType::String:
				case VarType::Binary:
				{
					size_t Sizing = Size();
					if (Sizing != Other.Size())
						return false;

					const char* Src1 = GetString();
					const char* Src2 = Other.GetString();
					if (!Src1 || !Src2)
						return false;

					return strncmp(Src1, Src2, sizeof(char) * Sizing) == 0;
				}
				case VarType::Decimal:
					return (*(Decimal*)Value.Pointer) == (*(Decimal*)Other.Value.Pointer);
				case VarType::Integer:
					return GetInteger() == Other.GetInteger();
				case VarType::Number:
					return abs(GetNumber() - Other.GetNumber()) < std::numeric_limits<double>::epsilon();
				case VarType::Boolean:
					return GetBoolean() == Other.GetBoolean();
				default:
					return false;
			}
		}
		void Variant::Copy(const Variant& Other)
		{
			Type = Other.Type;
			Length = Other.Length;

			switch (Type)
			{
				case VarType::Null:
				case VarType::Undefined:
				case VarType::Object:
				case VarType::Array:
					Value.Pointer = nullptr;
					break;
				case VarType::Pointer:
					Value.Pointer = Other.Value.Pointer;
					break;
				case VarType::String:
				case VarType::Binary:
				{
					size_t StringSize = sizeof(char) * (Length + 1);
					if (Length > GetMaxSmallStringSize())
						Value.Pointer = VI_MALLOC(char, StringSize);
					memcpy((void*)GetString(), Other.GetString(), StringSize);
					break;
				}
				case VarType::Decimal:
				{
					Decimal* From = (Decimal*)Other.Value.Pointer;
					Value.Pointer = (char*)VI_NEW(Decimal, *From);
					break;
				}
				case VarType::Integer:
					Value.Integer = Other.Value.Integer;
					break;
				case VarType::Number:
					Value.Number = Other.Value.Number;
					break;
				case VarType::Boolean:
					Value.Boolean = Other.Value.Boolean;
					break;
				default:
					Value.Pointer = nullptr;
					break;
			}
		}
		void Variant::Move(Variant&& Other)
		{
			Type = Other.Type;
			Length = Other.Length;

			switch (Type)
			{
				case VarType::Null:
				case VarType::Undefined:
				case VarType::Object:
				case VarType::Array:
				case VarType::Pointer:
				case VarType::Decimal:
					Value.Pointer = Other.Value.Pointer;
					Other.Value.Pointer = nullptr;
					break;
				case VarType::String:
				case VarType::Binary:
					if (Length <= GetMaxSmallStringSize())
						memcpy((void*)GetString(), Other.GetString(), sizeof(char) * (Length + 1));
					else
						Value.Pointer = Other.Value.Pointer;
					Other.Value.Pointer = nullptr;
					break;
				case VarType::Integer:
					Value.Integer = Other.Value.Integer;
					break;
				case VarType::Number:
					Value.Number = Other.Value.Number;
					break;
				case VarType::Boolean:
					Value.Boolean = Other.Value.Boolean;
					break;
				default:
					break;
			}

			Other.Type = VarType::Undefined;
			Other.Length = 0;
		}
		void Variant::Free()
		{
			switch (Type)
			{
				case VarType::Pointer:
					Value.Pointer = nullptr;
					break;
				case VarType::String:
				case VarType::Binary:
				{
					if (!Value.Pointer || Length <= GetMaxSmallStringSize())
						break;

					VI_FREE(Value.Pointer);
					Value.Pointer = nullptr;
					break;
				}
				case VarType::Decimal:
				{
					if (!Value.Pointer)
						break;

					Decimal* Buffer = (Decimal*)Value.Pointer;
					VI_DELETE(Decimal, Buffer);
					Value.Pointer = nullptr;
					break;
				}
				default:
					break;
			}
		}
		size_t Variant::GetMaxSmallStringSize()
		{
			return sizeof(Tag::String) - 1;
		}

		Timeout::Timeout(const TaskCallback& NewCallback, const std::chrono::microseconds& NewTimeout, TaskId NewId, bool NewAlive) noexcept : Expires(NewTimeout), Callback(NewCallback), Id(NewId), Alive(NewAlive)
		{
		}
		Timeout::Timeout(TaskCallback&& NewCallback, const std::chrono::microseconds& NewTimeout, TaskId NewId, bool NewAlive) noexcept : Expires(NewTimeout), Callback(std::move(NewCallback)), Id(NewId), Alive(NewAlive)
		{
		}
		Timeout::Timeout(const Timeout& Other) noexcept : Expires(Other.Expires), Callback(Other.Callback), Id(Other.Id), Alive(Other.Alive)
		{
		}
		Timeout::Timeout(Timeout&& Other) noexcept : Expires(Other.Expires), Callback(std::move(Other.Callback)), Id(Other.Id), Alive(Other.Alive)
		{
		}
		Timeout& Timeout::operator= (const Timeout& Other) noexcept
		{
			Callback = Other.Callback;
			Expires = Other.Expires;
			Id = Other.Id;
			Alive = Other.Alive;
			return *this;
		}
		Timeout& Timeout::operator= (Timeout&& Other) noexcept
		{
			Callback = std::move(Other.Callback);
			Expires = Other.Expires;
			Id = Other.Id;
			Alive = Other.Alive;
			return *this;
		}

		DateTime::DateTime() noexcept : Time(std::chrono::system_clock::now().time_since_epoch()), DateRebuild(false)
		{
#ifdef VI_MICROSOFT
			RtlSecureZeroMemory(&DateValue, sizeof(DateValue));
#else
			memset(&DateValue, 0, sizeof(DateValue));
#endif
			time_t Now = (time_t)Seconds();
			LocalTime(&Now, &DateValue);
			DateRebuild = true;
		}
		DateTime::DateTime(uint64_t Seconds) noexcept : Time(std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(Seconds))), DateRebuild(false)
		{
#ifdef VI_MICROSOFT
			RtlSecureZeroMemory(&DateValue, sizeof(DateValue));
#else
			memset(&DateValue, 0, sizeof(DateValue));
#endif
			time_t Now = Seconds;
			LocalTime(&Now, &DateValue);
			DateRebuild = true;
		}
		DateTime::DateTime(const DateTime& Value) noexcept : Time(Value.Time), DateRebuild(Value.DateRebuild)
		{
			memcpy(&DateValue, &Value.DateValue, sizeof(DateValue));
		}
		void DateTime::Rebuild()
		{
			if (!DateRebuild)
				return;

			Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(mktime(&DateValue)));
			DateRebuild = false;
		}
		DateTime& DateTime::operator= (const DateTime& Other) noexcept
		{
			Time = Other.Time;
			DateRebuild = false;
#ifdef VI_MICROSOFT
			RtlSecureZeroMemory(&DateValue, sizeof(DateValue));
#else
			memset(&DateValue, 0, sizeof(DateValue));
#endif
			return *this;
		}
		void DateTime::operator +=(const DateTime& Right)
		{
			Time += Right.Time;
		}
		void DateTime::operator -=(const DateTime& Right)
		{
			Time -= Right.Time;
		}
		bool DateTime::operator >=(const DateTime& Right)
		{
			return Time >= Right.Time;
		}
		bool DateTime::operator <=(const DateTime& Right)
		{
			return Time <= Right.Time;
		}
		bool DateTime::operator >(const DateTime& Right)
		{
			return Time > Right.Time;
		}
		bool DateTime::operator <(const DateTime& Right)
		{
			return Time < Right.Time;
		}
		bool DateTime::operator ==(const DateTime& Right)
		{
			return Time == Right.Time;
		}
		String DateTime::Format(const String& Value)
		{
			if (DateRebuild)
				Rebuild();

			char Buffer[CHUNK_SIZE];
			strftime(Buffer, sizeof(Buffer), Value.c_str(), &DateValue);
			return Buffer;
		}
		String DateTime::Date(const String& Value)
		{
			auto Offset = std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(Time));
			if (DateRebuild)
				Rebuild();

			struct tm T;
			if (!LocalTime(&Offset, &T))
				return Value;

			T.tm_mon++;
			T.tm_year += 1900;

			String Result = Value;
			Stringify::Replace(Result, "{s}", T.tm_sec < 10 ? Stringify::Text("0%i", T.tm_sec) : Core::ToString(T.tm_sec));
			Stringify::Replace(Result, "{m}", T.tm_min < 10 ? Stringify::Text("0%i", T.tm_min) : Core::ToString(T.tm_min));
			Stringify::Replace(Result, "{h}", Core::ToString(T.tm_hour));
			Stringify::Replace(Result, "{D}", Core::ToString(T.tm_yday));
			Stringify::Replace(Result, "{MD}", T.tm_mday < 10 ? Stringify::Text("0%i", T.tm_mday) : Core::ToString(T.tm_mday));
			Stringify::Replace(Result, "{WD}", Core::ToString(T.tm_wday + 1));
			Stringify::Replace(Result, "{M}", T.tm_mon < 10 ? Stringify::Text("0%i", T.tm_mon) : Core::ToString(T.tm_mon));
			Stringify::Replace(Result, "{Y}", Core::ToString(T.tm_year));
			return Result;
		}
		String DateTime::Iso8601()
		{
			if (DateRebuild)
				Rebuild();

			char Buffer[64];
			strftime(Buffer, sizeof(Buffer), "%FT%TZ", &DateValue);
			return Buffer;
		}
		DateTime DateTime::Now()
		{
			DateTime New;
			New.Time = std::chrono::system_clock::now().time_since_epoch();

			return New;
		}
		DateTime DateTime::FromNanoseconds(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::nanoseconds(Value));

			return New;
		}
		DateTime DateTime::FromMicroseconds(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::microseconds(Value));

			return New;
		}
		DateTime DateTime::FromMilliseconds(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(Value));

			return New;
		}
		DateTime DateTime::FromSeconds(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(Value));

			return New;
		}
		DateTime DateTime::FromMinutes(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::minutes(Value));

			return New;
		}
		DateTime DateTime::FromHours(uint64_t Value)
		{
			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::hours(Value));

			return New;
		}
		DateTime DateTime::FromDays(uint64_t Value)
		{
			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(_Days(Value));

			return New;
		}
		DateTime DateTime::FromWeeks(uint64_t Value)
		{
			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;
			using _Weeks = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<7>, _Days::period>>;

			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(_Weeks(Value));

			return New;
		}
		DateTime DateTime::FromMonths(uint64_t Value)
		{
			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;
			using _Years = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<146097, 400>, _Days::period>>;
			using _Months = std::chrono::duration<uint64_t, std::ratio_divide<_Years::period, std::ratio<12>>>;

			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(_Months(Value));

			return New;
		}
		DateTime DateTime::FromYears(uint64_t Value)
		{
			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;
			using _Years = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<146097, 400>, _Days::period>>;

			DateTime New;
			New.Time = std::chrono::duration_cast<std::chrono::system_clock::duration>(_Years(Value));

			return New;
		}
		DateTime DateTime::operator +(const DateTime& Right) const
		{
			DateTime New;
			New.Time = Time + Right.Time;

			return New;
		}
		DateTime DateTime::operator -(const DateTime& Right) const
		{
			DateTime New;
			New.Time = Time - Right.Time;

			return New;
		}
		DateTime& DateTime::SetDateSeconds(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value > 60)
				Value = 60;

			DateValue.tm_sec = (int)Value;
			return *this;
		}
		DateTime& DateTime::SetDateMinutes(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value > 60)
				Value = 60;
			else if (Value < 1)
				Value = 1;

			DateValue.tm_min = (int)Value - 1;
			return *this;
		}
		DateTime& DateTime::SetDateHours(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value > 24)
				Value = 24;
			else if (Value < 1)
				Value = 1;

			DateValue.tm_hour = (int)Value - 1;
			return *this;
		}
		DateTime& DateTime::SetDateDay(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			uint64_t Month = DateMonth(), Days = 31;
			if (Month == 1 || Month == 3 || Month == 5 || Month == 7 || Month == 8 || Month == 10 || Month == 12)
				Days = 31;
			else if (Month != 2)
				Days = 30;
			else
				Days = 28;

			if (Value > Days)
				Value = Days;
			else if (Value < 1)
				Value = 1;

			if (DateValue.tm_mday > (int)Value)
				DateValue.tm_yday = DateValue.tm_yday - DateValue.tm_mday + (int)Value;
			else
				DateValue.tm_yday = DateValue.tm_yday - (int)Value + DateValue.tm_mday;

			if (Value <= 7)
				DateValue.tm_wday = (int)Value - 1;
			else if (Value <= 14)
				DateValue.tm_wday = (int)Value - 8;
			else if (Value <= 21)
				DateValue.tm_wday = (int)Value - 15;
			else
				DateValue.tm_wday = (int)Value - 22;

			DateValue.tm_mday = (int)Value;
			return *this;
		}
		DateTime& DateTime::SetDateWeek(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value > 7)
				Value = 7;
			else if (Value < 1)
				Value = 1;

			DateValue.tm_wday = (int)Value - 1;
			return *this;
		}
		DateTime& DateTime::SetDateMonth(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value < 1)
				Value = 1;
			else if (Value > 12)
				Value = 12;

			DateValue.tm_mon = (int)Value - 1;
			return *this;
		}
		DateTime& DateTime::SetDateYear(uint64_t Value, bool NoFlush)
		{
			if (!DateRebuild)
			{
				if (!NoFlush)
				{
					time_t TimeNow;
					time(&TimeNow);
					LocalTime(&TimeNow, &DateValue);
				}
				DateRebuild = true;
			}

			if (Value < 1900)
				Value = 1900;

			DateValue.tm_year = (int)Value - 1900;
			return *this;
		}
		uint64_t DateTime::DateSecond()
		{
			if (DateRebuild)
				Rebuild();

			return DateValue.tm_sec;
		}
		uint64_t DateTime::DateMinute()
		{
			if (DateRebuild)
				Rebuild();

			return DateValue.tm_min;
		}
		uint64_t DateTime::DateHour()
		{
			if (DateRebuild)
				Rebuild();

			return DateValue.tm_hour;
		}
		uint64_t DateTime::DateDay()
		{
			if (DateRebuild)
				Rebuild();

			return DateValue.tm_mday;
		}
		uint64_t DateTime::DateWeek()
		{
			if (DateRebuild)
				Rebuild();

			return DateValue.tm_wday + 1;
		}
		uint64_t DateTime::DateMonth()
		{
			if (DateRebuild)
				Rebuild();

			return DateValue.tm_mon + 1;
		}
		uint64_t DateTime::DateYear()
		{
			if (DateRebuild)
				Rebuild();

			return DateValue.tm_year + 1900;
		}
		uint64_t DateTime::Nanoseconds()
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(Time).count();
		}
		uint64_t DateTime::Microseconds()
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(Time).count();
		}
		uint64_t DateTime::Milliseconds()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(Time).count();
		}
		uint64_t DateTime::Seconds()
		{
			if (DateRebuild)
				Rebuild();

			return std::chrono::duration_cast<std::chrono::seconds>(Time).count();
		}
		uint64_t DateTime::Minutes()
		{
			if (DateRebuild)
				Rebuild();

			return (uint64_t)std::chrono::duration_cast<std::chrono::minutes>(Time).count();
		}
		uint64_t DateTime::Hours()
		{
			if (DateRebuild)
				Rebuild();

			return (uint64_t)std::chrono::duration_cast<std::chrono::hours>(Time).count();
		}
		uint64_t DateTime::Days()
		{
			if (DateRebuild)
				Rebuild();

			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			return std::chrono::duration_cast<_Days>(Time).count();
		}
		uint64_t DateTime::Weeks()
		{
			if (DateRebuild)
				Rebuild();

			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			using _Weeks = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<7>, _Days::period>>;

			return std::chrono::duration_cast<_Weeks>(Time).count();
		}
		uint64_t DateTime::Months()
		{
			if (DateRebuild)
				Rebuild();

			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			using _Years = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<146097, 400>, _Days::period>>;

			using _Months = std::chrono::duration<uint64_t, std::ratio_divide<_Years::period, std::ratio<12>>>;

			return std::chrono::duration_cast<_Months>(Time).count();
		}
		uint64_t DateTime::Years()
		{
			if (DateRebuild)
				Rebuild();

			using _Days = std::chrono::duration<uint64_t, std::ratio_multiply<std::ratio<24>, std::chrono::hours::period>>;

			using _Years = std::chrono::duration<int, std::ratio_multiply<std::ratio<146097, 400>, _Days::period>>;

			return std::chrono::duration_cast<_Years>(Time).count();
		}
		String DateTime::FetchWebDateGMT(int64_t TimeStamp)
		{
			auto Time = (time_t)TimeStamp;
			struct tm Date { };
#ifdef VI_MICROSOFT
			if (gmtime_s(&Date, &Time) != 0)
#elif defined(VI_LINUX)
			if (gmtime_r(&Time, &Date) == nullptr)
#endif
				return "Thu, 01 Jan 1970 00:00:00 GMT";

			char Buffer[64];
			strftime(Buffer, sizeof(Buffer), "%a, %d %b %Y %H:%M:%S GMT", &Date);
			return Buffer;
		}
		String DateTime::FetchWebDateTime(int64_t TimeStamp)
		{
			auto Time = (time_t)TimeStamp;
			struct tm Date { };
			if (!LocalTime(&Time, &Date))
				return "Thu, 01 Jan 1970 00:00:00";

			char Buffer[64];
			strftime(Buffer, sizeof(Buffer), "%a, %d %b %Y %H:%M:%S GMT", &Date);
			return Buffer;
		}
		bool DateTime::FetchWebDateGMT(char* Buffer, size_t Length, int64_t Time)
		{
			if (!Buffer || !Length)
				return false;

			auto TimeStamp = (time_t)Time;
			struct tm Date { };
#if defined(_WIN32_CE)
			static const int DaysPerMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

			FILETIME FileTime;
			*(int64_t)&FileTime = ((int64_t)*clk) * RATE_DIFF * EPOCH_DIFF;

			SYSTEMTIME SystemTime;
			FileTimeToSystemTime(&FileTime, &SystemTime);

			Date.tm_year = SystemTime.wYear - 1900;
			Date.tm_mon = SystemTime.wMonth - 1;
			Date.tm_wday = SystemTime.wDayOfWeek;
			Date.tm_mday = SystemTime.wDay;
			Date.tm_hour = SystemTime.wHour;
			Date.tm_min = SystemTime.wMinute;
			Date.tm_sec = SystemTime.wSecond;
			Date.tm_isdst = false;

			int Day = Date.tm_mday;
			for (int i = 0; i < Date.tm_mon; i++)
				Day += DaysPerMonth[i];

			if (Date.tm_mon >= 2 && LEAP_YEAR(Date.tm_year + 1900))
				Day++;

			Date.tm_yday = Day;
			strftime(Buffer, Length, "%a, %d %b %Y %H:%M:%S GMT", &Date);
#elif defined(VI_MICROSOFT)
			if (gmtime_s(&Date, &TimeStamp) != 0)
				strncpy(Buffer, "Thu, 01 Jan 1970 00:00:00 GMT", Length);
			else
				strftime(Buffer, Length, "%a, %d %b %Y %H:%M:%S GMT", &Date);
#else
			if (gmtime_r(&TimeStamp, &Date) == nullptr)
				strncpy(Buffer, "Thu, 01 Jan 1970 00:00:00 GMT", Length);
			else
				strftime(Buffer, Length, "%a, %d %b %Y %H:%M:%S GMT", &Date);
#endif
			return true;
		}
		bool DateTime::FetchWebDateTime(char* Buffer, size_t Length, int64_t Time)
		{
			time_t TimeStamp = (time_t)Time;
			struct tm Date { };
#if defined(_WIN32_WCE)
			static const int DaysPerMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

			FILETIME FileTime, LocalFileTime;
			*(int64_t)&FileTime = ((int64_t)*clk) * RATE_DIFF * EPOCH_DIFF;
			FileTimeToLocalFileTime(&FileTime, &LocalFileTime);

			SYSTEMTIME SystemTime;
			FileTimeToSystemTime(&LocalFileTime, &SystemTime);

			TIME_ZONE_INFORMATION TimeZone;
			Date.tm_year = st.wYear - 1900;
			Date.tm_mon = st.wMonth - 1;
			Date.tm_wday = st.wDayOfWeek;
			Date.tm_mday = st.wDay;
			Date.tm_hour = st.wHour;
			Date.tm_min = st.wMinute;
			Date.tm_sec = st.wSecond;
			Date.tm_isdst = (GetTimeZoneInformation(&TimeZone) == TIME_ZONE_ID_DAYLIGHT) ? 1 : 0;

			int Day = Date.tm_mday;
			for (int i = 0; i < Date.tm_mon; i++)
				Day += DaysPerMonth[i];

			if (Date.tm_mon >= 2 && LEAP_YEAR(Date.tm_year + 1900))
				Day++;

			Date.tm_yday = doy;
			strftime(Buffer, Length, "%d-%b-%Y %H:%M:%S", &Date);
#elif defined(_WIN32)
			if (!LocalTime(&TimeStamp, &Date))
				strncpy(Buffer, "01-Jan-1970 00:00:00", Length);
			else
				strftime(Buffer, Length, "%d-%b-%Y %H:%M:%S", &Date);
#else
			if (!LocalTime(&TimeStamp, &Date))
				strncpy(Buffer, "01-Jan-1970 00:00:00", Length);
			else
				strftime(Buffer, Length, "%d-%b-%Y %H:%M:%S", &Date);
#endif
			return true;
		}
		void DateTime::FetchDateTime(char* Date, size_t Size, int64_t TargetTime)
		{
			tm Data{ }; time_t Time = (time_t)TargetTime;
			if (!LocalTime(&Time, &Data))
				strncpy(Date, "1970-01-01 00:00:00", Size);
			else
				strftime(Date, Size, "%Y-%m-%d %H:%M:%S", &Data);
		}
		int64_t DateTime::ParseWebDate(const char* Date)
		{
			static const char* MonthNames[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

			char Name[32] = { };
			int Second, Minute, Hour, Day, Year;
			if (sscanf(Date, "%d/%3s/%d %d:%d:%d", &Day, Name, &Year, &Hour, &Minute, &Second) != 6 && sscanf(Date, "%d %3s %d %d:%d:%d", &Day, Name, &Year, &Hour, &Minute, &Second) != 6 && sscanf(Date, "%*3s, %d %3s %d %d:%d:%d", &Day, Name, &Year, &Hour, &Minute, &Second) != 6 && sscanf(Date, "%d-%3s-%d %d:%d:%d", &Day, Name, &Year, &Hour, &Minute, &Second) != 6)
				return 0;

			if (Year <= 1970)
				return 0;

			for (uint64_t i = 0; i < 12; i++)
			{
				if (strcmp(Name, MonthNames[i]) != 0)
					continue;

				struct tm Time { };
				Time.tm_year = Year - 1900;
				Time.tm_mon = (int)i;
				Time.tm_mday = Day;
				Time.tm_hour = Hour;
				Time.tm_min = Minute;
				Time.tm_sec = Second;

#ifdef VI_MICROSOFT
				return _mkgmtime(&Time);
#else
				return mktime(&Time);
#endif
			}

			return 0;
		}

		String& Stringify::EscapePrint(String& Other)
		{
			for (size_t i = 0; i < Other.size(); i++)
			{
				if (Other.at(i) != '%')
					continue;

				if (i + 1 < Other.size())
				{
					if (Other.at(i + 1) != '%')
					{
						Other.insert(Other.begin() + i, '%');
						i++;
					}
				}
				else
				{
					Other.append(1, '%');
					i++;
				}
			}
			return Other;
		}
		String& Stringify::Escape(String& Other)
		{
			for (size_t i = 0; i < Other.size(); i++)
			{
				char& V = Other.at(i);
				if (V == '\"')
				{
					if (i > 0 && Other.at(i - 1) == '\\')
						continue;
				}
				else if (V == '\n')
					V = 'n';
				else if (V == '\t')
					V = 't';
				else if (V == '\v')
					V = 'v';
				else if (V == '\b')
					V = 'b';
				else if (V == '\r')
					V = 'r';
				else if (V == '\f')
					V = 'f';
				else if (V == '\a')
					V = 'a';
				else
					continue;

				Other.insert(Other.begin() + i, '\\');
				i++;
			}
			return Other;
		}
		String& Stringify::Unescape(String& Other)
		{
			for (size_t i = 0; i < Other.size(); i++)
			{
				if (Other.at(i) != '\\' || i + 1 >= Other.size())
					continue;

				char& V = Other.at(i + 1);
				if (V == 'n')
					V = '\n';
				else if (V == 't')
					V = '\t';
				else if (V == 'v')
					V = '\v';
				else if (V == 'b')
					V = '\b';
				else if (V == 'r')
					V = '\r';
				else if (V == 'f')
					V = '\f';
				else if (V == 'a')
					V = '\a';
				else
					continue;

				Other.erase(Other.begin() + i);
			}
			return Other;
		}
		String& Stringify::ToUpper(String& Other)
		{
			std::transform(Other.begin(), Other.end(), Other.begin(), ::toupper);
			return Other;
		}
		String& Stringify::ToLower(String& Other)
		{
			std::transform(Other.begin(), Other.end(), Other.begin(), ::tolower);
			return Other;
		}
		String& Stringify::Clip(String& Other, size_t Length)
		{
			if (Length < Other.size())
				Other.erase(Length, Other.size() - Length);
			return Other;
		}
		String& Stringify::Compress(String& Other, const char* Tokenbase, const char* NotInBetweenOf, size_t Start)
		{
			size_t TokenbaseSize = (Tokenbase ? strlen(Tokenbase) : 0);
			size_t NiboSize = (NotInBetweenOf ? strlen(NotInBetweenOf) : 0);
			char Skip = '\0';

			for (size_t i = Start; i < Other.size(); i++)
			{
				for (size_t j = 0; j < NiboSize; j++)
				{
					char& Next = Other.at(i);
					if (Next == NotInBetweenOf[j])
					{
						Skip = Next;
						++i;
						break;
					}
				}

				while (Skip != '\0' && i < Other.size() && Other.at(i) != Skip)
					++i;

				if (Skip != '\0')
				{
					Skip = '\0';
					if (i >= Other.size())
						break;
				}

				char& Next = Other.at(i);
				if (Next != ' ' && Next != '\r' && Next != '\n' && Next != '\t')
					continue;

				bool Removable = false;
				if (i > 0)
				{
					Next = Other.at(i - 1);
					for (size_t j = 0; j < TokenbaseSize; j++)
					{
						if (Next == Tokenbase[j])
						{
							Removable = true;
							break;
						}
					}
				}

				if (!Removable && i + 1 < Other.size())
				{
					Next = Other.at(i + 1);
					for (size_t j = 0; j < TokenbaseSize; j++)
					{
						if (Next == Tokenbase[j])
						{
							Removable = true;
							break;
						}
					}
				}

				if (Removable)
					Other.erase(Other.begin() + i--);
				else
					Other[i] = ' ';
			}
			return Other;
		}
		String& Stringify::ReplaceOf(String& Other, const char* Chars, const char* To, size_t Start)
		{
			VI_ASSERT(Chars != nullptr && Chars[0] != '\0' && To != nullptr, "match list and replacer should not be empty");
			TextSettle Result{ };
			size_t Offset = Start, ToSize = strlen(To);
			while ((Result = FindOf(Other, Chars, Offset)).Found)
			{
				EraseOffsets(Other, Result.Start, Result.End);
				Other.insert(Result.Start, To);
				Offset = Result.Start + ToSize;
			}
			return Other;
		}
		String& Stringify::ReplaceNotOf(String& Other, const char* Chars, const char* To, size_t Start)
		{
			VI_ASSERT(Chars != nullptr && Chars[0] != '\0' && To != nullptr, "match list and replacer should not be empty");
			TextSettle Result{};
			size_t Offset = Start, ToSize = strlen(To);
			while ((Result = FindNotOf(Other, Chars, Offset)).Found)
			{
				EraseOffsets(Other, Result.Start, Result.End);
				Other.insert(Result.Start, To);
				Offset = Result.Start + ToSize;
			}
			return Other;
		}
		String& Stringify::Replace(String& Other, const String& From, const String& To, size_t Start)
		{
			VI_ASSERT(!From.empty(), "match should not be empty");
			size_t Offset = Start;
			TextSettle Result{ };

			while ((Result = Find(Other, From, Offset)).Found)
			{
				EraseOffsets(Other, Result.Start, Result.End);
				Other.insert(Result.Start, To);
				Offset = Result.Start + To.size();
			}
			return Other;
		}
		String& Stringify::ReplaceGroups(String& Other, const String& FromRegex, const String& To)
		{
			Compute::RegexSource Source('(' + FromRegex + ')');
			Compute::Regex::Replace(&Source, To, Other);
			return Other;
		}
		String& Stringify::Replace(String& Other, const char* From, const char* To, size_t Start)
		{
			VI_ASSERT(From != nullptr && To != nullptr, "from and to should not be empty");
			size_t Offset = Start;
			size_t Size = strlen(To);
			TextSettle Result{ };

			while ((Result = Find(Other, From, Offset)).Found)
			{
				EraseOffsets(Other, Result.Start, Result.End);
				Other.insert(Result.Start, To);
				Offset = Result.Start + Size;
			}
			return Other;
		}
		String& Stringify::Replace(String& Other, const char& From, const char& To, size_t Position)
		{
			for (size_t i = Position; i < Other.size(); i++)
			{
				char& C = Other.at(i);
				if (C == From)
					C = To;
			}
			return Other;
		}
		String& Stringify::Replace(String& Other, const char& From, const char& To, size_t Position, size_t Count)
		{
			VI_ASSERT(Other.size() >= (Position + Count), "invalid offset");
			size_t Size = Position + Count;
			for (size_t i = Position; i < Size; i++)
			{
				char& C = Other.at(i);
				if (C == From)
					C = To;
			}
			return Other;
		}
		String& Stringify::ReplacePart(String& Other, size_t Start, size_t End, const String& Value)
		{
			return ReplacePart(Other, Start, End, Value.c_str());
		}
		String& Stringify::ReplacePart(String& Other, size_t Start, size_t End, const char* Value)
		{
			VI_ASSERT(Start < Other.size(), "invalid start");
			VI_ASSERT(End <= Other.size(), "invalid end");
			VI_ASSERT(Start < End, "start should be less than end");
			VI_ASSERT(Value != nullptr, "replacer should not be empty");
			if (Start == 0)
			{
				if (Other.size() != End)
					Other.assign(Value + Other.substr(End, Other.size() - End));
				else
					Other.assign(Value);
			}
			else if (Other.size() == End)
				Other.assign(Other.substr(0, Start) + Value);
			else
				Other.assign(Other.substr(0, Start) + Value + Other.substr(End, Other.size() - End));
			return Other;
		}
		String& Stringify::ReplaceStartsWithEndsOf(String& Other, const char* Begins, const char* EndsOf, const String& With, size_t Start)
		{
			VI_ASSERT(Begins != nullptr && Begins[0] != '\0', "begin should not be empty");
			VI_ASSERT(EndsOf != nullptr && EndsOf[0] != '\0', "end should not be empty");

			size_t BeginsSize = strlen(Begins), EndsOfSize = strlen(EndsOf);
			for (size_t i = Start; i < Other.size(); i++)
			{
				size_t From = i, BeginsOffset = 0;
				while (BeginsOffset < BeginsSize && From < Other.size() && Other.at(From) == Begins[BeginsOffset])
				{
					++From;
					++BeginsOffset;
				}

				bool Matching = false;
				if (BeginsOffset != BeginsSize)
				{
					i = From;
					continue;
				}

				size_t To = From;
				while (!Matching && To < Other.size())
				{
					auto& Next = Other.at(To++);
					for (size_t j = 0; j < EndsOfSize; j++)
					{
						if (Next == EndsOf[j])
						{
							Matching = true;
							break;
						}
					}
				}

				if (To >= Other.size())
					Matching = true;

				if (!Matching)
					continue;

				Other.replace(Other.begin() + From - BeginsSize, Other.begin() + To, With);
				i = With.size();
			}
			return Other;
		}
		String& Stringify::ReplaceInBetween(String& Other, const char* Begins, const char* Ends, const String& With, bool Recursive, size_t Start)
		{
			VI_ASSERT(Begins != nullptr && Begins[0] != '\0', "begin should not be empty");
			VI_ASSERT(Ends != nullptr && Ends[0] != '\0', "end should not be empty");

			size_t BeginsSize = strlen(Begins), EndsSize = strlen(Ends);
			for (size_t i = Start; i < Other.size(); i++)
			{
				size_t From = i, BeginsOffset = 0;
				while (BeginsOffset < BeginsSize && From < Other.size() && Other.at(From) == Begins[BeginsOffset])
				{
					++From;
					++BeginsOffset;
				}

				size_t Nesting = 1;
				if (BeginsOffset != BeginsSize)
				{
					i = From;
					continue;
				}

				size_t To = From, EndsOffset = 0;
				while (To < Other.size())
				{
					if (Other.at(To++) != Ends[EndsOffset])
					{
						if (!Recursive)
							continue;

						size_t Substep = To - 1, Suboffset = 0;
						while (Suboffset < BeginsSize && Substep < Other.size() && Other.at(Substep) == Begins[Suboffset])
						{
							++Substep;
							++Suboffset;
						}

						if (Suboffset == BeginsSize)
							++Nesting;
					}
					else if (++EndsOffset >= EndsSize)
					{
						if (!--Nesting)
							break;

						EndsOffset = 0;
					}
				}

				if (EndsOffset != EndsSize)
				{
					i = To;
					continue;
				}

				if (To > Other.size())
					To = Other.size();

				Other.replace(Other.begin() + From - BeginsSize, Other.begin() + To, With);
				i = With.size();
			}
			return Other;
		}
		String& Stringify::ReplaceNotInBetween(String& Other, const char* Begins, const char* Ends, const String& With, bool Recursive, size_t Start)
		{
			VI_ASSERT(Begins != nullptr && Begins[0] != '\0', "begin should not be empty");
			VI_ASSERT(Ends != nullptr && Ends[0] != '\0', "end should not be empty");

			size_t BeginsSize = strlen(Begins), EndsSize = strlen(Ends);
			size_t ReplaceAt = String::npos;

			for (size_t i = Start; i < Other.size(); i++)
			{
				size_t From = i, BeginsOffset = 0;
				while (BeginsOffset < BeginsSize && From < Other.size() && Other.at(From) == Begins[BeginsOffset])
				{
					++From;
					++BeginsOffset;
				}

				size_t Nesting = 1;
				if (BeginsOffset != BeginsSize)
				{
					if (ReplaceAt == String::npos)
						ReplaceAt = i;

					continue;
				}

				if (ReplaceAt != String::npos)
				{
					Other.replace(Other.begin() + ReplaceAt, Other.begin() + i, With);
					From = ReplaceAt + BeginsSize + With.size();
					i = From - BeginsSize;
					ReplaceAt = String::npos;
				}

				size_t To = From, EndsOffset = 0;
				while (To < Other.size())
				{
					if (Other.at(To++) != Ends[EndsOffset])
					{
						if (!Recursive)
							continue;

						size_t Substep = To - 1, Suboffset = 0;
						while (Suboffset < BeginsSize && Substep < Other.size() && Other.at(Substep) == Begins[Suboffset])
						{
							++Substep;
							++Suboffset;
						}

						if (Suboffset == BeginsSize)
							++Nesting;
					}
					else if (++EndsOffset >= EndsSize)
					{
						if (!--Nesting)
							break;

						EndsOffset = 0;
					}
				}

				i = To - 1;
			}

			if (ReplaceAt != String::npos)
				Other.replace(Other.begin() + ReplaceAt, Other.end(), With);
			return Other;
		}
		String& Stringify::ReplaceParts(String& Other, Vector<std::pair<String, TextSettle>>& Inout, const String& With, const std::function<char(const String&, char, int)>& Surrounding)
		{
			VI_SORT(Inout.begin(), Inout.end(), [](const std::pair<String, TextSettle>& A, const std::pair<String, TextSettle>& B)
			{
				return A.second.Start < B.second.Start;
			});

			int64_t Offset = 0;
			for (auto& Item : Inout)
			{
				size_t Size = Item.second.End - Item.second.Start;
				if (!Item.second.Found || !Size)
					continue;

				Item.second.Start = (size_t)((int64_t)Item.second.Start + Offset);
				Item.second.End = (size_t)((int64_t)Item.second.End + Offset);
				if (Surrounding != nullptr)
				{
					String Replacement = With;
					if (Item.second.Start > 0)
					{
						char Next = Surrounding(Item.first, Other.at(Item.second.Start - 1), -1);
						if (Next != '\0')
							Replacement.insert(Replacement.begin(), Next);
					}

					if (Item.second.End < Other.size())
					{
						char Next = Surrounding(Item.first, Other.at(Item.second.End), 1);
						if (Next != '\0')
							Replacement.push_back(Next);
					}

					ReplacePart(Other, Item.second.Start, Item.second.End, Replacement);
					Offset += (int64_t)Replacement.size() - (int64_t)Size;
					Item.second.End = Item.second.Start + Replacement.size();
				}
				else
				{
					ReplacePart(Other, Item.second.Start, Item.second.End, With);
					Offset += (int64_t)With.size() - (int64_t)Size;
					Item.second.End = Item.second.Start + With.size();
				}
			}
			return Other;
		}
		String& Stringify::ReplaceParts(String& Other, Vector<TextSettle>& Inout, const String& With, const std::function<char(char, int)>& Surrounding)
		{
			VI_SORT(Inout.begin(), Inout.end(), [](const TextSettle& A, const TextSettle& B)
			{
				return A.Start < B.Start;
			});

			int64_t Offset = 0;
			for (auto& Item : Inout)
			{
				size_t Size = Item.End - Item.Start;
				if (!Item.Found || !Size)
					continue;

				Item.Start = (size_t)((int64_t)Item.Start + Offset);
				Item.End = (size_t)((int64_t)Item.End + Offset);
				if (Surrounding != nullptr)
				{
					String Replacement = With;
					if (Item.Start > 0)
					{
						char Next = Surrounding(Other.at(Item.Start - 1), -1);
						if (Next != '\0')
							Replacement.insert(Replacement.begin(), Next);
					}

					if (Item.End < Other.size())
					{
						char Next = Surrounding(Other.at(Item.End), 1);
						if (Next != '\0')
							Replacement.push_back(Next);
					}

					ReplacePart(Other, Item.Start, Item.End, Replacement);
					Offset += (int64_t)Replacement.size() - (int64_t)Size;
					Item.End = Item.Start + Replacement.size();
				}
				else
				{
					ReplacePart(Other, Item.Start, Item.End, With);
					Offset += (int64_t)With.size() - (int64_t)Size;
					Item.End = Item.Start + With.size();
				}
			}
			return Other;
		}
		String& Stringify::RemovePart(String& Other, size_t Start, size_t End)
		{
			VI_ASSERT(Start < Other.size(), "invalid start");
			VI_ASSERT(End <= Other.size(), "invalid end");
			VI_ASSERT(Start < End, "start should be less than end");

			if (Start == 0)
			{
				if (Other.size() != End)
					Other.assign(Other.substr(End, Other.size() - End));
				else
					Other.clear();
			}
			else if (Other.size() == End)
				Other.assign(Other.substr(0, Start));
			else
				Other.assign(Other.substr(0, Start) + Other.substr(End, Other.size() - End));
			return Other;
		}
		String& Stringify::Reverse(String& Other)
		{
			if (!Other.empty())
				Reverse(Other, 0, Other.size());
			return Other;
		}
		String& Stringify::Reverse(String& Other, size_t Start, size_t End)
		{
			VI_ASSERT(Other.size() >= 2, "length should be at least 2 chars");
			VI_ASSERT(End < Other.size(), "end should be less than length - 1");
			VI_ASSERT(Start < Other.size(), "start should be less than length - 1");
			VI_ASSERT(Start < End, "start should be less than end");
			std::reverse(Other.begin() + Start, Other.begin() + End);
			return Other;
		}
		String& Stringify::Substring(String& Other, const TextSettle& Result)
		{
			VI_ASSERT(Result.Found, "result should be found");
			if (Result.Start >= Other.size())
			{
				Other.clear();
				return Other;
			}

			auto Offset = (int64_t)Result.End;
			if (Result.End > Other.size())
				Offset = (int64_t)(Other.size() - Result.Start);

			Offset = (int64_t)Result.Start - Offset;
			Other.assign(Other.substr(Result.Start, (size_t)(Offset < 0 ? -Offset : Offset)));
			return Other;
		}
		String& Stringify::Splice(String& Other, size_t Start, size_t End)
		{
			VI_ASSERT(Start < Other.size(), "result start should be less or equal than length - 1");
			if (End > Other.size())
				End = (Other.size() - Start);

			int64_t Offset = (int64_t)Start - (int64_t)End;
			Other.assign(Other.substr(Start, (size_t)(Offset < 0 ? -Offset : Offset)));
			return Other;
		}
		String& Stringify::Trim(String& Other)
		{
			TrimStart(Other);
			TrimEnd(Other);
			return Other;
		}
		String& Stringify::TrimStart(String& Other)
		{
			Other.erase(Other.begin(), std::find_if(Other.begin(), Other.end(), [](int C) -> int
			{
				if (C < -1 || C > 255)
					return 1;

				return std::isspace(C) == 0 ? 1 : 0;
			}));
			return Other;
		}
		String& Stringify::TrimEnd(String& Other)
		{
			Other.erase(std::find_if(Other.rbegin(), Other.rend(), [](int C) -> int
			{
				if (C < -1 || C > 255)
					return 1;

				return std::isspace(C) == 0 ? 1 : 0;
			}).base(), Other.end());
			return Other;
		}
		String& Stringify::Fill(String& Other, const char& Char)
		{
			for (char& i : Other)
				i = Char;
			return Other;
		}
		String& Stringify::Fill(String& Other, const char& Char, size_t Count)
		{
			Other.assign(Count, Char);
			return Other;
		}
		String& Stringify::Fill(String& Other, const char& Char, size_t Start, size_t Count)
		{
			VI_ASSERT(!Other.empty(), "length should be greater than Zero");
			VI_ASSERT(Start <= Other.size(), "start should be less or equal than length");
			if (Start + Count > Other.size())
				Count = Other.size() - Start;

			size_t Size = (Start + Count);
			for (size_t i = Start; i < Size; i++)
				Other.at(i) = Char;
			return Other;
		}
		String& Stringify::Append(String& Other, const char* Format, ...)
		{
			VI_ASSERT(Format != nullptr, "format should be set");
			char Buffer[BLOB_SIZE];
			va_list Args;
			va_start(Args, Format);
			int Count = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			if (Count > sizeof(Buffer))
				Count = sizeof(Buffer);

			Other.append(Buffer, Count);
			return Other;
		}
		String& Stringify::Erase(String& Other, size_t Position)
		{
			VI_ASSERT(Position < Other.size(), "position should be less than length");
			Other.erase(Position);
			return Other;
		}
		String& Stringify::Erase(String& Other, size_t Position, size_t Count)
		{
			VI_ASSERT(Position < Other.size(), "position should be less than length");
			Other.erase(Position, Count);
			return Other;
		}
		String& Stringify::EraseOffsets(String& Other, size_t Start, size_t End)
		{
			return Erase(Other, Start, End - Start);
		}
		String& Stringify::EvalEnvs(String& Other, const String& Net, const String& Dir)
		{
			if (Other.empty())
				return Other;

			if (StartsOf(Other, "./\\"))
			{
				ExpectsIO<String> Result = OS::Path::Resolve(Other.c_str(), Dir, false);
				if (Result)
					Other.assign(*Result);
			}
			else if (Other.front() == '$' && Other.size() > 1)
			{
				const char* Env = std::getenv(Other.c_str() + 1);
				if (!Env)
				{
					VI_WARN("[env] cannot resolve environmental variable [%s]", Other.c_str() + 1);
					Other.clear();
				}
				else
					Other.assign(Env);
			}
			else
				Replace(Other, "[subnet]", Net);

			return Other;
		}
		Vector<std::pair<String, TextSettle>> Stringify::FindInBetween(const String& Other, const char* Begins, const char* Ends, const char* NotInSubBetweenOf, size_t Offset)
		{
			Vector<std::pair<String, TextSettle>> Result;
			VI_ASSERT(Begins != nullptr && Begins[0] != '\0', "begin should not be empty");
			VI_ASSERT(Ends != nullptr && Ends[0] != '\0', "end should not be empty");

			size_t BeginsSize = strlen(Begins), EndsSize = strlen(Ends);
			size_t NisboSize = (NotInSubBetweenOf ? strlen(NotInSubBetweenOf) : 0);
			char Skip = '\0';

			for (size_t i = Offset; i < Other.size(); i++)
			{
				for (size_t j = 0; j < NisboSize; j++)
				{
					const char& Next = Other.at(i);
					if (Next == NotInSubBetweenOf[j])
					{
						Skip = Next;
						++i;
						break;
					}
				}

				while (Skip != '\0' && i < Other.size() && Other.at(i) != Skip)
					++i;

				if (Skip != '\0')
				{
					Skip = '\0';
					if (i >= Other.size())
						break;
				}

				size_t From = i, BeginsOffset = 0;
				while (BeginsOffset < BeginsSize && From < Other.size() && Other.at(From) == Begins[BeginsOffset])
				{
					++From;
					++BeginsOffset;
				}

				if (BeginsOffset != BeginsSize)
				{
					i = From;
					continue;
				}

				size_t To = From, EndsOffset = 0;
				while (To < Other.size())
				{
					if (Other.at(To++) != Ends[EndsOffset])
						continue;

					if (++EndsOffset >= EndsSize)
						break;
				}

				i = To;
				if (EndsOffset != EndsSize)
					continue;

				TextSettle At;
				At.Start = From - BeginsSize;
				At.End = To;
				At.Found = true;

				Result.push_back(std::make_pair(Other.substr(From, (To - EndsSize) - From), std::move(At)));
			}

			return Result;
		}
		Vector<std::pair<String, TextSettle>> Stringify::FindInBetweenInCode(const String& Other, const char* Begins, const char* Ends, size_t Offset)
		{
			Vector<std::pair<String, TextSettle>> Result;
			VI_ASSERT(Begins != nullptr && Begins[0] != '\0', "begin should not be empty");
			VI_ASSERT(Ends != nullptr && Ends[0] != '\0', "end should not be empty");

			size_t BeginsSize = strlen(Begins), EndsSize = strlen(Ends);
			for (size_t i = Offset; i < Other.size(); i++)
			{
				if (Other.at(i) == '/' && i + 1 < Other.size() && (Other[i + 1] == '/' || Other[i + 1] == '*'))
				{
					if (Other[++i] == '*')
					{
						while (i + 1 < Other.size())
						{
							char N = Other[i++];
							if (N == '*' && Other[i++] == '/')
								break;
						}
					}
					else
					{
						while (i < Other.size())
						{
							char N = Other[i++];
							if (N == '\r' || N == '\n')
								break;
						}
					}

					continue;
				}

				size_t From = i, BeginsOffset = 0;
				while (BeginsOffset < BeginsSize && From < Other.size() && Other.at(From) == Begins[BeginsOffset])
				{
					++From;
					++BeginsOffset;
				}

				if (BeginsOffset != BeginsSize)
				{
					i = From;
					continue;
				}

				size_t To = From, EndsOffset = 0;
				while (To < Other.size())
				{
					if (Other.at(To++) != Ends[EndsOffset])
						continue;

					if (++EndsOffset >= EndsSize)
						break;
				}

				i = To;
				if (EndsOffset != EndsSize)
					continue;

				TextSettle At;
				At.Start = From - BeginsSize;
				At.End = To;
				At.Found = true;

				Result.push_back(std::make_pair(Other.substr(From, (To - EndsSize) - From), std::move(At)));
			}

			return Result;
		}
		Vector<std::pair<String, TextSettle>> Stringify::FindStartsWithEndsOf(const String& Other, const char* Begins, const char* EndsOf, const char* NotInSubBetweenOf, size_t Offset)
		{
			Vector<std::pair<String, TextSettle>> Result;
			VI_ASSERT(Begins != nullptr && Begins[0] != '\0', "begin should not be empty");
			VI_ASSERT(EndsOf != nullptr && EndsOf[0] != '\0', "end should not be empty");

			size_t BeginsSize = strlen(Begins), EndsOfSize = strlen(EndsOf);
			size_t NisboSize = (NotInSubBetweenOf ? strlen(NotInSubBetweenOf) : 0);
			char Skip = '\0';

			for (size_t i = Offset; i < Other.size(); i++)
			{
				for (size_t j = 0; j < NisboSize; j++)
				{
					const char& Next = Other.at(i);
					if (Next == NotInSubBetweenOf[j])
					{
						Skip = Next;
						++i;
						break;
					}
				}

				while (Skip != '\0' && i < Other.size() && Other.at(i) != Skip)
					++i;

				if (Skip != '\0')
				{
					Skip = '\0';
					if (i >= Other.size())
						break;
				}

				size_t From = i, BeginsOffset = 0;
				while (BeginsOffset < BeginsSize && From < Other.size() && Other.at(From) == Begins[BeginsOffset])
				{
					++From;
					++BeginsOffset;
				}

				bool Matching = false;
				if (BeginsOffset != BeginsSize)
				{
					i = From;
					continue;
				}

				size_t To = From;
				while (!Matching && To < Other.size())
				{
					auto& Next = Other.at(To++);
					for (size_t j = 0; j < EndsOfSize; j++)
					{
						if (Next == EndsOf[j])
						{
							Matching = true;
							--To;
							break;
						}
					}
				}

				if (To >= Other.size())
					Matching = true;

				if (!Matching)
					continue;

				TextSettle At;
				At.Start = From - BeginsSize;
				At.End = To;
				At.Found = true;

				Result.push_back(std::make_pair(Other.substr(From, To - From), std::move(At)));
			}

			return Result;
		}
		TextSettle Stringify::ReverseFind(const String& Other, const String& Needle, size_t Offset)
		{
			if (Other.empty() || Offset >= Other.size())
				return { Other.size(), Other.size(), false };

			const char* Ptr = Other.c_str() - Offset;
			if (Needle.c_str() > Ptr)
				return { Other.size(), Other.size(), false };

			const char* It = nullptr;
			for (It = Ptr + Other.size() - Needle.size(); It > Ptr; --It)
			{
				if (strncmp(Ptr, Needle.c_str(), Needle.size()) == 0)
				{
					size_t Set = (size_t)(It - Ptr);
					return { Set, Set + Needle.size(), true };
				}
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::ReverseFind(const String& Other, const char* Needle, size_t Offset)
		{
			if (Other.empty() || Offset >= Other.size())
				return { Other.size(), Other.size(), false };

			if (!Needle)
				return { Other.size(), Other.size(), false };

			const char* Ptr = Other.c_str() - Offset;
			if (Needle > Ptr)
				return { Other.size(), Other.size(), false };

			const char* It = nullptr;
			size_t Length = strlen(Needle);
			for (It = Ptr + Other.size() - Length; It > Ptr; --It)
			{
				if (strncmp(Ptr, Needle, Length) == 0)
					return { (size_t)(It - Ptr), (size_t)(It - Ptr + Length), true };
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::ReverseFind(const String& Other, const char& Needle, size_t Offset)
		{
			if (Other.empty() || Offset >= Other.size())
				return { Other.size(), Other.size(), false };

			size_t Size = Other.size() - Offset;
			for (size_t i = Size; i-- > 0;)
			{
				if (Other.at(i) == Needle)
					return { i, i + 1, true };
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::ReverseFindUnescaped(const String& Other, const char& Needle, size_t Offset)
		{
			if (Other.empty() || Offset >= Other.size())
				return { Other.size(), Other.size(), false };

			size_t Size = Other.size() - Offset;
			for (size_t i = Size; i-- > 0;)
			{
				if (Other.at(i) == Needle && ((int64_t)i - 1 < 0 || Other.at(i - 1) != '\\'))
					return { i, i + 1, true };
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::ReverseFindOf(const String& Other, const String& Needle, size_t Offset)
		{
			if (Other.empty() || Offset >= Other.size())
				return { Other.size(), Other.size(), false };

			size_t Size = Other.size() - Offset;
			for (size_t i = Size; i-- > 0;)
			{
				for (char k : Needle)
				{
					if (Other.at(i) == k)
						return { i, i + 1, true };
				}
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::ReverseFindOf(const String& Other, const char* Needle, size_t Offset)
		{
			if (Other.empty() || Offset >= Other.size())
				return { Other.size(), Other.size(), false };

			if (!Needle)
				return { Other.size(), Other.size(), false };

			size_t Length = strlen(Needle);
			size_t Size = Other.size() - Offset;
			for (size_t i = Size; i-- > 0;)
			{
				for (size_t k = 0; k < Length; k++)
				{
					if (Other.at(i) == Needle[k])
						return { i, i + 1, true };
				}
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::Find(const String& Other, const String& Needle, size_t Offset)
		{
			if (Other.empty() || Offset >= Other.size())
				return { Other.size(), Other.size(), false };

			const char* It = strstr(Other.c_str() + Offset, Needle.c_str());
			if (It == nullptr)
				return { Other.size(), Other.size(), false };

			size_t Set = (size_t)(It - Other.c_str());
			return { Set, Set + Needle.size(), true };
		}
		TextSettle Stringify::Find(const String& Other, const char* Needle, size_t Offset)
		{
			VI_ASSERT(Needle != nullptr, "needle should be set");
			if (Other.empty() || Offset >= Other.size())
				return { Other.size(), Other.size(), false };

			const char* It = strstr(Other.c_str() + Offset, Needle);
			if (It == nullptr)
				return { Other.size(), Other.size(), false };

			size_t Set = (size_t)(It - Other.c_str());
			return { Set, Set + strlen(Needle), true };
		}
		TextSettle Stringify::Find(const String& Other, const char& Needle, size_t Offset)
		{
			for (size_t i = Offset; i < Other.size(); i++)
			{
				if (Other.at(i) == Needle)
					return { i, i + 1, true };
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::FindUnescaped(const String& Other, const char& Needle, size_t Offset)
		{
			for (size_t i = Offset; i < Other.size(); i++)
			{
				if (Other.at(i) == Needle && ((int64_t)i - 1 < 0 || Other.at(i - 1) != '\\'))
					return { i, i + 1, true };
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::FindOf(const String& Other, const String& Needle, size_t Offset)
		{
			for (size_t i = Offset; i < Other.size(); i++)
			{
				for (char k : Needle)
				{
					if (Other.at(i) == k)
						return { i, i + 1, true };
				}
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::FindOf(const String& Other, const char* Needle, size_t Offset)
		{
			VI_ASSERT(Needle != nullptr, "needle should be set");
			size_t Length = strlen(Needle);
			for (size_t i = Offset; i < Other.size(); i++)
			{
				for (size_t k = 0; k < Length; k++)
				{
					if (Other.at(i) == Needle[k])
						return { i, i + 1, true };
				}
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::FindNotOf(const String& Other, const String& Needle, size_t Offset)
		{
			for (size_t i = Offset; i < Other.size(); i++)
			{
				bool Result = false;
				for (char k : Needle)
				{
					if (Other.at(i) == k)
					{
						Result = true;
						break;
					}
				}

				if (!Result)
					return { i, i + 1, true };
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::FindNotOf(const String& Other, const char* Needle, size_t Offset)
		{
			VI_ASSERT(Needle != nullptr, "needle should be set");
			size_t Length = strlen(Needle);
			for (size_t i = Offset; i < Other.size(); i++)
			{
				bool Result = false;
				for (size_t k = 0; k < Length; k++)
				{
					if (Other.at(i) == Needle[k])
					{
						Result = true;
						break;
					}
				}

				if (!Result)
					return { i, i + 1, true };
			}

			return { Other.size(), Other.size(), false };
		}
		size_t Stringify::CountLines(const String& Other)
		{
			size_t Lines = 1;
			for (char Item : Other)
			{
				if (Item == '\n')
					++Lines;
			}
			return Lines;
		}
		bool Stringify::IsNotPrecededByEscape(const char* Buffer, size_t Offset, char Escape)
		{
			VI_ASSERT(Buffer != nullptr, "buffer should be set");
			if (Offset < 1 || Buffer[Offset - 1] != Escape)
				return true;

			return Offset > 1 && Buffer[Offset - 2] == Escape;
		}
		bool Stringify::IsNotPrecededByEscape(const String& Other, size_t Offset, char Escape)
		{
			return IsNotPrecededByEscape(Other.c_str(), Offset, Escape);
		}
		bool Stringify::IsEmptyOrWhitespace(const String& Other)
		{
			if (Other.empty())
				return true;

			for (char Next : Other)
			{
				if (!std::isspace(Next))
					return false;
			}

			return true;
		}
		bool Stringify::IsPrecededBy(const String& Other, size_t At, const char* Of)
		{
			VI_ASSERT(Of != nullptr, "tokenbase should be set");
			if (!At || At - 1 >= Other.size())
				return false;

			size_t Size = strlen(Of);
			const char& Next = Other.at(At - 1);
			for (size_t i = 0; i < Size; i++)
			{
				if (Next == Of[i])
					return true;
			}

			return false;
		}
		bool Stringify::IsFollowedBy(const String& Other, size_t At, const char* Of)
		{
			VI_ASSERT(Of != nullptr, "tokenbase should be set");
			if (At + 1 >= Other.size())
				return false;

			size_t Size = strlen(Of);
			const char& Next = Other.at(At + 1);
			for (size_t i = 0; i < Size; i++)
			{
				if (Next == Of[i])
					return true;
			}

			return false;
		}
		bool Stringify::StartsWith(const String& Other, const String& Value, size_t Offset)
		{
			if (Other.size() < Value.size())
				return false;

			for (size_t i = Offset; i < Value.size(); i++)
			{
				if (Value[i] != Other.at(i))
					return false;
			}

			return true;
		}
		bool Stringify::StartsWith(const String& Other, const char* Value, size_t Offset)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			size_t Length = strlen(Value);
			if (Other.size() < Length)
				return false;

			for (size_t i = Offset; i < Length; i++)
			{
				if (Value[i] != Other.at(i))
					return false;
			}

			return true;
		}
		bool Stringify::StartsOf(const String& Other, const char* Value, size_t Offset)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			size_t Length = strlen(Value);
			if (Offset >= Other.size())
				return false;

			for (size_t j = 0; j < Length; j++)
			{
				if (Other.at(Offset) == Value[j])
					return true;
			}

			return false;
		}
		bool Stringify::StartsNotOf(const String& Other, const char* Value, size_t Offset)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			size_t Length = strlen(Value);
			if (Offset >= Other.size())
				return false;

			bool Result = true;
			for (size_t j = 0; j < Length; j++)
			{
				if (Other.at(Offset) == Value[j])
				{
					Result = false;
					break;
				}
			}

			return Result;
		}
		bool Stringify::EndsWith(const String& Other, const String& Value)
		{
			if (Other.empty() || Value.size() > Other.size())
				return false;

			return strcmp(Other.c_str() + Other.size() - Value.size(), Value.c_str()) == 0;
		}
		bool Stringify::EndsWith(const String& Other, const char* Value)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			size_t Size = strlen(Value);
			if (Other.empty() || Size > Other.size())
				return false;

			return strcmp(Other.c_str() + Other.size() - Size, Value) == 0;
		}
		bool Stringify::EndsWith(const String& Other, const char& Value)
		{
			return !Other.empty() && Other.back() == Value;
		}
		bool Stringify::EndsOf(const String& Other, const char* Value)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			if (Other.empty())
				return false;

			size_t Length = strlen(Value);
			for (size_t j = 0; j < Length; j++)
			{
				if (Other.back() == Value[j])
					return true;
			}

			return false;
		}
		bool Stringify::EndsNotOf(const String& Other, const char* Value)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			if (Other.empty())
				return true;

			size_t Length = strlen(Value);
			for (size_t j = 0; j < Length; j++)
			{
				if (Other.back() == Value[j])
					return false;
			}

			return true;
		}
		bool Stringify::HasInteger(const String& Other)
		{
			if (Other.empty())
				return false;

			bool HadSign = false;
			for (size_t i = 0; i < Other.size(); i++)
			{
				const char& V = Other[i];
				if (IsDigit(V))
					continue;

				if ((V == '+' || V == '-') && i == 0 && !HadSign)
				{
					HadSign = true;
					continue;
				}

				return false;
			}

			if (HadSign && Other.size() < 2)
				return false;

			return true;
		}
		bool Stringify::HasNumber(const String& Other)
		{
			if (Other.empty() || (Other.size() == 1 && Other.front() == '.'))
				return false;

			bool HadPoint = false, HadSign = false;
			for (size_t i = 0; i < Other.size(); i++)
			{
				const char& V = Other[i];
				if (IsDigit(V))
					continue;

				if ((V == '+' || V == '-') && i == 0 && !HadSign)
				{
					HadSign = true;
					continue;
				}

				if (V == '.' && !HadPoint)
				{
					HadPoint = true;
					continue;
				}

				return false;
			}

			if (HadSign && HadPoint && Other.size() < 3)
				return false;
			else if ((HadSign || HadPoint) && Other.size() < 2)
				return false;

			return true;
		}
		bool Stringify::HasDecimal(const String& Other)
		{
			auto F = Find(Other, '.');
			if (!F.Found)
				return HasInteger(Other) && Other.size() >= 19;

			auto D1 = Other.substr(0, F.End - 1);
			if (D1.empty() || !HasInteger(D1))
				return false;

			auto D2 = Other.substr(F.End + 1, Other.size() - F.End - 1);
			if (D2.empty() || !HasInteger(D2))
				return false;

			return D1.size() >= 19 || D2.size() > 6;
		}
		bool Stringify::IsDigit(char Char)
		{
			return Char == '0' || Char == '1' || Char == '2' || Char == '3' || Char == '4' || Char == '5' || Char == '6' || Char == '7' || Char == '8' || Char == '9';
		}
		bool Stringify::IsAlphabetic(char Char)
		{
			if ((int)Char < -1 || (int)Char > 255)
				return false;

			return std::isalpha(Char) != 0;
		}
		int Stringify::CaseCompare(const char* Value1, const char* Value2)
		{
			VI_ASSERT(Value1 != nullptr && Value2 != nullptr, "both values should be set");

			int Result;
			do
			{
				Result = tolower(*(const unsigned char*)(Value1++)) - tolower(*(const unsigned char*)(Value2++));
			} while (Result == 0 && Value1[-1] != '\0');

			return Result;
		}
		int Stringify::Match(const char* Pattern, const char* Text)
		{
			VI_ASSERT(Pattern != nullptr && Text != nullptr, "pattern and text should be set");
			return Match(Pattern, strlen(Pattern), Text);
		}
		int Stringify::Match(const char* Pattern, size_t Length, const char* Text)
		{
			VI_ASSERT(Pattern != nullptr && Text != nullptr, "pattern and text should be set");
			const char* Token = (const char*)memchr(Pattern, '|', (size_t)Length);
			if (Token != nullptr)
			{
				int Output = Match(Pattern, (size_t)(Token - Pattern), Text);
				return (Output > 0) ? Output : Match(Token + 1, (size_t)((Pattern + Length) - (Token + 1)), Text);
			}

			int Offset = 0, Result = 0;
			size_t i = 0; int j = 0;
			while (i < Length)
			{
				if (Pattern[i] == '?' && Text[j] != '\0')
					continue;

				if (Pattern[i] == '$')
					return (Text[j] == '\0') ? j : -1;

				if (Pattern[i] == '*')
				{
					i++;
					if (Pattern[i] == '*')
					{
						Offset = (int)strlen(Text + j);
						i++;
					}
					else
						Offset = (int)strcspn(Text + j, "/");

					if (i == Length)
						return j + Offset;

					do
					{
						Result = Match(Pattern + i, Length - i, Text + j + Offset);
					} while (Result == -1 && Offset-- > 0);

					return (Result == -1) ? -1 : j + Result + Offset;
				}
				else if (tolower((const unsigned char)Pattern[i]) != tolower((const unsigned char)Text[j]))
					return -1;

				i++;
				j++;
			}

			return j;
		}
		String Stringify::Text(const char* Format, ...)
		{
			VI_ASSERT(Format != nullptr, "format should be set");
			va_list Args;
			va_start(Args, Format);
			char Buffer[BLOB_SIZE];
			int Size = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			if (Size > sizeof(Buffer))
				Size = sizeof(Buffer);
			va_end(Args);
			return String(Buffer, (size_t)Size);
		}
		WideString Stringify::ToWide(const String& Other)
		{
			WideString Output;
			Output.reserve(Other.size());

			wchar_t W;
			for (size_t i = 0; i < Other.size();)
			{
				char C = Other.at(i);
				if ((C & 0x80) == 0)
				{
					W = C;
					i++;
				}
				else if ((C & 0xE0) == 0xC0)
				{
					W = (C & 0x1F) << 6;
					W |= (Other.at(i + 1) & 0x3F);
					i += 2;
				}
				else if ((C & 0xF0) == 0xE0)
				{
					W = (C & 0xF) << 12;
					W |= (Other.at(i + 1) & 0x3F) << 6;
					W |= (Other.at(i + 2) & 0x3F);
					i += 3;
				}
				else if ((C & 0xF8) == 0xF0)
				{
					W = (C & 0x7) << 18;
					W |= (Other.at(i + 1) & 0x3F) << 12;
					W |= (Other.at(i + 2) & 0x3F) << 6;
					W |= (Other.at(i + 3) & 0x3F);
					i += 4;
				}
				else if ((C & 0xFC) == 0xF8)
				{
					W = (C & 0x3) << 24;
					W |= (C & 0x3F) << 18;
					W |= (C & 0x3F) << 12;
					W |= (C & 0x3F) << 6;
					W |= (C & 0x3F);
					i += 5;
				}
				else if ((C & 0xFE) == 0xFC)
				{
					W = (C & 0x1) << 30;
					W |= (C & 0x3F) << 24;
					W |= (C & 0x3F) << 18;
					W |= (C & 0x3F) << 12;
					W |= (C & 0x3F) << 6;
					W |= (C & 0x3F);
					i += 6;
				}
				else
					W = C;

				Output += W;
			}

			return Output;
		}
		Vector<String> Stringify::Split(const String& Other, const String& With, size_t Start)
		{
			Vector<String> Output;
			if (Start >= Other.size())
				return Output;

			size_t Offset = Start;
			TextSettle Result = Find(Other, With, Offset);
			while (Result.Found)
			{
				Output.emplace_back(Other.substr(Offset, Result.Start - Offset));
				Result = Find(Other, With, Offset = Result.End);
			}

			if (Offset < Other.size())
				Output.emplace_back(Other.substr(Offset));

			return Output;
		}
		Vector<String> Stringify::Split(const String& Other, char With, size_t Start)
		{
			Vector<String> Output;
			if (Start >= Other.size())
				return Output;

			size_t Offset = Start;
			TextSettle Result = Find(Other, With, Start);
			while (Result.Found)
			{
				Output.emplace_back(Other.substr(Offset, Result.Start - Offset));
				Result = Find(Other, With, Offset = Result.End);
			}

			if (Offset < Other.size())
				Output.emplace_back(Other.substr(Offset));

			return Output;
		}
		Vector<String> Stringify::SplitMax(const String& Other, char With, size_t Count, size_t Start)
		{
			Vector<String> Output;
			if (Start >= Other.size())
				return Output;

			size_t Offset = Start;
			TextSettle Result = Find(Other, With, Start);
			while (Result.Found && Output.size() < Count)
			{
				Output.emplace_back(Other.substr(Offset, Result.Start - Offset));
				Result = Find(Other, With, Offset = Result.End);
			}

			if (Offset < Other.size() && Output.size() < Count)
				Output.emplace_back(Other.substr(Offset));

			return Output;
		}
		Vector<String> Stringify::SplitOf(const String& Other, const char* With, size_t Start)
		{
			Vector<String> Output;
			if (Start >= Other.size())
				return Output;

			size_t Offset = Start;
			TextSettle Result = FindOf(Other, With, Start);
			while (Result.Found)
			{
				Output.emplace_back(Other.substr(Offset, Result.Start - Offset));
				Result = FindOf(Other, With, Offset = Result.End);
			}

			if (Offset < Other.size())
				Output.emplace_back(Other.substr(Offset));

			return Output;
		}
		Vector<String> Stringify::SplitNotOf(const String& Other, const char* With, size_t Start)
		{
			Vector<String> Output;
			if (Start >= Other.size())
				return Output;

			size_t Offset = Start;
			TextSettle Result = FindNotOf(Other, With, Start);
			while (Result.Found)
			{
				Output.emplace_back(Other.substr(Offset, Result.Start - Offset));
				Result = FindNotOf(Other, With, Offset = Result.End);
			}

			if (Offset < Other.size())
				Output.emplace_back(Other.substr(Offset));

			return Output;
		}
		void Stringify::ConvertToWide(const char* Input, size_t InputSize, wchar_t* Output, size_t OutputSize)
		{
			VI_ASSERT(Input != nullptr, "input should be set");
			VI_ASSERT(Output != nullptr && OutputSize > 0, "output should be set");

			wchar_t W = '\0'; size_t Size = 0;
			for (size_t i = 0; i < InputSize;)
			{
				char C = Input[i];
				if ((C & 0x80) == 0)
				{
					W = C;
					i++;
				}
				else if ((C & 0xE0) == 0xC0)
				{
					W = (C & 0x1F) << 6;
					W |= (Input[i + 1] & 0x3F);
					i += 2;
				}
				else if ((C & 0xF0) == 0xE0)
				{
					W = (C & 0xF) << 12;
					W |= (Input[i + 1] & 0x3F) << 6;
					W |= (Input[i + 2] & 0x3F);
					i += 3;
				}
				else if ((C & 0xF8) == 0xF0)
				{
					W = (C & 0x7) << 18;
					W |= (Input[i + 1] & 0x3F) << 12;
					W |= (Input[i + 2] & 0x3F) << 6;
					W |= (Input[i + 3] & 0x3F);
					i += 4;
				}
				else if ((C & 0xFC) == 0xF8)
				{
					W = (C & 0x3) << 24;
					W |= (C & 0x3F) << 18;
					W |= (C & 0x3F) << 12;
					W |= (C & 0x3F) << 6;
					W |= (C & 0x3F);
					i += 5;
				}
				else if ((C & 0xFE) == 0xFC)
				{
					W = (C & 0x1) << 30;
					W |= (C & 0x3F) << 24;
					W |= (C & 0x3F) << 18;
					W |= (C & 0x3F) << 12;
					W |= (C & 0x3F) << 6;
					W |= (C & 0x3F);
					i += 6;
				}
				else
					W = C;

				Output[Size++] = W;
				if (Size >= OutputSize)
					break;
			}

			if (Size < OutputSize)
				Output[Size] = '\0';
		}

		Schema* Var::Set::Auto(Variant&& Value)
		{
			return new Schema(std::move(Value));
		}
		Schema* Var::Set::Auto(const Variant& Value)
		{
			return new Schema(Value);
		}
		Schema* Var::Set::Auto(const Core::String& Value, bool Strict)
		{
			return new Schema(Var::Auto(Value, Strict));
		}
		Schema* Var::Set::Null()
		{
			return new Schema(Var::Null());
		}
		Schema* Var::Set::Undefined()
		{
			return new Schema(Var::Undefined());
		}
		Schema* Var::Set::Object()
		{
			return new Schema(Var::Object());
		}
		Schema* Var::Set::Array()
		{
			return new Schema(Var::Array());
		}
		Schema* Var::Set::Pointer(void* Value)
		{
			return new Schema(Var::Pointer(Value));
		}
		Schema* Var::Set::String(const Core::String& Value)
		{
			return new Schema(Var::String(Value));
		}
		Schema* Var::Set::String(const char* Value, size_t Size)
		{
			return new Schema(Var::String(Value, Size));
		}
		Schema* Var::Set::Binary(const Core::String& Value)
		{
			return new Schema(Var::Binary(Value));
		}
		Schema* Var::Set::Binary(const unsigned char* Value, size_t Size)
		{
			return new Schema(Var::Binary(Value, Size));
		}
		Schema* Var::Set::Binary(const char* Value, size_t Size)
		{
			return new Schema(Var::Binary(Value, Size));
		}
		Schema* Var::Set::Integer(int64_t Value)
		{
			return new Schema(Var::Integer(Value));
		}
		Schema* Var::Set::Number(double Value)
		{
			return new Schema(Var::Number(Value));
		}
		Schema* Var::Set::Decimal(const Core::Decimal& Value)
		{
			return new Schema(Var::Decimal(Value));
		}
		Schema* Var::Set::Decimal(Core::Decimal&& Value)
		{
			return new Schema(Var::Decimal(std::move(Value)));
		}
		Schema* Var::Set::DecimalString(const Core::String& Value)
		{
			return new Schema(Var::DecimalString(Value));
		}
		Schema* Var::Set::Boolean(bool Value)
		{
			return new Schema(Var::Boolean(Value));
		}

		Variant Var::Auto(const Core::String& Value, bool Strict)
		{
			Variant Result;
			Result.Deserialize(Value, Strict);

			return Result;
		}
		Variant Var::Null()
		{
			return Variant(VarType::Null);
		}
		Variant Var::Undefined()
		{
			return Variant(VarType::Undefined);
		}
		Variant Var::Object()
		{
			return Variant(VarType::Object);
		}
		Variant Var::Array()
		{
			return Variant(VarType::Array);
		}
		Variant Var::Pointer(void* Value)
		{
			if (!Value)
				return Null();

			Variant Result(VarType::Pointer);
			Result.Value.Pointer = (char*)Value;
			return Result;
		}
		Variant Var::String(const Core::String& Value)
		{
			return String(Value.c_str(), Value.size());
		}
		Variant Var::String(const char* Value, size_t Size)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			Variant Result(VarType::String);
			Result.Length = Size;

			size_t StringSize = sizeof(char) * (Result.Length + 1);
			if (Result.Length > Variant::GetMaxSmallStringSize())
				Result.Value.Pointer = VI_MALLOC(char, StringSize);

			char* Data = (char*)Result.GetString();
			memcpy(Data, Value, StringSize - sizeof(char));
			Data[StringSize - 1] = '\0';

			return Result;
		}
		Variant Var::Binary(const Core::String& Value)
		{
			return Binary(Value.c_str(), Value.size());
		}
		Variant Var::Binary(const unsigned char* Value, size_t Size)
		{
			return Binary((const char*)Value, Size);
		}
		Variant Var::Binary(const char* Value, size_t Size)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			Variant Result(VarType::Binary);
			Result.Length = Size;

			size_t StringSize = sizeof(char) * (Result.Length + 1);
			if (Result.Length > Variant::GetMaxSmallStringSize())
				Result.Value.Pointer = VI_MALLOC(char, StringSize);

			char* Data = (char*)Result.GetString();
			memcpy(Data, Value, StringSize - sizeof(char));
			Data[StringSize - 1] = '\0';

			return Result;
		}
		Variant Var::Integer(int64_t Value)
		{
			Variant Result(VarType::Integer);
			Result.Value.Integer = Value;
			return Result;
		}
		Variant Var::Number(double Value)
		{
			Variant Result(VarType::Number);
			Result.Value.Number = Value;
			return Result;
		}
		Variant Var::Decimal(const Core::Decimal& Value)
		{
			Core::Decimal* Buffer = VI_NEW(Core::Decimal, Value);
			Variant Result(VarType::Decimal);
			Result.Value.Pointer = (char*)Buffer;
			return Result;
		}
		Variant Var::Decimal(Core::Decimal&& Value)
		{
			Core::Decimal* Buffer = VI_NEW(Core::Decimal, std::move(Value));
			Variant Result(VarType::Decimal);
			Result.Value.Pointer = (char*)Buffer;
			return Result;
		}
		Variant Var::DecimalString(const Core::String& Value)
		{
			Core::Decimal* Buffer = VI_NEW(Core::Decimal, Value);
			Variant Result(VarType::Decimal);
			Result.Value.Pointer = (char*)Buffer;
			return Result;
		}
		Variant Var::Boolean(bool Value)
		{
			Variant Result(VarType::Boolean);
			Result.Value.Boolean = Value;
			return Result;
		}

		UnorderedSet<uint64_t> Composer::Fetch(uint64_t Id) noexcept
		{
			VI_ASSERT(Context != nullptr, "composer should be initialized");
			UnorderedSet<uint64_t> Hashes;
			for (auto& Item : Context->Factory)
			{
				if (Item.second.first == Id)
					Hashes.insert(Item.first);
			}

			return Hashes;
		}
		bool Composer::Pop(const String& Hash) noexcept
		{
			VI_ASSERT(Context != nullptr, "composer should be initialized");
			VI_TRACE("[composer] pop %s", Hash.c_str());

			auto It = Context->Factory.find(VI_HASH(Hash));
			if (It == Context->Factory.end())
				return false;

			Context->Factory.erase(It);
			return true;
		}
		void Composer::Cleanup() noexcept
		{
			VI_DELETE(State, Context);
			Context = nullptr;
		}
		void Composer::Push(uint64_t TypeId, uint64_t Tag, void* Callback) noexcept
		{
			VI_TRACE("[composer] push type %" PRIu64 " tagged as %" PRIu64, TypeId, Tag);
			if (!Context)
				Context = VI_NEW(State);

			if (Context->Factory.find(TypeId) == Context->Factory.end())
				Context->Factory[TypeId] = std::make_pair(Tag, Callback);
		}
		void* Composer::Find(uint64_t TypeId) noexcept
		{
			VI_ASSERT(Context != nullptr, "composer should be initialized");
			auto It = Context->Factory.find(TypeId);
			if (It != Context->Factory.end())
				return It->second.second;

			return nullptr;
		}
		Composer::State* Composer::Context = nullptr;

		Console::Console() noexcept : Status(Mode::Detached), Colors(true)
		{
			BaseTokens =
			{
				ColorToken(StdColor::DarkGreen, "OK"),
				ColorToken(StdColor::Yellow, "execute"),
				ColorToken(StdColor::Yellow, "spawn"),
				ColorToken(StdColor::Yellow, "acquire"),
				ColorToken(StdColor::Yellow, "release"),
				ColorToken(StdColor::Yellow, "join"),
				ColorToken(StdColor::Yellow, "bind"),
				ColorToken(StdColor::Yellow, "assign"),
				ColorToken(StdColor::Yellow, "resolve"),
				ColorToken(StdColor::Yellow, "prepare"),
				ColorToken(StdColor::Yellow, "listen"),
				ColorToken(StdColor::Yellow, "unlisten"),
				ColorToken(StdColor::Yellow, "accept"),
				ColorToken(StdColor::Yellow, "load"),
				ColorToken(StdColor::Yellow, "save"),
				ColorToken(StdColor::Yellow, "open"),
				ColorToken(StdColor::Yellow, "close"),
				ColorToken(StdColor::Yellow, "create"),
				ColorToken(StdColor::Yellow, "remove"),
				ColorToken(StdColor::Yellow, "compile"),
				ColorToken(StdColor::Yellow, "transpile"),
				ColorToken(StdColor::Yellow, "enter"),
				ColorToken(StdColor::Yellow, "exit"),
				ColorToken(StdColor::Yellow, "connect"),
				ColorToken(StdColor::Yellow, "reconnect"),
				ColorToken(StdColor::Yellow, "ASSERT"),
				ColorToken(StdColor::DarkRed, "ERR"),
				ColorToken(StdColor::DarkRed, "FATAL"),
				ColorToken(StdColor::DarkRed, "PANIC!"),
				ColorToken(StdColor::DarkRed, "leak"),
				ColorToken(StdColor::DarkRed, "leaking"),
				ColorToken(StdColor::DarkRed, "fail"),
				ColorToken(StdColor::DarkRed, "failure"),
				ColorToken(StdColor::DarkRed, "failed"),
				ColorToken(StdColor::DarkRed, "error"),
				ColorToken(StdColor::DarkRed, "errors"),
				ColorToken(StdColor::DarkRed, "not"),
				ColorToken(StdColor::DarkRed, "cannot"),
				ColorToken(StdColor::DarkRed, "could"),
				ColorToken(StdColor::DarkRed, "couldn't"),
				ColorToken(StdColor::DarkRed, "wasn't"),
				ColorToken(StdColor::DarkRed, "took"),
				ColorToken(StdColor::DarkRed, "missing"),
				ColorToken(StdColor::DarkRed, "invalid"),
				ColorToken(StdColor::DarkRed, "required"),
				ColorToken(StdColor::DarkRed, "already"),
				ColorToken(StdColor::Cyan, "undefined"),
				ColorToken(StdColor::Cyan, "nullptr"),
				ColorToken(StdColor::Cyan, "null"),
				ColorToken(StdColor::Cyan, "this"),
				ColorToken(StdColor::Cyan, "ms"),
				ColorToken(StdColor::Cyan, "us"),
				ColorToken(StdColor::Cyan, "ns"),
				ColorToken(StdColor::Cyan, "on"),
				ColorToken(StdColor::Cyan, "from"),
				ColorToken(StdColor::Cyan, "to"),
				ColorToken(StdColor::Cyan, "for"),
				ColorToken(StdColor::Cyan, "and"),
				ColorToken(StdColor::Cyan, "or"),
				ColorToken(StdColor::Cyan, "at"),
				ColorToken(StdColor::Cyan, "in"),
				ColorToken(StdColor::Cyan, "of"),
				ColorToken(StdColor::Magenta, "query"),
				ColorToken(StdColor::Magenta, "vcall"),
				ColorToken(StdColor::Blue, "template"),
			};
			Tokens = BaseTokens;
		}
		Console::~Console() noexcept
		{
			Deallocate();
		}
		void Console::Allocate()
		{
			if (Status != Mode::Detached)
				return;
#ifdef VI_MICROSOFT
			if (AllocConsole())
			{
				Streams.Input = freopen("conin$", "r", stdin);
				Streams.Output = freopen("conout$", "w", stdout);
				Streams.Errors = freopen("conout$", "w", stderr);
			}

			CONSOLE_SCREEN_BUFFER_INFO ScreenBuffer;
			SetConsoleCtrlHandler(ConsoleEventHandler, true);

			HANDLE Base = GetStdHandle(STD_OUTPUT_HANDLE);
			if (GetConsoleScreenBufferInfo(Base, &ScreenBuffer))
				Cache.Attributes = ScreenBuffer.wAttributes;

			VI_TRACE("[console] allocate window 0x%" PRIXPTR, (void*)Base);
#endif
			Status = Mode::Allocated;
		}
		void Console::Deallocate()
		{
			if (Status != Mode::Allocated)
				return;
#ifdef VI_MICROSOFT
			::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#if 0
			FreeConsole();
#endif
			VI_TRACE("[console] deallocate window");
#endif
			Status = Mode::Detached;
		}
		void Console::Hide()
		{
#ifdef VI_MICROSOFT
			VI_TRACE("[console] hide window");
			::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif
		}
		void Console::Show()
		{
			Allocate();
#ifdef VI_MICROSOFT
			::ShowWindow(::GetConsoleWindow(), SW_SHOW);
			VI_TRACE("[console] show window");
#endif
		}
		void Console::Clear()
		{
#ifdef VI_MICROSOFT
			HANDLE Wnd = GetStdHandle(STD_OUTPUT_HANDLE);

			CONSOLE_SCREEN_BUFFER_INFO Info;
			GetConsoleScreenBufferInfo((HANDLE)Wnd, &Info);

			COORD TopLeft = { 0, 0 };
			DWORD Written;
			FillConsoleOutputCharacterA((HANDLE)Wnd, ' ', Info.dwSize.X * Info.dwSize.Y, TopLeft, &Written);
			FillConsoleOutputAttribute((HANDLE)Wnd, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE, Info.dwSize.X * Info.dwSize.Y, TopLeft, &Written);
			SetConsoleCursorPosition((HANDLE)Wnd, TopLeft);
#else
			int ExitCode = std::system("clear");
			(void)ExitCode;
#endif
		}
		void Console::Attach()
		{
			if (Status != Mode::Detached)
				return;
#ifdef VI_MICROSOFT
			CONSOLE_SCREEN_BUFFER_INFO ScreenBuffer;
			SetConsoleCtrlHandler(ConsoleEventHandler, true);

			HANDLE Base = GetStdHandle(STD_OUTPUT_HANDLE);
			if (GetConsoleScreenBufferInfo(Base, &ScreenBuffer))
				Cache.Attributes = ScreenBuffer.wAttributes;

			VI_TRACE("[console] attach window 0x%" PRIXPTR, (void*)Base);
#endif
			Status = Mode::Attached;
		}
		void Console::Detach()
		{
			Status = Mode::Detached;
		}
		void Console::Flush()
		{
			std::cout.flush();
		}
		void Console::FlushWrite()
		{
			std::cout << std::flush;
		}
		void Console::Trace(uint32_t MaxFrames)
		{
			std::cout << ErrorHandling::GetStackTrace(0, MaxFrames) << '\n';
		}
		void Console::CaptureTime()
		{
			Cache.Time = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
		}
		void Console::SetColoring(bool Enabled)
		{
			Colors = Enabled;
		}
		void Console::SetCursor(uint32_t X, uint32_t Y)
		{
#ifdef VI_MICROSOFT
			HANDLE Wnd = GetStdHandle(STD_OUTPUT_HANDLE);
			COORD Position = { (short)X, (short)Y };

			SetConsoleCursorPosition(Wnd, Position);
#else
			printf("\033[%d;%dH", X, Y);
#endif
		}
		void Console::SetColorTokens(Vector<Console::ColorToken>&& AdditionalTokens)
		{
			Tokens = std::move(AdditionalTokens);
			Tokens.reserve(Tokens.size() + BaseTokens.size());
			Tokens.insert(Tokens.end(), BaseTokens.begin(), BaseTokens.end());
		}
		void Console::ColorBegin(StdColor Text, StdColor Background)
		{
			if (!Colors)
				return;
#if defined(_WIN32)
			if (Background == StdColor::Zero)
				Background = StdColor::Black;

			if (Text == StdColor::Zero)
				Text = StdColor::White;
			
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (int)Background << 4 | (int)Text);
#else
			std::cout << "\033[" << GetColorId(Text, false) << ";" << GetColorId(Background, true) << "m";
#endif
		}
		void Console::ColorEnd()
		{
			if (!Colors)
				return;
#if defined(_WIN32)
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Cache.Attributes);
#else
			std::cout << "\033[0m";
#endif
		}
		void Console::ColorPrint(StdColor BaseColor, const String& Buffer)
		{
			ColorPrintBuffer(BaseColor, Buffer.c_str(), Buffer.size());
		}
		void Console::ColorPrintBuffer(StdColor BaseColor, const char* Buffer, size_t Size)
		{
			VI_ASSERT(Buffer != nullptr, "buffer should be set");
			if (!Size)
				return;

			size_t Offset = 0;
			ColorBegin(BaseColor);
			while (Buffer[Offset] != '\0')
			{
				auto& V = Buffer[Offset];
				if (Stringify::IsDigit(V))
				{
					ColorBegin(StdColor::Yellow);
					while (Offset < Size)
					{
						auto N = std::tolower(Buffer[Offset]);
						if (!Stringify::IsDigit(N) && N != '.' && N != 'a' && N != 'b' && N != 'c' && N != 'd' && N != 'e' && N != 'f' && N != 'x')
							break;

						WriteChar(Buffer[Offset++]);
					}

					ColorBegin(BaseColor);
					continue;
				}
				else if (V == '@')
				{
					ColorBegin(StdColor::LightBlue);
					WriteChar(V);

					while (Offset < Size && (Stringify::IsDigit(Buffer[++Offset]) || Stringify::IsAlphabetic(Buffer[Offset]) || Buffer[Offset] == '-' || Buffer[Offset] == '_'))
						WriteChar(Buffer[Offset]);

					ColorBegin(BaseColor);
					continue;
				}
				else if (V == '[' && strstr(Buffer + Offset + 1, "]") != nullptr)
				{
					size_t Iterations = 0, Skips = 0;
					ColorBegin(StdColor::Cyan);
					do
					{
						WriteChar(Buffer[Offset]);
						if (Iterations++ > 0 && Buffer[Offset] == '[')
							Skips++;
					} while (Offset < Size && (Buffer[Offset++] != ']' || Skips > 0));

					ColorBegin(BaseColor);
					continue;
				}
				else if (V == '\"' && strstr(Buffer + Offset + 1, "\"") != nullptr)
				{
					ColorBegin(StdColor::LightBlue);
					do
					{
						WriteChar(Buffer[Offset]);
					} while (Offset < Size && Buffer[++Offset] != '\"');

					if (Offset < Size)
						WriteChar(Buffer[Offset++]);
					ColorBegin(BaseColor);
					continue;
				}
				else if (V == '\'' && strstr(Buffer + Offset + 1, "\'") != nullptr)
				{
					ColorBegin(StdColor::LightBlue);
					do
					{
						WriteChar(Buffer[Offset]);
					} while (Offset < Size && Buffer[++Offset] != '\'');

					if (Offset < Size)
						WriteChar(Buffer[Offset++]);
					ColorBegin(BaseColor);
					continue;
				}
				else if (Stringify::IsAlphabetic(V) && (!Offset || !Stringify::IsAlphabetic(Buffer[Offset - 1])))
				{
					bool IsMatched = false;
					for (auto& Token : Tokens)
					{
						if (V != Token.First || Size - Offset < Token.Size)
							continue;

						if (Offset + Token.Size < Size && Stringify::IsAlphabetic(Buffer[Offset + Token.Size]))
							continue;

						if (memcmp(Buffer + Offset, Token.Token, Token.Size) == 0)
						{
							ColorBegin(Token.Color);
							for (size_t j = 0; j < Token.Size; j++)
								WriteChar(Buffer[Offset++]);

							ColorBegin(BaseColor);
							IsMatched = true;
							break;
						}
					}

					if (IsMatched)
						continue;
				}

				WriteChar(V);
				++Offset;
			}
		}
		void Console::WriteBuffer(const char* Buffer)
		{
			VI_ASSERT(Buffer != nullptr, "buffer should be set");
			std::cout << Buffer;
		}
		void Console::WriteLine(const String& Line)
		{
			std::cout << Line << '\n';
		}
		void Console::WriteChar(char Value)
		{
			std::cout << Value;
		}
		void Console::Write(const String& Line)
		{
			std::cout << Line;
		}
		void Console::fWriteLine(const char* Format, ...)
		{
			VI_ASSERT(Format != nullptr, "format should be set");
			char Buffer[BLOB_SIZE] = { '\0' };
			va_list Args;
			va_start(Args, Format);
#ifdef VI_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);

			std::cout << Buffer << '\n';
		}
		void Console::fWrite(const char* Format, ...)
		{
			VI_ASSERT(Format != nullptr, "format should be set");
			char Buffer[BLOB_SIZE] = { '\0' };
			va_list Args;
			va_start(Args, Format);
#ifdef VI_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);

			std::cout << Buffer;
		}
		void Console::sWriteLine(const String& Line)
		{
			UMutex<std::recursive_mutex> Unique(Session);
			std::cout << Line << '\n';
		}
		void Console::sWrite(const String& Line)
		{
			UMutex<std::recursive_mutex> Unique(Session);
			std::cout << Line;
		}
		void Console::sfWriteLine(const char* Format, ...)
		{
			VI_ASSERT(Format != nullptr, "format should be set");
			char Buffer[BLOB_SIZE] = { '\0' };
			va_list Args;
			va_start(Args, Format);
#ifdef VI_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);

			UMutex<std::recursive_mutex> Unique(Session);
			std::cout << Buffer << '\n';
		}
		void Console::sfWrite(const char* Format, ...)
		{
			VI_ASSERT(Format != nullptr, "format should be set");
			char Buffer[BLOB_SIZE] = { '\0' };
			va_list Args;
			va_start(Args, Format);
#ifdef VI_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);

			UMutex<std::recursive_mutex> Unique(Session);
			std::cout << Buffer;
		}
		void Console::Size(uint32_t* Width, uint32_t* Height)
		{
#ifdef VI_MICROSOFT
			CONSOLE_SCREEN_BUFFER_INFO Size;
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &Size);

			if (Width != nullptr)
				*Width = (uint32_t)(Size.srWindow.Right - Size.srWindow.Left + 1);

			if (Height != nullptr)
				*Height = (uint32_t)(Size.srWindow.Bottom - Size.srWindow.Top + 1);
#else
			struct winsize Size;
			ioctl(STDOUT_FILENO, TIOCGWINSZ, &Size);

			if (Width != nullptr)
				*Width = (uint32_t)Size.ws_col;

			if (Height != nullptr)
				*Height = (uint32_t)Size.ws_row;
#endif
		}
		double Console::GetCapturedTime() const
		{
			return (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0 - Cache.Time;
		}
		bool Console::ReadLine(String& Data, size_t Size)
		{
			VI_ASSERT(Status != Mode::Detached, "console should be shown at least once");
			VI_ASSERT(Size > 0, "read length should be greater than zero");
			VI_TRACE("[console] read up to %" PRIu64 " bytes", (uint64_t)Size);

			bool Success;
			if (Size > CHUNK_SIZE - 1)
			{
				char* Value = VI_MALLOC(char, sizeof(char) * (Size + 1));
				memset(Value, 0, (Size + 1) * sizeof(char));
				if ((Success = (bool)std::cin.getline(Value, Size)))
					Data.assign(Value);
				else
					Data.clear();
				VI_FREE(Value);
			}
			else
			{
				char Value[CHUNK_SIZE] = { 0 };
				if ((Success = (bool)std::cin.getline(Value, Size)))
					Data.assign(Value);
				else
					Data.clear();
			}

			return Success;
		}
		String Console::Read(size_t Size)
		{
			String Data;
			Data.reserve(Size);
			ReadLine(Data, Size);
			return Data;
		}
		char Console::ReadChar()
		{
			return (char)getchar();
		}
		bool Console::IsAvailable()
		{
			return HasInstance() && Get()->Status != Mode::Detached;
		}

		static float UnitsToSeconds = 1000000.0f;
		static float UnitsToMills = 1000.0f;
		Timer::Timer() noexcept
		{
			Reset();
		}
		void Timer::SetFixedFrames(float Value)
		{
			FixedFrames = Value;
			if (FixedFrames > 0.0)
				MaxDelta = ToUnits(1.0f / FixedFrames);
		}
		void Timer::SetMaxFrames(float Value)
		{
			MaxFrames = Value;
			if (MaxFrames > 0.0)
				MinDelta = ToUnits(1.0f / MaxFrames);
		}
		void Timer::Reset()
		{
			Timing.Begin = Clock();
			Timing.When = Timing.Begin;
			Timing.Delta = Units(0);
			Timing.Frame = 0;
			Fixed.When = Timing.Begin;
			Fixed.Delta = Units(0);
			Fixed.Sum = Units(0);
			Fixed.Frame = 0;
			Fixed.InFrame = false;
		}
		void Timer::Begin()
		{
			Units Time = Clock();
			Timing.Delta = Time - Timing.When;
			++Timing.Frame;

			if (FixedFrames <= 0.0)
				return;

			Fixed.Sum += Timing.Delta;
			Fixed.InFrame = Fixed.Sum > MaxDelta;
			if (Fixed.InFrame)
			{
				Fixed.Sum = Units(0);
				Fixed.Delta = Time - Fixed.When;
				Fixed.When = Time;
				++Fixed.Frame;
			}
		}
		void Timer::Finish()
		{
			Timing.When = Clock();
			if (MaxFrames > 0.0 && Timing.Delta < MinDelta)
				std::this_thread::sleep_for(MinDelta - Timing.Delta);
		}
		void Timer::Push(const char* Name)
		{
			Capture Next = { Name, Clock() };
			Captures.emplace(std::move(Next));
		}
		bool Timer::PopIf(float GreaterThan, Capture* Out)
		{
			Capture Data = Pop();
			bool Overdue = (Data.Step > GreaterThan);
			if (Out != nullptr)
				*Out = std::move(Data);

			return Overdue;
		}
		bool Timer::IsFixed() const
		{
			return Fixed.InFrame;
		}
		Timer::Capture Timer::Pop()
		{
			VI_ASSERT(!Captures.empty(), "there is no time captured at the moment");
			Capture Base = Captures.front();
			Base.End = Clock();
			Base.Delta = Base.End - Base.Begin;
			Base.Step = ToSeconds(Base.Delta);
			Captures.pop();

			return Base;
		}
		size_t Timer::GetFrameIndex() const
		{
			return Timing.Frame;
		}
		size_t Timer::GetFixedFrameIndex() const
		{
			return Fixed.Frame;
		}
		float Timer::GetMaxFrames() const
		{
			return MaxFrames;
		}
		float Timer::GetMinStep() const
		{
			if (MaxFrames <= 0.0f)
				return 0.0f;

			return ToSeconds(MinDelta);
		}
		float Timer::GetFrames() const
		{
			return 1.0f / ToSeconds(Timing.Delta);
		}
		float Timer::GetElapsed() const
		{
			return ToSeconds(Clock() - Timing.Begin);
		}
		float Timer::GetElapsedMills() const
		{
			return ToMills(Clock() - Timing.Begin);
		}
		float Timer::GetStep() const
		{
			return ToSeconds(Timing.Delta);
		}
		float Timer::GetFixedStep() const
		{
			return ToSeconds(Fixed.Delta);
		}
		float Timer::GetFixedFrames() const
		{
			return FixedFrames;
		}
		float Timer::ToSeconds(const Units& Value)
		{
			return (float)((double)Value.count() / UnitsToSeconds);
		}
		float Timer::ToMills(const Units& Value)
		{
			return (float)((double)Value.count() / UnitsToMills);
		}
		Timer::Units Timer::ToUnits(float Value)
		{
			return Units((uint64_t)(Value * UnitsToSeconds));
		}
		Timer::Units Timer::Clock()
		{
			return std::chrono::duration_cast<Units>(std::chrono::system_clock::now().time_since_epoch());
		}

		Stream::Stream() noexcept : VSize(0)
		{
		}
		void Stream::SetVirtualSize(size_t Size)
		{
			VSize = 0;
		}
		size_t Stream::ReadAll(const std::function<void(char*, size_t)>& Callback)
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			VI_TRACE("[io] read all bytes on fd %i", GetFd());

			char Buffer[CHUNK_SIZE];
			size_t Size = 0, Total = 0;

			do
			{
				size_t Max = sizeof(Buffer);
				if (VSize > 0 && VSize - Total > Max)
					Max = VSize - Total;

				Size = Read(Buffer, Max);
				if (Size > 0)
				{
					Callback(Buffer, Size);
					Total += Size;
				}
			} while (Size > 0);

			return Size;
		}
		size_t Stream::VirtualSize() const
		{
			return VSize;
		}
		size_t Stream::Size()
		{
			if (!IsSized())
				return 0;

			size_t Position = Tell();
			Seek(FileSeek::End, 0);
			size_t Size = Tell();
			Seek(FileSeek::Begin, Position);

			return Size;
		}
		String& Stream::Source()
		{
			return Path;
		}

		FileStream::FileStream() noexcept : Resource(nullptr)
		{
		}
		FileStream::~FileStream() noexcept
		{
			Close();
		}
		ExpectsIO<void> FileStream::Clear()
		{
			VI_TRACE("[io] fs %i clear", GetFd());
			auto Result = Close();
			if (!Result)
				return Result;

			if (Path.empty())
				return std::make_error_condition(std::errc::invalid_argument);

			auto Target = OS::File::Open(Path.c_str(), "w");
			if (!Target)
				return Target.Error();
			
			Resource = *Target;
			return Optional::OK;
		}
		ExpectsIO<void> FileStream::Open(const char* File, FileMode Mode)
		{
			VI_ASSERT(File != nullptr, "filename should be set");
			VI_MEASURE(Timings::FileSystem);
			auto Result = Close();
			if (!Result)
				return Result;

			const char* Type = nullptr;
			switch (Mode)
			{
				case FileMode::Read_Only:
					Type = "r";
					break;
				case FileMode::Write_Only:
					Type = "w";
					break;
				case FileMode::Append_Only:
					Type = "a";
					break;
				case FileMode::Read_Write:
					Type = "r+";
					break;
				case FileMode::Write_Read:
					Type = "w+";
					break;
				case FileMode::Read_Append_Write:
					Type = "a+";
					break;
				case FileMode::Binary_Read_Only:
					Type = "rb";
					break;
				case FileMode::Binary_Write_Only:
					Type = "wb";
					break;
				case FileMode::Binary_Append_Only:
					Type = "ab";
					break;
				case FileMode::Binary_Read_Write:
					Type = "rb+";
					break;
				case FileMode::Binary_Write_Read:
					Type = "wb+";
					break;
				case FileMode::Binary_Read_Append_Write:
					Type = "ab+";
					break;
				default:
					break;
			} 

			VI_PANIC(Type != nullptr, "file open cannot be issued with mode:%i", (int)Mode);
			ExpectsIO<String> TargetPath = OS::Path::Resolve(File);
			if (!TargetPath)
				return TargetPath.Error();

			auto Target = OS::File::Open(TargetPath->c_str(), Type);
			if (!Target)
				return Target.Error();

			Resource = *Target;
			Path = *TargetPath;
			return Optional::OK;
		}
		ExpectsIO<void> FileStream::Close()
		{
			if (!Resource)
				return Optional::OK;

			void* Target = Resource;
			Resource = nullptr;
			return OS::File::Close(Target);
		}
		ExpectsIO<void> FileStream::Seek(FileSeek Mode, int64_t Offset)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_MEASURE(Timings::FileSystem);

			switch (Mode)
			{
				case FileSeek::Begin:
					VI_TRACE("[io] seek fs %i begin %" PRId64, GetFd(), Offset);
					if (fseek(Resource, (long)Offset, SEEK_SET) != 0)
						return OS::Error::GetConditionOr();
					return Optional::OK;
				case FileSeek::Current:
					VI_TRACE("[io] seek fs %i move %" PRId64, GetFd(), Offset);
					if (fseek(Resource, (long)Offset, SEEK_CUR) != 0)
						return OS::Error::GetConditionOr();
					return Optional::OK;
				case FileSeek::End:
					VI_TRACE("[io] seek fs %i end %" PRId64, GetFd(), Offset);
					if (fseek(Resource, (long)Offset, SEEK_END) != 0)
						return OS::Error::GetConditionOr();
					return Optional::OK;
				default:
					return std::make_error_condition(std::errc::invalid_argument);
			}
		}
		ExpectsIO<void> FileStream::Move(int64_t Offset)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] seek fs %i move %" PRId64, GetFd(), Offset);
			if (fseek(Resource, (long)Offset, SEEK_CUR) != 0)
				return OS::Error::GetConditionOr();
			return Optional::OK;
		}
		ExpectsIO<void> FileStream::Flush()
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] flush fs %i", GetFd());
			if (fflush(Resource) != 0)
				return OS::Error::GetConditionOr();
			return Optional::OK;
		}
		size_t FileStream::ReadAny(const char* Format, ...)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_ASSERT(Format != nullptr, "format should be set");
			VI_MEASURE(Timings::FileSystem);

			va_list Args;
			va_start(Args, Format);
			size_t R = (size_t)vfscanf(Resource, Format, Args);
			va_end(Args);

			VI_TRACE("[io] fs %i scan %i bytes", GetFd(), (int)R);
			return R;
		}
		size_t FileStream::Read(char* Data, size_t Length)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] fs %i read %i bytes", GetFd(), (int)Length);
			return fread(Data, 1, Length, Resource);
		}
		size_t FileStream::WriteAny(const char* Format, ...)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_ASSERT(Format != nullptr, "format should be set");
			VI_MEASURE(Timings::FileSystem);

			va_list Args;
			va_start(Args, Format);
			size_t R = (size_t)vfprintf(Resource, Format, Args);
			va_end(Args);

			VI_TRACE("[io] fs %i print %i bytes", GetFd(), (int)R);
			return R;
		}
		size_t FileStream::Write(const char* Data, size_t Length)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] fs %i write %i bytes", GetFd(), (int)Length);
			return fwrite(Data, 1, Length, Resource);
		}
		size_t FileStream::Tell()
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] fs %i tell", GetFd());
			return ftell(Resource);
		}
		socket_t FileStream::GetFd() const
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			return (socket_t)VI_FILENO(Resource);
		}
		void* FileStream::GetResource() const
		{
			return (void*)Resource;
		}
		bool FileStream::IsSized() const
		{
			return true;
		}

		GzStream::GzStream() noexcept : Resource(nullptr)
		{
		}
		GzStream::~GzStream() noexcept
		{
			Close();
		}
		ExpectsIO<void> GzStream::Clear()
		{
			VI_TRACE("[gz] fs %i clear", GetFd());
			auto Result = Close();
			if (!Result)
				return Result;

			if (Path.empty())
				return std::make_error_condition(std::errc::invalid_argument);

			auto Target = OS::File::Open(Path.c_str(), "w");
			if (!Target)
				return Target.Error();

			Result = OS::File::Close(*Target);
			if (!Result)
				return Result;

			return Open(Path.c_str(), FileMode::Binary_Write_Only);
		}
		ExpectsIO<void> GzStream::Open(const char* File, FileMode Mode)
		{
			VI_ASSERT(File != nullptr, "filename should be set");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			auto Result = Close();
			if (!Result)
				return Result;

			const char* Type = nullptr;
			switch (Mode)
			{
				case FileMode::Binary_Read_Only:
				case FileMode::Read_Only:
					Type = "rb";
					break;
				case FileMode::Binary_Write_Only:
				case FileMode::Write_Only:
					Type = "wb";
					break;
				default:
					break;
			}

			VI_PANIC(Type != nullptr, "file open cannot be issued with mode:%i", (int)Mode);
			ExpectsIO<String> TargetPath = OS::Path::Resolve(File);
			if (!TargetPath)
				return TargetPath.Error();

			VI_DEBUG("[gz] open %s %s", Type, TargetPath->c_str());
			Resource = gzopen(TargetPath->c_str(), Type);
			if (!Resource)
				return std::make_error_condition(std::errc::no_such_file_or_directory);

			Path = *TargetPath;
			return Optional::OK;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> GzStream::Close()
		{
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			if (!Resource)
				return Optional::OK;

			VI_DEBUG("[gz] close 0x%" PRIXPTR, (uintptr_t)Resource);
			if (gzclose((gzFile)Resource) != Z_OK)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			Resource = nullptr;
			return Optional::OK;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> GzStream::Seek(FileSeek Mode, int64_t Offset)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			switch (Mode)
			{
				case FileSeek::Begin:
					VI_TRACE("[gz] seek fs %i begin %" PRId64, GetFd(), Offset);
					if (gzseek((gzFile)Resource, (long)Offset, SEEK_SET) != 0)
						return OS::Error::GetConditionOr();
					return Optional::OK;
				case FileSeek::Current:
					VI_TRACE("[gz] seek fs %i move %" PRId64, GetFd(), Offset);
					if (gzseek((gzFile)Resource, (long)Offset, SEEK_CUR) != 0)
						return OS::Error::GetConditionOr();
					return Optional::OK;
				case FileSeek::End:
					return std::make_error_condition(std::errc::not_supported);
				default:
					return std::make_error_condition(std::errc::invalid_argument);
			}
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> GzStream::Move(int64_t Offset)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[gz] seek fs %i move %" PRId64, GetFd(), Offset);
			if (gzseek((gzFile)Resource, (long)Offset, SEEK_CUR) != 0)
				return OS::Error::GetConditionOr();
			return Optional::OK;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> GzStream::Flush()
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			if (gzflush((gzFile)Resource, Z_SYNC_FLUSH) != Z_OK)
				return OS::Error::GetConditionOr();
			return Optional::OK;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		size_t GzStream::ReadAny(const char* Format, ...)
		{
			VI_ASSERT(false, "gz read-format is not supported");
			return 0;
		}
		size_t GzStream::Read(char* Data, size_t Length)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[gz] fs %i read %i bytes", GetFd(), (int)Length);
			return gzread((gzFile)Resource, Data, (unsigned int)Length);
#else
			return 0;
#endif
		}
		size_t GzStream::WriteAny(const char* Format, ...)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_ASSERT(Format != nullptr, "format should be set");
			VI_MEASURE(Timings::FileSystem);

			va_list Args;
			va_start(Args, Format);
#ifdef VI_ZLIB
			size_t R = (size_t)gzvprintf((gzFile)Resource, Format, Args);
#else
			size_t R = 0;
#endif
			va_end(Args);

			VI_TRACE("[gz] fs %i print %i bytes", GetFd(), (int)R);
			return R;
		}
		size_t GzStream::Write(const char* Data, size_t Length)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[gz] fs %i write %i bytes", GetFd(), (int)Length);
			return gzwrite((gzFile)Resource, Data, (unsigned int)Length);
#else
			return 0;
#endif
		}
		size_t GzStream::Tell()
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[gz] fs %i tell", GetFd());
			auto Value = gztell((gzFile)Resource);
			return (Value > 0 ? (size_t)Value : 0);
#else
			return 0;
#endif
		}
		socket_t GzStream::GetFd() const
		{
			VI_ASSERT(false, "gz fd fetch is not supported");
			return (socket_t)-1;
		}
		void* GzStream::GetResource() const
		{
			return Resource;
		}
		bool GzStream::IsSized() const
		{
			return false;
		}

		WebStream::WebStream(bool IsAsync) noexcept : Resource(nullptr), Offset(0), Length(0), Async(IsAsync)
		{
		}
		WebStream::WebStream(bool IsAsync, UnorderedMap<String, String>&& NewHeaders) noexcept : WebStream(IsAsync)
		{
			Headers = std::move(NewHeaders);
		}
		WebStream::~WebStream() noexcept
		{
			Close();
		}
		ExpectsIO<void> WebStream::Clear()
		{
			VI_ASSERT(false, "web clear is not supported");
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<void> WebStream::Open(const char* File, FileMode Mode)
		{
			VI_ASSERT(File != nullptr, "filename should be set");
			auto Result = Close();
			if (!Result)
				return Result;

			const char* Type = nullptr;
			switch (Mode)
			{
				case FileMode::Binary_Read_Only:
				case FileMode::Read_Only:
					Type = "rb";
					break;
				default:
					break;
			}

			VI_PANIC(Type != nullptr, "file open cannot be issued with mode:%i", (int)Mode);
			Network::Location URL(File);
			if (URL.Protocol != "http" && URL.Protocol != "https")
				return std::make_error_condition(std::errc::address_family_not_supported);

			Network::RemoteHost Address;
			Address.Hostname = URL.Hostname;
			Address.Secure = (URL.Protocol == "https");
			Address.Port = (URL.Port < 0 ? (Address.Secure ? 443 : 80) : URL.Port);

			auto* Client = new Network::HTTP::Client(30000);
			Result = Client->Connect(&Address, false).Get();
			if (!Result)
			{
				VI_RELEASE(Client);
				return Result;
			}

			Network::HTTP::RequestFrame Request;
			Request.URI.assign(URL.Path);

			for (auto& Item : URL.Query)
				Request.Query += Item.first + "=" + Item.second + "&";

			if (!Request.Query.empty())
				Request.Query.pop_back();

			for (auto& Item : Headers)
				Request.SetHeader(Item.first, Item.second);

			auto Response = Client->Send(std::move(Request)).Get();
			if (!Response || Response->StatusCode < 0)
			{
				VI_RELEASE(Client);
				return Response ? std::make_error_condition(std::errc::protocol_error) : Response.Error();
			}
			else if (Response->Content.Limited)
				Length = Response->Content.Length;

			VI_DEBUG("[http] open %s %s", Type, File);
			Resource = Client;
			Path = File;

			return Optional::OK;
		}
		ExpectsIO<void> WebStream::Close()
		{
			auto* Client = (Network::HTTP::Client*)Resource;
			Resource = nullptr;
			Offset = Length = 0;
			Chunk.clear();

			if (!Client)
				return Optional::OK;

			auto Result = Client->Disconnect().Get();
			VI_RELEASE(Client);
			return Result;
		}
		ExpectsIO<void> WebStream::Seek(FileSeek Mode, int64_t NewOffset)
		{
			switch (Mode)
			{
				case FileSeek::Begin:
					VI_TRACE("[http] seek fd %i begin %" PRId64, GetFd(), (int)NewOffset);
					Offset = NewOffset;
					return Optional::OK;
				case FileSeek::Current:
					VI_TRACE("[http] seek fd %i move %" PRId64, GetFd(), (int)NewOffset);
					if (NewOffset < 0)
					{
						size_t Pointer = (size_t)(-NewOffset);
						if (Pointer > Offset)
						{
							Pointer -= Offset;
							if (Pointer > Length)
								return std::make_error_condition(std::errc::result_out_of_range);

							Offset = Length - Pointer;
						}
					}
					else
						Offset += NewOffset;
					return Optional::OK;
				case FileSeek::End:
					VI_TRACE("[http] seek fd %i end %" PRId64, GetFd(), (int)NewOffset);
					Offset = Length - NewOffset;
					return Optional::OK;
				default:
					return std::make_error_condition(std::errc::not_supported);
			}
		}
		ExpectsIO<void> WebStream::Move(int64_t Offset)
		{
			VI_ASSERT(false, "web move is not supported");
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<void> WebStream::Flush()
		{
			VI_ASSERT(false, "web flush is not supported");
			return std::make_error_condition(std::errc::not_supported);
		}
		size_t WebStream::ReadAny(const char* Format, ...)
		{
			VI_ASSERT(false, "web read-format is not supported");
			return 0;
		}
		size_t WebStream::Read(char* Data, size_t DataLength)
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_ASSERT(DataLength > 0, "length should be greater than zero");
			VI_TRACE("[http] fd %i read %i bytes", GetFd(), (int)DataLength);

			size_t Result = 0;
			if (Offset + DataLength > Chunk.size() && (Chunk.size() < Length || (!Length && !((Network::HTTP::Client*)Resource)->GetResponse()->Content.Limited)))
			{
				auto* Client = (Network::HTTP::Client*)Resource;
				if (!Client->Consume(DataLength).Get())
					return 0;

				auto* Response = Client->GetResponse();
				if (!Length && Response->Content.Limited)
				{
					Length = Response->Content.Length;
					if (!Length)
						return 0;
				}

				if (Response->Content.Data.empty())
					return 0;

				Chunk.insert(Chunk.end(), Response->Content.Data.begin(), Response->Content.Data.end());
			}

			Result = std::min(DataLength, Chunk.size() - (size_t)Offset);
			memcpy(Data, Chunk.data() + (size_t)Offset, Result);
			Offset += (size_t)Result;
			return Result;
		}
		size_t WebStream::WriteAny(const char* Format, ...)
		{
			VI_ASSERT(false, "web write-format is not supported");
			return 0;
		}
		size_t WebStream::Write(const char* Data, size_t Length)
		{
			VI_ASSERT(false, "web write is not supported");
			return 0;
		}
		size_t WebStream::Tell()
		{
			VI_TRACE("[http] fd %i tell", GetFd());
			return (size_t)Offset;
		}
		socket_t WebStream::GetFd() const
		{
			VI_ASSERT(Resource != nullptr, "file should be opened");
			return ((Network::HTTP::Client*)Resource)->GetStream()->GetFd();
		}
		void* WebStream::GetResource() const
		{
			return Resource;
		}
		bool WebStream::IsSized() const
		{
			return true;
		}

		ProcessStream::ProcessStream() noexcept : FileStream(), ExitCode(-1)
		{
		}
		ExpectsIO<void> ProcessStream::Clear()
		{
			VI_ASSERT(false, "process clear is not supported");
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<void> ProcessStream::Open(const char* File, FileMode Mode)
		{
			VI_ASSERT(File != nullptr, "command should be set");
			auto Result = Close();
			if (!Result)
				return Result;

			const char* Type = nullptr;
			switch (Mode)
			{
				case FileMode::Read_Only:
					Type = "r";
					break;
				case FileMode::Write_Only:
					Type = "w";
					break;
				case FileMode::Append_Only:
					Type = "a";
					break;
				case FileMode::Read_Write:
					Type = "r+";
					break;
				case FileMode::Write_Read:
					Type = "w+";
					break;
				case FileMode::Read_Append_Write:
					Type = "a+";
					break;
				case FileMode::Binary_Read_Only:
					Type = "rb";
					break;
				case FileMode::Binary_Write_Only:
					Type = "wb";
					break;
				case FileMode::Binary_Append_Only:
					Type = "ab";
					break;
				case FileMode::Binary_Read_Write:
					Type = "rb+";
					break;
				case FileMode::Binary_Write_Read:
					Type = "wb+";
					break;
				case FileMode::Binary_Read_Append_Write:
					Type = "ab+";
					break;
				default:
					break;
			}

			VI_PANIC(Type != nullptr, "file open cannot be issued with mode:%i", (int)Mode);
			Resource = OpenPipe(File, Type);
			if (!Resource)
				return OS::Error::GetConditionOr();

			Path = File;
			return Optional::OK;
		}
		ExpectsIO<void> ProcessStream::Close()
		{
			if (Resource != nullptr)
			{
				ExitCode = ClosePipe(Resource);
				Resource = nullptr;
			}

			return Optional::OK;
		}
		bool ProcessStream::IsSized() const
		{
			return false;
		}
		int ProcessStream::GetExitCode() const
		{
			return ExitCode;
		}
		FILE* ProcessStream::OpenPipe(const char* Path, const char* Mode)
		{
			VI_MEASURE(Timings::FileSystem);
			VI_ASSERT(Path != nullptr && Mode != nullptr, "path and mode should be set");
#ifdef VI_MICROSOFT
			wchar_t Buffer[CHUNK_SIZE], Type[20];
			Stringify::ConvertToWide(Path, strlen(Path), Buffer, CHUNK_SIZE);
			Stringify::ConvertToWide(Mode, strlen(Mode), Type, 20);

			FILE* Stream = _wpopen(Buffer, Type);
			if (Stream != nullptr)
				VI_DEBUG("[io] open ps %i %s %s", VI_FILENO(Stream), Mode, Path);

			return Stream;
#elif defined(VI_LINUX)
			FILE* Stream = popen(Path, Mode);
			if (Stream != nullptr)
				fcntl(VI_FILENO(Stream), F_SETFD, FD_CLOEXEC);

			if (Stream != nullptr)
				VI_DEBUG("[io] open ps %i %s %s", VI_FILENO(Stream), Mode, Path);

			return Stream;
#else
			return nullptr;
#endif
		}
		int ProcessStream::ClosePipe(void* Fd)
		{
			VI_ASSERT(Fd != nullptr, "stream should be set");
			VI_DEBUG("[io] close ps %i", VI_FILENO((FILE*)Fd));
#ifdef VI_MICROSOFT
			return _pclose((FILE*)Fd);
#elif defined(VI_LINUX)
			return pclose((FILE*)Fd);
#else
			return -1;
#endif
		}

		FileTree::FileTree(const String& Folder) noexcept
		{
			auto Target = OS::Path::Resolve(Folder.c_str());
			if (Target)
			{
				Vector<std::pair<String, FileEntry>> Entries;
				if (!OS::Directory::Scan(Target->c_str(), &Entries))
					return;
				
				Directories.reserve(Entries.size());
				Files.reserve(Entries.size());
				Path = *Target;
				
				for (auto& Item : Entries)
				{
					if (!Item.second.IsDirectory)
						Files.emplace_back(std::move(Item.first));
					else
						Directories.push_back(new FileTree(Item.first));
				}
			}
			else
			{
				Vector<String> Drives = OS::Directory::GetMounts();
				for (auto& Drive : Drives)
					Directories.push_back(new FileTree(Drive));
			}
		}
		FileTree::~FileTree() noexcept
		{
			for (auto& Directory : Directories)
				VI_RELEASE(Directory);
		}
		void FileTree::Loop(const std::function<bool(const FileTree*)>& Callback) const
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Callback(this))
				return;

			for (auto& Directory : Directories)
				Directory->Loop(Callback);
		}
		const FileTree* FileTree::Find(const String& V) const
		{
			if (Path == V)
				return this;

			for (const auto& Directory : Directories)
			{
				const FileTree* Ref = Directory->Find(V);
				if (Ref != nullptr)
					return Ref;
			}

			return nullptr;
		}
		size_t FileTree::GetFiles() const
		{
			size_t Count = Files.size();
			for (auto& Directory : Directories)
				Count += Directory->GetFiles();

			return Count;
		}

		int64_t ChildProcess::GetPid()
		{
#ifdef VI_MICROSOFT
			return GetProcessId(Process);
#else
			return (int64_t)Process;
#endif
		}

		OS::Process::ArgsContext::ArgsContext(int Argc, char** Argv, const String& WhenNoValue) noexcept
		{
			Base = OS::Process::GetArgs(Argc, Argv, WhenNoValue);
		}
		void OS::Process::ArgsContext::ForEach(const std::function<void(const String&, const String&)>& Callback) const
		{
			VI_ASSERT(Callback != nullptr, "callback should not be empty");
			for (auto& Item : Base)
				Callback(Item.first, Item.second);
		}
		bool OS::Process::ArgsContext::IsEnabled(const String& Option, const String& Shortcut) const
		{
			auto It = Base.find(Option);
			if (It == Base.end() || !IsTrue(It->second))
				return Shortcut.empty() ? false : IsEnabled(Shortcut);

			return true;
		}
		bool OS::Process::ArgsContext::IsDisabled(const String& Option, const String& Shortcut) const
		{
			auto It = Base.find(Option);
			if (It == Base.end())
				return Shortcut.empty() ? true : IsDisabled(Shortcut);

			return IsFalse(It->second);
		}
		bool OS::Process::ArgsContext::Has(const String& Option, const String& Shortcut) const
		{
			if (Base.find(Option) != Base.end())
				return true;

			return Shortcut.empty() ? false : Base.find(Shortcut) != Base.end();
		}
		String& OS::Process::ArgsContext::Get(const String& Option, const String& Shortcut)
		{
			if (Base.find(Option) != Base.end())
				return Base[Option];

			return Shortcut.empty() ? Base[Option] : Base[Shortcut];
		}
		String& OS::Process::ArgsContext::GetIf(const String& Option, const String& Shortcut, const String& WhenEmpty)
		{
			if (Base.find(Option) != Base.end())
				return Base[Option];

			if (!Shortcut.empty() && Base.find(Shortcut) != Base.end())
				return Base[Shortcut];

			String& Value = Base[Option];
			Value = WhenEmpty;
			return Value;
		}
		String& OS::Process::ArgsContext::GetAppPath()
		{
			return Get("__path__");
		}
		bool OS::Process::ArgsContext::IsTrue(const String& Value) const
		{
			if (Value.empty())
				return false;

			auto MaybeNumber = FromString<uint64_t>(Value);
			if (MaybeNumber && *MaybeNumber > 0)
				return true;

			String Data(Value);
			Stringify::ToLower(Data);
			return Data == "on" || Data == "true" || Data == "yes" || Data == "y";
		}
		bool OS::Process::ArgsContext::IsFalse(const String& Value) const
		{
			if (Value.empty())
				return true;

			auto MaybeNumber = FromString<uint64_t>(Value);
			if (MaybeNumber && *MaybeNumber > 0)
				return false;

			String Data(Value);
			Stringify::ToLower(Data);
			return Data == "off" || Data == "false" || Data == "no" || Data == "n";
		}

		OS::CPU::QuantityInfo OS::CPU::GetQuantityInfo()
		{
			QuantityInfo Result{};
#ifdef VI_MICROSOFT
			for (auto&& Info : CPUInfoBuffer())
			{
				switch (Info.Relationship)
				{
					case RelationProcessorCore:
						++Result.Physical;
						Result.Logical += static_cast<std::uint32_t>(std::bitset<sizeof(ULONG_PTR) * 8>(static_cast<std::uintptr_t>(Info.ProcessorMask)).count());
						break;
					case RelationProcessorPackage:
						++Result.Packages;
						break;
					default:
						break;
				}
			}
#elif VI_APPLE
			const auto CtlThreadData = SysControl("machdep.cpu.thread_count");
			if (!CtlThreadData.empty())
			{
				const auto ThreadData = SysExtract(CtlThreadData);
				if (ThreadData.first)
					Result.Logical = (unsigned int)ThreadData.second;
			}

			const auto CtlCoreData = SysControl("machdep.cpu.core_count");
			if (!CtlCoreData.empty())
			{
				const auto CoreData = SysExtract(CtlCoreData);
				if (CoreData.first)
					Result.Physical = (unsigned int)CoreData.second;
			}

			const auto CtlPackagesData = SysControl("hw.packages");
			if (!CtlPackagesData.empty())
			{
				const auto PackagesData = SysExtract(CtlPackagesData);
				if (PackagesData.first)
					Result.Packages = (unsigned int)PackagesData.second;
			}
#else
			Result.Logical = sysconf(_SC_NPROCESSORS_ONLN);
			std::ifstream Info("/proc/cpuinfo");

			if (!Info.is_open() || !Info)
				return Result;

			Vector<unsigned int> Packages;
			for (String Line; std::getline(Info, Line);)
			{
				if (Line.find("physical id") == 0)
				{
					const auto PhysicalId = std::strtoul(Line.c_str() + Line.find_first_of("1234567890"), nullptr, 10);
					if (std::find(Packages.begin(), Packages.end(), PhysicalId) == Packages.end())
						Packages.emplace_back(PhysicalId);
				}
			}

			Result.Packages = Packages.size();
			Result.Physical = Result.Logical / Result.Packages;
#endif
			return Result;
		}
		OS::CPU::CacheInfo OS::CPU::GetCacheInfo(unsigned int Level)
		{
#ifdef VI_MICROSOFT
			for (auto&& Info : CPUInfoBuffer())
			{
				if (Info.Relationship != RelationCache || Info.Cache.Level != Level)
					continue;

				Cache Type{};
				switch (Info.Cache.Type)
				{
					case CacheUnified:
						Type = Cache::Unified;
						break;
					case CacheInstruction:
						Type = Cache::Instruction;
						break;
					case CacheData:
						Type = Cache::Data;
						break;
					case CacheTrace:
						Type = Cache::Trace;
						break;
				}

				return { Info.Cache.Size, Info.Cache.LineSize, Info.Cache.Associativity, Type };
			}

			return {};
#elif VI_APPLE
			static const char* SizeKeys[][3]{ {}, {"hw.l1icachesize", "hw.l1dcachesize", "hw.l1cachesize"}, {"hw.l2cachesize"}, {"hw.l3cachesize"} };
			CacheInfo Result{};

			const auto CtlCacheLineSize = SysControl("hw.cachelinesize");
			if (!CtlCacheLineSize.empty())
			{
				const auto CacheLineSize = SysExtract(CtlCacheLineSize);
				if (CacheLineSize.first)
					Result.LineSize = CacheLineSize.second;
			}

			if (Level < sizeof(SizeKeys) / sizeof(*SizeKeys))
			{
				for (auto Key : SizeKeys[Level])
				{
					if (!Key)
						break;

					const auto CtlCacheSizeData = SysControl(Key);
					if (!CtlCacheSizeData.empty())
					{
						const auto CacheSizeData = SysExtract(CtlCacheSizeData);
						if (CacheSizeData.first)
							Result.Size += CacheSizeData.second;
					}
				}
			}

			return Result;
#else
			String Prefix("/sys/devices/system/cpu/cpu0/cache/index" + Core::ToString(Level) + '/');
			String SizePath(Prefix + "size");
			String LineSizePath(Prefix + "coherency_line_size");
			String AssociativityPath(Prefix + "associativity");
			String TypePath(Prefix + "type");
			std::ifstream Size(SizePath.c_str());
			std::ifstream LineSize(LineSizePath.c_str());
			std::ifstream Associativity(AssociativityPath.c_str());
			std::ifstream Type(TypePath.c_str());
			CacheInfo Result{};

			if (Size.is_open() && Size)
			{
				char Suffix;
				Size >> Result.Size >> Suffix;
				switch (Suffix)
				{
					case 'G':
						Result.Size *= 1024;
						[[fallthrough]];
					case 'M':
						Result.Size *= 1024;
						[[fallthrough]];
					case 'K':
						Result.Size *= 1024;
				}
			}

			if (LineSize.is_open() && LineSize)
				LineSize >> Result.LineSize;

			if (Associativity.is_open() && Associativity)
			{
				unsigned int Temp;
				Associativity >> Temp;
				Result.Associativity = Temp;
			}

			if (Type.is_open() && Type)
			{
				String Temp;
				Type >> Temp;

				if (Temp.find("nified") == 1)
					Result.Type = Cache::Unified;
				else if (Temp.find("nstruction") == 1)
					Result.Type = Cache::Instruction;
				else if (Temp.find("ata") == 1)
					Result.Type = Cache::Data;
				else if (Temp.find("race") == 1)
					Result.Type = Cache::Trace;
			}

			return Result;
#endif
		}
		OS::CPU::Arch OS::CPU::GetArch() noexcept
		{
#ifndef VI_MICROSOFT
			utsname Buffer;
			if (uname(&Buffer) == -1)
				return Arch::Unknown;

			if (!strcmp(Buffer.machine, "x86_64"))
				return Arch::X64;
			else if (strstr(Buffer.machine, "arm") == Buffer.machine)
				return Arch::ARM;
			else if (!strcmp(Buffer.machine, "ia64") || !strcmp(Buffer.machine, "IA64"))
				return Arch::Itanium;
			else if (!strcmp(Buffer.machine, "i686"))
				return Arch::X86;

			return Arch::Unknown;
#else
			SYSTEM_INFO Buffer;
			GetNativeSystemInfo(&Buffer);

			switch (Buffer.wProcessorArchitecture)
			{
				case PROCESSOR_ARCHITECTURE_AMD64:
					return Arch::X64;
				case PROCESSOR_ARCHITECTURE_ARM:
				case PROCESSOR_ARCHITECTURE_ARM64:
					return Arch::ARM;
				case PROCESSOR_ARCHITECTURE_IA64:
					return Arch::Itanium;
				case PROCESSOR_ARCHITECTURE_INTEL:
					return Arch::X86;
				default:
					return Arch::Unknown;
			}
#endif
		}
		OS::CPU::Endian OS::CPU::GetEndianness() noexcept
		{
			static const uint16_t Value = 0xFF00;
			static const uint8_t Result = *static_cast<const uint8_t*>(static_cast<const void*>(&Value));

			return Result == 0xFF ? Endian::Big : Endian::Little;
		}
		size_t OS::CPU::GetFrequency() noexcept
		{
#ifdef VI_MICROSOFT
			HKEY Key;
			if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)", 0, KEY_READ, &Key))
			{
				LARGE_INTEGER Frequency;
				QueryPerformanceFrequency(&Frequency);
				return (size_t)Frequency.QuadPart * 1000;
			}

			DWORD FrequencyMHZ, Size = sizeof(DWORD);
			if (RegQueryValueExA(Key, "~MHz", nullptr, nullptr, static_cast<LPBYTE>(static_cast<void*>(&FrequencyMHZ)), &Size))
				return 0;

			return (size_t)FrequencyMHZ * 1000000;
#elif VI_APPLE
			const auto Frequency = SysControl("hw.cpufrequency");
			if (Frequency.empty())
				return 0;

			const auto Data = SysExtract(Frequency);
			if (!Data.first)
				return 0;

			return Data.second;
#else
			std::ifstream Info("/proc/cpuinfo");
			if (!Info.is_open() || !Info)
				return 0;

			for (String Line; std::getline(Info, Line);)
			{
				if (Line.find("cpu MHz") == 0)
				{
					const auto ColonId = Line.find_first_of(':');
					return static_cast<size_t>(std::strtod(Line.c_str() + ColonId + 1, nullptr)) * 1000000;
				}
			}

			return 0;
#endif
		}

		bool OS::Directory::IsExists(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] check path %s", Path);

			auto TargetPath = Path::Resolve(Path);
			if (!TargetPath)
				return false;

			struct stat Buffer;
			if (stat(TargetPath->c_str(), &Buffer) != 0)
				return false;

			return Buffer.st_mode & S_IFDIR;
		}
		ExpectsIO<void> OS::Directory::SetWorking(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
			VI_TRACE("[io] set working dir %s", Path);
#ifdef VI_MICROSOFT
			if (SetCurrentDirectoryA(Path) != TRUE)
				return OS::Error::GetConditionOr();

			return Optional::OK;
#elif defined(VI_LINUX)
			if (chdir(Path) != 0)
				return OS::Error::GetConditionOr();

			return Optional::OK;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> OS::Directory::Patch(const String& Path)
		{
			if (IsExists(Path.c_str()))
				return Optional::OK;

			return Create(Path.c_str());
		}
		ExpectsIO<void> OS::Directory::Scan(const String& Path, Vector<std::pair<String, FileEntry>>* Entries)
		{
			VI_ASSERT(Entries != nullptr, "entries should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] scan dir %s", Path.c_str());
#if defined(VI_MICROSOFT)
			wchar_t Buffer[CHUNK_SIZE];
			Stringify::ConvertToWide(Path.c_str(), Path.size(), Buffer, CHUNK_SIZE);

			DWORD Attributes = GetFileAttributesW(Buffer);
			if (Attributes == 0xFFFFFFFF || (Attributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
				return std::make_error_condition(std::errc::not_a_directory);

			wcscat(Buffer, L"\\*");
			WIN32_FIND_DATAW Info;
			HANDLE Handle = FindFirstFileW(Buffer, &Info);

			do
			{
				char Directory[CHUNK_SIZE] = { 0 };
				WideCharToMultiByte(CP_UTF8, 0, Info.cFileName, -1, Directory, sizeof(Directory), nullptr, nullptr);
				if (strcmp(Directory, ".") != 0 && strcmp(Directory, "..") != 0)
				{
					auto State = File::GetState(Path + '/' + Directory);
					if (State)
						Entries->push_back(std::make_pair<String, FileEntry>(Directory, std::move(*State)));
				}
			} while (FindNextFileW(Handle, &Info) == TRUE);
			FindClose(Handle);
#else
			DIR* Handle = opendir(Path.c_str());
			if (!Handle)
				return OS::Error::GetConditionOr();

			dirent* Next = nullptr;
			while ((Next = readdir(Handle)) != nullptr)
			{
				if (strcmp(Next->d_name, ".") != 0 && strcmp(Next->d_name, "..") != 0)
				{
					auto State = File::GetState(Path + '/' + Next->d_name);
					if (State)
						Entries->push_back(std::make_pair<String, FileEntry>(Next->d_name, std::move(*State)));
				}
			}
			closedir(Handle);
#endif
			return Optional::OK;
		}
		ExpectsIO<void> OS::Directory::Create(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_DEBUG("[io] create dir %s", Path);
#ifdef VI_MICROSOFT
			wchar_t Buffer[CHUNK_SIZE];
			Stringify::ConvertToWide(Path, strlen(Path), Buffer, CHUNK_SIZE);

			size_t Length = wcslen(Buffer);
			if (!Length)
				return std::make_error_condition(std::errc::invalid_argument);

			if (::CreateDirectoryW(Buffer, nullptr) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS)
				return Optional::OK;

			size_t Index = Length - 1;
			while (Index > 0 && (Buffer[Index] == '/' || Buffer[Index] == '\\'))
				Index--;

			while (Index > 0 && Buffer[Index] != '/' && Buffer[Index] != '\\')
				Index--;

			String Subpath(Path, Index);
			if (Index > 0 && !Create(Subpath.c_str()))
				return OS::Error::GetConditionOr();

			if (::CreateDirectoryW(Buffer, nullptr) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS)
				return Optional::OK;
#else
			if (mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST)
				return Optional::OK;

			size_t Index = strlen(Path) - 1;
			while (Index > 0 && Path[Index] != '/' && Path[Index] != '\\')
				Index--;

			String Subpath(Path);
			Subpath.erase(0, Index);
			if (Index > 0 && !Create(Subpath.c_str()))
				return OS::Error::GetConditionOr();

			if (mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST)
				return Optional::OK;
#endif
			return OS::Error::GetConditionOr();
		}
		ExpectsIO<void> OS::Directory::Remove(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_DEBUG("[io] remove dir %s", Path);
#ifdef VI_MICROSOFT
			WIN32_FIND_DATA FileInformation;
			String FilePath, Pattern = String(Path) + "\\*.*";
			HANDLE Handle = ::FindFirstFile(Pattern.c_str(), &FileInformation);

			if (Handle == INVALID_HANDLE_VALUE)
			{
				auto Condition = OS::Error::GetConditionOr();
				::FindClose(Handle);
				return Condition;
			}

			do
			{
				if (!strcmp(FileInformation.cFileName, ".") || !strcmp(FileInformation.cFileName, ".."))
					continue;

				FilePath = String(Path) + "\\" + FileInformation.cFileName;
				if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					auto Result = Remove(FilePath.c_str());
					if (Result)
						continue;

					::FindClose(Handle);
					return Result;
				}

				if (::SetFileAttributes(FilePath.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
				{
					auto Condition = OS::Error::GetConditionOr();
					::FindClose(Handle);
					return Condition;
				}

				if (::DeleteFile(FilePath.c_str()) == FALSE)
				{
					auto Condition = OS::Error::GetConditionOr();
					::FindClose(Handle);
					return Condition;
				}
			} while (::FindNextFile(Handle, &FileInformation) != FALSE);

			::FindClose(Handle);
			if (::GetLastError() != ERROR_NO_MORE_FILES)
				return OS::Error::GetConditionOr();

			if (::SetFileAttributes(Path, FILE_ATTRIBUTE_NORMAL) == FALSE || ::RemoveDirectory(Path) == FALSE)
				return OS::Error::GetConditionOr();
#elif defined VI_LINUX
			DIR* Handle = opendir(Path);
			size_t Size = strlen(Path);

			if (!Handle)
			{
				if (rmdir(Path) != 0)
					return OS::Error::GetConditionOr();

				return Optional::OK;
			}

			struct dirent* It;
			while ((It = readdir(Handle)))
			{
				if (!strcmp(It->d_name, ".") || !strcmp(It->d_name, ".."))
					continue;

				size_t Length = Size + strlen(It->d_name) + 2;
				char* Buffer = VI_MALLOC(char, Length);
				snprintf(Buffer, Length, "%s/%s", Path, It->d_name);

				struct stat State;
				if (stat(Buffer, &State) != 0)
				{
					auto Condition = OS::Error::GetConditionOr();
					VI_FREE(Buffer);
					closedir(Handle);
					return Condition;
				}

				if (S_ISDIR(State.st_mode))
				{
					auto Result = Remove(Buffer);
					VI_FREE(Buffer);
					if (Result)
						continue;

					closedir(Handle);
					return Result;
				}

				if (unlink(Buffer) != 0)
				{
					auto Condition = OS::Error::GetConditionOr();
					VI_FREE(Buffer);
					closedir(Handle);
					return Condition;
				}

				VI_FREE(Buffer);
			}

			closedir(Handle);
			if (rmdir(Path) != 0)
				return OS::Error::GetConditionOr();
#endif
			return Optional::OK;
		}
		ExpectsIO<String> OS::Directory::GetModule()
		{
			VI_MEASURE(Timings::FileSystem);
#ifndef VI_SDL2
#ifdef VI_MICROSOFT
			char Buffer[MAX_PATH + 1] = { };
			if (GetModuleFileNameA(nullptr, Buffer, MAX_PATH) == 0)
				return OS::Error::GetConditionOr();
#else
#ifdef PATH_MAX
			char Buffer[PATH_MAX + 1] = { };
#elif defined(_POSIX_PATH_MAX)
			char Buffer[_POSIX_PATH_MAX + 1] = { };
#else
			char Buffer[CHUNK_SIZE + 1] = { };
#endif
#ifdef VI_APPLE
			uint32_t BufferSize = sizeof(Buffer) - 1;
			if (_NSGetExecutablePath(Buffer, &BufferSize) != 0)
				return OS::Error::GetConditionOr();
#elif defined(VI_SOLARIS)
			const char* TempBuffer = getexecname();
			if (!TempBuffer || *TempBuffer == '\0')
				return OS::Error::GetConditionOr();

			TempBuffer = OS::Path::GetDirectory(TempBuffer);
			size_t TempBufferSize = strlen(TempBuffer);
			memcpy(Buffer, TempBuffer, TempBufferSize > sizeof(Buffer) - 1 ? sizeof(Buffer) - 1 : TempBufferSize);
#else
			size_t BufferSize = sizeof(Buffer) - 1;
			if (readlink("/proc/self/exe", Buffer, BufferSize) == -1 && readlink("/proc/curproc/file", Buffer, BufferSize) == -1 && readlink("/proc/curproc/exe", Buffer, BufferSize) == -1)
				return OS::Error::GetConditionOr();
#endif
#endif
			String Result = OS::Path::GetDirectory(Buffer);
			if (!Result.empty() && Result.back() != '/' && Result.back() != '\\')
				Result += VI_SPLITTER;

			VI_TRACE("[io] fetch module dir %s", Result.c_str());
			return Result;
#else
			char* Buffer = SDL_GetBasePath();
			if (!Buffer)
				return std::make_error_condition(std::errc::io_error);

			String Result = Buffer;
			SDL_free(Buffer);
			VI_TRACE("[io] fetch module dir %s", Result.c_str());
			return Result;
#endif
		}
		ExpectsIO<String> OS::Directory::GetWorking()
		{
			VI_MEASURE(Timings::FileSystem);
#ifdef VI_MICROSOFT
			char Buffer[MAX_PATH + 1] = { };
			if (GetCurrentDirectoryA(MAX_PATH, Buffer) == 0)
				return OS::Error::GetConditionOr();
#else
#ifdef PATH_MAX
			char Buffer[PATH_MAX + 1] = { };
#elif defined(_POSIX_PATH_MAX)
			char Buffer[_POSIX_PATH_MAX + 1] = { };
#else
			char Buffer[CHUNK_SIZE + 1] = { };
#endif
			if (!getcwd(Buffer, sizeof(Buffer)))
				return OS::Error::GetConditionOr();
#endif
			String Result = Buffer;
			if (!Result.empty() && Result.back() != '/' && Result.back() != '\\')
				Result += VI_SPLITTER;

			VI_TRACE("[io] fetch working dir %s", Result.c_str());
			return Result;
		}
		Vector<String> OS::Directory::GetMounts()
		{
			VI_TRACE("[io] fetch mount points");
			Vector<String> Output;
#ifdef VI_MICROSOFT
			DWORD DriveMask = GetLogicalDrives();
			char Offset = 'A';
			while (DriveMask)
			{
				if (DriveMask & 1)
				{
					String Letter(1, Offset);
					Letter.append(":\\");
					Output.push_back(std::move(Letter));
				}
				DriveMask >>= 1;
				++Offset;
			}

			return Output;
#else
			Output.push_back("/");
			return Output;
#endif
		}

		bool OS::File::IsExists(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] check path %s", Path);
			auto TargetPath = OS::Path::Resolve(Path);
			return TargetPath && IsPathExists(TargetPath->c_str());
		}
		int OS::File::Compare(const String& FirstPath, const String& SecondPath)
		{
			VI_ASSERT(!FirstPath.empty(), "first path should not be empty");
			VI_ASSERT(!SecondPath.empty(), "second path should not be empty");

			auto Props1 = GetProperties(FirstPath.c_str());
			auto Props2 = GetProperties(SecondPath.c_str());
			if (!Props1 || !Props2)
			{
				if (!Props1 && !Props2)
					return 0;
				else if (Props1)
					return 1;
				else if (Props2)
					return -1;
			}

			size_t Size1 = Props1->Size, Size2 = Props2->Size;
			VI_TRACE("[io] compare paths { %s (%" PRIu64 "), %s (%" PRIu64 ") }", FirstPath.c_str(), Size1, SecondPath.c_str(), Size2);

			if (Size1 > Size2)
				return 1;
			else if (Size1 < Size2)
				return -1;

			auto First = Open(FirstPath.c_str(), "rb");
			if (!First)
				return -1;

			auto Second = Open(SecondPath.c_str(), "rb");
			if (!Second)
			{
				OS::File::Close(*First);
				return -1;
			}

			const size_t Size = CHUNK_SIZE;
			char Buffer1[Size];
			char Buffer2[Size];
			int Diff = 0;

			do
			{
				VI_MEASURE(Timings::FileSystem);
				size_t S1 = fread(Buffer1, sizeof(char), Size, (FILE*)*First);
				size_t S2 = fread(Buffer2, sizeof(char), Size, (FILE*)*Second);
				if (S1 == S2)
				{
					if (S1 == 0)
						break;

					Diff = memcmp(Buffer1, Buffer2, S1);
				}
				else if (S1 > S2)
					Diff = 1;
				else if (S1 < S2)
					Diff = -1;
			} while (Diff == 0);

			OS::File::Close(*First);
			OS::File::Close(*Second);
			return Diff;
		}
		uint64_t OS::File::GetHash(const String& Data)
		{
			return Compute::Crypto::CRC32(Data);
		}
		uint64_t OS::File::GetIndex(const char* Data, size_t Size)
		{
			VI_ASSERT(Data != nullptr, "data buffer should be set");
			uint64_t Result = 0xcbf29ce484222325;
			for (size_t i = 0; i < Size; i++)
			{
				Result ^= Data[i];
				Result *= 1099511628211;
			}

			return Result;
		}
		ExpectsIO<void> OS::File::Write(const String& Path, const char* Data, size_t Length)
		{
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::FileSystem);
			auto Stream = Open(Path, FileMode::Binary_Write_Only);
			if (!Stream)
				return Stream.Error();

			size_t Size = Stream->Write(Data, Length);
			VI_RELEASE(Stream);

			if (Size != Length)
				return std::make_error_condition(std::errc::broken_pipe);

			return Optional::OK;
		}
		ExpectsIO<void> OS::File::Write(const String& Path, const String& Data)
		{
			return Write(Path, Data.c_str(), Data.size());
		}
		ExpectsIO<void> OS::File::Copy(const char* From, const char* To)
		{
			VI_ASSERT(From != nullptr && To != nullptr, "from and to should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_DEBUG("[io] copy file from %s to %s", From, To);
			std::ifstream Source(From, std::ios::binary);
			if (!Source)
				return std::make_error_condition(std::errc::bad_file_descriptor);
			
			auto Result = OS::Directory::Patch(OS::Path::GetDirectory(To));
			if (!Result)
				return Result;

			std::ofstream Destination(To, std::ios::binary);
			if (!Source)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			Destination << Source.rdbuf();
			return Optional::OK;
		}
		ExpectsIO<void> OS::File::Move(const char* From, const char* To)
		{
			VI_ASSERT(From != nullptr && To != nullptr, "from and to should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_DEBUG("[io] move file from %s to %s", From, To);
#ifdef VI_MICROSOFT
			if (MoveFileA(From, To) != TRUE)
				return OS::Error::GetConditionOr();

			return Optional::OK;
#elif defined VI_LINUX
			if (rename(From, To) != 0)
				return OS::Error::GetConditionOr();

			return Optional::OK;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> OS::File::Remove(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_DEBUG("[io] remove file %s", Path);
#ifdef VI_MICROSOFT
			SetFileAttributesA(Path, 0);
			if (DeleteFileA(Path) != TRUE)
				return OS::Error::GetConditionOr();

			return Optional::OK;
#elif defined VI_LINUX
			if (unlink(Path) != 0)
				return OS::Error::GetConditionOr();

			return Optional::OK;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> OS::File::Close(void* Stream)
		{
			VI_ASSERT(Stream != nullptr, "stream should be set");
			VI_DEBUG("[io] close fs %i", VI_FILENO((FILE*)Stream));
			if (fclose((FILE*)Stream) != 0)
				return OS::Error::GetConditionOr();

			return Optional::OK;
		}
		ExpectsIO<void> OS::File::GetState(const String& Path, FileEntry* File)
		{
			VI_MEASURE(Timings::FileSystem);
#if defined(VI_MICROSOFT)
			wchar_t Buffer[CHUNK_SIZE];
			Stringify::ConvertToWide(Path.c_str(), Path.size(), Buffer, CHUNK_SIZE);

			WIN32_FILE_ATTRIBUTE_DATA Info;
			if (GetFileAttributesExW(Buffer, GetFileExInfoStandard, &Info) == 0)
				return OS::Error::GetConditionOr();

			File->Size = MAKEUQUAD(Info.nFileSizeLow, Info.nFileSizeHigh);
			File->LastModified = SYS2UNIX_TIME(Info.ftLastWriteTime.dwLowDateTime, Info.ftLastWriteTime.dwHighDateTime);
			File->CreationTime = SYS2UNIX_TIME(Info.ftCreationTime.dwLowDateTime, Info.ftCreationTime.dwHighDateTime);
			File->IsDirectory = Info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			File->IsExists = File->IsReferenced = false;
			if (File->CreationTime > File->LastModified)
				File->LastModified = File->CreationTime;
			if (File->IsDirectory)
				File->IsExists = true;
			else if (!Path.empty())
				File->IsExists = (isalnum(Path.back()) || strchr("_-", Path.back()) != nullptr);
#else
			struct stat State;
			if (stat(Path.c_str(), &State) != 0)
				return OS::Error::GetConditionOr();

			struct tm Time;
			LocalTime(&State.st_ctime, &Time);

			File->Size = (size_t)(State.st_size);
			File->LastModified = State.st_mtime;
			File->CreationTime = mktime(&Time);
			File->IsDirectory = S_ISDIR(State.st_mode);
			File->IsReferenced = false;
			File->IsExists = true;
#endif
			VI_TRACE("[io] stat %s: %s %" PRIu64 " bytes", Path.c_str(), File->IsDirectory ? "dir" : "file", (uint64_t)File->Size);
			return Core::Optional::OK;
		}
		ExpectsIO<size_t> OS::File::Join(const String& To, const Vector<String>& Paths)
		{
			VI_ASSERT(!To.empty(), "to should not be empty");
			VI_ASSERT(!Paths.empty(), "paths to join should not be empty");
			VI_TRACE("[io] join %i path to %s", (int)Paths.size(), To.c_str());

			auto Target = Open(To, FileMode::Binary_Write_Only);
			if (!Target)
				return Target.Error();

			size_t Total = 0;
			Stream* Streamer = *Target;
			for (auto& Path : Paths)
			{
				auto Base = Open(Path, FileMode::Binary_Read_Only);
				if (Base)
				{
					Total += Base->ReadAll([&Streamer](char* Buffer, size_t Size) { Streamer->Write(Buffer, Size); });
					VI_RELEASE(Base);
				}
			}

			VI_RELEASE(Target);
			return Total;
		}
		ExpectsIO<FileState> OS::File::GetProperties(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
			VI_MEASURE(Timings::FileSystem);

			struct stat Buffer;
			if (stat(Path, &Buffer) != 0)
				return OS::Error::GetConditionOr();

			FileState State;
			State.Exists = true;
			State.Size = Buffer.st_size;
			State.Links = Buffer.st_nlink;
			State.Permissions = Buffer.st_mode;
			State.Device = Buffer.st_dev;
			State.GroupId = Buffer.st_gid;
			State.UserId = Buffer.st_uid;
			State.Document = Buffer.st_ino;
			State.LastAccess = Buffer.st_atime;
			State.LastPermissionChange = Buffer.st_ctime;
			State.LastModified = Buffer.st_mtime;

			VI_TRACE("[io] stat %s: %" PRIu64 " bytes", Path, (uint64_t)State.Size);
			return State;
		}
		ExpectsIO<FileEntry> OS::File::GetState(const String& Path)
		{
			FileEntry File;
			auto Status = GetState(Path, &File);
			if (!Status)
				return Status.Error();

			return File;
		}
		ExpectsIO<FILE*> OS::File::Open(const char* Path, const char* Mode)
		{
			VI_MEASURE(Timings::FileSystem);
			VI_ASSERT(Path != nullptr && Mode != nullptr, "path and mode should be set");
#ifdef VI_MICROSOFT
			wchar_t Buffer[CHUNK_SIZE], Type[20];
			Stringify::ConvertToWide(Path, strlen(Path), Buffer, CHUNK_SIZE);
			Stringify::ConvertToWide(Mode, strlen(Mode), Type, 20);

			FILE* Stream = _wfopen(Buffer, Type);
			if (!Stream)
				return OS::Error::GetConditionOr();

			VI_DEBUG("[io] open fs %i %s %s", VI_FILENO(Stream), Mode, Path);
			return Stream;
#else
			FILE* Stream = fopen(Path, Mode);
			if (!Stream)
				return OS::Error::GetConditionOr();

			VI_DEBUG("[io] open fs %i %s %s", VI_FILENO(Stream), Mode, Path);
			fcntl(VI_FILENO(Stream), F_SETFD, FD_CLOEXEC);
			return Stream;
#endif
		}
		ExpectsIO<Stream*> OS::File::Open(const String& Path, FileMode Mode, bool Async)
		{
			if (Path.empty())
				return std::make_error_condition(std::errc::no_such_file_or_directory);

			Network::Location URL(Path);
			if (URL.Protocol == "file")
			{
				Stream* Target = nullptr;
				if (Stringify::EndsWith(Path, ".gz"))
					Target = new GzStream();
				else
					Target = new FileStream();

				auto Result = Target->Open(Path.c_str(), Mode);
				if (!Result)
				{
					VI_RELEASE(Target);
					return Result.Error();
				}

				return Target;
			}
			else if (URL.Protocol == "http" || URL.Protocol == "https")
			{
				Stream* Target = new WebStream(Async);
				auto Result = Target->Open(Path.c_str(), Mode);
				if (!Result)
				{
					VI_RELEASE(Target);
					return Result.Error();
				}

				return Target;
			}

			return std::make_error_condition(std::errc::invalid_argument);
		}
		ExpectsIO<Stream*> OS::File::OpenArchive(const String& Path, size_t UnarchivedMaxSize)
		{
			auto State = OS::File::GetState(Path);
			if (!State)
				return Open(Path, FileMode::Binary_Write_Only);

			String Temp = Path::GetNonExistant(Path);
			Move(Path.c_str(), Temp.c_str());

			if (State->Size <= UnarchivedMaxSize)
			{
				auto Target = OpenJoin(Path, { Temp });
				if (Target)
					Remove(Temp.c_str());
				return Target;
			}

			auto Target = Open(Path, FileMode::Binary_Write_Only);
			if (Stringify::EndsWith(Temp, ".gz"))
				return Target;

			auto Archive = OpenJoin(Temp + ".gz", { Temp });
			if (Archive)
			{
				Remove(Temp.c_str());
				VI_RELEASE(Archive);
			}

			return Target;
		}
		ExpectsIO<Stream*> OS::File::OpenJoin(const String& To, const Vector<String>& Paths)
		{
			VI_ASSERT(!To.empty(), "to should not be empty");
			VI_ASSERT(!Paths.empty(), "paths to join should not be empty");
			VI_TRACE("[io] open join %i path to %s", (int)Paths.size(), To.c_str());

			auto Target = Open(To, FileMode::Binary_Write_Only);
			if (!Target)
				return Target;

			Stream* Streamer = *Target;
			for (auto& Path : Paths)
			{
				auto Base = Open(Path, FileMode::Binary_Read_Only);
				if (Base)
				{
					Base->ReadAll([&Streamer](char* Buffer, size_t Size) { Streamer->Write(Buffer, Size); });
					VI_RELEASE(Base);
				}
			}

			return Target;
		}
		ExpectsIO<unsigned char*> OS::File::ReadAll(const String& Path, size_t* Length)
		{
			auto Base = Open(Path, FileMode::Binary_Read_Only);
			if (!Base)
				return Base.Error();

			auto Result = ReadAll(*Base, Length);
			VI_RELEASE(Base);
			return Result;
		}
		ExpectsIO<unsigned char*> OS::File::ReadAll(Stream* Stream, size_t* Length)
		{
			VI_ASSERT(Stream != nullptr, "path should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[io] fd %i read-all", Stream->GetFd());
			if (Length != nullptr)
				*Length = 0;

			bool IsVirtual = Stream->VirtualSize() > 0;
			if (IsVirtual || Stream->IsSized())
			{
				size_t Size = IsVirtual ? Stream->VirtualSize() : Stream->Size();
				auto* Bytes = VI_MALLOC(unsigned char, sizeof(unsigned char) * (Size + 1));
				if (!Stream->Read((char*)Bytes, Size))
				{
					auto Condition = OS::Error::GetConditionOr();
					VI_FREE(Bytes);
					return Condition;
				}

				Bytes[Size] = '\0';
				if (Length != nullptr)
					*Length = Size;

				return Bytes;
			}

			Core::String Data;
			Stream->ReadAll([&Data](char* Buffer, size_t Length)
			{
				Data.reserve(Data.size() + Length);
				Data.append(Buffer, Length);
			});

			size_t Size = Data.size();
			if (Data.empty())
				return OS::Error::GetConditionOr();

			auto* Bytes = VI_MALLOC(unsigned char, sizeof(unsigned char) * (Data.size() + 1));
			memcpy(Bytes, Data.data(), sizeof(unsigned char) * Data.size());
			Bytes[Size] = '\0';
			if (Length != nullptr)
				*Length = Size;

			return Bytes;
		}
		ExpectsIO<unsigned char*> OS::File::ReadChunk(Stream* Stream, size_t Length)
		{
			VI_ASSERT(Stream != nullptr, "stream should be set");
			auto* Bytes = VI_MALLOC(unsigned char, (Length + 1));
			Stream->Read((char*)Bytes, Length);
			Bytes[Length] = '\0';

			return Bytes;
		}
		ExpectsIO<String> OS::File::ReadAsString(const String& Path)
		{
			size_t Length = 0;
			auto FileData = ReadAll(Path, &Length);
			if (!FileData)
				return FileData.Error();

			auto* Data = (char*)*FileData;
			String Output(Data, Length);
			VI_FREE(Data);

			return Output;
		}
		ExpectsIO<Vector<String>> OS::File::ReadAsArray(const String& Path)
		{
			ExpectsIO<String> Result = ReadAsString(Path);
			if (!Result)
				return Result.Error();

			return Stringify::Split(*Result, '\n');
		}

		bool OS::Path::IsRemote(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
			return Network::Location(Path).Protocol != "file";
		}
		bool OS::Path::IsRelative(const char* Path)
		{
#ifdef VI_MICROSOFT
			return !IsAbsolute(Path);
#else
			return Path[0] == '/' || Path[0] == '\\';
#endif
		}
		bool OS::Path::IsAbsolute(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
#ifdef VI_MICROSOFT
			if (Path[0] == '/' || Path[0] == '\\')
				return true;

			size_t Size = strnlen(Path, 2);
			if (Size < 2)
				return false;

			return std::isalnum(Path[0]) && Path[1] == ':';
#else
			return Path[0] == '/' || Path[0] == '\\';
#endif
		}
		ExpectsIO<String> OS::Path::Resolve(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
			VI_MEASURE(Timings::FileSystem);
			char Buffer[BLOB_SIZE] = { };
#ifdef VI_MICROSOFT
			if (GetFullPathNameA(Path, sizeof(Buffer), Buffer, nullptr) == 0)
				return OS::Error::GetConditionOr();
#else
			if (!realpath(Path, Buffer))
			{
				if (Buffer[0] == '\0' || memcmp(Path, Buffer, strnlen(Buffer, BLOB_SIZE)) != 0)
				{
					if (*Path == '\0' || strstr(Path, "./") != nullptr || strstr(Path, ".\\") != nullptr)
						return OS::Error::GetConditionOr();
				}

				String Output(Path);
				VI_TRACE("[io] resolve %s path (non-existant)", Path);
				return Output;
			}
#endif
			String Output(Buffer, strnlen(Buffer, BLOB_SIZE));
			VI_TRACE("[io] resolve %s path: %s", Path, Output.c_str());
			return Output;
		}
		ExpectsIO<String> OS::Path::Resolve(const String& Path, const String& Directory, bool EvenIfExists)
		{
			VI_ASSERT(!Path.empty() && !Directory.empty(), "path and directory should not be empty");
			if (IsAbsolute(Path.c_str()))
				return Path;
			else if (!EvenIfExists && IsPathExists(Path.c_str()) && Path.find("..") == std::string::npos)
				return Path;

			bool Prefixed = Stringify::StartsOf(Path, "/\\");
			bool Relative = !Prefixed && (Stringify::StartsWith(Path, "./") || Stringify::StartsWith(Path, ".\\"));
			bool Postfixed = Stringify::EndsOf(Directory, "/\\");

			String Target = Directory;
			if (!Prefixed && !Postfixed)
				Target.append(1, VI_SPLITTER);

			if (Relative)
				Target.append(Path.c_str() + 2, Path.size() - 2);
			else
				Target.append(Path);

			return Resolve(Target.c_str());
		}
		ExpectsIO<String> OS::Path::ResolveDirectory(const char* Path)
		{
			ExpectsIO<String> Result = Resolve(Path);
			if (!Result)
				return Result;

			if (!Result->empty() && !Stringify::EndsOf(*Result, "/\\"))
				Result->append(1, VI_SPLITTER);

			return Result;
		}
		ExpectsIO<String> OS::Path::ResolveDirectory(const String& Path, const String& Directory, bool EvenIfExists)
		{
			ExpectsIO<String> Result = Resolve(Path, Directory, EvenIfExists);
			if (!Result)
				return Result;

			if (!Result->empty() && !Stringify::EndsOf(*Result, "/\\"))
				Result->append(1, VI_SPLITTER);

			return Result;
		}
		String OS::Path::GetNonExistant(const String& Path)
		{
			VI_ASSERT(!Path.empty(), "path should not be empty");
			const char* Extension = GetExtension(Path.c_str());
			bool IsTrueFile = Extension != nullptr && *Extension != '\0';
			size_t ExtensionAt = IsTrueFile ? Path.rfind(Extension) : Path.size();
			if (ExtensionAt == String::npos)
				return Path;

			String First = Path.substr(0, ExtensionAt).append(1, '.');
			String Second = IsTrueFile ? Extension : String();
			String Filename = Path;
			size_t Nonce = 0;

			while (true)
			{
				auto Data = OS::File::GetState(Filename);
				if (!Data || !Data->Size)
					break;

				Filename = First + Core::ToString(++Nonce) + Second;
			}

			return Filename;
		}
		String OS::Path::GetDirectory(const char* Path, size_t Level)
		{
			VI_ASSERT(Path != nullptr, "path should be set");

			String Buffer(Path);
			TextSettle Result = Stringify::ReverseFindOf(Buffer, "/\\");
			if (!Result.Found)
				return Path;

			size_t Size = Buffer.size();
			for (size_t i = 0; i < Level; i++)
			{
				TextSettle Current = Stringify::ReverseFindOf(Buffer, "/\\", Size - Result.Start);
				if (!Current.Found)
				{
					Stringify::Splice(Buffer, 0, Result.End);
					if (Buffer.empty())
						return "/";

					return Buffer;
				}

				Result = Current;
			}

			Stringify::Splice(Buffer, 0, Result.End);
			if (Buffer.empty())
				return "/";

			return Buffer;
		}
		const char* OS::Path::GetFilename(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
			size_t Size = strlen(Path);
			for (size_t i = Size; i-- > 0;)
			{
				if (Path[i] == '/' || Path[i] == '\\')
					return Path + i + 1;
			}

			return Path;
		}
		const char* OS::Path::GetExtension(const char* Path)
		{
			VI_ASSERT(Path != nullptr, "path should be set");
			const char* Buffer = Path;
			while (*Buffer != '\0')
				Buffer++;

			while (*Buffer != '.' && Buffer != Path)
				Buffer--;

			if (Buffer == Path)
				return nullptr;

			return Buffer;
		}

		bool OS::Net::GetETag(char* Buffer, size_t Length, FileEntry* Resource)
		{
			VI_ASSERT(Resource != nullptr, "resource should be set");
			return GetETag(Buffer, Length, Resource->LastModified, Resource->Size);
		}
		bool OS::Net::GetETag(char* Buffer, size_t Length, int64_t LastModified, size_t ContentLength)
		{
			VI_ASSERT(Buffer != nullptr && Length > 0, "buffer should be set and size should be greater than Zero");
			snprintf(Buffer, Length, "\"%lx.%" PRIu64 "\"", (unsigned long)LastModified, (uint64_t)ContentLength);
			return true;
		}
		socket_t OS::Net::GetFd(FILE* Stream)
		{
			VI_ASSERT(Stream != nullptr, "stream should be set");
			return VI_FILENO(Stream);
		}

		void OS::Process::Abort()
		{
#ifdef NDEBUG
			VI_DEBUG("[os] process terminate on thread %s", GetThreadId(std::this_thread::get_id()).c_str());
			std::terminate();
#else
			VI_DEBUG("[os] process abort on thread %s", GetThreadId(std::this_thread::get_id()).c_str());
			std::abort();
#endif
		}
		void OS::Process::Exit(int Code)
		{
			VI_DEBUG("[os] process exit:%i on thread %s", Code, GetThreadId(std::this_thread::get_id()).c_str());
			std::exit(Code);
		}
		void OS::Process::Interrupt()
		{
#ifndef NDEBUG
			VI_DEBUG("[os] process paused on thread %s", GetThreadId(std::this_thread::get_id()).c_str());
#ifndef VI_MICROSOFT
#ifndef SIGTRAP
			__debugbreak();
#else
			raise(SIGTRAP);
#endif
#else
			if (IsDebuggerPresent())
				__debugbreak();
#endif
#endif
		}
		int OS::Process::GetSignalId(Signal Type)
		{
			switch (Type)
			{
				case Signal::SIG_INT:
#ifdef SIGINT
					return SIGINT;
#else
					return -1;
#endif
				case Signal::SIG_ILL:
#ifdef SIGILL
					return SIGILL;
#else
					return -1;
#endif
				case Signal::SIG_FPE:
#ifdef SIGFPE
					return SIGFPE;
#else
					return -1;
#endif
				case Signal::SIG_SEGV:
#ifdef SIGSEGV
					return SIGSEGV;
#else
					return -1;
#endif
				case Signal::SIG_TERM:
#ifdef SIGTERM
					return SIGTERM;
#else
					return -1;
#endif
				case Signal::SIG_BREAK:
#ifdef SIGBREAK
					return SIGBREAK;
#else
					return -1;
#endif
				case Signal::SIG_ABRT:
#ifdef SIGABRT
					return SIGABRT;
#else
					return -1;
#endif
				case Signal::SIG_BUS:
#ifdef SIGBUS
					return SIGBUS;
#else
					return -1;
#endif
				case Signal::SIG_ALRM:
#ifdef SIGALRM
					return SIGALRM;
#else
					return -1;
#endif
				case Signal::SIG_HUP:
#ifdef SIGHUP
					return SIGHUP;
#else
					return -1;
#endif
				case Signal::SIG_QUIT:
#ifdef SIGQUIT
					return SIGQUIT;
#else
					return -1;
#endif
				case Signal::SIG_TRAP:
#ifdef SIGTRAP
					return SIGTRAP;
#else
					return -1;
#endif
				case Signal::SIG_CONT:
#ifdef SIGCONT
					return SIGCONT;
#else
					return -1;
#endif
				case Signal::SIG_STOP:
#ifdef SIGSTOP
					return SIGSTOP;
#else
					return -1;
#endif
				case Signal::SIG_PIPE:
#ifdef SIGPIPE
					return SIGPIPE;
#else
					return -1;
#endif
				case Signal::SIG_CHLD:
#ifdef SIGCHLD
					return SIGCHLD;
#else
					return -1;
#endif
				case Signal::SIG_USR1:
#ifdef SIGUSR1
					return SIGUSR1;
#else
					return -1;
#endif
				case Signal::SIG_USR2:
#ifdef SIGUSR2
					return SIGUSR2;
#else
					return -1;
#endif
				default:
					VI_ASSERT(false, "invalid signal type %i", (int)Type);
					return -1;
			}
		}
		bool OS::Process::SetSignalCallback(Signal Type, SignalCallback Callback)
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			int Id = GetSignalId(Type);
			if (Id == -1)
				return false;
			VI_DEBUG("[os] signal %i: callback", Id);
#ifdef VI_LINUX
			struct sigaction Handle;
			Handle.sa_handler = Callback;
			sigemptyset(&Handle.sa_mask);
			Handle.sa_flags = 0;

			return sigaction(Id, &Handle, NULL) != -1;
#else
			return signal(Id, Callback) != SIG_ERR;
#endif
		}
		bool OS::Process::SetSignalDefault(Signal Type)
		{
			int Id = GetSignalId(Type);
			if (Id == -1)
				return false;
			VI_DEBUG("[os] signal %i: default", Id);
#ifdef VI_LINUX
			struct sigaction Handle;
			Handle.sa_handler = SIG_DFL;
			sigemptyset(&Handle.sa_mask);
			Handle.sa_flags = 0;

			return sigaction(Id, &Handle, NULL) != -1;
#else
			return signal(Id, SIG_DFL) != SIG_ERR;
#endif
		}
		bool OS::Process::SetSignalIgnore(Signal Type)
		{
			int Id = GetSignalId(Type);
			if (Id == -1)
				return false;
			VI_DEBUG("[os] signal %i: ignore", Id);
#ifdef VI_LINUX
			struct sigaction Handle;
			Handle.sa_handler = SIG_IGN;
			sigemptyset(&Handle.sa_mask);
			Handle.sa_flags = 0;

			return sigaction(Id, &Handle, NULL) != -1;
#else
			return signal(Id, SIG_IGN) != SIG_ERR;
#endif
		}
		int OS::Process::ExecutePlain(const String& Command)
		{
			VI_ASSERT(!Command.empty(), "format should be set");
			VI_DEBUG("[os] execute sp:command [ %s ]", Command.c_str());
			return system(Command.c_str());
		}
		ExpectsIO<ProcessStream*> OS::Process::ExecuteWriteOnly(const String& Command)
		{
			VI_ASSERT(!Command.empty(), "format should be set");
			VI_DEBUG("[os] execute wo:command [ %s ]", Command.c_str());
			ProcessStream* Stream = new ProcessStream();
			auto Result = Stream->Open(Command.c_str(), FileMode::Write_Only);
			if (Result)
				return Stream;

			VI_RELEASE(Stream);
			return Result.Error();
		}
		ExpectsIO<ProcessStream*> OS::Process::ExecuteReadOnly(const String& Command)
		{
			VI_ASSERT(!Command.empty(), "format should be set");
			VI_DEBUG("[os] execute ro:command [ %s ]", Command.c_str());
			ProcessStream* Stream = new ProcessStream();
			auto Result = Stream->Open(Command.c_str(), FileMode::Read_Only);
			if (Result)
				return Stream;

			VI_RELEASE(Stream);
			return Result.Error();
		}
		bool OS::Process::Spawn(const String& Path, const Vector<String>& Params, ChildProcess* Child)
		{
			VI_MEASURE(Timings::FileSystem);
#ifdef VI_MICROSOFT
			HANDLE Job = CreateJobObject(nullptr, nullptr);
			if (Job == nullptr)
			{
				VI_ERR("[os] cannot create job object for process");
				return false;
			}

			JOBOBJECT_EXTENDED_LIMIT_INFORMATION Info = { };
			Info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
			if (SetInformationJobObject(Job, JobObjectExtendedLimitInformation, &Info, sizeof(Info)) == 0)
			{
				VI_ERR("[os] cannot set job object for process");
				return false;
			}

			STARTUPINFO StartupInfo;
			ZeroMemory(&StartupInfo, sizeof(StartupInfo));
			StartupInfo.cb = sizeof(StartupInfo);
			StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
			StartupInfo.wShowWindow = SW_HIDE;

			PROCESS_INFORMATION Process;
			ZeroMemory(&Process, sizeof(Process));

			auto Exe = Path::Resolve(Path.c_str());
			if (!Exe)
			{
				VI_ERR("[os] cannot spawn process: unknown path %s", Path.c_str());
				return false;
			}

			if (!Stringify::EndsWith(*Exe, ".exe"))
				Exe->append(".exe");

			String Args = Stringify::Text("\"%s\"", Exe->c_str());
			for (const auto& Param : Params)
				Args.append(1, ' ').append(Param);

			if (!CreateProcessA(Exe->c_str(), (char*)Args.data(), nullptr, nullptr, TRUE, CREATE_BREAKAWAY_FROM_JOB | HIGH_PRIORITY_CLASS, nullptr, nullptr, &StartupInfo, &Process))
			{
				VI_ERR("[os] cannot spawn process %s", Exe->c_str());
				return false;
			}

			VI_DEBUG("[os] spawn process %i on %s", (int)GetProcessId(Process.hProcess), Path.c_str());
			AssignProcessToJobObject(Job, Process.hProcess);
			if (Child != nullptr && !Child->Valid)
			{
				Child->Process = (void*)Process.hProcess;
				Child->Thread = (void*)Process.hThread;
				Child->Job = (void*)Job;
				Child->Valid = true;
			}

			return true;
#else
			if (!File::IsExists(Path.c_str()))
			{
				VI_ERR("[os] cannot spawn process %s (file does not exists)", Path.c_str());
				return false;
			}

			pid_t ProcessId = fork();
			if (ProcessId == 0)
			{
				Vector<char*> Args;
				for (auto It = Params.begin(); It != Params.end(); ++It)
					Args.push_back((char*)It->c_str());
				Args.push_back(nullptr);

				execve(Path.c_str(), Args.data(), nullptr);
				exit(0);
			}
			else if (Child != nullptr)
			{
				VI_DEBUG("[os] spawn process %i on %s", (int)ProcessId, Path.c_str());
				Child->Process = ProcessId;
				Child->Valid = (ProcessId > 0);
			}

			return (ProcessId > 0);
#endif
		}
		bool OS::Process::Await(ChildProcess* Process, int* ExitCode)
		{
			VI_ASSERT(Process != nullptr && Process->Valid, "process should be set and be valid");
#ifdef VI_MICROSOFT
			VI_TRACE("[os] await process %s", (int)GetProcessId(Process->Process));
			WaitForSingleObject(Process->Process, INFINITE);
			VI_DEBUG("[os] close process %s", (int)GetProcessId(Process->Process));

			if (ExitCode != nullptr)
			{
				DWORD Result;
				if (!GetExitCodeProcess(Process->Process, &Result))
				{
					Free(Process);
					return false;
				}

				*ExitCode = (int)Result;
			}
#else
			int Status;
			VI_TRACE("[os] await process %i", (int)Process->Process);
			waitpid(Process->Process, &Status, 0);

			VI_DEBUG("[os] close process %i", (int)Process->Process);
			if (ExitCode != nullptr)
				*ExitCode = WEXITSTATUS(Status);
#endif
			Free(Process);
			return true;
		}
		bool OS::Process::Free(ChildProcess* Child)
		{
			VI_ASSERT(Child != nullptr && Child->Valid, "child should be set and be valid");
#ifdef VI_MICROSOFT
			if (Child->Process != nullptr)
			{
				VI_TRACE("[os] free process handle 0x%" PRIXPTR, (void*)Child->Process);
				CloseHandle((HANDLE)Child->Process);
				Child->Process = nullptr;
			}

			if (Child->Thread != nullptr)
			{
				VI_TRACE("[os] free process thread 0x%" PRIXPTR, (void*)Child->Process);
				CloseHandle((HANDLE)Child->Thread);
				Child->Thread = nullptr;
			}

			if (Child->Job != nullptr)
			{
				VI_TRACE("[os] free process job 0x%" PRIXPTR, (void*)Child->Process);
				CloseHandle((HANDLE)Child->Job);
				Child->Job = nullptr;
			}
#endif
			Child->Valid = false;
			return true;
		}
		String OS::Process::GetThreadId(const std::thread::id& Id)
		{
			StringStream Stream;
			Stream << Id;

			return Stream.str();
		}
		ExpectsIO<String> OS::Process::GetEnv(const String& Name)
		{
			char* Value = std::getenv(Name.c_str());
			if (!Value)
				return OS::Error::GetConditionOr();

			String Output(Value, strlen(Value));
			return Output;
		}
		UnorderedMap<String, String> OS::Process::GetArgs(int ArgsCount, char** Args, const String& WhenNoValue)
		{
			UnorderedMap<String, String> Results;
			VI_ASSERT(Args != nullptr, "arguments should be set");
			VI_ASSERT(ArgsCount > 0, "arguments count should be greater than zero");

			Vector<String> Params;
			for (int i = 0; i < ArgsCount; i++)
			{
				VI_ASSERT(Args[i] != nullptr, "argument %i should be set", i);
				Params.push_back(Args[i]);
			}

			for (size_t i = 1; i < Params.size(); i++)
			{
				auto& Item = Params[i];
				if (Item.empty() || Item.front() != '-')
					continue;

				if (Item.size() > 1 && Item[1] == '-')
				{
					Item = Item.substr(2);
					size_t Position = Item.find('=');
					if (Position != String::npos)
					{
						String Value = Item.substr(Position + 1);
						Results[Item.substr(0, Position)] = Value.empty() ? WhenNoValue : Value;
					}
					else
						Results[Item] = WhenNoValue;
				}
				else if (i + 1 < Params.size() && Params[i + 1].front() != '-')
				{
					auto& Value = Params[++i];
					Results[Item.substr(1)] = Value.empty() ? WhenNoValue : Value;
				}
				else
					Results[Item.substr(1)] = WhenNoValue;
			}

			Results["__path__"] = Params.front();
			return Results;
		}

		ExpectsIO<void*> OS::Symbol::Load(const String& Path)
		{
			VI_MEASURE(Timings::FileSystem);
			String Name(Path);
#ifdef VI_MICROSOFT
			if (Path.empty())
			{
				HMODULE Module = GetModuleHandle(nullptr);
				if (!Module)
					return OS::Error::GetConditionOr();

				return (void*)Module;
			}
			else if (!Stringify::EndsWith(Name, ".dll"))
				Name.append(".dll");

			VI_DEBUG("[dl] load dll library %s", Name.c_str());
			HMODULE Module = LoadLibrary(Name.c_str());
			if (!Module)
				return OS::Error::GetConditionOr();

			return (void*)Module;
#elif defined(VI_APPLE)
			if (Path.empty())
			{
				void* Module = (void*)dlopen(nullptr, RTLD_LAZY);
				if (!Module)
					return OS::Error::GetConditionOr();

				return (void*)Module;
			}
			else if (!Stringify::EndsWith(Name, ".dylib"))
				Name.append(".dylib");

			VI_DEBUG("[dl] load dylib library %s", Name.c_str());
			void* Module = (void*)dlopen(Name.c_str(), RTLD_LAZY);
			if (!Module)
				return OS::Error::GetConditionOr();

			return (void*)Module;
#elif defined(VI_LINUX)
			if (Path.empty())
			{
				void* Module = (void*)dlopen(nullptr, RTLD_LAZY);
				if (!Module)
					return OS::Error::GetConditionOr();

				return (void*)Module;
			}
			else if (!Stringify::EndsWith(Name, ".so"))
				Name.append(".so");

			VI_DEBUG("[dl] load so library %s", Name.c_str());
			void* Module = (void*)dlopen(Name.c_str(), RTLD_LAZY);
			if (!Module)
				return OS::Error::GetConditionOr();

			return (void*)Module;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void*> OS::Symbol::LoadFunction(void* Handle, const String& Name)
		{
			VI_ASSERT(Handle != nullptr && !Name.empty(), "handle should be set and name should not be empty");
			VI_DEBUG("[dl] load function %s", Name.c_str());
			VI_MEASURE(Timings::FileSystem);
#ifdef VI_MICROSOFT
			void* Result = (void*)GetProcAddress((HMODULE)Handle, Name.c_str());
			if (!Result)
				return OS::Error::GetConditionOr();

			return Result;
#elif defined(VI_LINUX)
			void* Result = (void*)dlsym(Handle, Name.c_str());
			if (!Result)
				return OS::Error::GetConditionOr();

			return Result;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> OS::Symbol::Unload(void* Handle)
		{
			VI_ASSERT(Handle != nullptr, "handle should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_DEBUG("[dl] unload library 0x%" PRIXPTR, Handle);
#ifdef VI_MICROSOFT
			if (FreeLibrary((HMODULE)Handle) != TRUE)
				return OS::Error::GetConditionOr();

			return Optional::OK;
#elif defined(VI_LINUX)
			if (dlclose(Handle) != 0)
				return OS::Error::GetConditionOr();

			return Optional::OK;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}

		bool OS::Input::Text(const String& Title, const String& Message, const String& DefaultInput, String* Result)
		{
#ifdef VI_TINYFILEDIALOGS
			VI_TRACE("[dg] open input { title: %s, message: %s }", Title.c_str(), Message.c_str());
			const char* Data = tinyfd_inputBox(Title.c_str(), Message.c_str(), DefaultInput.c_str());
			if (!Data)
				return false;

			VI_TRACE("[dg] close input: %s", Data ? Data : "NULL");
			if (Result != nullptr)
				*Result = Data;

			return true;
#else
			VI_ERR("[dg] open input: unsupported");
			return false;
#endif
		}
		bool OS::Input::Password(const String& Title, const String& Message, String* Result)
		{
#ifdef VI_TINYFILEDIALOGS
			VI_TRACE("[dg] open password { title: %s, message: %s }", Title.c_str(), Message.c_str());
			const char* Data = tinyfd_inputBox(Title.c_str(), Message.c_str(), nullptr);
			if (!Data)
				return false;

			VI_TRACE("[dg] close password: %s", Data ? Data : "NULL");
			if (Result != nullptr)
				*Result = Data;

			return true;
#else
			VI_ERR("[dg] open password: unsupported");
			return false;
#endif
		}
		bool OS::Input::Save(const String& Title, const String& DefaultPath, const String& Filter, const String& FilterDescription, String* Result)
		{
#ifdef VI_TINYFILEDIALOGS
			Vector<String> Sources = Stringify::Split(Filter, ',');
			Vector<char*> Patterns;
			for (auto& It : Sources)
				Patterns.push_back((char*)It.c_str());

			VI_TRACE("[dg] open save { title: %s, filter: %s }", Title.c_str(), Filter.c_str());
			const char* Data = tinyfd_saveFileDialog(Title.c_str(), DefaultPath.c_str(), (int)Patterns.size(),
				Patterns.empty() ? nullptr : Patterns.data(), FilterDescription.empty() ? nullptr : FilterDescription.c_str());

			if (!Data)
				return false;

			VI_TRACE("[dg] close save: %s", Data ? Data : "NULL");
			if (Result != nullptr)
				*Result = Data;

			return true;
#else
			VI_ERR("[dg] open save: unsupported");
			return false;
#endif
		}
		bool OS::Input::Open(const String& Title, const String& DefaultPath, const String& Filter, const String& FilterDescription, bool Multiple, String* Result)
		{
#ifdef VI_TINYFILEDIALOGS
			Vector<String> Sources = Stringify::Split(Filter, ',');
			Vector<char*> Patterns;
			for (auto& It : Sources)
				Patterns.push_back((char*)It.c_str());

			VI_TRACE("[dg] open load { title: %s, filter: %s }", Title.c_str(), Filter.c_str());
			const char* Data = tinyfd_openFileDialog(Title.c_str(), DefaultPath.c_str(), (int)Patterns.size(),
				Patterns.empty() ? nullptr : Patterns.data(), FilterDescription.empty() ? nullptr : FilterDescription.c_str(), Multiple);

			if (!Data)
				return false;

			VI_TRACE("[dg] close load: %s", Data ? Data : "NULL");
			if (Result != nullptr)
				*Result = Data;

			return true;
#else
			VI_ERR("[dg] open load: unsupported");
			return false;
#endif
		}
		bool OS::Input::Folder(const String& Title, const String& DefaultPath, String* Result)
		{
#ifdef VI_TINYFILEDIALOGS
			VI_TRACE("[dg] open folder { title: %s }", Title.c_str());
			const char* Data = tinyfd_selectFolderDialog(Title.c_str(), DefaultPath.c_str());
			if (!Data)
				return false;

			VI_TRACE("[dg] close folder: %s", Data ? Data : "NULL");
			if (Result != nullptr)
				*Result = Data;

			return true;
#else
			VI_ERR("[dg] open folder: unsupported");
			return false;
#endif
		}
		bool OS::Input::Color(const String& Title, const String& DefaultHexRGB, String* Result)
		{
#ifdef VI_TINYFILEDIALOGS
			VI_TRACE("[dg] open color { title: %s }", Title.c_str());
			unsigned char RGB[3] = { 0, 0, 0 };
			const char* Data = tinyfd_colorChooser(Title.c_str(), DefaultHexRGB.c_str(), RGB, RGB);
			if (!Data)
				return false;

			VI_TRACE("[dg] close color: %s", Data ? Data : "NULL");
			if (Result != nullptr)
				*Result = Data;

			return true;
#else
			VI_ERR("[dg] open color: unsupported");
			return false;
#endif
		}

		int OS::Error::Get(bool ClearLastError)
		{
#ifdef VI_MICROSOFT
			int ErrorCode = GetLastError();
			if (ErrorCode != ERROR_SUCCESS)
			{
				if (ClearLastError)
					SetLastError(ERROR_SUCCESS);
				return ErrorCode;
			}

			ErrorCode = WSAGetLastError();
			if (ErrorCode != ERROR_SUCCESS)
			{
				if (ClearLastError)
					WSASetLastError(ERROR_SUCCESS);
			}

			ErrorCode = errno;
			if (ErrorCode != 0)
			{
				if (ClearLastError)
					errno = 0;
				return ErrorCode;
			}

			return ErrorCode;
#else
			int ErrorCode = errno;
			if (ErrorCode != 0)
			{
				if (ClearLastError)
					errno = 0;
			}

			return ErrorCode;
#endif
		}
		bool OS::Error::Occurred()
		{
			return IsError(Get(false));
		}
		bool OS::Error::IsError(int Code)
		{
#ifdef VI_MICROSOFT
			return Code != ERROR_SUCCESS;
#else
			return Code != 0;
#endif
		}
		void OS::Error::Clear()
		{
#ifdef VI_MICROSOFT
			SetLastError(ERROR_SUCCESS);
			WSASetLastError(ERROR_SUCCESS);
#endif
			errno = 0;
		}
		std::error_condition OS::Error::GetCondition()
		{
			return GetCondition(Get());
		}
		std::error_condition OS::Error::GetCondition(int Code)
		{
#ifdef VI_MICROSOFT
			return std::error_condition(Code, std::system_category());
#else
			return std::error_condition(Code, std::generic_category());
#endif
		}
		std::error_condition OS::Error::GetConditionOr(std::errc Code)
		{
			if (!Occurred())
				return std::make_error_condition(Code);

			return GetCondition(Get());
		}
		String OS::Error::GetName(int Code)
		{
#ifdef VI_MICROSOFT
			LPSTR Buffer = nullptr;
			size_t Size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, Code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&Buffer, 0, nullptr);
			String Result(Buffer, Size);
			Stringify::Replace(Result, "\r", "");
			Stringify::Replace(Result, "\n", "");
			LocalFree(Buffer);
			return Result;
#else
			char* Buffer = strerror(Code);
			return Buffer ? Buffer : "";
#endif
		}

		static thread_local Costate* Cothread = nullptr;
		Costate::Costate(size_t StackSize) noexcept : Thread(std::this_thread::get_id()), Current(nullptr), Master(VI_NEW(Cocontext)), Size(StackSize), ExternalCondition(nullptr), ExternalMutex(nullptr)
		{
			VI_TRACE("[co] spawn coroutine state 0x%" PRIXPTR " on thread %s", (void*)this, OS::Process::GetThreadId(Thread).c_str());
		}
		Costate::~Costate() noexcept
		{
			VI_TRACE("[co] despawn coroutine state 0x%" PRIXPTR " on thread %s", (void*)this, OS::Process::GetThreadId(Thread).c_str());
			if (Cothread == this)
				Cothread = nullptr;

			for (auto& Routine : Cached)
				VI_DELETE(Coroutine, Routine);

			for (auto& Routine : Used)
				VI_DELETE(Coroutine, Routine);

			VI_DELETE(Cocontext, Master);
		}
		Coroutine* Costate::Pop(const TaskCallback& Procedure)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot call outside costate thread");

			Coroutine* Routine = nullptr;
			if (!Cached.empty())
			{
				Routine = *Cached.begin();
				Routine->Callback = Procedure;
				Routine->State = Coexecution::Active;
				Cached.erase(Cached.begin());
			}
			else
				Routine = VI_NEW(Coroutine, this, Procedure);

			Used.emplace(Routine);
			return Routine;
		}
		Coroutine* Costate::Pop(TaskCallback&& Procedure)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot deactive coroutine outside costate thread");

			Coroutine* Routine = nullptr;
			if (!Cached.empty())
			{
				Routine = *Cached.begin();
				Routine->Callback = std::move(Procedure);
				Routine->State = Coexecution::Active;
				Cached.erase(Cached.begin());
			}
			else
				Routine = VI_NEW(Coroutine, this, std::move(Procedure));

			Used.emplace(Routine);
			return Routine;
		}
		Coexecution Costate::Resume(Coroutine* Routine)
		{
			VI_ASSERT(Routine != nullptr, "coroutine should be set");
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot resume coroutine outside costate thread");
			VI_ASSERT(Routine->Master == this, "coroutine should be created by this costate");

			if (Current == Routine)
				return Coexecution::Active;
			else if (Routine->State == Coexecution::Finished)
				return Coexecution::Finished;

			return Execute(Routine);
		}
		Coexecution Costate::Execute(Coroutine* Routine)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot call outside costate thread");
			VI_ASSERT(Routine != nullptr, "coroutine should be set");
			VI_ASSERT(Routine->State != Coexecution::Finished, "coroutine should not be dead");

			if (Routine->State == Coexecution::Suspended)
				return Coexecution::Suspended;

			if (Routine->State == Coexecution::Resumable)
				Routine->State = Coexecution::Active;

			Cocontext* Fiber = Routine->Slave;
			Current = Routine;
#ifdef VI_FCTX
			Fiber->Context = jump_fcontext(Fiber->Context, (void*)this).fctx;
#elif VI_MICROSOFT
			SwitchToFiber(Fiber->Context);
#else
			swapcontext(&Master->Context, &Fiber->Context);
#endif
			if (Routine->Return)
			{
				Routine->Return();
				Routine->Return = nullptr;
			}
			
			return Routine->State == Coexecution::Finished ? Coexecution::Finished : Coexecution::Active;
		}
		void Costate::Reuse(Coroutine* Routine)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot call outside costate thread");
			VI_ASSERT(Routine != nullptr, "coroutine should be set");
			VI_ASSERT(Routine->Master == this, "coroutine should be created by this costate");
			VI_ASSERT(Routine->State == Coexecution::Finished, "coroutine should be empty");

			Routine->Callback = nullptr;
			Routine->Return = nullptr;
			Routine->State = Coexecution::Active;
			Used.erase(Routine);
			Cached.emplace(Routine);
		}
		void Costate::Push(Coroutine* Routine)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot call outside costate thread");
			VI_ASSERT(Routine != nullptr, "coroutine should be set");
			VI_ASSERT(Routine->Master == this, "coroutine should be created by this costate");
			VI_ASSERT(Routine->State == Coexecution::Finished, "coroutine should be empty");

			Cached.erase(Routine);
			Used.erase(Routine);
			VI_DELETE(Coroutine, Routine);
		}
		void Costate::Activate(Coroutine* Routine)
		{
			VI_ASSERT(Routine != nullptr, "coroutine should be set");
			VI_ASSERT(Routine->Master == this, "coroutine should be created by this costate");
			VI_ASSERT(Routine->State != Coexecution::Finished, "coroutine should not be empty");

			if (ExternalMutex != nullptr && ExternalCondition != nullptr && Thread != std::this_thread::get_id())
			{
				UMutex<std::mutex> Unique(*ExternalMutex);
				if (Routine->State == Coexecution::Suspended)
					Routine->State = Coexecution::Resumable;
				ExternalCondition->notify_one();
			}
			else if (Routine->State == Coexecution::Suspended)
				Routine->State = Coexecution::Resumable;
		}
		void Costate::Deactivate(Coroutine* Routine)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot deactive coroutine outside costate thread");
			VI_ASSERT(Routine != nullptr, "coroutine should be set");
			VI_ASSERT(Routine->Master == this, "coroutine should be created by this costate");
			VI_ASSERT(Routine->State != Coexecution::Finished, "coroutine should not be empty");
			if (Current != Routine || Routine->State != Coexecution::Active)
				return;

			Routine->State = Coexecution::Suspended;
			Suspend();
		}
		void Costate::Deactivate(Coroutine* Routine, const TaskCallback& AfterSuspend)
		{
			VI_ASSERT(Routine != nullptr, "coroutine should be set");
			Routine->Return = AfterSuspend;
			Deactivate(Routine);
		}
		void Costate::Deactivate(Coroutine* Routine, TaskCallback&& AfterSuspend)
		{
			VI_ASSERT(Routine != nullptr, "coroutine should be set");
			Routine->Return = std::move(AfterSuspend);
			Deactivate(Routine);
		}
		void Costate::Clear()
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot call outside costate thread");
			for (auto& Routine : Cached)
				VI_DELETE(Coroutine, Routine);
			Cached.clear();
		}
		bool Costate::Dispatch()
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot dispatch coroutine outside costate thread");
			VI_ASSERT(!Current, "cannot dispatch coroutines inside another coroutine");
			size_t Executions = 0;
		Retry:
			for (auto* Routine : Used)
			{
				switch (Execute(Routine))
				{
					case Coexecution::Active:
						++Executions;
						break;
					case Coexecution::Suspended:
					case Coexecution::Resumable:
						break;
					case Coexecution::Finished:
						++Executions;
						Reuse(Routine);
						goto Retry;
				}
			}

			return Executions > 0;
		}
		bool Costate::Suspend()
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot suspend coroutine outside costate thread");

			Coroutine* Routine = Current;
			if (!Routine || Routine->Master != this)
				return false;
#ifdef VI_FCTX
			Current = nullptr;
			jump_fcontext(Master->Context, (void*)this);
#elif VI_MICROSOFT
			Current = nullptr;
			SwitchToFiber(Master->Context);
#else
			char Bottom = 0;
			char* Top = Routine->Slave->Stack + Size;
			if (size_t(Top - &Bottom) > Size)
				return false;

			Current = nullptr;
			swapcontext(&Routine->Slave->Context, &Master->Context);
#endif
			return true;
		}
		bool Costate::HasResumableCoroutines() const
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot call outside costate thread");
			for (const auto& Item : Used)
			{
				if (Item->State == Coexecution::Active || Item->State == Coexecution::Resumable)
					return true;
			}

			return false;
		}
		bool Costate::HasAliveCoroutines() const
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), "cannot call outside costate thread");
			for (const auto& Item : Used)
			{
				if (Item->State != Coexecution::Finished)
					return true;
			}

			return false;
		}
		size_t Costate::GetCount() const
		{
			return Used.size();
		}
		Coroutine* Costate::GetCurrent() const
		{
			return Current;
		}
		Costate* Costate::Get()
		{
			return Cothread;
		}
		Coroutine* Costate::GetCoroutine()
		{
			return Cothread ? Cothread->Current : nullptr;
		}
		bool Costate::GetState(Costate** State, Coroutine** Routine)
		{
			VI_ASSERT(State != nullptr, "state should be set");
			VI_ASSERT(Routine != nullptr, "state should be set");
			*Routine = (Cothread ? Cothread->Current : nullptr);
			*State = Cothread;

			return *Routine != nullptr;
		}
		bool Costate::IsCoroutine()
		{
			return Cothread ? Cothread->Current != nullptr : false;
		}
		void VI_COCALL Costate::ExecutionEntry(VI_CODATA)
		{
#ifdef VI_FCTX
			transfer_t* Transfer = (transfer_t*)Context;
			Costate* State = (Costate*)Transfer->data;
			State->Master->Context = Transfer->fctx;
#elif VI_MICROSOFT
			Costate* State = (Costate*)Context;
#else
			Costate* State = (Costate*)Unpack2_64(X, Y);
#endif
			Cothread = State;
			VI_ASSERT(State != nullptr, "costate should be set");
			Coroutine* Routine = State->Current;
			if (Routine != nullptr)
			{
			Reuse:
				if (Routine->Callback)
					Routine->Callback();
				Routine->Return = nullptr;
				Routine->State = Coexecution::Finished;
			}

			State->Current = nullptr;
#ifdef VI_FCTX
			jump_fcontext(State->Master->Context, Context);
#elif VI_MICROSOFT
			SwitchToFiber(State->Master->Context);
#else
			swapcontext(&Routine->Slave->Context, &State->Master->Context);
#endif
			if (Routine != nullptr && Routine->Callback)
				goto Reuse;
		}

		Schedule::Desc::Desc() : Desc(std::max<uint32_t>(2, OS::CPU::GetQuantityInfo().Logical) - 1)
		{
		}
		Schedule::Desc::Desc(size_t Cores) : PreallocatedSize(0), StackSize(STACK_SIZE), MaxCoroutines(96), Timeout(std::chrono::milliseconds(2000)), Parallel(true)
		{
			if (!Cores)
				Cores = 1;
#ifndef VI_CXX20
			size_t Async = (size_t)std::max(std::ceil(Cores * 0.20), 1.0);
#else
			size_t Async = 0;
#endif
			size_t Timeout = 1;
			size_t Normal = std::max<size_t>(std::min<size_t>(Cores - Async - Timeout, Cores), 1);
			Threads[((size_t)Difficulty::Async)] = Async;
			Threads[((size_t)Difficulty::Normal)] = Normal;
			Threads[((size_t)Difficulty::Timeout)] = Timeout;
			MaxCoroutines = std::min<size_t>(Cores * 8, 256);
		}

		Schedule::Schedule() noexcept : Generation(0), Debug(nullptr), Suspended(false), Enqueue(true), Terminate(false), Active(false), Immediate(false)
		{
			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
				Queues[i] = VI_NEW(ConcurrentQueue);
		}
		Schedule::~Schedule() noexcept
		{
			Stop();
			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
				VI_DELETE(ConcurrentQueue, Queues[i]);

			VI_RELEASE(Dispatcher.State);
			Scripting::VirtualMachine::CleanupThisThread();
		}
		TaskId Schedule::GetTaskId()
		{
			TaskId Id = ++Generation;
			while (Id == INVALID_TASK_ID)
				Id = ++Generation;
			return Id;
		}
		TaskId Schedule::SetInterval(uint64_t Milliseconds, const TaskCallback& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue || Immediate)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			PostDebug(ThreadTask::EnqueueTimer, 1);
#endif
			VI_MEASURE(Timings::Atomic);
			auto Duration = std::chrono::microseconds(Milliseconds * 1000);
			auto Expires = GetClock() + Duration;
			auto Id = GetTaskId();
			auto Queue = Queues[(size_t)Difficulty::Timeout];

			UMutex<std::mutex> Lock(Queue->Update);
			Queue->Timers.emplace(std::make_pair(GetTimeout(Expires), Timeout(Callback, Duration, Id, true)));
			Queue->Notify.notify_one();
			return Id;
		}
		TaskId Schedule::SetInterval(uint64_t Milliseconds, TaskCallback&& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue || Immediate)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			PostDebug(ThreadTask::EnqueueTimer, 1);
#endif
			VI_MEASURE(Timings::Atomic);
			auto Duration = std::chrono::microseconds(Milliseconds * 1000);
			auto Expires = GetClock() + Duration;
			auto Id = GetTaskId();
			auto Queue = Queues[(size_t)Difficulty::Timeout];

			UMutex<std::mutex> Lock(Queue->Update);
			Queue->Timers.emplace(std::make_pair(GetTimeout(Expires), Timeout(std::move(Callback), Duration, Id, true)));
			Queue->Notify.notify_one();
			return Id;
		}
		TaskId Schedule::SetTimeout(uint64_t Milliseconds, const TaskCallback& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue || Immediate)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			PostDebug(ThreadTask::EnqueueTimer, 1);
#endif
			VI_MEASURE(Timings::Atomic);
			auto Duration = std::chrono::microseconds(Milliseconds * 1000);
			auto Expires = GetClock() + Duration;
			auto Id = GetTaskId();
			auto Queue = Queues[(size_t)Difficulty::Timeout];

			UMutex<std::mutex> Lock(Queue->Update);
			Queue->Timers.emplace(std::make_pair(GetTimeout(Expires), Timeout(Callback, Duration, Id, false)));
			Queue->Notify.notify_one();
			return Id;
		}
		TaskId Schedule::SetTimeout(uint64_t Milliseconds, TaskCallback&& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue || Immediate)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			PostDebug(ThreadTask::EnqueueTimer, 1);
#endif
			VI_MEASURE(Timings::Atomic);
			auto Duration = std::chrono::microseconds(Milliseconds * 1000);
			auto Expires = GetClock() + Duration;
			auto Id = GetTaskId();
			auto Queue = Queues[(size_t)Difficulty::Timeout];

			UMutex<std::mutex> Lock(Queue->Update);
			Queue->Timers.emplace(std::make_pair(GetTimeout(Expires), Timeout(std::move(Callback), Duration, Id, false)));
			Queue->Notify.notify_one();
			return Id;
		}
		bool Schedule::SetTask(const TaskCallback& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue)
				return false;

			if (Immediate)
			{
				Callback();
				return true;
			}
#ifndef NDEBUG
			PostDebug(ThreadTask::EnqueueTask, 1);
#endif
			VI_MEASURE(Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Normal];
			Queue->Tasks.enqueue(Callback);
			return true;
		}
		bool Schedule::SetTask(TaskCallback&& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue)
				return false;

			if (Immediate)
			{
				Callback();
				return true;
			}
#ifndef NDEBUG
			PostDebug(ThreadTask::EnqueueTask, 1);
#endif
			VI_MEASURE(Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Normal];
			Queue->Tasks.enqueue(std::move(Callback));
			return true;
		}
		bool Schedule::SetCoroutine(const TaskCallback& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue)
				return false;

			if (Immediate)
			{
				Callback();
				return true;
			}

			VI_MEASURE(Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Async];
			Queue->Tasks.enqueue(Callback);

			UMutex<std::mutex> Lock(Queue->Update);
			for (auto* Thread : Threads[(size_t)Difficulty::Async])
				Thread->Notify.notify_one();
			return true;
		}
		bool Schedule::SetCoroutine(TaskCallback&& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue)
				return false;

			if (Immediate)
			{
				Callback();
				return true;
			}
#ifndef NDEBUG
			PostDebug(ThreadTask::EnqueueCoroutine, 1);
#endif
			VI_MEASURE(Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Async];
			Queue->Tasks.enqueue(std::move(Callback));

			UMutex<std::mutex> Lock(Queue->Update);
			for (auto* Thread : Threads[(size_t)Difficulty::Async])
				Thread->Notify.notify_one();
			return true;
		}
		bool Schedule::SetDebugCallback(const ThreadDebugCallback& Callback)
		{
#ifndef NDEBUG
			Debug = Callback;
			return true;
#else
			return false;
#endif
		}
		bool Schedule::SetImmediate(bool Enabled)
		{
			if (Active || !Enqueue)
				return false;

			Immediate = Enabled;
			return true;
		}
		bool Schedule::ClearTimeout(TaskId Target)
		{
			VI_MEASURE(Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Timeout];
			UMutex<std::mutex> Lock(Queue->Update);
			for (auto It = Queue->Timers.begin(); It != Queue->Timers.end(); ++It)
			{
				if (It->second.Id == Target)
				{
					Queue->Timers.erase(It);
					Queue->Notify.notify_all();
					return true;
				}
			}
			return false;
		}
		bool Schedule::Trigger(Difficulty Type)
		{
			auto* Queue = Queues[(size_t)Type];
			switch (Type)
			{
				case Difficulty::Timeout:
				{
					if (Queue->Timers.empty())
						return false;

					if (Suspended)
						return true;

					auto Clock = GetClock();
					auto It = Queue->Timers.begin();
					if (It->first >= Clock)
						return true;
#ifndef NDEBUG
					PostDebug(ThreadTask::ProcessTimer, 1);
#endif
					if (It->second.Alive && Active)
					{
						Timeout Next(std::move(It->second));
						Queue->Timers.erase(It);

						SetTask([this, Queue, Next = std::move(Next)]() mutable
						{
							Next.Callback();
							UMutex<std::mutex> Lock(Queue->Update);
							Queue->Timers.emplace(std::make_pair(GetTimeout(GetClock() + Next.Expires), std::move(Next)));
							Queue->Notify.notify_one();
						});
					}
					else
					{
						SetTask(std::move(It->second.Callback));
						Queue->Timers.erase(It);
					}
#ifndef NDEBUG
					PostDebug(ThreadTask::Awake, 0);
#endif
					return true;
				}
				case Difficulty::Async:
				{
					Dispatcher.Events.resize(Policy.MaxCoroutines);
					if (!Dispatcher.State)
						Dispatcher.State = new Costate(Policy.StackSize);

					size_t Pending = Dispatcher.State->GetCount();
					size_t Left = Policy.MaxCoroutines - Pending;
					size_t Count = Left, Passes = Pending;

					if (Suspended)
						return Pending > 0;

					while (Left > 0 && Count > 0)
					{
						Count = Queue->Tasks.try_dequeue_bulk(Dispatcher.Events.begin(), Left);
						Left -= Count;
						Passes += Count;
#ifndef NDEBUG
						PostDebug(ThreadTask::ConsumeCoroutine, Count);
#endif
						for (size_t i = 0; i < Count; ++i)
						{
							TaskCallback& Data = Dispatcher.Events[i];
							if (Data != nullptr)
								Dispatcher.State->Pop(std::move(Data));
						}
					}
#ifndef NDEBUG
					PostDebug(ThreadTask::ProcessCoroutine, Dispatcher.State->GetCount());
#endif
					while (Dispatcher.State->Dispatch())
						++Passes;
#ifndef NDEBUG
					PostDebug(ThreadTask::Awake, 0);
#endif
					return Passes > 0;
				}
				case Difficulty::Normal:
				{
					if (Suspended)
						return Queue->Tasks.size_approx() > 0;

					size_t Count = Queue->Tasks.try_dequeue_bulk(Dispatcher.Tasks, EVENTS_SIZE);
#ifndef NDEBUG
					PostDebug(ThreadTask::ProcessTask, Count);
#endif
					for (size_t i = 0; i < Count; ++i)
					{
						VI_MEASURE(Timings::Intensive);
						TaskCallback Data(std::move(Dispatcher.Tasks[i]));
						if (Data != nullptr)
							Data();
					}
#ifndef NDEBUG
					PostDebug(ThreadTask::Awake, 0);
#endif
					return Count > 0;
				}
				default:
					break;
			}

			return false;
		}
		bool Schedule::Start(const Desc& NewPolicy)
		{
			VI_ASSERT(!Active, "queue should be stopped");
			VI_ASSERT(NewPolicy.StackSize > 0, "stack size should not be zero");
			VI_ASSERT(NewPolicy.MaxCoroutines > 0, "there must be at least one coroutine");
			VI_TRACE("[schedule] start 0x%" PRIXPTR " on thread %s", (void*)this, OS::Process::GetThreadId(std::this_thread::get_id()).c_str());

			Policy = NewPolicy;
			Immediate = false;
			Active = true;

			if (!Policy.Parallel)
			{
				InitializeThread(nullptr, true);
				return true;
			}

			size_t Index = 0;
			for (size_t j = 0; j < Policy.Threads[(size_t)Difficulty::Async]; j++)
				PushThread(Difficulty::Async, Index++, j, false);

			for (size_t j = 0; j < Policy.Threads[(size_t)Difficulty::Normal]; j++)
				PushThread(Difficulty::Normal, Index++, j, false);

			if (Policy.Threads[(size_t)Difficulty::Timeout] > 0)
				PushThread(Difficulty::Timeout, Policy.Ping ? 0 : Index++, 0, !!Policy.Ping);

			return true;
		}
		bool Schedule::Stop()
		{
			VI_TRACE("[schedule] stop 0x%" PRIXPTR " on thread %s", (void*)this, OS::Process::GetThreadId(std::this_thread::get_id()).c_str());
			UMutex<std::mutex> Unique(Exclusive);
			if (!Active && !Terminate)
				return false;

			Active = Enqueue = false;
			Wakeup();

			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
			{
				for (auto* Thread : Threads[i])
				{
					if (!PopThread(Thread))
					{
						Terminate = true;
						return false;
					}
				}
			}

			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
			{
				auto* Queue = Queues[i];
				UMutex<std::mutex> Lock(Queue->Update);
				Queue->Timers.clear();
			}

			Terminate = false;
			Enqueue = true;
			ChunkCleanup();
			return true;
		}
		bool Schedule::Wakeup()
		{
			VI_TRACE("[schedule] wakeup 0x%" PRIXPTR " on thread %s", (void*)this, OS::Process::GetThreadId(std::this_thread::get_id()).c_str());
			TaskCallback Dummy[EVENTS_SIZE * 2] = { nullptr };
			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
			{
				auto* Queue = Queues[i];
				UMutex<std::mutex> Lock(Queue->Update);
				Queue->Notify.notify_all();

				for (auto* Thread : Threads[i])
				{
					Thread->Notify.notify_all();
					if (Thread->Type != Difficulty::Timeout)
						Queue->Tasks.enqueue_bulk(Dummy, EVENTS_SIZE);
				}
			}

			return true;
		}
		bool Schedule::Dispatch()
		{
			size_t Passes = 0;
			VI_MEASURE(Timings::Intensive);
			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
				Passes += (size_t)Trigger((Difficulty)i);

			return Passes > 0;
		}
		bool Schedule::TriggerThread(Difficulty Type, ThreadPtr* Thread)
		{
			auto* Queue = Queues[(size_t)Type];
			String ThreadId = OS::Process::GetThreadId(Thread->Id);
			InitializeThread(Thread, true);

			switch (Type)
			{
				case Difficulty::Timeout:
				{
					if (Thread->Daemon)
						VI_DEBUG("[schedule] acquire thread %s (timers)", ThreadId.c_str());
					else
						VI_DEBUG("[schedule] spawn thread %s (timers)", ThreadId.c_str());

					do
					{
						std::unique_lock<std::mutex> Lock(Queue->Update);
					Retry:
#ifndef NDEBUG
						PostDebug(ThreadTask::Awake, 0);
#endif
						std::chrono::microseconds When = std::chrono::microseconds(0);
						if (!Suspended && !Queue->Timers.empty())
						{
							auto Clock = GetClock();
							auto It = Queue->Timers.begin();
							if (It->first <= Clock)
							{
#ifndef NDEBUG
								PostDebug(ThreadTask::ProcessTimer, 1);
#endif
								if (It->second.Alive)
								{
									Timeout Next(std::move(It->second));
									Queue->Timers.erase(It);

									SetTask([this, Queue, Next = std::move(Next)]() mutable
									{
										Next.Callback();
										UMutex<std::mutex> Lock(Queue->Update);
										Queue->Timers.emplace(std::make_pair(GetTimeout(GetClock() + Next.Expires), std::move(Next)));
										Queue->Notify.notify_one();
									});
								}
								else
								{
									SetTask(std::move(It->second.Callback));
									Queue->Timers.erase(It);
								}

								goto Retry;
							}
							else
							{
								When = It->first - Clock;
								if (When > Policy.Timeout)
									When = Policy.Timeout;
							}
						}
						else
							When = Policy.Timeout;
#ifndef NDEBUG
						PostDebug(ThreadTask::Sleep, 0);
#endif
						Queue->Notify.wait_for(Lock, When);
					} while (ThreadActive(Thread));
					break;
				}
				case Difficulty::Async:
				{
					if (Thread->Daemon)
						VI_DEBUG("[schedule] acquire thread %s (coroutines)", ThreadId.c_str());
					else
						VI_DEBUG("[schedule] spawn thread %s (coroutines)", ThreadId.c_str());

					ReceiveToken Token(Queue->Tasks);
					Costate* State = new Costate(Policy.StackSize);
					State->ExternalCondition = &Thread->Notify;
					State->ExternalMutex = &Thread->Update;

					Vector<TaskCallback> Events;
					Events.resize(Policy.MaxCoroutines);

					do
					{
#ifndef NDEBUG
						PostDebug(ThreadTask::Awake, 0);
#endif
						size_t Left = Policy.MaxCoroutines - State->GetCount();
						while (Left > 0)
						{
							size_t Count = Queue->Tasks.try_dequeue_bulk(Token, Events.begin(), Left);
							if (!Count)
								break;
#ifndef NDEBUG
							PostDebug(ThreadTask::EnqueueCoroutine, Count);
#endif
							Left -= Count;
							while (Count--)
							{
								TaskCallback& Data = Events[Count];
								if (Data)
									State->Pop(std::move(Data));
							}
						}

						if (!Suspended)
						{
							VI_MEASURE(Timings::Frame);
#ifndef NDEBUG
							PostDebug(ThreadTask::ProcessCoroutine, State->GetCount());
#endif
							State->Dispatch();
						}
#ifndef NDEBUG
						PostDebug(ThreadTask::Sleep, 0);
#endif
						std::unique_lock<std::mutex> Lock(Thread->Update);
						Thread->Notify.wait_for(Lock, Policy.Timeout, [this, Queue, State, Thread]()
						{
							if (Suspended)
								return false;

							if (!ThreadActive(Thread) || State->HasResumableCoroutines())
								return true;

							return Queue->Tasks.size_approx() > 0 && State->GetCount() + 1 < Policy.MaxCoroutines;
						});
					} while (ThreadActive(Thread));

					VI_RELEASE(State);
					break;
				}
				case Difficulty::Normal:
				{
					if (Thread->Daemon)
						VI_DEBUG("[schedule] acquire thread %s (tasks)", ThreadId.c_str());
					else
						VI_DEBUG("[schedule] spawn thread %s (tasks)", ThreadId.c_str());

					ReceiveToken Token(Queue->Tasks);
					TaskCallback Events[EVENTS_SIZE];

					do
					{
#ifndef NDEBUG
						PostDebug(ThreadTask::Sleep, 0);
#endif
						if (Suspended)
						{
							std::unique_lock<std::mutex> Lock(Queue->Update);
							Queue->Notify.wait_for(Lock, Policy.Timeout);
							continue;
						}

						size_t Count = Queue->Tasks.wait_dequeue_bulk_timed(Token, Events, EVENTS_SIZE, Policy.Timeout);
#ifndef NDEBUG
						PostDebug(ThreadTask::Awake, 0);
						PostDebug(ThreadTask::ProcessTask, Count);
#endif
						while (Count--)
						{
							VI_MEASURE(Timings::Intensive);
							TaskCallback Data(std::move(Events[Count]));
							if (Data)
								Data();
						}
					} while (ThreadActive(Thread));
					break;
				}
				default:
					break;
			}

			if (Thread->Daemon)
				VI_DEBUG("[schedule] release thread %s", ThreadId.c_str());
			else
				VI_DEBUG("[schedule] join thread %s", ThreadId.c_str());

			Scripting::VirtualMachine::CleanupThisThread();
			InitializeThread(nullptr, true);
			return true;
		}
		bool Schedule::ThreadActive(ThreadPtr* Thread)
		{
			if (Thread->Daemon)
				return Active && (Policy.Ping ? Policy.Ping() : true);

			return Active;
		}
		bool Schedule::ChunkCleanup()
		{
			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
			{
				for (auto* Thread : Threads[i])
					VI_DELETE(ThreadPtr, Thread);
				Threads[i].clear();
			}

			return true;
		}
		bool Schedule::PushThread(Difficulty Type, size_t GlobalIndex, size_t LocalIndex, bool IsDaemon)
		{
			ThreadPtr* Thread = VI_NEW(ThreadPtr, Type, Policy.PreallocatedSize, GlobalIndex, LocalIndex, IsDaemon);
			if (!Thread->Daemon)
			{
				Thread->Handle = std::thread(&Schedule::TriggerThread, this, Type, Thread);
				Thread->Id = Thread->Handle.get_id();
			}
			else
				Thread->Id = std::this_thread::get_id();
#ifndef NDEBUG
			PostDebug(ThreadTask::Spawn, 0);
#endif
			Threads[(size_t)Type].emplace_back(Thread);
			return Thread->Daemon ? TriggerThread(Type, Thread) : Thread->Handle.joinable();
		}
		bool Schedule::PopThread(ThreadPtr* Thread)
		{
			if (Thread->Daemon)
				return true;

			if (Thread->Id == std::this_thread::get_id())
				return false;

			if (Thread->Handle.joinable())
				Thread->Handle.join();
#ifndef NDEBUG
			PostDebug(ThreadTask::Despawn, 0);
#endif
			return true;
		}
		bool Schedule::IsActive() const
		{
			return Active;
		}
		bool Schedule::CanEnqueue() const
		{
			return Enqueue;
		}
		bool Schedule::HasTasks(Difficulty Type) const
		{
			VI_ASSERT(Type != Difficulty::Count, "difficulty should be set");
			auto* Queue = Queues[(size_t)Type];
			switch (Type)
			{
				case Difficulty::Async:
				case Difficulty::Normal:
					return Queue->Tasks.size_approx() > 0;
				case Difficulty::Timeout:
					return !Queue->Timers.empty();
				default:
					return false;
			}
		}
		bool Schedule::HasAnyTasks() const
		{
			return HasTasks(Difficulty::Normal) || HasTasks(Difficulty::Async) || HasTasks(Difficulty::Timeout);
		}
		bool Schedule::IsSuspended() const
		{
			return Suspended;
		}
		void Schedule::Suspend()
		{
			Suspended = true;
		}
		void Schedule::Resume()
		{
			Suspended = false;
			Wakeup();
		}
		bool Schedule::PostDebug(ThreadTask State, size_t Tasks)
		{
			if (!Debug)
				return false;

			Debug(ThreadMessage(GetThread(), State, Tasks));
			return true;
		}
		size_t Schedule::GetThreadGlobalIndex()
		{
			auto* Thread = GetThread();
			return Thread ? Thread->GlobalIndex : 0;
		}
		size_t Schedule::GetThreadLocalIndex()
		{
			auto* Thread = GetThread();
			return Thread ? Thread->LocalIndex : 0;
		}
		size_t Schedule::GetTotalThreads() const
		{
			size_t Size = 0;
			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
				Size += GetThreads((Difficulty)i);

			return Size;
		}
		size_t Schedule::GetThreads(Difficulty Type) const
		{
			VI_ASSERT(Type != Difficulty::Count, "difficulty should be set");
			return Threads[(size_t)Type].size();
		}
		const Schedule::ThreadPtr* Schedule::InitializeThread(ThreadPtr* Source, bool Update) const
		{
			static thread_local ThreadPtr* Pointer = nullptr;
			if (Update)
				Pointer = Source;
			return Pointer;
		}
		const Schedule::ThreadPtr* Schedule::GetThread() const
		{
			return InitializeThread(nullptr, false);
		}
		const Schedule::Desc& Schedule::GetPolicy() const
		{
			return Policy;
		}
		std::chrono::microseconds Schedule::GetTimeout(std::chrono::microseconds Clock)
		{
			auto* Queue = Queues[(size_t)Difficulty::Timeout];
			while (Queue->Timers.find(Clock) != Queue->Timers.end())
				++Clock;
			return Clock;
		}
		std::chrono::microseconds Schedule::GetClock()
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
		}
		bool Schedule::IsAvailable()
		{
			return HasInstance() && Get()->Active;
		}

		Schema::Schema(const Variant& Base) noexcept : Nodes(nullptr), Parent(nullptr), Saved(true), Value(Base)
		{
		}
		Schema::Schema(Variant&& Base) noexcept : Nodes(nullptr), Parent(nullptr), Saved(true), Value(std::move(Base))
		{
		}
		Schema::~Schema() noexcept
		{
			Unlink();
			Clear();
		}
		UnorderedMap<String, size_t> Schema::GetNames() const
		{
			UnorderedMap<String, size_t> Mapping;
			size_t Index = 0;

			GenerateNamingTable(this, &Mapping, Index);
			return Mapping;
		}
		Vector<Schema*> Schema::FindCollection(const String& Name, bool Deep) const
		{
			Vector<Schema*> Result;
			if (!Nodes)
				return Result;

			for (auto Value : *Nodes)
			{
				if (Value->Key == Name)
					Result.push_back(Value);

				if (!Deep)
					continue;

				Vector<Schema*> New = Value->FindCollection(Name);
				for (auto& Subvalue : New)
					Result.push_back(Subvalue);
			}

			return Result;
		}
		Vector<Schema*> Schema::FetchCollection(const String& Notation, bool Deep) const
		{
			Vector<String> Names = Stringify::Split(Notation, '.');
			if (Names.empty())
				return Vector<Schema*>();

			if (Names.size() == 1)
				return FindCollection(*Names.begin());

			Schema* Current = Find(*Names.begin(), Deep);
			if (!Current)
				return Vector<Schema*>();

			for (auto It = Names.begin() + 1; It != Names.end() - 1; ++It)
			{
				Current = Current->Find(*It, Deep);
				if (!Current)
					return Vector<Schema*>();
			}

			return Current->FindCollection(*(Names.end() - 1), Deep);
		}
		Vector<Schema*> Schema::GetAttributes() const
		{
			Vector<Schema*> Attributes;
			if (!Nodes)
				return Attributes;

			for (auto It : *Nodes)
			{
				if (It->IsAttribute())
					Attributes.push_back(It);
			}

			return Attributes;
		}
		Vector<Schema*>& Schema::GetChilds()
		{
			Allocate();
			return *Nodes;
		}
		Schema* Schema::Find(const String& Name, bool Deep) const
		{
			if (!Nodes)
				return nullptr;

			if (Stringify::HasInteger(Name))
			{
				size_t Index = (size_t)*FromString<uint64_t>(Name);
				if (Index < Nodes->size())
					return (*Nodes)[Index];
			}

			for (auto K : *Nodes)
			{
				if (K->Key == Name)
					return K;

				if (!Deep)
					continue;

				Schema* V = K->Find(Name);
				if (V != nullptr)
					return V;
			}

			return nullptr;
		}
		Schema* Schema::Fetch(const String& Notation, bool Deep) const
		{
			Vector<String> Names = Stringify::Split(Notation, '.');
			if (Names.empty())
				return nullptr;

			Schema* Current = Find(*Names.begin(), Deep);
			if (!Current)
				return nullptr;

			for (auto It = Names.begin() + 1; It != Names.end(); ++It)
			{
				Current = Current->Find(*It, Deep);
				if (!Current)
					return nullptr;
			}

			return Current;
		}
		Schema* Schema::GetParent() const
		{
			return Parent;
		}
		Schema* Schema::GetAttribute(const String& Name) const
		{
			return Get(':' + Name);
		}
		Variant Schema::FetchVar(const String& fKey, bool Deep) const
		{
			Schema* Result = Fetch(fKey, Deep);
			if (!Result)
				return Var::Undefined();

			return Result->Value;
		}
		Variant Schema::GetVar(size_t Index) const
		{
			Schema* Result = Get(Index);
			if (!Result)
				return Var::Undefined();

			return Result->Value;
		}
		Variant Schema::GetVar(const String& fKey) const
		{
			Schema* Result = Get(fKey);
			if (!Result)
				return Var::Undefined();

			return Result->Value;
		}
		Variant Schema::GetAttributeVar(const String& Key) const
		{
			return GetVar(':' + Key);
		}
		Schema* Schema::Get(size_t Index) const
		{
			VI_ASSERT(Nodes != nullptr, "there must be at least one node");
			VI_ASSERT(Index < Nodes->size(), "index outside of range");

			return (*Nodes)[Index];
		}
		Schema* Schema::Get(const String& Name) const
		{
			VI_ASSERT(!Name.empty(), "name should not be empty");
			if (!Nodes)
				return nullptr;

			for (auto Schema : *Nodes)
			{
				if (Schema->Key == Name)
					return Schema;
			}

			return nullptr;
		}
		Schema* Schema::Set(const String& Name)
		{
			return Set(Name, Var::Object());
		}
		Schema* Schema::Set(const String& Name, const Variant& Base)
		{
			if (Value.Type == VarType::Object && Nodes != nullptr)
			{
				for (auto Node : *Nodes)
				{
					if (Node->Key == Name)
					{
						Node->Value = Base;
						Node->Saved = false;
						Node->Clear();
						Saved = false;

						return Node;
					}
				}
			}

			Schema* Result = new Schema(Base);
			Result->Key.assign(Name);
			Result->Attach(this);

			Allocate();
			Nodes->push_back(Result);
			return Result;
		}
		Schema* Schema::Set(const String& Name, Variant&& Base)
		{
			if (Value.Type == VarType::Object && Nodes != nullptr)
			{
				for (auto Node : *Nodes)
				{
					if (Node->Key == Name)
					{
						Node->Value = std::move(Base);
						Node->Saved = false;
						Node->Clear();
						Saved = false;

						return Node;
					}
				}
			}

			Schema* Result = new Schema(std::move(Base));
			Result->Key.assign(Name);
			Result->Attach(this);

			Allocate();
			Nodes->push_back(Result);
			return Result;
		}
		Schema* Schema::Set(const String& Name, Schema* Base)
		{
			if (!Base)
				return Set(Name, Var::Null());

			Base->Key.assign(Name);
			Base->Attach(this);

			if (Value.Type == VarType::Object && Nodes != nullptr)
			{
				for (auto It = Nodes->begin(); It != Nodes->end(); ++It)
				{
					if ((*It)->Key != Name)
						continue;

					if (*It == Base)
						return Base;

					(*It)->Parent = nullptr;
					VI_RELEASE(*It);
					*It = Base;

					return Base;
				}
			}

			Allocate();
			Nodes->push_back(Base);
			return Base;
		}
		Schema* Schema::SetAttribute(const String& Name, const Variant& fValue)
		{
			return Set(':' + Name, fValue);
		}
		Schema* Schema::SetAttribute(const String& Name, Variant&& fValue)
		{
			return Set(':' + Name, std::move(fValue));
		}
		Schema* Schema::Push(const Variant& Base)
		{
			Schema* Result = new Schema(Base);
			Result->Attach(this);

			Allocate();
			Nodes->push_back(Result);
			return Result;
		}
		Schema* Schema::Push(Variant&& Base)
		{
			Schema* Result = new Schema(std::move(Base));
			Result->Attach(this);

			Allocate();
			Nodes->push_back(Result);
			return Result;
		}
		Schema* Schema::Push(Schema* Base)
		{
			if (!Base)
				return Push(Var::Null());

			Base->Attach(this);

			Allocate();
			Nodes->push_back(Base);
			return Base;
		}
		Schema* Schema::Pop(size_t Index)
		{
			VI_ASSERT(Nodes != nullptr, "there must be at least one node");
			VI_ASSERT(Index < Nodes->size(), "index outside of range");

			auto It = Nodes->begin() + Index;
			Schema* Base = *It;
			Base->Parent = nullptr;
			VI_RELEASE(Base);
			Nodes->erase(It);

			return this;
		}
		Schema* Schema::Pop(const String& Name)
		{
			if (!Nodes)
				return this;

			for (auto It = Nodes->begin(); It != Nodes->end(); ++It)
			{
				if (!*It || (*It)->Key != Name)
					continue;

				(*It)->Parent = nullptr;
				VI_RELEASE(*It);
				Nodes->erase(It);
				break;
			}

			return this;
		}
		Schema* Schema::Copy() const
		{
			Schema* New = new Schema(Value);
			New->Key.assign(Key);
			New->Saved = Saved;

			if (!Nodes)
				return New;

			New->Allocate(*Nodes);
			for (auto*& Item : *New->Nodes)
			{
				if (Item != nullptr)
					Item = Item->Copy();
			}

			return New;
		}
		bool Schema::Rename(const String& Name, const String& NewName)
		{
			VI_ASSERT(!Name.empty() && !NewName.empty(), "name and new name should not be empty");

			Schema* Result = Get(Name);
			if (!Result)
				return false;

			Result->Key = NewName;
			return true;
		}
		bool Schema::Has(const String& Name) const
		{
			return Fetch(Name) != nullptr;
		}
		bool Schema::HasAttribute(const String& Name) const
		{
			return Fetch(':' + Name) != nullptr;
		}
		bool Schema::Empty() const
		{
			return !Nodes || Nodes->empty();
		}
		bool Schema::IsAttribute() const
		{
			if (Key.size() < 2)
				return false;

			return Key.front() == ':';
		}
		bool Schema::IsSaved() const
		{
			return Saved;
		}
		size_t Schema::Size() const
		{
			return Nodes ? Nodes->size() : 0;
		}
		String Schema::GetName() const
		{
			return IsAttribute() ? Key.substr(1) : Key;
		}
		void Schema::Join(Schema* Other, bool AppendOnly)
		{
			VI_ASSERT(Other != nullptr && Value.IsObject(), "other should be object and not empty");
			auto FillArena = [](UnorderedMap<String, Schema*>& Nodes, Schema* Base)
			{
				if (!Base->Nodes)
					return;

				for (auto& Node : *Base->Nodes)
				{
					auto& Next = Nodes[Node->Key];
					VI_RELEASE(Next);
					Next = Node;
				}

				Base->Nodes->clear();
			};

			Allocate();
			Nodes->reserve(Nodes->size() + Other->Nodes->size());
			Saved = false;

			if (!AppendOnly)
			{
				UnorderedMap<String, Schema*> Subnodes;
				Subnodes.reserve(Nodes->capacity());
				FillArena(Subnodes, this);
				FillArena(Subnodes, Other);

				for (auto& Node : Subnodes)
					Nodes->push_back(Node.second);
			}
			else if (Other->Nodes != nullptr)
			{
				Nodes->insert(Nodes->end(), Other->Nodes->begin(), Other->Nodes->end());
				Other->Nodes->clear();
			}

			for (auto& Node : *Nodes)
			{
				Node->Saved = false;
				Node->Parent = this;
			}
		}
		void Schema::Reserve(size_t Size)
		{
			Allocate();
			Nodes->reserve(Size);
		}
		void Schema::Unlink()
		{
			if (!Parent)
				return;

			if (!Parent->Nodes)
			{
				Parent = nullptr;
				return;
			}

			for (auto It = Parent->Nodes->begin(); It != Parent->Nodes->end(); ++It)
			{
				if (*It == this)
				{
					Parent->Nodes->erase(It);
					break;
				}
			}

			Parent = nullptr;
		}
		void Schema::Clear()
		{
			if (!Nodes)
				return;

			for (auto& Schema : *Nodes)
			{
				if (Schema != nullptr)
				{
					Schema->Parent = nullptr;
					VI_RELEASE(Schema);
				}
			}

			VI_DELETE(vector, Nodes);
			Nodes = nullptr;
		}
		void Schema::Save()
		{
			if (Nodes != nullptr)
			{
				for (auto& It : *Nodes)
				{
					if (It->Value.IsObject())
						It->Save();
					else
						It->Saved = true;
				}
			}

			Saved = true;
		}
		void Schema::Attach(Schema* Root)
		{
			Saved = false;
			if (Parent != nullptr && Parent->Nodes != nullptr)
			{
				for (auto It = Parent->Nodes->begin(); It != Parent->Nodes->end(); ++It)
				{
					if (*It == this)
					{
						Parent->Nodes->erase(It);
						break;
					}
				}
			}

			Parent = Root;
			if (Parent != nullptr)
				Parent->Saved = false;
		}
		void Schema::Allocate()
		{
			if (!Nodes)
				Nodes = VI_NEW(Vector<Schema*>);
		}
		void Schema::Allocate(const Vector<Schema*>& Other)
		{
			if (!Nodes)
				Nodes = VI_NEW(Vector<Schema*>, Other);
			else
				*Nodes = Other;
		}
		void Schema::Transform(Schema* Value, const SchemaNameCallback& Callback)
		{
			VI_ASSERT(!!Callback, "callback should not be empty");
			if (!Value)
				return;

			Value->Key = Callback(Value->Key);
			if (!Value->Nodes)
				return;

			for (auto* Item : *Value->Nodes)
				Transform(Item, Callback);
		}
		void Schema::ConvertToXML(Schema* Base, const SchemaWriteCallback& Callback)
		{
			VI_ASSERT(Base != nullptr && Callback, "base should be set and callback should not be empty");
			Vector<Schema*> Attributes = Base->GetAttributes();
			bool Scalable = (Base->Value.Size() > 0 || ((size_t)(Base->Nodes ? Base->Nodes->size() : 0) > (size_t)Attributes.size()));
			Callback(VarForm::Write_Tab, "", 0);
			Callback(VarForm::Dummy, "<", 1);
			Callback(VarForm::Dummy, Base->Key.c_str(), Base->Key.size());

			if (Attributes.empty())
			{
				if (Scalable)
					Callback(VarForm::Dummy, ">", 1);
				else
					Callback(VarForm::Dummy, " />", 3);
			}
			else
				Callback(VarForm::Dummy, " ", 1);

			for (auto It = Attributes.begin(); It != Attributes.end(); ++It)
			{
				String Key = (*It)->GetName();
				String Value = (*It)->Value.Serialize();

				Callback(VarForm::Dummy, Key.c_str(), Key.size());
				Callback(VarForm::Dummy, "=\"", 2);
				Callback(VarForm::Dummy, Value.c_str(), Value.size());
				++It;

				if (It == Attributes.end())
				{
					if (!Scalable)
					{
						Callback(VarForm::Write_Space, "\"", 1);
						Callback(VarForm::Dummy, "/>", 2);
					}
					else
						Callback(VarForm::Dummy, "\">", 2);
				}
				else
					Callback(VarForm::Write_Space, "\"", 1);

				--It;
			}

			Callback(VarForm::Tab_Increase, "", 0);
			if (Base->Value.Size() > 0)
			{
				String Text = Base->Value.Serialize();
				if (Base->Nodes != nullptr && !Base->Nodes->empty())
				{
					Callback(VarForm::Write_Line, "", 0);
					Callback(VarForm::Write_Tab, "", 0);
					Callback(VarForm::Dummy, Text.c_str(), Text.size());
					Callback(VarForm::Write_Line, "", 0);
				}
				else
					Callback(VarForm::Dummy, Text.c_str(), Text.size());
			}
			else
				Callback(VarForm::Write_Line, "", 0);

			if (Base->Nodes != nullptr)
			{
				for (auto&& It : *Base->Nodes)
				{
					if (!It->IsAttribute())
						ConvertToXML(It, Callback);
				}
			}

			Callback(VarForm::Tab_Decrease, "", 0);
			if (!Scalable)
				return;

			if (Base->Nodes != nullptr && !Base->Nodes->empty())
				Callback(VarForm::Write_Tab, "", 0);

			Callback(VarForm::Dummy, "</", 2);
			Callback(VarForm::Dummy, Base->Key.c_str(), Base->Key.size());
			Callback(Base->Parent ? VarForm::Write_Line : VarForm::Dummy, ">", 1);
		}
		void Schema::ConvertToJSON(Schema* Base, const SchemaWriteCallback& Callback)
		{
			VI_ASSERT(Base != nullptr && Callback, "base should be set and callback should not be empty");
			if (!Base->Value.IsObject())
			{
				String Value = Base->Value.Serialize();
				Stringify::Escape(Value);

				if (Base->Value.Type != VarType::String && Base->Value.Type != VarType::Binary)
				{
					if (Value.size() >= 2 && Value.front() == PREFIX_ENUM[0] && Value.back() == PREFIX_ENUM[0])
						Callback(VarForm::Dummy, Value.c_str() + 1, Value.size() - 2);
					else
						Callback(VarForm::Dummy, Value.c_str(), Value.size());
				}
				else
				{
					Callback(VarForm::Dummy, "\"", 1);
					Callback(VarForm::Dummy, Value.c_str(), Value.size());
					Callback(VarForm::Dummy, "\"", 1);
				}
				return;
			}

			size_t Size = (Base->Nodes ? Base->Nodes->size() : 0);
			bool Array = (Base->Value.Type == VarType::Array);

			if (Base->Parent != nullptr)
				Callback(VarForm::Write_Line, "", 0);

			Callback(VarForm::Write_Tab, "", 0);
			Callback(VarForm::Dummy, Array ? "[" : "{", 1);
			Callback(VarForm::Tab_Increase, "", 0);

			for (size_t i = 0; i < Size; i++)
			{
				auto* Next = (*Base->Nodes)[i];
				if (!Array)
				{
					Callback(VarForm::Write_Line, "", 0);
					Callback(VarForm::Write_Tab, "", 0);
					Callback(VarForm::Dummy, "\"", 1);
					Callback(VarForm::Dummy, Next->Key.c_str(), Next->Key.size());
					Callback(VarForm::Write_Space, "\":", 2);
				}

				if (!Next->Value.IsObject())
				{
					String Value = (Next->Value.GetType() == VarType::Undefined ? "null" : Next->Value.Serialize());
					Stringify::Escape(Value);

					if (Array)
					{
						Callback(VarForm::Write_Line, "", 0);
						Callback(VarForm::Write_Tab, "", 0);
					}

					if (!Next->Value.IsObject() && Next->Value.Type != VarType::String && Next->Value.Type != VarType::Binary)
					{
						if (Value.size() >= 2 && Value.front() == PREFIX_ENUM[0] && Value.back() == PREFIX_ENUM[0])
							Callback(VarForm::Dummy, Value.c_str() + 1, Value.size() - 2);
						else
							Callback(VarForm::Dummy, Value.c_str(), Value.size());
					}
					else
					{
						Callback(VarForm::Dummy, "\"", 1);
						Callback(VarForm::Dummy, Value.c_str(), Value.size());
						Callback(VarForm::Dummy, "\"", 1);
					}
				}
				else
					ConvertToJSON(Next, Callback);

				if (i + 1 < Size)
					Callback(VarForm::Dummy, ",", 1);
			}

			Callback(VarForm::Tab_Decrease, "", 0);
			Callback(VarForm::Write_Line, "", 0);

			if (Base->Parent != nullptr)
				Callback(VarForm::Write_Tab, "", 0);

			Callback(VarForm::Dummy, Array ? "]" : "}", 1);
		}
		void Schema::ConvertToJSONB(Schema* Base, const SchemaWriteCallback& Callback)
		{
			VI_ASSERT(Base != nullptr && Callback, "base should be set and callback should not be empty");
			UnorderedMap<String, size_t> Mapping = Base->GetNames();
			uint32_t Set = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint32_t)Mapping.size());
			uint64_t Version = OS::CPU::ToEndianness<uint64_t>(OS::CPU::Endian::Little, JSONB_VERSION);
			Callback(VarForm::Dummy, (const char*)&Version, sizeof(uint64_t));
			Callback(VarForm::Dummy, (const char*)&Set, sizeof(uint32_t));

			for (auto It = Mapping.begin(); It != Mapping.end(); ++It)
			{
				uint32_t Id = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint32_t)It->second);
				Callback(VarForm::Dummy, (const char*)&Id, sizeof(uint32_t));

				uint16_t Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint16_t)It->first.size());
				Callback(VarForm::Dummy, (const char*)&Size, sizeof(uint16_t));

				if (Size > 0)
					Callback(VarForm::Dummy, It->first.c_str(), sizeof(char) * (size_t)Size);
			}
			ProcessConvertionToJSONB(Base, &Mapping, Callback);
		}
		String Schema::ToXML(Schema* Value)
		{
			String Result;
			ConvertToXML(Value, [&](VarForm Type, const char* Buffer, size_t Length) { Result.append(Buffer, Length); });
			return Result;
		}
		String Schema::ToJSON(Schema* Value)
		{
			String Result;
			ConvertToJSON(Value, [&](VarForm Type, const char* Buffer, size_t Length) { Result.append(Buffer, Length); });
			return Result;
		}
		Vector<char> Schema::ToJSONB(Schema* Value)
		{
			Vector<char> Result;
			ConvertToJSONB(Value, [&](VarForm Type, const char* Buffer, size_t Length)
			{
				if (!Length)
					return;

				size_t Offset = Result.size();
				Result.resize(Offset + Length);
				memcpy(&Result[Offset], Buffer, Length);
			});
			return Result;
		}
		Expects<Schema*, Exceptions::ParserException> Schema::ConvertFromXML(const char* Buffer, size_t Size)
		{
#ifdef VI_PUGIXML
			VI_ASSERT(Buffer != nullptr, "buffer should not be null");
			if (!Size)
				return Exceptions::ParserException(ParserError::XMLNoDocumentElement, 0, "empty XML buffer");

			pugi::xml_document Data;
			pugi::xml_parse_result Status = Data.load_buffer(Buffer, Size);
			if (!Status)
			{
				switch (Status.status)
				{
					case pugi::status_out_of_memory:
						return Exceptions::ParserException(ParserError::XMLOutOfMemory, (int)Status.offset, Status.description());
					case pugi::status_internal_error:
						return Exceptions::ParserException(ParserError::XMLInternalError, (int)Status.offset, Status.description());
					case pugi::status_unrecognized_tag:
						return Exceptions::ParserException(ParserError::XMLUnrecognizedTag, (int)Status.offset, Status.description());
					case pugi::status_bad_pi:
						return Exceptions::ParserException(ParserError::XMLBadPi, (int)Status.offset, Status.description());
					case pugi::status_bad_comment:
						return Exceptions::ParserException(ParserError::XMLBadComment, (int)Status.offset, Status.description());
					case pugi::status_bad_cdata:
						return Exceptions::ParserException(ParserError::XMLBadCData, (int)Status.offset, Status.description());
					case pugi::status_bad_doctype:
						return Exceptions::ParserException(ParserError::XMLBadDocType, (int)Status.offset, Status.description());
					case pugi::status_bad_pcdata:
						return Exceptions::ParserException(ParserError::XMLBadPCData, (int)Status.offset, Status.description());
					case pugi::status_bad_start_element:
						return Exceptions::ParserException(ParserError::XMLBadStartElement, (int)Status.offset, Status.description());
					case pugi::status_bad_attribute:
						return Exceptions::ParserException(ParserError::XMLBadAttribute, (int)Status.offset, Status.description());
					case pugi::status_bad_end_element:
						return Exceptions::ParserException(ParserError::XMLBadEndElement, (int)Status.offset, Status.description());
					case pugi::status_end_element_mismatch:
						return Exceptions::ParserException(ParserError::XMLEndElementMismatch, (int)Status.offset, Status.description());
					case pugi::status_append_invalid_root:
						return Exceptions::ParserException(ParserError::XMLAppendInvalidRoot, (int)Status.offset, Status.description());
					case pugi::status_no_document_element:
						return Exceptions::ParserException(ParserError::XMLNoDocumentElement, (int)Status.offset, Status.description());
					default:
						return Exceptions::ParserException(ParserError::XMLInternalError, (int)Status.offset, Status.description());
				}
			}

			pugi::xml_node Main = Data.first_child();
			Schema* Result = Var::Set::Array();
			ProcessConvertionFromXML((void*)&Main, Result);
			return Result;
#else
			return Exceptions::ParserException(ParserError::NotSupported, 0, "no capabilities to parse XML");
#endif
		}
		Expects<Schema*, Exceptions::ParserException> Schema::ConvertFromJSON(const char* Buffer, size_t Size)
		{
#ifdef VI_RAPIDJSON
			VI_ASSERT(Buffer != nullptr, "buffer should not be null");
			if (!Size)
				return Exceptions::ParserException(ParserError::JSONDocumentEmpty, 0);

			rapidjson::Document Base;
			Base.Parse(Buffer, Size);

			Schema* Result = nullptr;
			if (Base.HasParseError())
			{
				int Offset = (int)Base.GetErrorOffset();
				switch (Base.GetParseError())
				{
					case rapidjson::kParseErrorDocumentEmpty:
						return Exceptions::ParserException(ParserError::JSONDocumentEmpty, Offset);
					case rapidjson::kParseErrorDocumentRootNotSingular:
						return Exceptions::ParserException(ParserError::JSONDocumentRootNotSingular, Offset);
					case rapidjson::kParseErrorValueInvalid:
						return Exceptions::ParserException(ParserError::JSONValueInvalid, Offset);
					case rapidjson::kParseErrorObjectMissName:
						return Exceptions::ParserException(ParserError::JSONObjectMissName, Offset);
					case rapidjson::kParseErrorObjectMissColon:
						return Exceptions::ParserException(ParserError::JSONObjectMissColon, Offset);
					case rapidjson::kParseErrorObjectMissCommaOrCurlyBracket:
						return Exceptions::ParserException(ParserError::JSONObjectMissCommaOrCurlyBracket, Offset);
					case rapidjson::kParseErrorArrayMissCommaOrSquareBracket:
						return Exceptions::ParserException(ParserError::JSONArrayMissCommaOrSquareBracket, Offset);
					case rapidjson::kParseErrorStringUnicodeEscapeInvalidHex:
						return Exceptions::ParserException(ParserError::JSONStringUnicodeEscapeInvalidHex, Offset);
					case rapidjson::kParseErrorStringUnicodeSurrogateInvalid:
						return Exceptions::ParserException(ParserError::JSONStringUnicodeSurrogateInvalid, Offset);
					case rapidjson::kParseErrorStringEscapeInvalid:
						return Exceptions::ParserException(ParserError::JSONStringEscapeInvalid, Offset);
					case rapidjson::kParseErrorStringMissQuotationMark:
						return Exceptions::ParserException(ParserError::JSONStringMissQuotationMark, Offset);
					case rapidjson::kParseErrorStringInvalidEncoding:
						return Exceptions::ParserException(ParserError::JSONStringInvalidEncoding, Offset);
					case rapidjson::kParseErrorNumberTooBig:
						return Exceptions::ParserException(ParserError::JSONNumberTooBig, Offset);
					case rapidjson::kParseErrorNumberMissFraction:
						return Exceptions::ParserException(ParserError::JSONNumberMissFraction, Offset);
					case rapidjson::kParseErrorNumberMissExponent:
						return Exceptions::ParserException(ParserError::JSONNumberMissExponent, Offset);
					case rapidjson::kParseErrorTermination:
						return Exceptions::ParserException(ParserError::JSONTermination, Offset);
					case rapidjson::kParseErrorUnspecificSyntaxError:
						return Exceptions::ParserException(ParserError::JSONUnspecificSyntaxError, Offset);
					default:
						return Exceptions::ParserException(ParserError::BadValue);
				}
			}

			rapidjson::Type Type = Base.GetType();
			switch (Type)
			{
				case rapidjson::kNullType:
					Result = new Schema(Var::Null());
					break;
				case rapidjson::kFalseType:
					Result = new Schema(Var::Boolean(false));
					break;
				case rapidjson::kTrueType:
					Result = new Schema(Var::Boolean(true));
					break;
				case rapidjson::kObjectType:
					Result = Var::Set::Object();
					ProcessConvertionFromJSON((void*)&Base, Result);
					break;
				case rapidjson::kArrayType:
					Result = Var::Set::Array();
					ProcessConvertionFromJSON((void*)&Base, Result);
					break;
				case rapidjson::kStringType:
				{
					const char* Buffer = Base.GetString(); size_t Size = Base.GetStringLength();
					if (Size >= 2 && *Buffer == PREFIX_BINARY[0] && Buffer[Size - 1] == PREFIX_BINARY[0])
						Result = new Schema(Var::Binary(Buffer + 1, Size - 2));
					else
						Result = new Schema(Var::String(Buffer, Size));
					break;
				}
				case rapidjson::kNumberType:
					if (Base.IsInt())
						Result = new Schema(Var::Integer(Base.GetInt64()));
					else
						Result = new Schema(Var::Number(Base.GetDouble()));
					break;
				default:
					Result = new Schema(Var::Undefined());
					break;
			}

			return Result;
#else
			return Exceptions::ParserException(ParserError::NotSupported, 0, "no capabilities to parse JSON");
#endif
		}
		Expects<Schema*, Exceptions::ParserException> Schema::ConvertFromJSONB(const SchemaReadCallback& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			uint64_t Version = 0;
			if (!Callback((char*)&Version, sizeof(uint64_t)))
				return Exceptions::ParserException(ParserError::BadVersion);

			Version = OS::CPU::ToEndianness<uint64_t>(OS::CPU::Endian::Little, Version);
			if (Version != JSONB_VERSION)
				return Exceptions::ParserException(ParserError::BadVersion);

			uint32_t Set = 0;
			if (!Callback((char*)&Set, sizeof(uint32_t)))
				return Exceptions::ParserException(ParserError::BadDictionary);

			UnorderedMap<size_t, String> Map;
			Set = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Set);

			for (uint32_t i = 0; i < Set; ++i)
			{
				uint32_t Index = 0;
				if (!Callback((char*)&Index, sizeof(uint32_t)))
					return Exceptions::ParserException(ParserError::BadNameIndex);

				uint16_t Size = 0;
				if (!Callback((char*)&Size, sizeof(uint16_t)))
					return Exceptions::ParserException(ParserError::BadName);

				Index = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Index);
				Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);

				if (Size <= 0)
					continue;

				String Name;
				Name.resize((size_t)Size);
				if (!Callback((char*)Name.c_str(), sizeof(char) * Size))
					return Exceptions::ParserException(ParserError::BadName);

				Map.insert({ Index, Name });
			}

			Schema* Current = Var::Set::Object();
			auto Status = ProcessConvertionFromJSONB(Current, &Map, Callback);
			if (!Status)
			{
				VI_RELEASE(Current);
				return Status.Error();
			}

			return Current;
		}
		Expects<Schema*, Exceptions::ParserException> Schema::FromXML(const String& Text)
		{
			return ConvertFromXML(Text.c_str(), Text.size());
		}
		Expects<Schema*, Exceptions::ParserException> Schema::FromJSON(const String& Text)
		{
			return ConvertFromJSON(Text.c_str(), Text.size());
		}
		Expects<Schema*, Exceptions::ParserException> Schema::FromJSONB(const Vector<char>& Binary)
		{
			size_t Offset = 0;
			return ConvertFromJSONB([&Binary, &Offset](char* Buffer, size_t Length)
			{
				Offset += Length;
				if (Offset >= Binary.size())
					return false;

				memcpy((void*)Buffer, Binary.data() + Offset, Length);
				return true;
			});
		}
		Expects<void, Exceptions::ParserException> Schema::ProcessConvertionFromJSONB(Schema* Current, UnorderedMap<size_t, String>* Map, const SchemaReadCallback& Callback)
		{
			uint32_t Id = 0;
			if (!Callback((char*)&Id, sizeof(uint32_t)))
				return Exceptions::ParserException(ParserError::BadKeyName);

			auto It = Map->find((size_t)OS::CPU::ToEndianness(OS::CPU::Endian::Little, Id));
			if (It != Map->end())
				Current->Key = It->second;

			if (!Callback((char*)&Current->Value.Type, sizeof(VarType)))
				return Exceptions::ParserException(ParserError::BadKeyType);

			switch (Current->Value.Type)
			{
				case VarType::Object:
				case VarType::Array:
				{
					uint32_t Count = 0;
					if (!Callback((char*)&Count, sizeof(uint32_t)))
						return Exceptions::ParserException(ParserError::BadValue);

					Count = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Count);
					if (!Count)
						break;

					Current->Allocate();
					Current->Nodes->resize((size_t)Count);

					for (auto*& Item : *Current->Nodes)
					{
						Item = Var::Set::Object();
						Item->Parent = Current;
						Item->Saved = true;

						auto Status = ProcessConvertionFromJSONB(Item, Map, Callback);
						if (!Status)
							return Status;
					}
					break;
				}
				case VarType::String:
				{
					uint32_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint32_t)))
						return Exceptions::ParserException(ParserError::BadValue);

					String Buffer;
					Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);
					Buffer.resize((size_t)Size);

					if (!Callback((char*)Buffer.c_str(), (size_t)Size * sizeof(char)))
						return Exceptions::ParserException(ParserError::BadString);

					Current->Value = Var::String(Buffer);
					break;
				}
				case VarType::Binary:
				{
					uint32_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint32_t)))
						return Exceptions::ParserException(ParserError::BadValue);

					String Buffer;
					Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);
					Buffer.resize(Size);

					if (!Callback((char*)Buffer.c_str(), (size_t)Size * sizeof(char)))
						return Exceptions::ParserException(ParserError::BadString);

					Current->Value = Var::Binary(Buffer);
					break;
				}
				case VarType::Integer:
				{
					int64_t Integer = 0;
					if (!Callback((char*)&Integer, sizeof(int64_t)))
						return Exceptions::ParserException(ParserError::BadInteger);

					Current->Value = Var::Integer(OS::CPU::ToEndianness(OS::CPU::Endian::Little, Integer));
					break;
				}
				case VarType::Number:
				{
					double Number = 0.0;
					if (!Callback((char*)&Number, sizeof(double)))
						return Exceptions::ParserException(ParserError::BadDouble);

					Current->Value = Var::Number(OS::CPU::ToEndianness(OS::CPU::Endian::Little, Number));
					break;
				}
				case VarType::Decimal:
				{
					uint16_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint16_t)))
						return Exceptions::ParserException(ParserError::BadValue);

					String Buffer;
					Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);
					Buffer.resize((size_t)Size);

					if (!Callback((char*)Buffer.c_str(), (size_t)Size * sizeof(char)))
						return Exceptions::ParserException(ParserError::BadString);

					Current->Value = Var::Decimal(Buffer);
					break;
				}
				case VarType::Boolean:
				{
					bool Boolean = false;
					if (!Callback((char*)&Boolean, sizeof(bool)))
						return Exceptions::ParserException(ParserError::BadBoolean);

					Current->Value = Var::Boolean(Boolean);
					break;
				}
				default:
					break;
			}

			return Core::Optional::OK;
		}
		void Schema::ProcessConvertionFromXML(void* Base, Schema* Current)
		{
#ifdef VI_PUGIXML
			VI_ASSERT(Base != nullptr && Current != nullptr, "base and current should be set");
			pugi::xml_node& Next = *(pugi::xml_node*)Base;
			Current->Key = Next.name();

			for (auto Attribute : Next.attributes())
				Current->SetAttribute(Attribute.name(), Attribute.empty() ? Var::Null() : Var::Auto(Attribute.value()));

			for (auto Child : Next.children())
			{
				Schema* Subresult = Current->Set(Child.name(), Var::Set::Array());
				ProcessConvertionFromXML((void*)&Child, Subresult);

				if (*Child.value() != '\0')
				{
					Subresult->Value.Deserialize(Child.value());
					continue;
				}

				auto Text = Child.text();
				if (!Text.empty())
					Subresult->Value.Deserialize(Child.text().get());
				else
					Subresult->Value = Var::Null();
			}
#endif
		}
		void Schema::ProcessConvertionFromJSON(void* Base, Schema* Current)
		{
#ifdef VI_RAPIDJSON
			VI_ASSERT(Base != nullptr && Current != nullptr, "base and current should be set");
			auto Child = (rapidjson::Value*)Base;
			if (!Child->IsArray())
			{
				String Name;
				Current->Reserve((size_t)Child->MemberCount());

				VarType Type = Current->Value.Type;
				Current->Value.Type = VarType::Array;

				for (auto It = Child->MemberBegin(); It != Child->MemberEnd(); ++It)
				{
					if (!It->name.IsString())
						continue;

					Name.assign(It->name.GetString(), (size_t)It->name.GetStringLength());
					switch (It->value.GetType())
					{
						case rapidjson::kNullType:
							Current->Set(Name, Var::Null());
							break;
						case rapidjson::kFalseType:
							Current->Set(Name, Var::Boolean(false));
							break;
						case rapidjson::kTrueType:
							Current->Set(Name, Var::Boolean(true));
							break;
						case rapidjson::kObjectType:
							ProcessConvertionFromJSON((void*)&It->value, Current->Set(Name));
							break;
						case rapidjson::kArrayType:
							ProcessConvertionFromJSON((void*)&It->value, Current->Set(Name, Var::Array()));
							break;
						case rapidjson::kStringType:
						{
							const char* Buffer = It->value.GetString(); size_t Size = It->value.GetStringLength();
							if (Size >= 2 && *Buffer == PREFIX_BINARY[0] && Buffer[Size - 1] == PREFIX_BINARY[0])
								Current->Set(Name, Var::Binary(Buffer + 1, Size - 2));
							else
								Current->Set(Name, Var::String(Buffer, Size));
							break;
						}
						case rapidjson::kNumberType:
							if (It->value.IsInt())
								Current->Set(Name, Var::Integer(It->value.GetInt64()));
							else
								Current->Set(Name, Var::Number(It->value.GetDouble()));
							break;
						default:
							break;
					}
				}

				Current->Value.Type = Type;
			}
			else
			{
				String Value;
				Current->Reserve((size_t)Child->Size());

				for (auto It = Child->Begin(); It != Child->End(); ++It)
				{
					switch (It->GetType())
					{
						case rapidjson::kNullType:
							Current->Push(Var::Null());
							break;
						case rapidjson::kFalseType:
							Current->Push(Var::Boolean(false));
							break;
						case rapidjson::kTrueType:
							Current->Push(Var::Boolean(true));
							break;
						case rapidjson::kObjectType:
							ProcessConvertionFromJSON((void*)It, Current->Push(Var::Object()));
							break;
						case rapidjson::kArrayType:
							ProcessConvertionFromJSON((void*)It, Current->Push(Var::Array()));
							break;
						case rapidjson::kStringType:
						{
							const char* Buffer = It->GetString(); size_t Size = It->GetStringLength();
							if (Size >= 2 && *Buffer == PREFIX_BINARY[0] && Buffer[Size - 1] == PREFIX_BINARY[0])
								Current->Push(Var::Binary(Buffer + 1, Size - 2));
							else
								Current->Push(Var::String(Buffer, Size));
							break;
						}
						case rapidjson::kNumberType:
							if (It->IsInt())
								Current->Push(Var::Integer(It->GetInt64()));
							else
								Current->Push(Var::Number(It->GetDouble()));
							break;
						default:
							break;
					}
				}
			}
#endif
		}
		void Schema::ProcessConvertionToJSONB(Schema* Current, UnorderedMap<String, size_t>* Map, const SchemaWriteCallback& Callback)
		{
			uint32_t Id = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint32_t)Map->at(Current->Key));
			Callback(VarForm::Dummy, (const char*)&Id, sizeof(uint32_t));
			Callback(VarForm::Dummy, (const char*)&Current->Value.Type, sizeof(VarType));

			switch (Current->Value.Type)
			{
				case VarType::Object:
				case VarType::Array:
				{
					uint32_t Count = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint32_t)(Current->Nodes ? Current->Nodes->size() : 0));
					Callback(VarForm::Dummy, (const char*)&Count, sizeof(uint32_t));
					if (Count > 0)
					{
						for (auto& Schema : *Current->Nodes)
							ProcessConvertionToJSONB(Schema, Map, Callback);
					}
					break;
				}
				case VarType::String:
				case VarType::Binary:
				{
					uint32_t Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint32_t)Current->Value.Size());
					Callback(VarForm::Dummy, (const char*)&Size, sizeof(uint32_t));
					Callback(VarForm::Dummy, Current->Value.GetString(), Size * sizeof(char));
					break;
				}
				case VarType::Decimal:
				{
					String Number = ((Decimal*)Current->Value.Value.Pointer)->ToString();
					uint16_t Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint16_t)Number.size());
					Callback(VarForm::Dummy, (const char*)&Size, sizeof(uint16_t));
					Callback(VarForm::Dummy, Number.c_str(), (size_t)Size * sizeof(char));
					break;
				}
				case VarType::Integer:
				{
					int64_t Value = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Current->Value.Value.Integer);
					Callback(VarForm::Dummy, (const char*)&Value, sizeof(int64_t));
					break;
				}
				case VarType::Number:
				{
					double Value = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Current->Value.Value.Number);
					Callback(VarForm::Dummy, (const char*)&Value, sizeof(double));
					break;
				}
				case VarType::Boolean:
				{
					Callback(VarForm::Dummy, (const char*)&Current->Value.Value.Boolean, sizeof(bool));
					break;
				}
				default:
					break;
			}
		}
		void Schema::GenerateNamingTable(const Schema* Current, UnorderedMap<String, size_t>* Map, size_t& Index)
		{
			auto M = Map->find(Current->Key);
			if (M == Map->end())
				Map->insert({ Current->Key, Index++ });

			if (!Current->Nodes)
				return;

			for (auto Schema : *Current->Nodes)
				GenerateNamingTable(Schema, Map, Index);
		}
	}
}
#pragma warning(pop)