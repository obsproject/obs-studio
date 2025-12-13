#include "../NodeFactory.h"
#include "AudioNode.h"
#include <memory>

namespace NeuralStudio {
namespace SceneGraph {

// Register AudioNode with Factory
namespace {
std::shared_ptr<IExecutableNode> createAudioNode(const std::string &id)
{
	return std::make_shared<AudioNode>(id);
}

struct AudioNodeRegistrar {
	AudioNodeRegistrar() { NodeFactory::registerNodeType("AudioNode", createAudioNode); }
};
static AudioNodeRegistrar registrar;
} // namespace

} // namespace SceneGraph
} // namespace NeuralStudio
