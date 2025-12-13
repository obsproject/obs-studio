```cpp
#include "RTXUpscaleNode.h"
#include "../NodeFactory.h"

	namespace NeuralStudio
{
	namespace SceneGraph {

	RTXUpscaleNode::RTXUpscaleNode(const std::string &nodeId) : BaseNodeBackend(nodeId, "RTXUpscale")
	{
		// Setup metadata
		NodeMetadata meta;
		meta.displayName = "RTX AI Upscale";
		meta.category = "Effect";
		meta.description = "NVIDIA AI-powered upscaling using Tensor Cores (4K→8K)";
		meta.tags = {"upscaling", "ai", "rtx", "nvidia", "super-resolution"};
		meta.nodeColorRGB = 0xFF76B900; // NVIDIA green
		meta.icon = ":/icons/nodes/rtx_upscale.svg";
		meta.supportsGPU = true;
		meta.supportsCPU = false;
		meta.computeRequirement = ComputeRequirement::VeryHigh;
		meta.estimatedMemoryMB = 500;
		setMetadata(meta);

		// Define pins
		addInput("input", "Video Input", DataType::Texture2D());
		addInput("quality", "Quality Mode", DataType::Scalar()); // 0=Performance, 1=HighQuality
		addInput("scale", "Scale Factor", DataType::Scalar());   // 1.33x, 1.5x, 2x, 3x, 4x
		addOutput("output", "Upscaled Output", DataType::Texture2D());

		// Default values
		m_qualityMode = Rendering::RTXUpscaler::QualityMode::HighQuality;
		m_scaleFactor = Rendering::RTXUpscaler::ScaleFactor::Scale2x;
	}

	bool RTXUpscaleNode::initialize(const NodeConfig &config)
	{
		// Check if RTX upscaling is available
		if (!Rendering::RTXUpscaler::isAvailable()) {
			return false; // No NVIDIA GPU or SDK not available
		}

		// Get configuration
		int quality = config.get<int>("quality_mode", 1); // 0=Performance, 1=HighQuality
		int scale = config.get<int>("scale_factor", 2);   // 2 = 2x scale

		m_qualityMode = (quality == 0) ? Rendering::RTXUpscaler::QualityMode::Performance
					       : Rendering::RTXUpscaler::QualityMode::HighQuality;

		// Map scale integer to enum
		switch (scale) {
		case 1:
			m_scaleFactor = Rendering::RTXUpscaler::ScaleFactor::Scale1_33x;
			break;
		case 2:
			m_scaleFactor = Rendering::RTXUpscaler::ScaleFactor::Scale2x;
			break;
		case 3:
			m_scaleFactor = Rendering::RTXUpscaler::ScaleFactor::Scale3x;
			break;
		case 4:
			m_scaleFactor = Rendering::RTXUpscaler::ScaleFactor::Scale4x;
			break;
		default:
			m_scaleFactor = Rendering::RTXUpscaler::ScaleFactor::Scale2x;
		}

		// RTXUpscaler will be created when we have renderer context
		m_initialized = true;
		return true;
	}

	ExecutionResult RTXUpscaleNode::process(ExecutionContext &ctx)
	{
		if (!m_initialized) {
			return ExecutionResult::failure("RTXUpscaleNode not initialized");
		}

		// Get input texture
		auto *inputTexture = getInputData<QRhiTexture *>("input");
		if (!inputTexture) {
			return ExecutionResult::failure("No input texture");
		}

		// Create upscaler if needed (lazy initialization)
		if (!m_upscaler && ctx.renderer) {
			m_upscaler = std::make_unique<Rendering::RTXUpscaler>(
				static_cast<Rendering::VulkanRenderer *>(ctx.renderer));

			// Initialize with texture dimensions
			QSize inputSize = inputTexture->pixelSize();
			uint32_t outputWidth = inputSize.width() * 2; // TODO: Calculate from scale factor
			uint32_t outputHeight = inputSize.height() * 2;

			if (!m_upscaler->initialize(inputSize.width(), inputSize.height(), outputWidth, outputHeight,
						    m_qualityMode, m_scaleFactor)) {
				return ExecutionResult::failure("Failed to initialize RTX upscaler");
			}
		}

		if (!m_upscaler) {
			return ExecutionResult::failure("No renderer available for RTX upscaling");
		}

		// Create output texture (TODO: get from texture pool)
		// For now, assume output texture is created externally
		QRhiTexture *outputTexture = nullptr; // TODO: Create output texture

		// Perform upscaling
		if (!m_upscaler->upscale(inputTexture, outputTexture)) {
			return ExecutionResult::failure("RTX upscaling failed");
		}

		// Set output
		setOutputData("output", outputTexture);

		return ExecutionResult::success();
	}

	void RTXUpscaleNode::cleanup()
	{
		m_upscaler.reset();
		m_initialized = false;
	}

	} // namespace SceneGraph
} // namespace NeuralStudio

// Register the node type
REGISTER_NODE_TYPE(NeuralStudio::SceneGraph::RTXUpscaleNode, "RTXUpscale")
