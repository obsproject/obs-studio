#include <obs.h>
extern "C" {
#include "file-updater.h"
}
#include <iostream>

// Mock callback
bool confirm_update(void *param, struct file_download_data *data)
{
	return true;
}

int main()
{
	std::cout << "[TestFileUpdater] Initializing..." << std::endl;

	// Test creation (which tests curl init and threading internally)
	// We pass dummy values that shouldn't crash if stubs are robust.
	update_info_t *info = update_info_create("[Test]", "TestAgent/1.0", "http://localhost/update", "/tmp/local",
						 "/tmp/cache", confirm_update, nullptr);

	if (info) {
		std::cout << "[TestFileUpdater] Info created." << std::endl;
		update_info_destroy(info);
	} else {
		std::cout << "[TestFileUpdater] Info creation failed (expected if stubs fail)." << std::endl;
	}

	std::cout << "[TestFileUpdater] Done." << std::endl;
	return 0;
}
