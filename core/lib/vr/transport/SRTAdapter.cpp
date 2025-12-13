#include "libvr/ITransportAdapter.h"
#include <iostream>
#include <string>
#include <vector>

#ifdef HAS_SRT
#include <srt/srt.h>
#include <arpa/inet.h>
#endif

namespace libvr {

#ifdef HAS_SRT
class SRTAdapter : public ITransportAdapter {
public:
    SRTAdapter() {
        srt_startup();

        static const ITransportAdapter_Vtbl vtbl_impl = {
            StaticInitialize,
            StaticSendPacket,
            StaticShutdown
        };
        this->vtbl = &vtbl_impl;
        this->user_data = this;
    }

    ~SRTAdapter() {
        Shutdown();
        srt_cleanup();
    }

    bool Initialize(const char* url, const char* options) {
        std::cout << "[SRT] Initializing connection to: " << url << std::endl;
        
        sock = srt_create_socket();
        if (sock == SRT_INVALID_SOCK) {
            std::cerr << "[SRT] Failed to create socket." << std::endl;
            return false;
        }

        std::string s_url(url);
        std::string protocol = "srt://";
        if (s_url.find(protocol) == 0) s_url = s_url.substr(protocol.length());
        
        size_t colon = s_url.find(':');
        if (colon == std::string::npos) return false;
        
        std::string ip = s_url.substr(0, colon);
        int port = std::stoi(s_url.substr(colon + 1));

        sockaddr_in sa = {};
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &sa.sin_addr) != 1) return false;

        if (srt_connect(sock, (sockaddr*)&sa, sizeof(sa)) == SRT_ERROR) {
             std::cerr << "[SRT] Connect failed." << std::endl;
             srt_close(sock);
             sock = SRT_INVALID_SOCK;
             return false;
        }

        connected = true;
        return true;
    }

    void SendPacket(const EncodedPacket* packet) {
        if (!connected || sock == SRT_INVALID_SOCK) return;
        if (srt_sendmsg(sock, (const char*)packet->data, (int)packet->size, -1, true) == SRT_ERROR) {
            // Log error
        }
    }

    void Shutdown() {
        if (sock != SRT_INVALID_SOCK) {
            srt_close(sock);
            sock = SRT_INVALID_SOCK;
        }
        connected = false;
    }

    static void StaticInitialize(ITransportAdapter* self, const char* url, const char* options) {
        static_cast<SRTAdapter*>(self->user_data)->Initialize(url, options);
    }
    static void StaticSendPacket(ITransportAdapter* self, const EncodedPacket* packet) {
        static_cast<SRTAdapter*>(self->user_data)->SendPacket(packet);
    }
    static void StaticShutdown(ITransportAdapter* self) {
        static_cast<SRTAdapter*>(self->user_data)->Shutdown();
    }

private:
    SRTSOCKET sock = SRT_INVALID_SOCK;
    bool connected = false;
};
#endif

extern "C" ITransportAdapter* CreateSRTAdapter() {
#ifdef HAS_SRT
    return new SRTAdapter();
#else
    std::cerr << "[SRT] Adapter not compiled (missing libsrt)" << std::endl;
    return nullptr;
#endif
}

} // namespace libvr
