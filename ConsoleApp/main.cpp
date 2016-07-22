#include<iostream>
#include<memory>

#include "obs.h"
#include "obs.hpp"
#include "util/platform.h"

struct AddSourceData {
	obs_source_t *source;
	bool visible;
};

struct obs_data {
	volatile long        ref;
	char                 *json;
	struct obs_data_item *first_item;
};

struct obs_data_item {
	volatile long        ref;
	struct obs_data      *parent;
	struct obs_data_item *next;
	enum obs_data_type   type;
	size_t               name_len;
	size_t               data_len;
	size_t               data_size;
	size_t               default_len;
	size_t               default_size;
	size_t               autoselect_size;
	size_t               capacity;
};

void print_obs_enum_input_types() {
	const char *type;
	bool foundValues = false;
	size_t idx = 0;

	std::cout << "Input types:" << std::endl;

	while (obs_enum_input_types(idx++, &type)) {
		const char *name = obs_source_get_display_name(type);

		std::cout << type << "\t" << name << std::endl;
		foundValues = true;
	}

	if (!foundValues) {
		std::cout << "Not Found!" << std::endl;
	}
	std::cout << "#############" << std::endl;
}

void print_obs_enum_output_types() {
	const char *type;
	bool foundValues = false;
	size_t idx = 0;

	std::cout << "Output types:" << std::endl;

	while (obs_enum_output_types(idx++, &type)) {
		const char *name = obs_output_get_display_name(type);

		std::cout << type << "\t" << name << std::endl;
		foundValues = true;
	}

	if (!foundValues) {
		std::cout << "Not Found!" << std::endl;
	}
	std::cout << "#############" << std::endl;
}

bool printme(void *nulldata, obs_source_t *source)
{
	const char *name = obs_source_get_name(source);
	const char *id = obs_source_get_id(source);

	if (strcmp(id, "monitor_capture") == 0)
		std::cout << "found it!" << std::endl;

	return true;
}

obs_output_t* load_output() {
	obs_output_t* fileOutput = obs_output_create("ffmpeg_output",
		"simple_ffmpeg_output", nullptr, nullptr);
	if (!fileOutput)
		throw "Failed to create recording FFmpeg output "
		"(simple output)";
	//obs_output_release(fileOutput);

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "format_name", "avi");
	obs_data_set_string(settings, "video_encoder", "utvideo");
	obs_data_set_string(settings, "audio_encoder", "pcm_s16le");

	obs_output_update(fileOutput, settings);
	// obs_data_release(settings);
}

int main() {
	// manages object context so that we don't have to call
	// obs_startup and obs_shutdown.
	OBSContext ObsScope("en-US", nullptr, nullptr);

	std::cout << "Obs was "
		<< (obs_initialized() ? "successfully " : "not ")
		<< "initialized." << std::endl;

	/*const char *sceneCollection = config_get_string(App()->GlobalConfig(),
		"Basic", "SceneCollectionFile");*/

	obs_load_all_modules();

	print_obs_enum_input_types();

	print_obs_enum_output_types();

	obs_scene_t* scene1 = obs_scene_create("Scene 1");

	obs_source_t *source = obs_source_create("monitor_capture", "capture 1", nullptr, nullptr);

	if (source) {
		AddSourceData data;
		data.source = source;
		data.visible = true;

		obs_sceneitem_t* sceneitem = obs_scene_add(scene1, data.source);
		obs_sceneitem_set_visible(sceneitem, data.visible);

		obs_data_t* settings = obs_source_get_settings(source);

		//obs_data_t *settings = obs_data_create();


		std::cout << ":)" << std::endl;
	}

	obs_output_t *output = load_output();
	obs_output_start(output);

	std::cout << "-----" << std::endl;

	return 0;
}
