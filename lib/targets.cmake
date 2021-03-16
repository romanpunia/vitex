#Query all sources
file(GLOB_RECURSE SOURCE
		${PROJECT_SOURCE_DIR}/src/supplies/*.inl*
		${PROJECT_SOURCE_DIR}/src/supplies/*.h*
		${PROJECT_SOURCE_DIR}/src/supplies/*.c*
		${PROJECT_SOURCE_DIR}/src/supplies/*.cc*
		${PROJECT_SOURCE_DIR}/src/supplies/*.hpp*
		${PROJECT_SOURCE_DIR}/src/supplies/*.cpp*
		${PROJECT_SOURCE_DIR}/src/supplies/*.hxx*
		${PROJECT_SOURCE_DIR}/src/supplies/*.cxx*
		${PROJECT_SOURCE_DIR}/src/tomahawk/*.inl*
		${PROJECT_SOURCE_DIR}/src/tomahawk/*.h*
		${PROJECT_SOURCE_DIR}/src/tomahawk/*.c*
		${PROJECT_SOURCE_DIR}/src/tomahawk/*.cc*
		${PROJECT_SOURCE_DIR}/src/tomahawk/*.hpp*
		${PROJECT_SOURCE_DIR}/src/tomahawk/*.cpp*
		${PROJECT_SOURCE_DIR}/src/tomahawk/*.hxx*
		${PROJECT_SOURCE_DIR}/src/tomahawk/*.cxx*)

#AngelScript specifics (ASM sources)
if (MSVC)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_x64_msvc_asm.asm")
	elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
		list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_arm_msvc.asm")
	endif()
else()
	list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_arm_gcc.S")
	list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_arm_vita.S")
	list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_arm_xcode.S")
endif()

#RmlUi specifics (FreeType sources)
if (TH_USE_FREETYPE)
	find_path(FREETYPE_LOCATION ft2build.h)
	if (FREETYPE_LOCATION)
		find_package(Freetype QUIET)
		if (NOT Freetype_FOUND)
			find_library(Freetype_FOUND "freetype")
		endif()
	endif()
	unset(FREETYPE_LOCATION CACHE)
	unset(Freetype_FOUND CACHE)
	unset(FREETYPE_LIBRARIES CACHE)
	unset(FREETYPE_DIR CACHE)
	unset(FREETYPE_LOCATION CACHE)
endif()
if (NOT Freetype_FOUND AND NOT FREETYPE_LIBRARIES)
	list(FILTER SOURCE EXCLUDE REGEX "rmlui[\\\/]Source[\\\/]Core[\\\/]FontEngineDefault")
endif()

#Shader embedding specifics (with compilation)
set(TH_SHADERS true CACHE BOOL "Allow to compile embedded shaders")
if (TH_SHADERS)
	set(BUFFER_DIR "${PROJECT_SOURCE_DIR}/src/shaders")
	set(BUFFER_OUT "${PROJECT_SOURCE_DIR}/src/tomahawk/core/shaders")
    set(BUFFER_H "#ifndef HAS_SHADER_BATCH\n#define HAS_SHADER_BATCH\n\nnamespace shader_batch\n{\n\textern void foreach(void* context, void(*callback)(void*, const char*, const unsigned char*, unsigned))\;\n}\n#endif")
    set(BUFFER_CPP "#include \"shaders.h\"\n\nnamespace shader_batch\n{\n\tvoid foreach(void* context, void(*callback)(void*, const char*, const unsigned char*, unsigned))\n\t{\n\t\tif (!callback)\n\t\t\treturn\;\n")
    file(GLOB_RECURSE BINARIES ${BUFFER_DIR}/*)
    foreach(BINARY ${BINARIES})
        string(REPLACE "${BUFFER_DIR}" "" FILENAME ${BINARY})
        string(REPLACE "${BUFFER_DIR}/" "" FILENAME ${BINARY})
        string(REGEX REPLACE "\\.| |-" "_" VARNAME ${FILENAME})
        string(REPLACE "/" "_" VARNAME ${VARNAME})
        string(TOLOWER ${VARNAME} VARNAME)
        file(READ ${BINARY} FILEDATA LIMIT 32768 HEX)	
		file(READ "${BINARY}" CONTENT HEX)
		string(LENGTH "${CONTENT}" FILESIZE)
		math(EXPR FILESIZE "${FILESIZE} / 2")
        if (NOT FILEDATA STREQUAL "")
            string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," FILEDATA ${FILEDATA})
            string(APPEND BUFFER_CPP "\n\t\tconst unsigned char ${VARNAME}[] = { ${FILEDATA} }\;\n\t\tcallback(context, \"${FILENAME}\", ${VARNAME}, ${FILESIZE})\;\n")
        endif()
    endforeach()
    string(APPEND BUFFER_CPP "\t}\n}")
    file(WRITE ${BUFFER_OUT}.h ${BUFFER_H})
    file(WRITE ${BUFFER_OUT}.cpp ${BUFFER_CPP})	
    list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/src/tomahawk/core/shaders.h")
    list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/src/tomahawk/core/shaders.cpp")
    message(STATUS "Shaders entry have been written to: ${BUFFER_OUT}.h")
    message(STATUS "Shaders have been written to: ${BUFFER_OUT}.cpp")
else()
    file(WRITE "${PROJECT_SOURCE_DIR}/src/tomahawk/core/shaders.h" "")
    file(WRITE "${PROJECT_SOURCE_DIR}/src/tomahawk/core/shaders.cpp" "")
	message(STATUS "Shaders file has been cleared")
endif()

#Group sources to their virtual directories
foreach(ITEM IN ITEMS ${SOURCE})
    get_filename_component(ITEM_PATH "${ITEM}" PATH)
    string(REPLACE "${PROJECT_SOURCE_DIR}" "" ITEM_GROUP "${ITEM_PATH}")
    string(REPLACE "/" "\\" ITEM_GROUP "${ITEM_GROUP}")
    source_group("${ITEM_GROUP}" FILES "${ITEM}")
endforeach()