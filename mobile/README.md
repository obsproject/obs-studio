# OBS Studio Mobile — scaffold

This folder is the first mobile application layer for the `gkurama/obs-studio` fork. It is intentionally separate from the desktop `frontend/`: the desktop Qt Widgets interface and desktop capture plugins remain untouched.

## What is here

- `android/`: a native Android shell using Kotlin and Jetpack Compose.
- `ios/`: a native iOS shell using SwiftUI.
- Both shells follow the OBS workspace shown in the desktop reference: menu bar, large black preview canvas, Scenes and Sources docks on the left, Audio Mixer, Scene Transitions, Controls, and a status bar. The selected-scene highlight uses OBS blue and panel chrome follows the reference's charcoal palette.
- In landscape/tablet width, the docks line up like the PC workspace. In portrait, the same panels stack in the same order so the text and touch targets remain usable.
- Scene creation, selection, deletion (while keeping at least one scene), and reordering work locally.
- Sources can be added from a list of OBS-style source types, selected, shown/hidden, and deleted. These are editable scene descriptors only; choosing a camera or screen source does not yet start native capture.
- Mixer mute controls and gain sliders, transition selection and duration, and the studio-mode UI toggle respond locally.
- Recording, streaming, virtual camera, and engine settings explicitly report that their media-engine integration is not available yet. The audio meters and status figures are visual placeholders, not live measurements.

The preview is still a UI scaffold. Camera, screen capture, recording, encoding, and streaming are **not connected**. Scene/source lists and mixer settings are currently in-memory UI state and do not persist across launches or control `libobs`.

## Why a mobile shell instead of reusing the desktop window

OBS's reusable engine is `libobs`; the current desktop app is built around Qt Widgets and desktop-specific plugins. A phone needs native capture entry points and lifecycle handling. The plan is to retain the existing OBS source tree and gradually connect a small mobile frontend to a curated `libobs` build through a C API/JNI bridge on Android and an Objective-C/Swift bridge on iOS. This scaffold does not claim that the upstream `libobs` target already builds on either mobile OS.

## Platform capture plan

- Android camera: CameraX, with OS camera/microphone consent.
- Android screen: `MediaProjection` in a foreground service. Consent is needed for each capture session, and Android 14+ requires the media-projection foreground-service permission and type.
- iOS camera: AVFoundation capture session.
- iOS screen/broadcast: ReplayKit broadcast sample handling, subject to the iOS extension and background execution rules.
- A shared mobile media graph will own scenes, sources, audio mixing, encoding, and output once the native sample buffers have been adapted to it. Hardware encoding should be preferred; full desktop plugin parity is not a realistic mobile target.

## Build prerequisites

Android: Android Studio, JDK 17+, Android SDK (API 36), and Gradle 9.1+. Open `android/` in Android Studio and run the `app` configuration.

iOS: macOS, Xcode 26+, and an iOS 17+ simulator or device. Open `ios/OBSStudioMobile.xcodeproj` and run the `OBSStudioMobile` scheme.

The current Windows workspace does not have Android SDK/ADB, Java, CMake, or Xcode available, so neither target could be compiled or launched here. The stock desktop CMake project is not an iOS/Android build target. No device-level capture or streaming claim is made by this UI-only first step.

## Milestones

1. **UI shell (this step):** matching dark OBS visual language, responsive phone layout, scene selection.
2. **Native capture:** camera preview and screen capture permission/stream lifecycles, separately per OS.
3. **Media graph:** connect native samples to a mobile-capable compositor/encoder, then save a local recording.
4. **Streaming:** add RTMP/YouTube ingest configuration and reconnect/error handling.
5. **Parity pass:** validate performance, thermals, audio routing, permissions, and accessibility on real devices.

## License

The upstream OBS Studio code is GPL-2.0-or-later. Keep its notices with any redistributed derivative. This scaffold follows the same repository license; the mobile app still needs a complete third-party dependency and license review before distribution.
