## About
Tomahawk is a cross-platform C++14 framework to create any type of application from a unified interface. It provides a set of common tools, so that users can focus on making apps without having to reinvent the wheel.

![CMake](https://github.com/romanpunia/tomahawk/workflows/CMake/badge.svg)

## Features
#### Core
+ Thread-safe event queue (with adjustable workers count)
+ File system
+ Process management
+ Timers
+ Memory management
+ OS functionality
+ String utils
+ Separate any-value serializator
+ Key-value storage documents
+ XML/JSON serialization (plus custom JSONB format)
+ Switchable logging system
+ Adaptable dependency system
+ Fast spinlock mutex without CPU time wasting
+ Ref. counting (opt. with new/delete) for ownership management
+ Coroutines via native fibers
+ Async/await promise-like object to handle chains of async data
+ Coasync/Coawait primitives to handle async functions like in JS
+ BigNumber for accuracy sensitive operations of any precision
+ Modular dependencies, can be disabled if not needed
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
+ Regular expressions (custom, 4x faster than STL regex)
+ Cryptography and hashes (MD5, SHA1, AES256 and 166 more)
+ Encoding (HYBI10, Base64, Base64URL, URI)
+ Templated math utils
+ Common utils for computations
+ Strong random functions
+ Collision detection
+ Mesh evaluation
+ File preprocessor (include, pragma, define, ifdef/else/endif)
+ Transform hierarchy system
+ SIMD optimisations included
#### Audio
+ Configurable audio playback
+ Positional sound with optional velocity
+ Multiple playback
+ Stream filtering
+ Lowpass filter
+ Bandpass filter
+ Highpass filter
+ Effects system is available
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
+ Async/sync sockets with IO queue
+ SSL/TLS support
+ Socket server and connection abstraction
+ Configurable server router
+ IO multiplexer
+ Socket client abstraction
+ BSON type support for data transfer
+ MongoDB support for database manipulations
+ PostgreSQL support for database manipulations
+ File transfer compression
+ Async/sync HTTP 1.1 server/client support
+ Async/sync WebSocket server support
+ Connection session support
+ Async/sync SMTP client support
+ Polling mechanisms: select, poll, epoll, kqueue, iocp
#### Scripting
+ Angel Script packed into library
+ Template abstraction over virtual machine
+ Support for real threads
+ Compiler with preprocessor from Compute module
+ Built-in switchable default interfaces
+ Promise-like async data type support
+ JIT compiler support for non-ARM platforms
+ Module system to add bindings as include files
+ Symbolic imports to import functions from C/C++ libraries directly to script
+ Pointer wrapper to work directly with raw pointers
+ Strings with pointer conversion support to work with C char arrays
+ Debugger interface
+ Standard library
+ Tomahawk bindings (WIP)
#### Graphics
+ Configurable windowing (activity) system
+ Input detection (keyboard, cursor, controller, joystick, multi-touch)
+ Render backend abstraction over DirectX 11 and OpenGL (WIP)
+ Shader materials
+ Structured shading system
+ Standard library for shaders
+ Default and skinned meshes
+ Element instancing for big particle systems
+ Renderer without shaders
+ Shader preprocessor from Compute module
+ Switchable render backend
#### GUI
+ Serializable GUI system
+ CSS paradigm-based styling system with overriding support
+ HTML-like GUI declarations
+ Logical nodes for conditional rendering
+ Read/write dynamic properties
+ Various widgets
+ Layouting system
+ Font system
+ Dynamic trees (and recursive)
+ Based on RmlUi
#### Engine
+ Thread-safe scene graph
+ Async/sync content management with processors
+ Entity system
+ Component system
+ Render system to handle any type of visualisation per camera
+ Data serialization
+ Built-in processors for many types of data
+ Built-in components for different simulations
+ Built-in renderers for different visualisations
+ Built-in shader code for every renderer
#### Built-in renderers
+ Skinned, default and soft-body models
+ Particle systems
+ Deferred decals
+ PBR Lighting with shadows
+ PBR Surfaces (aka env. mapping, O(1) recursive reflections included)
+ PBR global illumination (radiance, reflections and ambient occlusion)
+ Bloom for emissive materials
+ Screen-space reflections
+ Screen-space ambient occlusion
+ Screen-space GI (simplified)
+ Depth of field
+ Tone mapping
+ Glitch effect
+ UI rendering
#### Built-in components
+ Rigid body
+ Acceleration (force applier for rigid bodies)
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
+ Scene graph processor
+ Audio clip processor (WAVE, OGG)
+ Texture 2d processor (JPG, PNG, TGA, BMP, PSD, GIF, HDR, PIC)
+ Shader processor (render backend dependent, code preprocessor included)
+ Model processor (with Assimp's import options, if supported)
+ Skinned model processor (with Assimp's import options, if supported)
+ Document processor (XML, JSON, binary)
+ Server processor (for HTTP server to load router config)

*Note: some functionality might be stripped without needed dependencies. Also exceptions were not used, it's more C-like with return codes.*
## Cross platform
+ Windows 7/8/8.1/10+ x64/x86
+ Raspberian 3+ ARM
+ Solaris 9+ x64/x86
+ FreeBSD 11+ x64/x86
+ Ubuntu 16.04+ x64/x86
+ MacOS Catalina 10.15+ x64

## Building (standalone)
*Tomahawk uses CMake as building system. Because windows doesn't have default include/src folders [Microsoft's vcpkg](https://github.com/Microsoft/vcpkg) is suggested but not required.*
1. Install [CMake](https://cmake.org/install/).
2. Install dependencies listed below to have all the functionality. If you use vcpkg, execute
> /lib/install.sh $triplet

where $triplet is a target platform, for example, x86-windows.

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
4. Add subproject. This will link you application with Tomahawk.
```cmake
add_subdirectory(/path/to/th tomahawk)
link_directories(/path/to/th)
target_include_directories(app_name PRIVATE /path/to/th)
target_link_libraries(app_name PRIVATE tomahawk)
```
5. Execute CMake command to generate the files or use CMake GUI if you have one.
You can look at [Lynx's CMakeLists.txt](https://github.com/romanpunia/lynx/blob/master/CMakeLists.txt) to find out how it should be used in practice

There are several build options for this project.
+ **TH_MSG_INFO** to allow informational logs, defaults to true
+ **TH_MSG_WARN** to allow warning logs, defaults to true
+ **TH_MSG_ERROR** to allow error logs, defaults to true

These **will not** alter any interfaces
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

These **will** alter some interfaces like GUI and Compute
+ **TH_WITH_SHADERS** to embed shaders from **/src/shaders** to this project, defaults to true
+ **TH_WITH_SIMD** will enable simd optimisations (processor-specific), defaults to false
+ **TH_WITH_BULLET3** will enable Bullet3 library and physics interfaces, defaults to true
+ **TH_WITH_RMLUI** will enable RmlUi library and gui interfaces, defaults to true

## Core built-in dependencies from **/src/supplies**
These are used widely and presents useful features
* [bullet3](https://github.com/bulletphysics/bullet3)
* [angelscript](https://sourceforge.net/projects/angelscript/)
* [wepoll](https://github.com/piscisaureus/wepoll)
* [rmlui](https://github.com/mikke89/RmlUi)
* [tinyfiledialogs](https://github.com/native-toolkit/tinyfiledialogs)
* [rapidxml](https://github.com/discordapp/rapidxml)
* [rapidjson](https://github.com/tencent/rapidjson)
* [stb](https://github.com/nothings/stb)
* [vectorclass](https://github.com/vectorclass/version1)

## Optional dependencies from **/lib**
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

## License
Tomahawk is licensed under the MIT license

## Known Issues
This project is under development, bugs aren't the rare thing, THX to PVS-Studio, they became less often. Also dependency installation process can by quite tricky on some platforms like MacOS. OpenGL is not working properly, sooner or later it will be fully implemented, SPIRV-Cross will help with that. Script interface covers less than 5% of Tomahawk's abilities, bindings are wasting too much time to write, sorry :)