project(win-decklink)

include(IDLFileHelper)

set(win-decklink-sdk_IDLS
	decklink-sdk/DeckLinkAPI.idl
	)

set(win-decklink-sdk_HEADERS
	decklink-sdk/DeckLinkAPIVersion.h
	)

set(win-decklink_HEADERS
	../platform.hpp
	../decklink.hpp
	../decklink-device-instance.hpp
	../decklink-device-discovery.hpp
	../decklink-device.hpp
	../decklink-device-mode.hpp
	../audio-repack.h
	../audio-repack.hpp
	)

set(win-decklink_SOURCES
	../plugin-main.cpp
	../decklink.cpp
	../decklink-device-instance.cpp
	../decklink-device-discovery.cpp
	../decklink-device.cpp
	../decklink-device-mode.cpp
	../audio-repack.c
	platform.cpp)

add_idl_files(win-decklink-sdk_GENERATED_FILES
	${win-decklink-sdk_IDLS})

include_directories(
	${CMAKE_CURRENT_BINARY_DIR})

add_library(win-decklink MODULE
	${win-decklink_SOURCES}
	${win-decklink_HEADERS}
	${win-decklink-sdk_HEADERS}
	${win-decklink-sdk_GENERATED_FILES})

target_link_libraries(win-decklink
	libobs)

install_obs_plugin_with_data(win-decklink ../data)
