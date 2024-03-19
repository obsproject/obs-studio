#include "log-redaction.hpp"

using namespace std;

string redact(const char *source)
{
	string login_replaced, mac_user_replaced, lin_user_replaced,
		win_user_replaced, rtmps_replaced, srt_replaced,
		connect_replaced;

	login_replaced =
		regex_replace(source, login_url_regex, "//user:pass@$1");
	mac_user_replaced =
		regex_replace(login_replaced, macos_user_regex, "/Users/user/");
	lin_user_replaced = regex_replace(mac_user_replaced, linux_user_regex,
					  "/home/user/");
	win_user_replaced = regex_replace(lin_user_replaced, win_user_regex,
					  "C:\\Users\\User\\");
	rtmps_replaced = regex_replace(win_user_replaced, rtmps_regex, "$1...");
	srt_replaced = regex_replace(rtmps_replaced, srt_regex,
				     "$1redacted_parameters");
	connect_replaced =
		regex_replace(srt_replaced, connect_addr_regex,
			      "Possible connect address: [redacted]");

	return connect_replaced;
}
