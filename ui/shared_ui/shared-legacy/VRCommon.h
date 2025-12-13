#pragma once

#include <QString>
#include <QColor>
#include "ThemeManager.h"

namespace VRTheme {
    inline QString color(const QString &key)
    {
        return ThemeManager::instance().color(key).name();
    }

    inline QString buttonStyle()
    {
        return QString("QPushButton { "
                       "background-color: %1; "
                       "color: %2; "
                       "border: 1px solid %3; "
                       "border-radius: %4px; "
                       "padding: %5px; "
                       "} "
                       "QPushButton:hover { background-color: #3E3E42; } "  // TODO: Add hover to JSON
                       "QPushButton:pressed { background-color: %6; }")
            .arg(color("bg_light"), color("text"), color("border"))
            .arg(ThemeManager::instance().size("border_radius"))
            .arg(ThemeManager::instance().size("padding"))
            .arg(color("primary"));
    }

    inline QString sliderStyle()
    {
        return QString("QSlider::groove:horizontal { "
                       "border: 1px solid %1; "
                       "height: 8px; "
                       "background: %2; "
                       "margin: 2px 0; "
                       "border-radius: %3px; "
                       "} "
                       "QSlider::handle:horizontal { "
                       "background: %4; "
                       "border: 1px solid %1; "
                       "width: 18px; "
                       "height: 18px; "
                       "margin: -7px 0; "
                       "border-radius: 9px; "
                       "}")
            .arg(color("border"), color("bg_light"))
            .arg(ThemeManager::instance().size("border_radius"))
            .arg(color("primary"));
    }

    // Legacy/Compat constants mapping to TM
    const QString ACCENT_COLOR = color("accent");
    const QString BG_HIGH = color("bg_light");
}  // namespace VRTheme
// namespace VRTheme
