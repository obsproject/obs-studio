#include "libvr/pipeline/FrameRouter.h"
#include <iostream>
#include <algorithm>
#include "libvr/IStitcher.h"
#include "libvr/ISuperResolutionAdapter.h"
#include "libvr/VirtualCamOutput.h"

namespace libvr {

FrameRouter::FrameRouter(IRenderEngine* renderer) : renderer(renderer) {}

FrameRouter::~FrameRouter() {}

void FrameRouter::AddEncoder(IEncoderAdapter* encoder) {
    encoders.push_back(encoder);
}

void FrameRouter::RemoveEncoder(IEncoderAdapter* encoder) {
    encoders.erase(std::remove(encoders.begin(), encoders.end(), encoder), encoders.end());
}

void FrameRouter::AddVirtualCam(void* virtualCam) {
    virtualCams.push_back(virtualCam);
}

void FrameRouter::SetStitcher(void* stitcherPtr) {
    this->stitcher = stitcherPtr;
}

void FrameRouter::SetSuperRes(void* superResPtr) {
    this->superRes = superResPtr;
}

void FrameRouter::ProcessFrame(void* presentationTarget) {
    if (!renderer) return;

    // 1. Begin Frame
    if (!renderer->BeginFrame()) {
        std::cerr << "[FrameRouter] Dropping frame: BeginFrame failed" << std::endl;
        return;
    }

    // 2. Update Scene (TODO: Link SceneManager here)
    // Future: sceneManager->Update(deltaTime);

    // 3. Render Input (Capture)
    renderer->DrawScene(nullptr);

    // 4. Get Initial Output (The Capture)
    GPUFrameView frame = renderer->GetOutputFrame();
    
    // 5. Stitching (4K -> 4K Equirect)
    if (stitcher) {
        IStitcher* algo = (IStitcher*)stitcher;
        StitchConfig cfg = {nullptr, true}; // Should come from member/config
        if (algo->vtbl && algo->vtbl->Process) {
             algo->vtbl->Process(algo, &frame, &frame, &cfg); // In-place mockup
        }
    }

    // 6. Super Resolution (4K -> 8K)
    if (superRes) {
        ISuperResolutionAdapter* algo = (ISuperResolutionAdapter*)superRes;
        SRConfig cfg = {0.0f, 1, nullptr, 2.0f}; // Sharpness, Quality, Model, Scale
        if (algo->vtbl && algo->vtbl->Process) {
             algo->vtbl->Process(algo, &frame, &frame, &cfg); // In-place mockup
             // Update frame dims to 8K if successful
             frame.width *= 2; 
             frame.height *= 2;
        }
    }

    // 7. Broadcast to Encoders (RTMP/Disk)
    for (auto* encoder : encoders) {
        if (encoder && encoder->vtbl && encoder->vtbl->EncodeFrame) {
            if (!encoder->vtbl->EncodeFrame(const_cast<IEncoderAdapter*>(encoder), &frame)) {
                 // Log error
            }
        }
    }
    
    // 8. Broadcast to Virtual Cams
    // Ensures Virtual Cam receives the exact same frame as the Encoders (LayerMixer logic)
    for (void* vcamPtr : virtualCams) {
        if (vcamPtr) {
             reinterpret_cast<VirtualCamOutput*>(vcamPtr)->SendFrame(frame);
        }
    }

    // 9. Presentation Blit (Output to Headset)
    if (presentationTarget) {
        renderer->BlitToExternal(presentationTarget, frame.width, frame.height);
    }
    
    // 10. End Frame (Implicit in renderer cleanup usually, or add EndFrame() method)
}

} // namespace libvr
