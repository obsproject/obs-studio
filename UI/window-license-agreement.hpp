#pragma once

#include <memory>
#include "ui_OBSLicenseAgreement.h"

class OBSLicenseAgreement : public QDialog {
	Q_OBJECT

private:
	std::unique_ptr<Ui::OBSLicenseAgreement> ui;

public:
	OBSLicenseAgreement(QWidget *parent);
};
