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

#include <util/lexer.h>

#include "obs-app.hpp"
#include "settings-basic.hpp"
#include "window-settings-basic.hpp"
#include "wx-wrappers.hpp"
#include "platform.hpp"

#include <sstream>
#include <unordered_set>
using namespace std;

class BasicVideoData : public BasicSettingsData {
	ConnectorList connections;

	int AddRes(uint32_t cx, uint32_t cy);
	void LoadOther();
	void LoadResolutionData();
	void LoadFPSData();
	void LoadFPSCommon();
	void LoadFPSInteger();
	void LoadFPSFraction();
	void LoadFPSNanoseconds();
	void ResetScaleList(uint32_t cx, uint32_t cy);

	void BaseResListChanged(wxCommandEvent &event);
	void OutputResListChanged(wxCommandEvent &event);

	void SaveOther();
	void SaveFPSData();
	void SaveFPSCommon();
	void SaveFPSInteger();
	void SaveFPSFraction();
	void SaveFPSNanoseconds();

public:
	BasicVideoData(OBSBasicSettings *window);

	void Apply();
};

struct BaseLexer {
	lexer lex;
public:
	inline BaseLexer() {lexer_init(&lex);}
	inline ~BaseLexer() {lexer_free(&lex);}
	operator lexer*() {return &lex;}
};

/* parses "[width]x[height]", string, i.e. 1024x768 */
static bool ConvertTextRes(wxComboBox *combo, uint32_t &cx, uint32_t &cy)
{
	string str = combo->GetValue().ToStdString();

	BaseLexer lex;
	lexer_start(lex, str.c_str());

	base_token token;

	/* parse width */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != BASETOKEN_DIGIT)
		return false;

	cx = std::stoul(token.text.array);

	/* parse 'x' */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (strref_cmpi(&token.text, "x") != 0)
		return false;

	/* parse height */
	if (!lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;
	if (token.type != BASETOKEN_DIGIT)
		return false;

	cy = std::stoul(token.text.array);

	/* shouldn't be any more tokens after this */
	if (lexer_getbasetoken(lex, &token, IGNORE_WHITESPACE))
		return false;

	return true;
}

int BasicVideoData::AddRes(uint32_t cx, uint32_t cy)
{
	stringstream res;
	res << cx << "x" << cy;
	return window->baseResList->Append(res.str().c_str());
}

void BasicVideoData::LoadOther()
{
	const char *renderer = config_get_string(GetGlobalConfig(), "Video",
			"Renderer");

	window->rendererList->Clear();
	window->rendererList->Append("OpenGL");
#ifdef _WIN32
	window->rendererList->Append("Direct3D 11");
#endif

	int sel = window->rendererList->FindString(renderer);
	if (sel == wxNOT_FOUND)
		sel = 0;

	window->rendererList->SetSelection(sel);
}

static uint64_t append_uint32_t(uint64_t first, uint64_t second)
{
	return (first << 32) | second;
}

void BasicVideoData::LoadResolutionData()
{
	window->baseResList->Clear();

	uint32_t cx = config_get_uint(GetGlobalConfig(), "Video", "BaseCX");
	uint32_t cy = config_get_uint(GetGlobalConfig(), "Video", "BaseCY");

	vector<MonitorInfo> monitors;
	GetMonitors(monitors);
	unordered_set<uint64_t> resolutions;
	for (size_t i = 0; i < monitors.size(); i++) {
		uint64_t res = append_uint32_t(monitors[i].cx, monitors[i].cy);
		if(resolutions.emplace(res).second)
			AddRes(monitors[i].cx, monitors[i].cy);
	}

	stringstream res;
	res << cx << "x" << cy;
	window->baseResList->SetValue(res.str());

	ResetScaleList(cx, cy);

	cx = config_get_uint(GetGlobalConfig(), "Video", "OutputCX");
	cy = config_get_uint(GetGlobalConfig(), "Video", "OutputCY");

	res.str(string());
	res.clear();
	res << cx << "x" << cy;
	window->outputResList->SetValue(res.str());
}

void BasicVideoData::LoadFPSData()
{
	const char *fpsType = config_get_string(GetGlobalConfig(), "Video",
			"FPSType");

	LoadFPSCommon();
	LoadFPSInteger();
	LoadFPSFraction();
	LoadFPSNanoseconds();

	if (!fpsType)
		return;
	else if (strcmp(fpsType, "Common") == 0)
		window->fpsTypeList->SetSelection(0);
	else if (strcmp(fpsType, "Integer") == 0)
		window->fpsTypeList->SetSelection(1);
	else if (strcmp(fpsType, "Fraction") == 0)
		window->fpsTypeList->SetSelection(2);
	else if (strcmp(fpsType, "Nanoseconds") == 0)
		window->fpsTypeList->SetSelection(3);
}

void BasicVideoData::LoadFPSCommon()
{
	const char *str = config_get_string(GetGlobalConfig(), "Video",
			"FPSCommon");

	int val = 3;
	if (strcmp(str, "10") == 0)
		val = 0;
	else if (strcmp(str, "20") == 0)
		val = 1;
	else if (strcmp(str, "29.97") == 0)
		val = 2;
	else if (strcmp(str, "30") == 0)
		val = 3;
	else if (strcmp(str, "48") == 0)
		val = 4;
	else if (strcmp(str, "59.94") == 0)
		val = 5;
	else if (strcmp(str, "60") == 0)
		val = 6;

	window->fpsCommonList->SetSelection(val);
}

void BasicVideoData::LoadFPSInteger()
{
	window->fpsTypeList->SetSelection(1);

	int val = config_get_int(GetGlobalConfig(), "Video", "FPSInt");
	window->fpsIntegerScroller->SetValue(val);
}

void BasicVideoData::LoadFPSFraction()
{
	window->fpsTypeList->SetSelection(2);

	int num = config_get_int(GetGlobalConfig(), "Video", "FPSNum");
	int den = config_get_int(GetGlobalConfig(), "Video", "FPSDen");

	window->fpsNumeratorScroller->SetValue(num);
	window->fpsDenominatorScroller->SetValue(den);
}

void BasicVideoData::LoadFPSNanoseconds()
{
	window->fpsTypeList->SetSelection(3);

	int val = config_get_int(GetGlobalConfig(), "Video", "FPSNS");
	window->fpsNanosecondsScroller->SetValue(val);
}

/* some nice default output resolution vals */
static const double vals[] =
{
	1.0,
	1.25,
	(1.0/0.75),
	1.5,
	(1.0/0.6),
	1.75,
	2.0,
	2.25,
	2.5,
	2.75,
	3.0
};

static const size_t numVals = sizeof(vals)/sizeof(double);

void BasicVideoData::ResetScaleList(uint32_t cx, uint32_t cy)
{
	window->outputResList->Clear();

	for (size_t i = 0; i < numVals; i++) {
		stringstream res;
		res << uint32_t(double(cx) / vals[i]);
		res << "x";
		res << uint32_t(double(cy) / vals[i]);
		window->outputResList->Append(res.str().c_str());
	}
}

BasicVideoData::BasicVideoData(OBSBasicSettings *window)
	: BasicSettingsData(window)
{
	LoadResolutionData();
	LoadFPSData();
	LoadOther();

	/* load connectors after loading data to prevent them from triggering */
	connections.Add(window->baseResList, wxEVT_TEXT,
			wxCommandEventHandler(
				BasicVideoData::BaseResListChanged),
			NULL, this);
	connections.Add(window->outputResList, wxEVT_TEXT,
			wxCommandEventHandler(
				BasicVideoData::OutputResListChanged),
			NULL, this);

	window->videoChangedText->Hide();
}

void BasicVideoData::BaseResListChanged(wxCommandEvent &event)
{
	uint32_t cx, cy;
	if (!ConvertTextRes(window->baseResList, cx, cy)) {
		window->videoChangedText->SetLabel(
				WXStr("Settings.Video.InvalidResolution"));
		window->videoChangedText->Show();
		return;
	}

	dataChanged = true;
	window->videoChangedText->SetLabel(WXStr("Settings.StreamRestart"));
	window->videoChangedText->Show();

	ResetScaleList(cx, cy);
}

void BasicVideoData::OutputResListChanged(wxCommandEvent &event)
{
	uint32_t cx, cy;
	if (!ConvertTextRes(window->outputResList, cx, cy)) {
		window->videoChangedText->SetLabel(
				WXStr("Settings.Video.InvalidResolution"));
		window->videoChangedText->Show();
		return;
	}

	dataChanged = true;
	window->videoChangedText->SetLabel(WXStr("Settings.StreamRestart"));
	window->videoChangedText->Show();
}

void BasicVideoData::SaveOther()
{
	int sel = window->rendererList->GetSelection();
	if (sel == wxNOT_FOUND)
		return;

	wxString renderer = window->rendererList->GetString(sel);
	config_set_string(GetGlobalConfig(), "Video", "Renderer",
			renderer.c_str());
}

void BasicVideoData::SaveFPSData()
{
	int id = window->fpsTypeList->GetCurrentPage()->GetId();

	const char *type;
	switch (id) {
	case ID_FPSPANEL_COMMON:      type = "Common"; break;
	case ID_FPSPANEL_INTEGER:     type = "Integer"; break;
	case ID_FPSPANEL_FRACTION:    type = "Fraction"; break;
	case ID_FPSPANEL_NANOSECONDS: type = "Nanoseconds"; break;
	}

	config_set_string(GetGlobalConfig(), "Video", "FPSType", type);
	SaveFPSCommon();
	SaveFPSInteger();
	SaveFPSFraction();
	SaveFPSNanoseconds();
}

void BasicVideoData::SaveFPSCommon()
{
	int sel = window->fpsCommonList->GetSelection();
	wxString str = window->fpsCommonList->GetString(sel);

	config_set_string(GetGlobalConfig(), "Video", "FPSCommon", str.c_str());
}

void BasicVideoData::SaveFPSInteger()
{
	int val = window->fpsIntegerScroller->GetValue();

	config_set_int(GetGlobalConfig(), "Video", "FPSInt", val);
}

void BasicVideoData::SaveFPSFraction()
{
	int num = window->fpsNumeratorScroller->GetValue();
	int den = window->fpsDenominatorScroller->GetValue();

	config_set_int(GetGlobalConfig(), "Video", "FPSNum", num);
	config_set_int(GetGlobalConfig(), "Video", "FPSDen", den);
}

void BasicVideoData::SaveFPSNanoseconds()
{
	int val = window->fpsNanosecondsScroller->GetValue();

	config_set_int(GetGlobalConfig(), "Video", "FPSNS", val);
}

void BasicVideoData::Apply()
{
	uint32_t cx, cy;

	if (!ConvertTextRes(window->baseResList, cx, cy)) {
		config_set_uint(GetGlobalConfig(), "Video", "BaseCX", cx);
		config_set_uint(GetGlobalConfig(), "Video", "BaseCY", cy);
	}

	if (ConvertTextRes(window->outputResList, cx, cy)) {
		config_set_uint(GetGlobalConfig(), "Video", "OutputCX", cx);
		config_set_uint(GetGlobalConfig(), "Video", "OutputCY", cy);
	}

	SaveOther();
	SaveFPSData();
}

BasicSettingsData *CreateBasicVideoSettings(OBSBasicSettings *window)
{
	return new BasicVideoData(window);
}
