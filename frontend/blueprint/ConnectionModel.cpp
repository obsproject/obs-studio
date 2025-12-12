#include "ConnectionModel.h"
#include <QUuid>

ConnectionModel::ConnectionModel(QObject *parent) : QAbstractListModel(parent)
{
	// Mock connection
	addConnection("11111111-1111-1111-1111-111111111111", "22222222-2222-2222-2222-222222222222");
}

int ConnectionModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;
	return m_connections.count();
}

QVariant ConnectionModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_connections.count())
		return QVariant();

	const ConnectionData &conn = m_connections[index.row()];
	switch (role) {
	case IdRole:
		return conn.id;
	case SourceNodeRole:
		return conn.sourceNodeId;
	case TargetNodeRole:
		return conn.targetNodeId;
	}
	return QVariant();
}

QHash<int, QByteArray> ConnectionModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[IdRole] = "connId";
	roles[SourceNodeRole] = "sourceNodeId";
	roles[TargetNodeRole] = "targetNodeId";
	return roles;
}

void ConnectionModel::addConnection(const QString &sourceId, const QString &targetId)
{
	beginInsertRows(QModelIndex(), m_connections.count(), m_connections.count());

	ConnectionData conn;
	conn.id = QUuid::createUuid().toString();
	conn.sourceNodeId = sourceId;
	conn.targetNodeId = targetId;

	m_connections.append(conn);

	endInsertRows();
}
