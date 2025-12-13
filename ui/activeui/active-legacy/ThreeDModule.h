#pragma once
#include <QWidget>
#include "VRFilePicker.h"
#include "VRSlider.h"

class ThreeDModule : public QWidget
{
    Q_OBJECT
      public:
    explicit ThreeDModule(QWidget *parent = nullptr);

      private:
    VRFilePicker *m_meshPicker;
    VRSlider *m_scaleSlider;
    VRSlider *m_rotSlider;
};
