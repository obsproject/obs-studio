#include <obs.h>
extern "C" {
#include "happy-eyeballs.h"
}
#include <iostream>

int main()
{
	std::cout << "[TestHappyEyeballs] Initializing..." << std::endl;

	struct happy_eyeballs_ctx *ctx = nullptr;
	int res = happy_eyeballs_create(&ctx);

	if (res != 0) {
		std::cerr << "Failed to create context: " << res << std::endl;
		return 1;
	}

	std::cout << "[TestHappyEyeballs] Context created. Attempting connect to localhost:80..." << std::endl;
	// We don't actually expect this to succeed in a test env without a server, but it shouldn't crash.
	// We just test the linking and init logic.

	happy_eyeballs_destroy(ctx);

	std::cout << "[TestHappyEyeballs] Done." << std::endl;
	return 0;
}
