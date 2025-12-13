/*
obs-websocket
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
Copyright (C) 2020-2021 Kyle Manning <tt2468@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#pragma once

#include <mutex>
#include <string>
#include <atomic>
#include <memory>

#include "../../eventhandler/types/EventSubscription.h"
#include "plugin-macros.generated.h"

class WebSocketSession;
typedef std::shared_ptr<WebSocketSession> SessionPtr;

class WebSocketSession {
public:
	inline std::string RemoteAddress()
	{
		std::lock_guard<std::mutex> lock(_remoteAddressMutex);
		return _remoteAddress;
	}
	inline void SetRemoteAddress(std::string address)
	{
		std::lock_guard<std::mutex> lock(_remoteAddressMutex);
		_remoteAddress = address;
	}

	inline uint64_t ConnectedAt() { return _connectedAt; }
	inline void SetConnectedAt(uint64_t at) { _connectedAt = at; }

	inline uint64_t IncomingMessages() { return _incomingMessages; }
	inline void IncrementIncomingMessages() { _incomingMessages++; }

	inline uint64_t OutgoingMessages() { return _outgoingMessages; }
	inline void IncrementOutgoingMessages() { _outgoingMessages++; }

	inline uint8_t Encoding() { return _encoding; }
	inline void SetEncoding(uint8_t encoding) { _encoding = encoding; }

	inline bool AuthenticationRequired() { return _authenticationRequired; }
	inline void SetAuthenticationRequired(bool required) { _authenticationRequired = required; }

	inline std::string Secret()
	{
		std::lock_guard<std::mutex> lock(_secretMutex);
		return _secret;
	}
	inline void SetSecret(std::string secret)
	{
		std::lock_guard<std::mutex> lock(_secretMutex);
		_secret = secret;
	}

	inline std::string Challenge()
	{
		std::lock_guard<std::mutex> lock(_challengeMutex);
		return _challenge;
	}
	inline void SetChallenge(std::string challenge)
	{
		std::lock_guard<std::mutex> lock(_challengeMutex);
		_challenge = challenge;
	}

	inline uint8_t RpcVersion() { return _rpcVersion; }
	inline void SetRpcVersion(uint8_t version) { _rpcVersion = version; }

	inline bool IsIdentified() { return _isIdentified; }
	inline void SetIsIdentified(bool identified) { _isIdentified = identified; }

	inline uint64_t EventSubscriptions() { return _eventSubscriptions; }
	inline void SetEventSubscriptions(uint64_t subscriptions) { _eventSubscriptions = subscriptions; }

	std::mutex OperationMutex;

private:
	std::mutex _remoteAddressMutex;
	std::string _remoteAddress;
	std::atomic<uint64_t> _connectedAt = 0;
	std::atomic<uint64_t> _incomingMessages = 0;
	std::atomic<uint64_t> _outgoingMessages = 0;
	std::atomic<uint8_t> _encoding = 0;
	std::atomic<bool> _authenticationRequired = false;
	std::mutex _secretMutex;
	std::string _secret;
	std::mutex _challengeMutex;
	std::string _challenge;
	std::atomic<uint8_t> _rpcVersion = OBS_WEBSOCKET_RPC_VERSION;
	std::atomic<bool> _isIdentified = false;
	std::atomic<uint64_t> _eventSubscriptions = EventSubscription::All;
};
