#pragma once
#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {
        // Generic ML/AI node supporting multiple backends (ONNX, TensorFlow, PyTorch, etc.)
        class MLNode : public BaseNodeBackend
        {
              public:
            MLNode(const std::string &id);
            ~MLNode() override;
            void execute(const ExecutionContext &context) override;
            void setModelPath(const std::string &path);
            std::string getModelPath() const;

              private:
            std::string m_modelPath;
            bool m_dirty = false;
            void *m_modelSession = nullptr;  // Generic pointer for various ML backends
        };
    }  // namespace SceneGraph
}  // namespace NeuralStudio
