#pragma once

#include <vector>
#include <string>
#include <array>
#include <mutex>
#include <cstdint>  // For uint32_t

namespace libvr {

    struct Transform {
        float position[3];
        float rotation[4];  // Quaternion
        float scale[3];
    };

    struct Vertex {
        float position[3];
        float normal[3];
        float uv[2];
    };

    struct Mesh {
        uint32_t id;
        std::string name;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct Material {
        uint32_t id;
        float base_color[4];  // RGBA
        uint32_t texture_id;
        float roughness;
        float metallic;
    };

    struct Light {
        uint32_t id;
        float position[3];  // or direction for directional light
        float color[3];     // RGB
        float intensity;
        int type;  // 0 = Point, 1 = Directional
    };

    enum class StereoMode {
        MONO,
        STEREO_SBS,  // Side-by-Side
        STEREO_TB,   // Top-Bottom
        LEFT_EYE,    // Only visible to left eye
        RIGHT_EYE    // Only visible to right eye
    };

    struct AABB {
        float min[3];
        float max[3];
    };

    struct SemanticData {
        // Identity
        std::string label;              // e.g., "Office Desk"
        std::string category;           // e.g., "Furniture"
        std::vector<std::string> tags;  // e.g., ["flat", "wooden"]

        // Spatial Properties
        AABB boundingBox;

        // VR/Stereo Properties
        StereoMode stereoMode = StereoMode::MONO;
        float ipdOffset = 0.0f;
    };

    struct SceneNode {
        uint32_t id;
        Transform transform;
        uint32_t mesh_id;
        uint32_t material_id;
        uint32_t texture_id;  // Legacy/Override
        std::vector<uint32_t> children;

        // Semantic Layer
        SemanticData semantics;
    };

    enum class RelationType {
        SUPPORTED_BY,
        NEAR,
        FACING,
        INSIDE,
        PART_OF
    };

    struct SemanticEdge {
        uint32_t sourceNodeId;
        uint32_t targetNodeId;
        RelationType type;
        float weight;
    };

    class SceneManager
    {
          public:
        SceneManager();
        ~SceneManager();

        // Node Management
        uint32_t AddNode(const Transform &transform);
        uint32_t AddMeshNode(const Transform &transform, uint32_t mesh_id);
        uint32_t AddVideoNode(const Transform &transform, uint32_t mesh_id, uint32_t texture_id);
        void RemoveNode(uint32_t id);
        void SetTransform(uint32_t id, const Transform &transform);
        void SetSemantics(uint32_t id, const SemanticData &data);
        const std::vector<SceneNode> &GetNodes() const;

        // Resource Management
        uint32_t AddMesh(const Mesh &mesh);
        const Mesh *GetMesh(uint32_t id) const;

        uint32_t AddMaterial(const Material &material);
        const Material *GetMaterial(uint32_t id) const;

        uint32_t AddLight(const Light &light);
        const std::vector<Light> &GetLights() const;

        // Semantic Graph API
        void AddRelation(uint32_t sourceId, uint32_t targetId, RelationType type, float weight = 1.0f);
        const std::vector<SemanticEdge> &GetRelations() const;

          private:
        std::vector<SceneNode> nodes;
        int next_id = 1;
        mutable std::mutex nodes_mutex;

        // Semantic Graph
        std::vector<SemanticEdge> edges;

        // Resources
        std::vector<Mesh> meshes;
        int next_mesh_id = 1;

        std::vector<Material> materials;
        int next_material_id = 1;

        std::vector<Light> lights;
        int next_light_id = 1;
    };

}  // namespace libvr
