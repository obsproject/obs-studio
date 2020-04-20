/******************************************************************************
    Copyright (C) 2020 by Hugh Bailey <obs.jim@gmail.com>

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

#include "qt-wrappers.hpp"
#include "ui_NameDialog.h"
#include "obs-app.hpp"
#include "helpwidget.hpp"
#include <QStyleOptionFocusRect>

using namespace std;

HelpWidget::HelpWidget(QWidget *parent, const char *helpText,
		       const char *fixButtonText, const char *helpButtonText)
	: QWidget(parent),
	  ui(new Ui::HelpWidget),
	  helpText(helpText),
	  fixButtonText(fixButtonText),
	  helpButtonText(helpButtonText)
{
	ui->setupUi(this);

	QPixmap pixmap = style()->standardIcon(QStyle::SP_MessageBoxWarning)
				 .pixmap(QSize(60, 60));
	ui->icon->setPixmap(pixmap);

	ui->labelHelpText->setText(QTStr(helpText));
	ui->fixButton->setText(QTStr(fixButtonText));
	ui->helpButton->setText(QTStr(helpButtonText));

	connect(ui->fixButton, &QAbstractButton::clicked, this,
		&HelpWidget::FixButtonClicked);

	connect(ui->helpButton, &QAbstractButton::clicked, this,
		&HelpWidget::HelpButtonClicked);

	adjustSize();
	setFixedSize(geometry().width(), geometry().height());
}

HelpWidget::HelpWidget(QWidget *parent, const char *helpText,
		       const char *fixButtonText)
	: HelpWidget(parent, helpText, fixButtonText,
		     "HelpWidget.HelpButtonText")
{
}
