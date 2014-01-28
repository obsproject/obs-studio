#include <stdlib.h>
#include "test-desktop.h"

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/CGLIOSurface.h>
#import <OpenGL/CGLMacro.h>
#import <CoreGraphics/CGDisplayStream.h>
#include <pthread.h>


const char *desktop_getname(const char *locale)
{
	return "OSX Monitor Capture";
}

static IOSurfaceRef current = NULL,
		    prev = NULL;
static texture_t tex = NULL;
static pthread_mutex_t c_mutex;

struct desktop_tex *desktop_create(const char *settings, obs_source_t source)
{
	struct desktop_tex *rt = bmalloc(sizeof(struct desktop_tex));
	char *effect_file;

	memset(rt, 0, sizeof(struct desktop_tex));

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
		desktop_destroy(rt);
		return NULL;
	}

	if ([[NSScreen screens] count] < 1) {
		desktop_destroy(rt);
		return NULL;
	}

	NSScreen *screen = [NSScreen screens][0];

	NSRect frame = [screen convertRectToBacking:[screen frame]];

	rt->width = frame.size.width;
	rt->height = frame.size.height;

	printf("%u %u\n", rt->width, rt->height);

	pthread_mutex_init(&c_mutex, NULL);

	NSDictionary *dict = @{
		(__bridge NSString*)kCGDisplayStreamSourceRect:
		(__bridge NSDictionary*)CGRectCreateDictionaryRepresentation(
				CGRectMake(0, 0, rt->width, rt->height)),
		(__bridge NSString*)kCGDisplayStreamQueueDepth: @5
	};

	rt->disp = CGDisplayStreamCreateWithDispatchQueue(CGMainDisplayID(),
			rt->width, rt->height, 'BGRA',
			(__bridge CFDictionaryRef)dict,
			dispatch_queue_create("dispstream", NULL),
			^(CGDisplayStreamFrameStatus status, uint64_t
				displayTime, IOSurfaceRef frameSurface,
				CGDisplayStreamUpdateRef updateRef)
			{
				if (!frameSurface ||
					pthread_mutex_trylock(&c_mutex))
					return;

				if (current) {
					IOSurfaceDecrementUseCount(current);
					CFRelease(current);
					current = NULL;
				}

				current = frameSurface;
				CFRetain(current);
				IOSurfaceIncrementUseCount(current);
				pthread_mutex_unlock(&c_mutex);
			}
	);

	gs_leavecontext();

	if (CGDisplayStreamStart(rt->disp)) {
		desktop_destroy(rt);
		return NULL;
	}

	return rt;
}

void desktop_destroy(struct desktop_tex *rt)
{
	if (rt) {
		pthread_mutex_lock(&c_mutex);
		gs_entercontext(obs_graphics());

		if (current) {
			IOSurfaceDecrementUseCount(current);
			CFRelease(current);
		}
		if (rt->sampler)
			samplerstate_destroy(rt->sampler);
		if (tex)
			texture_destroy(tex);
		CGDisplayStreamStop(rt->disp);
		effect_destroy(rt->whatever);
		bfree(rt);

		gs_leavecontext();
		pthread_mutex_unlock(&c_mutex);
	}
}

uint32_t desktop_get_output_flags(struct desktop_tex *rt)
{
	return SOURCE_VIDEO;
}

void desktop_video_render(struct desktop_tex *rt, obs_source_t filter_target)
{
	pthread_mutex_lock(&c_mutex);

	if (prev != current) {
		if (tex)
			texture_rebind_iosurface(tex, current);
		else
			tex = gs_create_texture_from_iosurface(current);
		prev = current;
	}

	if (!tex) goto fail;

	gs_load_samplerstate(rt->sampler, 0);
	technique_t tech = effect_gettechnique(rt->whatever, "Default");
	effect_settexture(rt->whatever, effect_getparambyidx(rt->whatever, 1),
			tex);
	technique_begin(tech);
	technique_beginpass(tech, 0);

	gs_draw_sprite(tex, 0, 0, 0);

	technique_endpass(tech);
	technique_end(tech);

fail:
	pthread_mutex_unlock(&c_mutex);
}

uint32_t desktop_getwidth(struct desktop_tex *rt)
{
	return rt->width;
}

uint32_t desktop_getheight(struct desktop_tex *rt)
{
	return rt->height;
}
