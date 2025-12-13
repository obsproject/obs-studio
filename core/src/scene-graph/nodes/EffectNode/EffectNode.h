#pragma once
#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {
        class EffectNode : public BaseNodeBackend
        {
              public:
            EffectNode(const std::string &id);
            ~EffectNode() override;
            void execute(const ExecutionContext &context) override;
            void setEffectPath(const std::string &path);
            std::string getEffectPath() const;

              private:
            std::string m_effectPath;
            bool m_dirty = false;
        };
    }  // namespace SceneGraph
}  // namespace NeuralStudio
