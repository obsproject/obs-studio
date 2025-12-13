#include <iostream>
#include <libvr/OpenXRRuntime.h>
#include <libvr/IRenderEngine.h>
#include <libvr/pipeline/FrameRouter.h>
#include <libvr/IStitcher.h>
#include <libvr/ISuperResolutionAdapter.h>
#include <libvr/SceneManager.h>
#include <nlohmann/json.hpp>
#include <libvr/ipc/IPCServer.h>
#include <memory>

// Factory from libvr
int main(int argc, char **argv)
{
	std::cout << "[obs-vr] Starting VR Engine..." << std::endl;

	// 1. Initialize OpenXR Runtime
	libvr::OpenXRRuntime xrRuntime;
	libvr::OpenXRRuntime::Config xrConfig;
	xrConfig.app_name = "OBS-VR-Fork";
	xrConfig.enable_debug = true;

	if (!xrRuntime.Initialize(xrConfig)) {
		std::cerr << "[obs-vr] Failed to initialize OpenXR. (Is Monado/SteamVR running?)" << std::endl;
		// In production, we might fallback to 2D mode, but for now error out.
		// For development scaffold, we proceed to test rest of logic if config allows,
		// but RunLoop requires init.
		// return 1;
	}

	// 2. Query OpenXR Requirements
	std::vector<std::string> requiredExtensions;
	if (!xrRuntime.GetRequiredVulkanExtensions(requiredExtensions)) {
		std::cerr << "[obs-vr] Failed to get OpenXR Vulkan requirements." << std::endl;
		return 1;
	}

	// 3. Initialize Renderer with Requirements
	auto renderer = libvr::CreateVulkanRenderEngine();
	libvr::RenderConfig renderConfig;
	renderConfig.width = 1920;
	renderConfig.height = 1080;
	renderConfig.enable_validation_layers = true;
	renderConfig.required_extensions = requiredExtensions;

	if (!renderer->Initialize(renderConfig)) {
		std::cerr << "[obs-vr] Failed to initialize Vulkan Renderer." << std::endl;
		return 1;
	}

	// 4. Bind OpenXR Session
	if (!xrRuntime.CreateVulkanSession((VkInstance)renderer->GetInstance(),
					   (VkPhysicalDevice)renderer->GetPhysicalDevice(),
					   (VkDevice)renderer->GetDevice(), renderer->GetGraphicsQueueFamily())) {
		std::cerr << "[obs-vr] Failed to bind OpenXR session to Vulkan." << std::endl;
		return 1;
	}

	// Capture/Encode Resolution (e.g. 1920x1080 per eye for testing, or 4K)
	// In Phase 9/10, we will fetch this from Config/Encoder settings.
	if (!xrRuntime.CreateSwapchains(1920, 1080)) {
		std::cerr << "[obs-vr] Failed to create OpenXR Swapchains." << std::endl;
		return 1;
	}

	// 5. Initialize Router & Pipeline Components
	libvr::FrameRouter router(renderer.get());

	// Scene Manager
	libvr::SceneManager sceneManager;

	// Create Stitcher
	auto stitcher = libvr::CreateStitcher();
	if (stitcher && stitcher->vtbl->Initialize) {
		stitcher->vtbl->Initialize(stitcher, 4096, 4096); // 4K input
	}
	router.SetStitcher(stitcher);

	// Create SuperResolution Adapter
	auto srAdapter = CreateSRAdapter();
	if (srAdapter && srAdapter->vtbl->Initialize) {
		srAdapter->vtbl->Initialize(srAdapter, 4096, 4096); // 4K input
	}
	router.SetSuperRes(srAdapter);

#include <unordered_map>

	// ...
	// 5.5 IPC Server (Backend UI Support)
	libvr::IPCServer ipcServer;
	std::unordered_map<std::string, uint32_t> uuidMap;

	// Simple command handler
	ipcServer.SetCommandCallback([&](const std::string &msg) -> std::string {
		std::cout << "[obs-vr] IPC Command Received: " << msg << std::endl;
		try {
			auto j = nlohmann::json::parse(msg);
			if (j.contains("command")) {
				std::string cmd = j["command"];

				if (cmd == "createNode") {
					float x = j.value("x", 0.0f);
					float y = j.value("y", 0.0f);

					libvr::Transform t = {{x, y, 0.0f}, {0, 0, 0, 1}, {1, 1, 1}};
					uint32_t id = sceneManager.AddNode(t);

					if (j.contains("id")) {
						std::string uuid = j["id"];
						uuidMap[uuid] = id;
					}

					libvr::SemanticData sData;
					if (j.contains("label"))
						sData.label = j["label"];
					sceneManager.SetSemantics(id, sData);

					std::cout << "Created Node ID: " << id << " Label: " << sData.label
						  << std::endl;
					return "{\"status\": \"ok\", \"id\": " + std::to_string(id) + "}";
				}

				if (cmd == "updateNode") {
					if (j.contains("id")) {
						std::string uuid = j["id"];
						if (uuidMap.find(uuid) != uuidMap.end()) {
							uint32_t nodeId = uuidMap[uuid];
							if (j.contains("updates")) {
								auto updates = j["updates"];

								// Get current state (Naive linear search for now)
								const auto &nodes = sceneManager.GetNodes();
								libvr::Transform currentT;
								libvr::SemanticData currentS;
								bool found = false;
								for (const auto &n : nodes) {
									if (n.id == nodeId) {
										currentT = n.transform;
										currentS = n.semantics;
										found = true;
										break;
									}
								}

								if (found) {
									bool tChanged = false;
									if (updates.contains("position")) {
										auto p = updates["position"];
										currentT.position[0] = p[0];
										currentT.position[1] = p[1];
										currentT.position[2] = p[2];
										tChanged = true;
									}
									if (updates.contains("scale")) {
										auto s = updates["scale"];
										currentT.scale[0] = s[0];
										currentT.scale[1] = s[1];
										currentT.scale[2] = s[2];
										tChanged = true;
									}
									if (updates.contains("rotation")) {
										auto r = updates["rotation"];
										currentT.rotation[0] = r[0];
										currentT.rotation[1] = r[1];
										currentT.rotation[2] = r[2];
										currentT.rotation[3] = r[3];
										tChanged = true;
									}
									if (tChanged)
										sceneManager.SetTransform(nodeId,
													  currentT);

									bool sChanged = false;
									if (updates.contains("stereoMode")) {
										currentS.stereoMode =
											(libvr::StereoMode)
												updates["stereoMode"]
													.get<int>();
										sChanged = true;
									}
									/* Handle LensType if we add it to SemanticData later */

									if (sChanged)
										sceneManager.SetSemantics(nodeId,
													  currentS);
								}
							}
						}
					}
					return "{\"status\": \"ok\"}";
				}

				if (cmd == "getScenes") {
					return "{\"scenes\": [\"Scene 1\", \"Scene 2\"], \"current\": \"Scene 1\"}";
				}
			}
		} catch (...) {
			return "{\"status\": \"error\", \"message\": \"Invalid JSON\"}";
		}
		return "{\"status\": \"ok\", \"echo\": \"accepted\"}";
	});

	if (!ipcServer.Start("/tmp/vrobs.sock")) {
		std::cerr << "[obs-vr] Failed to start IPC Server." << std::endl;
		// Warning only
	}

	// 6. Run Loop
	std::cout << "[obs-vr] Entering Main Loop..." << std::endl;
	xrRuntime.RunLoop([&]() {
		// This callback runs every frame synchronized with VR
		// 1. Acquire Image for View 0 (Left Eye) - MVP: Single resolved image or stereo handling
		// For MVP, we render mono or assume side-by-side or just View 0.
		// Let's do View 0.
		uint32_t imageIndex = 0;
		void *dstImage = xrRuntime.GetResolvedSwapchainImage(0, imageIndex);

		if (dstImage) {
			router.ProcessFrame(dstImage);
			xrRuntime.ReleaseSwapchainImage(0);
		} else {
			// Fallback or error logging
			router.ProcessFrame(nullptr);
		}

		// MVP: Handle View 1 (Right Eye) by recalling ProcessFrame or doing Stereo logic
		// For now, we only drive one loop.
	});

	ipcServer.Stop();

	std::cout << "[obs-vr] Shutdown." << std::endl;
	renderer->Shutdown();
	xrRuntime.Shutdown();
	return 0;
}
