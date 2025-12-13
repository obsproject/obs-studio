#include "BaseManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <QDebug>

namespace NeuralStudio {
namespace UI {

BaseManagerController::BaseManagerController(QObject *parent) : QObject(parent), m_graphController(nullptr)
{
	loadPresetsFromDisk();
}

void BaseManagerController::setGraphController(NodeGraphController *controller)
{
	if (m_graphController) {
		disconnect(m_graphController, nullptr, this, nullptr);
	}

	m_graphController = controller;

	if (m_graphController) {
		// Listen for node deletion to clean up tracking
		connect(m_graphController, &NodeGraphController::nodeDeleted, this,
			&BaseManagerController::onNodeDeleted);
	}

	emit graphControllerChanged();
}

QStringList BaseManagerController::managedNodes() const
{
	return m_managedNodeInfo.keys();
}

bool BaseManagerController::isManaging(const QString &nodeId) const
{
	return m_managedNodeInfo.contains(nodeId);
}

QString BaseManagerController::getResourceForNode(const QString &nodeId) const
{
	if (!m_managedNodeInfo.contains(nodeId))
		return QString();
	return m_managedNodeInfo.value(nodeId).value("resourceId").toString();
}

QVariantMap BaseManagerController::getNodeInfo(const QString &nodeId) const
{
	return m_managedNodeInfo.value(nodeId);
}

void BaseManagerController::trackNode(const QString &nodeId, const QString &resourceId, const QVariantMap &metadata)
{
	QVariantMap info;
	info["resourceId"] = resourceId;
	info["nodeType"] = nodeType();
	info["createdTime"] = QDateTime::currentDateTime();

	// Merge custom metadata
	for (auto it = metadata.begin(); it != metadata.end(); ++it) {
		info[it.key()] = it.value();
	}

	m_managedNodeInfo[nodeId] = info;
	emit managedNodesChanged();

	qDebug() << title() << "now managing" << m_managedNodeInfo.size() << "nodes";
}

void BaseManagerController::untrackNode(const QString &nodeId)
{
	if (m_managedNodeInfo.remove(nodeId)) {
		emit managedNodesChanged();
		qDebug() << title() << "untracked node:" << nodeId;
	}
}

void BaseManagerController::onNodeDeleted(const QString &nodeId)
{
	if (isManaging(nodeId)) {
		qDebug() << title() << "detected deletion of managed node:" << nodeId;
		untrackNode(nodeId);
	}
}

void BaseManagerController::savePreset(const QString &nodeId, const QString &presetName)
{
	if (!m_graphController || !isManaging(nodeId)) {
		qWarning() << "Cannot save preset for unmanaged node:" << nodeId;
		return;
	}

	// Get node properties from graph controller
	// TODO: Add getNodeProperties() to NodeGraphController
	QVariantMap preset;
	preset["name"] = presetName;
	preset["nodeType"] = nodeType();
	preset["resourceId"] = getResourceForNode(nodeId);
	preset["savedTime"] = QDateTime::currentDateTime();

	// Remove existing preset with same name
	for (int i = 0; i < m_presets.size(); ++i) {
		if (m_presets[i].toMap().value("name").toString() == presetName) {
			m_presets.removeAt(i);
			break;
		}
	}

	m_presets.append(preset);
	savePresetsToDisk();
	emit nodePresetsChanged();

	qDebug() << title() << "saved preset:" << presetName;
}

QString BaseManagerController::loadPreset(const QString &presetName, float x, float y)
{
	// Find preset
	QVariantMap preset;
	for (const QVariant &p : m_presets) {
		if (p.toMap().value("name").toString() == presetName) {
			preset = p.toMap();
			break;
		}
	}

	if (preset.isEmpty()) {
		qWarning() << "Preset not found:" << presetName;
		return QString();
	}

	// Create node
	if (!m_graphController)
		return QString();

	QString nodeId = m_graphController->createNode(nodeType(), x, y);
	if (nodeId.isEmpty())
		return QString();

	// Apply preset properties
	QString resourceId = preset.value("resourceId").toString();

	// Track the node
	QVariantMap metadata;
	metadata["loadedFromPreset"] = presetName;
	trackNode(nodeId, resourceId, metadata);

	qDebug() << title() << "loaded preset" << presetName << "as node" << nodeId;
	return nodeId;
}

void BaseManagerController::deletePreset(const QString &presetName)
{
	for (int i = 0; i < m_presets.size(); ++i) {
		if (m_presets[i].toMap().value("name").toString() == presetName) {
			m_presets.removeAt(i);
			savePresetsToDisk();
			emit nodePresetsChanged();
			qDebug() << title() << "deleted preset:" << presetName;
			return;
		}
	}
}

void BaseManagerController::createNode(const QString &nodeType, float x, float y)
{
	if (!m_graphController)
		return;

	QString nodeId = m_graphController->createNode(nodeType, x, y);
	if (!nodeId.isEmpty()) {
		trackNode(nodeId, "manual");
	}
}

QString BaseManagerController::getPresetsFilePath() const
{
	QDir configDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
	if (!configDir.exists()) {
		configDir.mkpath(".");
	}

	QString filename = nodeType().toLower() + "_presets.json";
	return configDir.filePath(filename);
}

void BaseManagerController::loadPresetsFromDisk()
{
	QString filePath = getPresetsFilePath();
	QFile file(filePath);

	if (!file.open(QIODevice::ReadOnly)) {
		qDebug() << title() << "no saved presets found";
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	file.close();

	if (doc.isArray()) {
		m_presets.clear();
		QJsonArray array = doc.array();
		for (const QJsonValue &val : array) {
			m_presets.append(val.toObject().toVariantMap());
		}
		qDebug() << title() << "loaded" << m_presets.size() << "presets from disk";
		emit nodePresetsChanged();
	}
}

void BaseManagerController::savePresetsToDisk()
{
	QString filePath = getPresetsFilePath();
	QFile file(filePath);

	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Failed to save presets to" << filePath;
		return;
	}

	QJsonArray array;
	for (const QVariant &preset : m_presets) {
		array.append(QJsonObject::fromVariantMap(preset.toMap()));
	}

	QJsonDocument doc(array);
	file.write(doc.toJson());
	file.close();

	qDebug() << title() << "saved" << m_presets.size() << "presets to disk";
}

} // namespace UI
} // namespace NeuralStudio
