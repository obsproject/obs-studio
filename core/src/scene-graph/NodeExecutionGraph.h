#pragma once

#include "IExecutableNode.h"
#include <queue>
#include <set>
#include <chrono>

namespace NeuralStudio {
    namespace SceneGraph {

        //=============================================================================
        // Pin Connection
        //=============================================================================

        struct PinConnection {
            std::string sourceNodeId;
            std::string sourcePinId;
            std::string targetNodeId;
            std::string targetPinId;

            bool operator==(const PinConnection &other) const
            {
                return sourceNodeId == other.sourceNodeId && sourcePinId == other.sourcePinId &&
                       targetNodeId == other.targetNodeId && targetPinId == other.targetPinId;
            }
        };

        //=============================================================================
        // Validation Error
        //=============================================================================

        struct ValidationError {
            enum class Type {
                CycleDetected,
                TypeMismatch,
                MissingConnection,
                InvalidNode,
                DuplicateConnection
            };

            Type type;
            std::string message;
            std::vector<std::string> affectedNodes;
        };

        //=============================================================================
        // Cache Entry
        //=============================================================================

        struct CacheEntry {
            std::map<std::string, std::any> outputData;
            uint64_t frameNumber = 0;
            std::chrono::steady_clock::time_point timestamp;
            size_t inputHash = 0;
        };

        //=============================================================================
        // Node Execution Graph
        //=============================================================================

        class NodeExecutionGraph
        {
              public:
            NodeExecutionGraph();
            ~NodeExecutionGraph();

            // Node management
            void addNode(std::shared_ptr<IExecutableNode> node);
            void removeNode(const std::string &nodeId);
            std::shared_ptr<IExecutableNode> getNode(const std::string &nodeId) const;
            size_t getNodeCount() const;
            std::vector<std::string> getNodeIds() const;

            // Pin connections
            bool connectPins(const std::string &sourceNodeId, const std::string &sourcePinId,
                             const std::string &targetNodeId, const std::string &targetPinId);
            void disconnectPins(const std::string &targetNodeId, const std::string &targetPinId);
            void disconnectAllPins(const std::string &nodeId);

            std::vector<PinConnection> getConnections() const;
            std::vector<PinConnection> getInputConnections(const std::string &nodeId) const;
            std::vector<PinConnection> getOutputConnections(const std::string &nodeId) const;

            // Compilation (topological sort + validation)
            bool compile();
            bool isCompiled() const
            {
                return m_isCompiled;
            }

            // Execution
            bool execute(ExecutionContext &ctx);
            bool executeParallel(ExecutionContext &ctx);  // Parallel execution

            // Query
            const std::vector<std::string> &getExecutionOrder() const
            {
                return m_executionOrder;
            }
            bool hasCycle() const
            {
                return m_hasCycle;
            }
            const std::vector<ValidationError> &getValidationErrors() const
            {
                return m_validationErrors;
            }

            // Caching
            void enableCaching(bool enable)
            {
                m_cachingEnabled = enable;
            }
            void clearCache();
            void clearCacheForNode(const std::string &nodeId);

            // Error handling
            void setErrorHandler(std::function<void(const ExecutionResult &, const std::string &nodeId)> handler);
            void setEnableFallbackMode(bool enable)
            {
                m_fallbackMode = enable;
            }

              private:
            // Graph algorithms
            bool topologicalSort();
            bool detectCycles();
            bool validateConnections();
            bool validatePinTypes(const PinConnection &connection);

            // Execution helpers
            bool executeNode(const std::string &nodeId, ExecutionContext &ctx);
            void propagateData();

            // Caching helpers
            size_t computeInputHash(const std::string &nodeId) const;
            bool tryGetFromCache(const std::string &nodeId, size_t inputHash, CacheEntry &entry);
            void updateCache(const std::string &nodeId, size_t inputHash);

            // Parallel execution helpers
            std::vector<std::vector<std::string>> computeExecutionLevels();

            // Data
            std::map<std::string, std::shared_ptr<IExecutableNode>> m_nodes;
            std::vector<PinConnection> m_connections;

            // Execution state
            std::vector<std::string> m_executionOrder;
            bool m_isCompiled = false;
            bool m_hasCycle = false;
            std::vector<ValidationError> m_validationErrors;

            // Caching
            bool m_cachingEnabled = true;
            std::map<std::string, CacheEntry> m_cache;

            // Error handling
            bool m_fallbackMode = false;
            std::function<void(const ExecutionResult &, const std::string &)> m_errorHandler;
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio
