#pragma once
#include <QWidget>
#include "VRButton.h"
#include <QComboBox>

class CamModule : public QWidget
{
    Q_OBJECT
      public:
    explicit CamModule(QWidget *parent = nullptr);

      private:
    QComboBox *m_deviceList;
    VRButton *m_activateBtn;
};
