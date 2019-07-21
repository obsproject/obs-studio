project(websocketclient)

#------------------------------------------------------------------------
# Global setup
#
set(CMAKE_CXX_STANDARD 11)

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4503 /wd4267 /wd4244 /wd4996")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/restclient-cpp/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/websocketpp)
include_directories(${CMAKE_SOURCE_DIR}/libobs)

#------------------------------------------------------------------------
# JSON
#
set(JSON_BuildTests OFF CACHE INTERNAL "")
# set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(nlohmann_json)

#------------------------------------------------------------------------
# OpenSSL
#
find_package(OpenSSL 1.1 REQUIRED)

if(NOT ${OPENSSL_FOUND})
	message(FATAL_ERROR "OpenSSL not found")
endif()

message(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OPENSSL_CRYPTO_LIBRARY: ${OPENSSL_CRYPTO_LIBRARY}")
message(STATUS "OPENSSL_SSL_LIBRARY: ${OPENSSL_SSL_LIBRARY}")
message(STATUS "OPENSSL_VERSION: ${OPENSSL_VERSION}")

include_directories(${OPENSSL_INCLUDE_DIR})

#------------------------------------------------------------------------
# ASIO
#
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include)
include_directories(${CURL_INCLUDE_DIR})

#------------------------------------------------------------------------
# WebSocket
#
set(websocketclient_SOURCES
	WebsocketClient.cpp
	JanusWebsocketClientImpl.cpp
	WowzaWebsocketClientImpl.cpp
	MillicastWebsocketClientImpl.cpp
	EvercastWebsocketClientImpl.cpp
	restclient-cpp/source/connection.cc
	restclient-cpp/source/helpers.cc
	restclient-cpp/source/restclient.cc)
if(WIN32)
	set(websocketclient_SOURCES ${websocketclient_SOURCES} dllmain.cpp)
endif()

add_definitions(-DWEBSOCKETCLIENT_EXPORTS)

set(websocketclient_HEADERS
	WebsocketClient.h
	JanusWebsocketClientImpl.h
	WowzaWebsocketClientImpl.h
	MillicastWebsocketClientImpl.h
	EvercastWebsocketClientImpl.h)

add_library(websocketclient SHARED
	${websocketclient_SOURCES}
	${websocketclient_HEADERS})

target_link_libraries(websocketclient
	PUBLIC libobs
	PUBLIC ${CURL_LIB}
	PUBLIC ${OPENSSL_LIBRARIES}
	PRIVATE nlohmann_json::nlohmann_json)

install_obs_plugin(websocketclient)
