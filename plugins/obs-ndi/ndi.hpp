#include <string>

extern std::string ndibridge_path;
void NDISourceRegister();
void NDIOutputRegister();

#ifdef _WIN32_

#else
#ifdef __APPLE__
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

// Helper used when cleaning up ndibridge processes
void cleanup_kill(void *arg);

#endif
#endif
