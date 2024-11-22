#include "aja-ui-main.h"
#include "AJAOutputUI.h"

#include "../../../plugins/aja/aja-ui-props.hpp"
#include "../../../plugins/aja/aja-card-manager.hpp"
#include "../../../plugins/aja/aja-routing.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QAction>
#include <util/util.hpp>
#include <util/platform.h>
#include <media-io/video-io.h>
#include <media-io/video-frame.h>
#include <ajantv2/includes/ntv2devicescanner.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("aja-output-ui", "en-US")

static aja::CardManager *cardManager = nullptr;
static AJAOutputUI *ajaOutputUI = nullptr;
static obs_output_t *output = nullptr;

bool main_output_running = false;
bool preview_output_running = false;

struct preview_output {
	bool enabled;
	obs_source_t *current_source;
	obs_output_t *output;

	video_t *video_queue;
	gs_texrender_t *texrender;
	gs_stagesurf_t *stagesurface;
	uint8_t *video_data;
	uint32_t video_linesize;

	obs_video_info ovi;
};

static struct preview_output context = {0};

OBSData load_settings(const char *filename)
{
	BPtr<char> path = obs_module_get_config_path(obs_current_module(), filename);
	BPtr<char> jsonData = os_quick_read_utf8_file(path);
	if (!!jsonData) {
		obs_data_t *data = obs_data_create_from_json(jsonData);
		OBSData dataRet(data);
		obs_data_release(data);

		return dataRet;
	}

	return nullptr;
}

void output_stop()
{
	obs_output_stop(output);
	obs_output_release(output);
	main_output_running = false;
	ajaOutputUI->OutputStateChanged(false);
}

void output_start()
{
	OBSData settings = load_settings(kProgramPropsFilename);

	if (settings != nullptr) {
		output = obs_output_create("aja_output", kProgramOutputID, settings, NULL);

		bool started = obs_output_start(output);
		obs_data_release(settings);

		main_output_running = started;

		ajaOutputUI->OutputStateChanged(started);

		if (!started)
			output_stop();
	}
}

void output_toggle()
{
	if (main_output_running)
		output_stop();
	else
		output_start();
}

void on_preview_scene_changed(enum obs_frontend_event event, void *param);
void render_preview_source(void *param, uint32_t cx, uint32_t cy);

void preview_output_stop()
{
	obs_output_stop(context.output);
	obs_output_release(context.output);

	obs_remove_main_render_callback(render_preview_source, &context);
	obs_frontend_remove_event_callback(on_preview_scene_changed, &context);

	obs_source_release(context.current_source);

	obs_enter_graphics();
	gs_stagesurface_destroy(context.stagesurface);
	gs_texrender_destroy(context.texrender);
	obs_leave_graphics();

	video_output_close(context.video_queue);

	preview_output_running = false;
	ajaOutputUI->PreviewOutputStateChanged(false);
}

void preview_output_start()
{
	OBSData settings = load_settings(kPreviewPropsFilename);

	if (settings != nullptr) {
		context.output = obs_output_create("aja_output", kPreviewOutputID, settings, NULL);

		obs_get_video_info(&context.ovi);

		uint32_t width = context.ovi.base_width;
		uint32_t height = context.ovi.base_height;

		obs_enter_graphics();
		context.texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
		context.stagesurface = gs_stagesurface_create(width, height, GS_BGRA);
		obs_leave_graphics();

		const video_output_info *mainVOI = video_output_get_info(obs_get_video());

		video_output_info vi = {0};
		vi.format = VIDEO_FORMAT_BGRA;
		vi.width = width;
		vi.height = height;
		vi.fps_den = context.ovi.fps_den;
		vi.fps_num = context.ovi.fps_num;
		vi.cache_size = 16;
		vi.colorspace = mainVOI->colorspace;
		vi.range = mainVOI->range;
		vi.name = kPreviewOutputID;

		video_output_open(&context.video_queue, &vi);

		obs_frontend_add_event_callback(on_preview_scene_changed, &context);
		if (obs_frontend_preview_program_mode_active()) {
			context.current_source = obs_frontend_get_current_preview_scene();
		} else {
			context.current_source = obs_frontend_get_current_scene();
		}
		obs_add_main_render_callback(render_preview_source, &context);

		obs_output_set_media(context.output, context.video_queue, obs_get_audio());
		bool started = obs_output_start(context.output);

		obs_data_release(settings);

		preview_output_running = started;
		ajaOutputUI->PreviewOutputStateChanged(started);

		if (!started)
			preview_output_stop();
	}
}

void preview_output_toggle()
{
	if (preview_output_running)
		preview_output_stop();
	else
		preview_output_start();
}

void populate_misc_device_list(obs_property_t *list, aja::CardManager *cardManager, NTV2DeviceID &firstDeviceID)
{
	for (const auto &iter : *cardManager) {
		if (!iter.second)
			continue;
		if (firstDeviceID == DEVICE_ID_NOTFOUND)
			firstDeviceID = iter.second->GetDeviceID();
		obs_property_list_add_string(list, iter.second->GetDisplayName().c_str(),
					     iter.second->GetCardID().c_str());
	}
}

void populate_multi_view_audio_sources(obs_property_t *list, NTV2DeviceID id)
{
	obs_property_list_clear(list);
	const QList<NTV2InputSource> kMultiViewAudioInputs = {
		NTV2_INPUTSOURCE_SDI1,
		NTV2_INPUTSOURCE_SDI2,
		NTV2_INPUTSOURCE_SDI3,
		NTV2_INPUTSOURCE_SDI4,
	};
	for (const auto &inp : kMultiViewAudioInputs) {
		if (NTV2DeviceCanDoInputSource(id, inp)) {
			std::string inputSourceStr = NTV2InputSourceToString(inp, true);
			obs_property_list_add_int(list, inputSourceStr.c_str(), (long long)inp);
		}
	}
}

bool on_misc_device_selected(void *data, obs_properties_t *props, obs_property_t *, obs_data_t *settings)
{
	const char *cardID = obs_data_get_string(settings, kUIPropDevice.id);
	if (!cardID || !cardID[0])
		return false;
	aja::CardManager *cardManager = (aja::CardManager *)data;
	if (!cardManager)
		return false;
	auto cardEntry = cardManager->GetCardEntry(cardID);
	if (!cardEntry)
		return false;

	NTV2DeviceID deviceID = cardEntry->GetDeviceID();
	bool enableMultiViewUI = NTV2DeviceCanDoHDMIMultiView(deviceID);
	obs_property_t *multiViewCheckbox = obs_properties_get(props, kUIPropMultiViewEnable.id);
	obs_property_t *multiViewAudioSource = obs_properties_get(props, kUIPropMultiViewAudioSource.id);
	populate_multi_view_audio_sources(multiViewAudioSource, deviceID);
	obs_property_set_enabled(multiViewCheckbox, enableMultiViewUI);
	obs_property_set_enabled(multiViewAudioSource, enableMultiViewUI);
	return true;
}

static void toggle_multi_view(CNTV2Card *card, NTV2InputSource src, bool enable)
{
	std::ostringstream oss;
	for (int i = 0; i < 4; i++) {
		std::string datastream = std::to_string(i);
		oss << "sdi[" << datastream << "][0]->hdmi[0][" << datastream << "];";
	}

	NTV2DeviceID deviceId = card->GetDeviceID();
	NTV2AudioSystem audioSys = NTV2InputSourceToAudioSystem(src);
	if (NTV2DeviceCanDoHDMIMultiView(deviceId)) {
		NTV2XptConnections cnx;
		if (aja::Routing::ParseRouteString(oss.str(), cnx)) {
			card->SetMultiRasterBypassEnable(!enable);
			if (enable) {
				card->ApplySignalRoute(cnx, false);
				if (NTV2DeviceCanDoAudioMixer(deviceId)) {
					card->SetAudioMixerInputAudioSystem(NTV2_AudioMixerInputMain, audioSys);
					card->SetAudioMixerInputChannelSelect(NTV2_AudioMixerInputMain,
									      NTV2_AudioChannel1_2);
					card->SetAudioMixerInputChannelsMute(NTV2_AudioMixerInputAux1,
									     NTV2AudioChannelsMuteAll);
					card->SetAudioMixerInputChannelsMute(NTV2_AudioMixerInputAux2,
									     NTV2AudioChannelsMuteAll);
				}
				card->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_ON, audioSys);
				card->SetAudioOutputMonitorSource(NTV2_AudioChannel1_2, audioSys);
				card->SetHDMIOutAudioChannels(NTV2_HDMIAudio8Channels);
				card->SetHDMIOutAudioSource2Channel(NTV2_AudioChannel1_2, audioSys);
				card->SetHDMIOutAudioSource8Channel(NTV2_AudioChannel1_8, audioSys);
			} else {
				card->RemoveConnections(cnx);
			}
		}
	}
}

bool on_multi_view_toggle(void *data, obs_properties_t *, obs_property_t *, obs_data_t *settings)
{
	bool multiViewEnabled = obs_data_get_bool(settings, kUIPropMultiViewEnable.id) && !main_output_running &&
				!preview_output_running;
	const int audioInputSource = obs_data_get_int(settings, kUIPropMultiViewAudioSource.id);
	const char *cardID = obs_data_get_string(settings, kUIPropDevice.id);
	if (!cardID || !cardID[0])
		return false;
	aja::CardManager *cardManager = (aja::CardManager *)data;
	if (!cardManager)
		return false;
	CNTV2Card *card = cardManager->GetCard(cardID);
	if (!card)
		return false;

	NTV2InputSource inputSource = (NTV2InputSource)audioInputSource;
	toggle_multi_view(card, inputSource, multiViewEnabled);

	return true;
}

// MISC PROPS

void on_preview_scene_changed(enum obs_frontend_event event, void *param)
{
	auto ctx = (struct preview_output *)param;
	switch (event) {
	case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
	case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
		obs_source_release(ctx->current_source);
		ctx->current_source = obs_frontend_get_current_preview_scene();
		break;
	case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
		obs_source_release(ctx->current_source);
		ctx->current_source = obs_frontend_get_current_scene();
		break;
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		if (!obs_frontend_preview_program_mode_active()) {
			obs_source_release(ctx->current_source);
			ctx->current_source = obs_frontend_get_current_scene();
		}
		break;
	default:
		break;
	}
}

void render_preview_source(void *param, uint32_t, uint32_t)
{
	auto ctx = (struct preview_output *)param;

	if (!ctx->current_source)
		return;

	uint32_t width = obs_source_get_base_width(ctx->current_source);
	uint32_t height = obs_source_get_base_height(ctx->current_source);

	gs_texrender_reset(ctx->texrender);

	if (gs_texrender_begin(ctx->texrender, width, height)) {
		struct vec4 background;
		vec4_zero(&background);

		gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
		gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

		obs_source_video_render(ctx->current_source);

		gs_blend_state_pop();
		gs_texrender_end(ctx->texrender);

		struct video_frame output_frame;
		if (video_output_lock_frame(ctx->video_queue, &output_frame, 1, os_gettime_ns())) {
			gs_stage_texture(ctx->stagesurface, gs_texrender_get_texture(ctx->texrender));

			if (gs_stagesurface_map(ctx->stagesurface, &ctx->video_data, &ctx->video_linesize)) {
				uint32_t linesize = output_frame.linesize[0];
				for (uint32_t i = 0; i < ctx->ovi.base_height; i++) {
					uint32_t dst_offset = linesize * i;
					uint32_t src_offset = ctx->video_linesize * i;
					memcpy(output_frame.data[0] + dst_offset, ctx->video_data + src_offset,
					       linesize);
				}

				gs_stagesurface_unmap(ctx->stagesurface);
				ctx->video_data = nullptr;
			}

			video_output_unlock_frame(ctx->video_queue);
		}
	}
}

void addOutputUI(void)
{
	QAction *action = (QAction *)obs_frontend_add_tools_menu_qaction("AJA I/O Device Output");

	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();

	obs_frontend_push_ui_translation(obs_module_get_string);
	ajaOutputUI = new AJAOutputUI(window);
	obs_frontend_pop_ui_translation();

	auto cb = []() {
		ajaOutputUI->ShowHideDialog();
	};

	action->connect(action, &QAction::triggered, cb);
}

static void OBSEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		OBSData settings = load_settings(kProgramPropsFilename);
		if (settings && obs_data_get_bool(settings, kUIPropAutoStartOutput.id))
			output_start();

		OBSData previewSettings = load_settings(kPreviewPropsFilename);
		if (previewSettings && obs_data_get_bool(previewSettings, kUIPropAutoStartOutput.id))
			preview_output_start();

		OBSData miscSettings = load_settings(kMiscPropsFilename);
		if (miscSettings && ajaOutputUI) {
			on_multi_view_toggle(ajaOutputUI->GetCardManager(), nullptr, nullptr, miscSettings);
		}
	} else if (event == OBS_FRONTEND_EVENT_EXIT) {
		if (main_output_running)
			output_stop();
		if (preview_output_running)
			preview_output_stop();
	}
}

static void aja_loaded(void * /* data */, calldata_t *calldata)
{
	// Receive CardManager pointer from the main AJA plugin
	calldata_get_ptr(calldata, "card_manager", &cardManager);
	if (ajaOutputUI)
		ajaOutputUI->SetCardManager(cardManager);
}

bool obs_module_load(void)
{
	CNTV2DeviceScanner scanner;
	auto numDevices = scanner.GetNumDevices();
	if (numDevices == 0) {
		blog(LOG_WARNING, "No AJA devices found, skipping loading AJA UI plugin");
		return false;
	}

	// Signal to wait for AJA plugin to finish loading so we can access the CardManager instance
	auto signal_handler = obs_get_signal_handler();
	signal_handler_add(signal_handler, "void aja_loaded(ptr card_manager)");
	signal_handler_connect(signal_handler, "aja_loaded", aja_loaded, nullptr);

	addOutputUI();

	obs_frontend_add_event_callback(OBSEvent, nullptr);

	return true;
}
