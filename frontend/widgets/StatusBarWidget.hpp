#pragma once

#include <QWidget>

class OBSBasicStatusBar;
class Ui_StatusBarWidget;

class StatusBarWidget : public QWidget {
	Q_OBJECT

	friend class OBSBasicStatusBar;

private:
	std::unique_ptr<Ui_StatusBarWidget> ui;

public:
	StatusBarWidget(QWidget *parent = nullptr);
	~StatusBarWidget();
};
