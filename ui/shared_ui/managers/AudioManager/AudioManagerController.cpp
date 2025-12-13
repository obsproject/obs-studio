#include "AudioManagerController.h"
#include "../../blueprint/core/NodeGraphController.h"
#include <QDir>
#include <QDebug>
#include <QDir>
#include <QDebug>

namespace NeuralStudio {
namespace Blueprint {

AudioManagerController::AudioManagerController(QObject *parent) : UI::BaseManagerController(parent)
{
	m_audioDirectory = QDir::currentPath() + "/assets/audio";
	scanAudio();
}

AudioManagerController::~AudioManagerController() {}

void AudioManagerController::scanAudio()
{
	m_availableAudio.clear();
	QDir audioDir(m_audioDirectory);

	if (!audioDir.exists()) {
		qWarning() << "Audio directory does not exist:" << m_audioDirectory;
		emit availableAudioChanged();
		emit audioChanged();
		return;
	}

	QStringList filters;
	filters << "*.wav" << "*.mp3" << "*.ogg" << "*.flac" << "*.aac";
	QFileInfoList files = audioDir.entryInfoList(filters, QDir::Files);

	for (const QFileInfo &fileInfo : files) {
		QVariantMap audio;
		audio["name"] = fileInfo.baseName();
		audio["path"] = fileInfo.absoluteFilePath();
		audio["size"] = fileInfo.size();
		audio["format"] = fileInfo.suffix().toUpper();

		bool inUse = false;
		for (const QString &nodeId : managedNodes()) {
			if (getResourceForNode(nodeId) == fileInfo.absoluteFilePath()) {
				inUse = true;
				audio["nodeId"] = nodeId;
				audio["managed"] = true;
				break;
			}
		}
		audio["inUse"] = inUse;

		m_availableAudio.append(audio);
	}

	qDebug() << "Found" << m_availableAudio.size() << "audio files," << managedNodes().size() << "managed nodes";
	emit availableAudioChanged();
	emit audioChanged();
}

void AudioManagerController::createAudioNode(const QString &audioPath)
{
	if (!m_graphController)
		return;

	QString nodeId = m_graphController->createNode("AudioNode", 0.0f, 0.0f);
	if (nodeId.isEmpty())
		return;

	m_graphController->setNodeProperty(nodeId, "audioPath", audioPath);

	QVariantMap metadata;
	metadata["audioPath"] = audioPath;
	trackNode(nodeId, audioPath, metadata);

	scanAudio();
	qDebug() << "AudioManager managing node:" << nodeId;
}

} // namespace Blueprint
} // namespace NeuralStudio
