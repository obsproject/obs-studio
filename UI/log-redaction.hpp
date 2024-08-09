#pragma once

#include <regex>

const std::regex login_url_regex = std::regex("//[^\\s]+?:[^\\s]+?@([^\\s]+?)",
					      std::regex_constants::icase);
const std::regex macos_user_regex =
	std::regex("/Users/.+?/", std::regex_constants::icase);
const std::regex linux_user_regex =
	std::regex("/home/.+?/", std::regex_constants::icase);
const std::regex win_user_regex = std::regex(
	"[A-Za-z]:([\\\\])Users[\\\\].+?[\\\\]", std::regex_constants::icase);
const std::regex rtmps_regex = std::regex("(rtmps?://[^\\s]+?/)[^\\s]+");
const std::regex srt_regex = std::regex("(srt://.+?\\?).+?(?=$|\\s)");
const std::regex connect_addr_regex =
	std::regex("Possible connect address: .+");

std::string redact(const char *source);
