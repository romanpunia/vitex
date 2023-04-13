cmake_minimum_required(VERSION 3.6)

#Setup toolchain file if needed
include(lib/toolchain.cmake)

#Project's metadata
project(edge)
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
add_library(edge SHARED ${SOURCE})
set_target_properties(edge PROPERTIES
        OUTPUT_NAME "edge"
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION})

#Project's settings
set(ED_USE_BINDINGS true CACHE BOOL "Enable full script bindings")
set(ED_USE_FAST_MEMORY false CACHE BOOL "Enable custom allocator for standard containers")
set(ED_MSG_TRACE false CACHE BOOL "Enable tracing messages")
set(ED_MSG_DEBUG false CACHE BOOL "Enable debugging messages")
set(ED_MSG_INFO true CACHE BOOL "Enable info messages")
set(ED_MSG_WARN true CACHE BOOL "Enable warning messages")
set(ED_MSG_ERROR true CACHE BOOL "Enable error messages")
if (ED_MSG_TRACE AND ED_MSG_DEBUG AND ED_MSG_INFO AND ED_MSG_WARN AND ED_MSG_ERROR)
    target_compile_definitions(edge PUBLIC -DED_DLEVEL=5)
    message(STATUS "Logging messages: tracing")
elseif (ED_MSG_DEBUG AND ED_MSG_INFO AND ED_MSG_WARN AND ED_MSG_ERROR)
    target_compile_definitions(edge PUBLIC -DED_DLEVEL=4)
    message(STATUS "Logging messages: debugging")
elseif (ED_MSG_INFO AND ED_MSG_WARN AND ED_MSG_ERROR)
    target_compile_definitions(edge PUBLIC -DED_DLEVEL=3)
    message(STATUS "Logging messages: essentials only")
elseif (ED_MSG_WARN AND ED_MSG_ERROR)
    target_compile_definitions(edge PUBLIC -DED_DLEVEL=2)
    message(STATUS "Logging messages: violations only")
elseif (ED_MSG_ERROR)
    target_compile_definitions(edge PUBLIC -DED_DLEVEL=1)
    message(STATUS "Logging messages: errors only")
else()
    target_compile_definitions(edge PUBLIC -DED_DLEVEL=0)
    message(STATUS "Logging messages: none")
endif()
if (ED_USE_FAST_MEMORY)
    target_compile_definitions(edge PUBLIC -DED_HAS_FAST_MEMORY)
    message(STATUS "Allocation strategy: aggresive")
else()
    message(STATUS "Allocation strategy: default")
endif()
if (ED_USE_BINDINGS)
    target_compile_definitions(edge PUBLIC -DED_HAS_BINDINGS)
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