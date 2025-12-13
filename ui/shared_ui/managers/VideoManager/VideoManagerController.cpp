#include "VideoManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDir>
#include <QDebug>
#include <QDir>
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

VideoManagerController::VideoManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	m_videosDirectory = QDir::currentPath() + "/assets/videos";
	scanVideos();
}

VideoManagerController::~VideoManagerController() {}

void VideoManagerController::scanVideos()
{
	m_availableVideos.clear();
	QDir videosDir(m_videosDirectory);

	if (!videosDir.exists()) {
		qWarning() << "Videos directory does not exist:" << m_videosDirectory;
		emit availableVideosChanged();
		emit videosChanged();
		return;
	}

	QStringList filters;
	filters << "*.mp4" << "*.avi" << "*.mov" << "*.mkv" << "*.webm";
	QFileInfoList files = videosDir.entryInfoList(filters, QDir::Files);

	for (const QFileInfo &fileInfo : files) {
		QVariantMap video;
		video["name"] = fileInfo.baseName();
		video["path"] = fileInfo.absoluteFilePath();
		video["size"] = fileInfo.size();
		video["format"] = fileInfo.suffix().toUpper();

		bool inUse = false;
		for (const QString &nodeId : managedNodes()) {
			if (getResourceForNode(nodeId) == fileInfo.absoluteFilePath()) {
				inUse = true;
				video["nodeId"] = nodeId;
				video["managed"] = true;
				break;
			}
		}
		video["inUse"] = inUse;

		m_availableVideos.append(video);
	}

	qDebug() << "Found" << m_availableVideos.size() << "videos," << managedNodes().size() << "managed nodes";
	emit availableVideosChanged();
	emit videosChanged();
}

void VideoManagerController::createVideoNode(const QString &videoPath)
{
	if (!m_graphController)
		return;

	QString nodeId = m_graphController->createNode("VideoNode", 0.0f, 0.0f);
	if (nodeId.isEmpty())
		return;

	m_graphController->setNodeProperty(nodeId, "videoPath", videoPath);

	QVariantMap metadata;
	metadata["videoPath"] = videoPath;
	trackNode(nodeId, videoPath, metadata);

	scanVideos();
	qDebug() << "VideoManager managing node:" << nodeId;
}

} // namespace Blueprint
} // namespace NeuralStudio
