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

#include <vector>
using namespace std;

class BasicVideoData : public BasicSettingsData {
	ConnectorList connections;

	void BaseResListChanged(wxCommandEvent &event);

public:
	BasicVideoData(OBSBasicSettings *window);
};

BasicVideoData::BasicVideoData(OBSBasicSettings *window)
	: BasicSettingsData(window)
{
	connections.Add(window->baseResList, wxEVT_COMBOBOX,
			wxCommandEventHandler(
				BasicVideoData::BaseResListChanged),
			NULL, this);
}

void BasicVideoData::BaseResListChanged(wxCommandEvent &event)
{
}
