/*
 * Carla plugin for OBS
 * Copyright (C) 2023 Filipe Coelho <falktx@falktx.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// needed for strcasestr
#if !defined(_GNU_SOURCE) && !defined(_WIN32)
#define _GNU_SOURCE
#endif

#include <CarlaBackendUtils.hpp>
#include <CarlaBinaryUtils.hpp>

#include <QtCore/QFileInfo>
#include <QtCore/QString>

#include <util/platform.h>

#include "carla-bridge.hpp"
#include "carla-wrapper.h"
#include "common.h"
#include "qtutils.h"

#if CARLA_VERSION_HEX >= 0x020591
#define CARLA_2_6_FEATURES
#endif

// ----------------------------------------------------------------------------
// private data methods

struct carla_priv : carla_bridge_callback {
	obs_source_t *source = nullptr;
	uint32_t bufferSize = 0;
	double sampleRate = 0;

	// update properties when timeout is reached, 0 means do nothing
	uint64_t update_request = 0;

	carla_bridge bridge;

	void bridge_parameter_changed(uint index, float value) override
	{
		char pname[PARAM_NAME_SIZE] = PARAM_NAME_INIT;
		param_index_to_name(index, pname);

		obs_data_t *settings = obs_source_get_settings(source);

		/**/ if (bridge.paramDetails[index].hints &
			 PARAMETER_IS_BOOLEAN)
			obs_data_set_bool(settings, pname, value > 0.5f);
		else if (bridge.paramDetails[index].hints &
			 PARAMETER_IS_INTEGER)
			obs_data_set_int(settings, pname, value);
		else
			obs_data_set_double(settings, pname, value);

		obs_data_release(settings);

		postpone_update_request(&update_request);
	}
};

// ----------------------------------------------------------------------------
// carla + obs integration methods

struct carla_priv *carla_priv_create(obs_source_t *source,
				     enum buffer_size_mode bufsize,
				     uint32_t srate)
{
	struct carla_priv *priv = new struct carla_priv;
	if (priv == NULL)
		return NULL;

	priv->bridge.callback = priv;
	priv->source = source;
	priv->bufferSize = bufsize_mode_to_frames(bufsize);
	priv->sampleRate = srate;

	return priv;
}

void carla_priv_destroy(struct carla_priv *priv)
{
	priv->bridge.cleanup();
	delete priv;
}

// ----------------------------------------------------------------------------

void carla_priv_activate(struct carla_priv *priv)
{
	priv->bridge.activate();
}

void carla_priv_deactivate(struct carla_priv *priv)
{
	priv->bridge.deactivate();
}

void carla_priv_process_audio(struct carla_priv *priv,
			      float *buffers[MAX_AV_PLANES], uint32_t frames)
{
	priv->bridge.process(buffers, frames);
}

void carla_priv_idle(struct carla_priv *priv)
{
	priv->bridge.idle();
	handle_update_request(priv->source, &priv->update_request);
}

// ----------------------------------------------------------------------------

void carla_priv_save(struct carla_priv *priv, obs_data_t *settings)
{
	priv->bridge.save_and_wait();

	obs_data_set_string(settings, "btype",
			    getBinaryTypeAsString(priv->bridge.info.btype));
	obs_data_set_string(settings, "ptype",
			    getPluginTypeAsString(priv->bridge.info.ptype));
	obs_data_set_string(settings, "filename", priv->bridge.info.filename);
	obs_data_set_string(settings, "label", priv->bridge.info.label);
	obs_data_set_int(settings, "uniqueId",
			 static_cast<long long>(priv->bridge.info.uniqueId));

	if (!priv->bridge.customData.empty()) {
		obs_data_array_t *array = obs_data_array_create();

		for (CustomData &cdata : priv->bridge.customData) {
			obs_data_t *data = obs_data_create();
			obs_data_set_string(data, "type", cdata.type);
			obs_data_set_string(data, "key", cdata.key);
			obs_data_set_string(data, "value", cdata.value);
			obs_data_array_push_back(array, data);
			obs_data_release(data);
		}

		obs_data_set_array(settings, PROP_CUSTOM_DATA, array);
		obs_data_array_release(array);
	} else {
		obs_data_erase(settings, PROP_CUSTOM_DATA);
	}

	char pname[PARAM_NAME_SIZE] = PARAM_NAME_INIT;

	if ((priv->bridge.info.options & PLUGIN_OPTION_USE_CHUNKS) &&
	    !priv->bridge.chunk.isEmpty()) {
		char *b64ptr = CarlaString::asBase64(priv->bridge.chunk.data(),
						     priv->bridge.chunk.size())
				       .releaseBufferPointer();
		const CarlaString b64chunk(b64ptr, false);
		obs_data_set_string(settings, PROP_CHUNK, b64chunk.buffer());

		for (uint32_t i = 0;
		     i < priv->bridge.paramCount && i < MAX_PARAMS; ++i) {
			const carla_param_data &param(
				priv->bridge.paramDetails[i]);

			if ((param.hints & PARAMETER_IS_ENABLED) == 0)
				continue;

			param_index_to_name(i, pname);
			obs_data_erase(settings, pname);
		}
	} else {
		obs_data_erase(settings, PROP_CHUNK);

		for (uint32_t i = 0;
		     i < priv->bridge.paramCount && i < MAX_PARAMS; ++i) {
			const carla_param_data &param(
				priv->bridge.paramDetails[i]);

			if ((param.hints & PARAMETER_IS_ENABLED) == 0)
				continue;

			param_index_to_name(i, pname);

			if (param.hints & PARAMETER_IS_BOOLEAN) {
				obs_data_set_bool(settings, pname,
						  carla_isEqual(param.value,
								param.max));
			} else if (param.hints & PARAMETER_IS_INTEGER) {
				obs_data_set_int(settings, pname, param.value);
			} else {
				obs_data_set_double(settings, pname,
						    param.value);
			}
		}
	}
}

void carla_priv_load(struct carla_priv *priv, obs_data_t *settings)
{
	const BinaryType btype =
		getBinaryTypeFromString(obs_data_get_string(settings, "btype"));
	const PluginType ptype =
		getPluginTypeFromString(obs_data_get_string(settings, "ptype"));

	// abort early if both of these are null, likely from an empty config
	if (btype == BINARY_NONE && ptype == PLUGIN_NONE)
		return;

	const char *const filename = obs_data_get_string(settings, "filename");
	const char *const label = obs_data_get_string(settings, "label");
	const int64_t uniqueId =
		static_cast<int64_t>(obs_data_get_int(settings, "uniqueId"));

	priv->bridge.cleanup();
	priv->bridge.init(priv->bufferSize, priv->sampleRate);

	if (!priv->bridge.start(btype, ptype, label, filename, uniqueId)) {
		carla_show_error_dialog("Failed to load plugin",
					priv->bridge.get_last_error());
		return;
	}

	obs_data_array_t *array =
		obs_data_get_array(settings, PROP_CUSTOM_DATA);
	if (array) {
		const size_t count = obs_data_array_count(array);
		for (size_t i = 0; i < count; ++i) {
			obs_data_t *data = obs_data_array_item(array, i);
			const char *type = obs_data_get_string(data, "type");
			const char *key = obs_data_get_string(data, "key");
			const char *value = obs_data_get_string(data, "value");
			priv->bridge.add_custom_data(type, key, value);
		}
		priv->bridge.custom_data_loaded();
	}

	if (priv->bridge.info.options & PLUGIN_OPTION_USE_CHUNKS) {
		const char *b64chunk =
			obs_data_get_string(settings, PROP_CHUNK);
		priv->bridge.load_chunk(b64chunk);
	} else {
		for (uint32_t i = 0; i < priv->bridge.paramCount; ++i) {
			const carla_param_data &param(
				priv->bridge.paramDetails[i]);

			priv->bridge.set_value(i, param.value);
		}
	}

	char pname[PARAM_NAME_SIZE] = PARAM_NAME_INIT;

	for (uint32_t i = 0; i < priv->bridge.paramCount && i < MAX_PARAMS;
	     ++i) {
		const carla_param_data &param(priv->bridge.paramDetails[i]);

		if ((param.hints & PARAMETER_IS_ENABLED) == 0)
			continue;

		param_index_to_name(i, pname);

		if (param.hints & PARAMETER_IS_BOOLEAN) {
			obs_data_set_bool(settings, pname,
					  carla_isEqual(param.value,
							param.max));
		} else if (param.hints & PARAMETER_IS_INTEGER) {
			obs_data_set_int(settings, pname, param.value);
		} else {
			obs_data_set_double(settings, pname, param.value);
		}
	}

	priv->bridge.activate();
}

// ----------------------------------------------------------------------------

uint32_t carla_priv_get_num_channels(struct carla_priv *priv)
{
	return std::max(priv->bridge.info.numAudioIns,
			priv->bridge.info.numAudioOuts);
}

void carla_priv_set_buffer_size(struct carla_priv *priv,
				enum buffer_size_mode bufsize)
{
	priv->bridge.set_buffer_size(bufsize_mode_to_frames(bufsize));
}

// ----------------------------------------------------------------------------

static bool carla_post_load_callback(struct carla_priv *priv,
				     obs_properties_t *props)
{
	obs_source_t *source = priv->source;
	obs_data_t *settings = obs_source_get_settings(source);
	remove_all_props(props, settings);
	carla_priv_readd_properties(priv, props, true);
	obs_data_release(settings);
	return true;
}

static bool carla_priv_load_file_callback(obs_properties_t *props,
					  obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(property);

	struct carla_priv *priv = static_cast<struct carla_priv *>(data);

	char *filename = carla_qt_file_dialog(
		false, false, obs_module_text("Load File"), NULL);

	if (filename == NULL)
		return false;

#ifndef _WIN32
	// truncate plug.vst3/Contents/<plat>/plug.so -> plug.vst3
	if (char *const vst3str = strcasestr(filename, ".vst3/Contents/"))
		vst3str[5] = '\0';
#endif

	BinaryType btype;
	PluginType ptype;

	{
		const QFileInfo fileInfo(QString::fromUtf8(filename));
		const QString extension(fileInfo.suffix());

#if defined(CARLA_OS_MAC)
		if (extension == "vst")
			ptype = PLUGIN_VST2;
#else
		if (extension == "dll" || extension == "so")
			ptype = PLUGIN_VST2;
#endif
		else if (extension == "vst3")
			ptype = PLUGIN_VST3;
#ifdef CARLA_2_6_FEATURES
		else if (extension == "clap")
			ptype = PLUGIN_CLAP;
#endif
		else {
			carla_show_error_dialog("Failed to load file",
						"Unknown file type");
			return false;
		}

		btype = getBinaryTypeFromFile(filename);
	}

	priv->bridge.cleanup();
	priv->bridge.init(priv->bufferSize, priv->sampleRate);

	if (priv->bridge.start(btype, ptype, "(none)", filename, 0))
		priv->bridge.activate();
	else
		carla_show_error_dialog("Failed to load file",
					priv->bridge.get_last_error());

	return carla_post_load_callback(priv, props);
}

static bool carla_priv_select_plugin_callback(obs_properties_t *props,
					      obs_property_t *property,
					      void *data)
{
	UNUSED_PARAMETER(property);

	struct carla_priv *priv = static_cast<struct carla_priv *>(data);

	const PluginListDialogResults *plugin = carla_exec_plugin_list_dialog();

	if (plugin == NULL)
		return false;

	priv->bridge.cleanup();
	priv->bridge.init(priv->bufferSize, priv->sampleRate);

	if (priv->bridge.start(static_cast<BinaryType>(plugin->build),
			       static_cast<PluginType>(plugin->type),
			       plugin->label, plugin->filename,
			       plugin->uniqueId))
		priv->bridge.activate();
	else
		carla_show_error_dialog("Failed to load plugin",
					priv->bridge.get_last_error());

	return carla_post_load_callback(priv, props);
}

static bool carla_priv_reload_callback(obs_properties_t *props,
				       obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(property);

	struct carla_priv *priv = static_cast<struct carla_priv *>(data);

	if (priv->bridge.is_running()) {
		priv->bridge.reload();
		return true;
	}

	if (priv->bridge.info.btype == BINARY_NONE)
		return false;

	// cache relevant information for later
	const BinaryType btype = priv->bridge.info.btype;
	const PluginType ptype = priv->bridge.info.ptype;
	const int64_t uniqueId = priv->bridge.info.uniqueId;
	char *const label = priv->bridge.info.label.releaseBufferPointer();
	char *const filename =
		priv->bridge.info.filename.releaseBufferPointer();

	priv->bridge.cleanup(false);
	priv->bridge.init(priv->bufferSize, priv->sampleRate);

	if (priv->bridge.start(btype, ptype, label, filename, uniqueId)) {
		priv->bridge.restore_state();
		priv->bridge.activate();
	} else {
		carla_show_error_dialog("Failed to reload plugin",
					priv->bridge.get_last_error());
	}

	std::free(label);
	std::free(filename);

	return carla_post_load_callback(priv, props);
}

static bool carla_priv_show_gui_callback(obs_properties_t *props,
					 obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);

	struct carla_priv *priv = static_cast<struct carla_priv *>(data);

	priv->bridge.show_ui();

	return false;
}

static bool carla_priv_param_changed(void *data, obs_properties_t *props,
				     obs_property_t *property,
				     obs_data_t *settings)
{
	UNUSED_PARAMETER(props);

	struct carla_priv *priv = static_cast<struct carla_priv *>(data);

	const char *const pname = obs_property_name(property);
	if (pname == NULL)
		return false;

	const char *pname2 = pname + 1;
	while (*pname2 == '0')
		++pname2;

	const int pindex = atoi(pname2);

	if (pindex < 0 || pindex >= (int)priv->bridge.paramCount)
		return false;

	const uint index = static_cast<uint>(pindex);

	const float min = priv->bridge.paramDetails[index].min;
	const float max = priv->bridge.paramDetails[index].max;

	float value;
	switch (obs_property_get_type(property)) {
	case OBS_PROPERTY_BOOL:
		value = obs_data_get_bool(settings, pname) ? max : min;
		break;
	case OBS_PROPERTY_INT:
		value = obs_data_get_int(settings, pname);
		if (value < min)
			value = min;
		else if (value > max)
			value = max;
		break;
	case OBS_PROPERTY_FLOAT:
		value = obs_data_get_double(settings, pname);
		if (value < min)
			value = min;
		else if (value > max)
			value = max;
		break;
	default:
		return false;
	}

	priv->bridge.set_value(index, value);

	return false;
}

void carla_priv_readd_properties(struct carla_priv *priv,
				 obs_properties_t *props, bool reset)
{
	if (!reset) {
		obs_properties_add_button2(props, PROP_SELECT_PLUGIN,
					   obs_module_text("Select plugin..."),
					   carla_priv_select_plugin_callback,
					   priv);

		obs_properties_add_button2(props, PROP_LOAD_FILE,
					   obs_module_text("Load file..."),
					   carla_priv_load_file_callback, priv);
	}

	if (priv->bridge.info.ptype != PLUGIN_NONE)
		obs_properties_add_button2(props, PROP_RELOAD_PLUGIN,
					   obs_module_text("Reload"),
					   carla_priv_reload_callback, priv);

	if (priv->bridge.info.hints & PLUGIN_HAS_CUSTOM_UI)
		obs_properties_add_button2(props, PROP_SHOW_GUI,
					   obs_module_text("Show custom GUI"),
					   carla_priv_show_gui_callback, priv);

	obs_data_t *settings = obs_source_get_settings(priv->source);

	char pname[PARAM_NAME_SIZE] = PARAM_NAME_INIT;

	for (uint32_t i = 0; i < priv->bridge.paramCount && i < MAX_PARAMS;
	     ++i) {
		const carla_param_data &param(priv->bridge.paramDetails[i]);

		if ((param.hints & PARAMETER_IS_ENABLED) == 0)
			continue;

		obs_property_t *prop;
		param_index_to_name(i, pname);

		if (param.hints & PARAMETER_IS_BOOLEAN) {
			prop = obs_properties_add_bool(props, pname,
						       param.name);

			obs_data_set_default_bool(settings, pname,
						  carla_isEqual(param.def,
								param.max));

			if (reset)
				obs_data_set_bool(settings, pname,
						  carla_isEqual(param.value,
								param.max));
		} else if (param.hints & PARAMETER_IS_INTEGER) {
			prop = obs_properties_add_int_slider(
				props, pname, param.name, param.min, param.max,
				param.step);

			obs_data_set_default_int(settings, pname, param.def);

			if (param.unit.isNotEmpty())
				obs_property_int_set_suffix(prop, param.unit);

			if (reset)
				obs_data_set_int(settings, pname, param.value);
		} else {
			prop = obs_properties_add_float_slider(
				props, pname, param.name, param.min, param.max,
				param.step);

			obs_data_set_default_double(settings, pname, param.def);

			if (param.unit.isNotEmpty())
				obs_property_float_set_suffix(prop, param.unit);

			if (reset)
				obs_data_set_double(settings, pname,
						    param.value);
		}

		obs_property_set_modified_callback2(
			prop, carla_priv_param_changed, priv);
	}

	obs_data_release(settings);
}

// ----------------------------------------------------------------------------
