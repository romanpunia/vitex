## About
Tomahawk is a cross-platform C++14 framework to create any type of application from a unified interface. It provides a set of common tools, so that users can focus on making apps without having to reinvent the wheel.

## Features
#### Base
+ Thread-safe event queue
+ File system
+ Process management
+ Timers
+ Memory management
+ OS functionality
+ String utils
+ XML/JSON/Binary abstraction
+ Switchable logging system
+ Adaptable dependency system
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
+ Regular expressions (custom)
+ Cryptography (MD5, SHA1, SHA256, AES256)
+ Encoding (HYBI10, Base64, URI)
+ Templated math utils
+ Common utils for computations
+ Strong random functions
+ Collision detection
+ Mesh evaluation
+ File preprocessor (include, pragma, define, ifdef/else/endif)
+ Transform hierarchy system
#### Audio
+ Configurable audio playback
+ Positional sound with optional velocity
+ Multiple playback
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
+ Async/sync HTTP 1.1 server/client support
+ Async/sync WebSocket server support
+ Connection session support
+ Async/sync SMTP client support
+ Polling mechanisms: select, poll, epoll, kqueue, iocp
#### Scripting
+ AngelScript packed into library
+ Template abstraction over virtual machine
+ Support for real threads
+ Compiler with simple preprocessor (defines, include, pragmas)
+ Built-in switchable default interfaces
+ Promise-like async data type support
+ Simple debugger support
+ Wrapper over most of the functionality
+ JIT compiler support for Windows and Linux (non-ARM)
#### Graphics
+ Configurable windowing (activity) system
+ Input detection (keyboard, cursor, controller, joystick, multi-touch)
+ Render backend abstraction over DirectX 11 and OpenGL
+ Shader materials
+ Structured shading system
+ Standard library for shaders
+ Default and skinned meshes
+ Element instancing for big particle systems
+ Renderer without shaders
+ Simple shader preprocessor for include/pragma directives
+ Switchable render backend
#### Engine
+ Thread-safe scene graph
+ Async/sync content management with file processors
+ Entity system
+ Component system
+ Render system to handle any type of visualisation per camera
+ Data serialization
+ Built-in file processors for many types of data
+ Built-in components for different simulations
+ Built-in renderers for different visualisations
+ Built-in shader code for every renderer
+ Retain mode dynamic GUI subsystem based on Nuklear
#### Built-in renderers
+ Model renderer
+ Skinned model renderer
+ Depth renderer (shadow maps for meshes and particles)
+ Probe renderer (probes for reflections and illumination lights)
+ Light renderer (to render point, spot, line and probe lights with shadowing)
+ Element system renderer (for particles)
+ Image renderer (to render current image to the screen or elsewhere)
+ Reflection renderer (for screen-space reflections)
+ Depth of field renderer
+ Emission renderer (to render glowing materials)
+ Glitch renderer (to render different glitches on the screen)
+ Ambient occlusion renderer (simple screen-space AO)
+ Indirect occlusion renderer (simple GI)
+ Tone renderer (tone mapping)
+ GUI renderer (Nuklear-based ui renderer)
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
#### Built-in file processors
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

*Those were tested*

## Building
*Tomahawk uses CMake as building system. Because windows doesn't have default include/src folders [Microsoft's Vcpkg](https://github.com/Microsoft/vcpkg) is suggested but not required.*
1. Install [CMake](https://cmake.org/install/).
2. Install dependencies listed below to have all the functionality.
3. Execute CMake command to generate the files or use CMake GUI if you have one.
> cmake [params] [tomahawk's directory]

There are several build options for this project.
+ **THAWK_INFO** to allow informational logs, defaults to true
+ **THAWK_WARN** to allow warning logs, defaults to true
+ **THAWK_ERROR** to allow error logs, defaults to true
+ **THAWK_RESOURCE** to embed resources from **/lib/shaders** to this project, defaults to true 

## Linking
Tomahawk has support for CMake's install command, to link it with your project you can use CMake as usual.

## Resources
Tomahawk has embedded resources. They are located at **/lib/shaders**. Resources will be packed to **/src/core/shaders.h** and **/src/core/shaders.cpp** at CMake's configuration stage. If you want to disable resource embedding then shaders must not use standard library otherwise error will be raised.

## Core built-in dependencies from **/lib/internal**
These are used widely and presents useful features
* [Bullet Physics](https://github.com/bulletphysics/bullet3)
* [AngelScript](https://sourceforge.net/projects/angelscript/)
* [Wepoll](https://github.com/piscisaureus/wepoll)
* [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
* [Tiny File Dialogs](https://github.com/native-toolkit/tinyfiledialogs)
* [RapidXML](https://github.com/discordapp/rapidxml)
* [STB](https://github.com/nothings/stb)

## Optional dependencies from **/lib/external**
These are recommended to be installed, but are not required to. Every entry has **install.sh** script.
* [OpenSSL](https://github.com/openssl/openssl)
* [GLEW](https://github.com/nigels-com/glew)
* [Zlib](https://github.com/madler/zlib)
* [Assimp](https://github.com/assimp/assimp)
* [MongoC](https://github.com/mongodb/mongo-c-driver)
* [OpenAL Soft](https://github.com/kcat/openal-soft)
* [SDL2](https://www.libsdl.org/download-2.0.php)

## Platform dependencies from **/lib/external**
These are resolved automatically.
* [OpenGL](https://github.com/KhronosGroup/OpenGL-Registry)
* [D3D11](https://www.microsoft.com/en-us/download/details.aspx?id=6812)

## License
Tomahawk is licensed under the MIT license

## Known Issues
This project is under development, bugs aren't the rare thing. Also dependency installation process can by quite tricky on some platforms like MacOS.

## Doubtful moments
Coding style isn't standard, because i like C# and it's readability. It means that project uses Pascal Case almost everywhere. Be ready for that.