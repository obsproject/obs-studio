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

#include "forms/OBSWindows.h"

#include <obs.hpp>

class OBSBasic : public OBSBasicBase {
	void SceneAdded(obs_source_t scene);
	void SceneRemoved(obs_source_t scene);

	static void SourceAdded(void *data, calldata_t params);
	static void SourceDestroyed(void *data, calldata_t params);

	bool InitGraphics();

	void NewProject();
	void SaveProject();
	void LoadProject();

protected:
	virtual void OnClose(wxCloseEvent &event);
	virtual void OnMinimize(wxIconizeEvent &event);
	virtual void OnSize(wxSizeEvent &event);
	virtual void fileNewClicked(wxCommandEvent &event);
	virtual void fileOpenClicked(wxCommandEvent &event);
	virtual void fileSaveClicked(wxCommandEvent &event);
	virtual void fileExitClicked(wxCommandEvent &event);
	virtual void scenesClicked(wxCommandEvent &event);
	virtual void scenesRDown(wxMouseEvent &event);
	virtual void sceneAddClicked(wxCommandEvent &event);
	virtual void sceneRemoveClicked(wxCommandEvent &event);
	virtual void scenePropertiesClicked(wxCommandEvent &event);
	virtual void sceneUpClicked(wxCommandEvent &event);
	virtual void sceneDownClicked(wxCommandEvent &event);
	virtual void sourcesClicked(wxCommandEvent &event);
	virtual void sourcesToggled(wxCommandEvent &event);
	virtual void sourcesRDown(wxMouseEvent &event);
	virtual void sourceAddClicked(wxCommandEvent &event);
	virtual void sourceRemoveClicked(wxCommandEvent &event);
	virtual void sourcePropertiesClicked(wxCommandEvent &event);
	virtual void sourceUpClicked(wxCommandEvent &event);
	virtual void sourceDownClicked(wxCommandEvent &event);
	virtual void settingsClicked(wxCommandEvent &event);
	virtual void exitClicked(wxCommandEvent &event);

public:
	inline OBSBasic() : OBSBasicBase(NULL) {}
	virtual ~OBSBasic();

	bool Init();

	inline wxPanel *GetPreviewPanel() {return previewPanel;}
	inline wxSizer *GetPreviewContainer() {return previewContainer;}
};
