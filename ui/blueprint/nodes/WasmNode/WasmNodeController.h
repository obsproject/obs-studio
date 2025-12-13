#pragma once

#include "../BaseNodeController.h"

namespace NeuralStudio {
    namespace UI {

        /**
     * @brief WasmNodeController
     * 
     * UI Controller for the WASM Node.
     * Inherits from BaseNodeController to integrate with the Blueprint Graph.
     */
        class WasmNodeController : public BaseNodeController
        {
            Q_OBJECT
            Q_PROPERTY(QString wasmPath READ wasmPath WRITE setWasmPath NOTIFY wasmPathChanged)
            Q_PROPERTY(QString entryFunction READ entryFunction WRITE setEntryFunction NOTIFY entryFunctionChanged)

              public:
            explicit WasmNodeController(QObject *parent = nullptr);
            ~WasmNodeController() override = default;

            QString wasmPath() const
            {
                return m_wasmPath;
            }
            void setWasmPath(const QString &path);

            QString entryFunction() const
            {
                return m_entryFunction;
            }
            void setEntryFunction(const QString &func);

              signals:
            void wasmPathChanged();
            void entryFunctionChanged();

              private:
            QString m_wasmPath;
            QString m_entryFunction = "process";
        };

    }  // namespace UI
}  // namespace NeuralStudio
