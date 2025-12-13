#pragma once
#include "../BaseManagerController.h"
#include <QStringList>
#include <QVariantList>

namespace NeuralStudio {
    namespace Blueprint {

        class GraphicsManagerController : public UI::BaseManagerController
        {
            Q_OBJECT
            Q_PROPERTY(QVariantList availableGraphics READ availableGraphics NOTIFY availableGraphicsChanged)

              public:
            explicit GraphicsManagerController(QObject *parent = nullptr);
            ~GraphicsManagerController() override;

            QString title() const override
            {
                return "Graphics";
            }
            QString nodeType() const override
            {
                return "GraphicsNode";
            }

            QVariantList availableGraphics() const
            {
                return m_availableGraphics;
            }
            Q_INVOKABLE QVariantList getGraphics() const
            {
                return m_availableGraphics;
            }

              public slots:
            void scanGraphics();
            void createGraphicsNode(const QString &graphicPath);

              signals:
            void availableGraphicsChanged();
            void graphicsChanged();

              private:
            QString m_graphicsDirectory;
            QVariantList m_availableGraphics;
        };

    }  // namespace Blueprint
}  // namespace NeuralStudio
