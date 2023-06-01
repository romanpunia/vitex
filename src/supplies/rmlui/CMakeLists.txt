set(VI_FREETYPE ON CACHE BOOL "Enable FreeType library")
set(VI_RMLUI ON CACHE BOOL "Enable RmlUi built-in library")
if (VI_RMLUI)
	#Resolve source targets
	file(GLOB_RECURSE SOURCE_RMLUI
			${CMAKE_CURRENT_LIST_DIR}/*.inl*
			${CMAKE_CURRENT_LIST_DIR}/*.h*
			${CMAKE_CURRENT_LIST_DIR}/*.c*
			${CMAKE_CURRENT_LIST_DIR}/*.cc*
			${CMAKE_CURRENT_LIST_DIR}/*.hpp*
			${CMAKE_CURRENT_LIST_DIR}/*.cpp*
			${CMAKE_CURRENT_LIST_DIR}/*.hxx*
			${CMAKE_CURRENT_LIST_DIR}/*.cxx*)
	
	#Resolve FreeType targets
	find_path(FREETYPE_LOCATION ft2build.h PATH_SUFFIXES "freetype2")
	if (FREETYPE_LOCATION)
		find_package(Freetype QUIET)
		if (NOT Freetype_FOUND)
			find_library(Freetype_FOUND "freetype")
		endif()
	endif()
	
	#Resolve font engine targets
	if (NOT VI_FREETYPE OR (NOT Freetype_FOUND AND NOT FREETYPE_LIBRARIES))
		unset(Freetype_FOUND CACHE)
		unset(FREETYPE_LIBRARIES CACHE)
		list(FILTER SOURCE_RMLUI EXCLUDE REGEX "rmlui[\\\/]Source[\\\/]Core[\\\/]FontEngineDefault")
		message("RmlUi default font engine disabled")
	endif()
	
	#Cleanup
	unset(FREETYPE_LOCATION CACHE)
	unset(FREETYPE_DIR CACHE)
	unset(FREETYPE_LOCATION CACHE)
	list (APPEND SOURCE ${SOURCE_RMLUI})
	message(STATUS "RmlUi library enabled")
endif()
#Check for FreeType