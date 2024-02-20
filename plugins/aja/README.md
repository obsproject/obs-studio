# AJA I/O Device Capture and Output Plugin for OBS

A capture and output plugin for OBS for use with AJA NTV2 I/O devices. The plugin communicates with the AJA device driver via the open-source AJA NTV2 SDK (MIT license). See instructions below for installing the AJA device driver.

---

## AJA Device Driver Installation

Usage of this plugin with an AJA I/O device requires installation of the AJA Device Driver.

Follow one of the methods below to install the AJA device driver for your platform.

### Method 1 - AJA Software Installer (Windows/macOS/Ubuntu Linux)

Pre-built driver packages for Windows, macOS and Ubuntu Linux are provided as a component of the AJA Software Installer. Navigate to the AJA Support page and download the latest AJA Software Installer for your platform:

https://www.aja.com/support

### Method 2 - Build From Sources (Linux)

The Linux version of the AJA device driver can be built from the MIT-licensed AJA NTV2 SDK sources found here:

https://github.com/aja-video/ntv2

- Clone the ntv2 repository.
- `cd` into the `ntv2/ajadriver/linux` directory
- Run `make`
- `cd` into the `ntv2/bin` directory and run `sudo sh load_ajantv2` to install the AJA kernel module.

NOTE: To uniunstall the Linux kernel module, `cd` into the `ntv2/bin` directory and run `sudo sh unload_ajantv2`.

---

## How to use the plugin
- Capture plugin instances can be added from `Sources -> Add -> AJA I/O Device Capture`.

- The Output plugin is accessible from `Tools -> AJA I/O Device Output`.

## Updating the plugin AJA NTV2 SDK code

The AJA NTV2 SDK is open-source and licensed under the MIT license. A copy of the SDK is included in the source tree of the OBS AJA plugin directory.

To update the plugin's working copy of the AJA NTV2 SDK:

1. Clone the AJA NTV2 SDK from https://github.com/aja-video/ntv2
1. Place the contents of the ntv2 directory into `plugins/aja/sdk` within the cloned OBS repository.

You should end up with a directory structure like:

```
plugins/aja/sdk
    /ajadriver
    /ajalibraries
    /cmake
    /CMakeLists.txt
    /LICENSE
    /README.md
```

## Known Issues/TODOS
1. Fix AV sync in the AJA I/O Output plugin with certain OBS/device frame rate combinations.
1. Improve frame tearing in the AJA I/O Device Capture plugin with certain OBS/device frame rate combinations.
1. Capture plugins not always auto-detecting signal properly after disconnecting and reconnecting the physical SDI/HDMI cable from the device.
1. Improve handling of certain invalid video/pixel/SDI 4K transport setting combinations.
1. Optimize framebuffer memory usage on the device.
1. Add support for Analog I/O capture and output on supported devices.
1. Luminance is too bright/washed out in output plugin video.
1. Certain combinations of video format with RGB pixel format needs additional testing.
1. 4K video formats and SDI 4K transports need more testing in general.
1. 8K video not yet implemented.
1. Add support for selecting an alternate audio input source on supported devices (i.e. Microphone Input on AJA io4K+, analog audio, AES audio, etc). Capture and output currently default to using up to 8 channels of embedded SDI/HDMI audio.
1. All translation strings are placeholders and default to English. Translation strings needed for additional language support.

---

_Development Credits_
- ddrboxman
- paulh-aja
