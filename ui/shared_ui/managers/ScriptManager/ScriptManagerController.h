#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class ScriptManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableScripts READ availableScripts NOTIFY availableScriptsChanged)

              public:
            explicit ScriptManagerController(QObject *parent = nullptr);
            ~ScriptManagerController() override;

            QString title() const override
            {
                return "Scripts & Logic";
            }
            QString nodeType() const override
            {
                return "ScriptNode";
            }

            QVariantList availableScripts() const
            {
                return m_availableScripts;
            }
            Q_INVOKABLE QVariantList getScripts() const
            {
                return m_availableScripts;
            }

              public slots:
            void scanScripts();
            void createScriptNode(const QString &scriptPath, const QString &language);

              signals:
            void availableScriptsChanged();
            void scriptsChanged();  // Alias for widget compatibility

              private:
            QString m_scriptsDirectory;
            QVariantList m_availableScripts;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
