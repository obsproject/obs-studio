#include "TextureNode.h"
#include <iostream>

namespace NeuralStudio {
namespace SceneGraph {
TextureNode::TextureNode(const std::string &id) : BaseNodeBackend(id, "TextureNode")
{
	addOutput("texture_out", "Texture Output", DataType::Texture2D());
}
TextureNode::~TextureNode() {}
void TextureNode::execute(const ExecutionContext &context)
{
	if (m_dirty && !m_texturePath.empty()) {
		std::cout << "TextureNode: Loading " << m_texturePath << std::endl;
		m_dirty = false;
	}
}
void TextureNode::setTexturePath(const std::string &path)
{
	if (m_texturePath != path) {
		m_texturePath = path;
		m_dirty = true;
	}
}
std::string TextureNode::getTexturePath() const
{
	return m_texturePath;
}
} // namespace SceneGraph
} // namespace NeuralStudio
