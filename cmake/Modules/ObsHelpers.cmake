set(OBS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/rundir")

set(OBS_EXECUTABLE_DESTINATION "bin")
set(OBS_LIBRARY_DESTINATION "lib")
set(OBS_PLUGIN_DESTINATION "lib/obs-plugins")
set(OBS_DATA_DESTINATION "share/obs")

if(WIN32 OR APPLE)
    set(_struct_def FALSE)
else()
    set(_struct_def TRUE)
endif()

option(UNIX_STRUCTURE "Build with standard unix filesystem structure" ${_struct_def})

if(NOT UNIX_STRUCTURE)
	set(OBS_EXECUTABLE_DESTINATION ".")
	set(OBS_LIBRARY_DESTINATION ".")
	set(OBS_PLUGIN_DESTINATION "obs-plugins")
	set(OBS_DATA_DESTINATION "data")
else()
	add_definitions(-DOBS_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}/")
endif()

add_definitions(-DOBS_DATA_PATH="${OBS_DATA_DESTINATION}")

function(obs_install_additional)
	set(addfdir "${CMAKE_SOURCE_DIR}/additional_install_files")
	if(DEFINED ENV{obsAdditionalInstallFiles})
		set(addfdir "$ENV{obsAdditionalInstallFiles}")
	endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_lib_suffix 64)
	else()
		set(_lib_suffix 32)
	endif()

	install(DIRECTORY "${addfdir}/misc/"
		DESTINATION "."
		USE_SOURCE_PERMISSIONS)
	install(DIRECTORY "${addfdir}/data/"
		DESTINATION "${OBS_DATA_DESTINATION}"
		USE_SOURCE_PERMISSIONS)
	install(DIRECTORY "${addfdir}/libs${_lib_suffix}/"
		DESTINATION "${OBS_LIBRARY_DESTINATION}"
		USE_SOURCE_PERMISSIONS)
	install(DIRECTORY "${addfdir}/exec${_lib_suffix}/"
		DESTINATION "${OBS_EXECUTABLE_DESTINATION}"
		USE_SOURCE_PERMISSIONS)
endfunction()

macro(install_obs_core target)
	install(TARGETS ${target}
		LIBRARY DESTINATION "${OBS_LIBRARY_DESTINATION}"
		RUNTIME DESTINATION "${OBS_EXECUTABLE_DESTINATION}")
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			"$<TARGET_FILE:${target}>" "${OBS_OUTPUT_DIR}/$<CONFIGURATION>/$<TARGET_FILE_NAME:${target}>"
		VERBATIM)
endmacro()

macro(install_obs_plugin target)
	install(TARGETS ${target}
		LIBRARY DESTINATION "${OBS_PLUGIN_DESTINATION}"
		RUNTIME DESTINATION "${OBS_PLUGIN_DESTINATION}")
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			"$<TARGET_FILE:${target}>" "${OBS_OUTPUT_DIR}/$<CONFIGURATION>/obs-plugins/$<TARGET_FILE_NAME:${target}>"
		VERBATIM)
endmacro()

macro(install_obs_data target datadir datadest)
	install(DIRECTORY ${datadir}/
		DESTINATION "${OBS_DATA_DESTINATION}/${datadest}"
		USE_SOURCE_PERMISSIONS)
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CMAKE_CURRENT_SOURCE_DIR}/${datadir}" "${OBS_OUTPUT_DIR}/$<CONFIGURATION>/data/${datadest}"
		VERBATIM)
endmacro()

macro(install_obs_plugin_data target datadir)
	install_obs_plugin(${target})
	install_obs_data(${target} "${datadir}" "obs-plugins/${target}")
endmacro()
