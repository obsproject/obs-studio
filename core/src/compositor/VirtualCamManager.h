#pragma once

#include "VRHeadsetProfile.h"
#include <vector>
#include <map>
#include <memory>
#include <string>

namespace NeuralStudio {

    // Forward declarations
    class SceneManager;
    class VideoEncoder;
    class SRTOutputStream;

    struct FramebufferHandle {
        void *leftEyeBuffer;
        void *rightEyeBuffer;
        uint32_t width;
        uint32_t height;
    };

    class VirtualCamManager
    {
          public:
        VirtualCamManager();
        ~VirtualCamManager();

        // Profile Management
        void loadProfilesFromDirectory(const std::string &profileDir);
        void addProfile(const VRHeadsetProfile &profile);
        void removeProfile(const std::string &profileId);
        void enableProfile(const std::string &profileId, bool enabled);

        std::vector<VRHeadsetProfile> getProfiles() const;
        VRHeadsetProfile *getProfile(const std::string &profileId);

        // Streaming Control
        void startStreaming(SceneManager *sceneManager);
        void stopStreaming();
        bool isStreaming() const
        {
            return m_streaming;
        }

        // Frame Rendering (called per frame)
        void renderFrame(SceneManager *sceneManager, double deltaTime);

        // Query active streams
        struct StreamInfo {
            std::string profileId;
            std::string profileName;
            std::string srtUrl;
            std::string status;  // "streaming", "initializing", "error"
            uint64_t frameCount;
            double framerate;
        };
        std::vector<StreamInfo> getActiveStreams() const;

          private:
        void allocateFramebuffer(const VRHeadsetProfile &profile);
        void freeFramebuffer(const std::string &profileId);

        void renderProfileFrame(const VRHeadsetProfile &profile, SceneManager *sceneManager);
        void encodeAndStream(const std::string &profileId, const FramebufferHandle &fb);

        std::vector<VRHeadsetProfile> m_profiles;
        std::map<std::string, FramebufferHandle> m_framebuffers;

        // TODO Phase 3: Encoding & Streaming
        // std::map<std::string, std::unique_ptr<VideoEncoder>> m_encoders;
        // std::map<std::string, std::unique_ptr<SRTOutputStream>> m_srtStreams;

        bool m_streaming;
        SceneManager *m_sceneManager;
    };

}  // namespace NeuralStudio
