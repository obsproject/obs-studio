#pragma once

#include "SourceToolbar.hpp"

class Ui_TextSourceToolbar;

class TextSourceToolbar : public SourceToolbar {
	Q_OBJECT

	std::unique_ptr<Ui_TextSourceToolbar> ui;
	QFont font;
	QColor color;

public:
	TextSourceToolbar(QWidget *parent, OBSSource source);
	~TextSourceToolbar();

public slots:
	void on_selectFont_clicked();
	void on_selectColor_clicked();
	void on_text_textChanged();
};
