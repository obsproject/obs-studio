#pragma once

#include "SourceToolbar.hpp"

class Ui_ImageSourceToolbar;

class ImageSourceToolbar : public SourceToolbar {
	Q_OBJECT

	std::unique_ptr<Ui_ImageSourceToolbar> ui;

public:
	ImageSourceToolbar(QWidget *parent, OBSSource source);
	~ImageSourceToolbar();

public slots:
	void on_browse_clicked();
};
