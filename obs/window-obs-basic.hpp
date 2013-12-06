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

class OBSBasic : public OBSBasicBase {
protected:
	virtual void OnClose(wxCloseEvent& event);
	virtual void OnMinimize(wxIconizeEvent& event);
	virtual void OnSize(wxSizeEvent& event);
	virtual void file_newOnMenuSelection(wxCommandEvent& event);
	virtual void file_openOnMenuSelection(wxCommandEvent& event);
	virtual void file_saveOnMenuSelection(wxCommandEvent& event);
	virtual void file_exitOnMenuSelection(wxCommandEvent& event);
	virtual void scenesOnRightDown(wxMouseEvent& event);
	virtual void sceneAddOnToolClicked(wxCommandEvent& event);
	virtual void sceneRemoveOnToolClicked(wxCommandEvent& event);
	virtual void scenePropertiesOnToolClicked(wxCommandEvent& event);
	virtual void sceneUpOnToolClicked(wxCommandEvent& event);
	virtual void sceneDownOnToolClicked(wxCommandEvent& event);
	virtual void sourcesOnRightDown(wxMouseEvent& event);
	virtual void sourceAddOnToolClicked(wxCommandEvent& event);
	virtual void sourceRemoveOnToolClicked(wxCommandEvent& event);
	virtual void sourcePropertiesOnToolClicked(wxCommandEvent& event);
	virtual void sourceUpOnToolClicked(wxCommandEvent& event);
	virtual void sourceDownOnToolClicked(wxCommandEvent& event);

public:
	inline OBSBasic() : OBSBasicBase(NULL) {}

	inline wxPanel *GetPreviewPanel() {return previewPanel;}
	inline wxSizer *GetPreviewContainer() {return previewContainer;}
};
