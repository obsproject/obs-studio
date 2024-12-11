if(SPARKLE_APPCAST_URL AND SPARKLE_PUBLIC_KEY)
  find_library(SPARKLE Sparkle)
  mark_as_advanced(SPARKLE)
  target_sources(
    obs-studio
    PRIVATE
      utility/MacUpdateThread.cpp
      utility/MacUpdateThread.hpp
      utility/OBSSparkle.hpp
      utility/OBSSparkle.mm
      utility/OBSUpdateDelegate.h
      utility/OBSUpdateDelegate.mm
  )
  set_source_files_properties(utility/OBSSparkle.mm PROPERTIES COMPILE_FLAGS -fobjc-arc)

  target_link_libraries(obs-studio PRIVATE "$<LINK_LIBRARY:FRAMEWORK,${SPARKLE}>")

  if(OBS_BETA GREATER 0 OR OBS_RELEASE_CANDIDATE GREATER 0)
    set(SPARKLE_UPDATE_INTERVAL 3600) # 1 hour
  else()
    set(SPARKLE_UPDATE_INTERVAL 86400) # 24 hours
  endif()

  target_enable_feature(obs-studio "Sparkle updater" ENABLE_SPARKLE_UPDATER)

  include(cmake/feature-macos-update.cmake)
else()
  set(SPARKLE_UPDATE_INTERVAL 0) # Set anything that's not an empty integer
  target_disable_feature(obs-studio "Sparkle updater")
endif()
