add_executable(test_media_playback_timecode)

target_sources(
  test_media_playback_timecode
  PRIVATE test_timecode.c ../../shared/media-playback/media-playback/timecode.c
)

target_include_directories(test_media_playback_timecode PRIVATE ../../shared/media-playback/media-playback)

add_test(NAME test_media_playback_timecode COMMAND test_media_playback_timecode)
