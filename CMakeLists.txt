cmake_minimum_required(VERSION 3.6)

#Setup toolchain file if needed
include(deps/toolchain.cmake)

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

#Project's source code lists generation
include(deps/sources.cmake)

#Project's library target and properties
add_library(mavi SHARED ${SOURCE})
set_target_properties(mavi PROPERTIES
		CXX_STANDARD ${VI_CXX}
        CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION})

#Project's basic settings
set(VI_BINDINGS ON CACHE BOOL "Enable full script bindings")
set(VI_ALLOCATOR ON CACHE BOOL "Enable custom allocator for standard containers")
set(VI_LOGGING "default" CACHE STRING "Logging level (errors, warnings, default, debug, verbose)")
if (${VI_LOGGING} STREQUAL "verbose")
    message(STATUS "Verbose logging is enabled")
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=5)
elseif (${VI_LOGGING} STREQUAL "debug")
    message(STATUS "Debug logging is enabled")
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=4)
elseif (${VI_LOGGING} STREQUAL "default")
    message(STATUS "Default logging is enabled")
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=3)
elseif (${VI_LOGGING} STREQUAL "warnings")
    message(STATUS "Warnings logging is enabled")
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=2)
elseif (${VI_LOGGING} STREQUAL "errors")
    message(STATUS "Errors logging is enabled")
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=1)
else()
    target_compile_definitions(mavi PUBLIC -DVI_DLEVEL=0)
endif()
if (VI_ALLOCATOR)
    target_compile_definitions(mavi PUBLIC -DVI_ALLOCATOR)
    message(STATUS "Aggresive allocator strategy is enabled")
endif()
if (VI_BINDINGS)
    target_compile_definitions(mavi PUBLIC -DVI_BINDINGS)
    message(STATUS "Full script bindings are enabled")
endif()

#Project's options (compile flags/definitions, source directories, shared libs)
include(deps/internals.cmake)
include(deps/externals.cmake)
include(deps/compiler.cmake)
include(deps/install.cmake)