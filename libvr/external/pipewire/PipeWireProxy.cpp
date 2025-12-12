#include "PipeWireProxy.h"
#include <dlfcn.h>
#include <iostream>

namespace libvr {

static void* libpipewire = nullptr;

t_pw_init wrapper_pw_init = nullptr;
t_pw_thread_loop_new wrapper_pw_thread_loop_new = nullptr;
t_pw_thread_loop_start wrapper_pw_thread_loop_start = nullptr;

bool LoadPipeWireFunctions() {
    if (libpipewire) return true;

    libpipewire = dlopen("libpipewire-0.3.so.0", RTLD_LAZY | RTLD_GLOBAL);
    if (!libpipewire) {
         // Try generic name
         libpipewire = dlopen("libpipewire-0.3.so", RTLD_LAZY | RTLD_GLOBAL);
    }

    if (!libpipewire) {
        std::cerr << "[PipeWire] Failed to load libpipewire-0.3" << std::endl;
        return false;
    }

    wrapper_pw_init = (t_pw_init)dlsym(libpipewire, "pw_init");
    wrapper_pw_thread_loop_new = (t_pw_thread_loop_new)dlsym(libpipewire, "pw_thread_loop_new");
    wrapper_pw_thread_loop_start = (t_pw_thread_loop_start)dlsym(libpipewire, "pw_thread_loop_start");

    if (!wrapper_pw_init || !wrapper_pw_thread_loop_new) {
         std::cerr << "[PipeWire] Missing symbols" << std::endl;
         return false;
    }
    
    std::cout << "[PipeWire] Library loaded successfully." << std::endl;
    return true;
}

} // namespace libvr
