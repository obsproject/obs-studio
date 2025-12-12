#include <nlohmann/json.hpp> // Include FIRST to ensure clean env
#include "VRController.h"
#include <iostream>
// We need a JSON parser. Assuming we have one or can use a simple manual parser for MVP.
// Or we can include nlohmann/json if available. For now, simple manual parsing for "command" key.
// Actually, earlier checks (step 671) showed JsonCpp was NOT found.
// We should check if we have any json capability or write a trivial one.
// The frontend sends { "command": "...", "payload": { ... } }

#include <cstring>
#include <thread>
#include <chrono>

using json = nlohmann::json;

namespace libvr {

VRController::VRController()
{
	m_ipc = std::make_unique<IPCServer>();
	m_scene = std::make_shared<SceneManager>();
}

VRController::~VRController()
{
	Shutdown();
}

bool VRController::Initialize()
{
	// Start IPC
	// Path must match frontend: /tmp/vrobs.sock
	if (!m_ipc->Start("/tmp/vrobs.sock")) {
		std::cerr << "[VRController] Failed to start IPC Server" << std::endl;
		return false;
	}

	m_ipc->SetCommandCallback([this](const std::string &payload) { return this->HandleCommand(payload); });

	m_running = true;
	return true;
}

void VRController::Shutdown()
{
	if (m_ipc)
		m_ipc->Stop();
	m_running = false;
}

void VRController::Run()
{
	while (m_running) {
		// Main engine loop (physics, rendering updates)
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}

// Very basic JSON command dispatcher for MVP
// Basic manual parsing to avoid C++20/nlohmann conflicts
std::string VRController::HandleCommand(const std::string &jsonPayload)
{
	std::cout << "[VRController] Recv: " << jsonPayload << std::endl;

	try {
		json j = json::parse(jsonPayload);
		std::string cmd = j.value("command", "unknown");

		if (cmd == "addNode") {
			auto p = j["payload"];
			std::string uuid = p.value("id", "");
			float x = p.value("x", 0.0f);
			float y = p.value("y", 0.0f);
			float z = p.value("z", 0.0f);

			Transform t = {};
			t.position[0] = x;
			t.position[1] = y;
			t.position[2] = z;
			t.scale[0] = 1.0f;
			t.scale[1] = 1.0f;
			t.scale[2] = 1.0f;
			t.rotation[3] = 1.0f; // Identity w

			uint32_t internalId = m_scene->AddNode(t);
			m_nodeMap[uuid] = internalId;

			std::cout << "[VRController] ACTION: Add Node " << uuid << " -> ID " << internalId << std::endl;
		} else if (cmd == "moveNode") {
			auto p = j["payload"];
			std::string uuid = p.value("id", "");

			if (m_nodeMap.find(uuid) != m_nodeMap.end()) {
				float x = p.value("x", 0.0f);
				float y = p.value("y", 0.0f);
				float z = p.value("z", 0.0f);

				Transform t = {};
				t.position[0] = x;
				t.position[1] = y;
				t.position[2] = z;
				t.scale[0] = 1.0f;
				t.scale[1] = 1.0f;
				t.scale[2] = 1.0f;
				t.rotation[3] = 1.0f;

				m_scene->SetTransform(m_nodeMap[uuid], t);
				// std::cout << "[VRController] Moved Node " << uuid << std::endl;
			}
		} else if (cmd == "connectNodes") {
			auto p = j["payload"];
			std::cout << "[VRController] ACTION: Connect " << p.value("source", "?") << " -> "
				  << p.value("target", "?") << std::endl;
		} else if (cmd == "updateNode") {
			auto p = j["payload"];
			std::cout << "[VRController] ACTION: Update Node " << p.value("id", "?") << " ["
				  << p.value("key", "?") << "]" << std::endl;
		}
	} catch (const std::exception &e) {
		std::cerr << "[VRController] JSON Error: " << e.what() << std::endl;
		return "{\"status\":\"error\"}";
	}

	return "{\"status\":\"ok\"}";
}

} // namespace libvr
