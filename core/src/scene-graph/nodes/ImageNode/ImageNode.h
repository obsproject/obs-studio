#pragma once

#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {

        class ImageNode : public BaseNodeBackend
        {
              public:
            ImageNode(const std::string &id);
            ~ImageNode() override;

            // Execution
            void execute(const ExecutionContext &context) override;

            // Properties
            void setImagePath(const std::string &path);
            std::string getImagePath() const;

            void setFilterMode(const std::string &filterMode);  // "linear", "nearest"
            std::string getFilterMode() const;

              private:
            std::string m_imagePath;
            std::string m_filterMode = "linear";
            bool m_dirty = false;

            // Texture/image handle
            uint32_t m_textureId = 0;

            // Image metadata
            int m_width = 0;
            int m_height = 0;
            bool m_hasAlpha = false;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
