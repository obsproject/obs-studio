#include "OBSThemeWatcher.hpp"

#include <QGuiApplication>
#include <QStyleHints>

ThemeWatcher::ThemeWatcher(QObject *parent) : QObject(parent)
{
	auto *hints = qApp->styleHints();
	connect(hints, &QStyleHints::colorSchemeChanged, this, &ThemeWatcher::handleScheme);

	handleScheme(hints->colorScheme());
}

void ThemeWatcher::handleScheme(Qt::ColorScheme scheme)
{
	const Theme newTheme = (scheme == Qt::ColorScheme::Light) ? Theme::Light : Theme::Dark;

	if (newTheme != m_currentTheme) {
		m_currentTheme = newTheme;
		emit themeChanged(this);
	}
}

ThemeWatcher::Theme ThemeWatcher::systemTheme()
{
	const auto scheme = qApp->styleHints()->colorScheme();
	return (scheme == Qt::ColorScheme::Light) ? Theme::Light : Theme::Dark;
}
