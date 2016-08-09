/******************************************************************************
	Copyright (C) 2016 by João Portela <email@joaoportela.net>

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
	******************************************************************************/

#include<iostream>
#include<string>
#include<memory>

#include<boost/program_options.hpp>

#include<obs.h>
#include<obs.hpp>
#include<util/platform.h>

#include"enum_types.hpp"
#include"monitor_info.hpp"
#include"setup_obs.hpp"

namespace {
	namespace Ret {
		enum ENUM : int {
			success = 0,
			error_in_command_line = 1,
			error_unhandled_exception = 2,
			error_obs = 3
		};
	}

	struct CliOptions {
		// default values
		static const std::string default_encoder;
		static const int default_video_bitrate = 2500;

		// cli options
		int monitor_to_record = 0;
		std::string encoder;
		std::string audio_device;
		int video_bitrate = 2500;
		std::vector<std::string> outputs_paths;
		bool show_help = false;
		bool list_monitors = false;
		bool list_audios = false;
		bool list_encoders = false;
		bool list_inputs = false;
		bool list_outputs = false;
	} cli_options;
	const std::string CliOptions::default_encoder = "obs_x264";
} // namespace

/**
* Resets/Initializes video settings.
*
*   Calls obs_reset_video internally. Assumes some video options.
*/
void reset_video(int monitor_index) {
	struct obs_video_info ovi;

	ovi.fps_num = 60;
	ovi.fps_den = 1;

	MonitorInfo monitor = monitor_at_index(monitor_index);
	ovi.graphics_module = "libobs-d3d11.dll"; // DL_D3D11
	ovi.base_width = monitor.width;
	ovi.base_height = monitor.height;
	ovi.output_width = monitor.width;
	ovi.output_height = monitor.height;
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

/**
* Resets/Initializes video settings.
*
*   Calls obs_reset_audio internally. Assumes some audio options.
*/
void reset_audio() {
	struct obs_audio_info ai;
	ai.samples_per_sec = 44100;
	ai.speakers = SPEAKERS_STEREO;

	bool success = obs_reset_audio(&ai);
	if (!success)
		std::cerr << "Audio reset failed!" << std::endl;
}

/**
* Stop recording of multiple outputs
*
*   Calls obs_output_stop(output) internally.
*/
void stop_recording(std::vector<OBSOutput> outputs){

	for (auto output : outputs) {
		obs_output_stop(output);
	}
}
/**
* Start recording to multiple outputs
*
*   Calls obs_output_start(output) internally.
*/
void start_recording(std::vector<OBSOutput> outputs) {
	int outputs_started = 0;
	for (auto output : outputs) {
		bool success = obs_output_start(output);
		outputs_started += success ? 1 : 0;
	}

	if (outputs_started == outputs.size()) {
		std::cout << "Recording started for all outputs!" << std::endl;
	}
	else {
		size_t outputs_failed = outputs.size() - outputs_started;
		std::cerr << outputs_failed << "/" << outputs.size() << " file outputs are not recording." << std::endl;
	}
}

bool should_print_lists() {
	return cli_options.list_monitors
		|| cli_options.list_audios
		|| cli_options.list_encoders
		|| cli_options.list_inputs
		|| cli_options.list_outputs;
}

bool do_print_lists() {
	if (cli_options.list_monitors) {
		print_monitors_info();
	}
	if (cli_options.list_audios) {
		print_obs_enum_audio_types();
	}
	if (cli_options.list_encoders) {
		print_obs_enum_encoder_types();
	}
	if (cli_options.list_inputs) {
		print_obs_enum_input_types();
	}
	if (cli_options.list_outputs) {
		print_obs_enum_output_types();
	}

	return should_print_lists();
}


int parse_args(int argc, char **argv) {
	namespace po = boost::program_options;

	// Declare the supported options.
	po::options_description desc("obs-cli Allowed options");
	desc.add_options()
		("help,h", "Show help message")
		("listmonitors", "List available monitors resolutions")
		("listinputs", "List available inputs")
		("listencoders", "List available encoders")
		("listoutputs", "List available outputs")
		("listaudios", "List available audios")
		("monitor,m", po::value<int>(&cli_options.monitor_to_record)->required(), "set monitor to be recorded")
		("audio,a", po::value<std::string>(&cli_options.audio_device)->default_value("")->implicit_value("default"), "set audio to be recorded (default to mic) -a\"device_name\" ")
		("encoder,e", po::value<std::string>(&cli_options.encoder)->default_value(CliOptions::default_encoder), "set encoder")
		("vbitrate,v", po::value<int>(&cli_options.video_bitrate)->default_value(CliOptions::default_video_bitrate), "set video bitrate. suggested values: 1200 for low, 2500 for medium, 5000 for high")
		("output,o", po::value<std::vector<std::string>>(&cli_options.outputs_paths)->required(), "set file destination, can be set multiple times for multiple outputs")
		;

	try {
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);

		// must be manually set instead of using po::switch because they are
		// called before po::notify
		cli_options.show_help = vm.count("help") > 0;
		cli_options.list_monitors = vm.count("listmonitors") > 0;
		cli_options.list_inputs = vm.count("listinputs") > 0;
		cli_options.list_audios = vm.count("listaudios") > 0;
		cli_options.list_encoders = vm.count("listencoders") > 0;
		cli_options.list_outputs = vm.count("listoutputs") > 0;

		if (cli_options.show_help) {
			std::cout << desc << "\n";
			return Ret::success;
		}

		if (should_print_lists())
			// will be properly handled later.
			return Ret::success;

		// only called here because it checks required fields, and when are trying to
		// use `help` or `list*` we want to ignore required fields.
		po::notify(vm);
	}
	catch (po::error& e) {
		std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
		std::cerr << desc << std::endl;
		return Ret::error_in_command_line;
	}

	return Ret::success;
}

int main(int argc, char **argv) {
	try {
		int ret = parse_args(argc, argv);
		if (ret != Ret::success)
			return ret;

		if (cli_options.show_help) {
			// We've already printed help. Exit.
			return Ret::success;
		}

		// manages object context so that we don't have to call
		// obs_startup and obs_shutdown.
		OBSContext ObsScope("en-US", nullptr, nullptr);

		if (!obs_initialized()) {
			std::cerr << "Obs initialization failed." << std::endl;
			return Ret::error_obs;
		}

		// must be called before reset_video
		if (!detect_monitors()){
			std::cerr << "No monitors found!" << std::endl;
			return Ret::success;
		}

		// resets must be called before loading modules.
		reset_video(cli_options.monitor_to_record);
		reset_audio();
		obs_load_all_modules();

		// can only be called after loading modules and detecting monitors.
		if (do_print_lists())
			return Ret::success;

		// declared in "main" scope, so that they are not disposed too soon.
		OBSSource video_source, audio_source;

		video_source = setup_video_input(cli_options.monitor_to_record);
		if (!cli_options.audio_device.empty()){
			audio_source = setup_audio_input(cli_options.audio_device);
			if (!audio_source){
				std::cout << "failed to find audio device " << cli_options.audio_device << "." << std::endl;
			}
		}
		// Also declared in "main" scope. While the outputs are kept in scope, we will continue recording.
		Outputs output = setup_outputs(cli_options.encoder, cli_options.video_bitrate, cli_options.outputs_paths);

		start_recording(output.outputs);

		// wait for user input to stop recording.
		std::cout << "press any key to stop recording." << std::endl;
		std::string str;
		std::getline(std::cin, str);
		stop_recording(output.outputs);

		obs_set_output_source(0, nullptr);
		obs_set_output_source(1, nullptr);
		return Ret::success;
	}
	catch (std::exception& e) {
		std::cerr << "Unhandled Exception reached the top of main: "
			<< e.what() << ", application will now exit" << std::endl;
		return Ret::error_unhandled_exception;
	}
}
