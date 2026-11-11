include_guard(DIRECTORY)

option(ENABLE_WHATSNEW "Enable WhatsNew dialog" ON)

if(ENABLE_WHATSNEW AND TARGET OBS::browser-panels)
  if(OS_MACOS)
    include(cmake/feature-macos-update.cmake)
  elseif(OS_LINUX)
    find_package(nlohmann_json 3.11 REQUIRED)

    include("${CMAKE_SOURCE_DIR}/cmake/common/crypto-backend.cmake")

    if(NOT TARGET OBS::blake2)
      add_subdirectory("${CMAKE_SOURCE_DIR}/deps/blake2" "${CMAKE_BINARY_DIR}/deps/blake2")
    endif()

    get_target_property(crypto_helpers_source OBS::crypto OBS_CRYPTO_HELPERS_SOURCE)

    target_link_libraries(obs-studio PRIVATE OBS::crypto nlohmann_json::nlohmann_json OBS::blake2)

    target_sources(
      obs-studio
      PRIVATE
        utility/WhatsNewBrowserInitThread.cpp
        utility/WhatsNewBrowserInitThread.hpp
        utility/WhatsNewInfoThread.cpp
        utility/WhatsNewInfoThread.hpp
        ${crypto_helpers_source}
        utility/crypto-helpers.hpp
        utility/models/whatsnew.hpp
        utility/update-helpers.cpp
        utility/update-helpers.hpp
    )
  endif()

  target_enable_feature(obs-studio "What's New panel" WHATSNEW_ENABLED)
endif()
