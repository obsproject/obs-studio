#pragma once

#include "../BaseNodeController.h"

namespace NeuralStudio {
    namespace UI {

        /**
 * @brief RTXUpscaleNode UI Controller
 */
        class RTXUpscaleNodeController : public BaseNodeController
        {
            Q_OBJECT
            Q_PROPERTY(int qualityMode READ qualityMode WRITE setQualityMode NOTIFY qualityModeChanged)
            Q_PROPERTY(int scaleFactor READ scaleFactor WRITE setScaleFactor NOTIFY scaleFactorChanged)
            QML_ELEMENT

              public:
            explicit RTXUpscaleNodeController(QObject *parent = nullptr);
            ~RTXUpscaleNodeController() override = default;

            int qualityMode() const
            {
                return m_qualityMode;
            }
            void setQualityMode(int mode);

            int scaleFactor() const
            {
                return m_scaleFactor;
            }
            void setScaleFactor(int factor);

              signals:
            void qualityModeChanged();
            void scaleFactorChanged();

              private:
            int m_qualityMode = 1;
            int m_scaleFactor = 2;
        };

    }  // namespace UI
}  // namespace NeuralStudio
