#include "MLManagerWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QVariantMap>

namespace NeuralStudio {
namespace UI {

MLManagerWidget::MLManagerWidget(Blueprint::MLManagerController *controller, QWidget *parent)
	: BaseManagerWidget(parent),
	  m_controller(controller),
	  m_modelList(nullptr),
	  m_refreshButton(nullptr)
{
	setTitle("ML Models");
	setDescription("ONNX machine learning models");
	setupConnections();
}

MLManagerWidget::~MLManagerWidget() = default;

QWidget *MLManagerWidget::createContent()
{
	QWidget *content = new QWidget(this);
	QVBoxLayout *layout = new QVBoxLayout(content);
	layout->setSpacing(10);
	layout->setContentsMargins(0, 0, 0, 0);

	// Top bar with count and refresh button
	QHBoxLayout *topBar = new QHBoxLayout();
	QLabel *countLabel = new QLabel("0 models", content);
	countLabel->setObjectName("countLabel");
	topBar->addWidget(countLabel);
	topBar->addStretch();

	m_refreshButton = new QPushButton("Refresh", content);
	connect(m_refreshButton, &QPushButton::clicked, this, &MLManagerWidget::refresh);
	topBar->addWidget(m_refreshButton);

	layout->addLayout(topBar);

	// Model list
	m_modelList = new QListWidget(content);
	m_modelList->setSpacing(5);
	layout->addWidget(m_modelList, 1);

	// Initial load
	updateModelList();

	return content;
}

void MLManagerWidget::refresh()
{
	if (m_controller) {
		m_controller->scanModels();
	}
}

void MLManagerWidget::setupConnections()
{
	if (m_controller) {
		connect(m_controller, &Blueprint::MLManagerController::modelsChanged, this,
			&MLManagerWidget::onModelListChanged);
	}
}

void MLManagerWidget::onModelListChanged()
{
	updateModelList();
}

void MLManagerWidget::updateModelList()
{
	if (!m_controller || !m_modelList)
		return;

	m_modelList->clear();

	QVariantList models = m_controller->getModels();

	// Update count label
	if (QLabel *countLabel = findChild<QLabel *>("countLabel")) {
		countLabel->setText(QString("%1 models").arg(models.size()));
	}

	for (const QVariant &modelVar : models) {
		QVariantMap model = modelVar.toMap();
		QString name = model.value("name").toString();
		QString path = model.value("path").toString();
		qint64 size = model.value("size").toLongLong();

		// Create custom widget for list item
		QWidget *itemWidget = new QWidget();
		QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
		itemLayout->setContentsMargins(8, 8, 8, 8);

		// Icon
		QLabel *icon = new QLabel("🧠");
		icon->setStyleSheet("font-size: 18px; background-color: #00A67E; border-radius: 4px; padding: 6px;");
		icon->setFixedSize(32, 32);
		icon->setAlignment(Qt::AlignCenter);
		itemLayout->addWidget(icon);

		// Name and info
		QVBoxLayout *textLayout = new QVBoxLayout();
		textLayout->setSpacing(2);

		QLabel *nameLabel = new QLabel(name);
		nameLabel->setStyleSheet("color: #FFF; font-size: 14px;");
		textLayout->addWidget(nameLabel);

		QLabel *infoLabel = new QLabel(QString("ONNX - %1 MB").arg(size / 1024.0 / 1024.0, 0, 'f', 1));
		infoLabel->setStyleSheet("color: #888; font-size: 11px;");
		textLayout->addWidget(infoLabel);

		itemLayout->addLayout(textLayout, 1);

		// Add button
		QPushButton *addBtn = new QPushButton("Add");
		addBtn->setFixedWidth(60);
		connect(addBtn, &QPushButton::clicked, this, [this, path]() { onAddModelClicked(path); });
		itemLayout->addWidget(addBtn);

		QListWidgetItem *item = new QListWidgetItem(m_modelList);
		item->setSizeHint(itemWidget->sizeHint());
		m_modelList->addItem(item);
		m_modelList->setItemWidget(item, itemWidget);
	}
}

void MLManagerWidget::onAddModelClicked(const QString &modelPath)
{
	if (m_controller) {
		m_controller->createMLNode(modelPath);
	}
}

} // namespace UI
} // namespace NeuralStudio
