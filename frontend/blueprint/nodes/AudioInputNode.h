#pragma once

#include "../NodeItem.h"
#include <QVector3D>
#include <QString>

namespace NeuralStudio {

    enum class AudioSourceType {
        Microphone,
        File,
        NetworkStream,
        Generated
    };

    class AudioInputNode : public NodeItem
    {
        Q_OBJECT

          public:
        explicit AudioInputNode(const QString &title, QGraphicsItem *parent = nullptr);

        // Source configuration
        void setSourceType(AudioSourceType type);
        AudioSourceType sourceType() const
        {
            return m_sourceType;
        }

        void setSourcePath(const QString &path);
        QString sourcePath() const
        {
            return m_sourcePath;
        }

        // Audio properties
        void setSampleRate(int rate);
        int sampleRate() const
        {
            return m_sampleRate;
        }

        void setChannels(int channels);
        int channels() const
        {
            return m_channels;
        }

        // 3D spatial positioning (for spatial audio)
        void setPosition3D(const QVector3D &pos);
        QVector3D position3D() const
        {
            return m_position3D;
        }

        // Audio processing
        void setVolume(float volume);  // 0.0 to 1.0
        float volume() const
        {
            return m_volume;
        }

        void setGain(float gain);  // dB
        float gain() const
        {
            return m_gain;
        }

        void setMuted(bool muted);
        bool isMuted() const
        {
            return m_muted;
        }

        // Active state
        void setActive(bool active);
        bool isActive() const
        {
            return m_active;
        }

          signals:
        void sourceChanged();
        void audioPropertiesChanged();
        void position3DChanged();
        void volumeChanged(float volume);
        void activeStateChanged(bool active);

          private:
        AudioSourceType m_sourceType;
        QString m_sourcePath;
        int m_sampleRate;
        int m_channels;
        QVector3D m_position3D;
        float m_volume;
        float m_gain;
        bool m_muted;
        bool m_active;
    };

}  // namespace NeuralStudio
