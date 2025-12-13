#pragma once

#include "../BaseNodeController.h"

namespace NeuralStudio {
    namespace UI {

        /**
 * @brief HeadsetOutputNode UI Controller
 * 
 * QML-exposed controller for Headset Output node in Blueprint editor.
 * Manages profile selection and communicates with backend HeadsetOutputNode.
 */
        class HeadsetOutputNodeController : public BaseNodeController
        {
            Q_OBJECT
            Q_PROPERTY(QString profileId READ profileId WRITE setProfileId NOTIFY profileIdChanged)
            QML_ELEMENT

              public:
            explicit HeadsetOutputNodeController(QObject *parent = nullptr);
            ~HeadsetOutputNodeController() override = default;

            QString profileId() const
            {
                return m_profileId;
            }
            void setProfileId(const QString &id);

              signals:
            void profileIdChanged();

              private:
            QString m_profileId = "quest3";
        };

    }  // namespace UI
}  // namespace NeuralStudio
