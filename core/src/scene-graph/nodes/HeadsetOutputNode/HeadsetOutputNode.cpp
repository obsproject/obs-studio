#include "HeadsetOutputNode.h"
#include "../NodeFactory.h"

namespace NeuralStudio {
namespace SceneGraph {

HeadsetOutputNode::HeadsetOutputNode(const std::string &nodeId) : BaseNodeBackend(nodeId, "HeadsetOutput")
{
	// Setup metadata
	NodeMetadata meta;
	meta.displayName = "Headset Output";
	meta.category = "Output";
	meta.description = "Output to VR headsets (Quest 3, Index, Vive Pro 2) with per-profile encoding";
	meta.tags = {"output", "vr", "headset", "streaming"};
	meta.nodeColorRGB = 0xFFFF4444; // Red for output
	meta.icon = ":/icons/nodes/headset_output.svg";
	meta.supportsGPU = true;
	meta.computeRequirement = ComputeRequirement::High;
	meta.estimatedMemoryMB = 200;
	setMetadata(meta);

	// Define pins
	addInput("video_in", "Video Input", DataType::Texture2D());
	addInput("profile", "Headset Profile", DataType::String());
}

bool HeadsetOutputNode::initialize(const NodeConfig &config)
{
	m_profileId = config.get<std::string>("profile_id", "quest3");

	// VirtualCamManager expected to be provided via context
	m_initialized = true;
	return true;
}

ExecutionResult HeadsetOutputNode::process(ExecutionContext &ctx)
{
	if (!m_initialized) {
		return ExecutionResult::failure("HeadsetOutputNode not initialized");
	}

	// Get VirtualCamManager from shared data or renderer context
	if (!m_virtualCamManager) {
		// Try to get from shared context data
		if (ctx.sharedData.find("virtualCamManager") != ctx.sharedData.end()) {
			try {
				m_virtualCamManager =
					std::any_cast<VirtualCamManager *>(ctx.sharedData["virtualCamManager"]);
			} catch (...) {
				return ExecutionResult::failure("Invalid VirtualCamManager in context");
			}
		}
	}

	if (!m_virtualCamManager) {
		return ExecutionResult::failure("No VirtualCamManager available");
	}

	// Get input texture
	auto *inputTexture = getInputData<QRhiTexture *>("video_in");
	if (!inputTexture) {
		return ExecutionResult::failure("No input texture");
	}

	// Get profile override if provided
	std::string profileId = m_profileId;
	if (hasPinData("profile")) {
		profileId = getInputData<std::string>("profile", m_profileId);
	}

	// Output to VirtualCam
	// TODO: Implement actual output via VirtualCamManager
	// For now, this is a sink node that just receives the texture

	// In the future, this would:
	// 1. Get framebuffer for the profile
	// 2. Copy texture data to framebuffer
	// 3. Trigger encoding/streaming

	return ExecutionResult::success();
}

void HeadsetOutputNode::cleanup()
{
	m_virtualCamManager = nullptr;
	m_initialized = false;
}

} // namespace SceneGraph
} // namespace NeuralStudio

// Register the node type
REGISTER_NODE_TYPE(NeuralStudio::SceneGraph::HeadsetOutputNode, "HeadsetOutput")
