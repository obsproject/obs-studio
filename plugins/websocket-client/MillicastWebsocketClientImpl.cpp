#include "MillicastWebsocketClientImpl.h"
#include "nlohmann/json.hpp"
#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"

#include <util/base.h>

#include <string>
/* clang-format off */

#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR, format, ##__VA_ARGS__)

using json = nlohmann::json;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

MillicastWebsocketClientImpl::MillicastWebsocketClientImpl()
{
	// Set logging to be pretty verbose (everything except message payloads)
	client.set_access_channels(websocketpp::log::alevel::all);
	client.clear_access_channels(websocketpp::log::alevel::frame_payload);
	client.set_error_channels(websocketpp::log::elevel::all);
	// Initialize ASIO
	client.init_asio();
}

MillicastWebsocketClientImpl::~MillicastWebsocketClientImpl()
{
	disconnect(false);
}

bool MillicastWebsocketClientImpl::connect(
	const std::string & /* publish_api_url */,
	const std::string & /* room */, const std::string &stream_name,
	const std::string &token, WebsocketClient::Listener *listener)
{
	this->token = sanitizeString(token);

	RestClient::init();
	RestClient::Connection *conn = new RestClient::Connection("");
	RestClient::HeaderFields headers;
	headers["Authorization"] = "Bearer " + this->token;
	headers["Content-Type"] = "application/json";
	conn->SetHeaders(headers);
	conn->SetTimeout(5);
	const std::string publish_api_url =
		"https://director.millicast.com/api/director/publish";
	json data = {{"streamName", sanitizeString(stream_name)}};
	RestClient::Response r = conn->post(publish_api_url, data.dump());
	delete conn;
	RestClient::disable();

	std::string url;
	std::string jwt;
	if (r.code == 200) {
		auto wssData = json::parse(r.body);
		url = wssData["data"]["urls"][0];
		jwt = wssData["data"]["jwt"];
		info("WSS url:          %s", url.c_str());
		info("JWT (token):      %s", jwt.c_str());
	} else {
		error("Error querying publishing websocket url");
		error("code: %d", r.code);
		error("body: %s", r.body.c_str());
		return false;
	}

	websocketpp::lib::error_code ec;
	try {
		// --- TLS handler
		client.set_tls_init_handler([&](websocketpp::connection_hdl /* con */) {
			// Create context
			auto ctx =
				websocketpp::lib::make_shared<asio::ssl::context>(
					asio::ssl::context::tlsv12_client);
			try {
				// Remove support for undesired TLS versions
				ctx->set_options(
					asio::ssl::context::default_workarounds |
					asio::ssl::context::no_sslv2 |
					asio::ssl::context::no_sslv3 |
					asio::ssl::context::single_dh_use);
			} catch (std::exception &e) {
				warn("TLS exception: %s", e.what());
			}
			return ctx;
		});

		// Create websocket url
		std::string wss = url + "?token=" + jwt;
		info("Connection URL:   %s", wss.c_str());

		connection = client.get_connection(wss, ec);
		if (!connection)
			warn("No Connection");
		connection->set_close_handshake_timeout(5000);
		if (ec) {
			error("Error establishing websocket connection: %s",
			      ec.message().c_str());
			return 0;
		}

		// --- Message handler
		connection->set_message_handler([=](websocketpp::connection_hdl /* con */,
						    message_ptr frame) {
			const char *x = frame->get_payload().c_str();
			info("MESSAGE RECEIVED:\n%s\n", x);
			auto msg = json::parse(frame->get_payload());

			if (msg.find("type") == msg.end())
				return;

			std::string type = msg["type"].get<std::string>();
			if (type.compare("response") == 0) {
				if (msg.find("transId") == msg.end())
					return;
				if (msg.find("data") == msg.end())
					return;

				auto data = msg["data"];

				int feedId = 0;
				if (data.find("feedId") != data.end())
					feedId = data["feedId"].get<int>();

				std::string sdp = "";
				if (data.find("sdp") != data.end()) {
					sdp = data["sdp"].get<std::string>() +
					      std::string("a=x-google-flag:conference\r\n");
					// Event
					listener->onOpened(sdp);
				}
			} else if (type.compare("error") == 0) {
				listener->onDisconnected();
			}
		});

		// --- Open handler
		connection->set_open_handler([=](websocketpp::connection_hdl /* con */) {
			// Launch event
			listener->onConnected();
			// Launch logged event
			listener->onLogged(0);
		});

		// --- Close handler
		connection->set_close_handler([=](...) {
			info("> set_close_handler called");
			// Don't wait for connection close
			thread.detach();
			// Remove connection
			connection = nullptr;
			// Call listener
			listener->onDisconnected();
		});

		// -- Failure handler
		connection->set_fail_handler([=](...) {
			info("> set_fail_handler called");
			listener->onDisconnected();
		});

		// -- HTTP handler
		connection->set_http_handler([=](...) {
			info("> https called");
		});

		// Note that connect here only requests a connection. No network messages
		// exchanged until the event loop starts running in the next line.
		client.connect(connection);
		// Async
		thread = std::thread([&]() {
			// Start ASIO io_service run loop
			// (single connection will be made to the server)
			client.run(); // will exit when this connection is closed
		});
	} catch (const websocketpp::exception &e) {
		warn("connect exception: %s", e.what());
		return false;
	}
	return true;
}

bool MillicastWebsocketClientImpl::open(const std::string &sdp,
					const std::string &video_codec,
					const std::string &stream_name)
{
	info("WS-OPEN: stream_name: %s", stream_name.c_str());
	try {
		json data_without_codec = {
			{ "name", sanitizeString(stream_name) },
			{ "streamId", sanitizeString(stream_name) },
			{ "sdp" , sdp }
		};
		json data_with_codec = {
			{ "name", sanitizeString(stream_name) },
			{ "streamId", sanitizeString(stream_name) },
			{ "sdp" , sdp },
			{ "codec", video_codec }
		};
		// Publish command (send offer)
		json open = {
			{ "type", "cmd" },
			{ "name", "publish" },
			{ "transId", rand() },
			{ "data", video_codec.empty() ? data_without_codec : data_with_codec }
		};
		// Serialize and send
		if (connection->send(open.dump())) {
			warn("Error sending open message");
			return false;
		}
	} catch (const websocketpp::exception &e) {
		warn("open exception: %s", e.what());
		return false;
	}
	return true;
}

bool MillicastWebsocketClientImpl::trickle(const std::string & /* mid */,
					   int /* index */,
					   const std::string & /* candidate */,
					   bool /* last */)
{
	return true;
}

bool MillicastWebsocketClientImpl::disconnect(bool /* wait */)
{
	if (!connection)
		return true;
	websocketpp::lib::error_code ec;
	try {
		json close = {
			{ "type", "cmd" },
			{ "name", "unpublish" }
		};
		// Serialize and send
		if (connection->send(close.dump()))
			return false;
		// Wait for unpublish message to be sent
		std::this_thread::sleep_for(std::chrono::seconds(2));
		// Stop client
		if (connection->get_state() == websocketpp::session::state::open)
			client.close(connection,
				     websocketpp::close::status::normal,
				     std::string("disconnect"), ec);
		if (ec)
			warn("> Error on disconnect close: %s", ec.message().c_str());
		// Don't wait for connection close
		client.stop();
		// Remove handlers
		client.set_open_handler([](...) {});
		client.set_close_handler([](...) {});
		client.set_fail_handler([](...) {});
		// Detach thread
		if (thread.joinable())
			thread.detach();
	} catch (const websocketpp::exception &e) {
		warn("disconnect exception: %s", e.what());
		return false;
	}
	return true;
}

std::string MillicastWebsocketClientImpl::sanitizeString(const std::string &s)
{
	std::string _my_s = s;
	size_t p = _my_s.find_first_not_of(" \n\r\t");
	_my_s.erase(0, p);
	p = _my_s.find_last_not_of(" \n\r\t");
	if (p != std::string::npos)
		_my_s.erase(p + 1);
	return _my_s;
}
