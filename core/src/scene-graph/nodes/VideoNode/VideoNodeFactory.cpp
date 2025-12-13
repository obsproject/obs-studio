#include "../NodeFactory.h"
#include "VideoNode.h"
#include <memory>

namespace NeuralStudio {
namespace SceneGraph {

// Register VideoNode with Factory
namespace {
std::shared_ptr<IExecutableNode> createVideoNode(const std::string &id)
{
	return std::make_shared<VideoNode>(id);
}

struct VideoNodeRegistrar {
	VideoNodeRegistrar() { NodeFactory::registerNodeType("VideoNode", createVideoNode); }
};
static VideoNodeRegistrar registrar;
} // namespace

} // namespace SceneGraph
} // namespace NeuralStudio
