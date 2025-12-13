#include "../NodeFactory.h"
#include "FontNode.h"
#include <memory>

namespace NeuralStudio {
namespace SceneGraph {

namespace {
std::shared_ptr<IExecutableNode> createFontNode(const std::string &id)
{
	return std::make_shared<FontNode>(id);
}

struct FontNodeRegistrar {
	FontNodeRegistrar() { NodeFactory::registerNodeType("FontNode", createFontNode); }
};
static FontNodeRegistrar registrar;
} // namespace

} // namespace SceneGraph
} // namespace NeuralStudio
