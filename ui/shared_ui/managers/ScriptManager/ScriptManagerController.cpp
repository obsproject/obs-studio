#include "ScriptManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDir>
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

ScriptManagerController::ScriptManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	m_scriptsDirectory = QDir::currentPath() + "/assets/scripts";
	scanScripts();
}

ScriptManagerController::~ScriptManagerController() {}

void ScriptManagerController::scanScripts()
{
	m_availableScripts.clear();
	QDir scriptsDir(m_scriptsDirectory);

	if (!scriptsDir.exists()) {
		qWarning() << "Scripts directory does not exist:" << m_scriptsDirectory;
		emit availableScriptsChanged();
		emit scriptsChanged();
		return;
	}

	QStringList filters;
	filters << "*.lua" << "*.py" << "*.LUA" << "*.PY";
	QFileInfoList files = scriptsDir.entryInfoList(filters, QDir::Files);

	for (const QFileInfo &fileInfo : files) {
		QVariantMap script;
		script["name"] = fileInfo.baseName();
		script["path"] = fileInfo.absoluteFilePath();
		script["language"] = fileInfo.suffix().toLower() == "lua" ? "lua" : "python";

		// Check if this script is already in use
		bool inUse = false;
		for (const QString &nodeId : managedNodes()) {
			if (getResourceForNode(nodeId) == fileInfo.absoluteFilePath()) {
				inUse = true;
				script["nodeId"] = nodeId;
				script["managed"] = true;
				break;
			}
		}
		script["inUse"] = inUse;

		m_availableScripts.append(script);
	}

	qDebug() << "Found" << m_availableScripts.size() << "scripts," << managedNodes().size() << "managed nodes";
	emit availableScriptsChanged();
	emit scriptsChanged();
}

void ScriptManagerController::createScriptNode(const QString &scriptPath, const QString &language)
{
	if (!m_graphController) {
		qWarning() << "No graph controller set";
		return;
	}

	QString nodeId = m_graphController->createNode("ScriptNode", 0.0f, 0.0f);
	if (nodeId.isEmpty()) {
		qWarning() << "Failed to create ScriptNode";
		return;
	}

	m_graphController->setNodeProperty(nodeId, "scriptPath", scriptPath);
	m_graphController->setNodeProperty(nodeId, "language", language);

	// Track with metadata
	QVariantMap metadata;
	metadata["scriptPath"] = scriptPath;
	metadata["language"] = language;
	trackNode(nodeId, scriptPath, metadata);

	scanScripts();
	qDebug() << "ScriptManager created and is managing node:" << nodeId;
}

} // namespace Blueprint
} // namespace NeuralStudio
