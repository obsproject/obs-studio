#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QVector3D>

namespace NeuralStudio {

    class MixerViewModel : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QVariantList videoFeeds READ videoFeeds NOTIFY videoFeedsChanged)
        Q_PROPERTY(QVariantList audioSources READ audioSources NOTIFY audioSourcesChanged)
        Q_PROPERTY(int layoutType READ layoutType WRITE setLayoutType NOTIFY layoutTypeChanged)

          public:
        explicit MixerViewModel(QObject *parent = nullptr);

        enum LayoutType {
            Single = 0,
            PictureInPicture = 1,
            SideBySide = 2,
            Grid2x2 = 3,
            Grid3x3 = 4
        };
        Q_ENUM(LayoutType)

        // Video feed management
        Q_INVOKABLE void addVideoFeed(const QString &id, const QString &label, const QString &source);
        Q_INVOKABLE void removeVideoFeed(const QString &id);
        Q_INVOKABLE void setActiveFeed(const QString &id);
        Q_INVOKABLE void updateVideoPosition(const QString &id, const QVector3D &position);

        // Audio source management
        Q_INVOKABLE void addAudioSource(const QString &id, const QString &label, const QVector3D &position);
        Q_INVOKABLE void removeAudioSource(const QString &id);
        Q_INVOKABLE void updateAudioLevel(const QString &id, float level);
        Q_INVOKABLE void updateAudioPosition(const QString &id, const QVector3D &position);
        Q_INVOKABLE void setAudioMuted(const QString &id, bool muted);

        // Layout control
        int layoutType() const
        {
            return static_cast<int>(m_layoutType);
        }
        void setLayoutType(int type);

        // Getters
        QVariantList videoFeeds() const;
        QVariantList audioSources() const;

          signals:
        void videoFeedsChanged();
        void audioSourcesChanged();
        void layoutTypeChanged();
        void activeFeedChanged(const QString &id);

          private:
        struct VideoFeed {
            QString id;
            QString label;
            QString source;  // Camera ID, file path, etc.
            QVector3D position;
            bool active;
            float aspectRatio;
        };

        struct AudioSource {
            QString id;
            QString label;
            QVector3D position;
            float level;  // 0.0 to 1.0
            bool muted;
        };

        QVector<VideoFeed> m_videoFeeds;
        QVector<AudioSource> m_audioSources;
        LayoutType m_layoutType;
    };

}  // namespace NeuralStudio
