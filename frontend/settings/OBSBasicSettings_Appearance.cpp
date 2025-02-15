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
	LoadThemeList(reload);

	if (reload) {
		QString themeId = ui->theme->currentData().toString();
		if (ui->themeVariant->currentIndex() != -1)
			themeId = ui->themeVariant->currentData().toString();

		App()->SetTheme(themeId);
	}
}

void OBSBasicSettings::SaveAppearanceSettings()
{
	config_t *config = App()->GetUserConfig();

	OBSTheme *currentTheme = App()->GetTheme();
	if (savedTheme != currentTheme) {
		config_set_string(config, "Appearance", "Theme", QT_TO_UTF8(currentTheme->id));
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
