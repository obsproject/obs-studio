#include "GraphicsManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDir>
#include <QDebug>
#include <QDir>
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

GraphicsManagerController::GraphicsManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	m_graphicsDirectory = QDir::currentPath() + "/assets/graphics";
	scanGraphics();
}

GraphicsManagerController::~GraphicsManagerController() {}

void GraphicsManagerController::scanGraphics()
{
	m_availableGraphics.clear();
	QDir graphicsDir(m_graphicsDirectory);

	if (!graphicsDir.exists()) {
		qWarning() << "Graphics directory does not exist:" << m_graphicsDirectory;
		emit availableGraphicsChanged();
		emit graphicsChanged();
		return;
	}

	QStringList filters;
	filters << "*.svg" << "*.ai" << "*.eps";
	QFileInfoList files = graphicsDir.entryInfoList(filters, QDir::Files);

	for (const QFileInfo &fileInfo : files) {
		QVariantMap graphic;
		graphic["name"] = fileInfo.baseName();
		graphic["path"] = fileInfo.absoluteFilePath();
		graphic["format"] = fileInfo.suffix().toUpper();

		bool inUse = false;
		for (const QString &nodeId : managedNodes()) {
			if (getResourceForNode(nodeId) == fileInfo.absoluteFilePath()) {
				inUse = true;
				graphic["nodeId"] = nodeId;
				graphic["managed"] = true;
				break;
			}
		}
		graphic["inUse"] = inUse;

		m_availableGraphics.append(graphic);
	}

	qDebug() << "Found" << m_availableGraphics.size() << "graphics," << managedNodes().size() << "managed nodes";
	emit availableGraphicsChanged();
	emit graphicsChanged();
}

void GraphicsManagerController::createGraphicsNode(const QString &graphicPath)
{
	if (!m_graphController)
		return;

	QString nodeId = m_graphController->createNode("GraphicsNode", 0.0f, 0.0f);
	if (nodeId.isEmpty())
		return;

	m_graphController->setNodeProperty(nodeId, "graphicPath", graphicPath);

	QVariantMap metadata;
	metadata["graphicPath"] = graphicPath;
	trackNode(nodeId, graphicPath, metadata);

	scanGraphics();
}

} // namespace Blueprint
} // namespace NeuralStudio
