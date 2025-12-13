#include "NodeItem.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QJsonObject>
#include <QJsonArray>

#include <QGraphicsDropShadowEffect>
#include <QLinearGradient>

NodeItem::NodeItem(const QString &title, QGraphicsItem *parent)
	: QGraphicsObject(parent),
	  m_id(QUuid::createUuid()),
	  m_title(title)
{
	setFlag(ItemIsMovable);
	setFlag(ItemIsSelectable);
	setFlag(ItemSendsGeometryChanges); // Important for connections later

	// Premium Colors
	m_headerColor = QColor(60, 60, 60);        // Gradient base
	m_bodyColor = QColor(30, 30, 35, 220);     // Dark semi-transparent
	m_borderColor = QColor(255, 255, 255, 30); // Subtle highlight

	// Shadow Effect
	auto shadow = new QGraphicsDropShadowEffect();
	shadow->setBlurRadius(20);
	shadow->setOffset(0, 4);
	shadow->setColor(QColor(0, 0, 0, 100));
	setGraphicsEffect(shadow);
}

QRectF NodeItem::boundingRect() const
{
	return QRectF(0, 0, m_width, m_height);
}

void NodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);

	painter->setRenderHint(QPainter::Antialiasing);

	// Body Background
	painter->setBrush(m_bodyColor);
	painter->setPen(Qt::NoPen);
	painter->drawRoundedRect(0, 0, m_width, m_height, 8, 8); // Softer corners

	// Header Gradient
	QRectF headerRect(0, 0, m_width, 30);
	QLinearGradient gradient(headerRect.topLeft(), headerRect.bottomLeft());
	gradient.setColorAt(0, QColor(60, 60, 65));
	gradient.setColorAt(1, QColor(40, 40, 45));

	painter->setBrush(gradient);
	// Clip header to top rounded corners
	QPainterPath path;
	path.addRoundedRect(0, 0, m_width, m_height, 8, 8);
	painter->setClipPath(path);
	painter->drawRect(headerRect); // Draw strict rect over clipped area
	painter->setClipping(false);

	// Border (Overlay)
	if (isSelected()) {
		painter->setPen(QPen(QColor(0, 120, 215), 2));
	} else {
		painter->setPen(QPen(m_borderColor, 1));
	}
	painter->setBrush(Qt::NoBrush);
	painter->drawRoundedRect(0, 0, m_width, m_height, 8, 8);

	// Title
	painter->setPen(QColor(240, 240, 240));
	painter->setFont(QFont("Segoe UI", 9, QFont::Bold));
	QRectF textRect = headerRect;
	textRect.setLeft(10); // Check padding
	painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_title);
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
