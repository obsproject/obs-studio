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

obs_scene_t OBSBasic::GetCurrentScene()
{
	int sel = scenes->GetSelection();
	if (sel == wxNOT_FOUND)
		return NULL;

	return (obs_scene_t)scenes->GetClientData(sel);
}

void OBSBasic::AddScene(obs_source_t source)
{
	const char *name  = obs_source_getname(source);
	obs_scene_t scene = obs_scene_fromsource(source);
	scenes->Append(WX_UTF8(name), scene);

	signal_handler_t handler = obs_source_signalhandler(source);
	signal_handler_connect(handler, "add", OBSBasic::SceneItemAdded,
			this);
	signal_handler_connect(handler, "remove", OBSBasic::SceneItemRemoved,
			this);
}

void OBSBasic::RemoveScene(obs_source_t source)
{
	const char *name = obs_source_getname(source);

	int idx = scenes->FindString(name);
	if (idx != wxNOT_FOUND)
		scenes->Delete(idx);
}

void OBSBasic::AddSceneItem(obs_sceneitem_t item)
{
	obs_source_t source = obs_sceneitem_getsource(item);
	const char *name = obs_source_getname(source);
	sources->Insert(WX_UTF8(name), 0, item);
}

void OBSBasic::RemoveSceneItem(obs_sceneitem_t item)
{
	for (unsigned int idx = 0; idx < sources->GetCount(); idx++) {
		obs_sceneitem_t curItem;
		curItem = (obs_sceneitem_t)sources->GetClientData(idx);

		if (item == curItem) {
			sources->Delete(idx);
			break;
		}
	}
}

void OBSBasic::UpdateSources(obs_scene_t scene)
{
	sources->Clear();

	obs_scene_enum_items(scene,
			[] (obs_scene_t scene, obs_sceneitem_t item, void *p)
			{
				OBSBasic *window = static_cast<OBSBasic*>(p);
				window->AddSceneItem(item);
				return true;
			}, this);
}

void OBSBasic::UpdateSceneSelection(obs_source_t source)
{
	if (source) {
		obs_source_type type;
		obs_source_gettype(source, &type, NULL);

		if (type == SOURCE_SCENE) {
			obs_scene_t scene = obs_scene_fromsource(source);
			const char *name = obs_source_getname(source);
			int idx = scenes->FindString(WX_UTF8(name));
			int sel = scenes->GetSelection();

			if (idx != sel) {
				scenes->SetSelection(idx);
				UpdateSources(scene);
			}
		}
	} else {
		scenes->SetSelection(wxNOT_FOUND);
	}
}

/* OBS Callbacks */

void OBSBasic::SceneItemAdded(void *data, calldata_t params)
{
	OBSBasic *window = static_cast<OBSBasic*>(data);

	obs_scene_t scene = (obs_scene_t)calldata_ptr(params, "scene");
	obs_sceneitem_t item = (obs_sceneitem_t)calldata_ptr(params, "item");

	if (window->GetCurrentScene() == scene)
		window->AddSceneItem(item);
}

void OBSBasic::SceneItemRemoved(void *data, calldata_t params)
{
	OBSBasic *window = static_cast<OBSBasic*>(data);

	obs_scene_t scene = (obs_scene_t)calldata_ptr(params, "scene");
	obs_sceneitem_t item = (obs_sceneitem_t)calldata_ptr(params, "item");

	if (window->GetCurrentScene() == scene)
		window->RemoveSceneItem(item);
}

void OBSBasic::SourceAdded(void *data, calldata_t params)
{
	obs_source_t source = (obs_source_t)calldata_ptr(params, "source");

	obs_source_type type;
	obs_source_gettype(source, &type, NULL);

	if (type == SOURCE_SCENE)
		static_cast<OBSBasic*>(data)->AddScene(source);
}

void OBSBasic::SourceDestroyed(void *data, calldata_t params)
{
	obs_source_t source = (obs_source_t)calldata_ptr(params, "source");

	obs_source_type type;
	obs_source_gettype(source, &type, NULL);

	if (type == SOURCE_SCENE)
		static_cast<OBSBasic*>(data)->RemoveScene(source);
}

void OBSBasic::ChannelChanged(void *data, calldata_t params)
{
	obs_source_t source = (obs_source_t)calldata_ptr(params, "source");
	uint32_t channel = calldata_uint32(params, "channel");

	if (channel == 0)
		static_cast<OBSBasic*>(data)->UpdateSceneSelection(source);
}

/* Main class functions */

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
	signal_handler_connect(obs_signalhandler(), "channel-change",
			OBSBasic::ChannelChanged, this);

	/* TODO: this is a test */
	obs_load_module("test-input");

	return true;
}

OBSBasic::~OBSBasic()
{
	obs_shutdown();
}

bool OBSBasic::InitGraphics()
{
	struct obs_video_info ovi;
	wxGetApp().GetConfigFPS(ovi.fps_num, ovi.fps_den);
	ovi.graphics_module = wxGetApp().GetRenderModule();
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

	//required to make opengl display stuff on osx(?)
	ResizePreview(ovi.base_width, ovi.base_height);
	SendSizeEvent();

	wxSize size = previewPanel->GetClientSize();
	ovi.window_width  = size.x;
	ovi.window_height = size.y;

	if (!obs_reset_video(&ovi))
		return false;

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

void OBSBasic::ResizePreview(uint32_t cx, uint32_t cy)
{
	/* resize preview panel to fix to the top section of the window */
	wxSize targetSize   = GetPreviewContainer()->GetSize();
	double targetAspect = double(targetSize.x) / double(targetSize.y);
	double baseAspect   = double(cx) / double(cy);
	wxSize newSize;

	if (targetAspect > baseAspect)
		newSize = wxSize(targetSize.y * baseAspect, targetSize.y);
	else
		newSize = wxSize(targetSize.x, targetSize.x / baseAspect);

	GetPreviewPanel()->SetMinSize(newSize);
}

void OBSBasic::OnSize(wxSizeEvent &event)
{
	struct obs_video_info ovi;

	event.Skip();

	if (!obs_get_video_info(&ovi))
		return;

	ResizePreview(ovi.base_width, ovi.base_height);
}

void OBSBasic::OnResizePreview(wxSizeEvent &event)
{
	event.Skip();

	wxSize newSize = previewPanel->GetClientSize();

	graphics_t graphics = obs_graphics();
	if (graphics) {
		gs_entercontext(graphics);
		gs_resize(newSize.x, newSize.y);
		gs_leavecontext();
	}
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

void OBSBasic::scenesClicked(wxCommandEvent &event)
{
	int sel = scenes->GetSelection();

	obs_source_t source = NULL;
	if (sel != wxNOT_FOUND) {
		obs_scene_t scene = (obs_scene_t)scenes->GetClientData(sel);
		source = obs_scene_getsource(scene);
		UpdateSources(scene);
	}

	/* TODO: allow transitions */
	obs_set_output_source(0, source);
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
		source = obs_scene_getsource(scene);
		obs_add_source(source);
		obs_scene_release(scene);

		obs_set_output_source(0, source);
	}
}

void OBSBasic::sceneRemoveClicked(wxCommandEvent &event)
{
	int sel = scenes->GetSelection();
	if (sel == wxNOT_FOUND)
		return;

	obs_scene_t scene = (obs_scene_t)scenes->GetClientData(sel);
	obs_source_t source = obs_scene_getsource(scene);
	obs_source_remove(source);
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

void OBSBasic::sourcesClicked(wxCommandEvent &event)
{
}

void OBSBasic::sourcesToggled(wxCommandEvent &event)
{
}

void OBSBasic::sourcesRDown(wxMouseEvent &event)
{
}

void OBSBasic::AddSource(obs_scene_t scene, const char *id)
{
	string name;

	bool success = false;
	while (!success) {
		int ret = NameDialog::AskForName(this,
				Str("MainWindow.AddSourceDlg.Title"),
				Str("MainWindow.AddSourceDlg.Text"),
				name);

		if (ret == wxID_CANCEL)
			break;

		obs_source_t source = obs_get_source_by_name(
				name.c_str());
		if (!source) {
			success = true;
		} else {
			wxMessageBox(WXStr("MainWindow.NameExists.Text"),
			             WXStr("MainWindow.NameExists.Title"),
			             wxOK|wxCENTRE, this);
			obs_source_release(source);
		}
	}

	if (success) {
		obs_source_t source = obs_source_create(SOURCE_INPUT, id,
				name.c_str(), NULL);
		obs_add_source(source);
		obs_sceneitem_t item = obs_scene_add(scene, source);
		obs_source_release(source);
	}
}

void OBSBasic::AddSourcePopupMenu()
{
	int sceneSel = scenes->GetSelection();
	size_t idx = 0;
	const char *type;
	vector<const char *> types;

	if (sceneSel == wxNOT_FOUND)
		return;

	obs_scene_t scene = (obs_scene_t)scenes->GetClientData(sceneSel);
	obs_scene_addref(scene);

	unique_ptr<wxMenu> popup(new wxMenu());
	while (obs_enum_input_types(idx, &type)) {
		const char *name = obs_source_getdisplayname(SOURCE_INPUT,
				type, wxGetApp().GetLocale());

		types.push_back(type);
		popup->Append((int)idx+1, wxString(name, wxConvUTF8));

		idx++;
	}

	if (idx) {
		int id = WXDoPopupMenu(this, popup.get());
		if (id != -1)
			AddSource(scene, types[id-1]);
	}

	obs_scene_release(scene);
}

void OBSBasic::sourceAddClicked(wxCommandEvent &event)
{
	AddSourcePopupMenu();
}

void OBSBasic::sourceRemoveClicked(wxCommandEvent &event)
{
	int sel = sources->GetSelection();
	if (sel == wxNOT_FOUND)
		return;

	obs_sceneitem_t item = (obs_sceneitem_t)sources->GetClientData(sel);
	obs_source_t source = obs_sceneitem_getsource(item);
	int ref = obs_sceneitem_destroy(item);

	/* If this is the last reference in the scene, mark the source for
	 * removal.  Reference count being at 1 means that it's no longer in
	 * any more scenes. */
	if (ref == 1)
		obs_source_remove(source);
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
