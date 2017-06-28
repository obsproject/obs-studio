#include "metal-subsystem.hpp"

#import <QuartzCore/QuartzCore.h>

using namespace std;

gs_texture_2d *gs_swap_chain::GetTarget()
{
	if (!nextTarget)
		return NextTarget();

	return nextTarget.get();
}

gs_texture_2d *gs_swap_chain::NextTarget()
{
	nextDrawable = metalLayer.nextDrawable;
	if (nextDrawable != nil)
		nextTarget.reset(new gs_texture_2d(device,
				nextDrawable.texture));
	else
		nextTarget.reset();

	return nextTarget.get();
}

void gs_swap_chain::Resize(uint32_t cx, uint32_t cy)
{
	initData.cx = cx;
	initData.cy = cy;

	if (cx == 0 || cy == 0) {
		NSRect clientRect = view.layer.frame;
		if (cx == 0) cx = clientRect.size.width - clientRect.origin.x;
		if (cy == 0) cy = clientRect.size.height - clientRect.origin.y;
	}

	metalLayer.drawableSize = CGSizeMake(cx, cy);
}

void gs_swap_chain::Rebuild()
{
	metalLayer.device = device->device;
}

gs_swap_chain::gs_swap_chain(gs_device *device, const gs_init_data *data)
	: gs_obj     (device, gs_type::gs_swap_chain),
	  numBuffers (data->num_backbuffers),
	  view       (data->window.view),
	  initData   (*data)
{
	if (metalLayer.pixelFormat != ConvertGSTextureFormat(data->format))
		blog(LOG_WARNING, "device_stage_texture (Metal): "
			          "pixel format is not matched (RGBA only)");

	metalLayer = [CAMetalLayer layer];
	metalLayer.device = device->device;
	metalLayer.drawableSize = CGSizeMake(initData.cx, initData.cy);
	view.wantsLayer = YES;
	view.layer = metalLayer;
}
