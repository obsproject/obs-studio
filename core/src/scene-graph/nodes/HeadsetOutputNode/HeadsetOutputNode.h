```
#pragma once

#include "../BaseNodeBackend.h"
#include "../../compositor/VirtualCamManager.h"

    namespace NeuralStudio
{
    namespace SceneGraph {

        /**
 * @brief Headset Output Node - Wraps VirtualCamManager for multi-profile output
 * 
 * Outputs rendered frames to VR headsets (Quest 3, Index, Vive Pro 2, etc.)
 * with per-profile encoding and streaming.
 */
        class HeadsetOutputNode : public BaseNodeBackend
        {
              public:
            HeadsetOutputNode(const std::string &nodeId);
            ~HeadsetOutputNode() override = default;

            bool initialize(const NodeConfig &config) override;
            ExecutionResult process(ExecutionContext &ctx) override;
            void cleanup() override;

              private:
            VirtualCamManager *m_virtualCamManager = nullptr;
            std::string m_profileId;  // Which headset profile to output to
            bool m_initialized = false;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
