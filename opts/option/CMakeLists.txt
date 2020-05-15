#Project's log options
set(THAWK_INFO true CACHE BOOL "allow to log info messages")
set(THAWK_WARN true CACHE BOOL "allow to log warning messages")
set(THAWK_ERROR true CACHE BOOL "allow to log error messages")
set(THAWK_RESOURCES true CACHE BOOL "allow to compile embedded resources")

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