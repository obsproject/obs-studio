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

#pragma once

#include <obs.h>

class OBSSource {
	obs_source_t source;

public:
	inline OBSSource(obs_source_t source) : source(source) {}
	inline ~OBSSource() {obs_source_release(source);}

	inline operator obs_source_t() {return source;}

	inline bool operator==(obs_source_t p) const {return source == p;}
	inline bool operator!=(obs_source_t p) const {return source != p;}
};

class OBSSourceRef {
	obs_source_t source;

public:
	inline OBSSourceRef(obs_source_t source) : source(source)
	{
		obs_source_addref(source);
	}

	inline OBSSourceRef(const OBSSourceRef &ref) : source(ref.source)
	{
		obs_source_addref(source);
	}

	inline OBSSourceRef(OBSSourceRef &&ref) : source(ref.source)
	{
		ref.source = NULL;
	}

	inline ~OBSSourceRef() {obs_source_release(source);}

	inline OBSSourceRef &operator=(obs_source_t sourceIn)
	{
		obs_source_addref(sourceIn);
		obs_source_release(source);
		source = sourceIn;
		return *this;
	}

	inline OBSSourceRef &operator=(const OBSSourceRef &ref)
	{
		obs_source_addref(ref.source);
		obs_source_release(source);
		source = ref.source;
		return *this;
	}

	inline OBSSourceRef &operator=(OBSSourceRef &&ref)
	{
		if (this != &ref) {
			source = ref.source;
			ref.source = NULL;
		}

		return *this;
	}

	inline operator obs_source_t() const {return source;}

	inline bool operator==(obs_source_t p) const {return source == p;}
	inline bool operator!=(obs_source_t p) const {return source != p;}
};
