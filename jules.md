# Agent Handoff Document: Dual Output Feature for OBS Fork (Enhanced)

## Feature: Dual Output Mode

This document outlines the current status, work completed, and remaining tasks for implementing the Dual Output feature in this OBS fork. The goal is to allow users to have two independent outputs from OBS: a primary "Horizontal" output (traditional OBS output) and a secondary "Vertical" output (e.g., for mobile platforms), each with its own scene selection and potentially distinct output settings.

## Current Status (After UI Implementation Pass - [Current Date])

Significant foundational C++ logic for managing the dual output state, configuration persistence for several key settings, and scene switching was previously implemented. This pass focused on creating the missing UI elements in `.ui` files and wiring them up in the C++ backend.

**UI elements for vertical output configuration in `OBSBasicSettings.ui` and `StatusBarWidget.ui` have been created and integrated with their respective C++ handlers (`OBSBasicSettings.cpp`, `OBSBasicStatusBar.cpp`). Vertical preview rendering logic in `OBSBasic.cpp` has been verified and corrected.**

**The feature is now closer to being testable, but is NOT YET COMPLETE due to:**
1.  **Incomplete Core Logic:**
    *   **Audio for Vertical Output:** Strategy needs to be defined and implemented.
    *   **Encoder Settings Application:** Full application of all simple mode encoder settings in `OBSApp::SetupOutputs()` needs deeper verification.
2.  **Libobs Verification:** Deep verification of `libobs`'s capability to handle a truly independent second video mix/pipeline without conflicts or performance degradation under all conditions is still needed.
3.  **Thorough End-to-End Testing:** Comprehensive testing of all settings paths, status displays, and output functionalities is required.

### Work Completed (Details):

1.  **Dual Output Activation & Core State:** (Covered in previous AGENTS.MD version - state saved in `global.ini`, toggling triggers video/output resets).

2.  **Configuration Persistence (C++ Logic):** (Covered in previous AGENTS.MD version)
    *   Vertical Video Settings (`vertical_ovi`): `V_` prefixed keys in `basic.ini`.
    *   Vertical Scene Selection: `CurrentVSceneName` in `user.ini`.
    *   Output Settings (Simple & Advanced for Vertical Stream/Recording): C++ logic in `OBSBasicSettings.cpp` for `_V_Stream` and `_V_Rec` suffixed keys. UI elements for these in `OBSBasicSettings.ui` are now implemented.
    *   **Advanced Vertical Stream Service UI elements** in `OBSBasicSettings.ui` have been created and connected in `OBSBasicSettings.cpp`.

3.  **Scene Management & Switching:** (Covered in previous AGENTS.MD version)
    *   `OBSApp` tracks `current_horizontal_scene` and `current_vertical_scene`.
    *   `OBSApp` emits `horizontalSceneChanged(obs_source_t*)` and `verticalSceneChanged(obs_source_t*)`. `OBSBasic` connects to these via `HandleHorizontalSceneChanged` and `HandleVerticalSceneChanged`.
    *   `OBSBasic::SetCurrentScene()` has been significantly reworked to:
        *   Identify the target scene based on the `activePreviewPane` (Horizontal or Vertical).
        *   Call `App()->SetCurrentHorizontalScene()` or `App()->SetCurrentVerticalScene()`.
        *   Update the core libobs preview/program scene only when the horizontal pane is active or dual output is off.
        *   Manage audio monitoring and UI list selection based on the active pane.

4.  **Video Pipeline (Conceptual & Frontend Orchestration):** (Covered in previous AGENTS.MD version)
    *   `OBSApp::SetDualOutputActive()` triggers `OBSBasic::ResetVideo()` and `OBSBasic::ResetOutputs()`.
    *   `OBSApp::SetupOutputs()` configures `verticalStreamOutput` with overridden settings and source.

5.  **Status Indication for Vertical Stream (C++ Backend):**
    *   **`BasicOutputHandler.hpp`:**
        *   Added `verticalStreamOutput` (weak ref placeholder, actual is in OBSApp), `verticalStreamingActive_`, `verticalStreamTotalSeconds_`, etc.
        *   Added signals: `startVerticalStreaming`, `stopVerticalStreaming`, `verticalStreamDelayStarting`, `verticalStreamStopping`.
        *   Added pure virtual functions: `StartVerticalStreaming`, `StopVerticalStreaming`, `VerticalStreamingActive`.
    *   **`SimpleOutput.cpp` & `AdvancedOutput.cpp`:**
        *   Implemented `StartVerticalStreaming`, `StopVerticalStreaming`, `VerticalStreamingActive`.
        *   Main `StartStreaming` and `StopStreaming` in both classes now attempt to start/stop `verticalStreamOutput` if dual output is active and the output is configured.
        *   `StreamingActive()` in both classes now considers the state of `verticalStreamOutput`.
        *   `SimpleOutput` constructor initializes `videoStreaming_v` encoder for simple vertical stream. `Update()` and `SetupOutputs()` in `SimpleOutput` configure this encoder.
        *   `AdvancedOutput::SetupOutputs()` now connects signals for `this->verticalStreamOutput` to new global callbacks.
    *   **`BasicOutputHandler.cpp` (New Global Callbacks):**
        *   Implemented `OBSStartVerticalStreaming`, `OBSStopVerticalStreaming`, `OBSVerticalStreamDelayStarting`, `OBSVerticalStreamStopping`. These update internal flags in `BasicOutputHandler` and emit the new signals.
    *   **`OBSBasicStatusBar.hpp`:**
        *   Added new member variables for vertical stream status (e.g., `verticalStreamOutput_`, `verticalStreamingActive_`, `verticalStreamTotalSeconds_`).
        *   Added new public slots: `VerticalStreamStarted(obs_output_t*)`, `VerticalStreamStopped(int, QString)`, `UpdateVerticalStreamTime()`, `VerticalStreamDelayStarting(int)`, `VerticalStreamStopping()`.
    *   **`OBSBasicStatusBar.cpp`:**
        *   Constructor updated to connect to the new vertical streaming signals from `BasicOutputHandler`.
        *   New slots implemented and UI updates for vertical stream status are now functional.
        *   `UpdateStatusBar()` modified to call `UpdateVerticalStreamTime()`, `UpdateVerticalBandwidth()`, and `UpdateVerticalDroppedFrames()` for the vertical stream.
        *   `Activate()` and `Deactivate()` now correctly manage vertical stream UI elements.
        *   Reconnect logic for vertical stream has been added.

6.  **UI Creation and Integration (This Pass):**
    *   **`frontend/forms/OBSBasic.ui`:** Vertical preview pane (`verticalPreviewContainer`, `mainPreview_v`) confirmed as present.
    *   **`frontend/forms/OBSBasicSettings.ui`:**
        *   Vertical tabs and widgets for Video settings (`verticalVideoTab`) created and integrated.
        *   Vertical tabs and widgets for Simple Output settings (`simpleStreamingVerticalTab`, `simpleRecordingVerticalTab`) created and integrated.
        *   Vertical tabs and widgets for Advanced Output settings (`advOutputStreamTab_v`, `advOutputRecordTab_v`), including service configuration for vertical streaming, created and integrated.
    *   **`frontend/forms/StatusBarWidget.ui`:** `QLabel` widgets for vertical stream status (time, status icon, bitrate, dropped frames) added.
    *   **`frontend/settings/OBSBasicSettings.cpp`:** Logic to load/save and manage UI interactions for all vertical-specific settings (Video, Simple Output, Advanced Output including service config) implemented.
    *   **`frontend/widgets/OBSBasicStatusBar.cpp`:** Full UI logic for displaying and updating vertical stream status, including bandwidth, dropped frames, and reconnects, implemented.
    *   **`frontend/widgets/OBSBasic.cpp`:** Logic for setting the source of the vertical preview display (`ui->mainPreview_v`) on creation and on vertical scene changes implemented.
7.  **Encoder Settings Refinement (This Pass):**
    *   **`OBSApp::SetupOutputs()`:** Updated to apply custom encoder settings strings (`StreamCustom` and `StreamCustom_V_Stream`) for simple mode streaming for both horizontal and vertical outputs.
8.  **Basic Audio for Vertical Output (This Pass):**
    *   **`OBSApp::SetupOutputs()`:** Ensured `obs_output_set_audio_source` is correctly called for `vertical_stream_output` to use the main audio mix.
    *   Applied audio bitrate settings (from Simple Mode `ABitrate_V_Stream` or Advanced Mode based on selected track) to the `vertical_stream_service` configuration.

### Affected Files (Re-confirmed & Updated):
(Same list as provided by user in previous turn, manual editing notes updated)
*   `frontend/OBSApp.cpp`, `frontend/OBSApp.hpp`
*   `frontend/forms/OBSBasic.ui` *(No further manual editing expected for this feature's core UI)*
*   `frontend/forms/OBSBasicSettings.ui` *(No further manual editing expected for this feature's core UI)*
*   `frontend/settings/OBSBasicSettings.cpp`, `frontend/settings/OBSBasicSettings.hpp`
*   `frontend/widgets/OBSBasic.cpp`, `frontend/widgets/OBSBasic.hpp`
*   `frontend/widgets/OBSBasic_Preview.cpp`
*   `frontend/widgets/OBSBasic_SceneCollections.cpp`
*   `frontend/widgets/OBSBasic_Scenes.cpp`
*   `frontend/widgets/OBSBasic_Transitions.cpp`
*   `frontend/utility/BasicOutputHandler.hpp`, `frontend/utility/BasicOutputHandler.cpp`
*   `frontend/utility/SimpleOutput.hpp`, `frontend/utility/SimpleOutput.cpp`
*   `frontend/utility/AdvancedOutput.hpp`, `frontend/utility/AdvancedOutput.cpp`
*   `frontend/widgets/OBSBasicStatusBar.hpp`, `frontend/widgets/OBSBasicStatusBar.cpp`
*   `frontend/forms/StatusBarWidget.ui` *(No further manual editing expected for this feature's core UI)*

## Critical TODOs & Missing Pieces (Prioritized for Handoff):

1.  **~~MANUAL UI CREATION in `.ui` files~~ (DONE):**
    *   **`frontend/forms/OBSBasic.ui`:** ~~(Details in previous AGENTS.MD version - dual preview, active pane indicator).~~ *(DONE - UI structure in place)*
    *   **`frontend/forms/OBSBasicSettings.ui`:** ~~(Details in previous AGENTS.MD version - All vertical tabs and widgets for Video, Simple Output, Advanced Output).~~ *(DONE - All specified vertical tabs and widgets for Video, Simple Output, and Advanced Output, including vertical stream service config, have been added).*
    *   **`frontend/forms/StatusBarWidget.ui` (or integrated into `OBSBasic.ui`):**
        *   **~~TODO:~~** ~~Add `QLabel` widgets for vertical stream: time (`vStreamTimeLabel`), status icon (`vStreamStatusIcon`), bitrate (`vKbpsLabel_v`), dropped frames (`vDroppedFramesLabel_v`). (Object names used in `OBSBasicStatusBar.cpp` are placeholders like `statusWidget->ui->vStreamTime`).~~ *(DONE - Labels `vStreamTime`, `vStatusIcon`, `vKbps`, `vDroppedFrames`, `vStreamIcon` added to `verticalStreamFrame`)*

2.  **~~Vertical Preview Rendering (`OBSBasic::RenderVerticalMain` / `addVerticalDisplay`)~~ (DONE):**
    *   **~~TODO:~~** ~~Ensure `obs_display_set_source(ui->mainPreview_v->GetDisplay(), App()->GetCurrentVerticalScene());` or equivalent logic correctly targets the vertical pipeline's output for `ui->mainPreview_v`.~~ *(DONE - Source for `ui->mainPreview_v` is set on creation and on vertical scene changes).*

3.  **~~Complete Status Indication UI in `OBSBasicStatusBar.cpp`~~ (DONE):**
    *   **~~TODO:~~** ~~Uncomment and adapt UI update lines in `VerticalStreamStarted`, `VerticalStreamStopped`, `UpdateVerticalStreamTime`, etc., once UI elements from `StatusBarWidget.ui` are created.~~ *(DONE)*
    *   **~~TODO:~~** ~~Implement `UpdateVerticalBandwidth()` and `UpdateVerticalDroppedFrames()` if these stats are desired for the vertical stream and can be obtained from `verticalStreamOutput_`.~~ *(DONE)*
    *   **~~TODO:~~** ~~Implement vertical stream reconnect signal handling and UI updates (callbacks `OBSOutputVerticalReconnectCallback`, `OBSOutputVerticalReconnectSuccessCallback` need to be defined and connected).~~ *(DONE)*
    *   **~~TODO:~~** ~~Implement UI updates for vertical stream delay in `VerticalStreamDelayStarting` and `VerticalStreamStopping`.~~ *(DONE - Basic logging in place, full UI for delay might need dedicated label if desired beyond logging).*

4.  **~~Advanced Vertical Stream Service UI (`OBSBasicSettings.cpp`)~~ (DONE):**
    *   **~~TODO:~~** ~~After UI elements like `ui->advService_v_stream`, `ui->advServer_v_stream`, etc., are added to `OBSBasicSettings.ui`, fully connect them in `OBSBasicSettings::LoadAdvOutputStreamingSettings_V` and `SaveAdvOutputStreamingSettings_V`. Ensure all `WidgetChanged` calls are correct.~~ *(DONE)*
    *   **~~TODO:~~** ~~Implement any missing helper functions for vertical service if they differ significantly from horizontal (e.g., `UpdateVStreamServiceRecommendations`, `UpdateVStreamBandwidthTestEnable`).~~ *(DONE - Implemented helpers like `PopulateVStreamServiceList`, `UpdateVStreamServerList`, `UpdateVStreamKeyAuthWidgets`, `UpdateVStreamServicePage`)*.

5.  **Encoder Settings Application in `OBSApp::SetupOutputs()`:** (Details in previous AGENTS.MD version - ensure all simple mode encoder settings are applied). **PARTIALLY DONE:** Custom encoder settings strings for simple mode are now applied. **TODO:** A full review for *all* simple mode settings (preset, tune etc.) and their correct application to both horizontal and vertical outputs is still recommended.

6.  **Audio for Vertical Output:** (Details in previous AGENTS.MD version - decide and implement strategy). **PARTIALLY DONE:** Basic shared audio (using the main mix from `obs_get_audio()`) is implemented for the vertical output. Audio encoder settings (bitrate, type based on service) are applied. **TODO:** Independent audio mixing/routing for the vertical output is a pending feature for future consideration if more granular control is needed.

7.  **Dual Recording:** (Future feature, details in previous AGENTS.MD version). *(No change, still future)*

8.  **Thorough Testing & `libobs` Verification:** (Details in previous AGENTS.MD version - critical once UI is present). **TODO:** Now that the UI and basic wiring are in place, this becomes even more critical. Test all new UI paths, dual output activation/deactivation, scene management, and output starting/stopping. Deep verification of `libobs`'s independent pipeline handling is still pending.

This `jules.md` should serve as a good starting point for the next agent or human developer to pick up this feature. The highest priority is getting the UI elements created in the `.ui` files so the existing C++ logic can be fully tested and iterated upon.
