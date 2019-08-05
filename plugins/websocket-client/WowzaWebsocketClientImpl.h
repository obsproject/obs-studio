#include "WebsocketClient.h"

// Use http://think-async.com/ insted of boost
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_

#include "websocketpp/client.hpp"
#include "websocketpp/config/asio_client.hpp"

#include <string>

typedef websocketpp::client<websocketpp::config::asio_tls_client> Client;

class WowzaWebsocketClientImpl : public WebsocketClient {
public:
	WowzaWebsocketClientImpl();
	~WowzaWebsocketClientImpl();

	// WebsocketClient::Listener implementation
	bool connect(const std::string &url, const std::string &appName,
		     const std::string &streamName,
		     const std::string & /* token */,
		     WebsocketClient::Listener *listener) override;
	bool open(const std::string &sdp, const std::string & /* codec */,
		  const std::string &streamName) override;
	bool trickle(const std::string & /* mid */, int /* index */,
		     const std::string &candidate, bool /* last */) override;
	bool disconnect(bool wait) override;

private:
	std::string appName;
	std::string streamName;
	long long session_id;

	Client client;
	Client::connection_ptr connection;
	std::thread thread;

	std::string sanitizeString(const std::string &s);
};
