#pragma once

#include "../BaseNodeController.h"

namespace NeuralStudio {
    namespace UI {

        /**
     * @brief OnnxNodeController
     * 
     * UI Controller for the ONNX Inference Node.
     * Inherits from BaseNodeController.
     */
        class OnnxNodeController : public BaseNodeController
        {
            Q_OBJECT
            Q_PROPERTY(QString modelPath READ modelPath WRITE setModelPath NOTIFY modelPathChanged)
            Q_PROPERTY(bool useGpu READ useGpu WRITE setUseGpu NOTIFY useGpuChanged)

              public:
            explicit OnnxNodeController(QObject *parent = nullptr);
            ~OnnxNodeController() override = default;

            QString modelPath() const
            {
                return m_modelPath;
            }
            void setModelPath(const QString &path);

            bool useGpu() const
            {
                return m_useGpu;
            }
            void setUseGpu(bool use);

              signals:
            void modelPathChanged();
            void useGpuChanged();

              private:
            QString m_modelPath;
            bool m_useGpu = false;
        };

    }  // namespace UI
}  // namespace NeuralStudio
