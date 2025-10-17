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

#include <widgets/VolumeName.hpp>

#include <QStyle>
#include <QStylePainter>
#include <QStyleOptionButton>

#include "moc_VolumeName.cpp"

VolumeName::VolumeName(const OBSSource source, QWidget *parent)
	: QAbstractButton(parent),
	  renamedSignal(obs_source_get_signal_handler(source), "rename", &VolumeName::obsSourceRenamed, this),
	  removedSignal(obs_source_get_signal_handler(source), "remove", &VolumeName::obsSourceRemoved, this),
	  destroyedSignal(obs_source_get_signal_handler(source), "destroy", &VolumeName::obsSourceDestroyed, this)
{
	setText(obs_source_get_name(source));
}

VolumeName::~VolumeName() {}

void VolumeName::setAlignment(Qt::Alignment alignment_)
{
	if (textAlignment != alignment_) {
		textAlignment = alignment_;
		update();
	}
}

QSize VolumeName::sizeHint() const
{
	QStyleOptionButton opt;
	opt.initFrom(this);

	const QFontMetrics metrics = fontMetrics();
	QSize textSize = metrics.size(Qt::TextShowMnemonic, text());

	int iconWidth = style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, this);
	int iconHeight = iconWidth;

	int width = textSize.width();
	int height = textSize.height();

	if (!opt.icon.isNull()) {
		height = std::max(height, iconHeight);
	}

	const int spacing = style()->pixelMetric(QStyle::PM_ButtonMargin, &opt, this) / 2;
	width += iconWidth + spacing;

	QSize contents(width, height);
	return style()->sizeFromContents(QStyle::CT_PushButton, &opt, contents, this);
}

void VolumeName::obsSourceRenamed(void *data, calldata_t *params)
{
	auto &widget = *static_cast<VolumeName *>(data);

	const char *name = calldata_string(params, "new_name");
	widget.setText(name);

	emit widget.renamed(name);
}

void VolumeName::obsSourceRemoved(void *data, calldata_t *)
{
	auto &widget = *static_cast<VolumeName *>(data);
	emit widget.removed();
}

void VolumeName::obsSourceDestroyed(void *data, calldata_t *)
{
	auto &widget = *static_cast<VolumeName *>(data);
	widget.destroy();
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
	QRect textRect = contentRect.adjusted(0, 0, -indicatorWidth - paddingRight / 2, 0);

	painter.setFont(font());
	painter.setPen(opt.palette.color(QPalette::ButtonText));
	Qt::Alignment align = (textAlignment & (Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter)) ? textAlignment
												    : Qt::AlignHCenter;
	align |= Qt::AlignVCenter;

	QFontMetrics metrics(font());
	QString elidedText = metrics.elidedText(text(), Qt::ElideMiddle, textRect.width());

	painter.drawText(textRect, align, elidedText);

	QStyleOption arrowOpt = opt;
	QRect arrowRect(opt.rect.right() - indicatorWidth - paddingRight / 2,
			opt.rect.center().y() - indicatorWidth / 2, indicatorWidth, indicatorWidth);
	arrowOpt.rect = arrowRect;

	painter.drawPrimitive(QStyle::PE_IndicatorArrowDown, arrowOpt);
}
