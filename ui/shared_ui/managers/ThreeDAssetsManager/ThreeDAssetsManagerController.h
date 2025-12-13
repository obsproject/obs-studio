#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class ThreeDAssetsManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableAssets READ availableAssets NOTIFY availableAssetsChanged)

              public:
            explicit ThreeDAssetsManagerController(QObject *parent = nullptr);
            ~ThreeDAssetsManagerController() override;

            QString title() const override
            {
                return "3D Assets";
            }
            QString nodeType() const override
            {
                return "ThreeDModelNode";
            }

            QVariantList availableAssets() const
            {
                return m_availableAssets;
            }
            Q_INVOKABLE QVariantList getAssets() const
            {
                return m_availableAssets;
            }

              public slots:
            void scanAssets();
            void createThreeDModelNode(const QString &modelPath);

              signals:
            void availableAssetsChanged();
            void assetsChanged();

              private:
            QString m_assetsDirectory;
            QVariantList m_availableAssets;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
