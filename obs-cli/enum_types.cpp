#include "enum_types.hpp"

#include<iostream>

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

void print_obs_enum_encoder_types() {
	const char *type;
	bool foundValues = false;
	size_t idx = 0;

	std::cout << "Encoder types:" << std::endl;

	while (obs_enum_encoder_types(idx++, &type)) {
		const char *name = obs_encoder_get_display_name(type);
		const char *codec = obs_get_encoder_codec(type);

		std::cout << type << "\t" << name << "\t" << codec << std::endl;
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
void print_obs_enum_audio_types(){

	const char *type;
	bool foundValues = false;
	size_t idx = 0;

	obs_properties_t* props = obs_get_source_properties(INPUT_AUDIO_SOURCE);


	props = obs_get_source_properties(OUTPUT_AUDIO_SOURCE);

	////obs_data_item_get_array(source_settings, "device_id", "default");
	//std::cout << "Audio types:" << std::endl;

	//while (obs_enum_source_types(idx++, &type)) {
	//	const char *name = obs_encoder_get_display_name(type);
	//	const char *codec = obs_get_encoder_codec(type);

	//	std::cout << type << "\t" << name << "\t" << codec << std::endl;
	//	foundValues = true;
	//}

	//if (!foundValues) {
	//	std::cout << "Not Found!" << std::endl;
	//}
	//std::cout << "#############" << std::endl;
}
