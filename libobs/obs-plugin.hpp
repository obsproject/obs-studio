/******************************************************************************
    Copyright (C) 2014 by Timo Rothenpieler <btbn@btbn.de>

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

/* C++ magic to make writing plugins with C++ nicer */

#pragma once

#include "obs.h"

#include <type_traits>

class IObsPlugin
{
	public:
	virtual ~IObsPlugin() {}

	static bool initialize()
	{
		return true;
	}

	static void deinitialize()
	{
	}

	static obs_properties_t properties()
	{
		return obs_properties_create();
	}

	template<typename T>
	static inline void deinitializeObsPlugin()
	{
		T::deinitialize();
	}

	template<typename T, typename I>
	static inline bool registerObsPlugin(I *info)
	{
		if(!T::initialize())
			return false;

		info->id = T::getId();
		info->getname = &T::getName;

		info->create = [](obs_data_t settings, obs_source_t source)
		{
			return (void*)new T(settings, source);
		};

		info->destroy = [](void *data)
		{
			T *ddata = (T*)data;
			delete ddata;
		};

		info->properties = &T::properties;
		info->defaults = &T::defaults;

		return true;
	}
};

class IObsSourcePlugin : public IObsPlugin
{
	public:

	virtual uint32_t getWidth() = 0;
	virtual uint32_t getHeight() = 0;

	virtual void update(obs_data_t settings)
	{
		UNUSED_PARAMETER(settings);
	}

	virtual void activate()
	{
	}

	virtual void deactivate()
	{
	}

	virtual void show()
	{
	}

	virtual void hide()
	{
	}

	template<typename T>
	static inline bool registerObsPlugin(obs_source_info *info)
	{
		if(!IObsPlugin::registerObsPlugin<T, obs_source_info>(info))
			return false;

		info->getwidth = [](void *data)
		{
			T *ddata = (T*)data;
			return ddata->getWidth();
		};

		info->getheight = [](void *data)
		{
			T *ddata = (T*)data;
			return ddata->getHeight();
		};

		info->update = [](void *data, obs_data_t settings)
		{
			T *ddata = (T*)data;
			return ddata->update(settings);
		};

		info->activate = [](void *data)
		{
			T *ddata = (T*)data;
			return ddata->activate();
		};

		info->deactivate = [](void *data)
		{
			T *ddata = (T*)data;
			return ddata->deactivate();
		};

		info->show = [](void *data)
		{
			T *ddata = (T*)data;
			return ddata->show();
		};

		info->hide = [](void *data)
		{
			T *ddata = (T*)data;
			return ddata->hide();
		};

		return true;
	}
};

class IObsVideoSourcePlugin : public IObsSourcePlugin
{
	public:

	virtual void tick(float seconds) = 0;
	virtual void render(effect_t effect) = 0;

	template<typename T>
	static inline bool registerObsPlugin()
	{
		obs_source_info info;
		memset(&info, 0, sizeof(obs_source_info));

		if(!IObsSourcePlugin::registerObsPlugin<T>(&info))
			return false;

		info.type = OBS_SOURCE_TYPE_INPUT;
		info.output_flags = OBS_SOURCE_VIDEO;

		info.video_tick = [](void *data, float seconds)
		{
			T *ddata = (T*)data;
			return ddata->tick(seconds);
		};

		info.video_render = [](void *data, effect_t effect)
		{
			T *ddata = (T*)data;
			return ddata->render(effect);
		};

		obs_register_source(&info);

		return true;
	}
};

template<typename T>
inline bool registerPlugin()
{
	static_assert(
		std::is_base_of<IObsVideoSourcePlugin, T>::value,
		"Trying to register an unsupported plugin class");

	if(std::is_base_of<IObsVideoSourcePlugin, T>::value)
	{
		return IObsVideoSourcePlugin::registerObsPlugin<T>();
	}

	return true;
}

template<typename T>
inline void unregisterPlugin()
{
	static_assert(
		std::is_base_of<IObsPlugin, T>::value,
		"Trying to unregister an unsupported plugin class");

	IObsPlugin::deinitializeObsPlugin<T>();
}

#define OBS_DECLARE_CPP_CLASS_MODULE(cname) \
	OBS_DECLARE_MODULE() \
	bool obs_module_load(uint32_t libobs_version) { \
		UNUSED_PARAMETER(libobs_version); \
		bool res = registerPlugin<cname>(); \
		 \
		if(res) \
			blog(LOG_INFO, "Loaded %s module", #cname); \
		else \
			blog(LOG_ERROR, "Failed loading %s module", #cname); \
		 \
		return res; \
	} \
	void obs_module_unload() { \
		blog(LOG_INFO, "Unloading %s module", #cname); \
		unregisterPlugin<cname>(); \
	}
