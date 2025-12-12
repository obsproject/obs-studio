#pragma once

#include <memory>
#include <string>
#include "libvr/ipc/IPCServer.h"
#include "libvr/SceneManager.h"
#include <map>

namespace libvr {

    class VRController
    {
          public:
        VRController();
        ~VRController();

        bool Initialize();
        void Shutdown();
        void Run();  // Main loop

          private:
        std::string HandleCommand(const std::string &jsonPayload);

        std::unique_ptr<IPCServer> m_ipc;
        std::shared_ptr<SceneManager> m_scene;      // Shared potentially with Renderer
        std::map<std::string, uint32_t> m_nodeMap;  // Frontend UUID -> Backend ID

        bool m_running = false;
    };

}  // namespace libvr
