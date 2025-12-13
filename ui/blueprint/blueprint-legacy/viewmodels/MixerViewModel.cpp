#include "MixerViewModel.h"
#include <QDebug>

namespace NeuralStudio {

MixerViewModel::MixerViewModel(QObject *parent) : QObject(parent), m_layoutType(LayoutType::Single)
{
	// Add mock feeds for testing
	addVideoFeed("camera1", "Main Camera", "camera://0");
	addVideoFeed("camera2", "Close-up Camera", "camera:// 1");
	addAudioSource("mic1", "Host Mic", QVector3D(-2, 0, -3000));
	addAudioSource("mic2", "Guest Mic", QVector3D(2, 0, -3000));
}

void MixerViewModel::addVideoFeed(const QString &id, const QString &label, const QString &source)
{
	VideoFeed feed;
	feed.id = id;
	feed.label = label;
	feed.source = source;
	feed.position = QVector3D(0, 0, -5000); // Default at video plane
	feed.active = false;
	feed.aspectRatio = 16.0f / 9.0f;

	m_videoFeeds.append(feed);

	// First feed is automatically active
	if (m_videoFeeds.size() == 1) {
		m_videoFeeds[0].active = true;
	}

	emit videoFeedsChanged();
}

void MixerViewModel::removeVideoFeed(const QString &id)
{
	for (int i = 0; i < m_videoFeeds.size(); ++i) {
		if (m_videoFeeds[i].id == id) {
			m_videoFeeds.removeAt(i);
			emit videoFeedsChanged();
			return;
		}
	}
}

void MixerViewModel::setActiveFeed(const QString &id)
{
	for (auto &feed : m_videoFeeds) {
		feed.active = (feed.id == id);
	}
	emit videoFeedsChanged();
	emit activeFeedChanged(id);
}

void MixerViewModel::updateVideoPosition(const QString &id, const QVector3D &position)
{
	for (auto &feed : m_videoFeeds) {
		if (feed.id == id) {
			feed.position = position;
			emit videoFeedsChanged();
			return;
		}
	}
}

void MixerViewModel::addAudioSource(const QString &id, const QString &label, const QVector3D &position)
{
	AudioSource source;
	source.id = id;
	source.label = label;
	source.position = position;
	source.level = 0.0f;
	source.muted = false;

	m_audioSources.append(source);
	emit audioSourcesChanged();
}

void MixerViewModel::removeAudioSource(const QString &id)
{
	for (int i = 0; i < m_audioSources.size(); ++i) {
		if (m_audioSources[i].id == id) {
			m_audioSources.removeAt(i);
			emit audioSourcesChanged();
			return;
		}
	}
}

void MixerViewModel::updateAudioLevel(const QString &id, float level)
{
	for (auto &source : m_audioSources) {
		if (source.id == id) {
			source.level = qBound(0.0f, level, 1.0f);
			emit audioSourcesChanged();
			return;
		}
	}
}

void MixerViewModel::updateAudioPosition(const QString &id, const QVector3D &position)
{
	for (auto &source : m_audioSources) {
		if (source.id == id) {
			source.position = position;
			emit audioSourcesChanged();
			return;
		}
	}
}

void MixerViewModel::setAudioMuted(const QString &id, bool muted)
{
	for (auto &source : m_audioSources) {
		if (source.id == id) {
			source.muted = muted;
			emit audioSourcesChanged();
			return;
		}
	}
}

void MixerViewModel::setLayoutType(int type)
{
	LayoutType newLayout = static_cast<LayoutType>(type);
	if (m_layoutType != newLayout) {
		m_layoutType = newLayout;
		emit layoutTypeChanged();
	}
}

QVariantList MixerViewModel::videoFeeds() const
{
	QVariantList list;
	for (const auto &feed : m_videoFeeds) {
		QVariantMap map;
		map["id"] = feed.id;
		map["label"] = feed.label;
		map["source"] = feed.source;
		map["active"] = feed.active;
		map["x"] = feed.position.x();
		map["y"] = feed.position.y();
		map["z"] = feed.position.z();
		list.append(map);
	}
	return list;
}

QVariantList MixerViewModel::audioSources() const
{
	QVariantList list;
	for (const auto &source : m_audioSources) {
		QVariantMap map;
		map["id"] = source.id;
		map["label"] = source.label;
		map["level"] = source.level;
		map["muted"] = source.muted;
		map["x"] = source.position.x();
		map["y"] = source.position.y();
		map["z"] = source.position.z();
		list.append(map);
	}
	return list;
}

} // namespace NeuralStudio
