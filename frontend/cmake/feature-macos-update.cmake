include_guard(DIRECTORY)

find_package(nlohmann_json 3.11 REQUIRED)

if(NOT TARGET OBS::blake2)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/blake2" "${CMAKE_BINARY_DIR}/deps/blake2")
endif()

target_sources(
  obs-studio
  PRIVATE
    utility/crypto-helpers-mac.mm
    utility/crypto-helpers.hpp
    utility/models/branches.hpp
    utility/models/whatsnew.hpp
    utility/update-helpers.cpp
    utility/update-helpers.hpp
    utility/WhatsNewBrowserInitThread.cpp
    utility/WhatsNewBrowserInitThread.hpp
    utility/WhatsNewInfoThread.cpp
    utility/WhatsNewInfoThread.hpp
)

target_link_libraries(
  obs-studio
  PRIVATE "$<LINK_LIBRARY:FRAMEWORK,Security.framework>" nlohmann_json::nlohmann_json OBS::blake2
)
