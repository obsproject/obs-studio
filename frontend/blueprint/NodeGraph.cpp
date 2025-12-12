#include "NodeGraph.h"
#include "NodeItem.h"
#include "NodeRegistry.h"
#include <QScrollBar>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>
#include <QWheelEvent>
#include <QGraphicsSceneMouseEvent>
#include <QContextMenuEvent>
#include <cmath>

NodeGraph::NodeGraph(QWidget *parent) : QGraphicsView(parent)
{
	m_scene = new QGraphicsScene(this);
	m_scene->setSceneRect(-5000, -5000, 10000, 10000);
	setScene(m_scene);

	setRenderHint(QPainter::Antialiasing);
	setRenderHint(QPainter::SmoothPixmapTransform);
	setRenderHint(QPainter::TextAntialiasing);

	setDragMode(QGraphicsView::ScrollHandDrag);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setResizeAnchor(QGraphicsView::AnchorUnderMouse);

	// Grid background
	setBackgroundBrush(QBrush(QColor(30, 30, 30), Qt::CrossPattern));
}

void NodeGraph::addNode(const QString &title, int x, int y)
{
	NodeItem *node = new NodeItem(title);
	node->setPos(x, y);
	m_scene->addItem(node);
}

void NodeGraph::wheelEvent(QWheelEvent *event)
{
	const double scaleFactor = 1.15;
	if (event->angleDelta().y() > 0) {
		scale(scaleFactor, scaleFactor);
	} else {
		scale(1.0 / scaleFactor, 1.0 / scaleFactor);
	}
}

void NodeGraph::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::MiddleButton) {
		// Pan logic if needed, or rely on ScrollHandDrag
	}
	QGraphicsView::mousePressEvent(event);
}

void NodeGraph::mouseMoveEvent(QMouseEvent *event)
{
	QGraphicsView::mouseMoveEvent(event);
}

void NodeGraph::mouseReleaseEvent(QMouseEvent *event)
{
	QGraphicsView::mouseReleaseEvent(event);
}

void NodeGraph::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu menu(this);

	for (int i = 0; i <= (int)NodeCategory::IncomingStreams; ++i) {
		NodeCategory cat = (NodeCategory)i;
		QMenu *catMenu = menu.addMenu(NodeRegistry::instance().categoryName(cat));

		auto nodes = NodeRegistry::instance().getNodesByCategory(cat);
		for (const auto &nodeDef : nodes) {
			catMenu->addAction(nodeDef.name, [this, nodeDef, event]() {
				QPoint pos = event->pos(); // View pos
				QPointF scenePos = mapToScene(pos);
				this->addNode(nodeDef.name, scenePos.x(), scenePos.y());
			});
		}
	}

	menu.exec(event->globalPos());
}
