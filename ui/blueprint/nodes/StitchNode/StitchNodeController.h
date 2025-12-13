#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include "../BaseNodeController.h"

namespace NeuralStudio {
    namespace UI {

        /**
 * @brief StitchNode UI Controller
 * 
 * QML-exposed controller for STMap stitching node in Blueprint editor.
 * Manages node properties and communicates with backend SceneGraph::StitchNode.
 */
        class StitchNodeController : public BaseNodeController
        {
            Q_OBJECT
            Q_PROPERTY(QString stmapPath READ stmapPath WRITE setStmapPath NOTIFY stmapPathChanged)
            QML_ELEMENT

              public:
            explicit StitchNodeController(QObject *parent = nullptr);
            ~StitchNodeController() override = default;

            QString stmapPath() const
            {
                return m_stmapPath;
            }
            void setStmapPath(const QString &path);

              signals:
            void stmapPathChanged();
            void propertyChanged(const QString &property, const QVariant &value);

              private:
            QString m_stmapPath;
        };

    }  // namespace UI
}  // namespace NeuralStudio
```
