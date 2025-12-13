#include "AINodeBase.h"

namespace NeuralStudio {
namespace SceneGraph {

AINodeBase::AINodeBase(const std::string &nodeId, const std::string &typeName) : m_nodeId(nodeId), m_typeName(typeName)
{
	// Default metadata for AI nodes
	m_metadata.category = "AI";
	m_metadata.supportsAsync = true;
	m_metadata.supportsGPU = true;
	m_metadata.computeRequirement = ComputeRequirement::High;
}

std::string AINodeBase::getModelPath() const
{
	return ""; // Override in derived classes or get from config
}

void AINodeBase::setPinData(const std::string &pinId, const std::any &data)
{
	m_pinData[pinId] = data;
}

std::any AINodeBase::getPinData(const std::string &pinId) const
{
	auto it = m_pinData.find(pinId);
	if (it != m_pinData.end()) {
		return it->second;
	}
	return {};
}

bool AINodeBase::hasPinData(const std::string &pinId) const
{
	return m_pinData.find(pinId) != m_pinData.end();
}

ExecutionResult AINodeBase::modelError(const std::string &message)
{
	return ExecutionResult::failure("AI Model Error [" + getBackendName() + "]: " + message);
}

} // namespace SceneGraph
} // namespace NeuralStudio
