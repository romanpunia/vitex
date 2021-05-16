#Resolve refs linkage
if (WIN32)
    target_link_libraries(tomahawk PRIVATE
            d3d11.lib
            d3dcompiler.lib)
endif()