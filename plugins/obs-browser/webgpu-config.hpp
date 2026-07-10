#pragma once

#include <string>
#include <vector>

enum class BrowserWebGPUMode {
	Auto,
	Disabled,
};

struct BrowserWebGPUConfig {
	BrowserWebGPUMode mode = BrowserWebGPUMode::Disabled;
	std::vector<std::string> insecure_origins;

	bool Enabled() const { return mode == BrowserWebGPUMode::Auto; }
	std::string CommandLineOrigins() const;
	bool OriginAllowed(const std::string &url) const;
};

// Parses user-configured HTTP origins and adds the legacy local-file origin.
// Invalid entries are returned for display in the OBS log.
BrowserWebGPUConfig ParseBrowserWebGPUConfig(const std::string &mode, const std::string &origins,
					     std::vector<std::string> &invalid_entries);
