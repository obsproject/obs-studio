#include "OnnxNodeController.h"

namespace NeuralStudio {
namespace UI {

OnnxNodeController::OnnxNodeController(QObject *parent) : BaseNodeController(parent) {}

void OnnxNodeController::setModelPath(const QString &path)
{
	if (m_modelPath != path) {
		m_modelPath = path;
		emit modelPathChanged();
		emit propertyChanged("modelPath", path);
	}
}

void OnnxNodeController::setUseGpu(bool use)
{
	if (m_useGpu != use) {
		m_useGpu = use;
		emit useGpuChanged();
		emit propertyChanged("useGpu", use);
	}
}

} // namespace UI
} // namespace NeuralStudio
