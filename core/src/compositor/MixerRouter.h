#pragma once

#include <QObject>
#include <QMap>
#include <QVector>
#include <QString>
#include "../shared/MixerData.h"

namespace NeuralStudio {

    class MixerRouter : public QObject
    {
        Q_OBJECT

          public:
        explicit MixerRouter(QObject *parent = nullptr);
        ~MixerRouter();

        // Video feed management (using data objects)
        void addVideoFeed(const VideoFeedData &data);
        void updateVideoFeed(const VideoFeedData &data);
        void removeVideoFeed(const QString &feedId);
        bool hasVideoFeed(const QString &feedId) const;

        // Audio source management (using data objects)
        void addAudioSource(const AudioSourceData &data);
        void updateAudioSource(const AudioSourceData &data);
        void removeAudioSource(const QString &sourceId);
        bool hasAudioSource(const QString &sourceId) const;

        // Get all feeds/sources
        QVector<VideoFeedData> getVideoFeeds() const;
        QVector<AudioSourceData> getAudioSources() const;
        QVector<QString> getVideoFeedIds() const;
        QVector<QString> getAudioSourceIds() const;

        // Get specific feed/source
        VideoFeedData getVideoFeed(const QString &feedId) const;
        AudioSourceData getAudioSource(const QString &sourceId) const;

          signals:
        void videoFeedAdded(const QString &feedId);
        void videoFeedUpdated(const QString &feedId);
        void videoFeedRemoved(const QString &feedId);
        void audioSourceAdded(const QString &sourceId);
        void audioSourceUpdated(const QString &sourceId);
        void audioSourceRemoved(const QString &sourceId);

          private:
        QMap<QString, VideoFeedData> m_videoFeeds;
        QMap<QString, AudioSourceData> m_audioSources;
    };

}  // namespace NeuralStudio
