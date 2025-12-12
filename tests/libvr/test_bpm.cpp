#include <obs.h>
#include "bpm.h"
#include <iostream>

// Mock obs_output_t (if not fully defined in obs.h for C++)
// obs.h declares struct obs_output; typedef struct obs_output obs_output_t;
// So we don't need to redeclare if we include obs.h.

int main() {
    std::cout << "[TestBPM] Initializing..." << std::endl;
    
    // Call a function to force linking
    // We pass null, it handles it safely? checking bpm.c...
    // bfs.c:20: void bpm_destroy(obs_output_t *output) ...
    // bfs.c:566 check: if (!output) blog(...) return;
    // So safe to pass nullptr.
    
    bpm_destroy(nullptr);
    
    std::cout << "[TestBPM] Done (Compile Test)." << std::endl;
    return 0;
}
