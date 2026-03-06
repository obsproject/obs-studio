#include "ColorSourceToolbar.hpp"
#include "ui_color-source-toolbar.h"

#include <QColorDialog>
#include <QStyle>

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

namespace {
float getPerceptualLuminance(QColor color)
{
	// sRGB linearization formula
	auto toLinear = [](double v) {
		float value = v / 255.0;
		if (value <= 0.04045) {
			return value / 12.92;
		}

		return std::pow((value + 0.055) / 1.055, 2.4);
	};

	double red = toLinear(color.red());
	double green = toLinear(color.green());
	double blue = toLinear(color.blue());

	// Rec. 709 weights for luma estimate
	return ((red * 0.2125) + (green * 0.7154) + (blue * 0.0721));
}
} // namespace

ColorSourceToolbar::ColorSourceToolbar(QWidget *parent, OBSSource source)
	: SourceToolbar(parent, source),
	  ui(new Ui_ColorSourceToolbar)
{
	ui->setupUi(this);

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	unsigned int val = (unsigned int)obs_data_get_int(settings, "color");

	utils = std::make_unique<idian::Utils>(this);
	utils->applyStateStylingEventFilter(ui->colorPreview);

	color = color_from_int(val);
	UpdateColor();
}

ColorSourceToolbar::~ColorSourceToolbar() {}

void ColorSourceToolbar::UpdateColor()
{
	QPalette palette = QPalette(color);
	ui->colorPreview->setText(color.name(QColor::HexRgb).toUpper());

	float luminance = getPerceptualLuminance(color);

	double contrastWhite = (1.0 + 0.05) / (luminance + 0.05);
	double contrastBlack = (luminance + 0.05) / (0.0 + 0.05);

	bool useLightText = contrastWhite > contrastBlack;

	QColor background;
	QColor text;
	QColor backgroundHover;
	QColor borderHover;
	QColor textHover;

	if (useLightText) {
		background = color;
		text = color.lighter(500);
		text.setHslF(text.hueF(), text.hslSaturationF() * 0.3f, std::max(text.lightnessF(), 0.75f));
		backgroundHover = color.lighter(120);
		borderHover = color.lighter(180);
		borderHover.setHslF(borderHover.hueF(), borderHover.hslSaturationF(),
				    std::max(borderHover.lightnessF(), 0.2f));
		textHover = text.lighter(150);
	} else {
		background = color;
		text = color.darker(500);
		text.setHslF(text.hueF(), text.hslSaturationF() * 0.3f, std::min(text.lightnessF(), 0.2f));
		backgroundHover = color.darker(120);
		borderHover = color.lighter(110);
		textHover = text.darker(300);
	}

	ui->colorPreview->setStyleSheet(QString(R"(
						#colorPreview {
							background-color: %1;
							border-color: %1;
							color: %2;
						}

						#colorPreview:hover,
						#colorPreview.hover {
							background-color: %3;
							border-color: %4;
							color: %5;
						}
					)")
						.arg(background.name(QColor::HexRgb))
						.arg(text.name(QColor::HexRgb))
						.arg(backgroundHover.name(QColor::HexRgb))
						.arg(borderHover.name(QColor::HexRgb))
						.arg(textHover.name(QColor::HexRgb)));
	ui->colorPreview->setAutoFillBackground(true);

	utils->applyColorToIcon(ui->colorPreview);
	idian::Utils::repolish(ui->colorPreview);
}

void ColorSourceToolbar::on_colorPreview_clicked()
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
