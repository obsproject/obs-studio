option(ENABLE_WIDGET_PLAYGROUND "Enable building custom widget demo window" OFF)

if(ENABLE_WIDGET_PLAYGROUND)
  target_sources(obs-studio PRIVATE forms/IdianPlayground.ui idian/widget-playground.hpp idian/widget-playground.cpp)
  target_enable_feature(obs-studio "Widget Playground" ENABLE_WIDGET_PLAYGROUND)
endif()
