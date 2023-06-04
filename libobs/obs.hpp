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

/* Useful C++ classes and bindings for base obs data */

#pragma once

#include "obs.h"

/* RAII wrappers */

template<typename T, void release(T)> class OBSRefAutoRelease;
template<typename T, void addref(T), void release(T)> class OBSRef;
template<typename T, T getref(T), void release(T)> class OBSSafeRef;

using OBSObject =
	OBSSafeRef<obs_object_t *, obs_object_get_ref, obs_object_release>;
using OBSSource =
	OBSSafeRef<obs_source_t *, obs_source_get_ref, obs_source_release>;
using OBSScene =
	OBSSafeRef<obs_scene_t *, obs_scene_get_ref, obs_scene_release>;
using OBSSceneItem =
	OBSRef<obs_sceneitem_t *, obs_sceneitem_addref, obs_sceneitem_release>;
using OBSData = OBSRef<obs_data_t *, obs_data_addref, obs_data_release>;
using OBSDataArray = OBSRef<obs_data_array_t *, obs_data_array_addref,
			    obs_data_array_release>;
using OBSOutput =
	OBSSafeRef<obs_output_t *, obs_output_get_ref, obs_output_release>;
using OBSEncoder =
	OBSSafeRef<obs_encoder_t *, obs_encoder_get_ref, obs_encoder_release>;
using OBSService =
	OBSSafeRef<obs_service_t *, obs_service_get_ref, obs_service_release>;

using OBSWeakObject = OBSRef<obs_weak_object_t *, obs_weak_object_addref,
			     obs_weak_object_release>;
using OBSWeakSource = OBSRef<obs_weak_source_t *, obs_weak_source_addref,
			     obs_weak_source_release>;
using OBSWeakOutput = OBSRef<obs_weak_output_t *, obs_weak_output_addref,
			     obs_weak_output_release>;
using OBSWeakEncoder = OBSRef<obs_weak_encoder_t *, obs_weak_encoder_addref,
			      obs_weak_encoder_release>;
using OBSWeakService = OBSRef<obs_weak_service_t *, obs_weak_service_addref,
			      obs_weak_service_release>;

#define OBS_AUTORELEASE
using OBSObjectAutoRelease =
	OBSRefAutoRelease<obs_object_t *, obs_object_release>;
using OBSSourceAutoRelease =
	OBSRefAutoRelease<obs_source_t *, obs_source_release>;
using OBSSceneAutoRelease = OBSRefAutoRelease<obs_scene_t *, obs_scene_release>;
using OBSSceneItemAutoRelease =
	OBSRefAutoRelease<obs_sceneitem_t *, obs_sceneitem_release>;
using OBSDataAutoRelease = OBSRefAutoRelease<obs_data_t *, obs_data_release>;
using OBSDataArrayAutoRelease =
	OBSRefAutoRelease<obs_data_array_t *, obs_data_array_release>;
using OBSOutputAutoRelease =
	OBSRefAutoRelease<obs_output_t *, obs_output_release>;
using OBSEncoderAutoRelease =
	OBSRefAutoRelease<obs_encoder_t *, obs_encoder_release>;
using OBSServiceAutoRelease =
	OBSRefAutoRelease<obs_service_t *, obs_service_release>;

using OBSWeakObjectAutoRelease =
	OBSRefAutoRelease<obs_weak_object_t *, obs_weak_object_release>;
using OBSWeakSourceAutoRelease =
	OBSRefAutoRelease<obs_weak_source_t *, obs_weak_source_release>;
using OBSWeakOutputAutoRelease =
	OBSRefAutoRelease<obs_weak_output_t *, obs_weak_output_release>;
using OBSWeakEncoderAutoRelease =
	OBSRefAutoRelease<obs_weak_encoder_t *, obs_weak_encoder_release>;
using OBSWeakServiceAutoRelease =
	OBSRefAutoRelease<obs_weak_service_t *, obs_weak_service_release>;

template<typename T, void release(T)> class OBSRefAutoRelease {
protected:
	T val;

public:
	inline OBSRefAutoRelease() : val(nullptr) {}
	inline OBSRefAutoRelease(T val_) : val(val_) {}
	OBSRefAutoRelease(const OBSRefAutoRelease &ref) = delete;
	inline OBSRefAutoRelease(OBSRefAutoRelease &&ref) : val(ref.val)
	{
		ref.val = nullptr;
	}

	inline ~OBSRefAutoRelease() { release(val); }

	inline operator T() const { return val; }
	inline T Get() const { return val; }

	inline bool operator==(T p) const { return val == p; }
	inline bool operator!=(T p) const { return val != p; }

	inline OBSRefAutoRelease &operator=(OBSRefAutoRelease &&ref)
	{
		if (this != &ref) {
			release(val);
			val = ref.val;
			ref.val = nullptr;
		}

		return *this;
	}

	inline OBSRefAutoRelease &operator=(T new_val)
	{
		release(val);
		val = new_val;
		return *this;
	}
};

template<typename T, void addref(T), void release(T)>
class OBSRef : public OBSRefAutoRelease<T, release> {

	inline OBSRef &Replace(T valIn)
	{
		addref(valIn);
		release(this->val);
		this->val = valIn;
		return *this;
	}

	struct TakeOwnership {
	};
	inline OBSRef(T val_, TakeOwnership)
		: OBSRefAutoRelease<T, release>::OBSRefAutoRelease(val_)
	{
	}

public:
	inline OBSRef()
		: OBSRefAutoRelease<T, release>::OBSRefAutoRelease(nullptr)
	{
	}
	inline OBSRef(const OBSRef &ref)
		: OBSRefAutoRelease<T, release>::OBSRefAutoRelease(ref.val)
	{
		addref(this->val);
	}
	inline OBSRef(T val_)
		: OBSRefAutoRelease<T, release>::OBSRefAutoRelease(val_)
	{
		addref(this->val);
	}

	inline OBSRef &operator=(const OBSRef &ref) { return Replace(ref.val); }
	inline OBSRef &operator=(T valIn) { return Replace(valIn); }

	friend OBSWeakObject OBSGetWeakRef(obs_object_t *object);
	friend OBSWeakSource OBSGetWeakRef(obs_source_t *source);
	friend OBSWeakOutput OBSGetWeakRef(obs_output_t *output);
	friend OBSWeakEncoder OBSGetWeakRef(obs_encoder_t *encoder);
	friend OBSWeakService OBSGetWeakRef(obs_service_t *service);
};

template<typename T, T getref(T), void release(T)>
class OBSSafeRef : public OBSRefAutoRelease<T, release> {

	inline OBSSafeRef &Replace(T valIn)
	{
		T newVal = getref(valIn);
		release(this->val);
		this->val = newVal;
		return *this;
	}

	struct TakeOwnership {
	};
	inline OBSSafeRef(T val_, TakeOwnership)
		: OBSRefAutoRelease<T, release>::OBSRefAutoRelease(val_)
	{
	}

public:
	inline OBSSafeRef()
		: OBSRefAutoRelease<T, release>::OBSRefAutoRelease(nullptr)
	{
	}
	inline OBSSafeRef(const OBSSafeRef &ref)
		: OBSRefAutoRelease<T, release>::OBSRefAutoRelease(ref.val)
	{
		this->val = getref(ref.val);
	}
	inline OBSSafeRef(T val_)
		: OBSRefAutoRelease<T, release>::OBSRefAutoRelease(val_)
	{
		this->val = getref(this->val);
	}

	inline OBSSafeRef &operator=(const OBSSafeRef &ref)
	{
		return Replace(ref.val);
	}
	inline OBSSafeRef &operator=(T valIn) { return Replace(valIn); }

	friend OBSObject OBSGetStrongRef(obs_weak_object_t *weak);
	friend OBSSource OBSGetStrongRef(obs_weak_source_t *weak);
	friend OBSOutput OBSGetStrongRef(obs_weak_output_t *weak);
	friend OBSEncoder OBSGetStrongRef(obs_weak_encoder_t *weak);
	friend OBSService OBSGetStrongRef(obs_weak_service_t *weak);
};

inline OBSObject OBSGetStrongRef(obs_weak_object_t *weak)
{
	return {obs_weak_object_get_object(weak), OBSObject::TakeOwnership()};
}

inline OBSWeakObject OBSGetWeakRef(obs_object_t *object)
{
	return {obs_object_get_weak_object(object),
		OBSWeakObject::TakeOwnership()};
}

inline OBSSource OBSGetStrongRef(obs_weak_source_t *weak)
{
	return {obs_weak_source_get_source(weak), OBSSource::TakeOwnership()};
}

inline OBSWeakSource OBSGetWeakRef(obs_source_t *source)
{
	return {obs_source_get_weak_source(source),
		OBSWeakSource::TakeOwnership()};
}

inline OBSOutput OBSGetStrongRef(obs_weak_output_t *weak)
{
	return {obs_weak_output_get_output(weak), OBSOutput::TakeOwnership()};
}

inline OBSWeakOutput OBSGetWeakRef(obs_output_t *output)
{
	return {obs_output_get_weak_output(output),
		OBSWeakOutput::TakeOwnership()};
}

inline OBSEncoder OBSGetStrongRef(obs_weak_encoder_t *weak)
{
	return {obs_weak_encoder_get_encoder(weak),
		OBSEncoder::TakeOwnership()};
}

inline OBSWeakEncoder OBSGetWeakRef(obs_encoder_t *encoder)
{
	return {obs_encoder_get_weak_encoder(encoder),
		OBSWeakEncoder::TakeOwnership()};
}

inline OBSService OBSGetStrongRef(obs_weak_service_t *weak)
{
	return {obs_weak_service_get_service(weak),
		OBSService::TakeOwnership()};
}

inline OBSWeakService OBSGetWeakRef(obs_service_t *service)
{
	return {obs_service_get_weak_service(service),
		OBSWeakService::TakeOwnership()};
}

/* objects that are not meant to be instanced */
template<typename T, void destroy(T)> class OBSPtr {
	T obj;

public:
	inline OBSPtr() : obj(nullptr) {}
	inline OBSPtr(T obj_) : obj(obj_) {}
	inline OBSPtr(const OBSPtr &) = delete;
	inline OBSPtr(OBSPtr &&other) : obj(other.obj) { other.obj = nullptr; }

	inline ~OBSPtr() { destroy(obj); }

	inline OBSPtr &operator=(T obj_)
	{
		if (obj_ != obj)
			destroy(obj);
		obj = obj_;
		return *this;
	}
	inline OBSPtr &operator=(const OBSPtr &) = delete;
	inline OBSPtr &operator=(OBSPtr &&other)
	{
		if (obj)
			destroy(obj);
		obj = other.obj;
		other.obj = nullptr;
		return *this;
	}

	inline operator T() const { return obj; }

	inline bool operator==(T p) const { return obj == p; }
	inline bool operator!=(T p) const { return obj != p; }
};

using OBSDisplay = OBSPtr<obs_display_t *, obs_display_destroy>;
using OBSView = OBSPtr<obs_view_t *, obs_view_destroy>;

/* signal handler connection */
class OBSSignal {
	signal_handler_t *handler;
	const char *signal;
	signal_callback_t callback;
	void *param;

public:
	inline OBSSignal()
		: handler(nullptr),
		  signal(nullptr),
		  callback(nullptr),
		  param(nullptr)
	{
	}

	inline OBSSignal(signal_handler_t *handler_, const char *signal_,
			 signal_callback_t callback_, void *param_)
		: handler(handler_),
		  signal(signal_),
		  callback(callback_),
		  param(param_)
	{
		signal_handler_connect_ref(handler, signal, callback, param);
	}

	inline void Disconnect()
	{
		signal_handler_disconnect(handler, signal, callback, param);
		handler = nullptr;
		signal = nullptr;
		callback = nullptr;
		param = nullptr;
	}

	inline ~OBSSignal() { Disconnect(); }

	inline void Connect(signal_handler_t *handler_, const char *signal_,
			    signal_callback_t callback_, void *param_)
	{
		Disconnect();

		handler = handler_;
		signal = signal_;
		callback = callback_;
		param = param_;
		signal_handler_connect_ref(handler, signal, callback, param);
	}

	OBSSignal(const OBSSignal &) = delete;
	OBSSignal(OBSSignal &&other) noexcept
		: handler(other.handler),
		  signal(other.signal),
		  callback(other.callback),
		  param(other.param)
	{
		other.handler = nullptr;
		other.signal = nullptr;
		other.callback = nullptr;
		other.param = nullptr;
	}

	OBSSignal &operator=(const OBSSignal &) = delete;
	OBSSignal &operator=(OBSSignal &&other) noexcept
	{
		Disconnect();

		handler = other.handler;
		signal = other.signal;
		callback = other.callback;
		param = other.param;

		other.handler = nullptr;
		other.signal = nullptr;
		other.callback = nullptr;
		other.param = nullptr;

		return *this;
	}
};
