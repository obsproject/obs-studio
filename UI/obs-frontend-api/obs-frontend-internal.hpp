#pragma once

#include "obs-frontend-api.h"

#include <vector>
#include <string>

struct obs_frontend_callbacks {
	virtual ~obs_frontend_callbacks() {}
	virtual void *obs_frontend_get_main_window(void)=0;
	virtual void *obs_frontend_get_main_window_handle(void)=0;

	virtual void obs_frontend_get_scenes(
			struct obs_frontend_source_list *sources)=0;
	virtual obs_source_t *obs_frontend_get_current_scene(void)=0;
	virtual void obs_frontend_set_current_scene(obs_source_t *scene)=0;

	virtual void obs_frontend_get_transitions(
			struct obs_frontend_source_list *sources)=0;
	virtual obs_source_t *obs_frontend_get_current_transition(void)=0;
	virtual void obs_frontend_set_current_transition(
			obs_source_t *transition)=0;

	virtual void obs_frontend_get_scene_collections(
			std::vector<std::string> &strings)=0;
	virtual char *obs_frontend_get_current_scene_collection(void)=0;
	virtual void obs_frontend_set_current_scene_collection(
			const char *collection)=0;

	virtual void obs_frontend_get_profiles(
			std::vector<std::string> &strings)=0;
	virtual char *obs_frontend_get_current_profile(void)=0;
	virtual void obs_frontend_set_current_profile(const char *profile)=0;

	virtual void obs_frontend_streaming_start(void)=0;
	virtual void obs_frontend_streaming_stop(void)=0;
	virtual bool obs_frontend_streaming_active(void)=0;

	virtual void obs_frontend_recording_start(void)=0;
	virtual void obs_frontend_recording_stop(void)=0;
	virtual bool obs_frontend_recording_active(void)=0;

	virtual void obs_frontend_replay_buffer_start(void)=0;
	virtual void obs_frontend_replay_buffer_stop(void)=0;
	virtual bool obs_frontend_replay_buffer_active(void)=0;

	virtual void *obs_frontend_add_tools_menu_qaction(const char *name)=0;
	virtual void obs_frontend_add_tools_menu_item(const char *name,
			obs_frontend_cb callback, void *private_data)=0;

	virtual void obs_frontend_add_event_callback(
			obs_frontend_event_cb callback, void *private_data)=0;
	virtual void obs_frontend_remove_event_callback(
			obs_frontend_event_cb callback, void *private_data)=0;

	virtual obs_output_t *obs_frontend_get_streaming_output(void)=0;
	virtual obs_output_t *obs_frontend_get_recording_output(void)=0;
	virtual obs_output_t *obs_frontend_get_replay_buffer_output(void)=0;

	virtual config_t *obs_frontend_get_profile_config(void)=0;
	virtual config_t *obs_frontend_get_global_config(void)=0;

	virtual void obs_frontend_save(void)=0;
	virtual void obs_frontend_add_save_callback(
			obs_frontend_save_cb callback, void *private_data)=0;
	virtual void obs_frontend_remove_save_callback(
			obs_frontend_save_cb callback, void *private_data)=0;

	virtual void obs_frontend_push_ui_translation(
			obs_frontend_translate_ui_cb translate)=0;
	virtual void obs_frontend_pop_ui_translation(void)=0;

	virtual void obs_frontend_set_streaming_service(
			obs_service_t *service)=0;
	virtual obs_service_t *obs_frontend_get_streaming_service(void)=0;
	virtual void obs_frontend_save_streaming_service()=0;

	virtual void on_load(obs_data_t *settings)=0;
	virtual void on_save(obs_data_t *settings)=0;
	virtual void on_event(enum obs_frontend_event event)=0;
};

EXPORT void obs_frontend_set_callbacks_internal(
		obs_frontend_callbacks *callbacks);
