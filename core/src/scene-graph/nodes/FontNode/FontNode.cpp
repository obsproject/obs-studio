#include "FontNode.h"
#include <iostream>

namespace NeuralStudio {
namespace SceneGraph {

FontNode::FontNode(const std::string &id) : BaseNodeBackend(id, "FontNode")
{
	addOutput("font_out", "Font Output", DataType(DataCategory::Primitive, "Font"));
}

FontNode::~FontNode() {}

void FontNode::execute(const ExecutionContext &context)
{
	if (m_dirty && !m_fontPath.empty()) {
		std::cout << "FontNode: Loading " << m_fontPath << std::endl;
		m_dirty = false;
	}
}

void FontNode::setFontPath(const std::string &path)
{
	if (m_fontPath != path) {
		m_fontPath = path;
		m_dirty = true;
	}
}

std::string FontNode::getFontPath() const
{
	return m_fontPath;
}

void FontNode::setFontSize(int size)
{
	m_fontSize = size;
}
int FontNode::getFontSize() const
{
	return m_fontSize;
}

} // namespace SceneGraph
} // namespace NeuralStudio
