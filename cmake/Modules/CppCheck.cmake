set(CPPCHECK_PROJECTS "")
set(CPPCHECK_ARGUMENTS "")
set(CPPCHECK_PLATFORM "")
set(CPPCHECK_LIBRARIES "")
set(CPPCHECK_ENABLED OFF)

include(CMakeParseArguments)

function(cppcheck)
	if(WIN32)
		set(CPPCHECK_BIN_SUFFIX ".exe")
	endif()

	set(CPPCHECK_PATH "" CACHE PATH "Path to CppCheck binary")
	set(CPPCHECK_BIN "cppcheck${CPPCHECK_BIN_SUFFIX}" CACHE STRING "CppCheck Binary File")
	
	# Detect if CPPCheck is correctly set up
	if(NOT EXISTS "${CPPCHECK_PATH}/${CPPCHECK_BIN}")
		message("CppCheck disabled, ${CPPCHECK_BIN} not found.")
		set(CPPCHECK_ENABLED OFF PARENT_SCOPE)
		return()
	else()
		set(CPPCHECK_ENABLED ON PARENT_SCOPE)
	endif()	
	
	set(CPPCHECK_ENABLE_INCONCLUSIVE OFF CACHE BOOL "Enable inconclusive checks?")
	set(CPPCHECK_ENABLE_MISSING_INCLUDE ON CACHE BOOL "Check for missing includes?")
	set(CPPCHECK_ENABLE_UNUSED_FUNCTION OFF CACHE BOOL "Check for unused functions?")
	set(CPPCHECK_ENABLE_INFORMATION ON CACHE BOOL "Enable information messages?")
	set(CPPCHECK_ENABLE_PORTABILITY ON CACHE BOOL "Enable portability messages?")
	set(CPPCHECK_ENABLE_PERFORMANCE ON CACHE BOOL "Enable performance messages?")
	set(CPPCHECK_ENABLE_WARNING ON CACHE BOOL "Enable warning messages?")	
	set(CPPCHECK_STD_POSIX OFF CACHE BOOL "POSIX Standard Compatibility Checks")
	set(CPPCHECK_STD_C89 OFF CACHE BOOL "C89 Standard Compatibility Checks")
	set(CPPCHECK_STD_C99 OFF CACHE BOOL "C99 Standard Compatibility Checks")
	set(CPPCHECK_STD_C11 ON CACHE BOOL "C11 Standard Compatibility Checks")
	set(CPPCHECK_STD_CPP03 OFF CACHE BOOL "C++03 Standard Compatibility Checks")
	set(CPPCHECK_STD_CPP11 OFF CACHE BOOL "C++11 Standard Compatibility Checks")
	set(CPPCHECK_STD_CPP14 ON CACHE BOOL "C++14 Standard Compatibility Checks")
	set(CPPCHECK_FORCE_C OFF CACHE BOOL "Force checking with C language")
	set(CPPCHECK_FORCE_CPP OFF CACHE BOOL "Force checking with C++ language (overrides CPPCHECK_FORCE_C)")
	set(CPPCHECK_VERBOSE ON CACHE BOOL "Show more detailed error reports")
	set(CPPCHECK_QUIET ON CACHE BOOL "Hide progress reports")
	set(CPPCHECK_ENABLE_INLINE_SUPPRESSION ON CACHE BOOL "Enable inline suppression of warnings via '// cppcheck-suppress id' comments")
	set(CPPCHECK_LIBRARIES "" CACHE STRING "List of Libraries to load separated by semicolon")
	set(CPPCHECK_EXCLUDE_DIRECTORIES "" CACHE STRING "List of directories to exclude separated by semicolon")
	set(CPPCHECK_PARALLEL_TASKS "4" CACHE STRING "Number of threads to use for CppCheck")
	set(CPPCHECK_TEMPLATE "{file}({line}:{column}): {severity} {id}: {message}" CACHE STRING "Template for reported messages")
	set(CPPCHECK_PROJECT_PREFIX "" CACHE STRING "Prefix for Project Names")
	set(CPPCHECK_PROJECT_SUFFIX "_CPPCHECK" CACHE STRING "Prefix for Project Names")
	if(WIN32)
		set(CPPCHECK_WIN32_UNICODE ON CACHE BOOL "Use Unicode character encoding for Win32")
	endif()
	
	mark_as_advanced(CPPCHECK_BIN CPPCHECK_QUIET CPPCHECK_VERBOSE CPPCHECK_LIBRARIES CPPCHECK_ENABLE_INCONCLUSIVE CPPCHECK_PARALLEL_TASKS CPPCHECK_TEMPLATE)
	
	# Parse arguments
	set(cppcheck_options )
	set(cppcheck_oneval )
	set(cppcheck_mulval EXCLUDE)
	cmake_parse_arguments(
		CPPCHECKP
		"${cppcheck_options}"
		"${cppcheck_oneval}"
		"${cppcheck_mulval}"
		${ARGN}
	)
	
	# Detect Architecture (Bitness)
	math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
	
	# Detect Platform
	if(WIN32)
		if(BITS EQUAL "32")
			if(CPPCHECK_WIN32_UNICODE)
				set(CPPCHECK_PLATFORM "win32W")
			else()
				set(CPPCHECK_PLATFORM "win32A")
			endif()
		else()
			set(CPPCHECK_PLATFORM "win64")
		endif()
	elseif(("${CMAKE_SYSTEM_NAME}" MATCHES "Linux") OR ("${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD") OR APPLE)
		if(BITS EQUAL "32")		
			set(CPPCHECK_PLATFORM "unix32")
		else()
			set(CPPCHECK_PLATFORM "unix64")
		endif()
	else()
		set(CPPCHECK_PLATFORM "unspecified")		
	endif()
	if(WIN32)
		list(APPEND CPPCHECK_LIBRARIES "windows")
	elseif("${CMAKE_SYSTEM_NAME}" MATCHES "FreeBSD")
		list(APPEND CPPCHECK_LIBRARIES "bsd")
	elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
		list(APPEND CPPCHECK_LIBRARIES "gnu")
	endif()
	
	# Arguments
	set(CPPCHECK_ARGUMENTS "")
	
	# Compiler
	list(APPEND CPPCHECK_ARGUMENTS --template=${CPPCHECK_TEMPLATE})
	
	# Flags
	if(CPPCHECK_ENABLE_INCONCLUSIVE)
		list(APPEND CPPCHECK_ARGUMENTS --inconclusive)
	endif()
	if(CPPCHECK_VERBOSE)
		list(APPEND CPPCHECK_ARGUMENTS -v)
	endif()
	if(CPPCHECK_QUIET)
		list(APPEND CPPCHECK_ARGUMENTS -q)
	endif()
	if(CPPCHECK_PLATFORM)
		list(APPEND CPPCHECK_ARGUMENTS --platform=${CPPCHECK_PLATFORM})
	endif()
	if(CPPCHECK_PARALLEL_TASKS)
		list(APPEND CPPCHECK_ARGUMENTS -j ${CPPCHECK_PARALLEL_TASKS})
	endif()
	if(CPPCHECK_ENABLE_INLINE_SUPPRESSION)
		list(APPEND CPPCHECK_ARGUMENTS --inline-suppr)
	endif()
	
	# Libraries
	foreach(_library ${CPPCHECK_LIBRARIES})
		list(APPEND CPPCHECK_ARGUMENTS --library=${_library})
	endforeach()
	
	# Exclusion
	foreach(_path ${CPPCHECK_EXCLUDE_DIRECTORIES})
		file(TO_NATIVE_PATH "${_path}" _npath)
		if(MSVC)
			list(APPEND CPPCHECK_ARGUMENTS --suppress=*:${_npath}\\*)
		else()
			list(APPEND CPPCHECK_ARGUMENTS -i${_npath})
		endif()
	endforeach()
	if(CPPCHECKP_EXCLUDE)
		foreach(_path ${CPPCHECKP_EXCLUDE})
			file(TO_NATIVE_PATH "${_path}" _npath)
			if(MSVC)
				list(APPEND CPPCHECK_ARGUMENTS --suppress=*:${_npath}\\*)
			else()
				list(APPEND CPPCHECK_ARGUMENTS -i${_npath})
			endif()
		endforeach()
	endif()
	
	# Checks
	if(CPPCHECK_ENABLE_MISSING_INCLUDE)
		list(APPEND CPPCHECK_ARGUMENTS --enable=missingInclude)
	endif()
	if(CPPCHECK_ENABLE_UNUSED_FUNCTION)
		list(APPEND CPPCHECK_ARGUMENTS --enable=unusedFunction)
	endif()
	if(CPPCHECK_ENABLE_INFORMATION)
		list(APPEND CPPCHECK_ARGUMENTS --enable=information)
	endif()
	if(CPPCHECK_ENABLE_PORTABILITY)
		list(APPEND CPPCHECK_ARGUMENTS --enable=portability)
	endif()
	if(CPPCHECK_ENABLE_PERFORMANCE)
		list(APPEND CPPCHECK_ARGUMENTS --enable=performance)
	endif()
	if(CPPCHECK_ENABLE_WARNING)
		list(APPEND CPPCHECK_ARGUMENTS --enable=warning)
	endif()
	
	# Std
	if(CPPCHECK_STD_POSIX)
		list(APPEND CPPCHECK_ARGUMENTS --std=posix)
	endif()
	if(CPPCHECK_STD_C89)
		list(APPEND CPPCHECK_ARGUMENTS --std=c89)
	endif()
	if(CPPCHECK_STD_C99)
		list(APPEND CPPCHECK_ARGUMENTS --std=c99)
	endif()
	if(CPPCHECK_STD_C11)
		list(APPEND CPPCHECK_ARGUMENTS --std=c11)
	endif()
	if(CPPCHECK_STD_CPP03)
		list(APPEND CPPCHECK_ARGUMENTS --std=c++03)
	endif()
	if(CPPCHECK_STD_CPP11)
		list(APPEND CPPCHECK_ARGUMENTS --std=c++11)
	endif()
	if(CPPCHECK_STD_CPP14)
		list(APPEND CPPCHECK_ARGUMENTS --std=c++14)
	endif()
	
	# Force Language
	if(CPPCHECK_FORCE_CPP)
		list(APPEND CPPCHECK_ARGUMENTS --language=c++)
	elseif(CPPCHECK_FORCE_C)
		list(APPEND CPPCHECK_ARGUMENTS --language=c)		
	endif()
		
	add_custom_target(
		CPPCHECK
	)
	
	# Propagate to parent scope
	set(CPPCHECK_PROJECTS "${CPPCHECK_PROJECTS}" PARENT_SCOPE)
	set(CPPCHECK_ARGUMENTS "${CPPCHECK_ARGUMENTS}" PARENT_SCOPE)
	set(CPPCHECK_PLATFORM "${CPPCHECK_PLATFORM}" PARENT_SCOPE)
	set(CPPCHECK_LIBRARIES "${CPPCHECK_LIBRARIES}" PARENT_SCOPE)
endfunction()

function(cppcheck_add_project u_project)
	list(APPEND CPPCHECK_PROJECTS ${u_project})
	set(CPPCHECK_PROJECTS "${CPPCHECK_PROJECTS}" PARENT_SCOPE)
	
	if(NOT CPPCHECK_ENABLED)
		return()
	endif()	
	
	# Include Directories
	get_target_property(_INCLUDE_DIRECTORIES ${u_project} INCLUDE_DIRECTORIES)
	foreach(_path ${_INCLUDE_DIRECTORIES})
		file(TO_NATIVE_PATH "${_path}" _npath)
		list(APPEND CPPCHECK_ARGUMENTS -I${_npath})
	endforeach()
	
	if(MSVC)
		add_custom_target(
			${CPPCHECK_PROJECT_PREFIX}${u_project}${CPPCHECK_PROJECT_SUFFIX}
			COMMAND "${CPPCHECK_PATH}/${CPPCHECK_BIN}" ${CPPCHECK_ARGUMENTS} --project=${${u_project}_BINARY_DIR}/${u_project}.sln
			COMMAND_EXPAND_LISTS
			VERBATIM
		)
	else()
		# Non-MSVC and Unix (Linux, FreeBSD, APPLE) need to have -I, -i, -D and -U specified manually.
		
		# Defines
		get_target_property(_COMPILE_DEFINITIONS ${u_project} COMPILE_DEFINITIONS)
		foreach(_def ${_COMPILE_DEFINITIONS})
			list(APPEND CPPCHECK_ARGUMENTS -D${_def})
		endforeach()
		
		# Source Files
		get_target_property(_SOURCES ${u_project} SOURCES)
		foreach(_path ${_SOURCES})
			file(TO_NATIVE_PATH "${_path}" _npath)
			list(APPEND CPPCHECK_ARGUMENTS ${_npath})
		endforeach()
		
		add_custom_target(
			${CPPCHECK_PROJECT_PREFIX}${u_project}${CPPCHECK_PROJECT_SUFFIX}
			COMMAND "${CPPCHECK_PATH}/${CPPCHECK_BIN}" ${CPPCHECK_ARGUMENTS}
			COMMAND_EXPAND_LISTS
			VERBATIM
		)		
	endif()
	add_dependencies(CPPCHECK ${CPPCHECK_PROJECT_PREFIX}${u_project}${CPPCHECK_PROJECT_SUFFIX})
endfunction()
