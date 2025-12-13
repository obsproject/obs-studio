#include "CameraNode.h"
#include "../NodeFactory.h"

namespace NeuralStudio {
namespace SceneGraph {

// Register with Factory
namespace {
std::shared_ptr<IExecutableNode> createCameraNode(const std::string &id)
{
	return std::make_shared<CameraNode>(id);
}
struct CameraNodeRegistrar {
	CameraNodeRegistrar() { NodeFactory::registerNodeType("CameraNode", createCameraNode); }
};
static CameraNodeRegistrar registrar;
} // namespace

CameraNode::CameraNode(const std::string &id) : BaseNodeBackend(id, "CameraNode") {}

void CameraNode::initialize()
{
	// Define Output Ports
	// Visual Output (Right)
	PinDescriptor visualOut;
	visualOut.id = "visual_out";
	visualOut.name = "Visual Out";
	visualOut.type = DataType::Texture;
	visualOut.direction = PinDirection::Output;
	addPin(visualOut);

	// Audio Output (Bottom)
	PinDescriptor audioOut;
	audioOut.id = "audio_out";
	audioOut.name = "Audio Out";
	audioOut.type = DataType::Audio;
	audioOut.direction = PinDirection::Output;
	addPin(audioOut);
}

void CameraNode::execute(ExecutionContext &ctx)
{
	// Logic to capture frame from m_deviceId and setPinData("visual_out", texture)
	// For now, no-op or placeholder
}

void CameraNode::setDeviceId(const std::string &deviceId)
{
	m_deviceId = deviceId;
	// Trigger re-initialization of camera source
}

std::string CameraNode::getDeviceId() const
{
	return m_deviceId;
}

} // namespace SceneGraph
} // namespace NeuralStudio
