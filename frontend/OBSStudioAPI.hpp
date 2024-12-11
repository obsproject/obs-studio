/******************************************************************************
 Copyright (C) 2024 by Patrick Heyer <opensource@patrickheyer.com>

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

#pragma once

#include <obs-frontend-internal.hpp>

class OBSBasic;

using namespace std;

template<typename T> struct OBSStudioCallback {
	T callback;
	void *private_data;

	inline OBSStudioCallback(T cb, void *p) : callback(cb), private_data(p) {}
};

struct OBSStudioAPI : obs_frontend_callbacks {
	OBSBasic *main;
	vector<OBSStudioCallback<obs_frontend_event_cb>> callbacks;
	vector<OBSStudioCallback<obs_frontend_save_cb>> saveCallbacks;
	vector<OBSStudioCallback<obs_frontend_save_cb>> preloadCallbacks;

	inline OBSStudioAPI(OBSBasic *main_) : main(main_) {}

	void *obs_frontend_get_main_window(void) override;

	void *obs_frontend_get_main_window_handle(void) override;

	void *obs_frontend_get_system_tray(void) override;

	void obs_frontend_get_scenes(struct obs_frontend_source_list *sources) override;

	obs_source_t *obs_frontend_get_current_scene(void) override;

	void obs_frontend_set_current_scene(obs_source_t *scene) override;

	void obs_frontend_get_transitions(struct obs_frontend_source_list *sources) override;

	obs_source_t *obs_frontend_get_current_transition(void) override;

	void obs_frontend_set_current_transition(obs_source_t *transition) override;

	int obs_frontend_get_transition_duration(void) override;

	void obs_frontend_set_transition_duration(int duration) override;

	void obs_frontend_release_tbar(void) override;

	void obs_frontend_set_tbar_position(int position) override;

	int obs_frontend_get_tbar_position(void) override;

	void obs_frontend_get_scene_collections(std::vector<std::string> &strings) override;

	char *obs_frontend_get_current_scene_collection(void) override;

	void obs_frontend_set_current_scene_collection(const char *collection) override;

	bool obs_frontend_add_scene_collection(const char *name) override;

	void obs_frontend_get_profiles(std::vector<std::string> &strings) override;

	char *obs_frontend_get_current_profile(void) override;

	char *obs_frontend_get_current_profile_path(void) override;

	void obs_frontend_set_current_profile(const char *profile) override;

	void obs_frontend_create_profile(const char *name) override;

	void obs_frontend_duplicate_profile(const char *name) override;

	void obs_frontend_delete_profile(const char *profile) override;

	void obs_frontend_streaming_start(void) override;

	void obs_frontend_streaming_stop(void) override;

	bool obs_frontend_streaming_active(void) override;

	void obs_frontend_recording_start(void) override;

	void obs_frontend_recording_stop(void) override;

	bool obs_frontend_recording_active(void) override;

	void obs_frontend_recording_pause(bool pause) override;

	bool obs_frontend_recording_paused(void) override;

	bool obs_frontend_recording_split_file(void) override;

	bool obs_frontend_recording_add_chapter(const char *name) override;

	void obs_frontend_replay_buffer_start(void) override;

	void obs_frontend_replay_buffer_save(void) override;

	void obs_frontend_replay_buffer_stop(void) override;

	bool obs_frontend_replay_buffer_active(void) override;

	void *obs_frontend_add_tools_menu_qaction(const char *name) override;

	void obs_frontend_add_tools_menu_item(const char *name, obs_frontend_cb callback, void *private_data) override;

	void *obs_frontend_add_dock(void *dock) override;

	bool obs_frontend_add_dock_by_id(const char *id, const char *title, void *widget) override;

	void obs_frontend_remove_dock(const char *id) override;

	bool obs_frontend_add_custom_qdock(const char *id, void *dock) override;

	void obs_frontend_add_event_callback(obs_frontend_event_cb callback, void *private_data) override;

	void obs_frontend_remove_event_callback(obs_frontend_event_cb callback, void *private_data) override;

	obs_output_t *obs_frontend_get_streaming_output(void) override;

	obs_output_t *obs_frontend_get_recording_output(void) override;

	obs_output_t *obs_frontend_get_replay_buffer_output(void) override;

	config_t *obs_frontend_get_profile_config(void) override;

	config_t *obs_frontend_get_global_config(void) override;

	config_t *obs_frontend_get_app_config(void) override;

	config_t *obs_frontend_get_user_config(void) override;

	void obs_frontend_open_projector(const char *type, int monitor, const char *geometry,
					 const char *name) override;

	void obs_frontend_save(void) override;

	void obs_frontend_defer_save_begin(void) override;

	void obs_frontend_defer_save_end(void) override;

	void obs_frontend_add_save_callback(obs_frontend_save_cb callback, void *private_data) override;

	void obs_frontend_remove_save_callback(obs_frontend_save_cb callback, void *private_data) override;

	void obs_frontend_add_preload_callback(obs_frontend_save_cb callback, void *private_data) override;

	void obs_frontend_remove_preload_callback(obs_frontend_save_cb callback, void *private_data) override;

	void obs_frontend_push_ui_translation(obs_frontend_translate_ui_cb translate) override;

	void obs_frontend_pop_ui_translation(void) override;

	void obs_frontend_set_streaming_service(obs_service_t *service) override;

	obs_service_t *obs_frontend_get_streaming_service(void) override;

	void obs_frontend_save_streaming_service(void) override;

	bool obs_frontend_preview_program_mode_active(void) override;

	void obs_frontend_set_preview_program_mode(bool enable) override;

	void obs_frontend_preview_program_trigger_transition(void) override;

	bool obs_frontend_preview_enabled(void) override;

	void obs_frontend_set_preview_enabled(bool enable) override;

	obs_source_t *obs_frontend_get_current_preview_scene(void) override;

	void obs_frontend_set_current_preview_scene(obs_source_t *scene) override;

	void obs_frontend_take_screenshot(void) override;

	void obs_frontend_take_source_screenshot(obs_source_t *source) override;

	obs_output_t *obs_frontend_get_virtualcam_output(void) override;

	void obs_frontend_start_virtualcam(void) override;

	void obs_frontend_stop_virtualcam(void) override;

	bool obs_frontend_virtualcam_active(void) override;

	void obs_frontend_reset_video(void) override;

	void obs_frontend_open_source_properties(obs_source_t *source) override;

	void obs_frontend_open_source_filters(obs_source_t *source) override;

	void obs_frontend_open_source_interaction(obs_source_t *source) override;

	void obs_frontend_open_sceneitem_edit_transform(obs_sceneitem_t *item) override;

	char *obs_frontend_get_current_record_output_path(void) override;

	const char *obs_frontend_get_locale_string(const char *string) override;

	bool obs_frontend_is_theme_dark(void) override;

	char *obs_frontend_get_last_recording(void) override;

	char *obs_frontend_get_last_screenshot(void) override;

	char *obs_frontend_get_last_replay(void) override;

	void obs_frontend_add_undo_redo_action(const char *name, const undo_redo_cb undo, const undo_redo_cb redo,
					       const char *undo_data, const char *redo_data, bool repeatable) override;

	void on_load(obs_data_t *settings) override;

	void on_preload(obs_data_t *settings) override;

	void on_save(obs_data_t *settings) override;

	void on_event(enum obs_frontend_event event) override;
};

obs_frontend_callbacks *InitializeAPIInterface(OBSBasic *main);
