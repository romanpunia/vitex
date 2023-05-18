cmake_minimum_required(VERSION 3.6)

#Setup toolchain file if needed
include(lib/toolchain.cmake)

#Project's metadata
project(mavi)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

#Build directory if not defined
if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin)
endif()

#Project's embedded dependencies and source files
include(lib/sources.cmake)
include(src/supplies/fcontext/CMakeLists.txt)
include(src/supplies/angelscript/CMakeLists.txt)
include(src/supplies/bullet3/CMakeLists.txt)
include(src/supplies/rmlui/CMakeLists.txt)
include(src/supplies/tinyfiledialogs/CMakeLists.txt)
include(src/supplies/rapidxml/CMakeLists.txt)
include(src/supplies/rapidjson/CMakeLists.txt)
include(src/supplies/stb/CMakeLists.txt)
include(src/supplies/wepoll/CMakeLists.txt)
include(src/supplies/vcl/CMakeLists.txt)
include(src/supplies/backward/CMakeLists.txt)
include(src/supplies/cqueue/CMakeLists.txt)

#Group sources from previous stage
foreach(ITEM IN ITEMS ${SOURCE})
    get_filename_component(ITEM_PATH "${ITEM}" PATH)
    string(REPLACE "${PROJECT_SOURCE_DIR}" "" ITEM_GROUP "${ITEM_PATH}")
    string(REPLACE "/" "\\" ITEM_GROUP "${ITEM_GROUP}")
    source_group("${ITEM_GROUP}" FILES "${ITEM}")
endforeach()

#Project's outputs
add_library(mavi SHARED ${SOURCE})
set_target_properties(mavi PROPERTIES
		CXX_STANDARD ${VI_CXX}
        CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION})

#Project's settings
set(VI_USE_BINDINGS ON CACHE BOOL "Enable full script bindings")
set(VI_USE_FAST_MEMORY OFF CACHE BOOL "Enable custom allocator for standard containers")
set(VI_USE_JIT OFF CACHE BOOL "Enable JIT compilation for AngelScript")
set(VI_LOGGING "default" CACHE STRING "Logging level (errors, warnings, default, debug, verbose)")
message(STATUS "Logging messages: ${VI_LOGGING}")

if (${VI_LOGGING} STREQUAL "verbose")
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=5)
elseif (${VI_LOGGING} STREQUAL "debug")
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=4)
elseif (${VI_LOGGING} STREQUAL "default")
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=3)
elseif (${VI_LOGGING} STREQUAL "warnings")
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=2)
elseif (${VI_LOGGING} STREQUAL "errors")
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=1)
else()
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=0)
endif()
if (VI_USE_JIT)
    target_compile_definitions(mavi PUBLIC -DVI_HAS_JIT)
    message(STATUS "Script translation: jit compiler")
else()
    message(STATUS "Script translation: none")
endif()
if (VI_USE_FAST_MEMORY)
    target_compile_definitions(mavi PUBLIC -DVI_HAS_FAST_MEMORY)
    message(STATUS "Allocation strategy: aggresive")
else()
    message(STATUS "Allocation strategy: default")
endif()
if (VI_USE_BINDINGS)
    target_compile_definitions(mavi PUBLIC -DVI_HAS_BINDINGS)
    message(STATUS "Script bindings: full coverage")
else()
    message(STATUS "Script bindings: minimal coverage")
endif()

#Project's compile options, directories and dependency flags
include(lib/compiler.cmake)
include(lib/options.cmake)
include(lib/dependencies.cmake)

#Project's dependencies
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
include(lib/spirv/CMakeLists.txt)