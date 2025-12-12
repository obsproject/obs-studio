#include "AudioManager.h"
#include <iostream>

namespace libvr {

AudioManager::AudioManager() {
    std::cout << "[AudioManager] Subsystem initialized." << std::endl;
}

AudioManager::~AudioManager() {
    for (auto* src : input_sources) {
        DestroySource(src);
    }
}

IAudioAdapter* AudioManager::CreateSource(const AudioSourceConfig& config) {
    std::cout << "[AudioManager] Creating audio source: " << config.backend << std::endl;
    // Factory logic (stub)
    return nullptr;
}

void AudioManager::DestroySource(IAudioAdapter* adapter) {
    // cleanup
}

void AudioManager::Update() {
    // Mix logic
}

} // namespace libvr
