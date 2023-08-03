if(YOUTUBE_CLIENTID
   AND YOUTUBE_SECRET
   AND YOUTUBE_CLIENTID_HASH MATCHES "(0|[a-fA-F0-9]+)"
   AND YOUTUBE_SECRET_HASH MATCHES "(0|[a-fA-F0-9]+)")
  target_sources(
    obs-studio
    PRIVATE # cmake-format: sortable
            auth-youtube.cpp
            auth-youtube.hpp
            window-dock-youtube-app.cpp
            window-dock-youtube-app.hpp
            window-youtube-actions.cpp
            window-youtube-actions.hpp
            youtube-api-wrappers.cpp
            youtube-api-wrappers.hpp)

  target_enable_feature(obs-studio "YouTube API connection" YOUTUBE_ENABLED)
else()
  target_disable_feature(obs-studio "YouTube API connection")
  set(YOUTUBE_SECRET_HASH 0)
  set(YOUTUBE_CLIENTID_HASH 0)
endif()
