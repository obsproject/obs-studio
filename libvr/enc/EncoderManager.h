#pragma once

#include "libvr/IEncoderAdapter.h"
#include <vector>
#include <memory>
#include <string>

namespace libvr {

class EncoderManager {
public:
    EncoderManager();
    ~EncoderManager();

    // Factory method to create an encoder based on config
    // We retain ownership, or pass it to FrameRouter?
    // Let's have FrameRouter take ownership for simplicity, or manage it here.
    // Ideally, Manager manages lifecycle.
    IEncoderAdapter* CreateEncoder(const EncoderConfig& config);
    
    void DestroyEncoder(IEncoderAdapter* encoder);

private:
    std::vector<IEncoderAdapter*> active_encoders;
};

} // namespace libvr
