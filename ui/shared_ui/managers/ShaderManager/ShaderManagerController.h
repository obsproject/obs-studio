#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class ShaderManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableShaders READ availableShaders NOTIFY availableShadersChanged)

              public:
            explicit ShaderManagerController(QObject *parent = nullptr);
            ~ShaderManagerController() override;

            QString title() const override
            {
                return "Shaders";
            }
            QString nodeType() const override
            {
                return "ShaderNode";
            }

            QVariantList availableShaders() const
            {
                return m_availableShaders;
            }
            Q_INVOKABLE QVariantList getShaders() const
            {
                return m_availableShaders;
            }

              public slots:
            void scanShaders();
            void createShaderNode(const QString &shaderPath);

              signals:
            void availableShadersChanged();
            void shadersChanged();

              private:
            QString m_shadersDirectory;
            QVariantList m_availableShaders;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
