#include "SceneGraphManager.h"
#include "NodeFactory.h"
#include "IExecutableNode.h"
#include <iostream>

// Include SceneManager definition
#include "SceneManager.h"

namespace NeuralStudio {
namespace SceneGraph {

SceneGraphManager::SceneGraphManager()
{
	m_nodeGraph = std::make_unique<NodeExecutionGraph>();
}

SceneGraphManager::~SceneGraphManager()
{
	shutdown();
}

bool SceneGraphManager::initialize(Rendering::IRenderEngine *renderer)
{
	m_renderer = renderer;

	// Initialize SceneManager (Shared with legacy or standalone)
	// If it was passed in externally, we would use that.
	// For now, we create one if not provided (though architecture suggests it might come from App)
	if (!m_sceneManager) {
		m_sceneManager = std::make_shared<libvr::SceneManager>();
	}

	// Initialize Execution Context
	m_executionContext.renderer = m_renderer;
	m_executionContext.sceneManager = m_sceneManager.get();
	// m_executionContext.sharedData is auto-init

	return true;
}

void SceneGraphManager::shutdown()
{
	// Cleanup graph
	if (m_nodeGraph) {
		// m_nodeGraph->clear(); // If such method exists
	}
	m_sceneManager.reset();
	m_renderer = nullptr;
}

void SceneGraphManager::update(double deltaTime)
{
	if (!m_nodeGraph)
		return;

	// Update Context
	m_executionContext.deltaTime = deltaTime;
	m_executionContext.frameNumber++;

	// Execute Graph
	// The graph determines where data goes (Output Nodes)
	m_nodeGraph->executeParallel(m_executionContext);
}

void SceneGraphManager::render()
{
	// Rendering is usually triggered by the graph's Render nodes.
	// However, if we need a main loop render call for specific managers, do it here.
}

NodeExecutionGraph *SceneGraphManager::getNodeGraph() const
{
	return m_nodeGraph.get();
}

libvr::SceneManager *SceneGraphManager::getSceneManager() const
{
	return m_sceneManager.get();
}

bool SceneGraphManager::createNode(const std::string &type, const std::string &id)
{
	if (!m_nodeGraph)
		return false;

	// 1. Create Node via Factory
	auto node = NodeFactory::create(type, id);
	if (!node) {
		std::cerr << "SceneGraphManager: Failed to create node of type " << type << std::endl;
		return false;
	}

	// 2. Add to Flow Graph
	m_nodeGraph->addNode(node);

	// 3. Sync to Scene (if applicable)
	// If this node represents a 3D object, we might want to create a generic node in SceneManager
	// But we shouldn't assume every node is a scene object (e.g. Audio Filter).
	// This logic belongs in the Node implementation (initialize) or a specific Sync Helper.
	// For now, we leave it decoupled.

	return true;
}

bool SceneGraphManager::deleteNode(const std::string &id)
{
	if (!m_nodeGraph)
		return false;

	// 1. Remove from Graph
	m_nodeGraph->removeNode(id);

	// 2. Cleanup Scene resources if tracked
	// auto it = m_nodeToSceneId.find(id);
	// if (it != m_nodeToSceneId.end()) {
	//     m_sceneManager->RemoveNode(it->second);
	//     m_nodeToSceneId.erase(it);
	// }

	return true;
}

bool SceneGraphManager::connectNodes(const std::string &srcId, const std::string &srcPin, const std::string &dstId,
				     const std::string &dstPin)
{
	if (!m_nodeGraph)
		return false;
	return m_nodeGraph->connectPins(srcId, srcPin, dstId, dstPin);
}

void SceneGraphManager::markDirty(const std::string &nodeId)
{
	// Signal graph to re-evaluate or invalidate cache
}

} // namespace SceneGraph
} // namespace NeuralStudio
