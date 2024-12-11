#pragma once

#include <util/config-file.h>

#include <QMainWindow>

class OBSMainWindow : public QMainWindow {
	Q_OBJECT

public:
	inline OBSMainWindow(QWidget *parent) : QMainWindow(parent) {}

	virtual config_t *Config() const = 0;
	virtual void OBSInit() = 0;
};
