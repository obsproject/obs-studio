/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "window-subclass.hpp"

#ifndef WX_PRECOMP
    #include <wx/dcclient.h>
#endif

#ifdef _WIN32
#include <wx/fontutil.h>

#define WIN32_MEAN_AND_LEAN
#include <windows.h>

/* copied from the wx windows-specific stuff */
wxFont wxCreateFontFromStockObject2(int index)
{
	wxFont font;

	HFONT hFont = (HFONT)::GetStockObject(index);
	if (hFont) {
		LOGFONT lf;
		if (::GetObject(hFont, sizeof(LOGFONT), &lf) != 0) {
			wxNativeFontInfo info;
			info.lf = lf;
#ifdef __WXMICROWIN__
			font.Create(info, (WXHFONT)hFont);
#else
			font.Create(info);
#endif
		} else {
			wxFAIL_MSG(wxT("failed to get LOGFONT"));
		}
	} else {
		wxFAIL_MSG(wxT("stock font not found"));
	}

	return font;
}
#endif

WindowSubclass::WindowSubclass(wxWindow* parent, wxWindowID id,
		const wxString& title, const wxPoint& pos, const wxSize& size,
		long style)
	: wxFrame(parent, id, title, pos, size, style)
{
#ifdef _WIN32
	this->SetFont(wxFont(wxCreateFontFromStockObject2(DEFAULT_GUI_FONT)));
#endif
}

ListCtrlFixed::ListCtrlFixed(wxWindow *parent,
		wxWindowID id,
		const wxPoint& pos,
		const wxSize& size,
		long style,
		const wxValidator& validator,
		const wxString& name)
	: wxListCtrl(parent, id, pos, size, style, validator, name)
{
	m_bestSizeCache.Set(wxDefaultCoord, wxDefaultCoord);
	SetInitialSize(size);
}

wxSize ListCtrlFixed::DoGetBestClientSize() const
{
	if (!InReportView())
		return wxControl::DoGetBestClientSize();

	int totalWidth;
	wxClientDC dc(const_cast<ListCtrlFixed*>(this));

	const int columns = GetColumnCount();
	if (HasFlag(wxLC_NO_HEADER) || !columns) {
		totalWidth = 50*dc.GetCharWidth();
	} else {
		totalWidth = 0;
		for ( int col = 0; col < columns; col++ )
			totalWidth += GetColumnWidth(col);
	}

	/*
	 * This is what we're fixing.  Some..  very foolish person decided,
	 * "Oh, let's give this an 'arbitrary' height!  How about, let's see,
	 * I don't know!  LET'S USE 10 * FONT HEIGHT!"  ..Unfortunately, this
	 * person basically makes it impossible to use smaller sized list
	 * views in report mode.  It will always become tremendously large in
	 * size, despite what constrains you originally have set with sizers.
	 * brilliant job, whoever you are.  10 * character height..  just..
	 * unbeleivably wow.  I am ASTOUNDED.
	 */
	return wxSize(totalWidth, 3*dc.GetCharHeight());
}
