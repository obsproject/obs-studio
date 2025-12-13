#pragma once

#include <QObject>
#include <QRhiTexture>
#include <memory>

namespace NeuralStudio {
    namespace Rendering {

        class VulkanRenderer;

        /**
 * @brief HDRProcessor - HDR color space conversion and tone mapping
 * 
 * Handles Rec.709 (SDR) ↔ Rec.2020 (HDR) conversions with PQ/HLG support.
 * Designed as a processing node for Blueprint graph.
 * 
 * Use Cases:
 * - SDR→HDR: Expand Rec.709 to Rec.2020 for HDR streaming
 * - HDR→SDR: Tone map PQ/HLG to Rec.709 for SDR monitors
 * - HDR→HDR: EOTF conversion (PQ ↔ HLG)
 * 
 * Blueprint Node Parameters:
 * - Input Color Space: Rec.709 / Rec.2020
 * - Output Color Space: Rec.709 / Rec.2020
 * - EOTF: Linear / PQ / HLG
 * - Tone Map Mode: Passthrough / ACES / Reinhard / Hable
 */
        class HDRProcessor : public QObject
        {
            Q_OBJECT

              public:
            enum class ColorSpace {
                Rec709,  // SDR (sRGB-ish)
                Rec2020  // HDR wide gamut
            };

            enum class EOTF {
                Linear,  // No transfer function
                sRGB,    // Standard sRGB gamma
                PQ,      // Perceptual Quantizer (HDR10)
                HLG      // Hybrid Log-Gamma (HDR broadcast)
            };

            enum class ToneMapMode {
                Passthrough,  // No tone mapping
                ACES,         // ACES Filmic (industry standard)
                Reinhard,     // Simple Reinhard
                Hable         // Uncharted 2 (game-style)
            };

            struct HDRConfig {
                ColorSpace inputColorSpace = ColorSpace::Rec709;
                ColorSpace outputColorSpace = ColorSpace::Rec709;
                EOTF inputEOTF = EOTF::sRGB;
                EOTF outputEOTF = EOTF::sRGB;
                ToneMapMode toneMapMode = ToneMapMode::Passthrough;
                float exposure = 1.0f;          // Exposure adjustment (0.1 - 10.0)
                float maxLuminance = 10000.0f;  // Max nits for PQ (100 - 10000)
            };

            explicit HDRProcessor(VulkanRenderer *renderer, QObject *parent = nullptr);
            ~HDRProcessor() override;

            /**
     * @brief Initialize HDR processing pipeline
     */
            bool initialize(const HDRConfig &config);

            /**
     * @brief Process a frame (color space conversion + tone mapping)
     * @param input Source texture
     * @param output Destination texture (can be same as input for in-place)
     */
            bool process(QRhiTexture *input, QRhiTexture *output);

            /**
     * @brief Update configuration (rebuilds shader if needed)
     */
            void setConfig(const HDRConfig &config);

            /**
     * @brief Get current configuration
     */
            HDRConfig getConfig() const
            {
                return m_config;
            }

            /**
     * @brief Auto-detect input color space from texture metadata
     */
            ColorSpace detectColorSpace(QRhiTexture *texture);

              signals:
            void processingCompleted();
            void processingFailed(const QString &error);

              private:
            void createShaderPipeline();
            void releaseShaderPipeline();
            void updateShaderConstants();

            VulkanRenderer *m_renderer;
            HDRConfig m_config;

            // Vulkan compute pipeline
            std::unique_ptr<QRhiShaderResourceBindings> m_bindings;
            std::unique_ptr<QRhiComputePipeline> m_pipeline;

            bool m_initialized = false;
        };

    }  // namespace Rendering
}  // namespace NeuralStudio
