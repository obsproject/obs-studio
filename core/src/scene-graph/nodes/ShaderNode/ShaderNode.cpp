#include "ShaderNode.h"
#include <iostream>

namespace NeuralStudio {
namespace SceneGraph {
ShaderNode::ShaderNode(const std::string &id) : BaseNodeBackend(id, "ShaderNode")
{
	addOutput("shader_out", "Shader Output", DataType(DataCategory::Composite, "Shader"));
}
ShaderNode::~ShaderNode() {}
void ShaderNode::execute(const ExecutionContext &context)
{
	if (m_dirty && !m_shaderPath.empty()) {
		std::cout << "ShaderNode: Loading " << m_shaderPath << std::endl;
		m_dirty = false;
	}
}
void ShaderNode::setShaderPath(const std::string &path)
{
	if (m_shaderPath != path) {
		m_shaderPath = path;
		m_dirty = true;
	}
}
std::string ShaderNode::getShaderPath() const
{
	return m_shaderPath;
}
} // namespace SceneGraph
} // namespace NeuralStudio
