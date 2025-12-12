#include "libvr/SceneManager.h"
#include <algorithm>

namespace libvr {

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

int SceneManager::AddNode(const std::string& name) {
    std::lock_guard<std::mutex> lock(nodes_mutex);
    int id = next_id++;
    SceneNode node;
    node.id = id;
    node.name = name;
    // Default transform
    nodes.push_back(node);
    return id;
}

void SceneManager::RemoveNode(int id) {
    std::lock_guard<std::mutex> lock(nodes_mutex);
    nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
        [id](const SceneNode& node) { return node.id == id; }), nodes.end());
}

void SceneManager::SetTransform(int id, const Transform& transform) {
    std::lock_guard<std::mutex> lock(nodes_mutex);
    for (auto& node : nodes) {
        if (node.id == id) {
            node.transform = transform;
            break;
        }
    }
}

const std::vector<SceneNode>& SceneManager::GetNodes() const {
    // Note: returning reference is risky if list changes, but for now we trust caller retains lock or copy
    // A better way is to return a copy or use a callback.
    // implementing naive lock for this getter is tricky with reference return.
    // For now, no lock here just to keep signature same, assuming rendering happens when updates are paused or double buffered.
    return nodes; 
}

} // namespace libvr
