#include "libvr/SceneManager.h"
#include "libvr/OBJLoader.h"
#include <iostream>
#include <fstream>
#include <cassert>

int main()
{
	std::cout << "[Test3D] Initializing..." << std::endl;
	libvr::SceneManager scene;

	// Create dummy .obj
	std::string filename = "test_cube.obj";
	std::ofstream outfile(filename);
	outfile << "# Test Cube\n";
	outfile << "v -1.0 -1.0 1.0\n";
	outfile << "v 1.0 -1.0 1.0\n";
	outfile << "v -1.0 1.0 1.0\n";
	outfile << "f 1 2 3\n"; // Triangle
	outfile.close();

	// Load Mesh
	libvr::Mesh mesh = libvr::OBJLoader::Load(filename);
	if (mesh.vertices.empty()) {
		std::cerr << "[Test3D] Failed to load mesh!" << std::endl;
		return 1;
	}
	std::cout << "[Test3D] Loaded mesh with " << mesh.vertices.size() << " vertices." << std::endl;

	// Add to Scene
	uint32_t meshId = scene.AddMesh(mesh);

	libvr::Transform t = {{0, 0, 0}, {0, 0, 0, 1}, {1, 1, 1}};
	uint32_t nodeId = scene.AddMeshNode(t, meshId);

	const auto &nodes = scene.GetNodes();
	bool found = false;
	for (const auto &n : nodes) {
		if (n.id == nodeId && n.mesh_id == meshId) {
			found = true;
			break;
		}
	}

	if (found) {
		std::cout << "[Test3D] Node added successfully." << std::endl;
	} else {
		std::cerr << "[Test3D] Node not found in scene!" << std::endl;
		return 1;
	}

	// Add Light
	libvr::Light l = {0, {10, 10, 10}, {1, 1, 1}, 1.0f, 0};
	uint32_t lightId = scene.AddLight(l);

	assert(scene.GetLights().size() == 1);
	std::cout << "[Test3D] Light added." << std::endl;

	std::cout << "[Test3D] Done." << std::endl;
	return 0;
}
