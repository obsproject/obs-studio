#pragma once

#include <obs-module.h>

static const char *kProgramOutputID = "aja_output";
static const char *kPreviewOutputID = "aja_preview_output";

struct UIProperty {
	const char *id;
	const char *text;
	const char *tooltip;
};

static const UIProperty kUIPropCaptureModule = {
	"aja_source",
	"AJACapture.Device",
	"",
};

static const UIProperty kUIPropOutputModule = {
	"aja_output",
	"AJAOutput.Device",
	"",
};

// This is used as an "invisible" property to give the program and preview
// plugin instances an identifier before the output has been created/started.
// This ID is then used by the CardManager class for tracking device channel
// usage across the capture and output plugin instances.
static const UIProperty kUIPropAJAOutputID = {
	"aja_output_id",
	"",
	"",
};

static const UIProperty kUIPropDevice = {
	"ui_prop_device",
	"Device",
	"",
};

static const UIProperty kUIPropOutput = {
	"ui_prop_output",
	"Output",
	"",
};

static const UIProperty kUIPropInput = {
	"ui_prop_input",
	"Input",
	"",
};

// Used for showing "Select..." item in Input/Output selection drop-downs
static const UIProperty kUIPropIOSelectNone = {"ui_prop_select_input",
					       "IOSelect", ""};

static const UIProperty kUIPropSDITransport = {
	"ui_prop_sdi_transport",
	"SDITransport",
	"",
};

static const UIProperty kUIPropSDITransport4K = {
	"ui_prop_sdi_transport_4k",
	"SDITransport4K",
	"",
};

static const UIProperty kUIPropVideoFormatSelect = {
	"ui_prop_vid_fmt",
	"VideoFormat",
	"",
};

static const UIProperty kUIPropPixelFormatSelect = {
	"ui_prop_pix_fmt",
	"PixelFormat",
	"",
};

static const UIProperty kUIPropAutoStartOutput = {
	"ui_prop_auto_start_output",
	"AutoStart",
	"",
};

static const UIProperty kUIPropDeactivateWhenNotShowing = {
	"ui_prop_deactivate_when_not_showing",
	"DeactivateWhenNotShowing",
	"",
};

static const UIProperty kUIPropBuffering = {
	"ui_prop_buffering",
	"Buffering",
	"",
};

static const UIProperty kUIPropMultiViewEnable = {
	"ui_prop_multi_view_enable",
	"Enable Multi View",
	"",
};

static const UIProperty kUIPropMultiViewAudioSource = {
	"ui_prop_multi_view_audio_source",
	"Multi View Audio Source",
	"",
};

static const UIProperty kUIPropChannelFormat = {
	"ui_prop_channel_format",
	"ChannelFormat",
	"",
};

static const UIProperty kUIPropChannelSwap_FC_LFE = {
	"ui_prop_channel_swap_fc_lfe",
	"SwapFC-LFE",
	"SwapFC-LFE.Tooltip",
};

#define TEXT_CHANNEL_FORMAT_NONE obs_module_text("ChannelFormat.None")
#define TEXT_CHANNEL_FORMAT_2_0CH obs_module_text("ChannelFormat.2_0ch")
#define TEXT_CHANNEL_FORMAT_2_1CH obs_module_text("ChannelFormat.2_1ch")
#define TEXT_CHANNEL_FORMAT_4_0CH obs_module_text("ChannelFormat.4_0ch")
#define TEXT_CHANNEL_FORMAT_4_1CH obs_module_text("ChannelFormat.4_1ch")
#define TEXT_CHANNEL_FORMAT_5_1CH obs_module_text("ChannelFormat.5_1ch")
#define TEXT_CHANNEL_FORMAT_7_1CH obs_module_text("ChannelFormat.7_1ch")
