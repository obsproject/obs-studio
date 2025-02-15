#pragma once

#include "ui_OBSAbout.h"

#include <QDialog>

class OBSAbout : public QDialog {
	Q_OBJECT

public:
	explicit OBSAbout(QWidget *parent = 0);

	std::unique_ptr<Ui::OBSAbout> ui;

private slots:
	void ShowAbout();
	void ShowAuthors();
	void ShowLicense();
};
