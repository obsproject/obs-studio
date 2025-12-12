#include <obs.h>
extern "C" {
#include "pipe.h"
}
#include <iostream>

int main()
{
	std::cout << "[TestIPC] Initializing..." << std::endl;

	struct ipc_pipe_server server;
	bool res = ipc_pipe_server_start(&server, "test_pipe", nullptr, nullptr);

	// We expect false because it's stubbed
	if (res) {
		std::cerr << "Unexpected success: Server started (stub logic fail?)" << std::endl;
		return 1;
	} else {
		std::cout << "[TestIPC] Server start check passed (returned false as expected for stub)." << std::endl;
	}

	std::cout << "[TestIPC] Done." << std::endl;
	return 0;
}
