#include "libvr/pipeline/FrameRouter.h"
#include <iostream>
#include <algorithm>

namespace libvr {

FrameRouter::FrameRouter(IRenderEngine* renderer) : renderer(renderer) {}

FrameRouter::~FrameRouter() {}

void FrameRouter::AddEncoder(IEncoderAdapter* encoder) {
    encoders.push_back(encoder);
}

void FrameRouter::RemoveEncoder(IEncoderAdapter* encoder) {
    encoders.erase(std::remove(encoders.begin(), encoders.end(), encoder), encoders.end());
}

void FrameRouter::ProcessFrame() {
    if (!renderer) return;

    // 1. Begin Frame
    if (!renderer->BeginFrame()) {
        std::cerr << "[FrameRouter] Dropping frame: BeginFrame failed" << std::endl;
        return;
    }

    // 2. Update Scene (TODO: Link SceneManager here)
    // Future: sceneManager->Update(deltaTime);

    // 3. Render
    // Pass nullptr for now as FrameRouter doesn't own SceneManager yet or needs refactor
    renderer->DrawScene(nullptr);

    // 4. Get Output
    GPUFrameView frame = renderer->GetOutputFrame();

    // 5. Broadcast to Encoders
    for (auto* encoder : encoders) {
        if (encoder && encoder->vtbl && encoder->vtbl->EncodeFrame) {
            // Note: Ownership of 'frame' remains with Renderer (it's a view).
            // Encoder must copy or consume immediately.
            if (!encoder->vtbl->EncodeFrame(const_cast<IEncoderAdapter*>(encoder), &frame)) {
                 // std::cerr << "[FrameRouter] Encoder failed to consume frame" << std::endl;
            }
        }
    }
    
    // 6. End Frame (Implied in IRenderEngine specific flow, or added explicit EndFrame if needed)
}

} // namespace libvr
