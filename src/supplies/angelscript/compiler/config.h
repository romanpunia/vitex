#ifndef COMPILER_CORE_H
#define COMPILER_CORE_H
#ifdef VI_HAS_JIT
#if defined(_WIN32) || defined(_WIN64)
#ifndef _M_ARM
#define HAS_AS_JIT
#define HAS_AS_JIT_WINDOWS
#ifdef _WIN64
#define HAS_AS_JIT_64
#else
#define HAS_AS_JIT_32
#endif
#endif
#elif defined __linux__ && defined __GNUC__
#ifndef __arm__
#define HAS_AS_JIT
#define HAS_AS_JIT_UNIX
#if __x86_64__ || __ppc64__
#define HAS_AS_JIT_64
#else
#define HAS_AS_JIT_32
#endif
#endif
#endif
#endif
#endif