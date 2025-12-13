#include "HeadsetOutputNodeController.h"

namespace NeuralStudio {
namespace UI {

HeadsetOutputNodeController::HeadsetOutputNodeController(QObject *parent) : BaseNodeController(parent) {}

void HeadsetOutputNodeController::setProfileId(const QString &id)
{
	if (m_profileId != id) {
		m_profileId = id;
		emit profileIdChanged();
		updateProperty("profile_id", id);
		// Also update backend node via BaseNodeController logic (future)
	}
}

} // namespace UI
} // namespace NeuralStudio
