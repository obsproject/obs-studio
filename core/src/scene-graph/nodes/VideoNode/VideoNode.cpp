#include "VideoNode.h"
#include <iostream>

namespace NeuralStudio {
namespace SceneGraph {

VideoNode::VideoNode(const std::string &id) : BaseNodeBackend(id, "VideoNode")
{
	// Define Outputs - video is a visual source
	addOutput("visual_out", "Visual Output", DataType::Video());
	addOutput("audio_out", "Audio Output", DataType::Audio());
}

VideoNode::~VideoNode()
{
	// Cleanup video player resources
}

void VideoNode::execute(const ExecutionContext &context)
{
	if (m_dirty && !m_videoPath.empty()) {
		// Load/initialize video player
		std::cout << "VideoNode: Loading " << m_videoPath << std::endl;
		m_dirty = false;
	}

	// Decode current frame and output texture + audio
	if (m_videoPlayerId > 0) {
		// setOutputData("visual_out", currentFrameTexture);
		// setOutputData("audio_out", currentAudioBuffer);
	}
}

void VideoNode::setVideoPath(const std::string &path)
{
	if (m_videoPath != path) {
		m_videoPath = path;
		m_dirty = true;
	}
}

std::string VideoNode::getVideoPath() const
{
	return m_videoPath;
}

void VideoNode::setLoop(bool loop)
{
	m_loop = loop;
}

bool VideoNode::getLoop() const
{
	return m_loop;
}

} // namespace SceneGraph
} // namespace NeuralStudio
