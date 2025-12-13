#include "libvr/IBrowserAdapter.h"
#include <iostream>

// Include real CEF or Stubs
#include "cef_base.h" // Fallback to our stub for now

namespace libvr {

// Skeleton logic for CEF integration
class CEFBrowserSource : public CefClient {
public:
    CEFBrowserSource() {
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
        
        std::cout << "[CEF] Initializing Browser for " << (cfg->initial_url ? cfg->initial_url : "blank") << std::endl;
        
        // Real CEF Init Logic:
        // CefWindowInfo window_info;
        // window_info.SetAsWindowless(0); // Off-screen rendering
        // CefBrowserSettings browser_settings;
        // CefBrowserHost::CreateBrowser(window_info, this, url, browser_settings, nullptr, nullptr);
        
        std::cout << "[CEF] (Stub) Browser Created." << std::endl;
        return true;
    }

    void Shutdown() {
        // if (browserHost) browserHost->CloseBrowser(true);
        std::cout << "[CEF] Shutdown." << std::endl;
    }

    GPUFrameView AcquireFrame() {
        // In real impl (`OnPaint` callback), we lock the pixel buffer.
        // Here we just return empty for the skeleton.
        return {}; 
    }

    void ReleaseFrame(GPUFrameView* frame) {
    }

    void Navigate(const char* url) {
        std::cout << "[CEF] LoadURL: " << url << std::endl;
        // if (browser) browser->LoadURL(url);
    }
    
    void Reload() {
        // if (browser) browser->Reload();
    }

    void ExecuteJavascript(const char* script) {
        // browser->GetMainFrame()->ExecuteJavaScript(script, "", 0);
    }

    void SendMouseMove(int x, int y) {
        // CefMouseEvent event;
        // event.x = x; event.y = y;
        // host->SendMouseMoveEvent(event, false);
    }

    void SendMouseClick(int x, int y, bool down) {
        // host->SendMouseClickEvent(event, MBT_LEFT, !down, 1);
    }
    
    void SendKey(int key, bool down) {}

    // RefCounting for CefClient
    void AddRef() const override {}
    void Release() const override {}
    bool HasOneRef() const override { return true; }

private:
    IBrowserAdapter adapter;
    int width, height;
    
    // CefRefPtr<CefBrowser> browser;
    // CefRefPtr<CefBrowserHost> host;

    // Trampolines (Standard Pattern)
    static bool StaticInitialize(IBrowserAdapter* self, const BrowserConfig* cfg) { return static_cast<CEFBrowserSource*>(self->user_data)->Initialize(cfg); }
    static void StaticShutdown(IBrowserAdapter* self) { static_cast<CEFBrowserSource*>(self->user_data)->Shutdown(); }
    static GPUFrameView StaticAcquireFrame(IBrowserAdapter* self) { return static_cast<CEFBrowserSource*>(self->user_data)->AcquireFrame(); }
    static void StaticReleaseFrame(IBrowserAdapter* self, GPUFrameView* frame) { static_cast<CEFBrowserSource*>(self->user_data)->ReleaseFrame(frame); }
    static void StaticNavigate(IBrowserAdapter* self, const char* url) { static_cast<CEFBrowserSource*>(self->user_data)->Navigate(url); }
    static void StaticReload(IBrowserAdapter* self) { static_cast<CEFBrowserSource*>(self->user_data)->Reload(); }
    static void StaticExecuteJavascript(IBrowserAdapter* self, const char* script) { static_cast<CEFBrowserSource*>(self->user_data)->ExecuteJavascript(script); }
    static void StaticSendMouseMove(IBrowserAdapter* self, int x, int y) { static_cast<CEFBrowserSource*>(self->user_data)->SendMouseMove(x, y); }
    static void StaticSendMouseClick(IBrowserAdapter* self, int x, int y, bool down) { static_cast<CEFBrowserSource*>(self->user_data)->SendMouseClick(x, y, down); }
    static void StaticSendKey(IBrowserAdapter* self, int k, bool d) { static_cast<CEFBrowserSource*>(self->user_data)->SendKey(k, d); }
};

extern "C" IBrowserAdapter* CreateCEFBrowserSource() {
    auto* src = new CEFBrowserSource();
    return src->GetAdapter();
}

} // namespace libvr
