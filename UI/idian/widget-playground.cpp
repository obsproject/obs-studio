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

#include <QTimer>

#include "obs-widgets.hpp"
#include "widget-playground.hpp"

// This file is a temporary playground for building new widgets,
// it may be removed in the future.

IdianPlayground::IdianPlayground(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui_IdianPlayground)
{
	ui->setupUi(this);

	OBSGroupBox *test;
	OBSActionRow *tmp;

	OBSComboBox *cbox = new OBSComboBox;
	cbox->addItem("Test 1");
	cbox->addItem("Test 2");

	/* Group box 1 */
	test = new OBSGroupBox(this);

	tmp = new OBSActionRow("Row with a dropdown");
	tmp->setSuffix(cbox);
	test->properties()->addRow(tmp);

	cbox = new OBSComboBox;
	cbox->addItem("Test 3");
	cbox->addItem("Test 4");
	tmp = new OBSActionRow("Row with a dropdown", "And a subtitle!");
	tmp->setSuffix(cbox);
	test->properties()->addRow(tmp);

	tmp = new OBSActionRow("Toggle Switch");
	tmp->setSuffix(new OBSToggleSwitch());
	test->properties()->addRow(tmp);
	ui->scrollAreaWidgetContents->layout()->addWidget(test);

	tmp = new OBSActionRow("Delayed toggle switch",
			       "The state can be set separately");
	auto tswitch = new OBSToggleSwitch;
	tswitch->setManualStatusChange(true);
	connect(tswitch, &OBSToggleSwitch::toggled, this, [=](bool toggled) {
		QTimer::singleShot(1000,
				   [=]() { tswitch->setStatus(toggled); });
	});
	tmp->setSuffix(tswitch);
	test->properties()->addRow(tmp);

	/* Group box 2 */
	test = new OBSGroupBox("Just a few checkboxes", this);

	//QTimer::singleShot(10000, this, [=]() { test->properties()->clear(); });

	tmp = new OBSActionRow("Box 1");
	tmp->setPrefix(new OBSCheckBox);
	test->properties()->addRow(tmp);

	tmp = new OBSActionRow("Box 2");
	tmp->setPrefix(new OBSCheckBox);
	test->properties()->addRow(tmp);

	ui->scrollAreaWidgetContents->layout()->addWidget(test);

	/* Group box 2 */
	test = new OBSGroupBox("Another Group",
			       "With a subtitle", this);

	tmp = new OBSActionRow("Placeholder");
	tmp->setSuffix(new OBSToggleSwitch);
	test->properties()->addRow(tmp);

	OBSCollapsibleActionRow *tmp2 = new OBSCollapsibleActionRow(
		"A Collapsible row!", nullptr, true, this);
	test->addRow(tmp2);

	tmp = new OBSActionRow("Spin box demo");
	tmp->setSuffix(new OBSDoubleSpinBox());
	tmp2->addRow(tmp);

	tmp = new OBSActionRow("Just another placeholder");
	tmp->setSuffix(new OBSToggleSwitch(true));
	tmp2->addRow(tmp);

	tmp = new OBSActionRow("Placeholder 2");
	tmp->setSuffix(new OBSToggleSwitch);
	test->properties()->addRow(tmp);

	ui->scrollAreaWidgetContents->setContentsMargins(0, 0, 0, 0);
	ui->scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
	ui->scrollAreaWidgetContents->layout()->addWidget(test);
	ui->scrollAreaWidgetContents->layout()->setAlignment(Qt::AlignTop |
							     Qt::AlignHCenter);
}
