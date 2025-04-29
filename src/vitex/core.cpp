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
#include <cstdarg>
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
#include <windows.h>
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
#define MAKEUQUAD(l, h) ((uint64_t)(((uint32_t)(l)) | ((uint64_t)((uint32_t)(h))) << 32))
#define RATE_DIFF (10000000)
#define EPOCH_DIFF (MAKEUQUAD(0xd53e8000, 0x019db1de))
#define SYS2UNIX_TIME(l, h) ((int64_t)((MAKEUQUAD((l), (h)) - EPOCH_DIFF) / RATE_DIFF))
#define LEAP_YEAR(x) (((x) % 4 == 0) && (((x) % 100) != 0 || ((x) % 400) == 0))
#ifdef VI_MICROSOFT
namespace
{
	BOOL WINAPI console_event_handler(DWORD event)
	{
		switch (event)
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
	bool local_time(time_t const* const a, struct tm* const b)
	{
		return localtime_s(b, a) == 0;
	}
}
#else
namespace
{
	void pack264(void* value, int* x, int* y)
	{
		uint64_t subvalue = (uint64_t)value;
		*x = (int)(uint32_t)((subvalue & 0xFFFFFFFF00000000LL) >> 32);
		*y = (int)(uint32_t)(subvalue & 0xFFFFFFFFLL);
	}
	void* unpack264(int x, int y)
	{
		uint64_t subvalue = ((uint64_t)(uint32_t)x) << 32 | (uint32_t)y;
		return (void*)subvalue;
	}
	bool local_time(time_t const* const a, struct tm* const b)
	{
		return localtime_r(a, b) != nullptr;
	}
	const char* get_color_id(vitex::core::std_color color, bool background)
	{
		switch (color)
		{
			case vitex::core::std_color::black:
				return background ? "40" : "30";
			case vitex::core::std_color::dark_blue:
				return background ? "44" : "34";
			case vitex::core::std_color::dark_green:
				return background ? "42" : "32";
			case vitex::core::std_color::dark_red:
				return background ? "41" : "31";
			case vitex::core::std_color::magenta:
				return background ? "45" : "35";
			case vitex::core::std_color::orange:
				return background ? "43" : "93";
			case vitex::core::std_color::light_gray:
				return background ? "47" : "97";
			case vitex::core::std_color::light_blue:
				return background ? "46" : "94";
			case vitex::core::std_color::gray:
				return background ? "100" : "90";
			case vitex::core::std_color::blue:
				return background ? "104" : "94";
			case vitex::core::std_color::green:
				return background ? "102" : "92";
			case vitex::core::std_color::cyan:
				return background ? "106" : "36";
			case vitex::core::std_color::red:
				return background ? "101" : "91";
			case vitex::core::std_color::pink:
				return background ? "105" : "95";
			case vitex::core::std_color::yellow:
				return background ? "103" : "33";
			case vitex::core::std_color::white:
				return background ? "107" : "37";
			case vitex::core::std_color::zero:
				return background ? "49" : "39";
			default:
				return background ? "40" : "107";
		}
	}
}
#endif
namespace
{
	bool global_time(const time_t* timep, struct tm* tm)
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
	bool is_path_exists(const std::string_view& path)
	{
		struct stat buffer;
		return stat(path.data(), &buffer) == 0;
	}
	void escape_text(char* text, size_t size)
	{
		size_t index = 0;
		while (size-- > 0)
		{
			char& v = text[index++];
			if (v == '\a' || v == '\b' || v == '\f' || v == '\v' || v == '\0')
				v = '\?';
		}
	}
#ifdef VI_APPLE
#define SYSCTL(fname, ...) std::size_t size{};if(fname(__VA_ARGS__,nullptr,&size,nullptr,0))return{};vitex::core::vector<char> result(size);if(fname(__VA_ARGS__,result.data(),&size,nullptr,0))return{};return result
	template <class t>
	static std::pair<bool, t> sys_decompose(const vitex::core::vector<char>& data)
	{
		std::pair<bool, t> out { true, { } };
		std::memcpy(&out.second, data.data(), sizeof(out.second));
		return out;
	}
	vitex::core::vector<char> sys_control(const char* name)
	{
		SYSCTL(::sysctlbyname, name);
	}
	vitex::core::vector<char> sys_control(int M1, int M2)
	{
		int name[2] { M1, M2 };
		SYSCTL(::sysctl, name, sizeof(name) / sizeof(*name));
	}
	std::pair<bool, uint64_t> sys_extract(const vitex::core::vector<char>& data)
	{
		switch (data.size())
		{
			case sizeof(uint16_t) :
				return sys_decompose<uint16_t>(data);
				case sizeof(uint32_t) :
					return sys_decompose<uint32_t>(data);
					case sizeof(uint64_t) :
						return sys_decompose<uint64_t>(data);
					default:
						return { };
		}
	}
#endif
#ifdef VI_MICROSOFT
	static vitex::core::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> cpu_info_buffer()
	{
		DWORD byte_count = 0;
		vitex::core::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer;
		GetLogicalProcessorInformation(nullptr, &byte_count);
		buffer.resize(byte_count / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
		GetLogicalProcessorInformation(buffer.data(), &byte_count);

		return buffer;
	}
#endif
}

namespace vitex
{
	namespace core
	{
		namespace allocators
		{
			debug_allocator::tracing_info::tracing_info() : thread(std::this_thread::get_id()), time(0), size(0), active(false)
			{
			}
			debug_allocator::tracing_info::tracing_info(const char* new_type_name, memory_location&& new_location, time_t new_time, size_t new_size, bool is_active, bool is_static) : thread(std::this_thread::get_id()), type_name(new_type_name ? new_type_name : "void"), location(std::move(new_location)), time(new_time), size(new_size), active(is_active), constant(is_static)
			{
			}

			void* debug_allocator::allocate(size_t size) noexcept
			{
				return allocate(memory_location("[unknown]", "[external]", "void", 0), size);
			}
			void* debug_allocator::allocate(memory_location&& location, size_t size) noexcept
			{
				void* address = malloc(size);
				VI_ASSERT(address != nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)size);

				umutex<std::recursive_mutex> unique(mutex);
				blocks[address] = tracing_info(location.type_name, std::move(location), time(nullptr), size, true, false);
				return address;
			}
			void debug_allocator::free(void* address) noexcept
			{
				umutex<std::recursive_mutex> unique(mutex);
				auto it = blocks.find(address);
				VI_ASSERT(it != blocks.end() && it->second.active, "cannot free memory that was not allocated by this allocator at 0x%" PRIXPTR, address);

				blocks.erase(it);
				::free(address);
			}
			void debug_allocator::transfer(void* address, size_t size) noexcept
			{
				VI_ASSERT(false, "invalid allocator transfer call without memory context");
			}
			void debug_allocator::transfer(void* address, memory_location&& location, size_t size) noexcept
			{
				umutex<std::recursive_mutex> unique(mutex);
				blocks[address] = tracing_info(location.type_name, std::move(location), time(nullptr), size, true, true);
			}
			void debug_allocator::watch(memory_location&& location, void* address) noexcept
			{
				umutex<std::recursive_mutex> unique(mutex);
				auto it = watchers.find(address);

				VI_ASSERT(it == watchers.end() || !it->second.active, "cannot watch memory that is already being tracked at 0x%" PRIXPTR, address);
				if (it != watchers.end())
					it->second = tracing_info(location.type_name, std::move(location), time(nullptr), sizeof(void*), false, false);
				else
					watchers[address] = tracing_info(location.type_name, std::move(location), time(nullptr), sizeof(void*), false, false);
			}
			void debug_allocator::unwatch(void* address) noexcept
			{
				umutex<std::recursive_mutex> unique(mutex);
				auto it = watchers.find(address);

				VI_ASSERT(it != watchers.end() && !it->second.active, "address at 0x%" PRIXPTR " cannot be cleared from tracking because it was not allocated by this allocator", address);
				watchers.erase(it);
			}
			void debug_allocator::finalize() noexcept
			{
				dump(nullptr);
			}
			bool debug_allocator::is_valid(void* address) noexcept
			{
				umutex<std::recursive_mutex> unique(mutex);
				auto it = blocks.find(address);

				VI_ASSERT(it != blocks.end(), "address at 0x%" PRIXPTR " cannot be used as it was already freed");
				return it != blocks.end();
			}
			bool debug_allocator::is_finalizable() noexcept
			{
				return true;
			}
			bool debug_allocator::dump(void* address)
			{
#if VI_DLEVEL >= 4
				VI_TRACE("[mem] dump internal memory state on 0x%" PRIXPTR, address);
				umutex<std::recursive_mutex> unique(mutex);
				if (address != nullptr)
				{
					bool log_active = error_handling::has_flag(log_option::active), exists = false;
					if (!log_active)
						error_handling::set_flag(log_option::active, true);

					auto it = blocks.find(address);
					if (it != blocks.end())
					{
						char date[64];
						date_time::serialize_local(date, sizeof(date), std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(it->second.time)), date_time::format_compact_time());
						error_handling::message(log_level::debug, it->second.location.line, it->second.location.source, "[mem] %saddress at 0x%" PRIXPTR " is used since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
							it->second.constant ? "static " : "",
							it->first, date, it->second.type_name.c_str(),
							(uint64_t)it->second.size,
							it->second.location.function,
							os::process::get_thread_id(it->second.thread).c_str());
						exists = true;
					}

					it = watchers.find(address);
					if (it != watchers.end())
					{
						char date[64];
						date_time::serialize_local(date, sizeof(date), std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(it->second.time)), date_time::format_compact_time());
						error_handling::message(log_level::debug, it->second.location.line, it->second.location.source, "[mem-watch] %saddress at 0x%" PRIXPTR " is being watched since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
							it->second.constant ? "static " : "",
							it->first, date, it->second.type_name.c_str(),
							(uint64_t)it->second.size,
							it->second.location.function,
							os::process::get_thread_id(it->second.thread).c_str());
						exists = true;
					}

					error_handling::set_flag(log_option::active, log_active);
					return exists;
				}
				else if (!blocks.empty() || !watchers.empty())
				{
					size_t static_addresses = 0;
					for (auto& item : blocks)
					{
						if (item.second.constant || item.second.type_name.find("ontainer_proxy") != std::string::npos || item.second.type_name.find("ist_node") != std::string::npos)
							++static_addresses;
					}
					for (auto& item : watchers)
					{
						if (item.second.constant || item.second.type_name.find("ontainer_proxy") != std::string::npos || item.second.type_name.find("ist_node") != std::string::npos)
							++static_addresses;
					}

					bool log_active = error_handling::has_flag(log_option::active);
					if (!log_active)
						error_handling::set_flag(log_option::active, true);

					if (static_addresses == blocks.size() + watchers.size())
					{
						VI_DEBUG("[mem] memory tracing OK: no memory leaked");
						error_handling::set_flag(log_option::active, log_active);
						return false;
					}

					size_t total_memory = 0;
					for (auto& item : blocks)
						total_memory += item.second.size;
					for (auto& item : watchers)
						total_memory += item.second.size;

					uint64_t count = (uint64_t)(blocks.size() + watchers.size() - static_addresses);
					VI_DEBUG("[mem] %" PRIu64 " address%s still used (memory still in use: %" PRIu64 " bytes inc. static allocations)", count, count > 1 ? "es are" : " is", (uint64_t)total_memory);
					for (auto& item : blocks)
					{
						if (item.second.constant || item.second.type_name.find("ontainer_proxy") != std::string::npos || item.second.type_name.find("ist_node") != std::string::npos)
							continue;

						char date[64];
						date_time::serialize_local(date, sizeof(date), std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(item.second.time)), date_time::format_compact_time());
						error_handling::message(log_level::debug, item.second.location.line, item.second.location.source, "[mem] address at 0x%" PRIXPTR " is used since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
							item.first,
							date,
							item.second.type_name.c_str(),
							(uint64_t)item.second.size,
							item.second.location.function,
							os::process::get_thread_id(item.second.thread).c_str());
					}
					for (auto& item : watchers)
					{
						if (item.second.constant || item.second.type_name.find("ontainer_proxy") != std::string::npos || item.second.type_name.find("ist_node") != std::string::npos)
							continue;

						char date[64];
						date_time::serialize_local(date, sizeof(date), std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(item.second.time)), date_time::format_compact_time());
						error_handling::message(log_level::debug, item.second.location.line, item.second.location.source, "[mem-watch] address at 0x%" PRIXPTR " is being watched since %s as %s (%" PRIu64 " bytes) at %s() on thread %s",
							item.first,
							date,
							item.second.type_name.c_str(),
							(uint64_t)item.second.size,
							item.second.location.function,
							os::process::get_thread_id(item.second.thread).c_str());
					}

					error_handling::set_flag(log_option::active, log_active);
					return true;
				}
#endif
				return false;
			}
			bool debug_allocator::find_block(void* address, tracing_info* output)
			{
				VI_ASSERT(address != nullptr, "address should not be null");
				umutex<std::recursive_mutex> unique(mutex);
				auto it = blocks.find(address);
				if (it == blocks.end())
				{
					it = watchers.find(address);
					if (it == watchers.end())
						return false;
				}

				if (output != nullptr)
					*output = it->second;

				return true;
			}
			const std::unordered_map<void*, debug_allocator::tracing_info>& debug_allocator::get_blocks() const
			{
				return blocks;
			}
			const std::unordered_map<void*, debug_allocator::tracing_info>& debug_allocator::get_watchers() const
			{
				return watchers;
			}

			void* default_allocator::allocate(size_t size) noexcept
			{
				void* address = malloc(size);
				VI_ASSERT(address != nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)size);
				return address;
			}
			void* default_allocator::allocate(memory_location&& location, size_t size) noexcept
			{
				void* address = malloc(size);
				VI_ASSERT(address != nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)size);
				return address;
			}
			void default_allocator::free(void* address) noexcept
			{
				::free(address);
			}
			void default_allocator::transfer(void* address, size_t size) noexcept
			{
			}
			void default_allocator::transfer(void* address, memory_location&& location, size_t size) noexcept
			{
			}
			void default_allocator::watch(memory_location&& location, void* address) noexcept
			{
			}
			void default_allocator::unwatch(void* address) noexcept
			{
			}
			void default_allocator::finalize() noexcept
			{
			}
			bool default_allocator::is_valid(void* address) noexcept
			{
				return true;
			}
			bool default_allocator::is_finalizable() noexcept
			{
				return true;
			}

			cached_allocator::cached_allocator(uint64_t minimal_life_time_ms, size_t max_elements_per_allocation, size_t elements_reducing_base_bytes, double elements_reducing_factor_rate) : minimal_life_time(minimal_life_time_ms), elements_reducing_factor(elements_reducing_factor_rate), elements_reducing_base(elements_reducing_base_bytes), elements_per_allocation(max_elements_per_allocation)
			{
				VI_ASSERT(elements_per_allocation > 0, "elements count per allocation should be greater then zero");
				VI_ASSERT(elements_reducing_factor > 1.0, "elements reducing factor should be greater then zero");
				VI_ASSERT(elements_reducing_base > 0, "elements reducing base should be greater then zero");
			}
			cached_allocator::~cached_allocator() noexcept
			{
				for (auto& page : pages)
				{
					for (auto* cache : page.second)
					{
						cache->~page_cache();
						free(cache);
					}
				}
				pages.clear();
			}
			void* cached_allocator::allocate(size_t size) noexcept
			{
				umutex<std::recursive_mutex> unique(mutex);
				auto* cache = get_page_cache(size);
				if (!cache)
					return nullptr;

				page_address* address = cache->addresses.back();
				cache->addresses.pop_back();
				return address->address;
			}
			void* cached_allocator::allocate(memory_location&&, size_t size) noexcept
			{
				return allocate(size);
			}
			void cached_allocator::free(void* address) noexcept
			{
				char* source_address = nullptr;
				page_address* source = (page_address*)((char*)address - sizeof(page_address));
				memcpy(&source_address, (char*)source + sizeof(void*), sizeof(void*));
				if (source_address != address)
					return free(address);

				page_cache* cache = nullptr;
				memcpy(&cache, source, sizeof(void*));

				umutex<std::recursive_mutex> unique(mutex);
				cache->addresses.push_back(source);

				if (cache->addresses.size() >= cache->capacity && (cache->capacity == 1 || get_clock() - cache->timing > (int64_t)minimal_life_time))
				{
					cache->page.erase(std::find(cache->page.begin(), cache->page.end(), cache));
					cache->~page_cache();
					free(cache);
				}
			}
			void cached_allocator::transfer(void* address, size_t size) noexcept
			{
			}
			void cached_allocator::transfer(void* address, memory_location&& location, size_t size) noexcept
			{
			}
			void cached_allocator::watch(memory_location&& location, void* address) noexcept
			{
			}
			void cached_allocator::unwatch(void* address) noexcept
			{
			}
			void cached_allocator::finalize() noexcept
			{
			}
			bool cached_allocator::is_valid(void* address) noexcept
			{
				return true;
			}
			bool cached_allocator::is_finalizable() noexcept
			{
				return false;
			}
			cached_allocator::page_cache* cached_allocator::get_page_cache(size_t size)
			{
				auto& page = pages[size];
				for (auto* cache : page)
				{
					if (!cache->addresses.empty())
						return cache;
				}

				size_t address_size = sizeof(page_address) + size;
				size_t page_elements = get_elements_count(page, size);
				size_t page_size = sizeof(page_cache) + address_size * page_elements;
				page_cache* cache = (page_cache*)malloc(page_size);
				VI_ASSERT(cache != nullptr, "not enough memory to malloc %" PRIu64 " bytes", (uint64_t)page_size);

				if (!cache)
					return nullptr;

				char* base_address = (char*)cache + sizeof(page_cache);
				new(cache) page_cache(page, get_clock(), page_elements);
				for (size_t i = 0; i < page_elements; i++)
				{
					page_address* next = (page_address*)(base_address + address_size * i);
					next->address = (void*)(base_address + address_size * i + sizeof(page_address));
					next->cache = cache;
					cache->addresses[i] = next;
				}

				page.push_back(cache);
				return cache;
			}
			int64_t cached_allocator::get_clock()
			{
				return (int64_t)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			}
			size_t cached_allocator::get_elements_count(page_group& page, size_t size)
			{
				double frequency;
				if (pages.size() > 1)
				{
					frequency = 0.0;
					for (auto& next : pages)
					{
						if (next.second != page)
							frequency += (double)next.second.size();
					}

					frequency /= (double)pages.size() - 1;
					if (frequency < 1.0)
						frequency = std::max(1.0, (double)page.size());
					else
						frequency = std::max(1.0, (double)page.size() / frequency);
				}
				else
					frequency = 1.0;

				double total = (double)elements_per_allocation;
				double reducing = (double)size / (double)elements_reducing_base;
				if (reducing > 1.0)
				{
					total /= elements_reducing_factor * reducing;
					if (total < 1.0)
						total = 1.0;
				}
				else if (frequency > 1.0)
					total *= frequency;

				return (size_t)total;
			}

			linear_allocator::linear_allocator(size_t size) : top(nullptr), bottom(nullptr), latest_size(0), sizing(size)
			{
				if (sizing > 0)
					next_region(sizing);
			}
			linear_allocator::~linear_allocator() noexcept
			{
				flush_regions();
			}
			void* linear_allocator::allocate(size_t size) noexcept
			{
				if (!bottom)
					next_region(size);
			retry:
				char* max_address = bottom->base_address + bottom->size;
				char* offset_address = bottom->free_address;
				size_t leftovers = max_address - offset_address;
				if (leftovers < size)
				{
					next_region(size);
					goto retry;
				}

				char* address = offset_address;
				bottom->free_address = address + size;
				latest_size = size;
				return address;
			}
			void linear_allocator::free(void* address) noexcept
			{
				VI_ASSERT(is_valid(address), "address is not valid");
				char* origin_address = bottom->free_address - latest_size;
				if (origin_address == address)
				{
					bottom->free_address = origin_address;
					latest_size = 0;
				}
			}
			void linear_allocator::reset() noexcept
			{
				size_t total_size = 0;
				region* next = top;
				while (next != nullptr)
				{
					total_size += next->size;
					next->free_address = next->base_address;
					next = next->lower_address;
				}

				if (total_size > sizing)
				{
					sizing = total_size;
					flush_regions();
					next_region(sizing);
				}
			}
			bool linear_allocator::is_valid(void* address) noexcept
			{
				char* target = (char*)address;
				region* next = top;
				while (next != nullptr)
				{
					if (target >= next->base_address && target <= next->base_address + next->size)
						return true;

					next = next->lower_address;
				}

				return false;
			}
			void linear_allocator::next_region(size_t size) noexcept
			{
				local_allocator* current = memory::get_local_allocator();
				memory::set_local_allocator(nullptr);

				region* next = memory::allocate<region>(sizeof(region) + size);
				next->base_address = (char*)next + sizeof(region);
				next->free_address = next->base_address;
				next->upper_address = bottom;
				next->lower_address = nullptr;
				next->size = size;

				if (!top)
					top = next;
				if (bottom != nullptr)
					bottom->lower_address = next;

				bottom = next;
				memory::set_local_allocator(current);
			}
			void linear_allocator::flush_regions() noexcept
			{
				local_allocator* current = memory::get_local_allocator();
				memory::set_local_allocator(nullptr);

				region* next = bottom;
				bottom = nullptr;
				top = nullptr;

				while (next != nullptr)
				{
					void* address = (void*)next;
					next = next->upper_address;
					memory::deallocate(address);
				}

				memory::set_local_allocator(current);
			}
			size_t linear_allocator::get_leftovers() const noexcept
			{
				if (!bottom)
					return 0;

				char* max_address = bottom->base_address + bottom->size;
				char* offset_address = bottom->free_address;
				return max_address - offset_address;
			}

			stack_allocator::stack_allocator(size_t size) : top(nullptr), bottom(nullptr), sizing(size)
			{
				if (sizing > 0)
					next_region(sizing);
			}
			stack_allocator::~stack_allocator() noexcept
			{
				flush_regions();
			}
			void* stack_allocator::allocate(size_t size) noexcept
			{
				size = size + sizeof(size_t);
				if (!bottom)
					next_region(size);
			retry:
				char* max_address = bottom->base_address + bottom->size;
				char* offset_address = bottom->free_address;
				size_t leftovers = max_address - offset_address;
				if (leftovers < size)
				{
					next_region(size);
					goto retry;
				}

				char* address = offset_address;
				bottom->free_address = address + size;
				*(size_t*)address = size;
				return address + sizeof(size_t);
			}
			void stack_allocator::free(void* address) noexcept
			{
				VI_ASSERT(is_valid(address), "address is not valid");
				char* offset_address = (char*)address - sizeof(size_t);
				char* origin_address = bottom->free_address - *(size_t*)offset_address;
				if (origin_address == offset_address)
					bottom->free_address = origin_address;
			}
			void stack_allocator::reset() noexcept
			{
				size_t total_size = 0;
				region* next = top;
				while (next != nullptr)
				{
					total_size += next->size;
					next->free_address = next->base_address;
					next = next->lower_address;
				}

				if (total_size > sizing)
				{
					sizing = total_size;
					flush_regions();
					next_region(sizing);
				}
			}
			bool stack_allocator::is_valid(void* address) noexcept
			{
				char* target = (char*)address;
				region* next = top;
				while (next != nullptr)
				{
					if (target >= next->base_address && target <= next->base_address + next->size)
						return true;

					next = next->lower_address;
				}

				return false;
			}
			void stack_allocator::next_region(size_t size) noexcept
			{
				local_allocator* current = memory::get_local_allocator();
				memory::set_local_allocator(nullptr);

				region* next = memory::allocate<region>(sizeof(region) + size);
				next->base_address = (char*)next + sizeof(region);
				next->free_address = next->base_address;
				next->upper_address = bottom;
				next->lower_address = nullptr;
				next->size = size;

				if (!top)
					top = next;
				if (bottom != nullptr)
					bottom->lower_address = next;

				bottom = next;
				memory::set_local_allocator(current);
			}
			void stack_allocator::flush_regions() noexcept
			{
				local_allocator* current = memory::get_local_allocator();
				memory::set_local_allocator(nullptr);

				region* next = bottom;
				bottom = nullptr;
				top = nullptr;

				while (next != nullptr)
				{
					void* address = (void*)next;
					next = next->upper_address;
					memory::deallocate(address);
				}

				memory::set_local_allocator(current);
			}
			size_t stack_allocator::get_leftovers() const noexcept
			{
				if (!bottom)
					return 0;

				char* max_address = bottom->base_address + bottom->size;
				char* offset_address = bottom->free_address;
				return max_address - offset_address;
			}
		}

		typedef moodycamel::BlockingConcurrentQueue<task_callback> fast_queue;
		typedef moodycamel::ConsumerToken receive_token;

		struct measurement
		{
#ifndef NDEBUG
			const char* file = nullptr;
			const char* function = nullptr;
			void* id = nullptr;
			uint64_t threshold = 0;
			uint64_t time = 0;
			int line = 0;

			void notify_of_over_consumption()
			{
				if (threshold == (uint64_t)timings::infinite)
					return;

				uint64_t delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - time;
				if (delta <= threshold)
					return;

				string back_trace = error_handling::get_measure_trace();
				VI_WARN("[stall] sync operation took %" PRIu64 " ms (%" PRIu64 " us), back trace %s", delta / 1000, delta, back_trace.c_str());
			}
#endif
		};

		struct cocontext
		{
#ifdef VI_FCONTEXT
			fcontext_t context = nullptr;
			char* stack = nullptr;
#elif VI_MICROSOFT
			LPVOID context = nullptr;
			bool main = false;
#else
			ucontext_t context = nullptr;
			char* stack = nullptr;
#endif
			cocontext()
			{
#ifndef VI_FCONTEXT
#ifdef VI_MICROSOFT
				context = ConvertThreadToFiber(nullptr);
				main = true;
#endif
#endif
			}
			cocontext(costate* state)
			{
#ifdef VI_FCONTEXT
				stack = memory::allocate<char>(sizeof(char) * state->size);
				context = make_fcontext(stack + state->size, state->size, [](transfer_t transfer)
				{
					costate::execution_entry(&transfer);
				});
#elif VI_MICROSOFT
				context = CreateFiber(state->size, &costate::execution_entry, (LPVOID)state);
#else
				getcontext(&context);
				stack = memory::allocate<char>(sizeof(char) * state->size);
				context.uc_stack.ss_sp = stack;
				context.uc_stack.ss_size = state->size;
				context.uc_stack.ss_flags = 0;
				context.uc_link = &state->master->context;

				int x, y;
				pack264((void*)state, &x, &y);
				makecontext(&context, (void(*)()) & costate::execution_entry, 2, x, y);
#endif
			}
			~cocontext()
			{
#ifdef VI_FCONTEXT
				memory::deallocate(stack);
#elif VI_MICROSOFT
				if (main)
					ConvertFiberToThread();
				else if (context != nullptr)
					DeleteFiber(context);
#else
				memory::deallocate(stack);
#endif
			}
		};

		struct concurrent_sync_queue
		{
			fast_queue queue;
		};

		struct concurrent_async_queue : concurrent_sync_queue
		{
			std::condition_variable notify;
			std::mutex update;
			std::atomic<bool> resync = true;
		};

		basic_exception::basic_exception(const std::string_view& new_message) noexcept : error_message(new_message)
		{
		}
		basic_exception::basic_exception(string&& new_message) noexcept : error_message(std::move(new_message))
		{
		}
		const char* basic_exception::what() const noexcept
		{
			return error_message.c_str();
		}
		const string& basic_exception::message() const& noexcept
		{
			return error_message;
		}
		const string&& basic_exception::message() const&& noexcept
		{
			return std::move(error_message);
		}
		string& basic_exception::message() & noexcept
		{
			return error_message;
		}
		string&& basic_exception::message() && noexcept
		{
			return std::move(error_message);
		}

		parser_exception::parser_exception(parser_error new_type) : parser_exception(new_type, -1, core::string())
		{
		}
		parser_exception::parser_exception(parser_error new_type, size_t new_offset) : parser_exception(new_type, new_offset, core::string())
		{
		}
		parser_exception::parser_exception(parser_error new_type, size_t new_offset, const std::string_view& new_message) : basic_exception(), error_type(new_type), error_offset(new_offset)
		{
			if (new_message.empty())
			{
				switch (error_type)
				{
					case parser_error::not_supported:
						error_message = "required libraries are not loaded";
						break;
					case parser_error::bad_version:
						error_message = "corrupted JSONB version header";
						break;
					case parser_error::bad_dictionary:
						error_message = "corrupted JSONB dictionary body";
						break;
					case parser_error::bad_name_index:
						error_message = "invalid JSONB dictionary name index";
						break;
					case parser_error::bad_name:
						error_message = "invalid JSONB name";
						break;
					case parser_error::bad_key_name:
						error_message = "invalid JSONB key name";
						break;
					case parser_error::bad_key_type:
						error_message = "invalid JSONB key type";
						break;
					case parser_error::bad_value:
						error_message = "invalid JSONB value for specified key";
						break;
					case parser_error::bad_string:
						error_message = "invalid JSONB value string";
						break;
					case parser_error::bad_integer:
						error_message = "invalid JSONB value integer";
						break;
					case parser_error::bad_double:
						error_message = "invalid JSONB value double";
						break;
					case parser_error::bad_boolean:
						error_message = "invalid JSONB value boolean";
						break;
					case parser_error::xml_out_of_memory:
						error_message = "XML out of memory";
						break;
					case parser_error::xml_internal_error:
						error_message = "XML internal error";
						break;
					case parser_error::xml_unrecognized_tag:
						error_message = "XML unrecognized tag";
						break;
					case parser_error::xml_bad_pi:
						error_message = "XML bad pi";
						break;
					case parser_error::xml_bad_comment:
						error_message = "XML bad comment";
						break;
					case parser_error::xml_bad_cdata:
						error_message = "XML bad cdata";
						break;
					case parser_error::xml_bad_doc_type:
						error_message = "XML bad doctype";
						break;
					case parser_error::xml_bad_pc_data:
						error_message = "XML bad pcdata";
						break;
					case parser_error::xml_bad_start_element:
						error_message = "XML bad start element";
						break;
					case parser_error::xml_bad_attribute:
						error_message = "XML bad attribute";
						break;
					case parser_error::xml_bad_end_element:
						error_message = "XML bad end element";
						break;
					case parser_error::xml_end_element_mismatch:
						error_message = "XML end element mismatch";
						break;
					case parser_error::xml_append_invalid_root:
						error_message = "XML append invalid root";
						break;
					case parser_error::xml_no_document_element:
						error_message = "XML no document element";
						break;
					case parser_error::json_document_empty:
						error_message = "the JSON document is empty";
						break;
					case parser_error::json_document_root_not_singular:
						error_message = "the JSON document root must not follow by other values";
						break;
					case parser_error::json_value_invalid:
						error_message = "the JSON document contains an invalid value";
						break;
					case parser_error::json_object_miss_name:
						error_message = "missing a name for a JSON object member";
						break;
					case parser_error::json_object_miss_colon:
						error_message = "missing a colon after a name of a JSON object member";
						break;
					case parser_error::json_object_miss_comma_or_curly_bracket:
						error_message = "missing a comma or '}' after a JSON object member";
						break;
					case parser_error::json_array_miss_comma_or_square_bracket:
						error_message = "missing a comma or ']' after a JSON array element";
						break;
					case parser_error::json_string_unicode_escape_invalid_hex:
						error_message = "incorrect hex digit after \\u escape in a JSON string";
						break;
					case parser_error::json_string_unicode_surrogate_invalid:
						error_message = "the surrogate pair in a JSON string is invalid";
						break;
					case parser_error::json_string_escape_invalid:
						error_message = "invalid escape character in a JSON string";
						break;
					case parser_error::json_string_miss_quotation_mark:
						error_message = "missing a closing quotation mark in a JSON string";
						break;
					case parser_error::json_string_invalid_encoding:
						error_message = "invalid encoding in a JSON string";
						break;
					case parser_error::json_number_too_big:
						error_message = "JSON number too big to be stored in double";
						break;
					case parser_error::json_number_miss_fraction:
						error_message = "missing fraction part in a JSON number";
						break;
					case parser_error::json_number_miss_exponent:
						error_message = "missing exponent in a JSON number";
						break;
					case parser_error::json_termination:
						error_message = "unexpected end of file while parsing a JSON document";
						break;
					case parser_error::json_unspecific_syntax_error:
						error_message = "unspecified JSON syntax error";
						break;
					default:
						error_message = "(unrecognized condition)";
						break;
				}
			}
			else
				error_message = new_message;

			if (error_offset > 0)
			{
				error_message += " at offset ";
				error_message += to_string(error_offset);
			}
		}
		const char* parser_exception::type() const noexcept
		{
			return "parser_error";
		}
		parser_error parser_exception::status() const noexcept
		{
			return error_type;
		}
		size_t parser_exception::offset() const noexcept
		{
			return error_offset;
		}

		system_exception::system_exception() : system_exception(string())
		{
		}
		system_exception::system_exception(const std::string_view& new_message) : error_condition(os::error::get_condition_or(std::errc::operation_not_permitted))
		{
			if (!new_message.empty())
			{
				error_message += new_message;
				error_message += " (error = ";
				error_message += error_condition.message().c_str();
				error_message += ")";
			}
			else
				error_message = copy<string>(error_condition.message());
		}
		system_exception::system_exception(const std::string_view& new_message, std::error_condition&& condition) : error_condition(std::move(condition))
		{
			if (!new_message.empty())
			{
				error_message += new_message;
				error_message += " (error = ";
				error_message += error_condition.message().c_str();
				error_message += ")";
			}
			else
				error_message = copy<string>(error_condition.message());
		}
		const char* system_exception::type() const noexcept
		{
			return "system_error";
		}
		const std::error_condition& system_exception::error() const& noexcept
		{
			return error_condition;
		}
		const std::error_condition&& system_exception::error() const&& noexcept
		{
			return std::move(error_condition);
		}
		std::error_condition& system_exception::error() & noexcept
		{
			return error_condition;
		}
		std::error_condition&& system_exception::error() && noexcept
		{
			return std::move(error_condition);
		}

		memory_location::memory_location() : source("?.cpp"), function("?"), type_name("void"), line(0)
		{
		}
		memory_location::memory_location(const char* new_source, const char* new_function, const char* new_type_name, int new_line) : source(new_source ? new_source : "?.cpp"), function(new_function ? new_function : "?"), type_name(new_type_name ? new_type_name : "void"), line(new_line >= 0 ? new_line : 0)
		{
		}

		static thread_local local_allocator* internal_allocator = nullptr;
		void* memory::default_allocate(size_t size) noexcept
		{
			VI_ASSERT(size > 0, "cannot allocate zero bytes");
			if (internal_allocator != nullptr)
			{
				void* address = internal_allocator->allocate(size);
				VI_PANIC(address != nullptr, "application is out of local memory allocating %" PRIu64 " bytes", (uint64_t)size);
				return address;
			}
			else if (global != nullptr)
			{
				void* address = global->allocate(size);
				VI_PANIC(address != nullptr, "application is out of global memory allocating %" PRIu64 " bytes", (uint64_t)size);
				return address;
			}
			else if (!context)
				context = new state();

			void* address = malloc(size);
			VI_PANIC(address != nullptr, "application is out of system memory allocating %" PRIu64 " bytes", (uint64_t)size);
			umutex<std::mutex> unique(context->mutex);
			context->allocations[address].second = size;
			return address;
		}
		void* memory::tracing_allocate(size_t size, memory_location&& origin) noexcept
		{
			VI_ASSERT(size > 0, "cannot allocate zero bytes");
			if (internal_allocator != nullptr)
			{
				void* address = internal_allocator->allocate(size);
				VI_PANIC(address != nullptr, "application is out of global memory allocating %" PRIu64 " bytes", (uint64_t)size);
				return address;
			}
			else if (global != nullptr)
			{
				void* address = global->allocate(std::move(origin), size);
				VI_PANIC(address != nullptr, "application is out of global memory allocating %" PRIu64 " bytes", (uint64_t)size);
				return address;
			}
			else if (!context)
				context = new state();

			void* address = malloc(size);
			VI_PANIC(address != nullptr, "application is out of system memory allocating %" PRIu64 " bytes", (uint64_t)size);
			umutex<std::mutex> unique(context->mutex);
			auto& item = context->allocations[address];
			item.first = std::move(origin);
			item.second = size;
			return address;
		}
		void memory::default_deallocate(void* address) noexcept
		{
			if (!address)
				return;

			if (internal_allocator != nullptr)
				return internal_allocator->free(address);
			else if (global != nullptr)
				return global->free(address);
			else if (!context)
				context = new state();

			umutex<std::mutex> unique(context->mutex);
			context->allocations.erase(address);
			free(address);
		}
		void memory::watch(void* address, memory_location&& origin) noexcept
		{
			VI_ASSERT(global != nullptr, "allocator should be set");
			VI_ASSERT(address != nullptr, "address should be set");
			global->watch(std::move(origin), address);
		}
		void memory::unwatch(void* address) noexcept
		{
			VI_ASSERT(global != nullptr, "allocator should be set");
			VI_ASSERT(address != nullptr, "address should be set");
			global->unwatch(address);
		}
		void memory::cleanup() noexcept
		{
			set_global_allocator(nullptr);
		}
		void memory::set_global_allocator(global_allocator* new_allocator) noexcept
		{
			if (global != nullptr)
				global->finalize();

			global = new_allocator;
			if (global != nullptr && context != nullptr)
			{
				for (auto& item : context->allocations)
				{
#ifndef NDEBUG
					global->transfer(item.first, memory_location(item.second.first), item.second.second);
#else
					global->transfer(item.first, item.second.second);
#endif
				}
			}

			delete context;
			context = nullptr;
		}
		void memory::set_local_allocator(local_allocator* new_allocator) noexcept
		{
			internal_allocator = new_allocator;
		}
		bool memory::is_valid_address(void* address) noexcept
		{
			VI_ASSERT(global != nullptr, "allocator should be set");
			VI_ASSERT(address != nullptr, "address should be set");
			if (internal_allocator != nullptr && internal_allocator->is_valid(address))
				return true;

			return global->is_valid(address);
		}
		global_allocator* memory::get_global_allocator() noexcept
		{
			return global;
		}
		local_allocator* memory::get_local_allocator() noexcept
		{
			return internal_allocator;
		}
		global_allocator* memory::global = nullptr;
		memory::state* memory::context = nullptr;

		stack_trace::stack_trace(size_t skips, size_t max_depth)
		{
			scripting::immediate_context* context = scripting::immediate_context::get();
			if (context != nullptr)
			{
				scripting::virtual_machine* VM = context->get_vm();
				size_t callstack_size = context->get_callstack_size();
				frames.reserve(callstack_size);
				for (size_t i = 0; i < callstack_size; i++)
				{
					int column_number = 0;
					int line_number = context->get_line_number(i, &column_number);
					scripting::function next = context->get_function(i);
					auto section_name = next.get_section_name();
					frame target;
					if (!section_name.empty())
					{
						auto source_code = VM->get_source_code_appendix_by_path("source", section_name, (uint32_t)line_number, (uint32_t)column_number, 5);
						if (source_code)
							target.code = *source_code;
						target.file = section_name;
					}
					else
						target.file = "[native]";
					target.function = (next.get_decl().empty() ? "[optimized]" : next.get_decl());
					target.line = (uint32_t)line_number;
					target.column = (uint32_t)column_number;
					target.handle = (void*)next.get_function();
					target.native = false;
					frames.emplace_back(std::move(target));
				}
			}
#ifdef VI_CXX23
			using stack_trace_container = std::basic_stacktrace<standard_allocator<std::stacktrace_entry>>;
			stack_trace_container stack = stack_trace_container::current(skips + 2, max_depth + skips + 2);
			frames.reserve((size_t)stack.size());

			for (auto& next : stack)
			{
				frame target;
				target.file = copy<string>(next.source_file());
				target.function = copy<string>(next.description());
				target.line = (uint32_t)next.source_line();
				target.column = 0;
				target.handle = (void*)next.native_handle();
				target.native = true;
				if (target.file.empty())
					target.file = "[external]";
				if (target.function.empty())
					target.function = "[optimized]";
				frames.emplace_back(std::move(target));
			}
#elif defined(VI_BACKWARDCPP)
			static bool is_preloaded = false;
			if (!is_preloaded)
			{
				backward::StackTrace empty_stack;
				empty_stack.load_here();
				backward::TraceResolver empty_resolver;
				empty_resolver.load_stacktrace(empty_stack);
				is_preloaded = true;
			}

			backward::StackTrace stack;
			stack.load_here(max_depth + skips + 3);
			stack.skip_n_firsts(skips + 3);
			frames.reserve(stack.size());

			backward::TraceResolver resolver;
			resolver.load_stacktrace(stack);

			size_t size = stack.size();
			for (size_t i = 0; i < size; i++)
			{
				backward::ResolvedTrace next = resolver.resolve(stack[i]);
				frame target;
				target.file = next.source.filename.empty() ? (next.object_filename.empty() ? "[external]" : next.object_filename.c_str()) : next.source.filename.c_str();
				target.function = next.source.function.empty() ? "[optimized]" : next.source.function.c_str();
				target.line = (uint32_t)next.source.line;
				target.column = 0;
				target.handle = next.addr;
				target.native = true;
				if (!next.source.function.empty() && target.function.back() != ')')
					target.function += "()";
				frames.emplace_back(std::move(target));
			}
#endif
		}
		stack_trace::stack_ptr::const_iterator stack_trace::begin() const
		{
			return frames.begin();
		}
		stack_trace::stack_ptr::const_iterator stack_trace::end() const
		{
			return frames.end();
		}
		stack_trace::stack_ptr::const_reverse_iterator stack_trace::rbegin() const
		{
			return frames.rbegin();
		}
		stack_trace::stack_ptr::const_reverse_iterator stack_trace::rend() const
		{
			return frames.rend();
		}
		stack_trace::operator bool() const
		{
			return !frames.empty();
		}
		const stack_trace::stack_ptr& stack_trace::range() const
		{
			return frames;
		}
		bool stack_trace::empty() const
		{
			return frames.empty();
		}
		size_t stack_trace::size() const
		{
			return frames.size();
		}
#ifndef NDEBUG
		static thread_local std::stack<measurement> internal_stacktrace;
#endif
		static thread_local bool internal_logging = false;
		error_handling::tick::tick(bool active) noexcept : is_counting(active)
		{
		}
		error_handling::tick::tick(tick&& other) noexcept : is_counting(other.is_counting)
		{
			other.is_counting = false;
		}
		error_handling::tick::~tick() noexcept
		{
#ifndef NDEBUG
			if (internal_logging || !is_counting)
				return;

			VI_ASSERT(!internal_stacktrace.empty(), "debug frame should be set");
			auto& next = internal_stacktrace.top();
			next.notify_of_over_consumption();
			internal_stacktrace.pop();
#endif
		}
		error_handling::tick& error_handling::tick::operator =(tick&& other) noexcept
		{
			if (&other == this)
				return *this;

			is_counting = other.is_counting;
			other.is_counting = false;
			return *this;
		}

		void error_handling::panic(int line, const char* source, const char* function, const char* condition, const char* format, ...) noexcept
		{
			details data;
			data.origin.file = os::path::get_filename(source).data();
			data.origin.line = line;
			data.type.level = log_level::error;
			data.type.fatal = true;
			if (has_flag(log_option::report_sys_errors) || true)
			{
				int error_code = os::error::get();
				if (!os::error::is_error(error_code))
					goto default_format;

				data.message.size = snprintf(data.message.data, sizeof(data.message.data), "thread %s PANIC! %s(): %s on \"!(%s)\", latest system error [%s]\n%s",
					os::process::get_thread_id(std::this_thread::get_id()).c_str(),
					function ? function : "?",
					format ? format : "check failed",
					condition ? condition : "?",
					os::error::get_name(error_code).c_str(),
					error_handling::get_stack_trace(1).c_str());
			}
			else
			{
			default_format:
				data.message.size = snprintf(data.message.data, sizeof(data.message.data), "thread %s PANIC! %s(): %s on \"!(%s)\"\n%s",
					os::process::get_thread_id(std::this_thread::get_id()).c_str(),
					function ? function : "?",
					format ? format : "check failed",
					condition ? condition : "?",
					error_handling::get_stack_trace(1).c_str());
			}
			if (has_flag(log_option::dated))
				date_time::serialize_local(data.message.date, sizeof(data.message.date), date_time::now(), date_time::format_compact_time());

			if (format != nullptr)
			{
				va_list args;
				va_start(args, format);
				char buffer[BLOB_SIZE] = { '\0' };
				data.message.size = vsnprintf(buffer, sizeof(buffer), data.message.data, args);
				if (data.message.size > sizeof(data.message.data))
					data.message.size = sizeof(data.message.data);
				memcpy(data.message.data, buffer, sizeof(buffer));
				va_end(args);
			}

			if (data.message.size > 0)
			{
				auto* terminal = console::get();
				terminal->show();
				escape_text(data.message.data, (size_t)data.message.size);
				dispatch(data);
				terminal->flush_write();
			}

			error_handling::pause();
			os::process::abort();
		}
		void error_handling::assertion(int line, const char* source, const char* function, const char* condition, const char* format, ...) noexcept
		{
			details data;
			data.origin.file = os::path::get_filename(source).data();
			data.origin.line = line;
			data.type.level = log_level::error;
			data.type.fatal = true;
			data.message.size = snprintf(data.message.data, sizeof(data.message.data), "thread %s ASSERT %s(): %s on \"!(%s)\"\n%s",
				os::process::get_thread_id(std::this_thread::get_id()).c_str(),
				function ? function : "?",
				format ? format : "assertion failed",
				condition ? condition : "?",
				error_handling::get_stack_trace(1).c_str());
			if (has_flag(log_option::dated))
				date_time::serialize_local(data.message.date, sizeof(data.message.date), date_time::now(), date_time::format_compact_time());

			if (format != nullptr)
			{
				va_list args;
				va_start(args, format);
				char buffer[BLOB_SIZE] = { '\0' };
				data.message.size = vsnprintf(buffer, sizeof(buffer), data.message.data, args);
				if (data.message.size > sizeof(data.message.data))
					data.message.size = sizeof(data.message.data);
				memcpy(data.message.data, buffer, sizeof(buffer));
				va_end(args);
			}

			if (data.message.size > 0)
			{
				auto* terminal = console::get();
				terminal->show();
				escape_text(data.message.data, (size_t)data.message.size);
				dispatch(data);
				terminal->flush_write();
			}

			error_handling::pause();
			os::process::abort();
		}
		void error_handling::message(log_level level, int line, const char* source, const char* format, ...) noexcept
		{
			VI_ASSERT(format != nullptr, "format string should be set");
			if (internal_logging || (!has_flag(log_option::active) && !has_callback()))
				return;

			details data;
			data.origin.file = os::path::get_filename(source).data();
			data.origin.line = line;
			data.type.level = level;
			data.type.fatal = false;
			if (has_flag(log_option::dated))
				date_time::serialize_local(data.message.date, sizeof(data.message.date), date_time::now(), date_time::format_compact_time());

			char buffer[512] = { '\0' };
			if (level == log_level::error && has_flag(log_option::report_sys_errors))
			{
				int error_code = os::error::get();
				if (!os::error::is_error(error_code))
					goto default_format;

				snprintf(buffer, sizeof(buffer), "%s, latest system error [%s]", format, os::error::get_name(error_code).c_str());
			}
			else
			{
			default_format:
				memcpy(buffer, format, std::min(sizeof(buffer), strlen(format)));
			}

			va_list args;
			va_start(args, format);
			data.message.size = vsnprintf(data.message.data, sizeof(data.message.data), buffer, args);
			if (data.message.size > sizeof(data.message.data))
				data.message.size = sizeof(data.message.data);
			va_end(args);

			if (data.message.size > 0)
			{
				escape_text(data.message.data, (size_t)data.message.size);
				enqueue(std::move(data));
			}
		}
		void error_handling::enqueue(details&& data) noexcept
		{
			if (has_flag(log_option::async))
				codefer([data = std::move(data)]() mutable { dispatch(data); });
			else
				dispatch(data);
		}
		void error_handling::dispatch(details& data) noexcept
		{
			internal_logging = true;
			if (has_callback())
				context->callback(data);
#if defined(VI_MICROSOFT) && !defined(NDEBUG)
			OutputDebugStringA(get_message_text(data).c_str());
#endif
			if (console::is_available())
			{
				auto* terminal = console::get();
				if (has_flag(log_option::pretty))
					terminal->synced([&data](console* terminal) { colorify(terminal, data); });
				else
					terminal->write(get_message_text(data));
			}
			internal_logging = false;
		}
		void error_handling::colorify(console* terminal, details& data) noexcept
		{
#if VI_DLEVEL < 5
			bool parse_tokens = data.type.level != log_level::trace && data.type.level != log_level::debug;
#else
			bool parse_tokens = data.type.level != log_level::trace;
#endif
			terminal->write_color(parse_tokens ? std_color::cyan : std_color::gray);
			if (has_flag(log_option::dated))
			{
				terminal->write(data.message.date);
				terminal->write(" ");
#ifndef NDEBUG
				if (data.origin.file != nullptr)
				{
					terminal->write_color(std_color::gray);
					terminal->write(data.origin.file);
					terminal->write(":");
					if (data.origin.line > 0)
					{
						char numeric[NUMSTR_SIZE];
						terminal->write(core::to_string_view(numeric, sizeof(numeric), data.origin.line));
						terminal->write(" ");
					}
				}
#endif
			}
			terminal->write_color(get_message_color(data));
			terminal->write(get_message_type(data));
			terminal->write(" ");
			if (parse_tokens)
				terminal->colorize(std_color::light_gray, std::string_view(data.message.data, data.message.size));
			else
				terminal->write(data.message.data);
			terminal->write("\n");
			terminal->clear_color();
		}
		void error_handling::pause() noexcept
		{
			os::process::interrupt();
		}
		void error_handling::cleanup() noexcept
		{
			delete context;
			context = nullptr;
		}
		void error_handling::set_callback(std::function<void(details&)>&& callback) noexcept
		{
			if (!context)
				context = new state();

			context->callback = std::move(callback);
		}
		void error_handling::set_flag(log_option option, bool active) noexcept
		{
			if (!context)
				context = new state();

			if (active)
				context->flags = context->flags | (uint32_t)option;
			else
				context->flags = context->flags & ~(uint32_t)option;
		}
		bool error_handling::has_flag(log_option option) noexcept
		{
			if (!context)
				return ((uint32_t)log_option::pretty) & (uint32_t)option;

			return context->flags & (uint32_t)option;
		}
		bool error_handling::has_callback() noexcept
		{
			return context != nullptr && context->callback != nullptr;
		}
		error_handling::tick error_handling::measure(const char* file, const char* function, int line, uint64_t threshold_ms) noexcept
		{
#ifndef NDEBUG
			VI_ASSERT(file != nullptr, "file should be set");
			VI_ASSERT(function != nullptr, "function should be set");
			VI_ASSERT(threshold_ms > 0 || threshold_ms == (uint64_t)timings::infinite, "threshold time should be greater than zero");
			if (internal_logging)
				return error_handling::tick(false);

			measurement next;
			next.file = file;
			next.function = function;
			next.threshold = threshold_ms * 1000;
			next.time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			next.line = line;

			internal_stacktrace.emplace(std::move(next));
			return error_handling::tick(true);
#else
			return error_handling::tick(false);
#endif
		}
		void error_handling::measure_loop() noexcept
		{
#ifndef NDEBUG
			if (!internal_logging)
			{
				VI_ASSERT(!internal_stacktrace.empty(), "debug frame should be set");
				auto& next = internal_stacktrace.top();
				next.notify_of_over_consumption();
			}
#endif
		}
		string error_handling::get_measure_trace() noexcept
		{
#ifndef NDEBUG
			auto source = internal_stacktrace;
			string_stream stream;
			stream << "in thread " << std::this_thread::get_id() << ":\n";

			size_t size = source.size();
			for (size_t trace_idx = source.size(); trace_idx > 0; --trace_idx)
			{
				auto& frame = source.top();
				stream << "  #" << (size - trace_idx) + 1 << " at " << os::path::get_filename(frame.file);
				if (frame.line > 0)
					stream << ":" << frame.line;
				stream << " in " << frame.function << "()";
				if (frame.id != nullptr)
					stream << " pointed here 0x" << frame.id;
				stream << " - expects < " << frame.threshold / 1000 << " ms\n";
				source.pop();
			}

			string out(stream.str());
			return out.substr(0, out.size() - 1);
#else
			return string();
#endif
		}
		string error_handling::get_stack_trace(size_t skips, size_t max_frames) noexcept
		{
			stack_trace stack(skips, max_frames);
			if (!stack)
				return "  * #0 [unavailable]";

			string_stream stream;
			size_t size = stack.size();
			for (auto& frame : stack)
			{
				stream << "  #" << --size << " at " << os::path::get_filename(frame.file.c_str());
				if (frame.line > 0)
					stream << ":" << frame.line;
				if (frame.column > 0)
					stream << "," << frame.column;
				stream << " in " << frame.function;
				if (!frame.native)
					stream << " (vcall)";
				if (!frame.code.empty())
				{
					auto lines = stringify::split(frame.code, '\n');
					for (size_t i = 0; i < lines.size(); i++)
					{
						stream << "  " << lines[i];
						if (i + 1 < lines.size())
							stream << "\n";
					}
				}
				if (size > 0)
					stream << "\n";
			}

			return stream.str();
		}
		std::string_view error_handling::get_message_type(const details& base) noexcept
		{
			switch (base.type.level)
			{
				case log_level::error:
					return "ERROR";
				case log_level::warning:
					return "WARN";
				case log_level::info:
					return "INFO";
				case log_level::debug:
					return "DEBUG";
				case log_level::trace:
					return "TRACE";
				default:
					return "LOG";
			}
		}
		std_color error_handling::get_message_color(const details& base) noexcept
		{
			switch (base.type.level)
			{
				case log_level::error:
					return std_color::dark_red;
				case log_level::warning:
					return std_color::orange;
				case log_level::info:
					return std_color::light_blue;
				case log_level::debug:
					return std_color::gray;
				case log_level::trace:
					return std_color::gray;
				default:
					return std_color::light_gray;
			}
		}
		string error_handling::get_message_text(const details& base) noexcept
		{
			string_stream stream;
			if (has_flag(log_option::dated))
			{
				stream << base.message.date;
#ifndef NDEBUG
				if (base.origin.file != nullptr)
				{
					stream << ' ' << base.origin.file << ':';
					if (base.origin.line > 0)
						stream << base.origin.line;
				}
#endif
				stream << ' ';
			}
			stream << get_message_type(base) << ' ';
			stream << base.message.data << '\n';
			return stream.str();
		}
		error_handling::state* error_handling::context = nullptr;

		coroutine::coroutine(costate* base, task_callback&& procedure) noexcept : state(coexecution::active), callback(std::move(procedure)), slave(memory::init<cocontext>(base)), master(base), user_data(nullptr)
		{
		}
		coroutine::~coroutine() noexcept
		{
			memory::deinit(slave);
		}

		decimal::decimal() noexcept : length(0), sign('\0')
		{
		}
		decimal::decimal(const std::string_view& text) noexcept : length(0)
		{
			apply_base10(text);
		}
		decimal::decimal(const decimal& value) noexcept : source(value.source), length(value.length), sign(value.sign)
		{
		}
		decimal::decimal(decimal&& value) noexcept : source(std::move(value.source)), length(value.length), sign(value.sign)
		{
		}
		decimal& decimal::truncate(uint32_t precision)
		{
			if (is_nan())
				return *this;

			int32_t new_precision = precision;
			if (length < new_precision)
			{
				while (length < new_precision)
				{
					length++;
					source.insert(0, 1, '0');
				}
			}
			else if (length > new_precision)
			{
				while (length > new_precision)
				{
					length--;
					source.erase(0, 1);
				}
			}

			return *this;
		}
		decimal& decimal::round(uint32_t precision)
		{
			if (is_nan())
				return *this;

			int32_t new_precision = precision;
			if (length < new_precision)
			{
				while (length < new_precision)
				{
					length++;
					source.insert(0, 1, '0');
				}
			}
			else if (length > new_precision)
			{
				char last;
				while (length > new_precision)
				{
					last = source[0];
					length--;
					source.erase(0, 1);
				}

				if (char_to_int(last) >= 5)
				{
					if (new_precision != 0)
					{
						string result = "0.";
						result.reserve(3 + (size_t)new_precision);
						for (int32_t i = 1; i < new_precision; i++)
							result += '0';
						result += '1';

						decimal temp(result);
						*this = *this + temp;
					}
					else
						++(*this);
				}
			}

			return *this;
		}
		decimal& decimal::trim()
		{
			return unlead().untrail();
		}
		decimal& decimal::unlead()
		{
			for (int32_t i = (int32_t)source.size() - 1; i > length; --i)
			{
				if (source[i] != '0')
					break;

				source.pop_back();
			}

			return *this;
		}
		decimal& decimal::untrail()
		{
			if (is_nan() || source.empty())
				return *this;

			while ((source[0] == '0') && (length > 0))
			{
				source.erase(0, 1);
				length--;
			}

			return *this;
		}
		void decimal::apply_base10(const std::string_view& text)
		{
			if (text.empty())
			{
			invalid_number:
				source.clear();
				length = 0;
				sign = '\0';
				return;
			}
			else if (text.size() == 1)
			{
				uint8_t value = (uint8_t)text.front();
				if (!isdigit(value))
					goto invalid_number;

				source.push_back(value);
				sign = '+';
				return;
			}

			source.reserve(text.size());
			sign = text[0];

			size_t index = 0;
			if (sign != '+' && sign != '-')
			{
				if (!isdigit(sign))
					goto invalid_number;
				sign = '+';
			}
			else
				index = 1;

			size_t position = text.find('.', index);
			size_t base = std::min(position, text.size());
			while (index < base)
			{
				uint8_t value = text[index++];
				source.push_back(value);
				if (!isdigit(value))
					goto invalid_number;
			}

			if (position == std::string::npos)
			{
				std::reverse(source.begin(), source.end());
				return;
			}

			++index;
			while (index < text.size())
			{
				uint8_t value = text[index++];
				source.push_back(value);
				if (!isdigit(value))
					goto invalid_number;
				++length;
			}

			std::reverse(source.begin(), source.end());
		}
		void decimal::apply_zero()
		{
			source.push_back('0');
			sign = '+';
		}
		bool decimal::is_nan() const
		{
			return sign == '\0';
		}
		bool decimal::is_zero() const
		{
			for (char item : source)
			{
				if (item != '0')
					return false;
			}

			return true;
		}
		bool decimal::is_zero_or_nan() const
		{
			return is_nan() || is_zero();
		}
		bool decimal::is_positive() const
		{
			return !is_nan() && *this > 0.0;
		}
		bool decimal::is_negative() const
		{
			return !is_nan() && *this < 0.0;
		}
		bool decimal::is_integer() const
		{
			return !length;
		}
		bool decimal::is_fractional() const
		{
			return length > 0;
		}
		bool decimal::is_safe_number() const
		{
			if (is_nan())
				return true;

			auto numeric = to_string();
			if (is_fractional())
			{
				auto number = from_string<double>(numeric);
				if (!number)
					return false;

				char buffer[NUMSTR_SIZE];
				return to_string_view(buffer, sizeof(buffer), *number) == numeric;
			}
			else if (is_positive())
			{
				auto number = from_string<uint64_t>(numeric);
				if (!number)
					return false;

				char buffer[NUMSTR_SIZE];
				return to_string_view(buffer, sizeof(buffer), *number) == numeric;
			}
			else
			{
				auto number = from_string<int64_t>(numeric);
				if (!number)
					return false;

				char buffer[NUMSTR_SIZE];
				return to_string_view(buffer, sizeof(buffer), *number) == numeric;
			}
		}
		double decimal::to_double() const
		{
			if (is_nan())
				return std::nan("");

			double dec = 1;
			if (length > 0)
			{
				int32_t aus = length;
				while (aus != 0)
				{
					dec /= 10;
					aus--;
				}
			}

			double var = 0;
			for (char symbol : source)
			{
				var += char_to_int(symbol) * dec;
				dec *= 10;
			}

			if (sign == '-')
				var *= -1;

			return var;
		}
		float decimal::to_float() const
		{
			return (float)to_double();
		}
		int8_t decimal::to_int8() const
		{
			return (int8_t)to_int16();
		}
		uint8_t decimal::to_uint8() const
		{
			return (uint8_t)to_uint16();
		}
		int16_t decimal::to_int16() const
		{
			return (int16_t)to_int32();
		}
		uint16_t decimal::to_uint16() const
		{
			return (uint16_t)to_uint32();
		}
		int32_t decimal::to_int32() const
		{
			return (int32_t)to_int64();
		}
		uint32_t decimal::to_uint32() const
		{
			return (uint32_t)to_uint64();
		}
		int64_t decimal::to_int64() const
		{
			if (is_nan() || source.empty())
				return 0;

			string result;
			if (sign == '-')
				result += sign;

			int32_t offset = 0, size = length;
			while ((source[offset] == '0') && (size > 0))
			{
				offset++;
				size--;
			}

			for (int32_t i = (int32_t)source.size() - 1; i >= offset; i--)
			{
				result += source[i];
				if ((i == length) && (i != 0) && offset != length)
					break;
			}

			return strtoll(result.c_str(), nullptr, 10);
		}
		uint64_t decimal::to_uint64() const
		{
			if (is_nan() || source.empty())
				return 0;

			string result;
			int32_t offset = 0, size = length;
			while ((source[offset] == '0') && (size > 0))
			{
				offset++;
				size--;
			}

			for (int32_t i = (int32_t)source.size() - 1; i >= offset; i--)
			{
				result += source[i];
				if ((i == length) && (i != 0) && offset != length)
					break;
			}

			return strtoull(result.c_str(), nullptr, 10);
		}
		string decimal::to_string() const
		{
			if (is_nan() || source.empty())
				return "NaN";

			string result;
			if (sign == '-')
				result += sign;

			int32_t offset = 0, size = length;
			while ((source[offset] == '0') && (size > 0))
			{
				offset++;
				size--;
			}

			for (int32_t i = (int32_t)source.size() - 1; i >= offset; i--)
			{
				result += source[i];
				if ((i == length) && (i != 0) && offset != length)
					result += '.';
			}

			return result;
		}
		string decimal::to_exponent() const
		{
			if (is_nan())
				return "NaN";

			string result;
			int compare = decimal::compare_num(*this, decimal(1));
			if (compare == 0)
			{
				result += sign;
				result += "1e+0";
			}
			else if (compare == 1)
			{
				result += sign;
				int32_t i = (int32_t)source.size() - 1;
				result += source[i];
				i--;

				if (i > 0)
				{
					result += '.';
					for (; (i >= (int32_t)source.size() - 6) && (i >= 0); --i)
						result += source[i];
				}
				result += "e+";
				result += core::to_string(integer_places() - 1);
			}
			else if (compare == 2)
			{
				int32_t exp = 0, count = (int32_t)source.size() - 1;
				while (count > 0 && source[count] == '0')
				{
					count--;
					exp++;
				}

				if (count == 0)
				{
					if (source[count] != '0')
					{
						result += sign;
						result += source[count];
						result += "e-";
						result += core::to_string(exp);
					}
					else
						result += "+0";
				}
				else
				{
					result += sign;
					result += source[count];
					result += '.';

					for (int32_t i = count - 1; (i >= count - 5) && (i >= 0); --i)
						result += source[i];

					result += "e-";
					result += core::to_string(exp);
				}
			}

			return result;
		}
		const string& decimal::numeric() const
		{
			return source;
		}
		uint32_t decimal::decimal_places() const
		{
			return length;
		}
		uint32_t decimal::integer_places() const
		{
			return (int32_t)source.size() - length;
		}
		uint32_t decimal::size() const
		{
			return (int32_t)(sizeof(*this) + source.size() * sizeof(char));
		}
		int8_t decimal::position() const
		{
			switch (sign)
			{
				case '+':
					return 1;
				case '-':
					return -1;
				default:
					return 0;
			}
		}
		decimal decimal::operator -() const
		{
			decimal result = *this;
			if (result.sign == '+')
				result.sign = '-';
			else if (result.sign == '-')
				result.sign = '+';

			return result;
		}
		decimal& decimal::operator *=(const decimal& v)
		{
			*this = *this * v;
			return *this;
		}
		decimal& decimal::operator /=(const decimal& v)
		{
			*this = *this / v;
			return *this;
		}
		decimal& decimal::operator +=(const decimal& v)
		{
			*this = *this + v;
			return *this;
		}
		decimal& decimal::operator -=(const decimal& v)
		{
			*this = *this - v;
			return *this;
		}
		decimal& decimal::operator=(const decimal& value) noexcept
		{
			source = value.source;
			length = value.length;
			sign = value.sign;
			return *this;
		}
		decimal& decimal::operator=(decimal&& value) noexcept
		{
			source = std::move(value.source);
			length = value.length;
			sign = value.sign;
			return *this;
		}
		decimal& decimal::operator++(int)
		{
			*this = *this + 1;
			return *this;
		}
		decimal& decimal::operator++()
		{
			*this = *this + 1;
			return *this;
		}
		decimal& decimal::operator--(int)
		{
			*this = *this - 1;
			return *this;
		}
		decimal& decimal::operator--()
		{
			*this = *this - 1;
			return *this;
		}
		bool decimal::operator==(const decimal& right) const
		{
			if (is_nan() || right.is_nan())
				return false;

			int check = compare_num(*this, right);
			if ((check == 0) && (sign == right.sign))
				return true;

			return false;
		}
		bool decimal::operator!=(const decimal& right) const
		{
			if (is_nan() || right.is_nan())
				return false;

			return !(*this == right);
		}
		bool decimal::operator>(const decimal& right) const
		{
			if (is_nan() || right.is_nan())
				return false;

			if (((sign == '+') && (right.sign == '+')))
			{
				int check = compare_num(*this, right);
				return check == 1;
			}

			if (((sign == '-') && (right.sign == '-')))
			{
				int check = compare_num(*this, right);
				return check == 2;
			}

			if (((sign == '-') && (right.sign == '+')))
				return false;

			if (((sign == '+') && (right.sign == '-')))
				return true;

			return false;
		}
		bool decimal::operator>=(const decimal& right) const
		{
			if (is_nan() || right.is_nan())
				return false;

			return !(*this < right);
		}
		bool decimal::operator<(const decimal& right) const
		{
			if (is_nan() || right.is_nan())
				return false;

			if (((sign == '+') && (right.sign == '+')))
			{
				int check = compare_num(*this, right);
				return check == 2;
			}

			if (((sign == '-') && (right.sign == '-')))
			{
				int check = compare_num(*this, right);
				return check == 1;
			}

			if (((sign == '-') && (right.sign == '+')))
				return true;

			if (((sign == '+') && (right.sign == '-')))
				return false;

			return false;
		}
		bool decimal::operator<=(const decimal& right) const
		{
			if (is_nan() || right.is_nan())
				return false;

			return !(*this > right);
		}
		decimal operator+(const decimal& _Left, const decimal& _Right)
		{
			decimal temp;
			if (_Left.is_nan() || _Right.is_nan())
				return temp;

			decimal left, right;
			left = _Left;
			right = _Right;

			if (left.length > right.length)
			{
				while (left.length > right.length)
				{
					right.length++;
					right.source.insert(0, 1, '0');
				}
			}
			else if (left.length < right.length)
			{
				while (left.length < right.length)
				{
					left.length++;
					left.source.insert(0, 1, '0');
				}
			}

			if ((left.sign == '+') && (right.sign == '-'))
			{
				int check = decimal::compare_num(left, right);
				if (check == 0)
				{
					temp = 0;
					return temp;
				}

				if (check == 1)
				{
					temp = decimal::subtract(left, right);
					temp.sign = '+';
					temp.length = left.length;
					temp.unlead();
					return temp;
				}

				if (check == 2)
				{
					temp = decimal::subtract(right, left);
					temp.sign = '-';
					temp.length = left.length;
					temp.unlead();
					return temp;
				}
			}

			if ((left.sign == '-') && (right.sign == '+'))
			{
				int check = decimal::compare_num(left, right);
				if (check == 0)
				{
					temp = 0;
					return temp;
				}

				if (check == 1)
				{
					temp = decimal::subtract(left, right);
					temp.sign = '-';
					temp.length = left.length;
					temp.unlead();
					return temp;
				}

				if (check == 2)
				{
					temp = decimal::subtract(right, left);
					temp.sign = '+';
					temp.length = left.length;
					temp.unlead();
					return temp;
				}
			}

			if ((left.sign == '+') && (right.sign == '+'))
			{
				temp = decimal::sum(left, right);
				temp.sign = '+';
				temp.length = left.length;
				return temp;
			}

			if ((left.sign == '-') && (right.sign == '-'))
			{
				temp = decimal::sum(left, right);
				temp.sign = '-';
				temp.length = left.length;
				return temp;
			}

			return temp;
		}
		decimal operator+(const decimal& left, const int& vright)
		{
			decimal right;
			right = vright;
			return left + right;
		}
		decimal operator+(const decimal& left, const double& vright)
		{
			decimal right;
			right = vright;
			return left + right;
		}
		decimal operator-(const decimal& _Left, const decimal& _Right)
		{
			decimal temp;
			if (_Left.is_nan() || _Right.is_nan())
				return temp;

			decimal left, right;
			left = _Left;
			right = _Right;

			if (left.length > right.length)
			{
				while (left.length > right.length)
				{
					right.length++;
					right.source.insert(0, 1, '0');
				}
			}
			else if (left.length < right.length)
			{
				while (left.length < right.length)
				{
					left.length++;
					left.source.insert(0, 1, '0');
				}
			}

			if ((left.sign == '+') && (right.sign == '-'))
			{
				temp = decimal::sum(left, right);
				temp.sign = '+';
				temp.length = left.length;
				return temp;
			}
			if ((left.sign == '-') && (right.sign == '+'))
			{
				temp = decimal::sum(left, right);
				temp.sign = '-';
				temp.length = left.length;
				return temp;
			}

			if ((left.sign == '+') && (right.sign == '+'))
			{
				int check = decimal::compare_num(left, right);
				if (check == 0)
				{
					temp = 0;
					return temp;
				}

				if (check == 1)
				{
					temp = decimal::subtract(left, right);
					temp.sign = '+';
					temp.length = left.length;
					temp.unlead();
					return temp;
				}

				if (check == 2)
				{
					temp = decimal::subtract(right, left);
					temp.sign = '-';
					temp.length = left.length;
					temp.unlead();
					return temp;
				}
			}

			if ((left.sign == '-') && (right.sign == '-'))
			{
				int check = decimal::compare_num(left, right);
				if (check == 0)
				{
					temp = 0;
					return temp;
				}

				if (check == 1)
				{
					temp = decimal::subtract(left, right);
					temp.sign = '-';
					temp.length = left.length;
					temp.unlead();
					return temp;
				}

				if (check == 2)
				{
					temp = decimal::subtract(right, left);
					temp.sign = '+';
					temp.length = left.length;
					temp.unlead();
					return temp;
				}
			}

			return temp;
		}
		decimal operator-(const decimal& left, const int& vright)
		{
			decimal right;
			right = vright;
			return left - right;
		}
		decimal operator-(const decimal& left, const double& vright)
		{
			decimal right;
			right = vright;
			return left - right;
		}
		decimal operator*(const decimal& left, const decimal& right)
		{
			decimal temp;
			if (left.is_nan() || right.is_nan())
				return temp;

			temp = decimal::multiply(left, right);
			if (((left.sign == '-') && (right.sign == '-')) || ((left.sign == '+') && (right.sign == '+')))
				temp.sign = '+';
			else
				temp.sign = '-';

			temp.length = left.length + right.length;
			temp.unlead();

			return temp;
		}
		decimal operator*(const decimal& left, const int& vright)
		{
			decimal right;
			right = vright;
			return left * right;
		}
		decimal operator*(const decimal& left, const double& vright)
		{
			decimal right;
			right = vright;
			return left * right;
		}
		decimal operator/(const decimal& left, const decimal& right)
		{
			decimal temp;
			if (left.is_nan() || right.is_nan())
				return temp;

			decimal q, r, d, n, zero;
			zero = 0;

			if (right == zero)
				return temp;

			n = (left > zero) ? (left) : (left * (-1));
			d = (right > zero) ? (right) : (right * (-1));
			r.sign = '+';

			while ((n.length != 0) || (d.length != 0))
			{
				if (n.length == 0)
					n.source.insert(0, 1, '0');
				else
					n.length--;

				if (d.length == 0)
					d.source.insert(0, 1, '0');
				else
					d.length--;
			}

			n.unlead();
			d.unlead();

			int32_t div_precision = (left.length > right.length) ? (left.length) : (right.length);
			for (int32_t i = 0; i < div_precision; i++)
				n.source.insert(0, 1, '0');

			int check = decimal::compare_num(n, d);
			if (check == 0)
				temp.source.insert(0, 1, '1');

			if (check == 2)
				return zero;

			while (!n.source.empty())
			{
				r.source.insert(0, 1, *(n.source.rbegin()));
				n.source.pop_back();

				bool is_zero = true;
				auto zero_it = r.source.begin();
				for (; zero_it != r.source.end(); ++zero_it)
				{
					if (*zero_it != '0')
						is_zero = false;
				}

				if ((r >= d) && (!is_zero))
				{
					int32_t qsub = 0;
					int32_t min = 0;
					int32_t max = 9;

					while (r >= d)
					{
						int32_t avg = max - min;
						int32_t mod_avg = avg / 2;
						avg = (avg - mod_avg * 2) ? (mod_avg + 1) : (mod_avg);

						int div_check = decimal::compare_num(r, d * avg);
						if (div_check != 2)
						{
							qsub = qsub + avg;
							r = r - d * avg;

							max = 9;
						}
						else
							max = avg;
					}

					q.source.insert(0, 1, decimal::int_to_char(qsub));

					bool is_zero = true;
					auto zero_it = r.source.begin();
					for (; zero_it != r.source.end(); ++zero_it)
					{
						if (*zero_it != '0')
							is_zero = false;
					}

					if (is_zero)
						r.source.clear();
				}
				else
					q.source.insert(0, 1, '0');
			}

			temp = q;
			if (((left.sign == '-') && (right.sign == '-')) || ((left.sign == '+') && (right.sign == '+')))
				temp.sign = '+';
			else
				temp.sign = '-';
			temp.length = div_precision;
			temp.unlead();
			return temp;
		}
		decimal operator/(const decimal& left, const int& vright)
		{
			decimal right;
			right = vright;
			return left / right;
		}
		decimal operator/(const decimal& left, const double& vright)
		{
			decimal right;
			right = vright;
			return left / right;
		}
		decimal operator%(const decimal& left, const decimal& right)
		{
			decimal temp;
			if (left.is_nan() || right.is_nan())
				return temp;

			if ((left.length != 0) || (right.length != 0))
				return temp;

			decimal q, r, d, n, zero, result;
			zero = 0;

			if (right == zero)
				return temp;

			n = (left > zero) ? (left) : (left * (-1));
			d = (right > zero) ? (right) : (right * (-1));
			r.sign = '+';

			int check = decimal::compare_num(n, d);
			if (check == 0)
				return zero;

			if (check == 2)
				return left;

			while (!n.source.empty())
			{
				r.source.insert(0, 1, *(n.source.rbegin()));
				n.source.pop_back();

				bool is_zero = true;
				auto zero_it = r.source.begin();
				for (; zero_it != r.source.end(); ++zero_it)
				{
					if (*zero_it != '0')
						is_zero = false;
				}

				if ((r >= d) && (!is_zero))
				{
					int32_t qsub = 0;
					int32_t min = 0;
					int32_t max = 9;

					while (r >= d)
					{
						int32_t avg = max - min;
						int32_t mod_avg = avg / 2;
						avg = (avg - mod_avg * 2) ? (mod_avg + 1) : (mod_avg);

						int div_check = decimal::compare_num(r, d * avg);
						if (div_check != 2)
						{
							qsub = qsub + avg;
							r = r - d * avg;

							max = 9;
						}
						else
							max = avg;
					}

					q.source.insert(0, 1, decimal::int_to_char(qsub));
					result = r;

					bool is_zero = true;
					auto zero_it = r.source.begin();
					for (; zero_it != r.source.end(); ++zero_it)
					{
						if (*zero_it != '0')
							is_zero = false;
					}

					if (is_zero)
						r.source.clear();
				}
				else
				{
					result = r;
					q.source.insert(0, 1, '0');
				}
			}

			q.unlead();
			result.unlead();
			temp = result;
			if (((left.sign == '-') && (right.sign == '-')) || ((left.sign == '+') && (right.sign == '+')))
				temp.sign = '+';
			else
				temp.sign = '-';
			if (!decimal::compare_num(temp, zero))
				temp.sign = '+';
			return temp;
		}
		decimal operator%(const decimal& left, const int& vright)
		{
			decimal right;
			right = vright;
			return left % right;
		}
		decimal decimal::from(const std::string_view& data, uint8_t base)
		{
			static uint8_t mapping[] =
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
			decimal radix = decimal(base);
			decimal value = decimal("0");
			for (auto& item : data)
			{
				uint8_t number = mapping[(uint8_t)item];
				value = value * radix + decimal(number);
			}

			return value;
		}
		decimal decimal::zero()
		{
			decimal result;
			result.apply_zero();
			return result;
		}
		decimal decimal::nan()
		{
			decimal result;
			return result;
		}
		decimal decimal::sum(const decimal& left, const decimal& right)
		{
			decimal temp;
			size_t loop_size = (left.source.size() > right.source.size() ? left.source.size() : right.source.size());
			int32_t carry = 0;

			for (size_t i = 0; i < loop_size; ++i)
			{
				int32_t val1, val2;
				val1 = (i > left.source.size() - 1) ? 0 : char_to_int(left.source[i]);
				val2 = (i > right.source.size() - 1) ? 0 : char_to_int(right.source[i]);

				int32_t aus = val1 + val2 + carry;
				carry = 0;

				if (aus > 9)
				{
					carry = 1;
					aus = aus - 10;
				}

				temp.source.push_back(int_to_char(aus));
			}

			if (carry != 0)
				temp.source.push_back(int_to_char(carry));

			return temp;
		}
		decimal decimal::subtract(const decimal& left, const decimal& right)
		{
			decimal temp;
			int32_t carry = 0;
			int32_t aus;

			for (size_t i = 0; i < left.source.size(); ++i)
			{
				int32_t val1, val2;
				val1 = char_to_int(left.source[i]);
				val2 = (i > right.source.size() - 1) ? 0 : char_to_int(right.source[i]);
				val1 -= carry;

				if (val1 < val2)
				{
					aus = 10 + val1 - val2;
					carry = 1;
				}
				else
				{
					aus = val1 - val2;
					carry = 0;
				}

				temp.source.push_back(int_to_char(aus));
			}

			return temp;
		}
		decimal decimal::multiply(const decimal& left, const decimal& right)
		{
			decimal result;
			decimal temp;
			result.source.push_back('0');
			int32_t carry = 0;

			for (size_t i = 0; i < right.source.size(); ++i)
			{
				for (size_t k = 0; k < i; ++k)
					temp.source.insert(0, 1, '0');

				for (size_t j = 0; j < left.source.size(); ++j)
				{
					int32_t aus = char_to_int(right.source[i]) * char_to_int(left.source[j]) + carry;
					carry = 0;
					if (aus > 9)
					{
						while (aus > 9)
						{
							carry++;
							aus -= 10;
						}
					}

					temp.source.push_back(int_to_char(aus));
				}

				if (carry != 0)
					temp.source.push_back(int_to_char(carry));

				carry = 0;
				result = sum(result, temp);
				temp.source.clear();
			}

			return result;
		}
		int decimal::compare_num(const decimal& left, const decimal& right)
		{
			if ((left.source.size() - left.length) > (right.source.size() - right.length))
				return 1;

			if ((left.source.size() - left.length) < (right.source.size() - right.length))
				return 2;

			if (left.length > right.length)
			{
				decimal temp;
				temp = right;
				while (left.length > temp.length)
				{
					temp.length++;
					temp.source.insert(0, 1, '0');
				}

				for (int32_t i = (int32_t)left.source.size() - 1; i >= 0; i--)
				{
					if (left.source[i] > temp.source[i])
						return 1;

					if (left.source[i] < temp.source[i])
						return 2;
				}

				return 0;
			}
			else if (left.length < right.length)
			{
				decimal temp;
				temp = left;
				while (temp.length < right.length)
				{
					temp.length++;
					temp.source.insert(0, 1, '0');
				}

				for (int32_t i = (int32_t)temp.source.size() - 1; i >= 0; i--)
				{
					if (temp.source[i] > right.source[i])
						return 1;

					if (temp.source[i] < right.source[i])
						return 2;
				}

				return 0;
			}
			else
			{
				for (int32_t i = (int32_t)left.source.size() - 1; i >= 0; i--)
				{
					if (left.source[i] > right.source[i])
						return 1;
					else if (left.source[i] < right.source[i])
						return 2;
				}

				return 0;
			}
		}
		int decimal::char_to_int(char value)
		{
			return value - '0';
		}
		char decimal::int_to_char(const int& value)
		{
			return value + '0';
		}

		variant::variant() noexcept : type(var_type::undefined), length(0)
		{
			value.pointer = nullptr;
		}
		variant::variant(var_type new_type) noexcept : type(new_type), length(0)
		{
			value.pointer = nullptr;
		}
		variant::variant(const variant& other) noexcept
		{
			copy(other);
		}
		variant::variant(variant&& other) noexcept
		{
			move(std::move(other));
		}
		variant::~variant() noexcept
		{
			free();
		}
		bool variant::deserialize(const std::string_view& text, bool strict)
		{
			free();
			if (!strict && !text.empty())
			{
				if (text.size() > 2 && text.front() == PREFIX_ENUM[0] && text.back() == PREFIX_ENUM[0])
				{
					if (text == PREFIX_ENUM "null" PREFIX_ENUM)
					{
						type = var_type::null;
						return true;
					}
					else if (text == PREFIX_ENUM "undefined" PREFIX_ENUM)
					{
						type = var_type::undefined;
						return true;
					}
					else if (text == PREFIX_ENUM "{}" PREFIX_ENUM)
					{
						type = var_type::object;
						return true;
					}
					else if (text == PREFIX_ENUM "[]" PREFIX_ENUM)
					{
						type = var_type::array;
						return true;
					}
					else if (text == PREFIX_ENUM "void*" PREFIX_ENUM)
					{
						type = var_type::pointer;
						return true;
					}
				}
				else if (text.front() == 't' && text == "true")
				{
					move(var::boolean(true));
					return true;
				}
				else if (text.front() == 'f' && text == "false")
				{
					move(var::boolean(false));
					return true;
				}
				else if (stringify::has_number(text))
				{
					if (stringify::has_integer(text))
					{
						auto number = from_string<int64_t>(text);
						if (number)
						{
							move(var::integer(*number));
							return true;
						}
					}
					else if (stringify::has_decimal(text))
					{
						move(var::decimal_string(text));
						return true;
					}
					else
					{
						auto number = from_string<double>(text);
						if (number)
						{
							move(var::number(*number));
							return true;
						}
					}
				}
			}

			if (text.size() > 2 && text.front() == PREFIX_BINARY[0] && text.back() == PREFIX_BINARY[0])
			{
				auto data = compute::codec::bep45_decode(text.substr(1, text.size() - 2));
				move(var::binary((uint8_t*)data.data(), data.size()));
			}
			else
				move(var::string(text));

			return true;
		}
		string variant::serialize() const
		{
			switch (type)
			{
				case var_type::null:
					return PREFIX_ENUM "null" PREFIX_ENUM;
				case var_type::undefined:
					return PREFIX_ENUM "undefined" PREFIX_ENUM;
				case var_type::object:
					return PREFIX_ENUM "{}" PREFIX_ENUM;
				case var_type::array:
					return PREFIX_ENUM "[]" PREFIX_ENUM;
				case var_type::pointer:
					return PREFIX_ENUM "void*" PREFIX_ENUM;
				case var_type::string:
					return string(get_string());
				case var_type::binary:
					return PREFIX_BINARY + compute::codec::bep45_encode(get_string()) + PREFIX_BINARY;
				case var_type::decimal:
				{
					auto* data = ((decimal*)value.pointer);
					if (data->is_nan())
						return PREFIX_ENUM "null" PREFIX_ENUM;

					return data->to_string();
				}
				case var_type::integer:
					return core::to_string(value.integer);
				case var_type::number:
					return core::to_string(value.number);
				case var_type::boolean:
					return value.boolean ? "true" : "false";
				default:
					return "";
			}
		}
		string variant::get_blob() const
		{
			if (type == var_type::string || type == var_type::binary)
				return string(get_string());

			if (type == var_type::decimal)
				return ((decimal*)value.pointer)->to_string();

			if (type == var_type::integer)
				return core::to_string(get_integer());

			if (type == var_type::number)
				return core::to_string(get_number());

			if (type == var_type::boolean)
				return value.boolean ? "1" : "0";

			return "";
		}
		decimal variant::get_decimal() const
		{
			if (type == var_type::decimal)
				return *(decimal*)value.pointer;

			if (type == var_type::integer)
				return decimal(core::to_string(value.integer));

			if (type == var_type::number)
				return decimal(core::to_string(value.number));

			if (type == var_type::boolean)
				return decimal(value.boolean ? "1" : "0");

			if (type == var_type::string)
				return decimal(get_string());

			return decimal::nan();
		}
		void* variant::get_pointer() const
		{
			if (type == var_type::pointer)
				return (void*)value.pointer;

			return nullptr;
		}
		void* variant::get_container()
		{
			switch (type)
			{
				case var_type::pointer:
					return value.pointer;
				case var_type::string:
				case var_type::binary:
					return (void*)get_string().data();
				case var_type::integer:
					return &value.integer;
				case var_type::number:
					return &value.number;
				case var_type::decimal:
					return value.pointer;
				case var_type::boolean:
					return &value.boolean;
				case var_type::null:
				case var_type::undefined:
				case var_type::object:
				case var_type::array:
				default:
					return nullptr;
			}
		}
		std::string_view variant::get_string() const
		{
			if (type != var_type::string && type != var_type::binary)
				return std::string_view("", 0);

			return std::string_view(length <= get_max_small_string_size() ? value.string : value.pointer, length);
		}
		uint8_t* variant::get_binary() const
		{
			if (type != var_type::string && type != var_type::binary)
				return nullptr;

			return length <= get_max_small_string_size() ? (uint8_t*)value.string : (uint8_t*)value.pointer;
		}
		int64_t variant::get_integer() const
		{
			if (type == var_type::integer)
				return value.integer;

			if (type == var_type::number)
				return (int64_t)value.number;

			if (type == var_type::decimal)
				return (int64_t)((decimal*)value.pointer)->to_double();

			if (type == var_type::boolean)
				return value.boolean ? 1 : 0;

			if (type == var_type::string)
			{
				auto result = from_string<int64_t>(get_string());
				if (result)
					return *result;
			}

			return 0;
		}
		double variant::get_number() const
		{
			if (type == var_type::number)
				return value.number;

			if (type == var_type::integer)
				return (double)value.integer;

			if (type == var_type::decimal)
				return ((decimal*)value.pointer)->to_double();

			if (type == var_type::boolean)
				return value.boolean ? 1.0 : 0.0;

			if (type == var_type::string)
			{
				auto result = from_string<double>(get_string());
				if (result)
					return *result;
			}

			return 0.0;
		}
		bool variant::get_boolean() const
		{
			if (type == var_type::boolean)
				return value.boolean;

			if (type == var_type::number)
				return value.number > 0.0;

			if (type == var_type::integer)
				return value.integer > 0;

			if (type == var_type::decimal)
				return ((decimal*)value.pointer)->to_double() > 0.0;

			return size() > 0;
		}
		var_type variant::get_type() const
		{
			return type;
		}
		size_t variant::size() const
		{
			switch (type)
			{
				case var_type::null:
				case var_type::undefined:
				case var_type::object:
				case var_type::array:
					return 0;
				case var_type::pointer:
					return sizeof(void*);
				case var_type::string:
				case var_type::binary:
					return length;
				case var_type::decimal:
					return ((decimal*)value.pointer)->size();
				case var_type::integer:
					return sizeof(int64_t);
				case var_type::number:
					return sizeof(double);
				case var_type::boolean:
					return sizeof(bool);
			}

			return 0;
		}
		bool variant::operator== (const variant& other) const
		{
			return same(other);
		}
		bool variant::operator!= (const variant& other) const
		{
			return !same(other);
		}
		variant& variant::operator= (const variant& other) noexcept
		{
			free();
			copy(other);

			return *this;
		}
		variant& variant::operator= (variant&& other) noexcept
		{
			free();
			move(std::move(other));

			return *this;
		}
		variant::operator bool() const
		{
			return !empty();
		}
		bool variant::is_string(const std::string_view& text) const
		{
			return get_string() == text;
		}
		bool variant::is_object() const
		{
			return type == var_type::object || type == var_type::array;
		}
		bool variant::empty() const
		{
			switch (type)
			{
				case var_type::null:
				case var_type::undefined:
					return true;
				case var_type::object:
				case var_type::array:
					return false;
				case var_type::pointer:
					return value.pointer == nullptr;
				case var_type::string:
				case var_type::binary:
					return length == 0;
				case var_type::decimal:
					return ((decimal*)value.pointer)->to_double() == 0.0;
				case var_type::integer:
					return value.integer == 0;
				case var_type::number:
					return value.number == 0.0;
				case var_type::boolean:
					return value.boolean == false;
				default:
					return true;
			}
		}
		bool variant::is(var_type value) const
		{
			return type == value;
		}
		bool variant::same(const variant& other) const
		{
			if (type != other.type)
				return false;

			switch (type)
			{
				case var_type::null:
				case var_type::undefined:
					return true;
				case var_type::pointer:
					return get_pointer() == other.get_pointer();
				case var_type::string:
				case var_type::binary:
				{
					size_t sizing = size();
					if (sizing != other.size())
						return false;

					return get_string() == other.get_string();
				}
				case var_type::decimal:
					return (*(decimal*)value.pointer) == (*(decimal*)other.value.pointer);
				case var_type::integer:
					return get_integer() == other.get_integer();
				case var_type::number:
					return abs(get_number() - other.get_number()) < std::numeric_limits<double>::epsilon();
				case var_type::boolean:
					return get_boolean() == other.get_boolean();
				default:
					return false;
			}
		}
		void variant::copy(const variant& other)
		{
			type = other.type;
			length = other.length;

			switch (type)
			{
				case var_type::null:
				case var_type::undefined:
				case var_type::object:
				case var_type::array:
					value.pointer = nullptr;
					break;
				case var_type::pointer:
					value.pointer = other.value.pointer;
					break;
				case var_type::string:
				case var_type::binary:
				{
					size_t string_size = sizeof(char) * (length + 1);
					if (length > get_max_small_string_size())
						value.pointer = memory::allocate<char>(string_size);
					memcpy((void*)get_string().data(), other.get_string().data(), string_size);
					break;
				}
				case var_type::decimal:
				{
					decimal* from = (decimal*)other.value.pointer;
					value.pointer = (char*)memory::init<decimal>(*from);
					break;
				}
				case var_type::integer:
					value.integer = other.value.integer;
					break;
				case var_type::number:
					value.number = other.value.number;
					break;
				case var_type::boolean:
					value.boolean = other.value.boolean;
					break;
				default:
					value.pointer = nullptr;
					break;
			}
		}
		void variant::move(variant&& other)
		{
			type = other.type;
			length = other.length;

			switch (type)
			{
				case var_type::null:
				case var_type::undefined:
				case var_type::object:
				case var_type::array:
				case var_type::pointer:
				case var_type::decimal:
					value.pointer = other.value.pointer;
					other.value.pointer = nullptr;
					break;
				case var_type::string:
				case var_type::binary:
					if (length <= get_max_small_string_size())
						memcpy((void*)get_string().data(), other.get_string().data(), sizeof(char) * (length + 1));
					else
						value.pointer = other.value.pointer;
					other.value.pointer = nullptr;
					break;
				case var_type::integer:
					value.integer = other.value.integer;
					break;
				case var_type::number:
					value.number = other.value.number;
					break;
				case var_type::boolean:
					value.boolean = other.value.boolean;
					break;
				default:
					break;
			}

			other.type = var_type::undefined;
			other.length = 0;
		}
		void variant::free()
		{
			switch (type)
			{
				case var_type::pointer:
					value.pointer = nullptr;
					break;
				case var_type::string:
				case var_type::binary:
				{
					if (!value.pointer || length <= get_max_small_string_size())
						break;

					memory::deallocate(value.pointer);
					value.pointer = nullptr;
					break;
				}
				case var_type::decimal:
				{
					if (!value.pointer)
						break;

					decimal* buffer = (decimal*)value.pointer;
					memory::deinit(buffer);
					value.pointer = nullptr;
					break;
				}
				default:
					break;
			}
		}
		size_t variant::get_max_small_string_size()
		{
			return sizeof(tag::string) - 1;
		}

		timeout::timeout(task_callback&& new_callback, const std::chrono::microseconds& new_timeout, task_id new_id, bool new_alive) noexcept : expires(new_timeout), callback(std::move(new_callback)), id(new_id), alive(new_alive)
		{
		}
		timeout::timeout(const timeout& other) noexcept : expires(other.expires), callback(other.callback), id(other.id), alive(other.alive)
		{
		}
		timeout::timeout(timeout&& other) noexcept : expires(other.expires), callback(std::move(other.callback)), id(other.id), alive(other.alive)
		{
		}
		timeout& timeout::operator= (const timeout& other) noexcept
		{
			callback = other.callback;
			expires = other.expires;
			id = other.id;
			alive = other.alive;
			return *this;
		}
		timeout& timeout::operator= (timeout&& other) noexcept
		{
			callback = std::move(other.callback);
			expires = other.expires;
			id = other.id;
			alive = other.alive;
			return *this;
		}

		date_time::date_time() noexcept : date_time(std::chrono::system_clock::now().time_since_epoch())
		{
		}
		date_time::date_time(const struct tm& duration) noexcept : offset({ }), timepoint(duration), synchronized(false), globalized(false)
		{
			apply_timepoint();
		}
		date_time::date_time(std::chrono::system_clock::duration&& duration) noexcept : offset(std::move(duration)), timepoint({ }), synchronized(false), globalized(false)
		{
			apply_offset();
		}
		date_time& date_time::operator +=(const date_time& right)
		{
			offset += right.offset;
			return apply_offset(true);
		}
		date_time& date_time::operator -=(const date_time& right)
		{
			offset -= right.offset;
			return apply_offset(true);
		}
		bool date_time::operator >=(const date_time& right)
		{
			return offset >= right.offset;
		}
		bool date_time::operator <=(const date_time& right)
		{
			return offset <= right.offset;
		}
		bool date_time::operator >(const date_time& right)
		{
			return offset > right.offset;
		}
		bool date_time::operator <(const date_time& right)
		{
			return offset < right.offset;
		}
		bool date_time::operator ==(const date_time& right)
		{
			return offset == right.offset;
		}
		date_time date_time::operator +(const date_time& right) const
		{
			date_time init = *this;
			init.offset = offset + right.offset;
			return init.apply_offset(true);
		}
		date_time date_time::operator -(const date_time& right) const
		{
			date_time init = *this;
			init.offset = offset - right.offset;
			return init.apply_offset(true);
		}
		date_time& date_time::apply_offset(bool always)
		{
			if (!synchronized || always)
			{
				time_t time = std::chrono::duration_cast<std::chrono::seconds>(offset).count();
				if (globalized)
					global_time(&time, &timepoint);
				else
					local_time(&time, &timepoint);
				synchronized = true;
			}
			return *this;
		}
		date_time& date_time::apply_timepoint(bool always)
		{
			if (!synchronized || always)
			{
				offset = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(mktime(&timepoint)));
				synchronized = true;
			}
			return *this;
		}
		date_time& date_time::use_global_time()
		{
			globalized = true;
			return *this;
		}
		date_time& date_time::use_local_time()
		{
			globalized = false;
			return *this;
		}
		date_time& date_time::set_second(uint8_t value)
		{
			if (value > 60)
				value = 60;

			timepoint.tm_sec = (int)value;
			return apply_timepoint(true);
		}
		date_time& date_time::set_minute(uint8_t value)
		{
			if (value > 60)
				value = 60;
			else if (value < 1)
				value = 1;

			timepoint.tm_min = (int)value - 1;
			return apply_timepoint(true);
		}
		date_time& date_time::set_hour(uint8_t value)
		{
			if (value > 24)
				value = 24;
			else if (value < 1)
				value = 1;

			timepoint.tm_hour = (int)value - 1;
			return apply_timepoint(true);
		}
		date_time& date_time::set_day(uint8_t value)
		{
			uint8_t month = this->month(), days = 31;
			if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12)
				days = 31;
			else if (month != 2)
				days = 30;
			else
				days = 28;

			if (value > days)
				value = days;
			else if (value < 1)
				value = 1;

			if (timepoint.tm_mday > (int)value)
				timepoint.tm_yday = timepoint.tm_yday - timepoint.tm_mday + (int)value;
			else
				timepoint.tm_yday = timepoint.tm_yday - (int)value + timepoint.tm_mday;

			if (value <= 7)
				timepoint.tm_wday = (int)value - 1;
			else if (value <= 14)
				timepoint.tm_wday = (int)value - 8;
			else if (value <= 21)
				timepoint.tm_wday = (int)value - 15;
			else
				timepoint.tm_wday = (int)value - 22;

			timepoint.tm_mday = (int)value;
			return apply_timepoint(true);
		}
		date_time& date_time::set_week(uint8_t value)
		{
			if (value > 7)
				value = 7;
			else if (value < 1)
				value = 1;

			timepoint.tm_wday = (int)value - 1;
			return apply_timepoint(true);
		}
		date_time& date_time::set_month(uint8_t value)
		{
			if (value < 1)
				value = 1;
			else if (value > 12)
				value = 12;

			timepoint.tm_mon = (int)value - 1;
			return apply_timepoint(true);
		}
		date_time& date_time::set_year(uint32_t value)
		{
			if (value < 1900)
				value = 1900;

			timepoint.tm_year = (int)value - 1900;
			return apply_timepoint(true);
		}
		string date_time::serialize(const std::string_view& format) const
		{
			VI_ASSERT(stringify::is_cstring(format) && !format.empty(), "format should be set");
			char buffer[CHUNK_SIZE];
			strftime(buffer, sizeof(buffer), format.data(), &timepoint);
			return buffer;
		}
		uint8_t date_time::second() const
		{
			return timepoint.tm_sec;
		}
		uint8_t date_time::minute() const
		{
			return timepoint.tm_min;
		}
		uint8_t date_time::hour() const
		{
			return timepoint.tm_hour;
		}
		uint8_t date_time::day() const
		{
			return timepoint.tm_mday;
		}
		uint8_t date_time::week() const
		{
			return timepoint.tm_wday + 1;
		}
		uint8_t date_time::month() const
		{
			return timepoint.tm_mon + 1;
		}
		uint32_t date_time::year() const
		{
			return timepoint.tm_year + 1900;
		}
		int64_t date_time::nanoseconds() const
		{
			return std::chrono::duration_cast<std::chrono::nanoseconds>(offset).count();
		}
		int64_t date_time::microseconds() const
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(offset).count();
		}
		int64_t date_time::milliseconds() const
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(offset).count();
		}
		int64_t date_time::seconds() const
		{
			return std::chrono::duration_cast<std::chrono::seconds>(offset).count();
		}
		const struct tm& date_time::current_timepoint() const
		{
			return timepoint;
		}
		const std::chrono::system_clock::duration& date_time::current_offset() const
		{
			return offset;
		}
		std::chrono::system_clock::duration date_time::now()
		{
			return std::chrono::system_clock::now().time_since_epoch();
		}
		date_time date_time::from_nanoseconds(int64_t value)
		{
			return date_time(std::chrono::nanoseconds(value));
		}
		date_time date_time::from_microseconds(int64_t value)
		{
			return date_time(std::chrono::microseconds(value));
		}
		date_time date_time::from_milliseconds(int64_t value)
		{
			return date_time(std::chrono::milliseconds(value));
		}
		date_time date_time::from_seconds(int64_t value)
		{
			return date_time(std::chrono::seconds(value));
		}
		date_time date_time::from_serialized(const std::string_view& text, const std::string_view& format)
		{
			VI_ASSERT(stringify::is_cstring(format) && !format.empty(), "format should be set");
			std::istringstream stream = std::istringstream(std::string(text));
			stream.imbue(std::locale(setlocale(LC_ALL, nullptr)));

			tm date { };
			stream >> std::get_time(&date, format.data());
			return stream.fail() ? date_time(std::chrono::seconds(0)) : date_time(date);
		}
		string date_time::serialize_global(const std::chrono::system_clock::duration& time, const std::string_view& format)
		{
			char buffer[CHUNK_SIZE];
			return string(serialize_global(buffer, sizeof(buffer), time, format));
		}
		string date_time::serialize_local(const std::chrono::system_clock::duration& time, const std::string_view& format)
		{
			char buffer[CHUNK_SIZE];
			return string(serialize_local(buffer, sizeof(buffer), time, format));
		}
		std::string_view date_time::serialize_global(char* buffer, size_t length, const std::chrono::system_clock::duration& time, const std::string_view& format)
		{
			VI_ASSERT(buffer != nullptr && length > 0, "buffer should be set");
			VI_ASSERT(stringify::is_cstring(format) && !format.empty(), "format should be set");
			time_t offset = (time_t)std::chrono::duration_cast<std::chrono::seconds>(time).count();
			struct tm date { };
			if (global_time(&offset, &date))
				return std::string_view(buffer, strftime(buffer, length, format.data(), &date));

			memset(buffer, 0, length);
			return std::string_view(buffer, 0);
		}
		std::string_view date_time::serialize_local(char* buffer, size_t length, const std::chrono::system_clock::duration& time, const std::string_view& format)
		{
			VI_ASSERT(buffer != nullptr && length > 0, "buffer should be set");
			VI_ASSERT(stringify::is_cstring(format) && !format.empty(), "format should be set");
			time_t offset = (time_t)std::chrono::duration_cast<std::chrono::seconds>(time).count();
			struct tm date { };
			if (local_time(&offset, &date))
				return std::string_view(buffer, strftime(buffer, length, format.data(), &date));

			memset(buffer, 0, length);
			return std::string_view(buffer, 0);
		}
		std::string_view date_time::format_iso8601_time()
		{
			return "%FT%TZ";
		}
		std::string_view date_time::format_web_time()
		{
			return "%a, %d %b %Y %H:%M:%S GMT";
		}
		std::string_view date_time::format_web_local_time()
		{
			return "%a, %d %b %Y %H:%M:%S %Z";
		}
		std::string_view date_time::format_compact_time()
		{
			return "%Y-%m-%d %H:%M:%S";
		}
		int64_t date_time::seconds_from_serialized(const std::string_view& text, const std::string_view& format)
		{
			VI_ASSERT(stringify::is_cstring(format) && !format.empty(), "format should be set");
			std::istringstream stream = std::istringstream(std::string(text));
			stream.imbue(std::locale(setlocale(LC_ALL, nullptr)));

			tm date { };
			stream >> std::get_time(&date, format.data());
			return stream.fail() ? 0 : mktime(&date);
		}
		bool date_time::make_global_time(time_t time, struct tm* timepoint)
		{
			VI_ASSERT(timepoint != nullptr, "time should be set");
			return global_time(&time, timepoint);
		}
		bool date_time::make_local_time(time_t time, struct tm* timepoint)
		{
			VI_ASSERT(timepoint != nullptr, "time should be set");
			return local_time(&time, timepoint);
		}

		string& stringify::escape_print(string& other)
		{
			for (size_t i = 0; i < other.size(); i++)
			{
				if (other.at(i) != '%')
					continue;

				if (i + 1 < other.size())
				{
					if (other.at(i + 1) != '%')
					{
						other.insert(other.begin() + i, '%');
						i++;
					}
				}
				else
				{
					other.append(1, '%');
					i++;
				}
			}
			return other;
		}
		string& stringify::escape(string& other)
		{
			for (size_t i = 0; i < other.size(); i++)
			{
				char& v = other.at(i);
				if (v == '\"')
				{
					if (i > 0 && other.at(i - 1) == '\\')
						continue;
				}
				else if (v == '\n')
					v = 'n';
				else if (v == '\t')
					v = 't';
				else if (v == '\v')
					v = 'v';
				else if (v == '\b')
					v = 'b';
				else if (v == '\r')
					v = 'r';
				else if (v == '\f')
					v = 'f';
				else if (v == '\a')
					v = 'a';
				else
					continue;

				other.insert(other.begin() + i, '\\');
				i++;
			}
			return other;
		}
		string& stringify::unescape(string& other)
		{
			for (size_t i = 0; i < other.size(); i++)
			{
				if (other.at(i) != '\\' || i + 1 >= other.size())
					continue;

				char& v = other.at(i + 1);
				if (v == 'n')
					v = '\n';
				else if (v == 't')
					v = '\t';
				else if (v == 'v')
					v = '\v';
				else if (v == 'b')
					v = '\b';
				else if (v == 'r')
					v = '\r';
				else if (v == 'f')
					v = '\f';
				else if (v == 'a')
					v = '\a';
				else
					continue;

				other.erase(other.begin() + i);
			}
			return other;
		}
		string& stringify::to_upper(string& other)
		{
			std::transform(other.begin(), other.end(), other.begin(), ::toupper);
			return other;
		}
		string& stringify::to_lower(string& other)
		{
			std::transform(other.begin(), other.end(), other.begin(), ::tolower);
			return other;
		}
		string& stringify::clip(string& other, size_t length)
		{
			if (length < other.size())
				other.erase(length, other.size() - length);
			return other;
		}
		string& stringify::compress(string& other, const std::string_view& tokenbase, const std::string_view& not_in_between_of, size_t start)
		{
			size_t tokenbase_size = tokenbase.size();
			size_t nibo_size = not_in_between_of.size();
			char skip = '\0';

			for (size_t i = start; i < other.size(); i++)
			{
				for (size_t j = 0; j < nibo_size; j++)
				{
					char& next = other.at(i);
					if (next == not_in_between_of[j])
					{
						skip = next;
						++i;
						break;
					}
				}

				while (skip != '\0' && i < other.size() && other.at(i) != skip)
					++i;

				if (skip != '\0')
				{
					skip = '\0';
					if (i >= other.size())
						break;
				}

				char& next = other.at(i);
				if (next != ' ' && next != '\r' && next != '\n' && next != '\t')
					continue;

				bool removable = false;
				if (i > 0)
				{
					next = other.at(i - 1);
					for (size_t j = 0; j < tokenbase_size; j++)
					{
						if (next == tokenbase[j])
						{
							removable = true;
							break;
						}
					}
				}

				if (!removable && i + 1 < other.size())
				{
					next = other.at(i + 1);
					for (size_t j = 0; j < tokenbase_size; j++)
					{
						if (next == tokenbase[j])
						{
							removable = true;
							break;
						}
					}
				}

				if (removable)
					other.erase(other.begin() + i--);
				else
					other[i] = ' ';
			}
			return other;
		}
		string& stringify::replace_of(string& other, const std::string_view& chars, const std::string_view& to, size_t start)
		{
			VI_ASSERT(!chars.empty(), "match list and replacer should not be empty");
			text_settle result { };
			size_t offset = start, to_size = to.size();
			while ((result = find_of(other, chars, offset)).found)
			{
				erase_offsets(other, result.start, result.end);
				other.insert(result.start, to);
				offset = result.start + to_size;
			}
			return other;
		}
		string& stringify::replace_not_of(string& other, const std::string_view& chars, const std::string_view& to, size_t start)
		{
			VI_ASSERT(!chars.empty(), "match list and replacer should not be empty");
			text_settle result { };
			size_t offset = start, to_size = to.size();
			while ((result = find_not_of(other, chars, offset)).found)
			{
				erase_offsets(other, result.start, result.end);
				other.insert(result.start, to);
				offset = result.start + to_size;
			}
			return other;
		}
		string& stringify::replace(string& other, const std::string_view& from, const std::string_view& to, size_t start)
		{
			VI_ASSERT(!from.empty(), "match should not be empty");
			size_t offset = start;
			text_settle result { };

			while ((result = find(other, from, offset)).found)
			{
				erase_offsets(other, result.start, result.end);
				other.insert(result.start, to);
				offset = result.start + to.size();
			}
			return other;
		}
		string& stringify::replace_groups(string& other, const std::string_view& from_regex, const std::string_view& to)
		{
			compute::regex_source source('(' + string(from_regex) + ')');
			compute::regex::replace(&source, to, other);
			return other;
		}
		string& stringify::replace(string& other, char from, char to, size_t position)
		{
			for (size_t i = position; i < other.size(); i++)
			{
				char& c = other.at(i);
				if (c == from)
					c = to;
			}
			return other;
		}
		string& stringify::replace(string& other, char from, char to, size_t position, size_t count)
		{
			VI_ASSERT(other.size() >= (position + count), "invalid offset");
			size_t size = position + count;
			for (size_t i = position; i < size; i++)
			{
				char& c = other.at(i);
				if (c == from)
					c = to;
			}
			return other;
		}
		string& stringify::replace_part(string& other, size_t start, size_t end, const std::string_view& value)
		{
			VI_ASSERT(start < other.size(), "invalid start");
			VI_ASSERT(end <= other.size(), "invalid end");
			VI_ASSERT(start < end, "start should be less than end");
			if (start == 0)
			{
				if (other.size() != end)
					other.assign(string(value) + other.substr(end, other.size() - end));
				else
					other.assign(value);
			}
			else if (other.size() == end)
				other.assign(other.substr(0, start) + string(value));
			else
				other.assign(other.substr(0, start) + string(value) + other.substr(end, other.size() - end));
			return other;
		}
		string& stringify::replace_starts_with_ends_of(string& other, const std::string_view& begins, const std::string_view& ends_of, const std::string_view& with, size_t start)
		{
			VI_ASSERT(!begins.empty(), "begin should not be empty");
			VI_ASSERT(!ends_of.empty(), "end should not be empty");

			size_t begins_size = begins.size(), ends_of_size = ends_of.size();
			for (size_t i = start; i < other.size(); i++)
			{
				size_t from = i, begins_offset = 0;
				while (begins_offset < begins_size && from < other.size() && other.at(from) == begins[begins_offset])
				{
					++from;
					++begins_offset;
				}

				bool matching = false;
				if (begins_offset != begins_size)
				{
					i = from;
					continue;
				}

				size_t to = from;
				while (!matching && to < other.size())
				{
					auto& next = other.at(to++);
					for (size_t j = 0; j < ends_of_size; j++)
					{
						if (next == ends_of[j])
						{
							matching = true;
							break;
						}
					}
				}

				if (to >= other.size())
					matching = true;

				if (!matching)
					continue;

				other.replace(other.begin() + from - begins_size, other.begin() + to, with);
				i = with.size();
			}
			return other;
		}
		string& stringify::replace_in_between(string& other, const std::string_view& begins, const std::string_view& ends, const std::string_view& with, bool recursive, size_t start)
		{
			VI_ASSERT(!begins.empty(), "begin should not be empty");
			VI_ASSERT(!ends.empty(), "end should not be empty");

			size_t begins_size = begins.size(), ends_size = ends.size();
			for (size_t i = start; i < other.size(); i++)
			{
				size_t from = i, begins_offset = 0;
				while (begins_offset < begins_size && from < other.size() && other.at(from) == begins[begins_offset])
				{
					++from;
					++begins_offset;
				}

				size_t nesting = 1;
				if (begins_offset != begins_size)
				{
					i = from;
					continue;
				}

				size_t to = from, ends_offset = 0;
				while (to < other.size())
				{
					if (other.at(to++) != ends[ends_offset])
					{
						if (!recursive)
							continue;

						size_t substep = to - 1, suboffset = 0;
						while (suboffset < begins_size && substep < other.size() && other.at(substep) == begins[suboffset])
						{
							++substep;
							++suboffset;
						}

						if (suboffset == begins_size)
							++nesting;
					}
					else if (++ends_offset >= ends_size)
					{
						if (!--nesting)
							break;

						ends_offset = 0;
					}
				}

				if (ends_offset != ends_size)
				{
					i = to;
					continue;
				}

				if (to > other.size())
					to = other.size();

				other.replace(other.begin() + from - begins_size, other.begin() + to, with);
				i = with.size();
			}
			return other;
		}
		string& stringify::replace_not_in_between(string& other, const std::string_view& begins, const std::string_view& ends, const std::string_view& with, bool recursive, size_t start)
		{
			VI_ASSERT(!begins.empty(), "begin should not be empty");
			VI_ASSERT(!ends.empty(), "end should not be empty");

			size_t begins_size = begins.size(), ends_size = ends.size();
			size_t replace_at = string::npos;

			for (size_t i = start; i < other.size(); i++)
			{
				size_t from = i, begins_offset = 0;
				while (begins_offset < begins_size && from < other.size() && other.at(from) == begins[begins_offset])
				{
					++from;
					++begins_offset;
				}

				size_t nesting = 1;
				if (begins_offset != begins_size)
				{
					if (replace_at == string::npos)
						replace_at = i;

					continue;
				}

				if (replace_at != string::npos)
				{
					other.replace(other.begin() + replace_at, other.begin() + i, with);
					from = replace_at + begins_size + with.size();
					i = from - begins_size;
					replace_at = string::npos;
				}

				size_t to = from, ends_offset = 0;
				while (to < other.size())
				{
					if (other.at(to++) != ends[ends_offset])
					{
						if (!recursive)
							continue;

						size_t substep = to - 1, suboffset = 0;
						while (suboffset < begins_size && substep < other.size() && other.at(substep) == begins[suboffset])
						{
							++substep;
							++suboffset;
						}

						if (suboffset == begins_size)
							++nesting;
					}
					else if (++ends_offset >= ends_size)
					{
						if (!--nesting)
							break;

						ends_offset = 0;
					}
				}

				i = to - 1;
			}

			if (replace_at != string::npos)
				other.replace(other.begin() + replace_at, other.end(), with);
			return other;
		}
		string& stringify::replace_parts(string& other, vector<std::pair<string, text_settle>>& inout, const std::string_view& with, const std::function<char(const std::string_view&, char, int)>& surrounding)
		{
			VI_SORT(inout.begin(), inout.end(), [](const std::pair<string, text_settle>& a, const std::pair<string, text_settle>& b)
			{
				return a.second.start < b.second.start;
			});

			int64_t offset = 0;
			for (auto& item : inout)
			{
				size_t size = item.second.end - item.second.start;
				if (!item.second.found || !size)
					continue;

				item.second.start = (size_t)((int64_t)item.second.start + offset);
				item.second.end = (size_t)((int64_t)item.second.end + offset);
				if (surrounding != nullptr)
				{
					string replacement = string(with);
					if (item.second.start > 0)
					{
						char next = surrounding(item.first, other.at(item.second.start - 1), -1);
						if (next != '\0')
							replacement.insert(replacement.begin(), next);
					}

					if (item.second.end < other.size())
					{
						char next = surrounding(item.first, other.at(item.second.end), 1);
						if (next != '\0')
							replacement.push_back(next);
					}

					replace_part(other, item.second.start, item.second.end, replacement);
					offset += (int64_t)replacement.size() - (int64_t)size;
					item.second.end = item.second.start + replacement.size();
				}
				else
				{
					replace_part(other, item.second.start, item.second.end, with);
					offset += (int64_t)with.size() - (int64_t)size;
					item.second.end = item.second.start + with.size();
				}
			}
			return other;
		}
		string& stringify::replace_parts(string& other, vector<text_settle>& inout, const std::string_view& with, const std::function<char(char, int)>& surrounding)
		{
			VI_SORT(inout.begin(), inout.end(), [](const text_settle& a, const text_settle& b)
			{
				return a.start < b.start;
			});

			int64_t offset = 0;
			for (auto& item : inout)
			{
				size_t size = item.end - item.start;
				if (!item.found || !size)
					continue;

				item.start = (size_t)((int64_t)item.start + offset);
				item.end = (size_t)((int64_t)item.end + offset);
				if (surrounding != nullptr)
				{
					string replacement = string(with);
					if (item.start > 0)
					{
						char next = surrounding(other.at(item.start - 1), -1);
						if (next != '\0')
							replacement.insert(replacement.begin(), next);
					}

					if (item.end < other.size())
					{
						char next = surrounding(other.at(item.end), 1);
						if (next != '\0')
							replacement.push_back(next);
					}

					replace_part(other, item.start, item.end, replacement);
					offset += (int64_t)replacement.size() - (int64_t)size;
					item.end = item.start + replacement.size();
				}
				else
				{
					replace_part(other, item.start, item.end, with);
					offset += (int64_t)with.size() - (int64_t)size;
					item.end = item.start + with.size();
				}
			}
			return other;
		}
		string& stringify::remove_part(string& other, size_t start, size_t end)
		{
			VI_ASSERT(start < other.size(), "invalid start");
			VI_ASSERT(end <= other.size(), "invalid end");
			VI_ASSERT(start < end, "start should be less than end");

			if (start == 0)
			{
				if (other.size() != end)
					other.assign(other.substr(end, other.size() - end));
				else
					other.clear();
			}
			else if (other.size() == end)
				other.assign(other.substr(0, start));
			else
				other.assign(other.substr(0, start) + other.substr(end, other.size() - end));
			return other;
		}
		string& stringify::reverse(string& other)
		{
			if (other.size() > 1)
				reverse(other, 0, other.size());
			return other;
		}
		string& stringify::reverse(string& other, size_t start, size_t end)
		{
			VI_ASSERT(!other.empty(), "length should be at least 1 char");
			VI_ASSERT(end <= other.size(), "end should be less than length");
			VI_ASSERT(start < other.size(), "start should be less than length - 1");
			VI_ASSERT(start < end, "start should be less than end");
			std::reverse(other.begin() + start, other.begin() + end);
			return other;
		}
		string& stringify::substring(string& other, const text_settle& result)
		{
			VI_ASSERT(result.found, "result should be found");
			if (result.start >= other.size())
			{
				other.clear();
				return other;
			}

			auto offset = (int64_t)result.end;
			if (result.end > other.size())
				offset = (int64_t)(other.size() - result.start);

			offset = (int64_t)result.start - offset;
			other.assign(other.substr(result.start, (size_t)(offset < 0 ? -offset : offset)));
			return other;
		}
		string& stringify::splice(string& other, size_t start, size_t end)
		{
			VI_ASSERT(start < other.size(), "result start should be less or equal than length - 1");
			if (end > other.size())
				end = (other.size() - start);

			int64_t offset = (int64_t)start - (int64_t)end;
			other.assign(other.substr(start, (size_t)(offset < 0 ? -offset : offset)));
			return other;
		}
		string& stringify::trim(string& other)
		{
			trim_start(other);
			trim_end(other);
			return other;
		}
		string& stringify::trim_start(string& other)
		{
			other.erase(other.begin(), std::find_if(other.begin(), other.end(), [](uint8_t v) -> bool { return std::isspace(v) == 0; }));
			return other;
		}
		string& stringify::trim_end(string& other)
		{
			other.erase(std::find_if(other.rbegin(), other.rend(), [](uint8_t v) -> bool { return std::isspace(v) == 0; }).base(), other.end());
			return other;
		}
		string& stringify::fill(string& other, char symbol)
		{
			for (char& i : other)
				i = symbol;
			return other;
		}
		string& stringify::fill(string& other, char symbol, size_t count)
		{
			other.assign(count, symbol);
			return other;
		}
		string& stringify::fill(string& other, char symbol, size_t start, size_t count)
		{
			VI_ASSERT(!other.empty(), "length should be greater than Zero");
			VI_ASSERT(start <= other.size(), "start should be less or equal than length");
			if (start + count > other.size())
				count = other.size() - start;

			size_t size = (start + count);
			for (size_t i = start; i < size; i++)
				other.at(i) = symbol;
			return other;
		}
		string& stringify::append(string& other, const char* format, ...)
		{
			VI_ASSERT(format != nullptr, "format should be set");
			char buffer[BLOB_SIZE];
			va_list args;
			va_start(args, format);
			int count = vsnprintf(buffer, sizeof(buffer), format, args);
			va_end(args);

			if (count > sizeof(buffer))
				count = sizeof(buffer);

			other.append(buffer, count);
			return other;
		}
		string& stringify::erase(string& other, size_t position)
		{
			VI_ASSERT(position < other.size(), "position should be less than length");
			other.erase(position);
			return other;
		}
		string& stringify::erase(string& other, size_t position, size_t count)
		{
			VI_ASSERT(position < other.size(), "position should be less than length");
			other.erase(position, count);
			return other;
		}
		string& stringify::erase_offsets(string& other, size_t start, size_t end)
		{
			return erase(other, start, end - start);
		}
		expects_system<void> stringify::eval_envs(string& other, const std::string_view& directory, const vector<string>& tokens, const std::string_view& token)
		{
			if (other.empty())
				return core::expectation::met;

			if (starts_of(other, "./\\"))
			{
				expects_io<string> result = os::path::resolve(other.c_str(), directory, true);
				if (result)
					other.assign(*result);
			}
			else if (other.front() == '$' && other.size() > 1)
			{
				auto env = os::process::get_env(other.c_str() + 1);
				if (!env)
					return system_exception("invalid env name: " + other.substr(1), std::move(env.error()));
				other.assign(*env);
			}
			else if (!tokens.empty())
			{
				size_t start = std::string::npos, offset = 0;
				while ((start = other.find(token, offset)) != std::string::npos)
				{
					size_t subset = start + token.size(); size_t end = subset;
					while (end < other.size() && is_numeric(other[end]))
						++end;

					auto index = from_string<uint8_t>(std::string_view(other.data() + subset, end - subset));
					if (index && *index < tokens.size())
					{
						auto& target = tokens[*index];
						other.replace(other.begin() + start, other.begin() + end, target);
						offset = start + target.size();
					}
					else
						offset = end;
				}
			}

			return core::expectation::met;
		}
		vector<std::pair<string, text_settle>> stringify::find_in_between(const std::string_view& other, const std::string_view& begins, const std::string_view& ends, const std::string_view& not_in_sub_between_of, size_t offset)
		{
			vector<std::pair<string, text_settle>> result;
			pm_find_in_between(result, other, begins, ends, not_in_sub_between_of, offset);
			return result;
		}
		vector<std::pair<string, text_settle>> stringify::find_in_between_in_code(const std::string_view& other, const std::string_view& begins, const std::string_view& ends, size_t offset)
		{
			vector<std::pair<string, text_settle>> result;
			pm_find_in_between_in_code(result, other, begins, ends, offset);
			return result;
		}
		vector<std::pair<string, text_settle>> stringify::find_starts_with_ends_of(const std::string_view& other, const std::string_view& begins, const std::string_view& ends_of, const std::string_view& not_in_sub_between_of, size_t offset)
		{
			vector<std::pair<string, text_settle>> result;
			pm_find_starts_with_ends_of(result, other, begins, ends_of, not_in_sub_between_of, offset);
			return result;
		}
		void stringify::pm_find_in_between(vector<std::pair<string, text_settle>>& result, const std::string_view& other, const std::string_view& begins, const std::string_view& ends, const std::string_view& not_in_sub_between_of, size_t offset)
		{
			VI_ASSERT(!begins.empty(), "begin should not be empty");
			VI_ASSERT(!ends.empty(), "end should not be empty");

			size_t begins_size = begins.size(), ends_size = ends.size();
			size_t nisbo_size = not_in_sub_between_of.size();
			char skip = '\0';

			for (size_t i = offset; i < other.size(); i++)
			{
				for (size_t j = 0; j < nisbo_size; j++)
				{
					char next = other.at(i);
					if (next == not_in_sub_between_of[j])
					{
						skip = next;
						++i;
						break;
					}
				}

				while (skip != '\0' && i < other.size() && other.at(i) != skip)
					++i;

				if (skip != '\0')
				{
					skip = '\0';
					if (i >= other.size())
						break;
				}

				size_t from = i, begins_offset = 0;
				while (begins_offset < begins_size && from < other.size() && other.at(from) == begins[begins_offset])
				{
					++from;
					++begins_offset;
				}

				if (begins_offset != begins_size)
				{
					i = from;
					continue;
				}

				size_t to = from, ends_offset = 0;
				while (to < other.size())
				{
					if (other.at(to++) != ends[ends_offset])
						continue;

					if (++ends_offset >= ends_size)
						break;
				}

				i = to;
				if (ends_offset != ends_size)
					continue;

				text_settle at;
				at.start = from - begins_size;
				at.end = to;
				at.found = true;

				result.push_back(std::make_pair(string(other.substr(from, (to - ends_size) - from)), std::move(at)));
			}
		}
		void stringify::pm_find_in_between_in_code(vector<std::pair<string, text_settle>>& result, const std::string_view& other, const std::string_view& begins, const std::string_view& ends, size_t offset)
		{
			VI_ASSERT(!begins.empty(), "begin should not be empty");
			VI_ASSERT(!ends.empty(), "end should not be empty");

			size_t begins_size = begins.size(), ends_size = ends.size();
			for (size_t i = offset; i < other.size(); i++)
			{
				if (other.at(i) == '/' && i + 1 < other.size() && (other[i + 1] == '/' || other[i + 1] == '*'))
				{
					if (other[++i] == '*')
					{
						while (i + 1 < other.size())
						{
							char n = other[i++];
							if (n == '*' && other[i++] == '/')
								break;
						}
					}
					else
					{
						while (i < other.size())
						{
							char n = other[i++];
							if (n == '\r' || n == '\n')
								break;
						}
					}

					continue;
				}

				size_t from = i, begins_offset = 0;
				while (begins_offset < begins_size && from < other.size() && other.at(from) == begins[begins_offset])
				{
					++from;
					++begins_offset;
				}

				if (begins_offset != begins_size)
				{
					i = from;
					continue;
				}

				size_t to = from, ends_offset = 0;
				while (to < other.size())
				{
					if (other.at(to++) != ends[ends_offset])
						continue;

					if (++ends_offset >= ends_size)
						break;
				}

				i = to;
				if (ends_offset != ends_size)
					continue;

				text_settle at;
				at.start = from - begins_size;
				at.end = to;
				at.found = true;

				result.push_back(std::make_pair(string(other.substr(from, (to - ends_size) - from)), std::move(at)));
			}
		}
		void stringify::pm_find_starts_with_ends_of(vector<std::pair<string, text_settle>>& result, const std::string_view& other, const std::string_view& begins, const std::string_view& ends_of, const std::string_view& not_in_sub_between_of, size_t offset)
		{
			VI_ASSERT(!begins.empty(), "begin should not be empty");
			VI_ASSERT(!ends_of.empty(), "end should not be empty");

			size_t begins_size = begins.size(), ends_of_size = ends_of.size();
			size_t nisbo_size = not_in_sub_between_of.size();
			char skip = '\0';

			for (size_t i = offset; i < other.size(); i++)
			{
				for (size_t j = 0; j < nisbo_size; j++)
				{
					char next = other.at(i);
					if (next == not_in_sub_between_of[j])
					{
						skip = next;
						++i;
						break;
					}
				}

				while (skip != '\0' && i < other.size() && other.at(i) != skip)
					++i;

				if (skip != '\0')
				{
					skip = '\0';
					if (i >= other.size())
						break;
				}

				size_t from = i, begins_offset = 0;
				while (begins_offset < begins_size && from < other.size() && other.at(from) == begins[begins_offset])
				{
					++from;
					++begins_offset;
				}

				bool matching = false;
				if (begins_offset != begins_size)
				{
					i = from;
					continue;
				}

				size_t to = from;
				while (!matching && to < other.size())
				{
					auto& next = other.at(to++);
					for (size_t j = 0; j < ends_of_size; j++)
					{
						if (next == ends_of[j])
						{
							matching = true;
							--to;
							break;
						}
					}
				}

				if (to >= other.size())
					matching = true;

				if (!matching)
					continue;

				text_settle at;
				at.start = from - begins_size;
				at.end = to;
				at.found = true;

				result.push_back(std::make_pair(string(other.substr(from, to - from)), std::move(at)));
			}
		}
		text_settle stringify::reverse_find(const std::string_view& other, const std::string_view& needle, size_t offset)
		{
			if (other.empty() || offset >= other.size())
				return { other.size(), other.size(), false };

			const char* ptr = other.data() - offset;
			if (needle.data() > ptr)
				return { other.size(), other.size(), false };

			const char* it = nullptr;
			for (it = ptr + other.size() - needle.size(); it > ptr; --it)
			{
				if (strncmp(ptr, needle.data(), needle.size()) == 0)
				{
					size_t set = (size_t)(it - ptr);
					return { set, set + needle.size(), true };
				}
			}

			return { other.size(), other.size(), false };
		}
		text_settle stringify::reverse_find(const std::string_view& other, char needle, size_t offset)
		{
			if (other.empty() || offset >= other.size())
				return { other.size(), other.size(), false };

			size_t size = other.size() - offset;
			for (size_t i = size; i-- > 0;)
			{
				if (other.at(i) == needle)
					return { i, i + 1, true };
			}

			return { other.size(), other.size(), false };
		}
		text_settle stringify::reverse_find_unescaped(const std::string_view& other, char needle, size_t offset)
		{
			if (other.empty() || offset >= other.size())
				return { other.size(), other.size(), false };

			size_t size = other.size() - offset;
			for (size_t i = size; i-- > 0;)
			{
				if (other.at(i) == needle && ((int64_t)i - 1 < 0 || other.at(i - 1) != '\\'))
					return { i, i + 1, true };
			}

			return { other.size(), other.size(), false };
		}
		text_settle stringify::reverse_find_of(const std::string_view& other, const std::string_view& needle, size_t offset)
		{
			if (other.empty() || offset >= other.size())
				return { other.size(), other.size(), false };

			size_t size = other.size() - offset;
			for (size_t i = size; i-- > 0;)
			{
				for (char k : needle)
				{
					if (other.at(i) == k)
						return { i, i + 1, true };
				}
			}

			return { other.size(), other.size(), false };
		}
		text_settle stringify::find(const std::string_view& other, const std::string_view& needle, size_t offset)
		{
			if (other.empty() || offset >= other.size())
				return { other.size(), other.size(), false };

			const char* it = strstr(other.data() + offset, needle.data());
			if (it == nullptr)
				return { other.size(), other.size(), false };

			size_t set = (size_t)(it - other.data());
			return { set, set + needle.size(), true };
		}
		text_settle stringify::find(const std::string_view& other, char needle, size_t offset)
		{
			for (size_t i = offset; i < other.size(); i++)
			{
				if (other.at(i) == needle)
					return { i, i + 1, true };
			}

			return { other.size(), other.size(), false };
		}
		text_settle stringify::find_unescaped(const std::string_view& other, char needle, size_t offset)
		{
			for (size_t i = offset; i < other.size(); i++)
			{
				if (other.at(i) == needle && ((int64_t)i - 1 < 0 || other.at(i - 1) != '\\'))
					return { i, i + 1, true };
			}

			return { other.size(), other.size(), false };
		}
		text_settle stringify::find_of(const std::string_view& other, const std::string_view& needle, size_t offset)
		{
			for (size_t i = offset; i < other.size(); i++)
			{
				for (char k : needle)
				{
					if (other.at(i) == k)
						return { i, i + 1, true };
				}
			}

			return { other.size(), other.size(), false };
		}
		text_settle stringify::find_not_of(const std::string_view& other, const std::string_view& needle, size_t offset)
		{
			for (size_t i = offset; i < other.size(); i++)
			{
				bool result = false;
				for (char k : needle)
				{
					if (other.at(i) == k)
					{
						result = true;
						break;
					}
				}

				if (!result)
					return { i, i + 1, true };
			}

			return { other.size(), other.size(), false };
		}
		size_t stringify::count_lines(const std::string_view& other)
		{
			size_t lines = 1;
			for (char item : other)
			{
				if (item == '\n')
					++lines;
			}
			return lines;
		}
		bool stringify::is_cstring(const std::string_view& other)
		{
			auto* data = other.data();
			return data != nullptr && data[other.size()] == '\0';
		}
		bool stringify::is_not_preceded_by_escape(const std::string_view& buffer, size_t offset, char escape)
		{
			VI_ASSERT(!buffer.empty(), "buffer should be set");
			if (offset < 1 || buffer[offset - 1] != escape)
				return true;

			return offset > 1 && buffer[offset - 2] == escape;
		}
		bool stringify::is_empty_or_whitespace(const std::string_view& other)
		{
			if (other.empty())
				return true;

			for (char next : other)
			{
				if (!is_whitespace(next))
					return false;
			}

			return true;
		}
		bool stringify::is_preceded_by(const std::string_view& other, size_t at, const std::string_view& of)
		{
			VI_ASSERT(!of.empty(), "tokenbase should be set");
			if (!at || at - 1 >= other.size())
				return false;

			size_t size = of.size();
			char next = other.at(at - 1);
			for (size_t i = 0; i < size; i++)
			{
				if (next == of[i])
					return true;
			}

			return false;
		}
		bool stringify::is_followed_by(const std::string_view& other, size_t at, const std::string_view& of)
		{
			VI_ASSERT(!of.empty(), "tokenbase should be set");
			if (at + 1 >= other.size())
				return false;

			size_t size = of.size();
			char next = other.at(at + 1);
			for (size_t i = 0; i < size; i++)
			{
				if (next == of[i])
					return true;
			}

			return false;
		}
		bool stringify::starts_with(const std::string_view& other, const std::string_view& value, size_t offset)
		{
			if (other.size() < value.size())
				return false;

			for (size_t i = offset; i < value.size(); i++)
			{
				if (value[i] != other.at(i))
					return false;
			}

			return true;
		}
		bool stringify::starts_of(const std::string_view& other, const std::string_view& value, size_t offset)
		{
			VI_ASSERT(!value.empty(), "value should be set");
			size_t length = value.size();
			if (offset >= other.size())
				return false;

			for (size_t j = 0; j < length; j++)
			{
				if (other.at(offset) == value[j])
					return true;
			}

			return false;
		}
		bool stringify::starts_not_of(const std::string_view& other, const std::string_view& value, size_t offset)
		{
			VI_ASSERT(!value.empty(), "value should be set");
			size_t length = value.size();
			if (offset >= other.size())
				return false;

			bool result = true;
			for (size_t j = 0; j < length; j++)
			{
				if (other.at(offset) == value[j])
				{
					result = false;
					break;
				}
			}

			return result;
		}
		bool stringify::ends_with(const std::string_view& other, const std::string_view& value)
		{
			if (other.empty() || value.size() > other.size())
				return false;

			return strcmp(other.data() + other.size() - value.size(), value.data()) == 0;
		}
		bool stringify::ends_with(const std::string_view& other, char value)
		{
			return !other.empty() && other.back() == value;
		}
		bool stringify::ends_of(const std::string_view& other, const std::string_view& value)
		{
			VI_ASSERT(!value.empty(), "value should be set");
			if (other.empty())
				return false;

			size_t length = value.size();
			for (size_t j = 0; j < length; j++)
			{
				if (other.back() == value[j])
					return true;
			}

			return false;
		}
		bool stringify::ends_not_of(const std::string_view& other, const std::string_view& value)
		{
			VI_ASSERT(!value.empty(), "value should be set");
			if (other.empty())
				return true;

			size_t length = value.size();
			for (size_t j = 0; j < length; j++)
			{
				if (other.back() == value[j])
					return false;
			}

			return true;
		}
		bool stringify::consists_of(const std::string_view& other, const std::string_view& alphabet)
		{
			size_t offset = 0;
			while (offset < other.size())
			{
				if (alphabet.find(other[offset++]) == std::string::npos)
					return false;
			}
			return true;
		}
		bool stringify::has_integer(const std::string_view& other)
		{
			if (other.empty() || (other.size() == 1 && !is_numeric(other.front())))
				return false;

			size_t digits = 0;
			size_t i = (other.front() == '+' || other.front() == '-' ? 1 : 0);
			for (; i < other.size(); i++)
			{
				char v = other[i];
				if (!is_numeric(v))
					return false;

				++digits;
			}

			return digits > 0;
		}
		bool stringify::has_number(const std::string_view& other)
		{
			if (other.empty() || (other.size() == 1 && !is_numeric(other.front())))
				return false;

			size_t digits = 0, points = 0;
			size_t i = (other.front() == '+' || other.front() == '-' ? 1 : 0);
			for (; i < other.size(); i++)
			{
				char v = other[i];
				if (is_numeric(v))
				{
					++digits;
					continue;
				}

				if (!points && v == '.' && i + 1 < other.size())
				{
					++points;
					continue;
				}

				return false;
			}

			return digits > 0 && points < 2;
		}
		bool stringify::has_decimal(const std::string_view& other)
		{
			auto f = find(other, '.');
			if (!f.found)
				return has_integer(other) && other.size() >= 19;

			auto D1 = other.substr(0, f.end - 1);
			if (D1.empty() || !has_integer(D1))
				return false;

			auto D2 = other.substr(f.end + 1, other.size() - f.end - 1);
			if (D2.empty() || !has_integer(D2))
				return false;

			return D1.size() >= 19 || D2.size() > 2;
		}
		bool stringify::is_numeric_or_dot(char symbol)
		{
			return symbol == '.' || is_numeric(symbol);
		}
		bool stringify::is_numeric_or_dot_or_whitespace(char symbol)
		{
			return is_whitespace(symbol) || is_numeric_or_dot(symbol);
		}
		bool stringify::is_hex(char symbol)
		{
			return is_numeric(symbol) || (symbol >= 'a' && symbol <= 'f');
		}
		bool stringify::is_hex_or_dot(char symbol)
		{
			return symbol == '.' || is_hex(symbol);
		}
		bool stringify::is_hex_or_dot_or_whitespace(char symbol)
		{
			return is_whitespace(symbol) || is_hex_or_dot(symbol);
		}
		bool stringify::is_alphabetic(char symbol)
		{
			return std::isalpha(static_cast<uint8_t>(symbol)) != 0;
		}
		bool stringify::is_numeric(char symbol)
		{
			return std::isdigit(static_cast<uint8_t>(symbol)) != 0;
		}
		bool stringify::is_alphanum(char symbol)
		{
			return std::isalnum(static_cast<uint8_t>(symbol)) != 0;
		}
		bool stringify::is_whitespace(char symbol)
		{
			return std::isspace(static_cast<uint8_t>(symbol)) != 0;
		}
		char stringify::to_lower_literal(char symbol)
		{
			return static_cast<char>(std::tolower(static_cast<uint8_t>(symbol)));
		}
		char stringify::to_upper_literal(char symbol)
		{
			return static_cast<char>(std::toupper(static_cast<uint8_t>(symbol)));
		}
		bool stringify::case_equals(const std::string_view& value1, const std::string_view& value2)
		{
			return value1.size() == value2.size() && std::equal(value1.begin(), value1.end(), value2.begin(), [](const uint8_t a, const uint8_t b)
			{
				return std::tolower(a) == std::tolower(b);
			});
		}
		int stringify::case_compare(const std::string_view& value1, const std::string_view& value2)
		{
			int diff = 0;
			size_t size = std::min(value1.size(), value2.size());
			size_t offset = 0;
			do
			{
				diff = tolower((uint8_t)value1[offset]) - tolower((uint8_t)value2[offset]);
				++offset;
			} while (diff == 0 && offset < size);
			return diff;
		}
		int stringify::match(const char* pattern, const std::string_view& text)
		{
			VI_ASSERT(pattern != nullptr, "pattern and text should be set");
			return match(pattern, strlen(pattern), text);
		}
		int stringify::match(const char* pattern, size_t length, const std::string_view& text)
		{
			VI_ASSERT(pattern != nullptr, "pattern and text should be set");
			const char* token = (const char*)memchr(pattern, '|', (size_t)length);
			if (token != nullptr)
			{
				int output = match(pattern, (size_t)(token - pattern), text);
				return (output > 0) ? output : match(token + 1, (size_t)((pattern + length) - (token + 1)), text);
			}

			int offset = 0, result = 0;
			size_t i = 0; int j = 0;
			while (i < length)
			{
				if (pattern[i] == '?' && text[j] != '\0')
					continue;

				if (pattern[i] == '$')
					return (text[j] == '\0') ? j : -1;

				if (pattern[i] == '*')
				{
					i++;
					if (pattern[i] == '*')
					{
						offset = (int)text.substr(j).size();
						i++;
					}
					else
						offset = (int)strcspn(text.data() + j, "/");

					if (i == length)
						return j + offset;

					do
					{
						result = match(pattern + i, length - i, text.substr(j + offset));
					} while (result == -1 && offset-- > 0);

					return (result == -1) ? -1 : j + result + offset;
				}
				else if (tolower((const uint8_t)pattern[i]) != tolower((const uint8_t)text[j]))
					return -1;

				i++;
				j++;
			}

			return j;
		}
		string stringify::text(const char* format, ...)
		{
			VI_ASSERT(format != nullptr, "format should be set");
			va_list args;
			va_start(args, format);
			char buffer[BLOB_SIZE];
			int size = vsnprintf(buffer, sizeof(buffer), format, args);
			if (size > sizeof(buffer))
				size = sizeof(buffer);
			va_end(args);
			return string(buffer, (size_t)size);
		}
		wide_string stringify::to_wide(const std::string_view& other)
		{
			wide_string output;
			output.reserve(other.size());

			wchar_t w;
			for (size_t i = 0; i < other.size();)
			{
				char c = other.at(i);
				if ((c & 0x80) == 0)
				{
					w = c;
					i++;
				}
				else if ((c & 0xE0) == 0xC0)
				{
					w = (c & 0x1F) << 6;
					w |= (other.at(i + 1) & 0x3F);
					i += 2;
				}
				else if ((c & 0xF0) == 0xE0)
				{
					w = (c & 0xF) << 12;
					w |= (other.at(i + 1) & 0x3F) << 6;
					w |= (other.at(i + 2) & 0x3F);
					i += 3;
				}
				else if ((c & 0xF8) == 0xF0)
				{
					w = (c & 0x7) << 18;
					w |= (other.at(i + 1) & 0x3F) << 12;
					w |= (other.at(i + 2) & 0x3F) << 6;
					w |= (other.at(i + 3) & 0x3F);
					i += 4;
				}
				else if ((c & 0xFC) == 0xF8)
				{
					w = (c & 0x3) << 24;
					w |= (c & 0x3F) << 18;
					w |= (c & 0x3F) << 12;
					w |= (c & 0x3F) << 6;
					w |= (c & 0x3F);
					i += 5;
				}
				else if ((c & 0xFE) == 0xFC)
				{
					w = (c & 0x1) << 30;
					w |= (c & 0x3F) << 24;
					w |= (c & 0x3F) << 18;
					w |= (c & 0x3F) << 12;
					w |= (c & 0x3F) << 6;
					w |= (c & 0x3F);
					i += 6;
				}
				else
					w = c;

				output += w;
			}

			return output;
		}
		vector<string> stringify::split(const std::string_view& other, const std::string_view& with, size_t start)
		{
			vector<string> output_view;
			pm_split(output_view, other, with, start);
			return output_view;
		}
		vector<string> stringify::split(const std::string_view& other, char with, size_t start)
		{
			vector<string> output_view;
			pm_split(output_view, other, with, start);
			return output_view;
		}
		vector<string> stringify::split_max(const std::string_view& other, char with, size_t count, size_t start)
		{
			vector<string> output_view;
			pm_split_max(output_view, other, with, count, start);
			return output_view;
		}
		vector<string> stringify::split_of(const std::string_view& other, const std::string_view& with, size_t start)
		{
			vector<string> output_view;
			pm_split_of(output_view, other, with, start);
			return output_view;
		}
		vector<string> stringify::split_not_of(const std::string_view& other, const std::string_view& with, size_t start)
		{
			vector<string> output_view;
			pm_split_not_of(output_view, other, with, start);
			return output_view;
		}
		void stringify::pm_split(vector<string>& output, const std::string_view& other, const std::string_view& with, size_t start)
		{
			if (start >= other.size())
				return;

			size_t offset = start;
			text_settle result = find(other, with, offset);
			while (result.found)
			{
				output.emplace_back(other.substr(offset, result.start - offset));
				result = find(other, with, offset = result.end);
			}

			if (offset < other.size())
				output.emplace_back(other.substr(offset));
		}
		void stringify::pm_split(vector<string>& output, const std::string_view& other, char with, size_t start)
		{
			if (start >= other.size())
				return;

			size_t offset = start;
			text_settle result = find(other, with, start);
			while (result.found)
			{
				output.emplace_back(other.substr(offset, result.start - offset));
				result = find(other, with, offset = result.end);
			}

			if (offset < other.size())
				output.emplace_back(other.substr(offset));
		}
		void stringify::pm_split_max(vector<string>& output, const std::string_view& other, char with, size_t count, size_t start)
		{
			if (start >= other.size())
				return;

			size_t offset = start;
			text_settle result = find(other, with, start);
			while (result.found && output.size() < count)
			{
				output.emplace_back(other.substr(offset, result.start - offset));
				result = find(other, with, offset = result.end);
			}

			if (offset < other.size() && output.size() < count)
				output.emplace_back(other.substr(offset));
		}
		void stringify::pm_split_of(vector<string>& output, const std::string_view& other, const std::string_view& with, size_t start)
		{
			if (start >= other.size())
				return;

			size_t offset = start;
			text_settle result = find_of(other, with, start);
			while (result.found)
			{
				output.emplace_back(other.substr(offset, result.start - offset));
				result = find_of(other, with, offset = result.end);
			}

			if (offset < other.size())
				output.emplace_back(other.substr(offset));
		}
		void stringify::pm_split_not_of(vector<string>& output, const std::string_view& other, const std::string_view& with, size_t start)
		{
			if (start >= other.size())
				return;

			size_t offset = start;
			text_settle result = find_not_of(other, with, start);
			while (result.found)
			{
				output.emplace_back(other.substr(offset, result.start - offset));
				result = find_not_of(other, with, offset = result.end);
			}

			if (offset < other.size())
				output.emplace_back(other.substr(offset));
		}
		void stringify::convert_to_wide(const std::string_view& input, wchar_t* output, size_t output_size)
		{
			VI_ASSERT(output != nullptr && output_size > 0, "output should be set");

			wchar_t w = '\0'; size_t size = 0;
			for (size_t i = 0; i < input.size();)
			{
				char c = input[i];
				if ((c & 0x80) == 0)
				{
					w = c;
					i++;
				}
				else if ((c & 0xE0) == 0xC0)
				{
					w = (c & 0x1F) << 6;
					w |= (input[i + 1] & 0x3F);
					i += 2;
				}
				else if ((c & 0xF0) == 0xE0)
				{
					w = (c & 0xF) << 12;
					w |= (input[i + 1] & 0x3F) << 6;
					w |= (input[i + 2] & 0x3F);
					i += 3;
				}
				else if ((c & 0xF8) == 0xF0)
				{
					w = (c & 0x7) << 18;
					w |= (input[i + 1] & 0x3F) << 12;
					w |= (input[i + 2] & 0x3F) << 6;
					w |= (input[i + 3] & 0x3F);
					i += 4;
				}
				else if ((c & 0xFC) == 0xF8)
				{
					w = (c & 0x3) << 24;
					w |= (c & 0x3F) << 18;
					w |= (c & 0x3F) << 12;
					w |= (c & 0x3F) << 6;
					w |= (c & 0x3F);
					i += 5;
				}
				else if ((c & 0xFE) == 0xFC)
				{
					w = (c & 0x1) << 30;
					w |= (c & 0x3F) << 24;
					w |= (c & 0x3F) << 18;
					w |= (c & 0x3F) << 12;
					w |= (c & 0x3F) << 6;
					w |= (c & 0x3F);
					i += 6;
				}
				else
					w = c;

				output[size++] = w;
				if (size >= output_size)
					break;
			}

			if (size < output_size)
				output[size] = '\0';
		}

		schema* var::set::any(variant&& value)
		{
			return new schema(std::move(value));
		}
		schema* var::set::any(const variant& value)
		{
			return new schema(value);
		}
		schema* var::set::any(const std::string_view& value, bool strict)
		{
			return new schema(var::any(value, strict));
		}
		schema* var::set::null()
		{
			return new schema(var::null());
		}
		schema* var::set::undefined()
		{
			return new schema(var::undefined());
		}
		schema* var::set::object()
		{
			return new schema(var::object());
		}
		schema* var::set::array()
		{
			return new schema(var::array());
		}
		schema* var::set::pointer(void* value)
		{
			return new schema(var::pointer(value));
		}
		schema* var::set::string(const std::string_view& value)
		{
			return new schema(var::string(value));
		}
		schema* var::set::binary(const std::string_view& value)
		{
			return binary((uint8_t*)value.data(), value.size());
		}
		schema* var::set::binary(const uint8_t* value, size_t size)
		{
			return new schema(var::binary(value, size));
		}
		schema* var::set::integer(int64_t value)
		{
			return new schema(var::integer(value));
		}
		schema* var::set::number(double value)
		{
			return new schema(var::number(value));
		}
		schema* var::set::decimal(const core::decimal& value)
		{
			return new schema(var::decimal(value));
		}
		schema* var::set::decimal(core::decimal&& value)
		{
			return new schema(var::decimal(std::move(value)));
		}
		schema* var::set::decimal_string(const std::string_view& value)
		{
			return new schema(var::decimal_string(value));
		}
		schema* var::set::boolean(bool value)
		{
			return new schema(var::boolean(value));
		}

		variant var::any(const std::string_view& value, bool strict)
		{
			variant result;
			result.deserialize(value, strict);
			return result;
		}
		variant var::null()
		{
			return variant(var_type::null);
		}
		variant var::undefined()
		{
			return variant(var_type::undefined);
		}
		variant var::object()
		{
			return variant(var_type::object);
		}
		variant var::array()
		{
			return variant(var_type::array);
		}
		variant var::pointer(void* value)
		{
			if (!value)
				return null();

			variant result(var_type::pointer);
			result.value.pointer = (char*)value;
			return result;
		}
		variant var::string(const std::string_view& value)
		{
			variant result(var_type::string);
			result.length = (uint32_t)value.size();

			size_t string_size = sizeof(char) * (result.length + 1);
			if (result.length > variant::get_max_small_string_size())
				result.value.pointer = memory::allocate<char>(string_size);

			char* data = (char*)result.get_string().data();
			memcpy(data, value.data(), string_size - sizeof(char));
			data[string_size - 1] = '\0';

			return result;
		}
		variant var::binary(const std::string_view& value)
		{
			return binary((uint8_t*)value.data(), value.size());
		}
		variant var::binary(const uint8_t* value, size_t size)
		{
			VI_ASSERT(value != nullptr, "value should be set");
			variant result(var_type::binary);
			result.length = (uint32_t)size;

			size_t string_size = sizeof(char) * (result.length + 1);
			if (result.length > variant::get_max_small_string_size())
				result.value.pointer = memory::allocate<char>(string_size);

			char* data = (char*)result.get_string().data();
			memcpy(data, value, string_size - sizeof(char));
			data[string_size - 1] = '\0';

			return result;
		}
		variant var::integer(int64_t value)
		{
			variant result(var_type::integer);
			result.value.integer = value;
			return result;
		}
		variant var::number(double value)
		{
			variant result(var_type::number);
			result.value.number = value;
			return result;
		}
		variant var::decimal(const core::decimal& value)
		{
			core::decimal* buffer = memory::init<core::decimal>(value);
			variant result(var_type::decimal);
			result.value.pointer = (char*)buffer;
			return result;
		}
		variant var::decimal(core::decimal&& value)
		{
			core::decimal* buffer = memory::init<core::decimal>(std::move(value));
			variant result(var_type::decimal);
			result.value.pointer = (char*)buffer;
			return result;
		}
		variant var::decimal_string(const std::string_view& value)
		{
			core::decimal* buffer = memory::init<core::decimal>(value);
			variant result(var_type::decimal);
			result.value.pointer = (char*)buffer;
			return result;
		}
		variant var::boolean(bool value)
		{
			variant result(var_type::boolean);
			result.value.boolean = value;
			return result;
		}

		unordered_set<uint64_t> composer::fetch(uint64_t id) noexcept
		{
			VI_ASSERT(context != nullptr, "composer should be initialized");
			unordered_set<uint64_t> hashes;
			for (auto& item : context->factory)
			{
				if (item.second.first == id)
					hashes.insert(item.first);
			}

			return hashes;
		}
		bool composer::pop(const std::string_view& hash) noexcept
		{
			VI_ASSERT(context != nullptr, "composer should be initialized");
			VI_TRACE("[composer] pop %.*s", (int)hash.size(), hash.data());

			auto it = context->factory.find(VI_HASH(hash));
			if (it == context->factory.end())
				return false;

			context->factory.erase(it);
			return true;
		}
		void composer::cleanup() noexcept
		{
			memory::deinit(context);
			context = nullptr;
		}
		void composer::push(uint64_t type_id, uint64_t tag, void* callback) noexcept
		{
			VI_TRACE("[composer] push type %" PRIu64 " tagged as %" PRIu64, type_id, tag);
			if (!context)
				context = memory::init<state>();

			if (context->factory.find(type_id) == context->factory.end())
				context->factory[type_id] = std::make_pair(tag, callback);
		}
		void* composer::find(uint64_t type_id) noexcept
		{
			VI_ASSERT(context != nullptr, "composer should be initialized");
			auto it = context->factory.find(type_id);
			if (it != context->factory.end())
				return it->second.second;

			return nullptr;
		}
		composer::state* composer::context = nullptr;

		console::console() noexcept
		{
			colorization.reserve(146);

			/* scheduling */
			colorization.push_back({ "spawn", std_color::yellow, std_color::zero });
			colorization.push_back({ "despawn", std_color::yellow, std_color::zero });
			colorization.push_back({ "start", std_color::yellow, std_color::zero });
			colorization.push_back({ "stop", std_color::yellow, std_color::zero });
			colorization.push_back({ "resume", std_color::yellow, std_color::zero });
			colorization.push_back({ "suspend", std_color::yellow, std_color::zero });
			colorization.push_back({ "acquire", std_color::yellow, std_color::zero });
			colorization.push_back({ "release", std_color::yellow, std_color::zero });
			colorization.push_back({ "execute", std_color::yellow, std_color::zero });
			colorization.push_back({ "join", std_color::yellow, std_color::zero });
			colorization.push_back({ "terminate", std_color::dark_red, std_color::zero });
			colorization.push_back({ "abort", std_color::dark_red, std_color::zero });
			colorization.push_back({ "exit", std_color::dark_red, std_color::zero });
			colorization.push_back({ "thread", std_color::cyan, std_color::zero });
			colorization.push_back({ "process", std_color::cyan, std_color::zero });
			colorization.push_back({ "sync", std_color::cyan, std_color::zero });
			colorization.push_back({ "async", std_color::cyan, std_color::zero });
			colorization.push_back({ "ms", std_color::cyan, std_color::zero });
			colorization.push_back({ "us", std_color::cyan, std_color::zero });
			colorization.push_back({ "ns", std_color::cyan, std_color::zero });

			/* networking and IO */
			colorization.push_back({ "open", std_color::yellow, std_color::zero });
			colorization.push_back({ "close", std_color::yellow, std_color::zero });
			colorization.push_back({ "closed", std_color::yellow, std_color::zero });
			colorization.push_back({ "shutdown", std_color::yellow, std_color::zero });
			colorization.push_back({ "bind", std_color::yellow, std_color::zero });
			colorization.push_back({ "assign", std_color::yellow, std_color::zero });
			colorization.push_back({ "resolve", std_color::yellow, std_color::zero });
			colorization.push_back({ "listen", std_color::yellow, std_color::zero });
			colorization.push_back({ "unlisten", std_color::yellow, std_color::zero });
			colorization.push_back({ "accept", std_color::yellow, std_color::zero });
			colorization.push_back({ "connect", std_color::yellow, std_color::zero });
			colorization.push_back({ "reconnect", std_color::yellow, std_color::zero });
			colorization.push_back({ "handshake", std_color::yellow, std_color::zero });
			colorization.push_back({ "reset", std_color::yellow, std_color::zero });
			colorization.push_back({ "read", std_color::yellow, std_color::zero });
			colorization.push_back({ "write", std_color::yellow, std_color::zero });
			colorization.push_back({ "seek", std_color::yellow, std_color::zero });
			colorization.push_back({ "tell", std_color::yellow, std_color::zero });
			colorization.push_back({ "scan", std_color::yellow, std_color::zero });
			colorization.push_back({ "fetch", std_color::yellow, std_color::zero });
			colorization.push_back({ "check", std_color::yellow, std_color::zero });
			colorization.push_back({ "compare", std_color::yellow, std_color::zero });
			colorization.push_back({ "stat", std_color::yellow, std_color::zero });
			colorization.push_back({ "migrate", std_color::yellow, std_color::zero });
			colorization.push_back({ "prepare", std_color::yellow, std_color::zero });
			colorization.push_back({ "load", std_color::yellow, std_color::zero });
			colorization.push_back({ "unload", std_color::yellow, std_color::zero });
			colorization.push_back({ "save", std_color::yellow, std_color::zero });
			colorization.push_back({ "query", std_color::magenta, std_color::zero });
			colorization.push_back({ "template", std_color::blue, std_color::zero });
			colorization.push_back({ "byte", std_color::cyan, std_color::zero });
			colorization.push_back({ "bytes", std_color::cyan, std_color::zero });
			colorization.push_back({ "epoll", std_color::cyan, std_color::zero });
			colorization.push_back({ "kqueue", std_color::cyan, std_color::zero });
			colorization.push_back({ "poll", std_color::cyan, std_color::zero });
			colorization.push_back({ "dns", std_color::cyan, std_color::zero });
			colorization.push_back({ "file", std_color::cyan, std_color::zero });
			colorization.push_back({ "sock", std_color::cyan, std_color::zero });
			colorization.push_back({ "dir", std_color::cyan, std_color::zero });
			colorization.push_back({ "fd", std_color::cyan, std_color::zero });

			/* graphics */
			colorization.push_back({ "compile", std_color::yellow, std_color::zero });
			colorization.push_back({ "transpile", std_color::yellow, std_color::zero });
			colorization.push_back({ "show", std_color::yellow, std_color::zero });
			colorization.push_back({ "hide", std_color::yellow, std_color::zero });
			colorization.push_back({ "clear", std_color::yellow, std_color::zero });
			colorization.push_back({ "resize", std_color::yellow, std_color::zero });
			colorization.push_back({ "vcall", std_color::magenta, std_color::zero });
			colorization.push_back({ "shader", std_color::cyan, std_color::zero });
			colorization.push_back({ "bytecode", std_color::cyan, std_color::zero });

			/* audio */
			colorization.push_back({ "play", std_color::yellow, std_color::zero });
			colorization.push_back({ "stop", std_color::yellow, std_color::zero });
			colorization.push_back({ "apply", std_color::yellow, std_color::zero });

			/* engine */
			colorization.push_back({ "configure", std_color::yellow, std_color::zero });
			colorization.push_back({ "actualize", std_color::yellow, std_color::zero });
			colorization.push_back({ "register", std_color::yellow, std_color::zero });
			colorization.push_back({ "unregister", std_color::yellow, std_color::zero });
			colorization.push_back({ "entity", std_color::cyan, std_color::zero });
			colorization.push_back({ "component", std_color::cyan, std_color::zero });
			colorization.push_back({ "material", std_color::cyan, std_color::zero });

			/* crypto */
			colorization.push_back({ "encode", std_color::yellow, std_color::zero });
			colorization.push_back({ "decode", std_color::yellow, std_color::zero });
			colorization.push_back({ "encrypt", std_color::yellow, std_color::zero });
			colorization.push_back({ "decrypt", std_color::yellow, std_color::zero });
			colorization.push_back({ "compress", std_color::yellow, std_color::zero });
			colorization.push_back({ "decompress", std_color::yellow, std_color::zero });
			colorization.push_back({ "transform", std_color::yellow, std_color::zero });
			colorization.push_back({ "shuffle", std_color::yellow, std_color::zero });
			colorization.push_back({ "sign", std_color::yellow, std_color::zero });
			colorization.push_back({ "expose", std_color::dark_red, std_color::zero });

			/* memory */
			colorization.push_back({ "add", std_color::yellow, std_color::zero });
			colorization.push_back({ "remove", std_color::yellow, std_color::zero });
			colorization.push_back({ "new", std_color::yellow, std_color::zero });
			colorization.push_back({ "delete", std_color::yellow, std_color::zero });
			colorization.push_back({ "create", std_color::yellow, std_color::zero });
			colorization.push_back({ "destroy", std_color::yellow, std_color::zero });
			colorization.push_back({ "push", std_color::yellow, std_color::zero });
			colorization.push_back({ "pop", std_color::yellow, std_color::zero });
			colorization.push_back({ "malloc", std_color::yellow, std_color::zero });
			colorization.push_back({ "free", std_color::yellow, std_color::zero });
			colorization.push_back({ "allocate", std_color::yellow, std_color::zero });
			colorization.push_back({ "deallocate", std_color::yellow, std_color::zero });
			colorization.push_back({ "initialize", std_color::yellow, std_color::zero });
			colorization.push_back({ "generate", std_color::yellow, std_color::zero });
			colorization.push_back({ "finalize", std_color::yellow, std_color::zero });
			colorization.push_back({ "cleanup", std_color::yellow, std_color::zero });
			colorization.push_back({ "copy", std_color::yellow, std_color::zero });
			colorization.push_back({ "fill", std_color::yellow, std_color::zero });
			colorization.push_back({ "store", std_color::yellow, std_color::zero });
			colorization.push_back({ "reuse", std_color::yellow, std_color::zero });
			colorization.push_back({ "update", std_color::yellow, std_color::zero });
			colorization.push_back({ "true", std_color::cyan, std_color::zero });
			colorization.push_back({ "false", std_color::cyan, std_color::zero });
			colorization.push_back({ "on", std_color::cyan, std_color::zero });
			colorization.push_back({ "off", std_color::cyan, std_color::zero });
			colorization.push_back({ "undefined", std_color::cyan, std_color::zero });
			colorization.push_back({ "nullptr", std_color::cyan, std_color::zero });
			colorization.push_back({ "null", std_color::cyan, std_color::zero });
			colorization.push_back({ "this", std_color::cyan, std_color::zero });

			/* statuses */
			colorization.push_back({ "ON", std_color::dark_green, std_color::zero });
			colorization.push_back({ "TRUE", std_color::dark_green, std_color::zero });
			colorization.push_back({ "OK", std_color::dark_green, std_color::zero });
			colorization.push_back({ "SUCCESS", std_color::dark_green, std_color::zero });
			colorization.push_back({ "ASSERT", std_color::yellow, std_color::zero });
			colorization.push_back({ "CAUSING", std_color::yellow, std_color::zero });
			colorization.push_back({ "warn", std_color::yellow, std_color::zero });
			colorization.push_back({ "warning", std_color::yellow, std_color::zero });
			colorization.push_back({ "debug", std_color::yellow, std_color::zero });
			colorization.push_back({ "debugging", std_color::yellow, std_color::zero });
			colorization.push_back({ "trace", std_color::yellow, std_color::zero });
			colorization.push_back({ "trading", std_color::yellow, std_color::zero });
			colorization.push_back({ "OFF", std_color::dark_red, std_color::zero });
			colorization.push_back({ "FALSE", std_color::dark_red, std_color::zero });
			colorization.push_back({ "NULL", std_color::dark_red, std_color::zero });
			colorization.push_back({ "ERR", std_color::dark_red, std_color::zero });
			colorization.push_back({ "FATAL", std_color::dark_red, std_color::zero });
			colorization.push_back({ "PANIC!", std_color::dark_red, std_color::zero });
			colorization.push_back({ "leaking", std_color::dark_red, std_color::zero });
			colorization.push_back({ "failure", std_color::dark_red, std_color::zero });
			colorization.push_back({ "failed", std_color::dark_red, std_color::zero });
			colorization.push_back({ "error", std_color::dark_red, std_color::zero });
			colorization.push_back({ "errors", std_color::dark_red, std_color::zero });
			colorization.push_back({ "cannot", std_color::dark_red, std_color::zero });
			colorization.push_back({ "missing", std_color::dark_red, std_color::zero });
			colorization.push_back({ "invalid", std_color::dark_red, std_color::zero });
			colorization.push_back({ "required", std_color::dark_red, std_color::zero });
			colorization.push_back({ "already", std_color::dark_red, std_color::zero });
		}
		console::~console() noexcept
		{
			deallocate();
		}
		void console::allocate()
		{
			umutex<std::recursive_mutex> unique(state.session);
			if (state.status != mode::detached)
				return;
#ifdef VI_MICROSOFT
			if (AllocConsole())
			{
				streams.input = freopen("conin$", "r", stdin);
				streams.output = freopen("conout$", "w", stdout);
				streams.errors = freopen("conout$", "w", stderr);
			}

			CONSOLE_SCREEN_BUFFER_INFO screen_buffer;
			HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
			if (GetConsoleScreenBufferInfo(handle, &screen_buffer))
				state.attributes = screen_buffer.wAttributes;

			SetConsoleCtrlHandler(console_event_handler, true);
			VI_TRACE("[console] allocate window 0x%" PRIXPTR, (void*)handle);
#endif
			state.status = mode::allocated;
		}
		void console::deallocate()
		{
			umutex<std::recursive_mutex> unique(state.session);
			if (state.status != mode::allocated)
				return;
#ifdef VI_MICROSOFT
			::ShowWindow(::GetConsoleWindow(), SW_HIDE);
			VI_TRACE("[console] deallocate window");
#endif
			state.status = mode::detached;
		}
		void console::hide()
		{
			umutex<std::recursive_mutex> unique(state.session);
#ifdef VI_MICROSOFT
			VI_TRACE("[console] hide window");
			::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#endif
		}
		void console::show()
		{
			umutex<std::recursive_mutex> unique(state.session);
			allocate();
#ifdef VI_MICROSOFT
			::ShowWindow(::GetConsoleWindow(), SW_SHOW);
			VI_TRACE("[console] show window");
#endif
		}
		void console::clear()
		{
			umutex<std::recursive_mutex> unique(state.session);
			state.elements.clear();
#ifdef VI_MICROSOFT
			HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
			DWORD written = 0;

			CONSOLE_SCREEN_BUFFER_INFO info;
			GetConsoleScreenBufferInfo(handle, &info);

			COORD top_left = { 0, 0 };
			FillConsoleOutputCharacterA(handle, ' ', info.dwSize.X * info.dwSize.Y, top_left, &written);
			FillConsoleOutputAttribute(handle, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE, info.dwSize.X * info.dwSize.Y, top_left, &written);
			SetConsoleCursorPosition(handle, top_left);
#else
			int exit_code = std::system("clear");
			(void)exit_code;
#endif
		}
		void console::attach()
		{
			umutex<std::recursive_mutex> unique(state.session);
			if (state.status != mode::detached)
				return;
#ifdef VI_MICROSOFT
			CONSOLE_SCREEN_BUFFER_INFO screen_buffer;
			HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
			if (GetConsoleScreenBufferInfo(handle, &screen_buffer))
				state.attributes = screen_buffer.wAttributes;

			SetConsoleCtrlHandler(console_event_handler, true);
			VI_TRACE("[console] attach window 0x%" PRIXPTR, (void*)handle);
#endif
			state.status = mode::attached;
		}
		void console::detach()
		{
			umutex<std::recursive_mutex> unique(state.session);
			state.status = mode::detached;
		}
		void console::trace(uint32_t max_frames)
		{
			string stacktrace = error_handling::get_stack_trace(0, max_frames);
			umutex<std::recursive_mutex> unique(state.session);
			std::cout << stacktrace << '\n';
		}
		void console::set_colorization(bool enabled)
		{
			state.colorizer = enabled;
		}
		void console::add_colorization(const std::string_view& name, std_color foreground_color, std_color background_color)
		{
			umutex<std::recursive_mutex> unique(state.session);
			for (auto it = colorization.begin(); it != colorization.end(); it++)
			{
				if (it->identifier == name)
				{
					colorization.erase(it);
					break;
				}
			}
			if (!((foreground_color == std_color::zero || foreground_color == std_color::white) && (background_color == std_color::black || background_color == std_color::white)))
				colorization.push_back({ name, foreground_color, background_color });
		}
		void console::clear_colorization()
		{
			umutex<std::recursive_mutex> unique(state.session);
			colorization.clear();
		}
		void console::write_color(std_color text, std_color background)
		{
			if (!state.colorizer)
				return;

			umutex<std::recursive_mutex> unique(state.session);
#ifdef VI_MICROSOFT
			if (background == std_color::zero)
				background = std_color::black;

			if (text == std_color::zero)
				text = std_color::white;

			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (int)background << 4 | (int)text);
#else
			std::cout << "\033[" << get_color_id(text, false) << ";" << get_color_id(background, true) << "m";
#endif
		}
		void console::clear_color()
		{
			if (!state.colorizer)
				return;
			umutex<std::recursive_mutex> unique(state.session);
#ifdef VI_MICROSOFT
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), state.attributes);
#else
			std::cout << "\033[0m";
#endif
		}
		void console::colorize(std_color base_color, const std::string_view& buffer)
		{
			if (buffer.empty())
				return;

			umutex<std::recursive_mutex> unique(state.session);
			write_color(base_color);

			size_t offset = 0;
			while (offset < buffer.size())
			{
				auto v = buffer[offset];
				if (stringify::is_numeric_or_dot(v) && (!offset || !stringify::is_alphanum(buffer[offset - 1])))
				{
					write_color(std_color::yellow);
					while (offset < buffer.size())
					{
						char n = buffer[offset];
						if (!stringify::is_hex_or_dot(n) && n != 'x')
							break;

						write_char(buffer[offset++]);
					}

					write_color(base_color);
					continue;
				}
				else if (v == '[' && buffer.substr(offset + 1).find(']') != std::string::npos)
				{
					size_t iterations = 0, skips = 0;
					write_color(std_color::cyan);
					do
					{
						write_char(buffer[offset]);
						if (iterations++ > 0 && buffer[offset] == '[')
							skips++;
					} while (offset < buffer.size() && (buffer[offset++] != ']' || skips > 0));

					write_color(base_color);
					continue;
				}
				else if (v == '\"' && buffer.substr(offset + 1).find('\"') != std::string::npos)
				{
					write_color(std_color::light_blue);
					do
					{
						write_char(buffer[offset]);
					} while (offset < buffer.size() && buffer[++offset] != '\"');

					if (offset < buffer.size())
						write_char(buffer[offset++]);
					write_color(base_color);
					continue;
				}
				else if (v == '\'' && buffer.substr(offset + 1).find('\'') != std::string::npos)
				{
					write_color(std_color::light_blue);
					do
					{
						write_char(buffer[offset]);
					} while (offset < buffer.size() && buffer[++offset] != '\'');

					if (offset < buffer.size())
						write_char(buffer[offset++]);
					write_color(base_color);
					continue;
				}
				else if (stringify::is_alphabetic(v) && (!offset || !stringify::is_alphabetic(buffer[offset - 1])))
				{
					bool is_matched = false;
					for (auto& token : colorization)
					{
						if (token.identifier.empty() || v != token.identifier.front() || buffer.size() - offset < token.identifier.size())
							continue;

						if (offset + token.identifier.size() < buffer.size() && stringify::is_alphabetic(buffer[offset + token.identifier.size()]))
							continue;

						if (memcmp(buffer.data() + offset, token.identifier.data(), token.identifier.size()) == 0)
						{
							write_color(token.foreground_color, token.background_color);
							write(token.identifier);
							write_color(base_color);
							offset += token.identifier.size();
							is_matched = true;
							break;
						}
					}

					if (is_matched)
						continue;
				}

				write_char(v);
				++offset;
			}
		}
		void console::capture_time()
		{
			state.time = (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
		}
		uint64_t console::capture_window(uint32_t height)
		{
			if (!height)
				return 0;

			window_state window;
			window.elements.reserve(height);

			umutex<std::recursive_mutex> unique(state.session);
			for (size_t i = 0; i < height; i++)
			{
				uint64_t id = capture_element();
				if (id > 0)
				{
					window.elements.push_back(std::make_pair(id, string()));
					continue;
				}

				for (auto& element : window.elements)
					free_element(element.first);

				return 0;
			}

			uint64_t id = ++state.id;
			state.windows[id] = std::move(window);
			return id;
		}
		void console::free_window(uint64_t id, bool restore_position)
		{
			umutex<std::recursive_mutex> unique(state.session);
			auto it = state.windows.find(id);
			if (it == state.windows.end())
				return;

			uint32_t y = 0;
			bool exists = false;
			if (restore_position && !it->second.elements.empty())
			{
				auto root = state.elements.find(it->second.elements.front().first);
				if (root != state.elements.end())
				{
					y = root->second.y;
					exists = true;
				}
			}

			for (auto& element : it->second.elements)
			{
				clear_element(element.first);
				free_element(element.first);
			}

			state.windows.erase(id);
			if (exists && restore_position)
				write_position(0, y);
		}
		void console::emplace_window(uint64_t id, const std::string_view& text)
		{
			umutex<std::recursive_mutex> unique(state.session);
			auto it = state.windows.find(id);
			if (it == state.windows.end() || it->second.elements.empty())
				return;

			size_t count = it->second.elements.size();
			if (it->second.position >= count)
			{
				--count;
				for (size_t i = 0; i < count; i++)
				{
					auto& prev_element = it->second.elements[i + 0];
					auto& next_element = it->second.elements[i + 1];
					if (next_element.second == prev_element.second)
						continue;

					replace_element(prev_element.first, next_element.second);
					prev_element.second = std::move(next_element.second);
				}
				it->second.position = count;
			}

			auto& element = it->second.elements[it->second.position++];
			element.second = text;
			replace_element(element.first, element.second);
		}
		uint64_t console::capture_element()
		{
			element_state element;
			element.state = compute::crypto::random();

			umutex<std::recursive_mutex> unique(state.session);
			if (!read_screen(nullptr, nullptr, &element.x, &element.y))
				return 0;

			uint64_t id = ++state.id;
			state.elements[id] = element;
			std::cout << '\n';
			return id;
		}
		void console::free_element(uint64_t id)
		{
			umutex<std::recursive_mutex> unique(state.session);
			auto it = state.elements.find(id);
			if (it != state.elements.end())
				state.elements.erase(id);
		}
		void console::resize_element(uint64_t id, uint32_t x)
		{
			umutex<std::recursive_mutex> unique(state.session);
			auto it = state.elements.find(id);
			if (it != state.elements.end())
				it->second.x = x;
		}
		void console::move_element(uint64_t id, uint32_t y)
		{
			umutex<std::recursive_mutex> unique(state.session);
			auto it = state.elements.find(id);
			if (it != state.elements.end())
				it->second.y = y;
		}
		void console::read_element(uint64_t id, uint32_t* x, uint32_t* y)
		{
			umutex<std::recursive_mutex> unique(state.session);
			auto it = state.elements.find(id);
			if (it != state.elements.end())
			{
				if (x != nullptr)
					*x = it->second.x;
				if (y != nullptr)
					*y = it->second.y;
			}
		}
		void console::replace_element(uint64_t id, const std::string_view& text)
		{
			string writeable = string(text);
			stringify::replace_of(writeable, "\b\f\n\r\v", " ");
			stringify::replace(writeable, "\t", "  ");

			umutex<std::recursive_mutex> unique(state.session);
			auto it = state.elements.find(id);
			if (it == state.elements.end())
				return;

			uint32_t width = 0, height = 0, x = 0, y = 0;
			if (!read_screen(&width, &height, &x, &y))
				return;

			write_position(0, it->second.y >= --height ? it->second.y - 1 : it->second.y);
			writeable = writeable.substr(0, --width);
			writeable.append(width - writeable.size(), ' ');
			writeable.append(1, '\n');
			std::cout << writeable;
			write_position(x, y);
		}
		void console::progress_element(uint64_t id, double value, double coverage)
		{
			uint32_t screen_width;
			if (!read_screen(&screen_width, nullptr, nullptr, nullptr))
				return;
			else if (screen_width < 8)
				return;

			string bar_content;
			bar_content.reserve(screen_width);
			bar_content += '[';
			screen_width -= 8;

			size_t bar_width = (size_t)(screen_width * std::max<double>(0.0, std::min<double>(1.0, coverage)));
			size_t bar_position = (size_t)(bar_width * std::max<double>(0.0, std::min<double>(1.0, value)));
			for (size_t i = 0; i < bar_width; i++)
			{
				if (i < bar_position)
					bar_content += '=';
				else if (i == bar_position)
					bar_content += '>';
				else
					bar_content += ' ';
			}

			char numeric[NUMSTR_SIZE];
			bar_content += "] ";
			bar_content += to_string_view<uint32_t>(numeric, sizeof(numeric), (uint32_t)(value * 100));
			bar_content += " %";
			replace_element(id, bar_content);
		}
		void console::spinning_element(uint64_t id, const std::string_view& label)
		{
			umutex<std::recursive_mutex> unique(state.session);
			auto it = state.elements.find(id);
			if (it == state.elements.end())
				return;

			uint64_t status = it->second.state++ % 4;
			switch (status)
			{
				case 0:
					replace_element(id, label.empty() ? "[|]" : string(label) + " [|]");
					break;
				case 1:
					replace_element(id, label.empty() ? "[/]" : string(label) + " [/]");
					break;
				case 2:
					replace_element(id, label.empty() ? "[-]" : string(label) + " [-]");
					break;
				case 3:
					replace_element(id, label.empty() ? "[\\]" : string(label) + " [\\]");
					break;
				default:
					replace_element(id, label.empty() ? "[ ]" : string(label) + " [ ]");
					break;
			}
		}
		void console::spinning_progress_element(uint64_t id, double value, double coverage)
		{
			umutex<std::recursive_mutex> unique(state.session);
			auto it = state.elements.find(id);
			if (it == state.elements.end())
				return;

			uint32_t screen_width;
			if (!read_screen(&screen_width, nullptr, nullptr, nullptr))
				return;
			else if (screen_width < 8)
				return;

			string bar_content;
			bar_content.reserve(screen_width);
			bar_content += '[';
			screen_width -= 8;

			size_t bar_width = (size_t)(screen_width * std::max<double>(0.0, std::min<double>(1.0, coverage)));
			size_t bar_position = (size_t)(bar_width * std::max<double>(0.0, std::min<double>(1.0, value)));
			for (size_t i = 0; i < bar_width; i++)
			{
				if (i == bar_position)
				{
					uint8_t status = it->second.state++ % 4;
					switch (status)
					{
						case 0:
							bar_content += '|';
							break;
						case 1:
							bar_content += '/';
							break;
						case 2:
							bar_content += '-';
							break;
						case 3:
							bar_content += '\\';
							break;
						default:
							bar_content += '>';
							break;
					}
				}
				else if (i < bar_position)
					bar_content += '=';
				else
					bar_content += ' ';
			}

			char numeric[NUMSTR_SIZE];
			bar_content += "] ";
			bar_content += to_string_view<uint32_t>(numeric, sizeof(numeric), (uint32_t)(value * 100));
			bar_content += " %";
			replace_element(id, bar_content);
		}
		void console::clear_element(uint64_t id)
		{
			replace_element(id, string());
		}
		void console::flush()
		{
			umutex<std::recursive_mutex> unique(state.session);
			std::cout.flush();
		}
		void console::flush_write()
		{
			umutex<std::recursive_mutex> unique(state.session);
			std::cout << std::flush;
		}
		void console::write_size(uint32_t width, uint32_t height)
		{
			umutex<std::recursive_mutex> unique(state.session);
#ifdef VI_MICROSOFT
			SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), { (short)width, (short)height });
#else
			struct winsize size;
			size.ws_col = width;
			size.ws_row = height;
			ioctl(STDOUT_FILENO, TIOCSWINSZ, &size);
#endif
		}
		void console::write_position(uint32_t x, uint32_t y)
		{
			umutex<std::recursive_mutex> unique(state.session);
#ifdef VI_MICROSOFT
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (short)x, (short)y });
#else
			std::cout << "\033[" << x << ';' << y << 'h';
#endif
		}
		void console::write_line(const std::string_view& line)
		{
			umutex<std::recursive_mutex> unique(state.session);
			std::cout << line << '\n';
		}
		void console::write_char(char value)
		{
			umutex<std::recursive_mutex> unique(state.session);
			std::cout << value;
		}
		void console::write(const std::string_view& text)
		{
			umutex<std::recursive_mutex> unique(state.session);
			std::cout << text;
		}
		void console::jwrite(schema* data)
		{
			umutex<std::recursive_mutex> unique(state.session);
			if (!data)
			{
				std::cout << "null";
				return;
			}

			string offset;
			schema::convert_to_json(data, [&offset](core::var_form pretty, const std::string_view& buffer)
			{
				if (!buffer.empty())
					std::cout << buffer;

				switch (pretty)
				{
					case vitex::core::var_form::tab_decrease:
						offset.erase(offset.size() - 2);
						break;
					case vitex::core::var_form::tab_increase:
						offset.append(2, ' ');
						break;
					case vitex::core::var_form::write_space:
						std::cout << ' ';
						break;
					case vitex::core::var_form::write_line:
						std::cout << '\n';
						break;
					case vitex::core::var_form::write_tab:
						std::cout << offset;
						break;
					default:
						break;
				}
			});
		}
		void console::jwrite_line(schema* data)
		{
			umutex<std::recursive_mutex> unique(state.session);
			jwrite(data);
			std::cout << '\n';
		}
		void console::fwrite_line(const char* format, ...)
		{
			VI_ASSERT(format != nullptr, "format should be set");
			char buffer[BLOB_SIZE] = { '\0' };
			va_list args;
			va_start(args, format);
#ifdef VI_MICROSOFT
			_vsnprintf(buffer, sizeof(buffer), format, args);
#else
			vsnprintf(buffer, sizeof(buffer), format, args);
#endif
			va_end(args);

			umutex<std::recursive_mutex> unique(state.session);
			std::cout << buffer << '\n';
		}
		void console::fwrite(const char* format, ...)
		{
			VI_ASSERT(format != nullptr, "format should be set");
			char buffer[BLOB_SIZE] = { '\0' };
			va_list args;
			va_start(args, format);
#ifdef VI_MICROSOFT
			_vsnprintf(buffer, sizeof(buffer), format, args);
#else
			vsnprintf(buffer, sizeof(buffer), format, args);
#endif
			va_end(args);

			umutex<std::recursive_mutex> unique(state.session);
			std::cout << buffer;
		}
		double console::get_captured_time() const
		{
			return (double)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0 - state.time;
		}
		bool console::read_screen(uint32_t* width, uint32_t* height, uint32_t* x, uint32_t* y)
		{
			umutex<std::recursive_mutex> unique(state.session);
#ifdef VI_MICROSOFT
			CONSOLE_SCREEN_BUFFER_INFO size;
			if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &size) != TRUE)
				return false;

			if (width != nullptr)
				*width = (uint32_t)(size.srWindow.Right - size.srWindow.Left + 1);

			if (height != nullptr)
				*height = (uint32_t)(size.srWindow.Bottom - size.srWindow.Top + 1);

			if (x != nullptr)
				*x = (uint32_t)size.dwCursorPosition.X;

			if (y != nullptr)
				*y = (uint32_t)size.dwCursorPosition.Y;

			return true;
#else
			if (width != nullptr || height != nullptr)
			{
				struct winsize size;
				if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) < 0)
					return false;

				if (width != nullptr)
					*width = (uint32_t)size.ws_col;

				if (height != nullptr)
					*height = (uint32_t)size.ws_row;
			}

			if (x != nullptr || y != nullptr)
			{
				static bool responsive = true;
				if (!responsive)
					return false;

				struct termios prev;
				tcgetattr(STDIN_FILENO, &prev);

				struct termios next = prev;
				next.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
				next.c_oflag &= ~(OPOST);
				next.c_cflag |= (CS8);
				next.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
				next.c_cc[VMIN] = 0;
				next.c_cc[VTIME] = 0;

				tcsetattr(STDIN_FILENO, TCSANOW, &next);
				tcsetattr(STDOUT_FILENO, TCSANOW, &next);
				fwrite("\033[6n", 4, 1, stdout);
				fflush(stdout);

				fd_set fd;
				FD_ZERO(&fd);
				FD_SET(STDIN_FILENO, &fd);

				struct timeval time;
				time.tv_sec = 0;
				time.tv_usec = 500000;

				if (select(STDIN_FILENO + 1, &fd, nullptr, nullptr, &time) != 1)
				{
					responsive = false;
					tcsetattr(STDIN_FILENO, TCSADRAIN, &prev);
					return false;
				}

				int target_x = 0, target_y = 0;
				if (scanf("\033[%d;%dR", &target_x, &target_y) != 2)
				{
					tcsetattr(STDIN_FILENO, TCSADRAIN, &prev);
					return false;
				}

				if (x != nullptr)
					*x = target_x;

				if (y != nullptr)
					*y = target_y;

				tcsetattr(STDIN_FILENO, TCSADRAIN, &prev);
			}

			return true;
#endif
		}
		bool console::read_line(string& data, size_t size)
		{
			VI_ASSERT(state.status != mode::detached, "console should be shown at least once");
			VI_ASSERT(size > 0, "read length should be greater than zero");
			VI_TRACE("[console] read up to %" PRIu64 " bytes", (uint64_t)size);

			bool success;
			if (size > CHUNK_SIZE - 1)
			{
				char* value = memory::allocate<char>(sizeof(char) * (size + 1));
				memset(value, 0, (size + 1) * sizeof(char));
				if ((success = (bool)std::cin.getline(value, size)))
					data.assign(value);
				else
					data.clear();
				memory::deallocate(value);
			}
			else
			{
				char value[CHUNK_SIZE] = { 0 };
				if ((success = (bool)std::cin.getline(value, size)))
					data.assign(value);
				else
					data.clear();
			}

			return success;
		}
		string console::read(size_t size)
		{
			string data;
			data.reserve(size);
			read_line(data, size);
			return data;
		}
		char console::read_char()
		{
			return (char)getchar();
		}
		bool console::is_available()
		{
			return has_instance() && get()->state.status != mode::detached;
		}

		static float units_to_seconds = 1000000.0f;
		static float units_to_mills = 1000.0f;
		timer::timer() noexcept
		{
			reset();
		}
		void timer::set_fixed_frames(float value)
		{
			fixed_frames = value;
			if (fixed_frames > 0.0)
				max_delta = to_units(1.0f / fixed_frames);
		}
		void timer::set_max_frames(float value)
		{
			max_frames = value;
			if (max_frames > 0.0)
				min_delta = to_units(1.0f / max_frames);
		}
		void timer::reset()
		{
			timing.begin = clock();
			timing.when = timing.begin;
			timing.delta = units(0);
			timing.frame = 0;
			fixed.when = timing.begin;
			fixed.delta = units(0);
			fixed.sum = units(0);
			fixed.frame = 0;
			fixed.in_frame = false;
		}
		void timer::begin()
		{
			units time = clock();
			timing.delta = time - timing.when;
			++timing.frame;

			if (fixed_frames <= 0.0)
				return;

			fixed.sum += timing.delta;
			fixed.in_frame = fixed.sum > max_delta;
			if (fixed.in_frame)
			{
				fixed.sum = units(0);
				fixed.delta = time - fixed.when;
				fixed.when = time;
				++fixed.frame;
			}
		}
		void timer::finish()
		{
			timing.when = clock();
			if (max_frames > 0.0 && timing.delta < min_delta)
				std::this_thread::sleep_for(min_delta - timing.delta);
		}
		void timer::push(const char* name)
		{
			capture next = { name, clock() };
			captures.emplace(std::move(next));
		}
		bool timer::pop_if(float greater_than, capture* out)
		{
			capture data = pop();
			bool overdue = (data.step > greater_than);
			if (out != nullptr)
				*out = std::move(data);

			return overdue;
		}
		bool timer::is_fixed() const
		{
			return fixed.in_frame;
		}
		timer::capture timer::pop()
		{
			VI_ASSERT(!captures.empty(), "there is no time captured at the moment");
			capture base = captures.front();
			base.end = clock();
			base.delta = base.end - base.begin;
			base.step = to_seconds(base.delta);
			captures.pop();

			return base;
		}
		size_t timer::get_frame_index() const
		{
			return timing.frame;
		}
		size_t timer::get_fixed_frame_index() const
		{
			return fixed.frame;
		}
		float timer::get_max_frames() const
		{
			return max_frames;
		}
		float timer::get_min_step() const
		{
			if (max_frames <= 0.0f)
				return 0.0f;

			return to_seconds(min_delta);
		}
		float timer::get_frames() const
		{
			return 1.0f / to_seconds(timing.delta);
		}
		float timer::get_elapsed() const
		{
			return to_seconds(clock() - timing.begin);
		}
		float timer::get_elapsed_mills() const
		{
			return to_mills(clock() - timing.begin);
		}
		float timer::get_step() const
		{
			return to_seconds(timing.delta);
		}
		float timer::get_fixed_step() const
		{
			return to_seconds(fixed.delta);
		}
		float timer::get_fixed_frames() const
		{
			return fixed_frames;
		}
		float timer::to_seconds(const units& value)
		{
			return (float)((double)value.count() / units_to_seconds);
		}
		float timer::to_mills(const units& value)
		{
			return (float)((double)value.count() / units_to_mills);
		}
		timer::units timer::to_units(float value)
		{
			return units((uint64_t)(value * units_to_seconds));
		}
		timer::units timer::clock()
		{
			return std::chrono::duration_cast<units>(std::chrono::system_clock::now().time_since_epoch());
		}

		stream::stream() noexcept : vsize(0)
		{
		}
		expects_io<void> stream::move(int64_t offset)
		{
			return seek(file_seek::current, offset);
		}
		void stream::open_virtual(string&& path)
		{
			vname = std::move(path);
			vsize = 0;
		}
		void stream::close_virtual()
		{
			vname.clear();
			vsize = 0;
		}
		void stream::set_virtual_size(size_t size)
		{
			vsize = size;
		}
		void stream::set_virtual_name(const std::string_view& file)
		{
			vname = file;
		}
		expects_io<size_t> stream::read_all(const std::function<void(uint8_t*, size_t)>& callback)
		{
			VI_ASSERT(callback != nullptr, "callback should be set");
			VI_TRACE("[io] read all bytes on fd %i", get_readable_fd());

			size_t total = 0;
			uint8_t buffer[CHUNK_SIZE];
			while (true)
			{
				size_t length = vsize > 0 ? std::min<size_t>(sizeof(buffer), vsize - total) : sizeof(buffer);
				if (!length)
					break;

				auto size = read(buffer, length);
				if (!size || !*size)
					return size;

				length = *size;
				total += length;
				callback(buffer, length);
			}

			return total;
		}
		expects_io<size_t> stream::size()
		{
			if (!is_sized())
				return std::make_error_condition(std::errc::not_supported);

			auto position = tell();
			if (!position)
				return position;

			auto status = seek(file_seek::end, 0);
			if (!status)
				return status.error();

			auto size = tell();
			status = seek(file_seek::begin, *position);
			if (!status)
				return status.error();

			return size;
		}
		size_t stream::virtual_size() const
		{
			return vsize;
		}
		std::string_view stream::virtual_name() const
		{
			return vname;
		}

		memory_stream::memory_stream() noexcept : offset(0), readable(false), writeable(false)
		{
		}
		memory_stream::~memory_stream() noexcept
		{
			close();
		}
		expects_io<void> memory_stream::clear()
		{
			VI_TRACE("[mem] fd %i clear", get_writeable_fd());
			buffer.clear();
			offset = 0;
			readable = false;
			return expectation::met;
		}
		expects_io<void> memory_stream::open(const std::string_view& file, file_mode mode)
		{
			VI_ASSERT(!file.empty(), "filename should be set");
			VI_MEASURE(timings::pass);
			auto result = close();
			if (!result)
				return result;
			else if (!os::control::has(access_option::mem))
				return std::make_error_condition(std::errc::permission_denied);

			switch (mode)
			{
				case file_mode::read_only:
				case file_mode::binary_read_only:
					readable = true;
					break;
				case file_mode::write_only:
				case file_mode::binary_write_only:
				case file_mode::append_only:
				case file_mode::binary_append_only:
					writeable = true;
					break;
				case file_mode::read_write:
				case file_mode::binary_read_write:
				case file_mode::write_read:
				case file_mode::binary_write_read:
				case file_mode::read_append_write:
				case file_mode::binary_read_append_write:
					readable = true;
					writeable = true;
					break;
				default:
					break;
			}

			VI_PANIC(readable || writeable, "file open cannot be issued with mode:%i", (int)mode);
			VI_DEBUG("[mem] open %s%s %s fd", readable ? "r" : "", writeable ? "w" : "", file, (int)get_readable_fd());
			open_virtual(string(file));
			return expectation::met;
		}
		expects_io<void> memory_stream::close()
		{
			VI_MEASURE(timings::pass);
			VI_DEBUG("[mem] close fd %i", get_readable_fd());
			close_virtual();
			buffer.clear();
			offset = 0;
			readable = false;
			writeable = false;
			return expectation::met;
		}
		expects_io<void> memory_stream::seek(file_seek mode, int64_t seek)
		{
			VI_ASSERT(!virtual_name().empty(), "file should be opened");
			VI_MEASURE(timings::pass);
			switch (mode)
			{
				case file_seek::begin:
					VI_TRACE("[mem] seek-64 fd %i begin %" PRId64, get_readable_fd(), seek);
					offset = seek < 0 ? 0 : buffer.size() + (size_t)seek;
					break;
				case file_seek::current:
					VI_TRACE("[mem] seek-64 fd %i move %" PRId64, get_readable_fd(), seek);
					if (seek < 0)
					{
						size_t position = (size_t)(-seek);
						offset -= position > offset ? 0 : position;
					}
					else
						offset += (size_t)seek;
					break;
				case file_seek::end:
					VI_TRACE("[mem] seek-64 fd %i end %" PRId64, get_readable_fd(), seek);
					if (seek < 0)
					{
						size_t position = (size_t)(-seek);
						offset = position > buffer.size() ? 0 : buffer.size() - position;
					}
					else
						offset = buffer.size() + (size_t)seek;
					break;
				default:
					break;
			}
			return expectation::met;
		}
		expects_io<void> memory_stream::flush()
		{
			return expectation::met;
		}
		expects_io<size_t> memory_stream::read_scan(const char* format, ...)
		{
			VI_ASSERT(!virtual_name().empty(), "file should be opened");
			VI_ASSERT(format != nullptr, "format should be set");
			VI_MEASURE(timings::pass);
			char* memory = prepare_buffer(0);
			if (!memory)
				return std::make_error_condition(std::errc::broken_pipe);

			va_list args;
			va_start(args, format);
			int value = vsscanf(memory, format, args);
			VI_TRACE("[mem] fd %i scan %i bytes", get_readable_fd(), (int)value);
			va_end(args);
			if (value >= 0)
				return (size_t)value;

			return os::error::get_condition_or(std::errc::broken_pipe);
		}
		expects_io<size_t> memory_stream::read_line(char* data, size_t length)
		{
			VI_ASSERT(!virtual_name().empty(), "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
			VI_MEASURE(timings::pass);
			VI_TRACE("[mem] fd %i readln %i bytes", get_readable_fd(), (int)length);
			size_t offset = 0;
			while (offset < length)
			{
				auto status = read((uint8_t*)data + offset, sizeof(uint8_t));
				if (!status)
					return status;
				else if (*status != sizeof(uint8_t))
					break;
				else if (stringify::is_whitespace(data[offset++]))
					break;
			}
			return offset;
		}
		expects_io<size_t> memory_stream::read(uint8_t* data, size_t length)
		{
			VI_ASSERT(!virtual_name().empty(), "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
			VI_MEASURE(timings::pass);
			VI_TRACE("[mem] fd %i read %i bytes", get_readable_fd(), (int)length);
			if (!length)
				return 0;

			char* memory = prepare_buffer(length);
			if (!memory)
				return std::make_error_condition(std::errc::broken_pipe);

			memcpy(data, memory, length);
			return length;
		}
		expects_io<size_t> memory_stream::write_format(const char* format, ...)
		{
			VI_ASSERT(!virtual_name().empty(), "file should be opened");
			VI_ASSERT(format != nullptr, "format should be set");
			VI_MEASURE(timings::pass);
			char* memory = prepare_buffer(0);
			if (!memory)
				return std::make_error_condition(std::errc::broken_pipe);

			va_list args;
			va_start(args, format);
			int value = vsnprintf(memory, memory - buffer.data(), format, args);
			VI_TRACE("[mem] fd %i write %i bytes", get_writeable_fd(), value);
			va_end(args);
			if (value >= 0)
				return (size_t)value;

			return os::error::get_condition_or(std::errc::broken_pipe);
		}
		expects_io<size_t> memory_stream::write(const uint8_t* data, size_t length)
		{
			VI_ASSERT(!virtual_name().empty(), "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
			VI_MEASURE(timings::pass);
			VI_TRACE("[mem] fd %i write %i bytes", get_writeable_fd(), (int)length);
			if (!length)
				return 0;

			char* memory = prepare_buffer(length);
			if (!memory)
				return std::make_error_condition(std::errc::broken_pipe);

			memcpy(memory, data, length);
			return length;
		}
		expects_io<size_t> memory_stream::tell()
		{
			return offset;
		}
		socket_t memory_stream::get_readable_fd() const
		{
			return (socket_t)(uintptr_t)buffer.data();
		}
		socket_t memory_stream::get_writeable_fd() const
		{
			return (socket_t)(uintptr_t)buffer.data();
		}
		void* memory_stream::get_readable() const
		{
			return (void*)buffer.data();
		}
		void* memory_stream::get_writeable() const
		{
			return (void*)buffer.data();
		}
		bool memory_stream::is_sized() const
		{
			return true;
		}
		char* memory_stream::prepare_buffer(size_t size)
		{
			if (offset + size > buffer.size())
				buffer.resize(offset);

			char* target = buffer.data() + offset;
			offset += size;
			return target;
		}

		file_stream::file_stream() noexcept : io_stream(nullptr)
		{
		}
		file_stream::~file_stream() noexcept
		{
			close();
		}
		expects_io<void> file_stream::clear()
		{
			VI_TRACE("[io] fd %i clear", get_writeable_fd());
			auto result = close();
			if (!result)
				return result;

			auto path = virtual_name();
			if (path.empty())
				return std::make_error_condition(std::errc::invalid_argument);

			auto target = os::file::open(path, "w");
			if (!target)
				return target.error();

			io_stream = *target;
			return expectation::met;
		}
		expects_io<void> file_stream::open(const std::string_view& file, file_mode mode)
		{
			VI_ASSERT(!file.empty(), "filename should be set");
			VI_MEASURE(timings::file_system);
			auto result = close();
			if (!result)
				return result;
			else if (!os::control::has(access_option::fs))
				return std::make_error_condition(std::errc::permission_denied);

			const char* type = nullptr;
			switch (mode)
			{
				case file_mode::read_only:
					type = "r";
					break;
				case file_mode::write_only:
					type = "w";
					break;
				case file_mode::append_only:
					type = "a";
					break;
				case file_mode::read_write:
					type = "r+";
					break;
				case file_mode::write_read:
					type = "w+";
					break;
				case file_mode::read_append_write:
					type = "a+";
					break;
				case file_mode::binary_read_only:
					type = "rb";
					break;
				case file_mode::binary_write_only:
					type = "wb";
					break;
				case file_mode::binary_append_only:
					type = "ab";
					break;
				case file_mode::binary_read_write:
					type = "rb+";
					break;
				case file_mode::binary_write_read:
					type = "wb+";
					break;
				case file_mode::binary_read_append_write:
					type = "ab+";
					break;
				default:
					break;
			}

			VI_PANIC(type != nullptr, "file open cannot be issued with mode:%i", (int)mode);
			expects_io<string> target_path = os::path::resolve(file);
			if (!target_path)
				return target_path.error();

			auto target = os::file::open(*target_path, type);
			if (!target)
				return target.error();

			io_stream = *target;
			open_virtual(std::move(*target_path));
			return expectation::met;
		}
		expects_io<void> file_stream::close()
		{
			VI_MEASURE(timings::file_system);
			close_virtual();

			if (!io_stream)
				return expectation::met;

			FILE* target = io_stream;
			io_stream = nullptr;
			return os::file::close(target);
		}
		expects_io<void> file_stream::seek(file_seek mode, int64_t offset)
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_MEASURE(timings::file_system);
			return os::file::seek64(io_stream, offset, mode);
		}
		expects_io<void> file_stream::flush()
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_MEASURE(timings::file_system);
			VI_TRACE("[io] flush fd %i", get_writeable_fd());
			if (fflush(io_stream) != 0)
				return os::error::get_condition_or();
			return expectation::met;
		}
		expects_io<size_t> file_stream::read_scan(const char* format, ...)
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_ASSERT(format != nullptr, "format should be set");
			VI_MEASURE(timings::file_system);

			va_list args;
			va_start(args, format);
			int value = vfscanf(io_stream, format, args);
			VI_TRACE("[io] fd %i scan %i bytes", get_readable_fd(), value);
			va_end(args);
			if (value >= 0)
				return (size_t)value;

			return os::error::get_condition_or(std::errc::broken_pipe);
		}
		expects_io<size_t> file_stream::read_line(char* data, size_t length)
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
			VI_MEASURE(timings::file_system);
			VI_TRACE("[io] fd %i readln %i bytes", get_readable_fd(), (int)length);
			return fgets(data, (int)length, io_stream) ? strnlen(data, length) : 0;
		}
		expects_io<size_t> file_stream::read(uint8_t* data, size_t length)
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
			VI_MEASURE(timings::file_system);
			VI_TRACE("[io] fd %i read %i bytes", get_readable_fd(), (int)length);
			size_t value = fread(data, 1, length, io_stream);
			if (value > 0 || feof(io_stream) != 0)
				return (size_t)value;

			return os::error::get_condition_or(std::errc::broken_pipe);
		}
		expects_io<size_t> file_stream::write_format(const char* format, ...)
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_ASSERT(format != nullptr, "format should be set");
			VI_MEASURE(timings::file_system);

			va_list args;
			va_start(args, format);
			int value = vfprintf(io_stream, format, args);
			VI_TRACE("[io] fd %i write %i bytes", get_writeable_fd(), value);
			va_end(args);
			if (value >= 0)
				return (size_t)value;

			return os::error::get_condition_or(std::errc::broken_pipe);
		}
		expects_io<size_t> file_stream::write(const uint8_t* data, size_t length)
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
			VI_MEASURE(timings::file_system);
			VI_TRACE("[io] fd %i write %i bytes", get_writeable_fd(), (int)length);
			size_t value = fwrite(data, 1, length, io_stream);
			if (value > 0 || feof(io_stream) != 0)
				return (size_t)value;

			return os::error::get_condition_or(std::errc::broken_pipe);
		}
		expects_io<size_t> file_stream::tell()
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_MEASURE(timings::file_system);
			return os::file::tell64(io_stream);
		}
		socket_t file_stream::get_readable_fd() const
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			return (socket_t)VI_FILENO(io_stream);
		}
		socket_t file_stream::get_writeable_fd() const
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			return (socket_t)VI_FILENO(io_stream);
		}
		void* file_stream::get_readable() const
		{
			return io_stream;
		}
		void* file_stream::get_writeable() const
		{
			return io_stream;
		}
		bool file_stream::is_sized() const
		{
			return true;
		}

		gz_stream::gz_stream() noexcept : io_stream(nullptr)
		{
		}
		gz_stream::~gz_stream() noexcept
		{
			close();
		}
		expects_io<void> gz_stream::clear()
		{
			VI_TRACE("[gz] fd %i clear", get_writeable_fd());
			auto result = close();
			if (!result)
				return result;

			auto path = virtual_name();
			if (path.empty())
				return std::make_error_condition(std::errc::invalid_argument);

			auto target = os::file::open(path, "w");
			if (!target)
				return target.error();

			result = os::file::close(*target);
			if (!result)
				return result;

			return open(path, file_mode::binary_write_only);
		}
		expects_io<void> gz_stream::open(const std::string_view& file, file_mode mode)
		{
			VI_ASSERT(!file.empty(), "filename should be set");
#ifdef VI_ZLIB
			VI_MEASURE(timings::file_system);
			auto result = close();
			if (!result)
				return result;
			else if (!os::control::has(access_option::gz))
				return std::make_error_condition(std::errc::permission_denied);

			const char* type = nullptr;
			switch (mode)
			{
				case file_mode::read_only:
				case file_mode::binary_read_only:
					type = "rb";
					break;
				case file_mode::write_only:
				case file_mode::binary_write_only:
				case file_mode::append_only:
				case file_mode::binary_append_only:
					type = "wb";
					break;
				default:
					break;
			}

			VI_PANIC(type != nullptr, "file open cannot be issued with mode:%i", (int)mode);
			expects_io<string> target_path = os::path::resolve(file);
			if (!target_path)
				return target_path.error();

			VI_DEBUG("[gz] open %s %s", type, target_path->c_str());
			io_stream = gzopen(target_path->c_str(), type);
			if (!io_stream)
				return std::make_error_condition(std::errc::no_such_file_or_directory);

			open_virtual(std::move(*target_path));
			return expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<void> gz_stream::close()
		{
#ifdef VI_ZLIB
			VI_MEASURE(timings::file_system);
			close_virtual();

			if (!io_stream)
				return expectation::met;

			VI_DEBUG("[gz] close 0x%" PRIXPTR, (uintptr_t)io_stream);
			if (gzclose((gzFile)io_stream) != Z_OK)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			io_stream = nullptr;
			return expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<void> gz_stream::seek(file_seek mode, int64_t offset)
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
#ifdef VI_ZLIB
			VI_MEASURE(timings::file_system);
			switch (mode)
			{
				case file_seek::begin:
					VI_TRACE("[gz] seek fd %i begin %" PRId64, get_readable_fd(), offset);
					if (gzseek((gzFile)io_stream, (long)offset, SEEK_SET) == -1)
						return os::error::get_condition_or();
					return expectation::met;
				case file_seek::current:
					VI_TRACE("[gz] seek fd %i move %" PRId64, get_readable_fd(), offset);
					if (gzseek((gzFile)io_stream, (long)offset, SEEK_CUR) == -1)
						return os::error::get_condition_or();
					return expectation::met;
				case file_seek::end:
					return std::make_error_condition(std::errc::not_supported);
				default:
					return std::make_error_condition(std::errc::invalid_argument);
			}
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<void> gz_stream::flush()
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
#ifdef VI_ZLIB
			VI_MEASURE(timings::file_system);
			if (gzflush((gzFile)io_stream, Z_SYNC_FLUSH) != Z_OK)
				return os::error::get_condition_or();
			return expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<size_t> gz_stream::read_scan(const char* format, ...)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		expects_io<size_t> gz_stream::read_line(char* data, size_t length)
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
#ifdef VI_ZLIB
			VI_MEASURE(timings::file_system);
			VI_TRACE("[gz] fd %i readln %i bytes", get_readable_fd(), (int)length);
			return gzgets((gzFile)io_stream, data, (int)length) ? strnlen(data, length) : 0;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<size_t> gz_stream::read(uint8_t* data, size_t length)
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
#ifdef VI_ZLIB
			VI_MEASURE(timings::file_system);
			VI_TRACE("[gz] fd %i read %i bytes", get_readable_fd(), (int)length);
			int value = gzread((gzFile)io_stream, data, (uint32_t)length);
			if (value >= 0)
				return (size_t)value;

			return std::make_error_condition(std::errc::broken_pipe);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<size_t> gz_stream::write_format(const char* format, ...)
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_ASSERT(format != nullptr, "format should be set");
			VI_MEASURE(timings::file_system);
#ifdef VI_ZLIB
			va_list args;
			va_start(args, format);
			int value = gzvprintf((gzFile)io_stream, format, args);
			VI_TRACE("[gz] fd %i write %i bytes", get_writeable_fd(), value);
			va_end(args);
			if (value >= 0)
				return (size_t)value;

			return std::make_error_condition(std::errc::broken_pipe);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<size_t> gz_stream::write(const uint8_t* data, size_t length)
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
#ifdef VI_ZLIB
			VI_MEASURE(timings::file_system);
			VI_TRACE("[gz] fd %i write %i bytes", get_writeable_fd(), (int)length);
			int value = gzwrite((gzFile)io_stream, data, (uint32_t)length);
			if (value >= 0)
				return (size_t)value;

			return std::make_error_condition(std::errc::broken_pipe);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<size_t> gz_stream::tell()
		{
			VI_ASSERT(io_stream != nullptr, "file should be opened");
#ifdef VI_ZLIB
			VI_MEASURE(timings::file_system);
			VI_TRACE("[gz] fd %i tell", get_readable_fd());
			long value = gztell((gzFile)io_stream);
			if (value >= 0)
				return (size_t)value;

			return std::make_error_condition(std::errc::broken_pipe);
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		socket_t gz_stream::get_readable_fd() const
		{
			return (socket_t)(uintptr_t)io_stream;
		}
		socket_t gz_stream::get_writeable_fd() const
		{
			return (socket_t)(uintptr_t)io_stream;
		}
		void* gz_stream::get_readable() const
		{
			return io_stream;
		}
		void* gz_stream::get_writeable() const
		{
			return io_stream;
		}
		bool gz_stream::is_sized() const
		{
			return false;
		}

		web_stream::web_stream(bool is_async) noexcept : output_stream(nullptr), offset(0), length(0), async(is_async)
		{
		}
		web_stream::web_stream(bool is_async, unordered_map<string, string>&& new_headers) noexcept : web_stream(is_async)
		{
			headers = std::move(new_headers);
		}
		web_stream::~web_stream() noexcept
		{
			close();
		}
		expects_io<void> web_stream::clear()
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		expects_io<void> web_stream::open(const std::string_view& file, file_mode mode)
		{
			VI_ASSERT(!file.empty(), "filename should be set");
			auto result = close();
			if (!result)
				return result;

			network::location origin(file);
			if (origin.protocol != "http" && origin.protocol != "https")
				return std::make_error_condition(std::errc::address_family_not_supported);

			bool secure = (origin.protocol == "https");
			if (secure && !os::control::has(access_option::https))
				return std::make_error_condition(std::errc::permission_denied);
			else if (!secure && !os::control::has(access_option::http))
				return std::make_error_condition(std::errc::permission_denied);

			auto* dns = network::dns::get();
			auto address = dns->lookup(origin.hostname, origin.port > 0 ? to_string(origin.port) : (secure ? "443" : "80"), network::dns_type::connect);
			if (!address)
				return address.error().error();

			core::uptr<network::http::client> client = new network::http::client(30000);
			auto status = client->connect_sync(*address, secure ? network::PEER_VERITY_DEFAULT : network::PEER_NOT_SECURE).get();
			if (!status)
				return status.error().error();

			network::http::request_frame request;
			request.location.assign(origin.path);
			VI_DEBUG("[ws] open ro %.*s", (int)file.size(), file.data());

			for (auto& item : origin.query)
				request.query += item.first + "=" + item.second + "&";

			if (!request.query.empty())
				request.query.pop_back();

			for (auto& item : headers)
				request.set_header(item.first, item.second);

			auto* response = client->get_response();
			status = client->send(std::move(request)).get();
			if (!status || response->status_code < 0)
				return status ? std::make_error_condition(std::errc::protocol_error) : status.error().error();
			else if (response->content.limited)
				length = response->content.length;

			output_stream = client.reset();
			open_virtual(string(file));
			return expectation::met;
		}
		expects_io<void> web_stream::close()
		{
			auto* client = (network::http::client*)output_stream;
			VI_DEBUG("[ws] close 0x%" PRIXPTR, (uintptr_t)client);
			output_stream = nullptr;
			offset = length = 0;
			chunk.clear();
			close_virtual();

			if (!client)
				return expectation::met;

			auto status = client->disconnect().get();
			memory::release(client);
			if (!status)
				return status.error().error();

			return expectation::met;
		}
		expects_io<void> web_stream::seek(file_seek mode, int64_t new_offset)
		{
			switch (mode)
			{
				case file_seek::begin:
					VI_TRACE("[ws] seek fd %i begin %" PRId64, get_readable_fd(), (int)new_offset);
					offset = new_offset;
					return expectation::met;
				case file_seek::current:
					VI_TRACE("[ws] seek fd %i move %" PRId64, get_readable_fd(), (int)new_offset);
					if (new_offset < 0)
					{
						size_t pointer = (size_t)(-new_offset);
						if (pointer > offset)
						{
							pointer -= offset;
							if (pointer > length)
								return std::make_error_condition(std::errc::result_out_of_range);

							offset = length - pointer;
						}
					}
					else
						offset += new_offset;
					return expectation::met;
				case file_seek::end:
					VI_TRACE("[ws] seek fd %i end %" PRId64, get_readable_fd(), (int)new_offset);
					offset = length - new_offset;
					return expectation::met;
				default:
					return std::make_error_condition(std::errc::not_supported);
			}
		}
		expects_io<void> web_stream::flush()
		{
			return expectation::met;
		}
		expects_io<size_t> web_stream::read_scan(const char* format, ...)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		expects_io<size_t> web_stream::read_line(char* data, size_t data_length)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		expects_io<size_t> web_stream::read(uint8_t* data, size_t data_length)
		{
			VI_ASSERT(output_stream != nullptr, "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
			VI_ASSERT(data_length > 0, "length should be greater than zero");
			VI_TRACE("[ws] fd %i read %i bytes", get_readable_fd(), (int)data_length);

			size_t result = 0;
			if (offset + data_length > chunk.size() && (chunk.size() < length || (!length && !((network::http::client*)output_stream)->get_response()->content.limited)))
			{
				auto* client = (network::http::client*)output_stream;
				auto status = client->fetch(data_length).get();
				if (!status)
					return status.error().error();

				auto* response = client->get_response();
				if (!length && response->content.limited)
				{
					length = response->content.length;
					if (!length)
						return 0;
				}

				if (response->content.data.empty())
					return 0;

				chunk.insert(chunk.end(), response->content.data.begin(), response->content.data.end());
			}

			result = std::min(data_length, chunk.size() - (size_t)offset);
			memcpy(data, chunk.data() + (size_t)offset, result);
			offset += (size_t)result;
			return result;
		}
		expects_io<size_t> web_stream::write_format(const char* format, ...)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		expects_io<size_t> web_stream::write(const uint8_t* data, size_t length)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		expects_io<size_t> web_stream::tell()
		{
			VI_TRACE("[ws] fd %i tell", get_readable_fd());
			return offset;
		}
		socket_t web_stream::get_readable_fd() const
		{
			VI_ASSERT(output_stream != nullptr, "file should be opened");
			return ((network::http::client*)output_stream)->get_stream()->get_fd();
		}
		socket_t web_stream::get_writeable_fd() const
		{
#ifndef INVALID_SOCKET
			return (socket_t)-1;
#else
			return INVALID_SOCKET;
#endif
		}
		void* web_stream::get_readable() const
		{
			return output_stream;
		}
		void* web_stream::get_writeable() const
		{
			return nullptr;
		}
		bool web_stream::is_sized() const
		{
			return true;
		}

		process_stream::process_stream() noexcept : output_stream(nullptr), input_fd(-1), exit_code(-1)
		{
		}
		process_stream::~process_stream() noexcept
		{
			close();
		}
		expects_io<void> process_stream::clear()
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		expects_io<void> process_stream::open(const std::string_view& file, file_mode mode)
		{
			VI_ASSERT(!file.empty(), "command should be set");
			auto result = close();
			if (!result)
				return result;
			else if (!os::control::has(access_option::shell))
				return std::make_error_condition(std::errc::permission_denied);

			bool readable = false;
			bool writeable = false;
			switch (mode)
			{
				case file_mode::read_only:
				case file_mode::binary_read_only:
					readable = true;
					break;
				case file_mode::write_only:
				case file_mode::binary_write_only:
				case file_mode::append_only:
				case file_mode::binary_append_only:
					writeable = true;
					break;
				case file_mode::read_write:
				case file_mode::binary_read_write:
				case file_mode::write_read:
				case file_mode::binary_write_read:
				case file_mode::read_append_write:
				case file_mode::binary_read_append_write:
					readable = true;
					writeable = true;
					break;
				default:
					break;
			}

			VI_PANIC(readable || writeable, "file open cannot be issued with mode:%i", (int)mode);
			internal.error_exit_code = compute::crypto::random() % std::numeric_limits<int>::max();
			auto shell = os::process::get_shell();
			if (!shell)
				return shell.error();
#ifdef VI_MICROSOFT
			SECURITY_ATTRIBUTES pipe_policy = { 0 };
			pipe_policy.nLength = sizeof(SECURITY_ATTRIBUTES);
			pipe_policy.lpSecurityDescriptor = nullptr;
			pipe_policy.bInheritHandle = TRUE;

			HANDLE output_readable = nullptr, output_writeable = nullptr;
			if (readable)
			{
				if (CreatePipe(&output_readable, &output_writeable, &pipe_policy, 0) == FALSE)
				{
				output_error:
					auto condition = os::error::get_condition_or();
					if (output_readable != nullptr && output_readable != INVALID_HANDLE_VALUE)
						CloseHandle(output_readable);
					if (output_writeable != nullptr && output_writeable != INVALID_HANDLE_VALUE)
						CloseHandle(output_writeable);
					output_stream = nullptr;
					return condition;
				}

				output_stream = _fdopen(_open_osfhandle((intptr_t)output_readable, _O_RDONLY | _O_BINARY), "rb");
				if (!output_stream)
					goto output_error;

				VI_DEBUG("[sh] open ro:pipe fd %i", VI_FILENO(output_stream));
			}

			HANDLE input_readable = nullptr, input_writeable = nullptr;
			if (writeable)
			{
				if (CreatePipe(&input_readable, &input_writeable, &pipe_policy, 0) == FALSE)
				{
				input_error:
					auto condition = os::error::get_condition_or();
					if (output_readable != nullptr && output_readable != INVALID_HANDLE_VALUE)
						CloseHandle(output_readable);
					if (output_writeable != nullptr && output_writeable != INVALID_HANDLE_VALUE)
						CloseHandle(output_writeable);
					if (input_readable != nullptr && input_readable != INVALID_HANDLE_VALUE)
						CloseHandle(input_readable);
					if (input_writeable != nullptr && input_writeable != INVALID_HANDLE_VALUE)
						CloseHandle(input_writeable);
					output_stream = nullptr;
					input_fd = -1;
					return condition;
				}

				input_fd = _open_osfhandle((intptr_t)input_writeable, _O_WRONLY | _O_BINARY);
				if (input_fd < 0)
					goto input_error;

				VI_DEBUG("[sh] open wo:pipe fd %i", input_fd);
			}

			STARTUPINFO startup_policy;
			ZeroMemory(&startup_policy, sizeof(STARTUPINFO));
			startup_policy.cb = sizeof(STARTUPINFO);
			startup_policy.hStdError = output_writeable;
			startup_policy.hStdOutput = output_writeable;
			startup_policy.hStdInput = input_readable;
			startup_policy.dwFlags |= STARTF_USESTDHANDLES;

			string executable = "/c ";
			executable += file;

			PROCESS_INFORMATION process_info = { 0 };
			if (CreateProcessA(shell->c_str(), (LPSTR)executable.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup_policy, &process_info) == FALSE)
				goto input_error;

			VI_DEBUG("[sh] spawn piped process pid %i (tid = %i)", (int)process_info.dwProcessId, (int)process_info.dwThreadId);
			internal.output_pipe = output_readable;
			internal.input_pipe = input_writeable;
			internal.process = process_info.hProcess;
			internal.thread = process_info.hThread;
			internal.process_id = (socket_t)process_info.dwProcessId;
			internal.thread_id = (socket_t)process_info.dwThreadId;
			if (output_writeable != nullptr && output_writeable != INVALID_HANDLE_VALUE)
				CloseHandle(output_writeable);
			if (input_readable != nullptr && input_readable != INVALID_HANDLE_VALUE)
				CloseHandle(input_readable);
#else
			int input_pipe[2] = { 0, 0 };
			if (writeable)
			{
				if (pipe(input_pipe) != 0)
				{
					auto condition = os::error::get_condition_or();
					if (output_stream != nullptr)
						fclose(output_stream);
					if (input_pipe[0] > 0)
						::close(input_pipe[0]);
					if (input_pipe[1] > 0)
						::close(input_pipe[1]);
					return condition;
				}

				input_fd = input_pipe[1];
				VI_DEBUG("[sh] open writeable wo:pipe fd %i", input_fd);
			}

			int output_pipe[2] = { 0, 0 };
			if (readable)
			{
				if (pipe(output_pipe) != 0)
				{
				pipe_error:
					auto condition = os::error::get_condition_or();
					if (input_pipe[0] > 0)
						::close(input_pipe[0]);
					if (input_pipe[1] > 0)
						::close(input_pipe[1]);
					if (output_pipe[0] > 0)
						::close(output_pipe[0]);
					if (output_pipe[1] > 0)
						::close(output_pipe[1]);
					if (output_stream != nullptr)
						fclose(output_stream);
					return condition;
				}

				output_stream = fdopen(output_pipe[0], "r");
				if (!output_stream)
					goto pipe_error;

				VI_DEBUG("[sh] open readable ro:pipe fd %i", VI_FILENO(output_stream));
			}

			pid_t process_id = fork();
			if (process_id < 0)
				goto pipe_error;

			string executable = string(file);
			int error_exit_code = internal.error_exit_code;
			if (process_id == 0)
			{
				::close(0);
				::close(1);
				dup2(input_pipe[0], 0);
				dup2(output_pipe[1], 1);
				execl(shell->c_str(), "sh", "-c", executable.c_str(), nullptr);
				exit(error_exit_code);
				return std::make_error_condition(std::errc::broken_pipe);
			}
			else
			{
				VI_DEBUG("[sh] spawn piped process pid %i", (int)process_id);
				internal.input_pipe = (void*)(uintptr_t)input_pipe[1];
				internal.output_pipe = (void*)(uintptr_t)output_pipe[0];
				internal.process = (void*)(uintptr_t)process_id;
				internal.thread = (void*)(uintptr_t)process_id;
				internal.process_id = (socket_t)process_id;
				internal.thread_id = (socket_t)process_id;
				if (input_pipe[0] > 0)
					::close(input_pipe[0]);
				if (output_pipe[1] > 0)
					::close(output_pipe[1]);
			}
#endif
			open_virtual(string(file));
			return expectation::met;
		}
		expects_io<void> process_stream::close()
		{
			close_virtual();
#ifdef VI_MICROSOFT
			if (internal.thread != nullptr)
			{
				VI_TRACE("[sh] close thread tid %i (wait)", (int)internal.thread_id);
				WaitForSingleObject((HANDLE)internal.thread, INFINITE);
				CloseHandle((HANDLE)internal.thread);
			}
			if (internal.process != nullptr)
			{
				VI_DEBUG("[sh] close process pid %i (wait)", (int)internal.thread_id);
				WaitForSingleObject((HANDLE)internal.process, INFINITE);

				DWORD status_code = 0;
				if (GetExitCodeProcess((HANDLE)internal.process, &status_code) == TRUE)
					exit_code = (int)status_code;

				CloseHandle((HANDLE)internal.process);
			}
#else
			if (internal.process != nullptr)
			{
				VI_DEBUG("[sh] close process pid %i (wait)", (int)internal.thread_id);
				while (waitpid((pid_t)internal.process_id, &exit_code, WNOHANG) != -1)
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				exit_code = WEXITSTATUS(exit_code);
			}
#endif
			if (output_stream != nullptr)
			{
#ifndef VI_MICROSOFT
				int fd = VI_FILENO(output_stream);
				os::file::close(output_stream);
				if (fd > 0)
					::close(fd);
#else
				os::file::close(output_stream);
#endif
				output_stream = nullptr;
			}
			if (input_fd > 0)
			{
				VI_DEBUG("[sh] close fd %i", input_fd);
				::close(input_fd);
				input_fd = -1;
			}
			memset(&internal, 0, sizeof(internal));
			return expectation::met;
		}
		expects_io<void> process_stream::seek(file_seek mode, int64_t offset)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		expects_io<void> process_stream::flush()
		{
			return expectation::met;
		}
		expects_io<size_t> process_stream::read_scan(const char* format, ...)
		{
			VI_ASSERT(output_stream != nullptr, "file should be opened");
			VI_ASSERT(format != nullptr, "format should be set");
			VI_MEASURE(timings::file_system);

			va_list args;
			va_start(args, format);
			int value = vfscanf(output_stream, format, args);
			VI_TRACE("[sh] fd %i scan %i bytes", get_readable_fd(), value);
			va_end(args);
			if (value >= 0)
				return (size_t)value;

			return os::error::get_condition_or(std::errc::broken_pipe);
		}
		expects_io<size_t> process_stream::read_line(char* data, size_t length)
		{
			VI_ASSERT(output_stream != nullptr, "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
			VI_MEASURE(timings::file_system);
			VI_TRACE("[sh] fd %i readln %i bytes", get_readable_fd(), (int)length);
			return fgets(data, (int)length, output_stream) ? strnlen(data, length) : 0;
		}
		expects_io<size_t> process_stream::read(uint8_t* data, size_t length)
		{
			VI_ASSERT(output_stream != nullptr, "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
			VI_MEASURE(timings::file_system);
			VI_TRACE("[sh] fd %i read %i bytes", get_readable_fd(), (int)length);
			size_t value = fread(data, 1, length, output_stream);
			if (value > 0 || feof(output_stream) != 0)
				return (size_t)value;

			return os::error::get_condition_or(std::errc::broken_pipe);
		}
		expects_io<size_t> process_stream::write_format(const char* format, ...)
		{
			return std::make_error_condition(std::errc::not_supported);
		}
		expects_io<size_t> process_stream::write(const uint8_t* data, size_t length)
		{
			VI_ASSERT(input_fd > 0, "file should be opened");
			VI_ASSERT(data != nullptr, "data should be set");
			VI_MEASURE(timings::file_system);
			VI_TRACE("[sh] fd %i write %i bytes", get_writeable_fd(), (int)length);
			int value = (int)::write(input_fd, data, length);
			if (value >= 0)
				return (size_t)value;

			return os::error::get_condition_or(std::errc::broken_pipe);
		}
		expects_io<size_t> process_stream::tell()
		{
			VI_ASSERT(output_stream != nullptr, "file should be opened");
			VI_MEASURE(timings::file_system);
			return os::file::tell64(output_stream);
		}
		socket_t process_stream::get_process_id() const
		{
			return internal.process_id;
		}
		socket_t process_stream::get_thread_id() const
		{
			return internal.thread_id;
		}
		socket_t process_stream::get_readable_fd() const
		{
			VI_ASSERT(output_stream != nullptr, "file should be opened");
			return (socket_t)VI_FILENO(output_stream);
		}
		socket_t process_stream::get_writeable_fd() const
		{
			return (socket_t)input_fd;
		}
		void* process_stream::get_readable() const
		{
			return output_stream;
		}
		void* process_stream::get_writeable() const
		{
			return (void*)(uintptr_t)input_fd;
		}
		bool process_stream::is_sized() const
		{
			return false;
		}
		bool process_stream::is_alive()
		{
			if (!internal.process)
				return false;
#ifdef VI_MICROSOFT
			return WaitForSingleObject((HANDLE)internal.process, 0) == WAIT_TIMEOUT;
#else
			return waitpid((pid_t)(uintptr_t)internal.process, &exit_code, WNOHANG) == 0;
#endif
		}
		int process_stream::get_exit_code() const
		{
			return exit_code;
		}

		file_tree::file_tree(const std::string_view& folder) noexcept
		{
			auto target = os::path::resolve(folder);
			if (!target)
				return;

			vector<std::pair<string, file_entry>> entries;
			if (!os::directory::scan(target->c_str(), entries))
				return;

			directories.reserve(entries.size());
			files.reserve(entries.size());
			path = *target;

			for (auto& item : entries)
			{
				if (item.second.is_directory)
					directories.push_back(new file_tree(*target + VI_SPLITTER + item.first));
				else
					files.emplace_back(std::move(item.first));
			}
		}
		file_tree::~file_tree() noexcept
		{
			for (auto& directory : directories)
				memory::release(directory);
		}
		void file_tree::loop(const std::function<bool(const file_tree*)>& callback) const
		{
			VI_ASSERT(callback, "callback should not be empty");
			if (!callback(this))
				return;

			for (auto& directory : directories)
				directory->loop(callback);
		}
		const file_tree* file_tree::find(const std::string_view& v) const
		{
			if (path == v)
				return this;

			for (const auto& directory : directories)
			{
				const file_tree* ref = directory->find(v);
				if (ref != nullptr)
					return ref;
			}

			return nullptr;
		}
		size_t file_tree::get_files() const
		{
			size_t count = files.size();
			for (auto& directory : directories)
				count += directory->get_files();

			return count;
		}

		inline_args::inline_args() noexcept
		{
		}
		bool inline_args::is_enabled(const std::string_view& option, const std::string_view& shortcut) const
		{
			auto it = args.find(key_lookup_cast(option));
			if (it == args.end() || !is_true(it->second))
				return shortcut.empty() ? false : is_enabled(shortcut);

			return true;
		}
		bool inline_args::is_disabled(const std::string_view& option, const std::string_view& shortcut) const
		{
			auto it = args.find(key_lookup_cast(option));
			if (it == args.end())
				return shortcut.empty() ? true : is_disabled(shortcut);

			return is_false(it->second);
		}
		bool inline_args::has(const std::string_view& option, const std::string_view& shortcut) const
		{
			if (args.find(key_lookup_cast(option)) != args.end())
				return true;

			return shortcut.empty() ? false : args.find(key_lookup_cast(shortcut)) != args.end();
		}
		string& inline_args::get(const std::string_view& option, const std::string_view& shortcut)
		{
			auto it = args.find(key_lookup_cast(option));
			if (it != args.end())
				return it->second;
			else if (shortcut.empty())
				return args[string(option)];

			it = args.find(key_lookup_cast(shortcut));
			if (it != args.end())
				return it->second;

			return args[string(shortcut)];
		}
		string& inline_args::get_if(const std::string_view& option, const std::string_view& shortcut, const std::string_view& when_empty)
		{
			auto it = args.find(key_lookup_cast(option));
			if (it != args.end())
				return it->second;

			if (!shortcut.empty())
			{
				it = args.find(key_lookup_cast(shortcut));
				if (it != args.end())
					return it->second;
			}

			string& value = args[string(option)];
			value = when_empty;
			return value;
		}
		bool inline_args::is_true(const std::string_view& value) const
		{
			if (value.empty())
				return false;

			auto maybe_number = from_string<uint64_t>(value);
			if (maybe_number && *maybe_number > 0)
				return true;

			string data(value);
			stringify::to_lower(data);
			return data == "on" || data == "true" || data == "yes" || data == "y";
		}
		bool inline_args::is_false(const std::string_view& value) const
		{
			if (value.empty())
				return true;

			auto maybe_number = from_string<uint64_t>(value);
			if (maybe_number && *maybe_number > 0)
				return false;

			string data(value);
			stringify::to_lower(data);
			return data == "off" || data == "false" || data == "no" || data == "n";
		}

		os::hw::quantity_info os::hw::get_quantity_info()
		{
			quantity_info result { };
#ifdef VI_MICROSOFT
			for (auto&& info : cpu_info_buffer())
			{
				switch (info.Relationship)
				{
					case RelationProcessorCore:
						++result.physical;
						result.logical += static_cast<std::uint32_t>(std::bitset<sizeof(ULONG_PTR) * 8>(static_cast<std::uintptr_t>(info.ProcessorMask)).count());
						break;
					case RelationProcessorPackage:
						++result.packages;
						break;
					default:
						break;
				}
			}
#elif VI_APPLE
			const auto ctl_thread_data = sys_control("machdep.cpu.thread_count");
			if (!ctl_thread_data.empty())
			{
				const auto thread_data = sys_extract(ctl_thread_data);
				if (thread_data.first)
					result.logical = (uint32_t)thread_data.second;
			}

			const auto ctl_core_data = sys_control("machdep.cpu.core_count");
			if (!ctl_core_data.empty())
			{
				const auto core_data = sys_extract(ctl_core_data);
				if (core_data.first)
					result.physical = (uint32_t)core_data.second;
			}

			const auto ctl_packages_data = sys_control("hw.packages");
			if (!ctl_packages_data.empty())
			{
				const auto packages_data = sys_extract(ctl_packages_data);
				if (packages_data.first)
					result.packages = (uint32_t)packages_data.second;
			}
#else
			result.logical = sysconf(_SC_NPROCESSORS_ONLN);
			std::ifstream info("/proc/cpuinfo");

			if (!info.is_open() || !info)
				return result;

			vector<uint32_t> packages;
			for (string line; std::getline(info, line);)
			{
				if (line.find("physical id") == 0)
				{
					const auto physical_id = std::strtoul(line.c_str() + line.find_first_of("1234567890"), nullptr, 10);
					if (std::find(packages.begin(), packages.end(), physical_id) == packages.end())
						packages.emplace_back(physical_id);
				}
			}

			result.packages = packages.size();
			result.physical = result.logical / result.packages;
#endif
			return result;
		}
		os::hw::cache_info os::hw::get_cache_info(uint32_t level)
		{
#ifdef VI_MICROSOFT
			for (auto&& info : cpu_info_buffer())
			{
				if (info.Relationship != RelationCache || info.Cache.Level != level)
					continue;

				cache type { };
				switch (info.Cache.Type)
				{
					case CacheUnified:
						type = cache::unified;
						break;
					case CacheInstruction:
						type = cache::instruction;
						break;
					case CacheData:
						type = cache::data;
						break;
					case CacheTrace:
						type = cache::trace;
						break;
				}

				return { info.Cache.Size, info.Cache.LineSize, info.Cache.Associativity, type };
			}

			return { };
#elif VI_APPLE
			static const char* size_keys[][3] { { }, { "hw.l1icachesize", "hw.l1dcachesize", "hw.l1cachesize" }, { "hw.l2cachesize" }, { "hw.l3cachesize" } };
			cache_info result { };

			const auto ctl_cache_line_size = sys_control("hw.cachelinesize");
			if (!ctl_cache_line_size.empty())
			{
				const auto cache_line_size = sys_extract(ctl_cache_line_size);
				if (cache_line_size.first)
					result.line_size = cache_line_size.second;
			}

			if (level < sizeof(size_keys) / sizeof(*size_keys))
			{
				for (auto key : size_keys[level])
				{
					if (!key)
						break;

					const auto ctl_cache_size_data = sys_control(key);
					if (!ctl_cache_size_data.empty())
					{
						const auto cache_size_data = sys_extract(ctl_cache_size_data);
						if (cache_size_data.first)
							result.size += cache_size_data.second;
					}
				}
			}

			return result;
#else
			string prefix("/sys/devices/system/cpu/cpu0/cache/index" + core::to_string(level) + '/');
			string size_path(prefix + "size");
			string line_size_path(prefix + "coherency_line_size");
			string associativity_path(prefix + "associativity");
			string type_path(prefix + "type");
			std::ifstream size(size_path.c_str());
			std::ifstream line_size(line_size_path.c_str());
			std::ifstream associativity(associativity_path.c_str());
			std::ifstream type(type_path.c_str());
			cache_info result { };

			if (size.is_open() && size)
			{
				char suffix;
				size >> result.size >> suffix;
				switch (suffix)
				{
					case 'g':
						result.size *= 1024;
						[[fallthrough]];
					case 'm':
						result.size *= 1024;
						[[fallthrough]];
					case 'k':
						result.size *= 1024;
				}
			}

			if (line_size.is_open() && line_size)
				line_size >> result.line_size;

			if (associativity.is_open() && associativity)
			{
				uint32_t temp;
				associativity >> temp;
				result.associativity = temp;
			}

			if (type.is_open() && type)
			{
				string temp;
				type >> temp;

				if (temp.find("nified") == 1)
					result.type = cache::unified;
				else if (temp.find("nstruction") == 1)
					result.type = cache::instruction;
				else if (temp.find("ata") == 1)
					result.type = cache::data;
				else if (temp.find("race") == 1)
					result.type = cache::trace;
			}

			return result;
#endif
		}
		os::hw::arch os::hw::get_arch() noexcept
		{
#ifndef VI_MICROSOFT
			utsname buffer;
			if (uname(&buffer) == -1)
				return arch::unknown;

			if (!strcmp(buffer.machine, "x86_64"))
				return arch::x64;
			else if (strstr(buffer.machine, "arm") == buffer.machine)
				return arch::arm;
			else if (!strcmp(buffer.machine, "ia64") || !strcmp(buffer.machine, "IA64"))
				return arch::itanium;
			else if (!strcmp(buffer.machine, "i686"))
				return arch::x86;

			return arch::unknown;
#else
			SYSTEM_INFO buffer;
			GetNativeSystemInfo(&buffer);

			switch (buffer.wProcessorArchitecture)
			{
				case PROCESSOR_ARCHITECTURE_AMD64:
					return arch::x64;
				case PROCESSOR_ARCHITECTURE_ARM:
				case PROCESSOR_ARCHITECTURE_ARM64:
					return arch::arm;
				case PROCESSOR_ARCHITECTURE_IA64:
					return arch::itanium;
				case PROCESSOR_ARCHITECTURE_INTEL:
					return arch::x86;
				default:
					return arch::unknown;
			}
#endif
		}
		os::hw::endian os::hw::get_endianness() noexcept
		{
			static const uint16_t value = 0xFF00;
			static const uint8_t result = *static_cast<const uint8_t*>(static_cast<const void*>(&value));

			return result == 0xFF ? endian::big : endian::little;
		}
		size_t os::hw::get_frequency() noexcept
		{
#ifdef VI_MICROSOFT
			HKEY key;
			if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(HARDWARE\DESCRIPTION\system\central_processor\0)", 0, KEY_READ, &key))
			{
				LARGE_INTEGER frequency;
				QueryPerformanceFrequency(&frequency);
				return (size_t)frequency.QuadPart * 1000;
			}

			DWORD frequency_mhz, size = sizeof(DWORD);
			if (RegQueryValueExA(key, "~MHz", nullptr, nullptr, static_cast<LPBYTE>(static_cast<void*>(&frequency_mhz)), &size))
				return 0;

			return (size_t)frequency_mhz * 1000000;
#elif VI_APPLE
			const auto frequency = sys_control("hw.cpufrequency");
			if (frequency.empty())
				return 0;

			const auto data = sys_extract(frequency);
			if (!data.first)
				return 0;

			return data.second;
#else
			std::ifstream info("/proc/cpuinfo");
			if (!info.is_open() || !info)
				return 0;

			for (string line; std::getline(info, line);)
			{
				if (line.find("cpu MHz") == 0)
				{
					const auto colon_id = line.find_first_of(':');
					return static_cast<size_t>(std::strtod(line.c_str() + colon_id + 1, nullptr)) * 1000000;
				}
			}

			return 0;
#endif
		}

		bool os::directory::is_exists(const std::string_view& path)
		{
			VI_MEASURE(timings::file_system);
			VI_TRACE("[io] check path %.*s", (int)path.size(), path.data());
			if (!control::has(access_option::fs))
				return false;

			auto target_path = path::resolve(path);
			if (!target_path)
				return false;

			struct stat buffer;
			if (stat(target_path->c_str(), &buffer) != 0)
				return false;

			return buffer.st_mode & S_IFDIR;
		}
		bool os::directory::is_empty(const std::string_view& path)
		{
			VI_MEASURE(timings::file_system);
			VI_TRACE("[io] check dir %.*s", (int)path.size(), path.data());
			if (path.empty() || !control::has(access_option::fs))
				return true;
#if defined(VI_MICROSOFT)
			wchar_t buffer[CHUNK_SIZE];
			stringify::convert_to_wide(path, buffer, CHUNK_SIZE);

			DWORD attributes = GetFileAttributesW(buffer);
			if (attributes == 0xFFFFFFFF || (attributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
				return true;

			if (path[0] != '\\' && path[0] != '/')
				wcscat(buffer, L"\\*");
			else
				wcscat(buffer, L"*");

			WIN32_FIND_DATAW info;
			HANDLE handle = FindFirstFileW(buffer, &info);
			if (!handle)
				return true;

			while (FindNextFileW(handle, &info) == TRUE)
			{
				if (wcscmp(info.cFileName, L".") != 0 && wcscmp(info.cFileName, L"..") != 0)
				{
					FindClose(handle);
					return false;
				}
			}

			FindClose(handle);
			return true;
#else
			DIR* handle = opendir(path.data());
			if (!handle)
				return true;

			dirent* next = nullptr;
			while ((next = readdir(handle)) != nullptr)
			{
				if (strcmp(next->d_name, ".") != 0 && strcmp(next->d_name, "..") != 0)
				{
					closedir(handle);
					return false;
				}
			}

			closedir(handle);
			return true;
#endif
		}
		expects_io<void> os::directory::set_working(const std::string_view& path)
		{
			VI_TRACE("[io] apply working dir %.*s", (int)path.size(), path.data());
#ifdef VI_MICROSOFT
			if (SetCurrentDirectoryA(path.data()) != TRUE)
				return os::error::get_condition_or();

			return expectation::met;
#elif defined(VI_LINUX)
			if (chdir(path.data()) != 0)
				return os::error::get_condition_or();

			return expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<void> os::directory::patch(const std::string_view& path)
		{
			if (is_exists(path.data()))
				return expectation::met;

			return create(path.data());
		}
		expects_io<void> os::directory::scan(const std::string_view& path, vector<std::pair<string, file_entry>>& entries)
		{
			VI_MEASURE(timings::file_system);
			VI_TRACE("[io] scan dir %.*s", (int)path.size(), path.data());
			if (!control::has(access_option::fs))
				return std::make_error_condition(std::errc::permission_denied);

			if (path.empty())
				return std::make_error_condition(std::errc::no_such_file_or_directory);
#if defined(VI_MICROSOFT)
			wchar_t buffer[CHUNK_SIZE];
			stringify::convert_to_wide(path, buffer, CHUNK_SIZE);

			DWORD attributes = GetFileAttributesW(buffer);
			if (attributes == 0xFFFFFFFF || (attributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
				return std::make_error_condition(std::errc::not_a_directory);

			if (path.back() != '\\' && path.back() != '/')
				wcscat(buffer, L"\\*");
			else
				wcscat(buffer, L"*");

			WIN32_FIND_DATAW info;
			HANDLE handle = FindFirstFileW(buffer, &info);
			if (!handle)
				return std::make_error_condition(std::errc::no_such_file_or_directory);

			do
			{
				char directory[CHUNK_SIZE] = { 0 };
				WideCharToMultiByte(CP_UTF8, 0, info.cFileName, -1, directory, sizeof(directory), nullptr, nullptr);
				if (strcmp(directory, ".") != 0 && strcmp(directory, "..") != 0)
				{
					auto state = file::get_state(string(path) + '/' + directory);
					if (state)
						entries.push_back(std::make_pair<string, file_entry>(directory, std::move(*state)));
				}
			} while (FindNextFileW(handle, &info) == TRUE);
			FindClose(handle);
#else
			DIR* handle = opendir(path.data());
			if (!handle)
				return os::error::get_condition_or();

			dirent* next = nullptr;
			while ((next = readdir(handle)) != nullptr)
			{
				if (strcmp(next->d_name, ".") != 0 && strcmp(next->d_name, "..") != 0)
				{
					auto state = file::get_state(string(path) + '/' + next->d_name);
					if (state)
						entries.push_back(std::make_pair<string, file_entry>(next->d_name, std::move(*state)));
				}
			}
			closedir(handle);
#endif
			return expectation::met;
		}
		expects_io<void> os::directory::create(const std::string_view& path)
		{
			VI_MEASURE(timings::file_system);
			VI_DEBUG("[io] create dir %.*s", (int)path.size(), path.data());
			if (!control::has(access_option::fs))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			wchar_t buffer[CHUNK_SIZE];
			stringify::convert_to_wide(path, buffer, CHUNK_SIZE);

			size_t length = wcslen(buffer);
			if (!length)
				return std::make_error_condition(std::errc::invalid_argument);

			if (::CreateDirectoryW(buffer, nullptr) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS)
				return expectation::met;

			size_t index = length - 1;
			while (index > 0 && (buffer[index] == '/' || buffer[index] == '\\'))
				index--;

			while (index > 0 && buffer[index] != '/' && buffer[index] != '\\')
				index--;

			string subpath(path.data(), index);
			if (index > 0 && !create(subpath.c_str()))
				return os::error::get_condition_or();

			if (::CreateDirectoryW(buffer, nullptr) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS)
				return expectation::met;
#else
			if (mkdir(path.data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST)
				return expectation::met;

			size_t index = path.empty() ? 0 : path.size() - 1;
			while (index > 0 && path[index] != '/' && path[index] != '\\')
				index--;

			string subpath(path.data(), index);
			if (index > 0 && !create(subpath.c_str()))
				return os::error::get_condition_or();

			if (mkdir(path.data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != -1 || errno == EEXIST)
				return expectation::met;
#endif
			return os::error::get_condition_or();
		}
		expects_io<void> os::directory::remove(const std::string_view& path)
		{
			VI_MEASURE(timings::file_system);
			VI_DEBUG("[io] remove dir %.*s", (int)path.size(), path.data());
			if (!control::has(access_option::fs))
				return std::make_error_condition(std::errc::permission_denied);

			string target_path = string(path);
			if (!target_path.empty() && (target_path.back() == '/' || target_path.back() == '\\'))
				target_path.pop_back();
#ifdef VI_MICROSOFT
			WIN32_FIND_DATA file_information;
			string file_path, pattern = target_path + "\\*.*";
			HANDLE handle = ::FindFirstFile(pattern.c_str(), &file_information);

			if (handle == INVALID_HANDLE_VALUE)
			{
				auto condition = os::error::get_condition_or();
				::FindClose(handle);
				return condition;
			}

			do
			{
				if (!strcmp(file_information.cFileName, ".") || !strcmp(file_information.cFileName, ".."))
					continue;

				file_path = target_path + "\\" + file_information.cFileName;
				if (file_information.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					auto result = remove(file_path.c_str());
					if (result)
						continue;

					::FindClose(handle);
					return result;
				}

				if (::SetFileAttributes(file_path.c_str(), FILE_ATTRIBUTE_NORMAL) == FALSE)
				{
					auto condition = os::error::get_condition_or();
					::FindClose(handle);
					return condition;
				}

				if (::DeleteFile(file_path.c_str()) == FALSE)
				{
					auto condition = os::error::get_condition_or();
					::FindClose(handle);
					return condition;
				}
			} while (::FindNextFile(handle, &file_information) != FALSE);

			::FindClose(handle);
			if (::GetLastError() != ERROR_NO_MORE_FILES)
				return os::error::get_condition_or();

			if (::SetFileAttributes(target_path.data(), FILE_ATTRIBUTE_NORMAL) == FALSE || ::RemoveDirectory(target_path.data()) == FALSE)
				return os::error::get_condition_or();
#elif defined VI_LINUX
			DIR* handle = opendir(target_path.c_str());
			size_t size = target_path.size();

			if (!handle)
			{
				if (rmdir(target_path.c_str()) != 0)
					return os::error::get_condition_or();

				return expectation::met;
			}

			struct dirent* it;
			while ((it = readdir(handle)))
			{
				if (!strcmp(it->d_name, ".") || !strcmp(it->d_name, ".."))
					continue;

				size_t length = size + strlen(it->d_name) + 2;
				char* buffer = memory::allocate<char>(length);
				snprintf(buffer, length, "%.*s/%s", (int)target_path.size(), target_path.c_str(), it->d_name);

				struct stat state;
				if (stat(buffer, &state) != 0)
				{
					auto condition = os::error::get_condition_or();
					memory::deallocate(buffer);
					closedir(handle);
					return condition;
				}

				if (S_ISDIR(state.st_mode))
				{
					auto result = remove(buffer);
					memory::deallocate(buffer);
					if (result)
						continue;

					closedir(handle);
					return result;
				}

				if (unlink(buffer) != 0)
				{
					auto condition = os::error::get_condition_or();
					memory::deallocate(buffer);
					closedir(handle);
					return condition;
				}

				memory::deallocate(buffer);
			}

			closedir(handle);
			if (rmdir(target_path.c_str()) != 0)
				return os::error::get_condition_or();
#endif
			return expectation::met;
		}
		expects_io<string> os::directory::get_module()
		{
			VI_MEASURE(timings::file_system);
#ifdef VI_MICROSOFT
			char buffer[MAX_PATH + 1] = { };
			if (GetModuleFileNameA(nullptr, buffer, MAX_PATH) == 0)
				return os::error::get_condition_or();
#else
#ifdef PATH_MAX
			char buffer[PATH_MAX + 1] = { };
#elif defined(_POSIX_PATH_MAX)
			char buffer[_POSIX_PATH_MAX + 1] = { };
#else
			char buffer[CHUNK_SIZE + 1] = { };
#endif
#ifdef VI_APPLE
			uint32_t buffer_size = sizeof(buffer) - 1;
			if (_NSGetExecutablePath(buffer, &buffer_size) != 0)
				return os::error::get_condition_or();
#elif defined(VI_SOLARIS)
			const char* temp_buffer = getexecname();
			if (!temp_buffer || *temp_buffer == '\0')
				return os::error::get_condition_or();

			temp_buffer = os::path::get_directory(temp_buffer);
			size_t temp_buffer_size = strlen(temp_buffer);
			memcpy(buffer, temp_buffer, temp_buffer_size > sizeof(buffer) - 1 ? sizeof(buffer) - 1 : temp_buffer_size);
#else
			size_t buffer_size = sizeof(buffer) - 1;
			if (readlink("/proc/self/exe", buffer, buffer_size) == -1 && readlink("/proc/curproc/file", buffer, buffer_size) == -1 && readlink("/proc/curproc/exe", buffer, buffer_size) == -1)
				return os::error::get_condition_or();
#endif
#endif
			string result = os::path::get_directory(buffer);
			if (!result.empty() && result.back() != '/' && result.back() != '\\')
				result += VI_SPLITTER;

			VI_TRACE("[io] fetch library dir %s", result.c_str());
			return result;
		}
		expects_io<string> os::directory::get_working()
		{
			VI_MEASURE(timings::file_system);
#ifdef VI_MICROSOFT
			char buffer[MAX_PATH + 1] = { };
			if (GetCurrentDirectoryA(MAX_PATH, buffer) == 0)
				return os::error::get_condition_or();
#else
#ifdef PATH_MAX
			char buffer[PATH_MAX + 1] = { };
#elif defined(_POSIX_PATH_MAX)
			char buffer[_POSIX_PATH_MAX + 1] = { };
#else
			char buffer[CHUNK_SIZE + 1] = { };
#endif
			if (!getcwd(buffer, sizeof(buffer)))
				return os::error::get_condition_or();
#endif
			string result = buffer;
			if (!result.empty() && result.back() != '/' && result.back() != '\\')
				result += VI_SPLITTER;

			VI_TRACE("[io] fetch working dir %s", result.c_str());
			return result;
		}
		vector<string> os::directory::get_mounts()
		{
			VI_TRACE("[io] fetch mount points");
			vector<string> output;
#ifdef VI_MICROSOFT
			DWORD drive_mask = GetLogicalDrives();
			char offset = 'a';
			while (drive_mask)
			{
				if (drive_mask & 1)
				{
					string letter(1, offset);
					letter.append(":\\");
					output.push_back(std::move(letter));
				}
				drive_mask >>= 1;
				++offset;
			}

			return output;
#else
			output.push_back("/");
			return output;
#endif
		}

		bool os::file::is_exists(const std::string_view& path)
		{
			VI_MEASURE(timings::file_system);
			VI_TRACE("[io] check path %.*s", (int)path.size(), path.data());
			auto target_path = os::path::resolve(path);
			return target_path && is_path_exists(target_path->c_str());
		}
		int os::file::compare(const std::string_view& first_path, const std::string_view& second_path)
		{
			VI_ASSERT(!first_path.empty(), "first path should not be empty");
			VI_ASSERT(!second_path.empty(), "second path should not be empty");

			auto props1 = get_properties(first_path);
			auto props2 = get_properties(second_path);
			if (!props1 || !props2)
			{
				if (!props1 && !props2)
					return 0;
				else if (props1)
					return 1;
				else if (props2)
					return -1;
			}

			size_t size1 = props1->size, size2 = props2->size;
			VI_TRACE("[io] compare paths { %.*s (%" PRIu64 "), %.*s (%" PRIu64 ") }", (int)first_path.size(), first_path.data(), size1, (int)second_path.size(), second_path.data(), size2);

			if (size1 > size2)
				return 1;
			else if (size1 < size2)
				return -1;

			auto first = open(first_path, "rb");
			if (!first)
				return -1;

			auto second = open(second_path, "rb");
			if (!second)
			{
				os::file::close(*first);
				return -1;
			}

			const size_t size = CHUNK_SIZE;
			char buffer1[size];
			char buffer2[size];
			int diff = 0;

			do
			{
				VI_MEASURE(timings::file_system);
				size_t S1 = fread(buffer1, sizeof(char), size, (FILE*)*first);
				size_t S2 = fread(buffer2, sizeof(char), size, (FILE*)*second);
				if (S1 == S2)
				{
					if (S1 == 0)
						break;

					diff = memcmp(buffer1, buffer2, S1);
				}
				else if (S1 > S2)
					diff = 1;
				else if (S1 < S2)
					diff = -1;
			} while (diff == 0);

			os::file::close(*first);
			os::file::close(*second);
			return diff;
		}
		uint64_t os::file::get_hash(const std::string_view& data)
		{
			return compute::crypto::crc64(data);
		}
		uint64_t os::file::get_index(const std::string_view& data)
		{
			uint64_t result = 0xcbf29ce484222325;
			for (size_t i = 0; i < data.size(); i++)
			{
				result ^= data[i];
				result *= 1099511628211;
			}
			return result;
		}
		expects_io<size_t> os::file::write(const std::string_view& path, const uint8_t* data, size_t length)
		{
			VI_ASSERT(data != nullptr, "data should be set");
			VI_MEASURE(timings::file_system);
			auto status = open(path, file_mode::binary_write_only);
			if (!status)
				return status.error();

			uptr<core::stream> stream = *status;
			if (!length)
				return 0;

			return stream->write(data, length);
		}
		expects_io<void> os::file::copy(const std::string_view& from, const std::string_view& to)
		{
			VI_ASSERT(stringify::is_cstring(from) && stringify::is_cstring(to), "from and to should be set");
			VI_MEASURE(timings::file_system);
			VI_DEBUG("[io] copy file from %.*s to %.*s", (int)from.size(), from.data(), (int)to.size(), to.data());
			if (!control::has(access_option::fs))
				return std::make_error_condition(std::errc::permission_denied);

			std::ifstream source(std::string(from), std::ios::binary);
			if (!source)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			auto result = os::directory::patch(os::path::get_directory(to));
			if (!result)
				return result;

			std::ofstream destination(std::string(to), std::ios::binary);
			if (!source)
				return std::make_error_condition(std::errc::bad_file_descriptor);

			destination << source.rdbuf();
			return expectation::met;
		}
		expects_io<void> os::file::move(const std::string_view& from, const std::string_view& to)
		{
			VI_ASSERT(stringify::is_cstring(from) && stringify::is_cstring(to), "from and to should be set");
			VI_MEASURE(timings::file_system);
			VI_DEBUG("[io] move file from %.*s to %.*s", (int)from.size(), from.data(), (int)to.size(), to.data());
			if (!control::has(access_option::fs))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			if (MoveFileA(from.data(), to.data()) != TRUE)
				return os::error::get_condition_or();

			return expectation::met;
#elif defined VI_LINUX
			if (rename(from.data(), to.data()) != 0)
				return os::error::get_condition_or();

			return expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<void> os::file::remove(const std::string_view& path)
		{
			VI_ASSERT(stringify::is_cstring(path), "path should be set");
			VI_MEASURE(timings::file_system);
			VI_DEBUG("[io] remove file %.*s", (int)path.size(), path.data());
			if (!control::has(access_option::fs))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			SetFileAttributesA(path.data(), 0);
			if (DeleteFileA(path.data()) != TRUE)
				return os::error::get_condition_or();

			return expectation::met;
#elif defined VI_LINUX
			if (unlink(path.data()) != 0)
				return os::error::get_condition_or();

			return expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<void> os::file::close(FILE* stream)
		{
			VI_ASSERT(stream != nullptr, "stream should be set");
			VI_DEBUG("[io] close fd %i", VI_FILENO(stream));
			if (fclose(stream) != 0)
				return os::error::get_condition_or();

			return expectation::met;
		}
		expects_io<void> os::file::get_state(const std::string_view& path, file_entry* file)
		{
			VI_ASSERT(stringify::is_cstring(path), "path should be set");
			VI_MEASURE(timings::file_system);
			if (!control::has(access_option::fs))
				return std::make_error_condition(std::errc::permission_denied);
#if defined(VI_MICROSOFT)
			wchar_t buffer[CHUNK_SIZE];
			stringify::convert_to_wide(path, buffer, CHUNK_SIZE);

			WIN32_FILE_ATTRIBUTE_DATA info;
			if (GetFileAttributesExW(buffer, GetFileExInfoStandard, &info) == 0)
				return os::error::get_condition_or();

			file->size = MAKEUQUAD(info.nFileSizeLow, info.nFileSizeHigh);
			file->last_modified = SYS2UNIX_TIME(info.ftLastWriteTime.dwLowDateTime, info.ftLastWriteTime.dwHighDateTime);
			file->creation_time = SYS2UNIX_TIME(info.ftCreationTime.dwLowDateTime, info.ftCreationTime.dwHighDateTime);
			file->is_directory = info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			file->is_exists = file->is_referenced = false;
			if (file->creation_time > file->last_modified)
				file->last_modified = file->creation_time;
			if (file->is_directory)
				file->is_exists = true;
			else if (!path.empty())
				file->is_exists = (isalnum(path.back()) || strchr("_-", path.back()) != nullptr);
#else
			struct stat state;
			if (stat(path.data(), &state) != 0)
				return os::error::get_condition_or();

			struct tm time;
			local_time(&state.st_ctime, &time);

			file->size = (size_t)(state.st_size);
			file->last_modified = state.st_mtime;
			file->creation_time = mktime(&time);
			file->is_directory = S_ISDIR(state.st_mode);
			file->is_referenced = false;
			file->is_exists = true;
#endif
			VI_TRACE("[io] stat %.*s: %s %" PRIu64 " bytes", (int)path.size(), path.data(), file->is_directory ? "dir" : "file", (uint64_t)file->size);
			return core::expectation::met;
		}
		expects_io<void> os::file::seek64(FILE* stream, int64_t offset, file_seek mode)
		{
			int origin;
			switch (mode)
			{
				case file_seek::begin:
					VI_TRACE("[io] seek-64 fd %i begin %" PRId64, VI_FILENO(stream), offset);
					origin = SEEK_SET;
					break;
				case file_seek::current:
					VI_TRACE("[io] seek-64 fd %i move %" PRId64, VI_FILENO(stream), offset);
					origin = SEEK_CUR;
					break;
				case file_seek::end:
					VI_TRACE("[io] seek-64 fd %i end %" PRId64, VI_FILENO(stream), offset);
					origin = SEEK_END;
					break;
				default:
					return std::make_error_condition(std::errc::invalid_argument);
			}
#ifdef VI_MICROSOFT
			if (_fseeki64(stream, offset, origin) != 0)
				return os::error::get_condition_or();
#else
			if (fseeko(stream, offset, origin) != 0)
				return os::error::get_condition_or();
#endif
			return expectation::met;
		}
		expects_io<size_t> os::file::tell64(FILE* stream)
		{
			VI_TRACE("[io] fd %i tell-64", VI_FILENO(stream));
#ifdef VI_MICROSOFT
			int64_t offset = _ftelli64(stream);
#else
			int64_t offset = ftello(stream);
#endif
			if (offset < 0)
				return os::error::get_condition_or();

			return (size_t)offset;
		}
		expects_io<size_t> os::file::join(const std::string_view& to, const vector<string>& paths)
		{
			VI_ASSERT(!to.empty(), "to should not be empty");
			VI_ASSERT(!paths.empty(), "paths to join should not be empty");
			VI_TRACE("[io] join %i path to %.*s", (int)paths.size(), (int)to.size(), to.data());
			auto target = open(to, file_mode::binary_write_only);
			if (!target)
				return target.error();

			size_t total = 0;
			uptr<stream> pipe = *target;
			for (auto& path : paths)
			{
				uptr<stream> base = open(path, file_mode::binary_read_only).or_else(nullptr);
				if (base)
					total += base->read_all([&pipe](uint8_t* buffer, size_t size) { pipe->write(buffer, size); }).or_else(0);
			}

			return total;
		}
		expects_io<file_state> os::file::get_properties(const std::string_view& path)
		{
			VI_ASSERT(stringify::is_cstring(path), "path should be set");
			VI_MEASURE(timings::file_system);
			if (!control::has(access_option::fs))
				return std::make_error_condition(std::errc::permission_denied);

			struct stat buffer;
			if (stat(path.data(), &buffer) != 0)
				return os::error::get_condition_or();

			file_state state;
			state.exists = true;
			state.size = buffer.st_size;
			state.links = buffer.st_nlink;
			state.permissions = buffer.st_mode;
			state.device = buffer.st_dev;
			state.group_id = buffer.st_gid;
			state.user_id = buffer.st_uid;
			state.document = buffer.st_ino;
			state.last_access = buffer.st_atime;
			state.last_permission_change = buffer.st_ctime;
			state.last_modified = buffer.st_mtime;

			VI_TRACE("[io] stat %.*s: %" PRIu64 " bytes", (int)path.size(), path.data(), (uint64_t)state.size);
			return state;
		}
		expects_io<file_entry> os::file::get_state(const std::string_view& path)
		{
			file_entry file;
			auto status = get_state(path, &file);
			if (!status)
				return status.error();

			return file;
		}
		expects_io<FILE*> os::file::open(const std::string_view& path, const std::string_view& mode)
		{
			VI_MEASURE(timings::file_system);
			VI_ASSERT(stringify::is_cstring(path), "path should be set");
			VI_ASSERT(stringify::is_cstring(mode), "mode should be set");
			if (!control::has(access_option::fs))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			wchar_t buffer[CHUNK_SIZE], type[20];
			stringify::convert_to_wide(path, buffer, CHUNK_SIZE);
			stringify::convert_to_wide(mode, type, 20);

			FILE* stream = _wfopen(buffer, type);
			if (!stream)
				return os::error::get_condition_or();

			VI_DEBUG("[io] open %.*s:file fd %i on %.*s", (int)mode.size(), mode.data(), VI_FILENO(stream), (int)path.size(), path.data());
			return stream;
#else
			FILE* stream = fopen(path.data(), mode.data());
			if (!stream)
				return os::error::get_condition_or();

			VI_DEBUG("[io] open %.*s:file fd %i on %.*s", (int)mode.size(), mode.data(), VI_FILENO(stream), (int)path.size(), path.data());
			fcntl(VI_FILENO(stream), F_SETFD, FD_CLOEXEC);
			return stream;
#endif
		}
		expects_io<stream*> os::file::open(const std::string_view& path, file_mode mode, bool async)
		{
			if (path.empty())
				return std::make_error_condition(std::errc::no_such_file_or_directory);
			else if (!control::has(access_option::fs))
				return std::make_error_condition(std::errc::permission_denied);

			network::location origin(path);
			if (origin.protocol == "file")
			{
				uptr<stream> target;
				if (stringify::ends_with(path, ".gz"))
					target = new gz_stream();
				else
					target = new file_stream();

				auto result = target->open(path, mode);
				if (!result)
					return result.error();

				return target.reset();
			}
			else if (origin.protocol == "http" || origin.protocol == "https")
			{
				uptr<stream> target = new web_stream(async);
				auto result = target->open(path, mode);
				if (!result)
					return result.error();

				return target.reset();
			}
			else if (origin.protocol == "shell")
			{
				uptr<stream> target = new process_stream();
				auto result = target->open(origin.path.c_str(), mode);
				if (!result)
					return result.error();

				return target.reset();
			}
			else if (origin.protocol == "mem")
			{
				uptr<stream> target = new memory_stream();
				auto result = target->open(path, mode);
				if (!result)
					return result.error();

				return target.reset();
			}

			return std::make_error_condition(std::errc::invalid_argument);
		}
		expects_io<stream*> os::file::open_archive(const std::string_view& path, size_t unarchived_max_size)
		{
			auto state = os::file::get_state(path);
			if (!state)
				return open(path, file_mode::binary_write_only);

			string temp = path::get_non_existant(path);
			move(path, temp.c_str());

			if (state->size <= unarchived_max_size)
			{
				auto target = open_join(path, { temp });
				if (target)
					remove(temp.c_str());
				return target;
			}

			auto target = open(path, file_mode::binary_write_only);
			if (stringify::ends_with(temp, ".gz"))
				return target;

			uptr<stream> archive = open_join(temp + ".gz", { temp }).or_else(nullptr);
			if (archive)
				remove(temp.c_str());

			return target;
		}
		expects_io<stream*> os::file::open_join(const std::string_view& to, const vector<string>& paths)
		{
			VI_ASSERT(!to.empty(), "to should not be empty");
			VI_ASSERT(!paths.empty(), "paths to join should not be empty");
			VI_TRACE("[io] open join %i path to %.*s", (int)paths.size(), (int)to.size(), to.data());
			auto target = open(to, file_mode::binary_write_only);
			if (!target)
				return target;

			auto* channel = *target;
			for (auto& path : paths)
			{
				uptr<stream> base = open(path, file_mode::binary_read_only).or_else(nullptr);
				if (base)
					base->read_all([&channel](uint8_t* buffer, size_t size) { channel->write(buffer, size); });
			}

			return target;
		}
		expects_io<uint8_t*> os::file::read_all(const std::string_view& path, size_t* length)
		{
			auto target = open(path, file_mode::binary_read_only);
			if (!target)
				return target.error();

			uptr<stream> base = *target;
			return read_all(*base, length);
		}
		expects_io<uint8_t*> os::file::read_all(stream* stream, size_t* length)
		{
			VI_ASSERT(stream != nullptr, "path should be set");
			VI_MEASURE(core::timings::file_system);
			VI_TRACE("[io] fd %i read-all", stream->get_readable_fd());
			if (length != nullptr)
				*length = 0;

			bool is_virtual = stream->virtual_size() > 0;
			if (is_virtual || stream->is_sized())
			{
				size_t size = is_virtual ? stream->virtual_size() : stream->size().or_else(0);
				auto* bytes = memory::allocate<uint8_t>(sizeof(uint8_t) * (size + 1));
				if (size > 0)
				{
					auto status = stream->read(bytes, size);
					if (!status)
					{
						memory::deallocate(bytes);
						return status.error();
					}
				}

				bytes[size] = '\0';
				if (length != nullptr)
					*length = size;

				return bytes;
			}

			core::string data;
			auto status = stream->read_all([&data](uint8_t* buffer, size_t length)
			{
				data.reserve(data.size() + length);
				data.append((char*)buffer, length);
			});
			if (!status)
				return status.error();

			size_t size = data.size();
			auto* bytes = memory::allocate<uint8_t>(sizeof(uint8_t) * (data.size() + 1));
			memcpy(bytes, data.data(), sizeof(uint8_t) * data.size());
			bytes[size] = '\0';
			if (length != nullptr)
				*length = size;

			return bytes;
		}
		expects_io<uint8_t*> os::file::read_chunk(stream* stream, size_t length)
		{
			VI_ASSERT(stream != nullptr, "stream should be set");
			auto* bytes = memory::allocate<uint8_t>(length + 1);
			if (length > 0)
				stream->read(bytes, length);
			bytes[length] = '\0';
			return bytes;
		}
		expects_io<string> os::file::read_as_string(const std::string_view& path)
		{
			size_t length = 0;
			auto file_data = read_all(path, &length);
			if (!file_data)
				return file_data.error();

			auto* data = (char*)*file_data;
			string output(data, length);
			memory::deallocate(data);
			return output;
		}
		expects_io<vector<string>> os::file::read_as_array(const std::string_view& path)
		{
			expects_io<string> result = read_as_string(path);
			if (!result)
				return result.error();

			return stringify::split(*result, '\n');
		}

		bool os::path::is_remote(const std::string_view& path)
		{
			return network::location(path).protocol != "file";
		}
		bool os::path::is_relative(const std::string_view& path)
		{
#ifdef VI_MICROSOFT
			return !is_absolute(path);
#else
			return path[0] == '/' || path[0] == '\\';
#endif
		}
		bool os::path::is_absolute(const std::string_view& path)
		{
#ifdef VI_MICROSOFT
			if (path[0] == '/' || path[0] == '\\')
				return true;

			if (path.size() < 2)
				return false;

			return path[1] == ':' && stringify::is_alphanum(path[0]);
#else
			return path[0] == '/' || path[0] == '\\';
#endif
		}
		expects_io<string> os::path::resolve(const std::string_view& path)
		{
			VI_ASSERT(stringify::is_cstring(path), "path should be set");
			VI_MEASURE(timings::file_system);
			char buffer[BLOB_SIZE] = { 0 };
#ifdef VI_MICROSOFT
			if (GetFullPathNameA(path.data(), sizeof(buffer), buffer, nullptr) == 0)
				return os::error::get_condition_or();
#else
			if (!realpath(path.data(), buffer))
			{
				if (!*buffer && (path.empty() || path.find("./") != std::string::npos || path.find(".\\") != std::string::npos))
					return os::error::get_condition_or();

				string output = *buffer > 0 ? string(buffer, strnlen(buffer, BLOB_SIZE)) : string(path);
				VI_TRACE("[io] resolve %.*s path (non-existant)", (int)path.size(), path.data());
				return output;
			}
#endif
			string output(buffer, strnlen(buffer, BLOB_SIZE));
			VI_TRACE("[io] resolve %.*s path: %s", (int)path.size(), path.data(), output.c_str());
			return output;
		}
		expects_io<string> os::path::resolve(const std::string_view& path, const std::string_view& directory, bool even_if_exists)
		{
			VI_ASSERT(!path.empty() && !directory.empty(), "path and directory should not be empty");
			if (is_absolute(path))
				return string(path);
			else if (!even_if_exists && is_path_exists(path) && path.find("..") == std::string::npos)
				return string(path);

			bool prefixed = stringify::starts_of(path, "/\\");
			bool relative = !prefixed && (stringify::starts_with(path, "./") || stringify::starts_with(path, ".\\"));
			bool postfixed = stringify::ends_of(directory, "/\\");

			string target = string(directory);
			if (!prefixed && !postfixed)
				target.append(1, VI_SPLITTER);

			if (relative)
				target.append(path.data() + 2, path.size() - 2);
			else
				target.append(path);

			return resolve(target.c_str());
		}
		expects_io<string> os::path::resolve_directory(const std::string_view& path)
		{
			expects_io<string> result = resolve(path);
			if (!result)
				return result;

			if (!result->empty() && !stringify::ends_of(*result, "/\\"))
				result->append(1, VI_SPLITTER);

			return result;
		}
		expects_io<string> os::path::resolve_directory(const std::string_view& path, const std::string_view& directory, bool even_if_exists)
		{
			expects_io<string> result = resolve(path, directory, even_if_exists);
			if (!result)
				return result;

			if (!result->empty() && !stringify::ends_of(*result, "/\\"))
				result->append(1, VI_SPLITTER);

			return result;
		}
		string os::path::get_non_existant(const std::string_view& path)
		{
			VI_ASSERT(!path.empty(), "path should not be empty");
			auto extension = get_extension(path);
			bool is_true_file = !extension.empty();
			size_t extension_at = is_true_file ? path.rfind(extension) : path.size();
			if (extension_at == string::npos)
				return string(path);

			string first = string(path.substr(0, extension_at));
			string second = string(1, '.') + string(extension);
			string filename = string(path);
			size_t nonce = 0;

			while (true)
			{
				auto data = os::file::get_state(filename);
				if (!data || !data->size)
					break;

				filename = first + core::to_string(++nonce) + second;
			}

			return filename;
		}
		string os::path::get_directory(const std::string_view& path, size_t level)
		{
			string buffer(path);
			text_settle result = stringify::reverse_find_of(buffer, "/\\");
			if (!result.found)
				return buffer;

			size_t size = buffer.size();
			for (size_t i = 0; i < level; i++)
			{
				text_settle current = stringify::reverse_find_of(buffer, "/\\", size - result.start);
				if (!current.found)
				{
					stringify::splice(buffer, 0, result.end);
					if (buffer.empty())
						return "/";

					return buffer;
				}

				result = current;
			}

			stringify::splice(buffer, 0, result.end);
			if (buffer.empty())
				buffer.assign("/");

			return buffer;
		}
		std::string_view os::path::get_filename(const std::string_view& path)
		{
			size_t size = path.size();
			for (size_t i = size; i-- > 0;)
			{
				if (path[i] == '/' || path[i] == '\\')
					return path.substr(i + 1);
			}

			return path;
		}
		std::string_view os::path::get_extension(const std::string_view& path)
		{
			if (path.empty())
				return "";

			size_t index = path.rfind('.');
			if (index == std::string::npos)
				return "";

			return path.substr(index + 1);
		}

		bool os::net::get_etag(char* buffer, size_t length, file_entry* resource)
		{
			VI_ASSERT(resource != nullptr, "resource should be set");
			return get_etag(buffer, length, resource->last_modified, resource->size);
		}
		bool os::net::get_etag(char* buffer, size_t length, int64_t last_modified, size_t content_length)
		{
			VI_ASSERT(buffer != nullptr && length > 0, "buffer should be set and size should be greater than zero");
			snprintf(buffer, length, "\"%lx.%" PRIu64 "\"", (unsigned long)last_modified, (uint64_t)content_length);
			return true;
		}
		socket_t os::net::get_fd(FILE* stream)
		{
			VI_ASSERT(stream != nullptr, "stream should be set");
			return VI_FILENO(stream);
		}

		void os::process::abort()
		{
#ifdef NDEBUG
			VI_DEBUG("[os] process terminate on thread %s", get_thread_id(std::this_thread::get_id()).c_str());
			std::terminate();
#else
			VI_DEBUG("[os] process abort on thread %s", get_thread_id(std::this_thread::get_id()).c_str());
			std::abort();
#endif
		}
		void os::process::exit(int code)
		{
			VI_DEBUG("[os] process exit:%i on thread %s", code, get_thread_id(std::this_thread::get_id()).c_str());
			std::exit(code);
		}
		void os::process::interrupt()
		{
#ifndef NDEBUG
			VI_DEBUG("[os] process suspend on thread %s", get_thread_id(std::this_thread::get_id()).c_str());
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
		int os::process::get_signal_id(signal_code type)
		{
			int invalid_offset = 0x2d42;
			switch (type)
			{
				case signal_code::SIG_INT:
#ifdef SIGINT
					return SIGINT;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_ILL:
#ifdef SIGILL
					return SIGILL;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_FPE:
#ifdef SIGFPE
					return SIGFPE;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_SEGV:
#ifdef SIGSEGV
					return SIGSEGV;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_TERM:
#ifdef SIGTERM
					return SIGTERM;
#else
					return -1;
#endif
				case signal_code::SIG_BREAK:
#ifdef SIGBREAK
					return SIGBREAK;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_ABRT:
#ifdef SIGABRT
					return SIGABRT;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_BUS:
#ifdef SIGBUS
					return SIGBUS;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_ALRM:
#ifdef SIGALRM
					return SIGALRM;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_HUP:
#ifdef SIGHUP
					return SIGHUP;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_QUIT:
#ifdef SIGQUIT
					return SIGQUIT;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_TRAP:
#ifdef SIGTRAP
					return SIGTRAP;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_CONT:
#ifdef SIGCONT
					return SIGCONT;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_STOP:
#ifdef SIGSTOP
					return SIGSTOP;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_PIPE:
#ifdef SIGPIPE
					return SIGPIPE;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_CHLD:
#ifdef SIGCHLD
					return SIGCHLD;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_USR1:
#ifdef SIGUSR1
					return SIGUSR1;
#else
					return invalid_offset + (int)type;
#endif
				case signal_code::SIG_USR2:
#ifdef SIGUSR2
					return SIGUSR2;
#else
					return invalid_offset + (int)type;
#endif
				default:
					VI_ASSERT(false, "invalid signal type %i", (int)type);
					return -1;
			}
		}
		bool os::process::raise_signal(signal_code type)
		{
			int id = get_signal_id(type);
			if (id == -1)
				return false;

			return raise(id) == 0;
		}
		bool os::process::bind_signal(signal_code type, signal_callback callback)
		{
			int id = get_signal_id(type);
			if (id == -1)
				return false;
			VI_DEBUG("[os] signal %i: %s", id, callback ? "callback" : "ignore");
#ifdef VI_LINUX
			struct sigaction handle;
			handle.sa_handler = callback ? callback : SIG_IGN;
			sigemptyset(&handle.sa_mask);
			handle.sa_flags = 0;

			return sigaction(id, &handle, NULL) != -1;
#else
			return signal(id, callback ? callback : SIG_IGN) != SIG_ERR;
#endif
		}
		bool os::process::rebind_signal(signal_code type)
		{
			int id = get_signal_id(type);
			if (id == -1)
				return false;
			VI_DEBUG("[os] signal %i: default", id);
#ifdef VI_LINUX
			struct sigaction handle;
			handle.sa_handler = SIG_DFL;
			sigemptyset(&handle.sa_mask);
			handle.sa_flags = 0;

			return sigaction(id, &handle, NULL) != -1;
#else
			return signal(id, SIG_DFL) != SIG_ERR;
#endif
		}
		bool os::process::has_debugger()
		{
#ifdef VI_MICROSOFT
			return IsDebuggerPresent();
#else
			return false;
#endif
		}
		expects_io<int> os::process::execute(const std::string_view& command, file_mode mode, process_callback&& callback)
		{
			VI_ASSERT(!command.empty(), "commmand should be set");
			VI_DEBUG("[os] execute sh [ %.*s ]", (int)command.size(), command.data());
			uptr<process_stream> stream = new process_stream();
			auto result = stream->open(command, file_mode::read_only);
			if (!result)
				return result.error();

			bool notify = true;
			char buffer[CHUNK_SIZE];
			while (true)
			{
				auto size = stream->read_line(buffer, sizeof(buffer));
				if (!size || !*size)
					break;
				else if (notify && callback)
					notify = callback(std::string_view(buffer, *size));
			}

			result = stream->close();
			if (!result)
				return result.error();

			return stream->get_exit_code();
		}
		expects_io<process_stream*> os::process::spawn(const std::string_view& command, file_mode mode)
		{
			VI_ASSERT(!command.empty(), "command should be set");
			VI_DEBUG("[os] execute sh [ %.*s ]", (int)command.size(), command.data());
			uptr<process_stream> stream = new process_stream();
			auto result = stream->open(command, mode);
			if (result)
				return stream.reset();

			return result.error();
		}
		string os::process::get_thread_id(const std::thread::id& id)
		{
			string_stream stream;
			stream << id;
			return stream.str();
		}
		expects_io<string> os::process::get_env(const std::string_view& name)
		{
			VI_ASSERT(stringify::is_cstring(name), "name should be set");
			VI_TRACE("[os] load env %.*s", (int)name.size(), name.data());
			if (!control::has(access_option::env))
				return std::make_error_condition(std::errc::permission_denied);

			char* value = std::getenv(name.data());
			if (!value)
				return os::error::get_condition_or();

			string output(value, strlen(value));
			return output;
		}
		expects_io<string> os::process::get_shell()
		{
#ifdef VI_MICROSOFT
			auto shell = get_env("ComSpec");
			if (shell)
				return shell;
#else
			auto shell = get_env("SHELL");
			if (shell)
				return shell;
#endif
			return std::make_error_condition(std::errc::no_such_file_or_directory);
		}
		inline_args os::process::parse_args(int args_count, char** args, size_t opts, const unordered_set<string>& flags)
		{
			VI_ASSERT(args != nullptr, "arguments should be set");
			VI_ASSERT(args_count > 0, "arguments count should be greater than zero");

			inline_args context;
			for (int i = 0; i < args_count; i++)
			{
				VI_ASSERT(args[i] != nullptr, "argument %i should be set", i);
				context.params.push_back(args[i]);
			}

			vector<string> params;
			params.reserve(context.params.size());

			string placeholder = "1";
			auto inline_text = [&placeholder](const core::string& value) -> const core::string& { return value.empty() ? placeholder : value; };
			for (size_t i = 1; i < context.params.size(); i++)
			{
				auto& item = context.params[i];
				if (item.empty() || item.front() != '-')
					goto no_match;

				if ((opts & (size_t)args_format::key || opts & (size_t)args_format::key_value) && item.size() > 1 && item[1] == '-')
				{
					item = item.substr(2);
					size_t position = item.find('=');
					if (position != string::npos)
					{
						string name = item.substr(0, position);
						if (flags.find(name) != flags.end())
						{
							context.args[item] = placeholder;
							if (opts & (size_t)args_format::stop_if_no_match)
							{
								params.insert(params.begin(), context.params.begin() + i + 1, context.params.end());
								break;
							}
						}
						else
							context.args[name] = inline_text(item.substr(position + 1));
					}
					else if (opts & (size_t)args_format::key || flags.find(item) != flags.end())
						context.args[item] = placeholder;
					else
						goto no_match;
				}
				else
				{
					string name = item.substr(1);
					if ((opts & (size_t)args_format::flag_value) && i + 1 < context.params.size() && context.params[i + 1].front() != '-')
					{
						if (flags.find(name) != flags.end())
						{
							context.args[name] = placeholder;
							if (opts & (size_t)args_format::stop_if_no_match)
							{
								params.insert(params.begin(), context.params.begin() + i + 1, context.params.end());
								break;
							}
						}
						else
							context.args[name] = inline_text(context.params[++i]);
					}
					else if (opts & (size_t)args_format::flag || flags.find(name) != flags.end())
						context.args[item.substr(1)] = placeholder;
					else
						goto no_match;
				}

				continue;
			no_match:
				if (opts & (size_t)args_format::stop_if_no_match)
				{
					params.insert(params.begin(), context.params.begin() + i, context.params.end());
					break;
				}
				else
					params.push_back(item);
			}

			if (!context.params.empty())
				context.path = context.params.front();
			context.params = std::move(params);
			return context;
		}

		expects_io<void*> os::symbol::load(const std::string_view& path)
		{
			VI_MEASURE(timings::file_system);
			if (!control::has(access_option::lib))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			if (path.empty())
			{
				HMODULE library = GetModuleHandle(nullptr);
				if (!library)
					return os::error::get_condition_or();

				return (void*)library;
			}

			string name = string(path);
			if (!stringify::ends_with(name, ".dll"))
				name.append(".dll");

			VI_DEBUG("[dl] load dll library %s", name.c_str());
			HMODULE library = LoadLibrary(name.c_str());
			if (!library)
				return os::error::get_condition_or();

			return (void*)library;
#elif defined(VI_APPLE)
			if (path.empty())
			{
				void* library = (void*)dlopen(nullptr, RTLD_LAZY);
				if (!library)
					return os::error::get_condition_or();

				return (void*)library;
			}

			string name = string(path);
			if (!stringify::ends_with(name, ".dylib"))
				name.append(".dylib");

			VI_DEBUG("[dl] load dylib library %s", name.c_str());
			void* library = (void*)dlopen(name.c_str(), RTLD_LAZY);
			if (!library)
				return os::error::get_condition_or();

			return (void*)library;
#elif defined(VI_LINUX)
			if (path.empty())
			{
				void* library = (void*)dlopen(nullptr, RTLD_LAZY);
				if (!library)
					return os::error::get_condition_or();

				return (void*)library;
			}

			string name = string(path);
			if (!stringify::ends_with(name, ".so"))
				name.append(".so");

			VI_DEBUG("[dl] load so library %s", name.c_str());
			void* library = (void*)dlopen(name.c_str(), RTLD_LAZY);
			if (!library)
				return os::error::get_condition_or();

			return (void*)library;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<void*> os::symbol::load_function(void* handle, const std::string_view& name)
		{
			VI_ASSERT(handle != nullptr && stringify::is_cstring(name), "handle should be set and name should not be empty");
			VI_DEBUG("[dl] load function %.*s", (int)name.size(), name.data());
			VI_MEASURE(timings::file_system);
			if (!control::has(access_option::lib))
				return std::make_error_condition(std::errc::permission_denied);
#ifdef VI_MICROSOFT
			void* result = (void*)GetProcAddress((HMODULE)handle, name.data());
			if (!result)
				return os::error::get_condition_or();

			return result;
#elif defined(VI_LINUX)
			void* result = (void*)dlsym(handle, name.data());
			if (!result)
				return os::error::get_condition_or();

			return result;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}
		expects_io<void> os::symbol::unload(void* handle)
		{
			VI_ASSERT(handle != nullptr, "handle should be set");
			VI_MEASURE(timings::file_system);
			VI_DEBUG("[dl] unload library 0x%" PRIXPTR, handle);
#ifdef VI_MICROSOFT
			if (FreeLibrary((HMODULE)handle) != TRUE)
				return os::error::get_condition_or();

			return expectation::met;
#elif defined(VI_LINUX)
			if (dlclose(handle) != 0)
				return os::error::get_condition_or();

			return expectation::met;
#else
			return std::make_error_condition(std::errc::not_supported);
#endif
		}

		int os::error::get(bool clear_last_error)
		{
#ifdef VI_MICROSOFT
			int error_code = GetLastError();
			if (error_code != ERROR_SUCCESS)
			{
				if (clear_last_error)
					SetLastError(ERROR_SUCCESS);
				return error_code;
			}

			error_code = WSAGetLastError();
			if (error_code != ERROR_SUCCESS)
			{
				if (clear_last_error)
					WSASetLastError(ERROR_SUCCESS);
			}

			error_code = errno;
			if (error_code != 0)
			{
				if (clear_last_error)
					errno = 0;
				return error_code;
			}

			return error_code;
#else
			int error_code = errno;
			if (error_code != 0)
			{
				if (clear_last_error)
					errno = 0;
			}

			return error_code;
#endif
		}
		bool os::error::occurred()
		{
			return is_error(get(false));
		}
		bool os::error::is_error(int code)
		{
#ifdef VI_MICROSOFT
			return code != ERROR_SUCCESS;
#else
			return code != 0;
#endif
		}
		void os::error::clear()
		{
#ifdef VI_MICROSOFT
			SetLastError(ERROR_SUCCESS);
			WSASetLastError(ERROR_SUCCESS);
#endif
			errno = 0;
		}
		std::error_condition os::error::get_condition()
		{
			return get_condition(get());
		}
		std::error_condition os::error::get_condition(int code)
		{
#ifdef VI_MICROSOFT
			return std::error_condition(code, std::system_category());
#else
			return std::error_condition(code, std::generic_category());
#endif
		}
		std::error_condition os::error::get_condition_or(std::errc code)
		{
			if (!occurred())
				return std::make_error_condition(code);

			return get_condition(get());
		}
		string os::error::get_name(int code)
		{
#ifdef VI_MICROSOFT
			LPSTR buffer = nullptr;
			size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&buffer, 0, nullptr);
			string result(buffer, size);
			stringify::replace(result, "\r", "");
			stringify::replace(result, "\n", "");
			LocalFree(buffer);
			return result;
#else
			char* buffer = strerror(code);
			return buffer ? buffer : "";
#endif
		}

		void os::control::set(access_option option, bool enabled)
		{
			VI_DEBUG("[os] control %s set %s", control::to_string(option).data(), enabled ? "ON" : "OFF");
			uint64_t prev_options = options.load();
			if (enabled)
				prev_options |= (uint64_t)option;
			else
				prev_options &= ~((uint64_t)option);
			options = prev_options;
		}
		bool os::control::has(access_option option)
		{
			return options & (uint64_t)option;
		}
		option<access_option> os::control::to_option(const std::string_view& option)
		{
			if (option == "mem")
				return access_option::mem;
			if (option == "fs")
				return access_option::fs;
			if (option == "gz")
				return access_option::gz;
			if (option == "net")
				return access_option::net;
			if (option == "lib")
				return access_option::lib;
			if (option == "http")
				return access_option::http;
			if (option == "https")
				return access_option::https;
			if (option == "shell")
				return access_option::shell;
			if (option == "env")
				return access_option::env;
			if (option == "addons")
				return access_option::addons;
			if (option == "all")
				return access_option::all;
			return optional::none;
		}
		std::string_view os::control::to_string(access_option option)
		{
			switch (option)
			{
				case access_option::mem:
					return "mem";
				case access_option::fs:
					return "fs";
				case access_option::gz:
					return "gz";
				case access_option::net:
					return "net";
				case access_option::lib:
					return "lib";
				case access_option::http:
					return "http";
				case access_option::https:
					return "https";
				case access_option::shell:
					return "shell";
				case access_option::env:
					return "env";
				case access_option::addons:
					return "addons";
				case access_option::all:
					return "all";
				default:
					return "";
			}
		}
		std::string_view os::control::to_options()
		{
			return "mem, fs, gz, net, lib, http, https, shell, env, addons, all";
		}
		std::atomic<uint64_t> os::control::options = (uint64_t)access_option::all;

		static thread_local costate* internal_coroutine = nullptr;
		costate::costate(size_t stack_size) noexcept : thread(std::this_thread::get_id()), current(nullptr), master(memory::init<cocontext>()), size(stack_size), external_condition(nullptr), external_mutex(nullptr)
		{
			VI_TRACE("[co] spawn coroutine state 0x%" PRIXPTR " on thread %s", (void*)this, os::process::get_thread_id(thread).c_str());
		}
		costate::~costate() noexcept
		{
			VI_TRACE("[co] despawn coroutine state 0x%" PRIXPTR " on thread %s", (void*)this, os::process::get_thread_id(thread).c_str());
			if (internal_coroutine == this)
				internal_coroutine = nullptr;

			for (auto& routine : cached)
				memory::deinit(routine);

			for (auto& routine : used)
				memory::deinit(routine);

			memory::deinit(master);
		}
		coroutine* costate::pop(task_callback&& procedure)
		{
			VI_ASSERT(thread == std::this_thread::get_id(), "cannot deactive coroutine outside costate thread");

			coroutine* routine = nullptr;
			if (!cached.empty())
			{
				routine = *cached.begin();
				routine->callback = std::move(procedure);
				routine->state = coexecution::active;
				cached.erase(cached.begin());
			}
			else
				routine = memory::init<coroutine>(this, std::move(procedure));

			used.emplace(routine);
			return routine;
		}
		coexecution costate::resume(coroutine* routine)
		{
			VI_ASSERT(routine != nullptr, "coroutine should be set");
			VI_ASSERT(thread == std::this_thread::get_id(), "cannot resume coroutine outside costate thread");
			VI_ASSERT(routine->master == this, "coroutine should be created by this costate");

			if (current == routine)
				return coexecution::active;
			else if (routine->state == coexecution::finished)
				return coexecution::finished;

			return execute(routine);
		}
		coexecution costate::execute(coroutine* routine)
		{
			VI_ASSERT(thread == std::this_thread::get_id(), "cannot call outside costate thread");
			VI_ASSERT(routine != nullptr, "coroutine should be set");
			VI_ASSERT(routine->state != coexecution::finished, "coroutine should not be dead");

			if (routine->state == coexecution::suspended)
				return coexecution::suspended;

			if (routine->state == coexecution::resumable)
				routine->state = coexecution::active;

			cocontext* fiber = routine->slave;
			current = routine;
#ifdef VI_FCONTEXT
			fiber->context = jump_fcontext(fiber->context, (void*)this).fctx;
#elif VI_MICROSOFT
			SwitchToFiber(fiber->context);
#else
			swapcontext(&master->context, &fiber->context);
#endif
			if (routine->defer)
			{
				routine->defer();
				routine->defer = nullptr;
			}

			return routine->state == coexecution::finished ? coexecution::finished : coexecution::active;
		}
		void costate::reuse(coroutine* routine)
		{
			VI_ASSERT(thread == std::this_thread::get_id(), "cannot call outside costate thread");
			VI_ASSERT(routine != nullptr, "coroutine should be set");
			VI_ASSERT(routine->master == this, "coroutine should be created by this costate");
			VI_ASSERT(routine->state == coexecution::finished, "coroutine should be empty");

			routine->callback = nullptr;
			routine->defer = nullptr;
			routine->state = coexecution::active;
			used.erase(routine);
			cached.emplace(routine);
		}
		void costate::push(coroutine* routine)
		{
			VI_ASSERT(thread == std::this_thread::get_id(), "cannot call outside costate thread");
			VI_ASSERT(routine != nullptr, "coroutine should be set");
			VI_ASSERT(routine->master == this, "coroutine should be created by this costate");
			VI_ASSERT(routine->state == coexecution::finished, "coroutine should be empty");

			cached.erase(routine);
			used.erase(routine);
			memory::deinit(routine);
		}
		void costate::activate(coroutine* routine)
		{
			VI_ASSERT(routine != nullptr, "coroutine should be set");
			VI_ASSERT(routine->master == this, "coroutine should be created by this costate");
			VI_ASSERT(routine->state != coexecution::finished, "coroutine should not be empty");

			if (external_mutex != nullptr && external_condition != nullptr && thread != std::this_thread::get_id())
			{
				umutex<std::mutex> unique(*external_mutex);
				if (routine->state == coexecution::suspended)
					routine->state = coexecution::resumable;
				external_condition->notify_one();
			}
			else if (routine->state == coexecution::suspended)
				routine->state = coexecution::resumable;
		}
		void costate::deactivate(coroutine* routine)
		{
			VI_ASSERT(thread == std::this_thread::get_id(), "cannot deactive coroutine outside costate thread");
			VI_ASSERT(routine != nullptr, "coroutine should be set");
			VI_ASSERT(routine->master == this, "coroutine should be created by this costate");
			VI_ASSERT(routine->state != coexecution::finished, "coroutine should not be empty");
			if (current != routine || routine->state != coexecution::active)
				return;

			routine->state = coexecution::suspended;
			suspend();
		}
		void costate::deactivate(coroutine* routine, task_callback&& after_suspend)
		{
			VI_ASSERT(routine != nullptr, "coroutine should be set");
			routine->defer = std::move(after_suspend);
			deactivate(routine);
		}
		void costate::clear()
		{
			VI_ASSERT(thread == std::this_thread::get_id(), "cannot call outside costate thread");
			for (auto& routine : cached)
				memory::deinit(routine);
			cached.clear();
		}
		bool costate::dispatch()
		{
			VI_ASSERT(thread == std::this_thread::get_id(), "cannot dispatch coroutine outside costate thread");
			VI_ASSERT(!current, "cannot dispatch coroutines inside another coroutine");
			size_t executions = 0;
		retry:
			for (auto* routine : used)
			{
				switch (execute(routine))
				{
					case coexecution::active:
						++executions;
						break;
					case coexecution::suspended:
					case coexecution::resumable:
						break;
					case coexecution::finished:
						++executions;
						reuse(routine);
						goto retry;
				}
			}

			return executions > 0;
		}
		bool costate::suspend()
		{
			VI_ASSERT(thread == std::this_thread::get_id(), "cannot suspend coroutine outside costate thread");

			coroutine* routine = current;
			if (!routine || routine->master != this)
				return false;
#ifdef VI_FCONTEXT
			current = nullptr;
			jump_fcontext(master->context, (void*)this);
#elif VI_MICROSOFT
			current = nullptr;
			SwitchToFiber(master->context);
#else
			char bottom = 0;
			char* top = routine->slave->stack + size;
			if (size_t(top - &bottom) > size)
				return false;

			current = nullptr;
			swapcontext(&routine->slave->context, &master->context);
#endif
			return true;
		}
		bool costate::has_resumable_coroutines() const
		{
			VI_ASSERT(thread == std::this_thread::get_id(), "cannot call outside costate thread");
			for (const auto& item : used)
			{
				if (item->state == coexecution::active || item->state == coexecution::resumable)
					return true;
			}

			return false;
		}
		bool costate::has_alive_coroutines() const
		{
			VI_ASSERT(thread == std::this_thread::get_id(), "cannot call outside costate thread");
			for (const auto& item : used)
			{
				if (item->state != coexecution::finished)
					return true;
			}

			return false;
		}
		bool costate::has_coroutines() const
		{
			return !used.empty();
		}
		size_t costate::get_count() const
		{
			return used.size();
		}
		coroutine* costate::get_current() const
		{
			return current;
		}
		costate* costate::get()
		{
			return internal_coroutine;
		}
		coroutine* costate::get_coroutine()
		{
			return internal_coroutine ? internal_coroutine->current : nullptr;
		}
		bool costate::get_state(costate** state, coroutine** routine)
		{
			VI_ASSERT(state != nullptr, "state should be set");
			VI_ASSERT(routine != nullptr, "state should be set");
			*routine = (internal_coroutine ? internal_coroutine->current : nullptr);
			*state = internal_coroutine;

			return *routine != nullptr;
		}
		bool costate::is_coroutine()
		{
			return internal_coroutine ? internal_coroutine->current != nullptr : false;
		}
		void VI_COCALL costate::execution_entry(VI_CODATA)
		{
#ifdef VI_FCONTEXT
			transfer_t* transfer = (transfer_t*)context;
			costate* state = (costate*)transfer->data;
			state->master->context = transfer->fctx;
#elif VI_MICROSOFT
			costate* state = (costate*)context;
#else
			costate* state = (costate*)unpack264(x, y);
#endif
			internal_coroutine = state;
			VI_ASSERT(state != nullptr, "costate should be set");
			coroutine* routine = state->current;
			if (routine != nullptr)
			{
			reuse:
				if (routine->callback)
					routine->callback();
				routine->defer = nullptr;
				routine->state = coexecution::finished;
			}

			state->current = nullptr;
#ifdef VI_FCONTEXT
			jump_fcontext(state->master->context, context);
#elif VI_MICROSOFT
			SwitchToFiber(state->master->context);
#else
			swapcontext(&routine->slave->context, &state->master->context);
#endif
			if (routine != nullptr && routine->callback)
				goto reuse;
		}

		schedule::desc::desc() : desc(std::max<uint32_t>(2, os::hw::get_quantity_info().logical) - 1)
		{
		}
		schedule::desc::desc(size_t size) : preallocated_size(0), stack_size(STACK_SIZE), max_coroutines(96), max_recycles(64), idle_timeout(std::chrono::milliseconds(2000)), clock_timeout(std::chrono::milliseconds((uint64_t)timings::intensive)), parallel(true)
		{
			if (!size)
				size = 1;
#ifndef VI_CXX20
			const size_t async = (size_t)std::max(std::ceil(size * 0.20), 1.0);
#else
			const size_t async = 0;
#endif
			const size_t timeout = 1;
			const size_t sync = std::max<size_t>(std::min<size_t>(size - async - timeout, size), 1);
			threads[((size_t)difficulty::async)] = async;
			threads[((size_t)difficulty::sync)] = sync;
			threads[((size_t)difficulty::timeout)] = timeout;
			max_coroutines = std::min<size_t>(size * 8, 256);
		}

		schedule::schedule() noexcept : generation(0), debug(nullptr), terminate(false), enqueue(true), suspended(false), active(false)
		{
			timeouts = memory::init<concurrent_timeout_queue>();
			async = memory::init<concurrent_async_queue>();
			sync = memory::init<concurrent_sync_queue>();
		}
		schedule::~schedule() noexcept
		{
			stop();
			memory::deinit(sync);
			memory::deinit(async);
			memory::deinit(timeouts);
			memory::release(dispatcher.state);
			scripting::virtual_machine::cleanup_this_thread();
		}
		task_id schedule::get_task_id()
		{
			task_id id = ++generation;
			while (id == INVALID_TASK_ID)
				id = ++generation;
			return id;
		}
		task_id schedule::set_interval(uint64_t milliseconds, task_callback&& callback)
		{
			VI_ASSERT(callback, "callback should not be empty");
			if (!enqueue)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			report_thread(thread_task::enqueue_timer, 1, get_thread());
#endif
			VI_MEASURE(timings::atomic);
			auto duration = std::chrono::microseconds(milliseconds * 1000);
			auto expires = get_clock() + duration;
			auto id = get_task_id();

			umutex<std::mutex> unique(timeouts->update);
			timeouts->queue.emplace(std::make_pair(get_timeout(expires), timeout(std::move(callback), duration, id, true)));
			timeouts->resync = true;
			timeouts->notify.notify_all();
			return id;
		}
		task_id schedule::set_timeout(uint64_t milliseconds, task_callback&& callback)
		{
			VI_ASSERT(callback, "callback should not be empty");
			if (!enqueue)
				return INVALID_TASK_ID;
#ifndef NDEBUG
			report_thread(thread_task::enqueue_timer, 1, get_thread());
#endif
			VI_MEASURE(timings::atomic);
			auto duration = std::chrono::microseconds(milliseconds * 1000);
			auto expires = get_clock() + duration;
			auto id = get_task_id();

			umutex<std::mutex> unique(timeouts->update);
			timeouts->queue.emplace(std::make_pair(get_timeout(expires), timeout(std::move(callback), duration, id, false)));
			timeouts->resync = true;
			timeouts->notify.notify_all();
			return id;
		}
		bool schedule::set_task(task_callback&& callback, bool recyclable)
		{
			VI_ASSERT(callback, "callback should not be empty");
			if (!enqueue)
				return false;
#ifndef NDEBUG
			report_thread(thread_task::enqueue_task, 1, get_thread());
#endif
			VI_MEASURE(timings::atomic);
			if (!recyclable || !fast_bypass_enqueue(difficulty::sync, std::move(callback)))
				sync->queue.enqueue(std::move(callback));
			return true;
		}
		bool schedule::set_coroutine(task_callback&& callback, bool recyclable)
		{
			VI_ASSERT(callback, "callback should not be empty");
			if (!enqueue)
				return false;
#ifndef NDEBUG
			report_thread(thread_task::enqueue_coroutine, 1, get_thread());
#endif
			VI_MEASURE(timings::atomic);
			if (recyclable && fast_bypass_enqueue(difficulty::async, std::move(callback)))
				return true;

			async->queue.enqueue(std::move(callback));
			umutex<std::mutex> unique(async->update);
			for (auto* thread : threads[(size_t)difficulty::async])
				thread->notify.notify_all();

			return true;
		}
		bool schedule::set_debug_callback(thread_debug_callback&& callback)
		{
#ifndef NDEBUG
			debug = std::move(callback);
			return true;
#else
			return false;
#endif
		}
		bool schedule::clear_timeout(task_id target)
		{
			VI_MEASURE(timings::atomic);
			if (target == INVALID_TASK_ID)
				return false;

			umutex<std::mutex> unique(timeouts->update);
			for (auto it = timeouts->queue.begin(); it != timeouts->queue.end(); ++it)
			{
				if (it->second.id == target)
				{
					timeouts->resync = true;
					timeouts->queue.erase(it);
					timeouts->notify.notify_all();
					return true;
				}
			}
			return false;
		}
		bool schedule::trigger_timers()
		{
			VI_MEASURE(timings::pass);
			umutex<std::mutex> unique(timeouts->update);
			for (auto& item : timeouts->queue)
				set_task(std::move(item.second.callback));

			size_t size = timeouts->queue.size();
			timeouts->resync = true;
			timeouts->queue.clear();
			return size > 0;
		}
		bool schedule::trigger(difficulty type)
		{
			VI_MEASURE(timings::intensive);
			switch (type)
			{
				case difficulty::timeout:
				{
					if (timeouts->queue.empty())
						return false;
					else if (suspended)
						return true;

					auto clock = get_clock();
					auto it = timeouts->queue.begin();
					if (it->first >= clock)
						return true;
#ifndef NDEBUG
					report_thread(thread_task::process_timer, 1, nullptr);
#endif
					if (it->second.alive && active)
					{
						timeout next(std::move(it->second));
						timeouts->queue.erase(it);

						set_task([this, next = std::move(next)]() mutable
						{
							next.callback();
							umutex<std::mutex> unique(timeouts->update);
							timeouts->queue.emplace(std::make_pair(get_timeout(get_clock() + next.expires), std::move(next)));
							timeouts->resync = true;
							timeouts->notify.notify_all();
						});
					}
					else
					{
						set_task(std::move(it->second.callback));
						timeouts->queue.erase(it);
					}
#ifndef NDEBUG
					report_thread(thread_task::awake, 0, nullptr);
#endif
					return true;
				}
				case difficulty::async:
				{
					if (!dispatcher.state)
						dispatcher.state = new costate(policy.stack_size);

					if (suspended)
						return dispatcher.state->has_coroutines();

					size_t active = dispatcher.state->get_count();
					size_t cache = policy.max_coroutines - active, executions = active;
					while (cache > 0 && async->queue.try_dequeue(dispatcher.event))
					{
						--cache; ++executions;
						dispatcher.state->pop(std::move(dispatcher.event));
#ifndef NDEBUG
						report_thread(thread_task::consume_coroutine, 1, nullptr);
#endif
					}
#ifndef NDEBUG
					report_thread(thread_task::process_coroutine, dispatcher.state->get_count(), nullptr);
#endif
					{
						VI_MEASURE(timings::frame);
						while (dispatcher.state->dispatch())
							++executions;
					}
#ifndef NDEBUG
					report_thread(thread_task::awake, 0, nullptr);
#endif
					return executions > 0;
				}
				case difficulty::sync:
				{
					if (suspended)
						return sync->queue.size_approx() > 0;
					else if (!sync->queue.try_dequeue(dispatcher.event))
						return false;
#ifndef NDEBUG
					report_thread(thread_task::process_task, 1, nullptr);
#endif
					dispatcher.event();
#ifndef NDEBUG
					report_thread(thread_task::awake, 0, nullptr);
#endif
					return true;
				}
				default:
					break;
			}

			return false;
		}
		bool schedule::start(const desc& new_policy)
		{
			VI_ASSERT(!active, "queue should be stopped");
			VI_ASSERT(new_policy.stack_size > 0, "stack size should not be zero");
			VI_ASSERT(new_policy.max_coroutines > 0, "there must be at least one coroutine");
			VI_TRACE("[schedule] start 0x%" PRIXPTR " on thread %s", (void*)this, os::process::get_thread_id(std::this_thread::get_id()).c_str());

			policy = new_policy;
			active = true;

			if (!policy.parallel)
			{
				initialize_thread(nullptr, true);
				return true;
			}

			size_t index = 0;
			for (size_t j = 0; j < policy.threads[(size_t)difficulty::async]; j++)
				push_thread(difficulty::async, index++, j, false);

			for (size_t j = 0; j < policy.threads[(size_t)difficulty::sync]; j++)
				push_thread(difficulty::sync, index++, j, false);

			initialize_spawn_trigger();
			if (policy.threads[(size_t)difficulty::timeout] > 0)
			{
				if (policy.ping)
					push_thread(difficulty::timeout, 0, 0, true);
				push_thread(difficulty::timeout, index++, 0, false);
			}

			return true;
		}
		bool schedule::stop()
		{
			VI_TRACE("[schedule] stop 0x%" PRIXPTR " on thread %s", (void*)this, os::process::get_thread_id(std::this_thread::get_id()).c_str());
			umutex<std::mutex> unique(exclusive);
			if (!active && !terminate)
				return false;

			active = enqueue = false;
			wakeup();

			for (size_t i = 0; i < (size_t)difficulty::count; i++)
			{
				for (auto* thread : threads[i])
				{
					if (!pop_thread(thread))
					{
						terminate = true;
						return false;
					}
				}
			}

			timeouts->queue.clear();
			terminate = false;
			enqueue = true;
			chunk_cleanup();
			return true;
		}
		bool schedule::wakeup()
		{
			VI_TRACE("[schedule] wakeup 0x%" PRIXPTR " on thread %s", (void*)this, os::process::get_thread_id(std::this_thread::get_id()).c_str());
			task_callback dummy[2] = { []() { }, []() { } };
			size_t dummy_size = sizeof(dummy) / sizeof(task_callback);
			for (size_t i = 0; i < (size_t)difficulty::count; i++)
			{
				for (auto* thread : threads[i])
				{
					if (thread->type == difficulty::async)
						async->queue.enqueue_bulk(dummy, dummy_size);
					else if (thread->type == difficulty::sync || thread->type == difficulty::timeout)
						sync->queue.enqueue_bulk(dummy, dummy_size);
					thread->notify.notify_all();
				}

				if (i == (size_t)difficulty::async)
				{
					umutex<std::mutex> unique(async->update);
					async->resync = true;
					async->notify.notify_all();
				}
				else if (i == (size_t)difficulty::timeout)
				{
					umutex<std::mutex> unique(timeouts->update);
					timeouts->resync = true;
					timeouts->notify.notify_all();
				}
			}
			return true;
		}
		bool schedule::dispatch()
		{
			size_t passes = 0;
			VI_MEASURE(timings::intensive);
			for (size_t i = 0; i < (size_t)difficulty::count; i++)
				passes += (size_t)trigger((difficulty)i);

			return passes > 0;
		}
		bool schedule::trigger_thread(difficulty type, thread_data* thread)
		{
			string thread_id = os::process::get_thread_id(thread->id);
			initialize_thread(thread, true);
			if (!thread_active(thread))
				goto exit_thread;

			switch (type)
			{
				case difficulty::timeout:
				{
					task_callback event;
					receive_token token(sync->queue);
					if (thread->daemon)
						VI_DEBUG("[schedule] acquire thread %s (timers)", thread_id.c_str());
					else
						VI_DEBUG("[schedule] spawn thread %s (timers)", thread_id.c_str());

					do
					{
						if (sleep_thread(type, thread))
							continue;

						std::unique_lock<std::mutex> unique(timeouts->update);
					retry:
#ifndef NDEBUG
						report_thread(thread_task::awake, 0, thread);
#endif
						std::chrono::microseconds when = std::chrono::microseconds(0);
						if (!timeouts->queue.empty())
						{
							auto clock = get_clock();
							auto it = timeouts->queue.begin();
							if (it->first <= clock)
							{
#ifndef NDEBUG
								report_thread(thread_task::process_timer, 1, thread);
#endif
								if (it->second.alive)
								{
									timeout next(std::move(it->second));
									timeouts->queue.erase(it);

									set_task([this, next = std::move(next)]() mutable
									{
										next.callback();
										umutex<std::mutex> unique(timeouts->update);
										timeouts->queue.emplace(std::make_pair(get_timeout(get_clock() + next.expires), std::move(next)));
										timeouts->resync = true;
										timeouts->notify.notify_all();
									});
								}
								else
								{
									set_task(std::move(it->second.callback));
									timeouts->queue.erase(it);
								}

								goto retry;
							}
							else
							{
								when = it->first - clock;
								if (when > policy.idle_timeout)
									when = policy.idle_timeout;
							}
						}
						else
							when = policy.idle_timeout;
#ifndef NDEBUG
						report_thread(thread_task::sleep, 0, thread);
#endif
						timeouts->notify.wait_for(unique, when, [this, thread]() { return !thread_active(thread) || timeouts->resync || sync->queue.size_approx() > 0; });
						timeouts->resync = false;
						unique.unlock();

						if (!sync->queue.try_dequeue(token, event))
							continue;
#ifndef NDEBUG
						report_thread(thread_task::awake, 0, thread);
						report_thread(thread_task::process_task, 1, thread);
#endif
						VI_MEASURE(timings::intensive);
						event();
					} while (thread_active(thread));
					break;
				}
				case difficulty::async:
				{
					task_callback event;
					receive_token token(async->queue);
					if (thread->daemon)
						VI_DEBUG("[schedule] acquire thread %s (coroutines)", thread_id.c_str());
					else
						VI_DEBUG("[schedule] spawn thread %s (coroutines)", thread_id.c_str());

					uptr<costate> state = new costate(policy.stack_size);
					state->external_condition = &thread->notify;
					state->external_mutex = &thread->update;

					do
					{
						if (sleep_thread(type, thread))
							continue;
#ifndef NDEBUG
						report_thread(thread_task::awake, 0, thread);
#endif
						size_t cache = policy.max_coroutines - state->get_count();
						while (cache > 0)
						{
							if (!thread->queue.empty())
							{
								event = std::move(thread->queue.front());
								thread->queue.pop();
							}
							else if (!async->queue.try_dequeue(token, event))
								break;

							--cache;
							state->pop(std::move(event));
#ifndef NDEBUG
							report_thread(thread_task::enqueue_coroutine, 1, thread);
#endif
						}
#ifndef NDEBUG
						report_thread(thread_task::process_coroutine, state->get_count(), thread);
#endif
						{
							VI_MEASURE(timings::frame);
							state->dispatch();
						}
#ifndef NDEBUG
						report_thread(thread_task::sleep, 0, thread);
#endif
						std::unique_lock<std::mutex> unique(thread->update);
						thread->notify.wait_for(unique, policy.idle_timeout, [this, &state, thread]()
						{
							return !thread_active(thread) || state->has_resumable_coroutines() || async->resync.load() || (async->queue.size_approx() > 0 && state->get_count() + 1 < policy.max_coroutines);
						});
						async->resync = false;
					} while (thread_active(thread));
					while (!thread->queue.empty())
					{
						async->queue.enqueue(std::move(thread->queue.front()));
						thread->queue.pop();
					}
					break;
				}
				case difficulty::sync:
				{
					task_callback event;
					receive_token token(sync->queue);
					if (thread->daemon)
						VI_DEBUG("[schedule] acquire thread %s (tasks)", thread_id.c_str());
					else
						VI_DEBUG("[schedule] spawn thread %s (tasks)", thread_id.c_str());

					do
					{
						if (sleep_thread(type, thread))
							continue;
#ifndef NDEBUG
						report_thread(thread_task::sleep, 0, thread);
#endif
						if (!thread->queue.empty())
						{
							event = std::move(thread->queue.front());
							thread->queue.pop();
						}
						else if (!sync->queue.wait_dequeue_timed(token, event, policy.idle_timeout))
							continue;
#ifndef NDEBUG
						report_thread(thread_task::awake, 0, thread);
						report_thread(thread_task::process_task, 1, thread);
#endif
						VI_MEASURE(timings::intensive);
						event();
					} while (thread_active(thread));
					while (!thread->queue.empty())
					{
						sync->queue.enqueue(std::move(thread->queue.front()));
						thread->queue.pop();
					}
					break;
				}
				default:
					break;
			}

		exit_thread:
			if (thread->daemon)
				VI_DEBUG("[schedule] release thread %s", thread_id.c_str());
			else
				VI_DEBUG("[schedule] join thread %s", thread_id.c_str());

			scripting::virtual_machine::cleanup_this_thread();
			initialize_thread(nullptr, true);
			return true;
		}
		bool schedule::sleep_thread(difficulty type, thread_data* thread)
		{
			if (!suspended)
				return false;

#ifndef NDEBUG
			report_thread(thread_task::sleep, 0, thread);
#endif
			std::unique_lock<std::mutex> unique(thread->update);
			thread->notify.wait_for(unique, policy.idle_timeout, [this, thread]()
			{
				return !thread_active(thread) || !suspended;
			});
#ifndef NDEBUG
			report_thread(thread_task::awake, 0, thread);
#endif
			return true;
		}
		bool schedule::thread_active(thread_data* thread)
		{
			if (thread->daemon)
				return active && (policy.ping ? policy.ping() : true);

			return active;
		}
		bool schedule::chunk_cleanup()
		{
			for (size_t i = 0; i < (size_t)difficulty::count; i++)
			{
				for (auto* thread : threads[i])
					memory::deinit(thread);
				threads[i].clear();
			}

			return true;
		}
		bool schedule::push_thread(difficulty type, size_t global_index, size_t local_index, bool is_daemon)
		{
			thread_data* thread = memory::init<thread_data>(type, policy.preallocated_size, global_index, local_index, is_daemon);
			if (!thread->daemon)
			{
				thread->handle = std::thread(&schedule::trigger_thread, this, type, thread);
				thread->id = thread->handle.get_id();
			}
			else
				thread->id = std::this_thread::get_id();
#ifndef NDEBUG
			report_thread(thread_task::spawn, 0, thread);
#endif
			threads[(size_t)type].emplace_back(thread);
			return thread->daemon ? trigger_thread(type, thread) : thread->handle.joinable();
		}
		bool schedule::pop_thread(thread_data* thread)
		{
			if (thread->daemon)
				return true;

			if (thread->id == std::this_thread::get_id())
				return false;

			if (thread->handle.joinable())
				thread->handle.join();
#ifndef NDEBUG
			report_thread(thread_task::despawn, 0, thread);
#endif
			return true;
		}
		bool schedule::is_active() const
		{
			return active;
		}
		bool schedule::can_enqueue() const
		{
			return enqueue;
		}
		bool schedule::has_tasks(difficulty type) const
		{
			VI_ASSERT(type != difficulty::count, "difficulty should be set");
			switch (type)
			{
				case difficulty::async:
					return async->queue.size_approx() > 0;
				case difficulty::sync:
					return sync->queue.size_approx() > 0;
				case difficulty::timeout:
					return timeouts->queue.size() > 0;
				default:
					return false;
			}
		}
		bool schedule::has_any_tasks() const
		{
			return has_tasks(difficulty::sync) || has_tasks(difficulty::async) || has_tasks(difficulty::timeout);
		}
		bool schedule::is_suspended() const
		{
			return suspended;
		}
		void schedule::suspend()
		{
			suspended = true;
		}
		void schedule::resume()
		{
			suspended = false;
			wakeup();
		}
		bool schedule::report_thread(thread_task state, size_t tasks, const thread_data* thread)
		{
			if (!debug)
				return false;

			debug(thread_message(thread, state, tasks));
			return true;
		}
		bool schedule::fast_bypass_enqueue(difficulty type, task_callback&& callback)
		{
			if (!has_parallel_threads(type))
				return false;

			auto* thread = (thread_data*)initialize_thread(nullptr, false);
			if (!thread || thread->type != type || thread->queue.size() >= policy.max_recycles)
				return false;

			thread->queue.push(std::move(callback));
			return true;
		}
		size_t schedule::get_thread_global_index()
		{
			auto* thread = get_thread();
			return thread ? thread->global_index : 0;
		}
		size_t schedule::get_thread_local_index()
		{
			auto* thread = get_thread();
			return thread ? thread->local_index : 0;
		}
		size_t schedule::get_total_threads() const
		{
			size_t size = 0;
			for (size_t i = 0; i < (size_t)difficulty::count; i++)
				size += get_threads((difficulty)i);

			return size;
		}
		size_t schedule::get_threads(difficulty type) const
		{
			VI_ASSERT(type != difficulty::count, "difficulty should be set");
			return threads[(size_t)type].size();
		}
		bool schedule::has_parallel_threads(difficulty type) const
		{
			VI_ASSERT(type != difficulty::count, "difficulty should be set");
			return threads[(size_t)type].size() > 1;
		}
		void schedule::initialize_spawn_trigger()
		{
			if (!policy.initialize)
				return;

			bool is_pending = true;
			std::recursive_mutex awaiting_mutex;
			std::unique_lock<std::recursive_mutex> unique(awaiting_mutex);
			std::condition_variable_any ready;
			policy.initialize([&awaiting_mutex, &ready, &is_pending]()
			{
				std::unique_lock<std::recursive_mutex> unique(awaiting_mutex);
				is_pending = false;
				ready.notify_all();
			});
			ready.wait(unique, [&is_pending]() { return !is_pending; });
		}
		const schedule::thread_data* schedule::initialize_thread(thread_data* source, bool update) const
		{
			static thread_local thread_data* internal_thread = nullptr;
			if (update)
				internal_thread = source;
			return internal_thread;
		}
		const schedule::thread_data* schedule::get_thread() const
		{
			return initialize_thread(nullptr, false);
		}
		const schedule::desc& schedule::get_policy() const
		{
			return policy;
		}
		std::chrono::microseconds schedule::get_timeout(std::chrono::microseconds clock)
		{
			while (timeouts->queue.find(clock) != timeouts->queue.end())
				++clock;
			return clock;
		}
		std::chrono::microseconds schedule::get_clock()
		{
			return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
		}
		bool schedule::is_available(difficulty type)
		{
			if (!has_instance())
				return false;

			auto* instance = get();
			if (!instance->active || !instance->enqueue)
				return false;

			return type == difficulty::count || instance->has_parallel_threads(type);
		}

		schema::schema(const variant& base) noexcept : nodes(nullptr), parent(nullptr), saved(true), value(base)
		{
		}
		schema::schema(variant&& base) noexcept : nodes(nullptr), parent(nullptr), saved(true), value(std::move(base))
		{
		}
		schema::~schema() noexcept
		{
			unlink();
			clear();
		}
		unordered_map<string, size_t> schema::get_names() const
		{
			size_t index = 0;
			unordered_map<string, size_t> mapping;
			generate_naming_table(this, &mapping, index);
			return mapping;
		}
		vector<schema*> schema::find_collection(const std::string_view& name, bool deep) const
		{
			vector<schema*> result;
			if (!nodes)
				return result;

			for (auto value : *nodes)
			{
				if (value->key == name)
					result.push_back(value);

				if (!deep)
					continue;

				vector<schema*> init = value->find_collection(name);
				for (auto& subvalue : init)
					result.push_back(subvalue);
			}

			return result;
		}
		vector<schema*> schema::fetch_collection(const std::string_view& notation, bool deep) const
		{
			vector<string> names = stringify::split(notation, '.');
			if (names.empty())
				return vector<schema*>();

			if (names.size() == 1)
				return find_collection(*names.begin());

			schema* current = find(*names.begin(), deep);
			if (!current)
				return vector<schema*>();

			for (auto it = names.begin() + 1; it != names.end() - 1; ++it)
			{
				current = current->find(*it, deep);
				if (!current)
					return vector<schema*>();
			}

			return current->find_collection(*(names.end() - 1), deep);
		}
		vector<schema*> schema::get_attributes() const
		{
			vector<schema*> attributes;
			if (!nodes)
				return attributes;

			for (auto it : *nodes)
			{
				if (it->is_attribute())
					attributes.push_back(it);
			}

			return attributes;
		}
		vector<schema*>& schema::get_childs()
		{
			allocate();
			return *nodes;
		}
		schema* schema::find(const std::string_view& name, bool deep) const
		{
			if (!nodes)
				return nullptr;

			if (stringify::has_integer(name))
			{
				size_t index = (size_t)*from_string<uint64_t>(name);
				if (index < nodes->size())
					return (*nodes)[index];
			}

			for (auto k : *nodes)
			{
				if (k->key == name)
					return k;

				if (!deep)
					continue;

				schema* v = k->find(name);
				if (v != nullptr)
					return v;
			}

			return nullptr;
		}
		schema* schema::fetch(const std::string_view& notation, bool deep) const
		{
			vector<string> names = stringify::split(notation, '.');
			if (names.empty())
				return nullptr;

			schema* current = find(*names.begin(), deep);
			if (!current)
				return nullptr;

			for (auto it = names.begin() + 1; it != names.end(); ++it)
			{
				current = current->find(*it, deep);
				if (!current)
					return nullptr;
			}

			return current;
		}
		schema* schema::get_parent() const
		{
			return parent;
		}
		schema* schema::get_attribute(const std::string_view& name) const
		{
			return get(':' + string(name));
		}
		variant schema::fetch_var(const std::string_view& fkey, bool deep) const
		{
			schema* result = fetch(fkey, deep);
			if (!result)
				return var::undefined();

			return result->value;
		}
		variant schema::get_var(size_t index) const
		{
			schema* result = get(index);
			if (!result)
				return var::undefined();

			return result->value;
		}
		variant schema::get_var(const std::string_view& fkey) const
		{
			schema* result = get(fkey);
			if (!result)
				return var::undefined();

			return result->value;
		}
		variant schema::get_attribute_var(const std::string_view& key) const
		{
			return get_var(':' + string(key));
		}
		schema* schema::get(size_t index) const
		{
			VI_ASSERT(nodes != nullptr, "there must be at least one node");
			VI_ASSERT(index < nodes->size(), "index outside of range");

			return (*nodes)[index];
		}
		schema* schema::get(const std::string_view& name) const
		{
			VI_ASSERT(!name.empty(), "name should not be empty");
			if (!nodes)
				return nullptr;

			for (auto schema : *nodes)
			{
				if (schema->key == name)
					return schema;
			}

			return nullptr;
		}
		schema* schema::set(const std::string_view& name)
		{
			return set(name, var::object());
		}
		schema* schema::set(const std::string_view& name, const variant& base)
		{
			if (value.type == var_type::object && nodes != nullptr)
			{
				for (auto node : *nodes)
				{
					if (node->key == name)
					{
						node->value = base;
						node->saved = false;
						node->clear();
						saved = false;

						return node;
					}
				}
			}

			schema* result = new schema(base);
			result->key.assign(name);
			result->attach(this);

			allocate();
			nodes->push_back(result);
			return result;
		}
		schema* schema::set(const std::string_view& name, variant&& base)
		{
			if (value.type == var_type::object && nodes != nullptr)
			{
				for (auto node : *nodes)
				{
					if (node->key == name)
					{
						node->value = std::move(base);
						node->saved = false;
						node->clear();
						saved = false;

						return node;
					}
				}
			}

			schema* result = new schema(std::move(base));
			result->key.assign(name);
			result->attach(this);

			allocate();
			nodes->push_back(result);
			return result;
		}
		schema* schema::set(const std::string_view& name, schema* base)
		{
			if (!base)
				return set(name, var::null());

			base->key.assign(name);
			base->attach(this);

			if (value.type == var_type::object && nodes != nullptr)
			{
				for (auto it = nodes->begin(); it != nodes->end(); ++it)
				{
					if ((*it)->key != name)
						continue;

					if (*it == base)
						return base;

					(*it)->parent = nullptr;
					memory::release(*it);
					*it = base;
					return base;
				}
			}

			allocate();
			nodes->push_back(base);
			return base;
		}
		schema* schema::set_attribute(const std::string_view& name, const variant& fvalue)
		{
			return set(':' + string(name), fvalue);
		}
		schema* schema::set_attribute(const std::string_view& name, variant&& fvalue)
		{
			return set(':' + string(name), std::move(fvalue));
		}
		schema* schema::push(const variant& base)
		{
			schema* result = new schema(base);
			result->attach(this);

			allocate();
			nodes->push_back(result);
			return result;
		}
		schema* schema::push(variant&& base)
		{
			schema* result = new schema(std::move(base));
			result->attach(this);

			allocate();
			nodes->push_back(result);
			return result;
		}
		schema* schema::push(schema* base)
		{
			if (!base)
				return push(var::null());

			base->attach(this);
			allocate();
			nodes->push_back(base);
			return base;
		}
		schema* schema::pop(size_t index)
		{
			VI_ASSERT(nodes != nullptr, "there must be at least one node");
			VI_ASSERT(index < nodes->size(), "index outside of range");

			auto it = nodes->begin() + index;
			schema* base = *it;
			base->parent = nullptr;
			memory::release(base);
			nodes->erase(it);
			saved = false;

			return this;
		}
		schema* schema::pop(const std::string_view& name)
		{
			if (!nodes)
				return this;

			for (auto it = nodes->begin(); it != nodes->end(); ++it)
			{
				if (!*it || (*it)->key != name)
					continue;

				(*it)->parent = nullptr;
				memory::release(*it);
				nodes->erase(it);
				saved = false;
				break;
			}

			return this;
		}
		schema* schema::copy() const
		{
			schema* init = new schema(value);
			init->key.assign(key);
			init->saved = saved;

			if (!nodes)
				return init;

			init->allocate(*nodes);
			for (auto*& item : *init->nodes)
			{
				if (item != nullptr)
					item = item->copy();
			}

			return init;
		}
		bool schema::rename(const std::string_view& name, const std::string_view& new_name)
		{
			VI_ASSERT(!name.empty() && !new_name.empty(), "name and new name should not be empty");

			schema* result = get(name);
			if (!result)
				return false;

			result->key = new_name;
			return true;
		}
		bool schema::has(const std::string_view& name) const
		{
			return fetch(name) != nullptr;
		}
		bool schema::has_attribute(const std::string_view& name) const
		{
			return fetch(':' + string(name)) != nullptr;
		}
		bool schema::empty() const
		{
			return !nodes || nodes->empty();
		}
		bool schema::is_attribute() const
		{
			if (key.size() < 2)
				return false;

			return key.front() == ':';
		}
		bool schema::is_saved() const
		{
			return saved;
		}
		size_t schema::size() const
		{
			return nodes ? nodes->size() : 0;
		}
		string schema::get_name() const
		{
			return is_attribute() ? key.substr(1) : key;
		}
		void schema::join(schema* other, bool append_only)
		{
			VI_ASSERT(other != nullptr && value.is_object(), "other should be object and not empty");
			auto fill_arena = [](unordered_map<string, schema*>& nodes, schema* base)
			{
				if (!base->nodes)
					return;

				for (auto& node : *base->nodes)
				{
					auto& next = nodes[node->key];
					memory::release(next);
					next = node;
				}

				base->nodes->clear();
			};

			allocate();
			nodes->reserve(nodes->size() + other->nodes->size());
			saved = false;

			if (!append_only)
			{
				unordered_map<string, schema*> subnodes;
				subnodes.reserve(nodes->capacity());
				fill_arena(subnodes, this);
				fill_arena(subnodes, other);

				for (auto& node : subnodes)
					nodes->push_back(node.second);
			}
			else if (other->nodes != nullptr)
			{
				nodes->insert(nodes->end(), other->nodes->begin(), other->nodes->end());
				other->nodes->clear();
			}

			for (auto& node : *nodes)
			{
				node->saved = false;
				node->parent = this;
			}
		}
		void schema::reserve(size_t size)
		{
			allocate();
			nodes->reserve(size);
		}
		void schema::unlink()
		{
			if (!parent)
				return;

			if (!parent->nodes)
			{
				parent = nullptr;
				return;
			}

			for (auto it = parent->nodes->begin(); it != parent->nodes->end(); ++it)
			{
				if (*it == this)
				{
					parent->nodes->erase(it);
					break;
				}
			}

			parent = nullptr;
		}
		void schema::clear()
		{
			if (!nodes)
				return;

			for (auto& next : *nodes)
			{
				if (next != nullptr)
				{
					next->parent = nullptr;
					memory::release(next);
				}
			}

			memory::deinit(nodes);
			nodes = nullptr;
		}
		void schema::save()
		{
			if (nodes != nullptr)
			{
				for (auto& it : *nodes)
				{
					if (it->value.is_object())
						it->save();
					else
						it->saved = true;
				}
			}

			saved = true;
		}
		void schema::attach(schema* root)
		{
			saved = false;
			if (parent != nullptr && parent->nodes != nullptr)
			{
				for (auto it = parent->nodes->begin(); it != parent->nodes->end(); ++it)
				{
					if (*it == this)
					{
						parent->nodes->erase(it);
						break;
					}
				}
			}

			parent = root;
			if (parent != nullptr)
				parent->saved = false;
		}
		void schema::allocate()
		{
			if (!nodes)
				nodes = memory::init<vector<schema*>>();
		}
		void schema::allocate(const vector<schema*>& other)
		{
			if (!nodes)
				nodes = memory::init<vector<schema*>>(other);
			else
				*nodes = other;
		}
		void schema::transform(schema* value, const schema_name_callback& callback)
		{
			VI_ASSERT(!!callback, "callback should not be empty");
			if (!value)
				return;

			value->key = callback(value->key);
			if (!value->nodes)
				return;

			for (auto* item : *value->nodes)
				transform(item, callback);
		}
		void schema::convert_to_xml(schema* base, const schema_write_callback& callback)
		{
			VI_ASSERT(base != nullptr && callback, "base should be set and callback should not be empty");
			vector<schema*> attributes = base->get_attributes();
			bool scalable = (base->value.size() > 0 || ((size_t)(base->nodes ? base->nodes->size() : 0) > (size_t)attributes.size()));
			callback(var_form::write_tab, "");
			callback(var_form::dummy, "<");
			callback(var_form::dummy, base->key);

			if (attributes.empty())
			{
				if (scalable)
					callback(var_form::dummy, ">");
				else
					callback(var_form::dummy, " />");
			}
			else
				callback(var_form::dummy, " ");

			for (auto it = attributes.begin(); it != attributes.end(); ++it)
			{
				string key = (*it)->get_name();
				string value = (*it)->value.serialize();

				callback(var_form::dummy, key);
				callback(var_form::dummy, "=\"");
				callback(var_form::dummy, value);
				++it;

				if (it == attributes.end())
				{
					if (!scalable)
					{
						callback(var_form::write_space, "\"");
						callback(var_form::dummy, "/>");
					}
					else
						callback(var_form::dummy, "\">");
				}
				else
					callback(var_form::write_space, "\"");

				--it;
			}

			callback(var_form::tab_increase, "");
			if (base->value.size() > 0)
			{
				string text = base->value.serialize();
				if (base->nodes != nullptr && !base->nodes->empty())
				{
					callback(var_form::write_line, "");
					callback(var_form::write_tab, "");
					callback(var_form::dummy, text);
					callback(var_form::write_line, "");
				}
				else
					callback(var_form::dummy, text);
			}
			else
				callback(var_form::write_line, "");

			if (base->nodes != nullptr)
			{
				for (auto&& it : *base->nodes)
				{
					if (!it->is_attribute())
						convert_to_xml(it, callback);
				}
			}

			callback(var_form::tab_decrease, "");
			if (!scalable)
				return;

			if (base->nodes != nullptr && !base->nodes->empty())
				callback(var_form::write_tab, "");

			callback(var_form::dummy, "</");
			callback(var_form::dummy, base->key);
			callback(base->parent ? var_form::write_line : var_form::dummy, ">");
		}
		void schema::convert_to_json(schema* base, const schema_write_callback& callback)
		{
			VI_ASSERT(base != nullptr && callback, "base should be set and callback should not be empty");
			if (!base->value.is_object())
			{
				string value = base->value.serialize();
				stringify::escape(value);

				if (base->value.type != var_type::string && base->value.type != var_type::binary)
				{
					if (value.size() >= 2 && value.front() == PREFIX_ENUM[0] && value.back() == PREFIX_ENUM[0])
						callback(var_form::dummy, value.substr(1, value.size() - 2));
					else
						callback(var_form::dummy, value);
				}
				else
				{
					callback(var_form::dummy, "\"");
					callback(var_form::dummy, value);
					callback(var_form::dummy, "\"");
				}
				return;
			}

			size_t size = (base->nodes ? base->nodes->size() : 0);
			bool array = (base->value.type == var_type::array);
			if (!size)
			{
				callback(var_form::dummy, array ? "[]" : "{}");
				return;
			}

			callback(var_form::dummy, array ? "[" : "{");
			callback(var_form::tab_increase, "");

			for (size_t i = 0; i < size; i++)
			{
				auto* next = (*base->nodes)[i];
				if (!array)
				{
					callback(var_form::write_line, "");
					callback(var_form::write_tab, "");
					callback(var_form::dummy, "\"");
					callback(var_form::dummy, next->key);
					callback(var_form::write_space, "\":");
				}

				if (!next->value.is_object())
				{
					auto type = next->value.get_type();
					string value = (type == var_type::undefined ? "null" : next->value.serialize());
					stringify::escape(value);
					if (array)
					{
						callback(var_form::write_line, "");
						callback(var_form::write_tab, "");
					}

					if (type == var_type::decimal)
					{
						bool big_number = !((decimal*)next->value.get_container())->is_safe_number();
						if (big_number)
							callback(var_form::dummy, "\"");

						if (value.size() >= 2 && value.front() == PREFIX_ENUM[0] && value.back() == PREFIX_ENUM[0])
							callback(var_form::dummy, value.substr(1, value.size() - 2));
						else
							callback(var_form::dummy, value);

						if (big_number)
							callback(var_form::dummy, "\"");
					}
					else if (!next->value.is_object() && type != var_type::string && type != var_type::binary)
					{
						if (value.size() >= 2 && value.front() == PREFIX_ENUM[0] && value.back() == PREFIX_ENUM[0])
							callback(var_form::dummy, value.substr(1, value.size() - 2));
						else
							callback(var_form::dummy, value);
					}
					else
					{
						callback(var_form::dummy, "\"");
						callback(var_form::dummy, value);
						callback(var_form::dummy, "\"");
					}
				}
				else
				{
					if (array)
					{
						callback(var_form::write_line, "");
						callback(var_form::write_tab, "");
					}
					convert_to_json(next, callback);
				}

				if (i + 1 < size)
					callback(var_form::dummy, ",");
			}

			callback(var_form::tab_decrease, "");
			callback(var_form::write_line, "");
			if (base->parent != nullptr)
				callback(var_form::write_tab, "");

			callback(var_form::dummy, array ? "]" : "}");
		}
		void schema::convert_to_jsonb(schema* base, const schema_write_callback& callback)
		{
			VI_ASSERT(base != nullptr && callback, "base should be set and callback should not be empty");
			unordered_map<string, size_t> mapping = base->get_names();
			uint32_t set = os::hw::to_endianness(os::hw::endian::little, (uint32_t)mapping.size());
			uint64_t version = os::hw::to_endianness<uint64_t>(os::hw::endian::little, JSONB_VERSION);
			callback(var_form::dummy, std::string_view((const char*)&version, sizeof(uint64_t)));
			callback(var_form::dummy, std::string_view((const char*)&set, sizeof(uint32_t)));

			for (auto it = mapping.begin(); it != mapping.end(); ++it)
			{
				uint32_t id = os::hw::to_endianness(os::hw::endian::little, (uint32_t)it->second);
				callback(var_form::dummy, std::string_view((const char*)&id, sizeof(uint32_t)));

				uint16_t size = os::hw::to_endianness(os::hw::endian::little, (uint16_t)it->first.size());
				callback(var_form::dummy, std::string_view((const char*)&size, sizeof(uint16_t)));

				if (size > 0)
					callback(var_form::dummy, std::string_view(it->first.c_str(), sizeof(char) * (size_t)size));
			}
			process_convertion_to_jsonb(base, &mapping, callback);
		}
		string schema::to_xml(schema* value)
		{
			string result;
			convert_to_xml(value, [&](var_form type, const std::string_view& buffer) { result.append(buffer); });
			return result;
		}
		string schema::to_json(schema* value)
		{
			string result;
			convert_to_json(value, [&](var_form type, const std::string_view& buffer) { result.append(buffer); });
			return result;
		}
		vector<char> schema::to_jsonb(schema* value)
		{
			vector<char> result;
			convert_to_jsonb(value, [&](var_form type, const std::string_view& buffer)
			{
				if (buffer.empty())
					return;

				size_t offset = result.size();
				result.resize(offset + buffer.size());
				memcpy(&result[offset], buffer.data(), buffer.size());
			});
			return result;
		}
		expects_parser<schema*> schema::convert_from_xml(const std::string_view& buffer)
		{
#ifdef VI_PUGIXML
			if (buffer.empty())
				return parser_exception(parser_error::xml_no_document_element, 0, "empty XML buffer");

			pugi::xml_document data;
			pugi::xml_parse_result status = data.load_buffer(buffer.data(), buffer.size());
			if (!status)
			{
				switch (status.status)
				{
					case pugi::status_out_of_memory:
						return parser_exception(parser_error::xml_out_of_memory, (size_t)status.offset, status.description());
					case pugi::status_internal_error:
						return parser_exception(parser_error::xml_internal_error, (size_t)status.offset, status.description());
					case pugi::status_unrecognized_tag:
						return parser_exception(parser_error::xml_unrecognized_tag, (size_t)status.offset, status.description());
					case pugi::status_bad_pi:
						return parser_exception(parser_error::xml_bad_pi, (size_t)status.offset, status.description());
					case pugi::status_bad_comment:
						return parser_exception(parser_error::xml_bad_comment, (size_t)status.offset, status.description());
					case pugi::status_bad_cdata:
						return parser_exception(parser_error::xml_bad_cdata, (size_t)status.offset, status.description());
					case pugi::status_bad_doctype:
						return parser_exception(parser_error::xml_bad_doc_type, (size_t)status.offset, status.description());
					case pugi::status_bad_pcdata:
						return parser_exception(parser_error::xml_bad_pc_data, (size_t)status.offset, status.description());
					case pugi::status_bad_start_element:
						return parser_exception(parser_error::xml_bad_start_element, (size_t)status.offset, status.description());
					case pugi::status_bad_attribute:
						return parser_exception(parser_error::xml_bad_attribute, (size_t)status.offset, status.description());
					case pugi::status_bad_end_element:
						return parser_exception(parser_error::xml_bad_end_element, (size_t)status.offset, status.description());
					case pugi::status_end_element_mismatch:
						return parser_exception(parser_error::xml_end_element_mismatch, (size_t)status.offset, status.description());
					case pugi::status_append_invalid_root:
						return parser_exception(parser_error::xml_append_invalid_root, (size_t)status.offset, status.description());
					case pugi::status_no_document_element:
						return parser_exception(parser_error::xml_no_document_element, (size_t)status.offset, status.description());
					default:
						return parser_exception(parser_error::xml_internal_error, (size_t)status.offset, status.description());
				}
			}

			pugi::xml_node main = data.first_child();
			schema* result = var::set::array();
			process_convertion_from_xml((void*)&main, result);
			return result;
#else
			return parser_exception(parser_error::not_supported, 0, "no capabilities to parse XML");
#endif
		}
		expects_parser<schema*> schema::convert_from_json(const std::string_view& buffer)
		{
#ifdef VI_RAPIDJSON
			if (buffer.empty())
				return parser_exception(parser_error::json_document_empty, 0);

			rapidjson::Document base;
			base.Parse<rapidjson::kParseNumbersAsStringsFlag>(buffer.data(), buffer.size());

			schema* result = nullptr;
			if (base.HasParseError())
			{
				size_t offset = base.GetErrorOffset();
				switch (base.GetParseError())
				{
					case rapidjson::kParseErrorDocumentEmpty:
						return parser_exception(parser_error::json_document_empty, offset);
					case rapidjson::kParseErrorDocumentRootNotSingular:
						return parser_exception(parser_error::json_document_root_not_singular, offset);
					case rapidjson::kParseErrorValueInvalid:
						return parser_exception(parser_error::json_value_invalid, offset);
					case rapidjson::kParseErrorObjectMissName:
						return parser_exception(parser_error::json_object_miss_name, offset);
					case rapidjson::kParseErrorObjectMissColon:
						return parser_exception(parser_error::json_object_miss_colon, offset);
					case rapidjson::kParseErrorObjectMissCommaOrCurlyBracket:
						return parser_exception(parser_error::json_object_miss_comma_or_curly_bracket, offset);
					case rapidjson::kParseErrorArrayMissCommaOrSquareBracket:
						return parser_exception(parser_error::json_array_miss_comma_or_square_bracket, offset);
					case rapidjson::kParseErrorStringUnicodeEscapeInvalidHex:
						return parser_exception(parser_error::json_string_unicode_escape_invalid_hex, offset);
					case rapidjson::kParseErrorStringUnicodeSurrogateInvalid:
						return parser_exception(parser_error::json_string_unicode_surrogate_invalid, offset);
					case rapidjson::kParseErrorStringEscapeInvalid:
						return parser_exception(parser_error::json_string_escape_invalid, offset);
					case rapidjson::kParseErrorStringMissQuotationMark:
						return parser_exception(parser_error::json_string_miss_quotation_mark, offset);
					case rapidjson::kParseErrorStringInvalidEncoding:
						return parser_exception(parser_error::json_string_invalid_encoding, offset);
					case rapidjson::kParseErrorNumberTooBig:
						return parser_exception(parser_error::json_number_too_big, offset);
					case rapidjson::kParseErrorNumberMissFraction:
						return parser_exception(parser_error::json_number_miss_fraction, offset);
					case rapidjson::kParseErrorNumberMissExponent:
						return parser_exception(parser_error::json_number_miss_exponent, offset);
					case rapidjson::kParseErrorTermination:
						return parser_exception(parser_error::json_termination, offset);
					case rapidjson::kParseErrorUnspecificSyntaxError:
						return parser_exception(parser_error::json_unspecific_syntax_error, offset);
					default:
						return parser_exception(parser_error::bad_value);
				}
			}

			rapidjson::Type type = base.GetType();
			switch (type)
			{
				case rapidjson::kNullType:
					result = new schema(var::null());
					break;
				case rapidjson::kFalseType:
					result = new schema(var::boolean(false));
					break;
				case rapidjson::kTrueType:
					result = new schema(var::boolean(true));
					break;
				case rapidjson::kObjectType:
					result = var::set::object();
					process_convertion_from_json((void*)&base, result);
					break;
				case rapidjson::kArrayType:
					result = var::set::array();
					process_convertion_from_json((void*)&base, result);
					break;
				case rapidjson::kStringType:
					result = process_conversion_from_json_string_or_number(&base, true);
					break;
				case rapidjson::kNumberType:
					if (base.IsInt())
						result = new schema(var::integer(base.GetInt64()));
					else
						result = new schema(var::number(base.GetDouble()));
					break;
				default:
					result = new schema(var::undefined());
					break;
			}

			return result;
#else
			return parser_exception(parser_error::not_supported, 0, "no capabilities to parse JSON");
#endif
		}
		expects_parser<schema*> schema::convert_from_jsonb(const schema_read_callback& callback)
		{
			VI_ASSERT(callback, "callback should not be empty");
			uint64_t version = 0;
			if (!callback((uint8_t*)&version, sizeof(uint64_t)))
				return parser_exception(parser_error::bad_version);

			version = os::hw::to_endianness<uint64_t>(os::hw::endian::little, version);
			if (version != JSONB_VERSION)
				return parser_exception(parser_error::bad_version);

			uint32_t set = 0;
			if (!callback((uint8_t*)&set, sizeof(uint32_t)))
				return parser_exception(parser_error::bad_dictionary);

			unordered_map<size_t, string> map;
			set = os::hw::to_endianness(os::hw::endian::little, set);

			for (uint32_t i = 0; i < set; ++i)
			{
				uint32_t index = 0;
				if (!callback((uint8_t*)&index, sizeof(uint32_t)))
					return parser_exception(parser_error::bad_name_index);

				uint16_t size = 0;
				if (!callback((uint8_t*)&size, sizeof(uint16_t)))
					return parser_exception(parser_error::bad_name);

				index = os::hw::to_endianness(os::hw::endian::little, index);
				size = os::hw::to_endianness(os::hw::endian::little, size);

				if (size <= 0)
					continue;

				string name;
				name.resize((size_t)size);
				if (!callback((uint8_t*)name.c_str(), sizeof(char) * size))
					return parser_exception(parser_error::bad_name);

				map.insert({ index, name });
			}

			uptr<schema> current = var::set::object();
			auto status = process_convertion_from_jsonb(*current, &map, callback);
			if (!status)
				return status.error();

			return current.reset();
		}
		expects_parser<schema*> schema::from_xml(const std::string_view& text)
		{
			return convert_from_xml(text);
		}
		expects_parser<schema*> schema::from_json(const std::string_view& text)
		{
			return convert_from_json(text);
		}
		expects_parser<schema*> schema::from_jsonb(const std::string_view& binary)
		{
			size_t offset = 0;
			return convert_from_jsonb([&binary, &offset](uint8_t* buffer, size_t length)
			{
				if (offset + length > binary.size())
					return false;

				memcpy((void*)buffer, binary.data() + offset, length);
				offset += length;
				return true;
			});
		}
		expects<void, parser_exception> schema::process_convertion_from_jsonb(schema* current, unordered_map<size_t, string>* map, const schema_read_callback& callback)
		{
			uint32_t id = 0;
			if (!callback((uint8_t*)&id, sizeof(uint32_t)))
				return parser_exception(parser_error::bad_key_name);

			if (id != (uint32_t)-1)
			{
				auto it = map->find((size_t)os::hw::to_endianness(os::hw::endian::little, id));
				if (it != map->end())
					current->key = it->second;
			}

			if (!callback((uint8_t*)&current->value.type, sizeof(var_type)))
				return parser_exception(parser_error::bad_key_type);

			switch (current->value.type)
			{
				case var_type::object:
				case var_type::array:
				{
					uint32_t count = 0;
					if (!callback((uint8_t*)&count, sizeof(uint32_t)))
						return parser_exception(parser_error::bad_value);

					count = os::hw::to_endianness(os::hw::endian::little, count);
					if (!count)
						break;

					current->allocate();
					current->nodes->resize((size_t)count);

					for (auto*& item : *current->nodes)
					{
						item = var::set::object();
						item->parent = current;
						item->saved = true;

						auto status = process_convertion_from_jsonb(item, map, callback);
						if (!status)
							return status;
					}
					break;
				}
				case var_type::string:
				{
					uint32_t size = 0;
					if (!callback((uint8_t*)&size, sizeof(uint32_t)))
						return parser_exception(parser_error::bad_value);

					string buffer;
					size = os::hw::to_endianness(os::hw::endian::little, size);
					buffer.resize((size_t)size);

					if (!callback((uint8_t*)buffer.c_str(), (size_t)size * sizeof(char)))
						return parser_exception(parser_error::bad_string);

					current->value = var::string(buffer);
					break;
				}
				case var_type::binary:
				{
					uint32_t size = 0;
					if (!callback((uint8_t*)&size, sizeof(uint32_t)))
						return parser_exception(parser_error::bad_value);

					string buffer;
					size = os::hw::to_endianness(os::hw::endian::little, size);
					buffer.resize(size);

					if (!callback((uint8_t*)buffer.c_str(), (size_t)size * sizeof(char)))
						return parser_exception(parser_error::bad_string);

					current->value = var::binary((uint8_t*)buffer.data(), buffer.size());
					break;
				}
				case var_type::integer:
				{
					int64_t integer = 0;
					if (!callback((uint8_t*)&integer, sizeof(int64_t)))
						return parser_exception(parser_error::bad_integer);

					current->value = var::integer(os::hw::to_endianness(os::hw::endian::little, integer));
					break;
				}
				case var_type::number:
				{
					double number = 0.0;
					if (!callback((uint8_t*)&number, sizeof(double)))
						return parser_exception(parser_error::bad_double);

					current->value = var::number(os::hw::to_endianness(os::hw::endian::little, number));
					break;
				}
				case var_type::decimal:
				{
					uint16_t size = 0;
					if (!callback((uint8_t*)&size, sizeof(uint16_t)))
						return parser_exception(parser_error::bad_value);

					string buffer;
					size = os::hw::to_endianness(os::hw::endian::little, size);
					buffer.resize((size_t)size);

					if (!callback((uint8_t*)buffer.c_str(), (size_t)size * sizeof(char)))
						return parser_exception(parser_error::bad_string);

					current->value = var::decimal_string(buffer);
					break;
				}
				case var_type::boolean:
				{
					bool boolean = false;
					if (!callback((uint8_t*)&boolean, sizeof(bool)))
						return parser_exception(parser_error::bad_boolean);

					current->value = var::boolean(boolean);
					break;
				}
				default:
					break;
			}

			return core::expectation::met;
		}
		schema* schema::process_conversion_from_json_string_or_number(void* base, bool is_document)
		{
#ifdef VI_RAPIDJSON
			const char* buffer = (is_document ? ((rapidjson::Document*)base)->GetString() : ((rapidjson::Value*)base)->GetString());
			size_t size = (is_document ? ((rapidjson::Document*)base)->GetStringLength() : ((rapidjson::Value*)base)->GetStringLength());
			string text(buffer, size);

			if (!stringify::has_number(text))
				return new schema(var::string(text));

			if (stringify::has_decimal(text))
				return new schema(var::decimal_string(text));

			if (stringify::has_integer(text))
			{
				auto number = from_string<int64_t>(text);
				if (number)
					return new schema(var::integer(*number));
			}
			else
			{
				auto number = from_string<double>(text);
				if (number)
					return new schema(var::number(*number));
			}

			return new schema(var::string(text));
#else
			return var::set::undefined();
#endif
		}
		void schema::process_convertion_from_xml(void* base, schema* current)
		{
#ifdef VI_PUGIXML
			VI_ASSERT(base != nullptr && current != nullptr, "base and current should be set");
			pugi::xml_node& next = *(pugi::xml_node*)base;
			current->key = next.name();

			for (auto attribute : next.attributes())
				current->set_attribute(attribute.name(), attribute.empty() ? var::null() : var::any(attribute.value()));

			for (auto child : next.children())
			{
				schema* subresult = current->set(child.name(), var::set::array());
				process_convertion_from_xml((void*)&child, subresult);

				if (*child.value() != '\0')
				{
					subresult->value.deserialize(child.value());
					continue;
				}

				auto text = child.text();
				if (!text.empty())
					subresult->value.deserialize(child.text().get());
				else
					subresult->value = var::null();
			}
#endif
		}
		void schema::process_convertion_from_json(void* base, schema* current)
		{
#ifdef VI_RAPIDJSON
			VI_ASSERT(base != nullptr && current != nullptr, "base and current should be set");
			auto child = (rapidjson::Value*)base;
			if (!child->IsArray())
			{
				string name;
				current->reserve((size_t)child->MemberCount());

				var_type type = current->value.type;
				current->value.type = var_type::array;

				for (auto it = child->MemberBegin(); it != child->MemberEnd(); ++it)
				{
					if (!it->name.IsString())
						continue;

					name.assign(it->name.GetString(), (size_t)it->name.GetStringLength());
					switch (it->value.GetType())
					{
						case rapidjson::kNullType:
							current->set(name, var::null());
							break;
						case rapidjson::kFalseType:
							current->set(name, var::boolean(false));
							break;
						case rapidjson::kTrueType:
							current->set(name, var::boolean(true));
							break;
						case rapidjson::kObjectType:
							process_convertion_from_json((void*)&it->value, current->set(name));
							break;
						case rapidjson::kArrayType:
							process_convertion_from_json((void*)&it->value, current->set(name, var::array()));
							break;
						case rapidjson::kStringType:
							current->set(name, process_conversion_from_json_string_or_number(&it->value, false));
							break;
						case rapidjson::kNumberType:
							if (it->value.IsInt())
								current->set(name, var::integer(it->value.GetInt64()));
							else
								current->set(name, var::number(it->value.GetDouble()));
							break;
						default:
							break;
					}
				}

				current->value.type = type;
			}
			else
			{
				string value;
				current->reserve((size_t)child->Size());

				for (auto it = child->Begin(); it != child->End(); ++it)
				{
					switch (it->GetType())
					{
						case rapidjson::kNullType:
							current->push(var::null());
							break;
						case rapidjson::kFalseType:
							current->push(var::boolean(false));
							break;
						case rapidjson::kTrueType:
							current->push(var::boolean(true));
							break;
						case rapidjson::kObjectType:
							process_convertion_from_json((void*)it, current->push(var::object()));
							break;
						case rapidjson::kArrayType:
							process_convertion_from_json((void*)it, current->push(var::array()));
							break;
						case rapidjson::kStringType:
						{
							const char* buffer = it->GetString(); size_t size = it->GetStringLength();
							if (size < 2 || *buffer != PREFIX_BINARY[0] || buffer[size - 1] != PREFIX_BINARY[0])
								current->push(process_conversion_from_json_string_or_number((void*)it, false));
							else
								current->push(var::binary((uint8_t*)buffer + 1, size - 2));
							break;
						}
						case rapidjson::kNumberType:
							if (it->IsInt())
								current->push(var::integer(it->GetInt64()));
							else
								current->push(var::number(it->GetDouble()));
							break;
						default:
							break;
					}
				}
			}
#endif
		}
		void schema::process_convertion_to_jsonb(schema* current, unordered_map<string, size_t>* map, const schema_write_callback& callback)
		{
			uint32_t id = os::hw::to_endianness(os::hw::endian::little, current->key.empty() ? (uint32_t)-1 : (uint32_t)map->at(current->key));
			callback(var_form::dummy, std::string_view((const char*)&id, sizeof(uint32_t)));
			callback(var_form::dummy, std::string_view((const char*)&current->value.type, sizeof(var_type)));

			switch (current->value.type)
			{
				case var_type::object:
				case var_type::array:
				{
					uint32_t count = os::hw::to_endianness(os::hw::endian::little, (uint32_t)(current->nodes ? current->nodes->size() : 0));
					callback(var_form::dummy, std::string_view((const char*)&count, sizeof(uint32_t)));
					if (count > 0)
					{
						for (auto& schema : *current->nodes)
							process_convertion_to_jsonb(schema, map, callback);
					}
					break;
				}
				case var_type::string:
				case var_type::binary:
				{
					uint32_t size = os::hw::to_endianness(os::hw::endian::little, (uint32_t)current->value.size());
					callback(var_form::dummy, std::string_view((const char*)&size, sizeof(uint32_t)));
					callback(var_form::dummy, std::string_view(current->value.get_string().data(), size * sizeof(char)));
					break;
				}
				case var_type::decimal:
				{
					string number = ((decimal*)current->value.value.pointer)->to_string();
					uint16_t size = os::hw::to_endianness(os::hw::endian::little, (uint16_t)number.size());
					callback(var_form::dummy, std::string_view((const char*)&size, sizeof(uint16_t)));
					callback(var_form::dummy, std::string_view(number.c_str(), (size_t)size * sizeof(char)));
					break;
				}
				case var_type::integer:
				{
					int64_t value = os::hw::to_endianness(os::hw::endian::little, current->value.value.integer);
					callback(var_form::dummy, std::string_view((const char*)&value, sizeof(int64_t)));
					break;
				}
				case var_type::number:
				{
					double value = os::hw::to_endianness(os::hw::endian::little, current->value.value.number);
					callback(var_form::dummy, std::string_view((const char*)&value, sizeof(double)));
					break;
				}
				case var_type::boolean:
				{
					callback(var_form::dummy, std::string_view((const char*)&current->value.value.boolean, sizeof(bool)));
					break;
				}
				default:
					break;
			}
		}
		void schema::generate_naming_table(const schema* current, unordered_map<string, size_t>* map, size_t& index)
		{
			if (!current->key.empty())
			{
				auto m = map->find(current->key);
				if (m == map->end())
					map->insert({ current->key, index++ });
			}

			if (!current->nodes)
				return;

			for (auto schema : *current->nodes)
				generate_naming_table(schema, map, index);
		}
	}
}
#pragma warning(pop)
