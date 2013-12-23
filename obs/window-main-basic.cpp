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

#include "obs-app.hpp"
#include "window-settings-basic.hpp"
#include "window-main-basic.hpp"

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
