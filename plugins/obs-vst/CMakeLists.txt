if(DISABLE_UI)
	message(STATUS "UI disabled,so vst plugin disabled")
	return()
endif()
if(DISABLE_VST)
	message(STATUS "obs-vst is disabled")
	return()
endif()

cmake_minimum_required(VERSION 3.5.1)

project(obs-vst C CXX)

if (WIN32)
	include(common.cmake)

	# Proto file
	get_filename_component(papi_proto "obs_vst_api.proto" ABSOLUTE)
	get_filename_component(papi_proto_path "${papi_proto}" PATH)

	# Generated sources
	set(papi_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/obs_vst_api.pb.cc")
	set(papi_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/obs_vst_api.pb.h")
	set(papi_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/obs_vst_api.grpc.pb.cc")
	set(papi_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/obs_vst_api.grpc.pb.h")

	add_custom_command(
	      OUTPUT "${papi_proto_srcs}" "${papi_proto_hdrs}" "${papi_grpc_srcs}" "${papi_grpc_hdrs}"
	      COMMAND ${_PROTOBUF_PROTOC}
	      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
		--cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
		-I "${papi_proto_path}"
		--plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
		"${papi_proto}"
	      DEPENDS "${papi_proto}")

	include_directories("${CMAKE_CURRENT_BINARY_DIR}")

	# papi_grpc_proto
	add_library(papi_grpc_proto
		${papi_grpc_srcs}
		${papi_grpc_hdrs}
		${papi_proto_srcs}
		${papi_proto_hdrs})
	target_link_libraries(papi_grpc_proto
		${_REFLECTION}
		${_GRPC_GRPCPP}
		${_PROTOBUF_LIBPROTOBUF})

endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

option(VST_USE_BUNDLED_HEADERS "Build with Bundled Headers" ON)

if(VST_USE_BUNDLED_HEADERS)
	message(STATUS "Using the bundled VST header.")
	include_directories(vst_header)
	set(vst_HEADER
		vst_header/aeffectx.h)
else()
	set(VST_INCLUDE_DIR "" CACHE PATH
		"Path to Steinburg headers (e.g. C:/VST3 SDK/pluginterfaces/vst2.x)")

	message(WARNING "You should only use the Steinburg headers for debugging or local
	 builds. It is illegal to distribute the Steinburg headers with anything, and
	 possibly against the GPL to distribute the binaries from the resultant compile.")
	include_directories(${VST_INCLUDE_DIR})
	set(vst_HEADER
		${VST_INCLUDE_DIR}/aeffectx.h)
endif()

if(APPLE)
	find_library(FOUNDATION_FRAMEWORK Foundation)
	find_library(COCOA_FRAMEWORK Cocoa)
endif(APPLE)

set(obs-vst_SOURCES
	obs-vst.cpp
	VSTPlugin.cpp
	grpc_vst_communicatorClient.cpp)

if(APPLE)
	list(APPEND obs-vst_SOURCES
		mac/VSTPlugin-osx.mm)

elseif(WIN32)
	list(APPEND obs-vst_SOURCES
		win/VSTPlugin-win.cpp
		${papi_proto_srcs}
		${papi_grpc_srcs})

elseif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	list (APPEND obs-vst_SOURCES
		linux/VSTPlugin-linux.cpp)
endif()

list(APPEND obs-vst_HEADERS
	headers/VSTPlugin.h
	headers/grpc_vst_communicatorClient.h)


add_library(obs-vst MODULE
	${obs-vst_SOURCES}
	${obs-vst_HEADERS}
	${vst-HEADER})

target_link_libraries(obs-vst
	libobs)

set_target_properties(obs-vst PROPERTIES FOLDER "plugins")

if(APPLE)
	target_link_libraries(obs-vst
		${COCOA_FRAMEWORK}
		${FOUNDATION_FRAMEWORK})
endif(APPLE)

if (WIN32)
	target_link_libraries(obs-vst
	  ${_REFLECTION}
	  ${_GRPC_GRPCPP}
	  ${_PROTOBUF_LIBPROTOBUF}
	)
endif(WIN32)

install_obs_plugin_with_data(obs-vst data)

target_compile_features(obs-vst PRIVATE cxx_std_17)

if (WIN32)
	project(win-streamlabs-vst C CXX)

	set(win-streamlabs-vst_SOURCES
	  WIN32 proxy/win-streamlabs-vst.cpp
	  proxy/VstWindow.cpp
	  proxy/VstModule.cpp
	  ${papi_proto_srcs}
	  ${papi_grpc_srcs}
	)
	
	add_executable(win-streamlabs-vst
	  ${win-streamlabs-vst_SOURCES}
	  ${papi_proto_srcs}
	  ${papi_grpc_srcs}
	)

	target_link_libraries(win-streamlabs-vst
	  papi_grpc_proto
	  ${_REFLECTION}
	  ${_GRPC_GRPCPP}
	  ${_PROTOBUF_LIBPROTOBUF}
	)

	############################
	# Setup crashpad libraries
	############################

	if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")

	include(ExternalProject)
	include(DownloadProject.cmake)

	if(WIN32)
		download_project(
			PROJ crashpad
			URL https://obsstudionodes3.streamlabs.com/crashpad/crashpad-release-1.0.21-win-x64.tar.gz
			UPDATE_DISCONNECTED 1
		)
	elseif(APPLE)
		download_project(
			PROJ crashpad
			URL https://obsstudionodes3.streamlabs.com/crashpad/crashpad-release-1.0.21-osx.zip
			UPDATE_DISCONNECTED 1
		)
	endif()

	ExternalProject_Add(
		crashpad_ep
		DOWNLOAD_COMMAND ""
		SOURCE_DIR "${crashpad_SOURCE_DIR}"
		INSTALL_COMMAND ""
		BUILD_COMMAND ""
		CONFIGURE_COMMAND ""
		BUILD_BYPRODUCTS
			"<SOURCE_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}base${CMAKE_STATIC_LIBRARY_SUFFIX}"
			"<SOURCE_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}util${CMAKE_STATIC_LIBRARY_SUFFIX}"
			"<SOURCE_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}client${CMAKE_STATIC_LIBRARY_SUFFIX}"
	)

	# Our crashpad artifacts assume a particular format
	# <dir>\bin contains external processes
	# <dir>\lib contains static libraries we need to link against
	# <dir>\include contains the primary include path
	# <dir>\include\third_party\mini_chromium contains chromium include files

	add_library(crashpad_util STATIC IMPORTED)
	add_library(crashpad_base STATIC IMPORTED)
	add_library(crashpad_client STATIC IMPORTED)

	# From this, we get three total targets:
	#   crashpad_base
	#   crashpad_util
	#   crashpad_client
	# It's recommended to use util but not required as far as I know.

	ExternalProject_Get_Property(crashpad_ep source_dir)

	set_property(TARGET crashpad_base PROPERTY IMPORTED_LOCATION
		"${source_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}base${CMAKE_STATIC_LIBRARY_SUFFIX}")

	set_property(TARGET crashpad_util PROPERTY IMPORTED_LOCATION
		"${source_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}util${CMAKE_STATIC_LIBRARY_SUFFIX}")

	set_property(TARGET crashpad_client PROPERTY IMPORTED_LOCATION
		"${source_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}client${CMAKE_STATIC_LIBRARY_SUFFIX}")

	target_include_directories(
		crashpad_base
		INTERFACE "${source_dir}/include/third_party/mini_chromium/mini_chromium"
	)

	target_include_directories(
		crashpad_client
		INTERFACE "${source_dir}/include"
	)

	add_library(crashpad INTERFACE)

	target_link_libraries(
		crashpad
		INTERFACE
			crashpad_base
			crashpad_client
			crashpad_util
	)

	target_link_libraries(crashpad_util INTERFACE crashpad_client)

	add_dependencies(crashpad_base crashpad_ep)
	add_dependencies(crashpad_client crashpad_base)
	add_dependencies(crashpad_util crashpad_client)

	target_link_libraries(win-streamlabs-vst crashpad)

	endif()

	target_compile_features(win-streamlabs-vst PRIVATE cxx_std_17)

endif()

if (WIN32)
	install_obs_plugin(win-streamlabs-vst)
endif(WIN32)
