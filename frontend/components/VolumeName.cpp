/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "VolumeName.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStylePainter>

#include "moc_VolumeName.cpp"

namespace {
QString getPlainText(const QString &text)
{
	if (Qt::mightBeRichText(text)) {
		QTextDocument doc;
		doc.setHtml(text);

		return doc.toPlainText();
	}

	return text;
}
} // namespace

VolumeName::VolumeName(obs_source_t *source, QWidget *parent)
	: QAbstractButton(parent),
	  indicatorWidth(style()->pixelMetric(QStyle::PM_MenuButtonIndicator, nullptr, this))
{
	renamedSignal = OBSSignal(obs_source_get_signal_handler(source), "rename", &VolumeName::obsSourceRenamed, this);
	removedSignal = OBSSignal(obs_source_get_signal_handler(source), "remove", &VolumeName::obsSourceRemoved, this);
	destroyedSignal =
		OBSSignal(obs_source_get_signal_handler(source), "destroy", &VolumeName::obsSourceDestroyed, this);

	QHBoxLayout *layout = new QHBoxLayout();
	setLayout(layout);

	label = new QLabel(this);
	label->setIndent(0);
	layout->addWidget(label);

	layout->setContentsMargins(0, 0, indicatorWidth, 0);

	QString name = obs_source_get_name(source);
	setText(name);
}

VolumeName::~VolumeName() {}

void VolumeName::setAlignment(Qt::Alignment alignment_)
{
	if (textAlignment != alignment_) {
		textAlignment = alignment_;
		update();
	}
}

QSize VolumeName::minimumSizeHint() const
{
	QStyleOptionButton opt;
	opt.initFrom(this);

	QString plainText = getPlainText(fullText);

	QFontMetrics metrics(label->font());
	QSize textSize = metrics.size(Qt::TextSingleLine, plainText);

	int width = textSize.width();
	int height = textSize.height();

	if (!opt.icon.isNull()) {
		height = std::max(height, indicatorWidth);
	}

	QSize contentsSize = style()->sizeFromContents(QStyle::CT_PushButton, &opt, QSize(width, height), this);

	contentsSize.rwidth() += indicatorWidth;

	return contentsSize;
}

QSize VolumeName::sizeHint() const
{
	QString plainText = getPlainText(fullText);

	QFontMetrics metrics(label->font());

	int textWidth = metrics.horizontalAdvance(plainText);
	int textHeight = metrics.height();

	int width = textWidth + indicatorWidth;

	// Account for label margins if needed
	QMargins margins = label->contentsMargins();
	width += margins.left() + margins.right();
	int height = textHeight + margins.top() + margins.bottom();

	return QSize(width, height);
}

void VolumeName::obsSourceRenamed(void *data, calldata_t *params)
{
	VolumeName *widget = static_cast<VolumeName *>(data);
	const char *name = calldata_string(params, "new_name");

	QMetaObject::invokeMethod(widget, "onRenamed", Qt::QueuedConnection, Q_ARG(QString, name));
}

void VolumeName::obsSourceRemoved(void *data, calldata_t *)
{
	VolumeName *widget = static_cast<VolumeName *>(data);

	QMetaObject::invokeMethod(widget, "onRemoved", Qt::QueuedConnection);
}

void VolumeName::obsSourceDestroyed(void *data, calldata_t *)
{
	VolumeName *widget = static_cast<VolumeName *>(data);

	QMetaObject::invokeMethod(widget, "onDestroyed", Qt::QueuedConnection);
}

void VolumeName::resizeEvent(QResizeEvent *event)
{
	updateLabelText(text());
	QAbstractButton::resizeEvent(event);
}

void VolumeName::paintEvent(QPaintEvent *)
{
	QStylePainter painter(this);
	QStyleOptionButton opt;
	opt.initFrom(this);

	painter.drawControl(QStyle::CE_PushButtonBevel, opt);

	QRect contentRect = style()->subElementRect(QStyle::SE_PushButtonContents, &opt, this);
	int paddingRight = opt.rect.right() - contentRect.right();

	int indicatorWidth = style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);

	QStyleOption arrowOpt = opt;
	QRect arrowRect(opt.rect.right() - indicatorWidth - paddingRight / 2,
			opt.rect.center().y() - indicatorWidth / 2, indicatorWidth, indicatorWidth);
	arrowOpt.rect = arrowRect;

	painter.drawPrimitive(QStyle::PE_IndicatorArrowDown, arrowOpt);
}

void VolumeName::onRenamed(QString name)
{
	setText(name);

	std::string nameStr = name.toStdString();
	emit renamed(nameStr.c_str());
}

void VolumeName::setText(const QString &text)
{
	QAbstractButton::setText(text);
	updateGeometry();
	updateLabelText(text);
}

void VolumeName::updateLabelText(const QString &name)
{
	QString plainText = getPlainText(name);

	QFontMetrics metrics(label->font());

	int availableWidth = label->contentsRect().width();
	if (availableWidth <= 0) {
		label->clear();
		fullText = name;
		return;
	}

	int textWidth = metrics.horizontalAdvance(plainText);

	bool isRichText = (plainText != name);
	bool needsElide = textWidth > availableWidth;

	if (needsElide && !isRichText) {
		QString elided = metrics.elidedText(plainText, Qt::ElideMiddle, availableWidth);
		label->setText(elided);
	} else {
		label->setText(name);
	}

	fullText = name;
}

void VolumeName::onRemoved()
{
	emit removed();
}

void VolumeName::onDestroyed()
{
	emit destroyed();
}
