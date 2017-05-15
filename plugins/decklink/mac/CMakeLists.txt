project(mac-decklink)

if(DISABLE_DECKLINK)
	message(STATUS "decklink plugin disabled")
	return()
endif()

find_library(COREFOUNDATION CoreFoundation)

include_directories(${COREFOUNDATION})

set(mac-decklink-sdk_HEADERS
	decklink-sdk/DeckLinkAPI.h
	decklink-sdk/DeckLinkAPIConfiguration.h
	decklink-sdk/DeckLinkAPIDeckControl.h
	decklink-sdk/DeckLinkAPIDiscovery.h
	decklink-sdk/DeckLinkAPIModes.h
	decklink-sdk/DeckLinkAPIStreaming.h
	decklink-sdk/DeckLinkAPITypes.h
	decklink-sdk/DeckLinkAPIVersion.h
	)

set(mac-decklink-sdk_SOURCES
	decklink-sdk/DeckLinkAPIDispatch.cpp
	)

set(mac-decklink_HEADERS
	../platform.hpp
	../decklink.hpp
	../decklink-device-instance.hpp
	../decklink-device-discovery.hpp
	../decklink-device.hpp
	../decklink-device-mode.hpp
	../audio-repack.h
	../audio-repack.hpp
	)

set(mac-decklink_SOURCES
	../plugin-main.cpp
	../decklink.cpp
	../decklink-device-instance.cpp
	../decklink-device-discovery.cpp
	../decklink-device.cpp
	../decklink-device-mode.cpp
	../audio-repack.c
	platform.cpp)

add_library(mac-decklink MODULE
	${mac-decklink_SOURCES}
	${mac-decklink_HEADERS}
	${mac-decklink-sdk_HEADERS}
	${mac-decklink-sdk_SOURCES})

target_link_libraries(mac-decklink
	libobs
	${COREFOUNDATION})

install_obs_plugin_with_data(mac-decklink ../data)
