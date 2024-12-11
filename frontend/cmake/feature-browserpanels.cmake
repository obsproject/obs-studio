if(TARGET OBS::browser-panels)
  target_enable_feature(obs-studio "Browser panels" BROWSER_AVAILABLE)

  target_link_libraries(obs-studio PRIVATE OBS::browser-panels)

  target_sources(
    obs-studio
    PRIVATE
      dialogs/OBSExtraBrowsers.cpp
      dialogs/OBSExtraBrowsers.hpp
      docks/BrowserDock.cpp
      docks/BrowserDock.hpp
      utility/ExtraBrowsersDelegate.cpp
      utility/ExtraBrowsersDelegate.hpp
      utility/ExtraBrowsersModel.cpp
      utility/ExtraBrowsersModel.hpp
  )
endif()
