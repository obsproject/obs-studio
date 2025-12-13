#include <libvr/ipc/IPCServer.h>
#include "../../shared/vr-protocol/VRProtocol.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <vector>

namespace libvr {

IPCServer::IPCServer() : m_running(false) {}

IPCServer::~IPCServer()
{
	Stop();
}

bool IPCServer::Start(const std::string &socketPath)
{
	if (m_running)
		return false;

	m_socketPath = socketPath;
	m_serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (m_serverFd < 0) {
		std::cerr << "[IPC] Failed to create socket" << std::endl;
		return false;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);

	unlink(m_socketPath.c_str()); // Remove old socket

	if (bind(m_serverFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		std::cerr << "[IPC] Failed to bind socket: " << m_socketPath << std::endl;
		return false;
	}

	if (listen(m_serverFd, 5) < 0) {
		std::cerr << "[IPC] Failed to listen" << std::endl;
		return false;
	}

	m_running = true;
	m_serverThread = std::thread(&IPCServer::ServerLoop, this);
	std::cout << "[IPC] Server started at " << m_socketPath << std::endl;
	return true;
}

void IPCServer::Stop()
{
	m_running = false;
	if (m_serverFd >= 0) {
		close(m_serverFd);
		m_serverFd = -1;
	}
	if (m_serverThread.joinable()) {
		m_serverThread.join();
	}
	unlink(m_socketPath.c_str());
}

void IPCServer::ServerLoop()
{
	while (m_running) {
		struct sockaddr_un clientAddr;
		socklen_t clientLen = sizeof(clientAddr);
		int clientFd = accept(m_serverFd, (struct sockaddr *)&clientAddr, &clientLen);

		if (clientFd < 0) {
			if (m_running)
				std::cerr << "[IPC] Accept failed" << std::endl;
			continue;
		}

		// For simplicity in this skeleton, handle client in same thread or spawn new
		// For production, use a thread pool or async.
		// We'll spawn a detached thread per client for now.
		std::thread clientThread(&IPCServer::HandleClient, this, clientFd);
		clientThread.detach();
	}
}

void IPCServer::HandleClient(int clientFd)
{
	while (m_running) {
		VRProtocol::Header header;
		ssize_t bytes = recv(clientFd, &header, sizeof(header), 0);
		if (bytes <= 0)
			break; // Disconnect

		if (header.type == VRProtocol::MessageType::Command) {
			std::vector<char> payload(header.payloadSize + 1);
			// Read payload (simple loop for full read omitted for brevity in skeleton)
			recv(clientFd, payload.data(), header.payloadSize, 0);
			payload[header.payloadSize] = 0;

			std::string responseStr = "{}";
			if (m_commandCallback) {
				responseStr = m_commandCallback(std::string(payload.data()));
			}

			// Send Response
			VRProtocol::Header respHeader;
			respHeader.type = VRProtocol::MessageType::Response;
			respHeader.payloadSize = responseStr.size();
			send(clientFd, &respHeader, sizeof(respHeader), 0);
			send(clientFd, responseStr.data(), responseStr.size(), 0);
		}
	}
	close(clientFd);
}

void IPCServer::SetCommandCallback(std::function<std::string(const std::string &)> cb)
{
	m_commandCallback = cb;
}

} // namespace libvr
