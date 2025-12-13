#pragma once
#include "../BaseManagerWidget.h"
#include "../MLManager/MLManagerController.h"
#include <QListWidget>
#include <QPushButton>

namespace NeuralStudio {
    namespace UI {

        class MLManagerWidget : public BaseManagerWidget
        {
            Q_OBJECT

              public:
            explicit MLManagerWidget(Blueprint::MLManagerController *controller, QWidget *parent = nullptr);
            ~MLManagerWidget() override;

            QWidget *createContent() override;

              public slots:
            void refresh() override;

              private slots:
            void onModelListChanged();
            void onAddModelClicked(const QString &modelPath);

              private:
            void setupConnections();
            void updateModelList();

            Blueprint::MLManagerController *m_controller;
            QListWidget *m_modelList;
            QPushButton *m_refreshButton;
        };

    }  // namespace UI
}  // namespace NeuralStudio
