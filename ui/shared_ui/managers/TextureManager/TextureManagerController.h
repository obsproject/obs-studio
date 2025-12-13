#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class TextureManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableTextures READ availableTextures NOTIFY availableTexturesChanged)

              public:
            explicit TextureManagerController(QObject *parent = nullptr);
            ~TextureManagerController() override;

            QString title() const override
            {
                return "Textures";
            }
            QString nodeType() const override
            {
                return "TextureNode";
            }

            QVariantList availableTextures() const
            {
                return m_availableTextures;
            }
            Q_INVOKABLE QVariantList getTextures() const
            {
                return m_availableTextures;
            }

              public slots:
            void scanTextures();
            void createTextureNode(const QString &texturePath);

              signals:
            void availableTexturesChanged();
            void texturesChanged();

              private:
            QString m_texturesDirectory;
            QVariantList m_availableTextures;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
