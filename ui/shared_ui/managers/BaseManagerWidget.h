#pragma once
#include <QWidget>
#include <QString>

class QVBoxLayout;
class QLabel;
class QFrame;

namespace NeuralStudio {
    namespace UI {

        /**
 * @brief Base class for all manager widgets
 * 
 * Provides common UI structure: title bar, description, and content area.
 * Subclasses override createContent() to provide their specific UI.
 */
        class BaseManagerWidget : public QWidget
        {
            Q_OBJECT
            Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
            Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)

              public:
            explicit BaseManagerWidget(QWidget *parent = nullptr);
            virtual ~BaseManagerWidget();

            QString title() const
            {
                return m_title;
            }

            QString description() const
            {
                return m_description;
            }

            QStringList managedNodes() const;
            QStringList nodePresets() const;

            // Virtual method for subclasses to create their content
            virtual QWidget *createContent() = 0;

              public slots:
            virtual void refresh() {}

              signals:
            void titleChanged();
            void descriptionChanged();

              protected:
            void setupUI();
            void applyDarkTheme();

            QVBoxLayout *m_mainLayout;
            QWidget *m_contentWidget;

              private:
            QString m_title;
            QString m_description;
            QLabel *m_titleLabel;
            QLabel *m_descriptionLabel;
            QFrame *m_contentFrame;
        };

    }  // namespace UI
}  // namespace NeuralStudio
