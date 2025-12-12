#include "NodeRegistry.h"

NodeRegistry &NodeRegistry::instance()
{
	static NodeRegistry inst;
	return inst;
}

NodeRegistry::NodeRegistry()
{
	// Register Default Nodes based on User's List
	registerNode({"PTZ Camera", NodeCategory::Cameras, "Camera with Pan/Tilt/Zoom"});
	registerNode({"Webcam", NodeCategory::Cameras, "Standard V4L2 Device"});

	registerNode({"Media File", NodeCategory::Video, "Video File Source"});
	registerNode({"Playlist", NodeCategory::Video, "Sequential Video Playback"});

	registerNode({"Image Slide", NodeCategory::Photo, "Single Image or Slideshow"});

	registerNode({"Audio Input", NodeCategory::Audio, "Mic or Aux Input"});
	registerNode({"VST Plugin", NodeCategory::Audio, "Audio Effect Plugin"});

	registerNode({"Lua Script", NodeCategory::Script, "Custom Logic Script"});

	registerNode({"3D Mesh", NodeCategory::ThreeD, "OBJ/GLTF Model"});
	registerNode({"Animation", NodeCategory::ThreeD, "3D Animation Controller"});

	registerNode({"Stinger", NodeCategory::Transitions, "Video Transition"});
	registerNode({"Crossfade", NodeCategory::Transitions, "Standard Mix"});

	registerNode({"Color Correction", NodeCategory::Effects, "LUTs and Adjustments"});
	registerNode({"Chroma Key", NodeCategory::Filters, "Green Screen Removal"});

	registerNode({"SuperRes", NodeCategory::ML, "AI Upscaling"});
	registerNode({"Face Track", NodeCategory::ML, "Face Detection Coordinates"});

	registerNode({"Stable Diffusion", NodeCategory::AI, "Generative Image Stream"});

	registerNode({"RTMP Stream", NodeCategory::Broadcasting, "Streaming Output"});
	registerNode({"Virtual Cam", NodeCategory::Broadcasting, "Loopback Output"});

	registerNode({"SRT Input", NodeCategory::IncomingStreams, "Low Latency Stream In"});
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

QString NodeRegistry::categoryName(NodeCategory cat) const
{
	switch (cat) {
	case NodeCategory::Cameras:
		return "Cameras + Automation";
	case NodeCategory::Video:
		return "Video";
	case NodeCategory::Photo:
		return "Photo";
	case NodeCategory::Audio:
		return "Audio";
	case NodeCategory::Script:
		return "Script";
	case NodeCategory::ThreeD:
		return "3D Assets";
	case NodeCategory::Transitions:
		return "Transitions";
	case NodeCategory::Effects:
		return "Effects";
	case NodeCategory::Filters:
		return "Filters";
	case NodeCategory::ML:
		return "ML";
	case NodeCategory::AI:
		return "AI";
	case NodeCategory::Broadcasting:
		return "Broadcasting";
	case NodeCategory::IncomingStreams:
		return "Incoming Streams";
	default:
		return "Unknown";
	}
}
