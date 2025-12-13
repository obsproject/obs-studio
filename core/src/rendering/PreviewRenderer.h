#pragma once

#include <QObject>
#include <QRhi>
#include <QRhiTexture>
#include <memory>

namespace NeuralStudio {
    namespace Rendering {

        class VulkanRenderer;

        /**
 * @brief PreviewRenderer - Low-latency preview for Blueprint/ActiveFrame UI
 * 
 * Provides real-time viewport rendering at reduced resolution for:
 * - Blueprint node graph (scene preview while editing)
 * - ActiveFrame monitoring (live source/output monitoring)
 * 
 * Features:
 * - Downsampled from main render (e.g. 1920x1920 → 960x540)
 * - Monoscopic mode (shows left eye only for simplicity)
 * - Low latency (<16ms target for 60fps UI)
 * - Optional side-by-side stereo preview
 */
        class PreviewRenderer : public QObject
        {
            Q_OBJECT

              public:
            enum class DisplayType {
                DesktopMonitor,  // Standard 2D monitor
                VRHeadset        // VR HMD (OpenXR device)
            };

            enum class PreviewMode {
                Auto,       // Auto-detect display and choose mode
                Desktop2D,  // Monoscopic for desktop monitor (left eye only)
                VRStereo    // True stereo for VR headset (not SBS)
            };

            struct PreviewConfig {
                uint32_t width = 960;
                uint32_t height = 540;
                PreviewMode mode = PreviewMode::Auto;  // Auto-detect by default
                bool showGrid = false;                 // Overlay grid for alignment
                bool showSafeArea = false;             // Show VR safe viewing area
            };

            explicit PreviewRenderer(VulkanRenderer *renderer, QObject *parent = nullptr);
            ~PreviewRenderer() override;

            /**
     * @brief Initialize preview renderer
     */
            bool initialize(const PreviewConfig &config);

            /**
     * @brief Render preview from source textures
     * @param leftEye Left eye source texture (full-res)
     * @param rightEye Right eye source texture (full-res)
     */
            void renderPreview(QRhiTexture *leftEye, QRhiTexture *rightEye);

            /**
     * @brief Get preview output texture
     */
            QRhiTexture *getPreviewTexture()
            {
                return m_previewTexture.get();
            }

            /**
     * @brief Set preview mode
     */
            void setPreviewMode(PreviewMode mode);

            /**
     * @brief Detect current display type
     * @return DisplayType (DesktopMonitor or VRHeadset)
     */
            DisplayType detectDisplayType();

            /**
     * @brief Get current display type
     */
            DisplayType getDisplayType() const
            {
                return m_displayType;
            }

            /**
     * @brief Update preview resolution
     */
            bool resize(uint32_t width, uint32_t height);

              signals:
            void previewRendered();
            void displayTypeChanged(DisplayType newType);

              private:
            void createPreviewResources();
            void releasePreviewResources();
            void renderDesktop2D(QRhiTexture *leftEye);
            void renderVRStereo(QRhiTexture *leftEye, QRhiTexture *rightEye);
            PreviewMode resolvePreviewMode();         // Resolve Auto mode to actual mode
            bool isSBSTexture(QRhiTexture *texture);  // Detect if texture is SBS format
            void extractLeftEyeFromSBS(QRhiTexture *sbsInput, QRhiTexture *leftOutput);  // Extract left half

            VulkanRenderer *m_renderer;
            PreviewConfig m_config;
            DisplayType m_displayType = DisplayType::DesktopMonitor;  // Default to desktop

            // Preview output texture
            std::unique_ptr<QRhiTexture> m_previewTexture;
            std::unique_ptr<QRhiTextureRenderTarget> m_previewTarget;
            std::unique_ptr<QRhiRenderPassDescriptor> m_previewPass;
            std::unique_ptr<QRhiCommandBuffer> m_commandBuffer;  // Command buffer for rendering
            std::unique_ptr<QRhiBuffer> m_vertexBuffer;          // Fullscreen quad
            std::unique_ptr<QRhiSampler> m_sampler;              // Texture sampler

            // Render pipeline
            std::unique_ptr<QRhiShaderResourceBindings> m_bindings;
            std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;

            bool m_initialized = false;
        };

    }  // namespace Rendering
}  // namespace NeuralStudio
