#include "../NodeFactory.h"
#include "MLNode.h"
#include <memory>
namespace NeuralStudio {
namespace SceneGraph {
namespace {
std::shared_ptr<IExecutableNode> createMLNode(const std::string &id)
{
	return std::make_shared<MLNode>(id);
}
struct MLNodeRegistrar {
	MLNodeRegistrar() { NodeFactory::registerNodeType("MLNode", createMLNode); }
};
static MLNodeRegistrar registrar;
} // namespace
} // namespace SceneGraph
} // namespace NeuralStudio
