#include "ThemeManager.h"
#include <QDebug>

ThemeManager &ThemeManager::instance()
{
	static ThemeManager inst;
	return inst;
}

ThemeManager::ThemeManager()
{
	// Load default or hardcoded fallback
	loadTheme("frontend/assets/theme.json");
}

void ThemeManager::loadTheme(const QString &path)
{
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Failed to load theme:" << path;
		return;
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	m_root = doc.object();
	m_colors = m_root["colors"].toObject();
	m_fonts = m_root["fonts"].toObject();
	m_sizes = m_root["sizes"].toObject();
}

QColor ThemeManager::color(const QString &key) const
{
	QString val = m_colors[key].toString();
	if (val.isEmpty())
		return Qt::red; // Error color
	return QColor(val);
}

int ThemeManager::size(const QString &key) const
{
	return m_sizes[key].toInt(0);
}

QFont ThemeManager::font(const QString &key) const
{
	// For now simple sizing logic, ideally we parse family/weight objects
	QFont f;
	if (key == "base")
		f.setPointSize(m_fonts["base_size"].toInt(10));
	else if (key == "header")
		f.setPointSize(m_fonts["header_size"].toInt(12));
	return f;
}
