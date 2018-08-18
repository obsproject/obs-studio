#pragma once

#include <memory>
#include <QDialog>

#include "ui_OBSAbout.h"

class OBSAbout : public QDialog {
	Q_OBJECT

public:
	explicit OBSAbout(QWidget *parent = 0);

private:
	std::unique_ptr<Ui::OBSAbout> ui;

private slots:
	void ShowAbout();
	void ShowAuthors();
	void ShowLicense();
};
