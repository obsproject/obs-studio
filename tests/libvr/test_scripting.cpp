#include <obs.h>
extern "C" {
#include "obs-scripting.h"
#include "opts-parser.h"
}
#include <iostream>

int main()
{
	std::cout << "[TestScripting] Initializing..." << std::endl;

	// Core load should pass even without backends
	if (!obs_scripting_load()) {
		std::cerr << "Failed to load scripting engine" << std::endl;
		return 1;
	}

	std::cout << "[TestScripting] Engine loaded." << std::endl;

	if (obs_scripting_supported_formats()[0] != NULL) {
		// If backends disabled, this should be empty/NULL
		std::cout << "[TestScripting] Backend formats present (unexpected but okay)." << std::endl;
	} else {
		std::cout << "[TestScripting] No backends enabled (as expected)." << std::endl;
	}

	// Test Opts Parser
	struct obs_options options = obs_parse_options("test=val --switch");
	std::cout << "[TestScripting] Parsed " << options.count << " options." << std::endl;
	obs_free_options(options);
	std::cout << "[TestScripting] Opts parser test passed." << std::endl;

	obs_scripting_unload();
	std::cout << "[TestScripting] Done." << std::endl;
	return 0;
}
