find_package(LibUUID REQUIRED)
find_package(X11 REQUIRED)
find_package(X11-xcb REQUIRED)
# cmake-format: off
find_package(Xcb REQUIRED xcb OPTIONAL_COMPONENTS xcb-xinput)
# cmake-format: on
find_package(Gio)
find_package(Sysinfo REQUIRED)

set(CMAKE_M_LIBS "")
include(CheckCSourceCompiles)
set(LIBM_TEST_SOURCE "#include<math.h>\nfloat f; int main(){sqrt(f);return 0;}")
check_c_source_compiles("${LIBM_TEST_SOURCE}" HAVE_MATH_IN_STD_LIB)

set(CMAKE_REQUIRED_INCLUDES "/usr/local/include")
set(UUID_TEST_SOURCE "#include<uuid/uuid.h>\nint main(){return 0;}")
check_c_source_compiles("${UUID_TEST_SOURCE}" HAVE_UUID_HEADER)

if(NOT HAVE_UUID_HEADER)
  message(FATAL_ERROR "Required system header <uuid/uuid.h> not found.")
endif()

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

target_compile_definitions(
  libobs PRIVATE OBS_INSTALL_PREFIX="${OBS_INSTALL_PREFIX}" $<$<COMPILE_LANG_AND_ID:C,GNU>:ENABLE_DARRAY_TYPE_TEST>
                 $<$<COMPILE_LANG_AND_ID:CXX,GNU>:ENABLE_DARRAY_TYPE_TEST>)

target_link_libraries(
  libobs
  PRIVATE X11::x11-xcb
          xcb::xcb
          LibUUID::LibUUID
          Sysinfo::Sysinfo
          ${CMAKE_DL_LIBS}
          $<$<NOT:$<BOOL:${HAVE_MATH_IN_STD_LIB}>>:m>
          $<$<TARGET_EXISTS:xcb::xcb-input>:xcb::xcb-input>)

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
  target_enable_feature(libobs "PulseAudio audio monitoring (FreeBSD)")
else()
  target_sources(libobs PRIVATE audio-monitoring/null/null-audio-monitoring.c)
  target_disable_feature(libobs "PulseAudio audio monitoring (FreeBSD)")
endif()

if(TARGET gio::gio)
  target_sources(libobs PRIVATE util/platform-nix-dbus.c util/platform-nix-portal.c)
  target_link_libraries(libobs PRIVATE gio::gio)
endif()

if(ENABLE_WAYLAND)
  find_package(Wayland REQUIRED Client)
  find_package(Xkbcommon REQUIRED)

  target_sources(libobs PRIVATE obs-nix-wayland.c)
  target_link_libraries(libobs PRIVATE Wayland::Client xkbcommon::xkbcommon)
  target_enable_feature(libobs "Wayland compositor support (FreeBSD)")
else()
  target_disable_feature(libobs "Wayland compositor support (FreebSD)")
endif()

set_target_properties(libobs PROPERTIES OUTPUT_NAME obs)
