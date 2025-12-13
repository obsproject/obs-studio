#include "../NodeFactory.h"
#include "LLMNode.h"
#include <memory>
namespace NeuralStudio {
namespace SceneGraph {
namespace {
std::shared_ptr<IExecutableNode> createLLMNode(const std::string &id)
{
	return std::make_shared<LLMNode>(id);
}
struct LLMNodeRegistrar {
	LLMNodeRegistrar() { NodeFactory::registerNodeType("LLMNode", createLLMNode); }
};
static LLMNodeRegistrar registrar;
} // namespace
} // namespace SceneGraph
} // namespace NeuralStudio
