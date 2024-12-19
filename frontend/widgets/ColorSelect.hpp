#pragma once

#include "ui_ColorSelect.h"

#include <QWidget>

class ColorSelect : public QWidget {

public:
	explicit ColorSelect(QWidget *parent = 0);

private:
	std::unique_ptr<Ui::ColorSelect> ui;
};
