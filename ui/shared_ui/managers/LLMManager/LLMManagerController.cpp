#include "LLMManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

LLMManagerController::LLMManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	loadPredefinedModels();
}

LLMManagerController::~LLMManagerController() {}

void LLMManagerController::loadPredefinedModels()
{
	m_availableModels.clear();

	// Gemini models
	QVariantMap geminiFlash;
	geminiFlash["name"] = "Gemini 2.0 Flash";
	geminiFlash["id"] = "gemini-2.0-flash-exp";
	geminiFlash["provider"] = "Google Gemini";
	m_availableModels.append(geminiFlash);

	QVariantMap geminiPro;
	geminiPro["name"] = "Gemini 1.5 Pro";
	geminiPro["id"] = "gemini-1.5-pro";
	geminiPro["provider"] = "Google Gemini";
	m_availableModels.append(geminiPro);

	// OpenAI models
	QVariantMap gpt4;
	gpt4["name"] = "GPT-4 Turbo";
	gpt4["id"] = "gpt-4-turbo";
	gpt4["provider"] = "OpenAI";
	m_availableModels.append(gpt4);

	QVariantMap gpt35;
	gpt35["name"] = "GPT-3.5 Turbo";
	gpt35["id"] = "gpt-3.5-turbo";
	gpt35["provider"] = "OpenAI";
	m_availableModels.append(gpt35);

	// Claude models
	QVariantMap claude3;
	claude3["name"] = "Claude 3 Opus";
	claude3["id"] = "claude-3-opus";
	claude3["provider"] = "Anthropic Claude";
	m_availableModels.append(claude3);

	// Mark which are in use
	for (QVariant &modelVar : m_availableModels) {
		QVariantMap model = modelVar.toMap();
		QString modelKey = model.value("provider").toString() + "/" + model.value("id").toString();

		bool inUse = false;
		for (const QString &nodeId : managedNodes()) {
			if (getResourceForNode(nodeId) == modelKey) {
				inUse = true;
				model["nodeId"] = nodeId;
				model["managed"] = true;
				break;
			}
		}
		model["inUse"] = inUse;
		modelVar = model;
	}

	qDebug() << "Loaded" << m_availableModels.size() << "LLM models," << managedNodes().size() << "managed nodes";
	emit availableModelsChanged();
	emit modelsChanged();
}

void LLMManagerController::refreshModels()
{
	loadPredefinedModels();
}

void LLMManagerController::createLLMNode(const QString &modelId, const QString &provider)
{
	if (!m_graphController) {
		qWarning() << "No graph controller set";
		return;
	}

	QString nodeId = m_graphController->createNode("LLMNode", 0.0f, 0.0f);
	if (nodeId.isEmpty()) {
		qWarning() << "Failed to create LLMNode";
		return;
	}

	m_graphController->setNodeProperty(nodeId, "model", modelId);
	m_graphController->setNodeProperty(nodeId, "provider", provider);

	// Track with composite key
	QString resourceKey = provider + "/" + modelId;
	QVariantMap metadata;
	metadata["modelId"] = modelId;
	metadata["provider"] = provider;
	trackNode(nodeId, resourceKey, metadata);

	refreshModels();
	qDebug() << "LLMManager created and is managing node:" << nodeId;
}

} // namespace Blueprint
} // namespace NeuralStudio
