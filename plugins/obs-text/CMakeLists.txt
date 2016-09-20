if (NOT WIN32)
	return()
endif()

project(obs-text)

if(WIN32)
	set(obs-text_PLATFORM_SOURCES
		gdiplus/obs-text.cpp)
	set(obs-text_PLATFORM_DEPS
		gdiplus)
endif()

add_library(obs-text MODULE
	${obs-text_PLATFORM_SOURCES}
	${obs-text_PLATFORM_HEADERS})
target_link_libraries(obs-text
	libobs
	${obs-text_PLATFORM_DEPS})

install_obs_plugin_with_data(obs-text data)
