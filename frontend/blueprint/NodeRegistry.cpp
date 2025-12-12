#include "NodeRegistry.h"
#include "NodeItem.h"
#include "nodes/CameraNode.h"
#include "nodes/VideoNode.h"
#include "nodes/EffectNode.h"

NodeRegistry &NodeRegistry::instance()
{
	static NodeRegistry inst;
	return inst;
}

NodeRegistry::NodeRegistry()
{
	// Helper to reduce verbosity
	auto reg = [this](const QString &name, NodeCategory cat, const QString &desc,
			  std::function<NodeItem *()> func = nullptr) {
		if (!func)
			func = [name]() {
				return new NodeItem(name);
			};
		registerNode({name, cat, desc, func});
	};

	// Register Default Nodes based on User's List
	reg("PTZ Camera", NodeCategory::Cameras, "Camera with Pan/Tilt/Zoom",
	    []() { return new CameraNode("PTZ Camera"); });
	reg("Webcam", NodeCategory::Cameras, "Standard V4L2 Device", []() { return new CameraNode("Webcam"); });

	reg("Media File", NodeCategory::Video, "Video File Source", []() { return new VideoNode("Media File"); });
	reg("Playlist", NodeCategory::Video, "Sequential Video Playback", []() { return new VideoNode("Playlist"); });

	reg("Image Slide", NodeCategory::Photo, "Single Image or Slideshow");

	reg("Audio Input", NodeCategory::Audio, "Mic or Aux Input");
	reg("VST Plugin", NodeCategory::Audio, "Audio Effect Plugin");

	reg("Lua Script", NodeCategory::Script, "Custom Logic Script");

	reg("3D Mesh", NodeCategory::ThreeD, "OBJ/GLTF Model");
	reg("Animation", NodeCategory::ThreeD, "3D Animation Controller");

	reg("Stinger", NodeCategory::Transitions, "Video Transition", []() { return new EffectNode("Stinger"); });
	reg("Crossfade", NodeCategory::Transitions, "Standard Mix", []() { return new EffectNode("Crossfade"); });

	reg("Color Correction", NodeCategory::Effects, "LUTs and Adjustments",
	    []() { return new EffectNode("Color Correction"); });
	reg("Chroma Key", NodeCategory::Filters, "Green Screen Removal", []() { return new EffectNode("Chroma Key"); });

	reg("SuperRes", NodeCategory::ML, "AI Upscaling", []() { return new EffectNode("SuperRes"); });
	reg("Face Track", NodeCategory::ML, "Face Detection Coordinates",
	    []() { return new EffectNode("Face Track"); });

	reg("Stable Diffusion", NodeCategory::AI, "Generative Image Stream",
	    []() { return new CameraNode("Stable Diffusion"); }); // Generates video

	reg("RTMP Stream", NodeCategory::Broadcasting, "Streaming Output");
	reg("Virtual Cam", NodeCategory::Broadcasting, "Loopback Output");

	reg("SRT Input", NodeCategory::IncomingStreams, "Low Latency Stream In",
	    []() { return new VideoNode("SRT Input"); });
}

void NodeRegistry::registerNode(const NodeDefinition &def)
{
	m_definitions.push_back(def);
}

std::vector<NodeDefinition> NodeRegistry::getNodesByCategory(NodeCategory cat) const
{
	std::vector<NodeDefinition> result;
	for (const auto &def : m_definitions) {
		if (def.category == cat) {
			result.push_back(def);
		}
	}
	return result;
}

std::vector<NodeDefinition> NodeRegistry::getAllNodes() const
{
	return m_definitions;
}

const NodeDefinition *NodeRegistry::getNodeByName(const QString &name) const
{
	for (const auto &def : m_definitions) {
		if (def.name == name)
			return &def;
	}
	return nullptr;
}

QString NodeRegistry::categoryName(NodeCategory cat) const
{
	switch (cat) {
	case NodeCategory::Cameras:
		return "Manager Cameras";
	case NodeCategory::Video:
		return "Manager Video";
	case NodeCategory::Photo:
		return "Manager Photo";
	case NodeCategory::Audio:
		return "Manager Audio";
	case NodeCategory::Script:
		return "Manager Script";
	case NodeCategory::ThreeD:
		return "Manager 3D Assets";
	case NodeCategory::Transitions:
		return "Manager Transitions";
	case NodeCategory::Effects:
		return "Manager Effects";
	case NodeCategory::Filters:
		return "Manager Filters";
	case NodeCategory::ML:
		return "Manager ML";
	case NodeCategory::AI:
		return "Manager AI";
	case NodeCategory::Broadcasting:
		return "Manager Broadcasting";
	case NodeCategory::IncomingStreams:
		return "Manager Incoming Streams";
	default:
		return "Unknown";
	}
}
