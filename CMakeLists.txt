cmake_minimum_required(VERSION 3.6)
if (POLICY CMP0077)
	cmake_policy(SET CMP0077 NEW)
else()
	cmake_policy(SET CMP0054 NEW)
endif()

#Setup toolchain file if needed
include(lib/toolchain.cmake)

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

#Project's inputs and source targets
include(lib/targets.cmake)
include(src/supplies/angelscript/CMakeLists.txt)
include(src/supplies/bullet3/CMakeLists.txt)
include(src/supplies/rmlui/CMakeLists.txt)
include(src/supplies/tinyfiledialogs/CMakeLists.txt)
include(src/supplies/rapidxml/CMakeLists.txt)
include(src/supplies/rapidjson/CMakeLists.txt)
include(src/supplies/stb/CMakeLists.txt)
include(src/supplies/wepoll/CMakeLists.txt)
include(src/supplies/vcl/CMakeLists.txt)
include(src/supplies/cqueue/CMakeLists.txt)

#Group sources from previous stage
foreach(ITEM IN ITEMS ${SOURCE})
    get_filename_component(ITEM_PATH "${ITEM}" PATH)
    string(REPLACE "${PROJECT_SOURCE_DIR}" "" ITEM_GROUP "${ITEM_PATH}")
    string(REPLACE "/" "\\" ITEM_GROUP "${ITEM_GROUP}")
    source_group("${ITEM_GROUP}" FILES "${ITEM}")
endforeach()

#Project's outputs
add_library(tomahawk SHARED ${SOURCE})
set_target_properties(tomahawk PROPERTIES
        OUTPUT_NAME "tomahawk"
		CXX_STANDARD 14
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION})

#Project's logging
set(TH_MSG_TRACE false CACHE BOOL "Enable tracing messages")
set(TH_MSG_INFO true CACHE BOOL "Enable info messages")
set(TH_MSG_WARN true CACHE BOOL "Enable warning messages")
set(TH_MSG_ERROR true CACHE BOOL "Enable error messages")
if (TH_MSG_TRACE AND TH_MSG_INFO AND TH_MSG_WARN AND TH_MSG_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=4)
elseif (TH_MSG_INFO AND TH_MSG_WARN AND TH_MSG_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=3)
elseif (TH_MSG_WARN AND TH_MSG_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=2)
elseif (TH_MSG_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=1)
else()
    target_compile_definitions(tomahawk PUBLIC -DTH_DLEVEL=0)
endif()

#Project's compile options and directories
include(lib/options.cmake)
include(src/tomahawk/CMakeLists.txt)

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
include(lib/reshade/CMakeLists.txt)