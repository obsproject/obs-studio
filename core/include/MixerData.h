#pragma once

#include <QString>
#include <QVector3D>
#include <QSize>

namespace NeuralStudio {

    // Data transfer object for video feed information
    struct VideoFeedData {
        QString id;
        QString label;
        QString sourcePath;  // Camera ID, file path, URL, etc.

        // Video properties
        QSize resolution;
        int frameRate;
        bool isStereo;

        // 3D spatial positioning
        QVector3D position;
        QVector3D scale;

        // State
        bool active;

        VideoFeedData() :
            resolution(1920, 1080),
            frameRate(30),
            isStereo(false),
            position(0, 0, -5000),
            scale(10, 5.6, 1),
            active(false)
        {}
    };

    // Data transfer object for audio source information
    struct AudioSourceData {
        QString id;
        QString label;
        QString sourcePath;

        // Audio properties
        int sampleRate;
        int channels;

        // 3D spatial positioning
        QVector3D position;

        // Processing
        float volume;  // 0.0 to 1.0
        float gain;    // dB
        bool muted;

        // State
        bool active;

        AudioSourceData() :
            sampleRate(48000), channels(1), position(0, 0, -3000), volume(1.0f), gain(0.0f), muted(false), active(false)
        {}
    };

}  // namespace NeuralStudio
