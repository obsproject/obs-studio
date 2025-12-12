#include "libvr/IRenderEngine.h"
#include "libvr/SceneManager.h"
#include <iostream>

using namespace libvr;

// namespace libvr {
//    extern IRenderEngine* CreateVulkanRenderEngine();
// }

int main()
{
	std::cout << "[TestScene] Creating Engine & Scene..." << std::endl;
	auto engine = libvr::CreateVulkanRenderEngine();
	SceneManager scene;

	RenderConfig config = {};
	config.enable_validation_layers = false;
	engine->Initialize(config);

	std::cout << "[TestScene] Building Scene Graph..." << std::endl;
	Transform t = {};
	t.scale[0] = 1.0f;

	// Add Mesh Node (ID 1, Mesh ID 100)
	uint32_t node1 = scene.AddMeshNode(t, 100);
	std::cout << "[TestScene] Added Node " << node1 << " with Mesh 100" << std::endl;

	// Add Empty Node
	uint32_t node2 = scene.AddNode(t);
	std::cout << "[TestScene] Added Node " << node2 << " (Empty)" << std::endl;

	std::cout << "[TestScene] Rendering..." << std::endl;
	engine->BeginFrame();
	engine->DrawScene(&scene);

	// Verify output manually via logs in this skeleton

	engine->Shutdown();
	// delete engine; // Managed by unique_ptr
	return 0;
}
