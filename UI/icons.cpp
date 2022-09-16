#include "icons.hpp"
#include "obs-frontend-api.h"

#include <QAction>
#include <QAbstractButton>
#include <QDomDocument>
#include <QSvgRenderer>
#include <QBitmap>
#include <QFile>
#include <QPainter>
#include <QDirIterator>
#include <QStyle>
#include <vector>

#define ICON_COLOR_DARK "#fefefe"
#define ICON_COLOR_LIGHT "#222222"
#define ICON_COLOR_DISABLED "#9a9996"

bool iconColorStyled = false;
bool iconColorDisabledStyled = false;
QString iconColor;
QString iconColorDisabled;
std::vector<IconData *> icons;

static void SetAttrRecur(QDomElement elem, const QString &tag,
			 const QString &attr, const QString &val)
{
	if (elem.tagName().compare(tag) == 0)
		elem.setAttribute(attr, val);

	for (int i = 0; i < elem.childNodes().count(); i++) {
		if (!elem.childNodes().at(i).isElement())
			continue;

		SetAttrRecur(elem.childNodes().at(i).toElement(), tag, attr,
			     val);
	}
}

static QPixmap CreatePixmap(QByteArray data, const QString &color)
{
	QDomDocument doc;
	doc.setContent(data);
	SetAttrRecur(doc.documentElement(), "path", "fill", color);
	SetAttrRecur(doc.documentElement(), "svg", "fill", color);

	QSvgRenderer svgRenderer(doc.toByteArray());

	QPixmap pix(svgRenderer.defaultSize());
	pix.fill(Qt::transparent);

	QPainter pixPainter(&pix);
	svgRenderer.render(&pixPainter);

	return pix;
}

static void CreateIcon(const QString &path)
{
	bool colored = IconColorStyled();

	QString color;

	if (colored)
		color = iconColor;
	else
		color = obs_frontend_is_theme_dark() ? ICON_COLOR_DARK
						     : ICON_COLOR_LIGHT;

	QString disabledColor = colored ? iconColorDisabled
					: ICON_COLOR_DISABLED;

	QFile file(path);
	file.open(QIODevice::ReadOnly);
	QByteArray baData = file.readAll();

	QPixmap pix = CreatePixmap(baData, color);
	QPixmap pixDisabled = CreatePixmap(baData, disabledColor);

	QIcon icon(pix);
	icon.addPixmap(pixDisabled, QIcon::Disabled);

	IconData *data = new IconData();
	data->icon = icon;
	data->path = path;

	icons.emplace_back(data);
}

void SetIconColorStyled(bool style)
{
	iconColorStyled = style;
}

void SetIconDisabledColorStyled(bool style)
{
	iconColorDisabledStyled = style;
}

void SetIconColorInternal(const QString &color)
{
	iconColor = color;
}

void SetIconDisabledColorInternal(const QString &color)
{
	iconColorDisabled = color;
}

bool IconColorStyled()
{
	return iconColorStyled && iconColorDisabledStyled;
}

QIcon GetIcon(const QString &path, enum IconType type)
{
	if (!IconColorStyled())
		return QIcon();

	QIcon icon;

	if (type != IconType::Original) {
		for (IconData *data : icons) {
			if (data->path == path) {
				icon = data->icon;
				break;
			}
		}
	} else {
		icon = QIcon(path);
	}

	if (icon.isNull())
		return QIcon();

	if (type != IconType::Disabled)
		return icon;

	QPixmap pixmap = icon.pixmap(256, QIcon::Disabled);
	return QIcon(pixmap);
}

void ClearIcons()
{
	for (IconData *data : icons) {
		delete data;
	}

	icons.clear();
}

void LoadIcons()
{
	ClearIcons();

	if (!IconColorStyled())
		return;

	QDirIterator it(":", QDirIterator::Subdirectories);

	while (it.hasNext()) {
		QString path = it.next();

		if (path.contains(".svg"))
			CreateIcon(path);
	}
}

void SetIcon(QAbstractButton *button, const QString &iconPath,
	     const QString &themeID)
{
	if (!button)
		return;

	if (IconColorStyled()) {
		button->setIcon(GetIcon(iconPath));
	} else {
		if (!themeID.isEmpty())
			button->setProperty("themeID", themeID);

		button->style()->unpolish(button);
		button->style()->polish(button);
	}
}

void SetIcon(QAction *action, const QString &iconPath)
{
	if (!action)
		return;

	if (IconColorStyled())
		action->setIcon(GetIcon(iconPath));
}
