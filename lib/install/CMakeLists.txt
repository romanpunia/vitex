#Project's installation target
install(TARGETS tomahawk DESTINATION lib)
install(FILES
        src/tomahawk/tomahawk.h
        DESTINATION include/tomahawk)
install(FILES
        src/tomahawk/core/audio.h
        src/tomahawk/core/compute.h
        src/tomahawk/core/engine.h
        src/tomahawk/core/graphics.h
        src/tomahawk/core/network.h
        src/tomahawk/core/rest.h
        src/tomahawk/core/script.h
        DESTINATION include/tomahawk/core)
install(FILES
        src/tomahawk/network/bson.h
        src/tomahawk/network/http.h
        src/tomahawk/network/mongodb.h
        src/tomahawk/network/smtp.h
        DESTINATION include/tomahawk/network)
install(FILES
        src/tomahawk/script/core.h
        src/tomahawk/script/rest.h
        DESTINATION include/tomahawk/script)
install(FILES
        src/tomahawk/audio/effects.h
        src/tomahawk/audio/filters.h
        DESTINATION include/tomahawk/audio)
install(FILES
        src/tomahawk/engine/components.h
        src/tomahawk/engine/processors.h
        src/tomahawk/engine/renderers.h
        DESTINATION include/tomahawk/engine)
install(FILES
        src/tomahawk/engine/gui/context.h
        src/tomahawk/engine/gui/elements.h
        src/tomahawk/engine/gui/layout.h
        src/tomahawk/engine/gui/logical.h
        src/tomahawk/engine/gui/shapes.h
        DESTINATION include/tomahawk/engine/gui)