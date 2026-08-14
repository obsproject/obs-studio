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

#include <Idian/CollapsibleGroup.hpp>
#include <Idian/Idian.hpp>
#include <Idian/ListHeader.hpp>
#include <Idian/RowInfo.hpp>

#include <QTimer>

#include "moc_OBSIdianPlayground.cpp"

using namespace idian;

OBSIdianPlayground::OBSIdianPlayground(QWidget *parent) : QDialog(parent), ui(new Ui_OBSIdianPlayground)
{
	ui->setupUi(this);

	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

	QLayout *contents = ui->scrollAreaWidgetContents->layout();

	RowList *rowList;
	Row *tmp;

	ComboBox *cbox = new ComboBox;
	cbox->addItem("Test 1");
	cbox->addItem("Test 2");

	// Group box 1
	rowList = new RowList(this);

	tmp = new Row();
	tmp->addWidget(new RowInfo(tmp, "Row with a dropdown"));
	tmp->addBuddy(cbox);
	rowList->addRow(tmp);

	cbox = new ComboBox;
	cbox->addItem("Test 3");
	cbox->addItem("Test 4");
	tmp = new Row();
	tmp->addWidget(new RowInfo(tmp, "Row with a dropdown", "And a subtitle!"));
	tmp->addBuddy(cbox);
	rowList->addRow(tmp);

	tmp = new Row();
	tmp->addWidget(new RowInfo(tmp, "Toggle Switch"));
	tmp->addBuddy(new ToggleSwitch());
	rowList->addRow(tmp);
	contents->addWidget(rowList);

	tmp = new Row();
	tmp->addWidget(new RowInfo(tmp, "Delayed toggle switch", "The state can be set separately"));
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
	tmp->addBuddy(tswitch);
	rowList->addRow(tmp);

	// Group box 2
	contents->addWidget(new ListHeader(rowList, "Just a few checkboxes"));
	rowList = new RowList();

	tmp = new Row();
	tmp->addBuddy(new CheckBox);
	tmp->addWidget(new RowInfo(tmp, "Box 1"));
	rowList->addRow(tmp);

	tmp = new Row();
	tmp->addBuddy(new CheckBox);
	tmp->addWidget(new RowInfo(tmp, "Box 2"));
	rowList->addRow(tmp);

	ui->scrollAreaWidgetContents->layout()->addWidget(rowList);

	// Group box 2
	rowList = new RowList();
	contents->addWidget(new ListHeader(rowList, "Another Group", "With a subtitle"));

	tmp = new Row();
	tmp->addWidget(new RowInfo(tmp, "Placeholder"));
	tmp->addBuddy(new ToggleSwitch);
	rowList->addRow(tmp);

	CollapsibleGroup *tmp2 = new CollapsibleGroup(this);
	tmp2->row()->layout()->insertWidget(0, new RowInfo(tmp, "A Collapsible row!"));
	tmp2->setCheckable(true);
	rowList->addRow(tmp2);

	tmp = new Row();
	tmp->addWidget(new RowInfo(tmp, "Spin box demo"));
	tmp->addBuddy(new DoubleSpinBox());
	tmp2->list()->addRow(tmp);

	tmp = new Row();
	tmp->addWidget(new RowInfo(tmp, "Just another placeholder"));
	tmp->addBuddy(new ToggleSwitch(true));
	tmp2->list()->addRow(tmp);

	tmp = new Row();
	tmp->addWidget(new RowInfo(tmp, "Placeholder 2"));
	tmp->addBuddy(new ToggleSwitch);
	rowList->addRow(tmp);

	ui->scrollAreaWidgetContents->setContentsMargins(0, 0, 0, 0);
	ui->scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
	ui->scrollAreaWidgetContents->layout()->addWidget(rowList);
	ui->scrollAreaWidgetContents->layout()->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

	// Test Checkable Group
	auto *test2 = new RowList(this);

	auto *header = new ListHeader(this, "Checkable Group", "Description goes here");
	header->setCheckable(true);
	test2->addHeader(header);

	ui->scrollAreaWidgetContents->layout()->addWidget(test2);
}
