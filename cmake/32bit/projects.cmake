# OBS CMake 32-bit slice module

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-lint: disable=E1121
# cmake-format: on

include_guard(GLOBAL)

include(compilerconfig)

# legacy_check: Helper macro to automatically include available 32-bit build script
macro(legacy_check)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/32bit-build.cmake")
    include(cmake/32bit-build.cmake)
  endif()
  return()
endmacro()

# cmake-format: off
# target_disable_feature: Stub macro for 32-bit projects
macro(target_disable_feature)
endmacro()

# target_disable: Stub macro for 32-bit projects
macro(target_disable)
endmacro()
# cmake-format: on

# check_uuid: Helper function to check for valid UUID
function(check_uuid uuid_string return_value)
  set(valid_uuid TRUE)
  set(uuid_token_lengths 8 4 4 4 12)
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
  # cmake-format: off
  set(${return_value} ${valid_uuid} PARENT_SCOPE)
  # cmake-format: on
endfunction()

if(OS_WINDOWS)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/windows/buildspec.cmake")
  add_subdirectory(libobs)
  add_subdirectory(plugins/win-capture)
  add_subdirectory(plugins/win-dshow)
endif()
