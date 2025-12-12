#pragma once
#include <QPushButton>
#include "VRCommon.h"

class VRButton : public QPushButton
{
    Q_OBJECT
      public:
    explicit VRButton(const QString &text, QWidget *parent = nullptr);
};
