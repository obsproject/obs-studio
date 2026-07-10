/******************************************************************************
 Copyright (C) 2014 by John R. Bradley <jrb@turrettech.com>
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

#pragma once

#include <obs-module.h>

#include "cef-headers.hpp"
#include "browser-app.hpp"
#include "webgpu-config.hpp"
#include <atomic>
#include <functional>
#include <string>
#include <mutex>

enum class ControlLevel : int {
	None,
	ReadObs,
	ReadUser,
	Basic,
	Advanced,
	All,
};
inline constexpr ControlLevel DEFAULT_CONTROL_LEVEL = ControlLevel::ReadObs;

extern bool hwaccel;
extern BrowserWebGPUConfig browser_webgpu_config;

struct BrowserSource {
	BrowserSource **p_prev_next = nullptr;
	BrowserSource *next = nullptr;

	obs_source_t *source = nullptr;

	bool tex_sharing_avail = false;
	bool create_browser = false;
	std::recursive_mutex lockBrowser;
	CefRefPtr<CefBrowser> cefBrowser;

	std::string url;
	std::string css;
	gs_texture_t *texture = nullptr;
	gs_texture_t *extra_texture = nullptr;
	uint32_t last_cx = 0;
	uint32_t last_cy = 0;
	gs_color_format last_format = GS_UNKNOWN;

#ifdef ENABLE_BROWSER_SHARED_TEXTURE
#ifdef _WIN32
	void *last_handle = INVALID_HANDLE_VALUE;
#elif defined(__APPLE__)
	void *last_handle = nullptr;
#endif
#endif

	int width = 0;
	int height = 0;
	bool fps_custom = false;
	int fps = 0;
	double canvas_fps = 0;
	bool restart = false;
	bool shutdown_on_invisible = false;
	bool is_local = false;
	bool first_update = true;
	bool reroute_audio = true;
	std::atomic<bool> destroying = false;
	ControlLevel webpage_control_level = DEFAULT_CONTROL_LEVEL;
#if defined(BROWSER_EXTERNAL_BEGIN_FRAME_ENABLED) && defined(ENABLE_BROWSER_SHARED_TEXTURE)
	bool reset_frame = false;
#endif
	bool is_showing = false;

	inline void DestroyTextures()
	{
		obs_enter_graphics();
		if (extra_texture) {
			gs_texture_destroy(extra_texture);
			extra_texture = nullptr;
			last_cx = 0;
			last_cy = 0;
			last_format = GS_UNKNOWN;
		}
		if (texture) {
			gs_texture_destroy(texture);
			texture = nullptr;
		}
		obs_leave_graphics();
	}

	/* ---------------------------- */

	bool CreateBrowser();
	void DestroyBrowser();
	void ExecuteOnBrowser(BrowserFunc func, bool async = false);

	/* ---------------------------- */

	BrowserSource(obs_data_t *settings, obs_source_t *source);
	~BrowserSource();

	void Destroy();

	void Update(obs_data_t *settings = nullptr);
	void Tick();
	void Render();

	void SendMouseClick(const struct obs_mouse_event *event, int32_t type, bool mouse_up, uint32_t click_count);
	void SendMouseMove(const struct obs_mouse_event *event, bool mouse_leave);
	void SendMouseWheel(const struct obs_mouse_event *event, int x_delta, int y_delta);
	void SendFocus(bool focus);
	void SendKeyClick(const struct obs_key_event *event, bool key_up);
	void SetShowing(bool showing);
	void SetActive(bool active);
	void Refresh();

#if defined(BROWSER_EXTERNAL_BEGIN_FRAME_ENABLED) && defined(ENABLE_BROWSER_SHARED_TEXTURE)
	inline void SignalBeginFrame();
#endif

	void SetBrowser(CefRefPtr<CefBrowser> b);
	CefRefPtr<CefBrowser> GetBrowser();
};
