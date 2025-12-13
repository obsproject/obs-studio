#include "CameraManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDir>
#include <QFile>
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

CameraManagerController::CameraManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	scanCameras();
}

CameraManagerController::~CameraManagerController() {}

void CameraManagerController::scanV4L2Devices()
{
	m_availableCameras.clear();

	// Scan /dev/video* devices
	QDir devdir("/dev");
	QStringList videos = devdir.entryList(QStringList() << "video*", QDir::System);

	for (const QString &device : videos) {
		QString devicePath = "/dev/" + device;

		// Try to read device name from V4L2
		QString deviceName = device; // Default to device name

		// Check if accessible
		if (QFile::exists(devicePath)) {
			QVariantMap camera;
			camera["name"] = deviceName;
			camera["deviceId"] = devicePath;
			camera["device"] = device;

			// Check if this camera is already in use
			bool inUse = false;
			for (const QString &nodeId : managedNodes()) {
				if (getResourceForNode(nodeId) == devicePath) {
					inUse = true;
					camera["nodeId"] = nodeId;
					camera["managed"] = true;
					break;
				}
			}
			camera["inUse"] = inUse;

			m_availableCameras.append(camera);
		}
	}

	qDebug() << "Found" << m_availableCameras.size() << "V4L2 cameras," << managedNodes().size() << "managed nodes";
}

void CameraManagerController::scanCameras()
{
	scanV4L2Devices();
	emit availableCamerasChanged();
	emit camerasChanged();
}

void CameraManagerController::createCameraNode(const QString &deviceId, const QString &deviceName)
{
	if (!m_graphController) {
		qWarning() << "No graph controller set";
		return;
	}

	QString nodeId = m_graphController->createNode("CameraNode", 0.0f, 0.0f);
	if (nodeId.isEmpty()) {
		qWarning() << "Failed to create CameraNode";
		return;
	}

	m_graphController->setNodeProperty(nodeId, "deviceId", deviceId);
	m_graphController->setNodeProperty(nodeId, "deviceName", deviceName);

	// Track with metadata
	QVariantMap metadata;
	metadata["deviceId"] = deviceId;
	metadata["deviceName"] = deviceName;
	trackNode(nodeId, deviceId, metadata);

	scanCameras();
	qDebug() << "CameraManager created and is managing node:" << nodeId;
}

} // namespace Blueprint
} // namespace NeuralStudio
