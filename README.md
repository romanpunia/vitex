<br/>
<div align="center">
    <br />
    <img src="https://github.com/romanpunia/vitex/blob/master/var/assets/logo.png?raw=true" alt="Vitex Logo" width="300" />
    <h3>C++ Cross-Platform Framework</h3>
</div>
<div align="center">

  [![Build Status](https://github.com/romanpunia/vitex/workflows/CMake/badge.svg)](https://github.com/eclipse-theia/theia/actions?query=branch%3Amaster+event%3Apush+event%3Aschedule)

</div>

## About 👻
Vitex is a cross-platform C++17/20/23 framework targeted for sync/async/promise client/server development. Applications developed with Vitex are CPU and memory optimized for high load. In it's core, Vitex is based on the same concepts as in Node.js but made to it's extreme by utilizing powers of raw C++.

This framework introduces quite a few important features from popular game engines, languages like Rust and runtimes like Node.js, including (but not limited to): core agnostic scheduler with blocking/non-blocking/coroutine/timeout/interval tasks, optionals, expectations (no try/catch blocks ever), text serialization formats, ready-to-use coroutines and promises, control over OS level access, common networking protocols, memory tracing and management, stacktraces, logging, data compression, big numbers, high precision integers, cryptographic primitives for different purposes: hashing, symmetric/asymmetric encryption and blockchain required cryptography.

## Details
Vitex's core is based on a worker pool that consumes and publishes tasks, there is no concept of main event loop, every thread is on it's own an event loop. Scheduler processes blocking and non-blocking tasks such as event dispatching, non-blocking IO and coroutines, CPU bound tasks and blocking IO. Vitex is made to be as scalable as possible without even thinking about scalability which in turn makes development of multithreaded or concurrent systems quite a lot easier.

Rule of thumb:
1. Split up work into small pieces.
2. Schedule those small pieces into the queue.

[Game engine fork - Vengeance.](https://github.com/romanpunia/vengeance)

## Documentation
You may take a look at Wiki pages.

## Cross platform (tested platforms)
+ Windows 7/8/8.1/10+ x64/x86 (MSVC, MSVC ClangCL, MinGW)
+ Raspberian 3+ ARM (GCC)
+ Solaris 9+ x64/x86 (GCC)
+ FreeBSD 11+ x64/x86 (GCC)
+ Debian 11+ x64/x86 (GCC, LLVM)
+ Ubuntu 16.04+ x64/x86 (GCC)
+ MacOS Catalina 10.15+ x64 (Xcode)

## Building
There are several ways to build this project that are explained here:
* [Manually performed builds](var/MANUAL.md)
* [Precomposed docker builds](var/DOCKER.md)

### Configuration
+ **VI_CXX** is the C++ standard (17, 20, 23) which will be used, C++20 and higher will replace stateful coroutines with std coroutines, defaults to 20
+ **VI_LOGGING** is a logging level (errors, warnings, default, debug, verbose), defaults to "default"
+ **VI_PESSIMISTIC** will enable assertion statements in release mode, defaults to OFF
+ **VI_BINDINGS** will enable full script bindings otherwise only essentials will be used to reduce lib size, defaults to ON
+ **VI_ALLOCATOR** will enable custom allocator for all used standard containers, making them incompatible with std::allocator based ones but adding opportunity to use pool allocator, defaults to ON
+ **VI_FCONTEXT** will enable internal fcontext implementation for coroutines, defaults to ON

## Dependencies
* [concurrentqueue (submodule)](https://github.com/cameron314/concurrentqueue)

## Serialization features
* [pugixml (submodule)](https://github.com/zeux/pugixml): VI_PUGIXML=ON
* [rapidjson (submodule)](https://github.com/tencent/rapidjson): VI_RAPIDJSON=ON
* [zlib (so)](https://github.com/madler/zlib): VI_ZLIB=ON

## Networking features
* [wepoll (submodule)](https://github.com/piscisaureus/wepoll): VI_WEPOLL=ON
* [openssl (so)](https://github.com/openssl/openssl): VI_OPENSSL=ON
* [mongo-c-driver (so)](https://github.com/mongodb/mongo-c-driver): VI_MONGOC=ON
* [libpq (so)](https://github.com/postgres/postgres/tree/master/src/interfaces/libpq): VI_POSTGRESQL=ON
* [sqlite (so)](https://github.com/sqlite/sqlite): VI_SQLITE=ON

## Other features
* Scripting [angelscript (submodule)](https://github.com/codecat/angelscript-mirror): VI_ANGELSCRIPT=ON
* Debugging [backward-cpp (submodule)](https://github.com/bombela/backward-cpp): VI_BACKWARDCPP=ON

## License
Vitex is licensed under the MIT license

## Known Issues
Documentation in it's usual form is non-existant at the moment. In the nearest future that could be changed.