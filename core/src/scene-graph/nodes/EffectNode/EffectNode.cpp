#include "EffectNode.h"
#include <iostream>

namespace NeuralStudio {
namespace SceneGraph {
EffectNode::EffectNode(const std::string &id) : BaseNodeBackend(id, "EffectNode")
{
	addInput("visual_in", "Visual Input", DataType::Texture2D());
	addOutput("visual_out", "Visual Output", DataType::Texture2D());
}
EffectNode::~EffectNode() {}
void EffectNode::execute(const ExecutionContext &context)
{
	if (m_dirty && !m_effectPath.empty()) {
		std::cout << "EffectNode: Loading effect " << m_effectPath << std::endl;
		m_dirty = false;
	}
}
void EffectNode::setEffectPath(const std::string &path)
{
	if (m_effectPath != path) {
		m_effectPath = path;
		m_dirty = true;
	}
}
std::string EffectNode::getEffectPath() const
{
	return m_effectPath;
}
} // namespace SceneGraph
} // namespace NeuralStudio
