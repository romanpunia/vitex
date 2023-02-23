#Resolve headers and compile options for main project and then for all internal dependencies
target_include_directories(edge PUBLIC ${PROJECT_SOURCE_DIR}/src/)
target_compile_definitions(edge PRIVATE
		-D_GNU_SOURCE
		-DNOMINMAX
		-DED_EXPORT)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_link_libraries(edge PRIVATE ${CMAKE_DL_LIBS})
endif()

#Resolve headers and compile options for Bullet3
if (ED_USE_BULLET3)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletCollision/BroadphaseCollision)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletCollision/CollisionDispatch)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletCollision/CollisionShapes)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletCollision/Gimpact)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletCollision/NarrowPhaseCollision)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/Character)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/ConstraintSolver)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/Dynamics)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/Featherstone)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/MLCPSolvers)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/Vehicle)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletSoftBody)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/LinearMath)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/LinearMath/TaskScheduler)
	target_compile_definitions(edge PRIVATE
		-DBT_NO_PROFILE
		-DED_USE_BULLET3)
	target_compile_definitions(edge PUBLIC -DED_USE_BULLET3)
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		target_compile_definitions(edge PRIVATE -DBT_NO_SIMD_OPERATOR_OVERLOADS)
	endif()
    if (MSVC)
        target_compile_options(edge PRIVATE /wd4305 /wd4244 /wd4018 /wd4267)
    endif()
endif()

#Resolve headers and compile options for RmlUI
if (ED_USE_RMLUI)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/rmlui)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/rmlui/Include)
	target_compile_definitions(edge PRIVATE
			-DRMLUI_STATIC_LIB
			-DRMLUI_MATRIX_ROW_MAJOR)
	target_compile_definitions(edge PUBLIC -DED_USE_RMLUI)
		
	#Resolve default font engine
	if (NOT ED_USE_FREETYPE OR (NOT Freetype_FOUND AND NOT FREETYPE_LIBRARIES))
		target_compile_definitions(edge PRIVATE -DRMLUI_NO_FONT_INTERFACE_DEFAULT)
	else()
		unset(Freetype_FOUND CACHE)
		unset(FREETYPE_LIBRARIES CACHE)
	endif()
endif()

#Resolve headers and compile options for VCL
if (ED_USE_SIMD)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/vcl)
	target_compile_definitions(edge PUBLIC -DED_USE_SIMD)
endif()

#Resolve headers and compile options for Wepoll
if (WIN32)
	target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/wepoll)
	target_compile_definitions(edge PUBLIC -DED_WIED_WEPOLL)
endif()

#Resolve headers for FContext
target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/fcontext)
if (ED_USE_FCTX)
	target_compile_definitions(edge PRIVATE
			-DBOOST_CONTEXT_EXPORT
			-DED_USE_FCTX)
	if (MSVC)
		set_source_files_properties(${FCTX_SOURCE_ASSEMBLY} PROPERTIES COMPILE_FLAGS "/safeseh")
	endif()
	unset(FCTX_SOURCE_ASSEMBLY)
endif()

#Resolve headers and compile options for AngelScript
target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/angelscript/include)
target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source)
target_compile_definitions(edge PRIVATE
        -DANGELSCRIPT_EXPORT
        -DAS_USE_STLNAMES)
		
#Resolve headers and compile options for STB
target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/stb)
target_compile_definitions(edge PRIVATE
        -DSTB_IMAGE_IMPLEMENTATION)

#Resolve headers for RapidJSON
target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/rapidjson)

#Resolve headers for RapidXML
target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/rapidxml)

#Resolve headers for TinyFileDialogs
target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/tinyfiledialogs)

#Resolve headers for CQueue
target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/cqueue)

#Resolve headers for BackwardCpp
target_include_directories(edge PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/backward)