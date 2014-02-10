#include <stdlib.h>
#include "test-desktop.h"

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/CGLIOSurface.h>
#import <OpenGL/CGLMacro.h>
#import <CoreGraphics/CGDisplayStream.h>
#include <pthread.h>


const char *osx_desktop_test_getname(const char *locale)
{
	return "OSX Monitor Capture";
}

static IOSurfaceRef current = NULL,
		    prev = NULL;
static pthread_mutex_t c_mutex;

struct desktop_tex *osx_desktop_test_create(const char *settings,
		obs_source_t source)
{
	struct desktop_tex *rt = bzalloc(sizeof(struct desktop_tex));
	char *effect_file;

	gs_entercontext(obs_graphics());

	struct gs_sampler_info info = {
		.filter = GS_FILTER_LINEAR,
		.address_u = GS_ADDRESS_CLAMP,
		.address_v = GS_ADDRESS_CLAMP,
		.address_w = GS_ADDRESS_CLAMP,
		.max_anisotropy = 1,
	};
	rt->sampler = gs_create_samplerstate(&info);

	effect_file = obs_find_plugin_file("test-input/draw_rect.effect");
	rt->whatever = gs_create_effect_from_file(effect_file, NULL);
	bfree(effect_file);

	if (!rt->whatever) {
		osx_desktop_test_destroy(rt);
		return NULL;
	}

	if ([[NSScreen screens] count] < 1) {
		osx_desktop_test_destroy(rt);
		return NULL;
	}

	NSScreen *screen = [NSScreen screens][0];

	NSRect frame = [screen convertRectToBacking:[screen frame]];

	rt->width = frame.size.width;
	rt->height = frame.size.height;

	pthread_mutex_init(&rt->mutex, NULL);

	NSDictionary *dict = @{
		(__bridge NSString*)kCGDisplayStreamSourceRect:
		(__bridge NSDictionary*)CGRectCreateDictionaryRepresentation(
				CGRectMake(0, 0, rt->width, rt->height)),
		(__bridge NSString*)kCGDisplayStreamQueueDepth: @5
	};

	rt->disp = CGDisplayStreamCreateWithDispatchQueue(CGMainDisplayID(),
			rt->width, rt->height, 'BGRA',
			(__bridge CFDictionaryRef)dict,
			dispatch_queue_create(NULL, NULL),
			^(CGDisplayStreamFrameStatus status, uint64_t
				displayTime, IOSurfaceRef frameSurface,
				CGDisplayStreamUpdateRef updateRef)
			{
				if (!frameSurface ||
					pthread_mutex_trylock(&rt->mutex))
					return;

				if (rt->current) {
					IOSurfaceDecrementUseCount(rt->current);
					CFRelease(rt->current);
					rt->current = NULL;
				}

				rt->current = frameSurface;
				CFRetain(rt->current);
				IOSurfaceIncrementUseCount(rt->current);
				pthread_mutex_unlock(&rt->mutex);
			}
	);

	gs_leavecontext();

	if (CGDisplayStreamStart(rt->disp)) {
		osx_desktop_test_destroy(rt);
		return NULL;
	}

	return rt;
}

void osx_desktop_test_destroy(struct desktop_tex *rt)
{
	if (rt) {
		pthread_mutex_lock(&rt->mutex);
		gs_entercontext(obs_graphics());

		if (current) {
			IOSurfaceDecrementUseCount(rt->current);
			CFRelease(rt->current);
		}
		if (rt->sampler)
			samplerstate_destroy(rt->sampler);
		if (rt->tex)
			texture_destroy(rt->tex);
		CGDisplayStreamStop(rt->disp);
		effect_destroy(rt->whatever);
		bfree(rt);

		gs_leavecontext();
		pthread_mutex_unlock(&rt->mutex);
	}
}

uint32_t osx_desktop_test_get_output_flags(struct desktop_tex *rt)
{
	return SOURCE_VIDEO;
}

void osx_desktop_test_video_render(struct desktop_tex *rt,
		obs_source_t filter_target)
{
	pthread_mutex_lock(&rt->mutex);

	if (rt->prev != rt->current) {
		if (rt->tex)
			texture_rebind_iosurface(rt->tex, rt->current);
		else
			rt->tex = gs_create_texture_from_iosurface(
					rt->current);
		rt->prev = rt->current;
	}

	if (!rt->tex) goto fail;

	gs_load_samplerstate(rt->sampler, 0);
	technique_t tech = effect_gettechnique(rt->whatever, "Default");
	effect_settexture(rt->whatever, effect_getparambyidx(rt->whatever, 1),
			rt->tex);
	technique_begin(tech);
	technique_beginpass(tech, 0);

	gs_draw_sprite(rt->tex, 0, 0, 0);

	technique_endpass(tech);
	technique_end(tech);

fail:
	pthread_mutex_unlock(&rt->mutex);
}

uint32_t osx_desktop_test_getwidth(struct desktop_tex *rt)
{
	return rt->width;
}

uint32_t osx_desktop_test_getheight(struct desktop_tex *rt)
{
	return rt->height;
}
