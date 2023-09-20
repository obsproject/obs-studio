#pragma once

#include <obs.h>

#include <string>
#include <sstream>
#include <random>

static std::string trim_string(const std::string &source)
{
	std::string ret(source);
	ret.erase(0, ret.find_first_not_of(" \n\r\t"));
	ret.erase(ret.find_last_not_of(" \n\r\t") + 1);
	return ret;
}

static size_t curl_writefunction(char *data, size_t size, size_t nmemb,
				 void *priv_data)
{
	auto read_buffer = static_cast<std::string *>(priv_data);

	size_t real_size = size * nmemb;

	read_buffer->append(data, real_size);
	return real_size;
}

#define LOCATION_HEADER_LENGTH 10

static size_t curl_header_location_function(char *data, size_t size,
					    size_t nmemb, void *priv_data)
{
	auto header_buffer = static_cast<std::vector<std::string> *>(priv_data);

	size_t real_size = size * nmemb;

	if (real_size < LOCATION_HEADER_LENGTH)
		return real_size;

	if (!astrcmpi_n(data, "location: ", LOCATION_HEADER_LENGTH)) {
		char *val = data + LOCATION_HEADER_LENGTH;
		auto header_temp =
			std::string(val, real_size - LOCATION_HEADER_LENGTH);

		header_temp = trim_string(header_temp);
		header_buffer->push_back(header_temp);
	}

	return real_size;
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

enum webrtc_network_status : int {
	Success = 0,
	ConnectFailed = 1,
	InvalidHTTPStatusCode = 2,
	NoHTTPData = 3,
	NoLocationHeader = 4,
	FailedToBuildResourceURL = 5,
	InvalidLocationHeader = 6,
	DeleteFailed = 7,
	InvalidAnswer = 8,
	SetRemoteDescriptionFailed = 9,
	Conflict = 10,
};

static inline webrtc_network_status
send_offer(std::string bearer_token, std::string endpoint_url,
	   std::shared_ptr<rtc::PeerConnection> peer_connection,
	   std::string &resource_url, CURLcode *curl_code, char *error_buffer,
	   int &retry_after)
{
	const std::string user_agent = generate_user_agent();

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/sdp");
	if (!bearer_token.empty()) {
		auto bearer_token_header =
			std::string("Authorization: Bearer ") + bearer_token;
		headers =
			curl_slist_append(headers, bearer_token_header.c_str());
	}

	std::string read_buffer;
	std::vector<std::string> location_headers;

	auto offer_sdp =
		std::string(peer_connection->localDescription().value());

	// Add user-agent to our requests
	headers = curl_slist_append(headers, user_agent.c_str());

	CURL *c = curl_easy_init();
	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_writefunction);
	curl_easy_setopt(c, CURLOPT_WRITEDATA, (void *)&read_buffer);
	curl_easy_setopt(c, CURLOPT_HEADERFUNCTION,
			 curl_header_location_function);
	curl_easy_setopt(c, CURLOPT_HEADERDATA, (void *)&location_headers);
	curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(c, CURLOPT_URL, endpoint_url.c_str());
	curl_easy_setopt(c, CURLOPT_POST, 1L);
	curl_easy_setopt(c, CURLOPT_COPYPOSTFIELDS, offer_sdp.c_str());
	curl_easy_setopt(c, CURLOPT_TIMEOUT, 8L);
	curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(c, CURLOPT_UNRESTRICTED_AUTH, 1L);
	curl_easy_setopt(c, CURLOPT_ERRORBUFFER, error_buffer);

	auto cleanup = [&]() {
		curl_easy_cleanup(c);
		curl_slist_free_all(headers);
	};

	*curl_code = curl_easy_perform(c);
	if (*curl_code != CURLE_OK) {
		cleanup();
		return webrtc_network_status::ConnectFailed;
	}

	long response_code;
	curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &response_code);
	if (response_code == 409) {
		struct curl_header *header = nullptr;
		if (curl_easy_header(c, "Retry-After", 0, CURLH_HEADER, -1,
				     &header) == CURLHE_OK) {
			retry_after = atoi(header->value);
		}
		cleanup();
		return webrtc_network_status::Conflict;
	} else if (response_code != 201) {
		cleanup();
		return webrtc_network_status::InvalidHTTPStatusCode;
	}

	if (read_buffer.empty()) {
		cleanup();
		return webrtc_network_status::NoHTTPData;
	}

	long redirect_count = 0;
	curl_easy_getinfo(c, CURLINFO_REDIRECT_COUNT, &redirect_count);

	if (location_headers.size() < static_cast<size_t>(redirect_count) + 1) {
		cleanup();
		return webrtc_network_status::NoLocationHeader;
	}

	CURLU *url_builder = curl_url();
	auto last_location_header = location_headers.back();

	// If Location header doesn't start with `http` it is a relative URL.
	// Construct a absolute URL using the host of the effective URL
	if (last_location_header.find("http") != 0) {
		char *effective_url = nullptr;
		curl_easy_getinfo(c, CURLINFO_EFFECTIVE_URL, &effective_url);
		if (effective_url == nullptr) {
			cleanup();
			return webrtc_network_status::FailedToBuildResourceURL;
		}

		curl_url_set(url_builder, CURLUPART_URL, effective_url, 0);
		curl_url_set(url_builder, CURLUPART_PATH,
			     last_location_header.c_str(), 0);
		curl_url_set(url_builder, CURLUPART_QUERY, "", 0);
	} else {
		curl_url_set(url_builder, CURLUPART_URL,
			     last_location_header.c_str(), 0);
	}

	char *url = nullptr;
	CURLUcode rc = curl_url_get(url_builder, CURLUPART_URL, &url,
				    CURLU_NO_DEFAULT_PORT);
	if (rc) {
		cleanup();
		return webrtc_network_status::InvalidLocationHeader;
	}

	resource_url = url;
	curl_free(url);
	curl_url_cleanup(url_builder);

	auto response = std::string(read_buffer);
	response.erase(0, response.find("v=0"));

	rtc::Description answer(response, "answer");
	try {
		peer_connection->setRemoteDescription(answer);
	} catch (const std::invalid_argument &err) {
		snprintf(error_buffer, CURL_ERROR_SIZE, "%s", err.what());
		return webrtc_network_status::InvalidAnswer;
	} catch (const std::exception &err) {
		snprintf(error_buffer, CURL_ERROR_SIZE, "%s", err.what());
		return webrtc_network_status::SetRemoteDescriptionFailed;
	}
	cleanup();

	return webrtc_network_status::Success;
}

static inline webrtc_network_status send_delete(std::string bearer_token,
						std::string resource_url,
						CURLcode *curl_code,
						char *error_buffer)
{
	const std::string user_agent = generate_user_agent();

	struct curl_slist *headers = NULL;
	if (!bearer_token.empty()) {
		auto bearer_token_header =
			std::string("Authorization: Bearer ") + bearer_token;
		headers =
			curl_slist_append(headers, bearer_token_header.c_str());
	}

	// Add user-agent to our requests
	headers = curl_slist_append(headers, user_agent.c_str());

	CURL *c = curl_easy_init();
	curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(c, CURLOPT_URL, resource_url.c_str());
	curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, "DELETE");
	curl_easy_setopt(c, CURLOPT_TIMEOUT, 8L);
	curl_easy_setopt(c, CURLOPT_ERRORBUFFER, error_buffer);

	auto cleanup = [&]() {
		curl_easy_cleanup(c);
		curl_slist_free_all(headers);
	};

	*curl_code = curl_easy_perform(c);
	if (*curl_code != CURLE_OK) {
		cleanup();
		return webrtc_network_status::DeleteFailed;
	}

	long response_code;
	curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &response_code);
	cleanup();

	if (response_code != 200) {
		return webrtc_network_status::InvalidHTTPStatusCode;
	}
	return webrtc_network_status::Success;
}

static uint32_t generate_random_u32()
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<uint32_t> dist(1, (UINT32_MAX - 1));
	return dist(gen);
}
