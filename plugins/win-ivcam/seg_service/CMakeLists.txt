project(seg_service)

include(IDLFileHelper)

set(seg_service_IDLS
	seg_service.idl
	)

add_idl_files_with_tlb(seg_service_GENERATED_FILES
	${seg_service_IDLS})

include_directories(
	${RSSDK_INCLUDE_DIRS}	
	${CMAKE_CURRENT_BINARY_DIR}
	)

set(seg_service_HEADERS
	resource.h
	SegProc.h
	stdafx.h
	targetver.h
	xdlldata.h
	)

set(seg_service_SOURCES
	SegProc.rgs
	seg_service.rgs
	seg_service.cpp
	SegProc.cpp
	stdafx.cpp
	seg_service.rc
	${seg_service_GENERATED_FILES}
	)

if(MSVC)
	add_compile_options($<IF:$<CONFIG:Debug>,/MTd,/MT>)
endif()

add_executable(seg_service WIN32
	${seg_service_SOURCES}
	${seg_service_HEADERS})

target_link_libraries(seg_service
	)

install_obs_datatarget(seg_service "obs-plugins/win-ivcam")
