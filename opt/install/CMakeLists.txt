#Project's installation target
install(TARGETS tomahawk DESTINATION lib)
install(FILES
        tomahawk/audio.h
        tomahawk/compute.h
        tomahawk/engine.h
        tomahawk/graphics.h
        tomahawk/network.h
        tomahawk/rest.h
        tomahawk/script.h
        tomahawk/tomahawk.h
        DESTINATION include/tomahawk)
install(FILES
        tomahawk/network/bson.h
        tomahawk/network/http.h
        tomahawk/network/mongodb.h
        tomahawk/network/smtp.h
        DESTINATION include/tomahawk/network)
install(FILES
        tomahawk/engine/components.h
        tomahawk/engine/processors.h
        tomahawk/engine/renderers.h
        DESTINATION include/tomahawk/engine)
install(FILES
        tomahawk/engine/gui/context.h
        tomahawk/engine/gui/elements.h
        tomahawk/engine/gui/layout.h
        tomahawk/engine/gui/logical.h
        tomahawk/engine/gui/shapes.h
        DESTINATION include/tomahawk/engine/gui)