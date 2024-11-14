#pragma once

#include "aja-props.hpp"
#include "aja-presets.hpp"

#include <ajantv2/includes/ntv2enums.h>
#include <ajantv2/includes/ntv2signalrouter.h>

#include <string>

class CNTV2Card;

namespace aja {
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

// Applies RoutingConfig settings to the card to configure a specific SDI/HDMI capture/output mode.
class Routing {
public:
	static bool ParseRouteString(const std::string &route,
				     NTV2XptConnections &cnx);

	static void StartSourceAudio(const SourceProps &props, CNTV2Card *card);
	static void StopSourceAudio(const SourceProps &props, CNTV2Card *card);

	static bool ConfigureSourceRoute(const SourceProps &props,
					 NTV2Mode mode, CNTV2Card *card,
					 NTV2XptConnections &cnx);
	static bool ConfigureOutputRoute(const OutputProps &props,
					 NTV2Mode mode, CNTV2Card *card,
					 NTV2XptConnections &cnx);
	static void ConfigureOutputAudio(const OutputProps &props,
					 CNTV2Card *card);
	static void LogRoutingPreset(const RoutingPreset &rp);
};

} // namespace aja
