find_package(X11 REQUIRED)
find_package(x11-xcb REQUIRED)
find_package(
  xcb
  OPTIONAL_COMPONENTS xcb-xinput
  QUIET)
find_package(gio)
find_package(Sysinfo REQUIRED)

target_link_libraries(libobs PRIVATE X11::x11-xcb xcb::xcb Sysinfo::Sysinfo)

if(TARGET xcb::xcb-xinput)
  target_link_libraries(libobs PRIVATE xcb::xcb-xinput)
endif()

target_compile_definitions(libobs PRIVATE $<$<OR:$<C_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:GNU>>:ENABLE_DARRAY_TYPE_TEST>)

target_sources(
  libobs
  PRIVATE obs-nix.c
          obs-nix-platform.c
          obs-nix-platform.h
          obs-nix-x11.c
          util/pipe-posix.c
          util/platform-nix.c
          util/threading-posix.c
          util/threading-posix.h)

if(ENABLE_PULSEAUDIO)
  find_package(PulseAudio REQUIRED)

  target_sources(
    libobs
    PRIVATE audio-monitoring/pulse/pulseaudio-enum-devices.c audio-monitoring/pulse/pulseaudio-monitoring-available.c
            audio-monitoring/pulse/pulseaudio-output.c audio-monitoring/pulse/pulseaudio-wrapper.c
            audio-monitoring/pulse/pulseaudio-wrapper.h)

  target_link_libraries(libobs PRIVATE PulseAudio::PulseAudio)

  set_property(GLOBAL APPEND PROPERTY OBS_FEATURES_ENABLED "PulseAudio audio monitoring (Linux)")
else()
  target_sources(libobs PRIVATE audio-monitoring/null/null-audio-monitoring.c)
  set_property(GLOBAL APPEND PROPERTY OBS_FEATURES_DISABLED "PulseAudio audio monitoring (Linux)")
endif()

if(TARGET gio::gio)
  target_link_libraries(libobs PRIVATE gio::gio)

  target_sources(libobs PRIVATE util/platform-nix-dbus.c util/platform-nix-portal.c)
endif()

if(ENABLE_WAYLAND)
  find_package(
    Wayland
    COMPONENTS Client
    REQUIRED)
  find_package(xkbcommon REQUIRED)

  target_link_libraries(libobs PRIVATE Wayland::Client xkbcommon::xkbcommon)
  target_sources(libobs PRIVATE obs-nix-wayland.c)
  set_property(GLOBAL APPEND PROPERTY OBS_FEATURES_ENABLED "Wayland compositor support (Linux)")
else()
  set_property(GLOBAL APPEND PROPERTY OBS_FEATURES_DISABLED "Wayland compositor support (Linux)")
endif()

set_target_properties(libobs PROPERTIES OUTPUT_NAME obs)
