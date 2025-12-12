#pragma once
#include <QGraphicsItem>
#include <QBrush>
#include <QPen>

class NodeItem;
class ConnectionItem;

enum class PortType {
    Input,
    Output
};

enum class PortDataType {
    Any,
    Video,
    Audio,
    Data,
    Event
};

class PortItem : public QGraphicsItem
{
      public:
    PortItem(NodeItem *parent, PortType type, const QString &name, PortDataType dataType = PortDataType::Any);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    PortType portType() const
    {
        return m_type;
    }
    NodeItem *node() const;
    QPointF scenePos() const;  // Helper to get center in scene coords

      protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

      private:
    NodeItem *m_node;
    PortType m_type;
    QString m_name;
    PortDataType m_dataType;
    QColor m_color;
    friend class ConnectionItem;
};
