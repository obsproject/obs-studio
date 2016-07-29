#include "setup_obs.hpp"

#include<boost/algorithm/string/predicate.hpp>

#ifdef __APPLE__
#define INPUT_AUDIO_SOURCE  "coreaudio_input_capture"
#define OUTPUT_AUDIO_SOURCE "coreaudio_output_capture"
#elif _WIN32
#define INPUT_AUDIO_SOURCE  "wasapi_input_capture"
#define OUTPUT_AUDIO_SOURCE "wasapi_output_capture"
#else
#define INPUT_AUDIO_SOURCE  "pulse_input_capture"
#define OUTPUT_AUDIO_SOURCE "pulse_output_capture"
#endif

OBSSource setup_video_input(int monitor) {
	std::string name("monitor " + std::to_string(monitor) + " capture");
	OBSSource source = obs_source_create("monitor_capture", name.c_str(), nullptr, nullptr);
	obs_source_release(source);
	{
		obs_data_t * source_settings = obs_data_create();
		obs_data_set_int(source_settings, "monitor", monitor);
		obs_data_set_bool(source_settings, "capture_cursor", false);

		obs_source_update(source, source_settings);
		obs_data_release(source_settings);
	}

	// set this source as output.
	obs_set_output_source(0, source);

	return source;
}
std::string search_audio_device_by_type(std::string* audio_device, std::string* device_type){

	std::string device_id;
	obs_properties_t* props = obs_get_source_properties(device_type->c_str());

	obs_property_t *property = obs_properties_first(props);

	while (property){
		const char        *name = obs_property_name(property);
		//only check device_id properties
		if (strcmp(name, "device_id") != 0)
			break;

		obs_property_type type = obs_property_get_type(property);
		obs_combo_format cformat = obs_property_list_format(property);
		size_t           ccount = obs_property_list_item_count(property);

		switch (type)
		{
		case OBS_PROPERTY_LIST:

			for (size_t cidx = 0; cidx < ccount; cidx++){
				const char *nameListItem = obs_property_list_item_name(property, cidx);

				if (cformat == OBS_COMBO_FORMAT_STRING){
					if (boost::iequals(nameListItem, audio_device->c_str())) {
						device_id = obs_property_list_item_string(property, cidx);
						return device_id;
					}
				}
			}

			break;
		default:
			break;
		}
		obs_property_next(&property);
	}
	return device_id;
}

void search_audio_device_by_name(std::string audio_device, std::string* device_id, std::string* device_type){
	//std::string device_id;
	std::string st = std::string(INPUT_AUDIO_SOURCE);
	*device_type = st;
	*device_id = search_audio_device_by_type(&audio_device, device_type);

	//device has been found exit
	if (!device_id->empty())
		return;

	*device_type = std::string(OUTPUT_AUDIO_SOURCE);
	*device_id = search_audio_device_by_type(&audio_device, device_type);
	return;
}
OBSSource setup_audio_input(std::string audio_device) {

	bool  isOutput;
	std::string device_id;
	std::string device_type;

	//TODO RP
	//this sets audio device..it shouldn't 
	search_audio_device_by_name(audio_device, &device_id, &device_type);
	if (device_id.empty())
		return nullptr;

	OBSSource source = obs_source_create(device_type.c_str(), "audio capture", nullptr, nullptr);
	obs_source_release(source);
	{
		obs_data_t * source_settings = obs_data_create();

		//TODO RP
		//obs_data_set_string(source_settings, "device_id", device_id.c_str());

		obs_source_update(source, source_settings);
		obs_data_release(source_settings);
	}

	// set this source as output.
	obs_set_output_source(1, source);

	return source;
}

Outputs setup_outputs(std::string video_encoder_id, int video_bitrate, std::vector<std::string> output_paths) {
	OBSEncoder video_encoder = obs_video_encoder_create(video_encoder_id.c_str(), "video_encoder", nullptr, nullptr);
	obs_encoder_release(video_encoder);
	obs_encoder_set_video(video_encoder, obs_get_video());
	{
		obs_data_t * encoder_settings = obs_data_create();
		obs_data_set_string(encoder_settings, "rate_control", "CBR");
		obs_data_set_int(encoder_settings, "bitrate", video_bitrate);

		obs_encoder_update(video_encoder, encoder_settings);
		obs_data_release(encoder_settings);
	}

	OBSEncoder audio_encoder = obs_audio_encoder_create("mf_aac", "audio_encoder", nullptr, 0, nullptr);
	obs_encoder_release(audio_encoder);
	obs_encoder_set_audio(audio_encoder, obs_get_audio());
	{
		obs_data_t * encoder_settings = obs_data_create();
		obs_data_set_string(encoder_settings, "rate_control", "CBR");
		obs_data_set_int(encoder_settings, "samplerate", 44100);
		obs_data_set_int(encoder_settings, "bitrate", 160);
		obs_data_set_default_bool(encoder_settings, "allow he-aac", true);

		obs_encoder_update(audio_encoder, encoder_settings);
		obs_data_release(encoder_settings);
	}


	std::vector<OBSOutput> outputs;
	for (int i = 0; i < output_paths.size(); i++) {
		auto output_path = output_paths[i];
		std::string name("file_output_" + std::to_string(i));
		OBSOutput file_output = obs_output_create("ffmpeg_muxer", name.c_str(), nullptr, nullptr);
		obs_output_release(file_output);
		{
			obs_data_t * output_settings = obs_data_create();
			obs_data_set_string(output_settings, "path", output_path.c_str());
			obs_data_set_string(output_settings, "muxer_settings", NULL);

			obs_output_update(file_output, output_settings);
			obs_data_release(output_settings);
		}

		obs_output_set_video_encoder(file_output, video_encoder);
		obs_output_set_audio_encoder(file_output, audio_encoder, 0);

		outputs.push_back(file_output);
	}

	Outputs out;
	out.video_encoder = video_encoder;
	out.audio_encoder = audio_encoder;
	out.outputs = outputs;
	return out;
}

