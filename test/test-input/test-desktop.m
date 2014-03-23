#include <stdlib.h>
#include <obs.h>
#include <util/threading.h>
#include <pthread.h>

#import <CoreGraphics/CGDisplayStream.h>
#import <Cocoa/Cocoa.h>

struct display_capture {
	samplerstate_t sampler;
	effect_t draw_effect;
	texture_t tex;

	unsigned display;
	uint32_t width, height;

	os_event_t disp_finished;
	CGDisplayStreamRef disp;
	IOSurfaceRef current, prev;

	pthread_mutex_t mutex;
};

static void destroy_display_stream(struct display_capture *dc)
{
	if (dc->disp) {
		CGDisplayStreamStop(dc->disp);
		os_event_wait(dc->disp_finished);
	}

	if (dc->tex) {
		texture_destroy(dc->tex);
		dc->tex = NULL;
	}

	if (dc->current) {
		IOSurfaceDecrementUseCount(dc->current);
		CFRelease(dc->current);
		dc->current = NULL;
	}

	if (dc->disp) {
		CFRelease(dc->disp);
		dc->disp = NULL;
	}

	os_event_destroy(dc->disp_finished);
}

static void display_capture_destroy(void *data)
{
	struct display_capture *dc = data;

	if (!dc)
		return;

	pthread_mutex_lock(&dc->mutex);
	gs_entercontext(obs_graphics());

	destroy_display_stream(dc);

	if (dc->sampler)
		samplerstate_destroy(dc->sampler);
	if (dc->draw_effect)
		effect_destroy(dc->draw_effect);

	gs_leavecontext();

	pthread_mutex_destroy(&dc->mutex);
	bfree(dc);
}

static bool init_display_stream(struct display_capture *dc)
{
	if (dc->display >= [NSScreen screens].count)
		return false;

	NSScreen *screen = [NSScreen screens][dc->display];

	NSRect frame = [screen convertRectToBacking:screen.frame];

	dc->width = frame.size.width;
	dc->height = frame.size.height;

	NSNumber *screen_num = screen.deviceDescription[@"NSScreenNumber"];
	CGDirectDisplayID disp_id = (CGDirectDisplayID)screen_num.pointerValue;

	NSDictionary *dict = @{
		(__bridge NSString*)kCGDisplayStreamSourceRect:
		(__bridge NSDictionary*)CGRectCreateDictionaryRepresentation(
				CGRectMake(0, 0, dc->width, dc->height)),
		(__bridge NSString*)kCGDisplayStreamQueueDepth: @5
	};

	os_event_init(&dc->disp_finished, OS_EVENT_TYPE_MANUAL);

	dc->disp = CGDisplayStreamCreateWithDispatchQueue(disp_id,
			dc->width, dc->height, 'BGRA',
			(__bridge CFDictionaryRef)dict,
			dispatch_queue_create(NULL, NULL),
			^(CGDisplayStreamFrameStatus status, uint64_t
				displayTime, IOSurfaceRef frameSurface,
				CGDisplayStreamUpdateRef updateRef)
			{
				UNUSED_PARAMETER(displayTime);
				UNUSED_PARAMETER(updateRef);

				if (status ==
					kCGDisplayStreamFrameStatusStopped) {
					os_event_signal(dc->disp_finished);
					return;
				}

				if (!frameSurface ||
					pthread_mutex_trylock(&dc->mutex))
					return;

				if (dc->current) {
					IOSurfaceDecrementUseCount(dc->current);
					CFRelease(dc->current);
					dc->current = NULL;
				}

				dc->current = frameSurface;
				CFRetain(dc->current);
				IOSurfaceIncrementUseCount(dc->current);
				pthread_mutex_unlock(&dc->mutex);
			}
	);

	return !CGDisplayStreamStart(dc->disp);
}

static void *display_capture_create(obs_data_t settings,
		obs_source_t source)
{
	UNUSED_PARAMETER(source);
	UNUSED_PARAMETER(settings);

	struct display_capture *dc = bzalloc(sizeof(struct display_capture));

	gs_entercontext(obs_graphics());

	struct gs_sampler_info info = {
		.filter = GS_FILTER_LINEAR,
		.address_u = GS_ADDRESS_CLAMP,
		.address_v = GS_ADDRESS_CLAMP,
		.address_w = GS_ADDRESS_CLAMP,
		.max_anisotropy = 1,
	};
	dc->sampler = gs_create_samplerstate(&info);
	if (!dc->sampler)
		goto fail;

	char *effect_file = obs_find_plugin_file("test-input/draw_rect.effect");
	dc->draw_effect = gs_create_effect_from_file(effect_file, NULL);
	bfree(effect_file);
	if (!dc->draw_effect)
		goto fail;

	gs_leavecontext();

	dc->display = obs_data_getint(settings, "display");
	pthread_mutex_init(&dc->mutex, NULL);

	if (!init_display_stream(dc))
		goto fail;

	return dc;

fail:
	gs_leavecontext();
	display_capture_destroy(dc);
	return NULL;
}

static void display_capture_video_render(void *data, effect_t effect)
{
	UNUSED_PARAMETER(effect);

	struct display_capture *dc = data;

	pthread_mutex_lock(&dc->mutex);

	if (dc->prev != dc->current) {
		if (dc->tex)
			texture_rebind_iosurface(dc->tex, dc->current);
		else
			dc->tex = gs_create_texture_from_iosurface(
					dc->current);
		dc->prev = dc->current;
	}

	if (!dc->tex) goto fail;

	gs_load_samplerstate(dc->sampler, 0);
	technique_t tech = effect_gettechnique(dc->draw_effect, "Default");
	effect_settexture(dc->draw_effect,
			effect_getparambyidx(dc->draw_effect, 1),
			dc->tex);
	technique_begin(tech);
	technique_beginpass(tech, 0);

	gs_draw_sprite(dc->tex, 0, 0, 0);

	technique_endpass(tech);
	technique_end(tech);

fail:
	pthread_mutex_unlock(&dc->mutex);
}

static const char *display_capture_getname(const char *locale)
{
	UNUSED_PARAMETER(locale);
	return "Display Capture";
}

static uint32_t display_capture_getwidth(void *data)
{
	struct display_capture *dc = data;
	return dc->width;
}

static uint32_t display_capture_getheight(void *data)
{
	struct display_capture *dc = data;
	return dc->height;
}

static void display_capture_defaults(obs_data_t settings)
{
	obs_data_set_default_int(settings, "display", 0);
}

static void display_capture_update(void *data, obs_data_t settings)
{
	struct display_capture *dc = data;
	unsigned display = obs_data_getint(settings, "display");
	if (dc->display == display)
		return;

	gs_entercontext(obs_graphics());

	destroy_display_stream(dc);
	dc->display = display;
	init_display_stream(dc);

	gs_leavecontext();
}

static obs_properties_t display_capture_properties(char const *locale)
{
	UNUSED_PARAMETER(locale);
	obs_properties_t props = obs_properties_create();

	obs_property_t list = obs_properties_add_list(props,
			"display", "Display",
			OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	for (unsigned i = 0; i < [NSScreen screens].count; i++)
	{
		char buf[10];
		sprintf(buf, "%u", i);
		obs_property_list_add_item(list, buf, buf);
	}

	return props;
}

struct obs_source_info display_capture_info = {
	.id           = "display_capture",
	.type         = OBS_SOURCE_TYPE_INPUT,
	.getname      = display_capture_getname,

	.create       = display_capture_create,
	.destroy      = display_capture_destroy,

	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW,
	.video_render = display_capture_video_render,

	.getwidth     = display_capture_getwidth,
	.getheight    = display_capture_getheight,

	.defaults     = display_capture_defaults,
	.properties   = display_capture_properties,
	.update       = display_capture_update,
};
