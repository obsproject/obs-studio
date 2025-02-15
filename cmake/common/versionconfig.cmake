# OBS CMake common version helper module

include_guard(GLOBAL)

set(_obs_version ${_obs_default_version})
set(_obs_version_canonical ${_obs_default_version})

# Attempt to automatically discover expected OBS version
if(NOT DEFINED OBS_VERSION_OVERRIDE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git describe --always --tags --dirty=-modified
    OUTPUT_VARIABLE _obs_version
    ERROR_VARIABLE _git_describe_err
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE _obs_version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(_git_describe_err)
    message(FATAL_ERROR "Could not fetch OBS version tag from git.\n" ${_git_describe_err})
  endif()

  if(_obs_version_result EQUAL 0)
    string(REGEX REPLACE "([0-9]+)\\.([0-9]+)\\.([0-9]+).*" "\\1;\\2;\\3" _obs_version_canonical ${_obs_version})
  endif()
elseif(DEFINED OBS_VERSION_OVERRIDE)
  if(OBS_VERSION_OVERRIDE MATCHES "([0-9]+)\\.([0-9]+)\\.([0-9]+).*")
    string(
      REGEX REPLACE
      "([0-9]+)\\.([0-9]+)\\.([0-9]+).*"
      "\\1;\\2;\\3"
      _obs_version_canonical
      ${OBS_VERSION_OVERRIDE}
    )
    set(_obs_version ${OBS_VERSION_OVERRIDE})
  else()
    message(FATAL_ERROR "Invalid version supplied - must be <MAJOR>.<MINOR>.<PATCH>[-(rc|beta)<NUMBER>].")
  endif()
endif()

# Set beta/rc versions if suffix included in version string
if(_obs_version MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+-rc[0-9]+")
  string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.[0-9]+-rc([0-9]+).*$" "\\1" _obs_release_candidate ${_obs_version})
elseif(_obs_version MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+-beta[0-9]+")
  string(REGEX REPLACE "[0-9]+\\.[0-9]+\\.[0-9]+-beta([0-9]+).*$" "\\1" _obs_beta ${_obs_version})
endif()

list(GET _obs_version_canonical 0 OBS_VERSION_MAJOR)
list(GET _obs_version_canonical 1 OBS_VERSION_MINOR)
list(GET _obs_version_canonical 2 OBS_VERSION_PATCH)

set(OBS_RELEASE_CANDIDATE ${_obs_release_candidate})
set(OBS_BETA ${_obs_beta})

string(REPLACE ";" "." OBS_VERSION_CANONICAL "${_obs_version_canonical}")
string(REPLACE ";" "." OBS_VERSION "${_obs_version}")

if(OBS_RELEASE_CANDIDATE GREATER 0)
  message(
    AUTHOR_WARNING
    "******************************************************************************\n"
    "  + OBS-Studio - Release candidate detected, OBS_VERSION is now: ${OBS_VERSION}\n"
    "******************************************************************************"
  )
elseif(OBS_BETA GREATER 0)
  message(
    AUTHOR_WARNING
    "******************************************************************************\n"
    "  + OBS-Studio - Beta detected, OBS_VERSION is now: ${OBS_VERSION}\n"
    "******************************************************************************"
  )
endif()

unset(_obs_default_version)
unset(_obs_version)
unset(_obs_version_canonical)
unset(_obs_release_candidate)
unset(_obs_beta)
unset(_obs_version_result)
