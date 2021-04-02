cmake_minimum_required(VERSION 3.6)
cmake_policy(SET CMP0054 NEW)

#Set standard toolchain file if any
if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

#Project's metadata
project(tomahawk)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

#Build directory if not defined
if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
endif()

#RmlUi's FreeType resolver
set(TH_USE_FREETYPE ON CACHE BOOL "Enable FreeType library")

#Project's inputs
include(lib/targets.cmake)

#Project's outputs
add_library(tomahawk SHARED ${SOURCE})
set_target_properties(tomahawk PROPERTIES
        OUTPUT_NAME "tomahawk"
        CXX_STANDARD_REQUIRED 14
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION})
target_compile_definitions(tomahawk PRIVATE
		-D_GNU_SOURCE
		-DTH_EXPORT
		-DNOMINMAX)

#Project's logging
set(TH_INFO true CACHE BOOL "Allow to log info messages")
set(TH_WARN true CACHE BOOL "Allow to log warning messages")
set(TH_ERROR true CACHE BOOL "Allow to log error messages")
if (TH_INFO AND TH_WARN AND TH_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=3)
elseif (TH_WARN AND TH_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=2)
elseif (TH_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=1)
else()
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=0)
endif()

#Project's options
include(lib/options.cmake)

#Project's dependency resolvers
include(lib/openssl/CMakeLists.txt)
include(lib/openal/CMakeLists.txt)
include(lib/opengl/CMakeLists.txt)
include(lib/d3d11/CMakeLists.txt)
include(lib/glew/CMakeLists.txt)
include(lib/zlib/CMakeLists.txt)
include(lib/sdl2/CMakeLists.txt)
include(lib/freetype/CMakeLists.txt)
include(lib/assimp/CMakeLists.txt)
include(lib/mongoc/CMakeLists.txt)
include(lib/pq/CMakeLists.txt)

#Project's source resolvers
include(src/supplies/angelscript/CMakeLists.txt)
include(src/supplies/bullet3/CMakeLists.txt)
include(src/supplies/rmlui/CMakeLists.txt)
include(src/supplies/tinyfiledialogs/CMakeLists.txt)
include(src/supplies/rapidxml/CMakeLists.txt)
include(src/supplies/rapidjson/CMakeLists.txt)
include(src/supplies/stb/CMakeLists.txt)
include(src/supplies/wepoll/CMakeLists.txt)
include(src/supplies/vcl/CMakeLists.txt)
include(src/tomahawk/CMakeLists.txt)