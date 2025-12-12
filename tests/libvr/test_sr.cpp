#include "libvr/ISuperResolutionAdapter.h"
#include <iostream>

extern "C" ISuperResolutionAdapter *CreateSRAdapter();

int main()
{
	std::cout << "[TestSR] Creating SuperResolution Adapter..." << std::endl;
	ISuperResolutionAdapter *adapter = CreateSRAdapter();

	if (!adapter) {
		std::cerr << "[TestSR] Failed to create adapter." << std::endl;
		return 1;
	}

	std::cout << "[TestSR] Initializing (Expect VFX SDK load)..." << std::endl;
	// Initialize with dummy values, actual logic depends on GPU context which might not be standalone here
	// But it should at least try to load the library and symbols.
	adapter->vtbl->Initialize(adapter, 1920, 1080);

	std::cout << "[TestSR] Shutdown..." << std::endl;
	adapter->vtbl->Shutdown(adapter);

	// We don't have a Destroy function exposed in C interface yet (leak for test)
	// In real app, we'd have DestroySRAdapter() or similar.

	std::cout << "[TestSR] Complete." << std::endl;
	return 0;
}
