#ifndef VI_CONFIG_HPP
#define VI_CONFIG_HPP
#pragma warning(disable: 4251)
#if defined(_WIN32) || defined(_WIN64)
#define VI_MICROSOFT 1
#define VI_FILENO _fileno
#define VI_SPLITTER '\\'
#ifdef _WIN64
#define VI_64 1
#endif
#ifdef VI_FCONTEXT
#define VI_COCALL
#define VI_CODATA void* context
#else
#define VI_COCALL __stdcall
#define VI_CODATA void* context
#endif
typedef size_t socket_t;
typedef int socket_size_t;
typedef void* epoll_handle;
#endif
#if defined(__APPLE__) && defined(__MACH__)
#define _XOPEN_SOURCE
#define VI_APPLE 1
#define VI_LINUX 1
#define VI_FILENO fileno
#define VI_SPLITTER '/'
#if defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
#define VI_64 1
#endif
#ifdef VI_FCONTEXT
#define VI_COCALL
#define VI_CODATA void* context
#else
#define VI_COCALL
#define VI_CODATA int x, int y
#endif
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1 || TARGET_OS_IPHONE == 1
#define VI_IOS 1
#elif TARGET_OS_MAC == 1
#define VI_OSX 1
#endif
#include <sys/types.h>
#include <sys/socket.h>
typedef int epoll_handle;
typedef int socket_t;
typedef socklen_t socket_size_t;
#endif
#if !defined(VI_APPLE) && (defined(__linux__) || defined(__linux)  || defined(linux) || defined(__unix__) || defined(__unix) || defined(unix) || defined(__FreeBSD__) || defined(__ANDROID__) || (defined(__sun) && defined(__SVR4)))
#define VI_LINUX 1
#define VI_FILENO fileno
#define VI_SPLITTER '/'
#ifdef __ANDROID__
#define VI_ANDROID 1
#endif
#ifdef __FreeBSD__
#define VI_FREEBSD 1
#endif
#if defined(__sun) && defined(__SVR4)
#define VI_SOLARIS 1
#endif
#if defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
#define VI_64 1
#endif
#ifdef VI_FCONTEXT
#define VI_COCALL
#define VI_CODATA void* context
#else
#define VI_COCALL
#define VI_CODATA int x, int y
#endif
#include <sys/types.h>
#include <sys/socket.h>
typedef int epoll_handle;
typedef int socket_t;
typedef socklen_t socket_size_t;
#endif
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || defined(VI_ENDIAN_BIG) || defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#define VI_ENDIAN_BIG
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || defined(VI_ENDIAN_LITTLE) || defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__) || defined(_WIN32) || defined(__i386__) || defined(__x86_64__) || defined(_X86_) || defined(_IA64_)
#define VI_ENDIAN_LITTLE
#endif
#if __cplusplus >= 201703L || _MSVC_LANG >= 201703L || defined(_HAS_CXX17)
#define VI_CXX17 1
#else
#error "C++14 or older is now not supported"
#endif
#if VI_CXX >= 20
#if __cplusplus >= 202002L || _MSVC_LANG >= 202002L || defined(_HAS_CXX20)
#define VI_CXX20 1
#endif
#endif
#if VI_CXX >= 23
#if __cplusplus >= 202004L || _MSVC_LANG >= 202004L || defined(_HAS_CXX23)
#define VI_CXX23 1
#endif
#endif
#ifdef VI_MICROSOFT
#include <execution>
#define VI_SORT(begin, end, comparator) std::sort(std::execution::par_unseq, begin, end, comparator)
#else
#define VI_SORT(begin, end, comparator) std::sort(begin, end, comparator)
#endif
#ifdef NDEBUG
#ifdef VI_PESSIMISTIC
#define VI_ASSERT(condition, format, ...) if (!(condition)) { vitex::core::error_handling::panic(__LINE__, __FILE__, __func__, #condition, format, ##__VA_ARGS__); }
#else
#define VI_ASSERT(condition, format, ...) ((void)0)
#endif
#define VI_MEASURE(threshold) ((void)0)
#define VI_MEASURE_LOOP() ((void)0)
#define VI_WATCH(ptr, label) ((void)0)
#define VI_WATCH_AT(ptr, function, label) ((void)0)
#define VI_UNWATCH(ptr) ((void)0)
#ifndef VI_CXX20
#define VI_AWAIT(value) vitex::core::coawait(value)
#endif
#else
#define VI_ASSERT(condition, format, ...) if (!(condition)) { vitex::core::error_handling::assertion(__LINE__, __FILE__, __func__, #condition, format, ##__VA_ARGS__); }
#define VI_MEASURE_START(x) _measure_line_##x
#define VI_MEASURE_PREPARE(x) VI_MEASURE_START(x)
#define VI_MEASURE(threshold) auto VI_MEASURE_PREPARE(__LINE__) = vitex::core::error_handling::measure(__FILE__, __func__, __LINE__, (uint64_t)(threshold))
#define VI_MEASURE_LOOP() vitex::core::error_handling::measure_loop()
#define VI_WATCH(ptr, label) vitex::core::memory::watch(ptr, vitex::core::memory_location(__FILE__, __func__, label, __LINE__))
#define VI_WATCH_AT(ptr, function, label) vitex::core::memory::watch(ptr, vitex::core::memory_location(__FILE__, function, label, __LINE__))
#define VI_UNWATCH(ptr) vitex::core::memory::unwatch(ptr)
#ifndef VI_CXX20
#define VI_AWAIT(value) vitex::core::coawait(value, __func__, #value)
#endif
#endif
#if VI_DLEVEL >= 5
#define VI_TRACE(format, ...) vitex::core::error_handling::message(vitex::core::log_level::trace, __LINE__, __FILE__, format, ##__VA_ARGS__)
#else
#define VI_TRACE(format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 4
#define VI_DEBUG(format, ...) vitex::core::error_handling::message(vitex::core::log_level::debug, __LINE__, __FILE__, format, ##__VA_ARGS__)
#else
#define VI_DEBUG(format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 3
#define VI_INFO(format, ...) vitex::core::error_handling::message(vitex::core::log_level::info, __LINE__, __FILE__, format, ##__VA_ARGS__)
#else
#define VI_INFO(format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 2
#define VI_WARN(format, ...) vitex::core::error_handling::message(vitex::core::log_level::warning, __LINE__, __FILE__, format, ##__VA_ARGS__)
#else
#define VI_WARN(format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 1
#define VI_ERR(format, ...) vitex::core::error_handling::message(vitex::core::log_level::error, __LINE__, __FILE__, format, ##__VA_ARGS__)
#else
#define VI_ERR(format, ...) ((void)0)
#endif
#define VI_PANIC(condition, format, ...) if (!(condition)) { vitex::core::error_handling::panic(__LINE__, __FILE__, __func__, #condition, format, ##__VA_ARGS__); }
#define VI_HASH(name) vitex::core::os::file::get_hash(name)
#define VI_STRINGIFY(text) #text
#define VI_COMPONENT_ROOT(name) virtual const char* get_name() { return name; } virtual uint64_t get_id() { static uint64_t v = VI_HASH(name); return v; } static const char* get_type_name() { return name; } static uint64_t get_type_id() { static uint64_t v = VI_HASH(name); return v; }
#define VI_COMPONENT(name) virtual const char* get_name() override { return name; } virtual uint64_t get_id() override { static uint64_t v = VI_HASH(name); return v; } static const char* get_type_name() { return name; } static uint64_t get_type_id() { static uint64_t v = VI_HASH(name); return v; }
#ifdef VI_CXX20
#define VI_AWAIT(value) (co_await (value))
#define coawait(value) (co_await (value))
#define coreturn co_return
#define coreturn_void co_return
#else
#define coreturn return
#define coreturn_void return vitex::core::promise<void>::null()
#endif
#endif
