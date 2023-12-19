#pragma once

#include <string>

enum VCamOutputType {
	InternalOutput,
	SceneOutput,
	SourceOutput,
};

enum VCamInternalType {
	Default,
	Preview,
};

struct VCamConfig {
	VCamOutputType type = VCamOutputType::InternalOutput;
	VCamInternalType internal = VCamInternalType::Default;
	std::string scene;
	std::string source;
};
