#include "VerticalLabel.hpp"
#include <QStylePainter>

#include "moc_VerticalLabel.cpp"

VerticalLabel::VerticalLabel(const QString &text, QWidget *parent) : QLabel(text, parent) {}

void VerticalLabel::ShowNormal()
{
	vertical = false;
}

void VerticalLabel::ShowVertical()
{
	vertical = true;
}

void VerticalLabel::paintEvent(QPaintEvent *event)
{
	if (!vertical) {
		QLabel::paintEvent(event);
		return;
	}

	QStylePainter painter(this);
	painter.translate(0, height());
	painter.rotate(270);
	painter.drawText(QRect(QPoint(0, 0), QLabel::sizeHint()), alignment(), text());
}

QSize VerticalLabel::minimumSizeHint() const
{
	QSize s = QLabel::minimumSizeHint();
	return QSize(vertical ? s.height() : s.width(), vertical ? s.width() : s.height());
}

QSize VerticalLabel::sizeHint() const
{
	QSize s = QLabel::sizeHint();
	return QSize(vertical ? s.height() : s.width(), vertical ? s.width() : s.height());
}
