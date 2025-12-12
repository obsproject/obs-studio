#pragma once

#include <vector>
#include <string>
#include <array>
#include <mutex>

namespace libvr {

struct Transform {
    std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
    std::array<float, 3> rotation = {0.0f, 0.0f, 0.0f}; // Euler angles
    std::array<float, 3> scale = {1.0f, 1.0f, 1.0f};
};

struct SceneNode {
    int id;
    std::string name;
    Transform transform;
    // Type info, mesh data, etc. would go here
};

class SceneManager {
public:
    SceneManager();
    ~SceneManager();

    int AddNode(const std::string& name);
    void RemoveNode(int id);
    void SetTransform(int id, const Transform& transform);
    const std::vector<SceneNode>& GetNodes() const;

private:
    std::vector<SceneNode> nodes;
    int next_id = 1;
    mutable std::mutex nodes_mutex;
};

} // namespace libvr
