#include "PortItem.h"
#include "NodeItem.h"
#include <QPainter>
#include <QToolTip>

PortItem::PortItem(NodeItem *parent, PortType type, const QString &name, PortDataType dataType)
	: QGraphicsItem(parent),
	  m_node(parent),
	  m_type(type),
	  m_name(name),
	  m_dataType(dataType)
{
	setAcceptHoverEvents(true);

	// Color coding based on data type
	switch (m_dataType) {
	case PortDataType::Video:
		m_color = QColor(255, 100, 100);
		break; // Red
	case PortDataType::Audio:
		m_color = QColor(100, 255, 100);
		break; // Green
	case PortDataType::Data:
		m_color = QColor(100, 100, 255);
		break; // Blue
	case PortDataType::Event:
		m_color = QColor(255, 255, 255);
		break; // White
	default:
		m_color = QColor(200, 200, 200); // Grey
	}

	// Tooltip for Accessibility
	QString typeStr = (m_type == PortType::Input ? "Input" : "Output");
	setToolTip(QString("%1 (%2)").arg(m_name, typeStr));
}

QRectF PortItem::boundingRect() const
{
	return QRectF(-6, -6, 12, 12); // 12x12 hit target (visual)
}

void PortItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	painter->setBrush(m_color);
	painter->setPen(QPen(Qt::black, 1));
	painter->drawEllipse(-5, -5, 10, 10);

	// Label
	painter->setPen(Qt::white);
	QFont font = painter->font();
	font.setPointSize(8);
	painter->setFont(font);

	if (m_type == PortType::Input) {
		painter->drawText(15, 5, m_name);
	} else {
		QFontMetrics fm(font);
		int w = fm.horizontalAdvance(m_name);
		painter->drawText(-15 - w, 5, m_name);
	}
}

NodeItem *PortItem::node() const
{
	return m_node;
}

QPointF PortItem::scenePos() const
{
	return mapToScene(0, 0);
}

QVariant PortItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	// Logic to update connections if moved
	return QGraphicsItem::itemChange(change, value);
}
