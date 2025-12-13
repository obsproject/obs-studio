#include "SceneManager.h"
#include <algorithm>

namespace libvr {

SceneManager::SceneManager() = default;
SceneManager::~SceneManager() = default;

uint32_t SceneManager::AddNode(const Transform &transform)
{
	return AddMeshNode(transform, 0);
}

uint32_t SceneManager::AddMeshNode(const Transform &transform, uint32_t mesh_id)
{
	return AddVideoNode(transform, mesh_id, 0);
}

uint32_t SceneManager::AddVideoNode(const Transform &transform, uint32_t mesh_id, uint32_t texture_id)
{
	// Delegate to string-based AddVideoNode with empty string (no storage)
	// Or simpler: Just keep existing logic for internal calls
	std::lock_guard<std::mutex> lock(nodes_mutex);
	uint32_t id = next_id++;
	SceneNode node;
	node.id = id;
	node.transform = transform;
	node.mesh_id = mesh_id;
	node.material_id = 0;
	node.texture_id = texture_id;
	nodes.push_back(node);
	return id;
}

uint32_t SceneManager::AddNode(const std::string &externalId, const Transform &transform)
{
	return AddMeshNode(externalId, transform, 0);
}

uint32_t SceneManager::AddMeshNode(const std::string &externalId, const Transform &transform, uint32_t mesh_id)
{
	return AddVideoNode(externalId, transform, mesh_id, 0);
}

uint32_t SceneManager::AddVideoNode(const std::string &externalId, const Transform &transform, uint32_t mesh_id,
				    uint32_t texture_id)
{
	std::lock_guard<std::mutex> lock(nodes_mutex);

	// Check if ID already exists
	if (m_externalIdMap.find(externalId) != m_externalIdMap.end()) {
		// Log warning or return existing ID
		return m_externalIdMap[externalId];
	}

	uint32_t id = next_id++;
	SceneNode node;
	node.id = id;
	node.transform = transform;
	node.mesh_id = mesh_id;
	node.material_id = 0;
	node.texture_id = texture_id;
	nodes.push_back(node);

	// Store mapping
	m_externalIdMap[externalId] = id;

	return id;
}

uint32_t SceneManager::GetNodeId(const std::string &externalId) const
{
	std::lock_guard<std::mutex> lock(nodes_mutex);
	auto it = m_externalIdMap.find(externalId);
	if (it != m_externalIdMap.end()) {
		return it->second;
	}
	return 0; // 0 is invalid ID
}

void SceneManager::RemoveNode(const std::string &externalId)
{
	uint32_t id = 0;
	{
		std::lock_guard<std::mutex> lock(nodes_mutex);
		auto it = m_externalIdMap.find(externalId);
		if (it != m_externalIdMap.end()) {
			id = it->second;
			m_externalIdMap.erase(it);
		}
	}

	if (id != 0) {
		RemoveNode(id);
	}
}

void SceneManager::RemoveNode(uint32_t id)
{
	std::lock_guard<std::mutex> lock(nodes_mutex);
	nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [id](const SceneNode &node) { return node.id == id; }),
		    nodes.end());
}

void SceneManager::SetTransform(uint32_t id, const Transform &transform)
{
	std::lock_guard<std::mutex> lock(nodes_mutex);
	for (auto &node : nodes) {
		if (node.id == id) {
			node.transform = transform;
			break;
		}
	}
}

void SceneManager::SetSemantics(uint32_t id, const SemanticData &data)
{
	std::lock_guard<std::mutex> lock(nodes_mutex);
	for (auto &node : nodes) {
		if (node.id == id) {
			node.semantics = data;
			break;
		}
	}
}

const std::vector<SceneNode> &SceneManager::GetNodes() const
{
	return nodes;
}

uint32_t SceneManager::AddMesh(const Mesh &mesh)
{
	std::lock_guard<std::mutex> lock(nodes_mutex);
	uint32_t id = next_mesh_id++;
	Mesh m = mesh;
	m.id = id;
	meshes.push_back(m);
	return id;
}

const Mesh *SceneManager::GetMesh(uint32_t id) const
{
	// Linear search for now, verify perf later
	// In real engine use map
	for (const auto &m : meshes) {
		if (m.id == id)
			return &m;
	}
	return nullptr;
}

uint32_t SceneManager::AddMaterial(const Material &material)
{
	std::lock_guard<std::mutex> lock(nodes_mutex);
	uint32_t id = next_material_id++;
	Material m = material;
	m.id = id;
	materials.push_back(m);
	return id;
}

const Material *SceneManager::GetMaterial(uint32_t id) const
{
	for (const auto &m : materials) {
		if (m.id == id)
			return &m;
	}
	return nullptr;
}

uint32_t SceneManager::AddLight(const Light &light)
{
	std::lock_guard<std::mutex> lock(nodes_mutex);
	uint32_t id = next_light_id++;
	Light l = light;
	l.id = id;
	lights.push_back(l);
	return id;
}

const std::vector<Light> &SceneManager::GetLights() const
{
	return lights;
}

void SceneManager::AddRelation(uint32_t sourceId, uint32_t targetId, RelationType type, float weight)
{
	std::lock_guard<std::mutex> lock(nodes_mutex);
	SemanticEdge edge{sourceId, targetId, type, weight};
	edges.push_back(edge);
}

const std::vector<SemanticEdge> &SceneManager::GetRelations() const
{
	return edges;
}

} // namespace libvr
