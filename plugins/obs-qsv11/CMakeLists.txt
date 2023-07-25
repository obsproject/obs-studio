cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

option(ENABLE_QSV11 "Build Intel QSV11 Hardware Encoder." TRUE)
if(NOT ENABLE_QSV11)
  target_disable_feature(obs-qsv11 "Intel QSV11 Hardware Encoder")
  target_disable(obs-qsv11)
  return()
endif()

add_library(obs-qsv11 MODULE)
add_library(OBS::qsv11 ALIAS obs-qsv11)

target_sources(
  obs-qsv11
  PRIVATE # cmake-format: sortable
          common_utils.cpp
          common_utils.h
          obs-qsv11-plugin-main.c
          obs-qsv11.c
          QSV_Encoder.cpp
          QSV_Encoder.h
          QSV_Encoder_Internal.cpp
          QSV_Encoder_Internal.h)

target_link_libraries(obs-qsv11 PRIVATE OBS::libobs)

# cmake-format: off
set_target_properties_obs(obs-qsv11 PROPERTIES FOLDER plugins/obs-qsv11 PREFIX "")
# cmake-format: on

if(OS_WINDOWS)
  add_subdirectory(obs-qsv-test)

  add_library(libmfx INTERFACE)
  add_library(OBS::libmfx ALIAS libmfx)

  target_sources(
    libmfx
    INTERFACE # cmake-format: sortable
              libmfx/include/mfx_critical_section.h
              libmfx/include/mfx_dispatcher.h
              libmfx/include/mfx_dispatcher_defs.h
              libmfx/include/mfx_dispatcher_log.h
              libmfx/include/mfx_driver_store_loader.h
              libmfx/include/mfx_dxva2_device.h
              libmfx/include/mfx_exposed_functions_list.h
              libmfx/include/mfx_library_iterator.h
              libmfx/include/mfx_load_dll.h
              libmfx/include/mfx_load_plugin.h
              libmfx/include/mfx_plugin_hive.h
              libmfx/include/mfx_vector.h
              libmfx/include/mfx_win_reg_key.h
              libmfx/include/mfxaudio_exposed_functions_list.h
              libmfx/include/msdk/include/mfxadapter.h
              libmfx/include/msdk/include/mfxastructures.h
              libmfx/include/msdk/include/mfxaudio++.h
              libmfx/include/msdk/include/mfxaudio.h
              libmfx/include/msdk/include/mfxcommon.h
              libmfx/include/msdk/include/mfxdefs.h
              libmfx/include/msdk/include/mfxjpeg.h
              libmfx/include/msdk/include/mfxmvc.h
              libmfx/include/msdk/include/mfxplugin++.h
              libmfx/include/msdk/include/mfxplugin.h
              libmfx/include/msdk/include/mfxsession.h
              libmfx/include/msdk/include/mfxstructures.h
              libmfx/include/msdk/include/mfxvideo++.h
              libmfx/include/msdk/include/mfxvideo.h
              libmfx/include/msdk/include/mfxvstructures.h
              libmfx/src/main.cpp
              libmfx/src/mfx_critical_section.cpp
              libmfx/src/mfx_dispatcher.cpp
              libmfx/src/mfx_dispatcher_log.cpp
              libmfx/src/mfx_driver_store_loader.cpp
              libmfx/src/mfx_dxva2_device.cpp
              libmfx/src/mfx_function_table.cpp
              libmfx/src/mfx_library_iterator.cpp
              libmfx/src/mfx_load_dll.cpp
              libmfx/src/mfx_load_plugin.cpp
              libmfx/src/mfx_plugin_hive.cpp
              libmfx/src/mfx_win_reg_key.cpp)

  target_include_directories(libmfx INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libmfx/include/msdk/include"
                                              "${CMAKE_CURRENT_SOURCE_DIR}/libmfx/include")
  get_target_property(mfx_sources libmfx SOURCES)
  list(FILTER mfx_sources INCLUDE REGEX ".*\\.(m|c[cp]?p?|swift)")
  list(FILTER mfx_headers INCLUDE REGEX ".*\\.h(pp)?")
  source_group("libmfx\\Source Files" FILES ${mfx_sources})
  source_group("libmfx\\Header Files" FILES ${mfx_headers})

  configure_file(cmake/windows/obs-module.rc.in obs-qsv11.rc)
  target_sources(obs-qsv11 PRIVATE obs-qsv11.rc)

  target_compile_definitions(obs-qsv11 PRIVATE DX11_D3D)

  target_sources(
    obs-qsv11
    PRIVATE # cmake-format: sortable
            common_directx11.cpp
            common_directx11.h
            common_directx9.cpp
            common_directx9.h
            common_utils_windows.cpp
            device_directx9.cpp
            device_directx9.h)

  target_link_libraries(obs-qsv11 PRIVATE OBS::libmfx d3d9 d3d11 dxva2 dxgi dxguid)
elseif(OS_LINUX)
  find_package(VPL REQUIRED)
  find_package(Libva REQUIRED)

  target_sources(obs-qsv11 PRIVATE common_utils_linux.cpp)

  target_link_libraries(obs-qsv11 PRIVATE Libva::va Libva::drm VPL::VPL)
endif()

get_target_property(target_sources obs-qsv11 SOURCES)
list(FILTER target_sources INCLUDE REGEX ".*\\.(m|c[cp]?p?|swift)")
list(FILTER target_headers INCLUDE REGEX ".*\\.h(pp)?")
source_group("obs-qsv11\\Source Files" FILES ${target_sources})
source_group("obs-qsv11\\Header Files" FILES ${target_headers})
