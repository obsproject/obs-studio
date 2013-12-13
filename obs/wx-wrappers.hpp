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

#include <wx/window.h>
#include <wx/event.h>

struct gs_window;

gs_window WxToGSWindow(const wxWindow *window);
void OBSErrorBox(wxWindow *parent, const char *message, ...);

/*
 * RAII wx connection wrapper
 *
 *   Automatically disconnects events on destruction rather than having to
 * manually call Disconnect for every Connect.
 */

class WXConnector {
	wxEvtHandler          *obj;
	wxEventType           eventType;
	wxObjectEventFunction func;
	wxObject              *userData;
	wxEvtHandler          *eventSink;

public:
	inline WXConnector()
		: obj       (NULL),
		  eventType (0),
		  func      (NULL),
		  userData  (NULL),
		  eventSink (NULL)
	{
	}

	inline WXConnector(wxEvtHandler *obj, wxEventType eventType,
			wxObjectEventFunction func, wxObject *userData,
			wxEvtHandler *eventSink)
		: obj       (obj),
		  eventType (eventType),
		  func      (func),
		  userData  (userData),
		  eventSink (eventSink)
	{
		obj->Connect(eventType, func, userData, eventSink);
	}

	inline ~WXConnector()
	{
		Disconnect();
	}

	inline void Connect(wxEvtHandler *obj, wxEventType eventType,
			wxObjectEventFunction func, wxObject *userData,
			wxEvtHandler *eventSink)
	{
		Disconnect();

		this->obj       = obj;
		this->eventType = eventType;
		this->func      = func;
		this->userData  = userData;
		this->eventSink = eventSink;

		obj->Connect(eventType, func, userData, eventSink);
	}

	inline void Disconnect()
	{
		if (obj)
			obj->Disconnect(eventType, func, userData, eventSink);
		obj = NULL;
	}
};
