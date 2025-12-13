#include "GlobalViewController.h"
#include <QDebug>

namespace NeuralStudio {
namespace UI {

GlobalViewController::GlobalViewController(QObject *parent) : QObject(parent) {}

void GlobalViewController::setMode(ViewMode mode)
{
	if (m_currentMode != mode) {
		m_currentMode = mode;
		qInfo() << "Global View Mode switched to:"
			<< (mode == ViewMode::Traditional ? "Traditional" : "Blueprint");
		emit modeChanged(m_currentMode);
	}
}

void GlobalViewController::toggleMode()
{
	if (m_currentMode == ViewMode::Traditional)
		setMode(ViewMode::Blueprint);
	else
		setMode(ViewMode::Traditional);
}

} // namespace UI
} // namespace NeuralStudio
