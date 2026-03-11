#include "OBSBasicSettings.hpp"

#include <OBSApp.hpp>
#include <utility/platform.hpp>
#include <utility/OBSThemeWatcher.hpp>

#include <qt-wrappers.hpp>

void OBSBasicSettings::InitAppearancePage()
{
	savedTheme = App()->GetTheme();
	const QString currentBaseTheme = savedTheme->isBaseTheme ? savedTheme->id : savedTheme->parent;

	for (const OBSTheme &theme : App()->GetThemes()) {
		if (theme.isBaseTheme && (HighContrastEnabled() || theme.isVisible || theme.id == currentBaseTheme)) {
			ui->theme->addItem(theme.name, theme.id);
		}
	}

	int idx = ui->theme->findData(currentBaseTheme);
	if (idx != -1)
		ui->theme->setCurrentIndex(idx);

	ui->themeVariant->setPlaceholderText(QTStr("Basic.Settings.Appearance.General.NoVariant"));
	ui->themeVariantLight->setPlaceholderText(QTStr("Basic.Settings.Appearance.General.NoVariant"));
	ui->themeVariantDark->setPlaceholderText(QTStr("Basic.Settings.Appearance.General.NoVariant"));

	ui->appearanceFontScale->setDisplayTicks(true);

	connect(ui->autoVariant, &QCheckBox::checkStateChanged, this, [&] {
		enableAppearanceAutoThemeControls(ui->autoVariant->isChecked());
		this->SaveAppearanceSettings();
	});

	connect(ui->appearanceFontScale, &QSlider::valueChanged, ui->appearanceFontScaleText,
		[this](int value) { ui->appearanceFontScaleText->setText(QString::number(value)); });
	ui->appearanceFontScaleText->setText(QString::number(ui->appearanceFontScale->value()));

	connect(App(), &OBSApp::StyleChanged, this, &OBSBasicSettings::updateAppearanceControls);
	updateAppearanceControls();
}

void OBSBasicSettings::LoadThemeList(bool reload)
{
	ProfileScope("OBSBasicSettings::LoadThemeList");

	const OBSTheme *currentTheme = App()->GetTheme();
	const QString currentBaseTheme = currentTheme->isBaseTheme ? currentTheme->id : currentTheme->parent;

	/* Nothing to do if current and last base theme were the same */
	const QString baseThemeId = ui->theme->currentData().toString();
	if (reload && baseThemeId == currentBaseTheme)
		return;

	ui->themeVariant->blockSignals(true);
	ui->themeVariant->clear();

	ui->themeVariantLight->blockSignals(true);
	ui->themeVariantLight->clear();

	ui->themeVariantDark->blockSignals(true);
	ui->themeVariantDark->clear();

	auto themes = App()->GetThemes();
	std::sort(themes.begin(), themes.end(), [](const OBSTheme &a, const OBSTheme &b) -> bool {
		return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
	});

	QString defaultVariant;
	const OBSTheme *baseTheme = App()->GetTheme(baseThemeId);

	for (const OBSTheme &theme : themes) {
		/* Skip non-visible themes */
		if (!theme.isVisible || theme.isHighContrast)
			continue;
		/* Skip non-child themes */
		if (theme.isBaseTheme || theme.parent != baseThemeId)
			continue;

		ui->themeVariant->addItem(theme.name, theme.id);
		ui->themeVariantLight->addItem(theme.name, theme.id);
		ui->themeVariantDark->addItem(theme.name, theme.id);

		if (baseTheme && theme.filename == baseTheme->filename)
			defaultVariant = theme.id;
	}

	config_t *config = App()->GetUserConfig();
	auto themeID = config_get_string(config, "Appearance", "Theme");
	int idx = ui->themeVariant->findData(themeID);
	if (idx != -1)
		ui->themeVariant->setCurrentIndex(idx);

	auto themeLightID = config_get_string(config, "Appearance", "ThemeLight");
	int lightIdx = ui->themeVariantLight->findData(themeLightID);
	if (lightIdx != -1)
		ui->themeVariantLight->setCurrentIndex(lightIdx);

	auto themeDarkID = config_get_string(config, "Appearance", "ThemeDark");
	int darkIdx = ui->themeVariantDark->findData(themeDarkID);
	if (darkIdx != -1)
		ui->themeVariantDark->setCurrentIndex(darkIdx);

	ui->themeVariant->setEnabled(ui->themeVariant->count() > 0);
	ui->themeVariant->blockSignals(false);

	ui->themeVariantLight->setEnabled(ui->themeVariantLight->count() > 0);
	ui->themeVariantLight->blockSignals(false);

	ui->themeVariantDark->setEnabled(ui->themeVariantDark->count() > 0);
	ui->themeVariantDark->blockSignals(false);

	/* If no variant is selected but variants are available set the first one. */
	if (idx == -1 && ui->themeVariant->count() > 0) {
		idx = ui->themeVariant->findData(defaultVariant);
		ui->themeVariant->setCurrentIndex(idx != -1 ? idx : 0);
	}

	if (lightIdx == -1 && ui->themeVariantLight->count() > 0) {
		idx = ui->themeVariantLight->findData(defaultVariant);
		ui->themeVariantLight->setCurrentIndex(idx != -1 ? idx : 0);
	}

	if (darkIdx == -1 && ui->themeVariantDark->count() > 0) {
		idx = ui->themeVariantDark->findData(defaultVariant);
		ui->themeVariantDark->setCurrentIndex(idx != -1 ? idx : 0);
	}
}

void OBSBasicSettings::LoadAppearanceSettings(bool reload)
{
	loading = true;

	LoadThemeList(reload);

	if (reload) {
		QString themeId = ui->theme->currentData().toString();
		if (ui->themeVariant->currentIndex() != -1)
			themeId = ui->themeVariant->currentData().toString();

		App()->SetTheme(themeId);
	}

	bool autoTheme = config_get_bool(App()->GetUserConfig(), "Appearance", "AutoTheme");
	ui->autoVariant->setChecked(autoTheme);

	int fontScale = config_get_int(App()->GetUserConfig(), "Appearance", "FontScale");
	ui->appearanceFontScale->setValue(fontScale);

	int densityId = config_get_int(App()->GetUserConfig(), "Appearance", "Density");
	QAbstractButton *densityButton = ui->appearanceDensityButtonGroup->button(densityId);
	if (densityButton) {
		densityButton->setChecked(true);
	}
	updateAppearanceControls();

	loading = false;
}

void OBSBasicSettings::SaveAppearanceSettings()
{
	config_t *config = App()->GetUserConfig();

	auto autoVariant = ui->autoVariant->isChecked();
	config_set_bool(config, "Appearance", "AutoTheme", autoVariant);

	QString themeId = ui->theme->currentData().toString();
	if (ui->themeVariant->currentIndex() != -1)
		themeId = ui->themeVariant->currentData().toString();

	if (autoVariant) {
		QString themeIdLight = ui->theme->currentData().toString();
		if (ui->themeVariantLight->currentIndex() != -1)
			themeIdLight = ui->themeVariantLight->currentData().toString();

		QString themeIdDark = ui->theme->currentData().toString();
		if (ui->themeVariantDark->currentIndex() != -1)
			themeIdDark = ui->themeVariantDark->currentData().toString();

		config_set_string(config, "Appearance", "ThemeLight", QT_TO_UTF8(themeIdLight));
		config_set_string(config, "Appearance", "ThemeDark", QT_TO_UTF8(themeIdDark));

		themeId = themeIdDark;
		if (ThemeWatcher::systemTheme() == ThemeWatcher::Theme::Light) {
			themeId = themeIdLight;
		}
	} else {
		config_set_string(config, "Appearance", "Theme", QT_TO_UTF8(themeId));
	}

	config_set_int(config, "Appearance", "FontScale", ui->appearanceFontScale->value());

	int densityId = ui->appearanceDensityButtonGroup->checkedId();
	config_set_int(config, "Appearance", "Density", densityId);

	if (App()->GetTheme()->id != themeId) {
		App()->SetTheme(themeId);
	}
}

void OBSBasicSettings::on_theme_activated(int)
{
	LoadAppearanceSettings(true);
}

void OBSBasicSettings::on_themeVariant_activated(int)
{
	LoadAppearanceSettings(true);
}

void OBSBasicSettings::updateAppearanceControls()
{
	OBSTheme *theme = App()->GetTheme();
	enableAppearanceAutoThemeControls(ui->autoVariant->isChecked());
	enableAppearanceFontControls(theme->usesFontScale);
	enableAppearanceDensityControls(theme->usesDensity);
	if (!theme->usesFontScale || !theme->usesDensity) {
		ui->appearanceOptionsWarning->setVisible(true);
	} else {
		ui->appearanceOptionsWarning->setVisible(false);
	}
	style()->polish(ui->appearanceOptionsWarningLabel);
}

void OBSBasicSettings::enableAppearanceAutoThemeControls(bool enable)
{
	ui->themeVariant->setVisible(!enable);
	ui->themeVariantLabel->setVisible(!enable);

	ui->themeVariantLight->setVisible(enable);
	ui->themeVariantLabelLight->setVisible(enable);

	ui->themeVariantDark->setVisible(enable);
	ui->themeVariantLabelDark->setVisible(enable);
}

void OBSBasicSettings::enableAppearanceFontControls(bool enable)
{
	ui->appearanceFontScale->setEnabled(enable);
	ui->appearanceFontScaleText->setEnabled(enable);
}

void OBSBasicSettings::enableAppearanceDensityControls(bool enable)
{
	const QList<QAbstractButton *> buttons = ui->appearanceDensityButtonGroup->buttons();
	for (QAbstractButton *button : buttons) {
		button->setEnabled(enable);
	}
}
