# target_add_idl_files: Compile IDL file and add generated source files to target
function(target_add_idl_files target)
  set(options target WITH_TLB)
  set(oneValueArgs "")
  set(multiValueArgs "")
  cmake_parse_arguments(PARSE_ARGV 1 _AIF "${options}" "${oneValueArgs}" "${multiValueArgs}")
  set(aif_files ${_AIF_UNPARSED_ARGUMENTS})

  foreach(idl_file IN LISTS aif_files)
    cmake_path(GET idl_file STEM idl_file_name)
    cmake_path(GET idl_file PARENT_PATH idl_file_path)

    set(idl_file_header "${CMAKE_CURRENT_BINARY_DIR}/${idl_file_name}.h")
    set(idl_file_source "${CMAKE_CURRENT_BINARY_DIR}/${idl_file_name}_i.c")

    add_custom_command(
      OUTPUT "${idl_file_header}" "${idl_file_source}"
      DEPENDS "${idl_file}"
      COMMAND midl /h "${idl_file_name}.h" /iid "${idl_file_name}_i.c" "$<$<NOT:$<BOOL:${_AIF_WITH_TLIB}>>:/notlb>"
              /win64 "${CMAKE_CURRENT_SOURCE_DIR}/${idl_file}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMENT "Generate idl files")

    set_source_files_properties(${idl_file} PROPERTIES HEADER_FILE_ONLY TRUE)
    target_sources(${target} PRIVATE "${idl_file_source}" "${idl_file_header}")
  endforeach()
endfunction()
