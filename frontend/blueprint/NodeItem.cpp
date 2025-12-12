#include "NodeItem.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QJsonObject>
#include <QJsonArray>

NodeItem::NodeItem(const QString &title, QGraphicsItem *parent)
	: QGraphicsObject(parent),
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
		// Sync 2D position to 3D Spatial Position (assuming Z=0 for now)
		m_spatialPosition.setX(pos().x());
		m_spatialPosition.setY(pos().y());
		emitSpatialUpdate();
	}
	return QGraphicsItem::itemChange(change, value);
}

void NodeItem::emitSpatialUpdate()
{
	QJsonObject spatialData;
	QJsonArray posArray = {m_spatialPosition.x(), m_spatialPosition.y(), m_spatialPosition.z()};
	QJsonArray rotArray = {m_spatialRotation.scalar(), m_spatialRotation.x(), m_spatialRotation.y(),
			       m_spatialRotation.z()};
	QJsonArray scaleArray = {m_spatialScale.x(), m_spatialScale.y(), m_spatialScale.z()};

	spatialData["position"] = posArray;
	spatialData["rotation"] = rotArray;
	spatialData["scale"] = scaleArray;

	// Emit generic update
	emit nodeUpdated(m_id.toString(), spatialData);
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

void NodeItem::addInputPort(const QString &name, PortDataType type)
{
	PortItem *port = new PortItem(this, PortType::Input, name, type);
	m_inputs.push_back(port);
	calculateLayout();
}

void NodeItem::addOutputPort(const QString &name, PortDataType type)
{
	PortItem *port = new PortItem(this, PortType::Output, name, type);
	m_outputs.push_back(port);
	calculateLayout();
}

void NodeItem::calculateLayout()
{
	// Simple vertical layout
	const int headerHeight = 30;
	const int portStep = 25;

	int currentY = headerHeight + 10;

	for (auto port : m_inputs) {
		port->setPos(0, currentY); // Left edge
		currentY += portStep;
	}

	// Reset for outputs, but maybe align right
	currentY = headerHeight + 10;
	for (auto port : m_outputs) {
		port->setPos(m_width, currentY); // Right edge
		currentY += portStep;
	}

	// Resize node if needed
	int maxPorts = std::max(m_inputs.size(), m_outputs.size());
	int neededHeight = headerHeight + 10 + (maxPorts * portStep) + 10;
	if (neededHeight > m_height) {
		m_height = neededHeight;
		update();
	}
}
