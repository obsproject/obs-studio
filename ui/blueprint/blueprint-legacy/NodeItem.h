#pragma once

#include <QGraphicsItem>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QString>
#include <QUuid>
#include <vector>
#include <QGraphicsObject>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QVector3D>
#include <QQuaternion>
#include "PortItem.h"

class NodeItem : public QGraphicsObject
{
    Q_OBJECT
      public:
    NodeItem(const QString &title, QGraphicsItem *parent = nullptr);

      signals:
    void nodeUpdated(const QString &uuid, const QJsonObject &data);

      public:
    void addInputPort(const QString &name, PortDataType type);
    void addOutputPort(const QString &name, PortDataType type);

    QUuid id() const
    {
        return m_id;
    }
    QString title() const
    {
        return m_title;
    }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // Spatial Properties (The Truth for the SceneGraph)
    void setSpatialPosition(const QVector3D &pos)
    {
        m_spatialPosition = pos;
        emitSpatialUpdate();
    }
    QVector3D spatialPosition() const
    {
        return m_spatialPosition;
    }

    void setSpatialRotation(const QQuaternion &rot)
    {
        m_spatialRotation = rot;
        emitSpatialUpdate();
    }
    QQuaternion spatialRotation() const
    {
        return m_spatialRotation;
    }

    void setSpatialScale(const QVector3D &scale)
    {
        m_spatialScale = scale;
        emitSpatialUpdate();
    }
    QVector3D spatialScale() const
    {
        return m_spatialScale;
    }

      protected:
    void emitSpatialUpdate();
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

      private:
    QUuid m_id;
    QString m_title;
    float m_width = 150;
    int m_height = 80;
    QColor m_headerColor;
    QColor m_bodyColor;
    QColor m_borderColor;

    // SceneGraph Spatial Data
    QVector3D m_spatialPosition {0.0f, 0.0f, 0.0f};
    QQuaternion m_spatialRotation;  // Identity by default
    QVector3D m_spatialScale {1.0f, 1.0f, 1.0f};

    std::vector<PortItem *> m_inputs;
    std::vector<PortItem *> m_outputs;

    void calculateLayout();
};
