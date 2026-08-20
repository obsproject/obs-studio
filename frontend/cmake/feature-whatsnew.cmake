include_guard(DIRECTORY)

option(ENABLE_WHATSNEW "Enable WhatsNew dialog" ON)

if(ENABLE_WHATSNEW AND TARGET OBS::browser-panels)
  if(OS_MACOS)
    include(cmake/feature-macos-update.cmake)
  elseif(OS_LINUX)
    find_package(nlohmann_json 3.11 REQUIRED)

    if(CRYPTO_BACKEND_U STREQUAL "mbedtls")
      set(crypto_helpers_source utility/crypto-helpers-mbedtls.cpp)
      set(crypto_helpers_library MbedTLS::mbedtls)
    elseif(CRYPTO_BACKEND_U STREQUAL "openssl")
      set(crypto_helpers_source utility/crypto-helpers-openssl.cpp)
      set(crypto_helpers_library OpenSSL::Crypto)
    endif()

    if(NOT TARGET OBS::blake2)
      add_subdirectory("${CMAKE_SOURCE_DIR}/deps/blake2" "${CMAKE_BINARY_DIR}/deps/blake2")
    endif()

    target_link_libraries(obs-studio PRIVATE ${crypto_helpers_library} nlohmann_json::nlohmann_json OBS::blake2)

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
