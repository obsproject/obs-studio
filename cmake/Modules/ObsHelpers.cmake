set(OBS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/rundir")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

if(WIN32 OR APPLE)
	set(_struct_def FALSE)
else()
	set(_struct_def TRUE)
endif()

option(INSTALLER_RUN "Build a multiarch installer, needs to run indenepdently after both archs have compiled" FALSE)
option(UNIX_STRUCTURE "Build with standard unix filesystem structure" ${_struct_def})
if(APPLE)
	option(BUILD_REDISTRIBUTABLE "Fix rpath of external libraries" FALSE)
endif()

if(INSTALLER_RUN AND NOT DEFINED ENV{obsInstallerTempDir})
	message(FATAL_ERROR "Environment variable obsInstallerTempDir is needed for multiarch installer generation")
endif()

if(DEFINED ENV{obsInstallerTempDir})
	file(TO_CMAKE_PATH "$ENV{obsInstallerTempDir}" ENV{obsInstallerTempDir})
endif()

if(DEFINED ENV{obsAdditionalInstallFiles})
	file(TO_CMAKE_PATH "$ENV{obsAdditionalInstallFiles}" ENV{obsAdditionalInstallFiles})
endif()

if(NOT UNIX_STRUCTURE)
	set(OBS_DATA_DESTINATION "data")
	if(APPLE)
		set(OBS_EXECUTABLE_DESTINATION "bin")
		set(OBS_EXECUTABLE32_DESTINATION "bin")
		set(OBS_EXECUTABLE64_DESTINATION "bin")
		set(OBS_LIBRARY_DESTINATION "bin")
		set(OBS_LIBRARY32_DESTINATION "bin")
		set(OBS_LIBRARY64_DESTINATION "bin")
		set(OBS_PLUGIN_DESTINATION "obs-plugins")
		set(OBS_PLUGIN32_DESTINATION "obs-plugins")
		set(OBS_PLUGIN64_DESTINATION "obs-plugins")
		add_definitions(-DOBS_DATA_PATH="../${OBS_DATA_DESTINATION}")
	else()
		set(OBS_EXECUTABLE_DESTINATION "bin/${_lib_suffix}bit")
		set(OBS_EXECUTABLE32_DESTINATION "bin/32bit")
		set(OBS_EXECUTABLE64_DESTINATION "bin/64bit")
		set(OBS_LIBRARY_DESTINATION "bin/${_lib_suffix}bit")
		set(OBS_LIBRARY32_DESTINATION "bin/32bit")
		set(OBS_LIBRARY64_DESTINATION "bin/64bit")
		set(OBS_PLUGIN_DESTINATION "obs-plugins/${_lib_suffix}bit")
		set(OBS_PLUGIN32_DESTINATION "obs-plugins/32bit")
		set(OBS_PLUGIN64_DESTINATION "obs-plugins/64bit")
		add_definitions(-DOBS_DATA_PATH="../../${OBS_DATA_DESTINATION}")
	endif()
	set(OBS_CMAKE_DESTINATION "cmake")
	set(OBS_INCLUDE_DESTINATION "include")
else()
	set(OBS_EXECUTABLE_DESTINATION "bin")
	set(OBS_EXECUTABLE32_DESTINATION "bin32")
	set(OBS_EXECUTABLE64_DESTINATION "bin64")
	set(OBS_LIBRARY_DESTINATION "lib")
	set(OBS_LIBRARY32_DESTINATION "lib32")
	set(OBS_LIBRARY64_DESTINATION "lib64")
	set(OBS_PLUGIN_DESTINATION "lib/obs-plugins")
	set(OBS_PLUGIN32_DESTINATION "lib32/obs-plugins")
	set(OBS_PLUGIN64_DESTINATION "lib64/obs-plugins")
	set(OBS_DATA_DESTINATION "share/obs")
	set(OBS_CMAKE_DESTINATION "lib/cmake")
	set(OBS_INCLUDE_DESTINATION "include/obs")
	add_definitions(-DOBS_DATA_PATH="${OBS_DATA_DESTINATION}")
	add_definitions(-DOBS_INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}/")
endif()

function(obs_finish_bundle)
	if(NOT APPLE OR UNIX_STRUCTURE)
		return()
	endif()

	install(CODE
		"if(DEFINED ENV{FIXUP_BUNDLE})
			execute_process(COMMAND \"${CMAKE_SOURCE_DIR}/cmake/osxbundle/fixup_bundle.sh\" . bin WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}\")
		endif()")
endfunction()

function(obs_generate_multiarch_installer)
	install(DIRECTORY "$ENV{obsInstallerTempDir}/"
		DESTINATION "."
		USE_SOURCE_PERMISSIONS)
endfunction()

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
		USE_SOURCE_PERMISSIONS
		PATTERN ".gitignore" EXCLUDE)
	install(DIRECTORY "${addfdir}/data/"
		DESTINATION "${OBS_DATA_DESTINATION}"
		USE_SOURCE_PERMISSIONS
		PATTERN ".gitignore" EXCLUDE)
	
	if(INSTALLER_RUN)
		install(DIRECTORY "${addfdir}/libs32/"
			DESTINATION "${OBS_LIBRARY32_DESTINATION}"
			USE_SOURCE_PERMISSIONS
			PATTERN ".gitignore" EXCLUDE)
		install(DIRECTORY "${addfdir}/exec32/"
			DESTINATION "${OBS_EXECUTABLE32_DESTINATION}"
			USE_SOURCE_PERMISSIONS
			PATTERN ".gitignore" EXCLUDE)
		install(DIRECTORY "${addfdir}/libs64/"
			DESTINATION "${OBS_LIBRARY64_DESTINATION}"
			USE_SOURCE_PERMISSIONS
			PATTERN ".gitignore" EXCLUDE)
		install(DIRECTORY "${addfdir}/exec64/"
			DESTINATION "${OBS_EXECUTABLE64_DESTINATION}"
			USE_SOURCE_PERMISSIONS
			PATTERN ".gitignore" EXCLUDE)
	else()
		install(DIRECTORY "${addfdir}/libs${_lib_suffix}/"
			DESTINATION "${OBS_LIBRARY_DESTINATION}"
			USE_SOURCE_PERMISSIONS
			PATTERN ".gitignore" EXCLUDE)
		install(DIRECTORY "${addfdir}/exec${_lib_suffix}/"
			DESTINATION "${OBS_EXECUTABLE_DESTINATION}"
			USE_SOURCE_PERMISSIONS
			PATTERN ".gitignore" EXCLUDE)
	endif()
endfunction()

macro(export_obs_core target exportname)
	install(TARGETS ${target}
		EXPORT "${exportname}Target"
		LIBRARY DESTINATION "${OBS_LIBRARY_DESTINATION}"
		RUNTIME DESTINATION "${OBS_EXECUTABLE_DESTINATION}")

	export(TARGETS ${target} FILE "${CMAKE_CURRENT_BINARY_DIR}/${exportname}Target.cmake")
	export(PACKAGE "${exportname}")

	set(CONF_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}")
	configure_file("${exportname}Config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/${exportname}Config.cmake" @ONLY)

	set(CONF_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/${OBS_INCLUDE_DESTINATION}")
	configure_file("${exportname}Config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${exportname}Config.cmake" @ONLY)

	configure_file("${exportname}ConfigVersion.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/${exportname}ConfigVersion.cmake" @ONLY)

	install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${exportname}Config.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/${exportname}ConfigVersion.cmake"
		DESTINATION "${OBS_CMAKE_DESTINATION}/${exportname}")

	install(EXPORT "${exportname}Target"
		DESTINATION "${OBS_CMAKE_DESTINATION}/${exportname}")
endmacro()

macro(install_obs_headers)
	foreach(hdr ${ARGN})
		if("${hdr}" MATCHES "^(/|[a-zA-Z]:[/\\\\]).*$")
			set(subdir)
		else()
			get_filename_component(subdir "${hdr}" DIRECTORY)
			if(subdir)
				set(subdir "/${subdir}")
			endif()
		endif()
		install(FILES "${hdr}" DESTINATION "${OBS_INCLUDE_DESTINATION}${subdir}")
	endforeach()
endmacro()

macro(install_obs_core target)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_bit_suffix "64bit/")
	else()
		set(_bit_suffix "32bit/")
	endif()

	if(APPLE)
		set(_bit_suffix "")
	endif()

	if("${ARGV1}" STREQUAL "EXPORT")
		export_obs_core("${target}" "${ARGV2}")
	else()
		install(TARGETS ${target}
			LIBRARY DESTINATION "${OBS_LIBRARY_DESTINATION}"
			RUNTIME DESTINATION "${OBS_EXECUTABLE_DESTINATION}")
	endif()

	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy
			"$<TARGET_FILE:${target}>"
			"${OBS_OUTPUT_DIR}/$<CONFIGURATION>/bin/${_bit_suffix}$<TARGET_FILE_NAME:${target}>"
		VERBATIM)


	if(DEFINED ENV{obsInstallerTempDir})
		get_property(target_type TARGET ${target} PROPERTY TYPE)
		if("${target_type}" STREQUAL "EXECUTABLE")
			set(tmp_target_dir "${OBS_EXECUTABLE_DESTINATION}")
		else()
			set(tmp_target_dir "${OBS_LIBRARY_DESTINATION}")
		endif()

		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"$<TARGET_FILE:${target}>" "$ENV{obsInstallerTempDir}/${tmp_target_dir}/$<TARGET_FILE_NAME:${target}>"
			VERBATIM)
	endif()
endmacro()

macro(install_obs_plugin target)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_bit_suffix "64bit/")
	else()
		set(_bit_suffix "32bit/")
	endif()

	if(APPLE)
		set(_bit_suffix "")
	endif()

	install(TARGETS ${target}
		LIBRARY DESTINATION "${OBS_PLUGIN_DESTINATION}"
		RUNTIME DESTINATION "${OBS_PLUGIN_DESTINATION}")
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy
			"$<TARGET_FILE:${target}>"
			"${OBS_OUTPUT_DIR}/$<CONFIGURATION>/obs-plugins/${_bit_suffix}$<TARGET_FILE_NAME:${target}>"
		VERBATIM)

	if(DEFINED ENV{obsInstallerTempDir})
		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"$<TARGET_FILE:${target}>" "$ENV{obsInstallerTempDir}/${OBS_PLUGIN_DESTINATION}/$<TARGET_FILE_NAME:${target}>"
			VERBATIM)
	endif()
endmacro()

macro(install_obs_data target datadir datadest)
	install(DIRECTORY ${datadir}/
		DESTINATION "${OBS_DATA_DESTINATION}/${datadest}"
		USE_SOURCE_PERMISSIONS)
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_directory
			"${CMAKE_CURRENT_SOURCE_DIR}/${datadir}" "${OBS_OUTPUT_DIR}/$<CONFIGURATION>/data/${datadest}"
		VERBATIM)

	if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND DEFINED ENV{obsInstallerTempDir})
		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_directory
				"${CMAKE_CURRENT_SOURCE_DIR}/${datadir}" "$ENV{obsInstallerTempDir}/${OBS_DATA_DESTINATION}/${datadest}"
			VERBATIM)
	endif()
endmacro()

macro(install_obs_datatarget target datadest)
	install(TARGETS ${target}
		LIBRARY DESTINATION "${OBS_DATA_DESTINATION}/${datadest}"
		RUNTIME DESTINATION "${OBS_DATA_DESTINATION}/${datadest}")
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy
			"$<TARGET_FILE:${target}>"
			"${OBS_OUTPUT_DIR}/$<CONFIGURATION>/data/${datadest}/$<TARGET_FILE_NAME:${target}>"
		VERBATIM)

	if(DEFINED ENV{obsInstallerTempDir})
		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"$<TARGET_FILE:${target}>"
				"$ENV{obsInstallerTempDir}/${OBS_DATA_DESTINATION}/${datadest}/$<TARGET_FILE_NAME:${target}>"
			VERBATIM)
	endif()
endmacro()

macro(install_obs_plugin_data target datadir)
	install_obs_plugin(${target})
	install_obs_data(${target} "${datadir}" "obs-plugins/${target}")
endmacro()
