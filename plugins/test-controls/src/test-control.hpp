#pragma once
#include <obs.hpp>
#include <memory>
#include <QObject>

void ___source_dummy_addref(obs_source_t *);
void ___sceneitem_dummy_addref(obs_sceneitem_t *);
void ___data_dummy_addref(obs_data_t *);
void ___data_array_dummy_addref(obs_data_array_t *);
void ___output_dummy_addref(obs_output_t *);

using OBSSourceAutoRelease =
	OBSRef<obs_source_t *, ___source_dummy_addref, obs_source_release>;
using OBSSceneItemAutoRelease =
	OBSRef<obs_sceneitem_t *, ___sceneitem_dummy_addref,
	       obs_sceneitem_release>;
using OBSDataAutoRelease =
	OBSRef<obs_data_t *, ___data_dummy_addref, obs_data_release>;
using OBSDataArrayAutoRelease =
	OBSRef<obs_data_array_t *, ___data_array_dummy_addref,
	       obs_data_array_release>;
using OBSOutputAutoRelease =
	OBSRef<obs_output_t *, ___output_dummy_addref, obs_output_release>;
void setup_ui();

void ___data_item_dummy_addref(obs_data_item_t *);
void ___data_item_release(obs_data_item_t *);
using OBSDataItemAutoRelease =
	OBSRef<obs_data_item_t *, ___data_item_dummy_addref,
	       ___data_item_release>;

#define OBS_MIDI_VERSION "0.1"

#define blog(level, msg, ...) blog(level, "[test-control] " msg, ##__VA_ARGS__)
