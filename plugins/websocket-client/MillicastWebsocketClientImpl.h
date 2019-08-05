#include "WebsocketClient.h"

// Use http://think-async.com/ insted of boost
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_

#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"

typedef websocketpp::client<websocketpp::config::asio_tls_client> Client;

class MillicastWebsocketClientImpl : public WebsocketClient {
public:
	MillicastWebsocketClientImpl();
	~MillicastWebsocketClientImpl();

	// WebsocketClient::Listener implementation
	bool connect(const std::string & /* publish_api_url */,
		     const std::string & /* room */,
		     const std::string &stream_name, const std::string &token,
		     WebsocketClient::Listener *listener) override;
	bool open(const std::string &sdp, const std::string &video_codec,
		  const std::string &stream_name) override;
	bool trickle(const std::string & /* mid */, int /* index */,
		     const std::string & /* candidate */,
		     bool /* last */) override;
	bool disconnect(bool /* wait */) override;

private:
	std::string token;

	Client client;
	Client::connection_ptr connection;
	std::thread thread;

	std::string sanitizeString(const std::string &s);
};
