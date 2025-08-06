#include "NotificationBell.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QApplication>

#include <qt-wrappers.hpp>

#include "moc_NotificationBell.cpp"

constexpr int iconSize = 20;
constexpr int counterHeight = 14;
constexpr int importantIconSize = 12;

NotificationBell::NotificationBell(QWidget *parent) : QFrame(parent)
{
	setCount(0, 0);
}

void NotificationBell::setCount(int count_, int importantCount_)
{
	count = count_;
	importantCount = importantCount_;

	setClasses(this, count ? "icon-bell-active" : "icon-bell-inactive");

	if (count <= 9)
		text = QString::number(count);
	else
		text = "9+";

	update();
}

void NotificationBell::paintEvent(QPaintEvent *)
{
	QPainter painter(this);

	QRect iconRect(0, (height() - iconSize) / 2, iconSize, iconSize);
	icon.paint(&painter, iconRect, Qt::AlignCenter, QIcon::Normal);

	if (!count)
		return;

	painter.setBrush(QBrush(countColor));
	painter.setPen(Qt::NoPen);

	QFont font = QApplication::font();
	font.setBold(true);

	QFontMetrics fm(font);
	QRectF textRect = fm.boundingRect(text);

	float radius = counterHeight / 2.0f;
	float width = counterHeight;

	if (textRect.width() + radius > width)
		width = textRect.width() + radius + 1;

	QRectF rect((iconSize / 2.0f), 0.0f, width, counterHeight);

	QPainterPath path;
	path.addRoundedRect(rect, radius, radius);

	painter.fillPath(path, painter.brush());

	rect.setX(rect.x() + 1.0f);
	rect.setY(rect.y() - 1.0f);

	QPen pen = QApplication::palette().text().color();

	painter.setFont(font);
	painter.setPen(pen);
	painter.drawText(rect, Qt::AlignCenter, text);

	if (!importantCount)
		return;

	QIcon importantIcon(":/res/images/important.svg");
	QRect importantRect((iconSize / 2) + (counterHeight - importantIconSize), height() - importantIconSize,
			    importantIconSize, importantIconSize);

	importantIcon.paint(&painter, importantRect, Qt::AlignCenter, QIcon::Normal);
}

void NotificationBell::mousePressEvent(QMouseEvent *event)
{
	emit clicked();
	QWidget::mousePressEvent(event);
}
