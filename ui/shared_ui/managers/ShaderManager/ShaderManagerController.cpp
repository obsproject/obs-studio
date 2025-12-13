#include "ShaderManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDir>
#include <QDebug>
#include <QDir>
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

ShaderManagerController::ShaderManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	m_shadersDirectory = QDir::currentPath() + "/assets/shaders";
	scanShaders();
}

ShaderManagerController::~ShaderManagerController() {}

void ShaderManagerController::scanShaders()
{
	m_availableShaders.clear();
	QDir shadersDir(m_shadersDirectory);

	if (!shadersDir.exists()) {
		qWarning() << "Shaders directory does not exist:" << m_shadersDirectory;
		emit availableShadersChanged();
		emit shadersChanged();
		return;
	}

	QStringList filters;
	filters << "*.glsl" << "*.vert" << "*.frag" << "*.comp";
	QFileInfoList files = shadersDir.entryInfoList(filters, QDir::Files);

	for (const QFileInfo &fileInfo : files) {
		QVariantMap shader;
		shader["name"] = fileInfo.baseName();
		shader["path"] = fileInfo.absoluteFilePath();
		shader["type"] = fileInfo.suffix().toUpper();

		bool inUse = false;
		for (const QString &nodeId : managedNodes()) {
			if (getResourceForNode(nodeId) == fileInfo.absoluteFilePath()) {
				inUse = true;
				shader["nodeId"] = nodeId;
				shader["managed"] = true;
				break;
			}
		}
		shader["inUse"] = inUse;

		m_availableShaders.append(shader);
	}

	qDebug() << "Found" << m_availableShaders.size() << "shaders," << managedNodes().size() << "managed nodes";
	emit availableShadersChanged();
	emit shadersChanged();
}

void ShaderManagerController::createShaderNode(const QString &shaderPath)
{
	if (!m_graphController)
		return;

	QString nodeId = m_graphController->createNode("ShaderNode", 0.0f, 0.0f);
	if (nodeId.isEmpty())
		return;

	m_graphController->setNodeProperty(nodeId, "shaderPath", shaderPath);

	QVariantMap metadata;
	metadata["shaderPath"] = shaderPath;
	trackNode(nodeId, shaderPath, metadata);

	scanShaders();
}

} // namespace Blueprint
} // namespace NeuralStudio
