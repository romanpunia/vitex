# Include installation targets
install(TARGETS vitex DESTINATION lib)
install(FILES
        src/vitex/audio/effects.h
        src/vitex/audio/filters.h
        DESTINATION include/vitex/audio)
install(FILES
        src/vitex/engine/components.h
        src/vitex/engine/gui.h
        src/vitex/engine/processors.h
        src/vitex/engine/renderers.h
        DESTINATION include/vitex/engine)
install(FILES
        src/vitex/network/http.h
        src/vitex/network/ldb.h
        src/vitex/network/mdb.h
        src/vitex/network/pdb.h
        src/vitex/network/smtp.h
        DESTINATION include/vitex/network)
install(FILES
        src/vitex/config.hpp
        src/vitex/audio.h
        src/vitex/bindings.h
        src/vitex/compute.h
        src/vitex/core.h
        src/vitex/engine.h
        src/vitex/graphics.h
        src/vitex/network.h
        src/vitex/scripting.h
        src/vitex/vitex.h
        DESTINATION include/vitex)