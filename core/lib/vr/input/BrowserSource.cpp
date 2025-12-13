#include "libvr/IBrowserAdapter.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cmath>

namespace libvr {

// A mock browser that renders a "page" to a software buffer (simulating shared memory)
class MockBrowserSource {
public:
    MockBrowserSource() {
        static const IBrowserAdapter_Vtbl vtbl_impl = {
            StaticInitialize,
            StaticShutdown,
            StaticAcquireFrame,
            StaticReleaseFrame,
            StaticNavigate,
            StaticReload,
            StaticExecuteJavascript,
            StaticSendMouseMove,
            StaticSendMouseClick,
            StaticSendKey
        };
        adapter.vtbl = &vtbl_impl;
        adapter.user_data = this;
    }

    IBrowserAdapter* GetAdapter() { return &adapter; }

    bool Initialize(const BrowserConfig* cfg) {
        width = cfg->base.width > 0 ? cfg->base.width : 1280;
        height = cfg->base.height > 0 ? cfg->base.height : 720;
        current_url = cfg->initial_url ? cfg->initial_url : "about:blank";
        
        std::cout << "[MockBrowser] Initialized " << width << "x" << height << " Url: " << current_url << std::endl;
        
        // Allocate mock buffer (RGBA)
        buffer.resize(width * height * 4, 0xFF);
        return true;
    }

    void Shutdown() {
        buffer.clear();
    }

    GPUFrameView AcquireFrame() {
        // Render a pattern
        UpdateFrame();

        GPUFrameView view = {};
        view.handle = buffer.data(); // Software pointer for now
        // In real Vulkan usage, we would upload this to a VkImage or use mapped memory.
        // For the Source interface contract, we return generic handle. 
        // Logic downstream needs to know if it's CPU or GPU.
        // Let's assume downstream handles CPU pointers for "Generic" type sources or we flag it.
        view.width = width;
        view.height = height;
        view.stride = width * 4;
        return view;
    }

    void ReleaseFrame(GPUFrameView* frame) {
        // Nothing to free, we own the buffer
    }

    void Navigate(const char* url) {
        current_url = url ? url : "";
        std::cout << "[MockBrowser] Navigating to: " << current_url << std::endl;
        frame_counter = 0; // Reset animation
    }

    void Reload() {
        std::cout << "[MockBrowser] Reloading..." << std::endl;
    }

    void ExecuteJavascript(const char* script) {
        std::cout << "[MockBrowser] JS: " << (script ? script : "") << std::endl;
    }

    void SendMouseMove(int x, int y) {
        cursor_x = x;
        cursor_y = y;
    }

    void SendMouseClick(int x, int y, bool down) {
        std::cout << "[MockBrowser] Click " << (down ? "Down" : "Up") << " at " << x << "," << y << std::endl;
        if (down) color_mode = !color_mode; // Toggle color on click
    }

    void SendKey(int key, bool down) {
        // ignore
    }

private:
    void UpdateFrame() {
        frame_counter++;
        
        // Simple software render: Fill background
        uint8_t r = color_mode ? 200 : 240;
        uint8_t g = color_mode ? 240 : 200;
        uint8_t b = 255;
        
        // Optimize: Don't fill every frame if static, but this is a mock
        // Fill a small region to simulate activity
        for (int y=0; y<height; y+=10) {
            for (int x=0; x<width; x+=10) {
                 // pattern
                 uint8_t* p = &buffer[(y * width + x) * 4];
                 p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
            }
        }

        // Draw cursor
        if (cursor_x >= 0 && cursor_x < width && cursor_y >= 0 && cursor_y < height) {
             int idx = (cursor_y * width + cursor_x) * 4;
             buffer[idx+0] = 255; 
             buffer[idx+1] = 0; 
             buffer[idx+2] = 0;
        }
    }

    // Static Trampolines
    static bool StaticInitialize(IBrowserAdapter* self, const BrowserConfig* cfg) { return static_cast<MockBrowserSource*>(self->user_data)->Initialize(cfg); }
    static void StaticShutdown(IBrowserAdapter* self) { static_cast<MockBrowserSource*>(self->user_data)->Shutdown(); }
    static GPUFrameView StaticAcquireFrame(IBrowserAdapter* self) { return static_cast<MockBrowserSource*>(self->user_data)->AcquireFrame(); }
    static void StaticReleaseFrame(IBrowserAdapter* self, GPUFrameView* frame) { static_cast<MockBrowserSource*>(self->user_data)->ReleaseFrame(frame); }
    static void StaticNavigate(IBrowserAdapter* self, const char* url) { static_cast<MockBrowserSource*>(self->user_data)->Navigate(url); }
    static void StaticReload(IBrowserAdapter* self) { static_cast<MockBrowserSource*>(self->user_data)->Reload(); }
    static void StaticExecuteJavascript(IBrowserAdapter* self, const char* script) { static_cast<MockBrowserSource*>(self->user_data)->ExecuteJavascript(script); }
    static void StaticSendMouseMove(IBrowserAdapter* self, int x, int y) { static_cast<MockBrowserSource*>(self->user_data)->SendMouseMove(x, y); }
    static void StaticSendMouseClick(IBrowserAdapter* self, int x, int y, bool down) { static_cast<MockBrowserSource*>(self->user_data)->SendMouseClick(x, y, down); }
    static void StaticSendKey(IBrowserAdapter* self, int k, bool d) { static_cast<MockBrowserSource*>(self->user_data)->SendKey(k, d); }

    IBrowserAdapter adapter;
    int width=0, height=0;
    std::string current_url;
    std::vector<uint8_t> buffer;
    int frame_counter = 0;
    int cursor_x = 0, cursor_y = 0;
    bool color_mode = false;
};

extern "C" IBrowserAdapter* CreateBrowserSource() {
    auto* src = new MockBrowserSource();
    return src->GetAdapter();
}

} // namespace libvr
