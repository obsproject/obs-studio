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
    ML,
    AI,
    IncomingStreams,
    Broadcasting
};

class NodeItem;

struct NodeDefinition {
    QString name;
    NodeCategory category;
    QString description;
    std::function<NodeItem *()> createFunc;
};

class NodeRegistry
{
      public:
    static NodeRegistry &instance();

    void registerNode(const NodeDefinition &def);
    std::vector<NodeDefinition> getNodesByCategory(NodeCategory cat) const;
    std::vector<NodeDefinition> getAllNodes() const;
    const NodeDefinition *getNodeByName(const QString &name) const;

    QString categoryName(NodeCategory cat) const;

      private:
    NodeRegistry();
    std::vector<NodeDefinition> m_definitions;
};
