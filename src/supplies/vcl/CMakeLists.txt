#Resolve headers and compile options
target_include_directories(tomahawk PRIVATE ${CMAKE_CURRENT_LIST_DIR})
set(TH_USE_SIMD ON CACHE BOOL "Enable SIMD instructions (release mode perf. gain)")
if (NOT TH_USE_SIMD)
	target_compile_definitions(tomahawk PRIVATE
			-DTH_NO_SIMD)
endif()