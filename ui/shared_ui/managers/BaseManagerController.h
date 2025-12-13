#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QMap>

namespace NeuralStudio {
    namespace UI {

        class NodeGraphController;

        /**
     * @brief Base Manager Controller - Hub for managing multiple nodes
     * 
     * Each manager acts as a central hub that:
     * - Tracks multiple nodes of its type (e.g., 4 cameras)
     * - Saves/loads node presets with configurations
     * - Provides a unified interface for node management
     */
        class BaseManagerController : public QObject
        {
            Q_OBJECT
            Q_PROPERTY(NodeGraphController *graphController READ graphController WRITE setGraphController NOTIFY
                           graphControllerChanged)
            Q_PROPERTY(QString title READ title CONSTANT)
            Q_PROPERTY(QStringList managedNodes READ managedNodes NOTIFY managedNodesChanged)
            Q_PROPERTY(QVariantList nodePresets READ nodePresets NOTIFY nodePresetsChanged)

              public:
            explicit BaseManagerController(QObject *parent = nullptr);
            virtual ~BaseManagerController() = default;

            // Abstract: Title of the manager (e.g., "Camera Manager")
            virtual QString title() const = 0;

            // Abstract: Node type this manager handles
            virtual QString nodeType() const = 0;

            NodeGraphController *graphController() const
            {
                return m_graphController;
            }
            void setGraphController(NodeGraphController *controller);

            // Multi-node tracking
            QStringList managedNodes() const;
            Q_INVOKABLE bool isManaging(const QString &nodeId) const;
            Q_INVOKABLE QString getResourceForNode(const QString &nodeId) const;
            Q_INVOKABLE QVariantMap getNodeInfo(const QString &nodeId) const;

            // Preset management
            QVariantList nodePresets() const
            {
                return m_presets;
            }
            Q_INVOKABLE void savePreset(const QString &nodeId, const QString &presetName);
            Q_INVOKABLE QString loadPreset(const QString &presetName, float x = 0, float y = 0);
            Q_INVOKABLE void deletePreset(const QString &presetName);

            // Common Actions
            Q_INVOKABLE virtual void createNode(const QString &nodeType, float x, float y);

              signals:
            void graphControllerChanged();
            void managedNodesChanged();
            void nodePresetsChanged();
            void errorOccurred(const QString &message);

              protected:
            // Track a created node
            void trackNode(const QString &nodeId, const QString &resourceId,
                           const QVariantMap &metadata = QVariantMap());

            // Remove tracking when node deleted
            void untrackNode(const QString &nodeId);

            // Node deletion handler
            void onNodeDeleted(const QString &nodeId);

            // Preset storage helpers
            void loadPresetsFromDisk();
            void savePresetsToDisk();
            QString getPresetsFilePath() const;

            NodeGraphController *m_graphController = nullptr;

              protected:
            QMap<QString, QVariantMap> m_managedNodeInfo;  // nodeId -> {resourceId, metadata}
            QVariantList m_presets;                        // [{name, properties, metadata}]
        };

    }  // namespace UI
}  // namespace NeuralStudio
