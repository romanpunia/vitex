<br/>
<div align="center">
    <br />
    <img src="https://github.com/romanpunia/mavi/blob/master/var/logo.png?raw=true" alt="Mavi Logo" width="300" />
    <h3>C++ Cross-Platform Framework</h3>
</div>
<div align="center">

  [![Build Status](https://github.com/romanpunia/mavi/workflows/CMake/badge.svg)](https://github.com/eclipse-theia/theia/actions?query=branch%3Amaster+event%3Apush+event%3Aschedule)

</div>

## About 👻
Mavi is a cross-platform C++14/17/20/23 framework to create any type of application from a unified interface. In it's core, Mavi is based on same concepts as Node.js but made to it's extreme. As in Node, Mavi has a worker pool that not only consumes but also publishes tasks, here we don't really have a concept of event loop, every thread is on it's own an event loop.

## Details
Using concept of tasks and queues, in Mavi there are two rules for optimal performace and proper CPU loading:
1. Split work in to small pieces.
2. Use scheduler to process those small pieces.

There are two types of thread workers in a thread pool: light and heavy. Light threads processes non-blocking tasks such as event dispatching, non-blocking IO and coroutines. Heavy threads are in charge of everything else such as CPU bound tasks and blocking IO. Mavi is made to be as scalable as possible without even thinking about scalability which in turn makes development of multithreaded systems quite a lot easier.

Originally, Mavi was only a game engine but now it isn't. All the features for game development are there but it can be easily stripped out to reduce size of executable and use only needed functionality. There are cases when Mavi is used as a framework for building a high performance backend server or a daemon, most of the time all we need in that case is OpenSSL and maybe Zlib with PostgreSQL or MongoDB, so we strip out everything else that way reducing compile time, executable size and runtime memory usage to minimum. Turns out, Mavi can easily run in a very limited machines that were slow even in 2006.

If your goal is to make a game, Mavi is not really a consumer ready product to build AAA games as it is very low level. I tried to make it so that if you need some functionality such as a new entity component, advanced rendering techinque such as Ray-Tracing, a new rendering backend or even a motion capture system to create cutscenes, you can make it in a relatively short time without hurting performance. Why isn't it already contained in this engine? I'm the only developer, physically impossible.

For games, Mavi is a 3D optimized engine, there is a posibility to render efficient 2D graphics with batching but that is not a priority. Main shading language is HLSL, it is transpiled or directly compiled and saved to cache when needed. Rendering is based on stacking, that way you specify renderers for a camera (order matters), when rendering is initiated we process each renderer step by step, renderer can initiate sub-pass to render another part of scene for shadows, reflections, transparency and others. Before rendering begins, render lists are prepared for geometry rendering based on culling results, for sub-passes we use frustum and indexed culling, for main passes we also use occlusion culling. Culling, physics, audio and rendering are done multhreaded if possible. In best case, we don't have any synchronization between threads but when we need to write to shared data,
for example, destroy a rigid body owned by physics engine, we use scene transactions that are just callbacks that are guaranteed to be executed thread safe (in scope of scene) which in turn makes scene eventual consistent as transactions are fired later when all parallel tasks are finished.

Another important aspect of Mavi is schemas, they are used to serialize and deserialize data. For game, their main purpose is to provide containers for serialized game states such as meshes, animations, materials, scenes, configurations and other. For services, they can be used as a data transmitting containers to convert between XML, JSON, JSONB, MongoDB documents, PostgreSQL results and others.

## Documentation
You may take a look at Wiki pages. There are some practical usage examples that can be drag-and-drop executed.

## Cross platform (tested platforms)
+ Windows 7/8/8.1/10+ x64/x86 (MSVC, MSVC ClangCL, MinGW)
+ Raspberian 3+ ARM (GCC)
+ Solaris 9+ x64/x86 (GCC)
+ FreeBSD 11+ x64/x86 (GCC)
+ Debian 11+ x64/x86 (GCC, LLVM)
+ Ubuntu 16.04+ x64/x86 (GCC)
+ MacOS Catalina 10.15+ x64 (Xcode)

## Building (standalone)
Mavi uses CMake as building system. Microsoft [vcpkg](https://github.com/Microsoft/vcpkg) is suggested.
1. Install [CMake](https://cmake.org/install/).
2. Install desired dynamic dependencies listed below to have all the functionality (if you are not using vcpkg).
3. Execute CMake command to generate the files or use CMake GUI if you have one.
If you want to use vcpkg then add VCPKG_ROOT environment variable and execute CMake generate, it will install dependencies automatically using JSON manifest file. Another option is to set CMAKE_TOOLCHAIN_FILE option (standard workflow for vcpkg), dependency installation will also be automatic. For example,
> cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake ...

## Building (subproject)
1. *Optional* If you use vcpkg then add toolchain file (vcpkg doesn't know how to install dependencies of subproject, you will have to specify needed dependencies in vcpkg.json near your CMakeLists.txt):
```cmake
include(path/to/mavi/lib/toolchain.cmake)
#...
project(app_name)
```
2. *Optional* If you don't specify any compile flags then you may use Mavi's compiler flags:
```cmake
include(path/to/mavi/lib/compiler.cmake)
```
3. Add subproject. This will fully link your application with Mavi.
```cmake
add_subdirectory(/path/to/mavi mavi)
link_directories(/path/to/mavi)
target_include_directories(app_name PRIVATE /path/to/mavi)
target_link_libraries(app_name PRIVATE mavi)
```
4. Execute CMake command to generate the files or use CMake GUI if you have one.
You can look at [Lynx's CMakeLists.txt](https://github.com/romanpunia/lynx/blob/master/CMakeLists.txt) to find out how it should be used in practice

## Building options
Core options
+ **VI_CXX** is the C++ standard (14, 17, 20, 23) which will be used, C++17 and higher will add parallel sort and other optimizations, C++20 and higher will replace stateful coroutines with coroutines TS, defaults to 20
+ **VI_LOGGING** is a logging level (errors, warnings, default, debug, verbose), defaults to "default"

Dependency options
+ **VI_ASSIMP** will enable Assimp library if any, defaults to true
+ **VI_FREETYPE** will enable FreeType library if any, defaults to true
+ **VI_GLEW** will enable GLEW library if any, defaults to true
+ **VI_MONGOC** will enable MongoDB library if any, defaults to true
+ **VI_POSTGRESQL** will enable PostgreSQL library if any, defaults to true
+ **VI_OPENAL** will enable OpenAL library if any, defaults to true
+ **VI_OPENGL** will enable OpenGL library if any, defaults to true
+ **VI_OPENSSL** will enable OpenSSL library if any, defaults to true
+ **VI_SDL2** will enable SDL2 library if any, defaults to true
+ **VI_ZLIB** will enable zlib library if any, defaults to true
+ **VI_SPIRV** will enable SPIRV Cross and Glslang libraries if any, defaults to true
+ **VI_SHADERS** to embed shaders from **/src/shaders** to **/src/mavi/graphics/dynamic/shaders.hpp**, defaults to true
+ **VI_SIMD** will enable simd optimisations (processor-specific), defaults to false
+ **VI_JIT** will enable optional JIT compiler for AngelScript, defaults to false
+ **VI_FCTX** will enable internal fcontext implementation for coroutines, defaults to true
+ **VI_BULLET3** will enable built-in Bullet3 library and physics interfaces, defaults to true
+ **VI_RMLUI** will enable built-in RmlUi library and gui interfaces, defaults to true
+ **VI_BINDINGS** will enable full script bindings otherwise only essentials will be used to reduce lib size, defaults to true
+ **VI_ALLOCATOR** will enable custom allocator for all used standard containers, making them incompatible with std::allocator based ones but adding opportunity to use pool allocator, defaults to true
+ **VI_BACKTRACE** will enable stacktrace that will be produced by backward-cpp, on some lightweight linux platforms it might be unavailable, defaults to true

## Dependencies
Shared objects (so) are optional.
* [Bullet3 (embedded, optional)](https://github.com/bulletphysics/bullet3)
* [AngelScript (embedded)](https://sourceforge.net/projects/angelscript/)
* [wepoll (embedded)](https://github.com/piscisaureus/wepoll)
* [RmlUi (embedded, optional)](https://github.com/mikke89/RmlUi)
* [tinyfiledialogs (embedded)](https://github.com/native-toolkit/tinyfiledialogs)
* [RapidXML (embedded)](https://github.com/discordapp/rapidxml)
* [RapidJSON (embedded)](https://github.com/tencent/rapidjson)
* [stb (embedded)](https://github.com/nothings/stb)
* [vectorclass (embedded, optional)](https://github.com/vectorclass/version1)
* [concurrentqueue (embedded)](https://github.com/cameron314/concurrentqueue)
* [backward-cpp (embedded, optional)](https://github.com/bombela/backward-cpp)
* [D3D11 (so)](https://www.microsoft.com/en-us/download/details.aspx?id=6812)
* [OpenGL (so)](https://github.com/KhronosGroup/OpenGL-Registry)
* [OpenSSL (so)](https://github.com/openssl/openssl)
* [OpenAL-Soft (so)](https://github.com/kcat/openal-soft)
* [GLEW (so)](https://github.com/nigels-com/glew)
* [zlib (so)](https://github.com/madler/zlib)
* [Assimp (so)](https://github.com/assimp/assimp)
* [mongo-c-driver (so)](https://github.com/mongodb/mongo-c-driver)
* [libpq (so)](https://github.com/postgres/postgres/tree/master/src/interfaces/libpq)
* [freetype (so)](https://github.com/freetype/freetype)
* [SDL2 (so)](https://www.libsdl.org/download-2.0.php)
* [spirv-cross (so)](https://github.com/KhronosGroup/SPIRV-Cross)
* [glslang (so)](https://github.com/KhronosGroup/glslang)

## License
Mavi is licensed under the MIT license

## Known Issues
1. Script bindings cover only these modules for now: Core, Compute, Audio, Network, Network HTTP, Network SMTP, Graphics, Engine, Engine GUI, Engine components, Engine renderers.<br />
_TODO: Audio effects, Audio filters, Network MongoDB, Network PostgreSQL._

2. Functions that could fail will be using expects<> or option<> wrappers in the nearest future but at the moment they are not. 

3. Documentation in it's usual form is non-existant at the moment. In the nearest future that could be changed.