#include "../NodeFactory.h"
#include "ImageNode.h"
#include <memory>

namespace NeuralStudio {
namespace SceneGraph {

// Register ImageNode with Factory
namespace {
std::shared_ptr<IExecutableNode> createImageNode(const std::string &id)
{
	return std::make_shared<ImageNode>(id);
}

struct ImageNodeRegistrar {
	ImageNodeRegistrar() { NodeFactory::registerNodeType("ImageNode", createImageNode); }
};
static ImageNodeRegistrar registrar;
} // namespace

} // namespace SceneGraph
} // namespace NeuralStudio
