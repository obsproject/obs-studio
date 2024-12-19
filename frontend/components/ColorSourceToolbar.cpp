#include "ColorSourceToolbar.hpp"
#include "ui_color-source-toolbar.h"

#include <QColorDialog>

#include "moc_ColorSourceToolbar.cpp"

QColor color_from_int(long long val)
{
	return QColor(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff, (val >> 24) & 0xff);
}

long long color_to_int(QColor color)
{
	auto shift = [&](unsigned val, int shift) {
		return ((val & 0xff) << shift);
	};

	return shift(color.red(), 0) | shift(color.green(), 8) | shift(color.blue(), 16) | shift(color.alpha(), 24);
}

ColorSourceToolbar::ColorSourceToolbar(QWidget *parent, OBSSource source)
	: SourceToolbar(parent, source),
	  ui(new Ui_ColorSourceToolbar)
{
	ui->setupUi(this);

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	unsigned int val = (unsigned int)obs_data_get_int(settings, "color");

	color = color_from_int(val);
	UpdateColor();
}

ColorSourceToolbar::~ColorSourceToolbar() {}

void ColorSourceToolbar::UpdateColor()
{
	QPalette palette = QPalette(color);
	ui->color->setFrameStyle(QFrame::Sunken | QFrame::Panel);
	ui->color->setText(color.name(QColor::HexRgb));
	ui->color->setPalette(palette);
	ui->color->setStyleSheet(QString("background-color :%1; color: %2;")
					 .arg(palette.color(QPalette::Window).name(QColor::HexRgb))
					 .arg(palette.color(QPalette::WindowText).name(QColor::HexRgb)));
	ui->color->setAutoFillBackground(true);
	ui->color->setAlignment(Qt::AlignCenter);
}

void ColorSourceToolbar::on_choose_clicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t *p = obs_properties_get(props.get(), "color");
	const char *desc = obs_property_description(p);

	QColorDialog::ColorDialogOptions options;

	options |= QColorDialog::ShowAlphaChannel;
#ifdef __linux__
	// TODO: Revisit hang on Ubuntu with native dialog
	options |= QColorDialog::DontUseNativeDialog;
#endif

	QColor newColor = QColorDialog::getColor(color, this, desc, options);
	if (!newColor.isValid()) {
		return;
	}

	color = newColor;
	UpdateColor();

	SaveOldProperties(source);

	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_int(settings, "color", color_to_int(color));
	obs_source_update(source, settings);

	SetUndoProperties(source);
}
