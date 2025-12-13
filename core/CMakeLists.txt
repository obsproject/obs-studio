# Core Module Aggregator

# Libraries
# Source Modules
add_subdirectory(src/scene-graph)
add_subdirectory(src/scene)          # New location for SceneManager
add_subdirectory(src/pipeline)       # New location for FrameRouter
add_subdirectory(src/rendering)
add_subdirectory(src/compositor)
add_subdirectory(src/audio)
add_subdirectory(src/video)

# Updater
add_subdirectory(updater)

# Utilities
add_subdirectory(utilities/bpm)
# add_subdirectory(utilities/frontend-utility) # If this has CMakeLists

# Scripting
add_subdirectory(scripting/obs-scripting)
