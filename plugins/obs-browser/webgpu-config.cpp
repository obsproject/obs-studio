#include "webgpu-config.hpp"

#include "cef-headers.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

namespace {
std::string trim(std::string value)
{
	auto is_space = [](unsigned char character) { return std::isspace(character) != 0; };
	value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), is_space));
	value.erase(std::find_if_not(value.rbegin(), value.rend(), is_space).base(), value.end());
	return value;
}

std::string lower(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(),
		       [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
	return value;
}

bool valid_hostname(const std::string &host)
{
	if (host.empty() || host.size() > 253)
		return false;

	for (const unsigned char character : host) {
		if (!std::isalnum(character) && character != '.' && character != '-')
			return false;
	}
	return host.front() != '.' && host.back() != '.' && host.find("..") == std::string::npos;
}

bool normalize_origin(const std::string &value, std::string &normalized)
{
	CefURLParts parts;
	if (!CefParseURL(value, parts))
		return false;

	const std::string scheme = lower(CefString(&parts.scheme).ToString());
	const std::string host = lower(CefString(&parts.host).ToString());
	const std::string username = CefString(&parts.username).ToString();
	const std::string password = CefString(&parts.password).ToString();
	const std::string path = CefString(&parts.path).ToString();
	const std::string query = CefString(&parts.query).ToString();
	const std::string fragment = CefString(&parts.fragment).ToString();
	const std::string port = CefString(&parts.port).ToString();

	if (scheme != "http" || !valid_hostname(host) || !username.empty() || !password.empty() ||
	    (!path.empty() && path != "/") || !query.empty() || !fragment.empty())
		return false;

	normalized = "http://" + host;
	if (!port.empty() && port != "80")
		normalized += ":" + port;
	return true;
}

} // namespace

std::string BrowserWebGPUConfig::CommandLineOrigins() const
{
	std::ostringstream result;
	for (size_t index = 0; index < insecure_origins.size(); index++) {
		if (index)
			result << ',';
		result << insecure_origins[index];
	}
	return result.str();
}

bool BrowserWebGPUConfig::OriginAllowed(const std::string &url) const
{
	CefURLParts parts;
	if (!CefParseURL(url, parts))
		return false;

	const std::string scheme = lower(CefString(&parts.scheme).ToString());
	if (scheme == "https")
		return true;
	if (scheme != "http")
		return false;

	const std::string host = lower(CefString(&parts.host).ToString());
	const std::string port = CefString(&parts.port).ToString();
	std::string origin = "http://" + host;
	if (!port.empty() && port != "80")
		origin += ":" + port;

	for (const std::string &entry : insecure_origins) {
		if (entry == origin)
			return true;
	}

	return false;
}

BrowserWebGPUConfig ParseBrowserWebGPUConfig(const std::string &mode, const std::string &origins,
						     std::vector<std::string> &invalid_entries)
{
	BrowserWebGPUConfig config;
	config.mode = lower(trim(mode)) == "auto" ? BrowserWebGPUMode::Auto : BrowserWebGPUMode::Disabled;

	std::set<std::string> unique_origins = {"http://absolute"};
	std::string token;
	for (const char character : origins) {
		if (character == ',' || character == ';' || character == '\n' || character == '\r') {
			const std::string entry = trim(token);
			if (!entry.empty()) {
				std::string normalized;
				if (normalize_origin(entry, normalized))
					unique_origins.insert(normalized);
				else
					invalid_entries.push_back(entry);
			}
			token.clear();
		} else {
			token += character;
		}
	}

	const std::string entry = trim(token);
	if (!entry.empty()) {
		std::string normalized;
		if (normalize_origin(entry, normalized))
			unique_origins.insert(normalized);
		else
			invalid_entries.push_back(entry);
	}

	config.insecure_origins.assign(unique_origins.begin(), unique_origins.end());
	return config;
}
