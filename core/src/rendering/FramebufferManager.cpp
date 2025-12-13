#include "FramebufferManager.h"
#include <QDebug>
#include <algorithm>

namespace NeuralStudio {
namespace Rendering {

FramebufferManager::FramebufferManager(QRhi *rhi, QObject *parent) : QObject(parent), m_rhi(rhi) {}

FramebufferManager::~FramebufferManager()
{
	// Release all framebuffers
	for (const auto &profile : m_profiles) {
		releaseFramebuffersForProfile(profile.id);
	}
}

bool FramebufferManager::addProfile(const ProfileConfig &config)
{
	// Check if profile already exists
	auto it = std::find_if(m_profiles.begin(), m_profiles.end(),
			       [&config](const ProfileConfig &p) { return p.id == config.id; });

	if (it != m_profiles.end()) {
		qWarning() << "Profile already exists:" << QString::fromStdString(config.id);
		return false;
	}

	if (!m_rhi) {
		qCritical() << "Invalid QRhi instance";
		return false;
	}

	// Add profile
	m_profiles.push_back(config);

	// Create framebuffers if profile is enabled
	if (config.enabled) {
		createFramebuffersForProfile(config);
	}

	qInfo() << "Added VR profile:" << QString::fromStdString(config.name);
	qInfo() << "  Resolution:" << config.eyeWidth << "x" << config.eyeHeight << "per eye";
	qInfo() << "  FOV:" << config.fovHorizontal << "°H x" << config.fovVertical << "°V";
	qInfo() << "  Refresh:" << config.refreshRate << "Hz";
	qInfo() << "  Codec:" << QString::fromStdString(config.codec) << "@" << config.bitrate << "kbps";

	emit profileAdded(QString::fromStdString(config.id));
	return true;
}

void FramebufferManager::removeProfile(const std::string &profileId)
{
	// Remove framebuffers
	releaseFramebuffersForProfile(profileId);

	// Remove profile
	m_profiles.erase(std::remove_if(m_profiles.begin(), m_profiles.end(),
					[&profileId](const ProfileConfig &p) { return p.id == profileId; }),
			 m_profiles.end());

	qInfo() << "Removed profile:" << QString::fromStdString(profileId);
	emit profileRemoved(QString::fromStdString(profileId));
}

void FramebufferManager::setProfileEnabled(const std::string &profileId, bool enabled)
{
	auto it = std::find_if(m_profiles.begin(), m_profiles.end(),
			       [&profileId](const ProfileConfig &p) { return p.id == profileId; });

	if (it == m_profiles.end()) {
		qWarning() << "Profile not found:" << QString::fromStdString(profileId);
		return;
	}

	if (it->enabled == enabled) {
		return; // No change
	}

	it->enabled = enabled;

	if (enabled) {
		// Create framebuffers
		createFramebuffersForProfile(*it);
		qInfo() << "Enabled profile:" << QString::fromStdString(profileId);
	} else {
		// Release framebuffers
		releaseFramebuffersForProfile(profileId);
		qInfo() << "Disabled profile:" << QString::fromStdString(profileId);
	}
}

FramebufferManager::FramebufferSet *FramebufferManager::getFramebufferSet(const std::string &profileId)
{
	auto it = std::find_if(m_framebuffers.begin(), m_framebuffers.end(),
			       [&profileId](const FramebufferSet &fb) { return fb.profileId == profileId; });

	if (it != m_framebuffers.end()) {
		return &(*it);
	}

	return nullptr;
}

std::vector<std::string> FramebufferManager::getActiveProfiles() const
{
	std::vector<std::string> active;
	for (const auto &profile : m_profiles) {
		if (profile.enabled) {
			active.push_back(profile.id);
		}
	}
	return active;
}

void FramebufferManager::swapBuffers(const std::string &profileId)
{
	auto *fbSet = getFramebufferSet(profileId);
	if (fbSet) {
		// Rotate triple buffers
		fbSet->writeBufferIndex = (fbSet->writeBufferIndex + 1) % fbSet->NumBuffers;
		fbSet->renderBufferIndex = (fbSet->renderBufferIndex + 1) % fbSet->NumBuffers;
		fbSet->displayBufferIndex = (fbSet->displayBufferIndex + 1) % fbSet->NumBuffers;
	}
}

bool FramebufferManager::resizeProfile(const std::string &profileId, uint32_t width, uint32_t height)
{
	auto it = std::find_if(m_profiles.begin(), m_profiles.end(),
			       [&profileId](const ProfileConfig &p) { return p.id == profileId; });

	if (it == m_profiles.end()) {
		return false;
	}

	// Update resolution
	it->eyeWidth = width;
	it->eyeHeight = height;

	// Recreate framebuffers
	if (it->enabled) {
		releaseFramebuffersForProfile(profileId);
		createFramebuffersForProfile(*it);
		emit framebuffersRecreated();
	}

	qInfo() << "Resized profile" << QString::fromStdString(profileId) << "to" << width << "x" << height;
	return true;
}

void FramebufferManager::createFramebuffersForProfile(const ProfileConfig &config)
{
	FramebufferSet fbSet;
	fbSet.profileId = config.id;

	// Create triple-buffered textures for each eye
	for (int i = 0; i < fbSet.NumBuffers; ++i) {
		// Left eye texture
		fbSet.leftEyeTextures[i].reset(
			m_rhi->newTexture(QRhiTexture::RGBA8, QSize(config.eyeWidth, config.eyeHeight), 1,
					  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));

		if (!fbSet.leftEyeTextures[i]->create()) {
			qCritical() << "Failed to create left eye texture" << i << "for"
				    << QString::fromStdString(config.id);
			return;
		}

		// Right eye texture
		fbSet.rightEyeTextures[i].reset(
			m_rhi->newTexture(QRhiTexture::RGBA8, QSize(config.eyeWidth, config.eyeHeight), 1,
					  QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));

		if (!fbSet.rightEyeTextures[i]->create()) {
			qCritical() << "Failed to create right eye texture" << i << "for"
				    << QString::fromStdString(config.id);
			return;
		}

		// Left eye render target
		fbSet.leftEyeTargets[i].reset(m_rhi->newTextureRenderTarget({fbSet.leftEyeTextures[i].get()}));
		fbSet.leftEyePasses[i].reset(fbSet.leftEyeTargets[i]->newCompatibleRenderPassDescriptor());
		fbSet.leftEyeTargets[i]->setRenderPassDescriptor(fbSet.leftEyePasses[i].get());

		if (!fbSet.leftEyeTargets[i]->create()) {
			qCritical() << "Failed to create left eye render target" << i << "for"
				    << QString::fromStdString(config.id);
			return;
		}

		// Right eye render target
		fbSet.rightEyeTargets[i].reset(m_rhi->newTextureRenderTarget({fbSet.rightEyeTextures[i].get()}));
		fbSet.rightEyePasses[i].reset(fbSet.rightEyeTargets[i]->newCompatibleRenderPassDescriptor());
		fbSet.rightEyeTargets[i]->setRenderPassDescriptor(fbSet.rightEyePasses[i].get());

		if (!fbSet.rightEyeTargets[i]->create()) {
			qCritical() << "Failed to create right eye render target" << i << "for"
				    << QString::fromStdString(config.id);
			return;
		}
	}

	// Add to framebuffer list
	m_framebuffers.push_back(std::move(fbSet));

	qInfo() << "Created triple-buffered framebuffers (3x buffers) for profile" << QString::fromStdString(config.id);
	qInfo() << "  Total textures:" << (fbSet.NumBuffers * 2) << "(" << fbSet.NumBuffers << "L +" << fbSet.NumBuffers
		<< "R)";
}

void FramebufferManager::releaseFramebuffersForProfile(const std::string &profileId)
{
	m_framebuffers.erase(
		std::remove_if(m_framebuffers.begin(), m_framebuffers.end(),
			       [&profileId](const FramebufferSet &fb) { return fb.profileId == profileId; }),
		m_framebuffers.end());

	qDebug() << "Released framebuffers for profile" << QString::fromStdString(profileId);
}

} // namespace Rendering
} // namespace NeuralStudio
