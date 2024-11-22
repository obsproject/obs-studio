#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QAction>
#include <util/util.hpp>
#include <util/platform.h>
#include <media-io/video-io.h>
#include <media-io/video-frame.h>
#include "DecklinkOutputUI.h"
#include "../../../plugins/decklink/const.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("decklink-output-ui", "en-US")

DecklinkOutputUI *doUI;

bool shutting_down = false;

bool main_output_running = false;
bool preview_output_running = false;

constexpr size_t STAGE_BUFFER_COUNT = 3;

struct decklink_ui_output {
	bool enabled;
	obs_source_t *current_source;
	obs_output_t *output;

	video_t *video_queue;
	gs_texrender_t *texrender_premultiplied;
	gs_texrender_t *texrender;
	gs_stagesurf_t *stagesurfaces[STAGE_BUFFER_COUNT];
	bool surf_written[STAGE_BUFFER_COUNT];
	size_t stage_index;
	uint8_t *video_data;
	uint32_t video_linesize;

	obs_video_info ovi;
};

static struct decklink_ui_output context = {0};
static struct decklink_ui_output context_preview = {0};

OBSData load_settings()
{
	BPtr<char> path = obs_module_get_config_path(obs_current_module(), "decklinkOutputProps.json");
	BPtr<char> jsonData = os_quick_read_utf8_file(path);
	if (!!jsonData) {
		obs_data_t *data = obs_data_create_from_json(jsonData);
		OBSData dataRet(data);
		obs_data_release(data);

		return dataRet;
	}

	return nullptr;
}

static void decklink_ui_tick(void *param, float sec);
static void decklink_ui_render(void *param);

void output_stop()
{
	obs_remove_main_rendered_callback(decklink_ui_render, &context);

	obs_output_stop(context.output);
	obs_output_release(context.output);

	obs_enter_graphics();
	for (gs_stagesurf_t *&surf : context.stagesurfaces) {
		gs_stagesurface_destroy(surf);
		surf = nullptr;
	}
	gs_texrender_destroy(context.texrender);
	context.texrender = nullptr;
	obs_leave_graphics();

	video_output_close(context.video_queue);
	obs_remove_tick_callback(decklink_ui_tick, &context);

	main_output_running = false;

	if (!shutting_down)
		doUI->OutputStateChanged(false);
}

void output_start()
{
	OBSData settings = load_settings();

	if (settings != nullptr) {
		obs_output_t *const output = obs_output_create("decklink_output", "decklink_output", settings, NULL);
		const struct video_scale_info *const conversion = obs_output_get_video_conversion(output);
		if (conversion != nullptr) {
			context.output = output;
			obs_add_tick_callback(decklink_ui_tick, &context);

			obs_get_video_info(&context.ovi);

			const uint32_t width = conversion->width;
			const uint32_t height = conversion->height;

			obs_enter_graphics();
			context.texrender_premultiplied = nullptr;
			context.texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
			for (gs_stagesurf_t *&surf : context.stagesurfaces)
				surf = gs_stagesurface_create(width, height, GS_BGRA);
			obs_leave_graphics();

			for (bool &written : context.surf_written)
				written = false;

			context.stage_index = 0;

			video_output_info vi = {0};
			vi.format = VIDEO_FORMAT_BGRA;
			vi.width = width;
			vi.height = height;
			vi.fps_den = context.ovi.fps_den;
			vi.fps_num = context.ovi.fps_num;
			vi.cache_size = 16;
			vi.colorspace = VIDEO_CS_DEFAULT;
			vi.range = VIDEO_RANGE_FULL;
			vi.name = "decklink_output";

			video_output_open(&context.video_queue, &vi);

			context.current_source = nullptr;
			obs_add_main_rendered_callback(decklink_ui_render, &context);

			obs_output_set_media(context.output, context.video_queue, obs_get_audio());
			bool started = obs_output_start(context.output);

			main_output_running = started;

			if (!shutting_down)
				doUI->OutputStateChanged(started);

			if (!started)
				output_stop();
		} else {
			obs_output_release(output);
		}
	}
}

void output_toggle()
{
	if (main_output_running)
		output_stop();
	else
		output_start();
}

OBSData load_preview_settings()
{
	BPtr<char> path = obs_module_get_config_path(obs_current_module(), "decklinkPreviewOutputProps.json");
	BPtr<char> jsonData = os_quick_read_utf8_file(path);
	if (!!jsonData) {
		obs_data_t *data = obs_data_create_from_json(jsonData);
		OBSData dataRet(data);
		obs_data_release(data);

		return dataRet;
	}

	return nullptr;
}

void on_preview_scene_changed(enum obs_frontend_event event, void *param);

static void decklink_ui_tick(void *param, float /* sec */)
{
	auto ctx = (struct decklink_ui_output *)param;

	if (ctx->texrender_premultiplied)
		gs_texrender_reset(ctx->texrender_premultiplied);
	if (ctx->texrender)
		gs_texrender_reset(ctx->texrender);
}

void preview_output_stop()
{
	obs_remove_main_rendered_callback(decklink_ui_render, &context_preview);
	obs_frontend_remove_event_callback(on_preview_scene_changed, &context_preview);

	obs_output_stop(context_preview.output);
	obs_output_release(context_preview.output);

	obs_source_release(context_preview.current_source);

	obs_enter_graphics();
	for (gs_stagesurf_t *&surf : context_preview.stagesurfaces) {
		gs_stagesurface_destroy(surf);
		surf = nullptr;
	}
	gs_texrender_destroy(context_preview.texrender);
	context_preview.texrender = nullptr;
	gs_texrender_destroy(context_preview.texrender_premultiplied);
	context_preview.texrender_premultiplied = nullptr;
	obs_leave_graphics();

	video_output_close(context_preview.video_queue);
	obs_remove_tick_callback(decklink_ui_tick, &context_preview);

	preview_output_running = false;

	if (!shutting_down)
		doUI->PreviewOutputStateChanged(false);
}

void preview_output_start()
{
	OBSData settings = load_preview_settings();

	if (settings != nullptr) {
		obs_output_t *const output = obs_output_create("decklink_output", "decklink_output", settings, NULL);
		const struct video_scale_info *const conversion = obs_output_get_video_conversion(output);
		if (conversion != nullptr) {
			context_preview.output = output;
			obs_add_tick_callback(decklink_ui_tick, &context_preview);

			obs_get_video_info(&context_preview.ovi);

			const uint32_t width = conversion->width;
			const uint32_t height = conversion->height;

			obs_enter_graphics();
			context_preview.texrender_premultiplied = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
			context_preview.texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
			for (gs_stagesurf_t *&surf : context_preview.stagesurfaces)
				surf = gs_stagesurface_create(width, height, GS_BGRA);
			obs_leave_graphics();

			for (bool &written : context_preview.surf_written)
				written = false;

			context_preview.stage_index = 0;

			video_output_info vi = {0};
			vi.format = VIDEO_FORMAT_BGRA;
			vi.width = width;
			vi.height = height;
			vi.fps_den = context_preview.ovi.fps_den;
			vi.fps_num = context_preview.ovi.fps_num;
			vi.cache_size = 16;
			vi.colorspace = VIDEO_CS_DEFAULT;
			vi.range = VIDEO_RANGE_FULL;
			vi.name = "decklink_preview_output";

			video_output_open(&context_preview.video_queue, &vi);

			obs_frontend_add_event_callback(on_preview_scene_changed, &context_preview);
			if (obs_frontend_preview_program_mode_active()) {
				context_preview.current_source = obs_frontend_get_current_preview_scene();
			} else {
				context_preview.current_source = obs_frontend_get_current_scene();
			}
			obs_add_main_rendered_callback(decklink_ui_render, &context_preview);

			obs_output_set_media(context_preview.output, context_preview.video_queue, obs_get_audio());
			bool started = obs_output_start(context_preview.output);

			preview_output_running = started;
			if (!shutting_down)
				doUI->PreviewOutputStateChanged(started);

			if (!started)
				preview_output_stop();
		} else {
			obs_output_release(output);
		}
	}
}

void preview_output_toggle()
{
	if (preview_output_running)
		preview_output_stop();
	else
		preview_output_start();
}

void on_preview_scene_changed(enum obs_frontend_event event, void *param)
{
	auto ctx = (struct decklink_ui_output *)param;
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

static void decklink_ui_render(void *param)
{
	auto *const ctx = (struct decklink_ui_output *)param;

	uint32_t width = 0;
	uint32_t height = 0;
	gs_texture_t *tex = nullptr;

	if (ctx == &context) {
		if (!main_output_running)
			return;

		tex = obs_get_main_texture();
		if (!tex)
			return;

		width = gs_texture_get_width(tex);
		height = gs_texture_get_height(tex);
	} else if (ctx == &context_preview) {
		if (!preview_output_running)
			return;

		if (!ctx->current_source)
			return;

		width = obs_source_get_base_width(ctx->current_source);
		height = obs_source_get_base_height(ctx->current_source);

		gs_texrender_t *const texrender_premultiplied = ctx->texrender_premultiplied;
		if (!gs_texrender_begin(texrender_premultiplied, width, height))
			return;

		struct vec4 background;
		vec4_zero(&background);

		gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
		gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f, 100.0f);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

		obs_source_video_render(ctx->current_source);

		gs_blend_state_pop();
		gs_texrender_end(texrender_premultiplied);

		tex = gs_texrender_get_texture(texrender_premultiplied);
	} else {
		return;
	}

	const struct video_scale_info *const conversion = obs_output_get_video_conversion(ctx->output);
	const uint32_t scaled_width = conversion->width;
	const uint32_t scaled_height = conversion->height;

	if (!gs_texrender_begin(ctx->texrender, scaled_width, scaled_height))
		return;

	const bool previous = gs_framebuffer_srgb_enabled();
	const bool source_hdr = (ctx->ovi.colorspace == VIDEO_CS_2100_PQ) || (ctx->ovi.colorspace == VIDEO_CS_2100_HLG);
	const bool target_hdr = source_hdr && (conversion->colorspace == VIDEO_CS_2100_PQ);
	gs_enable_framebuffer_srgb(!target_hdr);
	gs_enable_blending(false);

	gs_effect_t *const effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
	gs_effect_set_texture_srgb(gs_effect_get_param_by_name(effect, "image"), tex);
	const char *const tech_name = target_hdr ? "DrawAlphaDivideR10L"
						 : (source_hdr ? "DrawAlphaDivideTonemap" : "DrawAlphaDivide");
	while (gs_effect_loop(effect, tech_name)) {
		gs_effect_set_float(gs_effect_get_param_by_name(effect, "multiplier"),
				    obs_get_video_sdr_white_level() / 10000.f);
		gs_draw_sprite(tex, 0, 0, 0);
	}

	gs_enable_blending(true);
	gs_enable_framebuffer_srgb(previous);

	gs_texrender_end(ctx->texrender);

	const size_t write_stage_index = ctx->stage_index;
	gs_stage_texture(ctx->stagesurfaces[write_stage_index], gs_texrender_get_texture(ctx->texrender));
	ctx->surf_written[write_stage_index] = true;

	const size_t read_stage_index = (write_stage_index + 1) % STAGE_BUFFER_COUNT;
	if (ctx->surf_written[read_stage_index]) {
		struct video_frame output_frame;
		if (video_output_lock_frame(ctx->video_queue, &output_frame, 1, os_gettime_ns())) {
			gs_stagesurf_t *const read_surf = ctx->stagesurfaces[read_stage_index];
			if (gs_stagesurface_map(read_surf, &ctx->video_data, &ctx->video_linesize)) {
				uint32_t linesize = output_frame.linesize[0];
				for (uint32_t i = 0; i < scaled_height; i++) {
					uint32_t dst_offset = linesize * i;
					uint32_t src_offset = ctx->video_linesize * i;
					memcpy(output_frame.data[0] + dst_offset, ctx->video_data + src_offset,
					       linesize);
				}

				gs_stagesurface_unmap(read_surf);
				ctx->video_data = nullptr;
			}

			video_output_unlock_frame(ctx->video_queue);
		}
	}

	ctx->stage_index = read_stage_index;
}

void addOutputUI(void)
{
	QAction *action = (QAction *)obs_frontend_add_tools_menu_qaction(obs_module_text("Decklink Output"));

	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();

	obs_frontend_push_ui_translation(obs_module_get_string);
	doUI = new DecklinkOutputUI(window);
	obs_frontend_pop_ui_translation();

	auto cb = []() {
		doUI->ShowHideDialog();
	};

	action->connect(action, &QAction::triggered, cb);
}

static void OBSEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_FINISHED_LOADING) {
		OBSData settings = load_settings();

		if (settings && obs_data_get_bool(settings, "auto_start"))
			output_start();

		OBSData previewSettings = load_preview_settings();

		if (previewSettings && obs_data_get_bool(previewSettings, "auto_start"))
			preview_output_start();
	} else if (event == OBS_FRONTEND_EVENT_EXIT) {
		shutting_down = true;

		if (preview_output_running)
			preview_output_stop();

		if (main_output_running)
			output_stop();
	}
}

bool obs_module_load(void)
{
	return true;
}

void obs_module_unload(void)
{
	shutting_down = true;

	if (preview_output_running)
		preview_output_stop();

	if (main_output_running)
		output_stop();
}

void obs_module_post_load(void)
{
	if (!obs_get_module("decklink"))
		return;

	addOutputUI();

	obs_frontend_add_event_callback(OBSEvent, nullptr);
}
