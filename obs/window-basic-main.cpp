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

#include <obs.hpp>

#include <wx/msgdlg.h>

#include "obs-app.hpp"
#include "wx-wrappers.hpp"
#include "window-basic-settings.hpp"
#include "window-basic-main.hpp"
#include "window-namedialog.hpp"
using namespace std;

void OBSBasic::SceneAdded(obs_source_t source)
{
	const char *name  = obs_source_getname(source);
	obs_scene_t scene = obs_scene_fromsource(source);
	scenes->Append(wxString(name, wxConvUTF8), scene);
}

void OBSBasic::SceneRemoved(obs_source_t source)
{
	const char *name = obs_source_getname(source);

	int item = scenes->FindString(name);
	if (item == wxNOT_FOUND) {
		blog(LOG_WARNING, "Tried to remove the scene '%s', but "
		                  "apparently it wasn't found", name);
		return;
	}

	scenes->Delete(item);
}

void OBSBasic::SourceAdded(void *data, calldata_t params)
{
	OBSBasic *window = (OBSBasic*)data;

	obs_source_t source;
	calldata_getptr(params, "source", (void**)&source);

	obs_source_type type;
	obs_source_gettype(source, &type, NULL);

	if (type == SOURCE_SCENE)
		window->SceneAdded(source);
}

void OBSBasic::SourceDestroyed(void *data, calldata_t params)
{
	OBSBasic *window = (OBSBasic*)data;

	obs_source_t source;
	calldata_getptr(params, "source", (void**)&source);

	obs_source_type type;
	obs_source_gettype(source, &type, NULL);

	if (type == SOURCE_SCENE)
		window->SceneRemoved(source);
}

bool OBSBasic::Init()
{
	if (!obs_startup())
		return false;
	if (!InitGraphics())
		return false;

	signal_handler_connect(obs_signalhandler(), "source-add",
			OBSBasic::SourceAdded, this);
	signal_handler_connect(obs_signalhandler(), "source-destroy",
			OBSBasic::SourceDestroyed, this);

	//obs_scene_t scene = obs_scene_create("test scene");
	//obs_add_source(obs_scene_getsource(scene));

	//obs_load_module("test-input");

	return true;
}

OBSBasic::~OBSBasic()
{
	obs_shutdown();
}

bool OBSBasic::InitGraphics()
{
	wxSize size = previewPanel->GetClientSize();

	struct obs_video_info ovi;
	wxGetApp().GetConfigFPS(ovi.fps_num, ovi.fps_den);
	ovi.graphics_module = wxGetApp().GetRenderModule();
	ovi.window_width  = size.x;
	ovi.window_height = size.y;
	ovi.base_width    = (uint32_t)config_get_uint(GetGlobalConfig(),
			"Video", "BaseCX");
	ovi.base_height   = (uint32_t)config_get_uint(GetGlobalConfig(),
			"Video", "BaseCY");
	ovi.output_width  = (uint32_t)config_get_uint(GetGlobalConfig(),
			"Video", "OutputCX");
	ovi.output_height = (uint32_t)config_get_uint(GetGlobalConfig(),
			"Video", "OutputCY");
	ovi.output_format = VIDEO_FORMAT_RGBA;
	ovi.adapter       = 0;
	ovi.window        = WxToGSWindow(previewPanel);

	if (!obs_reset_video(&ovi))
		return false;

	//required to make opengl display stuff on osx(?)
	SendSizeEvent();

	return true;
}

void OBSBasic::OnClose(wxCloseEvent &event)
{
	wxGetApp().ExitMainLoop();
	event.Skip();
}

void OBSBasic::OnMinimize(wxIconizeEvent &event)
{
	event.Skip();
}

void OBSBasic::OnSize(wxSizeEvent &event)
{
	struct obs_video_info ovi;

	event.Skip();

	if (!obs_get_video_info(&ovi))
		return;

	/* resize preview panel to fix to the top section of the window */
	wxSize targetSize   = GetPreviewContainer()->GetSize();
	double targetAspect = double(targetSize.x) / double(targetSize.y);
	double baseAspect   = double(ovi.base_width) / double(ovi.base_height);

	if (targetAspect > baseAspect)
		GetPreviewPanel()->SetMinSize(wxSize(targetSize.y * baseAspect,
				targetSize.y));
	else
		GetPreviewPanel()->SetMinSize(wxSize(targetSize.x,
				targetSize.x / baseAspect));
}

void OBSBasic::fileNewClicked(wxCommandEvent &event)
{
}

void OBSBasic::fileOpenClicked(wxCommandEvent &event)
{
}

void OBSBasic::fileSaveClicked(wxCommandEvent &event)
{
}

void OBSBasic::fileExitClicked(wxCommandEvent &event)
{
	wxGetApp().ExitMainLoop();
}

void OBSBasic::scenesRDown(wxMouseEvent &event)
{
}

void OBSBasic::sceneAddClicked(wxCommandEvent &event)
{
	string name;
	int ret = NameDialog::AskForName(this,
			Str("MainWindow.AddSceneDlg.Title"),
			Str("MainWindow.AddSceneDlg.Text"),
			name);

	if (ret == wxID_OK) {
		obs_source_t source = obs_get_source_by_name(name.c_str());
		if (source) {
			wxMessageBox(WXStr("MainWindow.NameExists.Text"),
			             WXStr("MainWindow.NameExists.Title"),
			             wxOK|wxCENTRE, this);

			obs_source_release(source);
			sceneAddClicked(event);
			return;
		}

		obs_scene_t scene = obs_scene_create(name.c_str());
		obs_add_source(obs_scene_getsource(scene));
		obs_scene_release(scene);
	}
}

void OBSBasic::sceneRemoveClicked(wxCommandEvent &event)
{
}

void OBSBasic::scenePropertiesClicked(wxCommandEvent &event)
{
}

void OBSBasic::sceneUpClicked(wxCommandEvent &event)
{
}

void OBSBasic::sceneDownClicked(wxCommandEvent &event)
{
}

void OBSBasic::sourcesRDown(wxMouseEvent &event)
{
}

void OBSBasic::sourceAddClicked(wxCommandEvent &event)
{
}

void OBSBasic::sourceRemoveClicked(wxCommandEvent &event)
{
}

void OBSBasic::sourcePropertiesClicked(wxCommandEvent &event)
{
}

void OBSBasic::sourceUpClicked(wxCommandEvent &event)
{
}

void OBSBasic::sourceDownClicked(wxCommandEvent &event)
{
}

void OBSBasic::settingsClicked(wxCommandEvent &event)
{
	OBSBasicSettings test(this);
	test.ShowModal();
}

void OBSBasic::exitClicked(wxCommandEvent &event)
{
	wxGetApp().ExitMainLoop();
}
