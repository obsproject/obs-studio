#pragma once
#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {
        class ShaderNode : public BaseNodeBackend
        {
              public:
            Shader

                Node(const std::string &id);
            ~ShaderNode() override;
            void execute(const ExecutionContext &context) override;
            void setShaderPath(const std::string &path);
            std::string getShaderPath() const;

              private:
            std::string m_shaderPath;
            bool m_dirty = false;
            uint32_t m_shaderId = 0;
        };
    }  // namespace SceneGraph
}  // namespace NeuralStudio
