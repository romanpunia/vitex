#Resolve source targets
file(GLOB_RECURSE SOURCE_ANGELSCRIPT
		${CMAKE_CURRENT_LIST_DIR}/*.inl*
		${CMAKE_CURRENT_LIST_DIR}/*.h*
		${CMAKE_CURRENT_LIST_DIR}/*.c*
		${CMAKE_CURRENT_LIST_DIR}/*.cc*
		${CMAKE_CURRENT_LIST_DIR}/*.hpp*
		${CMAKE_CURRENT_LIST_DIR}/*.cpp*
		${CMAKE_CURRENT_LIST_DIR}/*.hxx*
		${CMAKE_CURRENT_LIST_DIR}/*.cxx*)

#Resolve ASM targets
if (MSVC)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm64")
			list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_arm64_msvc.asm")
		else()
			list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_x64_msvc_asm.asm")
		endif()
	elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
		list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_arm_msvc.asm")
	endif()
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^arm")
	list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_arm_gcc.S")
	list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_arm_vita.S")
	list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_arm_xcode.S")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "^aarch64")
	list(APPEND SOURCE_ANGELSCRIPT "${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source/as_callfunc_arm64_gcc.S")
endif()
list (APPEND SOURCE ${SOURCE_ANGELSCRIPT})