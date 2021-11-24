#pragma once

// Additional enums used throughout the AJA plugins for signal routing configuration.

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

enum class SDI4KTransport { Squares = 0, TwoSampleInterleave = 1, Unknown = 2 };

enum class RasterDefinition {
	SD = 0,
	HD = 1,
	UHD_4K = 2,
	UHD_4K_Retail_12G = 3,
	UHD2_8K = 4,
	Unknown = 5
};

enum class SDIWireFormat {
	SD_ST352 = 0,
	HD_720p_ST292 = 1,
	HD_1080_ST292 = 2,
	HD_1080_ST372_Dual = 3,
	HD_720p_ST425_3Ga = 4,
	HD_1080p_ST425_3Ga = 5,
	HD_1080p_ST425_3Gb_DL = 6,
	HD_720p_ST425_3Gb = 7,
	HD_1080p_ST425_3Gb = 8,
	HD_1080p_ST425_Dual_3Ga = 9,
	HD_1080p_ST425_Dual_3Gb = 10,
	UHD4K_ST292_Dual_1_5_Squares = 11,
	UHD4K_ST292_Quad_1_5_Squares = 12,
	UHD4K_ST425_Quad_3Ga_Squares = 13,
	UHD4K_ST425_Quad_3Gb_Squares = 14,
	UHD4K_ST425_Dual_3Gb_2SI = 15,
	UHD4K_ST425_Quad_3Ga_2SI = 16,
	UHD4K_ST425_Quad_3Gb_2SI = 17,
	UHD4K_ST2018_6G_Squares_2SI = 18,
	UHD4K_ST2018_6G_Squares_2SI_Kona5_io4KPlus = 19,
	UHD4K_ST2018_12G_Squares_2SI = 20,
	UHD4K_ST2018_12G_Squares_2SI_Kona5_io4KPlus = 21,
	UHD28K_ST2082_Dual_12G = 22,
	UHD28K_ST2082_RGB_Dual_12G = 23,
	UHD28K_ST2082_Quad_12G = 24,
	Unknown = 25,
};

enum class HDMIWireFormat {
	SD_HDMI = 0,
	HD_YCBCR_LFR = 1,
	HD_YCBCR_HFR = 2,
	HD_RGB_LFR = 3,
	HD_RGB_HFR = 4,
	UHD_4K_YCBCR_LFR = 5,
	UHD_4K_YCBCR_HFR = 6,
	UHD_4K_RGB_LFR = 7,
	UHD_4K_RGB_HFR = 8,
	TTAP_PRO = 9,
	Unknown = 10
};
