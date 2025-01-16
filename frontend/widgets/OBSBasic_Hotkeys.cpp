/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Zachary Lund <admin@computerquip.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "OBSBasic.hpp"

#include <widgets/OBSBasicStats.hpp>

void OBSBasic::InitHotkeys()
{
	ProfileScope("OBSBasic::InitHotkeys");

	struct obs_hotkeys_translations t = {};
	t.insert = Str("Hotkeys.Insert");
	t.del = Str("Hotkeys.Delete");
	t.home = Str("Hotkeys.Home");
	t.end = Str("Hotkeys.End");
	t.page_up = Str("Hotkeys.PageUp");
	t.page_down = Str("Hotkeys.PageDown");
	t.num_lock = Str("Hotkeys.NumLock");
	t.scroll_lock = Str("Hotkeys.ScrollLock");
	t.caps_lock = Str("Hotkeys.CapsLock");
	t.backspace = Str("Hotkeys.Backspace");
	t.tab = Str("Hotkeys.Tab");
	t.print = Str("Hotkeys.Print");
	t.pause = Str("Hotkeys.Pause");
	t.left = Str("Hotkeys.Left");
	t.right = Str("Hotkeys.Right");
	t.up = Str("Hotkeys.Up");
	t.down = Str("Hotkeys.Down");
#ifdef _WIN32
	t.meta = Str("Hotkeys.Windows");
#else
	t.meta = Str("Hotkeys.Super");
#endif
	t.menu = Str("Hotkeys.Menu");
	t.space = Str("Hotkeys.Space");
	t.numpad_num = Str("Hotkeys.NumpadNum");
	t.numpad_multiply = Str("Hotkeys.NumpadMultiply");
	t.numpad_divide = Str("Hotkeys.NumpadDivide");
	t.numpad_plus = Str("Hotkeys.NumpadAdd");
	t.numpad_minus = Str("Hotkeys.NumpadSubtract");
	t.numpad_decimal = Str("Hotkeys.NumpadDecimal");
	t.apple_keypad_num = Str("Hotkeys.AppleKeypadNum");
	t.apple_keypad_multiply = Str("Hotkeys.AppleKeypadMultiply");
	t.apple_keypad_divide = Str("Hotkeys.AppleKeypadDivide");
	t.apple_keypad_plus = Str("Hotkeys.AppleKeypadAdd");
	t.apple_keypad_minus = Str("Hotkeys.AppleKeypadSubtract");
	t.apple_keypad_decimal = Str("Hotkeys.AppleKeypadDecimal");
	t.apple_keypad_equal = Str("Hotkeys.AppleKeypadEqual");
	t.mouse_num = Str("Hotkeys.MouseButton");
	t.escape = Str("Hotkeys.Escape");
	obs_hotkeys_set_translations(&t);

	obs_hotkeys_set_audio_hotkeys_translations(Str("Mute"), Str("Unmute"), Str("Push-to-mute"),
						   Str("Push-to-talk"));

	obs_hotkeys_set_sceneitem_hotkeys_translations(Str("SceneItemShow"), Str("SceneItemHide"));

	obs_hotkey_enable_callback_rerouting(true);
	obs_hotkey_set_callback_routing_func(OBSBasic::HotkeyTriggered, this);
}

void OBSBasic::ProcessHotkey(obs_hotkey_id id, bool pressed)
{
	obs_hotkey_trigger_routed_callback(id, pressed);
}

void OBSBasic::HotkeyTriggered(void *data, obs_hotkey_id id, bool pressed)
{
	OBSBasic &basic = *static_cast<OBSBasic *>(data);
	QMetaObject::invokeMethod(&basic, "ProcessHotkey", Q_ARG(obs_hotkey_id, id), Q_ARG(bool, pressed));
}

void OBSBasic::CreateHotkeys()
{
	ProfileScope("OBSBasic::CreateHotkeys");

	auto LoadHotkeyData = [&](const char *name) -> OBSData {
		const char *info = config_get_string(activeConfiguration, "Hotkeys", name);
		if (!info)
			return {};

		OBSDataAutoRelease data = obs_data_create_from_json(info);
		if (!data)
			return {};

		return data.Get();
	};

	auto LoadHotkey = [&](obs_hotkey_id id, const char *name) {
		OBSDataArrayAutoRelease array = obs_data_get_array(LoadHotkeyData(name), "bindings");

		obs_hotkey_load(id, array);
	};

	auto LoadHotkeyPair = [&](obs_hotkey_pair_id id, const char *name0, const char *name1,
				  const char *oldName = NULL) {
		if (oldName) {
			const auto info = config_get_string(activeConfiguration, "Hotkeys", oldName);
			if (info) {
				config_set_string(activeConfiguration, "Hotkeys", name0, info);
				config_set_string(activeConfiguration, "Hotkeys", name1, info);
				config_remove_value(activeConfiguration, "Hotkeys", oldName);
				activeConfiguration.Save();
			}
		}
		OBSDataArrayAutoRelease array0 = obs_data_get_array(LoadHotkeyData(name0), "bindings");
		OBSDataArrayAutoRelease array1 = obs_data_get_array(LoadHotkeyData(name1), "bindings");

		obs_hotkey_pair_load(id, array0, array1);
	};

#define MAKE_CALLBACK(pred, method, log_action)                            \
	[](void *data, obs_hotkey_pair_id, obs_hotkey_t *, bool pressed) { \
		OBSBasic &basic = *static_cast<OBSBasic *>(data);          \
		if ((pred) && pressed) {                                   \
			blog(LOG_INFO, log_action " due to hotkey");       \
			method();                                          \
			return true;                                       \
		}                                                          \
		return false;                                              \
	}

	streamingHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.StartStreaming", Str("Basic.Main.StartStreaming"), "OBSBasic.StopStreaming",
		Str("Basic.Main.StopStreaming"),
		MAKE_CALLBACK(!basic.outputHandler->StreamingActive() && !basic.streamingStarting, basic.StartStreaming,
			      "Starting stream"),
		MAKE_CALLBACK(basic.outputHandler->StreamingActive() && !basic.streamingStarting, basic.StopStreaming,
			      "Stopping stream"),
		this, this);
	LoadHotkeyPair(streamingHotkeys, "OBSBasic.StartStreaming", "OBSBasic.StopStreaming");

	auto cb = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		OBSBasic &basic = *static_cast<OBSBasic *>(data);
		if (basic.outputHandler->StreamingActive() && pressed) {
			basic.ForceStopStreaming();
		}
	};

	forceStreamingStopHotkey = obs_hotkey_register_frontend("OBSBasic.ForceStopStreaming",
								Str("Basic.Main.ForceStopStreaming"), cb, this);
	LoadHotkey(forceStreamingStopHotkey, "OBSBasic.ForceStopStreaming");

	recordingHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.StartRecording", Str("Basic.Main.StartRecording"), "OBSBasic.StopRecording",
		Str("Basic.Main.StopRecording"),
		MAKE_CALLBACK(!basic.outputHandler->RecordingActive() && !basic.recordingStarted, basic.StartRecording,
			      "Starting recording"),
		MAKE_CALLBACK(basic.outputHandler->RecordingActive() && basic.recordingStarted, basic.StopRecording,
			      "Stopping recording"),
		this, this);
	LoadHotkeyPair(recordingHotkeys, "OBSBasic.StartRecording", "OBSBasic.StopRecording");

	pauseHotkeys =
		obs_hotkey_pair_register_frontend("OBSBasic.PauseRecording", Str("Basic.Main.PauseRecording"),
						  "OBSBasic.UnpauseRecording", Str("Basic.Main.UnpauseRecording"),
						  MAKE_CALLBACK(basic.isRecordingPausable && !basic.recordingPaused,
								basic.PauseRecording, "Pausing recording"),
						  MAKE_CALLBACK(basic.isRecordingPausable && basic.recordingPaused,
								basic.UnpauseRecording, "Unpausing recording"),
						  this, this);
	LoadHotkeyPair(pauseHotkeys, "OBSBasic.PauseRecording", "OBSBasic.UnpauseRecording");

	splitFileHotkey = obs_hotkey_register_frontend(
		"OBSBasic.SplitFile", Str("Basic.Main.SplitFile"),
		[](void *, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			if (pressed)
				obs_frontend_recording_split_file();
		},
		this);
	LoadHotkey(splitFileHotkey, "OBSBasic.SplitFile");

	addChapterHotkey = obs_hotkey_register_frontend(
		"OBSBasic.AddChapterMarker", Str("Basic.Main.AddChapterMarker"),
		[](void *, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
			if (pressed)
				obs_frontend_recording_add_chapter(nullptr);
		},
		this);
	LoadHotkey(addChapterHotkey, "OBSBasic.AddChapterMarker");

	replayBufHotkeys =
		obs_hotkey_pair_register_frontend("OBSBasic.StartReplayBuffer", Str("Basic.Main.StartReplayBuffer"),
						  "OBSBasic.StopReplayBuffer", Str("Basic.Main.StopReplayBuffer"),
						  MAKE_CALLBACK(!basic.outputHandler->ReplayBufferActive(),
								basic.StartReplayBuffer, "Starting replay buffer"),
						  MAKE_CALLBACK(basic.outputHandler->ReplayBufferActive(),
								basic.StopReplayBuffer, "Stopping replay buffer"),
						  this, this);
	LoadHotkeyPair(replayBufHotkeys, "OBSBasic.StartReplayBuffer", "OBSBasic.StopReplayBuffer");

	if (vcamEnabled) {
		vcamHotkeys = obs_hotkey_pair_register_frontend(
			"OBSBasic.StartVirtualCam", Str("Basic.Main.StartVirtualCam"), "OBSBasic.StopVirtualCam",
			Str("Basic.Main.StopVirtualCam"),
			MAKE_CALLBACK(!basic.outputHandler->VirtualCamActive(), basic.StartVirtualCam,
				      "Starting virtual camera"),
			MAKE_CALLBACK(basic.outputHandler->VirtualCamActive(), basic.StopVirtualCam,
				      "Stopping virtual camera"),
			this, this);
		LoadHotkeyPair(vcamHotkeys, "OBSBasic.StartVirtualCam", "OBSBasic.StopVirtualCam");
	}

	togglePreviewHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.EnablePreview", Str("Basic.Main.PreviewConextMenu.Enable"), "OBSBasic.DisablePreview",
		Str("Basic.Main.Preview.Disable"),
		MAKE_CALLBACK(!basic.previewEnabled, basic.EnablePreview, "Enabling preview"),
		MAKE_CALLBACK(basic.previewEnabled, basic.DisablePreview, "Disabling preview"), this, this);
	LoadHotkeyPair(togglePreviewHotkeys, "OBSBasic.EnablePreview", "OBSBasic.DisablePreview");

	togglePreviewProgramHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.EnablePreviewProgram", Str("Basic.EnablePreviewProgramMode"),
		"OBSBasic.DisablePreviewProgram", Str("Basic.DisablePreviewProgramMode"),
		MAKE_CALLBACK(!basic.IsPreviewProgramMode(), basic.EnablePreviewProgram, "Enabling preview program"),
		MAKE_CALLBACK(basic.IsPreviewProgramMode(), basic.DisablePreviewProgram, "Disabling preview program"),
		this, this);
	LoadHotkeyPair(togglePreviewProgramHotkeys, "OBSBasic.EnablePreviewProgram", "OBSBasic.DisablePreviewProgram",
		       "OBSBasic.TogglePreviewProgram");

	contextBarHotkeys = obs_hotkey_pair_register_frontend(
		"OBSBasic.ShowContextBar", Str("Basic.Main.ShowContextBar"), "OBSBasic.HideContextBar",
		Str("Basic.Main.HideContextBar"),
		MAKE_CALLBACK(!basic.ui->contextContainer->isVisible(), basic.ShowContextBar, "Showing Context Bar"),
		MAKE_CALLBACK(basic.ui->contextContainer->isVisible(), basic.HideContextBar, "Hiding Context Bar"),
		this, this);
	LoadHotkeyPair(contextBarHotkeys, "OBSBasic.ShowContextBar", "OBSBasic.HideContextBar");
#undef MAKE_CALLBACK

	auto transition = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		if (pressed)
			QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "TransitionClicked",
						  Qt::QueuedConnection);
	};

	transitionHotkey = obs_hotkey_register_frontend("OBSBasic.Transition", Str("Transition"), transition, this);
	LoadHotkey(transitionHotkey, "OBSBasic.Transition");

	auto resetStats = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		if (pressed)
			QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "ResetStatsHotkey",
						  Qt::QueuedConnection);
	};

	statsHotkey =
		obs_hotkey_register_frontend("OBSBasic.ResetStats", Str("Basic.Stats.ResetStats"), resetStats, this);
	LoadHotkey(statsHotkey, "OBSBasic.ResetStats");

	auto screenshot = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		if (pressed)
			QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "Screenshot", Qt::QueuedConnection);
	};

	screenshotHotkey = obs_hotkey_register_frontend("OBSBasic.Screenshot", Str("Screenshot"), screenshot, this);
	LoadHotkey(screenshotHotkey, "OBSBasic.Screenshot");

	auto screenshotSource = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		if (pressed)
			QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "ScreenshotSelectedSource",
						  Qt::QueuedConnection);
	};

	sourceScreenshotHotkey = obs_hotkey_register_frontend("OBSBasic.SelectedSourceScreenshot",
							      Str("Screenshot.SourceHotkey"), screenshotSource, this);
	LoadHotkey(sourceScreenshotHotkey, "OBSBasic.SelectedSourceScreenshot");
}

void OBSBasic::ClearHotkeys()
{
	obs_hotkey_pair_unregister(streamingHotkeys);
	obs_hotkey_pair_unregister(recordingHotkeys);
	obs_hotkey_pair_unregister(pauseHotkeys);
	obs_hotkey_unregister(splitFileHotkey);
	obs_hotkey_unregister(addChapterHotkey);
	obs_hotkey_pair_unregister(replayBufHotkeys);
	obs_hotkey_pair_unregister(vcamHotkeys);
	obs_hotkey_pair_unregister(togglePreviewHotkeys);
	obs_hotkey_pair_unregister(contextBarHotkeys);
	obs_hotkey_pair_unregister(togglePreviewProgramHotkeys);
	obs_hotkey_unregister(forceStreamingStopHotkey);
	obs_hotkey_unregister(transitionHotkey);
	obs_hotkey_unregister(statsHotkey);
	obs_hotkey_unregister(screenshotHotkey);
	obs_hotkey_unregister(sourceScreenshotHotkey);
}

void OBSBasic::ResetStatsHotkey()
{
	const QList<OBSBasicStats *> list = findChildren<OBSBasicStats *>();

	for (OBSBasicStats *s : list) {
		s->Reset();
	}
}
