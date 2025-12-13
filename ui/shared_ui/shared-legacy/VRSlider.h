#pragma once
#include <QSlider>
#include "VRCommon.h"

class VRSlider : public QSlider
{
    Q_OBJECT
      public:
    explicit VRSlider(Qt::Orientation orientation, QWidget *parent = nullptr);
};
