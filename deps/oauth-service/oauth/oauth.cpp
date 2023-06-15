// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "oauth.hpp"

#include <curl-helper.h>

RequestError::RequestError(const RequestErrorType &type,
			   const std::string &error)
	: type(type), error(error)
{
	switch (type) {
	case RequestErrorType::UNKNOWN_OR_CUSTOM:
		break;
	case RequestErrorType::CURL_REQUEST_FAILED:
		message = "Failed Curl request";
		break;
	case RequestErrorType::JSON_PARSING_FAILED:
		message = "Failed to parse JSON response";
		break;
	case RequestErrorType::OAUTH_REQUEST_FAILED:
		message = "Failed OAuth request";
		break;
	case RequestErrorType::ERROR_JSON_PARSING_FAILED:
		message = "Failed to parse JSON error response";
		break;
	case RequestErrorType::UNMANAGED_HTTP_RESPONSE_CODE:
		message = "Request returned an unexpected HTTP reponse";
		break;
	}
}

namespace OAuth {

static std::string
AccessTokenErrorToQuotedStdString(const AccessTokenError &error)
{
	switch (error) {
	case AccessTokenError::INVALID_REQUEST:
		return "\"invalid_request\"";
	case AccessTokenError::INVALID_CLIENT:
		return "\"invalid_client\"";
	case AccessTokenError::INVALID_GRANT:
		return "\"invalid_grant\"";
	case AccessTokenError::UNAUTHORIZED_CLIENT:
		return "\"unauthorized_client\"";
	case AccessTokenError::UNSUPPORTED_GRANT_TYPE:
		return "\"unsupported_grant_type\"";
	case AccessTokenError::INVALID_SCOPE:
		return "\"invalid_scope\"";
	}

	/* NOTE: It will never go there but GCC and MSVC do not see it this way */
	return {};
}

static bool AccessTokenRequestInternal(AccessTokenResponse &response,
				       RequestError &error,
				       const std::string &userAgent,
				       const std::string &tokenUrl,
				       const std::string &postData)
{
	std::string output;
	std::string errorStr;
	long responseCode = 0;

	CURLcode curlCode = GetRemoteFile(
		userAgent.c_str(), tokenUrl.c_str(), output, errorStr,
		&responseCode, "application/x-www-form-urlencoded", "",
		postData.c_str(), std::vector<std::string>(), nullptr, 5);

	if (curlCode != CURLE_OK && curlCode != CURLE_HTTP_RETURNED_ERROR) {
		error = RequestError(RequestErrorType::CURL_REQUEST_FAILED,
				     errorStr);
		return false;
	}

	switch (responseCode) {
	case 200:
		try {
			response = nlohmann::json::parse(output);
			return true;
		} catch (nlohmann::json::exception &e) {
			error = RequestError(
				RequestErrorType::JSON_PARSING_FAILED,
				e.what());
		}
		break;
	case 400:
		try {
			AccessTokenErrorResponse errorResponse =
				nlohmann::json::parse(output);
			error = RequestError(
				RequestErrorType::OAUTH_REQUEST_FAILED,
				errorResponse.errorDescription.value_or(
					AccessTokenErrorToQuotedStdString(
						errorResponse.error)));
		} catch (nlohmann::json::exception &e) {
			error = RequestError(
				RequestErrorType::ERROR_JSON_PARSING_FAILED,
				e.what());
		}
		break;
	default:
		error = RequestError(
			RequestErrorType::UNMANAGED_HTTP_RESPONSE_CODE,
			errorStr);
		break;
	}

	return false;
}

bool AccessTokenRequest(AccessTokenResponse &response, RequestError &error,
			const std::string &userAgent,
			const std::string &tokenUrl, const std::string &code,
			const std::string &redirectUri,
			const std::string &clientId,
			const std::string &clientSecret)
{
	std::string postData = "action=redirect&client_id=";
	postData += clientId;
	/* NOTE: In RFC 6749, Authorization Code Grant do not list client_secret in its request format */
	if (!clientSecret.empty()) {
		postData += "&client_secret=";
		postData += clientSecret;
	}
	if (!redirectUri.empty()) {
		postData += "&redirect_uri=";
		postData += redirectUri;
	}
	postData += "&grant_type=authorization_code&code=";
	postData += code;

	return AccessTokenRequestInternal(response, error, userAgent, tokenUrl,
					  postData);
}

bool RefreshAccessToken(AccessTokenResponse &response, RequestError &error,
			const std::string &userAgent,
			const std::string &tokenUrl,
			const std::string &refreshToken,
			const std::string &clientId,
			const std::string &clientSecret)
{
	std::string postData = "action=redirect&client_id=";
	postData += clientId;
	/* NOTE: In RFC 6749, Authorization Code Grant do not list client_secret in its request format */
	if (!clientSecret.empty()) {
		postData += "&client_secret=";
		postData += clientSecret;
	}
	postData += "&grant_type=refresh_token&refresh_token=";
	postData += refreshToken;

	return AccessTokenRequestInternal(response, error, userAgent, tokenUrl,
					  postData);
}

}

/***************************************************/

static auto curl_deleter = [](CURL *curl) { curl_easy_cleanup(curl); };
using Curl = std::unique_ptr<CURL, decltype(curl_deleter)>;

static size_t string_write(char *ptr, size_t size, size_t nmemb,
			   std::string &str)
{
	size_t total = size * nmemb;
	if (total)
		str.append(ptr, total);

	return total;
}

static size_t header_write(char *ptr, size_t size, size_t nmemb,
			   std::vector<std::string> &list)
{
	std::string str;

	size_t total = size * nmemb;
	if (total)
		str.append(ptr, total);

	if (str.back() == '\n')
		str.resize(str.size() - 1);
	if (str.back() == '\r')
		str.resize(str.size() - 1);

	list.push_back(std::move(str));
	return total;
}

CURLcode GetRemoteFile(const char *userAgent_, const char *url,
		       std::string &str, std::string &error, long *responseCode,
		       const char *contentType, std::string request_type,
		       const char *postData,
		       std::vector<std::string> extraHeaders,
		       std::string *signature, int timeoutSec, int postDataSize)
{
	std::vector<std::string> header_in_list;
	char error_in[CURL_ERROR_SIZE];

	error_in[0] = 0;

	std::string userAgent("User-Agent: ");
	userAgent += userAgent_;

	std::string contentTypeString;
	if (contentType) {
		contentTypeString += "Content-Type: ";
		contentTypeString += contentType;
	}

	Curl curl{curl_easy_init(), curl_deleter};
	if (!curl)
		return CURLE_FAILED_INIT;

	CURLcode code = CURLE_FAILED_INIT;
	struct curl_slist *header = nullptr;

	header = curl_slist_append(header, userAgent.c_str());

	if (!contentTypeString.empty()) {
		header = curl_slist_append(header, contentTypeString.c_str());
	}

	for (std::string &h : extraHeaders)
		header = curl_slist_append(header, h.c_str());

	curl_easy_setopt(curl.get(), CURLOPT_URL, url);
	curl_easy_setopt(curl.get(), CURLOPT_ACCEPT_ENCODING, "");
	curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, header);
	curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_in);
	curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, string_write);
	curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &str);
	curl_obs_set_revoke_setting(curl.get());

	if (signature) {
		curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION,
				 header_write);
		curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA,
				 &header_in_list);
	}

	if (timeoutSec)
		curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, timeoutSec);

	if (!request_type.empty()) {
		if (request_type != "GET")
			curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST,
					 request_type.c_str());

		// Special case of "POST"
		if (request_type == "POST") {
			curl_easy_setopt(curl.get(), CURLOPT_POST, 1);
			if (!postData)
				curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS,
						 "{}");
		}
	}
	if (postData) {
		if (postDataSize > 0) {
			curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE,
					 (long)postDataSize);
		}
		curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, postData);
	}

	code = curl_easy_perform(curl.get());
	if (responseCode)
		curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE,
				  responseCode);

	if (code != CURLE_OK) {
		error = strlen(error_in) ? error_in : curl_easy_strerror(code);
	} else if (signature) {
		for (std::string &h : header_in_list) {
			std::string name = h.substr(0, 13);
			// HTTP headers are technically case-insensitive
			if (name == "X-Signature: " ||
			    name == "x-signature: ") {
				*signature = h.substr(13);
				break;
			}
		}
	}

	curl_slist_free_all(header);

	return code;
}
