#pragma once

#include <string>

#if defined(_WIN32) || defined(__APPLE__)
#define VIRTUAL_CAM_ID "virtualcam_output"
#else
#define VIRTUAL_CAM_ID "v4l2_output"
#endif

enum VCamOutputType {
	Invalid,
	SceneOutput,
	SourceOutput,
	ProgramView,
	PreviewOutput,
};

// Kept for config upgrade
enum VCamInternalType {
	Default,
	Preview,
};

struct VCamConfig {
	VCamOutputType type = VCamOutputType::ProgramView;
	std::string scene;
	std::string source;
};
