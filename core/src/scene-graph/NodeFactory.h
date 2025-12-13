#pragma once

#include "IExecutableNode.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace NeuralStudio {
    namespace SceneGraph {

        /**
 * @brief Factory class for creating node instances by type name.
 * 
 * Supports dynamic registration of node types, enabling plugin-based
 * extensibility.
 */
        class NodeFactory
        {
              public:
            // Function type for creating a node instance
            using NodeCreator = std::function<std::shared_ptr<IExecutableNode>(const std::string &)>;

            /**
     * @brief Register a node type with the factory.
     * 
     * @param typeName Unique identifier for the node type.
     * @param creator Function that creates a new instance of the node.
     */
            static void registerNodeType(const std::string &typeName, NodeCreator creator);

            /**
     * @brief Create a new node instance.
     * 
     * @param typeName Type of node to create.
     * @param nodeId Unique ID for the new node instance.
     * @return Shared pointer to the new node, or nullptr if type not found.
     */
            static std::shared_ptr<IExecutableNode> create(const std::string &typeName, const std::string &nodeId);

            /**
     * @brief Get a list of all registered node types.
     * 
     * @return Vector of registered type names.
     */
            static std::vector<std::string> getRegisteredTypes();

              private:
            static std::map<std::string, NodeCreator> &getRegistry();
        };

        /**
 * @brief Helper class for static registration of node types.
 * 
 * Instantiating this class in a global/static scope will strictly
 * register the node type at startup.
 */
        class NodeRegistrar
        {
              public:
            NodeRegistrar(const std::string &typeName, NodeFactory::NodeCreator creator)
            {
                NodeFactory::registerNodeType(typeName, creator);
            }
        };

    }  // namespace SceneGraph
}  // namespace NeuralStudio

/**
 * @brief Macro to register a node type.
 * 
 * Usage: REGISTER_NODE_TYPE(MyNodeClass, "MyNode")
 * This should be placed in the .cpp file of the node implementation.
 */
#define REGISTER_NODE_TYPE(ClassType, TypeName)                                               \
	namespace                                                                                 \
	{                                                                                         \
	NeuralStudio::SceneGraph::NodeRegistrar registrar_##ClassType(                            \
		TypeName, [](const std::string &id) -> std::shared_ptr<NeuralStudio::SceneGraph::IExecutableNode> { \
			return std::make_shared<ClassType>(id);                                           \
		});                                                                                   \
	}
