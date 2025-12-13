#include "NodeExecutionGraph.h"
#include <algorithm>
#include <iostream>
#include <future>

namespace NeuralStudio {
namespace SceneGraph {

NodeExecutionGraph::NodeExecutionGraph() {}

NodeExecutionGraph::~NodeExecutionGraph()
{
	m_nodes.clear();
}

//=============================================================================
// Node Management
//=============================================================================

void NodeExecutionGraph::addNode(std::shared_ptr<IExecutableNode> node)
{
	if (!node)
		return;

	std::string nodeId = node->getNodeId();
	m_nodes[nodeId] = node;
	m_isCompiled = false; // Graph changed, need recompilation
}

void NodeExecutionGraph::removeNode(const std::string &nodeId)
{
	// Remove all connections involving this node
	disconnectAllPins(nodeId);

	// Remove the node
	m_nodes.erase(nodeId);
	m_isCompiled = false;
}

std::shared_ptr<IExecutableNode> NodeExecutionGraph::getNode(const std::string &nodeId) const
{
	auto it = m_nodes.find(nodeId);
	return (it != m_nodes.end()) ? it->second : nullptr;
}

size_t NodeExecutionGraph::getNodeCount() const
{
	return m_nodes.size();
}

std::vector<std::string> NodeExecutionGraph::getNodeIds() const
{
	std::vector<std::string> ids;
	for (const auto &[id, _] : m_nodes) {
		ids.push_back(id);
	}
	return ids;
}

//=============================================================================
// Pin Connections
//=============================================================================

bool NodeExecutionGraph::connectPins(const std::string &sourceNodeId, const std::string &sourcePinId,
				     const std::string &targetNodeId, const std::string &targetPinId)
{
	// Validate nodes exist
	if (!getNode(sourceNodeId) || !getNode(targetNodeId)) {
		return false;
	}

	// Check for duplicate connection
	for (const auto &conn : m_connections) {
		if (conn.targetNodeId == targetNodeId && conn.targetPinId == targetPinId) {
			// Input already connected, disconnect first
			disconnectPins(targetNodeId, targetPinId);
			break;
		}
	}

	// Add connection
	PinConnection connection;
	connection.sourceNodeId = sourceNodeId;
	connection.sourcePinId = sourcePinId;
	connection.targetNodeId = targetNodeId;
	connection.targetPinId = targetPinId;

	m_connections.push_back(connection);
	m_isCompiled = false; // Graph changed

	return true;
}

void NodeExecutionGraph::disconnectPins(const std::string &targetNodeId, const std::string &targetPinId)
{
	m_connections.erase(std::remove_if(m_connections.begin(), m_connections.end(),
					   [&](const PinConnection &conn) {
						   return conn.targetNodeId == targetNodeId &&
							  conn.targetPinId == targetPinId;
					   }),
			    m_connections.end());

	m_isCompiled = false;
}

void NodeExecutionGraph::disconnectAllPins(const std::string &nodeId)
{
	m_connections.erase(std::remove_if(m_connections.begin(), m_connections.end(),
					   [&](const PinConnection &conn) {
						   return conn.sourceNodeId == nodeId || conn.targetNodeId == nodeId;
					   }),
			    m_connections.end());

	m_isCompiled = false;
}

std::vector<PinConnection> NodeExecutionGraph::getConnections() const
{
	return m_connections;
}

std::vector<PinConnection> NodeExecutionGraph::getInputConnections(const std::string &nodeId) const
{
	std::vector<PinConnection> inputs;
	for (const auto &conn : m_connections) {
		if (conn.targetNodeId == nodeId) {
			inputs.push_back(conn);
		}
	}
	return inputs;
}

std::vector<PinConnection> NodeExecutionGraph::getOutputConnections(const std::string &nodeId) const
{
	std::vector<PinConnection> outputs;
	for (const auto &conn : m_connections) {
		if (conn.sourceNodeId == nodeId) {
			outputs.push_back(conn);
		}
	}
	return outputs;
}

//=============================================================================
// Compilation (Topological Sort + Validation)
//=============================================================================

bool NodeExecutionGraph::compile()
{
	m_validationErrors.clear();
	m_executionOrder.clear();
	m_hasCycle = false;

	// Step 1: Validate connections
	if (!validateConnections()) {
		m_isCompiled = false;
		return false;
	}

	// Step 2: Detect cycles
	if (detectCycles()) {
		m_hasCycle = true;
		m_isCompiled = false;

		ValidationError error;
		error.type = ValidationError::Type::CycleDetected;
		error.message = "Cycle detected in node graph. Execution order cannot be determined.";
		m_validationErrors.push_back(error);

		return false;
	}

	// Step 3: Topological sort
	if (!topologicalSort()) {
		m_isCompiled = false;
		return false;
	}

	m_isCompiled = true;
	return true;
}

bool NodeExecutionGraph::topologicalSort()
{
	// Kahn's algorithm for topological sort

	// Step 1: Compute in-degrees for all nodes
	std::map<std::string, int> inDegree;
	for (const auto &[nodeId, _] : m_nodes) {
		inDegree[nodeId] = 0;
	}

	for (const auto &conn : m_connections) {
		inDegree[conn.targetNodeId]++;
	}

	// Step 2: Queue all nodes with in-degree 0
	std::queue<std::string> queue;
	for (const auto &[nodeId, degree] : inDegree) {
		if (degree == 0) {
			queue.push(nodeId);
		}
	}

	// Step 3: Process queue
	m_executionOrder.clear();
	while (!queue.empty()) {
		std::string nodeId = queue.front();
		queue.pop();
		m_executionOrder.push_back(nodeId);

		// Reduce in-degree of dependent nodes
		auto outgoingConnections = getOutputConnections(nodeId);
		for (const auto &conn : outgoingConnections) {
			if (--inDegree[conn.targetNodeId] == 0) {
				queue.push(conn.targetNodeId);
			}
		}
	}

	// Step 4: Check if all nodes were processed (if not, there's a cycle)
	if (m_executionOrder.size() != m_nodes.size()) {
		return false; // Cycle exists
	}

	return true;
}

bool NodeExecutionGraph::detectCycles()
{
	// DFS-based cycle detection
	std::set<std::string> visited;
	std::set<std::string> recursionStack;

	std::function<bool(const std::string &)> dfs = [&](const std::string &nodeId) -> bool {
		visited.insert(nodeId);
		recursionStack.insert(nodeId);

		// Visit all outgoing connections
		auto outgoing = getOutputConnections(nodeId);
		for (const auto &conn : outgoing) {
			if (recursionStack.find(conn.targetNodeId) != recursionStack.end()) {
				// Back edge found - cycle detected
				return true;
			}

			if (visited.find(conn.targetNodeId) == visited.end()) {
				if (dfs(conn.targetNodeId)) {
					return true;
				}
			}
		}

		recursionStack.erase(nodeId);
		return false;
	};

	// Run DFS from all unvisited nodes
	for (const auto &[nodeId, _] : m_nodes) {
		if (visited.find(nodeId) == visited.end()) {
			if (dfs(nodeId)) {
				return true; // Cycle found
			}
		}
	}

	return false; // No cycle
}

bool NodeExecutionGraph::validateConnections()
{
	bool valid = true;

	for (const auto &conn : m_connections) {
		if (!validatePinTypes(conn)) {
			valid = false;

			ValidationError error;
			error.type = ValidationError::Type::TypeMismatch;
			error.message = "Type mismatch between " + conn.sourceNodeId + "." + conn.sourcePinId +
					" and " + conn.targetNodeId + "." + conn.targetPinId;
			error.affectedNodes = {conn.sourceNodeId, conn.targetNodeId};
			m_validationErrors.push_back(error);
		}
	}

	return valid;
}

bool NodeExecutionGraph::validatePinTypes(const PinConnection &connection)
{
	auto sourceNode = getNode(connection.sourceNodeId);
	auto targetNode = getNode(connection.targetNodeId);

	if (!sourceNode || !targetNode) {
		return false;
	}

	// Find pin descriptors
	const auto &sourceOutputs = sourceNode->getOutputPins();
	const auto &targetInputs = targetNode->getInputPins();

	const PinDescriptor *sourcePin = nullptr;
	const PinDescriptor *targetPin = nullptr;

	for (const auto &pin : sourceOutputs) {
		if (pin.pinId == connection.sourcePinId) {
			sourcePin = &pin;
			break;
		}
	}

	for (const auto &pin : targetInputs) {
		if (pin.pinId == connection.targetPinId) {
			targetPin = &pin;
			break;
		}
	}

	if (!sourcePin || !targetPin) {
		return false; // Pin not found
	}

	// Check type compatibility
	return sourcePin->dataType.isCompatibleWith(targetPin->dataType);
}

//=============================================================================
// Execution
//=============================================================================

bool NodeExecutionGraph::execute(ExecutionContext &ctx)
{
	if (!m_isCompiled) {
		return false;
	}

	// Execute nodes in topological order
	for (const auto &nodeId : m_executionOrder) {
		if (!executeNode(nodeId, ctx)) {
			// Handle error
			if (m_errorHandler) {
				ExecutionResult result = ExecutionResult::failure("Node execution failed");
				m_errorHandler(result, nodeId);
			}

			if (!m_fallbackMode) {
				return false; // Stop execution on error
			}
			// Continue with fallback mode
		}

		// Propagate data to connected nodes
		propagateData();
	}

	return true;
}

bool NodeExecutionGraph::executeNode(const std::string &nodeId, ExecutionContext &ctx)
{
	auto node = getNode(nodeId);
	if (!node) {
		return false;
	}

	// Check cache (if caching enabled and node supports it)
	if (m_cachingEnabled && node->supportsCaching()) {
		size_t inputHash = computeInputHash(nodeId);
		CacheEntry entry;
		if (tryGetFromCache(nodeId, inputHash, entry)) {
			// Use cached data
			for (const auto &[pinId, data] : entry.outputData) {
				node->setPinData(pinId, data);
			}
			return true;
		}
	}

	// Execute node
	ExecutionResult result = node->process(ctx);

	// Update cache
	if (result.status == ExecutionResult::Status::Success && m_cachingEnabled) {
		size_t inputHash = computeInputHash(nodeId);
		updateCache(nodeId, inputHash);
	}

	return result.status == ExecutionResult::Status::Success;
}

void NodeExecutionGraph::propagateData()
{
	// Transfer data from output pins to input pins via connections
	for (const auto &conn : m_connections) {
		auto sourceNode = getNode(conn.sourceNodeId);
		auto targetNode = getNode(conn.targetNodeId);

		if (sourceNode && targetNode) {
			if (sourceNode->hasPinData(conn.sourcePinId)) {
				std::any data = sourceNode->getPinData(conn.sourcePinId);
				targetNode->setPinData(conn.targetPinId, data);
			}
		}
	}
}

//=============================================================================
// Parallel Execution
//=============================================================================

std::vector<std::vector<std::string>> NodeExecutionGraph::computeExecutionLevels()
{
	std::vector<std::vector<std::string>> levels;
	std::map<std::string, int> nodeLevel;

	// Compute level for each node (max distance from source nodes)
	for (const auto &nodeId : m_executionOrder) {
		int maxPredecessorLevel = -1;

		auto incoming = getInputConnections(nodeId);
		for (const auto &conn : incoming) {
			maxPredecessorLevel = std::max(maxPredecessorLevel, nodeLevel[conn.sourceNodeId]);
		}

		nodeLevel[nodeId] = maxPredecessorLevel + 1;
	}

	// Group nodes by level
	int maxLevel = 0;
	for (const auto &[_, level] : nodeLevel) {
		maxLevel = std::max(maxLevel, level);
	}

	levels.resize(maxLevel + 1);
	for (const auto &[nodeId, level] : nodeLevel) {
		levels[level].push_back(nodeId);
	}

	return levels;
}

bool NodeExecutionGraph::executeParallel(ExecutionContext &ctx)
{
	if (!m_isCompiled) {
		return false;
	}

	auto levels = computeExecutionLevels();

	// Execute each level in parallel
	for (const auto &level : levels) {
		std::vector<std::future<bool>> futures;

		for (const auto &nodeId : level) {
			futures.push_back(std::async(std::launch::async,
						     [this, nodeId, &ctx]() { return executeNode(nodeId, ctx); }));
		}

		// Wait for all nodes in this level to complete
		bool levelSuccess = true;
		for (auto &future : futures) {
			if (!future.get()) {
				levelSuccess = false;
			}
		}

		if (!levelSuccess && !m_fallbackMode) {
			return false;
		}

		// Propagate data after level completion
		propagateData();
	}

	return true;
}

//=============================================================================
// Caching
//=============================================================================

void NodeExecutionGraph::clearCache()
{
	m_cache.clear();
}

void NodeExecutionGraph::clearCacheForNode(const std::string &nodeId)
{
	m_cache.erase(nodeId);
}

size_t NodeExecutionGraph::computeInputHash(const std::string &nodeId) const
{
	size_t hash = 0;
	auto node = getNode(nodeId);
	if (node) {
		for (const auto &pin : node->getInputPins()) {
			if (!node->hasPinData(pin.pinId))
				continue;

			auto data = node->getPinData(pin.pinId);
			size_t valueHash = 0;

			// Hash based on type
			try {
				if (data.type() == typeid(float)) {
					valueHash = std::hash<float>{}(std::any_cast<float>(data));
				} else if (data.type() == typeid(int)) {
					valueHash = std::hash<int>{}(std::any_cast<int>(data));
				} else if (data.type() == typeid(bool)) {
					valueHash = std::hash<bool>{}(std::any_cast<bool>(data));
				} else if (data.type() == typeid(std::string)) {
					valueHash = std::hash<std::string>{}(std::any_cast<std::string>(data));
				} else if (data.type() == typeid(void *)) {
					valueHash = std::hash<void *>{}(std::any_cast<void *>(data));
				} else if (data.type() == typeid(std::shared_ptr<void>)) {
					valueHash = std::hash<std::shared_ptr<void>>{}(
						std::any_cast<std::shared_ptr<void>>(data));
				} else {
					// Fallback: hash the type info name as a placeholder
					// (Weak, but better than nothing for unknown types)
					valueHash = std::hash<std::string>{}(data.type().name());
				}
			} catch (...) {
				// Ignore casting errors
			}

			// Combine hashes (FNV-1a style)
			hash ^= std::hash<std::string>{}(pin.pinId);
			hash *= 0x100000001b3;
			hash ^= valueHash;
			hash *= 0x100000001b3;
		}
	}
	return hash;
}

bool NodeExecutionGraph::tryGetFromCache(const std::string &nodeId, size_t inputHash, CacheEntry &entry)
{
	auto it = m_cache.find(nodeId);
	if (it != m_cache.end() && it->second.inputHash == inputHash) {
		entry = it->second;
		return true;
	}
	return false;
}

void NodeExecutionGraph::updateCache(const std::string &nodeId, size_t inputHash)
{
	auto node = getNode(nodeId);
	if (!node)
		return;

	CacheEntry entry;
	entry.inputHash = inputHash;
	entry.timestamp = std::chrono::steady_clock::now();

	// Store output data
	for (const auto &pin : node->getOutputPins()) {
		if (node->hasPinData(pin.pinId)) {
			entry.outputData[pin.pinId] = node->getPinData(pin.pinId);
		}
	}

	m_cache[nodeId] = entry;
}

//=============================================================================
// Error Handling
//=============================================================================

void NodeExecutionGraph::setErrorHandler(std::function<void(const ExecutionResult &, const std::string &)> handler)
{
	m_errorHandler = handler;
}

} // namespace SceneGraph
} // namespace NeuralStudio
