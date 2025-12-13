#pragma once

#include <QObject>
#include <QString>

namespace NeuralStudio {
    namespace UI {

        /**
     * @brief Global View Controller
     * 
     * Manages the high-level UI state of the application, specifically the transition
     * between the Traditional (Legacy/OBS-like) interface and the Blueprint (Node-based) interface.
     */
        class GlobalViewController : public QObject
        {
            Q_OBJECT
            Q_PROPERTY(ViewMode currentMode READ currentMode WRITE setMode NOTIFY modeChanged)

              public:
            enum class ViewMode {
                Traditional,  // Classic OBS-like interface
                Blueprint     // Node-based interface
            };
            Q_ENUM(ViewMode)

            explicit GlobalViewController(QObject *parent = nullptr);
            virtual ~GlobalViewController() = default;

            ViewMode currentMode() const
            {
                return m_currentMode;
            }

            Q_INVOKABLE void setMode(ViewMode mode);
            Q_INVOKABLE void toggleMode();

              signals:
            void modeChanged(ViewMode newMode);

              private:
            ViewMode m_currentMode = ViewMode::Traditional;
        };

    }  // namespace UI
}  // namespace NeuralStudio
