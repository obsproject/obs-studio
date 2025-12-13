#pragma once

#include "../BaseNodeBackend.h"
#include "../../rendering/STMapLoader.h"
#include "../../rendering/StereoRenderer.h"

namespace NeuralStudio {
    namespace SceneGraph {

        /**
 * @brief Stitch Node - Wraps STMapLoader for fisheye stitching
 * 
 * Converts fisheye camera input to equirectangular output using STMap UV remapping.
 */
        class StitchNode : public BaseNodeBackend
        {
              public:
            StitchNode(const std::string &nodeId);
            ~StitchNode() override = default;

            bool initialize(const NodeConfig &config) override;
            ExecutionResult process(ExecutionContext &ctx) override;
            void cleanup() override;

              private:
            std::unique_ptr<Rendering::STMapLoader> m_stmapLoader;
            Rendering::StereoRenderer *m_renderer = nullptr;
            std::string m_stmapPath;
            bool m_initialized = false;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
