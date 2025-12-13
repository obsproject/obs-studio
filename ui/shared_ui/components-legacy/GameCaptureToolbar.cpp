#include "GameCaptureToolbar.hpp"
#include "ui_game-capture-toolbar.h"

#include <qt-wrappers.hpp>

#include "moc_GameCaptureToolbar.cpp"

extern int FillPropertyCombo(QComboBox *c, obs_property_t *p, const std::string &cur_id, bool is_int = false);

GameCaptureToolbar::GameCaptureToolbar(QWidget *parent, OBSSource source)
	: SourceToolbar(parent, source),
	  ui(new Ui_GameCaptureToolbar)
{
	obs_property_t *p;
	int cur_idx;

	ui->setupUi(this);

	obs_module_t *mod = obs_get_module("win-capture");
	if (!mod)
		return;

	ui->modeLabel->setText(obs_module_get_locale_text(mod, "Mode"));
	ui->windowLabel->setText(obs_module_get_locale_text(mod, "WindowCapture.Window"));

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	std::string cur_mode = obs_data_get_string(settings, "capture_mode");
	std::string cur_window = obs_data_get_string(settings, "window");

	ui->mode->blockSignals(true);
	p = obs_properties_get(props.get(), "capture_mode");
	cur_idx = FillPropertyCombo(ui->mode, p, cur_mode);
	ui->mode->setCurrentIndex(cur_idx);
	ui->mode->blockSignals(false);

	ui->window->blockSignals(true);
	p = obs_properties_get(props.get(), "window");
	cur_idx = FillPropertyCombo(ui->window, p, cur_window);
	ui->window->setCurrentIndex(cur_idx);
	ui->window->blockSignals(false);

	if (cur_idx != -1 && obs_property_list_item_disabled(p, cur_idx)) {
		SetComboItemEnabled(ui->window, cur_idx, false);
	}

	UpdateWindowVisibility();
}

GameCaptureToolbar::~GameCaptureToolbar() {}

void GameCaptureToolbar::UpdateWindowVisibility()
{
	QString mode = ui->mode->currentData().toString();
	bool is_window = (mode == "window");
	ui->windowLabel->setVisible(is_window);
	ui->window->setVisible(is_window);
}

void GameCaptureToolbar::on_mode_currentIndexChanged(int idx)
{
	OBSSource source = GetSource();
	if (idx == -1 || !source) {
		return;
	}

	QString id = ui->mode->itemData(idx).toString();

	SaveOldProperties(source);
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "capture_mode", QT_TO_UTF8(id));
	obs_source_update(source, settings);
	SetUndoProperties(source);

	UpdateWindowVisibility();
}

void GameCaptureToolbar::on_window_currentIndexChanged(int idx)
{
	OBSSource source = GetSource();
	if (idx == -1 || !source) {
		return;
	}

	QString id = ui->window->itemData(idx).toString();

	SaveOldProperties(source);
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_string(settings, "window", QT_TO_UTF8(id));
	obs_source_update(source, settings);
	SetUndoProperties(source);
}
