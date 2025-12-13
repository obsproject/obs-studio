#include "../NodeFactory.h"
#include "ThreeDModelNode.h"
#include <memory>

namespace NeuralStudio {
namespace SceneGraph {

// Register ThreeDModelNode with Factory
namespace {
std::shared_ptr<IExecutableNode> createThreeDModelNode(const std::string &id)
{
	return std::make_shared<ThreeDModelNode>(id);
}

struct ThreeDModelNodeRegistrar {
	ThreeDModelNodeRegistrar() { NodeFactory::registerNodeType("ThreeDModelNode", createThreeDModelNode); }
};
static ThreeDModelNodeRegistrar registrar;
} // namespace

} // namespace SceneGraph
} // namespace NeuralStudio
