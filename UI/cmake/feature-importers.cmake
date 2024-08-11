target_sources(
  obs-studio
  PRIVATE # cmake-format: sortable
          importers/classic.cpp importers/importers.cpp importers/importers.hpp importers/sl.cpp importers/studio.cpp
          importers/xsplit.cpp)
