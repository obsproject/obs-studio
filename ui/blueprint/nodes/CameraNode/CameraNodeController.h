#pragma once

#include "../BaseNodeController.h"

namespace NeuralStudio {
    namespace UI {

        class CameraNodeController : public BaseNodeController
        {
            Q_OBJECT
            Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)
            QML_ELEMENT

              public:
            explicit CameraNodeController(QObject *parent = nullptr);
            ~CameraNodeController() override = default;

            QString deviceId() const;
            void setDeviceId(const QString &id);

              signals:
            void deviceIdChanged();

              protected:
            void onBackendNodeSet() override;

              private:
            QString m_deviceId;
        };

    }  // namespace UI
}  // namespace NeuralStudio
