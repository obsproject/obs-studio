#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class VideoManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableVideos READ availableVideos NOTIFY availableVideosChanged)

              public:
            explicit VideoManagerController(QObject *parent = nullptr);
            ~VideoManagerController() override;

            QString title() const override
            {
                return "Video Files";
            }
            QString nodeType() const override
            {
                return "VideoNode";
            }

            QVariantList availableVideos() const
            {
                return m_availableVideos;
            }
            Q_INVOKABLE QVariantList getVideos() const
            {
                return m_availableVideos;
            }

              public slots:
            void scanVideos();
            void createVideoNode(const QString &videoPath);

              signals:
            void availableVideosChanged();
            void videosChanged();

              private:
            QString m_videosDirectory;
            QVariantList m_availableVideos;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
