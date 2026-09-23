#include "CurlPublicRedirect.hpp"

#include <curl/curl.h>

#include <cctype>
#include <cstdlib>
#include <cstring>

namespace {

struct HeaderList {
	std::vector<std::string> lines;
};

size_t WriteBody(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	auto *out = static_cast<std::string *>(userdata);
	const size_t total = size * nmemb;
	out->append(ptr, total);
	return total;
}

size_t WriteHeader(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	auto *headers = static_cast<HeaderList *>(userdata);
	const size_t total = size * nmemb;
	std::string line(ptr, total);
	while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
		line.pop_back();
	}
	if (!line.empty()) {
		headers->lines.push_back(std::move(line));
	}
	return total;
}

bool StartsWithInsensitive(const std::string &text, const char *prefix)
{
	const size_t length = std::strlen(prefix);
	if (text.size() < length) {
		return false;
	}
	for (size_t i = 0; i < length; i++) {
		const unsigned char left = static_cast<unsigned char>(text[i]);
		const unsigned char right = static_cast<unsigned char>(prefix[i]);
		if (std::tolower(left) != std::tolower(right)) {
			return false;
		}
	}
	return true;
}

bool IsAuthorizationHeader(const std::string &header)
{
	return StartsWithInsensitive(header, "authorization:") || StartsWithInsensitive(header, "proxy-authorization:");
}

std::string HeaderValue(const std::string &header)
{
	const size_t colon = header.find(':');
	if (colon == std::string::npos) {
		return {};
	}
	size_t start = colon + 1;
	while (start < header.size() && (header[start] == ' ' || header[start] == '\t')) {
		start++;
	}
	return header.substr(start);
}

std::string LocationFrom(const HeaderList &headers)
{
	for (const std::string &header : headers.lines) {
		if (StartsWithInsensitive(header, "location:")) {
			return HeaderValue(header);
		}
	}
	return {};
}

struct UrlView {
	std::string host;
	int port = 0;
	bool ok = false;
};

UrlView ParseUrl(const std::string &url)
{
	UrlView view;
	const size_t scheme = url.find("://");
	if (scheme == std::string::npos) {
		return view;
	}
	const std::string proto = url.substr(0, scheme);
	const int defaultPort = (proto == "https" || proto == "HTTPS") ? 443 : 80;
	const size_t hostStart = scheme + 3;
	const size_t path = url.find('/', hostStart);
	std::string hostport = path == std::string::npos ? url.substr(hostStart)
							 : url.substr(hostStart, path - hostStart);
	const size_t at = hostport.rfind('@');
	if (at != std::string::npos) {
		hostport = hostport.substr(at + 1);
	}
	if (hostport.empty()) {
		return view;
	}
	if (hostport[0] == '[') {
		const size_t end = hostport.find(']');
		if (end == std::string::npos) {
			return view;
		}
		view.host = hostport.substr(1, end - 1);
		if (end + 1 < hostport.size() && hostport[end + 1] == ':') {
			view.port = std::atoi(hostport.c_str() + end + 2);
		} else {
			view.port = defaultPort;
		}
	} else {
		const size_t colon = hostport.rfind(':');
		if (colon != std::string::npos) {
			view.host = hostport.substr(0, colon);
			view.port = std::atoi(hostport.c_str() + colon + 1);
		} else {
			view.host = hostport;
			view.port = defaultPort;
		}
	}
	for (char &ch : view.host) {
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}
	view.ok = !view.host.empty() && view.port > 0;
	return view;
}

std::string ResolveRedirect(const std::string &base, const std::string &location)
{
	if (location.find("://") != std::string::npos) {
		return location;
	}
	const size_t scheme = base.find("://");
	if (scheme == std::string::npos) {
		return location;
	}
	if (location.rfind("//", 0) == 0) {
		return base.substr(0, scheme + 1) + location;
	}
	const size_t path = base.find('/', scheme + 3);
	const std::string origin = path == std::string::npos ? base : base.substr(0, path);
	if (!location.empty() && location[0] == '/') {
		return origin + location;
	}
	const std::string prefix = path == std::string::npos ? origin + "/" : base.substr(0, base.rfind('/') + 1);
	return prefix + location;
}

bool DropsAuthorization(const std::string &from, const std::string &to)
{
	const UrlView left = ParseUrl(from);
	const UrlView right = ParseUrl(to);
	if (!left.ok || !right.ok) {
		return true;
	}
	return left.host != right.host || left.port != right.port;
}

std::vector<std::string> WithoutAuthorization(const std::vector<std::string> &headers)
{
	std::vector<std::string> kept;
	kept.reserve(headers.size());
	for (const std::string &header : headers) {
		if (!IsAuthorizationHeader(header)) {
			kept.push_back(header);
		}
	}
	return kept;
}

bool IsRedirect(long status)
{
	return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
}

} // namespace

ObsPublicGetResult obs_curl_get_follow_public(const char *url, const std::vector<std::string> &headers, int timeoutSec)
{
	ObsPublicGetResult result;
	if (!url || !url[0]) {
		result.error = "missing url";
		return result;
	}

	std::string current = url;
	std::vector<std::string> active = headers;
	for (int hop = 0; hop < 6; hop++) {
		std::string body;
		HeaderList responseHeaders;
		CURL *curl = curl_easy_init();
		if (!curl) {
			result.error = "curl init failed";
			return result;
		}
		struct curl_slist *header = nullptr;
		for (const std::string &line : active) {
			header = curl_slist_append(header, line.c_str());
		}
		curl_easy_setopt(curl, CURLOPT_URL, current.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
		curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 0L);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 0L);
#ifdef CURLOPT_PROTOCOLS_STR
		curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https");
#endif
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteBody);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteHeader);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);
		if (timeoutSec > 0) {
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
		}
		const CURLcode code = curl_easy_perform(curl);
		long status = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
		curl_slist_free_all(header);
		curl_easy_cleanup(curl);
		if (code != CURLE_OK) {
			result.status = status;
			result.error = curl_easy_strerror(code);
			return result;
		}
		if (!IsRedirect(status)) {
			result.transportOk = true;
			result.status = status;
			result.body = std::move(body);
			return result;
		}
		if (hop == 5) {
			result.status = status;
			result.error = "too many redirects";
			return result;
		}
		const std::string location = LocationFrom(responseHeaders);
		if (location.empty()) {
			result.status = status;
			result.error = "redirect missing Location";
			return result;
		}
		const std::string next = ResolveRedirect(current, location);
		if (DropsAuthorization(current, next)) {
			active = WithoutAuthorization(active);
			result.droppedAuthorization = true;
		}
		current = next;
	}
	result.error = "too many redirects";
	return result;
}
