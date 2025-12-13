#pragma once

#include "../BaseNodeBackend.h"
#include "../../rendering/RTXUpscaler.h"

namespace NeuralStudio {
    namespace SceneGraph {

        /**
 * @brief RTX Upscale Node - Wraps RTXUpscaler for AI upscaling
 * 
 * NVIDIA AI-powered upscaling (4K→8K) using Maxine SDK.
 * Requires NVIDIA RTX GPU with Tensor Cores.
 */
        class RTXUpscaleNode : public BaseNodeBackend
        {
              public:
            RTXUpscaleNode(const std::string &nodeId);
            ~RTXUpscaleNode() override = default;

            bool initialize(const NodeConfig &config) override;
            ExecutionResult process(ExecutionContext &ctx) override;
            void cleanup() override;

            bool supportsGPU() const override
            {
                return true;
            }

              private:
            std::unique_ptr<Rendering::RTXUpscaler> m_upscaler;
            Rendering::RTXUpscaler::QualityMode m_qualityMode;
            Rendering::RTXUpscaler::ScaleFactor m_scaleFactor;
            bool m_initialized = false;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
