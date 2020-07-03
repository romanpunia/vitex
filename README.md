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
#### Built-in file processor
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
+ **THAWK_RESOURCE** to embed resources from **/resource** to this project, defaults to true 

## Linking
Tomahawk has support for CMake's install command, to link it with your project you can use CMake as usual.

## Resources
Tomahawk has embedded resources. They are located at **/resource**. Resources will be packed to **/tomahawk/resource.h** at CMake's configuration stage. If you want to disable resource embedding then shaders must not use standard library otherwise error will be raised.

## Core built-in dependencies from **/refs**
*These are used widely and presents useful features*

#### [Bullet Physics](https://github.com/bulletphysics/bullet3)
##### Usage
Physics simulation support mostly for games, it needs only "BulletCollision", "BulletDynamics", "BulletInverseDynamics", "BulletSoftBody", "LinearMath" and headers from directory where all of this lies.
##### Built-in because
It is used in core parts of the engine, mostly in scene graph and in components.

#### [AngelScript](https://sourceforge.net/projects/angelscript/)
##### Usage
Angel Script support for scripting module.
##### Built-in because
Script module built on top of it, so it can't be compiled without the library.

#### [Wepoll](https://github.com/piscisaureus/wepoll)
##### Usage
Socket polling mechanism that emulates posix's epoll with windows' iocp used by networking module.
##### Built-in because
Windows offers too complicated polling system that is hard to combine with UNIX.

#### [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear)
##### Usage
User interface calculation for proper rendering by backend.
##### Built-in because
It is used in core parts of the engine, mostly in application callbacks.

#### [RapidXML](https://github.com/discordapp/rapidxml)
##### Usage
Fast parser of xml documents used by many internal systems to discover needed data and mostly used to read raw xml data.
#### Built-in because
It cannot be built as library and is used widely in content managment.

#### [STB](https://github.com/nothings/stb)
##### Usage
Media processing library mostly used to load images and process them.
##### Built-in because
It is used in core parts of the engine, mostly in content managment.

## Optional dependencies from **/refs**
*These are recommended to be installed, but are not required to.*

#### [OpenSSL](https://github.com/openssl/openssl)
##### Usage
Adds an ability to accept/connect/read/write/close socket data using selected security protocol under the hood and it is also used by other libraries.
##### Install
vcpkg:
> vcpkg install openssl:%TRIPLET%

apt:
> sudo apt-get install libssl-dev

brew:
> brew install openssl@1.1 && ln -s /usr/local/opt/openssl@1.1/include/openssl /usr/local/include && ln -s /usr/local/opt/openssl@1.1/lib/*.dylib /usr/local/lib && ln -s /usr/local/opt/openssl@1.1/lib/*.a /usr/local/lib

#### [GLEW](https://github.com/nigels-com/glew)
##### Usage
Extention loading system for OpenGL rendering backend.
##### Install
vcpkg:
> vcpkg install glew:%TRIPLET%

apt:
> sudo apt-get install libglew-dev

brew:
> brew install glew && brew link glew --force

#### [Zlib](https://github.com/madler/zlib)
##### Usage
Makes possible to compress/decompress any type of data by using several compressing algorithms and it is also used by other libraries.
##### Install
vcpkg:
> vcpkg install zlib:%TRIPLET%

apt:
> sudo apt-get install zlib1g-dev

brew:
> brew install zlib && ln -s /usr/local/opt/zlib/include/* /usr/local/include && ln -s /usr/local/opt/zlib/lib/*.dylib /usr/local/lib && ln -s /usr/local/opt/zlib/lib/*.a /usr/local/lib

#### [Assimp](https://github.com/assimp/assimp)
##### Usage
Makes possible to import multiple types of assets from modelling software.
##### Install
vcpkg:
> vcpkg install assimp:%TRIPLET%

apt:
> sudo git clone https://github.com/assimp/assimp && mkdir make && cd make && sudo cmake .. && sudo make && sudo make install

brew:
> git clone https://github.com/assimp/assimp && cd assimp && mkdir make && cd make && cmake .. && make && make install

#### [MongoC](https://github.com/mongodb/mongo-c-driver)
##### Usage
MongoDB database driver for data storage, also BSON that is used for data transfer.
##### Install
vcpkg:
> vcpkg install mongo-c-driver:%TRIPLET%

apt:
> sudo git clone https://github.com/mongodb/mongo-c-driver mongodb && cd mongodb && mkdir make && cd make && sudo cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. && sudo make && sudo make install

brew:
> git clone https://github.com/mongodb/mongo-c-driver mongodb && cd mongodb && mkdir make && cd make && cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. && make && make install

#### [OpenAL Soft](https://github.com/kcat/openal-soft)
##### Usage
Creates an ability to play mono/positional sounds with effects and other stuff mostly used for games.
##### Install
vcpkg:
> vcpkg install openal-soft:%TRIPLET%

apt:
> sudo apt-get install libopenal-dev

brew:
> brew install openal-soft && ln -s /usr/local/opt/openal-soft/include/AL /usr/local/include && ln -s /usr/local/opt/openal-soft/lib/*.dylib /usr/local/lib && ln -s /usr/local/opt/openal-soft/lib/*.a /usr/local/lib

#### [SDL2](https://www.libsdl.org/download-2.0.php)
##### Usage
Cross platform window managment system with input/output controllers.
##### Install
vcpkg:
> vcpkg install sdl2:%TRIPLET%

apt:
> sudo apt-get install libsdl2-dev

brew:
> brew install sdl2 && brew link sdl2 --force
	
## Platform dependencies from **/refs**
*These are resolved automatically.*

#### [OpenGL](https://github.com/KhronosGroup/OpenGL-Registry)
##### Usage
Rendering backend used for non-Microsoft platforms.
##### Install
apt:
> sudo apt-get install libglu1-mesa-dev mesa-common-dev`

#### [D3D11](https://www.microsoft.com/en-us/download/details.aspx?id=6812)
##### Usage
Rendering backend used for Microsoft platforms.

## License
Tomahawk is licensed under the MIT license

## Known Issues
This project is under development, bugs aren't the rare thing. Also dependency installation process can by quite tricky on some platforms like MacOS.

## Doubtful moments
Coding style isn't standard, because i like C# and it's readability. It means that project uses Pascal Case almost everywhere. Be ready for that.