#include "StereoRenderer.h"
#include <QDebug>

namespace NeuralStudio {
namespace Rendering {

StereoRenderer::StereoRenderer(VulkanRenderer *renderer, QObject *parent) : QObject(parent), m_renderer(renderer) {}

StereoRenderer::~StereoRenderer() {}

bool StereoRenderer::initialize(const StereoConfig &config)
{
	if (m_initialized) {
		qWarning() << "StereoRenderer already initialized";
		return true;
	}

	if (!m_renderer || !m_renderer->rhi()) {
		qCritical() << "Invalid VulkanRenderer";
		return false;
	}

	m_config = config;

	// Create per-eye textures
	createPerEyeFramebuffers();

	// Create SBS splitter pipeline (splits 4K SBS → 2x 2K)
	createSplitterPipeline();

	// Create composite pipeline (video + 3D overlays)
	createCompositePipeline();

	m_initialized = true;
	qInfo() << "StereoRenderer initialized";
	qInfo() << "  Input: " << m_config.inputWidth << "x" << m_config.inputHeight << "(SBS)";
	qInfo() << "  Per-eye:" << m_config.eyeWidth << "x" << m_config.eyeHeight;
	qInfo() << "  IPD:" << m_config.ipd << "mm";
	qInfo() << "  Convergence:" << m_config.convergence << "mm (Z=-5m video plane)";

	return true;
}

void StereoRenderer::renderFrame(QRhiTexture *sbsVideoFrame, float deltaTime)
{
	if (!m_initialized || !sbsVideoFrame) {
		return;
	}

	QRhi *rhi = m_renderer->rhi();
	if (!rhi)
		return;

	// Step 1: Split SBS 4K input into L/R 2K textures
	splitSBSFrame(sbsVideoFrame, m_leftVideoTexture.get(), m_rightVideoTexture.get());

	// Step 2: Apply STMap stitching to each eye
	// TODO: Call STMapLoader and apply fisheye→equirect shader
	// For now, pass-through (assume video is already equirectangular)

	// Step 3: Render 3D overlays for left eye
	render3DOverlay(EyeIndex::Left, m_left3DTexture.get());

	// Step 4: Render 3D overlays for right eye (with IPD offset)
	render3DOverlay(EyeIndex::Right, m_right3DTexture.get());

	// Step 5: Composite video + 3D for each eye
	// TODO: Blend m_leftVideoTexture + m_left3DTexture → final left output
	// TODO: Blend m_rightVideoTexture + m_right3DTexture → final right output

	// Step 6: Output to per-headset profile streams
	// TODO: Encode and stream based on profile configs

	emit stereoFrameRendered();
}

void StereoRenderer::renderFrame(QRhiTexture *leftVideoFrame, QRhiTexture *rightVideoFrame, float deltaTime)
{
	if (!m_initialized || !leftVideoFrame || !rightVideoFrame) {
		return;
	}

	// AV2 multi-stream mode: already have separate L/R textures
	// Step 1: Apply STMap stitching to each eye
	// TODO: Apply fisheye→equirect shader to leftVideoFrame → m_leftVideoTexture
	// TODO: Apply fisheye→equirect shader to rightVideoFrame → m_rightVideoTexture

	// Step 2: Render 3D overlays for left eye
	render3DOverlay(EyeIndex::Left, m_left3DTexture.get());

	// Step 3: Render 3D overlays for right eye (with IPD offset)
	render3DOverlay(EyeIndex::Right, m_right3DTexture.get());

	// Step 4: Composite video + 3D for each eye
	// TODO: Blend m_leftVideoTexture + m_left3DTexture → final left output
	// TODO: Blend m_rightVideoTexture + m_right3DTexture → final right output

	// Step 5: Output to per-headset profile streams
	// TODO: Encode and stream based on profile configs

	qDebug() << "Dual-stream mode (AV2 multi-stream)";
	emit stereoFrameRendered();
}

void StereoRenderer::splitSBSFrame(QRhiTexture *sbsInput, QRhiTexture *leftOut, QRhiTexture *rightOut)
{
	// SBS split shader:
	// Left eye: UV.x range [0.0, 0.5]  → sample left half
	// Right eye: UV.x range [0.5, 1.0] → sample right half

	QRhi *rhi = m_renderer->rhi();
	if (!rhi || !m_splitterPipeline)
		return;

	// TODO: Implement compute shader or render pass to split texture
	// For now, placeholder
	qDebug() << "Splitting SBS frame: 4K → 2x 2K";
}

void StereoRenderer::render3DOverlay(EyeIndex eye, QRhiTexture *targetTexture)
{
	// Render 3D scene with camera offset based on IPD
	// Left eye: camera.x = -IPD/2
	// Right eye: camera.x = +IPD/2

	float eyeOffset = (eye == EyeIndex::Left) ? -m_config.ipd / 2.0f : m_config.ipd / 2.0f;

	QRhi *rhi = m_renderer->rhi();
	if (!rhi)
		return;

	// TODO: Set camera view matrix with IPD offset
	// TODO: Render all 3D objects (positioned at various Z-depths)
	// TODO: Video plane is at Z=-5m (convergence point)
	// TODO: 3D overlays can be in front (Z > -5m) or behind (Z < -5m)

	qDebug() << "Rendering 3D overlay for" << (eye == EyeIndex::Left ? "LEFT" : "RIGHT")
		 << "eye, offset=" << eyeOffset << "mm";
}

void StereoRenderer::createPerEyeFramebuffers()
{
	QRhi *rhi = m_renderer->rhi();
	if (!rhi)
		return;

	// Left video texture (stitched 2K equirectangular)
	m_leftVideoTexture.reset(rhi->newTexture(QRhiTexture::RGBA8, QSize(m_config.eyeWidth, m_config.eyeHeight), 1,
						 QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
	m_leftVideoTexture->create();

	// Right video texture (stitched 2K equirectangular)
	m_rightVideoTexture.reset(rhi->newTexture(QRhiTexture::RGBA8, QSize(m_config.eyeWidth, m_config.eyeHeight), 1,
						  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
	m_rightVideoTexture->create();

	// Left 3D overlay texture
	m_left3DTexture.reset(rhi->newTexture(QRhiTexture::RGBA8, QSize(m_config.eyeWidth, m_config.eyeHeight), 1,
					      QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
	m_left3DTexture->create();

	// Right 3D overlay texture
	m_right3DTexture.reset(rhi->newTexture(QRhiTexture::RGBA8, QSize(m_config.eyeWidth, m_config.eyeHeight), 1,
					       QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
	m_right3DTexture->create();

	qInfo() << "Created stereo framebuffers (4 textures, 2K each)";
}

void StereoRenderer::createSplitterPipeline()
{
	// TODO: Create shader pipeline for SBS split
	// Vertex shader: Full-screen quad
	// Fragment shader: Sample left half or right half based on target
	qInfo() << "SBS splitter pipeline created (placeholder)";
}

void StereoRenderer::createCompositePipeline()
{
	// TODO: Create shader pipeline for video + 3D composite
	// Blend stitched video texture with 3D overlay texture
	// Alpha blending for transparent 3D objects
	qInfo() << "Composite pipeline created (placeholder)";
}

} // namespace Rendering
} // namespace NeuralStudio
