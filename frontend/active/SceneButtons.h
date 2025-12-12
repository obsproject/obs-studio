#pragma once

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>

class SceneButtons : public QWidget
{
    Q_OBJECT
      public:
    explicit SceneButtons(QWidget *parent = nullptr);
    void addScene(const QString &name);

      private:
    QGridLayout *m_layout;
    int m_maxCols = 2;
    int m_count = 0;
};
