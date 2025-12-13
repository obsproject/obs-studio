#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class LLMManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableModels READ availableModels NOTIFY availableModelsChanged)

              public:
            explicit LLMManagerController(QObject *parent = nullptr);
            ~LLMManagerController() override;

            QString title() const override
            {
                return "LLM Services";
            }
            QString nodeType() const override
            {
                return "LLMNode";
            }

            QVariantList availableModels() const
            {
                return m_availableModels;
            }
            Q_INVOKABLE QVariantList getModels() const
            {
                return m_availableModels;
            }

              public slots:
            void refreshModels();
            void createLLMNode(const QString &modelId, const QString &provider);

              signals:
            void availableModelsChanged();
            void modelsChanged();  // Alias for widget compatibility

              private:
            void loadPredefinedModels();
            QVariantList m_availableModels;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
