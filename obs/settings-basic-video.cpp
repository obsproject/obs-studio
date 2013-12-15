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

#include "obs-app.hpp"
#include "settings-basic.hpp"
#include "window-settings-basic.hpp"
#include "wx-wrappers.hpp"

class BasicVideoData : public BasicSettingsData {
	ConnectorList connections;

	void BaseResListChanged(wxCommandEvent &event);

public:
	BasicVideoData(OBSBasicSettings *window);

	void Apply();
};

BasicVideoData::BasicVideoData(OBSBasicSettings *window)
	: BasicSettingsData(window)
{
	connections.Add(window->baseResList, wxEVT_TEXT,
			wxCommandEventHandler(
				BasicVideoData::BaseResListChanged),
			NULL, this);

	window->baseResList->Clear();
	window->baseResList->Append("640x480");
	window->baseResList->Append("800x600");
	window->baseResList->Append("1024x768");
	window->baseResList->Append("1280x720");
	window->baseResList->Append("1920x1080");
}

void BasicVideoData::BaseResListChanged(wxCommandEvent &event)
{
}

void BasicVideoData::Apply()
{
}

BasicSettingsData *CreateBasicVideoSettings(OBSBasicSettings *window)
{
	BasicSettingsData *data = NULL;

	try {
		data = new BasicVideoData(window);
	} catch (const char *error) {
		blog(LOG_ERROR, "CreateBasicVideoSettings failed: %s", error);
	}

	return data;
}
