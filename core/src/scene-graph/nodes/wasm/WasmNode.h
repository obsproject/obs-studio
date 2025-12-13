#pragma once

#include "../ai/AINodeBase.h"
#include <vector>
#include <memory>

namespace NeuralStudio {
    namespace SceneGraph {

        /**
     * @brief WasmNode - Executes WebAssembly modules as nodes
     * 
     * Supports loading .wasm files (Addons) and executing them securely.
     * Can be used for:
     * - Custom processing logic
     * - AI inference (WASI-NN)
     * - Third-party integrations
     */
        class WasmNode : public AINodeBase
        {
              public:
            WasmNode(const std::string &nodeId);
            virtual ~WasmNode();

            // IExecutableNode implementation
            bool initialize() override;
            ExecutionResult execute(ExecutionContext &ctx) override;

            // AINodeBase implementation
            std::string getBackendName() const override
            {
                return "WASM";
            }
            std::string getModelPath() const override
            {
                return m_wasmPath;
            }

            // Configuration
            void setWasmPath(const std::string &path);
            void setEntryFunction(const std::string &functionName);

              private:
            std::string m_wasmPath;
            std::string m_entryFunction = "process";

            struct WasmRuntime;  // PIMPL for WASM runtime (WasmEdge/Wasmtime)
            std::unique_ptr<WasmRuntime> m_runtime;

            bool loadModule();
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
