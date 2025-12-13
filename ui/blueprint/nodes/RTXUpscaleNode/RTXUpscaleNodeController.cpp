#include "RTXUpscaleNodeController.h"

namespace NeuralStudio {
namespace UI {

RTXUpscaleNodeController::RTXUpscaleNodeController(QObject *parent) : BaseNodeController(parent) {}

void RTXUpscaleNodeController::setQualityMode(int mode)
{
	if (m_qualityMode != mode) {
		m_qualityMode = mode;
		emit qualityModeChanged();
		updateProperty("quality_mode", mode);
	}
}

void RTXUpscaleNodeController::setScaleFactor(int factor)
{
	if (m_scaleFactor != factor) {
		m_scaleFactor = factor;
		emit scaleFactorChanged();
		updateProperty("scale_factor", factor);
	}
}

} // namespace UI
} // namespace NeuralStudio
