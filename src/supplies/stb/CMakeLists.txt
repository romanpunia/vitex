#Resolve headers and compile options
target_include_directories(tomahawk PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(tomahawk PRIVATE
        -DSTB_IMAGE_IMPLEMENTATION)