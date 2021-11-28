#pragma once

#include "../aja-routing.hpp"

static inline const std::map<HDMIWireFormat, RoutingConfig>
	kHDMIYCbCrCaptureConfigs = {
		{HDMIWireFormat::HD_YCBCR_LFR,
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
		 }},
		{HDMIWireFormat::UHD_4K_YCBCR_LFR,
		 {
			 NTV2_MODE_CAPTURE,
			 1,
			 2,
			 false,
			 false,
			 false,
			 false,
			 false,
			 false,
			 false,
			 false,
			 false,
			 true,
			 "hdmi[0][0]->tsi[{ch1}][0];"
			 "hdmi[0][1]->tsi[{ch1}][1];"
			 "hdmi[0][2]->tsi[{ch2}][0];"
			 "hdmi[0][3]->tsi[{ch2}][1];"
			 "tsi[{ch1}][0]->fb[{ch1}][0];"
			 "tsi[{ch1}][1]->fb[{ch1}][1];"
			 "tsi[{ch2}][0]->fb[{ch2}][0];"
			 "tsi[{ch2}][1]->fb[{ch2}][1];",
		 }},
};
