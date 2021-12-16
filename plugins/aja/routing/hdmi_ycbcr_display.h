#pragma once

#include "../aja-routing.hpp"

static inline const std::map<HDMIWireFormat, RoutingConfig>
	kHDMIYCbCrDisplayConfigs = {{HDMIWireFormat::HD_YCBCR_LFR,
				     {
					     NTV2_MODE_DISPLAY,
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
					     "fb[{ch1}][0]->hdmi[0][0];",
				     }},
				    {HDMIWireFormat::UHD_4K_YCBCR_LFR,
				     {
					     NTV2_MODE_DISPLAY,
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
					     "fb[{ch1}][0]->tsi[{ch1}][0];"
					     "fb[{ch1}][1]->tsi[{ch1}][1];"
					     "fb[{ch2}][0]->tsi[{ch2}][0];"
					     "fb[{ch2}][1]->tsi[{ch2}][1];"
					     "tsi[{ch1}][0]->hdmi[0][0];"
					     "tsi[{ch1}][1]->hdmi[0][1];"
					     "tsi[{ch2}][0]->hdmi[0][2];"
					     "tsi[{ch2}][1]->hdmi[0][3];",
				     }},
				    {HDMIWireFormat::TTAP_PRO,
				     {
					     NTV2_MODE_DISPLAY,
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
					     "fb[{ch1}][0]->sdi[{ch1}][0];"
					     "fb[{ch1}][0]->hdmi[{ch1}][0];",
				     }}};
