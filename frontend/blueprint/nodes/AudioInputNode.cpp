#include "AudioInputNode.h"

namespace NeuralStudio {

AudioInputNode::AudioInputNode(const QString &title, QGraphicsItem *parent)
	: NodeItem(title, parent),
	  m_sourceType(AudioSourceType::Microphone),
	  m_sampleRate(48000),
	  m_channels(1),
	  m_position3D(0, 0, -3000),
	  m_volume(1.0f),
	  m_gain(0.0f),
	  m_muted(false),
	  m_active(false)
{
	// Add output port for audio buffer
	addOutputPort("Audio Out", PortDataType::Audio);
	addOutputPort("Metadata", PortDataType::Data);
}

void AudioInputNode::setSourceType(AudioSourceType type)
{
	if (m_sourceType != type) {
		m_sourceType = type;
		emit sourceChanged();
		emitSpatialUpdate();
	}
}

void AudioInputNode::setSourcePath(const QString &path)
{
	if (m_sourcePath != path) {
		m_sourcePath = path;
		emit sourceChanged();
	}
}

void AudioInputNode::setSampleRate(int rate)
{
	if (m_sampleRate != rate) {
		m_sampleRate = rate;
		emit audioPropertiesChanged();
	}
}

void AudioInputNode::setChannels(int channels)
{
	if (m_channels != channels) {
		m_channels = channels;
		emit audioPropertiesChanged();
	}
}

void AudioInputNode::setPosition3D(const QVector3D &pos)
{
	if (m_position3D != pos) {
		m_position3D = pos;
		emit position3DChanged();
		emitSpatialUpdate();
	}
}

void AudioInputNode::setVolume(float volume)
{
	float clampedVol = qBound(0.0f, volume, 1.0f);
	if (m_volume != clampedVol) {
		m_volume = clampedVol;
		emit volumeChanged(m_volume);
		emitSpatialUpdate();
	}
}

void AudioInputNode::setGain(float gain)
{
	if (m_gain != gain) {
		m_gain = gain;
		emit audioPropertiesChanged();
	}
}

void AudioInputNode::setMuted(bool muted)
{
	if (m_muted != muted) {
		m_muted = muted;
		emit audioPropertiesChanged();
	}
}

void AudioInputNode::setActive(bool active)
{
	if (m_active != active) {
		m_active = active;
		emit activeStateChanged(active);
		update();
	}
}

} // namespace NeuralStudio
