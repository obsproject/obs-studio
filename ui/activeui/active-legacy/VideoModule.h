#pragma once
#include <QWidget>
#include "VRFilePicker.h"
#include "VRButton.h"
#include "VRSlider.h"

class VideoModule : public QWidget
{
    Q_OBJECT
      public:
    explicit VideoModule(QWidget *parent = nullptr);

      private:
    VRFilePicker *m_filePicker;
    VRButton *m_playBtn;
    VRButton *m_stopBtn;
    VRSlider *m_seekSlider;
};
