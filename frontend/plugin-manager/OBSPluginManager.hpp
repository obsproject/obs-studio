#pragma once
#include "ui_OBSPluginManager.h"

#include <QDialog>
#include <QWidget>
#include <obs-module.h>

#include "../widgets/OBSBasic.hpp"

class OBSPluginManager : public QDialog {
	Q_OBJECT
	std::unique_ptr<Ui::OBSPluginManager> ui;

public:
	explicit OBSPluginManager(std::vector<OBSModuleInfo> const &modules, QWidget *parent = nullptr);
	inline std::vector<OBSModuleInfo> const result() { return _modules; }

private:
	std::vector<OBSModuleInfo> _modules;
};
