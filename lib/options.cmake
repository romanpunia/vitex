#Check if compiler supports at least C++ 14
if (NOT "${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_14")
    message(FATAL_ERROR "Compiler must have at least C++ 14 standard support")
endif()

#Resolve needed options for different compilers and refs linkage
if (NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    if (NOT (WIN32 AND MINGW))
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat=0 -fexceptions -std=c++17")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-function-declaration -fexceptions")
		if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
			if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
				set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
			else()
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
				set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
			endif()
		endif()
	else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows -D_WIN32_WINNT=0x0600")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mwindows -D_WIN32_WINNT=0x0600")
    endif()
else()
    set(CMAKE_SHARED_LINKER_FLAGS "/SUBSYSTEM:WINDOWS")
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /MP /bigobj")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG /MP /bigobj")
    set(CMAKE_C_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /MP /bigobj")
    set(CMAKE_C_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG /MP /bigobj") 
    if (TH_WITH_BULLET3)
        target_compile_options(tomahawk PRIVATE /wd4305 /wd4244 /wd4018 /wd4267)
    endif()
endif()

#Link system libraries libraries
if (WIN32)
    target_link_libraries(tomahawk PUBLIC  ws2_32.lib mswsock.lib)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_link_libraries(tomahawk PUBLIC ${CMAKE_DL_LIBS} pthread)
endif()

#Select compiler for ASM sources
if (NOT MSVC)
    if (CMAKE_CXX_PLATFORM_ID MATCHES "Cygwin")
        enable_language(ASM-ATT)
    else()
        enable_language(ASM)
    endif()
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^[Aa][Rr][Mm]" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    enable_language(ASM_ARMASM)
else()
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "^i386" OR (CMAKE_SIZEOF_VOID_P EQUAL 4 AND (NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^[Aa][Rr][Mm]") AND (NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^mips")))
        set_source_files_properties(${FCTX_SOURCES} PROPERTIES COMPILE_FLAGS "/safeseh")
    endif()
    enable_language(ASM_MASM)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp")
endif()
unset(FCTX_SOURCES)

#Installation target
install(TARGETS tomahawk DESTINATION lib)
install(FILES
        src/tomahawk/audio/effects.h
        src/tomahawk/audio/filters.h
        DESTINATION include/tomahawk/audio)
install(FILES
        src/tomahawk/core/audio.h
        src/tomahawk/core/compute.h
        src/tomahawk/core/core.h
        src/tomahawk/core/engine.h
        src/tomahawk/core/graphics.h
        src/tomahawk/core/network.h
        src/tomahawk/core/script.h
        DESTINATION include/tomahawk/core)
install(FILES
        src/tomahawk/engine/components.h
        src/tomahawk/engine/gui.h
        src/tomahawk/engine/processors.h
        src/tomahawk/engine/renderers.h
        DESTINATION include/tomahawk/engine)
install(FILES
        src/tomahawk/network/http.h
        src/tomahawk/network/mdb.h
        src/tomahawk/network/pdb.h
        src/tomahawk/network/smtp.h
        DESTINATION include/tomahawk/network)
install(FILES
        src/tomahawk/script/std-lib.h
        src/tomahawk/script/core-lib.h
        src/tomahawk/script/compute-lib.h
        src/tomahawk/script/gui-lib.h
        DESTINATION include/tomahawk/script)
install(FILES
        src/tomahawk/tomahawk.h
        DESTINATION include/tomahawk)