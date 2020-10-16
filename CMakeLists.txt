cmake_minimum_required(VERSION 3.6)
cmake_policy(SET CMP0054 NEW)

#Project's package support for non-UNIX platforms
include(lib/package/CMakeLists.txt)

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

#Project's source resolver
include(lib/source/CMakeLists.txt)

#Project's compile options
add_library(tomahawk SHARED ${SOURCE})
set_target_properties(tomahawk PROPERTIES
        OUTPUT_NAME "tomahawk"
        CXX_STANDARD_REQUIRED 14
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION})
target_compile_definitions(tomahawk PRIVATE
		-DNOMINMAX
		-D_GNU_SOURCE
		-DTH_EXPORT)

#Project's options
set(TH_INFO true CACHE BOOL "allow to log info messages")
set(TH_WARN true CACHE BOOL "allow to log warning messages")
set(TH_ERROR true CACHE BOOL "allow to log error messages")

#Project's log level
if (TH_INFO AND TH_WARN AND TH_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=3)
elseif (TH_WARN AND TH_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=2)
elseif (TH_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=1)
else()
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=0)
endif()

#Project's compiler support for non-UNIX platforms
include(lib/compiler/CMakeLists.txt)

#Project's external dependency resolvers
include(lib/external/openssl/CMakeLists.txt)
include(lib/external/openal/CMakeLists.txt)
include(lib/external/opengl/CMakeLists.txt)
include(lib/external/d3d11/CMakeLists.txt)
include(lib/external/glew/CMakeLists.txt)
include(lib/external/zlib/CMakeLists.txt)
include(lib/external/sdl2/CMakeLists.txt)
include(lib/external/freetype/CMakeLists.txt)
include(lib/external/assimp/CMakeLists.txt)
include(lib/external/mongoc/CMakeLists.txt)

#Project's internal dependency resolvers
include(src/internal/angelscript/CMakeLists.txt)
include(src/internal/bullet3/CMakeLists.txt)
include(src/internal/rmlui/CMakeLists.txt)
include(src/internal/tinyfiledialogs/CMakeLists.txt)
include(src/internal/rapidxml/CMakeLists.txt)
include(src/internal/stb/CMakeLists.txt)
include(src/internal/wepoll/CMakeLists.txt)

#Project's target and installation
include(src/tomahawk/CMakeLists.txt)
include(lib/install/CMakeLists.txt)