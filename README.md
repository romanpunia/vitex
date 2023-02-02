## Tomahawk
<p align="center">
  <img width="640" height="336" src="https://github.com/romanpunia/tomahawk/blob/master/var/screenshot.png?raw=true">
</p>

## About
Tomahawk is a cross-platform C++14 framework to create any type of application from a unified interface. In it's core, Tomahawk is based on same concepts as Node.js but made to it's extreme. As in Node, Tomahawk has a worker pool that not only consumes but also publishes tasks, here we don't really have a concept of event loop, every thread is on it's own an event loop.

Using concept of tasks and queues, in Tomahawk there are two rules for optimal performace and proper CPU loading:
1. Split work in to small pieces.
2. Use scheduler to process those small pieces.

There are two types of thread workers in a thread pool: light and heavy. Light threads processes non-blocking tasks such as event dispatching, non-blocking IO and coroutines. Heavy threads are in charge of everything else such as CPU bound tasks and blocking IO. Tomahawk is made to be as scalable as possible without even thinking about scalability which in turn makes development of multithreaded systems quite a lot easier.

Originally, Tomahawk was only a game engine but now it isn't. All the features for game development are there but it can be easily stripped out to reduce size of executable and use only needed functionality. There are cases when Tomahawk is used as a framework for building a high performance backend server or a daemon, most of the time all we need in that case is OpenSSL and maybe Zlib with PostgreSQL or MongoDB, so we strip out everything else that way reducing compile time, executable size and runtime memory usage to minimum. Turns out, Tomahawk can easily run in a very limited machines that were slow even in 2006.

If your goal is to make a game, Tomahawk is not really a consumer ready product to build AAA games as it is very low level. I tried to make it so that if you need some functionality such as a new entity component, advanced rendering techinque such as Ray-Tracing, a new rendering backend or even a motion capture system to create cutscenes, you can make it in a relatively short time without hurting performance. Why isn't it already contained in this engine? I'm the only developer, physically impossible.

For games, Tomahawk is a 3D optimized engine, there is a posibility to render efficient 2D graphics with batching but that is not a priority. Main shading language is HLSL, it is transpiled or directly compiled and saved to cache when needed. Rendering is based on stacking, that way you specify renderers for a camera (order matters), when rendering is initiated we process each renderer step by step, renderer can initiate sub-pass to render another part of scene for shadows, reflections, transparency and others. Before rendering begins, render lists are prepared for geometry rendering based on culling results, for sub-passes we use frustum and indexed culling, for main passes we also use occlusion culling. Culling, physics, audio and rendering are done multhreaded if possible. In best case, we don't have any synchronization between threads but when we need to write to shared data,
for example, destroy a rigid body owned by physics engine, we use scene transactions that are just callbacks that are guaranteed to be executed thread safe (in scope of scene) which in turn makes scene eventual consistent as transactions are fired later when all parallel tasks are finished.

Another important aspect of Tomahawk is schemas, they are used to serialize and deserialize data. For game, their main purpose is to provide containers for serialized game states such as meshes, animations, materials, scenes, configurations and other. For services, they can be used as a data transmitting containers to convert between XML, JSON, JSONB, MongoDB documents, PostgreSQL results and others.

As for memory management, there are no smart pointers, instead raw pointers with reference counting are used. **Unique\<T\>** is just an alias for **T\*** pointer, nothing more. Reference counting is used mostly for script engine virtual machine not for actual C++ code, you can use it within C++ if you want, every heap allocated object is based on **Tomahawk::Core::Object** and every heap allocation goes through **Tomahawk::Core::Mem** malloc, realloc and free.

There are two basic rules of memory ownership:
1. If function accept a pointer argument like **char\*** then it means that it will not delete it, it may only read or modify the contents. If function returns a pointer like **char\*** then this pointer should not be deleted manually; in some cases manual deletion is considered OK (singletons, tree structures like Schema).
2. If function accept a pointer alias argument like **Tomahawk::Core::Unique\<char\>** then it will delete it now or later (means ownership is lost). If function returns a pointer like **Tomahawk::Core::Unique\<char\>** then this pointer should be deleted manually.

![CMake](https://github.com/romanpunia/tomahawk/workflows/CMake/badge.svg)

## Features
#### Core
+ Thread-safe event queue (with adjustable workers count)
+ File system
+ Process management
+ Timers
+ Memory management (custom malloc/realloc/free)
+ OS functionality
+ String utils
+ Variants
+ Key-value storage documents
+ XML/JSON/JSONB serialization/deserialization
+ Adjustable logging system
+ Dynamic libraries importer
+ Ref. counting (opt. with new/delete) for ownership management
+ Coroutines via native fibers (WinAPI/ucontext_t/fcontext_t)
+ Async/await promise-like object to handle chains of async data
+ Coasync/Coawait primitives to handle async functions like in JS
+ BigNumber for accuracy sensitive operations of any precision
+ Category-based tasking for event queue to process blocking and non-blocking tasks
+ Synchronization primitives: spin lock (Spin), readers-writer lock (Guard)
#### Math
+ Vertices
+ Vectors
+ Matrices
+ Quaternions
+ Joints (bones)
+ Animation keys
+ Rigid body physics
+ Soft body physics
+ Constraint physics
+ Physics simulator
+ Regular expressions (custom, lightweight)
+ Cryptography and hashes (MD5, SHA1, AES256 and 166 more)
+ Encoding (HYBI10, Base64, Base64URL, URI)
+ Templated math utils
+ Common utils for computations
+ Strong random functions
+ Collision detection
+ Mesh evaluation
+ File preprocessor (include, pragma, define, ifdef/ifndef/else/endif)
+ Transform hierarchy system
+ SIMD optimisations included
+ Cosmos to index arbitrary amount of AABBs (balanced binary-tree index, mostly constant AABB to \<privitive\> intersection queries)
#### Audio
+ Configurable audio playback
+ Positional sound with optional velocity
+ Multiple playback
+ Stream filtering
+ Lowpass filter
+ Bandpass filter
+ Highpass filter
+ Reverb effect
+ Chorus effect
+ Distortion effect
+ Echo effect
+ Flanger effect
+ Frequency-shifter effect
+ Vocal morpher effect
+ Pitch-shifter effect
+ Ring modulator effect
+ Autowah effect
+ Equalizer effect
+ Compressor
#### Network
+ Socket abstraction
+ SSL/TLS support
+ Socket server and connection abstraction
+ Configurable server router
+ IO multiplexer
+ Socket client abstraction
+ BSON type support for data transfer
+ MongoDB support for database manipulations (async emulation)
+ PostgreSQL support for database manipulations (fully async)
+ File transfer compression
+ Connection session support
+ Server router
+ Async/sync sockets with IO queue
+ Async/sync HTTP 1.1 server/client support
+ Async/sync WebSocket server/client support (RFC 6455, 13)
+ Async/sync SMTP client support
+ Polling mechanisms: select, poll, epoll, kqueue, iocp
+ Connection pooling for server with backlog
#### Scripting
+ Angel Script packed into library
+ Template abstraction over virtual machine
+ Support for real threads
+ Compiler with preprocessor from Compute module
+ Async functions are managed outside script context (looks and feels fully synchronous)
+ Module system to add bindings as include files
+ Symbolic imports to import functions from C/C++ libraries directly to script
+ Pointer wrapper to work directly with raw pointers
+ Strings with pointer conversion support to work with C char arrays
+ Debugger interface
+ Standard library + bindings (WIP)
#### Graphics
+ Window (activity) system
+ Input detection (keyboard, cursor, controller, joystick, multi-touch)
+ Render backend abstraction over DirectX 11 and OpenGL 4.5
+ HLSL is a primary shading language
+ Shader cross complier (HLSL to GLSL, MSL or SPV)
+ Shader registry cache (compiles/transpiles only once)
+ Shader materials
+ Structured shading system
+ Standard library for shaders
+ Default and skinned meshes
+ Element instancing for big particle systems
+ Shader preprocessor from Compute module
+ Immediate renderer (OpenGL legacy like, to draw debug tools)
#### GUI
+ Fully dynamic GUI system based on web-stack
+ Styling is done using CSS 3.0 specifications (partial)
+ Layout is done using HTML 5.0 specifications (partial)
+ Scripting is done using angel script (including async)
+ API tries to mimic web browser APIs as much as possible
+ Free type fonts are needed for text rendering (no default)
+ Simple MVC system to conditionally render HTML
+ Added custom HTML elements for GUI specifics
+ Added custom decorators such as box-shadow, background-blur and others
+ Resources may as well be loaded from external web servers
#### Engine
+ Async/sync content management with processors (may load from web server)
+ Entity component renderer system (where entity = set of components, component = data and behaviour, renderer = visualisation of data if possible)
+ Scene management events/mutations and transactions (always thread safe)
+ Scene data is processed in parallel using scheduler
+ Scene is designed to hold and process as much entities as possible by RAM without losing performance
+ Stack-based rendering system with frustum/indexed/occlusion culling and instanced batching
+ Mostly constant time search for drawable components in some area
+ Render system to handle any type of visualisation per camera
+ Application class to simplify usage of engine features (optional to use)
+ Built-in processors (file loader/saver)
+ Built-in components (entity behaviours)
+ Built-in renderers (behaviour visualisations)
+ Built-in shaders (for renderers)
#### Built-in renderers
+ Skinned and soft-body models
+ Static models with instanced batching
+ Particle systems
+ Deferred decals
+ PBR lighting with shadows
+ PBR surfaces (aka env. mapping, incremental recursive reflections included)
+ PBR global illumination volumes (radiance, reflections and ambient occlusion)
+ Bloom for emissive materials
+ Motion blur
+ Screen-space global illumination
+ Screen-space reflections
+ Screen-space ambient occlusion
+ Depth of field
+ Tone mapping
+ Glitch effect
+ UI rendering
#### Built-in components
+ Rigid body
+ Acceleration (force application for rigid bodies)
+ Slider constraint (constraints for rigid bodies)
+ Audio source
+ Audio listener
+ Skin animator (for skinned models)
+ Key animator (for any entity)
+ Element system (particle buffer)
+ Element animator (for particle systems)
+ Free-look (rotation with cursor, for cameras originally)
+ Fly (direction-oriented movement with input keys, for cameras originally)
+ Model
+ Skinned model
+ Point light (lamp)
+ Spot light (flashlight)
+ Line light (sun)
+ Probe light (can cast reflections to entities)
+ Camera (with rendering system that holds renderers)
#### Built-in processors
+ Scene processor
+ Material processor
+ Audio processor (WAVE, OGG)
+ Texture processor (JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC)
+ Shader processor (render backend dependent, code preprocessor included)
+ Model processor (with Assimp's import options, if supported)
+ Skinned model processor (with Assimp's import options, if supported)
+ Schema processor (XML, JSON, JSONB)
+ Server processor (for HTTP server to load router config)

## Cross platform
+ Windows 7/8/8.1/10+ x64/x86
+ Raspberian 3+ ARM
+ Solaris 9+ x64/x86
+ FreeBSD 11+ x64/x86
+ Ubuntu 16.04+ x64/x86
+ MacOS Catalina 10.15+ x64

## Building (standalone)
Tomahawk uses CMake as building system. Microsoft [vcpkg](https://github.com/Microsoft/vcpkg) is suggested.
1. Install [CMake](https://cmake.org/install/).
2. Install dependencies listed below to have all the functionality. If you use vcpkg, execute
> vcpkg install zlib assimp glew mongo-c-driver openal-soft openssl libpq sdl2 freetype spirv-cross glslang --triplet=$triplet

where $triplet is a target platform, for example, x86-windows or x64-windows.

3. Execute CMake command to generate the files or use CMake GUI if you have one.
If you want to use vcpkg then add VCPKG_ROOT environment variable and if you want to execute install script, add vcpkg executable to PATH environment variable. It should contain full path to vcpkg executable. Another option is to set CMAKE_TOOLCHAIN_FILE option (standard workflow for vcpkg). For example,
> cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake ...

## Building (subproject)
1-2. Same steps.

3. Add toolchain before first
```cmake
project(app_name)
```
This will apply vcpkg toolchain file if it can be located and none CMAKE_TOOLCHAIN_FILE was set before. This step is not required if you don't use vcpkg at all.
```cmake
include(path/to/th/lib/toolchain.cmake)
```
4. Add subproject. This will link your application with Tomahawk.
```cmake
add_subdirectory(/path/to/th tomahawk)
link_directories(/path/to/th)
target_include_directories(app_name PRIVATE /path/to/th)
target_link_libraries(app_name PRIVATE tomahawk)
```
5. Execute CMake command to generate the files or use CMake GUI if you have one.
You can look at [Lynx's CMakeLists.txt](https://github.com/romanpunia/lynx/blob/master/CMakeLists.txt) to find out how it should be used in practice

There are several build options for this project.
+ **TH_MSG_DEBUG** to allow verbose logs, defaults to false
+ **TH_MSG_INFO** to allow informational logs, defaults to true
+ **TH_MSG_WARN** to allow warning logs, defaults to true
+ **TH_MSG_ERROR** to allow error logs, defaults to true

These **will not** alter any interfaces but all related functions will return **nullptr** or it's alternative
+ **TH_USE_ASSIMP** will enable Assimp library if any, defaults to true
+ **TH_USE_FREETYPE** will enable FreeType library if any, defaults to true
+ **TH_USE_GLEW** will enable GLEW library if any, defaults to true
+ **TH_USE_MONGOC** will enable MongoDB library if any, defaults to true
+ **TH_USE_POSTGRESQL** will enable PostgreSQL library if any, defaults to true
+ **TH_USE_OPENAL** will enable OpenAL library if any, defaults to true
+ **TH_USE_OPENGL** will enable OpenGL library if any, defaults to true
+ **TH_USE_OPENSSL** will enable OpenSSL library if any, defaults to true
+ **TH_USE_SDL2** will enable SDL2 library if any, defaults to true
+ **TH_USE_ZLIB** will enable zlib library if any, defaults to true
+ **TH_USE_SPIRV** will enable SPIRV Cross and Glslang libraries if any, defaults to true
+ **TH_WITH_SHADERS** to embed shaders from **/src/shaders** to this project, defaults to true
+ **TH_WITH_SIMD** will enable simd optimisations (processor-specific), defaults to false
+ **TH_WITH_FCTX** will enable internal fcontext implementation for coroutines, defaults to true
+ **TH_WITH_BULLET3** will enable built-in Bullet3 library and physics interfaces, defaults to true
+ **TH_WITH_RMLUI** will enable built-in RmlUi library and gui interfaces, defaults to true

## Static dependencies
* [bullet3](https://github.com/bulletphysics/bullet3)
* [angelscript](https://sourceforge.net/projects/angelscript/)
* [wepoll](https://github.com/piscisaureus/wepoll)
* [rmlui](https://github.com/mikke89/RmlUi)
* [tinyfiledialogs](https://github.com/native-toolkit/tinyfiledialogs)
* [rapidxml](https://github.com/discordapp/rapidxml)
* [rapidjson](https://github.com/tencent/rapidjson)
* [stb](https://github.com/nothings/stb)
* [vectorclass](https://github.com/vectorclass/version1)
* [concurrentqueue](https://github.com/cameron314/concurrentqueue)
* [backward-cpp](https://github.com/bombela/backward-cpp)

## Dynamic dependencies
These are recommended to be installed, but are not required to.
* [d3d11](https://www.microsoft.com/en-us/download/details.aspx?id=6812)
* [opengl](https://github.com/KhronosGroup/OpenGL-Registry)
* [openssl](https://github.com/openssl/openssl)
* [openal-soft](https://github.com/kcat/openal-soft)
* [glew](https://github.com/nigels-com/glew)
* [zlib](https://github.com/madler/zlib)
* [assimp](https://github.com/assimp/assimp)
* [mongo-c-driver](https://github.com/mongodb/mongo-c-driver)
* [libpq](https://github.com/postgres/postgres/tree/master/src/interfaces/libpq)
* [freetype](https://github.com/freetype/freetype)
* [sdl2](https://www.libsdl.org/download-2.0.php)
* [spirv-cross](https://github.com/KhronosGroup/SPIRV-Cross)
* [glslang](https://github.com/KhronosGroup/glslang)

## License
Tomahawk is licensed under the MIT license

## Known Issues
This project is under development, bugs aren't the rare thing. Script interface covers limited part of Tomahawk.