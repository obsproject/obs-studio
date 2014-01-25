if(APPLE)
	cmake_minimum_required(VERSION 2.8.12)
else()
	cmake_minimum_required(VERSION 2.8.11)
endif()

project(obs-studio)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(ObsHelpers)

if(NOT _one_time_action_done)
	if(MSVC)
		file(GENERATE
			OUTPUT "${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user"
			INPUT "${CMAKE_SOURCE_DIR}/cmake/ALL_BUILD.vcxproj.user.in"
			CONDITION ${MSVC})
	endif()
	set(_one_time_action_done TRUE CACHE INTERNAL "")
endif()

if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(CXX11 REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_FLAGS}")

if(${CMAKE_C_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	set(CMAKE_COMPILER_IS_CLANG TRUE)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-unused-parameter -Wall -Wextra")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wno-unused-function -Wno-unused-parameter -Wall -Wextra")
	
	option(USE_LIBC++ "Use libc++ instead of libstdc++" ${APPLE})
	if(USE_LIBC++)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	endif()
endif()

if(WIN32)
	add_definitions(-DUNICODE -D_UNICODE)
endif()

if(APPLE)
	set(CMAKE_MACOSX_RPATH TRUE)
	set(CMAKE_INSTALL_RPATH
		"${CMAKE_INSTALL_RPATH};@loader_path/;@executable_path/")
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

if(NOT INSTALLER_RUN)
	add_subdirectory(deps)
	add_subdirectory(libobs)

	if(WIN32)
		add_subdirectory(libobs-d3d11)
		add_subdirectory(libobs-d3d9)
	endif()

	add_subdirectory(libobs-opengl)
	add_subdirectory(obs)
	add_subdirectory(plugins)
	add_subdirectory(test)
else()
	obs_generate_multiarch_installer()
endif()

obs_install_additional()

# CPack Configuration

if(APPLE AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "Bundle")
elseif(WIN32 AND NOT CPACK_GENERATOR)
	set(CPACK_GENERATOR "NSIS" "ZIP")
endif()

set(CPACK_PACKAGE_NAME "OBS Studio")
set(CPACK_PACKAGE_VENDOR "obsproject.com")
set(CPACK_PACKAGE_VERSION "0.0.1")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OBS Studio - Multi-platform broadcasting software")

if(INSTALLER_RUN)
	file(TO_NATIVE_PATH "${OBS_EXECUTABLE32_DESTINATION}/obs" _obs32)
	file(TO_NATIVE_PATH "${OBS_EXECUTABLE64_DESTINATION}/obs" _obs64)

	#Workaround a cmake bug, remove when fixed
	string(REPLACE "\\" "\\\\" _obs32 "${_obs32}")
	string(REPLACE "\\" "\\\\" _obs64 "${_obs64}")

	set(CPACK_PACKAGE_EXECUTABLES
		"${_obs32}" "OBS Studio (32bit)"
		"${_obs64}" "OBS Studio (64bit)")
	set(CPACK_CREATE_DESKTOP_LINKS
		"${_obs32}"
		"${_obs64}")
else()
	file(TO_NATIVE_PATH "${OBS_EXECUTABLE_DESTINATION}/obs" _obs)

	#Workaround a cmake bug, remove when fixed
	string(REPLACE "\\" "\\\\" _obs "${_obs}")

	set(CPACK_PACKAGE_EXECUTABLES "${_obs}" "OBS Studio")
	set(CPACK_CREATE_DESKTOP_LINKS "${_obs}")
endif()

set(CPACK_BUNDLE_NAME "OBS Studio")
set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/cmake/osxbundle/Info.plist")
set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obs.icns")
set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_SOURCE_DIR}/cmake/osxbundle/obslaunch.sh")

set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
set(CPACK_NSIS_MODIFY_PATH ON)

if(INSTALLER_RUN)
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "OBSStudio")
	set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
	set(CPACK_PACKAGE_FILE_NAME "obs-studio")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
	if(WIN32)
		set(CPACK_PACKAGE_NAME "OBS Studio (64bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "OBSStudio64")
	set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
	set(CPACK_PACKAGE_FILE_NAME "obs-studio-x64")
else()
	if(WIN32)
		set(CPACK_PACKAGE_NAME "OBS Studio (32bit)")
	endif()
	set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "OBSStudio32")
	set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES32")
	set(CPACK_PACKAGE_FILE_NAME "obs-studio-x86")
endif()

include(CPack)
