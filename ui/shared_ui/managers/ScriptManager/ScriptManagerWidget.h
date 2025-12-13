#pragma once
#include "../BaseManagerWidget.h"
#include "../ScriptManager/ScriptManagerController.h"
#include <QListWidget>
#include <QPushButton>

namespace NeuralStudio {
    namespace UI {

        class ScriptManagerWidget : public BaseManagerWidget
        {
            Q_OBJECT

              public:
            explicit ScriptManagerWidget(Blueprint::ScriptManagerController *controller, QWidget *parent = nullptr);
            ~ScriptManagerWidget() override;

            QWidget *createContent() override;

              public slots:
            void refresh() override;

              private slots:
            void onScriptListChanged();
            void onAddScriptClicked(const QString &scriptPath, const QString &language);

              private:
            void setupConnections();
            void updateScriptList();

            Blueprint::ScriptManagerController *m_controller;
            QListWidget *m_scriptList;
            QPushButton *m_refreshButton;
        };

    }  // namespace UI
}  // namespace NeuralStudio
