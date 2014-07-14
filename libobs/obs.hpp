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
using OBSScene = OBSRef<obs_scene_t,  obs_scene_addref,  obs_scene_release>;
using OBSSceneItem = OBSRef<obs_sceneitem_t, obs_sceneitem_addref,
						obs_sceneitem_release>;
using OBSData = OBSRef<obs_data_t, obs_data_addref, obs_data_release>;
using OBSDataArray = OBSRef<obs_data_array_t, obs_data_array_addref,
						obs_data_array_release>;

/* objects that are not meant to be instanced */
template<typename T, void destroy(T)> class OBSObj {
	T obj;

public:
	inline OBSObj() : obj(nullptr)    {}
	inline OBSObj(T obj_) : obj(obj_) {}

	inline ~OBSObj()                  {destroy(obj);}

	inline OBSObj &operator=(T obj_)
	{
		if (obj_ != obj)
			destroy(obj);
		obj = obj_;
		return *this;
	}

	inline operator T() const {return obj;}

	inline bool operator==(T p) const {return obj == p;}
	inline bool operator!=(T p) const {return obj != p;}
};

using OBSDisplay = OBSObj<obs_display_t, obs_display_destroy>;
using OBSEncoder = OBSObj<obs_encoder_t, obs_encoder_destroy>;
using OBSView    = OBSObj<obs_view_t,    obs_view_destroy>;
using OBSOutput  = OBSObj<obs_output_t,  obs_output_destroy>;

/* signal handler connection */
class OBSSignal {
	signal_handler_t  handler;
	const char        *signal;
	signal_callback_t callback;
	void              *param;

public:
	inline OBSSignal()
		: handler  (nullptr),
		  signal   (nullptr),
		  callback (nullptr),
		  param    (nullptr)
	{}

	inline OBSSignal(signal_handler_t handler_,
			const char        *signal_,
			signal_callback_t callback_,
			void              *param_)
		: handler  (handler_),
		  signal   (signal_),
		  callback (callback_),
		  param    (param_)
	{
		signal_handler_connect(handler, signal, callback, param);
	}

	inline void Disconnect()
	{
		signal_handler_disconnect(handler, signal, callback, param);
		handler  = nullptr;
		signal   = nullptr;
		callback = nullptr;
		param    = nullptr;
	}

	inline ~OBSSignal() {Disconnect();}

	inline void Connect(signal_handler_t handler_,
			const char *signal_,
			signal_callback_t callback_,
			void *param_)
	{
		Disconnect();

		handler  = handler_;
		signal   = signal_;
		callback = callback_;
		param    = param_;
		signal_handler_connect(handler, signal, callback, param);
	}
};
