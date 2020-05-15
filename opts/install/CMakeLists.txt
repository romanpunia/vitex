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
        tomahawk/wrapper/audio.h
        tomahawk/wrapper/compute.h
        tomahawk/wrapper/engine.h
        tomahawk/wrapper/graphics.h
        tomahawk/wrapper/network.h
        tomahawk/wrapper/rest.h
        DESTINATION include/tomahawk/wrapper)
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