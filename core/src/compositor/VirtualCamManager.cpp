#include "VirtualCamManager.h"
#include <filesystem>
#include <algorithm>
#include <iostream>

// JSON parsing (will use nlohmann/json or similar)
// For now, manual parsing stub

namespace NeuralStudio {

VirtualCamManager::VirtualCamManager() : m_streaming(false), m_sceneManager(nullptr) {}

VirtualCamManager::~VirtualCamManager()
{
	stopStreaming();
}

void VirtualCamManager::loadProfilesFromDirectory(const std::string &profileDir)
{
	namespace fs = std::filesystem;

	if (!fs::exists(profileDir) || !fs::is_directory(profileDir)) {
		std::cerr << "Profile directory does not exist: " << profileDir << std::endl;
		return;
	}

	// Load predefined profiles as fallback
	m_profiles.push_back(Profiles::Quest3());
	m_profiles.push_back(Profiles::ValveIndex());
	m_profiles.push_back(Profiles::VivePro2());

	std::cout << "Loaded " << m_profiles.size() << " VR headset profiles" << std::endl;

	// TODO: Parse JSON files from profileDir and override/add profiles
}

void VirtualCamManager::addProfile(const VRHeadsetProfile &profile)
{
	// Check if profile already exists
	for (auto &p : m_profiles) {
		if (p.id == profile.id) {
			p = profile; // Update existing
			return;
		}
	}
	m_profiles.push_back(profile);
}

void VirtualCamManager::removeProfile(const std::string &profileId)
{
	m_profiles.erase(std::remove_if(m_profiles.begin(), m_profiles.end(),
					[&](const VRHeadsetProfile &p) { return p.id == profileId; }),
			 m_profiles.end());
}

void VirtualCamManager::enableProfile(const std::string &profileId, bool enabled)
{
	for (auto &profile : m_profiles) {
		if (profile.id == profileId) {
			profile.enabled = enabled;

			if (m_streaming) {
				if (enabled) {
					allocateFramebuffer(profile);
					// TODO: Start encoder and SRT stream
				} else {
					freeFramebuffer(profileId);
					// TODO: Stop encoder and SRT stream
				}
			}
			break;
		}
	}
}

std::vector<VRHeadsetProfile> VirtualCamManager::getProfiles() const
{
	return m_profiles;
}

VRHeadsetProfile *VirtualCamManager::getProfile(const std::string &profileId)
{
	for (auto &profile : m_profiles) {
		if (profile.id == profileId) {
			return &profile;
		}
	}
	return nullptr;
}

void VirtualCamManager::startStreaming(SceneManager *sceneManager)
{
	if (m_streaming) {
		std::cerr << "VirtualCam already streaming" << std::endl;
		return;
	}

	m_sceneManager = sceneManager;
	m_streaming = true;

	// Allocate framebuffers for all enabled profiles
	for (const auto &profile : m_profiles) {
		if (profile.enabled) {
			allocateFramebuffer(profile);
			// TODO: Initialize encoder
			// TODO: Initialize SRT stream
			std::cout << "Started streaming for profile: " << profile.name << std::endl;
		}
	}
}

void VirtualCamManager::stopStreaming()
{
	if (!m_streaming) {
		return;
	}

	// Free all framebuffers and stop streams
	for (const auto &profile : m_profiles) {
		if (profile.enabled) {
			freeFramebuffer(profile.id);
			// TODO: Stop encoder
			// TODO: Stop SRT stream
		}
	}

	m_streaming = false;
	m_sceneManager = nullptr;
}

void VirtualCamManager::renderFrame(SceneManager *sceneManager, double deltaTime)
{
	if (!m_streaming) {
		return;
	}

	for (const auto &profile : m_profiles) {
		if (profile.enabled) {
			renderProfileFrame(profile, sceneManager);
		}
	}
}

std::vector<VirtualCamManager::StreamInfo> VirtualCamManager::getActiveStreams() const
{
	std::vector<StreamInfo> streams;

	for (const auto &profile : m_profiles) {
		if (profile.enabled) {
			StreamInfo info;
			info.profileId = profile.id;
			info.profileName = profile.name;
			info.srtUrl = "srt://localhost:" + std::to_string(profile.srtPort);
			info.status = m_streaming ? "streaming" : "initializing";
			info.frameCount = 0; // TODO: Track from encoder
			info.framerate = profile.framerate;
			streams.push_back(info);
		}
	}

	return streams;
}

void VirtualCamManager::allocateFramebuffer(const VRHeadsetProfile &profile)
{
	FramebufferHandle fb;

	// Calculate total width for side-by-side stereo
	uint32_t totalWidth = profile.eyeWidth * 2; // SBS
	uint32_t totalHeight = profile.eyeHeight;

	fb.width = totalWidth;
	fb.height = totalHeight;

	// TODO: Allocate actual GPU buffers (Vulkan/OpenGL)
	// For now, just placeholders
	fb.leftEyeBuffer = nullptr;
	fb.rightEyeBuffer = nullptr;

	m_framebuffers[profile.id] = fb;

	std::cout << "Allocated framebuffer for " << profile.name << ": " << totalWidth << "x" << totalHeight
		  << std::endl;
}

void VirtualCamManager::freeFramebuffer(const std::string &profileId)
{
	auto it = m_framebuffers.find(profileId);
	if (it != m_framebuffers.end()) {
		// TODO: Free GPU buffers
		m_framebuffers.erase(it);
	}
}

void VirtualCamManager::renderProfileFrame(const VRHeadsetProfile &profile, SceneManager *sceneManager)
{
	auto it = m_framebuffers.find(profile.id);
	if (it == m_framebuffers.end()) {
		return;
	}

	FramebufferHandle &fb = it->second;

	// TODO: Stereo rendering
	// 1. Calculate left eye camera position (offset by -IPD/2)
	// 2. Render scene to left half of framebuffer
	// 3. Calculate right eye camera position (offset by +IPD/2)
	// 4. Render scene to right half of framebuffer

	// TODO: Encode and stream
	// encodeAndStream(profile.id, fb);
}

void VirtualCamManager::encodeAndStream(const std::string &profileId, const FramebufferHandle &fb)
{
	// TODO: Pass framebuffer data to encoder
	// TODO: Get encoded packets
	// TODO: Send to SRT stream
}

} // namespace NeuralStudio
