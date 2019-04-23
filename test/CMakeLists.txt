
add_subdirectory(test-input)

if(WIN32)
	add_subdirectory(win)
elseif(APPLE)
	add_subdirectory(osx)
elseif(UNIX)
	add_subdirectory(linux)
endif()
