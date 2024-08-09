#pragma once

#include <obs.hpp>
#include <QDialog>
#include <window-projector.hpp>
#include "ui_OBSProjectorCustomSizeSetting.h"

class OBSProjectorCustomSizeSetting : public QDialog {
	Q_OBJECT

public:
	OBSProjectorCustomSizeSetting(OBSProjector *parent);

private:
	std::unique_ptr<Ui::OBSProjectorCustomSizeSetting> ui;
	void InitDialog(OBSProjector *parent);

signals:
	void ApplyResolution(int width, int height);
	void ApplyScale(int scale);

private slots:
	void ChangeMethod(int index);
	void ConfirmAndClose();
};
