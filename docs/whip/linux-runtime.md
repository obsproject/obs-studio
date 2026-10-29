# Running the Linux WHIP archives

The x86_64 archives target **Ubuntu 26.04**. They contain OBS and its plugins,
but depend on system libraries. They are not an AppImage or a package for older
Ubuntu releases. Use the archive's `whip-build.json` to identify its exact sources.

The following runtime packages were used for the Ubuntu 26.04 playback checks:

```sh
sudo apt-get install --no-install-recommends \
  ffmpeg libqt6widgets6 libqt6svg6 libqt6network6 libqt6xml6 \
  libqt6openglwidgets6 qt6-qpa-plugins libqt6waylandclient6 \
  libpipewire-0.3-0 libpulse0 libjansson4 libcurl4t64 \
  libfdk-aac2 libspeexdsp1 libv4l-0 libvlc5 libpci3 \
  libsrt1.5-openssl libmbedtls21 libmbedx509-7 \
  libx11-xcb1 libxcb-cursor0 libxcb-composite0 libxcb-xinerama0 \
  libxcb-randr0 libxcb-shm0 libxcb-xfixes0 libxss1 \
  libxcomposite1 libxdamage1 libnss3 libnspr4 libasound2t64 \
  libatk1.0-0t64 libatk-bridge2.0-0t64 \
  libgl1-mesa-dri libegl1 libglx-mesa0 libgbm1 libgtk-3-0t64 fonts-liberation
```

Extract the archive, enter its directory, and run `./bin/obs --portable` from a
normal desktop session. Keep the `bin`, `lib`, and `share` directories together.
Do not run normal desktop OBS as root. The validation environment used a private
Ubuntu filesystem, Xvfb and software OpenGL inside WSL2; the root account was used
there to create an isolated network namespace for packet-loss injection.

Two initially missing dependencies were found during runtime validation:

- Without `libsrt1.5-openssl`, the FFmpeg plugin could not load, so Media Source
  was unavailable. Installing FFmpeg alone did not supply this OpenSSL variant.
- Without `libmbedtls21` / `libmbedx509-7`, the OBS outputs plugin could not load.
  OBS 33 then refused to finish startup because a required core module was absent.

Check the OBS log for unloaded modules if a source or encoder is missing.
Linux WHIP itself still uses the patched **OpenSSL** libdatachannel build; the
Mbed TLS packages above are dependencies of other OBS outputs.

Live Linux validation used x264 and a generated Media Source with audio. It did
not validate Linux NVIDIA/AMD/VAAPI/QSV drivers, desktop capture, physical audio
devices, or Browser Source rendering. Those depend on the host's graphics and
desktop setup. Windows hardware-encoder results do not certify Linux drivers.
