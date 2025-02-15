#pragma once

#include "SourceToolbar.hpp"

class Ui_ColorSourceToolbar;

class ColorSourceToolbar : public SourceToolbar {
	Q_OBJECT

	std::unique_ptr<Ui_ColorSourceToolbar> ui;
	QColor color;

	void UpdateColor();

public:
	ColorSourceToolbar(QWidget *parent, OBSSource source);
	~ColorSourceToolbar();

public slots:
	void on_choose_clicked();
};
