#include "../NodeFactory.h"
#include "ShaderNode.h"
#include <memory>
namespace NeuralStudio {
namespace SceneGraph {
namespace {
std::shared_ptr<IExecutableNode> createShaderNode(const std::string &id)
{
	return std::make_shared<ShaderNode>(id);
}
struct ShaderNodeRegistrar {
	ShaderNodeRegistrar() { NodeFactory::registerNodeType("ShaderNode", createShaderNode); }
};
static ShaderNodeRegistrar registrar;
} // namespace
} // namespace SceneGraph
} // namespace NeuralStudio
