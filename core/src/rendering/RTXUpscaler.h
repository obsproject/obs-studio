#pragma once

#include <QObject>
#include <QRhiTexture>
#include <memory>

// Forward declarations for NVIDIA SDK types
struct NvVFX_Handle_s;
typedef struct NvVFX_Handle_s *NvVFX_Handle;
struct NvCVImage;

namespace NeuralStudio {
    namespace Rendering {

        class VulkanRenderer;

        /**
 * @brief RTXUpscaler - NVIDIA AI Super Resolution for 4K→8K upscaling
 * 
 * Uses NVIDIA Maxine Video Effects SDK (Tensor Cores) to upscale
 * per-eye stereo frames from 4K to 8K for high-end VR headsets.
 * 
 * Pipeline:
 * 1. Qt RHI Texture (Vulkan) → CUDA buffer (via external memory API)
 * 2. NVIDIA AI Super Resolution (Mode 1 - High Quality, 2x scale)
 * 3. CUDA buffer → Qt RHI Texture (Vulkan)
 * 
 * Requirements:
 * - NVIDIA RTX GPU (2060+, recommended 4090/5090)
 * - NVIDIA Video Effects SDK installed
 * - CUDA Toolkit 11.0+
 */
        class RTXUpscaler : public QObject
        {
            Q_OBJECT

              public:
            enum class QualityMode {
                Performance = 0,  // Faster, lower quality
                HighQuality = 1   // Slower, better quality (recommended)
            };

            enum class ScaleFactor {
                Scale_1_33x = 0,  // 1.33x
                Scale_1_5x = 1,   // 1.5x
                Scale_2x = 2,     // 2x (4K → 8K)
                Scale_3x = 3,     // 3x
                Scale_4x = 4      // 4x
            };

            explicit RTXUpscaler(VulkanRenderer *renderer, QObject *parent = nullptr);
            ~RTXUpscaler() override;

            /**
     * @brief Initialize NVIDIA SDK and create upscaling effect
     * @param width Input width per eye
     * @param height Input height per eye
     * @param scale Scale factor (default 2x for 4K→8K)
     * @param quality Quality mode (default High Quality)
     * @return true if initialized successfully
     */
            bool initialize(uint32_t width, uint32_t height, ScaleFactor scale = ScaleFactor::Scale_2x,
                            QualityMode quality = QualityMode::HighQuality);

            /**
     * @brief Upscale a texture using NVIDIA AI
     * @param input Source texture (e.g. 1920x1920)
     * @param output Destination texture (e.g. 3840x3840)
     * @return true if upscaling succeeded
     */
            bool upscale(QRhiTexture *input, QRhiTexture *output);

            /**
     * @brief Check if NVIDIA SDK is available
     */
            static bool isAvailable();

            /**
     * @brief Get NVIDIA SDK version
     */
            static unsigned int getSDKVersion();

            /**
     * @brief Set quality mode
     */
            void setQualityMode(QualityMode mode);

            /**
     * @brief Get current input/output resolution
     */
            void getResolution(uint32_t &inWidth, uint32_t &inHeight, uint32_t &outWidth, uint32_t &outHeight) const;

              signals:
            void upscaleCompleted();
            void upscaleFailed(const QString &error);

              private:
            bool loadNVIDIASDK();
            void unloadNVIDIASDK();
            bool createNVVFXEffect();
            void destroyNVVFXEffect();
            bool createCUDABuffers();
            void destroyCUDABuffers();
            bool mapVulkanToCUDA(QRhiTexture *texture, void **cudaBuffer);
            void unmapVulkanFromCUDA(void *cudaBuffer);

            VulkanRenderer *m_renderer;

            // NVIDIA SDK handles
            NvVFX_Handle m_nvvfxHandle = nullptr;
            void *m_nvvfxLibrary = nullptr;  // Dynamic library handle

            // CUDA resources
            NvCVImage *m_cudaInputImage = nullptr;
            NvCVImage *m_cudaOutputImage = nullptr;
            void *m_cudaStream = nullptr;  // CUstream

            // CUDA-GL interop tracking (Ubuntu/Linux)
            void *m_cudaInputResource = nullptr;   // cudaGraphicsResource_t
            void *m_cudaOutputResource = nullptr;  // cudaGraphicsResource_t
            uint32_t m_lastInputTexID = 0;
            uint32_t m_lastOutputTexID = 0;
            bool m_inputRegistered = false;
            bool m_outputRegistered = false;
            void *m_currentlyMappedInput = nullptr;
            void *m_currentlyMappedOutput = nullptr;

            // Configuration
            uint32_t m_inputWidth = 0;
            uint32_t m_inputHeight = 0;
            uint32_t m_outputWidth = 0;
            uint32_t m_outputHeight = 0;
            ScaleFactor m_scaleFactor = ScaleFactor::Scale_2x;
            QualityMode m_qualityMode = QualityMode::HighQuality;

            bool m_initialized = false;
            bool m_sdkAvailable = false;
        };

    }  // namespace Rendering
}  // namespace NeuralStudio
