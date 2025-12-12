#include <obs.h>
#include <iostream>

// Defined via CMake renaming
extern "C" bool image_source_module_load(void);

int main()
{
	std::cout << "[TestPlugin] Initializing..." << std::endl;

	// Minimal OBS Init (Stubbed in ObsCompat/vr_core usually, but we need the signals to exist)
	if (!obs_startup("en-US", nullptr, nullptr)) {
		std::cerr << "[TestPlugin] obs_startup failed!" << std::endl;
		return 1;
	}

	std::cout << "[TestPlugin] Loading Image Source Module..." << std::endl;
	if (image_source_module_load()) {
		std::cout << "[TestPlugin] Module load returned true." << std::endl;
	} else {
		std::cerr << "[TestPlugin] Module load returned false." << std::endl;
		return 1;
	}

	// Verify Registration
	const char *id = "image_source";
	uint32_t flags = obs_get_source_output_flags(id);
	if (flags != 0) {
		std::cout << "[TestPlugin] SUCCESS: Source '" << id << "' registered with flags " << flags << std::endl;
	} else {
		std::cerr << "[TestPlugin] FAILURE: Source '" << id << "' not found (flags=0)." << std::endl;
		return 1;
	}

	const char *color_id =
		"color_source"; // v1, v2, v3... generic ID lookup might fail if implementation details differ, but usually it registers the string ID.
	// Actually color-source.c registers "color_source" (v1), "color_source_v2", "color_source_v3".
	if (obs_get_source_output_flags("color_source")) {
		std::cout << "[TestPlugin] SUCCESS: Source 'color_source' registered." << std::endl;
	}

	obs_shutdown();
	std::cout << "[TestPlugin] Done." << std::endl;
	return 0;
}
