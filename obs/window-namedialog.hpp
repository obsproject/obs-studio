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

#pragma once

#include "forms/OBSWindows.h"

#include <vector>

class NameDialog : public NameDialogBase {
protected:
	virtual void OnClose(wxCommandEvent &event);
	virtual void OKPressed(wxCommandEvent &event);
	virtual void CancelPressed(wxCommandEvent &event);

public:
	inline NameDialog(wxWindow *parent)
		: NameDialogBase(parent)
	{
	}

	static int AskForName(wxWindow *parent, const char *title,
			const char *text, std::string &str);
};
