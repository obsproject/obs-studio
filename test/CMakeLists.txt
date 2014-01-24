
add_subdirectory(test-input)

if(WIN32)
	add_subdirectory(win)
endif()

if(APPLE AND UNIX)
	add_subdirectory(osx)
endif()
