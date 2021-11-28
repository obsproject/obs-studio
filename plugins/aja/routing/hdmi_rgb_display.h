#pragma once

#include "../aja-routing.hpp"

static inline const std::map<HDMIWireFormat, RoutingConfig>
	kHDMIRGBDisplayConfigs = {{HDMIWireFormat::HD_RGB_LFR,
				   {NTV2_MODE_DISPLAY, 1, 1, true, false, false,
				    false, false, false, false, false, false,
				    false, "fb[{ch1}][0]->hdmi[0][0];"}},
				  {HDMIWireFormat::TTAP_PRO,
				   {
					   NTV2_MODE_DISPLAY,
					   1,
					   1,
					   true,
					   false,
					   false,
					   false,
					   false,
					   false,
					   false,
					   false,
					   false,
					   false,
					   "fb[{ch1}][2]->dlo[{ch1}][0];"
					   "dlo[{ch1}][0]->sdi[{ch1}][0];"
					   "dlo[{ch1}][1]->sdi[{ch1}][1];"
					   "fb[{ch1}][2]->hdmi[{ch1}][0];",
				   }}};
