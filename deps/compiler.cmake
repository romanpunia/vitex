# Check if compiler supports at least C++ 14
if (NOT "${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_14")
    message(FATAL_ERROR "Compiler must have at least C++ 14 standard support")
endif()

# Setup required compiler flags and include system libs
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wno-unused-private-field")
    if (NOT MSVC)
        set(THREADS_PREFER_PTHREAD_FLAG ON)
        target_link_libraries(vitex PRIVATE ${CMAKE_DL_LIBS} pthread)
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(vitex PUBLIC ${CMAKE_DL_LIBS} pthread)
endif()
if (MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "/ENTRY:mainCRTStartup")
    set(CMAKE_SHARED_LINKER_FLAGS "/SUBSYSTEM:WINDOWS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /bigobj")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP /bigobj")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        if ((VI_BULLET3 AND VI_SIMD) OR VI_SIMD)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX512")
        elseif (VI_BULLET3)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
        endif()
    endif()
else()
    if (WIN32 AND MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows -D_WIN32_WINNT=0x0600")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mwindows -D_WIN32_WINNT=0x0600")
    elseif (NOT (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm"))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
    endif()
endif()

# Enable required ASM compiler
if (WIN32)
    target_link_libraries(vitex PUBLIC 
        ws2_32.lib
        mswsock.lib)
    target_link_libraries(vitex PRIVATE
        d3d11.lib
        d3dcompiler.lib
        crypt32.lib)
endif()
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

# Include main headers and sources as well as installation targets
target_compile_definitions(vitex PRIVATE
    -DVI_EXPORT
    -DNOMINMAX
    -D_GNU_SOURCE)
target_include_directories(vitex PUBLIC ${PROJECT_SOURCE_DIR}/src/)