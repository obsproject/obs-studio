#include "NodeModel.h"
#include <QDebug>
#include <QJsonArray>
#include "VRProtocol.h"

NodeModel::NodeModel(QObject *parent) : QAbstractListModel(parent)
{
	// Add some test nodes for immediate visual feedback
	// Mock connection logic demo
	addNode("Webcam Input", "CameraNode", 100, 100, "11111111-1111-1111-1111-111111111111");
	addNode("Face Filter", "EffectNode", 400, 150, "22222222-2222-2222-2222-222222222222");

	// Extended Nodes
	addNode("Crossfade", "TransitionNode", 700, 200);
	addNode("Start Trigger", "EventTriggerNode", 100, 400);
	addNode("Gemini Transcribe", "LLMTranscriptionNode", 400, 400);

	// Video Node Test
	addNode("4K Sample", "VideoNode", 100, 250);
}

int NodeModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	return m_nodes.count();
}

QVariant NodeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_nodes.count())
		return QVariant();

	const NodeData &node = m_nodes[index.row()];
	switch (role) {
	case IdRole:
		return node.id;
	case TitleRole:
		return node.title;
	case XRole:
		return node.x;
	case YRole:
		return node.y;
	case ZRole:
		return node.z;
	case TypeRole:
		return node.type;
	case LabelRole:
		return node.label;
	case Is3DRole:
		return node.is3D;
	}
	return QVariant();
}

QHash<int, QByteArray> NodeModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[IdRole] = "nodeId";
	roles[TitleRole] = "nodeTitle";
	roles[XRole] = "nodeX";
	roles[YRole] = "nodeY";
	roles[ZRole] = "nodeZ";
	roles[TypeRole] = "nodeType";
	roles[LabelRole] = "label";
	roles[Is3DRole] = "is3D";
	return roles;
}

void NodeModel::addNode(const QString &title, const QString &type, float x, float y, const QString &forceId)
{
	beginInsertRows(QModelIndex(), m_nodes.count(), m_nodes.count());

	NodeData node;
	node.id = forceId.isEmpty() ? QUuid::createUuid().toString() : forceId;
	node.title = title;
	node.type = type;
	node.x = x;
	node.y = y;
	node.z = 0; // Default 2D nodes to z=0
	node.label = title;
	node.is3D = false;

	m_nodes.append(node);

	endInsertRows();

	// Notify Backend
	QJsonObject args;
	args["title"] = title;
	args["id"] = node.id;
	args["x"] = (int)x;
	args["y"] = (int)y;
	args["label"] = title;
	args["category"] = "General"; // simplified

	VRClient::instance().sendCommand(QString::fromUtf8(VRProtocol::CMD_CREATE_NODE), args);
}

void NodeModel::updatePosition(const QString &id, float x, float y)
{
	for (int i = 0; i < m_nodes.count(); ++i) {
		if (m_nodes[i].id == id) {
			m_nodes[i].x = x;
			m_nodes[i].y = y;
			QModelIndex idx = index(i);
			emit dataChanged(idx, idx, {XRole, YRole});

			// IPC Update
			QJsonObject updates;
			QJsonArray posArray = {x, y, 0.0}; // Z=0 for canvas
			updates["position"] = posArray;
			VRClient::instance().updateNode(id, updates);

			return;
		}
	}
}

void NodeModel::addNode3D(const QString &title, const QString &type, float x, float y, float z, const QString &label,
			  const QString &forceId)
{
	beginInsertRows(QModelIndex(), m_nodes.count(), m_nodes.count());

	NodeData node;
	node.id = forceId.isEmpty() ? QUuid::createUuid().toString() : forceId;
	node.title = title;
	node.type = type;
	node.x = x;
	node.y = y;
	node.z = z;
	node.label = label.isEmpty() ? title : label;
	node.is3D = true;

	m_nodes.append(node);

	endInsertRows();

	// Notify Backend with 3D position
	QJsonObject args;
	args["title"] = title;
	args["id"] = node.id;
	QJsonArray posArray = {x, y, z};
	args["position"] = posArray;
	args["label"] = node.label;
	args["category"] = "Spatial";

	VRClient::instance().sendCommand(QString::fromUtf8(VRProtocol::CMD_CREATE_NODE), args);
}

void NodeModel::updatePosition3D(const QString &id, float x, float y, float z)
{
	for (int i = 0; i < m_nodes.count(); ++i) {
		if (m_nodes[i].id == id) {
			m_nodes[i].x = x;
			m_nodes[i].y = y;
			m_nodes[i].z = z;
			QModelIndex idx = index(i);
			emit dataChanged(idx, idx, {XRole, YRole, ZRole});

			// IPC Update
			QJsonObject updates;
			QJsonArray posArray = {x, y, z};
			updates["position"] = posArray;
			VRClient::instance().updateNode(id, updates);

			return;
		}
	}
}
