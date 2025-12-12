#pragma once
#include <QString>
#include <vector>
#include <map>
#include <functional>

enum class NodeCategory {
    Cameras,
    Video,
    Photo,
    Script,
    ThreeD,
    Transitions,
    Effects,
    Filters,
    Audio,
    ThreeD,
    ML,
    AI,
    IncomingStreams,
    // Gaps
    Photo,
    Script,
    Transitions,
    Effects,
    Filters,
    Broadcasting
};

struct NodeDefinition {
    QString name;
    NodeCategory category;
    QString description;
};

class NodeRegistry
{
      public:
    static NodeRegistry &instance();

    void registerNode(const NodeDefinition &def);
    std::vector<NodeDefinition> getNodesByCategory(NodeCategory cat) const;
    std::vector<NodeDefinition> getAllNodes() const;

    QString categoryName(NodeCategory cat) const;

      private:
    NodeRegistry();
    std::vector<NodeDefinition> m_definitions;
};
