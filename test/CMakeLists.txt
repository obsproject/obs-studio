
add_subdirectory(test-input)

if(WIN32)
	add_subdirectory(win)
endif()

if(APPLE AND UNIX)
	add_subdirectory(osx)
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
    add_subdirectory(linux)
endif()