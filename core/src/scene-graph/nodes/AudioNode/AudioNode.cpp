#include "AudioNode.h"
#include <iostream>

namespace NeuralStudio {
namespace SceneGraph {

AudioNode::AudioNode(const std::string &id) : BaseNodeBackend(id, "AudioNode")
{
	// Define Outputs - audio is a sound source
	addOutput("audio_out", "Audio Output", DataType::Audio());
}

AudioNode::~AudioNode()
{
	// Cleanup audio player resources
}

void AudioNode::execute(const ExecutionContext &context)
{
	if (m_dirty && !m_audioPath.empty()) {
		// Load audio file
		std::cout << "AudioNode: Loading " << m_audioPath << std::endl;

		// Future: Load using SDL_mixer, miniaudio, or similar
		// m_audioPlayerId = AudioEngine::load(m_audioPath);
		// Extract metadata: m_sampleRate, m_channels, m_duration

		m_dirty = false;
	}

	// Output audio stream
	if (m_audioPlayerId > 0) {
		// Audio data with volume and loop applied
		// setOutputData("audio_out", currentAudioBuffer);
	}
}

void AudioNode::setAudioPath(const std::string &path)
{
	if (m_audioPath != path) {
		m_audioPath = path;
		m_dirty = true;
	}
}

std::string AudioNode::getAudioPath() const
{
	return m_audioPath;
}

void AudioNode::setLoop(bool loop)
{
	m_loop = loop;
}

bool AudioNode::getLoop() const
{
	return m_loop;
}

void AudioNode::setVolume(float volume)
{
	m_volume = std::max(0.0f, std::min(1.0f, volume)); // Clamp [0,1]
}

float AudioNode::getVolume() const
{
	return m_volume;
}

} // namespace SceneGraph
} // namespace NeuralStudio
