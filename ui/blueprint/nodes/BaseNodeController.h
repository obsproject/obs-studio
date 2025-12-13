#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QQmlEngine>
#include <memory>

namespace NeuralStudio {
    namespace SceneGraph {
        class IExecutableNode;
    }
}  // namespace NeuralStudio

namespace NeuralStudio {
    namespace UI {

        class NodeGraphController;

        /**
         * @brief Base Node Controller for Blueprint UI
         * 
         * Provides common property management and signals for all UI nodes.
         * Bridges QML visual properties with C++ backend.
         */
        class BaseNodeController : public QObject
        {
            Q_OBJECT
            Q_PROPERTY(NodeGraphController *graphController READ graphController WRITE setGraphController NOTIFY
                           graphControllerChanged)

            // Port Configuration
            Q_PROPERTY(bool visualInputEnabled READ visualInputEnabled WRITE setVisualInputEnabled NOTIFY
                           visualInputEnabledChanged)
            Q_PROPERTY(bool visualOutputEnabled READ visualOutputEnabled WRITE setVisualOutputEnabled NOTIFY
                           visualOutputEnabledChanged)
            Q_PROPERTY(bool audioInputEnabled READ audioInputEnabled WRITE setAudioInputEnabled NOTIFY
                           audioInputEnabledChanged)
            Q_PROPERTY(bool audioOutputEnabled READ audioOutputEnabled WRITE setAudioOutputEnabled NOTIFY
                           audioOutputEnabledChanged)

              public:
            explicit BaseNodeController(QObject *parent = nullptr);
            virtual ~BaseNodeController() = default;

            QString nodeId() const
            {
                return m_nodeId;
            }
            void setNodeId(const QString &id);

            bool enabled() const
            {
                return m_enabled;
            }
            void setEnabled(bool enabled);

            // Port Getters/Setters
            bool visualInputEnabled() const
            {
                return m_visualInputEnabled;
            }
            void setVisualInputEnabled(bool enabled);

            bool visualOutputEnabled() const
            {
                return m_visualOutputEnabled;
            }
            void setVisualOutputEnabled(bool enabled);

            bool audioInputEnabled() const
            {
                return m_audioInputEnabled;
            }
            void setAudioInputEnabled(bool enabled);

            bool audioOutputEnabled() const
            {
                return m_audioOutputEnabled;
            }
            void setAudioOutputEnabled(bool enabled);

            NodeGraphController *graphController() const
            {
                return m_graphController;
            }
            void setGraphController(NodeGraphController *controller);

              signals:
            void nodeIdChanged();
            void enabledChanged();
            void graphControllerChanged();
            void propertyChanged(const QString &property, const QVariant &value);

            void visualInputEnabledChanged();
            void visualOutputEnabledChanged();
            void audioInputEnabledChanged();
            void audioOutputEnabledChanged();

              protected:
            void updateProperty(const QString &name, const QVariant &value);

            // Access to backend node (derived classes use this)
            std::shared_ptr<SceneGraph::IExecutableNode> getBackendNode() const;

              private:
            void tryConnectToBackend();

            QString m_nodeId;
            bool m_enabled = true;

            // Port Configuration Defaults
            bool m_visualInputEnabled = false;
            bool m_visualOutputEnabled = false;
            bool m_audioInputEnabled = false;
            bool m_audioOutputEnabled = false;

            NodeGraphController *m_graphController = nullptr;
            std::weak_ptr<SceneGraph::IExecutableNode> m_backendNode;
        };

    }  // namespace UI
}  // namespace NeuralStudio
