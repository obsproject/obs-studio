#pragma once

#include "libvr/IAudioAdapter.h"
#include <vector>

namespace libvr {

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // Create an audio source (e.g. Mic input)
    IAudioAdapter* CreateSource(const AudioSourceConfig& config);
    void DestroySource(IAudioAdapter* adapter);

    // Process/Mix Audio (Called periodically)
    void Update();

private:
    std::vector<IAudioAdapter*> input_sources;
};

// Built-in Adapters
extern "C" IAudioAdapter* CreatePipeWireAudioSource(); // TBD

} // namespace libvr
