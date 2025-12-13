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

#include <Idian/Row.hpp>

#include <QApplication>
#include <QComboBox>
#include <QSizePolicy>
#include <QSvgRenderer>

#include <Idian/moc_Row.cpp>

namespace idian {

Row::Row(QWidget *parent) : GenericRow(parent)
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
	Utils::addClass(nameLabel, "title");

	descriptionLabel = new QLabel();
	descriptionLabel->setVisible(false);
	Utils::addClass(descriptionLabel, "description");

	labelLayout->addWidget(nameLabel);
	labelLayout->addWidget(descriptionLabel);

	layout->addLayout(labelLayout, 0, 1, Qt::AlignLeft);
}

void Row::setPrefix(QWidget *w, bool auto_connect)
{
	setSuffixEnabled(false);

	prefix_ = w;

	if (auto_connect)
		this->connectBuddyWidget(w);

	prefix_->setParent(this);
	layout->addWidget(prefix_, 0, 0, Qt::AlignLeft);
	layout->setColumnStretch(0, 3);
}

void Row::setSuffix(QWidget *w, bool auto_connect)
{
	setPrefixEnabled(false);

	suffix_ = w;

	if (auto_connect)
		this->connectBuddyWidget(w);

	suffix_->setParent(this);
	layout->addWidget(suffix_, 0, 2, Qt::AlignRight | Qt::AlignVCenter);
}

void Row::setPrefixEnabled(bool enabled)
{
	if (!prefix_)
		return;
	if (enabled)
		setSuffixEnabled(false);
	if (enabled == prefix_->isEnabled() && enabled == prefix_->isVisible())
		return;

	layout->setColumnStretch(0, enabled ? 3 : 0);
	prefix_->setEnabled(enabled);
	prefix_->setVisible(enabled);
}

void Row::setSuffixEnabled(bool enabled)
{
	if (!suffix_)
		return;
	if (enabled)
		setPrefixEnabled(false);
	if (enabled == suffix_->isEnabled() && enabled == suffix_->isVisible())
		return;

	suffix_->setEnabled(enabled);
	suffix_->setVisible(enabled);
}

void Row::setTitle(const QString &name)
{
	nameLabel->setText(name);
	setAccessibleName(name);
	showTitle(true);
}

void Row::setDescription(const QString &description)
{
	descriptionLabel->setText(description);
	setAccessibleDescription(description);
	showDescription(true);
}

void Row::showTitle(bool visible)
{
	nameLabel->setVisible(visible);
}

void Row::showDescription(bool visible)
{
	descriptionLabel->setVisible(visible);
}

void Row::setBuddy(QWidget *widget)
{
	buddyWidget = widget;
	Utils::addClass(widget, "row-buddy");
}

void Row::setChangeCursor(bool change)
{
	changeCursor = change;
	Utils::toggleClass("cursor-pointer", change);
}

void Row::enterEvent(QEnterEvent *event)
{
	if (!isEnabled())
		return;

	if (changeCursor) {
		setCursor(Qt::PointingHandCursor);
	}

	Utils::addClass("hover");

	if (buddyWidget)
		Utils::repolish(buddyWidget);

	if (hasPrefix() || hasSuffix()) {
		Utils::polishChildren();
	}

	GenericRow::enterEvent(event);
}

void Row::leaveEvent(QEvent *event)
{
	Utils::removeClass("hover");

	if (buddyWidget)
		Utils::repolish(buddyWidget);

	if (hasPrefix() || hasSuffix()) {
		Utils::polishChildren();
	}

	GenericRow::leaveEvent(event);
}

void Row::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() & Qt::LeftButton) {
		emit clicked();
	}
	QFrame::mouseReleaseEvent(event);
}

void Row::keyReleaseEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Enter) {
		emit clicked();
	}
	QFrame::keyReleaseEvent(event);
}

void Row::connectBuddyWidget(QWidget *widget)
{
	setAccessibleName(nameLabel->text());
	setFocusProxy(widget);
	setBuddy(widget);

	// If element is a ToggleSwitch and checkable, forward clicks to the widget
	ToggleSwitch *obsToggle = dynamic_cast<ToggleSwitch *>(widget);
	if (obsToggle && obsToggle->isCheckable()) {
		setChangeCursor(true);

		connect(this, &Row::clicked, obsToggle, &ToggleSwitch::click);
		return;
	}

	// If element is any other QAbstractButton subclass, and checkable, forward clicks to the widget.
	QAbstractButton *button = dynamic_cast<QAbstractButton *>(widget);
	if (button && button->isCheckable()) {
		setChangeCursor(true);

		connect(this, &Row::clicked, button, &QAbstractButton::click);
		return;
	}

	// If element is an ComboBox, clicks toggle the dropdown.
	ComboBox *obsCombo = dynamic_cast<ComboBox *>(widget);
	if (obsCombo) {
		setChangeCursor(true);

		connect(this, &Row::clicked, obsCombo, &ComboBox::togglePopup);
		return;
	}
}

// Button for expanding a collapsible ActionRow
ExpandButton::ExpandButton(QWidget *parent) : QAbstractButton(parent), Utils(this)
{
	setCheckable(true);
}

void ExpandButton::paintEvent(QPaintEvent *)
{
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

// Row variant that can be expanded to show another properties list
CollapsibleRow::CollapsibleRow(QWidget *parent) : GenericRow(parent)
{
	layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	setLayout(layout);

	rowWidget = new RowFrame();
	rowLayout = new QHBoxLayout();
	rowLayout->setContentsMargins(0, 0, 0, 0);
	rowLayout->setSpacing(0);
	rowWidget->setLayout(rowLayout);

	actionRow = new Row();
	actionRow->setChangeCursor(false);

	rowLayout->addWidget(actionRow);

	propertyList = new PropertiesList(this);
	propertyList->setVisible(false);

	expandFrame = new QFrame();
	btnLayout = new QHBoxLayout();
	btnLayout->setContentsMargins(0, 0, 0, 0);
	btnLayout->setSpacing(0);
	expandFrame->setLayout(btnLayout);
	Utils::addClass(expandFrame, "btn-frame");
	actionRow->setBuddy(expandFrame);

	expandButton = new ExpandButton(this);
	btnLayout->addWidget(expandButton);

	rowLayout->addWidget(expandFrame);

	layout->addWidget(rowWidget);
	layout->addWidget(propertyList);

	actionRow->setFocusProxy(expandButton);

	connect(expandButton, &QAbstractButton::clicked, this, &CollapsibleRow::toggleVisibility);
	connect(actionRow, &Row::clicked, expandButton, &QAbstractButton::click);
}

void CollapsibleRow::setCheckable(bool check)
{
	checkable = check;

	if (checkable && !toggleSwitch) {
		propertyList->setEnabled(false);
		Utils::polishChildren(propertyList);

		toggleSwitch = new ToggleSwitch(false);

		actionRow->setSuffix(toggleSwitch, false);
		connect(toggleSwitch, &ToggleSwitch::toggled, propertyList, &PropertiesList::setEnabled);
		connect(toggleSwitch, &ToggleSwitch::toggled, this, &CollapsibleRow::toggled);
	}

	if (!checkable && toggleSwitch) {
		propertyList->setEnabled(true);
		Utils::polishChildren(propertyList);

		actionRow->suffix()->deleteLater();
	}
}

void CollapsibleRow::setChecked(bool checked)
{
	if (!isCheckable()) {
		throw std::logic_error("Called setChecked on a non-checkable row.");
	}

	toggleSwitch->setChecked(checked);
}

void CollapsibleRow::setTitle(const QString &name)
{
	actionRow->setTitle(name);
}

void CollapsibleRow::setDescription(const QString &description)
{
	actionRow->setDescription(description);
}

void CollapsibleRow::toggleVisibility()
{
	bool visible = !propertyList->isVisible();

	propertyList->setVisible(visible);
	expandButton->setChecked(visible);
}

void CollapsibleRow::addRow(GenericRow *actionRow)
{
	propertyList->addRow(actionRow);
}

RowFrame::RowFrame(QWidget *parent) : QFrame(parent), Utils(this) {}

void RowFrame::enterEvent(QEnterEvent *event)
{
	setCursor(Qt::PointingHandCursor);

	Utils::addClass("hover");
	Utils::polishChildren();

	QWidget::enterEvent(event);
}

void RowFrame::leaveEvent(QEvent *event)
{
	Utils::removeClass("hover");
	Utils::polishChildren();

	QWidget::leaveEvent(event);
}
} // namespace idian
