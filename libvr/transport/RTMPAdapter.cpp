#include "libvr/ITransportAdapter.h"
#include <iostream>
#include <string>

// RTMP typically handled by librtmp or FFmpeg's libavformat.
// For this adapter, we will assume a direct librtmp wrapping or similar logic.

namespace libvr {

class RTMPAdapter : public ITransportAdapter {
public:
    RTMPAdapter() {
        static const ITransportAdapter_Vtbl vtbl_impl = {
            StaticInitialize,
            StaticSendPacket,
            StaticShutdown
        };
        this->vtbl = &vtbl_impl;
        this->user_data = this;
    }

    ~RTMPAdapter() {
        Shutdown();
    }

    bool Initialize(const char* url, const char* options) {
        std::cout << "[RTMP] Initializing stream to: " << url << std::endl;
        // In reality: RTMP_Init(), RTMP_SetupURL(), RTMP_Connect()
        connected = true;
        return true;
    }

    void SendPacket(const EncodedPacket* packet) {
        if (!connected) return;
        // In reality: RTMP_Write(rtmp, packet->data, packet->size)
        // FLV tagging would happen here or before this adapter.
        // std::cout << "[RTMP] Sent " << packet->size << " bytes." << std::endl;
    }

    void Shutdown() {
        if (connected) {
             std::cout << "[RTMP] Closing stream." << std::endl;
             // RTMP_Close()
        }
        connected = false;
    }

    // Static Trampolines
    static void StaticInitialize(ITransportAdapter* self, const char* url, const char* options) {
        static_cast<RTMPAdapter*>(self->user_data)->Initialize(url, options);
    }
    static void StaticSendPacket(ITransportAdapter* self, const EncodedPacket* packet) {
        static_cast<RTMPAdapter*>(self->user_data)->SendPacket(packet);
    }
    static void StaticShutdown(ITransportAdapter* self) {
        static_cast<RTMPAdapter*>(self->user_data)->Shutdown();
    }

private:
    bool connected = false;
    // RTMP* rtmp = nullptr; 
};

extern "C" ITransportAdapter* CreateRTMPAdapter() {
    return new RTMPAdapter();
}

} // namespace libvr
