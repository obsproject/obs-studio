#include "TextureManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDir>
#include <QDebug>
#include <QDir>
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

TextureManagerController::TextureManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	m_texturesDirectory = QDir::currentPath() + "/assets/textures";
	scanTextures();
}

TextureManagerController::~TextureManagerController() {}

void TextureManagerController::scanTextures()
{
	m_availableTextures.clear();
	QDir texturesDir(m_texturesDirectory);

	if (!texturesDir.exists()) {
		qWarning() << "Textures directory does not exist:" << m_texturesDirectory;
		emit availableTexturesChanged();
		emit texturesChanged();
		return;
	}

	QStringList filters;
	filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp" << "*.tga" << "*.dds";
	QFileInfoList files = texturesDir.entryInfoList(filters, QDir::Files);

	for (const QFileInfo &fileInfo : files) {
		QVariantMap texture;
		texture["name"] = fileInfo.baseName();
		texture["path"] = fileInfo.absoluteFilePath();
		texture["size"] = fileInfo.size();
		texture["format"] = fileInfo.suffix().toUpper();

		bool inUse = false;
		for (const QString &nodeId : managedNodes()) {
			if (getResourceForNode(nodeId) == fileInfo.absoluteFilePath()) {
				inUse = true;
				texture["nodeId"] = nodeId;
				texture["managed"] = true;
				break;
			}
		}
		texture["inUse"] = inUse;

		m_availableTextures.append(texture);
	}

	qDebug() << "Found" << m_availableTextures.size() << "textures," << managedNodes().size() << "managed nodes";
	emit availableTexturesChanged();
	emit texturesChanged();
}

void TextureManagerController::createTextureNode(const QString &texturePath)
{
	if (!m_graphController)
		return;

	QString nodeId = m_graphController->createNode("TextureNode", 0.0f, 0.0f);
	if (nodeId.isEmpty())
		return;

	m_graphController->setNodeProperty(nodeId, "texturePath", texturePath);

	QVariantMap metadata;
	metadata["texturePath"] = texturePath;
	trackNode(nodeId, texturePath, metadata);

	scanTextures();
}

} // namespace Blueprint
} // namespace NeuralStudio
