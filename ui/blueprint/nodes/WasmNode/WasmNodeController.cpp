#include "WasmNodeController.h"
#include <QDebug>

namespace NeuralStudio {
namespace UI {

WasmNodeController::WasmNodeController(QObject *parent) : BaseNodeController(parent) {}

void WasmNodeController::setWasmPath(const QString &path)
{
	if (m_wasmPath != path) {
		m_wasmPath = path;
		emit wasmPathChanged();
		// Propagate change to backend
		// BaseNodeController might have a mechanism, or we emit a localized change
		// For now, we assume the GraphController binds to this property or we send an event.
		// TODO: Call backend update via graph controller if necessary
		emit propertyChanged("wasmPath", path);
	}
}

void WasmNodeController::setEntryFunction(const QString &func)
{
	if (m_entryFunction != func) {
		m_entryFunction = func;
		emit entryFunctionChanged();
		emit propertyChanged("entryFunction", func);
	}
}

} // namespace UI
} // namespace NeuralStudio
