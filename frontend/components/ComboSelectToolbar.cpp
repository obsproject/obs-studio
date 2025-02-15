#include "ComboSelectToolbar.hpp"
#include "ui_device-select-toolbar.h"

#include <OBSApp.hpp>
#include <qt-wrappers.hpp>

#include "moc_ComboSelectToolbar.cpp"

ComboSelectToolbar::ComboSelectToolbar(QWidget *parent, OBSSource source)
	: SourceToolbar(parent, source),
	  ui(new Ui_DeviceSelectToolbar)
{
	ui->setupUi(this);
}

ComboSelectToolbar::~ComboSelectToolbar() {}

int FillPropertyCombo(QComboBox *c, obs_property_t *p, const std::string &cur_id, bool is_int = false)
{
	size_t count = obs_property_list_item_count(p);
	int cur_idx = -1;

	for (size_t i = 0; i < count; i++) {
		const char *name = obs_property_list_item_name(p, i);
		std::string id;

		if (is_int) {
			id = std::to_string(obs_property_list_item_int(p, i));
		} else {
			const char *val = obs_property_list_item_string(p, i);
			id = val ? val : "";
		}

		if (cur_id == id)
			cur_idx = (int)i;

		c->addItem(name, id.c_str());
	}

	return cur_idx;
}

void UpdateSourceComboToolbarProperties(QComboBox *combo, OBSSource source, obs_properties_t *props,
					const char *prop_name, bool is_int)
{
	std::string cur_id;

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	if (is_int) {
		cur_id = std::to_string(obs_data_get_int(settings, prop_name));
	} else {
		cur_id = obs_data_get_string(settings, prop_name);
	}

	combo->blockSignals(true);

	obs_property_t *p = obs_properties_get(props, prop_name);
	int cur_idx = FillPropertyCombo(combo, p, cur_id, is_int);

	if (cur_idx == -1 || obs_property_list_item_disabled(p, cur_idx)) {
		if (cur_idx == -1) {
			combo->insertItem(0, QTStr("Basic.Settings.Audio.UnknownAudioDevice"));
			cur_idx = 0;
		}

		SetComboItemEnabled(combo, cur_idx, false);
	}

	combo->setCurrentIndex(cur_idx);
	combo->blockSignals(false);
}

void ComboSelectToolbar::Init()
{
	OBSSource source = GetSource();
	if (!source) {
		return;
	}

	UpdateSourceComboToolbarProperties(ui->device, source, props.get(), prop_name, is_int);
}

void UpdateSourceComboToolbarValue(QComboBox *combo, OBSSource source, int idx, const char *prop_name, bool is_int)
{
	QString id = combo->itemData(idx).toString();

	OBSDataAutoRelease settings = obs_data_create();
	if (is_int) {
		obs_data_set_int(settings, prop_name, id.toInt());
	} else {
		obs_data_set_string(settings, prop_name, QT_TO_UTF8(id));
	}
	obs_source_update(source, settings);
}

void ComboSelectToolbar::on_device_currentIndexChanged(int idx)
{
	OBSSource source = GetSource();
	if (idx == -1 || !source) {
		return;
	}

	SaveOldProperties(source);
	UpdateSourceComboToolbarValue(ui->device, source, idx, prop_name, is_int);
	SetUndoProperties(source);
}
