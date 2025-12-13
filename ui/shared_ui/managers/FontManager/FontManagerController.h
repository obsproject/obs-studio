#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class FontManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableFonts READ availableFonts NOTIFY availableFontsChanged)

              public:
            explicit FontManagerController(QObject *parent = nullptr);
            ~FontManagerController() override;

            QString title() const override
            {
                return "Fonts";
            }
            QString nodeType() const override
            {
                return "FontNode";
            }

            QVariantList availableFonts() const
            {
                return m_availableFonts;
            }
            Q_INVOKABLE QVariantList getFonts() const
            {
                return m_availableFonts;
            }

              public slots:
            void scanFonts();
            void createFontNode(const QString &fontPath);

              signals:
            void availableFontsChanged();
            void fontsChanged();

              private:
            QString m_fontsDirectory;
            QVariantList m_availableFonts;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
