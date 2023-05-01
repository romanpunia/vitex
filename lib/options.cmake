#Check if compiler supports at least C++ 14
if (NOT "${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_14")
    message(FATAL_ERROR "Compiler must have at least C++ 14 standard support")
endif()

#Link system libraries libraries
if (WIN32)
    target_link_libraries(mavi PUBLIC  ws2_32.lib mswsock.lib)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_link_libraries(mavi PUBLIC ${CMAKE_DL_LIBS} pthread)
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
install(TARGETS mavi DESTINATION lib)
install(FILES
        src/mavi/audio/effects.h
        src/mavi/audio/filters.h
        DESTINATION include/mavi/audio)
install(FILES
        src/mavi/core/audio.h
        src/mavi/core/compute.h
        src/mavi/core/core.h
        src/mavi/core/engine.h
        src/mavi/core/graphics.h
        src/mavi/core/network.h
        src/mavi/core/script.h
        src/mavi/core/bindings.h
        DESTINATION include/mavi/core)
install(FILES
        src/mavi/engine/components.h
        src/mavi/engine/gui.h
        src/mavi/engine/processors.h
        src/mavi/engine/renderers.h
        DESTINATION include/mavi/engine)
install(FILES
        src/mavi/network/http.h
        src/mavi/network/mdb.h
        src/mavi/network/pdb.h
        src/mavi/network/smtp.h
        DESTINATION include/mavi/network)
install(FILES
        src/mavi/mavi.h
        DESTINATION include/mavi)