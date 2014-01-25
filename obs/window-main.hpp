#pragma once

#include <QMainWindow>

class OBSMainWindow : public QMainWindow {
	Q_OBJECT

public:
	inline OBSMainWindow(QWidget *parent) : QMainWindow(parent) {}

	virtual void OBSInit()=0;
};
