#pragma once
#include "../BaseManagerWidget.h"
#include "../LLMManager/LLMManagerController.h"
#include <QListWidget>
#include <QPushButton>

namespace NeuralStudio {
    namespace UI {

        class LLMManagerWidget : public BaseManagerWidget
        {
            Q_OBJECT

              public:
            explicit LLMManagerWidget(Blueprint::LLMManagerController *controller, QWidget *parent = nullptr);
            ~LLMManagerWidget() override;

            QWidget *createContent() override;

              public slots:
            void refresh() override;

              private slots:
            void onModelListChanged();
            void onAddModelClicked(const QString &modelId, const QString &provider);

              private:
            void setupConnections();
            void updateModelList();

            Blueprint::LLMManagerController *m_controller;
            QListWidget *m_modelList;
            QPushButton *m_refreshButton;
        };

    }  // namespace UI
}  // namespace NeuralStudio
