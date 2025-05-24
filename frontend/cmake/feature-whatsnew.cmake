include_guard(DIRECTORY)

option(ENABLE_WHATSNEW "Enable WhatsNew dialog" ON)

if(ENABLE_WHATSNEW AND TARGET OBS::browser-panels)
  if(OS_MACOS)
    include(cmake/feature-macos-update.cmake)
  elseif(OS_LINUX)
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
    find_package(MbedTLS REQUIRED)
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG FALSE)
    find_package(nlohmann_json 3.11 REQUIRED)

    if(NOT TARGET OBS::blake2)
      add_subdirectory("${CMAKE_SOURCE_DIR}/deps/blake2" "${CMAKE_BINARY_DIR}/deps/blake2")
    endif()

    target_link_libraries(obs-studio PRIVATE MbedTLS::mbedtls nlohmann_json::nlohmann_json OBS::blake2)

    target_sources(
      obs-studio
      PRIVATE
        utility/crypto-helpers-mbedtls.cpp
        utility/crypto-helpers.hpp
        utility/models/whatsnew.hpp
        utility/update-helpers.cpp
        utility/update-helpers.hpp
        utility/WhatsNewBrowserInitThread.cpp
        utility/WhatsNewBrowserInitThread.hpp
        utility/WhatsNewInfoThread.cpp
        utility/WhatsNewInfoThread.hpp
    )
  endif()

  target_enable_feature(obs-studio "What's New panel" WHATSNEW_ENABLED)
endif()
