#pragma once

#include "checkbox.hpp"

class LockedCheckBox : public OBSCheckBox {
	Q_OBJECT

public:
	LockedCheckBox(QWidget *parent = nullptr);
};
