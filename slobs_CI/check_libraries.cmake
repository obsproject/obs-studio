# Get list of installed binaries
file(GLOB_RECURSE INSTALLED_BINARIES "${CMAKE_BINARY_DIR}/rundir/${deps_CONFIGURATION}/*.dll" "${CMAKE_BINARY_DIR}/rundir/${deps_CONFIGURATION}/*.exe")

# use dependency checker on each found binary to find missing dependencies
foreach(binary ${INSTALLED_BINARIES})
    execute_process( 
        COMMAND_ECHO NONE
        COMMAND_ERROR_IS_FATAL ANY
        COMMAND ${deps_checker_SOURCE_DIR}/check_dependencies.cmd 
        ${binary} 
        ${CMAKE_BINARY_DIR}/rundir/${deps_CONFIGURATION}/bin/
        ${deps_CMAKE_SOURCE_DIR} 
        ${deps_CONFIG}
    )
endforeach()

