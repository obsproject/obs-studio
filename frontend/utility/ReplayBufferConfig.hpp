#pragma once

#include <string>

enum ReplayBufferOutputType {
	RBOutputProgramView,
	RBOutputSceneView,
};

struct ReplayBufferConfig {
	ReplayBufferOutputType type = RBOutputProgramView;
	std::string scene;
};
