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
	obs_module_text("AJACapture.Device"),
	"",
};

static const UIProperty kUIPropOutputModule = {
	"aja_output",
	obs_module_text("AJAOutput.Device"),
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
	obs_module_text("Device"),
	"",
};

static const UIProperty kUIPropOutput = {
	"ui_prop_output",
	obs_module_text("Output"),
	"",
};

static const UIProperty kUIPropInput = {
	"ui_prop_input",
	obs_module_text("Input"),
	"",
};

static const UIProperty kUIPropIOSelect = {"ui_prop_select_input",
					   obs_module_text("IOSelect"), ""};

static const UIProperty kUIPropSDI4KTransport = {
	"ui_prop_sdi_transport",
	obs_module_text("SDI4KTransport"),
	"",
};

static const UIProperty kUIPropVideoFormatSelect = {
	"ui_prop_vid_fmt",
	obs_module_text("VideoFormat"),
	"",
};

static const UIProperty kUIPropPixelFormatSelect = {
	"ui_prop_pix_fmt",
	obs_module_text("PixelFormat"),
	"",
};

static const UIProperty kUIPropAutoStartOutput = {
	"ui_prop_auto_start_output",
	obs_module_text("AutoStart"),
	"",
};

static const UIProperty kUIPropDeactivateWhenNotShowing = {
	"ui_prop_deactivate_when_not_showing",
	obs_module_text("DeactivateWhenNotShowing"),
	"",
};

static const UIProperty kUIPropBuffering = {
	"ui_prop_buffering",
	obs_module_text("Buffering"),
	"",
};
