#include "ndi.hpp"
#include <obs-module.h>

#ifdef _WIN32_

#else
#ifdef __APPLE__
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

// Helper used when cleaning up ndibridge processes
void cleanup_kill(void *arg) {
	pid_t *pid = reinterpret_cast<pid_t*>(arg);
	kill(*pid, SIGKILL);
	waitpid(*pid, nullptr, 0);
	delete pid;
}

#endif
#endif
