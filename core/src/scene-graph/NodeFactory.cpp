#include "NodeFactory.h"

namespace NeuralStudio {
namespace SceneGraph {

// Use a static function to access the registry to avoid static initialization order fiasco
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
	auto &registry = getRegistry();
	for (const auto &pair : registry) {
		types.push_back(pair.first);
	}
	return types;
}

} // namespace SceneGraph
} // namespace NeuralStudio
