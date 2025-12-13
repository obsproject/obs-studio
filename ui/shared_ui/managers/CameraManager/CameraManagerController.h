#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class CameraManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableCameras READ availableCameras NOTIFY availableCamerasChanged)

              public:
            explicit CameraManagerController(QObject *parent = nullptr);
            ~CameraManagerController() override;

            QString title() const override
            {
                return "Camera Sources";
            }
            QString nodeType() const override
            {
                return "CameraNode";
            }

            QVariantList availableCameras() const
            {
                return m_availableCameras;
            }
            Q_INVOKABLE QVariantList getCameras() const
            {
                return m_availableCameras;
            }

              public slots:
            void scanCameras();
            void createCameraNode(const QString &deviceId, const QString &deviceName);

              signals:
            void availableCamerasChanged();
            void camerasChanged();  // Alias for widget compatibility

              private:
            void scanV4L2Devices();
            QVariantList m_availableCameras;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
