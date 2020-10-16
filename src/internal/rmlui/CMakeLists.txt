#Resolve headers
target_include_directories(tomahawk PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(tomahawk PRIVATE ${CMAKE_CURRENT_LIST_DIR}/Include)
target_compile_definitions(tomahawk PRIVATE
        -DRMLUI_STATIC_LIB
        -DRMLUI_MATRIX_ROW_MAJOR)

#Check for FreeType
get_target_property(TH_DEFS tomahawk COMPILE_DEFINITIONS)
if (NOT (TH_DEFS MATCHES "^TH_HAS_FREETYPE" OR TH_DEFS MATCHES ";TH_HAS_FREETYPE"))
	target_compile_definitions(tomahawk PRIVATE -DRMLUI_NO_FONT_INTERFACE_DEFAULT)
endif()