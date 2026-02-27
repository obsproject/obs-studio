#pragma once

#include "ui_OBSAbout.h"

#include <QDialog>

class OBSAbout : public QDialog {
	Q_OBJECT

private:
	QScopedPointer<QThread> patronJsonThread;

public:
	explicit OBSAbout(QWidget *parent = 0);
	~OBSAbout();

	std::unique_ptr<Ui::OBSAbout> ui;

private slots:
	void ShowAbout();
	void ShowAuthors();
	void ShowLicense();

	void updatePatronJson(const QString &text, const QString &error);
};
