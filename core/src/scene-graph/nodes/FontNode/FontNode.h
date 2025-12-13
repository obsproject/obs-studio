#pragma once

#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {

        class FontNode : public BaseNodeBackend
        {
              public:
            FontNode(const std::string &id);
            ~FontNode() override;

            void execute(const ExecutionContext &context) override;

            void setFontPath(const std::string &path);
            std::string getFontPath() const;

            void setFontSize(int size);
            int getFontSize() const;

              private:
            std::string m_fontPath;
            int m_fontSize = 48;
            bool m_dirty = false;
            uint32_t m_fontId = 0;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
