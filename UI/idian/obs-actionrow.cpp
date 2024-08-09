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

#include "obs-actionrow.hpp"
#include <util/base.h>
#include <QSvgRenderer>

OBSActionRow::OBSActionRow(QWidget *parent) : OBSActionBaseClass(parent)
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

	nameLbl = new QLabel();
	nameLbl->setVisible(false);
	OBSWidgetUtils::addClass(nameLbl, "title");

	descLbl = new QLabel();
	descLbl->setVisible(false);
	OBSWidgetUtils::addClass(descLbl, "description");

	labelLayout->addWidget(nameLbl);
	labelLayout->addWidget(descLbl);

	layout->addLayout(labelLayout, 0, 1, Qt::AlignLeft);
}

void OBSActionRow::setPrefix(QWidget *w, bool auto_connect)
{
	setSuffixEnabled(false);

	_prefix = w;

	if (auto_connect)
		this->autoConnectWidget(w);

	_prefix->setParent(this);
	layout->addWidget(_prefix, 0, 0, Qt::AlignLeft);
	layout->setColumnStretch(0, 3);
}

void OBSActionRow::setSuffix(QWidget *w, bool auto_connect)
{
	setPrefixEnabled(false);

	_suffix = w;

	if (auto_connect)
		this->autoConnectWidget(w);

	_suffix->setParent(this);
	layout->addWidget(_suffix, 0, 2, Qt::AlignRight | Qt::AlignVCenter);
}

void OBSActionRow::setPrefixEnabled(bool enabled)
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

void OBSActionRow::setSuffixEnabled(bool enabled)
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

void OBSActionRow::setTitle(QString name)
{
	nameLbl->setText(name);
	setAccessibleName(name);
	showTitle(true);
}

void OBSActionRow::setDescription(QString desc)
{
	descLbl->setText(desc);
	setAccessibleDescription(desc);
	showDescription(true);
}

void OBSActionRow::showTitle(bool visible)
{
	nameLbl->setVisible(visible);
}

void OBSActionRow::showDescription(bool visible)
{
	descLbl->setVisible(visible);
}

void OBSActionRow::setHighlight(QWidget *w)
{
	highlightWidget = w;
	OBSWidgetUtils::addClass(w, "row-highlight");
}

void OBSActionRow::setChangeCursor(bool change)
{
	changeCursor = change;
	OBSWidgetUtils::toggleClass("cursorPointer", change);
}

void OBSActionRow::enterEvent(QEnterEvent *e)
{
	if (!isEnabled())
		return;

	if (changeCursor) {
		setCursor(Qt::PointingHandCursor);
	}

	OBSWidgetUtils::addClass("hover");

	if (highlightWidget)
		OBSWidgetUtils::repolish(highlightWidget);

	if (hasPrefix() || hasSuffix()) {
		OBSWidgetUtils::polishChildren();
	}

	OBSActionBaseClass::enterEvent(e);
}

void OBSActionRow::leaveEvent(QEvent *e)
{
	OBSWidgetUtils::removeClass("hover");

	if (highlightWidget)
		OBSWidgetUtils::repolish(highlightWidget);

	if (hasPrefix() || hasSuffix()) {
		OBSWidgetUtils::polishChildren();
	}

	OBSActionBaseClass::leaveEvent(e);
}

void OBSActionRow::mouseReleaseEvent(QMouseEvent *e)
{
	if (e->button() & Qt::LeftButton) {
		emit clicked();
	}
	QFrame::mouseReleaseEvent(e);
}

void OBSActionRow::keyReleaseEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Space || e->key() == Qt::Key_Enter) {
		emit clicked();
	}
	QFrame::keyReleaseEvent(e);
}

void OBSActionRow::autoConnectWidget(QWidget *w)
{
	setAccessibleName(nameLbl->text());
	setFocusProxy(w);

	setHighlight(w);

	/* If element is an OBSToggleSwitch and checkable, forward
	 * clicks to the widget */
	OBSToggleSwitch *tgle = dynamic_cast<OBSToggleSwitch *>(w);
	if (tgle && tgle->isCheckable()) {
		setChangeCursor(true);

		connect(this, &OBSActionRow::clicked, tgle,
			&OBSToggleSwitch::click);
		return;
	}

	/* If element is any other QAbstractButton subclass,
	 * and checkable, forward clicks to the widget. */
	QAbstractButton *abtn = dynamic_cast<QAbstractButton *>(w);
	if (abtn && abtn->isCheckable()) {
		setChangeCursor(true);

		connect(this, &OBSActionRow::clicked, abtn,
			&QAbstractButton::click);
		return;
	}
}

/*
* Button for expanding a collapsible ActionRow
*/
OBSActionCollapseButton::OBSActionCollapseButton(QWidget *parent)
	: QAbstractButton(parent),
	  OBSWidgetUtils(this)
{
	setCheckable(true);
}

void OBSActionCollapseButton::paintEvent(QPaintEvent *e)
{
	UNUSED_PARAMETER(e);

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

OBSCollapsibleContainer::OBSCollapsibleContainer(const QString &name,
						 QWidget *parent)
	: OBSActionBaseClass(parent)
{
	layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	setLayout(layout);

	rowWidget = new OBSCollapsibleRow();
	rowLayout = new QHBoxLayout();
	rowLayout->setContentsMargins(0, 0, 0, 0);
	rowLayout->setSpacing(0);
	rowWidget->setLayout(rowLayout);

	ar = new OBSActionRow();
	ar->setTitle(name);
	ar->setChangeCursor(false);

	rowLayout->addWidget(ar);

	plist = new OBSPropertiesList(this);
	plist->setVisible(false);

	collapseFrame = new QFrame();
	btnLayout = new QHBoxLayout();
	btnLayout->setContentsMargins(0, 0, 0, 0);
	btnLayout->setSpacing(0);
	collapseFrame->setLayout(btnLayout);
	OBSWidgetUtils::addClass(collapseFrame, "btn-frame");
	ar->setHighlight(collapseFrame);

	collapseBtn = new OBSActionCollapseButton(this);
	btnLayout->addWidget(collapseBtn);

	rowLayout->addWidget(collapseFrame);

	layout->addWidget(rowWidget);
	layout->addWidget(plist);

	ar->setFocusProxy(collapseBtn);

	connect(collapseBtn, &QAbstractButton::clicked, this,
		&OBSCollapsibleContainer::toggleVisibility);

	connect(ar, &OBSActionRow::clicked, collapseBtn,
		&QAbstractButton::click);
}

/*
* ActionRow variant that can be expanded to show another properties list
*/
OBSCollapsibleContainer::OBSCollapsibleContainer(const QString &name,
						 const QString &desc,
						 QWidget *parent)
	: OBSCollapsibleContainer(name, parent)
{
	ar->setDescription(desc);
}

void OBSCollapsibleContainer::setCheckable(bool check)
{
	checkable = check;

	if (checkable && !toggleSwitch) {
		plist->setEnabled(false);
		OBSWidgetUtils::polishChildren(plist);

		toggleSwitch = new OBSToggleSwitch(false);

		ar->setSuffix(toggleSwitch, false);
		connect(toggleSwitch, &OBSToggleSwitch::toggled, plist,
			&OBSPropertiesList::setEnabled);
	}

	if (!checkable && toggleSwitch) {
		plist->setEnabled(true);
		OBSWidgetUtils::polishChildren(plist);

		ar->suffix()->deleteLater();
	}
}

void OBSCollapsibleContainer::toggleVisibility()
{
	bool visible = !plist->isVisible();

	plist->setVisible(visible);
	collapseBtn->setChecked(visible);
}

void OBSCollapsibleContainer::addRow(OBSActionBaseClass *ar)
{
	plist->addRow(ar);
}

OBSCollapsibleRow::OBSCollapsibleRow(QWidget *parent)
	: QWidget(parent),
	  OBSWidgetUtils(this)
{
}

void OBSCollapsibleRow::enterEvent(QEnterEvent *e)
{
	setCursor(Qt::PointingHandCursor);

	OBSWidgetUtils::addClass("hover");
	OBSWidgetUtils::polishChildren();

	QWidget::enterEvent(e);
}

void OBSCollapsibleRow::leaveEvent(QEvent *e)
{
	OBSWidgetUtils::removeClass("hover");
	OBSWidgetUtils::polishChildren();

	QWidget::leaveEvent(e);
}
