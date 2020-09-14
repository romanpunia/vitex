#Loading sources for project
file(GLOB_RECURSE SOURCE
        ${PROJECT_SOURCE_DIR}/src/internal/*.inl*
        ${PROJECT_SOURCE_DIR}/src/internal/*.h*
        ${PROJECT_SOURCE_DIR}/src/internal/*.c*
        ${PROJECT_SOURCE_DIR}/src/internal/*.cc*
        ${PROJECT_SOURCE_DIR}/src/internal/*.hpp*
        ${PROJECT_SOURCE_DIR}/src/internal/*.cpp*
        ${PROJECT_SOURCE_DIR}/src/internal/*.hxx*
        ${PROJECT_SOURCE_DIR}/src/internal/*.cxx*
        ${PROJECT_SOURCE_DIR}/src/tomahawk/*.inl*
        ${PROJECT_SOURCE_DIR}/src/tomahawk/*.h*
        ${PROJECT_SOURCE_DIR}/src/tomahawk/*.c*
        ${PROJECT_SOURCE_DIR}/src/tomahawk/*.cc*
        ${PROJECT_SOURCE_DIR}/src/tomahawk/*.hpp*
        ${PROJECT_SOURCE_DIR}/src/tomahawk/*.cpp*
        ${PROJECT_SOURCE_DIR}/src/tomahawk/*.hxx*
        ${PROJECT_SOURCE_DIR}/src/tomahawk/*.cxx*)

#Resources compile switch option
set(THAWK_RESOURCES true CACHE BOOL "allow to compile embedded resources")

#Compiling resources
if (THAWK_RESOURCES)
    add_resource_batch("${PROJECT_SOURCE_DIR}/src/shaders" "${PROJECT_SOURCE_DIR}/src/tomahawk/core/shaders")
    list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/src/tomahawk/core/shaders.h")
    list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/src/tomahawk/core/shaders.cpp")
else()
    file(WRITE "${PROJECT_SOURCE_DIR}/src/tomahawk/core/shaders.h" "")
    file(WRITE "${PROJECT_SOURCE_DIR}/src/tomahawk/core/shaders.cpp" "")
	message(STATUS "Resource file has been cleared")
endif()

#Resolving default source groups
foreach(ITEM IN ITEMS ${SOURCE})
    get_filename_component(ITEM_PATH "${ITEM}" PATH)
    string(REPLACE "${PROJECT_SOURCE_DIR}" "" ITEM_GROUP "${ITEM_PATH}")
    string(REPLACE "/" "\\" ITEM_GROUP "${ITEM_GROUP}")
    source_group("${ITEM_GROUP}" FILES "${ITEM}")
endforeach()