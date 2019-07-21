#include "WowzaWebsocketClientImpl.h"

#include "nlohmann/json.hpp"
#include "restclient-cpp/connection.h"

#include <util/base.h>

#include <iostream>
#include <math.h>
#include <string>
#include <vector>

#define warn(format, ...) blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) blog(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) blog(LOG_DEBUG, format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR, format, ##__VA_ARGS__)

using json = nlohmann::json;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

WowzaWebsocketClientImpl::WowzaWebsocketClientImpl()
{
	debug("WowzaWebsocketClientImpl()");
	// Set logging to be pretty verbose (everything except message payloads)
	client.set_access_channels(websocketpp::log::alevel::all);
	client.clear_access_channels(websocketpp::log::alevel::frame_payload);
	client.set_error_channels(websocketpp::log::elevel::all);
	// Initialize ASIO
	client.init_asio();
}

WowzaWebsocketClientImpl::~WowzaWebsocketClientImpl()
{
	debug("~WowzaWebsocketClientImpl()");
	// Disconnect just in case
	disconnect(false);
}

std::string WowzaWebsocketClientImpl::sanitizeString(const std::string &s)
{
	std::string _my_s = s;
	_my_s.erase(0, _my_s.find_first_not_of(" \n\r\t"));
	_my_s.erase(_my_s.find_last_not_of(" \n\r\t") + 1);
	return _my_s;
}

bool WowzaWebsocketClientImpl::connect(const std::string &url,
				       const std::string &appName,
				       const std::string &streamName,
				       const std::string & /* token */,
				       WebsocketClient::Listener *listener)
{
	debug("WowzaWebsocketClientImpl::connect");
	websocketpp::lib::error_code ec;
	this->appName = sanitizeString(appName);
	this->streamName = sanitizeString(streamName);
	std::string wss = sanitizeString(url);

	info("Server URL: %s", wss.c_str());
	info("Application Name: %s", this->appName.c_str());
	info("Stream Name: %s", this->streamName.c_str());

	try {
		// --- TLS handler
		client.set_tls_init_handler([&](websocketpp::
							connection_hdl /* con */) {
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

		connection = client.get_connection(wss, ec);
		if (ec) {
			error("Error establishing TLS connection: %s",
			      ec.message().c_str());
			return 0;
		}

		// --- Message handler
		connection->set_message_handler([=](websocketpp::
							    connection_hdl /* con */,
						    message_ptr frame) {
			bool candidateFound = false;
			const char *x = frame->get_payload().c_str();
			auto msg = json::parse(frame->get_payload());
			std::string sdp = "";
			json sdpData;

			if (msg.find("status") == msg.end())
				return;

			int status = msg["status"].get<int>();
			if (status == 200) {
				if (msg.find("sdp") != msg.end())
					sdp = msg["sdp"]["sdp"];
				if (msg.find("iceCandidates") != msg.end()) {
					candidateFound = true;
					sdpData = msg["iceCandidates"];
				}
				if (msg.find("streamInfo") != msg.end()) {
					std::string session_id_str =
						msg["streamInfo"]["sessionId"];
					session_id = std::stoll(session_id_str);
					info("Session ID: %s",
					     session_id_str.c_str());
				}

				std::string command = msg["command"];
				if (command.compare("sendOffer") == 0) {
					info("sendOffer response received:\n%s\n",
					     x);
					if (!sdp.empty()) {
						// Event
						listener->onOpened(sdp);
					}
					if (candidateFound) {
						for (const auto &iceCandidate :
						     sdpData) {
							listener->onRemoteIceCandidate(
								iceCandidate["candidate"]
									.dump());
						}
						listener->onRemoteIceCandidate(
							"");
					}
				}
			} else {
				info("RECEIVED MESSAGE:\n%s\n", x);
				if (status > 499) {
					warn("Server returned status code: %d",
					     status);
					listener->onDisconnected();
				}
			}
		});

		// --- Open handler
		connection->set_open_handler(
			[=](websocketpp::connection_hdl /* con */) {
				debug("** set_open_handler called **");
				// Launch event
				listener->onConnected();
				// Launch logged event
				listener->onLogged(0);
			});

		// --- Close handler
		connection->set_close_handler([=](...) {
			debug("** set_close_handler called **");
			listener->onDisconnected();
		});

		// -- Failure handler
		connection->set_fail_handler([=](...) {
			debug("** set_fail_handler called **");
			listener->onDisconnected();
		});

		// -- Interrupt handler
		connection->set_interrupt_handler([=](...) {
			debug("** set_interrupt_handler called **");
			listener->onDisconnected();
		});

		// Request connection (no network messages exchanged until event loop starts running)
		client.connect(connection);
		// Async
		thread = std::thread([&]() {
			debug("** Starting ASIO io_service run loop **");
			// Start ASIO io_service run loop (single connection will be made to the server)
			client.run(); // will exit when this connection is closed
		});
	} catch (const websocketpp::exception &e) {
		warn("WowzaWebsocketClientImpl::connect exception: %s",
		     e.what());
		return false;
	}
	// OK
	return true;
}

bool WowzaWebsocketClientImpl::open(const std::string &sdp,
				    const std::string & /* codec */,
				    const std::string &stream_name)
{
	json offer = {{"direction", "publish"},
		      {"command", "sendOffer"},
		      {"streamInfo",
		       {{"applicationName", appName},
			{"streamName", stream_name},
			{"sessionId", "[empty]"}}},
		      {"sdp", {{"type", "offer"}, {"sdp", sdp}}}};
	info("Sending offer...");
	try {
		// Serialize and send
		if (connection->send(offer.dump())) {
			warn("Error sending offer");
			return false;
		}
	} catch (const websocketpp::exception &e) {
		warn("Error sending offer: %s", e.what());
		return false;
	}
	// OK
	return true;
}

bool WowzaWebsocketClientImpl::trickle(const std::string & /* mid */,
				       int /* index */,
				       const std::string &candidate,
				       bool /* last */)
{
	debug("Trickle candidate: %s", candidate.c_str());
	// OK
	return true;
}

bool WowzaWebsocketClientImpl::disconnect(bool wait)
{
	debug("WowzaWebsocketClientImpl::disconnect");
	if (!connection)
		return true;
	websocketpp::lib::error_code ec;
	try {
		if (connection->get_state() ==
		    websocketpp::session::state::open)
			client.close(connection,
				     websocketpp::close::status::normal,
				     std::string("disconnect"), ec);
		if (ec)
			warn("WowzaWebsocketClientImpl::disconnect close exception: %s",
			     ec.message().c_str());
		client.stop();
		if (wait && thread.joinable()) {
			thread.join();
		} else {
			client.set_open_handler([](...) {});
			client.set_close_handler([](...) {});
			client.set_fail_handler([](...) {});
			if (thread.joinable())
				thread.detach();
		}
	} catch (const websocketpp::exception &e) {
		warn("WowzaWebsocketClientImpl::disconnect exception: %s",
		     e.what());
		return false;
	}
	return true;
}
