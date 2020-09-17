#Resolve needed options for different compilers and refs linkage
if (NOT MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    if (WIN32 AND MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows -D_WIN32_WINNT=0x0600")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mwindows -D_WIN32_WINNT=0x0600")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat=0 -std=c++14 -O0 -fexceptions -fno-strict-aliasing")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-function-declaration -lz -O0 -fexceptions -fno-strict-aliasing")
		else()
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat=0 -std=c++14 -msse2 -fexceptions -fno-strict-aliasing")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-function-declaration -lz -msse2 -fexceptions -fno-strict-aliasing")
		endif()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat=0 -std=c++14 -fexceptions")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-implicit-function-declaration -fexceptions")
    endif()
else()
    set(CMAKE_SHARED_LINKER_FLAGS "/SUBSYSTEM:WINDOWS")
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /bigobj")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG /bigobj")
    set(CMAKE_C_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /bigobj")
    set(CMAKE_C_FLAGS_RELEASE "/MD /O2 /Ob2 /DNDEBUG /bigobj")
    target_compile_definitions(tomahawk PRIVATE
            -D_CRT_SECURE_NO_WARNINGS
            -D_SCL_SECURE_NO_WARNINGS)
    target_compile_options(tomahawk PRIVATE
            /wd4305
            /wd4244
            /wd4316
            /wd4018
            /wd4200)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		target_compile_options(tomahawk PRIVATE /wd4267)
	endif()
endif()

#Network libraries for Windows
if (WIN32)
    target_link_libraries(tomahawk PRIVATE
            ws2_32.lib
            mswsock.lib)
endif()