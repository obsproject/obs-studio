#pragma once
#include <QGraphicsPathItem>

class PortItem;

class ConnectionItem : public QGraphicsPathItem
{
      public:
    ConnectionItem(PortItem *start, PortItem *end);

    void updatePath();
    PortItem *startPort() const
    {
        return m_start;
    }
    PortItem *endPort() const
    {
        return m_end;
    }

    enum {
        Type = UserType + 2
    };
    int type() const override
    {
        return Type;
    }

      private:
    PortItem *m_start;
    PortItem *m_end;
};
