#ifndef _WEBSOCKETCLIENT_H_
#define _WEBSOCKETCLIENT_H_

#ifdef _MSC_VER
#ifdef WEBSOCKETCLIENT_EXPORTS
#define WEBSOCKETCLIENT_API __declspec(dllexport)
#else
#define WEBSOCKETCLIENT_API __declspec(dllimport)
#endif
#else
#ifndef __APPLE__
#define WEBSOCKETCLIENT_API
#else
#define WEBSOCKETCLIENT_API __attribute__((visibility("default")))
#endif
#endif

#include <string>

enum Type { Janus = 0, Wowza = 1, Millicast = 2 };

class WEBSOCKETCLIENT_API WebsocketClient {
public:
	class Listener {
	public:
		virtual ~Listener() = default;
		virtual void onConnected() = 0;
		virtual void onDisconnected() = 0;
		virtual void onLogged(int code) = 0;
		virtual void onLoggedError(int code) = 0;
		virtual void onOpened(const std::string &sdp) = 0;
		virtual void onOpenedError(int code) = 0;
		virtual void
		onRemoteIceCandidate(const std::string &sdpData) = 0;
	};

public:
	virtual ~WebsocketClient() = default;
	virtual bool connect(const std::string &url, const std::string &room,
			     const std::string &username,
			     const std::string &token,
			     WebsocketClient::Listener *listener) = 0;
	virtual bool open(const std::string &sdp, const std::string &codec,
			  const std::string &username) = 0;
	virtual bool trickle(const std::string &mid, int index,
			     const std::string &candidate, bool last) = 0;
	virtual bool disconnect(bool wait) = 0;
};

WEBSOCKETCLIENT_API WebsocketClient *createWebsocketClient(int type);

#endif
