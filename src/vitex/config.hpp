#ifndef VI_CONFIG_HPP
#define VI_CONFIG_HPP
#pragma warning(disable: 4251)
#if defined(_WIN32) || defined(_WIN64)
#define VI_MICROSOFT 1
#define VI_FILENO _fileno
#define VI_SPLITTER '\\'
#define VI_EXPOSE __declspec(dllexport)
#ifndef VI_EXPORT
#define VI_OUT __declspec(dllimport)
#else
#define VI_OUT __declspec(dllexport)
#endif
#define VI_OUT_TS VI_OUT
#ifdef _WIN64
#define VI_64 1
#endif
#ifdef VI_FCONTEXT
#define VI_COCALL
#define VI_CODATA void* Context
#else
#define VI_COCALL __stdcall
#define VI_CODATA void* Context
#endif
typedef size_t socket_t;
typedef int socket_size_t;
typedef void* epoll_handle;
#endif
#if defined(__APPLE__) && defined(__MACH__)
#define _XOPEN_SOURCE
#define VI_APPLE 1
#define VI_LINUX 1
#define VI_EXPOSE
#define VI_OUT
#define VI_OUT_TS
#define VI_FILENO fileno
#define VI_SPLITTER '/'
#if defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__)
#define VI_64 1
#endif
#ifdef VI_FCONTEXT
#define VI_COCALL
#define VI_CODATA void* Context
#else
#define VI_COCALL
#define VI_CODATA int X, int Y
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
#define VI_EXPOSE
#define VI_OUT
#define VI_OUT_TS
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
#define VI_CODATA void* Context
#else
#define VI_COCALL
#define VI_CODATA int X, int Y
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
#define VI_SORT(Begin, End, Comparator) std::sort(std::execution::par_unseq, Begin, End, Comparator)
#else
#define VI_SORT(Begin, End, Comparator) std::sort(Begin, End, Comparator)
#endif
#ifdef NDEBUG
#ifdef VI_PESSIMISTIC
#define VI_ASSERT(Condition, Format, ...) if (!(Condition)) { Vitex::Core::ErrorHandling::Panic(__LINE__, __FILE__, __func__, #Condition, Format, ##__VA_ARGS__); }
#else
#define VI_ASSERT(Condition, Format, ...) ((void)0)
#endif
#define VI_MEASURE(Threshold) ((void)0)
#define VI_MEASURE_LOOP() ((void)0)
#define VI_WATCH(Ptr, Label) ((void)0)
#define VI_WATCH_AT(Ptr, Function, Label) ((void)0)
#define VI_UNWATCH(Ptr) ((void)0)
#ifndef VI_CXX20
#define VI_AWAIT(Value) Vitex::Core::Coawait(Value)
#endif
#else
#define VI_ASSERT(Condition, Format, ...) if (!(Condition)) { Vitex::Core::ErrorHandling::Assert(__LINE__, __FILE__, __func__, #Condition, Format, ##__VA_ARGS__); }
#define VI_MEASURE_START(X) _measure_line_##X
#define VI_MEASURE_PREPARE(X) VI_MEASURE_START(X)
#define VI_MEASURE(Threshold) auto VI_MEASURE_PREPARE(__LINE__) = Vitex::Core::ErrorHandling::Measure(__FILE__, __func__, __LINE__, (uint64_t)(Threshold))
#define VI_MEASURE_LOOP() Vitex::Core::ErrorHandling::MeasureLoop()
#define VI_WATCH(Ptr, Label) Vitex::Core::Memory::Watch(Ptr, Vitex::Core::MemoryLocation(__FILE__, __func__, Label, __LINE__))
#define VI_WATCH_AT(Ptr, Function, Label) Vitex::Core::Memory::Watch(Ptr, Vitex::Core::MemoryLocation(__FILE__, Function, Label, __LINE__))
#define VI_UNWATCH(Ptr) Vitex::Core::Memory::Unwatch(Ptr)
#ifndef VI_CXX20
#define VI_AWAIT(Value) Vitex::Core::Coawait(Value, __func__, #Value)
#endif
#endif
#if VI_DLEVEL >= 5
#define VI_TRACE(Format, ...) Vitex::Core::ErrorHandling::Message(Vitex::Core::LogLevel::Trace, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define VI_TRACE(Format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 4
#define VI_DEBUG(Format, ...) Vitex::Core::ErrorHandling::Message(Vitex::Core::LogLevel::Debug, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define VI_DEBUG(Format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 3
#define VI_INFO(Format, ...) Vitex::Core::ErrorHandling::Message(Vitex::Core::LogLevel::Info, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define VI_INFO(Format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 2
#define VI_WARN(Format, ...) Vitex::Core::ErrorHandling::Message(Vitex::Core::LogLevel::Warning, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define VI_WARN(Format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 1
#define VI_ERR(Format, ...) Vitex::Core::ErrorHandling::Message(Vitex::Core::LogLevel::Error, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define VI_ERR(Format, ...) ((void)0)
#endif
#define VI_PANIC(Condition, Format, ...) if (!(Condition)) { Vitex::Core::ErrorHandling::Panic(__LINE__, __FILE__, __func__, #Condition, Format, ##__VA_ARGS__); }
#define VI_HASH(Name) Vitex::Core::OS::File::GetHash(Name)
#define VI_STRINGIFY(Text) #Text
#define VI_COMPONENT_ROOT(Name) virtual const char* GetName() { return Name; } virtual uint64_t GetId() { static uint64_t V = VI_HASH(Name); return V; } static const char* GetTypeName() { return Name; } static uint64_t GetTypeId() { static uint64_t V = VI_HASH(Name); return V; }
#define VI_COMPONENT(Name) virtual const char* GetName() override { return Name; } virtual uint64_t GetId() override { static uint64_t V = VI_HASH(Name); return V; } static const char* GetTypeName() { return Name; } static uint64_t GetTypeId() { static uint64_t V = VI_HASH(Name); return V; }
#ifdef VI_CXX20
#define VI_AWAIT(Value) (co_await (Value))
#define Coawait(Value) (co_await (Value))
#define Coreturn co_return
#define CoreturnVoid co_return
#else
#define Coreturn return
#define CoreturnVoid return Vitex::Core::Promise<void>::Null()
#endif
#endif
