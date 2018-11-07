project(obs-qsv11)

include_directories(libmfx/include/msdk/include)
include_directories(libmfx/include)
include_directories(include)

set(obs-qsv11_libmfx_SOURCES
	libmfx/src/main.cpp
	libmfx/src/mfx_critical_section.cpp
	libmfx/src/mfx_dispatcher.cpp
	libmfx/src/mfx_dispatcher_log.cpp
	libmfx/src/mfx_dxva2_device.cpp
	libmfx/src/mfx_function_table.cpp
	libmfx/src/mfx_library_iterator.cpp
	libmfx/src/mfx_load_dll.cpp
	libmfx/src/mfx_load_plugin.cpp
	libmfx/src/mfx_plugin_hive.cpp
	libmfx/src/mfx_win_reg_key.cpp
	)

set(obs-qsv11_libmfx_HEADERS
	libmfx/include/msdk/include/mfxastructures.h
	libmfx/include/msdk/include/mfxaudio.h
	libmfx/include/msdk/include/mfxaudio++.h
	libmfx/include/msdk/include/mfxcommon.h
	libmfx/include/msdk/include/mfxdefs.h
	libmfx/include/msdk/include/mfxjpeg.h
	libmfx/include/msdk/include/mfxmvc.h
	libmfx/include/msdk/include/mfxplugin.h
	libmfx/include/msdk/include/mfxplugin++.h
	libmfx/include/msdk/include/mfxsession.h
	libmfx/include/msdk/include/mfxstructures.h
	libmfx/include/msdk/include/mfxvideo.h
	libmfx/include/msdk/include/mfxvideo++.h
	libmfx/include/msdk/include/mfxvstructures.h
	libmfx/include/mfx_critical_section.h
	libmfx/include/mfx_dispatcher.h
	libmfx/include/mfx_dispatcher_defs.h
	libmfx/include/mfx_dispatcher_log.h
	libmfx/include/mfx_dxva2_device.h
	libmfx/include/mfx_exposed_functions_list.h
	libmfx/include/mfx_library_iterator.h
	libmfx/include/mfx_load_dll.h
	libmfx/include/mfx_load_plugin.h
	libmfx/include/mfx_plugin_hive.h
	libmfx/include/mfx_vector.h
	libmfx/include/mfx_win_reg_key.h
	libmfx/include/mfxaudio_exposed_functions_list.h
	)

set(obs-qsv11_SOURCES
	src/base_allocator.cpp
	src/general_allocator.cpp
	src/sysmem_allocator.cpp
	src/d3d_allocator.cpp
	src/d3d_device.cpp
	src/d3d11_allocator.cpp
	src/d3d11_device.cpp
	src/common_utils.cpp
	src/common_utils_windows.cpp
	src/QSV_Encoder.cpp
	src/QSV_Encoder_Internal.cpp
	src/obs-qsv11.c
	src/obs-qsv11-plugin-main.c)

set(obs-qsv11_HEADERS
	include/linux_defs.h
	include/windows_defs.h
	include/base_allocator.h
	include/general_allocator.h
	include/hw_device.h
	include/sysmem_allocator.h
	include/d3d_allocator.h
	include/d3d_device.h
	include/d3d11_allocator.h
	include/d3d11_device.h
	include/common_utils.h
	include/QSV_Encoder.h
	include/QSV_Encoder_Internal.h)

add_library(obs-qsv11 MODULE
	${obs-qsv11_SOURCES}
	${obs-qsv11_HEADERS}
	${obs-qsv11_libmfx_SOURCES}
	${obs-qsv11_libmfx_HEADERS}
	)

target_link_libraries(obs-qsv11
	libobs
	d3d9
	d3d11
	dxva2
	dxgi
	)

target_compile_definitions(obs-qsv11 PRIVATE DX11_D3D)

source_group("obs-qsv11\\Source Files" FILES ${obs-qsv11_SOURCES})
source_group("obs-qsv11\\Header Files" FILES ${obs-qsv11_HEADERS})
source_group("libmfx\\Source Files" FILES ${obs-qsv11_libmfx_SOURCES})
source_group("libmfx\\Header Files" FILES ${obs-qsv11_libmfx_HEADERS})

install_obs_plugin_with_data(obs-qsv11 data)
