# OBS CMake Windows Architecture Helper

include_guard(GLOBAL)

include(compilerconfig)

if(NOT DEFINED OBS_PARENT_ARCHITECTURE)
  if(CMAKE_GENERATOR_PLATFORM MATCHES "(Win32|x64)")
    set(OBS_PARENT_ARCHITECTURE ${CMAKE_GENERATOR_PLATFORM})
  else()
    message(FATAL_ERROR "Unsupported generator platform for Windows builds: ${CMAKE_GENERATOR_PLATFORM}!")
  endif()
endif()

if(OBS_PARENT_ARCHITECTURE STREQUAL CMAKE_GENERATOR_PLATFORM)
  if(OBS_PARENT_ARCHITECTURE STREQUAL x64)
    execute_process(
      COMMAND
        "${CMAKE_COMMAND}" -S ${CMAKE_CURRENT_SOURCE_DIR} -B ${CMAKE_SOURCE_DIR}/build_x86 -A Win32 -G
        "${CMAKE_GENERATOR}" -DCMAKE_SYSTEM_VERSION:STRING='${CMAKE_SYSTEM_VERSION}' -DOBS_CMAKE_VERSION:STRING=3.0.0
        -DVIRTUALCAM_GUID:STRING=${VIRTUALCAM_GUID} -DCMAKE_MESSAGE_LOG_LEVEL:STRING=${CMAKE_MESSAGE_LOG_LEVEL}
        -DENABLE_CCACHE:BOOL=${ENABLE_CCACHE} -DOBS_PARENT_ARCHITECTURE:STRING=x64
      RESULT_VARIABLE _process_result
      COMMAND_ERROR_IS_FATAL ANY
    )
  endif()
else()
  # legacy_check: Stub macro for child architecture builds
  macro(legacy_check)
  endmacro()

  # target_disable_feature: Stub macro for child architecture builds
  macro(target_disable_feature)
  endmacro()

  # target_disable: Stub macro for child architecture builds
  macro(target_disable)
  endmacro()

  # target_add_resource: Stub macro for child architecture builds
  macro(target_add_resource)
  endmacro()

  # target_export: Stub macro for child architecture builds
  macro(target_export)
  endmacro()

  # set_target_properties_obs: Stub macro for child architecture builds
  macro(set_target_properties_obs)
    set_target_properties(${ARGV})
  endmacro()

  # check_uuid: Helper function to check for valid UUID
  function(check_uuid uuid_string return_value)
    set(valid_uuid TRUE)
    # gersemi: off
    set(uuid_token_lengths 8 4 4 4 12)
    # gersemi: on
    set(token_num 0)

    string(REPLACE "-" ";" uuid_tokens ${uuid_string})
    list(LENGTH uuid_tokens uuid_num_tokens)

    if(uuid_num_tokens EQUAL 5)
      message(DEBUG "UUID ${uuid_string} is valid with 5 tokens.")
      foreach(uuid_token IN LISTS uuid_tokens)
        list(GET uuid_token_lengths ${token_num} uuid_target_length)
        string(LENGTH "${uuid_token}" uuid_actual_length)
        if(uuid_actual_length EQUAL uuid_target_length)
          string(REGEX MATCH "[0-9a-fA-F]+" uuid_hex_match ${uuid_token})
          if(NOT uuid_hex_match STREQUAL uuid_token)
            set(valid_uuid FALSE)
            break()
          endif()
        else()
          set(valid_uuid FALSE)
          break()
        endif()
        math(EXPR token_num "${token_num}+1")
      endforeach()
    else()
      set(valid_uuid FALSE)
    endif()
    message(DEBUG "UUID ${uuid_string} valid: ${valid_uuid}")
    set(${return_value} ${valid_uuid} PARENT_SCOPE)
  endfunction()

  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/buildspec.cmake")

  add_subdirectory(libobs)
  add_subdirectory(plugins/win-capture/get-graphics-offsets)
  add_subdirectory(plugins/win-capture/graphics-hook)
  add_subdirectory(plugins/win-capture/inject-helper)
  add_subdirectory(plugins/win-dshow/virtualcam-module)

  return()
endif()
