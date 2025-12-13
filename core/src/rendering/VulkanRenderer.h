#pragma once

#include <QObject>
#include <QRhi>
#include <QRhiCommandBuffer>
#include <QRhiRenderTarget>
#include <QRhiSwapChain>
#include <memory>
#include <vector>

namespace NeuralStudio {
namespace Rendering {

/**
 * @brief VulkanRenderer - Core VR180/360 rendering engine using Qt RHI
 * 
 * This renderer provides hardware-accelerated 3D rendering using Qt's RHI
 * (Rendering Hardware Interface), which abstracts Vulkan, OpenGL, and Metal.
 * 
 * Features:
 * - Multi-pass stereo rendering (L/R eyes)
 * - VR headset profile support (Quest 3, Index, Vive Pro 2)
 * - STMap-based fisheye stitching
 * - HDR support (Rec.2020 PQ/HLG)
 * - Preview rendering for Blueprint/ActiveFrame
 */
class VulkanRenderer : public QObject
{
    Q_OBJECT

public:
    struct RenderConfig {
        uint32_t eyeWidth = 1920;
        uint32_t eyeHeight = 1920;
        float fovHorizontal = 110.0f;
        float fovVertical = 96.0f;
        float ipd = 63.0f;  // mm
        bool stereoEnabled = true;
        bool hdrEnabled = false;
    };

    explicit VulkanRenderer(QObject *parent = nullptr);
    ~VulkanRenderer() override;

    /**
     * @brief Initialize the Vulkan rendering context
     * @param window Native window handle for swap chain
     * @return true if initialization succeeded
     */
    bool initialize(QWindow *window);

    /**
     * @brief Set rendering configuration
     */
    void setConfig(const RenderConfig &config);

    /**
     * @brief Render a frame
     * @param deltaTime Time since last frame (seconds)
     */
    void renderFrame(float deltaTime);

    /**
     * @brief Get the RHI instance
     */
    QRhi* rhi() const { return m_rhi.get(); }

signals:
    void frameRendered();
    void errorOccurred(const QString &error);

private:
    void createResources();
    void releaseResources();
    void createFramebuffers();
    void createCommandBuffers();

    // Qt RHI resources
    std::unique_ptr<QRhi> m_rhi;
    std::unique_ptr<QRhiSwapChain> m_swapChain;
    std::unique_ptr<QRhiRenderPassDescriptor> m_renderPass;
    
    std::vector<std::unique_ptr<QRhiTextureRenderTarget>> m_eyeFramebuffers;
    std::vector<std::unique_ptr<QRhiCommandBuffer>> m_commandBuffers;

    RenderConfig m_config;
    bool m_initialized = false;
    QWindow *m_window = nullptr;
};

} // namespace Rendering
} // namespace NeuralStudio
