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

#include "obs-browser-source.hpp"
#include "browser-client.hpp"
#include "browser-scheme.hpp"
#include "wide-string.hpp"
#include <nlohmann/json.hpp>
#include <util/threading.h>
#include <QApplication>
#include <util/dstr.h>
#include <functional>
#include <thread>
#include <mutex>

#ifdef __linux__
#include "linux-keyboard-helpers.hpp"
#endif

#ifdef ENABLE_BROWSER_QT_LOOP
#include <QEventLoop>
#include <QThread>
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
#include "drm-format.hpp"
#endif

using namespace std;

extern os_event_t *cef_started_event;
extern bool QueueCEFTask(std::function<void()> task);

static mutex browser_list_mutex;
static BrowserSource *first_browser = nullptr;

static void SendBrowserVisibility(CefRefPtr<CefBrowser> browser, bool isVisible)
{
	if (!browser)
		return;

	if (isVisible) {
		browser->GetHost()->WasResized();
		browser->GetHost()->WasHidden(false);
		browser->GetHost()->Invalidate(PET_VIEW);
	} else {
		browser->GetHost()->WasHidden(true);
	}

	CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Visibility");
	CefRefPtr<CefListValue> args = msg->GetArgumentList();
	args->SetBool(0, isVisible);
	SendBrowserProcessMessage(browser, PID_RENDERER, msg);
}

void DispatchJSEvent(std::string eventName, std::string jsonString, BrowserSource *browser = nullptr);

BrowserSource::BrowserSource(obs_data_t *, obs_source_t *source_) : source(source_)
{

	/* Register Refresh hotkey */
	auto refreshFunction = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		if (pressed) {
			BrowserSource *bs = (BrowserSource *)data;
			bs->Refresh();
		}
	};

	obs_hotkey_register_source(source, "ObsBrowser.Refresh", obs_module_text("RefreshNoCache"), refreshFunction,
				   (void *)this);

	auto jsEventFunction = [](void *p, calldata_t *calldata) {
		const auto eventName = calldata_string(calldata, "eventName");
		if (!eventName)
			return;
		auto jsonString = calldata_string(calldata, "jsonString");
		if (!jsonString)
			jsonString = "null";
		DispatchJSEvent(eventName, jsonString, (BrowserSource *)p);
	};

	proc_handler_t *ph = obs_source_get_proc_handler(source);
	proc_handler_add(ph, "void javascript_event(string eventName, string jsonString)", jsEventFunction,
			 (void *)this);

	/* defer update */
	obs_source_update(source, nullptr);

	lock_guard<mutex> lock(browser_list_mutex);
	p_prev_next = &first_browser;
	next = first_browser;
	if (first_browser)
		first_browser->p_prev_next = &next;
	first_browser = this;
}

static void ActuallyCloseBrowser(CefRefPtr<CefBrowser> cefBrowser)
{
	CefRefPtr<CefClient> client = cefBrowser->GetHost()->GetClient();
	BrowserClient *bc = reinterpret_cast<BrowserClient *>(client.get());
	if (bc) {
		bc->bs = nullptr;
	}

	/*
         * This stops rendering
         * http://magpcss.org/ceforum/viewtopic.php?f=6&t=12079
         * https://bitbucket.org/chromiumembedded/cef/issues/1363/washidden-api-got-broken-on-branch-2062)
         */
	cefBrowser->GetHost()->WasHidden(true);
	cefBrowser->GetHost()->CloseBrowser(true);
}

BrowserSource::~BrowserSource()
{
	if (cefBrowser)
		ActuallyCloseBrowser(cefBrowser);
}

void BrowserSource::Destroy()
{
	destroying = true;
	DestroyTextures();

	lock_guard<mutex> lock(browser_list_mutex);
	if (next)
		next->p_prev_next = p_prev_next;
	*p_prev_next = next;

	QueueCEFTask([this]() { delete this; });
}

void BrowserSource::ExecuteOnBrowser(BrowserFunc func, bool async)
{
	if (!async) {
#ifdef ENABLE_BROWSER_QT_LOOP
		if (QThread::currentThread() == qApp->thread()) {
			if (!!cefBrowser)
				func(cefBrowser);
			return;
		}
#endif
		os_event_t *finishedEvent;
		os_event_init(&finishedEvent, OS_EVENT_TYPE_AUTO);
		bool success = QueueCEFTask([&]() {
			if (!!cefBrowser)
				func(cefBrowser);
			os_event_signal(finishedEvent);
		});
		if (success) {
			os_event_wait(finishedEvent);
		}
		os_event_destroy(finishedEvent);
	} else {
		CefRefPtr<CefBrowser> browser = GetBrowser();
		if (!!browser) {
#ifdef ENABLE_BROWSER_QT_LOOP
			QueueBrowserTask(cefBrowser, func);
#else
			QueueCEFTask([func, browser]() { func(browser); });
#endif
		}
	}
}

bool BrowserSource::CreateBrowser()
{
	return QueueCEFTask([this]() {
#ifdef ENABLE_BROWSER_SHARED_TEXTURE
		if (hwaccel) {
			obs_enter_graphics();
#if defined(__APPLE__) || defined(_WIN32)
			tex_sharing_avail = gs_shared_texture_available();
#else
			tex_sharing_avail = obs_cef_all_drm_formats_supported();
#endif
			obs_leave_graphics();
		}
#else
		bool hwaccel = false;
#endif

		CefRefPtr<BrowserClient> browserClient =
			new BrowserClient(this, hwaccel && tex_sharing_avail, reroute_audio, webpage_control_level);

		CefWindowInfo windowInfo;
		windowInfo.bounds.width = width;
		windowInfo.bounds.height = height;
		windowInfo.windowless_rendering_enabled = true;

#ifdef ENABLE_BROWSER_SHARED_TEXTURE
		windowInfo.shared_texture_enabled = hwaccel;
#endif

		CefBrowserSettings cefBrowserSettings;

#ifdef ENABLE_BROWSER_SHARED_TEXTURE
#ifdef BROWSER_EXTERNAL_BEGIN_FRAME_ENABLED
		if (!fps_custom) {
			windowInfo.external_begin_frame_enabled = true;
			cefBrowserSettings.windowless_frame_rate = 0;
		} else {
			cefBrowserSettings.windowless_frame_rate = fps;
		}
#else
		struct obs_video_info ovi;
		obs_get_video_info(&ovi);
		canvas_fps = (double)ovi.fps_num / (double)ovi.fps_den;
		cefBrowserSettings.windowless_frame_rate = (fps_custom) ? fps : canvas_fps;
#endif
#else
		cefBrowserSettings.windowless_frame_rate = fps;
#endif

		cefBrowserSettings.default_font_size = 16;
		cefBrowserSettings.default_fixed_font_size = 16;

		auto browser = CefBrowserHost::CreateBrowserSync(windowInfo, browserClient, url, cefBrowserSettings,
								 CefRefPtr<CefDictionaryValue>(), nullptr);

		SetBrowser(browser);

		if (reroute_audio)
			cefBrowser->GetHost()->SetAudioMuted(true);
		if (obs_source_showing(source))
			is_showing = true;

		SendBrowserVisibility(cefBrowser, is_showing);
	});
}

void BrowserSource::DestroyBrowser()
{
	ExecuteOnBrowser(ActuallyCloseBrowser, true);
	SetBrowser(nullptr);
}

void BrowserSource::SendMouseClick(const struct obs_mouse_event *event, int32_t type, bool mouse_up,
				   uint32_t click_count)
{
	uint32_t modifiers = event->modifiers;
	int32_t x = event->x;
	int32_t y = event->y;

	ExecuteOnBrowser(
		[modifiers, x, y, type, mouse_up, click_count](CefRefPtr<CefBrowser> cefBrowser) {
			CefMouseEvent e;
			e.modifiers = modifiers;
			e.x = x;
			e.y = y;
			CefBrowserHost::MouseButtonType buttonType = (CefBrowserHost::MouseButtonType)type;
			cefBrowser->GetHost()->SendMouseClickEvent(e, buttonType, mouse_up, click_count);
		},
		true);
}

void BrowserSource::SendMouseMove(const struct obs_mouse_event *event, bool mouse_leave)
{
	uint32_t modifiers = event->modifiers;
	int32_t x = event->x;
	int32_t y = event->y;

	ExecuteOnBrowser(
		[modifiers, x, y, mouse_leave](CefRefPtr<CefBrowser> cefBrowser) {
			CefMouseEvent e;
			e.modifiers = modifiers;
			e.x = x;
			e.y = y;
			cefBrowser->GetHost()->SendMouseMoveEvent(e, mouse_leave);
		},
		true);
}

void BrowserSource::SendMouseWheel(const struct obs_mouse_event *event, int x_delta, int y_delta)
{
	uint32_t modifiers = event->modifiers;
	int32_t x = event->x;
	int32_t y = event->y;

	ExecuteOnBrowser(
		[modifiers, x, y, x_delta, y_delta](CefRefPtr<CefBrowser> cefBrowser) {
			CefMouseEvent e;
			e.modifiers = modifiers;
			e.x = x;
			e.y = y;
			cefBrowser->GetHost()->SendMouseWheelEvent(e, x_delta, y_delta);
		},
		true);
}

void BrowserSource::SendFocus(bool focus)
{
	ExecuteOnBrowser([focus](CefRefPtr<CefBrowser> cefBrowser) { cefBrowser->GetHost()->SetFocus(focus); }, true);
}

void BrowserSource::SendKeyClick(const struct obs_key_event *event, bool key_up)
{
	if (destroying)
		return;

	std::string text = event->text;
#ifdef __linux__
	uint32_t native_vkey = KeyboardCodeFromXKeysym(event->native_vkey);
	uint32_t modifiers = event->native_modifiers;
#elif defined(_WIN32) || defined(__APPLE__)
	uint32_t native_vkey = event->native_vkey;
	uint32_t modifiers = event->modifiers;
#else
	uint32_t native_vkey = event->native_vkey;
	uint32_t native_scancode = event->native_scancode;
	uint32_t modifiers = event->native_modifiers;
#endif

	ExecuteOnBrowser(
		[native_vkey, key_up, text, modifiers](CefRefPtr<CefBrowser> cefBrowser) {
			CefKeyEvent e;
			e.windows_key_code = native_vkey;
#ifdef __APPLE__
			e.native_key_code = native_vkey;
#endif

			e.type = key_up ? KEYEVENT_KEYUP : KEYEVENT_RAWKEYDOWN;

			if (!text.empty()) {
				wstring wide = to_wide(text);
				if (wide.size())
					e.character = wide[0];
			}

			//e.native_key_code = native_vkey;
			e.modifiers = modifiers;

			cefBrowser->GetHost()->SendKeyEvent(e);
			if (!text.empty() && !key_up) {
				e.type = KEYEVENT_CHAR;
#ifdef __linux__
				e.windows_key_code = KeyboardCodeFromXKeysym(e.character);
#elif defined(_WIN32)
				e.windows_key_code = e.character;
#elif !defined(__APPLE__)
				e.native_key_code = native_scancode;
#endif
				cefBrowser->GetHost()->SendKeyEvent(e);
			}
		},
		true);
}

void BrowserSource::SetShowing(bool showing)
{
	if (destroying)
		return;

	is_showing = showing;

	if (shutdown_on_invisible) {
		if (showing) {
			Update();
		} else {
			DestroyBrowser();
		}
	} else {
		ExecuteOnBrowser(
			[showing](CefRefPtr<CefBrowser> cefBrowser) {
				CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Visibility");
				CefRefPtr<CefListValue> args = msg->GetArgumentList();
				args->SetBool(0, showing);
				SendBrowserProcessMessage(cefBrowser, PID_RENDERER, msg);
			},
			true);
		nlohmann::json json;
		json["visible"] = showing;
		DispatchJSEvent("obsSourceVisibleChanged", json.dump(), this);
#if defined(BROWSER_EXTERNAL_BEGIN_FRAME_ENABLED) && defined(ENABLE_BROWSER_SHARED_TEXTURE)
		if (showing && !fps_custom) {
			reset_frame = false;
		}
#endif

		SendBrowserVisibility(cefBrowser, showing);

		if (showing)
			return;

		obs_enter_graphics();

		if (!hwaccel && texture) {
			DestroyTextures();
		}

		obs_leave_graphics();
	}
}

void BrowserSource::SetActive(bool active)
{
	ExecuteOnBrowser(
		[active](CefRefPtr<CefBrowser> cefBrowser) {
			CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("Active");
			CefRefPtr<CefListValue> args = msg->GetArgumentList();
			args->SetBool(0, active);
			SendBrowserProcessMessage(cefBrowser, PID_RENDERER, msg);
		},
		true);
	nlohmann::json json;
	json["active"] = active;
	DispatchJSEvent("obsSourceActiveChanged", json.dump(), this);
}

void BrowserSource::Refresh()
{
	ExecuteOnBrowser([](CefRefPtr<CefBrowser> cefBrowser) { cefBrowser->ReloadIgnoreCache(); }, true);
}

void BrowserSource::SetBrowser(CefRefPtr<CefBrowser> b)
{
	std::lock_guard<std::recursive_mutex> auto_lock(lockBrowser);
	cefBrowser = b;
}

CefRefPtr<CefBrowser> BrowserSource::GetBrowser()
{
	std::lock_guard<std::recursive_mutex> auto_lock(lockBrowser);
	return cefBrowser;
}

#ifdef ENABLE_BROWSER_SHARED_TEXTURE
#ifdef BROWSER_EXTERNAL_BEGIN_FRAME_ENABLED
inline void BrowserSource::SignalBeginFrame()
{
	if (reset_frame) {
		ExecuteOnBrowser(
			[](CefRefPtr<CefBrowser> cefBrowser) { cefBrowser->GetHost()->SendExternalBeginFrame(); },
			true);

		reset_frame = false;
	}
}
#endif
#endif

void BrowserSource::Update(obs_data_t *settings)
{
	if (settings) {
		bool n_is_local;
		int n_width;
		int n_height;
		bool n_fps_custom;
		int n_fps;
		bool n_shutdown;
		bool n_restart;
		bool n_reroute;
		ControlLevel n_webpage_control_level;
		std::string n_url;
		std::string n_css;

		n_is_local = obs_data_get_bool(settings, "is_local_file");
		n_width = (int)obs_data_get_int(settings, "width");
		n_height = (int)obs_data_get_int(settings, "height");
		n_fps_custom = obs_data_get_bool(settings, "fps_custom");
		n_fps = (int)obs_data_get_int(settings, "fps");
		n_shutdown = obs_data_get_bool(settings, "shutdown");
		n_restart = obs_data_get_bool(settings, "restart_when_active");
		n_css = obs_data_get_string(settings, "css");
		n_url = obs_data_get_string(settings, n_is_local ? "local_file" : "url");
		n_reroute = obs_data_get_bool(settings, "reroute_audio");
		n_webpage_control_level =
			static_cast<ControlLevel>(obs_data_get_int(settings, "webpage_control_level"));

		if (n_is_local && !n_url.empty()) {
			n_url = CefURIEncode(n_url, false);

#ifdef _WIN32
			size_t slash = n_url.find("%2F");
			size_t colon = n_url.find("%3A");

			if (slash != std::string::npos && colon != std::string::npos && colon < slash)
				n_url.replace(colon, 3, ":");
#endif

			while (n_url.find("%5C") != std::string::npos)
				n_url.replace(n_url.find("%5C"), 3, "/");

			while (n_url.find("%2F") != std::string::npos)
				n_url.replace(n_url.find("%2F"), 3, "/");

			// Local files are routed through our custom scheme handler to give them acess to other local files
			n_url = "http://absolute/" + n_url;
		}

		if (n_is_local == is_local && n_fps_custom == fps_custom && n_fps == fps &&
		    n_shutdown == shutdown_on_invisible && n_restart == restart && n_css == css && n_url == url &&
		    n_reroute == reroute_audio && n_webpage_control_level == webpage_control_level) {

			if (n_width == width && n_height == height)
				return;

			width = n_width;
			height = n_height;
			ExecuteOnBrowser(
				[this](CefRefPtr<CefBrowser> cefBrowser) {
					const CefSize cefSize(width, height);
					cefBrowser->GetHost()->GetClient()->GetDisplayHandler()->OnAutoResize(
						cefBrowser, cefSize);
					cefBrowser->GetHost()->WasResized();
					cefBrowser->GetHost()->Invalidate(PET_VIEW);
				},
				true);
			return;
		}

		is_local = n_is_local;
		width = n_width;
		height = n_height;
		fps = n_fps;
		fps_custom = n_fps_custom;
		shutdown_on_invisible = n_shutdown;
		reroute_audio = n_reroute;
		webpage_control_level = n_webpage_control_level;
		restart = n_restart;
		css = n_css;
		url = n_url;

		obs_source_set_audio_active(source, reroute_audio);
	}

	DestroyBrowser();
	DestroyTextures();

	if (!shutdown_on_invisible || obs_source_showing(source))
		create_browser = true;

	first_update = false;
}

void BrowserSource::Tick()
{
	if (os_event_try(cef_started_event) != 0)
		return;

	if (create_browser && CreateBrowser())
		create_browser = false;
#if defined(ENABLE_BROWSER_SHARED_TEXTURE)
#if defined(BROWSER_EXTERNAL_BEGIN_FRAME_ENABLED)
	if (!fps_custom)
		reset_frame = true;
#else
	struct obs_video_info ovi;
	obs_get_video_info(&ovi);
	double video_fps = (double)ovi.fps_num / (double)ovi.fps_den;

	if (!fps_custom) {
		if (!!cefBrowser && canvas_fps != video_fps) {
			cefBrowser->GetHost()->SetWindowlessFrameRate(video_fps);
			canvas_fps = video_fps;
		}
	}
#endif
#endif
}

extern void ProcessCef();

void BrowserSource::Render()
{
	bool flip = false;
#if defined(ENABLE_BROWSER_SHARED_TEXTURE) && CHROME_VERSION_BUILD < 6367
	flip = hwaccel;
#endif

	if (texture) {
#ifdef __APPLE__
		int type = gs_get_device_type();
		gs_effect_t *effect;

		if (type == GS_DEVICE_OPENGL) {
			effect = obs_get_base_effect((hwaccel) ? OBS_EFFECT_DEFAULT_RECT : OBS_EFFECT_DEFAULT);
		} else {
			effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
		}
#else
		gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
#endif

		bool linear_sample = extra_texture == NULL;
		gs_texture_t *draw_texture = texture;
		if (!linear_sample && !obs_source_get_texcoords_centered(source)) {
			gs_copy_texture(extra_texture, texture);
			draw_texture = extra_texture;

			linear_sample = true;
		}

		const bool previous = gs_framebuffer_srgb_enabled();
		gs_enable_framebuffer_srgb(true);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

		gs_eparam_t *const image = gs_effect_get_param_by_name(effect, "image");

		const char *tech;
		if (linear_sample) {
			gs_effect_set_texture_srgb(image, draw_texture);
			tech = "Draw";
		} else {
			gs_effect_set_texture(image, draw_texture);
			tech = "DrawSrgbDecompress";
		}

		const uint32_t flip_flag = flip ? GS_FLIP_V : 0;
		while (gs_effect_loop(effect, tech))
			gs_draw_sprite(draw_texture, flip_flag, 0, 0);

		gs_blend_state_pop();

		gs_enable_framebuffer_srgb(previous);
	}

#if defined(BROWSER_EXTERNAL_BEGIN_FRAME_ENABLED) && defined(ENABLE_BROWSER_SHARED_TEXTURE)
	SignalBeginFrame();
#elif defined(ENABLE_BROWSER_QT_LOOP)
	ProcessCef();
#endif
}

static void ExecuteOnBrowser(BrowserFunc func, BrowserSource *bs)
{
	lock_guard<mutex> lock(browser_list_mutex);

	if (bs) {
		BrowserSource *bsw = reinterpret_cast<BrowserSource *>(bs);
		bsw->ExecuteOnBrowser(func, true);
	}
}

static void ExecuteOnAllBrowsers(BrowserFunc func)
{
	lock_guard<mutex> lock(browser_list_mutex);

	BrowserSource *bs = first_browser;
	while (bs) {
		BrowserSource *bsw = reinterpret_cast<BrowserSource *>(bs);
		bsw->ExecuteOnBrowser(func, true);
		bs = bs->next;
	}
}

void DispatchJSEvent(std::string eventName, std::string jsonString, BrowserSource *browser)
{
	const auto jsEvent = [eventName, jsonString](CefRefPtr<CefBrowser> cefBrowser) {
		CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("DispatchJSEvent");
		CefRefPtr<CefListValue> args = msg->GetArgumentList();

		args->SetString(0, eventName);
		args->SetString(1, jsonString);
		SendBrowserProcessMessage(cefBrowser, PID_RENDERER, msg);
	};

	if (!browser)
		ExecuteOnAllBrowsers(jsEvent);
	else
		ExecuteOnBrowser(jsEvent, browser);
}
