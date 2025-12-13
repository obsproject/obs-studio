#include "LLMNode.h"
#include <iostream>

namespace NeuralStudio {
namespace SceneGraph {
LLMNode::LLMNode(const std::string &id) : BaseNodeBackend(id, "LLMNode")
{
	addInput("prompt", "Prompt", DataType::String());
	addOutput("response", "Response", DataType::String());
}

LLMNode::~LLMNode() {}

void LLMNode::execute(const ExecutionContext &context)
{
	if (m_dirty && !m_prompt.empty()) {
		std::cout << "LLMNode: Sending prompt to " << m_model << std::endl;
		// TODO: Call appropriate LLM API (Gemini, GPT, Claude, etc.)
		m_dirty = false;
	}
}

void LLMNode::setPrompt(const std::string &prompt)
{
	if (m_prompt != prompt) {
		m_prompt = prompt;
		m_dirty = true;
	}
}

void LLMNode::setModel(const std::string &model)
{
	if (m_model != model) {
		m_model = model;
		m_dirty = true;
	}
}

std::string LLMNode::getPrompt() const
{
	return m_prompt;
}

std::string LLMNode::getModel() const
{
	return m_model;
}
} // namespace SceneGraph
} // namespace NeuralStudio
