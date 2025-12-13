#include "ThreeDModelNodeController.h"
#include "../../core/NodeGraphController.h"

namespace NeuralStudio {
namespace Blueprint {

ThreeDModelNodeController::ThreeDModelNodeController(QObject *parent) : BaseNodeController(parent) {}

ThreeDModelNodeController::~ThreeDModelNodeController() {}

QString ThreeDModelNodeController::getModelPath() const
{
	return m_modelPath;
}

void ThreeDModelNodeController::setModelPath(const QString &path)
{
	if (m_modelPath != path) {
		m_modelPath = path;
		emit modelPathChanged();

		// Sync with backend if node ID is set
		if (m_nodeGraphController && !m_nodeId.isEmpty()) {
			m_nodeGraphController->setNodeProperty(m_nodeId, "modelPath", path);
		}
	}
}

QVector3D ThreeDModelNodeController::getScale() const
{
	return m_scale;
}
void ThreeDModelNodeController::setScale(const QVector3D &scale)
{
	if (m_scale != scale) {
		m_scale = scale;
		emit scaleChanged();
		if (m_nodeGraphController && !m_nodeId.isEmpty()) {
			// Serialize as string or pass components
			// m_nodeGraphController->setNodeProperty(m_nodeId, "scale", ...);
		}
	}
}

QVector3D ThreeDModelNodeController::getPosition() const
{
	return m_position;
}
void ThreeDModelNodeController::setPosition(const QVector3D &pos)
{
	if (m_position != pos) {
		m_position = pos;
		emit positionChanged();
	}
}

QVector3D ThreeDModelNodeController::getRotation() const
{
	return m_rotation;
}
void ThreeDModelNodeController::setRotation(const QVector3D &rot)
{
	if (m_rotation != rot) {
		m_rotation = rot;
		emit rotationChanged();
	}
}

} // namespace Blueprint
} // namespace NeuralStudio
