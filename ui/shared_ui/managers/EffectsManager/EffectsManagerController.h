#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class EffectsManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableEffects READ availableEffects NOTIFY availableEffectsChanged)

              public:
            explicit EffectsManagerController(QObject *parent = nullptr);
            ~EffectsManagerController() override;

            QString title() const override
            {
                return "Effects";
            }
            QString nodeType() const override
            {
                return "EffectNode";
            }

            QVariantList availableEffects() const
            {
                return m_availableEffects;
            }
            Q_INVOKABLE QVariantList getEffects() const
            {
                return m_availableEffects;
            }

              public slots:
            void refreshEffects();
            void createEffectNode(const QString &effectType);

              signals:
            void availableEffectsChanged();
            void effectsChanged();

              private:
            void loadPredefinedEffects();
            QVariantList m_availableEffects;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
