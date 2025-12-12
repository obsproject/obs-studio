#include "NodeItem.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>

NodeItem::NodeItem(const QString &title, QGraphicsItem *parent)
	: QGraphicsItem(parent),
	  m_id(QUuid::createUuid()),
	  m_title(title)
{
	setFlag(ItemIsMovable);
	setFlag(ItemIsSelectable);
	setFlag(ItemSendsGeometryChanges); // Important for connections later

	m_headerColor = QColor(60, 60, 60);
	m_bodyColor = QColor(40, 40, 40);
	m_borderColor = QColor(20, 20, 20);
}

QRectF NodeItem::boundingRect() const
{
	return QRectF(0, 0, m_width, m_height);
}

void NodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	// Body
	painter->setBrush(m_bodyColor);
	painter->setPen(QPen(m_borderColor, 2));
	painter->drawRoundedRect(0, 0, m_width, m_height, 5, 5);

	// Header
	QRectF headerRect(0, 0, m_width, 25);
	painter->setBrush(m_headerColor);
	painter->setPen(Qt::NoPen);
	painter->drawRoundedRect(headerRect, 5, 5);
	// Fix bottom corners of header to be square so they merge with body, or just draw over
	// Simpler: Just draw header content.

	// Selection Highlight
	if (isSelected()) {
		painter->setPen(QPen(QColor(255, 165, 0), 2));
		painter->setBrush(Qt::NoBrush);
		painter->drawRoundedRect(0, 0, m_width, m_height, 5, 5);
	}

	// Title
	painter->setPen(Qt::white);
	painter->setFont(QFont("Arial", 10, QFont::Bold));
	painter->drawText(headerRect, Qt::AlignCenter, m_title);
}

void NodeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	update();
	QGraphicsItem::mousePressEvent(event);
}

QVariant NodeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemPositionHasChanged) {
		// In future: Notify connected links to update
		// for (auto port : m_inputs) ...
		// for (auto port : m_outputs) ...
	}
	return QGraphicsItem::itemChange(change, value);
}

void NodeItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mouseMoveEvent(event);
}

void NodeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	update();
	QGraphicsItem::mouseReleaseEvent(event);
}
