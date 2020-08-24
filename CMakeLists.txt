cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0054 NEW)

#Project's package support for non-UNIX platforms
include(opts/package/CMakeLists.txt)

#Project's metadata
project(tomahawk)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

#Build directory if not defined
if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
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
set(THAWK_INFO true CACHE BOOL "allow to log info messages")
set(THAWK_WARN true CACHE BOOL "allow to log warning messages")
set(THAWK_ERROR true CACHE BOOL "allow to log error messages")

#Project's log level
if (THAWK_INFO AND THAWK_WARN AND THAWK_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTHAWK_DLEVEL=3)
elseif (THAWK_WARN AND THAWK_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTHAWK_DLEVEL=2)
elseif (THAWK_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTHAWK_DLEVEL=1)
else()
    target_compile_definitions(tomahawk PUBLIC -DTHAWK_DLEVEL=0)
endif()

#Project's compiler support for non-UNIX platforms
include(opts/windows/CMakeLists.txt)

#Project's dependency resolvers
include(refs/angelscript/CMakeLists.txt)
include(refs/bullet3/CMakeLists.txt)
include(refs/nuklear/CMakeLists.txt)
include(refs/tinyfiledialogs/CMakeLists.txt)
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