/******************************************************************************
    Copyright (C) 2023 by Dennis Sädtler <dennis@obsproject.com>

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

#include <QComboBox>
#include <QApplication>
#include <QSizePolicy>

#include "OBSActionRow.hpp"
#include <util/base.h>
#include <QSvgRenderer>

OBSActionRowWidget::OBSActionRowWidget(QWidget *parent) : OBSActionRow(parent)
{
	layout = new QGridLayout(this);
	layout->setVerticalSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	labelLayout = new QVBoxLayout();
	labelLayout->setSpacing(0);
	labelLayout->setContentsMargins(0, 0, 0, 0);

	setFocusPolicy(Qt::StrongFocus);
	setLayout(layout);

	layout->setColumnMinimumWidth(0, 0);
	layout->setColumnStretch(0, 0);
	layout->setColumnStretch(1, 40);
	layout->setColumnStretch(2, 55);

	nameLabel = new QLabel();
	nameLabel->setVisible(false);
	OBSIdianUtils::addClass(nameLabel, "title");

	descriptionLabel = new QLabel();
	descriptionLabel->setVisible(false);
	OBSIdianUtils::addClass(descriptionLabel, "description");

	labelLayout->addWidget(nameLabel);
	labelLayout->addWidget(descriptionLabel);

	layout->addLayout(labelLayout, 0, 1, Qt::AlignLeft);
}

void OBSActionRowWidget::setPrefix(QWidget *w, bool auto_connect)
{
	setSuffixEnabled(false);

	_prefix = w;

	if (auto_connect)
		this->connectBuddyWidget(w);

	_prefix->setParent(this);
	layout->addWidget(_prefix, 0, 0, Qt::AlignLeft);
	layout->setColumnStretch(0, 3);
}

void OBSActionRowWidget::setSuffix(QWidget *w, bool auto_connect)
{
	setPrefixEnabled(false);

	_suffix = w;

	if (auto_connect)
		this->connectBuddyWidget(w);

	_suffix->setParent(this);
	layout->addWidget(_suffix, 0, 2, Qt::AlignRight | Qt::AlignVCenter);
}

void OBSActionRowWidget::setPrefixEnabled(bool enabled)
{
	if (!_prefix)
		return;
	if (enabled)
		setSuffixEnabled(false);
	if (enabled == _prefix->isEnabled() && enabled == _prefix->isVisible())
		return;

	layout->setColumnStretch(0, enabled ? 3 : 0);
	_prefix->setEnabled(enabled);
	_prefix->setVisible(enabled);
}

void OBSActionRowWidget::setSuffixEnabled(bool enabled)
{
	if (!_suffix)
		return;
	if (enabled)
		setPrefixEnabled(false);
	if (enabled == _suffix->isEnabled() && enabled == _suffix->isVisible())
		return;

	_suffix->setEnabled(enabled);
	_suffix->setVisible(enabled);
}

void OBSActionRowWidget::setTitle(QString name)
{
	nameLabel->setText(name);
	setAccessibleName(name);
	showTitle(true);
}

void OBSActionRowWidget::setDescription(QString description)
{
	descriptionLabel->setText(description);
	setAccessibleDescription(description);
	showDescription(true);
}

void OBSActionRowWidget::showTitle(bool visible)
{
	nameLabel->setVisible(visible);
}

void OBSActionRowWidget::showDescription(bool visible)
{
	descriptionLabel->setVisible(visible);
}

void OBSActionRowWidget::setBuddy(QWidget *widget)
{
	buddyWidget = widget;
	OBSIdianUtils::addClass(widget, "row-buddy");
}

void OBSActionRowWidget::setChangeCursor(bool change)
{
	changeCursor = change;
	OBSIdianUtils::toggleClass("cursor-pointer", change);
}

void OBSActionRowWidget::enterEvent(QEnterEvent *event)
{
	if (!isEnabled())
		return;

	if (changeCursor) {
		setCursor(Qt::PointingHandCursor);
	}

	OBSIdianUtils::addClass("hover");

	if (buddyWidget)
		OBSIdianUtils::repolish(buddyWidget);

	if (hasPrefix() || hasSuffix()) {
		OBSIdianUtils::polishChildren();
	}

	OBSActionRow::enterEvent(event);
}

void OBSActionRowWidget::leaveEvent(QEvent *event)
{
	OBSIdianUtils::removeClass("hover");

	if (buddyWidget)
		OBSIdianUtils::repolish(buddyWidget);

	if (hasPrefix() || hasSuffix()) {
		OBSIdianUtils::polishChildren();
	}

	OBSActionRow::leaveEvent(event);
}

void OBSActionRowWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() & Qt::LeftButton) {
		emit clicked();
	}
	QFrame::mouseReleaseEvent(event);
}

void OBSActionRowWidget::keyReleaseEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Enter) {
		emit clicked();
	}
	QFrame::keyReleaseEvent(event);
}

void OBSActionRowWidget::connectBuddyWidget(QWidget *widget)
{
	setAccessibleName(nameLabel->text());
	setFocusProxy(widget);
	setBuddy(widget);

	/* If element is an OBSToggleSwitch and checkable, forward
	 * clicks to the widget */
	OBSToggleSwitch *obsToggle = dynamic_cast<OBSToggleSwitch *>(widget);
	if (obsToggle && obsToggle->isCheckable()) {
		setChangeCursor(true);

		connect(this, &OBSActionRowWidget::clicked, obsToggle, &OBSToggleSwitch::click);
		return;
	}

	/* If element is any other QAbstractButton subclass,
	 * and checkable, forward clicks to the widget. */
	QAbstractButton *button = dynamic_cast<QAbstractButton *>(widget);
	if (button && button->isCheckable()) {
		setChangeCursor(true);

		connect(this, &OBSActionRowWidget::clicked, button, &QAbstractButton::click);
		return;
	}

	/* If element is an OBSComboBox, clicks toggle the dropdown. */
	OBSComboBox *obsCombo = dynamic_cast<OBSComboBox *>(widget);
	if (obsCombo) {
		setChangeCursor(true);

		connect(this, &OBSActionRowWidget::clicked, obsCombo, &OBSComboBox::togglePopup);
		return;
	}
}

/*
* Button for expanding a collapsible ActionRow
*/
OBSActionRowExpandButton::OBSActionRowExpandButton(QWidget *parent) : QAbstractButton(parent), OBSIdianUtils(this)
{
	setCheckable(true);
}

void OBSActionRowExpandButton::paintEvent(QPaintEvent *event)
{
	UNUSED_PARAMETER(event);

	QStyleOptionButton opt;
	opt.initFrom(this);
	QPainter p(this);

	bool checked = isChecked();

	opt.state.setFlag(QStyle::State_On, checked);
	opt.state.setFlag(QStyle::State_Off, !checked);

	opt.state.setFlag(QStyle::State_Sunken, checked);

	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);

	style()->drawPrimitive(QStyle::PE_PanelButtonCommand, &opt, &p, this);
	style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &opt, &p, this);
}

/*
* ActionRow variant that can be expanded to show another properties list
*/
OBSCollapsibleRowWidget::OBSCollapsibleRowWidget(const QString &name, QWidget *parent) : OBSActionRow(parent)
{
	layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	setLayout(layout);

	rowWidget = new OBSCollapsibleRowFrame();
	rowLayout = new QHBoxLayout();
	rowLayout->setContentsMargins(0, 0, 0, 0);
	rowLayout->setSpacing(0);
	rowWidget->setLayout(rowLayout);

	actionRow = new OBSActionRowWidget();
	actionRow->setTitle(name);
	actionRow->setChangeCursor(false);

	rowLayout->addWidget(actionRow);

	propertyList = new OBSPropertiesList(this);
	propertyList->setVisible(false);

	expandFrame = new QFrame();
	btnLayout = new QHBoxLayout();
	btnLayout->setContentsMargins(0, 0, 0, 0);
	btnLayout->setSpacing(0);
	expandFrame->setLayout(btnLayout);
	OBSIdianUtils::addClass(expandFrame, "btn-frame");
	actionRow->setBuddy(expandFrame);

	expandButton = new OBSActionRowExpandButton(this);
	btnLayout->addWidget(expandButton);

	rowLayout->addWidget(expandFrame);

	layout->addWidget(rowWidget);
	layout->addWidget(propertyList);

	actionRow->setFocusProxy(expandButton);

	connect(expandButton, &QAbstractButton::clicked, this, &OBSCollapsibleRowWidget::toggleVisibility);

	connect(actionRow, &OBSActionRowWidget::clicked, expandButton, &QAbstractButton::click);
}

OBSCollapsibleRowWidget::OBSCollapsibleRowWidget(const QString &name, const QString &desc, QWidget *parent)
	: OBSCollapsibleRowWidget(name, parent)
{
	actionRow->setDescription(desc);
}

void OBSCollapsibleRowWidget::setCheckable(bool check)
{
	checkable = check;

	if (checkable && !toggleSwitch) {
		propertyList->setEnabled(false);
		OBSIdianUtils::polishChildren(propertyList);

		toggleSwitch = new OBSToggleSwitch(false);

		actionRow->setSuffix(toggleSwitch, false);
		connect(toggleSwitch, &OBSToggleSwitch::toggled, propertyList, &OBSPropertiesList::setEnabled);
	}

	if (!checkable && toggleSwitch) {
		propertyList->setEnabled(true);
		OBSIdianUtils::polishChildren(propertyList);

		actionRow->suffix()->deleteLater();
	}
}

void OBSCollapsibleRowWidget::toggleVisibility()
{
	bool visible = !propertyList->isVisible();

	propertyList->setVisible(visible);
	expandButton->setChecked(visible);
}

void OBSCollapsibleRowWidget::addRow(OBSActionRow *actionRow)
{
	propertyList->addRow(actionRow);
}

OBSCollapsibleRowFrame::OBSCollapsibleRowFrame(QWidget *parent) : QFrame(parent), OBSIdianUtils(this) {}

void OBSCollapsibleRowFrame::enterEvent(QEnterEvent *event)
{
	setCursor(Qt::PointingHandCursor);

	OBSIdianUtils::addClass("hover");
	OBSIdianUtils::polishChildren();

	QWidget::enterEvent(event);
}

void OBSCollapsibleRowFrame::leaveEvent(QEvent *event)
{
	OBSIdianUtils::removeClass("hover");
	OBSIdianUtils::polishChildren();

	QWidget::leaveEvent(event);
}
