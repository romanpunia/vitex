# Include installation targets
install(TARGETS vitex DESTINATION lib)
install(FILES
        src/vitex/layer/processors.h
        DESTINATION include/vitex/layer)
install(FILES
        src/vitex/network/http.h
        src/vitex/network/sqlite.h
        src/vitex/network/mongo.h
        src/vitex/network/pq.h
        src/vitex/network/smtp.h
        DESTINATION include/vitex/network)
install(FILES
        src/vitex/config.hpp
        src/vitex/bindings.h
        src/vitex/compute.h
        src/vitex/core.h
        src/vitex/layer.h
        src/vitex/network.h
        src/vitex/scripting.h
        src/vitex/vitex.h
        DESTINATION include/vitex)