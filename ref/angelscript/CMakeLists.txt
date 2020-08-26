#Resolve headers and compile options
target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/ref/angelscript/include)
target_include_directories(tomahawk PRIVATE ${PROJECT_SOURCE_DIR}/ref/angelscript/source)
target_compile_definitions(tomahawk PRIVATE
        -DANGELSCRIPT_EXPORT
        -DAS_USE_STLNAMES)