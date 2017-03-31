project(linux-decklink)

set(linux-decklink-sdk_HEADERS
	decklink-sdk/DeckLinkAPI.h
	decklink-sdk/DeckLinkAPIConfiguration.h
	decklink-sdk/DeckLinkAPIDeckControl.h
	decklink-sdk/DeckLinkAPIDiscovery.h
	decklink-sdk/DeckLinkAPIModes.h
	decklink-sdk/DeckLinkAPITypes.h
	decklink-sdk/DeckLinkAPIVersion.h
	decklink-sdk/LinuxCOM.h
	)

set(linux-decklink-sdk_SOURCES
	decklink-sdk/DeckLinkAPIDispatch.cpp
	)

set(linux-decklink_HEADERS
	../platform.hpp
	../decklink.hpp
	../decklink-device-instance.hpp
	../decklink-device-discovery.hpp
	../decklink-device.hpp
	../decklink-device-mode.hpp
	../audio-repack.h
	../audio-repack.hpp
	)

set(linux-decklink_SOURCES
	../plugin-main.cpp
	../decklink.cpp
	../decklink-device-instance.cpp
	../decklink-device-discovery.cpp
	../decklink-device.cpp
	../decklink-device-mode.cpp
	../audio-repack.c
	platform.cpp)

add_library(linux-decklink MODULE
	${linux-decklink_SOURCES}
	${linux-decklink_HEADERS}
	${linux-decklink-sdk_HEADERS}
	${linux-decklink-sdk_SOURCES})

target_link_libraries(linux-decklink
	libobs)

install_obs_plugin_with_data(linux-decklink ../data)
