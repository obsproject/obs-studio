#pragma once

#include <QGraphicsItem>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QString>
#include <QUuid>
#include <vector>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include "PortItem.h"

class NodeItem : public QGraphicsItem
{
      public:
    NodeItem(const QString &title, QGraphicsItem *parent = nullptr);

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

      protected:
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
};
