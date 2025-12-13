#include "EffectsManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDebug>
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

EffectsManagerController::EffectsManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	loadPredefinedEffects();
}

EffectsManagerController::~EffectsManagerController() {}

void EffectsManagerController::loadPredefinedEffects()
{
	m_availableEffects.clear();

	// Predefined effects library
	QStringList effects = {"Blur",         "Sharpen", "Bloom",    "Vignette",   "ChromaticAberration",
			       "ColorGrading", "Glitch",  "Pixelate", "EdgeDetect", "Emboss"};

	for (const QString &effect : effects) {
		QVariantMap effectData;
		effectData["name"] = effect;
		effectData["type"] = effect;

		bool inUse = false;
		for (const QString &nodeId : managedNodes()) {
			if (getResourceForNode(nodeId) == effect) {
				inUse = true;
				effectData["nodeId"] = nodeId;
				effectData["managed"] = true;
				break;
			}
		}
		effectData["inUse"] = inUse;

		m_availableEffects.append(effectData);
	}

	qDebug() << "Loaded" << m_availableEffects.size() << "effects," << managedNodes().size() << "managed nodes";
	emit availableEffectsChanged();
	emit effectsChanged();
}

void EffectsManagerController::refreshEffects()
{
	loadPredefinedEffects();
}

void EffectsManagerController::createEffectNode(const QString &effectType)
{
	if (!m_graphController)
		return;

	QString nodeId = m_graphController->createNode("EffectNode", 0.0f, 0.0f);
	if (nodeId.isEmpty())
		return;

	m_graphController->setNodeProperty(nodeId, "effectType", effectType);

	QVariantMap metadata;
	metadata["effectType"] = effectType;
	trackNode(nodeId, effectType, metadata);

	refreshEffects();
}

} // namespace Blueprint
} // namespace NeuralStudio
