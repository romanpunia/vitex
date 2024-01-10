# Include installation targets
install(TARGETS mavi DESTINATION lib)
install(FILES
        src/mavi/audio/effects.h
        src/mavi/audio/filters.h
        DESTINATION include/mavi/audio)
install(FILES
        src/mavi/core/audio.h
        src/mavi/core/bindings.h
        src/mavi/core/compute.h
        src/mavi/core/core.h
        src/mavi/core/engine.h
        src/mavi/core/graphics.h
        src/mavi/core/network.h
        src/mavi/core/scripting.h
        DESTINATION include/mavi/core)
install(FILES
        src/mavi/engine/components.h
        src/mavi/engine/gui.h
        src/mavi/engine/processors.h
        src/mavi/engine/renderers.h
        DESTINATION include/mavi/engine)
install(FILES
        src/mavi/network/http.h
        src/mavi/network/ldb.h
        src/mavi/network/mdb.h
        src/mavi/network/pdb.h
        src/mavi/network/smtp.h
        DESTINATION include/mavi/network)
install(FILES
        src/mavi/config.hpp
        src/mavi/mavi.h
        DESTINATION include/mavi)