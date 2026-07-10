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

#include "browser-client.hpp"
#include "obs-browser-source.hpp"
#include "base64/base64.hpp"
#include <nlohmann/json.hpp>
#include <obs-frontend-api.h>
#include <obs.hpp>
#include <util/platform.h>
#include <QApplication>
#include <QThread>
#include <QToolTip>
#ifdef _WIN32
#include <d3d11_1.h>
#include <dxgi.h>
#include <util/windows/ComPtr.hpp>
#endif
#if defined(__APPLE__) && CHROME_VERSION_BUILD > 4430
#include <IOSurface/IOSurface.h>
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
#include <obs-nix-platform.h>

#include "drm-format.hpp"
#endif

inline bool BrowserClient::valid() const
{
	return !!bs && !bs->destroying;
}

#ifdef _WIN32
static bool CopyCefSharedTexture(BrowserSource *bs, HANDLE shared_handle)
{
	if (!shared_handle || shared_handle == INVALID_HANDLE_VALUE)
		return false;

	auto *device = static_cast<ID3D11Device *>(gs_get_device_obj());
	if (!device)
		return false;

	ComQIPtr<ID3D11Device1> device1(device);
	if (!device1)
		return false;

	ComPtr<ID3D11Texture2D> source;
	if (FAILED(device1->OpenSharedResource1(shared_handle, __uuidof(ID3D11Texture2D),
							  reinterpret_cast<void **>(source.Assign()))))
		return false;

	D3D11_TEXTURE2D_DESC source_desc = {};
	source->GetDesc(&source_desc);
	if (source_desc.Width == 0 || source_desc.Height == 0 ||
	    (source_desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM && source_desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM_SRGB))
		return false;

	if (!bs->texture || gs_texture_get_width(bs->texture) != source_desc.Width ||
	    gs_texture_get_height(bs->texture) != source_desc.Height ||
	    gs_texture_get_color_format(bs->texture) != GS_BGRA) {
		if (bs->texture) {
			gs_texture_destroy(bs->texture);
			bs->texture = nullptr;
		}
		bs->texture = gs_texture_create(source_desc.Width, source_desc.Height, GS_BGRA, 1, nullptr, 0);
		if (!bs->texture)
			return false;
		bs->width = static_cast<int>(source_desc.Width);
		bs->height = static_cast<int>(source_desc.Height);
		bs->last_cx = 0;
		bs->last_cy = 0;
		bs->last_format = GS_UNKNOWN;
	}

	auto *destination = static_cast<ID3D11Texture2D *>(gs_texture_get_obj(bs->texture));
	if (!destination)
		return false;

	ComPtr<ID3D11DeviceContext> context;
	device->GetImmediateContext(&context);
	if (!context)
		return false;

	context->CopyResource(destination, source);

	// CEF returns this resource to its pool when this callback returns. Wait for
	// the GPU copy so a reused CEF texture can never race the OBS-owned copy.
	D3D11_QUERY_DESC query_desc = {D3D11_QUERY_EVENT, 0};
	ComPtr<ID3D11Query> copy_finished;
	if (SUCCEEDED(device->CreateQuery(&query_desc, &copy_finished))) {
		context->End(copy_finished);
		context->Flush();
		while (context->GetData(copy_finished, nullptr, 0, 0) == S_FALSE)
			SwitchToThread();
	}

	return true;
}
#endif

CefRefPtr<CefLoadHandler> BrowserClient::GetLoadHandler()
{
	return this;
}

CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler()
{
	return this;
}

CefRefPtr<CefDisplayHandler> BrowserClient::GetDisplayHandler()
{
	return this;
}

CefRefPtr<CefLifeSpanHandler> BrowserClient::GetLifeSpanHandler()
{
	return this;
}

CefRefPtr<CefContextMenuHandler> BrowserClient::GetContextMenuHandler()
{
	return this;
}

CefRefPtr<CefAudioHandler> BrowserClient::GetAudioHandler()
{
	return reroute_audio ? this : nullptr;
}

CefRefPtr<CefRequestHandler> BrowserClient::GetRequestHandler()
{
	return this;
}

CefRefPtr<CefResourceRequestHandler> BrowserClient::GetResourceRequestHandler(CefRefPtr<CefBrowser>,
									      CefRefPtr<CefFrame>,
									      CefRefPtr<CefRequest> request, bool, bool,
									      const CefString &, bool &)
{
	if (request->GetHeaderByName("origin") == "null") {
		return this;
	}

	return nullptr;
}

void BrowserClient::OnRenderProcessTerminated(CefRefPtr<CefBrowser>, TerminationStatus
#if CHROME_VERSION_BUILD >= 6367
					      ,
					      int, const CefString &error_string
#endif
)
{
	if (!valid())
		return;

#if CHROME_VERSION_BUILD >= 6367
	std::string str_text = error_string;
#else
	std::string str_text = "<unknown>";
#endif

	const char *sourceName = "<unknown>";

	if (bs && bs->source)
		sourceName = obs_source_get_name(bs->source);

	blog(LOG_ERROR, "[obs-browser: '%s'] Webpage has crashed unexpectedly! Reason: '%s'", sourceName,
	     str_text.c_str());
}

CefResourceRequestHandler::ReturnValue BrowserClient::OnBeforeResourceLoad(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>,
									   CefRefPtr<CefRequest>,
									   CefRefPtr<CefCallback>)
{
	return RV_CONTINUE;
}

bool BrowserClient::OnBeforePopup(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>,
#if CHROME_VERSION_BUILD >= 6834
				  int,
#endif
				  const CefString &, const CefString &, cef_window_open_disposition_t, bool,
				  const CefPopupFeatures &, CefWindowInfo &, CefRefPtr<CefClient> &,
				  CefBrowserSettings &, CefRefPtr<CefDictionaryValue> &, bool *)
{
	/* block popups */
	return true;
}

void BrowserClient::OnBeforeContextMenu(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame>, CefRefPtr<CefContextMenuParams>,
					CefRefPtr<CefMenuModel> model)
{
	/* remove all context menu contributions */
	model->Clear();
}

bool BrowserClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame>, CefProcessId,
					     CefRefPtr<CefProcessMessage> message)
{
	const std::string &name = message->GetName();
	CefRefPtr<CefListValue> input_args = message->GetArgumentList();
	nlohmann::json json;

	if (!valid()) {
		return false;
	}

	// Fall-through switch, so that higher levels also have lower-level rights
	switch (webpage_control_level) {
	case ControlLevel::All:
		if (name == "startRecording") {
			obs_frontend_recording_start();
		} else if (name == "stopRecording") {
			obs_frontend_recording_stop();
		} else if (name == "startStreaming") {
			obs_frontend_streaming_start();
		} else if (name == "stopStreaming") {
			obs_frontend_streaming_stop();
		} else if (name == "pauseRecording") {
			obs_frontend_recording_pause(true);
		} else if (name == "unpauseRecording") {
			obs_frontend_recording_pause(false);
		} else if (name == "startVirtualcam") {
			obs_frontend_start_virtualcam();
		} else if (name == "stopVirtualcam") {
			obs_frontend_stop_virtualcam();
		}
		[[fallthrough]];
	case ControlLevel::Advanced:
		if (name == "startReplayBuffer") {
			obs_frontend_replay_buffer_start();
		} else if (name == "stopReplayBuffer") {
			obs_frontend_replay_buffer_stop();
		} else if (name == "setCurrentScene") {
			const std::string scene_name = input_args->GetString(1).ToString();
			OBSSourceAutoRelease source = obs_get_source_by_name(scene_name.c_str());
			if (!source) {
				blog(LOG_WARNING,
				     "Browser source '%s' tried to switch to scene '%s' which doesn't exist",
				     obs_source_get_name(bs->source), scene_name.c_str());
			} else if (!obs_source_is_scene(source)) {
				blog(LOG_WARNING, "Browser source '%s' tried to switch to '%s' which isn't a scene",
				     obs_source_get_name(bs->source), scene_name.c_str());
			} else {
				obs_frontend_set_current_scene(source);
			}
		} else if (name == "setCurrentTransition") {
			const std::string transition_name = input_args->GetString(1).ToString();
			obs_frontend_source_list transitions = {};
			obs_frontend_get_transitions(&transitions);

			OBSSourceAutoRelease transition;
			for (size_t i = 0; i < transitions.sources.num; i++) {
				obs_source_t *source = transitions.sources.array[i];
				if (obs_source_get_name(source) == transition_name) {
					transition = obs_source_get_ref(source);
					break;
				}
			}

			obs_frontend_source_list_free(&transitions);

			if (transition)
				obs_frontend_set_current_transition(transition);
			else
				blog(LOG_WARNING,
				     "Browser source '%s' tried to change the current transition to '%s' which doesn't exist",
				     obs_source_get_name(bs->source), transition_name.c_str());
		}
		[[fallthrough]];
	case ControlLevel::Basic:
		if (name == "saveReplayBuffer") {
			obs_frontend_replay_buffer_save();
		}
		[[fallthrough]];
	case ControlLevel::ReadUser:
		if (name == "getScenes") {
			struct obs_frontend_source_list list = {};
			obs_frontend_get_scenes(&list);
			std::vector<nlohmann::json> scenes_vector;
			for (size_t i = 0; i < list.sources.num; i++) {
				obs_source_t *source = list.sources.array[i];
				scenes_vector.push_back(obs_source_get_name(source));
			}
			json = scenes_vector;
			obs_frontend_source_list_free(&list);
		} else if (name == "getCurrentScene") {
			OBSSourceAutoRelease current_scene = obs_frontend_get_current_scene();

			if (!current_scene)
				return false;

			const char *name = obs_source_get_name(current_scene);
			if (!name)
				return false;

			json = {{"name", name},
				{"width", obs_source_get_width(current_scene)},
				{"height", obs_source_get_height(current_scene)}};
		} else if (name == "getTransitions") {
			struct obs_frontend_source_list list = {};
			obs_frontend_get_transitions(&list);
			std::vector<nlohmann::json> transitions_vector;
			for (size_t i = 0; i < list.sources.num; i++) {
				obs_source_t *source = list.sources.array[i];
				transitions_vector.push_back(obs_source_get_name(source));
			}
			json = transitions_vector;
			obs_frontend_source_list_free(&list);
		} else if (name == "getCurrentTransition") {
			OBSSourceAutoRelease source = obs_frontend_get_current_transition();
			json = obs_source_get_name(source);
		}
		[[fallthrough]];
	case ControlLevel::ReadObs:
		if (name == "getStatus") {
			json = {{"recording", obs_frontend_recording_active()},
				{"streaming", obs_frontend_streaming_active()},
				{"recordingPaused", obs_frontend_recording_paused()},
				{"replaybuffer", obs_frontend_replay_buffer_active()},
				{"virtualcam", obs_frontend_virtualcam_active()}};
		}
		[[fallthrough]];
	case ControlLevel::None:
		if (name == "getControlLevel") {
			json = (int)webpage_control_level;
		}
	}

	CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("executeCallback");

	CefRefPtr<CefListValue> execute_args = msg->GetArgumentList();
	execute_args->SetInt(0, input_args->GetInt(0));
	execute_args->SetString(1, json.dump());

	SendBrowserProcessMessage(browser, PID_RENDERER, msg);

	return true;
}

void BrowserClient::GetViewRect(CefRefPtr<CefBrowser>, CefRect &rect)
{
	if (!valid()) {
		rect.Set(0, 0, 16, 16);
		return;
	}

	rect.Set(0, 0, bs->width < 1 ? 1 : bs->width, bs->height < 1 ? 1 : bs->height);
}

bool BrowserClient::OnTooltip(CefRefPtr<CefBrowser>, CefString &text)
{
	std::string str_text = text;
	QMetaObject::invokeMethod(QCoreApplication::instance()->thread(),
				  [str_text]() { QToolTip::showText(QCursor::pos(), str_text.c_str()); });
	return true;
}

void BrowserClient::OnPaint(CefRefPtr<CefBrowser>, PaintElementType type, const RectList &, const void *buffer,
			    int width, int height)
{
	if (type != PET_VIEW) {
		// TODO Overlay texture on top of bs->texture
		return;
	}

#ifdef ENABLE_BROWSER_SHARED_TEXTURE
	if (sharing_available) {
		return;
	}
#endif

	if (!valid()) {
		return;
	}
	if (bs->width != width || bs->height != height) {
		obs_enter_graphics();
		bs->DestroyTextures();
		obs_leave_graphics();
	}

	if (!bs->texture && width && height) {
		obs_enter_graphics();
		bs->texture = gs_texture_create(width, height, GS_BGRA, 1, (const uint8_t **)&buffer, GS_DYNAMIC);
		bs->width = width;
		bs->height = height;
		obs_leave_graphics();
	} else {
		obs_enter_graphics();
		gs_texture_set_image(bs->texture, (const uint8_t *)buffer, width * 4, false);
		obs_leave_graphics();
	}
}

#ifdef ENABLE_BROWSER_SHARED_TEXTURE
void BrowserClient::UpdateExtraTexture()
{
	if (bs->texture) {
		const uint32_t cx = gs_texture_get_width(bs->texture);
		const uint32_t cy = gs_texture_get_height(bs->texture);
		const gs_color_format format = gs_texture_get_color_format(bs->texture);
		const gs_color_format linear_format = gs_generalize_format(format);

		if (linear_format != format) {
			if (!bs->extra_texture || bs->last_format != linear_format || bs->last_cx != cx ||
			    bs->last_cy != cy) {
				if (bs->extra_texture) {
					gs_texture_destroy(bs->extra_texture);
					bs->extra_texture = nullptr;
				}
				bs->extra_texture = gs_texture_create(cx, cy, linear_format, 1, nullptr, 0);
				bs->last_cx = cx;
				bs->last_cy = cy;
				bs->last_format = linear_format;
			}
		} else if (bs->extra_texture) {
			gs_texture_destroy(bs->extra_texture);
			bs->extra_texture = nullptr;
			bs->last_cx = 0;
			bs->last_cy = 0;
			bs->last_format = GS_UNKNOWN;
		}
	}
}

void BrowserClient::OnAcceleratedPaint(CefRefPtr<CefBrowser>, PaintElementType type, const RectList &,
#if CHROME_VERSION_BUILD >= 6367
				       const CefAcceleratedPaintInfo &info)
#else
				       void *shared_handle)
#endif
{
	if (type != PET_VIEW) {
		// TODO Overlay texture on top of bs->texture
		return;
	}

	if (!valid()) {
		return;
	}

#if defined(_WIN32) && CHROME_VERSION_BUILD >= 6367
	obs_enter_graphics();
	const bool copied = CopyCefSharedTexture(bs, info.shared_texture_handle);
	if (copied)
		UpdateExtraTexture();
	obs_leave_graphics();

	if (!copied && !logged_shared_texture_copy_failure.exchange(true)) {
		blog(LOG_WARNING,
		     "[obs-browser]: CEF shared texture import failed. This browser source will need software rendering after a restart.");
	}
	return;
#endif

#if !defined(_WIN32) && !defined(__APPLE__)
	if (info.plane_count == 0)
		return;

	struct obs_cef_video_format format = obs_cef_format_from_cef_type(info.format);
	uint64_t modifier = info.modifier;

	if (format.gs_format == GS_UNKNOWN)
		return;

	uint32_t *strides = (uint32_t *)alloca(info.plane_count * sizeof(uint32_t));
	uint32_t *offsets = (uint32_t *)alloca(info.plane_count * sizeof(uint32_t));
	uint64_t *modifiers = (uint64_t *)alloca(info.plane_count * sizeof(uint64_t));
	int *fds = (int *)alloca(info.plane_count * sizeof(int));

	/* NOTE: This a workaround under X11 where the modifier is always invalid where it can mean "no modifier" in
	 * Chromium's code. */
	if (obs_get_nix_platform() == OBS_NIX_PLATFORM_X11_EGL && modifier == DRM_FORMAT_MOD_INVALID)
		modifier = DRM_FORMAT_MOD_LINEAR;

	for (size_t i = 0; i < kAcceleratedPaintMaxPlanes; i++) {
		auto *plane = &info.planes[i];

		strides[i] = plane->stride;
		offsets[i] = plane->offset;
		fds[i] = plane->fd;

		modifiers[i] = modifier;
	}
#endif

#if !defined(_WIN32) && CHROME_VERSION_BUILD < 6367
	if (shared_handle == bs->last_handle)
		return;
#endif

	obs_enter_graphics();

	if (bs->texture) {
#ifdef _WIN32
		//gs_texture_release_sync(bs->texture, 0);
#endif
		gs_texture_destroy(bs->texture);
		bs->texture = nullptr;
	}

#if defined(__APPLE__) && CHROME_VERSION_BUILD > 6367
	bs->texture = gs_texture_create_from_iosurface((IOSurfaceRef)(uintptr_t)info.shared_texture_io_surface);
#elif defined(__APPLE__) && CHROME_VERSION_BUILD > 4183
	bs->texture = gs_texture_create_from_iosurface((IOSurfaceRef)(uintptr_t)shared_handle);
#elif defined(_WIN32) && CHROME_VERSION_BUILD > 4183
	bs->texture =
#if CHROME_VERSION_BUILD >= 6367
		gs_texture_open_nt_shared((uint32_t)(uintptr_t)info.shared_texture_handle);
#else
		gs_texture_open_nt_shared((uint32_t)(uintptr_t)shared_handle);
#endif
	//if (bs->texture)
	//	gs_texture_acquire_sync(bs->texture, 1, INFINITE);

#elif defined(_WIN32)
	bs->texture = gs_texture_open_shared((uint32_t)(uintptr_t)shared_handle);
#else
	bs->texture = gs_texture_create_from_dmabuf(info.extra.coded_size.width, info.extra.coded_size.height,
						    format.drm_format, format.gs_format, info.plane_count, fds, strides,
						    offsets, modifier != DRM_FORMAT_MOD_INVALID ? modifiers : NULL);
#endif
	UpdateExtraTexture();
	obs_leave_graphics();

#if defined(__APPLE__) && CHROME_VERSION_BUILD >= 6367
	bs->last_handle = info.shared_texture_io_surface;
#elif defined(_WIN32) && CHROME_VERSION_BUILD >= 6367
	bs->last_handle = info.shared_texture_handle;
#elif defined(__APPLE__) || defined(_WIN32)
	bs->last_handle = shared_handle;
#endif
}

#ifdef CEF_ON_ACCELERATED_PAINT2
void BrowserClient::OnAcceleratedPaint2(CefRefPtr<CefBrowser>, PaintElementType type, const RectList &,
					void *shared_handle, bool new_texture)
{
	if (type != PET_VIEW) {
		// TODO Overlay texture on top of bs->texture
		return;
	}

	if (!valid()) {
		return;
	}

	if (!new_texture) {
		return;
	}

	obs_enter_graphics();

	if (bs->texture) {
		gs_texture_destroy(bs->texture);
		bs->texture = nullptr;
	}

#if defined(__APPLE__) && CHROME_VERSION_BUILD > 4183
	bs->texture = gs_texture_create_from_iosurface((IOSurfaceRef)(uintptr_t)shared_handle);
#elif defined(_WIN32) && CHROME_VERSION_BUILD > 4183
	bs->texture = gs_texture_open_nt_shared((uint32_t)(uintptr_t)shared_handle);

#else
	bs->texture = gs_texture_open_shared((uint32_t)(uintptr_t)shared_handle);
#endif
	UpdateExtraTexture();
	obs_leave_graphics();
}
#endif
#endif

static speaker_layout GetSpeakerLayout(CefAudioHandler::ChannelLayout cefLayout)
{
	switch (cefLayout) {
	case CEF_CHANNEL_LAYOUT_MONO:
		return SPEAKERS_MONO; /**< Channels: MONO */
	case CEF_CHANNEL_LAYOUT_STEREO:
		return SPEAKERS_STEREO; /**< Channels: FL, FR */
	case CEF_CHANNEL_LAYOUT_2POINT1:
	case CEF_CHANNEL_LAYOUT_2_1:
	case CEF_CHANNEL_LAYOUT_SURROUND:
		return SPEAKERS_2POINT1; /**< Channels: FL, FR, LFE */
	case CEF_CHANNEL_LAYOUT_2_2:
	case CEF_CHANNEL_LAYOUT_QUAD:
	case CEF_CHANNEL_LAYOUT_4_0:
		return SPEAKERS_4POINT0; /**< Channels: FL, FR, FC, RC */
	case CEF_CHANNEL_LAYOUT_4_1:
	case CEF_CHANNEL_LAYOUT_5_0:
	case CEF_CHANNEL_LAYOUT_5_0_BACK:
		return SPEAKERS_4POINT1; /**< Channels: FL, FR, FC, LFE, RC */
	case CEF_CHANNEL_LAYOUT_5_1:
	case CEF_CHANNEL_LAYOUT_5_1_BACK:
		return SPEAKERS_5POINT1; /**< Channels: FL, FR, FC, LFE, RL, RR */
	case CEF_CHANNEL_LAYOUT_7_1:
	case CEF_CHANNEL_LAYOUT_7_1_WIDE_BACK:
	case CEF_CHANNEL_LAYOUT_7_1_WIDE:
		return SPEAKERS_7POINT1; /**< Channels: FL, FR, FC, LFE, RL, RR, SL, SR */
	default:
		return SPEAKERS_UNKNOWN;
	}
}

void BrowserClient::OnAudioStreamStarted(CefRefPtr<CefBrowser> browser, const CefAudioParameters &params_,
					 int channels_)
{
	UNUSED_PARAMETER(browser);
	channels = channels_;
	channel_layout = (ChannelLayout)params_.channel_layout;
	sample_rate = params_.sample_rate;
	frames_per_buffer = params_.frames_per_buffer;
}

void BrowserClient::OnAudioStreamPacket(CefRefPtr<CefBrowser> browser, const float **data, int frames, int64_t pts)
{
	UNUSED_PARAMETER(browser);
	if (!valid()) {
		return;
	}
	struct obs_source_audio audio = {};
	const uint8_t **pcm = (const uint8_t **)data;
	speaker_layout speakers = GetSpeakerLayout(channel_layout);
	int speaker_count = get_audio_channels(speakers);
	for (int i = 0; i < speaker_count; i++)
		audio.data[i] = pcm[i];
	audio.samples_per_sec = sample_rate;
	audio.frames = frames;
	audio.format = AUDIO_FORMAT_FLOAT_PLANAR;
	audio.speakers = speakers;
	audio.timestamp = (uint64_t)pts * 1000000LLU;
	obs_source_output_audio(bs->source, &audio);
}

void BrowserClient::OnAudioStreamStopped(CefRefPtr<CefBrowser> browser)
{
	UNUSED_PARAMETER(browser);
}

void BrowserClient::OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString &message)
{
	UNUSED_PARAMETER(browser);
	UNUSED_PARAMETER(message);
}

static CefAudioHandler::ChannelLayout Convert2CEFSpeakerLayout(int channels)
{
	switch (channels) {
	case 1:
		return CEF_CHANNEL_LAYOUT_MONO;
	case 2:
		return CEF_CHANNEL_LAYOUT_STEREO;
	case 3:
		return CEF_CHANNEL_LAYOUT_2_1;
	case 4:
		return CEF_CHANNEL_LAYOUT_4_0;
	case 5:
		return CEF_CHANNEL_LAYOUT_4_1;
	case 6:
		return CEF_CHANNEL_LAYOUT_5_1;
	case 8:
		return CEF_CHANNEL_LAYOUT_7_1;
	default:
		return CEF_CHANNEL_LAYOUT_UNSUPPORTED;
	}
}

bool BrowserClient::GetAudioParameters(CefRefPtr<CefBrowser> browser, CefAudioParameters &params)
{
	UNUSED_PARAMETER(browser);
	int channels = (int)audio_output_get_channels(obs_get_audio());
	params.channel_layout = Convert2CEFSpeakerLayout(channels);
	params.sample_rate = (int)audio_output_get_sample_rate(obs_get_audio());
	params.frames_per_buffer = kFramesPerBuffer;
	return true;
}

void BrowserClient::OnLoadEnd(CefRefPtr<CefBrowser>, CefRefPtr<CefFrame> frame, int)
{
	if (!valid()) {
		return;
	}

	if (frame->IsMain() && bs->css.length()) {
		std::string uriEncodedCSS = CefURIEncode(bs->css, false).ToString();

		std::string script;
		script += "const obsCSS = document.createElement('style');";
		script += "obsCSS.appendChild(document.createTextNode("
			  "decodeURIComponent(\"" +
			  uriEncodedCSS + "\")));";
		script += "document.querySelector('head').appendChild(obsCSS);";

		frame->ExecuteJavaScript(script, "", 0);
	}
}

bool BrowserClient::OnConsoleMessage(CefRefPtr<CefBrowser>, cef_log_severity_t level, const CefString &message,
				     const CefString &source, int line)
{
	int errorLevel = LOG_INFO;
	const char *code = "Info";
	switch (level) {
	case LOGSEVERITY_ERROR:
		errorLevel = LOG_WARNING;
		code = "Error";
		break;
	case LOGSEVERITY_FATAL:
		errorLevel = LOG_ERROR;
		code = "Fatal";
		break;
	default:
		return false;
	}

	const char *sourceName = "<unknown>";

	if (bs && bs->source)
		sourceName = obs_source_get_name(bs->source);

	blog(errorLevel, "[obs-browser: '%s'] %s: %s (%s:%d)", sourceName, code, message.ToString().c_str(),
	     source.ToString().c_str(), line);
	return false;
}
