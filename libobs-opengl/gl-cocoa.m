/******************************************************************************
    Copyright (C) 2013 by Ruwen Hahn <palana@stunned.de>

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

#include "gl-subsystem.h"
#include <OpenGL/OpenGL.h>

#import <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#define OBS_USE_HDR 0

struct gl_platform {
	NSOpenGLContext *context;
	id<MTLDevice> metalDevice;
	id<MTLCommandQueue> commandQueue;
};

@interface OpenGLMetalInteropTexture : NSObject

- (nonnull instancetype)initWithDevice:(gs_device_t *)device
			 openGLContext:(nonnull NSOpenGLContext *)glContext
				format:(enum gs_color_format)format
				  size:(CGSize)size;

@property (readonly, nonnull, nonatomic) id<MTLDevice> metalDevice;
@property (readonly, nonnull, nonatomic) id<MTLTexture> metalTexture;

@property (readonly, nonnull, nonatomic) NSOpenGLContext *openGLContext;
@property (readonly, nonatomic) gs_texture_t *openGLTexture;

@property (readonly, nonatomic) CGSize size;

@end

typedef struct {
	int cvPixelFormat;
	MTLPixelFormat mtlFormat;
	GLuint glInternalFormat;
	GLuint glFormat;
	GLuint glType;
} TextureFormatInfo;

const TextureFormatInfo *const
textureFormatInfoFromMetalPixelFormat(MTLPixelFormat pixelFormat)
{
	static const TextureFormatInfo InteropFormatTable[] = {
		{kCVPixelFormatType_32BGRA, MTLPixelFormatBGRA8Unorm_sRGB,
		 GL_SRGB8_ALPHA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV},
		{kCVPixelFormatType_64RGBAHalf, MTLPixelFormatRGBA16Float,
		 GL_RGBA, GL_RGBA, GL_HALF_FLOAT},
	};

	static const size_t NumInteropFormats =
		sizeof(InteropFormatTable) / sizeof(TextureFormatInfo);

	for (int i = 0; i < NumInteropFormats; i++) {
		if (pixelFormat == InteropFormatTable[i].mtlFormat) {
			return &InteropFormatTable[i];
		}
	}

	return NULL;
}

@implementation OpenGLMetalInteropTexture {
	const TextureFormatInfo *_formatInfo;
	CVPixelBufferRef _CVPixelBuffer;
	CVMetalTextureRef _CVMTLTexture;

	CVOpenGLTextureCacheRef _CVGLTextureCache;
	CVOpenGLTextureRef _CVGLTexture;
	CGLPixelFormatObj _CGLPixelFormat;

	CVMetalTextureCacheRef _CVMTLTextureCache;

	CGSize _size;
}

- (nonnull instancetype)initWithDevice:(gs_device_t *)device
			 openGLContext:(nonnull NSOpenGLContext *)glContext
				format:(enum gs_color_format)format
				  size:(CGSize)size
{
	self = [super init];
	if (self) {
		const MTLPixelFormat mtlPixelFormat =
			(format == GS_RGBA16F) ? MTLPixelFormatRGBA16Float
					       : MTLPixelFormatBGRA8Unorm_sRGB;
		_formatInfo =
			textureFormatInfoFromMetalPixelFormat(mtlPixelFormat);

		NSAssert(_formatInfo, @"Metal Format supplied not supported");

		_size = size;
		_metalDevice = device->plat->metalDevice;
		_openGLContext = glContext;
		_CGLPixelFormat = _openGLContext.pixelFormat.CGLPixelFormatObj;

		NSDictionary *cvBufferProperties = @{
			(__bridge NSString *)
			kCVPixelBufferOpenGLCompatibilityKey: @YES,
			(__bridge NSString *)
			kCVPixelBufferMetalCompatibilityKey: @YES,
		};
		CVReturn cvret = CVPixelBufferCreate(
			kCFAllocatorDefault, size.width, size.height,
			_formatInfo->cvPixelFormat,
			(__bridge CFDictionaryRef)cvBufferProperties,
			&_CVPixelBuffer);

		NSAssert(cvret == kCVReturnSuccess,
			 @"Failed to create CVPixelBuffer");

		[self createGLTexture:device format:format];
		[self createMetalTexture];
	}
	return self;
}

extern gs_texture_t *device_drawable_wrap(gs_device_t *device, GLuint tex,
					  uint32_t width, uint32_t height,
					  enum gs_color_format color_format);

- (void)createGLTexture:(gs_device_t *)device
		 format:(enum gs_color_format)format
{
	CVReturn cvret = CVOpenGLTextureCacheCreate(
		kCFAllocatorDefault, nil, _openGLContext.CGLContextObj,
		_CGLPixelFormat, nil, &_CVGLTextureCache);

	NSAssert(cvret == kCVReturnSuccess,
		 @"Failed to create OpenGL Texture Cache");

	cvret = CVOpenGLTextureCacheCreateTextureFromImage(kCFAllocatorDefault,
							   _CVGLTextureCache,
							   _CVPixelBuffer, nil,
							   &_CVGLTexture);

	NSAssert(cvret == kCVReturnSuccess,
		 @"Failed to create OpenGL Texture From Image");

	_openGLTexture = device_drawable_wrap(
		device, CVOpenGLTextureGetName(_CVGLTexture), _size.width,
		_size.height, format);
}

- (void)createMetalTexture
{
	CVReturn cvret = CVMetalTextureCacheCreate(kCFAllocatorDefault, nil,
						   _metalDevice, nil,
						   &_CVMTLTextureCache);

	NSAssert(cvret == kCVReturnSuccess,
		 @"Failed to create Metal texture cache");

	cvret = CVMetalTextureCacheCreateTextureFromImage(
		kCFAllocatorDefault, _CVMTLTextureCache, _CVPixelBuffer, nil,
		_formatInfo->mtlFormat, _size.width, _size.height, 0,
		&_CVMTLTexture);

	NSAssert(cvret == kCVReturnSuccess,
		 @"Failed to create CoreVideo Metal texture from image");

	_metalTexture = CVMetalTextureGetTexture(_CVMTLTexture);

	NSAssert(_metalTexture,
		 @"Failed to create Metal texture CoreVideo Metal Texture");
}

@end

struct gl_windowinfo {
	dispatch_semaphore_t inFlightSemaphore;
	NSView *view;
	CAMetalLayer *metalLayer;
	NSOpenGLContext *context;
	OpenGLMetalInteropTexture *interopTexture;
};

static NSOpenGLContext *gl_context_create(NSOpenGLContext *share)
{
	static const NSOpenGLPixelFormatAttribute attributes[] = {
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAOpenGLProfile,
		NSOpenGLProfileVersion3_2Core,
		0,
	};

	NSOpenGLPixelFormat *const pf =
		[[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	if (!pf) {
		blog(LOG_ERROR, "Failed to create pixel format");
		return NULL;
	}

	NSOpenGLContext *const context =
		[[NSOpenGLContext alloc] initWithFormat:pf shareContext:share];
	[pf release];
	if (!context) {
		blog(LOG_ERROR, "Failed to create context");
		return NULL;
	}

	[context clearDrawable];

	return context;
}

struct gl_platform *gl_platform_create(gs_device_t *device, uint32_t adapter)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(adapter);

	NSOpenGLContext *context = gl_context_create(nil);
	if (!context) {
		blog(LOG_ERROR, "gl_context_create failed");
		return NULL;
	}

	[context makeCurrentContext];
	const bool success = gladLoadGL() != 0;

	if (!success) {
		blog(LOG_ERROR, "gladLoadGL failed");
		[context release];
		return NULL;
	}

	struct gl_platform *plat = bzalloc(sizeof(struct gl_platform));
	plat->context = context;
	plat->metalDevice = MTLCreateSystemDefaultDevice();
	plat->commandQueue = [plat->metalDevice newCommandQueue];
	return plat;
}

void gl_platform_destroy(struct gl_platform *platform)
{
	if (!platform)
		return;

	[platform->commandQueue release];
	platform->commandQueue = nil;
	[platform->metalDevice release];
	platform->metalDevice = nil;
	[platform->context release];
	platform->context = nil;

	bfree(platform);
}

bool gl_platform_init_swapchain(struct gs_swap_chain *swap)
{
	NSOpenGLContext *parent = swap->device->plat->context;
#if OBS_USE_HDR
	const bool hdr =
		[swap->wi->view window]
			.screen
			.maximumPotentialExtendedDynamicRangeColorComponentValue >
		1.f;
#else
	const bool hdr = false;
#endif
	NSOpenGLContext *context = gl_context_create(parent);
	bool success = context != nil;
	if (success) {
		swap->wi->inFlightSemaphore = dispatch_semaphore_create(1);

		CGLContextObj parent_obj = [parent CGLContextObj];
		CGLLockContext(parent_obj);

		CGLContextObj context_obj = [context CGLContextObj];
		CGLLockContext(context_obj);

		[context makeCurrentContext];
		struct gs_init_data *init_data = &swap->info;
		const enum gs_color_format format = hdr ? GS_RGBA16F : GS_BGRA;
		swap->wi->interopTexture = [[OpenGLMetalInteropTexture alloc]
			initWithDevice:swap->device
			 openGLContext:context
				format:format
				  size:CGSizeMake(init_data->cx,
						  init_data->cy)];
		glFlush();
		[NSOpenGLContext clearCurrentContext];

		CGLUnlockContext(context_obj);

		CGLUnlockContext(parent_obj);

		swap->wi->context = context;
	}

	return success;
}

void gl_platform_cleanup_swapchain(struct gs_swap_chain *swap)
{
	NSOpenGLContext *parent = swap->device->plat->context;
	CGLContextObj parent_obj = [parent CGLContextObj];
	CGLLockContext(parent_obj);

	NSOpenGLContext *context = swap->wi->context;
	CGLContextObj context_obj = [context CGLContextObj];
	CGLLockContext(context_obj);

	[context makeCurrentContext];
	[swap->wi->interopTexture release];
	swap->wi->interopTexture = nil;
	glFlush();
	[NSOpenGLContext clearCurrentContext];

	CGLUnlockContext(context_obj);

	swap->wi->context = nil;

	CGLUnlockContext(parent_obj);
}

struct gl_windowinfo *gl_windowinfo_create(gs_device_t *device,
					   const struct gs_init_data *info)
{
	if (!info)
		return NULL;

	if (!info->window.view)
		return NULL;

	struct gl_windowinfo *wi = bzalloc(sizeof(struct gl_windowinfo));
	wi->view = info->window.view;

#if OBS_USE_HDR
	const bool hdr =
		[info->window.view window]
			.screen
			.maximumPotentialExtendedDynamicRangeColorComponentValue >
		1.f;
#else
	const bool hdr = false;
#endif

	CAMetalLayer *metalLayer = [CAMetalLayer layer];
	if (hdr) {
		metalLayer.wantsExtendedDynamicRangeContent = YES;
		metalLayer.colorspace = CGColorSpaceCreateWithName(
			kCGColorSpaceExtendedLinearSRGB);
		metalLayer.pixelFormat = MTLPixelFormatRGBA16Float;
	} else {
		metalLayer.wantsExtendedDynamicRangeContent = NO;
		metalLayer.colorspace =
			CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
		metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
	}
	metalLayer.device = device->plat->metalDevice;
	metalLayer.drawableSize = CGSizeMake(info->cx, info->cy);
	metalLayer.framebufferOnly = NO;
	[wi->view setWantsLayer:YES];
	[wi->view setLayer:metalLayer];
	wi->metalLayer = metalLayer;

	[info->window.view setWantsBestResolutionOpenGLSurface:YES];
	if (hdr) {
		CGColorSpaceRef colorSpaceCG = CGColorSpaceCreateWithName(
			kCGColorSpaceExtendedLinearSRGB);
		NSColorSpace *const colorSpace = [[NSColorSpace alloc]
			initWithCGColorSpace:colorSpaceCG];
		[info->window.view window].colorSpace = colorSpace;
		CGColorSpaceRelease(colorSpaceCG);
	} else {
		CGColorSpaceRef colorSpaceCG =
			CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
		NSColorSpace *const colorSpace = [[NSColorSpace alloc]
			initWithCGColorSpace:colorSpaceCG];
		[info->window.view window].colorSpace = colorSpace;
		CGColorSpaceRelease(colorSpaceCG);
	}

	return wi;
}

void gl_windowinfo_destroy(struct gl_windowinfo *wi)
{
	if (!wi)
		return;

	wi->view = nil;
	bfree(wi);
}

void gl_update(gs_device_t *device)
{
	gs_swapchain_t *swap = device->cur_swap;
	NSOpenGLContext *parent = device->plat->context;
	NSOpenGLContext *context = swap->wi->context;
	dispatch_async(dispatch_get_main_queue(), ^() {
		CGLContextObj parent_obj = [parent CGLContextObj];
		CGLLockContext(parent_obj);

		CGLContextObj context_obj = [context CGLContextObj];
		CGLLockContext(context_obj);

		[context makeCurrentContext];
		[context update];
		struct gs_init_data *info = &swap->info;
		OpenGLMetalInteropTexture *previous = swap->wi->interopTexture;
#if OBS_USE_HDR
		const bool hdr =
			[swap->wi->view window]
				.screen
				.maximumPotentialExtendedDynamicRangeColorComponentValue >
			1.f;
#else
		const bool hdr = false;
#endif
		const enum gs_color_format format = hdr ? GS_RGBA16F : GS_BGRA;
		swap->wi->interopTexture = [[OpenGLMetalInteropTexture alloc]
			initWithDevice:swap->device
			 openGLContext:context
				format:format
				  size:CGSizeMake(info->cx, info->cy)];
		[previous release];
		glFlush();
		[NSOpenGLContext clearCurrentContext];

		CGLUnlockContext(context_obj);

		CGLUnlockContext(parent_obj);

		swap->wi->metalLayer.drawableSize =
			CGSizeMake(info->cx, info->cy);
	});
}

void gl_clear_context(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	[NSOpenGLContext clearCurrentContext];
}

void device_enter_context(gs_device_t *device)
{
	CGLLockContext([device->plat->context CGLContextObj]);

	[device->plat->context makeCurrentContext];
}

void device_leave_context(gs_device_t *device)
{
	glFlush();
	[NSOpenGLContext clearCurrentContext];
	device->cur_render_target = NULL;
	device->cur_zstencil_buffer = NULL;
	device->cur_swap = NULL;
	device->cur_fbo = NULL;

	CGLUnlockContext([device->plat->context CGLContextObj]);
}

void *device_get_device_obj(gs_device_t *device)
{
	return device->plat->context;
}

void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swap)
{
	if (device->cur_swap == swap)
		return;

	device->cur_swap = swap;
	if (swap) {
		const enum gs_color_space space =
			(swap->wi->interopTexture.metalTexture.pixelFormat ==
			 MTLPixelFormatRGBA16Float)
				? GS_CS_709_EXTENDED
				: GS_CS_SRGB;
		device_set_render_target_with_color_space(
			device, swap->wi->interopTexture.openGLTexture, NULL,
			space);
	}
}

bool device_is_present_ready(gs_device_t *device)
{
	return !dispatch_semaphore_wait(device->cur_swap->wi->inFlightSemaphore,
					DISPATCH_TIME_NOW);
}

void device_present(gs_device_t *device)
{
	glFlush();
	[NSOpenGLContext clearCurrentContext];

	id<MTLCommandBuffer> commandBuffer =
		[device->plat->commandQueue commandBuffer];
	commandBuffer.label = @"Drawable Command Buffer";

	__block dispatch_semaphore_t dsema =
		device->cur_swap->wi->inFlightSemaphore;
	[commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
		UNUSED_PARAMETER(buffer);
		dispatch_semaphore_signal(dsema);
	}];

	id<CAMetalDrawable> drawable =
		device->cur_swap->wi->metalLayer.nextDrawable;

	MTLBlitPassDescriptor *blitDescriptor = [MTLBlitPassDescriptor new];
	id<MTLBlitCommandEncoder> blitEncoder =
		[commandBuffer blitCommandEncoderWithDescriptor:blitDescriptor];
	blitEncoder.label = @"Drawable Copy Encoder";
	[blitEncoder
		copyFromTexture:device->cur_swap->wi->interopTexture.metalTexture
		      toTexture:drawable.texture];
	[blitEncoder endEncoding];

	[commandBuffer presentDrawable:drawable];

	[commandBuffer commit];

	[device->plat->context makeCurrentContext];
}

bool device_is_monitor_hdr(gs_device_t *device, void *monitor)
{
	UNUSED_PARAMETER(device);

	NSScreen *const screen = monitor;
	return screen.maximumPotentialExtendedDynamicRangeColorComponentValue >
	       1.f;
}

void gl_getclientsize(const struct gs_swap_chain *swap, uint32_t *width,
		      uint32_t *height)
{
	if (width)
		*width = swap->info.cx;
	if (height)
		*height = swap->info.cy;
}

gs_texture_t *device_texture_create_from_iosurface(gs_device_t *device,
						   void *iosurf)
{
	IOSurfaceRef ref = (IOSurfaceRef)iosurf;
	struct gs_texture_2d *tex = bzalloc(sizeof(struct gs_texture_2d));

	OSType pf = IOSurfaceGetPixelFormat(ref);
	if (pf == 0)
		blog(LOG_ERROR, "Invalid IOSurface Buffer");
	else if (pf != 'BGRA')
		blog(LOG_ERROR, "Unexpected pixel format: %d (%c%c%c%c)", pf,
		     pf >> 24, pf >> 16, pf >> 8, pf);

	const enum gs_color_format color_format = GS_BGRA;

	tex->base.device = device;
	tex->base.type = GS_TEXTURE_2D;
	tex->base.format = GS_BGRA;
	tex->base.levels = 1;
	tex->base.gl_format = convert_gs_format(color_format);
	tex->base.gl_internal_format = convert_gs_internal_format(color_format);
	tex->base.gl_type = GL_UNSIGNED_INT_8_8_8_8_REV;
	tex->base.gl_target = GL_TEXTURE_RECTANGLE_ARB;
	tex->base.is_dynamic = false;
	tex->base.is_render_target = false;
	tex->base.gen_mipmaps = false;
	tex->width = IOSurfaceGetWidth(ref);
	tex->height = IOSurfaceGetHeight(ref);

	if (!gl_gen_textures(1, &tex->base.texture))
		goto fail;

	if (!gl_bind_texture(tex->base.gl_target, tex->base.texture))
		goto fail;

	CGLError err = CGLTexImageIOSurface2D(
		[[NSOpenGLContext currentContext] CGLContextObj],
		tex->base.gl_target, tex->base.gl_internal_format, tex->width,
		tex->height, tex->base.gl_format, tex->base.gl_type, ref, 0);

	if (err != kCGLNoError) {
		blog(LOG_ERROR,
		     "CGLTexImageIOSurface2D: %u, %s"
		     " (device_texture_create_from_iosurface)",
		     err, CGLErrorString(err));

		gl_success("CGLTexImageIOSurface2D");
		goto fail;
	}

	if (!gl_tex_param_i(tex->base.gl_target, GL_TEXTURE_MAX_LEVEL, 0))
		goto fail;

	if (!gl_bind_texture(tex->base.gl_target, 0))
		goto fail;

	return (gs_texture_t *)tex;

fail:
	gs_texture_destroy((gs_texture_t *)tex);
	blog(LOG_ERROR, "device_texture_create_from_iosurface (GL) failed");
	return NULL;
}

gs_texture_t *device_texture_open_shared(gs_device_t *device, uint32_t handle)
{
	gs_texture_t *texture = NULL;
	IOSurfaceRef ref = IOSurfaceLookupFromMachPort((mach_port_t)handle);
	texture = device_texture_create_from_iosurface(device, ref);
	CFRelease(ref);
	return texture;
}

bool device_shared_texture_available(void)
{
	return true;
}

bool gs_texture_rebind_iosurface(gs_texture_t *texture, void *iosurf)
{
	if (!texture)
		return false;

	if (!iosurf)
		return false;

	struct gs_texture_2d *tex = (struct gs_texture_2d *)texture;
	IOSurfaceRef ref = (IOSurfaceRef)iosurf;

	OSType pf = IOSurfaceGetPixelFormat(ref);
	if (pf == 0) {
		blog(LOG_ERROR, "Invalid IOSurface buffer");
	} else {
		if (pf != 'BGRA')
			blog(LOG_ERROR,
			     "Unexpected pixel format: %d (%c%c%c%c)", pf,
			     pf >> 24, pf >> 16, pf >> 8, pf);
	}

	tex->width = IOSurfaceGetWidth(ref);
	tex->height = IOSurfaceGetHeight(ref);

	if (!gl_bind_texture(tex->base.gl_target, tex->base.texture))
		return false;

	CGLError err = CGLTexImageIOSurface2D(
		[[NSOpenGLContext currentContext] CGLContextObj],
		tex->base.gl_target, tex->base.gl_internal_format, tex->width,
		tex->height, tex->base.gl_format, tex->base.gl_type, ref, 0);

	if (err != kCGLNoError) {
		blog(LOG_ERROR,
		     "CGLTexImageIOSurface2D: %u, %s"
		     " (gs_texture_rebind_iosurface)",
		     err, CGLErrorString(err));

		gl_success("CGLTexImageIOSurface2D");
		return false;
	}

	if (!gl_bind_texture(tex->base.gl_target, 0))
		return false;

	return true;
}
