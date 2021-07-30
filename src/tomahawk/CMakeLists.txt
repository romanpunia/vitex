#Resolve headers and compile options for main project and then for all internal dependencies
target_include_directories(tomahawk PUBLIC ${PROJECT_SOURCE_DIR}/src/)
target_compile_definitions(tomahawk PRIVATE
		-D_GNU_SOURCE
		-DNOMINMAX
		-DTH_EXPORT)
		
#Resolve headers and compile options for Bullet3
if (TH_WITH_BULLET3)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletCollision/BroadphaseCollision)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletCollision/CollisionDispatch)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletCollision/CollisionShapes)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletCollision/Gimpact)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletCollision/NarrowPhaseCollision)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/Character)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/ConstraintSolver)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/Dynamics)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/Featherstone)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/MLCPSolvers)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletDynamics/Vehicle)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/BulletSoftBody)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/LinearMath)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/bullet3/LinearMath/TaskScheduler)
	target_compile_definitions(tomahawk PRIVATE
		-DBT_NO_PROFILE
		-DTH_WITH_BULLET3)
	target_compile_definitions(tomahawk PUBLIC
		-DTH_WITH_BULLET3)
endif()

#Resolve headers and compile options for RmlUI
if (TH_WITH_RMLUI)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/rmlui)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/rmlui/Include)
	target_compile_definitions(tomahawk PRIVATE
			-DRMLUI_STATIC_LIB
			-DRMLUI_MATRIX_ROW_MAJOR)
	target_compile_definitions(tomahawk PUBLIC
		-DTH_WITH_RMLUI)
		
	#Resolve default font engine
	if (NOT TH_USE_FREETYPE OR (NOT Freetype_FOUND AND NOT FREETYPE_LIBRARIES))
		target_compile_definitions(tomahawk PRIVATE
			-DRMLUI_NO_FONT_INTERFACE_DEFAULT)
	else()
		unset(Freetype_FOUND CACHE)
		unset(FREETYPE_LIBRARIES CACHE)
	endif()
endif()

#Resolve headers and compile options for VCL
if (TH_WITH_SIMD)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/vcl)
	target_compile_definitions(tomahawk PUBLIC
			-DTH_WITH_SIMD)
endif()

#Resolve headers and compile options for Wepoll
if (WIN32)
	target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/wepoll)
	target_compile_definitions(tomahawk PUBLIC
			-DTH_WITH_WEPOLL)
endif()

#Resolve headers and compile options for AngelScript
target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/angelscript/include)
target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/angelscript/source)
target_compile_definitions(tomahawk PRIVATE
        -DANGELSCRIPT_EXPORT
        -DAS_USE_STLNAMES)
		
#Resolve headers and compile options for STB
target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/stb)
target_compile_definitions(tomahawk PRIVATE
        -DSTB_IMAGE_IMPLEMENTATION)

#Resolve headers for RapidJSON
target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/rapidjson)

#Resolve headers for RapidXML
target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/rapidxml)

#Resolve headers for TinyFileDialogs
target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/tinyfiledialogs)

#Resolve headers for CQueue
target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/src/supplies/cqueue)