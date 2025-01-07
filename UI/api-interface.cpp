#include <obs-frontend-internal.hpp>
#include <qt-wrappers.hpp>
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "window-basic-main-outputs.hpp"

#include <functional>

using namespace std;

Q_DECLARE_METATYPE(OBSScene);
Q_DECLARE_METATYPE(OBSSource);

template<typename T> static T GetOBSRef(QListWidgetItem *item)
{
	return item->data(static_cast<int>(QtDataRole::OBSRef)).value<T>();
}

extern volatile bool streaming_active;
extern volatile bool recording_active;
extern volatile bool recording_paused;
extern volatile bool replaybuf_active;
extern volatile bool virtualcam_active;

/* ------------------------------------------------------------------------- */

template<typename T> struct OBSStudioCallback {
	T callback;
	void *private_data;

	inline OBSStudioCallback(T cb, void *p) : callback(cb), private_data(p) {}
};

template<typename T>
inline size_t GetCallbackIdx(vector<OBSStudioCallback<T>> &callbacks, T callback, void *private_data)
{
	for (size_t i = 0; i < callbacks.size(); i++) {
		OBSStudioCallback<T> curCB = callbacks[i];
		if (curCB.callback == callback && curCB.private_data == private_data)
			return i;
	}

	return (size_t)-1;
}

struct OBSStudioAPI : obs_frontend_callbacks {
	OBSBasic *main;
	vector<OBSStudioCallback<obs_frontend_event_cb>> callbacks;
	vector<OBSStudioCallback<obs_frontend_save_cb>> saveCallbacks;
	vector<OBSStudioCallback<obs_frontend_save_cb>> preloadCallbacks;

	inline OBSStudioAPI(OBSBasic *main_) : main(main_) {}

	void *obs_frontend_get_main_window(void) override { return (void *)main; }

	void *obs_frontend_get_main_window_handle(void) override { return (void *)main->winId(); }

	void *obs_frontend_get_system_tray(void) override { return (void *)main->trayIcon.data(); }

	void obs_frontend_get_scenes(struct obs_frontend_source_list *sources) override
	{
		for (int i = 0; i < main->ui->scenes->count(); i++) {
			QListWidgetItem *item = main->ui->scenes->item(i);
			OBSScene scene = GetOBSRef<OBSScene>(item);
			obs_source_t *source = obs_scene_get_source(scene);

			if (obs_source_get_ref(source) != nullptr)
				da_push_back(sources->sources, &source);
		}
	}

	obs_source_t *obs_frontend_get_current_scene(void) override
	{
		if (main->IsPreviewProgramMode()) {
			return obs_weak_source_get_source(main->programScene);
		} else {
			OBSSource source = main->GetCurrentSceneSource();
			return obs_source_get_ref(source);
		}
	}

	void obs_frontend_set_current_scene(obs_source_t *scene) override
	{
		if (main->IsPreviewProgramMode()) {
			QMetaObject::invokeMethod(main, "TransitionToScene", WaitConnection(),
						  Q_ARG(OBSSource, OBSSource(scene)));
		} else {
			QMetaObject::invokeMethod(main, "SetCurrentScene", WaitConnection(),
						  Q_ARG(OBSSource, OBSSource(scene)), Q_ARG(bool, false));
		}
	}

	void obs_frontend_get_transitions(struct obs_frontend_source_list *sources) override
	{
		for (int i = 0; i < main->ui->transitions->count(); i++) {
			OBSSource tr = main->ui->transitions->itemData(i).value<OBSSource>();

			if (!tr)
				continue;

			if (obs_source_get_ref(tr) != nullptr)
				da_push_back(sources->sources, &tr);
		}
	}

	obs_source_t *obs_frontend_get_current_transition(void) override
	{
		OBSSource tr = main->GetCurrentTransition();
		return obs_source_get_ref(tr);
	}

	void obs_frontend_set_current_transition(obs_source_t *transition) override
	{
		QMetaObject::invokeMethod(main, "SetTransition", Q_ARG(OBSSource, OBSSource(transition)));
	}

	int obs_frontend_get_transition_duration(void) override { return main->ui->transitionDuration->value(); }

	void obs_frontend_set_transition_duration(int duration) override
	{
		QMetaObject::invokeMethod(main->ui->transitionDuration, "setValue", Q_ARG(int, duration));
	}

	void obs_frontend_release_tbar(void) override { QMetaObject::invokeMethod(main, "TBarReleased"); }

	void obs_frontend_set_tbar_position(int position) override
	{
		QMetaObject::invokeMethod(main, "TBarChanged", Q_ARG(int, position));
	}

	int obs_frontend_get_tbar_position(void) override { return main->tBar->value(); }

	void obs_frontend_get_scene_collections(std::vector<std::string> &strings) override
	{
		for (auto &[collectionName, collection] : main->GetSceneCollectionCache()) {
			strings.emplace_back(collectionName);
		}
	}

	char *obs_frontend_get_current_scene_collection(void) override
	{
		const OBSSceneCollection &currentCollection = main->GetCurrentSceneCollection();
		return bstrdup(currentCollection.name.c_str());
	}

	void obs_frontend_set_current_scene_collection(const char *collection) override
	{
		QList<QAction *> menuActions = main->ui->sceneCollectionMenu->actions();
		QString qstrCollection = QT_UTF8(collection);

		for (int i = 0; i < menuActions.count(); i++) {
			QAction *action = menuActions[i];
			QVariant v = action->property("file_name");

			if (v.typeName() != nullptr) {
				if (action->text() == qstrCollection) {
					action->trigger();
					break;
				}
			}
		}
	}

	bool obs_frontend_add_scene_collection(const char *name) override
	{
		bool success = false;
		QMetaObject::invokeMethod(main, "CreateNewSceneCollection", WaitConnection(),
					  Q_RETURN_ARG(bool, success), Q_ARG(QString, QT_UTF8(name)));
		return success;
	}

	void obs_frontend_get_profiles(std::vector<std::string> &strings) override
	{
		const OBSProfileCache &profiles = main->GetProfileCache();

		for (auto &[profileName, profile] : profiles) {
			strings.emplace_back(profileName);
		}
	}

	char *obs_frontend_get_current_profile(void) override
	{
		const OBSProfile &profile = main->GetCurrentProfile();
		return bstrdup(profile.name.c_str());
	}

	char *obs_frontend_get_current_profile_path(void) override
	{
		const OBSProfile &profile = main->GetCurrentProfile();

		return bstrdup(profile.path.u8string().c_str());
	}

	void obs_frontend_set_current_profile(const char *profile) override
	{
		QList<QAction *> menuActions = main->ui->profileMenu->actions();
		QString qstrProfile = QT_UTF8(profile);

		for (int i = 0; i < menuActions.count(); i++) {
			QAction *action = menuActions[i];
			QVariant v = action->property("file_name");

			if (v.typeName() != nullptr) {
				if (action->text() == qstrProfile) {
					action->trigger();
					break;
				}
			}
		}
	}

	void obs_frontend_create_profile(const char *name) override
	{
		QMetaObject::invokeMethod(main, "CreateNewProfile", Q_ARG(QString, name));
	}

	void obs_frontend_duplicate_profile(const char *name) override
	{
		QMetaObject::invokeMethod(main, "CreateDuplicateProfile", Q_ARG(QString, name));
	}

	void obs_frontend_delete_profile(const char *profile) override
	{
		QMetaObject::invokeMethod(main, "DeleteProfile", Q_ARG(QString, profile));
	}

	void obs_frontend_streaming_start(void) override { QMetaObject::invokeMethod(main, "StartStreaming"); }

	void obs_frontend_streaming_stop(void) override { QMetaObject::invokeMethod(main, "StopStreaming"); }

	bool obs_frontend_streaming_active(void) override { return os_atomic_load_bool(&streaming_active); }

	void obs_frontend_recording_start(void) override { QMetaObject::invokeMethod(main, "StartRecording"); }

	void obs_frontend_recording_stop(void) override { QMetaObject::invokeMethod(main, "StopRecording"); }

	bool obs_frontend_recording_active(void) override { return os_atomic_load_bool(&recording_active); }

	void obs_frontend_recording_pause(bool pause) override
	{
		QMetaObject::invokeMethod(main, pause ? "PauseRecording" : "UnpauseRecording");
	}

	bool obs_frontend_recording_paused(void) override { return os_atomic_load_bool(&recording_paused); }

	bool obs_frontend_recording_split_file(void) override
	{
		if (os_atomic_load_bool(&recording_active) && !os_atomic_load_bool(&recording_paused)) {
			proc_handler_t *ph = obs_output_get_proc_handler(main->outputHandler->fileOutput);
			uint8_t stack[128];
			calldata cd;
			calldata_init_fixed(&cd, stack, sizeof(stack));
			proc_handler_call(ph, "split_file", &cd);
			bool result = calldata_bool(&cd, "split_file_enabled");
			return result;
		} else {
			return false;
		}
	}

	bool obs_frontend_recording_add_chapter(const char *name) override
	{
		if (!os_atomic_load_bool(&recording_active) || os_atomic_load_bool(&recording_paused))
			return false;

		proc_handler_t *ph = obs_output_get_proc_handler(main->outputHandler->fileOutput);

		calldata cd;
		calldata_init(&cd);
		calldata_set_string(&cd, "chapter_name", name);
		bool result = proc_handler_call(ph, "add_chapter", &cd);
		calldata_free(&cd);
		return result;
	}

	void obs_frontend_replay_buffer_start(void) override { QMetaObject::invokeMethod(main, "StartReplayBuffer"); }

	void obs_frontend_replay_buffer_save(void) override { QMetaObject::invokeMethod(main, "ReplayBufferSave"); }

	void obs_frontend_replay_buffer_stop(void) override { QMetaObject::invokeMethod(main, "StopReplayBuffer"); }

	bool obs_frontend_replay_buffer_active(void) override { return os_atomic_load_bool(&replaybuf_active); }

	void *obs_frontend_add_tools_menu_qaction(const char *name) override
	{
		main->ui->menuTools->setEnabled(true);
		return (void *)main->ui->menuTools->addAction(QT_UTF8(name));
	}

	void obs_frontend_add_tools_menu_item(const char *name, obs_frontend_cb callback, void *private_data) override
	{
		main->ui->menuTools->setEnabled(true);

		auto func = [private_data, callback]() {
			callback(private_data);
		};

		QAction *action = main->ui->menuTools->addAction(QT_UTF8(name));
		QObject::connect(action, &QAction::triggered, func);
	}

	void *obs_frontend_add_dock(void *dock) override
	{
		QDockWidget *d = reinterpret_cast<QDockWidget *>(dock);

		QString name = d->objectName();
		if (name.isEmpty() || main->IsDockObjectNameUsed(name)) {
			blog(LOG_WARNING, "The object name of the added dock is empty or already used,"
					  " a temporary one will be set to avoid conflicts");

			char *uuid = os_generate_uuid();
			name = QT_UTF8(uuid);
			bfree(uuid);
			name.append("_oldExtraDock");

			d->setObjectName(name);
		}

		return (void *)main->AddDockWidget(d);
	}

	bool obs_frontend_add_dock_by_id(const char *id, const char *title, void *widget) override
	{
		if (main->IsDockObjectNameUsed(QT_UTF8(id))) {
			blog(LOG_WARNING,
			     "Dock id '%s' already used!  "
			     "Duplicate library?",
			     id);
			return false;
		}

		OBSDock *dock = new OBSDock(main);
		dock->setWidget((QWidget *)widget);
		dock->setWindowTitle(QT_UTF8(title));
		dock->setObjectName(QT_UTF8(id));

		main->AddDockWidget(dock, Qt::RightDockWidgetArea);

		dock->setVisible(false);
		dock->setFloating(true);

		return true;
	}

	void obs_frontend_remove_dock(const char *id) override { main->RemoveDockWidget(QT_UTF8(id)); }

	bool obs_frontend_add_custom_qdock(const char *id, void *dock) override
	{
		if (main->IsDockObjectNameUsed(QT_UTF8(id))) {
			blog(LOG_WARNING,
			     "Dock id '%s' already used!  "
			     "Duplicate library?",
			     id);
			return false;
		}

		QDockWidget *d = reinterpret_cast<QDockWidget *>(dock);
		d->setObjectName(QT_UTF8(id));

		main->AddCustomDockWidget(d);

		return true;
	}

	void obs_frontend_add_event_callback(obs_frontend_event_cb callback, void *private_data) override
	{
		size_t idx = GetCallbackIdx(callbacks, callback, private_data);
		if (idx == (size_t)-1)
			callbacks.emplace_back(callback, private_data);
	}

	void obs_frontend_remove_event_callback(obs_frontend_event_cb callback, void *private_data) override
	{
		size_t idx = GetCallbackIdx(callbacks, callback, private_data);
		if (idx == (size_t)-1)
			return;

		callbacks.erase(callbacks.begin() + idx);
	}

	obs_output_t *obs_frontend_get_streaming_output(void) override
	{
		auto multitrackVideo = main->outputHandler->multitrackVideo.get();
		auto mtvOutput = multitrackVideo ? obs_output_get_ref(multitrackVideo->StreamingOutput()) : nullptr;
		if (mtvOutput)
			return mtvOutput;

		OBSOutput output = main->outputHandler->streamOutput.Get();
		return obs_output_get_ref(output);
	}

	obs_output_t *obs_frontend_get_recording_output(void) override
	{
		OBSOutput out = main->outputHandler->fileOutput.Get();
		return obs_output_get_ref(out);
	}

	obs_output_t *obs_frontend_get_replay_buffer_output(void) override
	{
		OBSOutput out = main->outputHandler->replayBuffer.Get();
		return obs_output_get_ref(out);
	}

	config_t *obs_frontend_get_profile_config(void) override { return main->activeConfiguration; }

	config_t *obs_frontend_get_global_config(void) override
	{
		blog(LOG_WARNING,
		     "DEPRECATION: obs_frontend_get_global_config is deprecated. Read from global or user configuration explicitly instead.");
		return App()->GetAppConfig();
	}

	config_t *obs_frontend_get_app_config(void) override { return App()->GetAppConfig(); }

	config_t *obs_frontend_get_user_config(void) override { return App()->GetUserConfig(); }

	void obs_frontend_open_projector(const char *type, int monitor, const char *geometry, const char *name) override
	{
		SavedProjectorInfo proj = {
			ProjectorType::Preview,
			monitor,
			geometry ? geometry : "",
			name ? name : "",
		};
		if (type) {
			if (astrcmpi(type, "Source") == 0)
				proj.type = ProjectorType::Source;
			else if (astrcmpi(type, "Scene") == 0)
				proj.type = ProjectorType::Scene;
			else if (astrcmpi(type, "StudioProgram") == 0)
				proj.type = ProjectorType::StudioProgram;
			else if (astrcmpi(type, "Multiview") == 0)
				proj.type = ProjectorType::Multiview;
		}
		QMetaObject::invokeMethod(main, "OpenSavedProjector", WaitConnection(),
					  Q_ARG(SavedProjectorInfo *, &proj));
	}

	void obs_frontend_save(void) override { main->SaveProject(); }

	void obs_frontend_defer_save_begin(void) override { QMetaObject::invokeMethod(main, "DeferSaveBegin"); }

	void obs_frontend_defer_save_end(void) override { QMetaObject::invokeMethod(main, "DeferSaveEnd"); }

	void obs_frontend_add_save_callback(obs_frontend_save_cb callback, void *private_data) override
	{
		size_t idx = GetCallbackIdx(saveCallbacks, callback, private_data);
		if (idx == (size_t)-1)
			saveCallbacks.emplace_back(callback, private_data);
	}

	void obs_frontend_remove_save_callback(obs_frontend_save_cb callback, void *private_data) override
	{
		size_t idx = GetCallbackIdx(saveCallbacks, callback, private_data);
		if (idx == (size_t)-1)
			return;

		saveCallbacks.erase(saveCallbacks.begin() + idx);
	}

	void obs_frontend_add_preload_callback(obs_frontend_save_cb callback, void *private_data) override
	{
		size_t idx = GetCallbackIdx(preloadCallbacks, callback, private_data);
		if (idx == (size_t)-1)
			preloadCallbacks.emplace_back(callback, private_data);
	}

	void obs_frontend_remove_preload_callback(obs_frontend_save_cb callback, void *private_data) override
	{
		size_t idx = GetCallbackIdx(preloadCallbacks, callback, private_data);
		if (idx == (size_t)-1)
			return;

		preloadCallbacks.erase(preloadCallbacks.begin() + idx);
	}

	void obs_frontend_push_ui_translation(obs_frontend_translate_ui_cb translate) override
	{
		App()->PushUITranslation(translate);
	}

	void obs_frontend_pop_ui_translation(void) override { App()->PopUITranslation(); }

	void obs_frontend_set_streaming_service(obs_service_t *service) override { main->SetService(service); }

	obs_service_t *obs_frontend_get_streaming_service(void) override { return main->GetService(); }

	void obs_frontend_save_streaming_service(void) override { main->SaveService(); }

	bool obs_frontend_preview_program_mode_active(void) override { return main->IsPreviewProgramMode(); }

	void obs_frontend_set_preview_program_mode(bool enable) override { main->SetPreviewProgramMode(enable); }

	void obs_frontend_preview_program_trigger_transition(void) override
	{
		QMetaObject::invokeMethod(main, "TransitionClicked");
	}

	bool obs_frontend_preview_enabled(void) override { return main->previewEnabled; }

	void obs_frontend_set_preview_enabled(bool enable) override
	{
		if (main->previewEnabled != enable)
			main->EnablePreviewDisplay(enable);
	}

	obs_source_t *obs_frontend_get_current_preview_scene(void) override
	{
		if (main->IsPreviewProgramMode()) {
			OBSSource source = main->GetCurrentSceneSource();
			return obs_source_get_ref(source);
		}

		return nullptr;
	}

	void obs_frontend_set_current_preview_scene(obs_source_t *scene) override
	{
		if (main->IsPreviewProgramMode()) {
			QMetaObject::invokeMethod(main, "SetCurrentScene", Q_ARG(OBSSource, OBSSource(scene)),
						  Q_ARG(bool, false));
		}
	}

	void obs_frontend_take_screenshot(void) override { QMetaObject::invokeMethod(main, "Screenshot"); }

	void obs_frontend_take_source_screenshot(obs_source_t *source) override
	{
		QMetaObject::invokeMethod(main, "Screenshot", Q_ARG(OBSSource, OBSSource(source)));
	}

	obs_output_t *obs_frontend_get_virtualcam_output(void) override
	{
		OBSOutput output = main->outputHandler->virtualCam.Get();
		return obs_output_get_ref(output);
	}

	void obs_frontend_start_virtualcam(void) override { QMetaObject::invokeMethod(main, "StartVirtualCam"); }

	void obs_frontend_stop_virtualcam(void) override { QMetaObject::invokeMethod(main, "StopVirtualCam"); }

	bool obs_frontend_virtualcam_active(void) override { return os_atomic_load_bool(&virtualcam_active); }

	void obs_frontend_reset_video(void) override { main->ResetVideo(); }

	void obs_frontend_open_source_properties(obs_source_t *source) override
	{
		QMetaObject::invokeMethod(main, "OpenProperties", Q_ARG(OBSSource, OBSSource(source)));
	}

	void obs_frontend_open_source_filters(obs_source_t *source) override
	{
		QMetaObject::invokeMethod(main, "OpenFilters", Q_ARG(OBSSource, OBSSource(source)));
	}

	void obs_frontend_open_source_interaction(obs_source_t *source) override
	{
		QMetaObject::invokeMethod(main, "OpenInteraction", Q_ARG(OBSSource, OBSSource(source)));
	}

	void obs_frontend_open_sceneitem_edit_transform(obs_sceneitem_t *item) override
	{
		QMetaObject::invokeMethod(main, "OpenEditTransform", Q_ARG(OBSSceneItem, OBSSceneItem(item)));
	}

	char *obs_frontend_get_current_record_output_path(void) override
	{
		const char *recordOutputPath = main->GetCurrentOutputPath();

		return bstrdup(recordOutputPath);
	}

	const char *obs_frontend_get_locale_string(const char *string) override { return Str(string); }

	bool obs_frontend_is_theme_dark(void) override { return App()->IsThemeDark(); }

	char *obs_frontend_get_last_recording(void) override
	{
		return bstrdup(main->outputHandler->lastRecordingPath.c_str());
	}

	char *obs_frontend_get_last_screenshot(void) override { return bstrdup(main->lastScreenshot.c_str()); }

	char *obs_frontend_get_last_replay(void) override { return bstrdup(main->lastReplay.c_str()); }

	void obs_frontend_add_undo_redo_action(const char *name, const undo_redo_cb undo, const undo_redo_cb redo,
					       const char *undo_data, const char *redo_data, bool repeatable) override
	{
		main->undo_s.add_action(
			name, [undo](const std::string &data) { undo(data.c_str()); },
			[redo](const std::string &data) { redo(data.c_str()); }, undo_data, redo_data, repeatable);
	}

	void on_load(obs_data_t *settings) override
	{
		for (size_t i = saveCallbacks.size(); i > 0; i--) {
			auto cb = saveCallbacks[i - 1];
			cb.callback(settings, false, cb.private_data);
		}
	}

	void on_preload(obs_data_t *settings) override
	{
		for (size_t i = preloadCallbacks.size(); i > 0; i--) {
			auto cb = preloadCallbacks[i - 1];
			cb.callback(settings, false, cb.private_data);
		}
	}

	void on_save(obs_data_t *settings) override
	{
		for (size_t i = saveCallbacks.size(); i > 0; i--) {
			auto cb = saveCallbacks[i - 1];
			cb.callback(settings, true, cb.private_data);
		}
	}

	void on_event(enum obs_frontend_event event) override
	{
		if (main->disableSaving && event != OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP &&
		    event != OBS_FRONTEND_EVENT_EXIT)
			return;

		for (size_t i = callbacks.size(); i > 0; i--) {
			auto cb = callbacks[i - 1];
			cb.callback(event, cb.private_data);
		}
	}

	void obs_frontend_multitrack_video_register(const char *name,
						    obs_frontend_multitrack_video_start_cb start_video,
						    obs_frontend_multitrack_video_stop_cb stop_video,
						    void *private_data) override
	{
		main->MultitrackVideoRegister(name, start_video, stop_video, private_data);
	}

	void obs_frontend_multitrack_video_unregister(const char *name) override
	{
		main->MultitrackVideoUnregister(name);
	}
};

obs_frontend_callbacks *InitializeAPIInterface(OBSBasic *main)
{
	obs_frontend_callbacks *api = new OBSStudioAPI(main);
	obs_frontend_set_callbacks_internal(api);
	return api;
}
