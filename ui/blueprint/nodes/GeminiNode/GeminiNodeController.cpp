#include "GeminiNodeController.h"

namespace NeuralStudio {
namespace UI {

GeminiNodeController::GeminiNodeController(QObject *parent) : BaseNodeController(parent) {}

void GeminiNodeController::setApiKey(const QString &key)
{
	if (m_apiKey != key) {
		m_apiKey = key;
		emit apiKeyChanged();
		emit propertyChanged("apiKey", key);
	}
}

void GeminiNodeController::setModelName(const QString &name)
{
	if (m_modelName != name) {
		m_modelName = name;
		emit modelNameChanged();
		emit propertyChanged("modelName", name);
	}
}

} // namespace UI
} // namespace NeuralStudio
