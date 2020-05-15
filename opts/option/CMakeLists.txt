#Project's log options
option(THAWK_INFO "allow to log info messages" ON)
option(THAWK_WARN "allow to log warning messages" ON)
option(THAWK_ERROR "allow to log error messages" ON)

#Project's log level
if (THAWK_INFO AND THAWK_WARN AND THAWK_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTHAWK_DLEVEL=3)
elseif (THAWK_WARN AND THAWK_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTHAWK_DLEVEL=2)
elseif (THAWK_ERROR)
    target_compile_definitions(tomahawk PUBLIC -DTHAWK_DLEVEL=1)
else()
    target_compile_definitions(tomahawk PUBLIC -DTHAWK_DLEVEL=0)
endif()