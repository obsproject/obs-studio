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

#include <util/bmem.h>

#include "obs-app.hpp"
#include "settings-basic.hpp"
#include "window-settings-basic.hpp"
#include "wx-wrappers.hpp"
#include "platform.hpp"

class BasicGenData : public BasicSettingsData {
	ConfigFile localeIni;
	WXConnector languageBoxConnector;

	void LanguageChanged(wxCommandEvent &event);

	int AddLanguage(const char *tag);
	void FillLanguageList(const char *currentLang);

public:
	BasicGenData(OBSBasicSettings *window);

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

int BasicGenData::AddLanguage(const char *tag)
{
	LanguageInfo *info = new LanguageInfo(localeIni, tag);
	return window->languageList->Append(wxString(info->name, wxConvUTF8),
			info);
}

void BasicGenData::FillLanguageList(const char *currentLang)
{
	window->languageList->Clear();

	size_t numSections = config_num_sections(localeIni);
	for (size_t i = 0; i < numSections; i++) {
		const char *lang = config_get_section(localeIni, i);
		int idx = AddLanguage(lang);

		if (strcmp(lang, currentLang) == 0)
			window->languageList->SetSelection(idx);
	}
}

BasicGenData::BasicGenData(OBSBasicSettings *window)
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
			wxCommandEventHandler(BasicGenData::LanguageChanged),
			NULL,
			this);

	window->generalChangedText->Hide();
}

void BasicGenData::LanguageChanged(wxCommandEvent &event)
{
	SetChanged();
	window->generalChangedText->SetLabel(
			WXStr("Settings.General.LanguageChanged"));
	window->generalChangedText->Show();
}

void BasicGenData::Apply()
{
	int sel = window->languageList->GetSelection();
	if (sel == wxNOT_FOUND)
		return;

	LanguageInfo *info = static_cast<LanguageInfo*>(
			window->languageList->GetClientObject(sel));

	config_set_string(GetGlobalConfig(), "General", "Language", info->tag);

	config_save(GetGlobalConfig());

	SetSaved();
}

BasicSettingsData *CreateBasicGeneralSettings(OBSBasicSettings *window)
{
	BasicSettingsData *data = NULL;

	try {
		data = new BasicGenData(window);
	} catch (const char *error) {
		blog(LOG_ERROR, "CreateBasicGeneralSettings failed: %s", error);
	}

	return data;
}
