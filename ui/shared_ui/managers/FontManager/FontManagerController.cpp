#include "FontManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDir>
#include <QDebug>
#include <QDir>
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

FontManagerController::FontManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	m_fontsDirectory = QDir::currentPath() + "/assets/fonts";
	scanFonts();
}

FontManagerController::~FontManagerController() {}

void FontManagerController::scanFonts()
{
	m_availableFonts.clear();
	QDir fontsDir(m_fontsDirectory);

	if (!fontsDir.exists()) {
		qWarning() << "Fonts directory does not exist:" << m_fontsDirectory;
		emit availableFontsChanged();
		emit fontsChanged();
		return;
	}

	QStringList filters;
	filters << "*.ttf" << "*.otf" << "*.woff" << "*.woff2";
	QFileInfoList files = fontsDir.entryInfoList(filters, QDir::Files);

	for (const QFileInfo &fileInfo : files) {
		QVariantMap font;
		font["name"] = fileInfo.baseName();
		font["path"] = fileInfo.absoluteFilePath();
		font["format"] = fileInfo.suffix().toUpper();

		bool inUse = false;
		for (const QString &nodeId : managedNodes()) {
			if (getResourceForNode(nodeId) == fileInfo.absoluteFilePath()) {
				inUse = true;
				font["nodeId"] = nodeId;
				font["managed"] = true;
				break;
			}
		}
		font["inUse"] = inUse;

		m_availableFonts.append(font);
	}

	qDebug() << "Found" << m_availableFonts.size() << "fonts," << managedNodes().size() << "managed nodes";
	emit availableFontsChanged();
	emit fontsChanged();
}

void FontManagerController::createFontNode(const QString &fontPath)
{
	if (!m_graphController)
		return;

	QString nodeId = m_graphController->createNode("FontNode", 0.0f, 0.0f);
	if (nodeId.isEmpty())
		return;

	m_graphController->setNodeProperty(nodeId, "fontPath", fontPath);

	QVariantMap metadata;
	metadata["fontPath"] = fontPath;
	trackNode(nodeId, fontPath, metadata);

	scanFonts();
}

} // namespace Blueprint
} // namespace NeuralStudio
