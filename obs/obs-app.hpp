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

#include <wx/app.h>

#include <util/util.hpp>

class OBSAppBase : public wxApp {
public:
	virtual ~OBSAppBase();
};

class OBSApp : public OBSAppBase {
	ConfigFile globalConfig;
	TextLookup textLookup;

	bool InitGlobalConfig();
	bool InitGlobalConfigDefaults();
	bool InitConfigDefaults();
	bool InitLocale();

public:
	virtual bool OnInit();
	virtual int  OnExit();
	virtual void CleanUp();

	inline config_t GlobalConfig() {return globalConfig;}

	inline const char *GetString(const char *lookupVal)
	{
		return textLookup.GetString(lookupVal);
	}
};

wxDECLARE_APP(OBSApp);

inline config_t GetGlobalConfig() {return wxGetApp().GlobalConfig();}

#define Str(lookupVal) wxGetApp().GetString(lookupVal)
#define WXStr(lookupVal) wxString(Str(lookupVal), wxConvUTF8)
