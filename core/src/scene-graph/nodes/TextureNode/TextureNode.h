#pragma once
#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {
        class TextureNode : public BaseNodeBackend
        {
              public:
            TextureNode(const std::string &id);
            ~TextureNode() override;
            void execute(const ExecutionContext &context) override;
            void setTexturePath(const std::string &path);
            std::string getTexturePath() const;

              private:
            std::string m_texturePath;
            bool m_dirty = false;
            uint32_t m_textureId = 0;
        };
    }  // namespace SceneGraph
}  // namespace NeuralStudio
