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

#include <wx/event.h>

#include "obs-app.hpp"
#include "settings-basic.hpp"
#include "window-settings-basic.hpp"
#include "wx-wrappers.hpp"
#include "platform.hpp"

class GeneralSettings : public BasicSettingsData {
	ConfigFile localeIni;
	WXConnector languageBoxConnector;

	void LanguageChanged(wxCommandEvent &event);

	int AddLanguage(const char *tag);
	void FillLanguageList(const char *currentLang);

public:
	GeneralSettings(OBSBasicSettings *window);

	virtual void Apply();
};

class LanguageInfo : public wxClientData {
public:
	const char *tag;
	const char *name;
	const char *subLang;
	bool       isDefault;

	inline LanguageInfo(config_t config, const char *tag)
		: wxClientData (),
		  tag          (tag),
		  name         (config_get_string(config, tag, "Name")),
		  subLang      (config_get_string(config, tag, "SubLang")),
		  isDefault    (config_get_bool(config, tag, "DefaultSubLang"))
	{
	}
};

int GeneralSettings::AddLanguage(const char *tag)
{
	LanguageInfo *info = new LanguageInfo(localeIni, tag);
	return window->languageList->Append(wxString(info->name, wxConvUTF8),
			info);
}

void GeneralSettings::FillLanguageList(const char *currentLang)
{
	size_t numSections = config_num_sections(localeIni);
	for (size_t i = 0; i < numSections; i++) {
		const char *lang = config_get_section(localeIni, i);
		int idx = AddLanguage(lang);

		if (strcmp(lang, currentLang) == 0)
			window->languageList->SetSelection(idx);
	}
}

GeneralSettings::GeneralSettings(OBSBasicSettings *window)
	: BasicSettingsData (window)
{
	string path;
	if (!GetDataFilePath("locale/locale.ini", path))
		throw "Could not find locale/locale.ini path";
	if (localeIni.Open(path.c_str(), CONFIG_OPEN_EXISTING) != 0)
		throw "Could not open locale.ini";

	const char *currentLang = config_get_string(GetGlobalConfig(),
			"General", "Language");
	FillLanguageList(currentLang);

	languageBoxConnector.Connect(
			window->languageList,
			wxEVT_COMBOBOX,
			wxCommandEventHandler(GeneralSettings::LanguageChanged),
			NULL,
			this);
}

void GeneralSettings::LanguageChanged(wxCommandEvent &event)
{
}

void GeneralSettings::Apply()
{
	
}

BasicSettingsData *CreateBasicGeneralSettings(OBSBasicSettings *window)
{
	BasicSettingsData *data = NULL;

	try {
		data = new GeneralSettings(window);
	} catch (const char *error) {
		blog(LOG_ERROR, "CreateBasicGeneralSettings failed: %s", error);
	}

	return data;
}
