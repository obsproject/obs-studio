#pragma once

#include "SourceToolbar.hpp"

#include <Idian/Utils.hpp>

class Ui_ColorSourceToolbar;

class ColorSourceToolbar : public SourceToolbar {
	Q_OBJECT

	std::unique_ptr<Ui_ColorSourceToolbar> ui;
	QColor color;

	std::unique_ptr<idian::Utils> utils;

	void UpdateColor();

public:
	ColorSourceToolbar(QWidget *parent, OBSSource source);
	~ColorSourceToolbar();

public slots:
	void on_colorPreview_clicked();
};
