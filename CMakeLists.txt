cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0054 NEW)

#Project's package support for non-UNIX platforms
include(opt/package/CMakeLists.txt)

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
include(opt/source/CMakeLists.txt)

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
include(opt/windows/CMakeLists.txt)

#Project's dependency resolvers
include(ref/angelscript/CMakeLists.txt)
include(ref/bullet3/CMakeLists.txt)
include(ref/nuklear/CMakeLists.txt)
include(ref/tinyfiledialogs/CMakeLists.txt)
include(ref/rapidxml/CMakeLists.txt)
include(ref/stb/CMakeLists.txt)
include(ref/wepoll/CMakeLists.txt)
include(ref/openssl/CMakeLists.txt)
include(ref/openal/CMakeLists.txt)
include(ref/opengl/CMakeLists.txt)
include(ref/d3d11/CMakeLists.txt)
include(ref/glew/CMakeLists.txt)
include(ref/zlib/CMakeLists.txt)
include(ref/sdl2/CMakeLists.txt)
include(ref/assimp/CMakeLists.txt)
include(ref/mongoc/CMakeLists.txt)

#Project's target installation
include(opt/install/CMakeLists.txt)