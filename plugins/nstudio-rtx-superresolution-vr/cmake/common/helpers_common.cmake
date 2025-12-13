# CMake common helper functions module

# cmake-format: off
# cmake-lint: disable=C0103
# cmake-format: on

include_guard(GLOBAL)

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
  set(${return_value}
      ${valid_uuid}
      PARENT_SCOPE)
endfunction()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/plugin-support.c.in")
  configure_file(src/plugin-support.c.in plugin-support.c @ONLY)
  add_library(plugin-support STATIC)
  target_sources(
    plugin-support
    PRIVATE plugin-support.c
    PUBLIC src/plugin-support.h)
  target_include_directories(plugin-support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
  if(OS_LINUX
     OR OS_FREEBSD
     OR OS_OPENBSD)
    # add fPIC on Linux to prevent shared object errors
    set_property(TARGET plugin-support PROPERTY POSITION_INDEPENDENT_CODE ON)
  endif()
endif()
