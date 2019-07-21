#include <obs-module.h>
#include <openssl/opensslv.h>
#include "JanusWebsocketClientImpl.h"
#include "WowzaWebsocketClientImpl.h"
#include "MillicastWebsocketClientImpl.h"
#include "EvercastWebsocketClientImpl.h"

OBS_DECLARE_MODULE()

bool obs_module_load(void)
{
	OPENSSL_init_ssl(0, NULL);
	return true;
}

WEBSOCKETCLIENT_API WebsocketClient *createWebsocketClient(int type)
{
	if (type == Type::Janus)
		return new JanusWebsocketClientImpl();
	if (type == Type::Wowza)
		return new WowzaWebsocketClientImpl();
	if (type == Type::Millicast)
		return new MillicastWebsocketClientImpl();
	if (type == Type::Evercast)
		return new EvercastWebsocketClientImpl();
	return nullptr;
}
