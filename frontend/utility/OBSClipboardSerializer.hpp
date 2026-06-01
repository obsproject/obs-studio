/******************************************************************************
    Copyright (C) 2026 by Kunal Dubey <xakep8@protonmail.com>

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

#include <obs.hpp>

class OBSClipboardSerializer {
public:
	static OBSData SerializeSceneItem(OBSSceneItem item);
	static OBSData SerializeFilters(OBSSource source);
	static OBSData SerializeTransform(OBSSceneItem item);
	static OBSData SerializeTransition(OBSSceneItem item, bool show);

	static bool DeserializeSceneItem(const OBSData &data);
	static bool DeserializeFilters(const OBSData &data, OBSDataArrayAutoRelease &filters);
	static bool DeserializeTransform(const OBSData &data, obs_transform_info &transform, obs_sceneitem_crop &crop);

	// not needed as obs_sceneitem_transition_load does the job for us
	// static bool DeserializeTransition(const OBSData &data);
};
