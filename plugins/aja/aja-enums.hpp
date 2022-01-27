#pragma once

#include <ajantv2/includes/ntv2vpid.h>

#include <utility>

// Flags corresponding to card register enables
typedef enum {
	kEnable3GOut = 1 << 0,
	kEnable6GOut = 1 << 1,
	kEnable12GOut = 1 << 2,
	kConvert3GIn = 1 << 3,
	kConvert3GOut = 1 << 4,
	kConvert3GaRGBOut = 1 << 5,
	kEnable3GbOut = 1 << 6,
	kEnable4KSquares = 1 << 7,
	kEnable8KSquares = 1 << 8,
	kEnable4KTSI = 1 << 9,
} RoutingPresetFlags;

enum class ConnectionKind { SDI = 0, HDMI = 1, Analog = 2, Unknown };

enum class IOSelection {
	SDI1 = 0,
	SDI2 = 1,
	SDI3 = 2,
	SDI4 = 3,
	SDI5 = 4,
	SDI6 = 5,
	SDI7 = 6,
	SDI8 = 7,
	SDI1_2 = 8,
	// special case for 2xSDI 4K Squares (requires 4x framestores)
	SDI1_2_Squares = 9,
	SDI3_4 = 10,
	// special case for 2xSDI 4K Squares (requires 4x framestores)
	SDI3_4_Squares = 11,
	SDI5_6 = 12,
	SDI7_8 = 13,
	SDI1__4 = 14,
	SDI5__8 = 15,
	HDMI1 = 16,
	HDMI2 = 17,
	HDMI3 = 18,
	HDMI4 = 19,
	HDMIMonitorIn = 20,
	HDMIMonitorOut = 21,
	AnalogIn = 22,
	AnalogOut = 23,
	Invalid = 24,
	NumIOSelections = Invalid
};

enum class SDITransport {
	SingleLink = 0, // SD/HD up to 1.5Gbps link
	HDDualLink = 1, // HD Dual-1.5Gbps Links
	SDI3Ga = 2,     // 3Gbps Level-A
	SDI3Gb = 3,     // 3Gbps Level-B
	SDI6G = 4,      // 6Gbps
	SDI12G = 5,     // 12Gbps
	Unknown
};

enum class SDITransport4K { Squares = 0, TwoSampleInterleave = 1, Unknown = 2 };

enum class RasterDefinition {
	SD = 0,
	HD = 1,
	UHD_4K = 2,
	UHD2_8K = 3,
	Unknown
};

enum class HDMIWireFormat {
	SD_HD_YCBCR = 0,
	SD_HD_RGB = 1,
	UHD_4K_YCBCR = 2,
	UHD_4K_RGB = 3,
	Unknown
};

using VPIDSpec = std::pair<RasterDefinition, VPIDStandard>;
