if(RESTREAM_CLIENTID AND RESTREAM_HASH MATCHES "(0|[a-fA-F0-9]+)")
  if(NOT TARGET OBS::obf)
    add_subdirectory("${CMAKE_SOURCE_DIR}/deps/obf" "${CMAKE_BINARY_DIR}/deps/obf")
  endif()
  if(NOT TARGET OBS::oauth-obs-browser-login)
    add_subdirectory("${CMAKE_SOURCE_DIR}/deps/oauth-service/obs-browser-login"
                     "${CMAKE_BINARY_DIR}/deps/oauth-service/obs-browser-login")
  endif()
  if(NOT TARGET OBS::oauth-service-base)
    add_subdirectory("${CMAKE_SOURCE_DIR}/deps/oauth-service/service-base"
                     "${CMAKE_BINARY_DIR}/deps/oauth-service/service-base")
  endif()

  if(NOT OAUTH_BASE_URL)
    set(OAUTH_BASE_URL
        "https://auth.obsproject.com/"
        CACHE STRING "Default OAuth base URL")

    mark_as_advanced(OAUTH_BASE_URL)
  endif()

  find_qt(COMPONENTS Core Widgets)

  target_sources(
    obs-restream PRIVATE # cmake-format: sortable
                         restream-browser-widget.cpp restream-browser-widget.hpp restream-oauth.cpp restream-oauth.hpp)

  target_link_libraries(obs-restream PRIVATE OBS::obf OBS::oauth-service-base OBS::oauth-obs-browser-login Qt::Core
                                             Qt::Widgets)

  target_compile_definitions(
    obs-restream PRIVATE OAUTH_ENABLED OAUTH_BASE_URL="${OAUTH_BASE_URL}" RESTREAM_CLIENTID="${RESTREAM_CLIENTID}"
                         RESTREAM_HASH=0x${RESTREAM_HASH})

  set_target_properties(
    obs-restream
    PROPERTIES AUTOMOC ON
               AUTOUIC ON
               AUTORCC ON)

  if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
    target_enable_feature(obs-restream "Restream OAuth connection")
  endif()
else()
  if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
    target_disable_feature(obs-restream "Resteam OAuth connection")
  endif()
endif()
