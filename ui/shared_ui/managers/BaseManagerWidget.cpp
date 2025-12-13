#include "BaseManagerWidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

namespace NeuralStudio {
namespace UI {

BaseManagerWidget::BaseManagerWidget(QWidget *parent)
	: QWidget(parent),
	  m_mainLayout(nullptr),
	  m_contentWidget(nullptr),
	  m_titleLabel(nullptr),
	  m_descriptionLabel(nullptr),
	  m_contentFrame(nullptr)
{
	setupUI();
	applyDarkTheme();
}

BaseManagerWidget::~BaseManagerWidget() = default;

void BaseManagerWidget::setTitle(const QString &title)
{
	if (m_title != title) {
		m_title = title;
		if (m_titleLabel) {
			m_titleLabel->setText(title);
		}
		emit titleChanged();
	}
}

void BaseManagerWidget::setDescription(const QString &description)
{
	if (m_description != description) {
		m_description = description;
		if (m_descriptionLabel) {
			m_descriptionLabel->setText(description);
		}
		emit descriptionChanged();
	}
}

void BaseManagerWidget::setupUI()
{
	m_mainLayout = new QVBoxLayout(this);
	m_mainLayout->setContentsMargins(0, 0, 0, 0);
	m_mainLayout->setSpacing(0);

	// Title section
	m_titleLabel = new QLabel(m_title, this);
	m_titleLabel->setObjectName("managerTitle");
	m_mainLayout->addWidget(m_titleLabel);

	// Description section
	m_descriptionLabel = new QLabel(m_description, this);
	m_descriptionLabel->setObjectName("managerDescription");
	m_mainLayout->addWidget(m_descriptionLabel);

	// Content frame
	m_contentFrame = new QFrame(this);
	m_contentFrame->setObjectName("managerContent");
	QVBoxLayout *contentLayout = new QVBoxLayout(m_contentFrame);
	contentLayout->setContentsMargins(10, 10, 10, 10);

	// Create content from subclass
	m_contentWidget = createContent();
	if (m_contentWidget) {
		contentLayout->addWidget(m_contentWidget);
	}

	m_mainLayout->addWidget(m_contentFrame, 1);
}

void BaseManagerWidget::applyDarkTheme()
{
	// Dark theme stylesheet matching QML version
	QString styleSheet = R"(
        QWidget {
            background-color: #1E1E1E;
            color: #CCCCCC;
            font-family: 'Inter', 'Roboto', sans-serif;
        }
        
        QLabel#managerTitle {
            color: #FFFFFF;
            font-size: 16px;
            font-weight: bold;
            padding: 10px;
            background-color: #252525;
        }
        
        QLabel#managerDescription {
            color: #888888;
            font-size: 12px;
            padding: 5px 10px;
            background-color: #252525;
        }
        
        QFrame#managerContent {
            background-color: #1E1E1E;
            border-top: 1px solid #333333;
        }
        
        QPushButton {
            background-color: #2A2A2A;
            color: #CCCCCC;
            border: 1px solid #444444;
            border-radius: 4px;
            padding: 6px 12px;
            font-size: 12px;
        }
        
        QPushButton:hover {
            background-color: #333333;
            border-color: #555555;
        }
        
        QPushButton:pressed {
            background-color: #1A1A1A;
        }
        
        QListWidget {
            background-color: #1E1E1E;
            border: none;
            outline: none;
        }
        
        QListWidget::item {
            background-color: #1E1E1E;
            color: #CCCCCC;
            border: 1px solid #444444;
            border-radius: 4px;
            padding: 8px;
            margin: 2px;
        }
        
        QListWidget::item:hover {
            background-color: #2A2A2A;
        }
        
        QListWidget::item:selected {
            background-color: #333333;
        }
        
        QScrollBar:vertical {
            background-color: #1E1E1E;
            width: 12px;
            border-radius: 6px;
        }
        
        QScrollBar::handle:vertical {
            background-color: #444444;
            border-radius: 6px;
            min-height: 20px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: #555555;
        }
    )";

	setStyleSheet(styleSheet);
}

} // namespace UI
} // namespace NeuralStudio
