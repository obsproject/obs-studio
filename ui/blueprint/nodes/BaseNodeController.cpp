#include "BaseNodeController.h"
#include "../core/NodeGraphController.h"
#include <QDebug>
#include <QMetaType>
#include <any>

namespace NeuralStudio {
namespace UI {

BaseNodeController::BaseNodeController(QObject *parent) : QObject(parent) {}

void BaseNodeController::setNodeId(const QString &id)
{
	if (m_nodeId != id) {
		m_nodeId = id;
		emit nodeIdChanged();
		tryConnectToBackend();
	}
}

void BaseNodeController::setEnabled(bool enabled)
{
	if (m_enabled != enabled) {
		m_enabled = enabled;
		emit enabledChanged();
		updateProperty("enabled", enabled);
	}
}

void BaseNodeController::setGraphController(NodeGraphController *controller)
{
	if (m_graphController != controller) {
		m_graphController = controller;
		emit graphControllerChanged();
		tryConnectToBackend();
	}
}

// Port Configuration Implementation
void BaseNodeController::setVisualInputEnabled(bool enabled)
{
	if (m_visualInputEnabled != enabled) {
		m_visualInputEnabled = enabled;
		emit visualInputEnabledChanged();
		emit propertyChanged("visualInputEnabled", enabled);
	}
}

void BaseNodeController::setVisualOutputEnabled(bool enabled)
{
	if (m_visualOutputEnabled != enabled) {
		m_visualOutputEnabled = enabled;
		emit visualOutputEnabledChanged();
		emit propertyChanged("visualOutputEnabled", enabled);
	}
}

void BaseNodeController::setAudioInputEnabled(bool enabled)
{
	if (m_audioInputEnabled != enabled) {
		m_audioInputEnabled = enabled;
		emit audioInputEnabledChanged();
		emit propertyChanged("audioInputEnabled", enabled);
	}
}

void BaseNodeController::setAudioOutputEnabled(bool enabled)
{
	if (m_audioOutputEnabled != enabled) {
		m_audioOutputEnabled = enabled;
		emit audioOutputEnabledChanged();
		emit propertyChanged("audioOutputEnabled", enabled);
	}
}

void BaseNodeController::updateProperty(const QString &name, const QVariant &value)
{
	emit propertyChanged(name, value);

	// Propagate to backend
	if (auto backend = getBackendNode()) {
		std::any data;
		// Best-effort QVariant -> std::any conversion
		if (value.typeId() == QMetaType::Float || value.typeId() == QMetaType::Double)
			data = value.toFloat();
		else if (value.typeId() == QMetaType::Int)
			data = value.toInt();
		else if (value.typeId() == QMetaType::Bool)
			data = value.toBool();
		else if (value.typeId() == QMetaType::QString)
			data = value.toString().toStdString();

		if (data.has_value()) {
			backend->setPinData(name.toStdString(), data);
		}
	}
}

std::shared_ptr<SceneGraph::IExecutableNode> BaseNodeController::getBackendNode() const
{
	return m_backendNode.lock();
}

void BaseNodeController::tryConnectToBackend()
{
	if (m_graphController && !m_nodeId.isEmpty()) {
		// Retrieve backend graph
		auto graph = m_graphController->getBackendGraph();
		if (graph) {
			// Find node by ID
			auto backend = graph->getNode(m_nodeId.toStdString());
			if (backend) {
				m_backendNode = backend;
				// Optional: Sync initial properties from backend to UI if needed
			}
		}
	}
}

} // namespace UI
} // namespace NeuralStudio
