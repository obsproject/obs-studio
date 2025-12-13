#pragma once

#include <QObject>
#include <QRhi>
#include <QRhiTexture>
#include <memory>
#include <vector>
#include <string>
#include <array>

namespace NeuralStudio {
    namespace Rendering {

        /**
 * @brief FramebufferManager - Manages framebuffers for multiple VR headset profiles
 * 
 * Allocates and manages per-profile framebuffers for simultaneous streaming
 * to different VR headsets (Quest 3, Index, Vive Pro 2, etc.). Each profile
 * has unique resolution, FOV, and encoding requirements.
 */
        class FramebufferManager : public QObject
        {
            Q_OBJECT

              public:
            struct ProfileConfig {
                std::string id;    // "quest3", "index", "vive_pro_2"
                std::string name;  // "Meta Quest 3"

                // Resolution per eye
                uint32_t eyeWidth = 1920;
                uint32_t eyeHeight = 1920;

                // Display specs
                float fovHorizontal = 110.0f;  // degrees
                float fovVertical = 96.0f;     // degrees
                float refreshRate = 90.0f;     // Hz

                // Encoding
                std::string codec = "h265";  // "h265", "av1"
                uint32_t bitrate = 100000;   // kbps

                // Output
                uint16_t streamPort = 9001;  // SRT port
                bool enabled = true;
            };

            struct FramebufferSet {
                std::string profileId;

                // Triple buffering for smooth VR rendering
                // Buffer 0: CPU writes (current frame being prepared)
                // Buffer 1: GPU renders (previous frame being composited)
                // Buffer 2: Display reads (frame being encoded/streamed)
                static constexpr int NumBuffers = 3;

                // Stereo framebuffers (L/R) x 3 buffers
                std::array<std::unique_ptr<QRhiTexture>, NumBuffers> leftEyeTextures;
                std::array<std::unique_ptr<QRhiTexture>, NumBuffers> rightEyeTextures;

                // Render targets x 3 buffers
                std::array<std::unique_ptr<QRhiTextureRenderTarget>, NumBuffers> leftEyeTargets;
                std::array<std::unique_ptr<QRhiTextureRenderTarget>, NumBuffers> rightEyeTargets;

                // Render pass descriptors x 3 buffers
                std::array<std::unique_ptr<QRhiRenderPassDescriptor>, NumBuffers> leftEyePasses;
                std::array<std::unique_ptr<QRhiRenderPassDescriptor>, NumBuffers> rightEyePasses;

                // Current buffer indices
                int writeBufferIndex = 0;    // CPU writes here
                int renderBufferIndex = 2;   // GPU renders from here
                int displayBufferIndex = 1;  // Encoder reads from here

                // Get current textures for writing
                QRhiTexture *getCurrentLeftTexture()
                {
                    return leftEyeTextures[writeBufferIndex].get();
                }
                QRhiTexture *getCurrentRightTexture()
                {
                    return rightEyeTextures[writeBufferIndex].get();
                }

                // Get textures for rendering (previous frame)
                QRhiTexture *getRenderLeftTexture()
                {
                    return leftEyeTextures[renderBufferIndex].get();
                }
                QRhiTexture *getRenderRightTexture()
                {
                    return rightEyeTextures[renderBufferIndex].get();
                }

                // Get textures for display/encoding (oldest frame)
                QRhiTexture *getDisplayLeftTexture()
                {
                    return leftEyeTextures[displayBufferIndex].get();
                }
                QRhiTexture *getDisplayRightTexture()
                {
                    return rightEyeTextures[displayBufferIndex].get();
                }
            };
            explicit FramebufferManager(QRhi *rhi, QObject *parent = nullptr);
            ~FramebufferManager() override;

            /**
     * @brief Add a VR headset profile
     */
            bool addProfile(const ProfileConfig &config);

            /**
     * @brief Remove a profile
     */
            void removeProfile(const std::string &profileId);

            /**
     * @brief Enable/disable a profile
     */
            void setProfileEnabled(const std::string &profileId, bool enabled);

            /**
     * @brief Get framebuffer set for a profile
     */
            FramebufferSet *getFramebufferSet(const std::string &profileId);

            /**
     * @brief Get all active profile IDs
     */
            std::vector<std::string> getActiveProfiles() const;

            /**
     * @brief Swap buffers for a profile (double buffering)
     */
            void swapBuffers(const std::string &profileId);

            /**
     * @brief Resize framebuffers for a profile
     */
            bool resizeProfile(const std::string &profileId, uint32_t width, uint32_t height);

              signals:
            void profileAdded(const QString &profileId);
            void profileRemoved(const QString &profileId);
            void framebuffersRecreated();

              private:
            void createFramebuffersForProfile(const ProfileConfig &config);
            void releaseFramebuffersForProfile(const std::string &profileId);

            QRhi *m_rhi;
            std::vector<ProfileConfig> m_profiles;
            std::vector<FramebufferSet> m_framebuffers;
        };

    }  // namespace Rendering
}  // namespace NeuralStudio
