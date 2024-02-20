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
		  RasterDefinition::HD,
		  HDMIWireFormat::SD_HD_RGB,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  0,
		  "hdmi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"HDMI_HD_RGB_HFR_RGB_Capture",
		 {"HDMI_HD_RGB_HFR_RGB_Capture",
		  ConnectionKind::HDMI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::SD_HD_RGB,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  0,
		  "hdmi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"HDMI_UHD_4K_RGB_Capture (io4K+)",
		 {"HDMI_UHD_4K_RGB_Capture (io4K+)",
		  ConnectionKind::HDMI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::UHD_4K_RGB,
		  VPIDStandard_Unknown,
		  1,
		  2,
		  kEnable4KTSI,
		  "hdmi[{ch1}][4]->tsi[{ch1}][0];"
		  "hdmi[{ch1}][5]->tsi[{ch1}][1];"
		  "hdmi[{ch1}][6]->tsi[{ch2}][0];"
		  "hdmi[{ch1}][7]->tsi[{ch2}][1];"
		  "tsi[{ch1}][2]->fb[{ch1}][0];"
		  "tsi[{ch1}][3]->fb[{ch1}][1];"
		  "tsi[{ch2}][2]->fb[{ch2}][0];"
		  "tsi[{ch2}][3]->fb[{ch2}][1];",
		  {DEVICE_ID_IO4KPLUS},
		  true,
		  false}},
		/*
		 * HDMI RGB Display
		 */
		{"HDMI_HD_RGB_LFR_RGB_Display",
		 {"HDMI_HD_RGB_LFR_RGB_Display",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::SD_HD_RGB,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  0,
		  "fb[{ch1}][0]->hdmi[0][0];",
		  {},
		  true,
		  false}},
		{"HDMI_HD_RGB_HFR_RGB_Display",
		 {"HDMI_HD_RGB_HFR_RGB_Display",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::SD_HD_RGB,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  0,
		  "fb[{ch1}][0]->hdmi[0][0];",
		  {},
		  true,
		  false}},
		{"HDMI_RGB_LFR_RGB_Display (TTap Pro)",
		 {"HDMI_RGB_LFR_RGB_Display (TTap Pro)",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::SD_HD_RGB,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  kEnable3GOut,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];"
		  "fb[{ch1}][2]->hdmi[{ch1}][0];",
		  {DEVICE_ID_TTAP_PRO},
		  true,
		  false}},
		{"HDMI_RGB_HFR_RGB_Display (TTap Pro)",
		 {"HDMI_RGB_HFR_RGB_Display (TTap Pro)",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::SD_HD_RGB,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  kConvert3GaRGBOut,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];"
		  "fb[{ch1}][2]->hdmi[{ch1}][0];",
		  {DEVICE_ID_TTAP_PRO},
		  true,
		  false}},
		/*
		 * HDMI YCbCr Capture
		 */
		{"HDMI_HD_LFR_YCbCr_Capture",
		 {"HDMI_HD_LFR_YCbCr_Capture",
		  ConnectionKind::HDMI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::SD_HD_YCBCR,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  0,
		  "hdmi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  false,
		  false}},
		{"HDMI_UHD_4K_YCbCr_Capture (io4K+)",
		 {"HDMI_UHD_4K_YCbCr_Capture (io4K+)",
		  ConnectionKind::HDMI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::UHD_4K_YCBCR,
		  VPIDStandard_Unknown,
		  1,
		  2,
		  kEnable4KTSI,
		  "hdmi[{ch1}][0]->tsi[{ch1}][0];"
		  "hdmi[{ch1}][1]->tsi[{ch1}][1];"
		  "hdmi[{ch1}][2]->tsi[{ch2}][0];"
		  "hdmi[{ch1}][3]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {DEVICE_ID_IO4KPLUS},
		  false,
		  false}},
		{"HDMI_UHD_4K_YCbCr_Capture",
		 {"HDMI_UHD_4K_YCbCr_Capture",
		  ConnectionKind::HDMI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::UHD_4K_YCBCR,
		  VPIDStandard_Unknown,
		  1,
		  2,
		  kEnable4KTSI,
		  "hdmi[{ch1}][0]->tsi[{ch1}][0];"
		  "hdmi[{ch1}][1]->tsi[{ch1}][1];"
		  "hdmi[{ch1}][2]->tsi[{ch2}][0];"
		  "hdmi[{ch1}][3]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {},
		  false,
		  false}},
		/*
		 * HDMI YCbCr Display
		 */
		{"HDMI_HD_LFR_YCbCr_Display",
		 {"HDMI_HD_LFR_YCbCr_Display",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::SD_HD_YCBCR,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  0,
		  "fb[{ch1}][0]->hdmi[0][0];",
		  {},
		  false,
		  false}},
		{"HDMI_UHD_4K_LFR_YCbCr_Display",
		 {"HDMI_UHD_4K_LFR_YCbCr_Display",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::UHD_4K_YCBCR,
		  VPIDStandard_Unknown,
		  1,
		  2,
		  kEnable4KTSI,
		  "fb[{ch1}][0]->tsi[{ch1}][0];"
		  "fb[{ch1}][1]->tsi[{ch1}][1];"
		  "fb[{ch2}][0]->tsi[{ch2}][0];"
		  "fb[{ch2}][1]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->hdmi[{ch1}][0];"
		  "tsi[{ch1}][1]->hdmi[{ch1}][1];"
		  "tsi[{ch2}][0]->hdmi[{ch1}][2];"
		  "tsi[{ch2}][1]->hdmi[{ch1}][3];",
		  {},
		  false,
		  false}},
		{"HDMI_UHD_4K_LFR_YCbCr_Display_Kona5_8K",
		 {"HDMI_UHD_4K_LFR_YCbCr_Display_Kona5_8K",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::UHD_4K_YCBCR,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  kEnable4KTSI,
		  "fb[{ch1}][0]->hdmi[{ch1}][0];",
		  {DEVICE_ID_KONA5_8K},
		  false,
		  false}},
		{"HDMI_HD_LFR_YCbCr_Display (TTap Pro)",
		 {"HDMI_HD_LFR_YCbCr_Display (TTap Pro)",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::SD_HD_YCBCR,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  0,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch1}][0]->hdmi[{ch1}][0];",
		  {DEVICE_ID_TTAP_PRO},
		  false,
		  false}},
		{"HDMI_HD_HFR_YCbCr_Display (TTap Pro)",
		 {"HDMI_HD_HFR_YCbCr_Display (TTap Pro)",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::SD_HD_YCBCR,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  kEnable3GOut,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch1}][0]->hdmi[{ch1}][0];",
		  {DEVICE_ID_TTAP_PRO},
		  false,
		  false}},
		{"HDMI_UHD_4K_LFR_YCbCr_Display (TTap Pro)",
		 {"HDMI_UHD_4K_LFR_YCbCr_Display (TTap Pro)",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::UHD_4K_YCBCR,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  kEnable4KTSI | kEnable6GOut,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch1}][0]->hdmi[{ch1}][0];",
		  {DEVICE_ID_TTAP_PRO},
		  false,
		  false}},
		{"HDMI_UHD_4K_HFR_YCbCr_Display (TTap Pro)",
		 {"HDMI_UHD_4K_HFR_YCbCr_Display (TTap Pro)",
		  ConnectionKind::HDMI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::UHD_4K_YCBCR,
		  VPIDStandard_Unknown,
		  1,
		  1,
		  kEnable4KTSI | kEnable12GOut,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch1}][0]->hdmi[{ch1}][0];",
		  {DEVICE_ID_TTAP_PRO},
		  false,
		  false}},
		/*
		 * SDI RGB Capture
		 */
		{"HD_720p_ST292_RGB_Capture",
		 {"HD_720p_ST292_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720,
		  1,
		  1,
		  0,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"HD_1080_ST292_RGB_Capture",
		 {"HD_1080_ST292_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080,
		  1,
		  1,
		  0,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"HD_1080_ST372_Dual_RGB_Capture",
		 {"HD_1080_ST372_Dual_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink,
		  2,
		  1,
		  0,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch2}][0]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"HD_720p_ST425_3Ga_RGB_Capture",
		 {"HD_720p_ST425_3Ga_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Ga,
		  1,
		  1,
		  0,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"HD_1080p_ST425_3Ga_RGB_Capture",
		 {"HD_1080p_ST425_3Ga_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga,
		  1,
		  1,
		  0,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"HD_1080p_ST425_3Gb_DL_RGB_Capture",
		 {"HD_1080p_ST425_3Gb_DL_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb,
		  1,
		  1,
		  0,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"HD_720p_ST425_3Gb_RGB_Capture",
		 {"HD_720p_ST425_3Gb_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Gb,
		  1,
		  2,
		  0,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"HD_1080p_ST425_3Gb_RGB_Capture",
		 {"HD_1080p_ST425_3Gb_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Gb,
		  1,
		  2,
		  0,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"HD_1080p_ST425_Dual_3Ga_RGB_Capture",
		 {"HD_1080p_ST425_Dual_3Ga_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Ga,
		  2,
		  2,
		  0,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "sdi[{ch2}][0]->dli[{ch2}][0];"
		  "sdi[{ch2}][1]->dli[{ch2}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];"
		  "dli[{ch2}][0]->fb[{ch2}][0];",
		  {},
		  true,
		  false}},
		{"HD_1080p_ST425_Dual_3Gb_RGB_Capture",
		 {"HD_1080p_ST425_Dual_3Gb_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Gb,
		  2,
		  2,
		  kConvert3GIn,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "sdi[{ch2}][0]->dli[{ch2}][0];"
		  "sdi[{ch2}][1]->dli[{ch2}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];"
		  "dli[{ch2}][0]->fb[{ch2}][0];",
		  {},
		  true,
		  false}},
		{"UHD4K_ST425_Quad_3Ga_Squares_RGB_Capture",
		 {"UHD4K_ST425_Quad_3Ga_Squares_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga,
		  4,
		  4,
		  kEnable4KSquares,
		  // SDIs -> Dual-Links
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "sdi[{ch2}][0]->dli[{ch2}][0];"
		  "sdi[{ch2}][1]->dli[{ch2}][1];"
		  "sdi[{ch3}][0]->dli[{ch3}][0];"
		  "sdi[{ch3}][1]->dli[{ch3}][1];"
		  "sdi[{ch4}][0]->dli[{ch4}][0];"
		  "sdi[{ch4}][1]->dli[{ch4}][1];" // Dual-Links -> Framestores
		  "dli[{ch1}][0]->fb[{ch1}][0];"
		  "dli[{ch2}][0]->fb[{ch2}][0];"
		  "dli[{ch3}][0]->fb[{ch3}][0];"
		  "dli[{ch4}][0]->fb[{ch4}][0];",
		  {},
		  true,
		  false}},
		{"UHD4K_ST425_Quad_3Gb_Squares_RGB_Capture",
		 {"UHD4K_ST425_Quad_3Gb_Squares_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb,
		  4,
		  4,
		  kEnable4KSquares,
		  // SDIs -> Dual-Links
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "sdi[{ch2}][0]->dli[{ch2}][0];"
		  "sdi[{ch2}][1]->dli[{ch2}][1];"
		  "sdi[{ch3}][0]->dli[{ch3}][0];"
		  "sdi[{ch3}][1]->dli[{ch3}][1];"
		  "sdi[{ch4}][0]->dli[{ch4}][0];"
		  "sdi[{ch4}][1]->dli[{ch4}][1];" // Dual-Links -> Framestores
		  "dli[{ch1}][0]->fb[{ch1}][0];"
		  "dli[{ch2}][0]->fb[{ch2}][0];"
		  "dli[{ch3}][0]->fb[{ch3}][0];"
		  "dli[{ch4}][0]->fb[{ch4}][0];",
		  {},
		  true,
		  false}},
		{"UHD4K_ST425_Quad_3Ga_2SI_RGB_Capture",
		 {"UHD4K_ST425_Quad_3Ga_2SI_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadLink_3Ga,
		  4,
		  4,
		  kEnable4KTSI,
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
		  true,
		  false}},
		{"UHD4K_ST425_Quad_3Gb_2SI_RGB_Capture",
		 {"UHD4K_ST425_Quad_3Gb_2SI_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadDualLink_3Gb,
		  4,
		  4,
		  (kEnable3GOut | kEnable3GbOut | kEnable4KTSI),
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
		  true,
		  false}},

		/////////////////////////////////
		{"UHD4K_ST2018_6G_Squares_2SI_RGB_Capture",
		 {"UHD4K_ST2018_6G_Squares_2SI_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_6Gb,
		  1,
		  1,
		  kEnable4KTSI,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"UHD4K_ST2018_6G_Squares_2SI_RGB_Capture (Kona5/io4K+)",
		 {"UHD4K_ST2018_6G_Squares_2SI_RGB_Capture (Kona5/io4K+)",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_6Gb,
		  4,
		  4,
		  kEnable4KTSI,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "sdi[{ch2}][0]->dli[{ch2}][0];"
		  "sdi[{ch2}][1]->dli[{ch2}][1];"
		  "sdi[{ch3}][0]->dli[{ch3}][0];"
		  "sdi[{ch3}][1]->dli[{ch3}][1];"
		  "sdi[{ch4}][0]->dli[{ch4}][0];"
		  "sdi[{ch4}][1]->dli[{ch4}][1];"
		  "dli[{ch1}][0]->tsi[{ch1}][0];"
		  "dli[{ch2}][0]->tsi[{ch1}][1];"
		  "dli[{ch3}][0]->tsi[{ch2}][0];"
		  "dli[{ch4}][0]->tsi[{ch2}][1];"
		  "tsi[{ch1}][2]->fb[{ch1}][0];"
		  "tsi[{ch1}][3]->fb[{ch1}][1];"
		  "tsi[{ch2}][2]->fb[{ch2}][0];"
		  "tsi[{ch2}][3]->fb[{ch2}][1];",
		  {DEVICE_ID_KONA5, DEVICE_ID_IO4KPLUS},
		  true,
		  true}},
		{"UHD4K_ST2018_12G_Squares_2SI_RGB_Capture",
		 {"UHD4K_ST2018_12G_Squares_2SI_RGB_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_12Gb,
		  1,
		  1,
		  kEnable4KTSI,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "dli[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  true,
		  false}},
		{"UHD4K_ST2018_12G_Squares_2SI_RGB_Capture (Kona5/io4K+)",
		 {"UHD4K_ST2018_12G_Squares_2SI_RGB_Capture (Kona5/io4K+)",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_12Gb,
		  4,
		  4,
		  kEnable4KTSI,
		  "sdi[{ch1}][0]->dli[{ch1}][0];"
		  "sdi[{ch1}][1]->dli[{ch1}][1];"
		  "sdi[{ch2}][0]->dli[{ch2}][0];"
		  "sdi[{ch2}][1]->dli[{ch2}][1];"
		  "sdi[{ch3}][0]->dli[{ch3}][0];"
		  "sdi[{ch3}][1]->dli[{ch3}][1];"
		  "sdi[{ch4}][0]->dli[{ch4}][0];"
		  "sdi[{ch4}][1]->dli[{ch4}][1];"
		  "dli[{ch1}][0]->tsi[{ch1}][0];"
		  "dli[{ch2}][0]->tsi[{ch1}][1];"
		  "dli[{ch3}][0]->tsi[{ch2}][0];"
		  "dli[{ch4}][0]->tsi[{ch2}][1];"
		  "tsi[{ch1}][2]->fb[{ch1}][0];"
		  "tsi[{ch1}][3]->fb[{ch1}][1];"
		  "tsi[{ch2}][2]->fb[{ch2}][0];"
		  "tsi[{ch2}][3]->fb[{ch2}][1];",
		  {DEVICE_ID_KONA5, DEVICE_ID_IO4KPLUS},
		  true,
		  true}},
		/////////////////////////////////
		/*
		 * SDI RGB Display
		 */
		{"HD_720p_ST292_RGB_Display",
		 {"HD_720p_ST292_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720,
		  1,
		  1,
		  0,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}[0];"
		  "dlo[{ch1}][1]->sdi[{ch2}][0];",
		  {},
		  true,
		  false}},
		{"HD_1080_ST292_RGB_Display",
		 {"HD_1080_ST292_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080,
		  1,
		  1,
		  0,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}[0];"
		  "dlo[{ch1}][1]->sdi[{ch2}][0];",
		  {},
		  true,
		  false}},
		{"HD_1080_ST372_Dual_RGB_Display",
		 {"HD_1080_ST372_Dual_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink,
		  2,
		  1,
		  0,
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}[0];"
		  "dlo[{ch1}][1]->sdi[{ch2}][0];",
		  {},
		  true,
		  false}},
		{"HD_720p_ST425_3Ga_RGB_Display",
		 {"HD_720p_ST425_3Ga_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Ga,
		  1,
		  1,
		  (kEnable3GOut | kConvert3GaRGBOut),
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];",
		  {},
		  true,
		  false}},
		{"HD_1080p_ST425_3Ga_RGB_Display",
		 {"HD_1080p_ST425_3Ga_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga,
		  1,
		  1,
		  (kEnable3GOut | kConvert3GaRGBOut),
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];",
		  {},
		  true,
		  false}},
		{"HD_1080p_ST425_3Gb_DL_RGB_Display",
		 {"HD_1080p_ST425_3Gb_DL_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb,
		  1,
		  1,
		  (kEnable3GOut | kEnable3GbOut),
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];",
		  {},
		  true,
		  false}},
		{"HD_720p_ST425_3Gb_RGB_Display",
		 {"HD_720p_ST425_3Gb_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Gb,
		  1,
		  2,
		  (kEnable3GOut | kEnable3GbOut),
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];",
		  {},
		  true,
		  false}},
		{"HD_1080p_ST425_3Gb_RGB_Display",
		 {"HD_1080p_ST425_3Gb_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Gb,
		  1,
		  2,
		  (kEnable3GOut | kEnable3GbOut),
		  "fb[{ch1}][2]->dlo[{ch1}][0];"
		  "dlo[{ch1}][0]->sdi[{ch1}][0];"
		  "dlo[{ch1}][1]->sdi[{ch1}][1];",
		  {},
		  true,
		  false}},
		// { "HD_1080p_ST425_Dual_3Ga_RGB_Display", {
		//     "HD_1080p_ST425_Dual_3Ga_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 2,
		//     kEnable3GOut,
		//     "",
		//     {},
		//     RasterDefinition::HD,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_1080_Dual_3Ga, true, false}},
		// { "HD_1080p_ST425_Dual_3Gb_RGB_Display", {
		//     "HD_1080p_ST425_Dual_3Gb_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 2,
		//     (kEnable3GOut | kEnable3GbOut),
		//     "",
		//     {},
		//     RasterDefinition::HD,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_1080_Dual_3Gb, true, false}},
		// { "UHD4K_ST292_Quad_1_5_Squares_RGB_Display", {
		//     "UHD4K_ST292_Quad_1_5_Squares_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     4, 4,
		//     kEnable4KSquares,
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_1080, true, false}},
		// { "UHD4K_ST425_Quad_3Ga_Squares_RGB_Display", {
		//     "UHD4K_ST425_Quad_3Ga_Squares_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     4, 4,
		//     (kEnable3GOut | kEnable4KSquares),
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_1080_3Ga, true, false}},
		{"UHD4K_ST425_Quad_3Gb_Squares_RGB_Display",
		 {"UHD4K_ST425_Quad_3Gb_Squares_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb,
		  4,
		  4,
		  (kEnable3GOut | kEnable3GbOut | kEnable4KSquares),
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
		  true,
		  false}},
		// { "UHD4K_ST425_Dual_3Gb_2SI_RGB_Display", {
		//     "UHD4K_ST425_Dual_3Gb_2SI_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable3GOut | kEnable3GbOut | kEnable4KSquares),
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_2160_DualLink, true, false}},
		{"UHD4K_ST425_Quad_3Ga_2SI_RGB_Display",
		 {"UHD4K_ST425_Quad_3Ga_2SI_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadLink_3Ga,
		  4,
		  4,
		  (kEnable3GOut | kEnable4KTSI),
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
		  true,
		  false}},
		{"UHD4K_ST425_Quad_3Gb_2SI_RGB_Display",
		 {"UHD4K_ST425_Quad_3Gb_2SI_RGB_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadDualLink_3Gb,
		  4,
		  4,
		  (kEnable3GOut | kEnable4KTSI),
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
		  true,
		  false}},
		// TODO(paulh): Find out proper settings for this route
		// { "UHD4K_ST2018_6G_Squares_2SI_RGB_Display", {
		//     "UHD4K_ST2018_6G_Squares_2SI_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable6GOut | kEnable4KTSI),
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_2160_Single_6Gb, true, false}},
		// TODO(paulh): Find out proper settings for this route
		// { "UHD4K_ST2018_12G_Squares_2SI_RGB_Display", {
		//     "UHD4K_ST2018_12G_Squares_2SI_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable12GOut | kEnable4KTSI),
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_2160_Single_12Gb, true, false}},
		// { "UHD28K_ST2082_Dual_12G_RGB_Display", {
		//     "UHD28K_ST2082_Dual_12G_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable12GOut | kEnable4KTSI),
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_4320_DualLink_12Gb, true, false}},
		// { "UHD28K_ST2082_RGB_Dual_12G_RGB_Display", {
		//     "UHD28K_ST2082_RGB_Dual_12G_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable12GOut | kEnable4KTSI),
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_2160_DualLink_12Gb, true, false}},
		// { "UHD28K_ST2082_Quad_12G_RGB_Display", {
		//     "UHD28K_ST2082_Quad_12G_RGB_Display",
		//     ConnectionKind::SDI,
		//     NTV2_MODE_DISPLAY,
		//     2, 4,
		//     (kEnable12GOut | kEnable4KTSI),
		//     "",
		//     {},
		//     RasterDefinition::UHD_4K,
		//     HDMIWireFormat::Unknown,
		//     VPIDStandard_4320_QuadLink_12Gb, true, false}},
		/*
		 * SDI YCbCr Capture
		 */
		{"SD_ST352_YCbCr_Capture",
		 {"SD_ST352_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::SD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_483_576,
		  1,
		  1,
		  0,
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  false,
		  false}},
		{"HD_720p_ST292_YCbCr_Capture",
		 {"HD_720p_ST292_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720,
		  1,
		  1,
		  0,
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  false,
		  false}},
		{"HD_1080_ST292_YCbCr_Capture",
		 {"HD_1080_ST292_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080,
		  1,
		  1,
		  0,
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  false,
		  false}},
		{"HD_1080_ST372_Dual_YCbCr_Capture",
		 {"HD_1080_ST372_Dual_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink,
		  2,
		  2,
		  0,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0]",
		  {},
		  false,
		  false}},
		{"HD_720p_ST425_3Ga_YCbCr_Capture",
		 {"HD_720p_ST425_3Ga_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Ga,
		  1,
		  1,
		  kEnable3GOut,
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  false,
		  false}},
		{"HD_1080p_ST425_3Ga_YCbCr_Capture",
		 {"HD_1080p_ST425_3Ga_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga,
		  1,
		  1,
		  kEnable3GOut,
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  false,
		  false}},
		{"HD_1080p_ST425_3Gb_DL_YCbCr_Capture",
		 {"HD_1080p_ST425_3Gb_DL_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb,
		  1,
		  1,
		  (kEnable3GOut | kConvert3GIn | kConvert3GOut),
		  "sdi[{ch1}][0]->fb[{ch1}][0]",
		  {},
		  false,
		  false}},
		{"HD_720p_ST425_3Gb_YCbCr_Capture",
		 {"HD_720p_ST425_3Gb_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Gb,
		  1,
		  2,
		  kEnable3GOut,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch1}][1]->fb[{ch2}][0];",
		  {},
		  false,
		  false}},
		{"HD_1080p_ST425_3Gb_YCbCr_Capture",
		 {"HD_1080p_ST425_3Gb_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Gb,
		  1,
		  2,
		  kEnable3GOut,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch1}][1]->fb[{ch2}][0];",
		  {},
		  false,
		  false}},
		{"HD_1080p_ST425_Dual_3Ga_YCbCr_Capture",
		 {"HD_1080p_ST425_Dual_3Ga_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Ga,
		  2,
		  2,
		  0,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0];",
		  {},
		  false,
		  false}},
		{"HD_1080p_ST425_Dual_3Gb_YCbCr_Capture",
		 {"HD_1080p_ST425_Dual_3Gb_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Gb,
		  2,
		  2,
		  0,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST292_Quad_1_5_Squares_YCbCr_Capture",
		 {"UHD4K_ST292_Quad_1_5_Squares_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080,
		  4,
		  4,
		  kEnable4KSquares,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0];"
		  "sdi[{ch3}][0]->fb[{ch3}][0];"
		  "sdi[{ch4}][0]->fb[{ch4}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST425_Quad_3Ga_Squares_YCbCr_Capture",
		 {"UHD4K_ST425_Quad_3Ga_Squares_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga,
		  4,
		  4,
		  kEnable4KSquares,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0];"
		  "sdi[{ch3}][0]->fb[{ch3}][0];"
		  "sdi[{ch4}][0]->fb[{ch4}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST425_Quad_3Gb_Squares_YCbCr_Capture",
		 {"UHD4K_ST425_Quad_3Gb_Squares_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb,
		  4,
		  4,
		  kEnable4KSquares | kConvert3GIn,
		  "sdi[{ch1}][0]->fb[{ch1}][0];"
		  "sdi[{ch2}][0]->fb[{ch2}][0];"
		  "sdi[{ch3}][0]->fb[{ch3}][0];"
		  "sdi[{ch4}][0]->fb[{ch4}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST425_Dual_3Gb_2SI_YCbCr_Capture",
		 {"UHD4K_ST425_Dual_3Gb_2SI_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_DualLink,
		  2,
		  2,
		  kEnable4KTSI,
		  "sdi[{ch1}][0]->tsi[{ch1}][0];"
		  "sdi[{ch1}][1]->tsi[{ch1}][1];"
		  "sdi[{ch2}][0]->tsi[{ch2}][0];"
		  "sdi[{ch2}][1]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST425_Quad_3Ga_2SI_YCbCr_Capture",
		 {"UHD4K_ST425_Quad_3Ga_2SI_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadLink_3Ga,
		  4,
		  4,
		  kEnable4KTSI,
		  "sdi[{ch1}][0]->tsi[{ch1}][0];"
		  "sdi[{ch2}][0]->tsi[{ch1}][1];"
		  "sdi[{ch3}][0]->tsi[{ch2}][0];"
		  "sdi[{ch4}][0]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST425_Quad_3Gb_2SI_YCbCr_Capture",
		 {"UHD4K_ST425_Quad_3Gb_2SI_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadDualLink_3Gb,
		  4,
		  4,
		  (kConvert3GIn | kEnable4KTSI),
		  "sdi[{ch1}][0]->tsi[{ch1}][0];"
		  "sdi[{ch2}][0]->tsi[{ch1}][1];"
		  "sdi[{ch3}][0]->tsi[{ch2}][0];"
		  "sdi[{ch4}][0]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Capture",
		 {"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_6Gb,
		  1,
		  1,
		  (kEnable6GOut | kEnable4KTSI),
		  "sdi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Capture (Kona5/io4K+)",
		 {"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Capture (Kona5/io4K+)",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_6Gb,
		  4,
		  4,
		  (kEnable6GOut | kEnable4KTSI),
		  "sdi[{ch1}][0]->tsi[{ch1}][0];"
		  "sdi[{ch2}][0]->tsi[{ch1}][1];"
		  "sdi[{ch3}][0]->tsi[{ch2}][0];"
		  "sdi[{ch4}][0]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {DEVICE_ID_KONA5, DEVICE_ID_IO4KPLUS},
		  false,
		  false}},
		{"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Capture",
		 {"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_12Gb,
		  1,
		  1,
		  (kEnable12GOut | kConvert3GaRGBOut | kEnable4KTSI),
		  "sdi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Capture (Kona5/io4K+)",
		 {"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Capture (Kona5/io4K+)",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_12Gb,
		  4,
		  4,
		  (kEnable6GOut | kEnable4KTSI),
		  "sdi[{ch1}][0]->tsi[{ch1}][0];"
		  "sdi[{ch2}][0]->tsi[{ch1}][1];"
		  "sdi[{ch3}][0]->tsi[{ch2}][0];"
		  "sdi[{ch4}][0]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->fb[{ch1}][0];"
		  "tsi[{ch1}][1]->fb[{ch1}][1];"
		  "tsi[{ch2}][0]->fb[{ch2}][0];"
		  "tsi[{ch2}][1]->fb[{ch2}][1];",
		  {DEVICE_ID_KONA5, DEVICE_ID_IO4KPLUS},
		  false,
		  false}},
		// TODO
		{"UHD28K_ST2082_Dual_12G_YCbCr_Capture",
		 {"UHD28K_ST2082_Dual_12G_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_4320_DualLink_12Gb,
		  2,
		  2,
		  (kEnable12GOut | kConvert3GaRGBOut | kEnable4KTSI),
		  "sdi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  false,
		  false}},
		// TODO
		{"UHD28K_ST2082_Quad_12G_YCbCr_Capture",
		 {"UHD28K_ST2082_Quad_12G_YCbCr_Capture",
		  ConnectionKind::SDI,
		  NTV2_MODE_CAPTURE,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_4320_QuadLink_12Gb,
		  4,
		  4,
		  (kEnable12GOut | kConvert3GaRGBOut | kEnable4KTSI),
		  "sdi[{ch1}][0]->fb[{ch1}][0];",
		  {},
		  false,
		  false}},
		/*
		 * SDI YCbCr Display
		 */
		{"SD_ST352_YCbCr_Display",
		 {"SD_ST352_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::SD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_483_576,
		  1,
		  1,
		  0,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  false,
		  false}},
		{"HD_720p_ST292_YCbCr_Display",
		 {"HD_720p_ST292_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720,
		  1,
		  1,
		  0,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  false,
		  false}},
		{"HD_1080_ST292_YCbCr_Display",
		 {"HD_1080_ST292_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080,
		  1,
		  1,
		  0,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  false,
		  false}},
		{"HD_1080_ST372_Dual_YCbCr_Display",
		 {"HD_1080_ST372_Dual_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink,
		  2,
		  2,
		  0,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch2}][0]",
		  {},
		  false,
		  false}},
		{"HD_720p_ST425_3Ga_YCbCr_Display",
		 {"HD_720p_ST425_3Ga_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Ga,
		  1,
		  1,
		  kEnable3GOut,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  false,
		  false}},
		{"HD_1080p_ST425_3Ga_YCbCr_Display",
		 {"HD_1080p_ST425_3Ga_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga,
		  1,
		  1,
		  kEnable3GOut,
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  false,
		  false}},
		{"HD_1080p_ST425_3Gb_DL_YCbCr_Display",
		 {"HD_1080p_ST425_3Gb_DL_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb,
		  1,
		  1,
		  (kEnable3GOut | kConvert3GIn | kConvert3GOut),
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  false,
		  false}},
		{"HD_720p_ST425_3Gb_YCbCr_Display",
		 {"HD_720p_ST425_3Gb_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_720_3Gb,
		  1,
		  2,
		  kEnable3GOut,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch1}][1];",
		  {},
		  false,
		  false}},
		{"HD_1080p_ST425_3Gb_YCbCr_Display",
		 {"HD_1080p_ST425_3Gb_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Gb,
		  1,
		  2,
		  kEnable3GOut,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch1}][1];",
		  {},
		  false,
		  false}},
		{"HD_1080p_ST425_Dual_3Ga_YCbCr_Display",
		 {"HD_1080p_ST425_Dual_3Ga_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Ga,
		  2,
		  2,
		  kEnable3GOut,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch1}][1];",
		  {},
		  false,
		  false}},
		{"HD_1080p_ST425_Dual_3Gb_YCbCr_Display",
		 {"HD_1080p_ST425_Dual_3Gb_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::HD,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_Dual_3Gb,
		  2,
		  2,
		  kEnable3GOut,
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch1}][1];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST292_Quad_1_5_Squares_YCbCr_Display",
		 {"UHD4K_ST292_Quad_1_5_Squares_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080,
		  4,
		  4,
		  (kEnable3GbOut | kEnable4KSquares),
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch2}][0];"
		  "fb[{ch3}][0]->sdi[{ch3}][0];"
		  "fb[{ch4}][0]->sdi[{ch4}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST425_Quad_3Ga_Squares_YCbCr_Display",
		 {"UHD4K_ST425_Quad_3Ga_Squares_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_3Ga,
		  4,
		  4,
		  (kEnable3GOut | kEnable3GbOut),
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch2}][0];"
		  "fb[{ch3}][0]->sdi[{ch3}][0];"
		  "fb[{ch4}][0]->sdi[{ch4}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST425_Quad_3Gb_Squares_YCbCr_Display",
		 {"UHD4K_ST425_Quad_3Gb_Squares_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_1080_DualLink_3Gb,
		  4,
		  4,
		  (kEnable3GOut | kEnable3GbOut | kEnable4KSquares),
		  "fb[{ch1}][0]->sdi[{ch1}][0];"
		  "fb[{ch2}][0]->sdi[{ch2}][0];"
		  "fb[{ch3}][0]->sdi[{ch3}][0];"
		  "fb[{ch4}][0]->sdi[{ch4}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST425_Dual_3Gb_2SI_YCbCr_Display",
		 {"UHD4K_ST425_Dual_3Gb_2SI_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_DualLink,
		  2,
		  2,
		  (kEnable3GOut | kEnable4KTSI),
		  "fb[{ch1}][0]->tsi[{ch1}][0];"
		  "fb[{ch1}][1]->tsi[{ch1}][1];"
		  "fb[{ch2}][0]->tsi[{ch2}][0];"
		  "fb[{ch2}][1]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->sdi[{ch1}][0];"
		  "tsi[{ch1}][1]->sdi[{ch1}][1];"
		  "tsi[{ch2}][0]->sdi[{ch2}][0];"
		  "tsi[{ch2}][1]->sdi[{ch2}][1];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST425_Quad_3Ga_2SI_YCbCr_Display",
		 {"UHD4K_ST425_Quad_3Ga_2SI_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadLink_3Ga,
		  4,
		  4,
		  (kEnable3GOut | kEnable4KTSI),
		  "fb[{ch1}][0]->tsi[{ch1}][0];"
		  "fb[{ch1}][1]->tsi[{ch1}][1];"
		  "fb[{ch2}][0]->tsi[{ch2}][0];"
		  "fb[{ch2}][1]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->sdi[{ch1}][0];"
		  "tsi[{ch1}][1]->sdi[{ch2}][0];"
		  "tsi[{ch2}][0]->sdi[{ch3}][0];"
		  "tsi[{ch2}][1]->sdi[{ch4}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST425_Quad_3Gb_2SI_YCbCr_Display",
		 {"UHD4K_ST425_Quad_3Gb_2SI_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_QuadDualLink_3Gb,
		  4,
		  4,
		  (kEnable3GOut | kConvert3GaRGBOut | kEnable4KTSI),
		  "fb[{ch1}][0]->tsi[{ch1}][0];"
		  "fb[{ch1}][1]->tsi[{ch1}][1];"
		  "fb[{ch2}][0]->tsi[{ch2}][0];"
		  "fb[{ch2}][1]->tsi[{ch2}][1];"
		  "tsi[{ch1}][0]->sdi[{ch1}][0];"
		  "tsi[{ch1}][1]->sdi[{ch2}][0];"
		  "tsi[{ch2}][0]->sdi[{ch3}][0];"
		  "tsi[{ch2}][1]->sdi[{ch4}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Display",
		 {"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_6Gb,
		  1,
		  1,
		  (kEnable6GOut | kEnable4KTSI),
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Display (Kona5/io4K+)",
		 {"UHD4K_ST2018_6G_Squares_2SI_YCbCr_Display (Kona5/io4K+)",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_6Gb,
		  4,
		  4,
		  (kEnable6GOut | kEnable4KTSI),
		  "fb[{ch3}][0]->tsi[{ch3}][0];"
		  "fb[{ch3}][1]->tsi[{ch3}][1];"
		  "fb[{ch4}][0]->tsi[{ch4}][0];"
		  "fb[{ch4}][1]->tsi[{ch4}][1];"
		  "tsi[{ch3}][0]->sdi[{ch1}][0];"
		  "tsi[{ch3}][1]->sdi[{ch2}][0];"
		  "tsi[{ch4}][0]->sdi[{ch3}][0];"
		  "tsi[{ch4}][1]->sdi[{ch4}][0];",
		  {DEVICE_ID_KONA5, DEVICE_ID_IO4KPLUS},
		  false,
		  true}},
		{"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Display",
		 {"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_12Gb,
		  1,
		  1,
		  (kEnable12GOut | kEnable4KTSI),
		  "fb[{ch1}][0]->sdi[{ch1}][0];",
		  {},
		  false,
		  false}},
		{"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Display (Kona5/io4K+)",
		 {"UHD4K_ST2018_12G_Squares_2SI_YCbCr_Display (Kona5/io4K+)",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_2160_Single_12Gb,
		  4,
		  4,
		  (kEnable12GOut | kConvert3GOut | kEnable4KTSI),
		  "fb[{ch3}][0]->tsi[{ch3}][0];"
		  "fb[{ch3}][1]->tsi[{ch3}][1];"
		  "fb[{ch4}][0]->tsi[{ch4}][0];"
		  "fb[{ch4}][1]->tsi[{ch4}][1];"
		  "tsi[{ch3}][0]->sdi[{ch1}][0];"
		  "tsi[{ch3}][1]->sdi[{ch2}][0];"
		  "tsi[{ch4}][0]->sdi[{ch3}][0];"
		  "tsi[{ch4}][1]->sdi[{ch4}][0];",
		  {DEVICE_ID_KONA5, DEVICE_ID_IO4KPLUS},
		  false,
		  true}},
		// TODO
		{"UHD28K_ST2082_Dual_12G_YCbCr_Display",
		 {"UHD28K_ST2082_Dual_12G_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_4320_DualLink_12Gb,
		  2,
		  2,
		  (kEnable12GOut | kConvert3GOut | kEnable4KTSI),
		  "",
		  {},
		  false,
		  false}},
		// TODO
		{"UHD28K_ST2082_Quad_12G_YCbCr_Display",
		 {"UHD28K_ST2082_Quad_12G_YCbCr_Display",
		  ConnectionKind::SDI,
		  NTV2_MODE_DISPLAY,
		  RasterDefinition::UHD_4K,
		  HDMIWireFormat::Unknown,
		  VPIDStandard_4320_QuadLink_12Gb,
		  4,
		  4,
		  (kEnable12GOut | kConvert3GOut | kEnable4KTSI),
		  "",
		  {},
		  false,
		  false}},
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
					  HDMIWireFormat hwf,
					  RoutingPreset &preset)
{
	if (NTV2DeviceCanDoVideoFormat(id, vf) &&
	    NTV2DeviceCanDoFrameBufferFormat(id, pf)) {
		const auto &rd = DetermineRasterDefinition(vf);
		bool is_rgb = NTV2_IS_FBF_RGB(pf);
		std::vector<RoutingPresetPair> query;
		for (const auto &p : m_presets) {
			if (p.second.kind == kind && p.second.mode == mode &&
			    p.second.raster_def == rd &&
			    p.second.is_rgb == is_rgb &&
			    p.second.vpid_standard == standard &&
			    p.second.hdmi_wire_format == hwf) {
				query.push_back(p);
			}
		}
		RoutingPresets device_presets;
		RoutingPresets non_device_presets;
		for (auto &q : query) {
			if (q.second.device_ids.size() == 0)
				non_device_presets.push_back(q.second);
			for (const auto &device_id : q.second.device_ids) {
				if (device_id == id) {
					device_presets.push_back(q.second);
					break;
				}
			}
		}

		if (device_presets.size() > 0) {
			preset = device_presets.at(0);
			return true;
		}
		if (non_device_presets.size() > 0) {
			preset = non_device_presets.at(0);
			return true;
		}
	}

	return false;
}

} // namespace aja
