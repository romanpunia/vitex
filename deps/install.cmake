# Include installation targets
install(TARGETS vitex DESTINATION lib)
install(FILES
        src/vitex/audio/effects.h
        src/vitex/audio/filters.h
        DESTINATION include/vitex/audio)
install(FILES
        src/vitex/core/audio.h
        src/vitex/core/bindings.h
        src/vitex/core/compute.h
        src/vitex/core/core.h
        src/vitex/core/engine.h
        src/vitex/core/graphics.h
        src/vitex/core/network.h
        src/vitex/core/scripting.h
        DESTINATION include/vitex/core)
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
        src/vitex/vitex.h
        DESTINATION include/vitex)