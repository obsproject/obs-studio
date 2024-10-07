#pragma once

#include "aja-enums.hpp"

#include <ajantv2/includes/ntv2enums.h>

#include <map>
#include <string>
#include <vector>

namespace aja {

struct RoutingPreset {
	std::string name;
	ConnectionKind kind;
	NTV2Mode mode;
	RasterDefinition raster_def;
	HDMIWireFormat hdmi_wire_format;
	VPIDStandard vpid_standard;
	uint32_t num_channels;
	uint32_t num_framestores;
	uint32_t flags;
	std::string route_string;
	std::vector<NTV2DeviceID> device_ids;
	bool is_rgb;
	bool verbatim;
};

using RoutingPresets = std::vector<RoutingPreset>;
using RoutingPresetPair = std::pair<std::string, RoutingPreset>;
using RoutingPresetMap = std::map<std::string, RoutingPreset>;

class RoutingConfigurator {
public:
	RoutingConfigurator();
	void AddPreset(const std::string &name, const RoutingPreset &preset);
	bool PresetByName(const std::string &name, RoutingPreset &preset) const;
	RoutingPresetMap GetPresetTable() const;
	bool FindFirstPreset(ConnectionKind kind, NTV2DeviceID id,
			     NTV2Mode mode, NTV2VideoFormat vf,
			     NTV2PixelFormat pf, VPIDStandard standard,
			     HDMIWireFormat hwf, RoutingPreset &preset);

private:
	void build_preset_table();
	RoutingPresetMap m_presets;
};

} // namespace aja
