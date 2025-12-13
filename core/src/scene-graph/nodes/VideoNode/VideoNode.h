#pragma once

#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {

        class VideoNode : public BaseNodeBackend
        {
              public:
            VideoNode(const std::string &id);
            ~VideoNode() override;

            // Execution
            void execute(const ExecutionContext &context) override;

            // Properties
            void setVideoPath(const std::string &path);
            std::string getVideoPath() const;

            void setLoop(bool loop);
            bool getLoop() const;

              private:
            std::string m_videoPath;
            bool m_loop = true;
            bool m_dirty = false;
            // Video decoder/player state
            uint32_t m_videoPlayerId = 0;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
