#ifndef ED_CONFIG_HPP
#define ED_CONFIG_HPP
#pragma warning(disable: 4251)
#if defined(_WIN32) || defined(_WIN64)
#define ED_MICROSOFT 1
#define ED_FILENO _fileno
#define ED_SPLITTER '\\'
#ifndef ED_EXPORT
#define ED_OUT __declspec(dllimport)
#else
#define ED_OUT __declspec(dllexport)
#endif
#define ED_OUT_TS ED_OUT
#ifdef _WIN64
#define ED_64 1
#endif
#ifdef ED_USE_FCTX
#define ED_COCALL
#define ED_CODATA void* Context
#else
#define ED_COCALL __stdcall
#define ED_CODATA void* Context
#endif
typedef size_t socket_t;
typedef int socket_size_t;
typedef void* epoll_handle;
#endif
#if defined(__APPLE__) && defined(__MACH__)
#define _XOPEN_SOURCE
#define ED_APPLE 1
#define ED_LINUX 1
#define ED_OUT
#define ED_OUT_TS
#define ED_FILENO fileno
#define ED_SPLITTER '/'
#if __x86_64__ || __ppc64__
#define ED_64 1
#endif
#ifdef ED_USE_FCTX
#define ED_COCALL
#define ED_CODATA void* Context
#else
#define ED_COCALL
#define ED_CODATA int X, int Y
#endif
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR == 1 || TARGET_OS_IPHONE == 1
#define ED_IOS 1
#elif TARGET_OS_MAC == 1
#define ED_OSX 1
#endif
#include <sys/types.h>
#include <sys/socket.h>
typedef int epoll_handle;
typedef int socket_t;
typedef socklen_t socket_size_t;
#endif
#if !defined(ED_APPLE) && (defined(__linux__) || defined(__linux)  || defined(linux) || defined(__unix__) || defined(__unix) || defined(unix) || defined(__FreeBSD__) || defined(__ANDROID__))
#define ED_LINUX 1
#define ED_OUT
#define ED_OUT_TS
#define ED_FILENO fileno
#define ED_SPLITTER '/'
#ifdef __ANDROID__
#define ED_ANDROID 1
#endif
#if __x86_64__ || __ppc64__
#define ED_64 1
#endif
#ifdef ED_USE_FCTX
#define ED_COCALL
#define ED_CODATA void* Context
#else
#define ED_COCALL
#define ED_CODATA int X, int Y
#endif
#include <sys/types.h>
#include <sys/socket.h>
typedef int epoll_handle;
typedef int socket_t;
typedef socklen_t socket_size_t;
#endif
#if ED_CXX >= 17
#if __cplusplus >= 201703L || _MSVC_LANG >= 201703L || defined(_HAS_CXX17)
#define ED_CXX17 1
#endif
#endif
#if ED_CXX >= 20
#if __cplusplus >= 202002L || _MSVC_LANG >= 202002L || defined(_HAS_CXX20)
#define ED_CXX20 1
#endif
#endif
#if defined(ED_CXX17) && defined(ED_MICROSOFT)
#include <execution>
#define ED_SORT(Begin, End, Comparator) std::sort(std::execution::par_unseq, Begin, End, Comparator)
#else
#define ED_SORT(Begin, End, Comparator) std::sort(Begin, End, Comparator)
#endif
#ifdef NDEBUG
#if ED_DLEVEL >= 5
#define ED_TRACE(Format, ...) Edge::Core::OS::Log((int)Edge::Core::LogLevel::Trace, 0, nullptr, Format, ##__VA_ARGS__)
#else
#define ED_TRACE(Format, ...) ((void)0)
#endif
#if ED_DLEVEL >= 4
#define ED_DEBUG(Format, ...) Edge::Core::OS::Log((int)Edge::Core::LogLevel::Debug, 0, nullptr, Format, ##__VA_ARGS__)
#else
#define ED_DEBUG(Format, ...) ((void)0)
#endif
#if ED_DLEVEL >= 3
#define ED_INFO(Format, ...) Edge::Core::OS::Log((int)Edge::Core::LogLevel::Info, 0, nullptr, Format, ##__VA_ARGS__)
#else
#define ED_INFO(Format, ...) ((void)0)
#endif
#if ED_DLEVEL >= 2
#define ED_WARN(Format, ...) Edge::Core::OS::Log((int)Edge::Core::LogLevel::Warning, 0, nullptr, Format, ##__VA_ARGS__)
#else
#define ED_WARN(Format, ...) ((void)0)
#endif
#if ED_DLEVEL >= 1
#define ED_ERR(Format, ...) Edge::Core::OS::Log((int)Edge::Core::LogLevel::Error, 0, nullptr, Format, ##__VA_ARGS__)
#else
#define ED_ERR(Format, ...) ((void)0)
#endif
#define ED_ASSERT(Condition, Returnable, Format, ...) ((void)0)
#define ED_ASSERT_V(Condition, Format, ...) ((void)0)
#define ED_MEASURE(Threshold) ((void)0)
#define ED_MEASURE_LOOP() ((void)0)
#define ED_WATCH(Ptr, Label) ((void)0)
#define ED_WATCH_AT(Ptr, Function, Label) ((void)0)
#define ED_UNWATCH(Ptr) ((void)0)
#define ED_MALLOC(Type, Size) (Type*)Edge::Core::Memory::Malloc(Size)
#define ED_NEW(Type, ...) new((void*)ED_MALLOC(Type, sizeof(Type))) Type(__VA_ARGS__)
#ifndef ED_CXX20
#define ED_AWAIT(Value) Edge::Core::Coawait(Value)
#endif
#else
#if ED_DLEVEL >= 5
#define ED_TRACE(Format, ...) Edge::Core::OS::Log((int)Edge::Core::LogLevel::Trace, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define ED_TRACE(Format, ...) ((void)0)
#endif
#if ED_DLEVEL >= 4
#define ED_DEBUG(Format, ...) Edge::Core::OS::Log((int)Edge::Core::LogLevel::Debug, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define ED_DEBUG(Format, ...) ((void)0)
#endif
#if ED_DLEVEL >= 3
#define ED_INFO(Format, ...) Edge::Core::OS::Log((int)Edge::Core::LogLevel::Info, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define ED_INFO(Format, ...) ((void)0)
#endif
#if ED_DLEVEL >= 2
#define ED_WARN(Format, ...) Edge::Core::OS::Log((int)Edge::Core::LogLevel::Warning, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#else
#define ED_WARN(Format, ...) ((void)0)
#endif
#if ED_DLEVEL >= 1
#define ED_ERR(Format, ...) Edge::Core::OS::Log((int)Edge::Core::LogLevel::Error, __LINE__, __FILE__, Format, ##__VA_ARGS__)
#define ED_ASSERT(Condition, Returnable, Format, ...) if (!(Condition)) { Edge::Core::OS::Assert(true, __LINE__, __FILE__, __func__, #Condition, Format, ##__VA_ARGS__); return Returnable; }
#define ED_ASSERT_V(Condition, Format, ...) if (!(Condition)) { Edge::Core::OS::Assert(true, __LINE__, __FILE__, __func__, #Condition, Format, ##__VA_ARGS__); return; }
#else
#define ED_ERR(Format, ...) ((void)0)
#define ED_ASSERT(Condition, Returnable, Format, ...) if (!(Condition)) { Edge::Core::OS::Process::Interrupt(); return Returnable; }
#define ED_ASSERT_V(Condition, Format, ...) if (!(Condition)) { Edge::Core::OS::Process::Interrupt(); return; }
#endif
#define ED_MEASURE_START(X) _measure_line_##X
#define ED_MEASURE_PREPARE(X) ED_MEASURE_START(X)
#define ED_MEASURE(Threshold) auto ED_MEASURE_PREPARE(__LINE__) = Edge::Core::OS::Timing::Measure(__FILE__, __func__, __LINE__, (uint64_t)(Threshold))
#define ED_MEASURE_LOOP() Edge::Core::OS::Timing::MeasureLoop()
#define ED_WATCH(Ptr, Label) Edge::Core::Memory::Watch(Ptr, Edge::Core::MemoryContext(__FILE__, __func__, Label, __LINE__))
#define ED_WATCH_AT(Ptr, Function, Label) Edge::Core::Memory::Watch(Ptr, Edge::Core::MemoryContext(__FILE__, Function, Label, __LINE__))
#define ED_UNWATCH(Ptr) Edge::Core::Memory::Unwatch(Ptr)
#define ED_MALLOC(Type, Size) (Type*)Edge::Core::Memory::MallocContext(Size, Edge::Core::MemoryContext(__FILE__, __func__, typeid(Type).name(), __LINE__))
#define ED_NEW(Type, ...) new((void*)ED_MALLOC(Type, sizeof(Type))) Type(__VA_ARGS__)
#ifndef ED_CXX20
#define ED_AWAIT(Value) Edge::Core::Coawait(Value, __func__, #Value)
#endif
#endif
#define ED_DELETE(Destructor, Var) { if (Var != nullptr) { (Var)->~Destructor(); ED_FREE((void*)Var); } }
#define ED_FREE(Ptr) Edge::Core::Memory::Free(Ptr)
#define ED_RELEASE(Ptr) { if (Ptr != nullptr) (Ptr)->Release(); }
#define ED_CLEAR(Ptr) { if (Ptr != nullptr) { (Ptr)->Release(); Ptr = nullptr; } }
#define ED_ASSIGN(FromPtr, ToPtr) { (FromPtr) = ToPtr; if (FromPtr != nullptr) (FromPtr)->AddRef(); }
#define ED_REASSIGN(FromPtr, ToPtr) { ED_RELEASE(FromPtr); (FromPtr) = ToPtr; if (FromPtr != nullptr) (FromPtr)->AddRef(); }
#define ED_HASH(Name) Edge::Core::OS::File::GetHash(Name)
#define ED_SHUFFLE(Name) Edge::Core::OS::File::GetIndex<sizeof(Name)>(Name)
#define ED_STRINGIFY(Text) #Text
#define ED_COMPONENT_ROOT(Name) virtual const char* GetName() { return Name; } virtual uint64_t GetId() { static uint64_t V = ED_HASH(Name); return V; } static const char* GetTypeName() { return Name; } static uint64_t GetTypeId() { static uint64_t V = ED_HASH(Name); return V; }
#define ED_COMPONENT(Name) virtual const char* GetName() override { return Name; } virtual uint64_t GetId() override { static uint64_t V = ED_HASH(Name); return V; } static const char* GetTypeName() { return Name; } static uint64_t GetTypeId() { static uint64_t V = ED_HASH(Name); return V; }
#ifdef ED_CXX20
#define ED_AWAIT(Value) (co_await (Value))
#define Coawait(Value) (co_await (Value))
#define Coreturn co_return
#define CoreturnVoid co_return
#else
#define Coreturn return
#define CoreturnVoid return Edge::Core::Promise<void>::Ready()
#endif
#endif