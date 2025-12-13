#include "ScriptManagerWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QVariantMap>

namespace NeuralStudio {
namespace UI {

ScriptManagerWidget::ScriptManagerWidget(Blueprint::ScriptManagerController *controller, QWidget *parent)
	: BaseManagerWidget(parent),
	  m_controller(controller),
	  m_scriptList(nullptr),
	  m_refreshButton(nullptr)
{
	setTitle("Scripts & Logic");
	setDescription("Lua and Python automation scripts");
	setupConnections();
}

ScriptManagerWidget::~ScriptManagerWidget() = default;

QWidget *ScriptManagerWidget::createContent()
{
	QWidget *content = new QWidget(this);
	QVBoxLayout *layout = new QVBoxLayout(content);
	layout->setSpacing(10);
	layout->setContentsMargins(0, 0, 0, 0);

	// Top bar
	QHBoxLayout *topBar = new QHBoxLayout();
	QLabel *countLabel = new QLabel("0 scripts", content);
	countLabel->setObjectName("countLabel");
	topBar->addWidget(countLabel);
	topBar->addStretch();

	m_refreshButton = new QPushButton("Refresh", content);
	connect(m_refreshButton, &QPushButton::clicked, this, &ScriptManagerWidget::refresh);
	topBar->addWidget(m_refreshButton);

	layout->addLayout(topBar);

	// Script list
	m_scriptList = new QListWidget(content);
	m_scriptList->setSpacing(5);
	layout->addWidget(m_scriptList, 1);

	updateScriptList();
	return content;
}

void ScriptManagerWidget::refresh()
{
	if (m_controller) {
		m_controller->scanScripts();
	}
}

void ScriptManagerWidget::setupConnections()
{
	if (m_controller) {
		connect(m_controller, &Blueprint::ScriptManagerController::scriptsChanged, this,
			&ScriptManagerWidget::onScriptListChanged);
	}
}

void ScriptManagerWidget::onScriptListChanged()
{
	updateScriptList();
}

void ScriptManagerWidget::updateScriptList()
{
	if (!m_controller || !m_scriptList)
		return;

	m_scriptList->clear();
	QVariantList scripts = m_controller->getScripts();

	if (QLabel *countLabel = findChild<QLabel *>("countLabel")) {
		countLabel->setText(QString("%1 scripts").arg(scripts.size()));
	}

	for (const QVariant &scriptVar : scripts) {
		QVariantMap script = scriptVar.toMap();
		QString name = script.value("name").toString();
		QString path = script.value("path").toString();
		QString language = script.value("language").toString();

		QWidget *itemWidget = new QWidget();
		QHBoxLayout *itemLayout = new QHBoxLayout(itemWidget);
		itemLayout->setContentsMargins(8, 8, 8, 8);

		// Language-specific icon
		QLabel *icon = new QLabel(language == "lua" ? "🌙" : "🐍");
		QString iconColor = language == "lua" ? "#00007C" : "#FFD43B";
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

		QLabel *langLabel = new QLabel(language.toUpper());
		langLabel->setStyleSheet("color: #888; font-size: 11px;");
		textLayout->addWidget(langLabel);

		itemLayout->addLayout(textLayout, 1);

		// Add button
		QPushButton *addBtn = new QPushButton("Add");
		addBtn->setFixedWidth(60);
		connect(addBtn, &QPushButton::clicked, this,
			[this, path, language]() { onAddScriptClicked(path, language); });
		itemLayout->addWidget(addBtn);

		QListWidgetItem *item = new QListWidgetItem(m_scriptList);
		item->setSizeHint(itemWidget->sizeHint());
		m_scriptList->addItem(item);
		m_scriptList->setItemWidget(item, itemWidget);
	}
}

void ScriptManagerWidget::onAddScriptClicked(const QString &scriptPath, const QString &language)
{
	if (m_controller) {
		m_controller->createScriptNode(scriptPath, language);
	}
}

} // namespace UI
} // namespace NeuralStudio
