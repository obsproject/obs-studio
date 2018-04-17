/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include "window-scenecollectiondialog.hpp"
#include "ui_SceneCollectionDialog.h"
#include "obs-app.hpp"

using namespace std;

SceneCollectionDialog::SceneCollectionDialog(QWidget *parent)
	: QDialog (parent),
	  ui      (new Ui::SceneCollectionDialog)
{
	ui->setupUi(this);

	installEventFilter(CreateShortcutFilter());
}

static bool IsWhitespace(char ch)
{
	return ch == ' ' || ch == '\t';
}

bool SceneCollectionDialog::AskForName(QWidget *parent, const QString &title,
		const QString &text, string &str, vector<string> &strings,
		const QString &placeHolder)
{
	SceneCollectionDialog dialog(parent);
	dialog.setWindowTitle(title);
	dialog.ui->label->setText(text);
	//dialog.ui->userText->lineEdit()->setText(placeHolder);
	//dialog.ui->userText->lineEdit()->selectAll();

	for (size_t idx = 0; idx < strings.size(); idx++) {
		dialog.ui->userText->addItem(strings[idx].c_str());
	}

	bool accepted = (dialog.exec() == DialogCode::Accepted);
	if (accepted) {
		str = dialog.ui->userText->currentText().toStdString();

		while (str.size() && IsWhitespace(str.back()))
			str.erase(str.end() - 1);
		while (str.size() && IsWhitespace(str.front()))
			str.erase(str.begin());
	}

	return accepted;
}
