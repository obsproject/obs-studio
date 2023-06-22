if(NOT DEFINED YOUTUBE_CLIENTID
   OR "${YOUTUBE_CLIENTID}" STREQUAL ""
   OR NOT DEFINED YOUTUBE_SECRET
   OR "${YOUTUBE_SECRET}" STREQUAL ""
   OR NOT DEFINED YOUTUBE_CLIENTID_HASH
   OR "${YOUTUBE_CLIENTID_HASH}" STREQUAL ""
   OR NOT DEFINED YOUTUBE_SECRET_HASH
   OR "${YOUTUBE_SECRET_HASH}" STREQUAL "")
  if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
    target_disable_feature(obs-youtube "YouTube OAuth connection")
  endif()
else()
  if(NOT TARGET OBS::obf)
    add_subdirectory("${CMAKE_SOURCE_DIR}/deps/obf" "${CMAKE_BINARY_DIR}/deps/obf")
  endif()
  if(NOT TARGET OBS::oauth-local-redirect)
    add_subdirectory("${CMAKE_SOURCE_DIR}/deps/oauth-service/local-redirect"
                     "${CMAKE_BINARY_DIR}/deps/oauth-service/local-redirect")
  endif()
  if(NOT TARGET OBS::oauth-service-base)
    add_subdirectory("${CMAKE_SOURCE_DIR}/deps/oauth-service/service-base"
                     "${CMAKE_BINARY_DIR}/deps/oauth-service/service-base")
  endif()

  find_package(CURL)
  find_package(nlohmann_json)
  find_qt(COMPONENTS Core Widgets)

  target_sources(obs-youtube PRIVATE forms/OBSYoutubeActions.ui)

  target_sources(
    obs-youtube
    PRIVATE # cmake-format: sortable
            clickable-label.hpp
            lineedit-autoresize.cpp
            lineedit-autoresize.hpp
            window-youtube-actions.cpp
            window-youtube-actions.hpp
            youtube-api.hpp
            youtube-chat.cpp
            youtube-chat.hpp
            youtube-oauth.cpp
            youtube-oauth.hpp)

  target_link_libraries(obs-youtube PRIVATE OBS::frontend-api OBS::obf OBS::oauth-service-base
                                            OBS::oauth-local-redirect Qt::Core Qt::Widgets)

  target_compile_definitions(
    obs-youtube
    PRIVATE OAUTH_ENABLED YOUTUBE_CLIENTID="${YOUTUBE_CLIENTID}" YOUTUBE_CLIENTID_HASH=0x${YOUTUBE_CLIENTID_HASH}
            YOUTUBE_SECRET="${YOUTUBE_SECRET}" YOUTUBE_SECRET_HASH=0x${YOUTUBE_SECRET_HASH})

  set_target_properties(
    obs-youtube
    PROPERTIES AUTOMOC ON
               AUTOUIC ON
               AUTORCC ON
               AUTOUIC_SEARCH_PATHS forms)

  if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
    target_enable_feature(obs-youtube "YouTube OAuth connection")
  else()
    target_include_directories(obs-youtube PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
  endif()
endif()
