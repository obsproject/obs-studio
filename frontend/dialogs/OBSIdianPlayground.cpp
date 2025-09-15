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

#include "OBSIdianPlayground.hpp"

#include <Idian/Idian.hpp>

#include <QTimer>

#include "moc_OBSIdianPlayground.cpp"

using namespace idian;

OBSIdianPlayground::OBSIdianPlayground(QWidget *parent) : QDialog(parent), ui(new Ui_OBSIdianPlayground)
{
	ui->setupUi(this);

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

	Group *test;
	Row *tmp;

	ComboBox *cbox = new ComboBox;
	cbox->addItem("Test 1");
	cbox->addItem("Test 2");

	// Group box 1
	test = new Group(this);

	tmp = new Row();
	tmp->setTitle("Row with a dropdown");
	tmp->setSuffix(cbox);
	test->properties()->addRow(tmp);

	cbox = new ComboBox;
	cbox->addItem("Test 3");
	cbox->addItem("Test 4");
	tmp = new Row();
	tmp->setTitle("Row with a dropdown");
	tmp->setDescription("And a subtitle!");
	tmp->setSuffix(cbox);
	test->properties()->addRow(tmp);

	tmp = new Row();
	tmp->setTitle("Toggle Switch");
	tmp->setSuffix(new ToggleSwitch());
	test->properties()->addRow(tmp);
	ui->scrollAreaWidgetContents->layout()->addWidget(test);

	tmp = new Row();
	tmp->setTitle("Delayed toggle switch");
	tmp->setDescription("The state can be set separately");
	auto tswitch = new ToggleSwitch;
	tswitch->setDelayed(true);
	connect(tswitch, &ToggleSwitch::pendingChecked, this, [=]() {
		// Do async enable stuff, then set toggle status when complete
		QTimer::singleShot(1000, [=]() { tswitch->setStatus(true); });
	});
	connect(tswitch, &ToggleSwitch::pendingUnchecked, this, [=]() {
		// Do async disable stuff, then set toggle status when complete
		QTimer::singleShot(1000, [=]() { tswitch->setStatus(false); });
	});
	tmp->setSuffix(tswitch);
	test->properties()->addRow(tmp);

	// Group box 2
	test = new Group();
	test->setTitle("Just a few checkboxes");

	tmp = new Row();
	tmp->setTitle("Box 1");
	tmp->setPrefix(new CheckBox);
	test->properties()->addRow(tmp);

	tmp = new Row();
	tmp->setTitle("Box 2");
	tmp->setPrefix(new CheckBox);
	test->properties()->addRow(tmp);

	ui->scrollAreaWidgetContents->layout()->addWidget(test);

	// Group box 2
	test = new Group();
	test->setTitle("Another Group");
	test->setDescription("With a subtitle");

	tmp = new Row();
	tmp->setTitle("Placeholder");
	tmp->setSuffix(new ToggleSwitch);
	test->properties()->addRow(tmp);

	CollapsibleRow *tmp2 = new CollapsibleRow(this);
	tmp2->setTitle("A Collapsible row!");
	tmp2->setCheckable(true);
	test->addRow(tmp2);

	tmp = new Row();
	tmp->setTitle("Spin box demo");
	tmp->setSuffix(new DoubleSpinBox());
	tmp2->addRow(tmp);

	tmp = new Row();
	tmp->setTitle("Just another placeholder");
	tmp->setSuffix(new ToggleSwitch(true));
	tmp2->addRow(tmp);

	tmp = new Row();
	tmp->setTitle("Placeholder 2");
	tmp->setSuffix(new ToggleSwitch);
	test->properties()->addRow(tmp);

	ui->scrollAreaWidgetContents->setContentsMargins(0, 0, 0, 0);
	ui->scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
	ui->scrollAreaWidgetContents->layout()->addWidget(test);
	ui->scrollAreaWidgetContents->layout()->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

	// Test Checkable Group
	Group *test2 = new Group();
	test2->setTitle("Checkable Group");
	test2->setDescription("Description goes here");
	test2->setCheckable(true);
	ui->scrollAreaWidgetContents->layout()->addWidget(test2);
}
