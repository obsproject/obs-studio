#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include "NodeExecutionGraph.h"

// Forward Declarations
namespace NeuralStudio {
    namespace Rendering {
        class IRenderEngine;
    }

    namespace libvr {
        class SceneManager;
    }
}  // namespace NeuralStudio

namespace NeuralStudio {
    namespace SceneGraph {

        /**
     * @brief SceneGraphManager
     * 
     * Central coordinator for the Dual-View architecture.
     * Manages the synchronization between the Blueprint Node Graph (Flow)
     * and the 3D Scene Graph (Spatial).
     */
        class SceneGraphManager
        {
              public:
            SceneGraphManager();
            virtual ~SceneGraphManager();

            // Initialization
            bool initialize(Rendering::IRenderEngine *renderer);
            void shutdown();

            // Main Loop
            void update(double deltaTime);
            void render();

            // Graph Management
            NodeExecutionGraph *getNodeGraph() const;
            libvr::SceneManager *getSceneManager() const;

            // High-Level Operations (Transactional)
            // These ensure both graphs are kept in sync
            bool createNode(const std::string &type, const std::string &id);
            bool deleteNode(const std::string &id);
            bool connectNodes(const std::string &srcId, const std::string &srcPin, const std::string &dstId,
                              const std::string &dstPin);

            // Synchronization
            void markDirty(const std::string &nodeId);  // Flag for update

              private:
            // Core Subsystems
            std::unique_ptr<NodeExecutionGraph> m_nodeGraph;
            std::shared_ptr<libvr::SceneManager> m_sceneManager;  // Shared with Legacy VR system?
            Rendering::IRenderEngine *m_renderer = nullptr;

            // Context
            ExecutionContext m_executionContext;

            // Mapping
            // Blueprint ID (String) -> Scene ID (uint32_t)
            std::map<std::string, uint32_t> m_nodeToSceneId;
            std::map<uint32_t, std::string> m_sceneToNodeId;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
