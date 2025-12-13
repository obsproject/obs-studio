#include "ThreeDModelNode.h"
#include <iostream>

namespace NeuralStudio {
namespace SceneGraph {

ThreeDModelNode::ThreeDModelNode(const std::string &id) : BaseNodeBackend(id, "ThreeDModelNode")
{
	// Define Outputs - 3D model is a visual source
	addOutput("visual_out", "Visual Output", DataType(DataCategory::Media, "Mesh"));
}

ThreeDModelNode::~ThreeDModelNode()
{
	// Cleanup from SceneManager handled in dedicated cleanup method with context
}

void ThreeDModelNode::execute(const ExecutionContext &context)
{
	if (m_dirty && !m_modelPath.empty()) {
		if (context.sceneManager) {
			// Logic to load model into SceneManager
			// e.g. m_sceneObjectId = context.sceneManager->LoadModel(m_modelPath);
			std::cout << "ThreeDModelNode: Loading " << m_modelPath << std::endl;

			// Reset dirty flag
			m_dirty = false;
		}
	}

	// Pass scene object ID to output
	if (m_sceneObjectId > 0) {
		setOutputData("Visual Out", m_sceneObjectId);
	}
}

void ThreeDModelNode::setModelPath(const std::string &path)
{
	if (m_modelPath != path) {
		m_modelPath = path;
		m_dirty = true;
	}
}

std::string ThreeDModelNode::getModelPath() const
{
	return m_modelPath;
}

} // namespace SceneGraph
} // namespace NeuralStudio
