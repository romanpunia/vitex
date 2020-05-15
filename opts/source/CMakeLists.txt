#Loading sources for project
file(GLOB_RECURSE SOURCE
        ${PROJECT_SOURCE_DIR}/refs/*.inl*
        ${PROJECT_SOURCE_DIR}/refs/*.h*
        ${PROJECT_SOURCE_DIR}/refs/*.c*
        ${PROJECT_SOURCE_DIR}/refs/*.cc*
        ${PROJECT_SOURCE_DIR}/refs/*.hpp*
        ${PROJECT_SOURCE_DIR}/refs/*.cpp*
        ${PROJECT_SOURCE_DIR}/refs/*.hxx*
        ${PROJECT_SOURCE_DIR}/refs/*.cxx*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.inl*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.h*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.c*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.cc*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.hpp*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.cpp*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.hxx*
        ${PROJECT_SOURCE_DIR}/tomahawk/*.cxx*)
file(GLOB_RECURSE RESOURCE ${PROJECT_SOURCE_DIR}/opts/resource/*)

#Resolving default source groups
foreach(ITEM IN ITEMS ${SOURCE})
    get_filename_component(ITEM_PATH "${ITEM}" PATH)
    string(REPLACE "${PROJECT_SOURCE_DIR}" "" ITEM_GROUP "${ITEM_PATH}")
    string(REPLACE "/" "\\" ITEM_GROUP "${ITEM_GROUP}")
    source_group("${ITEM_GROUP}" FILES "${ITEM}")
endforeach()

#Resolving compiled source groups
foreach(ITEM IN ITEMS ${RESOURCE})
    get_filename_component(ITEM_PATH "${ITEM}" PATH)
    string(REPLACE "${PROJECT_SOURCE_DIR}" "" ITEM_GROUP "${ITEM_PATH}")
    string(REPLACE "/" "\\" ITEM_GROUP "${ITEM_GROUP}")
    source_group("${ITEM_GROUP}" FILES "${ITEM}")
endforeach()

#Compiling resources
if (THAWK_RESOURCES)
    add_resource_batch("${PROJECT_SOURCE_DIR}/opts/resource" "${PROJECT_SOURCE_DIR}/opts/resource.h")
    list(APPEND SOURCE "${PROJECT_SOURCE_DIR}/opts/resource.h")
endif()