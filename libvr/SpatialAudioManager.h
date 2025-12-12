#pragma once

#include <QAudioEngine>
#include <QSpatialSound>
#include <QAmbientSound>
#include <QAudioRoom>
#include <QVector3D>
#include <QMap>
#include <QString>

namespace NeuralStudio {

    class SpatialAudioManager
    {
          public:
        SpatialAudioManager();
        ~SpatialAudioManager();

        // Initialize audio engine
        void initialize();
        void shutdown();

        // Spatial sound management
        void addSpatialSound(const QString &nodeId, const QString &audioSource);
        void removeSpatialSound(const QString &nodeId);
        void updateSoundPosition(const QString &nodeId, const QVector3D &position);
        void updateSoundVolume(const QString &nodeId, float volume);

        // Ambient sounds (non-positional)
        void addAmbientSound(const QString &id, const QString &audioSource);
        void removeAmbientSound(const QString &id);

        // Listener (VR camera)
        void updateListenerPosition(const QVector3D &position);
        void updateListenerOrientation(const QVector3D &forward, const QVector3D &up);

        // Room simulation
        void setVirtualRoom(const QVector3D &roomDimensions, float reflectionGain = 0.5f);
        void disableRoom();

        // Output control
        void setMasterVolume(float volume);
        float getMasterVolume() const;

        // Retrieve mixed audio for encoding
        QByteArray getMixedAudioBuffer();

          private:
        QAudioEngine *m_audioEngine;
        QMap<QString, QSpatialSound *> m_spatialSounds;
        QMap<QString, QAmbientSound *> m_ambientSounds;
        QAudioRoom *m_virtualRoom;

        bool m_initialized;
    };

}  // namespace NeuralStudio
