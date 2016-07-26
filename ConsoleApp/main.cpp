/******************************************************************************
	OBS Studio Console
	Copyright(C) 2016  João Portela
	******************************************************************************/

#include<iostream>
#include<string>
#include<memory>

#include<boost/program_options.hpp>

#include "obs.h"
#include "obs.hpp"
#include "util/platform.h"

// default
int monitor_to_record = 0;
std::string encoder_selected = "obs_x264";
std::string output_filepath = "default.mp4";

std::string output_filepath2 = "default2.mp4";
int video_bitrate = 2500;


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

void reset_video() {
	struct obs_video_info ovi;

	ovi.fps_num = 60;
	ovi.fps_den = 1;

	ovi.graphics_module = "libobs-d3d11.dll"; // DL_D3D11
	ovi.base_width = 1920;
	ovi.base_height = 1080;
	ovi.output_width = 1920;
	ovi.output_height = 1080;
	ovi.output_format = VIDEO_FORMAT_NV12;
	ovi.colorspace = VIDEO_CS_601;
	ovi.range = VIDEO_RANGE_PARTIAL;
	ovi.adapter = 0;
	ovi.gpu_conversion = true;
	ovi.scale_type = OBS_SCALE_BICUBIC;

	int ret = obs_reset_video(&ovi);
	if (ret != OBS_VIDEO_SUCCESS) {
		std::cout << "reset_video failed!" << std::endl;
	}
}

void reset_audio() {
	struct obs_audio_info ai;
	ai.samples_per_sec = 44100;
	ai.speakers = SPEAKERS_STEREO;

	bool success = obs_reset_audio(&ai);
	if (!success)
		std::cout << "Audio reset failed!" << std::endl;
}

void setup_input() {
	obs_source_t *source = obs_source_create("monitor_capture", "my capture 1", nullptr, nullptr);

	{
		obs_data_t *source_settings = obs_data_create();
		obs_data_set_int(source_settings, "monitor", monitor_to_record);
		obs_data_set_bool(source_settings, "capture_cursor", false);

		obs_source_update(source, source_settings);

		obs_data_release(source_settings);
	}

	// set this source as output.
	obs_set_output_source(0, source);
}

obs_output_t* setup_output() {
	obs_encoder_t* video_encoder = obs_video_encoder_create(encoder_selected.c_str(), "video_encoder", nullptr, nullptr);
	obs_encoder_set_video(video_encoder, obs_get_video());
	{
		obs_data_t *encoder_settings = obs_data_create();
		obs_data_set_string(encoder_settings, "rate_control", "CBR");
		obs_data_set_int(encoder_settings, "bitrate", video_bitrate);

		obs_encoder_update(video_encoder, encoder_settings);

		obs_data_release(encoder_settings);
	}

	obs_encoder_t* audio_encoder = obs_audio_encoder_create("mf_aac", "audio_encoder", nullptr, 0, nullptr);
	obs_encoder_set_audio(audio_encoder, obs_get_audio());
	{
		obs_data_t *encoder_settings = obs_data_create();
		obs_data_set_string(encoder_settings, "rate_control", "CBR");
		obs_data_set_int(encoder_settings, "samplerate", 44100);
		obs_data_set_int(encoder_settings, "bitrate", 160);
		obs_data_set_default_bool(encoder_settings, "allow he-aac", true);

		obs_encoder_update(audio_encoder, encoder_settings);

		obs_data_release(encoder_settings);
	}

	obs_output_t* file_output = obs_output_create("ffmpeg_muxer", "simple_file_output", nullptr, nullptr);
	{
		obs_data_t* output_settings = obs_data_create();
		obs_data_set_string(output_settings, "path", output_filepath.c_str());
		obs_data_set_string(output_settings, "muxer_settings", NULL);

		obs_output_update(file_output, output_settings);

		obs_data_release(output_settings);
	}

	obs_output_t* file_output2 = obs_output_create("ffmpeg_muxer", "simple_file_output2", nullptr, nullptr);
	{
		obs_data_t* output_settings2 = obs_data_create();
		obs_data_set_string(output_settings2, "path", output_filepath2.c_str());
		obs_data_set_string(output_settings2, "muxer_settings", NULL);

		obs_output_update(file_output2, output_settings2);

		obs_data_release(output_settings2);
	}

	obs_output_set_video_encoder(file_output, video_encoder);
	obs_output_set_video_encoder(file_output2, video_encoder);
	obs_output_set_audio_encoder(file_output, audio_encoder, 0);
	obs_output_set_audio_encoder(file_output2, audio_encoder, 0);

	if (obs_output_start(file_output) && obs_output_start(file_output2)) {
		std::cout << "Recording stuff!!!" << std::endl;
	}
	else {
		std::cout << "Fail!" << std::endl;
	}

	return file_output;
}

int parse_args(int argc, char **argv) {
	namespace po = boost::program_options;

	// Declare the supported options.
	po::options_description desc("\n****************************************************************\nAllowed options");
	desc.add_options()
		("help,h", "produce help message")
		("monitor,m", po::value<int>(), "set monitor to be recorded")
		("encoder,e", po::value<std::string>(), "set encoder")
		("vbitrate,v", po::value<int>(), "set video bitrate. suggested values: 1200 for low, 2500 for medium, 5000 for high")
		("output,o", po::value<std::string>(), "set file destination")
		("secondary_output,s", po::value<std::string>(), "set file destination")
		;

	try {
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";
			//print_obs_enum_input_types();
			print_obs_enum_encoder_types();
			//print_obs_enum_output_types();
			return 1;
		}

		if (vm.count("monitor")) {
			std::cout << "Monitor was set to "
				<< vm["monitor"].as<int>() << ".\n";
			monitor_to_record = vm["monitor"].as<int>();
		}
		else {
			std::cout << "Monitor not set.\n";
			return 2;
		}

		if (vm.count("encoder")) {
			std::cout << "Encoder was set to "
				<< vm["encoder"].as<std::string>() << ".\n";
			encoder_selected = vm["encoder"].as<std::string>();
		}
		else {
			std::cout << "Encoder not set.\n";
			return 3;
		}

		if (vm.count("vbitrate")) {
			std::cout << "Video bitrate was set to "
				<< vm["vbitrate"].as<int>() << ".\n";
			video_bitrate = vm["vbitrate"].as<int>();
		}
		else {
			std::cout << "Encoder not set.\n";
			return 4;
		}

		if (vm.count("output")) {
			std::cout << "Output was set to "
				<< vm["output"].as<std::string>() << ".\n";
			output_filepath = vm["output"].as<std::string>();
		}
		else {
			std::cout << "Output not set.\n";
			return 5;
		}
		if (vm.count("secondary_output")) {
			std::cout << "Secondary output was set to "
				<< vm["secondary_output"].as<std::string>() << ".\n";
			output_filepath2 = vm["secondary_output"].as<std::string>();
		}
		else {
			std::cout << "Secondary output not set.\n";
		}

	}
	catch (po::error& e) {
		std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
		std::cerr << desc << std::endl;
		return 1;
	}

	return 0;
}

int main(int argc, char **argv) {

	// manages object context so that we don't have to call
	// obs_startup and obs_shutdown.
	OBSContext ObsScope("en-US", nullptr, nullptr);

	std::cout << "Obs was "
		<< (obs_initialized() ? "successfully " : "not ")
		<< "initialized." << std::endl;

	reset_video();
	reset_audio();
	obs_load_all_modules();


	//After Initializing OBS so that the help can print available encoders, etc.
	int ret = parse_args(argc, argv);
	if (ret != 0)
		return ret;

	/*
	print_obs_enum_input_types();
	print_obs_enum_encoder_types();
	print_obs_enum_output_types();
	*/

	setup_input();

	obs_output_t* fileOutput = setup_output();

	std::string str;
	std::getline(std::cin, str);

	obs_output_stop(fileOutput);

	std::cout << "-------" << std::endl;

	return 0;
}
