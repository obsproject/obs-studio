#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class MLManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableModels READ availableModels NOTIFY availableModelsChanged)

              public:
            explicit MLManagerController(QObject *parent = nullptr);
            ~MLManagerController() override;

            QString title() const override
            {
                return "ML Models";
            }
            QString nodeType() const override
            {
                return "MLNode";
            }

            QVariantList availableModels() const
            {
                return m_availableModels;
            }

            // Renamed from getAvailableModels for consistency
            Q_INVOKABLE QVariantList getModels() const
            {
                return m_availableModels;
            }

              public slots:
            void scanModels();
            void createMLNode(const QString &modelPath);

              signals:
            void availableModelsChanged();
            void modelsChanged();  // Alias for widget compatibility

              private:
            QString m_modelsDirectory;
            QVariantList m_availableModels;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
