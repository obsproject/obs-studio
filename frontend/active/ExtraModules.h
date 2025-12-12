#pragma once
#include <QWidget>
#include "VRFilePicker.h"
#include "VRSlider.h"

class PhotoModule : public QWidget
{
    Q_OBJECT
      public:
    explicit PhotoModule(QWidget *parent = nullptr);
};

class StreamModule : public QWidget
{
    Q_OBJECT
      public:
    explicit StreamModule(QWidget *parent = nullptr);
};
