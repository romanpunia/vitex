cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0054 NEW)

#Project's package support for non-UNIX platforms
include(opts/package/CMakeLists.txt)

#Project's metadata
project(tomahawk)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE bin)
endif()

#Project's source resolver
include(opts/source/CMakeLists.txt)

#Project's compile options
add_library(tomahawk SHARED ${SOURCE})
set_target_properties(tomahawk PROPERTIES
        OUTPUT_NAME "tomahawk"
        CXX_STANDARD_REQUIRED 14
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION})
target_compile_definitions(tomahawk PRIVATE
		-DNOMINMAX
		-D_GNU_SOURCE)

#Project's options
include(opts/option/CMakeLists.txt)

#Project's compiler support for non-UNIX platforms
include(opts/windows/CMakeLists.txt)

#Project's dependency resolvers
include(refs/angelscript/CMakeLists.txt)
include(refs/bullet/CMakeLists.txt)
include(refs/imgui/CMakeLists.txt)
include(refs/rapidxml/CMakeLists.txt)
include(refs/stb/CMakeLists.txt)
include(refs/wepoll/CMakeLists.txt)
include(refs/openssl/CMakeLists.txt)
include(refs/openal/CMakeLists.txt)
include(refs/opengl/CMakeLists.txt)
include(refs/d3d11/CMakeLists.txt)
include(refs/glew/CMakeLists.txt)
include(refs/zlib/CMakeLists.txt)
include(refs/sdl2/CMakeLists.txt)
include(refs/assimp/CMakeLists.txt)
include(refs/mongoc/CMakeLists.txt)

#Project's target installation
include(opts/install/CMakeLists.txt)