#include "obs-app.hpp"
#include "qt-wrappers.hpp"
#include "window-projector-custom-size-setting.hpp"
#include "window-basic-settings.hpp"

OBSProjectorCustomSizeSetting::OBSProjectorCustomSizeSetting(
	OBSProjector *parent)
	: QDialog(parent),
	  ui(new Ui::OBSProjectorCustomSizeSetting)
{
	ui->setupUi(this);
	this->InitDialog(parent);
	this->setModal(true);
}

void OBSProjectorCustomSizeSetting::InitDialog(OBSProjector *parent)
{
	// Resize method
	ui->resizeMethodComboBox->addItem(
		QTStr("ResizeProjectorWindowCustomResolution"));
	ui->resizeMethodComboBox->addItem(
		QTStr("ResizeProjectorWindowCustomScale"));

	// Resolution input, use the same input format in settings
	std::vector<std::pair<int, int>> resolutionPresets =
		parent->GetResizeResolutionPresets();

	for (size_t i = 0; i < resolutionPresets.size(); i++) {
		ui->resolutionComboBox->addItem(
			QString("%1x%2")
				.arg(resolutionPresets[i].first)
				.arg(resolutionPresets[i].second));
	}
	QRegularExpression rx("\\d{1,5}x\\d{1,5}");
	QValidator *validator = new QRegularExpressionValidator(rx, this);
	ui->resolutionComboBox->lineEdit()->setValidator(validator);

	// Show resolution input on init
	this->ChangeMethod(0);

	connect(ui->resizeMethodComboBox, &QComboBox::currentIndexChanged, this,
		&OBSProjectorCustomSizeSetting::ChangeMethod);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
		&OBSProjectorCustomSizeSetting::ConfirmAndClose);
}

void OBSProjectorCustomSizeSetting::ChangeMethod(int index)
{
	ui->stackedWidget->setCurrentIndex(index);
}

void OBSProjectorCustomSizeSetting::ConfirmAndClose()
{
	if (ui->resizeMethodComboBox->currentIndex() == 0) {
		uint32_t width;
		uint32_t height;
		QString resolution = ui->resolutionComboBox->lineEdit()->text();
		ConvertResText(QT_TO_UTF8(resolution), width, height);
		emit ApplyResolution(int(width), int(height));
	} else {
		emit ApplyScale(ui->scaleSpinBox->value());
	}
	emit accept();
}
