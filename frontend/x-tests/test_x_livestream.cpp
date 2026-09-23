#include <cstdio>
#include <fstream>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

class QWidget;

#include "oauth/Auth.hpp"
#include "oauth/AuthListener.hpp"
#include "oauth/XAuth.hpp"
#include "utility/CurlPublicRedirect.hpp"
#include "utility/XApiWrappers.hpp"
#include "utility/XBroadcastLogic.hpp"

#ifndef OBS_SOURCE_DIR
#define OBS_SOURCE_DIR "."
#endif

namespace {

int g_failures = 0;

void Check(bool ok, const char *file, int line, const char *expr)
{
	if (ok) {
		return;
	}
	std::fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expr);
	g_failures++;
}

#define CHECK(expr) Check(static_cast<bool>(expr), __FILE__, __LINE__, #expr)

std::string ReadFile(const std::string &path)
{
	std::ifstream in(path);
	std::ostringstream buffer;
	buffer << in.rdbuf();
	return buffer.str();
}

json11::Json Parse(const std::string &text)
{
	std::string error;
	json11::Json json = json11::Json::parse(text, error);
	CHECK(error.empty());
	return json;
}

const char *kSourceObject = R"({
  "source": {
    "id": "6ep48v6ar5q4",
    "owner_id": 172483972,
    "name": "My Primary Encoder",
    "rtmp_region": "eu-central-1",
    "rtmp_url": "rtmp://de.pscp.tv:80/x",
    "rtmps_url": "rtmps://de.pscp.tv:443/x",
    "rtmp_stream_key": "6ep48v6ar5q4",
    "is_stream_active": false
  }
})";

void TestParse()
{
	const XStreamSource source = XParseSource(Parse(kSourceObject));
	CHECK(source.id == "6ep48v6ar5q4");
	CHECK(source.name == "My Primary Encoder");
	CHECK(source.region == "eu-central-1");
	CHECK(source.rtmpUrl == "rtmp://de.pscp.tv:80/x");
	CHECK(source.rtmpsUrl == "rtmps://de.pscp.tv:443/x");
	CHECK(source.streamKey == source.id);
	CHECK(source.streamActive == false);
	CHECK(XPreferredIngest(source) == "rtmps://de.pscp.tv:443/x");

	XStreamSource rtmpOnly = source;
	rtmpOnly.rtmpsUrl.clear();
	CHECK(XPreferredIngest(rtmpOnly) == "rtmp://de.pscp.tv:80/x");

	QVector<XStreamSource> wrapped;
	XCollectSourceResponse(Parse(kSourceObject), wrapped);
	CHECK(wrapped.size() == 1);

	const std::string sources = std::string("{\"sources\":[") + kSourceObject + "]}";
	QVector<XStreamSource> fromSources;
	XCollectSourceResponse(Parse(sources), fromSources);
	CHECK(fromSources.size() == 1);
	CHECK(fromSources[0].id == "6ep48v6ar5q4");

	const std::string data = std::string("{\"data\":[") + kSourceObject + "]}";
	QVector<XStreamSource> fromData;
	XCollectSourceResponse(Parse(data), fromData);
	CHECK(fromData.size() == 1);

	const std::string nested = std::string("{\"data\":") + kSourceObject + "}";
	QVector<XStreamSource> fromNested;
	XCollectSourceResponse(Parse(nested), fromNested);
	CHECK(fromNested.size() == 1);
	CHECK(fromNested[0].region == "eu-central-1");
}

void TestServiceMatches()
{
	static_assert(AuthListener::EphemeralPort == 0, "YouTube keeps an ephemeral redirect port");
	static_assert(XOAuthRedirectPort == 42813, "X redirect is fixed by the app registration");

	CHECK(Auth::ServiceMatches("X", "X"));
	CHECK(!Auth::ServiceMatches("XLoveCam.com", "X"));
	CHECK(!Auth::ServiceMatches("XLive", "X"));
	CHECK(Auth::ServiceMatches("Restream.io", "Restream"));
	CHECK(Auth::ServiceMatches("Restream", "Restream"));
	CHECK(IsXService("X"));
	CHECK(!IsXService("XLoveCam.com"));
	CHECK(!IsXService("Twitter (Legacy)"));
}

void TestBodies()
{
	const json11::Json publish = Parse(XPublishStateBody("Live from the studio", true, 2));
	CHECK(publish["state"].string_value() == "PUBLISH");
	CHECK(publish["title"].string_value() == "Live from the studio");
	CHECK(publish["should_not_tweet"].bool_value() == true);
	CHECK(publish["chat_option"].int_value() == 2);
	CHECK(publish.object_items().size() == 4);

	const char *end = XEndStateBody();
	CHECK(std::string(end) == "{\"state\":\"END\"}");
	const json11::Json ended = Parse(end);
	CHECK(ended["state"].string_value() == "END");
	CHECK(ended.object_items().size() == 1);
	CHECK(!ended["title"].is_string());
	CHECK(!ended["chat_option"].is_number());
	CHECK(!ended["should_not_tweet"].is_bool());
}

void TestEnsureSource()
{
	XStreamSource remembered;
	remembered.id = "saved";
	remembered.region = "us-east-1";
	remembered.rtmpsUrl = "rtmps://va.pscp.tv:443/x";
	remembered.streamKey = "saved";

	XStreamSource listed;
	listed.id = "listed";
	listed.region = "eu-central-1";
	listed.rtmpsUrl = "rtmps://de.pscp.tv:443/x";
	listed.streamKey = "listed";

	int regionCalls = 0;
	int getCalls = 0;
	int listCalls = 0;
	int createCalls = 0;

	auto run = [&](const QString &savedId, const QString &recommended, bool savedMatches) {
		regionCalls = getCalls = listCalls = createCalls = 0;
		XStreamSource chosen;
		QString region;
		return XRunEnsureSource(
			savedId,
			[&](QString &out) {
				regionCalls++;
				out = recommended;
				return true;
			},
			[&](const QString &, XStreamSource &out) {
				getCalls++;
				if (!savedMatches) {
					return false;
				}
				out = remembered;
				out.region = recommended;
				return true;
			},
			[&](QVector<XStreamSource> &out) {
				listCalls++;
				if (listed.region == recommended) {
					out.push_back(listed);
				}
				return true;
			},
			[&](const QString &regionName, XStreamSource &out) {
				createCalls++;
				out.id = "created";
				out.region = regionName;
				out.rtmpsUrl = "rtmps://de.pscp.tv:443/x";
				out.streamKey = "created";
				return true;
			},
			chosen, region);
	};

	XStreamSource chosen;
	QString region;
	const XEnsureResult created = XRunEnsureSource(
		QString(),
		[&](QString &out) {
			regionCalls++;
			out = "us-east-1";
			return true;
		},
		[&](const QString &, XStreamSource &) {
			getCalls++;
			return false;
		},
		[&](QVector<XStreamSource> &) {
			listCalls++;
			return true;
		},
		[&](const QString &regionName, XStreamSource &out) {
			createCalls++;
			out.id = "created";
			out.region = regionName;
			out.rtmpUrl = "rtmp://va.pscp.tv:80/x";
			out.rtmpsUrl = "rtmps://va.pscp.tv:443/x";
			out.streamKey = "created";
			return true;
		},
		chosen, region);
	CHECK(created == XEnsureResult::Created);
	CHECK(regionCalls == 1);
	CHECK(getCalls == 0);
	CHECK(listCalls == 1);
	CHECK(createCalls == 1);
	CHECK(chosen.id == "created");
	CHECK(chosen.region == "us-east-1");
	CHECK(XPreferredIngest(chosen).startsWith("rtmps://"));

	regionCalls = getCalls = listCalls = createCalls = 0;
	const XEnsureResult reused = run("saved", "us-east-1", true);
	CHECK(reused == XEnsureResult::ReusedSaved);
	CHECK(getCalls == 1);
	CHECK(listCalls == 0);
	CHECK(createCalls == 0);

	const XEnsureResult fromList = run("saved", "eu-central-1", false);
	CHECK(fromList == XEnsureResult::ReusedListed);
	CHECK(createCalls == 0);
	CHECK(listCalls == 1);
}

void TestGoLiveAndEnd()
{
	int creates = 0;
	int publishes = 0;
	QString id;
	const XGoLiveResult inactive = XRunGoLive([]() { return false; },
						  [&](QString &) {
							  creates++;
							  return true;
						  },
						  [&](const QString &) {
							  publishes++;
							  return true;
						  },
						  id);
	CHECK(inactive == XGoLiveResult::FailedBeforeCreate);
	CHECK(creates == 0);
	CHECK(publishes == 0);
	CHECK(!XShouldEndBroadcast(false, false));

	const XGoLiveResult publishFailed = XRunGoLive([]() { return true; },
						       [&](QString &created) {
							       creates++;
							       created = "broadcast";
							       return true;
						       },
						       [&](const QString &) {
							       publishes++;
							       return false;
						       },
						       id);
	CHECK(publishFailed == XGoLiveResult::FailedPublish);
	CHECK(creates == 1);
	CHECK(publishes == 1);
	CHECK(id == "broadcast");
	CHECK(!XShouldEndBroadcast(false, true));
	CHECK(XStopOutputAfterPublishFailure(false));
	CHECK(!XStopOutputAfterPublishFailure(true));

	const XGoLiveResult published = XRunGoLive([]() { return true; },
						   [&](QString &created) {
							   created = "live";
							   return true;
						   },
						   [&](const QString &) { return true; }, id);
	CHECK(published == XGoLiveResult::Published);
	CHECK(XShouldEndBroadcast(true, true));
	CHECK(!XShouldEndBroadcast(true, false));
}

void TestStartKeyGuard()
{
	CHECK(XPlanStartKey(true, false) == XStartKeyAction::Allow);
	CHECK(!XStartBlockedAfterApply(XStartKeyAction::Allow, false));
	CHECK(XPlanStartKey(false, true) == XStartKeyAction::ApplyThenRecheck);
	CHECK(!XStartBlockedAfterApply(XStartKeyAction::ApplyThenRecheck, true));
	CHECK(XStartBlockedAfterApply(XStartKeyAction::ApplyThenRecheck, false));
	CHECK(XPlanStartKey(false, false) == XStartKeyAction::Block);
	CHECK(XStartBlockedAfterApply(XStartKeyAction::Block, false));
}

void TestAccessDeniedCopy()
{
	CHECK(XIsAccessDenied(401));
	CHECK(XIsAccessDenied(403));
	CHECK(!XIsAccessDenied(400));
	CHECK(!XIsAccessDenied(404));
	const std::string ini = ReadFile(std::string(OBS_SOURCE_DIR) + "/frontend/data/locale/en-US.ini");
	const std::string key = "X.Settings.AccessDenied=\"";
	const size_t start = ini.find(key);
	CHECK(start != std::string::npos);
	if (start == std::string::npos) {
		return;
	}
	const size_t value = start + key.size();
	const size_t end = ini.find('"', value);
	const std::string text = ini.substr(value, end - value);
	CHECK(text.find("https://docs.x.com/livestream-api/getting-started") != std::string::npos);
	CHECK(text.find("https://docs.x.com/forms/livestream-api-access") != std::string::npos);
}

void TestServicesJson()
{
	const std::string root = OBS_SOURCE_DIR;
	const json11::Json services = Parse(ReadFile(root + "/plugins/rtmp-services/data/services.json"));
	const json11::Json package = Parse(ReadFile(root + "/plugins/rtmp-services/data/package.json"));
	CHECK(package["version"].int_value() >= 293);
	CHECK(package["files"][0]["version"].int_value() == package["version"].int_value());

	bool sawX = false;
	bool sawLegacy = false;
	for (const json11::Json &service : services["services"].array_items()) {
		const std::string name = service["name"].string_value();
		if (name == "X") {
			sawX = true;
			CHECK(service["common"].bool_value());
			CHECK(service["stream_key_link"].string_value() == "https://x.com/i/live-studio");
			int rtmps = 0;
			bool firstIsRtmps = false;
			const auto &servers = service["servers"].array_items();
			CHECK(!servers.empty());
			if (!servers.empty()) {
				firstIsRtmps = servers[0]["url"].string_value().rfind("rtmps://", 0) == 0;
			}
			CHECK(firstIsRtmps);
			for (const json11::Json &server : servers) {
				const std::string url = server["url"].string_value();
				if (url.rfind("rtmps://", 0) == 0 && url.find(":443/x") != std::string::npos) {
					rtmps++;
				}
			}
			CHECK(rtmps >= 1);
		}
		if (name == "Twitter (Legacy)") {
			sawLegacy = true;
			CHECK(!service["common"].bool_value());
			bool hasTwitter = false;
			for (const json11::Json &alt : service["alt_names"].array_items()) {
				if (alt.string_value() == "Twitter") {
					hasTwitter = true;
				}
			}
			CHECK(hasTwitter);
			bool keptServer = false;
			for (const json11::Json &server : service["servers"].array_items()) {
				if (server["url"].string_value() == "rtmp://ca.pscp.tv:80/x") {
					keptServer = true;
				}
			}
			CHECK(keptServer);
		}
	}
	CHECK(sawX);
	CHECK(sawLegacy);
}

struct OneShotServer {
	int listenFd = -1;
	uint16_t port = 0;
	std::string seen;
	std::string response;
	std::thread worker;

	bool Listen()
	{
		listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
		if (listenFd < 0) {
			return false;
		}
		const int yes = 1;
		::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = 0;
		if (::bind(listenFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0) {
			return false;
		}
		socklen_t len = sizeof(addr);
		if (::getsockname(listenFd, reinterpret_cast<sockaddr *>(&addr), &len) != 0) {
			return false;
		}
		port = ntohs(addr.sin_port);
		return ::listen(listenFd, 1) == 0;
	}

	void Serve()
	{
		worker = std::thread([this]() {
			const int client = ::accept(listenFd, nullptr, nullptr);
			if (client < 0) {
				return;
			}
			std::string request;
			char buffer[1024];
			while (request.find("\r\n\r\n") == std::string::npos) {
				const ssize_t count = ::recv(client, buffer, sizeof(buffer), 0);
				if (count <= 0) {
					break;
				}
				request.append(buffer, buffer + count);
				if (request.size() > 65536) {
					break;
				}
			}
			seen = std::move(request);
			if (!response.empty()) {
				::send(client, response.data(), response.size(), 0);
			}
			::close(client);
		});
	}

	~OneShotServer()
	{
		if (listenFd >= 0) {
			::shutdown(listenFd, SHUT_RDWR);
			::close(listenFd);
			listenFd = -1;
		}
		if (worker.joinable()) {
			worker.join();
		}
	}
};

void TestRegionRedirect()
{
	OneShotServer api;
	OneShotServer region;
	CHECK(api.Listen());
	CHECK(region.Listen());
	const std::string body = "{\"region\":\"us-east-1\"}";
	region.response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " +
			  std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
	api.response = "HTTP/1.1 307 Temporary Redirect\r\nLocation: http://localhost:" + std::to_string(region.port) +
		       "/region\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
	api.Serve();
	region.Serve();

	const std::vector<std::string> headers = {"Authorization: Bearer secret-token", "Accept: application/json"};
	const std::string url = "http://127.0.0.1:" + std::to_string(api.port) + "/2/region";
	const ObsPublicGetResult got = obs_curl_get_follow_public(url.c_str(), headers, 5);
	if (!got.transportOk) {
		std::fprintf(stderr, "redirect transport error: %s\n", got.error.c_str());
	}
	CHECK(got.transportOk);
	CHECK(got.status == 200);
	CHECK(got.body == body);
	CHECK(got.droppedAuthorization);
	CHECK(api.seen.find("Authorization: Bearer secret-token") != std::string::npos);
	CHECK(region.seen.find("Authorization:") == std::string::npos);
	const json11::Json json = Parse(got.body);
	CHECK(json["region"].string_value() == "us-east-1");
}

} // namespace

int main()
{
	TestParse();
	TestServiceMatches();
	TestBodies();
	TestEnsureSource();
	TestGoLiveAndEnd();
	TestStartKeyGuard();
	TestAccessDeniedCopy();
	TestServicesJson();
	TestRegionRedirect();
	if (g_failures != 0) {
		std::fprintf(stderr, "%d check(s) failed\n", g_failures);
		return 1;
	}
	std::puts("obs-x-livestream-tests passed");
	return 0;
}
