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

#include <string.h>
#include <stdarg.h>

#include <util/config-file.h>
#include <util/text-lookup.h>
#include <obs.h>

/* RAII wrappers */

template<typename T> class BPtr {
	T *ptr;

	BPtr(BPtr const&) = delete;

	BPtr &operator=(BPtr const&) = delete;

public:
	inline BPtr(T *p=nullptr) : ptr(p)         {}
	inline BPtr(BPtr &&other) : ptr(other.ptr) {other.ptr = nullptr;}
	inline ~BPtr()                             {bfree(ptr);}

	inline T *operator=(T *p)   {bfree(ptr); ptr = p; return p;}
	inline operator T*()        {return ptr;}
	inline T **operator&()      {bfree(ptr); ptr = nullptr; return &ptr;}

	inline bool operator!()     {return ptr == NULL;}
	inline bool operator==(T p) {return ptr == p;}
	inline bool operator!=(T p) {return ptr != p;}
};

class ConfigFile {
	config_t config;

	ConfigFile(ConfigFile const&) = delete;
	ConfigFile &operator=(ConfigFile const&) = delete;

public:
	inline ConfigFile() : config(NULL) {}
	inline ConfigFile(ConfigFile &&other) : config(other.config)
	{
		other.config = nullptr;
	}
	inline ~ConfigFile()
	{
		config_close(config);
	}

	inline bool Create(const char *file)
	{
		Close();
		config = config_create(file);
		return config != NULL;
	}

	int Open(const char *file, config_open_type openType)
	{
		Close();
		return config_open(&config, file, openType);
	}

	int Save()
	{
		return config_save(config);
	}

	void Close()
	{
		config_close(config);
		config = NULL;
	}

	inline operator config_t() {return config;}
};

class TextLookup {
	lookup_t lookup;

	TextLookup(TextLookup const&) = delete;

	TextLookup &operator=(TextLookup const&) = delete;

public:
	inline TextLookup(lookup_t lookup=nullptr) : lookup(lookup) {}
	inline TextLookup(TextLookup &&other) : lookup(other.lookup)
	{
		other.lookup = nullptr;
	}
	inline ~TextLookup() {text_lookup_destroy(lookup);}

	inline TextLookup& operator=(lookup_t val)
	{
		text_lookup_destroy(lookup);
		lookup = val;
		return *this;
	}

	inline operator lookup_t() {return lookup;}

	inline const char *GetString(const char *lookupVal)
	{
		const char *out;
		if (!text_lookup_getstr(lookup, lookupVal, &out))
			return lookupVal;

		return out;
	}
};

class OBSSource {
	obs_source_t source;

	OBSSource(OBSSource &&) = delete;
	OBSSource(OBSSource const&) = delete;

	OBSSource &operator=(OBSSource const&) = delete;

public:
	inline OBSSource(obs_source_t source) : source(source) {}
	inline ~OBSSource() {obs_source_release(source);}

	inline OBSSource& operator=(obs_source_t p) {source = p; return *this;}

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
