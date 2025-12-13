#include "../NodeFactory.h"
#include "ScriptNode.h"
#include <memory>
namespace NeuralStudio {
namespace SceneGraph {
namespace {
std::shared_ptr<IExecutableNode> createScriptNode(const std::string &id)
{
	return std::make_shared<ScriptNode>(id);
}
struct ScriptNodeRegistrar {
	ScriptNodeRegistrar() { NodeFactory::registerNodeType("ScriptNode", createScriptNode); }
};
static ScriptNodeRegistrar registrar;
} // namespace
} // namespace SceneGraph
} // namespace NeuralStudio
