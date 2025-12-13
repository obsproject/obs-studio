#pragma once

#include "../NodeItem.h"
#include <QVector3D>
#include <QString>

namespace NeuralStudio {

    enum class VideoSourceType {
        Camera,
        File,
        ScreenCapture,
        NetworkStream
    };

    enum class StereoMode {
        Mono,
        SideBySide,
        TopBottom,
        LeftEye,
        RightEye
    };

    class VideoInputNode : public NodeItem
    {
        Q_OBJECT

          public:
        explicit VideoInputNode(const QString &title, QGraphicsItem *parent = nullptr);

        // Source configuration
        void setSourceType(VideoSourceType type);
        VideoSourceType sourceType() const
        {
            return m_sourceType;
        }

        void setSourcePath(const QString &path);
        QString sourcePath() const
        {
            return m_sourcePath;
        }

        // Video properties
        void setResolution(int width, int height);
        QSize resolution() const
        {
            return m_resolution;
        }

        void setFrameRate(int fps);
        int frameRate() const
        {
            return m_frameRate;
        }

        void setStereoMode(StereoMode mode);
        StereoMode stereoMode() const
        {
            return m_stereoMode;
        }

        // 3D spatial positioning
        void setPosition3D(const QVector3D &pos);
        QVector3D position3D() const
        {
            return m_position3D;
        }

        void setScale3D(const QVector3D &scale);
        QVector3D scale3D() const
        {
            return m_scale3D;
        }

        // Active state
        void setActive(bool active);
        bool isActive() const
        {
            return m_active;
        }

          signals:
        void sourceChanged();
        void videoPropertiesChanged();
        void position3DChanged();
        void activeStateChanged(bool active);

          private:
        VideoSourceType m_sourceType;
        QString m_sourcePath;
        QSize m_resolution;
        int m_frameRate;
        StereoMode m_stereoMode;
        QVector3D m_position3D;
        QVector3D m_scale3D;
        bool m_active;
    };

}  // namespace NeuralStudio
