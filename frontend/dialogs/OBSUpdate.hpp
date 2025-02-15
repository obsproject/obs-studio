#pragma once

#include <QDialog>

class Ui_OBSUpdate;

class OBSUpdate : public QDialog {
	Q_OBJECT

public:
	enum ReturnVal { No, Yes, Skip };

	OBSUpdate(QWidget *parent, bool manualUpdate, const QString &text);

public slots:
	void on_yes_clicked();
	void on_no_clicked();
	void on_skip_clicked();
	virtual void accept() override;
	virtual void reject() override;

private:
	std::unique_ptr<Ui_OBSUpdate> ui;
};
