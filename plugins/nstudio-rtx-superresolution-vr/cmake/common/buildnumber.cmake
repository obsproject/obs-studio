# CMake build number module

include_guard(GLOBAL)

# Define build number cache file
set(_BUILD_NUMBER_CACHE
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/.CMakeBuildNumber"
    CACHE INTERNAL "OBS build number cache file")

# Read build number from cache file or manual override
if(NOT DEFINED PLUGIN_BUILD_NUMBER AND EXISTS "${_BUILD_NUMBER_CACHE}")
  file(READ "${_BUILD_NUMBER_CACHE}" PLUGIN_BUILD_NUMBER)
  math(EXPR PLUGIN_BUILD_NUMBER "${PLUGIN_BUILD_NUMBER}+1")
elseif(NOT DEFINED PLUGIN_BUILD_NUMBER)
  if($ENV{CI})
    if($ENV{GITHUB_RUN_ID})
      set(PLUGIN_BUILD_NUMBER "$ENV{GITHUB_RUN_ID}")
    elseif($ENV{GITLAB_RUN_ID})
      set(PLUGIN_BUILD_NUMBER "$ENV{GITLAB_RUN_ID}")
    else()
      set(PLUGIN_BUILD_NUMBER "1")
    endif()
  else()
    set(PLUGIN_BUILD_NUMBER "1")
  endif()
endif()
file(WRITE "${_BUILD_NUMBER_CACHE}" "${PLUGIN_BUILD_NUMBER}")
