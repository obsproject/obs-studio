#include "aja-common.hpp"
#include "aja-presets.hpp"

#include <ajantv2/includes/ntv2devicefeatures.h>
#include <ajantv2/includes/ntv2utils.h>

namespace aja {

RoutingConfigurator::RoutingConfigurator()
{
	build_preset_table();
}

void RoutingConfigurator::AddPreset(const std::string &name,
				    const RoutingPreset &preset)
{
	if (m_presets.find(name) != m_presets.end())
		return;
	m_presets.insert(RoutingPresetPair{name, preset});
}

bool RoutingConfigurator::PresetByName(const std::string &name,
				       RoutingPreset &preset) const
{
	if (m_presets.find(name) != m_presets.end()) {
		preset = m_presets.at(name);
		return true;
	}
	return false;
}

void RoutingConfigurator::build_preset_table()
{
	static const RoutingPresetMap kRoutingPresets = {
		/*
        * HDMI RGB Capture
        */
		{"HDMI_HD_RGB_LFR_RGB_Capture",
		 {"HDMI_HD_RGB_LFR_RGB_Capture",
		  ConnectionKind::HDMI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  0,
		  true,
		  "hdmi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::HD_RGB_LFR,
		  VPIDStandard_Unknown}},
		/*
        * HDMI RGB Display
        */
		{"HDMI_HD_RGB_LFR_RGB_Display",
		 {"HDMI_HD_RGB_LFR_RGB_Display",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  0,
		  true,
		  "fb[{ch1}][0]->hdmi[0][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::HD_RGB_LFR,
		  VPIDStandard_Unknown}},
		{"HDMI_HD_RGB_LFR_RGB_Display (TTap Pro)",
		 {"HDMI_HD_RGB_LFR_RGB_Display (TTap Pro)",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  0,
		  true,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch1}][0]->hdmi[{ch1}][0];",
		  {DEVICE_ID_TTAP_PRO},
		  RasterDefinition::HD,
		  HDMIWireFormat::HD_RGB_LFR,
		  VPIDStandard_Unknown}},
		/*
        * HDMI YCbCr Capture
        */
		{"HDMI_HD_YCBCR_LFR_YCbCr_Capture",
		 {"HDMI_HD_YCBCR_LFR_YCbCr_Capture",
		  ConnectionKind::HDMI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  0,
		  false,
		  "hdmi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::HD_YCBCR_LFR,
		  VPIDStandard_Unknown}},
		/*
        * HDMI YCbCr Display
        */
		{"HDMI_HD_YCBCR_LFR_YCbCr_Display",
		 {"HDMI_HD_YCBCR_LFR_YCbCr_Display",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  0,
		  false,
		  "fb[{ch1}][0]->hdmi[0][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::HD_YCBCR_LFR,
		  VPIDStandard_Unknown}},
		{"HDMI_HD_YCBCR_LFR_YCbCr_Display (TTap Pro)",
		 {"HDMI_HD_YCBCR_LFR_YCbCr_Display (TTap Pro)",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  0,
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch1}][0]->hdmi[{ch1}][0];",
		  {DEVICE_ID_TTAP_PRO},
		  RasterDefinition::HD,
		  HDMIWireFormat::HD_YCBCR_LFR,
		  VPIDStandard_Unknown}},
		/*
        * SDI RGB Capture
        */
		// { "SD_ST352_RGB_Capture", {
		//     "SD_ST352_RGB_Capture",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_CAPTURE,
		//     1, 1,
		//     0,
		//     true,
		//     "", {},
		//     RasterDefinition::SD,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_483_576
		// } }, // NOTE(paulh): SD RGB not a valid capture config
		{"HD_720p_ST292_RGB_Capture",
		 {"HD_720p_ST292_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  0,
		  true,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720}},
		{"HD_1080_ST292_RGB_Capture",
		 {"HD_1080_ST292_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  0,
		  true,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080}},
		{"HD_1080_ST372_Dual_RGB_Capture",
		 {"HD_1080_ST372_Dual_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  2,
		  1,
		  0,
		  true,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch2}][0]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink}},
		{"HD_720p_ST425_3Ga_RGB_Capture",
		 {"HD_720p_ST425_3Ga_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  0,
		  true,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Ga}},
		{"HD_1080p_ST425_3Ga_RGB_Capture",
		 {"HD_1080p_ST425_3Ga_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  0,
		  true,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga}},
		{"HD_1080p_ST425_3Gb_DL_RGB_Capture",
		 {"HD_1080p_ST425_3Gb_DL_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  0,
		  true,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb}},
		{"HD_720p_ST425_3Gb_RGB_Capture",
		 {"HD_720p_ST425_3Gb_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  2,
		  0,
		  true,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Gb}},
		{"HD_1080p_ST425_3Gb_RGB_Capture",
		 {"HD_1080p_ST425_3Gb_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  2,
		  0,
		  true,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Gb}},
		{"HD_1080p_ST425_Dual_3Ga_RGB_Capture",
		 {"HD_1080p_ST425_Dual_3Ga_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  2,
		  2,
		  0,
		  true,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "sdi[{ch2}][0]->dli[{ch2}][0];"
		  "sdi[{ch2}][1]->dli[{ch2}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];"
		  "dli[{ch2}][0]->fb[{ch2}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Ga}},
		{"HD_1080p_ST425_Dual_3Gb_RGB_Capture",
		 {"HD_1080p_ST425_Dual_3Gb_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  2,
		  2,
		  kConvert3GIn,
		  true,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "sdi[{ch2}][0]->dli[{ch2}][0];"
		  "sdi[{ch2}][1]->dli[{ch2}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];"
		  "dli[{ch2}][0]->fb[{ch2}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Gb}},
		// { "UHD4K_ST292_Quad_1_5_Squares_RGB_Capture", {
		//     "UHD4K_ST292_Quad_1_5_Squares_RGB_Capture",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_CAPTURE,
		//     4, 4,
		//     0,
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_1080
		// } }, // NOTE(paulh): Not a valid RGB capture config
		// { "UHD4K_ST425_Quad_3Ga_Squares_RGB_Capture", {
		//     "UHD4K_ST425_Quad_3Ga_Squares_RGB_Capture",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_CAPTURE,
		//     4, 4,
		//     0,
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_1080_3Ga
		// } }, // NOTE(paulh): Not a valid RGB capture config
		{"UHD4K_ST425_Quad_3Gb_Squares_RGB_Capture",
		 {
			 "UHD4K_ST425_Quad_3Gb_Squares_RGB_Capture",
			 ConnectionKind::SDI,
			 NTV2_MODE_CAPTURE,
			 4,
			 4,
			 kEnable4KSquares,
			 true,
			 // SDIs -> Dual-Links
			 "sdi[{ch1}][0]->dli[{ch1}][0];"
			 "sdi[{ch1}][1]->dli[{ch1}][1];"
			 "sdi[{ch2}][0]->dli[{ch2}][0];"
			 "sdi[{ch2}][1]->dli[{ch2}][1];"
			 "sdi[{ch3}][0]->dli[{ch3}][0];"
			 "sdi[{ch3}][1]->dli[{ch3}][1];"
			 "sdi[{ch4}][0]->dli[{ch4}][0];"
			 "sdi[{ch4}][1]->dli[{ch4}][1];" // Dual-Links -> Framestores
			 "dli[{ch1}][0]->fb[{ch1}][2];"
			 "dli[{ch2}][0]->fb[{ch2}][2];"
			 "dli[{ch3}][0]->fb[{ch3}][2];"
			 "dli[{ch4}][0]->fb[{ch4}][2];",
			 {},
			 RasterDefinition::UHD_4K,
			 HDMIWireFormat::Unknown,
			 VPIDStandard_1080_DualLink_3Gb,
		 }},
		// { "UHD4K_ST425_Dual_3Gb_2SI_RGB_Capture", {
		//     "UHD4K_ST425_Dual_3Gb_2SI_RGB_Capture",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_CAPTURE,
		//     2, 4,
		//     (kEnable3GOut | kEnable3GbOut),
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_2160_DualLink
		// } }, // NOTE(paulh): Not a valid RGB capture config
		{"UHD4K_ST425_Quad_3Ga_2SI_RGB_Capture",
		 {"UHD4K_ST425_Quad_3Ga_2SI_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  4,
		  4,
		  (kEnable3GOut | kEnable4KTSI),
		  true,
		  // SDIs -> Dual-Links
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "sdi[{ch2}][0]->dli[{ch2}][0];"
		  "sdi[{ch2}][1]->dli[{ch2}][1];"
		  "sdi[{ch3}][0]->dli[{ch3}][0];"
		  "sdi[{ch3}][1]->dli[{ch3}][1];"
		  "sdi[{ch4}][0]->dli[{ch4}][0];"
		  "sdi[{ch4}][1]->dli[{ch4}][1];" // Dual-Links -> TSI Mux
		  "dli[{ch1}][0]->tsi[{ch1}][0];"
		  "dli[{ch2}][0]->tsi[{ch1}][1];"
		  "dli[{ch3}][0]->tsi[{ch2}][0];"
		  "dli[{ch4}][0]->tsi[{ch2}][1];" // TSI Mux -> Framestores
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadLink_3Ga}},
		{"UHD4K_ST425_Quad_3Gb_2SI_RGB_Capture",
		 {"UHD4K_ST425_Quad_3Gb_2SI_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  4,
		  4,
		  (kEnable3GOut | kEnable3GbOut | kEnable4KTSI),
		  true,
		  // SDIs -> Dual-Links
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "sdi[{ch2}][0]->dli[{ch2}][0];"
		  "sdi[{ch2}][1]->dli[{ch2}][1];"
		  "sdi[{ch3}][0]->dli[{ch3}][0];"
		  "sdi[{ch3}][1]->dli[{ch3}][1];"
		  "sdi[{ch4}][0]->dli[{ch4}][0];"
		  "sdi[{ch4}][1]->dli[{ch4}][1];" // Dual-Links -> TSI Mux
		  "dli[{ch1}][0]->tsi[{ch1}][0];"
		  "dli[{ch2}][0]->tsi[{ch1}][1];"
		  "dli[{ch3}][0]->tsi[{ch2}][0];"
		  "dli[{ch4}][0]->tsi[{ch2}][1];" // TSI Mux -> Framestores
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadDualLink_3Gb}},
		// { "UHD4K_ST2018_6G_Squares_2SI_RGB_Capture", {
		//     "UHD4K_ST2018_6G_Squares_2SI_RGB_Capture",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_CAPTURE,
		//     2, 2,
		//     kEnable6GOut,
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_2160_Single_6Gb
		// } }, // TODO(paulh)
		/*
        * SDI RGB Display
        */
		// { "SD_ST352_RGB_Display", {
		//     "SD_ST352_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     1, 1,
		//     0,
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::SD,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_483_576
		// } }, // NOTE(paulh): Not a valid RGB display config
		{"HD_720p_ST292_RGB_Display",
		 {"HD_720p_ST292_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  0,
		  true,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}[0];"
		  "dlo[{ch1}][1]->sdi[{ch2}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720}},
		{"HD_1080_ST292_RGB_Display",
		 {"HD_1080_ST292_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  0,
		  true,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}[0];"
		  "dlo[{ch1}][1]->sdi[{ch2}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080}},
		{"HD_1080_ST372_Dual_RGB_Display",
		 {"HD_1080_ST372_Dual_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  2,
		  1,
		  0,
		  true,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}[0];"
		  "dlo[{ch1}][1]->sdi[{ch2}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink}},
		{"HD_720p_ST425_3Ga_RGB_Display",
		 {"HD_720p_ST425_3Ga_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  (kEnable3GOut | kConvert3GaRGBOut),
		  true,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Ga}},
		{"HD_1080p_ST425_3Ga_RGB_Display",
		 {"HD_1080p_ST425_3Ga_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  (kEnable3GOut | kConvert3GaRGBOut),
		  true,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga}},
		{"HD_1080p_ST425_3Gb_DL_RGB_Display",
		 {"HD_1080p_ST425_3Gb_DL_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  (kEnable3GOut | kEnable3GbOut),
		  true,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb}},
		{"HD_720p_ST425_3Gb_RGB_Display",
		 {"HD_720p_ST425_3Gb_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  2,
		  (kEnable3GOut | kEnable3GbOut),
		  true,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Gb}},
		{"HD_1080p_ST425_3Gb_RGB_Display",
		 {"HD_1080p_ST425_3Gb_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  2,
		  (kEnable3GOut | kEnable3GbOut),
		  true,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Gb}},
		// { "HD_1080p_ST425_Dual_3Ga_RGB_Display", {
		//     "HD_1080p_ST425_Dual_3Ga_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 2,
		//     kEnable3GOut,
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::HD,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_1080_Dual_3Ga
		// } },
		// { "HD_1080p_ST425_Dual_3Gb_RGB_Display", {
		//     "HD_1080p_ST425_Dual_3Gb_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 2,
		//     (kEnable3GOut | kEnable3GbOut),
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::HD,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_1080_Dual_3Gb
		// } },
		// { "UHD4K_ST292_Quad_1_5_Squares_RGB_Display", {
		//     "UHD4K_ST292_Quad_1_5_Squares_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     4, 4,
		//     kEnable4KSquares,
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_1080
		// } },
		// { "UHD4K_ST425_Quad_3Ga_Squares_RGB_Display", {
		//     "UHD4K_ST425_Quad_3Ga_Squares_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     4, 4,
		//     (kEnable3GOut | kEnable4KSquares),
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_1080_3Ga
		// } },
		{"UHD4K_ST425_Quad_3Gb_Squares_RGB_Display",
		 {"UHD4K_ST425_Quad_3Gb_Squares_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  4,
		  4,
		  (kEnable3GOut | kEnable3GbOut | kEnable4KSquares),
		  true,
		  // Framestores -> Dual-Links
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "fb[{ch2}][2]->dlo[{ch2}][0];"
		  "fb[{ch3}][2]->dlo[{ch3}][0];"
		  "fb[{ch4}][2]->dlo[{ch4}][0];" // Dual-Links -> SDIs
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];"
		  "dlo[{ch2}][0]->sdi[{ch2}][0];"
		  "dlo[{ch2}][1]->sdi[{ch2}][1];"
		  "dlo[{ch3}][0]->sdi[{ch3}][0];"
		  "dlo[{ch3}][1]->sdi[{ch3}][1];"
		  "dlo[{ch4}][0]->sdi[{ch4}][0];"
		  "dlo[{ch4}][1]->sdi[{ch4}][1];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb}},
		// { "UHD4K_ST425_Dual_3Gb_2SI_RGB_Display", {
		//     "UHD4K_ST425_Dual_3Gb_2SI_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable3GOut | kEnable3GbOut | kEnable4KSquares),
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_2160_DualLink
		// } },
		{"UHD4K_ST425_Quad_3Ga_2SI_RGB_Display",
		 {"UHD4K_ST425_Quad_3Ga_2SI_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  4,
		  4,
		  (kEnable3GOut | kEnable4KTSI),
		  true,
		  "fb[{ch1}][2]->tsi[{ch1}][0];"
		  "fb[{ch1}][3]->tsi[{ch1}][1];"

		  "fb[{ch2}][2]->tsi[{ch2}][0];"
		  "fb[{ch2}][3]->tsi[{ch2}][1];"

		  "tsi[{ch1}][2]->dlo[{ch1}][0];"
		  "tsi[{ch1}][3]->dlo[{ch2}][0];"
		  "tsi[{ch2}][2]->dlo[{ch3}][0];"
		  "tsi[{ch2}][3]->dlo[{ch4}][0];"

		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];"
		  "dlo[{ch2}][0]->sdi[{ch2}][0];"
		  "dlo[{ch2}][1]->sdi[{ch2}][1];"
		  "dlo[{ch3}][0]->sdi[{ch3}][0];"
		  "dlo[{ch3}][1]->sdi[{ch3}][1];"
		  "dlo[{ch4}][0]->sdi[{ch4}][0];"
		  "dlo[{ch4}][1]->sdi[{ch4}][1];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadLink_3Ga}},
		{"UHD4K_ST425_Quad_3Gb_2SI_RGB_Display",
		 {"UHD4K_ST425_Quad_3Gb_2SI_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  4,
		  4,
		  (kEnable3GOut | kEnable4KTSI),
		  true,
		  "fb[{ch1}][2]->tsi[{ch1}][0];"
		  "fb[{ch1}][3]->tsi[{ch1}][1];"

		  "fb[{ch2}][2]->tsi[{ch2}][0];"
		  "fb[{ch2}][3]->tsi[{ch2}][1];"

		  "tsi[{ch1}][2]->dlo[{ch1}][0];"
		  "tsi[{ch1}][3]->dlo[{ch2}][0];"
		  "tsi[{ch2}][2]->dlo[{ch3}][0];"
		  "tsi[{ch2}][3]->dlo[{ch4}][0];"

		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];"
		  "dlo[{ch2}][0]->sdi[{ch2}][0];"
		  "dlo[{ch2}][1]->sdi[{ch2}][1];"
		  "dlo[{ch3}][0]->sdi[{ch3}][0];"
		  "dlo[{ch3}][1]->sdi[{ch3}][1];"
		  "dlo[{ch4}][0]->sdi[{ch4}][0];"
		  "dlo[{ch4}][1]->sdi[{ch4}][1];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadDualLink_3Gb}},
		// { "UHD4K_ST2018_6G_Squares_2SI_RGB_Display", {
		//     "UHD4K_ST2018_6G_Squares_2SI_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable6GOut | kEnable4KTSI),
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_2160_Single_6Gb
		// } }, // TODO(paulh)
		// { "UHD4K_ST2018_12G_Squares_2SI_RGB_Display", {
		//     "UHD4K_ST2018_12G_Squares_2SI_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable12GOut | kEnable4KTSI),
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_2160_Single_12Gb
		// } }, // TODO(paulh)
		// { "UHD28K_ST2082_Dual_12G_RGB_Display", {
		//     "UHD28K_ST2082_Dual_12G_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable12GOut | kEnable4KTSI),
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_4320_DualLink_12Gb
		// } }, // TODO(paulh)
		// { "UHD28K_ST2082_RGB_Dual_12G_RGB_Display", {
		//     "UHD28K_ST2082_RGB_Dual_12G_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable12GOut | kEnable4KTSI),
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_2160_DualLink_12Gb
		// } }, // TODO(paulh)
		// { "UHD28K_ST2082_Quad_12G_RGB_Display", {
		//     "UHD28K_ST2082_Quad_12G_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable12GOut | kEnable4KTSI),
		//     true,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_4320_QuadLink_12Gb
		// } }, // TODO(paulh)
		// ################################################################
		// SDI YCbCr Capture
		// ################################################################
		{"SD_ST352_YCbCr_Capture",
		 {"SD_ST352_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  0,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  RasterDefinition::SD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_483_576}},
		{"HD_720p_ST292_YCbCr_Capture",
		 {"HD_720p_ST292_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  0,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720}},
		{"HD_1080_ST292_YCbCr_Capture",
		 {"HD_1080_ST292_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  0,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080}},
		{"HD_1080_ST372_Dual_YCbCr_Capture",
		 {"HD_1080_ST372_Dual_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  2,
		  2,
		  0,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0]",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink}},
		{"HD_720p_ST425_3Ga_YCbCr_Capture",
		 {"HD_720p_ST425_3Ga_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  kEnable3GOut,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Ga}},
		{"HD_1080p_ST425_3Ga_YCbCr_Capture",
		 {"HD_1080p_ST425_3Ga_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  kEnable3GOut,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga}},
		{"HD_1080p_ST425_3Gb_DL_YCbCr_Capture",
		 {"HD_1080p_ST425_3Gb_DL_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  (kEnable3GOut | kConvert3GIn | kConvert3GOut),
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb}},
		{"HD_720p_ST425_3Gb_YCbCr_Capture",
		 {"HD_720p_ST425_3Gb_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  2,
		  kEnable3GOut,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch1}][1]->fb[{ch2}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Gb}},
		{"HD_1080p_ST425_3Gb_YCbCr_Capture",
		 {"HD_1080p_ST425_3Gb_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  2,
		  kEnable3GOut,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch1}][1]->fb[{ch2}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Gb}},
		{"HD_1080p_ST425_Dual_3Ga_YCbCr_Capture",
		 {"HD_1080p_ST425_Dual_3Ga_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  2,
		  2,
		  0,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Ga}},
		{"HD_1080p_ST425_Dual_3Gb_YCbCr_Capture",
		 {"HD_1080p_ST425_Dual_3Gb_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  2,
		  2,
		  0,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Gb}},
		{"UHD4K_ST292_Quad_1_5_Squares_YCbCr_Capture",
		 {"UHD4K_ST292_Quad_1_5_Squares_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  4,
		  4,
		  kEnable4KSquares,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0];"
		  "sdi[{ch3}][0]->fb[{ch3}][0];"
		  "sdi[{ch4}][0]->fb[{ch4}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080}},
		{"UHD4K_ST425_Quad_3Ga_Squares_YCbCr_Capture",
		 {"UHD4K_ST425_Quad_3Ga_Squares_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  4,
		  4,
		  kEnable4KSquares,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0];"
		  "sdi[{ch3}][0]->fb[{ch3}][0];"
		  "sdi[{ch4}][0]->fb[{ch4}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga}},
		{"UHD4K_ST425_Quad_3Gb_Squares_YCbCr_Capture",
		 {"UHD4K_ST425_Quad_3Gb_Squares_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  4,
		  4,
		  kEnable4KSquares | kConvert3GIn,
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0];"
		  "sdi[{ch3}][0]->fb[{ch3}][0];"
		  "sdi[{ch4}][0]->fb[{ch4}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb}},
		{"UHD4K_ST425_Dual_3Gb_2SI_YCbCr_Capture",
		 {"UHD4K_ST425_Dual_3Gb_2SI_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  2,
		  2,
		  kConvert3GIn | kEnable4KTSI,
		  false,
		  "sdi[{ch1}][0]->tsi[{ch1}][0];"
		  "sdi[{ch1}][1]->tsi[{ch1}][1];"
		  "sdi[{ch2}][0]->tsi[{ch2}][0];"
		  "sdi[{ch2}][1]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_DualLink}},
		{"UHD4K_ST425_Quad_3Ga_2SI_YCbCr_Capture",
		 {"UHD4K_ST425_Quad_3Ga_2SI_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  4,
		  4,
		  kEnable4KTSI,
		  false,
		  "sdi[{ch1}][0]->tsi[{ch1}][0];"
		  "sdi[{ch2}][0]->tsi[{ch1}][1];"
		  "sdi[{ch3}][0]->tsi[{ch2}][0];"
		  "sdi[{ch4}][0]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadLink_3Ga}},
		{"UHD4K_ST425_Quad_3Gb_2SI_YCbCr_Capture",
		 {"UHD4K_ST425_Quad_3Gb_2SI_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  4,
		  4,
		  (kConvert3GIn | kEnable4KTSI),
		  false,
		  "sdi[{ch1}][0]->tsi[{ch1}][0];"
		  "sdi[{ch2}][0]->tsi[{ch1}][1];"
		  "sdi[{ch3}][0]->tsi[{ch2}][0];"
		  "sdi[{ch4}][0]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadDualLink_3Gb}},
		{"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Capture",
		 {"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  (kEnable6GOut | kEnable4KTSI),
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_6Gb}},
		{"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Capture (Kona5/io4K+)",
		 {"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Capture (Kona5/io4K+)",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  4,
		  4,
		  (kEnable6GOut | kEnable4KTSI),
		  false,
		  "sdi[{ch1}][0]->tsi[{ch1}][0];"
		  "sdi[{ch2}][0]->tsi[{ch1}][1];"
		  "sdi[{ch3}][0]->tsi[{ch2}][0];"
		  "sdi[{ch4}][0]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {DEVICE_ID_KONA5, DEVICE_ID_IO4KPLUS},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_6Gb}},
		{"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Capture",
		 {"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  1,
		  1,
		  (kEnable12GOut | kConvert3GaRGBOut | kEnable4KTSI),
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_12Gb}},
		{"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Capture (Kona5/io4K+)",
		 {"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Capture (Kona5/io4K+)",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  4,
		  4,
		  (kEnable6GOut | kEnable4KTSI),
		  false,
		  "sdi[{ch1}][0]->tsi[{ch1}][0];"
		  "sdi[{ch2}][0]->tsi[{ch1}][1];"
		  "sdi[{ch3}][0]->tsi[{ch2}][0];"
		  "sdi[{ch4}][0]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {DEVICE_ID_KONA5, DEVICE_ID_IO4KPLUS},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_12Gb}},
		// TODO
		{"UHD28K_ST2082_Dual_12G_YCbCr_Capture",
		 {"UHD28K_ST2082_Dual_12G_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  2,
		  2,
		  (kEnable12GOut | kConvert3GaRGBOut | kEnable4KTSI),
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_4320_DualLink_12Gb}},
		// TODO
		{"UHD28K_ST2082_Quad_12G_YCbCr_Capture",
		 {"UHD28K_ST2082_Quad_12G_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  4,
		  4,
		  (kEnable12GOut | kConvert3GaRGBOut | kEnable4KTSI),
		  false,
		  "sdi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_4320_QuadLink_12Gb}},
		/*
        * SDI YCbCr Display
        */
		{"SD_ST352_YCbCr_Display",
		 {"SD_ST352_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  0,
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  RasterDefinition::SD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_483_576}},
		{"HD_720p_ST292_YCbCr_Display",
		 {"HD_720p_ST292_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  0,
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720}},
		{"HD_1080_ST292_YCbCr_Display",
		 {"HD_1080_ST292_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  0,
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080}},
		{"HD_1080_ST372_Dual_YCbCr_Display",
		 {"HD_1080_ST372_Dual_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  2,
		  2,
		  0,
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch2}][0]",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink}},
		{"HD_720p_ST425_3Ga_YCbCr_Display",
		 {"HD_720p_ST425_3Ga_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  kEnable3GOut,
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Ga}},
		{"HD_1080p_ST425_3Ga_YCbCr_Display",
		 {"HD_1080p_ST425_3Ga_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  kEnable3GOut,
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga}},
		{"HD_1080p_ST425_3Gb_DL_YCbCr_Display",
		 {"HD_1080p_ST425_3Gb_DL_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  (kEnable3GOut | kConvert3GIn | kConvert3GOut),
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb}},
		{"HD_720p_ST425_3Gb_YCbCr_Display",
		 {"HD_720p_ST425_3Gb_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  2,
		  kEnable3GOut,
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch1}][1];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Gb}},
		{"HD_1080p_ST425_3Gb_YCbCr_Display",
		 {"HD_1080p_ST425_3Gb_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  2,
		  kEnable3GOut,
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch1}][1];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Gb}},
		{"HD_1080p_ST425_Dual_3Ga_YCbCr_Display",
		 {"HD_1080p_ST425_Dual_3Ga_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  2,
		  2,
		  kEnable3GOut,
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch1}][1];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Ga}},
		{"HD_1080p_ST425_Dual_3Gb_YCbCr_Display",
		 {"HD_1080p_ST425_Dual_3Gb_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  2,
		  2,
		  kEnable3GOut,
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch1}][1];",
		  {},
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Gb}},
		{"UHD4K_ST292_Quad_1_5_Squares_YCbCr_Display",
		 {"UHD4K_ST292_Quad_1_5_Squares_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  4,
		  4,
		  (kEnable3GbOut | kEnable4KSquares),
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch2}][0];"
		  "fb[{ch3}][0]->sdi[{ch3}][0];"
		  "fb[{ch4}][0]->sdi[{ch4}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080}},
		{"UHD4K_ST425_Quad_3Ga_Squares_YCbCr_Display",
		 {"UHD4K_ST425_Quad_3Ga_Squares_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  4,
		  4,
		  (kEnable3GOut | kEnable3GbOut),
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch2}][0];"
		  "fb[{ch3}][0]->sdi[{ch3}][0];"
		  "fb[{ch4}][0]->sdi[{ch4}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga}},
		{"UHD4K_ST425_Quad_3Gb_Squares_YCbCr_Display",
		 {"UHD4K_ST425_Quad_3Gb_Squares_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  4,
		  4,
		  (kEnable3GOut | kEnable3GbOut | kEnable4KSquares),
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch2}][0];"
		  "fb[{ch3}][0]->sdi[{ch3}][0];"
		  "fb[{ch4}][0]->sdi[{ch4}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb}},
		{"UHD4K_ST425_Dual_3Gb_2SI_YCbCr_Display",
		 {"UHD4K_ST425_Dual_3Gb_2SI_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  2,
		  2,
		  (kEnable3GOut | kEnable4KTSI),
		  false,
		  "fb[{ch1}][0]->tsi[{ch1}][0];"
		  "fb[{ch1}][1]->tsi[{ch1}][1];"
		  "fb[{ch2}][0]->tsi[{ch2}][0];"
		  "fb[{ch2}][1]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->sdi[{ch1}][0];"
		  "tsi[{ch1}][1]->sdi[{ch1}][1];"
		  "tsi[{ch2}][0]->sdi[{ch2}][0];"
		  "tsi[{ch2}][1]->sdi[{ch2}][1];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_DualLink}},
		{"UHD4K_ST425_Quad_3Ga_2SI_YCbCr_Display",
		 {"UHD4K_ST425_Quad_3Ga_2SI_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  4,
		  4,
		  (kEnable3GOut | kEnable4KTSI),
		  false,
		  "fb[{ch1}][0]->tsi[{ch1}][0];"
		  "fb[{ch1}][1]->tsi[{ch1}][1];"
		  "fb[{ch2}][0]->tsi[{ch2}][0];"
		  "fb[{ch2}][1]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->sdi[{ch1}][0];"
		  "tsi[{ch1}][1]->sdi[{ch2}][0];"
		  "tsi[{ch2}][0]->sdi[{ch3}][0];"
		  "tsi[{ch2}][1]->sdi[{ch4}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadLink_3Ga}},
		{"UHD4K_ST425_Quad_3Gb_2SI_YCbCr_Display",
		 {"UHD4K_ST425_Quad_3Gb_2SI_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  4,
		  4,
		  (kEnable3GOut | kConvert3GaRGBOut | kEnable4KTSI),
		  false,
		  "fb[{ch1}][0]->tsi[{ch1}][0];"
		  "fb[{ch1}][1]->tsi[{ch1}][1];"
		  "fb[{ch2}][0]->tsi[{ch2}][0];"
		  "fb[{ch2}][1]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->sdi[{ch1}][0];"
		  "tsi[{ch1}][1]->sdi[{ch2}][0];"
		  "tsi[{ch2}][0]->sdi[{ch3}][0];"
		  "tsi[{ch2}][1]->sdi[{ch4}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadDualLink_3Gb}},
		{"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Display",
		 {"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  (kEnable6GOut | kEnable4KTSI),
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_6Gb}},
		{"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Display (Kona5/io4K+)",
		 {"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Display (Kona5/io4K+)",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  4,
		  4,
		  (kEnable6GOut | kEnable4KTSI),
		  false,
		  "fb[{ch3}][0]->tsi[{ch3}][0];"
		  "fb[{ch3}][1]->tsi[{ch3}][1];"
		  "fb[{ch4}][0]->tsi[{ch4}][0];"
		  "fb[{ch4}][1]->tsi[{ch4}][1];"
		  "tsi[{ch3}][0]->sdi[{ch1}][0];"
		  "tsi[{ch3}][1]->sdi[{ch2}][0];"
		  "tsi[{ch4}][0]->sdi[{ch3}][0];"
		  "tsi[{ch4}][1]->sdi[{ch4}][0];",
		  {DEVICE_ID_KONA5, DEVICE_ID_IO4KPLUS},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_6Gb}},
		{"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Display",
		 {"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  1,
		  1,
		  (kEnable12GOut | kEnable4KTSI),
		  false,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_12Gb}},
		{"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Display (Kona5/io4K+)",
		 {"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Display (Kona5/io4K+)",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  4,
		  4,
		  (kEnable12GOut | kConvert3GOut | kEnable4KTSI),
		  false,
		  "fb[{ch3}][0]->tsi[{ch3}][0];"
		  "fb[{ch3}][1]->tsi[{ch3}][1];"
		  "fb[{ch4}][0]->tsi[{ch4}][0];"
		  "fb[{ch4}][1]->tsi[{ch4}][1];"
		  "tsi[{ch3}][0]->sdi[{ch1}][0];"
		  "tsi[{ch3}][1]->sdi[{ch2}][0];"
		  "tsi[{ch4}][0]->sdi[{ch3}][0];"
		  "tsi[{ch4}][1]->sdi[{ch4}][0];",
		  {DEVICE_ID_KONA5, DEVICE_ID_IO4KPLUS},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_12Gb}},
		// TODO
		{"UHD28K_ST2082_Dual_12G_YCbCr_Display",
		 {"UHD28K_ST2082_Dual_12G_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  2,
		  2,
		  (kEnable12GOut | kConvert3GOut | kEnable4KTSI),
		  false,
		  "",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_4320_DualLink_12Gb}},
		// TODO
		{"UHD28K_ST2082_Quad_12G_YCbCr_Display",
		 {"UHD28K_ST2082_Quad_12G_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  4,
		  4,
		  (kEnable12GOut | kConvert3GOut | kEnable4KTSI),
		  false,
		  "",
		  {},
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_4320_QuadLink_12Gb}},
	};
	for (auto &&rp : kRoutingPresets)
		AddPreset(std::move(rp.first), std::move(rp.second));
}

RoutingPresetMap RoutingConfigurator::GetPresetTable() const
{
	return m_presets;
}

bool RoutingConfigurator::FindFirstPreset(ConnectionKind kind, NTV2DeviceID id,
					  NTV2Mode mode, NTV2VideoFormat vf,
					  NTV2PixelFormat pf,
					  VPIDStandard standard,
					  RoutingPreset &preset)
{
	// if (NTV2DeviceCanDoVideoFormat(id, vf) &&
	//     NTV2DeviceCanDoFrameBufferFormat(id, pf)) {
	{
		const auto &rd = DetermineRasterDefinition(vf);
		bool is_rgb = NTV2_IS_FBF_RGB(pf);
		std::vector<RoutingPresetPair> query;
		for (const auto &p : m_presets) {
			if (p.second.kind == kind && p.second.mode == mode &&
			    p.second.raster_def == rd &&
			    p.second.is_rgb == is_rgb &&
			    p.second.vpid_standard == standard) {
				query.push_back(p);
			}
		}
		RoutingPresets device_presets;
		for (const auto &q : query) {
			for (const auto &device_id : q.second.device_ids) {
				if (device_id == id)
					device_presets.push_back(q.second);
			}
		}
		if (device_presets.size() > 0) {
			preset = device_presets.at(0);
			return true;
		}
		if (query.size() > 0) {
			preset = query.at(0).second;
			return true;
		}
	}

	return false;
}

} // aja
