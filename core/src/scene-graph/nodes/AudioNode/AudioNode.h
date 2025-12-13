#pragma once

#include "../BaseNodeBackend.h"
#include <string>

namespace NeuralStudio {
    namespace SceneGraph {

        class AudioNode : public BaseNodeBackend
        {
              public:
            AudioNode(const std::string &id);
            ~AudioNode() override;

            // Execution
            void execute(const ExecutionContext &context) override;

            // Properties
            void setAudioPath(const std::string &path);
            std::string getAudioPath() const;

            void setLoop(bool loop);
            bool getLoop() const;

            void setVolume(float volume);  // 0.0 to 1.0
            float getVolume() const;

              private:
            std::string m_audioPath;
            bool m_loop = true;
            float m_volume = 1.0f;
            bool m_dirty = false;

            // Audio player handle
            uint32_t m_audioPlayerId = 0;

            // Audio metadata
            int m_sampleRate = 0;
            int m_channels = 0;
            float m_duration = 0.0f;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
