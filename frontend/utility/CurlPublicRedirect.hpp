#pragma once

#include <string>
#include <vector>

// GET /2/region answers 307 and the next host is public. Callers put
// Authorization on the first request only. This follower does not use
// libcurl's automatic redirect, because a custom Authorization header is
// kept on some libcurl versions unless the next host is detected by hand.

struct ObsPublicGetResult {
	bool transportOk = false;
	long status = 0;
	std::string body;
	std::string error;
	bool droppedAuthorization = false;
};

ObsPublicGetResult obs_curl_get_follow_public(const char *url, const std::vector<std::string> &headers, int timeoutSec);
