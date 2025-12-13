#include "StitchNodeController.h"

namespace NeuralStudio {
namespace UI {

StitchNodeController::StitchNodeController(QObject *parent) : BaseNodeController(parent) {}

void StitchNodeController::setStmapPath(const QString &path)
{
	if (m_stmapPath != path) {
		m_stmapPath = path;
		emit stmapPathChanged();
		updateProperty("stmap_path", path);
	}
}

} // namespace UI
} // namespace NeuralStudio
