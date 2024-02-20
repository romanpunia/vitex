# Include headers and sources of internal dependencies
if (VI_ALLOCATOR)
    target_compile_definitions(vitex PUBLIC -DVI_ALLOCATOR)
endif()
if (VI_PESSIMISTIC)
    target_compile_definitions(vitex PUBLIC -DVI_PESSIMISTIC)
endif()
if (VI_BINDINGS)
    target_compile_definitions(vitex PUBLIC -DVI_BINDINGS)
endif()
if (VI_BULLET3)
    target_compile_definitions(vitex PUBLIC -DVI_BULLET3)
    target_compile_definitions(vitex PRIVATE -DBT_NO_PROFILE)
    target_include_directories(vitex PRIVATE
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletCollision/BroadphaseCollision
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletCollision/CollisionDispatch
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletCollision/CollisionShapes
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletCollision/Gimpact
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletCollision/NarrowPhaseCollision
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletDynamics/Character
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletDynamics/ConstraintSolver
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletDynamics/Dynamics
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletDynamics/Featherstone
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletDynamics/MLCPSolvers
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletDynamics/Vehicle
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/BulletSoftBody
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/LinearMath
        ${PROJECT_SOURCE_DIR}/deps/bullet3/src/LinearMath/TaskScheduler)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_definitions(vitex PRIVATE -DBT_NO_SIMD_OPERATOR_OVERLOADS)
    endif()
    if (MSVC)
        target_compile_options(vitex PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:/wd4305>
            $<$<COMPILE_LANGUAGE:CXX>:/wd4244>
            $<$<COMPILE_LANGUAGE:CXX>:/wd4018>
            $<$<COMPILE_LANGUAGE:CXX>:/wd4267>
            $<$<COMPILE_LANGUAGE:CXX>:/wd4056>)
    endif()
endif()
if (VI_RMLUI)
    target_compile_definitions(vitex PUBLIC -DVI_RMLUI)
    target_compile_definitions(vitex PRIVATE
        -DRMLUI_STATIC_LIB
        -DRMLUI_MATRIX_ROW_MAJOR
        -DRMLUI_CUSTOM_CONFIGURATION_FILE="${PROJECT_SOURCE_DIR}/src/vitex/engine/gui/config.hpp")
    target_include_directories(vitex PRIVATE
        ${PROJECT_SOURCE_DIR}/deps/rmlui
        ${PROJECT_SOURCE_DIR}/deps/rmlui/Include)
    if (NOT VI_FREETYPE OR (NOT Freetype_FOUND AND NOT FREETYPE_LIBRARIES))
        target_compile_definitions(vitex PRIVATE -DRMLUI_NO_FONT_INTERFACE_DEFAULT)
    else()
        unset(Freetype_FOUND CACHE)
        unset(FREETYPE_LIBRARIES CACHE)
    endif()
endif()
if (VI_VECTORCLASS)
    target_compile_definitions(vitex PUBLIC -DVI_VECTORCLASS)
    target_include_directories(vitex PRIVATE ${PROJECT_SOURCE_DIR}/deps/vectorclass)
endif()
if (VI_BACKWARDCPP)
    target_compile_definitions(vitex PUBLIC -DVI_BACKWARDCPP)
    target_include_directories(vitex PRIVATE ${PROJECT_SOURCE_DIR}/deps/backward-cpp)
    if (NOT "${BACKWARD_PATHS}" STREQUAL "")
        target_include_directories(vitex PRIVATE ${BACKWARD_PATHS})
    endif()
    if (NOT "${BACKWARD_USES}" STREQUAL "")
        target_compile_definitions(vitex PRIVATE ${BACKWARD_USES})
    endif()
    if (NOT "${BACKWARD_LINKS}" STREQUAL "")
        target_link_libraries(vitex PRIVATE ${BACKWARD_LINKS})
    endif()
    unset(BACKWARD_PATHS CACHE)
    unset(BACKWARD_USES CACHE)
    unset(BACKWARD_LINKS CACHE)
endif()
if (VI_TINYFILEDIALOGS)
    target_compile_definitions(vitex PUBLIC -DVI_TINYFILEDIALOGS)
    target_include_directories(vitex PRIVATE ${PROJECT_SOURCE_DIR}/deps/tinyfiledialogs)
endif()
if (VI_STB)
    target_compile_definitions(vitex PUBLIC -DVI_STB)
    target_compile_definitions(vitex PRIVATE -DSTB_IMAGE_IMPLEMENTATION)
    target_include_directories(vitex PRIVATE ${PROJECT_SOURCE_DIR}/deps/stb)
endif()
if (VI_PUGIXML)
    target_compile_definitions(vitex PUBLIC -DVI_PUGIXML)
    target_include_directories(vitex PRIVATE ${PROJECT_SOURCE_DIR}/deps/pugixml/src)
endif()
if (VI_RAPIDJSON)
    target_compile_definitions(vitex PUBLIC -DVI_RAPIDJSON)
    target_include_directories(vitex PRIVATE ${PROJECT_SOURCE_DIR}/deps/rapidjson/include)
endif()
if (VI_ANGELSCRIPT)
    target_compile_definitions(vitex PUBLIC -DVI_ANGELSCRIPT)
    target_compile_definitions(vitex PRIVATE -DAS_USE_STLNAMES)
    target_include_directories(vitex PRIVATE
        ${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/include
        ${PROJECT_SOURCE_DIR}/deps/angelscript/sdk/angelscript/source)
endif()
if (VI_WEPOLL AND WIN32)
    target_compile_definitions(vitex PUBLIC -DVI_WEPOLL)
    target_include_directories(vitex PRIVATE ${PROJECT_SOURCE_DIR}/deps/wepoll)
endif()
if (VI_FCONTEXT)
    target_compile_definitions(vitex PUBLIC -DVI_FCONTEXT)
    target_compile_definitions(vitex PRIVATE -DBOOST_CONTEXT_EXPORT)
endif()
if (VI_CONCURRENTQUEUE OR TRUE)
    target_include_directories(vitex PRIVATE ${PROJECT_SOURCE_DIR}/deps/concurrentqueue)
endif()