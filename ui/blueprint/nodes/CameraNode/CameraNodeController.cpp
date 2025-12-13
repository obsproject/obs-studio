#include "CameraNodeController.h"

namespace NeuralStudio {
namespace UI {

CameraNodeController::CameraNodeController(QObject *parent) : BaseNodeController(parent) {}

QString CameraNodeController::deviceId() const
{
	return m_deviceId;
}

void CameraNodeController::setDeviceId(const QString &id)
{
	if (m_deviceId != id) {
		m_deviceId = id;
		emit deviceIdChanged();
		updateProperty("deviceId", id);
	}
}

void CameraNodeController::onBackendNodeSet()
{
	// Sync initial state if needed
	// For now, assume property binding handles it or default is empty
}

} // namespace UI
} // namespace NeuralStudio
