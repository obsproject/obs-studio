#pragma once

#include "../aja-routing.hpp"

static inline const std::map<HDMIWireFormat, RoutingConfig>
	kHDMIRGBCaptureConfigs = {{HDMIWireFormat::HD_RGB_LFR,
				   {
					   NTV2_MODE_CAPTURE,
					   1,
					   1,
					   false,
					   false,
					   false,
					   false,
					   false,
					   false,
					   false,
					   false,
					   false,
					   false,
					   "hdmi[{ch1}][0]->fb[{ch1}][0];",
				   }}};
