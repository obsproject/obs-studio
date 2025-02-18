#include "OBSStudioAPI.hpp"

#include <models/SceneCollection.hpp>
#include <widgets/OBSBasic.hpp>
#include <widgets/OBSProjector.hpp>

#include <qt-wrappers.hpp>

extern volatile bool streaming_active;
extern volatile bool recording_active;
extern volatile bool recording_paused;
extern volatile bool replaybuf_active;
extern volatile bool virtualcam_active;

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

void *OBSStudioAPI::obs_frontend_get_main_window()
{
	return (void *)main;
}

void *OBSStudioAPI::obs_frontend_get_main_window_handle()
{
	return (void *)main->winId();
}

void *OBSStudioAPI::obs_frontend_get_system_tray()
{
	return (void *)main->trayIcon.data();
}

void OBSStudioAPI::obs_frontend_get_scenes(struct obs_frontend_source_list *sources)
{
	for (int i = 0; i < main->ui->scenes->count(); i++) {
		QListWidgetItem *item = main->ui->scenes->item(i);
		OBSScene scene = GetOBSRef<OBSScene>(item);
		obs_source_t *source = obs_scene_get_source(scene);

		if (obs_source_get_ref(source) != nullptr)
			da_push_back(sources->sources, &source);
	}
}

obs_source_t *OBSStudioAPI::obs_frontend_get_current_scene()
{
	if (main->IsPreviewProgramMode()) {
		return obs_weak_source_get_source(main->programScene);
	} else {
		OBSSource source = main->GetCurrentSceneSource();
		return obs_source_get_ref(source);
	}
}

void OBSStudioAPI::obs_frontend_set_current_scene(obs_source_t *scene)
{
	if (main->IsPreviewProgramMode()) {
		QMetaObject::invokeMethod(main, "TransitionToScene", WaitConnection(),
					  Q_ARG(OBSSource, OBSSource(scene)));
	} else {
		QMetaObject::invokeMethod(main, "SetCurrentScene", WaitConnection(), Q_ARG(OBSSource, OBSSource(scene)),
					  Q_ARG(bool, false));
	}
}

void OBSStudioAPI::obs_frontend_get_transitions(struct obs_frontend_source_list *sources)
{
	for (const auto &[uuid, transition] : main->transitions) {
		obs_source_t *source = transition;

		if (obs_source_get_ref(source) != nullptr)
			da_push_back(sources->sources, &source);
	}
}

obs_source_t *OBSStudioAPI::obs_frontend_get_current_transition()
{
	OBSSource tr = main->GetCurrentTransition();
	return obs_source_get_ref(tr);
}

void OBSStudioAPI::obs_frontend_set_current_transition(obs_source_t *transition)
{
	QMetaObject::invokeMethod(main, "SetTransition", Q_ARG(OBSSource, OBSSource(transition)));
}

int OBSStudioAPI::obs_frontend_get_transition_duration()
{
	return main->GetTransitionDuration();
}

void OBSStudioAPI::obs_frontend_set_transition_duration(int duration)
{
	QMetaObject::invokeMethod(main, "SetTransitionDuration", Q_ARG(int, duration));
}

void OBSStudioAPI::obs_frontend_release_tbar()
{
	QMetaObject::invokeMethod(main, "TBarReleased");
}

void OBSStudioAPI::obs_frontend_set_tbar_position(int position)
{
	QMetaObject::invokeMethod(main, "TBarChanged", Q_ARG(int, position));
}

int OBSStudioAPI::obs_frontend_get_tbar_position()
{
	return main->tBar->value();
}

void OBSStudioAPI::obs_frontend_get_scene_collections(std::vector<std::string> &strings)
{
	for (auto &[collectionName, collection] : main->GetSceneCollectionCache()) {
		strings.emplace_back(collectionName);
	}
}

char *OBSStudioAPI::obs_frontend_get_current_scene_collection()
{
	try {
		const OBS::SceneCollection &currentCollection = main->GetCurrentSceneCollection();
		return bstrdup(currentCollection.getName().c_str());
	} catch (const std::exception &error) {
		blog(LOG_DEBUG, "%s", error.what());
		blog(LOG_ERROR, "Failed to get current scene collection name");
		return nullptr;
	}
}

void OBSStudioAPI::obs_frontend_set_current_scene_collection(const char *collection)
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

bool OBSStudioAPI::obs_frontend_add_scene_collection(const char *name)
{
	bool success = false;
	QMetaObject::invokeMethod(main, "CreateNewSceneCollection", WaitConnection(), Q_RETURN_ARG(bool, success),
				  Q_ARG(QString, QT_UTF8(name)));
	return success;
}

void OBSStudioAPI::obs_frontend_get_profiles(std::vector<std::string> &strings)
{
	const OBSProfileCache &profiles = main->GetProfileCache();

	for (auto &[profileName, profile] : profiles) {
		strings.emplace_back(profileName);
	}
}

char *OBSStudioAPI::obs_frontend_get_current_profile()
{
	const OBSProfile &profile = main->GetCurrentProfile();
	return bstrdup(profile.name.c_str());
}

char *OBSStudioAPI::obs_frontend_get_current_profile_path()
{
	const OBSProfile &profile = main->GetCurrentProfile();

	return bstrdup(profile.path.u8string().c_str());
}

void OBSStudioAPI::obs_frontend_set_current_profile(const char *profile)
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

void OBSStudioAPI::obs_frontend_create_profile(const char *name)
{
	QMetaObject::invokeMethod(main, "CreateNewProfile", Q_ARG(QString, name));
}

void OBSStudioAPI::obs_frontend_duplicate_profile(const char *name)
{
	QMetaObject::invokeMethod(main, "CreateDuplicateProfile", Q_ARG(QString, name));
}

void OBSStudioAPI::obs_frontend_delete_profile(const char *profile)
{
	QMetaObject::invokeMethod(main, "DeleteProfile", Q_ARG(QString, profile));
}

void OBSStudioAPI::obs_frontend_streaming_start()
{
	QMetaObject::invokeMethod(main, "StartStreaming");
}

void OBSStudioAPI::obs_frontend_streaming_stop()
{
	QMetaObject::invokeMethod(main, "StopStreaming");
}

bool OBSStudioAPI::obs_frontend_streaming_active()
{
	return os_atomic_load_bool(&streaming_active);
}

void OBSStudioAPI::obs_frontend_recording_start()
{
	QMetaObject::invokeMethod(main, "StartRecording");
}

void OBSStudioAPI::obs_frontend_recording_stop()
{
	QMetaObject::invokeMethod(main, "StopRecording");
}

bool OBSStudioAPI::obs_frontend_recording_active()
{
	return os_atomic_load_bool(&recording_active);
}

void OBSStudioAPI::obs_frontend_recording_pause(bool pause)
{
	QMetaObject::invokeMethod(main, pause ? "PauseRecording" : "UnpauseRecording");
}

bool OBSStudioAPI::obs_frontend_recording_paused()
{
	return os_atomic_load_bool(&recording_paused);
}

bool OBSStudioAPI::obs_frontend_recording_split_file()
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

bool OBSStudioAPI::obs_frontend_recording_add_chapter(const char *name)
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

void OBSStudioAPI::obs_frontend_replay_buffer_start()
{
	QMetaObject::invokeMethod(main, "StartReplayBuffer");
}

void OBSStudioAPI::obs_frontend_replay_buffer_save()
{
	QMetaObject::invokeMethod(main, "ReplayBufferSave");
}

void OBSStudioAPI::obs_frontend_replay_buffer_stop()
{
	QMetaObject::invokeMethod(main, "StopReplayBuffer");
}

bool OBSStudioAPI::obs_frontend_replay_buffer_active()
{
	return os_atomic_load_bool(&replaybuf_active);
}

void *OBSStudioAPI::obs_frontend_add_tools_menu_qaction(const char *name)
{
	main->ui->menuTools->setEnabled(true);
	QAction *action = main->ui->menuTools->addAction(QT_UTF8(name));
	action->setMenuRole(QAction::NoRole);
	return static_cast<void *>(action);
}

void OBSStudioAPI::obs_frontend_add_tools_menu_item(const char *name, obs_frontend_cb callback, void *private_data)
{
	main->ui->menuTools->setEnabled(true);

	auto func = [private_data, callback]() {
		callback(private_data);
	};

	QAction *action = main->ui->menuTools->addAction(QT_UTF8(name));
	action->setMenuRole(QAction::NoRole);
	QObject::connect(action, &QAction::triggered, func);
}

bool OBSStudioAPI::obs_frontend_add_dock_by_id(const char *id, const char *title, void *widget)
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

void OBSStudioAPI::obs_frontend_remove_dock(const char *id)
{
	main->RemoveDockWidget(QT_UTF8(id));
}

bool OBSStudioAPI::obs_frontend_add_custom_qdock(const char *id, void *dock)
{
	if (main->IsDockObjectNameUsed(QT_UTF8(id))) {
		blog(LOG_WARNING,
		     "Dock id '%s' already used!  "
		     "Duplicate library?",
		     id);
		return false;
	}

	QDockWidget *d = static_cast<QDockWidget *>(dock);
	d->setObjectName(QT_UTF8(id));

	main->AddCustomDockWidget(d);

	return true;
}

void OBSStudioAPI::obs_frontend_add_event_callback(obs_frontend_event_cb callback, void *private_data)
{
	size_t idx = GetCallbackIdx(callbacks, callback, private_data);
	if (idx == (size_t)-1)
		callbacks.emplace_back(callback, private_data);
}

void OBSStudioAPI::obs_frontend_remove_event_callback(obs_frontend_event_cb callback, void *private_data)
{
	size_t idx = GetCallbackIdx(callbacks, callback, private_data);
	if (idx == (size_t)-1)
		return;

	callbacks.erase(callbacks.begin() + idx);
}

obs_output_t *OBSStudioAPI::obs_frontend_get_streaming_output()
{
	auto multitrackVideo = main->outputHandler->multitrackVideo.get();
	auto mtvOutput = multitrackVideo ? obs_output_get_ref(multitrackVideo->StreamingOutput()) : nullptr;
	if (mtvOutput)
		return mtvOutput;

	OBSOutput output = main->outputHandler->streamOutput.Get();
	return obs_output_get_ref(output);
}

obs_output_t *OBSStudioAPI::obs_frontend_get_recording_output()
{
	OBSOutput out = main->outputHandler->fileOutput.Get();
	return obs_output_get_ref(out);
}

obs_output_t *OBSStudioAPI::obs_frontend_get_replay_buffer_output()
{
	OBSOutput out = main->outputHandler->replayBuffer.Get();
	return obs_output_get_ref(out);
}

config_t *OBSStudioAPI::obs_frontend_get_profile_config()
{
	return main->activeConfiguration;
}

config_t *OBSStudioAPI::obs_frontend_get_global_config()
{
	blog(LOG_WARNING,
	     "DEPRECATION: obs_frontend_get_global_config is deprecated. Read from global or user configuration explicitly instead.");
	return App()->GetAppConfig();
}

config_t *OBSStudioAPI::obs_frontend_get_app_config()
{
	return App()->GetAppConfig();
}

config_t *OBSStudioAPI::obs_frontend_get_user_config()
{
	return App()->GetUserConfig();
}

void OBSStudioAPI::obs_frontend_open_projector(const char *type, int monitor, const char *geometry, const char *name)
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
	QMetaObject::invokeMethod(main, "OpenSavedProjector", WaitConnection(), Q_ARG(SavedProjectorInfo *, &proj));
}

void OBSStudioAPI::obs_frontend_save()
{
	main->SaveProject();
}

void OBSStudioAPI::obs_frontend_defer_save_begin()
{
	QMetaObject::invokeMethod(main, "DeferSaveBegin");
}

void OBSStudioAPI::obs_frontend_defer_save_end()
{
	QMetaObject::invokeMethod(main, "DeferSaveEnd");
}

void OBSStudioAPI::obs_frontend_add_save_callback(obs_frontend_save_cb callback, void *private_data)
{
	size_t idx = GetCallbackIdx(saveCallbacks, callback, private_data);
	if (idx == (size_t)-1)
		saveCallbacks.emplace_back(callback, private_data);
}

void OBSStudioAPI::obs_frontend_remove_save_callback(obs_frontend_save_cb callback, void *private_data)
{
	size_t idx = GetCallbackIdx(saveCallbacks, callback, private_data);
	if (idx == (size_t)-1)
		return;

	saveCallbacks.erase(saveCallbacks.begin() + idx);
}

void OBSStudioAPI::obs_frontend_add_preload_callback(obs_frontend_save_cb callback, void *private_data)
{
	size_t idx = GetCallbackIdx(preloadCallbacks, callback, private_data);
	if (idx == (size_t)-1)
		preloadCallbacks.emplace_back(callback, private_data);
}

void OBSStudioAPI::obs_frontend_remove_preload_callback(obs_frontend_save_cb callback, void *private_data)
{
	size_t idx = GetCallbackIdx(preloadCallbacks, callback, private_data);
	if (idx == (size_t)-1)
		return;

	preloadCallbacks.erase(preloadCallbacks.begin() + idx);
}

void OBSStudioAPI::obs_frontend_push_ui_translation(obs_frontend_translate_ui_cb translate)
{
	App()->PushUITranslation(translate);
}

void OBSStudioAPI::obs_frontend_pop_ui_translation()
{
	App()->PopUITranslation();
}

void OBSStudioAPI::obs_frontend_set_streaming_service(obs_service_t *service)
{
	main->SetService(service);
}

obs_service_t *OBSStudioAPI::obs_frontend_get_streaming_service()
{
	return main->GetService();
}

void OBSStudioAPI::obs_frontend_save_streaming_service()
{
	main->SaveService();
}

bool OBSStudioAPI::obs_frontend_preview_program_mode_active()
{
	return main->IsPreviewProgramMode();
}

void OBSStudioAPI::obs_frontend_set_preview_program_mode(bool enable)
{
	main->SetPreviewProgramMode(enable);
}

void OBSStudioAPI::obs_frontend_preview_program_trigger_transition()
{
	QMetaObject::invokeMethod(main, "TransitionClicked");
}

bool OBSStudioAPI::obs_frontend_preview_enabled()
{
	return main->previewEnabled;
}

void OBSStudioAPI::obs_frontend_set_preview_enabled(bool enable)
{
	if (main->previewEnabled != enable)
		main->EnablePreviewDisplay(enable);
}

obs_source_t *OBSStudioAPI::obs_frontend_get_current_preview_scene()
{
	if (main->IsPreviewProgramMode()) {
		OBSSource source = main->GetCurrentSceneSource();
		return obs_source_get_ref(source);
	}

	return nullptr;
}

void OBSStudioAPI::obs_frontend_set_current_preview_scene(obs_source_t *scene)
{
	if (main->IsPreviewProgramMode()) {
		QMetaObject::invokeMethod(main, "SetCurrentScene", Q_ARG(OBSSource, OBSSource(scene)),
					  Q_ARG(bool, false));
	}
}

void OBSStudioAPI::obs_frontend_take_screenshot()
{
	QMetaObject::invokeMethod(main, "Screenshot");
}

void OBSStudioAPI::obs_frontend_take_source_screenshot(obs_source_t *source)
{
	QMetaObject::invokeMethod(main, "Screenshot", Q_ARG(OBSSource, OBSSource(source)));
}

obs_output_t *OBSStudioAPI::obs_frontend_get_virtualcam_output()
{
	OBSOutput output = main->outputHandler->virtualCam.Get();
	return obs_output_get_ref(output);
}

void OBSStudioAPI::obs_frontend_start_virtualcam()
{
	QMetaObject::invokeMethod(main, "StartVirtualCam");
}

void OBSStudioAPI::obs_frontend_stop_virtualcam()
{
	QMetaObject::invokeMethod(main, "StopVirtualCam");
}

bool OBSStudioAPI::obs_frontend_virtualcam_active()
{
	return os_atomic_load_bool(&virtualcam_active);
}

void OBSStudioAPI::obs_frontend_reset_video()
{
	main->ResetVideo();
}

void OBSStudioAPI::obs_frontend_open_source_properties(obs_source_t *source)
{
	QMetaObject::invokeMethod(main, "OpenProperties", Q_ARG(OBSSource, OBSSource(source)));
}

void OBSStudioAPI::obs_frontend_open_source_filters(obs_source_t *source)
{
	QMetaObject::invokeMethod(main, "OpenFilters", Q_ARG(OBSSource, OBSSource(source)));
}

void OBSStudioAPI::obs_frontend_open_source_interaction(obs_source_t *source)
{
	QMetaObject::invokeMethod(main, "OpenInteraction", Q_ARG(OBSSource, OBSSource(source)));
}

void OBSStudioAPI::obs_frontend_open_sceneitem_edit_transform(obs_sceneitem_t *item)
{
	QMetaObject::invokeMethod(main, "OpenEditTransform", Q_ARG(OBSSceneItem, OBSSceneItem(item)));
}

char *OBSStudioAPI::obs_frontend_get_current_record_output_path()
{
	const char *recordOutputPath = main->GetCurrentOutputPath();

	return bstrdup(recordOutputPath);
}

const char *OBSStudioAPI::obs_frontend_get_locale_string(const char *string)
{
	return Str(string);
}

bool OBSStudioAPI::obs_frontend_is_theme_dark()
{
	return App()->IsThemeDark();
}

char *OBSStudioAPI::obs_frontend_get_last_recording()
{
	return bstrdup(main->outputHandler->lastRecordingPath.c_str());
}

char *OBSStudioAPI::obs_frontend_get_last_screenshot()
{
	return bstrdup(main->lastScreenshot.c_str());
}

char *OBSStudioAPI::obs_frontend_get_last_replay()
{
	return bstrdup(main->lastReplay.c_str());
}

void OBSStudioAPI::obs_frontend_add_undo_redo_action(const char *name, const undo_redo_cb undo, const undo_redo_cb redo,
						     const char *undo_data, const char *redo_data, bool repeatable)
{
	main->undo_s.add_action(
		name, [undo](const std::string &data) { undo(data.c_str()); },
		[redo](const std::string &data) { redo(data.c_str()); }, undo_data, redo_data, repeatable);
}

void OBSStudioAPI::obs_frontend_get_canvases(obs_frontend_canvas_list *canvas_list)
{
	for (const auto &canvas : main->canvases) {
		obs_canvas_t *ref = obs_canvas_get_ref(canvas);
		if (ref)
			da_push_back(canvas_list->canvases, &ref);
	}
}

obs_canvas_t *OBSStudioAPI::obs_frontend_add_canvas(const char *name, obs_video_info *ovi, int flags)
{
	auto &canvas = main->AddCanvas(std::string(name), ovi, flags);
	return obs_canvas_get_ref(canvas);
}

bool OBSStudioAPI::obs_frontend_remove_canvas(obs_canvas_t *canvas)
{
	return main->RemoveCanvas(canvas);
}

void OBSStudioAPI::on_load(obs_data_t *settings)
{
	for (size_t i = saveCallbacks.size(); i > 0; i--) {
		auto cb = saveCallbacks[i - 1];
		cb.callback(settings, false, cb.private_data);
	}
}

void OBSStudioAPI::on_preload(obs_data_t *settings)
{
	for (size_t i = preloadCallbacks.size(); i > 0; i--) {
		auto cb = preloadCallbacks[i - 1];
		cb.callback(settings, false, cb.private_data);
	}
}

void OBSStudioAPI::on_save(obs_data_t *settings)
{
	for (size_t i = saveCallbacks.size(); i > 0; i--) {
		auto cb = saveCallbacks[i - 1];
		cb.callback(settings, true, cb.private_data);
	}
}

void OBSStudioAPI::on_event(enum obs_frontend_event event)
{
	if (main->disableSaving && event != OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP &&
	    event != OBS_FRONTEND_EVENT_EXIT)
		return;

	for (size_t i = callbacks.size(); i > 0; i--) {
		auto cb = callbacks[i - 1];
		cb.callback(event, cb.private_data);
	}
}

obs_frontend_callbacks *InitializeAPIInterface(OBSBasic *main)
{
	obs_frontend_callbacks *api = new OBSStudioAPI(main);
	obs_frontend_set_callbacks_internal(api);
	return api;
}
