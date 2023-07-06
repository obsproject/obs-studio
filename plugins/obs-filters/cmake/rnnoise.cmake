option(ENABLE_RNNOISE "Enable building with RNNoise noise supression filter" ON)

if(ENABLE_RNNOISE)
  if(OS_WINDOWS OR OS_MACOS)
    find_package(Librnnoise REQUIRED)

    target_sources(obs-filters PRIVATE noise-suppress-filter.c)
  elseif(OS_LINUX OR OS_FREEBSD)
    add_library(
      obs-rnnoise
      INTERFACE
      rnnoise/src/arch.h
      rnnoise/src/celt_lpc.c
      rnnoise/src/celt_lpc.h
      rnnoise/src/common.h
      rnnoise/src/denoise.c
      rnnoise/src/kiss_fft.c
      rnnoise/src/kiss_fft.h
      rnnoise/src/opus_types.h
      rnnoise/src/pitch.c
      rnnoise/src/pitch.h
      rnnoise/src/rnn_data.c
      rnnoise/src/rnn_data.h
      rnnoise/src/rnn_reader.c
      rnnoise/src/rnn.c
      rnnoise/src/rnn.h
      rnnoise/src/tansig_table.h
      rnnoise/src/_kiss_fft_guts.h
      rnnoise/include/rnnoise.h)

    add_library(Librnnoise::Librnnoise ALIAS obs-rnnoise)

    target_include_directories(obs-rnnoise INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/rnnoise/include")

    target_compile_definitions(obs-rnnoise INTERFACE COMPILE_OPUS)

    set_target_properties(obs-rnnoise PROPERTIES FOLDER plugins/obs-filters/rnnoise)
  endif()

  target_compile_definitions(obs-filters PRIVATE LIBRNNOISE_ENABLED)

  target_link_libraries(obs-filters PRIVATE Librnnoise::Librnnoise)
  target_enable_feature(obs-filters "RNNoise noise suppression" LIBRRNOISE_ENABLED HAS_NOISEREDUCTION)
else()
  target_disable_feature(obs-filters "RNNoise noise suppression")
endif()
