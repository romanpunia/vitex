#Resolve headers and compile options
target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/ref/stb)
target_compile_definitions(tomahawk PRIVATE
        -DSTB_IMAGE_IMPLEMENTATION)