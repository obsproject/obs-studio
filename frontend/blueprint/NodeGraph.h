#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>

class NodeGraph : public QGraphicsView
{
    Q_OBJECT
      public:
    explicit NodeGraph(QWidget *parent = nullptr);
    void addNode(const QString &title, int x, int y);
    QGraphicsScene *scene() const
    {
        return m_scene;
    }

      protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

      private:
    QGraphicsScene *m_scene;
    bool isPanning = false;
    QPoint lastPanPos;
};
