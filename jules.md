# Agent Handoff Document: Dual Output Feature for OBS Fork (Enhanced)

## Feature: Dual Output Mode

This document outlines the current status, work completed, and remaining tasks for implementing the Dual Output feature in this OBS fork. The goal is to allow users to have two independent outputs from OBS: a primary "Horizontal" output (traditional OBS output) and a secondary "Vertical" output (e.g., for mobile platforms), each with its own scene selection and potentially distinct output settings.

## Current Status (Memory Checkpoint - October 2023 - Third Pass)

Significant foundational C++ logic for managing the dual output state, configuration persistence for several key settings, and scene switching has been implemented. The UI logic within `OBSBasicSettings.cpp` has been expanded to handle loading and saving many vertical-specific settings, **assuming the corresponding UI elements exist in the `.ui` files.** The `BasicOutputHandler` hierarchy and `OBSBasicStatusBar` have been updated to support vertical stream status.

**The feature is NOT YET COMPLETE OR FULLY TESTABLE due to:**
1.  **Missing UI Elements:** Crucial UI widgets and tabs for vertical output configuration are not yet present in `OBSBasic.ui` and `OBSBasicSettings.ui`. The C++ code has been written to interface with these assumed elements.
2.  **Incomplete Integration:** Full end-to-end testing of all settings paths and status displays is blocked by the missing UI.
3.  **Libobs Verification:** Deep verification of `libobs`'s capability to handle a truly independent second video mix/pipeline without conflicts or performance degradation under all conditions is still needed.

### Work Completed (Details):

1.  **Dual Output Activation & Core State:** (Covered in previous AGENTS.MD version - state saved in `global.ini`, toggling triggers video/output resets).

2.  **Configuration Persistence (C++ Logic):** (Covered in previous AGENTS.MD version)
    *   Vertical Video Settings (`vertical_ovi`): `V_` prefixed keys in `basic.ini`.
    *   Vertical Scene Selection: `CurrentVSceneName` in `user.ini`.
    *   Output Settings (Simple & Advanced for Vertical Stream/Recording): C++ logic in `OBSBasicSettings.cpp` for `_V_Stream` and `_V_Rec` suffixed keys. **Advanced Vertical Stream Service UI elements are assumed and need to be added to `.ui` file.**

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
        *   New slots implemented (with logging and TODOs for UI updates, as UI elements are pending).
        *   `UpdateStatusBar()` modified to call `UpdateVerticalStreamTime()`.
        *   `Activate()` and `Deactivate()` modified with TODOs for managing vertical stream UI elements.

### Affected Files (Re-confirmed):
(Same list as provided by user in previous turn)
*   `frontend/OBSApp.cpp`, `frontend/OBSApp.hpp`
*   `frontend/forms/OBSBasic.ui`, `frontend/forms/OBSBasicSettings.ui` **(NEEDS MANUAL EDITING)**
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
*   `frontend/forms/StatusBarWidget.ui` **(NEEDS MANUAL EDITING for vertical status)**

## Critical TODOs & Missing Pieces (Prioritized for Handoff):

1.  **MANUAL UI CREATION in `.ui` files (Highest Priority Blocker):**
    *   **`frontend/forms/OBSBasic.ui`:** (Details in previous AGENTS.MD version - dual preview, active pane indicator).
    *   **`frontend/forms/OBSBasicSettings.ui`:** (Details in previous AGENTS.MD version - All vertical tabs and widgets for Video, Simple Output, Advanced Output).
    *   **`frontend/forms/StatusBarWidget.ui` (or integrated into `OBSBasic.ui`):**
        *   **TODO:** Add `QLabel` widgets for vertical stream: time (`vStreamTimeLabel`), status icon (`vStreamStatusIcon`), bitrate (`vKbpsLabel_v`), dropped frames (`vDroppedFramesLabel_v`). (Object names used in `OBSBasicStatusBar.cpp` are placeholders like `statusWidget->ui->vStreamTime`).

2.  **Vertical Preview Rendering (`OBSBasic::RenderVerticalMain` / `addVerticalDisplay`):** (Details in previous AGENTS.MD version - needs to render the vertical mix/scene).
    *   **TODO:** Ensure `obs_display_set_source(ui->mainPreview_v->GetDisplay(), App()->GetCurrentVerticalScene());` or equivalent logic correctly targets the vertical pipeline's output for `ui->mainPreview_v`.

3.  **Complete Status Indication UI in `OBSBasicStatusBar.cpp`:**
    *   **TODO:** Uncomment and adapt UI update lines in `VerticalStreamStarted`, `VerticalStreamStopped`, `UpdateVerticalStreamTime`, etc., once UI elements from `StatusBarWidget.ui` are created.
    *   **TODO:** Implement `UpdateVerticalBandwidth()` and `UpdateVerticalDroppedFrames()` if these stats are desired for the vertical stream and can be obtained from `verticalStreamOutput_`.
    *   **TODO:** Implement vertical stream reconnect signal handling and UI updates (callbacks `OBSOutputVerticalReconnectCallback`, `OBSOutputVerticalReconnectSuccessCallback` need to be defined and connected).
    *   **TODO:** Implement UI updates for vertical stream delay in `VerticalStreamDelayStarting` and `VerticalStreamStopping`.

4.  **Advanced Vertical Stream Service UI (`OBSBasicSettings.cpp`):**
    *   **TODO:** After UI elements like `ui->advService_v_stream`, `ui->advServer_v_stream`, etc., are added to `OBSBasicSettings.ui`, fully connect them in `OBSBasicSettings::LoadAdvOutputStreamingSettings_V` and `SaveAdvOutputStreamingSettings_V`. Ensure all `WidgetChanged` calls are correct.
    *   **TODO:** Implement any missing helper functions for vertical service if they differ significantly from horizontal (e.g., `UpdateVStreamServiceRecommendations`, `UpdateVStreamBandwidthTestEnable`).

5.  **Encoder Settings Application in `OBSApp::SetupOutputs()`:** (Details in previous AGENTS.MD version - ensure all simple mode encoder settings are applied).

6.  **Audio for Vertical Output:** (Details in previous AGENTS.MD version - decide and implement strategy).

7.  **Dual Recording:** (Future feature, details in previous AGENTS.MD version).

8.  **Thorough Testing & `libobs` Verification:** (Details in previous AGENTS.MD version - critical once UI is present).

This `jules.md` should serve as a good starting point for the next agent or human developer to pick up this feature. The highest priority is getting the UI elements created in the `.ui` files so the existing C++ logic can be fully tested and iterated upon.
