#include "MLManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDir>
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

MLManagerController::MLManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	m_modelsDirectory = QDir::currentPath() + "/assets/models";
	scanModels();
}

MLManagerController::~MLManagerController() {}

void MLManagerController::scanModels()
{
	m_availableModels.clear();
	QDir modelsDir(m_modelsDirectory);

	if (!modelsDir.exists()) {
		qWarning() << "Models directory does not exist:" << m_modelsDirectory;
		emit availableModelsChanged();
		emit modelsChanged();
		return;
	}

	QStringList filters;
	filters << "*.onnx" << "*.ONNX";
	QFileInfoList files = modelsDir.entryInfoList(filters, QDir::Files);

	for (const QFileInfo &fileInfo : files) {
		QVariantMap model;
		model["name"] = fileInfo.baseName();
		model["path"] = fileInfo.absoluteFilePath();
		model["type"] = "ONNX";
		model["size"] = fileInfo.size();

		// Check if this model is already in use by a managed node
		bool inUse = false;
		for (const QString &nodeId : managedNodes()) {
			if (getResourceForNode(nodeId) == fileInfo.absoluteFilePath()) {
				inUse = true;
				model["nodeId"] = nodeId;
				model["managed"] = true;
				break;
			}
		}
		model["inUse"] = inUse;

		m_availableModels.append(model);
	}

	qDebug() << "Found" << m_availableModels.size() << "ONNX models," << managedNodes().size() << "managed nodes";
	emit availableModelsChanged();
	emit modelsChanged();
}

void MLManagerController::createMLNode(const QString &modelPath)
{
	if (!m_graphController) {
		qWarning() << "No graph controller set";
		return;
	}

	// Create the node
	QString nodeId = m_graphController->createNode("MLNode", 0.0f, 0.0f, 0.0f);
	if (nodeId.isEmpty()) {
		qWarning() << "Failed to create MLNode";
		return;
	}

	// Set the model path property
	m_graphController->setNodeProperty(nodeId, "modelPath", modelPath);

	// Track this node in the manager hub
	QVariantMap metadata;
	metadata["modelPath"] = modelPath;
	metadata["modelType"] = "ONNX";
	trackNode(nodeId, modelPath, metadata);

	// Refresh the model list to update "in use" status
	scanModels();

	qDebug() << "MLManager created and is now managing node:" << nodeId;
}

} // namespace Blueprint
} // namespace NeuralStudio
