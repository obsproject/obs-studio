#include "WebsocketClient.h"

//Use http://think-async.com/ instead of boost
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

class EvercastWebsocketClientImpl : public WebsocketClient {
public:
	EvercastWebsocketClientImpl();
	~EvercastWebsocketClientImpl();

	// WebsocketClient::Listener implementation
	bool connect(const std::string &url, const std::string &room,
		     const std::string &username, const std::string &token,
		     WebsocketClient::Listener *listener) override;
	bool open(const std::string &sdp, const std::string &codec,
		  const std::string & /* Id */) override;
	bool trickle(const std::string &mid, const int index,
		     const std::string &candidate, const bool last) override;
	bool disconnect(const bool wait) override;

	void keepConnectionAlive();
	void destroy();

private:
	bool logged;
	long long session_id;
	long long handle_id;

	std::atomic<bool> is_running;
	std::future<void> handle;
	std::thread thread;
	std::thread thread_keepAlive;

	Client client;
	Client::connection_ptr connection;

	std::string sanitizeString(const std::string &s);
};
