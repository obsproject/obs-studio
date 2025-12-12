#include "MixerRouter.h"
#include <QDebug>

namespace NeuralStudio {

MixerRouter::MixerRouter(QObject *parent) : QObject(parent) {}

MixerRouter::~MixerRouter() {}

void MixerRouter::addVideoFeed(const VideoFeedData &data)
{
	if (data.id.isEmpty()) {
		qWarning() << "MixerRouter: Cannot add video feed with empty ID";
		return;
	}

	if (m_videoFeeds.contains(data.id)) {
		qWarning() << "MixerRouter: Video feed already exists:" << data.id;
		return;
	}

	m_videoFeeds[data.id] = data;

	qDebug() << "MixerRouter: Added video feed:" << data.id << data.label;
	emit videoFeedAdded(data.id);
}

void MixerRouter::updateVideoFeed(const VideoFeedData &data)
{
	if (!m_videoFeeds.contains(data.id)) {
		qWarning() << "MixerRouter: Cannot update non-existent video feed:" << data.id;
		return;
	}

	m_videoFeeds[data.id] = data;

	emit videoFeedUpdated(data.id);
}

void MixerRouter::removeVideoFeed(const QString &feedId)
{
	if (!m_videoFeeds.contains(feedId)) {
		return;
	}

	m_videoFeeds.remove(feedId);

	qDebug() << "MixerRouter: Removed video feed:" << feedId;
	emit videoFeedRemoved(feedId);
}

bool MixerRouter::hasVideoFeed(const QString &feedId) const
{
	return m_videoFeeds.contains(feedId);
}

void MixerRouter::addAudioSource(const AudioSourceData &data)
{
	if (data.id.isEmpty()) {
		qWarning() << "MixerRouter: Cannot add audio source with empty ID";
		return;
	}

	if (m_audioSources.contains(data.id)) {
		qWarning() << "MixerRouter: Audio source already exists:" << data.id;
		return;
	}

	m_audioSources[data.id] = data;

	qDebug() << "MixerRouter: Added audio source:" << data.id << data.label;
	emit audioSourceAdded(data.id);
}

void MixerRouter::updateAudioSource(const AudioSourceData &data)
{
	if (!m_audioSources.contains(data.id)) {
		qWarning() << "MixerRouter: Cannot update non-existent audio source:" << data.id;
		return;
	}

	m_audioSources[data.id] = data;

	emit audioSourceUpdated(data.id);
}

void MixerRouter::removeAudioSource(const QString &sourceId)
{
	if (!m_audioSources.contains(sourceId)) {
		return;
	}

	m_audioSources.remove(sourceId);

	qDebug() << "MixerRouter: Removed audio source:" << sourceId;
	emit audioSourceRemoved(sourceId);
}

bool MixerRouter::hasAudioSource(const QString &sourceId) const
{
	return m_audioSources.contains(sourceId);
}

QVector<VideoFeedData> MixerRouter::getVideoFeeds() const
{
	return m_videoFeeds.values().toVector();
}

QVector<AudioSourceData> MixerRouter::getAudioSources() const
{
	return m_audioSources.values().toVector();
}

QVector<QString> MixerRouter::getVideoFeedIds() const
{
	return m_videoFeeds.keys().toVector();
}

QVector<QString> MixerRouter::getAudioSourceIds() const
{
	return m_audioSources.keys().toVector();
}

VideoFeedData MixerRouter::getVideoFeed(const QString &feedId) const
{
	return m_videoFeeds.value(feedId);
}

AudioSourceData MixerRouter::getAudioSource(const QString &sourceId) const
{
	return m_audioSources.value(sourceId);
}

} // namespace NeuralStudio
