find_package(Boost 1.49 REQUIRED)

if(ENABLE_UI)
	find_package(Qt5Widgets REQUIRED)
	include_directories(SYSTEM
		"${CMAKE_SOURCE_DIR}/UI/obs-frontend-api")

	if(APPLE)
		find_library(COCOA Cocoa)
		include_directories(${COCOA})
	endif()
endif()

if(MSVC)
	set(obs-ndi_PLATFORM_DEPS
		w32-pthreads)
endif()

set(obs-ndi_HEADERS
	ndi.hpp
	ndi-finder.hpp
	ndi-receiver.hpp
	)
set(obs-ndi_SOURCES
	ndi.cpp
	ndi-utils.cpp
	ndi-source.cpp
	ndi-output.cpp
	ndi-finder.cpp
	ndi-receiver.cpp)

add_library(obs-ndi MODULE
	${obs-ndi_HEADERS}
	${obs-ndi_SOURCES})
target_link_libraries(obs-ndi
	libobs
	Qt5::Widgets
	${obs-ndi_UI_DEPS}
	${obs-ndi_PLATFORM_DEPS})

install_obs_plugin_with_data(obs-ndi data)
