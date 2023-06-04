cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

option(ENABLE_FFMPEG_MUX_DEBUG "Enable FFmpeg-mux debugging" OFF)

find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil avformat)

add_executable(obs-ffmpeg-mux)
add_executable(OBS::ffmpeg-mux ALIAS obs-ffmpeg-mux)

target_sources(obs-ffmpeg-mux PRIVATE ffmpeg-mux.c ffmpeg-mux.h)

target_link_libraries(obs-ffmpeg-mux PRIVATE OBS::libobs FFmpeg::avcodec FFmpeg::avutil FFmpeg::avformat
                                             $<$<PLATFORM_ID:Windows>:OBS::w32-pthreads>)

target_compile_definitions(obs-ffmpeg-mux PRIVATE $<$<BOOL:${ENABLE_FFMPEG_MUX_DEBUG}>:ENABLE_FFMPEG_MUX_DEBUG>)

set_target_properties_obs(obs-ffmpeg-mux PROPERTIES FOLDER plugins/obs-ffmpeg)
