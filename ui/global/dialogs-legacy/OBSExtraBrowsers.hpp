#pragma once

#include <utility/ExtraBrowsersModel.hpp>

#include <QDialog>

class Ui_OBSExtraBrowsers;

class OBSExtraBrowsers : public QDialog {
	Q_OBJECT

	std::unique_ptr<Ui_OBSExtraBrowsers> ui;
	ExtraBrowsersModel *model;

public:
	OBSExtraBrowsers(QWidget *parent);
	~OBSExtraBrowsers();

	void closeEvent(QCloseEvent *event) override;

public slots:
	void on_apply_clicked();
};
