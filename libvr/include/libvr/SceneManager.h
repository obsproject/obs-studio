#pragma once

#include <vector>
#include <string>
#include <array>
#include <mutex>
#include <cstdint> // For uint32_t

namespace libvr {

struct Transform {
    float position[3];
    float rotation[4]; // Quaternion
    float scale[3];
};

struct Mesh {
    uint32_t id;
    // Simplified for skeleton: assume static buffers are managed elsewhere by ID
    // In real engine: std::vector<Vertex> vertices;
    std::string name;
};

struct SceneNode {
    uint32_t id;
    Transform transform;
    uint32_t mesh_id; // 0 = no mesh
    std::vector<uint32_t> children;
};

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    uint32_t AddNode(const Transform& transform);
    uint32_t AddMeshNode(const Transform& transform, uint32_t mesh_id); // New
    void RemoveNode(uint32_t id);
    void SetTransform(uint32_t id, const Transform& transform);
    const std::vector<SceneNode>& GetNodes() const;

private:
    std::vector<SceneNode> nodes;
    int next_id = 1;
    mutable std::mutex nodes_mutex;
};

} // namespace libvr
