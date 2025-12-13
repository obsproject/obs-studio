#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class AudioManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableAudio READ availableAudio NOTIFY availableAudioChanged)

              public:
            explicit AudioManagerController(QObject *parent = nullptr);
            ~AudioManagerController() override;

            QString title() const override
            {
                return "Audio Files";
            }
            QString nodeType() const override
            {
                return "AudioNode";
            }

            QVariantList availableAudio() const
            {
                return m_availableAudio;
            }
            Q_INVOKABLE QVariantList getAudio() const
            {
                return m_availableAudio;
            }

              public slots:
            void scanAudio();
            void createAudioNode(const QString &audioPath);

              signals:
            void availableAudioChanged();
            void audioChanged();

              private:
            QString m_audioDirectory;
            QVariantList m_availableAudio;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
