#include "libvr/pipeline/FrameRouter.h"
#include <iostream>
#include <vector>

// Mock Renderer
class MockRenderEngine : public libvr::IRenderEngine {
public:
    bool Initialize(const libvr::RenderConfig& config) override { return true; }
    void Shutdown() override {}
    bool BeginFrame() override { return true; }
    void DrawScene() override {}
    libvr::GPUFrameView GetOutputFrame() override {
        libvr::GPUFrameView view = {};
        view.width = 100;
        view.height = 100;
        view.timestamp = 123456789;
        return view;
    }
};

// Mock Encoder
struct MockEncoder : public libvr::IEncoderAdapter {
    static bool MockEncode(libvr::IEncoderAdapter* self, const libvr::GPUFrameView* frame) {
        // Validation check
        if (frame->width == 100 && frame->height == 100 && frame->timestamp == 123456789) {
            std::cout << "[TestFrameRouter] RECEIVED VALID FRAME " << frame->timestamp << std::endl;
            return true;
        }
        std::cerr << "[TestFrameRouter] RECEIVED INVALID FRAME" << std::endl;
        return false;
    }
    
    // Stub table
    static const libvr::IEncoderAdapter_Vtbl vtbl_inst;
};

const libvr::IEncoderAdapter_Vtbl MockEncoder::vtbl_inst = {
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
    router.RunFrame();

    return 0;
}
