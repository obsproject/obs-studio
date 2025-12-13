#pragma once

#include "../BaseNodeController.h"

namespace NeuralStudio {
    namespace Blueprint {

        class ThreeDModelNodeController : public BaseNodeController
        {
            Q_OBJECT
            Q_PROPERTY(QString modelPath READ getModelPath WRITE setModelPath NOTIFY modelPathChanged)
            Q_PROPERTY(QVector3D scale READ getScale WRITE setScale NOTIFY scaleChanged)
            Q_PROPERTY(QVector3D position READ getPosition WRITE setPosition NOTIFY positionChanged)
            Q_PROPERTY(QVector3D rotation READ getRotation WRITE setRotation NOTIFY rotationChanged)

              public:
            explicit ThreeDModelNodeController(QObject *parent = nullptr);
            ~ThreeDModelNodeController() override;

            QString getModelPath() const;
            void setModelPath(const QString &path);

            QVector3D getScale() const;
            void setScale(const QVector3D &scale);

            QVector3D getPosition() const;
            void setPosition(const QVector3D &pos);

            QVector3D getRotation() const;
            void setRotation(const QVector3D &rot);

              signals:
            void modelPathChanged();
            void scaleChanged();
            void positionChanged();
            void rotationChanged();

              private:
            QString m_modelPath;
            QVector3D m_scale {1.0f, 1.0f, 1.0f};
            QVector3D m_position {0.0f, 0.0f, 0.0f};
            QVector3D m_rotation {0.0f, 0.0f, 0.0f};
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
