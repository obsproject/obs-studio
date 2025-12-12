#include "libvr/SceneManager.h"
#include <algorithm>

namespace libvr {

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

uint32_t SceneManager::AddNode(const Transform& transform) {
    return AddMeshNode(transform, 0);
}

uint32_t SceneManager::AddMeshNode(const Transform& transform, uint32_t mesh_id) {
    std::lock_guard<std::mutex> lock(nodes_mutex);
    uint32_t id = next_id++;
    SceneNode node;
    node.id = id;
    node.transform = transform;
    node.mesh_id = mesh_id;
    // Store in vector
    nodes.push_back(node);
    return id;
}

void SceneManager::RemoveNode(uint32_t id) {
    std::lock_guard<std::mutex> lock(nodes_mutex);
    nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
        [id](const SceneNode& node) { return node.id == id; }), nodes.end());
}

void SceneManager::SetTransform(uint32_t id, const Transform& transform) {
    std::lock_guard<std::mutex> lock(nodes_mutex);
    for (auto& node : nodes) {
        if (node.id == id) {
            node.transform = transform;
            break;
        }
    }
}

const std::vector<SceneNode>& SceneManager::GetNodes() const {
    return nodes; 
}

} // namespace libvr
