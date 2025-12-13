#pragma once

#include <QWidget>
#include <QGridLayout>
#include <vector>
#include "MonitorWidget.h"

class MonitorGrid : public QWidget
{
    Q_OBJECT
      public:
    explicit MonitorGrid(QWidget *parent = nullptr);
    void addMonitor(const QString &title);

      private:
    QGridLayout *m_layout;
    std::vector<MonitorWidget *> m_monitors;
    int m_cols = 3;
};
