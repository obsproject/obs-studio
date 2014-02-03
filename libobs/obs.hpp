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

/* Useful C++ classes and bindings for base obs data */

#pragma once

#include "obs.h"

/* RAII wrappers */

template<typename T, void addref(T), void release(T)>
class OBSRef {
	T val;

	inline OBSRef &Replace(T valIn)
	{
		addref(valIn);
		release(val);
		val = valIn;
		return *this;
	}

public:
	inline OBSRef() : val(nullptr)                  {}
	inline OBSRef(T val_) : val(val_)               {addref(val);}
	inline OBSRef(const OBSRef &ref) : val(ref.val) {addref(val);}
	inline OBSRef(OBSRef &&ref) : val(ref.val)      {ref.val = nullptr;}

	inline ~OBSRef() {release(val);}

	inline OBSRef &operator=(T valIn)           {return Replace(valIn);}
	inline OBSRef &operator=(const OBSRef &ref) {return Replace(ref.val);}

	inline OBSRef &operator=(OBSRef &&ref)
	{
		if (this != &ref) {
			val = ref.val;
			ref.val = nullptr;
		}

		return *this;
	}

	inline operator T() const {return val;}

	inline bool operator==(T p) const {return val == p;}
	inline bool operator!=(T p) const {return val != p;}
};

using OBSSource = OBSRef<obs_source_t, obs_source_addref, obs_source_release>;
using OBSScene = OBSRef<obs_scene_t, obs_scene_addref, obs_scene_release>;
using OBSSceneItem = OBSRef<obs_sceneitem_t, obs_sceneitem_addref,
						obs_sceneitem_release>;

using OBSData = OBSRef<obs_data_t, obs_data_addref, obs_data_release>;
using OBSDataArray = OBSRef<obs_data_array_t, obs_data_array_addref,
						obs_data_array_release>;
