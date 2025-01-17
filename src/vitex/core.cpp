#define _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include "core.h"
#include "scripting.h"
#include "network/http.h"
#include <iomanip>
#include <cctype>
#include <ctime>
#include <thread>
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>
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
#ifdef VI_CXX23
#include <stacktrace>
#elif defined(VI_BACKWARDCPP)
#include <backward.hpp>
#endif
#ifdef VI_FCONTEXT
#include "internal/fcontext.h"
#endif
#ifdef VI_MICROSOFT
#include <Windows.h>
#include <fcntl.h>
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
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <dlfcn.h>
#ifndef VI_FCONTEXT
#include <ucontext.h>
#endif
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
	const char* GetColorId(Vitex::Core::StdColor Color, bool Background)
	{
		switch (Color)
		{
			case Vitex::Core::StdColor::Black:
				return Background ? "40" : "30";
			case Vitex::Core::StdColor::DarkBlue:
				return Background ? "44" : "34";
			case Vitex::Core::StdColor::DarkGreen:
				return Background ? "42" : "32";
			case Vitex::Core::StdColor::DarkRed:
				return Background ? "41" : "31";
			case Vitex::Core::StdColor::Magenta:
				return Background ? "45" : "35";
			case Vitex::Core::StdColor::Orange:
				return Background ? "43" : "93";
			case Vitex::Core::StdColor::LightGray:
				return Background ? "47" : "97";
			case Vitex::Core::StdColor::LightBlue:
				return Background ? "46" : "94";
			case Vitex::Core::StdColor::Gray:
				return Background ? "100" : "90";
			case Vitex::Core::StdColor::Blue:
				return Background ? "104" : "94";
			case Vitex::Core::StdColor::Green:
				return Background ? "102" : "92";
			case Vitex::Core::StdColor::Cyan:
				return Background ? "106" : "36";
			case Vitex::Core::StdColor::Red:
				return Background ? "101" : "91";
			case Vitex::Core::StdColor::Pink:
				return Background ? "105" : "95";
			case Vitex::Core::StdColor::Yellow:
				return Background ? "103" : "33";
			case Vitex::Core::StdColor::White:
				return Background ? "107" : "37";
			case Vitex::Core::StdColor::Zero:
				return Background ? "49" : "39";
			default:
				return Background ? "40" : "107";
		}
	}
}
#endif
namespace
{
	bool GlobalTime(const time_t* timep, struct tm* tm)
	{
		const time_t ts = *timep;
		time_t t = ts / 86400;
		uint32_t hms = ts % 86400;
		if ((int)hms < 0)
		{
			--t;
			hms += 86400;
		}

		time_t c;
		tm->tm_sec = hms % 60;
		hms /= 60;
		tm->tm_min = hms % 60;
		tm->tm_hour = hms / 60;
		if (sizeof(time_t) > sizeof(uint32_t))
		{
			time_t f = (t + 4) % 7;
			if (f < 0)
				f += 7;

			tm->tm_wday = (int)f;
			c = (t << 2) + 102032;
			f = c / 146097;
			if (c % 146097 < 0)
				--f;
			--f;
			t += f;
			f >>= 2;
			t -= f;
			f = (t << 2) + 102035;
			c = f / 1461;
			if (f % 1461 < 0)
				--c;
		}
		else
		{
			tm->tm_wday = (t + 24861) % 7;
			c = ((t << 2) + 102035) / 1461;
		}

		uint32_t yday = (uint32_t)(t - 365 * c - (c >> 2) + 25568);
		uint32_t a = yday * 5 + 8;
		tm->tm_mon = a / 153;
		a %= 153;
		tm->tm_mday = 1 + a / 5;
		if (tm->tm_mon >= 12)
		{
			tm->tm_mon -= 12;
			++c;
			yday -= 366;
		}
		else if (!((c & 3) == 0 && (sizeof(time_t) <= 4 || c % 100 != 0 || (c + 300) % 400 == 0)))
			--yday;

		tm->tm_year = (int)c;
		tm->tm_yday = yday;
		tm->tm_isdst = 0;
		return true;
	}
	bool IsPathExists(const std::string_view& Path)
	{
		struct stat Buffer;
		return stat(Path.data(), &Buffer) == 0;
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
#define SYSCTL(fname, ...) std::size_t Size{};if(fname(__VA_ARGS__,nullptr,&Size,nullptr,0))return{};Vitex::Core::Vector<char> Result(Size);if(fname(__VA_ARGS__,Result.data(),&Size,nullptr,0))return{};return Result
	template <class T>
	static std::pair<bool, T> SysDecompose(const Vitex::Core::Vector<char>& Data)
	{
		std::pair<bool, T> Out { true, {} };
		std::memcpy(&Out.second, Data.data(), sizeof(Out.second));
		return Out;
	}
	Vitex::Core::Vector<char> SysControl(const char* Name)
	{
		SYSCTL(::sysctlbyname, Name);
	}
	Vitex::Core::Vector<char> SysControl(int M1, int M2)
	{
		int Name[2]{ M1, M2 };
		SYSCTL(::sysctl, Name, sizeof(Name) / sizeof(*Name));
	}
	std::pair<bool, uint64_t> SysExtract(const Vitex::Core::Vector<char>& Data)
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
	static Vitex::Core::Vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> CPUInfoBuffer()
	{
		DWORD ByteCount = 0;
		Vitex::Core::Vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> Buffer;
		GetLogicalProcessorInformation(nullptr, &ByteCount);
		Buffer.resize(ByteCount / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
		GetLogicalProcessorInformation(Buffer.data(), &ByteCount);

		return Buffer;
	}
#endif
}

namespace Vitex
{
	namespace Core
	{
		namespace Allocators
		{
			DebugAllocator::TracingInfo::TracingInfo() : Thread(std::this_thread::get_id()), Time(0), Size(0), Active(false)
			{
			}
			DebugAllocator::TracingInfo::TracingInfo(const char* NewTypeName, MemoryLocation&& NewLocation, time_t NewTime, size_t NewSize, bool IsActive, bool IsStatic) : Thread(std::this_thread::get_id()), TypeName(NewTypeName ? NewTypeName : "void"), Location(std::move(NewLocation)), Time(NewTime), Size(NewSize), Active(IsActive), Static(IsStatic)
			{
			}

			void* DebugAllocator::Allocate(size_t Size) noexcept
			{
				return Allocate(MemoryLocation("[unknown]", "[external]", "void", 0), Size);
			}
			void* DebugAllocator::Allocate(MemoryLocation&& Location, size_t Size) noexcept
			{
				void* Address = malloc(Size);
				VI_ASSERT(Address != nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)Size);

				UMutex<std::recursive_mutex> Unique(Mutex);
				Blocks[Address] = TracingInfo(Location.TypeName, std::move(Location), time(nullptr), Size, true, false);
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
			void DebugAllocator::Transfer(void* Address, size_t Size) noexcept
			{
				VI_ASSERT(false, "invalid allocator transfer call without memory context");
			}
			void DebugAllocator::Transfer(void* Address, MemoryLocation&& Location, size_t Size) noexcept
			{
				UMutex<std::recursive_mutex> Unique(Mutex);
				Blocks[Address] = TracingInfo(Location.TypeName, std::move(Location), time(nullptr), Size, true, true);
			}
			void DebugAllocator::Watch(MemoryLocation&& Location, void* Address) noexcept
			{
				UMutex<std::recursive_mutex> Unique(Mutex);
				auto It = Watchers.find(Address);

				VI_ASSERT(It == Watchers.end() || !It->second.Active, "cannot watch memory that is already being tracked at 0x%" PRIXPTR, Address);
				if (It != Watchers.end())
					It->second = TracingInfo(Location.TypeName, std::move(Location), time(nullptr), sizeof(void*), false, false);
				else
					Watchers[Address] = TracingInfo(Location.TypeName, std::move(Location), time(nullptr), sizeof(void*), false, false);
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
						DateTime::SerializeLocal(Date, sizeof(Date), std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(It->second.Time)), DateTime::FormatCompactTime());
						ErrorHandling::Message(LogLevel::Debug, It->second.Location.Line, It->second.Location.Source, "[mem] %saddress at 0x%" PRIXPTR " is used since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
							It->second.Static ? "static " : "",
							It->first, Date, It->second.TypeName.c_str(),
							(uint64_t)It->second.Size,
							It->second.Location.Function,
							OS::Process::GetThreadId(It->second.Thread).c_str());
						Exists = true;
					}

					It = Watchers.find(Address);
					if (It != Watchers.end())
					{
						char Date[64];
						DateTime::SerializeLocal(Date, sizeof(Date), std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(It->second.Time)), DateTime::FormatCompactTime());
						ErrorHandling::Message(LogLevel::Debug, It->second.Location.Line, It->second.Location.Source, "[mem-watch] %saddress at 0x%" PRIXPTR " is being watched since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
							It->second.Static ? "static " : "",
							It->first, Date, It->second.TypeName.c_str(),
							(uint64_t)It->second.Size,
							It->second.Location.Function,
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

					bool LogActive = ErrorHandling::HasFlag(LogOption::Active);
					if (!LogActive)
						ErrorHandling::SetFlag(LogOption::Active, true);

					if (StaticAddresses == Blocks.size() + Watchers.size())
					{
						VI_DEBUG("[mem] memory tracing OK: no memory leaked");
						ErrorHandling::SetFlag(LogOption::Active, LogActive);
						return false;
					}

					size_t TotalMemory = 0;
					for (auto& Item : Blocks)
						TotalMemory += Item.second.Size;
					for (auto& Item : Watchers)
						TotalMemory += Item.second.Size;

					uint64_t Count = (uint64_t)(Blocks.size() + Watchers.size() - StaticAddresses);
					VI_DEBUG("[mem] %" PRIu64 " address%s still used (memory still in use: %" PRIu64 " bytes inc. static allocations)", Count, Count > 1 ? "es are" : " is", (uint64_t)TotalMemory);
					for (auto& Item : Blocks)
					{
						if (Item.second.Static || Item.second.TypeName.find("ontainer_proxy") != std::string::npos || Item.second.TypeName.find("ist_node") != std::string::npos)
							continue;

						char Date[64];
						DateTime::SerializeLocal(Date, sizeof(Date), std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(Item.second.Time)), DateTime::FormatCompactTime());
						ErrorHandling::Message(LogLevel::Debug, Item.second.Location.Line, Item.second.Location.Source, "[mem] address at 0x%" PRIXPTR " is used since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
							Item.first,
							Date,
							Item.second.TypeName.c_str(),
							(uint64_t)Item.second.Size,
							Item.second.Location.Function,
							OS::Process::GetThreadId(Item.second.Thread).c_str());
					}
					for (auto& Item : Watchers)
					{
						if (Item.second.Static || Item.second.TypeName.find("ontainer_proxy") != std::string::npos || Item.second.TypeName.find("ist_node") != std::string::npos)
							continue;

						char Date[64];
						DateTime::SerializeLocal(Date, sizeof(Date), std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(Item.second.Time)), DateTime::FormatCompactTime());
						ErrorHandling::Message(LogLevel::Debug, Item.second.Location.Line, Item.second.Location.Source, "[mem-watch] address at 0x%" PRIXPTR " is being watched since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
							Item.first,
							Date,
							Item.second.TypeName.c_str(),
							(uint64_t)Item.second.Size,
							Item.second.Location.Function,
							OS::Process::GetThreadId(Item.second.Thread).c_str());
					}

					ErrorHandling::SetFlag(LogOption::Active, LogActive);
					return true;
				}
#endif
				return false;
			}
			bool DebugAllocator::FindBlock(void* Address, TracingInfo* Output)
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
			const std::unordered_map<void*, DebugAllocator::TracingInfo>& DebugAllocator::GetBlocks() const
			{
				return Blocks;
			}
			const std::unordered_map<void*, DebugAllocator::TracingInfo>& DebugAllocator::GetWatchers() const
			{
				return Watchers;
			}

			void* DefaultAllocator::Allocate(size_t Size) noexcept
			{
				void* Address = malloc(Size);
				VI_ASSERT(Address != nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)Size);
				return Address;
			}
			void* DefaultAllocator::Allocate(MemoryLocation&& Location, size_t Size) noexcept
			{
				void* Address = malloc(Size);
				VI_ASSERT(Address != nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)Size);
				return Address;
			}
			void DefaultAllocator::Free(void* Address) noexcept
			{
				free(Address);
			}
			void DefaultAllocator::Transfer(void* Address, size_t Size) noexcept
			{
			}
			void DefaultAllocator::Transfer(void* Address, MemoryLocation&& Location, size_t Size) noexcept
			{
			}
			void DefaultAllocator::Watch(MemoryLocation&& Location, void* Address) noexcept
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
			void* CachedAllocator::Allocate(MemoryLocation&&, size_t Size) noexcept
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
			void CachedAllocator::Transfer(void* Address, size_t Size) noexcept
			{
			}
			void CachedAllocator::Transfer(void* Address, MemoryLocation&& Location, size_t Size) noexcept
			{
			}
			void CachedAllocator::Watch(MemoryLocation&& Location, void* Address) noexcept
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

				Region* Next = Memory::Allocate<Region>(sizeof(Region) + Size);
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
					Memory::Deallocate(Address);
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

				Region* Next = Memory::Allocate<Region>(sizeof(Region) + Size);
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
					Memory::Deallocate(Address);
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
				VI_WARN("[stall] sync operation took %" PRIu64 " ms (%" PRIu64 " us), back trace %s", Delta / 1000, Delta, BackTrace.c_str());
			}
#endif
		};

		struct Cocontext
		{
#ifdef VI_FCONTEXT
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
#ifndef VI_FCONTEXT
#ifdef VI_MICROSOFT
				Context = ConvertThreadToFiber(nullptr);
				Main = true;
#endif
#endif
			}
			Cocontext(Costate* State)
			{
#ifdef VI_FCONTEXT
				Stack = Memory::Allocate<char>(sizeof(char) * State->Size);
				Context = make_fcontext(Stack + State->Size, State->Size, [](transfer_t Transfer)
				{
					Costate::ExecutionEntry(&Transfer);
				});
#elif VI_MICROSOFT
				Context = CreateFiber(State->Size, &Costate::ExecutionEntry, (LPVOID)State);
#else
				getcontext(&Context);
				Stack = Memory::Allocate<char>(sizeof(char) * State->Size);
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
#ifdef VI_FCONTEXT
				Memory::Deallocate(Stack);
#elif VI_MICROSOFT
				if (Main)
					ConvertFiberToThread();
				else if (Context != nullptr)
					DeleteFiber(Context);
#else
				Memory::Deallocate(Stack);
#endif
			}
		};

		struct ConcurrentSyncQueue
		{
			FastQueue Queue;
		};

		struct ConcurrentAsyncQueue : ConcurrentSyncQueue
		{
			std::condition_variable Notify;
			std::mutex Update;
			std::atomic<bool> Resync = true;
		};

		BasicException::BasicException(const std::string_view& NewMessage) noexcept : Message(NewMessage)
		{
		}
		BasicException::BasicException(String&& NewMessage) noexcept : Message(std::move(NewMessage))
		{
		}
		const char* BasicException::what() const noexcept
		{
			return Message.c_str();
		}
		const String& BasicException::message() const& noexcept
		{
			return Message;
		}
		const String&& BasicException::message() const&& noexcept
		{
			return std::move(Message);
		}
		String& BasicException::message() & noexcept
		{
			return Message;
		}
		String&& BasicException::message() && noexcept
		{
			return std::move(Message);
		}

		ParserException::ParserException(ParserError NewType) : ParserException(NewType, -1, Core::String())
		{
		}
		ParserException::ParserException(ParserError NewType, size_t NewOffset) : ParserException(NewType, NewOffset, Core::String())
		{
		}
		ParserException::ParserException(ParserError NewType, size_t NewOffset, const std::string_view& NewMessage) : BasicException(), Type(NewType), Offset(NewOffset)
		{
			if (NewMessage.empty())
			{
				switch (Type)
				{
					case ParserError::NotSupported:
						Message = "required libraries are not loaded";
						break;
					case ParserError::BadVersion:
						Message = "corrupted JSONB version header";
						break;
					case ParserError::BadDictionary:
						Message = "corrupted JSONB dictionary body";
						break;
					case ParserError::BadNameIndex:
						Message = "invalid JSONB dictionary name index";
						break;
					case ParserError::BadName:
						Message = "invalid JSONB name";
						break;
					case ParserError::BadKeyName:
						Message = "invalid JSONB key name";
						break;
					case ParserError::BadKeyType:
						Message = "invalid JSONB key type";
						break;
					case ParserError::BadValue:
						Message = "invalid JSONB value for specified key";
						break;
					case ParserError::BadString:
						Message = "invalid JSONB value string";
						break;
					case ParserError::BadInteger:
						Message = "invalid JSONB value integer";
						break;
					case ParserError::BadDouble:
						Message = "invalid JSONB value double";
						break;
					case ParserError::BadBoolean:
						Message = "invalid JSONB value boolean";
						break;
					case ParserError::XMLOutOfMemory:
						Message = "XML out of memory";
						break;
					case ParserError::XMLInternalError:
						Message = "XML internal error";
						break;
					case ParserError::XMLUnrecognizedTag:
						Message = "XML unrecognized tag";
						break;
					case ParserError::XMLBadPi:
						Message = "XML bad pi";
						break;
					case ParserError::XMLBadComment:
						Message = "XML bad comment";
						break;
					case ParserError::XMLBadCData:
						Message = "XML bad cdata";
						break;
					case ParserError::XMLBadDocType:
						Message = "XML bad doctype";
						break;
					case ParserError::XMLBadPCData:
						Message = "XML bad pcdata";
						break;
					case ParserError::XMLBadStartElement:
						Message = "XML bad start element";
						break;
					case ParserError::XMLBadAttribute:
						Message = "XML bad attribute";
						break;
					case ParserError::XMLBadEndElement:
						Message = "XML bad end element";
						break;
					case ParserError::XMLEndElementMismatch:
						Message = "XML end element mismatch";
						break;
					case ParserError::XMLAppendInvalidRoot:
						Message = "XML append invalid root";
						break;
					case ParserError::XMLNoDocumentElement:
						Message = "XML no document element";
						break;
					case ParserError::JSONDocumentEmpty:
						Message = "the JSON document is empty";
						break;
					case ParserError::JSONDocumentRootNotSingular:
						Message = "the JSON document root must not follow by other values";
						break;
					case ParserError::JSONValueInvalid:
						Message = "the JSON document contains an invalid value";
						break;
					case ParserError::JSONObjectMissName:
						Message = "missing a name for a JSON object member";
						break;
					case ParserError::JSONObjectMissColon:
						Message = "missing a colon after a name of a JSON object member";
						break;
					case ParserError::JSONObjectMissCommaOrCurlyBracket:
						Message = "missing a comma or '}' after a JSON object member";
						break;
					case ParserError::JSONArrayMissCommaOrSquareBracket:
						Message = "missing a comma or ']' after a JSON array element";
						break;
					case ParserError::JSONStringUnicodeEscapeInvalidHex:
						Message = "incorrect hex digit after \\u escape in a JSON string";
						break;
					case ParserError::JSONStringUnicodeSurrogateInvalid:
						Message = "the surrogate pair in a JSON string is invalid";
						break;
					case ParserError::JSONStringEscapeInvalid:
						Message = "invalid escape character in a JSON string";
						break;
					case ParserError::JSONStringMissQuotationMark:
						Message = "missing a closing quotation mark in a JSON string";
						break;
					case ParserError::JSONStringInvalidEncoding:
						Message = "invalid encoding in a JSON string";
						break;
					case ParserError::JSONNumberTooBig:
						Message = "JSON number too big to be stored in double";
						break;
					case ParserError::JSONNumberMissFraction:
						Message = "missing fraction part in a JSON number";
						break;
					case ParserError::JSONNumberMissExponent:
						Message = "missing exponent in a JSON number";
						break;
					case ParserError::JSONTermination:
						Message = "unexpected end of file while parsing a JSON document";
						break;
					case ParserError::JSONUnspecificSyntaxError:
						Message = "unspecified JSON syntax error";
						break;
					default:
						Message = "(unrecognized condition)";
						break;
				}
			}
			else
				Message = NewMessage;

			if (Offset > 0)
			{
				Message += " at offset ";
				Message += ToString(Offset);
			}
		}
		const char* ParserException::type() const noexcept
		{
			return "parser_error";
		}
		ParserError ParserException::status() const noexcept
		{
			return Type;
		}
		size_t ParserException::offset() const noexcept
		{
			return Offset;
		}

		SystemException::SystemException() : SystemException(String())
		{
		}
		SystemException::SystemException(const std::string_view& NewMessage) : Error(OS::Error::GetConditionOr(std::errc::operation_not_permitted))
		{
			if (!NewMessage.empty())
			{
				Message += NewMessage;
				Message += " (error = ";
				Message += Error.message().c_str();
				Message += ")";
			}
			else
				Message = Copy<String>(Error.message());
		}
		SystemException::SystemException(const std::string_view& NewMessage, std::error_condition&& Condition) : Error(std::move(Condition))
		{
			if (!NewMessage.empty())
			{
				Message += NewMessage;
				Message += " (error = ";
				Message += Error.message().c_str();
				Message += ")";
			}
			else
				Message = Copy<String>(Error.message());
		}
		const char* SystemException::type() const noexcept
		{
			return "system_error";
		}
		const std::error_condition& SystemException::error() const& noexcept
		{
			return Error;
		}
		const std::error_condition&& SystemException::error() const&& noexcept
		{
			return std::move(Error);
		}
		std::error_condition& SystemException::error() & noexcept
		{
			return Error;
		}
		std::error_condition&& SystemException::error() && noexcept
		{
			return std::move(Error);
		}

		MemoryLocation::MemoryLocation() : Source("?.cpp"), Function("?"), TypeName("void"), Line(0)
		{
		}
		MemoryLocation::MemoryLocation(const char* NewSource, const char* NewFunction, const char* NewTypeName, int NewLine) : Source(NewSource ? NewSource : "?.cpp"), Function(NewFunction ? NewFunction : "?"), TypeName(NewTypeName ? NewTypeName : "void"), Line(NewLine >= 0 ? NewLine : 0)
		{
		}

		static thread_local LocalAllocator* InternalAllocator = nullptr;
		void* Memory::DefaultAllocate(size_t Size) noexcept
		{
			VI_ASSERT(Size > 0, "cannot allocate zero bytes");
			if (InternalAllocator != nullptr)
			{
				void* Address = InternalAllocator->Allocate(Size);
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
		void* Memory::TracingAllocate(size_t Size, MemoryLocation&& Origin) noexcept
		{
			VI_ASSERT(Size > 0, "cannot allocate zero bytes");
			if (InternalAllocator != nullptr)
			{
				void* Address = InternalAllocator->Allocate(Size);
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
		void Memory::DefaultDeallocate(void* Address) noexcept
		{
			if (!Address)
				return;

			if (InternalAllocator != nullptr)
				return InternalAllocator->Free(Address);
			else if (Global != nullptr)
				return Global->Free(Address);
			else if (!Context)
				Context = new State();

			UMutex<std::mutex> Unique(Context->Mutex);
			Context->Allocations.erase(Address);
			free(Address);
		}
		void Memory::Watch(void* Address, MemoryLocation&& Origin) noexcept
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
					Global->Transfer(Item.first, MemoryLocation(Item.second.first), Item.second.second);
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
			InternalAllocator = NewAllocator;
		}
		bool Memory::IsValidAddress(void* Address) noexcept
		{
			VI_ASSERT(Global != nullptr, "allocator should be set");
			VI_ASSERT(Address != nullptr, "address should be set");
			if (InternalAllocator != nullptr && InternalAllocator->IsValid(Address))
				return true;

			return Global->IsValid(Address);
		}
		GlobalAllocator* Memory::GetGlobalAllocator() noexcept
		{
			return Global;
		}
		LocalAllocator* Memory::GetLocalAllocator() noexcept
		{
			return InternalAllocator;
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
					auto SectionName = Next.GetSectionName();
					Frame Target;
					if (!SectionName.empty())
					{
						auto SourceCode = VM->GetSourceCodeAppendixByPath("source", SectionName, (uint32_t)LineNumber, (uint32_t)ColumnNumber, 5);
						if (SourceCode)
							Target.Code = *SourceCode;
						Target.File = SectionName;
					}
					else
						Target.File = "[native]";
					Target.Function = (Next.GetDecl().empty() ? "[optimized]" : Next.GetDecl());
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
#elif defined(VI_BACKWARDCPP)
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
		static thread_local std::stack<Measurement> InternalStacktrace;
#endif
		static thread_local bool InternalLogging = false;
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
			if (InternalLogging || !IsCounting)
				return;

			VI_ASSERT(!InternalStacktrace.empty(), "debug frame should be set");
			auto& Next = InternalStacktrace.top();
			Next.NotifyOfOverConsumption();
			InternalStacktrace.pop();
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
			Data.Origin.File = OS::Path::GetFilename(Source).data();
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
				DateTime::SerializeLocal(Data.Message.Date, sizeof(Data.Message.Date), DateTime::Now(), DateTime::FormatCompactTime());

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
				auto* Terminal = Console::Get();
				Terminal->Show();
				EscapeText(Data.Message.Data, (size_t)Data.Message.Size);
				Dispatch(Data);
				Terminal->FlushWrite();
			}

			ErrorHandling::Pause();
			OS::Process::Abort();
		}
		void ErrorHandling::Assert(int Line, const char* Source, const char* Function, const char* Condition, const char* Format, ...) noexcept
		{
			Details Data;
			Data.Origin.File = OS::Path::GetFilename(Source).data();
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
				DateTime::SerializeLocal(Data.Message.Date, sizeof(Data.Message.Date), DateTime::Now(), DateTime::FormatCompactTime());

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
				auto* Terminal = Console::Get();
				Terminal->Show();
				EscapeText(Data.Message.Data, (size_t)Data.Message.Size);
				Dispatch(Data);
				Terminal->FlushWrite();
			}

			ErrorHandling::Pause();
			OS::Process::Abort();
		}
		void ErrorHandling::Message(LogLevel Level, int Line, const char* Source, const char* Format, ...) noexcept
		{
			VI_ASSERT(Format != nullptr, "format string should be set");
			if (InternalLogging || (!HasFlag(LogOption::Active) && !HasCallback()))
				return;

			Details Data;
			Data.Origin.File = OS::Path::GetFilename(Source).data();
			Data.Origin.Line = Line;
			Data.Type.Level = Level;
			Data.Type.Fatal = false;
			if (HasFlag(LogOption::Dated))
				DateTime::SerializeLocal(Data.Message.Date, sizeof(Data.Message.Date), DateTime::Now(), DateTime::FormatCompactTime());

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
			if (HasFlag(LogOption::Async))
				Codefer([Data = std::move(Data)]() mutable { Dispatch(Data); });
			else
				Dispatch(Data);
		}
		void ErrorHandling::Dispatch(Details& Data) noexcept
		{
			InternalLogging = true;
			if (HasCallback())
				Context->Callback(Data);
#if defined(VI_MICROSOFT) && !defined(NDEBUG)
			OutputDebugStringA(GetMessageText(Data).c_str());
#endif
			if (Console::IsAvailable())
			{
				auto* Terminal = Console::Get();
				if (HasFlag(LogOption::Pretty))
					Terminal->Synced([&Data](Console* Terminal) { Colorify(Terminal, Data); });
				else
					Terminal->Write(GetMessageText(Data));
			}
			InternalLogging = false;
		}
		void ErrorHandling::Colorify(Console* Terminal, Details& Data) noexcept
		{
#if VI_DLEVEL < 5
			bool ParseTokens = Data.Type.Level != LogLevel::Trace && Data.Type.Level != LogLevel::Debug;
#else
			bool ParseTokens = Data.Type.Level != LogLevel::Trace;
#endif
			Terminal->ColorBegin(ParseTokens ? StdColor::Cyan : StdColor::Gray);
			if (HasFlag(LogOption::Dated))
			{
				Terminal->Write(Data.Message.Date);
				Terminal->Write(" ");
#ifndef NDEBUG
				if (Data.Origin.File != nullptr)
				{
					Terminal->ColorBegin(StdColor::Gray);
					Terminal->Write(Data.Origin.File);
					Terminal->Write(":");
					if (Data.Origin.Line > 0)
					{
						char Numeric[NUMSTR_SIZE];
						Terminal->Write(Core::ToStringView(Numeric, sizeof(Numeric), Data.Origin.Line));
						Terminal->Write(" ");
					}
				}
#endif
			}
			Terminal->ColorBegin(GetMessageColor(Data));
			Terminal->Write(GetMessageType(Data));
			Terminal->Write(" ");
			if (ParseTokens)
				Terminal->ColorPrint(StdColor::LightGray, std::string_view(Data.Message.Data, Data.Message.Size));
			else
				Terminal->Write(Data.Message.Data);
			Terminal->Write("\n");
			Terminal->ColorEnd();
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
		void ErrorHandling::SetCallback(std::function<void(Details&)>&& Callback) noexcept
		{
			if (!Context)
				Context = new State();

			Context->Callback = std::move(Callback);
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
			if (InternalLogging)
				return ErrorHandling::Tick(false);

			Measurement Next;
			Next.File = File;
			Next.Function = Function;
			Next.Threshold = ThresholdMS * 1000;
			Next.Time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			Next.Line = Line;

			InternalStacktrace.emplace(std::move(Next));
			return ErrorHandling::Tick(true);
#else
			return ErrorHandling::Tick(false);
#endif
		}
		void ErrorHandling::MeasureLoop() noexcept
		{
#ifndef NDEBUG
			if (!InternalLogging)
			{
				VI_ASSERT(!InternalStacktrace.empty(), "debug frame should be set");
				auto& Next = InternalStacktrace.top();
				Next.NotifyOfOverConsumption();
			}
#endif
		}
		String ErrorHandling::GetMeasureTrace() noexcept
		{
#ifndef NDEBUG
			auto Source = InternalStacktrace;
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
				Stream << " - expects < " << Frame.Threshold / 1000 << " ms\n";
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
		std::string_view ErrorHandling::GetMessageType(const Details& Base) noexcept
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

		Coroutine::Coroutine(Costate* Base, TaskCallback&& Procedure) noexcept : State(Coexecution::Active), Callback(std::move(Procedure)), Slave(Memory::New<Cocontext>(Base)), Master(Base), UserData(nullptr)
		{
		}
		Coroutine::~Coroutine() noexcept
		{
			Memory::Delete(Slave);
		}

		Decimal::Decimal() noexcept : Length(0), Sign('\0')
		{
		}
		Decimal::Decimal(const std::string_view& Text) noexcept : Length(0)
		{
			ApplyBase10(Text);
		}
		Decimal::Decimal(const Decimal& Value) noexcept : Source(Value.Source), Length(Value.Length), Sign(Value.Sign)
		{
		}
		Decimal::Decimal(Decimal&& Value) noexcept : Source(std::move(Value.Source)), Length(Value.Length), Sign(Value.Sign)
		{
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
		void Decimal::ApplyBase10(const std::string_view& Text)
		{
			if (Text.empty())
			{
			InvalidNumber:
				Source.clear();
				Length = 0;
				Sign = '\0';
				return;
			}
			else if (Text.size() == 1)
			{
				uint8_t Value = (uint8_t)Text.front();
				if (!isdigit(Value))
					goto InvalidNumber;

				Source.push_back(Value);
				Sign = '+';
				return;
			}

			Source.reserve(Text.size());
			Sign = Text[0];

			size_t Index = 0;
			if (Sign != '+' && Sign != '-')
			{
				if (!isdigit(Sign))
					goto InvalidNumber;
				Sign = '+';
			}
			else
				Index = 1;

			size_t Position = Text.find('.', Index);
			size_t Base = std::min(Position, Text.size());
			while (Index < Base)
			{
				uint8_t Value = Text[Index++];
				Source.push_back(Value);
				if (!isdigit(Value))
					goto InvalidNumber;
			}

			if (Position == std::string::npos)
			{
				std::reverse(Source.begin(), Source.end());
				return;
			}

			++Index;
			while (Index < Text.size())
			{
				uint8_t Value = Text[Index++];
				Source.push_back(Value);
				if (!isdigit(Value))
					goto InvalidNumber;
				++Length;
			}

			std::reverse(Source.begin(), Source.end());
		}
		void Decimal::ApplyZero()
		{
			Source.push_back('0');
			Sign = '+';
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
		bool Decimal::IsInteger() const
		{
			return !Length;
		}
		bool Decimal::IsFractional() const
		{
			return Length > 0;
		}
		bool Decimal::IsSafeNumber() const
		{
			if (IsNaN())
				return true;

			auto Numeric = ToString();
			if (IsFractional())
			{
				auto Number = FromString<double>(Numeric);
				if (!Number)
					return false;

				char Buffer[NUMSTR_SIZE];
				return ToStringView(Buffer, sizeof(Buffer), *Number) == Numeric;
			}
			else if (IsPositive())
			{
				auto Number = FromString<uint64_t>(Numeric);
				if (!Number)
					return false;

				char Buffer[NUMSTR_SIZE];
				return ToStringView(Buffer, sizeof(Buffer), *Number) == Numeric;
			}
			else
			{
				auto Number = FromString<int64_t>(Numeric);
				if (!Number)
					return false;

				char Buffer[NUMSTR_SIZE];
				return ToStringView(Buffer, sizeof(Buffer), *Number) == Numeric;
			}
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
		String Decimal::ToExponent() const
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
				Result += Core::ToString(IntegerPlaces() - 1);
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
		const String& Decimal::Numeric() const
		{
			return Source;
		}
		uint32_t Decimal::DecimalPlaces() const
		{
			return Length;
		}
		uint32_t Decimal::IntegerPlaces() const
		{
			return (int32_t)Source.size() - Length;
		}
		uint32_t Decimal::Size() const
		{
			return (int32_t)(sizeof(*this) + Source.size() * sizeof(char));
		}
		int8_t Decimal::Position() const
		{
			switch (Sign)
			{
				case '+':
					return 1;
				case '-':
					return -1;
				default:
					return 0;
			}
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
		Decimal Decimal::From(const std::string_view& Data, uint8_t Base)
		{
			static uint8_t Mapping[] =
			{
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0,
				0, 0, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
				0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
				31, 32, 33, 34, 35, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
			};
			Decimal Radix = Decimal(Base);
			Decimal Value = Decimal("0");
			for (auto& Item : Data)
			{
				uint8_t Number = Mapping[(uint8_t)Item];
				Value = Value * Radix + Decimal(Number);
			}

			return Value;
		}
		Decimal Decimal::Zero()
		{
			Decimal Result;
			Result.ApplyZero();
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
		int Decimal::CharToInt(char Value)
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
		bool Variant::Deserialize(const std::string_view& Text, bool Strict)
		{
			Free();
			if (!Strict && !Text.empty())
			{
				if (Text.size() > 2 && Text.front() == PREFIX_ENUM[0] && Text.back() == PREFIX_ENUM[0])
				{
					if (Text == PREFIX_ENUM "null" PREFIX_ENUM)
					{
						Type = VarType::Null;
						return true;
					}
					else if (Text == PREFIX_ENUM "undefined" PREFIX_ENUM)
					{
						Type = VarType::Undefined;
						return true;
					}
					else if (Text == PREFIX_ENUM "{}" PREFIX_ENUM)
					{
						Type = VarType::Object;
						return true;
					}
					else if (Text == PREFIX_ENUM "[]" PREFIX_ENUM)
					{
						Type = VarType::Array;
						return true;
					}
					else if (Text == PREFIX_ENUM "void*" PREFIX_ENUM)
					{
						Type = VarType::Pointer;
						return true;
					}
				}
				else if (Text.front() == 't' && Text == "true")
				{
					Move(Var::Boolean(true));
					return true;
				}
				else if (Text.front() == 'f' && Text == "false")
				{
					Move(Var::Boolean(false));
					return true;
				}
				else if (Stringify::HasNumber(Text))
				{
					if (Stringify::HasInteger(Text))
					{
						auto Number = FromString<int64_t>(Text);
						if (Number)
						{
							Move(Var::Integer(*Number));
							return true;
						}
					}
					else if (Stringify::HasDecimal(Text))
					{
						Move(Var::DecimalString(Text));
						return true;
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
				auto Data = Compute::Codec::Bep45Decode(Text.substr(1, Text.size() - 2));
				Move(Var::Binary((uint8_t*)Data.data(), Data.size()));
			}
			else
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
					return String(GetString());
				case VarType::Binary:
					return PREFIX_BINARY + Compute::Codec::Bep45Encode(GetString()) + PREFIX_BINARY;
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
				return String(GetString());

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
		void* Variant::GetContainer()
		{
			switch (Type)
			{
				case VarType::Pointer:
					return Value.Pointer;
				case VarType::String:
				case VarType::Binary:
					return (void*)GetString().data();
				case VarType::Integer:
					return &Value.Integer;
				case VarType::Number:
					return &Value.Number;
				case VarType::Decimal:
					return Value.Pointer;
				case VarType::Boolean:
					return &Value.Boolean;
				case VarType::Null:
				case VarType::Undefined:
				case VarType::Object:
				case VarType::Array:
				default:
					return nullptr;
			}
		}
		std::string_view Variant::GetString() const
		{
			if (Type != VarType::String && Type != VarType::Binary)
				return std::string_view("", 0);

			return std::string_view(Length <= GetMaxSmallStringSize() ? Value.String : Value.Pointer, Length);
		}
		uint8_t* Variant::GetBinary() const
		{
			if (Type != VarType::String && Type != VarType::Binary)
				return nullptr;

			return Length <= GetMaxSmallStringSize() ? (uint8_t*)Value.String : (uint8_t*)Value.Pointer;
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
				auto Result = FromString<int64_t>(GetString());
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
				auto Result = FromString<double>(GetString());
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
		bool Variant::IsString(const std::string_view& Text) const
		{
			return GetString() == Text;
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

					return GetString() == Other.GetString();
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
						Value.Pointer = Memory::Allocate<char>(StringSize);
					memcpy((void*)GetString().data(), Other.GetString().data(), StringSize);
					break;
				}
				case VarType::Decimal:
				{
					Decimal* From = (Decimal*)Other.Value.Pointer;
					Value.Pointer = (char*)Memory::New<Decimal>(*From);
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
						memcpy((void*)GetString().data(), Other.GetString().data(), sizeof(char) * (Length + 1));
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

					Memory::Deallocate(Value.Pointer);
					Value.Pointer = nullptr;
					break;
				}
				case VarType::Decimal:
				{
					if (!Value.Pointer)
						break;

					Decimal* Buffer = (Decimal*)Value.Pointer;
					Memory::Delete(Buffer);
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

		DateTime::DateTime() noexcept : DateTime(std::chrono::system_clock::now().time_since_epoch())
		{
		}
		DateTime::DateTime(const struct tm& Duration) noexcept : Offset({ }), Timepoint(Duration), Synchronized(false), Globalized(false)
		{
			ApplyTimepoint();
		}
		DateTime::DateTime(std::chrono::system_clock::duration&& Duration) noexcept : Offset(std::move(Duration)), Timepoint({ }), Synchronized(false), Globalized(false)
		{
			ApplyOffset();
		}
		DateTime& DateTime::operator +=(const DateTime& Right)
		{
			Offset += Right.Offset;
			return ApplyOffset(true);
		}
		DateTime& DateTime::operator -=(const DateTime& Right)
		{
			Offset -= Right.Offset;
			return ApplyOffset(true);
		}
		bool DateTime::operator >=(const DateTime& Right)
		{
			return Offset >= Right.Offset;
		}
		bool DateTime::operator <=(const DateTime& Right)
		{
			return Offset <= Right.Offset;
		}
		bool DateTime::operator >(const DateTime& Right)
		{
			return Offset > Right.Offset;
		}
		bool DateTime::operator <(const DateTime& Right)
		{
			return Offset < Right.Offset;
		}
		bool DateTime::operator ==(const DateTime& Right)
		{
			return Offset == Right.Offset;
		}
		DateTime DateTime::operator +(const DateTime& Right) const
		{
			DateTime New = *this;
			New.Offset = Offset + Right.Offset;
			return New.ApplyOffset(true);
		}
		DateTime DateTime::operator -(const DateTime& Right) const
		{
			DateTime New = *this;
			New.Offset = Offset - Right.Offset;
			return New.ApplyOffset(true);
		}
		DateTime& DateTime::ApplyOffset(bool Always)
		{
			if (!Synchronized || Always)
			{
				time_t Time = std::chrono::duration_cast<std::chrono::seconds>(Offset).count();
				if (Globalized)
					GlobalTime(&Time, &Timepoint);
				else
					LocalTime(&Time, &Timepoint);
				Synchronized = true;
			}
			return *this;
		}
		DateTime& DateTime::ApplyTimepoint(bool Always)
		{
			if (!Synchronized || Always)
			{
				Offset = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(mktime(&Timepoint)));
				Synchronized = true;
			}
			return *this;
		}
		DateTime& DateTime::UseGlobalTime()
		{
			Globalized = true;
			return *this;
		}
		DateTime& DateTime::UseLocalTime()
		{
			Globalized = false;
			return *this;
		}
		DateTime& DateTime::SetSecond(uint8_t Value)
		{
			if (Value > 60)
				Value = 60;

			Timepoint.tm_sec = (int)Value;
			return ApplyTimepoint(true);
		}
		DateTime& DateTime::SetMinute(uint8_t Value)
		{
			if (Value > 60)
				Value = 60;
			else if (Value < 1)
				Value = 1;

			Timepoint.tm_min = (int)Value - 1;
			return ApplyTimepoint(true);
		}
		DateTime& DateTime::SetHour(uint8_t Value)
		{
			if (Value > 24)
				Value = 24;
			else if (Value < 1)
				Value = 1;

			Timepoint.tm_hour = (int)Value - 1;
			return ApplyTimepoint(true);
		}
		DateTime& DateTime::SetDay(uint8_t Value)
		{
			uint8_t Month = this->Month(), Days = 31;
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

			if (Timepoint.tm_mday > (int)Value)
				Timepoint.tm_yday = Timepoint.tm_yday - Timepoint.tm_mday + (int)Value;
			else
				Timepoint.tm_yday = Timepoint.tm_yday - (int)Value + Timepoint.tm_mday;

			if (Value <= 7)
				Timepoint.tm_wday = (int)Value - 1;
			else if (Value <= 14)
				Timepoint.tm_wday = (int)Value - 8;
			else if (Value <= 21)
				Timepoint.tm_wday = (int)Value - 15;
			else
				Timepoint.tm_wday = (int)Value - 22;

			Timepoint.tm_mday = (int)Value;
			return ApplyTimepoint(true);
		}
		DateTime& DateTime::SetWeek(uint8_t Value)
		{
			if (Value > 7)
				Value = 7;
			else if (Value < 1)
				Value = 1;

			Timepoint.tm_wday = (int)Value - 1;
			return ApplyTimepoint(true);
		}
		DateTime& DateTime::SetMonth(uint8_t Value)
		{
			if (Value < 1)
				Value = 1;
			else if (Value > 12)
				Value = 12;

			Timepoint.tm_mon = (int)Value - 1;
			return ApplyTimepoint(true);
		}
		DateTime& DateTime::SetYear(uint32_t Value)
		{
			if (Value < 1900)
				Value = 1900;

			Timepoint.tm_year = (int)Value - 1900;
			return ApplyTimepoint(true);
		}
		String DateTime::Serialize(const std::string_view& Format) const
		{
			VI_ASSERT(Stringify::IsCString(Format) && !Format.empty(), "format should be set");
			char Buffer[CHUNK_SIZE];
			strftime(Buffer, sizeof(Buffer), Format.data(), &Timepoint);
			return Buffer;
		}
		uint8_t DateTime::Second() const
		{
			return Timepoint.tm_sec;
		}
		uint8_t DateTime::Minute() const
		{
			return Timepoint.tm_min;
		}
		uint8_t DateTime::Hour() const
		{
			return Timepoint.tm_hour;
		}
		uint8_t DateTime::Day() const
		{
			return Timepoint.tm_mday;
		}
		uint8_t DateTime::Week() const
		{
			return Timepoint.tm_wday + 1;
		}
		uint8_t DateTime::Month() const
		{
			return Timepoint.tm_mon + 1;
		}
		uint32_t DateTime::Year() const
		{
			return Timepoint.tm_year + 1900;
		}
		int64_t DateTime::Nanoseconds() const
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(Offset).count();
		}
		int64_t DateTime::Microseconds() const
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(Offset).count();
		}
		int64_t DateTime::Milliseconds() const
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(Offset).count();
		}
		int64_t DateTime::Seconds() const
		{
			return std::chrono::duration_cast<std::chrono::seconds>(Offset).count();
		}
		const struct tm& DateTime::CurrentTimepoint() const
		{
			return Timepoint;
		}
		const std::chrono::system_clock::duration& DateTime::CurrentOffset() const
		{
			return Offset;
		}
		std::chrono::system_clock::duration DateTime::Now()
		{
			return std::chrono::system_clock::now().time_since_epoch();
		}
		DateTime DateTime::FromNanoseconds(int64_t Value)
		{
			return DateTime(std::chrono::nanoseconds(Value));
		}
		DateTime DateTime::FromMicroseconds(int64_t Value)
		{
			return DateTime(std::chrono::microseconds(Value));
		}
		DateTime DateTime::FromMilliseconds(int64_t Value)
		{
			return DateTime(std::chrono::milliseconds(Value));
		}
		DateTime DateTime::FromSeconds(int64_t Value)
		{
			return DateTime(std::chrono::seconds(Value));
		}
		DateTime DateTime::FromSerialized(const std::string_view& Text, const std::string_view& Format)
		{
			VI_ASSERT(Stringify::IsCString(Format) && !Format.empty(), "format should be set");
			std::istringstream Stream = std::istringstream(std::string(Text));
			Stream.imbue(std::locale(setlocale(LC_ALL, nullptr)));

			tm Date { };
			Stream >> std::get_time(&Date, Format.data());
			return Stream.fail() ? DateTime(std::chrono::seconds(0)) : DateTime(Date);
		}
		String DateTime::SerializeGlobal(const std::chrono::system_clock::duration& Time, const std::string_view& Format)
		{
			char Buffer[CHUNK_SIZE];
			return String(SerializeGlobal(Buffer, sizeof(Buffer), Time, Format));
		}
		String DateTime::SerializeLocal(const std::chrono::system_clock::duration& Time, const std::string_view& Format)
		{
			char Buffer[CHUNK_SIZE];
			return String(SerializeLocal(Buffer, sizeof(Buffer), Time, Format));
		}
		std::string_view DateTime::SerializeGlobal(char* Buffer, size_t Length, const std::chrono::system_clock::duration& Time, const std::string_view& Format)
		{
			VI_ASSERT(Buffer != nullptr && Length > 0, "buffer should be set");
			VI_ASSERT(Stringify::IsCString(Format) && !Format.empty(), "format should be set");
			time_t Offset = (time_t)std::chrono::duration_cast<std::chrono::seconds>(Time).count();
			struct tm Date { };
			if (GlobalTime(&Offset, &Date))
				return std::string_view(Buffer, strftime(Buffer, Length, Format.data(), &Date));

			memset(Buffer, 0, Length);
			return std::string_view(Buffer, 0);
		}
		std::string_view DateTime::SerializeLocal(char* Buffer, size_t Length, const std::chrono::system_clock::duration& Time, const std::string_view& Format)
		{
			VI_ASSERT(Buffer != nullptr && Length > 0, "buffer should be set");
			VI_ASSERT(Stringify::IsCString(Format) && !Format.empty(), "format should be set");
			time_t Offset = (time_t)std::chrono::duration_cast<std::chrono::seconds>(Time).count();
			struct tm Date { };
			if (LocalTime(&Offset, &Date))
				return std::string_view(Buffer, strftime(Buffer, Length, Format.data(), &Date));

			memset(Buffer, 0, Length);
			return std::string_view(Buffer, 0);
		}
		std::string_view DateTime::FormatIso8601Time()
		{
			return "%FT%TZ";
		}
		std::string_view DateTime::FormatWebTime()
		{
			return "%a, %d %b %Y %H:%M:%S GMT";
		}
		std::string_view DateTime::FormatWebLocalTime()
		{
			return "%a, %d %b %Y %H:%M:%S %Z";
		}
		std::string_view DateTime::FormatCompactTime()
		{
			return "%Y-%m-%d %H:%M:%S";
		}
		int64_t DateTime::SecondsFromSerialized(const std::string_view& Text, const std::string_view& Format)
		{
			VI_ASSERT(Stringify::IsCString(Format) && !Format.empty(), "format should be set");
			std::istringstream Stream = std::istringstream(std::string(Text));
			Stream.imbue(std::locale(setlocale(LC_ALL, nullptr)));

			tm Date { };
			Stream >> std::get_time(&Date, Format.data());
			return Stream.fail() ? 0 : mktime(&Date);
		}
		bool DateTime::MakeGlobalTime(time_t Time, struct tm* Timepoint)
		{
			VI_ASSERT(Timepoint != nullptr, "time should be set");
			return GlobalTime(&Time, Timepoint);
		}
		bool DateTime::MakeLocalTime(time_t Time, struct tm* Timepoint)
		{
			VI_ASSERT(Timepoint != nullptr, "time should be set");
			return LocalTime(&Time, Timepoint);
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
		String& Stringify::Compress(String& Other, const std::string_view& Tokenbase, const std::string_view& NotInBetweenOf, size_t Start)
		{
			size_t TokenbaseSize = Tokenbase.size();
			size_t NiboSize = NotInBetweenOf.size();
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
		String& Stringify::ReplaceOf(String& Other, const std::string_view& Chars, const std::string_view& To, size_t Start)
		{
			VI_ASSERT(!Chars.empty(), "match list and replacer should not be empty");
			TextSettle Result{ };
			size_t Offset = Start, ToSize = To.size();
			while ((Result = FindOf(Other, Chars, Offset)).Found)
			{
				EraseOffsets(Other, Result.Start, Result.End);
				Other.insert(Result.Start, To);
				Offset = Result.Start + ToSize;
			}
			return Other;
		}
		String& Stringify::ReplaceNotOf(String& Other, const std::string_view& Chars, const std::string_view& To, size_t Start)
		{
			VI_ASSERT(!Chars.empty(), "match list and replacer should not be empty");
			TextSettle Result{};
			size_t Offset = Start, ToSize = To.size();
			while ((Result = FindNotOf(Other, Chars, Offset)).Found)
			{
				EraseOffsets(Other, Result.Start, Result.End);
				Other.insert(Result.Start, To);
				Offset = Result.Start + ToSize;
			}
			return Other;
		}
		String& Stringify::Replace(String& Other, const std::string_view& From, const std::string_view& To, size_t Start)
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
		String& Stringify::ReplaceGroups(String& Other, const std::string_view& FromRegex, const std::string_view& To)
		{
			Compute::RegexSource Source('(' + String(FromRegex) + ')');
			Compute::Regex::Replace(&Source, To, Other);
			return Other;
		}
		String& Stringify::Replace(String& Other, char From, char To, size_t Position)
		{
			for (size_t i = Position; i < Other.size(); i++)
			{
				char& C = Other.at(i);
				if (C == From)
					C = To;
			}
			return Other;
		}
		String& Stringify::Replace(String& Other, char From, char To, size_t Position, size_t Count)
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
		String& Stringify::ReplacePart(String& Other, size_t Start, size_t End, const std::string_view& Value)
		{
			VI_ASSERT(Start < Other.size(), "invalid start");
			VI_ASSERT(End <= Other.size(), "invalid end");
			VI_ASSERT(Start < End, "start should be less than end");
			if (Start == 0)
			{
				if (Other.size() != End)
					Other.assign(String(Value) + Other.substr(End, Other.size() - End));
				else
					Other.assign(Value);
			}
			else if (Other.size() == End)
				Other.assign(Other.substr(0, Start) + String(Value));
			else
				Other.assign(Other.substr(0, Start) + String(Value) + Other.substr(End, Other.size() - End));
			return Other;
		}
		String& Stringify::ReplaceStartsWithEndsOf(String& Other, const std::string_view& Begins, const std::string_view& EndsOf, const std::string_view& With, size_t Start)
		{
			VI_ASSERT(!Begins.empty(), "begin should not be empty");
			VI_ASSERT(!EndsOf.empty(), "end should not be empty");

			size_t BeginsSize = Begins.size(), EndsOfSize = EndsOf.size();
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
		String& Stringify::ReplaceInBetween(String& Other, const std::string_view& Begins, const std::string_view& Ends, const std::string_view& With, bool Recursive, size_t Start)
		{
			VI_ASSERT(!Begins.empty(), "begin should not be empty");
			VI_ASSERT(!Ends.empty(), "end should not be empty");

			size_t BeginsSize = Begins.size(), EndsSize = Ends.size();
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
		String& Stringify::ReplaceNotInBetween(String& Other, const std::string_view& Begins, const std::string_view& Ends, const std::string_view& With, bool Recursive, size_t Start)
		{
			VI_ASSERT(!Begins.empty(), "begin should not be empty");
			VI_ASSERT(!Ends.empty(), "end should not be empty");

			size_t BeginsSize = Begins.size(), EndsSize = Ends.size();
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
		String& Stringify::ReplaceParts(String& Other, Vector<std::pair<String, TextSettle>>& Inout, const std::string_view& With, const std::function<char(const std::string_view&, char, int)>& Surrounding)
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
					String Replacement = String(With);
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
		String& Stringify::ReplaceParts(String& Other, Vector<TextSettle>& Inout, const std::string_view& With, const std::function<char(char, int)>& Surrounding)
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
					String Replacement = String(With);
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
			if (Other.size() > 1)
				Reverse(Other, 0, Other.size());
			return Other;
		}
		String& Stringify::Reverse(String& Other, size_t Start, size_t End)
		{
			VI_ASSERT(!Other.empty(), "length should be at least 1 char");
			VI_ASSERT(End <= Other.size(), "end should be less than length");
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
			Other.erase(Other.begin(), std::find_if(Other.begin(), Other.end(), [](uint8_t V) -> bool { return std::isspace(V) == 0; }));
			return Other;
		}
		String& Stringify::TrimEnd(String& Other)
		{
			Other.erase(std::find_if(Other.rbegin(), Other.rend(), [](uint8_t V) -> bool { return std::isspace(V) == 0; }).base(), Other.end());
			return Other;
		}
		String& Stringify::Fill(String& Other, char Char)
		{
			for (char& i : Other)
				i = Char;
			return Other;
		}
		String& Stringify::Fill(String& Other, char Char, size_t Count)
		{
			Other.assign(Count, Char);
			return Other;
		}
		String& Stringify::Fill(String& Other, char Char, size_t Start, size_t Count)
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
		ExpectsSystem<void> Stringify::EvalEnvs(String& Other, const std::string_view& Directory, const Vector<String>& Tokens, const std::string_view& Token)
		{
			if (Other.empty())
				return Core::Expectation::Met;

			if (StartsOf(Other, "./\\"))
			{
				ExpectsIO<String> Result = OS::Path::Resolve(Other.c_str(), Directory, true);
				if (Result)
					Other.assign(*Result);
			}
			else if (Other.front() == '$' && Other.size() > 1)
			{
				auto Env = OS::Process::GetEnv(Other.c_str() + 1);
				if (!Env)
					return SystemException("invalid env name: " + Other.substr(1), std::move(Env.Error()));
				Other.assign(*Env);
			}
			else if (!Tokens.empty())
			{
				size_t Start = std::string::npos, Offset = 0;
				while ((Start = Other.find(Token, Offset)) != std::string::npos)
				{
					size_t Subset = Start + Token.size(); size_t End = Subset;
					while (End < Other.size() && IsNumeric(Other[End]))
						++End;

					auto Index = FromString<uint8_t>(std::string_view(Other.data() + Subset, End - Subset));
					if (Index && *Index < Tokens.size())
					{
						auto& Target = Tokens[*Index];
						Other.replace(Other.begin() + Start, Other.begin() + End, Target);
						Offset = Start + Target.size();
					}
					else
						Offset = End;
				}
			}

			return Core::Expectation::Met;
		}
		Vector<std::pair<String, TextSettle>> Stringify::FindInBetween(const std::string_view& Other, const std::string_view& Begins, const std::string_view& Ends, const std::string_view& NotInSubBetweenOf, size_t Offset)
		{
			Vector<std::pair<String, TextSettle>> Result;
			PmFindInBetween(Result, Other, Begins, Ends, NotInSubBetweenOf, Offset);
			return Result;
		}
		Vector<std::pair<String, TextSettle>> Stringify::FindInBetweenInCode(const std::string_view& Other, const std::string_view& Begins, const std::string_view& Ends, size_t Offset)
		{
			Vector<std::pair<String, TextSettle>> Result;
			PmFindInBetweenInCode(Result, Other, Begins, Ends, Offset);
			return Result;
		}
		Vector<std::pair<String, TextSettle>> Stringify::FindStartsWithEndsOf(const std::string_view& Other, const std::string_view& Begins, const std::string_view& EndsOf, const std::string_view& NotInSubBetweenOf, size_t Offset)
		{
			Vector<std::pair<String, TextSettle>> Result;
			PmFindStartsWithEndsOf(Result, Other, Begins, EndsOf, NotInSubBetweenOf, Offset);
			return Result;
		}
		void Stringify::PmFindInBetween(Vector<std::pair<String, TextSettle>>& Result, const std::string_view& Other, const std::string_view& Begins, const std::string_view& Ends, const std::string_view& NotInSubBetweenOf, size_t Offset)
		{
			VI_ASSERT(!Begins.empty(), "begin should not be empty");
			VI_ASSERT(!Ends.empty(), "end should not be empty");

			size_t BeginsSize = Begins.size(), EndsSize = Ends.size();
			size_t NisboSize = NotInSubBetweenOf.size();
			char Skip = '\0';

			for (size_t i = Offset; i < Other.size(); i++)
			{
				for (size_t j = 0; j < NisboSize; j++)
				{
					char Next = Other.at(i);
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

				Result.push_back(std::make_pair(String(Other.substr(From, (To - EndsSize) - From)), std::move(At)));
			}
		}
		void Stringify::PmFindInBetweenInCode(Vector<std::pair<String, TextSettle>>& Result, const std::string_view& Other, const std::string_view& Begins, const std::string_view& Ends, size_t Offset)
		{
			VI_ASSERT(!Begins.empty(), "begin should not be empty");
			VI_ASSERT(!Ends.empty(), "end should not be empty");

			size_t BeginsSize = Begins.size(), EndsSize = Ends.size();
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

				Result.push_back(std::make_pair(String(Other.substr(From, (To - EndsSize) - From)), std::move(At)));
			}
		}
		void Stringify::PmFindStartsWithEndsOf(Vector<std::pair<String, TextSettle>>& Result, const std::string_view& Other, const std::string_view& Begins, const std::string_view& EndsOf, const std::string_view& NotInSubBetweenOf, size_t Offset)
		{
			VI_ASSERT(!Begins.empty(), "begin should not be empty");
			VI_ASSERT(!EndsOf.empty(), "end should not be empty");

			size_t BeginsSize = Begins.size(), EndsOfSize = EndsOf.size();
			size_t NisboSize = NotInSubBetweenOf.size();
			char Skip = '\0';

			for (size_t i = Offset; i < Other.size(); i++)
			{
				for (size_t j = 0; j < NisboSize; j++)
				{
					char Next = Other.at(i);
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

				Result.push_back(std::make_pair(String(Other.substr(From, To - From)), std::move(At)));
			}
		}
		TextSettle Stringify::ReverseFind(const std::string_view& Other, const std::string_view& Needle, size_t Offset)
		{
			if (Other.empty() || Offset >= Other.size())
				return { Other.size(), Other.size(), false };

			const char* Ptr = Other.data() - Offset;
			if (Needle.data() > Ptr)
				return { Other.size(), Other.size(), false };

			const char* It = nullptr;
			for (It = Ptr + Other.size() - Needle.size(); It > Ptr; --It)
			{
				if (strncmp(Ptr, Needle.data(), Needle.size()) == 0)
				{
					size_t Set = (size_t)(It - Ptr);
					return { Set, Set + Needle.size(), true };
				}
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::ReverseFind(const std::string_view& Other, char Needle, size_t Offset)
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
		TextSettle Stringify::ReverseFindUnescaped(const std::string_view& Other, char Needle, size_t Offset)
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
		TextSettle Stringify::ReverseFindOf(const std::string_view& Other, const std::string_view& Needle, size_t Offset)
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
		TextSettle Stringify::Find(const std::string_view& Other, const std::string_view& Needle, size_t Offset)
		{
			if (Other.empty() || Offset >= Other.size())
				return { Other.size(), Other.size(), false };

			const char* It = strstr(Other.data() + Offset, Needle.data());
			if (It == nullptr)
				return { Other.size(), Other.size(), false };

			size_t Set = (size_t)(It - Other.data());
			return { Set, Set + Needle.size(), true };
		}
		TextSettle Stringify::Find(const std::string_view& Other, char Needle, size_t Offset)
		{
			for (size_t i = Offset; i < Other.size(); i++)
			{
				if (Other.at(i) == Needle)
					return { i, i + 1, true };
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::FindUnescaped(const std::string_view& Other, char Needle, size_t Offset)
		{
			for (size_t i = Offset; i < Other.size(); i++)
			{
				if (Other.at(i) == Needle && ((int64_t)i - 1 < 0 || Other.at(i - 1) != '\\'))
					return { i, i + 1, true };
			}

			return { Other.size(), Other.size(), false };
		}
		TextSettle Stringify::FindOf(const std::string_view& Other, const std::string_view& Needle, size_t Offset)
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
		TextSettle Stringify::FindNotOf(const std::string_view& Other, const std::string_view& Needle, size_t Offset)
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
		size_t Stringify::CountLines(const std::string_view& Other)
		{
			size_t Lines = 1;
			for (char Item : Other)
			{
				if (Item == '\n')
					++Lines;
			}
			return Lines;
		}
		bool Stringify::IsCString(const std::string_view& Other)
		{
			auto* Data = Other.data();
			return Data != nullptr && Data[Other.size()] == '\0';
		}
		bool Stringify::IsNotPrecededByEscape(const std::string_view& Buffer, size_t Offset, char Escape)
		{
			VI_ASSERT(!Buffer.empty(), "buffer should be set");
			if (Offset < 1 || Buffer[Offset - 1] != Escape)
				return true;

			return Offset > 1 && Buffer[Offset - 2] == Escape;
		}
		bool Stringify::IsEmptyOrWhitespace(const std::string_view& Other)
		{
			if (Other.empty())
				return true;

			for (char Next : Other)
			{
				if (!IsWhitespace(Next))
					return false;
			}

			return true;
		}
		bool Stringify::IsPrecededBy(const std::string_view& Other, size_t At, const std::string_view& Of)
		{
			VI_ASSERT(!Of.empty(), "tokenbase should be set");
			if (!At || At - 1 >= Other.size())
				return false;

			size_t Size = Of.size();
			char Next = Other.at(At - 1);
			for (size_t i = 0; i < Size; i++)
			{
				if (Next == Of[i])
					return true;
			}

			return false;
		}
		bool Stringify::IsFollowedBy(const std::string_view& Other, size_t At, const std::string_view& Of)
		{
			VI_ASSERT(!Of.empty(), "tokenbase should be set");
			if (At + 1 >= Other.size())
				return false;

			size_t Size = Of.size();
			char Next = Other.at(At + 1);
			for (size_t i = 0; i < Size; i++)
			{
				if (Next == Of[i])
					return true;
			}

			return false;
		}
		bool Stringify::StartsWith(const std::string_view& Other, const std::string_view& Value, size_t Offset)
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
		bool Stringify::StartsOf(const std::string_view& Other, const std::string_view& Value, size_t Offset)
		{
			VI_ASSERT(!Value.empty(), "value should be set");
			size_t Length = Value.size();
			if (Offset >= Other.size())
				return false;

			for (size_t j = 0; j < Length; j++)
			{
				if (Other.at(Offset) == Value[j])
					return true;
			}

			return false;
		}
		bool Stringify::StartsNotOf(const std::string_view& Other, const std::string_view& Value, size_t Offset)
		{
			VI_ASSERT(!Value.empty(), "value should be set");
			size_t Length = Value.size();
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
		bool Stringify::EndsWith(const std::string_view& Other, const std::string_view& Value)
		{
			if (Other.empty() || Value.size() > Other.size())
				return false;

			return strcmp(Other.data() + Other.size() - Value.size(), Value.data()) == 0;
		}
		bool Stringify::EndsWith(const std::string_view& Other, char Value)
		{
			return !Other.empty() && Other.back() == Value;
		}
		bool Stringify::EndsOf(const std::string_view& Other, const std::string_view& Value)
		{
			VI_ASSERT(!Value.empty(), "value should be set");
			if (Other.empty())
				return false;

			size_t Length = Value.size();
			for (size_t j = 0; j < Length; j++)
			{
				if (Other.back() == Value[j])
					return true;
			}

			return false;
		}
		bool Stringify::EndsNotOf(const std::string_view& Other, const std::string_view& Value)
		{
			VI_ASSERT(!Value.empty(), "value should be set");
			if (Other.empty())
				return true;

			size_t Length = Value.size();
			for (size_t j = 0; j < Length; j++)
			{
				if (Other.back() == Value[j])
					return false;
			}

			return true;
		}
		bool Stringify::HasInteger(const std::string_view& Other)
		{
			if (Other.empty() || (Other.size() == 1 && !IsNumeric(Other.front())))
				return false;
			
			size_t Digits = 0;
			size_t i = (Other.front() == '+' || Other.front() == '-' ? 1 : 0); 
			for (; i < Other.size(); i++)
			{
				char V = Other[i];
				if (!IsNumeric(V))
					return false;

				++Digits;
			}

			return Digits > 0;
		}
		bool Stringify::HasNumber(const std::string_view& Other)
		{
			if (Other.empty() || (Other.size() == 1 && !IsNumeric(Other.front())))
				return false;

			size_t Digits = 0, Points = 0;
			size_t i = (Other.front() == '+' || Other.front() == '-' ? 1 : 0);
			for (; i < Other.size(); i++)
			{
				char V = Other[i];
				if (IsNumeric(V))
				{
					++Digits;
					continue;
				}

				if (!Points && V == '.' && i + 1 < Other.size())
				{
					++Points;
					continue;
				}

				return false;
			}

			return Digits > 0 && Points < 2;
		}
		bool Stringify::HasDecimal(const std::string_view& Other)
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
		bool Stringify::IsNumericOrDot(char Char)
		{
			return Char == '.' || IsNumeric(Char);
		}
		bool Stringify::IsNumericOrDotOrWhitespace(char Char)
		{
			return IsWhitespace(Char) || IsNumericOrDot(Char);
		}
		bool Stringify::IsHex(char Char)
		{
			return IsNumeric(Char) || (Char >= 'a' && Char <= 'f') || (Char >= 'A' && Char <= 'F');
		}
		bool Stringify::IsHexOrDot(char Char)
		{
			return Char == '.' || IsHex(Char);
		}
		bool Stringify::IsHexOrDotOrWhitespace(char Char)
		{
			return IsWhitespace(Char) || IsHexOrDot(Char);
		}
		bool Stringify::IsAlphabetic(char Char)
		{
			return std::isalpha(static_cast<uint8_t>(Char)) != 0;
		}
		bool Stringify::IsNumeric(char Char)
		{
			return std::isdigit(static_cast<uint8_t>(Char)) != 0;
		}
		bool Stringify::IsAlphanum(char Char)
		{
			return std::isalnum(static_cast<uint8_t>(Char)) != 0;
		}
		bool Stringify::IsWhitespace(char Char)
		{
			return std::isspace(static_cast<uint8_t>(Char)) != 0;
		}
		char Stringify::ToLowerLiteral(char Char)
		{
			return static_cast<char>(std::tolower(static_cast<uint8_t>(Char)));
		}
		char Stringify::ToUpperLiteral(char Char)
		{
			return static_cast<char>(std::toupper(static_cast<uint8_t>(Char)));
		}
		bool Stringify::CaseEquals(const std::string_view& Value1, const std::string_view& Value2)
		{
			return Value1.size() == Value2.size() && std::equal(Value1.begin(), Value1.end(), Value2.begin(), [](const uint8_t A, const uint8_t B)
			{
				return std::tolower(A) == std::tolower(B);
			});
		}
		int Stringify::CaseCompare(const std::string_view& Value1, const std::string_view& Value2)
		{
			int Diff = 0;
			size_t Size = std::min(Value1.size(), Value2.size());
			size_t Offset = 0;
			do
			{
				Diff = tolower((uint8_t)Value1[Offset]) - tolower((uint8_t)Value2[Offset]);
				++Offset;
			} while (Diff == 0 && Offset < Size);
			return Diff;
		}
		int Stringify::Match(const char* Pattern, const std::string_view& Text)
		{
			VI_ASSERT(Pattern != nullptr, "pattern and text should be set");
			return Match(Pattern, strlen(Pattern), Text);
		}
		int Stringify::Match(const char* Pattern, size_t Length, const std::string_view& Text)
		{
			VI_ASSERT(Pattern != nullptr, "pattern and text should be set");
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
						Offset = (int)Text.substr(j).size();
						i++;
					}
					else
						Offset = (int)strcspn(Text.data() + j, "/");

					if (i == Length)
						return j + Offset;

					do
					{
						Result = Match(Pattern + i, Length - i, Text.substr(j + Offset));
					} while (Result == -1 && Offset-- > 0);

					return (Result == -1) ? -1 : j + Result + Offset;
				}
				else if (tolower((const uint8_t)Pattern[i]) != tolower((const uint8_t)Text[j]))
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
		WideString Stringify::ToWide(const std::string_view& Other)
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
		Vector<String> Stringify::Split(const std::string_view& Other, const std::string_view& With, size_t Start)
		{
			Vector<String> Output;
			PmSplit(Output, Other, With, Start);
			return Output;
		}
		Vector<String> Stringify::Split(const std::string_view& Other, char With, size_t Start)
		{
			Vector<String> Output;
			PmSplit(Output, Other, With, Start);
			return Output;
		}
		Vector<String> Stringify::SplitMax(const std::string_view& Other, char With, size_t Count, size_t Start)
		{
			Vector<String> Output;
			PmSplitMax(Output, Other, With, Count, Start);
			return Output;
		}
		Vector<String> Stringify::SplitOf(const std::string_view& Other, const std::string_view& With, size_t Start)
		{
			Vector<String> Output;
			PmSplitOf(Output, Other, With, Start);
			return Output;
		}
		Vector<String> Stringify::SplitNotOf(const std::string_view& Other, const std::string_view& With, size_t Start)
		{
			Vector<String> Output;
			PmSplitNotOf(Output, Other, With, Start);
			return Output;
		}
		void Stringify::PmSplit(Vector<String>& Output, const std::string_view& Other, const std::string_view& With, size_t Start)
		{
			if (Start >= Other.size())
				return;

			size_t Offset = Start;
			TextSettle Result = Find(Other, With, Offset);
			while (Result.Found)
			{
				Output.emplace_back(Other.substr(Offset, Result.Start - Offset));
				Result = Find(Other, With, Offset = Result.End);
			}

			if (Offset < Other.size())
				Output.emplace_back(Other.substr(Offset));
		}
		void Stringify::PmSplit(Vector<String>& Output, const std::string_view& Other, char With, size_t Start)
		{
			if (Start >= Other.size())
				return;

			size_t Offset = Start;
			TextSettle Result = Find(Other, With, Start);
			while (Result.Found)
			{
				Output.emplace_back(Other.substr(Offset, Result.Start - Offset));
				Result = Find(Other, With, Offset = Result.End);
			}

			if (Offset < Other.size())
				Output.emplace_back(Other.substr(Offset));
		}
		void Stringify::PmSplitMax(Vector<String>& Output, const std::string_view& Other, char With, size_t Count, size_t Start)
		{
			if (Start >= Other.size())
				return;

			size_t Offset = Start;
			TextSettle Result = Find(Other, With, Start);
			while (Result.Found && Output.size() < Count)
			{
				Output.emplace_back(Other.substr(Offset, Result.Start - Offset));
				Result = Find(Other, With, Offset = Result.End);
			}

			if (Offset < Other.size() && Output.size() < Count)
				Output.emplace_back(Other.substr(Offset));
		}
		void Stringify::PmSplitOf(Vector<String>& Output, const std::string_view& Other, const std::string_view& With, size_t Start)
		{
			if (Start >= Other.size())
				return;

			size_t Offset = Start;
			TextSettle Result = FindOf(Other, With, Start);
			while (Result.Found)
			{
				Output.emplace_back(Other.substr(Offset, Result.Start - Offset));
				Result = FindOf(Other, With, Offset = Result.End);
			}

			if (Offset < Other.size())
				Output.emplace_back(Other.substr(Offset));
		}
		void Stringify::PmSplitNotOf(Vector<String>& Output, const std::string_view& Other, const std::string_view& With, size_t Start)
		{
			if (Start >= Other.size())
				return;

			size_t Offset = Start;
			TextSettle Result = FindNotOf(Other, With, Start);
			while (Result.Found)
			{
				Output.emplace_back(Other.substr(Offset, Result.Start - Offset));
				Result = FindNotOf(Other, With, Offset = Result.End);
			}

			if (Offset < Other.size())
				Output.emplace_back(Other.substr(Offset));
		}
		void Stringify::ConvertToWide(const std::string_view& Input, wchar_t* Output, size_t OutputSize)
		{
			VI_ASSERT(Output != nullptr && OutputSize > 0, "output should be set");

			wchar_t W = '\0'; size_t Size = 0;
			for (size_t i = 0; i < Input.size();)
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
		Schema* Var::Set::Auto(const std::string_view& Value, bool Strict)
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
		Schema* Var::Set::String(const std::string_view& Value)
		{
			return new Schema(Var::String(Value));
		}
		Schema* Var::Set::Binary(const std::string_view& Value)
		{
			return Binary((uint8_t*)Value.data(), Value.size());
		}
		Schema* Var::Set::Binary(const uint8_t* Value, size_t Size)
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
		Schema* Var::Set::DecimalString(const std::string_view& Value)
		{
			return new Schema(Var::DecimalString(Value));
		}
		Schema* Var::Set::Boolean(bool Value)
		{
			return new Schema(Var::Boolean(Value));
		}

		Variant Var::Auto(const std::string_view& Value, bool Strict)
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
		Variant Var::String(const std::string_view& Value)
		{
			Variant Result(VarType::String);
			Result.Length = (uint32_t)Value.size();

			size_t StringSize = sizeof(char) * (Result.Length + 1);
			if (Result.Length > Variant::GetMaxSmallStringSize())
				Result.Value.Pointer = Memory::Allocate<char>(StringSize);

			char* Data = (char*)Result.GetString().data();
			memcpy(Data, Value.data(), StringSize - sizeof(char));
			Data[StringSize - 1] = '\0';

			return Result;
		}
		Variant Var::Binary(const std::string_view& Value)
		{
			return Binary((uint8_t*)Value.data(), Value.size());
		}
		Variant Var::Binary(const uint8_t* Value, size_t Size)
		{
			VI_ASSERT(Value != nullptr, "value should be set");
			Variant Result(VarType::Binary);
			Result.Length = (uint32_t)Size;

			size_t StringSize = sizeof(char) * (Result.Length + 1);
			if (Result.Length > Variant::GetMaxSmallStringSize())
				Result.Value.Pointer = Memory::Allocate<char>(StringSize);

			char* Data = (char*)Result.GetString().data();
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
			Core::Decimal* Buffer = Memory::New<Core::Decimal>(Value);
			Variant Result(VarType::Decimal);
			Result.Value.Pointer = (char*)Buffer;
			return Result;
		}
		Variant Var::Decimal(Core::Decimal&& Value)
		{
			Core::Decimal* Buffer = Memory::New<Core::Decimal>(std::move(Value));
			Variant Result(VarType::Decimal);
			Result.Value.Pointer = (char*)Buffer;
			return Result;
		}
		Variant Var::DecimalString(const std::string_view& Value)
		{
			Core::Decimal* Buffer = Memory::New<Core::Decimal>(Value);
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
		bool Composer::Pop(const std::string_view& Hash) noexcept
		{
			VI_ASSERT(Context != nullptr, "composer should be initialized");
			VI_TRACE("[composer] pop %.*s", (int)Hash.size(), Hash.data());

			auto It = Context->Factory.find(VI_HASH(Hash));
			if (It == Context->Factory.end())
				return false;

			Context->Factory.erase(It);
			return true;
		}
		void Composer::Cleanup() noexcept
		{
			Memory::Delete(Context);
			Context = nullptr;
		}
		void Composer::Push(uint64_t TypeId, uint64_t Tag, void* Callback) noexcept
		{
			VI_TRACE("[composer] push type %" PRIu64 " tagged as %" PRIu64, TypeId, Tag);
			if (!Context)
				Context = Memory::New<State>();

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

		Console::Console() noexcept
		{
			Tokens.Default =
			{
				/* Scheduling */
				ColorToken(StdColor::Yellow, "spawn"),
				ColorToken(StdColor::Yellow, "despawn"),
				ColorToken(StdColor::Yellow, "start"),
				ColorToken(StdColor::Yellow, "stop"),
				ColorToken(StdColor::Yellow, "resume"),
				ColorToken(StdColor::Yellow, "suspend"),
				ColorToken(StdColor::Yellow, "acquire"),
				ColorToken(StdColor::Yellow, "release"),
				ColorToken(StdColor::Yellow, "execute"),
				ColorToken(StdColor::Yellow, "join"),
				ColorToken(StdColor::DarkRed, "terminate"),
				ColorToken(StdColor::DarkRed, "abort"),
				ColorToken(StdColor::DarkRed, "exit"),
				ColorToken(StdColor::Cyan, "thread"),
				ColorToken(StdColor::Cyan, "process"),
				ColorToken(StdColor::Cyan, "sync"),
				ColorToken(StdColor::Cyan, "async"),
				ColorToken(StdColor::Cyan, "ms"),
				ColorToken(StdColor::Cyan, "us"),
				ColorToken(StdColor::Cyan, "ns"),

				/* Networking and IO */
				ColorToken(StdColor::Yellow, "open"),
				ColorToken(StdColor::Yellow, "close"),
				ColorToken(StdColor::Yellow, "closed"),
				ColorToken(StdColor::Yellow, "shutdown"),
				ColorToken(StdColor::Yellow, "bind"),
				ColorToken(StdColor::Yellow, "assign"),
				ColorToken(StdColor::Yellow, "resolve"),
				ColorToken(StdColor::Yellow, "listen"),
				ColorToken(StdColor::Yellow, "unlisten"),
				ColorToken(StdColor::Yellow, "accept"),
				ColorToken(StdColor::Yellow, "connect"),
				ColorToken(StdColor::Yellow, "reconnect"),
				ColorToken(StdColor::Yellow, "handshake"),
				ColorToken(StdColor::Yellow, "reset"),
				ColorToken(StdColor::Yellow, "read"),
				ColorToken(StdColor::Yellow, "write"),
				ColorToken(StdColor::Yellow, "seek"),
				ColorToken(StdColor::Yellow, "tell"),
				ColorToken(StdColor::Yellow, "scan"),
				ColorToken(StdColor::Yellow, "fetch"),
				ColorToken(StdColor::Yellow, "check"),
				ColorToken(StdColor::Yellow, "compare"),
				ColorToken(StdColor::Yellow, "stat"),
				ColorToken(StdColor::Yellow, "migrate"),
				ColorToken(StdColor::Yellow, "prepare"),
				ColorToken(StdColor::Yellow, "load"),
				ColorToken(StdColor::Yellow, "unload"),
				ColorToken(StdColor::Yellow, "save"),
				ColorToken(StdColor::Magenta, "query"),
				ColorToken(StdColor::Blue, "template"),
				ColorToken(StdColor::Cyan, "byte"),
				ColorToken(StdColor::Cyan, "bytes"),
				ColorToken(StdColor::Cyan, "epoll"),
				ColorToken(StdColor::Cyan, "kqueue"),
				ColorToken(StdColor::Cyan, "poll"),
				ColorToken(StdColor::Cyan, "dns"),
				ColorToken(StdColor::Cyan, "file"),
				ColorToken(StdColor::Cyan, "sock"),
				ColorToken(StdColor::Cyan, "dir"),
				ColorToken(StdColor::Cyan, "fd"),

				/* Graphics */
				ColorToken(StdColor::Yellow, "compile"),
				ColorToken(StdColor::Yellow, "transpile"),
				ColorToken(StdColor::Yellow, "show"),
				ColorToken(StdColor::Yellow, "hide"),
				ColorToken(StdColor::Yellow, "clear"),
				ColorToken(StdColor::Yellow, "resize"),
				ColorToken(StdColor::Magenta, "vcall"),
				ColorToken(StdColor::Cyan, "shader"),
				ColorToken(StdColor::Cyan, "bytecode"),

				/* Audio */
				ColorToken(StdColor::Yellow, "play"),
				ColorToken(StdColor::Yellow, "stop"),
				ColorToken(StdColor::Yellow, "apply"),

				/* Engine */
				ColorToken(StdColor::Yellow, "configure"),
				ColorToken(StdColor::Yellow, "actualize"),
				ColorToken(StdColor::Yellow, "register"),
				ColorToken(StdColor::Yellow, "unregister"),
				ColorToken(StdColor::Cyan, "entity"),
				ColorToken(StdColor::Cyan, "component"),
				ColorToken(StdColor::Cyan, "material"),

				/* Crypto */
				ColorToken(StdColor::Yellow, "encode"),
				ColorToken(StdColor::Yellow, "decode"),
				ColorToken(StdColor::Yellow, "encrypt"),
				ColorToken(StdColor::Yellow, "decrypt"),
				ColorToken(StdColor::Yellow, "compress"),
				ColorToken(StdColor::Yellow, "decompress"),
				ColorToken(StdColor::Yellow, "transform"),
				ColorToken(StdColor::Yellow, "shuffle"),
				ColorToken(StdColor::Yellow, "sign"),
				ColorToken(StdColor::DarkRed, "expose"),

				/* Memory */
				ColorToken(StdColor::Yellow, "add"),
				ColorToken(StdColor::Yellow, "remove"),
				ColorToken(StdColor::Yellow, "new"),
				ColorToken(StdColor::Yellow, "delete"),
				ColorToken(StdColor::Yellow, "create"),
				ColorToken(StdColor::Yellow, "destroy"),
				ColorToken(StdColor::Yellow, "push"),
				ColorToken(StdColor::Yellow, "pop"),
				ColorToken(StdColor::Yellow, "malloc"),
				ColorToken(StdColor::Yellow, "free"),
				ColorToken(StdColor::Yellow, "allocate"),
				ColorToken(StdColor::Yellow, "deallocate"),
				ColorToken(StdColor::Yellow, "initialize"),
				ColorToken(StdColor::Yellow, "generate"),
				ColorToken(StdColor::Yellow, "finalize"),
				ColorToken(StdColor::Yellow, "cleanup"),
				ColorToken(StdColor::Yellow, "copy"),
				ColorToken(StdColor::Yellow, "fill"),
				ColorToken(StdColor::Yellow, "store"),
				ColorToken(StdColor::Yellow, "reuse"),
				ColorToken(StdColor::Yellow, "update"),
				ColorToken(StdColor::Cyan, "true"),
				ColorToken(StdColor::Cyan, "false"),
				ColorToken(StdColor::Cyan, "on"),
				ColorToken(StdColor::Cyan, "off"),
				ColorToken(StdColor::Cyan, "undefined"),
				ColorToken(StdColor::Cyan, "nullptr"),
				ColorToken(StdColor::Cyan, "null"),
				ColorToken(StdColor::Cyan, "this"),

				/* Statuses */
				ColorToken(StdColor::DarkGreen, "ON"),
				ColorToken(StdColor::DarkGreen, "TRUE"),
				ColorToken(StdColor::DarkGreen, "OK"),
				ColorToken(StdColor::DarkGreen, "SUCCESS"),
				ColorToken(StdColor::Yellow, "ASSERT"),
				ColorToken(StdColor::Yellow, "CAUSING"),
				ColorToken(StdColor::Yellow, "warn"),
				ColorToken(StdColor::Yellow, "warning"),
				ColorToken(StdColor::Yellow, "debug"),
				ColorToken(StdColor::Yellow, "debugging"),
				ColorToken(StdColor::Yellow, "trace"),
				ColorToken(StdColor::Yellow, "trading"),
				ColorToken(StdColor::DarkRed, "OFF"),
				ColorToken(StdColor::DarkRed, "FALSE"),
				ColorToken(StdColor::DarkRed, "NULL"),
				ColorToken(StdColor::DarkRed, "ERR"),
				ColorToken(StdColor::DarkRed, "FATAL"),
				ColorToken(StdColor::DarkRed, "PANIC!"),
				ColorToken(StdColor::DarkRed, "leaking"),
				ColorToken(StdColor::DarkRed, "failure"),
				ColorToken(StdColor::DarkRed, "failed"),
				ColorToken(StdColor::DarkRed, "error"),
				ColorToken(StdColor::DarkRed, "errors"),
				ColorToken(StdColor::DarkRed, "cannot"),
				ColorToken(StdColor::DarkRed, "missing"),
				ColorToken(StdColor::DarkRed, "invalid"),
				ColorToken(StdColor::DarkRed, "required"),
				ColorToken(StdColor::DarkRed, "already"),
			};
			Tokens.Custom = Tokens.Default;
		}
		Console::~Console() noexcept
		{
			Deallocate();
		}
		void Console::Allocate()
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			if (State.Status != Mode::Detached)
				return;
#ifdef VI_MICROSOFT
			if (AllocConsole())
			{
				Streams.Input = freopen("conin$", "r", stdin);
				Streams.Output = freopen("conout$", "w", stdout);
				Streams.Errors = freopen("conout$", "w", stderr);
			}

			CONSOLE_SCREEN_BUFFER_INFO ScreenBuffer;
			HANDLE Handle = GetStdHandle(STD_OUTPUT_HANDLE);
			if (GetConsoleScreenBufferInfo(Handle, &ScreenBuffer))
				State.Attributes = ScreenBuffer.wAttributes;

			SetConsoleCtrlHandler(ConsoleEventHandler, true);
			VI_TRACE("[console] allocate window 0x%" PRIXPTR, (void*)Handle);
#endif
			State.Status = Mode::Allocated;
		}
		void Console::Deallocate()
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			if (State.Status != Mode::Allocated)
				return;
#ifdef VI_MICROSOFT
			::ShowWindow(::GetConsoleWindow(), SW_HIDE);
			VI_TRACE("[console] deallocate window");
#endif
			State.Status = Mode::Detached;
		}
		void Console::Hide()
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
#ifdef VI_MICROSOFT
			VI_TRACE("[console] hide window");
			::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif
		}
		void Console::Show()
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			Allocate();
#ifdef VI_MICROSOFT
			::ShowWindow(::GetConsoleWindow(), SW_SHOW);
			VI_TRACE("[console] show window");
#endif
		}
		void Console::Clear()
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			State.Elements.clear();
#ifdef VI_MICROSOFT
			HANDLE Handle = GetStdHandle(STD_OUTPUT_HANDLE);
			DWORD Written = 0;

			CONSOLE_SCREEN_BUFFER_INFO Info;
			GetConsoleScreenBufferInfo(Handle, &Info);

			COORD TopLeft = { 0, 0 };
			FillConsoleOutputCharacterA(Handle, ' ', Info.dwSize.X * Info.dwSize.Y, TopLeft, &Written);
			FillConsoleOutputAttribute(Handle, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE, Info.dwSize.X * Info.dwSize.Y, TopLeft, &Written);
			SetConsoleCursorPosition(Handle, TopLeft);
#else
			int ExitCode = std::system("clear");
			(void)ExitCode;
#endif
		}
		void Console::Attach()
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			if (State.Status != Mode::Detached)
				return;
#ifdef VI_MICROSOFT
			CONSOLE_SCREEN_BUFFER_INFO ScreenBuffer;
			HANDLE Handle = GetStdHandle(STD_OUTPUT_HANDLE);
			if (GetConsoleScreenBufferInfo(Handle, &ScreenBuffer))
				State.Attributes = ScreenBuffer.wAttributes;

			SetConsoleCtrlHandler(ConsoleEventHandler, true);
			VI_TRACE("[console] attach window 0x%" PRIXPTR, (void*)Handle);
#endif
			State.Status = Mode::Attached;
		}
		void Console::Detach()
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			State.Status = Mode::Detached;
		}
		void Console::Trace(uint32_t MaxFrames)
		{
			String Stacktrace = ErrorHandling::GetStackTrace(0, MaxFrames);
			UMutex<std::recursive_mutex> Unique(State.Session);
			std::cout << Stacktrace << '\n';
		}
		void Console::SetColoring(bool Enabled)
		{
			State.Colors = Enabled;
		}
		void Console::SetColorTokens(Vector<Console::ColorToken>&& AdditionalTokens)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			Tokens.Custom = std::move(AdditionalTokens);
			Tokens.Custom.reserve(Tokens.Custom.size() + Tokens.Default.size());
			Tokens.Custom.insert(Tokens.Custom.end(), Tokens.Default.begin(), Tokens.Default.end());
		}
		void Console::ColorBegin(StdColor Text, StdColor Background)
		{
			if (!State.Colors)
				return;

			UMutex<std::recursive_mutex> Unique(State.Session);
#ifdef VI_MICROSOFT
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
			if (!State.Colors)
				return;
			UMutex<std::recursive_mutex> Unique(State.Session);
#ifdef VI_MICROSOFT
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), State.Attributes);
#else
			std::cout << "\033[0m";
#endif
		}
		void Console::ColorPrint(StdColor BaseColor, const std::string_view& Buffer)
		{
			if (Buffer.empty())
				return;

			UMutex<std::recursive_mutex> Unique(State.Session);
			ColorBegin(BaseColor);

			size_t Offset = 0;
			while (Offset < Buffer.size())
			{
				auto V = Buffer[Offset];
				if (Stringify::IsNumericOrDot(V) && (!Offset || !Stringify::IsAlphanum(Buffer[Offset - 1])))
				{
					ColorBegin(StdColor::Yellow);
					while (Offset < Buffer.size())
					{
						char N = Buffer[Offset];
						if (!Stringify::IsHexOrDot(N) && N != 'x')
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

					while (Offset < Buffer.size() && (Stringify::IsNumeric(Buffer[++Offset]) || Stringify::IsAlphabetic(Buffer[Offset]) || Buffer[Offset] == '-' || Buffer[Offset] == '_'))
						WriteChar(Buffer[Offset]);

					ColorBegin(BaseColor);
					continue;
				}
				else if (V == '[' && Buffer.substr(Offset + 1).find(']') != std::string::npos)
				{
					size_t Iterations = 0, Skips = 0;
					ColorBegin(StdColor::Cyan);
					do
					{
						WriteChar(Buffer[Offset]);
						if (Iterations++ > 0 && Buffer[Offset] == '[')
							Skips++;
					} while (Offset < Buffer.size() && (Buffer[Offset++] != ']' || Skips > 0));

					ColorBegin(BaseColor);
					continue;
				}
				else if (V == '\"' && Buffer.substr(Offset + 1).find('\"') != std::string::npos)
				{
					ColorBegin(StdColor::LightBlue);
					do
					{
						WriteChar(Buffer[Offset]);
					} while (Offset < Buffer.size() && Buffer[++Offset] != '\"');

					if (Offset < Buffer.size())
						WriteChar(Buffer[Offset++]);
					ColorBegin(BaseColor);
					continue;
				}
				else if (V == '\'' && Buffer.substr(Offset + 1).find('\'') != std::string::npos)
				{
					ColorBegin(StdColor::LightBlue);
					do
					{
						WriteChar(Buffer[Offset]);
					} while (Offset < Buffer.size() && Buffer[++Offset] != '\'');

					if (Offset < Buffer.size())
						WriteChar(Buffer[Offset++]);
					ColorBegin(BaseColor);
					continue;
				}
				else if (Stringify::IsAlphabetic(V) && (!Offset || !Stringify::IsAlphabetic(Buffer[Offset - 1])))
				{
					bool IsMatched = false;
					for (auto& Token : Tokens.Custom)
					{
						if (V != Token.First || Buffer.size() - Offset < Token.Size)
							continue;

						if (Offset + Token.Size < Buffer.size() && Stringify::IsAlphabetic(Buffer[Offset + Token.Size]))
							continue;

						if (memcmp(Buffer.data() + Offset, Token.Token, Token.Size) == 0)
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
		void Console::CaptureTime()
		{
			State.Time = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
		}
		uint64_t Console::CaptureWindow(uint32_t Height)
		{
			if (!Height)
				return 0;

			WindowState Window;
			Window.Elements.reserve(Height);

			UMutex<std::recursive_mutex> Unique(State.Session);
			for (size_t i = 0; i < Height; i++)
			{
				uint64_t Id = CaptureElement();
				if (Id > 0)
				{
					Window.Elements.push_back(std::make_pair(Id, String()));
					continue;
				}

				for (auto& Element : Window.Elements)
					FreeElement(Element.first);

				return 0;
			}

			uint64_t Id = ++State.Id;
			State.Windows[Id] = std::move(Window);
			return Id;
		}
		void Console::FreeWindow(uint64_t Id, bool RestorePosition)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			auto It = State.Windows.find(Id);
			if (It == State.Windows.end())
				return;

			uint32_t Y = 0;
			bool Exists = false;
			if (RestorePosition && !It->second.Elements.empty())
			{
				auto Root = State.Elements.find(It->second.Elements.front().first);
				if (Root != State.Elements.end())
				{
					Y = Root->second.Y;
					Exists = true;
				}
			}

			for (auto& Element : It->second.Elements)
			{
				ClearElement(Element.first);
				FreeElement(Element.first);
			}

			State.Windows.erase(Id);
			if (Exists && RestorePosition)
				WritePosition(0, Y);
		}
		void Console::EmplaceWindow(uint64_t Id, const std::string_view& Text)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			auto It = State.Windows.find(Id);
			if (It == State.Windows.end() || It->second.Elements.empty())
				return;

			size_t Count = It->second.Elements.size();
			if (It->second.Position >= Count)
			{
				--Count;
				for (size_t i = 0; i < Count; i++)
				{
					auto& PrevElement = It->second.Elements[i + 0];
					auto& NextElement = It->second.Elements[i + 1];
					if (NextElement.second == PrevElement.second)
						continue;

					ReplaceElement(PrevElement.first, NextElement.second);
					PrevElement.second = std::move(NextElement.second);
				}
				It->second.Position = Count;
			}

			auto& Element = It->second.Elements[It->second.Position++];
			Element.second = Text;
			ReplaceElement(Element.first, Element.second);
		}
		uint64_t Console::CaptureElement()
		{
			ElementState Element;
			Element.State = Compute::Crypto::Random();

			UMutex<std::recursive_mutex> Unique(State.Session);
			if (!ReadScreen(nullptr, nullptr, &Element.X, &Element.Y))
				return 0;

			uint64_t Id = ++State.Id;
			State.Elements[Id] = Element;
			std::cout << '\n';
			return Id;
		}
		void Console::FreeElement(uint64_t Id)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			auto It = State.Elements.find(Id);
			if (It != State.Elements.end())
				State.Elements.erase(Id);
		}
		void Console::ResizeElement(uint64_t Id, uint32_t X)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			auto It = State.Elements.find(Id);
			if (It != State.Elements.end())
				It->second.X = X;
		}
		void Console::MoveElement(uint64_t Id, uint32_t Y)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			auto It = State.Elements.find(Id);
			if (It != State.Elements.end())
				It->second.Y = Y;
		}
		void Console::ReadElement(uint64_t Id, uint32_t* X, uint32_t* Y)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			auto It = State.Elements.find(Id);
			if (It != State.Elements.end())
			{
				if (X != nullptr)
					*X = It->second.X;
				if (Y != nullptr)
					*Y = It->second.Y;
			}
		}
		void Console::ReplaceElement(uint64_t Id, const std::string_view& Text)
		{
			String Writeable = String(Text);
			Stringify::ReplaceOf(Writeable, "\b\f\n\r\v", " ");
			Stringify::Replace(Writeable, "\t", "  ");

			UMutex<std::recursive_mutex> Unique(State.Session);
			auto It = State.Elements.find(Id);
			if (It == State.Elements.end())
				return;

			uint32_t Width = 0, Height = 0, X = 0, Y = 0;
			if (!ReadScreen(&Width, &Height, &X, &Y))
				return;

			WritePosition(0, It->second.Y >= --Height ? It->second.Y - 1 : It->second.Y);
			Writeable = Writeable.substr(0, --Width);
			Writeable.append(Width - Writeable.size(), ' ');
			Writeable.append(1, '\n');
			std::cout << Writeable;
			WritePosition(X, Y);
		}
		void Console::ProgressElement(uint64_t Id, double Value, double Coverage)
		{
			uint32_t ScreenWidth;
			if (!ReadScreen(&ScreenWidth, nullptr, nullptr, nullptr))
				return;
			else if (ScreenWidth < 8)
				return;

			String BarContent;
			BarContent.reserve(ScreenWidth);
			BarContent += '[';
			ScreenWidth -= 8;

			size_t BarWidth = (size_t)(ScreenWidth * std::max<double>(0.0, std::min<double>(1.0, Coverage)));
			size_t BarPosition = (size_t)(BarWidth * std::max<double>(0.0, std::min<double>(1.0, Value)));
			for (size_t i = 0; i < BarWidth; i++)
			{
				if (i < BarPosition)
					BarContent += '=';
				else if (i == BarPosition)
					BarContent += '>';
				else
					BarContent += ' ';
			}

			char Numeric[NUMSTR_SIZE];
			BarContent += "] ";
			BarContent += ToStringView<uint32_t>(Numeric, sizeof(Numeric), (uint32_t)(Value * 100));
			BarContent += " %";
			ReplaceElement(Id, BarContent);
		}
		void Console::SpinningElement(uint64_t Id, const std::string_view& Label)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			auto It = State.Elements.find(Id);
			if (It == State.Elements.end())
				return;

			uint64_t Status = It->second.State++ % 4;
			switch (Status)
			{
				case 0:
					ReplaceElement(Id, Label.empty() ? "[|]" : String(Label) + " [|]");
					break;
				case 1:
					ReplaceElement(Id, Label.empty() ? "[/]" : String(Label) + " [/]");
					break;
				case 2:
					ReplaceElement(Id, Label.empty() ? "[-]" : String(Label) + " [-]");
					break;
				case 3:
					ReplaceElement(Id, Label.empty() ? "[\\]" : String(Label) + " [\\]");
					break;
				default:
					ReplaceElement(Id, Label.empty() ? "[ ]" : String(Label) + " [ ]");
					break;
			}
		}
		void Console::SpinningProgressElement(uint64_t Id, double Value, double Coverage)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			auto It = State.Elements.find(Id);
			if (It == State.Elements.end())
				return;

			uint32_t ScreenWidth;
			if (!ReadScreen(&ScreenWidth, nullptr, nullptr, nullptr))
				return;
			else if (ScreenWidth < 8)
				return;

			String BarContent;
			BarContent.reserve(ScreenWidth);
			BarContent += '[';
			ScreenWidth -= 8;

			size_t BarWidth = (size_t)(ScreenWidth * std::max<double>(0.0, std::min<double>(1.0, Coverage)));
			size_t BarPosition = (size_t)(BarWidth * std::max<double>(0.0, std::min<double>(1.0, Value)));
			for (size_t i = 0; i < BarWidth; i++)
			{
				if (i == BarPosition)
				{
					uint8_t Status = It->second.State++ % 4;
					switch (Status)
					{
						case 0:
							BarContent += '|';
							break;
						case 1:
							BarContent += '/';
							break;
						case 2:
							BarContent += '-';
							break;
						case 3:
							BarContent += '\\';
							break;
						default:
							BarContent += '>';
							break;
					}
				}
				else if (i < BarPosition)
					BarContent += '=';
				else
					BarContent += ' ';
			}

			char Numeric[NUMSTR_SIZE];
			BarContent += "] ";
			BarContent += ToStringView<uint32_t>(Numeric, sizeof(Numeric), (uint32_t)(Value * 100));
			BarContent += " %";
			ReplaceElement(Id, BarContent);
		}
		void Console::ClearElement(uint64_t Id)
		{
			ReplaceElement(Id, String());
		}
		void Console::Flush()
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			std::cout.flush();
		}
		void Console::FlushWrite()
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			std::cout << std::flush;
		}
		void Console::WriteSize(uint32_t Width, uint32_t Height)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
#ifdef VI_MICROSOFT
			SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), { (short)Width, (short)Height });
#else
			struct winsize Size;
			Size.ws_col = Width;
			Size.ws_row = Height;
			ioctl(STDOUT_FILENO, TIOCSWINSZ, &Size);
#endif
		}
		void Console::WritePosition(uint32_t X, uint32_t Y)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
#ifdef VI_MICROSOFT
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (short)X, (short)Y });
#else
			std::cout << "\033[" << X << ';' << Y << 'H';
#endif
		}
		void Console::WriteLine(const std::string_view& Line)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			std::cout << Line << '\n';
		}
		void Console::WriteChar(char Value)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			std::cout << Value;
		}
		void Console::Write(const std::string_view& Text)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			std::cout << Text;
		}
		void Console::jWrite(Schema* Data)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			if (!Data)
			{
				std::cout << "null";
				return;
			}

			String Offset;
			Schema::ConvertToJSON(Data, [&Offset](Core::VarForm Pretty, const std::string_view& Buffer)
			{
				if (!Buffer.empty())
					std::cout << Buffer;

				switch (Pretty)
				{
					case Vitex::Core::VarForm::Tab_Decrease:
						Offset.erase(Offset.size() - 2);
						break;
					case Vitex::Core::VarForm::Tab_Increase:
						Offset.append(2, ' ');
						break;
					case Vitex::Core::VarForm::Write_Space:
						std::cout << ' ';
						break;
					case Vitex::Core::VarForm::Write_Line:
						std::cout << '\n';
						break;
					case Vitex::Core::VarForm::Write_Tab:
						std::cout << Offset;
						break;
					default:
						break;
				}
			});
		}
		void Console::jWriteLine(Schema* Data)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
			jWrite(Data);
			std::cout << '\n';
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

			UMutex<std::recursive_mutex> Unique(State.Session);
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

			UMutex<std::recursive_mutex> Unique(State.Session);
			std::cout << Buffer;
		}
		double Console::GetCapturedTime() const
		{
			return (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0 - State.Time;
		}
		bool Console::ReadScreen(uint32_t* Width, uint32_t* Height, uint32_t* X, uint32_t* Y)
		{
			UMutex<std::recursive_mutex> Unique(State.Session);
#ifdef VI_MICROSOFT
			CONSOLE_SCREEN_BUFFER_INFO Size;
			if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &Size) != TRUE)
				return false;

			if (Width != nullptr)
				*Width = (uint32_t)(Size.srWindow.Right - Size.srWindow.Left + 1);

			if (Height != nullptr)
				*Height = (uint32_t)(Size.srWindow.Bottom - Size.srWindow.Top + 1);

			if (X != nullptr)
				*X = (uint32_t)Size.dwCursorPosition.X;

			if (Y != nullptr)
				*Y = (uint32_t)Size.dwCursorPosition.Y;

			return true;
#else
			if (Width != nullptr || Height != nullptr)
			{
				struct winsize Size;
				if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &Size) < 0)
					return false;
				
				if (Width != nullptr)
					*Width = (uint32_t)Size.ws_col;

				if (Height != nullptr)
					*Height = (uint32_t)Size.ws_row;
			}
			
			if (X != nullptr || Y != nullptr)
			{
				static bool Responsive = true;
				if (!Responsive)
					return false;
				
				struct termios Prev;
				tcgetattr(STDIN_FILENO, &Prev);

				struct termios Next = Prev;
				Next.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
				Next.c_oflag &= ~(OPOST);
				Next.c_cflag |= (CS8);
				Next.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
				Next.c_cc[VMIN] = 0;
				Next.c_cc[VTIME] = 0;

				tcsetattr(STDIN_FILENO, TCSANOW, &Next);
				tcsetattr(STDOUT_FILENO, TCSANOW, &Next);
				fwrite("\033[6n", 4, 1, stdout);
				fflush(stdout);

				fd_set Fd;
				FD_ZERO(&Fd);
				FD_SET(STDIN_FILENO, &Fd);

				struct timeval Time;
				Time.tv_sec = 0;
				Time.tv_usec = 500000;

				if (select(STDIN_FILENO + 1, &Fd, nullptr, nullptr, &Time) != 1)
				{
					Responsive = false;
					tcsetattr(STDIN_FILENO, TCSADRAIN, &Prev);
					return false;
				}
				
				int TargetX = 0, TargetY = 0;
				if (scanf("\033[%d;%dR", &TargetX, &TargetY) != 2)
				{
					tcsetattr(STDIN_FILENO, TCSADRAIN, &Prev);
					return false;
				}
		
				if (X != nullptr)
					*X = TargetX;
			
				if (Y != nullptr)
					*Y = TargetY;
		
				tcsetattr(STDIN_FILENO, TCSADRAIN, &Prev);
			}

			return true;
#endif
		}
		bool Console::ReadLine(String& Data, size_t Size)
		{
			VI_ASSERT(State.Status != Mode::Detached, "console should be shown at least once");
			VI_ASSERT(Size > 0, "read length should be greater than zero");
			VI_TRACE("[console] read up to %" PRIu64 " bytes", (uint64_t)Size);

			bool Success;
			if (Size > CHUNK_SIZE - 1)
			{
				char* Value = Memory::Allocate<char>(sizeof(char) * (Size + 1));
				memset(Value, 0, (Size + 1) * sizeof(char));
				if ((Success = (bool)std::cin.getline(Value, Size)))
					Data.assign(Value);
				else
					Data.clear();
				Memory::Deallocate(Value);
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
			return HasInstance() && Get()->State.Status != Mode::Detached;
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
		ExpectsIO<void> Stream::Move(int64_t Offset)
		{
			return Seek(FileSeek::Current, Offset);
		}
		void Stream::OpenVirtual(String&& Path)
		{
			VName = std::move(Path);
			VSize = 0;
		}
		void Stream::CloseVirtual()
		{
			VName.clear();
			VSize = 0;
		}
		void Stream::SetVirtualSize(size_t Size)
		{
			VSize = Size;
		}
		void Stream::SetVirtualName(const std::string_view& File)
		{
			VName = File;
		}
		ExpectsIO<size_t> Stream::ReadAll(const std::function<void(uint8_t*, size_t)>& Callback)
		{
			VI_ASSERT(Callback != nullptr, "callback should be set");
			VI_TRACE("[io] read all bytes on fd %i", GetReadableFd());

			size_t Total = 0;
			uint8_t Buffer[CHUNK_SIZE];
			while (true)
			{
				size_t Length = VSize > 0 ? std::min<size_t>(sizeof(Buffer), VSize - Total) : sizeof(Buffer);
				if (!Length)
					break;

				auto Size = Read(Buffer, Length);
				if (!Size || !*Size)
					return Size;

				Length = *Size;
				Total += Length;
				Callback(Buffer, Length);
			}

			return Total;
		}
		ExpectsIO<size_t> Stream::Size()
		{
			if (!IsSized())
				return std::make_error_condition(std::errc::not_supported);

			auto Position = Tell();
			if (!Position)
				return Position;

			auto Status = Seek(FileSeek::End, 0);
			if (!Status)
				return Status.Error();

			auto Size = Tell();
			Status = Seek(FileSeek::Begin, *Position);
			if (!Status)
				return Status.Error();

			return Size;
		}
		size_t Stream::VirtualSize() const
		{
			return VSize;
		}
		std::string_view Stream::VirtualName() const
		{
			return VName;
		}

		MemoryStream::MemoryStream() noexcept : Offset(0), Readable(false), Writeable(false)
		{
		}
		MemoryStream::~MemoryStream() noexcept
		{
			Close();
		}
		ExpectsIO<void> MemoryStream::Clear()
		{
			VI_TRACE("[mem] fd %i clear", GetWriteableFd());
			Buffer.clear();
			Offset = 0;
			Readable = false;
			return Expectation::Met;
		}
		ExpectsIO<void> MemoryStream::Open(const std::string_view& File, FileMode Mode)
		{
			VI_ASSERT(!File.empty(), "filename should be set");
			VI_MEASURE(Timings::Pass);
			auto Result = Close();
			if (!Result)
				return Result;
			else if (!OS::Control::Has(AccessOption::Mem))
				return std::make_error_condition(std::errc::permission_denied);

			switch (Mode)
			{
				case FileMode::Read_Only:
				case FileMode::Binary_Read_Only:
					Readable = true;
					break;
				case FileMode::Write_Only:
				case FileMode::Binary_Write_Only:
				case FileMode::Append_Only:
				case FileMode::Binary_Append_Only:
					Writeable = true;
					break;
				case FileMode::Read_Write:
				case FileMode::Binary_Read_Write:
				case FileMode::Write_Read:
				case FileMode::Binary_Write_Read:
				case FileMode::Read_Append_Write:
				case FileMode::Binary_Read_Append_Write:
					Readable = true;
					Writeable = true;
					break;
				default:
					break;
			}

			VI_PANIC(Readable || Writeable, "file open cannot be issued with mode:%i", (int)Mode);
			VI_DEBUG("[mem] open %s%s %s fd", Readable ? "r" : "", Writeable ? "w" : "", File, (int)GetReadableFd());
			OpenVirtual(String(File));
			return Expectation::Met;
		}
		ExpectsIO<void> MemoryStream::Close()
		{
			VI_MEASURE(Timings::Pass);
			VI_DEBUG("[mem] close fd %i", GetReadableFd());
			CloseVirtual();
			Buffer.clear();
			Offset = 0;
			Readable = false;
			Writeable = false;
			return Expectation::Met;
		}
		ExpectsIO<void> MemoryStream::Seek(FileSeek Mode, int64_t Seek)
		{
			VI_ASSERT(!VirtualName().empty(), "file should be opened");
			VI_MEASURE(Timings::Pass);
			switch (Mode)
			{
				case FileSeek::Begin:
					VI_TRACE("[mem] seek-64 fd %i begin %" PRId64, GetReadableFd(), Seek);
					Offset = Seek < 0 ? 0 : Buffer.size() + (size_t)Seek;
					break;
				case FileSeek::Current:
					VI_TRACE("[mem] seek-64 fd %i move %" PRId64, GetReadableFd(), Seek);
					if (Seek < 0)
					{
						size_t Position = (size_t)(-Seek);
						Offset -= Position > Offset ? 0 : Position;
					}
					else
						Offset += (size_t)Seek;
					break;
				case FileSeek::End:
					VI_TRACE("[mem] seek-64 fd %i end %" PRId64, GetReadableFd(), Seek);
					if (Seek < 0)
					{
						size_t Position = (size_t)(-Seek);
						Offset = Position > Buffer.size() ? 0 : Buffer.size() - Position;
					}
					else
						Offset = Buffer.size() + (size_t)Seek;
					break;
				default:
					break;
			}
			return Expectation::Met;
		}
		ExpectsIO<void> MemoryStream::Flush()
		{
			return Expectation::Met;
		}
		ExpectsIO<size_t> MemoryStream::ReadScan(const char* Format, ...)
		{
			VI_ASSERT(!VirtualName().empty(), "file should be opened");
			VI_ASSERT(Format != nullptr, "format should be set");
			VI_MEASURE(Timings::Pass);
			char* Memory = PrepareBuffer(0);
			if (!Memory)
				return std::make_error_condition(std::errc::broken_pipe);

			va_list Args;
			va_start(Args, Format);
			int Value = vsscanf(Memory, Format, Args);
			VI_TRACE("[mem] fd %i scan %i bytes", GetReadableFd(), (int)Value);
			va_end(Args);
			if (Value >= 0)
				return (size_t)Value;

			return OS::Error::GetConditionOr(std::errc::broken_pipe);
		}
		ExpectsIO<size_t> MemoryStream::ReadLine(char* Data, size_t Length)
		{
			VI_ASSERT(!VirtualName().empty(), "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::Pass);
			VI_TRACE("[mem] fd %i readln %i bytes", GetReadableFd(), (int)Length);
			size_t Offset = 0;
			while (Offset < Length)
			{
				auto Status = Read((uint8_t*)Data + Offset, sizeof(uint8_t));
				if (!Status)
					return Status;
				else if (*Status != sizeof(uint8_t))
					break;
				else if (Stringify::IsWhitespace(Data[Offset++]))
					break;
			}
			return Offset;
		}
		ExpectsIO<size_t> MemoryStream::Read(uint8_t* Data, size_t Length)
		{
			VI_ASSERT(!VirtualName().empty(), "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::Pass);
			VI_TRACE("[mem] fd %i read %i bytes", GetReadableFd(), (int)Length);
			if (!Length)
				return 0;

			char* Memory = PrepareBuffer(Length);
			if (!Memory)
				return std::make_error_condition(std::errc::broken_pipe);

			memcpy(Data, Memory, Length);
			return Length;
		}
		ExpectsIO<size_t> MemoryStream::WriteFormat(const char* Format, ...)
		{
			VI_ASSERT(!VirtualName().empty(), "file should be opened");
			VI_ASSERT(Format != nullptr, "format should be set");
			VI_MEASURE(Timings::Pass);
			char* Memory = PrepareBuffer(0);
			if (!Memory)
				return std::make_error_condition(std::errc::broken_pipe);

			va_list Args;
			va_start(Args, Format);
			int Value = vsnprintf(Memory, Memory - Buffer.data(), Format, Args);
			VI_TRACE("[mem] fd %i write %i bytes", GetWriteableFd(), Value);
			va_end(Args);
			if (Value >= 0)
				return (size_t)Value;

			return OS::Error::GetConditionOr(std::errc::broken_pipe);
		}
		ExpectsIO<size_t> MemoryStream::Write(const uint8_t* Data, size_t Length)
		{
			VI_ASSERT(!VirtualName().empty(), "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::Pass);
			VI_TRACE("[mem] fd %i write %i bytes", GetWriteableFd(), (int)Length);
			if (!Length)
				return 0;

			char* Memory = PrepareBuffer(Length);
			if (!Memory)
				return std::make_error_condition(std::errc::broken_pipe);

			memcpy(Memory, Data, Length);
			return Length;
		}
		ExpectsIO<size_t> MemoryStream::Tell()
		{
			return Offset;
		}
		socket_t MemoryStream::GetReadableFd() const
		{
			return (socket_t)(uintptr_t)Buffer.data();
		}
		socket_t MemoryStream::GetWriteableFd() const
		{
			return (socket_t)(uintptr_t)Buffer.data();
		}
		void* MemoryStream::GetReadable() const
		{
			return (void*)Buffer.data();
		}
		void* MemoryStream::GetWriteable() const
		{
			return (void*)Buffer.data();
		}
		bool MemoryStream::IsSized() const
		{
			return true;
		}
		char* MemoryStream::PrepareBuffer(size_t Size)
		{
			if (Offset + Size > Buffer.size())
				Buffer.resize(Offset);

			char* Target = Buffer.data() + Offset;
			Offset += Size;
			return Target;
		}

		FileStream::FileStream() noexcept : IoStream(nullptr)
		{
		}
		FileStream::~FileStream() noexcept
		{
			Close();
		}
		ExpectsIO<void> FileStream::Clear()
		{
			VI_TRACE("[io] fd %i clear", GetWriteableFd());
			auto Result = Close();
			if (!Result)
				return Result;

			auto Path = VirtualName();
			if (Path.empty())
				return std::make_error_condition(std::errc::invalid_argument);

			auto Target = OS::File::Open(Path, "w");
			if (!Target)
				return Target.Error();
			
			IoStream = *Target;
			return Expectation::Met;
		}
		ExpectsIO<void> FileStream::Open(const std::string_view& File, FileMode Mode)
		{
			VI_ASSERT(!File.empty(), "filename should be set");
			VI_MEASURE(Timings::FileSystem);
			auto Result = Close();
			if (!Result)
				return Result;
			else if (!OS::Control::Has(AccessOption::Fs))
				return std::make_error_condition(std::errc::permission_denied);

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

			auto Target = OS::File::Open(*TargetPath, Type);
			if (!Target)
				return Target.Error();

			IoStream = *Target;
			OpenVirtual(std::move(*TargetPath));
			return Expectation::Met;
		}
		ExpectsIO<void> FileStream::Close()
		{
			VI_MEASURE(Timings::FileSystem);
			CloseVirtual();

			if (!IoStream)
				return Expectation::Met;

			FILE* Target = IoStream;
			IoStream = nullptr;
			return OS::File::Close(Target);
		}
		ExpectsIO<void> FileStream::Seek(FileSeek Mode, int64_t Offset)
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_MEASURE(Timings::FileSystem);
			return OS::File::Seek64(IoStream, Offset, Mode);
		}
		ExpectsIO<void> FileStream::Flush()
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] flush fd %i", GetWriteableFd());
			if (fflush(IoStream) != 0)
				return OS::Error::GetConditionOr();
			return Expectation::Met;
		}
		ExpectsIO<size_t> FileStream::ReadScan(const char* Format, ...)
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_ASSERT(Format != nullptr, "format should be set");
			VI_MEASURE(Timings::FileSystem);

			va_list Args;
			va_start(Args, Format);
			int Value = vfscanf(IoStream, Format, Args);
			VI_TRACE("[io] fd %i scan %i bytes", GetReadableFd(), Value);
			va_end(Args);
			if (Value >= 0)
				return (size_t)Value;

			return OS::Error::GetConditionOr(std::errc::broken_pipe);
		}
		ExpectsIO<size_t> FileStream::ReadLine(char* Data, size_t Length)
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] fd %i readln %i bytes", GetReadableFd(), (int)Length);
			return fgets(Data, (int)Length, IoStream) ? strnlen(Data, Length) : 0;
		}
		ExpectsIO<size_t> FileStream::Read(uint8_t* Data, size_t Length)
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] fd %i read %i bytes", GetReadableFd(), (int)Length);
			size_t Value = fread(Data, 1, Length, IoStream);
			if (Value > 0 || feof(IoStream) != 0)
				return (size_t)Value;

			return OS::Error::GetConditionOr(std::errc::broken_pipe);
		}
		ExpectsIO<size_t> FileStream::WriteFormat(const char* Format, ...)
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_ASSERT(Format != nullptr, "format should be set");
			VI_MEASURE(Timings::FileSystem);

			va_list Args;
			va_start(Args, Format);
			int Value = vfprintf(IoStream, Format, Args);
			VI_TRACE("[io] fd %i write %i bytes", GetWriteableFd(), Value);
			va_end(Args);
			if (Value >= 0)
				return (size_t)Value;

			return OS::Error::GetConditionOr(std::errc::broken_pipe);
		}
		ExpectsIO<size_t> FileStream::Write(const uint8_t* Data, size_t Length)
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] fd %i write %i bytes", GetWriteableFd(), (int)Length);
			size_t Value = fwrite(Data, 1, Length, IoStream);
			if (Value > 0 || feof(IoStream) != 0)
				return (size_t)Value;

			return OS::Error::GetConditionOr(std::errc::broken_pipe);
		}
		ExpectsIO<size_t> FileStream::Tell()
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_MEASURE(Timings::FileSystem);
			return OS::File::Tell64(IoStream);
		}
		socket_t FileStream::GetReadableFd() const
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			return (socket_t)VI_FILENO(IoStream);
		}
		socket_t FileStream::GetWriteableFd() const
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			return (socket_t)VI_FILENO(IoStream);
		}
		void* FileStream::GetReadable() const
		{
			return IoStream;
		}
		void* FileStream::GetWriteable() const
		{
			return IoStream;
		}
		bool FileStream::IsSized() const
		{
			return true;
		}

		GzStream::GzStream() noexcept : IoStream(nullptr)
		{
		}
		GzStream::~GzStream() noexcept
		{
			Close();
		}
		ExpectsIO<void> GzStream::Clear()
		{
			VI_TRACE("[gz] fd %i clear", GetWriteableFd());
			auto Result = Close();
			if (!Result)
				return Result;

			auto Path = VirtualName();
			if (Path.empty())
				return std::make_error_condition(std::errc::invalid_argument);

			auto Target = OS::File::Open(Path, "w");
			if (!Target)
				return Target.Error();

			Result = OS::File::Close(*Target);
			if (!Result)
				return Result;

			return Open(Path, FileMode::Binary_Write_Only);
		}
		ExpectsIO<void> GzStream::Open(const std::string_view& File, FileMode Mode)
		{
			VI_ASSERT(!File.empty(), "filename should be set");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			auto Result = Close();
			if (!Result)
				return Result;
			else if (!OS::Control::Has(AccessOption::Gz))
				return std::make_error_condition(std::errc::permission_denied);

			const char* Type = nullptr;
			switch (Mode)
			{
				case FileMode::Read_Only:
				case FileMode::Binary_Read_Only:
					Type = "rb";
					break;
				case FileMode::Write_Only:
				case FileMode::Binary_Write_Only:
				case FileMode::Append_Only:
				case FileMode::Binary_Append_Only:
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
			IoStream = gzopen(TargetPath->c_str(), Type);
			if (!IoStream)
				return std::make_error_condition(std::errc::no_such_file_or_directory);

			OpenVirtual(std::move(*TargetPath));
			return Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> GzStream::Close()
		{
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			CloseVirtual();

			if (!IoStream)
				return Expectation::Met;

			VI_DEBUG("[gz] close 0x%" PRIXPTR, (uintptr_t)IoStream);
			if (gzclose((gzFile)IoStream) != Z_OK)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			IoStream = nullptr;
			return Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> GzStream::Seek(FileSeek Mode, int64_t Offset)
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			switch (Mode)
			{
				case FileSeek::Begin:
					VI_TRACE("[gz] seek fd %i begin %" PRId64, GetReadableFd(), Offset);
					if (gzseek((gzFile)IoStream, (long)Offset, SEEK_SET) == -1)
						return OS::Error::GetConditionOr();
					return Expectation::Met;
				case FileSeek::Current:
					VI_TRACE("[gz] seek fd %i move %" PRId64, GetReadableFd(), Offset);
					if (gzseek((gzFile)IoStream, (long)Offset, SEEK_CUR) == -1)
						return OS::Error::GetConditionOr();
					return Expectation::Met;
				case FileSeek::End:
					return std::make_error_condition(std::errc::not_supported);
				default:
					return std::make_error_condition(std::errc::invalid_argument);
			}
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> GzStream::Flush()
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			if (gzflush((gzFile)IoStream, Z_SYNC_FLUSH) != Z_OK)
				return OS::Error::GetConditionOr();
			return Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<size_t> GzStream::ReadScan(const char* Format, ...)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<size_t> GzStream::ReadLine(char* Data, size_t Length)
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[gz] fd %i readln %i bytes", GetReadableFd(), (int)Length);
			return gzgets((gzFile)IoStream, Data, (int)Length) ? strnlen(Data, Length) : 0;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<size_t> GzStream::Read(uint8_t* Data, size_t Length)
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[gz] fd %i read %i bytes", GetReadableFd(), (int)Length);
			int Value = gzread((gzFile)IoStream, Data, (uint32_t)Length);
			if (Value >= 0)
				return (size_t)Value;

			return std::make_error_condition(std::errc::broken_pipe);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<size_t> GzStream::WriteFormat(const char* Format, ...)
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_ASSERT(Format != nullptr, "format should be set");
			VI_MEASURE(Timings::FileSystem);
#ifdef VI_ZLIB
			va_list Args;
			va_start(Args, Format);
			int Value = gzvprintf((gzFile)IoStream, Format, Args);
			VI_TRACE("[gz] fd %i write %i bytes", GetWriteableFd(), Value);
			va_end(Args);
			if (Value >= 0)
				return (size_t)Value;

			return std::make_error_condition(std::errc::broken_pipe);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<size_t> GzStream::Write(const uint8_t* Data, size_t Length)
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[gz] fd %i write %i bytes", GetWriteableFd(), (int)Length);
			int Value = gzwrite((gzFile)IoStream, Data, (uint32_t)Length);
			if (Value >= 0)
				return (size_t)Value;

			return std::make_error_condition(std::errc::broken_pipe);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<size_t> GzStream::Tell()
		{
			VI_ASSERT(IoStream != nullptr, "file should be opened");
#ifdef VI_ZLIB
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[gz] fd %i tell", GetReadableFd());
			long Value = gztell((gzFile)IoStream);
			if (Value >= 0)
				return (size_t)Value;

			return std::make_error_condition(std::errc::broken_pipe);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		socket_t GzStream::GetReadableFd() const
		{
			return (socket_t)(uintptr_t)IoStream;
		}
		socket_t GzStream::GetWriteableFd() const
		{
			return (socket_t)(uintptr_t)IoStream;
		}
		void* GzStream::GetReadable() const
		{
			return IoStream;
		}
		void* GzStream::GetWriteable() const
		{
			return IoStream;
		}
		bool GzStream::IsSized() const
		{
			return false;
		}

		WebStream::WebStream(bool IsAsync) noexcept : OutputStream(nullptr), Offset(0), Length(0), Async(IsAsync)
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
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<void> WebStream::Open(const std::string_view& File, FileMode Mode)
		{
			VI_ASSERT(!File.empty(), "filename should be set");
			auto Result = Close();
			if (!Result)
				return Result;

			Network::Location Origin(File);
			if (Origin.Protocol != "http" && Origin.Protocol != "https")
				return std::make_error_condition(std::errc::address_family_not_supported);

			bool Secure = (Origin.Protocol == "https");
			if (Secure && !OS::Control::Has(AccessOption::Https))
				return std::make_error_condition(std::errc::permission_denied);
			else if (!Secure && !OS::Control::Has(AccessOption::Http))
				return std::make_error_condition(std::errc::permission_denied);

			auto* DNS = Network::DNS::Get();
			auto Address = DNS->Lookup(Origin.Hostname, Origin.Port > 0 ? ToString(Origin.Port) : (Secure ? "443" : "80"), Network::DNSType::Connect);
			if (!Address)
				return Address.Error().error();

			Core::UPtr<Network::HTTP::Client> Client = new Network::HTTP::Client(30000);
			auto Status = Client->ConnectSync(*Address, Secure ? Network::PEER_VERITY_DEFAULT : Network::PEER_NOT_SECURE).Get();
			if (!Status)
				return Status.Error().error();

			Network::HTTP::RequestFrame Request;
			Request.Location.assign(Origin.Path);
			VI_DEBUG("[ws] open ro %.*s", (int)File.size(), File.data());

			for (auto& Item : Origin.Query)
				Request.Query += Item.first + "=" + Item.second + "&";

			if (!Request.Query.empty())
				Request.Query.pop_back();

			for (auto& Item : Headers)
				Request.SetHeader(Item.first, Item.second);

			auto* Response = Client->GetResponse();
			Status = Client->Send(std::move(Request)).Get();
			if (!Status || Response->StatusCode < 0)
				return Status ? std::make_error_condition(std::errc::protocol_error) : Status.Error().error();
			else if (Response->Content.Limited)
				Length = Response->Content.Length;

			OutputStream = Client.Reset();
			OpenVirtual(String(File));
			return Expectation::Met;
		}
		ExpectsIO<void> WebStream::Close()
		{
			auto* Client = (Network::HTTP::Client*)OutputStream;
			VI_DEBUG("[ws] close 0x%" PRIXPTR, (uintptr_t)Client);
			OutputStream = nullptr;
			Offset = Length = 0;
			Chunk.clear();
			CloseVirtual();

			if (!Client)
				return Expectation::Met;

			auto Status = Client->Disconnect().Get();
			Memory::Release(Client);
			if (!Status)
				return Status.Error().error();

			return Expectation::Met;
		}
		ExpectsIO<void> WebStream::Seek(FileSeek Mode, int64_t NewOffset)
		{
			switch (Mode)
			{
				case FileSeek::Begin:
					VI_TRACE("[ws] seek fd %i begin %" PRId64, GetReadableFd(), (int)NewOffset);
					Offset = NewOffset;
					return Expectation::Met;
				case FileSeek::Current:
					VI_TRACE("[ws] seek fd %i move %" PRId64, GetReadableFd(), (int)NewOffset);
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
					return Expectation::Met;
				case FileSeek::End:
					VI_TRACE("[ws] seek fd %i end %" PRId64, GetReadableFd(), (int)NewOffset);
					Offset = Length - NewOffset;
					return Expectation::Met;
				default:
					return std::make_error_condition(std::errc::not_supported);
			}
		}
		ExpectsIO<void> WebStream::Flush()
		{
			return Expectation::Met;
		}
		ExpectsIO<size_t> WebStream::ReadScan(const char* Format, ...)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<size_t> WebStream::ReadLine(char* Data, size_t DataLength)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<size_t> WebStream::Read(uint8_t* Data, size_t DataLength)
		{
			VI_ASSERT(OutputStream != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_ASSERT(DataLength > 0, "length should be greater than zero");
			VI_TRACE("[ws] fd %i read %i bytes", GetReadableFd(), (int)DataLength);

			size_t Result = 0;
			if (Offset + DataLength > Chunk.size() && (Chunk.size() < Length || (!Length && !((Network::HTTP::Client*)OutputStream)->GetResponse()->Content.Limited)))
			{
				auto* Client = (Network::HTTP::Client*)OutputStream;
				auto Status = Client->Fetch(DataLength).Get();
				if (!Status)
					return Status.Error().error();

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
		ExpectsIO<size_t> WebStream::WriteFormat(const char* Format, ...)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<size_t> WebStream::Write(const uint8_t* Data, size_t Length)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<size_t> WebStream::Tell()
		{
			VI_TRACE("[ws] fd %i tell", GetReadableFd());
			return Offset;
		}
		socket_t WebStream::GetReadableFd() const
		{
			VI_ASSERT(OutputStream != nullptr, "file should be opened");
			return ((Network::HTTP::Client*)OutputStream)->GetStream()->GetFd();
		}
		socket_t WebStream::GetWriteableFd() const
		{
#ifndef INVALID_SOCKET
			return (socket_t)-1;
#else
			return INVALID_SOCKET;
#endif
		}
		void* WebStream::GetReadable() const
		{
			return OutputStream;
		}
		void* WebStream::GetWriteable() const
		{
			return nullptr;
		}
		bool WebStream::IsSized() const
		{
			return true;
		}

		ProcessStream::ProcessStream() noexcept : OutputStream(nullptr), InputFd(-1), ExitCode(-1)
		{
		}
		ProcessStream::~ProcessStream() noexcept
		{
			Close();
		}
		ExpectsIO<void> ProcessStream::Clear()
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<void> ProcessStream::Open(const std::string_view& File, FileMode Mode)
		{
			VI_ASSERT(!File.empty(), "command should be set");
			auto Result = Close();
			if (!Result)
				return Result;
			else if (!OS::Control::Has(AccessOption::Shell))
				return std::make_error_condition(std::errc::permission_denied);

			bool Readable = false;
			bool Writeable = false;
			switch (Mode)
			{
				case FileMode::Read_Only:
				case FileMode::Binary_Read_Only:
					Readable = true;
					break;
				case FileMode::Write_Only:
				case FileMode::Binary_Write_Only:
				case FileMode::Append_Only:
				case FileMode::Binary_Append_Only:
					Writeable = true;
					break;
				case FileMode::Read_Write:
				case FileMode::Binary_Read_Write:
				case FileMode::Write_Read:
				case FileMode::Binary_Write_Read:
				case FileMode::Read_Append_Write:
				case FileMode::Binary_Read_Append_Write:
					Readable = true;
					Writeable = true;
					break;
				default:
					break;
			}

			VI_PANIC(Readable || Writeable, "file open cannot be issued with mode:%i", (int)Mode);
			Internal.ErrorExitCode = Compute::Crypto::Random() % std::numeric_limits<int>::max();
			auto Shell = OS::Process::GetShell();
			if (!Shell)
				return Shell.Error();
#ifdef VI_MICROSOFT
			SECURITY_ATTRIBUTES PipePolicy = { 0 };
			PipePolicy.nLength = sizeof(SECURITY_ATTRIBUTES);
			PipePolicy.lpSecurityDescriptor = nullptr;
			PipePolicy.bInheritHandle = TRUE;

			HANDLE OutputReadable = nullptr, OutputWriteable = nullptr;
			if (Readable)
			{
				if (CreatePipe(&OutputReadable, &OutputWriteable, &PipePolicy, 0) == FALSE)
				{
				OutputError:
					auto Condition = OS::Error::GetConditionOr();
					if (OutputReadable != nullptr && OutputReadable != INVALID_HANDLE_VALUE)
						CloseHandle(OutputReadable);
					if (OutputWriteable != nullptr && OutputWriteable != INVALID_HANDLE_VALUE)
						CloseHandle(OutputWriteable);
					OutputStream = nullptr;
					return Condition;
				}

				OutputStream = _fdopen(_open_osfhandle((intptr_t)OutputReadable, _O_RDONLY | _O_BINARY), "rb");
				if (!OutputStream)
					goto OutputError;

				VI_DEBUG("[sh] open ro:pipe fd %i", VI_FILENO(OutputStream));
			}

			HANDLE InputReadable = nullptr, InputWriteable = nullptr;
			if (Writeable)
			{
				if (CreatePipe(&InputReadable, &InputWriteable, &PipePolicy, 0) == FALSE)
				{
				InputError:
					auto Condition = OS::Error::GetConditionOr();
					if (OutputReadable != nullptr && OutputReadable != INVALID_HANDLE_VALUE)
						CloseHandle(OutputReadable);
					if (OutputWriteable != nullptr && OutputWriteable != INVALID_HANDLE_VALUE)
						CloseHandle(OutputWriteable);
					if (InputReadable != nullptr && InputReadable != INVALID_HANDLE_VALUE)
						CloseHandle(InputReadable);
					if (InputWriteable != nullptr && InputWriteable != INVALID_HANDLE_VALUE)
						CloseHandle(InputWriteable);
					OutputStream = nullptr;
					InputFd = -1;
					return Condition;
				}

				InputFd = _open_osfhandle((intptr_t)InputWriteable, _O_WRONLY | _O_BINARY);
				if (InputFd < 0)
					goto InputError;

				VI_DEBUG("[sh] open wo:pipe fd %i", InputFd);
			}

			STARTUPINFO StartupPolicy;
			ZeroMemory(&StartupPolicy, sizeof(STARTUPINFO));
			StartupPolicy.cb = sizeof(STARTUPINFO);
			StartupPolicy.hStdError = OutputWriteable;
			StartupPolicy.hStdOutput = OutputWriteable;
			StartupPolicy.hStdInput = InputReadable;
			StartupPolicy.dwFlags |= STARTF_USESTDHANDLES;

			String Executable = "/c ";
			Executable += File;

			PROCESS_INFORMATION ProcessInfo = { 0 };
			if (CreateProcessA(Shell->c_str(), (LPSTR)Executable.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &StartupPolicy, &ProcessInfo) == FALSE)
				goto InputError;

			VI_DEBUG("[sh] spawn piped process pid %i (tid = %i)", (int)ProcessInfo.dwProcessId, (int)ProcessInfo.dwThreadId);
			Internal.OutputPipe = OutputReadable;
			Internal.InputPipe = InputWriteable;
			Internal.Process = ProcessInfo.hProcess;
			Internal.Thread = ProcessInfo.hThread;
			Internal.ProcessId = (socket_t)ProcessInfo.dwProcessId;
			Internal.ThreadId = (socket_t)ProcessInfo.dwThreadId;
			if (OutputWriteable != nullptr && OutputWriteable != INVALID_HANDLE_VALUE)
				CloseHandle(OutputWriteable);
			if (InputReadable != nullptr && InputReadable != INVALID_HANDLE_VALUE)
				CloseHandle(InputReadable);
#else
			int OutputPipe[2] = { 0, 0 };
			if (Readable)
			{
				if (pipe(OutputPipe) != 0)
				{
				OutputError:
					auto Condition = OS::Error::GetConditionOr();
					if (OutputPipe[0] > 0)
						close(OutputPipe[0]);
					if (OutputPipe[1] > 0)
						close(OutputPipe[1]);
					if (OutputStream != nullptr)
						fclose(OutputStream);
					return Condition;
				}

				OutputStream = fdopen(OutputPipe[0], "r");
				if (!OutputStream)
					goto OutputError;

				VI_DEBUG("[sh] open readable ro:pipe fd %i", VI_FILENO(OutputStream));
			}

			int InputPipe[2] = { 0, 0 };
			if (Writeable)
			{
				if (pipe(InputPipe) != 0)
				{
				InputError:
					auto Condition = OS::Error::GetConditionOr();
					if (OutputPipe[0] > 0)
						close(OutputPipe[0]);
					if (OutputPipe[1] > 0)
						close(OutputPipe[1]);
					if (OutputStream != nullptr)
						fclose(OutputStream);
					if (InputPipe[0] > 0)
						close(InputPipe[0]);
					if (InputPipe[1] > 0)
						close(InputPipe[1]);
					return Condition;
				}

				InputFd = InputPipe[1];
				VI_DEBUG("[sh] open writeable wo:pipe fd %i", InputFd);
			}

			pid_t ProcessId = fork();
			if (ProcessId < 0)
				goto InputError;

			String Executable = String(File);
			int ErrorExitCode = Internal.ErrorExitCode;
			if (ProcessId == 0)
			{
				if (!Readable)
					OutputPipe[0] = open("/dev/null", O_WRONLY | O_CREAT, 0666);
				else
					close(OutputPipe[1]);

				if (!Writeable)
					InputPipe[1] = open("/dev/null", O_RDONLY | O_CREAT, 0666);
				else
					close(InputPipe[0]);

				dup2(OutputPipe[0], 0);
				dup2(InputPipe[1], 1);
				execl(Shell->c_str(), "sh", "-c", Executable.c_str(), NULL);
				exit(ErrorExitCode);
				return std::make_error_condition(std::errc::broken_pipe);
			}
			else
			{
				VI_DEBUG("[sh] spawn piped process pid %i", (int)ProcessId);
				Internal.OutputPipe = (void*)(uintptr_t)OutputPipe[1];
				Internal.InputPipe = (void*)(uintptr_t)InputPipe[0];
				Internal.Process = (void*)(uintptr_t)ProcessId;
				Internal.Thread = (void*)(uintptr_t)ProcessId;
				Internal.ProcessId = (socket_t)ProcessId;
				Internal.ThreadId = (socket_t)ProcessId;
				if (OutputPipe[0] > 0)
					close(OutputPipe[0]);
				if (InputPipe[1] > 0)
					close(InputPipe[1]);
			}
#endif
			OpenVirtual(String(File));
			return Expectation::Met;
		}
		ExpectsIO<void> ProcessStream::Close()
		{
			CloseVirtual();
#ifdef VI_MICROSOFT
			if (Internal.Thread != nullptr)
			{
				VI_TRACE("[sh] close thread tid %i (wait)", (int)Internal.ThreadId);
				WaitForSingleObject((HANDLE)Internal.Thread, INFINITE);
				CloseHandle((HANDLE)Internal.Thread);
			}
			if (Internal.Process != nullptr)
			{
				VI_DEBUG("[sh] close process pid %i (wait)", (int)Internal.ThreadId);
				WaitForSingleObject((HANDLE)Internal.Process, INFINITE);

				DWORD StatusCode = 0;
				if (GetExitCodeProcess((HANDLE)Internal.Process, &StatusCode) == TRUE)
					ExitCode = (int)StatusCode;

				CloseHandle((HANDLE)Internal.Process);
			}
#else
			if (Internal.Process != nullptr)
			{
				VI_DEBUG("[sh] close process pid %i (wait)", (int)Internal.ThreadId);
				waitpid((pid_t)(uintptr_t)Internal.Process, &ExitCode, 0);
			}
#endif
			if (OutputStream != nullptr)
			{
				OS::File::Close(OutputStream);
				OutputStream = nullptr;
			}
			if (InputFd > 0)
			{
				VI_DEBUG("[sh] close fd %i", InputFd);
				close(InputFd);
				InputFd = -1;
			}
			memset(&Internal, 0, sizeof(Internal));
			return Expectation::Met;
		}
		ExpectsIO<void> ProcessStream::Seek(FileSeek Mode, int64_t Offset)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<void> ProcessStream::Flush()
		{
			return Expectation::Met;
		}
		ExpectsIO<size_t> ProcessStream::ReadScan(const char* Format, ...)
		{
			VI_ASSERT(OutputStream != nullptr, "file should be opened");
			VI_ASSERT(Format != nullptr, "format should be set");
			VI_MEASURE(Timings::FileSystem);

			va_list Args;
			va_start(Args, Format);
			int Value = vfscanf(OutputStream, Format, Args);
			VI_TRACE("[sh] fd %i scan %i bytes", GetReadableFd(), Value);
			va_end(Args);
			if (Value >= 0)
				return (size_t)Value;

			return OS::Error::GetConditionOr(std::errc::broken_pipe);
		}
		ExpectsIO<size_t> ProcessStream::ReadLine(char* Data, size_t Length)
		{
			VI_ASSERT(OutputStream != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[sh] fd %i readln %i bytes", GetReadableFd(), (int)Length);
			return fgets(Data, (int)Length, OutputStream) ? strnlen(Data, Length) : 0;
		}
		ExpectsIO<size_t> ProcessStream::Read(uint8_t* Data, size_t Length)
		{
			VI_ASSERT(OutputStream != nullptr, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[sh] fd %i read %i bytes", GetReadableFd(), (int)Length);
			size_t Value = fread(Data, 1, Length, OutputStream);
			if (Value > 0 || feof(OutputStream) != 0)
				return (size_t)Value;

			return OS::Error::GetConditionOr(std::errc::broken_pipe);
		}
		ExpectsIO<size_t> ProcessStream::WriteFormat(const char* Format, ...)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		ExpectsIO<size_t> ProcessStream::Write(const uint8_t* Data, size_t Length)
		{
			VI_ASSERT(InputFd > 0, "file should be opened");
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[sh] fd %i write %i bytes", GetWriteableFd(), (int)Length);
			int Value = (int)write(InputFd, Data, Length);
			if (Value >= 0)
				return (size_t)Value;

			return OS::Error::GetConditionOr(std::errc::broken_pipe);
		}
		ExpectsIO<size_t> ProcessStream::Tell()
		{
			VI_ASSERT(OutputStream != nullptr, "file should be opened");
			VI_MEASURE(Timings::FileSystem);
			return OS::File::Tell64(OutputStream);
		}
		socket_t ProcessStream::GetProcessId() const
		{
			return Internal.ProcessId;
		}
		socket_t ProcessStream::GetThreadId() const
		{
			return Internal.ThreadId;
		}
		socket_t ProcessStream::GetReadableFd() const
		{
			VI_ASSERT(OutputStream != nullptr, "file should be opened");
			return (socket_t)VI_FILENO(OutputStream);
		}
		socket_t ProcessStream::GetWriteableFd() const
		{
			return (socket_t)InputFd;
		}
		void* ProcessStream::GetReadable() const
		{
			return OutputStream;
		}
		void* ProcessStream::GetWriteable() const
		{
			return (void*)(uintptr_t)InputFd;
		}
		bool ProcessStream::IsSized() const
		{
			return false;
		}
		bool ProcessStream::IsAlive()
		{
			if (!Internal.Process)
				return false;
#ifdef VI_MICROSOFT
			return WaitForSingleObject((HANDLE)Internal.Process, 0) == WAIT_TIMEOUT;
#else
			return waitpid((pid_t)(uintptr_t)Internal.Process, &ExitCode, WNOHANG) == 0;
#endif
		}
		int ProcessStream::GetExitCode() const
		{
			return ExitCode;
		}

		FileTree::FileTree(const std::string_view& Folder) noexcept
		{
			auto Target = OS::Path::Resolve(Folder);
			if (!Target)
				return;

			Vector<std::pair<String, FileEntry>> Entries;
			if (!OS::Directory::Scan(Target->c_str(), Entries))
				return;
				
			Directories.reserve(Entries.size());
			Files.reserve(Entries.size());
			Path = *Target;
				
			for (auto& Item : Entries)
			{
				if (Item.second.IsDirectory)
					Directories.push_back(new FileTree(*Target + VI_SPLITTER + Item.first));
				else
					Files.emplace_back(std::move(Item.first));
			}
		}
		FileTree::~FileTree() noexcept
		{
			for (auto& Directory : Directories)
				Memory::Release(Directory);
		}
		void FileTree::Loop(const std::function<bool(const FileTree*)>& Callback) const
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Callback(this))
				return;

			for (auto& Directory : Directories)
				Directory->Loop(Callback);
		}
		const FileTree* FileTree::Find(const std::string_view& V) const
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

		InlineArgs::InlineArgs() noexcept
		{
		}
		bool InlineArgs::IsEnabled(const std::string_view& Option, const std::string_view& Shortcut) const
		{
			auto It = Args.find(KeyLookupCast(Option));
			if (It == Args.end() || !IsTrue(It->second))
				return Shortcut.empty() ? false : IsEnabled(Shortcut);

			return true;
		}
		bool InlineArgs::IsDisabled(const std::string_view& Option, const std::string_view& Shortcut) const
		{
			auto It = Args.find(KeyLookupCast(Option));
			if (It == Args.end())
				return Shortcut.empty() ? true : IsDisabled(Shortcut);

			return IsFalse(It->second);
		}
		bool InlineArgs::Has(const std::string_view& Option, const std::string_view& Shortcut) const
		{
			if (Args.find(KeyLookupCast(Option)) != Args.end())
				return true;

			return Shortcut.empty() ? false : Args.find(KeyLookupCast(Shortcut)) != Args.end();
		}
		String& InlineArgs::Get(const std::string_view& Option, const std::string_view& Shortcut)
		{
			auto It = Args.find(KeyLookupCast(Option));
			if (It != Args.end())
				return It->second;
			else if (Shortcut.empty())
				return Args[String(Option)];

			It = Args.find(KeyLookupCast(Shortcut));
			if (It != Args.end())
				return It->second;
			
			return Args[String(Shortcut)];
		}
		String& InlineArgs::GetIf(const std::string_view& Option, const std::string_view& Shortcut, const std::string_view& WhenEmpty)
		{
			auto It = Args.find(KeyLookupCast(Option));
			if (It != Args.end())
				return It->second;

			if (!Shortcut.empty())
			{
				It = Args.find(KeyLookupCast(Shortcut));
				if (It != Args.end())
					return It->second;
			}

			String& Value = Args[String(Option)];
			Value = WhenEmpty;
			return Value;
		}
		bool InlineArgs::IsTrue(const std::string_view& Value) const
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
		bool InlineArgs::IsFalse(const std::string_view& Value) const
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
					Result.Logical = (uint32_t)ThreadData.second;
			}

			const auto CtlCoreData = SysControl("machdep.cpu.core_count");
			if (!CtlCoreData.empty())
			{
				const auto CoreData = SysExtract(CtlCoreData);
				if (CoreData.first)
					Result.Physical = (uint32_t)CoreData.second;
			}

			const auto CtlPackagesData = SysControl("hw.packages");
			if (!CtlPackagesData.empty())
			{
				const auto PackagesData = SysExtract(CtlPackagesData);
				if (PackagesData.first)
					Result.Packages = (uint32_t)PackagesData.second;
			}
#else
			Result.Logical = sysconf(_SC_NPROCESSORS_ONLN);
			std::ifstream Info("/proc/cpuinfo");

			if (!Info.is_open() || !Info)
				return Result;

			Vector<uint32_t> Packages;
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
		OS::CPU::CacheInfo OS::CPU::GetCacheInfo(uint32_t Level)
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
				uint32_t Temp;
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

		bool OS::Directory::IsExists(const std::string_view& Path)
		{
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] check path %.*s", (int)Path.size(), Path.data());
			if (!Control::Has(AccessOption::Fs))
				return false;

			auto TargetPath = Path::Resolve(Path);
			if (!TargetPath)
				return false;

			struct stat Buffer;
			if (stat(TargetPath->c_str(), &Buffer) != 0)
				return false;

			return Buffer.st_mode & S_IFDIR;
		}
		bool OS::Directory::IsEmpty(const std::string_view& Path)
		{
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] check dir %.*s", (int)Path.size(), Path.data());
			if (Path.empty() || !Control::Has(AccessOption::Fs))
				return true;
#if defined(VI_MICROSOFT)
			wchar_t Buffer[CHUNK_SIZE];
			Stringify::ConvertToWide(Path, Buffer, CHUNK_SIZE);

			DWORD Attributes = GetFileAttributesW(Buffer);
			if (Attributes == 0xFFFFFFFF || (Attributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
				return true;

			if (Path[0] != '\\' && Path[0] != '/')
				wcscat(Buffer, L"\\*");
			else
				wcscat(Buffer, L"*");

			WIN32_FIND_DATAW Info;
			HANDLE Handle = FindFirstFileW(Buffer, &Info);
			if (!Handle)
				return true;

			while (FindNextFileW(Handle, &Info) == TRUE)
			{
				if (wcscmp(Info.cFileName, L".") != 0 && wcscmp(Info.cFileName, L"..") != 0)
				{
					FindClose(Handle);
					return false;
				}
			}

			FindClose(Handle);
			return true;
#else
			DIR* Handle = opendir(Path.data());
			if (!Handle)
				return true;

			dirent* Next = nullptr;
			while ((Next = readdir(Handle)) != nullptr)
			{
				if (strcmp(Next->d_name, ".") != 0 && strcmp(Next->d_name, "..") != 0)
				{
					closedir(Handle);
					return false;
				}
			}

			closedir(Handle);
			return true;
#endif
		}
		ExpectsIO<void> OS::Directory::SetWorking(const std::string_view& Path)
		{
			VI_TRACE("[io] apply working dir %.*s", (int)Path.size(), Path.data());
#ifdef VI_MICROSOFT
			if (SetCurrentDirectoryA(Path.data()) != TRUE)
				return OS::Error::GetConditionOr();

			return Expectation::Met;
#elif defined(VI_LINUX)
			if (chdir(Path.data()) != 0)
				return OS::Error::GetConditionOr();

			return Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> OS::Directory::Patch(const std::string_view& Path)
		{
			if (IsExists(Path.data()))
				return Expectation::Met;

			return Create(Path.data());
		}
		ExpectsIO<void> OS::Directory::Scan(const std::string_view& Path, Vector<std::pair<String, FileEntry>>& Entries)
		{
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] scan dir %.*s", (int)Path.size(), Path.data());
			if (!Control::Has(AccessOption::Fs))
				return std::make_error_condition(std::errc::permission_denied);

			if (Path.empty())
				return std::make_error_condition(std::errc::no_such_file_or_directory);
#if defined(VI_MICROSOFT)
			wchar_t Buffer[CHUNK_SIZE];
			Stringify::ConvertToWide(Path, Buffer, CHUNK_SIZE);

			DWORD Attributes = GetFileAttributesW(Buffer);
			if (Attributes == 0xFFFFFFFF || (Attributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
				return std::make_error_condition(std::errc::not_a_directory);

			if (Path.back() != '\\' && Path.back() != '/')
				wcscat(Buffer, L"\\*");
			else
				wcscat(Buffer, L"*");

			WIN32_FIND_DATAW Info;
			HANDLE Handle = FindFirstFileW(Buffer, &Info);
			if (!Handle)
				return std::make_error_condition(std::errc::no_such_file_or_directory);

			do
			{
				char Directory[CHUNK_SIZE] = { 0 };
				WideCharToMultiByte(CP_UTF8, 0, Info.cFileName, -1, Directory, sizeof(Directory), nullptr, nullptr);
				if (strcmp(Directory, ".") != 0 && strcmp(Directory, "..") != 0)
				{
					auto State = File::GetState(String(Path) + '/' + Directory);
					if (State)
						Entries.push_back(std::make_pair<String, FileEntry>(Directory, std::move(*State)));
				}
			} while (FindNextFileW(Handle, &Info) == TRUE);
			FindClose(Handle);
#else
			DIR* Handle = opendir(Path.data());
			if (!Handle)
				return OS::Error::GetConditionOr();

			dirent* Next = nullptr;
			while ((Next = readdir(Handle)) != nullptr)
			{
				if (strcmp(Next->d_name, ".") != 0 && strcmp(Next->d_name, "..") != 0)
				{
					auto State = File::GetState(String(Path) + '/' + Next->d_name);
					if (State)
						Entries.push_back(std::make_pair<String, FileEntry>(Next->d_name, std::move(*State)));
				}
			}
			closedir(Handle);
#endif
			return Expectation::Met;
		}
		ExpectsIO<void> OS::Directory::Create(const std::string_view& Path)
		{
			VI_MEASURE(Timings::FileSystem);
			VI_DEBUG("[io] create dir %.*s", (int)Path.size(), Path.data());
			if (!Control::Has(AccessOption::Fs))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			wchar_t Buffer[CHUNK_SIZE];
			Stringify::ConvertToWide(Path, Buffer, CHUNK_SIZE);

			size_t Length = wcslen(Buffer);
			if (!Length)
				return std::make_error_condition(std::errc::invalid_argument);

			if (::CreateDirectoryW(Buffer, nullptr) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS)
				return Expectation::Met;

			size_t Index = Length - 1;
			while (Index > 0 && (Buffer[Index] == '/' || Buffer[Index] == '\\'))
				Index--;

			while (Index > 0 && Buffer[Index] != '/' && Buffer[Index] != '\\')
				Index--;

			String Subpath(Path.data(), Index);
			if (Index > 0 && !Create(Subpath.c_str()))
				return OS::Error::GetConditionOr();

			if (::CreateDirectoryW(Buffer, nullptr) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS)
				return Expectation::Met;
#else
			if (mkdir(Path.data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST)
				return Expectation::Met;

			size_t Index = Path.empty() ? 0 : Path.size() - 1;
			while (Index > 0 && Path[Index] != '/' && Path[Index] != '\\')
				Index--;

			String Subpath(Path.data(), Index);
			if (Index > 0 && !Create(Subpath.c_str()))
				return OS::Error::GetConditionOr();

			if (mkdir(Path.data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST)
				return Expectation::Met;
#endif
			return OS::Error::GetConditionOr();
		}
		ExpectsIO<void> OS::Directory::Remove(const std::string_view& Path)
		{
			VI_MEASURE(Timings::FileSystem);
			VI_DEBUG("[io] remove dir %.*s", (int)Path.size(), Path.data());
			if (!Control::Has(AccessOption::Fs))
				return std::make_error_condition(std::errc::permission_denied);

			String TargetPath = String(Path);
			if (!TargetPath.empty() && (TargetPath.back() == '/' || TargetPath.back() == '\\'))
				TargetPath.pop_back();
#ifdef VI_MICROSOFT
			WIN32_FIND_DATA FileInformation;
			String FilePath, Pattern = TargetPath + "\\*.*";
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

				FilePath = TargetPath + "\\" + FileInformation.cFileName;
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

			if (::SetFileAttributes(TargetPath.data(), FILE_ATTRIBUTE_NORMAL) == FALSE || ::RemoveDirectory(TargetPath.data()) == FALSE)
				return OS::Error::GetConditionOr();
#elif defined VI_LINUX
			DIR* Handle = opendir(TargetPath.c_str());
			size_t Size = TargetPath.size();

			if (!Handle)
			{
				if (rmdir(TargetPath.c_str()) != 0)
					return OS::Error::GetConditionOr();

				return Expectation::Met;
			}

			struct dirent* It;
			while ((It = readdir(Handle)))
			{
				if (!strcmp(It->d_name, ".") || !strcmp(It->d_name, ".."))
					continue;

				size_t Length = Size + strlen(It->d_name) + 2;
				char* Buffer = Memory::Allocate<char>(Length);
				snprintf(Buffer, Length, "%.*s/%s", TargetPath.c_str(), It->d_name);

				struct stat State;
				if (stat(Buffer, &State) != 0)
				{
					auto Condition = OS::Error::GetConditionOr();
					Memory::Deallocate(Buffer);
					closedir(Handle);
					return Condition;
				}

				if (S_ISDIR(State.st_mode))
				{
					auto Result = Remove(Buffer);
					Memory::Deallocate(Buffer);
					if (Result)
						continue;

					closedir(Handle);
					return Result;
				}

				if (unlink(Buffer) != 0)
				{
					auto Condition = OS::Error::GetConditionOr();
					Memory::Deallocate(Buffer);
					closedir(Handle);
					return Condition;
				}

				Memory::Deallocate(Buffer);
			}

			closedir(Handle);
			if (rmdir(TargetPath.c_str()) != 0)
				return OS::Error::GetConditionOr();
#endif
			return Expectation::Met;
		}
		ExpectsIO<String> OS::Directory::GetModule()
		{
			VI_MEASURE(Timings::FileSystem);
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

		bool OS::File::IsExists(const std::string_view& Path)
		{
			VI_MEASURE(Timings::FileSystem);
			VI_TRACE("[io] check path %.*s", (int)Path.size(), Path.data());
			auto TargetPath = OS::Path::Resolve(Path);
			return TargetPath && IsPathExists(TargetPath->c_str());
		}
		int OS::File::Compare(const std::string_view& FirstPath, const std::string_view& SecondPath)
		{
			VI_ASSERT(!FirstPath.empty(), "first path should not be empty");
			VI_ASSERT(!SecondPath.empty(), "second path should not be empty");

			auto Props1 = GetProperties(FirstPath);
			auto Props2 = GetProperties(SecondPath);
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
			VI_TRACE("[io] compare paths { %.*s (%" PRIu64 "), %.*s (%" PRIu64 ") }", (int)FirstPath.size(), FirstPath.data(), Size1, (int)SecondPath.size(), SecondPath.data(), Size2);

			if (Size1 > Size2)
				return 1;
			else if (Size1 < Size2)
				return -1;

			auto First = Open(FirstPath, "rb");
			if (!First)
				return -1;

			auto Second = Open(SecondPath, "rb");
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
		uint64_t OS::File::GetHash(const std::string_view& Data)
		{
			return Compute::Crypto::CRC32(Data);
		}
		uint64_t OS::File::GetIndex(const std::string_view& Data)
		{
			uint64_t Result = 0xcbf29ce484222325;
			for (size_t i = 0; i < Data.size(); i++)
			{
				Result ^= Data[i];
				Result *= 1099511628211;
			}

			return Result;
		}
		ExpectsIO<size_t> OS::File::Write(const std::string_view& Path, const uint8_t* Data, size_t Length)
		{
			VI_ASSERT(Data != nullptr, "data should be set");
			VI_MEASURE(Timings::FileSystem);
			auto Status = Open(Path, FileMode::Binary_Write_Only);
			if (!Status)
				return Status.Error();

			UPtr<Core::Stream> Stream = *Status;
			if (!Length)
				return 0;

			return Stream->Write(Data, Length);
		}
		ExpectsIO<void> OS::File::Copy(const std::string_view& From, const std::string_view& To)
		{
			VI_ASSERT(Stringify::IsCString(From) && Stringify::IsCString(To), "from and to should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_DEBUG("[io] copy file from %.*s to %.*s", (int)From.size(), From.data(), (int)To.size(), To.data());
			if (!Control::Has(AccessOption::Fs))
				return std::make_error_condition(std::errc::permission_denied);

			std::ifstream Source(std::string(From), std::ios::binary);
			if (!Source)
				return std::make_error_condition(std::errc::bad_file_descriptor);
			
			auto Result = OS::Directory::Patch(OS::Path::GetDirectory(To));
			if (!Result)
				return Result;

			std::ofstream Destination(std::string(To), std::ios::binary);
			if (!Source)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			Destination << Source.rdbuf();
			return Expectation::Met;
		}
		ExpectsIO<void> OS::File::Move(const std::string_view& From, const std::string_view& To)
		{
			VI_ASSERT(Stringify::IsCString(From) && Stringify::IsCString(To), "from and to should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_DEBUG("[io] move file from %.*s to %.*s", (int)From.size(), From.data(), (int)To.size(), To.data());
			if (!Control::Has(AccessOption::Fs))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			if (MoveFileA(From.data(), To.data()) != TRUE)
				return OS::Error::GetConditionOr();

			return Expectation::Met;
#elif defined VI_LINUX
			if (rename(From.data(), To.data()) != 0)
				return OS::Error::GetConditionOr();

			return Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> OS::File::Remove(const std::string_view& Path)
		{
			VI_ASSERT(Stringify::IsCString(Path), "path should be set");
			VI_MEASURE(Timings::FileSystem);
			VI_DEBUG("[io] remove file %.*s", (int)Path.size(), Path.data());
			if (!Control::Has(AccessOption::Fs))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			SetFileAttributesA(Path.data(), 0);
			if (DeleteFileA(Path.data()) != TRUE)
				return OS::Error::GetConditionOr();

			return Expectation::Met;
#elif defined VI_LINUX
			if (unlink(Path.data()) != 0)
				return OS::Error::GetConditionOr();

			return Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		ExpectsIO<void> OS::File::Close(FILE* Stream)
		{
			VI_ASSERT(Stream != nullptr, "stream should be set");
			VI_DEBUG("[io] close fd %i", VI_FILENO(Stream));
			if (fclose(Stream) != 0)
				return OS::Error::GetConditionOr();

			return Expectation::Met;
		}
		ExpectsIO<void> OS::File::GetState(const std::string_view& Path, FileEntry* File)
		{
			VI_ASSERT(Stringify::IsCString(Path), "path should be set");
			VI_MEASURE(Timings::FileSystem);
			if (!Control::Has(AccessOption::Fs))
				return std::make_error_condition(std::errc::permission_denied);
#if defined(VI_MICROSOFT)
			wchar_t Buffer[CHUNK_SIZE];
			Stringify::ConvertToWide(Path, Buffer, CHUNK_SIZE);

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
			if (stat(Path.data(), &State) != 0)
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
			VI_TRACE("[io] stat %.*s: %s %" PRIu64 " bytes", (int)Path.size(), Path.data(), File->IsDirectory ? "dir" : "file", (uint64_t)File->Size);
			return Core::Expectation::Met;
		}
		ExpectsIO<void> OS::File::Seek64(FILE* Stream, int64_t Offset, FileSeek Mode)
		{
			int Origin;
			switch (Mode)
			{
				case FileSeek::Begin:
					VI_TRACE("[io] seek-64 fd %i begin %" PRId64, VI_FILENO(Stream), Offset);
					Origin = SEEK_SET;
					break;
				case FileSeek::Current:
					VI_TRACE("[io] seek-64 fd %i move %" PRId64, VI_FILENO(Stream), Offset);
					Origin = SEEK_CUR;
					break;
				case FileSeek::End:
					VI_TRACE("[io] seek-64 fd %i end %" PRId64, VI_FILENO(Stream), Offset);
					Origin = SEEK_END;
					break;
				default:
					return std::make_error_condition(std::errc::invalid_argument);
			}
#ifdef VI_MICROSOFT
			if (_fseeki64(Stream, Offset, Origin) != 0)
				return OS::Error::GetConditionOr();
#else
			if (fseeko(Stream, Offset, Origin) != 0)
				return OS::Error::GetConditionOr();
#endif
			return Expectation::Met;
		}
		ExpectsIO<size_t> OS::File::Tell64(FILE* Stream)
		{
			VI_TRACE("[io] fd %i tell-64", VI_FILENO(Stream));
#ifdef VI_MICROSOFT
			int64_t Offset = _ftelli64(Stream);
#else
			int64_t Offset = ftello(Stream);
#endif
			if (Offset < 0)
				return OS::Error::GetConditionOr();

			return (size_t)Offset;
		}
		ExpectsIO<size_t> OS::File::Join(const std::string_view& To, const Vector<String>& Paths)
		{
			VI_ASSERT(!To.empty(), "to should not be empty");
			VI_ASSERT(!Paths.empty(), "paths to join should not be empty");
			VI_TRACE("[io] join %i path to %.*s", (int)Paths.size(), (int)To.size(), To.data());
			auto Target = Open(To, FileMode::Binary_Write_Only);
			if (!Target)
				return Target.Error();

			size_t Total = 0;
			UPtr<Stream> Pipe = *Target;
			for (auto& Path : Paths)
			{
				UPtr<Stream> Base = Open(Path, FileMode::Binary_Read_Only).Or(nullptr);
				if (Base)
					Total += Base->ReadAll([&Pipe](uint8_t* Buffer, size_t Size) { Pipe->Write(Buffer, Size); }).Or(0);
			}

			return Total;
		}
		ExpectsIO<FileState> OS::File::GetProperties(const std::string_view& Path)
		{
			VI_ASSERT(Stringify::IsCString(Path), "path should be set");
			VI_MEASURE(Timings::FileSystem);
			if (!Control::Has(AccessOption::Fs))
				return std::make_error_condition(std::errc::permission_denied);

			struct stat Buffer;
			if (stat(Path.data(), &Buffer) != 0)
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

			VI_TRACE("[io] stat %.*s: %" PRIu64 " bytes", (int)Path.size(), Path.data(), (uint64_t)State.Size);
			return State;
		}
		ExpectsIO<FileEntry> OS::File::GetState(const std::string_view& Path)
		{
			FileEntry File;
			auto Status = GetState(Path, &File);
			if (!Status)
				return Status.Error();

			return File;
		}
		ExpectsIO<FILE*> OS::File::Open(const std::string_view& Path, const std::string_view& Mode)
		{
			VI_MEASURE(Timings::FileSystem);
			VI_ASSERT(Stringify::IsCString(Path), "path should be set");
			VI_ASSERT(Stringify::IsCString(Mode), "mode should be set");
			if (!Control::Has(AccessOption::Fs))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			wchar_t Buffer[CHUNK_SIZE], Type[20];
			Stringify::ConvertToWide(Path, Buffer, CHUNK_SIZE);
			Stringify::ConvertToWide(Mode, Type, 20);

			FILE* Stream = _wfopen(Buffer, Type);
			if (!Stream)
				return OS::Error::GetConditionOr();

			VI_DEBUG("[io] open %.*s:file fd %i on %.*s", (int)Mode.size(), Mode.data(), VI_FILENO(Stream), (int)Path.size(), Path.data());
			return Stream;
#else
			FILE* Stream = fopen(Path.data(), Mode.data());
			if (!Stream)
				return OS::Error::GetConditionOr();

			VI_DEBUG("[io] open %.*s:file fd %i on %.*s", (int)Mode.size(), Mode.data(), VI_FILENO(Stream), (int)Path.size(), Path.data());
			fcntl(VI_FILENO(Stream), F_SETFD, FD_CLOEXEC);
			return Stream;
#endif
		}
		ExpectsIO<Stream*> OS::File::Open(const std::string_view& Path, FileMode Mode, bool Async)
		{
			if (Path.empty())
				return std::make_error_condition(std::errc::no_such_file_or_directory);
			else if (!Control::Has(AccessOption::Fs))
				return std::make_error_condition(std::errc::permission_denied);

			Network::Location Origin(Path);
			if (Origin.Protocol == "file")
			{
				UPtr<Stream> Target;
				if (Stringify::EndsWith(Path, ".gz"))
					Target = new GzStream();
				else
					Target = new FileStream();

				auto Result = Target->Open(Path, Mode);
				if (!Result)
					return Result.Error();

				return Target.Reset();
			}
			else if (Origin.Protocol == "http" || Origin.Protocol == "https")
			{
				UPtr<Stream> Target = new WebStream(Async);
				auto Result = Target->Open(Path, Mode);
				if (!Result)
					return Result.Error();

				return Target.Reset();
			}
			else if (Origin.Protocol == "shell")
			{
				UPtr<Stream> Target = new ProcessStream();
				auto Result = Target->Open(Origin.Path.c_str(), Mode);
				if (!Result)
					return Result.Error();

				return Target.Reset();
			}
			else if (Origin.Protocol == "mem")
			{
				UPtr<Stream> Target = new MemoryStream();
				auto Result = Target->Open(Path, Mode);
				if (!Result)
					return Result.Error();

				return Target.Reset();
			}

			return std::make_error_condition(std::errc::invalid_argument);
		}
		ExpectsIO<Stream*> OS::File::OpenArchive(const std::string_view& Path, size_t UnarchivedMaxSize)
		{
			auto State = OS::File::GetState(Path);
			if (!State)
				return Open(Path, FileMode::Binary_Write_Only);

			String Temp = Path::GetNonExistant(Path);
			Move(Path, Temp.c_str());

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

			UPtr<Stream> Archive = OpenJoin(Temp + ".gz", { Temp }).Or(nullptr);
			if (Archive)
				Remove(Temp.c_str());

			return Target;
		}
		ExpectsIO<Stream*> OS::File::OpenJoin(const std::string_view& To, const Vector<String>& Paths)
		{
			VI_ASSERT(!To.empty(), "to should not be empty");
			VI_ASSERT(!Paths.empty(), "paths to join should not be empty");
			VI_TRACE("[io] open join %i path to %.*s", (int)Paths.size(), (int)To.size(), To.data());
			auto Target = Open(To, FileMode::Binary_Write_Only);
			if (!Target)
				return Target;

			auto* Channel = *Target;
			for (auto& Path : Paths)
			{
				UPtr<Stream> Base = Open(Path, FileMode::Binary_Read_Only).Or(nullptr);
				if (Base)
					Base->ReadAll([&Channel](uint8_t* Buffer, size_t Size) { Channel->Write(Buffer, Size); });
			}

			return Target;
		}
		ExpectsIO<uint8_t*> OS::File::ReadAll(const std::string_view& Path, size_t* Length)
		{
			auto Target = Open(Path, FileMode::Binary_Read_Only);
			if (!Target)
				return Target.Error();

			UPtr<Stream> Base = *Target;
			return ReadAll(*Base, Length);
		}
		ExpectsIO<uint8_t*> OS::File::ReadAll(Stream* Stream, size_t* Length)
		{
			VI_ASSERT(Stream != nullptr, "path should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[io] fd %i read-all", Stream->GetReadableFd());
			if (Length != nullptr)
				*Length = 0;

			bool IsVirtual = Stream->VirtualSize() > 0;
			if (IsVirtual || Stream->IsSized())
			{
				size_t Size = IsVirtual ? Stream->VirtualSize() : Stream->Size().Or(0);
				auto* Bytes = Memory::Allocate<uint8_t>(sizeof(uint8_t) * (Size + 1));
				if (Size > 0)
				{
					auto Status = Stream->Read(Bytes, Size);
					if (!Status)
					{
						Memory::Deallocate(Bytes);
						return Status.Error();
					}
				}

				Bytes[Size] = '\0';
				if (Length != nullptr)
					*Length = Size;

				return Bytes;
			}

			Core::String Data;
			auto Status = Stream->ReadAll([&Data](uint8_t* Buffer, size_t Length)
			{
				Data.reserve(Data.size() + Length);
				Data.append((char*)Buffer, Length);
			});
			if (!Status)
				return Status.Error();

			size_t Size = Data.size();
			auto* Bytes = Memory::Allocate<uint8_t>(sizeof(uint8_t) * (Data.size() + 1));
			memcpy(Bytes, Data.data(), sizeof(uint8_t) * Data.size());
			Bytes[Size] = '\0';
			if (Length != nullptr)
				*Length = Size;

			return Bytes;
		}
		ExpectsIO<uint8_t*> OS::File::ReadChunk(Stream* Stream, size_t Length)
		{
			VI_ASSERT(Stream != nullptr, "stream should be set");
			auto* Bytes = Memory::Allocate<uint8_t>(Length + 1);
			if (Length > 0)
				Stream->Read(Bytes, Length);
			Bytes[Length] = '\0';
			return Bytes;
		}
		ExpectsIO<String> OS::File::ReadAsString(const std::string_view& Path)
		{
			size_t Length = 0;
			auto FileData = ReadAll(Path, &Length);
			if (!FileData)
				return FileData.Error();

			auto* Data = (char*)*FileData;
			String Output(Data, Length);
			Memory::Deallocate(Data);
			return Output;
		}
		ExpectsIO<Vector<String>> OS::File::ReadAsArray(const std::string_view& Path)
		{
			ExpectsIO<String> Result = ReadAsString(Path);
			if (!Result)
				return Result.Error();

			return Stringify::Split(*Result, '\n');
		}
		
		bool OS::Path::IsRemote(const std::string_view& Path)
		{
			return Network::Location(Path).Protocol != "file";
		}
		bool OS::Path::IsRelative(const std::string_view& Path)
		{
#ifdef VI_MICROSOFT
			return !IsAbsolute(Path);
#else
			return Path[0] == '/' || Path[0] == '\\';
#endif
		}
		bool OS::Path::IsAbsolute(const std::string_view& Path)
		{
#ifdef VI_MICROSOFT
			if (Path[0] == '/' || Path[0] == '\\')
				return true;

			if (Path.size() < 2)
				return false;

			return Path[1] == ':' && Stringify::IsAlphanum(Path[0]);
#else
			return Path[0] == '/' || Path[0] == '\\';
#endif
		}
		ExpectsIO<String> OS::Path::Resolve(const std::string_view& Path)
		{
			VI_ASSERT(Stringify::IsCString(Path), "path should be set");
			VI_MEASURE(Timings::FileSystem);
			char Buffer[BLOB_SIZE] = { 0 };
#ifdef VI_MICROSOFT
			if (GetFullPathNameA(Path.data(), sizeof(Buffer), Buffer, nullptr) == 0)
				return OS::Error::GetConditionOr();
#else
			if (!realpath(Path.data(), Buffer))
			{
				if (!*Buffer && (Path.empty() || Path.find("./") != std::string::npos || Path.find(".\\") != std::string::npos))
					return OS::Error::GetConditionOr();
				
				String Output = *Buffer > 0 ? String(Buffer, strnlen(Buffer, BLOB_SIZE)) : String(Path);
				VI_TRACE("[io] resolve %.*s path (non-existant)", (int)Path.size(), Path.data());
				return Output;
			}
#endif
			String Output(Buffer, strnlen(Buffer, BLOB_SIZE));
			VI_TRACE("[io] resolve %.*s path: %s", (int)Path.size(), Path.data(), Output.c_str());
			return Output;
		}
		ExpectsIO<String> OS::Path::Resolve(const std::string_view& Path, const std::string_view& Directory, bool EvenIfExists)
		{
			VI_ASSERT(!Path.empty() && !Directory.empty(), "path and directory should not be empty");
			if (IsAbsolute(Path))
				return String(Path);
			else if (!EvenIfExists && IsPathExists(Path) && Path.find("..") == std::string::npos)
				return String(Path);

			bool Prefixed = Stringify::StartsOf(Path, "/\\");
			bool Relative = !Prefixed && (Stringify::StartsWith(Path, "./") || Stringify::StartsWith(Path, ".\\"));
			bool Postfixed = Stringify::EndsOf(Directory, "/\\");

			String Target = String(Directory);
			if (!Prefixed && !Postfixed)
				Target.append(1, VI_SPLITTER);

			if (Relative)
				Target.append(Path.data() + 2, Path.size() - 2);
			else
				Target.append(Path);

			return Resolve(Target.c_str());
		}
		ExpectsIO<String> OS::Path::ResolveDirectory(const std::string_view& Path)
		{
			ExpectsIO<String> Result = Resolve(Path);
			if (!Result)
				return Result;

			if (!Result->empty() && !Stringify::EndsOf(*Result, "/\\"))
				Result->append(1, VI_SPLITTER);

			return Result;
		}
		ExpectsIO<String> OS::Path::ResolveDirectory(const std::string_view& Path, const std::string_view& Directory, bool EvenIfExists)
		{
			ExpectsIO<String> Result = Resolve(Path, Directory, EvenIfExists);
			if (!Result)
				return Result;

			if (!Result->empty() && !Stringify::EndsOf(*Result, "/\\"))
				Result->append(1, VI_SPLITTER);

			return Result;
		}
		String OS::Path::GetNonExistant(const std::string_view& Path)
		{
			VI_ASSERT(!Path.empty(), "path should not be empty");
			auto Extension = GetExtension(Path);
			bool IsTrueFile = !Extension.empty();
			size_t ExtensionAt = IsTrueFile ? Path.rfind(Extension) : Path.size();
			if (ExtensionAt == String::npos)
				return String(Path);

			String First = String(Path.substr(0, ExtensionAt));
			String Second = String(1, '.') + String(Extension);
			String Filename = String(Path);
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
		String OS::Path::GetDirectory(const std::string_view& Path, size_t Level)
		{
			String Buffer(Path);
			TextSettle Result = Stringify::ReverseFindOf(Buffer, "/\\");
			if (!Result.Found)
				return Buffer;

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
				Buffer.assign("/");

			return Buffer;
		}
		std::string_view OS::Path::GetFilename(const std::string_view& Path)
		{
			size_t Size = Path.size();
			for (size_t i = Size; i-- > 0;)
			{
				if (Path[i] == '/' || Path[i] == '\\')
					return Path.substr(i + 1);
			}

			return Path;
		}
		std::string_view OS::Path::GetExtension(const std::string_view& Path)
		{
			if (Path.empty())
				return "";

			size_t Index = Path.rfind('.');
			if (Index == std::string::npos)
				return "";

			return Path.substr(Index + 1);
		}

		bool OS::Net::GetETag(char* Buffer, size_t Length, FileEntry* Resource)
		{
			VI_ASSERT(Resource != nullptr, "resource should be set");
			return GetETag(Buffer, Length, Resource->LastModified, Resource->Size);
		}
		bool OS::Net::GetETag(char* Buffer, size_t Length, int64_t LastModified, size_t ContentLength)
		{
			VI_ASSERT(Buffer != nullptr && Length > 0, "buffer should be set and size should be greater than zero");
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
			VI_DEBUG("[os] process suspend on thread %s", GetThreadId(std::this_thread::get_id()).c_str());
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
			int InvalidOffset = 0x2d42;
			switch (Type)
			{
				case Signal::SIG_INT:
#ifdef SIGINT
					return SIGINT;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_ILL:
#ifdef SIGILL
					return SIGILL;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_FPE:
#ifdef SIGFPE
					return SIGFPE;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_SEGV:
#ifdef SIGSEGV
					return SIGSEGV;
#else
					return InvalidOffset + (int)Type;
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
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_ABRT:
#ifdef SIGABRT
					return SIGABRT;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_BUS:
#ifdef SIGBUS
					return SIGBUS;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_ALRM:
#ifdef SIGALRM
					return SIGALRM;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_HUP:
#ifdef SIGHUP
					return SIGHUP;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_QUIT:
#ifdef SIGQUIT
					return SIGQUIT;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_TRAP:
#ifdef SIGTRAP
					return SIGTRAP;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_CONT:
#ifdef SIGCONT
					return SIGCONT;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_STOP:
#ifdef SIGSTOP
					return SIGSTOP;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_PIPE:
#ifdef SIGPIPE
					return SIGPIPE;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_CHLD:
#ifdef SIGCHLD
					return SIGCHLD;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_USR1:
#ifdef SIGUSR1
					return SIGUSR1;
#else
					return InvalidOffset + (int)Type;
#endif
				case Signal::SIG_USR2:
#ifdef SIGUSR2
					return SIGUSR2;
#else
					return InvalidOffset + (int)Type;
#endif
				default:
					VI_ASSERT(false, "invalid signal type %i", (int)Type);
					return -1;
			}
		}
		bool OS::Process::RaiseSignal(Signal Type)
		{
			int Id = GetSignalId(Type);
			if (Id == -1)
				return false;

			return raise(Id) == 0;
		}
		bool OS::Process::BindSignal(Signal Type, SignalCallback Callback)
		{
			int Id = GetSignalId(Type);
			if (Id == -1)
				return false;
			VI_DEBUG("[os] signal %i: %s", Id, Callback ? "callback" : "ignore");
#ifdef VI_LINUX
			struct sigaction Handle;
			Handle.sa_handler = Callback ? Callback : SIG_IGN;
			sigemptyset(&Handle.sa_mask);
			Handle.sa_flags = 0;

			return sigaction(Id, &Handle, NULL) != -1;
#else
			return signal(Id, Callback ? Callback : SIG_IGN) != SIG_ERR;
#endif
		}
		bool OS::Process::RebindSignal(Signal Type)
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
		ExpectsIO<int> OS::Process::Execute(const std::string_view& Command, FileMode Mode, ProcessCallback&& Callback)
		{
			VI_ASSERT(!Command.empty(), "commmand should be set");
			VI_DEBUG("[os] execute sh [ %.*s ]", (int)Command.size(), Command.data());
			UPtr<ProcessStream> Stream = new ProcessStream();
			auto Result = Stream->Open(Command, FileMode::Read_Only);
			if (!Result)
				return Result.Error();

			bool Notify = true;
			char Buffer[CHUNK_SIZE];
			while (true)
			{
				auto Size = Stream->ReadLine(Buffer, sizeof(Buffer));
				if (!Size || !*Size)
					break;
				else if (Notify && Callback)
					Notify = Callback(std::string_view(Buffer, *Size));
			}

			Result = Stream->Close();
			if (!Result)
				return Result.Error();

			return Stream->GetExitCode();
		}
		ExpectsIO<Unique<ProcessStream>> OS::Process::Spawn(const std::string_view& Command, FileMode Mode)
		{
			VI_ASSERT(!Command.empty(), "command should be set");
			VI_DEBUG("[os] execute sh [ %.*s ]", (int)Command.size(), Command.data());
			UPtr<ProcessStream> Stream = new ProcessStream();
			auto Result = Stream->Open(Command, Mode);
			if (Result)
				return Stream.Reset();

			return Result.Error();
		}
		String OS::Process::GetThreadId(const std::thread::id& Id)
		{
			StringStream Stream;
			Stream << Id;
			return Stream.str();
		}
		ExpectsIO<String> OS::Process::GetEnv(const std::string_view& Name)
		{
			VI_ASSERT(Stringify::IsCString(Name), "name should be set");
			VI_TRACE("[os] load env %.*s", (int)Name.size(), Name.data());
			if (!Control::Has(AccessOption::Env))
				return std::make_error_condition(std::errc::permission_denied);

			char* Value = std::getenv(Name.data());
			if (!Value)
				return OS::Error::GetConditionOr();

			String Output(Value, strlen(Value));
			return Output;
		}
		ExpectsIO<String> OS::Process::GetShell()
		{
#ifdef VI_MICROSOFT
			auto Shell = GetEnv("ComSpec");
			if (Shell)
				return Shell;
#else
			auto Shell = GetEnv("SHELL");
			if (Shell)
				return Shell;
#endif
			return std::make_error_condition(std::errc::no_such_file_or_directory);
		}
		InlineArgs OS::Process::ParseArgs(int ArgsCount, char** Args, size_t Opts, const UnorderedSet<String>& Flags)
		{
			VI_ASSERT(Args != nullptr, "arguments should be set");
			VI_ASSERT(ArgsCount > 0, "arguments count should be greater than zero");

			InlineArgs Context;
			for (int i = 0; i < ArgsCount; i++)
			{
				VI_ASSERT(Args[i] != nullptr, "argument %i should be set", i);
				Context.Params.push_back(Args[i]);
			}

			Vector<String> Params;
			Params.reserve(Context.Params.size());

			String Default = "1";
			auto InlineText = [&Default](const Core::String& Value) -> const Core::String& { return Value.empty() ? Default : Value; };
			for (size_t i = 1; i < Context.Params.size(); i++)
			{
				auto& Item = Context.Params[i];
				if (Item.empty() || Item.front() != '-')
					goto NoMatch;

				if ((Opts & (size_t)ArgsFormat::Key || Opts & (size_t)ArgsFormat::KeyValue) && Item.size() > 1 && Item[1] == '-')
				{
					Item = Item.substr(2);
					size_t Position = Item.find('=');
					if (Position != String::npos)
					{
						String Name = Item.substr(0, Position);
						if (Flags.find(Name) != Flags.end())
						{
							Context.Args[Item] = Default;
							if (Opts & (size_t)ArgsFormat::StopIfNoMatch)
							{
								Params.insert(Params.begin(), Context.Params.begin() + i + 1, Context.Params.end());
								break;
							}
						}
						else
							Context.Args[Name] = InlineText(Item.substr(Position + 1));
					}
					else if (Opts & (size_t)ArgsFormat::Key || Flags.find(Item) != Flags.end())
						Context.Args[Item] = Default;
					else
						goto NoMatch;
				}
				else 
				{
					String Name = Item.substr(1);
					if ((Opts & (size_t)ArgsFormat::FlagValue) && i + 1 < Context.Params.size() && Context.Params[i + 1].front() != '-')
					{
						if (Flags.find(Name) != Flags.end())
						{
							Context.Args[Name] = Default;
							if (Opts & (size_t)ArgsFormat::StopIfNoMatch)
							{
								Params.insert(Params.begin(), Context.Params.begin() + i + 1, Context.Params.end());
								break;
							}
						}
						else
							Context.Args[Name] = InlineText(Context.Params[++i]);
					}
					else if (Opts & (size_t)ArgsFormat::Flag || Flags.find(Name) != Flags.end())
						Context.Args[Item.substr(1)] = Default;
					else
						goto NoMatch;
				}

				continue;
			NoMatch:
				if (Opts & (size_t)ArgsFormat::StopIfNoMatch)
				{
					Params.insert(Params.begin(), Context.Params.begin() + i, Context.Params.end());
					break;
				}
				else
					Params.push_back(Item);
			}

			if (!Context.Params.empty())
				Context.Path = Context.Params.front();
			Context.Params = std::move(Params);
			return Context;
		}

		ExpectsIO<void*> OS::Symbol::Load(const std::string_view& Path)
		{
			VI_MEASURE(Timings::FileSystem);
			if (!Control::Has(AccessOption::Lib))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			if (Path.empty())
			{
				HMODULE Module = GetModuleHandle(nullptr);
				if (!Module)
					return OS::Error::GetConditionOr();

				return (void*)Module;
			}

			String Name = String(Path);
			if (!Stringify::EndsWith(Name, ".dll"))
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

			String Name = String(Path);
			if (!Stringify::EndsWith(Name, ".dylib"))
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

			String Name = String(Path);
			if (!Stringify::EndsWith(Name, ".so"))
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
		ExpectsIO<void*> OS::Symbol::LoadFunction(void* Handle, const std::string_view& Name)
		{
			VI_ASSERT(Handle != nullptr && Stringify::IsCString(Name), "handle should be set and name should not be empty");
			VI_DEBUG("[dl] load function %.*s", (int)Name.size(), Name.data());
			VI_MEASURE(Timings::FileSystem);
			if (!Control::Has(AccessOption::Lib))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			void* Result = (void*)GetProcAddress((HMODULE)Handle, Name.data());
			if (!Result)
				return OS::Error::GetConditionOr();

			return Result;
#elif defined(VI_LINUX)
			void* Result = (void*)dlsym(Handle, Name.data());
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

			return Expectation::Met;
#elif defined(VI_LINUX)
			if (dlclose(Handle) != 0)
				return OS::Error::GetConditionOr();

			return Expectation::Met;
#else
			return std::make_error_condition(std::errc::not_supported);
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

		void OS::Control::Set(AccessOption Option, bool Enabled)
		{
			VI_DEBUG("[os] control %s set %s", Control::ToString(Option).data(), Enabled ? "ON" : "OFF");
			uint64_t PrevOptions = Options.load();
			if (Enabled)
				PrevOptions |= (uint64_t)Option;
			else
				PrevOptions &= ~((uint64_t)Option);
			Options = PrevOptions;
		}
		bool OS::Control::Has(AccessOption Option)
		{
			return Options & (uint64_t)Option;
		}
		Option<AccessOption> OS::Control::ToOption(const std::string_view& Option)
		{
			if (Option == "mem")
				return AccessOption::Mem;
			if (Option == "fs")
				return AccessOption::Fs;
			if (Option == "gz")
				return AccessOption::Gz;
			if (Option == "net")
				return AccessOption::Net;
			if (Option == "lib")
				return AccessOption::Lib;
			if (Option == "http")
				return AccessOption::Http;
			if (Option == "https")
				return AccessOption::Https;
			if (Option == "shell")
				return AccessOption::Shell;
			if (Option == "env")
				return AccessOption::Env;
			if (Option == "addons")
				return AccessOption::Addons;
			if (Option == "all")
				return AccessOption::All;
			return Optional::None;
		}
		std::string_view OS::Control::ToString(AccessOption Option)
		{
			switch (Option)
			{
				case AccessOption::Mem:
					return "mem";
				case AccessOption::Fs:
					return "fs";
				case AccessOption::Gz:
					return "gz";
				case AccessOption::Net:
					return "net";
				case AccessOption::Lib:
					return "lib";
				case AccessOption::Http:
					return "http";
				case AccessOption::Https:
					return "https";
				case AccessOption::Shell:
					return "shell";
				case AccessOption::Env:
					return "env";
				case AccessOption::Addons:
					return "addons";
				case AccessOption::All:
					return "all";
				default:
					return "";
			}
		}
		std::string_view OS::Control::ToOptions()
		{
			return "mem, fs, gz, net, lib, http, https, shell, env, addons, all";
		}
		std::atomic<uint64_t> OS::Control::Options = (uint64_t)AccessOption::All;

		static thread_local Costate* InternalCoroutine = nullptr;
		Costate::Costate(size_t StackSize) noexcept : Thread(std::this_thread::get_id()), Current(nullptr), Master(Memory::New<Cocontext>()), Size(StackSize), ExternalCondition(nullptr), ExternalMutex(nullptr)
		{
			VI_TRACE("[co] spawn coroutine state 0x%" PRIXPTR " on thread %s", (void*)this, OS::Process::GetThreadId(Thread).c_str());
		}
		Costate::~Costate() noexcept
		{
			VI_TRACE("[co] despawn coroutine state 0x%" PRIXPTR " on thread %s", (void*)this, OS::Process::GetThreadId(Thread).c_str());
			if (InternalCoroutine == this)
				InternalCoroutine = nullptr;

			for (auto& Routine : Cached)
				Memory::Delete(Routine);

			for (auto& Routine : Used)
				Memory::Delete(Routine);

			Memory::Delete(Master);
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
				Routine = Memory::New<Coroutine>(this, std::move(Procedure));

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
#ifdef VI_FCONTEXT
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
			Memory::Delete(Routine);
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
				Memory::Delete(Routine);
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
#ifdef VI_FCONTEXT
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
		bool Costate::HasCoroutines() const
		{
			return !Used.empty();
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
			return InternalCoroutine;
		}
		Coroutine* Costate::GetCoroutine()
		{
			return InternalCoroutine ? InternalCoroutine->Current : nullptr;
		}
		bool Costate::GetState(Costate** State, Coroutine** Routine)
		{
			VI_ASSERT(State != nullptr, "state should be set");
			VI_ASSERT(Routine != nullptr, "state should be set");
			*Routine = (InternalCoroutine ? InternalCoroutine->Current : nullptr);
			*State = InternalCoroutine;

			return *Routine != nullptr;
		}
		bool Costate::IsCoroutine()
		{
			return InternalCoroutine ? InternalCoroutine->Current != nullptr : false;
		}
		void VI_COCALL Costate::ExecutionEntry(VI_CODATA)
		{
#ifdef VI_FCONTEXT
			transfer_t* Transfer = (transfer_t*)Context;
			Costate* State = (Costate*)Transfer->data;
			State->Master->Context = Transfer->fctx;
#elif VI_MICROSOFT
			Costate* State = (Costate*)Context;
#else
			Costate* State = (Costate*)Unpack2_64(X, Y);
#endif
			InternalCoroutine = State;
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
#ifdef VI_FCONTEXT
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
		Schedule::Desc::Desc(size_t Size) : PreallocatedSize(0), StackSize(STACK_SIZE), MaxCoroutines(96), MaxRecycles(64), IdleTimeout(std::chrono::milliseconds(2000)), ClockTimeout(std::chrono::milliseconds((uint64_t)Timings::Intensive)), Parallel(true)
		{
			if (!Size)
				Size = 1;
#ifndef VI_CXX20
			const size_t Async = (size_t)std::max(std::ceil(Size * 0.20), 1.0);
#else
			const size_t Async = 0;
#endif
			const size_t Timeout = 1;
			const size_t Sync = std::max<size_t>(std::min<size_t>(Size - Async - Timeout, Size), 1);
			Threads[((size_t)Difficulty::Async)] = Async;
			Threads[((size_t)Difficulty::Sync)] = Sync;
			Threads[((size_t)Difficulty::Timeout)] = Timeout;
			MaxCoroutines = std::min<size_t>(Size * 8, 256);
		}

		Schedule::Schedule() noexcept : Generation(0), Debug(nullptr), Terminate(false), Enqueue(true), Suspended(false), Active(false)
		{
			Timeouts = Memory::New<ConcurrentTimeoutQueue>();
			Async = Memory::New<ConcurrentAsyncQueue>();
			Sync = Memory::New<ConcurrentSyncQueue>();
		}
		Schedule::~Schedule() noexcept
		{
			Stop();
			Memory::Delete(Sync);
			Memory::Delete(Async);
			Memory::Delete(Timeouts);
			Memory::Release(Dispatcher.State);
			Scripting::VirtualMachine::CleanupThisThread();
		}
		TaskId Schedule::GetTaskId()
		{
			TaskId Id = ++Generation;
			while (Id == INVALID_TASK_ID)
				Id = ++Generation;
			return Id;
		}
		TaskId Schedule::SetInterval(uint64_t Milliseconds, TaskCallback&& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			ReportThread(ThreadTask::EnqueueTimer, 1, GetThread());
#endif
			VI_MEASURE(Timings::Atomic);
			auto Duration = std::chrono::microseconds(Milliseconds * 1000);
			auto Expires = GetClock() + Duration;
			auto Id = GetTaskId();

			UMutex<std::mutex> Unique(Timeouts->Update);
			Timeouts->Queue.emplace(std::make_pair(GetTimeout(Expires), Timeout(std::move(Callback), Duration, Id, true)));
			Timeouts->Resync = true;
			Timeouts->Notify.notify_all();
			return Id;
		}
		TaskId Schedule::SetTimeout(uint64_t Milliseconds, TaskCallback&& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			ReportThread(ThreadTask::EnqueueTimer, 1, GetThread());
#endif
			VI_MEASURE(Timings::Atomic);
			auto Duration = std::chrono::microseconds(Milliseconds * 1000);
			auto Expires = GetClock() + Duration;
			auto Id = GetTaskId();

			UMutex<std::mutex> Unique(Timeouts->Update);
			Timeouts->Queue.emplace(std::make_pair(GetTimeout(Expires), Timeout(std::move(Callback), Duration, Id, false)));
			Timeouts->Resync = true;
			Timeouts->Notify.notify_all();
			return Id;
		}
		bool Schedule::SetTask(TaskCallback&& Callback, bool Recyclable)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue)
				return false;
#ifndef NDEBUG
			ReportThread(ThreadTask::EnqueueTask, 1, GetThread());
#endif
			VI_MEASURE(Timings::Atomic);
			if (!Recyclable || !FastBypassEnqueue(Difficulty::Sync, std::move(Callback)))
				Sync->Queue.enqueue(std::move(Callback));
			return true;
		}
		bool Schedule::SetCoroutine(TaskCallback&& Callback, bool Recyclable)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			if (!Enqueue)
				return false;
#ifndef NDEBUG
			ReportThread(ThreadTask::EnqueueCoroutine, 1, GetThread());
#endif
			VI_MEASURE(Timings::Atomic);
			if (Recyclable && FastBypassEnqueue(Difficulty::Async, std::move(Callback)))
				return true;

			Async->Queue.enqueue(std::move(Callback));
			UMutex<std::mutex> Unique(Async->Update);
			for (auto* Thread : Threads[(size_t)Difficulty::Async])
				Thread->Notify.notify_all();

			return true;
		}
		bool Schedule::SetDebugCallback(ThreadDebugCallback&& Callback)
		{
#ifndef NDEBUG
			Debug = std::move(Callback);
			return true;
#else
			return false;
#endif
		}
		bool Schedule::ClearTimeout(TaskId Target)
		{
			VI_MEASURE(Timings::Atomic);
			if (Target == INVALID_TASK_ID)
				return false;

			UMutex<std::mutex> Unique(Timeouts->Update);
			for (auto It = Timeouts->Queue.begin(); It != Timeouts->Queue.end(); ++It)
			{
				if (It->second.Id == Target)
				{
					Timeouts->Resync = true;
					Timeouts->Queue.erase(It);
					Timeouts->Notify.notify_all();
					return true;
				}
			}
			return false;
		}
		bool Schedule::TriggerTimers()
		{
			VI_MEASURE(Timings::Pass);
			UMutex<std::mutex> Unique(Timeouts->Update);
			for (auto& Item : Timeouts->Queue)
				SetTask(std::move(Item.second.Callback));

			size_t Size = Timeouts->Queue.size();
			Timeouts->Resync = true;
			Timeouts->Queue.clear();
			return Size > 0;
		}
		bool Schedule::Trigger(Difficulty Type)
		{
			VI_MEASURE(Timings::Intensive);
			switch (Type)
			{
				case Difficulty::Timeout:
				{
					if (Timeouts->Queue.empty())
						return false;
					else if (Suspended)
						return true;

					auto Clock = GetClock();
					auto It = Timeouts->Queue.begin();
					if (It->first >= Clock)
						return true;
#ifndef NDEBUG
					ReportThread(ThreadTask::ProcessTimer, 1, nullptr);
#endif
					if (It->second.Alive && Active)
					{
						Timeout Next(std::move(It->second));
						Timeouts->Queue.erase(It);

						SetTask([this, Next = std::move(Next)]() mutable
						{
							Next.Callback();
							UMutex<std::mutex> Unique(Timeouts->Update);
							Timeouts->Queue.emplace(std::make_pair(GetTimeout(GetClock() + Next.Expires), std::move(Next)));
							Timeouts->Resync = true;
							Timeouts->Notify.notify_all();
						});
					}
					else
					{
						SetTask(std::move(It->second.Callback));
						Timeouts->Queue.erase(It);
					}
#ifndef NDEBUG
					ReportThread(ThreadTask::Awake, 0, nullptr);
#endif
					return true;
				}
				case Difficulty::Async:
				{
					if (!Dispatcher.State)
						Dispatcher.State = new Costate(Policy.StackSize);

					if (Suspended)
						return Dispatcher.State->HasCoroutines();

					size_t Active = Dispatcher.State->GetCount();
					size_t Cache = Policy.MaxCoroutines - Active, Executions = Active;
					while (Cache > 0 && Async->Queue.try_dequeue(Dispatcher.Event))
					{
						--Cache; ++Executions;
						Dispatcher.State->Pop(std::move(Dispatcher.Event));
#ifndef NDEBUG
						ReportThread(ThreadTask::ConsumeCoroutine, 1, nullptr);
#endif
					}
#ifndef NDEBUG
					ReportThread(ThreadTask::ProcessCoroutine, Dispatcher.State->GetCount(), nullptr);
#endif
					{
						VI_MEASURE(Timings::Frame);
						while (Dispatcher.State->Dispatch())
							++Executions;
					}
#ifndef NDEBUG
					ReportThread(ThreadTask::Awake, 0, nullptr);
#endif
					return Executions > 0;
				}
				case Difficulty::Sync:
				{
					if (Suspended)
						return Sync->Queue.size_approx() > 0;
					else if (!Sync->Queue.try_dequeue(Dispatcher.Event))
						return false;
#ifndef NDEBUG
					ReportThread(ThreadTask::ProcessTask, 1, nullptr);
#endif
					Dispatcher.Event();
#ifndef NDEBUG
					ReportThread(ThreadTask::Awake, 0, nullptr);
#endif
					return true;
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
			Active = true;

			if (!Policy.Parallel)
			{
				InitializeThread(nullptr, true);
				return true;
			}

			size_t Index = 0;
			for (size_t j = 0; j < Policy.Threads[(size_t)Difficulty::Async]; j++)
				PushThread(Difficulty::Async, Index++, j, false);

			for (size_t j = 0; j < Policy.Threads[(size_t)Difficulty::Sync]; j++)
				PushThread(Difficulty::Sync, Index++, j, false);

			InitializeSpawnTrigger();
			if (Policy.Threads[(size_t)Difficulty::Timeout] > 0)
			{
				if (Policy.Ping)
					PushThread(Difficulty::Timeout, 0, 0, true);
				PushThread(Difficulty::Timeout, Index++, 0, false);
			}

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

			Timeouts->Queue.clear();
			Terminate = false;
			Enqueue = true;
			ChunkCleanup();
			return true;
		}
		bool Schedule::Wakeup()
		{
			VI_TRACE("[schedule] wakeup 0x%" PRIXPTR " on thread %s", (void*)this, OS::Process::GetThreadId(std::this_thread::get_id()).c_str());
			TaskCallback Dummy[2] = { []() { }, []() { } };
			size_t DummySize = sizeof(Dummy) / sizeof(TaskCallback);
			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
			{
				for (auto* Thread : Threads[i])
				{
					if (Thread->Type == Difficulty::Async)
						Async->Queue.enqueue_bulk(Dummy, DummySize);
					else if (Thread->Type == Difficulty::Sync || Thread->Type == Difficulty::Timeout)
						Sync->Queue.enqueue_bulk(Dummy, DummySize);
					Thread->Notify.notify_all();
				}

				if (i == (size_t)Difficulty::Async)
				{
					UMutex<std::mutex> Unique(Async->Update);
					Async->Resync = true;
					Async->Notify.notify_all();
				}
				else if (i == (size_t)Difficulty::Timeout)
				{
					UMutex<std::mutex> Unique(Timeouts->Update);
					Timeouts->Resync = true;
					Timeouts->Notify.notify_all();
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
		bool Schedule::TriggerThread(Difficulty Type, ThreadData* Thread)
		{
			String ThreadId = OS::Process::GetThreadId(Thread->Id);
			InitializeThread(Thread, true);
			if (!ThreadActive(Thread))
				goto ExitThread;

			switch (Type)
			{
				case Difficulty::Timeout:
				{
					TaskCallback Event;
					ReceiveToken Token(Sync->Queue);
					if (Thread->Daemon)
						VI_DEBUG("[schedule] acquire thread %s (timers)", ThreadId.c_str());
					else
						VI_DEBUG("[schedule] spawn thread %s (timers)", ThreadId.c_str());

					do
					{
						if (SleepThread(Type, Thread))
							continue;

						std::unique_lock<std::mutex> Unique(Timeouts->Update);
					Retry:
#ifndef NDEBUG
						ReportThread(ThreadTask::Awake, 0, Thread);
#endif
						std::chrono::microseconds When = std::chrono::microseconds(0);
						if (!Timeouts->Queue.empty())
						{
							auto Clock = GetClock();
							auto It = Timeouts->Queue.begin();
							if (It->first <= Clock)
							{
#ifndef NDEBUG
								ReportThread(ThreadTask::ProcessTimer, 1, Thread);
#endif
								if (It->second.Alive)
								{
									Timeout Next(std::move(It->second));
									Timeouts->Queue.erase(It);

									SetTask([this, Next = std::move(Next)]() mutable
									{
										Next.Callback();
										UMutex<std::mutex> Unique(Timeouts->Update);
										Timeouts->Queue.emplace(std::make_pair(GetTimeout(GetClock() + Next.Expires), std::move(Next)));
										Timeouts->Resync = true;
										Timeouts->Notify.notify_all();
									});
								}
								else
								{
									SetTask(std::move(It->second.Callback));
									Timeouts->Queue.erase(It);
								}

								goto Retry;
							}
							else
							{
								When = It->first - Clock;
								if (When > Policy.IdleTimeout)
									When = Policy.IdleTimeout;
							}
						}
						else
							When = Policy.IdleTimeout;
#ifndef NDEBUG
						ReportThread(ThreadTask::Sleep, 0, Thread);
#endif
						Timeouts->Notify.wait_for(Unique, When, [this, Thread]() { return !ThreadActive(Thread) || Timeouts->Resync || Sync->Queue.size_approx() > 0; });
						Timeouts->Resync = false;
						Unique.unlock();
	
						if (!Sync->Queue.try_dequeue(Token, Event))
							continue;
#ifndef NDEBUG
						ReportThread(ThreadTask::Awake, 0, Thread);
						ReportThread(ThreadTask::ProcessTask, 1, Thread);
#endif
						VI_MEASURE(Timings::Intensive);
						Event();
					} while (ThreadActive(Thread));
					break;
				}
				case Difficulty::Async:
				{
					TaskCallback Event;
					ReceiveToken Token(Async->Queue);
					if (Thread->Daemon)
						VI_DEBUG("[schedule] acquire thread %s (coroutines)", ThreadId.c_str());
					else
						VI_DEBUG("[schedule] spawn thread %s (coroutines)", ThreadId.c_str());

					UPtr<Costate> State = new Costate(Policy.StackSize);
					State->ExternalCondition = &Thread->Notify;
					State->ExternalMutex = &Thread->Update;

					do
					{
						if (SleepThread(Type, Thread))
							continue;
#ifndef NDEBUG
						ReportThread(ThreadTask::Awake, 0, Thread);
#endif
						size_t Cache = Policy.MaxCoroutines - State->GetCount();
						while (Cache > 0)
						{
							if (!Thread->Queue.empty())
							{
								Event = std::move(Thread->Queue.front());
								Thread->Queue.pop();
							}
							else if (!Async->Queue.try_dequeue(Token, Event))
								break;

							--Cache;
							State->Pop(std::move(Event));
#ifndef NDEBUG
							ReportThread(ThreadTask::EnqueueCoroutine, 1, Thread);
#endif
						}
#ifndef NDEBUG
						ReportThread(ThreadTask::ProcessCoroutine, State->GetCount(), Thread);
#endif
						{
							VI_MEASURE(Timings::Frame);
							State->Dispatch();
						}
#ifndef NDEBUG
						ReportThread(ThreadTask::Sleep, 0, Thread);
#endif
						std::unique_lock<std::mutex> Unique(Thread->Update);
						Thread->Notify.wait_for(Unique, Policy.IdleTimeout, [this, &State, Thread]()
						{
							return !ThreadActive(Thread) || State->HasResumableCoroutines() || Async->Resync.load() || (Async->Queue.size_approx() > 0 && State->GetCount() + 1 < Policy.MaxCoroutines);
						});
						Async->Resync = false;
					} while (ThreadActive(Thread));
					while (!Thread->Queue.empty())
					{
						Async->Queue.enqueue(std::move(Thread->Queue.front()));
						Thread->Queue.pop();
					}
					break;
				}
				case Difficulty::Sync:
				{
					TaskCallback Event;
					ReceiveToken Token(Sync->Queue);
					if (Thread->Daemon)
						VI_DEBUG("[schedule] acquire thread %s (tasks)", ThreadId.c_str());
					else
						VI_DEBUG("[schedule] spawn thread %s (tasks)", ThreadId.c_str());

					do
					{
						if (SleepThread(Type, Thread))
							continue;
#ifndef NDEBUG
						ReportThread(ThreadTask::Sleep, 0, Thread);
#endif
						if (!Thread->Queue.empty())
						{
							Event = std::move(Thread->Queue.front());
							Thread->Queue.pop();
						}
						else if (!Sync->Queue.wait_dequeue_timed(Token, Event, Policy.IdleTimeout))
							continue;
#ifndef NDEBUG
						ReportThread(ThreadTask::Awake, 0, Thread);
						ReportThread(ThreadTask::ProcessTask, 1, Thread);
#endif
						VI_MEASURE(Timings::Intensive);
						Event();
					} while (ThreadActive(Thread));
					while (!Thread->Queue.empty())
					{
						Sync->Queue.enqueue(std::move(Thread->Queue.front()));
						Thread->Queue.pop();
					}
					break;
				}
				default:
					break;
			}

		ExitThread:
			if (Thread->Daemon)
				VI_DEBUG("[schedule] release thread %s", ThreadId.c_str());
			else
				VI_DEBUG("[schedule] join thread %s", ThreadId.c_str());

			Scripting::VirtualMachine::CleanupThisThread();
			InitializeThread(nullptr, true);
			return true;
		}
		bool Schedule::SleepThread(Difficulty Type, ThreadData* Thread)
		{
			if (!Suspended)
				return false;

#ifndef NDEBUG
			ReportThread(ThreadTask::Sleep, 0, Thread);
#endif
			std::unique_lock<std::mutex> Unique(Thread->Update);
			Thread->Notify.wait_for(Unique, Policy.IdleTimeout, [this, Thread]()
			{
				return !ThreadActive(Thread) || !Suspended;
			});
#ifndef NDEBUG
			ReportThread(ThreadTask::Awake, 0, Thread);
#endif
			return true;
		}
		bool Schedule::ThreadActive(ThreadData* Thread)
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
					Memory::Delete(Thread);
				Threads[i].clear();
			}

			return true;
		}
		bool Schedule::PushThread(Difficulty Type, size_t GlobalIndex, size_t LocalIndex, bool IsDaemon)
		{
			ThreadData* Thread = Memory::New<ThreadData>(Type, Policy.PreallocatedSize, GlobalIndex, LocalIndex, IsDaemon);
			if (!Thread->Daemon)
			{
				Thread->Handle = std::thread(&Schedule::TriggerThread, this, Type, Thread);
				Thread->Id = Thread->Handle.get_id();
			}
			else
				Thread->Id = std::this_thread::get_id();
#ifndef NDEBUG
			ReportThread(ThreadTask::Spawn, 0, Thread);
#endif
			Threads[(size_t)Type].emplace_back(Thread);
			return Thread->Daemon ? TriggerThread(Type, Thread) : Thread->Handle.joinable();
		}
		bool Schedule::PopThread(ThreadData* Thread)
		{
			if (Thread->Daemon)
				return true;

			if (Thread->Id == std::this_thread::get_id())
				return false;

			if (Thread->Handle.joinable())
				Thread->Handle.join();
#ifndef NDEBUG
			ReportThread(ThreadTask::Despawn, 0, Thread);
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
			switch (Type)
			{
				case Difficulty::Async:
					return Async->Queue.size_approx() > 0;
				case Difficulty::Sync:
					return Sync->Queue.size_approx() > 0;
				case Difficulty::Timeout:
					return Timeouts->Queue.size() > 0;
				default:
					return false;
			}
		}
		bool Schedule::HasAnyTasks() const
		{
			return HasTasks(Difficulty::Sync) || HasTasks(Difficulty::Async) || HasTasks(Difficulty::Timeout);
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
		bool Schedule::ReportThread(ThreadTask State, size_t Tasks, const ThreadData* Thread)
		{
			if (!Debug)
				return false;

			Debug(ThreadMessage(Thread, State, Tasks));
			return true;
		}
		bool Schedule::FastBypassEnqueue(Difficulty Type, TaskCallback&& Callback)
		{
			if (!HasParallelThreads(Type))
				return false;

			auto* Thread = (ThreadData*)InitializeThread(nullptr, false);
			if (!Thread || Thread->Type != Type || Thread->Queue.size() >= Policy.MaxRecycles)
				return false;

			Thread->Queue.push(std::move(Callback));
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
		bool Schedule::HasParallelThreads(Difficulty Type) const
		{
			VI_ASSERT(Type != Difficulty::Count, "difficulty should be set");
			return Threads[(size_t)Type].size() > 1;
		}
		void Schedule::InitializeSpawnTrigger()
		{
			if (!Policy.Initialize)
				return;

			bool IsPending = true;
			std::recursive_mutex AwaitingMutex;
			std::unique_lock<std::recursive_mutex> Unique(AwaitingMutex);
			std::condition_variable_any Ready;
			Policy.Initialize([&AwaitingMutex, &Ready, &IsPending]()
			{
				std::unique_lock<std::recursive_mutex> Unique(AwaitingMutex);
				IsPending = false;
				Ready.notify_all();
			});
			Ready.wait(Unique, [&IsPending]() { return !IsPending; });
		}
		const Schedule::ThreadData* Schedule::InitializeThread(ThreadData* Source, bool Update) const
		{
			static thread_local ThreadData* InternalThread = nullptr;
			if (Update)
				InternalThread = Source;
			return InternalThread;
		}
		const Schedule::ThreadData* Schedule::GetThread() const
		{
			return InitializeThread(nullptr, false);
		}
		const Schedule::Desc& Schedule::GetPolicy() const
		{
			return Policy;
		}
		std::chrono::microseconds Schedule::GetTimeout(std::chrono::microseconds Clock)
		{
			while (Timeouts->Queue.find(Clock) != Timeouts->Queue.end())
				++Clock;
			return Clock;
		}
		std::chrono::microseconds Schedule::GetClock()
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
		}
		bool Schedule::IsAvailable(Difficulty Type)
		{
			if (!HasInstance())
				return false;

			auto* Instance = Get();
			if (!Instance->Active || !Instance->Enqueue)
				return false;

			return Type == Difficulty::Count || Instance->HasParallelThreads(Type);
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
			size_t Index = 0;
			UnorderedMap<String, size_t> Mapping;
			GenerateNamingTable(this, &Mapping, Index);
			return Mapping;
		}
		Vector<Schema*> Schema::FindCollection(const std::string_view& Name, bool Deep) const
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
		Vector<Schema*> Schema::FetchCollection(const std::string_view& Notation, bool Deep) const
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
		Schema* Schema::Find(const std::string_view& Name, bool Deep) const
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
		Schema* Schema::Fetch(const std::string_view& Notation, bool Deep) const
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
		Schema* Schema::GetAttribute(const std::string_view& Name) const
		{
			return Get(':' + String(Name));
		}
		Variant Schema::FetchVar(const std::string_view& fKey, bool Deep) const
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
		Variant Schema::GetVar(const std::string_view& fKey) const
		{
			Schema* Result = Get(fKey);
			if (!Result)
				return Var::Undefined();

			return Result->Value;
		}
		Variant Schema::GetAttributeVar(const std::string_view& Key) const
		{
			return GetVar(':' + String(Key));
		}
		Schema* Schema::Get(size_t Index) const
		{
			VI_ASSERT(Nodes != nullptr, "there must be at least one node");
			VI_ASSERT(Index < Nodes->size(), "index outside of range");

			return (*Nodes)[Index];
		}
		Schema* Schema::Get(const std::string_view& Name) const
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
		Schema* Schema::Set(const std::string_view& Name)
		{
			return Set(Name, Var::Object());
		}
		Schema* Schema::Set(const std::string_view& Name, const Variant& Base)
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
		Schema* Schema::Set(const std::string_view& Name, Variant&& Base)
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
		Schema* Schema::Set(const std::string_view& Name, Schema* Base)
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
					Memory::Release(*It);
					*It = Base;
					return Base;
				}
			}

			Allocate();
			Nodes->push_back(Base);
			return Base;
		}
		Schema* Schema::SetAttribute(const std::string_view& Name, const Variant& fValue)
		{
			return Set(':' + String(Name), fValue);
		}
		Schema* Schema::SetAttribute(const std::string_view& Name, Variant&& fValue)
		{
			return Set(':' + String(Name), std::move(fValue));
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
			Memory::Release(Base);
			Nodes->erase(It);
			Saved = false;

			return this;
		}
		Schema* Schema::Pop(const std::string_view& Name)
		{
			if (!Nodes)
				return this;

			for (auto It = Nodes->begin(); It != Nodes->end(); ++It)
			{
				if (!*It || (*It)->Key != Name)
					continue;

				(*It)->Parent = nullptr;
				Memory::Release(*It);
				Nodes->erase(It);
				Saved = false;
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
		bool Schema::Rename(const std::string_view& Name, const std::string_view& NewName)
		{
			VI_ASSERT(!Name.empty() && !NewName.empty(), "name and new name should not be empty");

			Schema* Result = Get(Name);
			if (!Result)
				return false;

			Result->Key = NewName;
			return true;
		}
		bool Schema::Has(const std::string_view& Name) const
		{
			return Fetch(Name) != nullptr;
		}
		bool Schema::HasAttribute(const std::string_view& Name) const
		{
			return Fetch(':' + String(Name)) != nullptr;
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
					Memory::Release(Next);
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

			for (auto& Next : *Nodes)
			{
				if (Next != nullptr)
				{
					Next->Parent = nullptr;
					Memory::Release(Next);
				}
			}

			Memory::Delete(Nodes);
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
				Nodes = Memory::New<Vector<Schema*>>();
		}
		void Schema::Allocate(const Vector<Schema*>& Other)
		{
			if (!Nodes)
				Nodes = Memory::New<Vector<Schema*>>(Other);
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
			Callback(VarForm::Write_Tab, "");
			Callback(VarForm::Dummy, "<");
			Callback(VarForm::Dummy, Base->Key);

			if (Attributes.empty())
			{
				if (Scalable)
					Callback(VarForm::Dummy, ">");
				else
					Callback(VarForm::Dummy, " />");
			}
			else
				Callback(VarForm::Dummy, " ");

			for (auto It = Attributes.begin(); It != Attributes.end(); ++It)
			{
				String Key = (*It)->GetName();
				String Value = (*It)->Value.Serialize();

				Callback(VarForm::Dummy, Key);
				Callback(VarForm::Dummy, "=\"");
				Callback(VarForm::Dummy, Value);
				++It;

				if (It == Attributes.end())
				{
					if (!Scalable)
					{
						Callback(VarForm::Write_Space, "\"");
						Callback(VarForm::Dummy, "/>");
					}
					else
						Callback(VarForm::Dummy, "\">");
				}
				else
					Callback(VarForm::Write_Space, "\"");

				--It;
			}

			Callback(VarForm::Tab_Increase, "");
			if (Base->Value.Size() > 0)
			{
				String Text = Base->Value.Serialize();
				if (Base->Nodes != nullptr && !Base->Nodes->empty())
				{
					Callback(VarForm::Write_Line, "");
					Callback(VarForm::Write_Tab, "");
					Callback(VarForm::Dummy, Text);
					Callback(VarForm::Write_Line, "");
				}
				else
					Callback(VarForm::Dummy, Text);
			}
			else
				Callback(VarForm::Write_Line, "");

			if (Base->Nodes != nullptr)
			{
				for (auto&& It : *Base->Nodes)
				{
					if (!It->IsAttribute())
						ConvertToXML(It, Callback);
				}
			}

			Callback(VarForm::Tab_Decrease, "");
			if (!Scalable)
				return;

			if (Base->Nodes != nullptr && !Base->Nodes->empty())
				Callback(VarForm::Write_Tab, "");

			Callback(VarForm::Dummy, "</");
			Callback(VarForm::Dummy, Base->Key);
			Callback(Base->Parent ? VarForm::Write_Line : VarForm::Dummy, ">");
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
						Callback(VarForm::Dummy, Value.substr(1, Value.size() - 2));
					else
						Callback(VarForm::Dummy, Value);
				}
				else
				{
					Callback(VarForm::Dummy, "\"");
					Callback(VarForm::Dummy, Value);
					Callback(VarForm::Dummy, "\"");
				}
				return;
			}

			size_t Size = (Base->Nodes ? Base->Nodes->size() : 0);
			bool Array = (Base->Value.Type == VarType::Array);
			if (!Size)
			{
				Callback(VarForm::Dummy, Array ? "[]" : "{}");
				return;
			}

			Callback(VarForm::Dummy, Array ? "[" : "{");
			Callback(VarForm::Tab_Increase, "");

			for (size_t i = 0; i < Size; i++)
			{
				auto* Next = (*Base->Nodes)[i];
				if (!Array)
				{
					Callback(VarForm::Write_Line, "");
					Callback(VarForm::Write_Tab, "");
					Callback(VarForm::Dummy, "\"");
					Callback(VarForm::Dummy, Next->Key);
					Callback(VarForm::Write_Space, "\":");
				}

				if (!Next->Value.IsObject())
				{
					auto Type = Next->Value.GetType();
					String Value = (Type == VarType::Undefined ? "null" : Next->Value.Serialize());
					Stringify::Escape(Value);
					if (Array)
					{
						Callback(VarForm::Write_Line, "");
						Callback(VarForm::Write_Tab, "");
					}

					if (Type == VarType::Decimal)
					{
						bool BigNumber = !((Decimal*)Next->Value.GetContainer())->IsSafeNumber();
						if (BigNumber)
							Callback(VarForm::Dummy, "\"");

						if (Value.size() >= 2 && Value.front() == PREFIX_ENUM[0] && Value.back() == PREFIX_ENUM[0])
							Callback(VarForm::Dummy, Value.substr(1, Value.size() - 2));
						else
							Callback(VarForm::Dummy, Value);

						if (BigNumber)
							Callback(VarForm::Dummy, "\"");
					}
					else if (!Next->Value.IsObject() && Type != VarType::String && Type != VarType::Binary)
					{
						if (Value.size() >= 2 && Value.front() == PREFIX_ENUM[0] && Value.back() == PREFIX_ENUM[0])
							Callback(VarForm::Dummy, Value.substr(1, Value.size() - 2));
						else
							Callback(VarForm::Dummy, Value);
					}
					else
					{
						Callback(VarForm::Dummy, "\"");
						Callback(VarForm::Dummy, Value);
						Callback(VarForm::Dummy, "\"");
					}
				}
				else
				{
					if (Array)
					{
						Callback(VarForm::Write_Line, "");
						Callback(VarForm::Write_Tab, "");
					}
					ConvertToJSON(Next, Callback);
				}

				if (i + 1 < Size)
					Callback(VarForm::Dummy, ",");
			}

			Callback(VarForm::Tab_Decrease, "");
			Callback(VarForm::Write_Line, "");
			if (Base->Parent != nullptr)
				Callback(VarForm::Write_Tab, "");

			Callback(VarForm::Dummy, Array ? "]" : "}");
		}
		void Schema::ConvertToJSONB(Schema* Base, const SchemaWriteCallback& Callback)
		{
			VI_ASSERT(Base != nullptr && Callback, "base should be set and callback should not be empty");
			UnorderedMap<String, size_t> Mapping = Base->GetNames();
			uint32_t Set = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint32_t)Mapping.size());
			uint64_t Version = OS::CPU::ToEndianness<uint64_t>(OS::CPU::Endian::Little, JSONB_VERSION);
			Callback(VarForm::Dummy, std::string_view((const char*)&Version, sizeof(uint64_t)));
			Callback(VarForm::Dummy, std::string_view((const char*)&Set, sizeof(uint32_t)));

			for (auto It = Mapping.begin(); It != Mapping.end(); ++It)
			{
				uint32_t Id = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint32_t)It->second);
				Callback(VarForm::Dummy, std::string_view((const char*)&Id, sizeof(uint32_t)));

				uint16_t Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint16_t)It->first.size());
				Callback(VarForm::Dummy, std::string_view((const char*)&Size, sizeof(uint16_t)));

				if (Size > 0)
					Callback(VarForm::Dummy, std::string_view(It->first.c_str(), sizeof(char) * (size_t)Size));
			}
			ProcessConvertionToJSONB(Base, &Mapping, Callback);
		}
		String Schema::ToXML(Schema* Value)
		{
			String Result;
			ConvertToXML(Value, [&](VarForm Type, const std::string_view& Buffer) { Result.append(Buffer); });
			return Result;
		}
		String Schema::ToJSON(Schema* Value)
		{
			String Result;
			ConvertToJSON(Value, [&](VarForm Type, const std::string_view& Buffer) { Result.append(Buffer); });
			return Result;
		}
		Vector<char> Schema::ToJSONB(Schema* Value)
		{
			Vector<char> Result;
			ConvertToJSONB(Value, [&](VarForm Type, const std::string_view& Buffer)
			{
				if (Buffer.empty())
					return;

				size_t Offset = Result.size();
				Result.resize(Offset + Buffer.size());
				memcpy(&Result[Offset], Buffer.data(), Buffer.size());
			});
			return Result;
		}
		ExpectsParser<Schema*> Schema::ConvertFromXML(const std::string_view& Buffer)
		{
#ifdef VI_PUGIXML
			if (Buffer.empty())
				return ParserException(ParserError::XMLNoDocumentElement, 0, "empty XML buffer");

			pugi::xml_document Data;
			pugi::xml_parse_result Status = Data.load_buffer(Buffer.data(), Buffer.size());
			if (!Status)
			{
				switch (Status.status)
				{
					case pugi::status_out_of_memory:
						return ParserException(ParserError::XMLOutOfMemory, (size_t)Status.offset, Status.description());
					case pugi::status_internal_error:
						return ParserException(ParserError::XMLInternalError, (size_t)Status.offset, Status.description());
					case pugi::status_unrecognized_tag:
						return ParserException(ParserError::XMLUnrecognizedTag, (size_t)Status.offset, Status.description());
					case pugi::status_bad_pi:
						return ParserException(ParserError::XMLBadPi, (size_t)Status.offset, Status.description());
					case pugi::status_bad_comment:
						return ParserException(ParserError::XMLBadComment, (size_t)Status.offset, Status.description());
					case pugi::status_bad_cdata:
						return ParserException(ParserError::XMLBadCData, (size_t)Status.offset, Status.description());
					case pugi::status_bad_doctype:
						return ParserException(ParserError::XMLBadDocType, (size_t)Status.offset, Status.description());
					case pugi::status_bad_pcdata:
						return ParserException(ParserError::XMLBadPCData, (size_t)Status.offset, Status.description());
					case pugi::status_bad_start_element:
						return ParserException(ParserError::XMLBadStartElement, (size_t)Status.offset, Status.description());
					case pugi::status_bad_attribute:
						return ParserException(ParserError::XMLBadAttribute, (size_t)Status.offset, Status.description());
					case pugi::status_bad_end_element:
						return ParserException(ParserError::XMLBadEndElement, (size_t)Status.offset, Status.description());
					case pugi::status_end_element_mismatch:
						return ParserException(ParserError::XMLEndElementMismatch, (size_t)Status.offset, Status.description());
					case pugi::status_append_invalid_root:
						return ParserException(ParserError::XMLAppendInvalidRoot, (size_t)Status.offset, Status.description());
					case pugi::status_no_document_element:
						return ParserException(ParserError::XMLNoDocumentElement, (size_t)Status.offset, Status.description());
					default:
						return ParserException(ParserError::XMLInternalError, (size_t)Status.offset, Status.description());
				}
			}

			pugi::xml_node Main = Data.first_child();
			Schema* Result = Var::Set::Array();
			ProcessConvertionFromXML((void*)&Main, Result);
			return Result;
#else
			return ParserException(ParserError::NotSupported, 0, "no capabilities to parse XML");
#endif
		}
		ExpectsParser<Schema*> Schema::ConvertFromJSON(const std::string_view& Buffer)
		{
#ifdef VI_RAPIDJSON
			if (Buffer.empty())
				return ParserException(ParserError::JSONDocumentEmpty, 0);

			rapidjson::Document Base;
			Base.Parse<rapidjson::kParseNumbersAsStringsFlag>(Buffer.data(), Buffer.size());

			Schema* Result = nullptr;
			if (Base.HasParseError())
			{
				size_t Offset = Base.GetErrorOffset();
				switch (Base.GetParseError())
				{
					case rapidjson::kParseErrorDocumentEmpty:
						return ParserException(ParserError::JSONDocumentEmpty, Offset);
					case rapidjson::kParseErrorDocumentRootNotSingular:
						return ParserException(ParserError::JSONDocumentRootNotSingular, Offset);
					case rapidjson::kParseErrorValueInvalid:
						return ParserException(ParserError::JSONValueInvalid, Offset);
					case rapidjson::kParseErrorObjectMissName:
						return ParserException(ParserError::JSONObjectMissName, Offset);
					case rapidjson::kParseErrorObjectMissColon:
						return ParserException(ParserError::JSONObjectMissColon, Offset);
					case rapidjson::kParseErrorObjectMissCommaOrCurlyBracket:
						return ParserException(ParserError::JSONObjectMissCommaOrCurlyBracket, Offset);
					case rapidjson::kParseErrorArrayMissCommaOrSquareBracket:
						return ParserException(ParserError::JSONArrayMissCommaOrSquareBracket, Offset);
					case rapidjson::kParseErrorStringUnicodeEscapeInvalidHex:
						return ParserException(ParserError::JSONStringUnicodeEscapeInvalidHex, Offset);
					case rapidjson::kParseErrorStringUnicodeSurrogateInvalid:
						return ParserException(ParserError::JSONStringUnicodeSurrogateInvalid, Offset);
					case rapidjson::kParseErrorStringEscapeInvalid:
						return ParserException(ParserError::JSONStringEscapeInvalid, Offset);
					case rapidjson::kParseErrorStringMissQuotationMark:
						return ParserException(ParserError::JSONStringMissQuotationMark, Offset);
					case rapidjson::kParseErrorStringInvalidEncoding:
						return ParserException(ParserError::JSONStringInvalidEncoding, Offset);
					case rapidjson::kParseErrorNumberTooBig:
						return ParserException(ParserError::JSONNumberTooBig, Offset);
					case rapidjson::kParseErrorNumberMissFraction:
						return ParserException(ParserError::JSONNumberMissFraction, Offset);
					case rapidjson::kParseErrorNumberMissExponent:
						return ParserException(ParserError::JSONNumberMissExponent, Offset);
					case rapidjson::kParseErrorTermination:
						return ParserException(ParserError::JSONTermination, Offset);
					case rapidjson::kParseErrorUnspecificSyntaxError:
						return ParserException(ParserError::JSONUnspecificSyntaxError, Offset);
					default:
						return ParserException(ParserError::BadValue);
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
					Result = ProcessConversionFromJSONStringOrNumber(&Base, true);
					break;
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
			return ParserException(ParserError::NotSupported, 0, "no capabilities to parse JSON");
#endif
		}
		ExpectsParser<Schema*> Schema::ConvertFromJSONB(const SchemaReadCallback& Callback)
		{
			VI_ASSERT(Callback, "callback should not be empty");
			uint64_t Version = 0;
			if (!Callback((uint8_t*)&Version, sizeof(uint64_t)))
				return ParserException(ParserError::BadVersion);

			Version = OS::CPU::ToEndianness<uint64_t>(OS::CPU::Endian::Little, Version);
			if (Version != JSONB_VERSION)
				return ParserException(ParserError::BadVersion);

			uint32_t Set = 0;
			if (!Callback((uint8_t*)&Set, sizeof(uint32_t)))
				return ParserException(ParserError::BadDictionary);

			UnorderedMap<size_t, String> Map;
			Set = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Set);

			for (uint32_t i = 0; i < Set; ++i)
			{
				uint32_t Index = 0;
				if (!Callback((uint8_t*)&Index, sizeof(uint32_t)))
					return ParserException(ParserError::BadNameIndex);

				uint16_t Size = 0;
				if (!Callback((uint8_t*)&Size, sizeof(uint16_t)))
					return ParserException(ParserError::BadName);

				Index = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Index);
				Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);

				if (Size <= 0)
					continue;

				String Name;
				Name.resize((size_t)Size);
				if (!Callback((uint8_t*)Name.c_str(), sizeof(char) * Size))
					return ParserException(ParserError::BadName);

				Map.insert({ Index, Name });
			}

			UPtr<Schema> Current = Var::Set::Object();
			auto Status = ProcessConvertionFromJSONB(*Current, &Map, Callback);
			if (!Status)
				return Status.Error();

			return Current.Reset();
		}
		ExpectsParser<Schema*> Schema::FromXML(const std::string_view& Text)
		{
			return ConvertFromXML(Text);
		}
		ExpectsParser<Schema*> Schema::FromJSON(const std::string_view& Text)
		{
			return ConvertFromJSON(Text);
		}
		ExpectsParser<Schema*> Schema::FromJSONB(const std::string_view& Binary)
		{
			size_t Offset = 0;
			return ConvertFromJSONB([&Binary, &Offset](uint8_t* Buffer, size_t Length)
			{
				if (Offset + Length > Binary.size())
					return false;

				memcpy((void*)Buffer, Binary.data() + Offset, Length);
				Offset += Length;
				return true;
			});
		}
		Expects<void, ParserException> Schema::ProcessConvertionFromJSONB(Schema* Current, UnorderedMap<size_t, String>* Map, const SchemaReadCallback& Callback)
		{
			uint32_t Id = 0;
			if (!Callback((uint8_t*)&Id, sizeof(uint32_t)))
				return ParserException(ParserError::BadKeyName);

			if (Id != (uint32_t)-1)
			{
				auto It = Map->find((size_t)OS::CPU::ToEndianness(OS::CPU::Endian::Little, Id));
				if (It != Map->end())
					Current->Key = It->second;
			}

			if (!Callback((uint8_t*)&Current->Value.Type, sizeof(VarType)))
				return ParserException(ParserError::BadKeyType);

			switch (Current->Value.Type)
			{
				case VarType::Object:
				case VarType::Array:
				{
					uint32_t Count = 0;
					if (!Callback((uint8_t*)&Count, sizeof(uint32_t)))
						return ParserException(ParserError::BadValue);

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
					if (!Callback((uint8_t*)&Size, sizeof(uint32_t)))
						return ParserException(ParserError::BadValue);

					String Buffer;
					Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);
					Buffer.resize((size_t)Size);

					if (!Callback((uint8_t*)Buffer.c_str(), (size_t)Size * sizeof(char)))
						return ParserException(ParserError::BadString);

					Current->Value = Var::String(Buffer);
					break;
				}
				case VarType::Binary:
				{
					uint32_t Size = 0;
					if (!Callback((uint8_t*)&Size, sizeof(uint32_t)))
						return ParserException(ParserError::BadValue);

					String Buffer;
					Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);
					Buffer.resize(Size);

					if (!Callback((uint8_t*)Buffer.c_str(), (size_t)Size * sizeof(char)))
						return ParserException(ParserError::BadString);

					Current->Value = Var::Binary((uint8_t*)Buffer.data(), Buffer.size());
					break;
				}
				case VarType::Integer:
				{
					int64_t Integer = 0;
					if (!Callback((uint8_t*)&Integer, sizeof(int64_t)))
						return ParserException(ParserError::BadInteger);

					Current->Value = Var::Integer(OS::CPU::ToEndianness(OS::CPU::Endian::Little, Integer));
					break;
				}
				case VarType::Number:
				{
					double Number = 0.0;
					if (!Callback((uint8_t*)&Number, sizeof(double)))
						return ParserException(ParserError::BadDouble);

					Current->Value = Var::Number(OS::CPU::ToEndianness(OS::CPU::Endian::Little, Number));
					break;
				}
				case VarType::Decimal:
				{
					uint16_t Size = 0;
					if (!Callback((uint8_t*)&Size, sizeof(uint16_t)))
						return ParserException(ParserError::BadValue);

					String Buffer;
					Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);
					Buffer.resize((size_t)Size);

					if (!Callback((uint8_t*)Buffer.c_str(), (size_t)Size * sizeof(char)))
						return ParserException(ParserError::BadString);

					Current->Value = Var::DecimalString(Buffer);
					break;
				}
				case VarType::Boolean:
				{
					bool Boolean = false;
					if (!Callback((uint8_t*)&Boolean, sizeof(bool)))
						return ParserException(ParserError::BadBoolean);

					Current->Value = Var::Boolean(Boolean);
					break;
				}
				default:
					break;
			}

			return Core::Expectation::Met;
		}
		Schema* Schema::ProcessConversionFromJSONStringOrNumber(void* Base, bool IsDocument)
		{
#ifdef VI_RAPIDJSON
			const char* Buffer = (IsDocument ? ((rapidjson::Document*)Base)->GetString() : ((rapidjson::Value*)Base)->GetString());
			size_t Size = (IsDocument ? ((rapidjson::Document*)Base)->GetStringLength() : ((rapidjson::Value*)Base)->GetStringLength());
			String Text(Buffer, Size);

			if (!Stringify::HasNumber(Text))
				return new Schema(Var::String(Text));

			if (Stringify::HasDecimal(Text))
				return new Schema(Var::DecimalString(Text));

			if (Stringify::HasInteger(Text))
			{
				auto Number = FromString<int64_t>(Text);
				if (Number)
					return new Schema(Var::Integer(*Number));
			}
			else
			{
				auto Number = FromString<double>(Text);
				if (Number)
					return new Schema(Var::Number(*Number));
			}

			return new Schema(Var::String(Text));
#else
			return Var::Set::Undefined();
#endif
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
							Current->Set(Name, ProcessConversionFromJSONStringOrNumber(&It->value, false));
							break;
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
							if (Size < 2 || *Buffer != PREFIX_BINARY[0] || Buffer[Size - 1] != PREFIX_BINARY[0])
								Current->Push(ProcessConversionFromJSONStringOrNumber((void*)It, false));
							else
								Current->Push(Var::Binary((uint8_t*)Buffer + 1, Size - 2));
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
			uint32_t Id = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Current->Key.empty() ? (uint32_t)-1 : (uint32_t)Map->at(Current->Key));
			Callback(VarForm::Dummy, std::string_view((const char*)&Id, sizeof(uint32_t)));
			Callback(VarForm::Dummy, std::string_view((const char*)&Current->Value.Type, sizeof(VarType)));

			switch (Current->Value.Type)
			{
				case VarType::Object:
				case VarType::Array:
				{
					uint32_t Count = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint32_t)(Current->Nodes ? Current->Nodes->size() : 0));
					Callback(VarForm::Dummy, std::string_view((const char*)&Count, sizeof(uint32_t)));
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
					Callback(VarForm::Dummy, std::string_view((const char*)&Size, sizeof(uint32_t)));
					Callback(VarForm::Dummy, std::string_view(Current->Value.GetString().data(), Size * sizeof(char)));
					break;
				}
				case VarType::Decimal:
				{
					String Number = ((Decimal*)Current->Value.Value.Pointer)->ToString();
					uint16_t Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint16_t)Number.size());
					Callback(VarForm::Dummy, std::string_view((const char*)&Size, sizeof(uint16_t)));
					Callback(VarForm::Dummy, std::string_view(Number.c_str(), (size_t)Size * sizeof(char)));
					break;
				}
				case VarType::Integer:
				{
					int64_t Value = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Current->Value.Value.Integer);
					Callback(VarForm::Dummy, std::string_view((const char*)&Value, sizeof(int64_t)));
					break;
				}
				case VarType::Number:
				{
					double Value = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Current->Value.Value.Number);
					Callback(VarForm::Dummy, std::string_view((const char*)&Value, sizeof(double)));
					break;
				}
				case VarType::Boolean:
				{
					Callback(VarForm::Dummy, std::string_view((const char*)&Current->Value.Value.Boolean, sizeof(bool)));
					break;
				}
				default:
					break;
			}
		}
		void Schema::GenerateNamingTable(const Schema* Current, UnorderedMap<String, size_t>* Map, size_t& Index)
		{
			if (!Current->Key.empty())
			{
				auto M = Map->find(Current->Key);
				if (M == Map->end())
					Map->insert({ Current->Key, Index++ });
			}

			if (!Current->Nodes)
				return;

			for (auto Schema : *Current->Nodes)
				GenerateNamingTable(Schema, Map, Index);
		}
	}
}
#pragma warning(pop)
