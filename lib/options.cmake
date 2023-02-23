#Check if compiler supports at least C++ 14
if (NOT "${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_14")
    message(FATAL_ERROR "Compiler must have at least C++ 14 standard support")
endif()

#Link system libraries libraries
if (WIN32)
    target_link_libraries(edge PUBLIC  ws2_32.lib mswsock.lib)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_link_libraries(edge PUBLIC ${CMAKE_DL_LIBS} pthread)
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
install(TARGETS edge DESTINATION lib)
install(FILES
        src/edge/audio/effects.h
        src/edge/audio/filters.h
        DESTINATION include/edge/audio)
install(FILES
        src/edge/core/audio.h
        src/edge/core/compute.h
        src/edge/core/core.h
        src/edge/core/engine.h
        src/edge/core/graphics.h
        src/edge/core/network.h
        src/edge/core/script.h
        src/edge/core/bindings.h
        DESTINATION include/edge/core)
install(FILES
        src/edge/engine/components.h
        src/edge/engine/gui.h
        src/edge/engine/processors.h
        src/edge/engine/renderers.h
        DESTINATION include/edge/engine)
install(FILES
        src/edge/network/http.h
        src/edge/network/mdb.h
        src/edge/network/pdb.h
        src/edge/network/smtp.h
        DESTINATION include/edge/network)
install(FILES
        src/edge/edge.h
        DESTINATION include/edge)