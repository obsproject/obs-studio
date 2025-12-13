#include "LLMManagerWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QVariantMap>

namespace NeuralStudio {
namespace UI {

LLMManagerWidget::LLMManagerWidget(Blueprint::LLMManagerController *controller, QWidget *parent)
	: BaseManagerWidget(parent),
	  m_controller(controller),
	  m_modelList(nullptr),
	  m_refreshButton(nullptr)
{
	setTitle("LLM Services");
	setDescription("Cloud AI language models");
	setupConnections();
}

LLMManagerWidget::~LLMManagerWidget() = default;

QWidget *LLMManagerWidget::createContent()
{
	QWidget *content = new QWidget(this);
	QVBoxLayout *layout = new QVBoxLayout(content);
	layout->setSpacing(10);
	layout->setContentsMargins(0, 0, 0, 0);

	// Top bar
	QHBoxLayout *topBar = new QHBoxLayout();
	QLabel *countLabel = new QLabel("0 models", content);
	countLabel->setObjectName("countLabel");
	topBar->addWidget(countLabel);
	topBar->addStretch();

	m_refreshButton = new QPushButton("Refresh", content);
	connect(m_refreshButton, &QPushButton::clicked, this, &LLMManagerWidget::refresh);
	topBar->addWidget(m_refreshButton);

	layout->addLayout(topBar);

	// Model list
	m_model List = new QListWidget(content);
	m_modelList->setSpacing(5);
	layout->addWidget(m_modelList, 1);

	updateModelList();
	return content;
}

void LLMManagerWidget::refresh()
{
	if (m_controller) {
		m_controller->refreshModels();
	}
}

void LLMManagerWidget::setupConnections()
{
	if (m_controller) {
		connect(m_controller, &Blueprint::LLMManagerController::modelsChanged, this,
			&LLMManagerWidget::onModelListChanged);
	}
}

void LLMManagerWidget::onModelListChanged()
{
	updateModelList();
}

void LLMManagerWidget::updateModelList()
{
	if (!m_controller || !m_modelList)
		return;

	m_modelList->clear();
	QVariantList models = m_controller->getModels();

	if (QLabel *countLabel = findChild<QLabel *>("countLabel")) {
		countLabel->setText(QString("%1 models").arg(models.size()));
	}

	for (const QVariant &modelVar : models) {
		QVariantMap model = modelVar.toMap();
		QString name = model.value("name").toString();
		QString modelId = model.value("id").toString();
		QString provider = model.value("provider").toString();

		QWidget *itemWidget = new QWidget();
		QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
		itemLayout->setContentsMargins(8, 8, 8, 8);

		// Provider-specific icon and color
		QString iconEmoji;
		QString iconColor;
		if (provider.toLower().contains("gemini")) {
			iconEmoji = "💎";
			iconColor = "#4285F4"; // Google Blue
		} else if (provider.toLower().contains("gpt") || provider.toLower().contains("openai")) {
			iconEmoji = "🤖";
			iconColor = "#10A37F"; // OpenAI Green
		} else if (provider.toLower().contains("claude")) {
			iconEmoji = "🧠";
			iconColor = "#D97757"; // Claude Orange
		} else {
			iconEmoji = "🔮";
			iconColor = "#9B59B6"; // Generic Purple
		}

		QLabel *icon = new QLabel(iconEmoji);
		icon->setStyleSheet(QString("font-size: 18px; background-color: %1; border-radius: 4px; padding: 6px;")
					    .arg(iconColor));
		icon->setFixedSize(32, 32);
		icon->setAlignment(Qt::AlignCenter);
		itemLayout->addWidget(icon);

		// Text info
		QVBoxLayout *textLayout = new QVBoxLayout();
		textLayout->setSpacing(2);

		QLabel *nameLabel = new QLabel(name);
		nameLabel->setStyleSheet("color: #FFF; font-size: 14px;");
		textLayout->addWidget(nameLabel);

		QLabel *providerLabel = new QLabel(provider);
		providerLabel->setStyleSheet("color: #888; font-size: 11px;");
		textLayout->addWidget(providerLabel);

		itemLayout->addLayout(textLayout, 1);

		// Add button
		QPushButton *addBtn = new QPushButton("Add");
		addBtn->setFixedWidth(60);
		connect(addBtn, &QPushButton::clicked, this,
			[this, modelId, provider]() { onAddModelClicked(modelId, provider); });
		itemLayout->addWidget(addBtn);

		QListWidgetItem *item = new QListWidgetItem(m_modelList);
		item->setSizeHint(itemWidget->sizeHint());
		m_modelList->addItem(item);
		m_modelList->setItemWidget(item, itemWidget);
	}
}

void LLMManagerWidget::onAddModelClicked(const QString &modelId, const QString &provider)
{
	if (m_controller) {
		m_controller->createLLMNode(modelId, provider);
	}
}

} // namespace UI
} // namespace NeuralStudio
