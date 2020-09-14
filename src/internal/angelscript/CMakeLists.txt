#Resolve headers and compile options
target_include_directories(tomahawk PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(tomahawk PRIVATE ${CMAKE_CURRENT_LIST_DIR}/source)
target_compile_definitions(tomahawk PRIVATE
        -DANGELSCRIPT_EXPORT
        -DAS_USE_STLNAMES)