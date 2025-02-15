/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include "OBSBasic.hpp"

#include <utility/ScreenshotObj.hpp>

#include <qt-wrappers.hpp>

void OBSBasic::Screenshot(OBSSource source)
{
	if (!!screenshotData) {
		blog(LOG_WARNING, "Cannot take new screenshot, "
				  "screenshot currently in progress");
		return;
	}

	screenshotData = new ScreenshotObj(source);
}

void OBSBasic::ScreenshotSelectedSource()
{
	OBSSceneItem item = GetCurrentSceneItem();
	if (item) {
		Screenshot(obs_sceneitem_get_source(item));
	} else {
		blog(LOG_INFO, "Could not take a source screenshot: "
			       "no source selected");
	}
}

void OBSBasic::ScreenshotProgram()
{
	Screenshot(GetProgramSource());
}

void OBSBasic::ScreenshotScene()
{
	Screenshot(GetCurrentSceneSource());
}
