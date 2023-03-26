if(YOUTUBE_CLIENTID
   AND YOUTUBE_SECRET
   AND YOUTUBE_CLIENTID_HASH
   AND YOUTUBE_SECRET_HASH)
  target_sources(obs-studio PRIVATE auth-youtube.cpp auth-youtube.hpp youtube-api-wrappers.cpp youtube-api-wrappers.hpp
                                    window-youtube-actions.cpp window-youtube-actions.hpp)

  target_enable_feature(obs-studio "YouTube API connection" YOUTUBE_ENABLED)
else()
  target_disable_feature(obs-studio "YouTube API connection")
  set(YOUTUBE_SECRET_HASH 0)
  set(YOUTUBE_CLIENTID_HASH 0)
endif()
