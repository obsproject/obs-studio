/*****************************************************************************
Copyright (C) 2016-2017 by Colin Edwards.
Additional Code Copyright (C) 2016-2017 by c3r1c3 <c3r1c3@nevermindonline.com>

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
*****************************************************************************/

#include <string>
#include <vector>
#include <algorithm>

#include <util/platform.h>
#include <util/dstr.h>

#include "headers/VSTPlugin.h"

#define OPEN_VST_SETTINGS "open_vst_settings"
#define CLOSE_VST_SETTINGS "close_vst_settings"
#define OPEN_WHEN_ACTIVE_VST_SETTINGS "open_when_active_vst_settings"
#define SAVE_VST_TEXT obs_module_text("Save")

#define PLUG_IN_NAME obs_module_text("VstPlugin")
#define OPEN_VST_TEXT obs_module_text("OpenPluginInterface")
#define CLOSE_VST_TEXT obs_module_text("ClosePluginInterface")
#define OPEN_WHEN_ACTIVE_VST_TEXT obs_module_text("OpenInterfaceWhenActive")

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-vst", "en-US")
MODULE_EXPORT const char *obs_module_description(void)
{
	return "VST 2.x Plug-in filter";
}

static void vst_save(void *data, obs_data_t *settings);

static bool open_editor_button_clicked(obs_properties_t *props, obs_property_t *property, void *data)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;

	blog(LOG_WARNING, "VST Plug-in: Open editor btn clicked");

	vstPlugin->openEditor();

	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);

	return true;
}

static bool close_editor_button_clicked(obs_properties_t *props, obs_property_t *property, void *data)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;
	blog(LOG_WARNING, "VST Plug-in: Close editor btn clicked");

	vstPlugin->hideEditor();

	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(props);

	return true;
}

static const char *vst_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return PLUG_IN_NAME;
}

static void vst_destroy(void *data)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;
	vstPlugin->closeEditor();
	vstPlugin->unloadEffect();
	delete vstPlugin;
}

static void vst_update(void *data, obs_data_t *settings)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;

	vstPlugin->setOpenInterfaceWhenActive(obs_data_get_bool(settings, OPEN_WHEN_ACTIVE_VST_SETTINGS));
	const char *path = obs_data_get_string(settings, "plugin_path");

	if (!path || !strcmp(path, ""))
		return;

	bool load_vst = false;

	if (!vstPlugin->isProxyDisconnected())
	{
		// Load VST plugin only when creating the filter or when changing plugin
		if (vstPlugin->getPluginPath() != std::string(path) || vstPlugin->getEffect() == nullptr)
			load_vst = true;
	}

	if (load_vst)
	{
		const bool openWindow = vstPlugin->hasWindowOpen();

		vstPlugin->unloadEffect();
		vstPlugin->loadEffectFromPath(path);

		if (openWindow)
			vstPlugin->openEditor();

		// Load chunk only when creating the filter
		const char *chunkDataBankV3 = obs_data_get_string(settings, "chunk_data_0_v3");
		const char *chunkDataProgramV3 = obs_data_get_string(settings, "chunk_data_1_v3");
		const char *chunkDataPV3 = obs_data_get_string(settings, "chunk_data_p_v3");
		const char *chunkDataPathV3 = obs_data_get_string(settings, "chunk_data_path_v3");

		std::string str_chunkDataBank = "";
		std::string str_chunkDataProgram = "";
		std::string str_chunkDataParameter = "";
		std::string str_chunkDataPath = "";

		if (chunkDataPathV3 != NULL && strlen(chunkDataPathV3) > 0)
		{
			// check if we have v3
			blog(LOG_DEBUG, "VST Plug-in: Got path from v3 chunk data continue loading v3");
			str_chunkDataBank = chunkDataBankV3;
			str_chunkDataProgram = chunkDataProgramV3;
			str_chunkDataParameter = chunkDataPV3;
			str_chunkDataPath = chunkDataPathV3;
		}
		else
		{
			// migrations from 0.27.1 and from 1.0.0-1.0.3
			const char *chunkDataV2 = obs_data_get_string(settings, "chunk_data_v2");
			if (chunkDataV2 != NULL && strlen(chunkDataV2)>0)
			{
				// check if we have v2
				blog(LOG_DEBUG, "VST Plug-in: Got v2 chunk data continue migrating from v2");
				std::string chunkData = chunkDataV2;
				size_t pathPos = chunkData.find_first_of('|');

				if (pathPos == std::string::npos)
				{
					// strange not to have path in v2
					str_chunkDataPath = path;
					str_chunkDataBank = chunkData;
					str_chunkDataParameter = str_chunkDataBank;
				}
				else
				{
					str_chunkDataPath = chunkData.substr(0, pathPos);
					size_t lastPos = chunkData.find_last_of('|');
					str_chunkDataBank = chunkData.substr(lastPos+1);
					str_chunkDataParameter = str_chunkDataBank;
				}

				obs_data_set_string(settings, "chunk_data_v2", "");
			}
			else
			{
				const char *chunkDataOld = obs_data_get_string(settings, "chunk_data");
				blog(LOG_DEBUG, "VST Plug-in: Got old version chunk data continue migrating from old version");

				if (chunkDataOld != NULL && strlen(chunkDataOld)>0)
				{
					// do we have old data
					std::string chunkData = chunkDataOld;
					size_t pathPos = chunkData.find_first_of('|');

					if (pathPos == std::string::npos)
					{
						str_chunkDataPath = path;
						str_chunkDataProgram = chunkData;
						str_chunkDataParameter = str_chunkDataProgram;
					}
					else
					{
						str_chunkDataPath = chunkData.substr(0, pathPos);
						size_t lastPos = chunkData.find_last_of('|');
						str_chunkDataBank = chunkData.substr(lastPos+1);
						str_chunkDataParameter = str_chunkDataBank;
						obs_data_set_string(settings, "chunk_data", "");
					}
				}
				else
				{
					// no data just create empty
				}
			}
		}

		if (str_chunkDataPath.size() > 0)
		{
			vstPlugin->setChunk(VstChunkType::Parameter, str_chunkDataParameter);
			vstPlugin->setChunk(VstChunkType::Program, str_chunkDataProgram);
			vstPlugin->setChunk(VstChunkType::Bank, str_chunkDataBank);
		}
	}	
	
	vst_save(data, settings);
}

static void *vst_create(obs_data_t *settings, obs_source_t *filter)
{
	VSTPlugin *vstPlugin = new VSTPlugin(filter);
	vst_update(vstPlugin, settings);
	return vstPlugin;
}

static void vst_save(void *data, obs_data_t *settings)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;
	
	auto chunk1 = vstPlugin->getChunk(VstChunkType::Bank);
	auto chunk2 = vstPlugin->getChunk(VstChunkType::Program);
	auto chunk3 = vstPlugin->getChunk(VstChunkType::Parameter);

	if (vstPlugin->verifyProxy())
	{
		obs_data_set_string(settings, "chunk_data_0_v3", chunk1.c_str());
		obs_data_set_string(settings, "chunk_data_1_v3", chunk2.c_str());
		obs_data_set_string(settings, "chunk_data_p_v3", chunk3.c_str());
	}

	const char *path = obs_data_get_string(settings, "plugin_path");
	obs_data_set_string(settings, "chunk_data_path_v3", path);
}

static struct obs_audio_data *vst_filter_audio(void *data, struct obs_audio_data *audio)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;
	vstPlugin->process(audio);

	/*
	 * OBS can only guarantee getting the filter source's parent and own name
	 * in this call, so we grab it and return the results for processing
	 * by the EditorWidget.
	 */
	vstPlugin->getSourceNames();

	return audio;
}

bool valid_extension(const char *filepath)
{
	const char *ext          = os_get_path_extension(filepath);
	int         filters_size = 1;

#ifdef __APPLE__
	const char *filters[] = {".vst"};
#elif WIN32
	const char *             filters[] = {".dll"};
#elif __linux__
	const char *filters[] = {".so", ".o"};
	++filters_size;
#endif

	for (int i = 0; i < filters_size; ++i) {
		if (astrcmpi(filters[i], ext) == 0) {
			return true;
		}
	}

	return false;
}

std::vector<std::string> win32_build_dir_list()
{
	const char *program_files_path = getenv("ProgramFiles");

	const char *dir_list[] = {"/Steinberg/VstPlugins/",
	                          "/Common Files/Steinberg/Shared Components/",
	                          "/Common Files/VST2/",
	                          "/Common Files/VSTPlugins/",
	                          "/VSTPlugins/"};

	const int dir_list_size = sizeof(dir_list) / sizeof(dir_list[0]);

	std::vector<std::string> result(dir_list_size, program_files_path);

	for (int i = 0; i < result.size(); ++i) {
		result[i].append(dir_list[i]);
	}

	return result;
}

typedef std::pair<std::string, std::string> vst_data;

static void find_plugins(std::vector<vst_data> &plugin_list, const char *dir_name)
{
	os_dir_t * dir = os_opendir(dir_name);
	os_dirent *ent = os_readdir(dir);

	while (ent != NULL) {
		std::string path(dir_name);

		if (ent->d_name[0] == '.')
			goto next_entry;

		path.append("/");
		path.append(ent->d_name);

		/* If it's a directory, recurse */
		if (ent->directory) {
			find_plugins(plugin_list, path.c_str());
			goto next_entry;
		}

		/* This works well on Apple since we use *.vst
		 * for the extension but not for other platforms.
		 * A Dll dependency will be added even if it's not
		 * an actual VST plugin. Can't do much about it
		 * unfortunately unless everyone suddenly decided to
		 * use a more sane extension. */
		if (valid_extension(ent->d_name)) {
			plugin_list.push_back({ent->d_name, path});
		}

	next_entry:
		ent = os_readdir(dir);
	}

	os_closedir(dir);
}

static void fill_out_plugins(obs_property_t *list)
{
#ifdef __APPLE__
	std::vector<std::string> dir_list({"/Library/Audio/Plug-Ins/VST/", "~/Library/Audio/Plug-ins/VST/"});

#elif WIN32
	std::vector<std::string> dir_list  = win32_build_dir_list();

#elif __linux__
	char *vstPathEnv = getenv("VST_PATH");
	if (vstPathEnv != nullptr) {
		std::string dir_list[] = {vstPathEnv};
	} else {
		/* FIXME: Platform dependent areas.
		   Should use environment variables */
		std::vector<std::string> dir_list({"/usr/lib/vst/",
		                                   "/usr/lib/lxvst/",
		                                   "/usr/lib/linux_vst/",
		                                   "/usr/lib64/vst/",
		                                   "/usr/lib64/lxvst/",
		                                   "/usr/lib64/linux_vst/",
		                                   "/usr/local/lib/vst/",
		                                   "/usr/local/lib/lxvst/",
		                                   "/usr/local/lib/linux_vst/",
		                                   "/usr/local/lib64/vst/",
		                                   "/usr/local/lib64/lxvst/",
		                                   "/usr/local/lib64/linux_vst/",
		                                   "~/.vst/",
		                                   "~/.lxvst/"});
	}
#endif

	std::vector<vst_data> vst_list;

	for (int i = 0; i < dir_list.size(); ++i) {
		find_plugins(vst_list, dir_list[i].c_str());
	}

	obs_property_list_add_string(list, "{Please select a plug-in}", nullptr);
	for (int i = 0; i < vst_list.size(); ++i) {
		obs_property_list_add_string(list, vst_list[i].first.c_str(), vst_list[i].second.c_str());
	}
}

static bool open_btn_changed(obs_properties_t *props, obs_property_t *p, obs_data_t */*settings*/)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)obs_properties_get_param(props);
	if (!vstPlugin) {
		blog(LOG_WARNING, "VST Plug-in:open_btn_changed no VST plugin");
		return true;
	}

	if (vstPlugin->hasWindowOpen()) {
		blog(LOG_WARNING, "VST Plug-in:open_btn_changed has window open");
		obs_property_set_visible(obs_properties_get(props, CLOSE_VST_SETTINGS), true);
	} else {
		blog(LOG_WARNING, "VST Plug-in:open_btn_changed has window closed");
	}
	if (obs_property_is_visible(p) && (vstPlugin->hasWindowOpen() || vstPlugin->isEditorOpen())) {
		obs_property_set_visible(p, false);
		if (!obs_property_is_visible(obs_properties_get(props, CLOSE_VST_SETTINGS))) {
			obs_property_set_visible(obs_properties_get(props, CLOSE_VST_SETTINGS), true);
		}
	}

	return true;
}


static obs_properties_t *vst_properties(void *data)
{
	VSTPlugin *vstPlugin = (VSTPlugin *)data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, vstPlugin, NULL);

	obs_property_t *list = obs_properties_add_list(
	        props, "plugin_path", PLUG_IN_NAME, OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	fill_out_plugins(list);

	obs_property_t *open_button =
	        obs_properties_add_button(props, OPEN_VST_SETTINGS, OPEN_VST_TEXT, open_editor_button_clicked);

	obs_property_t *close_button =
	        obs_properties_add_button(props, CLOSE_VST_SETTINGS, CLOSE_VST_TEXT, close_editor_button_clicked);

	obs_property_set_visible(open_button, true);
	obs_property_set_visible(close_button, false);

	obs_property_set_modified_callback(open_button, open_btn_changed);

	obs_properties_add_bool(props, OPEN_WHEN_ACTIVE_VST_SETTINGS, OPEN_WHEN_ACTIVE_VST_TEXT);

	UNUSED_PARAMETER(data);

	return props;
}

bool obs_module_load(void)
{
	struct obs_source_info vst_filter = {};
	vst_filter.id                     = "vst_filter";
	vst_filter.type                   = OBS_SOURCE_TYPE_FILTER;
	vst_filter.output_flags           = OBS_SOURCE_AUDIO;
	vst_filter.get_name               = vst_name;
	vst_filter.create                 = vst_create;
	vst_filter.destroy                = vst_destroy;
	vst_filter.update                 = vst_update;
	vst_filter.filter_audio           = vst_filter_audio;
	vst_filter.get_properties         = vst_properties;
	vst_filter.save                   = vst_save;

	obs_register_source(&vst_filter);
	return true;
}
