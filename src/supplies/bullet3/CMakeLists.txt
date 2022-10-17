set(TH_WITH_BULLET3 ON CACHE BOOL "Enable Bullet3 built-in library")
if (TH_WITH_BULLET3)
	#Resolve source targets
	file(GLOB_RECURSE SOURCE_BULLET3
			${CMAKE_CURRENT_LIST_DIR}/*.inl*
			${CMAKE_CURRENT_LIST_DIR}/*.h*
			${CMAKE_CURRENT_LIST_DIR}/*.c*
			${CMAKE_CURRENT_LIST_DIR}/*.cc*
			${CMAKE_CURRENT_LIST_DIR}/*.hpp*
			${CMAKE_CURRENT_LIST_DIR}/*.cpp*
			${CMAKE_CURRENT_LIST_DIR}/*.hxx*
			${CMAKE_CURRENT_LIST_DIR}/*.cxx*)
	list (APPEND SOURCE ${SOURCE_BULLET3})
	message(STATUS "Bullet3 library enabled")
endif()