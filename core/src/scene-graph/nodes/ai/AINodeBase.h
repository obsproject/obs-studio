#pragma once

#include "../../IExecutableNode.h"
#include <string>
#include <vector>
#include <future>

namespace NeuralStudio {
    namespace SceneGraph {

        // Base class for all AI/ML nodes (WASM, ONNX, Gemini, etc.)
        class AINodeBase : public IExecutableNode
        {
              public:
            AINodeBase(const std::string &nodeId, const std::string &typeName);
            virtual ~AINodeBase() = default;

            // Common AI Metadata
            virtual std::string getModelPath() const;
            virtual std::string getBackendName() const = 0;  // "WASM", "ONNX", "Gemini"

            // Capabilities
            bool supportsAsync() const override
            {
                return true;
            }
            bool supportsGPU() const override
            {
                return true;
            }

            // Data Management
            void setPinData(const std::string &pinId, const std::any &data) override;
            std::any getPinData(const std::string &pinId) const override;
            bool hasPinData(const std::string &pinId) const override;

              protected:
            std::string m_nodeId;
            std::string m_typeName;
            NodeMetadata m_metadata;
            std::map<std::string, std::any> m_pinData;

            // Helper to report model loading errors
            ExecutionResult modelError(const std::string &message);
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
