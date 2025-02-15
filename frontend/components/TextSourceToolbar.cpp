#include "TextSourceToolbar.hpp"
#include "ui_text-source-toolbar.h"

#include <OBSApp.hpp>

#include <qt-wrappers.hpp>

#include <QColorDialog>
#include <QFontDialog>

#include "moc_TextSourceToolbar.cpp"

extern void MakeQFont(obs_data_t *font_obj, QFont &font, bool limit = false);
extern QColor color_from_int(long long val);
extern long long color_to_int(QColor color);

TextSourceToolbar::TextSourceToolbar(QWidget *parent, OBSSource source)
	: SourceToolbar(parent, source),
	  ui(new Ui_TextSourceToolbar)
{
	ui->setupUi(this);

	OBSDataAutoRelease settings = obs_source_get_settings(source);

	const char *id = obs_source_get_unversioned_id(source);
	bool ft2 = strcmp(id, "text_ft2_source") == 0;
	bool read_from_file = obs_data_get_bool(settings, ft2 ? "from_file" : "read_from_file");

	OBSDataAutoRelease font_obj = obs_data_get_obj(settings, "font");
	MakeQFont(font_obj, font);

	// Use "color1" if it's a freetype source and "color" elsewise
	unsigned int val = (unsigned int)obs_data_get_int(
		settings, (strncmp(obs_source_get_id(source), "text_ft2_source", 15) == 0) ? "color1" : "color");

	color = color_from_int(val);

	const char *text = obs_data_get_string(settings, "text");

	bool single_line = !read_from_file && (!text || (strchr(text, '\n') == nullptr));
	ui->emptySpace->setVisible(!single_line);
	ui->text->setVisible(single_line);
	if (single_line)
		ui->text->setText(text);
}

TextSourceToolbar::~TextSourceToolbar() {}

void TextSourceToolbar::on_selectFont_clicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	QFontDialog::FontDialogOptions options;
	uint32_t flags;
	bool success;

#ifndef _WIN32
	options = QFontDialog::DontUseNativeDialog;
#endif

	font = QFontDialog::getFont(&success, font, this, QTStr("Basic.PropertiesWindow.SelectFont.WindowTitle"),
				    options);
	if (!success) {
		return;
	}

	OBSDataAutoRelease font_obj = obs_data_create();

	obs_data_set_string(font_obj, "face", QT_TO_UTF8(font.family()));
	obs_data_set_string(font_obj, "style", QT_TO_UTF8(font.styleName()));
	obs_data_set_int(font_obj, "size", font.pointSize());
	flags = font.bold() ? OBS_FONT_BOLD : 0;
	flags |= font.italic() ? OBS_FONT_ITALIC : 0;
	flags |= font.underline() ? OBS_FONT_UNDERLINE : 0;
	flags |= font.strikeOut() ? OBS_FONT_STRIKEOUT : 0;
	obs_data_set_int(font_obj, "flags", flags);

	SaveOldProperties(source);

	OBSDataAutoRelease settings = obs_data_create();

	obs_data_set_obj(settings, "font", font_obj);

	obs_source_update(source, settings);

	SetUndoProperties(source);
}

void TextSourceToolbar::on_selectColor_clicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	bool freetype = strncmp(obs_source_get_id(source), "text_ft2_source", 15) == 0;

	obs_property_t *p = obs_properties_get(props.get(), freetype ? "color1" : "color");

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

	SaveOldProperties(source);

	OBSDataAutoRelease settings = obs_data_create();
	if (freetype) {
		obs_data_set_int(settings, "color1", color_to_int(color));
		obs_data_set_int(settings, "color2", color_to_int(color));
	} else {
		obs_data_set_int(settings, "color", color_to_int(color));
	}
	obs_source_update(source, settings);

	SetUndoProperties(source);
}

void TextSourceToolbar::on_text_textChanged()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}
	std::string newText = QT_TO_UTF8(ui->text->text());
	OBSDataAutoRelease settings = obs_source_get_settings(source);
	if (newText == obs_data_get_string(settings, "text")) {
		return;
	}
	SaveOldProperties(source);

	obs_data_set_string(settings, "text", newText.c_str());
	obs_source_update(source, nullptr);

	SetUndoProperties(source, true);
}
