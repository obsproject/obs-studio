#copy data files and libs into (additional) directory (trees)
#todo: improve dependency interaction

set(OBS_CORE_LIBS "" CACHE INTERNAL "obs core libs")
set(OBS_CORE_LIB_TARGETS "" CACHE INTERNAL "obs core lib targets")

function(obs_add_core_lib lib)
	get_property(location TARGET ${lib} PROPERTY LOCATION)
	list(APPEND OBS_CORE_LIBS ${location})
	set(OBS_CORE_LIBS ${OBS_CORE_LIBS} CACHE INTERNAL "")

	get_filename_component(filename ${location} NAME)

	foreach(target ${OBS_CORE_LIB_TARGETS})
		get_property(tar_location TARGET ${target} PROPERTY LOCATION)
		get_filename_component(dir ${tar_location} DIRECTORY)
		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy ${location}
							 "${dir}/")

		set_property(DIRECTORY ${dir} APPEND PROPERTY
			ADDITIONAL_MAKE_CLEAN_FILES ${filename})
	endforeach()
endfunction()

function(obs_add_core_lib_target target)
	list(APPEND OBS_CORE_LIB_DIRS ${target})
	set(OBS_CORE_LIB_TARGETS ${OBS_CORE_LIB_TARGETS} CACHE INTERNAL "")

	get_property(tar_location TARGET ${target} PROPERTY LOCATION)
	get_filename_component(dir ${tar_location} DIRECTORY)

	foreach(lib ${OBS_CORE_LIBS})
		get_filename_component(filename ${lib} NAME)
		add_custom_command(TARGET ${target} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy ${lib}
							 "${dir}/")

		set_property(DIRECTORY APPEND PROPERTY
			ADDITIONAL_MAKE_CLEAN_FILES "${dir}/${filename}")
	endforeach()
endfunction()

set(OBS_DATA_DIRS "" CACHE INTERNAL "data_dirs")
set(OBS_DATA_SOURCES "" CACHE INTERNAL "source_dirs")

function(obs_add_data_source prefix)
	string(MD5 hash ${prefix})
	set(source_name "OBS_DATA_SOURCES_${hash}")
	set(prefix_name "OBS_DATA_SOURCE_${hash}")
	set(args ${ARGN})

	foreach(target ${OBS_DATA_DIRS})
		file(COPY
			${args}
		     DESTINATION
			"${target}/${prefix}")
	endforeach()

	set(${source_name} ${args} CACHE INTERNAL "")
	set(${prefix_name} ${prefix} CACHE INTERNAL "")
	list(APPEND OBS_DATA_SOURCES ${hash})
	set(OBS_DATA_SOURCES ${OBS_DATA_SOURCES} CACHE INTERNAL "")
endfunction()

function(obs_add_data_dir dir)
	set(dir "${obs_BINARY_DIR}/${dir}")
	list(APPEND OBS_DATA_DIRS ${dir})
	set(OBS_DATA_DIRS ${OBS_DATA_DIRS} CACHE INTERNAL "")

	foreach(hash ${OBS_DATA_SOURCES})
		set(source_name "OBS_DATA_SOURCES_${hash}")
		set(prefix_name "OBS_DATA_SOURCE_${hash}")
		file(COPY
			${${source_name}}
		     DESTINATION
			"${dir}/${${prefix_name}}")
	endforeach()
endfunction()
