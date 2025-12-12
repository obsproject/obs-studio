#include <iostream>
extern "C" {
#include "obs.h"
#include "obs-module.h"

// Forward declaration from linux-alsa.c (renamed via macro)
bool linux_alsa_module_load(void);
}

int main()
{
	std::cout << "[TestPlugin] Initializing..." << std::endl;
	if (!obs_startup("en-US", nullptr, nullptr)) {
		std::cerr << "[TestPlugin] Failed to startup OBS" << std::endl;
		return 1;
	}

	std::cout << "[TestPlugin] Loading ALSA Module..." << std::endl;
	// We only call this if ALSA was found and compiled in.
	// If not, we might get a linker error if we just call it unconditionally.
	// Ideally we'd use a weak symbol or check compile definition.
	// But for this test, we assume ALSA is present.

	// If we are strictly checking CMake output, we should wrap this test in CMake-time check.
	// But let's try to call it.

	// Note: If linux-alsa.c wasn't compiled, this function won't exist.
	// But we are creating this file NOW, so we can't check CMake yet.
	// We'll rely on CMake to only build this test if ALSA is found.

	bool result = linux_alsa_module_load();
	std::cout << "[TestPlugin] Module load returned " << (result ? "true" : "false") << "." << std::endl;

	// Verify source registration
	uint32_t flags = obs_get_source_output_flags("alsa_input_capture");
	if (flags & OBS_SOURCE_AUDIO) {
		std::cout << "[TestPlugin] SUCCESS: Source 'alsa_input_capture' registered." << std::endl;
		return 0;
	} else {
		std::cerr << "[TestPlugin] FAILURE: Source 'alsa_input_capture' not found." << std::endl;
		return 1;
	}
}
