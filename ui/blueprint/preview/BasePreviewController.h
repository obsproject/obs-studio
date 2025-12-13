#pragma once

#include <QObject>
#include <QSize>

namespace NeuralStudio {
    namespace UI {

        /**
     * @brief Base Preview Controller
     * 
     * Base class for Querying and Controlling 3D Viewports in Blueprint.
     * Manages active state, texture binding, and camera interactions.
     */
        class BasePreviewController : public QObject
        {
            Q_OBJECT
            Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
            Q_PROPERTY(int textureId READ textureId NOTIFY textureChanged)

              public:
            explicit BasePreviewController(QObject *parent = nullptr);
            virtual ~BasePreviewController() = default;

            bool active() const
            {
                return m_active;
            }
            void setActive(bool active);

            // To be implemented by concrete previews (ScenePreview, StereoPreview)
            virtual int textureId() const
            {
                return m_textureId;
            }

            Q_INVOKABLE virtual void resize(int width, int height);

            // Camera Interaction
            Q_INVOKABLE virtual void orbit(float dx, float dy);
            Q_INVOKABLE virtual void pan(float dx, float dy);
            Q_INVOKABLE virtual void zoom(float delta);

              signals:
            void activeChanged();
            void textureChanged();

              protected:
            bool m_active = true;
            int m_textureId = 0;
            QSize m_size;
        };

    }  // namespace UI
}  // namespace NeuralStudio
