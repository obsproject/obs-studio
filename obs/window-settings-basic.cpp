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

#include "window-settings-basic.hpp"

OBSBasicSettings::OBSBasicSettings(wxWindow *parent)
	: OBSBasicSettingsBase(parent)
{
	unique_ptr<BasicSettingsData> data(CreateBasicGeneralSettings(this));
	settings = move(data);
}

void OBSBasicSettings::PageChanged(wxListbookEvent &event)
{
}

void OBSBasicSettings::PageChanging(wxListbookEvent &event)
{
}

void OBSBasicSettings::OnClose(wxCloseEvent &event)
{
	Destroy();
}
