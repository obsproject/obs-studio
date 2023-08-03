option(ENABLE_WHATSNEW "Enable WhatsNew dialog" ON)

if(ENABLE_WHATSNEW AND TARGET OBS::browser-panels)
  if(OS_MACOS)
    include(cmake/feature-macos-update.cmake)
  elseif(OS_LINUX)
    find_package(MbedTLS REQUIRED)
    find_package(nlohmann_json REQUIRED)
    target_link_libraries(obs-studio PRIVATE MbedTLS::MbedTLS nlohmann_json::nlohmann_json OBS::blake2)

    target_sources(
      obs-studio
      PRIVATE # cmake-format: sortable
              update/crypto-helpers-mbedtls.cpp
              update/crypto-helpers.hpp
              update/models/whatsnew.hpp
              update/shared-update.cpp
              update/shared-update.hpp
              update/update-helpers.cpp
              update/update-helpers.hpp)
  endif()

  target_enable_feature(obs-studio "What's New panel" WHATSNEW_ENABLED)
endif()
