#pragma once
#include "aja-props.hpp"

#include <ajantv2/includes/ntv2enums.h>

#include <iostream>
#include <string>
#include <map>
#include <vector>

class CNTV2Card;

/* The AJA hardware and NTV2 SDK uses a concept called "Signal Routing" to connect high-level firmware
 * blocks known as "Widgets" to one another via "crosspoint" connections. This facilitates streaming
 * data from one Widget to another to achieve specific functionality.
 * Such functionality may include SDI/HDMI capture/output, colorspace conversion, hardware LUTs, etc.
 * 
 * This code references a table of RoutingConfig entries, where each entry contains the settings required
 * to configure an AJA device for a particular capture or output task. These settings include the number of
 * physical IO Widgets (SDI or HDMI) required, number of framestore Widgets required, register settings
 * that must be enabled or disabled, and a special short-hand "route string".
 * Of special note is the route string, which is parsed into a map of NTV2XptConnections. These connections
 * are then applied as the "signal route", connecting the Widget's crosspoints together.
 */

struct RoutingConfig {
	NTV2Mode mode;            // capture or playout?
	uint32_t num_wires;       // number of physical connections
	uint32_t num_framestores; // number of framestores used
	bool enable_3g_out;       // enable register for 3G SDI Output?
	bool enable_6g_out;       // enable register for 6G SDI Output?
	bool enable_12g_out;      // enable register for 12G SDI Output?
	bool convert_3g_in; // enable register for 3G level-B -> level-A SDI input conversion?
	bool convert_3g_out; // enable register for 3G level-A -> level-B SDI output conversion?
	bool enable_rgb_3ga_convert; // enable register for RGB 3G level-B -> level-A SDI output conversion?
	bool enable_3gb_out;    // enable register for 3G level-B SDI output?
	bool enable_4k_squares; // enable register for 4K square division?
	bool enable_8k_squares; // enable register for 8K square division?
	bool enable_tsi; // enable register for two-sample interleave (UHD/4K/8K)
	std::string
		route_string; // signal routing shorthand string to parse into crosspoint connections
};

/* This table is used to correlate a particular "raster definition" (i.e. SD/HD/4K/etc.)
 * and SMPTE VPID transport byte (VPIDStandard) to an SDIWireFormat enum.
 * This allows mapping SDI video signals to the correct format, particularly in the case
 * where multiple SDI formats share the same VPID transport value.
 * For example: VPIDStandard_1080 (0x85) is used on the wire for both single-link (1x SDI wire)
 * 1080-line HD SDI video AND quad-link (4x SDI wires) UHD/4K "square-division" video.
 */
using VPIDSpec = std::pair<RasterDefinition, VPIDStandard>;

static inline const std::map<VPIDSpec, SDIWireFormat> kSDIWireFormats = {
	{{RasterDefinition::SD, VPIDStandard_483_576}, SDIWireFormat::SD_ST352},
	{{RasterDefinition::HD, VPIDStandard_720},
	 SDIWireFormat::HD_720p_ST292},
	{{RasterDefinition::HD, VPIDStandard_1080},
	 SDIWireFormat::HD_1080_ST292},
	{{RasterDefinition::HD, VPIDStandard_1080_DualLink},
	 SDIWireFormat::HD_1080_ST372_Dual},
	{{RasterDefinition::HD, VPIDStandard_720_3Ga},
	 SDIWireFormat::HD_720p_ST425_3Ga},
	{{RasterDefinition::HD, VPIDStandard_1080_3Ga},
	 SDIWireFormat::HD_1080p_ST425_3Ga},
	{{RasterDefinition::HD, VPIDStandard_1080_DualLink_3Gb},
	 SDIWireFormat::HD_1080p_ST425_3Gb_DL},
	{{RasterDefinition::HD, VPIDStandard_720_3Gb},
	 SDIWireFormat::HD_720p_ST425_3Gb},
	{{RasterDefinition::HD, VPIDStandard_1080_3Gb},
	 SDIWireFormat::HD_1080p_ST425_3Gb},
	{{RasterDefinition::HD, VPIDStandard_1080_Dual_3Ga},
	 SDIWireFormat::HD_1080p_ST425_Dual_3Ga},
	{{RasterDefinition::HD, VPIDStandard_1080_Dual_3Gb},
	 SDIWireFormat::HD_1080p_ST425_Dual_3Gb},
	{{RasterDefinition::UHD_4K, VPIDStandard_1080_3Gb},
	 SDIWireFormat::UHD4K_ST292_Dual_1_5_Squares},
	{{RasterDefinition::UHD_4K, VPIDStandard_1080},
	 SDIWireFormat::UHD4K_ST292_Quad_1_5_Squares},
	{{RasterDefinition::UHD_4K, VPIDStandard_1080_3Ga},
	 SDIWireFormat::UHD4K_ST425_Quad_3Ga_Squares},
	{{RasterDefinition::UHD_4K, VPIDStandard_1080_DualLink_3Gb},
	 SDIWireFormat::UHD4K_ST425_Quad_3Gb_Squares},
	{{RasterDefinition::UHD_4K, VPIDStandard_2160_DualLink},
	 SDIWireFormat::UHD4K_ST425_Dual_3Gb_2SI},
	{{RasterDefinition::UHD_4K, VPIDStandard_2160_QuadLink_3Ga},
	 SDIWireFormat::UHD4K_ST425_Quad_3Ga_2SI},
	{{RasterDefinition::UHD_4K, VPIDStandard_2160_QuadDualLink_3Gb},
	 SDIWireFormat::UHD4K_ST425_Quad_3Gb_2SI},
	{{RasterDefinition::UHD_4K, VPIDStandard_2160_Single_6Gb},
	 SDIWireFormat::UHD4K_ST2018_6G_Squares_2SI},
	{{RasterDefinition::UHD_4K_Retail_12G, VPIDStandard_2160_Single_6Gb},
	 SDIWireFormat::UHD4K_ST2018_6G_Squares_2SI_Kona5_io4KPlus},
	{{RasterDefinition::UHD_4K, VPIDStandard_2160_Single_12Gb},
	 SDIWireFormat::UHD4K_ST2018_12G_Squares_2SI},
	{{RasterDefinition::UHD_4K_Retail_12G, VPIDStandard_2160_Single_12Gb},
	 SDIWireFormat::UHD4K_ST2018_12G_Squares_2SI_Kona5_io4KPlus},
	{{RasterDefinition::UHD2_8K, VPIDStandard_4320_DualLink_12Gb},
	 SDIWireFormat::UHD28K_ST2082_Dual_12G},
	{{RasterDefinition::UHD2_8K, VPIDStandard_2160_DualLink_12Gb},
	 SDIWireFormat::UHD28K_ST2082_RGB_Dual_12G},
	{{RasterDefinition::UHD2_8K, VPIDStandard_4320_QuadLink_12Gb},
	 SDIWireFormat::UHD28K_ST2082_Quad_12G},
};

extern RasterDefinition
GetRasterDefinition(IOSelection io, NTV2VideoFormat vf,
		    NTV2DeviceID deviceID = DEVICE_ID_NOTFOUND);

extern std::string RasterDefinitionToString(RasterDefinition rd);

// Applies RoutingConfig settings to the card to configure a specific SDI/HDMI capture/output mode.
class Routing {
public:
	static bool ParseRouteString(const std::string &route,
				     NTV2XptConnections &cnx);
	static bool DetermineSDIWireFormat(NTV2DeviceID deviceID, VPIDSpec spec,
					   SDIWireFormat &swf);
	static bool FindRoutingConfigHDMI(HDMIWireFormat hwf, NTV2Mode mode,
					  bool isRGB, NTV2DeviceID deviceID,
					  RoutingConfig &routing);
	static bool FindRoutingConfigSDI(SDIWireFormat swf, NTV2Mode mode,
					 bool isRGB, NTV2DeviceID deviceID,
					 RoutingConfig &routing);

	static void StartSourceAudio(const SourceProps &props, CNTV2Card *card);

	static void StopSourceAudio(const SourceProps &props, CNTV2Card *card);

	static bool ConfigureSourceRoute(const SourceProps &props,
					 NTV2Mode mode, CNTV2Card *card);
	static bool ConfigureOutputRoute(const OutputProps &props,
					 NTV2Mode mode, CNTV2Card *card);
	static ULWord initial_framestore_output_index(NTV2DeviceID deviceID,
						      IOSelection io,
						      NTV2Channel init_channel);

	static void ConfigureOutputAudio(const OutputProps &props,
					 CNTV2Card *card);
};
