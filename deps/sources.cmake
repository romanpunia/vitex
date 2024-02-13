# Create sources list with main sources
file(GLOB_RECURSE SOURCE
    ${PROJECT_SOURCE_DIR}/src/vitex/*.inl
    ${PROJECT_SOURCE_DIR}/src/vitex/*.h
    ${PROJECT_SOURCE_DIR}/src/vitex/*.c
    ${PROJECT_SOURCE_DIR}/src/vitex/*.cc
    ${PROJECT_SOURCE_DIR}/src/vitex/*.hpp
    ${PROJECT_SOURCE_DIR}/src/vitex/*.cpp
    ${PROJECT_SOURCE_DIR}/src/vitex/*.hxx
    ${PROJECT_SOURCE_DIR}/src/vitex/*.cxx)

# Append shaders into the sources list
set(VI_SHADERS ON CACHE BOOL "Enable built-in shaders")
set(BUFFER_OUT "${PROJECT_SOURCE_DIR}/src/vitex/graphics/shaders/bundle")
if (VI_SHADERS)
	set(BUFFER_DIR "${PROJECT_SOURCE_DIR}/src/vitex/graphics/shaders")
    set(BUFFER_DATA "#ifndef HAS_SHADER_BUNDLE\n#define HAS_SHADER_BUNDLE\n\nnamespace shader_bundle\n{\n\tvoid foreach(void* context, void(*callback)(void*, const char*, const unsigned char*, unsigned))\n\t{\n\t\tif (!callback)\n\t\t\treturn;\n")
    file(GLOB_RECURSE SOURCE_SHADERS ${BUFFER_DIR}/*.hlsl)
    foreach(BINARY ${SOURCE_SHADERS})
        string(REPLACE "${BUFFER_DIR}" "" FILENAME ${BINARY})
        string(REPLACE "${BUFFER_DIR}/" "" FILENAME ${BINARY})
        string(REGEX REPLACE "\\.| |-" "_" VARNAME ${FILENAME})
        string(REPLACE "/" "_" VARNAME ${VARNAME})
        string(TOLOWER ${VARNAME} VARNAME)
        file(READ ${BINARY} FILEDATA LIMIT 32768)
        if (NOT FILEDATA STREQUAL "")
            string(REGEX REPLACE "\t" "" FILEDATA "${FILEDATA}")
            string(REGEX REPLACE "[\r\n][\r\n]" "\n" FILEDATA "${FILEDATA}")
            string(REGEX REPLACE "  *" " " FILEDATA "${FILEDATA}")
            string(REGEX REPLACE "[\r\n]" "\\\\n" FILEDATA "${FILEDATA}")
            string(REGEX REPLACE "\\\"" "\\\\\"" FILEDATA "${FILEDATA}")
            string(APPEND BUFFER_DATA "\n\t\tconst unsigned char ${VARNAME}[] = \"${FILEDATA}\";\n\t\tcallback(context, \"${FILENAME}\", ${VARNAME}, sizeof(${VARNAME}));\n")
        endif()
    endforeach()
    string(APPEND BUFFER_DATA "\t}\n}\n#endif")
    file(WRITE ${BUFFER_OUT}.hpp "${BUFFER_DATA}")
    list(APPEND SOURCE ${SOURCE_SHADERS})
    list(APPEND SOURCE "${BUFFER_OUT}.hpp")
    set_source_files_properties(${SOURCE_SHADERS} PROPERTIES VS_TOOL_OVERRIDE "None")
    message(STATUS "Shaders have been written to: ${BUFFER_OUT}.hpp")
else()
    file(WRITE "${BUFFER_OUT}.hpp" "")
    message(STATUS "Shaders have been erased from: ${BUFFER_OUT}.hpp")
endif()

# Append source files of dependencies
set(VI_ANGELSCRIPT ON CACHE BOOL "Enable AngelScript built-in library")
set(VI_BACKWARDCPP ON CACHE BOOL "Enable backward-cpp built-in library (requires execinfo lib)")
set(VI_RMLUI ON CACHE BOOL "Enable RmlUi built-in library")
set(VI_FREETYPE ON CACHE BOOL "Enable FreeType library")
set(VI_BULLET3 ON CACHE BOOL "Enable Bullet3 built-in library")
set(VI_TINYFILEDIALOGS ON CACHE BOOL "Enable tinyfiledialogs built-in library")
set(VI_WEPOLL ON CACHE BOOL "Enable efficient epoll implementation for Windows")
set(VI_STB ON CACHE BOOL "Enable stb built-in library")
set(VI_PUGIXML ON CACHE BOOL "Enable pugixml built-in library")
set(VI_RAPIDJSON ON CACHE BOOL "Enable rapidjson built-in library")
set(VI_VECTORCLASS OFF CACHE BOOL "Enable vectorclass built-in library (release mode perf. gain)")
if (VI_BULLET3)
	file(GLOB_RECURSE SOURCE_BULLET3
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletCollision/*.h*
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletCollision/*.cpp*
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletDynamics/*.h*
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletDynamics/*.cpp*
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletSoftBody/*.h*
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletSoftBody/*.cpp*
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/LinearMath/*.h*
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/LinearMath/*.cpp*
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/btBulletCollisionCommon.h
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/btBulletDynamicsCommon.h)
	list(APPEND SOURCE ${SOURCE_BULLET3})
	message(STATUS "Bullet3 library enabled")
endif()
if (VI_RMLUI)
    file(GLOB_RECURSE SOURCE_RMLUI_ALL
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Include/RmlUi/Core.h
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Include/RmlUi/Config/*.h*
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Include/RmlUi/Core/*.h*
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Include/RmlUi/Core/*.hpp*
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Include/RmlUi/Core/*.inl*
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Source/Core/Elements/*.h*
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Source/Core/Elements/*.cpp*
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Source/Core/Layout/*.h*
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Source/Core/Layout/*.cpp*)
    file(GLOB SOURCE_RMLUI
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Source/Core/*.h
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Source/Core/*.cpp)
	find_path(FREETYPE_LOCATION ft2build.h PATH_SUFFIXES "freetype2")
	if (FREETYPE_LOCATION)
		find_package(Freetype QUIET)
		if (NOT Freetype_FOUND)
			find_library(Freetype_FOUND "freetype")
		endif()
	endif()
	if (VI_FREETYPE AND (Freetype_FOUND OR FREETYPE_LIBRARIES))
        file(GLOB SOURCE_RMLUI_FONT_ENGINE
            ${PROJECT_SOURCE_DIR}/deps/rmlui/Source/Core/FontEngineDefault/*.h*
            ${PROJECT_SOURCE_DIR}/deps/rmlui/Source/Core/FontEngineDefault/*.cpp*)
        list(APPEND SOURCE_RMLUI ${SOURCE_RMLUI_FONT_ENGINE})
    else()
        message("RmlUi default font engine disabled")
	endif()
	list(APPEND SOURCE
        ${SOURCE_RMLUI_ALL}
        ${SOURCE_RMLUI})
	message(STATUS "RmlUi library enabled")
    unset(Freetype_FOUND CACHE)
    unset(FREETYPE_LIBRARIES CACHE)
    unset(FREETYPE_LOCATION CACHE)
	unset(FREETYPE_DIR CACHE)
	unset(FREETYPE_LOCATION CACHE)
endif()
if (VI_VECTORCLASS)
	file(GLOB_RECURSE SOURCE_SIMD ${PROJECT_SOURCE_DIR}/deps/vectorclass/*.h*)
	list(APPEND SOURCE ${SOURCE_SIMD})
	message(STATUS "Simd instructions using vectorclass enabled")
endif()
if (VI_BACKWARDCPP)
    list(APPEND SOURCE ${PROJECT_SOURCE_DIR}/deps/backward-cpp/backward.hpp)
    message(STATUS "Backtraces using backward-cpp enabled")
endif()
if (VI_TINYFILEDIALOGS)
    list(APPEND SOURCE 
        ${PROJECT_SOURCE_DIR}/deps/tinyfiledialogs/tinyfiledialogs.h
        ${PROJECT_SOURCE_DIR}/deps/tinyfiledialogs/tinyfiledialogs.c)
endif()
if (VI_STB)
    file(GLOB SOURCE_STB
        ${PROJECT_SOURCE_DIR}/deps/stb/*.h
        ${PROJECT_SOURCE_DIR}/deps/stb/*.c)
    list(APPEND SOURCE ${SOURCE_STB})
endif()
if (VI_PUGIXML)
    file(GLOB_RECURSE SOURCE_PUGIXML ${PROJECT_SOURCE_DIR}/deps/pugixml/src/*.*)
    list(APPEND SOURCE ${SOURCE_PUGIXML})
endif()
if (VI_RAPIDJSON)
    file(GLOB_RECURSE SOURCE_RAPIDJSON
        ${PROJECT_SOURCE_DIR}/deps/rapidjson/include/*.h*)
    list(APPEND SOURCE ${SOURCE_RAPIDJSON})
endif()
if (VI_ANGELSCRIPT)
    file(GLOB_RECURSE SOURCE_ANGELSCRIPT
        ${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/include/angelscript.h
        ${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/source/*.h
        ${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/source/*.cpp)
    if (MSVC)
        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm64")
                list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/source/as_callfunc_arm64_msvc.asm")
            else()
                list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/source/as_callfunc_x64_msvc_asm.asm")
            endif()
        elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
            list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/source/as_callfunc_arm_msvc.asm")
        endif()
    elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
        list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/source/as_callfunc_arm_gcc.S")
        list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/source/as_callfunc_arm_vita.S")
        list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/source/as_callfunc_arm_xcode.S")
    elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^aarch64")
        if (APPLE)
            list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/source/as_callfunc_arm64_xcode.S")
        else()
            list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/source/as_callfunc_arm64_gcc.S")
        endif()
    endif()
    list(APPEND SOURCE ${SOURCE_ANGELSCRIPT})
endif()
if (VI_WEPOLL AND WIN32)
	list(APPEND SOURCE
        ${PROJECT_SOURCE_DIR}/deps/wepoll/wepoll.h
        ${PROJECT_SOURCE_DIR}/deps/wepoll/wepoll.c)
endif()
if (VI_FCONTEXT OR TRUE)
    set(FCONTEXT_ARCHS arm arm64 loongarch64 mips32 mips64 ppc32 ppc64 riscv64 s390x i386 x86_64 combined)
    if (WIN32)
        set(FCONTEXT_BIN pe)
    elseif (APPLE)
        set(FCONTEXT_BIN mach-o)
    else()
        set(FCONTEXT_BIN elf)
    endif()
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "^[Aa][Rr][Mm]" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(FCONTEXT_ABI aapcs)
    elseif (WIN32)
        set(FCONTEXT_ABI ms)
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^mips")
        if (CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(FCONTEXT_ABI o32)
        else()
            set(FCONTEXT_ABI n64)
        endif()
    else()
        set(FCONTEXT_ABI sysv)
    endif()
    if (CMAKE_SYSTEM_PROCESSOR IN_LIST FCONTEXT_ARCHS)
        set(FCONTEXT_ARCH ${CMAKE_SYSTEM_PROCESSOR})
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "^[Aa][Rr][Mm]")
            set(FCONTEXT_ARCH arm)
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^mips")
            set(FCONTEXT_ARCH mips32)
        else()
            set(FCONTEXT_ARCH i386)
        endif()
    else()
        if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^[Aa][Rr][Mm]")
            set(FCONTEXT_ARCH arm64)
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^mips")
            set(FCONTEXT_ARCH mips64)
        else()
            set(FCONTEXT_ARCH x86_64)
        endif()
    endif()
    if (MSVC)
        if(FCONTEXT_ARCH STREQUAL arm64 OR FCONTEXT_ARCH STREQUAL arm)
            set(FCONTEXT_ASM armasm)
        else()
            set(FCONTEXT_ASM masm)
        endif()
    else()
        set(FCONTEXT_ASM gas)
    endif()
    if (FCONTEXT_BIN STREQUAL pe)
        set(FCONTEXT_EXT .asm)
    elseif (FCONTEXT_ASM STREQUAL gas)
        set(FCONTEXT_EXT .S)
    else()
        set(FCONTEXT_EXT .asm)
    endif()
    if (FCONTEXT_BIN STREQUAL mach-o)
        set(FCONTEXT_BIN macho)
    endif()
    if (EXISTS ${PROJECT_SOURCE_DIR}/src/vitex/internal/fcontext/make_${FCONTEXT_ARCH}_${FCONTEXT_ABI}_${FCONTEXT_BIN}_${FCONTEXT_ASM}${FCONTEXT_EXT})
        set(VI_FCONTEXT ON CACHE BOOL "Enable fcontext built-in library (otherwise coroutines based on ucontext, WinAPI or C++20)")
        if (VI_FCONTEXT)
            set(FCONTEXT_SOURCES
                ${PROJECT_SOURCE_DIR}/src/vitex/internal/fcontext/fcontext.h
                ${PROJECT_SOURCE_DIR}/src/vitex/internal/fcontext/make_${FCONTEXT_ARCH}_${FCONTEXT_ABI}_${FCONTEXT_BIN}_${FCONTEXT_ASM}${FCONTEXT_EXT}
                ${PROJECT_SOURCE_DIR}/src/vitex/internal/fcontext/jump_${FCONTEXT_ARCH}_${FCONTEXT_ABI}_${FCONTEXT_BIN}_${FCONTEXT_ASM}${FCONTEXT_EXT}
                ${PROJECT_SOURCE_DIR}/src/vitex/internal/fcontext/ontop_${FCONTEXT_ARCH}_${FCONTEXT_ABI}_${FCONTEXT_BIN}_${FCONTEXT_ASM}${FCONTEXT_EXT})
            list(APPEND SOURCE ${FCONTEXT_SOURCES})
            message(STATUS "Async fcontext implementation: ${FCONTEXT_ARCH}_${FCONTEXT_ABI}_${FCONTEXT_BIN}_${FCONTEXT_ASM}")
        else()
            message(STATUS "Async fcontext implementation: ucontext or WinAPI")
        endif()
    else()
        set(VI_FCONTEXT OFF CACHE BOOL "FContext is unavailable")
        message("Async fcontext is unavailable on ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}. Fallback to ucontext or WinAPI")
    endif()
    unset(FCONTEXT_BIN)
    unset(FCONTEXT_ABI)
    unset(FCONTEXT_ARCHS)
    unset(FCONTEXT_ARCH)
    unset(FCONTEXT_ASM)
    unset(FCONTEXT_EXT)
    unset(FCONTEXT_SUFFIX)
endif()
if (VI_CONCURRENTQUEUE OR TRUE)
    list(APPEND SOURCE
        ${PROJECT_SOURCE_DIR}/deps/concurrentqueue/concurrentqueue.h
        ${PROJECT_SOURCE_DIR}/deps/concurrentqueue/lightweightsemaphore.h
        ${PROJECT_SOURCE_DIR}/deps/concurrentqueue/blockingconcurrentqueue.h)
endif()

# Group all sources for nice IDE preview
foreach(ITEM IN ITEMS ${SOURCE})
    get_filename_component(ITEM_PATH "${ITEM}" PATH)
    string(REPLACE "${PROJECT_SOURCE_DIR}" "" ITEM_GROUP "${ITEM_PATH}")
    string(REPLACE "/" "\\" ITEM_GROUP "${ITEM_GROUP}")
    source_group("${ITEM_GROUP}" FILES "${ITEM}")
endforeach()