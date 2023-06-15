// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <optional>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

#ifndef NLOHMANN_OPT_HELPER
#define NLOHMANN_OPT_HELPER
namespace nlohmann {
template<typename T> struct adl_serializer<std::optional<T>> {
	static void to_json(json &j, const std::optional<T> &opt)
	{
		if (!opt)
			j = nullptr;
		else
			j = *opt;
	}

	static std::optional<T> from_json(const json &j)
	{
		if (j.is_null())
			return std::make_optional<T>();
		else
			return std::make_optional<T>(j.get<T>());
	}
};
}
#endif

enum class RequestErrorType : int {
	UNKNOWN_OR_CUSTOM,
	CURL_REQUEST_FAILED,
	JSON_PARSING_FAILED,
	OAUTH_REQUEST_FAILED,
	ERROR_JSON_PARSING_FAILED,
	UNMANAGED_HTTP_RESPONSE_CODE,
};

struct RequestError {
	RequestErrorType type = RequestErrorType::UNKNOWN_OR_CUSTOM;
	std::string message;
	std::string error;

	RequestError() {}

	RequestError(const RequestErrorType &type, const std::string &error);

	RequestError(const std::string &message, const std::string &error)
		: message(message), error(error)
	{
	}
};

namespace OAuth {
using nlohmann::json;

#ifndef NLOHMANN_OPTIONAL_OAuth_HELPER
#define NLOHMANN_OPTIONAL_OAuth_HELPER
template<typename T>
inline std::optional<T> get_stack_optional(const json &j, const char *property)
{
	auto it = j.find(property);
	if (it != j.end() && !it->is_null()) {
		return j.at(property).get<std::optional<T>>();
	}
	return std::optional<T>();
}
#endif

/* https://datatracker.ietf.org/doc/html/rfc6749#section-4.1.4
 * https://datatracker.ietf.org/doc/html/rfc6749#section-5.1 */
struct AccessTokenResponse {
	std::string accessToken;
	std::string tokenType;
	std::optional<int64_t> expiresIn;
	std::optional<std::string> refreshToken;
	std::optional<std::string> scope;
};

/* https://datatracker.ietf.org/doc/html/rfc6749#section-5.2 */
enum class AccessTokenError : int {
	INVALID_REQUEST,
	INVALID_CLIENT,
	INVALID_GRANT,
	UNAUTHORIZED_CLIENT,
	UNSUPPORTED_GRANT_TYPE,
	INVALID_SCOPE,
};

/* https://datatracker.ietf.org/doc/html/rfc6749#section-4.1.4
 * https://datatracker.ietf.org/doc/html/rfc6749#section-5.2 */
struct AccessTokenErrorResponse {
	AccessTokenError error;
	std::optional<std::string> errorDescription;
	std::optional<std::string> errorUri;
};

inline void from_json(const json &j, AccessTokenResponse &s)
{
	s.accessToken = j.at("access_token").get<std::string>();
	/* NOTE: token_type is case insensitive, force to lower for easier compare check */
	s.tokenType = j.at("token_type").get<std::string>();
	std::transform(s.tokenType.begin(), s.tokenType.end(),
		       s.tokenType.begin(),
		       [](unsigned char c) { return std::tolower(c); });
	s.expiresIn = get_stack_optional<int64_t>(j, "expires_in");
	s.refreshToken = get_stack_optional<std::string>(j, "refresh_token");
	s.scope = get_stack_optional<std::string>(j, "scope");
}

inline void from_json(const json &j, AccessTokenError &e)
{
	if (j == "invalid_request")
		e = AccessTokenError::INVALID_REQUEST;
	else if (j == "invalid_client")
		e = AccessTokenError::INVALID_CLIENT;
	else if (j == "invalid_grant")
		e = AccessTokenError::INVALID_GRANT;
	else if (j == "unauthorized_client")
		e = AccessTokenError::UNAUTHORIZED_CLIENT;
	else if (j == "unsupported_grant_type")
		e = AccessTokenError::UNSUPPORTED_GRANT_TYPE;
	else if (j == "invalid_scope")
		e = AccessTokenError::INVALID_SCOPE;
	else {
		throw std::runtime_error(
			"Input JSON does not conform to the RFC 6749!");
	}
}

inline void from_json(const json &j, AccessTokenErrorResponse &s)
{
	s.error = j.at("error").get<AccessTokenError>();
	s.errorDescription =
		get_stack_optional<std::string>(j, "error_description");
	s.errorUri = get_stack_optional<std::string>(j, "error_uri");
}

bool AccessTokenRequest(AccessTokenResponse &response, RequestError &error,
			const std::string &userAgent,
			const std::string &tokenUrl, const std::string &code,
			const std::string &redirectUri,
			const std::string &clientId,
			const std::string &clientSecret = {});
bool RefreshAccessToken(AccessTokenResponse &response, RequestError &error,
			const std::string &userAgent,
			const std::string &tokenUrl,
			const std::string &refreshToken,
			const std::string &clientId,
			const std::string &clientSecret = {});

}

CURLcode GetRemoteFile(
	const char *userAgent, const char *url, std::string &str,
	std::string &error, long *responseCode = nullptr,
	const char *contentType = nullptr, std::string request_type = "",
	const char *postData = nullptr,
	std::vector<std::string> extraHeaders = std::vector<std::string>(),
	std::string *signature = nullptr, int timeoutSec = 0,
	int postDataSize = 0);
