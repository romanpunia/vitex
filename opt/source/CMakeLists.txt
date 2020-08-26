#Loading sources for project
file(GLOB_RECURSE SOURCE
        ${PROJECT_SOURCE_DIR}/ref/*.inl*
        ${PROJECT_SOURCE_DIR}/ref/*.h*
        ${PROJECT_SOURCE_DIR}/ref/*.c*
        ${PROJECT_SOURCE_DIR}/ref/*.cc*
        ${PROJECT_SOURCE_DIR}/ref/*.hpp*
        ${PROJECT_SOURCE_DIR}/ref/*.cpp*
        ${PROJECT_SOURCE_DIR}/ref/*.hxx*
        ${PROJECT_SOURCE_DIR}/ref/*.cxx*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.inl*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.h*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.c*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.cc*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.hpp*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.cpp*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.hxx*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.cxx*)
file(GLOB_RECURSE RESOURCE ${PROJECT_SOURCE_DIR}/data/*)

#Resolving default source groups
foreach(ITEM IN ITEMS ${SOURCE})
    get_filename_component(ITEM_PATH "${ITEM}" PATH)
    string(REPLACE "${PROJECT_SOURCE_DIR}" "" ITEM_GROUP "${ITEM_PATH}")
    string(REPLACE "/" "\\" ITEM_GROUP "${ITEM_GROUP}")
    source_group("${ITEM_GROUP}" FILES "${ITEM}")
endforeach()

#Resources compile switch option
set(THAWK_RESOURCES true CACHE BOOL "allow to compile embedded resources")

#Compiling resources
if (THAWK_RESOURCES)
    add_resource_batch("${PROJECT_SOURCE_DIR}/data" "${PROJECT_SOURCE_DIR}/tomahawk/data")
    list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/tomahawk/data.h")
    list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/tomahawk/data.cpp")
	source_group("tomahawk" FILES
		"${PROJECT_SOURCE_DIR}/tomahawk/data.h"
		"${PROJECT_SOURCE_DIR}/tomahawk/data.cpp")
else()
    file(WRITE "${PROJECT_SOURCE_DIR}/tomahawk/data.h" "")
    file(WRITE "${PROJECT_SOURCE_DIR}/tomahawk/data.cpp" "")
	message(STATUS "Resource file has been cleared")
endif()