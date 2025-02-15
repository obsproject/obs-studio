#include "ImageSourceToolbar.hpp"
#include "ui_image-source-toolbar.h"

#include <qt-wrappers.hpp>

#include "moc_ImageSourceToolbar.cpp"

ImageSourceToolbar::ImageSourceToolbar(QWidget *parent, OBSSource source)
	: SourceToolbar(parent, source),
	  ui(new Ui_ImageSourceToolbar)
{
	ui->setupUi(this);

	obs_module_t *mod = obs_get_module("image-source");
	ui->pathLabel->setText(obs_module_get_locale_text(mod, "File"));

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	std::string file = obs_data_get_string(settings, "file");

	ui->path->setText(file.c_str());
}

ImageSourceToolbar::~ImageSourceToolbar() {}

void ImageSourceToolbar::on_browse_clicked()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	obs_property_t *p = obs_properties_get(props.get(), "file");
	const char *desc = obs_property_description(p);
	const char *filter = obs_property_path_filter(p);
	const char *default_path = obs_property_path_default_path(p);

	QString startDir = ui->path->text();
	if (startDir.isEmpty())
		startDir = default_path;

	QString path = OpenFile(this, desc, startDir, filter);
	if (path.isEmpty()) {
		return;
	}

	ui->path->setText(path);

	SaveOldProperties(source);
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "file", QT_TO_UTF8(path));
	obs_source_update(source, settings);
	SetUndoProperties(source);
}
