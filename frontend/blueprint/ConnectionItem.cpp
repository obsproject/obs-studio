#include "ConnectionItem.h"
#include "PortItem.h"
#include <QPen>
#include <QPainterPath>

ConnectionItem::ConnectionItem(PortItem *start, PortItem *end) : m_start(start), m_end(end)
{
	setZValue(-1); // Behind nodes
	setPen(QPen(QColor(200, 200, 200), 2));
	updatePath();
}

void ConnectionItem::updatePath()
{
	if (!m_start || !m_end)
		return;

	QPointF p1 = m_start->scenePos();
	QPointF p2 = m_end->scenePos();

	QPainterPath path;
	path.moveTo(p1);

	qreal dx = p2.x() - p1.x();
	qreal dy = p2.y() - p1.y();

	QPointF c1(p1.x() + dx * 0.5, p1.y());
	QPointF c2(p2.x() - dx * 0.5, p2.y());

	path.cubicTo(c1, c2, p2);
	setPath(path);
}
