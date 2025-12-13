#include "OBSBasicSettings.hpp"

#include <OBSApp.hpp>
#include <utility/platform.hpp>

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

	ui->appearanceFontScale->setDisplayTicks(true);

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
		if (baseTheme && theme.filename == baseTheme->filename)
			defaultVariant = theme.id;
	}

	int idx = ui->themeVariant->findData(currentTheme->id);
	if (idx != -1)
		ui->themeVariant->setCurrentIndex(idx);

	ui->themeVariant->setEnabled(ui->themeVariant->count() > 0);
	ui->themeVariant->blockSignals(false);
	/* If no variant is selected but variants are available set the first one. */
	if (idx == -1 && ui->themeVariant->count() > 0) {
		idx = ui->themeVariant->findData(defaultVariant);
		ui->themeVariant->setCurrentIndex(idx != -1 ? idx : 0);
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

	OBSTheme *currentTheme = App()->GetTheme();
	if (savedTheme != currentTheme) {
		config_set_string(config, "Appearance", "Theme", QT_TO_UTF8(currentTheme->id));
	}

	config_set_int(config, "Appearance", "FontScale", ui->appearanceFontScale->value());

	int densityId = ui->appearanceDensityButtonGroup->checkedId();
	config_set_int(config, "Appearance", "Density", densityId);

	App()->SetTheme(currentTheme->id);
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
	enableAppearanceFontControls(theme->usesFontScale);
	enableAppearanceDensityControls(theme->usesDensity);
	if (!theme->usesFontScale || !theme->usesDensity) {
		ui->appearanceOptionsWarning->setVisible(true);
	} else {
		ui->appearanceOptionsWarning->setVisible(false);
	}
	style()->polish(ui->appearanceOptionsWarningLabel);
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
