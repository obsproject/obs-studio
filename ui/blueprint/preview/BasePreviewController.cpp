#include "BasePreviewController.h"
#include <QDebug>

namespace NeuralStudio {
namespace UI {

BasePreviewController::BasePreviewController(QObject *parent) : QObject(parent) {}

void BasePreviewController::setActive(bool active)
{
	if (m_active != active) {
		m_active = active;
		emit activeChanged();
	}
}

void BasePreviewController::resize(int width, int height)
{
	if (m_size.width() != width || m_size.height() != height) {
		m_size = QSize(width, height);
		qDebug() << "Preview resized:" << width << "x" << height;
		// TODO: Notify backend renderer
	}
}

void BasePreviewController::orbit(float dx, float dy)
{
	// TODO: Forward to PreviewRenderer::rotateCamera(dx, dy)
	qDebug() << "Orbit:" << dx << dy;
}

void BasePreviewController::pan(float dx, float dy)
{
	// TODO: Forward to PreviewRenderer::panCamera(dx, dy)
	qDebug() << "Pan:" << dx << dy;
}

void BasePreviewController::zoom(float delta)
{
	// TODO: Forward to PreviewRenderer::zoomCamera(delta)
	qDebug() << "Zoom:" << delta;
}

} // namespace UI
} // namespace NeuralStudio
