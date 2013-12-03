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
#include <wx/frame.h>
#include <wx/listctrl.h>

/*
 * Fixes windows fonts to be default dialog fonts (I don't give a crap what
 * microsoft "recommends", the fonts they recommend look like utter garbage)
 */

#ifdef _
#undef _
#define _(str) str
#endif

class WindowSubclass : public wxFrame {
public:
	WindowSubclass(wxWindow* parent, wxWindowID id, const wxString& title,
			const wxPoint& pos, const wxSize& size, long style);
};

/*
 * To fix report view default sizing because it defaults to 10 * fontheight in
 * report view.  Why?  Who knows.
 */
class ListCtrlFixed : public wxListCtrl {
public:
	ListCtrlFixed(wxWindow *parent,
			wxWindowID id = wxID_ANY,
			const wxPoint& pos = wxDefaultPosition,
			const wxSize& size = wxDefaultSize,
			long style = wxLC_ICON,
			const wxValidator& validator = wxDefaultValidator,
			const wxString& name = wxListCtrlNameStr);

	virtual wxSize DoGetBestClientSize() const;
};
