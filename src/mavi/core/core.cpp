#include "core.h"
#include "../network/http.h"
#include <cctype>
#include <ctime>
#include <thread>
#include <functional>
#include <iostream>
#include <csignal>
#include <bitset>
#include <sys/stat.h>
#include <rapidxml.hpp>
#include <json/document.h>
#include <tinyfiledialogs.h>
#include <concurrentqueue.h>
#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4267)
#include <backward.hpp>
#ifdef VI_USE_FCTX
#include <fcontext.h>
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
#ifndef VI_USE_FCTX
#include <ucontext.h>
#endif
#endif
#ifdef VI_HAS_SDL2
#include <SDL2/SDL.h>
#endif
#ifdef VI_HAS_ZLIB
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
				return "0";
			default:
				return Background ? "40" : "107";
		}
	}
}
#endif
namespace
{
	struct PrettyToken
	{
		Mavi::Core::StdColor Color;
		const char* Token;
		char First;
		size_t Size;

		PrettyToken(Mavi::Core::StdColor BaseColor, const char* Name) : Color(BaseColor), Token(Name), First(*Token), Size(strlen(Name))
		{
		}
	};

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
	void GetDateTime(time_t Time, char* Date, size_t Size)
	{
		tm DateTime { };
		if (!LocalTime(&Time, &DateTime))
			strncpy(Date, "1970-01-01 00:00:00", Size);
		else
			strftime(Date, Size, "%Y-%m-%d %H:%M:%S", &DateTime);
	}
	void GetLocation(Mavi::Core::StringStream& Stream, const char* Indent, const backward::ResolvedTrace::SourceLoc& Location, void* Address = nullptr)
	{
		if (!Location.filename.empty())
			Stream << Indent << "source \"" << Location.filename << "\", line " << Location.line << ", in " << Location.function;
		else
			Stream << Indent << "source ?, line " << Location.line << ", in " << Location.function;

		if (Address != nullptr)
			Stream << " 0x" << Address << "";
		else
			Stream << " nullptr";
		Stream << "\n";
	}
	Mavi::Core::String GetStack(backward::StackTrace& Source)
	{
		size_t ThreadId = Source.thread_id();
		backward::TraceResolver Resolver;
		Resolver.load_stacktrace(Source);

		Mavi::Core::StringStream Stream;
		Stream << "stack trace (most recent call last)" << (ThreadId ? " in thread " : ":\n");
		if (ThreadId)
			Stream << ThreadId << ":\n";

		for (size_t TraceIdx = Source.size(); TraceIdx > 0; --TraceIdx)
		{
			backward::ResolvedTrace Trace = Resolver.resolve(Source[TraceIdx - 1]);
			Stream << "#" << Trace.idx;

			bool Indentation = true;
			if (Trace.source.filename.empty())
			{
				if (!Trace.object_filename.empty())
					Stream << "   object \"" << Trace.object_filename << "\", at 0x" << Trace.addr << ", in " << Trace.object_function << "\n";
				else
					Stream << "   object ?, at 0x" << Trace.addr << ", in " << Trace.object_function << "\n";
				Indentation = false;
			}

			for (size_t InlineIdx = Trace.inliners.size(); InlineIdx > 0; --InlineIdx)
			{
				if (!Indentation)
					Stream << "   ";

				const backward::ResolvedTrace::SourceLoc& Location = Trace.inliners[InlineIdx - 1];
				GetLocation(Stream, " | ", Location);
				Indentation = false;
			}

			if (Trace.source.filename.empty())
				continue;

			if (!Indentation)
				Stream << "   ";

			GetLocation(Stream, "   ", Trace.source, Trace.addr);
		}

		Mavi::Core::String Out(Stream.str());
		return Out.substr(0, Out.size() - 1);
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
		int Name[2] { M1, M2 };
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
		typedef moodycamel::ConcurrentQueue<TaskCallback> FastQueue;
		typedef moodycamel::ConsumerToken ReceiveToken;
#ifndef NDEBUG
		struct Measurement
		{
			const char* File = nullptr;
			const char* Function = nullptr;
			void* Id = nullptr;
			uint64_t Threshold = 0;
			uint64_t Time = 0;
			int Line = 0;

			void NotifyOfOverConsumption()
			{
				if (true || Threshold == (uint64_t)Core::Timings::Infinite)
					return;

				uint64_t Delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - Time;
				if (Delta <= Threshold)
					return;

				Core::String BackTrace = OS::Timing::GetMeasureTrace();
				VI_WARN("[stall] operation took %" PRIu64 " ms (%" PRIu64 " us), back trace %s", Delta / 1000, Delta, BackTrace.c_str());
			}
		};
#endif
		struct ConcurrentQueuePtr
		{
			Core::OrderedMap<std::chrono::microseconds, Timeout> Timers;
			std::condition_variable Notify;
			std::mutex Update;
			FastQueue Tasks;
		};

		struct Cocontext
		{
#ifdef VI_USE_FCTX
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
#ifndef VI_USE_FCTX
#ifdef VI_MICROSOFT
				Context = ConvertThreadToFiber(nullptr);
				Main = true;
#endif
#endif
			}
			Cocontext(Costate* State)
			{
#ifdef VI_USE_FCTX
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
				makecontext(&Context, (void(*)())&Costate::ExecutionEntry, 2, X, Y);
#endif
			}
			~Cocontext()
			{
#ifdef VI_USE_FCTX
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

		DebugAllocator::TracingBlock::TracingBlock() : Time(0), Size(0), Active(false)
		{
		}
		DebugAllocator::TracingBlock::TracingBlock(const char* NewTypeName, MemoryContext&& NewOrigin, time_t NewTime, size_t NewSize, bool IsActive, bool IsStatic) : TypeName(NewTypeName ? NewTypeName : "void"), Origin(std::move(NewOrigin)), Time(NewTime), Size(NewSize), Active(IsActive), Static(IsStatic)
		{
		}

		void* DebugAllocator::Allocate(MemoryContext&& Origin, size_t Size) noexcept
		{
			void* Address = malloc(Size);
			VI_ASSERT(Address != nullptr, nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)Size);

			std::unique_lock<std::recursive_mutex> Unique(Mutex);
			Blocks[Address] = TracingBlock(Origin.TypeName, std::move(Origin), time(nullptr), Size, true, false);
			return Address;
		}
		void DebugAllocator::Free(void* Address) noexcept
		{
			std::unique_lock<std::recursive_mutex> Unique(Mutex);
			auto It = Blocks.find(Address);
			VI_ASSERT_V(It != Blocks.end() && It->second.Active, "cannot free memory that was not allocated by this allocator at 0x%" PRIXPTR, Address);

			Blocks.erase(It);
			free(Address);
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

			VI_ASSERT_V(It == Blocks.end() || !It->second.Active, "cannot watch memory that is already being tracked at 0x%" PRIXPTR, Address);
			Blocks[Address] = TracingBlock(Origin.TypeName, std::move(Origin), time(nullptr), sizeof(void*), false, false);
		}
		void DebugAllocator::Unwatch(void* Address) noexcept
		{
			std::unique_lock<std::recursive_mutex> Unique(Mutex);
			auto It = Blocks.find(Address);

			VI_ASSERT_V(It != Blocks.end() && !It->second.Active, "address at 0x%" PRIXPTR " cannot be cleared from tracking because it was not allocated by this allocator", Address);
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

			VI_ASSERT(It != Blocks.end(), false, "address at 0x%" PRIXPTR " cannot be used as it was already freed");
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
				bool IsLogActive = OS::IsLogActive();
				if (!IsLogActive)
					OS::SetLogActive(true);

				auto It = Blocks.find(Address);
				if (It != Blocks.end())
				{
					char Date[64];
					GetDateTime(It->second.Time, Date, sizeof(Date));
					OS::Log(4, It->second.Origin.Line, It->second.Origin.Source, "[mem] %saddress at 0x%" PRIXPTR " is active since %s as %s (%" PRIu64 " bytes) at %s()", It->second.Static ? "static " : "", It->first, Date, It->second.TypeName.c_str(), (uint64_t)It->second.Size, It->second.Origin.Function);
				}

				OS::SetLogActive(IsLogActive);
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

				bool IsLogActive = OS::IsLogActive();
				if (!IsLogActive)
					OS::SetLogActive(true);

				size_t TotalMemory = 0;
				for (auto& Item : Blocks)
					TotalMemory += Item.second.Size;

				VI_DEBUG("[mem] %" PRIu64 " addresses are still used (%" PRIu64 " bytes)", (uint64_t)(Blocks.size() - StaticAddresses), (uint64_t)TotalMemory);
				for (auto& Item : Blocks)
				{
					if (Item.second.Static || Item.second.TypeName.find("ontainer_proxy") != std::string::npos || Item.second.TypeName.find("ist_node") != std::string::npos)
						continue;

					char Date[64];
					GetDateTime(Item.second.Time, Date, sizeof(Date));
					OS::Log(4, Item.second.Origin.Line, Item.second.Origin.Source, "[mem] address at 0x%" PRIXPTR " is active since %s as %s (%" PRIu64 " bytes) at %s()", Item.first, Date, Item.second.TypeName.c_str(), (uint64_t)Item.second.Size, Item.second.Origin.Function);
				}

				OS::SetLogActive(IsLogActive);
				return true;
			}
#endif
			return false;
		}
		bool DebugAllocator::FindBlock(void* Address, TracingBlock* Output)
		{
			VI_ASSERT(Address != nullptr, false, "address should not be null");
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

		void* DefaultAllocator::Allocate(MemoryContext&& Origin, size_t Size) noexcept
		{
			void* Address = malloc(Size);
			VI_ASSERT(Address != nullptr, nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)Size);
			return Address;
		}
		void DefaultAllocator::Free(void* Address) noexcept
		{
			free(Address);
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

		PoolAllocator::PoolAllocator(uint64_t MinimalLifeTimeMs, size_t MaxElementsPerAllocation, size_t ElementsReducingBaseBytes, double ElementsReducingFactorRate) : MinimalLifeTime(MinimalLifeTimeMs), ElementsReducingFactor(ElementsReducingFactorRate), ElementsReducingBase(ElementsReducingBaseBytes), ElementsPerAllocation(MaxElementsPerAllocation)
		{
			VI_ASSERT_V(ElementsPerAllocation > 0, "elements count per allocation should be greater then zero");
			VI_ASSERT_V(ElementsReducingFactor > 1.0, "elements reducing factor should be greater then zero");
			VI_ASSERT_V(ElementsReducingBase > 0, "elements reducing base should be greater then zero");
		}
		PoolAllocator::~PoolAllocator() noexcept
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
		void* PoolAllocator::Allocate(MemoryContext&&, size_t Size) noexcept
		{
			std::unique_lock<std::recursive_mutex> Unique(Mutex);
			auto* Cache = GetPageCache(Size);
			if (!Cache)
				return nullptr;

			PageAddress* Address = Cache->Addresses.back();
			Cache->Addresses.pop_back();
			return Address->Address;
		}
		void PoolAllocator::Free(void* Address) noexcept
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
		void PoolAllocator::Transfer(Unique<void> Address, MemoryContext&& Origin, size_t Size) noexcept
		{
		}
		void PoolAllocator::Watch(MemoryContext&& Origin, void* Address) noexcept
		{
		}
		void PoolAllocator::Unwatch(void* Address) noexcept
		{
		}
		void PoolAllocator::Finalize() noexcept
		{
		}
		bool PoolAllocator::IsValid(void* Address) noexcept
		{
			return true;
		}
		bool PoolAllocator::IsFinalizable() noexcept
		{
			return false;
		}
		PoolAllocator::PageCache* PoolAllocator::GetPageCache(size_t Size)
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
			VI_ASSERT(Cache != nullptr, nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)PageSize);

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
		int64_t PoolAllocator::GetClock()
		{
			return (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		}
		size_t PoolAllocator::GetElementsCount(PageGroup& Page, size_t Size)
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

		void Memory::Initialize()
		{
			if (!Allocations)
				Allocations = new std::unordered_map<void*, std::pair<MemoryContext, size_t>>();

			if (!Mutex)
				Mutex = new std::mutex();
		}
		void Memory::Uninitialize()
		{
			if (Allocations != nullptr)
			{
				delete Allocations;
				Allocations = nullptr;
			}

			if (Mutex != nullptr)
			{
				delete Mutex;
				Mutex = nullptr;
			}
		}
		void* Memory::Malloc(size_t Size) noexcept
		{
			return MallocContext(Size, MemoryContext());
		}
		void* Memory::MallocContext(size_t Size, MemoryContext&& Origin) noexcept
		{
			VI_ASSERT(Size > 0, nullptr, "cannot allocate zero bytes");
			if (Base != nullptr)
				return Base->Allocate(std::move(Origin), Size);

			void* Address = malloc(Size);
			Initialize();

			std::unique_lock<std::mutex> Unique(*Mutex);
			auto& Item = (*Allocations)[Address];
			Item.first = std::move(Origin);
			Item.second = Size;
			return Address;
		}
		void Memory::Free(void* Address) noexcept
		{
			if (!Address)
				return;

			if (Base != nullptr)
				return Base->Free(Address);

			if (Allocations != nullptr)
			{
				std::unique_lock<std::mutex> Unique(*Mutex);
				Allocations->erase(Address);
				free(Address);
			}
		}
		void Memory::Watch(void* Address, MemoryContext&& Origin) noexcept
		{
			VI_ASSERT_V(Base != nullptr, "allocator should be set");
			VI_ASSERT_V(Address != nullptr, "address should be set");
			Base->Watch(std::move(Origin), Address);
		}
		void Memory::Unwatch(void* Address) noexcept
		{
			VI_ASSERT_V(Base != nullptr, "allocator should be set");
			VI_ASSERT_V(Address != nullptr, "address should be set");
			Base->Unwatch(Address);
		}
		void Memory::SetAllocator(Allocator* NewAllocator)
		{
			if (Base != nullptr)
				Base->Finalize();

			Base = NewAllocator;
			if (Base != nullptr && Allocations != nullptr)
			{
				for (auto& Item : *Allocations)
					Base->Transfer(Item.first, MemoryContext(Item.second.first), Item.second.second);
			}
			else
				Uninitialize();
		}
		bool Memory::IsValidAddress(void* Address)
		{
			VI_ASSERT(Base != nullptr, false, "allocator should be set");
			VI_ASSERT(Address != nullptr, false, "address should be set");
			return Base->IsValid(Address);
		}
		Allocator* Memory::GetAllocator()
		{
			return Base;
		}
		std::unordered_map<void*, std::pair<MemoryContext, size_t>>* Memory::Allocations = nullptr;
		std::mutex* Memory::Mutex = nullptr;
		Allocator* Memory::Base = nullptr;

		Coroutine::Coroutine(Costate* Base, const TaskCallback& Procedure) noexcept : State(Coactive::Active), Dead(0), Callback(Procedure), Slave(VI_NEW(Cocontext, Base)), Master(Base)
		{
		}
		Coroutine::Coroutine(Costate* Base, TaskCallback&& Procedure) noexcept : State(Coactive::Active), Dead(0), Callback(std::move(Procedure)), Slave(VI_NEW(Cocontext, Base)), Master(Base)
		{
		}
		Coroutine::~Coroutine() noexcept
		{
			VI_DELETE(Cocontext, Slave);
		}

		Decimal::Decimal() noexcept : Length(0), Sign('\0'), Invalid(true)
		{
		}
		Decimal::Decimal(const char* Value) noexcept : Length(0), Sign('\0'), Invalid(false)
		{
			int Count = 0;
			if (Value[Count] == '+')
			{
				Sign = '+';
				Count++;
			}
			else if (Value[Count] == '-')
			{
				Sign = '-';
				Count++;
			}
			else if (isdigit(Value[Count]))
			{
				Sign = '+';
			}
			else
			{
				Invalid = 1;
				return;
			}

			bool Points = false;
			while (Value[Count] != '\0')
			{
				if (!Points && Value[Count] == '.')
				{
					if (Source.empty())
					{
						Sign = '\0';
						Invalid = 1;
						return;
					}

					Points = true;
					Count++;
				}

				if (isdigit(Value[Count]))
				{
					Source.push_front(Value[Count]);
					Count++;

					if (Points)
						Length++;
				}
				else
				{
					Sign = '\0';
					Source.clear();
					Length = 0;
					Invalid = 1;
					return;
				}
			}

			Unlead();
		}
		Decimal::Decimal(const Core::String& Value) noexcept : Decimal(Value.c_str())
		{
		}
		Decimal::Decimal(int32_t Value) noexcept : Decimal(Core::ToString(Value))
		{
		}
		Decimal::Decimal(uint32_t Value) noexcept : Decimal(Core::ToString(Value))
		{
		}
		Decimal::Decimal(int64_t Value) noexcept : Decimal(Core::ToString(Value))
		{
		}
		Decimal::Decimal(uint64_t Value) noexcept : Decimal(Core::ToString(Value))
		{
		}
		Decimal::Decimal(float Value) noexcept : Decimal(Core::ToString(Value))
		{
		}
		Decimal::Decimal(double Value) noexcept : Decimal(Core::ToString(Value))
		{
		}
		Decimal::Decimal(const Decimal& Value) noexcept : Source(Value.Source), Length(Value.Length), Sign(Value.Sign), Invalid(Value.Invalid)
		{
		}
		Decimal::Decimal(Decimal&& Value) noexcept : Source(std::move(Value.Source)), Length(Value.Length), Sign(Value.Sign), Invalid(Value.Invalid)
		{
		}
		Decimal& Decimal::Truncate(int Precision)
		{
			if (Invalid || Precision < 0)
				return *this;

			if (Length < Precision)
			{
				while (Length < Precision)
				{
					Length++;
					Source.push_front('0');
				}
			}
			else if (Length > Precision)
			{
				while (Length > Precision)
				{
					Length--;
					Source.pop_front();
				}
			}

			return *this;
		}
		Decimal& Decimal::Round(int Precision)
		{
			if (Invalid || Precision < 0)
				return *this;

			if (Length < Precision)
			{
				while (Length < Precision)
				{
					Length++;
					Source.push_front('0');
				}
			}
			else if (Length > Precision)
			{
				char Last;
				while (Length > Precision)
				{
					Last = Source[0];
					Length--;
					Source.pop_front();
				}

				if (CharToInt(Last) >= 5)
				{
					if (Precision != 0)
					{
						Core::String Result = "0.";
						Result.reserve(3 + (size_t)Precision);
						for (int i = 1; i < Precision; i++)
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
			for (int i = (int)Source.size() - 1; i > Length; --i)
			{
				if (Source[i] != '0')
					break;

				Source.pop_back();
			}

			return *this;
		}
		Decimal& Decimal::Untrail()
		{
			if (Invalid || Source.empty())
				return *this;

			while ((Source[0] == '0') && (Length > 0))
			{
				Source.pop_front();
				Length--;
			}

			return *this;
		}
		bool Decimal::IsNaN() const
		{
			return Invalid;
		}
		bool Decimal::IsZero() const
		{
			for (auto& Item : Source)
			{
				if (Item != '0')
					return false;
			}

			return true;
		}
		bool Decimal::IsZeroOrNaN() const
		{
			return Invalid || IsZero();
		}
		bool Decimal::IsPositive() const
		{
			return !Invalid && *this > 0.0;
		}
		bool Decimal::IsNegative() const
		{
			return !Invalid && *this < 0.0;
		}
		double Decimal::ToDouble() const
		{
			if (Invalid)
				return std::nan("");

			double Dec = 1;
			if (Length > 0)
			{
				int Aus = Length;
				while (Aus != 0)
				{
					Dec /= 10;
					Aus--;
				}
			}

			double Var = 0;
			for (auto& Char : Source)
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
		int64_t Decimal::ToInt64() const
		{
			if (Invalid || Source.empty())
				return 0;

			Core::String Result;
			if (Sign == '-')
				Result += Sign;

			int Offset = 0, Size = Length;
			while ((Source[Offset] == '0') && (Size > 0))
			{
				Offset++;
				Size--;
			}

			for (int i = (int)Source.size() - 1; i >= Offset; i--)
			{
				Result += Source[i];
				if ((i == Length) && (i != 0) && Offset != Length)
					break;
			}

			return strtoll(Result.c_str(), nullptr, 10);
		}
		uint64_t Decimal::ToUInt64() const
		{
			if (Invalid || Source.empty())
				return 0;

			Core::String Result;
			int Offset = 0, Size = Length;
			while ((Source[Offset] == '0') && (Size > 0))
			{
				Offset++;
				Size--;
			}

			for (int i = (int)Source.size() - 1; i >= Offset; i--)
			{
				Result += Source[i];
				if ((i == Length) && (i != 0) && Offset != Length)
					break;
			}

			return strtoull(Result.c_str(), nullptr, 10);
		}
		Core::String Decimal::ToString() const
		{
			if (Invalid || Source.empty())
				return "NaN";

			Core::String Result;
			if (Sign == '-')
				Result += Sign;

			int Offset = 0, Size = Length;
			while ((Source[Offset] == '0') && (Size > 0))
			{
				Offset++;
				Size--;
			}

			for (int i = (int)Source.size() - 1; i >= Offset; i--)
			{
				Result += Source[i];
				if ((i == Length) && (i != 0) && Offset != Length)
					Result += '.';
			}

			return Result;
		}
		Core::String Decimal::Exp() const
		{
			if (Invalid)
				return "NaN";

			Core::String Result;
			int Compare = Decimal::CompareNum(*this, Decimal(1));
			if (Compare == 0)
			{
				Result += Sign;
				Result += "1e+0";
			}
			else if (Compare == 1)
			{
				Result += Sign;
				int i = (int)Source.size() - 1;
				Result += Source[i];
				i--;

				if (i > 0)
				{
					Result += '.';
					for (; (i >= (int)Source.size() - 6) && (i >= 0); --i)
						Result += Source[i];
				}
				Result += "e+";
				Result += Core::ToString(Ints() - 1);
			}
			else if (Compare == 2)
			{
				int Exp = 0, Count = (int)Source.size() - 1;
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

					for (int i = Count - 1; (i >= (int)Count - 5) && (i >= 0); --i)
						Result += Source[i];

					Result += "e-";
					Result += Core::ToString(Exp);
				}
			}

			return Result;
		}
		int Decimal::Decimals() const
		{
			return Length;
		}
		int Decimal::Ints() const
		{
			return (int)Source.size() - Length;
		}
		int Decimal::Size() const
		{
			return (int)(sizeof(*this) + Source.size() * sizeof(char));
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
			Invalid = Value.Invalid;

			return *this;
		}
		Decimal& Decimal::operator=(Decimal&& Value) noexcept
		{
			Source = std::move(Value.Source);
			Length = Value.Length;
			Sign = Value.Sign;
			Invalid = Value.Invalid;

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
			if (Invalid || Right.Invalid)
				return false;

			int Check = CompareNum(*this, Right);
			if ((Check == 0) && (Sign == Right.Sign))
				return true;

			return false;
		}
		bool Decimal::operator!=(const Decimal& Right) const
		{
			if (Invalid || Right.Invalid)
				return false;

			return !(*this == Right);
		}
		bool Decimal::operator>(const Decimal& Right) const
		{
			if (Invalid || Right.Invalid)
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
			if (Invalid || Right.Invalid)
				return false;

			return !(*this < Right);
		}
		bool Decimal::operator<(const Decimal& Right) const
		{
			if (Invalid || Right.Invalid)
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
			if (Invalid || Right.Invalid)
				return false;

			return !(*this > Right);
		}
		Decimal operator+(const Decimal& _Left, const Decimal& _Right)
		{
			Decimal Temp;
			if (_Left.Invalid || _Right.Invalid)
				return Temp;

			Decimal Left, Right;
			Left = _Left;
			Right = _Right;

			if (Left.Length > Right.Length)
			{
				while (Left.Length > Right.Length)
				{
					Right.Length++;
					Right.Source.push_front('0');
				}
			}
			else if (Left.Length < Right.Length)
			{
				while (Left.Length < Right.Length)
				{
					Left.Length++;
					Left.Source.push_front('0');
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
					Temp.Invalid = 0;
					return Temp;
				}

				if (Check == 2)
				{
					Temp = Decimal::Subtract(Right, Left);
					Temp.Sign = '-';
					Temp.Length = Left.Length;
					Temp.Unlead();
					Temp.Invalid = 0;
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
					Temp.Invalid = 0;
					return Temp;
				}

				if (Check == 2)
				{
					Temp = Decimal::Subtract(Right, Left);
					Temp.Sign = '+';
					Temp.Length = Left.Length;
					Temp.Unlead();
					Temp.Invalid = 0;
					return Temp;
				}
			}

			if ((Left.Sign == '+') && (Right.Sign == '+'))
			{
				Temp = Decimal::Sum(Left, Right);
				Temp.Sign = '+';
				Temp.Length = Left.Length;
				Temp.Invalid = 0;
				return Temp;
			}

			if ((Left.Sign == '-') && (Right.Sign == '-'))
			{
				Temp = Decimal::Sum(Left, Right);
				Temp.Sign = '-';
				Temp.Length = Left.Length;
				Temp.Invalid = 0;
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
			if (_Left.Invalid || _Right.Invalid)
				return Temp;

			Decimal Left, Right;
			Left = _Left;
			Right = _Right;

			if (Left.Length > Right.Length)
			{
				while (Left.Length > Right.Length)
				{
					Right.Length++;
					Right.Source.push_front('0');
				}
			}
			else if (Left.Length < Right.Length)
			{
				while (Left.Length < Right.Length)
				{
					Left.Length++;
					Left.Source.push_front('0');
				}
			}

			if ((Left.Sign == '+') && (Right.Sign == '-'))
			{
				Temp = Decimal::Sum(Left, Right);
				Temp.Sign = '+';
				Temp.Length = Left.Length;
				Temp.Invalid = 0;
				return Temp;
			}
			if ((Left.Sign == '-') && (Right.Sign == '+'))
			{
				Temp = Decimal::Sum(Left, Right);
				Temp.Sign = '-';
				Temp.Length = Left.Length;
				Temp.Invalid = 0;
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
					Temp.Invalid = 0;
					return Temp;
				}

				if (Check == 2)
				{
					Temp = Decimal::Subtract(Right, Left);
					Temp.Sign = '-';
					Temp.Length = Left.Length;
					Temp.Unlead();
					Temp.Invalid = 0;
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
					Temp.Invalid = 0;
					return Temp;
				}

				if (Check == 2)
				{
					Temp = Decimal::Subtract(Right, Left);
					Temp.Sign = '+';
					Temp.Length = Left.Length;
					Temp.Unlead();
					Temp.Invalid = 0;
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
			if (Left.Invalid || Right.Invalid)
				return Temp;

			Temp = Decimal::Multiply(Left, Right);
			if (((Left.Sign == '-') && (Right.Sign == '-')) || ((Left.Sign == '+') && (Right.Sign == '+')))
				Temp.Sign = '+';
			else
				Temp.Sign = '-';

			Temp.Length = Left.Length + Right.Length;
			Temp.Invalid = 0;
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
			if (Left.Invalid || Right.Invalid)
				return Temp;

			Decimal Q, R, D, N, Zero;
			Zero = 0;

			if (Right == Zero)
				return Temp;

			N = (Left > Zero) ? (Left) : (Left * (-1));
			D = (Right > Zero) ? (Right) : (Right * (-1));
			R.Sign = '+';
			R.Invalid = 0;

			while ((N.Length != 0) || (D.Length != 0))
			{
				if (N.Length == 0)
					N.Source.push_front('0');
				else
					N.Length--;

				if (D.Length == 0)
					D.Source.push_front('0');
				else
					D.Length--;
			}

			N.Unlead();
			D.Unlead();

			int DivPrecision = (Left.Length > Right.Length) ? (Left.Length) : (Right.Length);
			for (int i = 0; i < DivPrecision; i++)
				N.Source.push_front('0');

			int Check = Decimal::CompareNum(N, D);
			if (Check == 0)
				Temp.Source.push_front('1');

			if (Check == 2)
				return Zero;

			while (!N.Source.empty())
			{
				R.Source.push_front(*(N.Source.rbegin()));
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
					int QSub = 0;
					int Min = 0;
					int Max = 9;

					while (R >= D)
					{
						int Avg = Max - Min;
						int ModAvg = Avg / 2;
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

					Q.Source.push_front(Decimal::IntToChar(QSub));

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
					Q.Source.push_front('0');
			}

			Temp = Q;
			if (((Left.Sign == '-') && (Right.Sign == '-')) || ((Left.Sign == '+') && (Right.Sign == '+')))
				Temp.Sign = '+';
			else
				Temp.Sign = '-';

			Temp.Length = DivPrecision;
			Temp.Invalid = 0;
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
			if (Left.Invalid || Right.Invalid)
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
			R.Invalid = 0;

			int Check = Decimal::CompareNum(N, D);

			if (Check == 0)
				return Zero;

			if (Check == 2)
				return Left;

			while (!N.Source.empty())
			{
				R.Source.push_front(*(N.Source.rbegin()));
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
					int QSub = 0;
					int Min = 0;
					int Max = 9;

					while (R >= D)
					{
						int Avg = Max - Min;
						int ModAvg = Avg / 2;
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

					Q.Source.push_front(Decimal::IntToChar(QSub));
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
					Q.Source.push_front('0');
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

			Temp.Invalid = 0;
			return Temp;
		}
		Decimal operator%(const Decimal& Left, const int& VRight)
		{
			Decimal Right;
			Right = VRight;
			return Left % Right;
		}
		Decimal Decimal::Divide(const Decimal& Left, const Decimal& Right, int DivPrecision)
		{
			Decimal Temp;
			Decimal Q, R, D, N, Zero;
			Zero = 0;

			if (Right == Zero)
				return Temp;

			N = (Left > Zero) ? (Left) : (Left * (-1));
			D = (Right > Zero) ? (Right) : (Right * (-1));
			R.Sign = '+';
			R.Invalid = 0;

			while ((N.Length != 0) || (D.Length != 0))
			{
				if (N.Length == 0)
					N.Source.push_front('0');
				else
					N.Length--;

				if (D.Length == 0)
					D.Source.push_front('0');
				else
					D.Length--;
			}

			N.Unlead();
			D.Unlead();

			for (int i = 0; i < DivPrecision; i++)
				N.Source.push_front('0');

			int Check = Decimal::CompareNum(N, D);
			if (Check == 0)
				Temp.Source.push_front('1');

			if (Check == 2)
				return Zero;

			while (!N.Source.empty())
			{
				R.Source.push_front(*(N.Source.rbegin()));
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
					int QSub = 0;
					int Min = 0;
					int Max = 9;

					while (R >= D)
					{
						int Avg = Max - Min;
						int ModAvg = Avg / 2;
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

					Q.Source.push_front(Decimal::IntToChar(QSub));

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
					Q.Source.push_front('0');
			}

			Temp = Q;
			if (((Left.Sign == '-') && (Right.Sign == '-')) || ((Left.Sign == '+') && (Right.Sign == '+')))
				Temp.Sign = '+';
			else
				Temp.Sign = '-';

			Temp.Length = DivPrecision;
			Temp.Invalid = 0;
			Temp.Unlead();

			return Temp;
		}
		Decimal Decimal::NaN()
		{
			Decimal Result;
			Result.Invalid = true;

			return Result;
		}
		Decimal Decimal::Sum(const Decimal& Left, const Decimal& Right)
		{
			Decimal Temp;
			size_t LoopSize = (Left.Source.size() > Right.Source.size() ? Left.Source.size() : Right.Source.size());
			int Carry = 0;

			for (size_t i = 0; i < LoopSize; ++i)
			{
				int Val1, Val2;
				Val1 = (i > Left.Source.size() - 1) ? 0 : CharToInt(Left.Source[i]);
				Val2 = (i > Right.Source.size() - 1) ? 0 : CharToInt(Right.Source[i]);

				int Aus = Val1 + Val2 + Carry;
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
			int Carry = 0;
			int Aus;

			for (size_t i = 0; i < Left.Source.size(); ++i)
			{
				int Val1, Val2;
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
			int Carry = 0;

			for (size_t i = 0; i < Right.Source.size(); ++i)
			{
				for (size_t k = 0; k < i; ++k)
					Temp.Source.push_front('0');

				for (size_t j = 0; j < Left.Source.size(); ++j)
				{
					int Aus = CharToInt(Right.Source[i]) * CharToInt(Left.Source[j]) + Carry;
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
					Temp.Source.push_front('0');
				}

				for (int i = (int)Left.Source.size() - 1; i >= 0; i--)
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
					Temp.Source.push_front('0');
				}

				for (int i = (int)Temp.Source.size() - 1; i >= 0; i--)
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
				for (int i = (int)Left.Source.size() - 1; i >= 0; i--)
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

		Variant::Variant() noexcept : Type(VarType::Undefined), Size(0)
		{
			Value.Pointer = nullptr;
		}
		Variant::Variant(VarType NewType) noexcept : Type(NewType), Size(0)
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
		bool Variant::Deserialize(const Core::String& Text, bool Strict)
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

				Stringify Buffer(&Text);
				if (Buffer.HasNumber())
				{
					if (Buffer.HasDecimal())
						Move(Var::DecimalString(Buffer.R()));
					else if (Buffer.HasInteger())
						Move(Var::Integer(Buffer.ToInt64()));
					else
						Move(Var::Number(Buffer.ToDouble()));

					return true;
				}
			}

			if (Text.size() > 2 && Text.front() == PREFIX_BINARY[0] && Text.back() == PREFIX_BINARY[0])
			{
				Move(Var::Binary(Compute::Codec::Bep45Decode(Core::String(Text.substr(1).c_str(), Text.size() - 2))));
				return true;
			}

			Move(Var::String(Text));
			return true;
		}
		Core::String Variant::Serialize() const
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
					return Core::String(GetString(), GetSize());
				case VarType::Binary:
					return PREFIX_BINARY + Compute::Codec::Bep45Encode(Core::String(GetString(), GetSize())) + PREFIX_BINARY;
				case VarType::Decimal:
				{
					auto* Data = ((Decimal*)Value.Pointer);
					if (Data->IsNaN())
						return PREFIX_ENUM "null" PREFIX_ENUM;

					return Data->ToString();
				}
				case VarType::Integer:
					return ToString(Value.Integer);
				case VarType::Number:
					return ToString(Value.Number);
				case VarType::Boolean:
					return Value.Boolean ? "true" : "false";
				default:
					return "";
			}
		}
		Core::String Variant::GetBlob() const
		{
			if (Type == VarType::String || Type == VarType::Binary)
				return Core::String(GetString(), Size);

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

			return Size <= GetMaxSmallStringSize() ? Value.String : Value.Pointer;
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
				return Stringify(GetString(), GetSize()).ToInt64();

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
				return Stringify(GetString(), GetSize()).ToDouble();

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

			return GetSize() > 0;
		}
		VarType Variant::GetType() const
		{
			return Type;
		}
		size_t Variant::GetSize() const
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
					return Size;
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
			return !IsEmpty();
		}
		bool Variant::IsString(const char* Text) const
		{
			VI_ASSERT(Text != nullptr, false, "text should be set");
			const char* Other = GetString();
			if (Other == Text)
				return true;

			return strcmp(Other, Text) == 0;
		}
		bool Variant::IsObject() const
		{
			return Type == VarType::Object || Type == VarType::Array;
		}
		bool Variant::IsEmpty() const
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
					return Size == 0;
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
					size_t Size = GetSize();
					if (Size != Other.GetSize())
						return false;

					const char* Src1 = GetString();
					const char* Src2 = Other.GetString();
					if (!Src1 || !Src2)
						return false;

					return strncmp(Src1, Src2, sizeof(char) * Size) == 0;
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
			Size = Other.Size;

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
					size_t StringSize = sizeof(char) * (Size + 1);
					if (Size > GetMaxSmallStringSize())
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
			Size = Other.Size;

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
					if (Size <= GetMaxSmallStringSize())
						memcpy((void*)GetString(), Other.GetString(), sizeof(char) * (Size + 1));
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
			Other.Size = 0;
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
					if (!Value.Pointer || Size <= GetMaxSmallStringSize())
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

		Timeout::Timeout(const SeqTaskCallback& NewCallback, const std::chrono::microseconds& NewTimeout, TaskId NewId, bool NewAlive, Difficulty NewType) noexcept : Expires(NewTimeout), Callback(NewCallback), Type(NewType), Id(NewId), Invocations(0), Alive(NewAlive)
		{
		}
		Timeout::Timeout(SeqTaskCallback&& NewCallback, const std::chrono::microseconds& NewTimeout, TaskId NewId, bool NewAlive, Difficulty NewType) noexcept : Expires(NewTimeout), Callback(std::move(NewCallback)), Type(NewType), Id(NewId), Invocations(0), Alive(NewAlive)
		{
		}
		Timeout::Timeout(const Timeout& Other) noexcept : Expires(Other.Expires), Callback(Other.Callback), Type(Other.Type), Id(Other.Id), Invocations(Other.Invocations), Alive(Other.Alive)
		{
		}
		Timeout::Timeout(Timeout&& Other) noexcept : Expires(Other.Expires), Callback(std::move(Other.Callback)), Type(Other.Type), Id(Other.Id), Invocations(Other.Invocations), Alive(Other.Alive)
		{
		}
		Timeout& Timeout::operator= (const Timeout& Other) noexcept
		{
			Callback = Other.Callback;
			Expires = Other.Expires;
			Id = Other.Id;
			Alive = Other.Alive;
			Invocations = Other.Invocations;
			Type = Other.Type;
			return *this;
		}
		Timeout& Timeout::operator= (Timeout&& Other) noexcept
		{
			Callback = std::move(Other.Callback);
			Expires = Other.Expires;
			Id = Other.Id;
			Alive = Other.Alive;
			Invocations = Other.Invocations;
			Type = Other.Type;
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
		Core::String DateTime::Format(const Core::String& Value)
		{
			if (DateRebuild)
				Rebuild();

			char Buffer[CHUNK_SIZE];
			strftime(Buffer, sizeof(Buffer), Value.c_str(), &DateValue);
			return Buffer;
		}
		Core::String DateTime::Date(const Core::String& Value)
		{
			auto Offset = std::chrono::system_clock::to_time_t(std::chrono::system_clock::time_point(Time));
			if (DateRebuild)
				Rebuild();

			struct tm T;
			if (!LocalTime(&Offset, &T))
				return Value;

			T.tm_mon++;
			T.tm_year += 1900;

			return Stringify(Value).Replace("{s}", T.tm_sec < 10 ? Form("0%i", T.tm_sec).R() : Core::ToString(T.tm_sec)).Replace("{m}", T.tm_min < 10 ? Form("0%i", T.tm_min).R() : Core::ToString(T.tm_min)).Replace("{h}", Core::ToString(T.tm_hour)).Replace("{D}", Core::ToString(T.tm_yday)).Replace("{MD}", T.tm_mday < 10 ? Form("0%i", T.tm_mday).R() : Core::ToString(T.tm_mday)).Replace("{WD}", Core::ToString(T.tm_wday + 1)).Replace("{M}", T.tm_mon < 10 ? Form("0%i", T.tm_mon).R() : Core::ToString(T.tm_mon)).Replace("{Y}", Core::ToString(T.tm_year)).R();
		}
		Core::String DateTime::Iso8601()
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
		Core::String DateTime::FetchWebDateGMT(int64_t TimeStamp)
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
		Core::String DateTime::FetchWebDateTime(int64_t TimeStamp)
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

		Stringify::Stringify() noexcept : Deletable(true)
		{
			Base = VI_NEW(Core::String);
		}
		Stringify::Stringify(int Value) noexcept : Deletable(true)
		{
			Base = VI_NEW(Core::String, Core::ToString(Value));
		}
		Stringify::Stringify(unsigned int Value) noexcept : Deletable(true)
		{
			Base = VI_NEW(Core::String, Core::ToString(Value));
		}
		Stringify::Stringify(int64_t Value) noexcept : Deletable(true)
		{
			Base = VI_NEW(Core::String, Core::ToString(Value));
		}
		Stringify::Stringify(uint64_t Value) noexcept : Deletable(true)
		{
			Base = VI_NEW(Core::String, Core::ToString(Value));
		}
		Stringify::Stringify(float Value) noexcept : Deletable(true)
		{
			Base = VI_NEW(Core::String, Core::ToString(Value));
		}
		Stringify::Stringify(double Value) noexcept : Deletable(true)
		{
			Base = VI_NEW(Core::String, Core::ToString(Value));
		}
		Stringify::Stringify(long double Value) noexcept : Deletable(true)
		{
			Base = VI_NEW(Core::String, Core::ToString(Value));
		}
#ifdef VI_HAS_FAST_MEMORY
		Stringify::Stringify(const std::string& Value) noexcept : Deletable(true)
		{
			Base = VI_NEW(Core::String, Core::Copy<Core::String>(Value));
		}
#endif
		Stringify::Stringify(Core::String&& Buffer) noexcept : Deletable(true)
		{
			Base = VI_NEW(Core::String, std::move(Buffer));
		}
		Stringify::Stringify(const Core::String& Buffer) noexcept : Deletable(true)
		{
			Base = VI_NEW(Core::String, Buffer);
		}
		Stringify::Stringify(Core::String* Buffer) noexcept
		{
			Deletable = (!Buffer);
			Base = (Deletable ? VI_NEW(Core::String) : Buffer);
		}
		Stringify::Stringify(const Core::String* Buffer) noexcept
		{
			Deletable = (!Buffer);
			Base = (Deletable ? VI_NEW(Core::String) : (Core::String*)Buffer);
		}
		Stringify::Stringify(const char* Buffer) noexcept : Deletable(true)
		{
			if (Buffer != nullptr)
				Base = VI_NEW(Core::String, Buffer);
			else
				Base = VI_NEW(Core::String);
		}
		Stringify::Stringify(const char* Buffer, size_t Length) noexcept : Deletable(true)
		{
			if (Buffer != nullptr)
				Base = VI_NEW(Core::String, Buffer, Length);
			else
				Base = VI_NEW(Core::String);
		}
		Stringify::Stringify(Stringify&& Value) noexcept : Base(Value.Base), Deletable(Value.Deletable)
		{
			Value.Base = nullptr;
			Value.Deletable = false;
		}
		Stringify::Stringify(const Stringify& Value) noexcept : Deletable(true)
		{
			if (Value.Base != nullptr)
				Base = VI_NEW(Core::String, *Value.Base);
			else
				Base = VI_NEW(Core::String);
		}
		Stringify::~Stringify() noexcept
		{
			if (Deletable)
				VI_DELETE(basic_string, Base);
		}
		Stringify& Stringify::EscapePrint()
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			for (size_t i = 0; i < Base->size(); i++)
			{
				if (Base->at(i) != '%')
					continue;

				if (i + 1 < Base->size())
				{
					if (Base->at(i + 1) != '%')
					{
						Base->insert(Base->begin() + i, '%');
						i++;
					}
				}
				else
				{
					Base->append(1, '%');
					i++;
				}
			}

			return *this;
		}
		Stringify& Stringify::Escape()
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			for (size_t i = 0; i < Base->size(); i++)
			{
				char& V = Base->at(i);
				if (V == '\"')
				{
					if (i > 0 && Base->at(i - 1) == '\\')
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

				Base->insert(Base->begin() + i, '\\');
				i++;
			}

			return *this;
		}
		Stringify& Stringify::Unescape()
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			for (size_t i = 0; i < Base->size(); i++)
			{
				if (Base->at(i) != '\\' || i + 1 >= Base->size())
					continue;

				char& V = Base->at(i + 1);
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

				Base->erase(Base->begin() + i);
			}

			return *this;
		}
		Stringify& Stringify::Reserve(size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Count > 0, *this, "count should be greater than Zero");

			Base->reserve(Base->capacity() + Count);
			return *this;
		}
		Stringify& Stringify::Resize(size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			Base->resize(Count);
			return *this;
		}
		Stringify& Stringify::Resize(size_t Count, char Char)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Count > 0, *this, "count should be greater than Zero");

			Base->resize(Count, Char);
			return *this;
		}
		Stringify& Stringify::Clear()
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			Base->clear();
			return *this;
		}
		Stringify& Stringify::ToUpper()
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			std::transform(Base->begin(), Base->end(), Base->begin(), ::toupper);
			return *this;
		}
		Stringify& Stringify::ToLower()
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			std::transform(Base->begin(), Base->end(), Base->begin(), ::tolower);
			return *this;
		}
		Stringify& Stringify::Clip(size_t Length)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Length < Base->size())
				Base->erase(Length, Base->size() - Length);

			return *this;
		}
		Stringify& Stringify::Compress(const char* Tokenbase, const char* NotInBetweenOf, size_t Start)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");

			size_t TokenbaseSize = (Tokenbase ? strlen(Tokenbase) : 0);
			size_t NiboSize = (NotInBetweenOf ? strlen(NotInBetweenOf) : 0);
			char Skip = '\0';

			for (size_t i = Start; i < Base->size(); i++)
			{
				for (size_t j = 0; j < NiboSize; j++)
				{
					char& Next = Base->at(i);
					if (Next == NotInBetweenOf[j])
					{
						Skip = Next;
						++i;
						break;
					}
				}

				while (Skip != '\0' && i < Base->size() && Base->at(i) != Skip)
					++i;

				if (Skip != '\0')
				{
					Skip = '\0';
					if (i >= Base->size())
						break;
				}

				char& Next = Base->at(i);
				if (Next != ' ' && Next != '\r' && Next != '\n' && Next != '\t')
					continue;

				bool Removable = false;
				if (i > 0)
				{
					Next = Base->at(i - 1);
					for (size_t j = 0; j < TokenbaseSize; j++)
					{
						if (Next == Tokenbase[j])
						{
							Removable = true;
							break;
						}
					}
				}

				if (!Removable && i + 1 < Base->size())
				{
					Next = Base->at(i + 1);
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
					Base->erase(Base->begin() + i--);
				else
					(*Base)[i] = ' ';
			}

			return *this;
		}
		Stringify& Stringify::ReplaceOf(const char* Chars, const char* To, size_t Start)
		{
			VI_ASSERT(Chars != nullptr && Chars[0] != '\0' && To != nullptr, *this, "match list and replacer should not be empty");

			Stringify::Settle Result { };
			size_t Offset = Start, ToSize = strlen(To);
			while ((Result = FindOf(Chars, Offset)).Found)
			{
				EraseOffsets(Result.Start, Result.End);
				Insert(To, Result.Start);
				Offset = Result.Start + ToSize;
			}

			return *this;
		}
		Stringify& Stringify::ReplaceNotOf(const char* Chars, const char* To, size_t Start)
		{
			VI_ASSERT(Chars != nullptr && Chars[0] != '\0' && To != nullptr, *this, "match list and replacer should not be empty");

			Stringify::Settle Result {};
			size_t Offset = Start, ToSize = strlen(To);
			while ((Result = FindNotOf(Chars, Offset)).Found)
			{
				EraseOffsets(Result.Start, Result.End);
				Insert(To, Result.Start);
				Offset = Result.Start + ToSize;
			}

			return *this;
		}
		Stringify& Stringify::Replace(const Core::String& From, const Core::String& To, size_t Start)
		{
			VI_ASSERT(!From.empty(), *this, "match should not be empty");

			size_t Offset = Start;
			Stringify::Settle Result { };

			while ((Result = Find(From, Offset)).Found)
			{
				EraseOffsets(Result.Start, Result.End);
				Insert(To, Result.Start);
				Offset = Result.Start + To.size();
			}

			return *this;
		}
		Stringify& Stringify::ReplaceGroups(const Core::String& FromRegex, const Core::String& To)
		{
			Compute::RegexSource Source('(' + FromRegex + ')');
			Compute::Regex::Replace(&Source, To, *Base);
			return *this;
		}
		Stringify& Stringify::Replace(const char* From, const char* To, size_t Start)
		{
			VI_ASSERT(From != nullptr && To != nullptr, *this, "from and to should not be empty");

			size_t Offset = Start;
			size_t Size = strlen(To);
			Stringify::Settle Result { };

			while ((Result = Find(From, Offset)).Found)
			{
				EraseOffsets(Result.Start, Result.End);
				Insert(To, Result.Start);
				Offset = Result.Start + Size;
			}

			return *this;
		}
		Stringify& Stringify::Replace(const char& From, const char& To, size_t Position)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			for (size_t i = Position; i < Base->size(); i++)
			{
				char& C = Base->at(i);
				if (C == From)
					C = To;
			}

			return *this;
		}
		Stringify& Stringify::Replace(const char& From, const char& To, size_t Position, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Base->size() >= (Position + Count), *this, "invalid offset");

			size_t Size = Position + Count;
			for (size_t i = Position; i < Size; i++)
			{
				char& C = Base->at(i);
				if (C == From)
					C = To;
			}

			return *this;
		}
		Stringify& Stringify::ReplacePart(size_t Start, size_t End, const Core::String& Value)
		{
			return ReplacePart(Start, End, Value.c_str());
		}
		Stringify& Stringify::ReplacePart(size_t Start, size_t End, const char* Value)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Start < Base->size(), *this, "invalid start");
			VI_ASSERT(End <= Base->size(), *this, "invalid end");
			VI_ASSERT(Start < End, *this, "start should be less than end");
			VI_ASSERT(Value != nullptr, *this, "replacer should not be empty");

			if (Start == 0)
			{
				if (Base->size() != End)
					Base->assign(Value + Base->substr(End, Base->size() - End));
				else
					Base->assign(Value);
			}
			else if (Base->size() == End)
				Base->assign(Base->substr(0, Start) + Value);
			else
				Base->assign(Base->substr(0, Start) + Value + Base->substr(End, Base->size() - End));

			return *this;
		}
		Stringify& Stringify::ReplaceStartsWithEndsOf(const char* Begins, const char* EndsOf, const Core::String& With, size_t Start)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Begins != nullptr && Begins[0] != '\0', *this, "begin should not be empty");
			VI_ASSERT(EndsOf != nullptr && EndsOf[0] != '\0', *this, "end should not be empty");

			size_t BeginsSize = strlen(Begins), EndsOfSize = strlen(EndsOf);
			for (size_t i = Start; i < Base->size(); i++)
			{
				size_t From = i, BeginsOffset = 0;
				while (BeginsOffset < BeginsSize && From < Base->size() && Base->at(From) == Begins[BeginsOffset])
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
				while (!Matching && To < Base->size())
				{
					auto& Next = Base->at(To++);
					for (size_t j = 0; j < EndsOfSize; j++)
					{
						if (Next == EndsOf[j])
						{
							Matching = true;
							break;
						}
					}
				}

				if (To >= Base->size())
					Matching = true;

				if (!Matching)
					continue;

				Base->replace(Base->begin() + From - BeginsSize, Base->begin() + To, With);
				i = With.size();
			}

			return *this;
		}
		Stringify& Stringify::ReplaceInBetween(const char* Begins, const char* Ends, const Core::String& With, bool Recursive, size_t Start)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Begins != nullptr && Begins[0] != '\0', *this, "begin should not be empty");
			VI_ASSERT(Ends != nullptr && Ends[0] != '\0', *this, "end should not be empty");
			
			size_t BeginsSize = strlen(Begins), EndsSize = strlen(Ends);
			for (size_t i = Start; i < Base->size(); i++)
			{
				size_t From = i, BeginsOffset = 0;
				while (BeginsOffset < BeginsSize && From < Base->size() && Base->at(From) == Begins[BeginsOffset])
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
				while (To < Base->size())
				{
					if (Base->at(To++) != Ends[EndsOffset])
					{
						if (!Recursive)
							continue;

						size_t Substep = To - 1, Suboffset = 0;
						while (Suboffset < BeginsSize && Substep < Base->size() && Base->at(Substep) == Begins[Suboffset])
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

				if (To > Base->size())
					To = Base->size();

				Base->replace(Base->begin() + From - BeginsSize, Base->begin() + To, With);
				i = With.size();
			}

			return *this;
		}
		Stringify& Stringify::ReplaceNotInBetween(const char* Begins, const char* Ends, const Core::String& With, bool Recursive, size_t Start)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Begins != nullptr && Begins[0] != '\0', *this, "begin should not be empty");
			VI_ASSERT(Ends != nullptr && Ends[0] != '\0', *this, "end should not be empty");

			size_t BeginsSize = strlen(Begins), EndsSize = strlen(Ends);
			size_t ReplaceAt = Core::String::npos;

			for (size_t i = Start; i < Base->size(); i++)
			{
				size_t From = i, BeginsOffset = 0;
				while (BeginsOffset < BeginsSize && From < Base->size() && Base->at(From) == Begins[BeginsOffset])
				{
					++From;
					++BeginsOffset;
				}

				size_t Nesting = 1;
				if (BeginsOffset != BeginsSize)
				{
					if (ReplaceAt == Core::String::npos)
						ReplaceAt = i;

					continue;
				}

				if (ReplaceAt != Core::String::npos)
				{
					Base->replace(Base->begin() + ReplaceAt, Base->begin() + i, With);
					From = ReplaceAt + BeginsSize + With.size();
					i = From - BeginsSize;
					ReplaceAt = Core::String::npos;
				}

				size_t To = From, EndsOffset = 0;
				while (To < Base->size())
				{
					if (Base->at(To++) != Ends[EndsOffset])
					{
						if (!Recursive)
							continue;

						size_t Substep = To - 1, Suboffset = 0;
						while (Suboffset < BeginsSize && Substep < Base->size() && Base->at(Substep) == Begins[Suboffset])
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

			if (ReplaceAt == Core::String::npos)
				return *this;

			Base->replace(Base->begin() + ReplaceAt, Base->end(), With);
			return *this;
		}
		Stringify& Stringify::ReplaceParts(Core::Vector<std::pair<Core::String, Stringify::Settle>>& Inout, const Core::String& With, const std::function<char(const Core::String&, char, int)>& Surrounding)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_SORT(Inout.begin(), Inout.end(), [](const std::pair<Core::String, Stringify::Settle>& A, const std::pair<Core::String, Stringify::Settle>& B)
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
					Core::String Replacement = With;
					if (Item.second.Start > 0)
					{
						char Next = Surrounding(Item.first, Base->at(Item.second.Start - 1), -1);
						if (Next != '\0')
							Replacement.insert(Replacement.begin(), Next);
					}

					if (Item.second.End < Base->size())
					{
						char Next = Surrounding(Item.first, Base->at(Item.second.End), 1);
						if (Next != '\0')
							Replacement.push_back(Next);
					}

					ReplacePart(Item.second.Start, Item.second.End, Replacement);
					Offset += (int64_t)Replacement.size() - (int64_t)Size;
					Item.second.End = Item.second.Start + Replacement.size();
				}
				else
				{
					ReplacePart(Item.second.Start, Item.second.End, With);
					Offset += (int64_t)With.size() - (int64_t)Size;
					Item.second.End = Item.second.Start + With.size();
				}
			}

			return *this;
		}
		Stringify& Stringify::ReplaceParts(Core::Vector<Stringify::Settle>& Inout, const Core::String& With, const std::function<char(char, int)>& Surrounding)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");	
			VI_SORT(Inout.begin(), Inout.end(), [](const Stringify::Settle& A, const Stringify::Settle& B)
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
					Core::String Replacement = With;
					if (Item.Start > 0)
					{
						char Next = Surrounding(Base->at(Item.Start - 1), -1);
						if (Next != '\0')
							Replacement.insert(Replacement.begin(), Next);
					}

					if (Item.End < Base->size())
					{
						char Next = Surrounding(Base->at(Item.End), 1);
						if (Next != '\0')
							Replacement.push_back(Next);
					}

					ReplacePart(Item.Start, Item.End, Replacement);
					Offset += (int64_t)Replacement.size() - (int64_t)Size;
					Item.End = Item.Start + Replacement.size();
				}
				else
				{
					ReplacePart(Item.Start, Item.End, With);
					Offset += (int64_t)With.size() - (int64_t)Size;
					Item.End = Item.Start + With.size();
				}
			}

			return *this;
		}
		Stringify& Stringify::RemovePart(size_t Start, size_t End)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Start < Base->size(), *this, "invalid start");
			VI_ASSERT(End <= Base->size(), *this, "invalid end");
			VI_ASSERT(Start < End, *this, "start should be less than end");

			if (Start == 0)
			{
				if (Base->size() != End)
					Base->assign(Base->substr(End, Base->size() - End));
				else
					Base->clear();
			}
			else if (Base->size() == End)
				Base->assign(Base->substr(0, Start));
			else
				Base->assign(Base->substr(0, Start) + Base->substr(End, Base->size() - End));

			return *this;
		}
		Stringify& Stringify::Reverse()
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Base->empty())
				return *this;

			return Reverse(0, Base->size());
		}
		Stringify& Stringify::Reverse(size_t Start, size_t End)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Base->size() >= 2, *this, "length should be at least 2 chars");
			VI_ASSERT(End < Base->size(), *this, "end should be less than length - 1");
			VI_ASSERT(Start < Base->size(), *this, "start should be less than length - 1");
			VI_ASSERT(Start < End, *this, "start should be less than end");

			std::reverse(Base->begin() + Start, Base->begin() + End);
			return *this;
		}
		Stringify& Stringify::Substring(size_t Start)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Start < Base->size())
				Base->assign(Base->substr(Start));
			else
				Base->clear();

			return *this;
		}
		Stringify& Stringify::Substring(size_t Start, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Count > 0 && Start < Base->size())
				Base->assign(Base->substr(Start, Count));
			else
				Base->clear();

			return *this;
		}
		Stringify& Stringify::Substring(const Stringify::Settle& Result)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Result.Found, *this, "result should be found");

			if (Result.Start >= Base->size())
			{
				Base->clear();
				return *this;
			}

			auto Offset = (int64_t)Result.End;
			if (Result.End > Base->size())
				Offset = (int64_t)(Base->size() - Result.Start);

			Offset = (int64_t)Result.Start - Offset;
			Base->assign(Base->substr(Result.Start, (size_t)(Offset < 0 ? -Offset : Offset)));
			return *this;
		}
		Stringify& Stringify::Splice(size_t Start, size_t End)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Start < Base->size(), *this, "result start should be less or equal than length - 1");

			if (End > Base->size())
				End = (Base->size() - Start);

			int64_t Offset = (int64_t)Start - (int64_t)End;
			Base->assign(Base->substr(Start, (size_t)(Offset < 0 ? -Offset : Offset)));
			return *this;
		}
		Stringify& Stringify::Trim()
		{
			return TrimStart().TrimEnd();
		}
		Stringify& Stringify::TrimStart()
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			Base->erase(Base->begin(), std::find_if(Base->begin(), Base->end(), [](int C) -> int
			{
				if (C < -1 || C > 255)
					return 1;

				return std::isspace(C) == 0 ? 1 : 0;
			}));

			return *this;
		}
		Stringify& Stringify::TrimEnd()
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			Base->erase(std::find_if(Base->rbegin(), Base->rend(), [](int C) -> int
			{
				if (C < -1 || C > 255)
					return 1;

				return std::isspace(C) == 0 ? 1 : 0;
			}).base(), Base->end());

			return *this;
		}
		Stringify& Stringify::Fill(const char& Char)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(!Base->empty(), *this, "length should be greater than Zero");

			for (char& i : *Base)
				i = Char;

			return *this;
		}
		Stringify& Stringify::Fill(const char& Char, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(!Base->empty(), *this, "length should be greater than Zero");

			Base->assign(Count, Char);
			return *this;
		}
		Stringify& Stringify::Fill(const char& Char, size_t Start, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(!Base->empty(), *this, "length should be greater than Zero");
			VI_ASSERT(Start <= Base->size(), *this, "start should be less or equal than length");

			if (Start + Count > Base->size())
				Count = Base->size() - Start;

			size_t Size = (Start + Count);
			for (size_t i = Start; i < Size; i++)
				Base->at(i) = Char;

			return *this;
		}
		Stringify& Stringify::Assign(const char* Raw)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Raw != nullptr)
				Base->assign(Raw);
			else
				Base->clear();

			return *this;
		}
		Stringify& Stringify::Assign(const char* Raw, size_t Length)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Raw != nullptr)
				Base->assign(Raw, Length);
			else
				Base->clear();

			return *this;
		}
		Stringify& Stringify::Assign(const Core::String& Raw)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			Base->assign(Raw);
			return *this;
		}
		Stringify& Stringify::Assign(const Core::String& Raw, size_t Start, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Start >= Raw.size() || !Count)
				Base->clear();
			else
				Base->assign(Raw.substr(Start, Count));
			return *this;
		}
		Stringify& Stringify::Assign(const char* Raw, size_t Start, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Raw != nullptr, *this, "assign string should be set");

			Base->assign(Raw);
			return Substring(Start, Count);
		}
		Stringify& Stringify::Append(const char* Raw)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Raw != nullptr, *this, "append string should be set");

			Base->append(Raw);
			return *this;
		}
		Stringify& Stringify::Append(const char& Char)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			Base->append(1, Char);
			return *this;
		}
		Stringify& Stringify::Append(const char& Char, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			Base->append(Count, Char);
			return *this;
		}
		Stringify& Stringify::Append(const Core::String& Raw)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			Base->append(Raw);
			return *this;
		}
		Stringify& Stringify::Append(const char* Raw, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Raw != nullptr, *this, "append string should be set");

			Base->append(Raw, Count);
			return *this;
		}
		Stringify& Stringify::Append(const char* Raw, size_t Start, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Raw != nullptr, *this, "append string should be set");
			VI_ASSERT(Count > 0, *this, "count should be greater than Zero");
			VI_ASSERT(strlen(Raw) >= Start + Count, *this, "offset should be less than append string length");

			Base->append(Raw + Start, Count - Start);
			return *this;
		}
		Stringify& Stringify::Append(const Core::String& Raw, size_t Start, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Count > 0, *this, "count should be greater than Zero");
			VI_ASSERT(Raw.size() >= Start + Count, *this, "offset should be less than append string length");

			Base->append(Raw.substr(Start, Count));
			return *this;
		}
		Stringify& Stringify::fAppend(const char* Format, ...)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Format != nullptr, *this, "format should be set");

			char Buffer[BLOB_SIZE];
			va_list Args;
			va_start(Args, Format);
			int Count = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
			va_end(Args);

			return Append(Buffer, Count);
		}
		Stringify& Stringify::Insert(const Core::String& Raw, size_t Position)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Position >= Base->size())
				Position = Base->size();

			Base->insert(Position, Raw);
			return *this;
		}
		Stringify& Stringify::Insert(const Core::String& Raw, size_t Position, size_t Start, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Position >= Base->size())
				Position = Base->size();

			if (Raw.size() >= Start + Count)
				Base->insert(Position, Raw.substr(Start, Count));

			return *this;
		}
		Stringify& Stringify::Insert(const Core::String& Raw, size_t Position, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Position >= Base->size())
				Position = Base->size();

			if (Count >= Raw.size())
				Count = Raw.size();

			Base->insert(Position, Raw.substr(0, Count));
			return *this;
		}
		Stringify& Stringify::Insert(const char& Char, size_t Position, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Position >= Base->size())
				Position = Base->size();

			Base->insert(Position, Count, Char);
			return *this;
		}
		Stringify& Stringify::Insert(const char& Char, size_t Position)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			if (Position >= Base->size())
				Position = Base->size();

			Base->insert(Base->begin() + Position, Char);
			return *this;
		}
		Stringify& Stringify::Erase(size_t Position)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Position < Base->size(), *this, "position should be less than length");
			Base->erase(Position);
			return *this;
		}
		Stringify& Stringify::Erase(size_t Position, size_t Count)
		{
			VI_ASSERT(Base != nullptr, *this, "cannot parse without context");
			VI_ASSERT(Position < Base->size(), *this, "position should be less than length");
			Base->erase(Position, Count);
			return *this;
		}
		Stringify& Stringify::EraseOffsets(size_t Start, size_t End)
		{
			return Erase(Start, End - Start);
		}
		Stringify& Stringify::Eval(const Core::String& Net, const Core::String& Dir)
		{
			if (Base->empty())
				return *this;

			if (StartsOf("./\\"))
			{
				Core::String Result = Core::OS::Path::Resolve(Base->c_str(), Dir);
				if (!Result.empty())
					Assign(Result);
			}
			else if (Base->front() == '$' && Base->size() > 1)
			{
				const char* Env = std::getenv(Base->c_str() + 1);
				if (!Env)
				{
					VI_WARN("[env] cannot resolve environmental variable\n\t%s", Base->c_str() + 1);
					Base->clear();
				}
				else
					Base->assign(Env);
			}
			else
				Replace("[subnet]", Net);

			return *this;
		}
		Core::Vector<std::pair<Core::String, Stringify::Settle>> Stringify::FindInBetween(const char* Begins, const char* Ends, const char* NotInSubBetweenOf, size_t Offset) const
		{
			Core::Vector<std::pair<Core::String, Stringify::Settle>> Result;
			VI_ASSERT(Begins != nullptr && Begins[0] != '\0', Result, "begin should not be empty");
			VI_ASSERT(Ends != nullptr && Ends[0] != '\0', Result, "end should not be empty");

			size_t BeginsSize = strlen(Begins), EndsSize = strlen(Ends);
			size_t NisboSize = (NotInSubBetweenOf ? strlen(NotInSubBetweenOf) : 0);
			char Skip = '\0';

			for (size_t i = Offset; i < Base->size(); i++)
			{
				for (size_t j = 0; j < NisboSize; j++)
				{
					char& Next = Base->at(i);
					if (Next == NotInSubBetweenOf[j])
					{
						Skip = Next;
						++i;
						break;
					}
				}
				
				while (Skip != '\0' && i < Base->size() && Base->at(i) != Skip)
					++i;

				if (Skip != '\0')
				{
					Skip = '\0';
					if (i >= Base->size())
						break;
				}

				size_t From = i, BeginsOffset = 0;
				while (BeginsOffset < BeginsSize && From < Base->size() && Base->at(From) == Begins[BeginsOffset])
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
				while (To < Base->size())
				{
					if (Base->at(To++) != Ends[EndsOffset])
						continue;
					
					if (++EndsOffset >= EndsSize)
						break;
				}

				i = To;
				if (EndsOffset != EndsSize)
					continue;

				Settle At;
				At.Start = From - BeginsSize;
				At.End = To;
				At.Found = true;

				Result.push_back(std::make_pair(Base->substr(From, (To - EndsSize) - From), std::move(At)));
			}

			return Result;
		}
		Core::Vector<std::pair<Core::String, Stringify::Settle>> Stringify::FindStartsWithEndsOf(const char* Begins, const char* EndsOf, const char* NotInSubBetweenOf, size_t Offset) const
		{
			Core::Vector<std::pair<Core::String, Stringify::Settle>> Result;
			VI_ASSERT(Begins != nullptr && Begins[0] != '\0', Result, "begin should not be empty");
			VI_ASSERT(EndsOf != nullptr && EndsOf[0] != '\0', Result, "end should not be empty");

			size_t BeginsSize = strlen(Begins), EndsOfSize = strlen(EndsOf);
			size_t NisboSize = (NotInSubBetweenOf ? strlen(NotInSubBetweenOf) : 0);
			char Skip = '\0';

			for (size_t i = Offset; i < Base->size(); i++)
			{
				for (size_t j = 0; j < NisboSize; j++)
				{
					char& Next = Base->at(i);
					if (Next == NotInSubBetweenOf[j])
					{
						Skip = Next;
						++i;
						break;
					}
				}

				while (Skip != '\0' && i < Base->size() && Base->at(i) != Skip)
					++i;

				if (Skip != '\0')
				{
					Skip = '\0';
					if (i >= Base->size())
						break;
				}

				size_t From = i, BeginsOffset = 0;
				while (BeginsOffset < BeginsSize && From < Base->size() && Base->at(From) == Begins[BeginsOffset])
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
				while (!Matching && To < Base->size())
				{
					auto& Next = Base->at(To++);
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

				if (To >= Base->size())
					Matching = true;

				if (!Matching)
					continue;

				Settle At;
				At.Start = From - BeginsSize;
				At.End = To;
				At.Found = true;

				Result.push_back(std::make_pair(Base->substr(From, To - From), std::move(At)));
			}

			return Result;
		}
		Stringify::Settle Stringify::ReverseFind(const Core::String& Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			if (Base->empty() || Offset >= Base->size())
				return { Base->size(), Base->size(), false };

			const char* Ptr = Base->c_str() - Offset;
			if (Needle.c_str() > Ptr)
				return { Base->size(), Base->size(), false };

			const char* It = nullptr;
			for (It = Ptr + Base->size() - Needle.size(); It > Ptr; --It)
			{
				if (strncmp(Ptr, Needle.c_str(), Needle.size()) == 0)
				{
					size_t Set = (size_t)(It - Ptr);
					return { Set, Set + Needle.size(), true };
				}
			}

			return { Base->size(), Base->size(), false };
		}
		Stringify::Settle Stringify::ReverseFind(const char* Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			if (Base->empty() || Offset >= Base->size())
				return { Base->size(), Base->size(), false };

			if (!Needle)
				return { Base->size(), Base->size(), false };

			const char* Ptr = Base->c_str() - Offset;
			if (Needle > Ptr)
				return { Base->size(), Base->size(), false };

			const char* It = nullptr;
			size_t Length = strlen(Needle);
			for (It = Ptr + Base->size() - Length; It > Ptr; --It)
			{
				if (strncmp(Ptr, Needle, Length) == 0)
					return { (size_t)(It - Ptr), (size_t)(It - Ptr + Length), true };
			}

			return { Base->size(), Base->size(), false };
		}
		Stringify::Settle Stringify::ReverseFind(const char& Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			if (Base->empty() || Offset >= Base->size())
				return { Base->size(), Base->size(), false };

			size_t Size = Base->size() - Offset;
			for (size_t i = Size; i-- > 0;)
			{
				if (Base->at(i) == Needle)
					return { i, i + 1, true };
			}

			return { Base->size(), Base->size(), false };
		}
		Stringify::Settle Stringify::ReverseFindUnescaped(const char& Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			if (Base->empty() || Offset >= Base->size())
				return { Base->size(), Base->size(), false };

			size_t Size = Base->size() - Offset;
			for (size_t i = Size; i-- > 0;)
			{
				if (Base->at(i) == Needle && ((int64_t)i - 1 < 0 || Base->at(i - 1) != '\\'))
					return { i, i + 1, true };
			}

			return { Base->size(), Base->size(), false };
		}
		Stringify::Settle Stringify::ReverseFindOf(const Core::String& Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			if (Base->empty() || Offset >= Base->size())
				return { Base->size(), Base->size(), false };

			size_t Size = Base->size() - Offset;
			for (size_t i = Size; i-- > 0;)
			{
				for (char k : Needle)
				{
					if (Base->at(i) == k)
						return { i, i + 1, true };
				}
			}

			return { Base->size(), Base->size(), false };
		}
		Stringify::Settle Stringify::ReverseFindOf(const char* Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			if (Base->empty() || Offset >= Base->size())
				return { Base->size(), Base->size(), false };

			if (!Needle)
				return { Base->size(), Base->size(), false };

			size_t Length = strlen(Needle);
			size_t Size = Base->size() - Offset;
			for (size_t i = Size; i-- > 0;)
			{
				for (size_t k = 0; k < Length; k++)
				{
					if (Base->at(i) == Needle[k])
						return { i, i + 1, true };
				}
			}

			return { Base->size(), Base->size(), false };
		}
		Stringify::Settle Stringify::Find(const Core::String& Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			if (Base->empty() || Offset >= Base->size())
				return { Base->size(), Base->size(), false };

			const char* It = strstr(Base->c_str() + Offset, Needle.c_str());
			if (It == nullptr)
				return { Base->size(), Base->size(), false };

			size_t Set = (size_t)(It - Base->c_str());
			return { Set, Set + Needle.size(), true };
		}
		Stringify::Settle Stringify::Find(const char* Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			VI_ASSERT(Needle != nullptr, Stringify::Settle(), "needle should be set");

			if (Base->empty() || Offset >= Base->size())
				return { Base->size(), Base->size(), false };

			const char* It = strstr(Base->c_str() + Offset, Needle);
			if (It == nullptr)
				return { Base->size(), Base->size(), false };

			size_t Set = (size_t)(It - Base->c_str());
			return { Set, Set + strlen(Needle), true };
		}
		Stringify::Settle Stringify::Find(const char& Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			for (size_t i = Offset; i < Base->size(); i++)
			{
				if (Base->at(i) == Needle)
					return { i, i + 1, true };
			}

			return { Base->size(), Base->size(), false };
		}
		Stringify::Settle Stringify::FindUnescaped(const char& Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			for (size_t i = Offset; i < Base->size(); i++)
			{
				if (Base->at(i) == Needle && ((int64_t)i - 1 < 0 || Base->at(i - 1) != '\\'))
					return { i, i + 1, true };
			}

			return { Base->size(), Base->size(), false };
		}
		Stringify::Settle Stringify::FindOf(const Core::String& Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			for (size_t i = Offset; i < Base->size(); i++)
			{
				for (char k : Needle)
				{
					if (Base->at(i) == k)
						return { i, i + 1, true };
				}
			}

			return { Base->size(), Base->size(), false };
		}
		Stringify::Settle Stringify::FindOf(const char* Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			VI_ASSERT(Needle != nullptr, Stringify::Settle(), "needle should be set");

			size_t Length = strlen(Needle);
			for (size_t i = Offset; i < Base->size(); i++)
			{
				for (size_t k = 0; k < Length; k++)
				{
					if (Base->at(i) == Needle[k])
						return { i, i + 1, true };
				}
			}

			return { Base->size(), Base->size(), false };
		}
		Stringify::Settle Stringify::FindNotOf(const Core::String& Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			for (size_t i = Offset; i < Base->size(); i++)
			{
				bool Result = false;
				for (char k : Needle)
				{
					if (Base->at(i) == k)
					{
						Result = true;
						break;
					}
				}

				if (!Result)
					return { i, i + 1, true };
			}

			return { Base->size(), Base->size(), false };
		}
		Stringify::Settle Stringify::FindNotOf(const char* Needle, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, Stringify::Settle(), "cannot parse without context");
			VI_ASSERT(Needle != nullptr, Stringify::Settle(), "needle should be set");

			size_t Length = strlen(Needle);
			for (size_t i = Offset; i < Base->size(); i++)
			{
				bool Result = false;
				for (size_t k = 0; k < Length; k++)
				{
					if (Base->at(i) == Needle[k])
					{
						Result = true;
						break;
					}
				}

				if (!Result)
					return { i, i + 1, true };
			}

			return { Base->size(), Base->size(), false };
		}
		bool Stringify::IsPrecededBy(size_t At, const char* Of) const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			VI_ASSERT(Of != nullptr, false, "tokenbase should be set");

			if (!At || At - 1 >= Base->size())
				return false;

			size_t Size = strlen(Of);
			char& Next = Base->at(At - 1);
			for (size_t i = 0; i < Size; i++)
			{
				if (Next == Of[i])
					return true;
			}

			return false;
		}
		bool Stringify::IsFollowedBy(size_t At, const char* Of) const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			VI_ASSERT(Of != nullptr, false, "tokenbase should be set");

			if (At + 1 >= Base->size())
				return false;

			size_t Size = strlen(Of);
			char& Next = Base->at(At + 1);
			for (size_t i = 0; i < Size; i++)
			{
				if (Next == Of[i])
					return true;
			}

			return false;
		}
		bool Stringify::StartsWith(const Core::String& Value, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			if (Base->size() < Value.size())
				return false;

			for (size_t i = Offset; i < Value.size(); i++)
			{
				if (Value[i] != Base->at(i))
					return false;
			}

			return true;
		}
		bool Stringify::StartsWith(const char* Value, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			VI_ASSERT(Value != nullptr, false, "value should be set");

			size_t Length = strlen(Value);
			if (Base->size() < Length)
				return false;

			for (size_t i = Offset; i < Length; i++)
			{
				if (Value[i] != Base->at(i))
					return false;
			}

			return true;
		}
		bool Stringify::StartsOf(const char* Value, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			VI_ASSERT(Value != nullptr, false, "value should be set");

			size_t Length = strlen(Value);
			if (Offset >= Base->size())
				return false;

			for (size_t j = 0; j < Length; j++)
			{
				if (Base->at(Offset) == Value[j])
					return true;
			}

			return false;
		}
		bool Stringify::StartsNotOf(const char* Value, size_t Offset) const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			VI_ASSERT(Value != nullptr, false, "value should be set");

			size_t Length = strlen(Value);
			if (Offset >= Base->size())
				return false;

			bool Result = true;
			for (size_t j = 0; j < Length; j++)
			{
				if (Base->at(Offset) == Value[j])
				{
					Result = false;
					break;
				}
			}

			return Result;
		}
		bool Stringify::EndsWith(const Core::String& Value) const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			if (Base->empty() || Value.size() > Base->size())
				return false;

			return strcmp(Base->c_str() + Base->size() - Value.size(), Value.c_str()) == 0;
		}
		bool Stringify::EndsWith(const char* Value) const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			VI_ASSERT(Value != nullptr, false, "value should be set");

			size_t Size = strlen(Value);
			if (Base->empty() || Size > Base->size())
				return false;

			return strcmp(Base->c_str() + Base->size() - Size, Value) == 0;
		}
		bool Stringify::EndsWith(const char& Value) const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			return !Base->empty() && Base->back() == Value;
		}
		bool Stringify::EndsOf(const char* Value) const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			VI_ASSERT(Value != nullptr, false, "value should be set");

			if (Base->empty())
				return false;

			size_t Length = strlen(Value);
			for (size_t j = 0; j < Length; j++)
			{
				if (Base->back() == Value[j])
					return true;
			}

			return false;
		}
		bool Stringify::EndsNotOf(const char* Value) const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			VI_ASSERT(Value != nullptr, false, "value should be set");

			if (Base->empty())
				return true;

			size_t Length = strlen(Value);
			for (size_t j = 0; j < Length; j++)
			{
				if (Base->back() == Value[j])
					return false;
			}

			return true;
		}
		bool Stringify::Empty() const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			return Base->empty();
		}
		bool Stringify::HasInteger() const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			if (Base->empty())
				return false;

			bool HadSign = false;
			for (size_t i = 0; i < Base->size(); i++)
			{
				char& V = (*Base)[i];
				if (IsDigit(V))
					continue;

				if ((V == '+' || V == '-') && i == 0 && !HadSign)
				{
					HadSign = true;
					continue;
				}

				return false;
			}

			if (HadSign && Base->size() < 2)
				return false;

			return true;
		}
		bool Stringify::HasNumber() const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			if (Base->empty() || (Base->size() == 1 && Base->front() == '.'))
				return false;

			bool HadPoint = false, HadSign = false;
			for (size_t i = 0; i < Base->size(); i++)
			{
				char& V = (*Base)[i];
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

			if (HadSign && HadPoint && Base->size() < 3)
				return false;
			else if ((HadSign || HadPoint) && Base->size() < 2)
				return false;

			return true;
		}
		bool Stringify::HasDecimal() const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");

			auto F = Find('.');
			if (F.Found)
			{
				auto D1 = Stringify(Base->c_str(), F.End - 1);
				if (D1.Empty() || !D1.HasInteger())
					return false;

				auto D2 = Stringify(Base->c_str() + F.End + 1, Base->size() - F.End - 1);
				if (D2.Empty() || !D2.HasInteger())
					return false;

				return D1.Size() >= 19 || D2.Size() > 6;
			}

			return HasInteger() && Base->size() >= 19;
		}
		bool Stringify::ToBoolean() const
		{
			VI_ASSERT(Base != nullptr, false, "cannot parse without context");
			return !strncmp(Base->c_str(), "true", 4) || !strncmp(Base->c_str(), "1", 1);
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
			VI_ASSERT(Value1 != nullptr && Value2 != nullptr, 0, "both values should be set");

			int Result;
			do
			{
				Result = tolower(*(const unsigned char*)(Value1++)) - tolower(*(const unsigned char*)(Value2++));
			} while (Result == 0 && Value1[-1] != '\0');

			return Result;
		}
		int Stringify::Match(const char* Pattern, const char* Text)
		{
			VI_ASSERT(Pattern != nullptr && Text != nullptr, -1, "pattern and text should be set");
			return Match(Pattern, strlen(Pattern), Text);
		}
		int Stringify::Match(const char* Pattern, size_t Length, const char* Text)
		{
			VI_ASSERT(Pattern != nullptr && Text != nullptr, -1, "pattern and text should be set");
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
		int Stringify::ToInt() const
		{
			VI_ASSERT(Base != nullptr, 0, "cannot parse without context");
			return (int)strtol(Base->c_str(), nullptr, 10);
		}
		long Stringify::ToLong() const
		{
			VI_ASSERT(Base != nullptr, 0, "cannot parse without context");
			return strtol(Base->c_str(), nullptr, 10);
		}
		float Stringify::ToFloat() const
		{
			VI_ASSERT(Base != nullptr, 0.0f, "cannot parse without context");
			return strtof(Base->c_str(), nullptr);
		}
		unsigned int Stringify::ToUInt() const
		{
			VI_ASSERT(Base != nullptr, 0, "cannot parse without context");
			return (unsigned int)ToULong();
		}
		unsigned long Stringify::ToULong() const
		{
			VI_ASSERT(Base != nullptr, 0, "cannot parse without context");
			return strtoul(Base->c_str(), nullptr, 10);
		}
		int64_t Stringify::ToInt64() const
		{
			VI_ASSERT(Base != nullptr, 0, "cannot parse without context");
			return strtoll(Base->c_str(), nullptr, 10);
		}
		double Stringify::ToDouble() const
		{
			VI_ASSERT(Base != nullptr, 0.0, "cannot parse without context");
			return strtod(Base->c_str(), nullptr);
		}
		long double Stringify::ToLDouble() const
		{
			VI_ASSERT(Base != nullptr, 0.0, "cannot parse without context");
			return strtold(Base->c_str(), nullptr);
		}
		uint64_t Stringify::ToUInt64() const
		{
			VI_ASSERT(Base != nullptr, 0, "cannot parse without context");
			return strtoull(Base->c_str(), nullptr, 10);
		}
		size_t Stringify::Size() const
		{
			VI_ASSERT(Base != nullptr, 0, "cannot parse without context");
			return Base->size();
		}
		size_t Stringify::Capacity() const
		{
			VI_ASSERT(Base != nullptr, 0, "cannot parse without context");
			return Base->capacity();
		}
		char* Stringify::Value() const
		{
			VI_ASSERT(Base != nullptr, nullptr, "cannot parse without context");
			return (char*)Base->data();
		}
		const char* Stringify::Get() const
		{
			VI_ASSERT(Base != nullptr, nullptr, "cannot parse without context");
			return Base->c_str();
		}
		Core::String& Stringify::R()
		{
			VI_ASSERT(Base != nullptr, *Base, "cannot parse without context");
			return *Base;
		}
		Core::WideString Stringify::ToWide() const
		{
			VI_ASSERT(Base != nullptr, Core::WideString(), "cannot parse without context");
			Core::WideString Output;
			Output.reserve(Base->size());

			wchar_t W;
			for (size_t i = 0; i < Base->size();)
			{
				char C = Base->at(i);
				if ((C & 0x80) == 0)
				{
					W = C;
					i++;
				}
				else if ((C & 0xE0) == 0xC0)
				{
					W = (C & 0x1F) << 6;
					W |= (Base->at(i + 1) & 0x3F);
					i += 2;
				}
				else if ((C & 0xF0) == 0xE0)
				{
					W = (C & 0xF) << 12;
					W |= (Base->at(i + 1) & 0x3F) << 6;
					W |= (Base->at(i + 2) & 0x3F);
					i += 3;
				}
				else if ((C & 0xF8) == 0xF0)
				{
					W = (C & 0x7) << 18;
					W |= (Base->at(i + 1) & 0x3F) << 12;
					W |= (Base->at(i + 2) & 0x3F) << 6;
					W |= (Base->at(i + 3) & 0x3F);
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
		Core::Vector<Core::String> Stringify::Split(const Core::String& With, size_t Start) const
		{
			VI_ASSERT(Base != nullptr, Core::Vector<Core::String>(), "cannot parse without context");
			Core::Vector<Core::String> Output;
			if (Start >= Base->size())
				return Output;

			size_t Offset = Start;
			Stringify::Settle Result = Find(With, Offset);
			while (Result.Found)
			{
				Output.emplace_back(Base->substr(Offset, Result.Start - Offset));
				Result = Find(With, Offset = Result.End);
			}

			if (Offset < Base->size())
				Output.emplace_back(Base->substr(Offset));

			return Output;
		}
		Core::Vector<Core::String> Stringify::Split(char With, size_t Start) const
		{
			VI_ASSERT(Base != nullptr, Core::Vector<Core::String>(), "cannot parse without context");
			Core::Vector<Core::String> Output;
			if (Start >= Base->size())
				return Output;

			size_t Offset = Start;
			Stringify::Settle Result = Find(With, Start);
			while (Result.Found)
			{
				Output.emplace_back(Base->substr(Offset, Result.Start - Offset));
				Result = Find(With, Offset = Result.End);
			}

			if (Offset < Base->size())
				Output.emplace_back(Base->substr(Offset));

			return Output;
		}
		Core::Vector<Core::String> Stringify::SplitMax(char With, size_t Count, size_t Start) const
		{
			VI_ASSERT(Base != nullptr, Core::Vector<Core::String>(), "cannot parse without context");
			Core::Vector<Core::String> Output;
			if (Start >= Base->size())
				return Output;

			size_t Offset = Start;
			Stringify::Settle Result = Find(With, Start);
			while (Result.Found && Output.size() < Count)
			{
				Output.emplace_back(Base->substr(Offset, Result.Start - Offset));
				Result = Find(With, Offset = Result.End);
			}

			if (Offset < Base->size() && Output.size() < Count)
				Output.emplace_back(Base->substr(Offset));

			return Output;
		}
		Core::Vector<Core::String> Stringify::SplitOf(const char* With, size_t Start) const
		{
			VI_ASSERT(Base != nullptr, Core::Vector<Core::String>(), "cannot parse without context");
			Core::Vector<Core::String> Output;
			if (Start >= Base->size())
				return Output;

			size_t Offset = Start;
			Stringify::Settle Result = FindOf(With, Start);
			while (Result.Found)
			{
				Output.emplace_back(Base->substr(Offset, Result.Start - Offset));
				Result = FindOf(With, Offset = Result.End);
			}

			if (Offset < Base->size())
				Output.emplace_back(Base->substr(Offset));

			return Output;
		}
		Core::Vector<Core::String> Stringify::SplitNotOf(const char* With, size_t Start) const
		{
			VI_ASSERT(Base != nullptr, Core::Vector<Core::String>(), "cannot parse without context");
			Core::Vector<Core::String> Output;
			if (Start >= Base->size())
				return Output;

			size_t Offset = Start;
			Stringify::Settle Result = FindNotOf(With, Start);
			while (Result.Found)
			{
				Output.emplace_back(Base->substr(Offset, Result.Start - Offset));
				Result = FindNotOf(With, Offset = Result.End);
			}

			if (Offset < Base->size())
				Output.emplace_back(Base->substr(Offset));

			return Output;
		}
		void Stringify::ConvertToWide(const char* Input, size_t InputSize, wchar_t* Output, size_t OutputSize)
		{
			VI_ASSERT_V(Input != nullptr, "input should be set");
			VI_ASSERT_V(Output != nullptr && OutputSize > 0, "output should be set");

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
		Stringify& Stringify::operator= (Stringify&& Value) noexcept
		{
			VI_ASSERT(&Value != this, *this, "cannot set to self");
			if (Deletable)
				VI_DELETE(basic_string, Base);

			Base = Value.Base;
			Deletable = Value.Deletable;
			Value.Base = nullptr;
			Value.Deletable = false;

			return *this;
		}
		Stringify& Stringify::operator= (const Stringify& Value) noexcept
		{
			VI_ASSERT(&Value != this, *this, "cannot set to self");
			if (Deletable)
				VI_DELETE(basic_string, Base);

			Deletable = true;
			if (Value.Base != nullptr)
				Base = VI_NEW(Core::String, *Value.Base);
			else
				Base = VI_NEW(Core::String);

			return *this;
		}
		Core::String Stringify::ToString(float Number)
		{
			Core::String Result(Core::ToString(Number));
			Result.erase(Result.find_last_not_of('0') + 1, Core::String::npos);
			if (!Result.empty() && Result.back() == '.')
				Result.erase(Result.end() - 1);

			return Result;
		}
		Core::String Stringify::ToString(double Number)
		{
			Core::String Result(Core::ToString(Number));
			Result.erase(Result.find_last_not_of('0') + 1, Core::String::npos);
			if (!Result.empty() && Result.back() == '.')
				Result.erase(Result.end() - 1);

			return Result;
		}

		Spin::Spin() noexcept
		{
		}
		void Spin::Lock()
		{
			while (Atom.test_and_set(std::memory_order_acquire))
				std::this_thread::yield();
		}
		void Spin::Unlock()
		{
			Atom.clear(std::memory_order_release);
		}

		Guard::Loaded::Loaded(Guard* NewBase) noexcept : Base(NewBase)
		{
		}
		Guard::Loaded::Loaded(Loaded&& Other) noexcept : Base(Other.Base)
		{
			Other.Base = nullptr;
		}
		Guard::Loaded& Guard::Loaded::operator =(Loaded&& Other) noexcept
		{
			if (&Other == this)
				return *this;

			Base = Other.Base;
			Other.Base = nullptr;
			return *this;
		}
		Guard::Loaded::~Loaded() noexcept
		{
			Close();
		}
		void Guard::Loaded::Close()
		{
			if (Base != nullptr)
			{
				Base->LoadUnlock();
				Base = nullptr;
			}
		}
		Guard::Loaded::operator bool() const
		{
			return Base != nullptr;
		}

		Guard::Stored::Stored(Guard* NewBase) noexcept : Base(NewBase)
		{
		}
		Guard::Stored::Stored(Stored&& Other) noexcept : Base(Other.Base)
		{
			Other.Base = nullptr;
		}
		Guard::Stored& Guard::Stored::operator =(Stored&& Other) noexcept
		{
			if (&Other == this)
				return *this;

			Base = Other.Base;
			Other.Base = nullptr;
			return *this;
		}
		Guard::Stored::~Stored() noexcept
		{
			Close();
		}
		void Guard::Stored::Close()
		{
			if (Base != nullptr)
			{
				Base->StoreUnlock();
				Base = nullptr;
			}
		}
		Guard::Stored::operator bool() const
		{
			return Base != nullptr;
		}

		Guard::Guard() noexcept : Readers(0), Writers(0)
		{
		}
		Guard::Loaded Guard::TryLoad()
		{
			return TryLoadLock() ? Loaded(this) : Loaded(nullptr);
		}
		Guard::Loaded Guard::Load()
		{
			LoadLock();
			return Loaded(this);
		}
		Guard::Stored Guard::TryStore()
		{
			return TryStoreLock() ? Stored(this) : Stored(nullptr);
		}
		Guard::Stored Guard::Store()
		{
			StoreLock();
			return Stored(this);
		}
		bool Guard::TryLoadLock()
		{
			std::unique_lock<std::mutex> Unique(Mutex);
			if (Writers > 0)
				return false;

			++Readers;
			return true;
		}
		void Guard::LoadLock()
		{
			std::unique_lock<std::mutex> Unique(Mutex);
			while (Writers > 0)
				Condition.wait(Unique);
			++Readers;
		}
		void Guard::LoadUnlock()
		{
			std::unique_lock<std::mutex> Unique(Mutex);
			VI_ASSERT_V(Readers > 0, "value was not loaded");

			if (!--Readers)
				Condition.notify_one();
		}
		bool Guard::TryStoreLock()
		{
			std::unique_lock<std::mutex> Unique(Mutex);
			if (Readers > 0 || Writers > 0)
				return false;

			++Writers;
			return true;
		}
		void Guard::StoreLock()
		{
			std::unique_lock<std::mutex> Unique(Mutex);
			while (Readers > 0 || Writers > 0)
				Condition.wait(Unique);
			++Writers;
		}
		void Guard::StoreUnlock()
		{
			std::unique_lock<std::mutex> Unique(Mutex);
			VI_ASSERT_V(Writers > 0, "value was not stored");

			--Writers;
			Condition.notify_all();
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
			VI_ASSERT(Value != nullptr, Null(), "value should be set");
			Variant Result(VarType::String);
			Result.Size = Size;

			size_t StringSize = sizeof(char) * (Result.Size + 1);
			if (Result.Size > Variant::GetMaxSmallStringSize())
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
			VI_ASSERT(Value != nullptr, Null(), "value should be set");
			Variant Result(VarType::Binary);
			Result.Size = Size;

			size_t StringSize = sizeof(char) * (Result.Size + 1);
			if (Result.Size > Variant::GetMaxSmallStringSize())
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

		bool Composer::Clear()
		{
			if (!Factory)
				return false;

			VI_DELETE(Mapping, Factory);
			Factory = nullptr;
			return true;
		}
		bool Composer::Pop(const Core::String& Hash)
		{
			VI_ASSERT(Factory != nullptr, false, "composer should be initialized");
			VI_TRACE("[composer] pop %s", Hash.c_str());

			auto It = Factory->Map.find(VI_HASH(Hash));
			if (It == Factory->Map.end())
				return false;

			Factory->Map.erase(It);
			if (!Factory->Map.empty())
				return true;

			VI_DELETE(Mapping, Factory);
			Factory = nullptr;
			return true;
		}
		void Composer::Push(uint64_t TypeId, uint64_t Tag, void* Callback)
		{
			using Map = Mapping<Core::UnorderedMap<uint64_t, std::pair<uint64_t, void*>>>;
			if (!Factory)
				Factory = VI_NEW(Map);

			if (Factory->Map.find(TypeId) == Factory->Map.end())
				Factory->Map[TypeId] = std::make_pair(Tag, Callback);
			VI_TRACE("[composer] push type %" PRIu64 " tagged as %" PRIu64, TypeId, Tag);
		}
		void* Composer::Find(uint64_t TypeId)
		{
			VI_ASSERT(Factory != nullptr, nullptr, "composer should be initialized");
			auto It = Factory->Map.find(TypeId);
			if (It != Factory->Map.end())
				return It->second.second;

			return nullptr;
		}
		Core::UnorderedSet<uint64_t> Composer::Fetch(uint64_t Id)
		{
			VI_ASSERT(Factory != nullptr, Core::UnorderedSet<uint64_t>(), "composer should be initialized");

			Core::UnorderedSet<uint64_t> Hashes;
			for (auto& Item : Factory->Map)
			{
				if (Item.second.first == Id)
					Hashes.insert(Item.first);
			}

			return Hashes;
		}
		Mapping<Core::UnorderedMap<uint64_t, std::pair<uint64_t, void*>>>* Composer::Factory = nullptr;

		Console::Console() noexcept : Status(Mode::Detached), Colors(true)
		{
		}
		Console::~Console() noexcept
		{
			Deallocate();
			if (Singleton == this)
				Singleton = nullptr;
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
		void Console::Begin()
		{
			Session.lock();
		}
		void Console::End()
		{
			Session.unlock();
		}
		void Console::Hide()
		{
#ifdef VI_MICROSOFT
			::ShowWindow(::GetConsoleWindow(), SW_HIDE);
			VI_TRACE("[console] hide window");
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
			(void)std::system("clear");
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
			Deallocate();
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
			std::cout << OS::GetStackTrace(2, MaxFrames) << '\n';
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
		void Console::WriteBuffer(const char* Buffer)
		{
			VI_ASSERT_V(Buffer != nullptr, "buffer should be set");
			std::cout << Buffer;
		}
		void Console::WriteLine(const Core::String& Line)
		{
			std::cout << Line << '\n';
		}
		void Console::WriteChar(char Value)
		{
			std::cout << Value;
		}
		void Console::Write(const Core::String& Line)
		{
			std::cout << Line;
		}
		void Console::fWriteLine(const char* Format, ...)
		{
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
		void Console::sWriteLine(const Core::String& Line)
		{
			Session.lock();
			std::cout << Line << '\n';
			Session.unlock();
		}
		void Console::sWrite(const Core::String& Line)
		{
			Session.lock();
			std::cout << Line;
			Session.unlock();
		}
		void Console::sfWriteLine(const char* Format, ...)
		{
			VI_ASSERT_V(Format != nullptr, "format should be set");

			char Buffer[BLOB_SIZE] = { '\0' };
			va_list Args;
			va_start(Args, Format);
#ifdef VI_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);

			Session.lock();
			std::cout << Buffer << '\n';
			Session.unlock();
		}
		void Console::sfWrite(const char* Format, ...)
		{
			VI_ASSERT_V(Format != nullptr, "format should be set");

			char Buffer[BLOB_SIZE] = { '\0' };
			va_list Args;
			va_start(Args, Format);
#ifdef VI_MICROSOFT
			_vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#else
			vsnprintf(Buffer, sizeof(Buffer), Format, Args);
#endif
			va_end(Args);

			Session.lock();
			std::cout << Buffer;
			Session.unlock();
		}
		void Console::GetSize(uint32_t* Width, uint32_t* Height)
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
		bool Console::ReadLine(Core::String& Data, size_t Size)
		{
			VI_ASSERT(Status != Mode::Detached, false, "console should be shown at least once");
			VI_ASSERT(Size > 0, false, "read length should be greater than zero");
			VI_TRACE("[console] read up to %" PRIu64 " bytes", (uint64_t)Size);

			char* Value = VI_MALLOC(char, sizeof(char) * (Size + 1));
			memset(Value, 0, Size * sizeof(char));
			Value[Size] = '\0';
#ifndef VI_MICROSOFT
			std::cout.flush();
#endif
			if (!std::cin.getline(Value, Size))
			{
				Data.clear();
				VI_FREE(Value);
				return false;
			}

			Data = Value;
			VI_FREE(Value);
			return true;
		}
		Core::String Console::Read(size_t Size)
		{
			Core::String Data;
			Data.reserve(Size);
			ReadLine(Data, Size);
			return Data;
		}
		bool Console::IsPresent()
		{
			return Singleton != nullptr && Singleton->Status != Mode::Detached;
		}
		bool Console::Reset()
		{
			if (!Singleton)
				return false;

			VI_RELEASE(Singleton);
			return true;
		}
		Console* Console::Get()
		{
			if (Singleton == nullptr)
				Singleton = new Console();

			return Singleton;
		}
		Console* Console::Singleton = nullptr;

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
			VI_ASSERT(!Captures.empty(), Capture(), "there is no time captured at the moment");
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

		Stream::Stream() noexcept : VirtualSize(0)
		{
		}
		void Stream::SetVirtualSize(size_t Size)
		{
			VirtualSize = 0;
		}
		size_t Stream::ReadAll(const std::function<void(char*, size_t)>& Callback)
		{
			VI_ASSERT(Callback != nullptr, 0, "callback should be set");
			VI_TRACE("[io] read all bytes on fd %i", GetFd());

			char Buffer[CHUNK_SIZE];
			size_t Size = 0, Total = 0;

			do
			{
				size_t Max = sizeof(Buffer);
				if (VirtualSize > 0 && VirtualSize - Total > Max)
					Max = VirtualSize - Total;

				Size = Read(Buffer, Max);
				if (Size > 0)
				{
					Callback(Buffer, Size);
					Total += Size;
				}
			} while (Size > 0);

			return Size;
		}
		size_t Stream::GetVirtualSize() const
		{
			return VirtualSize;
		}
		size_t Stream::GetSize()
		{
			if (!IsSized())
				return 0;

			size_t Position = Tell();
			Seek(FileSeek::End, 0);
			size_t Size = Tell();
			Seek(FileSeek::Begin, Position);

			return Size;
		}
		Core::String& Stream::GetSource()
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
		bool FileStream::Clear()
		{
			VI_TRACE("[io] fs %i clear", GetFd());
			if (!Close())
				return false;

			if (Path.empty())
				return false;

			Resource = (FILE*)OS::File::Open(Path.c_str(), "w");
			return Resource != nullptr;
		}
		bool FileStream::Open(const char* File, FileMode Mode)
		{
			VI_ASSERT(File != nullptr, false, "filename should be set");

			Close();
			Path = OS::Path::Resolve(File);

			switch (Mode)
			{
				case FileMode::Read_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "r");
					break;
				case FileMode::Write_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "w");
					break;
				case FileMode::Append_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "a");
					break;
				case FileMode::Read_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "r+");
					break;
				case FileMode::Write_Read:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "w+");
					break;
				case FileMode::Read_Append_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "a+");
					break;
				case FileMode::Binary_Read_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "rb");
					break;
				case FileMode::Binary_Write_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "wb");
					break;
				case FileMode::Binary_Append_Only:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "ab");
					break;
				case FileMode::Binary_Read_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "rb+");
					break;
				case FileMode::Binary_Write_Read:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "wb+");
					break;
				case FileMode::Binary_Read_Append_Write:
					Resource = (FILE*)OS::File::Open(Path.c_str(), "ab+");
					break;
			}

			return Resource != nullptr;
		}
		bool FileStream::Close()
		{
			if (Resource != nullptr)
			{
				OS::File::Close(Resource);
				Resource = nullptr;
			}

			return true;
		}
		bool FileStream::Seek(FileSeek Mode, int64_t Offset)
		{
			VI_ASSERT(Resource != nullptr, false, "file should be opened");
			VI_MEASURE(Core::Timings::FileSystem);

			switch (Mode)
			{
				case FileSeek::Begin:
					VI_TRACE("[io] seek fs %i begin %" PRId64, GetFd(), Offset);
					return fseek(Resource, (long)Offset, SEEK_SET) == 0;
				case FileSeek::Current:
					VI_TRACE("[io] seek fs %i move %" PRId64, GetFd(), Offset);
					return fseek(Resource, (long)Offset, SEEK_CUR) == 0;
				case FileSeek::End:
					VI_TRACE("[io] seek fs %i end %" PRId64, GetFd(), Offset);
					return fseek(Resource, (long)Offset, SEEK_END) == 0;
				default:
					return false;
			}
		}
		bool FileStream::Move(int64_t Offset)
		{
			VI_ASSERT(Resource != nullptr, false, "file should be opened");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[io] seek fs %i move %" PRId64, GetFd(), Offset);
			return fseek(Resource, (long)Offset, SEEK_CUR) == 0;
		}
		int FileStream::Flush()
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[io] flush fs %i", GetFd());
			return fflush(Resource);
		}
		size_t FileStream::ReadAny(const char* Format, ...)
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
			VI_ASSERT(Format != nullptr, false, "format should be set");
			VI_MEASURE(Core::Timings::FileSystem);

			va_list Args;
			va_start(Args, Format);
			size_t R = (size_t)vfscanf(Resource, Format, Args);
			va_end(Args);

			VI_TRACE("[io] fs %i scan %i bytes", GetFd(), (int)R);
			return R;
		}
		size_t FileStream::Read(char* Data, size_t Length)
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
			VI_ASSERT(Data != nullptr, false, "data should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[io] fs %i read %i bytes", GetFd(), (int)Length);
			return fread(Data, 1, Length, Resource);
		}
		size_t FileStream::WriteAny(const char* Format, ...)
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
			VI_ASSERT(Format != nullptr, false, "format should be set");
			VI_MEASURE(Core::Timings::FileSystem);

			va_list Args;
			va_start(Args, Format);
			size_t R = (size_t)vfprintf(Resource, Format, Args);
			va_end(Args);

			VI_TRACE("[io] fs %i print %i bytes", GetFd(), (int)R);
			return R;
		}
		size_t FileStream::Write(const char* Data, size_t Length)
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
			VI_ASSERT(Data != nullptr, false, "data should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[io] fs %i write %i bytes", GetFd(), (int)Length);
			return fwrite(Data, 1, Length, Resource);
		}
		size_t FileStream::Tell()
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[io] fs %i tell", GetFd());
			return ftell(Resource);
		}
		int FileStream::GetFd() const
		{
			VI_ASSERT(Resource != nullptr, -1, "file should be opened");
			return VI_FILENO(Resource);
		}
		void* FileStream::GetBuffer() const
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
		bool GzStream::Clear()
		{
			VI_TRACE("[gz] fs %i clear", GetFd());
			if (!Close())
				return false;

			if (Path.empty())
				return false;

			OS::File::Close(OS::File::Open(Path.c_str(), "w"));
			return Open(Path.c_str(), FileMode::Binary_Write_Only);
		}
		bool GzStream::Open(const char* File, FileMode Mode)
		{
			VI_ASSERT(File != nullptr, 0, "filename should be set");
#ifdef VI_HAS_ZLIB
			VI_MEASURE(Core::Timings::FileSystem);
			Close();

			Path = OS::Path::Resolve(File);
			switch (Mode)
			{
				case FileMode::Binary_Read_Only:
				case FileMode::Read_Only:
					Resource = gzopen(Path.c_str(), "rb");
					VI_DEBUG("[gz] open rb %s", Path.c_str());
					break;
				case FileMode::Binary_Write_Only:
				case FileMode::Write_Only:
					Resource = gzopen(Path.c_str(), "wb");
					VI_DEBUG("[gz] open wb %s", Path.c_str());
					break;
				case FileMode::Append_Only:
				case FileMode::Binary_Append_Only:
				case FileMode::Read_Write:
				case FileMode::Write_Read:
				case FileMode::Read_Append_Write:
				case FileMode::Binary_Read_Write:
				case FileMode::Binary_Write_Read:
				case FileMode::Binary_Read_Append_Write:
					VI_ERR("[gz] compressed stream supports only rb and wb modes");
					Close();
					break;
			}

			return Resource != nullptr;
#else
			return false;
#endif
		}
		bool GzStream::Close()
		{
#ifdef VI_HAS_ZLIB
			VI_MEASURE(Core::Timings::FileSystem);
			if (Resource != nullptr)
			{
				VI_DEBUG("[gz] close 0x%" PRIXPTR, (uintptr_t)Resource);
				gzclose((gzFile)Resource);
				Resource = nullptr;
			}
#endif
			return true;
		}
		bool GzStream::Seek(FileSeek Mode, int64_t Offset)
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
#ifdef VI_HAS_ZLIB
			VI_MEASURE(Core::Timings::FileSystem);
			switch (Mode)
			{
				case FileSeek::Begin:
					VI_TRACE("[gz] seek fs %i begin %" PRId64, GetFd(), Offset);
					return gzseek((gzFile)Resource, (long)Offset, SEEK_SET) == 0;
				case FileSeek::Current:
					VI_TRACE("[gz] seek fs %i move %" PRId64, GetFd(), Offset);
					return gzseek((gzFile)Resource, (long)Offset, SEEK_CUR) == 0;
				case FileSeek::End:
					VI_ASSERT(false, false, "gz seek from end is not supported");
					return gzseek((gzFile)Resource, (long)Offset, SEEK_END) == 0;
			}
#endif
			return false;
		}
		bool GzStream::Move(int64_t Offset)
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
#ifdef VI_HAS_ZLIB
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[gz] seek fs %i move %" PRId64, GetFd(), Offset);
			return gzseek((gzFile)Resource, (long)Offset, SEEK_CUR) == 0;
#else
			return false;
#endif
		}
		int GzStream::Flush()
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
#ifdef VI_HAS_ZLIB
			VI_MEASURE(Core::Timings::FileSystem);
			return gzflush((gzFile)Resource, Z_SYNC_FLUSH);
#else
			return 0;
#endif
		}
		size_t GzStream::ReadAny(const char* Format, ...)
		{
			VI_ASSERT(false, -1, "gz read-format is not supported");
			return 0;
		}
		size_t GzStream::Read(char* Data, size_t Length)
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
			VI_ASSERT(Data != nullptr, 0, "data should be set");
#ifdef VI_HAS_ZLIB
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[gz] fs %i read %i bytes", GetFd(), (int)Length);
			return gzread((gzFile)Resource, Data, (unsigned int)Length);
#else
			return 0;
#endif
		}
		size_t GzStream::WriteAny(const char* Format, ...)
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
			VI_ASSERT(Format != nullptr, 0, "format should be set");
			VI_MEASURE(Core::Timings::FileSystem);

			va_list Args;
			va_start(Args, Format);
#ifdef VI_HAS_ZLIB
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
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
			VI_ASSERT(Data != nullptr, 0, "data should be set");
#ifdef VI_HAS_ZLIB
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[gz] fs %i write %i bytes", GetFd(), (int)Length);
			return gzwrite((gzFile)Resource, Data, (unsigned int)Length);
#else
			return 0;
#endif
		}
		size_t GzStream::Tell()
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
#ifdef VI_HAS_ZLIB
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[gz] fs %i tell", GetFd());
			return gztell((gzFile)Resource);
#else
			return 0;
#endif
		}
		int GzStream::GetFd() const
		{
			VI_ASSERT(false, -1, "gz fd fetch is not supported");
			return -1;
		}
		void* GzStream::GetBuffer() const
		{
			return (void*)Resource;
		}
		bool GzStream::IsSized() const
		{
			return false;
		}

		WebStream::WebStream(bool IsAsync) noexcept : Resource(nullptr), Offset(0), Size(0), Async(IsAsync)
		{
		}
		WebStream::WebStream(bool IsAsync, Core::UnorderedMap<Core::String, Core::String>&& NewHeaders) noexcept : WebStream(IsAsync)
		{
			Headers = std::move(NewHeaders);
		}
		WebStream::~WebStream() noexcept
		{
			Close();
		}
		bool WebStream::Clear()
		{
			VI_ASSERT(false, false, "web clear is not supported");
			return false;
		}
		bool WebStream::Open(const char* File, FileMode Mode)
		{
			VI_ASSERT(File != nullptr, 0, "filename should be set");
			Close();

			Network::Location URL(File);
			if (URL.Protocol != "http" && URL.Protocol != "https")
				return false;

			Network::RemoteHost Address;
			Address.Hostname = URL.Hostname;
			Address.Secure = (URL.Protocol == "https");
			Address.Port = (URL.Port < 0 ? (Address.Secure ? 443 : 80) : URL.Port);

			switch (Mode)
			{
				case FileMode::Binary_Read_Only:
				case FileMode::Read_Only:
				{
					auto* Client = new Network::HTTP::Client(30000);
					if (Client->Connect(&Address, false).Get() != 0)
					{
						VI_RELEASE(Client);
						break;
					}

					Network::HTTP::RequestFrame Request;
					Request.URI.assign(URL.Path);

					for (auto& Item : URL.Query)
						Request.Query += Item.first + "=" + Item.second;

					for (auto& Item : Headers)
						Request.SetHeader(Item.first, Item.second);

					Network::HTTP::ResponseFrame* Response = Client->Send(std::move(Request)).Get();
					if (!Response || Response->StatusCode < 0)
					{
						VI_RELEASE(Client);
						break;
					}

					Resource = Client;
					if (Response->Content.Limited)
						Size = Response->Content.Length;

					VI_DEBUG("[http] open ws %s", File);
					break;
				}
				case FileMode::Binary_Write_Only:
				case FileMode::Write_Only:
				case FileMode::Read_Write:
				case FileMode::Write_Read:
				case FileMode::Append_Only:
				case FileMode::Read_Append_Write:
				case FileMode::Binary_Append_Only:
				case FileMode::Binary_Read_Write:
				case FileMode::Binary_Write_Read:
				case FileMode::Binary_Read_Append_Write:
					Close();
					VI_ASSERT(false, false, "web stream supports only rb and r modes");
					break;
			}

			return Resource != nullptr;
		}
		bool WebStream::Close()
		{
			auto* Client = (Network::HTTP::Client*)Resource;
			if (Client != nullptr)
				Client->Close().Wait();

			VI_RELEASE(Client);
			Resource = nullptr;
			Offset = Size = 0;
			Chunk.clear();

			return true;
		}
		bool WebStream::Seek(FileSeek Mode, int64_t NewOffset)
		{
			switch (Mode)
			{
				case FileSeek::Begin:
					VI_TRACE("[http] seek fd %i begin %" PRId64, GetFd(), (int)NewOffset);
					Offset = NewOffset;
					return true;
				case FileSeek::Current:
					VI_TRACE("[http] seek fd %i move %" PRId64, GetFd(), (int)NewOffset);
					if (NewOffset < 0)
					{
						size_t Pointer = (size_t)(-NewOffset);
						if (Pointer > Offset)
						{
							Pointer -= Offset;
							if (Pointer > Size)
								return false;

							Offset = Size - Pointer;
						}
					}
					else
						Offset += NewOffset;
					return true;
				case FileSeek::End:
					VI_TRACE("[http] seek fd %i end %" PRId64, GetFd(), (int)NewOffset);
					Offset = Size - NewOffset;
					return true;
			}

			return false;
		}
		bool WebStream::Move(int64_t Offset)
		{
			VI_ASSERT(false, false, "web move is not supported");
			return false;
		}
		int WebStream::Flush()
		{
			VI_ASSERT(false, -1, "web flush is not supported");
			return 0;
		}
		size_t WebStream::ReadAny(const char* Format, ...)
		{
			VI_ASSERT(false, 0, "web read-format is not supported");
			return 0;
		}
		size_t WebStream::Read(char* Data, size_t Length)
		{
			VI_ASSERT(Resource != nullptr, 0, "file should be opened");
			VI_ASSERT(Data != nullptr, 0, "data should be set");
			VI_ASSERT(Length > 0, 0, "length should be greater than zero");
			VI_TRACE("[http] fd %i read %i bytes", GetFd(), (int)Length);

			size_t Result = 0;
			if (Offset + Length > Chunk.size() && (Chunk.size() < Size || (!Size && !((Network::HTTP::Client*)Resource)->GetResponse()->Content.Limited)))
			{
				auto* Client = (Network::HTTP::Client*)Resource;
				if (!Client->Consume(Length).Get())
					return 0;

				auto* Response = Client->GetResponse();
				if (!Size && Response->Content.Limited)
				{
					Size = Response->Content.Length;
					if (!Size)
						return 0;
				}

				if (Response->Content.Data.empty())
					return 0;

				Chunk.insert(Chunk.end(), Response->Content.Data.begin(), Response->Content.Data.end());
			}

			Result = std::min(Length, Chunk.size() - (size_t)Offset);
			memcpy(Data, Chunk.data() + (size_t)Offset, Result);
			Offset += (size_t)Result;
			return Result;
		}
		size_t WebStream::WriteAny(const char* Format, ...)
		{
			VI_ASSERT(false, 0, "web write-format is not supported");
			return 0;
		}
		size_t WebStream::Write(const char* Data, size_t Length)
		{
			VI_ASSERT(false, 0, "web write is not supported");
			return 0;
		}
		size_t WebStream::Tell()
		{
			VI_TRACE("[http] fd %i tell", GetFd());
			return (size_t)Offset;
		}
		int WebStream::GetFd() const
		{
			VI_ASSERT(Resource != nullptr, -1, "file should be opened");
			return (int)((Network::HTTP::Client*)Resource)->GetStream()->GetFd();
		}
		void* WebStream::GetBuffer() const
		{
			return (void*)Resource;
		}
		bool WebStream::IsSized() const
		{
			return true;
		}

		ProcessStream::ProcessStream() noexcept : FileStream(), ExitCode(-1)
		{
		}
		bool ProcessStream::Clear()
		{
			VI_ASSERT(false, false, "process clear is not supported");
			return false;
		}
		bool ProcessStream::Open(const char* File, FileMode Mode)
		{
			VI_ASSERT(File != nullptr, false, "command should be set");
			Close();
			Path = File;

			switch (Mode)
			{
				case FileMode::Read_Only:
					Resource = (FILE*)OpenPipe(Path.c_str(), "r");
					break;
				case FileMode::Write_Only:
					Resource = (FILE*)OpenPipe(Path.c_str(), "w");
					break;
				case FileMode::Append_Only:
					Resource = (FILE*)OpenPipe(Path.c_str(), "a");
					break;
				case FileMode::Read_Write:
					Resource = (FILE*)OpenPipe(Path.c_str(), "r+");
					break;
				case FileMode::Write_Read:
					Resource = (FILE*)OpenPipe(Path.c_str(), "w+");
					break;
				case FileMode::Read_Append_Write:
					Resource = (FILE*)OpenPipe(Path.c_str(), "a+");
					break;
				case FileMode::Binary_Read_Only:
					Resource = (FILE*)OpenPipe(Path.c_str(), "rb");
					break;
				case FileMode::Binary_Write_Only:
					Resource = (FILE*)OpenPipe(Path.c_str(), "wb");
					break;
				case FileMode::Binary_Append_Only:
					Resource = (FILE*)OpenPipe(Path.c_str(), "ab");
					break;
				case FileMode::Binary_Read_Write:
					Resource = (FILE*)OpenPipe(Path.c_str(), "rb+");
					break;
				case FileMode::Binary_Write_Read:
					Resource = (FILE*)OpenPipe(Path.c_str(), "wb+");
					break;
				case FileMode::Binary_Read_Append_Write:
					Resource = (FILE*)OpenPipe(Path.c_str(), "ab+");
					break;
			}

			return Resource != nullptr;
		}
		bool ProcessStream::Close()
		{
			if (Resource != nullptr)
			{
				ExitCode = ClosePipe(Resource);
				Resource = nullptr;
			}

			return ExitCode == 0;
		}
		bool ProcessStream::IsSized() const
		{
			return false;
		}
		int ProcessStream::GetExitCode() const
		{
			return ExitCode;
		}
		void* ProcessStream::OpenPipe(const char* Path, const char* Mode)
		{
			VI_MEASURE(Core::Timings::FileSystem);
			VI_ASSERT(Path != nullptr && Mode != nullptr, nullptr, "path and mode should be set");
#ifdef VI_MICROSOFT
			wchar_t Buffer[CHUNK_SIZE], Type[20];
			Stringify::ConvertToWide(Path, strlen(Path), Buffer, CHUNK_SIZE);
			Stringify::ConvertToWide(Mode, strlen(Mode), Type, 20);

			FILE* Stream = _wpopen(Buffer, Type);
			if (Stream != nullptr)
				VI_DEBUG("[io] open ps %i %s %s", VI_FILENO(Stream), Mode, Path);
			else
				VI_WARN("[io] open ps %s %s: cannot be opened", Mode, Path);

			return (void*)Stream;
#elif defined(VI_LINUX)
			FILE* Stream = popen(Path, Mode);
			if (Stream != nullptr)
				fcntl(VI_FILENO(Stream), F_SETFD, FD_CLOEXEC);

			if (Stream != nullptr)
				VI_DEBUG("[io] open ps %i %s %s", VI_FILENO(Stream), Mode, Path);
			else
				VI_WARN("[io] open ps %s %s: cannot be opened", Mode, Path);

			return (void*)Stream;
#else
			VI_ERR("[io] open ps %s %s: not supported", Mode, Path);
			return nullptr;
#endif
		}
		int ProcessStream::ClosePipe(void* Fd)
		{
			VI_ASSERT(Fd != nullptr, -1, "stream should be set");
			VI_DEBUG("[io] close ps %i", VI_FILENO((FILE*)Fd));
#ifdef VI_MICROSOFT
			return _pclose((FILE*)Fd);
#elif defined(VI_LINUX)
			return pclose((FILE*)Fd);
#else
			return -1;
#endif
		}

		FileTree::FileTree(const Core::String& Folder) noexcept
		{
			Path = OS::Path::Resolve(Folder.c_str());
			if (!Path.empty())
			{
				OS::Directory::Each(Path.c_str(), [this](FileEntry* Entry) -> bool
				{
					if (Entry->IsDirectory)
						Directories.push_back(new FileTree(Entry->Path));
					else
						Files.emplace_back(OS::Path::Resolve(Entry->Path.c_str()));

					return true;
				});
			}
			else
			{
				Core::Vector<Core::String> Drives = OS::Directory::GetMounts();
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
			VI_ASSERT_V(Callback, "callback should not be empty");
			if (!Callback(this))
				return;

			for (auto& Directory : Directories)
				Directory->Loop(Callback);
		}
		const FileTree* FileTree::Find(const Core::String& V) const
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
		
		OS::Process::ArgsContext::ArgsContext(int Argc, char** Argv, const Core::String& WhenNoValue) noexcept
        {
            Base = OS::Process::GetArgs(Argc, Argv, WhenNoValue);
        }
        void OS::Process::ArgsContext::ForEach(const std::function<void(const Core::String&, const Core::String&)>& Callback) const
        {
            VI_ASSERT_V(Callback != nullptr, "callback should not be empty");
            for (auto& Item : Base)
                Callback(Item.first, Item.second);
        }
        bool OS::Process::ArgsContext::IsEnabled(const Core::String& Option, const Core::String& Shortcut) const
        {
            auto It = Base.find(Option);
            if (It == Base.end() || !IsTrue(It->second))
                return Shortcut.empty() ? false : IsEnabled(Shortcut);
                            
            return true;
        }
        bool OS::Process::ArgsContext::IsDisabled(const Core::String& Option, const Core::String& Shortcut) const
        {
            auto It = Base.find(Option);
            if (It == Base.end())
                return Shortcut.empty() ? true : IsDisabled(Shortcut);
                            
            return IsFalse(It->second);
        }
        bool OS::Process::ArgsContext::Has(const Core::String& Option, const Core::String& Shortcut) const
        {
            if (Base.find(Option) != Base.end())
                return true;
                        
            return Shortcut.empty() ? false : Base.find(Shortcut) != Base.end();
        }
        Core::String& OS::Process::ArgsContext::Get(const Core::String& Option, const Core::String& Shortcut)
        {
            if (Base.find(Option) != Base.end())
                return Base[Option];
                        
            return Shortcut.empty() ? Base[Option] : Base[Shortcut];
        }
        Core::String& OS::Process::ArgsContext::GetIf(const Core::String& Option, const Core::String& Shortcut, const Core::String& WhenEmpty)
        {
            if (Base.find(Option) != Base.end())
                return Base[Option];
                        
            if (!Shortcut.empty() && Base.find(Shortcut) != Base.end())
                return Base[Shortcut];
                        
            Core::String& Value = Base[Option];
            Value = WhenEmpty;
            return Value;
        }
        Core::String& OS::Process::ArgsContext::GetAppPath()
        {
            return Get("__path__");
        }
        bool OS::Process::ArgsContext::IsTrue(const Core::String& Value) const
        {
            if (Value.empty())
                return false;
                        
            if (Stringify((Core::String*)&Value).ToUInt64() > 0)
                return true;
                        
			Stringify Data(Value);
            Data.ToLower();
            return Data.R() == "on" || Data.R() == "true" || Data.R() == "yes" || Data.R() == "y";
        }
        bool OS::Process::ArgsContext::IsFalse(const Core::String& Value) const
        {
            if (Value.empty())
                return true;
                        
            if (Stringify((Core::String*)&Value).ToUInt64() > 0)
                return false;
                        
			Stringify Data(Value);
            Data.ToLower();
            return Data.R() == "off" || Data.R() == "false" || Data.R() == "no" || Data.R() == "n";
        }

		OS::CPU::QuantityInfo OS::CPU::GetQuantityInfo()
		{
			QuantityInfo Result {};
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

			Core::Vector<unsigned int> Packages;
			for (Core::String Line; std::getline(Info, Line);)
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

				Cache Type {};
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
			static const char* SizeKeys[][3] { {}, {"hw.l1icachesize", "hw.l1dcachesize", "hw.l1cachesize"}, {"hw.l2cachesize"}, {"hw.l3cachesize"} };
			CacheInfo Result {};

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
			Core::String Prefix("/sys/devices/system/cpu/cpu0/cache/index" + Core::ToString(Level) + '/');
			Core::String SizePath(Prefix + "size");
			Core::String LineSizePath(Prefix + "coherency_line_size");
			Core::String AssociativityPath(Prefix + "associativity");
			Core::String TypePath(Prefix + "type");
			std::ifstream Size(SizePath.c_str());
			std::ifstream LineSize(LineSizePath.c_str());
			std::ifstream Associativity(AssociativityPath.c_str());
			std::ifstream Type(TypePath.c_str());
			CacheInfo Result {};

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
				Core::String Temp;
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

			for (Core::String Line; std::getline(Info, Line);)
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

		void OS::Directory::SetWorking(const char* Path)
		{
			VI_ASSERT_V(Path != nullptr, "path should be set");
			VI_TRACE("[io] set working dir %s", Path);
#ifdef VI_MICROSOFT
			if (!SetCurrentDirectoryA(Path))
				VI_ERR("[io] couldn't set current directory");
#elif defined(VI_LINUX)
			if (chdir(Path) != 0)
				VI_ERR("[io] couldn't set current directory");
#endif
		}
		void OS::Directory::Patch(const Core::String& Path)
		{
			if (!IsExists(Path.c_str()))
				Create(Path.c_str());
		}
		bool OS::Directory::Scan(const Core::String& Path, Core::Vector<FileEntry>* Entries)
		{
			VI_ASSERT(Entries != nullptr, false, "entries should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[io] scan dir %s", Path.c_str());

			FileEntry Entry;
#if defined(VI_MICROSOFT)
			struct Dirent
			{
				char Directory[CHUNK_SIZE];
			};

			struct Directory
			{
				HANDLE Handle;
				WIN32_FIND_DATAW Info;
				Dirent Result;
			};

			wchar_t Buffer[CHUNK_SIZE];
			Stringify::ConvertToWide(Path.c_str(), Path.size(), Buffer, CHUNK_SIZE);

			auto* Value = VI_MALLOC(Directory, sizeof(Directory));
			DWORD Attributes = GetFileAttributesW(Buffer);
			if (Attributes != 0xFFFFFFFF && ((Attributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
			{
				wcscat(Buffer, L"\\*");
				Value->Handle = FindFirstFileW(Buffer, &Value->Info);
				Value->Result.Directory[0] = '\0';
			}
			else
			{
				VI_FREE(Value);
				return false;
			}

			while (true)
			{
				Dirent* Next = &Value->Result;
				WideCharToMultiByte(CP_UTF8, 0, Value->Info.cFileName, -1, Next->Directory, sizeof(Next->Directory), nullptr, nullptr);
				if (strcmp(Next->Directory, ".") != 0 && strcmp(Next->Directory, "..") != 0 && File::State(Path + '/' + Next->Directory, &Entry))
				{
					Entry.Path = Next->Directory;
					Entries->push_back(Entry);
				}

				if (!FindNextFileW(Value->Handle, &Value->Info))
				{
					FindClose(Value->Handle);
					Value->Handle = INVALID_HANDLE_VALUE;
					break;
				}
			}

			if (Value->Handle != INVALID_HANDLE_VALUE)
				FindClose(Value->Handle);
			VI_FREE(Value);
#else
			DIR* Value = opendir(Path.c_str());
			if (!Value)
				return false;

			dirent* Dirent = nullptr;
			while ((Dirent = readdir(Value)) != nullptr)
			{
				if (strcmp(Dirent->d_name, ".") && strcmp(Dirent->d_name, "..") && File::State(Path + '/' + Dirent->d_name, &Entry))
				{
					Entry.Path = Dirent->d_name;
					Entries->push_back(Entry);
				}
			}
			closedir(Value);
#endif
			return true;
		}
		bool OS::Directory::Each(const char* Path, const std::function<bool(FileEntry*)>& Callback)
		{
			VI_ASSERT(Path != nullptr, false, "path should be set");
			Core::Vector<FileEntry> Entries;
			Core::String Result = Path::Resolve(Path);
			Scan(Result, &Entries);

			Stringify R(&Result);
			if (!R.EndsOf("/\\"))
				Result += VI_SPLITTER;

			for (auto& Entry : Entries)
			{
				if (!Callback(&Entry))
					break;
			}

			return true;
		}
		bool OS::Directory::Create(const char* Path)
		{
			VI_ASSERT(Path != nullptr, false, "path should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_DEBUG("[io] create dir %s", Path);
#ifdef VI_MICROSOFT
			wchar_t Buffer[CHUNK_SIZE];
			Stringify::ConvertToWide(Path, strlen(Path), Buffer, CHUNK_SIZE);

			size_t Length = wcslen(Buffer);
			if (!Length)
				return false;

			if (::CreateDirectoryW(Buffer, nullptr) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS)
				return true;

			size_t Index = Length - 1;
			while (Index > 0 && Buffer[Index] != '/' && Buffer[Index] != '\\')
				Index--;

			if (Index > 0 && !Create(Core::String(Path).substr(0, Index).c_str()))
				return false;

			return ::CreateDirectoryW(Buffer, nullptr) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS;
#else
			if (mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST)
				return true;

			size_t Index = strlen(Path) - 1;
			while (Index > 0 && Path[Index] != '/' && Path[Index] != '\\')
				Index--;

			if (Index > 0 && !Create(Core::String(Path).substr(0, Index).c_str()))
				return false;

			return mkdir(Path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST;
#endif
		}
		bool OS::Directory::Remove(const char* Path)
		{
			VI_ASSERT(Path != nullptr, false, "path should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_DEBUG("[io] remove dir %s", Path);

#ifdef VI_MICROSOFT
			WIN32_FIND_DATA FileInformation;
			Core::String FilePath, Pattern = Core::String(Path) + "\\*.*";
			HANDLE Handle = ::FindFirstFile(Pattern.c_str(), &FileInformation);

			if (Handle == INVALID_HANDLE_VALUE)
			{
				::FindClose(Handle);
				return false;
			}

			do
			{
				if (FileInformation.cFileName[0] == '.')
					continue;

				FilePath = Core::String(Path) + "\\" + FileInformation.cFileName;
				if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					::FindClose(Handle);
					return Remove(FilePath.c_str());
				}

				if (::SetFileAttributes(FilePath.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
				{
					::FindClose(Handle);
					return false;
				}

				if (::DeleteFile(FilePath.c_str()) == FALSE)
				{
					::FindClose(Handle);
					return false;
				}
			} while (::FindNextFile(Handle, &FileInformation) != FALSE);
			::FindClose(Handle);

			if (::GetLastError() != ERROR_NO_MORE_FILES)
				return false;

			if (::SetFileAttributes(Path, FILE_ATTRIBUTE_NORMAL) == FALSE)
				return false;

			return ::RemoveDirectory(Path) != FALSE;
#elif defined VI_LINUX
			DIR* Root = opendir(Path);
			size_t Size = strlen(Path);

			if (!Root)
				return rmdir(Path) == 0;

			struct dirent* It;
			while ((It = readdir(Root)))
			{
				char* Buffer; bool Next = false; size_t Length;
				if (!strcmp(It->d_name, ".") || !strcmp(It->d_name, ".."))
					continue;

				Length = Size + strlen(It->d_name) + 2;
				Buffer = VI_MALLOC(char, Length);

				if (!Buffer)
					continue;

				struct stat State;
				snprintf(Buffer, Length, "%s/%s", Path, It->d_name);

				if (!stat(Buffer, &State))
				{
					if (S_ISDIR(State.st_mode))
						Next = Remove(Buffer);
					else
						Next = (unlink(Buffer) == 0);
				}

				VI_FREE(Buffer);
				if (!Next)
					break;
			}

			closedir(Root);
			return rmdir(Path) == 0;
#endif
		}
		bool OS::Directory::IsExists(const char* Path)
		{
			VI_ASSERT(Path != nullptr, false, "path should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[io] check path %s", Path);

			struct stat Buffer;
			if (stat(Path::Resolve(Path).c_str(), &Buffer) != 0)
				return false;

			return Buffer.st_mode & S_IFDIR;
		}
		Core::String OS::Directory::GetModule()
		{
			VI_MEASURE(Core::Timings::FileSystem);
#ifdef VI_HAS_SDL2
#ifdef VI_MICROSOFT
			char Buffer[MAX_PATH + 1] = { };
			if (GetModuleFileNameA(nullptr, Buffer, MAX_PATH) == 0)
				return Core::String();
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
				return Core::String();
#elif defined(VI_SOLARIS)
			const char* TempBuffer = getexecname();
			if (!TempBuffer || *TempBuffer == '\0')
				return Core::String();

			TempBuffer = OS::Path::GetDirectory(TempBuffer);
			size_t TempBufferSize = strlen(TempBuffer);
			memcpy(Buffer, TempBuffer, TempBufferSize > sizeof(Buffer) - 1 ? sizeof(Buffer) - 1 : TempBufferSize);
#else
			size_t BufferSize = sizeof(Buffer) - 1;
			if (readlink("/proc/self/exe", Buffer, BufferSize) == -1 && readlink("/proc/curproc/file", Buffer, BufferSize) == -1 && readlink("/proc/curproc/exe", Buffer, BufferSize) == -1)
				return Core::String();
#endif
#endif
			Core::String Result = Path::GetDirectory(Buffer);
			if (!Result.empty() && Result.back() != '/' && Result.back() != '\\')
				Result += VI_SPLITTER;

			VI_TRACE("[io] fetch module dir %s", Result.c_str());
			return Result;
#else
			char* Buffer = SDL_GetBasePath();
			Core::String Result = Buffer;
			SDL_free(Buffer);

			VI_TRACE("[io] fetch module dir %s", Result.c_str());
			return Result;
#endif
		}
		Core::String OS::Directory::GetWorking()
		{
			VI_MEASURE(Core::Timings::FileSystem);
#ifdef VI_MICROSOFT
			char Buffer[MAX_PATH + 1] = { };
			if (GetCurrentDirectoryA(MAX_PATH, Buffer) == 0)
				return Core::String();
#else
#ifdef PATH_MAX
			char Buffer[PATH_MAX + 1] = { };
#elif defined(_POSIX_PATH_MAX)
			char Buffer[_POSIX_PATH_MAX + 1] = { };
#else
			char Buffer[CHUNK_SIZE + 1] = { };
#endif
			if (!getcwd(Buffer, sizeof(Buffer)))
				return Core::String();
#endif
			Core::String Result = Buffer;
			if (!Result.empty() && Result.back() != '/' && Result.back() != '\\')
				Result += VI_SPLITTER;

			VI_TRACE("[io] fetch working dir %s", Result.c_str());
			return Result;
		}
		Core::Vector<Core::String> OS::Directory::GetMounts()
		{
			VI_TRACE("[io] fetch mount points");
			Core::Vector<Core::String> Output;
#ifdef VI_MICROSOFT
			DWORD DriveMask = GetLogicalDrives();
			int Offset = (int)'A';

			while (DriveMask)
			{
				if (DriveMask & 1)
					Output.push_back(Core::String(1, (char)Offset) + '\\');
				DriveMask >>= 1;
				Offset++;
			}

			return Output;
#else
			Output.push_back("/");
			return Output;
#endif
		}

		bool OS::File::State(const Core::String& Path, FileEntry* Resource)
		{
			VI_ASSERT(Resource != nullptr, false, "resource should be set");
			VI_MEASURE(Core::Timings::FileSystem);

			*Resource = FileEntry();
			Resource->Path = Path;
#if defined(VI_MICROSOFT)
			wchar_t Buffer[CHUNK_SIZE];
			Stringify::ConvertToWide(Path.c_str(), Path.size(), Buffer, CHUNK_SIZE);

			WIN32_FILE_ATTRIBUTE_DATA Info;
			if (GetFileAttributesExW(Buffer, GetFileExInfoStandard, &Info) == 0)
			{
				VI_TRACE("[io] stat %s: non-existant", Path.c_str());
				return false;
			}

			Resource->Size = MAKEUQUAD(Info.nFileSizeLow, Info.nFileSizeHigh);
			Resource->LastModified = SYS2UNIX_TIME(Info.ftLastWriteTime.dwLowDateTime, Info.ftLastWriteTime.dwHighDateTime);
			Resource->CreationTime = SYS2UNIX_TIME(Info.ftCreationTime.dwLowDateTime, Info.ftCreationTime.dwHighDateTime);
			if (Resource->CreationTime > Resource->LastModified)
				Resource->LastModified = Resource->CreationTime;

			Resource->IsDirectory = Info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			VI_TRACE("[io] stat %s: %s %" PRIu64 " bytes", Path.c_str(), Resource->IsDirectory ? "dir" : "file", (uint64_t)Resource->Size);

			if (Resource->IsDirectory)
			{
				Resource->IsExists = true;
				return true;
			}

			if (Path.empty())
				return false;

			int End = Path.back();
			if (isalnum(End) || strchr("_-", End) != nullptr)
			{
				Resource->IsExists = true;
				return true;
			}

			return false;
#else
			struct stat State;
			if (stat(Path.c_str(), &State) != 0)
			{
				VI_TRACE("[io] stat %s: non-existant", Path.c_str());
				return false;
			}

			struct tm Time;
			LocalTime(&State.st_ctime, &Time);
			Resource->CreationTime = mktime(&Time);
			Resource->Size = (size_t)(State.st_size);
			Resource->LastModified = State.st_mtime;
			Resource->IsDirectory = S_ISDIR(State.st_mode);
			Resource->IsExists = true;

			VI_TRACE("[io] stat %s: %s %" PRIu64 " bytes", Path.c_str(), Resource->IsDirectory ? "dir" : "file", (uint64_t)Resource->Size);
			return true;
#endif
		}
		bool OS::File::Write(const Core::String& Path, const char* Data, size_t Length)
		{
			VI_ASSERT(Data != nullptr, false, "data should be set");
			auto* Stream = Open(Path, FileMode::Binary_Write_Only);
			if (!Stream)
				return false;

			VI_MEASURE(Core::Timings::FileSystem);
			size_t Size = Stream->Write(Data, Length);
			VI_RELEASE(Stream);

			return Size == Length;
		}
		bool OS::File::Write(const Core::String& Path, const Core::String& Data)
		{
			auto* Stream = Open(Path, FileMode::Binary_Write_Only);
			if (!Stream)
				return false;

			VI_MEASURE(Core::Timings::FileSystem);
			size_t Size = Stream->Write(Data.data(), Data.size());
			VI_RELEASE(Stream);

			return Size == Data.size();
		}
		bool OS::File::Move(const char* From, const char* To)
		{
			VI_ASSERT(From != nullptr && To != nullptr, false, "from and to should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_DEBUG("[io] move file from %s to %s", From, To);
#ifdef VI_MICROSOFT
			return MoveFileA(From, To) != 0;
#elif defined VI_LINUX
			return !rename(From, To);
#else
			return false;
#endif
		}
		bool OS::File::Remove(const char* Path)
		{
			VI_ASSERT(Path != nullptr, false, "path should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_DEBUG("[io] remove file %s", Path);
#ifdef VI_MICROSOFT
			SetFileAttributesA(Path, 0);
			return DeleteFileA(Path) != 0;
#elif defined VI_LINUX
			return unlink(Path) == 0;
#else
			return false;
#endif
		}
		bool OS::File::IsExists(const char* Path)
		{
			VI_ASSERT(Path != nullptr, false, "path should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[io] check path %s", Path);
			return IsPathExists(OS::Path::Resolve(Path).c_str());
		}
		void OS::File::Close(void* Stream)
		{
			VI_ASSERT_V(Stream != nullptr, "stream should be set");
			VI_DEBUG("[io] close fs %i", VI_FILENO((FILE*)Stream));
			fclose((FILE*)Stream);
		}
		int OS::File::Compare(const Core::String& FirstPath, const Core::String& SecondPath)
		{
			VI_ASSERT(!FirstPath.empty(), -1, "first path should not be empty");
			VI_ASSERT(!SecondPath.empty(), 1, "second path should not be empty");

			size_t Size1 = GetProperties(FirstPath.c_str()).Size;
			size_t Size2 = GetProperties(SecondPath.c_str()).Size;
			VI_TRACE("[io] compare paths { %s (%" PRIu64 "), %s (%" PRIu64 ") }", FirstPath.c_str(), Size1, SecondPath.c_str(), Size2);

			if (Size1 > Size2)
				return 1;
			else if (Size1 < Size2)
				return -1;

			FILE* First = (FILE*)Open(FirstPath.c_str(), "rb");
			if (!First)
				return -1;

			FILE* Second = (FILE*)Open(SecondPath.c_str(), "rb");
			if (!Second)
			{
				OS::File::Close(First);
				return -1;
			}

			const size_t Size = CHUNK_SIZE;
			char Buffer1[Size];
			char Buffer2[Size];
			int Diff = 0;

			do
			{
				VI_MEASURE(Core::Timings::FileSystem);
				size_t S1 = fread(Buffer1, sizeof(char), Size, First);
				size_t S2 = fread(Buffer2, sizeof(char), Size, Second);
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

			OS::File::Close(First);
			OS::File::Close(Second);
			return Diff;
		}
		size_t OS::File::Join(const Core::String& To, const Core::Vector<Core::String>& Paths)
		{
			VI_ASSERT(!To.empty(), 0, "to should not be empty");
			VI_ASSERT(!Paths.empty(), 0, "paths to join should not be empty");
			VI_TRACE("[io] join %i path to %s", (int)Paths.size(), To.c_str());

			Stream* Target = Open(To, FileMode::Binary_Write_Only);
			if (!Target)
				return 0;

			size_t Total = 0;
			for (auto& Path : Paths)
			{
				Stream* Base = Open(Path, FileMode::Binary_Read_Only);
				if (!Base)
					continue;

				Total += Base->ReadAll([&Target](char* Buffer, size_t Size)
				{
					Target->Write(Buffer, Size);
				});
				VI_RELEASE(Base);
			}

			VI_RELEASE(Target);
			return Total;
		}
		uint64_t OS::File::GetHash(const Core::String& Data)
		{
			return Compute::Crypto::CRC32(Data);
		}
		uint64_t OS::File::GetIndex(const char* Data, size_t Size)
		{
			VI_ASSERT(Data != nullptr, 0, "data buffer should be set");

			uint64_t Result = 0xcbf29ce484222325;
			for (size_t i = 0; i < Size; i++)
			{
				Result ^= Data[i];
				Result *= 1099511628211;
			}

			return Result;
		}
		FileState OS::File::GetProperties(const char* Path)
		{
			FileState State;
			struct stat Buffer;

			VI_ASSERT(Path != nullptr, State, "path should be set");
			VI_MEASURE(Core::Timings::FileSystem);

			if (stat(Path, &Buffer) != 0)
			{
				VI_TRACE("[io] stat %s: non-existant", Path);
				return State;
			}

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
		void* OS::File::Open(const char* Path, const char* Mode)
		{
			VI_MEASURE(Core::Timings::FileSystem);
			VI_ASSERT(Path != nullptr && Mode != nullptr, nullptr, "path and mode should be set");
#ifdef VI_MICROSOFT
			wchar_t Buffer[CHUNK_SIZE], Type[20];
			Stringify::ConvertToWide(Path, strlen(Path), Buffer, CHUNK_SIZE);
			Stringify::ConvertToWide(Mode, strlen(Mode), Type, 20);

			FILE* Stream = _wfopen(Buffer, Type);
			if (Stream != nullptr)
				VI_DEBUG("[io] open fs %i %s %s", VI_FILENO(Stream), Mode, Path);
			else
				VI_WARN("[io] open fs %s %s: cannot be opened", Mode, Path);

			return (void*)Stream;
#else
			FILE* Stream = fopen(Path, Mode);
			if (Stream != nullptr)
				fcntl(VI_FILENO(Stream), F_SETFD, FD_CLOEXEC);

			if (Stream != nullptr)
				VI_DEBUG("[io] open fs %i %s %s", VI_FILENO(Stream), Mode, Path);
			else
				VI_WARN("[io] open fs %s %s: cannot be opened", Mode, Path);

			return (void*)Stream;
#endif
		}
		Stream* OS::File::Open(const Core::String& Path, FileMode Mode, bool Async)
		{
			Network::Location URL(Path);
			if (URL.Protocol == "file")
			{
				Stream* Result = nullptr;
				if (Stringify(&Path).EndsWith(".gz"))
					Result = new GzStream();
				else
					Result = new FileStream();

				if (Result->Open(Path.c_str(), Mode))
					return Result;

				VI_RELEASE(Result);
			}
			else if (URL.Protocol == "http" || URL.Protocol == "https")
			{
				Stream* Result = new WebStream(Async);
				if (Result->Open(Path.c_str(), Mode))
					return Result;

				VI_RELEASE(Result);
			}

			return nullptr;
		}
		Stream* OS::File::OpenArchive(const Core::String& Path, size_t UnarchivedMaxSize)
		{
			FileEntry Data;
			if (!OS::File::State(Path, &Data))
				return Open(Path, FileMode::Binary_Write_Only);

			Core::String Temp = Path::GetNonExistant(Path);
			Move(Path.c_str(), Temp.c_str());

			Stream* Target = nullptr;
			if (Data.Size > UnarchivedMaxSize)
			{
				Target = Open(Path, FileMode::Binary_Write_Only);
				if (Stringify(&Temp).EndsWith(".gz"))
					return Target;

				Stream* Archive = OpenJoin(Temp + ".gz", { Temp });
				if (Archive != nullptr)
					Remove(Temp.c_str());
				VI_RELEASE(Archive);
			}
			else
			{
				Target = OpenJoin(Path, { Temp });
				if (Target != nullptr)
					Remove(Temp.c_str());
			}

			return Target;
		}
		Stream* OS::File::OpenJoin(const Core::String& To, const Core::Vector<Core::String>& Paths)
		{
			VI_ASSERT(!To.empty(), nullptr, "to should not be empty");
			VI_ASSERT(!Paths.empty(), nullptr, "paths to join should not be empty");
			VI_TRACE("[io] open join %i path to %s", (int)Paths.size(), To.c_str());

			Stream* Target = Open(To, FileMode::Binary_Write_Only);
			if (!Target)
				return nullptr;

			for (auto& Path : Paths)
			{
				Stream* Base = Open(Path, FileMode::Binary_Read_Only);
				if (!Base)
					continue;

				Base->ReadAll([&Target](char* Buffer, size_t Size)
				{
					Target->Write(Buffer, Size);
				});
				VI_RELEASE(Base);
			}

			return Target;
		}
		unsigned char* OS::File::ReadAll(const Core::String& Path, size_t* Length)
		{
			auto* Stream = Open(Path, FileMode::Binary_Read_Only);
			if (!Stream)
				return nullptr;

			return ReadAll(Stream, Length);
		}
		unsigned char* OS::File::ReadAll(Stream* Stream, size_t* Length)
		{
			VI_ASSERT(Stream != nullptr, nullptr, "path should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_TRACE("[io] fd %i read-all", Stream->GetFd());

			size_t Size = Stream->GetSize();
			if (Size > 0)
			{
				auto* Bytes = VI_MALLOC(unsigned char, sizeof(unsigned char) * (Size + 1));
				if (!Stream->Read((char*)Bytes, Size))
				{
					VI_RELEASE(Stream);
					VI_FREE(Bytes);

					if (Length != nullptr)
						*Length = 0;

					return nullptr;
				}

				Bytes[Size] = '\0';
				if (Length != nullptr)
					*Length = Size;

				VI_RELEASE(Stream);
				return Bytes;
			}
			else
			{
				Core::String Data;
				Stream->ReadAll([&Data](char* Buffer, size_t Length)
				{
					Data.reserve(Data.size() + Length);
					Data.append(Buffer, Length);
				});

				Size = Data.size();
				if (Data.empty())
				{
					VI_RELEASE(Stream);
					if (Length != nullptr)
						*Length = 0;

					return nullptr;
				}

				auto* Bytes = VI_MALLOC(unsigned char, sizeof(unsigned char) * (Data.size() + 1));
				memcpy(Bytes, Data.data(), sizeof(unsigned char) * Data.size());
				Bytes[Size] = '\0';

				if (Length != nullptr)
					*Length = Size;

				VI_RELEASE(Stream);
				return Bytes;
			}
		}
		unsigned char* OS::File::ReadChunk(Stream* Stream, size_t Length)
		{
			VI_ASSERT(Stream != nullptr, nullptr, "stream should be set");
			auto* Bytes = VI_MALLOC(unsigned char, (Length + 1));
			Stream->Read((char*)Bytes, Length);
			Bytes[Length] = '\0';

			return Bytes;
		}
		Core::String OS::File::ReadAsString(const Core::String& Path)
		{
			size_t Length = 0;
			char* Data = (char*)ReadAll(Path, &Length);
			if (!Data)
				return Core::String();

			Core::String Output = Core::String(Data, Length);
			VI_FREE(Data);

			return Output;
		}
		Core::Vector<Core::String> OS::File::ReadAsArray(const Core::String& Path)
		{
			Core::String Result = ReadAsString(Path);
			return Stringify(&Result).Split('\n');
		}

		bool OS::Path::IsRemote(const char* Path)
		{
			VI_ASSERT(Path != nullptr, false, "path should be set");
			return Network::Location(Path).Protocol != "file";
		}
		Core::String OS::Path::Resolve(const char* Path)
		{
			VI_ASSERT(Path != nullptr, Core::String(), "path should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			char Buffer[BLOB_SIZE] = { };
#ifdef VI_MICROSOFT
			if (GetFullPathNameA(Path, sizeof(Buffer), Buffer, nullptr) == 0)
			{
				VI_TRACE("[io] resolve %s path: non-existant", Path);
				return Path;
			}
#else
			if (!realpath(Path, Buffer))
			{
				VI_TRACE("[io] resolve %s path: non-existant", Path);
				return Path;
			}
#endif
			VI_TRACE("[io] resolve %s path: %s", Path, Buffer);
			return Buffer;
		}
		Core::String OS::Path::Resolve(const Core::String& Path, const Core::String& Directory)
		{
			VI_ASSERT(!Path.empty() && !Directory.empty(), "", "path and directory should not be empty");
			if (IsPathExists(Path.c_str()) && Path.find("..") == std::string::npos)
				return Path;

			Stringify PathData(&Path), DirectoryData(&Directory);
			bool Prefixed = PathData.StartsOf("/\\");
			bool Relative = !Prefixed && (PathData.StartsWith("./") || PathData.StartsWith(".\\"));
			bool Postfixed = DirectoryData.EndsOf("/\\");

			Core::String Target = Directory;
			if (!Prefixed && !Postfixed)
				Target.append(1, VI_SPLITTER);

			if (Relative)
				Target.append(Path.c_str() + 2, Path.size() - 2);
			else
				Target.append(Path);

			return Resolve(Target.c_str());
		}
		Core::String OS::Path::ResolveDirectory(const char* Path)
		{
			Core::String Result = Resolve(Path);
			if (!Result.empty() && !Stringify(&Result).EndsOf("/\\"))
				Result.append(1, VI_SPLITTER);

			return Result;
		}
		Core::String OS::Path::ResolveDirectory(const Core::String& Path, const Core::String& Directory)
		{
			Core::String Result = Resolve(Path, Directory);
			if (!Result.empty() && !Stringify(&Result).EndsOf("/\\"))
				Result.append(1, VI_SPLITTER);

			return Result;
		}
		Core::String OS::Path::GetNonExistant(const Core::String& Path)
		{
			VI_ASSERT(!Path.empty(), Path, "path should not be empty");
			const char* Extension = GetExtension(Path.c_str());
			bool IsTrueFile = Extension != nullptr && *Extension != '\0';
			size_t ExtensionAt = IsTrueFile ? Path.rfind(Extension) : Path.size();
			if (ExtensionAt == Core::String::npos)
				return Path;

			Core::String First = Path.substr(0, ExtensionAt).append(1, '.');
			Core::String Second = IsTrueFile ? Extension : Core::String();
			Core::String Filename = Path;
			size_t Nonce = 0;
			FileEntry Data;

			while (OS::File::State(Filename, &Data) && Data.Size > 0)
				Filename = First + Core::ToString(++Nonce) + Second;

			return Filename;
		}
		Core::String OS::Path::GetDirectory(const char* Path, size_t Level)
		{
			VI_ASSERT(Path != nullptr, Core::String(), "path should be set");

			Stringify Buffer(Path);
			Stringify::Settle Result = Buffer.ReverseFindOf("/\\");
			if (!Result.Found)
				return Path;

			size_t Size = Buffer.Size();
			for (size_t i = 0; i < Level; i++)
			{
				Stringify::Settle Current = Buffer.ReverseFindOf("/\\", Size - Result.Start);
				if (!Current.Found)
				{
					if (Buffer.Splice(0, Result.End).Empty())
						return "/";

					return Buffer.R();
				}

				Result = Current;
			}

			if (Buffer.Splice(0, Result.End).Empty())
				return "/";

			return Buffer.R();
		}
		const char* OS::Path::GetFilename(const char* Path)
		{
			VI_ASSERT(Path != nullptr, nullptr, "path should be set");
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
			VI_ASSERT(Path != nullptr, nullptr, "path should be set");
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
			VI_ASSERT(Resource != nullptr, false, "resource should be set");
			return GetETag(Buffer, Length, Resource->LastModified, Resource->Size);
		}
		bool OS::Net::GetETag(char* Buffer, size_t Length, int64_t LastModified, size_t ContentLength)
		{
			VI_ASSERT(Buffer != nullptr && Length > 0, false, "buffer should be set and size should be greater than Zero");
			snprintf(Buffer, Length, "\"%lx.%" PRIu64 "\"", (unsigned long)LastModified, (uint64_t)ContentLength);
			return true;
		}
		socket_t OS::Net::GetFd(FILE* Stream)
		{
			VI_ASSERT(Stream != nullptr, -1, "stream should be set");
			return VI_FILENO(Stream);
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
		int OS::Process::ExecutePlain(const String& Command)
		{
			VI_ASSERT(!Command.empty(), -1, "format should be set");
			VI_DEBUG("[os] execute sp:command [ %s ]", Buffer);
			return system(Command.c_str());
		}
		ProcessStream* OS::Process::ExecuteWriteOnly(const String& Command)
		{
			VI_ASSERT(!Command.empty(), nullptr, "format should be set");
			VI_DEBUG("[os] execute wo:command [ %s ]", Buffer);
			ProcessStream* Stream = new ProcessStream();
			if (Stream->Open(Command.c_str(), FileMode::Write_Only))
				return Stream;

			VI_RELEASE(Stream);
			return nullptr;
		}
		ProcessStream* OS::Process::ExecuteReadOnly(const String& Command)
		{
			VI_ASSERT(!Command.empty(), nullptr, "format should be set");
			VI_DEBUG("[os] execute ro:command [ %s ]", Buffer);
			ProcessStream* Stream = new ProcessStream();
			if (Stream->Open(Command.c_str(), FileMode::Read_Only))
				return Stream;

			VI_RELEASE(Stream);
			return nullptr;
		}
		bool OS::Process::Spawn(const Core::String& Path, const Core::Vector<Core::String>& Params, ChildProcess* Child)
		{
			VI_MEASURE(Core::Timings::FileSystem);
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

			Stringify Exe = Path::Resolve(Path.c_str());
			if (!Exe.EndsWith(".exe"))
				Exe.Append(".exe");

			Stringify Args = Form("\"%s\"", Exe.Get());
			for (const auto& Param : Params)
				Args.Append(' ').Append(Param);

			if (!CreateProcessA(Exe.Get(), Args.Value(), nullptr, nullptr, TRUE, CREATE_BREAKAWAY_FROM_JOB | HIGH_PRIORITY_CLASS, nullptr, nullptr, &StartupInfo, &Process))
			{
				VI_ERR("[os] cannot spawn process %s", Exe.Get());
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
				Core::Vector<char*> Args;
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
			VI_ASSERT(Process != nullptr && Process->Valid, false, "process should be set and be valid");
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
			VI_TRACE("[os] await process %s", (int)GetProcessId(Process->Process));
			waitpid(Process->Process, &Status, 0);

			VI_DEBUG("[os] close process %s", (int)Process->Process);
			if (ExitCode != nullptr)
				*ExitCode = WEXITSTATUS(Status);
#endif
			Free(Process);
			return true;
		}
		bool OS::Process::Free(ChildProcess* Child)
		{
			VI_ASSERT(Child != nullptr && Child->Valid, false, "child should be set and be valid");
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
		Core::String OS::Process::GetThreadId(const std::thread::id& Id)
		{
			Core::StringStream Stream;
			Stream << Id;

			return Stream.str();
		}
        Core::UnorderedMap<Core::String, Core::String> OS::Process::GetArgs(int ArgsCount, char** Args, const Core::String& WhenNoValue)
        {
            Core::UnorderedMap<Core::String, Core::String> Results;
            VI_ASSERT(Args != nullptr, Results, "arguments should be set");
            VI_ASSERT(ArgsCount > 0, Results, "arguments count should be greater than zero");
            
            Core::Vector<Core::String> Params;
            for (int i = 0; i < ArgsCount; i++)
            {
                VI_ASSERT(Args[i] != nullptr, Results, "argument %i should be set", i);
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
                    if (Position != Core::String::npos)
                    {
                        Core::String Value = Item.substr(Position + 1);
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

		void* OS::Symbol::Load(const Core::String& Path)
		{
			VI_MEASURE(Core::Timings::FileSystem);
			Stringify Name(Path);
#ifdef VI_MICROSOFT
			if (Path.empty())
				return GetModuleHandle(nullptr);

			if (!Name.EndsWith(".dll"))
				Name.Append(".dll");

			VI_DEBUG("[dl] load dll library %s", Name.Get());
			return (void*)LoadLibrary(Name.Get());
#elif defined(VI_APPLE)
			if (Path.empty())
				return (void*)dlopen(nullptr, RTLD_LAZY);

			if (!Name.EndsWith(".dylib"))
				Name.Append(".dylib");

			VI_DEBUG("[dl] load dylib library %s", Name.Get());
			return (void*)dlopen(Name.Get(), RTLD_LAZY);
#elif defined(VI_LINUX)
			if (Path.empty())
				return (void*)dlopen(nullptr, RTLD_LAZY);

			if (!Name.EndsWith(".so"))
				Name.Append(".so");

			VI_DEBUG("[dl] load so library %s", Name.Get());
			return (void*)dlopen(Name.Get(), RTLD_LAZY);
#else
			return nullptr;
#endif
		}
		void* OS::Symbol::LoadFunction(void* Handle, const Core::String& Name)
		{
			VI_ASSERT(Handle != nullptr && !Name.empty(), nullptr, "handle should be set and name should not be empty");
			VI_DEBUG("[dl] load function %s", Name.c_str());
			VI_MEASURE(Core::Timings::FileSystem);
#ifdef VI_MICROSOFT
			void* Result = (void*)GetProcAddress((HMODULE)Handle, Name.c_str());
			if (!Result)
			{
				LPVOID Buffer;
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&Buffer, 0, nullptr);
				LPVOID Display = (LPVOID)LocalAlloc(LMEM_ZEROINIT, (::lstrlen((LPCTSTR)Buffer) + 40) * sizeof(TCHAR));
				Core::String Text((LPCTSTR)Display);
				LocalFree(Buffer);
				LocalFree(Display);

				if (!Text.empty())
					VI_ERR("[dlerr] %s", Text.c_str());
			}

			return Result;
#elif defined(VI_LINUX)
			void* Result = (void*)dlsym(Handle, Name.c_str());
			if (!Result)
			{
				const char* Text = dlerror();
				if (Text != nullptr)
					VI_ERR("[dlerr] %s", Text);
			}

			return Result;
#else
			return nullptr;
#endif
		}
		bool OS::Symbol::Unload(void* Handle)
		{
			VI_ASSERT(Handle != nullptr, false, "handle should be set");
			VI_MEASURE(Core::Timings::FileSystem);
			VI_DEBUG("[dl] unload library 0x%" PRIXPTR, Handle);
#ifdef VI_MICROSOFT
			return (FreeLibrary((HMODULE)Handle) != 0);
#elif defined(VI_LINUX)
			return (dlclose(Handle) == 0);
#else
			return false;
#endif
		}

		bool OS::Input::Text(const Core::String& Title, const Core::String& Message, const Core::String& DefaultInput, Core::String* Result)
		{
			VI_TRACE("[dg] open input { title: %s, message: %s }", Title.c_str(), Message.c_str());
			const char* Data = tinyfd_inputBox(Title.c_str(), Message.c_str(), DefaultInput.c_str());
			if (!Data)
				return false;

			VI_TRACE("[dg] close input: %s", Data ? Data : "NULL");
			if (Result != nullptr)
				*Result = Data;

			return true;
		}
		bool OS::Input::Password(const Core::String& Title, const Core::String& Message, Core::String* Result)
		{
			VI_TRACE("[dg] open password { title: %s, message: %s }", Title.c_str(), Message.c_str());
			const char* Data = tinyfd_inputBox(Title.c_str(), Message.c_str(), nullptr);
			if (!Data)
				return false;

			VI_TRACE("[dg] close password: %s", Data ? Data : "NULL");
			if (Result != nullptr)
				*Result = Data;

			return true;
		}
		bool OS::Input::Save(const Core::String& Title, const Core::String& DefaultPath, const Core::String& Filter, const Core::String& FilterDescription, Core::String* Result)
		{
			Core::Vector<Core::String> Sources = Stringify(&Filter).Split(',');
			Core::Vector<char*> Patterns;
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
		}
		bool OS::Input::Open(const Core::String& Title, const Core::String& DefaultPath, const Core::String& Filter, const Core::String& FilterDescription, bool Multiple, Core::String* Result)
		{
			Core::Vector<Core::String> Sources = Stringify(&Filter).Split(',');
			Core::Vector<char*> Patterns;
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
		}
		bool OS::Input::Folder(const Core::String& Title, const Core::String& DefaultPath, Core::String* Result)
		{
			VI_TRACE("[dg] open folder { title: %s }", Title.c_str());
			const char* Data = tinyfd_selectFolderDialog(Title.c_str(), DefaultPath.c_str());
			if (!Data)
				return false;

			VI_TRACE("[dg] close folder: %s", Data ? Data : "NULL");
			if (Result != nullptr)
				*Result = Data;

			return true;
		}
		bool OS::Input::Color(const Core::String& Title, const Core::String& DefaultHexRGB, Core::String* Result)
		{
			VI_TRACE("[dg] open color { title: %s }", Title.c_str());
			unsigned char RGB[3] = { 0, 0, 0 };
			const char* Data = tinyfd_colorChooser(Title.c_str(), DefaultHexRGB.c_str(), RGB, RGB);
			if (!Data)
				return false;

			VI_TRACE("[dg] close color: %s", Data ? Data : "NULL");
			if (Result != nullptr)
				*Result = Data;

			return true;
		}
#ifndef NDEBUG
		static thread_local std::stack<Measurement> MeasuringTree;
		static thread_local bool MeasuringDisabled = false;
#endif
		int OS::Error::Get()
		{
#ifdef VI_MICROSOFT
			int ErrorCode = WSAGetLastError();
			WSASetLastError(0);

			return ErrorCode;
#else
			int ErrorCode = errno;
			errno = 0;

			return ErrorCode;
#endif
		}
		Core::String OS::Error::GetName(int Code)
		{
#ifdef VI_MICROSOFT
			LPSTR Buffer = nullptr;
			size_t Size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, Code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&Buffer, 0, nullptr);
			Core::String Result(Buffer, Size);
			LocalFree(Buffer);

			return Stringify(&Result).Replace("\r", "").Replace("\n", "").R();
#else
			char* Buffer = strerror(Code);
			return Buffer ? Buffer : "";
#endif
		}
		bool OS::Error::IsError(int Code)
		{
#ifdef VI_MICROSOFT
			return Code != ERROR_SUCCESS;
#else
			return Code > 0;
#endif
		}
		const char* OS::Message::GetLevelName() const
		{
			switch (Level)
			{
				case (int)LogLevel::Error:
					return "ERROR";
				case (int)LogLevel::Warning:
					return "WARN";
				case (int)LogLevel::Info:
					return "INFO";
				case (int)LogLevel::Debug:
					return "DEBUG";
				case (int)LogLevel::Trace:
					return "TRACE";
				default:
					return "LOG";
			}
		}
		StdColor OS::Message::GetLevelColor() const
		{
			switch (Level)
			{
				case (int)LogLevel::Error:
					return StdColor::DarkRed;
				case (int)LogLevel::Warning:
					return StdColor::Orange;
				case (int)LogLevel::Info:
					return StdColor::LightBlue;
				case (int)LogLevel::Debug:
					return StdColor::Gray;
				case (int)LogLevel::Trace:
					return StdColor::Gray;
				default:
					return StdColor::LightGray;
			}
		}
		Core::String& OS::Message::GetText()
		{
			if (!Temp.empty())
				return Temp;

			Core::StringStream Stream;
			Stream << Date;
#ifndef NDEBUG
			Stream << ' ' << Source << ':' << Line;
#endif
			Stream << ' ' << GetLevelName() << ' ';
			Stream << Buffer << '\n';

			Temp = Stream.str();
			return Temp;
		}

		OS::Timing::Tick::Tick(bool Active) noexcept : IsCounting(Active)
		{
		}
		OS::Timing::Tick::Tick(Tick&& Other) noexcept : IsCounting(Other.IsCounting)
		{
			Other.IsCounting = false;
		}
		OS::Timing::Tick::~Tick() noexcept
		{
#ifndef NDEBUG
			if (MeasuringDisabled || !IsCounting)
				return;

			VI_ASSERT_V(!MeasuringTree.empty(), "debug frame should be set");
			auto& Next = MeasuringTree.top();
			Next.NotifyOfOverConsumption();
			MeasuringTree.pop();
#endif
		}
		OS::Timing::Tick& OS::Timing::Tick::operator =(Tick&& Other) noexcept
		{
			if (&Other == this)
				return *this;

			IsCounting = Other.IsCounting;
			Other.IsCounting = false;
			return *this;
		}
		OS::Timing::Tick OS::Timing::Measure(const char* File, const char* Function, int Line, uint64_t ThresholdMS)
		{
#ifndef NDEBUG
			VI_ASSERT(File != nullptr, OS::Timing::Tick(false), "file should be set");
			VI_ASSERT(Function != nullptr, OS::Timing::Tick(false), "function should be set");
			VI_ASSERT(ThresholdMS > 0 || ThresholdMS == (uint64_t)Core::Timings::Infinite, OS::Timing::Tick(false), "threshold time should be greater than Zero");
			if (MeasuringDisabled)
				return OS::Timing::Tick(false);

			Measurement Next;
			Next.File = File;
			Next.Function = Function;
			Next.Threshold = ThresholdMS * 1000;
			Next.Time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			Next.Line = Line;

			MeasuringTree.emplace(std::move(Next));
			return OS::Timing::Tick(true);
#else
			return OS::Timing::Tick(false);
#endif
		}
		void OS::Timing::MeasureLoop()
		{
#ifndef NDEBUG
			if (!MeasuringDisabled)
			{
				VI_ASSERT_V(!MeasuringTree.empty(), "debug frame should be set");
				auto& Next = MeasuringTree.top();
				Next.NotifyOfOverConsumption();
			}
#endif
		}
		Core::String OS::Timing::GetMeasureTrace()
		{
#ifndef NDEBUG
			auto Source = MeasuringTree;
			Core::StringStream Stream;
			Stream << "in thread " << std::this_thread::get_id() << ":\n";

			size_t Size = Source.size();
			for (size_t TraceIdx = Source.size(); TraceIdx > 0; --TraceIdx)
			{
				auto& Next = Source.top();
				Stream << "\t#" << (Size - TraceIdx) + 1 << " source \"" << Next.File << "\", line " << Next.Line << ", in " << Next.Function;
				if (Next.Id != nullptr)
					Stream << " (0x" << Next.Id;
				else
					Stream << " (nullptr";
				Stream << "), at most " << Next.Threshold / 1000 << " ms\n";
				Source.pop();
			}

			Core::String Out(Stream.str());
			return Out.substr(0, Out.size() - 1);
#else
			return Core::String();
#endif
		}

		static thread_local bool IgnoreLogging = false;
		void OS::Assert(bool Fatal, int Line, const char* Source, const char* Function, const char* Condition, const char* Format, ...)
		{
			if (!Active && !Callback)
				return;

			Message Data;
			Data.Fatal = Fatal;
			Data.Level = 1;
			Data.Line = Line;
			Data.Source = Source ? OS::Path::GetFilename(Source) : "";
			Data.Pretty = true;
			GetDateTime(time(nullptr), Data.Date, sizeof(Data.Date));

			char Buffer[BLOB_SIZE] = { '\0' };
			Data.Size = snprintf(Buffer, sizeof(Buffer),
				"%s(): \"%s\" assertion failed\n\tdetails: %s\n\texecution flow dump: %s",
				Function ? Function : "anonymous",
				Condition ? Condition : "unknown assertion",
				Format ? Format : "no additional data",
				OS::GetStackTrace(2, 64).c_str());

			if (Format != nullptr)
			{
				va_list Args;
				va_start(Args, Format);
				Data.Size = vsnprintf(Data.Buffer, sizeof(Data.Buffer), Buffer, Args);
				va_end(Args);
			}
			else
				memcpy(Data.Buffer, Buffer, sizeof(Buffer));

			if (Data.Size > 0)
			{
				EscapeText(Data.Buffer, (size_t)Data.Size);
				EnqueueLog(std::move(Data));
			}

			if (Fatal)
				Pause();
		}
		void OS::Log(int Level, int Line, const char* Source, const char* Format, ...)
		{
			if (!Format || IgnoreLogging || (!Active && !Callback))
				return;

			Message Data;
			Data.Fatal = false;
			Data.Level = Level;
			Data.Line = Line;
			Data.Source = Source ? OS::Path::GetFilename(Source) : "";
#if VI_DLEVEL >= 5
			Data.Pretty = Level != (int)LogLevel::Trace;
#else
			Data.Pretty = Level != (int)LogLevel::Debug && Level != (int)LogLevel::Trace;
#endif
			GetDateTime(time(nullptr), Data.Date, sizeof(Data.Date));

			char Buffer[512] = { '\0' };
			if (Level == 1)
			{
				int ErrorCode = OS::Error::Get();
				if (OS::Error::IsError(ErrorCode))
					snprintf(Buffer, sizeof(Buffer), "%s\n\tsystem: %s", Format, OS::Error::GetName(ErrorCode).c_str());
				else
					memcpy(Buffer, Format, std::min(sizeof(Buffer), strlen(Format)));
			}
			else
				memcpy(Buffer, Format, std::min(sizeof(Buffer), strlen(Format)));

			va_list Args;
			va_start(Args, Format);
			Data.Size = vsnprintf(Data.Buffer, sizeof(Data.Buffer), Buffer, Args);
			va_end(Args);

			if (Data.Size > 0)
			{
				EscapeText(Data.Buffer, (size_t)Data.Size);
				EnqueueLog(std::move(Data));
			}
		}
		void OS::EnqueueLog(Message&& Data)
		{
			if (Deferred && Schedule::IsPresentAndActive())
			{
				Schedule::Get()->SetTask([Data = std::move(Data)]() mutable
				{
					DispatchLog(Data);
				});
			}
			else
				DispatchLog(Data);
		}
		void OS::DispatchLog(Message& Data)
		{
			if (Callback)
			{
				IgnoreLogging = true;
#ifndef NDEBUG
				if (!MeasuringDisabled)
				{
					MeasuringDisabled = true;
					Callback(Data);
					MeasuringDisabled = false;
				}
#else
				Callback(Data);
#endif
				IgnoreLogging = false;
			}

			if (!Active)
				return;
#if defined(VI_MICROSOFT) && !defined(NDEBUG)
			OutputDebugStringA(Data.GetText().c_str());
#endif
			if (!Console::IsPresent())
				return;

			Console* Log = Console::Get();
			Log->Begin();
			if (Pretty)
			{
				Log->ColorBegin(Data.Pretty ? StdColor::Cyan : StdColor::Gray);
				Log->WriteBuffer(Data.Date);
				Log->WriteBuffer(" ");
#ifndef NDEBUG
				Log->ColorBegin(StdColor::Gray);
				Log->WriteBuffer(Data.Source);
				Log->WriteBuffer(":");
				Log->Write(Core::ToString(Data.Line));
				Log->WriteBuffer(" ");
#endif
				Log->ColorBegin(Data.GetLevelColor());
				Log->WriteBuffer(Data.GetLevelName());
				Log->WriteBuffer(" ");
				if (Data.Pretty)
					PrettyPrintLog(Log, Data.Buffer, StdColor::LightGray);
				else
					Log->WriteBuffer(Data.Buffer);
				Log->WriteBuffer("\n");
				Log->ColorEnd();
			}
			else
				Log->Write(Data.GetText());
			Log->End();
		}
		void OS::PrettyPrintLog(Console* Log, const char* Buffer, StdColor BaseColor)
		{
			static PrettyToken Tokens[] =
			{
				PrettyToken(StdColor::DarkGreen, "OK"),
				PrettyToken(StdColor::Yellow, "execute"),
				PrettyToken(StdColor::Yellow, "spawn"),
				PrettyToken(StdColor::Yellow, "acquire"),
				PrettyToken(StdColor::Yellow, "release"),
				PrettyToken(StdColor::Yellow, "join"),
				PrettyToken(StdColor::Yellow, "bind"),
				PrettyToken(StdColor::Yellow, "assign"),
				PrettyToken(StdColor::Yellow, "resolve"),
				PrettyToken(StdColor::Yellow, "prepare"),
				PrettyToken(StdColor::Yellow, "listen"),
				PrettyToken(StdColor::Yellow, "unlisten"),
				PrettyToken(StdColor::Yellow, "accept"),
				PrettyToken(StdColor::Yellow, "load"),
				PrettyToken(StdColor::Yellow, "save"),
				PrettyToken(StdColor::Yellow, "open"),
				PrettyToken(StdColor::Yellow, "close"),
				PrettyToken(StdColor::Yellow, "create"),
				PrettyToken(StdColor::Yellow, "remove"),
				PrettyToken(StdColor::Yellow, "compile"),
				PrettyToken(StdColor::Yellow, "transpile"),
				PrettyToken(StdColor::Yellow, "enter"),
				PrettyToken(StdColor::Yellow, "exit"),
				PrettyToken(StdColor::Yellow, "connect"),
				PrettyToken(StdColor::Yellow, "reconnect"),
				PrettyToken(StdColor::DarkRed, "ERR"),
				PrettyToken(StdColor::DarkRed, "FATAL"),
				PrettyToken(StdColor::DarkRed, "leak"),
				PrettyToken(StdColor::DarkRed, "leaking"),
				PrettyToken(StdColor::DarkRed, "fail"),
				PrettyToken(StdColor::DarkRed, "failure"),
				PrettyToken(StdColor::DarkRed, "failed"),
				PrettyToken(StdColor::DarkRed, "error"),
				PrettyToken(StdColor::DarkRed, "errors"),
				PrettyToken(StdColor::DarkRed, "not"),
				PrettyToken(StdColor::DarkRed, "cannot"),
				PrettyToken(StdColor::DarkRed, "could"),
				PrettyToken(StdColor::DarkRed, "couldn't"),
				PrettyToken(StdColor::DarkRed, "wasn't"),
				PrettyToken(StdColor::DarkRed, "took"),
				PrettyToken(StdColor::DarkRed, "missing"),
				PrettyToken(StdColor::DarkRed, "invalid"),
				PrettyToken(StdColor::DarkRed, "required"),
				PrettyToken(StdColor::DarkRed, "already"),
				PrettyToken(StdColor::Cyan, "undefined"),
				PrettyToken(StdColor::Cyan, "nullptr"),
				PrettyToken(StdColor::Cyan, "null"),
				PrettyToken(StdColor::Cyan, "this"),
				PrettyToken(StdColor::Cyan, "ms"),
				PrettyToken(StdColor::Cyan, "us"),
				PrettyToken(StdColor::Cyan, "ns"),
				PrettyToken(StdColor::Cyan, "on"),
				PrettyToken(StdColor::Cyan, "from"),
				PrettyToken(StdColor::Cyan, "to"),
				PrettyToken(StdColor::Cyan, "for"),
				PrettyToken(StdColor::Cyan, "and"),
				PrettyToken(StdColor::Cyan, "or"),
				PrettyToken(StdColor::Cyan, "at"),
				PrettyToken(StdColor::Cyan, "in"),
				PrettyToken(StdColor::Cyan, "of"),
				PrettyToken(StdColor::Magenta, "query"),
				PrettyToken(StdColor::Blue, "template"),
			};

			const char* Text = Buffer;
			size_t Size = strlen(Buffer);
			size_t Offset = 0;

			Log->ColorBegin(BaseColor);
			while (Text[Offset] != '\0')
			{
				auto& V = Text[Offset];
				if (Stringify::IsDigit(V))
				{
					Log->ColorBegin(StdColor::Yellow);
					while (Offset < Size)
					{
						auto N = std::tolower(Text[Offset]);
						if (!Stringify::IsDigit(N) && N != '.' && N != 'a' && N != 'b' && N != 'c' && N != 'd' && N != 'e' && N != 'f' && N != 'x')
							break;

						Log->WriteChar(Text[Offset++]);
					}

					Log->ColorBegin(BaseColor);
					continue;
				}
				else if (V == '@')
				{
					Log->ColorBegin(StdColor::LightBlue);
					Log->WriteChar(V);

					while (Offset < Size && (Stringify::IsDigit(Text[++Offset]) || Stringify::IsAlphabetic(Text[Offset]) || Text[Offset] == '-' || Text[Offset] == '_'))
						Log->WriteChar(Text[Offset]);

					Log->ColorBegin(BaseColor);
					continue;
				}
				else if (V == '[' && strstr(Text + Offset + 1, "]") != nullptr)
				{
					size_t Iterations = 0, Skips = 0;
					Log->ColorBegin(StdColor::Cyan);
					do
					{
						Log->WriteChar(Text[Offset]);
						if (Iterations++ > 0 && Text[Offset] == '[')
							Skips++;
					} while (Offset < Size && (Text[Offset++] != ']' || Skips > 0));

					Log->ColorBegin(BaseColor);
					continue;
				}
				else if (V == '\"' && strstr(Text + Offset + 1, "\"") != nullptr)
				{
					Log->ColorBegin(StdColor::LightBlue);
					do
					{
						Log->WriteChar(Text[Offset]);
					} while (Offset < Size && Text[++Offset] != '\"');

					if (Offset < Size)
						Log->WriteChar(Text[Offset++]);
					Log->ColorBegin(BaseColor);
					continue;
				}
				else if (V == '\'' && strstr(Text + Offset + 1, "\'") != nullptr)
				{
					Log->ColorBegin(StdColor::LightBlue);
					do
					{
						Log->WriteChar(Text[Offset]);
					} while (Offset < Size && Text[++Offset] != '\'');

					if (Offset < Size)
						Log->WriteChar(Text[Offset++]);
					Log->ColorBegin(BaseColor);
					continue;
				}
				else if (Stringify::IsAlphabetic(V) && (!Offset || !Stringify::IsAlphabetic(Text[Offset - 1])))
				{
					bool IsMatched = false;
					for (size_t i = 0; i < sizeof(Tokens) / sizeof(PrettyToken); i++)
					{
						auto& Token = Tokens[i];
						if (V != Token.First || Size - Offset < Token.Size)
							continue;

						if (Offset + Token.Size < Size && Stringify::IsAlphabetic(Text[Offset + Token.Size]))
							continue;

						if (memcmp(Text + Offset, Token.Token, Token.Size) == 0)
						{
							Log->ColorBegin(Token.Color);
							for (size_t j = 0; j < Token.Size; j++)
								Log->WriteChar(Text[Offset++]);

							Log->ColorBegin(BaseColor);
							IsMatched = true;
							break;
						}
					}

					if (IsMatched)
						continue;
				}

				Log->WriteChar(V);
				++Offset;
			}
		}
		void OS::Pause()
		{
			OS::Process::Interrupt();
		}
		void OS::SetLogCallback(const std::function<void(Message&)>& _Callback)
		{
			Callback = _Callback;
		}
		void OS::SetLogActive(bool Enabled)
		{
			Active = Enabled;
		}
		void OS::SetLogDeferred(bool Enabled)
		{
			Deferred = Enabled;
		}
        void OS::SetLogPretty(bool Enabled)
        {
            Pretty = Enabled;
        }
		bool OS::IsLogActive()
		{
			return Active;
		}
		bool OS::IsLogDeferred()
		{
			return Deferred;
		}
		bool OS::IsLogPretty()
		{
			return Pretty;
		}
		Core::String OS::GetStackTrace(size_t Skips, size_t MaxFrames)
		{
			static bool IsPreloaded = false;
			if (!IsPreloaded)
			{
				IsPreloaded = true;
				GetStackTrace(0, 8);
			}

			backward::StackTrace Stack;
			Stack.load_here(MaxFrames + Skips);
			Stack.skip_n_firsts(Skips);
			return GetStack(Stack);
		}
		std::function<void(OS::Message&)> OS::Callback;
		std::mutex OS::Buffer;
		bool OS::Active = false;
		bool OS::Deferred = false;
        bool OS::Pretty = true;

		FileLog::FileLog(const Core::String& Root) noexcept : Offset(-1), Time(0), Path(Root)
		{
			Source = new FileStream();
			auto V = Stringify(&Path).Replace('\\', '/').Split('/');
			if (!V.empty())
				Name = V.back();
		}
		FileLog::~FileLog() noexcept
		{
			VI_RELEASE(Source);
		}
		void FileLog::Process(const std::function<bool(FileLog*, const char*, int64_t)>& Callback)
		{
			VI_ASSERT_V(Callback, "callback should not be empty");
			FileEntry State;
			if (Source->GetBuffer() && (!OS::File::State(Path, &State) || State.LastModified == Time))
				return;

			Source->Open(Path.c_str(), FileMode::Binary_Read_Only);
			Time = State.LastModified;

			size_t Length = Source->GetSize();
			if (Length <= Offset || Offset <= 0)
			{
				Offset = Length;
				return;
			}

			size_t Delta = Length - Offset;
			Source->Seek(FileSeek::Begin, Length - Delta);

			char* Data = VI_MALLOC(char, sizeof(char) * (Delta + 1));
			Source->Read(Data, sizeof(char) * Delta);

			Core::String Value = Data;
			int64_t ValueLength = -1;
			for (size_t i = Value.size(); i > 0; i--)
			{
				if (Value[i] == '\n' && ValueLength == -1)
					ValueLength = i;

				if ((int)Value[i] < 0)
					Value[i] = ' ';
			}

			if (ValueLength == -1)
				ValueLength = (int64_t)Value.size();

			auto V = Stringify(&Value).Substring(0, (size_t)ValueLength).Replace("\t", "\n").Replace("\n\n", "\n").Replace("\r", "");
			VI_FREE(Data);

			if (Value == LastValue)
				return;

			LastValue = Value;
			if (V.Find("\n").Found)
			{
				Core::Vector<Core::String> Lines = V.Split('\n');
				for (auto& Line : Lines)
				{
					if (Line.empty())
						continue;

					if (!Callback(this, Line.c_str(), (int64_t)Line.size()))
					{
						Offset = Length;
						return;
					}
				}
			}
			else if (!Value.empty())
				Callback(this, Value.c_str(), (int64_t)Value.size());

			Offset = Length;
		}

		static thread_local Costate* Cothread = nullptr;
		Costate::Costate(size_t StackSize) noexcept : Thread(std::this_thread::get_id()), Current(nullptr), Master(VI_NEW(Cocontext)), Size(StackSize)
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
			VI_ASSERT(Thread == std::this_thread::get_id(), nullptr, "cannot call outside costate thread");

			Coroutine* Routine = nullptr;
			if (!Cached.empty())
			{
				Routine = *Cached.begin();
				Routine->Callback = Procedure;
				Routine->State = Coactive::Active;
				Cached.erase(Cached.begin());
			}
			else
				Routine = VI_NEW(Coroutine, this, Procedure);

			Used.emplace(Routine);
			return Routine;
		}
		Coroutine* Costate::Pop(TaskCallback&& Procedure)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), nullptr, "cannot deactive coroutine outside costate thread");

			Coroutine* Routine = nullptr;
			if (!Cached.empty())
			{
				Routine = *Cached.begin();
				Routine->Callback = std::move(Procedure);
				Routine->State = Coactive::Active;
				Cached.erase(Cached.begin());
			}
			else
				Routine = VI_NEW(Coroutine, this, std::move(Procedure));

			Used.emplace(Routine);
			return Routine;
		}
		int Costate::Reuse(Coroutine* Routine, const TaskCallback& Procedure)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot call outside costate thread");
			VI_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			VI_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			VI_ASSERT(Routine->Dead > 0, -1, "coroutine should be dead");

			Routine->Callback = Procedure;
			Routine->Return = nullptr;
			Routine->Dead = 0;
			Routine->State = Coactive::Active;
			return 1;
		}
		int Costate::Reuse(Coroutine* Routine, TaskCallback&& Procedure)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot call outside costate thread");
			VI_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			VI_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			VI_ASSERT(Routine->Dead > 0, -1, "coroutine should be dead");

			Routine->Callback = std::move(Procedure);
			Routine->Return = nullptr;
			Routine->Dead = 0;
			Routine->State = Coactive::Active;
			return 1;
		}
		int Costate::Reuse(Coroutine* Routine)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot call outside costate thread");
			VI_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			VI_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			VI_ASSERT(Routine->Dead > 0, -1, "coroutine should be dead");

			Routine->Callback = nullptr;
			Routine->Return = nullptr;
			Routine->Dead = 0;
			Routine->State = Coactive::Active;

			Used.erase(Routine);
			Cached.emplace(Routine);

			return 1;
		}
		int Costate::Swap(Coroutine* Routine)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot call outside costate thread");
			VI_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			VI_ASSERT(Routine->Dead < 1, -1, "coroutine should not be dead");

			if (Routine->State == Coactive::Inactive)
				return 0;

			if (Routine->State == Coactive::Resumable)
				Routine->State = Coactive::Active;

			Cocontext* Fiber = Routine->Slave;
			Current = Routine;
#ifdef VI_USE_FCTX
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

			return Routine->Dead > 0 ? -1 : 1;
		}
		int Costate::Push(Coroutine* Routine)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot call outside costate thread");
			VI_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			VI_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			VI_ASSERT(Routine->Dead > 0, -1, "coroutine should be dead");

			Cached.erase(Routine);
			Used.erase(Routine);

			VI_DELETE(Coroutine, Routine);
			return 1;
		}
		int Costate::Activate(Coroutine* Routine)
		{
			VI_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			VI_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			VI_ASSERT(Routine->Dead < 1, -1, "coroutine should not be dead");

			bool MustLock = (Thread != std::this_thread::get_id());
			if (MustLock && NotifyLock)
				NotifyLock();

			int Result = (Routine->State == Coactive::Inactive ? 1 : -1);
			if (Result == 1)
				Routine->State = Coactive::Resumable;

			if (MustLock && NotifyUnlock)
				NotifyUnlock();

			return Result;
		}
		int Costate::Deactivate(Coroutine* Routine)
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot deactive coroutine outside costate thread");
			VI_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			VI_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");
			VI_ASSERT(Routine->Dead < 1, -1, "coroutine should not be dead");

			if (Current != Routine || Routine->State != Coactive::Active)
				return -1;

			Routine->State = Coactive::Inactive;
			return Suspend();
		}
		int Costate::Deactivate(Coroutine* Routine, const TaskCallback& AfterSuspend)
		{
			Routine->Return = AfterSuspend;
			return Deactivate(Routine);
		}
		int Costate::Deactivate(Coroutine* Routine, TaskCallback&& AfterSuspend)
		{
			Routine->Return = std::move(AfterSuspend);
			return Deactivate(Routine);
		}
		int Costate::Resume(Coroutine* Routine)
		{
			VI_ASSERT(Routine != nullptr, -1, "coroutine should be set");
			VI_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot resume coroutine outside costate thread");
			VI_ASSERT(Routine->Master == this, -1, "coroutine should be created by this costate");

			if (Current == Routine || Routine->Dead > 0)
				return -1;

			return Swap(Routine);
		}
		int Costate::Dispatch()
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot dispatch coroutine outside costate thread");
			size_t Activities = 0;
		Reset:
			for (auto* Routine : Used)
			{
				int Status = Swap(Routine);
				if (Status == 0)
					continue;

				++Activities;
				if (Status != -1)
					continue;

				Reuse(Routine);
				goto Reset;
			}

			return (int)Activities;
		}
		int Costate::Suspend()
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), -1, "cannot suspend coroutine outside costate thread");

			Coroutine* Routine = Current;
			if (!Routine || Routine->Master != this)
				return -1;
#ifdef VI_USE_FCTX
			Current = nullptr;
			jump_fcontext(Master->Context, (void*)this);
#elif VI_MICROSOFT
			Current = nullptr;
			SwitchToFiber(Master->Context);
#else
			char Bottom = 0;
			char* Top = Routine->Slave->Stack + Size;
			if (size_t(Top - &Bottom) > Size)
				return -1;

			Current = nullptr;
			swapcontext(&Routine->Slave->Context, &Master->Context);
#endif
			return 1;
		}
		void Costate::Clear()
		{
			VI_ASSERT_V(Thread == std::this_thread::get_id(), "cannot call outside costate thread");
			for (auto& Routine : Cached)
				VI_DELETE(Coroutine, Routine);
			Cached.clear();
		}
		bool Costate::HasActive() const
		{
			VI_ASSERT(Thread == std::this_thread::get_id(), false, "cannot call outside costate thread");
			for (const auto& Item : Used)
			{
				if (Item->Dead == 0 && Item->State != Coactive::Inactive)
					return true;
			}

			return false;
		}
		Coroutine* Costate::GetCurrent() const
		{
			return Current;
		}
		size_t Costate::GetCount() const
		{
			return Used.size();
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
			VI_ASSERT(State != nullptr, false, "state should be set");
			VI_ASSERT(Routine != nullptr, false, "state should be set");
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
#ifdef VI_USE_FCTX
			transfer_t* Transfer = (transfer_t*)Context;
			Costate* State = (Costate*)Transfer->data;
			State->Master->Context = Transfer->fctx;
#elif VI_MICROSOFT
			Costate* State = (Costate*)Context;
#else
			Costate* State = (Costate*)Unpack2_64(X, Y);
#endif
			Cothread = State;
			VI_ASSERT_V(State != nullptr, "costate should be set");
			Coroutine* Routine = State->Current;
			if (Routine != nullptr)
			{
			Reuse:
				if (Routine->Callback)
					Routine->Callback();
				Routine->Return = nullptr;
				Routine->Dead = 1;
			}

			State->Current = nullptr;
#ifdef VI_USE_FCTX
			jump_fcontext(State->Master->Context, Context);
#elif VI_MICROSOFT
			SwitchToFiber(State->Master->Context);
#else
			swapcontext(&Routine->Slave->Context, &State->Master->Context);
#endif
			if (Routine != nullptr && Routine->Callback)
				goto Reuse;
		}

		Schedule::Desc::Desc()
		{
			auto Quantity = Core::OS::CPU::GetQuantityInfo();
			SetThreads(std::max<uint32_t>(2, Quantity.Logical) - 1);
		}
		void Schedule::Desc::SetThreads(size_t Cores)
		{
			size_t Clock = 1;
#ifdef VI_CXX20
			size_t Chain = 0;
			size_t Light = (size_t)(std::max((double)Cores * 0.35, 1.0));
			size_t Heavy = std::max<size_t>(Cores - Light, 1);
#else
			size_t Chain = (size_t)(std::max((double)Cores * 0.20, 1.0));
			size_t Light = (size_t)(std::max((double)Cores * 0.30, 1.0));
			size_t Heavy = std::max<size_t>(Cores - Light, 1);
#endif
			Threads[((size_t)Difficulty::Clock)] = Clock;
			Threads[((size_t)Difficulty::Coroutine)] = Chain;
			Threads[((size_t)Difficulty::Light)] = Light;
			Threads[((size_t)Difficulty::Heavy)] = Heavy;
			Coroutines = std::min<size_t>(Cores * 8, 256);
		}

		Schedule::Schedule() noexcept : Generation(0), Debug(nullptr), Enqueue(true), Terminate(false), Active(false), Immediate(false)
		{
			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
				Queues[i] = VI_NEW(ConcurrentQueuePtr);
		}
		Schedule::~Schedule() noexcept
		{
			Stop();
			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
				VI_DELETE(ConcurrentQueuePtr, Queues[i]);

			VI_RELEASE(Dispatcher.State);
			Scripting::VirtualMachine::CleanupThisThread();
			if (Singleton == this)
				Singleton = nullptr;
		}
		TaskId Schedule::GetTaskId()
		{
			TaskId Id = ++Generation;
			while (Id == INVALID_TASK_ID)
				Id = ++Generation;
			return Id;
		}
		TaskId Schedule::SetInterval(uint64_t Milliseconds, const TaskCallback& Callback, Difficulty Type)
		{
			VI_ASSERT(Callback, INVALID_TASK_ID, "callback should not be empty");
			return SetSeqInterval(Milliseconds, [Callback](size_t)
			{
				Callback();
			}, Type);
		}
		TaskId Schedule::SetInterval(uint64_t Milliseconds, TaskCallback&& Callback, Difficulty Type)
		{
			VI_ASSERT(Callback, INVALID_TASK_ID, "callback should not be empty");
			return SetSeqInterval(Milliseconds, [Callback = std::move(Callback)](size_t) mutable
			{
				Callback();
			}, Type);
		}
		TaskId Schedule::SetSeqInterval(uint64_t Milliseconds, const SeqTaskCallback& Callback, Difficulty Type)
		{
			VI_ASSERT(Type != Difficulty::Count, INVALID_TASK_ID, "difficulty should be set");
			VI_ASSERT(Callback, INVALID_TASK_ID, "callback should not be empty");

			if (!Enqueue || Immediate)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			PostDebug(Type, ThreadTask::EnqueueTimer, 1);
#endif
			VI_MEASURE(Core::Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Clock];
			auto Duration = std::chrono::microseconds(Milliseconds * 1000);
			auto Expires = GetClock() + Duration;
			auto Id = GetTaskId();
			{
				std::unique_lock<std::mutex> Lock(Queue->Update);
				Queue->Timers.emplace(std::make_pair(GetTimeout(Expires), Timeout(Callback, Duration, Id, true, Type)));
				Queue->Notify.notify_all();
			}
			Queue->Notify.notify_all();
			return Id;
		}
		TaskId Schedule::SetSeqInterval(uint64_t Milliseconds, SeqTaskCallback&& Callback, Difficulty Type)
		{
			VI_ASSERT(Type != Difficulty::Count, INVALID_TASK_ID, "difficulty should be set");
			VI_ASSERT(Callback, INVALID_TASK_ID, "callback should not be empty");

			if (!Enqueue || Immediate)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			PostDebug(Type, ThreadTask::EnqueueTimer, 1);
#endif
			VI_MEASURE(Core::Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Clock];
			auto Duration = std::chrono::microseconds(Milliseconds * 1000);
			auto Expires = GetClock() + Duration;
			auto Id = GetTaskId();
			{
				std::unique_lock<std::mutex> Lock(Queue->Update);
				Queue->Timers.emplace(std::make_pair(GetTimeout(Expires), Timeout(std::move(Callback), Duration, Id, true, Type)));
				Queue->Notify.notify_all();
			}
			Queue->Notify.notify_all();
			return Id;
		}
		TaskId Schedule::SetTimeout(uint64_t Milliseconds, const TaskCallback& Callback, Difficulty Type)
		{
			VI_ASSERT(Callback, INVALID_TASK_ID, "callback should not be empty");
			return SetSeqTimeout(Milliseconds, [Callback](size_t)
			{
				Callback();
			}, Type);
		}
		TaskId Schedule::SetTimeout(uint64_t Milliseconds, TaskCallback&& Callback, Difficulty Type)
		{
			VI_ASSERT(Callback, INVALID_TASK_ID, "callback should not be empty");
			return SetSeqTimeout(Milliseconds, [Callback = std::move(Callback)](size_t) mutable
			{
				Callback();
			}, Type);
		}
		TaskId Schedule::SetSeqTimeout(uint64_t Milliseconds, const SeqTaskCallback& Callback, Difficulty Type)
		{
			VI_ASSERT(Type != Difficulty::Count, INVALID_TASK_ID, "difficulty should be set");
			VI_ASSERT(Callback, INVALID_TASK_ID, "callback should not be empty");

			if (!Enqueue || Immediate)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			PostDebug(Type, ThreadTask::EnqueueTimer, 1);
#endif
			VI_MEASURE(Core::Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Clock];
			auto Duration = std::chrono::microseconds(Milliseconds * 1000);
			auto Expires = GetClock() + Duration;
			auto Id = GetTaskId();
			{
				std::unique_lock<std::mutex> Lock(Queue->Update);
				Queue->Timers.emplace(std::make_pair(GetTimeout(Expires), Timeout(Callback, Duration, Id, false, Type)));
				Queue->Notify.notify_all();
			}
			Queue->Notify.notify_all();
			return Id;
		}
		TaskId Schedule::SetSeqTimeout(uint64_t Milliseconds, SeqTaskCallback&& Callback, Difficulty Type)
		{
			VI_ASSERT(Type != Difficulty::Count, INVALID_TASK_ID, "difficulty should be set");
			VI_ASSERT(Callback, INVALID_TASK_ID, "callback should not be empty");

			if (!Enqueue || Immediate)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			PostDebug(Type, ThreadTask::EnqueueTimer, 1);
#endif
			VI_MEASURE(Core::Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Clock];
			auto Duration = std::chrono::microseconds(Milliseconds * 1000);
			auto Expires = GetClock() + Duration;
			auto Id = GetTaskId();
			{
				std::unique_lock<std::mutex> Lock(Queue->Update);
				Queue->Timers.emplace(std::make_pair(GetTimeout(Expires), Timeout(std::move(Callback), Duration, Id, false, Type)));
				Queue->Notify.notify_all();
			}
			Queue->Notify.notify_all();
			return Id;
		}
		bool Schedule::SetTask(const TaskCallback& Callback, Difficulty Type)
		{
			VI_ASSERT(Type != Difficulty::Count, false, "difficulty should be set");
			VI_ASSERT(Callback, false, "callback should not be empty");

			if (!Enqueue)
				return false;
			
			if (Immediate)
			{
				Callback();
				return true;
			}
#ifndef NDEBUG
			PostDebug(Type, ThreadTask::EnqueueTask, 1);
#endif
			VI_MEASURE(Core::Timings::Atomic);
			auto Queue = Queues[(size_t)Type];
			std::unique_lock<std::mutex> Lock(Queue->Update);
			Queue->Tasks.enqueue(Callback);
			Queue->Notify.notify_all();
			return true;
		}
		bool Schedule::SetTask(TaskCallback&& Callback, Difficulty Type)
		{
			VI_ASSERT(Type != Difficulty::Count, false, "difficulty should be set");
			VI_ASSERT(Callback, false, "callback should not be empty");

			if (!Enqueue)
				return false;

			if (Immediate)
			{
				Callback();
				return true;
			}
#ifndef NDEBUG
			PostDebug(Type, ThreadTask::EnqueueTask, 1);
#endif
			VI_MEASURE(Core::Timings::Atomic);
			auto Queue = Queues[(size_t)Type];
			Queue->Tasks.enqueue(std::move(Callback));
			Queue->Notify.notify_all();
			return true;
		}
		bool Schedule::SetCoroutine(const TaskCallback& Callback)
		{
			VI_ASSERT(Callback, false, "callback should not be empty");
			if (!Enqueue)
				return false;

			if (Immediate)
			{
				Callback();
				return true;
			}

			VI_MEASURE(Core::Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Coroutine];
			Queue->Tasks.enqueue(Callback);
			for (auto* Thread : Threads[(size_t)Difficulty::Coroutine])
				Thread->Notify.notify_all();
			return true;
		}
		bool Schedule::SetCoroutine(TaskCallback&& Callback)
		{
			VI_ASSERT(Callback, false, "callback should not be empty");
			if (!Enqueue)
				return false;

			if (Immediate)
			{
				Callback();
				return true;
			}
#ifndef NDEBUG
			PostDebug(Difficulty::Coroutine, ThreadTask::EnqueueCoroutine, 1);
#endif
			VI_MEASURE(Core::Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Coroutine];
			Queue->Tasks.enqueue(std::move(Callback));
			for (auto* Thread : Threads[(size_t)Difficulty::Coroutine])
				Thread->Notify.notify_all();
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
			VI_MEASURE(Core::Timings::Atomic);
			auto Queue = Queues[(size_t)Difficulty::Clock];
			std::unique_lock<std::mutex> Lock(Queue->Update);
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
		bool Schedule::Start(const Desc& NewPolicy)
		{
			VI_ASSERT(!Active, false, "queue should be stopped");
			VI_ASSERT(NewPolicy.Memory > 0, false, "stack memory should not be zero");
			VI_ASSERT(NewPolicy.Coroutines > 0, false, "there must be at least one coroutine");
			VI_TRACE("[schedule] start 0x%" PRIXPTR " on thread %s", (void*)this, OS::Process::GetThreadId(std::this_thread::get_id()).c_str());

			Policy = NewPolicy;
			Immediate = false;
			Active = true;

			if (!Policy.Parallel)
			{
				InitializeThread(0, 0);
				return true;
			}

			size_t Index = 0;
			for (size_t j = 0; j < Policy.Threads[(size_t)Difficulty::Coroutine]; j++)
				PushThread(Difficulty::Coroutine, Index++, j, false);

			for (size_t j = 0; j < Policy.Threads[(size_t)Difficulty::Light]; j++)
				PushThread(Difficulty::Light, Index++, j, false);

			for (size_t j = 0; j < Policy.Threads[(size_t)Difficulty::Heavy]; j++)
				PushThread(Difficulty::Heavy, Index++, j, false);

			return PushThread(Difficulty::Clock, 0, 0, Policy.Ping != nullptr);
		}
		bool Schedule::Stop()
		{
			VI_TRACE("[schedule] stop 0x%" PRIXPTR " on thread %s", (void*)this, OS::Process::GetThreadId(std::this_thread::get_id()).c_str());
			Exclusive.lock();
			if (!Active && !Terminate)
			{
				Exclusive.unlock();
				return false;
			}

			Active = Enqueue = false;
			Wakeup();

			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
			{
				for (auto* Thread : Threads[i])
				{
					if (!PopThread(Thread))
					{
						Terminate = true;
						Exclusive.unlock();
						return false;
					}
				}
			}

			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
			{
				auto* Queue = Queues[i];
				std::unique_lock<std::mutex> Lock(Queue->Update);
				Queue->Timers.clear();
			}

			Terminate = false;
			Enqueue = true;
			ChunkCleanup();
			Exclusive.unlock();

			return true;
		}
		bool Schedule::Wakeup()
		{
			VI_TRACE("[schedule] wakeup 0x%" PRIXPTR " on thread %s", (void*)this, OS::Process::GetThreadId(std::this_thread::get_id()).c_str());
			TaskCallback Dummy[EVENTS_SIZE * 2] = { nullptr };
			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
			{
				auto* Queue = Queues[i];
				Queue->Notify.notify_all();

				for (auto* Thread : Threads[i])
				{
					Thread->Notify.notify_all();
					Queue->Tasks.enqueue_bulk(Dummy, EVENTS_SIZE);
				}
			}

			return true;
		}
		bool Schedule::Dispatch()
		{
			size_t Passes = 0;
			VI_MEASURE(Core::Timings::Intensive);
			for (size_t i = 0; i < (size_t)Difficulty::Count; i++)
			{
				if (ProcessTick((Difficulty)i))
					++Passes;
			}

			return Passes > 0;
		}
		bool Schedule::ProcessTick(Difficulty Type)
		{
			auto* Queue = Queues[(size_t)Type];
			if (!Policy.Threads[(size_t)Type])
				return false;

			switch (Type)
			{
				case Difficulty::Clock:
				{
					if (Queue->Timers.empty())
						return false;

					auto Clock = GetClock();
					auto It = Queue->Timers.begin();
					if (It->first >= Clock)
						return true;
#ifndef NDEBUG
					PostDebug(Type, ThreadTask::ProcessTimer, 1);
#endif
					if (It->second.Alive && Active)
					{
						Timeout Next(std::move(It->second));
						Queue->Timers.erase(It);

						SetTask(std::bind(Next.Callback, ++Next.Invocations), Next.Type);
						Queue->Timers.emplace(std::make_pair(GetTimeout(Clock + Next.Expires), std::move(Next)));
					}
					else
					{
						SetTask(std::bind(It->second.Callback, ++It->second.Invocations), It->second.Type);
						Queue->Timers.erase(It);
					}
#ifndef NDEBUG
					PostDebug(Type, ThreadTask::Awake, 0);
#endif
					return true;
				}
				case Difficulty::Coroutine:
				{
					Dispatcher.Events.resize(Policy.Coroutines);
					if (!Dispatcher.State)
						Dispatcher.State = new Costate(Policy.Memory);

					size_t Pending = Dispatcher.State->GetCount();
					size_t Left = Policy.Coroutines - Pending;
					size_t Count = Left, Passes = Pending;

					while (Left > 0 && Count > 0)
					{
						Count = Queue->Tasks.try_dequeue_bulk(Dispatcher.Events.begin(), Left);
						Left -= Count;
						Passes += Count;
#ifndef NDEBUG
						PostDebug(Type, ThreadTask::ConsumeCoroutine, Count);
#endif
						for (size_t i = 0; i < Count; ++i)
						{
							TaskCallback& Data = Dispatcher.Events[i];
							if (Data != nullptr)
								Dispatcher.State->Pop(std::move(Data));
						}
					}
#ifndef NDEBUG
					PostDebug(Type, ThreadTask::ProcessCoroutine, Dispatcher.State->GetCount());
#endif
					while (Dispatcher.State->Dispatch() > 0)
						++Passes;
#ifndef NDEBUG
					PostDebug(Type, ThreadTask::Awake, 0);
#endif
					return Passes > 0;
				}
				case Difficulty::Light:
				case Difficulty::Heavy:
				{
					size_t Count = Queue->Tasks.try_dequeue_bulk(Dispatcher.Tasks, EVENTS_SIZE);
#ifndef NDEBUG
					PostDebug(Type, ThreadTask::ProcessTask, Count);
#endif
					for (size_t i = 0; i < Count; ++i)
					{
						VI_MEASURE(Type == Difficulty::Heavy ? Core::Timings::Intensive : Core::Timings::FileSystem);
						TaskCallback Data(std::move(Dispatcher.Tasks[i]));
						if (Data != nullptr)
							Data();
					}
#ifndef NDEBUG
					PostDebug(Type, ThreadTask::Awake, 0);
#endif
					return Count > 0;
				}
                default:
                    break;
			}

			return false;
		}
		bool Schedule::ProcessLoop(Difficulty Type, ThreadPtr* Thread)
		{
			auto* Queue = Queues[(size_t)Type];
			Core::String ThreadId = OS::Process::GetThreadId(Thread->Id);
			InitializeThread(Thread->GlobalIndex, Thread->LocalIndex);

			switch (Type)
			{
				case Difficulty::Clock:
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
						PostDebug(Thread, ThreadTask::Awake, 0);
#endif
						std::chrono::microseconds When = std::chrono::microseconds(0);
						if (!Queue->Timers.empty())
						{
							auto Clock = GetClock();
							auto It = Queue->Timers.begin();
							if (It->first <= Clock)
							{
#ifndef NDEBUG
								PostDebug(Thread, ThreadTask::ProcessTimer, 1);
#endif
								if (It->second.Alive)
								{
									Timeout Next(std::move(It->second));
									Queue->Timers.erase(It);

									SetTask(std::bind(Next.Callback, ++Next.Invocations), Next.Type);
									Queue->Timers.emplace(std::make_pair(GetTimeout(Clock + Next.Expires), std::move(Next)));
								}
								else
								{
									SetTask(std::bind(It->second.Callback, ++It->second.Invocations), It->second.Type);
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
						PostDebug(Thread, ThreadTask::Sleep, 0);
#endif
						Queue->Notify.wait_for(Lock, When);
					} while (ThreadActive(Thread));
					break;
				}
				case Difficulty::Coroutine:
				{
					if (Thread->Daemon)
						VI_DEBUG("[schedule] acquire thread %s (coroutines)", ThreadId.c_str());
					else
						VI_DEBUG("[schedule] spawn thread %s (coroutines)", ThreadId.c_str());

					ReceiveToken Token(Queue->Tasks);
					Costate* State = new Costate(Policy.Memory);
					State->NotifyLock = [Thread]()
					{
						Thread->Update.lock();
					};
					State->NotifyUnlock = [Thread]()
					{
						Thread->Notify.notify_all();
						Thread->Update.unlock();
					};

					Core::Vector<TaskCallback> Events;
					Events.resize(Policy.Coroutines);

					do
					{
						size_t Left = Policy.Coroutines - State->GetCount();
						size_t Count = Left;
#ifndef NDEBUG
						PostDebug(Thread, ThreadTask::Awake, 0);
#endif
						while (Left > 0 && Count > 0)
						{
							Count = Queue->Tasks.try_dequeue_bulk(Token, Events.begin(), Left);
							Left -= Count;
#ifndef NDEBUG
							PostDebug(Type, ThreadTask::EnqueueCoroutine, Count);
#endif
							for (size_t i = 0; i < Count; ++i)
							{
								TaskCallback& Data = Events[i];
								if (Data != nullptr)
									State->Pop(std::move(Data));
							}
						}
#ifndef NDEBUG
						PostDebug(Type, ThreadTask::ProcessCoroutine, State->GetCount());
#endif
						{
							VI_MEASURE(Core::Timings::Frame);
							State->Dispatch();
						}
#ifndef NDEBUG
						PostDebug(Thread, ThreadTask::Sleep, 0);
#endif
						std::unique_lock<std::mutex> Lock(Thread->Update);
						Thread->Notify.wait_for(Lock, Policy.Timeout, [this, Queue, State, Thread]()
						{
							if (!ThreadActive(Thread) || State->HasActive())
								return true;

							if (State->GetCount() >= Policy.Coroutines)
								return false;

							return Queue->Tasks.size_approx() > 0 || State->HasActive();
						});
					} while (ThreadActive(Thread));

					VI_RELEASE(State);
					break;
				}
				case Difficulty::Light:
				case Difficulty::Heavy:
				{
					if (Thread->Daemon)
						VI_DEBUG("[schedule] acquire thread %s (%s)", ThreadId.c_str(), Thread->Type == Difficulty::Light ? "non-blocking" : "blocking");
					else
						VI_DEBUG("[schedule] spawn thread %s (%s)", ThreadId.c_str(), Thread->Type == Difficulty::Light ? "non-blocking" : "blocking");

					ReceiveToken Token(Queue->Tasks);
					TaskCallback Events[EVENTS_SIZE];

					do
					{
#ifndef NDEBUG
						PostDebug(Thread, ThreadTask::Awake, 0);
#endif
						size_t Count = 0;
						do
						{
							Count = Queue->Tasks.try_dequeue_bulk(Token, Events, EVENTS_SIZE);
#ifndef NDEBUG
							PostDebug(Thread, ThreadTask::ProcessTask, Count);
#endif
							for (size_t i = 0; i < Count; ++i)
							{
								VI_MEASURE(Core::Timings::Intensive);
								TaskCallback Data(std::move(Events[i]));
								if (Data != nullptr)
									Data();
							}
						} while (Count > 0);
#ifndef NDEBUG
						PostDebug(Thread, ThreadTask::Sleep, 0);
#endif
						std::unique_lock<std::mutex> Lock(Queue->Update);
						Queue->Notify.wait_for(Lock, Policy.Timeout, [this, Queue, Thread]()
						{
							return Queue->Tasks.size_approx() > 0 || !ThreadActive(Thread);
						});
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
			ThreadPtr* Thread = VI_NEW(ThreadPtr);
			Thread->Daemon = IsDaemon;
			Thread->Type = Type;
			Thread->GlobalIndex = GlobalIndex;
			Thread->LocalIndex = LocalIndex;

			if (!Thread->Daemon)
			{
				Thread->Handle = std::thread(&Schedule::ProcessLoop, this, Type, Thread);
				Thread->Id = Thread->Handle.get_id();
			}
			else
				Thread->Id = std::this_thread::get_id();
#ifndef NDEBUG
			PostDebug(Thread, ThreadTask::Spawn, 0);
#endif
			Threads[(size_t)Type].emplace_back(Thread);
			return Thread->Daemon ? ProcessLoop(Type, Thread) : Thread->Handle.joinable();
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
			PostDebug(Thread, ThreadTask::Despawn, 0);
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
			VI_ASSERT(Type != Difficulty::Count, false, "difficulty should be set");
			auto* Queue = Queues[(size_t)Type];
			switch (Type)
			{
				case Difficulty::Coroutine:
				case Difficulty::Light:
				case Difficulty::Heavy:
					return Queue->Tasks.size_approx() > 0;
				case Difficulty::Clock:
					return !Queue->Timers.empty();
				default:
					return false;
			}
		}
		bool Schedule::HasAnyTasks() const
		{
			return HasTasks(Difficulty::Light) || HasTasks(Difficulty::Heavy) || HasTasks(Difficulty::Coroutine) || HasTasks(Difficulty::Clock);
		}
		bool Schedule::PostDebug(Difficulty Type, ThreadTask State, size_t Tasks)
		{
			if (!Debug)
				return false;

			ThreadDebug Data;
			Data.Id = std::this_thread::get_id();
			Data.Type = Type;
			Data.State = State;
			Data.Tasks = Tasks;

			Debug(Data);
			return true;
		}
		bool Schedule::PostDebug(ThreadPtr* Ptr, ThreadTask State, size_t Tasks)
		{
			if (!Debug)
				return false;

			ThreadDebug Data;
			Data.Id = Ptr->Id;
			Data.Type = Ptr->Type;
			Data.State = State;
			Data.Tasks = Tasks;

			Debug(Data);
			return true;
		}
		void Schedule::InitializeThread(size_t GlobalIndex, size_t LocalIndex)
		{
			UpdateThreadGlobalCounter((int64_t)GlobalIndex);
			UpdateThreadLocalCounter((int64_t)LocalIndex);
		}
		size_t Schedule::UpdateThreadGlobalCounter(int64_t NewIndex)
		{
			static thread_local size_t Index = (size_t)-1;
			if (NewIndex != -2)
				Index = NewIndex;
			return Index;
		}
		size_t Schedule::UpdateThreadLocalCounter(int64_t NewIndex)
		{
			static thread_local size_t Index = (size_t)-1;
			if (NewIndex != -2)
				Index = NewIndex;
			return Index;
		}
		size_t Schedule::GetThreadGlobalIndex()
		{
			return UpdateThreadGlobalCounter(-2);
		}
		size_t Schedule::GetThreadLocalIndex()
		{
			return UpdateThreadLocalCounter(-2);
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
			VI_ASSERT(Type != Difficulty::Count, false, "difficulty should be set");
			return Threads[(size_t)Type].size();
		}
		const Schedule::Desc& Schedule::GetPolicy() const
		{
			return Policy;
		}
		std::chrono::microseconds Schedule::GetTimeout(std::chrono::microseconds Clock)
		{
			auto* Queue = Queues[(size_t)Difficulty::Clock];
			while (Queue->Timers.find(Clock) != Queue->Timers.end())
				++Clock;
			return Clock;
		}
		std::chrono::microseconds Schedule::GetClock()
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
		}
		bool Schedule::IsPresentAndActive()
		{
			return Singleton != nullptr && Singleton->Active;
		}
		bool Schedule::Reset()
		{
			if (!Singleton)
				return false;

			VI_RELEASE(Singleton);
			return true;
		}
		Schedule* Schedule::Get()
		{
			if (Singleton == nullptr)
				Singleton = new Schedule();

			return Singleton;
		}
		Schedule* Schedule::Singleton = nullptr;

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
		Core::UnorderedMap<Core::String, size_t> Schema::GetNames() const
		{
			Core::UnorderedMap<Core::String, size_t> Mapping;
			size_t Index = 0;

			GenerateNamingTable(this, &Mapping, Index);
			return Mapping;
		}
		Core::Vector<Schema*> Schema::FindCollection(const Core::String& Name, bool Deep) const
		{
			Core::Vector<Schema*> Result;
			if (!Nodes)
				return Result;

			for (auto Value : *Nodes)
			{
				if (Value->Key == Name)
					Result.push_back(Value);

				if (!Deep)
					continue;

				Core::Vector<Schema*> New = Value->FindCollection(Name);
				for (auto& Subvalue : New)
					Result.push_back(Subvalue);
			}

			return Result;
		}
		Core::Vector<Schema*> Schema::FetchCollection(const Core::String& Notation, bool Deep) const
		{
			Core::Vector<Core::String> Names = Stringify(Notation).Split('.');
			if (Names.empty())
				return Core::Vector<Schema*>();

			if (Names.size() == 1)
				return FindCollection(*Names.begin());

			Schema* Current = Find(*Names.begin(), Deep);
			if (!Current)
				return Core::Vector<Schema*>();

			for (auto It = Names.begin() + 1; It != Names.end() - 1; ++It)
			{
				Current = Current->Find(*It, Deep);
				if (!Current)
					return Core::Vector<Schema*>();
			}

			return Current->FindCollection(*(Names.end() - 1), Deep);
		}
		Core::Vector<Schema*> Schema::GetAttributes() const
		{
			Core::Vector<Schema*> Attributes;
			if (!Nodes)
				return Attributes;

			for (auto It : *Nodes)
			{
				if (It->IsAttribute())
					Attributes.push_back(It);
			}

			return Attributes;
		}
		Core::Vector<Schema*>& Schema::GetChilds()
		{
			Allocate();
			return *Nodes;
		}
		Schema* Schema::Find(const Core::String& Name, bool Deep) const
		{
			if (!Nodes)
				return nullptr;

			Core::Stringify Number(&Name);
			if (Number.HasInteger())
			{
				size_t Index = (size_t)Number.ToUInt64();
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
		Schema* Schema::Fetch(const Core::String& Notation, bool Deep) const
		{
			Core::Vector<Core::String> Names = Stringify(Notation).Split('.');
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
		Schema* Schema::GetAttribute(const Core::String& Name) const
		{
			return Get(':' + Name);
		}
		Variant Schema::FetchVar(const Core::String& fKey, bool Deep) const
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
		Variant Schema::GetVar(const Core::String& fKey) const
		{
			Schema* Result = Get(fKey);
			if (!Result)
				return Var::Undefined();

			return Result->Value;
		}
		Variant Schema::GetAttributeVar(const Core::String& Key) const
		{
			return GetVar(':' + Key);
		}
		Schema* Schema::Get(size_t Index) const
		{
			VI_ASSERT(Nodes != nullptr, nullptr, "there must be at least one node");
			VI_ASSERT(Index < Nodes->size(), nullptr, "index outside of range");

			return (*Nodes)[Index];
		}
		Schema* Schema::Get(const Core::String& Name) const
		{
			VI_ASSERT(!Name.empty(), nullptr, "name should not be empty");
			if (!Nodes)
				return nullptr;

			for (auto Schema : *Nodes)
			{
				if (Schema->Key == Name)
					return Schema;
			}

			return nullptr;
		}
		Schema* Schema::Set(const Core::String& Name)
		{
			return Set(Name, Var::Object());
		}
		Schema* Schema::Set(const Core::String& Name, const Variant& Base)
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
		Schema* Schema::Set(const Core::String& Name, Variant&& Base)
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
		Schema* Schema::Set(const Core::String& Name, Schema* Base)
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
		Schema* Schema::SetAttribute(const Core::String& Name, const Variant& fValue)
		{
			return Set(':' + Name, fValue);
		}
		Schema* Schema::SetAttribute(const Core::String& Name, Variant&& fValue)
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
			VI_ASSERT(Nodes != nullptr, nullptr, "there must be at least one node");
			VI_ASSERT(Index < Nodes->size(), nullptr, "index outside of range");

			auto It = Nodes->begin() + Index;
			Schema* Base = *It;
			Base->Parent = nullptr;
			VI_RELEASE(Base);
			Nodes->erase(It);

			return this;
		}
		Schema* Schema::Pop(const Core::String& Name)
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
		bool Schema::Rename(const Core::String& Name, const Core::String& NewName)
		{
			VI_ASSERT(!Name.empty() && !NewName.empty(), false, "name and new name should not be empty");

			Schema* Result = Get(Name);
			if (!Result)
				return false;

			Result->Key = NewName;
			return true;
		}
		bool Schema::Has(const Core::String& Name) const
		{
			return Fetch(Name) != nullptr;
		}
		bool Schema::HasAttribute(const Core::String& Name) const
		{
			return Fetch(':' + Name) != nullptr;
		}
		bool Schema::IsEmpty() const
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
		Core::String Schema::GetName() const
		{
			return IsAttribute() ? Key.substr(1) : Key;
		}
		void Schema::Join(Schema* Other, bool AppendOnly)
		{
			VI_ASSERT_V(Other != nullptr && Value.IsObject(), "other should be object and not empty");
			auto FillArena = [](UnorderedMap<Core::String, Schema*>& Nodes, Schema* Base)
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
				UnorderedMap<Core::String, Schema*> Subnodes;
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
				Nodes = VI_NEW(Core::Vector<Schema*>);
		}
		void Schema::Allocate(const Core::Vector<Schema*>& Other)
		{
			if (!Nodes)
				Nodes = VI_NEW(Core::Vector<Schema*>, Other);
			else
				*Nodes = Other;
		}
		bool Schema::Transform(Schema* Value, const SchemaNameCallback& Callback)
		{
			VI_ASSERT(!!Callback, false, "callback should not be empty");
			if (!Value)
				return false;

			Value->Key = Callback(Value->Key);
			if (!Value->Nodes)
				return true;

			for (auto* Item : *Value->Nodes)
				Transform(Item, Callback);

			return true;
		}
		bool Schema::ConvertToXML(Schema* Base, const SchemaWriteCallback& Callback)
		{
			VI_ASSERT(Base != nullptr && Callback, false, "base should be set and callback should not be empty");
			Core::Vector<Schema*> Attributes = Base->GetAttributes();
			bool Scalable = (Base->Value.GetSize() > 0 || ((size_t)(Base->Nodes ? Base->Nodes->size() : 0) > (size_t)Attributes.size()));
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
				Core::String Key = (*It)->GetName();
				Core::String Value = (*It)->Value.Serialize();

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
			if (Base->Value.GetSize() > 0)
			{
				Core::String Text = Base->Value.Serialize();
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
				return true;

			if (Base->Nodes != nullptr && !Base->Nodes->empty())
				Callback(VarForm::Write_Tab, "", 0);

			Callback(VarForm::Dummy, "</", 2);
			Callback(VarForm::Dummy, Base->Key.c_str(), Base->Key.size());
			Callback(Base->Parent ? VarForm::Write_Line : VarForm::Dummy, ">", 1);

			return true;
		}
		bool Schema::ConvertToJSON(Schema* Base, const SchemaWriteCallback& Callback)
		{
			VI_ASSERT(Base != nullptr && Callback, false, "base should be set and callback should not be empty");
			if (!Base->Value.IsObject())
			{
				Core::String Value = Base->Value.Serialize();
				Core::Stringify Safe(&Value);
				Safe.Escape();

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

				return true;
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
					Core::String Value = (Next->Value.GetType() == VarType::Undefined ? "null" : Next->Value.Serialize());
					Core::Stringify Safe(&Value);
					Safe.Escape();

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
			return true;
		}
		bool Schema::ConvertToJSONB(Schema* Base, const SchemaWriteCallback& Callback)
		{
			VI_ASSERT(Base != nullptr && Callback, false, "base should be set and callback should not be empty");
			Core::UnorderedMap<Core::String, size_t> Mapping = Base->GetNames();
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
			return true;
		}
		Core::String Schema::ToXML(Schema* Value)
		{
			Core::String Result;
			ConvertToXML(Value, [&](VarForm Type, const char* Buffer, size_t Length)
			{
				Result.append(Buffer, Length);
			});
			return Result;
		}
		Core::String Schema::ToJSON(Schema* Value)
		{
			Core::String Result;
			ConvertToJSON(Value, [&](VarForm Type, const char* Buffer, size_t Length)
			{
				Result.append(Buffer, Length);
			});
			return Result;
		}
		Core::Vector<char> Schema::ToJSONB(Schema* Value)
		{
			Core::Vector<char> Result;
			ConvertToJSONB(Value, [&](VarForm Type, const char* Buffer, size_t Length)
			{
				for (size_t i = 0; i < Length; i++)
				    Result.push_back(Buffer[i]);
			});
			return Result;
		}
		Schema* Schema::ConvertFromXML(const char* Buffer, bool Assert)
		{
			VI_ASSERT(Buffer != nullptr, nullptr, "buffer should not be null");
			if (*Buffer == '\0')
				return nullptr;

			rapidxml::xml_document<>* Data = VI_NEW(rapidxml::xml_document<>);
			if (!Data)
				return nullptr;

			try
			{
				Data->parse<rapidxml::parse_trim_whitespace>((char*)Buffer);
			}
			catch (const std::runtime_error& Exception)
			{
				VI_DELETE(xml_document, Data);
				if (Assert)
					VI_ERR("[xml] %s", Exception.what());
				
				((void)Exception);
				return nullptr;
			}
			catch (const rapidxml::parse_error& Exception)
			{
				VI_DELETE(xml_document, Data);
				if (Assert)
					VI_ERR("[xml] %s", Exception.what());

				((void)Exception);
				return nullptr;
			}
			catch (const std::exception& Exception)
			{
				VI_DELETE(xml_document, Data);
				if (Assert)
					VI_ERR("[xml] %s", Exception.what());

				((void)Exception);
				return nullptr;
			}
			catch (...)
			{
				VI_DELETE(xml_document, Data);
				if (Assert)
					VI_ERR("[xml] parsing error");

				return nullptr;
			}

			rapidxml::xml_node<>* Base = Data->first_node();
			if (!Base)
			{
				Data->clear();
				VI_DELETE(xml_document, Data);

				return nullptr;
			}

			Schema* Result = Var::Set::Array();
			Result->Key = Base->name();

			if (!ProcessConvertionFromXML((void*)Base, Result))
				VI_CLEAR(Result);

			Data->clear();
			VI_DELETE(xml_document, Data);

			return Result;
		}
		Schema* Schema::ConvertFromJSON(const char* Buffer, size_t Size, bool Assert)
		{
			VI_ASSERT(Buffer != nullptr, nullptr, "buffer should not be null");
			if (!Size)
				return nullptr;

			rapidjson::Document Base;
			Base.Parse(Buffer, Size);

			Core::Schema* Result = nullptr;
			if (Base.HasParseError())
			{
				if (!Assert)
					return nullptr;

				int Offset = (int)Base.GetErrorOffset();
				switch (Base.GetParseError())
				{
					case rapidjson::kParseErrorDocumentEmpty:
						VI_ERR("[json:%i] the document is empty", Offset);
						break;
					case rapidjson::kParseErrorDocumentRootNotSingular:
						VI_ERR("[json:%i] the document root must not follow by other values", Offset);
						break;
					case rapidjson::kParseErrorValueInvalid:
						VI_ERR("[json:%i] invalid value", Offset);
						break;
					case rapidjson::kParseErrorObjectMissName:
						VI_ERR("[json:%i] missing a name for object member", Offset);
						break;
					case rapidjson::kParseErrorObjectMissColon:
						VI_ERR("[json:%i] missing a colon after a name of object member", Offset);
						break;
					case rapidjson::kParseErrorObjectMissCommaOrCurlyBracket:
						VI_ERR("[json:%i] missing a comma or '}' after an object member", Offset);
						break;
					case rapidjson::kParseErrorArrayMissCommaOrSquareBracket:
						VI_ERR("[json:%i] missing a comma or ']' after an array element", Offset);
						break;
					case rapidjson::kParseErrorStringUnicodeEscapeInvalidHex:
						VI_ERR("[json:%i] incorrect hex digit after \\u escape in string", Offset);
						break;
					case rapidjson::kParseErrorStringUnicodeSurrogateInvalid:
						VI_ERR("[json:%i] the surrogate pair in string is invalid", Offset);
						break;
					case rapidjson::kParseErrorStringEscapeInvalid:
						VI_ERR("[json:%i] invalid escape character in string", Offset);
						break;
					case rapidjson::kParseErrorStringMissQuotationMark:
						VI_ERR("[json:%i] missing a closing quotation mark in string", Offset);
						break;
					case rapidjson::kParseErrorStringInvalidEncoding:
						VI_ERR("[json:%i] invalid encoding in string", Offset);
						break;
					case rapidjson::kParseErrorNumberTooBig:
						VI_ERR("[json:%i] number too big to be stored in double", Offset);
						break;
					case rapidjson::kParseErrorNumberMissFraction:
						VI_ERR("[json:%i] miss fraction part in number", Offset);
						break;
					case rapidjson::kParseErrorNumberMissExponent:
						VI_ERR("[json:%i] miss exponent in number", Offset);
						break;
					case rapidjson::kParseErrorTermination:
						VI_ERR("[json:%i] parsing was terminated", Offset);
						break;
					case rapidjson::kParseErrorUnspecificSyntaxError:
						VI_ERR("[json:%i] unspecific syntax error", Offset);
						break;
					default:
						break;
				}

				return nullptr;
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
					if (!ProcessConvertionFromJSON((void*)&Base, Result))
						VI_CLEAR(Result);
					break;
				case rapidjson::kArrayType:
					Result = Var::Set::Array();
					if (!ProcessConvertionFromJSON((void*)&Base, Result))
						VI_CLEAR(Result);
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
		}
		Schema* Schema::ConvertFromJSONB(const SchemaReadCallback& Callback, bool Assert)
		{
			VI_ASSERT(Callback, nullptr, "callback should not be empty");
			uint64_t Version = 0;
			if (!Callback((char*)&Version, sizeof(uint64_t)))
			{
				if (Assert)
					VI_ERR("[jsonb] form cannot be defined");

				return nullptr;
			}

			Version = OS::CPU::ToEndianness<uint64_t>(OS::CPU::Endian::Little, Version);
			if (Version != JSONB_VERSION)
			{
				if (Assert)
					VI_ERR("[jsonb] version %" PRIu64 " is not valid", Version);

				return nullptr;
			}

			uint32_t Set = 0;
			if (!Callback((char*)&Set, sizeof(uint32_t)))
			{
				if (Assert)
					VI_ERR("[jsonb] name map is undefined");

				return nullptr;
			}

			Core::UnorderedMap<size_t, Core::String> Map;
			Set = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Set);

			for (uint32_t i = 0; i < Set; ++i)
			{
				uint32_t Index = 0;
				if (!Callback((char*)&Index, sizeof(uint32_t)))
				{
					if (Assert)
						VI_ERR("[jsonb] name index is undefined");

					return nullptr;
				}

				uint16_t Size = 0;
				if (!Callback((char*)&Size, sizeof(uint16_t)))
				{
					if (Assert)
						VI_ERR("[jsonb] name size is undefined");

					return nullptr;
				}

				Index = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Index);
				Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);

				if (Size <= 0)
					continue;

				Core::String Name;
				Name.resize((size_t)Size);
				if (!Callback((char*)Name.c_str(), sizeof(char) * Size))
				{
					if (Assert)
						VI_ERR("[jsonb] name data is undefined");

					return nullptr;
				}

				Map.insert({ Index, Name });
			}

			Schema* Current = Var::Set::Object();
			if (!ProcessConvertionFromJSONB(Current, &Map, Callback))
			{
				VI_RELEASE(Current);
				return nullptr;
			}

			return Current;
		}
		Schema* Schema::FromXML(const Core::String& Text, bool Assert)
		{
			return ConvertFromXML(Text.c_str(), Assert);
		}
		Schema* Schema::FromJSON(const Core::String& Text, bool Assert)
		{
			return ConvertFromJSON(Text.c_str(), Text.size(), Assert);
		}
		Schema* Schema::FromJSONB(const Core::Vector<char>& Binary, bool Assert)
		{
			size_t Offset = 0;
			return Core::Schema::ConvertFromJSONB([&Binary, &Offset](char* Buffer, size_t Length)
			{
				Offset += Length;
				if (Offset >= Binary.size())
					return false;

				memcpy((void*)Buffer, Binary.data() + Offset, Length);
				return true;
			}, Assert);
		}
		bool Schema::ProcessConvertionFromXML(void* Base, Schema* Current)
		{
			VI_ASSERT(Base != nullptr && Current != nullptr, false, "base and current should be set");

			auto Ref = (rapidxml::xml_node<>*)Base;
			for (rapidxml::xml_attribute<>* It = Ref->first_attribute(); It; It = It->next_attribute())
				Current->SetAttribute(It->name(), Var::Auto(It->value()));

			for (rapidxml::xml_node<>* It = Ref->first_node(); It; It = It->next_sibling())
			{
				Schema* Subresult = Current->Set(It->name(), Var::Set::Array());
				ProcessConvertionFromXML((void*)It, Subresult);

				if (It->value_size() > 0)
					Subresult->Value.Deserialize(Core::String(It->value(), It->value_size()));
			}

			return true;
		}
		bool Schema::ProcessConvertionFromJSON(void* Base, Schema* Current)
		{
			VI_ASSERT(Base != nullptr && Current != nullptr, false, "base and current should be set");

			auto Ref = (rapidjson::Value*)Base;
			if (!Ref->IsArray())
			{
				Core::String Name;
				Current->Reserve((size_t)Ref->MemberCount());

				VarType Type = Current->Value.Type;
				Current->Value.Type = VarType::Array;

				for (auto It = Ref->MemberBegin(); It != Ref->MemberEnd(); ++It)
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
				Core::String Value;
				Current->Reserve((size_t)Ref->Size());

				for (auto It = Ref->Begin(); It != Ref->End(); ++It)
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

			return true;
		}
		bool Schema::ProcessConvertionToJSONB(Schema* Current, Core::UnorderedMap<Core::String, size_t>* Map, const SchemaWriteCallback& Callback)
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
					uint32_t Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, (uint32_t)Current->Value.GetSize());
					Callback(VarForm::Dummy, (const char*)&Size, sizeof(uint32_t));
					Callback(VarForm::Dummy, Current->Value.GetString(), Size * sizeof(char));
					break;
				}
				case VarType::Decimal:
				{
					Core::String Number = ((Decimal*)Current->Value.Value.Pointer)->ToString();
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

			return true;
		}
		bool Schema::ProcessConvertionFromJSONB(Schema* Current, Core::UnorderedMap<size_t, Core::String>* Map, const SchemaReadCallback& Callback)
		{
			uint32_t Id = 0;
			if (!Callback((char*)&Id, sizeof(uint32_t)))
			{
				VI_ERR("[jsonb] key name index is undefined");
				return false;
			}

			auto It = Map->find((size_t)OS::CPU::ToEndianness(OS::CPU::Endian::Little, Id));
			if (It != Map->end())
				Current->Key = It->second;

			if (!Callback((char*)&Current->Value.Type, sizeof(VarType)))
			{
				VI_ERR("[jsonb] key type is undefined");
				return false;
			}

			switch (Current->Value.Type)
			{
				case VarType::Object:
				case VarType::Array:
				{
					uint32_t Count = 0;
					if (!Callback((char*)&Count, sizeof(uint32_t)))
					{
						VI_ERR("[jsonb] key value size is undefined");
						return false;
					}

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

						ProcessConvertionFromJSONB(Item, Map, Callback);
					}
					break;
				}
				case VarType::String:
				{
					uint32_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint32_t)))
					{
						VI_ERR("[jsonb] key value size is undefined");
						return false;
					}

					Core::String Buffer;
					Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);
					Buffer.resize((size_t)Size);

					if (!Callback((char*)Buffer.c_str(), (size_t)Size * sizeof(char)))
					{
						VI_ERR("[jsonb] key value data is undefined");
						return false;
					}

					Current->Value = Var::String(Buffer);
					break;
				}
				case VarType::Binary:
				{
					uint32_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint32_t)))
					{
						VI_ERR("[jsonb] key value size is undefined");
						return false;
					}

					Core::String Buffer;
					Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);
					Buffer.resize(Size);

					if (!Callback((char*)Buffer.c_str(), (size_t)Size * sizeof(char)))
					{
						VI_ERR("[jsonb] key value data is undefined");
						return false;
					}

					Current->Value = Var::Binary(Buffer);
					break;
				}
				case VarType::Integer:
				{
					int64_t Integer = 0;
					if (!Callback((char*)&Integer, sizeof(int64_t)))
					{
						VI_ERR("[jsonb] key value is undefined");
						return false;
					}

					Current->Value = Var::Integer(OS::CPU::ToEndianness(OS::CPU::Endian::Little, Integer));
					break;
				}
				case VarType::Number:
				{
					double Number = 0.0;
					if (!Callback((char*)&Number, sizeof(double)))
					{
						VI_ERR("[jsonb] key value is undefined");
						return false;
					}

					Current->Value = Var::Number(OS::CPU::ToEndianness(OS::CPU::Endian::Little, Number));
					break;
				}
				case VarType::Decimal:
				{
					uint16_t Size = 0;
					if (!Callback((char*)&Size, sizeof(uint16_t)))
					{
						VI_ERR("[jsonb] key value size is undefined");
						return false;
					}

					Core::String Buffer;
					Size = OS::CPU::ToEndianness(OS::CPU::Endian::Little, Size);
					Buffer.resize((size_t)Size);

					if (!Callback((char*)Buffer.c_str(), (size_t)Size * sizeof(char)))
					{
						VI_ERR("[jsonb] key value data is undefined");
						return false;
					}

					Current->Value = Var::Decimal(Buffer);
					break;
				}
				case VarType::Boolean:
				{
					bool Boolean = false;
					if (!Callback((char*)&Boolean, sizeof(bool)))
					{
						VI_ERR("[jsonb] key value is undefined");
						return false;
					}

					Current->Value = Var::Boolean(Boolean);
					break;
				}
				default:
					break;
			}

			return true;
		}
		bool Schema::GenerateNamingTable(const Schema* Current, Core::UnorderedMap<Core::String, size_t>* Map, size_t& Index)
		{
			auto M = Map->find(Current->Key);
			if (M == Map->end())
				Map->insert({ Current->Key, Index++ });

			if (!Current->Nodes)
				return true;

			for (auto Schema : *Current->Nodes)
				GenerateNamingTable(Schema, Map, Index);

			return true;
		}
	}
}
#pragma warning(pop)