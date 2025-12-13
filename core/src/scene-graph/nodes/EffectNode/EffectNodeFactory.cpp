#include "../NodeFactory.h"
#include "EffectNode.h"
#include <memory>
namespace NeuralStudio {
namespace SceneGraph {
namespace {
std::shared_ptr<IExecutableNode> createEffectNode(const std::string &id)
{
	return std::make_shared<EffectNode>(id);
}
struct EffectNodeRegistrar {
	EffectNodeRegistrar() { NodeFactory::registerNodeType("EffectNode", createEffectNode); }
};
static EffectNodeRegistrar registrar;
} // namespace
} // namespace SceneGraph
} // namespace NeuralStudio
