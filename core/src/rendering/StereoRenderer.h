#pragma once

#include <QObject>
#include "VulkanRenderer.h"
#include <memory>

namespace NeuralStudio {
    namespace Rendering {

        /**
 * @brief StereoRenderer - Stereo video processing and 3D overlay compositor
 * 
 * Handles the complete VR180/360 stereo pipeline:
 * 1. INPUT: 4K SBS video (2x 2K left/right frames side-by-side)
 * 2. Split into separate L/R 2K textures
 * 3. Apply STMap stitching to each eye (fisheye → equirectangular)
 * 4. Render 3D objects/effects separately for each eye (with IPD offset)
 * 5. Composite 3D over stitched video for each eye
 * 6. Output per-headset profile streams
 */
        class StereoRenderer : public QObject
        {
            Q_OBJECT

              public:
            enum class EyeIndex {
                Left = 0,
                Right = 1
            };

            enum class InputMode {
                SideBySide,  // 4K SBS (current: 2x 2K frames in single texture)
                DualStream   // AV2 multi-stream (future: 2 separate 2K/4K textures)
            };

            struct StereoConfig {
                // Input mode
                InputMode inputMode = InputMode::SideBySide;

                // Input video configuration (SBS mode)
                uint32_t inputWidth = 3840;   // 4K width
                uint32_t inputHeight = 2160;  // 4K height
                uint32_t eyeWidth = 1920;     // 2K per eye
                uint32_t eyeHeight = 2160;    // Full height per eye

                // Stereo 3D rendering
                float ipd = 63.0f;            // Interpupillary distance (mm)
                float convergence = 5000.0f;  // Convergence plane (mm, Z=-5m for video plane)

                // Output profiles
                struct ProfileOutput {
                    uint32_t outputWidth = 1920;
                    uint32_t outputHeight = 1920;
                    float fov = 110.0f;
                };
                std::vector<ProfileOutput> profiles;  // Quest 3, Index, Vive Pro 2, etc.
            };

            explicit StereoRenderer(VulkanRenderer *renderer, QObject *parent = nullptr);
            ~StereoRenderer() override;

            /**
     * @brief Initialize stereo rendering pipeline
     */
            bool initialize(const StereoConfig &config);

            /**
     * @brief Process a stereo frame (SBS mode)
     * @param sbsVideoFrame 4K SBS input frame (2x 2K L/R)
     * @param deltaTime Time since last frame
     */
            void renderFrame(QRhiTexture *sbsVideoFrame, float deltaTime);

            /**
     * @brief Process a stereo frame (dual-stream mode for AV2 multi-stream)
     * @param leftVideoFrame Left eye video texture
     * @param rightVideoFrame Right eye video texture
     * @param deltaTime Time since last frame
     */
            void renderFrame(QRhiTexture *leftVideoFrame, QRhiTexture *rightVideoFrame, float deltaTime);

            /**
     * @brief Split SBS frame into L/R eye textures
     */
            void splitSBSFrame(QRhiTexture *sbsInput, QRhiTexture *leftOut, QRhiTexture *rightOut);

            /**
     * @brief Render 3D scene for one eye
     * @param eye Which eye to render
     * @param targetTexture Output texture
     */
            void render3DOverlay(EyeIndex eye, QRhiTexture *targetTexture);

              signals:
            void stereoFrameRendered();

              private:
            void createSplitterPipeline();
            void createCompositePipeline();
            void createPerEyeFramebuffers();

            VulkanRenderer *m_renderer;
            StereoConfig m_config;

            // SBS splitter resources
            std::unique_ptr<QRhiShaderResourceBindings> m_splitterBindings;
            std::unique_ptr<QRhiGraphicsPipeline> m_splitterPipeline;

            // Per-eye textures
            std::unique_ptr<QRhiTexture> m_leftVideoTexture;   // Stitched video (L eye)
            std::unique_ptr<QRhiTexture> m_rightVideoTexture;  // Stitched video (R eye)
            std::unique_ptr<QRhiTexture> m_left3DTexture;      // 3D overlays (L eye)
            std::unique_ptr<QRhiTexture> m_right3DTexture;     // 3D overlays (R eye)

            // Composite resources (video + 3D)
            std::unique_ptr<QRhiShaderResourceBindings> m_compositeBindings;
            std::unique_ptr<QRhiGraphicsPipeline> m_compositePipeline;

            bool m_initialized = false;
        };

    }  // namespace Rendering
}  // namespace NeuralStudio
