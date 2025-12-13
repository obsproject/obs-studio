#include "MLNode.h"
#include <iostream>

namespace NeuralStudio {
namespace SceneGraph {
MLNode::MLNode(const std::string &id) : BaseNodeBackend(id, "MLNode")
{
	addInput("input_tensor", "Input", DataType(DataCategory::Composite, "Tensor"));
	addOutput("output_tensor", "Output", DataType(DataCategory::Composite, "Tensor"));
}

MLNode::~MLNode()
{
	// TODO: Cleanup model session (ONNX/TensorFlow/PyTorch)
}

void MLNode::execute(const ExecutionContext &context)
{
	if (m_dirty && !m_modelPath.empty()) {
		std::cout << "MLNode: Loading AI model " << m_modelPath << std::endl;
		// TODO: Load model using appropriate backend (ONNX Runtime, TensorFlow, PyTorch)
		m_dirty = false;
	}
	// TODO: Run inference
}

void MLNode::setModelPath(const std::string &path)
{
	if (m_modelPath != path) {
		m_modelPath = path;
		m_dirty = true;
	}
}

std::string MLNode::getModelPath() const
{
	return m_modelPath;
}
} // namespace SceneGraph
} // namespace NeuralStudio
