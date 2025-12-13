#include "StitchNode.h"
#include "../NodeFactory.h"

namespace NeuralStudio {
namespace SceneGraph {

StitchNode::StitchNode(const std::string &nodeId) : BaseNodeBackend(nodeId, "Stitch")
{
	// Setup metadata
	NodeMetadata meta;
	meta.displayName = "STMap Stitch";
	meta.category = "Video";
	meta.description = "Fisheye to equirectangular stitching using STMap UV remapping";
	meta.tags = {"stitching", "fisheye", "360", "stmap"};
	meta.nodeColorRGB = 0xFF4488FF; // Blue
	meta.icon = ":/icons/nodes/stitch.svg";
	meta.supportsGPU = true;
	meta.supportsCPU = false;
	meta.computeRequirement = ComputeRequirement::Medium;
	meta.estimatedMemoryMB = 100;
	setMetadata(meta);

	// Define pins
	addInput("video_in", "Video Input", DataType::Texture2D());
	addInput("stmap", "STMap Texture", DataType::Texture2D());
	addOutput("video_out", "Stitched Output", DataType::Texture2D());
}

bool StitchNode::initialize(const NodeConfig &config)
{
	m_stmapPath = config.get<std::string>("stmap_path", "");

	if (m_stmapPath.empty()) {
		return false;
	}

	// Create STMapLoader
	m_stmapLoader = std::make_unique<Rendering::STMapLoader>();

	// Load STMap texture
	// Note: Renderer will be provided via ExecutionContext
	// Actual loading happens in process() when renderer is available

	m_initialized = true;
	return true;
}

ExecutionResult StitchNode::process(ExecutionContext &ctx)
{
	if (!m_initialized) {
		return ExecutionResult::failure("StitchNode not initialized");
	}

	// Get renderer from context
	if (!m_renderer && ctx.renderer) {
		m_renderer = static_cast<Rendering::StereoRenderer *>(ctx.renderer);
	}

	if (!m_renderer) {
		return ExecutionResult::failure("No renderer available");
	}

	// Get input texture
	auto *inputTexture = getInputData<QRhiTexture *>("video_in");
	if (!inputTexture) {
		return ExecutionResult::failure("No input texture");
	}

	// Get or use default STMap
	auto *stmapTexture = getInputData<QRhiTexture *>("stmap");
	if (!stmapTexture) {
		// Load from path if not provided via pin
		// TODO: Load STMap texture from m_stmapPath
		return ExecutionResult::failure("No STMap texture provided");
	}

	// Perform stitching
	// TODO: Call stitching compute shader via renderer
	// For now, pass through input
	QRhiTexture *outputTexture = inputTexture;

	// Set output
	setOutputData("video_out", outputTexture);

	return ExecutionResult::success();
}

void StitchNode::cleanup()
{
	m_stmapLoader.reset();
	m_renderer = nullptr;
	m_initialized = false;
}

} // namespace SceneGraph
} // namespace NeuralStudio

// Register the node type
REGISTER_NODE_TYPE(NeuralStudio::SceneGraph::StitchNode, "Stitch")
