#ifndef VR_PROTOCOL_H
#define VR_PROTOCOL_H

#include <cstdint>

namespace VRProtocol {

    enum class MessageType : uint32_t {
        Unknown = 0,
        Command = 1,      // UI -> Backend (e.g. "SetScene")
        Response = 2,     // Backend -> UI (e.g. "OK")
        Event = 3,        // Backend -> UI (e.g. "SceneChanged")
        PreviewFrame = 4  // Backend -> UI (Shared Handle)
    };

    struct Header {
        MessageType type;
        uint32_t payloadSize;
    };

    // Command IDs (in JSON payload usually, but defined here for ref)
    constexpr const char *CMD_GET_SCENES = "getScenes";
    constexpr const char *CMD_SET_SCENE = "setScene";
    constexpr const char *CMD_GET_NODES = "getNodes";

    // Limits
    constexpr uint32_t MAX_PAYLOAD_SIZE = 10 * 1024 * 1024;  // 10MB
    constexpr const char *SOCKET_PATH = "/tmp/vrobs.sock";

}  // namespace VRProtocol

#endif
