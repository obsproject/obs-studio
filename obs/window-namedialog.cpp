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

#include "window-namedialog.hpp"
using namespace std;

void NameDialog::OnClose(wxCloseEvent &event)
{
	EndModal(wxID_CANCEL);
}

void NameDialog::OKPressed(wxCommandEvent &event)
{
	EndModal(wxID_OK);
}

void NameDialog::CancelPressed(wxCommandEvent &event)
{
	EndModal(wxID_CANCEL);
}

int NameDialog::AskForName(wxWindow *parent, const char *title,
		const char *text, string &str)
{
	NameDialog *dialog = new NameDialog(parent);
	dialog->SetTitle(wxString(title, wxConvUTF8));
	dialog->questionText->SetLabel(wxString(text, wxConvUTF8));

	int ret = dialog->ShowModal();
	str = dialog->nameEdit->GetValue().ToUTF8().data();
	return ret;
}
