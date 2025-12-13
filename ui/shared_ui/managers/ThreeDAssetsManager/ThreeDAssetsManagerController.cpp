#include "ThreeDAssetsManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDir>
#include <QDebug>
#include <QDir>
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

ThreeDAssetsManagerController::ThreeDAssetsManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	m_assetsDirectory = QDir::currentPath() + "/assets/3d";
	scanAssets();
}

ThreeDAssetsManagerController::~ThreeDAssetsManagerController() {}

void ThreeDAssetsManagerController::scanAssets()
{
	m_availableAssets.clear();
	QDir assetsDir(m_assetsDirectory);

	if (!assetsDir.exists()) {
		qWarning() << "3D assets directory does not exist:" << m_assetsDirectory;
		emit availableAssetsChanged();
		emit assetsChanged();
		return;
	}

	QStringList filters;
	filters << "*.obj" << "*.glb" << "*.gltf" << "*.fbx" << "*.dae";
	QFileInfoList files = assetsDir.entryInfoList(filters, QDir::Files);

	for (const QFileInfo &fileInfo : files) {
		QVariantMap asset;
		asset["name"] = fileInfo.baseName();
		asset["path"] = fileInfo.absoluteFilePath();
		asset["size"] = fileInfo.size();
		asset["format"] = fileInfo.suffix().toUpper();

		bool inUse = false;
		for (const QString &nodeId : managedNodes()) {
			if (getResourceForNode(nodeId) == fileInfo.absoluteFilePath()) {
				inUse = true;
				asset["nodeId"] = nodeId;
				asset["managed"] = true;
				break;
			}
		}
		asset["inUse"] = inUse;

		m_availableAssets.append(asset);
	}

	qDebug() << "Found" << m_availableAssets.size() << "3D assets," << managedNodes().size() << "managed nodes";
	emit availableAssetsChanged();
	emit assetsChanged();
}

void ThreeDAssetsManagerController::createThreeDModelNode(const QString &modelPath)
{
	if (!m_graphController)
		return;

	QString nodeId = m_graphController->createNode("ThreeDModelNode", 0.0f, 0.0f);
	if (nodeId.isEmpty())
		return;

	m_graphController->setNodeProperty(nodeId, "modelPath", modelPath);

	QVariantMap metadata;
	metadata["modelPath"] = modelPath;
	trackNode(nodeId, modelPath, metadata);

	scanAssets();
	qDebug() << "ThreeDAssetsManager managing node:" << nodeId;
}

} // namespace Blueprint
} // namespace NeuralStudio
