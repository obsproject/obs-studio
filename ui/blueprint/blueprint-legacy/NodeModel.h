#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QUuid>
#include "VRClient.h"

struct NodeData {
    QString id;
    QString title;
    float x;
    float y;
    float z;
    QString type;
    QString label;  // Semantic label (e.g., "Office Desk")
    bool is3D;      // Whether this node exists in 3D space
};

class NodeModel : public QAbstractListModel
{
    Q_OBJECT
      public:
    enum NodeRoles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        XRole,
        YRole,
        ZRole,
        TypeRole,
        LabelRole,
        Is3DRole
    };

    explicit NodeModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addNode(const QString &title, const QString &type, float x, float y,
                             const QString &forceId = QString());
    Q_INVOKABLE void addNode3D(const QString &title, const QString &type, float x, float y, float z,
                               const QString &label = QString(), const QString &forceId = QString());
    Q_INVOKABLE void updatePosition(const QString &id, float x, float y);
    Q_INVOKABLE void updatePosition3D(const QString &id, float x, float y, float z);

      private:
    QVector<NodeData> m_nodes;
};
