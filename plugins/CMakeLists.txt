include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

if(WIN32)
	add_subdirectory(dshow)
elseif(APPLE)
	add_subdirectory(mac-capture)
endif()

add_subdirectory(obs-ffmpeg)
add_subdirectory(obs-outputs)
