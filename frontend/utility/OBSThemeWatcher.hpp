#pragma once

#include <QObject>

class ThemeWatcher : public QObject {
	Q_OBJECT
public:
	enum class Theme { Light, Dark };
	Q_ENUM(Theme)

	explicit ThemeWatcher(QObject *parent = nullptr);

	static Theme systemTheme();
	Theme currentTheme() const { return m_currentTheme; }

signals:
	void themeChanged(ThemeWatcher *self);

private:
	void handleScheme(Qt::ColorScheme scheme);

	Theme m_currentTheme = Theme::Light;
};