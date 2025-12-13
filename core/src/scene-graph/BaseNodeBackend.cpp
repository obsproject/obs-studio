#include "BaseNodeBackend.h"

namespace NeuralStudio {
namespace SceneGraph {

BaseNodeBackend::BaseNodeBackend(const std::string &nodeId, const std::string &nodeType)
	: m_nodeId(nodeId),
	  m_nodeType(nodeType)
{
	// Set default metadata
	m_metadata.displayName = nodeType;
	m_metadata.category = "Processing";
	m_metadata.description = "";
}

void BaseNodeBackend::setPinData(const std::string &pinId, const std::any &data)
{
	m_pinData[pinId] = data;
}

std::any BaseNodeBackend::getPinData(const std::string &pinId) const
{
	auto it = m_pinData.find(pinId);
	if (it != m_pinData.end()) {
		return it->second;
	}
	return std::any();
}

bool BaseNodeBackend::hasPinData(const std::string &pinId) const
{
	return m_pinData.find(pinId) != m_pinData.end();
}

void BaseNodeBackend::addInput(const std::string &pinId, const std::string &name, const DataType &type)
{
	PinDescriptor pin(pinId, name, type, true);
	m_inputs.push_back(pin);
}

void BaseNodeBackend::addOutput(const std::string &pinId, const std::string &name, const DataType &type)
{
	PinDescriptor pin(pinId, name, type, false);
	m_outputs.push_back(pin);
}

} // namespace SceneGraph
} // namespace NeuralStudio
