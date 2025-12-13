#include "ScriptNode.h"
#include <iostream>

namespace NeuralStudio {
namespace SceneGraph {
ScriptNode::ScriptNode(const std::string &id) : BaseNodeBackend(id, "ScriptNode")
{
	// Scripts can have inputs/outputs dynamically defined
	addInput("trigger_in", "Trigger", DataType(DataCategory::Flow, "Trigger"));
	addOutput("result_out", "Result", DataType::Any());
}
ScriptNode::~ScriptNode()
{ /* Cleanup script engine */
}
void ScriptNode::execute(const ExecutionContext &context)
{
	if (m_dirty && !m_scriptPath.empty()) {
		std::cout << "ScriptNode: Loading " << m_scriptLanguage << " script " << m_scriptPath << std::endl;
		// Future: Initialize Lua/Python interpreter and load script
		m_dirty = false;
	}
	// Execute script logic
}
void ScriptNode::setScriptPath(const std::string &path)
{
	if (m_scriptPath != path) {
		m_scriptPath = path;
		m_dirty = true;
	}
}
std::string ScriptNode::getScriptPath() const
{
	return m_scriptPath;
}
void ScriptNode::setScriptLanguage(const std::string &lang)
{
	m_scriptLanguage = lang;
}
std::string ScriptNode::getScriptLanguage() const
{
	return m_scriptLanguage;
}
} // namespace SceneGraph
} // namespace NeuralStudio
