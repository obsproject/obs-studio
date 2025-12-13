#include "VulkanRenderer.h"
#include <QRhiVulkanInitParams>
#include <QVulkanInstance>
#include <QDebug>

namespace NeuralStudio {
namespace Rendering {

VulkanRenderer::VulkanRenderer(QObject *parent) : QObject(parent) {}

VulkanRenderer::~VulkanRenderer()
{
	releaseResources();
}

bool VulkanRenderer::initialize(QWindow *window)
{
	if (m_initialized) {
		qWarning() << "VulkanRenderer already initialized";
		return true;
	}

	m_window = window;

	// Create Vulkan instance for Qt RHI
	QVulkanInstance *vulkanInstance = new QVulkanInstance();
	vulkanInstance->setLayers({"VK_LAYER_KHRONOS_validation"}); // Debug mode
	vulkanInstance->setExtensions(QByteArrayList() << "VK_KHR_get_physical_device_properties2");

	if (!vulkanInstance->create()) {
		qCritical() << "Failed to create Vulkan instance";
		emit errorOccurred("Failed to create Vulkan instance");
		return false;
	}

	// Initialize Qt RHI with Vulkan backend
	QRhiVulkanInitParams params;
	params.inst = vulkanInstance;
	params.window = window;

	m_rhi.reset(QRhi::create(QRhi::Vulkan, &params));
	if (!m_rhi) {
		qCritical() << "Failed to create QRhi instance";
		emit errorOccurred("Failed to create QRhi");
		return false;
	}

	// Create swap chain
	m_swapChain.reset(m_rhi->newSwapChain());
	m_swapChain->setWindow(window);
	m_swapChain->setSampleCount(1); // No MSAA for VR (performance)

	m_renderPass.reset(m_swapChain->newCompatibleRenderPassDescriptor());
	m_swapChain->setRenderPassDescriptor(m_renderPass.get());

	if (!m_swapChain->createOrResize()) {
		qCritical() << "Failed to create swap chain";
		emit errorOccurred("Failed to create swap chain");
		return false;
	}

	// Create rendering resources
	createResources();

	m_initialized = true;
	qInfo() << "VulkanRenderer initialized successfully";
	qInfo() << "  Backend:" << m_rhi->backendName();
	qInfo() << "  Device:" << m_rhi->driverInfo();

	return true;
}

void VulkanRenderer::setConfig(const RenderConfig &config)
{
	m_config = config;

	if (m_initialized) {
		// Recreate framebuffers with new resolution
		createFramebuffers();
	}
}

void VulkanRenderer::renderFrame(float deltaTime)
{
	if (!m_initialized || !m_swapChain) {
		return;
	}

	// Begin frame
	QRhi::FrameOpResult result = m_rhi->beginFrame(m_swapChain.get());
	if (result != QRhi::FrameOpSuccess) {
		if (result == QRhi::FrameOpSwapChainOutOfDate) {
			m_swapChain->createOrResize();
		}
		return;
	}

	// Get command buffer
	QRhiCommandBuffer *cb = m_swapChain->currentFrameCommandBuffer();

	// Clear color (dark blue for VR void)
	QRhiRenderTarget *rt = m_swapChain->currentFrameRenderTarget();
	const QColor clearColor = QColor::fromRgbF(0.02f, 0.02f, 0.05f, 1.0f);

	cb->beginPass(rt, clearColor, {1, 0});

	// TODO: Stereo rendering will go here
	// - Render left eye
	// - Render right eye
	// - Composite to swap chain

	cb->endPass();

	// End frame
	m_rhi->endFrame(m_swapChain.get());

	emit frameRendered();
}

void VulkanRenderer::createResources()
{
	createFramebuffers();
	createCommandBuffers();
}

void VulkanRenderer::releaseResources()
{
	m_commandBuffers.clear();
	m_eyeFramebuffers.clear();
	m_renderPass.reset();
	m_swapChain.reset();
	m_rhi.reset();

	m_initialized = false;
}

void VulkanRenderer::createFramebuffers()
{
	m_eyeFramebuffers.clear();

	if (!m_config.stereoEnabled) {
		return; // Mono rendering uses swap chain directly
	}

	// Create framebuffers for each eye
	for (int eye = 0; eye < 2; ++eye) {
		// Create texture for eye rendering
		std::unique_ptr<QRhiTexture> eyeTexture(
			m_rhi->newTexture(QRhiTexture::RGBA8, QSize(m_config.eyeWidth, m_config.eyeHeight), 1,
					  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));

		if (!eyeTexture->create()) {
			qCritical() << "Failed to create eye texture" << eye;
			continue;
		}

		// Create render target
		std::unique_ptr<QRhiTextureRenderTarget> eyeTarget(m_rhi->newTextureRenderTarget({eyeTexture.get()}));

		std::unique_ptr<QRhiRenderPassDescriptor> eyePass(eyeTarget->newCompatibleRenderPassDescriptor());
		eyeTarget->setRenderPassDescriptor(eyePass.get());

		if (!eyeTarget->create()) {
			qCritical() << "Failed to create eye render target" << eye;
			continue;
		}

		m_eyeFramebuffers.push_back(std::move(eyeTarget));

		qDebug() << "Created framebuffer for eye" << eye << "- Resolution:" << m_config.eyeWidth << "x"
			 << m_config.eyeHeight;
	}
}

void VulkanRenderer::createCommandBuffers()
{
	// Command buffers are managed by QRhi per frame
	// This is a placeholder for future multi-threaded command recording
}

} // namespace Rendering
} // namespace NeuralStudio
