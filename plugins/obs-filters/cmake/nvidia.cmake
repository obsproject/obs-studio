if(ENABLE_NVAFX)
  target_sources(obs-filters PRIVATE noise-suppress-filter.c)
  target_compile_definitions(obs-filters PRIVATE LIBNVAFX_ENABLED HAS_NOISEREDUCTION)
endif()
