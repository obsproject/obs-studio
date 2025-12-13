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

#include <Idian/SpinBox.hpp>

#include <Idian/moc_SpinBox.cpp>

using idian::SpinBox;

SpinBox::SpinBox(QWidget *parent) : QFrame(parent)
{
	layout = new QHBoxLayout();
	setLayout(layout);

	layout->setContentsMargins(0, 0, 0, 0);

	decr = new QPushButton("-");
	decr->setObjectName("obsSpinBoxButton");
	layout->addWidget(decr);

	sbox = new QSpinBox();
	sbox->setObjectName("obsSpinBox");
	sbox->setButtonSymbols(QAbstractSpinBox::NoButtons);
	layout->addWidget(sbox);

	incr = new QPushButton("+");
	incr->setObjectName("obsSpinBoxButton");
	layout->addWidget(incr);

	connect(decr, &QPushButton::pressed, sbox, &QSpinBox::stepDown);
	connect(incr, &QPushButton::pressed, sbox, &QSpinBox::stepUp);
}
