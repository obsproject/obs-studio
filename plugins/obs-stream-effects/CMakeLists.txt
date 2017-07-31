cmake_minimum_required(VERSION 3.8.2)
PROJECT(obs-stream-effects)

################################################################################
# Version
################################################################################
SET(VERSION_MAJOR 0)
SET(VERSION_MINOR 2)
SET(VERSION_PATCH 0)
#configure_file(
#	"${PROJECT_SOURCE_DIR}/#Resources/package.in.bat"
#	"${PROJECT_SOURCE_DIR}/#Resources/package.bat"
#)
#configure_file(
#	"${PROJECT_SOURCE_DIR}/#Resources/Installer.in.iss"
#	"${PROJECT_SOURCE_DIR}/#Resources/Installer.iss"
#)
configure_file(
	"${PROJECT_SOURCE_DIR}/source/version.h.in"
	"${PROJECT_BINARY_DIR}/source/version.h"
)

################################################################################
# Code
################################################################################
SET(obs-stream-effects_HEADERS
	"${PROJECT_SOURCE_DIR}/source/plugin.h"
	"${PROJECT_SOURCE_DIR}/source/filter-displacement.h"
	"${PROJECT_SOURCE_DIR}/source/filter-blur.h"
	"${PROJECT_SOURCE_DIR}/source/filter-shape.h"
	"${PROJECT_SOURCE_DIR}/source/filter-transform.h"
	"${PROJECT_SOURCE_DIR}/source/gs-helper.h"
	"${PROJECT_BINARY_DIR}/source/version.h"
	"${PROJECT_SOURCE_DIR}/source/strings.h"
)
SET(obs-stream-effects_SOURCES
	"${PROJECT_SOURCE_DIR}/source/plugin.cpp"
	"${PROJECT_SOURCE_DIR}/source/filter-displacement.cpp"
	"${PROJECT_SOURCE_DIR}/source/filter-blur.cpp"
	"${PROJECT_SOURCE_DIR}/source/filter-shape.cpp"
	"${PROJECT_SOURCE_DIR}/source/filter-transform.cpp"
	"${PROJECT_SOURCE_DIR}/source/gs-helper.cpp"
)
SET(obs-stream-effects_DATA
	"${PROJECT_SOURCE_DIR}/data/locale/en-US.ini"
	"${PROJECT_SOURCE_DIR}/data/effects/bilateral-blur.effect"
	"${PROJECT_SOURCE_DIR}/data/effects/box-blur.effect"
	"${PROJECT_SOURCE_DIR}/data/effects/gaussian-blur.effect"
	"${PROJECT_SOURCE_DIR}/data/effects/displace.effect"
	"${PROJECT_SOURCE_DIR}/data/effects/color-conversion.effect"
)
SET(obs-stream-effects_LIBRARIES
)
#set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group(TREE "${PROJECT_SOURCE_DIR}" FILES ${obs-stream-effects_DATA})

################################################################################
# Standalone and OBS Studio Build Data
################################################################################
if(TARGET libobs)
	# OBS Studio Specific

	# Directories
	INCLUDE_DIRECTORIES(
		"${CMAKE_SOURCE_DIR}"
		"${PROJECT_BINARY_DIR}"
		"${PROJECT_BINARY_DIR}/source"
		"${PROJECT_SOURCE_DIR}"
		"${PROJECT_SOURCE_DIR}/source"
	)
	SET(LIBOBS_LIBRARIES libobs)
else()
	# Standlone Specific

	SET(PATH_OBSStudio "" CACHE PATH "OBS Studio Source Code Directory")
	if(PATH_OBSStudio STREQUAL "")
		message(FATAL_ERROR "PATH_OBSStudio not set!")
		return()
	endif()
	if(NOT EXISTS "${PATH_OBSStudio}/libobs/obs-module.h")
		message(FATAL_ERROR "PATH_OBSStudio invalid!")
		return()
	endif()

	# Find OBS Libraries
	SET(obsPath "${PATH_OBSStudio}")
	INCLUDE("${PATH_OBSStudio}/cmake/external/Findlibobs.cmake")

	# Compiling
	INCLUDE_DIRECTORIES(
		"${PROJECT_BINARY_DIR}"
		"${PROJECT_BINARY_DIR}/source"
		"${PROJECT_SOURCE_DIR}"
		"${PROJECT_SOURCE_DIR}/source"
		"${PATH_OBSStudio}"
	)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

################################################################################
# Build
################################################################################
ADD_LIBRARY(obs-stream-effects MODULE
	${obs-stream-effects_HEADERS}
	${obs-stream-effects_SOURCES}
	${obs-stream-effects_DATA}
)
TARGET_LINK_LIBRARIES(obs-stream-effects
	${LIBOBS_LIBRARIES}
	${obs-stream-effects_LIBRARIES}
)

if(TARGET libobs)
	install_obs_plugin_with_data(obs-stream-effects data)
else()
	# All Warnings, Extra Warnings, Pedantic
	if(MSVC)
	  # Force to always compile with W4
	  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	  else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	  endif()
	elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	  # Update if necessary
	  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic")
	endif()
	
	math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
	add_custom_command(TARGET obs-stream-effects POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${PROJECT_SOURCE_DIR}/data"
		"${PROJECT_SOURCE_DIR}/#Build/data/obs-plugins/obs-stream-effects"
	)
	add_custom_command(TARGET obs-stream-effects POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"$<TARGET_FILE:obs-stream-effects>"
		"${PROJECT_SOURCE_DIR}/#Build/obs-plugins/${BITS}bit/$<TARGET_FILE_NAME:obs-stream-effects>"
	)
	add_custom_command(TARGET obs-stream-effects POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		"$<TARGET_FILE_DIR:obs-stream-effects>/obs-stream-effects.pdb"
		"${PROJECT_SOURCE_DIR}/#Build/obs-plugins/${BITS}bit/obs-stream-effects.pdb"
	)
endif()
