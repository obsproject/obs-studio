project(obs-cli)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.61 COMPONENTS program_options REQUIRED)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs" ${Boost_INCLUDE_DIRS} .)

if(WIN32)
	set(obscli_PLATFORM_SOURCES
		monitor_info-windows.cpp
	)
elseif(APPLE)
# TODO
elseif(UNIX)
# TODO
endif()

set(SRC_LIST
	${obscli_PLATFORM_SOURCES}
	main.cpp
	setup_obs.cpp
	setup_obs.hpp
	enum_types.cpp
	enum_types.hpp
	monitor_info.hpp
	event_loop.cpp
	event_loop.hpp
	)
add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} libobs ${Boost_LIBRARIES})

install_obs_core(${PROJECT_NAME})
