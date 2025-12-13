#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QAbstractListModel>
#include <QString>
#include <QVector3D>
#include <vector>
#include <memory>
#include "scene-graph/NodeExecutionGraph.h"
#include "scene-graph/NodeFactory.h"

namespace NeuralStudio {
    namespace UI {

        class BaseNodeController;

        class NodeGraphController : public QObject
        {
            Q_OBJECT
            Q_PROPERTY(bool isCompiled READ isCompiled NOTIFY isCompiledChanged)
            QML_ELEMENT
            QML_UNCREATABLE("Controller is managed by C++ backend or instantiated once")

              public:
            explicit NodeGraphController(QObject *parent = nullptr);
            ~NodeGraphController();

            // QML Invokables
            Q_INVOKABLE QString createNode(const QString &nodeType, float x, float y, float z = 0.0f);
            Q_INVOKABLE void deleteNode(const QString &nodeId);
            Q_INVOKABLE bool connectPins(const QString &sourceNodeId, const QString &sourcePinId,
                                         const QString &targetNodeId, const QString &targetPinId);
            Q_INVOKABLE void disconnectPins(const QString &targetNodeId, const QString &targetPinId);
            Q_INVOKABLE void compileAndRun();
            Q_INVOKABLE void setNodeProperty(const QString &nodeId, const QString &propertyName, const QVariant &value);

            bool isCompiled() const;

            // Access to backend graph
            std::shared_ptr<SceneGraph::NodeExecutionGraph> getBackendGraph() const;

              signals:
            void isCompiledChanged();
            void nodeCreated(const QString &nodeId, const QString &nodeType, float x, float y, float z);
            void nodeDeleted(const QString &nodeId);
            void connectionCreated(const QString &sourceNodeId, const QString &sourcePinId, const QString &targetNodeId,
                                   const QString &targetPinId);
            void connectionDeleted(const QString &targetNodeId, const QString &targetPinId);
            void executionFinished(bool success, const QString &message);

              private:
            std::shared_ptr<SceneGraph::NodeExecutionGraph> m_backendGraph;
            // UI State: Store node positions alongside backend creation
            // VR: Using QVector3D to support Z-axis
            std::map<QString, QVector3D> m_nodePositions;

              public:
            Q_INVOKABLE QVector3D getNodePosition(const QString &nodeId) const;
            Q_INVOKABLE void updateNodePosition(const QString &nodeId, float x, float y, float z = 0.0f);
            Q_INVOKABLE void clearPositions();  // Helper for loading
            const std::map<QString, QVector3D> &getAllNodePositions() const
            {
                return m_nodePositions;
            }

            // Serialization
            Q_INVOKABLE bool saveGraph(const QString &filePath);
            Q_INVOKABLE bool loadGraph(const QString &filePath);
        };

    }  // namespace UI
}  // namespace NeuralStudio
