#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>
#include <QHash>
#include <QByteArray>

struct ConnectionData {
    QString id;
    QString sourceNodeId;
    QString targetNodeId;
    // Ports excluded for prototype simplicity (assume index 0 to 0)
};

class ConnectionModel : public QAbstractListModel
{
    Q_OBJECT
      public:
    enum ConnectionRoles {
        IdRole = Qt::UserRole + 1,
        SourceNodeRole,
        TargetNodeRole
    };

    explicit ConnectionModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addConnection(const QString &sourceId, const QString &targetId);

      private:
    QVector<ConnectionData> m_connections;
};
