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

template<class RefClass> class OBSRef {
	typedef typename RefClass::type T;
	T val;

public:
	inline OBSRef() : val(nullptr)                  {}
	inline OBSRef(T val_) : val(val_)               {RefClass::AddRef(val);}
	inline OBSRef(const OBSRef &ref) : val(ref.val) {RefClass::AddRef(val);}
	inline OBSRef(OBSRef &&ref) : val(ref.val)      {ref.val = nullptr;}

	inline ~OBSRef() {RefClass::Release(val);}

	inline OBSRef &operator=(T valIn)
	{
		RefClass::AddRef(valIn);
		RefClass::Release(val);
		val = valIn;
		return *this;
	}

	inline OBSRef &operator=(const OBSRef &ref)
	{
		RefClass::AddRef(ref.val);
		RefClass::Release(val);
		val = ref.val;
		return *this;
	}

	inline OBSRef &operator=(OBSRef &&ref)
	{
		if (this != &ref) {
			val = ref.val;
			ref.val = NULL;
		}

		return *this;
	}

	inline operator T() const {return val;}

	inline bool operator==(T p) const {return val == p;}
	inline bool operator!=(T p) const {return val != p;}
};

class OBSSourceRefClass {
public:
	typedef obs_source_t type;
	static inline void AddRef(type val)  {obs_source_addref(val);}
	static inline void Release(type val) {obs_source_release(val);}
};

class OBSSceneRefClass {
public:
	typedef obs_scene_t type;
	static inline void AddRef(type val)  {obs_scene_addref(val);}
	static inline void Release(type val) {obs_scene_release(val);}
};

class OBSSceneItemRefClass {
public:
	typedef obs_sceneitem_t type;
	static inline void AddRef(type val)  {obs_sceneitem_addref(val);}
	static inline void Release(type val) {obs_sceneitem_release(val);}
};

typedef OBSRef<OBSSourceRefClass>    OBSSource;
typedef OBSRef<OBSSceneRefClass>     OBSScene;
typedef OBSRef<OBSSceneItemRefClass> OBSSceneItem;
