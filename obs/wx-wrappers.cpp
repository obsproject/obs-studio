/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>
    Copyright (C) 2014 by Zachary Lund <admin@computerquip.com>
 
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

#include <wx/window.h>
#include <wx/msgdlg.h>
#include <obs.h>
#include "wx-wrappers.hpp"
#include <wx/utils.h>

#ifdef __WXGTK__
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif

#include <memory>
using namespace std;

gs_window WxToGSWindow(const wxWindow *wxwin)
{
	gs_window window;

#ifdef __APPLE__
	window.view     = (id)wxwin->GetHandle();
#elif _WIN32
	window.hwnd     = wxwin->GetHandle();
#else
	GdkWindow* gdkwin = gtk_widget_get_window(wxwin->GetHandle());
	window.id = GDK_DRAWABLE_XID(gdkwin);
	window.display = GDK_DRAWABLE_XDISPLAY(gdkwin);
#endif
	
	return window;
}

void OBSErrorBox(wxWindow *parent, const char *message, ...)
{
	va_list args;
	char output[4096];

	va_start(args, message);
	vsnprintf(output, 4095, message, args);
	va_end(args);

	wxMessageBox(message, "Error", wxOK|wxCENTRE, parent);
	blog(LOG_ERROR, "%s", output);
}

class MenuWrapper : public wxEvtHandler {
public:
	int retId;

	inline MenuWrapper() : retId(-1) {}

	void GetItem(wxCommandEvent &event)
	{
		retId = event.GetId();
	}
};

int WXDoPopupMenu(wxWindow *parent, wxMenu *menu)
{
	unique_ptr<MenuWrapper> wrapper(new MenuWrapper);

	menu->Connect(wxEVT_MENU,
			wxCommandEventHandler(MenuWrapper::GetItem),
			NULL, wrapper.get());

	bool success = parent->PopupMenu(menu);

	menu->Disconnect(wxEVT_MENU,
			wxCommandEventHandler(MenuWrapper::GetItem),
			NULL, wrapper.get());

	return (success) ? wrapper->retId : -1;
}
