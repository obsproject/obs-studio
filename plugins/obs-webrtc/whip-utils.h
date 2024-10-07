#pragma once

#include <obs.h>

#include <string>
#include <random>
#include <sstream>

#define do_log(level, format, ...)                              \
	blog(level, "[obs-webrtc] [whip_output: '%s'] " format, \
	     obs_output_get_name(output), ##__VA_ARGS__)

static uint32_t generate_random_u32()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dist(1, (UINT32_MAX - 1));
	return dist(gen);
}

static std::string trim_string(const std::string &source)
{
	std::string ret(source);
	ret.erase(0, ret.find_first_not_of(" \n\r\t"));
	ret.erase(ret.find_last_not_of(" \n\r\t") + 1);
	return ret;
}

static std::string value_for_header(const std::string &header,
				    const std::string &val)
{
	if (val.size() <= header.size() ||
	    astrcmpi_n(header.c_str(), val.c_str(), header.size()) != 0) {
		return "";
	}

	auto delimiter = val.find_first_of(" ");
	if (delimiter == std::string::npos) {
		return "";
	}

	return val.substr(delimiter + 1);
}

static size_t curl_writefunction(char *data, size_t size, size_t nmemb,
				 void *priv_data)
{
	auto read_buffer = static_cast<std::string *>(priv_data);

	size_t real_size = size * nmemb;

	read_buffer->append(data, real_size);
	return real_size;
}

static size_t curl_header_function(char *data, size_t size, size_t nmemb,
				   void *priv_data)
{
	auto header_buffer = static_cast<std::vector<std::string> *>(priv_data);
	header_buffer->push_back(trim_string(std::string(data, size * nmemb)));
	return size * nmemb;
}

static inline std::string generate_user_agent()
{
#ifdef _WIN64
#define OS_NAME "Windows x86_64"
#elif __APPLE__
#define OS_NAME "Mac OS X"
#elif __OpenBSD__
#define OS_NAME "OpenBSD"
#elif __FreeBSD__
#define OS_NAME "FreeBSD"
#elif __linux__ && __LP64__
#define OS_NAME "Linux x86_64"
#else
#define OS_NAME "Linux"
#endif

	// Build the user-agent string
	std::stringstream ua;
	// User agent header prefix
	ua << "User-Agent: Mozilla/5.0 ";
	// OBS version info
	ua << "(OBS-Studio/" << obs_get_version_string() << "; ";
	// Operating system version info
	ua << OS_NAME << "; " << obs_get_locale() << ")";

	return ua.str();
}
