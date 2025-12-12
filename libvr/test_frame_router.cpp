#include "pipeline/FrameRouter.h"
#include <iostream>
#include <vector>

// Mock Renderer
class MockRenderEngine : public libvr::IRenderEngine {
public:
    bool Initialize(const libvr::RenderConfig& config) override { return true; }
    void Shutdown() override {}
    bool BeginFrame() override { return true; }
    void DrawScene(const libvr::SceneManager* scene) override {}
    GPUFrameView GetOutputFrame() override {
        GPUFrameView view = {};
        view.width = 100;
        view.height = 100;
        view.timestamp = 123456789;
        return view;
    }
};

// Mock Encoder
struct MockEncoder : public IEncoderAdapter {
    static bool MockEncode(IEncoderAdapter* self, const GPUFrameView* frame) {
        // Validation check
        if (frame->width == 100 && frame->height == 100 && frame->timestamp == 123456789) {
            std::cout << "[TestFrameRouter] RECEIVED VALID FRAME " << frame->timestamp << std::endl;
            return true;
        }
        std::cerr << "[TestFrameRouter] RECEIVED INVALID FRAME" << std::endl;
        return false;
    }
    
    // Stub table
    static const IEncoderAdapter_Vtbl vtbl_inst;
};

const IEncoderAdapter_Vtbl MockEncoder::vtbl_inst = {
    nullptr, // Init
    MockEncoder::MockEncode,
    nullptr, // Flush
    nullptr  // Shutdown
};

int main() {
    MockRenderEngine renderer;
    MockEncoder encoder;
    encoder.vtbl = &MockEncoder::vtbl_inst;

    libvr::FrameRouter router(&renderer);
    router.AddEncoder(&encoder);

    std::cout << "[TestFrameRouter] Running frame tick..." << std::endl;
    router.ProcessFrame();

    return 0;
}
