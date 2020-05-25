
project(watcher)

set(watcher_SOURCES
      watcher.c
      watcher-source.c
      queue.c)

add_library(watcher MODULE
      ${watcher_SOURCES})
target_link_libraries(watcher
      libobs)

install_obs_plugin_with_data(watcher data)