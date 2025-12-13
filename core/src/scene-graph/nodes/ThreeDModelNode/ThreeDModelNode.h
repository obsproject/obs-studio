#pragma once

#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {

        class ThreeDModelNode : public BaseNodeBackend
        {
              public:
            ThreeDModelNode(const std::string &id);
            ~ThreeDModelNode() override;

            // Execution
            void execute(const ExecutionContext &context) override;

            // Properties
            void setModelPath(const std::string &path);
            std::string getModelPath() const;

              private:
            std::string m_modelPath;
            // Transform properties could be added here or handled via Pins if dynamic
            // For simplicity, we assume properties are synced via QProperty for now
            // But backend storage is needed for serialization.

            // Scene Object ID referencing the loaded model in SceneManager
            uint32_t m_sceneObjectId = 0;
            bool m_dirty = false;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
