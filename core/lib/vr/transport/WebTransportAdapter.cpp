#include "libvr/ITransportAdapter.h"
#include <iostream>
#include <string>

// WebTransport typically uses HTTP/3. This skeleton assumes a generic implementation structure.
// In reality, this would wrap a library like ls-qpack, quiche, or msquic.

namespace libvr {

class WebTransportAdapter : public ITransportAdapter {
public:
    WebTransportAdapter() {
        static const ITransportAdapter_Vtbl vtbl_impl = {
            StaticInitialize,
            StaticSendPacket,
            StaticShutdown
        };
        this->vtbl = &vtbl_impl;
        this->user_data = this;
    }

    ~WebTransportAdapter() {
        Shutdown();
    }

    bool Initialize(const char* url, const char* options) {
        std::cout << "[WebTransport] Connecting to: " << url << " (Options: " << (options ? options : "none") << ")" << std::endl;
        // Mock connection setup
        connected = true;
        return true;
    }

    void SendPacket(const EncodedPacket* packet) {
        if (!connected) return;
        // std::cout << "[WebTransport] Sending packet size: " << packet->size << std::endl;
    }

    void Shutdown() {
        if (connected) {
             std::cout << "[WebTransport] Closing stream." << std::endl;
        }
        connected = false;
    }

    // Trampolines
    static void StaticInitialize(ITransportAdapter* self, const char* url, const char* options) {
        static_cast<WebTransportAdapter*>(self->user_data)->Initialize(url, options);
    }
    static void StaticSendPacket(ITransportAdapter* self, const EncodedPacket* packet) {
        static_cast<WebTransportAdapter*>(self->user_data)->SendPacket(packet);
    }
    static void StaticShutdown(ITransportAdapter* self) {
        static_cast<WebTransportAdapter*>(self->user_data)->Shutdown();
    }

private:
    bool connected = false;
};

extern "C" ITransportAdapter* CreateWebTransportAdapter() {
    return new WebTransportAdapter();
}

} // namespace libvr
