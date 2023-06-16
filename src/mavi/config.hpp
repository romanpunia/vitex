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
#ifdef VI_FCTX
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
#if __x86_64__ || __ppc64__
#define VI_64 1
#endif
#ifdef VI_FCTX
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
#if __x86_64__ || __ppc64__
#define VI_64 1
#endif
#ifdef VI_FCTX
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
#if VI_CXX >= 17
#if __cplusplus >= 201703L || _MSVC_LANG >= 201703L || defined(_HAS_CXX17)
#define VI_CXX17 1
#endif
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
#if defined(VI_CXX17) && defined(VI_MICROSOFT)
#include <execution>
#define VI_SORT(Begin, End, Comparator) std::sort(std::execution::par_unseq, Begin, End, Comparator)
#else
#define VI_SORT(Begin, End, Comparator) std::sort(Begin, End, Comparator)
#endif
#ifdef NDEBUG
#define VI_ASSERT(Condition, Format, ...) ((void)0)
#define VI_MEASURE(Threshold) ((void)0)
#define VI_MEASURE_LOOP() ((void)0)
#define VI_WATCH(Ptr, Label) ((void)0)
#define VI_WATCH_AT(Ptr, Function, Label) ((void)0)
#define VI_UNWATCH(Ptr) ((void)0)
#define VI_MALLOC(Type, Size) (Type*)Mavi::Core::Memory::Malloc(Size)
#define VI_NEW(Type, ...) new((void*)VI_MALLOC(Type, sizeof(Type))) Type(__VA_ARGS__)
#ifndef VI_CXX20
#define VI_AWAIT(Value) Mavi::Core::Coawait(Value)
#endif
#else
#define VI_ASSERT(Condition, Format, ...) if (!(Condition)) { Mavi::Core::ErrorHandling::Assert(__LINE__, __FILE__, __func__, #Condition, Format, ##__VA_ARGS__); }
#define VI_MEASURE_START(X) _measure_line_##X
#define VI_MEASURE_PREPARE(X) VI_MEASURE_START(X)
#define VI_MEASURE(Threshold) auto VI_MEASURE_PREPARE(__LINE__) = Mavi::Core::ErrorHandling::Measure(__FILE__, __func__, __LINE__, (uint64_t)(Threshold))
#define VI_MEASURE_LOOP() Mavi::Core::ErrorHandling::MeasureLoop()
#define VI_WATCH(Ptr, Label) Mavi::Core::Memory::Watch(Ptr, Mavi::Core::MemoryContext(__FILE__, __func__, Label, __LINE__))
#define VI_WATCH_AT(Ptr, Function, Label) Mavi::Core::Memory::Watch(Ptr, Mavi::Core::MemoryContext(__FILE__, Function, Label, __LINE__))
#define VI_UNWATCH(Ptr) Mavi::Core::Memory::Unwatch(Ptr)
#define VI_MALLOC(Type, Size) (Type*)Mavi::Core::Memory::MallocContext(Size, Mavi::Core::MemoryContext(__FILE__, __func__, typeid(Type).name(), __LINE__))
#define VI_NEW(Type, ...) new((void*)VI_MALLOC(Type, sizeof(Type))) Type(__VA_ARGS__)
#ifndef VI_CXX20
#define VI_AWAIT(Value) Mavi::Core::Coawait(Value, __func__, #Value)
#endif
#endif
#if VI_DLEVEL >= 5
#define VI_TRACE(Format, ...) Mavi::Core::ErrorHandling::Message(Mavi::Core::LogLevel::Trace, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define VI_TRACE(Format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 4
#define VI_DEBUG(Format, ...) Mavi::Core::ErrorHandling::Message(Mavi::Core::LogLevel::Debug, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define VI_DEBUG(Format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 3
#define VI_INFO(Format, ...) Mavi::Core::ErrorHandling::Message(Mavi::Core::LogLevel::Info, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define VI_INFO(Format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 2
#define VI_WARN(Format, ...) Mavi::Core::ErrorHandling::Message(Mavi::Core::LogLevel::Warning, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define VI_WARN(Format, ...) ((void)0)
#endif
#if VI_DLEVEL >= 1
#define VI_ERR(Format, ...) Mavi::Core::ErrorHandling::Message(Mavi::Core::LogLevel::Error, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define VI_ERR(Format, ...) ((void)0)
#endif
#define VI_PANIC(Condition, Format, ...) if (!(Condition)) { Mavi::Core::ErrorHandling::Panic(__LINE__, __FILE__, __func__, #Condition, Format, ##__VA_ARGS__); }
#define VI_DELETE(Destructor, Var) { if (Var) { (Var)->~Destructor(); VI_FREE((void*)Var); } }
#define VI_FREE(Ptr) Mavi::Core::Memory::Free(Ptr)
#define VI_RELEASE(Ptr) { if (Ptr) (Ptr)->Release(); }
#define VI_CLEAR(Ptr) { if (Ptr) { (Ptr)->Release(); Ptr = nullptr; } }
#define VI_HASH(Name) Mavi::Core::OS::File::GetHash(Name)
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
#define CoreturnVoid return Mavi::Core::Promise<void>::Null()
#endif
#endif