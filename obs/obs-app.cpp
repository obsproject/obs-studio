/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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
#include "window-obs-basic.hpp"
#include "obs-wrappers.hpp"
#include "wx-wrappers.hpp"

IMPLEMENT_APP(OBSApp)

bool OBSApp::OnInit()
{
	if (!wxApp::OnInit())
		return false;

	if (!obs_startup())
		return false;

	wxInitAllImageHandlers();

	OBSBasic *mainWindow = new OBSBasic();

	const wxPanel *preview = mainWindow->GetPreviewPanel();
	wxRect rc = mainWindow->GetPreviewPanel()->GetClientRect();

	struct obs_video_info ovi;
	ovi.adapter         = 0;
	ovi.base_width      = rc.width;
	ovi.base_height     = rc.height;
	ovi.fps_num         = 30000;
	ovi.fps_den         = 1001;
	ovi.graphics_module = "libobs-opengl";
	ovi.output_format   = VIDEO_FORMAT_RGBA;
	ovi.output_width    = rc.width;
	ovi.output_height   = rc.height;
	ovi.window          = WxToGSWindow(preview);

	if (!obs_reset_video(&ovi))
		return false;

	mainWindow->Show();
	return true;
}

int OBSApp::OnExit()
{
	obs_shutdown();
	blog(LOG_INFO, "Number of memory leaks: %u", bnum_allocs());

	wxApp::OnExit();
	return 0;
}
