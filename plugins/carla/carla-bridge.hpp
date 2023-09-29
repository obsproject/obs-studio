/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <CarlaBackend.h>
#include <CarlaBridgeUtils.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QProcess>
#include <QtCore/QString>

#include <vector>

#include <obs.h>

CARLA_BACKEND_USE_NAMESPACE

// ----------------------------------------------------------------------------
// custom class for allowing QProcess usage outside the main thread

class BridgeProcess : public QProcess {
	Q_OBJECT

public:
	BridgeProcess(const char *shmIds);

public Q_SLOTS:
	void start();
	void stop();
};

// ----------------------------------------------------------------------------
// relevant information for an exposed plugin parameter

struct carla_param_data {
	uint32_t hints = 0;
	float value = 0.f;
	float def = 0.f;
	float min = 0.f;
	float max = 1.f;
	float step = 0.01f;
	CarlaString name;
	CarlaString symbol;
	CarlaString unit;
};

// ----------------------------------------------------------------------------
// information about the currently active plugin

struct carla_bridge_info {
	BinaryType btype = BINARY_NONE;
	PluginType ptype = PLUGIN_NONE;
	uint32_t hints = 0;
	uint32_t options = PLUGIN_OPTIONS_NULL;
	bool hasCV = false;
	uint32_t numAudioIns = 0;
	uint32_t numAudioOuts = 0;
	int64_t uniqueId = 0;
	CarlaString filename;
	CarlaString label;
	CarlaString name;

	void clear()
	{
		btype = BINARY_NONE;
		ptype = PLUGIN_NONE;
		hints = 0;
		options = PLUGIN_OPTIONS_NULL;
		hasCV = false;
		numAudioIns = numAudioOuts = 0;
		uniqueId = 0;
		filename.clear();
		label.clear();
		name.clear();
	}
};

// ----------------------------------------------------------------------------
// bridge callbacks, triggered during carla_bridge::idle()

struct carla_bridge_callback {
	virtual ~carla_bridge_callback(){};
	virtual void bridge_parameter_changed(uint index, float value) = 0;
};

// ----------------------------------------------------------------------------
// bridge implementation

struct carla_bridge {
	carla_bridge_callback *callback = nullptr;

	// cached parameter info
	uint32_t paramCount = 0;
	carla_param_data *paramDetails = nullptr;

	// cached plugin info
	carla_bridge_info info;
	QByteArray chunk;
	std::vector<CustomData> customData;

	~carla_bridge()
	{
		delete[] paramDetails;
		clear_custom_data();
		bfree(lastError);
	}

	// initialize bridge shared memory details
	bool init(uint32_t maxBufferSize, double sampleRate);

	// stop bridge process and cleanup shared memory
	void cleanup(bool clearPluginData = true);

	// start plugin bridge
	bool start(BinaryType btype, PluginType ptype, const char *label,
		   const char *filename, int64_t uniqueId);

	// check if plugin bridge process is running
	// return status might be wrong when called outside the main thread
	bool is_running() const;

	// to be called at regular intervals, from the main thread
	// returns false if bridge process is not running
	bool idle();

	// wait on RT client, making sure it is still active
	// returns true on success
	// NOTE: plugin will be deactivated on next `idle()` if timed out
	bool wait(const char *action, uint msecs);

	// change a plugin parameter value
	void set_value(uint index, float value);

	// show the plugin's custom UI
	void show_ui();

	// [de]activate, a deactivated plugin does not process any audio
	bool is_active() const noexcept;
	void activate();
	void deactivate();

	// reactivate and reload plugin information
	void reload();

	// restore current state from known info, useful when bridge crashes
	void restore_state();

	// process plugin audio
	// frames must be <= `maxBufferSize` as passed during `init`
	void process(float *buffers[MAX_AV_PLANES], uint32_t frames);

	// add or replace custom data (non-parameter plugin values)
	void add_custom_data(const char *type, const char *key,
			     const char *value, bool sendToPlugin = true);

	// inform plugin that all custom data has been loaded
	// required after loading plugin state
	void custom_data_loaded();

	// clear all custom data stored so far
	void clear_custom_data();

	// load plugin state as base64 chunk
	// NOTE: do not save parameter values for plugins using "chunks"
	void load_chunk(const char *b64chunk);

	// request plugin bridge to save and report back its internal state
	// must be called just before saving plugin state
	void save_and_wait();

	// change the maximum expected buffer size
	// plugin is temporarily deactivated during the change
	void set_buffer_size(uint32_t maxBufferSize);

	// get last known error, e.g. reason for last bridge start to fail
	const char *get_last_error() const noexcept;

private:
	bool activated = false;
	bool pendingPing = false;
	bool ready = false;
	bool saved = false;
	bool timedErr = false;
	bool timedOut = false;
	uint32_t bufferSize = 0;
	uint32_t clientBridgeVersion = 0;
	char *lastError = nullptr;

	BridgeAudioPool audiopool;
	BridgeRtClientControl rtClientCtrl;
	BridgeNonRtClientControl nonRtClientCtrl;
	BridgeNonRtServerControl nonRtServerCtrl;

	BridgeProcess *childprocess = nullptr;

	void readMessages();
	void setLastError(const char *error);
};

// ----------------------------------------------------------------------------
