#Resolve refs linkage
if (WIN32)
    target_link_libraries(mavi PRIVATE
            d3d11.lib
            d3dcompiler.lib)
endif()