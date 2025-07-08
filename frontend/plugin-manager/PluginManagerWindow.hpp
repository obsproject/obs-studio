#pragma once
#include "ui_PluginManagerWindow.h"
#include "PluginManager.hpp"

#include <QDialog>
#include <QWidget>

namespace OBS {

class PluginManagerWindow : public QDialog {
	Q_OBJECT
	std::unique_ptr<Ui::PluginManagerWindow> ui;

public:
	explicit PluginManagerWindow(std::vector<ModuleInfo> const &modules, QWidget *parent = nullptr);
	inline std::vector<ModuleInfo> const result() { return modules_; }

private:
	std::vector<ModuleInfo> modules_;
};

}; // namespace OBS
