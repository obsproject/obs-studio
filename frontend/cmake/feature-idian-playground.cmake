option(ENABLE_IDIAN_PLAYGROUND "Enable building custom idian widget demo window" OFF)

if(ENABLE_IDIAN_PLAYGROUND)
  target_sources(
    obs-studio
    PRIVATE forms/OBSIdianPlayground.ui dialogs/OBSIdianPlayground.hpp dialogs/OBSIdianPlayground.cpp
  )
  target_enable_feature(obs-studio "Idian Playground" ENABLE_IDIAN_PLAYGROUND)
else()
  target_disable_feature(obs-studio "Idian Playground")
endif()
