cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Widgets)

add_library(live-caption-studio MODULE)
add_library(OBS::live-caption-studio ALIAS live-caption-studio)

target_sources(
  live-caption-studio
  PRIVATE
    plugin-main.cpp
    ai-dock/ai-control-dock.cpp
    ai-dock/ai-control-dock.hpp
    captions/caption-filter.c
    captions/caption-filter.h
    settings/lcs-config.cpp
    settings/lcs-config.hpp
    transcription/engine.hpp
    transcription/engine-registry.cpp
    transcription/engine-registry.hpp
    transcription/whisper-engine.cpp
    transcription/openai-engine.cpp
    transcription/gemini-engine.cpp
    transcription/ollama-engine.cpp
    transcription/custom-api-engine.cpp
)

target_include_directories(live-caption-studio PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(
  live-caption-studio
  PRIVATE OBS::frontend-api OBS::libobs Qt::Widgets
)

set_target_properties_obs(
  live-caption-studio
  PROPERTIES
    FOLDER plugins/live-caption-studio
    PREFIX ""
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_enable_feature(live-caption-studio "Live Caption Studio plugin")
