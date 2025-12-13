#pragma once

#include "../BaseNodeController.h"

namespace NeuralStudio {
    namespace UI {

        /**
     * @brief GeminiNodeController
     * 
     * UI Controller for the Gemini Cloud AI Node.
     * Inherits from BaseNodeController.
     */
        class GeminiNodeController : public BaseNodeController
        {
            Q_OBJECT
            Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
            Q_PROPERTY(QString modelName READ modelName WRITE setModelName NOTIFY modelNameChanged)

              public:
            explicit GeminiNodeController(QObject *parent = nullptr);
            ~GeminiNodeController() override = default;

            QString apiKey() const
            {
                return m_apiKey;
            }
            void setApiKey(const QString &key);

            QString modelName() const
            {
                return m_modelName;
            }
            void setModelName(const QString &name);

              signals:
            void apiKeyChanged();
            void modelNameChanged();

              private:
            QString m_apiKey;
            QString m_modelName = "gemini-1.5-flash";
        };

    }  // namespace UI
}  // namespace NeuralStudio
