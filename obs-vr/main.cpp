#include <iostream>

extern "C" void libvr_init(); // From libvr

int main() {
    std::cout << "Starting obs-vr..." << std::endl;
    libvr_init();
    std::cout << "obs-vr running." << std::endl;
    return 0;
}
