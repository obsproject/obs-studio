#pragma once
#include <QString>
#include <QColor>
#include <QFont>
#include <QJsonObject>
#include <QFile>
#include <QJsonDocument>

class ThemeManager
{
      public:
    static ThemeManager &instance();

    void loadTheme(const QString &path);

    QColor color(const QString &key) const;
    QFont font(const QString &key) const;
    int size(const QString &key) const;
    QString stylesheet(const QString &key) const;

      private:
    ThemeManager();
    QJsonObject m_root;
    QJsonObject m_colors;
    QJsonObject m_fonts;
    QJsonObject m_sizes;
};
