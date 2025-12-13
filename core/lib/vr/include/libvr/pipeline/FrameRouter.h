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
    
    // Add Virtual Cam Output
    void AddVirtualCam(void* virtualCam); // void* to avoid cyclical deps in header for now, or forward decl

    // ML/Stitch pipeline setup (for Phase 10)
    void SetStitcher(void* stitcher); // void* to avoid pulling headers yet, or strict include
    void SetSuperRes(void* superRes);
    
    // Main Pipeline Entry Point
    // presentationTarget: Optional handle (e.g. VkImage) to blit the final frame to (for OpenXR).
    void ProcessFrame(void* presentationTarget = nullptr);

private:
    IRenderEngine* renderer;
    std::vector<IEncoderAdapter*> encoders;
    std::vector<void*> virtualCams; // Store VirtualCamOutput*
    
    // Pipeline components
    void* stitcher = nullptr;
    void* superRes = nullptr;
};

} // namespace libvr
