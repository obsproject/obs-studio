if(SPARKLE_APPCAST_URL AND SPARKLE_PUBLIC_KEY)
  find_library(SPARKLE Sparkle)
  mark_as_advanced(SPARKLE)

  target_sources(obs-studio PRIVATE update/mac-update.cpp update/mac-update.hpp update/sparkle-updater.mm)
  set_source_files_properties(update/sparkle-updater.mm PROPERTIES COMPILE_FLAGS -fobjc-arc)
  target_link_libraries(obs-studio PRIVATE ${SPARKLE})

  target_enable_feature(obs-studio "Sparkle updater" ENABLE_SPARKLE_UPDATER)
else()
  target_disable_feature(obs-studio "Sparkle updater")
endif()
