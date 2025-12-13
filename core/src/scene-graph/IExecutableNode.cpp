#include "IExecutableNode.h"
#include <algorithm>

namespace NeuralStudio {
namespace SceneGraph {

//=============================================================================
// DataType Compatibility
//=============================================================================

bool DataType::isCompatibleWith(const DataType &other) const
{
	// Exact match
	if (*this == other) {
		return true;
	}

	// "Any" type is compatible with everything
	if (typeName == "Any" || other.typeName == "Any") {
		return true;
	}

	// Same category types are often compatible
	if (category == other.category) {
		// Textures are generally compatible
		if (category == DataCategory::Media && typeName.find("Texture") != std::string::npos &&
		    other.typeName.find("Texture") != std::string::npos) {
			return true;
		}
	}

	return false;
}

bool DataType::canConvertTo(const DataType &other) const
{
	if (isCompatibleWith(other)) {
		return true;
	}

	// Primitive conversions
	if (category == DataCategory::Primitive && other.category == DataCategory::Primitive) {
		return true; // Most primitives can convert to each other
	}

	return false;
}

//=============================================================================
// Node Factory Implementation
//=============================================================================

std::map<std::string, NodeFactory::NodeCreator> &NodeFactory::getRegistry()
{
	static std::map<std::string, NodeCreator> registry;
	return registry;
}

void NodeFactory::registerNodeType(const std::string &typeName, NodeCreator creator)
{
	getRegistry()[typeName] = creator;
}

std::shared_ptr<IExecutableNode> NodeFactory::create(const std::string &typeName, const std::string &nodeId)
{
	auto &registry = getRegistry();
	auto it = registry.find(typeName);
	if (it != registry.end()) {
		return it->second(nodeId);
	}
	return nullptr;
}

std::vector<std::string> NodeFactory::getRegisteredTypes()
{
	std::vector<std::string> types;
	for (const auto &[name, _] : getRegistry()) {
		types.push_back(name);
	}
	std::sort(types.begin(), types.end());
	return types;
}

bool NodeFactory::isTypeRegistered(const std::string &typeName)
{
	return getRegistry().find(typeName) != getRegistry().end();
}

} // namespace SceneGraph
} // namespace NeuralStudio
