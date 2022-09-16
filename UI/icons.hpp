#pragma once

#include <QIcon>
#include <QString>

class QAction;
class QAbstractButton;

struct IconData {
	QIcon icon;
	QString path;
};

enum class IconType {
	Normal,
	Disabled,
	Original,
};

void SetIconColorStyled(bool style);
void SetIconDisabledColorStyled(bool style);
bool IconColorStyled();

void SetIconColorInternal(const QString &color);
void SetIconDisabledColorInternal(const QString &color);

QIcon GetIcon(const QString &path, enum IconType type = IconType::Normal);

void ClearIcons();
void LoadIcons();

void SetIcon(QAbstractButton *button, const QString &iconPath,
	     const QString &themeID = "");
void SetIcon(QAction *action, const QString &iconPath);
