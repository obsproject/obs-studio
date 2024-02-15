find_package(X11 REQUIRED)
find_package(x11-xcb REQUIRED)
# cmake-format: off
find_package(xcb REQUIRED xcb OPTIONAL_COMPONENTS xcb-xinput)
# cmake-format: on
find_package(gio)
find_package(Sysinfo REQUIRED)

target_sources(
  libobs
  PRIVATE # cmake-format: sortable
          obs-nix-platform.c
          obs-nix-platform.h
          obs-nix-x11.c
          obs-nix.c
          util/pipe-posix.c
          util/platform-nix.c
          util/threading-posix.c
          util/threading-posix.h)

target_compile_definitions(libobs PRIVATE $<$<COMPILE_LANG_AND_ID:C,GNU>:ENABLE_DARRAY_TYPE_TEST>
                                          $<$<COMPILE_LANG_AND_ID:CXX,GNU>:ENABLE_DARRAY_TYPE_TEST>)

target_link_libraries(libobs PRIVATE X11::x11-xcb xcb::xcb Sysinfo::Sysinfo)

if(TARGET xcb::xcb-xinput)
  target_link_libraries(libobs PRIVATE xcb::xcb-xinput)
endif()

if(ENABLE_PULSEAUDIO)
  find_package(PulseAudio REQUIRED)

  target_sources(
    libobs
    PRIVATE # cmake-format: sortable
            audio-monitoring/pulse/pulseaudio-enum-devices.c
            audio-monitoring/pulse/pulseaudio-monitoring-available.c
            audio-monitoring/pulse/pulseaudio-output.c
            audio-monitoring/pulse/pulseaudio-wrapper.c
            audio-monitoring/pulse/pulseaudio-wrapper.h)

  target_link_libraries(libobs PRIVATE PulseAudio::PulseAudio)
  target_enable_feature(libobs "PulseAudio audio monitoring (Linux)")
else()
  target_sources(libobs PRIVATE audio-monitoring/null/null-audio-monitoring.c)
  target_disable_feature(libobs "PulseAudio audio monitoring (Linux)")
endif()

if(TARGET gio::gio)
  target_sources(libobs PRIVATE util/platform-nix-dbus.c util/platform-nix-portal.c)
  target_link_libraries(libobs PRIVATE gio::gio)
endif()

if(ENABLE_WAYLAND)
  # cmake-format: off
  find_package(Wayland COMPONENTS Client REQUIRED)
  # cmake-format: on
  find_package(xkbcommon REQUIRED)

  target_sources(libobs PRIVATE obs-nix-wayland.c)
  target_link_libraries(libobs PRIVATE Wayland::Client xkbcommon::xkbcommon)
  target_enable_feature(libobs "Wayland compositor support (Linux)")
else()
  target_disable_feature(libobs "Wayland compositor support (Linux)")
endif()

set_target_properties(libobs PROPERTIES OUTPUT_NAME obs)
