#include "../NodeFactory.h"
#include "TextureNode.h"
#include <memory>
namespace NeuralStudio {
namespace SceneGraph {
namespace {
std::shared_ptr<IExecutableNode> createTextureNode(const std::string &id)
{
	return std::make_shared<TextureNode>(id);
}
struct TextureNodeRegistrar {
	TextureNodeRegistrar() { NodeFactory::registerNodeType("TextureNode", createTextureNode); }
};
static TextureNodeRegistrar registrar;
} // namespace
} // namespace SceneGraph
} // namespace NeuralStudio
