#pragma once
#include "ui_OBSPluginManagerWindow.h"
#include "OBSPluginManager.hpp"

#include <QDialog>
#include <QWidget>

class OBSPluginManagerWindow : public QDialog {
	Q_OBJECT
	std::unique_ptr<Ui::OBSPluginManagerWindow> ui;

public:
	explicit OBSPluginManagerWindow(std::vector<OBSModuleInfo> const &modules, QWidget *parent = nullptr);
	inline std::vector<OBSModuleInfo> const result() { return modules_; }

private:
	std::vector<OBSModuleInfo> modules_;
};
