#pragma once

#include "libvr/IRenderEngine.h"
#include "libvr/IEncoderAdapter.h"
#include <vector>
#include <memory>

namespace libvr {

class FrameRouter {
public:
    FrameRouter(IRenderEngine* renderer);
    ~FrameRouter();

    void AddEncoder(IEncoderAdapter* encoder);
    void RemoveEncoder(IEncoderAdapter* encoder);

    // Main loop tick
    void RunFrame();

private:
    IRenderEngine* renderer;
    std::vector<IEncoderAdapter*> encoders;
};

} // namespace libvr
