#include "EvercastWebsocketClientImpl.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

EvercastWebsocketClientImpl::EvercastWebsocketClientImpl()
{
	// Set logging to be pretty verbose (everything except message payloads)
	client.set_access_channels(websocketpp::log::alevel::all);
	client.clear_access_channels(websocketpp::log::alevel::frame_payload);
	client.set_error_channels(websocketpp::log::elevel::all);
	// Initialize ASIO
	client.init_asio();
}

EvercastWebsocketClientImpl::~EvercastWebsocketClientImpl()
{
	// Disconnect just in case
	disconnect(false);
}

std::string EvercastWebsocketClientImpl::sanitizeString(const std::string &s)
{
	std::string _my_s = s;
	_my_s.erase(0, _my_s.find_first_not_of(" \n\r\t"));
	_my_s.erase(_my_s.find_last_not_of(" \n\r\t") + 1);
	return _my_s;
}

bool EvercastWebsocketClientImpl::connect(const std::string &url,
					  const std::string &room,
					  const std::string &username,
					  const std::string &token,
					  WebsocketClient::Listener *listener)
{
	websocketpp::lib::error_code ec;

	std::string _my_url = sanitizeString(url);
	std::string _my_token = sanitizeString(token);
	std::string _my_room = sanitizeString(room);
	// Reset login flag
	logged = false;
	try {
		// --- Message handler
		client.set_message_handler([=](websocketpp::
						       connection_hdl /* con */,
					       message_ptr frame) {
			// Get response
			auto msg = json::parse(frame->get_payload());
			const char *x = frame->get_payload().c_str();
			std::cout << x << std::endl << std::endl << std::endl;

			if (msg.find("janus") == msg.end())
				return;
			if (msg.find("ack") != msg.end())
				return;

			// Check if it is an event
			if (msg.find("jsep") != msg.end()) {
				std::string sdp = msg["jsep"]["sdp"];
				// Event
				listener->onOpened(sdp);
				return;
			}

			// Check type
			std::string event = msg["janus"];
			if (event.compare("success") == 0) {
				if (msg.find("transaction") == msg.end())
					return;
				if (msg.find("data") == msg.end())
					return;
				// Get the data session
				auto data = msg["data"];
				// Server is sending response twice, ignore second one
				if (!logged) {
					// Get response code
					session_id = data["id"];
					// Create handle command
					json attachPlugin = {
						{"janus", "attach"},
						{"transaction",
						 std::to_string(rand())},
						{"session_id", session_id},
						{"plugin", "janus.plugin.lua"}};
					// Serialize and send
					connection->send(attachPlugin.dump());
					logged = true;
					// Keep the connection alive
					is_running.store(true);
					thread_keepAlive = std::thread([&]() {
						EvercastWebsocketClientImpl::
							keepConnectionAlive();
					});
				} else { // logged
					handle_id = data["id"];
					json joinRoom = {
						{"janus", "message"},
						{"transaction",
						 std::to_string(rand())},
						{"session_id", session_id},
						{"handle_id", handle_id},
						{"body",
						 {{"room", room},
						  {"display", "EBS"},
						  {"ptype", "publisher"},
						  {"request", "join"}}}};
					// Serialize and send
					connection->send(joinRoom.dump());
					// Launch logged event
					listener->onLogged(session_id);
				}
			}
		});

		// --- Open handler
		client.set_open_handler(
			[=](websocketpp::connection_hdl /* con */) {
				// Launch event
				listener->onConnected();
				// Login command
				json login = {{"janus", "create"},
					      {"transaction",
					       std::to_string(rand())},
					      {"payload",
					       {{"username", username},
						{"token", token},
						{"room", room}}}};
				// Serialize and send
				connection->send(login.dump());
			});

		// --- Close hanlder
		client.set_close_handler(
			[=](...) { listener->onDisconnected(); });

		// --- Failure handler
		client.set_fail_handler(
			[=](...) { listener->onDisconnected(); });

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
					asio::ssl::context::single_dh_use);
			} catch (std::exception &e) {
				std::cout << "tls exception: " << e.what()
					  << std::endl;
			}
			return ctx;
		});

		// Create websocket url
		std::string wss = _my_url + "/?token=" + _my_token +
				  "&roomId=" + _my_room;
		// Get connection
		connection = client.get_connection(wss, ec);
		if (ec) {
			std::cout << "could not create connection because: "
				  << ec.message() << std::endl;
			return 0;
		}
		connection->add_subprotocol("janus-protocol");

		// Note that connect here only requests a connection. No network messages are
		// exchanged until the event loop starts running in the next line.
		client.connect(connection);
		// Async
		thread = std::thread([&]() {
			// Start the ASIO io_service run loop
			// this will cause a single connection to be made to the server. c.run()
			// will exit when this connection is closed.
			client.run();
		});
	} catch (websocketpp::exception const &e) {
		std::cout << "connect exception: " << e.what() << std::endl;
		return false;
	}
	// OK
	return true;
}

bool EvercastWebsocketClientImpl::open(const std::string &sdp,
				       const std::string &codec,
				       const std::string & /* Id */)
{
	try {
		json body_no_codec = {{"request", "configure"},
				      {"muted", false},
				      {"video", true},
				      {"audio", true}};
		json body_with_codec = {{"request", "configure"},
					{"videocodec", codec},
					{"muted", false},
					{"video", true},
					{"audio", true}};
		// Send offer
		json open = {
			{"janus", "message"},
			{"session_id", session_id},
			{"handle_id", handle_id},
			{"transaction", std::to_string(rand())},
			{"body",
			 codec.empty() ? body_no_codec : body_with_codec},
			{"jsep",
			 {{"type", "offer"}, {"sdp", sdp}, {"trickle", true}}}};
		// Serialize and send
		if (connection->send(open.dump()))
			return false;
	} catch (const websocketpp::exception &e) {
		std::cout << "open exception: " << e.what() << std::endl;
		return false;
	}
	// OK
	return true;
}

bool EvercastWebsocketClientImpl::trickle(const std::string &mid, int index,
					  const std::string &candidate,
					  bool last)
{
	try {
		// Check if it is last
		if (!last) {
			json trickle = {{"janus", "trickle"},
					{"handle_id", handle_id},
					{"session_id", session_id},
					{"transaction",
					 "trickle" + std::to_string(rand())},
					{"candidate",
					 {{"sdpMid", mid},
					  {"sdpMLineIndex", index},
					  {"candidate", candidate}}}};
			// Serialize and send
			if (connection->send(trickle.dump()))
				return false;
			// OK
			return true;
		} else {
			json trickle = {{"janus", "trickle"},
					{"handle_id", handle_id},
					{"session_id", session_id},
					{"transaction",
					 "trickle" + std::to_string(rand())},
					{"candidate", {{"completed", true}}}};
			// Serialize and send
			if (connection->send(trickle.dump()))
				return false;
		}
	} catch (const websocketpp::exception &e) {
		std::cout << "trickle exception: " << e.what() << std::endl;
		return false;
	}
	// OK
	return true;
}

void EvercastWebsocketClientImpl::keepConnectionAlive()
{
	while (is_running.load()) {
		if (connection) {
			json keepaliveMsg = {
				{"janus", "keepalive"},
				{"session_id", session_id},
				{"transaction",
				 "keepalive-" + std::to_string(rand())}};
			try {
				// Serialize and send
				connection->send(keepaliveMsg.dump());
			} catch (const websocketpp::exception &e) {
				std::cout << "keepConnectionAlive exception: "
					  << e.what() << std::endl;
			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
}

void EvercastWebsocketClientImpl::destroy()
{
	if (connection) {
		json destroyMsg = {{"janus", "destroy"},
				   {"session_id", session_id},
				   {"transaction", std::to_string(rand())}};
		try {
			// Serialize and send
			connection->send(destroyMsg.dump());
		} catch (const websocketpp::exception &e) {
			std::cout << "destroy exception: " << e.what()
				  << std::endl;
		}
	}
}

bool EvercastWebsocketClientImpl::disconnect(bool wait)
{
	destroy();
	if (!connection)
		return true;
	try {
		// Stop keepAlive
		if (thread_keepAlive.joinable()) {
			is_running.store(false);
			thread_keepAlive.join();
		}
		// Stop client
		if (connection->get_state() ==
		    websocketpp::session::state::open)
			client.close(connection,
				     websocketpp::close::status::normal,
				     std::string("disconnect"));
		client.stop();
		// Don't wait for connection close
		if (thread.joinable()) {
			if (wait) {
				thread.join();
			} else {
				// Remove handlers
				client.set_open_handler([](...) {});
				client.set_close_handler([](...) {});
				client.set_fail_handler([](...) {});
				// Detach thread
				thread.detach();
			}
		}
	} catch (const websocketpp::exception &e) {
		std::cout << "disconnect exception: " << e.what() << std::endl;
		return false;
	}
	// OK
	return true;
}
